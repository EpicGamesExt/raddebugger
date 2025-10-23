// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: API Implementation Helper Macros

#define RDIM_IdxedChunkListPush(arena, list, chunk_type, element_type, cap_value, result, ...) \
element_type *result = 0;\
do\
{\
chunk_type *n = list->last;\
if(n == 0 || n->count >= n->cap)\
{\
n = rdim_push_array(arena, chunk_type, 1);\
n->cap = cap_value;\
n->base_idx = list->total_count;\
__VA_ARGS__;\
n->v = rdim_push_array_no_zero(arena, element_type, n->cap);\
RDIM_SLLQueuePush(list->first, list->last, n);\
list->chunk_count += 1;\
}\
result = &n->v[n->count];\
result->chunk = n;\
n->count += 1;\
list->total_count += 1;\
}while(0)

#define RDIM_IdxedChunkListElementGetIdx(ptr, result) \
RDI_U64 idx = 0;\
if(ptr != 0 && ptr->chunk != 0)\
{\
idx = ptr->chunk->base_idx + (ptr - ptr->chunk->v) + 1;\
}

#define RDIM_IdxedChunkListConcatInPlace(chunk_type, dst, to_push, ...) \
for(chunk_type *n = to_push->first; n != 0; n = n->next)\
{\
n->base_idx += dst->total_count;\
}\
if(dst->last != 0 && to_push->first != 0)\
{\
dst->last->next = to_push->first;\
dst->last = to_push->last;\
dst->chunk_count += to_push->chunk_count;\
dst->total_count += to_push->total_count;\
__VA_ARGS__;\
}\
else if(dst->first == 0)\
{\
rdim_memcpy_struct(dst, to_push);\
}\
rdim_memzero_struct(to_push);

////////////////////////////////
//~ rjf: Basic Helpers

//- rjf: memory set

#if !defined(RDIM_MEMSET_OVERRIDE)
RDI_PROC void *
rdim_memset_fallback(void *dst, RDI_U8 c, RDI_U64 size)
{
  for(RDI_U64 idx = 0; idx < size; idx += 1)
  {
    ((RDI_U8 *)dst)[idx] = c;
  }
  return dst;
}
#endif

#if !defined(RDIM_MEMCPY_OVERRIDE)
RDI_PROC void *
rdim_memcpy_fallback(void *dst, void *src, RDI_U64 size)
{
  for(RDI_U64 idx = 0; idx < size; idx += 1)
  {
    ((RDI_U8 *)dst)[idx] = ((RDI_U8 *)src)[idx];
  }
  return dst;
}
#endif

//- rjf: arenas

#if !defined (RDIM_ARENA_OVERRIDE)

RDI_PROC RDIM_Arena *
rdim_arena_alloc_fallback(void)
{
  RDIM_Arena *arena = 0;
  // TODO(rjf)
  return arena;
}

RDI_PROC void
rdim_arena_release_fallback(RDIM_Arena *arena)
{
  // TODO(rjf)
}

RDI_PROC RDI_U64
rdim_arena_pos_fallback(RDIM_Arena *arena)
{
  // TODO(rjf)
  return 0;
}

RDI_PROC void *
rdim_arena_push_fallback(RDIM_Arena *arena, RDI_U64 size, RDI_U64 align, RDI_U32 zero)
{
  // TODO(rjf)
  return 0;
}

RDI_PROC void
rdim_arena_pop_to_fallback(RDIM_Arena *arena, RDI_U64 pos)
{
  // TODO(rjf)
}

#endif

//- rjf: thread-local scratch arenas

#if !defined (RDIM_SCRATCH_OVERRIDE)
static RDIM_THREAD_LOCAL RDIM_Arena *rdim_thread_scratches[2];

RDI_PROC RDIM_Temp
rdim_scratch_begin_fallback(RDIM_Arena **conflicts, RDI_U64 conflicts_count)
{
  if(rdim_thread_scratches[0] == 0)
  {
    rdim_thread_scratches[0] = rdim_arena_alloc();
    rdim_thread_scratches[1] = rdim_arena_alloc();
  }
  RDIM_Arena *arena = 0;
  for(RDI_U64 scratch_idx = 0;
      scratch_idx < sizeof(rdim_thread_scratches)/sizeof(rdim_thread_scratches[0]);
      scratch_idx += 1)
  {
    RDI_S32 scratch_conflicts = 0;
    for(RDI_U64 conflict_idx = 0; conflict_idx < conflicts_count; conflict_idx += 1)
    {
      if(conflicts[conflict_idx] == rdim_thread_scratches[scratch_idx])
      {
        scratch_conflicts = 1;
        break;
      }
    }
    if(!scratch_conflicts)
    {
      arena = rdim_thread_scratches[scratch_idx];
    }
  }
  RDIM_Temp temp;
  temp.arena = arena;
  temp.pos = rdim_arena_pos(arena);
  return temp;
}

RDI_PROC void
rdim_scratch_end_fallback(RDIM_Temp temp)
{
  rdim_arena_pop_to(temp.arena, temp.pos);
}

#endif

//- rjf: strings

RDI_PROC RDIM_String8
rdim_str8(RDI_U8 *str, RDI_U64 size)
{
  RDIM_String8 result;
  result.RDIM_String8_BaseMember = str;
  result.RDIM_String8_SizeMember = size;
  return result;
}

RDI_PROC RDIM_String8
rdim_str8_copy(RDIM_Arena *arena, RDIM_String8 src)
{
  RDIM_String8 dst;
  dst.RDIM_String8_SizeMember = src.RDIM_String8_SizeMember;
  dst.RDIM_String8_BaseMember = rdim_push_array_no_zero(arena, RDI_U8, dst.RDIM_String8_SizeMember+1);
  rdim_memcpy(dst.RDIM_String8_BaseMember, src.RDIM_String8_BaseMember, src.RDIM_String8_SizeMember);
  dst.RDIM_String8_BaseMember[dst.RDIM_String8_SizeMember] = 0;
  return dst;
}

RDI_PROC RDIM_String8
rdim_str8f(RDIM_Arena *arena, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  RDIM_String8 result = rdim_str8fv(arena, fmt, args);
  va_end(args);
  return(result);
}

RDI_PROC RDIM_String8
rdim_str8fv(RDIM_Arena *arena, char *fmt, va_list args)
{
  va_list args2;
  va_copy(args2, args);
  RDI_U32 needed_bytes = rdim_vsnprintf(0, 0, fmt, args) + 1;
  RDIM_String8 result = {0};
  result.RDIM_String8_BaseMember = rdim_push_array_no_zero(arena, RDI_U8, needed_bytes);
  result.RDIM_String8_SizeMember = rdim_vsnprintf((char*)result.str, needed_bytes, fmt, args2);
  result.RDIM_String8_BaseMember[result.RDIM_String8_SizeMember] = 0;
  va_end(args2);
  return(result);
}

RDI_PROC RDI_S32
rdim_str8_match(RDIM_String8 a, RDIM_String8 b, RDIM_StringMatchFlags flags)
{
  RDI_S32 result = 0;
  if(a.RDIM_String8_SizeMember == b.RDIM_String8_SizeMember)
  {
    RDI_S32 case_insensitive = (flags & RDIM_StringMatchFlag_CaseInsensitive);
    RDI_U64 size = a.RDIM_String8_SizeMember;
    result = 1;
    for(RDI_U64 idx = 0; idx < size; idx += 1)
    {
      RDI_U8 at = a.RDIM_String8_BaseMember[idx];
      RDI_U8 bt = b.RDIM_String8_BaseMember[idx];
      if(case_insensitive)
      {
        at = ('a' <= at && at <= 'z') ? at-('a'-'A') : at;
        bt = ('a' <= bt && bt <= 'z') ? bt-('a'-'A') : bt;
      }
      if(at != bt)
      {
        result = 0;
        break;
      }
    }
  }
  return result;
}

RDI_PROC RDIM_String8
rdim_lower_from_str8(RDIM_Arena *arena, RDIM_String8 string)
{
  RDIM_String8 result = rdim_str8_copy(arena, string);
  for(RDI_U64 idx = 0; idx < result.RDIM_String8_SizeMember; idx += 1)
  {
    RDI_U8 byte = result.RDIM_String8_BaseMember[idx];
    if('A' <= byte && byte <= 'Z')
    {
      result.RDIM_String8_BaseMember[idx] += ('a' - 'A');
    }
  }
  return result;
}

//- rjf: string lists

RDI_PROC void
rdim_str8_list_push(RDIM_Arena *arena, RDIM_String8List *list, RDIM_String8 string)
{
  RDIM_String8Node *n = rdim_push_array(arena, RDIM_String8Node, 1);
  n->RDIM_String8Node_StringMember = string;
  RDIM_SLLQueuePush_N(list->RDIM_String8List_FirstMember, list->RDIM_String8List_LastMember, n, RDIM_String8Node_NextPtrMember);
  list->RDIM_String8List_NodeCountMember += 1;
  list->RDIM_String8List_TotalSizeMember += string.RDIM_String8_SizeMember;
}

RDI_PROC void
rdim_str8_list_push_front(RDIM_Arena *arena, RDIM_String8List *list, RDIM_String8 string)
{
  RDIM_String8Node *n = rdim_push_array(arena, RDIM_String8Node, 1);
  n->RDIM_String8Node_StringMember = string;
  RDIM_SLLQueuePushFront_N(list->RDIM_String8List_FirstMember, list->RDIM_String8List_LastMember, n, RDIM_String8Node_NextPtrMember);
  list->RDIM_String8List_NodeCountMember += 1;
  list->RDIM_String8List_TotalSizeMember += string.RDIM_String8_SizeMember;
}

RDI_PROC void
rdim_str8_list_push_align(RDIM_Arena *arena, RDIM_String8List *list, RDI_U64 align)
{
  RDI_U64 total_size_pre_align  = list->total_size;
  RDI_U64 total_size_post_align = (total_size_pre_align + (align-1))&(~(align-1));
  RDI_U64 needed_size = total_size_post_align - total_size_pre_align;
  if(needed_size != 0)
  {
    RDI_U8 *padding = rdim_push_array(arena, RDI_U8, needed_size);
    rdim_str8_list_push(arena, list, rdim_str8(padding, needed_size));
  }
}

