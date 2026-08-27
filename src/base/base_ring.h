// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_RING_H
#define BASE_RING_H

typedef struct Ring Ring;
struct Ring
{
  U8 *base;
  U64 size;
  U64 write_pos;
  U64 read_pos;
};

typedef struct GuardedRing GuardedRing;
struct GuardedRing
{
  Ring *ring;
  Mutex mutex;
  CondVar cv;
};

typedef struct RingGuard RingGuard;
struct RingGuard
{
  GuardedRing *r;
};

////////////////////////////////
//~ rjf: Ring Functions

internal Ring *make_ring(Arena *arena, U64 size);
internal B32 ring_try_write(Ring *ring, U64 size, void *ptr);
internal B32 ring_try_read(Ring *ring, U64 size, void *ptr);
#define ring_try_write_struct(ring, ptr) ring_try_write((ring), sizeof(*(ptr)), (ptr))
#define ring_try_read_struct(ring, ptr) ring_try_read((ring), sizeof(*(ptr)), (ptr))

////////////////////////////////
//~ rjf: Guarded Ring Functions

internal GuardedRing *guarded_ring_alloc(Arena *arena, U64 size);
internal void guarded_ring_release(GuardedRing *ring);
internal RingGuard guarded_ring_open(GuardedRing *ring);
internal void guarded_ring_close(RingGuard *guard);
internal B32 guarded_ring_try_write(RingGuard *guard, U64 size, void *ptr);
internal B32 guarded_ring_try_read(RingGuard *guard, U64 size, void *ptr);
#define guarded_ring_try_write_struct(ring, ptr) guarded_ring_try_write((ring), sizeof(*(ptr)), (ptr))
#define guarded_ring_try_read_struct(ring, ptr) guarded_ring_try_read((ring), sizeof(*(ptr)), (ptr))
internal B32 guarded_ring_write_or_wait(RingGuard *guard, U64 size, void *ptr, U64 endt_us);
internal B32 guarded_ring_read_or_wait(RingGuard *guard, U64 size, void *ptr, U64 endt_us);
#define guarded_ring_write_struct_or_wait(ring, ptr, endt_us) guarded_ring_write_or_wait((ring), sizeof(*(ptr)), (ptr), (endt_us))
#define guarded_ring_read_struct_or_wait(ring, ptr, endt_us) guarded_ring_read_or_wait((ring), sizeof(*(ptr)), (ptr), (endt_us))
#define guarded_ring_write_string_or_wait(ring, string, endt_us) guarded_ring_write_or_wait((ring), (string).size, (string).str, (endt_us))

#endif // BASE_RING_H
