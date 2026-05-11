// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Global Arena Table

#if ARENA_TABLE_DEBUG
typedef struct ArenaTableNode ArenaTableNode;
struct ArenaTableNode
{
  ArenaTableNode *next;
  Arena *arena;
};
global U64 arena_table_take_init = 0;
global U64 arena_table_inited = 0;
global U64 arena_table_lock = 0;
global ArenaTableNode *arena_table = 0;
global U64 arena_table_count = 0;
global U64 arena_table_cap = 0;
global ArenaTableNode *free_arena_table_node = 0;
#endif

////////////////////////////////
//~ rjf: Arena Functions

//- rjf: arena creation/destruction

internal Arena *
arena_alloc_(ArenaParams *params)
{
  U64 reserve_size = params->reserve_size;
  U64 commit_size  = params->commit_size;
  
  // rjf: reserve/commit initial block
  void *base = params->optional_backing_buffer;
  if(base == 0)
  {
    // rjf: round up reserve/commit sizes
    if(params->flags & ArenaFlag_LargePages)
    {
      reserve_size = AlignPow2(reserve_size, get_system_info()->large_page_size);
      commit_size  = AlignPow2(commit_size,  get_system_info()->large_page_size);
    }
    else
    {
      reserve_size = AlignPow2(reserve_size, get_system_info()->page_size);
      commit_size  = AlignPow2(commit_size,  get_system_info()->page_size);
    }
    
    if(params->flags & ArenaFlag_LargePages)
    {
      base = reserve_memory_large(reserve_size);
      commit_memory_large(base, commit_size);
    }
    else
    {
      base = reserve_memory(reserve_size);
      commit_memory(base, commit_size);
    }
    AsanPoisonMemoryRegion(base, commit_size);
    // TODO(rjf): we need to reintroduce this later when we have the ability to remove annotations...
    // raddbg_annotate_vaddr_range(base, reserve_size, "arena %s:%i", params->allocation_site_file, params->allocation_site_line);
  }
  else
  {
    AsanPoisonMemoryRegion(base, params->reserve_size);
  }
  
  // rjf: panic on arena creation failure
#if OS_FEATURE_GRAPHICAL
  if(Unlikely(base == 0))
  {
    wm_graphical_message(1, str8_lit("Fatal Allocation Failure"), str8_lit("Unexpected memory allocation failure."));
    abort_self(1);
  }
#endif
  
  // rjf: extract arena header & fill
  AsanUnpoisonMemoryRegion(base, ARENA_HEADER_SIZE);
  Arena *arena                = base;
  arena->current              = arena;
  arena->flags                = params->flags;
  arena->cmt_size             = params->commit_size;
  arena->res_size             = params->reserve_size;
  arena->base_pos             = 0;
  arena->pos                  = ARENA_HEADER_SIZE;
  arena->cmt                  = commit_size;
  arena->res                  = reserve_size;
  arena->allocation_site_file = params->allocation_site_file;
  arena->allocation_site_line = params->allocation_site_line;
  arena->name                 = params->name;
#if ARENA_FREE_LIST
  arena->free_last = 0;
#endif
  
  // rjf: store in global arena table
#if ARENA_TABLE_DEBUG
  if(ins_atomic_u64_eval_cond_assign(&arena_table_take_init, 1, 0) == 0)
  {
    arena_table = reserve_memory(GB(256));
    arena_table_cap = 4096;
    commit_memory(arena_table, arena_table_cap * sizeof(arena_table[0]));
    ins_atomic_u64_inc_eval(&arena_table_inited);
  }
  for(;!ins_atomic_u64_eval(&arena_table_inited);) {}
  for(;;)
  {
    B32 got_lock = (ins_atomic_u64_eval_cond_assign(&arena_table_lock, 1, 0) == 0);
    if(got_lock)
    {
      ArenaTableNode *node = free_arena_table_node;
      if(node != 0)
      {
        SLLStackPop(free_arena_table_node);
      }
      else
      {
        node = &arena_table[arena_table_count];
        if(arena_table_count >= arena_table_cap)
        {
          U64 arena_table_cap__pre_grow = arena_table_cap;
          arena_table_cap *= 2;
          U64 arena_table_cap__post_grow = arena_table_cap;
          commit_memory(arena_table + arena_table_cap__pre_grow, sizeof(arena_table[0]) * (arena_table_cap__post_grow - arena_table_cap__pre_grow));
        }
        arena_table_count += 1;
      }
      node->arena = arena;
      arena->table_node = node;
      ins_atomic_u64_eval_assign(&arena_table_lock, 0);
      break;
    }
  }
#endif
  
  return arena;
}