RDI_PROC RDIM_String8
rdim_str8_list_join(RDIM_Arena *arena, RDIM_String8List *list, RDIM_String8 sep)
{
  RDIM_String8 result;
  rdim_memzero_struct(&result);
  RDI_U64 sep_count = (list->RDIM_String8List_NodeCountMember > 1) ? (list->RDIM_String8List_NodeCountMember-1) : 0;
  result.RDIM_String8_SizeMember = list->RDIM_String8List_TotalSizeMember+sep_count*sep.RDIM_String8_SizeMember;
  result.RDIM_String8_BaseMember = rdim_push_array_no_zero(arena, RDI_U8, result.RDIM_String8_SizeMember+1);
  RDI_U64 off = 0;
  for(RDIM_String8Node *node = list->RDIM_String8List_FirstMember;
      node != 0;
      node = node->RDIM_String8Node_NextPtrMember)
  {
    rdim_memcpy((RDI_U8*)result.RDIM_String8_BaseMember+off,
                node->RDIM_String8Node_StringMember.RDIM_String8_BaseMember,
                node->RDIM_String8Node_StringMember.RDIM_String8_SizeMember);
    off += node->RDIM_String8Node_StringMember.RDIM_String8_SizeMember;
    if(sep.RDIM_String8_SizeMember != 0 && node->RDIM_String8Node_NextPtrMember != 0)
    {
      rdim_memcpy((RDI_U8*)result.RDIM_String8_BaseMember+off,
                  sep.RDIM_String8_BaseMember,
                  sep.RDIM_String8_SizeMember);
      off += sep.RDIM_String8_SizeMember;
    }
  }
  result.RDIM_String8_BaseMember[off] = 0;
  return result;
}

//- rjf: sortable range sorting

RDI_PROC RSFORCEINLINE int
rdim_sort_key_is_before(void *l, void *r)
{
  return ((RDIM_SortKey *)l)->key < ((RDIM_SortKey *)r)->key;
}

RDI_PROC RDIM_SortKey *
rdim_sort_key_array(RDIM_Arena *arena, RDIM_SortKey *keys, RDI_U64 count)
{
  // This sort is designed to take advantage of lots of pre-existing sorted ranges.
  // Most line info is already sorted or close to already sorted.
  // Similarly most vmap data has lots of pre-sorted ranges. etc. etc.
  // Also - this sort should be a "stable" sort. In the use case of sorting vmap
  // ranges, we want to be able to rely on order, so it needs to be preserved here.
  
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  RDIM_SortKey *result = 0;
  
  if(count <= 1)
  {
    result = keys;
  }
  else
  {
    RDIM_OrderedRange *ranges_first = 0;
    RDIM_OrderedRange *ranges_last = 0;
    RDI_U64 range_count = 0;
    {
      RDI_U64 pos = 0;
      for(;pos < count;)
      {
        // identify ordered range
        RDI_U64 first = pos;
        RDI_U64 opl = pos + 1;
        for(; opl < count && keys[opl - 1].key <= keys[opl].key; opl += 1);
        
        // generate an ordered range node
        RDIM_OrderedRange *new_range = rdim_push_array(rdim_temp_arena(scratch), RDIM_OrderedRange, 1);
        RDIM_SLLQueuePush(ranges_first, ranges_last, new_range);
        range_count += 1;
        new_range->first = first;
        new_range->opl = opl;
        
        // update pos
        pos = opl;
      }
    }
    
    if(range_count == 1)
    {
      result = keys;
    }
    else
    {
      RDIM_SortKey *keys_swap = rdim_push_array_no_zero(arena, RDIM_SortKey, count);
      RDIM_SortKey *src = keys;
      RDIM_SortKey *dst = keys_swap;
      RDIM_OrderedRange *src_ranges = ranges_first;
      RDIM_OrderedRange *dst_ranges = 0;
      RDIM_OrderedRange *dst_ranges_last = 0;
      
      for(;;)
      {
        // begin a pass
        for(;;)
        {
          // end pass when out of ranges
          if(src_ranges == 0)
          {
            break;
          }
          
          // get first range
          RDIM_OrderedRange *range1 = src_ranges;
          RDIM_SLLStackPop(src_ranges);
          
          // if this range is the whole array, we are done
          if(range1->first == 0 && range1->opl == count)
          {
            result = src;
            goto sort_done;
          }
          
          // if there is not a second range, save this range for next time and end this pass
          if(src_ranges == 0)
          {
            RDI_U64 first = range1->first;
            rdim_memcpy(dst + first, src + first, sizeof(*src)*(range1->opl - first));
            RDIM_SLLQueuePush(dst_ranges, dst_ranges_last, range1);
            break;
          }
          
          // get second range
          RDIM_OrderedRange *range2 = src_ranges;
          RDIM_SLLStackPop(src_ranges);
#if 0
          rdim_assert(range1->opl == range2->first);
#endif
          
          // merge these ranges
          RDI_U64 jd = range1->first;
          RDI_U64 j1 = range1->first;
          RDI_U64 j1_opl = range1->opl;
          RDI_U64 j2 = range2->first;
          RDI_U64 j2_opl = range2->opl;
          for(;;)
          {
            if(src[j1].key <= src[j2].key)
            {
              rdim_memcpy(dst + jd, src + j1, sizeof(*src));
              j1 += 1;
              jd += 1;
              if(j1 >= j1_opl)
              {
                break;
              }
            }
            else
            {
              rdim_memcpy(dst + jd, src + j2, sizeof(*src));
              j2 += 1;
              jd += 1;
              if(j2 >= j2_opl)
              {
                break;
              }
            }
          }
          if(j1 < j1_opl)
          {
            rdim_memcpy(dst + jd, src + j1, sizeof(*src)*(j1_opl - j1));
          }
          else
          {
            rdim_memcpy(dst + jd, src + j2, sizeof(*src)*(j2_opl - j2));
          }
          
          // save this as one range
          range1->opl = range2->opl;
          SLLQueuePush(dst_ranges, dst_ranges_last, range1);
        }
        
        // end pass by swapping buffers and range nodes
        {
          RDIM_SortKey *temp = src;
          src = dst;
          dst = temp;
        }
        src_ranges = dst_ranges;
        dst_ranges = 0;
        dst_ranges_last = 0;
      }
    }
  }
  sort_done:;
  
#if 0
  // assert sortedness
  for(RDI_U64 i = 1; i < count; i += 1)
  {
    rdim_assert(result[i - 1].key <= result[i].key);
  }
#endif
  
  rdim_scratch_end(scratch);
  return result;
}

//- rjf: rng1u64 list

RDI_PROC void
rdim_rng1u64_list_push(RDIM_Arena *arena, RDIM_Rng1U64List *list, RDIM_Rng1U64 r)
{
  RDIM_Rng1U64Node *n = rdim_push_array(arena, RDIM_Rng1U64Node, 1);
  n->v = r;
  RDIM_SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  if(list->count == 1 || r.min < list->min)
  {
    list->min = r.min;
  }
}

RDI_PROC void
rdim_rng1u64_chunk_list_push(RDIM_Arena *arena, RDIM_Rng1U64ChunkList *list, RDI_U64 chunk_cap, RDIM_Rng1U64 r)
{
  RDIM_Rng1U64ChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = rdim_push_array(arena, RDIM_Rng1U64ChunkNode, 1);
    RDIM_SLLQueuePush(list->first, list->last, n);
    n->cap = chunk_cap;
    n->v = rdim_push_array_no_zero(arena, RDIM_Rng1U64, n->cap);
    list->chunk_count += 1;
  }
  n->v[n->count] = r;
  n->count += 1;
  list->total_count += 1;
  if(list->total_count == 1 || r.min < list->min)
  {
    list->min = r.min;
  }
}

////////////////////////////////
//~ rjf: [Building] Data Model

RDI_PROC RDI_TypeKind
rdim_short_type_kind_from_data_model(RDIM_DataModel data_model)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch((RDIM_DataModelEnum)data_model)
  {
    case RDIM_DataModel_Null:{}break;
    case RDIM_DataModel_ILP32 :{result = RDI_TypeKind_S16;}break;
    case RDIM_DataModel_LLP64 :{result = RDI_TypeKind_S16;}break;
    case RDIM_DataModel_LP64  :{result = RDI_TypeKind_S16;}break;
    case RDIM_DataModel_ILP64 :{result = RDI_TypeKind_S16;}break;
    case RDIM_DataModel_SILP64:{result = RDI_TypeKind_S64;}break;
  }
  return result;
}

RDI_PROC RDI_TypeKind
rdim_unsigned_short_type_kind_from_data_model(RDIM_DataModel data_model)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch((RDIM_DataModelEnum)data_model)
  {
    case RDIM_DataModel_Null:{}break;
    case RDIM_DataModel_ILP32 :{result = RDI_TypeKind_U16;}break;
    case RDIM_DataModel_LLP64 :{result = RDI_TypeKind_U16;}break;
    case RDIM_DataModel_LP64  :{result = RDI_TypeKind_U16;}break;
    case RDIM_DataModel_ILP64 :{result = RDI_TypeKind_U16;}break;
    case RDIM_DataModel_SILP64:{result = RDI_TypeKind_U64;}break;
  }
  return result;
}

RDI_PROC RDI_TypeKind
rdim_int_type_from_data_model(RDIM_DataModel data_model)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch((RDIM_DataModelEnum)data_model)
  {
    case RDIM_DataModel_Null:{}break;
    case RDIM_DataModel_ILP32 :{result = RDI_TypeKind_S32;}break;
    case RDIM_DataModel_LLP64 :{result = RDI_TypeKind_S32;}break;
    case RDIM_DataModel_LP64  :{result = RDI_TypeKind_S32;}break;
    case RDIM_DataModel_ILP64 :{result = RDI_TypeKind_S64;}break;
    case RDIM_DataModel_SILP64:{result = RDI_TypeKind_S64;}break;
  }
  return result;
}

RDI_PROC RDI_TypeKind
rdim_unsigned_int_type_from_data_model(RDIM_DataModel data_model)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch((RDIM_DataModelEnum)data_model)
  {
    case RDIM_DataModel_Null:{}break;
    case RDIM_DataModel_ILP32 :{result = RDI_TypeKind_U32;}break;
    case RDIM_DataModel_LLP64 :{result = RDI_TypeKind_U32;}break;
    case RDIM_DataModel_LP64  :{result = RDI_TypeKind_U32;}break;
    case RDIM_DataModel_ILP64 :{result = RDI_TypeKind_U64;}break;
    case RDIM_DataModel_SILP64:{result = RDI_TypeKind_U64;}break;
  }
  return result;
}

RDI_PROC RDI_TypeKind
rdim_long_type_kind_from_data_model(RDIM_DataModel data_model)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch((RDIM_DataModelEnum)data_model)
  {
    case RDIM_DataModel_Null:{}break;
    case RDIM_DataModel_ILP32 :{result = RDI_TypeKind_S32;}break;
    case RDIM_DataModel_LLP64 :{result = RDI_TypeKind_S32;}break;
    case RDIM_DataModel_LP64  :{result = RDI_TypeKind_S64;}break;
    case RDIM_DataModel_ILP64 :{result = RDI_TypeKind_S64;}break;
    case RDIM_DataModel_SILP64:{result = RDI_TypeKind_S64;}break;
  }
  return result;
}

