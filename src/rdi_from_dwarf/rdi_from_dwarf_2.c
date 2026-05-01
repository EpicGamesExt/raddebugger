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
  U64 arch_addr_size = byte_size_from_arch(arch);
  
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
  //- rjf: gather unit ranges from .debug_info, .debug_aranges
  //
  Rng1U64Array *unit_info_ranges = 0;
  Rng1U64Array *unit_arange_ranges = 0;
  ProfScope("gather unit ranges from .debug_info, .debug_aranges") if(lane_idx() == 0)
  {
    struct
    {
      String8 data;
      Rng1U64Array **dst_array;
    }
    tasks[] =
    {
      {raw->sec[DW_Section_Info].data,    &unit_info_ranges},
      {raw->sec[DW_Section_ARanges].data, &unit_arange_ranges},
    };
    for EachElement(task_idx, tasks)
    {
      Temp scratch2 = scratch_begin(&scratch.arena, 1);
      String8 data = tasks[task_idx].data;
      Rng1U64List unit_range_list = {0};
      for(U64 off = 0; off < data.size;)
      {
        U64 start_off = off;
        
        // rjf: read next unit size
        U64 unit_size = 0;
        U64 unit_size_size = dw2_read_initial_length(data, off, &unit_size, 0);
        
        // rjf: push
        if(unit_size > 0)
        {
          rng1u64_list_push(scratch2.arena, &unit_range_list, r1u64(off, off + unit_size_size + unit_size));
        }
        
        // rjf: advance
        off += unit_size_size;
        off += unit_size;
        
        // rjf: break if no movement
        if(off == start_off)
        {
          break;
        }
      }
      tasks[task_idx].dst_array[0] = push_array(scratch.arena, Rng1U64Array, 1);
      tasks[task_idx].dst_array[0][0] = rng1u64_array_from_list(scratch.arena, &unit_range_list);
      scratch_end(scratch2);
    }
  }
  lane_sync_u64(&unit_info_ranges, 0);
  lane_sync_u64(&unit_arange_ranges, 0);
  U64 unit_count = unit_info_ranges->count;
  
  ////////////////////////////
  //- rjf: parse all .debug_info unit headers
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
  //- rjf: parse all units from .debug_aranges
  //
  U64 *arange_info_offs = 0;
  RDIM_Rng1U64ChunkList *arange_voff_ranges = 0;
  {
    if(lane_idx() == 0)
    {
      arange_info_offs = push_array(scratch.arena, U64, unit_arange_ranges->count);
      arange_voff_ranges = push_array(scratch.arena, RDIM_Rng1U64ChunkList, unit_arange_ranges->count);
    }
    lane_sync_u64(&arange_info_offs, 0);
    lane_sync_u64(&arange_voff_ranges, 0);
    U64 unit_take_idx_ = 0;
    U64 *unit_take_idx_ptr = &unit_take_idx_;
    lane_sync_u64(&unit_take_idx_ptr, 0);
    String8 data = raw->sec[DW_Section_ARanges].data;
    for(;;)
    {
      // rjf: take unit
      U64 unit_idx = ins_atomic_u64_inc_eval(unit_take_idx_ptr)-1;
      if(unit_idx >= unit_arange_ranges->count)
      {
        break;
      }
      
      // rjf: unpack
      Rng1U64 arange_range = unit_arange_ranges->v[unit_idx];
      U64 off = arange_range.min;
      
      // rjf: read unit data size / format
      U64 unit_size = 0;
      DW_Format fmt = 0;
      off += dw2_read_initial_length(data, off, &unit_size, &fmt);
      U64 unit_opl = off + unit_size;
      
      // rjf: read version
      DW_Version version = 0;
      U64 version_off = off;
      off += str8_deserial_read_struct(data, off, &version);
      
      // rjf: warn on non-version 2
      if(version != DW_Version_2)
      {
        log_infof("[.debug_aranges@0x%I64x] DWARF version for unit #%I64d was expected to be 2, but it was read as %i.\n", version_off, unit_idx, (S32)version);
      }
      
      // rjf: read .debug_info off for this unit
      U64 info_off = 0;
      off += dw2_read_fmt_u64(data, off, fmt, &info_off);
      
      // rjf: read address / segment selector size
      U8 addr_size = 0;
      U8 segment_selector_size = 0;
      off += str8_deserial_read_struct(data, off, &addr_size);
      off += str8_deserial_read_struct(data, off, &segment_selector_size);
      
      // rjf: round up past padding
      {
        U64 tuple_size = addr_size*2 + segment_selector_size;
        off += tuple_size - (off%tuple_size);
      }
      
      // rjf: parse ranges
      RDIM_Rng1U64ChunkList voff_ranges = {0};
      if(segment_selector_size != 0)
      {
        log_infof("[.debug_aranges@0x%I64x] Non-zero (%i) segment selector size parsed; this form of addressing is not currently supported in DWARF info.\n", off, (S32)segment_selector_size);
      }
      else for(;off < unit_opl;)
      {
        U64 start_off = off;
        U64 base_addr = 0;
        U64 range_size = 0;
        off += str8_deserial_read(data, off, &base_addr, addr_size, addr_size);
        off += str8_deserial_read(data, off, &range_size, addr_size, addr_size);
        if(base_addr == 0 && range_size == 0)
        {
          break;
        }
        if(base_addr < base_vaddr)
        {
          log_infof("[.debug_aranges@0x%I64x] Address (0x%I64x) parsed which was less than the image base address (0x%I64x). Skipping.\n", start_off, base_addr, base_vaddr);
        }
        else
        {
          U64 voff_first = (base_addr - base_vaddr);
          U64 voff_opl = voff_first + range_size;
          RDIM_Rng1U64 range = {voff_first, voff_opl};
          rdim_rng1u64_chunk_list_push(arena, &voff_ranges, 256, range);
        }
        if(off == start_off)
        {
          break;
        }
      }
      
      // rjf: store
      arange_info_offs[unit_idx] = info_off;
      arange_voff_ranges[unit_idx] = voff_ranges;
    }
    lane_sync();
  }
  
  ////////////////////////////
  //- rjf: produce info_off -> list(voff_range) map from aranges units;
  // we must do this because technically we can't guarantee that unit_idxs
  // inside of .debug_aranges are the same as unit_idxs inside of .debug_info,
  // nor can we guarantee that they'd be in the same order, so we need to
  // correllate via the encoded .debug_info offset from .debug_aranges.
  //
  // more excellence.
  //
  typedef struct D2R2_ARangeUnitNode D2R2_ARangeUnitNode;
  struct D2R2_ARangeUnitNode
  {
    D2R2_ARangeUnitNode *next;
    U64 info_off;
    RDIM_Rng1U64ChunkList *ranges;
  };
  U64 arange_unit_from_info_off_map_slots_count = unit_arange_ranges->count;
  D2R2_ARangeUnitNode **arange_unit_from_info_off_map_slots = 0;
  {
    if(lane_idx() == 0)
    {
      arange_unit_from_info_off_map_slots = push_array(scratch.arena, D2R2_ARangeUnitNode *, arange_unit_from_info_off_map_slots_count);
    }
    lane_sync_u64(&arange_unit_from_info_off_map_slots, 0);
    for EachIndex(arange_unit_idx, unit_arange_ranges->count)
    {
      U64 info_off = arange_info_offs[arange_unit_idx];
      RDIM_Rng1U64ChunkList *ranges = &arange_voff_ranges[arange_unit_idx];
      U64 hash = u64_hash_from_str8(str8_struct(&info_off));
      U64 slot_idx = hash%arange_unit_from_info_off_map_slots_count;
      D2R2_ARangeUnitNode *n = push_array(scratch.arena, D2R2_ARangeUnitNode, 1);
      SLLStackPush(arange_unit_from_info_off_map_slots[slot_idx], n);
      n->info_off = info_off;
      n->ranges = ranges;
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
  //- rjf: parse all unit offsets tables
  //
  // on .debug_str_offsets, as one example:
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
  DW2_OffsetTable *str_offsets_tables = 0;
  Rng1U64 *str_offsets_tables_ranges = 0;
  U64 rnglists_tables_count = 0;
  DW2_OffsetTable *rnglists_tables = 0;
  Rng1U64 *rnglists_tables_ranges = 0;
  U64 addr_tables_count = 0;
  DW2_OffsetTable *addr_tables = 0;
  Rng1U64 *addr_tables_ranges = 0;
  U64 loclists_tables_count = 0;
  DW2_OffsetTable *loclists_tables = 0;
  Rng1U64 *loclists_tables_ranges = 0;
  ProfScope("parse all offset tables (.debug_rnglists, .debug_str_offsets, .debug_addr, .debug_loclists)") if(lane_idx() == 0)
  {
    struct
    {
      String8 data;
      U64 *tables_count_out;
      DW2_OffsetTable **tables_out;
      Rng1U64 **tables_ranges_out;
    }
    tasks[] =
    {
      {raw->sec[DW_Section_StrOffsets].data, &str_offsets_tables_count, &str_offsets_tables, &str_offsets_tables_ranges},
      {raw->sec[DW_Section_RngLists].data, &rnglists_tables_count, &rnglists_tables, &rnglists_tables_ranges},
      {raw->sec[DW_Section_Addr].data, &addr_tables_count, &addr_tables, &addr_tables_ranges},
      {raw->sec[DW_Section_LocLists].data, &loclists_tables_count, &loclists_tables, &loclists_tables_ranges},
    };
    for EachElement(task_idx, tasks)
    {
      Temp scratch2 = scratch_begin(&scratch.arena, 1);
      String8 data = tasks[task_idx].data;
      
      //- rjf: gather all tables (loose)
      typedef struct TableNode TableNode;
      struct TableNode
      {
        TableNode *next;
        DW2_OffsetTable v;
        Rng1U64 range;
      };
      TableNode *first_table = 0;
      TableNode *last_table = 0;
      U64 table_count = 0;
      for(U64 off = 0; off < data.size;)
      {
        U64 start_off = off;
        DW2_OffsetTable table = {0};
        off += dw2_read_offset_table(data, off, &table);
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
      tasks[task_idx].tables_count_out[0] = table_count;
      tasks[task_idx].tables_out[0] = push_array(scratch.arena, DW2_OffsetTable, table_count);
      tasks[task_idx].tables_ranges_out[0] = push_array(scratch.arena, Rng1U64, table_count);
      {
        U64 idx = 0;
        for EachNode(n, TableNode, first_table)
        {
          tasks[task_idx].tables_out[0][idx] = n->v;
          tasks[task_idx].tables_ranges_out[0][idx] = n->range;
          idx += 1;
        }
      }
      
      scratch_end(scratch2);
    }
  }
  lane_sync_u64(&str_offsets_tables_count, 0);
  lane_sync_u64(&str_offsets_tables, 0);
  lane_sync_u64(&str_offsets_tables_ranges, 0);
  lane_sync_u64(&rnglists_tables_count, 0);
  lane_sync_u64(&rnglists_tables, 0);
  lane_sync_u64(&rnglists_tables_ranges, 0);
  lane_sync_u64(&addr_tables_count, 0);
  lane_sync_u64(&addr_tables, 0);
  lane_sync_u64(&addr_tables_ranges, 0);
  lane_sync_u64(&loclists_tables_count, 0);
  lane_sync_u64(&loclists_tables, 0);
  lane_sync_u64(&loclists_tables_ranges, 0);
  
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
  // (all of the above is also true for RngListsBase)
  //
  DW2_Tag *unit_root_tags__pre_offset_tables = 0;
  {
    if(lane_idx() == 0)
    {
      unit_root_tags__pre_offset_tables = push_array(scratch.arena, DW2_Tag, unit_count);
    }
    lane_sync_u64(&unit_root_tags__pre_offset_tables, 0);
    Rng1U64 range = lane_range(unit_count);
    for EachInRange(unit_idx, range)
    {
      dw2_read_tag(scratch.arena, &unit_parse_ctxs[unit_idx], raw->sec[DW_Section_Info].data, unit_info_tag_ranges[unit_idx].min, &unit_root_tags__pre_offset_tables[unit_idx]);
    }
  }
  lane_sync();
  
  ////////////////////////////
  //- rjf: look up base addresses for each unit - equip to per-unit parsing contexts
  //
  {
    Rng1U64 range = lane_range(unit_count);
    for EachInRange(unit_idx, range)
    {
      DW2_Tag *unit_root_tag = &unit_root_tags__pre_offset_tables[unit_idx];
      DW2_Attrib *low_pc_attrib = dw2_attrib_from_kind(unit_root_tag, DW_AttribKind_LowPc);
      U64 low_pc = low_pc_attrib->val.addr;
      unit_parse_ctxs[unit_idx].unit_base_addr = low_pc;
    }
  }
  lane_sync();
  
  ////////////////////////////
  //- rjf: look up offset tables for each unit (.debug_str_offsets, .debug_rnglists, etc.);
  // equip to per-unit parsing contexts
  //
  {
    Rng1U64Array rnglists_tables_ranges_array = {rnglists_tables_ranges, rnglists_tables_count};
    Rng1U64Array str_offsets_tables_ranges_array = {str_offsets_tables_ranges, str_offsets_tables_count};
    Rng1U64Array addr_tables_ranges_array = {addr_tables_ranges, addr_tables_count};
    Rng1U64Array loclist_tables_ranges_array = {loclists_tables_ranges, loclists_tables_count};
    Rng1U64 range = lane_range(unit_count);
    for EachInRange(unit_idx, range)
    {
      DW2_Tag *unit_root_tag = &unit_root_tags__pre_offset_tables[unit_idx];
      
      // rjf: find rnglists table
      {
        DW2_Attrib *rnglists_base_off_attrib = dw2_attrib_from_kind(unit_root_tag, DW_AttribKind_RngListsBase);
        U64 rnglists_base_off = rnglists_base_off_attrib->val.u128.u64[0];
        U64 rnglists_table_num = rng1u64_array_num_from_value__binary_search(&rnglists_tables_ranges_array, rnglists_base_off);
        if(0 < rnglists_table_num && rnglists_table_num <= rnglists_tables_ranges_array.count)
        {
          DW2_OffsetTable *table = &rnglists_tables[rnglists_table_num-1];
          unit_parse_ctxs[unit_idx].rnglists_table = table;
        }
      }
      
      // rjf: find str offsets table
      {
        DW2_Attrib *str_offsets_base_off_attrib = dw2_attrib_from_kind(unit_root_tag, DW_AttribKind_StrOffsetsBase);
        U64 str_offsets_base_off = str_offsets_base_off_attrib->val.u128.u64[0];
        U64 str_offsets_table_num = rng1u64_array_num_from_value__binary_search(&str_offsets_tables_ranges_array, str_offsets_base_off);
        if(0 < str_offsets_table_num && str_offsets_table_num <= str_offsets_tables_ranges_array.count)
        {
          DW2_OffsetTable *table = &str_offsets_tables[str_offsets_table_num-1];
          unit_parse_ctxs[unit_idx].str_offsets_table = table;
        }
      }
      
      // rjf: find addr table
      {
        DW2_Attrib *addr_base_off_attrib = dw2_attrib_from_kind(unit_root_tag, DW_AttribKind_AddrBase);
        U64 addr_base_off = addr_base_off_attrib->val.u128.u64[0];
        U64 addr_table_num = rng1u64_array_num_from_value__binary_search(&addr_tables_ranges_array, addr_base_off);
        if(0 < addr_table_num && addr_table_num <= addr_tables_ranges_array.count)
        {
          DW2_OffsetTable *table = &addr_tables[addr_table_num-1];
          unit_parse_ctxs[unit_idx].addr_table = table;
        }
      }
      
      // rjf: find addr table
      {
        DW2_Attrib *loclist_base_off_attrib = dw2_attrib_from_kind(unit_root_tag, DW_AttribKind_LocListsBase);
        U64 loclist_base_off = loclist_base_off_attrib->val.u128.u64[0];
        U64 loclist_table_num = rng1u64_array_num_from_value__binary_search(&loclist_tables_ranges_array, loclist_base_off);
        if(0 < loclist_table_num && loclist_table_num <= loclist_tables_ranges_array.count)
        {
          DW2_OffsetTable *table = &loclists_tables[loclist_table_num-1];
          unit_parse_ctxs[unit_idx].loclists_table = table;
        }
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
#define d2r2_type_from_builtin_kind(k) ((RDI_TypeKind_FirstBuiltIn <= (k) && (k) <= RDI_TypeKind_LastBuiltIn) ? builtin_type_from_kind_map[k - RDI_TypeKind_FirstBuiltIn] : builtin_type_from_kind_map[RDI_TypeKind_Void])
  
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
    U64 order_idx;
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
              
              // rjf: determine if tag should be skipped
              B32 should_skip_tag = (tag.kind == DW_TagKind_LexicalBlock ||
                                     tag.kind == DW_TagKind_Variable);
              
              // rjf: record top-level info about this tag tree
              if(t_start_off == t->off && t == &start_task)
              {
                is_type_tag_tree = (tag.kind == DW_TagKind_ArrayType ||
                                    tag.kind == DW_TagKind_ClassType ||
                                    tag.kind == DW_TagKind_EnumerationType ||
                                    tag.kind == DW_TagKind_PointerType ||
                                    tag.kind == DW_TagKind_ReferenceType ||
                                    tag.kind == DW_TagKind_StringType ||
                                    tag.kind == DW_TagKind_StructureType ||
                                    tag.kind == DW_TagKind_SubroutineType ||
                                    tag.kind == DW_TagKind_SubProgram ||
                                    tag.kind == DW_TagKind_Typedef ||
                                    tag.kind == DW_TagKind_UnionType ||
                                    tag.kind == DW_TagKind_PtrToMemberType ||
                                    tag.kind == DW_TagKind_SetType ||
                                    tag.kind == DW_TagKind_BaseType ||
                                    tag.kind == DW_TagKind_ConstType ||
                                    tag.kind == DW_TagKind_FileType ||
                                    tag.kind == DW_TagKind_PackedType ||
                                    tag.kind == DW_TagKind_VolatileType ||
                                    tag.kind == DW_TagKind_RestrictType ||
                                    tag.kind == DW_TagKind_InterfaceType ||
                                    tag.kind == DW_TagKind_UnspecifiedType ||
                                    tag.kind == DW_TagKind_SharedType ||
                                    tag.kind == DW_TagKind_RValueReferenceType ||
                                    tag.kind == DW_TagKind_CoarrayType ||
                                    tag.kind == DW_TagKind_DynamicType ||
                                    tag.kind == DW_TagKind_AtomicType ||
                                    tag.kind == DW_TagKind_ImmutableType);
              }
              
              // rjf: is type -> combine tag's content into hash
              if(!should_skip_tag && is_type_tag_tree)
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
                     n->v.val.kind != DW_Form_RefSig8 &&
                     ((tag.kind != DW_TagKind_SubProgram &&
                       tag.kind != DW_TagKind_FormalParameter) ||
                      (n->v.attrib_kind != DW_AttribKind_Name &&
                       n->v.attrib_kind != DW_AttribKind_DeclFile &&
                       n->v.attrib_kind != DW_AttribKind_DeclLine &&
                       n->v.attrib_kind != DW_AttribKind_Prototyped &&
                       n->v.attrib_kind != DW_AttribKind_External &&
                       n->v.attrib_kind != DW_AttribKind_FrameBase &&
                       n->v.attrib_kind != DW_AttribKind_Location &&
                       n->v.attrib_kind != DW_AttribKind_LowPc &&
                       n->v.attrib_kind != DW_AttribKind_HighPc)))
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
  //- rjf: produce [0...n) <-> unique-tag-node mapping for all types
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
        n->order_idx = idx;
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
    U64 type_idx;
  };
  U64 *type_dep_chains_counts = 0;
  ProfScope("gather per-type dependency chains")
  {
    if(lane_idx() == 0)
    {
      type_dep_chains_counts = push_array(scratch.arena, U64, type_count);
    }
    lane_sync_u64(&type_dep_chains_counts, 0);
    Rng1U64 range = lane_range(type_count);
    for EachInRange(root_type_idx, range)
    {
      typedef struct TypeChainTask TypeChainTask;
      struct TypeChainTask
      {
        TypeChainTask *next;
        U64 hash;
      };
      TypeChainTask start_task = {0, type_tag_nodes[root_type_idx]->hash};
      TypeChainTask *top_task = &start_task;
      TypeChainTask *free_task = 0;
      TypeChainTask *last_t = 0;
      for(TypeChainTask *t = top_task; t != 0; (last_t = t, t = t->next))
      {
        Temp scratch2 = scratch_begin(&scratch.arena, 1);
        
        // rjf: recycle old tasks
        if(last_t != 0)
        {
          SLLStackPush(free_task, last_t);
        }
        
        // rjf: unpack task
        U64 hash = t->hash;
        
        // rjf: hash -> unique type tag node
        UniqueTypeTagNode *type_tag_node = 0;
        {
          U64 slot_idx = hash%unique_type_tag_slots_count;
          for(UniqueTypeTagNode *n = unique_type_tag_slots[slot_idx]; n != 0; n = n->next)
          {
            if(n->hash == hash)
            {
              type_tag_node = n;
              break;
            }
          }
        }
        
        // rjf: unpack type tag node
        U64 info_off = 0;
        U64 unit_idx = 0;
        if(type_tag_node != 0)
        {
          info_off = type_tag_node->info_off;
          unit_idx = type_tag_node->unit_idx;
        }
        
        // rjf: record this type in the dependency chain count
        if(type_tag_node != 0)
        {
          type_dep_chains_counts[root_type_idx] += 1;
        }
        
        // rjf: unpack unit
        DW2_ParseCtx *unit_parse_ctx = &unit_parse_ctxs[unit_idx];
        Rng1U64 unit_info_tag_range = unit_info_tag_ranges[unit_idx];
        
        // rjf: parse this type's tag
        U64 read_off = info_off;
        DW2_Tag tag = {0};
        read_off += dw2_read_tag(scratch2.arena, unit_parse_ctx, raw->sec[DW_Section_Info].data, read_off, &tag);
        
        // rjf: find direct types from this type tag
        typedef struct DirectTypeNode DirectTypeNode;
        struct DirectTypeNode
        {
          DirectTypeNode *next;
          U64 info_off;
          U64 unit_idx;
        };
        DirectTypeNode *first_direct_type = 0;
        DirectTypeNode *last_direct_type = 0;
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
          case DW_TagKind_SubProgram:
          case DW_TagKind_SubroutineType:
          case DW_TagKind_EnumerationType:
          {
            // rjf: gather direct type
            {
              DW2_Attrib *direct_type_attrib = dw2_attrib_from_kind(&tag, DW_AttribKind_Type);
              U64 direct_type_info_off = dw2_reference_info_off_from_form_val(unit_parse_ctx, &direct_type_attrib->val);
              U64 direct_type_unit_idx = unit_idx;
              if(!contains_1u64(unit_info_tag_range, direct_type_info_off))
              {
                Rng1U64Array unit_info_tag_ranges_array = {unit_info_tag_ranges, unit_count};
                U64 new_unit_num = rng1u64_array_num_from_value__binary_search(&unit_info_tag_ranges_array, direct_type_info_off);
                if(0 < new_unit_num && new_unit_num <= unit_count)
                {
                  direct_type_unit_idx = new_unit_num-1;
                }
              }
              DirectTypeNode *n = push_array(scratch2.arena, DirectTypeNode, 1);
              n->info_off = direct_type_info_off;
              n->unit_idx = direct_type_unit_idx;
              SLLQueuePush(first_direct_type, last_direct_type, n);
            }
            
            // rjf: functions -> gather parameters
            if(tag.has_children && (tag.kind == DW_TagKind_SubProgram || tag.kind == DW_TagKind_SubroutineType))
            {
              S64 depth = 1;
              for(;depth > 0 && contains_1u64(unit_info_tag_range, read_off);)
              {
                U64 start_read_off = read_off;
                
                // rjf: read child tag
                DW2_Tag child_tag = {0};
                read_off += dw2_read_tag(scratch2.arena, unit_parse_ctx, raw->sec[DW_Section_Info].data, read_off, &child_tag);
                
                // rjf: formal parameters -> gather direct types
                if(depth == 1 && child_tag.kind == DW_TagKind_FormalParameter)
                {
                  DW2_Attrib *direct_type_attrib = dw2_attrib_from_kind(&child_tag, DW_AttribKind_Type);
                  U64 direct_type_info_off = dw2_reference_info_off_from_form_val(unit_parse_ctx, &direct_type_attrib->val);
                  U64 direct_type_unit_idx = unit_idx;
                  if(!contains_1u64(unit_info_tag_range, direct_type_info_off))
                  {
                    Rng1U64Array unit_info_tag_ranges_array = {unit_info_tag_ranges, unit_count};
                    U64 new_unit_num = rng1u64_array_num_from_value__binary_search(&unit_info_tag_ranges_array, direct_type_info_off);
                    if(0 < new_unit_num && new_unit_num <= unit_count)
                    {
                      direct_type_unit_idx = new_unit_num-1;
                    }
                  }
                  DirectTypeNode *n = push_array(scratch2.arena, DirectTypeNode, 1);
                  n->info_off = direct_type_info_off;
                  n->unit_idx = direct_type_unit_idx;
                  SLLQueuePush(first_direct_type, last_direct_type, n);
                }
                
                // rjf: tree navigations
                if(child_tag.has_children)
                {
                  depth += 1;
                }
                if(child_tag.kind == DW_TagKind_Null)
                {
                  depth -= 1;
                }
                
                if(read_off == start_read_off)
                {
                  break;
                }
              }
            }
          }break;
        }
        
        // rjf: for each dependency type, look up their hash, + spawn new tasks for them
        for EachNode(n, DirectTypeNode, first_direct_type)
        {
          U64 direct_type_info_off = n->info_off;
          U64 direct_type_unit_idx = n->unit_idx;
          
          // rjf: direct type info offset -> hash
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
          
          // rjf: spawn task
          TypeChainTask *new_task = free_task;
          if(new_task != 0)
          {
            SLLStackPop(free_task);
          }
          else
          {
            new_task = push_array(scratch.arena, TypeChainTask, 1);
          }
          new_task->hash = direct_type_hash;
          t->next = new_task;
        }
        
        scratch_end(scratch2);
      }
    }
    lane_sync();
  }
  
  ////////////////////////////
  //- rjf: build all types - build types w/ 1 dependency (leaves) first, then 2, 3, etc.,
  // to ensure dependencies always travel backwards
  //
  // NOTE: * DWARF vs RDI Array Type Graph *
  //
  // For example lets take following decl:
  //
  //    int (*foo[2])[3];
  //
  //  This compiles to in DWARF:
  //
  //  foo -> DW_TAG_ArrayType -> (A0) DW_TAG_Subrange [2]
  //                          \
  //                           -> (B0) DW_TAG_PointerType -> (A1) DW_TAG_ArrayType -> DW_TAG_Subrange [3]
  //                                                      \
  //                                                       -> (B1) DW_TAG_BaseType (int)
  //
  // RDI expects:
  //
  //  foo -> Array[2] -> Pointer -> Array[3] -> int
  //
  // Note that DWARF forks the graph on DW_TAG_ArrayType to describe array ranges in branch A and
  // in branch B describes array type which might be a struct, pointer, base type, or any other type tag.
  // However, in RDI we have a simple list of type nodes and to convert we need to append type nodes from
  // B to A.
  //
  RDIM_TypeChunkList *all_types = 0;
  RDIM_Type **type_from_idx_map = 0;
  ProfScope("build all types")
  {
    if(lane_idx() == 0)
    {
      all_types = push_array(scratch.arena, RDIM_TypeChunkList, 1);
      type_from_idx_map = push_array(scratch.arena, RDIM_Type *, type_count);
      rdim_type_chunk_list_concat_in_place(all_types, builtin_types);
    }
    lane_sync_u64(&all_types, 0);
    lane_sync_u64(&type_from_idx_map, 0);
    Rng1U64 range = lane_range(type_count);
    U64 max_chain_count = 1;
    for(;max_chain_count < max_U64;)
    {
      Temp scratch2 = scratch_begin(&scratch.arena, 1);
      
      //- rjf: gather all types in this lane that fit the dependency restriction;
      // find this lane's next dependency restriction
      U64 next_max_chain_count = max_U64;
      RDIM_TypeChunkList lane_types = {0};
      U64 lane_types_chunk_count = 256;
      for EachInRange(type_idx, range)
      {
        // rjf: if this type has a higher chain count than the current,
        // but it is lower than our next maximum chain count, collect it,
        // so we will hit
        //
        // if this type has a lower chain count than the current,
        // we should've already built it from a previous pass.
        //
        if(type_dep_chains_counts[type_idx] > max_chain_count)
        {
          next_max_chain_count = Min(type_dep_chains_counts[type_idx], next_max_chain_count);
          continue;
        }
        else if(type_dep_chains_counts[type_idx] < max_chain_count)
        {
          continue;
        }
        Temp temp = temp_begin(scratch2.arena);
        
        // rjf: idx -> type tag node
        UniqueTypeTagNode *type_tag_node = type_tag_nodes[type_idx];
        U64 unit_idx = type_tag_node->unit_idx;
        U64 info_off = type_tag_node->info_off;
        
        // rjf: unpack unit
        Rng1U64 unit_info_tag_range = unit_info_tag_ranges[unit_idx];
        DW2_ParseCtx *unit_parse_ctx = &unit_parse_ctxs[unit_idx];
        
        // rjf: parse root-level tag
        DW2_Tag tag = {0};
        U64 tag_info_size = dw2_read_tag(temp.arena, unit_parse_ctx, raw->sec[DW_Section_Info].data, info_off, &tag);
        
        // rjf: extract common attributes
        DW2_Attrib *name_attrib = &dw2_attrib_nil;
        DW2_Attrib *direct_type_attrib = &dw2_attrib_nil;
        DW2_Attrib *bitsize_attrib = &dw2_attrib_nil;
        DW2_Attrib *bytesize_attrib = &dw2_attrib_nil;
        DW2_Attrib *encoding_attrib = &dw2_attrib_nil;
        DW2_Attrib *decl_attrib = &dw2_attrib_nil;
        //
        // TODO(rjf): if a DW_AttribKing_GNU_Vector is found on an DW_TagKind_ArrayType,
        // then: @native_vector_support extract byte size from the base type tag
        // and convert to U256, U512, S256 and S512
        //
        for EachNode(n, DW2_AttribNode, tag.attribs.first)
        {
          switch(n->v.attrib_kind)
          {
            default:{}break;
#define Case(dst_name, src_name) case DW_AttribKind_##src_name:{dst_name##_attrib = &n->v;}break
            Case(name,        Name);
            Case(direct_type, Type);
            Case(bitsize,     BitSize);
            Case(bytesize,    ByteSize);
            Case(encoding,    Encoding);
            Case(decl,        Declaration);
#undef Case
          }
        }
        
        // rjf: unpack common attributes
        String8 name = name_attrib->val.string;
        DW_ATE encoding = encoding_attrib->val.u128.u64[0];
        RDIM_Type *direct_type = d2r2_type_from_builtin_kind(RDI_TypeKind_Void);
        U64 bitsize = 0;
        B32 is_decl = (decl_attrib != &dw2_attrib_nil);
        {
          // rjf: unpack direct type
          if(direct_type_attrib != &dw2_attrib_nil)
          {
            U64 direct_type_info_off = dw2_reference_info_off_from_form_val(unit_parse_ctx, &direct_type_attrib->val);
            U64 direct_type_unit_idx = unit_idx;
            if(!contains_1u64(unit_info_tag_range, direct_type_info_off))
            {
              Rng1U64Array unit_info_tag_ranges_array = {unit_info_tag_ranges, unit_count};
              U64 new_unit_num = rng1u64_array_num_from_value__binary_search(&unit_info_tag_ranges_array, direct_type_info_off);
              direct_type_unit_idx = (new_unit_num > 0 ? new_unit_num-1 : unit_idx);
            }
            UnitTypeMap *direct_type_unit_type_map = &unit_type_maps[direct_type_unit_idx];
            U64 info_off_hash = u64_hash_from_str8(str8_struct(&direct_type_info_off));
            U64 info_off_slot_idx = info_off_hash%direct_type_unit_type_map->slots_count;
            U64 direct_type_hash = 0;
            for(UnitTypeNode *n = direct_type_unit_type_map->slots[info_off_slot_idx]; n != 0; n = n->next)
            {
              if(n->src_info_off == direct_type_info_off)
              {
                direct_type_hash = n->dst_hash;
                break;
              }
            }
            U64 unique_type_tag_slot_idx = direct_type_hash%unique_type_tag_slots_count;
            for(UniqueTypeTagNode *n = unique_type_tag_slots[unique_type_tag_slot_idx]; n != 0; n = n->next)
            {
              if(n->hash == direct_type_hash)
              {
                direct_type = type_from_idx_map[n->order_idx];
                break;
              }
            }
          }
          
          // rjf: unpack bit/byte sizes
          if(bitsize_attrib != &dw2_attrib_nil)
          {
            bitsize = bitsize_attrib->val.u128.u64[0];
          }
          else if(bytesize_attrib != &dw2_attrib_nil)
          {
            bitsize = bytesize_attrib->val.u128.u64[0]*8;
          }
        }
        
        // rjf: convert type
        RDIM_Type *dst_type = 0;
        RDI_TypeKind rdi_type_kind = RDI_TypeKind_NULL;
        RDI_TypeModifierFlags rdi_type_modifier_flags = 0;
        switch(tag.kind)
        {
          default:{}break;
          case DW_TagKind_ClassType:     rdi_type_kind = is_decl ? RDI_TypeKind_IncompleteClass  : RDI_TypeKind_Class; goto struct_type;
          case DW_TagKind_StructureType: rdi_type_kind = is_decl ? RDI_TypeKind_IncompleteStruct : RDI_TypeKind_Struct; goto struct_type;
          case DW_TagKind_UnionType:     rdi_type_kind = is_decl ? RDI_TypeKind_IncompleteUnion  : RDI_TypeKind_Union; goto struct_type;
          struct_type:;
          {
            dst_type = rdim_type_chunk_list_push(arena, &lane_types, lane_types_chunk_count);
            dst_type->kind      = rdi_type_kind;
            dst_type->name      = name;
            dst_type->byte_size = bitsize/8;
          }break;
          case DW_TagKind_EnumerationType:
          {
            dst_type = rdim_type_chunk_list_push(arena, &lane_types, lane_types_chunk_count);
            dst_type->kind        = is_decl ? RDI_TypeKind_IncompleteEnum : RDI_TypeKind_Enum;
            dst_type->name        = name;
            dst_type->direct_type = direct_type;
            dst_type->byte_size   = (direct_type != 0 ? direct_type->byte_size : bitsize/8);
          }break;
          case DW_TagKind_SubProgram:
          case DW_TagKind_SubroutineType:
          {
            // rjf: build type
            dst_type = rdim_type_chunk_list_push(arena, &lane_types, lane_types_chunk_count);
            dst_type->kind        = RDI_TypeKind_Function;
            dst_type->name        = name;
            dst_type->direct_type = direct_type;
            dst_type->byte_size   = arch_addr_size;
            
            // rjf: gather all parameter types
            typedef struct ParamNode ParamNode;
            struct ParamNode
            {
              ParamNode *next;
              U64 type_unit_idx;
              U64 type_info_off;
            };
            ParamNode *first_param = 0;
            ParamNode *last_param = 0;
            U64 param_count = 0;
            {
              U64 tag_children_off = info_off + tag_info_size;
              S64 depth = 1;
              for(U64 off = tag_children_off; depth > 0 && contains_1u64(unit_info_tag_range, off);)
              {
                U64 start_off = off;
                
                // rjf: read child tag
                DW2_Tag child_tag = {0};
                off += dw2_read_tag(scratch2.arena, unit_parse_ctx, raw->sec[DW_Section_Info].data, off, &child_tag);
                
                // rjf: gather parameters
                if(depth == 1 && child_tag.kind == DW_TagKind_FormalParameter)
                {
                  DW2_Attrib *type_attrib = dw2_attrib_from_kind(&child_tag, DW_AttribKind_Type);
                  ParamNode *n = push_array(scratch2.arena, ParamNode, 1);
                  n->type_info_off = dw2_reference_info_off_from_form_val(unit_parse_ctx, &direct_type_attrib->val);
                  n->type_unit_idx = unit_idx;
                  SLLQueuePush(first_param, last_param, n);
                  if(!contains_1u64(unit_info_tag_range, n->type_info_off))
                  {
                    Rng1U64Array unit_info_tag_ranges_array = {unit_info_tag_ranges, unit_count};
                    U64 new_unit_num = rng1u64_array_num_from_value__binary_search(&unit_info_tag_ranges_array, n->type_info_off);
                    n->type_unit_idx = (new_unit_num > 0 ? new_unit_num-1 : unit_idx);
                  }
                  param_count += 1;
                }
                
                // rjf: navigate tree
                if(child_tag.kind == DW_TagKind_Null)
                {
                  depth -= 1;
                }
                if(child_tag.has_children)
                {
                  depth += 1;
                }
                
                if(off == start_off)
                {
                  break;
                }
              }
            }
            
            // rjf: tighten parameter types
            dst_type->count = (RDI_U32)param_count; // TODO(rjf): @u64_to_u32
            dst_type->param_types = push_array(arena, RDIM_Type *, dst_type->count);
            {
              U64 param_idx = 0;
              for(ParamNode *n = first_param; n != 0; n = n->next, param_idx += 1)
              {
                U64 param_type_info_off = n->type_info_off;
                U64 param_type_unit_idx = n->type_unit_idx;
                
                // rjf: map info_off/unit_idx -> hash
                U64 param_type_hash = 0;
                {
                  UnitTypeMap *direct_type_unit_type_map = &unit_type_maps[param_type_unit_idx];
                  U64 info_off_hash = u64_hash_from_str8(str8_struct(&param_type_info_off));
                  U64 info_off_slot_idx = info_off_hash%direct_type_unit_type_map->slots_count;
                  for(UnitTypeNode *n = direct_type_unit_type_map->slots[info_off_slot_idx]; n != 0; n = n->next)
                  {
                    if(n->src_info_off == param_type_info_off)
                    {
                      param_type_hash = n->dst_hash;
                      break;
                    }
                  }
                }
                
                // rjf: map hash -> type
                RDIM_Type *param_type = 0;
                {
                  U64 unique_type_tag_slot_idx = param_type_hash%unique_type_tag_slots_count;
                  for(UniqueTypeTagNode *n = unique_type_tag_slots[unique_type_tag_slot_idx]; n != 0; n = n->next)
                  {
                    if(n->hash == param_type_hash)
                    {
                      param_type = type_from_idx_map[n->order_idx];
                      break;
                    }
                  }
                }
                
                // rjf: store
                dst_type->param_types[param_idx] = param_type;
              }
            }
          }break;
          case DW_TagKind_Typedef:
          {
            dst_type = rdim_type_chunk_list_push(arena, &lane_types, lane_types_chunk_count);
            dst_type->kind        = RDI_TypeKind_Alias;
            dst_type->name        = name;
            dst_type->direct_type = direct_type;
            dst_type->byte_size   = (direct_type ? direct_type->byte_size : 0);
          }break;
          case DW_TagKind_BaseType:
          {
            if(0){}
#define CaseA(rdi_type_kind_, encoding_)                                  else if(encoding == (encoding_)) { rdi_type_kind = (rdi_type_kind_); }
#define CaseB(rdi_type_kind_, encoding_, bit_size_)                       else if(encoding == (encoding_) && bitsize == (bit_size_)) { rdi_type_kind = (rdi_type_kind_); }
#define CaseC(rdi_type_kind_, encoding_, name_) /* assumes x64/arm64 */   else if(encoding == (encoding_) && str8_match(name, str8_lit(name_), 0)) { rdi_type_kind = (rdi_type_kind_); }
#define CaseD(rdi_type_kind_, encoding_, name_, bit_size_)                else if(encoding == (encoding_) && bitsize == (bit_size_) && str8_match(name, str8_lit(name_), 0)) { rdi_type_kind = (rdi_type_kind_); }
            CaseA(RDI_TypeKind_NULL,        DW_ATE_Null)
              CaseA(RDI_TypeKind_Void,        DW_ATE_Address)
              CaseA(RDI_TypeKind_Bool,        DW_ATE_Boolean)
              CaseB(RDI_TypeKind_ComplexF32,  DW_ATE_ComplexFloat, 32)
              CaseB(RDI_TypeKind_ComplexF64,  DW_ATE_ComplexFloat, 64)
              CaseB(RDI_TypeKind_ComplexF80,  DW_ATE_ComplexFloat, 80)
              CaseB(RDI_TypeKind_ComplexF128, DW_ATE_ComplexFloat, 128)
              CaseC(RDI_TypeKind_F80,         DW_ATE_Float,        "__float80")
              CaseC(RDI_TypeKind_F128,        DW_ATE_Float,        "__float128")
              CaseC(RDI_TypeKind_F16,         DW_ATE_Float,        "_Float16")
              CaseC(RDI_TypeKind_BF16,        DW_ATE_Float,        "__bf16")
              CaseB(RDI_TypeKind_F16,         DW_ATE_Float,        16)
              CaseB(RDI_TypeKind_F32,         DW_ATE_Float,        32)
              CaseB(RDI_TypeKind_F48,         DW_ATE_Float,        48)
              CaseB(RDI_TypeKind_F64,         DW_ATE_Float,        64)
              CaseB(RDI_TypeKind_F80,         DW_ATE_Float,        80)
              CaseB(RDI_TypeKind_F96,         DW_ATE_Float,        96)
              CaseB(RDI_TypeKind_F128,        DW_ATE_Float,        128)
              CaseD(RDI_TypeKind_Char8,       DW_ATE_Signed,       "wchar_t", 8)
              CaseD(RDI_TypeKind_Char16,      DW_ATE_Signed,       "wchar_t", 16)
              CaseD(RDI_TypeKind_Char32,      DW_ATE_Signed,       "wchar_t", 32)
              CaseB(RDI_TypeKind_S8,          DW_ATE_Signed,       8)
              CaseB(RDI_TypeKind_S16,         DW_ATE_Signed,       16)
              CaseB(RDI_TypeKind_S32,         DW_ATE_Signed,       32)
              CaseB(RDI_TypeKind_S64,         DW_ATE_Signed,       64)
              CaseB(RDI_TypeKind_S128,        DW_ATE_Signed,       128)
              CaseB(RDI_TypeKind_S256,        DW_ATE_Signed,       256)
              CaseB(RDI_TypeKind_S512,        DW_ATE_Signed,       512)
              CaseB(RDI_TypeKind_U8,          DW_ATE_Unsigned,     8)
              CaseB(RDI_TypeKind_U16,         DW_ATE_Unsigned,     16)
              CaseB(RDI_TypeKind_U32,         DW_ATE_Unsigned,     32)
              CaseB(RDI_TypeKind_U64,         DW_ATE_Unsigned,     64)
              CaseB(RDI_TypeKind_U128,        DW_ATE_Unsigned,     128)
              CaseB(RDI_TypeKind_U256,        DW_ATE_Unsigned,     256)
              CaseB(RDI_TypeKind_U512,        DW_ATE_Unsigned,     512)
              CaseB(RDI_TypeKind_UChar8,      DW_ATE_UnsignedChar, 8)
              CaseB(RDI_TypeKind_UChar8,      DW_ATE_Utf,          8)
              CaseB(RDI_TypeKind_UChar16,     DW_ATE_UnsignedChar, 16)
              CaseB(RDI_TypeKind_UChar16,     DW_ATE_Utf,          16)
              CaseB(RDI_TypeKind_UChar32,     DW_ATE_UnsignedChar, 32)
              CaseB(RDI_TypeKind_UChar32,     DW_ATE_Utf,          32)
              CaseB(RDI_TypeKind_Decimal32,   DW_ATE_DecimalFloat, 32)
              CaseB(RDI_TypeKind_Decimal64,   DW_ATE_DecimalFloat, 64)
              CaseB(RDI_TypeKind_Decimal128,  DW_ATE_DecimalFloat, 128)
#undef CaseA
#undef CaseB
#undef CaseC
#undef CaseD
            dst_type = rdim_type_chunk_list_push(arena, &lane_types, lane_types_chunk_count);
            dst_type->kind        = RDI_TypeKind_Alias;
            dst_type->name        = name;
            dst_type->direct_type = d2r2_type_from_builtin_kind(rdi_type_kind);
            dst_type->byte_size   = bitsize/8;
          }break;
          case DW_TagKind_PointerType:         rdi_type_kind = RDI_TypeKind_Ptr; goto ptr_or_ref_type;
          case DW_TagKind_ReferenceType:       rdi_type_kind = RDI_TypeKind_LRef; goto ptr_or_ref_type;
          case DW_TagKind_RValueReferenceType: rdi_type_kind = RDI_TypeKind_RRef; goto ptr_or_ref_type;
          ptr_or_ref_type:;
          {
            dst_type = rdim_type_chunk_list_push(arena, &lane_types, lane_types_chunk_count);
            dst_type->kind        = rdi_type_kind;
            dst_type->direct_type = direct_type;
            dst_type->byte_size   = arch_addr_size;
          }break;
          case DW_TagKind_RestrictType:
          {
            rdi_type_modifier_flags = RDI_TypeModifierFlag_Restrict;
          }goto basic_type_operators;
          case DW_TagKind_VolatileType:
          {
            rdi_type_modifier_flags = RDI_TypeModifierFlag_Volatile;
          }goto basic_type_operators;
          case DW_TagKind_ConstType:
          {
            rdi_type_modifier_flags = RDI_TypeModifierFlag_Const;
          }goto basic_type_operators;
          basic_type_operators:;
          {
            dst_type = rdim_type_chunk_list_push(arena, &lane_types, lane_types_chunk_count);
            dst_type->kind        = RDI_TypeKind_Modifier;
            dst_type->flags       = rdi_type_modifier_flags;
            dst_type->direct_type = direct_type;
            dst_type->byte_size   = (direct_type ? direct_type->byte_size : 0);
          }break;
          case DW_TagKind_ArrayType:
          if(tag.has_children)
          {
            // rjf: parse array type children; extract dimensions (each one gets a SubrangeType)
            typedef struct ArrayDimensionNode ArrayDimensionNode;
            struct ArrayDimensionNode
            {
              ArrayDimensionNode *next;
              U64 count;
            };
            ArrayDimensionNode *top_dimension = 0;
            {
              U64 tag_children_off = info_off + tag_info_size;
              S64 depth = 1;
              for(U64 off = tag_children_off; contains_1u64(unit_info_tag_range, off) && depth > 0;)
              {
                U64 start_off = off;
                DW2_Tag child_tag = {0};
                off += dw2_read_tag(temp.arena, unit_parse_ctx, raw->sec[DW_Section_Info].data, off, &child_tag);
                if(child_tag.kind == DW_TagKind_SubrangeType)
                {
                  // rjf: extract bounds attribs
                  DW2_Attrib *lower_bound_attrib = &dw2_attrib_nil;
                  DW2_Attrib *upper_bound_attrib = &dw2_attrib_nil;
                  DW2_Attrib *count_attrib = &dw2_attrib_nil;
                  for(DW2_AttribNode *n = child_tag.attribs.first;
                      n != 0 && (lower_bound_attrib == &dw2_attrib_nil || (upper_bound_attrib == &dw2_attrib_nil && count_attrib == &dw2_attrib_nil));
                      n = n->next)
                  {
                    if(n->v.attrib_kind == DW_AttribKind_LowerBound)
                    {
                      lower_bound_attrib = &n->v;
                    }
                    else if(n->v.attrib_kind == DW_AttribKind_UpperBound)
                    {
                      upper_bound_attrib = &n->v;
                    }
                    else if(n->v.attrib_kind == DW_AttribKind_Count)
                    {
                      count_attrib = &n->v;
                    }
                  }
                  
                  // rjf: resolve lower bound
                  U64 lower_bound = 0;
                  {
                    if(dw_is_form_kind_ref(unit_parse_ctx->version, unit_parse_ctx->ext, lower_bound_attrib->val.kind))
                    {
                      log_infof("[.debug_info@0x%I64x] Array type lower bound is a variable; this is not currently supported.\n", start_off);
                    }
                    else
                    {
                      lower_bound = lower_bound_attrib->val.u128.u64[0];
                    }
                  }
                  
                  // rjf: resolve upper bound
                  U64 upper_bound = 0;
                  {
                    if(count_attrib != &dw2_attrib_nil)
                    {
                      upper_bound = lower_bound + count_attrib->val.u128.u64[0];
                    }
                    else if(upper_bound_attrib != &dw2_attrib_nil)
                    {
                      if(dw_is_form_kind_ref(unit_parse_ctx->version, unit_parse_ctx->ext, upper_bound_attrib->val.kind))
                      {
                        log_infof("[.debug_info@0x%I64x] Array type upper bound is a variable; this is not currently supported.\n", start_off);
                      }
                      else
                      {
                        upper_bound = upper_bound_attrib->val.u128.u64[0];
                        upper_bound += 1; // NOTE(rjf): turn to exclusive range
                      }
                    }
                  }
                  
                  // rjf: push node
                  ArrayDimensionNode *dim_n = push_array(temp.arena, ArrayDimensionNode, 1);
                  SLLStackPush(top_dimension, dim_n);
                  dim_n->count = (upper_bound - lower_bound);
                }
                if(child_tag.has_children)
                {
                  depth += 1;
                }
                else if(child_tag.kind == DW_TagKind_Null)
                {
                  depth -= 1;
                }
                if(off == start_off)
                {
                  break;
                }
              }
            }
            
            // rjf: create array type operators for each dimension
            RDIM_Type *array_direct_type = direct_type;
            for EachNode(dim_n, ArrayDimensionNode, top_dimension)
            {
              RDIM_Type *array_type = rdim_type_chunk_list_push(arena, &lane_types, lane_types_chunk_count);
              array_type->kind = RDI_TypeKind_Array;
              array_type->byte_size = array_direct_type ? array_direct_type->byte_size*dim_n->count : 0;
              array_type->direct_type = array_direct_type;
              array_direct_type = array_type;
            }
            
            // rjf: final destination type -> the final array dimension type
            dst_type = array_direct_type;
          }break;
        }
        
        // rjf: store type in type-from-idx table
        type_from_idx_map[type_idx] = dst_type;
        
        temp_end(temp);
      }
      
      //- rjf: combine all types from all lanes
      RDIM_TypeChunkList *lanes_types = 0;
      if(lane_idx() == 0)
      {
        lanes_types = push_array(scratch2.arena, RDIM_TypeChunkList, lane_count());
      }
      lane_sync_u64(&lanes_types, 0);
      lanes_types[lane_idx()] = lane_types;
      lane_sync();
      if(lane_idx() == 0)
      {
        RDIM_TypeChunkList pass_lane_combined_types = {0};
        for EachIndex(l_idx, lane_count())
        {
          rdim_type_chunk_list_concat_in_place(&pass_lane_combined_types, &lanes_types[l_idx]);
        }
        rdim_type_chunk_list_concat_in_place(all_types, &pass_lane_combined_types);
      }
      lane_sync();
      
      //- rjf: find minimum next max chain count across all lanes, for next iteration
      U64 *lane_next_max_chain_counts = 0;
      if(lane_idx() == 0)
      {
        lane_next_max_chain_counts = push_array(scratch2.arena, U64, lane_count());
      }
      lane_sync_u64(&lane_next_max_chain_counts, 0);
      lane_next_max_chain_counts[lane_idx()] = next_max_chain_count;
      lane_sync();
      if(lane_idx() == 0)
      {
        for EachIndex(l_idx, lane_count())
        {
          next_max_chain_count = Min(next_max_chain_count, lane_next_max_chain_counts[l_idx]);
        }
      }
      lane_sync_u64(&next_max_chain_count, 0);
      
      //- rjf: update max chain count
      max_chain_count = next_max_chain_count;
      
      scratch_end(scratch2);
    }
  }
  lane_sync();
  
  ////////////////////////////
  //- rjf: convert all UDTs
  //
  RDIM_UDTChunkList *all_udts = 0;
  ProfScope("convert all UDTs")
  {
    //- rjf: produce UDTs across all lanes
    U64 chunk_count = 512;
    RDIM_UDTChunkList lane_udts = {0};
    Rng1U64 range = lane_range(type_count);
    for EachInRange(type_idx, range)
    {
      RDIM_Type *type = type_from_idx_map[type_idx];
      if(type->kind == RDI_TypeKind_Struct ||
         type->kind == RDI_TypeKind_Union ||
         type->kind == RDI_TypeKind_Class ||
         type->kind == RDI_TypeKind_Enum)
      {
        // rjf: produce UDT
        RDIM_UDT *udt = rdim_udt_chunk_list_push(arena, &lane_udts, chunk_count);
        type->udt = udt;
        udt->self_type = type;
        
        // rjf: idx -> type tag node
        UniqueTypeTagNode *type_tag_node = type_tag_nodes[type_idx];
        U64 unit_idx = type_tag_node->unit_idx;
        U64 info_off = type_tag_node->info_off;
        
        // rjf: unpack unit
        Rng1U64 unit_info_tag_range = unit_info_tag_ranges[unit_idx];
        DW2_ParseCtx *unit_parse_ctx = &unit_parse_ctxs[unit_idx];
        
        // rjf: parse all tags
        S64 depth = 0;
        for(U64 off = info_off; contains_1u64(unit_info_tag_range, off) && (depth > 0 || off == info_off);)
        {
          Temp scratch2 = scratch_begin(&scratch.arena, 1);
          U64 start_off = off;
          
          // rjf: parse tag
          DW2_Tag tag = {0};
          U64 tag_parse_off = off;
          off += dw2_read_tag(scratch2.arena, unit_parse_ctx, raw->sec[DW_Section_Info].data, off, &tag);
          
          // rjf: unpack tag attributes
          DW2_Attrib *name_attrib = &dw2_attrib_nil;
          DW2_Attrib *type_attrib = &dw2_attrib_nil;
          DW2_Attrib *off_attrib = &dw2_attrib_nil;
          DW2_Attrib *val_attrib = &dw2_attrib_nil;
          DW2_Attrib *declfile_attrib = &dw2_attrib_nil;
          DW2_Attrib *declline_attrib = &dw2_attrib_nil;
          DW2_Attrib *declcol_attrib = &dw2_attrib_nil;
          for EachNode(n, DW2_AttribNode, tag.attribs.first)
          {
            switch(n->v.attrib_kind)
            {
              default:{}break;
#define Case(dst, src) case DW_AttribKind_##src:{dst##_attrib = &n->v;}break
              Case(name,     Name);
              Case(type,     Type);
              Case(off,      DataMemberLocation);
              Case(val,      ConstValue);
              Case(declfile, DeclFile);
              Case(declline, DeclLine);
              Case(declcol,  DeclColumn);
#undef Case
            }
          }
          
          // rjf: unpack basic attributes
          String8 name = name_attrib->val.string;
          U64 off = off_attrib->val.u128.u64[0];
          U64 val = val_attrib->val.u128.u64[0];
          
          // rjf: unpack type
          RDIM_Type *type = 0;
          {
            // rjf: attrib -> info off / unit idx
            U64 type_info_off = dw2_reference_info_off_from_form_val(unit_parse_ctx, &type_attrib->val);
            U64 type_unit_idx = unit_idx;
            if(!contains_1u64(unit_info_tag_range, type_info_off))
            {
              Rng1U64Array unit_info_tag_ranges_array = {unit_info_tag_ranges, unit_count};
              U64 new_unit_num = rng1u64_array_num_from_value__binary_search(&unit_info_tag_ranges_array, type_info_off);
              type_unit_idx = (new_unit_num > 0 ? new_unit_num-1 : unit_idx);
            }
            
            // rjf: (info_off, unit_idx) -> hash
            U64 type_hash = 0;
            {
              UnitTypeMap *type_unit_type_map = &unit_type_maps[type_unit_idx];
              U64 info_off_hash = u64_hash_from_str8(str8_struct(&type_info_off));
              U64 info_off_slot_idx = info_off_hash%type_unit_type_map->slots_count;
              for(UnitTypeNode *n = type_unit_type_map->slots[info_off_slot_idx]; n != 0; n = n->next)
              {
                if(n->src_info_off == type_info_off)
                {
                  type_hash = n->dst_hash;
                  break;
                }
              }
            }
            
            // rjf: hash -> type
            {
              U64 unique_type_tag_slot_idx = type_hash%unique_type_tag_slots_count;
              for(UniqueTypeTagNode *n = unique_type_tag_slots[unique_type_tag_slot_idx]; n != 0; n = n->next)
              {
                if(n->hash == type_hash)
                {
                  type = type_from_idx_map[n->order_idx];
                  break;
                }
              }
            }
          }
          
          // rjf: root-level tag -> collect decl file/line info
          if(tag_parse_off == start_off)
          {
            // TODO(rjf): we need to have gathered the source files before this!!!!!!
            // currently, they only come from line info - we need to gather/dedup the
            // ones that also come from tags.
          }
          
          // rjf: gather children
          switch(tag.kind)
          {
            default:{}break;
            case DW_TagKind_Member:
            {
              RDIM_UDTMember *member = rdim_udt_push_member(arena, &lane_udts, udt);
              member->kind = RDI_MemberKind_DataField;
              member->name = name;
              member->type = type;
              member->off = (RDI_U32)off; // TODO(rjf): @u64_to_u32
            }break;
            case DW_TagKind_Enumerator:
            {
              RDIM_UDTEnumVal *enum_val = rdim_udt_push_enum_val(arena, &lane_udts, udt);
              enum_val->name = name;
              enum_val->val = val;
            }break;
          }
          
          // rjf: tree nav
          if(tag.kind == DW_TagKind_Null)
          {
            depth -= 1;
          }
          if(tag.has_children)
          {
            depth += 1;
          }
          
          scratch_end(scratch2);
          if(off == start_off)
          {
            break;
          }
        }
      }
    }
    lane_sync();
    
    //- rjf: combine all lanes
    RDIM_UDTChunkList *lanes_udts = 0;
    if(lane_idx() == 0)
    {
      lanes_udts = push_array(scratch.arena, RDIM_UDTChunkList, lane_count());
    }
    lane_sync_u64(&lanes_udts, 0);
    lanes_udts[lane_idx()] = lane_udts;
    lane_sync();
    if(lane_idx() == 0)
    {
      all_udts = push_array(scratch.arena, RDIM_UDTChunkList, 1);
      for EachIndex(l_idx, lane_count())
      {
        rdim_udt_chunk_list_concat_in_place(all_udts, &lanes_udts[l_idx]);
      }
    }
    lane_sync_u64(&all_udts, 0);
  }
  
  ////////////////////////////
  //- rjf: convert all units / symbols
  //
  RDIM_UnitChunkList *all_units = 0;
  RDIM_Unit **unit_from_idx_map = 0;
  ProfScope("convert all units / symbols")
  {
    if(lane_idx() == 0)
    {
      all_units = push_array(scratch.arena, RDIM_UnitChunkList, 1);
      unit_from_idx_map = push_array(scratch.arena, RDIM_Unit *, unit_count);
      for EachIndex(unit_idx, unit_count)
      {
        unit_from_idx_map[unit_idx] = rdim_unit_chunk_list_push(arena, all_units, unit_count);
      }
    }
    lane_sync_u64(&all_units, 0);
    lane_sync_u64(&unit_from_idx_map, 0);
    U64 unit_take_idx_ = 0;
    U64 *unit_take_idx_ptr = &unit_take_idx_;
    lane_sync_u64(&unit_take_idx_ptr, 0);
    for(;;)
    {
      U64 unit_idx = ins_atomic_u64_inc_eval(unit_take_idx_ptr) - 1;
      if(unit_idx >= unit_count)
      {
        break;
      }
      
      //- rjf: unpack unit info
      DW2_ParseCtx *unit_parse_ctx = &unit_parse_ctxs[unit_idx];
      Rng1U64 unit_info_tag_range = unit_info_tag_ranges[unit_idx];
      RDIM_Unit *dst_unit = unit_from_idx_map[unit_idx];
      
      //- rjf: unpack unit's root tag
      DW2_Tag *unit_root_tag = &unit_root_tags[unit_idx];
      DW2_Attrib *name_attrib = &dw2_attrib_nil;
      DW2_Attrib *comp_dir_attrib = &dw2_attrib_nil;
      DW2_Attrib *producer_attrib = &dw2_attrib_nil;
      DW2_Attrib *lang_attrib = &dw2_attrib_nil;
      DW2_Attrib *ranges_attrib = &dw2_attrib_nil;
      DW2_Attrib *lopc_attrib = &dw2_attrib_nil;
      DW2_Attrib *hipc_attrib = &dw2_attrib_nil;
      for EachNode(n, DW2_AttribNode, unit_root_tag->attribs.first)
      {
        switch(n->v.attrib_kind)
        {
          default:{}break;
#define Case(dst, src) case DW_AttribKind_##src:{dst##_attrib = &n->v;}break
          Case(name,     Name);
          Case(comp_dir, CompDir);
          Case(producer, Producer);
          Case(lang,     Language);
          Case(ranges,   Ranges);
          Case(lopc,     LowPc);
          Case(hipc,     HighPc);
#undef Case
        }
      }
      
      //- rjf: unpack attributes
      String8 unit_name = name_attrib->val.string;
      String8 unit_comp_dir = comp_dir_attrib->val.string;
      String8 unit_producer = producer_attrib->val.string;
      RDI_Language unit_lang = RDI_Language_NULL;
      {
        DW_Language dw_lang = lang_attrib->val.u128.u64[0];
        switch(dw_lang)
        {
          default:{}break;
          case DW_Language_C89:
          case DW_Language_C99:
          case DW_Language_C11:
          case DW_Language_C:
          {
            unit_lang = RDI_Language_C;
          }break;
          case DW_Language_CPlusPlus03:
          case DW_Language_CPlusPlus11:
          case DW_Language_CPlusPlus14:
          case DW_Language_CPlusPlus:
          {
            unit_lang = RDI_Language_CPlusPlus;
          }break;
        }
      }
      
      //- rjf: get unit's ranges from .debug_aranges parse artifacts, if we have them
      RDIM_Rng1U64ChunkList unit_voff_ranges = {0};
      if(arange_unit_from_info_off_map_slots_count != 0)
      {
        U64 unit_info_off = unit_info_ranges->v[unit_idx].min;
        U64 hash = u64_hash_from_str8(str8_struct(&unit_info_off));
        U64 slot_idx = hash%arange_unit_from_info_off_map_slots_count;
        for(D2R2_ARangeUnitNode *n = arange_unit_from_info_off_map_slots[slot_idx]; n != 0; n = n->next)
        {
          if(n->info_off == unit_info_off)
          {
            unit_voff_ranges = n->ranges[0];
            break;
          }
        }
      }
      
      //- rjf: if we have no voff ranges from .debug_aranges, then we need to extract
      // this info from the unit root rag instead (via ranges & low-pc/high-pc tags)
      if(unit_voff_ranges.total_count == 0)
      {
        // rjf: gather ranges from a ranges attribute
        if(ranges_attrib != &dw2_attrib_nil)
        {
          Rng1U64List ranges = dw2_rnglist_from_form_val(scratch.arena, unit_parse_ctx, raw, ranges_attrib->val);
          for EachNode(n, Rng1U64Node, ranges.first)
          {
            rdim_rng1u64_chunk_list_push(arena, &unit_voff_ranges, 256, (RDIM_Rng1U64){n->v.min - base_vaddr, n->v.max - base_vaddr});
          }
        }
        
        // rjf: gather contiguous range from low-pc / high-pc attribute
        if(lopc_attrib != &dw2_attrib_nil && hipc_attrib != &dw2_attrib_nil)
        {
          U64 voff_base = lopc_attrib->val.addr - base_vaddr;
          U64 voff_opl = 0;
          if(dw_attrib_class_from_form_kind(unit_parse_ctx->version, hipc_attrib->val.kind) & (1<<DW_AttribClass_Address))
          {
            voff_opl = voff_base + hipc_attrib->val.u128.u64[0];
          }
          else
          {
            voff_opl = hipc_attrib->val.addr;
          }
          rdim_rng1u64_chunk_list_push(arena, &unit_voff_ranges, 256, (RDIM_Rng1U64){voff_base, voff_opl});
        }
      }
      
      //- rjf: fill top-level unit info
      {
        dst_unit->unit_name     = unit_name;
        dst_unit->compiler_name = unit_producer;
        // TODO(rjf): dst_unit->source_file   = ???;
        // TODO(rjf): dst_unit->object_file   = ???;
        // TODO(rjf): dst_unit->archive_file  = ???;
        dst_unit->build_path    = unit_comp_dir;
        dst_unit->language      = unit_lang;
        dst_unit->line_table    = unit_line_tables[unit_idx];
        dst_unit->voff_ranges   = unit_voff_ranges;
      }
      
      //- rjf: produce all unit symbols
      typedef struct D2R2_ScopeNode D2R2_ScopeNode;
      struct D2R2_ScopeNode
      {
        D2R2_ScopeNode *next;
        RDIM_Scope *scope;
        RDIM_LocationCaseList framebase_location_cases;
      };
      D2R2_ScopeNode *top_scope = 0;
      D2R2_ScopeNode *free_scope = 0;
      U64 chunk_count = 512;
      for(U64 off = unit_info_tag_range.min; off < unit_info_tag_range.max;)
      {
        Temp scratch2 = scratch_begin(&scratch.arena, 1);
        U64 start_off = off;
        
        ////////////////////////
        //- rjf: parse tag
        //
        DW2_Tag tag = {0};
        off += dw2_read_tag(scratch2.arena, unit_parse_ctx, raw->sec[DW_Section_Info].data, off, &tag);
        
        ////////////////////////
        //- rjf: gather attributes from tag
        //
        DW2_Attrib *name_attrib = &dw2_attrib_nil;
        DW2_Attrib *linkname_attrib = &dw2_attrib_nil;
        DW2_Attrib *type_attrib = &dw2_attrib_nil;
        DW2_Attrib *inline_attrib = &dw2_attrib_nil;
        DW2_Attrib *ranges_attrib = &dw2_attrib_nil;
        DW2_Attrib *lopc_attrib = &dw2_attrib_nil;
        DW2_Attrib *hipc_attrib = &dw2_attrib_nil;
        DW2_Attrib *constval_attrib = &dw2_attrib_nil;
        DW2_Attrib *location_attrib = &dw2_attrib_nil;
        DW2_Attrib *framebase_attrib = &dw2_attrib_nil;
        DW2_Attrib *external_attrib = &dw2_attrib_nil;
        for EachNode(n, DW2_AttribNode, tag.attribs.first)
        {
          switch(n->v.attrib_kind)
          {
            default:{}break;
#define Case(dst, src) case DW_AttribKind_##src:{dst##_attrib = &n->v;}break
            Case(name,      Name);
            Case(linkname,  LinkageName);
            Case(type,      Type);
            Case(inline,    Inline);
            Case(ranges,    Ranges);
            Case(lopc,      LowPc);
            Case(hipc,      HighPc);
            Case(constval,  ConstValue);
            Case(location,  Location);
            Case(framebase, FrameBase);
            Case(external,  External);
#undef Case
          }
        }
        
        ////////////////////////
        //- rjf: unpack basic attributes
        //
        String8 name = name_attrib->val.string;
        String8 link_name = linkname_attrib->val.string;
        DW_InlKind inl_kind = (DW_InlKind)inline_attrib->val.u128.u64[0];
        B32 is_external = (external_attrib != &dw2_attrib_nil);
        
        ////////////////////////
        //- rjf: unpack ranges
        //
        RDIM_Rng1U64List ranges = {0};
        {
          if(ranges_attrib != &dw2_attrib_nil)
          {
            Temp temp = temp_begin(scratch2.arena);
            Rng1U64List tag_ranges = dw2_rnglist_from_form_val(temp.arena, unit_parse_ctx, raw, ranges_attrib->val);
            for EachNode(n, Rng1U64Node, tag_ranges.first)
            {
              rdim_rng1u64_list_push(arena, &ranges, (RDIM_Rng1U64){.min = n->v.min, .max = n->v.max});
            }
            temp_end(temp);
          }
          if(lopc_attrib != &dw2_attrib_nil && hipc_attrib != &dw2_attrib_nil)
          {
            U64 voff_base = lopc_attrib->val.addr - base_vaddr;
            U64 voff_opl = 0;
            if(dw_attrib_class_from_form_kind(unit_parse_ctx->version, hipc_attrib->val.kind) & (1<<DW_AttribClass_Address))
            {
              voff_opl = voff_base + hipc_attrib->val.u128.u64[0];
            }
            else
            {
              voff_opl = hipc_attrib->val.addr;
            }
            rdim_rng1u64_list_push(arena, &ranges, (RDIM_Rng1U64){voff_base, voff_opl});
          }
        }
        
        ////////////////////////
        //- rjf: unpack type
        //
        RDIM_Type *type = 0;
        if(type_attrib != &dw2_attrib_nil)
        {
          // rjf: functions need to get their type info from themselves; the type attrib
          // only references the return type. so, in that case, we'll use the procedure's
          // own info offset / unit index, rather than the type attribute's.
          U64 type_info_off = start_off;
          U64 type_unit_idx = unit_idx;
          
          // rjf: attrib -> (info_off, unit_idx)
          if(tag.kind != DW_TagKind_SubProgram)
          {
            type_info_off = dw2_reference_info_off_from_form_val(unit_parse_ctx, &type_attrib->val);
            type_unit_idx = unit_idx;
            if(!contains_1u64(unit_info_tag_range, type_info_off))
            {
              Rng1U64Array unit_info_tag_ranges_array = {unit_info_tag_ranges, unit_count};
              U64 new_unit_num = rng1u64_array_num_from_value__binary_search(&unit_info_tag_ranges_array, type_info_off);
              type_unit_idx = (new_unit_num > 0 ? new_unit_num-1 : unit_idx);
            }
          }
          
          // rjf: (info_off, unit_idx) -> hash
          U64 type_hash = 0;
          {
            UnitTypeMap *type_unit_type_map = &unit_type_maps[type_unit_idx];
            U64 info_off_hash = u64_hash_from_str8(str8_struct(&type_info_off));
            U64 info_off_slot_idx = info_off_hash%type_unit_type_map->slots_count;
            for(UnitTypeNode *n = type_unit_type_map->slots[info_off_slot_idx]; n != 0; n = n->next)
            {
              if(n->src_info_off == type_info_off)
              {
                type_hash = n->dst_hash;
                break;
              }
            }
          }
          
          // rjf: hash -> type
          {
            U64 unique_type_tag_slot_idx = type_hash%unique_type_tag_slots_count;
            for(UniqueTypeTagNode *n = unique_type_tag_slots[unique_type_tag_slot_idx]; n != 0; n = n->next)
            {
              if(n->hash == type_hash)
              {
                type = type_from_idx_map[n->order_idx];
                break;
              }
            }
          }
        }
        
        ////////////////////////
        //- rjf: unpack location info
        //
        RDIM_LocationCaseList location_cases = {0};
        RDIM_LocationCaseList framebase_location_cases = {0};
        B32 location_is_tls_dependent = 0;
        B32 framebase_location_is_tls_dependent = 0;
        {
          struct
          {
            DW2_Attrib *attrib;
            RDIM_LocationCaseList *dst_locations;
            B32 *dst_is_tls_dependent;
          }
          tasks[] =
          {
            {location_attrib, &location_cases, &location_is_tls_dependent},
            {framebase_attrib, &framebase_location_cases, &framebase_location_is_tls_dependent},
          };
          for EachElement(task_idx, tasks)
          {
            if(tasks[task_idx].attrib == &dw2_attrib_nil)
            {
              continue;
            }
            DW2_Attrib *attrib = tasks[task_idx].attrib;
            RDIM_LocationCaseList *dst_locations = tasks[task_idx].dst_locations;
            B32 *dst_is_tls_dependent = tasks[task_idx].dst_is_tls_dependent;
            
            //////////////////////
            //- rjf: gather DWARF location exprs
            //
            DW2_LocList locs = {0};
            switch(attrib->val.kind)
            {
              default:{}break;
              case DW_Form_ExprLoc:
              {
                U64 expr_info_off  = attrib->val.u128.u64[0];
                U64 expr_info_size = attrib->val.u128.u64[1];
                String8 expr = str8_substr(raw->sec[DW_Section_Info].data, r1u64(expr_info_off, expr_info_off+expr_info_size));
                DW2_LocNode *n = push_array(scratch2.arena, DW2_LocNode, 1);
                n->v.expr = expr;
                n->v.range = r1u64(0, max_U64);
                SLLQueuePush(locs.first, locs.last, n);
                locs.count += 1;
              }break;
              case DW_Form_LocListx:   // NOTE(rjf): dwarf5+, loclistx -> location list offset table
              case DW_Form_SecOffset:  // NOTE(rjf): pre-dwarf5, location section offset -> a location list
              {
                locs = dw2_loclist_from_form_val(scratch2.arena, unit_parse_ctx, raw, attrib->val);
              }break;
            }
            
            //////////////////////
            //- rjf: convert DWARF location exprs -> RDIM locations
            //
            // we do this for each possible frame base location case - locations in DWARF
            // refer to the frame base via a special op. in our case, we want to bake that
            // into the location directly to reduce extra context for evaluation.
            //
            // if we find that a location doesn't rely on the frame base at all, we just
            // skip the rest.
            //
            B32 is_tls_dependent = 0;
            for EachNode(n, DW2_LocNode, locs.first)
            {
              //- rjf: unpack location
              Rng1U64 range = n->v.range;
              String8 expr = n->v.expr;
              
              //- rjf: iterate each frame base location case, or once if there are none,
              // and convert the location in that context
              RDIM_LocationCase nil_framebase_loc_case = {0, {0}, {0, max_U64}};
              for(RDIM_LocationCase *framebase_loc_n = top_scope ? top_scope->framebase_location_cases.first : &nil_framebase_loc_case;
                  framebase_loc_n != 0;
                  framebase_loc_n = framebase_loc_n->next)
              {
                //- rjf: unpack framebase location
                RDIM_Location framebase_loc = framebase_loc_n->location;
                RDIM_Rng1U64 framebase_voff_range = framebase_loc_n->voff_range;
                
                //- rjf: set up type stack for type-evaluating bytecode
                typedef struct D2R2_ExprVal D2R2_ExprVal;
                struct D2R2_ExprVal
                {
                  RDI_TypeKind type_kind;
                  B32 is_addr;
                };
                typedef struct D2R2_ExprValNode D2R2_ExprValNode;
                struct D2R2_ExprValNode
                {
                  D2R2_ExprValNode *next;
                  D2R2_ExprVal v;
                };
                D2R2_ExprValNode *val_stack_top = 0;
                D2R2_ExprValNode *free_val = 0;
                
                //- rjf: process DWARF bytecode; produce RDI bytecode + mini-type-info for the expression's result
                typedef struct JumpOpNode JumpOpNode;
                struct JumpOpNode
                {
                  JumpOpNode *next;
                  RDIM_EvalBytecodeOp *op;
                  S64 inst_delta;
                };
                JumpOpNode *first_jump_op = 0;
                JumpOpNode *last_jump_op = 0;
                RDIM_EvalBytecode dst_bytecode = {0};
                B32 dst_bytecode_is_good = 1;
                B32 bytecode_is_framebase_dependent = 0;
                for(U64 expr_off = 0; expr_off < expr.size;)
                {
                  U64 start_expr_off = expr_off;
                  
                  //- rjf: read next opcode
                  DW_ExprOp opcode = 0;
                  expr_off += str8_deserial_read_struct(expr, expr_off, &opcode);
                  
                  //- rjf: read op operands
                  U64 operands_count = dw_operand_count_from_expr_op(opcode);
                  DW_ExprOperandType *operands_types = dw_operand_types_from_expr_opcode(opcode);
                  U64 operand_u64s[2] = {0};
                  S64 operand_s64s[2] = {0};
                  String8 operand_str8s[2] = {0};
                  for EachIndex(operand_idx, operands_count)
                  {
                    switch(operands_types[operand_idx])
                    {
                      case DW_ExprOperandType_Null:
                      default:{}break;
                      case DW_ExprOperandType_U8:        {expr_off += str8_deserial_read(expr, expr_off, &operand_u64s[operand_idx], 1, 1);}break;
                      case DW_ExprOperandType_U16:       {expr_off += str8_deserial_read(expr, expr_off, &operand_u64s[operand_idx], 2, 2);}break;
                      case DW_ExprOperandType_U32:       {expr_off += str8_deserial_read(expr, expr_off, &operand_u64s[operand_idx], 4, 4);}break;
                      case DW_ExprOperandType_U64:       {expr_off += str8_deserial_read(expr, expr_off, &operand_u64s[operand_idx], 8, 8);}break;
                      case DW_ExprOperandType_S8:        {expr_off += str8_deserial_read(expr, expr_off, &operand_s64s[operand_idx], 1, 1);}break;
                      case DW_ExprOperandType_S16:       {expr_off += str8_deserial_read(expr, expr_off, &operand_s64s[operand_idx], 2, 2);}break;
                      case DW_ExprOperandType_S32:       {expr_off += str8_deserial_read(expr, expr_off, &operand_s64s[operand_idx], 4, 4);}break;
                      case DW_ExprOperandType_S64:       {expr_off += str8_deserial_read(expr, expr_off, &operand_s64s[operand_idx], 8, 8);}break;
                      case DW_ExprOperandType_ULEB128:   {expr_off += str8_deserial_read_uleb128(expr, expr_off, &operand_u64s[operand_idx]);} break;
                      case DW_ExprOperandType_SLEB128:   {expr_off += str8_deserial_read_sleb128(expr, expr_off, &operand_s64s[operand_idx]);} break;
                      case DW_ExprOperandType_Addr:      {expr_off += str8_deserial_read(expr, expr_off, &operand_u64s[operand_idx], unit_parse_ctx->addr_size, unit_parse_ctx->addr_size);}break;
                      case DW_ExprOperandType_DwarfUInt: {expr_off += dw2_read_fmt_u64(expr, expr_off, unit_parse_ctx->format, &operand_u64s[operand_idx]);}break;
                      case DW_ExprOperandType_Block:
                      {
                        U8 block_size = 0;
                        expr_off += str8_deserial_read_struct(expr, expr_off, &block_size);
                        operand_str8s[operand_idx] = str8_substr(expr, r1u64(expr_off, expr_off+block_size));
                        expr_off += block_size;
                      }break;
                    }
                  }
                  
                  //- rjf: pop stack values
                  D2R2_ExprVal popped_vals[2] = {0};
                  {
                    U64 pop_count = dw_pop_count_from_expr_op(opcode);
                    pop_count = Min(pop_count, ArrayCount(popped_vals));
                    for EachIndex(pop_idx, pop_count)
                    {
                      if(val_stack_top != 0)
                      {
                        D2R2_ExprValNode *popped = val_stack_top;
                        popped_vals[pop_idx] = popped->v;
                        SLLStackPop(val_stack_top);
                        SLLStackPush(free_val, popped);
                      }
                    }
                  }
                  
                  //- rjf: exec op (produce RDI bytecode, + manipulate value stack)
                  D2R2_ExprVal push_vals[2] = {0};
                  U64 regcode_dw = 0;
                  S64 regval_off = 0;
                  B32 regread_is_addr = 0;
                  U64 target_pick_val_idx = 0;
                  RDI_EvalOp rdi_eval_op = 0;
                  U64 memread_size = 0;
                  switch(opcode)
                  {
                    default:{}break;
                    
                    //- rjf: small opcode-embedded unsigned literals
                    case DW_ExprOp_Lit0:  case DW_ExprOp_Lit1:  case DW_ExprOp_Lit2:
                    case DW_ExprOp_Lit3:  case DW_ExprOp_Lit4:  case DW_ExprOp_Lit5:
                    case DW_ExprOp_Lit6:  case DW_ExprOp_Lit7:  case DW_ExprOp_Lit8:
                    case DW_ExprOp_Lit9:  case DW_ExprOp_Lit10: case DW_ExprOp_Lit11:
                    case DW_ExprOp_Lit12: case DW_ExprOp_Lit13: case DW_ExprOp_Lit14:
                    case DW_ExprOp_Lit15: case DW_ExprOp_Lit16: case DW_ExprOp_Lit17:
                    case DW_ExprOp_Lit18: case DW_ExprOp_Lit19: case DW_ExprOp_Lit20:
                    case DW_ExprOp_Lit21: case DW_ExprOp_Lit22: case DW_ExprOp_Lit23:
                    case DW_ExprOp_Lit24: case DW_ExprOp_Lit25: case DW_ExprOp_Lit26:
                    case DW_ExprOp_Lit27: case DW_ExprOp_Lit28: case DW_ExprOp_Lit29:
                    case DW_ExprOp_Lit30: case DW_ExprOp_Lit31:
                    {
                      U64 lit_val = (U64)(opcode - DW_ExprOp_Lit0);
                      rdim_bytecode_push_uconst(arena, &dst_bytecode, lit_val);
                    }break;
                    
                    //- rjf: small unsigned literals
                    case DW_ExprOp_Const1U: push_vals[0].type_kind = RDI_TypeKind_U8;  goto const_u;
                    case DW_ExprOp_Const2U: push_vals[0].type_kind = RDI_TypeKind_U16; goto const_u;
                    case DW_ExprOp_Const4U: push_vals[0].type_kind = RDI_TypeKind_U32; goto const_u;
                    case DW_ExprOp_Const8U: push_vals[0].type_kind = RDI_TypeKind_U64; goto const_u;
                    case DW_ExprOp_ConstU:  push_vals[0].type_kind = RDI_TypeKind_U64; goto const_u;
                    const_u:;
                    {
                      rdim_bytecode_push_uconst(arena, &dst_bytecode, operand_u64s[0]);
                    }break;
                    
                    //- rjf: small signed literals
                    case DW_ExprOp_Const1S: push_vals[0].type_kind = RDI_TypeKind_S8;  goto const_s;
                    case DW_ExprOp_Const2S: push_vals[0].type_kind = RDI_TypeKind_S16; goto const_s;
                    case DW_ExprOp_Const4S: push_vals[0].type_kind = RDI_TypeKind_S32; goto const_s;
                    case DW_ExprOp_Const8S: push_vals[0].type_kind = RDI_TypeKind_S64; goto const_s;
                    case DW_ExprOp_ConstS : push_vals[0].type_kind = RDI_TypeKind_S64; goto const_s;
                    const_s:;
                    {
                      rdim_bytecode_push_sconst(arena, &dst_bytecode, operand_s64s[0]);
                    }break;
                    
                    //- rjf: address (module offsets)
                    case DW_ExprOp_Addr:
                    {
                      U64 voff = operand_u64s[0] - base_vaddr;
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_ModuleOff, voff);
                      push_vals[0].type_kind = RDI_TypeKind_U64;
                      push_vals[0].is_addr = 1;
                    }break;
                    case DW_ExprOp_Addrx:
                    if(unit_parse_ctx->addr_table != 0)
                    {
                      U64 addr_idx = operand_u64s[0];
                      U64 addr = 0;
                      if(dw2_try_offset_from_table_idx(unit_parse_ctx->addr_table, addr_idx, &addr))
                      {
                        U64 voff = (addr > base_vaddr) ? (addr - base_vaddr) : 0;
                        rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_ModuleOff, voff);
                        push_vals[0].type_kind = RDI_TypeKind_U64;
                        push_vals[0].is_addr = 1;
                      }
                    }break;
                    
                    //- rjf: register reads
                    case DW_ExprOp_Reg0:  case DW_ExprOp_Reg1:  case DW_ExprOp_Reg2:
                    case DW_ExprOp_Reg3:  case DW_ExprOp_Reg4:  case DW_ExprOp_Reg5:
                    case DW_ExprOp_Reg6:  case DW_ExprOp_Reg7:  case DW_ExprOp_Reg8:
                    case DW_ExprOp_Reg9:  case DW_ExprOp_Reg10: case DW_ExprOp_Reg11:
                    case DW_ExprOp_Reg12: case DW_ExprOp_Reg13: case DW_ExprOp_Reg14:
                    case DW_ExprOp_Reg15: case DW_ExprOp_Reg16: case DW_ExprOp_Reg17:
                    case DW_ExprOp_Reg18: case DW_ExprOp_Reg19: case DW_ExprOp_Reg20:
                    case DW_ExprOp_Reg21: case DW_ExprOp_Reg22: case DW_ExprOp_Reg23:
                    case DW_ExprOp_Reg24: case DW_ExprOp_Reg25: case DW_ExprOp_Reg26:
                    case DW_ExprOp_Reg27: case DW_ExprOp_Reg28: case DW_ExprOp_Reg29:
                    case DW_ExprOp_Reg30: case DW_ExprOp_Reg31:
                    {
                      regcode_dw = (U64)(opcode - DW_ExprOp_Reg0);
                    }goto reg_read;
                    case DW_ExprOp_RegX:
                    {
                      regcode_dw = operand_u64s[0];
                    }goto reg_read;
                    case DW_ExprOp_BReg0:  case DW_ExprOp_BReg1:  case DW_ExprOp_BReg2:
                    case DW_ExprOp_BReg3:  case DW_ExprOp_BReg4:  case DW_ExprOp_BReg5:
                    case DW_ExprOp_BReg6:  case DW_ExprOp_BReg7:  case DW_ExprOp_BReg8:
                    case DW_ExprOp_BReg9:  case DW_ExprOp_BReg10: case DW_ExprOp_BReg11:
                    case DW_ExprOp_BReg12: case DW_ExprOp_BReg13: case DW_ExprOp_BReg14:
                    case DW_ExprOp_BReg15: case DW_ExprOp_BReg16: case DW_ExprOp_BReg17:
                    case DW_ExprOp_BReg18: case DW_ExprOp_BReg19: case DW_ExprOp_BReg20:
                    case DW_ExprOp_BReg21: case DW_ExprOp_BReg22: case DW_ExprOp_BReg23:
                    case DW_ExprOp_BReg24: case DW_ExprOp_BReg25: case DW_ExprOp_BReg26:
                    case DW_ExprOp_BReg27: case DW_ExprOp_BReg28: case DW_ExprOp_BReg29:
                    case DW_ExprOp_BReg30: case DW_ExprOp_BReg31:
                    {
                      regcode_dw = (U64)(opcode - DW_ExprOp_BReg0);
                      regval_off = operand_s64s[1];
                      regread_is_addr = 1;
                    }goto reg_read;
                    case DW_ExprOp_BRegX:
                    {
                      regcode_dw = operand_u64s[0];
                      regval_off = operand_s64s[1];
                      regread_is_addr = 1;
                    }goto reg_read;
                    reg_read:;
                    {
                      // rjf: DWARF regcode -> RDI, off, size
                      RDI_RegCode regcode_rdi = 0;
                      U64 reg_off = 0;
                      U64 reg_size = 0;
                      switch((Arch)arch)
                      {
                        case Arch_Null:
                        case Arch_COUNT:
                        {}break;
                        case Arch_arm32:
                        case Arch_arm64:
                        case Arch_x86:
                        {
                          // TODO(rjf): unsupported architectures
                        }break;
                        case Arch_x64:
                        {
                          switch(regcode_dw)
                          {
                            default:{}break;
#define X(reg_dw, val_dw, reg_rdi, off, size) case DW_RegX64_##reg_dw:{regcode_rdi = RDI_RegCodeX64_##reg_rdi; reg_off = (off); reg_size = (size);}break;
                            DW_Regs_X64_XList
#undef X
                          }
                        }break;
                      }
                      
                      // rjf: push op
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_RegRead, RDI_EncodeRegReadParam(regcode_rdi, reg_size, reg_off));
                      
                      // rjf: push add to value offset, if needed
                      if(regval_off != 0)
                      {
                        rdim_bytecode_push_sconst(arena, &dst_bytecode, regval_off);
                        rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_Add, RDI_EvalTypeGroup_S);
                      }
                      
                      // rjf: choose if we're doing a float op
                      B32 is_float_op = 0;
                      if(arch == Arch_x64 &&
                         (regcode_rdi == RDI_RegCodeX64_st0 ||
                          regcode_rdi == RDI_RegCodeX64_st1 ||
                          regcode_rdi == RDI_RegCodeX64_st2 ||
                          regcode_rdi == RDI_RegCodeX64_st3 ||
                          regcode_rdi == RDI_RegCodeX64_st4 ||
                          regcode_rdi == RDI_RegCodeX64_st5 ||
                          regcode_rdi == RDI_RegCodeX64_st6 ||
                          regcode_rdi == RDI_RegCodeX64_st7))
                      {
                        is_float_op = 1;
                      }
                      
                      // rjf: set up push value
                      push_vals[0].is_addr = regread_is_addr;
                      if(is_float_op)
                      {
                        switch(reg_size)
                        {
                          default:{}break;
                          case 2: {push_vals[0].type_kind = RDI_TypeKind_F16;}break;
                          case 4: {push_vals[0].type_kind = RDI_TypeKind_F32;}break;
                          case 6: {push_vals[0].type_kind = RDI_TypeKind_F48;}break;
                          case 8: {push_vals[0].type_kind = RDI_TypeKind_F64;}break;
                          case 10:{push_vals[0].type_kind = RDI_TypeKind_F80;}break;
                          case 12:{push_vals[0].type_kind = RDI_TypeKind_F96;}break;
                          case 16:{push_vals[0].type_kind = RDI_TypeKind_F128;}break;
                        }
                      }
                      else
                      {
                        switch(reg_size)
                        {
                          default:{}break;
                          case 1:{push_vals[0].type_kind = RDI_TypeKind_U8;}break;
                          case 2:{push_vals[0].type_kind = RDI_TypeKind_U16;}break;
                          case 4:{push_vals[0].type_kind = RDI_TypeKind_U32;}break;
                          case 8:{push_vals[0].type_kind = RDI_TypeKind_U64;}break;
                        }
                      }
                    }break;
                    
                    //- rjf: implicit values
                    case DW_ExprOp_ImplicitValue:
                    {
                      if(operand_str8s[0].size <= sizeof(U64))
                      {
                        U64 implicit_value = 0;
                        MemoryCopy(&implicit_value, operand_str8s[0].str, Min(sizeof(U64), operand_str8s[0].size));
                        rdim_bytecode_push_uconst(arena, &dst_bytecode, implicit_value);
                        switch(operand_str8s[0].size)
                        {
                          default:{}break;
                          case 1:{push_vals[0].type_kind = RDI_TypeKind_U8;}break;
                          case 2:{push_vals[0].type_kind = RDI_TypeKind_U16;}break;
                          case 4:{push_vals[0].type_kind = RDI_TypeKind_U32;}break;
                          case 8:{push_vals[0].type_kind = RDI_TypeKind_U64;}break;
                        }
                      }
                      else
                      {
                        log_infof("[.debug_info@0x%I64x] Implicit value DWARF expression operation (DW_ExprOp_ImplicitValue) with block size >8 (%I64u) found. This is not currently supported.\n", start_off, operand_str8s[0].size);
                      }
                    }break;
                    
                    //- rjf: pieces
                    case DW_ExprOp_Piece:
                    {
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_PartialValue, operand_u64s[0]);
                    }break;
                    case DW_ExprOp_BitPiece:
                    {
                      U64 size = operand_u64s[0];
                      U64 off = operand_u64s[1];
                      U64 partial_value = ((size<<32)|(off));
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_PartialValueBit, partial_value);
                    }break;
                    
                    //- rjf: stack ops
                    case DW_ExprOp_Over: {target_pick_val_idx = 1;}goto pick;
                    case DW_ExprOp_Pick: {target_pick_val_idx = operand_u64s[0];}goto pick;
                    case DW_ExprOp_Dup:  {target_pick_val_idx = 0;}goto pick;
                    pick:;
                    {
                      // rjf: pick value from stack
                      D2R2_ExprVal picked_val = {0};
                      {
                        U64 idx = 0;
                        for(D2R2_ExprValNode *n = val_stack_top; n != 0; n = n->next)
                        {
                          if(idx == target_pick_val_idx)
                          {
                            picked_val = n->v;
                            break;
                          }
                        }
                      }
                      
                      // rjf: push opcode
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_Pick, target_pick_val_idx);
                      
                      // rjf: push val
                      push_vals[0] = picked_val;
                    }break;
                    case DW_ExprOp_Swap:
                    {
                      push_vals[0] = popped_vals[1];
                      push_vals[1] = popped_vals[0];
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_Swap, 0);
                    }break;
                    case DW_ExprOp_Drop:
                    {
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_Pop, 0);
                    }break;
                    
                    //- rjf: jumps
                    case DW_ExprOp_Skip: rdi_eval_op = RDI_EvalOp_Skip; goto jumps;
                    case DW_ExprOp_Bra:  rdi_eval_op = RDI_EvalOp_Cond; goto jumps;
                    jumps:;
                    {
                      // rjf: jump delta in bytes (how the instruction is encoded) -> jump delta in instructions
                      S64 jump_delta_bytes = operand_s64s[0];
                      S64 jump_delta_insts = 0;
                      {
                        // TODO(rjf): expr op jumps bytes -> inst counts
                      }
                      
                      // rjf: push incomplete op (we need to resolve inst delta -> byte delta later, once we have the full bytecode stream)
                      RDIM_EvalBytecodeOp *op = rdim_bytecode_push_op(arena, &dst_bytecode, rdi_eval_op, 0);
                      
                      // rjf: gather op
                      JumpOpNode *n = push_array(scratch2.arena, JumpOpNode, 1);
                      SLLQueuePush(first_jump_op, last_jump_op, n);
                      n->op = op;
                      n->inst_delta = jump_delta_insts;
                    }break;
                    
                    //- rjf: containing procedure frame offsets
                    case DW_ExprOp_FBReg:
                    {
                      bytecode_is_framebase_dependent = 1;
                      for EachNode(n, RDIM_EvalBytecodeOp, framebase_loc.bytecode.first_op)
                      {
                        rdim_bytecode_push_op(arena, &dst_bytecode, n->op, n->p);
                      }
                      rdim_bytecode_push_sconst(arena, &dst_bytecode, operand_s64s[0]);
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_Add, RDI_EvalTypeGroup_S);
                      push_vals[0].type_kind = RDI_TypeKind_U64;
                      push_vals[0].is_addr = 1;
                    }break;
                    
                    //- rjf: memory reads
                    case DW_ExprOp_Deref: memread_size = unit_parse_ctx->addr_size; goto deref;
                    case DW_ExprOp_DerefSize: memread_size = operand_u64s[0]; goto deref;
                    deref:;
                    {
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_MemRead, memread_size);
                      push_vals[0].is_addr = 1;
                      push_vals[0].type_kind = RDI_TypeKind_U64;
                    }break;
                    
                    //- rjf: TLS offsets
                    case DW_ExprOp_FormTlsAddress:
                    case DW_ExprOp_GNU_PushTlsAddress:
                    {
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_TLSOff, 0);
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_Add, RDI_EvalTypeGroup_U);
                      push_vals[0].is_addr = 1;
                      push_vals[0].type_kind = RDI_TypeKind_U64;
                      is_tls_dependent = 1;
                    }break;
                    
                    //- rjf: call site value
                    case DW_ExprOp_EntryValue:
                    case DW_ExprOp_GNU_EntryValue:
                    {
                      // TODO(rjf): expr op entry value ops
                    }break;
                    
                    //- rjf: fixed adds
                    case DW_ExprOp_PlusUConst:
                    {
                      rdim_bytecode_push_uconst(arena, &dst_bytecode, operand_u64s[0]);
                      rdim_bytecode_push_op(arena, &dst_bytecode, RDI_EvalOp_Add, RDI_EvalTypeGroup_U);
                      push_vals[0].type_kind = RDI_TypeKind_U64; // TODO(rjf): do we need to adjust this based on popped value at all, even?
                    }break;
                    
                    //- rjf: arithmetic ops
                    case DW_ExprOp_Eq:     {rdi_eval_op = RDI_EvalOp_EqEq;}goto arithmetic_op;
                    case DW_ExprOp_Ge:     {rdi_eval_op = RDI_EvalOp_GrEq;}goto arithmetic_op;
                    case DW_ExprOp_Gt:     {rdi_eval_op = RDI_EvalOp_Grtr;}goto arithmetic_op;
                    case DW_ExprOp_Le:     {rdi_eval_op = RDI_EvalOp_LsEq;}goto arithmetic_op;
                    case DW_ExprOp_Lt:     {rdi_eval_op = RDI_EvalOp_Less;}goto arithmetic_op;
                    case DW_ExprOp_Ne:     {rdi_eval_op = RDI_EvalOp_Neg;}goto arithmetic_op;
                    case DW_ExprOp_Div:    {rdi_eval_op = RDI_EvalOp_Div;}goto arithmetic_op;
                    case DW_ExprOp_Minus:  {rdi_eval_op = RDI_EvalOp_Sub;}goto arithmetic_op;
                    case DW_ExprOp_Mul:    {rdi_eval_op = RDI_EvalOp_Mul;}goto arithmetic_op;
                    case DW_ExprOp_Plus:   {rdi_eval_op = RDI_EvalOp_Add;}goto arithmetic_op;
                    case DW_ExprOp_Xor:    {rdi_eval_op = RDI_EvalOp_BitXor;}goto arithmetic_op;
                    case DW_ExprOp_And:    {rdi_eval_op = RDI_EvalOp_BitAnd;}goto arithmetic_op;
                    case DW_ExprOp_Or:     {rdi_eval_op = RDI_EvalOp_BitOr;}goto arithmetic_op;
                    case DW_ExprOp_Shl:    {rdi_eval_op = RDI_EvalOp_LShift;}goto arithmetic_op;
                    case DW_ExprOp_Shr:    {rdi_eval_op = RDI_EvalOp_RShift;}goto arithmetic_op;
                    case DW_ExprOp_Shra:   {rdi_eval_op = RDI_EvalOp_RShift;}goto arithmetic_op;
                    case DW_ExprOp_Mod:    {rdi_eval_op = RDI_EvalOp_Mod;}goto arithmetic_op;
                    case DW_ExprOp_Abs:    {rdi_eval_op = RDI_EvalOp_Abs;}goto arithmetic_op;
                    case DW_ExprOp_Neg:    {rdi_eval_op = RDI_EvalOp_Neg;}goto arithmetic_op;
                    case DW_ExprOp_Not:    {rdi_eval_op = RDI_EvalOp_LogNot;}goto arithmetic_op;
                    arithmetic_op:;
                    {
                      // TODO(rjf): eval arithmetic conversions etc.
                      rdim_bytecode_push_op(arena, &dst_bytecode, rdi_eval_op, RDI_EvalTypeGroup_U);
                    }break;
                    
                    //- rjf: currently unsupported
                    case DW_ExprOp_XDeref:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression XDeref operation encountered, implying multiple address spaces; this is not supported for location info.\n", start_off);
                    }break;
                    case DW_ExprOp_XDerefSize:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression XDerefSize operation encountered, implying multiple address spaces; this is not supported for location info.\n", start_off);
                    }break;
                    case DW_ExprOp_Call2:
                    case DW_ExprOp_Call4:
                    case DW_ExprOp_CallRef:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression call operation encountered. This is not supported for location info.\n", start_off);
                    }break;
                    case DW_ExprOp_ImplicitPointer:
                    case DW_ExprOp_GNU_ImplicitPointer:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression implicit pointer operation encountered. This is not supported for location info.\n", start_off);
                    }break;
                    case DW_ExprOp_GNU_ParameterRef:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression GNU_ParameterRef operation encountered. This is not supported for location info.\n", start_off);
                    }break;
                    case DW_ExprOp_DerefType:
                    case DW_ExprOp_GNU_DerefType:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression DerefType operation encountered. This is not supported for location info.\n", start_off);
                    }break;
                    case DW_ExprOp_ConstType:
                    case DW_ExprOp_GNU_ConstType:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression ConstType operation encountered. This is not supported for location info.\n", start_off);
                    }break;
                    case DW_ExprOp_RegvalType:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression RegvalType operation encountered. This is not supported for location info.\n", start_off);
                    }break;
                    case DW_ExprOp_PushObjectAddress:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression PushObjectAddress operation encountered. This is not supported for location info.\n", start_off);
                    }break;
                    case DW_ExprOp_Rot:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression Rot (rotate) operation encountered. This is not supported for location info.\n", start_off);
                    }break;
                    case DW_ExprOp_GNU_UnInit:
                    {
                      dst_bytecode_is_good = 0;
                      log_infof("[.debug_info@0x%I64x] DWARF expression GNU_UnInit operation encountered. This is not supported for location info.\n", start_off);
                      // TODO: flag value as unitialized; this must be last opcode; possible to use with DW_ExprOp_Piece;
                    } break;
                  }
                  
                  //- rjf: push values to stack
                  {
                    U64 push_count = dw_push_count_from_expr_op(opcode);
                    push_count = Min(push_count, ArrayCount(push_vals));
                    for EachIndex(push_idx, push_count)
                    {
                      D2R2_ExprValNode *n = free_val;
                      if(n != 0)
                      {
                        SLLStackPop(free_val);
                      }
                      else
                      {
                        n = push_array(scratch2.arena, D2R2_ExprValNode, 1);
                      }
                      n->v = push_vals[push_idx];
                      SLLStackPush(val_stack_top, n);
                    }
                  }
                  
                  if(expr_off == start_expr_off)
                  {
                    break;
                  }
                }
                
                //- rjf: apply RDI byte offsets to jump ops
                {
                  // TODO(rjf): apply RDI byte offsets to jump ops
                }
                
                //- rjf: map bytecode -> RDIM_Location - we may want to simplify
                RDIM_Location loc = {0};
                {
                  loc.kind = val_stack_top && val_stack_top->v.is_addr ? RDI_LocationKind_AddrBytecodeStream : RDI_LocationKind_ValBytecodeStream;
                  loc.bytecode = dst_bytecode;
                  
                  // rjf: gather first few bytecodes for pattern matching
                  RDI_EvalOp beginning_ops[5] = {0};
                  RDIM_EvalBytecodeOp *beginning_nodes[5] = {0};
                  {
                    U64 idx = 0;
                    for EachNode(n, RDIM_EvalBytecodeOp, dst_bytecode.first_op)
                    {
                      if(idx >= ArrayCount(beginning_ops))
                      {
                        break;
                      }
                      beginning_ops[idx] = n->op;
                      beginning_nodes[idx] = n;
                      idx += 1;
                    }
                  }
                  
                  // rjf: match val registers
                  if(loc.kind == RDI_LocationKind_ValBytecodeStream &&
                     beginning_ops[0] == RDI_EvalOp_RegRead &&
                     beginning_ops[1] == RDI_EvalOp_Stop)
                  {
                    U8 rdi_reg_code = (beginning_nodes[0]->p&0x0000FF)>>0;
                    U8 byte_size    = (beginning_nodes[0]->p&0x00FF00)>>8;
                    U8 byte_off     = (beginning_nodes[0]->p&0xFF0000)>>16;
                    if(byte_size == unit_parse_ctx->addr_size &&
                       byte_off == 0)
                    {
                      loc.kind = RDI_LocationKind_ValReg;
                      loc.reg_code = rdi_reg_code;
                    }
                  }
                  
                  // rjf: match simple register offsets
                  if(loc.kind == RDI_LocationKind_AddrBytecodeStream &&
                     beginning_ops[0] == RDI_EvalOp_RegRead &&
                     (beginning_ops[1] == RDI_EvalOp_ConstU8 ||
                      beginning_ops[1] == RDI_EvalOp_ConstU16) &&
                     beginning_ops[2] == RDI_EvalOp_TruncSigned &&
                     beginning_ops[3] == RDI_EvalOp_Add &&
                     beginning_ops[4] == RDI_EvalOp_Stop)
                  {
                    U8 rdi_reg_code = (beginning_nodes[0]->p&0x0000FF)>>0;
                    U8 byte_size    = (beginning_nodes[0]->p&0x00FF00)>>8;
                    U8 byte_off     = (beginning_nodes[0]->p&0xFF0000)>>16;
                    if(byte_off == 0 && byte_size == unit_parse_ctx->addr_size)
                    {
                      loc.kind = RDI_LocationKind_AddrRegPlusOff;
                      loc.reg_code = rdi_reg_code;
                      loc.offset   = beginning_nodes[1]->p;
                    }
                  }
                  
                  // rjf: match simple module offsets
                  if(beginning_ops[0] == RDI_EvalOp_ModuleOff && beginning_ops[1] == RDI_EvalOp_Stop)
                  {
                    U64 voff = beginning_nodes[0]->p;
                    loc.kind = RDI_LocationKind_ModuleOff;
                    loc.offset = voff;
                  }
                  else if(((beginning_ops[0] == RDI_EvalOp_ModuleOff &&
                            (beginning_ops[1] == RDI_EvalOp_ConstU8 ||
                             beginning_ops[1] == RDI_EvalOp_ConstU16 ||
                             beginning_ops[1] == RDI_EvalOp_ConstU32 ||
                             beginning_ops[1] == RDI_EvalOp_ConstU64)) ||
                           (beginning_ops[1] == RDI_EvalOp_ModuleOff &&
                            (beginning_ops[0] == RDI_EvalOp_ConstU8 ||
                             beginning_ops[0] == RDI_EvalOp_ConstU16 ||
                             beginning_ops[0] == RDI_EvalOp_ConstU32 ||
                             beginning_ops[0] == RDI_EvalOp_ConstU64))) &&
                          beginning_ops[2] == RDI_EvalOp_Add &&
                          beginning_ops[3] == RDI_EvalOp_Stop)
                  {
                    U64 voff = beginning_nodes[0]->p + beginning_nodes[1]->p;
                    loc.kind = RDI_LocationKind_ModuleOff;
                    loc.offset = voff;
                  }
                  
                  // rjf: match simple TLS offsets
                  if(beginning_ops[0] == RDI_EvalOp_TLSOff && beginning_ops[1] == RDI_EvalOp_Stop)
                  {
                    U64 toff = beginning_nodes[0]->p;
                    loc.kind = RDI_LocationKind_TLSOff;
                    loc.offset = toff;
                  }
                  else if(((beginning_ops[0] == RDI_EvalOp_TLSOff &&
                            (beginning_ops[1] == RDI_EvalOp_ConstU8 ||
                             beginning_ops[1] == RDI_EvalOp_ConstU16 ||
                             beginning_ops[1] == RDI_EvalOp_ConstU32 ||
                             beginning_ops[1] == RDI_EvalOp_ConstU64)) ||
                           (beginning_ops[1] == RDI_EvalOp_TLSOff &&
                            (beginning_ops[0] == RDI_EvalOp_ConstU8 ||
                             beginning_ops[0] == RDI_EvalOp_ConstU16 ||
                             beginning_ops[0] == RDI_EvalOp_ConstU32 ||
                             beginning_ops[0] == RDI_EvalOp_ConstU64))) &&
                          beginning_ops[2] == RDI_EvalOp_Add &&
                          beginning_ops[3] == RDI_EvalOp_Stop)
                  {
                    U64 toff = beginning_nodes[0]->p + beginning_nodes[1]->p;
                    loc.kind = RDI_LocationKind_TLSOff;
                    loc.offset = toff;
                  }
                }
                
                //- rjf: collect
                {
                  RDIM_Rng1U64 voff_range = {range.min - base_vaddr, range.max - base_vaddr};
                  if(bytecode_is_framebase_dependent)
                  {
                    voff_range.min = Max(voff_range.min, framebase_voff_range.min);
                    voff_range.max = Min(voff_range.max, framebase_voff_range.max);
                  }
                  rdim_location_case_list_push(arena, dst_locations, loc, voff_range);
                  dst_is_tls_dependent[0] = is_tls_dependent;
                }
                
                //- rjf: not frame-base dependent? -> break - don't do this per-framebase case
                if(!bytecode_is_framebase_dependent)
                {
                  break;
                }
              }
            }
          }
        }
        
        ////////////////////////
        //- rjf: produce symbols from tag
        //
        RDIM_Scope *new_scope_open = 0;
        switch(tag.kind)
        {
          default:{}break;
          
          //- rjf: subprograms (procedures)
          case DW_TagKind_SubProgram:
          {
            RDIM_Scope *root_scope = rdim_scope_chunk_list_push(arena, &dst_unit->scopes, chunk_count);
            RDIM_Symbol *procedure = rdim_symbol_chunk_list_push(arena, &dst_unit->procedures, chunk_count);
            procedure->name           = name;
            procedure->link_name      = link_name;
            procedure->type           = type;
            procedure->root_scope     = root_scope;
            procedure->location_cases = framebase_location_cases;
            root_scope->symbol = procedure;
            root_scope->voff_ranges = ranges;
            dst_unit->scopes.scope_voff_count += 2*ranges.count;
            new_scope_open = root_scope;
          }break;
          
          //- rjf: inline site
          case DW_TagKind_InlinedSubroutine:
          {
            
          }break;
          
          //- rjf: variables
          case DW_TagKind_Variable:
          case DW_TagKind_FormalParameter:
          if(name.size != 0)
          {
            U64 var_chunk_count = chunk_count;
            RDIM_SymbolChunkList *dst_symbols = &dst_unit->global_variables;
            if(location_is_tls_dependent)
            {
              dst_symbols = &dst_unit->thread_variables;
            }
            else if(top_scope != 0)
            {
              dst_symbols = &top_scope->scope->locals;
              var_chunk_count = 8;
            }
            RDIM_Symbol *var = rdim_symbol_chunk_list_push(arena, dst_symbols, var_chunk_count);
            var->is_extern = is_external;
            var->is_param  = (tag.kind == DW_TagKind_FormalParameter);
            var->name      = name;
            var->link_name = link_name;
            var->type      = type;
            if(top_scope != 0)
            {
              var->container_scope = top_scope->scope;
            }
            var->location_cases = location_cases;
          }break;
          
          //- rjf: lexical blocks (scopes)
          case DW_TagKind_LexicalBlock:
          {
            RDIM_Scope *scope = rdim_scope_chunk_list_push(arena, &dst_unit->scopes, chunk_count);
            scope->voff_ranges = ranges;
            dst_unit->scopes.scope_voff_count += 2*ranges.count;
            new_scope_open = scope;
          }break;
        }
        
        ////////////////////////
        //- rjf: push scopes
        //
        if(new_scope_open)
        {
          // rjf: insert scope to parent
          if(top_scope != 0)
          {
            RDIM_Scope *parent = top_scope->scope;
            SLLQueuePush_N(parent->first_child, parent->last_child, new_scope_open, next_sibling);
            new_scope_open->parent_scope = parent;
            if(new_scope_open->symbol == 0)
            {
              new_scope_open->symbol = parent->symbol;
            }
          }
          
          // rjf: push new scope to stack
          D2R2_ScopeNode *n = free_scope;
          if(n != 0)
          {
            SLLStackPop(free_scope);
          }
          else
          {
            n = push_array(scratch.arena, D2R2_ScopeNode, 1);
          }
          n->scope = new_scope_open;
          if(framebase_attrib != &dw2_attrib_nil)
          {
            n->framebase_location_cases = framebase_location_cases;
          }
          else if(top_scope != 0)
          {
            n->framebase_location_cases = top_scope->framebase_location_cases;
          }
          SLLStackPush(top_scope, n);
        }
        
        ////////////////////////
        //- rjf: pop scopes
        //
        if(tag.kind == DW_TagKind_Null && top_scope != 0)
        {
          D2R2_ScopeNode *n = top_scope;
          SLLStackPop(top_scope);
          SLLStackPush(free_scope, n);
        }
        
        scratch_end(scratch2);
        if(off == start_off)
        {
          break;
        }
      }
    }
  }
  
  ////////////////////////////
  //- rjf: fill result
  //
  RDIM_BakeParams result = {0};
  {
    result.subset_flags    = params->subset_flags;
    result.top_level_info  = top_level_info;
    result.binary_sections = binary_sections;
    result.units           = *all_units;
    result.types           = *all_types;
    result.udts            = *all_udts;
    result.src_files       = *all_src_files;
    result.line_tables     = *all_line_tables;
  }
  
#undef d2r2_type_from_builtin_kind
  scratch_end(scratch);
  return result;
}
