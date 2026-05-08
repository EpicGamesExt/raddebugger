// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Table Stripe Functions

internal StripeArray
stripe_array_alloc(Arena *arena)
{
  StripeArray array = {0};
  array.count = get_system_info()->logical_processor_count;
  array.v = push_array(arena, Stripe, array.count);
  for EachIndex(idx, array.count)
  {
    array.v[idx].arena = arena_alloc();
    array.v[idx].rw_mutex = rw_mutex_alloc();
    array.v[idx].cv = cond_var_alloc();
  }
  return array;
}

internal void
stripe_array_release(StripeArray *stripes)
{
  for EachIndex(idx, stripes->count)
  {
    arena_release(stripes->v[idx].arena);
    rw_mutex_release(stripes->v[idx].rw_mutex);
    cond_var_release(stripes->v[idx].cv);
  }
}

internal Stripe *
stripe_from_slot_idx(StripeArray *stripes, U64 slot_idx)
{
  Stripe *stripe = &stripes->v[slot_idx%stripes->count];
  return stripe;
}

////////////////////////////////
//~ rjf: Thread Info Helpers

internal void
set_thread_name(String8 string)
{
  ProfThreadName("%.*s", str8_varg(string));
  set_platform_thread_name(string);
}

internal void
set_thread_namef(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  set_thread_name(string);
  va_end(args);
  scratch_end(scratch);
}