RDI_PROC RDI_TypeKind
rdim_unsigned_long_type_kind_from_data_model(RDIM_DataModel data_model)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch((RDIM_DataModelEnum)data_model)
  {
    case RDIM_DataModel_Null:{}break;
    case RDIM_DataModel_ILP32 :{result = RDI_TypeKind_U32;}break;
    case RDIM_DataModel_LLP64 :{result = RDI_TypeKind_U32;}break;
    case RDIM_DataModel_LP64  :{result = RDI_TypeKind_U64;}break;
    case RDIM_DataModel_ILP64 :{result = RDI_TypeKind_U64;}break;
    case RDIM_DataModel_SILP64:{result = RDI_TypeKind_U64;}break;
  }
  return result;
}

RDI_PROC RDI_TypeKind
rdim_long_long_type_kind_from_data_model(RDIM_DataModel data_model)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch((RDIM_DataModelEnum)data_model)
  {
    case RDIM_DataModel_Null:{}break;
    case RDIM_DataModel_ILP32 :{result = RDI_TypeKind_S64;}break;
    case RDIM_DataModel_LLP64 :{result = RDI_TypeKind_S64;}break;
    case RDIM_DataModel_LP64  :{result = RDI_TypeKind_S64;}break;
    case RDIM_DataModel_ILP64 :{result = RDI_TypeKind_S64;}break;
    case RDIM_DataModel_SILP64:{result = RDI_TypeKind_S64;}break;
  }
  return result;
}

RDI_PROC RDI_TypeKind
rdim_unsigned_long_long_type_kind_from_data_model(RDIM_DataModel data_model)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch((RDIM_DataModelEnum)data_model)
  {
    case RDIM_DataModel_Null:{}break;
    case RDIM_DataModel_ILP32 :{result = RDI_TypeKind_U64;}break;
    case RDIM_DataModel_LLP64 :{result = RDI_TypeKind_U64;}break;
    case RDIM_DataModel_LP64  :{result = RDI_TypeKind_U64;}break;
    case RDIM_DataModel_ILP64 :{result = RDI_TypeKind_U64;}break;
    case RDIM_DataModel_SILP64:{result = RDI_TypeKind_U64;}break;
  }
  return result;
}

RDI_PROC RDI_TypeKind
rdim_pointer_size_t_type_kind_from_data_model(RDIM_DataModel data_model)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch((RDIM_DataModelEnum)data_model)
  {
    case RDIM_DataModel_Null:{}break;
    case RDIM_DataModel_ILP32 :{result = RDI_TypeKind_U32;}break;
    case RDIM_DataModel_LLP64 :{result = RDI_TypeKind_U64;}break;
    case RDIM_DataModel_LP64  :{result = RDI_TypeKind_U64;}break;
    case RDIM_DataModel_ILP64 :{result = RDI_TypeKind_U64;}break;
    case RDIM_DataModel_SILP64:{result = RDI_TypeKind_U64;}break;
  }
  return result;
}

////////////////////////////////
//~ rjf: [Building] Binary Section List Building

RDI_PROC RDIM_BinarySection *
rdim_binary_section_list_push(RDIM_Arena *arena, RDIM_BinarySectionList *list)
{
  RDIM_BinarySectionNode *n = rdim_push_array(arena, RDIM_BinarySectionNode, 1);
  RDIM_SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  RDIM_BinarySection *result = &n->v;
  return result;
}

////////////////////////////////
//~ rjf: [Building] Source File Info Building

RDI_PROC RDIM_SrcFile *
rdim_src_file_chunk_list_push(RDIM_Arena *arena, RDIM_SrcFileChunkList *list, RDI_U64 cap)
{
  RDIM_IdxedChunkListPush(arena, list, RDIM_SrcFileChunkNode, RDIM_SrcFile, cap, result);
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_src_file(RDIM_SrcFile *src_file)
{
  RDIM_IdxedChunkListElementGetIdx(src_file, idx);
  return idx;
}

RDI_PROC void
rdim_src_file_chunk_list_concat_in_place(RDIM_SrcFileChunkList *dst, RDIM_SrcFileChunkList *to_push)
{
  RDIM_IdxedChunkListConcatInPlace(RDIM_SrcFileChunkNode, dst, to_push,
                                   dst->source_line_map_count += to_push->source_line_map_count,
                                   dst->total_line_count += to_push->total_line_count);
}

RDI_PROC void
rdim_src_file_push_line_sequence(RDIM_Arena *arena, RDIM_SrcFileChunkList *src_files, RDIM_SrcFile *src_file, RDIM_LineSequence *seq)
{
  if(src_file->first_line_map_fragment == 0)
  {
    src_files->source_line_map_count += 1;
  }
  RDIM_SrcFileLineMapFragment *fragment = rdim_push_array(arena, RDIM_SrcFileLineMapFragment, 1);
  fragment->seq = seq;
  RDIM_SLLQueuePush(src_file->first_line_map_fragment, src_file->last_line_map_fragment, fragment);
  src_file->total_line_count += seq->line_count;
  src_files->total_line_count += seq->line_count;
}

////////////////////////////////
//~ rjf: [Building] Line Info Building

RDI_PROC RDIM_LineTable *
rdim_line_table_chunk_list_push(RDIM_Arena *arena, RDIM_LineTableChunkList *list, RDI_U64 cap)
{
  RDIM_IdxedChunkListPush(arena, list, RDIM_LineTableChunkNode, RDIM_LineTable, cap, result);
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_line_table(RDIM_LineTable *line_table)
{
  RDIM_IdxedChunkListElementGetIdx(line_table, idx);
  return idx;
}

RDI_PROC void
rdim_line_table_chunk_list_concat_in_place(RDIM_LineTableChunkList *dst, RDIM_LineTableChunkList *to_push)
{
  RDIM_IdxedChunkListConcatInPlace(RDIM_LineTableChunkNode, dst, to_push,
                                   dst->total_seq_count += to_push->total_seq_count,
                                   dst->total_line_count += to_push->total_line_count,
                                   dst->total_col_count += to_push->total_col_count);
}

RDI_PROC RDIM_LineSequence *
rdim_line_table_push_sequence(RDIM_Arena *arena, RDIM_LineTableChunkList *line_tables, RDIM_LineTable *line_table, RDIM_SrcFile *src_file, RDI_U64 *voffs, RDI_U32 *line_nums, RDI_U16 *col_nums, RDI_U64 line_count)
{
  RDIM_LineSequenceNode *n = push_array(arena, RDIM_LineSequenceNode, 1);
  n->v.src_file = src_file;
  n->v.voffs = voffs;
  n->v.line_nums = line_nums;
  n->v.col_nums = col_nums;
  n->v.line_count = line_count;
  SLLQueuePush(line_table->first_seq, line_table->last_seq, n);
  line_table->seq_count += 1;
  line_table->line_count += line_count;
  line_table->col_count += line_count*2*(col_nums != 0);
  line_tables->total_seq_count += 1;
  line_tables->total_line_count += line_count;
  line_tables->total_col_count += line_count*2*(col_nums != 0);
  return &n->v;
}

////////////////////////////////
//~ rjf: [Building] Unit List Building

RDI_PROC RDIM_Unit *
rdim_unit_chunk_list_push(RDIM_Arena *arena, RDIM_UnitChunkList *list, RDI_U64 cap)
{
  RDIM_IdxedChunkListPush(arena, list, RDIM_UnitChunkNode, RDIM_Unit, cap, result);
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_unit(RDIM_Unit *unit)
{
  RDIM_IdxedChunkListElementGetIdx(unit, idx);
  return idx;
}

RDI_PROC void
rdim_unit_chunk_list_concat_in_place(RDIM_UnitChunkList *dst, RDIM_UnitChunkList *to_push)
{
  RDIM_IdxedChunkListConcatInPlace(RDIM_UnitChunkNode, dst, to_push);
}

////////////////////////////////
//~ rjf: [Building] Type Info Building

//- rjf: type nodes

RDI_PROC RDIM_Type **
rdim_array_from_type_list(RDIM_Arena *arena, RDIM_TypeList list)
{
  RDIM_Type **arr = push_array(arena, RDIM_Type *, list.count);
  U64         i   = 0;
  for(RDIM_TypeNode *n = list.first; n != 0; n = n->next, ++i)
  {
    arr[i] = n->v;
  }
  return arr;
}

RDI_PROC RDIM_TypeNode *
rdim_type_list_push(RDIM_Arena *arena, RDIM_TypeList *list, RDIM_Type *v)
{
  RDIM_TypeNode *n = push_array(arena, RDIM_TypeNode, 1);
  n->v = v;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  return n;
}

RDI_PROC RDIM_Type *
rdim_type_chunk_list_push(RDIM_Arena *arena, RDIM_TypeChunkList *list, RDI_U64 cap)
{
  RDIM_IdxedChunkListPush(arena, list, RDIM_TypeChunkNode, RDIM_Type, cap, result);
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_type(RDIM_Type *type)
{
  RDIM_IdxedChunkListElementGetIdx(type, idx);
  return idx;
}

RDI_PROC void
rdim_type_chunk_list_concat_in_place(RDIM_TypeChunkList *dst, RDIM_TypeChunkList *to_push)
{
  RDIM_IdxedChunkListConcatInPlace(RDIM_TypeChunkNode, dst, to_push);
}

//- rjf: UDTs

RDI_PROC RDIM_UDT *
rdim_udt_chunk_list_push(RDIM_Arena *arena, RDIM_UDTChunkList *list, RDI_U64 cap)
{
  RDIM_IdxedChunkListPush(arena, list, RDIM_UDTChunkNode, RDIM_UDT, cap, result);
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_udt(RDIM_UDT *udt)
{
  RDIM_IdxedChunkListElementGetIdx(udt, idx);
  return idx;
}

RDI_PROC void
rdim_udt_chunk_list_concat_in_place(RDIM_UDTChunkList *dst, RDIM_UDTChunkList *to_push)
{
  RDIM_IdxedChunkListConcatInPlace(RDIM_UDTChunkNode, dst, to_push,
                                   dst->total_member_count += to_push->total_member_count,
                                   dst->total_enum_val_count += to_push->total_enum_val_count);
}

RDI_PROC RDIM_UDTMember *
rdim_udt_push_member(RDIM_Arena *arena, RDIM_UDTChunkList *list, RDIM_UDT *udt)
{
  RDIM_UDTMember *mem = rdim_push_array(arena, RDIM_UDTMember, 1);
  RDIM_SLLQueuePush(udt->first_member, udt->last_member, mem);
  udt->member_count += 1;
  list->total_member_count += 1;
  return mem;
}

RDI_PROC RDIM_UDTEnumVal *
rdim_udt_push_enum_val(RDIM_Arena *arena, RDIM_UDTChunkList *list, RDIM_UDT *udt)
{
  RDIM_UDTEnumVal *mem = rdim_push_array(arena, RDIM_UDTEnumVal, 1);
  RDIM_SLLQueuePush(udt->first_enum_val, udt->last_enum_val, mem);
  udt->enum_val_count += 1;
  list->total_enum_val_count += 1;
  return mem;
}

////////////////////////////////
//~ rjf: [Building] Symbol Info Building

RDI_PROC RDIM_Symbol *
rdim_symbol_chunk_list_push(RDIM_Arena *arena, RDIM_SymbolChunkList *list, RDI_U64 cap)
{
  RDIM_IdxedChunkListPush(arena, list, RDIM_SymbolChunkNode, RDIM_Symbol, cap, result);
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_symbol(RDIM_Symbol *symbol)
{
  RDIM_IdxedChunkListElementGetIdx(symbol, idx);
  return idx;
}

RDI_PROC void
rdim_symbol_chunk_list_concat_in_place(RDIM_SymbolChunkList *dst, RDIM_SymbolChunkList *to_push)
{
  RDIM_IdxedChunkListConcatInPlace(RDIM_SymbolChunkNode, dst, to_push, dst->total_value_data_size += to_push->total_value_data_size);
}

internal void
rdim_symbol_push_value_data(RDIM_Arena *arena, RDIM_SymbolChunkList *list, RDIM_Symbol *symbol, RDIM_String8 data)
{
  symbol->value_data = rdim_str8_copy(arena, data);
  list->total_value_data_size += data.size;
}

////////////////////////////////
//~ rjf: [Building] Inline Site Info Building

RDI_PROC RDIM_InlineSite *
rdim_inline_site_chunk_list_push(RDIM_Arena *arena, RDIM_InlineSiteChunkList *list, RDI_U64 cap)
{
  RDIM_IdxedChunkListPush(arena, list, RDIM_InlineSiteChunkNode, RDIM_InlineSite, cap, result);
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_inline_site(RDIM_InlineSite *inline_site)
{
  RDIM_IdxedChunkListElementGetIdx(inline_site, idx);
  return idx;
}

RDI_PROC void
rdim_inline_site_chunk_list_concat_in_place(RDIM_InlineSiteChunkList *dst, RDIM_InlineSiteChunkList *to_push)
{
  RDIM_IdxedChunkListConcatInPlace(RDIM_InlineSiteChunkNode, dst, to_push);
}

////////////////////////////////
//~ rjf: [Building] Location Info Building

//- rjf: bytecode

RDI_PROC RDIM_EvalBytecodeOp *
rdim_bytecode_push_op(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_EvalOp op, RDI_U64 p)
{
  RDI_U16 ctrlbits = rdi_eval_op_ctrlbits_table[op];
  RDI_U32 p_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
  
  RDIM_EvalBytecodeOp *node = rdim_push_array(arena, RDIM_EvalBytecodeOp, 1);
  node->op = op;
  node->p_size = p_size;
  node->p = p;
  
  RDIM_SLLQueuePush(bytecode->first_op, bytecode->last_op, node);
  bytecode->op_count += 1;
  bytecode->encoded_size += 1 + p_size;
  
  return node;
}

RDI_PROC void
rdim_bytecode_push_uconst(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_U64 x)
{
  if(x <= 0xFF)
  {
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU8, x);
  }
  else if(x <= 0xFFFF)
  {
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU16, x);
  }
  else if(x <= 0xFFFFFFFF)
  {
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU32, x);
  }
  else
  {
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU64, x);
  }
}

RDI_PROC void
rdim_bytecode_push_sconst(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_S64 x)
{
  if(-0x80 <= x && x <= 0x7F)
  {
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU8, (RDI_U64)x);
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_TruncSigned, 8);
  }
  else if(-0x8000 <= x && x <= 0x7FFF)
  {
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU16, (RDI_U64)x);
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_TruncSigned, 16);
  }
  else if(-0x80000000ll <= x && x <= 0x7FFFFFFFll)
  {
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU32, (RDI_U64)x);
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_TruncSigned, 32);
  }
  else
  {
    rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU64, (RDI_U64)x);
  }
}

