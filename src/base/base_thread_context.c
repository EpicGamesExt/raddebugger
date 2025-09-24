// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Globals

C_LINKAGE thread_static TCTX *tctx_thread_local;
#if !BUILD_SUPPLEMENTARY_UNIT
C_LINKAGE thread_static TCTX *tctx_thread_local = 0;
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

internal LaneCtx
tctx_set_lane_ctx(LaneCtx lane_ctx)
{
  TCTX *tctx = tctx_selected();
  LaneCtx restore = tctx->lane_ctx;
  tctx->lane_ctx = lane_ctx;
  return restore;
}

internal void
tctx_lane_barrier_wait(void *broadcast_ptr, U64 broadcast_size, U64 broadcast_src_lane_idx)
{
  ProfBeginFunction();
  ProfColor(0x00000ff);
  TCTX *tctx = tctx_selected();
  
  // rjf: doing broadcast -> copy to broadcast memory on source lane
  U64 broadcast_size_clamped = ClampTop(broadcast_size, sizeof(tctx->lane_ctx.broadcast_memory[0]));
  if(broadcast_ptr != 0 && lane_idx() == broadcast_src_lane_idx)
  {
    MemoryCopy(tctx->lane_ctx.broadcast_memory, broadcast_ptr, broadcast_size_clamped);
  }
  
  // rjf: all cases: barrier
  os_barrier_wait(tctx->lane_ctx.barrier);
  
  // rjf: doing broadcast -> copy from broadcast memory on destination lanes
  if(broadcast_ptr != 0 && lane_idx() != broadcast_src_lane_idx)
  {
    MemoryCopy(broadcast_ptr, tctx->lane_ctx.broadcast_memory, broadcast_size_clamped);
  }
  
  // rjf: doing broadcast -> barrier on all lanes
  if(broadcast_ptr != 0)
  {
    os_barrier_wait(tctx->lane_ctx.barrier);
  }
  
  ProfEnd();
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

////////////////////////////////
//~ rjf: Touch Scope Functions

internal Access *
access_open(void)
{
  if(tctx_thread_local->access_arena == 0)
  {
    tctx_thread_local->access_arena = arena_alloc();
  }
  Access *access = tctx_thread_local->free_access;
  if(access != 0)
  {
    SLLStackPop(tctx_thread_local->free_access);
  }
  else
  {
    access = push_array_no_zero(tctx_thread_local->access_arena, Access, 1);
  }
  MemoryZeroStruct(access);
  return access;
}

internal void
access_close(Access *access)
{
  for(Touch *touch = access->top_touch, *next = 0; touch != 0; touch = next)
  {
    next = touch->next;
    ins_atomic_u64_dec_eval(&touch->pt->access_refcount);
    if(touch->cv.u64[0] != 0) { cond_var_broadcast(touch->cv); }
    SLLStackPush(tctx_thread_local->free_touch, touch);
  }
  SLLStackPush(tctx_thread_local->free_access, access);
}

internal void
access_touch(Access *access, AccessPt *pt, CondVar cv)
{
  ins_atomic_u64_inc_eval(&pt->access_refcount);
  ins_atomic_u64_eval_assign(&pt->last_time_touched_us, os_now_microseconds());
  ins_atomic_u64_eval_assign(&pt->last_update_idx_touched, update_tick_idx());
  Touch *touch = tctx_thread_local->free_touch;
  if(touch != 0)
  {
    SLLStackPop(tctx_thread_local->free_touch);
  }
  else
  {
    touch = push_array_no_zero(tctx_thread_local->access_arena, Touch, 1);
  }
  MemoryZeroStruct(touch);
  SLLStackPush(access->top_touch, touch);
  touch->cv = cv;
  touch->pt = pt;
}

//- rjf: access points

internal B32
access_pt_is_expired(AccessPt *pt)
{
  U64 access_refcount = ins_atomic_u64_eval(&pt->access_refcount);
  U64 last_time_touched_us = ins_atomic_u64_eval(&pt->last_time_touched_us);
  U64 last_update_idx_touched = ins_atomic_u64_eval(&pt->last_update_idx_touched);
  B32 result = (access_refcount == 0 &&
                last_time_touched_us + 2000000 < os_now_microseconds() &&
                last_update_idx_touched + 10 < update_tick_idx());
  return result;
}
