// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: API Implementation Helper Macros

#define rdim_require(root, b32, else_code, error_msg)  do { if(!(b32)) {rdim_push_msg((root), (error_msg)); else_code;} }while(0)
#define rdim_requiref(root, b32, else_code, fmt, ...)  do { if(!(b32)) {rdim_push_msgf((root), (fmt), __VA_ARGS__); else_code;} }while(0)

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
rdim_arena_push_fallback(RDIM_Arena *arena, RDI_U64 size)
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
        SLLQueuePush(ranges_first, ranges_last, new_range);
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
          SLLStackPop(src_ranges);
          
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
            SLLQueuePush(dst_ranges, dst_ranges_last, range1);
            break;
          }
          
          // get second range
          RDIM_OrderedRange *range2 = src_ranges;
          SLLStackPop(src_ranges);
          
          rdim_assert(range1->opl == range2->first);
          
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
  
  scratch_end(scratch);
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

////////////////////////////////
//~ rjf: Binary Section List Building

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
//~ rjf: Unit List Building

RDI_PROC RDIM_Unit *
rdim_unit_chunk_list_push(RDIM_Arena *arena, RDIM_UnitChunkList *list, RDI_U64 cap)
{
  RDIM_UnitChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = rdim_push_array(arena, RDIM_UnitChunkNode, 1);
    n->cap = cap;
    n->base_idx = list->total_count;
    n->v = rdim_push_array(arena, RDIM_Unit, n->cap);
    RDIM_SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }
  RDIM_Unit *unit = &n->v[n->count];
  unit->chunk = n;
  n->count += 1;
  list->total_count += 1;
  return unit;
}

RDI_PROC RDI_U64
rdim_idx_from_unit(RDIM_Unit *unit)
{
  RDI_U64 idx = 0;
  if(unit != 0 && unit->chunk != 0)
  {
    idx = unit->chunk->base_idx + (unit - unit->chunk->v);
  }
  return idx;
}

RDI_PROC void
rdim_unit_chunk_list_concat_in_place(RDIM_UnitChunkList *dst, RDIM_UnitChunkList *to_push)
{
  for(RDIM_UnitChunkNode *n = to_push->first; n != 0; n = n->next)
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

RDI_PROC RDIM_LineSequence *
rdim_line_sequence_list_push(RDIM_Arena *arena, RDIM_LineSequenceList *list)
{
  RDIM_LineSequenceNode *n = rdim_push_array(arena, RDIM_LineSequenceNode, 1);
  RDIM_SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  return &n->v;
}

////////////////////////////////
//~ rjf: Type Info Building

RDI_PROC RDIM_Type *
rdim_type_chunk_list_push(RDIM_Arena *arena, RDIM_TypeChunkList *list, RDI_U64 cap)
{
  RDIM_TypeChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = rdim_push_array(arena, RDIM_TypeChunkNode, 1);
    n->cap = cap;
    n->base_idx = list->total_count;
    n->v = rdim_push_array(arena, RDIM_Type, n->cap);
    RDIM_SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }
  RDIM_Type *result = &n->v[n->count];
  result->chunk = n;
  n->count += 1;
  list->total_count += 1;
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_type(RDIM_Type *type)
{
  RDI_U64 idx = 0;
  if(type != 0 && type->chunk != 0)
  {
    idx = type->chunk->base_idx + (type - type->chunk->v);
  }
  return idx;
}

RDI_PROC void
rdim_type_chunk_list_concat_in_place(RDIM_TypeChunkList *dst, RDIM_TypeChunkList *to_push)
{
  for(RDIM_TypeChunkNode *n = to_push->first; n != 0; n = n->next)
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

RDI_PROC RDIM_UDT *
rdim_udt_chunk_list_push(RDIM_Arena *arena, RDIM_UDTChunkList *list, RDI_U64 cap)
{
  RDIM_UDTChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = rdim_push_array(arena, RDIM_UDTChunkNode, 1);
    n->cap = cap;
    n->base_idx = list->total_count;
    n->v = rdim_push_array(arena, RDIM_UDT, n->cap);
    RDIM_SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }
  RDIM_UDT *result = &n->v[n->count];
  result->chunk = n;
  n->count += 1;
  list->total_count += 1;
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_udt(RDIM_UDT *udt)
{
  RDI_U64 idx = 0;
  if(udt != 0 && udt->chunk != 0)
  {
    idx = udt->chunk->base_idx + (udt - udt->chunk->v);
  }
  return idx;
}

RDI_PROC void
rdim_udt_chunk_list_concat_in_place(RDIM_UDTChunkList *dst, RDIM_UDTChunkList *to_push)
{
  for(RDIM_UDTChunkNode *n = to_push->first; n != 0; n = n->next)
  {
    n->base_idx += dst->total_count;
  }
  if(dst->last != 0 && to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->chunk_count += to_push->chunk_count;
    dst->total_count += to_push->total_count;
    dst->total_member_count += to_push->total_member_count;
    dst->total_enum_val_count += to_push->total_enum_val_count;
  }
  else if(dst->first == 0)
  {
    rdim_memcpy_struct(dst, to_push);
  }
  rdim_memzero_struct(to_push);
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
//~ rjf: Symbol Info Building

RDI_PROC RDIM_Symbol *
rdim_symbol_chunk_list_push(RDIM_Arena *arena, RDIM_SymbolChunkList *list, RDI_U64 cap)
{
  RDIM_SymbolChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = rdim_push_array(arena, RDIM_SymbolChunkNode, 1);
    n->cap = cap;
    n->base_idx = list->total_count;
    n->v = rdim_push_array(arena, RDIM_Symbol, n->cap);
    RDIM_SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }
  RDIM_Symbol *result = &n->v[n->count];
  result->chunk = n;
  n->count += 1;
  list->total_count += 1;
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_symbol(RDIM_Symbol *symbol)
{
  RDI_U64 idx = 0;
  if(symbol != 0 && symbol->chunk != 0)
  {
    idx = symbol->chunk->base_idx + (symbol - symbol->chunk->v);
  }
  return idx;
}

RDI_PROC void
rdim_symbol_chunk_list_concat_in_place(RDIM_SymbolChunkList *dst, RDIM_SymbolChunkList *to_push)
{
  for(RDIM_SymbolChunkNode *n = to_push->first; n != 0; n = n->next)
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

////////////////////////////////
//~ rjf: Scope Info Building

//- rjf: scopes

RDI_PROC RDIM_Scope *
rdim_scope_chunk_list_push(RDIM_Arena *arena, RDIM_ScopeChunkList *list, RDI_U64 cap)
{
  RDIM_ScopeChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = rdim_push_array(arena, RDIM_ScopeChunkNode, 1);
    n->cap = cap;
    n->base_idx = list->total_count;
    n->v = rdim_push_array(arena, RDIM_Scope, n->cap);
    RDIM_SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }
  RDIM_Scope *result = &n->v[n->count];
  result->chunk = n;
  n->count += 1;
  list->total_count += 1;
  return result;
}

RDI_PROC RDI_U64
rdim_idx_from_scope(RDIM_Scope *scope)
{
  RDI_U64 idx = 0;
  if(scope != 0 && scope->chunk != 0)
  {
    idx = scope->chunk->base_idx + (scope - scope->chunk->v);
  }
  return idx;
}

RDI_PROC void
rdim_scope_chunk_list_concat_in_place(RDIM_ScopeChunkList *dst, RDIM_ScopeChunkList *to_push)
{
  for(RDIM_ScopeChunkNode *n = to_push->first; n != 0; n = n->next)
  {
    n->base_idx += dst->total_count;
  }
  if(dst->last != 0 && to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->chunk_count += to_push->chunk_count;
    dst->total_count += to_push->total_count;
    dst->scope_voff_count += to_push->scope_voff_count;
    dst->local_count += to_push->local_count;
    dst->location_count += to_push->location_count;
  }
  else if(dst->first == 0)
  {
    rdim_memcpy_struct(dst, to_push);
  }
  rdim_memzero_struct(to_push);
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

//- rjf: bytecode

RDI_PROC void
rdim_bytecode_push_op(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_EvalOp op, RDI_U64 p)
{
  RDI_U8 ctrlbits = rdi_eval_opcode_ctrlbits[op];
  RDI_U32 p_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
  
  RDIM_EvalBytecodeOp *node = rdim_push_array(arena, RDIM_EvalBytecodeOp, 1);
  node->op = op;
  node->p_size = p_size;
  node->p = p;
  
  RDIM_SLLQueuePush(bytecode->first_op, bytecode->last_op, node);
  bytecode->op_count += 1;
  bytecode->encoded_size += 1 + p_size;
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
      left_dst->last_op = right_destroyed->last_op;
      left_dst->op_count += right_destroyed->op_count;
      left_dst->encoded_size += right_destroyed->encoded_size;
    }
    rdim_memzero_struct(right_destroyed);
  }
}

//- rjf: individual locations

RDI_PROC RDIM_Location *
rdim_push_location_addr_bytecode_stream(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode)
{
  RDIM_Location *result = rdim_push_array(arena, RDIM_Location, 1);
  result->kind = RDI_LocationKind_AddrBytecodeStream;
  result->bytecode = *bytecode;
  return result;
}

RDI_PROC RDIM_Location *
rdim_push_location_val_bytecode_stream(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode)
{
  RDIM_Location *result = rdim_push_array(arena, RDIM_Location, 1);
  result->kind = RDI_LocationKind_ValBytecodeStream;
  result->bytecode = *bytecode;
  return result;
}

RDI_PROC RDIM_Location *
rdim_push_location_addr_reg_plus_u16(RDIM_Arena *arena, RDI_U8 reg_code, RDI_U16 offset)
{
  RDIM_Location *result = rdim_push_array(arena, RDIM_Location, 1);
  result->kind = RDI_LocationKind_AddrRegisterPlusU16;
  result->register_code = reg_code;
  result->offset = offset;
  return result;
}

RDI_PROC RDIM_Location *
rdim_push_location_addr_addr_reg_plus_u16(RDIM_Arena *arena, RDI_U8 reg_code, RDI_U16 offset)
{
  RDIM_Location *result = rdim_push_array(arena, RDIM_Location, 1);
  result->kind = RDI_LocationKind_AddrAddrRegisterPlusU16;
  result->register_code = reg_code;
  result->offset = offset;
  return result;
}

RDI_PROC RDIM_Location *
rdim_push_location_val_reg(RDIM_Arena *arena, RDI_U8 reg_code)
{
  RDIM_Location *result = rdim_push_array(arena, RDIM_Location, 1);
  result->kind = RDI_LocationKind_ValRegister;
  result->register_code = reg_code;
  return result;
}

//- rjf: location sets

RDI_PROC void
rdim_location_set_push_case(RDIM_Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_LocationSet *locset, RDIM_Rng1U64 voff_range, RDIM_Location *location)
{
  RDIM_LocationCase *location_case = rdim_push_array(arena, RDIM_LocationCase, 1);
  SLLQueuePush(locset->first_location_case, locset->last_location_case, location_case);
  locset->location_case_count += 1;
  location_case->voff_range = voff_range;
  location_case->location   = location;
  scopes->location_count +=1;
}

////////////////////////////////
//~ rjf: Baking

//- rjf: data section list building helpers

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
rdim_bake_section_list_push_new(RDIM_Arena *arena, RDIM_BakeSectionList *list, void *data, RDI_U64 size, RDI_DataSectionTag tag)
{
  RDIM_BakeSection *section = rdim_bake_section_list_push(arena, list);
  section->data = data;
  section->size = size;
  section->tag = tag;
  return section;
}

//- rjf: interned string building

RDI_PROC RDI_U32
rdim_bake_string(RDIM_Arena *arena, RDIM_BakeStringMap *map, RDIM_String8 string)
{
  RDI_U64 hash = rdi_hash(string.RDIM_String8_BaseMember, string.RDIM_String8_SizeMember);
  RDI_U64 slot_idx = hash%map->slots_count;
  
  // rjf: find existing node
  RDIM_BakeStringNode *node = 0;
  for(RDIM_BakeStringNode *n = map->slots[slot_idx]; n != 0; n = n->hash_next)
  {
    if(n->hash == hash && rdim_str8_match(n->string, string, 0))
    {
      node = n;
      break;
    }
  }
  
  // rjf: no node -> make new node
  if(node == 0)
  {
    node = rdim_push_array(arena, RDIM_BakeStringNode, 1);
    node->string = rdim_str8_copy(arena, string);
    node->hash   = hash;
    node->idx    = map->count;
    map->count += 1;
    RDIM_SLLQueuePush_N(map->order_first, map->order_last, node, order_next);
    RDIM_SLLStackPush_N(map->slots[slot_idx], node, hash_next);
    map->slot_collision_count += (node->hash_next != 0);
  }
  
  // rjf: node -> index
  RDI_U32 result = node->idx;
  return result;
}

//- rjf: interned index run building

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

RDI_PROC RDI_U32
rdim_bake_idx_run(RDIM_Arena *arena, RDIM_BakeIdxRunMap *map, RDI_U32 *idx_run, RDI_U32 count)
{
  RDI_U64 hash = rdim_hash_from_idx_run(idx_run, count);
  RDI_U64 slot_idx = hash%map->slots_count;
  
  // rjf: find existing node
  RDIM_BakeIdxRunNode *node = 0;
  for(RDIM_BakeIdxRunNode *n = map->slots[slot_idx]; n != 0; n = n->hash_next)
  {
    if(n->hash == hash)
    {
      RDI_S32 is_match = 1;
      RDI_U32 *n_idx = n->idx_run;
      for(RDI_U32 i = 0; i < count; i += 1)
      {
        if(n_idx[i] != idx_run[i])
        {
          is_match = 0;
          break;
        }
      }
      if(is_match)
      {
        node = n;
        break;
      }
    }
  }
  
  // rjf: no node -> make new node
  if(node == 0)
  {
    node = rdim_push_array_no_zero(arena, RDIM_BakeIdxRunNode, 1);
    RDI_U32 *idx_run_copy = rdim_push_array_no_zero(arena, RDI_U32, count);
    for(RDI_U32 i = 0; i < count; i += 1)
    {
      idx_run_copy[i] = idx_run[i];
    }
    node->idx_run = idx_run_copy;
    node->hash = hash;
    node->count = count;
    node->first_idx = map->idx_count;
    map->count += 1;
    map->idx_count += count;
    RDIM_SLLQueuePush_N(map->order_first, map->order_last, node, order_next);
    RDIM_SLLStackPush_N(map->slots[slot_idx], node, hash_next);
    map->slot_collision_count += (node->hash_next != 0);
  }
  
  // rjf: node -> index
  RDI_U32 result = node->first_idx;
  return result;
}

//- rjf: interned path/file building

RDI_PROC RDIM_String8
rdim_normal_string_from_bake_path_node(RDIM_Arena *arena, RDIM_BakePathNode *node)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  RDIM_String8List list = {0};
  for(RDIM_BakePathNode *n = node; n != 0; n = n->parent)
  {
    if(n->name.size != 0)
    {
      rdim_str8_list_push(scratch.arena, &list, n->name);
    }
  }
  RDIM_String8 result = rdim_str8_list_join(arena, &list, rdim_str8_lit("/"));
  {
    RDI_U8 *ptr = result.str;
    RDI_U8 *opl = result.str + result.size;
    for(;ptr < opl; ptr += 1)
    {
      RDI_U8 c = *ptr;
      if('A' <= c && c <= 'Z') { c += 'a' - 'A'; }
      *ptr = c;
    }
  }
  scratch_end(scratch);
  return result;
}

RDI_PROC RDIM_BakePathNode *
rdim_bake_path_node_from_string(RDIM_Arena *arena, RDIM_BakePathTree *tree, RDIM_String8 string)
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
      if(rdim_str8_match(child->name, sub_dir, RDIM_StringMatchFlag_CaseInsensitive))
      {
        sub_dir_node = child;
      }
    }
    
    // rjf: no child -> make one
    if(sub_dir_node == 0)
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

RDI_PROC RDIM_BakeSrcNode *
rdim_bake_src_node_from_path_node(RDIM_Arena *arena, RDIM_BakePathTree *tree, RDIM_BakePathNode *path_node)
{
  RDIM_BakeSrcNode *src_node = path_node->src_file;
  if(src_node == 0)
  {
    src_node = rdim_push_array(arena, RDIM_BakeSrcNode, 1);
    path_node->src_file = src_node;
    src_node->path_node = path_node;
    src_node->normal_full_path = rdim_normal_string_from_bake_path_node(arena, path_node);
    src_node->idx = tree->src_count;
    tree->src_count += 1;
    RDIM_SLLQueuePush(tree->src_first, tree->src_last, src_node);
  }
  return src_node;
}

RDI_PROC RDI_U32
rdim_bake_path(RDIM_Arena *arena, RDIM_BakePathTree *tree, RDIM_String8 string)
{
  RDIM_BakePathNode *path_node = rdim_bake_path_node_from_string(arena, tree, string);
  return path_node->idx;
}

//- rjf: name maps

RDI_PROC void
rdim_bake_name_map_push(RDIM_Arena *arena, RDIM_BakeNameMap *map, RDIM_String8 string, RDI_U32 idx)
{
  // rjf: hash
  RDI_U64 hash = rdi_hash(string.RDIM_String8_BaseMember, string.RDIM_String8_SizeMember);
  RDI_U64 slot_idx = hash%map->slots_count;
  
  // rjf: find existing node
  RDIM_BakeNameMapNode *node = 0;
  for(RDIM_BakeNameMapNode *n = map->slots[slot_idx]; n != 0; n = n->slot_next)
  {
    if(rdim_str8_match(string, n->string, 0))
    {
      node = n;
      break;
    }
  }
  
  // rjf: make node if necessary
  if(node == 0)
  {
    node = rdim_push_array(arena, RDIM_BakeNameMapNode, 1);
    node->string = rdim_str8_copy(arena, string);
    RDIM_SLLStackPush_N(map->slots[slot_idx], node, slot_next);
    RDIM_SLLQueuePush_N(map->first, map->last, node, order_next);
    map->name_count += 1;
    map->slot_collision_count += (node->slot_next != 0);
  }
  
  // rjf: find existing idx
  RDI_S32 existing_idx = 0;
  for(RDIM_BakeNameMapValNode *n = node->val_first; n != 0; n = n->next)
  {
    for(RDI_U32 i = 0; i < sizeof(n->val)/sizeof(n->val[0]); i += 1)
    {
      if(n->val[i] == 0)
      {
        break;
      }
      if(n->val[i] == idx)
      {
        existing_idx = 1;
        break;
      }
    }
  }
  
  // rjf: insert new idx if necessary
  if(!existing_idx)
  {
    RDIM_BakeNameMapValNode *val_node = node->val_last;
    RDI_U32 insert_i = node->val_count%(sizeof(val_node->val)/sizeof(val_node->val[0]));
    if(insert_i == 0)
    {
      val_node = rdim_push_array(arena, RDIM_BakeNameMapValNode, 1);
      SLLQueuePush(node->val_first, node->val_last, val_node);
    }
    val_node->val[insert_i] = idx;
    node->val_count += 1;
  }
}

//- rjf: vmap baking

RDI_PROC RDIM_VMap
rdim_vmap_from_markers(RDIM_Arena *arena, RDIM_VMapMarker *markers, RDIM_SortKey *keys, RDI_U64 marker_count)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  
  //- rjf: sort markers
  RDIM_SortKey *sorted_keys = rdim_sort_key_array(scratch.arena, keys, marker_count);
  
  //- rjf: determine if an extra vmap entry for zero is needed
  RDI_U32 extra_vmap_entry = 0;
  if(marker_count > 0 && sorted_keys[0].key != 0)
  {
    extra_vmap_entry = 1;
  }
  
  //- rjf: fill output vmap entries
  RDI_U32 vmap_count_raw = marker_count - 1 + extra_vmap_entry;
  RDI_VMapEntry *vmap = rdim_push_array_no_zero(arena, RDI_VMapEntry, vmap_count_raw + 1);
  RDI_U32 vmap_entry_count_pass_1 = 0;
  {
    typedef struct RDIM_VMapRangeTracker RDIM_VMapRangeTracker;
    struct RDIM_VMapRangeTracker
    {
      RDIM_VMapRangeTracker *next;
      RDI_U32 idx;
    };
    RDI_VMapEntry *vmap_ptr = vmap;
    if(extra_vmap_entry)
    {
      vmap_ptr->voff = 0;
      vmap_ptr->idx = 0;
      vmap_ptr += 1;
    }
    RDIM_VMapRangeTracker *tracker_stack = 0;
    RDIM_VMapRangeTracker *tracker_free = 0;
    RDIM_SortKey *key_ptr = sorted_keys;
    RDIM_SortKey *key_opl = sorted_keys + marker_count;
    for(;key_ptr < key_opl;)
    {
      // rjf: get initial map state from tracker stack
      RDI_U32 initial_idx = (RDI_U32)0xffffffff;
      if(tracker_stack != 0)
      {
        initial_idx = tracker_stack->idx;
      }
      
      // rjf: update tracker stack
      //
      // * we must process _all_ of the changes that apply at this voff before moving on
      //
      RDI_U64 voff = key_ptr->key;
      
      for(;key_ptr < key_opl && key_ptr->key == voff; key_ptr += 1)
      {
        RDIM_VMapMarker *marker = (RDIM_VMapMarker*)key_ptr->val;
        RDI_U32 idx = marker->idx;
        
        // rjf: range begin -> push to stack
        if(marker->begin_range)
        {
          RDIM_VMapRangeTracker *new_tracker = tracker_free;
          if(new_tracker != 0)
          {
            RDIM_SLLStackPop(tracker_free);
          }
          else
          {
            new_tracker = rdim_push_array(scratch.arena, RDIM_VMapRangeTracker, 1);
          }
          RDIM_SLLStackPush(tracker_stack, new_tracker);
          new_tracker->idx = idx;
        }
        
        // rjf: range ending -> pop matching node from stack (not always the top)
        else
        {
          RDIM_VMapRangeTracker **ptr_in = &tracker_stack;
          RDIM_VMapRangeTracker *match = 0;
          for(RDIM_VMapRangeTracker *node = tracker_stack; node != 0;)
          {
            if(node->idx == idx)
            {
              match = node;
              break;
            }
            ptr_in = &node->next;
            node = node->next;
          }
          if(match != 0)
          {
            *ptr_in = match->next;
            RDIM_SLLStackPush(tracker_free, match);
          }
        }
      }
      
      // rjf: get final map state from tracker stack
      RDI_U32 final_idx = 0;
      if(tracker_stack != 0)
      {
        final_idx = tracker_stack->idx;
      }
      
      // rjf: if final is different from initial - emit new vmap entry
      if(final_idx != initial_idx)
      {
        vmap_ptr->voff = voff;
        vmap_ptr->idx = final_idx;
        vmap_ptr += 1;
      }
    }
    
    vmap_entry_count_pass_1 = (RDI_U32)(vmap_ptr - vmap); // TODO(rjf): @u64_to_u32
  }
  
  //- rjf: combine duplicate neighbors
  RDI_U32 vmap_entry_count = 0;
  {
    RDI_VMapEntry *vmap_ptr = vmap;
    RDI_VMapEntry *vmap_opl = vmap + vmap_entry_count_pass_1;
    RDI_VMapEntry *vmap_out = vmap;
    for(;vmap_ptr < vmap_opl;)
    {
      RDI_VMapEntry *vmap_range_first = vmap_ptr;
      RDI_U64 idx = vmap_ptr->idx;
      vmap_ptr += 1;
      for(;vmap_ptr < vmap_opl && vmap_ptr->idx == idx;) vmap_ptr += 1;
      rdim_memcpy_struct(vmap_out, vmap_range_first);
      vmap_out += 1;
    }
    vmap_entry_count = (RDI_U32)(vmap_out - vmap); // TODO(rjf): @u64_to_u32
  }
  
  //- rjf: fill result
  RDIM_VMap result = {0};
  result.vmap = vmap;
  result.count = vmap_entry_count-1;
  rdim_scratch_end(scratch);
  return result;
}

//- rjf: main baking entry point