RDI_PROC void
rdim_bytecode_push_convert(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_EvalTypeGroup in, RDI_EvalTypeGroup out)
{
  rdim_bytecode_push_op(arena, bytecode, RDI_EvalOp_Convert, (U16)(in) | ((U16)(out) << 8));
}

RDI_PROC void
rdim_bytecode_concat_in_place(RDIM_EvalBytecode *left_dst, RDIM_EvalBytecode *right_destroyed)
{
  if(right_destroyed->first_op != 0)
  {
    if(left_dst->first_op == 0)
    {
      rdim_memcpy_struct(left_dst, right_destroyed);
    }
    else
    {
      left_dst->last_op->next  = right_destroyed->first_op;
      left_dst->last_op        = right_destroyed->last_op;
      left_dst->op_count      += right_destroyed->op_count;
      left_dst->encoded_size  += right_destroyed->encoded_size;
    }
    rdim_memzero_struct(right_destroyed);
  }
}

RDI_PROC B32
rdim_is_bytecode_tls_dependent(RDIM_EvalBytecode bytecode)
{
  B32 result = 0;
  for(RDIM_EvalBytecodeOp *n = bytecode.first_op; n != 0; n = n->next)
  {
    if(n->op == RDI_EvalOp_TLSOff)
    {
      result = 1;
      break;
    }
  }
  return result;
}

//- rjf: locations

RDI_PROC RDI_U64
rdim_encoded_size_from_location_info(RDIM_LocationInfo *info)
{
  RDI_U64 result = 0;
  switch((RDI_LocationKindEnum)info->kind)
  {
    case RDI_LocationKind_NULL:{}break;
    
    case RDI_LocationKind_AddrBytecodeStream:
    case RDI_LocationKind_ValBytecodeStream:
    {
      result = sizeof(RDI_LocationBytecodeStream) + info->bytecode.encoded_size + 1;
    }break;
    
    case RDI_LocationKind_AddrRegPlusU16:
    case RDI_LocationKind_AddrAddrRegPlusU16:
    {
      result = sizeof(RDI_LocationRegPlusU16);
    }break;
    
    case RDI_LocationKind_ValReg:
    {
      result = sizeof(RDI_LocationReg);
    }break;
  }
  return result;
}

RDI_PROC RDIM_Location *
rdim_location_chunk_list_push_new(RDIM_Arena *arena, RDIM_LocationChunkList *list, RDI_U64 cap, RDIM_LocationInfo *info)
{
  RDIM_IdxedChunkListPush(arena, list, RDIM_LocationChunkNode, RDIM_Location, cap, result, n->base_encoding_off = list->total_encoded_size);
  {
    RDI_U64 encoded_size = rdim_encoded_size_from_location_info(info);
    rdim_memcpy_struct(&result->info, info);
    result->relative_encoding_off = list->last->encoded_size;
    list->last->encoded_size += encoded_size;
    list->total_encoded_size += encoded_size;
  }
  return result;
}

RDI_PROC RDI_U64
rdim_off_from_location(RDIM_Location *location)
{
  RDI_U64 off = 0;
  if(location != 0 && location->chunk != 0)
  {
    off = location->chunk->base_encoding_off + location->relative_encoding_off + 1;
  }
  return off;
}

RDI_PROC void
rdim_location_chunk_list_concat_in_place(RDIM_LocationChunkList *dst, RDIM_LocationChunkList *to_push)
{
  for(RDIM_LocationChunkNode *n = to_push->first; n != 0; n = n->next)
  {
    n->base_encoding_off += dst->total_encoded_size;
  }
  RDIM_IdxedChunkListConcatInPlace(RDIM_LocationChunkNode, dst, to_push, dst->total_encoded_size += to_push->total_encoded_size);
}

////////////////////////////////
//~ rjf: [Building] Scope Info Building