internal void
arena_release(Arena *arena)
{
#if PROFILE_TELEMETRY
  {
    Arena *base_arena = arena;
    while (base_arena->prev) base_arena = base_arena->prev;
    tmPlot(0, TM_PLOT_UNITS_MEMORY, TM_PLOT_DRAW_LINE, 0, "%s/%p", base_arena->name, base_arena);
  }
#endif
  
#if ARENA_TABLE_DEBUG
  for(Arena *n = arena->current; n != 0; n = n->prev)
  {
    for(;;)
    {
      B32 got_lock = (ins_atomic_u64_eval_cond_assign(&arena_table_lock, 1, 0) == 0);
      if(got_lock)
      {
        ArenaTableNode *table_node = n->table_node;
        MemoryZeroStruct(table_node);
        SLLStackPush(free_arena_table_node, table_node);
        ins_atomic_u64_eval_assign(&arena_table_lock, 0);
        break;
      }
    }
  }
#endif
  
  for(Arena *n = arena->current, *prev = 0; n != 0; n = prev)
  {
    prev = n->prev;
    release_memory(n, n->res);
  }
}

//- rjf: arena push/pop core functions

internal void *
arena_push(Arena *arena, U64 size, U64 align, B32 zero)
{
  Arena *current = arena->current;
  U64 pos_pre = AlignPow2(current->pos, align);
  U64 pos_pst = pos_pre + size;
  
  // TODO: Newly allocated arenas already have zeroed commited pages,
  //       so this unnecessarily zeroes them again.
  //
  // rjf: compute the size we need to zero
  U64 size_to_zero = 0;
  if(zero)
  {
    size_to_zero = Min(current->cmt, pos_pst) - pos_pre;
  }
  
  // rjf: chain, if needed
  if(current->res < pos_pst && !(arena->flags & ArenaFlag_NoChain))
  {
    Arena *new_block = 0;
    
#if ARENA_FREE_LIST
    {
      Arena *prev_block;
      for(new_block = arena->free_last, prev_block = 0; new_block != 0; prev_block = new_block, new_block = new_block->prev)
      {
        if(new_block->res >= AlignPow2(new_block->pos, align) + size)
        {
          if(prev_block)
          {
            prev_block->prev = new_block->prev;
          }
          else
          {
            arena->free_last = new_block->prev;
          }
          break;
        }
      }
    }
#endif
    
    if(new_block == 0)
    {
      U64 res_size = current->res_size;
      U64 cmt_size = current->cmt_size;
      if(size + ARENA_HEADER_SIZE > res_size)
      {
        res_size = AlignPow2(size + ARENA_HEADER_SIZE, align);
        cmt_size = AlignPow2(size + ARENA_HEADER_SIZE, align);
      }
      new_block = arena_alloc(.reserve_size = res_size,
                              .commit_size  = cmt_size,
                              .flags        = current->flags,
                              .allocation_site_file = current->allocation_site_file,
                              .allocation_site_line = current->allocation_site_line);
      
      size_to_zero = 0;
    }
    else
    {
      size_to_zero = size;
    }
    
    new_block->base_pos = current->base_pos + current->res;
    SLLStackPush_N(arena->current, new_block, prev);
    
    current = new_block;
    pos_pre = AlignPow2(current->pos, align);
    pos_pst = pos_pre + size;
  }
  
  // rjf: commit new pages, if needed
  if(current->cmt < pos_pst)
  {
    U64 cmt_pst_aligned = pos_pst + current->cmt_size-1;
    cmt_pst_aligned -= cmt_pst_aligned%current->cmt_size;
    U64 cmt_pst_clamped = ClampTop(cmt_pst_aligned, current->res);
    U64 cmt_size = cmt_pst_clamped - current->cmt;
    U8 *cmt_ptr = (U8 *)current + current->cmt;
    if(current->flags & ArenaFlag_LargePages)
    {
      commit_memory_large(cmt_ptr, cmt_size);
    }
    else
    {
      commit_memory(cmt_ptr, cmt_size);
    }
    AsanPoisonMemoryRegion(cmt_ptr, cmt_size);
    current->cmt = cmt_pst_clamped;
  }
  
  // rjf: push onto current block
  void *result = 0;
  if(current->cmt >= pos_pst)
  {
    result = (U8 *)current+pos_pre;
    current->pos = pos_pst;
    AsanUnpoisonMemoryRegion(result, size);
    MemoryZero(result, size_to_zero);
  }
  
#if PROFILE_TELEMETRY
  if(size > KB(1))
  {
    Arena *base_arena = arena;
    while (base_arena->prev) base_arena = base_arena->prev;
    tmPlot(0, TM_PLOT_UNITS_MEMORY, TM_PLOT_DRAW_LINE, (double)(current->base_pos + current->pos) / 1024.0 / 1024.0, "%s/%p", base_arena->name, base_arena);
  }
#endif
  
  // rjf: panic on failure
#if OS_FEATURE_GRAPHICAL
  if(Unlikely(result == 0))
  {
    wm_graphical_message(1, str8_lit("Fatal Allocation Failure"), str8_lit("Unexpected memory allocation failure."));
    abort_self(1);
  }
#endif
  
  return result;
}

