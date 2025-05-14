// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/render.meta.c"

////////////////////////////////
//~ rjf: Helpers

internal Mat4x4F32
r_sample_channel_map_from_tex2dformat(R_Tex2DFormat fmt)
{
  Mat4x4F32 result =
  {
    {
      {1, 0, 0, 0},
      {0, 1, 0, 0},
      {0, 0, 1, 0},
      {0, 0, 0, 1},
    }
  };
  switch(fmt)
  {
    default:{}break;
    case R_Tex2DFormat_R8:
    {
      MemoryZeroArray(result.v[0]);
      result.v[0][0] = result.v[0][1] = result.v[0][2] = result.v[0][3] = 1.f;
    }break;
  }
  return result;
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
  return MemoryMatchStruct(&a, &b);
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