RDI_PROC RDIM_Scope *
rdim_scope_chunk_list_push(RDIM_Arena *arena, RDIM_ScopeChunkList *list, RDI_U64 cap)
{
  RDIM_IdxedChunkListPush(arena, list, RDIM_ScopeChunkNode, RDIM_Scope, cap, result);
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_scope(RDIM_Scope *scope)
{
  RDIM_IdxedChunkListElementGetIdx(scope, idx);
  return idx;
}

RDI_PROC void
rdim_scope_chunk_list_concat_in_place(RDIM_ScopeChunkList *dst, RDIM_ScopeChunkList *to_push)
{
  RDIM_IdxedChunkListConcatInPlace(RDIM_ScopeChunkNode, dst, to_push,
                                   dst->scope_voff_count      += to_push->scope_voff_count,
                                   dst->local_count           += to_push->local_count,
                                   dst->location_case_count   += to_push->location_case_count);
}

RDI_PROC void
rdim_scope_push_voff_range(RDIM_Arena *arena, RDIM_ScopeChunkList *list, RDIM_Scope *scope, RDIM_Rng1U64 range)
{
  rdim_rng1u64_list_push(arena, &scope->voff_ranges, range);
  list->scope_voff_count += 2;
}

RDI_PROC RDIM_Local *
rdim_scope_push_local(RDIM_Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Scope *scope)
{
  RDIM_Local *local = rdim_push_array(arena, RDIM_Local, 1);
  RDIM_SLLQueuePush(scope->first_local, scope->last_local, local);
  scope->local_count += 1;
  scopes->local_count += 1;
  return local;
}

RDI_PROC RDIM_LocationCase *
rdim_push_location_case(RDIM_Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_LocationCaseList *list, RDIM_Location *location, RDIM_Rng1U64 voff_range)
{
  RDIM_LocationCase *n = rdim_push_array(arena, RDIM_LocationCase, 1);
  RDIM_SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  n->location = location;
  n->voff_range = voff_range;
  scopes->location_case_count += 1;
  return n;
}

RDI_PROC RDIM_LocationCase *
rdim_local_push_location_case(RDIM_Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Local *local, RDIM_Location *location, RDIM_Rng1U64 voff_range)
{
  return rdim_push_location_case(arena, scopes, &local->location_cases, location, voff_range);
}

////////////////////////////////
//~ rjf: [Building] Bake Parameter Joining

RDI_PROC void
rdim_bake_params_concat_in_place(RDIM_BakeParams *dst, RDIM_BakeParams *src)
{
  // rjf: join top-level info (deduplicate - throw away conflicts)
  {
    dst->subset_flags |= src->subset_flags;
    if(dst->top_level_info.arch == RDI_Arch_NULL)
    {
      dst->top_level_info.arch = src->top_level_info.arch;
    }
    if(dst->top_level_info.exe_name.size == 0)
    {
      dst->top_level_info.exe_name = src->top_level_info.exe_name;
    }
    if(dst->top_level_info.exe_hash == 0)
    {
      dst->top_level_info.exe_hash = src->top_level_info.exe_hash;
    }
    if(dst->top_level_info.voff_max == 0)
    {
      dst->top_level_info.voff_max = src->top_level_info.voff_max;
    }
    if(dst->top_level_info.guid.u64[0] == 0 && 
       dst->top_level_info.guid.u64[1] == 0)
    {
      dst->top_level_info.guid = src->top_level_info.guid;
    }
    if(dst->top_level_info.producer_name.size == 0)
    {
      dst->top_level_info.producer_name = src->top_level_info.producer_name;
    }
  }
  
  // rjf: join binary sections (deduplicate)
  {
    RDIM_Temp scratch = rdim_scratch_begin(0, 0);
    RDI_U64 slots_count = 256;
    RDIM_BinarySectionNode **slots = rdim_push_array(scratch.arena, RDIM_BinarySectionNode *, slots_count);
    for(RDIM_BinarySectionNode *n = dst->binary_sections.first; n != 0; n = n->next)
    {
      RDIM_BinarySectionNode *hash_node = rdim_push_array(scratch.arena, RDIM_BinarySectionNode, 1);
      RDI_U64 hash = rdi_hash(n->v.name.str, n->v.name.size);
      RDI_U64 slot_idx = hash%slots_count;
      RDIM_SLLStackPush(slots[slot_idx], hash_node);
      hash_node->v = n->v;
    }
    for(RDIM_BinarySectionNode *n = src->binary_sections.first, *next = 0; n != 0; n = next)
    {
      next = n->next;
      RDI_U64 hash = rdi_hash(n->v.name.str, n->v.name.size);
      RDI_U64 slot_idx = hash%slots_count;
      RDI_S32 is_duplicate = 0;
      for(RDIM_BinarySectionNode *hash_n = slots[slot_idx]; hash_n != 0; hash_n = hash_n->next)
      {
        if(rdim_str8_match(hash_n->v.name, n->v.name, 0))
        {
          is_duplicate = 1;
          break;
        }
      }
      if(!is_duplicate)
      {
        RDIM_SLLQueuePush(dst->binary_sections.first, dst->binary_sections.last, n);
        dst->binary_sections.count += 1;
      }
    }
    rdim_scratch_end(scratch);
  }
  
  // rjf: join non-top-level chunk lists
  {
    rdim_unit_chunk_list_concat_in_place(&dst->units, &src->units);
    rdim_type_chunk_list_concat_in_place(&dst->types, &src->types);
    rdim_udt_chunk_list_concat_in_place(&dst->udts, &src->udts);
    rdim_src_file_chunk_list_concat_in_place(&dst->src_files, &src->src_files);
    rdim_line_table_chunk_list_concat_in_place(&dst->line_tables, &src->line_tables);
    rdim_location_chunk_list_concat_in_place(&dst->locations, &src->locations);
    rdim_symbol_chunk_list_concat_in_place(&dst->global_variables, &src->global_variables);
    rdim_symbol_chunk_list_concat_in_place(&dst->thread_variables, &src->thread_variables);
    rdim_symbol_chunk_list_concat_in_place(&dst->constants, &src->constants);
    rdim_symbol_chunk_list_concat_in_place(&dst->procedures, &src->procedures);
    rdim_scope_chunk_list_concat_in_place(&dst->scopes, &src->scopes);
    rdim_inline_site_chunk_list_concat_in_place(&dst->inline_sites, &src->inline_sites);
  }
}

////////////////////////////////
//~ rjf: [Baking Helpers] Deduplicated String Baking Map

//- rjf: chunk lists

RDI_PROC RDIM_BakeString *
rdim_bake_string_chunk_list_push(RDIM_Arena *arena, RDIM_BakeStringChunkList *list, RDI_U64 cap)
{
  RDIM_BakeStringChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = rdim_push_array(arena, RDIM_BakeStringChunkNode, 1);
    n->cap = cap;
    n->v = rdim_push_array(arena, RDIM_BakeString, n->cap);
    RDIM_SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }
  RDIM_BakeString *s = &n->v[n->count];
  n->count += 1;
  list->total_count += 1;
  return s;
}

RDI_PROC void
rdim_bake_string_chunk_list_concat_in_place(RDIM_BakeStringChunkList *dst, RDIM_BakeStringChunkList *to_push)
{
  for(RDIM_BakeStringChunkNode *n = to_push->first; n != 0; n = n->next)
  {
    n->base_idx += dst->total_count;
  }
  if(dst->last != 0 && to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->chunk_count += to_push->chunk_count;
    dst->total_count += to_push->total_count;
  }
  else if(dst->first == 0)
  {
    rdim_memcpy_struct(dst, to_push);
  }
  rdim_memzero_struct(to_push);
}

RDI_PROC RSFORCEINLINE int
rdim_bake_string_is_before(void *l, void *r)
{
  return str8_is_before(((RDIM_BakeString *)l)->string, ((RDIM_BakeString *)r)->string);
}

RDI_PROC RDIM_BakeStringChunkList
rdim_bake_string_chunk_list_sorted_from_unsorted(RDIM_Arena *arena, RDIM_BakeStringChunkList *src)
{
  //- rjf: produce unsorted destination list with single chunk node
  RDIM_BakeStringChunkList dst = {0};
  for(RDIM_BakeStringChunkNode *n = src->first; n != 0; n = n->next)
  {
    for(RDI_U64 idx = 0; idx < n->count; idx += 1)
    {
      RDIM_BakeString *src_str = &n->v[idx];
      RDIM_BakeString *dst_str = rdim_bake_string_chunk_list_push(arena, &dst, src->total_count);
      rdim_memcpy_struct(dst_str, src_str);
    }
  }
  
  //- rjf: sort chunk node
  if(dst.first != 0)
  {
    radsort(dst.first->v, dst.first->count, rdim_bake_string_is_before);
  }
  
  //- rjf: iterate sorted chunk node, remove duplicates, count # of duplicates
  RDI_U64 num_duplicates = 0;
  if(dst.first != 0)
  {
    RDI_U64 last_idx = 0;
    for(RDI_U64 idx = 1; idx < dst.first->count; idx += 1)
    {
      if(rdim_str8_match(dst.first->v[last_idx].string, dst.first->v[idx].string, 0))
      {
        rdim_memzero_struct(&dst.first->v[idx]);
        num_duplicates += 1;
      }
      else
      {
        last_idx = idx;
      }
    }
  }
  
  //- rjf: iterate sorted chunk node, make non-empty elements contiguous
  if(num_duplicates != 0)
  {
    RDI_U64 last_idx = 0;
    for(RDI_U64 idx = 1; idx < dst.first->count; idx += 1)
    {
      if(last_idx == 0 &&
         dst.first->v[idx].string.RDIM_String8_SizeMember == 0 &&
         dst.first->v[idx].hash == 0)
      {
        last_idx = idx;
      }
      if(last_idx != 0 && dst.first->v[idx].string.RDIM_String8_SizeMember != 0)
      {
        rdim_memcpy_struct(&dst.first->v[last_idx], &dst.first->v[idx]);
        rdim_memzero_struct(&dst.first->v[idx]);
        last_idx += 1;
      }
    }
    
    //- rjf: pop extras
    if(num_duplicates != 0)
    {
      RDI_U64 arena_pos_pre_pop = rdim_arena_pos(arena);
      rdim_arena_pop_to(arena, arena_pos_pre_pop - num_duplicates*sizeof(dst.first->v[0]));
      dst.first->count -= num_duplicates;
      dst.first->cap   -= num_duplicates;
      dst.total_count  -= num_duplicates;
    }
  }
  
  return dst;
}

//- rjf: loose map

RDI_PROC RDIM_BakeStringMapLoose *
rdim_bake_string_map_loose_make(RDIM_Arena *arena, RDIM_BakeStringMapTopology *top)
{
  RDIM_BakeStringMapLoose *map = rdim_push_array(arena, RDIM_BakeStringMapLoose, 1);
  map->slots = rdim_push_array(arena, RDIM_BakeStringChunkList *, top->slots_count);
  return map;
}

RDI_PROC void
rdim_bake_string_map_loose_insert(RDIM_Arena *arena, RDIM_BakeStringMapTopology *map_topology, RDIM_BakeStringMapLoose *map, RDI_U64 chunk_cap, RDIM_String8 string)
{
  if(string.RDIM_String8_SizeMember != 0)
  {
    RDI_U64 hash = rdi_hash(string.RDIM_String8_BaseMember, string.RDIM_String8_SizeMember);
    RDI_U64 slot_idx = hash%map_topology->slots_count;
    RDIM_BakeStringChunkList *slot = map->slots[slot_idx];
    if(slot == 0)
    {
      slot = map->slots[slot_idx] = rdim_push_array(arena, RDIM_BakeStringChunkList, 1);
    }
    RDI_S32 is_duplicate = 0;
    for(RDIM_BakeStringChunkNode *n = slot->first; n != 0; n = n->next)
    {
      for(RDI_U64 idx = 0; idx < n->count; idx += 1)
      {
        if(rdim_str8_match(n->v[idx].string, string, 0))
        {
          is_duplicate = 1;
          goto break_all;
        }
      }
    }
    break_all:;
    if(!is_duplicate)
    {
      RDIM_BakeString *bstr = rdim_bake_string_chunk_list_push(arena, slot, chunk_cap);
      bstr->string = string;
      bstr->hash = hash;
    }
  }
}

RDI_PROC RDIM_BakeStringMapBaseIndices
rdim_bake_string_map_base_indices_from_map_loose(RDIM_Arena *arena, RDIM_BakeStringMapTopology *map_topology, RDIM_BakeStringMapLoose *map)
{
  RDIM_BakeStringMapBaseIndices indices = {0};
  indices.slots_base_idxs = rdim_push_array(arena, RDI_U64, map_topology->slots_count+1);
  RDI_U64 total_count = 0;
  for(RDI_U64 idx = 0; idx < map_topology->slots_count; idx += 1)
  {
    indices.slots_base_idxs[idx] += total_count;
    if(map->slots[idx] != 0)
    {
      total_count += map->slots[idx]->total_count;
    }
  }
  indices.slots_base_idxs[map_topology->slots_count] = total_count;
  return indices;
}

//- rjf: finalized / tight map

