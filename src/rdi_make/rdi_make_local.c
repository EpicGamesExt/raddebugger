// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "lib_rdi_make/rdi_make.c"

internal RDIM_DataModel
rdim_data_model_from_os_arch(OperatingSystem os, RDI_Arch arch)
{
  RDIM_DataModel data_model = RDIM_DataModel_Null;
#define Case(os_name, arch_name, model_name) if(os == OperatingSystem_##os_name && arch == Arch_##arch_name) { data_model = RDIM_DataModel_##model_name; }
  Case(Windows, x86, LLP64);
  Case(Windows, x64, LLP64);
  Case(Linux,   x86, ILP32);
  Case(Linux,   x64, LLP64);
  Case(Mac,     x64, LP64);
#undef Case
  return data_model;
}

internal RDIM_TopLevelInfo
rdim_make_top_level_info(String8 image_name, Arch arch, U64 exe_hash, RDIM_BinarySectionList sections)
{
  // convert arch
  RDI_Arch arch_rdi = RDI_Arch_NULL;
  switch(arch)
  {
    default:{}break;
    case Arch_x64:{arch_rdi = RDI_Arch_X64;}break;
    case Arch_x86:{NotImplemented;}break;
  }
  
  // find max VOFF
  U64 exe_voff_max = 0;
  for EachNode(sect_n, RDIM_BinarySectionNode, sections.first)
  {
    exe_voff_max = Max(exe_voff_max, sect_n->v.voff_opl);
  }
  
  // fill out top level info
  RDIM_TopLevelInfo top_level_info = {0};
  top_level_info.arch              = arch_rdi;
  top_level_info.exe_hash          = exe_hash;
  top_level_info.voff_max          = exe_voff_max;
  top_level_info.producer_name     = str8_lit(BUILD_TITLE_STRING_LITERAL);
  
  return top_level_info;
}

internal RDIM_BakeParams
rdim_loose_from_rdi(Arena *arena, RDIM_SubsetFlags subset_flags, RDI_Parsed *rdi)
{
  //- rjf: setup bake params
  RDIM_BakeParams *bp = 0;
  if(lane_idx() == 0)
  {
    bp = push_array(arena, RDIM_BakeParams, 1);
    bp->subset_flags = subset_flags;
  }
  lane_sync_u64(&bp, 0);
  
  //- rjf: convert top level info
  if(lane_idx() == 0)
  {
    RDI_TopLevelInfo *tli = rdi_element_from_name_idx(rdi, TopLevelInfo, 0);
    bp->top_level_info.arch              = tli->arch;
    bp->top_level_info.exe_name.str      = rdi_string_from_idx(rdi, tli->exe_name_string_idx, &bp->top_level_info.exe_name.size);
    bp->top_level_info.exe_hash          = tli->exe_hash;
    bp->top_level_info.voff_max          = tli->voff_max;
    bp->top_level_info.guid              = tli->guid;
    bp->top_level_info.producer_name.str = rdi_string_from_idx(rdi, tli->producer_name_string_idx, &bp->top_level_info.producer_name.size);
  }
  lane_sync();
  
  //- rjf: convert binary sections
  if(lane_idx() == 0)
  {
    U64 count = 0;
    RDI_BinarySection *v = rdi_table_from_name(rdi, BinarySections, &count);
    for EachIndex(idx, count)
    {
      RDIM_BinarySection *bsec = rdim_binary_section_list_push(arena, &bp->binary_sections);
      bsec->name.str   = rdi_string_from_idx(rdi, v[idx].name_string_idx, &bsec->name.size);
      bsec->flags      = v[idx].flags;
      bsec->voff_first = v[idx].voff_first;
      bsec->voff_opl   = v[idx].voff_opl;
      bsec->foff_first = v[idx].foff_first;
      bsec->foff_opl   = v[idx].foff_opl;
    }
  }
  lane_sync();
  
  //- rjf: bucket voff ranges by unit idx
  RDIM_Rng1U64ChunkList *unit_ranges = 0;
  if(lane_idx() == 0)
  {
    U64 units_count = 0;
    rdi_table_from_name(rdi, Units, &units_count);
    U64 unit_vmap_count = 0;
    RDI_VMapEntry *unit_vmap = rdi_table_from_name(rdi, UnitVMap, &unit_vmap_count);
    unit_ranges = push_array(arena, RDIM_Rng1U64ChunkList, units_count);
    if(unit_vmap_count > 0)
    {
      for EachIndex(idx, unit_vmap_count-1)
      {
        RDIM_Rng1U64 rng = {unit_vmap[idx].voff, unit_vmap[idx+1].voff};
        rdim_rng1u64_chunk_list_push(arena, &unit_ranges[unit_vmap[idx].idx], 256, rng);
      }
    }
  }
  lane_sync_u64(&unit_ranges, 0);
  
  //- rjf: convert src files
  RDIM_SrcFileChunkList *src_files = 0;
  RDIM_SrcFile **src_file_from_idx_table = 0;
  {
    U64 src_file_count = 0;
    RDI_SourceFile *src_file_v = rdi_table_from_name(rdi, SourceFiles, &src_file_count);
    RDIM_SrcFileChunkList *lane_srcfiles = 0;
    if(lane_idx() == 0)
    {
      src_files = push_array(arena, RDIM_SrcFileChunkList, 1);
      src_file_from_idx_table = push_array(arena, RDIM_SrcFile *, src_file_count);
      lane_srcfiles = push_array(arena, RDIM_SrcFileChunkList, lane_count());
    }
    lane_sync_u64(&lane_srcfiles, 0);
    {
      Rng1U64 range = lane_range(src_file_count);
      for EachInRange(idx, range)
      {
        RDI_SourceFile *src = &src_file_v[idx];
        RDIM_SrcFile *dst = rdim_src_file_chunk_list_push(arena, &lane_srcfiles[lane_idx()], dim_1u64(range));
        
        // rjf: get checksum
        String8 checksum = {0};
        switch(src->checksum_kind)
        {
          default:{}break;
          case RDI_ChecksumKind_MD5:      {checksum = str8(rdi_element_from_name_idx(rdi, MD5Checksums, src->checksum_idx)->u8, sizeof(RDI_MD5));}break;
          case RDI_ChecksumKind_SHA1:     {checksum = str8(rdi_element_from_name_idx(rdi, SHA1Checksums, src->checksum_idx)->u8, sizeof(RDI_SHA1));}break;
          case RDI_ChecksumKind_SHA256:   {checksum = str8(rdi_element_from_name_idx(rdi, SHA256Checksums, src->checksum_idx)->u8, sizeof(RDI_SHA256));}break;
          case RDI_ChecksumKind_Timestamp:{checksum = str8((U8 *)rdi_element_from_name_idx(rdi, Timestamps, src->checksum_idx), sizeof(RDI_U64));}break;
        }
        
        // rjf: fill basics
        dst->path          = str8_from_rdi_path_node_idx(arena, rdi, PathStyle_Relative, src->file_path_node_idx);
        dst->checksum_kind = src->checksum_kind;
        dst->checksum      = checksum;
        src_file_from_idx_table[idx] = dst;
      }
    }
    lane_sync();
    if(lane_idx() == 0)
    {
      for EachIndex(lidx, lane_count())
      {
        rdim_src_file_chunk_list_concat_in_place(src_files, &lane_srcfiles[lidx]);
      }
    }
  }
  lane_sync();
  
  //- rjf: convert units
  RDIM_UnitChunkList *units = 0;
  RDIM_LineTableChunkList *line_tables = 0;
  {
    RDIM_UnitChunkList *lane_units = 0;
    RDIM_LineTableChunkList *lane_linetables = 0;
    if(lane_idx() == 0)
    {
      lane_units = push_array(arena, RDIM_UnitChunkList, lane_count());
      lane_linetables = push_array(arena, RDIM_LineTableChunkList, lane_count());
      units = push_array(arena, RDIM_UnitChunkList, 1);
      line_tables = push_array(arena, RDIM_LineTableChunkList, 1);
    }
    lane_sync_u64(&lane_units, 0);
    lane_sync_u64(&lane_linetables, 0);
    lane_sync_u64(&units, 0);
    lane_sync_u64(&line_tables, 0);
    U64 count = 0;
    RDI_Unit *v = rdi_table_from_name(rdi, Units, &count);
    U64 unit_take_idx_ = 0;
    U64 *unit_take_idx_ptr = &unit_take_idx_;
    lane_sync_u64(&unit_take_idx_ptr, 0);
    for(;;)
    {
      U64 unit_idx = ins_atomic_u64_inc_eval(unit_take_idx_ptr)-1;
      if(unit_idx >= count)
      {
        break;
      }
      RDI_Unit *src = &v[unit_idx];
      
      // rjf: convert flat top parts
      RDIM_Unit *dst = rdim_unit_chunk_list_push(arena, &lane_units[lane_idx()], 64);
      dst->unit_name     = str8_from_rdi_string_idx(rdi, src->unit_name_string_idx);
      dst->compiler_name = str8_from_rdi_string_idx(rdi, src->compiler_name_string_idx);
      dst->source_file   = str8_from_rdi_path_node_idx(arena, rdi, PathStyle_Relative, src->source_file_path_node);
      dst->object_file   = str8_from_rdi_path_node_idx(arena, rdi, PathStyle_Relative, src->object_file_path_node);
      dst->archive_file  = str8_from_rdi_path_node_idx(arena, rdi, PathStyle_Relative, src->archive_file_path_node);
      dst->build_path    = str8_from_rdi_path_node_idx(arena, rdi, PathStyle_Relative, src->build_path_node);
      dst->language      = src->language;
      dst->voff_ranges   = unit_ranges[unit_idx];
      
      // rjf: convert line table
      dst->line_table = rdim_line_table_chunk_list_push(arena, &lane_linetables[lane_idx()], 64);
      {
        RDI_LineTable *src_lt_unparsed = rdi_element_from_name_idx(rdi, LineTables, src->line_table_idx);
        RDI_ParsedLineTable src_lt = {0};
        rdi_parsed_from_line_table(rdi, src_lt_unparsed, &src_lt);
        RDIM_LineTable *dst_lt = dst->line_table;
        {
          RDIM_SrcFile *seq_src_file = 0;
          U64 seq_start_idx = 0;
          for(U64 line_idx = 0; line_idx <= src_lt.count; line_idx += 1)
          {
            // rjf: get next src file
            RDIM_SrcFile *next_src_file = 0;
            if(line_idx < src_lt.count)
            {
              next_src_file = src_file_from_idx_table[src_lt.lines[line_idx].file_idx];
            }
            
            // rjf: next file doesn't match current sequence? -> complete sequence
            if(next_src_file != seq_src_file && seq_src_file != 0)
            {
              U64 seq_line_count = (line_idx - seq_start_idx);
              U32 *seq_line_nums = push_array(arena, U32, seq_line_count);
              for(U64 line_idx_2 = seq_start_idx; line_idx_2 < line_idx; line_idx_2 += 1)
              {
                seq_line_nums[line_idx_2] = src_lt.lines[line_idx_2].line_num;
              }
              rdim_line_table_push_sequence(arena, &lane_linetables[lane_idx()], dst_lt, seq_src_file,
                                            src_lt.voffs + seq_start_idx,
                                            seq_line_nums,
                                            0, // TODO(rjf): column support
                                            seq_line_count);
            }
            
            // rjf: start next sequence
            if(next_src_file != seq_src_file)
            {
              seq_src_file = next_src_file;
              seq_start_idx = line_idx;
            }
          }
        }
      }
    }
    lane_sync();
    if(lane_idx() == 0)
    {
      for EachIndex(l_idx, lane_count())
      {
        rdim_unit_chunk_list_concat_in_place(units, &lane_units[l_idx]);
      }
      for EachIndex(l_idx, lane_count())
      {
        rdim_line_table_chunk_list_concat_in_place(line_tables, &lane_linetables[l_idx]);
      }
    }
  }
  lane_sync();
  
  // TODO(rjf): convert types
  // TODO(rjf): convert udts
  // TODO(rjf): convert locations
  // TODO(rjf): convert global variables
  // TODO(rjf): convert thread variables
  // TODO(rjf): convert constants
  // TODO(rjf): convert procedures
  // TODO(rjf): convert scopes
  // TODO(rjf): convert inline sites
  // TODO(rjf): package & return
  
  RDIM_BakeParams result = bp[0];
  return result;
}

