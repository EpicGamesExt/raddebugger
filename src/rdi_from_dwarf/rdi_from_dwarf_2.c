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
  //- rjf: parse all string offsets tables
  //
  // in an incredible twist of fate, DWARF decided to decouple these from
  // compilation units. compilation units *do* contain a
  // DW_AttribKind_StrOffsetsBase attribute. but this base offset does *not*
  // point to the beginning of a table in .debug_str_offsets! it instead points
  // PAST THE INITIAL VARIABLE-WIDTH LENGTH AND FORMAT ENCODING! this means
  // you can't actually use the StrOffsetsBase attribute for ANYTHING other
  // than correllating a unit to its associated string offset table - but you
  // *necessarily needed to have parsed that table beforehand*, completely
  // independently from units.
  //
  // so, we have to parse all the string offset tables up-front, then
  // *binary search* their ranges to determine which unit has which table.
  //
  // of course, in practice, it's perhaps likely/expected that these match
  // one-to-one with units, and in the same order, because that is what is
  // most natural for generators. but, the format does not *guarantee this*,
  // and instead specced something far more arbitrary.
  //
  // thank you, again, DWARF.
  //
  U64 str_offsets_tables_count = 0;
  DW2_StrOffsetsTable *str_offsets_tables = 0;
  Rng1U64 *str_offsets_tables_ranges = 0;
  ProfScope("parse all string offsets tables") if(lane_idx() == 0)
  {
    Temp scratch2 = scratch_begin(&scratch.arena, 1);
    
    //- rjf: gather all tables (loose)
    typedef struct TableNode TableNode;
    struct TableNode
    {
      TableNode *next;
      DW2_StrOffsetsTable v;
      Rng1U64 range;
    };
    TableNode *first_table = 0;
    TableNode *last_table = 0;
    U64 table_count = 0;
    for(U64 off = 0; off < raw->sec[DW_Section_StrOffsets].data.size;)
    {
      U64 start_off = off;
      DW2_StrOffsetsTable table = {0};
      off += dw2_read_str_offsets_table(raw->sec[DW_Section_StrOffsets].data, off, &table);
      if(table.entries != 0)
      {
        TableNode *n = push_array(scratch2.arena, TableNode, 1);
        SLLQueuePush(first_table, last_table, n);
        n->v = table;
        n->range = r1u64(start_off, off);
        table_count += 1;
      }
      if(off == start_off)
      {
        break;
      }
    }
    
    //- rjf: tighten
    str_offsets_tables_count = table_count;
    str_offsets_tables = push_array(scratch.arena, DW2_StrOffsetsTable, str_offsets_tables_count);
    str_offsets_tables_ranges = push_array(scratch.arena, Rng1U64, str_offsets_tables_count);
    {
      U64 idx = 0;
      for EachNode(n, TableNode, first_table)
      {
        str_offsets_tables[idx] = n->v;
        str_offsets_tables_ranges[idx] = n->range;
        idx += 1;
      }
    }
    
    scratch_end(scratch2);
  }
  lane_sync_u64(&str_offsets_tables_count, 0);
  lane_sync_u64(&str_offsets_tables, 0);
  lane_sync_u64(&str_offsets_tables_ranges, 0);
  
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
  //- rjf: do initial parse of each unit's root-level tag
  //
  // an incredible NOTE: this is actually not sufficient. because this tag parse
  // is what informs us which .debug_str_offsets table each unit should be associated
  // with (via the StrOffsetsBase attribute), we actually don't have the right string
  // offset table *before* we parse this, which means we can't resolve some of the
  // string attribute values.
  //
  // so, in another incredible twist of fate, we actually must do this *twice*. first,
  // to find *just the StrOffsetsBase*, and then, to actually fully resolve everything.
  //
  DW2_Tag *unit_root_tags__pre_str_offsets = 0;
  {
    if(lane_idx() == 0)
    {
      unit_root_tags__pre_str_offsets = push_array(scratch.arena, DW2_Tag, unit_count);
    }
    lane_sync_u64(&unit_root_tags__pre_str_offsets, 0);
    Rng1U64 range = lane_range(unit_count);
    for EachInRange(unit_idx, range)
    {
      dw2_read_tag(scratch.arena, &unit_parse_ctxs[unit_idx], raw->sec[DW_Section_Info].data, unit_info_tag_ranges[unit_idx].min, &unit_root_tags__pre_str_offsets[unit_idx]);
    }
  }
  lane_sync();
  
  ////////////////////////////
  //- rjf: look up string offset tables for each unit (.debug_str_offsets);
  // equip to per-unit parsing contexts
  //
  {
    Rng1U64Array str_offsets_tables_ranges_array = {str_offsets_tables_ranges, str_offsets_tables_count};
    Rng1U64 range = lane_range(unit_count);
    for EachInRange(unit_idx, range)
    {
      DW2_Tag *unit_root_tag = &unit_root_tags__pre_str_offsets[unit_idx];
      DW2_Attrib *str_offsets_base_off_attrib = dw2_attrib_from_kind(unit_root_tag, DW_AttribKind_StrOffsetsBase);
      U64 str_offsets_base_off = str_offsets_base_off_attrib->val.u128.u64[0];
      U64 str_offsets_table_num = rng1u64_array_num_from_value__binary_search(&str_offsets_tables_ranges_array, str_offsets_base_off);
      if(0 < str_offsets_table_num && str_offsets_table_num <= str_offsets_tables_ranges_array.count)
      {
        DW2_StrOffsetsTable *table = &str_offsets_tables[str_offsets_table_num-1];
        unit_parse_ctxs[unit_idx].str_offsets_table = table;
      }
    }
  }
  lane_sync();
  
  ////////////////////////////
  //- rjf: do parse of each unit's root tag AFTER finding the right .debug_str_offsets table
  //
  // (excellent work everyone - thank you for reminding me why RDI is necessary)
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
  RDIM_LineTableChunkList *unit_line_table_chunk_lists = 0;
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
      RDIM_SrcFileLineMapFragment *first_line_map_fragment;
      RDIM_SrcFileLineMapFragment *last_line_map_fragment;
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
      unit_line_table_chunk_lists = push_array(scratch.arena, RDIM_LineTableChunkList, unit_count);
      unit_line_tables = push_array(scratch.arena, RDIM_LineTable *, unit_count);
      unit_file_seq_maps = push_array(scratch.arena, FileSeqMap, unit_count);
    }
    lane_sync_u64(&unit_line_table_chunk_lists, 0);
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
      Temp scratch2 = scratch_begin(&scratch.arena, 1);
      
      //- rjf: unpack unit info
      DW2_LineTableHeader *line_table_header = &unit_line_table_headers[unit_idx];
      RDIM_LineTableChunkList *dst_line_tables = &unit_line_table_chunk_lists[unit_idx];
      DW2_Tag *unit_root_tag = &unit_root_tags[unit_idx];
      DW2_Attrib *stmt_list = dw2_attrib_from_kind(unit_root_tag, DW_AttribKind_StmtList);
      U64 line_info_off = stmt_list->val.u128.u64[0];
      String8 all_line_info_data = raw->sec[DW_Section_Line].data;
      String8 unit_line_table_data = str8_substr(all_line_info_data, r1u64(line_info_off + unit_line_table_header_sizes[unit_idx], line_info_off + line_table_header->unit_length));
      
      //- rjf: build unit's line table
      RDIM_LineTable *dst_line_table = rdim_line_table_chunk_list_push(arena, dst_line_tables, 1);
      unit_line_tables[unit_idx] = dst_line_table;
      
      //- rjf: set up per-unit file sequence map
      unit_file_seq_maps[unit_idx].slots_count = line_table_header->files.count + 1;
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
      RDIM_SrcFile *line_seq_src_file = 0;
      LineSeqChunk *first_line_seq_chunk = 0;
      LineSeqChunk *last_line_seq_chunk = 0;
      U64 total_line_seq_count = 0;
      for(U64 off = 0, next_off = 0; off <= unit_line_table_data.size; off = next_off)
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
        
        //- rjf: advance to next op
        if(op_read_off > off)
        {
          next_off = op_read_off;
        }
        
        //- rjf: map file index -> rdim src file
        RDIM_SrcFile *src_file = 0;
        if(vm_regs.file_index < line_table_header->files.count)
        {
          src_file = unit_src_file_maps[unit_idx].v[vm_regs.file_index];
        }
        
        //- rjf: sequence ended explicitly, or file change, or end of stream? -> push to line table
        if(line_seq_src_file != 0 && (vm_regs.end_sequence || (src_file != line_seq_src_file && first_line_seq_chunk != 0) || off >= unit_line_table_data.size))
        {
          // rjf: combine voffs/lines/cols
          U64 seq_line_count = total_line_seq_count;
          U64 *seq_voffs = push_array(arena, U64, seq_line_count+1);
          U32 *seq_lines = push_array(arena, U32, seq_line_count);
          U16 *seq_cols = push_array(arena, U16, 2*seq_line_count);
          {
            U64 voff_idx = 0;
            U64 line_idx = 0;
            U64 col_idx = 0;
            for(LineSeqChunk *c = first_line_seq_chunk; c != 0; c = c->next)
            {
              MemoryCopy(seq_voffs + voff_idx, c->voffs, sizeof(c->voffs[0]) * c->line_count);
              MemoryCopy(seq_lines + line_idx, c->line_nums, sizeof(c->line_nums[0]) * c->line_count);
              MemoryCopy(seq_cols + col_idx, c->col_nums, sizeof(c->col_nums[0]) * 2 * c->line_count);
              voff_idx += c->line_count;
              line_idx += c->line_count;
              col_idx += 2*c->line_count;
            }
            seq_voffs[seq_line_count] = vm_regs.address - base_vaddr;
          }
          
          // rjf: push sequence to line table
          RDIM_LineSequence *seq = rdim_line_table_push_sequence(arena, dst_line_tables, dst_line_table, line_seq_src_file, seq_voffs, seq_lines, seq_cols, seq_line_count);
          
          // rjf: map src file -> file seq node
          FileSeqNode *file_seq_n = 0;
          {
            U64 hash = u64_hash_from_str8(str8_struct(&line_seq_src_file));
            U64 slot_idx = hash%unit_file_seq_maps[unit_idx].slots_count;
            for(FileSeqNode *n = unit_file_seq_maps[unit_idx].slots[slot_idx]; n != 0; n = n->next)
            {
              if(n->src_file == line_seq_src_file)
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
          
          // rjf: record sequence in file seq node
          {
            RDIM_SrcFileLineMapFragment *f = push_array(scratch.arena, RDIM_SrcFileLineMapFragment, 1);
            SLLQueuePush(file_seq_n->first_line_map_fragment, file_seq_n->last_line_map_fragment, f);
            f->seq = seq;
          }
          
          // rjf: reset
          first_line_seq_chunk = last_line_seq_chunk = 0;
          total_line_seq_count = 0;
          line_seq_src_file = 0;
          MemoryZeroStruct(&vm_regs);
          vm_regs.file_index = 1;
          vm_regs.line = 1;
          vm_regs.is_stmt = line_table_header->default_is_stmt;
        }
        
        //- rjf: emit lines
        if(emit_line)
        {
          emit_line = 0;
          LineSeqChunk *chunk = last_line_seq_chunk;
          if(chunk == 0 || chunk->line_count >= chunk->line_cap)
          {
            chunk = push_array(scratch.arena, LineSeqChunk, 1);
            SLLQueuePush(first_line_seq_chunk, last_line_seq_chunk, chunk);
            chunk->line_cap = 64;
            chunk->voffs = push_array(scratch.arena, U64, chunk->line_cap + 1);
            chunk->line_nums = push_array(scratch.arena, U32, chunk->line_cap);
            chunk->col_nums = push_array(scratch.arena, U16, 2*chunk->line_cap);
          }
          U64 chunk_line_idx = chunk->line_count;
          chunk->voffs[chunk_line_idx] = vm_regs.address - base_vaddr;
          chunk->line_nums[chunk_line_idx] = (U32)vm_regs.line;
          chunk->col_nums[chunk_line_idx] = (U16)vm_regs.column;
          chunk->line_count += 1;
          total_line_seq_count += 1;
          line_seq_src_file = src_file;
        }
      }
      
      scratch_end(scratch2);
    }
    lane_sync();
    
    //- rjf: equip source files with their fragments
    //
    // TODO(rjf): this can *almost* be wide, but we are relying on a few top-level
    // summations inside the RDIM_SrcFileChunkList when we push a sequence to a src
    // file. if we just summed those later when baking (probably fine), then this
    // could go wide across all src files, which would be very nice. just lane-0ing
    // for now. when possible, we can probably do the same thing in PDB too.
    //
    if(lane_idx() == 0)
    {
      for EachIndex(unit_idx, unit_count)
      {
        FileSeqMap *file_seq_map = &unit_file_seq_maps[unit_idx];
        for EachIndex(slot_idx, file_seq_map->slots_count)
        {
          for(FileSeqNode *n = file_seq_map->slots[slot_idx]; n != 0; n = n->next)
          {
            for(RDIM_SrcFileLineMapFragment *f = n->first_line_map_fragment; f != 0; f = f->next)
            {
              rdim_src_file_push_line_sequence(arena, all_src_files, n->src_file, f->seq);
            }
          }
        }
      }
    }
    lane_sync();
  }
  
  ////////////////////////////
  //- rjf: join all line tables
  //
  RDIM_LineTableChunkList *all_line_tables = 0;
  if(lane_idx() == 0)
  {
    all_line_tables = push_array(scratch.arena, RDIM_LineTableChunkList, 1);
    for EachIndex(unit_idx, unit_count)
    {
      rdim_line_table_chunk_list_concat_in_place(all_line_tables, &unit_line_table_chunk_lists[unit_idx]);
    }
  }
  lane_sync_u64(&all_line_tables, 0);
  
  ////////////////////////////
  //- rjf: build built-in types
  //
  RDIM_TypeChunkList *builtin_types = 0;
  RDIM_Type **builtin_type_from_kind_map = 0; // [RDI_TypeKind_LastBuiltIn - RDI_TypeKind_FirstBuiltIn + 1]
  U64 builtin_type_count = RDI_TypeKind_LastBuiltIn - RDI_TypeKind_FirstBuiltIn + 1;
  ProfScope("build built-in types") if(lane_idx() == 0)
  {
    builtin_types = push_array(scratch.arena, RDIM_TypeChunkList, 1);
    builtin_type_from_kind_map = push_array(scratch.arena, RDIM_Type *, builtin_type_count);
    for(RDI_TypeKind k = RDI_TypeKind_FirstBuiltIn; k <= RDI_TypeKind_LastBuiltIn; k += 1)
    {
      RDIM_Type *type = rdim_type_chunk_list_push(arena, builtin_types, builtin_type_count);
      type->kind = k;
      type->name.str = rdi_string_from_type_kind(k, &type->name.size);
      type->byte_size = rdi_size_from_basic_type_kind(k);
      if(type->byte_size == max_U32) { type->byte_size = byte_size_from_arch(arch); }
      builtin_type_from_kind_map[k - RDI_TypeKind_FirstBuiltIn] = type;
    }
  }
  lane_sync_u64(&builtin_types, 0);
  lane_sync_u64(&builtin_type_from_kind_map, 0);
