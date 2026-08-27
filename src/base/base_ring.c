// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Ring Functions

internal Ring *
make_ring(Arena *arena, U64 size)
{
  Ring *ring = push_array(arena, Ring, 1);
  ring->size = size;
  ring->base = push_array(arena, U8, ring->size);
  return ring;
}

internal void *
ring_try_push(Ring *ring, U64 size)
{
  U64 bytes_unconsumed = (ring->write_pos - ring->read_pos);
  U64 bytes_available = ring->size - bytes_unconsumed;
  void *base = 0;
  if(bytes_available >= size)
  {
    base = ring->base + ring->write_pos%ring->size;
    ring->write_pos += size;
  }
  return base;
}

internal void *
ring_try_pop(Ring *ring, U64 size)
{
  U64 bytes_unconsumed = (ring->write_pos - ring->read_pos);
  void *base = 0;
  if(bytes_unconsumed >= size)
  {
    base = ring->base + ring->read_pos%ring->size;
    ring->read_pos += size;
  }
  return base;
}

internal B32
ring_try_write(Ring *ring, U64 size, void *ptr)
{
  B32 result = 0;
  void *dst = ring_try_push(ring, size);
  if(dst)
  {
    result = 1;
    MemoryCopy(dst, ptr, size);
  }
  return result;
}

internal B32
ring_try_read(Ring *ring, U64 size, void *ptr)
{
  B32 result = 0;
  void *src = ring_try_pop(ring, size);
  if(src)
  {
    result = 1;
    MemoryCopy(ptr, src, size);
  }
  return result;
}

////////////////////////////////
//~ rjf: Guarded Ring Functions

internal GuardedRing *
guarded_ring_alloc(Arena *arena, U64 size)
{
  ProfBeginFunction();
  GuardedRing *gr = push_array(arena, GuardedRing, 1);
  gr->ring = make_ring(arena, size);
  gr->mutex = mutex_alloc();
  gr->cv = cond_var_alloc();
  ProfEnd();
  return gr;
}

internal void
guarded_ring_release(GuardedRing *ring)
{
  ProfBeginFunction();
  mutex_release(ring->mutex);
  cond_var_release(ring->cv);
  ProfEnd();
}

internal RingGuard
guarded_ring_open(GuardedRing *ring)
{
  RingGuard guard = {ring};
  mutex_take(ring->mutex);
  return guard;
}

internal void
guarded_ring_close(RingGuard *guard)
{
  mutex_drop(guard->r->mutex);
  cond_var_broadcast(guard->r->cv);
}

internal void *
guarded_ring_try_push(RingGuard *guard, U64 size)
{
  void *result = ring_try_push(guard->r->ring, size);
  return result;
}

internal void *
guarded_ring_try_pop(RingGuard *guard, U64 size)
{
  void *result = ring_try_pop(guard->r->ring, size);
  return result;
}

internal B32
guarded_ring_try_write(RingGuard *guard, U64 size, void *ptr)
{
  B32 result = ring_try_write(guard->r->ring, size, ptr);
  return result;
}

internal B32
guarded_ring_try_read(RingGuard *guard, U64 size, void *ptr)
{
  B32 result = ring_try_read(guard->r->ring, size, ptr);
  return result;
}

internal void *
guarded_ring_push_or_wait(RingGuard *guard, U64 size, U64 endt_us)
{
  void *result = 0;
  for(;!result;)
  {
    result = guarded_ring_try_push(guard, size);
    if(now_time_us() >= endt_us)
    {
      break;
    }
    if(!result)
    {
      cond_var_wait(guard->r->cv, guard->r->mutex, endt_us);
    }
  }
  return result;
}

internal void *
guarded_ring_pop_or_wait(RingGuard *guard, U64 size, U64 endt_us)
{
  void *result = 0;
  for(;!result;)
  {
    result = guarded_ring_try_pop(guard, size);
    if(now_time_us() >= endt_us)
    {
      break;
    }
    if(!result)
    {
      cond_var_wait(guard->r->cv, guard->r->mutex, endt_us);
    }
  }
  return result;
}

internal B32
guarded_ring_write_or_wait(RingGuard *guard, U64 size, void *ptr, U64 endt_us)
{
  void *dst = guarded_ring_push_or_wait(guard, size, endt_us);
  B32 result = !!dst;
  if(dst)
  {
    MemoryCopy(dst, ptr, size);
  }
  return result;
}

internal B32
guarded_ring_read_or_wait(RingGuard *guard, U64 size, void *ptr, U64 endt_us)
{
  void *src = guarded_ring_pop_or_wait(guard, size, endt_us);
  B32 result = !!src;
  if(src)
  {
    MemoryCopy(ptr, src, size);
  }
  return result;
}