RDI_PROC RDI_U32
rdim_bake_idx_from_string(RDIM_BakeStringMapTight *map, RDIM_String8 string)
{
  RDI_U32 idx = 0;
  if(string.RDIM_String8_SizeMember != 0)
  {
    RDI_U64 hash = rdi_hash(string.RDIM_String8_BaseMember, string.RDIM_String8_SizeMember);
    RDI_U64 slot_idx = hash%map->slots_count;
    for(RDIM_BakeStringChunkNode *n = map->slots[slot_idx].first; n != 0; n = n->next)
    {
      for(RDI_U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1)
      {
        if(n->v[chunk_idx].hash == hash && rdim_str8_match(n->v[chunk_idx].string, string, 0))
        {
          idx = map->slots_base_idxs[slot_idx] + n->base_idx + chunk_idx + 1;
          break;
        }
      }
    }
  }
  return idx;
}

////////////////////////////////
//~ rjf: [Baking Helpers] Deduplicated Index Run Baking Map

//- rjf: bake idx run map reading/writing

RDI_PROC RDI_U64
rdim_hash_from_idx_run(RDI_U32 *idx_run, RDI_U32 count)
{
  RDI_U64 hash = 5381;
  RDI_U32 *ptr = idx_run;
  RDI_U32 *opl = idx_run + count;
  for(;ptr < opl; ptr += 1)
  {
    hash = ((hash << 5) + hash) + (*ptr);
  }
  return hash;
}

//- rjf: chunk lists

RDI_PROC RDIM_BakeIdxRun *
rdim_bake_idx_run_chunk_list_push(RDIM_Arena *arena, RDIM_BakeIdxRunChunkList *list, RDI_U64 cap)
{
  RDIM_BakeIdxRunChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = rdim_push_array(arena, RDIM_BakeIdxRunChunkNode, 1);
    n->cap = cap;
    n->v = rdim_push_array(arena, RDIM_BakeIdxRun, n->cap);
    RDIM_SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }
  RDIM_BakeIdxRun *s = &n->v[n->count];
  n->count += 1;
  list->total_count += 1;
  return s;
}

RDI_PROC void
rdim_bake_idx_run_chunk_list_concat_in_place(RDIM_BakeIdxRunChunkList *dst, RDIM_BakeIdxRunChunkList *to_push)
{
  if(dst->last != 0 && to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->chunk_count += to_push->chunk_count;
    dst->total_count += to_push->total_count;
  }
  else if(dst->first == 0)
  {
    rdim_memcpy_struct(dst, to_push);
  }
  rdim_memzero_struct(to_push);
}

RDI_PROC RSFORCEINLINE int
rdim_bake_idx_run_is_before(void *l, void *r)
{
  B32 is_less_than = 0;
  {
    RDIM_BakeIdxRun *lir = (RDIM_BakeIdxRun *)l;
    RDIM_BakeIdxRun *rir = (RDIM_BakeIdxRun *)r;
    U64 common_count = Min(lir->count, rir->count);
    for(U64 off = 0; off < common_count; off += 1)
    {
      if(lir->idxes[off] < rir->idxes[off])
      {
        is_less_than = 1;
        break;
      }
      else if(lir->idxes[off] > rir->idxes[off])
      {
        is_less_than = 0;
        break;
      }
      else if(off+1 == common_count)
      {
        is_less_than = (lir->count < rir->count);
      }
    }
  }
  return is_less_than;
}

RDI_PROC RDIM_BakeIdxRunChunkList
rdim_bake_idx_run_chunk_list_sorted_from_unsorted(RDIM_Arena *arena, RDIM_BakeIdxRunChunkList *src)
{
  //- rjf: produce unsorted destination list with single chunk node
  RDIM_BakeIdxRunChunkList dst = {0};
  for(RDIM_BakeIdxRunChunkNode *n = src->first; n != 0; n = n->next)
  {
    for(RDI_U64 idx = 0; idx < n->count; idx += 1)
    {
      RDIM_BakeIdxRun *src_str = &n->v[idx];
      RDIM_BakeIdxRun *dst_str = rdim_bake_idx_run_chunk_list_push(arena, &dst, src->total_count);
      rdim_memcpy_struct(dst_str, src_str);
    }
  }
  
  //- rjf: sort chunk node
  if(dst.first != 0)
  {
    radsort(dst.first->v, dst.first->count, rdim_bake_idx_run_is_before);
  }
  
  //- rjf: iterate sorted chunk node, remove duplicates, count # of duplicates
  RDI_U64 num_duplicates = 0;
  if(dst.first != 0)
  {
    RDI_U64 last_idx = 0;
    for(RDI_U64 idx = 1; idx < dst.first->count; idx += 1)
    {
      if(dst.first->v[last_idx].count == dst.first->v[idx].count &&
         MemoryMatch(dst.first->v[last_idx].idxes,
                     dst.first->v[idx].idxes,
                     sizeof(dst.first->v[idx].idxes[0]) * dst.first->v[idx].count))
      {
        rdim_memzero_struct(&dst.first->v[idx]);
        num_duplicates += 1;
      }
      else
      {
        last_idx = idx;
      }
    }
  }
  
  //- rjf: iterate sorted chunk node, make non-empty elements contiguous
  if(num_duplicates != 0)
  {
    RDI_U64 last_idx = 0;
    for(RDI_U64 idx = 1; idx < dst.first->count; idx += 1)
    {
      if(last_idx == 0 && dst.first->v[idx].hash == 0)
      {
        last_idx = idx;
      }
      if(last_idx != 0 && dst.first->v[idx].hash != 0)
      {
        rdim_memcpy_struct(&dst.first->v[last_idx], &dst.first->v[idx]);
        rdim_memzero_struct(&dst.first->v[idx]);
        last_idx += 1;
      }
    }
    
    //- rjf: pop extras
    if(num_duplicates != 0)
    {
      RDI_U64 arena_pos_pre_pop = rdim_arena_pos(arena);
      rdim_arena_pop_to(arena, arena_pos_pre_pop - num_duplicates*sizeof(dst.first->v[0]));
      dst.first->count -= num_duplicates;
      dst.first->cap   -= num_duplicates;
      dst.total_count  -= num_duplicates;
    }
  }
  
  return dst;
}

//- rjf: loose map

RDI_PROC RDIM_BakeIdxRunMapLoose *
rdim_bake_idx_run_map_loose_make(RDIM_Arena *arena, RDIM_BakeIdxRunMapTopology *top)
{
  RDIM_BakeIdxRunMapLoose *map = rdim_push_array(arena, RDIM_BakeIdxRunMapLoose, 1);
  map->slots = rdim_push_array(arena, RDIM_BakeIdxRunChunkList *, top->slots_count);
  map->slots_idx_counts = rdim_push_array(arena, RDI_U64, top->slots_count);
  return map;
}

RDI_PROC void
rdim_bake_idx_run_map_loose_insert(RDIM_Arena *arena, RDIM_BakeIdxRunMapTopology *map_topology, RDIM_BakeIdxRunMapLoose *map, RDI_U64 chunk_cap, RDI_U32 *idxes, RDI_U32 count)
{
  if(count != 0)
  {
    RDI_U64 hash = rdim_hash_from_idx_run(idxes, count);
    RDI_U64 slot_idx = hash%map_topology->slots_count;
    RDIM_BakeIdxRunChunkList *slot = map->slots[slot_idx];
    if(slot == 0)
    {
      slot = map->slots[slot_idx] = rdim_push_array(arena, RDIM_BakeIdxRunChunkList, 1);
    }
    RDI_S32 is_duplicate = 0;
    for(RDIM_BakeIdxRunChunkNode *n = slot->first; n != 0; n = n->next)
    {
      for(RDI_U64 idx = 0; idx < n->count; idx += 1)
      {
        if(n->v[idx].hash == hash &&
           n->v[idx].count == count &&
           MemoryMatch(n->v[idx].idxes, idxes, sizeof(idxes[0])*count))
        {
          is_duplicate = 1;
          goto break_all;
        }
      }
    }
    break_all:;
    if(!is_duplicate)
    {
      RDIM_BakeIdxRun *bir = rdim_bake_idx_run_chunk_list_push(arena, slot, chunk_cap);
      bir->hash = hash;
      bir->count = count;
      bir->idxes = idxes;
      map->slots_idx_counts[slot_idx] += count;
    }
  }
}

//- rjf: finalized / tight map

RDI_PROC RDI_U32
rdim_bake_idx_from_idx_run(RDIM_BakeIdxRunMap *map, RDI_U32 *idxes, RDI_U32 count)
{
  RDI_U32 idx = 0;
  if(count != 0)
  {
    RDI_U64 hash = rdim_hash_from_idx_run(idxes, count);
    RDI_U64 slot_idx = hash%map->slots_count;
    RDI_U64 off = 0;
    for(RDIM_BakeIdxRunChunkNode *n = map->slots[slot_idx].first; n != 0; n = n->next)
    {
      for(RDI_U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1)
      {
        if(n->v[chunk_idx].hash == hash &&
           n->v[chunk_idx].count == count &&
           MemoryMatch(n->v[chunk_idx].idxes, idxes, sizeof(idxes[0])*count))
        {
          idx = (RDI_U32)(map->slots_base_idxs[slot_idx] + off); // TODO(rjf): @u64_to_u32
          goto end_lookup;
        }
        off += n->v[chunk_idx].count;
      }
    }
    end_lookup:;
  }
  return idx;
}

////////////////////////////////
//~ rjf: [Baking Helpers] Deduplicated Name Map Baking Map

//- rjf: chunk lists

RDI_PROC RDIM_BakeName *
rdim_bake_name_chunk_list_push(RDIM_Arena *arena, RDIM_BakeNameChunkList *list, RDI_U64 cap)
{
  RDIM_BakeNameChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = rdim_push_array(arena, RDIM_BakeNameChunkNode, 1);
    n->cap = cap;
    n->base_idx = list->total_count;
    n->v = rdim_push_array(arena, RDIM_BakeName, n->cap);
    RDIM_SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }
  RDIM_BakeName *result = &n->v[n->count];
  n->count += 1;
  list->total_count += 1;
  return result;
}

RDI_PROC void
rdim_bake_name_chunk_list_concat_in_place(RDIM_BakeNameChunkList *dst, RDIM_BakeNameChunkList *to_push)
{
  for(RDIM_BakeNameChunkNode *n = to_push->first; n != 0; n = n->next)
  {
    n->base_idx += dst->total_count;
  }
  if(dst->last != 0 && to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->chunk_count += to_push->chunk_count;
    dst->total_count += to_push->total_count;
  }
  else if(dst->first == 0)
  {
    rdim_memcpy_struct(dst, to_push);
  }
  rdim_memzero_struct(to_push);
}

RDI_PROC RSFORCEINLINE int
rdim_bake_name_is_before(void *l, void *r)
{
  RDIM_BakeName *lhs = (RDIM_BakeName *)l;
  RDIM_BakeName *rhs = (RDIM_BakeName *)r;
  B32 lhs_name_lt = str8_is_before(lhs->string, rhs->string);
  B32 is_before = lhs_name_lt;
  if(!lhs_name_lt)
  {
    B32 rhs_name_lt = str8_is_before(rhs->string, lhs->string);
    if(!rhs_name_lt)
    {
      is_before = (lhs->idx > rhs->idx);
    }
  }
  return is_before;
}

