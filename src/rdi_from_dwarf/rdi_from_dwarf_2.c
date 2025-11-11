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
  //- rjf: build per-unit tag parsing contexts
  //
  DW2_TagParseCtx *unit_tag_parse_ctxs = 0;
  {
    if(lane_idx() == 0)
    {
      unit_tag_parse_ctxs = push_array(scratch.arena, DW2_TagParseCtx, unit_count);
    }
    lane_sync_u64(&unit_tag_parse_ctxs, 0);
    Rng1U64 range = lane_range(unit_count);
    for EachInRange(unit_idx, range)
    {
      DW2_UnitHeader *hdr = &unit_headers[unit_idx];
      DW2_TagParseCtx *ctx = &unit_tag_parse_ctxs[unit_idx];
      ctx->version     = hdr->version;
      ctx->format      = hdr->format;
      ctx->addr_size   = hdr->addr_size;
      ctx->abbrev_data = raw->sec[DW_Section_Abbrev].data;
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
      dw2_read_tag(scratch.arena, &unit_tag_parse_ctxs[unit_idx], raw->sec[DW_Section_Info].data, unit_info_tag_ranges[unit_idx].min, &unit_root_tags[unit_idx]);
    }
  }
  lane_sync();
  
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