#define d2r2_type_from_builtin_kind(k) ((RDI_TypeKind_FirstBuiltIn <= (k) && (k) <= RDI_TypeKind_LastBuiltIn) ? builtin_types[k - RDI_TypeKind_FirstBuiltIn] : 0)
  
  ////////////////////////////
  //- rjf: predict the total number of tags in all units
  //
  U64 total_tag_count_estimate = 1;
  {
    U64 tag_size_estimate = 32;
    for EachIndex(unit_idx, unit_count)
    {
      total_tag_count_estimate += dim_1u64(unit_info_tag_ranges[unit_idx]) / tag_size_estimate;
    }
  }
  
  ////////////////////////////
  //- rjf: gather all unique type tags across all units
  //
  typedef struct UniqueTypeTagNode UniqueTypeTagNode;
  struct UniqueTypeTagNode
  {
    UniqueTypeTagNode *next;
    U64 hash;
    U64 unit_idx;
    U64 info_off;
  };
  typedef struct UnitTypeNode UnitTypeNode;
  struct UnitTypeNode
  {
    UnitTypeNode *next;
    U64 src_info_off;
    U64 dst_hash;
  };
  typedef struct UnitTypeMap UnitTypeMap;
  struct UnitTypeMap
  {
    UnitTypeNode **slots;
    U64 slots_count;
  };
  UniqueTypeTagNode **unique_type_tag_slots = 0;
  U64 unique_type_tag_slots_count = total_tag_count_estimate/8 + 1;
  UnitTypeMap *unit_type_maps = 0;
  ProfScope("gather all unique type tags across all units")
  {
    if(lane_idx() == 0)
    {
      unique_type_tag_slots = push_array(scratch.arena, UniqueTypeTagNode *, unique_type_tag_slots_count);
      unit_type_maps = push_array(scratch.arena, UnitTypeMap, unit_count);
    }
    lane_sync_u64(&unique_type_tag_slots, 0);
    lane_sync_u64(&unit_type_maps, 0);
    U64 unit_take_idx_ = 0;
    U64 *unit_take_idx_ptr = &unit_take_idx_;
    lane_sync_u64(&unit_take_idx_ptr, 0);
    for(;;)
    {
      //- rjf: take next unit
      U64 origin_unit_idx = ins_atomic_u64_inc_eval(unit_take_idx_ptr) - 1;
      if(origin_unit_idx >= unit_count)
      {
        break;
      }
      
      //- rjf: unpack unit info
      Rng1U64 origin_unit_info_tag_range = unit_info_tag_ranges[origin_unit_idx];
      
      //- rjf: set up type map for this unit
      unit_type_maps[origin_unit_idx].slots_count = dim_1u64(origin_unit_info_tag_range) / 256 + 1;
      unit_type_maps[origin_unit_idx].slots = push_array(scratch.arena, UnitTypeNode *, unit_type_maps[origin_unit_idx].slots_count);
      
      //- rjf: hash all type content from tags in this unit; record if unique
      for(U64 off = origin_unit_info_tag_range.min; off < origin_unit_info_tag_range.max;)
      {
        Temp scratch2 = scratch_begin(&scratch.arena, 1);
        U64 start_off = off;
        
        //- rjf: hash type tags - this requires a hash of not only type tags'
        // attributes, but also a walk of all other type tags this tag references,
        // and a hash of them too. so we produce a list of tasks for
        // parsing/hashing tags, in order to find the full comprehensive hash
        // for each type tag.
        //
        // importantly, doing this can cause cycles in principle, so we also
        // record which tags we visited, & order them, so we can just hash the
        // order index, instead of doing a full recursion.
        //
        B32 is_type_tag_tree = 0;
        U64 hash = 0;
        {
          typedef struct TypeTagTask TypeTagTask;
          struct TypeTagTask
          {
            TypeTagTask *next;
            U64 unit_idx;
            U64 off;
            U64 order_idx;
          };
          TypeTagTask start_task = {0, origin_unit_idx, off};
          TypeTagTask *top_task = &start_task;
          TypeTagTask *free_task = 0;
          U64 seen_task_slots_count = 16;
          TypeTagTask **seen_task_slots = push_array(scratch2.arena, TypeTagTask *, seen_task_slots_count);
          for(TypeTagTask *t = top_task, *next = 0; t != 0; t = next)
          {
            next = 0;
            U64 t_off = t->off;
            
            // rjf: record this task in our seen task table
            {
              U64 hash = u64_hash_from_str8(str8_struct(&t->off));
              U64 slot_idx = hash%seen_task_slots_count;
              SLLStackPush(seen_task_slots[slot_idx], t);
            }
            
            // rjf: unpack unit
            Rng1U64 unit_info_tag_range = unit_info_tag_ranges[t->unit_idx];
            DW2_ParseCtx *unit_parse_ctx = &unit_parse_ctxs[t->unit_idx];
            
            // rjf: read/hash the full tag tree at `t_off`; kick off additional
            // tasks for referenced dependency types
            U64 depth = 0;
            for(;unit_info_tag_range.min <= t_off && t_off < unit_info_tag_range.max;)
            {
              U64 t_start_off = t_off;
              
              // rjf: read tag
              DW2_Tag tag = {0};
              t_off += dw2_read_tag(scratch2.arena, unit_parse_ctx, raw->sec[DW_Section_Info].data, t_off, &tag);
              
              // rjf: record top-level info about this tag tree
              if(t_start_off == t->off && t == &start_task)
              {
                is_type_tag_tree = (tag.kind == DW_TagKind_ArrayType || tag.kind == DW_TagKind_ClassType ||
                                    tag.kind == DW_TagKind_EnumerationType || tag.kind == DW_TagKind_PointerType ||
                                    tag.kind == DW_TagKind_ReferenceType || tag.kind == DW_TagKind_StringType ||
                                    tag.kind == DW_TagKind_StructureType || tag.kind == DW_TagKind_SubroutineType ||
                                    tag.kind == DW_TagKind_Typedef || tag.kind == DW_TagKind_UnionType ||
                                    tag.kind == DW_TagKind_PtrToMemberType || tag.kind == DW_TagKind_SetType ||
                                    tag.kind == DW_TagKind_SubrangeType || tag.kind == DW_TagKind_BaseType ||
                                    tag.kind == DW_TagKind_ConstType || tag.kind == DW_TagKind_FileType ||
                                    tag.kind == DW_TagKind_PackedType || tag.kind == DW_TagKind_VolatileType ||
                                    tag.kind == DW_TagKind_RestrictType || tag.kind == DW_TagKind_InterfaceType ||
                                    tag.kind == DW_TagKind_UnspecifiedType || tag.kind == DW_TagKind_SharedType ||
                                    tag.kind == DW_TagKind_RValueReferenceType || tag.kind == DW_TagKind_CoarrayType ||
                                    tag.kind == DW_TagKind_DynamicType || tag.kind == DW_TagKind_AtomicType ||
                                    tag.kind == DW_TagKind_ImmutableType);
              }
              
              // rjf: is type -> combine tag's content into hash
              if(is_type_tag_tree)
              {
                // rjf: combine tag's kind
                hash = u64_hash_from_seed_str8(hash, str8_struct(&tag.kind));
                
                // rjf: combine non-reference attributes (references could be different,
                // because of deduping, but they could match ultimately). for any
                // referenced dependency types, kick them off
                for(DW2_AttribNode *n = tag.attribs.first; n != 0; n = n->next)
                {
                  // rjf: non-reference? -> combine attribute value info
                  if(n->v.val.kind != DW_Form_RefAddr &&
                     n->v.val.kind != DW_Form_Ref1 &&
                     n->v.val.kind != DW_Form_Ref2 &&
                     n->v.val.kind != DW_Form_Ref4 &&
                     n->v.val.kind != DW_Form_Ref8 &&
                     n->v.val.kind != DW_Form_RefUData &&
                     n->v.val.kind != DW_Form_RefSup4 &&
                     n->v.val.kind != DW_Form_RefSig8)
                  {
                    hash = u64_hash_from_seed_str8(hash, str8_struct(&n->v.val.kind));
                    hash = u64_hash_from_seed_str8(hash, str8_struct(&n->v.val.u128));
                    hash = u64_hash_from_seed_str8(hash, n->v.val.string);
                  }
                  
                  // rjf: type reference? -> if seen, combine the order; if not, recurse
                  if(n->v.attrib_kind == DW_AttribKind_Type)
                  {
                    // rjf: unpack reference
                    U64 ref_info_off = dw2_reference_info_off_from_form_val(unit_parse_ctx, &n->v.val);
                    
                    // rjf: determine if we've seen this reference
                    B32 already_seen = 0;
                    U64 already_seen_order_idx = 0;
                    {
                      U64 off_hash = u64_hash_from_str8(str8_struct(&ref_info_off));
                      U64 off_slot_idx = off_hash%seen_task_slots_count;
                      for(TypeTagTask *t = seen_task_slots[off_slot_idx]; t != 0; t = t->next)
                      {
                        if(t->off == ref_info_off)
                        {
                          already_seen = 1;
                          already_seen_order_idx = t->order_idx;
                          break;
                        }
                      }
                    }
                    
                    // rjf: if we've seen -> hash the order
                    if(already_seen)
                    {
                      hash = u64_hash_from_seed_str8(hash, str8_struct(&already_seen_order_idx));
                    }
                    
                    // rjf: if we've not seen -> descend
                    if(!already_seen)
                    {
                      TypeTagTask *dependency_task = free_task;
                      if(dependency_task != 0)
                      {
                        SLLStackPop(free_task);
                      }
                      else
                      {
                        dependency_task = push_array(scratch2.arena, TypeTagTask, 1);
                      }
                      next = dependency_task;
                      dependency_task->off = ref_info_off;
                      dependency_task->unit_idx = t->unit_idx;
                      if(!contains_1u64(unit_info_tag_range, dependency_task->off))
                      {
                        Rng1U64Array unit_info_tag_ranges_array = {unit_info_tag_ranges, unit_count};
                        U64 new_unit_num = rng1u64_array_num_from_value__binary_search(&unit_info_tag_ranges_array, dependency_task->off);
                        if(0 < new_unit_num && new_unit_num <= unit_count)
                        {
                          dependency_task->unit_idx = new_unit_num-1;
                        }
                      }
                      dependency_task->order_idx = t->order_idx+1;
                    }
                  }
                }
              }
              
              // rjf: is type tag tree, has children -> descend
              if(is_type_tag_tree && tag.has_children)
              {
                depth += 1;
              }
              
              // rjf: zero tag kind -> ascend
              if(is_type_tag_tree && tag.kind == DW_TagKind_Null && depth > 0)
              {
                depth -= 1;
              }
              
              // rjf: no advancing? -> +1
              if(t_off == t_start_off)
              {
                t_off += 1;
              }
              
              // rjf: depth == 0? -> done
              if(depth == 0)
              {
                break;
              }
            }
            
            // rjf: is this the starter task? -> advance base reading offset
            if(t == &start_task)
            {
              off = t_off;
            }
          }
        }
        
        //- rjf: if offset not advanced -> increment
        if(off == start_off)
        {
          off += 1;
        }
        
        //- rjf: atomically gather this hash if not already gathered
        if(is_type_tag_tree)
        {
          B32 gathered = 0;
          U64 slot_idx = hash%unique_type_tag_slots_count;
          for(;!gathered;)
          {
            // rjf: read existing slot head pointer *before* we lookup / insert
            U64 slot_head_val = (U64)ins_atomic_u64_eval(&unique_type_tag_slots[slot_idx]);
            
            // rjf: determine if this hash has been gathered
            for(UniqueTypeTagNode *n = (UniqueTypeTagNode *)slot_head_val; n != 0; n = n->next)
            {
              if(n->hash == hash)
              {
                gathered = 1;
                break;
              }
            }
            
            // rjf: if this hash has *not* been gathered, try an insert. we:
            //
            //  1. allocate/fill a node
            //  2. set it up to point to the old head
            //  3. compare/exchange the old head with the new head - IFF the head matches what we expect from above
            //  4. if we fail, another thread has touched this slot, we pop the allocated node & try again
            //     (we may find that another thread has filled this hash, so we'll just be done)
            //
            if(!gathered)
            {
              Temp insert_temp = temp_begin(scratch.arena);
              UniqueTypeTagNode *n = push_array(scratch.arena, UniqueTypeTagNode, 1);
              n->next = (UniqueTypeTagNode *)slot_head_val;
              n->hash = hash;
              n->unit_idx = origin_unit_idx;
              n->info_off = start_off;
              U64 new_head_val = (U64)n;
              if(slot_head_val == ins_atomic_u64_eval_cond_assign(&unique_type_tag_slots[slot_idx], new_head_val, slot_head_val))
              {
                gathered = 1;
              }
              else
              {
                temp_end(insert_temp);
              }
            }
          }
        }
        
        //- rjf: record this (info_off -> hash) mapping, so that when we have
        // later references to this type, we can redirect to the deduplicated
        // type with the right hash later.
        if(is_type_tag_tree)
        {
          U64 info_off_hash = u64_hash_from_str8(str8_struct(&start_off));
          U64 info_off_slot_idx = info_off_hash%unit_type_maps[origin_unit_idx].slots_count;
          UnitTypeNode *n = push_array(scratch.arena, UnitTypeNode, 1);
          n->src_info_off = start_off;
          n->dst_hash = hash;
          SLLStackPush(unit_type_maps[origin_unit_idx].slots[info_off_slot_idx], n);
        }
        
        scratch_end(scratch2);
      }
    }
    lane_sync();
  }
  
  ////////////////////////////
  //- rjf: produce [0...n) -> unique-tag-node mapping for all types
  //
  U64 type_count = 0;
  UniqueTypeTagNode **type_tag_nodes = 0;
  ProfScope("produce [0...n) -> hash mapping for all types") if(lane_idx() == 0)
  {
    for EachIndex(slot_idx, unique_type_tag_slots_count)
    {
      for EachNode(n, UniqueTypeTagNode, unique_type_tag_slots[slot_idx])
      {
        type_count += 1;
      }
    }
    type_tag_nodes = push_array(scratch.arena, UniqueTypeTagNode *, type_count);
    U64 idx = 0;
    for EachIndex(slot_idx, unique_type_tag_slots_count)
    {
      for EachNode(n, UniqueTypeTagNode, unique_type_tag_slots[slot_idx])
      {
        type_tag_nodes[idx] = n;
        idx += 1;
      }
    }
  }
  lane_sync_u64(&type_count, 0);
  lane_sync_u64(&type_tag_nodes, 0);
  
  ////////////////////////////
  //- rjf: gather per-type dependency chains
  //
  typedef struct TypeDepChain TypeDepChain;
  struct TypeDepChain
  {
    TypeDepChain *next;
    U64 hash;
  };
  TypeDepChain **type_dep_chains = 0;
  ProfScope("gather per-type dependency chains")
  {
    if(lane_idx() == 0)
    {
      type_dep_chains = push_array(scratch.arena, TypeDepChain *, type_count);
    }
    lane_sync_u64(&type_dep_chains, 0);
    Rng1U64 range = lane_range(type_count);
    for EachInRange(type_idx, range)
    {
      Temp scratch2 = scratch_begin(&scratch.arena, 1);
      
      // rjf: unpack type's tag node
      UniqueTypeTagNode *type_tag_node = type_tag_nodes[type_idx];
      U64 info_off = type_tag_node->info_off;
      U64 unit_idx = type_tag_node->unit_idx;
      
      // rjf: unpack unit
      DW2_ParseCtx *unit_parse_ctx = &unit_parse_ctxs[unit_idx];
      Rng1U64 unit_info_tag_range = unit_info_tag_ranges[unit_idx];
      
      // rjf: parse this type's tag
      DW2_Tag tag = {0};
      dw2_read_tag(scratch2.arena, unit_parse_ctx, raw->sec[DW_Section_Info].data, info_off, &tag);
      
      // rjf: find direct type, if one exists
      U64 direct_type_info_off = 0;
      U64 direct_type_unit_idx = 0;
      switch(tag.kind)
      {
        default:{}break;
        case DW_TagKind_PointerType:
        case DW_TagKind_ReferenceType:
        case DW_TagKind_RValueReferenceType:
        case DW_TagKind_RestrictType:
        case DW_TagKind_VolatileType:
        case DW_TagKind_ConstType:
        case DW_TagKind_ArrayType:
        case DW_TagKind_SubrangeType:
        case DW_TagKind_Typedef:
        {
          DW2_Attrib *direct_type_attrib = dw2_attrib_from_kind(&tag, DW_AttribKind_Type);
          direct_type_info_off = dw2_reference_info_off_from_form_val(unit_parse_ctx, &direct_type_attrib->val);
          direct_type_unit_idx = unit_idx;
          if(!contains_1u64(unit_info_tag_range, direct_type_info_off))
          {
            Rng1U64Array unit_info_tag_ranges_array = {unit_info_tag_ranges, unit_count};
            U64 new_unit_num = rng1u64_array_num_from_value__binary_search(&unit_info_tag_ranges_array, direct_type_info_off);
            if(0 < new_unit_num && new_unit_num <= unit_count)
            {
              direct_type_unit_idx = new_unit_num-1;
            }
          }
        }break;
      }
      
      // rjf: look up hash for direct type
      U64 direct_type_hash = 0;
      {
        U64 off_hash = u64_hash_from_str8(str8_struct(&direct_type_info_off));
        U64 off_slot_idx = off_hash%unit_type_maps[direct_type_unit_idx].slots_count;
        for(UnitTypeNode *n = unit_type_maps[direct_type_unit_idx].slots[off_slot_idx]; n != 0; n = n->next)
        {
          if(n->src_info_off == direct_type_info_off)
          {
            direct_type_hash = n->dst_hash;
            break;
          }
        }
      }
      
      // rjf: if we got a hash -> record as dependency
      if(direct_type_hash != 0)
      {
        TypeDepChain *chain = push_array(scratch.arena, TypeDepChain, 1);
        chain->hash = direct_type_hash;
        SLLStackPush(type_dep_chains[type_idx], chain);
      }
      
      scratch_end(scratch2);
    }
  }
  
  ////////////////////////////
  //- rjf: fill result
  //
  RDIM_BakeParams result = {0};
  {
    // TODO(rjf)
  }
  
#undef d2r2_type_from_builtin_kind
  scratch_end(scratch);
  return result;
}
