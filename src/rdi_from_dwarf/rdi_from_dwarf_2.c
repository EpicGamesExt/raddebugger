// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Main Conversion Entry Point (New)

internal RDIM_BakeParams
d2r2_convert(Arena *arena, D2R2_ConvertParams *params)
{
  Temp scratch = scratch_begin(&arena, 1);
  DW_Raw *raw = &params->raw;
  RDIM_BinarySectionList binary_sections = params->binary_sections;
  Arch arch = params->arch;
  U64 base_vaddr = params->base_vaddr;
  
  ////////////////////////////
  //- rjf: determine acceptable address range
  //
  // in many cases, linkers seem to trample over addresses in various DWARF sections,
  // potentially due to optimizations. we'd like to filter out those busted addresses
  // from our final debug info - a good enough heuristic is to disqualify them by
  // whether or not they actually fall into the ranges covered by the binary sections.
  //
  Rng1U64 acceptable_vaddr_range = {0};
  {
    acceptable_vaddr_range.min = max_U64;
    acceptable_vaddr_range.max = 0;
    for EachNode(n, RDIM_BinarySectionNode, binary_sections.first)
    {
      acceptable_vaddr_range.min = Min(base_vaddr + n->v.voff_first, acceptable_vaddr_range.min);
      acceptable_vaddr_range.max = Max(base_vaddr + n->v.voff_opl, acceptable_vaddr_range.max);
    }
  }
  
  ////////////////////////////
  //- rjf: compute exe hash
  //
  U64 exe_hash = 0;
  ProfScope("compute exe hash")
  {
    if(lane_idx() == 0)
    {
      exe_hash = rdi_hash(params->exe_data.str, params->exe_data.size);
    }
    lane_sync_u64(&exe_hash, 0);
  }
  
  ////////////////////////////
  //- rjf: produce top-level-info
  //
  RDIM_TopLevelInfo top_level_info = {0};
  ProfScope("produce top-level-info")
  {
    // rjf: base arch -> rdi
    RDI_Arch arch_rdi = RDI_Arch_NULL;
    switch(arch)
    {
      case Arch_Null:
      case Arch_arm64:
      case Arch_arm32:
      case Arch_COUNT:
      {}break;
      case Arch_x64:{arch_rdi = RDI_Arch_X64;}break;
      case Arch_x86:{arch_rdi = RDI_Arch_X86;}break;
    }
    
    // rjf: fill
    top_level_info.arch     = arch_rdi;
    top_level_info.exe_name = params->exe_name;
    top_level_info.exe_hash = exe_hash;
    top_level_info.voff_max = acceptable_vaddr_range.max - base_vaddr;
    if(!params->deterministic)
    {
      // TODO(rjf): top_level_info.guid = ...;
      top_level_info.producer_name = str8_lit(BUILD_TITLE_STRING_LITERAL);
    }
  }
  
  ////////////////////////////
  //- rjf: gather unit ranges from .debug_info
  //
  Rng1U64Array *unit_info_ranges = 0;
  ProfScope("gather unit ranges from .debug_info") if(lane_idx() == 0)
  {
    Temp scratch2 = scratch_begin(&scratch.arena, 1);
    String8 data = raw->sec[DW_Section_Info].data;
    Rng1U64List unit_info_ranges_list = {0};
    for(U64 off = 0; off < data.size;)
    {
      U64 start_off = off;
      
      // rjf: read next unit info size
      U64 unit_info_size = 0;
      U64 unit_info_size_size = dw2_read_initial_length(data, off, &unit_info_size, 0);
      
      // rjf: push
      if(unit_info_size > 0)
      {
        rng1u64_list_push(scratch2.arena, &unit_info_ranges_list, r1u64(off, off + unit_info_size_size + unit_info_size));
      }
      
      // rjf: advance
      off += unit_info_size_size;
      off += unit_info_size;
      
      // rjf: break if no movement
      if(off == start_off)
      {
        break;
      }
    }
    unit_info_ranges = push_array(scratch.arena, Rng1U64Array, 1);
    unit_info_ranges[0] = rng1u64_array_from_list(scratch.arena, &unit_info_ranges_list);
    scratch_end(scratch2);
  }
  lane_sync_u64(&unit_info_ranges, 0);
  U64 unit_count = unit_info_ranges->count;
  
  ////////////////////////////
  //- rjf: parse all unit headers
  //
  DW2_UnitHeader *unit_headers = 0;
  Rng1U64 *unit_info_tag_ranges = 0;
  ProfScope("parse all unit headers")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      unit_headers = push_array(scratch.arena, DW2_UnitHeader, unit_count);
      unit_info_tag_ranges = push_array(scratch.arena, Rng1U64, unit_count);
    }
    lane_sync_u64(&unit_headers, 0);
    lane_sync_u64(&unit_info_tag_ranges, 0);
    
    // rjf: parse all unit headers
    String8 data = raw->sec[DW_Section_Info].data;
    Rng1U64 range = lane_range(unit_count);
    for EachInRange(idx, range)
    {
      Rng1U64 unit_info_range = unit_info_ranges->v[idx];
      U64 bytes_read = dw2_read_unit_header(str8_substr(data, unit_info_range), 0, &unit_headers[idx]);
      unit_info_tag_ranges[idx] = r1u64(unit_info_range.min + bytes_read, unit_info_range.max);
    }
  }
  lane_sync();
  
  ////////////////////////////
  //- rjf: build all abbreviation maps, build (unit -> abbrev map)
  //
  U64 abbrev_map_count = 0;
  U64 *abbrev_map_off_from_idx_table = 0;
  DW2_AbbrevMap *abbrev_map_from_idx_table = 0;
  DW2_AbbrevMap **abbrev_map_from_unit_idx_table = 0;
  ProfScope("build all abbreviation maps, build (unit -> abbrev map)")
  {
    //- rjf: gather deduplicated .debug_abbrev offsets, representing beginnings
    // of abbreviation tables. we want to do this because many units may refer
    // to the same abbreviation table.
    //
    typedef struct AbbrevOffNode AbbrevOffNode;
    struct AbbrevOffNode
    {
      AbbrevOffNode *order_next;
      AbbrevOffNode *hash_next;
      U64 off;
      U64 idx;
    };
    U64 abbrev_off_slots_count = unit_count;
    AbbrevOffNode **abbrev_off_slots = 0;
    ProfScope("gather deduplicated .debug_abbrev offsets") if(lane_idx() == 0)
    {
      AbbrevOffNode *abbrev_off_first = 0;
      AbbrevOffNode *abbrev_off_last = 0;
      abbrev_off_slots = push_array(scratch.arena, AbbrevOffNode *, abbrev_off_slots_count);
      for EachIndex(unit_idx, unit_count)
      {
        U64 abbrev_off = unit_headers[unit_idx].abbrev_off;
        U64 hash = u64_hash_from_str8(str8_struct(&abbrev_off));
        U64 slot_idx = hash%abbrev_off_slots_count;
        AbbrevOffNode *node = 0;
        for(AbbrevOffNode *n = abbrev_off_slots[slot_idx]; n != 0; n = n->hash_next)
        {
          if(n->off == abbrev_off)
          {
            node = n;
            break;
          }
        }
        if(node == 0)
        {
          node = push_array(scratch.arena, AbbrevOffNode, 1);
          SLLStackPush_N(abbrev_off_slots[slot_idx], node, hash_next);
          SLLQueuePush_N(abbrev_off_first, abbrev_off_last, node, order_next);
          node->off = abbrev_off;
          node->idx = abbrev_map_count;
          abbrev_map_count += 1;
        }
      }
      abbrev_map_off_from_idx_table = push_array(scratch.arena, U64, abbrev_map_count);
      abbrev_map_from_idx_table = push_array(scratch.arena, DW2_AbbrevMap, abbrev_map_count);
      abbrev_map_from_unit_idx_table = push_array(scratch.arena, DW2_AbbrevMap *, unit_count);
      {
        U64 abbrev_map_idx = 0;
        for(AbbrevOffNode *n = abbrev_off_first; n != 0; n = n->order_next)
        {
          abbrev_map_off_from_idx_table[abbrev_map_idx] = n->off;
          abbrev_map_idx += 1;
        }
      }
    }
    lane_sync_u64(&abbrev_off_slots, 0);
    lane_sync_u64(&abbrev_map_count, 0);
    lane_sync_u64(&abbrev_map_off_from_idx_table, 0);
    lane_sync_u64(&abbrev_map_from_idx_table, 0);
    lane_sync_u64(&abbrev_map_from_unit_idx_table, 0);
    
    //- rjf: build all unique abbreviation maps
    ProfScope("build all unique abbreviation maps")
    {
      String8 abbrev_data = raw->sec[DW_Section_Abbrev].data;
      U64 abbrev_map_take_idx = 0;
      U64 *abbrev_map_take_idx_ptr = &abbrev_map_take_idx;
      lane_sync_u64(&abbrev_map_take_idx_ptr, 0);
      for(;;)
      {
        U64 abbrev_map_idx = ins_atomic_u64_inc_eval(abbrev_map_take_idx_ptr) - 1;
        if(abbrev_map_idx >= abbrev_map_count)
        {
          break;
        }
        abbrev_map_from_idx_table[abbrev_map_idx] = dw2_abbrev_map_from_data(scratch.arena, abbrev_data, abbrev_map_off_from_idx_table[abbrev_map_idx]);
      }
      lane_sync();
    }
    
    //- rjf: build unit -> abbrev map table
    ProfScope("build unit -> abbrev map table")
    {
      Rng1U64 range = lane_range(unit_count);
      for EachInRange(unit_idx, range)
      {
        U64 abbrev_off = unit_headers[unit_idx].abbrev_off;
        U64 hash = u64_hash_from_str8(str8_struct(&abbrev_off));
        U64 slot_idx = hash%abbrev_off_slots_count;
        U64 abbrev_map_idx = 0;
        for(AbbrevOffNode *n = abbrev_off_slots[slot_idx]; n != 0; n = n->hash_next)
        {
          if(n->off == abbrev_off)
          {
            abbrev_map_idx = n->idx;
            break;
          }
        }
        abbrev_map_from_unit_idx_table[unit_idx] = &abbrev_map_from_idx_table[abbrev_map_idx];
      }
    }
  }
  lane_sync();
  
  ////////////////////////////
  //- rjf: build per-unit parsing contexts
  //
  DW2_ParseCtx *unit_parse_ctxs = 0;
  {
    if(lane_idx() == 0)
    {
      unit_parse_ctxs = push_array(scratch.arena, DW2_ParseCtx, unit_count);
    }
    lane_sync_u64(&unit_parse_ctxs, 0);
    Rng1U64 range = lane_range(unit_count);
    for EachInRange(unit_idx, range)
    {
      DW2_UnitHeader *hdr = &unit_headers[unit_idx];
      DW2_ParseCtx *ctx = &unit_parse_ctxs[unit_idx];
      ctx->raw         = raw;
      ctx->version     = hdr->version;
      ctx->format      = hdr->format;
      ctx->addr_size   = hdr->addr_size;
      ctx->abbrev_map  = abbrev_map_from_unit_idx_table[unit_idx];
    }
  }
  lane_sync();
  
  ////////////////////////////
  //- rjf: parse each unit's root-level tag
  //
  DW2_Tag *unit_root_tags = 0;
  {
    if(lane_idx() == 0)
    {
      unit_root_tags = push_array(scratch.arena, DW2_Tag, unit_count);
    }
    lane_sync_u64(&unit_root_tags, 0);
    Rng1U64 range = lane_range(unit_count);
    for EachInRange(unit_idx, range)
    {
      dw2_read_tag(scratch.arena, &unit_parse_ctxs[unit_idx], raw->sec[DW_Section_Info].data, unit_info_tag_ranges[unit_idx].min, &unit_root_tags[unit_idx]);
    }
  }
  lane_sync();
  
  ////////////////////////////
  //- rjf: parse each unit's line table header
  //
  DW2_LineTableHeader *unit_line_table_headers = 0;
  U64 *unit_line_table_header_sizes = 0;
  ProfScope("parse each unit's line table header")
  {
    if(lane_idx() == 0)
    {
      unit_line_table_headers = push_array(scratch.arena, DW2_LineTableHeader, unit_count);
      unit_line_table_header_sizes = push_array(scratch.arena, U64, unit_count);
    }
    lane_sync_u64(&unit_line_table_headers, 0);
    lane_sync_u64(&unit_line_table_header_sizes, 0);
    U64 unit_take_idx = 0;
    U64 *unit_take_idx_ptr = &unit_take_idx;
    lane_sync_u64(&unit_take_idx_ptr, 0);
    for(;;)
    {
      U64 unit_idx = ins_atomic_u64_inc_eval(unit_take_idx_ptr) - 1;
      if(unit_idx >= unit_count)
      {
        break;
      }
      DW2_ParseCtx *ctx = &unit_parse_ctxs[unit_idx];
      DW2_Tag *unit_root_tag = &unit_root_tags[unit_idx];
      DW2_Attrib *stmt_list = dw2_attrib_from_kind(unit_root_tag, DW_AttribKind_StmtList);
      U64 line_info_off = stmt_list->val.u128.u64[0];
      String8 line_info_data = raw->sec[DW_Section_Line].data;
      unit_line_table_header_sizes[unit_idx] = dw2_read_line_table_header(scratch.arena, ctx, line_info_data, line_info_off, &unit_line_table_headers[unit_idx]);
    }
    lane_sync();
  }
  
  ////////////////////////////
  //- rjf: deduplicate all source files
  //
  typedef struct UnitSrcFileMap UnitSrcFileMap;
  struct UnitSrcFileMap
  {
    RDIM_SrcFile **v;
  };
  RDIM_SrcFileChunkList *all_src_files = 0;
  UnitSrcFileMap *unit_src_file_maps = 0;
  ProfScope("deduplicate all source files") if(lane_idx() == 0)
  {
    Temp scratch2 = scratch_begin(&scratch.arena, 1);
    
    //- rjf: count all files in all units
    U64 total_file_count = 0;
    for EachIndex(unit_idx, unit_count)
    {
      total_file_count += unit_line_table_headers[unit_idx].files.count;
    }
    
    //- rjf: set up path -> src file map
    typedef struct SrcFileNode SrcFileNode;
    struct SrcFileNode
    {
      SrcFileNode *next;
      String8 full_path;
      RDIM_SrcFile *src_file;
    };
    all_src_files = push_array(scratch.arena, RDIM_SrcFileChunkList, 1);
    U64 slots_count = 1 + (total_file_count * 3) / 4;
    SrcFileNode **slots = push_array(scratch2.arena, SrcFileNode *, slots_count);
    
    //- rjf: set up unit -> src file maps
    unit_src_file_maps = push_array(scratch.arena, UnitSrcFileMap, unit_count);
    for EachIndex(unit_idx, unit_count)
    {
      unit_src_file_maps[unit_idx].v = push_array(scratch.arena, RDIM_SrcFile *, unit_line_table_headers[unit_idx].files.count);
    }
    
    //- rjf: build path -> src file map
    for EachIndex(unit_idx, unit_count)
    {
      DW2_LineTableHeader *hdr = &unit_line_table_headers[unit_idx];
      for EachIndex(file_idx, hdr->files.count)
      {
        DW2_LineTableFile *f = &hdr->files.v[file_idx];
        DW2_LineTableFile *dir = &hdr->dirs.v[f->dir_idx];
        String8 full_file_path = str8f(scratch2.arena, "%S/%S", dir->file_name, f->file_name);
        U64 hash = u64_hash_from_str8(full_file_path);
        U64 slot_idx = hash%slots_count;
        SrcFileNode *node = 0;
        for(SrcFileNode *n = slots[slot_idx]; n != 0; n = n->next)
        {
          if(str8_match(n->full_path, full_file_path, 0))
          {
            node = n;
            break;
          }
        }
        if(!node)
        {
          node = push_array(scratch2.arena, SrcFileNode, 1);
          node->full_path = full_file_path;
          node->src_file = rdim_src_file_chunk_list_push(arena, all_src_files, slots_count);
          node->src_file->path = str8_copy(arena, full_file_path);
          if(f->flags & DW2_LineTableFileFlag_HasMD5)
          {
            node->src_file->checksum_kind = RDI_ChecksumKind_MD5;
            node->src_file->checksum = str8_copy(arena, str8_struct(&f->md5));
          }
          else if(f->flags & DW2_LineTableFileFlag_HasModifyTime)
          {
            node->src_file->checksum_kind = RDI_ChecksumKind_Timestamp;
            node->src_file->checksum = str8_copy(arena, str8_struct(&f->modify_time));
          }
        }
        unit_src_file_maps[unit_idx].v[file_idx] = node->src_file;
      }
    }
    scratch_end(scratch2);
  }
  lane_sync_u64(&all_src_files, 0);
  lane_sync_u64(&unit_src_file_maps, 0);
  
  ////////////////////////////
  //- rjf: parse each unit's line info; produce line tables
  //
  RDIM_LineTableChunkList *all_line_tables = 0;
  RDIM_LineTable **unit_line_tables = 0;
  ProfScope("parse each unit's line info; produce line tables")
  {
    //- rjf: prep outputs
    typedef struct LineSeqChunk LineSeqChunk;
    struct LineSeqChunk
    {
      LineSeqChunk *next;
      U64 *voffs;
      U32 *line_nums;
      U16 *col_nums;
      U64 line_count;
      U64 line_cap;
    };
    typedef struct FileSeqNode FileSeqNode;
    struct FileSeqNode
    {
      FileSeqNode *next;
      RDIM_SrcFile *src_file;
      LineSeqChunk *first_seq_chunk;
      LineSeqChunk *last_seq_chunk;
      U64 total_line_count;
    };
    typedef struct FileSeqMap FileSeqMap;
    struct FileSeqMap
    {
      U64 slots_count;
      FileSeqNode **slots;
    };
    FileSeqMap *unit_file_seq_maps = 0;
    if(lane_idx() == 0)
    {
      all_line_tables = push_array(scratch.arena, RDIM_LineTableChunkList, 1);
      unit_line_tables = push_array(scratch.arena, RDIM_LineTable *, unit_count);
      for EachIndex(unit_idx, unit_count)
      {
        unit_line_tables[unit_idx] = rdim_line_table_chunk_list_push(arena, all_line_tables, unit_count);
      }
      unit_file_seq_maps = push_array(scratch.arena, FileSeqMap, unit_count);
    }
    lane_sync_u64(&all_line_tables, 0);
    lane_sync_u64(&unit_line_tables, 0);
    lane_sync_u64(&unit_file_seq_maps, 0);
    
    //- rjf: wide per-unit parse
    U64 unit_take_idx = 0;
    U64 *unit_take_idx_ptr = &unit_take_idx;
    lane_sync_u64(&unit_take_idx_ptr, 0);
    for(;;)
    {
      //- rjf: take unit
      U64 unit_idx = ins_atomic_u64_inc_eval(unit_take_idx_ptr) - 1;
      if(unit_idx >= unit_count)
      {
        break;
      }
      
      //- rjf: unpack unit info
      DW2_LineTableHeader *line_table_header = &unit_line_table_headers[unit_idx];
      RDIM_LineTable *dst_line_table = unit_line_tables[unit_idx];
      DW2_Tag *unit_root_tag = &unit_root_tags[unit_idx];
      DW2_Attrib *stmt_list = dw2_attrib_from_kind(unit_root_tag, DW_AttribKind_StmtList);
      U64 line_info_off = stmt_list->val.u128.u64[0];
      String8 all_line_info_data = raw->sec[DW_Section_Line].data;
      String8 unit_line_table_data = str8_substr(all_line_info_data, r1u64(line_info_off + unit_line_table_header_sizes[unit_idx], line_info_off + line_table_header->unit_length));
      
      //- rjf: set up per-unit file sequence map
      unit_file_seq_maps[unit_idx].slots_count = line_table_header->files.count;
      unit_file_seq_maps[unit_idx].slots = push_array(scratch.arena, FileSeqNode *, unit_file_seq_maps[unit_idx].slots_count);
      
      //- rjf: set up vm registers
      DW2_LineVMRegs vm_regs = {0};
      {
        vm_regs.file_index = 1;
        vm_regs.line = 1;
        vm_regs.is_stmt = line_table_header->default_is_stmt;
      }
      
      //- rjf: run the line opcode program
      B32 emit_line = 0;
      B32 done = 0;
      for(U64 off = 0, next_off = 0; !done && off < unit_line_table_data.size; off = next_off)
      {
        next_off = unit_line_table_data.size;
        
        //- rjf: read next opcode
        U64 op_read_off = off;
        U8 opcode = 0;
        str8_deserial_read_struct(unit_line_table_data, op_read_off, &opcode);
        op_read_off += 1;
        
        //- rjf: apply "special opcodes" (DWARF v5 6.2.5.1)
        if(opcode >= line_table_header->opcode_base)
        {
          U32 adjusted_opcode = (U32)(opcode - line_table_header->opcode_base);
          U32 op_advance = adjusted_opcode / line_table_header->line_range;
          S64 line_advance = (S64)line_table_header->line_base + (S64)adjusted_opcode%(S64)line_table_header->line_range;
          U64 addr_advance = line_table_header->min_inst_length + (vm_regs.vliw_op_index + op_advance) / line_table_header->max_ops_per_inst;
          vm_regs.address += addr_advance;
          vm_regs.vliw_op_index = (vm_regs.vliw_op_index + op_advance) % line_table_header->max_ops_per_inst;
          vm_regs.line += line_advance;
          vm_regs.basic_block = 0;
          vm_regs.prologue_end = 0;
          vm_regs.epilogue_begin = 0;
          vm_regs.discriminator = 0;
          emit_line = 1;
        }
        
        //- rjf: apply standard opcode
        else
        {
          U64 op_advance = 0;
          switch(opcode)
          {
            //- rjf: skip unknown opcode
            default:
            {
              if(opcode == 0 || opcode > line_table_header->opcode_lengths_count)
              {
                done = 1;
              }
              for EachIndex(uleb_idx, line_table_header->opcode_lengths[opcode - 1])
              {
                U64 v = 0;
                op_read_off += str8_deserial_read_uleb128(unit_line_table_data, op_read_off, &v);
              }
            }break;
            
            //- rjf: line emissions
            case DW_StdOpcode_Copy:
            {
              emit_line = 1;
              vm_regs.discriminator = 0;
              vm_regs.basic_block = 0;
              vm_regs.prologue_end = 0;
              vm_regs.epilogue_begin = 0;
            }break;
            
            //- rjf: PC advances
            case DW_StdOpcode_AdvancePc:
            {
              op_read_off += str8_deserial_read_uleb128(unit_line_table_data, op_read_off, &op_advance);
            }goto advance_pc;
            case DW_StdOpcode_ConstAddPc:
            {
              op_advance = (0xffu - line_table_header->opcode_base) / line_table_header->line_range;
            }goto advance_pc;
            advance_pc:;
            {
              U64 op_index = vm_regs.vliw_op_index + op_advance;
              vm_regs.address += line_table_header->min_inst_length * (op_index / line_table_header->max_ops_per_inst);
              vm_regs.vliw_op_index = op_index % line_table_header->max_ops_per_inst;
            }break;
            
            //- rjf: fixed PC advances
            case DW_StdOpcode_FixedAdvancePc:
            {
              U16 fixed_advance = 0;
              str8_deserial_read_struct(unit_line_table_data, op_read_off, &fixed_advance);
              op_read_off += sizeof(U16);
              vm_regs.address += fixed_advance;
              vm_regs.vliw_op_index = 0;
            }break;
            
            //- rjf: line number advance
            case DW_StdOpcode_AdvanceLine:
            {
              S64 advance = 0;
              op_read_off += str8_deserial_read_sleb128(unit_line_table_data, op_read_off, &advance);
              vm_regs.line += advance;
            }break;
            
            //- rjf: set file
            case DW_StdOpcode_SetFile:
            {
              op_read_off += str8_deserial_read_uleb128(unit_line_table_data, op_read_off, &vm_regs.file_index);
            }break;
            
            //- rjf: set column
            case DW_StdOpcode_SetColumn:
            {
              op_read_off += str8_deserial_read_uleb128(unit_line_table_data, op_read_off, &vm_regs.column);
            }break;
            
            //- rjf: negate statment
            case DW_StdOpcode_NegateStmt:
            {
              vm_regs.is_stmt = !vm_regs.is_stmt;
            }break;
            
            //- rjf: flag sets
            case DW_StdOpcode_SetBasicBlock:
            {
              vm_regs.basic_block = 1;
            }break;
            case DW_StdOpcode_SetPrologueEnd:
            {
              vm_regs.prologue_end = 1;
            }break;
            case DW_StdOpcode_SetEpilogueBegin:
            {
              vm_regs.epilogue_begin = 1;
            }break;
            
            //- rjf: isa
            case DW_StdOpcode_SetIsa:
            {
              op_read_off += str8_deserial_read_uleb128(unit_line_table_data, op_read_off, &vm_regs.isa);
            }break;
            
            //- rjf: extended opcode
            case DW_StdOpcode_ExtendedOpcode:
            {
              // rjf: read extended opcode size
              U64 ext_opcode_size = 0;
              op_read_off += str8_deserial_read_uleb128(unit_line_table_data, op_read_off, &ext_opcode_size);
              
              // rjf: read extended opcode
              U8 ext_opcode = 0;
              op_read_off += str8_deserial_read_struct(unit_line_table_data, op_read_off, &ext_opcode);
              
              // rjf: grab truncated data for just this extended opcode
              String8 ext_op_data = str8_prefix(unit_line_table_data, op_read_off + ext_opcode_size);
              
              // rjf: do extended opcode
              switch(ext_opcode)
              {
                default:
                case DW_ExtOpcode_Undefined:
                case DW_ExtOpcode_UserLo:
                case DW_ExtOpcode_UserHi:
                {}break;
                case DW_ExtOpcode_EndSequence:
                {
                  emit_line = 1;
                  vm_regs.end_sequence = 1;
                }break;
                case DW_ExtOpcode_SetAddress:
                {
                  U64 addr = 0;
                  op_read_off += str8_deserial_read(ext_op_data, op_read_off, &addr, line_table_header->addr_size, line_table_header->addr_size);
                  vm_regs.address = addr;
                  vm_regs.vliw_op_index = 0;
                }break;
                case DW_ExtOpcode_DefineFile:
                {
                  String8 file_name = {0};
                  U64 dir_idx = 0;
                  U64 modify_time = 0;
                  U64 file_size = 0;
                  op_read_off += str8_deserial_read_cstr(ext_op_data, op_read_off, &file_name);
                  op_read_off += str8_deserial_read_uleb128(ext_op_data, op_read_off, &dir_idx);
                  op_read_off += str8_deserial_read_uleb128(ext_op_data, op_read_off, &modify_time);
                  op_read_off += str8_deserial_read_uleb128(ext_op_data, op_read_off, &file_size);
                  //
                  // TODO(rjf): this is a real problem, because at this point, we've already gathered & deduped
                  // all source files, but now we're in a situation where a per-lane line info parse can produce
                  // new source files, which may - of course - be duplicates of files defined in other unit line
                  // info.
                  //
                }break;
                case DW_ExtOpcode_SetDiscriminator:
                {
                  op_read_off += str8_deserial_read_uleb128(ext_op_data, op_read_off, &vm_regs.discriminator);
                }break;
              }
            }break;
          }
        }
        
        //- rjf: emit lines / sequences
        if(emit_line)
        {
          emit_line = 0;
          
          // rjf: map file index -> rdim src file
          RDIM_SrcFile *src_file = 0;
          if(vm_regs.file_index < line_table_header->files.count)
          {
            src_file = unit_src_file_maps[unit_idx].v[vm_regs.file_index];
          }
          
          // rjf: map src file -> file seq node
          FileSeqNode *file_seq_n = 0;
          {
            U64 hash = u64_hash_from_str8(str8_struct(&src_file));
            U64 slot_idx = hash%unit_file_seq_maps[unit_idx].slots_count;
            for(FileSeqNode *n = unit_file_seq_maps[unit_idx].slots[slot_idx]; n != 0; n = n->next)
            {
              if(n->src_file == src_file)
              {
                file_seq_n = n;
                break;
              }
            }
            if(file_seq_n == 0)
            {
              file_seq_n = push_array(scratch.arena, FileSeqNode, 1);
              SLLStackPush(unit_file_seq_maps[unit_idx].slots[slot_idx], file_seq_n);
              file_seq_n->src_file = src_file;
            }
          }
          
          // rjf: push this line into the file sequence node
          {
            LineSeqChunk *chunk = file_seq_n->last_seq_chunk;
            if(vm_regs.end_sequence && chunk != 0)
            {
              chunk->voffs[chunk->line_count] = vm_regs.address;
            }
            if(vm_regs.end_sequence || chunk == 0 || chunk->line_count >= chunk->line_cap)
            {
              chunk = push_array(scratch.arena, LineSeqChunk, 1);
              SLLQueuePush(file_seq_n->first_seq_chunk, file_seq_n->last_seq_chunk, chunk);
              chunk->line_cap = 64;
              chunk->voffs = push_array(scratch.arena, U64, chunk->line_cap + 1);
              chunk->line_nums = push_array(scratch.arena, U32, chunk->line_cap);
              chunk->col_nums = push_array(scratch.arena, U16, 2*chunk->line_cap);
            }
            U64 chunk_line_idx = chunk->line_count;
            chunk->voffs[chunk_line_idx] = vm_regs.address;
            chunk->line_nums[chunk_line_idx] = (U32)vm_regs.line;
            chunk->col_nums[chunk_line_idx] = (U16)vm_regs.column;
            chunk->line_count += 1;
            file_seq_n->total_line_count += 1;
          }
        }
        
        //- rjf: advance to next opcode
        if(op_read_off > off)
        {
          next_off = op_read_off;
        }
      }
    }
    lane_sync();
    
    //- rjf: collect all sequences
    //
    // TODO(rjf)
  }
  
  ////////////////////////////
  //- rjf: fill result
  //
  RDIM_BakeParams result = {0};
  {
    // TODO(rjf)
  }
  
  scratch_end(scratch);
  return result;
}