RDI_PROC RDIM_BakeNameChunkList
rdim_bake_name_chunk_list_sorted_from_unsorted(RDIM_Arena *arena, RDIM_BakeNameChunkList *src)
{
  //- rjf: produce unsorted destination list with single chunk node
  RDIM_BakeNameChunkList dst = {0};
  for(RDIM_BakeNameChunkNode *n = src->first; n != 0; n = n->next)
  {
    for(RDI_U64 idx = 0; idx < n->count; idx += 1)
    {
      RDIM_BakeName *src_str = &n->v[idx];
      RDIM_BakeName *dst_str = rdim_bake_name_chunk_list_push(arena, &dst, src->total_count);
      rdim_memcpy_struct(dst_str, src_str);
    }
  }
  
  //- rjf: sort chunk node
  if(dst.first != 0)
  {
    radsort(dst.first->v, dst.first->count, rdim_bake_name_is_before);
  }
  
  //- rjf: iterate sorted chunk node, remove duplicates, count # of duplicates
  RDI_U64 num_duplicates = 0;
  if(dst.first != 0)
  {
    RDI_U64 last_idx = 0;
    for(RDI_U64 idx = 1; idx < dst.first->count; idx += 1)
    {
      if(rdim_str8_match(dst.first->v[last_idx].string, dst.first->v[idx].string, 0) &&
         dst.first->v[last_idx].idx == dst.first->v[idx].idx)
      {
        rdim_memzero_struct(&dst.first->v[idx]);
        num_duplicates += 1;
      }
      else
      {
        last_idx = idx;
      }
    }
  }
  
  //- rjf: iterate sorted chunk node, make non-empty elements contiguous
  if(num_duplicates != 0)
  {
    RDI_U64 last_idx = 0;
    for(RDI_U64 idx = 1; idx < dst.first->count; idx += 1)
    {
      if(last_idx == 0 &&
         dst.first->v[idx].string.RDIM_String8_SizeMember == 0 &&
         dst.first->v[idx].hash == 0)
      {
        last_idx = idx;
      }
      if(last_idx != 0 && dst.first->v[idx].string.RDIM_String8_SizeMember != 0)
      {
        rdim_memcpy_struct(&dst.first->v[last_idx], &dst.first->v[idx]);
        rdim_memzero_struct(&dst.first->v[idx]);
        last_idx += 1;
      }
    }
    
    //- rjf: pop extras
    if(num_duplicates != 0)
    {
      RDI_U64 arena_pos_pre_pop = rdim_arena_pos(arena);
      rdim_arena_pop_to(arena, arena_pos_pre_pop - num_duplicates*sizeof(dst.first->v[0]));
      dst.first->count -= num_duplicates;
      dst.first->cap   -= num_duplicates;
      dst.total_count  -= num_duplicates;
    }
  }
  
  return dst;
}

//- rjf: bake name chunk list maps

RDI_PROC RDIM_BakeNameMap *
rdim_bake_name_map_make(RDIM_Arena *arena, RDIM_BakeNameMapTopology *top)
{
  RDIM_BakeNameMap *map = rdim_push_array(arena, RDIM_BakeNameMap, 1);
  map->slots = rdim_push_array(arena, RDIM_BakeNameChunkList *, top->slots_count);
  return map;
}

RDI_PROC void
rdim_bake_name_map_insert(RDIM_Arena *arena, RDIM_BakeNameMapTopology *map_topology, RDIM_BakeNameMap *map, RDI_U64 chunk_cap, RDIM_String8 string, RDI_U64 idx)
{
  if(string.RDIM_String8_SizeMember != 0)
  {
    RDI_U64 hash = rdi_hash(string.RDIM_String8_BaseMember, string.RDIM_String8_SizeMember);
    RDI_U64 slot_idx = hash%map_topology->slots_count;
    RDIM_BakeNameChunkList *slot = map->slots[slot_idx];
    if(slot == 0)
    {
      slot = map->slots[slot_idx] = rdim_push_array(arena, RDIM_BakeNameChunkList, 1);
    }
    RDI_S32 is_duplicate = 0;
    for(RDIM_BakeNameChunkNode *n = slot->first; n != 0; n = n->next)
    {
      for(RDI_U64 idx = 0; idx < n->count; idx += 1)
      {
        if(rdim_str8_match(n->v[idx].string, string, 0) &&
           n->v[idx].idx == idx)
        {
          is_duplicate = 1;
          goto break_all;
        }
      }
    }
    break_all:;
    if(!is_duplicate)
    {
      RDIM_BakeName *bstr = rdim_bake_name_chunk_list_push(arena, slot, chunk_cap);
      bstr->string = string;
      bstr->idx = idx;
      bstr->hash = hash;
    }
  }
}

////////////////////////////////
//~ rjf: [Baking Helpers] Deduplicated Path Baking Tree

RDI_PROC RDIM_BakePathNode *
rdim_bake_path_node_from_string(RDIM_BakePathTree *tree, RDIM_String8 string)
{
  RDIM_BakePathNode *node = &tree->root;
  RDI_U8 *ptr = string.str;
  RDI_U8 *opl = string.str + string.size;
  for(;ptr < opl && node != 0;)
  {
    // rjf: skip past slashes
    for(;ptr < opl && (*ptr == '/' || *ptr == '\\'); ptr += 1);
    
    // rjf: save beginning of non-slash range
    RDI_U8 *range_first = ptr;
    
    // rjf: skip past non-slashes
    for(;ptr < opl && !(*ptr == '/' || *ptr == '\\'); ptr += 1);
    
    // rjf: empty range -> continue
    if(range_first >= ptr)
    {
      continue;
    }
    
    // rjf: range -> sub-directory string
    RDIM_String8 sub_dir = rdim_str8(range_first, (RDI_U64)(ptr-range_first));
    
    // rjf: sub-directory string -> find child of node
    RDIM_BakePathNode *sub_dir_node = 0;
    for(RDIM_BakePathNode *child = node->first_child; child != 0; child = child->next_sibling)
    {
      if(rdim_str8_match(child->name, sub_dir, 0))
      {
        sub_dir_node = child;
      }
    }
    
    // rjf: .. -> go up
    if(sub_dir.RDIM_String8_SizeMember == 2 &&
       sub_dir.RDIM_String8_BaseMember[0] == '.' &&
       sub_dir.RDIM_String8_BaseMember[1] == '.')
    {
      sub_dir_node = node->parent;
      if(sub_dir_node == 0)
      {
        sub_dir_node = &tree->root;
      }
    }
    
    // rjf: . -> stay here
    else if(sub_dir.RDIM_String8_SizeMember == 1 &&
            sub_dir.RDIM_String8_BaseMember[0] == '.')
    {
      sub_dir_node = node;
    }
    
    // rjf: descend to child
    node = sub_dir_node;
  }
  return node;
}

RDI_PROC RDI_U32
rdim_bake_path_node_idx_from_string(RDIM_BakePathTree *tree, RDIM_String8 string)
{
  RDIM_BakePathNode *path_node = rdim_bake_path_node_from_string(tree, string);
  RDI_U32 result = 0;
  if(path_node != 0)
  {
    result = path_node->idx;
  }
  return result;
}

RDI_PROC RDIM_BakePathNode *
rdim_bake_path_tree_insert(RDIM_Arena *arena, RDIM_BakePathTree *tree, RDIM_String8 string)
{
  RDIM_BakePathNode *node = &tree->root;
  RDI_U8 *ptr = string.str;
  RDI_U8 *opl = string.str + string.size;
  for(;ptr < opl;)
  {
    // rjf: skip past slashes
    for(;ptr < opl && (*ptr == '/' || *ptr == '\\'); ptr += 1);
    
    // rjf: save beginning of non-slash range
    RDI_U8 *range_first = ptr;
    
    // rjf: skip past non-slashes
    for(;ptr < opl && !(*ptr == '/' || *ptr == '\\'); ptr += 1);
    
    // rjf: empty range -> continue
    if(range_first >= ptr)
    {
      continue;
    }
    
    // rjf: range -> sub-directory string
    RDIM_String8 sub_dir = rdim_str8(range_first, (RDI_U64)(ptr-range_first));
    
    // rjf: sub-directory string -> find child of node
    RDIM_BakePathNode *sub_dir_node = 0;
    for(RDIM_BakePathNode *child = node->first_child; child != 0; child = child->next_sibling)
    {
      if(rdim_str8_match(child->name, sub_dir, 0))
      {
        sub_dir_node = child;
      }
    }
    
    // rjf: .. -> go up
    if(sub_dir.RDIM_String8_SizeMember == 2 &&
       sub_dir.RDIM_String8_BaseMember[0] == '.' &&
       sub_dir.RDIM_String8_BaseMember[1] == '.')
    {
      sub_dir_node = node->parent;
      if(sub_dir_node == 0)
      {
        sub_dir_node = &tree->root;
      }
    }
    
    // rjf: . -> stay here
    else if(sub_dir.RDIM_String8_SizeMember == 1 &&
            sub_dir.RDIM_String8_BaseMember[0] == '.')
    {
      sub_dir_node = node;
    }
    
    // rjf: no child -> make one
    else if(sub_dir_node == 0)
    {
      sub_dir_node = rdim_push_array(arena, RDIM_BakePathNode, 1);
      RDIM_SLLQueuePush_N(tree->first, tree->last, sub_dir_node, next_order);
      sub_dir_node->parent = node;
      RDIM_SLLQueuePush_N(node->first_child, node->last_child, sub_dir_node, next_sibling);
      sub_dir_node->name = rdim_str8_copy(arena, sub_dir);
      sub_dir_node->idx = tree->count;
      tree->count += 1;
    }
    
    // rjf: descend to child
    node = sub_dir_node;
  }
  return node;
}

////////////////////////////////
//~ rjf: [Baking Helpers] Data Section List Building Helpers

RDI_PROC RDIM_BakeSection *
rdim_bake_section_list_push(RDIM_Arena *arena, RDIM_BakeSectionList *list)
{
  RDIM_BakeSectionNode *n = rdim_push_array(arena, RDIM_BakeSectionNode, 1);
  RDIM_SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  RDIM_BakeSection *result = &n->v;
  return result;
}

RDI_PROC RDIM_BakeSection *
rdim_bake_section_list_push_new_unpacked(RDIM_Arena *arena, RDIM_BakeSectionList *list, void *data, RDI_U64 size, RDI_SectionKind tag, RDI_U64 tag_idx)
{
  RDIM_BakeSection *section = rdim_bake_section_list_push(arena, list);
  section->data = data;
  section->encoding = RDI_SectionEncoding_Unpacked;
  section->encoded_size = size;
  section->unpacked_size = size;
  section->tag = tag;
  section->tag_idx = tag_idx;
  return section;
}

