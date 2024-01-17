// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// Implementation

internal Arena *
arena_alloc__sized(U64 init_res, U64 init_cmt)
{
  ProfBeginFunction();
  Assert(ARENA_HEADER_SIZE < init_cmt && init_cmt <= init_res);
  
  void *memory;
  U64 res;
  U64 cmt;
  
  B32 large_pages = os_large_pages_enabled();
  if (large_pages) {
    U64 page_size = os_large_page_size();
    res = AlignPow2(init_res, page_size);
    
#if OS_WINDOWS
    cmt = res;
#else
    cmt = AlignPow2(init_cmt, page_size);
#endif
    
    memory = os_reserve_large(res);
    if (!os_commit_large(memory, cmt)) {
      memory = 0;
      os_release(memory, res);
    }
  } else {
    U64 page_size = os_page_size();
    res = AlignPow2(init_res, page_size);
    cmt = AlignPow2(init_cmt, page_size);
    
    memory = os_reserve(res);
    if (!os_commit(memory, cmt)) {
      memory = 0;
      os_release(memory, res);
    }
  }
  Assert(memory);
  
  AsanPoisonMemoryRegion(memory, cmt);
  AsanUnpoisonMemoryRegion(memory, ARENA_HEADER_SIZE);
  
  Arena *arena = (Arena*)memory;
  if (arena) {
    arena->prev        = 0;
    arena->current     = arena;
    arena->base_pos    = 0;
    arena->pos         = ARENA_HEADER_SIZE;
    arena->cmt         = cmt;
    arena->res         = res;
    arena->align       = 8;
#if ENABLE_DEV
    arena->dev         = 0;
#endif
    arena->grow        = 1;
    arena->large_pages = large_pages;
  }
  
  ProfEnd();
  return arena;
}

internal Arena *
arena_alloc(void)
{
  ProfBeginFunction();
  
  U64 init_res, init_cmt;
  if (os_large_pages_enabled()) {
    init_res = ARENA_RESERVE_SIZE_LARGE_PAGES;
    init_cmt = ARENA_COMMIT_SIZE_LARGE_PAGES;
  } else {
    init_res = ARENA_RESERVE_SIZE;
    init_cmt = ARENA_COMMIT_SIZE;
  }
  
  Arena *arena = arena_alloc__sized(init_res, init_cmt);
  
  ProfEnd();
  return arena;
}

internal void
arena_release(Arena *arena)
{
  for (Arena *node = arena->current, *prev = 0; node != 0; node = prev) {
    prev = node->prev;
    os_release(node, node->res);
  }
}

internal U64
arena_huge_push_threshold(void)
{
  U64 reserve_size = os_large_pages_enabled() ? ARENA_RESERVE_SIZE_LARGE_PAGES : ARENA_RESERVE_SIZE;
  U64 threshold = (reserve_size - ARENA_HEADER_SIZE) / 2 + 1;
  return threshold;
}

internal void *
arena_push__impl(Arena *arena, U64 size)
{
  Arena *current = arena->current;
  U64    pos_mem = AlignPow2(current->pos, arena->align);
  U64    pos_new = pos_mem + size;
  
  if (current->res < pos_new && arena->grow) {
    Arena *new_block;
    
    // normal growth path
    if (size < arena_huge_push_threshold()) {
      new_block = arena_alloc();
    }
    // huge growth path
    else {
      U64 new_block_size = size + ARENA_HEADER_SIZE;
      new_block = arena_alloc__sized(new_block_size, new_block_size);
    }
    
    if (new_block) {
      new_block->base_pos = current->base_pos + current->res;
      SLLStackPush_N(arena->current, new_block, prev);
      
      current = new_block;
      pos_mem = AlignPow2(current->pos, current->align);
      pos_new = pos_mem + size;
    }
  }
  
  if (current->cmt < pos_new) {
    U64 cmt_new_aligned, cmt_new_clamped, cmt_new_size;
    B32 is_cmt_ok;
    
    if (current->large_pages) {
      cmt_new_aligned = AlignPow2(pos_new, ARENA_COMMIT_SIZE_LARGE_PAGES);
      cmt_new_clamped = ClampTop(cmt_new_aligned, current->res);
      cmt_new_size    = cmt_new_clamped - current->cmt;
      is_cmt_ok       = os_commit_large((U8*)current + current->cmt, cmt_new_size);
    } else {
      cmt_new_aligned = AlignPow2(pos_new, ARENA_COMMIT_SIZE);
      cmt_new_clamped = ClampTop(cmt_new_aligned, current->res);
      cmt_new_size    = cmt_new_clamped - current->cmt;
      is_cmt_ok       = os_commit((U8*)current + current->cmt, cmt_new_size);
    }
    Assert(is_cmt_ok);
    
    if (is_cmt_ok) {
      current->cmt = cmt_new_clamped;
    }
  }
  
  void *memory = 0;
  
  if (current->cmt >= pos_new) {
    memory = (U8*)current + pos_mem;
    current->pos = pos_new;
    AsanUnpoisonMemoryRegion(memory, size);
  }
  
#if OS_FEATURE_GRAPHICAL
  if(Unlikely(memory == 0))
  {
    os_graphical_message(1, str8_lit("Fatal Allocation Failure"), str8_lit("Unexpected memory allocation failure."));
    os_exit_process(1);
  }
#endif
  
  return memory;
}