RDI_PROC RDIM_String8List
rdim_bake(RDIM_Arena *arena, RDIM_BakeParams *params)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  RDIM_String8List blobs;
  rdim_memzero_struct(&blobs);
  
  //////////////////////////////
  //- NOTE(rjf): On the ordering of baking phases:
  //
  // Baking is the process of taking all loose data structures passed in via
  // `params`, or constructed on-the-fly (e.g. the `BakeStringMap`), and
  // serializing them down into flat plain-old-data tables which can be
  // written directly into a RADDBGI file.
  //
  // Perhaps unsurprisingly, after each loose data structure is baked, it can
  // no longer be mutated and touched (otherwise you'd need to re-bake to
  // preserve new information added to the loose data structure).
  //
  // The phases of baking in this baking algorithm are organized such that re-
  // baking never occurs. This means that common baking data structures, like
  // the string interning map (`BakeStringMap`), the index run interning map
  // (`BakeIdxRunMap`), and so on, are serialized *last*, because they are
  // used in previous stages of the bake.
  
  //////////////////////////////
  //- rjf: set up intermediate baking data structures
  //
  RDIM_BakeSectionList sections = {0};
  RDIM_BakeStringMap strings = {0};
  RDIM_BakeIdxRunMap idx_runs = {0};
  RDIM_BakePathTree path_tree = {0};
  RDIM_BakeNameMap name_maps[RDI_NameMapKind_COUNT] = {0};
  {
    // TODO(rjf): adjust slots based on good estimates from params
    rdim_bake_section_list_push_new(arena, &sections, 0, 0, RDI_DataSectionTag_NULL);
    strings.slots_count = 65536;
    strings.slots = rdim_push_array(arena, RDIM_BakeStringNode *, strings.slots_count);
    idx_runs.slots_count = 65536;
    idx_runs.slots = rdim_push_array(arena, RDIM_BakeIdxRunNode *, idx_runs.slots_count);
    for(RDI_NameMapKind k = (RDI_NameMapKind)(RDI_NameMapKind_NULL+1);
        k < RDI_NameMapKind_COUNT;
        k = (RDI_NameMapKind)(k+1))
    {
      name_maps[k].slots_count = 65536;
      name_maps[k].slots = rdim_push_array(arena, RDIM_BakeNameMapNode *, name_maps[k].slots_count);
    }
  }
  
  //////////////////////////////
  //- rjf: build section for top-level-info
  //
  RDIM_ProfScope("build section for top-level-info")
  {
    RDI_TopLevelInfo  *dst_tli = rdim_push_array(arena, RDI_TopLevelInfo, 1);
    RDIM_TopLevelInfo *src_tli = &params->top_level_info;
    dst_tli->architecture        = src_tli->arch;
    dst_tli->exe_name_string_idx = rdim_bake_string(arena, &strings, src_tli->exe_name);
    dst_tli->exe_hash            = src_tli->exe_hash;
    dst_tli->voff_max            = src_tli->voff_max;
    rdim_bake_section_list_push_new(arena, &sections, dst_tli, sizeof(*dst_tli), RDI_DataSectionTag_TopLevelInfo);
  }
  
  //////////////////////////////
  //- rjf: build section for binary sections
  //
  RDIM_ProfScope("build section for binary sections")
  {
    RDIM_BinarySectionList *src_list = &params->binary_sections;
    RDI_BinarySection *dst_base = rdim_push_array(arena, RDI_BinarySection, src_list->count);
    U64 dst_idx = 0;
    for(RDIM_BinarySectionNode *src_n = src_list->first; src_n != 0; src_n = src_n->next, dst_idx += 1)
    {
      RDIM_BinarySection *src = &src_n->v;
      RDI_BinarySection *dst = &dst_base[dst_idx];
      dst->name_string_idx = rdim_bake_string(arena, &strings, src->name);
      dst->flags           = src->flags;
      dst->voff_first      = src->voff_first;
      dst->voff_opl        = src->voff_opl;
      dst->foff_first      = src->foff_first;
      dst->foff_opl        = src->foff_opl;
    }
    rdim_bake_section_list_push_new(arena, &sections, dst_base, sizeof(*dst_base)*dst_idx, RDI_DataSectionTag_BinarySections);
  }
  
  //////////////////////////////
  //- rjf: build sections for units
  //
  RDIM_ProfScope("build sections for units")
  {
    RDIM_UnitChunkList *src_list = &params->units;
    RDI_Unit *dst_base = rdim_push_array(arena, RDI_Unit, src_list->total_count);
    RDI_U64 dst_idx = 0;
    for(RDIM_UnitChunkNode *src_n = src_list->first; src_n != 0; src_n = src_n->next)
    {
      for(RDI_U64 src_chunk_idx = 0; src_chunk_idx < src_n->count; src_chunk_idx += 1, dst_idx += 1)
      {
        RDIM_Unit *src = &src_n->v[src_chunk_idx];
        RDI_Unit *dst = &dst_base[dst_idx];
        
        ////////////////////////
        //- rjf: produce combined unit line info
        //
        RDI_U64  *unit_voffs      = 0;
        RDI_Line *unit_lines      = 0;
        RDI_U16  *unit_cols       = 0;
        RDI_U32   unit_line_count = 0;
        {
          RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
          
          //- rjf: gather up all line info into two arrays:
          //
          // [1] keys: sortable array; pairs voffs with line info records; null records are sequence enders
          // [2] recs: contains all the source coordinates for a range of voffs
          //
          typedef struct RDIM_LineRec RDIM_LineRec;
          struct RDIM_LineRec
          {
            RDI_U32 file_id;
            RDI_U32 line_num;
            RDI_U16 col_first;
            RDI_U16 col_opl;
          };
          RDI_U64 line_count = 0;
          RDI_U64 seq_count = 0;
          for(RDIM_LineSequenceNode *seq_n = src->line_sequences.first; seq_n != 0; seq_n = seq_n->next)
          {
            seq_count += 1;
            line_count += seq_n->v.line_count;
          }
          RDI_U64 key_count = line_count + seq_count;
          RDIM_SortKey *line_keys = rdim_push_array_no_zero(scratch.arena, RDIM_SortKey, key_count);
          RDIM_LineRec *line_recs = rdim_push_array_no_zero(scratch.arena, RDIM_LineRec, line_count);
          {
            RDIM_SortKey *key_ptr = line_keys;
            RDIM_LineRec *rec_ptr = line_recs;
            for(RDIM_LineSequenceNode *seq_n = src->line_sequences.first; seq_n != 0; seq_n = seq_n->next)
            {
              RDIM_LineSequence *seq = &seq_n->v;
              RDIM_BakePathNode *src_path = rdim_bake_path_node_from_string(arena, &path_tree, seq->file_name);
              RDIM_BakeSrcNode  *src_file = rdim_bake_src_node_from_path_node(arena, &path_tree, src_path);
              for(RDI_U64 line_idx = 0; line_idx < seq->line_count; line_idx += 1)
              {
                key_ptr->key = seq->voffs[line_idx];
                key_ptr->val = rec_ptr;
                key_ptr += 1;
                rec_ptr->file_id = src_file->idx;
                rec_ptr->line_num = seq->line_nums[line_idx];
                if(seq->col_nums != 0)
                {
                  rec_ptr->col_first = seq->col_nums[line_idx*2];
                  rec_ptr->col_opl = seq->col_nums[line_idx*2 + 1];
                }
                rec_ptr += 1;
              }
              key_ptr->key = seq->voffs[seq->line_count];
              key_ptr->val = 0;
              key_ptr += 1;
              
              RDIM_BakeLineMapFragment *fragment = rdim_push_array(arena, RDIM_BakeLineMapFragment, 1);
              RDIM_SLLQueuePush(src_file->first_fragment, src_file->last_fragment, fragment);
              fragment->seq = seq;
            }
          }
          
          //- rjf: sort
          RDIM_SortKey *sorted_line_keys = rdim_sort_key_array(scratch.arena, line_keys, key_count);
          
          // TODO(rjf): do a pass over sorted keys to make sure duplicate keys
          // are sorted with null record first, and no more than one null
          // record and one non-null record
          
          //- rjf: arrange output
          RDI_U64 *arranged_voffs = rdim_push_array_no_zero(arena, RDI_U64, key_count + 1);
          RDI_Line *arranged_lines = rdim_push_array_no_zero(arena, RDI_Line, key_count);
          for(RDI_U64 i = 0; i < key_count; i += 1)
          {
            arranged_voffs[i] = sorted_line_keys[i].key;
          }
          arranged_voffs[key_count] = ~0ull;
          for(RDI_U64 i = 0; i < key_count; i += 1)
          {
            RDIM_LineRec *rec = (RDIM_LineRec*)sorted_line_keys[i].val;
            if(rec != 0)
            {
              arranged_lines[i].file_idx = rec->file_id;
              arranged_lines[i].line_num = rec->line_num;
            }
            else
            {
              arranged_lines[i].file_idx = 0;
              arranged_lines[i].line_num = 0;
            }
          }
          
          //- rjf: fill output
          unit_voffs      = arranged_voffs;
          unit_lines      = arranged_lines;
          unit_cols       = 0;
          unit_line_count = key_count;
          rdim_scratch_end(scratch);
        }
        
        ////////////////////////
        //- rjf: build line info sections
        //
        RDI_U32 line_info_voffs_data_idx = (RDI_U32)sections.count; // TODO(rjf): @u64_to_u32
        RDI_U32 line_info_data_idx       = line_info_voffs_data_idx+1;
        RDI_U32 line_info_col_data_idx   = unit_cols ? line_info_voffs_data_idx+2 : 0;
        rdim_bake_section_list_push_new(arena, &sections, unit_voffs, sizeof(RDI_U64)*(unit_line_count+1), RDI_DataSectionTag_LineInfoVoffs);
        rdim_bake_section_list_push_new(arena, &sections, unit_lines, sizeof(RDI_Line)*unit_line_count, RDI_DataSectionTag_LineInfoData);
        if(unit_cols != 0)
        {
          rdim_bake_section_list_push_new(arena, &sections, unit_cols, sizeof(RDI_Column)*unit_line_count, RDI_DataSectionTag_LineInfoColumns);
        }
        
        ////////////////////////
        //- rjf: fill output
        //
        dst->unit_name_string_idx     = rdim_bake_string(arena, &strings, src->unit_name);
        dst->compiler_name_string_idx = rdim_bake_string(arena, &strings, src->compiler_name);
        dst->source_file_path_node    = rdim_bake_path(arena, &path_tree, src->source_file);
        dst->object_file_path_node    = rdim_bake_path(arena, &path_tree, src->object_file);
        dst->archive_file_path_node   = rdim_bake_path(arena, &path_tree, src->archive_file);
        dst->build_path_node          = rdim_bake_path(arena, &path_tree, src->build_path);
        dst->language                 = src->language;
        dst->line_info_voffs_data_idx = line_info_voffs_data_idx;
        dst->line_info_data_idx       = line_info_data_idx;
        dst->line_info_col_data_idx   = line_info_col_data_idx;
      }
    }
    rdim_bake_section_list_push_new(arena, &sections, dst_base, sizeof(*dst_base)*dst_idx, RDI_DataSectionTag_Units);
  }
  
  //////////////////////////////
  //- rjf: build section for per-source-file line info
  //
  RDIM_ProfScope("build section for per-source-file line info")
  {
    for(RDIM_BakeSrcNode *src_file_node = path_tree.src_first;
        src_file_node != 0;
        src_file_node = src_file_node->next)
    {
      //////////////////////////
      //- rjf: produce combined source file line info
      //
      RDI_U32 *src_file_line_nums   = 0;
      RDI_U32 *src_file_line_ranges = 0;
      RDI_U64 *src_file_voffs       = 0;
      RDI_U32  src_file_line_count  = 0;
      RDI_U32  src_file_voff_count  = 0;
      {
        RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
        
        //- rjf: gather line number map
        typedef struct RDIM_SrcLineMapVoffBlock RDIM_SrcLineMapVoffBlock;
        struct RDIM_SrcLineMapVoffBlock
        {
          RDIM_SrcLineMapVoffBlock *next;
          RDI_U64 voff;
        };
        typedef struct RDIM_SrcLineMapBucket RDIM_SrcLineMapBucket;
        struct RDIM_SrcLineMapBucket
        {
          RDIM_SrcLineMapBucket *order_next;
          RDIM_SrcLineMapBucket *hash_next;
          RDI_U32 line_num;
          RDIM_SrcLineMapVoffBlock *first_voff_block;
          RDIM_SrcLineMapVoffBlock *last_voff_block;
          RDI_U64 voff_count;
        };
        RDIM_SrcLineMapBucket *first_bucket = 0;
        RDIM_SrcLineMapBucket *last_bucket = 0;
        RDI_U64 line_hash_slots_count = 2048;
        RDIM_SrcLineMapBucket **line_hash_slots = rdim_push_array(scratch.arena, RDIM_SrcLineMapBucket *, line_hash_slots_count);
        RDI_U64 line_count = 0;
        RDI_U64 voff_count = 0;
        RDI_U64 max_line_num = 0;
        {
          for(RDIM_BakeLineMapFragment *map_fragment = src_file_node->first_fragment;
              map_fragment != 0;
              map_fragment = map_fragment->next)
          {
            RDIM_LineSequence *sequence = map_fragment->seq;
            RDI_U64 *seq_voffs = sequence->voffs;
            RDI_U32 *seq_line_nums = sequence->line_nums;
            RDI_U64 seq_line_count = sequence->line_count;
            for(RDI_U64 i = 0; i < seq_line_count; i += 1)
            {
              RDI_U32 line_num = seq_line_nums[i];
              RDI_U64 voff = seq_voffs[i];
              RDI_U64 line_hash_slot_idx = line_num%line_hash_slots_count;
              
              // rjf: update unique voff counter & max line number
              voff_count += 1;
              max_line_num = Max(max_line_num, line_num);
              
              // rjf: find match
              RDIM_SrcLineMapBucket *match = 0;
              {
                for(RDIM_SrcLineMapBucket *node = line_hash_slots[line_hash_slot_idx];
                    node != 0;
                    node = node->hash_next)
                {
                  if(node->line_num == line_num)
                  {
                    match = node;
                    break;
                  }
                }
              }
              
              // rjf: introduce new map if no match
              if(match == 0)
              {
                match = rdim_push_array(scratch.arena, RDIM_SrcLineMapBucket, 1);
                RDIM_SLLQueuePush_N(first_bucket, last_bucket, match, order_next);
                RDIM_SLLStackPush_N(line_hash_slots[line_hash_slot_idx], match, hash_next);
                match->line_num = line_num;
                line_count += 1;
              }
              
              // rjf: insert new voff
              {
                RDIM_SrcLineMapVoffBlock *block = rdim_push_array(scratch.arena, RDIM_SrcLineMapVoffBlock, 1);
                RDIM_SLLQueuePush(match->first_voff_block, match->last_voff_block, block);
                match->voff_count += 1;
                block->voff = voff;
              }
            }
          }
        }
        
        //- rjf: bake sortable keys array
        RDIM_SortKey *keys = rdim_push_array_no_zero(scratch.arena, RDIM_SortKey, line_count);
        {
          RDIM_SortKey *key_ptr = keys;
          for(RDIM_SrcLineMapBucket *node = first_bucket;
              node != 0;
              node = node->order_next, key_ptr += 1){
            key_ptr->key = node->line_num;
            key_ptr->val = node;
          }
        }
        
        //- rjf: sort keys array
        RDIM_SortKey *sorted_keys = rdim_sort_key_array(scratch.arena, keys, line_count);
        
        //- rjf: bake result
        RDI_U32 *line_nums = rdim_push_array_no_zero(arena, RDI_U32, line_count);
        RDI_U32 *line_ranges = rdim_push_array_no_zero(arena, RDI_U32, line_count + 1);
        RDI_U64 *voffs = rdim_push_array_no_zero(arena, RDI_U64, voff_count);
        {
          RDI_U64 *voff_ptr = voffs;
          for(RDI_U32 i = 0; i < line_count; i += 1)
          {
            line_nums[i] = sorted_keys[i].key;
            line_ranges[i] = (RDI_U32)(voff_ptr - voffs); // TODO(rjf): @u64_to_u32
            RDIM_SrcLineMapBucket *bucket = (RDIM_SrcLineMapBucket*)sorted_keys[i].val;
            for(RDIM_SrcLineMapVoffBlock *node = bucket->first_voff_block; node != 0; node = node->next)
            {
              *voff_ptr = node->voff;
              voff_ptr += 1;
            }
          }
          line_ranges[line_count] = voff_count;
        }
        
        //- rjf: fill output
        src_file_line_nums   = line_nums;
        src_file_line_ranges = line_ranges;
        src_file_line_count  = line_count;
        src_file_voffs       = voffs;
        src_file_voff_count  = voff_count;
        rdim_scratch_end(scratch);
      }
      
      //////////////////////////
      //- rjf: produce data sections for this source file's line info tables
      //
      if(src_file_line_count != 0)
      {
        src_file_node->line_map_count = src_file_line_count;
        src_file_node->line_map_nums_data_idx  = (RDI_U32)sections.count; // TODO(rjf): @u64_to_u32
        src_file_node->line_map_range_data_idx = src_file_node->line_map_nums_data_idx+1;
        src_file_node->line_map_voff_data_idx  = src_file_node->line_map_nums_data_idx+2;
        rdim_bake_section_list_push_new(arena, &sections, src_file_line_nums, sizeof(*src_file_line_nums)*src_file_line_count, RDI_DataSectionTag_LineMapNumbers);
        rdim_bake_section_list_push_new(arena, &sections, src_file_line_ranges, sizeof(*src_file_line_ranges)*(src_file_line_count + 1), RDI_DataSectionTag_LineMapRanges);
        rdim_bake_section_list_push_new(arena, &sections, src_file_voffs, sizeof(*src_file_voffs)*src_file_voff_count, RDI_DataSectionTag_LineMapVoffs);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: push each source file into source file name map
  //
  RDIM_ProfScope("build section for per-source-file line info")
  {
    for(RDIM_BakeSrcNode *src_file_node = path_tree.src_first;
        src_file_node != 0;
        src_file_node = src_file_node->next)
    {
      rdim_bake_name_map_push(arena, &name_maps[RDI_NameMapKind_NormalSourcePaths], src_file_node->normal_full_path, src_file_node->idx);
    }
  }
  
  //////////////////////////////
  //- rjf: build section for unit vmap
  //
  RDIM_ProfScope("build section for unit vmap")
  {
    //- rjf: build vmap from unit voff ranges
    RDIM_VMap unit_vmap = {0};
    {
      RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
      
      // rjf: count voff ranges
      RDI_U64 voff_range_count = 0;
      for(RDIM_UnitChunkNode *n = params->units.first; n != 0; n = n->next)
      {
        for(RDI_U64 idx = 0; idx < n->count; idx += 1)
        {
          RDIM_Unit *unit = &n->v[idx];
          voff_range_count += unit->voff_ranges.count;
        }
      }
      
      // rjf: count necessary markers
      RDI_U64 marker_count = voff_range_count*2;
      
      // rjf: build keys/markers arrays
      RDIM_SortKey    *keys = rdim_push_array_no_zero(scratch.arena, RDIM_SortKey, marker_count);
      RDIM_VMapMarker *markers = rdim_push_array_no_zero(scratch.arena, RDIM_VMapMarker, marker_count);
      {
        RDIM_SortKey *key_ptr = keys;
        RDIM_VMapMarker *marker_ptr = markers;
        RDI_U32 unit_idx = 0;
        for(RDIM_UnitChunkNode *unit_chunk_n = params->units.first;
            unit_chunk_n != 0;
            unit_chunk_n = unit_chunk_n->next)
        {
          for(RDI_U64 idx = 0; idx < unit_chunk_n->count; idx += 1)
          {
            RDIM_Unit *unit = &unit_chunk_n->v[idx];
            for(RDIM_Rng1U64Node *n = unit->voff_ranges.first; n != 0; n = n->next)
            {
              RDIM_Rng1U64 range = n->v;
              if(range.min < range.max)
              {
                key_ptr->key = range.min;
                key_ptr->val = marker_ptr;
                marker_ptr->idx = unit_idx;
                marker_ptr->begin_range = 1;
                key_ptr += 1;
                marker_ptr += 1;
                
                key_ptr->key = range.max;
                key_ptr->val = marker_ptr;
                marker_ptr->idx = unit_idx;
                marker_ptr->begin_range = 0;
                key_ptr += 1;
                marker_ptr += 1;
              }
            }
            unit_idx += 1;
          }
        }
      }
      
      // rjf: keys/markers -> unit vmap
      unit_vmap = rdim_vmap_from_markers(arena, markers, keys, marker_count);
      rdim_scratch_end(scratch);
    }
    
    //- rjf: build section
    RDI_U64 unit_vmap_size = sizeof(unit_vmap.vmap[0])*(unit_vmap.count+1);
    rdim_bake_section_list_push_new(arena, &sections, unit_vmap.vmap, unit_vmap_size, RDI_DataSectionTag_UnitVmap);
  }
  
  //////////////////////////////
  //- rjf: build sections for type info
  //
  RDIM_ProfScope("build sections for type info")
  {
    ////////////////////////////
    //- rjf: build all type nodes
    //
    RDI_TypeNode *type_nodes = push_array(arena, RDI_TypeNode, params->types.total_count);
    RDIM_ProfScope("push all type nodes")
    {
      RDI_U32 dst_idx = 0;
      for(RDIM_TypeChunkNode *n = params->types.first; n != 0; n = n->next)
      {
        for(RDI_U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1, dst_idx += 1)
        {
          RDIM_Type *src = &n->v[chunk_idx];
          RDI_TypeNode *dst = &type_nodes[dst_idx];
          
          //- rjf: fill shared type node info
          dst->kind      = src->kind;
          dst->byte_size = src->byte_size;
          
          //- rjf: fill built-in-only type node info
          if(RDI_TypeKind_FirstBuiltIn <= dst->kind && dst->kind <= RDI_TypeKind_LastBuiltIn)
          {
            dst->built_in.name_string_idx = rdim_bake_string(arena, &strings, src->name);
            rdim_bake_name_map_push(arena, &name_maps[RDI_NameMapKind_Types], src->name, dst_idx);
          }
          
          //- rjf: fill constructed type node info
          else if(RDI_TypeKind_FirstConstructed <= dst->kind && dst->kind <= RDI_TypeKind_LastConstructed)
          {
            dst->constructed.direct_type_idx = (RDI_U32)rdim_idx_from_type(src->direct_type); // TODO(rjf): @u64_to_u32
            dst->constructed.count = src->count;
            if(dst->kind == RDI_TypeKind_Function || dst->kind == RDI_TypeKind_Method)
            {
              RDI_U32 param_idx_run_count = src->count;
              RDI_U32 *param_idx_run = rdim_push_array_no_zero(arena, RDI_U32, param_idx_run_count);
              for(RDI_U32 idx = 0; idx < param_idx_run_count; idx += 1)
              {
                param_idx_run[idx] = (RDI_U32)rdim_idx_from_type(src->param_types[idx]); // TODO(rjf): @u64_to_u32
              }
              dst->constructed.param_idx_run_first = rdim_bake_idx_run(arena, &idx_runs, param_idx_run, param_idx_run_count);
            }
            else if(dst->kind == RDI_TypeKind_MemberPtr)
            {
              // TODO(rjf): member pointers not currently supported.
            }
          }
          
          //- rjf: fill user-defined-type info
          else if(RDI_TypeKind_FirstUserDefined <= dst->kind && dst->kind <= RDI_TypeKind_LastUserDefined)
          {
            dst->user_defined.name_string_idx = rdim_bake_string(arena, &strings, src->name);
            dst->user_defined.udt_idx         = (RDI_U32)rdim_idx_from_udt(src->udt); // TODO(rjf): @u64_to_u32
            dst->user_defined.direct_type_idx = (RDI_U32)rdim_idx_from_type(src->direct_type); // TODO(rjf): @u64_to_u32
            rdim_bake_name_map_push(arena, &name_maps[RDI_NameMapKind_Types], src->name, dst_idx);
          }
          
          //- rjf: fill bitfield info
          else if(dst->kind == RDI_TypeKind_Bitfield)
          {
            dst->bitfield.off  = src->off;
            dst->bitfield.size = src->count;
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: build all udts & members
    //
    RDI_UDT *       udts         = push_array(arena, RDI_UDT,        params->udts.total_count);
    RDI_Member *    members      = push_array(arena, RDI_Member,     params->udts.total_member_count);
    RDI_EnumMember *enum_members = push_array(arena, RDI_EnumMember, params->udts.total_enum_val_count);
    RDIM_ProfScope("build all udts & members")
    {
      RDI_U32 dst_udt_idx = 0;
      RDI_U32 dst_member_idx = 0;
      RDI_U32 dst_enum_member_idx = 0;
      for(RDIM_UDTChunkNode *n = params->udts.first; n != 0; n = n->next)
      {
        for(RDI_U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1, dst_udt_idx += 1)
        {
          RDIM_UDT *src_udt = &n->v[chunk_idx];
          RDI_UDT *dst_udt = &udts[dst_udt_idx];
          
          //- rjf: fill basics
          dst_udt->self_type_idx = (RDI_U32)rdim_idx_from_type(src_udt->self_type); // TODO(rjf): @u64_to_u32
          if(src_udt->source_path.size != 0)
          {
            RDIM_BakePathNode *path_node = rdim_bake_path_node_from_string(arena, &path_tree, src_udt->source_path);
            RDIM_BakeSrcNode  *src_node  = rdim_bake_src_node_from_path_node(arena, &path_tree, path_node);
            dst_udt->file_idx = src_node->idx;
          }
          dst_udt->line = src_udt->line;
          dst_udt->col  = src_udt->col;
          
          //- rjf: fill members
          if(src_udt->member_count != 0)
          {
            dst_udt->member_first = dst_member_idx;
            dst_udt->member_count = src_udt->member_count;
            for(RDIM_UDTMember *src_member = src_udt->first_member;
                src_member != 0;
                src_member = src_member->next, dst_member_idx += 1)
            {
              RDI_Member *dst_member = &members[dst_member_idx];
              dst_member->kind            = src_member->kind;
              dst_member->name_string_idx = rdim_bake_string(arena, &strings, src_member->name);
              dst_member->type_idx        = (RDI_U32)rdim_idx_from_type(src_member->type); // TODO(rjf): @u64_to_u32
              dst_member->off             = src_member->off;
            }
          }
          
          //- rjf: fill enum members
          else if(src_udt->enum_val_count != 0)
          {
            dst_udt->flags |= RDI_UserDefinedTypeFlag_EnumMembers;
            dst_udt->member_first = dst_enum_member_idx;
            dst_udt->member_count = src_udt->enum_val_count;
            for(RDIM_UDTEnumVal *src_member = src_udt->first_enum_val;
                src_member != 0;
                src_member = src_member->next, dst_enum_member_idx += 1)
            {
              RDI_EnumMember *dst_member = &enum_members[dst_enum_member_idx];
              dst_member->name_string_idx = rdim_bake_string(arena, &strings, src_member->name);
              dst_member->val             = src_member->val;
            }
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: push all type info sections
    //
    RDIM_ProfScope("push all type info sections")
    {
      rdim_bake_section_list_push_new(arena, &sections, type_nodes,    sizeof(RDI_TypeNode)   * params->types.total_count,         RDI_DataSectionTag_TypeNodes);
      rdim_bake_section_list_push_new(arena, &sections, udts,          sizeof(RDI_UDT)        * params->udts.total_count,          RDI_DataSectionTag_UDTs);
      rdim_bake_section_list_push_new(arena, &sections, members   ,    sizeof(RDI_Member)     * params->udts.total_member_count,   RDI_DataSectionTag_Members);
      rdim_bake_section_list_push_new(arena, &sections, enum_members,  sizeof(RDI_EnumMember) * params->udts.total_enum_val_count, RDI_DataSectionTag_EnumMembers);
    }
  }
  
  //////////////////////////////
  //- rjf: build sections for symbol info
  //
  RDIM_ProfScope("build sections for symbol info")
  {
    ////////////////////////////
    //- rjf: build all global variables
    //
    RDI_GlobalVariable *global_variables = push_array(arena, RDI_GlobalVariable, params->global_variables.total_count);
    RDIM_ProfScope("build all global variables")
    {
      RDI_U32 dst_idx = 0;
      for(RDIM_SymbolChunkNode *n = params->global_variables.first; n != 0; n = n->next)
      {
        for(RDI_U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1, dst_idx += 1)
        {
          RDIM_Symbol *src = &n->v[chunk_idx];
          RDI_GlobalVariable *dst = &global_variables[dst_idx];
          dst->name_string_idx = rdim_bake_string(arena, &strings, src->name);
          dst->voff            = src->offset;
          dst->type_idx        = (RDI_U32)rdim_idx_from_type(src->type); // TODO(rjf): @u64_to_u32
          if(src->is_extern)
          {
            dst->link_flags |= RDI_LinkFlag_External;
          }
          if(src->container_type != 0)
          {
            dst->link_flags |= RDI_LinkFlag_TypeScoped;
            dst->container_idx = src->container_type ? (RDI_U32)rdim_idx_from_udt(src->container_type->udt) : 0; // TODO(rjf): @u64_to_u32
          }
          else if(src->container_symbol != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_symbol); // TODO(rjf): @u64_to_u32
          }
          rdim_bake_name_map_push(arena, &name_maps[RDI_NameMapKind_GlobalVariables], src->name, dst_idx);
        }
      }
    }
    
    ////////////////////////////
    //- rjf: build all thread variables
    //
    RDI_ThreadVariable *thread_variables = push_array(arena, RDI_ThreadVariable, params->thread_variables.total_count);
    RDIM_ProfScope("build all thread variables")
    {
      RDI_U32 dst_idx = 0;
      for(RDIM_SymbolChunkNode *n = params->thread_variables.first; n != 0; n = n->next)
      {
        for(RDI_U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1, dst_idx += 1)
        {
          RDIM_Symbol *src = &n->v[chunk_idx];
          RDI_ThreadVariable *dst = &thread_variables[dst_idx];
          dst->name_string_idx = rdim_bake_string(arena, &strings, src->name);
          dst->tls_off         = (RDI_U32)src->offset; // TODO(rjf): @u64_to_u32
          dst->type_idx        = (RDI_U32)rdim_idx_from_type(src->type);
          if(src->is_extern)
          {
            dst->link_flags |= RDI_LinkFlag_External;
          }
          if(src->container_type != 0)
          {
            dst->link_flags |= RDI_LinkFlag_TypeScoped;
            dst->container_idx = src->container_type ? (RDI_U32)rdim_idx_from_udt(src->container_type->udt) : 0; // TODO(rjf): @u64_to_u32
          }
          else if(src->container_symbol != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_symbol); // TODO(rjf): @u64_to_u32
          }
          rdim_bake_name_map_push(arena, &name_maps[RDI_NameMapKind_ThreadVariables], src->name, dst_idx);
        }
      }
    }
    
    ////////////////////////////
    //- rjf: build all procedures
    //
    RDI_Procedure *procedures = push_array(arena, RDI_Procedure, params->procedures.total_count);
    RDIM_ProfScope("build all procedures")
    {
      RDI_U32 dst_idx = 0;
      for(RDIM_SymbolChunkNode *n = params->global_variables.first; n != 0; n = n->next)
      {
        for(RDI_U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1, dst_idx += 1)
        {
          RDIM_Symbol *src = &n->v[chunk_idx];
          RDI_Procedure *dst = &procedures[dst_idx];
          dst->name_string_idx      = rdim_bake_string(arena, &strings, src->name);
          dst->link_name_string_idx = rdim_bake_string(arena, &strings, src->link_name);
          if(src->is_extern)
          {
            dst->link_flags |= RDI_LinkFlag_External;
          }
          if(src->container_type != 0)
          {
            dst->link_flags |= RDI_LinkFlag_TypeScoped;
            dst->container_idx = src->container_type ? (RDI_U32)rdim_idx_from_udt(src->container_type->udt) : 0; // TODO(rjf): @u64_to_u32
          }
          else if(src->container_symbol != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_symbol); // TODO(rjf): @u64_to_u32
          }
          dst->type_idx             = (RDI_U32)rdim_idx_from_type(src->type); // TODO(rjf): @u64_to_u32
          dst->root_scope_idx       = (RDI_U32)rdim_idx_from_scope(src->root_scope); // TODO(rjf): @u64_to_u32
          rdim_bake_name_map_push(arena, &name_maps[RDI_NameMapKind_Procedures], src->name, dst_idx);
          if(src->link_name.size > 0)
          {
            rdim_bake_name_map_push(arena, &name_maps[RDI_NameMapKind_LinkNameProcedures], src->link_name, dst_idx);
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: build global vmap
    //
    RDIM_VMap global_vmap = {0};
    RDIM_ProfScope("build global vmap")
    {
      RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
      
      //- rjf: allocate keys/markers
      RDI_U64 marker_count = params->global_variables.total_count*2;
      RDIM_SortKey    *keys    = rdim_push_array_no_zero(scratch.arena, RDIM_SortKey, marker_count);
      RDIM_VMapMarker *markers = rdim_push_array_no_zero(scratch.arena, RDIM_VMapMarker, marker_count);
      
      //- rjf: fill
      {
        RDIM_SortKey *key_ptr = keys;
        RDIM_VMapMarker *marker_ptr = markers;
        
        // rjf: fill actual globals
        for(RDIM_SymbolChunkNode *n = params->global_variables.first; n != 0; n = n->next)
        {
          for(RDI_U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1)
          {
            RDIM_Symbol *global_var = &n->v[chunk_idx];
            RDI_U32 global_var_idx = (RDI_U32)rdim_idx_from_symbol(global_var); // TODO(rjf): @u64_to_u32
            
            RDI_U64 first = global_var->offset;
            RDI_U64 opl   = first + global_var->type->byte_size;
            
            key_ptr->key = first;
            key_ptr->val = marker_ptr;
            marker_ptr->idx = global_var_idx;
            marker_ptr->begin_range = 1;
            key_ptr += 1;
            marker_ptr += 1;
            
            key_ptr->key = opl;
            key_ptr->val = marker_ptr;
            marker_ptr->idx = global_var_idx;
            marker_ptr->begin_range = 0;
            key_ptr += 1;
            marker_ptr += 1;
          }
        }
        
        // rjf: fill nil global
        {
          RDI_U32 global_idx = 0;
          RDI_U64 first = 0;
          RDI_U64 opl   = 0xffffffffffffffffull;
          key_ptr->key = first;
          key_ptr->val = marker_ptr;
          marker_ptr->idx = global_idx;
          marker_ptr->begin_range = 1;
          key_ptr += 1;
          marker_ptr += 1;
          key_ptr->key = opl;
          key_ptr->val = marker_ptr;
          marker_ptr->idx = global_idx;
          marker_ptr->begin_range = 0;
          key_ptr += 1;
          marker_ptr += 1;
        }
      }
      
      // rjf: construct vmap
      global_vmap = rdim_vmap_from_markers(arena, markers, keys, marker_count);
      
      rdim_scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: build all scopes, scope voffs, locals, and location blocks
    //
    RDI_Scope *         scopes          = rdim_push_array(arena, RDI_Scope,         params->scopes.total_count);
    RDI_U64 *           scope_voffs     = rdim_push_array(arena, RDI_U64,           params->scopes.scope_voff_count);
    RDI_Local *         locals          = rdim_push_array(arena, RDI_Local,         params->scopes.local_count);
    RDI_LocationBlock * location_blocks = rdim_push_array(arena, RDI_LocationBlock, params->scopes.location_count);
    RDIM_String8List    location_data_blobs = {0};
    RDIM_ProfScope("build all scopes, scope voffs, locals, and location blocks")
    {
      RDI_U64 dst_scope_idx = 0;
      RDI_U64 dst_scope_voff_idx = 0;
      RDI_U64 dst_local_idx = 0;
      RDI_U64 dst_location_block_idx = 0;
      for(RDIM_ScopeChunkNode *chunk_n = params->scopes.first; chunk_n != 0; chunk_n = chunk_n->next)
      {
        for(RDI_U64 chunk_idx = 0; chunk_idx < chunk_n->count; chunk_idx += 1, dst_scope_idx += 1)
        {
          RDIM_Scope *src_scope = &chunk_n->v[chunk_idx];
          RDI_Scope *dst_scope = &scopes[dst_scope_idx];
          
          //- rjf: push scope's voffs
          RDI_U64 voff_idx_first = dst_scope_voff_idx;
          {
            for(RDIM_Rng1U64Node *n = src_scope->voff_ranges.first; n != 0; n = n->next)
            {
              scope_voffs[dst_scope_voff_idx] = n->v.min;
              dst_scope_voff_idx += 1;
              scope_voffs[dst_scope_voff_idx] = n->v.max;
              dst_scope_voff_idx += 1;
            }
          }
          RDI_U64 voff_idx_opl = dst_scope_voff_idx;
          
          //- rjf: push locals
          RDI_U64 local_idx_first = dst_local_idx;
          for(RDIM_Local *src_local = src_scope->first_local;
              src_local != 0;
              src_local = src_local->next, dst_local_idx += 1)
          {
            //- rjf: push local's locations
            RDI_U64 location_block_idx_first = dst_location_block_idx;
            for(RDIM_LocationCase *loccase = src_local->locset.first_location_case;
                loccase != 0;
                loccase = loccase->next, dst_location_block_idx += 1)
            {
              // rjf: fill location block
              RDI_LocationBlock *dst_locblock = &location_blocks[dst_location_block_idx];
              dst_locblock->scope_off_first   = loccase->voff_range.min;
              dst_locblock->scope_off_opl     = loccase->voff_range.max;
              dst_locblock->location_data_off = location_data_blobs.total_size;
              
              // rjf: serialize location into location data
              RDIM_Location *src_location = loccase->location;
              {
                // rjf: nil location
                if(src_location == 0)
                {
                  rdim_str8_list_push_align(scratch.arena, &location_data_blobs, 8);
                  rdim_str8_list_push(scratch.arena, &location_data_blobs, rdim_str8_lit("\0"));
                }
                
                // rjf: valid location
                else switch(src_location->kind)
                {
                  // rjf: catchall unsupported case
                  default:
                  {
                    rdim_str8_list_push_align(scratch.arena, &location_data_blobs, 8);
                    rdim_str8_list_push(scratch.arena, &location_data_blobs, rdim_str8_lit("\0"));
                  }break;
                  
                  // rjf: bytecode streams
                  case RDI_LocationKind_AddrBytecodeStream:
                  case RDI_LocationKind_ValBytecodeStream:
                  {
                    rdim_str8_list_push(scratch.arena, &location_data_blobs, rdim_str8_copy(scratch.arena, rdim_str8_struct(&src_location->kind)));
                    for(RDIM_EvalBytecodeOp *op_node = src_location->bytecode.first_op;
                        op_node != 0;
                        op_node = op_node->next)
                    {
                      RDI_U8 op_data[9];
                      op_data[0] = op_node->op;
                      rdim_memcpy(op_data + 1, &op_node->p, op_node->p_size);
                      RDIM_String8 op_data_str = rdim_str8(op_data, 1 + op_node->p_size);
                      rdim_str8_list_push(scratch.arena, &location_data_blobs, rdim_str8_copy(scratch.arena, op_data_str));
                    }
                    {
                      RDI_U64 data = 0;
                      RDIM_String8 data_str = rdim_str8((RDI_U8 *)&data, 1);
                      rdim_str8_list_push(scratch.arena, &location_data_blobs, rdim_str8_copy(scratch.arena, data_str));
                    }
                  }break;
                  
                  // rjf: simple addr+off cases
                  case RDI_LocationKind_AddrRegisterPlusU16:
                  case RDI_LocationKind_AddrAddrRegisterPlusU16:
                  {
                    RDI_LocationRegisterPlusU16 loc = {0};
                    loc.kind = src_location->kind;
                    loc.register_code = src_location->register_code;
                    loc.offset = src_location->offset;
                    rdim_str8_list_push(scratch.arena, &location_data_blobs, rdim_str8_copy(scratch.arena, rdim_str8_struct(&loc)));
                  }break;
                  
                  // rjf: register cases
                  case RDI_LocationKind_ValRegister:
                  {
                    RDI_LocationRegister loc = {0};
                    loc.kind = src_location->kind;
                    loc.register_code = src_location->register_code;
                    rdim_str8_list_push(scratch.arena, &location_data_blobs, rdim_str8_copy(scratch.arena, rdim_str8_struct(&loc)));
                  }break;
                }
              }
            }
            RDI_U64 location_block_idx_opl = dst_location_block_idx;
            
            //- rjf: fill local
            RDI_Local *dst_local = &locals[dst_local_idx];
            dst_local->kind            = src_local->kind;
            dst_local->name_string_idx = rdim_bake_string(arena, &strings, src_local->name);
            dst_local->type_idx        = (RDI_U32)rdim_idx_from_type(src_local->type); // TODO(rjf): @u64_to_u32
            dst_local->location_first  = (RDI_U32)location_block_idx_first; // TODO(rjf): @u64_to_u32
            dst_local->location_opl    = (RDI_U32)location_block_idx_opl;   // TODO(rjf): @u64_to_u32
          }
          RDI_U64 local_idx_opl = dst_local_idx;
          
          //- rjf: fill scope
          dst_scope->proc_idx               = (RDI_U32)rdim_idx_from_symbol(src_scope->symbol); // TODO(rjf): @u64_to_u32
          dst_scope->parent_scope_idx       = (RDI_U32)rdim_idx_from_scope(src_scope->parent_scope); // TODO(rjf): @u64_to_u32
          dst_scope->first_child_scope_idx  = (RDI_U32)rdim_idx_from_scope(src_scope->first_child); // TODO(rjf): @u64_to_u32
          dst_scope->next_sibling_scope_idx = (RDI_U32)rdim_idx_from_scope(src_scope->next_sibling); // TODO(rjf): @u64_to_u32
          dst_scope->voff_range_first       = (RDI_U32)voff_idx_first;                    // TODO(rjf): @u64_to_u32
          dst_scope->voff_range_opl         = (RDI_U32)voff_idx_opl;                      // TODO(rjf): @u64_to_u32
          dst_scope->local_first            = (RDI_U32)local_idx_first;                   // TODO(rjf): @u64_to_u32
          dst_scope->local_count            = (RDI_U32)(local_idx_opl - local_idx_first); // TODO(rjf): @u64_to_u32
        }
      }
    }
    
    ////////////////////////////
    //- rjf: build flattened location data
    //
    RDIM_String8 location_data_blob = {0};
    RDIM_ProfScope("build flattened location data")
    {
      location_data_blob = rdim_str8_list_join(arena, &location_data_blobs, rdim_str8_lit(""));
    }
    
    ////////////////////////////
    //- rjf: build scope vmap
    //
    RDIM_VMap scope_vmap = {0};
    RDIM_ProfScope("build scope vmap")
    {
      RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
      
      // rjf: allocate keys/markers
      RDI_U64 marker_count = params->scopes.scope_voff_count;
      RDIM_SortKey    *keys    = rdim_push_array_no_zero(scratch.arena, RDIM_SortKey, marker_count);
      RDIM_VMapMarker *markers = rdim_push_array_no_zero(scratch.arena, RDIM_VMapMarker, marker_count);
      
      // rjf: fill
      {
        RDIM_SortKey *key_ptr = keys;
        RDIM_VMapMarker *marker_ptr = markers;
        for(RDIM_ScopeChunkNode *chunk_n = params->scopes.first; chunk_n != 0; chunk_n = chunk_n->next)
        {
          for(RDI_U64 chunk_idx = 0; chunk_idx < chunk_n->count; chunk_idx += 1)
          {
            RDIM_Scope *src_scope = &chunk_n->v[chunk_idx];
            RDI_U32 scope_idx = (RDI_U32)rdim_idx_from_scope(src_scope); // TODO(rjf): @u64_to_u32
            for(RDIM_Rng1U64Node *n = 0; n != 0; n = n->next)
            {
              key_ptr->key = n->v.min;
              key_ptr->val = marker_ptr;
              marker_ptr->idx = scope_idx;
              marker_ptr->begin_range = 1;
              key_ptr += 1;
              marker_ptr += 1;
              
              key_ptr->key = n->v.max;
              key_ptr->val = marker_ptr;
              marker_ptr->idx = scope_idx;
              marker_ptr->begin_range = 0;
              key_ptr += 1;
              marker_ptr += 1;
            }
          }
        }
      }
      
      // rjf: produce vmap
      scope_vmap = rdim_vmap_from_markers(arena, markers, keys, marker_count);
      rdim_scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: push all symbol info sections
    //
    RDIM_ProfScope("push all symbol info sections")
    {
      rdim_bake_section_list_push_new(arena, &sections, global_variables,         sizeof(RDI_GlobalVariable)   * params->global_variables.total_count,     RDI_DataSectionTag_GlobalVariables);
      rdim_bake_section_list_push_new(arena, &sections, global_vmap.vmap,         sizeof(RDI_U64)              * (global_vmap.count+1),                    RDI_DataSectionTag_GlobalVmap);
      rdim_bake_section_list_push_new(arena, &sections, thread_variables,         sizeof(RDI_ThreadVariable)   * params->thread_variables.total_count,     RDI_DataSectionTag_ThreadVariables);
      rdim_bake_section_list_push_new(arena, &sections, procedures,               sizeof(RDI_Procedure)        * params->procedures.total_count,           RDI_DataSectionTag_Procedures);
      rdim_bake_section_list_push_new(arena, &sections, scopes,                   sizeof(RDI_Scope)            * params->scopes.total_count,               RDI_DataSectionTag_Scopes);
      rdim_bake_section_list_push_new(arena, &sections, scope_vmap.vmap,          sizeof(RDI_U64)              * (scope_vmap.count+1),                     RDI_DataSectionTag_ScopeVmap);
      rdim_bake_section_list_push_new(arena, &sections, locals,                   sizeof(RDI_Local)            * params->scopes.local_count,               RDI_DataSectionTag_Locals);
      rdim_bake_section_list_push_new(arena, &sections, location_blocks,          sizeof(RDI_LocationBlock)    * params->scopes.location_count,            RDI_DataSectionTag_LocationBlocks);
      rdim_bake_section_list_push_new(arena, &sections, location_data_blob.str,   location_data_blob.size,                                                 RDI_DataSectionTag_LocationData);
    }
  }
  
  //////////////////////////////
  //- rjf: build sections for name maps
  //
  RDIM_ProfScope("build sections for name maps")
  {
    //- rjf: count the # of name maps we have with any content
    RDI_U32 name_map_count = 0;
    for(RDI_NameMapKind k = (RDI_NameMapKind)(RDI_NameMapKind_NULL+1);
        k < RDI_NameMapKind_COUNT;
        k = (RDI_NameMapKind)(k+1))
    {
      if(name_maps[k].name_count != 0)
      {
        name_map_count += 1;
      }
    }
    
    //- rjf: allocate & fill baked name maps
    RDI_NameMap *dst_maps = rdim_push_array(arena, RDI_NameMap, name_map_count);
    {
      RDI_U64 dst_map_idx = 0;
      for(RDI_NameMapKind k = (RDI_NameMapKind)(RDI_NameMapKind_NULL+1);
          k < RDI_NameMapKind_COUNT;
          k = (RDI_NameMapKind)(k+1))
      {
        RDI_NameMap *dst_map = &dst_maps[dst_map_idx];
        RDIM_BakeNameMap *src_map = &name_maps[k];
        if(src_map->name_count == 0) { continue; }
        
        // rjf: bake name map
        RDI_U32 baked_buckets_count = src_map->name_count;
        RDI_U32 baked_nodes_count = src_map->name_count;
        RDI_NameMapBucket *baked_buckets = rdim_push_array(arena, RDI_NameMapBucket, baked_buckets_count);
        RDI_NameMapNode *baked_nodes = rdim_push_array_no_zero(arena, RDI_NameMapNode, baked_nodes_count);
        {
          RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
          
          // rjf: setup the final bucket layouts
          typedef struct RDIM_NameMapSemiNode RDIM_NameMapSemiNode;
          struct RDIM_NameMapSemiNode
          {
            RDIM_NameMapSemiNode *next;
            RDIM_BakeNameMapNode *node;
          };
          typedef struct RDIM_NameMapSemiBucket RDIM_NameMapSemiBucket;
          struct RDIM_NameMapSemiBucket
          {
            RDIM_NameMapSemiNode *first;
            RDIM_NameMapSemiNode *last;
            RDI_U64 count;
          };
          RDIM_NameMapSemiBucket *sbuckets = rdim_push_array(scratch.arena, RDIM_NameMapSemiBucket, baked_buckets_count);
          for(RDIM_BakeNameMapNode *node = src_map->first;
              node != 0;
              node = node->order_next)
          {
            RDI_U64 hash = rdi_hash(node->string.str, node->string.size);
            RDI_U64 bi = hash%baked_buckets_count;
            RDIM_NameMapSemiNode *snode = rdim_push_array(scratch.arena, RDIM_NameMapSemiNode, 1);
            SLLQueuePush(sbuckets[bi].first, sbuckets[bi].last, snode);
            snode->node = node;
            sbuckets[bi].count += 1;
          }
          
          // rjf: convert to serialized buckets & nodes
          {
            RDI_NameMapBucket *bucket_ptr = baked_buckets;
            RDI_NameMapNode *node_ptr = baked_nodes;
            for(RDI_U32 i = 0; i < baked_buckets_count; i += 1, bucket_ptr += 1)
            {
              bucket_ptr->first_node = (RDI_U32)(node_ptr - baked_nodes);
              bucket_ptr->node_count = sbuckets[i].count;
              for(RDIM_NameMapSemiNode *snode = sbuckets[i].first;
                  snode != 0;
                  snode = snode->next)
              {
                RDIM_BakeNameMapNode *node = snode->node;
                
                // rjf: cons name and index(es)
                RDI_U32 string_idx = rdim_bake_string(arena, &strings, node->string);
                RDI_U32 match_count = node->val_count;
                RDI_U32 idx = 0;
                if(match_count == 1)
                {
                  idx = node->val_first->val[0];
                }
                else
                {
                  RDI_U64 temp_pos = rdim_arena_pos(scratch.arena);
                  RDI_U32 *idx_run = rdim_push_array_no_zero(scratch.arena, RDI_U32, match_count);
                  RDI_U32 *idx_ptr = idx_run;
                  for(RDIM_BakeNameMapValNode *idxnode = node->val_first;
                      idxnode != 0;
                      idxnode = idxnode->next)
                  {
                    for(RDI_U32 i = 0; i < sizeof(idxnode->val)/sizeof(idxnode->val[0]); i += 1)
                    {
                      if(idxnode->val[i] == 0)
                      {
                        goto dblbreak;
                      }
                      *idx_ptr = idxnode->val[i];
                      idx_ptr += 1;
                    }
                  }
                  dblbreak:;
                  idx = rdim_bake_idx_run(arena, &idx_runs, idx_run, match_count);
                  rdim_arena_pop_to(scratch.arena, temp_pos);
                }
                
                // rjf: write to node
                node_ptr->string_idx = string_idx;
                node_ptr->match_count = match_count;
                node_ptr->match_idx_or_idx_run_first = idx;
                node_ptr += 1;
              }
            }
          }
          rdim_scratch_end(scratch);
        }
        
        // rjf: fill output header, and push sections for buckets/nodes
        dst_map->kind = k;
        dst_map->bucket_data_idx = (RDI_U32)sections.count+0; // TODO(rjf): @u64_to_u32
        dst_map->node_data_idx   = (RDI_U32)sections.count+1; // TODO(rjf): @u64_to_u32
        rdim_bake_section_list_push_new(arena, &sections, baked_buckets, sizeof(RDI_NameMapBucket)* baked_buckets_count, RDI_DataSectionTag_NameMapBuckets);
        rdim_bake_section_list_push_new(arena, &sections, baked_nodes,   sizeof(RDI_NameMapNode)  * baked_nodes_count,   RDI_DataSectionTag_NameMapNodes);
        
        // rjf: inc
        dst_map_idx += 1;
      }
    }
    
    // rjf: push section for all name maps
    rdim_bake_section_list_push_new(arena, &sections, dst_maps, sizeof(RDI_NameMap)*name_map_count, RDI_DataSectionTag_NameMaps);
  }
  
  //////////////////////////////
  //- rjf: build sections for file paths
  //
  RDIM_ProfScope("build sections for file paths")
  {
    RDI_U32 dst_nodes_count = path_tree.count;
    RDI_FilePathNode *dst_nodes = rdim_push_array(arena, RDI_FilePathNode, dst_nodes_count);
    {
      RDI_U32 dst_node_idx = 0;
      for(RDIM_BakePathNode *src_node = path_tree.first;
          src_node != 0;
          src_node = src_node->next_order, dst_node_idx += 1)
      {
        RDI_FilePathNode *dst_node = &dst_nodes[dst_node_idx];
        dst_node->name_string_idx = rdim_bake_string(arena, &strings, src_node->name);
        if(src_node->parent != 0)
        {
          dst_node->parent_path_node = src_node->parent->idx;
        }
        if(src_node->first_child != 0)
        {
          dst_node->first_child = src_node->first_child->idx;
        }
        if(src_node->next_sibling != 0)
        {
          dst_node->next_sibling = src_node->next_sibling->idx;
        }
        if(src_node->src_file != 0)
        {
          dst_node->source_file_idx = src_node->src_file->idx;
        }
      }
    }
    rdim_bake_section_list_push_new(arena, &sections, dst_nodes, sizeof(RDI_FilePathNode)*dst_nodes_count, RDI_DataSectionTag_FilePathNodes);
  }
  
  //////////////////////////////
  //- rjf: build sections for source files
  //
  RDIM_ProfScope("build sections for source files")
  {
    
  }
  
  //////////////////////////////
  //- rjf: build sections for strings
  //
  RDIM_ProfScope("build sections for strings")
  {
    
  }
  
  //////////////////////////////
  //- rjf: build section for index runs
  //
  RDIM_ProfScope("build section for index runs")
  {
    
  }
  
  //////////////////////////////
  //- rjf: finalize: build blob strings for header & all sections
  //
  RDIM_ProfScope("finalize: build blob strings for header & all sections")
  {
    // rjf: push empty header & data section table
    RDI_Header *baked_rdi_header = rdim_push_array(arena, RDI_Header, 1);
    RDI_DataSection *baked_rdi_sections = rdim_push_array(arena, RDI_DataSection, sections.count);
    rdim_str8_list_push(arena, &blobs, rdim_str8_struct(baked_rdi_header));
    rdim_str8_list_push_align(arena, &blobs, 8);
    U32 data_section_off = (U32)blobs.total_size;
    rdim_str8_list_push(arena, &blobs, rdim_str8((RDI_U8 *)baked_rdi_sections, sizeof(RDI_DataSection)*sections.count));
    
    // rjf: fill baked header
    {
      baked_rdi_header->magic              = RDI_MAGIC_CONSTANT;
      baked_rdi_header->encoding_version   = RDI_ENCODING_VERSION;
      baked_rdi_header->data_section_off   = data_section_off;
      baked_rdi_header->data_section_count = sections.count;
    }
    
    // rjf: fill baked data section table
    U64 dst_idx = 0;
    for(RDIM_BakeSectionNode *src_n = sections.first; src_n != 0; src_n = src_n->next, dst_idx += 1)
    {
      RDIM_BakeSection *src = &src_n->v;
      RDI_DataSection *dst = baked_rdi_sections + dst_idx;
      U64 data_section_off = 0;
      if(src->size != 0)
      {
        rdim_str8_list_push_align(arena, &blobs, 8);
        data_section_off = blobs.total_size;
        rdim_str8_list_push(arena, &blobs, rdim_str8((RDI_U8 *)src->data, src->size));
      }
      dst->tag           = src->tag;
      dst->encoding      = RDI_DataSectionEncoding_Unpacked;
      dst->off           = data_section_off;
      dst->encoded_size  = src->size;
      dst->unpacked_size = src->size;
    }
  }
  
  rdim_scratch_end(scratch);
  return blobs;
}

#if 0
////////////////////////////////
//~ rjf: Loose Debug Info Construction (Anything -> Loose) Functions

//- rjf: u64 -> ptr map

RDI_PROC void
rdim_u64toptr_map_init(RDIM_Arena *arena, RDIM_U64ToPtrMap *map, RDI_U64 bucket_count)
{
  rdim_assert(IsPow2OrZero(bucket_count) && bucket_count > 0);
  map->buckets = rdim_push_array(arena, RDIM_U64ToPtrNode*, bucket_count);
  map->buckets_count = bucket_count;
}

RDI_PROC void
rdim_u64toptr_map_lookup(RDIM_U64ToPtrMap *map, RDI_U64 key, RDI_U64 hash, RDIM_U64ToPtrLookup *lookup_out)
{
  RDI_U64 bucket_idx = hash&(map->buckets_count - 1);
  RDIM_U64ToPtrNode *check_node = map->buckets[bucket_idx];
  for(;check_node != 0; check_node = check_node->next){
    for(RDI_U32 k = 0; k < ArrayCount(check_node->key); k += 1){
      if(check_node->ptr[k] == 0){
        lookup_out->fill_node = check_node;
        lookup_out->fill_k = k;
        break;
      }
      else if(check_node->key[k] == key){
        lookup_out->match = check_node->ptr[k];
        break;
      }
    }
  }
}

RDI_PROC void
rdim_u64toptr_map_insert(RDIM_Arena *arena, RDIM_U64ToPtrMap *map, RDI_U64 key, RDI_U64 hash, RDIM_U64ToPtrLookup *lookup, void *ptr)
{
  if(lookup->fill_node != 0)
  {
    RDIM_U64ToPtrNode *node = lookup->fill_node;
    RDI_U32 k = lookup->fill_k;
    node->key[k] = key;
    node->ptr[k] = ptr;
  }
  else
  {
    RDI_U64 bucket_idx = hash&(map->buckets_count - 1);
    
    RDIM_U64ToPtrNode *node = rdim_push_array(arena, RDIM_U64ToPtrNode, 1);
    SLLStackPush(map->buckets[bucket_idx], node);
    node->key[0] = key;
    node->ptr[0] = ptr;
    
    lookup->fill_node = node;
    lookup->fill_k = 0;
    
    map->pair_count += 1;
    map->bucket_collision_count += (node->next != 0);
  }
}

//- rjf: string8 -> ptr map

RDI_PROC void
rdim_str8toptr_map_init(RDIM_Arena *arena, RDIM_Str8ToPtrMap *map, RDI_U64 bucket_count)
{
  map->buckets_count = bucket_count;
  map->buckets = rdim_push_array(arena, RDIM_Str8ToPtrNode*, map->buckets_count);
}

RDI_PROC void*
rdim_str8toptr_map_lookup(RDIM_Str8ToPtrMap *map, RDIM_String8 key, RDI_U64 hash)
{
  void *result = 0;
  RDI_U64 bucket_idx = hash%map->buckets_count;
  for(RDIM_Str8ToPtrNode *node = map->buckets[bucket_idx];
      node != 0;
      node = node->next)
  {
    if(node->hash == hash && rdim_str8_match(node->key, key, 0))
    {
      result = node->ptr;
      break;
    }
  }
  return result;
}

RDI_PROC void
rdim_str8toptr_map_insert(RDIM_Arena *arena, RDIM_Str8ToPtrMap *map, RDIM_String8 key, RDI_U64 hash, void *ptr)
{
  RDI_U64 bucket_idx = hash%map->buckets_count;
  RDIM_Str8ToPtrNode *node = rdim_push_array(arena, RDIM_Str8ToPtrNode, 1);
  SLLStackPush(map->buckets[bucket_idx], node);
  
  node->key  = rdim_str8_copy(arena, key);
  node->hash = hash;
  node->ptr = ptr;
  map->bucket_collision_count += (node->next != 0);
  map->pair_count += 1;
}

//- rjf: root creation

RDI_PROC RDIM_Root *
rdim_root_alloc(RDIM_RootParams *params)
{
  RDIM_Arena *arena = rdim_arena_alloc();
  RDIM_Root *result = rdim_push_array(arena, RDIM_Root, 1);
  result->arena = arena;
  
  // fill in root parameters
  {
    result->addr_size = params->addr_size;
  }
  
  // setup singular types
  {
    result->nil_type = rdim_type_new(result);
    result->variadic_type = rdim_type_new(result);
    result->variadic_type->kind = RDI_TypeKind_Variadic;
    
    // references to "handled nil type" should be emitted as
    // references to nil - but should not generate error
    // messages when they are detected - they are expected!
    rdim_assert(result->nil_type->idx == result->handled_nil_type.idx);
  }
  
  // setup a null scope
  {
    RDIM_Scope *scope = rdim_push_array(result->arena, RDIM_Scope, 1);
    RDIM_SLLQueuePush_N(result->first_scope, result->last_scope, scope, next_order);
    result->scope_count += 1;
  }
  
  // rjf: setup null UDT
  {
    rdim_type_udt_from_any_type(result, result->nil_type);
  }
  
  // initialize maps
  {
#define BKTCOUNT(x) ((x)?(u64_up_to_pow2(x)):(128))
    
    rdim_u64toptr_map_init(arena, &result->unit_map, BKTCOUNT(params->bucket_count_units));
    rdim_u64toptr_map_init(arena, &result->symbol_map, BKTCOUNT(params->bucket_count_symbols));
    rdim_u64toptr_map_init(arena, &result->scope_map, BKTCOUNT(params->bucket_count_scopes));
    rdim_u64toptr_map_init(arena, &result->local_map, BKTCOUNT(params->bucket_count_locals));
    rdim_u64toptr_map_init(arena, &result->type_from_id_map, BKTCOUNT(params->bucket_count_types));
    rdim_str8toptr_map_init(arena, &result->construct_map, BKTCOUNT(params->bucket_count_type_constructs));
    
#undef BKTCOUNT
  }
  
  return result;
}

RDI_PROC void
rdim_root_release(RDIM_Root *root)
{
  arena_release(root->arena);
}

//- rjf: error accumulation

RDI_PROC void
rdim_push_msg(RDIM_Root *root, RDIM_String8 string)
{
  RDIM_Msg *msg = rdim_push_array(root->arena, RDIM_Msg, 1);
  SLLQueuePush(root->msgs.first, root->msgs.last, msg);
  root->msgs.count += 1;
  msg->string = string;
}

RDI_PROC void
rdim_push_msgf(RDIM_Root *root, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  RDIM_String8 str = rdim_str8fv(root->arena, fmt, args);
  rdim_push_msg(root, str);
  va_end(args);
}

RDI_PROC RDIM_Msg *
rdim_first_msg_from_root(RDIM_Root *root)
{
  return root->msgs.first;
}

//- rjf: type info lookups/reservations

RDI_PROC RDIM_Type*
rdim_type_from_id(RDIM_Root *root, RDI_U64 type_user_id, RDI_U64 type_user_id_hash)
{
  RDIM_U64ToPtrLookup lookup = {0};
  rdim_u64toptr_map_lookup(&root->type_from_id_map, type_user_id, type_user_id_hash, &lookup);
  RDIM_Type *result = (RDIM_Type*)lookup.match;
  return result;
}

RDI_PROC RDIM_Reservation*
rdim_type_reserve_id(RDIM_Root *root, RDI_U64 type_user_id, RDI_U64 type_user_id_hash)
{
  RDIM_Reservation *result = 0;
  RDIM_U64ToPtrLookup lookup = {0};
  rdim_u64toptr_map_lookup(&root->type_from_id_map, type_user_id, type_user_id_hash, &lookup);
  if(lookup.match == 0)
  {
    rdim_u64toptr_map_insert(root->arena, &root->type_from_id_map, type_user_id, type_user_id_hash, &lookup, root->nil_type);
    void **slot = &lookup.fill_node->ptr[lookup.fill_k];
    result = (RDIM_Reservation*)slot;
  }
  return result;
}

RDI_PROC void
rdim_type_fill_id(RDIM_Root *root, RDIM_Reservation *res, RDIM_Type *type)
{
  if(res != 0 && type != 0)
  {
    *(void**)res = type;
  }
}

//- rjf: nil/singleton types

RDI_PROC RDI_S32
rdim_type_is_unhandled_nil(RDIM_Root *root, RDIM_Type *type)
{
  RDI_S32 result = (type->kind == RDI_TypeKind_NULL && type != &root->handled_nil_type);
  return result;
}

RDI_PROC RDIM_Type*
rdim_type_handled_nil(RDIM_Root *root)
{
  return &root->handled_nil_type;
}

RDI_PROC RDIM_Type*
rdim_type_nil(RDIM_Root *root)
{
  return root->nil_type;
}

RDI_PROC RDIM_Type*
rdim_type_variadic(RDIM_Root *root)
{
  return root->variadic_type;
}

//- rjf: base type info constructors

RDI_PROC RDIM_Type*
rdim_type_new(RDIM_Root *root)
{
  RDIM_Type *result = rdim_push_array(root->arena, RDIM_Type, 1);
  result->idx = root->type_count;
  RDIM_SLLQueuePush_N(root->first_type, root->last_type, result, next_order);
  root->type_count += 1;
  return result;
}

RDI_PROC RDIM_TypeUDT*
rdim_type_udt_from_any_type(RDIM_Root *root, RDIM_Type *type)
{
  if(type->udt == 0)
  {
    RDIM_TypeUDT *new_udt = rdim_push_array(root->arena, RDIM_TypeUDT, 1);
    new_udt->idx = root->type_udt_count;
    RDIM_SLLQueuePush_N(root->first_udt, root->last_udt, new_udt, next_order);
    root->type_udt_count += 1;
    new_udt->self_type = type;
    type->udt = new_udt;
  }
  RDIM_TypeUDT *result = type->udt;
  return result;
}

RDI_PROC RDIM_TypeUDT*
rdim_type_udt_from_record_type(RDIM_Root *root, RDIM_Type *type)
{
  rdim_requiref(root, (type->kind == RDI_TypeKind_Struct ||
                       type->kind == RDI_TypeKind_Class ||
                       type->kind == RDI_TypeKind_Union),
                return 0,
                "Tried to use non-user-defined-type-kind to create user-defined-type.");
  RDIM_TypeUDT *result = 0;
  result = rdim_type_udt_from_any_type(root, type);
  return result;
}

//- rjf: basic/operator type construction helpers

RDI_PROC RDIM_Type*
rdim_type_basic(RDIM_Root *root, RDI_TypeKind type_kind, RDIM_String8 name)
{
  rdim_requiref(root, (RDI_TypeKind_FirstBuiltIn <= type_kind && type_kind <= RDI_TypeKind_LastBuiltIn), return root->nil_type, "Non-basic type kind passed to construct basic type.");
  RDIM_Type *result = root->nil_type;
  RDIM_Temp scratch = rdim_scratch_begin(0, 0);
  
  // setup construct buffer
  RDI_U64 buf_size = sizeof(RDIM_TypeConstructKind) + sizeof(type_kind) + name.size;
  RDI_U8 *buf = rdim_push_array(rdim_temp_arena(scratch), RDI_U8, buf_size);
  {
    RDI_U8 *ptr = buf;
    // "basic"
    *(RDIM_TypeConstructKind*)ptr = RDIM_TypeConstructKind_Basic;
    ptr += sizeof(RDIM_TypeConstructKind);
    // type_kind
    rdim_memcpy(ptr, &type_kind, sizeof(type_kind));
    ptr += sizeof(type_kind);
    // name
    rdim_memcpy(ptr, name.str, name.size);
    ptr += name.size;
  }
  
  // check for duplicate construct
  RDIM_String8 blob = rdim_str8(buf, buf_size);
  RDI_U64 blob_hash = rdi_hash(buf, buf_size);
  void *lookup_ptr = rdim_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RDIM_Type*)lookup_ptr;
  if(result == 0)
  {
    // calculate size
    RDI_U32 byte_size = rdi_size_from_basic_type_kind(type_kind);
    if(byte_size == 0xFFFFFFFF)
    {
      byte_size = root->addr_size;
    }
    
    // setup new node
    result = rdim_type_new(root);
    result->kind = type_kind;
    result->name = rdim_str8_copy(root->arena, name);
    result->byte_size = byte_size;
    
    // save in construct map
    rdim_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
    
    // save in name map
    {
      RDIM_NameMap *map = rdim_name_map_for_kind(root, RDI_NameMapKind_Types);
      rdim_name_map_add_pair(root, map, result->name, result->idx);
    }
  }
  
  scratch_end(scratch);
  rdim_assert(result != 0);
  return result;
}

RDI_PROC RDIM_Type*
rdim_type_modifier(RDIM_Root *root, RDIM_Type *direct_type, RDI_TypeModifierFlags flags)
{
  RDIM_Type *result = root->nil_type;
  RDIM_Temp scratch = rdim_scratch_begin(0, 0);
  
  // setup construct buffer
  RDI_U64 buf_size = sizeof(RDIM_TypeConstructKind) + sizeof(flags) + sizeof(direct_type->idx);
  RDI_U8 *buf = rdim_push_array(rdim_temp_arena(scratch), RDI_U8, buf_size);
  {
    RDI_U8 *ptr = buf;
    // "modifier"
    *(RDIM_TypeConstructKind*)ptr = RDIM_TypeConstructKind_Modifier;
    ptr += sizeof(RDIM_TypeConstructKind);
    // flags
    rdim_memcpy(ptr, &flags, sizeof(flags));
    ptr += sizeof(flags);
    // direct_type->idx
    rdim_memcpy(ptr, &direct_type->idx, sizeof(direct_type->idx));
    ptr += sizeof(direct_type->idx);
  }
  
  // check for duplicate construct
  RDIM_String8 blob = rdim_str8(buf, buf_size);
  RDI_U64 blob_hash = rdi_hash(buf, buf_size);
  void *lookup_ptr = rdim_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RDIM_Type*)lookup_ptr;
  if(result == 0){
    
    // setup new node
    result = rdim_type_new(root);
    result->kind = RDI_TypeKind_Modifier;
    result->flags = flags;
    result->byte_size = direct_type->byte_size;
    result->direct_type = direct_type;
    
    // save in construct map
    rdim_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  rdim_scratch_end(scratch);
  rdim_assert(result != 0);
  return result;
}

RDI_PROC RDIM_Type*
rdim_type_bitfield(RDIM_Root *root, RDIM_Type *direct_type, RDI_U32 bit_off, RDI_U32 bit_count)
{
  RDIM_Type *result = root->nil_type;
  RDIM_Temp scratch = rdim_scratch_begin(0, 0);
  
  // setup construct buffer
  RDI_U64 buf_size = sizeof(RDIM_TypeConstructKind) + sizeof(direct_type->idx) + sizeof(RDI_U32)*2;
  RDI_U8 *buf = rdim_push_array(rdim_temp_arena(scratch), RDI_U8, buf_size);
  {
    RDI_U8 *ptr = buf;
    // "bitfield"
    *(RDIM_TypeConstructKind*)ptr = RDIM_TypeConstructKind_Bitfield;
    ptr += sizeof(RDIM_TypeConstructKind);
    // direct_type->idx
    rdim_memcpy(ptr, &direct_type->idx, sizeof(direct_type->idx));
    ptr += sizeof(direct_type->idx);
    // bit_off
    rdim_memcpy(ptr, &bit_off, sizeof(bit_off));
    ptr += sizeof(bit_off);
    // bit_count
    rdim_memcpy(ptr, &bit_count, sizeof(bit_count));
    ptr += sizeof(bit_count);
  }
  
  // check for duplicate construct
  RDIM_String8 blob = rdim_str8(buf, buf_size);
  RDI_U64 blob_hash = rdi_hash(buf, buf_size);
  void *lookup_ptr = rdim_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RDIM_Type*)lookup_ptr;
  if(result == 0)
  {
    // setup new node
    result = rdim_type_new(root);
    result->kind = RDI_TypeKind_Bitfield;
    result->byte_size = direct_type->byte_size;
    result->off = bit_off;
    result->count = bit_count;
    result->direct_type = direct_type;
    
    // save in construct map
    rdim_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  rdim_scratch_end(scratch);
  rdim_assert(result != 0);
  return result;
}

RDI_PROC RDIM_Type*
rdim_type_pointer(RDIM_Root *root, RDIM_Type *direct_type, RDI_TypeKind ptr_type_kind)
{
  rdim_requiref(root, (ptr_type_kind == RDI_TypeKind_Ptr ||
                       ptr_type_kind == RDI_TypeKind_LRef ||
                       ptr_type_kind == RDI_TypeKind_RRef),
                return root->nil_type,
                "Non-pointer type kind used to construct pointer type.");
  RDIM_Type *result = root->nil_type;
  RDIM_Temp scratch = rdim_scratch_begin(0, 0);
  
  // setup construct buffer
  RDI_U64 buf_size = sizeof(RDIM_TypeConstructKind) + sizeof(ptr_type_kind) + sizeof(direct_type->idx);
  RDI_U8 *buf = rdim_push_array(rdim_temp_arena(scratch), RDI_U8, buf_size);
  {
    RDI_U8 *ptr = buf;
    // "pointer"
    *(RDIM_TypeConstructKind*)ptr = RDIM_TypeConstructKind_Pointer;
    ptr += sizeof(RDIM_TypeConstructKind);
    // type_kind
    rdim_memcpy(ptr, &ptr_type_kind, sizeof(ptr_type_kind));
    ptr += sizeof(ptr_type_kind);
    // direct_type->idx
    rdim_memcpy(ptr, &direct_type->idx, sizeof(direct_type->idx));
    ptr += sizeof(direct_type->idx);
  }
  
  // check for duplicate construct
  RDIM_String8 blob = rdim_str8(buf, buf_size);
  RDI_U64 blob_hash = rdi_hash(buf, buf_size);
  void *lookup_ptr = rdim_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RDIM_Type*)lookup_ptr;
  if(result == 0)
  {
    // setup new node
    result = rdim_type_new(root);
    result->kind = ptr_type_kind;
    result->byte_size = root->addr_size;
    result->direct_type = direct_type;
    
    // save in construct map
    rdim_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  rdim_scratch_end(scratch);
  rdim_assert(result != 0);
  return result;
}

RDI_PROC RDIM_Type*
rdim_type_array(RDIM_Root *root, RDIM_Type *direct_type, RDI_U64 count)
{
  RDIM_Type *result = root->nil_type;
  RDIM_Temp scratch = rdim_scratch_begin(0, 0);
  
  // setup construct buffer
  RDI_U64 buf_size =
    sizeof(RDIM_TypeConstructKind) + sizeof(direct_type->idx) + sizeof(count);
  RDI_U8 *buf = rdim_push_array(rdim_temp_arena(scratch), RDI_U8, buf_size);
  {
    RDI_U8 *ptr = buf;
    // "array"
    *(RDIM_TypeConstructKind*)ptr = RDIM_TypeConstructKind_Array;
    ptr += sizeof(RDIM_TypeConstructKind);
    // direct_type->idx
    rdim_memcpy(ptr, &direct_type->idx, sizeof(direct_type->idx));
    ptr += sizeof(direct_type->idx);
    // count
    rdim_memcpy(ptr, &count, sizeof(count));
    ptr += sizeof(count);
  }
  
  // check for duplicate construct
  RDIM_String8 blob = rdim_str8(buf, buf_size);
  RDI_U64 blob_hash = rdi_hash(buf, buf_size);
  void *lookup_ptr = rdim_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RDIM_Type*)lookup_ptr;
  if(result == 0)
  {
    // setup new node
    result = rdim_type_new(root);
    result->kind = RDI_TypeKind_Array;
    result->count = count;
    result->direct_type = direct_type;
    result->byte_size = direct_type->byte_size*count;
    
    // save in construct map
    rdim_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  scratch_end(scratch);
  rdim_assert(result != 0);
  return result;
}

RDI_PROC RDIM_Type*
rdim_type_proc(RDIM_Root *root, RDIM_Type *return_type, struct RDIM_TypeList *params)
{
  RDIM_Type *result = root->nil_type;
  RDIM_Temp scratch = rdim_scratch_begin(0, 0);
  
  // setup construct buffer
  RDI_U64 buf_size = sizeof(RDIM_TypeConstructKind) + sizeof(return_type->idx)*(1 + params->count);
  RDI_U8 *buf = rdim_push_array(rdim_temp_arena(scratch), RDI_U8, buf_size);
  {
    RDI_U8 *ptr = buf;
    // "procedure"
    *(RDIM_TypeConstructKind*)ptr = RDIM_TypeConstructKind_Procedure;
    ptr += sizeof(RDIM_TypeConstructKind);
    // ret_type->idx
    rdim_memcpy(ptr, &return_type->idx, sizeof(return_type->idx));
    ptr += sizeof(return_type->idx);
    // (params ...)->idx
    for(RDIM_TypeNode *node = params->first;
        node != 0;
        node = node->next)
    {
      rdim_memcpy(ptr, &node->type->idx, sizeof(node->type->idx));
      ptr += sizeof(node->type->idx);
    }
  }
  
  // check for duplicate construct
  RDIM_String8 blob = rdim_str8(buf, buf_size);
  RDI_U64 blob_hash = rdi_hash(buf, buf_size);
  void *lookup_ptr = rdim_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RDIM_Type*)lookup_ptr;
  if(result == 0)
  {
    // setup param buffer
    RDIM_Type **param_types = rdim_push_array(root->arena, RDIM_Type*, params->count);
    {
      RDIM_Type **ptr = param_types;
      for(RDIM_TypeNode *node = params->first;
          node != 0;
          node = node->next)
      {
        *ptr = node->type;
        ptr += 1;
      }
    }
    
    // setup new node
    result = rdim_type_new(root);
    result->kind = RDI_TypeKind_Function;
    result->byte_size = root->addr_size;
    result->count = params->count;
    result->direct_type = return_type;
    result->param_types = param_types;
    
    // save in construct map
    rdim_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  rdim_scratch_end(scratch);
  rdim_assert(result != 0);
  return result;
}

RDI_PROC RDIM_Type*
rdim_type_method(RDIM_Root *root, RDIM_Type *this_type, RDIM_Type *return_type, struct RDIM_TypeList *params)
{
  RDIM_Type *result = root->nil_type;
  RDIM_Temp scratch = rdim_scratch_begin(0, 0);
  
  // setup construct buffer
  RDI_U64 buf_size =
    sizeof(RDIM_TypeConstructKind) + sizeof(return_type->idx)*(2 + params->count);
  RDI_U8 *buf = rdim_push_array(rdim_temp_arena(scratch), RDI_U8, buf_size);
  {
    RDI_U8 *ptr = buf;
    // "method"
    *(RDIM_TypeConstructKind*)ptr = RDIM_TypeConstructKind_Method;
    ptr += sizeof(RDIM_TypeConstructKind);
    // ret_type->idx
    rdim_memcpy(ptr, &return_type->idx, sizeof(return_type->idx));
    ptr += sizeof(return_type->idx);
    // this_type->idx
    rdim_memcpy(ptr, &this_type->idx, sizeof(this_type->idx));
    ptr += sizeof(this_type->idx);
    // (params ...)->idx
    for(RDIM_TypeNode *node = params->first;
        node != 0;
        node = node->next)
    {
      rdim_memcpy(ptr, &node->type->idx, sizeof(node->type->idx));
      ptr += sizeof(node->type->idx);
    }
  }
  
  // check for duplicate construct
  RDIM_String8 blob = rdim_str8(buf, buf_size);
  RDI_U64 blob_hash = rdi_hash(buf, buf_size);
  void *lookup_ptr = rdim_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RDIM_Type*)lookup_ptr;
  if(result == 0)
  {
    // setup param buffer
    RDIM_Type **param_types = rdim_push_array(root->arena, RDIM_Type*, params->count + 1);
    {
      RDIM_Type **ptr = param_types;
      {
        *ptr = this_type;
        ptr += 1;
      }
      for(RDIM_TypeNode *node = params->first;
          node != 0;
          node = node->next)
      {
        *ptr = node->type;
        ptr += 1;
      }
    }
    
    // setup new node
    result = rdim_type_new(root);
    result->kind = RDI_TypeKind_Method;
    result->byte_size = root->addr_size;
    result->count = params->count;
    result->direct_type = return_type;
    result->param_types = param_types;
    
    // save in construct map
    rdim_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  rdim_scratch_end(scratch);
  rdim_assert(result != 0);
  return result;
}

//- rjf: udt type constructors

RDI_PROC RDIM_Type*
rdim_type_udt(RDIM_Root *root, RDI_TypeKind record_type_kind, RDIM_String8 name, RDI_U64 size)
{
  rdim_requiref(root, (record_type_kind == RDI_TypeKind_Struct ||
                       record_type_kind == RDI_TypeKind_Class ||
                       record_type_kind == RDI_TypeKind_Union),
                return root->nil_type,
                "Non-user-defined-type-kind used to create user-defined type.");
  
  // rjf: make type
  RDIM_Type *result = rdim_type_new(root);
  result->kind = record_type_kind;
  result->byte_size = size;
  result->name = rdim_str8_copy(root->arena, name);
  
  // rjf: save in name map
  {
    RDIM_NameMap *map = rdim_name_map_for_kind(root, RDI_NameMapKind_Types);
    rdim_name_map_add_pair(root, map, result->name, result->idx);
  }
  
  return result;
}

RDI_PROC RDIM_Type*
rdim_type_enum(RDIM_Root *root, RDIM_Type *direct_type, RDIM_String8 name)
{
  // rjf: make type
  RDIM_Type *result = rdim_type_new(root);
  result->kind = RDI_TypeKind_Enum;
  result->byte_size = direct_type->byte_size;
  result->name = rdim_str8_copy(root->arena, name);
  result->direct_type = direct_type;
  
  // rjf: save in name map
  {
    RDIM_NameMap *map = rdim_name_map_for_kind(root, RDI_NameMapKind_Types);
    rdim_name_map_add_pair(root, map, result->name, result->idx);
  }
  
  return result;
}

RDI_PROC RDIM_Type*
rdim_type_alias(RDIM_Root *root, RDIM_Type *direct_type, RDIM_String8 name)
{
  // rjf: make type
  RDIM_Type *result = rdim_type_new(root);
  result->kind = RDI_TypeKind_Alias;
  result->byte_size = direct_type->byte_size;
  result->name = rdim_str8_copy(root->arena, name);
  result->direct_type = direct_type;
  
  // rjf: save in name map
  {
    RDIM_NameMap *map = rdim_name_map_for_kind(root, RDI_NameMapKind_Types);
    rdim_name_map_add_pair(root, map, result->name, result->idx);
  }
  
  return result;
}

RDI_PROC RDIM_Type*
rdim_type_incomplete(RDIM_Root *root, RDI_TypeKind type_kind, RDIM_String8 name)
{
  rdim_requiref(root, (type_kind == RDI_TypeKind_IncompleteStruct ||
                       type_kind == RDI_TypeKind_IncompleteClass ||
                       type_kind == RDI_TypeKind_IncompleteUnion ||
                       type_kind == RDI_TypeKind_IncompleteEnum),
                return root->nil_type,
                "Non-incomplete-type-kind used to create incomplete type.");
  
  // rjf: make type
  RDIM_Type *result = rdim_type_new(root);
  result->kind = type_kind;
  result->name = rdim_str8_copy(root->arena, name);
  
  // save in name map
  {
    RDIM_NameMap *map = rdim_name_map_for_kind(root, RDI_NameMapKind_Types);
    rdim_name_map_add_pair(root, map, result->name, result->idx);
  }
  
  return result;
}

//- rjf: type member building

RDI_PROC void
rdim_type_add_member_data_field(RDIM_Root *root, RDIM_Type *record_type, RDIM_String8 name, RDIM_Type *mem_type, RDI_U32 off)
{
  RDIM_TypeUDT *udt = rdim_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RDIM_TypeMember *member = rdim_push_array(root->arena, RDIM_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RDI_MemberKind_DataField;
    member->name = rdim_str8_copy(root->arena, name);
    member->type = mem_type;
    member->off = off;
  }
}

RDI_PROC void
rdim_type_add_member_static_data(RDIM_Root *root, RDIM_Type *record_type, RDIM_String8 name, RDIM_Type *mem_type)
{
  RDIM_TypeUDT *udt = rdim_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RDIM_TypeMember *member = rdim_push_array(root->arena, RDIM_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RDI_MemberKind_StaticData;
    member->name = rdim_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

RDI_PROC void
rdim_type_add_member_method(RDIM_Root *root, RDIM_Type *record_type, RDIM_String8 name, RDIM_Type *mem_type)
{
  RDIM_TypeUDT *udt = rdim_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RDIM_TypeMember *member = rdim_push_array(root->arena, RDIM_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RDI_MemberKind_Method;
    member->name = rdim_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

RDI_PROC void
rdim_type_add_member_static_method(RDIM_Root *root, RDIM_Type *record_type, RDIM_String8 name, RDIM_Type *mem_type)
{
  RDIM_TypeUDT *udt = rdim_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RDIM_TypeMember *member = rdim_push_array(root->arena, RDIM_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    
    root->total_member_count += 1;
    
    member->kind = RDI_MemberKind_StaticMethod;
    member->name = rdim_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

RDI_PROC void
rdim_type_add_member_virtual_method(RDIM_Root *root, RDIM_Type *record_type, RDIM_String8 name, RDIM_Type *mem_type)
{
  RDIM_TypeUDT *udt = rdim_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RDIM_TypeMember *member = rdim_push_array(root->arena, RDIM_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RDI_MemberKind_VirtualMethod;
    member->name = rdim_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

RDI_PROC void
rdim_type_add_member_base(RDIM_Root *root, RDIM_Type *record_type, RDIM_Type *base_type, RDI_U32 off)
{
  RDIM_TypeUDT *udt = rdim_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RDIM_TypeMember *member = rdim_push_array(root->arena, RDIM_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RDI_MemberKind_Base;
    member->type = base_type;
    member->off = off;
  }
}

RDI_PROC void
rdim_type_add_member_virtual_base(RDIM_Root *root, RDIM_Type *record_type, RDIM_Type *base_type, RDI_U32 vptr_off, RDI_U32 vtable_off)
{
  RDIM_TypeUDT *udt = rdim_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RDIM_TypeMember *member = rdim_push_array(root->arena, RDIM_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RDI_MemberKind_VirtualBase;
    member->type = base_type;
    // TODO(allen): what to do with the two offsets in this case?
  }
}

RDI_PROC void
rdim_type_add_member_nested_type(RDIM_Root *root, RDIM_Type *record_type, RDIM_Type *nested_type)
{
  RDIM_TypeUDT *udt = rdim_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RDIM_TypeMember *member = rdim_push_array(root->arena, RDIM_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RDI_MemberKind_NestedType;
    member->type = nested_type;
  }
}

RDI_PROC void
rdim_type_add_enum_val(RDIM_Root *root, RDIM_Type *enum_type, RDIM_String8 name, RDI_U64 val)
{
  rdim_requiref(root, (enum_type->kind == RDI_TypeKind_Enum), return, "Tried to add enum value to non-enum type.");
  RDIM_TypeUDT *udt = rdim_type_udt_from_any_type(root, enum_type);
  if(udt != 0)
  {
    RDIM_TypeEnumVal *enum_val = rdim_push_array(root->arena, RDIM_TypeEnumVal, 1);
    SLLQueuePush(udt->first_enum_val, udt->last_enum_val, enum_val);
    udt->enum_val_count += 1;
    root->total_enum_val_count += 1;
    enum_val->name = rdim_str8_copy(root->arena, name);
    enum_val->val  = val;
  }
}

//- rjf: type source coordinate specifications
RDI_PROC void
rdim_type_set_source_coordinates(RDIM_Root *root, RDIM_Type *defined_type, RDIM_String8 source_path, RDI_U32 line, RDI_U32 col)
{
  rdim_requiref(root, (RDI_TypeKind_FirstUserDefined <= defined_type->kind && defined_type->kind <= RDI_TypeKind_LastUserDefined),
                return, "Tried to add source coordinates to non-user-defined type.");
  RDIM_TypeUDT *udt = rdim_type_udt_from_any_type(root, defined_type);
  if(udt != 0)
  {
    udt->source_path = rdim_str8_copy(root->arena, source_path);
    udt->line = line;
    udt->col = col;
  }
}

//- rjf: symbol info building

RDI_PROC RDIM_Symbol*
rdim_symbol_handle_from_user_id(RDIM_Root *root, RDI_U64 symbol_user_id, RDI_U64 symbol_user_id_hash)
{
  RDIM_U64ToPtrLookup lookup = {0};
  rdim_u64toptr_map_lookup(&root->symbol_map, symbol_user_id, symbol_user_id_hash, &lookup);
  RDIM_Symbol *result = 0;
  if(lookup.match != 0)
  {
    result = (RDIM_Symbol*)lookup.match;
  }
  else
  {
    result = rdim_push_array(root->arena, RDIM_Symbol, 1);
    RDIM_SLLQueuePush_N(root->first_symbol, root->last_symbol, result, next_order);
    root->symbol_count += 1;
    rdim_u64toptr_map_insert(root->arena, &root->symbol_map, symbol_user_id, symbol_user_id_hash, &lookup, result);
  }
  return result;
}

RDI_PROC void
rdim_symbol_set_info(RDIM_Root *root, RDIM_Symbol *symbol, RDIM_SymbolInfo *info)
{
  // rjf: unpack
  RDIM_SymbolKind kind = info->kind;
  RDIM_Symbol *container_symbol = info->container_symbol;
  RDIM_Type *container_type = info->container_type;
  
  // rjf: requirements
  rdim_requiref(root, RDIM_SymbolKind_NULL == symbol->kind, return, "Symbol information set multiple times.");
  rdim_requiref(root, RDIM_SymbolKind_NULL < info->kind && info->kind < RDIM_SymbolKind_COUNT, return, "Invalid symbol kind used to initialize symbol.");
  rdim_requiref(root, info->type != 0, return, "Invalid type used to initialize symbol.");
  rdim_requiref(root, info->container_symbol == 0 || info->container_type == 0, container_type = 0, "Symbol initialized with both a containing symbol and containing type, when only one is allowed.");
  
  // rjf: fill
  root->symbol_kind_counts[kind] += 1;
  symbol->idx = root->symbol_kind_counts[kind];
  symbol->kind = kind;
  symbol->name = rdim_str8_copy(root->arena, info->name);
  symbol->link_name = rdim_str8_copy(root->arena, info->link_name);
  symbol->type = info->type;
  symbol->is_extern = info->is_extern;
  symbol->offset = info->offset;
  symbol->container_symbol = container_symbol;
  symbol->container_type = container_type;
  
  // rjf: set root scope
  switch(kind)
  {
    default:{}break;
    case RDIM_SymbolKind_GlobalVariable:
    case RDIM_SymbolKind_ThreadVariable:
    {
      rdim_requiref(root, info->root_scope == 0, rdim_noop, "Global or thread variable initialized with root scope.");
    }break;
    case RDIM_SymbolKind_Procedure:
    {
      rdim_requiref(root, info->root_scope != 0, rdim_noop, "Procedure symbol initialized without root scope.");
      symbol->root_scope = info->root_scope;
      rdim_scope_recursive_set_symbol(info->root_scope, symbol);
    }break;
  }
  
  // save name map
  {
    RDIM_NameMap *map = 0;
    switch(kind)
    {
      default:{}break;
      case RDIM_SymbolKind_GlobalVariable:
      {
        map = rdim_name_map_for_kind(root, RDI_NameMapKind_GlobalVariables);
      }break;
      case RDIM_SymbolKind_ThreadVariable:
      {
        map = rdim_name_map_for_kind(root, RDI_NameMapKind_ThreadVariables);
      }break;
      case RDIM_SymbolKind_Procedure:
      {
        map = rdim_name_map_for_kind(root, RDI_NameMapKind_Procedures);
      }break;
    }
    if(map != 0)
    {
      rdim_name_map_add_pair(root, map, symbol->name, symbol->idx);
    }
  }
  
  // save link name map
  if(kind == RDIM_SymbolKind_Procedure && symbol->link_name.size > 0)
  {
    RDIM_NameMap *map = rdim_name_map_for_kind(root, RDI_NameMapKind_LinkNameProcedures);
    rdim_name_map_add_pair(root, map, symbol->link_name, symbol->idx);
  }
}

//- rjf: scope info building

RDI_PROC RDIM_Scope *
rdim_scope_handle_from_user_id(RDIM_Root *root, RDI_U64 scope_user_id, RDI_U64 scope_user_id_hash)
{
  RDIM_Scope *result = 0;
  RDIM_U64ToPtrLookup lookup = {0};
  rdim_u64toptr_map_lookup(&root->scope_map, scope_user_id, scope_user_id_hash, &lookup);
  if(lookup.match != 0)
  {
    result = (RDIM_Scope*)lookup.match;
  }
  else
  {
    result = rdim_push_array(root->arena, RDIM_Scope, 1);
    result->idx = root->scope_count;
    RDIM_SLLQueuePush_N(root->first_scope, root->last_scope, result, next_order);
    root->scope_count += 1;
    rdim_u64toptr_map_insert(root->arena, &root->scope_map, scope_user_id, scope_user_id_hash, &lookup, result);
  }
  return result;
}

RDI_PROC void
rdim_scope_set_parent(RDIM_Root *root, RDIM_Scope *scope, RDIM_Scope *parent)
{
  rdim_requiref(root, scope->parent_scope == 0, return, "Scope parent set multiple times.");
  rdim_requiref(root, parent != 0, return, "Tried to set invalid parent as scope parent.");
  scope->symbol = parent->symbol;
  scope->parent_scope = parent;
  RDIM_SLLQueuePush_N(parent->first_child, parent->last_child, scope, next_sibling);
}

RDI_PROC void
rdim_scope_add_voff_range(RDIM_Root *root, RDIM_Scope *scope, RDI_U64 voff_first, RDI_U64 voff_opl)
{
  RDIM_VOffRange *range = rdim_push_array(root->arena, RDIM_VOffRange, 1);
  SLLQueuePush(scope->first_range, scope->last_range, range);
  scope->range_count += 1;
  range->voff_first = voff_first;
  range->voff_opl   = voff_opl;
  scope->voff_base  = Min(scope->voff_base, voff_first);
  root->scope_voff_count += 2;
}

RDI_PROC void
rdim_scope_recursive_set_symbol(RDIM_Scope *scope, RDIM_Symbol *symbol)
{
  scope->symbol = symbol;
  for(RDIM_Scope *node = scope->first_child;
      node != 0;
      node = node->next_sibling)
  {
    rdim_scope_recursive_set_symbol(node, symbol);
  }
}

//- rjf: local info building

RDI_PROC RDIM_Local*
rdim_local_handle_from_user_id(RDIM_Root *root, RDI_U64 local_user_id, RDI_U64 local_user_id_hash)
{
  RDIM_Local *result = 0;
  RDIM_U64ToPtrLookup lookup = {0};
  rdim_u64toptr_map_lookup(&root->local_map, local_user_id, local_user_id_hash, &lookup);
  if(lookup.match != 0)
  {
    result = (RDIM_Local*)lookup.match;
  }
  else
  {
    result = rdim_push_array(root->arena, RDIM_Local, 1);
    rdim_u64toptr_map_insert(root->arena, &root->local_map, local_user_id, local_user_id_hash, &lookup, result);
  }
  return result;
}

RDI_PROC void
rdim_local_set_basic_info(RDIM_Root *root, RDIM_Local *local, RDIM_LocalInfo *info)
{
  rdim_requiref(root, local->kind == RDI_LocalKind_NULL, return, "Local information set multiple times.");
  rdim_requiref(root, info->scope != 0, return, "Tried to set invalid scope as local's containing scope.");
  rdim_requiref(root, RDI_LocalKind_NULL < info->kind && info->kind < RDI_LocalKind_COUNT, return, "Invalid local kind.");
  rdim_requiref(root, info->type != 0, return, "Tried to set invalid type as local's type.");
  RDIM_Scope *scope = info->scope;
  SLLQueuePush(scope->first_local, scope->last_local, local);
  scope->local_count += 1;
  root->local_count += 1;
  local->kind = info->kind;
  local->name = rdim_str8_copy(root->arena, info->name);
  local->type = info->type;
}

RDI_PROC RDIM_LocationSet*
rdim_location_set_from_local(RDIM_Root *root, RDIM_Local *local)
{
  RDIM_LocationSet *result = local->locset;
  if(result == 0)
  {
    local->locset = rdim_push_array(root->arena, RDIM_LocationSet, 1);
    result = local->locset;
  }
  return result;
}

//- rjf: location info building

RDI_PROC void
rdim_location_set_add_case(RDIM_Root *root, RDIM_LocationSet *locset, RDI_U64 voff_first, RDI_U64 voff_opl, RDIM_Location *location)
{
  RDIM_LocationCase *location_case = rdim_push_array(root->arena, RDIM_LocationCase, 1);
  SLLQueuePush(locset->first_location_case, locset->last_location_case, location_case);
  locset->location_case_count += 1;
  root->location_count += 1;
  location_case->voff_first = voff_first;
  location_case->voff_opl   = voff_opl;
  location_case->location   = location;
}

RDI_PROC RDIM_Location*
rdim_location_addr_bytecode_stream(RDIM_Root *root, struct RDIM_EvalBytecode *bytecode)
{
  RDIM_Location *result = rdim_push_array(root->arena, RDIM_Location, 1);
  result->kind = RDI_LocationKind_AddrBytecodeStream;
  result->bytecode = *bytecode;
  return result;
}

RDI_PROC RDIM_Location*
rdim_location_val_bytecode_stream(RDIM_Root *root, struct RDIM_EvalBytecode *bytecode)
{
  RDIM_Location *result = rdim_push_array(root->arena, RDIM_Location, 1);
  result->kind = RDI_LocationKind_ValBytecodeStream;
  result->bytecode = *bytecode;
  return result;
}

RDI_PROC RDIM_Location*
rdim_location_addr_reg_plus_u16(RDIM_Root *root, RDI_U8 reg_code, RDI_U16 offset)
{
  RDIM_Location *result = rdim_push_array(root->arena, RDIM_Location, 1);
  result->kind = RDI_LocationKind_AddrRegisterPlusU16;
  result->register_code = reg_code;
  result->offset = offset;
  return result;
}

RDI_PROC RDIM_Location*
rdim_location_addr_addr_reg_plus_u16(RDIM_Root *root, RDI_U8 reg_code, RDI_U16 offset)
{
  RDIM_Location *result = rdim_push_array(root->arena, RDIM_Location, 1);
  result->kind = RDI_LocationKind_AddrAddrRegisterPlusU16;
  result->register_code = reg_code;
  result->offset = offset;
  return result;
}

RDI_PROC RDIM_Location*
rdim_location_val_reg(RDIM_Root *root, RDI_U8 reg_code)
{
  RDIM_Location *result = rdim_push_array(root->arena, RDIM_Location, 1);
  result->kind = RDI_LocationKind_ValRegister;
  result->register_code = reg_code;
  return result;
}

//- rjf: name map building

RDI_PROC RDIM_NameMap*
rdim_name_map_for_kind(RDIM_Root *root, RDI_NameMapKind kind)
{
  RDIM_NameMap *result = 0;
  if(kind < RDI_NameMapKind_COUNT)
  {
    if(root->name_maps[kind] == 0)
    {
      root->name_maps[kind] = rdim_push_array(root->arena, RDIM_NameMap, 1);
      root->name_maps[kind]->buckets_count = 16384;
      root->name_maps[kind]->buckets = rdim_push_array(root->arena, RDIM_NameMapNode *, root->name_maps[kind]->buckets_count);
    }
    result = root->name_maps[kind];
  }
  return result;
}

RDI_PROC void
rdim_name_map_add_pair(RDIM_Root *root, RDIM_NameMap *map, RDIM_String8 string, RDI_U32 idx)
{
  // hash
  RDI_U64 hash = rdi_hash(string.str, string.size);
  RDI_U64 bucket_idx = hash%map->buckets_count;
  
  // find existing name node
  RDIM_NameMapNode *match = 0;
  for(RDIM_NameMapNode *node = map->buckets[bucket_idx];
      node != 0;
      node = node->bucket_next)
  {
    if(rdim_str8_match(string, node->string, 0))
    {
      match = node;
      break;
    }
  }
  
  // make name node if necessary
  if(match == 0)
  {
    match = rdim_push_array(root->arena, RDIM_NameMapNode, 1);
    match->string = rdim_str8_copy(root->arena, string);
    RDIM_SLLStackPush_N(map->buckets[bucket_idx], match, bucket_next);
    RDIM_SLLQueuePush_N(map->first, map->last, match, order_next);
    map->name_count += 1;
    map->bucket_collision_count += (match->bucket_next != 0);
  }
  
  // find existing idx
  RDI_S32 existing_idx = 0;
  for(RDIM_NameMapIdxNode *node = match->idx_first;
      node != 0;
      node = node->next)
  {
    for(RDI_U32 i = 0; i < ArrayCount(node->idx); i += 1)
    {
      if(node->idx[i] == 0)
      {
        break;
      }
      if(node->idx[i] == idx)
      {
        existing_idx = 1;
        break;
      }
    }
  }
  
  // insert new idx if necessary
  if(!existing_idx)
  {
    RDIM_NameMapIdxNode *idx_node = match->idx_last;
    RDI_U32 insert_i = match->idx_count%ArrayCount(idx_node->idx);
    if(insert_i == 0)
    {
      idx_node = rdim_push_array(root->arena, RDIM_NameMapIdxNode, 1);
      SLLQueuePush(match->idx_first, match->idx_last, idx_node);
    }
    
    idx_node->idx[insert_i] = idx;
    match->idx_count += 1;
  }
}

////////////////////////////////
//~ rjf: Debug Info Baking (Loose -> Tight) Functions

//- rjf: bake context construction

RDI_PROC RDIM_BakeCtx*
rdim_bake_ctx_begin(RDIM_BakeParams *params)
{
  RDIM_Arena *arena = rdim_arena_alloc();
  RDIM_BakeCtx *result = rdim_push_array(arena, RDIM_BakeCtx, 1);
  result->arena = arena;
#define BKTCOUNT(x) ((x)?(u64_up_to_pow2(x)):(16384))
  result->strs.buckets_count = BKTCOUNT(params->strings_bucket_count);
  result->idxs.buckets_count = BKTCOUNT(params->idx_runs_bucket_count);
#undef BKTCOUNT
  result->strs.buckets = rdim_push_array(arena, RDIM_StringNode *, result->strs.buckets_count);
  result->idxs.buckets = rdim_push_array(arena, RDIM_IdxRunNode *, result->idxs.buckets_count);
  
  rdim_string(result, rdim_str8_lit(""));
  rdim_idx_run(result, 0, 0);
  
  result->tree = rdim_push_array(arena, RDIM_PathTree, 1);
  {
    RDIM_PathNode *nil_path_node = rdim_paths_new_node(result);
    nil_path_node->name = rdim_str8_lit("<NIL>");
    RDIM_SrcNode *nil_src_node = rdim_paths_new_src_node(result);
    nil_src_node->path_node = nil_path_node;
    nil_src_node->normal_full_path = rdim_str8_lit("<NIL>");
    nil_path_node->src_file = nil_src_node;
  }
  
  return result;
}

RDI_PROC void
rdim_bake_ctx_release(RDIM_BakeCtx *bake_ctx)
{
  arena_release(bake_ctx->arena);
}

//- rjf: string baking

RDI_PROC RDI_U32
rdim_string(RDIM_BakeCtx *bctx, RDIM_String8 str)
{
  RDIM_Arena *arena = bctx->arena;
  RDIM_Strings *strs = &bctx->strs;
  RDI_U64 hash = rdi_hash(str.str, str.size);
  RDI_U64 bucket_idx = hash%strs->buckets_count;
  
  // look for a match
  RDIM_StringNode *match = 0;
  for(RDIM_StringNode *node = strs->buckets[bucket_idx];
      node != 0;
      node = node->bucket_next)
  {
    if(node->hash == hash && rdim_str8_match(node->str, str, 0))
    {
      match = node;
      break;
    }
  }
  
  // insert new node if no match
  if(match == 0)
  {
    RDIM_StringNode *node = rdim_push_array_no_zero(arena, RDIM_StringNode, 1);
    node->str = rdim_str8_copy(arena, str);
    node->hash = hash;
    node->idx = strs->count;
    strs->count += 1;
    RDIM_SLLQueuePush_N(strs->order_first, strs->order_last, node, order_next);
    RDIM_SLLStackPush_N(strs->buckets[bucket_idx], node, bucket_next);
    match = node;
    strs->bucket_collision_count += (node->bucket_next != 0);
  }
  
  // extract idx to return
  rdim_assert(match != 0);
  RDI_U32 result = match->idx;
  return result;
}

//- rjf: idx run baking

RDI_PROC RDI_U64
rdim_idx_run_hash(RDI_U32 *idx_run, RDI_U32 count)
{
  RDI_U64 hash = 5381;
  RDI_U32 *ptr = idx_run;
  RDI_U32 *opl = idx_run + count;
  for(;ptr < opl; ptr += 1)
  {
    hash = ((hash << 5) + hash) + (*ptr);
  }
  return(hash);
}

RDI_PROC RDI_U32
rdim_idx_run(RDIM_BakeCtx *bctx, RDI_U32 *idx_run, RDI_U32 count)
{
  RDIM_Arena *arena = bctx->arena;
  RDIM_IdxRuns *idxs = &bctx->idxs;
  
  RDI_U64 hash = rdim_idx_run_hash(idx_run, count);
  RDI_U64 bucket_idx = hash%idxs->buckets_count;
  
  // look for a match
  RDIM_IdxRunNode *match = 0;
  for(RDIM_IdxRunNode *node = idxs->buckets[bucket_idx];
      node != 0;
      node = node->bucket_next)
  {
    if(node->hash == hash)
    {
      RDI_S32 is_match = 1;
      RDI_U32 *node_idx = node->idx_run;
      for(RDI_U32 i = 0; i < count; i += 1)
      {
        if(node_idx[i] != idx_run[i])
        {
          is_match = 0;
          break;
        }
      }
      if(is_match)
      {
        match = node;
        break;
      }
    }
  }
  
  // insert new node if no match
  if(match == 0)
  {
    RDIM_IdxRunNode *node = rdim_push_array_no_zero(arena, RDIM_IdxRunNode, 1);
    RDI_U32 *idx_run_copy = rdim_push_array_no_zero(arena, RDI_U32, count);
    for(RDI_U32 i = 0; i < count; i += 1)
    {
      idx_run_copy[i] = idx_run[i];
    }
    node->idx_run = idx_run_copy;
    node->hash = hash;
    node->count = count;
    node->first_idx = idxs->idx_count;
    idxs->count += 1;
    idxs->idx_count += count;
    RDIM_SLLQueuePush_N(idxs->order_first, idxs->order_last, node, order_next);
    RDIM_SLLStackPush_N(idxs->buckets[bucket_idx], node, bucket_next);
    match = node;
    idxs->bucket_collision_count += (node->bucket_next != 0);
  }
  
  // extract idx to return
  rdim_assert(match != 0);
  RDI_U32 result = match->first_idx;
  return result;
}

//- rjf: data section baking

RDI_PROC RDI_U32
rdim_dsection(RDIM_Arena *arena, RDIM_DSections *dss, void *data, RDI_U64 size, RDI_DataSectionTag tag)
{
  RDI_U32 result = dss->count;
  RDIM_DSectionNode *node = rdim_push_array(arena, RDIM_DSectionNode, 1);
  SLLQueuePush(dss->first, dss->last, node);
  node->data = data;
  node->size = size;
  node->tag = tag;
  dss->count += 1;
  return result;
}

//- rjf: paths baking

RDI_PROC RDIM_String8
rdim_normal_string_from_path_node(RDIM_Arena *arena, RDIM_PathNode *node)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  RDIM_String8List list = {0};
  if(node != 0)
  {
    rdim_normal_string_from_path_node_build(scratch.arena, node, &list);
  }
  RDIM_String8 result = rdim_str8_list_join(arena, &list, rdim_str8_lit("/"));
  {
    RDI_U8 *ptr = result.str;
    RDI_U8 *opl = result.str + result.size;
    for(; ptr < opl; ptr += 1)
    {
      RDI_U8 c = *ptr;
      if('A' <= c && c <= 'Z') { c += 'a' - 'A'; }
      *ptr = c;
    }
  }
  scratch_end(scratch);
  return result;
}

RDI_PROC void
rdim_normal_string_from_path_node_build(RDIM_Arena *arena, RDIM_PathNode *node, RDIM_String8List *out)
{
  // TODO(rjf): why is this recursive...
  if(node->parent != 0)
  {
    rdim_normal_string_from_path_node_build(arena, node->parent, out);
  }
  if(node->name.size > 0)
  {
    rdim_str8_list_push(arena, out, node->name);
  }
}

RDI_PROC RDIM_PathNode*
rdim_paths_new_node(RDIM_BakeCtx *bctx)
{
  RDIM_PathTree *tree = bctx->tree;
  RDIM_PathNode *result = rdim_push_array(bctx->arena, RDIM_PathNode, 1);
  RDIM_SLLQueuePush_N(tree->first, tree->last, result, next_order);
  result->idx = tree->count;
  tree->count += 1;
  return result;
}

RDI_PROC RDIM_PathNode*
rdim_paths_sub_path(RDIM_BakeCtx *bctx, RDIM_PathNode *dir, RDIM_String8 sub_dir)
{
  // look for existing match
  RDIM_PathNode *match = 0;
  for(RDIM_PathNode *node = dir->first_child;
      node != 0;
      node = node->next_sibling)
  {
    if(rdim_str8_match(node->name, sub_dir, RDIM_StringMatchFlag_CaseInsensitive))
    {
      match = node;
      break;
    }
  }
  
  // construct new node if no match
  RDIM_PathNode *new_node = 0;
  if(match == 0){
    new_node = rdim_paths_new_node(bctx);
    new_node->parent = dir;
    RDIM_SLLQueuePush_N(dir->first_child, dir->last_child, new_node, next_sibling);
    new_node->name = rdim_str8_copy(bctx->arena, sub_dir);
  }
  
  // select result from the two paths
  RDIM_PathNode *result = match;
  if(match == 0){
    result = new_node;
  }
  
  return result;
}

RDI_PROC RDIM_PathNode*
rdim_paths_node_from_path(RDIM_BakeCtx *bctx,  RDIM_String8 path)
{
  RDIM_PathNode *node_cursor = &bctx->tree->root;
  
  RDI_U8 *ptr = path.str;
  RDI_U8 *opl = path.str + path.size;
  for(;ptr < opl;){
    // skip past slashes
    for(;ptr < opl && (*ptr == '/' || *ptr == '\\'); ptr += 1);
    
    // save beginning of non-slash range
    RDI_U8 *range_first = ptr;
    
    // skip past non-slashes
    for(;ptr < opl && !(*ptr == '/' || *ptr == '\\'); ptr += 1);
    
    // if range is non-empty advance the node cursor
    if(range_first < ptr){
      RDIM_String8 sub_dir = rdim_str8(range_first, (RDI_U64)(ptr-range_first));
      node_cursor = rdim_paths_sub_path(bctx, node_cursor, sub_dir);
    }
  }
  
  RDIM_PathNode *result = node_cursor;
  return result;
}

RDI_PROC RDI_U32
rdim_paths_idx_from_path(RDIM_BakeCtx *bctx, RDIM_String8 path)
{
  RDIM_PathNode *node = rdim_paths_node_from_path(bctx, path);
  RDI_U32 result = node->idx;
  return result;
}

RDI_PROC RDIM_SrcNode*
rdim_paths_new_src_node(RDIM_BakeCtx *bctx)
{
  RDIM_PathTree *tree = bctx->tree;
  RDIM_SrcNode *result = rdim_push_array(bctx->arena, RDIM_SrcNode, 1);
  SLLQueuePush(tree->src_first, tree->src_last, result);
  result->idx = tree->src_count;
  tree->src_count += 1;
  return result;
}

RDI_PROC RDIM_SrcNode*
rdim_paths_src_node_from_path_node(RDIM_BakeCtx *bctx, RDIM_PathNode *path_node)
{
  RDIM_SrcNode *result = path_node->src_file;
  if(result == 0)
  {
    RDIM_SrcNode *new_node = rdim_paths_new_src_node(bctx);
    new_node->path_node = path_node;
    new_node->normal_full_path = rdim_normal_string_from_path_node(bctx->arena, path_node);
    result = path_node->src_file = new_node;
  }
  return result;
}

//- rjf: per-unit line info baking

RDI_PROC RDIM_UnitLinesCombined*
rdim_unit_combine_lines(RDIM_Arena *arena, RDIM_BakeCtx *bctx, RDIM_LineSequenceNode *first_seq)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  
  // gather up all line info into two arrays
  //  keys: sortable array; pairs voffs with line info records; null records are sequence enders
  //  recs: contains all the source coordinates for a range of voffs
  RDI_U64 line_count = 0;
  RDI_U64 seq_count = 0;
  for(RDIM_LineSequenceNode *node = first_seq;
      node != 0;
      node = node->next)
  {
    seq_count += 1;
    line_count += node->line_seq.line_count;
  }
  
  RDI_U64 key_count = line_count + seq_count;
  RDIM_SortKey *line_keys = rdim_push_array_no_zero(scratch.arena, RDIM_SortKey, key_count);
  RDIM_LineRec *line_recs = rdim_push_array_no_zero(scratch.arena, RDIM_LineRec, line_count);
  
  {
    RDIM_SortKey *key_ptr = line_keys;
    RDIM_LineRec *rec_ptr = line_recs;
    
    for(RDIM_LineSequenceNode *node = first_seq;
        node != 0;
        node = node->next)
    {
      RDIM_PathNode *src_path =
        rdim_paths_node_from_path(bctx, node->line_seq.file_name);
      RDIM_SrcNode *src_file  = rdim_paths_src_node_from_path_node(bctx, src_path);
      RDI_U32 file_id = src_file->idx;
      
      RDI_U64 node_line_count = node->line_seq.line_count;
      for(RDI_U64 i = 0; i < node_line_count; i += 1){
        key_ptr->key = node->line_seq.voffs[i];
        key_ptr->val = rec_ptr;
        key_ptr += 1;
        
        rec_ptr->file_id = file_id;
        rec_ptr->line_num = node->line_seq.line_nums[i];
        if(node->line_seq.col_nums != 0){
          rec_ptr->col_first = node->line_seq.col_nums[i*2];
          rec_ptr->col_opl = node->line_seq.col_nums[i*2 + 1];
        }
        rec_ptr += 1;
      }
      
      key_ptr->key = node->line_seq.voffs[node_line_count];
      key_ptr->val = 0;
      key_ptr += 1;
      
      RDIM_LineMapFragment *fragment = rdim_push_array(arena, RDIM_LineMapFragment, 1);
      RDIM_SLLQueuePush(src_file->first_fragment, src_file->last_fragment, fragment);
      fragment->sequence = node;
    }
  }
  
  // sort
  RDIM_SortKey *sorted_line_keys = rdim_sort_key_array(scratch.arena, line_keys, key_count);
  
  // TODO(allen): do a pass over sorted keys to make sure duplicate keys are sorted with
  // null record first, and no more than one null record and one non-null record
  
  // arrange output
  RDI_U64 *arranged_voffs = rdim_push_array_no_zero(arena, RDI_U64, key_count + 1);
  RDI_Line *arranged_lines = rdim_push_array_no_zero(arena, RDI_Line, key_count);
  
  for(RDI_U64 i = 0; i < key_count; i += 1){
    arranged_voffs[i] = sorted_line_keys[i].key;
  }
  arranged_voffs[key_count] = ~0ull;
  for(RDI_U64 i = 0; i < key_count; i += 1){
    RDIM_LineRec *rec = (RDIM_LineRec*)sorted_line_keys[i].val;
    if(rec != 0){
      arranged_lines[i].file_idx = rec->file_id;
      arranged_lines[i].line_num = rec->line_num;
    }
    else{
      arranged_lines[i].file_idx = 0;
      arranged_lines[i].line_num = 0;
    }
  }
  
  RDIM_UnitLinesCombined *result = rdim_push_array(arena, RDIM_UnitLinesCombined, 1);
  result->voffs = arranged_voffs;
  result->lines = arranged_lines;
  result->cols = 0;
  result->line_count = key_count;
  
  scratch_end(scratch);
  return result;
}

//- rjf: per-src line info baking

RDI_PROC RDIM_SrcLinesCombined*
rdim_source_combine_lines(RDIM_Arena *arena, RDIM_LineMapFragment *first)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  
  // gather line number map
  RDIM_SrcLineMapBucket *first_bucket = 0;
  RDIM_SrcLineMapBucket *last_bucket = 0;
  RDI_U64 line_hash_slots_count = 1024;
  RDIM_SrcLineMapBucket **line_hash_slots = rdim_push_array(scratch.arena, RDIM_SrcLineMapBucket *, line_hash_slots_count);
  RDI_U64 line_count = 0;
  RDI_U64 voff_count = 0;
  RDI_U64 max_line_num = 0;
  {
    for(RDIM_LineMapFragment *map_fragment = first;
        map_fragment != 0;
        map_fragment = map_fragment->next)
    {
      RDIM_LineSequence *sequence = &map_fragment->sequence->line_seq;
      
      RDI_U64 *seq_voffs = sequence->voffs;
      RDI_U32 *seq_line_nums = sequence->line_nums;
      RDI_U64 seq_line_count = sequence->line_count;
      for(RDI_U64 i = 0; i < seq_line_count; i += 1){
        RDI_U32 line_num = seq_line_nums[i];
        RDI_U64 voff = seq_voffs[i];
        RDI_U64 line_hash_slot_idx = line_num%line_hash_slots_count;
        
        // update unique voff counter & max line number
        voff_count += 1;
        max_line_num = Max(max_line_num, line_num);
        
        // find match
        RDIM_SrcLineMapBucket *match = 0;
        {
          for(RDIM_SrcLineMapBucket *node = line_hash_slots[line_hash_slot_idx];
              node != 0;
              node = node->hash_next){
            if(node->line_num == line_num){
              match = node;
              break;
            }
          }
        }
        
        // introduce new line if no match
        if(match == 0){
          match = rdim_push_array(scratch.arena, RDIM_SrcLineMapBucket, 1);
          RDIM_SLLQueuePush_N(first_bucket, last_bucket, match, order_next);
          RDIM_SLLStackPush_N(line_hash_slots[line_hash_slot_idx], match, hash_next);
          match->line_num = line_num;
          line_count += 1;
        }
        
        // insert new voff
        {
          RDIM_SrcLineMapVoffBlock *block = rdim_push_array(scratch.arena, RDIM_SrcLineMapVoffBlock, 1);
          RDIM_SLLQueuePush(match->first_voff_block, match->last_voff_block, block);
          match->voff_count += 1;
          block->voff = voff;
        }
      }
    }
  }
  
  // bake sortable keys array
  RDIM_SortKey *keys = rdim_push_array_no_zero(scratch.arena, RDIM_SortKey, line_count);
  {
    RDIM_SortKey *key_ptr = keys;
    for(RDIM_SrcLineMapBucket *node = first_bucket;
        node != 0;
        node = node->order_next, key_ptr += 1){
      key_ptr->key = node->line_num;
      key_ptr->val = node;
    }
  }
  
  // sort
  RDIM_SortKey *sorted_keys = rdim_sort_key_array(scratch.arena, keys, line_count);
  
  // bake result
  RDI_U32 *line_nums = rdim_push_array_no_zero(arena, RDI_U32, line_count);
  RDI_U32 *line_ranges = rdim_push_array_no_zero(arena, RDI_U32, line_count + 1);
  RDI_U64 *voffs = rdim_push_array_no_zero(arena, RDI_U64, voff_count);
  {
    RDI_U64 *voff_ptr = voffs;
    for(RDI_U32 i = 0; i < line_count; i += 1){
      line_nums[i] = sorted_keys[i].key;
      line_ranges[i] = (RDI_U32)(voff_ptr - voffs);
      RDIM_SrcLineMapBucket *bucket = (RDIM_SrcLineMapBucket*)sorted_keys[i].val;
      for(RDIM_SrcLineMapVoffBlock *node = bucket->first_voff_block;
          node != 0;
          node = node->next){
        *voff_ptr = node->voff;
        voff_ptr += 1;
      }
    }
    line_ranges[line_count] = voff_count;
  }
  
  RDIM_SrcLinesCombined *result = rdim_push_array(arena, RDIM_SrcLinesCombined, 1);
  result->line_nums = line_nums;
  result->line_ranges = line_ranges;
  result->line_count = line_count;
  result->voffs = voffs;
  result->voff_count = voff_count;
  
  scratch_end(scratch);
  return result;
}

//- rjf: vmap baking
RDI_PROC RDIM_VMap
rdim_vmap_from_markers(RDIM_Arena *arena, RDIM_VMapMarker *markers, RDIM_SortKey *keys, RDI_U64 marker_count)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  
  // sort markers
  RDIM_SortKey *sorted_keys = rdim_sort_key_array(scratch.arena, keys, marker_count);
  
  // determine if an extra vmap entry for zero is needed
  RDI_U32 extra_vmap_entry = 0;
  if(marker_count > 0 && sorted_keys[0].key != 0){
    extra_vmap_entry = 1;
  }
  
  // fill output vmap entries
  RDI_U32 vmap_count_raw = marker_count - 1 + extra_vmap_entry;
  RDI_VMapEntry *vmap = rdim_push_array_no_zero(arena, RDI_VMapEntry, vmap_count_raw + 1);
  RDI_U32 vmap_entry_count_pass_1 = 0;
  
  {
    RDI_VMapEntry *vmap_ptr = vmap;
    
    if(extra_vmap_entry){
      vmap_ptr->voff = 0;
      vmap_ptr->idx = 0;
      vmap_ptr += 1;
    }
    
    RDIM_VMapRangeTracker *tracker_stack = 0;
    RDIM_VMapRangeTracker *tracker_free = 0;
    
    RDIM_SortKey *key_ptr = sorted_keys;
    RDIM_SortKey *key_opl = sorted_keys + marker_count;
    for(;key_ptr < key_opl;){
      // get initial map state from tracker stack
      RDI_U32 initial_idx = (RDI_U32)0xffffffff;
      if(tracker_stack != 0){
        initial_idx = tracker_stack->idx;
      }
      
      // update tracker stack
      // * we must process _all_ of the changes that apply at this voff before moving on
      RDI_U64 voff = key_ptr->key;
      
      for(;key_ptr < key_opl && key_ptr->key == voff; key_ptr += 1){
        RDIM_VMapMarker *marker = (RDIM_VMapMarker*)key_ptr->val;
        RDI_U32 idx = marker->idx;
        
        // push to stack
        if(marker->begin_range){
          RDIM_VMapRangeTracker *new_tracker = tracker_free;
          if(new_tracker != 0){
            RDIM_SLLStackPop(tracker_free);
          }
          else{
            new_tracker = rdim_push_array(scratch.arena, RDIM_VMapRangeTracker, 1);
          }
          RDIM_SLLStackPush(tracker_stack, new_tracker);
          new_tracker->idx = idx;
        }
        
        // pop matching node from stack (not always the top)
        else{
          RDIM_VMapRangeTracker **ptr_in = &tracker_stack;
          RDIM_VMapRangeTracker *match = 0;
          for(RDIM_VMapRangeTracker *node = tracker_stack;
              node != 0;){
            if(node->idx == idx){
              match = node;
              break;
            }
            ptr_in = &node->next;
            node = node->next;
          }
          if(match != 0){
            *ptr_in = match->next;
            RDIM_SLLStackPush(tracker_free, match);
          }
        }
      }
      
      // get final map state from tracker stack
      RDI_U32 final_idx = 0;
      if(tracker_stack != 0){
        final_idx = tracker_stack->idx;
      }
      
      // if final is different from initial - emit new vmap entry
      if(final_idx != initial_idx){
        vmap_ptr->voff = voff;
        vmap_ptr->idx = final_idx;
        vmap_ptr += 1;
      }
    }
    
    vmap_entry_count_pass_1 = (RDI_U32)(vmap_ptr - vmap);
  }
  
  // replace zero unit indexes that follow a non-zero
  // TODO(rjf): 0 *is* a real unit index right now
  if(0)
  {
    //  (the last entry is not replaced because it acts as a terminator)
    RDI_U32 last = vmap_entry_count_pass_1 - 1;
    
    RDI_VMapEntry *vmap_ptr = vmap;
    RDI_U64 real_idx = 0;
    
    for(RDI_U32 i = 0; i < last; i += 1, vmap_ptr += 1){
      // is this a zero after a real index?
      if(vmap_ptr->idx == 0){
        vmap_ptr->idx = real_idx;
      }
      
      // remember a real index
      else{
        real_idx = vmap_ptr->idx;
      }
    }
  }
  
  // combine duplicate neighbors
  RDI_U32 vmap_entry_count = 0;
  {
    RDI_VMapEntry *vmap_ptr = vmap;
    RDI_VMapEntry *vmap_opl = vmap + vmap_entry_count_pass_1;
    RDI_VMapEntry *vmap_out = vmap;
    
    for(;vmap_ptr < vmap_opl;){
      RDI_VMapEntry *vmap_range_first = vmap_ptr;
      RDI_U64 idx = vmap_ptr->idx;
      vmap_ptr += 1;
      for(;vmap_ptr < vmap_opl && vmap_ptr->idx == idx;) vmap_ptr += 1;
      rdim_memcpy_struct(vmap_out, vmap_range_first);
      vmap_out += 1;
    }
    
    vmap_entry_count = (RDI_U32)(vmap_out - vmap);
  }
  
  // fill result
  RDIM_VMap *result = rdim_push_array(arena, RDIM_VMap, 1);
  result->vmap = vmap;
  result->count = vmap_entry_count - 1;
  
  rdim_scratch_end(scratch);
  
  return result;
}

RDI_PROC RDIM_VMap*
rdim_vmap_from_unit_ranges(RDIM_Arena *arena, RDIM_UnitVMapRange *first, RDI_U64 count)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  
  // count necessary markers
  RDI_U64 marker_count = count*2;
  
  // fill markers
  RDIM_SortKey    *keys = rdim_push_array_no_zero(scratch.arena, RDIM_SortKey, marker_count);
  RDIM_VMapMarker *markers = rdim_push_array_no_zero(scratch.arena, RDIM_VMapMarker, marker_count);
  
  {
    RDIM_SortKey *key_ptr = keys;
    RDIM_VMapMarker *marker_ptr = markers;
    for(RDIM_UnitVMapRange *range = first;
        range != 0;
        range = range->next){
      if(range->first < range->opl){
        RDI_U32 unit_idx = range->unit->idx;
        
        key_ptr->key = range->first;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = unit_idx;
        marker_ptr->begin_range = 1;
        key_ptr += 1;
        marker_ptr += 1;
        
        key_ptr->key = range->opl;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = unit_idx;
        marker_ptr->begin_range = 0;
        key_ptr += 1;
        marker_ptr += 1;
      }
    }
  }
  
  // construct vmap
  RDIM_VMap *result = rdim_vmap_from_markers(arena, markers, keys, marker_count);
  rdim_scratch_end(scratch);
  return result;
}

//- rjf: type info baking

RDI_PROC RDI_U32*
rdim_idx_run_from_types(RDIM_Arena *arena, RDIM_Type **types, RDI_U32 count)
{
  RDI_U32 *result = rdim_push_array(arena, RDI_U32, count);
  for(RDI_U32 i = 0; i < count; i += 1){
    result[i] = types[i]->idx;
  }
  return result;
}

RDI_PROC RDIM_TypeData*
rdim_type_data_combine(RDIM_Arena *arena, RDIM_Root *root, RDIM_BakeCtx *bctx)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  
  // fill type nodes
  RDI_U32 type_count = root->type_count;
  RDI_TypeNode *type_nodes = rdim_push_array_no_zero(arena, RDI_TypeNode, type_count);
  
  {
    RDI_TypeNode *ptr = type_nodes;
    RDI_TypeNode *opl = ptr + type_count;
    RDIM_Type *loose_type = root->first_type;
    for(;loose_type != 0 && ptr < opl;
        loose_type = loose_type->next_order, ptr += 1){
      
      RDI_TypeKind kind = loose_type->kind;
      
      // shared
      ptr->kind = kind;
      ptr->flags = loose_type->flags;
      ptr->byte_size = loose_type->byte_size;
      
      // built-in
      if(RDI_TypeKind_FirstBuiltIn <= kind && kind <= RDI_TypeKind_LastBuiltIn){
        ptr->built_in.name_string_idx = rdim_string(bctx, loose_type->name);
      }
      
      // constructed
      else if(RDI_TypeKind_FirstConstructed <= kind && kind <= RDI_TypeKind_LastConstructed){
        ptr->constructed.direct_type_idx = loose_type->direct_type->idx;
        
        switch (kind){
          case RDI_TypeKind_Array:
          {
            ptr->constructed.count = loose_type->count;
          }break;
          
          case RDI_TypeKind_Function:
          {
            // parameters
            RDI_U32 count = loose_type->count;
            RDI_U32 *idx_run = rdim_idx_run_from_types(scratch.arena, loose_type->param_types, count);
            ptr->constructed.param_idx_run_first = rdim_idx_run(bctx, idx_run, count);
            ptr->constructed.count = count;
          }break;
          
          case RDI_TypeKind_Method:
          {
            // parameters
            RDI_U32 count = loose_type->count;
            RDI_U32 *idx_run = rdim_idx_run_from_types(scratch.arena, loose_type->param_types, count);
            ptr->constructed.param_idx_run_first = rdim_idx_run(bctx, idx_run, count);
            ptr->constructed.count = count;
          }break;
        }
      }
      
      // user-defined
      else if(RDI_TypeKind_FirstUserDefined <= kind && kind <= RDI_TypeKind_LastUserDefined){
        ptr->user_defined.name_string_idx = rdim_string(bctx, loose_type->name);
        if(loose_type->udt != 0){
          ptr->user_defined.udt_idx = loose_type->udt->idx;
        }
        if(loose_type->direct_type != 0){
          ptr->user_defined.direct_type_idx = loose_type->direct_type->idx;
        }
      }
      
      // bitfield
      else if(kind == RDI_TypeKind_Bitfield){
        ptr->bitfield.off = loose_type->off;
        ptr->bitfield.size = loose_type->count;
      }
      
      rdim_scratch_end(scratch);
    }
    
    // both iterators should end at the same time
    rdim_assert(loose_type == 0);
    rdim_assert(ptr == opl);
  }
  
  
  // fill udts
  RDI_U32 udt_count = root->type_udt_count;
  RDI_UDT *udts = rdim_push_array_no_zero(arena, RDI_UDT, udt_count);
  
  RDI_U32 member_count = root->total_member_count;
  RDI_Member *members = rdim_push_array_no_zero(arena, RDI_Member, member_count);
  
  RDI_U32 enum_member_count = root->total_enum_val_count;
  RDI_EnumMember *enum_members = rdim_push_array_no_zero(arena, RDI_EnumMember, enum_member_count);
  
  {
    RDI_UDT *ptr = udts;
    RDI_UDT *opl = ptr + udt_count;
    
    RDI_Member *member_ptr = members;
    RDI_Member *member_opl = members + member_count;
    
    RDI_EnumMember *enum_member_ptr = enum_members;
    RDI_EnumMember *enum_member_opl = enum_members + enum_member_count;
    
    RDIM_TypeUDT *loose_udt = root->first_udt;
    for(;loose_udt != 0 && ptr < opl;
        loose_udt = loose_udt->next_order, ptr += 1){
      ptr->self_type_idx = loose_udt->self_type->idx;
      
      rdim_assert(loose_udt->member_count == 0 ||
                  loose_udt->enum_val_count == 0);
      
      // enum members
      if(loose_udt->enum_val_count != 0){
        ptr->flags |= RDI_UserDefinedTypeFlag_EnumMembers;
        
        ptr->member_first = (RDI_U32)(enum_member_ptr - enum_members);
        ptr->member_count = loose_udt->enum_val_count;
        
        RDI_U32 local_enum_val_count = loose_udt->enum_val_count;
        RDIM_TypeEnumVal *loose_enum_val = loose_udt->first_enum_val;
        for(RDI_U32 i = 0;
            i < local_enum_val_count;
            i += 1, enum_member_ptr += 1, loose_enum_val = loose_enum_val->next){
          enum_member_ptr->name_string_idx = rdim_string(bctx, loose_enum_val->name);
          enum_member_ptr->val = loose_enum_val->val;
        }
      }
      
      // struct/class/union members
      else{
        ptr->member_first = (RDI_U32)(member_ptr - members);
        ptr->member_count = loose_udt->member_count;
        
        RDI_U32 local_member_count = loose_udt->member_count;
        RDIM_TypeMember *loose_member = loose_udt->first_member;
        for(RDI_U32 i = 0;
            i < local_member_count;
            i += 1, member_ptr += 1, loose_member = loose_member->next){
          member_ptr->kind = loose_member->kind;
          // TODO(allen): member_ptr->visibility = ;
          member_ptr->name_string_idx = rdim_string(bctx, loose_member->name);
          member_ptr->off = loose_member->off;
          member_ptr->type_idx = loose_member->type->idx;
          
          // TODO(allen): 
          if(loose_member->kind == RDI_MemberKind_Method){
            //loose_member_ptr->unit_idx = ;
            //loose_member_ptr->proc_symbol_idx = ;
          }
        }
        
      }
      
      RDI_U32 file_idx = 0;
      if(loose_udt->source_path.size > 0){
        RDIM_PathNode *path_node = rdim_paths_node_from_path(bctx, loose_udt->source_path);
        RDIM_SrcNode  *src_node  = rdim_paths_src_node_from_path_node(bctx, path_node);
        file_idx = src_node->idx;
      }
      
      ptr->file_idx = file_idx;
      ptr->line = loose_udt->line;
      ptr->col = loose_udt->col;
    }
    
    // all iterators should end at the same time
    rdim_assert(loose_udt == 0);
    rdim_assert(ptr == opl);
    rdim_assert(member_ptr == member_opl);
    rdim_assert(enum_member_ptr == enum_member_opl);
  }
  
  
  // fill result
  RDIM_TypeData *result = rdim_push_array(arena, RDIM_TypeData, 1);
  result->type_nodes = type_nodes;
  result->type_node_count = type_count;
  result->udts = udts;
  result->udt_count = udt_count;
  result->members = members;
  result->member_count = member_count;
  result->enum_members = enum_members;
  result->enum_member_count = enum_member_count;
  
  rdim_scratch_end(scratch);
  return result;
}

//- rjf: symbol data baking

RDI_PROC RDIM_SymbolData*
rdim_symbol_data_combine(RDIM_Arena *arena, RDIM_Root *root, RDIM_BakeCtx *bctx)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  
  // count symbol kinds
  RDI_U32 globalvar_count = 1 + root->symbol_kind_counts[RDIM_SymbolKind_GlobalVariable];
  RDI_U32 threadvar_count = 1 + root->symbol_kind_counts[RDIM_SymbolKind_ThreadVariable];
  RDI_U32 procedure_count = 1 + root->symbol_kind_counts[RDIM_SymbolKind_Procedure];
  
  // allocate symbol arrays
  RDI_GlobalVariable *global_variables =
    rdim_push_array(arena, RDI_GlobalVariable, globalvar_count);
  
  RDI_ThreadVariable *thread_variables =
    rdim_push_array(arena, RDI_ThreadVariable, threadvar_count);
  
  RDI_Procedure *procedures = rdim_push_array(arena, RDI_Procedure, procedure_count);
  
  // fill symbol arrays
  {
    RDI_GlobalVariable *global_ptr = global_variables;
    RDI_ThreadVariable *thread_local_ptr = thread_variables;
    RDI_Procedure *procedure_ptr = procedures;
    
    // nils
    global_ptr += 1;
    thread_local_ptr += 1;
    procedure_ptr += 1;
    
    // symbol nodes
    for(RDIM_Symbol *node = root->first_symbol;
        node != 0;
        node = node->next_order){
      RDI_U32 name_string_idx = rdim_string(bctx, node->name);
      RDI_U32 link_name_string_idx = rdim_string(bctx, node->link_name);
      RDI_U32 type_idx = node->type->idx;
      
      RDI_LinkFlags link_flags = 0;
      RDI_U32 container_idx = 0;
      {      
        if(node->is_extern){
          link_flags |= RDI_LinkFlag_External;
        }
        if(node->container_symbol != 0){
          container_idx = node->container_symbol->idx;
          link_flags |= RDI_LinkFlag_ProcScoped;
        }
        else if(node->container_type != 0 && node->container_type->udt != 0){
          container_idx = node->container_type->udt->idx;
          link_flags |= RDI_LinkFlag_TypeScoped;
        }
      }
      
      switch (node->kind){
        default:{}break;
        
        case RDIM_SymbolKind_GlobalVariable:
        {
          global_ptr->name_string_idx = name_string_idx;
          global_ptr->link_flags = link_flags;
          global_ptr->voff = node->offset;
          global_ptr->type_idx = type_idx;
          global_ptr->container_idx = container_idx;
          global_ptr += 1;
        }break;
        
        case RDIM_SymbolKind_ThreadVariable:
        {
          thread_local_ptr->name_string_idx = name_string_idx;
          thread_local_ptr->link_flags = link_flags;
          thread_local_ptr->tls_off = (RDI_U32)node->offset;
          thread_local_ptr->type_idx = type_idx;
          thread_local_ptr->container_idx = container_idx;
          thread_local_ptr += 1;
        }break;
        
        case RDIM_SymbolKind_Procedure:
        {
          procedure_ptr->name_string_idx = name_string_idx;
          procedure_ptr->link_name_string_idx = link_name_string_idx;
          procedure_ptr->link_flags = link_flags;
          procedure_ptr->type_idx = type_idx;
          procedure_ptr->root_scope_idx = node->root_scope->idx;
          procedure_ptr->container_idx = container_idx;
          procedure_ptr += 1;
        }break;
      }
    }
    
    rdim_assert(global_ptr - global_variables == globalvar_count);
    rdim_assert(thread_local_ptr - thread_variables == threadvar_count);
    rdim_assert(procedure_ptr - procedures == procedure_count);
  }
  
  // global vmap
  RDIM_VMap *global_vmap = 0;
  {
    // count necessary markers
    RDI_U32 marker_count = globalvar_count*2;
    
    // fill markers
    RDIM_SortKey    *keys = rdim_push_array_no_zero(scratch.arena, RDIM_SortKey, marker_count);
    RDIM_VMapMarker *markers = rdim_push_array_no_zero(scratch.arena, RDIM_VMapMarker, marker_count);
    
    RDIM_SortKey *key_ptr = keys;
    RDIM_VMapMarker *marker_ptr = markers;
    
    // real globals
    for(RDIM_Symbol *node = root->first_symbol;
        node != 0;
        node = node->next_order){
      if(node->kind == RDIM_SymbolKind_GlobalVariable){
        RDI_U32 global_idx = node->idx;
        
        RDI_U64 first = node->offset;
        RDI_U64 opl   = first + node->type->byte_size;
        
        key_ptr->key = first;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = global_idx;
        marker_ptr->begin_range = 1;
        key_ptr += 1;
        marker_ptr += 1;
        
        key_ptr->key = opl;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = global_idx;
        marker_ptr->begin_range = 0;
        key_ptr += 1;
        marker_ptr += 1;
      }
    }
    
    // nil global
    {
      RDI_U32 global_idx = 0;
      
      RDI_U64 first = 0;
      RDI_U64 opl   = (RDI_U64)0xffffffffffffffffull;
      
      key_ptr->key = first;
      key_ptr->val = marker_ptr;
      marker_ptr->idx = global_idx;
      marker_ptr->begin_range = 1;
      key_ptr += 1;
      marker_ptr += 1;
      
      key_ptr->key = opl;
      key_ptr->val = marker_ptr;
      marker_ptr->idx = global_idx;
      marker_ptr->begin_range = 0;
      key_ptr += 1;
      marker_ptr += 1;
    }
    
    // assert we filled all the markers
    rdim_assert(key_ptr - keys == marker_count &&
                marker_ptr - markers == marker_count);
    
    // construct vmap
    global_vmap = rdim_vmap_from_markers(arena, markers, keys, marker_count);
  }
  
  // allocate scope array
  
  // (assert there is a nil scope)
  rdim_assert(root->first_scope != 0 &&
              root->first_scope->symbol == 0 &&
              root->first_scope->first_child == 0 &&
              root->first_scope->next_sibling == 0 &&
              root->first_scope->range_count == 0);
  
  RDI_U32 scope_count = root->scope_count;
  RDI_Scope *scopes = rdim_push_array(arena, RDI_Scope, scope_count);
  
  RDI_U32 scope_voff_count = root->scope_voff_count;
  RDI_U64 *scope_voffs = rdim_push_array(arena, RDI_U64, scope_voff_count);
  
  RDI_U32 local_count = root->local_count;
  RDI_Local *locals = rdim_push_array(arena, RDI_Local, local_count);
  
  RDI_U32 location_block_count = root->location_count;
  RDI_LocationBlock *location_blocks =
    rdim_push_array(arena, RDI_LocationBlock, location_block_count);
  
  RDIM_String8List location_data = {0};
  
  // iterate scopes, locals, and locations
  //  fill scope voffs, locals, and location information
  {
    RDI_Scope *scope_ptr = scopes;
    RDI_U64 *scope_voff_ptr = scope_voffs;
    RDI_Local *local_ptr = locals;
    RDI_LocationBlock *location_block_ptr = location_blocks;
    
    for(RDIM_Scope *node = root->first_scope;
        node != 0;
        node = node->next_order, scope_ptr += 1){
      
      // emit voffs
      RDI_U32 voff_first = (RDI_U32)(scope_voff_ptr - scope_voffs);
      for(RDIM_VOffRange *range = node->first_range;
          range != 0;
          range = range->next){
        *scope_voff_ptr = range->voff_first;
        scope_voff_ptr += 1;
        *scope_voff_ptr = range->voff_opl;
        scope_voff_ptr += 1;
      }
      RDI_U32 voff_opl = (RDI_U32)(scope_voff_ptr - scope_voffs);
      
      // emit locals
      RDI_U32 scope_local_count = node->local_count;
      RDI_U32 scope_local_first = (RDI_U32)(local_ptr - locals);
      for(RDIM_Local *slocal = node->first_local;
          slocal != 0;
          slocal = slocal->next, local_ptr += 1){
        local_ptr->kind = slocal->kind;
        local_ptr->name_string_idx = rdim_string(bctx, slocal->name);
        local_ptr->type_idx = slocal->type->idx;
        
        RDIM_LocationSet *locset = slocal->locset;
        if(locset != 0){
          RDI_U32 location_first = (RDI_U32)(location_block_ptr - location_blocks);
          RDI_U32 location_opl   = location_first + locset->location_case_count;
          local_ptr->location_first = location_first;
          local_ptr->location_opl   = location_opl;
          
          for(RDIM_LocationCase *location_case = locset->first_location_case;
              location_case != 0;
              location_case = location_case->next){
            location_block_ptr->scope_off_first = location_case->voff_first;
            location_block_ptr->scope_off_opl   = location_case->voff_opl;
            location_block_ptr->location_data_off = location_data.total_size;
            location_block_ptr += 1;
            
            RDIM_Location *location = location_case->location;
            if(location == 0){
              RDI_U64 data = 0;
              str8_serial_push_align(scratch.arena, &location_data, 8);
              str8_serial_push_data(scratch.arena, &location_data, &data, 1);
            }
            else{
              switch (location->kind){
                default:
                {
                  RDI_U64 data = 0;
                  str8_serial_push_align(scratch.arena, &location_data, 8);
                  str8_serial_push_data(scratch.arena, &location_data, &data, 1);
                }break;
                
                case RDI_LocationKind_AddrBytecodeStream:
                case RDI_LocationKind_ValBytecodeStream:
                {
                  rdim_str8_list_push(scratch.arena, &location_data, rdim_str8_copy(scratch.arena, rdim_str8_struct(&location->kind)));
                  for(RDIM_EvalBytecodeOp *op_node = location->bytecode.first_op;
                      op_node != 0;
                      op_node = op_node->next){
                    RDI_U8 op_data[9];
                    op_data[0] = op_node->op;
                    rdim_memcpy(op_data + 1, &op_node->p, op_node->p_size);
                    RDIM_String8 op_data_str = rdim_str8(op_data, 1 + op_node->p_size);
                    rdim_str8_list_push(scratch.arena, &location_data, rdim_str8_copy(scratch.arena, op_data_str));
                  }
                  {
                    RDI_U64 data = 0;
                    RDIM_String8 data_str = rdim_str8((RDI_U8 *)&data, 1);
                    rdim_str8_list_push(scratch.arena, &location_data, rdim_str8_copy(scratch.arena, data_str));
                  }
                }break;
                
                case RDI_LocationKind_AddrRegisterPlusU16:
                case RDI_LocationKind_AddrAddrRegisterPlusU16:
                {
                  RDI_LocationRegisterPlusU16 loc = {0};
                  loc.kind = location->kind;
                  loc.register_code = location->register_code;
                  loc.offset = location->offset;
                  rdim_str8_list_push(scratch.arena, &location_data, rdim_str8_copy(scratch.arena, rdim_str8_struct(&loc)));
                }break;
                
                case RDI_LocationKind_ValRegister:
                {
                  RDI_LocationRegister loc = {0};
                  loc.kind = location->kind;
                  loc.register_code = location->register_code;
                  rdim_str8_list_push(scratch.arena, &location_data, rdim_str8_copy(scratch.arena, rdim_str8_struct(&loc)));
                }break;
              }
            }
          }
          
          rdim_assert(location_block_ptr - location_blocks == location_opl);
        }
      }
      
      rdim_assert(local_ptr - locals == scope_local_first + scope_local_count);
      
      // emit scope
      scope_ptr->proc_idx = (node->symbol == 0)?0:node->symbol->idx;
      scope_ptr->parent_scope_idx = (node->parent_scope == 0)?0:node->parent_scope->idx;
      scope_ptr->first_child_scope_idx = (node->first_child == 0)?0:node->first_child->idx;
      scope_ptr->next_sibling_scope_idx = (node->next_sibling == 0)?0:node->next_sibling->idx;
      scope_ptr->voff_range_first = voff_first;
      scope_ptr->voff_range_opl = voff_opl;
      scope_ptr->local_first = scope_local_first;
      scope_ptr->local_count = scope_local_count;
      
      // TODO(allen): 
      //scope_ptr->static_local_idx_run_first = ;
      //scope_ptr->static_local_count = ;
    }
    
    rdim_assert(scope_ptr - scopes == scope_count);
    rdim_assert(local_ptr - locals == local_count);
  }
  
  // flatten location data
  RDIM_String8 location_data_str = rdim_str8_list_join(arena, &location_data, rdim_str8_lit(""));
  
  // scope vmap
  RDIM_VMap *scope_vmap = 0;
  {
    // count necessary markers
    RDI_U32 marker_count = scope_voff_count;
    
    // fill markers
    RDIM_SortKey    *keys = rdim_push_array_no_zero(scratch.arena, RDIM_SortKey, marker_count);
    RDIM_VMapMarker *markers = rdim_push_array_no_zero(scratch.arena, RDIM_VMapMarker, marker_count);
    
    RDIM_SortKey *key_ptr = keys;
    RDIM_VMapMarker *marker_ptr = markers;
    
    for(RDIM_Scope *node = root->first_scope;
        node != 0;
        node = node->next_order){
      RDI_U32 scope_idx = node->idx;
      
      for(RDIM_VOffRange *range = node->first_range;
          range != 0;
          range = range->next){
        key_ptr->key = range->voff_first;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = scope_idx;
        marker_ptr->begin_range = 1;
        key_ptr += 1;
        marker_ptr += 1;
        
        key_ptr->key = range->voff_opl;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = scope_idx;
        marker_ptr->begin_range = 0;
        key_ptr += 1;
        marker_ptr += 1;
      }
    }
    
    scope_vmap = rdim_vmap_from_markers(arena, markers, keys, marker_count);
  }
  
  // fill result
  RDIM_SymbolData *result = rdim_push_array(arena, RDIM_SymbolData, 1);
  result->global_variables = global_variables;
  result->global_variable_count = globalvar_count;
  result->global_vmap = global_vmap;
  result->thread_variables = thread_variables;
  result->thread_variable_count = threadvar_count;
  result->procedures = procedures;
  result->procedure_count = procedure_count;
  result->scopes = scopes;
  result->scope_count = scope_count;
  result->scope_voffs = scope_voffs;
  result->scope_voff_count = scope_voff_count;
  result->scope_vmap = scope_vmap;
  result->locals = locals;
  result->local_count = local_count;
  result->location_blocks = location_blocks;
  result->location_block_count = location_block_count;
  result->location_data = location_data_str.str;
  result->location_data_size = location_data_str.size;
  
  rdim_scratch_end(scratch);
  return result;
}

//- rjf: name map baking

RDI_PROC RDIM_NameMapBaked*
rdim_name_map_bake(RDIM_Arena *arena, RDIM_Root *root, RDIM_BakeCtx *bctx, RDIM_NameMap *map)
{
  RDIM_Temp scratch = rdim_scratch_begin(&arena, 1);
  
  RDI_U32 bucket_count = map->name_count;
  RDI_U32 node_count = map->name_count;
  
  // setup the final bucket layouts
  RDIM_NameMapSemiBucket *sbuckets = rdim_push_array(scratch.arena, RDIM_NameMapSemiBucket, bucket_count);
  for(RDIM_NameMapNode *node = map->first;
      node != 0;
      node = node->order_next){
    RDI_U64 hash = rdi_hash(node->string.str, node->string.size);
    RDI_U64 bi = hash%bucket_count;
    RDIM_NameMapSemiNode *snode = rdim_push_array(scratch.arena, RDIM_NameMapSemiNode, 1);
    SLLQueuePush(sbuckets[bi].first, sbuckets[bi].last, snode);
    snode->node = node;
    sbuckets[bi].count += 1;
  }
  
  // allocate tables
  RDI_NameMapBucket *buckets = rdim_push_array(arena, RDI_NameMapBucket, bucket_count);
  RDI_NameMapNode *nodes = rdim_push_array_no_zero(arena, RDI_NameMapNode, node_count);
  
  // convert to serialized buckets & nodes
  {
    RDI_NameMapBucket *bucket_ptr = buckets;
    RDI_NameMapNode *node_ptr = nodes;
    for(RDI_U32 i = 0; i < bucket_count; i += 1, bucket_ptr += 1){
      bucket_ptr->first_node = (RDI_U32)(node_ptr - nodes);
      bucket_ptr->node_count = sbuckets[i].count;
      
      for(RDIM_NameMapSemiNode *snode = sbuckets[i].first;
          snode != 0;
          snode = snode->next){
        RDIM_NameMapNode *node = snode->node;
        
        // cons name and index(es)
        RDI_U32 string_idx = rdim_string(bctx, node->string);
        RDI_U32 match_count = node->idx_count;
        RDI_U32 idx = 0;
        if(match_count == 1){
          idx = node->idx_first->idx[0];
        }
        else{
          RDI_U64 temp_pos = rdim_arena_pos(scratch.arena);
          RDI_U32 *idx_run = rdim_push_array_no_zero(scratch.arena, RDI_U32, match_count);
          RDI_U32 *idx_ptr = idx_run;
          for(RDIM_NameMapIdxNode *idxnode = node->idx_first;
              idxnode != 0;
              idxnode = idxnode->next){
            for(RDI_U32 i = 0; i < ArrayCount(idxnode->idx); i += 1){
              if(idxnode->idx[i] == 0){
                goto dblbreak;
              }
              *idx_ptr = idxnode->idx[i];
              idx_ptr += 1;
            }
          }
          dblbreak:;
          rdim_assert(idx_ptr == idx_run + match_count);
          idx = rdim_idx_run(bctx, idx_run, match_count);
          rdim_arena_pop_to(scratch.arena, temp_pos);
        }
        
        // write to node
        node_ptr->string_idx = string_idx;
        node_ptr->match_count = match_count;
        node_ptr->match_idx_or_idx_run_first = idx;
        node_ptr += 1;
      }
    }
    rdim_assert(node_ptr - nodes == node_count);
  }
  
  rdim_scratch_end(scratch);
  
  RDIM_NameMapBaked *result = rdim_push_array(arena, RDIM_NameMapBaked, 1);
  result->buckets = buckets;
  result->nodes = nodes;
  result->bucket_count = bucket_count;
  result->node_count = node_count;
  return result;
}

//- rjf: top-level baking entry point

RDI_PROC void
rdim_bake_file(RDIM_Arena *arena, RDIM_Root *root, RDIM_String8List *out)
{
  str8_serial_begin(arena, out);
  
  // setup cons helpers
  RDIM_DSections dss = {0};
  rdim_dsection(arena, &dss, 0, 0, RDI_DataSectionTag_NULL);
  
  RDIM_BakeParams bctx_params = {0};
  {
    bctx_params.strings_bucket_count  = u64_up_to_pow2(root->symbol_map.pair_count*8);
    bctx_params.idx_runs_bucket_count = u64_up_to_pow2(root->symbol_map.pair_count*8);
  }
  RDIM_BakeCtx *bctx = rdim_bake_ctx_begin(&bctx_params);
  
  ////////////////////////////////
  // MAIN PART: allocating and filling out sections of the file
  
  // top level info
  RDI_TopLevelInfo *tli = rdim_push_array(arena, RDI_TopLevelInfo, 1);
  {
    RDIM_TopLevelInfo *rdim_tli = &root->top_level_info;
    tli->architecture = rdim_tli->architecture;
    tli->exe_name_string_idx = rdim_string(bctx, rdim_tli->exe_name);
    tli->exe_hash = rdim_tli->exe_hash;
    tli->voff_max = rdim_tli->voff_max;
  }
  rdim_dsection(arena, &dss, tli, sizeof(*tli), RDI_DataSectionTag_TopLevelInfo);
  
  // binary sections array
  {
    RDI_U32 count = root->binary_section_count;
    RDI_BinarySection *sections = rdim_push_array(arena, RDI_BinarySection, count);
    RDI_BinarySection *dsec = sections;
    for(RDIM_BinarySection *ssec = root->binary_section_first;
        ssec != 0;
        ssec = ssec->next, dsec += 1){
      dsec->name_string_idx = rdim_string(bctx, ssec->name);
      dsec->flags      = ssec->flags;
      dsec->voff_first = ssec->voff_first;
      dsec->voff_opl   = ssec->voff_opl;
      dsec->foff_first = ssec->foff_first;
      dsec->foff_opl   = ssec->foff_opl;
    }
    rdim_dsection(arena, &dss, sections, sizeof(*sections)*count, RDI_DataSectionTag_BinarySections);
  }
  
  // units array
  // * pass for per-unit information including:
  // * top-level unit information
  // * combining line info for whole unit
  {
    RDI_U32 count = root->unit_count;
    RDI_Unit *units = rdim_push_array(arena, RDI_Unit, count);
    RDI_Unit *dunit = units;
    for(RDIM_Unit *sunit = root->unit_first;
        sunit != 0;
        sunit = sunit->next_order, dunit += 1){
      // strings & paths
      RDI_U32 unit_name = rdim_string(bctx, sunit->unit_name);
      RDI_U32 cmp_name  = rdim_string(bctx, sunit->compiler_name);
      
      RDI_U32 src_path     = rdim_paths_idx_from_path(bctx, sunit->source_file);
      RDI_U32 obj_path     = rdim_paths_idx_from_path(bctx, sunit->object_file);
      RDI_U32 archive_path = rdim_paths_idx_from_path(bctx, sunit->archive_file);
      RDI_U32 build_path   = rdim_paths_idx_from_path(bctx, sunit->build_path);
      
      dunit->unit_name_string_idx     = unit_name;
      dunit->compiler_name_string_idx = cmp_name;
      dunit->source_file_path_node    = src_path;
      dunit->object_file_path_node    = obj_path;
      dunit->archive_file_path_node   = archive_path;
      dunit->build_path_node          = build_path;
      dunit->language                 = sunit->language;
      
      // line info (voff -> file*line*col)
      RDIM_LineSequenceNode *first_seq = sunit->line_seq_first;
      RDIM_UnitLinesCombined *lines = rdim_unit_combine_lines(arena, bctx, first_seq);
      
      RDI_U32 line_count = lines->line_count;
      if(line_count > 0){
        dunit->line_info_voffs_data_idx =
          rdim_dsection(arena, &dss, lines->voffs, sizeof(RDI_U64)*(line_count + 1),
                        RDI_DataSectionTag_LineInfoVoffs);
        dunit->line_info_data_idx =
          rdim_dsection(arena, &dss, lines->lines, sizeof(RDI_Line)*line_count,
                        RDI_DataSectionTag_LineInfoData);
        if(lines->cols != 0){
          dunit->line_info_col_data_idx =
            rdim_dsection(arena, &dss, lines->cols, sizeof(RDI_Column)*line_count,
                          RDI_DataSectionTag_LineInfoColumns);
        }
        dunit->line_info_count = line_count;
      }
    }
    
    rdim_dsection(arena, &dss, units, sizeof(*units)*count, RDI_DataSectionTag_Units);
  }
  
  // source file line info baking
  // * pass for "source_combine_line" for each source file -
  // * can only be run after a pass that does "unit_combine_lines" for each unit.
  for(RDIM_SrcNode *src_node = bctx->tree->src_first;
      src_node != 0;
      src_node = src_node->next){
    RDIM_LineMapFragment *first_fragment = src_node->first_fragment;
    RDIM_SrcLinesCombined *lines = rdim_source_combine_lines(arena, first_fragment);
    RDI_U32 line_count = lines->line_count;
    
    if(line_count > 0){
      src_node->line_map_count = line_count;
      
      src_node->line_map_nums_data_idx =
        rdim_dsection(arena, &dss, lines->line_nums, sizeof(*lines->line_nums)*line_count,
                      RDI_DataSectionTag_LineMapNumbers);
      
      src_node->line_map_range_data_idx =
        rdim_dsection(arena, &dss, lines->line_ranges, sizeof(*lines->line_ranges)*(line_count + 1),
                      RDI_DataSectionTag_LineMapRanges);
      
      src_node->line_map_voff_data_idx =
        rdim_dsection(arena, &dss, lines->voffs, sizeof(*lines->voffs)*lines->voff_count,
                      RDI_DataSectionTag_LineMapVoffs);
    }
  }
  
  // source file name mapping
  {
    RDIM_NameMap* map = rdim_name_map_for_kind(root, RDI_NameMapKind_NormalSourcePaths);
    for(RDIM_SrcNode *src_node = bctx->tree->src_first;
        src_node != 0;
        src_node = src_node->next){
      if(src_node->idx != 0){
        rdim_name_map_add_pair(root, map, src_node->normal_full_path, src_node->idx);
      }
    }
  }
  
  // unit vmap baking
  {
    RDIM_VMap *vmap = rdim_vmap_from_unit_ranges(arena,
                                                 root->unit_vmap_range_first,
                                                 root->unit_vmap_range_count);
    
    RDI_U64 vmap_size = sizeof(*vmap->vmap)*(vmap->count + 1);
    rdim_dsection(arena, &dss, vmap->vmap, vmap_size, RDI_DataSectionTag_UnitVmap);
  }
  
  // type info baking
  {
    RDIM_TypeData *types = rdim_type_data_combine(arena, root, bctx);
    
    RDI_U64 type_nodes_size = sizeof(*types->type_nodes)*types->type_node_count;
    rdim_dsection(arena, &dss, types->type_nodes, type_nodes_size, RDI_DataSectionTag_TypeNodes);
    
    RDI_U64 udt_size = sizeof(*types->udts)*types->udt_count;
    rdim_dsection(arena, &dss, types->udts, udt_size, RDI_DataSectionTag_UDTs);
    
    RDI_U64 member_size = sizeof(*types->members)*types->member_count;
    rdim_dsection(arena, &dss, types->members, member_size, RDI_DataSectionTag_Members);
    
    RDI_U64 enum_member_size = sizeof(*types->enum_members)*types->enum_member_count;
    rdim_dsection(arena, &dss, types->enum_members, enum_member_size, RDI_DataSectionTag_EnumMembers);
  }
  
  // symbol info baking
  {
    RDIM_SymbolData *symbol_data = rdim_symbol_data_combine(arena, root, bctx);
    
    RDI_U64 global_variables_size =
      sizeof(*symbol_data->global_variables)*symbol_data->global_variable_count;
    rdim_dsection(arena, &dss, symbol_data->global_variables, global_variables_size,
                  RDI_DataSectionTag_GlobalVariables);
    
    RDIM_VMap *global_vmap = symbol_data->global_vmap;
    RDI_U64 global_vmap_size = sizeof(*global_vmap->vmap)*(global_vmap->count + 1);
    rdim_dsection(arena, &dss, global_vmap->vmap, global_vmap_size,
                  RDI_DataSectionTag_GlobalVmap);
    
    RDI_U64 thread_variables_size =
      sizeof(*symbol_data->thread_variables)*symbol_data->thread_variable_count;
    rdim_dsection(arena, &dss, symbol_data->thread_variables, thread_variables_size,
                  RDI_DataSectionTag_ThreadVariables);
    
    RDI_U64 procedures_size = sizeof(*symbol_data->procedures)*symbol_data->procedure_count;
    rdim_dsection(arena, &dss, symbol_data->procedures, procedures_size,
                  RDI_DataSectionTag_Procedures);
    
    RDI_U64 scopes_size = sizeof(*symbol_data->scopes)*symbol_data->scope_count;
    rdim_dsection(arena, &dss, symbol_data->scopes, scopes_size, RDI_DataSectionTag_Scopes);
    
    RDI_U64 scope_voffs_size = sizeof(*symbol_data->scope_voffs)*symbol_data->scope_voff_count;
    rdim_dsection(arena, &dss, symbol_data->scope_voffs, scope_voffs_size,
                  RDI_DataSectionTag_ScopeVoffData);
    
    RDIM_VMap *scope_vmap = symbol_data->scope_vmap;
    RDI_U64 scope_vmap_size = sizeof(*scope_vmap->vmap)*(scope_vmap->count + 1);
    rdim_dsection(arena, &dss, scope_vmap->vmap, scope_vmap_size, RDI_DataSectionTag_ScopeVmap);
    
    RDI_U64 local_size = sizeof(*symbol_data->locals)*symbol_data->local_count;
    rdim_dsection(arena, &dss, symbol_data->locals, local_size, RDI_DataSectionTag_Locals);
    
    RDI_U64 location_blocks_size =
      sizeof(*symbol_data->location_blocks)*symbol_data->location_block_count;
    rdim_dsection(arena, &dss, symbol_data->location_blocks, location_blocks_size,
                  RDI_DataSectionTag_LocationBlocks);
    
    RDI_U64 location_data_size = symbol_data->location_data_size;
    rdim_dsection(arena, &dss, symbol_data->location_data, location_data_size,
                  RDI_DataSectionTag_LocationData);
  }
  
  // name map baking
  {
    RDI_U32 name_map_count = 0;
    for(RDI_U32 i = 0; i < RDI_NameMapKind_COUNT; i += 1){
      if(root->name_maps[i] != 0){
        name_map_count += 1;
      }
    }
    
    RDI_NameMap *name_maps = rdim_push_array(arena, RDI_NameMap, name_map_count);
    
    RDI_NameMap *name_map_ptr = name_maps;
    for(RDI_U32 i = 0; i < RDI_NameMapKind_COUNT; i += 1){
      RDIM_NameMap *map = root->name_maps[i];
      if(map != 0){
        RDIM_NameMapBaked *baked = rdim_name_map_bake(arena, root, bctx, map);
        
        name_map_ptr->kind = i;
        name_map_ptr->bucket_data_idx =
          rdim_dsection(arena, &dss, baked->buckets, sizeof(*baked->buckets)*baked->bucket_count,
                        RDI_DataSectionTag_NameMapBuckets);
        name_map_ptr->node_data_idx =
          rdim_dsection(arena, &dss, baked->nodes, sizeof(*baked->nodes)*baked->node_count,
                        RDI_DataSectionTag_NameMapNodes);
        name_map_ptr += 1;
      }
    }
    
    rdim_dsection(arena, &dss, name_maps, sizeof(*name_maps)*name_map_count,
                  RDI_DataSectionTag_NameMaps);
  }
  
  ////////////////////////////////
  // LATE PART: baking loose structures and creating final layout
  
  // generate data sections for file paths
  {
    RDI_U32 count = bctx->tree->count;
    RDI_FilePathNode *nodes = rdim_push_array(arena, RDI_FilePathNode, count);
    
    RDI_FilePathNode *out_node = nodes;
    for(RDIM_PathNode *node = bctx->tree->first;
        node != 0;
        node = node->next_order, out_node += 1){
      out_node->name_string_idx = rdim_string(bctx, node->name);
      if(node->parent != 0){
        out_node->parent_path_node = node->parent->idx;
      }
      if(node->first_child != 0){
        out_node->first_child = node->first_child->idx;
      }
      if(node->next_sibling != 0){
        out_node->next_sibling = node->next_sibling->idx;
      }
      if(node->src_file != 0){
        out_node->source_file_idx = node->src_file->idx;
      }
    }
    
    rdim_dsection(arena, &dss, nodes, sizeof(*nodes)*count, RDI_DataSectionTag_FilePathNodes);
  }
  
  // generate data sections for files
  {
    RDI_U32 count = bctx->tree->src_count;
    RDI_SourceFile *src_files = rdim_push_array(arena, RDI_SourceFile, count);
    
    RDI_SourceFile *out_src_file = src_files;
    for(RDIM_SrcNode *node = bctx->tree->src_first;
        node != 0;
        node = node->next, out_src_file += 1){
      out_src_file->file_path_node_idx = node->path_node->idx;
      out_src_file->normal_full_path_string_idx = rdim_string(bctx, node->normal_full_path);
      out_src_file->line_map_nums_data_idx = node->line_map_nums_data_idx;
      out_src_file->line_map_range_data_idx = node->line_map_range_data_idx;
      out_src_file->line_map_count = node->line_map_count;
      out_src_file->line_map_voff_data_idx = node->line_map_voff_data_idx;
    }
    
    rdim_dsection(arena, &dss, src_files, sizeof(*src_files)*count, RDI_DataSectionTag_SourceFiles);
  }
  
  // generate data sections for strings
  {
    RDI_U32 *str_offs = rdim_push_array_no_zero(arena, RDI_U32, bctx->strs.count + 1);
    
    RDI_U32 off_cursor = 0;
    {
      RDI_U32 *off_ptr = str_offs;
      *off_ptr = 0;
      off_ptr += 1;
      for(RDIM_StringNode *node = bctx->strs.order_first;
          node != 0;
          node = node->order_next){
        off_cursor += node->str.size;
        *off_ptr = off_cursor;
        off_ptr += 1;
      }
    }
    
    RDI_U8 *buf = rdim_push_array(arena, RDI_U8, off_cursor);
    {
      RDI_U8 *ptr = buf;
      for(RDIM_StringNode *node = bctx->strs.order_first;
          node != 0;
          node = node->order_next){
        rdim_memcpy(ptr, node->str.str, node->str.size);
        ptr += node->str.size;
      }
    }
    
    rdim_dsection(arena, &dss, str_offs, sizeof(*str_offs)*(bctx->strs.count + 1),
                  RDI_DataSectionTag_StringTable);
    rdim_dsection(arena, &dss, buf, off_cursor, RDI_DataSectionTag_StringData);
  }
  
  // generate data sections for index runs
  {
    RDI_U32 *idx_data = rdim_push_array_no_zero(arena, RDI_U32, bctx->idxs.idx_count);
    
    {
      RDI_U32 *out_ptr = idx_data;
      RDI_U32 *opl = out_ptr + bctx->idxs.idx_count;
      RDIM_IdxRunNode *node = bctx->idxs.order_first;
      for(;node != 0 && out_ptr < opl;
          node = node->order_next){
        rdim_memcpy(out_ptr, node->idx_run, sizeof(*node->idx_run)*node->count);
        out_ptr += node->count;
      }
      rdim_assert(out_ptr == opl);
    }
    
    rdim_dsection(arena, &dss, idx_data, sizeof(*idx_data)*bctx->idxs.idx_count,
                  RDI_DataSectionTag_IndexRuns);
  }
  
  // layout
  // * the header and data section table have to be initialized "out of order"
  // * so that the rest of the system can avoid this tricky order-layout interdependence stuff
  RDI_Header *header = rdim_push_array(arena, RDI_Header, 1);
  RDI_DataSection *dstable = rdim_push_array(arena, RDI_DataSection, dss.count);
  str8_serial_push_align(arena, out, 8);
  RDI_U64 header_off = out->total_size;
  str8_list_push(arena, out, str8_struct(header));
  str8_serial_push_align(arena, out, 8);
  RDI_U64 data_section_off = out->total_size;
  str8_list_push(arena, out, str8((RDI_U8 *)dstable, sizeof(*dstable)*dss.count));
  {
    header->magic = RDI_MAGIC_CONSTANT;
    header->encoding_version = RDI_ENCODING_VERSION;
    header->data_section_off = data_section_off;
    header->data_section_count = dss.count;
  }
  {
    RDI_U64 test_dss_count = 0;
    for(RDIM_DSectionNode *node = dss.first;
        node != 0;
        node = node->next){
      test_dss_count += 1;
    }
    rdim_assert(test_dss_count == dss.count);
    
    RDI_DataSection *ptr = dstable;
    for(RDIM_DSectionNode *node = dss.first;
        node != 0;
        node = node->next, ptr += 1){
      RDI_U64 data_section_offset = 0;
      if(node->size != 0)
      {
        str8_serial_push_align(arena, out, 8);
        data_section_offset = out->total_size;
        str8_list_push(arena, out, str8((RDI_U8 *)node->data, node->size));
      }
      ptr->tag = node->tag;
      ptr->encoding = RDI_DataSectionEncoding_Unpacked;
      ptr->off = data_section_offset;
      ptr->encoded_size = node->size;
      ptr->unpacked_size = node->size;
    }
    rdim_assert(ptr == dstable + dss.count);
  }
  
  rdim_bake_ctx_release(bctx);
}
#endif
