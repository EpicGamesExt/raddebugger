// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/render.meta.c"

////////////////////////////////
//~ dmylo: Blur Helpers (Helpers, Implemented once)
internal void
r_fill_blur_kernel(F32 blur_size, R_Blur_Kernel *kernel)
{
  F32 weights[ArrayCount(kernel->weights) * 2] = {0};

  blur_size = Min(blur_size, ArrayCount(weights));
  U64 blur_count = (U64)round_f32(blur_size);

  F32 stdev = (blur_size - 1.f) / 2.f;
  F32 one_over_root_2pi_stdev2 = 1 / sqrt_f32(2 * pi32 * stdev * stdev);
  F32 euler32 = 2.718281828459045f;

  weights[0] = 1.f;
  if (stdev > 0.f)
  {
    for (U64 idx = 0; idx < blur_count; idx += 1)
    {
      F32 kernel_x = (F32)idx;
      weights[idx] = one_over_root_2pi_stdev2 * pow_f32(euler32, -kernel_x * kernel_x / (2.f * stdev * stdev));
    }
  }
  if (weights[0] > 1.f)
  {
    MemoryZeroArray(weights);
    weights[0] = 1.f;
  }
  else
  {
    // prepare weights & offsets for bilinear lookup
    // blur filter wants to calculate w0*pixel[pos] + w1*pixel[pos+1] + ...
    // with bilinear filter we can do this calulation by doing only w*sample(pos+t) = w*((1-t)*pixel[pos] + t*pixel[pos+1])
    // we can see w0=w*(1-t) and w1=w*t
    // thus w=w0+w1 and t=w1/w
    for (U64 idx = 1; idx < blur_count; idx += 2)
    {
      F32 w0 = weights[idx + 0];
      F32 w1 = weights[idx + 1];
      F32 w = w0 + w1;
      F32 t = w1 / w;

      kernel->weights[(idx + 1) / 2] = v2f32(w, (F32)idx + t);
    }
  }
  kernel->weights[0].x = weights[0];
  kernel->blur_count = blur_count;
}

////////////////////////////////
//~ rjf: Basic Type Functions

internal R_Handle
r_handle_zero(void)
{
  R_Handle handle = {0};
  return handle;
}

internal B32
r_handle_match(R_Handle a, R_Handle b)
{
  return a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1];
}

////////////////////////////////
//~ rjf: Batch Type Functions

internal R_BatchList
r_batch_list_make(U64 instance_size)
{
  R_BatchList list = {0};
  list.bytes_per_inst = instance_size;
  return list;
}

internal void *
r_batch_list_push_inst(Arena *arena, R_BatchList *list, U64 batch_inst_cap)
{
  void *inst = 0;
  {
    R_BatchNode *n = list->last;
    if(n == 0 || n->v.byte_count+list->bytes_per_inst > n->v.byte_cap)
    {
      n = push_array(arena, R_BatchNode, 1);
      n->v.byte_cap = batch_inst_cap*list->bytes_per_inst;
      n->v.v = push_array_no_zero(arena, U8, n->v.byte_cap); 
      SLLQueuePush(list->first, list->last, n);
      list->batch_count += 1;
    }
    inst = n->v.v + n->v.byte_count;
    n->v.byte_count += list->bytes_per_inst;
    list->byte_count += list->bytes_per_inst;
  }
  return inst;
}

////////////////////////////////
//~ rjf: Pass Type Functions

internal R_Pass *
r_pass_from_kind(Arena *arena, R_PassList *list, R_PassKind kind)
{
  R_PassNode *n = list->last;
  if(!r_pass_kind_batch_table[kind])
  {
    n = 0;
  }
  if(n == 0 || n->v.kind != kind)
  {
    n = push_array(arena, R_PassNode, 1);
    SLLQueuePush(list->first, list->last, n);
    list->count += 1;
    n->v.kind = kind;
    n->v.params = push_array(arena, U8, r_pass_kind_params_size_table[kind]);
  }
  return &n->v;
}
