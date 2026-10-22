// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Send Completion Waiting Helper

internal B32
sock2_wait_for_send_completion(GuardedRing *out_ring, SOCK2_SendCompletion *out_completion, U64 endt_us)
{
  RingGuard g = guarded_ring_open(out_ring);
  B32 result = guarded_ring_read_struct_or_wait(&g, out_completion, endt_us);
  guarded_ring_close(&g);
  return result;
}