internal U64
arena_pos(Arena *arena)
{
  Arena *current = arena->current;
  U64 pos = current->base_pos + current->pos;
  return pos;
}

internal void
arena_pop_to(Arena *arena, U64 pos)
{
  U64 big_pos = ClampBot(ARENA_HEADER_SIZE, pos);
  Arena *current = arena->current;
  
#if ARENA_FREE_LIST
  for(Arena *prev = 0; current->base_pos >= big_pos; current = prev)
  {
    prev = current->prev;
    current->pos = ARENA_HEADER_SIZE;
    SLLStackPush_N(arena->free_last, current, prev);
    AsanPoisonMemoryRegion((U8*)current + ARENA_HEADER_SIZE, current->res - ARENA_HEADER_SIZE);
  }
#else
  for(Arena *prev = 0; current->base_pos >= big_pos; current = prev)
  {
    prev = current->prev;
    release_memory(current, current->res);
  }
#endif
  arena->current = current;
  U64 new_pos = big_pos - current->base_pos;
  AssertAlways(new_pos <= current->pos);
  AsanPoisonMemoryRegion((U8*)current + new_pos, (current->pos - new_pos));
  current->pos = new_pos;
  
#if PROFILE_TELEMETRY
  if((pos - (new_pos + current->base_pos)) > KB(1))
  {
    Arena *base_arena = arena;
    while (base_arena->prev) base_arena = base_arena->prev;
    tmPlot(0, TM_PLOT_UNITS_MEMORY, TM_PLOT_DRAW_LINE, (double)(current->base_pos + current->pos) / 1024.0 / 1024.0, "%s/%p", base_arena->name, base_arena);
  }
#endif
  
}

//- rjf: arena push/pop helpers

internal void
arena_clear(Arena *arena)
{
  arena_pop_to(arena, 0);
}

internal void
arena_pop(Arena *arena, U64 amt)
{
  U64 pos_old = arena_pos(arena);
  U64 pos_new = pos_old;
  if(amt < pos_old)
  {
    pos_new = pos_old - amt;
  }
  arena_pop_to(arena, pos_new);
}

//- rjf: temporary arena scopes

internal Temp
temp_begin(Arena *arena)
{
  U64 pos = arena_pos(arena);
  Temp temp = {arena, pos};
  return temp;
}

internal void
temp_end(Temp temp)
{
  arena_pop_to(temp.arena, temp.pos);
}