internal U64
arena_pos(Arena *arena)
{
  Arena *current = arena->current;
  U64 pos = current->base_pos + current->pos;
  return pos;
}

internal void
arena_pop_to(Arena *arena, U64 big_pos_unclamped)
{
  U64 big_pos = ClampBot(ARENA_HEADER_SIZE, big_pos_unclamped);
  
  // unroll the chain
  Arena *current = arena->current;
  for (Arena *prev = 0; current->base_pos >= big_pos; current = prev) {
    prev = current->prev;
    os_release(current, current->res);
  }
  AssertAlways(current);
  arena->current = current;
  
  // compute arena-relative position
  U64 new_pos = big_pos - current->base_pos;
  AssertAlways(new_pos <= current->pos);
  
  // poison popped memory block
  AsanPoisonMemoryRegion((U8*)current + new_pos, (current->pos - new_pos));
  
  // update position
  current->pos = new_pos;
}

internal void
arena_absorb(Arena *arena, Arena *sub)
{
#if ENABLE_DEV
  arena_annotate_absorb__dev(arena, sub);
#endif
  
  // base adjustment
  Arena *current = arena->current;
  U64 base_adjust = current->base_pos + current->res;
  for (Arena *node = sub->current; node != 0; node = node->prev) {
    node->base_pos += base_adjust;
  }
  
  // attach sub to arena
  sub->prev = arena->current;
  arena->current = sub->current;
  sub->current = sub;
}

////////////////////////////////
// Wrappers

internal void *
arena_push(Arena *arena, U64 size)
{
  void *memory = arena_push__impl(arena, size);
#if ENABLE_DEV
  arena_annotate_push__dev(arena, size, memory);
#endif
  return memory;
}

internal void *
arena_push_contiguous(Arena *arena, U64 size)
{
  B32 restore = arena->grow;
  arena->grow = 0;
  void *memory = arena_push(arena, size);
  arena->grow = restore;
#if ENABLE_DEV
  arena_annotate_push__dev(arena, size, memory);
#endif
  return memory;
}

internal void
arena_push_align(Arena *arena, U64 align)
{
  Assert(IsPow2(align));
  U64   amt = AlignPadPow2(arena->pos, align);
  void *ptr = arena_push(arena, amt);
  MemoryZero(ptr, amt);
}

internal void
arena_put_back(Arena *arena, U64 amt)
{
  U64 pos_old = arena_pos(arena);
  U64 pos_new = pos_old;
  if (amt < pos_old) {
    pos_new = pos_old - amt;
  }
  arena_pop_to(arena, pos_new);
}

internal void
arena_clear(Arena *arena)
{
  arena_pop_to(arena, 0);
}

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

////////////////////////////////
//~ NOTE(allen): "Mini-Arena" Helper

internal B32
ensure_commit(void **cmtptr, void *pos, U64 cmt_block_size){
  B32 result = 0;
  U8 *cmt = (U8*)*cmtptr;
  if (cmt < (U8*)pos){
    U64 cmt_size_raw = (U8*)pos - cmt;
    U64 cmt_size = AlignPow2(cmt_size_raw, cmt_block_size);
    if (os_commit(cmt, cmt_size)){
      *cmtptr = cmt + cmt_size;
      result = 1;
    }
  }
  else{
    result = 1;
  }
  return(result);
}