internal RDIM_BakeResults
rdim_bake(Arena *arena, RDIM_BakeParams *params)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake vmaps
  //
  RDIM_ScopeVMapBakeResult *baked_scope_vmap = 0;
  RDIM_UnitVMapBakeResult *baked_unit_vmap = 0;
  RDIM_GlobalVMapBakeResult *baked_global_vmap = 0;
  if(lane_idx() == 0)
  {
    baked_scope_vmap = push_array(scratch.arena, RDIM_ScopeVMapBakeResult, 1);
    baked_unit_vmap = push_array(scratch.arena, RDIM_UnitVMapBakeResult, 1);
    baked_global_vmap = push_array(scratch.arena, RDIM_GlobalVMapBakeResult, 1);
  }
  lane_sync_u64(&baked_scope_vmap, 0);
  lane_sync_u64(&baked_unit_vmap, 0);
  lane_sync_u64(&baked_global_vmap, 0);
  ProfScope("bake vmaps")
  {
    Temp scratch = scratch_begin(&arena, 1);
#pragma pack(push, 1)
    typedef struct VMapRecord VMapRecord;
    struct VMapRecord
    {
      union
      {
        struct
        {
          U32 negative_size;
          U64 voff;
        };
        U8 digits[12];
      }
      key;
      U32 idx;
    };
#pragma pack(pop)
    
    ////////////////////////////
    //- rjf: gather unsorted scope vmap records
    //
    VMapRecord *scope_vmap_records = 0;
    U64 scope_vmap_records_count = 0;
    ProfScope("gather unsorted scope vmap records")
    {
      //- rjf: calculate per-lane-chunk counts
      U64 *lane_chunk_range_counts = 0;
      if(lane_idx() == 0)
      {
        lane_chunk_range_counts = push_array(scratch.arena, U64, params->scopes.chunk_count * lane_count());
      }
      lane_sync_u64(&lane_chunk_range_counts, 0);
      {
        U64 chunk_idx = 0;
        for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
        {
          U64 slot_idx = lane_idx()*params->scopes.chunk_count + chunk_idx;
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            lane_chunk_range_counts[slot_idx] += n->v[n_idx].voff_ranges.count;
          }
          chunk_idx += 1;
        }
      }
      lane_sync();
      
      //- rjf: calculate per-lane-chunk offsets
      U64 *lane_chunk_range_offs = 0;
      U64 total_range_count = 0;
      if(lane_idx() == 0)
      {
        lane_chunk_range_offs = push_array(scratch.arena, U64, params->scopes.chunk_count * lane_count());
        U64 off = 0;
        U64 chunk_idx = 0;
        for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
        {
          for EachIndex(l_idx, lane_count())
          {
            U64 slot_idx = l_idx*params->scopes.chunk_count + chunk_idx;
            lane_chunk_range_offs[slot_idx] = off;
            off += lane_chunk_range_counts[slot_idx];
          }
          chunk_idx += 1;
        }
        total_range_count = off;
      }
      lane_sync_u64(&lane_chunk_range_offs, 0);
      lane_sync_u64(&total_range_count, 0);
      
      //- rjf: allocate records
      if(lane_idx() == 0)
      {
        scope_vmap_records_count = total_range_count;
        scope_vmap_records = push_array_no_zero(scratch.arena, VMapRecord, scope_vmap_records_count);
      }
      lane_sync_u64(&scope_vmap_records, 0);
      lane_sync_u64(&scope_vmap_records_count, 0);
      
      //- rjf: fill records
      {
        U64 chunk_idx = 0;
        for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
        {
          U64 slot_idx = lane_idx()*params->scopes.chunk_count + chunk_idx;
          U64 off = lane_chunk_range_offs[slot_idx];
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            RDI_U32 scope_idx = (RDI_U32)rdim_idx_from_scope(&n->v[n_idx]); // TODO(rjf): @u64_to_u32
            for EachNode(rng_n, RDIM_Rng1U64Node, n->v[n_idx].voff_ranges.first)
            {
              scope_vmap_records[off].key.voff = rng_n->v.min;
              scope_vmap_records[off].key.negative_size = -(RDI_U32)(rng_n->v.max - rng_n->v.min);
              scope_vmap_records[off].idx = scope_idx;
              off += 1;
            }
          }
          chunk_idx += 1;
        }
      }
    }
    lane_sync();
    
    ////////////////////////////
    //- rjf: gather unsorted global vmap records
    //
    VMapRecord *global_vmap_records = 0;
    U64 global_vmap_records_count = 0;
    ProfScope("gather unsorted global vmap records")
    {
      //- rjf: calculate per-lane-chunk counts
      U64 *lane_chunk_range_counts = 0;
      if(lane_idx() == 0)
      {
        lane_chunk_range_counts = push_array(scratch.arena, U64, params->global_variables.chunk_count * lane_count());
      }
      lane_sync_u64(&lane_chunk_range_counts, 0);
      {
        U64 chunk_idx = 0;
        for EachNode(n, RDIM_SymbolChunkNode, params->global_variables.first)
        {
          U64 slot_idx = lane_idx()*params->global_variables.chunk_count + chunk_idx;
          Rng1U64 range = lane_range(n->count);
          lane_chunk_range_counts[slot_idx] += dim_1u64(range);
          chunk_idx += 1;
        }
      }
      lane_sync();
      
      //- rjf: calculate per-lane-chunk offsets
      U64 *lane_chunk_range_offs = 0;
      U64 total_range_count = 0;
      if(lane_idx() == 0)
      {
        lane_chunk_range_offs = push_array(scratch.arena, U64, params->global_variables.chunk_count * lane_count());
        U64 off = 0;
        U64 chunk_idx = 0;
        for EachNode(n, RDIM_SymbolChunkNode, params->global_variables.first)
        {
          for EachIndex(l_idx, lane_count())
          {
            U64 slot_idx = l_idx*params->global_variables.chunk_count + chunk_idx;
            lane_chunk_range_offs[slot_idx] = off;
            off += lane_chunk_range_counts[slot_idx];
          }
          chunk_idx += 1;
        }
        total_range_count = off;
      }
      lane_sync_u64(&lane_chunk_range_offs, 0);
      lane_sync_u64(&total_range_count, 0);
      
      //- rjf: allocate records
      if(lane_idx() == 0)
      {
        global_vmap_records_count = total_range_count;
        global_vmap_records = push_array_no_zero(scratch.arena, VMapRecord, global_vmap_records_count);
      }
      lane_sync_u64(&global_vmap_records, 0);
      lane_sync_u64(&global_vmap_records_count, 0);
      
      //- rjf: fill records
      {
        U64 chunk_idx = 0;
        for EachNode(n, RDIM_SymbolChunkNode, params->global_variables.first)
        {
          U64 slot_idx = lane_idx()*params->global_variables.chunk_count + chunk_idx;
          U64 off = lane_chunk_range_offs[slot_idx];
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            RDI_U32 global_idx  = (RDI_U32)rdim_idx_from_symbol(&n->v[n_idx]); // TODO(rjf): @u64_to_u32
            RDI_U32 global_size = (RDI_U32)(n->v[n_idx].type ? n->v[n_idx].type->byte_size : 1);
            RDI_U64 global_voff = n->v[n_idx].offset;
            global_vmap_records[off].key.voff = global_voff;
            global_vmap_records[off].key.negative_size = -global_size;
            global_vmap_records[off].idx = global_idx;
            off += 1;
          }
          chunk_idx += 1;
        }
      }
    }
    lane_sync();
    
    ////////////////////////////
    //- rjf: gather unsorted unit vmap records
    //
    VMapRecord *unit_vmap_records = 0;
    U64 unit_vmap_records_count = 0;
    ProfScope("gather unsorted unit vmap records")
    {
      //- rjf: calculate per-lane-chunk counts
      U64 *lane_chunk_range_counts = 0;
      if(lane_idx() == 0)
      {
        lane_chunk_range_counts = push_array(scratch.arena, U64, params->units.chunk_count * lane_count());
      }
      lane_sync_u64(&lane_chunk_range_counts, 0);
      {
        U64 chunk_idx = 0;
        for EachNode(n, RDIM_UnitChunkNode, params->units.first)
        {
          U64 slot_idx = lane_idx()*params->units.chunk_count + chunk_idx;
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            lane_chunk_range_counts[slot_idx] += n->v[n_idx].voff_ranges.total_count;
          }
          chunk_idx += 1;
        }
      }
      lane_sync();
      
      //- rjf: calculate per-lane-chunk offsets
      U64 *lane_chunk_range_offs = 0;
      U64 total_range_count = 0;
      if(lane_idx() == 0)
      {
        lane_chunk_range_offs = push_array(scratch.arena, U64, params->units.chunk_count * lane_count());
        U64 off = 0;
        U64 chunk_idx = 0;
        for EachNode(n, RDIM_UnitChunkNode, params->units.first)
        {
          for EachIndex(l_idx, lane_count())
          {
            U64 slot_idx = l_idx*params->units.chunk_count + chunk_idx;
            lane_chunk_range_offs[slot_idx] = off;
            off += lane_chunk_range_counts[slot_idx];
          }
          chunk_idx += 1;
        }
        total_range_count = off;
      }
      lane_sync_u64(&lane_chunk_range_offs, 0);
      lane_sync_u64(&total_range_count, 0);
      
      //- rjf: allocate records
      if(lane_idx() == 0)
      {
        unit_vmap_records_count = total_range_count;
        unit_vmap_records = push_array_no_zero(scratch.arena, VMapRecord, unit_vmap_records_count);
      }
      lane_sync_u64(&unit_vmap_records, 0);
      lane_sync_u64(&unit_vmap_records_count, 0);
      
      //- rjf: fill records
      {
        U64 chunk_idx = 0;
        for EachNode(n, RDIM_UnitChunkNode, params->units.first)
        {
          U64 slot_idx = lane_idx()*params->units.chunk_count + chunk_idx;
          U64 off = lane_chunk_range_offs[slot_idx];
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            RDI_U32 unit_idx = (RDI_U32)rdim_idx_from_unit(&n->v[n_idx]); // TODO(rjf): @u64_to_u32
            for EachNode(rng_n, RDIM_Rng1U64ChunkNode, n->v[n_idx].voff_ranges.first)
            {
              for EachIndex(rng_n_idx, rng_n->count)
              {
                unit_vmap_records[off].key.voff = rng_n->v[rng_n_idx].min;
                unit_vmap_records[off].key.negative_size = -(RDI_U32)(rng_n->v[rng_n_idx].max - rng_n->v[rng_n_idx].min);
                unit_vmap_records[off].idx = unit_idx;
                off += 1;
              }
            }
          }
          chunk_idx += 1;
        }
      }
    }
    lane_sync();
    
    ////////////////////////////
    //- rjf: sort & bake all vmaps
    //
    struct
    {
      String8 name;
      VMapRecord *records;
      U64 records_count;
      RDI_VMapEntry **vmap_out;
      U32 *vmap_count_out;
    }
    vmap_tasks[] =
    {
      {str8_lit_comp("scopes"),  scope_vmap_records,   scope_vmap_records_count,  &baked_scope_vmap->vmap.vmap,  &baked_scope_vmap->vmap.count},
      {str8_lit_comp("globals"), global_vmap_records,  global_vmap_records_count, &baked_global_vmap->vmap.vmap, &baked_global_vmap->vmap.count},
      {str8_lit_comp("units"),   unit_vmap_records,    unit_vmap_records_count,   &baked_unit_vmap->vmap.vmap,   &baked_unit_vmap->vmap.count},
    };
    ProfScope("sort & bake all vmaps")
    {
      for EachElement(vmap_task_idx, vmap_tasks) ProfScope("sort & bake vmap for %.*s", str8_varg(vmap_tasks[vmap_task_idx].name))
      {
        VMapRecord *records = vmap_tasks[vmap_task_idx].records;
        U64 records_count = vmap_tasks[vmap_task_idx].records_count;
        
        ////////////////////////
        //- rjf: sort
        //
        ProfScope("sort")
        {
          //- rjf: set up constants
          U64 bytes_per_digit = 1;
          U64 num_possible_values_per_digit = 1<<(bytes_per_digit*8);
          U64 digits_count = sizeof(((VMapRecord *)0)->key)/bytes_per_digit;
          
          //- rjf: set up swap buffer / lane counters
          VMapRecord *records__swap = 0;
          U32 **lane_digit_counts = 0;
          U32 **lane_digit_offs = 0;
          if(lane_idx() == 0)
          {
            records__swap = push_array_no_zero(scratch.arena, VMapRecord, records_count);
            lane_digit_counts = push_array_no_zero(scratch.arena, U32 *, lane_count());
            lane_digit_offs = push_array_no_zero(scratch.arena, U32 *, lane_count());
          }
          lane_sync_u64(&records__swap, 0);
          lane_sync_u64(&lane_digit_counts, 0);
          lane_sync_u64(&lane_digit_offs, 0);
          lane_digit_counts[lane_idx()] = push_array_no_zero(scratch.arena, U32, num_possible_values_per_digit);
          lane_digit_offs[lane_idx()] = push_array_no_zero(scratch.arena, U32, num_possible_values_per_digit);
          
          //- rjf: do all sort passes
          {
            VMapRecord *src = records;
            VMapRecord *dst = records__swap;
            for EachIndex(digit_idx, digits_count)
            {
              // rjf: count digit value occurrences per-lane
              {
                U32 *digit_counts = lane_digit_counts[lane_idx()];
                MemoryZero(digit_counts, sizeof(digit_counts[0])*num_possible_values_per_digit);
                Rng1U64 range = lane_range(records_count);
                for EachInRange(idx, range)
                {
                  VMapRecord *rec = &src[idx];
                  U16 digit_value = (U16)rec->key.digits[digit_idx];
                  digit_counts[digit_value] += 1;
                }
              }
              lane_sync();
              
              // rjf: compute thread * digit value *relative* offset table
              {
                Rng1U64 range = lane_range(num_possible_values_per_digit);
                for EachInRange(value_idx, range)
                {
                  U64 layout_off = 0;
                  for EachIndex(lane_idx, lane_count())
                  {
                    lane_digit_offs[lane_idx][value_idx] = layout_off;
                    layout_off += lane_digit_counts[lane_idx][value_idx];
                  }
                }
              }
              lane_sync();
              
              // rjf: convert relative offsets -> absolute offsets
              if(lane_idx() == 0)
              {
                U64 last_off = 0;
                U64 num_of_nonzero_digit = 0;
                for EachIndex(value_idx, num_possible_values_per_digit)
                {
                  for EachIndex(lane_idx, lane_count())
                  {
                    lane_digit_offs[lane_idx][value_idx] += last_off;
                  }
                  last_off = lane_digit_offs[lane_count()-1][value_idx] + lane_digit_counts[lane_count()-1][value_idx];
                }
                // NOTE(rjf): required that: (last_off == element_count)
              }
              lane_sync();
              
              // rjf: move
              {
                U32 *lane_digit_offsets = lane_digit_offs[lane_idx()];
                Rng1U64 range = lane_range(records_count);
                for EachInRange(idx, range)
                {
                  VMapRecord *src_rec = &src[idx];
                  U16 digit_value = (U16)src_rec->key.digits[digit_idx];
                  U64 dst_off = lane_digit_offsets[digit_value];
                  lane_digit_offsets[digit_value] += 1;
                  MemoryCopyStruct(&dst[dst_off], src_rec);
                }
              }
              lane_sync();
              
              // rjf: swap source with destination for next pass
              Swap(VMapRecord *, src, dst);
            }
          }
        }
        lane_sync();
        
        ////////////////////////
        //- rjf: bake
        //
        RDI_VMapEntry *vmap = 0;
        RDI_U64 vmap_count = 0;
        {
          //- rjf: allocate vmap
          RDI_U64 vmap_count__cap = records_count*2 + 1;
          if(lane_idx() == 0)
          {
            vmap = push_array(arena, RDI_VMapEntry, vmap_count__cap);
          }
          lane_sync_u64(&vmap, 0);
          
          //- rjf: bake
          if(lane_idx() == 0)
          {
            typedef struct RangeNode RangeNode;
            struct RangeNode
            {
              RangeNode *next;
              Rng1U64 voff_range;
              U64 idx;
            };
            RDI_VMapEntry *vmap_ptr = vmap;
            RDI_VMapEntry *vmap_opl = vmap + vmap_count__cap;
            RangeNode *top_range = 0;
            RangeNode *free_range = 0;
            U64 last_recorded_voff = 0;
            for(U64 record_idx = 0; record_idx <= records_count; record_idx += 1)
            {
              // rjf: get next voff range and index
              Rng1U64 voff_range = r1u64(max_U64, max_U64);
              U64 idx = 0;
              if(record_idx < records_count)
              {
                VMapRecord *record = &records[record_idx];
                voff_range = r1u64(record->key.voff, record->key.voff + -record->key.negative_size);
                idx = (U64)record->idx;
              }
              
              // rjf: pop nodes we've advanced past
              {
                for(RangeNode *n = top_range, *next = 0; n != 0; n = next)
                {
                  next = n->next;
                  if(n->voff_range.max <= voff_range.min)
                  {
                    SLLStackPop(top_range);
                    SLLStackPush(free_range, n);
                    if(n->voff_range.max != last_recorded_voff)
                    {
                      vmap_ptr += 1;
                    }
                    vmap_ptr->voff = n->voff_range.max;
                    vmap_ptr->idx = next ? next->idx : 0;
                    last_recorded_voff = vmap_ptr->voff;
                  }
                  else
                  {
                    break;
                  }
                }
              }
              
              // rjf: push this node
              if(record_idx < records_count)
              {
                RangeNode *r = free_range;
                if(r)
                {
                  SLLStackPop(free_range);
                }
                else
                {
                  r = push_array(scratch.arena, RangeNode, 1);
                }
                SLLStackPush(top_range, r);
                r->voff_range = voff_range;
                r->idx = idx;
                if(voff_range.min != last_recorded_voff || (vmap_ptr->idx != idx && vmap_ptr->idx != 0))
                {
                  vmap_ptr += 1;
                }
                vmap_ptr->voff = voff_range.min;
                vmap_ptr->idx = idx;
                last_recorded_voff = voff_range.min;
              }
            }
            if(last_recorded_voff != 0)
            {
              vmap_ptr += 1;
            }
            vmap_count = (vmap_ptr - vmap);
          }
          lane_sync_u64(&vmap_count, 0);
        }
        lane_sync();
        
        ////////////////////////
        //- rjf: store
        //
        if(lane_idx() == 0)
        {
          vmap_tasks[vmap_task_idx].vmap_out[0] = vmap;
          vmap_tasks[vmap_task_idx].vmap_count_out[0] = vmap_count;
        }
      }
    }
    
    scratch_end(scratch);
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage build interned path tree
  //
  RDIM_BakePathTree *path_tree = 0;
  if(lane_idx() == 0) ProfScope("build interned path tree")
  {
    //- rjf: set up tree
    path_tree = rdim_push_array(arena, RDIM_BakePathTree, 1);
    rdim_bake_path_tree_insert(arena, path_tree, rdim_str8_lit("<nil>"));
    
    //- rjf: bake unit file paths
    RDIM_ProfScope("bake unit file paths")
    {
      for(RDIM_UnitChunkNode *n = params->units.first; n != 0; n = n->next)
      {
        for(RDI_U64 idx = 0; idx < n->count; idx += 1)
        {
          rdim_bake_path_tree_insert(arena, path_tree, n->v[idx].source_file);
          rdim_bake_path_tree_insert(arena, path_tree, n->v[idx].object_file);
          rdim_bake_path_tree_insert(arena, path_tree, n->v[idx].archive_file);
          rdim_bake_path_tree_insert(arena, path_tree, n->v[idx].build_path);
        }
      }
    }
    
    //- rjf: bake source file paths
    RDIM_ProfScope("bake source file paths")
    {
      for(RDIM_SrcFileChunkNode *n = params->src_files.first; n != 0; n = n->next)
      {
        for(RDI_U64 idx = 0; idx < n->count; idx += 1)
        {
          RDIM_BakePathNode *node = rdim_bake_path_tree_insert(arena, path_tree, n->v[idx].path);
          node->src_file = &n->v[idx];
        }
      }
    }
  }
  lane_sync_u64(&path_tree, 0);
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage gather all unsorted, joined, line table info; & sort
  //
  U64 line_tables_count = 0;
  RDIM_LineTable **src_line_tables = 0;
  RDIM_UnsortedJoinedLineTable *unsorted_joined_line_tables = 0;
  RDIM_SortKey **sorted_line_table_keys = 0;
  RDIM_LineTableBakeResult *baked_line_tables = 0;
  ProfScope("gather all unsorted, joined, line table info; & sort")
  {
    //- rjf: set up outputs
    ProfScope("set up outputs")
    {
      // rjf: calculate header info
      if(lane_idx() == 0)
      {
        line_tables_count = params->line_tables.total_count;
        src_line_tables = push_array(arena, RDIM_LineTable *, line_tables_count);
        ProfScope("flatten chunk list")
        {
          U64 joined_idx = 0;
          for(RDIM_LineTableChunkNode *n = params->line_tables.first; n != 0; n = n->next)
          {
            for EachIndex(idx, n->count)
            {
              src_line_tables[joined_idx] = &n->v[idx];
              joined_idx += 1;
            }
          }
        }
        baked_line_tables = push_array(scratch.arena, RDIM_LineTableBakeResult, 1);
        baked_line_tables->line_tables_count       = params->line_tables.total_count + 1;
        baked_line_tables->line_table_voffs_count  = params->line_tables.total_line_count + 2*params->line_tables.total_seq_count;
        baked_line_tables->line_table_lines_count  = params->line_tables.total_line_count + params->line_tables.total_seq_count;
        baked_line_tables->line_table_columns_count= 1;
      }
      lane_sync_u64(&line_tables_count, 0);
      lane_sync_u64(&src_line_tables, 0);
      lane_sync_u64(&baked_line_tables, 0);
      
      // rjf: allocate outputs
      ProfScope("allocate outputs")
      {
        if(lane_idx() == lane_from_task_idx(0))
        {
          unsorted_joined_line_tables = push_array(arena, RDIM_UnsortedJoinedLineTable, line_tables_count);
        }
        if(lane_idx() == lane_from_task_idx(1))
        {
          sorted_line_table_keys = push_array(arena, RDIM_SortKey *, line_tables_count);
        }
        if(lane_idx() == lane_from_task_idx(2))
        {
          baked_line_tables->line_tables = push_array(arena, RDI_LineTable, baked_line_tables->line_tables_count);
          ProfScope("lay out line tables")
          {
            U64 voffs_base_idx = 0;
            U64 lines_base_idx = 0;
            U64 cols_base_idx = 0;
            for EachIndex(idx, line_tables_count)
            {
              U64 final_idx = idx+1; // NOTE(rjf): +1, to reserve [0] for nil
              RDIM_LineTable *src = src_line_tables[idx];
              RDI_LineTable *dst = &baked_line_tables->line_tables[final_idx];
              dst->voffs_base_idx = voffs_base_idx; // TODO(rjf): @u64_to_u32
              dst->lines_base_idx = lines_base_idx; // TODO(rjf): @u64_to_u32
              dst->cols_base_idx  = cols_base_idx; // TODO(rjf): @u64_to_u32
              dst->lines_count    = src->line_count + src->seq_count; // TODO(rjf): @u64_to_u32
              voffs_base_idx += src->line_count + 2*src->seq_count;
              lines_base_idx += src->line_count + 1*src->seq_count;
            }
          }
        }
        if(lane_idx() == lane_from_task_idx(3))
        {
          baked_line_tables->line_table_voffs   = push_array(arena, RDI_U64, baked_line_tables->line_table_voffs_count);
        }
        if(lane_idx() == lane_from_task_idx(4))
        {
          baked_line_tables->line_table_lines   = push_array(arena, RDI_Line, baked_line_tables->line_table_lines_count);
        }
        if(lane_idx() == lane_from_task_idx(5))
        {
          baked_line_tables->line_table_columns = push_array(arena, RDI_Column, baked_line_tables->line_table_columns_count);
        }
      }
    }
    lane_sync_u64(&unsorted_joined_line_tables, lane_from_task_idx(0));
    lane_sync_u64(&sorted_line_table_keys, lane_from_task_idx(1));
    
    //- rjf: wide bake
    ProfScope("wide bake") 
    {
      U64 *line_table_block_take_counter = push_array(scratch.arena, U64, 1);
      lane_sync_u64(&line_table_block_take_counter, 0);
      U64 line_table_block_size = 4096;
      U64 line_table_block_count = (line_tables_count + line_table_block_size - 1) / line_table_block_size;
      for(;;)
      {
        U64 line_table_block_num = ins_atomic_u64_inc_eval(line_table_block_take_counter);
        if(0 == line_table_block_num || line_table_block_count < line_table_block_num)
        {
          break;
        }
        U64 line_table_block_idx = line_table_block_num-1;
        Rng1U64 line_table_range = r1u64(line_table_block_idx*line_table_block_size, (line_table_block_idx+1)*line_table_block_size);
        line_table_range.max = Min(line_tables_count, line_table_range.max);
        for EachInRange(line_table_idx, line_table_range)
        {
          RDIM_LineTable *src = src_line_tables[line_table_idx];
          RDIM_UnsortedJoinedLineTable *dst = &unsorted_joined_line_tables[line_table_idx];
          
          //- rjf: gather
          dst->line_count = src->line_count;
          dst->seq_count = src->seq_count;
          dst->key_count = dst->line_count + dst->seq_count;
          dst->line_keys = rdim_push_array_no_zero(arena, RDIM_SortKey, dst->key_count);
          dst->line_recs = rdim_push_array_no_zero(arena, RDIM_LineRec, dst->line_count);
          {
            RDIM_SortKey *key_ptr = dst->line_keys;
            RDIM_LineRec *rec_ptr = dst->line_recs;
            for(RDIM_LineSequenceNode *seq_n = src->first_seq; seq_n != 0; seq_n = seq_n->next)
            {
              RDIM_LineSequence *seq = &seq_n->v;
              for(RDI_U64 line_idx = 0; line_idx < seq->line_count; line_idx += 1)
              {
                key_ptr->key = seq->voffs[line_idx];
                key_ptr->val = rec_ptr;
                key_ptr += 1;
                rec_ptr->file_id = (RDI_U32)rdim_idx_from_src_file(seq->src_file); // TODO(rjf): @u64_to_u32
                rec_ptr->line_num = seq->line_nums[line_idx];
                if(seq->col_nums != 0)
                {
                  rec_ptr->col_first = seq->col_nums[line_idx*2];
                  rec_ptr->col_opl = seq->col_nums[line_idx*2 + 1];
                }
                rec_ptr += 1;
              }
              key_ptr->key = seq->voffs[seq->line_count];
              key_ptr->val = 0;
              key_ptr += 1;
            }
          }
          
          //- rjf: sort
          sorted_line_table_keys[line_table_idx] = rdim_sort_key_array(arena,
                                                                       unsorted_joined_line_tables[line_table_idx].line_keys,
                                                                       unsorted_joined_line_tables[line_table_idx].key_count);
          
          //- rjf: fill
          RDIM_SortKey *sorted_line_keys = sorted_line_table_keys[line_table_idx];
          U64 sorted_line_keys_count = unsorted_joined_line_tables[line_table_idx].key_count;
          RDI_LineTable *dst_line_table = &baked_line_tables->line_tables[line_table_idx+1];
          U64 *arranged_voffs           = baked_line_tables->line_table_voffs   + dst_line_table->voffs_base_idx;
          RDI_Line *arranged_lines      = baked_line_tables->line_table_lines   + dst_line_table->lines_base_idx;
          RDI_Column *arranged_cols     = baked_line_tables->line_table_columns + dst_line_table->cols_base_idx;
          if(sorted_line_keys_count > 0)
          {
            for EachIndex(idx, sorted_line_keys_count)
            {
              arranged_voffs[idx] = sorted_line_keys[idx].key;
            }
            arranged_voffs[sorted_line_keys_count] = ~0ull;
            for EachIndex(idx, sorted_line_keys_count)
            {
              RDIM_LineRec *rec = (RDIM_LineRec*)sorted_line_keys[idx].val;
              if(rec != 0)
              {
                arranged_lines[idx].file_idx = rec->file_id;
                arranged_lines[idx].line_num = rec->line_num;
              }
              else
              {
                arranged_lines[idx].file_idx = 0;
                arranged_lines[idx].line_num = 0;
              }
            }
          }
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage build string map
  //
  RDIM_BakeStringMapTight *bake_strings = 0;
  ProfScope("build string map")
  {
    Temp scratch2 = scratch_begin(&scratch.arena, 1);
    
    //- rjf: set up per-lane outputs
    RDIM_BakeStringMapTopology *top = 0;
    RDIM_BakeStringMapLoose **lane_maps__loose = 0;
    RDIM_BakeStringMapLoose *map__loose = 0;
    if(lane_idx() == 0) ProfScope("set up per-lane outputs")
    {
      bake_strings = push_array(scratch.arena, RDIM_BakeStringMapTight, 1);
      top = push_array(scratch2.arena, RDIM_BakeStringMapTopology, 1);
      top->slots_count = (64 +
                          params->procedures.total_count*1 +
                          params->global_variables.total_count*1 +
                          params->thread_variables.total_count*1 +
                          params->types.total_count/2);
      lane_maps__loose = push_array(scratch2.arena, RDIM_BakeStringMapLoose *, lane_count());
      map__loose = rdim_bake_string_map_loose_make(scratch2.arena, top);
    }
    lane_sync_u64(&bake_strings, 0);
    lane_sync_u64(&top, 0);
    lane_sync_u64(&lane_maps__loose, 0);
    lane_sync_u64(&map__loose, 0);
    
    //- rjf: set up this lane's map
    ProfScope("set up this lane's map")
    {
      lane_maps__loose[lane_idx()] = rdim_bake_string_map_loose_make(scratch2.arena, top);
    }
    RDIM_BakeStringMapLoose *lane_map = lane_maps__loose[lane_idx()];
    
    //- rjf: push all strings into this lane's map
    ProfScope("push all strings into this lane's map")
    {
      // rjf: push small top-level strings
      if(lane_idx() == 0) ProfScope("push small top-level strings")
      {
        rdim_bake_string_map_loose_insert(arena, top, lane_map, 1, params->top_level_info.exe_name);
        rdim_bake_string_map_loose_insert(arena, top, lane_map, 1, params->top_level_info.producer_name);
        for(RDIM_BinarySectionNode *n = params->binary_sections.first; n != 0; n = n->next)
        {
          rdim_bake_string_map_loose_insert(arena, top, lane_map, 1, n->v.name);
        }
        for(RDIM_BakePathNode *n = path_tree->first; n != 0; n = n->next_order)
        {
          rdim_bake_string_map_loose_insert(arena, top, lane_map, 1, n->name);
        }
      }
      
      // rjf: push strings from source files
      ProfScope("src files")
      {
        for EachNode(n, RDIM_SrcFileChunkNode, params->src_files.first)
        {
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            RDIM_String8 normalized_path = rdim_normalize_path_str8(arena, n->v[n_idx].path);
            rdim_bake_string_map_loose_insert(arena, top, lane_map, 1, normalized_path);
          }
        }
      }
      
      // rjf: push strings from units
      ProfScope("units")
      {
        for EachNode(n, RDIM_UnitChunkNode, params->units.first)
        {
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].unit_name);
            rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].compiler_name);
            rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].source_file);
            rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].object_file);
            rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].archive_file);
            rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].build_path);
          }
        }
      }
      
      // rjf: push strings from namespaces
      ProfScope("namespaces")
      {
        for EachNode(n, RDIM_NamespaceChunkNode, params->namespaces.first)
        {
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].name);
          }
        }
      }
      
      // rjf: push strings from types
      ProfScope("types")
      {
        for EachNode(n, RDIM_TypeChunkNode, params->types.first)
        {
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].name);
          }
        }
      }
      
      // rjf: push strings from udts
      ProfScope("udts")
      {
        for EachNode(n, RDIM_UDTChunkNode, params->udts.first)
        {
          Rng1U64 range = lane_range(n->count);
          for EachInRange(idx, range)
          {
            for EachNode(mem, RDIM_UDTMember, n->v[idx].first_member)
            {
              rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, mem->name);
            }
            for EachNode(enum_val, RDIM_UDTEnumVal, n->v[idx].first_enum_val)
            {
              rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, enum_val->name);
            }
          }
        }
      }
      
      // rjf: push strings from symbols
      RDIM_SymbolChunkList *symbol_lists[] =
      {
        &params->global_variables,
        &params->thread_variables,
        &params->procedures,
        &params->constants,
      };
      ProfScope("symbols")
      {
        for EachElement(list_idx, symbol_lists)
        {
          for EachNode(n, RDIM_SymbolChunkNode, symbol_lists[list_idx]->first)
          {
            Rng1U64 range = lane_range(n->count);
            for EachInRange(n_idx, range)
            {
              rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].name);
              rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].link_name);
            }
          }
        }
      }
      
      //- rjf: push strings from inline sites
      ProfScope("inline sites")
      {
        for EachNode(n, RDIM_InlineSiteChunkNode, params->inline_sites.first)
        {
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, n->v[n_idx].name);
          }
        }
      }
      
      //- rjf: push strings from scopes
      ProfScope("scopes")
      {
        for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
        {
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            for EachNode(local, RDIM_Local, n->v[n_idx].first_local)
            {
              rdim_bake_string_map_loose_insert(arena, top, lane_map, 4, local->name);
            }
          }
        }
      }
    }
    lane_sync();
    
    //- rjf: join
    ProfScope("join")
    {
      Rng1U64 slot_range = lane_range(top->slots_count);
      for EachInRange(slot_idx, slot_range)
      {
        for EachIndex(src_lane_idx, lane_count())
        {
          RDIM_BakeStringMapLoose *src_map = lane_maps__loose[src_lane_idx];
          RDIM_BakeStringMapLoose *dst_map = map__loose;
          if(dst_map->slots[slot_idx] == 0 && src_map->slots[slot_idx] != 0)
          {
            dst_map->slots[slot_idx] = src_map->slots[slot_idx];
          }
          else if(dst_map->slots[slot_idx] != 0 && src_map->slots[slot_idx] != 0)
          {
            rdim_bake_string_chunk_list_concat_in_place(dst_map->slots[slot_idx], src_map->slots[slot_idx]);
          }
        }
      }
    }
    lane_sync();
    
    //- rjf: sort
    ProfScope("sort")
    {
      RDIM_BakeStringMapLoose *map = map__loose;
      Rng1U64 slot_range = lane_range(top->slots_count);
      for EachInRange(slot_idx, slot_range)
      {
        if(map->slots[slot_idx] != 0)
        {
          *map->slots[slot_idx] = rdim_bake_string_chunk_list_sorted_from_unsorted(arena, map->slots[slot_idx]);
        }
      }
    }
    lane_sync();
    
    //- rjf: tighten string table
    ProfScope("tighten string table")
    {
      RDIM_BakeStringMapLoose *map = map__loose;
      if(lane_idx() == 0) ProfScope("calc base indices, set up tight map")
      {
        RDIM_BakeStringMapBaseIndices bake_string_map_base_indices = rdim_bake_string_map_base_indices_from_map_loose(arena, top, map);
        bake_strings->slots_count = top->slots_count;
        bake_strings->slots = rdim_push_array(arena, RDIM_BakeStringChunkList, bake_strings->slots_count);
        bake_strings->slots_base_idxs = bake_string_map_base_indices.slots_base_idxs;
        bake_strings->total_count = bake_strings->slots_base_idxs[bake_strings->slots_count];
      }
      lane_sync();
      ProfScope("fill tight map")
      {
        Rng1U64 slot_range = lane_range(bake_strings->slots_count);
        for EachInRange(idx, slot_range)
        {
          if(map->slots[idx] != 0)
          {
            rdim_memcpy_struct(&bake_strings->slots[idx], map->slots[idx]);
          }
        }
      }
    }
    
    lane_sync();
    scratch_end(scratch2);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage build name maps
  //
  B32 name_maps_need_build[RDI_NameMapKind_COUNT] = {0};
  {
    name_maps_need_build[RDI_NameMapKind_GlobalVariables]    = !!(params->subset_flags & RDIM_SubsetFlag_GlobalVariableNameMap);
    name_maps_need_build[RDI_NameMapKind_ThreadVariables]    = !!(params->subset_flags & RDIM_SubsetFlag_ThreadVariableNameMap);
    name_maps_need_build[RDI_NameMapKind_Constants]          = !!(params->subset_flags & RDIM_SubsetFlag_ConstantNameMap);
    name_maps_need_build[RDI_NameMapKind_Procedures]         = !!(params->subset_flags & RDIM_SubsetFlag_ProcedureNameMap);
    name_maps_need_build[RDI_NameMapKind_Types]              = !!(params->subset_flags & RDIM_SubsetFlag_TypeNameMap);
    name_maps_need_build[RDI_NameMapKind_LinkNameProcedures] = !!(params->subset_flags & RDIM_SubsetFlag_LinkNameProcedureNameMap);
    name_maps_need_build[RDI_NameMapKind_NormalSourcePaths]  = !!(params->subset_flags & RDIM_SubsetFlag_NormalSourcePathNameMap);
  }
  RDIM_BakeNameMapTopology *bake_name_maps_tops = 0;
  RDIM_BakeNameMap **bake_name_maps = 0;
  ProfScope("build name maps")
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: set up
    RDIM_BakeNameMap ***lane_maps = 0;
    if(lane_idx() == 0)
    {
      bake_name_maps_tops = push_array(arena, RDIM_BakeNameMapTopology, RDI_NameMapKind_COUNT);
      bake_name_maps = push_array(arena, RDIM_BakeNameMap *, RDI_NameMapKind_COUNT);
      lane_maps = push_array(scratch.arena, RDIM_BakeNameMap **, RDI_NameMapKind_COUNT);
      for EachNonZeroEnumVal(RDI_NameMapKind, k)
      {
        U64 slot_count = 0;
        switch((RDI_NameMapKindEnum)k)
        {
          case RDI_NameMapKind_NULL:
          case RDI_NameMapKind_COUNT:
          {}break;
#define Case(name, total_count) case RDI_NameMapKind_##name:{slot_count = ((total_count) + (total_count)/4);}break
          Case(GlobalVariables, params->global_variables.total_count);
          Case(ThreadVariables, params->thread_variables.total_count);
          Case(Constants, params->constants.total_count);
          Case(Procedures, params->procedures.total_count);
          Case(LinkNameProcedures, params->procedures.total_count);
          Case(Types, params->types.total_count);
          Case(NormalSourcePaths, params->src_files.total_count);
#undef Case
        }
        lane_maps[k] = push_array(arena, RDIM_BakeNameMap *, lane_count());
        bake_name_maps_tops[k].slots_count = slot_count;
      }
    }
    lane_sync_u64(&bake_name_maps_tops, 0);
    lane_sync_u64(&bake_name_maps, 0);
    lane_sync_u64(&lane_maps, 0);
    
    //- rjf: wide build
    for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("name map build %.*s", str8_varg(rdi_string_from_name_map_kind(k)))
    {
      if(!name_maps_need_build[k]) { continue; }
      RDIM_BakeNameMapTopology *top = &bake_name_maps_tops[k];
      lane_maps[k][lane_idx()] = rdim_bake_name_map_make(scratch.arena, top);
      RDIM_BakeNameMap *map = lane_maps[k][lane_idx()];
      B32 link_names = 0;
      RDIM_SymbolChunkList *symbols = 0;
      switch((RDI_NameMapKindEnum)k)
      {
        case RDI_NameMapKind_NULL:
        case RDI_NameMapKind_COUNT:
        {}break;
        case RDI_NameMapKind_GlobalVariables:   {symbols = &params->global_variables;}goto symbol_name_map_build;
        case RDI_NameMapKind_ThreadVariables:   {symbols = &params->thread_variables;}goto symbol_name_map_build;
        case RDI_NameMapKind_Constants:         {symbols = &params->constants;}goto symbol_name_map_build;
        case RDI_NameMapKind_Procedures:        {symbols = &params->procedures;}goto symbol_name_map_build;
        case RDI_NameMapKind_LinkNameProcedures:{symbols = &params->procedures; link_names = 1;}goto symbol_name_map_build;
        symbol_name_map_build:;
        {
          for EachNode(n, RDIM_SymbolChunkNode, symbols->first)
          {
            Rng1U64 n_range = lane_range(n->count);
            for EachInRange(n_idx, n_range)
            {
              RDIM_Symbol *symbol = &n->v[n_idx];
              rdim_bake_name_map_insert(scratch.arena, top, map, 4, link_names ? symbol->link_name : symbol->name, rdim_idx_from_symbol(symbol));
            }
          }
        }break;
        case RDI_NameMapKind_Types:
        {
          RDIM_TypeChunkList *types = &params->types;
          for EachNode(n, RDIM_TypeChunkNode, types->first)
          {
            Rng1U64 n_range = lane_range(n->count);
            for EachInRange(n_idx, n_range)
            {
              RDIM_Type *type = &n->v[n_idx];
              rdim_bake_name_map_insert(scratch.arena, top, map, 4, type->name, rdim_idx_from_type(type));
            }
          }
        }break;
        case RDI_NameMapKind_NormalSourcePaths:
        {
          RDIM_SrcFileChunkList *src_files = &params->src_files;
          for EachNode(n, RDIM_SrcFileChunkNode, src_files->first)
          {
            Rng1U64 n_range = lane_range(n->count);
            for EachInRange(n_idx, n_range)
            {
              RDIM_SrcFile *src_file = &n->v[n_idx];
              RDIM_String8 normalized_path = rdim_normalize_path_str8(arena, src_file->path);
              rdim_bake_name_map_insert(scratch.arena, top, map, 4, normalized_path, rdim_idx_from_src_file(src_file));
            }
          }
        }break;
      }
    }
    lane_sync();
    
    //- rjf: join & sort
    if(lane_idx() == 0)
    {
      for EachNonZeroEnumVal(RDI_NameMapKind, k)
      {
        if(!name_maps_need_build[k]) { continue; }
        bake_name_maps[k] = rdim_bake_name_map_make(arena, &bake_name_maps_tops[k]);
      }
    }
    lane_sync();
    for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("name map join & sort %.*s", str8_varg(rdi_string_from_name_map_kind(k)))
    {
      if(!name_maps_need_build[k]) { continue; }
      RDIM_BakeNameMapTopology *top = &bake_name_maps_tops[k];
      RDIM_BakeNameMap *map = bake_name_maps[k];
      
      //- rjf: join
      ProfScope("join")
      {
        Rng1U64 slot_range = lane_range(top->slots_count);
        for EachInRange(slot_idx, slot_range)
        {
          for EachIndex(src_lane_idx, lane_count())
          {
            RDIM_BakeNameMap *src_map = lane_maps[k][src_lane_idx];
            RDIM_BakeNameMap *dst_map = map;
            if(dst_map->slots[slot_idx] == 0 && src_map->slots[slot_idx] != 0)
            {
              dst_map->slots[slot_idx] = src_map->slots[slot_idx];
            }
            else if(dst_map->slots[slot_idx] != 0 && src_map->slots[slot_idx] != 0)
            {
              rdim_bake_name_chunk_list_concat_in_place(dst_map->slots[slot_idx], src_map->slots[slot_idx]);
            }
          }
        }
      }
      
      //- rjf: sort
      ProfScope("sort")
      {
        Rng1U64 slot_range = lane_range(top->slots_count);
        for EachInRange(slot_idx, slot_range)
        {
          if(map->slots[slot_idx] != 0)
          {
            RDIM_BakeNameChunkList *scratch_og_unsorted = map->slots[slot_idx];
            map->slots[slot_idx] = push_array(arena, RDIM_BakeNameChunkList, 1);
            *map->slots[slot_idx] = rdim_bake_name_chunk_list_sorted_from_unsorted(arena, scratch_og_unsorted);
          }
        }
      }
    }
    lane_sync();
    
    scratch_end(scratch);
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage build index runs
  //
  RDIM_BakeIdxRunMap *bake_idx_runs = 0;
  if(lane_idx() == 0)
  {
    bake_idx_runs = push_array(scratch.arena, RDIM_BakeIdxRunMap, 1);
  }
  lane_sync_u64(&bake_idx_runs, 0);
  B32 need_index_runs = (!!(params->subset_flags & RDIM_SubsetFlag_NameMaps) ||
                         !!(params->subset_flags & RDIM_SubsetFlag_Types));
  if(need_index_runs) ProfScope("build index runs")
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: set up per-lane outputs
    RDIM_BakeIdxRunMapTopology *top = 0;
    RDIM_BakeIdxRunMapLoose **lane_maps__loose = 0;
    RDIM_BakeIdxRunMapLoose *map__loose = 0;
    if(lane_idx() == 0) ProfScope("set up per-lane outputs")
    {
      top = push_array(scratch.arena, RDIM_BakeIdxRunMapTopology, 1);
      top->slots_count = 64 + ((params->procedures.total_count +
                                params->global_variables.total_count +
                                params->thread_variables.total_count +
                                params->udts.total_count) * 3) / 4;
      lane_maps__loose = push_array(scratch.arena, RDIM_BakeIdxRunMapLoose *, lane_count());
      map__loose = rdim_bake_idx_run_map_loose_make(scratch.arena, top);
    }
    lane_sync_u64(&top, 0);
    lane_sync_u64(&lane_maps__loose, 0);
    lane_sync_u64(&map__loose, 0);
    
    //- rjf: set up this lane's map
    ProfScope("set up this lane's map")
    {
      lane_maps__loose[lane_idx()] = rdim_bake_idx_run_map_loose_make(scratch.arena, top);
    }
    RDIM_BakeIdxRunMapLoose *lane_map = lane_maps__loose[lane_idx()];
    
    //- rjf: wide fill of all index runs
    ProfScope("fill all lane index run maps")
    {
      //- rjf: bake runs of function-type parameter lists
      ProfScope("bake runs of function-type parameter lists")
      {
        for EachNode(n, RDIM_TypeChunkNode, params->types.first)
        {
          Rng1U64 range = lane_range(n->count);
          ProfScope("[%I64u, %I64u)", range.min, range.max) for EachInRange(n_idx, range)
          {
            RDIM_Type *type = &n->v[n_idx];
            if(type->count == 0)
            {
              continue;
            }
            if(type->kind == RDI_TypeKind_Function || type->kind == RDI_TypeKind_Method)
            {
              RDI_U32 param_idx_run_count = type->count;
              RDI_U32 *param_idx_run = rdim_push_array_no_zero(arena, RDI_U32, param_idx_run_count);
              for(RDI_U32 idx = 0; idx < param_idx_run_count; idx += 1)
              {
                param_idx_run[idx] = (RDI_U32)rdim_idx_from_type(type->param_types[idx]); // TODO(rjf): @u64_to_u32
              }
              rdim_bake_idx_run_map_loose_insert(scratch.arena, top, lane_map, 4, param_idx_run, param_idx_run_count);
            }
          }
        }
      }
      
      //- rjf: bake runs of name map match lists
      for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("bake runs of name map match lists (%.*s)", str8_varg(rdi_string_from_name_map_kind(k)))
      {
        if(!name_maps_need_build[k]) { continue; }
        RDIM_BakeNameMapTopology *name_map_top = &bake_name_maps_tops[k];
        RDIM_BakeNameMap *name_map = bake_name_maps[k];
        Rng1U64 slot_idx_range = lane_range(name_map_top->slots_count);
        for EachInRange(slot_idx, slot_idx_range)
        {
          RDIM_BakeNameChunkList *slot = name_map->slots[slot_idx];
          if(slot != 0)
          {
            typedef struct IdxRunNode IdxRunNode;
            struct IdxRunNode
            {
              IdxRunNode *next;
              RDI_U64 idx;
            };
            IdxRunNode *first_idx_run_node = 0;
            IdxRunNode *last_idx_run_node = 0;
            U64 active_idx_count = 0;
            String8 active_string = {0};
            RDIM_BakeNameChunkNode *n = slot->first;
            U64 n_idx = 0;
            for(;;)
            {
              // rjf: advance chunk
              if(n != 0 && n_idx >= n->count)
              {
                n = n->next;
                n_idx = 0;
              }
              
              // rjf: grab next element
              String8 string = {0};
              U64 idx = 0;
              if(n != 0)
              {
                string = n->v[n_idx].string;
                idx  = n->v[n_idx].idx;
              }
              
              // rjf: next element hash doesn't match the active? -> push index run, clear active list, start new list
              if(!str8_match(string, active_string, 0))
              {
                if(active_idx_count > 1)
                {
                  RDI_U64 idxs_count = active_idx_count;
                  RDI_U32 *idxs = rdim_push_array(arena, RDI_U32, idxs_count);
                  {
                    U64 write_idx = 0;
                    for EachNode(idx_run_n, IdxRunNode, first_idx_run_node)
                    {
                      idxs[write_idx] = (RDI_U32)idx_run_n->idx; // TODO(rjf): @u64_to_u32
                      write_idx += 1;
                    }
                  }
                  rdim_bake_idx_run_map_loose_insert(scratch.arena, top, lane_map, 4, idxs, idxs_count);
                }
                active_string = string;
                first_idx_run_node = 0;
                last_idx_run_node = 0;
                active_idx_count = 0;
              }
              
              // rjf: new element matches the active list -> push
              if(active_string.size != 0 && str8_match(string, active_string, 0))
              {
                IdxRunNode *idx_run_n = push_array(scratch.arena, IdxRunNode, 1);
                idx_run_n->idx = idx;
                SLLQueuePush(first_idx_run_node, last_idx_run_node, idx_run_n);
                active_idx_count += 1;
              }
              
              // rjf: advance index
              n_idx += 1;
              
              // rjf: end on zero node
              if(n == 0)
              {
                break;
              }
            }
          }
        }
      }
    }
    lane_sync();
    
    //- rjf: join
    ProfScope("join")
    {
      Rng1U64 slot_range = lane_range(top->slots_count);
      for EachInRange(slot_idx, slot_range)
      {
        for EachIndex(src_lane_idx, lane_count())
        {
          RDIM_BakeIdxRunMapLoose *src_map = lane_maps__loose[src_lane_idx];
          RDIM_BakeIdxRunMapLoose *dst_map = map__loose;
          dst_map->slots_idx_counts[slot_idx] += src_map->slots_idx_counts[slot_idx];
          if(dst_map->slots[slot_idx] == 0 && src_map->slots[slot_idx] != 0)
          {
            dst_map->slots[slot_idx] = src_map->slots[slot_idx];
          }
          else if(dst_map->slots[slot_idx] != 0 && src_map->slots[slot_idx] != 0)
          {
            rdim_bake_idx_run_chunk_list_concat_in_place(dst_map->slots[slot_idx], src_map->slots[slot_idx]);
          }
        }
      }
    }
    lane_sync();
    
    //- rjf: sort
    ProfScope("sort")
    {
      RDIM_BakeIdxRunMapLoose *map = map__loose;
      Rng1U64 slot_range = lane_range(top->slots_count);
      for EachInRange(slot_idx, slot_range)
      {
        if(map->slots[slot_idx] != 0)
        {
          *map->slots[slot_idx] = rdim_bake_idx_run_chunk_list_sorted_from_unsorted(arena, map->slots[slot_idx]);
          map->slots_idx_counts[slot_idx] = 0;
          for EachNode(n, RDIM_BakeIdxRunChunkNode, map->slots[slot_idx]->first)
          {
            for EachIndex(idx, n->count)
            {
              map->slots_idx_counts[slot_idx] += n->v[idx].count;
            }
          }
        }
      }
    }
    lane_sync();
    
    //- rjf: tighten idx run table
    ProfScope("tighten idx run table")
    {
      RDIM_BakeIdxRunMapLoose *map = map__loose;
      if(lane_idx() == 0) ProfScope("calc base indices, set up tight map")
      {
        bake_idx_runs->slots_count = top->slots_count;
        bake_idx_runs->slots = rdim_push_array(arena, RDIM_BakeIdxRunChunkList, bake_idx_runs->slots_count);
        bake_idx_runs->slots_base_idxs = rdim_push_array(arena, RDI_U64, bake_idx_runs->slots_count+1);
        RDI_U64 encoding_idx_off = 0;
        for(RDI_U64 slot_idx = 0; slot_idx < top->slots_count; slot_idx += 1)
        {
          bake_idx_runs->slots_base_idxs[slot_idx] = encoding_idx_off;
          if(map->slots[slot_idx] != 0)
          {
            encoding_idx_off += map->slots_idx_counts[slot_idx];
          }
        }
        bake_idx_runs->slots_base_idxs[top->slots_count] = encoding_idx_off;
      }
      lane_sync();
      ProfScope("fill tight map")
      {
        Rng1U64 slot_range = lane_range(bake_idx_runs->slots_count);
        for EachInRange(idx, slot_range)
        {
          if(map->slots[idx] != 0)
          {
            rdim_memcpy_struct(&bake_idx_runs->slots[idx], map->slots[idx]);
          }
        }
      }
    }
    lane_sync();
    
    scratch_end(scratch);
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage build deduplicated location maps
  //
  RDIM_BakeStringMapTight *bake_locs__regs = 0;
  RDIM_BakeStringMapTight *bake_locs__reg_plus_u16 = 0;
  RDIM_BakeStringMapTight *bake_locs__bytecode = 0;
  {
    
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake strings
  //
  RDIM_StringBakeResult *baked_strings = 0;
  ProfScope("bake strings")
  {
    // rjf: set up 
    if(lane_idx() == 0) ProfScope("set up; lay out strings")
    {
      baked_strings = push_array(scratch.arena, RDIM_StringBakeResult, 1);
      baked_strings->string_offs_count = bake_strings->total_count + 1;
      baked_strings->string_offs = rdim_push_array(arena, RDI_U32, baked_strings->string_offs_count);
      RDI_U64 off_cursor = 0;
      for EachIndex(slot_idx, bake_strings->slots_count)
      {
        for EachNode(n, RDIM_BakeStringChunkNode, bake_strings->slots[slot_idx].first)
        {
          for EachIndex(n_idx, n->count)
          {
            RDIM_BakeString *src = &n->v[n_idx];
            U64 dst_idx = bake_strings->slots_base_idxs[slot_idx] + n->base_idx + n_idx + 1;
            baked_strings->string_offs[dst_idx] = off_cursor;
            off_cursor += src->string.size;
          }
        }
      }
      baked_strings->string_data_size = off_cursor;
      baked_strings->string_data = rdim_push_array(arena, RDI_U8, baked_strings->string_data_size);
    }
    lane_sync_u64(&baked_strings, 0);
    
    // rjf: wide fill string data
    ProfScope("wide fill")
    {
      Rng1U64 slot_idx_range = lane_range(bake_strings->slots_count);
      for EachInRange(slot_idx, slot_idx_range)
      {
        for EachNode(n, RDIM_BakeStringChunkNode, bake_strings->slots[slot_idx].first)
        {
          for EachIndex(n_idx, n->count)
          {
            RDIM_BakeString *src = &n->v[n_idx];
            U64 dst_idx = bake_strings->slots_base_idxs[slot_idx] + n->base_idx + n_idx + 1;
            U64 dst_off = baked_strings->string_offs[dst_idx];
            rdim_memcpy(baked_strings->string_data + dst_off, src->string.str, src->string.size);
          }
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake idx runs
  //
  RDIM_IndexRunBakeResult *baked_idx_runs = 0;
  if(lane_idx() == 0)
  {
    baked_idx_runs = push_array(scratch.arena, RDIM_IndexRunBakeResult, 1);
  }
  lane_sync_u64(&baked_idx_runs, 0);
  if(need_index_runs) ProfScope("bake idx runs")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      baked_idx_runs->idx_count = bake_idx_runs->slots_base_idxs[bake_idx_runs->slots_count];
      baked_idx_runs->idx_runs = push_array(arena, RDI_U32, baked_idx_runs->idx_count);
    }
    lane_sync();
    
    // rjf: wide fill
    {
      Rng1U64 range = lane_range(bake_idx_runs->slots_count);
      for EachInRange(slot_idx, range)
      {
        RDI_U64 off = bake_idx_runs->slots_base_idxs[slot_idx];
        for EachNode(n, RDIM_BakeIdxRunChunkNode, bake_idx_runs->slots[slot_idx].first)
        {
          StaticAssert(sizeof(baked_idx_runs->idx_runs[0]) == sizeof(n->v[0].idxes[0]), idx_run_size_check);
          for EachIndex(n_idx, n->count)
          {
            rdim_memcpy(baked_idx_runs->idx_runs + off, n->v[n_idx].idxes, sizeof(n->v[n_idx].idxes[0]) * n->v[n_idx].count);
            off += n->v[n_idx].count;
          }
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake name maps
  //
  RDIM_TopLevelNameMapBakeResult *baked_top_level_name_maps = 0;
  RDIM_NameMapBakeResult *baked_name_maps = 0;
  ProfScope("bake name maps")
  {
    // rjf: count unique names in all name maps; lay out baked nodes
    U64 **lane_name_map_node_counts = 0; // [RDI_NameMapKind_COUNT][..]
    U64 **lane_name_map_node_offs = 0; // [RDI_NameMapKind_COUNT][..]
    U64 *name_map_node_counts = 0; // [RDI_NameMapKind_COUNT]
    U64 total_name_map_node_count = 0;
    ProfScope("count unique names in all name maps; lay out baked nodes")
    {
      if(lane_idx() == 0)
      {
        lane_name_map_node_counts = push_array(scratch.arena, U64 *, RDI_NameMapKind_COUNT);
        lane_name_map_node_offs = push_array(scratch.arena, U64 *, RDI_NameMapKind_COUNT);
        name_map_node_counts = push_array(scratch.arena, U64, RDI_NameMapKind_COUNT);
        for EachNonZeroEnumVal(RDI_NameMapKind, k)
        {
          lane_name_map_node_counts[k] = push_array(scratch.arena, U64, lane_count());
          lane_name_map_node_offs[k] = push_array(scratch.arena, U64, lane_count());
        }
      }
      lane_sync_u64(&lane_name_map_node_counts, 0);
      lane_sync_u64(&lane_name_map_node_offs, 0);
      lane_sync_u64(&name_map_node_counts, 0);
      for EachNonZeroEnumVal(RDI_NameMapKind, k)
      {
        if(!name_maps_need_build[k]) { continue; }
        RDIM_BakeNameMapTopology *top = &bake_name_maps_tops[k];
        RDIM_BakeNameMap *map = bake_name_maps[k];
        Rng1U64 range = lane_range(top->slots_count);
        for EachInRange(idx, range)
        {
          if(map->slots[idx] != 0)
          {
            U64 total_unique_name_count = 0;
            U64 last_hash = 0;
            for EachNode(n, RDIM_BakeNameChunkNode, map->slots[idx]->first)
            {
              for EachIndex(n_idx, n->count)
              {
                if(n->v[n_idx].hash != last_hash)
                {
                  total_unique_name_count += 1;
                  last_hash = n->v[n_idx].hash;
                }
              }
            }
            lane_name_map_node_counts[k][lane_idx()] += total_unique_name_count;
          }
        }
      }
      lane_sync();
      if(lane_idx() == 0)
      {
        for EachNonZeroEnumVal(RDI_NameMapKind, k)
        {
          RDI_U64 node_off = 0;
          for EachIndex(l_idx, lane_count())
          {
            name_map_node_counts[k] += lane_name_map_node_counts[k][l_idx];
            lane_name_map_node_offs[k][l_idx] = node_off;
            node_off += lane_name_map_node_counts[k][l_idx];
          }
          total_name_map_node_count += name_map_node_counts[k];
        }
      }
      lane_sync_u64(&total_name_map_node_count, 0);
    }
    lane_sync();
    
    // rjf: setup
    ProfScope("setup") if(lane_idx() == 0)
    {
      baked_top_level_name_maps = push_array(scratch.arena, RDIM_TopLevelNameMapBakeResult, 1);
      baked_top_level_name_maps->name_maps_count = RDI_NameMapKind_COUNT;
      baked_top_level_name_maps->name_maps = push_array(arena, RDI_NameMap, baked_top_level_name_maps->name_maps_count);
      baked_name_maps = push_array(scratch.arena, RDIM_NameMapBakeResult, 1);
      RDI_U32 bucket_off = 0;
      RDI_U32 node_off = 0;
      for EachNonZeroEnumVal(RDI_NameMapKind, k)
      {
        baked_top_level_name_maps->name_maps[k].bucket_base_idx = bucket_off;
        baked_top_level_name_maps->name_maps[k].node_base_idx = node_off;
        baked_top_level_name_maps->name_maps[k].bucket_count = (RDI_U32)bake_name_maps_tops[k].slots_count; // TODO(rjf): @u64_to_u32
        baked_top_level_name_maps->name_maps[k].node_count = (RDI_U32)name_map_node_counts[k]; // TODO(rjf): @u64_to_u32
        bucket_off += baked_top_level_name_maps->name_maps[k].bucket_count;
        node_off += baked_top_level_name_maps->name_maps[k].node_count;
      }
      baked_name_maps->buckets_count = bucket_off;
      baked_name_maps->buckets = push_array(arena, RDI_NameMapBucket, baked_name_maps->buckets_count);
      baked_name_maps->nodes_count = total_name_map_node_count;
      baked_name_maps->nodes = push_array(arena, RDI_NameMapNode, baked_name_maps->nodes_count);
    }
    lane_sync_u64(&baked_top_level_name_maps, 0);
    lane_sync_u64(&baked_name_maps, 0);
    
    // rjf: wide fill baked name maps
    ProfScope("wide fill baked name maps")
    {
      for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("wide fill (%.*s)", str8_varg(rdi_string_from_name_map_kind(k)))
      {
        if(!name_maps_need_build[k]) { continue; }
        RDI_U64 write_node_off = lane_name_map_node_offs[k][lane_idx()];
        RDIM_BakeNameMapTopology *top = &bake_name_maps_tops[k];
        U64 slots_count = top->slots_count;
        RDIM_BakeNameMap *src_map = bake_name_maps[k];
        RDI_NameMap *dst_map = &baked_top_level_name_maps->name_maps[k];
        RDI_NameMapBucket *dst_buckets = baked_name_maps->buckets + dst_map->bucket_base_idx;
        RDI_NameMapNode *dst_nodes = baked_name_maps->nodes + dst_map->node_base_idx;
        Rng1U64 slot_range = lane_range(slots_count);
        for EachInRange(slot_idx, slot_range)
        {
          RDIM_BakeNameChunkList *src_slot = src_map->slots[slot_idx];
          if(src_slot == 0) { continue; }
          RDI_NameMapBucket *dst_bucket = &dst_buckets[slot_idx];
          dst_bucket->first_node = write_node_off;
          {
            Temp scratch2 = scratch_begin(&scratch.arena, 1);
            typedef struct IdxRunNode IdxRunNode;
            struct IdxRunNode
            {
              IdxRunNode *next;
              RDI_U64 idx;
            };
            IdxRunNode *first_idx_run_node = 0;
            IdxRunNode *last_idx_run_node = 0;
            U64 active_idx_count = 0;
            String8 active_string = {0};
            RDIM_BakeNameChunkNode *n = src_slot->first;
            U64 n_idx = 0;
            for(;;)
            {
              // rjf: advance chunk
              if(n != 0 && n_idx >= n->count)
              {
                n = n->next;
                n_idx = 0;
              }
              
              // rjf: grab next element
              U64 idx = 0;
              String8 string = {0};
              if(n != 0)
              {
                idx    = n->v[n_idx].idx;
                string = n->v[n_idx].string;
              }
              
              // rjf: next element doesn't match the active list? -> push index run, clear active list, start new list
              if(!str8_match(active_string, string, 0))
              {
                // rjf: has active run -> flatten & serialize
                if(active_string.size != 0)
                {
                  // rjf: flatten idxes
                  RDI_U64 idxs_count = active_idx_count;
                  RDI_U32 *idxs = rdim_push_array(scratch2.arena, RDI_U32, idxs_count);
                  {
                    U64 write_idx = 0;
                    for EachNode(idx_run_n, IdxRunNode, first_idx_run_node)
                    {
                      idxs[write_idx] = (RDI_U32)idx_run_n->idx; // TODO(rjf): @u64_to_u32
                      write_idx += 1;
                    }
                  }
                  
                  // rjf: serialize node
                  RDI_NameMapNode *dst_node = &dst_nodes[write_node_off];
                  dst_node->string_idx = rdim_bake_idx_from_string(bake_strings, active_string);
                  dst_node->match_count = idxs_count;
                  if(dst_node->match_count == 1)
                  {
                    dst_node->match_idx_or_idx_run_first = idxs[0];
                  }
                  else if(dst_node->match_count > 1)
                  {
                    dst_node->match_idx_or_idx_run_first = rdim_bake_idx_from_idx_run(bake_idx_runs, idxs, idxs_count);
                  }
                  dst_bucket->node_count += 1;
                  write_node_off += 1;
                }
                
                // rjf: start new list
                active_string = string;
                first_idx_run_node = 0;
                last_idx_run_node = 0;
                active_idx_count = 0;
                temp_end(scratch2);
              }
              
              // rjf: hash matches the active list -> push
              if(active_string.size != 0 && str8_match(active_string, string, 0))
              {
                IdxRunNode *idx_run_n = push_array(scratch2.arena, IdxRunNode, 1);
                idx_run_n->idx = idx;
                SLLQueuePush(first_idx_run_node, last_idx_run_node, idx_run_n);
                active_idx_count += 1;
              }
              
              // rjf: advance index
              n_idx += 1;
              
              // rjf: end on zero node
              if(n == 0)
              {
                break;
              }
            }
            scratch_end(scratch2);
          }
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage gather line-bucketed src line map data
  //
  RDIM_BakeSrcLineMap *bake_src_line_maps = 0;
  ProfScope("gather line-bucketed src line map data")
  {
    if(lane_idx() == 0)
    {
      bake_src_line_maps = push_array(scratch.arena, RDIM_BakeSrcLineMap, params->src_files.total_count);
    }
    lane_sync_u64(&bake_src_line_maps, 0);
    {
      for EachNode(n, RDIM_SrcFileChunkNode, params->src_files.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          U64 file_idx = n->base_idx + n_idx;
          RDIM_BakeSrcLineMap *map = &bake_src_line_maps[file_idx];
          
          // rjf: set up map
          map->slots_count = Max(n->v[n_idx].total_line_count, 1);
          map->slots = push_array(arena, RDIM_BakeSrcLineMapSlot, map->slots_count);
          
          // rjf: gather line-bucketed info
          for EachNode(frag, RDIM_SrcFileLineMapFragment, n->v[n_idx].first_line_map_fragment)
          {
            RDIM_LineSequence *seq = frag->seq;
            for EachIndex(idx, seq->line_count)
            {
              RDI_U32 line_num = seq->line_nums[idx];
              RDI_U64 voff_first = seq->voffs[idx];
              RDI_U64 voff_opl = seq->voffs[idx+1];
              RDI_U64 slot_idx = line_num%map->slots_count;
              
              // rjf: find existing line node
              RDIM_BakeSrcLineMapNode *line_node = 0;
              {
                for EachNode(line_n, RDIM_BakeSrcLineMapNode, map->slots[slot_idx].first)
                {
                  if(line_n->line_num == line_num)
                  {
                    line_node = line_n;
                    break;
                  }
                }
              }
              
              // rjf: construct new node if unseen
              if(line_node == 0)
              {
                line_node = push_array(arena, RDIM_BakeSrcLineMapNode, 1);
                SLLQueuePush(map->slots[slot_idx].first, map->slots[slot_idx].last, line_node);
                line_node->line_num = line_num;
                map->line_count += 1;
              }
              
              // rjf: push this voff range
              RDIM_Rng1U64 voff_range = {voff_first, voff_opl};
              rdim_rng1u64_list_push(arena, &line_node->voff_ranges, voff_range);
              map->voff_range_count += 1;
            }
          }
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage sort line-bucketed src line map data
  //
  RDIM_SortKey **bake_src_line_map_keys = 0;
  ProfScope("sort line-bucketed src line map data")
  {
    U64 *map_take_idx = push_array(scratch.arena, U64, 1);
    U64 map_count = params->src_files.total_count;
    if(lane_idx() == 0)
    {
      bake_src_line_map_keys = push_array(arena, RDIM_SortKey *, map_count);
    }
    lane_sync_u64(&bake_src_line_map_keys, 0);
    lane_sync_u64(&map_take_idx, 0);
    for(;;)
    {
      U64 map_num = ins_atomic_u64_inc_eval(map_take_idx);
      if(map_num < 1 || map_count < map_num)
      {
        break;
      }
      U64 map_idx = map_num-1;
      RDIM_BakeSrcLineMap *map = &bake_src_line_maps[map_idx];
      
      // rjf: gather keys
      bake_src_line_map_keys[map_idx] = push_array_no_zero(arena, RDIM_SortKey, map->line_count);
      RDIM_SortKey *keys = bake_src_line_map_keys[map_idx];
      {
        U64 key_idx = 0;
        for EachIndex(slot_idx, map->slots_count)
        {
          for EachNode(n, RDIM_BakeSrcLineMapNode, map->slots[slot_idx].first)
          {
            keys[key_idx].key = n->line_num;
            keys[key_idx].val = n;
            key_idx += 1;
          }
        }
      }
      
      // rjf: sort keys
      {
        radsort(keys, map->line_count, rdim_sort_key_is_before);
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage compute src file / src file line map layout
  //
  typedef struct SrcMapLayout SrcMapLayout;
  struct SrcMapLayout
  {
    RDI_U64 *lane_chunk_src_file_num_counts; // [lane_count * src_file_chunk_count]
    RDI_U64 *lane_chunk_src_file_voff_counts; // [lane_count * src_file_chunk_count]
    RDI_U64 *lane_chunk_src_file_map_counts; // [lane_count * src_file_chunk_count]
    RDI_U64 *lane_chunk_src_file_num_offs; // [lane_count * src_file_chunk_count]
    RDI_U64 *lane_chunk_src_file_voff_offs; // [lane_count * src_file_chunk_count]
    RDI_U64 *lane_chunk_src_file_map_offs; // [lane_count * src_file_chunk_count]
    RDI_U64 *lane_chunk_src_file_checksum_counts[RDI_ChecksumKind_COUNT]; // [lane_count * src_file_chunk_count]
    RDI_U64 *lane_chunk_src_file_checksum_offs[RDI_ChecksumKind_COUNT]; // [lane_count * src_file_chunk_count]
    U64 total_checksum_counts[RDI_ChecksumKind_COUNT];
    RDI_U64 total_src_map_line_count;
    RDI_U64 total_src_map_voff_count;
  };
  SrcMapLayout *src_map_layout = 0;
  ProfScope("compute src file / src file line map layout")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      src_map_layout = push_array(scratch.arena, SrcMapLayout, 1);
      src_map_layout->lane_chunk_src_file_num_counts  = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      src_map_layout->lane_chunk_src_file_voff_counts = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      src_map_layout->lane_chunk_src_file_map_counts  = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      src_map_layout->lane_chunk_src_file_num_offs    = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      src_map_layout->lane_chunk_src_file_voff_offs   = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      src_map_layout->lane_chunk_src_file_map_offs    = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      for EachEnumVal(RDI_ChecksumKind, k)
      {
        src_map_layout->lane_chunk_src_file_checksum_counts[k] = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
        src_map_layout->lane_chunk_src_file_checksum_offs[k] = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      }
    }
    lane_sync_u64(&src_map_layout, 0);
    
    // rjf: wide count
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_SrcFileChunkNode, params->src_files.first)
      {
        Rng1U64 range = lane_range(n->count);
        U64 slot_idx = lane_idx()*params->src_files.chunk_count + chunk_idx;
        for EachInRange(idx, range)
        {
          RDIM_BakeSrcLineMap *map = &bake_src_line_maps[n->base_idx + idx];
          src_map_layout->lane_chunk_src_file_num_counts[slot_idx] += map->line_count;
          src_map_layout->lane_chunk_src_file_voff_counts[slot_idx] += map->voff_range_count;
          src_map_layout->lane_chunk_src_file_map_counts[slot_idx] += !!map->line_count;
          RDI_ChecksumKind k = n->v[idx].checksum_kind;
          String8 val = n->v[idx].checksum;
          if(RDI_ChecksumKind_NULL < k && k < RDI_ChecksumKind_COUNT && val.size != 0)
          {
            src_map_layout->lane_chunk_src_file_checksum_counts[k][slot_idx] += 1;
          }
        }
        chunk_idx += 1;
      }
    }
    lane_sync();
    
    // rjf: layout
    if(lane_idx() == 0)
    {
      U64 chunk_idx = 0;
      U64 num_layout_off = 0;
      U64 voff_layout_off = 0;
      U64 map_layout_off = 1;
      U64 checksum_layout_offs[RDI_ChecksumKind_COUNT] = {0};
      for EachNode(n, RDIM_SrcFileChunkNode, params->src_files.first)
      {
        for EachIndex(l_idx, lane_count())
        {
          U64 slot_idx = l_idx*params->src_files.chunk_count + chunk_idx;
          src_map_layout->lane_chunk_src_file_num_offs[slot_idx] = num_layout_off;
          src_map_layout->lane_chunk_src_file_voff_offs[slot_idx] = voff_layout_off;
          src_map_layout->lane_chunk_src_file_map_offs[slot_idx] = map_layout_off;
          num_layout_off += src_map_layout->lane_chunk_src_file_num_counts[slot_idx];
          voff_layout_off += src_map_layout->lane_chunk_src_file_voff_counts[slot_idx];
          map_layout_off += src_map_layout->lane_chunk_src_file_map_counts[slot_idx];
          for EachEnumVal(RDI_ChecksumKind, k)
          {
            src_map_layout->lane_chunk_src_file_checksum_offs[k][slot_idx] = checksum_layout_offs[k];
            checksum_layout_offs[k] += src_map_layout->lane_chunk_src_file_checksum_counts[k][slot_idx];
          }
        }
        chunk_idx += 1;
      }
      src_map_layout->total_src_map_line_count = num_layout_off;
      src_map_layout->total_src_map_voff_count = voff_layout_off;
      for EachEnumVal(RDI_ChecksumKind, k)
      {
        src_map_layout->total_checksum_counts[k] = checksum_layout_offs[k];
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake src files
  //
  RDIM_SrcFileBakeResult *baked_src_files = 0;
  ProfScope("bake src files")
  {
    //- rjf: set up
    if(lane_idx() == 0)
    {
      baked_src_files = push_array(scratch.arena, RDIM_SrcFileBakeResult, 1);
      baked_src_files->source_files_count = params->src_files.total_count+1;
      baked_src_files->source_files = push_array(arena, RDI_SourceFile, baked_src_files->source_files_count);
      baked_src_files->source_line_maps_count = params->src_files.source_line_map_count+1;
      baked_src_files->source_line_maps = push_array(arena, RDI_SourceLineMap, baked_src_files->source_line_maps_count);
      baked_src_files->source_line_map_nums_count = src_map_layout->total_src_map_line_count;
      baked_src_files->source_line_map_nums = push_array(arena, RDI_U32, baked_src_files->source_line_map_nums_count);
      baked_src_files->source_line_map_rngs_count = src_map_layout->total_src_map_line_count + baked_src_files->source_line_maps_count;
      baked_src_files->source_line_map_rngs = push_array(arena, RDI_U32, baked_src_files->source_line_map_rngs_count);
      baked_src_files->source_line_map_voffs_count = src_map_layout->total_src_map_voff_count;
      baked_src_files->source_line_map_voffs = push_array(arena, RDI_U64, baked_src_files->source_line_map_voffs_count);
    }
    lane_sync_u64(&baked_src_files, 0);
    
    //- rjf: bake
    U64 chunk_idx = 0;
    for EachNode(n, RDIM_SrcFileChunkNode, params->src_files.first)
    {
      Rng1U64 range = lane_range(n->count);
      U64 slot_idx = lane_idx()*params->src_files.chunk_count + chunk_idx;
      U64 dst_num_off  = src_map_layout->lane_chunk_src_file_num_offs[slot_idx];
      U64 dst_map_off  = src_map_layout->lane_chunk_src_file_map_offs[slot_idx];
      U64 dst_voff_off = src_map_layout->lane_chunk_src_file_voff_offs[slot_idx];
      U64 dst_rng_off  = dst_num_off + dst_map_off;
      U64 dst_checksums_off[RDI_ChecksumKind_COUNT] = {0};
      for EachEnumVal(RDI_ChecksumKind, k)
      {
        dst_checksums_off[k] = 1 + src_map_layout->lane_chunk_src_file_checksum_offs[k][slot_idx];
      }
      for EachInRange(idx, range)
      {
        RDIM_BakeSrcLineMap *map = &bake_src_line_maps[n->base_idx + idx];
        RDIM_SortKey *sorted_map_keys = bake_src_line_map_keys[n->base_idx + idx];
        RDIM_SrcFile *src = &n->v[idx];
        RDI_SourceFile *dst = &baked_src_files->source_files[n->base_idx + idx + 1];
        RDI_SourceLineMap *dst_map = &baked_src_files->source_line_maps[dst_map_off];
        RDI_U32 *dst_nums  = &baked_src_files->source_line_map_nums[dst_num_off];
        RDI_U32 *dst_rngs  = &baked_src_files->source_line_map_rngs[dst_rng_off];
        RDI_U64 *dst_voffs = &baked_src_files->source_line_map_voffs[dst_voff_off];
        
        //- rjf: fill file info
        Temp scratch2 = scratch_begin(&scratch.arena, 1);
        String8 normalized_path = rdim_normalize_path_str8(scratch2.arena, src->path);
        B32 has_checksum = (RDI_ChecksumKind_NULL < src->checksum_kind && src->checksum_kind < RDI_ChecksumKind_COUNT && src->checksum.size != 0);
        dst->file_path_node_idx = rdim_bake_path_node_idx_from_string(path_tree, src->path);
        dst->normal_full_path_string_idx = rdim_bake_idx_from_string(bake_strings, normalized_path);
        dst->source_line_map_idx = src->total_line_count ? dst_map_off : 0;
        dst->checksum_kind = src->checksum_kind;
        dst->checksum_idx  = has_checksum ? dst_checksums_off[dst->checksum_kind] : 0;
        scratch_end(scratch2);
        
        //- rjf: advance checksum offset for this kind
        if(has_checksum)
        {
          dst_checksums_off[dst->checksum_kind] += 1;
        }
        
        //- rjf: fill map info
        if(src->total_line_count != 0)
        {
          dst_map->line_count = (RDI_U32)map->line_count; // TODO(rjf): @u64_to_u32
          dst_map->voff_count = (RDI_U32)map->voff_range_count; // TODO(rjf): @u64_to_u32
          dst_map->line_map_nums_base_idx = (RDI_U32)dst_num_off; // TODO(rjf): @u64_to_u32
          dst_map->line_map_range_base_idx = (RDI_U32)dst_rng_off; // TODO(rjf): @u64_to_u32
          dst_map->line_map_voff_base_idx = (RDI_U32)dst_voff_off; // TODO(rjf): @u64_to_u32
          dst_map_off += 1;
        }
        
        //- rjf: fill nums/ranges/voffs info
        if(src->total_line_count != 0)
        {
          U64 *dst_voff_ptr = dst_voffs;
          for EachIndex(line_num_idx, map->line_count)
          {
            dst_nums[line_num_idx] = (RDI_U32)sorted_map_keys[line_num_idx].key; // TODO(rjf): @u64_to_u32
            dst_rngs[line_num_idx] = (RDI_U32)(dst_voff_ptr - dst_voffs); // TODO(rjf): @u64_to_u32
            RDIM_BakeSrcLineMapNode *node = (RDIM_BakeSrcLineMapNode *)sorted_map_keys[line_num_idx].val;
            for EachNode(rng_n, RDIM_Rng1U64Node, node->voff_ranges.first)
            {
              dst_voff_ptr[0] = rng_n->v.min;
              dst_voff_ptr += 1;
            }
          }
          dst_rngs[map->line_count] = (RDI_U32)map->voff_range_count; // TODO(rjf): @u64_to_u32
          dst_num_off += map->line_count;
          dst_rng_off += map->line_count+1;
          dst_voff_off += map->voff_range_count;
        }
      }
      chunk_idx += 1;
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake checksums
  //
  RDIM_ChecksumBakeResult *baked_checksums = 0;
  ProfScope("bake checksums")
  {
    // rjf: allocate
    if(lane_idx() == 0)
    {
      baked_checksums = push_array(scratch.arena, RDIM_ChecksumBakeResult, 1);
      baked_checksums->md5s_count = src_map_layout->total_checksum_counts[RDI_ChecksumKind_MD5] + 1;
      baked_checksums->sha1s_count = src_map_layout->total_checksum_counts[RDI_ChecksumKind_SHA1] + 1;
      baked_checksums->sha256s_count = src_map_layout->total_checksum_counts[RDI_ChecksumKind_SHA256] + 1;
      baked_checksums->timestamps_count = src_map_layout->total_checksum_counts[RDI_ChecksumKind_Timestamp] + 1;
      baked_checksums->md5s = push_array(arena, RDI_MD5, baked_checksums->md5s_count);
      baked_checksums->sha1s = push_array(arena, RDI_SHA1, baked_checksums->sha1s_count);
      baked_checksums->sha256s = push_array(arena, RDI_SHA256, baked_checksums->sha256s_count);
      baked_checksums->timestamps = push_array(arena, RDI_U64, baked_checksums->timestamps_count);
    }
    lane_sync_u64(&baked_checksums, 0);
    
    // rjf: fill
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_SrcFileChunkNode, params->src_files.first)
      {
        Rng1U64 range = lane_range(n->count);
        U64 slot_idx = lane_idx()*params->src_files.chunk_count + chunk_idx;
        U64 dst_offs[RDI_ChecksumKind_COUNT] = {0};
        for EachEnumVal(RDI_ChecksumKind, k)
        {
          dst_offs[k] = 1 + src_map_layout->lane_chunk_src_file_checksum_offs[k][slot_idx];
        }
        for EachInRange(n_idx, range)
        {
          RDI_ChecksumKind k = n->v[n_idx].checksum_kind;
          String8 val = n->v[n_idx].checksum;
          if(RDI_ChecksumKind_NULL < k && k < RDI_ChecksumKind_COUNT && val.size != 0)
          {
            switch((RDI_ChecksumKindEnum)k)
            {
              case RDI_ChecksumKind_NULL:
              case RDI_ChecksumKind_COUNT:
              {}break;
#define Case(name, table_name) case RDI_ChecksumKind_##name:{MemoryCopy(&baked_checksums->table_name[dst_offs[k]], val.str, Min(val.size, sizeof(baked_checksums->table_name[0])));}break
              Case(MD5, md5s);
              Case(SHA1, sha1s);
              Case(SHA256, sha256s);
              Case(Timestamp, timestamps);
#undef Case
            }
            dst_offs[k] += 1;
          }
        }
        chunk_idx += 1;
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake namespaces
  //
  RDIM_NamespaceBakeResult *baked_namespaces = 0;
  ProfScope("bake namespaces")
  {
    // rjf: allocate
    if(lane_idx() == 0)
    {
      baked_namespaces = push_array(scratch.arena, RDIM_NamespaceBakeResult, 1);
      baked_namespaces->namespaces_count = params->namespaces.total_count + 1;
      baked_namespaces->namespaces = push_array(arena, RDI_Namespace, baked_namespaces->namespaces_count);
    }
    lane_sync_u64(&baked_namespaces, 0);
    
    // rjf: fill
    for EachNode(n, RDIM_NamespaceChunkNode, params->namespaces.first)
    {
      Rng1U64 range = lane_range(n->count);
      for EachInRange(n_idx, range)
      {
        U64 dst_idx = n->base_idx + n_idx + 1;
        RDIM_Namespace *src = &n->v[n_idx];
        RDI_Namespace *dst = &baked_namespaces->namespaces[dst_idx];
        dst->name_string_idx = rdim_bake_idx_from_string(bake_strings, src->name);
        if(src->parent_namespace != 0)
        {
          dst->container_flags |= RDI_ContainerKind_Namespace;
          dst->container_idx = rdim_idx_from_namespace(src->parent_namespace);
        }
        else if(src->parent_scope != 0)
        {
          dst->container_flags |= RDI_ContainerKind_Scope;
          dst->container_idx = rdim_idx_from_scope(src->parent_scope);
        }
        else if(src->parent_udt != 0)
        {
          dst->container_flags |= RDI_ContainerKind_Type;
          dst->container_idx = rdim_idx_from_udt(src->parent_udt);
        }
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage compute lane UDT member/enum-val layouts
  //
  typedef struct MemberLayout MemberLayout;
  struct MemberLayout
  {
    RDI_U64 *member_chunk_lane_counts; // [lane_count * udt_chunk_count]
    RDI_U64 *member_chunk_lane_offs; // [lane_count * udt_chunk_count]
    RDI_U64 *enum_val_chunk_lane_counts; // [lane_count * udt_chunk_count]
    RDI_U64 *enum_val_chunk_lane_offs; // [lane_count * udt_chunk_count]
  };
  MemberLayout *mem_layout = 0;
  ProfScope("compute lane UDT member/enum-val layouts")
  {
    // rjf: allocate
    if(lane_idx() == 0)
    {
      mem_layout = push_array(scratch.arena, MemberLayout, 1);
      mem_layout->member_chunk_lane_counts = push_array(arena, U64, lane_count() * params->udts.chunk_count);
      mem_layout->member_chunk_lane_offs = push_array(arena, U64, lane_count() * params->udts.chunk_count);
      mem_layout->enum_val_chunk_lane_counts = push_array(arena, U64, lane_count() * params->udts.chunk_count);
      mem_layout->enum_val_chunk_lane_offs = push_array(arena, U64, lane_count() * params->udts.chunk_count);
    }
    lane_sync_u64(&mem_layout, 0);
    
    // rjf: count
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_UDTChunkNode, params->udts.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(idx, range)
        {
          U64 slot_idx = lane_idx()*params->udts.chunk_count + chunk_idx;
          mem_layout->member_chunk_lane_counts[slot_idx] += n->v[idx].member_count;
          mem_layout->enum_val_chunk_lane_counts[slot_idx] += n->v[idx].enum_val_count;
        }
        chunk_idx += 1;
      }
    }
    lane_sync();
    
    // rjf: layout
    if(lane_idx() == 0)
    {
      U64 member_layout_off = 1;
      U64 enum_val_layout_off = 1;
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_UDTChunkNode, params->udts.first)
      {
        for EachIndex(l_idx, lane_count())
        {
          U64 slot_idx = l_idx*params->udts.chunk_count + chunk_idx;
          mem_layout->member_chunk_lane_offs[slot_idx] = member_layout_off;
          mem_layout->enum_val_chunk_lane_offs[slot_idx] = enum_val_layout_off;
          member_layout_off += mem_layout->member_chunk_lane_counts[slot_idx];
          enum_val_layout_off += mem_layout->enum_val_chunk_lane_counts[slot_idx];
        }
        chunk_idx += 1;
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake UDTs
  //
  RDIM_UDTBakeResult *baked_udts = 0;
  ProfScope("bake UDTs")
  {
    //- rjf: set up
    ProfScope("set up")
    {
      if(lane_idx() == 0)
      {
        baked_udts = push_array(scratch.arena, RDIM_UDTBakeResult, 1);
      }
      lane_sync_u64(&baked_udts, 0);
      if(lane_idx() == lane_from_task_idx(0))
      {
        baked_udts->udts_count = params->udts.total_count+1;
        baked_udts->udts = push_array(arena, RDI_UDT, baked_udts->udts_count);
      }
      if(lane_idx() == lane_from_task_idx(1))
      {
        baked_udts->members_count = params->udts.total_member_count+1;
        baked_udts->members = push_array(arena, RDI_Member, baked_udts->members_count);
      }
      if(lane_idx() == lane_from_task_idx(2))
      {
        baked_udts->enum_members_count = params->udts.total_enum_val_count+1;
        baked_udts->enum_members = push_array(arena, RDI_EnumMember, baked_udts->enum_members_count);
      }
    }
    lane_sync();
    
    //- rjf: bake UDTs
    ProfScope("bake UDTs")
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_UDTChunkNode, params->udts.first)
      {
        Rng1U64 range = lane_range(n->count);
        U64 layout_slot_idx = lane_idx()*params->udts.chunk_count + chunk_idx;
        U64 member_layout_off = mem_layout->member_chunk_lane_offs[layout_slot_idx];
        U64 enum_val_layout_off = mem_layout->enum_val_chunk_lane_offs[layout_slot_idx];
        for EachInRange(n_idx, range)
        {
          RDIM_UDT *src_udt = &n->v[n_idx];
          RDI_UDT *dst_udt = &baked_udts->udts[n->base_idx + n_idx + 1];
          
          //- rjf: fill basics
          dst_udt->self_type_idx = (RDI_U32)rdim_idx_from_type(src_udt->self_type); // TODO(rjf): @u64_to_u32
          dst_udt->file_idx = (RDI_U32)rdim_idx_from_src_file(src_udt->src_file); // TODO(rjf): @u64_to_u32
          dst_udt->line = src_udt->line;
          dst_udt->col  = src_udt->col;
          
          //- rjf: fill member info
          if(src_udt->first_member != 0)
          {
            U64 member_off_first = member_layout_off;
            for EachNode(src_member, RDIM_UDTMember, src_udt->first_member)
            {
              RDI_Member *dst_member = &baked_udts->members[member_layout_off];
              dst_member->kind            = src_member->kind;
              dst_member->name_string_idx = rdim_bake_idx_from_string(bake_strings, src_member->name);
              dst_member->type_idx        = (RDI_U32)rdim_idx_from_type(src_member->type); // TODO(rjf): @u64_to_u32
              dst_member->off             = src_member->off;
              member_layout_off += 1;
            }
            U64 member_off_opl = member_layout_off;
            dst_udt->member_first = (RDI_U32)member_off_first; // TODO(rjf): @u64_to_u32
            dst_udt->member_count = (RDI_U32)(member_off_opl - member_off_first); // TODO(rjf): @u64_to_u32
          }
          
          //- rjf: fill enum val info
          else if(src_udt->first_enum_val != 0)
          {
            U64 enum_val_off_first = enum_val_layout_off;
            for EachNode(src_enum_val, RDIM_UDTEnumVal, src_udt->first_enum_val)
            {
              RDI_EnumMember *dst_member = &baked_udts->enum_members[enum_val_layout_off];
              dst_member->name_string_idx = rdim_bake_idx_from_string(bake_strings, src_enum_val->name);
              dst_member->val             = src_enum_val->val;
              enum_val_layout_off += 1;
            }
            U64 enum_val_off_opl = enum_val_layout_off;
            dst_udt->flags |= RDI_UDTFlag_EnumMembers;
            dst_udt->member_first = (RDI_U32)enum_val_off_first; // TODO(rjf): @u64_to_u32
            dst_udt->member_count = (RDI_U32)(enum_val_off_opl - enum_val_off_first); // TODO(rjf): @u64_to_u32
          }
        }
        chunk_idx += 1;
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage compute lane location block layout
  //
  typedef struct LocLayout LocLayout;
  struct LocLayout
  {
    RDI_U64 *location_case_chunk_lane_counts; // [lane_count * (scope_chunk_count + procedure_chunk_count)
    RDI_U64 *location_case_chunk_lane_offs; // [lane_count * (scope_chunk_count + procedure_chunk_count)
    RDI_U64 total_location_case_count;
  };
  LocLayout *loc_layout = 0;
  U64 total_location_case_chunk_count = (params->scopes.chunk_count + params->procedures.chunk_count);
  ProfScope("compute lane location block layout")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      loc_layout = push_array(scratch.arena, LocLayout, 1);
      loc_layout->location_case_chunk_lane_counts = push_array(arena, RDI_U64, lane_count() * total_location_case_chunk_count);
      loc_layout->location_case_chunk_lane_offs = push_array(arena, RDI_U64, lane_count() * total_location_case_chunk_count);
    }
    lane_sync_u64(&loc_layout, 0);
    
    // rjf: per-chunk-lane count of location cases
    {
      // rjf: count location cases in scopes
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
      {
        U64 slot_idx = lane_idx() * total_location_case_chunk_count + chunk_idx;
        Rng1U64 range = lane_range(n->count);
        for EachInRange(idx, range)
        {
          for EachNode(local, RDIM_Local, n->v[idx].first_local)
          {
            loc_layout->location_case_chunk_lane_counts[slot_idx] += local->location_cases.count;
          }
        }
        chunk_idx += 1;
      }
      
      // rjf: count location cases in procedures
      for EachNode(n, RDIM_SymbolChunkNode, params->procedures.first)
      {
        U64 slot_idx = lane_idx() * total_location_case_chunk_count + chunk_idx;
        Rng1U64 range = lane_range(n->count);
        for EachInRange(idx, range)
        {
          loc_layout->location_case_chunk_lane_counts[slot_idx] += n->v[idx].location_cases.count;
        }
        chunk_idx += 1;
      }
    }
    lane_sync();
    
    // rjf: lay out location case offsets
    if(lane_idx() == 0)
    {
      U64 chunk_idx = 0;
      U64 location_case_layout_off = 1;
      for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
      {
        for EachIndex(l_idx, lane_count())
        {
          U64 slot_idx = l_idx * total_location_case_chunk_count + chunk_idx;
          loc_layout->location_case_chunk_lane_offs[slot_idx] = location_case_layout_off;
          location_case_layout_off += loc_layout->location_case_chunk_lane_counts[slot_idx];
        }
        chunk_idx += 1;
      }
      for EachNode(n, RDIM_SymbolChunkNode, params->procedures.first)
      {
        for EachIndex(l_idx, lane_count())
        {
          U64 slot_idx = l_idx * total_location_case_chunk_count + chunk_idx;
          loc_layout->location_case_chunk_lane_offs[slot_idx] = location_case_layout_off;
          location_case_layout_off += loc_layout->location_case_chunk_lane_counts[slot_idx];
        }
        chunk_idx += 1;
      }
      loc_layout->total_location_case_count = location_case_layout_off;
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake location blocks
  //
  RDIM_LocationBlockBakeResult *baked_location_blocks = 0;
  ProfScope("bake location blocks")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      baked_location_blocks = push_array(scratch.arena, RDIM_LocationBlockBakeResult, 1);
      baked_location_blocks->location_blocks_count = loc_layout->total_location_case_count;
      baked_location_blocks->location_blocks = push_array(arena, RDI_LocationBlock, baked_location_blocks->location_blocks_count);
    }
    lane_sync_u64(&baked_location_blocks, 0);
    
    // rjf: wide fill from scopes
    U64 chunk_idx = 0;
    ProfScope("wide fill from scopes")
    {
      for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
      {
        U64 layout_slot_idx = lane_idx() * total_location_case_chunk_count + chunk_idx;
        U64 layout_off = loc_layout->location_case_chunk_lane_offs[layout_slot_idx];
        Rng1U64 range = lane_range(n->count);
        for EachInRange(idx, range)
        {
          for EachNode(local, RDIM_Local, n->v[idx].first_local)
          {
            for EachNode(src, RDIM_LocationCase, local->location_cases.first)
            {
              RDI_LocationBlock *dst = &baked_location_blocks->location_blocks[layout_off];
              dst->scope_off_first   = (RDI_U32)src->voff_range.min; // TODO(rjf): @u64_to_u32
              dst->scope_off_opl     = (RDI_U32)src->voff_range.max; // TODO(rjf): @u64_to_u32
              dst->location_data_off = (RDI_U32)rdim_off_from_location(src->location); // TODO(rjf): @u64_to_u32
              layout_off += 1;
            }
          }
        }
        chunk_idx += 1;
      }
    }
    
    // rjf: wide fill from procedures
    ProfScope("wide fill from procedures")
    {
      for EachNode(n, RDIM_SymbolChunkNode, params->procedures.first)
      {
        U64 layout_slot_idx = lane_idx() * total_location_case_chunk_count + chunk_idx;
        U64 layout_off = loc_layout->location_case_chunk_lane_offs[layout_slot_idx];
        Rng1U64 range = lane_range(n->count);
        for EachInRange(idx, range)
        {
          for EachNode(src, RDIM_LocationCase, n->v[idx].location_cases.first)
          {
            RDI_LocationBlock *dst = &baked_location_blocks->location_blocks[layout_off];
            dst->scope_off_first = (RDI_U32)src->voff_range.min; // TODO(rjf): @u64_to_u32
            dst->scope_off_opl   = (RDI_U32)src->voff_range.max; // TODO(rjf): @u64_to_u32
            dst->location_data_off = (RDI_U32)rdim_off_from_location(src->location); // TODO(rjf): @u64_to_u32
            layout_off += 1;
          }
        }
        chunk_idx += 1;
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake locations
  //
  RDIM_LocationBakeResult *baked_locations = 0;
  ProfScope("bake locations")
  {
    if(lane_idx() == 0)
    {
      baked_locations = push_array(scratch.arena, RDIM_LocationBakeResult, 1);
      baked_locations->location_data_size = params->locations.total_encoded_size+1;
      baked_locations->location_data = push_array(arena, RDI_U8, baked_locations->location_data_size);
    }
    lane_sync_u64(&baked_locations, 0);
    for EachNode(n, RDIM_LocationChunkNode, params->locations.first)
    {
      Rng1U64 range = lane_range(n->count);
      for EachInRange(n_idx, range)
      {
        RDIM_Location *loc = &n->v[n_idx];
        RDI_U8 *dst = &baked_locations->location_data[n->base_encoding_off + loc->relative_encoding_off + 1];
        switch((RDI_LocationKindEnum)loc->info.kind)
        {
          case RDI_LocationKind_NULL:{}break;
          case RDI_LocationKind_AddrBytecodeStream:
          case RDI_LocationKind_ValBytecodeStream:
          {
            MemoryCopy(dst+0, &loc->info.kind, sizeof(loc->info.kind));
            RDI_U64 write_off = sizeof(loc->info.kind);
            for EachNode(op_node, RDIM_EvalBytecodeOp, loc->info.bytecode.first_op)
            {
              MemoryCopy(dst + write_off, &op_node->op, 1);
              write_off += 1;
              MemoryCopy(dst + write_off, &op_node->p, op_node->p_size);
              write_off += op_node->p_size;
            }
            dst[write_off] = 0;
          }break;
          case RDI_LocationKind_AddrRegPlusU16:
          case RDI_LocationKind_AddrAddrRegPlusU16:
          {
            RDI_LocationRegPlusU16 baked = {loc->info.kind, loc->info.reg_code, loc->info.offset};
            MemoryCopy(dst, &baked, sizeof(baked));
          }break;
          case RDI_LocationKind_ValReg:
          {
            RDI_LocationReg baked = {loc->info.kind, loc->info.reg_code};
            MemoryCopy(dst, &baked, sizeof(baked));
          }break;
        }
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage compute layout for scope sub-lists (locals / voffs)
  //
  typedef struct ScopeLayout ScopeLayout;
  struct ScopeLayout
  {
    RDI_U64 *scope_local_chunk_lane_counts; // [lane_count * scope_chunk_count]
    RDI_U64 *scope_local_chunk_lane_offs; // [lane_count * scope_chunk_count]
    RDI_U64 *scope_voff_chunk_lane_counts; // [lane_count * scope_chunk_count]
    RDI_U64 *scope_voff_chunk_lane_offs; // [lane_count * scope_chunk_count]
  };
  ScopeLayout *scope_layout = 0;
  ProfScope("compute layout for scope sub-lists (locals / voffs)")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      scope_layout = push_array(scratch.arena, ScopeLayout, 1);
      scope_layout->scope_local_chunk_lane_counts = push_array(arena, RDI_U64, lane_count() * params->scopes.chunk_count);
      scope_layout->scope_local_chunk_lane_offs = push_array(arena, RDI_U64, lane_count() * params->scopes.chunk_count);
      scope_layout->scope_voff_chunk_lane_counts = push_array(arena, RDI_U64, lane_count() * params->scopes.chunk_count);
      scope_layout->scope_voff_chunk_lane_offs = push_array(arena, RDI_U64, lane_count() * params->scopes.chunk_count);
    }
    lane_sync_u64(&scope_layout, 0);
    
    // rjf: count per-lane-chunk
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
      {
        U64 num_locals_in_this_lane_and_node = 0;
        U64 num_voffs_in_this_lane_and_node = 0;
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          num_locals_in_this_lane_and_node += n->v[n_idx].local_count;
          num_voffs_in_this_lane_and_node += n->v[n_idx].voff_ranges.count*2;
        }
        scope_layout->scope_local_chunk_lane_counts[lane_idx()*params->scopes.chunk_count + chunk_idx] = num_locals_in_this_lane_and_node;
        scope_layout->scope_voff_chunk_lane_counts[lane_idx()*params->scopes.chunk_count + chunk_idx] = num_voffs_in_this_lane_and_node;
        chunk_idx += 1;
      }
    }
    lane_sync();
    
    // rjf: lay out each lane's range
    if(lane_idx() == 0)
    {
      U64 local_layout_off = 1;
      U64 voff_layout_off = 1;
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
      {
        for EachIndex(l_idx, lane_count())
        {
          U64 slot_idx = l_idx*params->scopes.chunk_count + chunk_idx;
          scope_layout->scope_local_chunk_lane_offs[slot_idx] = local_layout_off;
          scope_layout->scope_voff_chunk_lane_offs[slot_idx] = voff_layout_off;
          local_layout_off += scope_layout->scope_local_chunk_lane_counts[slot_idx];
          voff_layout_off += scope_layout->scope_voff_chunk_lane_counts[slot_idx];
        }
        chunk_idx += 1;
      }
    }
    lane_sync();
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake scopes
  //
  RDIM_ScopeBakeResult *baked_scopes = 0;
  ProfScope("bake scopes")
  {
    //- rjf: setup outputs
    if(lane_idx() == 0)
    {
      baked_scopes = push_array(scratch.arena, RDIM_ScopeBakeResult, 1);
    }
    lane_sync_u64(&baked_scopes, 0);
    if(lane_idx() == lane_from_task_idx(0))
    {
      baked_scopes->scopes_count = params->scopes.total_count+1;
      baked_scopes->scopes = push_array(arena, RDI_Scope, baked_scopes->scopes_count);
    }
    if(lane_idx() == lane_from_task_idx(1))
    {
      baked_scopes->scope_voffs_count = params->scopes.scope_voff_count+1;
      baked_scopes->scope_voffs = push_array(arena, RDI_U64, baked_scopes->scope_voffs_count);
    }
    if(lane_idx() == lane_from_task_idx(2))
    {
      baked_scopes->locals_count = params->scopes.local_count+1;
      baked_scopes->locals = push_array(arena, RDI_Local, baked_scopes->locals_count);
    }
    lane_sync();
    
    //- rjf: wide fill
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
      {
        Rng1U64 range = lane_range(n->count);
        U64 scope_chunk_lane_slot_idx = lane_idx()*params->scopes.chunk_count + chunk_idx;
        U64 chunk_local_off = scope_layout->scope_local_chunk_lane_offs[scope_chunk_lane_slot_idx];
        U64 chunk_voff_off = scope_layout->scope_voff_chunk_lane_offs[scope_chunk_lane_slot_idx];
        U64 location_block_chunk_lane_slot_idx = lane_idx() * total_location_case_chunk_count + chunk_idx;
        U64 chunk_location_block_off = loc_layout->location_case_chunk_lane_offs[location_block_chunk_lane_slot_idx];
        for EachInRange(n_idx, range)
        {
          U64 dst_idx = 1 + n->base_idx + n_idx;
          RDIM_Scope *src_scope = &n->v[n_idx];
          RDI_Scope *dst_scope = &baked_scopes->scopes[dst_idx];
          
          //- rjf: fill voff ranges
          U64 voff_idx_first = chunk_voff_off;
          for EachNode(rng_n, RDIM_Rng1U64Node, src_scope->voff_ranges.first)
          {
            baked_scopes->scope_voffs[chunk_voff_off+0] = rng_n->v.min;
            baked_scopes->scope_voffs[chunk_voff_off+1] = rng_n->v.max;
            chunk_voff_off += 2;
          }
          U64 voff_idx_opl = chunk_voff_off;
          
          //- rjf: fill locals
          U64 local_idx_first = chunk_local_off;
          for EachNode(src_local, RDIM_Local, src_scope->first_local)
          {
            RDI_Local *dst_local = &baked_scopes->locals[chunk_local_off];
            dst_local->kind            = src_local->kind;
            dst_local->name_string_idx = rdim_bake_idx_from_string(bake_strings, src_local->name);
            dst_local->type_idx        = (RDI_U32)rdim_idx_from_type(src_local->type); // TODO(rjf): @u64_to_u32
            if(src_local->location_cases.count != 0)
            {
              dst_local->location_first  = chunk_location_block_off;
              dst_local->location_opl    = chunk_location_block_off + src_local->location_cases.count;
              chunk_location_block_off += src_local->location_cases.count;
            }
            chunk_local_off += 1;
          }
          U64 local_idx_opl = chunk_local_off;
          
          //- rjf: fill scope
          dst_scope->proc_idx               = (RDI_U32)rdim_idx_from_symbol(src_scope->symbol); // TODO(rjf): @u64_to_u32
          dst_scope->parent_scope_idx       = (RDI_U32)rdim_idx_from_scope(src_scope->parent_scope); // TODO(rjf): @u64_to_u32
          dst_scope->first_child_scope_idx  = (RDI_U32)rdim_idx_from_scope(src_scope->first_child); // TODO(rjf): @u64_to_u32
          dst_scope->next_sibling_scope_idx = (RDI_U32)rdim_idx_from_scope(src_scope->next_sibling); // TODO(rjf): @u64_to_u32
          dst_scope->voff_range_first       = (RDI_U32)voff_idx_first;                    // TODO(rjf): @u64_to_u32
          dst_scope->voff_range_opl         = (RDI_U32)voff_idx_opl;                      // TODO(rjf): @u64_to_u32
          dst_scope->local_first            = (RDI_U32)local_idx_first;                   // TODO(rjf): @u64_to_u32
          dst_scope->local_count            = (RDI_U32)(local_idx_opl - local_idx_first); // TODO(rjf): @u64_to_u32
          dst_scope->inline_site_idx        = (RDI_U32)rdim_idx_from_inline_site(src_scope->inline_site); // TODO(rjf): @u64_to_u32
        }
        chunk_idx += 1;
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake procedures
  //
  RDIM_ProcedureBakeResult *baked_procedures = 0;
  ProfScope("bake procedures")
  {
    if(lane_idx() == 0)
    {
      baked_procedures = push_array(scratch.arena, RDIM_ProcedureBakeResult, 1);
      baked_procedures->procedures_count = params->procedures.total_count+1;
      baked_procedures->procedures = push_array(arena, RDI_Procedure, baked_procedures->procedures_count);
    }
    lane_sync_u64(&baked_procedures, 0);
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_SymbolChunkNode, params->procedures.first)
      {
        U64 location_block_layout_slot_idx = lane_idx()*total_location_case_chunk_count + params->scopes.chunk_count + chunk_idx;
        U64 location_block_off = loc_layout->location_case_chunk_lane_offs[location_block_layout_slot_idx];
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Symbol *src = &n->v[n_idx];
          RDI_Procedure *dst = &baked_procedures->procedures[n->base_idx + n_idx + 1];
          dst->name_string_idx      = rdim_bake_idx_from_string(bake_strings, src->name);
          dst->link_name_string_idx = rdim_bake_idx_from_string(bake_strings, src->link_name);
          if(src->is_extern)
          {
            dst->link_flags |= RDI_LinkFlag_External;
          }
          if(src->container_type != 0)
          {
            dst->link_flags |= RDI_LinkFlag_TypeScoped;
            dst->container_idx = src->container_type ? (RDI_U32)rdim_idx_from_udt(src->container_type->udt) : 0; // TODO(rjf): @u64_to_u32
          }
          else if(src->container_scope != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_scope->symbol); // TODO(rjf): @u64_to_u32
          }
          dst->type_idx                  = (RDI_U32)rdim_idx_from_type(src->type); // TODO(rjf): @u64_to_u32
          dst->root_scope_idx            = (RDI_U32)rdim_idx_from_scope(src->root_scope); // TODO(rjf): @u64_to_u32
          if(src->location_cases.count != 0)
          {
            dst->frame_base_location_first = location_block_off;
            dst->frame_base_location_opl   = location_block_off + src->location_cases.count;
            location_block_off += src->location_cases.count;
          }
        }
        chunk_idx += 1;
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage compute layout for constant data
  //
  typedef struct ConstantLayout ConstantLayout;
  struct ConstantLayout
  {
    RDI_U64 *constant_data_chunk_lane_counts; // [lane_count * constant_chunk_count]
    RDI_U64 *constant_data_chunk_lane_offs; // [lane_count * constant_chunk_count]
  };
  ConstantLayout *constant_layout = 0;
  ProfScope("compute layout for constant data")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      constant_layout = push_array(scratch.arena, ConstantLayout, 1);
      constant_layout->constant_data_chunk_lane_counts = push_array(arena, U64, lane_count() * params->constants.chunk_count);
      constant_layout->constant_data_chunk_lane_offs = push_array(arena, U64, lane_count() * params->constants.chunk_count);
    }
    lane_sync_u64(&constant_layout, 0);
    
    // rjf: count
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_SymbolChunkNode, params->constants.first)
      {
        U64 slot_idx = lane_idx()*params->constants.chunk_count + chunk_idx;
        Rng1U64 range = lane_range(n->count);
        for EachInRange(idx, range)
        {
          constant_layout->constant_data_chunk_lane_counts[slot_idx] += n->v[idx].value_data.size;
        }
        chunk_idx += 1;
      }
    }
    lane_sync();
    
    // rjf: layout
    if(lane_idx() == 0)
    {
      U64 chunk_idx = 0;
      U64 layout_off = 0;
      for EachNode(n, RDIM_SymbolChunkNode, params->constants.first)
      {
        for EachIndex(l_idx, lane_count())
        {
          U64 slot_idx = l_idx*params->constants.chunk_count + chunk_idx;
          constant_layout->constant_data_chunk_lane_offs[slot_idx] = layout_off;
          layout_off += constant_layout->constant_data_chunk_lane_counts[slot_idx];
        }
        chunk_idx += 1;
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake constants
  //
  RDIM_ConstantsBakeResult *baked_constants = 0;
  ProfScope("bake constants")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      baked_constants = push_array(scratch.arena, RDIM_ConstantsBakeResult, 1);
    }
    lane_sync_u64(&baked_constants, 0);
    if(lane_idx() == lane_from_task_idx(0))
    {
      baked_constants->constant_values_count = params->constants.total_count+1;
      baked_constants->constant_values = push_array(arena, RDI_U32, baked_constants->constant_values_count);
    }
    if(lane_idx() == lane_from_task_idx(1))
    {
      baked_constants->constant_value_data_size = params->constants.total_value_data_size;
      baked_constants->constant_value_data = push_array(arena, RDI_U8, baked_constants->constant_value_data_size);
    }
    if(lane_idx() == lane_from_task_idx(2))
    {
      baked_constants->constants_count = params->constants.total_count+1;
      baked_constants->constants = push_array(arena, RDI_Constant, baked_constants->constants_count);
    }
    lane_sync();
    
    // rjf: wide bake
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_SymbolChunkNode, params->constants.first)
      {
        U64 slot_idx = lane_idx()*params->constants.chunk_count + chunk_idx;
        U64 value_data_off = constant_layout->constant_data_chunk_lane_offs[slot_idx];
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Symbol *src = &n->v[n_idx];
          RDI_Constant *dst = &baked_constants->constants[1 + n->base_idx + n_idx];
          RDI_U32 *dst_value_off = &baked_constants->constant_values[1 + n->base_idx + n_idx];
          RDI_U8 *dst_value_data = baked_constants->constant_value_data + value_data_off;
          dst->name_string_idx    = rdim_bake_idx_from_string(bake_strings, src->name);
          dst->type_idx           = (RDI_U32)rdim_idx_from_type(src->type); // TODO(rjf): @u64_to_u32
          dst->constant_value_idx = 1 + n->base_idx + n_idx;
          dst_value_off[0] = (RDI_U32)value_data_off; // TODO(rjf): @u64_to_u32
          rdim_memcpy(dst_value_data, src->value_data.str, src->value_data.size);
          value_data_off += src->value_data.size;
        }
        chunk_idx += 1;
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake units, symbols, types, UDTs
  //
  RDIM_UnitBakeResult *baked_units = 0;
  RDIM_TypeNodeBakeResult *baked_type_nodes = 0;
  RDIM_GlobalVariableBakeResult *baked_global_variables = 0;
  RDIM_ThreadVariableBakeResult *baked_thread_variables = 0;
  RDIM_InlineSiteBakeResult *baked_inline_sites = 0;
  {
    //- rjf: setup outputs
    if(lane_idx() == lane_from_task_idx(0))
    {
      baked_units = push_array(scratch.arena, RDIM_UnitBakeResult, 1);
      baked_units->units_count = params->units.total_count+1;
      baked_units->units = push_array(arena, RDI_Unit, baked_units->units_count);
    }
    if(lane_idx() == lane_from_task_idx(1))
    {
      baked_type_nodes = push_array(scratch.arena, RDIM_TypeNodeBakeResult, 1);
      baked_type_nodes->type_nodes_count = params->types.total_count+1;
      baked_type_nodes->type_nodes = push_array(arena, RDI_TypeNode, baked_type_nodes->type_nodes_count);
    }
    if(lane_idx() == lane_from_task_idx(2))
    {
      baked_global_variables = push_array(scratch.arena, RDIM_GlobalVariableBakeResult, 1);
      baked_global_variables->global_variables_count = params->global_variables.total_count+1;
      baked_global_variables->global_variables = push_array(arena, RDI_GlobalVariable, baked_global_variables->global_variables_count);
    }
    if(lane_idx() == lane_from_task_idx(3))
    {
      baked_thread_variables = push_array(scratch.arena, RDIM_ThreadVariableBakeResult, 1);
      baked_thread_variables->thread_variables_count = params->thread_variables.total_count+1;
      baked_thread_variables->thread_variables = push_array(arena, RDI_ThreadVariable, baked_thread_variables->thread_variables_count);
    }
    if(lane_idx() == lane_from_task_idx(4))
    {
      baked_inline_sites = push_array(scratch.arena, RDIM_InlineSiteBakeResult, 1);
      baked_inline_sites->inline_sites_count = params->inline_sites.total_count+1;
      baked_inline_sites->inline_sites = push_array(arena, RDI_InlineSite, baked_inline_sites->inline_sites_count);
    }
    lane_sync_u64(&baked_units, lane_from_task_idx(0));
    lane_sync_u64(&baked_type_nodes, lane_from_task_idx(1));
    lane_sync_u64(&baked_global_variables, lane_from_task_idx(2));
    lane_sync_u64(&baked_thread_variables, lane_from_task_idx(3));
    lane_sync_u64(&baked_inline_sites, lane_from_task_idx(4));
    
    //- rjf: bake units
    ProfScope("bake units")
    {
      for EachNode(n, RDIM_UnitChunkNode, params->units.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Unit *src = &n->v[n_idx];
          RDI_Unit *dst = &baked_units->units[n->base_idx + n_idx + 1];
          dst->unit_name_string_idx     = rdim_bake_idx_from_string(bake_strings, src->unit_name);
          dst->compiler_name_string_idx = rdim_bake_idx_from_string(bake_strings, src->compiler_name);
          dst->source_file_path_node    = rdim_bake_path_node_idx_from_string(path_tree, src->source_file);
          dst->object_file_path_node    = rdim_bake_path_node_idx_from_string(path_tree, src->object_file);
          dst->archive_file_path_node   = rdim_bake_path_node_idx_from_string(path_tree, src->archive_file);
          dst->build_path_node          = rdim_bake_path_node_idx_from_string(path_tree, src->build_path);
          dst->language                 = src->language;
          dst->line_table_idx           = (RDI_U32)rdim_idx_from_line_table(src->line_table); // TODO(rjf): @u64_to_u32
        }
      }
    }
    
    //- rjf: bake type nodes
    ProfScope("bake type nodes")
    {
      for EachNode(n, RDIM_TypeChunkNode, params->types.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Type *src = &n->v[n_idx];
          RDI_TypeNode *dst = &baked_type_nodes->type_nodes[n->base_idx + n_idx + 1];
          
          //- rjf: fill shared type node info
          dst->kind      = src->kind;
          dst->flags     = (RDI_U16)src->flags; // TODO(rjf): @u32_to_u16
          dst->byte_size = src->byte_size;
          
          //- rjf: fill built-in-only type node info
          if(RDI_TypeKind_FirstBuiltIn <= dst->kind && dst->kind <= RDI_TypeKind_LastBuiltIn)
          {
            dst->built_in.name_string_idx = rdim_bake_idx_from_string(bake_strings, src->name);
          }
          
          //- rjf: fill array sizes
          else if(dst->kind == RDI_TypeKind_Array)
          {
            U64 direct_byte_size = 1;
            if(src->direct_type && src->direct_type->byte_size > 0)
            {
              direct_byte_size = src->direct_type->byte_size;
            }
            dst->constructed.direct_type_idx = (RDI_U32)rdim_idx_from_type(src->direct_type);
            dst->constructed.count           = src->byte_size / direct_byte_size;
          }
          
          //- rjf: fill constructed type node info
          else if(RDI_TypeKind_FirstConstructed <= dst->kind && dst->kind <= RDI_TypeKind_LastConstructed)
          {
            dst->constructed.direct_type_idx = (RDI_U32)rdim_idx_from_type(src->direct_type); // TODO(rjf): @u64_to_u32
            dst->constructed.count = src->count;
            if(dst->kind == RDI_TypeKind_Function || dst->kind == RDI_TypeKind_Method)
            {
              RDI_U32 param_idx_run_count = src->count;
              RDI_U32 *param_idx_run = rdim_push_array_no_zero(arena, RDI_U32, param_idx_run_count);
              for(RDI_U32 idx = 0; idx < param_idx_run_count; idx += 1)
              {
                param_idx_run[idx] = (RDI_U32)rdim_idx_from_type(src->param_types[idx]); // TODO(rjf): @u64_to_u32
              }
              dst->constructed.param_idx_run_first = rdim_bake_idx_from_idx_run(bake_idx_runs, param_idx_run, param_idx_run_count);
            }
            else if(dst->kind == RDI_TypeKind_MemberPtr)
            {
              // TODO(rjf): member pointers not currently supported.
            }
          }
          
          //- rjf: fill user-defined-type info
          else if(RDI_TypeKind_FirstUserDefined <= dst->kind && dst->kind <= RDI_TypeKind_LastUserDefined)
          {
            dst->user_defined.name_string_idx = rdim_bake_idx_from_string(bake_strings, src->name);
            dst->user_defined.udt_idx         = (RDI_U32)rdim_idx_from_udt(src->udt); // TODO(rjf): @u64_to_u32
            dst->user_defined.direct_type_idx = (RDI_U32)rdim_idx_from_type(src->direct_type); // TODO(rjf): @u64_to_u32
          }
          
          //- rjf: fill bitfield info
          else if(dst->kind == RDI_TypeKind_Bitfield)
          {
            dst->bitfield.direct_type_idx = (RDI_U32)rdim_idx_from_type(src->direct_type); // TODO(rjf): @u64_to_u32
            dst->bitfield.off  = src->off;
            dst->bitfield.size = src->count;
          }
        }
      }
    }
    
    //- rjf: bake global variables
    ProfScope("bake global variables")
    {
      for EachNode(n, RDIM_SymbolChunkNode, params->global_variables.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Symbol *src = &n->v[n_idx];
          RDI_GlobalVariable *dst = &baked_global_variables->global_variables[n->base_idx + n_idx + 1];
          dst->name_string_idx = rdim_bake_idx_from_string(bake_strings, src->name);
          dst->voff            = src->offset;
          dst->type_idx        = (RDI_U32)rdim_idx_from_type(src->type); // TODO(rjf): @u64_to_u32
          if(src->is_extern)
          {
            dst->link_flags |= RDI_LinkFlag_External;
          }
          if(src->container_type != 0)
          {
            dst->link_flags |= RDI_LinkFlag_TypeScoped;
            dst->container_idx = src->container_type ? (RDI_U32)rdim_idx_from_udt(src->container_type->udt) : 0; // TODO(rjf): @u64_to_u32
          }
          else if(src->container_scope != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_scope->symbol); // TODO(rjf): @u64_to_u32
          }
        }
      }
    }
    
    //- rjf: bake thread variables
    ProfScope("bake thread variables")
    {
      for EachNode(n, RDIM_SymbolChunkNode, params->thread_variables.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Symbol *src = &n->v[n_idx];
          RDI_ThreadVariable *dst = &baked_thread_variables->thread_variables[n->base_idx + n_idx + 1];
          dst->name_string_idx = rdim_bake_idx_from_string(bake_strings, src->name);
          dst->tls_off         = (RDI_U32)src->offset; // TODO(rjf): @u64_to_u32
          dst->type_idx        = (RDI_U32)rdim_idx_from_type(src->type);
          if(src->is_extern)
          {
            dst->link_flags |= RDI_LinkFlag_External;
          }
          if(src->container_type != 0)
          {
            dst->link_flags |= RDI_LinkFlag_TypeScoped;
            dst->container_idx = src->container_type ? (RDI_U32)rdim_idx_from_udt(src->container_type->udt) : 0; // TODO(rjf): @u64_to_u32
          }
          else if(src->container_scope != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_scope->symbol); // TODO(rjf): @u64_to_u32
          }
        }
      }
    }
    
    //- rjf: bake inline sites
    ProfScope("bake inline sites")
    {
      for EachNode(n, RDIM_InlineSiteChunkNode, params->inline_sites.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDI_InlineSite *dst = &baked_inline_sites->inline_sites[n->base_idx + n_idx + 1];
          RDIM_InlineSite *src = &n->v[n_idx];
          dst->name_string_idx   = rdim_bake_idx_from_string(bake_strings, src->name);
          dst->type_idx          = (RDI_U32)rdim_idx_from_type(src->type); // TODO(rjf): @u64_to_u32
          dst->owner_type_idx    = (RDI_U32)rdim_idx_from_type(src->owner); // TODO(rjf): @u64_to_u32
          dst->line_table_idx    = (RDI_U32)rdim_idx_from_line_table(src->line_table); // TODO(rjf): @u64_to_u32
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////
  //- rjf: gather all lists which form symbol tables
  //
  struct
  {
    RDIM_SymbolChunkList *symbols;
  }
  symbol_table_lists[] =
  {
    {&params->global_variables},
    {&params->thread_variables},
  };
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake symbols (NEW)
  //
  ProfScope("bake symbols")
  {
    ////////////////////////////
    //- rjf: count chunks
    //
    U64 chunk_count = 0;
    for EachElement(symbol_table_list_idx, symbol_table_lists)
    {
      chunk_count += symbol_table_lists[symbol_table_list_idx].symbols->chunk_count;
    }
    
    ////////////////////////////
    //- rjf: count indirected location data - bytecode, constants, set elements
    //
    U64 *lane_chunk_bytecode_data_counts = 0;
    U64 *lane_chunk_constant_data_counts = 0;
    U64 *lane_chunk_set_element_counts = 0;
    if(lane_idx() == 0)
    {
      lane_chunk_bytecode_data_counts = push_array(scratch.arena, U64, lane_count() * chunk_count);
      lane_chunk_constant_data_counts = push_array(scratch.arena, U64, lane_count() * chunk_count);
      lane_chunk_set_element_counts = push_array(scratch.arena, U64, lane_count() * chunk_count);
    }
    lane_sync_u64(&lane_chunk_bytecode_data_counts, 0);
    lane_sync_u64(&lane_chunk_constant_data_counts, 0);
    lane_sync_u64(&lane_chunk_set_element_counts, 0);
    ProfScope("count indirected location data")
    {
      U64 chunk_idx = 0;
      for EachElement(symbol_table_list_idx, symbol_table_lists)
      {
        RDIM_SymbolChunkList *src_symbols = symbol_table_lists[symbol_table_list_idx].symbols;
        for EachNode(n, RDIM_SymbolChunkNode, src_symbols->first)
        {
          U64 slot_idx = lane_idx()*chunk_count + chunk_idx;
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            RDIM_Symbol *s = &n->v[n_idx];
            if(s->location_cases.count > 1)
            {
              lane_chunk_set_element_counts[slot_idx] += s->location_cases.count;
            }
            lane_chunk_constant_data_counts[slot_idx] += s->value_data.size;
            for EachNode(case_n, RDIM_LocationCase, s->location_cases.first)
            {
              lane_chunk_bytecode_data_counts[slot_idx] += case_n->location->info.bytecode.encoded_size;
            }
          }
          chunk_idx += 1;
        }
      }
    }
    
    ////////////////////////////
    //- rjf: lay out indirected location data
    //
    U64 *lane_chunk_bytecode_data_offs = 0;
    U64 *lane_chunk_constant_data_offs = 0;
    U64 *lane_chunk_set_element_offs = 0;
    U64 total_bytecode_data_count = 0;
    U64 total_constant_data_count = 0;
    U64 total_set_element_count = 0;
    ProfScope("lay out indirected location data")
    {
      if(lane_idx() == 0)
      {
        U64 bytecode_data_off = 0;
        U64 constant_data_off = 0;
        U64 set_element_off = 0;
        lane_chunk_bytecode_data_offs = push_array(scratch.arena, U64, lane_count() * chunk_count);
        lane_chunk_constant_data_offs = push_array(scratch.arena, U64, lane_count() * chunk_count);
        lane_chunk_set_element_offs = push_array(scratch.arena, U64, lane_count() * chunk_count);
        for EachIndex(chunk_idx, chunk_count)
        {
          for EachIndex(l_idx, lane_count())
          {
            U64 slot_idx = l_idx*chunk_count + chunk_idx;
            lane_chunk_bytecode_data_offs[slot_idx] = bytecode_data_off;
            lane_chunk_constant_data_offs[slot_idx] = constant_data_off;
            lane_chunk_set_element_offs[slot_idx] = set_element_off;
            bytecode_data_off += lane_chunk_bytecode_data_counts[slot_idx];
            constant_data_off += lane_chunk_constant_data_counts[slot_idx];
            set_element_off += lane_chunk_set_element_counts[slot_idx];
          }
        }
        total_bytecode_data_count = bytecode_data_off;
        total_constant_data_count = constant_data_off;
        total_set_element_count = set_element_off;
      }
      lane_sync_u64(&lane_chunk_bytecode_data_offs, 0);
      lane_sync_u64(&lane_chunk_constant_data_offs, 0);
      lane_sync_u64(&lane_chunk_set_element_offs, 0);
      lane_sync_u64(&total_bytecode_data_count, 0);
      lane_sync_u64(&total_constant_data_count, 0);
      lane_sync_u64(&total_set_element_count, 0);
    }
    
    ////////////////////////////
    //- rjf: set up location outputs
    //
    RDI_U8 *loc_bytecode_data = 0;
    RDI_U8 *loc_constant_data = 0;
    RDI_LocationSetElement *loc_set_elements = 0;
    if(lane_idx() == 0)
    {
      loc_bytecode_data = push_array(arena, RDI_U8, total_bytecode_data_count);
      loc_constant_data = push_array(arena, RDI_U8, total_constant_data_count);
      loc_set_elements = push_array(arena, RDI_LocationSetElement, total_set_element_count);
    }
    lane_sync_u64(&loc_bytecode_data, 0);
    lane_sync_u64(&loc_constant_data, 0);
    lane_sync_u64(&loc_set_elements, 0);
    
    ////////////////////////////
    //- rjf: bake flat symbol tables
    //
    ProfScope("bake flat symbol tables")
    {
      U64 chunk_idx = 0;
      for EachElement(symbol_table_list_idx, symbol_table_lists)
      {
        ProfBegin("table %I64u", symbol_table_list_idx);
        RDIM_SymbolChunkList *src_symbols = symbol_table_lists[symbol_table_list_idx].symbols;
        U64 dst_symbols_count = src_symbols->total_count + 1;
        RDI_Symbol *dst_symbols = 0;
        if(lane_idx() == 0)
        {
          dst_symbols = push_array(arena, RDI_Symbol, dst_symbols_count);
        }
        lane_sync_u64(&dst_symbols, 0);
        for EachNode(n, RDIM_SymbolChunkNode, src_symbols->first)
        {
          U64 slot_idx = lane_idx()*chunk_count + chunk_idx;
          U64 dst_bytecode_off = lane_chunk_bytecode_data_offs[slot_idx];
          U64 dst_constant_off = lane_chunk_constant_data_offs[slot_idx];
          U64 dst_set_element_idx = lane_chunk_set_element_offs[slot_idx];
          Rng1U64 range = lane_range(n->count);
          for EachInRange(n_idx, range)
          {
            RDIM_Symbol *src = &n->v[n_idx];
            RDI_Symbol *dst = &dst_symbols[n->base_idx + n_idx + 1];
            
            // rjf: fill basics
            dst->name_string_idx      = rdim_bake_idx_from_string(bake_strings, src->name);
            dst->type_idx             = (RDI_U32)rdim_idx_from_type(src->type); // TODO(rjf): @u64_to_u32
            dst->root_scope_idx       = (RDI_U32)rdim_idx_from_scope(src->root_scope); // TODO(rjf): @u64_to_u32
            dst->link_name_string_idx = rdim_bake_idx_from_string(bake_strings, src->link_name);
            
            // rjf: fill container info
            if(src->is_extern)
            {
              dst->container_flags |= RDI_ContainerFlag_External;
            }
            if(src->container_scope != 0)
            {
              dst->container_flags |= RDI_ContainerKind_Scope;
              dst->container_idx = rdim_idx_from_scope(src->container_scope);
            }
            else if(src->container_type != 0)
            {
              dst->container_flags |= RDI_ContainerKind_Type;
              dst->container_idx = rdim_idx_from_udt(src->container_type->udt);
            }
            
            // rjf: fill location set info, if applicable - get set elements to fill
            RDI_LocationSetElement *dst_loc_set_elements = loc_set_elements + dst_set_element_idx; 
            U64 dst_loc_set_elements_count = 0;
            if(src->location_cases.count > 1)
            {
              dst->location |= (((U64)RDI_LocationKind_Set) << RDI_Location_KindShift);
              dst->location |= ((src->location_cases.count << RDI_Location_SetCountShift) & RDI_Location_SetCountMask);
              dst->location |= ((dst_set_element_idx << RDI_Location_SetFirstIndexShift) & RDI_Location_SetFirstIndexMask);
              dst_loc_set_elements_count = src->location_cases.count;
              dst_set_element_idx += dst_loc_set_elements_count;
            }
            
            // rjf: fill virtual offset ranges of location set elements
            U64 loc_set_element_idx = 0;
            for EachNode(c, RDIM_LocationCase, src->location_cases.first)
            {
              dst_loc_set_elements[loc_set_element_idx].voff_first = c->voff_range.min;
              dst_loc_set_elements[loc_set_element_idx].voff_opl   = c->voff_range.max;
              loc_set_element_idx += 1;
            }
            
            // rjf: fill location(s)
            RDI_Location *dst_loc_first = &dst->location;
            U64 dst_loc_count = 1;
            if(dst_loc_set_elements_count > 0)
            {
              dst_loc_first = &dst_loc_set_elements[0].location;
              dst_loc_count = dst_loc_set_elements_count;
            }
            {
              RDI_Location *dst_loc = dst_loc_first;
              for EachNode(c, RDIM_LocationCase, src->location_cases.first)
              {
                RDIM_Location *src_loc = c->location;
                dst_loc[0] |= ((U64)src_loc->info.kind << RDI_Location_KindShift) & RDI_Location_KindMask;
                switch(src_loc->info.kind)
                {
                  default:{}break;
                  case RDI_LocationKind_AddrRegPlusU16:
                  case RDI_LocationKind_AddrAddrRegPlusU16:
                  {
                    dst_loc[0] |= ((U64)src_loc->info.reg_code << RDI_Location_RegCodeShift) & RDI_Location_RegCodeMask;
                    dst_loc[0] |= ((U64)src_loc->info.offset   << RDI_Location_RegOffShift) & RDI_Location_RegOffMask;
                  }break;
                  case RDI_LocationKind_ValReg:
                  {
                    dst_loc[0] |= ((U64)src_loc->info.reg_code << RDI_Location_RegCodeShift) & RDI_Location_RegCodeMask;
                  }break;
                  case RDI_LocationKind_AddrBytecodeStream:
                  case RDI_LocationKind_ValBytecodeStream:
                  {
                    dst_loc[0] |= ((U64)dst_bytecode_off << RDI_Location_OffShift) & RDI_Location_OffMask;
                    dst_bytecode_off += src_loc->info.bytecode.encoded_size;
                    // TODO(rjf): serialize bytecode
                  }break;
                  case RDI_LocationKind_ModuleOff:
                  case RDI_LocationKind_TLSOff:
                  {
                    dst_loc[0] |= ((U64)src_loc->info.offset << RDI_Location_OffShift) & RDI_Location_OffMask;
                  }break;
                  case RDI_LocationKind_ConstantDataOff:
                  {
                    dst_loc[0] |= ((U64)src_loc->info.offset << RDI_Location_OffShift) & RDI_Location_OffMask;
                    // TODO(rjf): need to move constant data, currently called "value_data", into
                    // location cases, to make this case work...
                  }break;
                }
                if(c->next != 0)
                {
                  dst_loc = &(CastFromMember(RDI_LocationSetElement, location, dst_loc) + 1)->location;
                }
              }
            }
          }
          chunk_idx += 1;
        }
        ProfEnd();
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake file paths
  //
  RDIM_FilePathBakeResult *baked_file_paths = 0;
  RDIM_BakePathNode **baked_file_path_src_nodes = 0;
  ProfScope("bake file paths")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      baked_file_paths = push_array(scratch.arena, RDIM_FilePathBakeResult, 1);
      baked_file_paths->nodes_count = path_tree->count;
      baked_file_paths->nodes = push_array(arena, RDI_FilePathNode, baked_file_paths->nodes_count);
      baked_file_path_src_nodes = push_array(arena, RDIM_BakePathNode *, baked_file_paths->nodes_count);
      {
        U64 idx = 0;
        for(RDIM_BakePathNode *n = path_tree->first; n != 0; n = n->next_order)
        {
          baked_file_path_src_nodes[idx] = n;
          idx += 1;
        }
      }
    }
    lane_sync_u64(&baked_file_paths, 0);
    lane_sync_u64(&baked_file_path_src_nodes, 0);
    
    // rjf: fill
    {
      Rng1U64 range = lane_range(baked_file_paths->nodes_count);
      for EachInRange(idx, range)
      {
        RDIM_BakePathNode *src = baked_file_path_src_nodes[idx];
        RDI_FilePathNode *dst = &baked_file_paths->nodes[idx];
        dst->name_string_idx = rdim_bake_idx_from_string(bake_strings, src->name);
        dst->source_file_idx = rdim_idx_from_src_file(src->src_file);
        if(src->parent != 0)
        {
          dst->parent_path_node = src->parent->idx;
        }
        if(src->first_child != 0)
        {
          dst->first_child = src->first_child->idx;
        }
        if(src->next_sibling != 0)
        {
          dst->next_sibling = src->next_sibling->idx;
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage do small final baking tasks
  //
  RDIM_TopLevelInfoBakeResult *baked_top_level_info = 0;
  RDIM_BinarySectionBakeResult *baked_binary_sections = 0;
  ProfScope("do small final baking tasks")
  {
    if(lane_idx() == lane_from_task_idx(0)) ProfScope("bake top level info")
    {
      baked_top_level_info = push_array(scratch.arena, RDIM_TopLevelInfoBakeResult, 1);
      baked_top_level_info->top_level_info                           = push_array(arena, RDI_TopLevelInfo, 1);
      baked_top_level_info->top_level_info->arch                     = params->top_level_info.arch;
      baked_top_level_info->top_level_info->exe_name_string_idx      = rdim_bake_idx_from_string(bake_strings, params->top_level_info.exe_name);
      baked_top_level_info->top_level_info->exe_hash                 = params->top_level_info.exe_hash;
      baked_top_level_info->top_level_info->voff_max                 = params->top_level_info.voff_max;
      baked_top_level_info->top_level_info->guid                     = params->top_level_info.guid;
      baked_top_level_info->top_level_info->producer_name_string_idx = rdim_bake_idx_from_string(bake_strings, params->top_level_info.producer_name);
    }
    if(lane_idx() == lane_from_task_idx(1)) ProfScope("bake binary sections")
    {
      baked_binary_sections = push_array(scratch.arena, RDIM_BinarySectionBakeResult, 1);
      RDIM_BinarySectionList *src = &params->binary_sections;
      RDI_BinarySection *dst_base = rdim_push_array(arena, RDI_BinarySection, src->count+1);
      U64 dst_idx = 1;
      for(RDIM_BinarySectionNode *src_n = src->first; src_n != 0; src_n = src_n->next, dst_idx += 1)
      {
        RDIM_BinarySection *src = &src_n->v;
        RDI_BinarySection *dst = &dst_base[dst_idx];
        dst->name_string_idx = rdim_bake_idx_from_string(bake_strings, src->name);
        dst->flags           = src->flags;
        dst->voff_first      = src->voff_first;
        dst->voff_opl        = src->voff_opl;
        dst->foff_first      = src->foff_first;
        dst->foff_opl        = src->foff_opl;
      }
      baked_binary_sections->binary_sections = dst_base;
      baked_binary_sections->binary_sections_count = dst_idx;
    }
  }
  lane_sync_u64(&baked_top_level_info, lane_from_task_idx(0));
  lane_sync_u64(&baked_binary_sections, lane_from_task_idx(1));
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage package results
  //
  RDIM_BakeResults result = {0};
  {
    result.top_level_info         = *baked_top_level_info;
    result.binary_sections        = *baked_binary_sections;
    result.units                  = *baked_units;
    result.unit_vmap              = *baked_unit_vmap;
    result.src_files              = *baked_src_files;
    result.checksums              = *baked_checksums;
    result.line_tables            = *baked_line_tables;
    result.type_nodes             = *baked_type_nodes;
    result.udts                   = *baked_udts;
    result.global_variables       = *baked_global_variables;
    result.global_vmap            = *baked_global_vmap;
    result.thread_variables       = *baked_thread_variables;
    result.constants              = *baked_constants;
    result.procedures             = *baked_procedures;
    result.scopes                 = *baked_scopes;
    result.inline_sites           = *baked_inline_sites;
    result.scope_vmap             = *baked_scope_vmap;
    result.top_level_name_maps    = *baked_top_level_name_maps;
    result.name_maps              = *baked_name_maps;
    result.file_paths             = *baked_file_paths;
    result.strings                = *baked_strings;
    result.idx_runs               = *baked_idx_runs;
    result.locations              = *baked_locations;
    result.location_blocks        = *baked_location_blocks;
  }
  lane_sync();
  
  scratch_end(scratch);
  return result;
}

internal RDIM_SerializedSectionBundle
rdim_compress(Arena *arena, RDIM_SerializedSectionBundle *in)
{
  Temp scratch = scratch_begin(&arena, 1);
  RDIM_SerializedSectionBundle out_ = {0};
  RDIM_SerializedSectionBundle *out = &out_;
  lane_sync_u64(&out, 0);
  
  //- rjf: set up compression context
  rr_lzb_simple_context ctx = {0};
  ctx.m_tableSizeBits = 14;
  ctx.m_hashTable = push_array(scratch.arena, U16, 1<<ctx.m_tableSizeBits);
  
  //- rjf: compress, or just copy, all sections
  Rng1U64 range = lane_range(RDI_SectionKind_COUNT);
  for EachInRange(idx, range)
  {
    RDI_SectionKind k = (RDI_SectionKind)idx;
    RDIM_SerializedSection *src = &in->sections[k];
    RDIM_SerializedSection *dst = &out->sections[k];
    MemoryCopyStruct(dst, src);
    if(src->encoded_size != 0)
    {
      MemoryZero(ctx.m_hashTable, sizeof(U16)*(1<<ctx.m_tableSizeBits));
      dst->data = push_array_no_zero(arena, U8, src->encoded_size);
      dst->encoded_size = rr_lzb_simple_encode_veryfast(&ctx, src->data, src->encoded_size, dst->data);
      dst->unpacked_size = src->encoded_size;
      dst->encoding = RDI_SectionEncoding_LZB;
    }
  }
  lane_sync();
  
  scratch_end(scratch);
  return *out;
}
