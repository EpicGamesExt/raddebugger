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

internal B32
ring_try_write(Ring *ring, U64 size, void *ptr)
{
  U64 bytes_unconsumed = (ring->write_pos - ring->read_pos);
  U64 bytes_available = ring->size - bytes_unconsumed;
  B32 result = 0;
  if(bytes_available >= size)
  {
    result = 1;
    ring->write_pos += wrapped_write(ring->base, ring->size, ring->write_pos, ptr, size);
  }
  return result;
}

internal B32
ring_try_read(Ring *ring, U64 size, void *ptr)
{
  U64 bytes_unconsumed = (ring->write_pos - ring->read_pos);
  B32 result = 0;
  if(bytes_unconsumed >= size)
  {
    result = 1;
    ring->read_pos += wrapped_read(ring->base, ring->size, ring->read_pos, ptr, size);
  }
  return result;
}

////////////////////////////////
//~ rjf: Guarded Ring Functions

internal GuardedRing *
guarded_ring_alloc(Arena *arena, U64 size)
{
  GuardedRing *gr = push_array(arena, GuardedRing, 1);
  gr->ring = make_ring(arena, size);
  gr->mutex = mutex_alloc();
  gr->cv = cond_var_alloc();
  return gr;
}

internal void
guarded_ring_release(GuardedRing *ring)
{
  mutex_release(ring->mutex);
  cond_var_release(ring->cv);
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
}

internal B32
guarded_ring_try_write(RingGuard *guard, U64 size, void *ptr)
{
  B32 result = ring_try_write(guard->r->ring, size, ptr);
  if(result)
  {
    cond_var_broadcast(guard->r->cv);
  }
  return result;
}

internal B32
guarded_ring_try_read(RingGuard *guard, U64 size, void *ptr)
{
  B32 result = ring_try_read(guard->r->ring, size, ptr);
  if(result)
  {
    cond_var_broadcast(guard->r->cv);
  }
  return result;
}

internal B32
guarded_ring_write_or_wait(RingGuard *guard, U64 size, void *ptr, U64 endt_us)
{
  B32 write_good = 0;
  for(;!write_good;)
  {
    write_good = guarded_ring_try_write(guard, size, ptr);
    if(now_time_us() >= endt_us)
    {
      break;
    }
    if(!write_good)
    {
      cond_var_wait(guard->r->cv, guard->r->mutex, endt_us);
    }
  }
  return write_good;
}

internal B32
guarded_ring_read_or_wait(RingGuard *guard, U64 size, void *ptr, U64 endt_us)
{
  B32 read_good = 0;
  for(;!read_good;)
  {
    read_good = guarded_ring_try_read(guard, size, ptr);
    if(now_time_us() >= endt_us)
    {
      break;
    }
    if(!read_good)
    {
      cond_var_wait(guard->r->cv, guard->r->mutex, endt_us);
    }
  }
  return read_good;
}
