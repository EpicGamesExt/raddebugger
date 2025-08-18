// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Globals

C_LINKAGE thread_static TCTX* tctx_thread_local;
#if !BUILD_SUPPLEMENTARY_UNIT
C_LINKAGE thread_static TCTX* tctx_thread_local = 0;
#endif

////////////////////////////////
//~ rjf: Thread Context Functions

//- rjf: thread-context allocation & selection

internal TCTX *
tctx_alloc(void)
{
  Arena *arena = arena_alloc();
  TCTX *tctx = push_array(arena, TCTX, 1);
  tctx->arenas[0] = arena;
  tctx->arenas[1] = arena_alloc();
  tctx->lane_ctx.lane_count = 1;
  return tctx;
}

internal void
tctx_release(TCTX *tctx)
{
  arena_release(tctx->arenas[1]);
  arena_release(tctx->arenas[0]);
}

internal void
tctx_select(TCTX *tctx)
{
  tctx_thread_local = tctx;
}

internal TCTX *
tctx_selected(void)
{
  return tctx_thread_local;
}

//- rjf: scratch arenas

internal Arena *
tctx_get_scratch(Arena **conflicts, U64 count)
{
  TCTX *tctx = tctx_selected();
  Arena *result = 0;
  Arena **arena_ptr = tctx->arenas;
  for(U64 i = 0; i < ArrayCount(tctx->arenas); i += 1, arena_ptr += 1)
  {
    Arena **conflict_ptr = conflicts;
    B32 has_conflict = 0;
    for(U64 j = 0; j < count; j += 1, conflict_ptr += 1)
    {
      if(*arena_ptr == *conflict_ptr)
      {
        has_conflict = 1;
        break;
      }
    }
    if(!has_conflict)
    {
      result = *arena_ptr;
      break;
    }
  }
  return result;
}

//- rjf: lane metadata

internal void
tctx_set_lane_ctx(LaneCtx lane_ctx)
{
  TCTX *tctx = tctx_selected();
  tctx->lane_ctx = lane_ctx;
}

internal void
tctx_lane_barrier_wait(void)
{
  ProfBeginFunction();
  ProfColor(0xff0000ff);
  TCTX *tctx = tctx_selected();
  os_barrier_wait(tctx->lane_ctx.barrier);
  ProfEnd();
}

internal Rng1U64
tctx_lane_idx_range_from_count(U64 count)
{
  U64 main_idxes_per_lane = count/lane_count();
  U64 leftover_idxes_count = count - main_idxes_per_lane*lane_count();
  U64 leftover_idxes_before_this_lane_count = Min(lane_idx(), leftover_idxes_count);
  U64 lane_base_idx = lane_idx()*main_idxes_per_lane + leftover_idxes_before_this_lane_count;
  U64 lane_base_idx__clamped = Min(lane_base_idx, count);
  U64 lane_opl_idx = lane_base_idx__clamped + main_idxes_per_lane + ((lane_idx() < leftover_idxes_count) ? 1 : 0);
  U64 lane_opl_idx__clamped = Min(lane_opl_idx, count);
  Rng1U64 result = r1u64(lane_base_idx__clamped, lane_opl_idx__clamped);
  return result;
}

//- rjf: thread names

internal void
tctx_set_thread_name(String8 string)
{
  TCTX *tctx = tctx_selected();
  U64 size = ClampTop(string.size, sizeof(tctx->thread_name));
  MemoryCopy(tctx->thread_name, string.str, size);
  tctx->thread_name_size = size;
}

internal String8
tctx_get_thread_name(void)
{
  TCTX *tctx = tctx_selected();
  String8 result = str8(tctx->thread_name, tctx->thread_name_size);
  return result;
}

//- rjf: thread source-locations

internal void
tctx_write_srcloc(char *file_name, U64 line_number)
{
  TCTX *tctx = tctx_selected();
  tctx->file_name = file_name;
  tctx->line_number = line_number;
}

internal void
tctx_read_srcloc(char **file_name, U64 *line_number)
{
  TCTX *tctx = tctx_selected();
  *file_name = tctx->file_name;
  *line_number = tctx->line_number;
}
