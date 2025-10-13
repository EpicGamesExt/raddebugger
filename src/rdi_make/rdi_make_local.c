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
  RDI_Arch arch_rdi;
  switch (arch) {
    case Arch_Null: arch_rdi = RDI_Arch_NULL; break;
    case Arch_x64:  arch_rdi = RDI_Arch_X64;  break;
    case Arch_x86:  arch_rdi = RDI_Arch_X86;  break;
    default: NotImplemented; break;
  }
  
  
  // find max VOFF
  U64 exe_voff_max = 0;
  for (RDIM_BinarySectionNode *sect_n = sections.first; sect_n != 0 ; sect_n = sect_n->next) {
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

internal RDIM_BakeResults
rdim_bake(Arena *arena, RDIM_BakeParams *params)
{
  //////////////////////////////////////////////////////////////
  //- rjf: set up shared state
  //
  if(lane_idx() == 0)
  {
    rdim_shared = push_array(arena, RDIM_Shared, 1);
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage gather unsorted vmap keys/markers
  //
  ProfScope("gather unsorted vmap keys/markers")
  {
    //- rjf: gather scope vmap keys/markers
    if(lane_idx() == lane_from_task_idx(0)) ProfScope("gather scope vmap keys/markers")
    {
      rdim_shared->scope_vmap_count = params->scopes.scope_voff_count;
      rdim_shared->scope_vmap_keys = push_array_no_zero(arena, RDIM_SortKey, rdim_shared->scope_vmap_count);
      rdim_shared->scope_vmap_keys__swap = push_array_no_zero(arena, RDIM_SortKey, rdim_shared->scope_vmap_count);
      rdim_shared->scope_vmap_markers = push_array_no_zero(arena, RDIM_VMapMarker, rdim_shared->scope_vmap_count);
      ProfScope("fill keys/markers")
      {
        RDIM_SortKey *key_ptr = rdim_shared->scope_vmap_keys;
        RDIM_VMapMarker *marker_ptr = rdim_shared->scope_vmap_markers;
        for(RDIM_ScopeChunkNode *chunk_n = params->scopes.first; chunk_n != 0; chunk_n = chunk_n->next)
        {
          for(RDI_U64 chunk_idx = 0; chunk_idx < chunk_n->count; chunk_idx += 1)
          {
            RDIM_Scope *src_scope = &chunk_n->v[chunk_idx];
            RDI_U32 scope_idx = (RDI_U32)rdim_idx_from_scope(src_scope); // TODO(rjf): @u64_to_u32
            for(RDIM_Rng1U64Node *n = src_scope->voff_ranges.first; n != 0; n = n->next)
            {
              key_ptr->key = n->v.min;
              key_ptr->val = marker_ptr;
              marker_ptr->idx = scope_idx;
              marker_ptr->begin_range = 1;
              key_ptr += 1;
              marker_ptr += 1;
              
              key_ptr->key = n->v.max;
              key_ptr->val = marker_ptr;
              marker_ptr->idx = scope_idx;
              marker_ptr->begin_range = 0;
              key_ptr += 1;
              marker_ptr += 1;
            }
          }
        }
      }
    }
    
    //- rjf: gather unit vmap keys/markers
    if(lane_idx() == lane_from_task_idx(1)) ProfScope("gather unit vmap keys/markers")
    {
      // rjf: count voff ranges
      RDI_U64 voff_range_count = 0;
      for(RDIM_UnitChunkNode *n = params->units.first; n != 0; n = n->next)
      {
        for(RDI_U64 idx = 0; idx < n->count; idx += 1)
        {
          RDIM_Unit *unit = &n->v[idx];
          voff_range_count += unit->voff_ranges.total_count;
        }
      }
      
      // rjf: count necessary markers
      RDI_U64 marker_count = voff_range_count*2;
      
      // rjf: build keys/markers arrays
      RDIM_SortKey    *keys = rdim_push_array_no_zero(arena, RDIM_SortKey, marker_count);
      RDIM_VMapMarker *markers = rdim_push_array_no_zero(arena, RDIM_VMapMarker, marker_count);
      {
        RDIM_SortKey *key_ptr = keys;
        RDIM_VMapMarker *marker_ptr = markers;
        RDI_U32 unit_idx = 1;
        for(RDIM_UnitChunkNode *unit_chunk_n = params->units.first;
            unit_chunk_n != 0;
            unit_chunk_n = unit_chunk_n->next)
        {
          for(RDI_U64 idx = 0; idx < unit_chunk_n->count; idx += 1)
          {
            RDIM_Unit *unit = &unit_chunk_n->v[idx];
            for(RDIM_Rng1U64ChunkNode *n = unit->voff_ranges.first; n != 0; n = n->next)
            {
              for(RDI_U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1)
              {
                RDIM_Rng1U64 range = n->v[chunk_idx];
                if(range.min < range.max)
                {
                  key_ptr->key = range.min;
                  key_ptr->val = marker_ptr;
                  marker_ptr->idx = unit_idx;
                  marker_ptr->begin_range = 1;
                  key_ptr += 1;
                  marker_ptr += 1;
                  
                  key_ptr->key = range.max;
                  key_ptr->val = marker_ptr;
                  marker_ptr->idx = unit_idx;
                  marker_ptr->begin_range = 0;
                  key_ptr += 1;
                  marker_ptr += 1;
                }
              }
            }
            unit_idx += 1;
          }
        }
      }
      
      // rjf: store
      rdim_shared->unit_vmap_count = marker_count;
      rdim_shared->unit_vmap_keys = keys;
      rdim_shared->unit_vmap_keys__swap = push_array_no_zero(arena, RDIM_SortKey, marker_count);
      rdim_shared->unit_vmap_markers = markers;
    }
    
    //- rjf: gather global vmap keys/markers
    if(lane_idx() == lane_from_task_idx(2)) ProfScope("gather global vmap keys/markers")
    {
      //- rjf: allocate keys/markers
      RDI_U64 marker_count = params->global_variables.total_count*2 + 2;
      RDIM_SortKey    *keys    = rdim_push_array_no_zero(arena, RDIM_SortKey, marker_count);
      RDIM_VMapMarker *markers = rdim_push_array_no_zero(arena, RDIM_VMapMarker, marker_count);
      
      //- rjf: fill
      {
        RDIM_SortKey *key_ptr = keys;
        RDIM_VMapMarker *marker_ptr = markers;
        
        // rjf: fill actual globals
        for(RDIM_SymbolChunkNode *n = params->global_variables.first; n != 0; n = n->next)
        {
          for(RDI_U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1)
          {
            RDIM_Symbol *global_var = &n->v[chunk_idx];
            RDI_U32 global_var_idx = (RDI_U32)rdim_idx_from_symbol(global_var); // TODO(rjf): @u64_to_u32
            RDI_U64 global_var_size = global_var->type ? global_var->type->byte_size : 1;
            
            RDI_U64 first = global_var->offset;
            RDI_U64 opl   = first + global_var_size;
            
            key_ptr->key = first;
            key_ptr->val = marker_ptr;
            marker_ptr->idx = global_var_idx;
            marker_ptr->begin_range = 1;
            key_ptr += 1;
            marker_ptr += 1;
            
            key_ptr->key = opl;
            key_ptr->val = marker_ptr;
            marker_ptr->idx = global_var_idx;
            marker_ptr->begin_range = 0;
            key_ptr += 1;
            marker_ptr += 1;
          }
        }
        
        // rjf: fill nil global
        {
          RDI_U32 global_idx = 0;
          RDI_U64 first = 0;
          RDI_U64 opl   = 0xffffffffffffffffull;
          key_ptr->key = first;
          key_ptr->val = marker_ptr;
          marker_ptr->idx = global_idx;
          marker_ptr->begin_range = 1;
          key_ptr += 1;
          marker_ptr += 1;
          key_ptr->key = opl;
          key_ptr->val = marker_ptr;
          marker_ptr->idx = global_idx;
          marker_ptr->begin_range = 0;
          key_ptr += 1;
          marker_ptr += 1;
        }
      }
      
      //- rjf: store
      rdim_shared->global_vmap_count = marker_count;
      rdim_shared->global_vmap_keys = keys;
      rdim_shared->global_vmap_keys__swap = push_array_no_zero(arena, RDIM_SortKey, marker_count);
      rdim_shared->global_vmap_markers = markers;
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage sort all vmap keys
  //
  ProfScope("sort all vmap keys")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      rdim_shared->lane_digit_counts = push_array(arena, U32 *, lane_count());
      rdim_shared->lane_digit_offsets = push_array(arena, U32 *, lane_count());
    }
    lane_sync();
    
    // rjf: sort
    struct
    {
      RDI_U64 vmap_count;
      RDIM_SortKey *keys;
      RDIM_SortKey *keys__swap;
    }
    sort_tasks[] =
    {
      {rdim_shared->scope_vmap_count,  rdim_shared->scope_vmap_keys,  rdim_shared->scope_vmap_keys__swap},
      {rdim_shared->unit_vmap_count,   rdim_shared->unit_vmap_keys,   rdim_shared->unit_vmap_keys__swap},
      {rdim_shared->global_vmap_count, rdim_shared->global_vmap_keys, rdim_shared->global_vmap_keys__swap},
    };
    for EachElement(sort_task_idx, sort_tasks) ProfScope("sort %I64u", sort_task_idx)
    {
      RDI_U64 vmap_count = sort_tasks[sort_task_idx].vmap_count;
      RDIM_SortKey *keys = sort_tasks[sort_task_idx].keys;
      RDIM_SortKey *keys__swap = sort_tasks[sort_task_idx].keys__swap;
      U64 bits_per_digit = 8;
      U64 digits_count = 64 / bits_per_digit;
      U64 num_possible_values_per_digit = 1 << bits_per_digit;
      rdim_shared->lane_digit_counts[lane_idx()] = push_array_no_zero(arena, U32, num_possible_values_per_digit);
      rdim_shared->lane_digit_offsets[lane_idx()] = push_array_no_zero(arena, U32, num_possible_values_per_digit);
      RDIM_SortKey *src = keys;
      RDIM_SortKey *dst = keys__swap;
      U64 element_count = vmap_count;
      for EachIndex(digit_idx, digits_count)
      {
        // rjf: count digit value occurrences per-lane
        {
          U32 *digit_counts = rdim_shared->lane_digit_counts[lane_idx()];
          MemoryZero(digit_counts, sizeof(digit_counts[0])*num_possible_values_per_digit);
          Rng1U64 range = lane_range(element_count);
          for EachInRange(idx, range)
          {
            RDIM_SortKey *sort_key = &src[idx];
            U16 digit_value = (U16)(U8)(sort_key->key >> (digit_idx*bits_per_digit));
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
              rdim_shared->lane_digit_offsets[lane_idx][value_idx] = layout_off;
              layout_off += rdim_shared->lane_digit_counts[lane_idx][value_idx];
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
              rdim_shared->lane_digit_offsets[lane_idx][value_idx] += last_off;
            }
            last_off = rdim_shared->lane_digit_offsets[lane_count()-1][value_idx] + rdim_shared->lane_digit_counts[lane_count()-1][value_idx];
          }
          // NOTE(rjf): required that: (last_off == element_count)
        }
        lane_sync();
        
        // rjf: move
        {
          U32 *lane_digit_offsets = rdim_shared->lane_digit_offsets[lane_idx()];
          Rng1U64 range = lane_range(element_count);
          for EachInRange(idx, range)
          {
            RDIM_SortKey *src_key = &src[idx];
            U16 digit_value = (U16)(U8)(src_key->key >> (digit_idx*bits_per_digit));
            U64 dst_off = lane_digit_offsets[digit_value];
            lane_digit_offsets[digit_value] += 1;
            MemoryCopyStruct(&dst[dst_off], src_key);
          }
        }
        lane_sync();
        
        // rjf: swap
        {
          RDIM_SortKey *swap = src;
          src = dst;
          dst = swap;
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake all vmaps
  //
  ProfScope("bake all vmaps")
  {
    Temp scratch = scratch_begin(&arena, 1);
    typedef struct VMapBakeTask VMapBakeTask;
    struct VMapBakeTask
    {
      VMapBakeTask *next;
      String8 name;
      RDI_U64 count;
      RDIM_SortKey *keys;
      RDIM_VMapMarker *markers;
      RDIM_BakeVMap *bake_vmap_out;
    };
    VMapBakeTask *first_task = 0;
    VMapBakeTask *last_task = 0;
    if(lane_idx() == lane_from_task_idx(0))
    {
      VMapBakeTask *task = push_array(scratch.arena, VMapBakeTask, 1);
      task->name          = str8_lit("scopes");
      task->count         = rdim_shared->scope_vmap_count;
      task->keys          = rdim_shared->scope_vmap_keys;
      task->markers       = rdim_shared->scope_vmap_markers;
      task->bake_vmap_out = &rdim_shared->baked_scope_vmap.vmap;
      SLLQueuePush(first_task, last_task, task);
    }
    if(lane_idx() == lane_from_task_idx(1))
    {
      VMapBakeTask *task = push_array(scratch.arena, VMapBakeTask, 1);
      task->name          = str8_lit("units");
      task->count         = rdim_shared->unit_vmap_count;
      task->keys          = rdim_shared->unit_vmap_keys;
      task->markers       = rdim_shared->unit_vmap_markers;
      task->bake_vmap_out = &rdim_shared->baked_unit_vmap.vmap;
      SLLQueuePush(first_task, last_task, task);
    }
    if(lane_idx() == lane_from_task_idx(2))
    {
      VMapBakeTask *task = push_array(scratch.arena, VMapBakeTask, 1);
      task->name          = str8_lit("globals");
      task->count         = rdim_shared->global_vmap_count;
      task->keys          = rdim_shared->global_vmap_keys;
      task->markers       = rdim_shared->global_vmap_markers;
      task->bake_vmap_out = &rdim_shared->baked_global_vmap.vmap;
      SLLQueuePush(first_task, last_task, task);
    }
    for(VMapBakeTask *task = first_task; task != 0; task = task->next) ProfScope("vmap bake for %.*s", str8_varg(task->name))
    {
      //- rjf: determine if an extra vmap entry for zero is needed
      RDI_U32 extra_vmap_entry = 0;
      if(task->count > 0 && task->keys[0].key != 0)
      {
        extra_vmap_entry = 1;
      }
      
      //- rjf: fill output vmap entries
      RDI_U32 vmap_count_raw = extra_vmap_entry + task->count;
      RDI_VMapEntry *vmap = rdim_push_array(arena, RDI_VMapEntry, vmap_count_raw);
      RDI_U32 vmap_entry_count_pass_1 = 0;
      ProfScope("fill output vmap entries")
      {
        typedef struct RDIM_VMapRangeTracker RDIM_VMapRangeTracker;
        struct RDIM_VMapRangeTracker
        {
          RDIM_VMapRangeTracker *next;
          RDI_U32 idx;
        };
        RDI_VMapEntry *vmap_ptr = vmap;
        if(extra_vmap_entry)
        {
          vmap_ptr->voff = 0;
          vmap_ptr->idx = 0;
          vmap_ptr += 1;
        }
        RDIM_VMapRangeTracker *tracker_stack = 0;
        RDIM_VMapRangeTracker *tracker_free = 0;
        RDIM_SortKey *key_ptr = task->keys;
        RDIM_SortKey *key_opl = task->keys + task->count;
        for(;key_ptr < key_opl;)
        {
          // rjf: get initial map state from tracker stack
          RDI_U32 initial_idx = (RDI_U32)0xffffffff;
          if(tracker_stack != 0)
          {
            initial_idx = tracker_stack->idx;
          }
          
          // rjf: update tracker stack
          //
          // * we must process _all_ of the changes that apply at this voff before moving on
          //
          RDI_U64 voff = key_ptr->key;
          
          for(;key_ptr < key_opl && key_ptr->key == voff; key_ptr += 1)
          {
            RDIM_VMapMarker *marker = (RDIM_VMapMarker*)key_ptr->val;
            RDI_U32 idx = marker->idx;
            
            // rjf: range begin -> push to stack
            if(marker->begin_range)
            {
              RDIM_VMapRangeTracker *new_tracker = tracker_free;
              if(new_tracker != 0)
              {
                RDIM_SLLStackPop(tracker_free);
              }
              else
              {
                new_tracker = rdim_push_array(scratch.arena, RDIM_VMapRangeTracker, 1);
              }
              RDIM_SLLStackPush(tracker_stack, new_tracker);
              new_tracker->idx = idx;
            }
            
            // rjf: range ending -> pop matching node from stack (not always the top)
            else
            {
              RDIM_VMapRangeTracker **ptr_in = &tracker_stack;
              RDIM_VMapRangeTracker *match = 0;
              for(RDIM_VMapRangeTracker *node = tracker_stack; node != 0;)
              {
                if(node->idx == idx)
                {
                  match = node;
                  break;
                }
                ptr_in = &node->next;
                node = node->next;
              }
              if(match != 0)
              {
                *ptr_in = match->next;
                RDIM_SLLStackPush(tracker_free, match);
              }
            }
          }
          
          // rjf: get final map state from tracker stack
          RDI_U32 final_idx = 0;
          if(tracker_stack != 0)
          {
            final_idx = tracker_stack->idx;
          }
          
          // rjf: if final is different from initial - emit new vmap entry
          if(final_idx != initial_idx)
          {
            vmap_ptr->voff = voff;
            vmap_ptr->idx = final_idx;
            vmap_ptr += 1;
          }
        }
        
        vmap_entry_count_pass_1 = (RDI_U32)(vmap_ptr - vmap); // TODO(rjf): @u64_to_u32
      }
      
      //- rjf: combine duplicate neighbors
      RDI_U32 vmap_entry_count = 0;
      ProfScope("combine duplicate neighbors")
      {
        RDI_VMapEntry *vmap_ptr = vmap;
        RDI_VMapEntry *vmap_opl = vmap + vmap_entry_count_pass_1;
        RDI_VMapEntry *vmap_out = vmap;
        for(;vmap_ptr < vmap_opl;)
        {
          RDI_VMapEntry *vmap_range_first = vmap_ptr;
          RDI_U64 idx = vmap_ptr->idx;
          vmap_ptr += 1;
          for(;vmap_ptr < vmap_opl && vmap_ptr->idx == idx;) vmap_ptr += 1;
          rdim_memcpy_struct(vmap_out, vmap_range_first);
          vmap_out += 1;
        }
        vmap_entry_count = (RDI_U32)(vmap_out - vmap); // TODO(rjf): @u64_to_u32
      }
      
      //- rjf: fill result
      task->bake_vmap_out->vmap = vmap;
      task->bake_vmap_out->count = vmap_entry_count;
    }
    scratch_end(scratch);
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage build interned path tree
  //
  if(lane_idx() == 0) ProfScope("build interned path tree")
  {
    //- rjf: set up tree
    RDIM_BakePathTree *tree = rdim_push_array(arena, RDIM_BakePathTree, 1);
    rdim_bake_path_tree_insert(arena, tree, rdim_str8_lit("<nil>"));
    
    //- rjf: bake unit file paths
    RDIM_ProfScope("bake unit file paths")
    {
      for(RDIM_UnitChunkNode *n = params->units.first; n != 0; n = n->next)
      {
        for(RDI_U64 idx = 0; idx < n->count; idx += 1)
        {
          rdim_bake_path_tree_insert(arena, tree, n->v[idx].source_file);
          rdim_bake_path_tree_insert(arena, tree, n->v[idx].object_file);
          rdim_bake_path_tree_insert(arena, tree, n->v[idx].archive_file);
          rdim_bake_path_tree_insert(arena, tree, n->v[idx].build_path);
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
          RDIM_BakePathNode *node = rdim_bake_path_tree_insert(arena, tree, n->v[idx].path);
          node->src_file = &n->v[idx];
        }
      }
    }
    
    rdim_shared->path_tree = tree;
  }
  lane_sync();
  RDIM_BakePathTree *path_tree = rdim_shared->path_tree;
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage gather all unsorted, joined, line table info; & sort
  //
  ProfScope("gather all unsorted, joined, line table info; & sort")
  {
    //- rjf: set up outputs
    ProfScope("set up outputs")
    {
      // rjf: calculate header info
      if(lane_idx() == 0)
      {
        rdim_shared->line_tables_count = params->line_tables.total_count;
        rdim_shared->src_line_tables = push_array(arena, RDIM_LineTable *, rdim_shared->line_tables_count);
        ProfScope("flatten chunk list")
        {
          U64 joined_idx = 0;
          for(RDIM_LineTableChunkNode *n = params->line_tables.first; n != 0; n = n->next)
          {
            for EachIndex(idx, n->count)
            {
              rdim_shared->src_line_tables[joined_idx] = &n->v[idx];
              joined_idx += 1;
            }
          }
        }
        rdim_shared->baked_line_tables.line_tables_count       = params->line_tables.total_count + 1;
        rdim_shared->baked_line_tables.line_table_voffs_count  = params->line_tables.total_line_count + 2*params->line_tables.total_seq_count;
        rdim_shared->baked_line_tables.line_table_lines_count  = params->line_tables.total_line_count + params->line_tables.total_seq_count;
        rdim_shared->baked_line_tables.line_table_columns_count= 1;
        rdim_shared->line_table_block_take_counter = 0;
      }
      lane_sync();
      
      // rjf: allocate outputs
      ProfScope("allocate outputs")
      {
        if(lane_idx() == lane_from_task_idx(0))
        {
          rdim_shared->unsorted_joined_line_tables = push_array(arena, RDIM_UnsortedJoinedLineTable, rdim_shared->line_tables_count);
        }
        if(lane_idx() == lane_from_task_idx(1))
        {
          rdim_shared->sorted_line_table_keys = push_array(arena, RDIM_SortKey *, rdim_shared->line_tables_count);
        }
        if(lane_idx() == lane_from_task_idx(2))
        {
          rdim_shared->baked_line_tables.line_tables = push_array(arena, RDI_LineTable, rdim_shared->baked_line_tables.line_tables_count);
          ProfScope("lay out line tables")
          {
            U64 voffs_base_idx = 0;
            U64 lines_base_idx = 0;
            U64 cols_base_idx = 0;
            for EachIndex(idx, rdim_shared->line_tables_count)
            {
              U64 final_idx = idx+1; // NOTE(rjf): +1, to reserve [0] for nil
              RDIM_LineTable *src = rdim_shared->src_line_tables[idx];
              RDI_LineTable *dst = &rdim_shared->baked_line_tables.line_tables[final_idx];
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
          rdim_shared->baked_line_tables.line_table_voffs   = push_array(arena, RDI_U64,       rdim_shared->baked_line_tables.line_table_voffs_count);
        }
        if(lane_idx() == lane_from_task_idx(4))
        {
          rdim_shared->baked_line_tables.line_table_lines   = push_array(arena, RDI_Line,      rdim_shared->baked_line_tables.line_table_lines_count);
        }
        if(lane_idx() == lane_from_task_idx(5))
        {
          rdim_shared->baked_line_tables.line_table_columns = push_array(arena, RDI_Column,    rdim_shared->baked_line_tables.line_table_columns_count);
        }
      }
    }
    lane_sync();
    
    //- rjf: wide bake
    ProfScope("wide bake") 
    {
      U64 line_table_block_size = 4096;
      U64 line_table_block_count = (rdim_shared->line_tables_count + line_table_block_size - 1) / line_table_block_size;
      for(;;)
      {
        U64 line_table_block_num = ins_atomic_u64_inc_eval(&rdim_shared->line_table_block_take_counter);
        if(0 == line_table_block_num || line_table_block_count < line_table_block_num)
        {
          break;
        }
        U64 line_table_block_idx = line_table_block_num-1;
        Rng1U64 line_table_range = r1u64(line_table_block_idx*line_table_block_size, (line_table_block_idx+1)*line_table_block_size);
        line_table_range.max = Min(rdim_shared->line_tables_count, line_table_range.max);
        for EachInRange(line_table_idx, line_table_range)
        {
          RDIM_LineTable *src = rdim_shared->src_line_tables[line_table_idx];
          RDIM_UnsortedJoinedLineTable *dst = &rdim_shared->unsorted_joined_line_tables[line_table_idx];
          
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
          rdim_shared->sorted_line_table_keys[line_table_idx] = rdim_sort_key_array(arena,
                                                                                    rdim_shared->unsorted_joined_line_tables[line_table_idx].line_keys,
                                                                                    rdim_shared->unsorted_joined_line_tables[line_table_idx].key_count);
          
          //- rjf: fill
          RDIM_SortKey *sorted_line_keys = rdim_shared->sorted_line_table_keys[line_table_idx];
          U64 sorted_line_keys_count = rdim_shared->unsorted_joined_line_tables[line_table_idx].key_count;
          RDI_LineTable *dst_line_table = &rdim_shared->baked_line_tables.line_tables[line_table_idx+1];
          U64 *arranged_voffs           = rdim_shared->baked_line_tables.line_table_voffs   + dst_line_table->voffs_base_idx;
          RDI_Line *arranged_lines      = rdim_shared->baked_line_tables.line_table_lines   + dst_line_table->lines_base_idx;
          RDI_Column *arranged_cols     = rdim_shared->baked_line_tables.line_table_columns + dst_line_table->cols_base_idx;
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
  RDI_U64 line_tables_count = rdim_shared->line_tables_count;
  RDIM_LineTable **src_line_tables = rdim_shared->src_line_tables;
  RDIM_UnsortedJoinedLineTable *unsorted_joined_line_tables = rdim_shared->unsorted_joined_line_tables;
  RDIM_SortKey **sorted_line_table_keys = rdim_shared->sorted_line_table_keys;
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage build string map
  //
  ProfScope("build string map")
  {
    //- rjf: set up per-lane outputs
    if(lane_idx() == 0) ProfScope("set up per-lane outputs")
    {
      rdim_shared->bake_string_map_topology.slots_count = (64 +
                                                           params->procedures.total_count*1 +
                                                           params->global_variables.total_count*1 +
                                                           params->thread_variables.total_count*1 +
                                                           params->types.total_count/2);
      rdim_shared->lane_bake_string_maps__loose = push_array(arena, RDIM_BakeStringMapLoose *, lane_count());
      rdim_shared->bake_string_map__loose = rdim_bake_string_map_loose_make(arena, &rdim_shared->bake_string_map_topology);
    }
    lane_sync();
    
    //- rjf: set up this lane's map
    ProfScope("set up this lane's map")
    {
      rdim_shared->lane_bake_string_maps__loose[lane_idx()] = rdim_bake_string_map_loose_make(arena, &rdim_shared->bake_string_map_topology);
    }
    RDIM_BakeStringMapTopology *lane_map_top = &rdim_shared->bake_string_map_topology;
    RDIM_BakeStringMapLoose *lane_map = rdim_shared->lane_bake_string_maps__loose[lane_idx()];
    
    //- rjf: push all strings into this lane's map
    ProfScope("push all strings into this lane's map")
    {
      // rjf: push small top-level strings
      if(lane_idx() == 0) ProfScope("push small top-level strings")
      {
        rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 1, params->top_level_info.exe_name);
        rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 1, params->top_level_info.producer_name);
        for(RDIM_BinarySectionNode *n = params->binary_sections.first; n != 0; n = n->next)
        {
          rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 1, n->v.name);
        }
        for(RDIM_BakePathNode *n = path_tree->first; n != 0; n = n->next_order)
        {
          rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 1, n->name);
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
            RDIM_String8 normalized_path = rdim_lower_from_str8(arena, n->v[n_idx].path);
            rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 1, normalized_path);
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
            rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, n->v[n_idx].unit_name);
            rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, n->v[n_idx].compiler_name);
            rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, n->v[n_idx].source_file);
            rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, n->v[n_idx].object_file);
            rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, n->v[n_idx].archive_file);
            rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, n->v[n_idx].build_path);
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
            rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, n->v[n_idx].name);
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
              rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, mem->name);
            }
            for EachNode(enum_val, RDIM_UDTEnumVal, n->v[idx].first_enum_val)
            {
              rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, enum_val->name);
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
              rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, n->v[n_idx].name);
              rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, n->v[n_idx].link_name);
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
            rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, n->v[n_idx].name);
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
              rdim_bake_string_map_loose_insert(arena, lane_map_top, lane_map, 4, local->name);
            }
          }
        }
      }
    }
    lane_sync();
    
    //- rjf: join
    ProfScope("join")
    {
      Rng1U64 slot_range = lane_range(rdim_shared->bake_string_map_topology.slots_count);
      for EachInRange(slot_idx, slot_range)
      {
        for EachIndex(src_lane_idx, lane_count())
        {
          RDIM_BakeStringMapLoose *src_map = rdim_shared->lane_bake_string_maps__loose[src_lane_idx];
          RDIM_BakeStringMapLoose *dst_map = rdim_shared->bake_string_map__loose;
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
      RDIM_BakeStringMapLoose *map = rdim_shared->bake_string_map__loose;
      Rng1U64 slot_range = lane_range(rdim_shared->bake_string_map_topology.slots_count);
      for EachInRange(slot_idx, slot_range)
      {
        if(map->slots[slot_idx] != 0 && map->slots[slot_idx]->total_count > 1)
        {
          *map->slots[slot_idx] = rdim_bake_string_chunk_list_sorted_from_unsorted(arena, map->slots[slot_idx]);
        }
      }
    }
    lane_sync();
    
    //- rjf: tighten string table
    ProfScope("tighten string table")
    {
      RDIM_BakeStringMapLoose *map = rdim_shared->bake_string_map__loose;
      RDIM_BakeStringMapTopology *map_top = &rdim_shared->bake_string_map_topology;
      if(lane_idx() == 0) ProfScope("calc base indices, set up tight map")
      {
        RDIM_BakeStringMapBaseIndices bake_string_map_base_indices = rdim_bake_string_map_base_indices_from_map_loose(arena, map_top, map);
        rdim_shared->bake_strings.slots_count = map_top->slots_count;
        rdim_shared->bake_strings.slots = rdim_push_array(arena, RDIM_BakeStringChunkList, rdim_shared->bake_strings.slots_count);
        rdim_shared->bake_strings.slots_base_idxs = bake_string_map_base_indices.slots_base_idxs;
        rdim_shared->bake_strings.total_count = rdim_shared->bake_strings.slots_base_idxs[rdim_shared->bake_strings.slots_count];
      }
      lane_sync();
      ProfScope("fill tight map")
      {
        Rng1U64 slot_range = lane_range(rdim_shared->bake_strings.slots_count);
        for EachInRange(idx, slot_range)
        {
          if(map->slots[idx] != 0)
          {
            rdim_memcpy_struct(&rdim_shared->bake_strings.slots[idx], map->slots[idx]);
          }
        }
      }
    }
  }
  lane_sync();
  RDIM_BakeStringMapTight *bake_strings = &rdim_shared->bake_strings;
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage build name maps
  //
  ProfScope("build name maps")
  {
    //- rjf: set up
    if(lane_idx() == 0)
    {
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
        rdim_shared->lane_bake_name_maps[k] = push_array(arena, RDIM_BakeNameMap *, lane_count());
        rdim_shared->bake_name_map_topology[k].slots_count = slot_count;
      }
    }
    lane_sync();
    
    //- rjf: wide build
    for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("name map build %.*s", str8_varg(rdi_string_from_name_map_kind(k)))
    {
      RDIM_BakeNameMapTopology *top = &rdim_shared->bake_name_map_topology[k];
      rdim_shared->lane_bake_name_maps[k][lane_idx()] = rdim_bake_name_map_make(arena, top);
      RDIM_BakeNameMap *map = rdim_shared->lane_bake_name_maps[k][lane_idx()];
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
              rdim_bake_name_map_insert(arena, top, map, 4, link_names ? symbol->link_name : symbol->name, rdim_idx_from_symbol(symbol));
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
              rdim_bake_name_map_insert(arena, top, map, 4, type->name, rdim_idx_from_type(type));
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
              RDIM_String8 normalized_path = rdim_lower_from_str8(arena, src_file->path);
              rdim_bake_name_map_insert(arena, top, map, 4, normalized_path, rdim_idx_from_src_file(src_file));
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
        rdim_shared->bake_name_maps[k] = rdim_bake_name_map_make(arena, &rdim_shared->bake_name_map_topology[k]);
      }
    }
    lane_sync();
    for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("name map join & sort %.*s", str8_varg(rdi_string_from_name_map_kind(k)))
    {
      RDIM_BakeNameMapTopology *top = &rdim_shared->bake_name_map_topology[k];
      RDIM_BakeNameMap *map = rdim_shared->bake_name_maps[k];
      
      //- rjf: join
      ProfScope("join")
      {
        Rng1U64 slot_range = lane_range(top->slots_count);
        for EachInRange(slot_idx, slot_range)
        {
          for EachIndex(src_lane_idx, lane_count())
          {
            RDIM_BakeNameMap *src_map = rdim_shared->lane_bake_name_maps[k][src_lane_idx];
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
          if(map->slots[slot_idx] != 0 && map->slots[slot_idx]->total_count > 1)
          {
            *map->slots[slot_idx] = rdim_bake_name_chunk_list_sorted_from_unsorted(arena, map->slots[slot_idx]);
          }
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage build index runs
  //
  ProfScope("build index runs")
  {
    //- rjf: set up per-lane outputs
    if(lane_idx() == 0) ProfScope("set up per-lane outputs")
    {
      rdim_shared->bake_idx_run_map_topology.slots_count = (64 +
                                                            params->procedures.total_count +
                                                            params->global_variables.total_count +
                                                            params->thread_variables.total_count +
                                                            params->types.total_count);
      rdim_shared->lane_bake_idx_run_maps__loose = push_array(arena, RDIM_BakeIdxRunMapLoose *, lane_count());
      rdim_shared->bake_idx_run_map__loose = rdim_bake_idx_run_map_loose_make(arena, &rdim_shared->bake_idx_run_map_topology);
    }
    lane_sync();
    
    //- rjf: set up this lane's map
    ProfScope("set up this lane's map")
    {
      rdim_shared->lane_bake_idx_run_maps__loose[lane_idx()] = rdim_bake_idx_run_map_loose_make(arena, &rdim_shared->bake_idx_run_map_topology);
    }
    RDIM_BakeIdxRunMapTopology *lane_map_top = &rdim_shared->bake_idx_run_map_topology;
    RDIM_BakeIdxRunMapLoose *lane_map = rdim_shared->lane_bake_idx_run_maps__loose[lane_idx()];
    
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
              rdim_bake_idx_run_map_loose_insert(arena, lane_map_top, lane_map, 4, param_idx_run, param_idx_run_count);
            }
          }
        }
      }
      
      //- rjf: bake runs of name map match lists
      for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("bake runs of name map match lists (%.*s)", str8_varg(rdi_string_from_name_map_kind(k)))
      {
        RDIM_BakeNameMapTopology *top = &rdim_shared->bake_name_map_topology[k];
        RDIM_BakeNameMap *map = rdim_shared->bake_name_maps[k];
        Rng1U64 slot_idx_range = lane_range(top->slots_count);
        for EachInRange(slot_idx, slot_idx_range)
        {
          RDIM_BakeNameChunkList *slot = map->slots[slot_idx];
          if(slot != 0)
          {
            Temp scratch = scratch_begin(&arena, 1);
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
                  rdim_bake_idx_run_map_loose_insert(arena, lane_map_top, lane_map, 4, idxs, idxs_count);
                }
                active_string = string;
                first_idx_run_node = 0;
                last_idx_run_node = 0;
                active_idx_count = 0;
                temp_end(scratch);
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
            scratch_end(scratch);
          }
        }
      }
    }
    lane_sync();
    
    //- rjf: join
    ProfScope("join")
    {
      Rng1U64 slot_range = lane_range(rdim_shared->bake_idx_run_map_topology.slots_count);
      for EachInRange(slot_idx, slot_range)
      {
        for EachIndex(src_lane_idx, lane_count())
        {
          RDIM_BakeIdxRunMapLoose *src_map = rdim_shared->lane_bake_idx_run_maps__loose[src_lane_idx];
          RDIM_BakeIdxRunMapLoose *dst_map = rdim_shared->bake_idx_run_map__loose;
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
      RDIM_BakeIdxRunMapLoose *map = rdim_shared->bake_idx_run_map__loose;
      Rng1U64 slot_range = lane_range(rdim_shared->bake_idx_run_map_topology.slots_count);
      for EachInRange(slot_idx, slot_range)
      {
        if(map->slots[slot_idx] != 0 && map->slots[slot_idx]->total_count > 1)
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
      RDIM_BakeIdxRunMapLoose *map = rdim_shared->bake_idx_run_map__loose;
      RDIM_BakeIdxRunMapTopology *map_top = &rdim_shared->bake_idx_run_map_topology;
      if(lane_idx() == 0) ProfScope("calc base indices, set up tight map")
      {
        rdim_shared->bake_idx_runs.slots_count = map_top->slots_count;
        rdim_shared->bake_idx_runs.slots = rdim_push_array(arena, RDIM_BakeIdxRunChunkList, rdim_shared->bake_idx_runs.slots_count);
        rdim_shared->bake_idx_runs.slots_base_idxs = rdim_push_array(arena, RDI_U64, rdim_shared->bake_idx_runs.slots_count+1);
        RDI_U64 encoding_idx_off = 0;
        for(RDI_U64 slot_idx = 0; slot_idx < map_top->slots_count; slot_idx += 1)
        {
          rdim_shared->bake_idx_runs.slots_base_idxs[slot_idx] = encoding_idx_off;
          if(map->slots[slot_idx] != 0)
          {
            encoding_idx_off += map->slots_idx_counts[slot_idx];
          }
        }
        rdim_shared->bake_idx_runs.slots_base_idxs[map_top->slots_count] = encoding_idx_off;
      }
      lane_sync();
      ProfScope("fill tight map")
      {
        Rng1U64 slot_range = lane_range(rdim_shared->bake_idx_runs.slots_count);
        for EachInRange(idx, slot_range)
        {
          if(map->slots[idx] != 0)
          {
            rdim_memcpy_struct(&rdim_shared->bake_idx_runs.slots[idx], map->slots[idx]);
          }
        }
      }
    }
  }
  lane_sync();
  RDIM_BakeIdxRunMap *bake_idx_runs = &rdim_shared->bake_idx_runs;
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake strings
  //
  ProfScope("bake strings")
  {
    // rjf: set up 
    if(lane_idx() == 0) ProfScope("set up; lay out strings")
    {
      rdim_shared->baked_strings.string_offs_count = bake_strings->total_count + 1;
      rdim_shared->baked_strings.string_offs = rdim_push_array(arena, RDI_U32, rdim_shared->baked_strings.string_offs_count);
      RDI_U64 off_cursor = 0;
      for EachIndex(slot_idx, bake_strings->slots_count)
      {
        for EachNode(n, RDIM_BakeStringChunkNode, bake_strings->slots[slot_idx].first)
        {
          for EachIndex(n_idx, n->count)
          {
            RDIM_BakeString *src = &n->v[n_idx];
            U64 dst_idx = bake_strings->slots_base_idxs[slot_idx] + n->base_idx + n_idx + 1;
            rdim_shared->baked_strings.string_offs[dst_idx] = off_cursor;
            off_cursor += src->string.size;
          }
        }
      }
      rdim_shared->baked_strings.string_data_size = off_cursor;
      rdim_shared->baked_strings.string_data = rdim_push_array(arena, RDI_U8, rdim_shared->baked_strings.string_data_size);
    }
    lane_sync();
    
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
            U64 dst_off = rdim_shared->baked_strings.string_offs[dst_idx];
            rdim_memcpy(rdim_shared->baked_strings.string_data + dst_off, src->string.str, src->string.size);
          }
        }
      }
    }
  }
  lane_sync();
  RDIM_StringBakeResult baked_strings = rdim_shared->baked_strings;
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake idx runs
  //
  ProfScope("bake idx runs")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      rdim_shared->baked_idx_runs.idx_count = bake_idx_runs->slots_base_idxs[bake_idx_runs->slots_count];
      rdim_shared->baked_idx_runs.idx_runs = push_array(arena, RDI_U32, rdim_shared->baked_idx_runs.idx_count);
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
          StaticAssert(sizeof(rdim_shared->baked_idx_runs.idx_runs[0]) == sizeof(n->v[0].idxes[0]), idx_run_size_check);
          for EachIndex(n_idx, n->count)
          {
            rdim_memcpy(rdim_shared->baked_idx_runs.idx_runs + off, n->v[n_idx].idxes, sizeof(n->v[n_idx].idxes[0]) * n->v[n_idx].count);
            off += n->v[n_idx].count;
          }
        }
      }
    }
  }
  lane_sync();
  RDIM_IndexRunBakeResult baked_idx_runs = rdim_shared->baked_idx_runs;
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake name maps
  //
  ProfScope("bake name maps")
  {
    // rjf: count unique names in all name maps; lay out baked nodes
    ProfScope("count unique names in all name maps; lay out baked nodes")
    {
      if(lane_idx() == 0)
      {
        for EachNonZeroEnumVal(RDI_NameMapKind, k)
        {
          rdim_shared->lane_name_map_node_counts[k] = push_array(arena, U64, lane_count());
          rdim_shared->lane_name_map_node_offs[k] = push_array(arena, U64, lane_count());
        }
      }
      lane_sync();
      for EachNonZeroEnumVal(RDI_NameMapKind, k)
      {
        RDIM_BakeNameMapTopology *top = &rdim_shared->bake_name_map_topology[k];
        RDIM_BakeNameMap *map = rdim_shared->bake_name_maps[k];
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
            rdim_shared->lane_name_map_node_counts[k][lane_idx()] += total_unique_name_count;
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
            rdim_shared->name_map_node_counts[k] += rdim_shared->lane_name_map_node_counts[k][l_idx];
            rdim_shared->lane_name_map_node_offs[k][l_idx] = node_off;
            node_off += rdim_shared->lane_name_map_node_counts[k][l_idx];
          }
          rdim_shared->total_name_map_node_count += rdim_shared->name_map_node_counts[k];
        }
      }
    }
    lane_sync();
    
    // rjf: setup
    ProfScope("setup")
    {
      if(lane_idx() == lane_from_task_idx(0))
      {
        rdim_shared->baked_top_level_name_maps.name_maps_count = RDI_NameMapKind_COUNT;
        rdim_shared->baked_top_level_name_maps.name_maps = push_array(arena, RDI_NameMap, rdim_shared->baked_top_level_name_maps.name_maps_count);
        RDI_U32 bucket_off = 0;
        RDI_U32 node_off = 0;
        for EachNonZeroEnumVal(RDI_NameMapKind, k)
        {
          rdim_shared->baked_top_level_name_maps.name_maps[k].bucket_base_idx = bucket_off;
          rdim_shared->baked_top_level_name_maps.name_maps[k].node_base_idx = node_off;
          rdim_shared->baked_top_level_name_maps.name_maps[k].bucket_count = (RDI_U32)rdim_shared->bake_name_map_topology[k].slots_count; // TODO(rjf): @u64_to_u32
          rdim_shared->baked_top_level_name_maps.name_maps[k].node_count = (RDI_U32)rdim_shared->name_map_node_counts[k]; // TODO(rjf): @u64_to_u32
          bucket_off += rdim_shared->baked_top_level_name_maps.name_maps[k].bucket_count;
          node_off += rdim_shared->baked_top_level_name_maps.name_maps[k].node_count;
        }
        rdim_shared->baked_name_maps.buckets_count = bucket_off;
        rdim_shared->baked_name_maps.buckets = push_array(arena, RDI_NameMapBucket, rdim_shared->baked_name_maps.buckets_count);
      }
      if(lane_idx() == lane_from_task_idx(1))
      {
        rdim_shared->baked_name_maps.nodes_count = rdim_shared->total_name_map_node_count;
        rdim_shared->baked_name_maps.nodes = push_array(arena, RDI_NameMapNode, rdim_shared->baked_name_maps.nodes_count);
      }
    }
    lane_sync();
    
    // rjf: wide fill baked name maps
    ProfScope("wide fill baked name maps")
    {
      for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("wide fill (%.*s)", str8_varg(rdi_string_from_name_map_kind(k)))
      {
        RDI_U64 write_node_off = rdim_shared->lane_name_map_node_offs[k][lane_idx()];
        RDIM_BakeNameMapTopology *top = &rdim_shared->bake_name_map_topology[k];
        U64 slots_count = top->slots_count;
        RDIM_BakeNameMap *src_map = rdim_shared->bake_name_maps[k];
        RDI_NameMap *dst_map = &rdim_shared->baked_top_level_name_maps.name_maps[k];
        RDI_NameMapBucket *dst_buckets = rdim_shared->baked_name_maps.buckets + dst_map->bucket_base_idx;
        RDI_NameMapNode *dst_nodes = rdim_shared->baked_name_maps.nodes + dst_map->node_base_idx;
        Rng1U64 slot_range = lane_range(slots_count);
        for EachInRange(slot_idx, slot_range)
        {
          RDIM_BakeNameChunkList *src_slot = src_map->slots[slot_idx];
          if(src_slot == 0) { continue; }
          RDI_NameMapBucket *dst_bucket = &dst_buckets[slot_idx];
          dst_bucket->first_node = write_node_off;
          {
            Temp scratch = scratch_begin(&arena, 1);
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
                  RDI_U32 *idxs = rdim_push_array(scratch.arena, RDI_U32, idxs_count);
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
                temp_end(scratch);
              }
              
              // rjf: hash matches the active list -> push
              if(active_string.size != 0 && str8_match(active_string, string, 0))
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
            scratch_end(scratch);
          }
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage gather line-bucketed src line map data
  //
  ProfScope("gather line-bucketed src line map data")
  {
    if(lane_idx() == 0)
    {
      rdim_shared->bake_src_line_maps = push_array(arena, RDIM_BakeSrcLineMap, params->src_files.total_count);
    }
    lane_sync();
    {
      for EachNode(n, RDIM_SrcFileChunkNode, params->src_files.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          U64 file_idx = n->base_idx + n_idx;
          RDIM_BakeSrcLineMap *map = &rdim_shared->bake_src_line_maps[file_idx];
          
          // rjf: set up map
          map->slots_count = n->v[n_idx].total_line_count;
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
  ProfScope("sort line-bucketed src line map data")
  {
    U64 map_count = params->src_files.total_count;
    if(lane_idx() == 0)
    {
      rdim_shared->bake_src_line_map_keys = push_array(arena, RDIM_SortKey *, map_count);
    }
    lane_sync();
    for(;;)
    {
      U64 map_num = ins_atomic_u64_inc_eval(&rdim_shared->bake_src_line_map_take_counter);
      if(map_num < 1 || map_count < map_num)
      {
        break;
      }
      U64 map_idx = map_num-1;
      RDIM_BakeSrcLineMap *map = &rdim_shared->bake_src_line_maps[map_idx];
      
      // rjf: gather keys
      rdim_shared->bake_src_line_map_keys[map_idx] = push_array_no_zero(arena, RDIM_SortKey, map->line_count);
      RDIM_SortKey *keys = rdim_shared->bake_src_line_map_keys[map_idx];
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
  ProfScope("compute src file / src file line map layout")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      rdim_shared->lane_chunk_src_file_num_counts  = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      rdim_shared->lane_chunk_src_file_voff_counts = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      rdim_shared->lane_chunk_src_file_map_counts  = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      rdim_shared->lane_chunk_src_file_num_offs    = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      rdim_shared->lane_chunk_src_file_voff_offs   = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      rdim_shared->lane_chunk_src_file_map_offs    = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      for EachEnumVal(RDI_ChecksumKind, k)
      {
        rdim_shared->lane_chunk_src_file_checksum_counts[k] = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
        rdim_shared->lane_chunk_src_file_checksum_offs[k] = push_array(arena, U64, lane_count()*params->src_files.chunk_count);
      }
    }
    lane_sync();
    
    // rjf: wide count
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_SrcFileChunkNode, params->src_files.first)
      {
        Rng1U64 range = lane_range(n->count);
        U64 slot_idx = lane_idx()*params->src_files.chunk_count + chunk_idx;
        for EachInRange(idx, range)
        {
          RDIM_BakeSrcLineMap *map = &rdim_shared->bake_src_line_maps[n->base_idx + idx];
          rdim_shared->lane_chunk_src_file_num_counts[slot_idx] += map->line_count;
          rdim_shared->lane_chunk_src_file_voff_counts[slot_idx] += map->voff_range_count;
          rdim_shared->lane_chunk_src_file_map_counts[slot_idx] += !!map->line_count;
          RDI_ChecksumKind k = n->v[idx].checksum_kind;
          String8 val = n->v[idx].checksum;
          if(RDI_ChecksumKind_NULL < k && k < RDI_ChecksumKind_COUNT && val.size != 0)
          {
            rdim_shared->lane_chunk_src_file_checksum_counts[k][slot_idx] += 1;
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
          rdim_shared->lane_chunk_src_file_num_offs[slot_idx] = num_layout_off;
          rdim_shared->lane_chunk_src_file_voff_offs[slot_idx] = voff_layout_off;
          rdim_shared->lane_chunk_src_file_map_offs[slot_idx] = map_layout_off;
          num_layout_off += rdim_shared->lane_chunk_src_file_num_counts[slot_idx];
          voff_layout_off += rdim_shared->lane_chunk_src_file_voff_counts[slot_idx];
          map_layout_off += rdim_shared->lane_chunk_src_file_map_counts[slot_idx];
          for EachEnumVal(RDI_ChecksumKind, k)
          {
            rdim_shared->lane_chunk_src_file_checksum_offs[k][slot_idx] = checksum_layout_offs[k];
            checksum_layout_offs[k] += rdim_shared->lane_chunk_src_file_checksum_counts[k][slot_idx];
          }
        }
        chunk_idx += 1;
      }
      rdim_shared->total_src_map_line_count = num_layout_off;
      rdim_shared->total_src_map_voff_count = voff_layout_off;
      for EachEnumVal(RDI_ChecksumKind, k)
      {
        rdim_shared->total_checksum_counts[k] = checksum_layout_offs[k];
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake src files
  //
  ProfScope("bake src files")
  {
    //- rjf: set up
    if(lane_idx() == 0)
    {
      rdim_shared->baked_src_files.source_files_count = params->src_files.total_count+1;
      rdim_shared->baked_src_files.source_files = push_array(arena, RDI_SourceFile, rdim_shared->baked_src_files.source_files_count);
      rdim_shared->baked_src_files.source_line_maps_count = params->src_files.source_line_map_count+1;
      rdim_shared->baked_src_files.source_line_maps = push_array(arena, RDI_SourceLineMap, rdim_shared->baked_src_files.source_line_maps_count);
      rdim_shared->baked_src_files.source_line_map_nums_count = rdim_shared->total_src_map_line_count;
      rdim_shared->baked_src_files.source_line_map_nums = push_array(arena, RDI_U32, rdim_shared->baked_src_files.source_line_map_nums_count);
      rdim_shared->baked_src_files.source_line_map_rngs_count = rdim_shared->total_src_map_line_count + rdim_shared->baked_src_files.source_line_maps_count;
      rdim_shared->baked_src_files.source_line_map_rngs = push_array(arena, RDI_U32, rdim_shared->baked_src_files.source_line_map_rngs_count);
      rdim_shared->baked_src_files.source_line_map_voffs_count = rdim_shared->total_src_map_voff_count;
      rdim_shared->baked_src_files.source_line_map_voffs = push_array(arena, RDI_U64, rdim_shared->baked_src_files.source_line_map_voffs_count);
    }
    lane_sync();
    
    //- rjf: bake
    U64 chunk_idx = 0;
    for EachNode(n, RDIM_SrcFileChunkNode, params->src_files.first)
    {
      Rng1U64 range = lane_range(n->count);
      U64 slot_idx = lane_idx()*params->src_files.chunk_count + chunk_idx;
      U64 dst_num_off  = rdim_shared->lane_chunk_src_file_num_offs[slot_idx];
      U64 dst_map_off  = rdim_shared->lane_chunk_src_file_map_offs[slot_idx];
      U64 dst_voff_off = rdim_shared->lane_chunk_src_file_voff_offs[slot_idx];
      U64 dst_rng_off  = dst_num_off + dst_map_off;
      U64 dst_checksums_off[RDI_ChecksumKind_COUNT] = {0};
      for EachEnumVal(RDI_ChecksumKind, k)
      {
        dst_checksums_off[k] = 1 + rdim_shared->lane_chunk_src_file_checksum_offs[k][slot_idx];
      }
      for EachInRange(idx, range)
      {
        RDIM_BakeSrcLineMap *map = &rdim_shared->bake_src_line_maps[n->base_idx + idx];
        RDIM_SortKey *sorted_map_keys = rdim_shared->bake_src_line_map_keys[n->base_idx + idx];
        RDIM_SrcFile *src = &n->v[idx];
        RDI_SourceFile *dst = &rdim_shared->baked_src_files.source_files[n->base_idx + idx + 1];
        RDI_SourceLineMap *dst_map = &rdim_shared->baked_src_files.source_line_maps[dst_map_off];
        RDI_U32 *dst_nums  = &rdim_shared->baked_src_files.source_line_map_nums[dst_num_off];
        RDI_U32 *dst_rngs  = &rdim_shared->baked_src_files.source_line_map_rngs[dst_rng_off];
        RDI_U64 *dst_voffs = &rdim_shared->baked_src_files.source_line_map_voffs[dst_voff_off];
        
        //- rjf: fill file info
        Temp scratch = scratch_begin(&arena, 1);
        String8 normalized_path = rdim_lower_from_str8(scratch.arena, src->path);
        B32 has_checksum = (RDI_ChecksumKind_NULL < src->checksum_kind && src->checksum_kind < RDI_ChecksumKind_COUNT && src->checksum.size != 0);
        dst->file_path_node_idx = rdim_bake_path_node_idx_from_string(path_tree, src->path);
        dst->normal_full_path_string_idx = rdim_bake_idx_from_string(bake_strings, normalized_path);
        dst->source_line_map_idx = src->total_line_count ? dst_map_off : 0;
        dst->checksum_kind = src->checksum_kind;
        dst->checksum_idx  = has_checksum ? dst_checksums_off[dst->checksum_kind] : 0;
        scratch_end(scratch);
        
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
  ProfScope("bake checksums")
  {
    // rjf: allocate
    if(lane_idx() == 0)
    {
      rdim_shared->baked_checksums.md5s_count = rdim_shared->total_checksum_counts[RDI_ChecksumKind_MD5] + 1;
      rdim_shared->baked_checksums.sha1s_count = rdim_shared->total_checksum_counts[RDI_ChecksumKind_SHA1] + 1;
      rdim_shared->baked_checksums.sha256s_count = rdim_shared->total_checksum_counts[RDI_ChecksumKind_SHA256] + 1;
      rdim_shared->baked_checksums.timestamps_count = rdim_shared->total_checksum_counts[RDI_ChecksumKind_Timestamp] + 1;
      rdim_shared->baked_checksums.md5s = push_array(arena, RDI_MD5, rdim_shared->baked_checksums.md5s_count);
      rdim_shared->baked_checksums.sha1s = push_array(arena, RDI_SHA1, rdim_shared->baked_checksums.sha1s_count);
      rdim_shared->baked_checksums.sha256s = push_array(arena, RDI_SHA256, rdim_shared->baked_checksums.sha256s_count);
      rdim_shared->baked_checksums.timestamps = push_array(arena, RDI_U64, rdim_shared->baked_checksums.timestamps_count);
    }
    lane_sync();
    
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
          dst_offs[k] = 1 + rdim_shared->lane_chunk_src_file_checksum_offs[k][slot_idx];
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
#define Case(name, table_name) case RDI_ChecksumKind_##name:{MemoryCopy(&rdim_shared->baked_checksums.table_name[dst_offs[k]], val.str, Min(val.size, sizeof(rdim_shared->baked_checksums.table_name[0])));}break
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
  //- rjf: @rdim_bake_stage compute lane UDT member/enum-val layouts
  //
  ProfScope("compute lane UDT member/enum-val layouts")
  {
    // rjf: allocate
    if(lane_idx() == 0)
    {
      rdim_shared->member_chunk_lane_counts = push_array(arena, U64, lane_count() * params->udts.chunk_count);
      rdim_shared->member_chunk_lane_offs = push_array(arena, U64, lane_count() * params->udts.chunk_count);
      rdim_shared->enum_val_chunk_lane_counts = push_array(arena, U64, lane_count() * params->udts.chunk_count);
      rdim_shared->enum_val_chunk_lane_offs = push_array(arena, U64, lane_count() * params->udts.chunk_count);
    }
    lane_sync();
    
    // rjf: count
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_UDTChunkNode, params->udts.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(idx, range)
        {
          U64 slot_idx = lane_idx()*params->udts.chunk_count + chunk_idx;
          rdim_shared->member_chunk_lane_counts[slot_idx] += n->v[idx].member_count;
          rdim_shared->enum_val_chunk_lane_counts[slot_idx] += n->v[idx].enum_val_count;
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
          rdim_shared->member_chunk_lane_offs[slot_idx] = member_layout_off;
          rdim_shared->enum_val_chunk_lane_offs[slot_idx] = enum_val_layout_off;
          member_layout_off += rdim_shared->member_chunk_lane_counts[slot_idx];
          enum_val_layout_off += rdim_shared->enum_val_chunk_lane_counts[slot_idx];
        }
        chunk_idx += 1;
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake UDTs
  //
  ProfScope("bake UDTs")
  {
    //- rjf: set up
    ProfScope("set up")
    {
      if(lane_idx() == lane_from_task_idx(0))
      {
        rdim_shared->baked_udts.udts_count = params->udts.total_count+1;
        rdim_shared->baked_udts.udts = push_array(arena, RDI_UDT, rdim_shared->baked_udts.udts_count);
      }
      if(lane_idx() == lane_from_task_idx(1))
      {
        rdim_shared->baked_udts.members_count = params->udts.total_member_count+1;
        rdim_shared->baked_udts.members = push_array(arena, RDI_Member, rdim_shared->baked_udts.members_count);
      }
      if(lane_idx() == lane_from_task_idx(2))
      {
        rdim_shared->baked_udts.enum_members_count = params->udts.total_enum_val_count+1;
        rdim_shared->baked_udts.enum_members = push_array(arena, RDI_EnumMember, rdim_shared->baked_udts.enum_members_count);
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
        U64 member_layout_off = rdim_shared->member_chunk_lane_offs[layout_slot_idx];
        U64 enum_val_layout_off = rdim_shared->enum_val_chunk_lane_offs[layout_slot_idx];
        for EachInRange(n_idx, range)
        {
          RDIM_UDT *src_udt = &n->v[n_idx];
          RDI_UDT *dst_udt = &rdim_shared->baked_udts.udts[n->base_idx + n_idx + 1];
          
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
              RDI_Member *dst_member = &rdim_shared->baked_udts.members[member_layout_off];
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
              RDI_EnumMember *dst_member = &rdim_shared->baked_udts.enum_members[enum_val_layout_off];
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
  U64 total_location_case_chunk_count = (params->scopes.chunk_count + params->procedures.chunk_count);
  ProfScope("compute lane location block layout")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      rdim_shared->location_case_chunk_lane_counts = push_array(arena, RDI_U64, lane_count() * total_location_case_chunk_count);
      rdim_shared->location_case_chunk_lane_offs = push_array(arena, RDI_U64, lane_count() * total_location_case_chunk_count);
    }
    lane_sync();
    
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
            rdim_shared->location_case_chunk_lane_counts[slot_idx] += local->location_cases.count;
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
          rdim_shared->location_case_chunk_lane_counts[slot_idx] += n->v[idx].location_cases.count;
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
          rdim_shared->location_case_chunk_lane_offs[slot_idx] = location_case_layout_off;
          location_case_layout_off += rdim_shared->location_case_chunk_lane_counts[slot_idx];
        }
        chunk_idx += 1;
      }
      for EachNode(n, RDIM_SymbolChunkNode, params->procedures.first)
      {
        for EachIndex(l_idx, lane_count())
        {
          U64 slot_idx = l_idx * total_location_case_chunk_count + chunk_idx;
          rdim_shared->location_case_chunk_lane_offs[slot_idx] = location_case_layout_off;
          location_case_layout_off += rdim_shared->location_case_chunk_lane_counts[slot_idx];
        }
        chunk_idx += 1;
      }
      rdim_shared->total_location_case_count = location_case_layout_off;
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake location blocks
  //
  ProfScope("bake location blocks")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      rdim_shared->baked_location_blocks.location_blocks_count = rdim_shared->total_location_case_count;
      rdim_shared->baked_location_blocks.location_blocks = push_array(arena, RDI_LocationBlock, rdim_shared->baked_location_blocks.location_blocks_count);
    }
    lane_sync();
    
    // rjf: wide fill from scopes
    U64 chunk_idx = 0;
    ProfScope("wide fill from scopes")
    {
      for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
      {
        U64 layout_slot_idx = lane_idx() * total_location_case_chunk_count + chunk_idx;
        U64 layout_off = rdim_shared->location_case_chunk_lane_offs[layout_slot_idx];
        Rng1U64 range = lane_range(n->count);
        for EachInRange(idx, range)
        {
          for EachNode(local, RDIM_Local, n->v[idx].first_local)
          {
            for EachNode(src, RDIM_LocationCase, local->location_cases.first)
            {
              RDI_LocationBlock *dst = &rdim_shared->baked_location_blocks.location_blocks[layout_off];
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
        U64 layout_off = rdim_shared->location_case_chunk_lane_offs[layout_slot_idx];
        Rng1U64 range = lane_range(n->count);
        for EachInRange(idx, range)
        {
          for EachNode(src, RDIM_LocationCase, n->v[idx].location_cases.first)
          {
            RDI_LocationBlock *dst = &rdim_shared->baked_location_blocks.location_blocks[layout_off];
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
  ProfScope("bake locations")
  {
    if(lane_idx() == 0)
    {
      rdim_shared->baked_locations.location_data_size = params->locations.total_encoded_size+1;
      rdim_shared->baked_locations.location_data = push_array(arena, RDI_U8, rdim_shared->baked_locations.location_data_size);
    }
    lane_sync();
    for EachNode(n, RDIM_LocationChunkNode, params->locations.first)
    {
      Rng1U64 range = lane_range(n->count);
      for EachInRange(n_idx, range)
      {
        RDIM_Location *loc = &n->v[n_idx];
        RDI_U8 *dst = &rdim_shared->baked_locations.location_data[n->base_encoding_off + loc->relative_encoding_off + 1];
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
  ProfScope("compute layout for scope sub-lists (locals / voffs)")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      rdim_shared->scope_local_chunk_lane_counts = push_array(arena, RDI_U64, lane_count() * params->scopes.chunk_count);
      rdim_shared->scope_local_chunk_lane_offs = push_array(arena, RDI_U64, lane_count() * params->scopes.chunk_count);
      rdim_shared->scope_voff_chunk_lane_counts = push_array(arena, RDI_U64, lane_count() * params->scopes.chunk_count);
      rdim_shared->scope_voff_chunk_lane_offs = push_array(arena, RDI_U64, lane_count() * params->scopes.chunk_count);
    }
    lane_sync();
    
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
        rdim_shared->scope_local_chunk_lane_counts[lane_idx()*params->scopes.chunk_count + chunk_idx] = num_locals_in_this_lane_and_node;
        rdim_shared->scope_voff_chunk_lane_counts[lane_idx()*params->scopes.chunk_count + chunk_idx] = num_voffs_in_this_lane_and_node;
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
          rdim_shared->scope_local_chunk_lane_offs[slot_idx] = local_layout_off;
          rdim_shared->scope_voff_chunk_lane_offs[slot_idx] = voff_layout_off;
          local_layout_off += rdim_shared->scope_local_chunk_lane_counts[slot_idx];
          voff_layout_off += rdim_shared->scope_voff_chunk_lane_counts[slot_idx];
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
  ProfScope("bake scopes")
  {
    //- rjf: setup outputs
    if(lane_idx() == lane_from_task_idx(0))
    {
      rdim_shared->baked_scopes.scopes_count = params->scopes.total_count+1;
      rdim_shared->baked_scopes.scopes = push_array(arena, RDI_Scope, rdim_shared->baked_scopes.scopes_count);
    }
    if(lane_idx() == lane_from_task_idx(1))
    {
      rdim_shared->baked_scopes.scope_voffs_count = params->scopes.scope_voff_count+1;
      rdim_shared->baked_scopes.scope_voffs = push_array(arena, RDI_U64, rdim_shared->baked_scopes.scope_voffs_count);
    }
    if(lane_idx() == lane_from_task_idx(2))
    {
      rdim_shared->baked_scopes.locals_count = params->scopes.local_count+1;
      rdim_shared->baked_scopes.locals = push_array(arena, RDI_Local, rdim_shared->baked_scopes.locals_count);
    }
    lane_sync();
    
    //- rjf: wide fill
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_ScopeChunkNode, params->scopes.first)
      {
        Rng1U64 range = lane_range(n->count);
        U64 scope_chunk_lane_slot_idx = lane_idx()*params->scopes.chunk_count + chunk_idx;
        U64 chunk_local_off = rdim_shared->scope_local_chunk_lane_offs[scope_chunk_lane_slot_idx];
        U64 chunk_voff_off = rdim_shared->scope_voff_chunk_lane_offs[scope_chunk_lane_slot_idx];
        U64 location_block_chunk_lane_slot_idx = lane_idx() * total_location_case_chunk_count + chunk_idx;
        U64 chunk_location_block_off = rdim_shared->location_case_chunk_lane_offs[location_block_chunk_lane_slot_idx];
        for EachInRange(n_idx, range)
        {
          U64 dst_idx = 1 + n->base_idx + n_idx;
          RDIM_Scope *src_scope = &n->v[n_idx];
          RDI_Scope *dst_scope = &rdim_shared->baked_scopes.scopes[dst_idx];
          
          //- rjf: fill voff ranges
          U64 voff_idx_first = chunk_voff_off;
          for EachNode(rng_n, RDIM_Rng1U64Node, src_scope->voff_ranges.first)
          {
            rdim_shared->baked_scopes.scope_voffs[chunk_voff_off+0] = rng_n->v.min;
            rdim_shared->baked_scopes.scope_voffs[chunk_voff_off+1] = rng_n->v.max;
            chunk_voff_off += 2;
          }
          U64 voff_idx_opl = chunk_voff_off;
          
          //- rjf: fill locals
          U64 local_idx_first = chunk_local_off;
          for EachNode(src_local, RDIM_Local, src_scope->first_local)
          {
            RDI_Local *dst_local = &rdim_shared->baked_scopes.locals[chunk_local_off];
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
  ProfScope("bake procedures")
  {
    if(lane_idx() == 0)
    {
      rdim_shared->baked_procedures.procedures_count = params->procedures.total_count+1;
      rdim_shared->baked_procedures.procedures = push_array(arena, RDI_Procedure, rdim_shared->baked_procedures.procedures_count);
    }
    lane_sync();
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_SymbolChunkNode, params->procedures.first)
      {
        U64 location_block_layout_slot_idx = lane_idx()*total_location_case_chunk_count + params->scopes.chunk_count + chunk_idx;
        U64 location_block_off = rdim_shared->location_case_chunk_lane_offs[location_block_layout_slot_idx];
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Symbol *src = &n->v[n_idx];
          RDI_Procedure *dst = &rdim_shared->baked_procedures.procedures[n->base_idx + n_idx + 1];
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
          else if(src->container_symbol != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_symbol); // TODO(rjf): @u64_to_u32
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
  ProfScope("compute layout for constant data")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      rdim_shared->constant_data_chunk_lane_counts = push_array(arena, U64, lane_count() * params->constants.chunk_count);
      rdim_shared->constant_data_chunk_lane_offs = push_array(arena, U64, lane_count() * params->constants.chunk_count);
    }
    lane_sync();
    
    // rjf: count
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_SymbolChunkNode, params->constants.first)
      {
        U64 slot_idx = lane_idx()*params->constants.chunk_count + chunk_idx;
        Rng1U64 range = lane_range(n->count);
        for EachInRange(idx, range)
        {
          rdim_shared->constant_data_chunk_lane_counts[slot_idx] += n->v[idx].value_data.size;
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
          rdim_shared->constant_data_chunk_lane_offs[slot_idx] = layout_off;
          layout_off += rdim_shared->constant_data_chunk_lane_counts[slot_idx];
        }
        chunk_idx += 1;
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake constants
  //
  ProfScope("bake constants")
  {
    // rjf: set up
    if(lane_idx() == lane_from_task_idx(0))
    {
      rdim_shared->baked_constants.constant_values_count = params->constants.total_count+1;
      rdim_shared->baked_constants.constant_values = push_array(arena, RDI_U32, rdim_shared->baked_constants.constant_values_count);
    }
    if(lane_idx() == lane_from_task_idx(1))
    {
      rdim_shared->baked_constants.constant_value_data_size = params->constants.total_value_data_size;
      rdim_shared->baked_constants.constant_value_data = push_array(arena, RDI_U8, rdim_shared->baked_constants.constant_value_data_size);
    }
    if(lane_idx() == lane_from_task_idx(2))
    {
      rdim_shared->baked_constants.constants_count = params->constants.total_count+1;
      rdim_shared->baked_constants.constants = push_array(arena, RDI_Constant, rdim_shared->baked_constants.constants_count);
    }
    lane_sync();
    
    // rjf: wide bake
    {
      U64 chunk_idx = 0;
      for EachNode(n, RDIM_SymbolChunkNode, params->constants.first)
      {
        U64 slot_idx = lane_idx()*params->constants.chunk_count + chunk_idx;
        U64 value_data_off = rdim_shared->constant_data_chunk_lane_offs[slot_idx];
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Symbol *src = &n->v[n_idx];
          RDI_Constant *dst = &rdim_shared->baked_constants.constants[1 + n->base_idx + n_idx];
          RDI_U32 *dst_value_off = &rdim_shared->baked_constants.constant_values[1 + n->base_idx + n_idx];
          RDI_U8 *dst_value_data = rdim_shared->baked_constants.constant_value_data + value_data_off;
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
  {
    //- rjf: setup outputs
    if(lane_idx() == lane_from_task_idx(0))
    {
      rdim_shared->baked_units.units_count = params->units.total_count+1;
      rdim_shared->baked_units.units = push_array(arena, RDI_Unit, rdim_shared->baked_units.units_count);
    }
    if(lane_idx() == lane_from_task_idx(1))
    {
      rdim_shared->baked_type_nodes.type_nodes_count = params->types.total_count+1;
      rdim_shared->baked_type_nodes.type_nodes = push_array(arena, RDI_TypeNode, rdim_shared->baked_type_nodes.type_nodes_count);
    }
    if(lane_idx() == lane_from_task_idx(2))
    {
      rdim_shared->baked_global_variables.global_variables_count = params->global_variables.total_count+1;
      rdim_shared->baked_global_variables.global_variables = push_array(arena, RDI_GlobalVariable, rdim_shared->baked_global_variables.global_variables_count);
    }
    if(lane_idx() == lane_from_task_idx(3))
    {
      rdim_shared->baked_thread_variables.thread_variables_count = params->thread_variables.total_count+1;
      rdim_shared->baked_thread_variables.thread_variables = push_array(arena, RDI_ThreadVariable, rdim_shared->baked_thread_variables.thread_variables_count);
    }
    if(lane_idx() == lane_from_task_idx(4))
    {
      rdim_shared->baked_inline_sites.inline_sites_count = params->inline_sites.total_count+1;
      rdim_shared->baked_inline_sites.inline_sites = push_array(arena, RDI_InlineSite, rdim_shared->baked_inline_sites.inline_sites_count);
    }
    lane_sync();
    
    //- rjf: bake units
    ProfScope("bake units")
    {
      for EachNode(n, RDIM_UnitChunkNode, params->units.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Unit *src = &n->v[n_idx];
          RDI_Unit *dst = &rdim_shared->baked_units.units[n->base_idx + n_idx + 1];
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
          RDI_TypeNode *dst = &rdim_shared->baked_type_nodes.type_nodes[n->base_idx + n_idx + 1];
          
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
          RDI_GlobalVariable *dst = &rdim_shared->baked_global_variables.global_variables[n->base_idx + n_idx + 1];
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
          else if(src->container_symbol != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_symbol); // TODO(rjf): @u64_to_u32
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
          RDI_ThreadVariable *dst = &rdim_shared->baked_thread_variables.thread_variables[n->base_idx + n_idx + 1];
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
          else if(src->container_symbol != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_symbol); // TODO(rjf): @u64_to_u32
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
          RDI_InlineSite *dst = &rdim_shared->baked_inline_sites.inline_sites[n->base_idx + n_idx + 1];
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
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage bake file paths
  //
  ProfScope("bake file paths")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      rdim_shared->baked_file_paths.nodes_count = path_tree->count;
      rdim_shared->baked_file_paths.nodes = push_array(arena, RDI_FilePathNode, rdim_shared->baked_file_paths.nodes_count);
      rdim_shared->baked_file_path_src_nodes = push_array(arena, RDIM_BakePathNode *, rdim_shared->baked_file_paths.nodes_count);
      {
        U64 idx = 0;
        for(RDIM_BakePathNode *n = path_tree->first; n != 0; n = n->next_order)
        {
          rdim_shared->baked_file_path_src_nodes[idx] = n;
          idx += 1;
        }
      }
    }
    lane_sync();
    
    // rjf: fill
    {
      Rng1U64 range = lane_range(rdim_shared->baked_file_paths.nodes_count);
      for EachInRange(idx, range)
      {
        RDIM_BakePathNode *src = rdim_shared->baked_file_path_src_nodes[idx];
        RDI_FilePathNode *dst = &rdim_shared->baked_file_paths.nodes[idx];
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
  ProfScope("do small final baking tasks")
  {
    if(lane_idx() == lane_from_task_idx(0)) ProfScope("bake top level info")
    {
      rdim_shared->baked_top_level_info.top_level_info.arch                     = params->top_level_info.arch;
      rdim_shared->baked_top_level_info.top_level_info.exe_name_string_idx      = rdim_bake_idx_from_string(bake_strings, params->top_level_info.exe_name);
      rdim_shared->baked_top_level_info.top_level_info.exe_hash                 = params->top_level_info.exe_hash;
      rdim_shared->baked_top_level_info.top_level_info.voff_max                 = params->top_level_info.voff_max;
      rdim_shared->baked_top_level_info.top_level_info.producer_name_string_idx = rdim_bake_idx_from_string(bake_strings, params->top_level_info.producer_name);
    }
    if(lane_idx() == lane_from_task_idx(1)) ProfScope("bake binary sections")
    {
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
      rdim_shared->baked_binary_sections.binary_sections = dst_base;
      rdim_shared->baked_binary_sections.binary_sections_count = dst_idx;
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: @rdim_bake_stage package results
  //
  RDIM_BakeResults result = {0};
  {
    result.top_level_info         = rdim_shared->baked_top_level_info;
    result.binary_sections        = rdim_shared->baked_binary_sections;
    result.units                  = rdim_shared->baked_units;
    result.unit_vmap              = rdim_shared->baked_unit_vmap;
    result.src_files              = rdim_shared->baked_src_files;
    result.checksums              = rdim_shared->baked_checksums;
    result.line_tables            = rdim_shared->baked_line_tables;
    result.type_nodes             = rdim_shared->baked_type_nodes;
    result.udts                   = rdim_shared->baked_udts;
    result.global_variables       = rdim_shared->baked_global_variables;
    result.global_vmap            = rdim_shared->baked_global_vmap;
    result.thread_variables       = rdim_shared->baked_thread_variables;
    result.constants              = rdim_shared->baked_constants;
    result.procedures             = rdim_shared->baked_procedures;
    result.scopes                 = rdim_shared->baked_scopes;
    result.inline_sites           = rdim_shared->baked_inline_sites;
    result.scope_vmap             = rdim_shared->baked_scope_vmap;
    result.top_level_name_maps    = rdim_shared->baked_top_level_name_maps;
    result.name_maps              = rdim_shared->baked_name_maps;
    result.file_paths             = rdim_shared->baked_file_paths;
    result.strings                = rdim_shared->baked_strings;
    result.idx_runs               = rdim_shared->baked_idx_runs;
    result.locations              = rdim_shared->baked_locations;
    result.location_blocks        = rdim_shared->baked_location_blocks;
  }
  
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