RDI_PROC void
rdim_bake_section_list_concat_in_place(RDIM_BakeSectionList *dst, RDIM_BakeSectionList *to_push)
{
  if(dst->last != 0 && to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->count += to_push->count;
  }
  else if(to_push->first != 0)
  {
    rdim_memcpy_struct(dst, to_push);
  }
  rdim_memzero_struct(to_push);
}

////////////////////////////////
//~ rjf: [Serializing] Bake Results -> String Blobs

RDI_PROC RDIM_SerializedSection
rdim_serialized_section_make_unpacked(void *data, RDI_U64 size)
{
  RDIM_SerializedSection s;
  rdim_memzero_struct(&s);
  s.data = data;
  s.encoded_size = s.unpacked_size = size;
  s.encoding = RDI_SectionEncoding_Unpacked;
  return s;
}

RDI_PROC RDIM_SerializedSectionBundle
rdim_serialized_section_bundle_from_bake_results(RDIM_BakeResults *results)
{
  RDIM_SerializedSectionBundle bundle;
  rdim_memzero_struct(&bundle);
  bundle.sections[RDI_SectionKind_TopLevelInfo]         = rdim_serialized_section_make_unpacked_struct(&results->top_level_info.top_level_info);
  bundle.sections[RDI_SectionKind_StringData]           = rdim_serialized_section_make_unpacked_array(results->strings.string_data, results->strings.string_data_size);
  bundle.sections[RDI_SectionKind_StringTable]          = rdim_serialized_section_make_unpacked_array(results->strings.string_offs, results->strings.string_offs_count);
  bundle.sections[RDI_SectionKind_IndexRuns]            = rdim_serialized_section_make_unpacked_array(results->idx_runs.idx_runs, results->idx_runs.idx_count);
  bundle.sections[RDI_SectionKind_BinarySections]       = rdim_serialized_section_make_unpacked_array(results->binary_sections.binary_sections, results->binary_sections.binary_sections_count);
  bundle.sections[RDI_SectionKind_FilePathNodes]        = rdim_serialized_section_make_unpacked_array(results->file_paths.nodes, results->file_paths.nodes_count);
  bundle.sections[RDI_SectionKind_SourceFiles]          = rdim_serialized_section_make_unpacked_array(results->src_files.source_files, results->src_files.source_files_count);
  bundle.sections[RDI_SectionKind_LineTables]           = rdim_serialized_section_make_unpacked_array(results->line_tables.line_tables, results->line_tables.line_tables_count);
  bundle.sections[RDI_SectionKind_LineInfoVOffs]        = rdim_serialized_section_make_unpacked_array(results->line_tables.line_table_voffs, results->line_tables.line_table_voffs_count);
  bundle.sections[RDI_SectionKind_LineInfoLines]        = rdim_serialized_section_make_unpacked_array(results->line_tables.line_table_lines, results->line_tables.line_table_lines_count);
  bundle.sections[RDI_SectionKind_LineInfoColumns]      = rdim_serialized_section_make_unpacked_array(results->line_tables.line_table_columns, results->line_tables.line_table_columns_count);
  bundle.sections[RDI_SectionKind_SourceLineMaps]       = rdim_serialized_section_make_unpacked_array(results->src_files.source_line_maps, results->src_files.source_line_maps_count);
  bundle.sections[RDI_SectionKind_SourceLineMapNumbers] = rdim_serialized_section_make_unpacked_array(results->src_files.source_line_map_nums, results->src_files.source_line_map_nums_count);
  bundle.sections[RDI_SectionKind_SourceLineMapRanges]  = rdim_serialized_section_make_unpacked_array(results->src_files.source_line_map_rngs, results->src_files.source_line_map_rngs_count);
  bundle.sections[RDI_SectionKind_SourceLineMapVOffs]   = rdim_serialized_section_make_unpacked_array(results->src_files.source_line_map_voffs, results->src_files.source_line_map_voffs_count);
  bundle.sections[RDI_SectionKind_Units]                = rdim_serialized_section_make_unpacked_array(results->units.units, results->units.units_count);
  bundle.sections[RDI_SectionKind_UnitVMap]             = rdim_serialized_section_make_unpacked_array(results->unit_vmap.vmap.vmap, results->unit_vmap.vmap.count);
  bundle.sections[RDI_SectionKind_TypeNodes]            = rdim_serialized_section_make_unpacked_array(results->type_nodes.type_nodes, results->type_nodes.type_nodes_count);
  bundle.sections[RDI_SectionKind_UDTs]                 = rdim_serialized_section_make_unpacked_array(results->udts.udts, results->udts.udts_count);
  bundle.sections[RDI_SectionKind_Members]              = rdim_serialized_section_make_unpacked_array(results->udts.members, results->udts.members_count);
  bundle.sections[RDI_SectionKind_EnumMembers]          = rdim_serialized_section_make_unpacked_array(results->udts.enum_members, results->udts.enum_members_count);
  bundle.sections[RDI_SectionKind_GlobalVariables]      = rdim_serialized_section_make_unpacked_array(results->global_variables.global_variables, results->global_variables.global_variables_count);
  bundle.sections[RDI_SectionKind_GlobalVMap]           = rdim_serialized_section_make_unpacked_array(results->global_vmap.vmap.vmap, results->global_vmap.vmap.count);
  bundle.sections[RDI_SectionKind_ThreadVariables]      = rdim_serialized_section_make_unpacked_array(results->thread_variables.thread_variables, results->thread_variables.thread_variables_count);
  bundle.sections[RDI_SectionKind_Constants]            = rdim_serialized_section_make_unpacked_array(results->constants.constants, results->constants.constants_count);
  bundle.sections[RDI_SectionKind_Procedures]           = rdim_serialized_section_make_unpacked_array(results->procedures.procedures, results->procedures.procedures_count);
  bundle.sections[RDI_SectionKind_Scopes]               = rdim_serialized_section_make_unpacked_array(results->scopes.scopes, results->scopes.scopes_count);
  bundle.sections[RDI_SectionKind_ScopeVOffData]        = rdim_serialized_section_make_unpacked_array(results->scopes.scope_voffs, results->scopes.scope_voffs_count);
  bundle.sections[RDI_SectionKind_ScopeVMap]            = rdim_serialized_section_make_unpacked_array(results->scope_vmap.vmap.vmap, results->scope_vmap.vmap.count);
  bundle.sections[RDI_SectionKind_InlineSites]          = rdim_serialized_section_make_unpacked_array(results->inline_sites.inline_sites, results->inline_sites.inline_sites_count);
  bundle.sections[RDI_SectionKind_Locals]               = rdim_serialized_section_make_unpacked_array(results->scopes.locals, results->scopes.locals_count);
  bundle.sections[RDI_SectionKind_LocationBlocks]       = rdim_serialized_section_make_unpacked_array(results->location_blocks.location_blocks, results->location_blocks.location_blocks_count);
  bundle.sections[RDI_SectionKind_LocationData]         = rdim_serialized_section_make_unpacked_array(results->locations.location_data, results->locations.location_data_size);
  bundle.sections[RDI_SectionKind_ConstantValueData]    = rdim_serialized_section_make_unpacked_array(results->constants.constant_value_data, results->constants.constant_value_data_size);
  bundle.sections[RDI_SectionKind_ConstantValueTable]   = rdim_serialized_section_make_unpacked_array(results->constants.constant_values, results->constants.constant_values_count);
  bundle.sections[RDI_SectionKind_MD5Checksums]         = rdim_serialized_section_make_unpacked_array(results->checksums.md5s, results->checksums.md5s_count);
  bundle.sections[RDI_SectionKind_SHA1Checksums]        = rdim_serialized_section_make_unpacked_array(results->checksums.sha1s, results->checksums.sha1s_count);
  bundle.sections[RDI_SectionKind_SHA256Checksums]      = rdim_serialized_section_make_unpacked_array(results->checksums.sha256s, results->checksums.sha256s_count);
  bundle.sections[RDI_SectionKind_NameMaps]             = rdim_serialized_section_make_unpacked_array(results->top_level_name_maps.name_maps, results->top_level_name_maps.name_maps_count);
  bundle.sections[RDI_SectionKind_NameMapBuckets]       = rdim_serialized_section_make_unpacked_array(results->name_maps.buckets, results->name_maps.buckets_count);
  bundle.sections[RDI_SectionKind_NameMapNodes]         = rdim_serialized_section_make_unpacked_array(results->name_maps.nodes, results->name_maps.nodes_count);
  return bundle;
}

RDI_PROC RDIM_String8List
rdim_file_blobs_from_section_bundle(RDIM_Arena *arena, RDIM_SerializedSectionBundle *bundle)
{
  RDIM_String8List strings;
  rdim_memzero_struct(&strings);
  {
    RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
    
    // rjf: push empty header & data section table
    RDI_Header *rdi_header = rdim_push_array(arena, RDI_Header, 1);
    RDI_Section *rdi_sections = rdim_push_array(arena, RDI_Section, RDI_SectionKind_COUNT);
    rdim_str8_list_push(arena, &strings, rdim_str8_struct(rdi_header));
    rdim_str8_list_push_align(arena, &strings, 8);
    U32 data_section_off = (U32)strings.total_size;
    rdim_str8_list_push(arena, &strings, rdim_str8((RDI_U8 *)rdi_sections, sizeof(RDI_Section)*RDI_SectionKind_COUNT));
    
    // rjf: fill baked header
    {
      rdi_header->magic              = RDI_MAGIC_CONSTANT;
      rdi_header->encoding_version   = RDI_ENCODING_VERSION;
      rdi_header->data_section_off   = data_section_off;
      rdi_header->data_section_count = RDI_SectionKind_COUNT;
    }
    
    // rjf: fill baked data section table
    for(RDI_SectionKind k = RDI_SectionKind_NULL; k < RDI_SectionKind_COUNT; k += 1)
    {
      RDI_Section *dst = rdi_sections+k;
      U64 data_section_off = 0;
      if(bundle->sections[k].encoded_size != 0)
      {
        rdim_str8_list_push_align(arena, &strings, 8);
        data_section_off = strings.total_size;
        rdim_str8_list_push(arena, &strings, rdim_str8((RDI_U8 *)bundle->sections[k].data, bundle->sections[k].encoded_size));
      }
      dst->encoding      = bundle->sections[k].encoding;
      dst->off           = data_section_off;
      dst->encoded_size  = bundle->sections[k].encoded_size;
      dst->unpacked_size = bundle->sections[k].unpacked_size;
    }
    
    rdim_scratch_end(scratch);
  }
  return strings;
}
