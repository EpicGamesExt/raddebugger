// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: API Implementation Helper Macros

#define raddbgic_require(root, b32, else_code, error_msg)  do { if(!(b32)) {raddbgic_push_error((root), (error_msg)); else_code;} }while(0)
#define raddbgic_requiref(root, b32, else_code, fmt, ...)  do { if(!(b32)) {raddbgic_push_errorf((root), (fmt), __VA_ARGS__); else_code;} }while(0)

////////////////////////////////
//~ rjf: Basic Helpers

//- rjf: memory set

#if !defined(RADDBGIC_MEMSET_OVERRIDE)
RADDBGI_PROC void *
raddbgic_memset_fallback(void *dst, RADDBGI_U8 c, RADDBGI_U64 size)
{
  for(RADDBGI_U64 idx = 0; idx < size; idx += 1)
  {
    ((RADDBGI_U8 *)dst)[idx] = c;
  }
  return dst;
}
#endif

#if !defined(RADDBGIC_MEMCPY_OVERRIDE)
RADDBGI_PROC void *
raddbgic_memcpy_fallback(void *dst, void *src, RADDBGI_U64 size)
{
  for(RADDBGI_U64 idx = 0; idx < size; idx += 1)
  {
    ((RADDBGI_U8 *)dst)[idx] = ((RADDBGI_U8 *)src)[idx];
  }
  return dst;
}
#endif

//- rjf: arenas

#if !defined (RADDBGIC_ARENA_OVERRIDE)

RADDBGI_PROC RADDBGIC_Arena *
raddbgic_arena_alloc_fallback(void)
{
  RADDBGIC_Arena *arena = 0;
  return arena;
}

RADDBGI_PROC void
raddbgic_arena_release_fallback(RADDBGIC_Arena *arena)
{
  
}

RADDBGI_PROC RADDBGI_U64
raddbgic_arena_pos_fallback(RADDBGIC_Arena *arena)
{
  return 0;
}

RADDBGI_PROC void *
raddbgic_arena_push_fallback(RADDBGIC_Arena *arena, RADDBGI_U64 size)
{
  return 0;
}

RADDBGI_PROC void
raddbgic_arena_pop_to_fallback(RADDBGIC_Arena *arena, RADDBGI_U64 pos)
{
  
}

#endif

//- rjf: thread-local scratch arenas

#if !defined (RADDBGIC_SCRATCH_OVERRIDE)
static RADDBGIC_THREAD_LOCAL RADDBGIC_Arena *raddbgic_thread_scratches[2];

RADDBGI_PROC RADDBGIC_Temp
raddbgic_scratch_begin_fallback(RADDBGIC_Arena **conflicts, RADDBGI_U64 conflicts_count)
{
  if(raddbgic_thread_scratches[0] == 0)
  {
    raddbgic_thread_scratches[0] = raddbgic_arena_alloc();
    raddbgic_thread_scratches[1] = raddbgic_arena_alloc();
  }
  RADDBGIC_Arena *arena = 0;
  for(RADDBGI_U64 scratch_idx = 0; 
      scratch_idx < sizeof(raddbgic_thread_scratches)/sizeof(raddbgic_thread_scratches[0]);
      scratch_idx += 1)
  {
    RADDBGI_S32 scratch_conflicts = 0;
    for(RADDBGI_U64 conflict_idx = 0; conflict_idx < conflicts_count; conflict_idx += 1)
    {
      if(conflicts[conflict_idx] == raddbgic_thread_scratches[scratch_idx])
      {
        scratch_conflicts = 1;
        break;
      }
    }
    if(!scratch_conflicts)
    {
      arena = raddbgic_thread_scratches[scratch_idx];
    }
  }
  RADDBGIC_Temp temp;
  temp.arena = arena;
  temp.pos = raddbgic_arena_pos(arena);
  return temp;
}

RADDBGI_PROC void
raddbgic_scratch_end_fallback(RADDBGIC_Temp temp)
{
  raddbgic_arena_pop_to(temp.arena, temp.pos);
}

#endif

//- rjf: strings

RADDBGI_PROC RADDBGIC_String8
raddbgic_str8(RADDBGI_U8 *str, RADDBGI_U64 size)
{
  RADDBGIC_String8 result;
  result.RADDBGIC_String8_BaseMember = str;
  result.RADDBGIC_String8_SizeMember = size;
  return result;
}

RADDBGI_PROC RADDBGIC_String8
raddbgic_str8_copy(RADDBGIC_Arena *arena, RADDBGIC_String8 src)
{
  RADDBGIC_String8 dst;
  dst.RADDBGIC_String8_SizeMember = src.RADDBGIC_String8_SizeMember;
  dst.RADDBGIC_String8_BaseMember = raddbgic_push_array_no_zero(arena, RADDBGI_U8, dst.RADDBGIC_String8_SizeMember+1);
  raddbgic_memcpy(dst.RADDBGIC_String8_BaseMember, src.RADDBGIC_String8_BaseMember, src.RADDBGIC_String8_SizeMember);
  dst.RADDBGIC_String8_BaseMember[dst.RADDBGIC_String8_SizeMember] = 0;
  return dst;
}

//- rjf: type lists

RADDBGI_PROC void
raddbgic_type_list_push(RADDBGIC_Arena *arena, RADDBGIC_TypeList *list, RADDBGIC_Type *type)
{
  RADDBGIC_TypeNode *node = raddbgic_push_array(arena, RADDBGIC_TypeNode, 1);
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
  node->type = type;
}

//- rjf: bytecode lists

RADDBGI_PROC void
raddbgic_bytecode_push_op(RADDBGIC_Arena *arena, RADDBGIC_EvalBytecode *bytecode, RADDBGI_EvalOp op, RADDBGI_U64 p)
{
  RADDBGI_U8 ctrlbits = raddbgi_eval_opcode_ctrlbits[op];
  RADDBGI_U32 p_size = RADDBGI_DECODEN_FROM_CTRLBITS(ctrlbits);
  
  RADDBGIC_EvalBytecodeOp *node = raddbgic_push_array(arena, RADDBGIC_EvalBytecodeOp, 1);
  node->op = op;
  node->p_size = p_size;
  node->p = p;
  
  SLLQueuePush(bytecode->first_op, bytecode->last_op, node);
  bytecode->op_count += 1;
  bytecode->encoded_size += 1 + p_size;
}

RADDBGI_PROC void
raddbgic_bytecode_push_uconst(RADDBGIC_Arena *arena, RADDBGIC_EvalBytecode *bytecode, RADDBGI_U64 x)
{
  if(x <= 0xFF)
  {
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_ConstU8, x);
  }
  else if(x <= 0xFFFF)
  {
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_ConstU16, x);
  }
  else if(x <= 0xFFFFFFFF)
  {
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_ConstU32, x);
  }
  else
  {
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_ConstU64, x);
  }
}

RADDBGI_PROC void
raddbgic_bytecode_push_sconst(RADDBGIC_Arena *arena, RADDBGIC_EvalBytecode *bytecode, RADDBGI_S64 x)
{
  if(-0x80 <= x && x <= 0x7F)
  {
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_ConstU8, (RADDBGI_U64)x);
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_TruncSigned, 8);
  }
  else if(-0x8000 <= x && x <= 0x7FFF)
  {
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_ConstU16, (RADDBGI_U64)x);
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_TruncSigned, 16);
  }
  else if(-0x80000000ll <= x && x <= 0x7FFFFFFFll)
  {
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_ConstU32, (RADDBGI_U64)x);
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_TruncSigned, 32);
  }
  else
  {
    raddbgic_bytecode_push_op(arena, bytecode, RADDBGI_EvalOp_ConstU64, (RADDBGI_U64)x);
  }
}

RADDBGI_PROC void
raddbgic_bytecode_concat_in_place(RADDBGIC_EvalBytecode *left_dst, RADDBGIC_EvalBytecode *right_destroyed)
{
  if(right_destroyed->first_op != 0)
  {
    if(left_dst->first_op == 0)
    {
      MemoryCopyStruct(left_dst, right_destroyed);
    }
    else
    {
      left_dst->last_op = right_destroyed->last_op;
      left_dst->op_count += right_destroyed->op_count;
      left_dst->encoded_size += right_destroyed->encoded_size;
    }
    MemoryZeroStruct(right_destroyed);
  }
}

//- rjf: sortable range sorting

RADDBGI_PROC RADDBGIC_SortKey*
raddbgic_sort_key_array(RADDBGIC_Arena *arena, RADDBGIC_SortKey *keys, RADDBGI_U64 count)
{
  // This sort is designed to take advantage of lots of pre-existing sorted ranges.
  // Most line info is already sorted or close to already sorted.
  // Similarly most vmap data has lots of pre-sorted ranges. etc. etc.
  // Also - this sort should be a "stable" sort. In the use case of sorting vmap
  // ranges, we want to be able to rely on order, so it needs to be preserved here.
  
  ProfBegin("raddbgic_sort_key_array");
  RADDBGIC_Temp scratch = raddbgic_scratch_begin(&arena, 1);
  RADDBGIC_SortKey *result = 0;
  
  if(count <= 1)
  {
    result = keys;
  }
  else
  {
    RADDBGIC_OrderedRange *ranges_first = 0;
    RADDBGIC_OrderedRange *ranges_last = 0;
    RADDBGI_U64 range_count = 0;
    {
      RADDBGI_U64 pos = 0;
      for(;pos < count;)
      {
        // identify ordered range
        RADDBGI_U64 first = pos;
        RADDBGI_U64 opl = pos + 1;
        for(; opl < count && keys[opl - 1].key <= keys[opl].key; opl += 1);
        
        // generate an ordered range node
        RADDBGIC_OrderedRange *new_range = raddbgic_push_array(scratch.arena, RADDBGIC_OrderedRange, 1);
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
      RADDBGIC_SortKey *keys_swap = raddbgic_push_array_no_zero(arena, RADDBGIC_SortKey, count);
      RADDBGIC_SortKey *src = keys;
      RADDBGIC_SortKey *dst = keys_swap;
      RADDBGIC_OrderedRange *src_ranges = ranges_first;
      RADDBGIC_OrderedRange *dst_ranges = 0;
      RADDBGIC_OrderedRange *dst_ranges_last = 0;
      
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
          RADDBGIC_OrderedRange *range1 = src_ranges;
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
            RADDBGI_U64 first = range1->first;
            MemoryCopy(dst + first, src + first, sizeof(*src)*(range1->opl - first));
            SLLQueuePush(dst_ranges, dst_ranges_last, range1);
            break;
          }
          
          // get second range
          RADDBGIC_OrderedRange *range2 = src_ranges;
          SLLStackPop(src_ranges);
          
          Assert(range1->opl == range2->first);
          
          // merge these ranges
          RADDBGI_U64 jd = range1->first;
          RADDBGI_U64 j1 = range1->first;
          RADDBGI_U64 j1_opl = range1->opl;
          RADDBGI_U64 j2 = range2->first;
          RADDBGI_U64 j2_opl = range2->opl;
          for(;;)
          {
            if(src[j1].key <= src[j2].key)
            {
              MemoryCopy(dst + jd, src + j1, sizeof(*src));
              j1 += 1;
              jd += 1;
              if(j1 >= j1_opl)
              {
                break;
              }
            }
            else
            {
              MemoryCopy(dst + jd, src + j2, sizeof(*src));
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
            MemoryCopy(dst + jd, src + j1, sizeof(*src)*(j1_opl - j1));
          }
          else
          {
            MemoryCopy(dst + jd, src + j2, sizeof(*src)*(j2_opl - j2));
          }
          
          // save this as one range
          range1->opl = range2->opl;
          SLLQueuePush(dst_ranges, dst_ranges_last, range1);
        }
        
        // end pass by swapping buffers and range nodes
        {
          RADDBGIC_SortKey *temp = src;
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
  for(RADDBGI_U64 i = 1; i < count; i += 1){
    Assert(result[i - 1].key <= result[i].key);
  }
#endif
  
  scratch_end(scratch);
  ProfEnd();
  
  return result;
}

////////////////////////////////
//~ rjf: Auxiliary Data Structure Functions

//- rjf: u64 -> ptr map

RADDBGI_PROC void
raddbgic_u64toptr_map_init(RADDBGIC_Arena *arena, RADDBGIC_U64ToPtrMap *map, RADDBGI_U64 bucket_count)
{
  Assert(IsPow2OrZero(bucket_count) && bucket_count > 0);
  map->buckets = raddbgic_push_array(arena, RADDBGIC_U64ToPtrNode*, bucket_count);
  map->buckets_count = bucket_count;
}

RADDBGI_PROC void
raddbgic_u64toptr_map_lookup(RADDBGIC_U64ToPtrMap *map, RADDBGI_U64 key, RADDBGI_U64 hash, RADDBGIC_U64ToPtrLookup *lookup_out)
{
  RADDBGI_U64 bucket_idx = hash&(map->buckets_count - 1);
  RADDBGIC_U64ToPtrNode *check_node = map->buckets[bucket_idx];
  for(;check_node != 0; check_node = check_node->next){
    for(RADDBGI_U32 k = 0; k < ArrayCount(check_node->key); k += 1){
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

RADDBGI_PROC void
raddbgic_u64toptr_map_insert(RADDBGIC_Arena *arena, RADDBGIC_U64ToPtrMap *map, RADDBGI_U64 key, RADDBGI_U64 hash, RADDBGIC_U64ToPtrLookup *lookup, void *ptr)
{
  if(lookup->fill_node != 0)
  {
    RADDBGIC_U64ToPtrNode *node = lookup->fill_node;
    RADDBGI_U32 k = lookup->fill_k;
    node->key[k] = key;
    node->ptr[k] = ptr;
  }
  else
  {
    RADDBGI_U64 bucket_idx = hash&(map->buckets_count - 1);
    
    RADDBGIC_U64ToPtrNode *node = raddbgic_push_array(arena, RADDBGIC_U64ToPtrNode, 1);
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

RADDBGI_PROC void
raddbgic_str8toptr_map_init(RADDBGIC_Arena *arena, RADDBGIC_Str8ToPtrMap *map, RADDBGI_U64 bucket_count)
{
  map->buckets_count = bucket_count;
  map->buckets = raddbgic_push_array(arena, RADDBGIC_Str8ToPtrNode*, map->buckets_count);
}

RADDBGI_PROC void*
raddbgic_str8toptr_map_lookup(RADDBGIC_Str8ToPtrMap *map, RADDBGIC_String8 key, RADDBGI_U64 hash)
{
  void *result = 0;
  RADDBGI_U64 bucket_idx = hash%map->buckets_count;
  for(RADDBGIC_Str8ToPtrNode *node = map->buckets[bucket_idx];
      node != 0;
      node = node->next)
  {
    if(node->hash == hash && str8_match(node->key, key, 0))
    {
      result = node->ptr;
      break;
    }
  }
  return result;
}

RADDBGI_PROC void
raddbgic_str8toptr_map_insert(RADDBGIC_Arena *arena, RADDBGIC_Str8ToPtrMap *map, RADDBGIC_String8 key, RADDBGI_U64 hash, void *ptr)
{
  RADDBGI_U64 bucket_idx = hash%map->buckets_count;
  RADDBGIC_Str8ToPtrNode *node = raddbgic_push_array(arena, RADDBGIC_Str8ToPtrNode, 1);
  SLLStackPush(map->buckets[bucket_idx], node);
  
  node->key  = raddbgic_str8_copy(arena, key);
  node->hash = hash;
  node->ptr = ptr;
  map->bucket_collision_count += (node->next != 0);
  map->pair_count += 1;
}

////////////////////////////////
//~ rjf: Loose Debug Info Construction (Anything -> Loose) Functions

//- rjf: root creation

RADDBGI_PROC RADDBGIC_Root*
raddbgic_root_alloc(RADDBGIC_RootParams *params)
{
  RADDBGIC_Arena *arena = raddbgic_arena_alloc();
  RADDBGIC_Root *result = raddbgic_push_array(arena, RADDBGIC_Root, 1);
  result->arena = arena;
  
  // fill in root parameters
  {
    result->addr_size = params->addr_size;
  }
  
  // setup singular types
  {
    result->nil_type = raddbgic_type_new(result);
    result->variadic_type = raddbgic_type_new(result);
    result->variadic_type->kind = RADDBGI_TypeKind_Variadic;
    
    // references to "handled nil type" should be emitted as
    // references to nil - but should not generate error
    // messages when they are detected - they are expected!
    Assert(result->nil_type->idx == result->handled_nil_type.idx);
  }
  
  // setup a null scope
  {
    RADDBGIC_Scope *scope = raddbgic_push_array(result->arena, RADDBGIC_Scope, 1);
    SLLQueuePush_N(result->first_scope, result->last_scope, scope, next_order);
    result->scope_count += 1;
  }
  
  // rjf: setup null UDT
  {
    raddbgic_type_udt_from_any_type(result, result->nil_type);
  }
  
  // initialize maps
  {
#define BKTCOUNT(x) ((x)?(u64_up_to_pow2(x)):(128))
    
    raddbgic_u64toptr_map_init(arena, &result->unit_map, BKTCOUNT(params->bucket_count_units));
    raddbgic_u64toptr_map_init(arena, &result->symbol_map, BKTCOUNT(params->bucket_count_symbols));
    raddbgic_u64toptr_map_init(arena, &result->scope_map, BKTCOUNT(params->bucket_count_scopes));
    raddbgic_u64toptr_map_init(arena, &result->local_map, BKTCOUNT(params->bucket_count_locals));
    raddbgic_u64toptr_map_init(arena, &result->type_from_id_map, BKTCOUNT(params->bucket_count_types));
    raddbgic_str8toptr_map_init(arena, &result->construct_map, BKTCOUNT(params->bucket_count_type_constructs));
    
#undef BKTCOUNT
  }
  
  return result;
}

RADDBGI_PROC void
raddbgic_root_release(RADDBGIC_Root *root)
{
  arena_release(root->arena);
}

//- rjf: error accumulation

RADDBGI_PROC void
raddbgic_push_error(RADDBGIC_Root *root, RADDBGIC_String8 string)
{
  RADDBGIC_Error *error = raddbgic_push_array(root->arena, RADDBGIC_Error, 1);
  SLLQueuePush(root->errors.first, root->errors.last, error);
  root->errors.count += 1;
  error->msg = string;
}

RADDBGI_PROC void
raddbgic_push_errorf(RADDBGIC_Root *root, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  RADDBGIC_String8 str = push_str8fv(root->arena, fmt, args);
  raddbgic_push_error(root, str);
  va_end(args);
}

RADDBGI_PROC RADDBGIC_Error*
raddbgic_first_error_from_root(RADDBGIC_Root *root)
{
  return root->errors.first;
}

//- rjf: top-level info specification

RADDBGI_PROC void
raddbgic_set_top_level_info(RADDBGIC_Root *root, RADDBGIC_TopLevelInfo *tli)
{
  raddbgic_requiref(root, !root->top_level_info_is_set, return, "Top level information set multiple times.");
  MemoryCopyStruct(&root->top_level_info, tli);
  root->top_level_info_is_set = 1;
}

//- rjf: binary section building

RADDBGI_PROC void
raddbgic_add_binary_section(RADDBGIC_Root *root, RADDBGIC_String8 name, RADDBGI_BinarySectionFlags flags, RADDBGI_U64 voff_first, RADDBGI_U64 voff_opl, RADDBGI_U64 foff_first, RADDBGI_U64 foff_opl)
{
  RADDBGIC_BinarySection *sec = raddbgic_push_array(root->arena, RADDBGIC_BinarySection, 1);
  SLLQueuePush(root->binary_section_first, root->binary_section_last, sec);
  root->binary_section_count += 1;
  sec->name  = name;
  sec->flags = flags;
  sec->voff_first = voff_first;
  sec->voff_opl   = voff_opl;
  sec->foff_first = foff_first;
  sec->foff_opl   = foff_opl;
}

//- rjf: unit info building

RADDBGI_PROC RADDBGIC_Unit*
raddbgic_unit_handle_from_user_id(RADDBGIC_Root *root, RADDBGI_U64 unit_user_id, RADDBGI_U64 unit_user_id_hash)
{
  RADDBGIC_U64ToPtrLookup lookup = {0};
  raddbgic_u64toptr_map_lookup(&root->unit_map, unit_user_id, unit_user_id_hash, &lookup);
  RADDBGIC_Unit *result = 0;
  if(lookup.match != 0)
  {
    result = (RADDBGIC_Unit*)lookup.match;
  }
  else
  {
    result = raddbgic_push_array(root->arena, RADDBGIC_Unit, 1);
    result->idx = root->unit_count;
    SLLQueuePush_N(root->unit_first, root->unit_last, result, next_order);
    root->unit_count += 1;
    raddbgic_u64toptr_map_insert(root->arena, &root->unit_map, unit_user_id, unit_user_id, &lookup, result);
  }
  return result;
}

RADDBGI_PROC void
raddbgic_unit_set_info(RADDBGIC_Root *root, RADDBGIC_Unit *unit, RADDBGIC_UnitInfo *info)
{
  raddbgic_requiref(root, !unit->info_is_set, return, "Unit information set multiple times.");
  unit->info_is_set = 1;
  unit->unit_name     = raddbgic_str8_copy(root->arena, info->unit_name);
  unit->compiler_name = raddbgic_str8_copy(root->arena, info->compiler_name);
  unit->source_file   = raddbgic_str8_copy(root->arena, info->source_file);
  unit->object_file   = raddbgic_str8_copy(root->arena, info->object_file);
  unit->archive_file  = raddbgic_str8_copy(root->arena, info->archive_file);
  unit->build_path    = raddbgic_str8_copy(root->arena, info->build_path);
  unit->language = info->language;
}

RADDBGI_PROC void
raddbgic_unit_add_line_sequence(RADDBGIC_Root *root, RADDBGIC_Unit *unit, RADDBGIC_LineSequence *line_sequence)
{
  RADDBGIC_LineSequenceNode *node = raddbgic_push_array(root->arena, RADDBGIC_LineSequenceNode, 1);
  SLLQueuePush(unit->line_seq_first, unit->line_seq_last, node);
  unit->line_seq_count += 1;
  
  node->line_seq.file_name = raddbgic_str8_copy(root->arena, line_sequence->file_name);
  
  node->line_seq.voffs = raddbgic_push_array(root->arena, RADDBGI_U64, line_sequence->line_count + 1);
  MemoryCopy(node->line_seq.voffs, line_sequence->voffs, sizeof(RADDBGI_U64)*(line_sequence->line_count + 1));
  
  node->line_seq.line_nums = raddbgic_push_array(root->arena, RADDBGI_U32, line_sequence->line_count);
  MemoryCopy(node->line_seq.line_nums, line_sequence->line_nums, sizeof(RADDBGI_U32)*line_sequence->line_count);
  
  if(line_sequence->col_nums != 0)
  {
    node->line_seq.col_nums = raddbgic_push_array(root->arena, U16, line_sequence->line_count);
    MemoryCopy(node->line_seq.col_nums, line_sequence->col_nums, sizeof(U16)*line_sequence->line_count);
  }
  
  node->line_seq.line_count = line_sequence->line_count;
}

RADDBGI_PROC void
raddbgic_unit_vmap_add_range(RADDBGIC_Root *root, RADDBGIC_Unit *unit, RADDBGI_U64 first, RADDBGI_U64 opl)
{
  RADDBGIC_UnitVMapRange *node = raddbgic_push_array(root->arena, RADDBGIC_UnitVMapRange, 1);
  SLLQueuePush(root->unit_vmap_range_first, root->unit_vmap_range_last, node);
  root->unit_vmap_range_count += 1;
  node->unit = unit;
  node->first = first;
  node->opl = opl;
}

//- rjf: type info lookups/reservations

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_from_id(RADDBGIC_Root *root, RADDBGI_U64 type_user_id, RADDBGI_U64 type_user_id_hash)
{
  RADDBGIC_U64ToPtrLookup lookup = {0};
  raddbgic_u64toptr_map_lookup(&root->type_from_id_map, type_user_id, type_user_id_hash, &lookup);
  RADDBGIC_Type *result = (RADDBGIC_Type*)lookup.match;
  return result;
}

RADDBGI_PROC RADDBGIC_Reservation*
raddbgic_type_reserve_id(RADDBGIC_Root *root, RADDBGI_U64 type_user_id, RADDBGI_U64 type_user_id_hash)
{
  RADDBGIC_Reservation *result = 0;
  RADDBGIC_U64ToPtrLookup lookup = {0};
  raddbgic_u64toptr_map_lookup(&root->type_from_id_map, type_user_id, type_user_id_hash, &lookup);
  if(lookup.match == 0)
  {
    raddbgic_u64toptr_map_insert(root->arena, &root->type_from_id_map, type_user_id, type_user_id_hash, &lookup, root->nil_type);
    void **slot = &lookup.fill_node->ptr[lookup.fill_k];
    result = (RADDBGIC_Reservation*)slot;
  }
  return result;
}

RADDBGI_PROC void
raddbgic_type_fill_id(RADDBGIC_Root *root, RADDBGIC_Reservation *res, RADDBGIC_Type *type)
{
  if(res != 0 && type != 0)
  {
    *(void**)res = type;
  }
}

//- rjf: nil/singleton types

RADDBGI_PROC B32
raddbgic_type_is_unhandled_nil(RADDBGIC_Root *root, RADDBGIC_Type *type)
{
  B32 result = (type->kind == RADDBGI_TypeKind_NULL && type != &root->handled_nil_type);
  return result;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_handled_nil(RADDBGIC_Root *root)
{
  return &root->handled_nil_type;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_nil(RADDBGIC_Root *root)
{
  return root->nil_type;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_variadic(RADDBGIC_Root *root)
{
  return root->variadic_type;
}

//- rjf: base type info constructors

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_new(RADDBGIC_Root *root)
{
  RADDBGIC_Type *result = raddbgic_push_array(root->arena, RADDBGIC_Type, 1);
  result->idx = root->type_count;
  SLLQueuePush_N(root->first_type, root->last_type, result, next_order);
  root->type_count += 1;
  return result;
}

RADDBGI_PROC RADDBGIC_TypeUDT*
raddbgic_type_udt_from_any_type(RADDBGIC_Root *root, RADDBGIC_Type *type)
{
  if(type->udt == 0)
  {
    RADDBGIC_TypeUDT *new_udt = raddbgic_push_array(root->arena, RADDBGIC_TypeUDT, 1);
    new_udt->idx = root->type_udt_count;
    SLLQueuePush_N(root->first_udt, root->last_udt, new_udt, next_order);
    root->type_udt_count += 1;
    new_udt->self_type = type;
    type->udt = new_udt;
  }
  RADDBGIC_TypeUDT *result = type->udt;
  return result;
}

RADDBGI_PROC RADDBGIC_TypeUDT*
raddbgic_type_udt_from_record_type(RADDBGIC_Root *root, RADDBGIC_Type *type)
{
  raddbgic_requiref(root, (type->kind == RADDBGI_TypeKind_Struct ||
                           type->kind == RADDBGI_TypeKind_Class ||
                           type->kind == RADDBGI_TypeKind_Union),
                    return 0,
                    "Tried to use non-user-defined-type-kind to create user-defined-type.");
  RADDBGIC_TypeUDT *result = 0;
  result = raddbgic_type_udt_from_any_type(root, type);
  return result;
}

//- rjf: basic/operator type construction helpers

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_basic(RADDBGIC_Root *root, RADDBGI_TypeKind type_kind, RADDBGIC_String8 name)
{
  raddbgic_requiref(root, (RADDBGI_TypeKind_FirstBuiltIn <= type_kind && type_kind <= RADDBGI_TypeKind_LastBuiltIn), return root->nil_type, "Non-basic type kind passed to construct basic type.");
  RADDBGIC_Type *result = root->nil_type;
  Temp scratch = scratch_begin(0, 0);
  
  // setup construct buffer
  RADDBGI_U64 buf_size = sizeof(RADDBGIC_TypeConstructKind) + sizeof(type_kind) + name.size;
  RADDBGI_U8 *buf = raddbgic_push_array(scratch.arena, RADDBGI_U8, buf_size);
  {
    RADDBGI_U8 *ptr = buf;
    // "basic"
    *(RADDBGIC_TypeConstructKind*)ptr = RADDBGIC_TypeConstructKind_Basic;
    ptr += sizeof(RADDBGIC_TypeConstructKind);
    // type_kind
    MemoryCopy(ptr, &type_kind, sizeof(type_kind));
    ptr += sizeof(type_kind);
    // name
    MemoryCopy(ptr, name.str, name.size);
    ptr += name.size;
  }
  
  // check for duplicate construct
  RADDBGIC_String8 blob = str8(buf, buf_size);
  RADDBGI_U64 blob_hash = raddbgi_hash(buf, buf_size);
  void *lookup_ptr = raddbgic_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RADDBGIC_Type*)lookup_ptr;
  if(result == 0)
  {
    // calculate size
    RADDBGI_U32 byte_size = raddbgi_size_from_basic_type_kind(type_kind);
    if(byte_size == 0xFFFFFFFF)
    {
      byte_size = root->addr_size;
    }
    
    // setup new node
    result = raddbgic_type_new(root);
    result->kind = type_kind;
    result->name = raddbgic_str8_copy(root->arena, name);
    result->byte_size = byte_size;
    
    // save in construct map
    raddbgic_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
    
    // save in name map
    {
      RADDBGIC_NameMap *map = raddbgic_name_map_for_kind(root, RADDBGI_NameMapKind_Types);
      raddbgic_name_map_add_pair(root, map, result->name, result->idx);
    }
  }
  
  scratch_end(scratch);
  Assert(result != 0);
  return result;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_modifier(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGI_TypeModifierFlags flags)
{
  RADDBGIC_Type *result = root->nil_type;
  Temp scratch = scratch_begin(0, 0);
  
  // setup construct buffer
  RADDBGI_U64 buf_size = sizeof(RADDBGIC_TypeConstructKind) + sizeof(flags) + sizeof(direct_type->idx);
  RADDBGI_U8 *buf = raddbgic_push_array(scratch.arena, RADDBGI_U8, buf_size);
  {
    RADDBGI_U8 *ptr = buf;
    // "modifier"
    *(RADDBGIC_TypeConstructKind*)ptr = RADDBGIC_TypeConstructKind_Modifier;
    ptr += sizeof(RADDBGIC_TypeConstructKind);
    // flags
    MemoryCopy(ptr, &flags, sizeof(flags));
    ptr += sizeof(flags);
    // direct_type->idx
    MemoryCopy(ptr, &direct_type->idx, sizeof(direct_type->idx));
    ptr += sizeof(direct_type->idx);
  }
  
  // check for duplicate construct
  RADDBGIC_String8 blob = str8(buf, buf_size);
  RADDBGI_U64 blob_hash = raddbgi_hash(buf, buf_size);
  void *lookup_ptr = raddbgic_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RADDBGIC_Type*)lookup_ptr;
  if(result == 0){
    
    // setup new node
    result = raddbgic_type_new(root);
    result->kind = RADDBGI_TypeKind_Modifier;
    result->flags = flags;
    result->byte_size = direct_type->byte_size;
    result->direct_type = direct_type;
    
    // save in construct map
    raddbgic_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  scratch_end(scratch);
  Assert(result != 0);
  return result;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_bitfield(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGI_U32 bit_off, RADDBGI_U32 bit_count)
{
  RADDBGIC_Type *result = root->nil_type;
  Temp scratch = scratch_begin(0, 0);
  
  // setup construct buffer
  RADDBGI_U64 buf_size = sizeof(RADDBGIC_TypeConstructKind) + sizeof(direct_type->idx) + sizeof(RADDBGI_U32)*2;
  RADDBGI_U8 *buf = raddbgic_push_array(scratch.arena, RADDBGI_U8, buf_size);
  {
    RADDBGI_U8 *ptr = buf;
    // "bitfield"
    *(RADDBGIC_TypeConstructKind*)ptr = RADDBGIC_TypeConstructKind_Bitfield;
    ptr += sizeof(RADDBGIC_TypeConstructKind);
    // direct_type->idx
    MemoryCopy(ptr, &direct_type->idx, sizeof(direct_type->idx));
    ptr += sizeof(direct_type->idx);
    // bit_off
    MemoryCopy(ptr, &bit_off, sizeof(bit_off));
    ptr += sizeof(bit_off);
    // bit_count
    MemoryCopy(ptr, &bit_count, sizeof(bit_count));
    ptr += sizeof(bit_count);
  }
  
  // check for duplicate construct
  RADDBGIC_String8 blob = str8(buf, buf_size);
  RADDBGI_U64 blob_hash = raddbgi_hash(buf, buf_size);
  void *lookup_ptr = raddbgic_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RADDBGIC_Type*)lookup_ptr;
  if(result == 0)
  {
    // setup new node
    result = raddbgic_type_new(root);
    result->kind = RADDBGI_TypeKind_Bitfield;
    result->byte_size = direct_type->byte_size;
    result->off = bit_off;
    result->count = bit_count;
    result->direct_type = direct_type;
    
    // save in construct map
    raddbgic_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  scratch_end(scratch);
  Assert(result != 0);
  return result;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_pointer(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGI_TypeKind ptr_type_kind)
{
  raddbgic_requiref(root, (ptr_type_kind == RADDBGI_TypeKind_Ptr ||
                           ptr_type_kind == RADDBGI_TypeKind_LRef ||
                           ptr_type_kind == RADDBGI_TypeKind_RRef),
                    return root->nil_type,
                    "Non-pointer type kind used to construct pointer type.");
  RADDBGIC_Type *result = root->nil_type;
  Temp scratch = scratch_begin(0, 0);
  
  // setup construct buffer
  RADDBGI_U64 buf_size = sizeof(RADDBGIC_TypeConstructKind) + sizeof(ptr_type_kind) + sizeof(direct_type->idx);
  RADDBGI_U8 *buf = raddbgic_push_array(scratch.arena, RADDBGI_U8, buf_size);
  {
    RADDBGI_U8 *ptr = buf;
    // "pointer"
    *(RADDBGIC_TypeConstructKind*)ptr = RADDBGIC_TypeConstructKind_Pointer;
    ptr += sizeof(RADDBGIC_TypeConstructKind);
    // type_kind
    MemoryCopy(ptr, &ptr_type_kind, sizeof(ptr_type_kind));
    ptr += sizeof(ptr_type_kind);
    // direct_type->idx
    MemoryCopy(ptr, &direct_type->idx, sizeof(direct_type->idx));
    ptr += sizeof(direct_type->idx);
  }
  
  // check for duplicate construct
  RADDBGIC_String8 blob = str8(buf, buf_size);
  RADDBGI_U64 blob_hash = raddbgi_hash(buf, buf_size);
  void *lookup_ptr = raddbgic_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RADDBGIC_Type*)lookup_ptr;
  if(result == 0)
  {
    // setup new node
    result = raddbgic_type_new(root);
    result->kind = ptr_type_kind;
    result->byte_size = root->addr_size;
    result->direct_type = direct_type;
    
    // save in construct map
    raddbgic_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  scratch_end(scratch);
  Assert(result != 0);
  return result;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_array(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGI_U64 count)
{
  RADDBGIC_Type *result = root->nil_type;
  Temp scratch = scratch_begin(0, 0);
  
  // setup construct buffer
  RADDBGI_U64 buf_size =
    sizeof(RADDBGIC_TypeConstructKind) + sizeof(direct_type->idx) + sizeof(count);
  RADDBGI_U8 *buf = raddbgic_push_array(scratch.arena, RADDBGI_U8, buf_size);
  {
    RADDBGI_U8 *ptr = buf;
    // "array"
    *(RADDBGIC_TypeConstructKind*)ptr = RADDBGIC_TypeConstructKind_Array;
    ptr += sizeof(RADDBGIC_TypeConstructKind);
    // direct_type->idx
    MemoryCopy(ptr, &direct_type->idx, sizeof(direct_type->idx));
    ptr += sizeof(direct_type->idx);
    // count
    MemoryCopy(ptr, &count, sizeof(count));
    ptr += sizeof(count);
  }
  
  // check for duplicate construct
  RADDBGIC_String8 blob = str8(buf, buf_size);
  RADDBGI_U64 blob_hash = raddbgi_hash(buf, buf_size);
  void *lookup_ptr = raddbgic_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RADDBGIC_Type*)lookup_ptr;
  if(result == 0)
  {
    // setup new node
    result = raddbgic_type_new(root);
    result->kind = RADDBGI_TypeKind_Array;
    result->count = count;
    result->direct_type = direct_type;
    result->byte_size = direct_type->byte_size*count;
    
    // save in construct map
    raddbgic_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  scratch_end(scratch);
  Assert(result != 0);
  return result;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_proc(RADDBGIC_Root *root, RADDBGIC_Type *return_type, struct RADDBGIC_TypeList *params)
{
  RADDBGIC_Type *result = root->nil_type;
  Temp scratch = scratch_begin(0, 0);
  
  // setup construct buffer
  RADDBGI_U64 buf_size = sizeof(RADDBGIC_TypeConstructKind) + sizeof(return_type->idx)*(1 + params->count);
  RADDBGI_U8 *buf = raddbgic_push_array(scratch.arena, RADDBGI_U8, buf_size);
  {
    RADDBGI_U8 *ptr = buf;
    // "procedure"
    *(RADDBGIC_TypeConstructKind*)ptr = RADDBGIC_TypeConstructKind_Procedure;
    ptr += sizeof(RADDBGIC_TypeConstructKind);
    // ret_type->idx
    MemoryCopy(ptr, &return_type->idx, sizeof(return_type->idx));
    ptr += sizeof(return_type->idx);
    // (params ...)->idx
    for(RADDBGIC_TypeNode *node = params->first;
        node != 0;
        node = node->next)
    {
      MemoryCopy(ptr, &node->type->idx, sizeof(node->type->idx));
      ptr += sizeof(node->type->idx);
    }
  }
  
  // check for duplicate construct
  RADDBGIC_String8 blob = str8(buf, buf_size);
  RADDBGI_U64 blob_hash = raddbgi_hash(buf, buf_size);
  void *lookup_ptr = raddbgic_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RADDBGIC_Type*)lookup_ptr;
  if(result == 0)
  {
    // setup param buffer
    RADDBGIC_Type **param_types = raddbgic_push_array(root->arena, RADDBGIC_Type*, params->count);
    {
      RADDBGIC_Type **ptr = param_types;
      for(RADDBGIC_TypeNode *node = params->first;
          node != 0;
          node = node->next)
      {
        *ptr = node->type;
        ptr += 1;
      }
    }
    
    // setup new node
    result = raddbgic_type_new(root);
    result->kind = RADDBGI_TypeKind_Function;
    result->byte_size = root->addr_size;
    result->count = params->count;
    result->direct_type = return_type;
    result->param_types = param_types;
    
    // save in construct map
    raddbgic_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  scratch_end(scratch);
  Assert(result != 0);
  return result;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_method(RADDBGIC_Root *root, RADDBGIC_Type *this_type, RADDBGIC_Type *return_type, struct RADDBGIC_TypeList *params)
{
  RADDBGIC_Type *result = root->nil_type;
  Temp scratch = scratch_begin(0, 0);
  
  // setup construct buffer
  RADDBGI_U64 buf_size =
    sizeof(RADDBGIC_TypeConstructKind) + sizeof(return_type->idx)*(2 + params->count);
  RADDBGI_U8 *buf = raddbgic_push_array(scratch.arena, RADDBGI_U8, buf_size);
  {
    RADDBGI_U8 *ptr = buf;
    // "method"
    *(RADDBGIC_TypeConstructKind*)ptr = RADDBGIC_TypeConstructKind_Method;
    ptr += sizeof(RADDBGIC_TypeConstructKind);
    // ret_type->idx
    MemoryCopy(ptr, &return_type->idx, sizeof(return_type->idx));
    ptr += sizeof(return_type->idx);
    // this_type->idx
    MemoryCopy(ptr, &this_type->idx, sizeof(this_type->idx));
    ptr += sizeof(this_type->idx);
    // (params ...)->idx
    for(RADDBGIC_TypeNode *node = params->first;
        node != 0;
        node = node->next)
    {
      MemoryCopy(ptr, &node->type->idx, sizeof(node->type->idx));
      ptr += sizeof(node->type->idx);
    }
  }
  
  // check for duplicate construct
  RADDBGIC_String8 blob = str8(buf, buf_size);
  RADDBGI_U64 blob_hash = raddbgi_hash(buf, buf_size);
  void *lookup_ptr = raddbgic_str8toptr_map_lookup(&root->construct_map, blob, blob_hash);
  result = (RADDBGIC_Type*)lookup_ptr;
  if(result == 0)
  {
    // setup param buffer
    RADDBGIC_Type **param_types = raddbgic_push_array(root->arena, RADDBGIC_Type*, params->count + 1);
    {
      RADDBGIC_Type **ptr = param_types;
      {
        *ptr = this_type;
        ptr += 1;
      }
      for(RADDBGIC_TypeNode *node = params->first;
          node != 0;
          node = node->next)
      {
        *ptr = node->type;
        ptr += 1;
      }
    }
    
    // setup new node
    result = raddbgic_type_new(root);
    result->kind = RADDBGI_TypeKind_Method;
    result->byte_size = root->addr_size;
    result->count = params->count;
    result->direct_type = return_type;
    result->param_types = param_types;
    
    // save in construct map
    raddbgic_str8toptr_map_insert(root->arena, &root->construct_map, blob, blob_hash, result);
  }
  
  scratch_end(scratch);
  Assert(result != 0);
  return result;
}

//- rjf: udt type constructors

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_udt(RADDBGIC_Root *root, RADDBGI_TypeKind record_type_kind, RADDBGIC_String8 name, RADDBGI_U64 size)
{
  raddbgic_requiref(root, (record_type_kind == RADDBGI_TypeKind_Struct ||
                           record_type_kind == RADDBGI_TypeKind_Class ||
                           record_type_kind == RADDBGI_TypeKind_Union),
                    return root->nil_type,
                    "Non-user-defined-type-kind used to create user-defined type.");
  
  // rjf: make type
  RADDBGIC_Type *result = raddbgic_type_new(root);
  result->kind = record_type_kind;
  result->byte_size = size;
  result->name = raddbgic_str8_copy(root->arena, name);
  
  // rjf: save in name map
  {
    RADDBGIC_NameMap *map = raddbgic_name_map_for_kind(root, RADDBGI_NameMapKind_Types);
    raddbgic_name_map_add_pair(root, map, result->name, result->idx);
  }
  
  return result;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_enum(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGIC_String8 name)
{
  // rjf: make type
  RADDBGIC_Type *result = raddbgic_type_new(root);
  result->kind = RADDBGI_TypeKind_Enum;
  result->byte_size = direct_type->byte_size;
  result->name = raddbgic_str8_copy(root->arena, name);
  result->direct_type = direct_type;
  
  // rjf: save in name map
  {
    RADDBGIC_NameMap *map = raddbgic_name_map_for_kind(root, RADDBGI_NameMapKind_Types);
    raddbgic_name_map_add_pair(root, map, result->name, result->idx);
  }
  
  return result;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_alias(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGIC_String8 name)
{
  // rjf: make type
  RADDBGIC_Type *result = raddbgic_type_new(root);
  result->kind = RADDBGI_TypeKind_Alias;
  result->byte_size = direct_type->byte_size;
  result->name = raddbgic_str8_copy(root->arena, name);
  result->direct_type = direct_type;
  
  // rjf: save in name map
  {
    RADDBGIC_NameMap *map = raddbgic_name_map_for_kind(root, RADDBGI_NameMapKind_Types);
    raddbgic_name_map_add_pair(root, map, result->name, result->idx);
  }
  
  return result;
}

RADDBGI_PROC RADDBGIC_Type*
raddbgic_type_incomplete(RADDBGIC_Root *root, RADDBGI_TypeKind type_kind, RADDBGIC_String8 name)
{
  raddbgic_requiref(root, (type_kind == RADDBGI_TypeKind_IncompleteStruct ||
                           type_kind == RADDBGI_TypeKind_IncompleteClass ||
                           type_kind == RADDBGI_TypeKind_IncompleteUnion ||
                           type_kind == RADDBGI_TypeKind_IncompleteEnum),
                    return root->nil_type,
                    "Non-incomplete-type-kind used to create incomplete type.");
  
  // rjf: make type
  RADDBGIC_Type *result = raddbgic_type_new(root);
  result->kind = type_kind;
  result->name = raddbgic_str8_copy(root->arena, name);
  
  // save in name map
  {
    RADDBGIC_NameMap *map = raddbgic_name_map_for_kind(root, RADDBGI_NameMapKind_Types);
    raddbgic_name_map_add_pair(root, map, result->name, result->idx);
  }
  
  return result;
}

//- rjf: type member building

RADDBGI_PROC void
raddbgic_type_add_member_data_field(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_String8 name, RADDBGIC_Type *mem_type, RADDBGI_U32 off)
{
  RADDBGIC_TypeUDT *udt = raddbgic_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RADDBGIC_TypeMember *member = raddbgic_push_array(root->arena, RADDBGIC_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RADDBGI_MemberKind_DataField;
    member->name = raddbgic_str8_copy(root->arena, name);
    member->type = mem_type;
    member->off = off;
  }
}

RADDBGI_PROC void
raddbgic_type_add_member_static_data(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_String8 name, RADDBGIC_Type *mem_type)
{
  RADDBGIC_TypeUDT *udt = raddbgic_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RADDBGIC_TypeMember *member = raddbgic_push_array(root->arena, RADDBGIC_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RADDBGI_MemberKind_StaticData;
    member->name = raddbgic_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

RADDBGI_PROC void
raddbgic_type_add_member_method(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_String8 name, RADDBGIC_Type *mem_type)
{
  RADDBGIC_TypeUDT *udt = raddbgic_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RADDBGIC_TypeMember *member = raddbgic_push_array(root->arena, RADDBGIC_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RADDBGI_MemberKind_Method;
    member->name = raddbgic_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

RADDBGI_PROC void
raddbgic_type_add_member_static_method(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_String8 name, RADDBGIC_Type *mem_type)
{
  RADDBGIC_TypeUDT *udt = raddbgic_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RADDBGIC_TypeMember *member = raddbgic_push_array(root->arena, RADDBGIC_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    
    root->total_member_count += 1;
    
    member->kind = RADDBGI_MemberKind_StaticMethod;
    member->name = raddbgic_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

RADDBGI_PROC void
raddbgic_type_add_member_virtual_method(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_String8 name, RADDBGIC_Type *mem_type)
{
  RADDBGIC_TypeUDT *udt = raddbgic_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RADDBGIC_TypeMember *member = raddbgic_push_array(root->arena, RADDBGIC_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RADDBGI_MemberKind_VirtualMethod;
    member->name = raddbgic_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

RADDBGI_PROC void
raddbgic_type_add_member_base(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_Type *base_type, RADDBGI_U32 off)
{
  RADDBGIC_TypeUDT *udt = raddbgic_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RADDBGIC_TypeMember *member = raddbgic_push_array(root->arena, RADDBGIC_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RADDBGI_MemberKind_Base;
    member->type = base_type;
    member->off = off;
  }
}

RADDBGI_PROC void
raddbgic_type_add_member_virtual_base(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_Type *base_type, RADDBGI_U32 vptr_off, RADDBGI_U32 vtable_off)
{
  RADDBGIC_TypeUDT *udt = raddbgic_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RADDBGIC_TypeMember *member = raddbgic_push_array(root->arena, RADDBGIC_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RADDBGI_MemberKind_VirtualBase;
    member->type = base_type;
    // TODO(allen): what to do with the two offsets in this case?
  }
}

RADDBGI_PROC void
raddbgic_type_add_member_nested_type(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_Type *nested_type)
{
  RADDBGIC_TypeUDT *udt = raddbgic_type_udt_from_record_type(root, record_type);
  if(udt != 0)
  {
    RADDBGIC_TypeMember *member = raddbgic_push_array(root->arena, RADDBGIC_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    root->total_member_count += 1;
    member->kind = RADDBGI_MemberKind_NestedType;
    member->type = nested_type;
  }
}

RADDBGI_PROC void
raddbgic_type_add_enum_val(RADDBGIC_Root *root, RADDBGIC_Type *enum_type, RADDBGIC_String8 name, RADDBGI_U64 val)
{
  raddbgic_requiref(root, (enum_type->kind == RADDBGI_TypeKind_Enum), return, "Tried to add enum value to non-enum type.");
  RADDBGIC_TypeUDT *udt = raddbgic_type_udt_from_any_type(root, enum_type);
  if(udt != 0)
  {
    RADDBGIC_TypeEnumVal *enum_val = raddbgic_push_array(root->arena, RADDBGIC_TypeEnumVal, 1);
    SLLQueuePush(udt->first_enum_val, udt->last_enum_val, enum_val);
    udt->enum_val_count += 1;
    root->total_enum_val_count += 1;
    enum_val->name = raddbgic_str8_copy(root->arena, name);
    enum_val->val  = val;
  }
}

//- rjf: type source coordinate specifications
RADDBGI_PROC void
raddbgic_type_set_source_coordinates(RADDBGIC_Root *root, RADDBGIC_Type *defined_type, RADDBGIC_String8 source_path, RADDBGI_U32 line, RADDBGI_U32 col)
{
  raddbgic_requiref(root, (RADDBGI_TypeKind_FirstUserDefined <= defined_type->kind && defined_type->kind <= RADDBGI_TypeKind_LastUserDefined),
                    return, "Tried to add source coordinates to non-user-defined type.");
  RADDBGIC_TypeUDT *udt = raddbgic_type_udt_from_any_type(root, defined_type);
  if(udt != 0)
  {
    udt->source_path = raddbgic_str8_copy(root->arena, source_path);
    udt->line = line;
    udt->col = col;
  }
}

//- rjf: symbol info building

RADDBGI_PROC RADDBGIC_Symbol*
raddbgic_symbol_handle_from_user_id(RADDBGIC_Root *root, RADDBGI_U64 symbol_user_id, RADDBGI_U64 symbol_user_id_hash)
{
  RADDBGIC_U64ToPtrLookup lookup = {0};
  raddbgic_u64toptr_map_lookup(&root->symbol_map, symbol_user_id, symbol_user_id_hash, &lookup);
  RADDBGIC_Symbol *result = 0;
  if(lookup.match != 0)
  {
    result = (RADDBGIC_Symbol*)lookup.match;
  }
  else
  {
    result = raddbgic_push_array(root->arena, RADDBGIC_Symbol, 1);
    SLLQueuePush_N(root->first_symbol, root->last_symbol, result, next_order);
    root->symbol_count += 1;
    raddbgic_u64toptr_map_insert(root->arena, &root->symbol_map, symbol_user_id, symbol_user_id_hash, &lookup, result);
  }
  return result;
}

RADDBGI_PROC void
raddbgic_symbol_set_info(RADDBGIC_Root *root, RADDBGIC_Symbol *symbol, RADDBGIC_SymbolInfo *info)
{
  // rjf: unpack
  RADDBGIC_SymbolKind kind = info->kind;
  RADDBGIC_Symbol *container_symbol = info->container_symbol;
  RADDBGIC_Type *container_type = info->container_type;
  
  // rjf: requirements
  raddbgic_requiref(root, RADDBGIC_SymbolKind_NULL == symbol->kind, return, "Symbol information set multiple times.");
  raddbgic_requiref(root, RADDBGIC_SymbolKind_NULL < info->kind && info->kind < RADDBGIC_SymbolKind_COUNT, return, "Invalid symbol kind used to initialize symbol.");
  raddbgic_requiref(root, info->type != 0, return, "Invalid type used to initialize symbol.");
  raddbgic_requiref(root, info->container_symbol == 0 || info->container_type == 0, container_type = 0, "Symbol initialized with both a containing symbol and containing type, when only one is allowed.");
  
  // rjf: fill
  root->symbol_kind_counts[kind] += 1;
  symbol->idx = root->symbol_kind_counts[kind];
  symbol->kind = kind;
  symbol->name = raddbgic_str8_copy(root->arena, info->name);
  symbol->link_name = raddbgic_str8_copy(root->arena, info->link_name);
  symbol->type = info->type;
  symbol->is_extern = info->is_extern;
  symbol->offset = info->offset;
  symbol->container_symbol = container_symbol;
  symbol->container_type = container_type;
  
  // rjf: set root scope
  switch(kind)
  {
    default:{}break;
    case RADDBGIC_SymbolKind_GlobalVariable:
    case RADDBGIC_SymbolKind_ThreadVariable:
    {
      raddbgic_requiref(root, info->root_scope == 0, NoOp, "Global or thread variable initialized with root scope.");
    }break;
    case RADDBGIC_SymbolKind_Procedure:
    {
      raddbgic_requiref(root, info->root_scope != 0, NoOp, "Procedure symbol initialized without root scope.");
      symbol->root_scope = info->root_scope;
      raddbgic_scope_recursive_set_symbol(info->root_scope, symbol);
    }break;
  }
  
  // save name map
  {
    RADDBGIC_NameMap *map = 0;
    switch(kind)
    {
      default:{}break;
      case RADDBGIC_SymbolKind_GlobalVariable:
      {
        map = raddbgic_name_map_for_kind(root, RADDBGI_NameMapKind_GlobalVariables);
      }break;
      case RADDBGIC_SymbolKind_ThreadVariable:
      {
        map = raddbgic_name_map_for_kind(root, RADDBGI_NameMapKind_ThreadVariables);
      }break;
      case RADDBGIC_SymbolKind_Procedure:
      {
        map = raddbgic_name_map_for_kind(root, RADDBGI_NameMapKind_Procedures);
      }break;
    }
    if(map != 0)
    {
      raddbgic_name_map_add_pair(root, map, symbol->name, symbol->idx);
    }
  }
  
  // save link name map
  if(kind == RADDBGIC_SymbolKind_Procedure && symbol->link_name.size > 0)
  {
    RADDBGIC_NameMap *map = raddbgic_name_map_for_kind(root, RADDBGI_NameMapKind_LinkNameProcedures);
    raddbgic_name_map_add_pair(root, map, symbol->link_name, symbol->idx);
  }
}

//- rjf: scope info building

RADDBGI_PROC RADDBGIC_Scope *
raddbgic_scope_handle_from_user_id(RADDBGIC_Root *root, RADDBGI_U64 scope_user_id, RADDBGI_U64 scope_user_id_hash)
{
  RADDBGIC_Scope *result = 0;
  RADDBGIC_U64ToPtrLookup lookup = {0};
  raddbgic_u64toptr_map_lookup(&root->scope_map, scope_user_id, scope_user_id_hash, &lookup);
  if(lookup.match != 0)
  {
    result = (RADDBGIC_Scope*)lookup.match;
  }
  else
  {
    result = raddbgic_push_array(root->arena, RADDBGIC_Scope, 1);
    result->idx = root->scope_count;
    SLLQueuePush_N(root->first_scope, root->last_scope, result, next_order);
    root->scope_count += 1;
    raddbgic_u64toptr_map_insert(root->arena, &root->scope_map, scope_user_id, scope_user_id_hash, &lookup, result);
  }
  return result;
}

RADDBGI_PROC void
raddbgic_scope_set_parent(RADDBGIC_Root *root, RADDBGIC_Scope *scope, RADDBGIC_Scope *parent)
{
  raddbgic_requiref(root, scope->parent_scope == 0, return, "Scope parent set multiple times.");
  raddbgic_requiref(root, parent != 0, return, "Tried to set invalid parent as scope parent.");
  scope->symbol = parent->symbol;
  scope->parent_scope = parent;
  SLLQueuePush_N(parent->first_child, parent->last_child, scope, next_sibling);
}

RADDBGI_PROC void
raddbgic_scope_add_voff_range(RADDBGIC_Root *root, RADDBGIC_Scope *scope, RADDBGI_U64 voff_first, RADDBGI_U64 voff_opl)
{
  RADDBGIC_VOffRange *range = raddbgic_push_array(root->arena, RADDBGIC_VOffRange, 1);
  SLLQueuePush(scope->first_range, scope->last_range, range);
  scope->range_count += 1;
  range->voff_first = voff_first;
  range->voff_opl   = voff_opl;
  scope->voff_base  = Min(scope->voff_base, voff_first);
  root->scope_voff_count += 2;
}

RADDBGI_PROC void
raddbgic_scope_recursive_set_symbol(RADDBGIC_Scope *scope, RADDBGIC_Symbol *symbol)
{
  scope->symbol = symbol;
  for(RADDBGIC_Scope *node = scope->first_child;
      node != 0;
      node = node->next_sibling)
  {
    raddbgic_scope_recursive_set_symbol(node, symbol);
  }
}

//- rjf: local info building

RADDBGI_PROC RADDBGIC_Local*
raddbgic_local_handle_from_user_id(RADDBGIC_Root *root, RADDBGI_U64 local_user_id, RADDBGI_U64 local_user_id_hash)
{
  RADDBGIC_Local *result = 0;
  RADDBGIC_U64ToPtrLookup lookup = {0};
  raddbgic_u64toptr_map_lookup(&root->local_map, local_user_id, local_user_id_hash, &lookup);
  if(lookup.match != 0)
  {
    result = (RADDBGIC_Local*)lookup.match;
  }
  else
  {
    result = raddbgic_push_array(root->arena, RADDBGIC_Local, 1);
    raddbgic_u64toptr_map_insert(root->arena, &root->local_map, local_user_id, local_user_id_hash, &lookup, result);
  }
  return result;
}

RADDBGI_PROC void
raddbgic_local_set_basic_info(RADDBGIC_Root *root, RADDBGIC_Local *local, RADDBGIC_LocalInfo *info)
{
  raddbgic_requiref(root, local->kind == RADDBGI_LocalKind_NULL, return, "Local information set multiple times.");
  raddbgic_requiref(root, info->scope != 0, return, "Tried to set invalid scope as local's containing scope.");
  raddbgic_requiref(root, RADDBGI_LocalKind_NULL < info->kind && info->kind < RADDBGI_LocalKind_COUNT, return, "Invalid local kind.");
  raddbgic_requiref(root, info->type != 0, return, "Tried to set invalid type as local's type.");
  RADDBGIC_Scope *scope = info->scope;
  SLLQueuePush(scope->first_local, scope->last_local, local);
  scope->local_count += 1;
  root->local_count += 1;
  local->kind = info->kind;
  local->name = raddbgic_str8_copy(root->arena, info->name);
  local->type = info->type;
}

RADDBGI_PROC RADDBGIC_LocationSet*
raddbgic_location_set_from_local(RADDBGIC_Root *root, RADDBGIC_Local *local)
{
  RADDBGIC_LocationSet *result = local->locset;
  if(result == 0)
  {
    local->locset = raddbgic_push_array(root->arena, RADDBGIC_LocationSet, 1);
    result = local->locset;
  }
  return result;
}

//- rjf: location info building

RADDBGI_PROC void
raddbgic_location_set_add_case(RADDBGIC_Root *root, RADDBGIC_LocationSet *locset, RADDBGI_U64 voff_first, RADDBGI_U64 voff_opl, RADDBGIC_Location *location)
{
  RADDBGIC_LocationCase *location_case = raddbgic_push_array(root->arena, RADDBGIC_LocationCase, 1);
  SLLQueuePush(locset->first_location_case, locset->last_location_case, location_case);
  locset->location_case_count += 1;
  root->location_count += 1;
  location_case->voff_first = voff_first;
  location_case->voff_opl   = voff_opl;
  location_case->location   = location;
}

RADDBGI_PROC RADDBGIC_Location*
raddbgic_location_addr_bytecode_stream(RADDBGIC_Root *root, struct RADDBGIC_EvalBytecode *bytecode)
{
  RADDBGIC_Location *result = raddbgic_push_array(root->arena, RADDBGIC_Location, 1);
  result->kind = RADDBGI_LocationKind_AddrBytecodeStream;
  result->bytecode = *bytecode;
  return result;
}

RADDBGI_PROC RADDBGIC_Location*
raddbgic_location_val_bytecode_stream(RADDBGIC_Root *root, struct RADDBGIC_EvalBytecode *bytecode)
{
  RADDBGIC_Location *result = raddbgic_push_array(root->arena, RADDBGIC_Location, 1);
  result->kind = RADDBGI_LocationKind_ValBytecodeStream;
  result->bytecode = *bytecode;
  return result;
}

RADDBGI_PROC RADDBGIC_Location*
raddbgic_location_addr_reg_plus_u16(RADDBGIC_Root *root, RADDBGI_U8 reg_code, U16 offset)
{
  RADDBGIC_Location *result = raddbgic_push_array(root->arena, RADDBGIC_Location, 1);
  result->kind = RADDBGI_LocationKind_AddrRegisterPlusU16;
  result->register_code = reg_code;
  result->offset = offset;
  return result;
}

RADDBGI_PROC RADDBGIC_Location*
raddbgic_location_addr_addr_reg_plus_u16(RADDBGIC_Root *root, RADDBGI_U8 reg_code, U16 offset)
{
  RADDBGIC_Location *result = raddbgic_push_array(root->arena, RADDBGIC_Location, 1);
  result->kind = RADDBGI_LocationKind_AddrAddrRegisterPlusU16;
  result->register_code = reg_code;
  result->offset = offset;
  return result;
}

RADDBGI_PROC RADDBGIC_Location*
raddbgic_location_val_reg(RADDBGIC_Root *root, RADDBGI_U8 reg_code)
{
  RADDBGIC_Location *result = raddbgic_push_array(root->arena, RADDBGIC_Location, 1);
  result->kind = RADDBGI_LocationKind_ValRegister;
  result->register_code = reg_code;
  return result;
}

//- rjf: name map building

RADDBGI_PROC RADDBGIC_NameMap*
raddbgic_name_map_for_kind(RADDBGIC_Root *root, RADDBGI_NameMapKind kind)
{
  RADDBGIC_NameMap *result = 0;
  if(kind < RADDBGI_NameMapKind_COUNT)
  {
    if(root->name_maps[kind] == 0)
    {
      root->name_maps[kind] = raddbgic_push_array(root->arena, RADDBGIC_NameMap, 1);
      root->name_maps[kind]->buckets_count = 16384;
      root->name_maps[kind]->buckets = raddbgic_push_array(root->arena, RADDBGIC_NameMapNode *, root->name_maps[kind]->buckets_count);
    }
    result = root->name_maps[kind];
  }
  return result;
}

RADDBGI_PROC void
raddbgic_name_map_add_pair(RADDBGIC_Root *root, RADDBGIC_NameMap *map, RADDBGIC_String8 string, RADDBGI_U32 idx)
{
  // hash
  RADDBGI_U64 hash = raddbgi_hash(string.str, string.size);
  RADDBGI_U64 bucket_idx = hash%map->buckets_count;
  
  // find existing name node
  RADDBGIC_NameMapNode *match = 0;
  for(RADDBGIC_NameMapNode *node = map->buckets[bucket_idx];
      node != 0;
      node = node->bucket_next)
  {
    if(str8_match(string, node->string, 0))
    {
      match = node;
      break;
    }
  }
  
  // make name node if necessary
  if(match == 0)
  {
    match = raddbgic_push_array(root->arena, RADDBGIC_NameMapNode, 1);
    match->string = raddbgic_str8_copy(root->arena, string);
    SLLStackPush_N(map->buckets[bucket_idx], match, bucket_next);
    SLLQueuePush_N(map->first, map->last, match, order_next);
    map->name_count += 1;
    map->bucket_collision_count += (match->bucket_next != 0);
  }
  
  // find existing idx
  B32 existing_idx = 0;
  for(RADDBGIC_NameMapIdxNode *node = match->idx_first;
      node != 0;
      node = node->next)
  {
    for(RADDBGI_U32 i = 0; i < ArrayCount(node->idx); i += 1)
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
    RADDBGIC_NameMapIdxNode *idx_node = match->idx_last;
    RADDBGI_U32 insert_i = match->idx_count%ArrayCount(idx_node->idx);
    if(insert_i == 0)
    {
      idx_node = raddbgic_push_array(root->arena, RADDBGIC_NameMapIdxNode, 1);
      SLLQueuePush(match->idx_first, match->idx_last, idx_node);
    }
    
    idx_node->idx[insert_i] = idx;
    match->idx_count += 1;
  }
}

////////////////////////////////
//~ rjf: Debug Info Baking (Loose -> Tight) Functions

//- rjf: bake context construction

RADDBGI_PROC RADDBGIC_BakeCtx*
raddbgic_bake_ctx_begin(RADDBGIC_BakeParams *params)
{
  Arena *arena = arena_alloc();
  RADDBGIC_BakeCtx *result = raddbgic_push_array(arena, RADDBGIC_BakeCtx, 1);
  result->arena = arena;
#define BKTCOUNT(x) ((x)?(u64_up_to_pow2(x)):(16384))
  result->strs.buckets_count = BKTCOUNT(params->strings_bucket_count);
  result->idxs.buckets_count = BKTCOUNT(params->idx_runs_bucket_count);
#undef BKTCOUNT
  result->strs.buckets = raddbgic_push_array(arena, RADDBGIC_StringNode *, result->strs.buckets_count);
  result->idxs.buckets = raddbgic_push_array(arena, RADDBGIC_IdxRunNode *, result->idxs.buckets_count);
  
  raddbgic_string(result, raddbgic_str8_lit(""));
  raddbgic_idx_run(result, 0, 0);
  
  result->tree = raddbgic_push_array(arena, RADDBGIC_PathTree, 1);
  {
    RADDBGIC_PathNode *nil_path_node = raddbgic_paths_new_node(result);
    nil_path_node->name = str8_lit("<NIL>");
    RADDBGIC_SrcNode *nil_src_node = raddbgic_paths_new_src_node(result);
    nil_src_node->path_node = nil_path_node;
    nil_src_node->normal_full_path = str8_lit("<NIL>");
    nil_path_node->src_file = nil_src_node;
  }
  
  return result;
}

RADDBGI_PROC void
raddbgic_bake_ctx_release(RADDBGIC_BakeCtx *bake_ctx)
{
  arena_release(bake_ctx->arena);
}

//- rjf: string baking

RADDBGI_PROC RADDBGI_U32
raddbgic_string(RADDBGIC_BakeCtx *bctx, RADDBGIC_String8 str)
{
  Arena *arena = bctx->arena;
  RADDBGIC_Strings *strs = &bctx->strs;
  RADDBGI_U64 hash = raddbgi_hash(str.str, str.size);
  RADDBGI_U64 bucket_idx = hash%strs->buckets_count;
  
  // look for a match
  RADDBGIC_StringNode *match = 0;
  for(RADDBGIC_StringNode *node = strs->buckets[bucket_idx];
      node != 0;
      node = node->bucket_next)
  {
    if(node->hash == hash && str8_match(node->str, str, 0))
    {
      match = node;
      break;
    }
  }
  
  // insert new node if no match
  if(match == 0)
  {
    RADDBGIC_StringNode *node = push_array_no_zero(arena, RADDBGIC_StringNode, 1);
    node->str = raddbgic_str8_copy(arena, str);
    node->hash = hash;
    node->idx = strs->count;
    strs->count += 1;
    SLLQueuePush_N(strs->order_first, strs->order_last, node, order_next);
    SLLStackPush_N(strs->buckets[bucket_idx], node, bucket_next);
    match = node;
    strs->bucket_collision_count += (node->bucket_next != 0);
  }
  
  // extract idx to return
  Assert(match != 0);
  RADDBGI_U32 result = match->idx;
  return result;
}

//- rjf: idx run baking

RADDBGI_PROC RADDBGI_U64
raddbgic_idx_run_hash(RADDBGI_U32 *idx_run, RADDBGI_U32 count)
{
  RADDBGI_U64 hash = 5381;
  RADDBGI_U32 *ptr = idx_run;
  RADDBGI_U32 *opl = idx_run + count;
  for(;ptr < opl; ptr += 1)
  {
    hash = ((hash << 5) + hash) + (*ptr);
  }
  return(hash);
}

RADDBGI_PROC RADDBGI_U32
raddbgic_idx_run(RADDBGIC_BakeCtx *bctx, RADDBGI_U32 *idx_run, RADDBGI_U32 count)
{
  Arena *arena = bctx->arena;
  RADDBGIC_IdxRuns *idxs = &bctx->idxs;
  
  RADDBGI_U64 hash = raddbgic_idx_run_hash(idx_run, count);
  RADDBGI_U64 bucket_idx = hash%idxs->buckets_count;
  
  // look for a match
  RADDBGIC_IdxRunNode *match = 0;
  for(RADDBGIC_IdxRunNode *node = idxs->buckets[bucket_idx];
      node != 0;
      node = node->bucket_next)
  {
    if(node->hash == hash)
    {
      S32 is_match = 1;
      RADDBGI_U32 *node_idx = node->idx_run;
      for(RADDBGI_U32 i = 0; i < count; i += 1)
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
    RADDBGIC_IdxRunNode *node = push_array_no_zero(arena, RADDBGIC_IdxRunNode, 1);
    RADDBGI_U32 *idx_run_copy = push_array_no_zero(arena, RADDBGI_U32, count);
    for(RADDBGI_U32 i = 0; i < count; i += 1)
    {
      idx_run_copy[i] = idx_run[i];
    }
    node->idx_run = idx_run_copy;
    node->hash = hash;
    node->count = count;
    node->first_idx = idxs->idx_count;
    idxs->count += 1;
    idxs->idx_count += count;
    SLLQueuePush_N(idxs->order_first, idxs->order_last, node, order_next);
    SLLStackPush_N(idxs->buckets[bucket_idx], node, bucket_next);
    match = node;
    idxs->bucket_collision_count += (node->bucket_next != 0);
  }
  
  // extract idx to return
  Assert(match != 0);
  RADDBGI_U32 result = match->first_idx;
  return result;
}

//- rjf: data section baking

RADDBGI_PROC RADDBGI_U32
raddbgic_dsection(RADDBGIC_Arena *arena, RADDBGIC_DSections *dss, void *data, RADDBGI_U64 size, RADDBGI_DataSectionTag tag)
{
  RADDBGI_U32 result = dss->count;
  RADDBGIC_DSectionNode *node = raddbgic_push_array(arena, RADDBGIC_DSectionNode, 1);
  SLLQueuePush(dss->first, dss->last, node);
  node->data = data;
  node->size = size;
  node->tag = tag;
  dss->count += 1;
  return result;
}

//- rjf: paths baking

RADDBGI_PROC RADDBGIC_String8
raddbgic_normal_string_from_path_node(RADDBGIC_Arena *arena, RADDBGIC_PathNode *node)
{
  Temp scratch = scratch_begin(&arena, 1);
  RADDBGIC_String8List list = {0};
  if(node != 0)
  {
    raddbgic_normal_string_from_path_node_build(scratch.arena, node, &list);
  }
  StringJoin join = {0};
  join.sep = str8_lit("/");
  RADDBGIC_String8 result = str8_list_join(arena, &list, &join);
  {
    RADDBGI_U8 *ptr = result.str;
    RADDBGI_U8 *opl = result.str + result.size;
    for(; ptr < opl; ptr += 1)
    {
      RADDBGI_U8 c = *ptr;
      if('A' <= c && c <= 'Z') { c += 'a' - 'A'; }
      *ptr = c;
    }
  }
  scratch_end(scratch);
  return result;
}

RADDBGI_PROC void
raddbgic_normal_string_from_path_node_build(RADDBGIC_Arena *arena, RADDBGIC_PathNode *node, RADDBGIC_String8List *out)
{
  // TODO(rjf): why is this recursive...
  if(node->parent != 0)
  {
    raddbgic_normal_string_from_path_node_build(arena, node->parent, out);
  }
  if(node->name.size > 0)
  {
    str8_list_push(arena, out, node->name);
  }
}

RADDBGI_PROC RADDBGIC_PathNode*
raddbgic_paths_new_node(RADDBGIC_BakeCtx *bctx)
{
  RADDBGIC_PathTree *tree = bctx->tree;
  RADDBGIC_PathNode *result = raddbgic_push_array(bctx->arena, RADDBGIC_PathNode, 1);
  SLLQueuePush_N(tree->first, tree->last, result, next_order);
  result->idx = tree->count;
  tree->count += 1;
  return result;
}

RADDBGI_PROC RADDBGIC_PathNode*
raddbgic_paths_sub_path(RADDBGIC_BakeCtx *bctx, RADDBGIC_PathNode *dir, RADDBGIC_String8 sub_dir)
{
  // look for existing match
  RADDBGIC_PathNode *match = 0;
  for(RADDBGIC_PathNode *node = dir->first_child;
      node != 0;
      node = node->next_sibling){
    if(str8_match(node->name, sub_dir, StringMatchFlag_CaseInsensitive)){
      match = node;
      break;
    }
  }
  
  // construct new node if no match
  RADDBGIC_PathNode *new_node = 0;
  if(match == 0){
    new_node = raddbgic_paths_new_node(bctx);
    new_node->parent = dir;
    SLLQueuePush_N(dir->first_child, dir->last_child, new_node, next_sibling);
    new_node->name = raddbgic_str8_copy(bctx->arena, sub_dir);
  }
  
  // select result from the two paths
  RADDBGIC_PathNode *result = match;
  if(match == 0){
    result = new_node;
  }
  
  return result;
}

RADDBGI_PROC RADDBGIC_PathNode*
raddbgic_paths_node_from_path(RADDBGIC_BakeCtx *bctx,  RADDBGIC_String8 path)
{
  RADDBGIC_PathNode *node_cursor = &bctx->tree->root;
  
  RADDBGI_U8 *ptr = path.str;
  RADDBGI_U8 *opl = path.str + path.size;
  for(;ptr < opl;){
    // skip past slashes
    for(;ptr < opl && (*ptr == '/' || *ptr == '\\'); ptr += 1);
    
    // save beginning of non-slash range
    RADDBGI_U8 *range_first = ptr;
    
    // skip past non-slashes
    for(;ptr < opl && !(*ptr == '/' || *ptr == '\\'); ptr += 1);
    
    // if range is non-empty advance the node cursor
    if(range_first < ptr){
      RADDBGIC_String8 sub_dir = str8_range(range_first, ptr);
      node_cursor = raddbgic_paths_sub_path(bctx, node_cursor, sub_dir);
    }
  }
  
  RADDBGIC_PathNode *result = node_cursor;
  return result;
}

RADDBGI_PROC RADDBGI_U32
raddbgic_paths_idx_from_path(RADDBGIC_BakeCtx *bctx, RADDBGIC_String8 path)
{
  RADDBGIC_PathNode *node = raddbgic_paths_node_from_path(bctx, path);
  RADDBGI_U32 result = node->idx;
  return result;
}

RADDBGI_PROC RADDBGIC_SrcNode*
raddbgic_paths_new_src_node(RADDBGIC_BakeCtx *bctx)
{
  RADDBGIC_PathTree *tree = bctx->tree;
  RADDBGIC_SrcNode *result = raddbgic_push_array(bctx->arena, RADDBGIC_SrcNode, 1);
  SLLQueuePush(tree->src_first, tree->src_last, result);
  result->idx = tree->src_count;
  tree->src_count += 1;
  return result;
}

RADDBGI_PROC RADDBGIC_SrcNode*
raddbgic_paths_src_node_from_path_node(RADDBGIC_BakeCtx *bctx, RADDBGIC_PathNode *path_node)
{
  RADDBGIC_SrcNode *result = path_node->src_file;
  if(result == 0)
  {
    RADDBGIC_SrcNode *new_node = raddbgic_paths_new_src_node(bctx);
    new_node->path_node = path_node;
    new_node->normal_full_path = raddbgic_normal_string_from_path_node(bctx->arena, path_node);
    result = path_node->src_file = new_node;
  }
  return result;
}

//- rjf: per-unit line info baking

RADDBGI_PROC RADDBGIC_UnitLinesCombined*
raddbgic_unit_combine_lines(RADDBGIC_Arena *arena, RADDBGIC_BakeCtx *bctx, RADDBGIC_LineSequenceNode *first_seq)
{
  ProfBegin("raddbgic_unit_combine_lines");
  Temp scratch = scratch_begin(&arena, 1);
  
  // gather up all line info into two arrays
  //  keys: sortable array; pairs voffs with line info records; null records are sequence enders
  //  recs: contains all the source coordinates for a range of voffs
  RADDBGI_U64 line_count = 0;
  RADDBGI_U64 seq_count = 0;
  for(RADDBGIC_LineSequenceNode *node = first_seq;
      node != 0;
      node = node->next){
    seq_count += 1;
    line_count += node->line_seq.line_count;
  }
  
  RADDBGI_U64 key_count = line_count + seq_count;
  RADDBGIC_SortKey *line_keys = push_array_no_zero(scratch.arena, RADDBGIC_SortKey, key_count);
  RADDBGIC_LineRec *line_recs = push_array_no_zero(scratch.arena, RADDBGIC_LineRec, line_count);
  
  {
    RADDBGIC_SortKey *key_ptr = line_keys;
    RADDBGIC_LineRec *rec_ptr = line_recs;
    
    for(RADDBGIC_LineSequenceNode *node = first_seq;
        node != 0;
        node = node->next){
      RADDBGIC_PathNode *src_path =
        raddbgic_paths_node_from_path(bctx, node->line_seq.file_name);
      RADDBGIC_SrcNode *src_file  = raddbgic_paths_src_node_from_path_node(bctx, src_path);
      RADDBGI_U32 file_id = src_file->idx;
      
      RADDBGI_U64 node_line_count = node->line_seq.line_count;
      for(RADDBGI_U64 i = 0; i < node_line_count; i += 1){
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
      
      RADDBGIC_LineMapFragment *fragment = raddbgic_push_array(arena, RADDBGIC_LineMapFragment, 1);
      SLLQueuePush(src_file->first_fragment, src_file->last_fragment, fragment);
      fragment->sequence = node;
    }
  }
  
  // sort
  RADDBGIC_SortKey *sorted_line_keys = raddbgic_sort_key_array(scratch.arena, line_keys, key_count);
  
  // TODO(allen): do a pass over sorted keys to make sure duplicate keys are sorted with
  // null record first, and no more than one null record and one non-null record
  
  // arrange output
  RADDBGI_U64 *arranged_voffs = push_array_no_zero(arena, RADDBGI_U64, key_count + 1);
  RADDBGI_Line *arranged_lines = push_array_no_zero(arena, RADDBGI_Line, key_count);
  
  for(RADDBGI_U64 i = 0; i < key_count; i += 1){
    arranged_voffs[i] = sorted_line_keys[i].key;
  }
  arranged_voffs[key_count] = ~0ull;
  for(RADDBGI_U64 i = 0; i < key_count; i += 1){
    RADDBGIC_LineRec *rec = (RADDBGIC_LineRec*)sorted_line_keys[i].val;
    if(rec != 0){
      arranged_lines[i].file_idx = rec->file_id;
      arranged_lines[i].line_num = rec->line_num;
    }
    else{
      arranged_lines[i].file_idx = 0;
      arranged_lines[i].line_num = 0;
    }
  }
  
  RADDBGIC_UnitLinesCombined *result = raddbgic_push_array(arena, RADDBGIC_UnitLinesCombined, 1);
  result->voffs = arranged_voffs;
  result->lines = arranged_lines;
  result->cols = 0;
  result->line_count = key_count;
  
  scratch_end(scratch);
  ProfEnd();
  
  return result;
}

//- rjf: per-src line info baking

RADDBGI_PROC RADDBGIC_SrcLinesCombined*
raddbgic_source_combine_lines(RADDBGIC_Arena *arena, RADDBGIC_LineMapFragment *first)
{
  ProfBegin("raddbgic_source_combine_lines");
  Temp scratch = scratch_begin(&arena, 1);
  
  // gather line number map
  RADDBGIC_SrcLineMapBucket *first_bucket = 0;
  RADDBGIC_SrcLineMapBucket *last_bucket = 0;
  RADDBGI_U64 line_hash_slots_count = 1024;
  RADDBGIC_SrcLineMapBucket **line_hash_slots = raddbgic_push_array(scratch.arena, RADDBGIC_SrcLineMapBucket *, line_hash_slots_count);
  RADDBGI_U64 line_count = 0;
  RADDBGI_U64 voff_count = 0;
  RADDBGI_U64 max_line_num = 0;
  ProfScope("gather line number map")
  {
    for(RADDBGIC_LineMapFragment *map_fragment = first;
        map_fragment != 0;
        map_fragment = map_fragment->next)
    {
      RADDBGIC_LineSequence *sequence = &map_fragment->sequence->line_seq;
      
      RADDBGI_U64 *seq_voffs = sequence->voffs;
      RADDBGI_U32 *seq_line_nums = sequence->line_nums;
      RADDBGI_U64 seq_line_count = sequence->line_count;
      for(RADDBGI_U64 i = 0; i < seq_line_count; i += 1){
        RADDBGI_U32 line_num = seq_line_nums[i];
        RADDBGI_U64 voff = seq_voffs[i];
        RADDBGI_U64 line_hash_slot_idx = line_num%line_hash_slots_count;
        
        // update unique voff counter & max line number
        voff_count += 1;
        max_line_num = Max(max_line_num, line_num);
        
        // find match
        RADDBGIC_SrcLineMapBucket *match = 0;
        {
          for(RADDBGIC_SrcLineMapBucket *node = line_hash_slots[line_hash_slot_idx];
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
          match = raddbgic_push_array(scratch.arena, RADDBGIC_SrcLineMapBucket, 1);
          SLLQueuePush_N(first_bucket, last_bucket, match, order_next);
          SLLStackPush_N(line_hash_slots[line_hash_slot_idx], match, hash_next);
          match->line_num = line_num;
          line_count += 1;
        }
        
        // insert new voff
        {
          RADDBGIC_SrcLineMapVoffBlock *block = raddbgic_push_array(scratch.arena, RADDBGIC_SrcLineMapVoffBlock, 1);
          SLLQueuePush(match->first_voff_block, match->last_voff_block, block);
          match->voff_count += 1;
          block->voff = voff;
        }
      }
    }
  }
  
  // bake sortable keys array
  RADDBGIC_SortKey *keys = push_array_no_zero(scratch.arena, RADDBGIC_SortKey, line_count);
  ProfScope("bake sortable keys array")
  {
    RADDBGIC_SortKey *key_ptr = keys;
    for(RADDBGIC_SrcLineMapBucket *node = first_bucket;
        node != 0;
        node = node->order_next, key_ptr += 1){
      key_ptr->key = node->line_num;
      key_ptr->val = node;
    }
  }
  
  // sort
  RADDBGIC_SortKey *sorted_keys = raddbgic_sort_key_array(scratch.arena, keys, line_count);
  
  // bake result
  RADDBGI_U32 *line_nums = push_array_no_zero(arena, RADDBGI_U32, line_count);
  RADDBGI_U32 *line_ranges = push_array_no_zero(arena, RADDBGI_U32, line_count + 1);
  RADDBGI_U64 *voffs = push_array_no_zero(arena, RADDBGI_U64, voff_count);
  ProfScope("bake result")
  {
    RADDBGI_U64 *voff_ptr = voffs;
    for(RADDBGI_U32 i = 0; i < line_count; i += 1){
      line_nums[i] = sorted_keys[i].key;
      line_ranges[i] = (RADDBGI_U32)(voff_ptr - voffs);
      RADDBGIC_SrcLineMapBucket *bucket = (RADDBGIC_SrcLineMapBucket*)sorted_keys[i].val;
      for(RADDBGIC_SrcLineMapVoffBlock *node = bucket->first_voff_block;
          node != 0;
          node = node->next){
        *voff_ptr = node->voff;
        voff_ptr += 1;
      }
    }
    line_ranges[line_count] = voff_count;
  }
  
  RADDBGIC_SrcLinesCombined *result = raddbgic_push_array(arena, RADDBGIC_SrcLinesCombined, 1);
  result->line_nums = line_nums;
  result->line_ranges = line_ranges;
  result->line_count = line_count;
  result->voffs = voffs;
  result->voff_count = voff_count;
  
  scratch_end(scratch);
  ProfEnd();
  
  return result;
}

//- rjf: vmap baking
RADDBGI_PROC RADDBGIC_VMap*
raddbgic_vmap_from_markers(RADDBGIC_Arena *arena, RADDBGIC_VMapMarker *markers, RADDBGIC_SortKey *keys, RADDBGI_U64 marker_count)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // sort markers
  RADDBGIC_SortKey *sorted_keys = raddbgic_sort_key_array(scratch.arena, keys, marker_count);
  
  // determine if an extra vmap entry for zero is needed
  RADDBGI_U32 extra_vmap_entry = 0;
  if(marker_count > 0 && sorted_keys[0].key != 0){
    extra_vmap_entry = 1;
  }
  
  // fill output vmap entries
  RADDBGI_U32 vmap_count_raw = marker_count - 1 + extra_vmap_entry;
  RADDBGI_VMapEntry *vmap = push_array_no_zero(arena, RADDBGI_VMapEntry, vmap_count_raw + 1);
  RADDBGI_U32 vmap_entry_count_pass_1 = 0;
  
  {
    RADDBGI_VMapEntry *vmap_ptr = vmap;
    
    if(extra_vmap_entry){
      vmap_ptr->voff = 0;
      vmap_ptr->idx = 0;
      vmap_ptr += 1;
    }
    
    RADDBGIC_VMapRangeTracker *tracker_stack = 0;
    RADDBGIC_VMapRangeTracker *tracker_free = 0;
    
    RADDBGIC_SortKey *key_ptr = sorted_keys;
    RADDBGIC_SortKey *key_opl = sorted_keys + marker_count;
    for(;key_ptr < key_opl;){
      // get initial map state from tracker stack
      RADDBGI_U32 initial_idx = max_U32;
      if(tracker_stack != 0){
        initial_idx = tracker_stack->idx;
      }
      
      // update tracker stack
      // * we must process _all_ of the changes that apply at this voff before moving on
      RADDBGI_U64 voff = key_ptr->key;
      
      for(;key_ptr < key_opl && key_ptr->key == voff; key_ptr += 1){
        RADDBGIC_VMapMarker *marker = (RADDBGIC_VMapMarker*)key_ptr->val;
        RADDBGI_U32 idx = marker->idx;
        
        // push to stack
        if(marker->begin_range){
          RADDBGIC_VMapRangeTracker *new_tracker = tracker_free;
          if(new_tracker != 0){
            SLLStackPop(tracker_free);
          }
          else{
            new_tracker = raddbgic_push_array(scratch.arena, RADDBGIC_VMapRangeTracker, 1);
          }
          SLLStackPush(tracker_stack, new_tracker);
          new_tracker->idx = idx;
        }
        
        // pop matching node from stack (not always the top)
        else{
          RADDBGIC_VMapRangeTracker **ptr_in = &tracker_stack;
          RADDBGIC_VMapRangeTracker *match = 0;
          for(RADDBGIC_VMapRangeTracker *node = tracker_stack;
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
            SLLStackPush(tracker_free, match);
          }
        }
      }
      
      // get final map state from tracker stack
      RADDBGI_U32 final_idx = 0;
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
    
    vmap_entry_count_pass_1 = (RADDBGI_U32)(vmap_ptr - vmap);
  }
  
  // replace zero unit indexes that follow a non-zero
  // TODO(rjf): 0 *is* a real unit index right now
  if(0)
  {
    //  (the last entry is not replaced because it acts as a terminator)
    RADDBGI_U32 last = vmap_entry_count_pass_1 - 1;
    
    RADDBGI_VMapEntry *vmap_ptr = vmap;
    RADDBGI_U64 real_idx = 0;
    
    for(RADDBGI_U32 i = 0; i < last; i += 1, vmap_ptr += 1){
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
  RADDBGI_U32 vmap_entry_count = 0;
  {
    RADDBGI_VMapEntry *vmap_ptr = vmap;
    RADDBGI_VMapEntry *vmap_opl = vmap + vmap_entry_count_pass_1;
    RADDBGI_VMapEntry *vmap_out = vmap;
    
    for(;vmap_ptr < vmap_opl;){
      RADDBGI_VMapEntry *vmap_range_first = vmap_ptr;
      RADDBGI_U64 idx = vmap_ptr->idx;
      vmap_ptr += 1;
      for(;vmap_ptr < vmap_opl && vmap_ptr->idx == idx;) vmap_ptr += 1;
      MemoryCopyStruct(vmap_out, vmap_range_first);
      vmap_out += 1;
    }
    
    vmap_entry_count = (RADDBGI_U32)(vmap_out - vmap);
  }
  
  // fill result
  RADDBGIC_VMap *result = raddbgic_push_array(arena, RADDBGIC_VMap, 1);
  result->vmap = vmap;
  result->count = vmap_entry_count - 1;
  
  scratch_end(scratch);
  
  return result;
}

RADDBGI_PROC RADDBGIC_VMap*
raddbgic_vmap_from_unit_ranges(RADDBGIC_Arena *arena, RADDBGIC_UnitVMapRange *first, RADDBGI_U64 count)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // count necessary markers
  RADDBGI_U64 marker_count = count*2;
  
  // fill markers
  RADDBGIC_SortKey    *keys = push_array_no_zero(scratch.arena, RADDBGIC_SortKey, marker_count);
  RADDBGIC_VMapMarker *markers = push_array_no_zero(scratch.arena, RADDBGIC_VMapMarker, marker_count);
  
  {
    RADDBGIC_SortKey *key_ptr = keys;
    RADDBGIC_VMapMarker *marker_ptr = markers;
    for(RADDBGIC_UnitVMapRange *range = first;
        range != 0;
        range = range->next){
      if(range->first < range->opl){
        RADDBGI_U32 unit_idx = range->unit->idx;
        
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
  RADDBGIC_VMap *result = raddbgic_vmap_from_markers(arena, markers, keys, marker_count);
  scratch_end(scratch);
  return result;
}

//- rjf: type info baking

RADDBGI_PROC RADDBGI_U32*
raddbgic_idx_run_from_types(RADDBGIC_Arena *arena, RADDBGIC_Type **types, RADDBGI_U32 count)
{
  RADDBGI_U32 *result = raddbgic_push_array(arena, RADDBGI_U32, count);
  for(RADDBGI_U32 i = 0; i < count; i += 1){
    result[i] = types[i]->idx;
  }
  return result;
}

RADDBGI_PROC RADDBGIC_TypeData*
raddbgic_type_data_combine(RADDBGIC_Arena *arena, RADDBGIC_Root *root, RADDBGIC_BakeCtx *bctx)
{
  ProfBegin("raddbgic_type_data_combine");
  Temp scratch = scratch_begin(&arena, 1);
  
  // fill type nodes
  RADDBGI_U32 type_count = root->type_count;
  RADDBGI_TypeNode *type_nodes = push_array_no_zero(arena, RADDBGI_TypeNode, type_count);
  
  {
    RADDBGI_TypeNode *ptr = type_nodes;
    RADDBGI_TypeNode *opl = ptr + type_count;
    RADDBGIC_Type *loose_type = root->first_type;
    for(;loose_type != 0 && ptr < opl;
        loose_type = loose_type->next_order, ptr += 1){
      
      RADDBGI_TypeKind kind = loose_type->kind;
      
      // shared
      ptr->kind = kind;
      ptr->flags = loose_type->flags;
      ptr->byte_size = loose_type->byte_size;
      
      // built-in
      if(RADDBGI_TypeKind_FirstBuiltIn <= kind && kind <= RADDBGI_TypeKind_LastBuiltIn){
        ptr->built_in.name_string_idx = raddbgic_string(bctx, loose_type->name);
      }
      
      // constructed
      else if(RADDBGI_TypeKind_FirstConstructed <= kind && kind <= RADDBGI_TypeKind_LastConstructed){
        ptr->constructed.direct_type_idx = loose_type->direct_type->idx;
        
        switch (kind){
          case RADDBGI_TypeKind_Array:
          {
            ptr->constructed.count = loose_type->count;
          }break;
          
          case RADDBGI_TypeKind_Function:
          {
            // parameters
            RADDBGI_U32 count = loose_type->count;
            RADDBGI_U32 *idx_run = raddbgic_idx_run_from_types(scratch.arena, loose_type->param_types, count);
            ptr->constructed.param_idx_run_first = raddbgic_idx_run(bctx, idx_run, count);
            ptr->constructed.count = count;
          }break;
          
          case RADDBGI_TypeKind_Method:
          {
            // parameters
            RADDBGI_U32 count = loose_type->count;
            RADDBGI_U32 *idx_run = raddbgic_idx_run_from_types(scratch.arena, loose_type->param_types, count);
            ptr->constructed.param_idx_run_first = raddbgic_idx_run(bctx, idx_run, count);
            ptr->constructed.count = count;
          }break;
        }
      }
      
      // user-defined
      else if(RADDBGI_TypeKind_FirstUserDefined <= kind && kind <= RADDBGI_TypeKind_LastUserDefined){
        ptr->user_defined.name_string_idx = raddbgic_string(bctx, loose_type->name);
        if(loose_type->udt != 0){
          ptr->user_defined.udt_idx = loose_type->udt->idx;
        }
        if(loose_type->direct_type != 0){
          ptr->user_defined.direct_type_idx = loose_type->direct_type->idx;
        }
      }
      
      // bitfield
      else if(kind == RADDBGI_TypeKind_Bitfield){
        ptr->bitfield.off = loose_type->off;
        ptr->bitfield.size = loose_type->count;
      }
      
      temp_end(scratch);
    }
    
    // both iterators should end at the same time
    Assert(loose_type == 0);
    Assert(ptr == opl);
  }
  
  
  // fill udts
  RADDBGI_U32 udt_count = root->type_udt_count;
  RADDBGI_UDT *udts = push_array_no_zero(arena, RADDBGI_UDT, udt_count);
  
  RADDBGI_U32 member_count = root->total_member_count;
  RADDBGI_Member *members = push_array_no_zero(arena, RADDBGI_Member, member_count);
  
  RADDBGI_U32 enum_member_count = root->total_enum_val_count;
  RADDBGI_EnumMember *enum_members = push_array_no_zero(arena, RADDBGI_EnumMember, enum_member_count);
  
  {
    RADDBGI_UDT *ptr = udts;
    RADDBGI_UDT *opl = ptr + udt_count;
    
    RADDBGI_Member *member_ptr = members;
    RADDBGI_Member *member_opl = members + member_count;
    
    RADDBGI_EnumMember *enum_member_ptr = enum_members;
    RADDBGI_EnumMember *enum_member_opl = enum_members + enum_member_count;
    
    RADDBGIC_TypeUDT *loose_udt = root->first_udt;
    for(;loose_udt != 0 && ptr < opl;
        loose_udt = loose_udt->next_order, ptr += 1){
      ptr->self_type_idx = loose_udt->self_type->idx;
      
      Assert(loose_udt->member_count == 0 ||
             loose_udt->enum_val_count == 0);
      
      // enum members
      if(loose_udt->enum_val_count != 0){
        ptr->flags |= RADDBGI_UserDefinedTypeFlag_EnumMembers;
        
        ptr->member_first = (RADDBGI_U32)(enum_member_ptr - enum_members);
        ptr->member_count = loose_udt->enum_val_count;
        
        RADDBGI_U32 local_enum_val_count = loose_udt->enum_val_count;
        RADDBGIC_TypeEnumVal *loose_enum_val = loose_udt->first_enum_val;
        for(RADDBGI_U32 i = 0;
            i < local_enum_val_count;
            i += 1, enum_member_ptr += 1, loose_enum_val = loose_enum_val->next){
          enum_member_ptr->name_string_idx = raddbgic_string(bctx, loose_enum_val->name);
          enum_member_ptr->val = loose_enum_val->val;
        }
      }
      
      // struct/class/union members
      else{
        ptr->member_first = (RADDBGI_U32)(member_ptr - members);
        ptr->member_count = loose_udt->member_count;
        
        RADDBGI_U32 local_member_count = loose_udt->member_count;
        RADDBGIC_TypeMember *loose_member = loose_udt->first_member;
        for(RADDBGI_U32 i = 0;
            i < local_member_count;
            i += 1, member_ptr += 1, loose_member = loose_member->next){
          member_ptr->kind = loose_member->kind;
          // TODO(allen): member_ptr->visibility = ;
          member_ptr->name_string_idx = raddbgic_string(bctx, loose_member->name);
          member_ptr->off = loose_member->off;
          member_ptr->type_idx = loose_member->type->idx;
          
          // TODO(allen): 
          if(loose_member->kind == RADDBGI_MemberKind_Method){
            //loose_member_ptr->unit_idx = ;
            //loose_member_ptr->proc_symbol_idx = ;
          }
        }
        
      }
      
      RADDBGI_U32 file_idx = 0;
      if(loose_udt->source_path.size > 0){
        RADDBGIC_PathNode *path_node = raddbgic_paths_node_from_path(bctx, loose_udt->source_path);
        RADDBGIC_SrcNode  *src_node  = raddbgic_paths_src_node_from_path_node(bctx, path_node);
        file_idx = src_node->idx;
      }
      
      ptr->file_idx = file_idx;
      ptr->line = loose_udt->line;
      ptr->col = loose_udt->col;
    }
    
    // all iterators should end at the same time
    Assert(loose_udt == 0);
    Assert(ptr == opl);
    Assert(member_ptr == member_opl);
    Assert(enum_member_ptr == enum_member_opl);
  }
  
  
  // fill result
  RADDBGIC_TypeData *result = raddbgic_push_array(arena, RADDBGIC_TypeData, 1);
  result->type_nodes = type_nodes;
  result->type_node_count = type_count;
  result->udts = udts;
  result->udt_count = udt_count;
  result->members = members;
  result->member_count = member_count;
  result->enum_members = enum_members;
  result->enum_member_count = enum_member_count;
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

//- rjf: symbol data baking

RADDBGI_PROC RADDBGIC_SymbolData*
raddbgic_symbol_data_combine(RADDBGIC_Arena *arena, RADDBGIC_Root *root, RADDBGIC_BakeCtx *bctx)
{
  ProfBegin("raddbgic_symbol_data_combine");
  Temp scratch = scratch_begin(&arena, 1);
  
  // count symbol kinds
  RADDBGI_U32 globalvar_count = 1 + root->symbol_kind_counts[RADDBGIC_SymbolKind_GlobalVariable];
  RADDBGI_U32 threadvar_count = 1 + root->symbol_kind_counts[RADDBGIC_SymbolKind_ThreadVariable];
  RADDBGI_U32 procedure_count = 1 + root->symbol_kind_counts[RADDBGIC_SymbolKind_Procedure];
  
  // allocate symbol arrays
  RADDBGI_GlobalVariable *global_variables =
    raddbgic_push_array(arena, RADDBGI_GlobalVariable, globalvar_count);
  
  RADDBGI_ThreadVariable *thread_variables =
    raddbgic_push_array(arena, RADDBGI_ThreadVariable, threadvar_count);
  
  RADDBGI_Procedure *procedures = raddbgic_push_array(arena, RADDBGI_Procedure, procedure_count);
  
  // fill symbol arrays
  {
    RADDBGI_GlobalVariable *global_ptr = global_variables;
    RADDBGI_ThreadVariable *thread_local_ptr = thread_variables;
    RADDBGI_Procedure *procedure_ptr = procedures;
    
    // nils
    global_ptr += 1;
    thread_local_ptr += 1;
    procedure_ptr += 1;
    
    // symbol nodes
    for(RADDBGIC_Symbol *node = root->first_symbol;
        node != 0;
        node = node->next_order){
      RADDBGI_U32 name_string_idx = raddbgic_string(bctx, node->name);
      RADDBGI_U32 link_name_string_idx = raddbgic_string(bctx, node->link_name);
      RADDBGI_U32 type_idx = node->type->idx;
      
      RADDBGI_LinkFlags link_flags = 0;
      RADDBGI_U32 container_idx = 0;
      {      
        if(node->is_extern){
          link_flags |= RADDBGI_LinkFlag_External;
        }
        if(node->container_symbol != 0){
          container_idx = node->container_symbol->idx;
          link_flags |= RADDBGI_LinkFlag_ProcScoped;
        }
        else if(node->container_type != 0 && node->container_type->udt != 0){
          container_idx = node->container_type->udt->idx;
          link_flags |= RADDBGI_LinkFlag_TypeScoped;
        }
      }
      
      switch (node->kind){
        default:{}break;
        
        case RADDBGIC_SymbolKind_GlobalVariable:
        {
          global_ptr->name_string_idx = name_string_idx;
          global_ptr->link_flags = link_flags;
          global_ptr->voff = node->offset;
          global_ptr->type_idx = type_idx;
          global_ptr->container_idx = container_idx;
          global_ptr += 1;
        }break;
        
        case RADDBGIC_SymbolKind_ThreadVariable:
        {
          thread_local_ptr->name_string_idx = name_string_idx;
          thread_local_ptr->link_flags = link_flags;
          thread_local_ptr->tls_off = (RADDBGI_U32)node->offset;
          thread_local_ptr->type_idx = type_idx;
          thread_local_ptr->container_idx = container_idx;
          thread_local_ptr += 1;
        }break;
        
        case RADDBGIC_SymbolKind_Procedure:
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
    
    Assert(global_ptr - global_variables == globalvar_count);
    Assert(thread_local_ptr - thread_variables == threadvar_count);
    Assert(procedure_ptr - procedures == procedure_count);
  }
  
  // global vmap
  RADDBGIC_VMap *global_vmap = 0;
  {
    // count necessary markers
    RADDBGI_U32 marker_count = globalvar_count*2;
    
    // fill markers
    RADDBGIC_SortKey    *keys = push_array_no_zero(scratch.arena, RADDBGIC_SortKey, marker_count);
    RADDBGIC_VMapMarker *markers = push_array_no_zero(scratch.arena, RADDBGIC_VMapMarker, marker_count);
    
    RADDBGIC_SortKey *key_ptr = keys;
    RADDBGIC_VMapMarker *marker_ptr = markers;
    
    // real globals
    for(RADDBGIC_Symbol *node = root->first_symbol;
        node != 0;
        node = node->next_order){
      if(node->kind == RADDBGIC_SymbolKind_GlobalVariable){
        RADDBGI_U32 global_idx = node->idx;
        
        RADDBGI_U64 first = node->offset;
        RADDBGI_U64 opl   = first + node->type->byte_size;
        
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
      RADDBGI_U32 global_idx = 0;
      
      RADDBGI_U64 first = 0;
      RADDBGI_U64 opl   = max_U64;
      
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
    Assert(key_ptr - keys == marker_count &&
           marker_ptr - markers == marker_count);
    
    // construct vmap
    global_vmap = raddbgic_vmap_from_markers(arena, markers, keys, marker_count);
  }
  
  // allocate scope array
  
  // (assert there is a nil scope)
  Assert(root->first_scope != 0 &&
         root->first_scope->symbol == 0 &&
         root->first_scope->first_child == 0 &&
         root->first_scope->next_sibling == 0 &&
         root->first_scope->range_count == 0);
  
  RADDBGI_U32 scope_count = root->scope_count;
  RADDBGI_Scope *scopes = raddbgic_push_array(arena, RADDBGI_Scope, scope_count);
  
  RADDBGI_U32 scope_voff_count = root->scope_voff_count;
  RADDBGI_U64 *scope_voffs = raddbgic_push_array(arena, RADDBGI_U64, scope_voff_count);
  
  RADDBGI_U32 local_count = root->local_count;
  RADDBGI_Local *locals = raddbgic_push_array(arena, RADDBGI_Local, local_count);
  
  RADDBGI_U32 location_block_count = root->location_count;
  RADDBGI_LocationBlock *location_blocks =
    raddbgic_push_array(arena, RADDBGI_LocationBlock, location_block_count);
  
  RADDBGIC_String8List location_data = {0};
  
  // iterate scopes, locals, and locations
  //  fill scope voffs, locals, and location information
  {
    RADDBGI_Scope *scope_ptr = scopes;
    RADDBGI_U64 *scope_voff_ptr = scope_voffs;
    RADDBGI_Local *local_ptr = locals;
    RADDBGI_LocationBlock *location_block_ptr = location_blocks;
    
    for(RADDBGIC_Scope *node = root->first_scope;
        node != 0;
        node = node->next_order, scope_ptr += 1){
      
      // emit voffs
      RADDBGI_U32 voff_first = (RADDBGI_U32)(scope_voff_ptr - scope_voffs);
      for(RADDBGIC_VOffRange *range = node->first_range;
          range != 0;
          range = range->next){
        *scope_voff_ptr = range->voff_first;
        scope_voff_ptr += 1;
        *scope_voff_ptr = range->voff_opl;
        scope_voff_ptr += 1;
      }
      RADDBGI_U32 voff_opl = (RADDBGI_U32)(scope_voff_ptr - scope_voffs);
      
      // emit locals
      RADDBGI_U32 scope_local_count = node->local_count;
      RADDBGI_U32 scope_local_first = (RADDBGI_U32)(local_ptr - locals);
      for(RADDBGIC_Local *slocal = node->first_local;
          slocal != 0;
          slocal = slocal->next, local_ptr += 1){
        local_ptr->kind = slocal->kind;
        local_ptr->name_string_idx = raddbgic_string(bctx, slocal->name);
        local_ptr->type_idx = slocal->type->idx;
        
        RADDBGIC_LocationSet *locset = slocal->locset;
        if(locset != 0){
          RADDBGI_U32 location_first = (RADDBGI_U32)(location_block_ptr - location_blocks);
          RADDBGI_U32 location_opl   = location_first + locset->location_case_count;
          local_ptr->location_first = location_first;
          local_ptr->location_opl   = location_opl;
          
          for(RADDBGIC_LocationCase *location_case = locset->first_location_case;
              location_case != 0;
              location_case = location_case->next){
            location_block_ptr->scope_off_first = location_case->voff_first;
            location_block_ptr->scope_off_opl   = location_case->voff_opl;
            location_block_ptr->location_data_off = location_data.total_size;
            location_block_ptr += 1;
            
            RADDBGIC_Location *location = location_case->location;
            if(location == 0){
              RADDBGI_U64 data = 0;
              str8_serial_push_align(scratch.arena, &location_data, 8);
              str8_serial_push_data(scratch.arena, &location_data, &data, 1);
            }
            else{
              switch (location->kind){
                default:
                {
                  RADDBGI_U64 data = 0;
                  str8_serial_push_align(scratch.arena, &location_data, 8);
                  str8_serial_push_data(scratch.arena, &location_data, &data, 1);
                }break;
                
                case RADDBGI_LocationKind_AddrBytecodeStream:
                case RADDBGI_LocationKind_ValBytecodeStream:
                {
                  str8_list_push(scratch.arena, &location_data, raddbgic_str8_copy(scratch.arena, str8_struct(&location->kind)));
                  for(RADDBGIC_EvalBytecodeOp *op_node = location->bytecode.first_op;
                      op_node != 0;
                      op_node = op_node->next){
                    RADDBGI_U8 op_data[9];
                    op_data[0] = op_node->op;
                    MemoryCopy(op_data + 1, &op_node->p, op_node->p_size);
                    RADDBGIC_String8 op_data_str = str8(op_data, 1 + op_node->p_size);
                    str8_list_push(scratch.arena, &location_data, raddbgic_str8_copy(scratch.arena, op_data_str));
                  }
                  {
                    RADDBGI_U64 data = 0;
                    RADDBGIC_String8 data_str = str8((RADDBGI_U8 *)&data, 1);
                    str8_list_push(scratch.arena, &location_data, raddbgic_str8_copy(scratch.arena, data_str));
                  }
                }break;
                
                case RADDBGI_LocationKind_AddrRegisterPlusU16:
                case RADDBGI_LocationKind_AddrAddrRegisterPlusU16:
                {
                  RADDBGI_LocationRegisterPlusU16 loc = {0};
                  loc.kind = location->kind;
                  loc.register_code = location->register_code;
                  loc.offset = location->offset;
                  str8_list_push(scratch.arena, &location_data, raddbgic_str8_copy(scratch.arena, str8_struct(&loc)));
                }break;
                
                case RADDBGI_LocationKind_ValRegister:
                {
                  RADDBGI_LocationRegister loc = {0};
                  loc.kind = location->kind;
                  loc.register_code = location->register_code;
                  str8_list_push(scratch.arena, &location_data, raddbgic_str8_copy(scratch.arena, str8_struct(&loc)));
                }break;
              }
            }
          }
          
          Assert(location_block_ptr - location_blocks == location_opl);
        }
      }
      
      Assert(local_ptr - locals == scope_local_first + scope_local_count);
      
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
    
    Assert(scope_ptr - scopes == scope_count);
    Assert(local_ptr - locals == local_count);
  }
  
  // flatten location data
  RADDBGIC_String8 location_data_str = str8_list_join(arena, &location_data, 0);
  
  // scope vmap
  RADDBGIC_VMap *scope_vmap = 0;
  {
    // count necessary markers
    RADDBGI_U32 marker_count = scope_voff_count;
    
    // fill markers
    RADDBGIC_SortKey    *keys = push_array_no_zero(scratch.arena, RADDBGIC_SortKey, marker_count);
    RADDBGIC_VMapMarker *markers = push_array_no_zero(scratch.arena, RADDBGIC_VMapMarker, marker_count);
    
    RADDBGIC_SortKey *key_ptr = keys;
    RADDBGIC_VMapMarker *marker_ptr = markers;
    
    for(RADDBGIC_Scope *node = root->first_scope;
        node != 0;
        node = node->next_order){
      RADDBGI_U32 scope_idx = node->idx;
      
      for(RADDBGIC_VOffRange *range = node->first_range;
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
    
    scope_vmap = raddbgic_vmap_from_markers(arena, markers, keys, marker_count);
  }
  
  // fill result
  RADDBGIC_SymbolData *result = raddbgic_push_array(arena, RADDBGIC_SymbolData, 1);
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
  
  scratch_end(scratch);
  ProfEnd();
  
  return result;
}

//- rjf: name map baking

RADDBGI_PROC RADDBGIC_NameMapBaked*
raddbgic_name_map_bake(RADDBGIC_Arena *arena, RADDBGIC_Root *root, RADDBGIC_BakeCtx *bctx, RADDBGIC_NameMap *map)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  RADDBGI_U32 bucket_count = map->name_count;
  RADDBGI_U32 node_count = map->name_count;
  
  // setup the final bucket layouts
  RADDBGIC_NameMapSemiBucket *sbuckets = raddbgic_push_array(scratch.arena, RADDBGIC_NameMapSemiBucket, bucket_count);
  for(RADDBGIC_NameMapNode *node = map->first;
      node != 0;
      node = node->order_next){
    RADDBGI_U64 hash = raddbgi_hash(node->string.str, node->string.size);
    RADDBGI_U64 bi = hash%bucket_count;
    RADDBGIC_NameMapSemiNode *snode = raddbgic_push_array(scratch.arena, RADDBGIC_NameMapSemiNode, 1);
    SLLQueuePush(sbuckets[bi].first, sbuckets[bi].last, snode);
    snode->node = node;
    sbuckets[bi].count += 1;
  }
  
  // allocate tables
  RADDBGI_NameMapBucket *buckets = raddbgic_push_array(arena, RADDBGI_NameMapBucket, bucket_count);
  RADDBGI_NameMapNode *nodes = push_array_no_zero(arena, RADDBGI_NameMapNode, node_count);
  
  // convert to serialized buckets & nodes
  {
    RADDBGI_NameMapBucket *bucket_ptr = buckets;
    RADDBGI_NameMapNode *node_ptr = nodes;
    for(RADDBGI_U32 i = 0; i < bucket_count; i += 1, bucket_ptr += 1){
      bucket_ptr->first_node = (RADDBGI_U32)(node_ptr - nodes);
      bucket_ptr->node_count = sbuckets[i].count;
      
      for(RADDBGIC_NameMapSemiNode *snode = sbuckets[i].first;
          snode != 0;
          snode = snode->next){
        RADDBGIC_NameMapNode *node = snode->node;
        
        // cons name and index(es)
        RADDBGI_U32 string_idx = raddbgic_string(bctx, node->string);
        RADDBGI_U32 match_count = node->idx_count;
        RADDBGI_U32 idx = 0;
        if(match_count == 1){
          idx = node->idx_first->idx[0];
        }
        else{
          Temp temp = temp_begin(scratch.arena);
          RADDBGI_U32 *idx_run = push_array_no_zero(temp.arena, RADDBGI_U32, match_count);
          RADDBGI_U32 *idx_ptr = idx_run;
          for(RADDBGIC_NameMapIdxNode *idxnode = node->idx_first;
              idxnode != 0;
              idxnode = idxnode->next){
            for(RADDBGI_U32 i = 0; i < ArrayCount(idxnode->idx); i += 1){
              if(idxnode->idx[i] == 0){
                goto dblbreak;
              }
              *idx_ptr = idxnode->idx[i];
              idx_ptr += 1;
            }
          }
          dblbreak:;
          Assert(idx_ptr == idx_run + match_count);
          idx = raddbgic_idx_run(bctx, idx_run, match_count);
          temp_end(temp);
        }
        
        // write to node
        node_ptr->string_idx = string_idx;
        node_ptr->match_count = match_count;
        node_ptr->match_idx_or_idx_run_first = idx;
        node_ptr += 1;
      }
    }
    Assert(node_ptr - nodes == node_count);
  }
  
  scratch_end(scratch);
  
  RADDBGIC_NameMapBaked *result = raddbgic_push_array(arena, RADDBGIC_NameMapBaked, 1);
  result->buckets = buckets;
  result->nodes = nodes;
  result->bucket_count = bucket_count;
  result->node_count = node_count;
  return result;
}

//- rjf: top-level baking entry point

RADDBGI_PROC void
raddbgic_bake_file(RADDBGIC_Arena *arena, RADDBGIC_Root *root, RADDBGIC_String8List *out)
{
  ProfBeginFunction();
  str8_serial_begin(arena, out);
  
  // setup cons helpers
  RADDBGIC_DSections dss = {0};
  raddbgic_dsection(arena, &dss, 0, 0, RADDBGI_DataSectionTag_NULL);
  
  RADDBGIC_BakeParams bctx_params = {0};
  {
    bctx_params.strings_bucket_count  = u64_up_to_pow2(root->symbol_map.pair_count*8);
    bctx_params.idx_runs_bucket_count = u64_up_to_pow2(root->symbol_map.pair_count*8);
  }
  RADDBGIC_BakeCtx *bctx = raddbgic_bake_ctx_begin(&bctx_params);
  
  ////////////////////////////////
  // MAIN PART: allocating and filling out sections of the file
  
  // top level info
  RADDBGI_TopLevelInfo *tli = raddbgic_push_array(arena, RADDBGI_TopLevelInfo, 1);
  {
    RADDBGIC_TopLevelInfo *raddbgic_tli = &root->top_level_info;
    tli->architecture = raddbgic_tli->architecture;
    tli->exe_name_string_idx = raddbgic_string(bctx, raddbgic_tli->exe_name);
    tli->exe_hash = raddbgic_tli->exe_hash;
    tli->voff_max = raddbgic_tli->voff_max;
  }
  raddbgic_dsection(arena, &dss, tli, sizeof(*tli), RADDBGI_DataSectionTag_TopLevelInfo);
  
  // binary sections array
  {
    RADDBGI_U32 count = root->binary_section_count;
    RADDBGI_BinarySection *sections = raddbgic_push_array(arena, RADDBGI_BinarySection, count);
    RADDBGI_BinarySection *dsec = sections;
    for(RADDBGIC_BinarySection *ssec = root->binary_section_first;
        ssec != 0;
        ssec = ssec->next, dsec += 1){
      dsec->name_string_idx = raddbgic_string(bctx, ssec->name);
      dsec->flags      = ssec->flags;
      dsec->voff_first = ssec->voff_first;
      dsec->voff_opl   = ssec->voff_opl;
      dsec->foff_first = ssec->foff_first;
      dsec->foff_opl   = ssec->foff_opl;
    }
    raddbgic_dsection(arena, &dss, sections, sizeof(*sections)*count, RADDBGI_DataSectionTag_BinarySections);
  }
  
  // units array
  // * pass for per-unit information including:
  // * top-level unit information
  // * combining line info for whole unit
  {
    RADDBGI_U32 count = root->unit_count;
    RADDBGI_Unit *units = raddbgic_push_array(arena, RADDBGI_Unit, count);
    RADDBGI_Unit *dunit = units;
    for(RADDBGIC_Unit *sunit = root->unit_first;
        sunit != 0;
        sunit = sunit->next_order, dunit += 1){
      // strings & paths
      RADDBGI_U32 unit_name = raddbgic_string(bctx, sunit->unit_name);
      RADDBGI_U32 cmp_name  = raddbgic_string(bctx, sunit->compiler_name);
      
      RADDBGI_U32 src_path     = raddbgic_paths_idx_from_path(bctx, sunit->source_file);
      RADDBGI_U32 obj_path     = raddbgic_paths_idx_from_path(bctx, sunit->object_file);
      RADDBGI_U32 archive_path = raddbgic_paths_idx_from_path(bctx, sunit->archive_file);
      RADDBGI_U32 build_path   = raddbgic_paths_idx_from_path(bctx, sunit->build_path);
      
      dunit->unit_name_string_idx     = unit_name;
      dunit->compiler_name_string_idx = cmp_name;
      dunit->source_file_path_node    = src_path;
      dunit->object_file_path_node    = obj_path;
      dunit->archive_file_path_node   = archive_path;
      dunit->build_path_node          = build_path;
      dunit->language                 = sunit->language;
      
      // line info (voff -> file*line*col)
      RADDBGIC_LineSequenceNode *first_seq = sunit->line_seq_first;
      RADDBGIC_UnitLinesCombined *lines = raddbgic_unit_combine_lines(arena, bctx, first_seq);
      
      RADDBGI_U32 line_count = lines->line_count;
      if(line_count > 0){
        dunit->line_info_voffs_data_idx =
          raddbgic_dsection(arena, &dss, lines->voffs, sizeof(RADDBGI_U64)*(line_count + 1),
                            RADDBGI_DataSectionTag_LineInfoVoffs);
        dunit->line_info_data_idx =
          raddbgic_dsection(arena, &dss, lines->lines, sizeof(RADDBGI_Line)*line_count,
                            RADDBGI_DataSectionTag_LineInfoData);
        if(lines->cols != 0){
          dunit->line_info_col_data_idx =
            raddbgic_dsection(arena, &dss, lines->cols, sizeof(RADDBGI_Column)*line_count,
                              RADDBGI_DataSectionTag_LineInfoColumns);
        }
        dunit->line_info_count = line_count;
      }
    }
    
    raddbgic_dsection(arena, &dss, units, sizeof(*units)*count, RADDBGI_DataSectionTag_Units);
  }
  
  // source file line info baking
  // * pass for "source_combine_line" for each source file -
  // * can only be run after a pass that does "unit_combine_lines" for each unit.
  for(RADDBGIC_SrcNode *src_node = bctx->tree->src_first;
      src_node != 0;
      src_node = src_node->next){
    RADDBGIC_LineMapFragment *first_fragment = src_node->first_fragment;
    RADDBGIC_SrcLinesCombined *lines = raddbgic_source_combine_lines(arena, first_fragment);
    RADDBGI_U32 line_count = lines->line_count;
    
    if(line_count > 0){
      src_node->line_map_count = line_count;
      
      src_node->line_map_nums_data_idx =
        raddbgic_dsection(arena, &dss, lines->line_nums, sizeof(*lines->line_nums)*line_count,
                          RADDBGI_DataSectionTag_LineMapNumbers);
      
      src_node->line_map_range_data_idx =
        raddbgic_dsection(arena, &dss, lines->line_ranges, sizeof(*lines->line_ranges)*(line_count + 1),
                          RADDBGI_DataSectionTag_LineMapRanges);
      
      src_node->line_map_voff_data_idx =
        raddbgic_dsection(arena, &dss, lines->voffs, sizeof(*lines->voffs)*lines->voff_count,
                          RADDBGI_DataSectionTag_LineMapVoffs);
    }
  }
  
  // source file name mapping
  {
    RADDBGIC_NameMap* map = raddbgic_name_map_for_kind(root, RADDBGI_NameMapKind_NormalSourcePaths);
    for(RADDBGIC_SrcNode *src_node = bctx->tree->src_first;
        src_node != 0;
        src_node = src_node->next){
      if(src_node->idx != 0){
        raddbgic_name_map_add_pair(root, map, src_node->normal_full_path, src_node->idx);
      }
    }
  }
  
  // unit vmap baking
  {
    RADDBGIC_VMap *vmap = raddbgic_vmap_from_unit_ranges(arena,
                                                         root->unit_vmap_range_first,
                                                         root->unit_vmap_range_count);
    
    RADDBGI_U64 vmap_size = sizeof(*vmap->vmap)*(vmap->count + 1);
    raddbgic_dsection(arena, &dss, vmap->vmap, vmap_size, RADDBGI_DataSectionTag_UnitVmap);
  }
  
  // type info baking
  {
    RADDBGIC_TypeData *types = raddbgic_type_data_combine(arena, root, bctx);
    
    RADDBGI_U64 type_nodes_size = sizeof(*types->type_nodes)*types->type_node_count;
    raddbgic_dsection(arena, &dss, types->type_nodes, type_nodes_size, RADDBGI_DataSectionTag_TypeNodes);
    
    RADDBGI_U64 udt_size = sizeof(*types->udts)*types->udt_count;
    raddbgic_dsection(arena, &dss, types->udts, udt_size, RADDBGI_DataSectionTag_UDTs);
    
    RADDBGI_U64 member_size = sizeof(*types->members)*types->member_count;
    raddbgic_dsection(arena, &dss, types->members, member_size, RADDBGI_DataSectionTag_Members);
    
    RADDBGI_U64 enum_member_size = sizeof(*types->enum_members)*types->enum_member_count;
    raddbgic_dsection(arena, &dss, types->enum_members, enum_member_size, RADDBGI_DataSectionTag_EnumMembers);
  }
  
  // symbol info baking
  {
    RADDBGIC_SymbolData *symbol_data = raddbgic_symbol_data_combine(arena, root, bctx);
    
    RADDBGI_U64 global_variables_size =
      sizeof(*symbol_data->global_variables)*symbol_data->global_variable_count;
    raddbgic_dsection(arena, &dss, symbol_data->global_variables, global_variables_size,
                      RADDBGI_DataSectionTag_GlobalVariables);
    
    RADDBGIC_VMap *global_vmap = symbol_data->global_vmap;
    RADDBGI_U64 global_vmap_size = sizeof(*global_vmap->vmap)*(global_vmap->count + 1);
    raddbgic_dsection(arena, &dss, global_vmap->vmap, global_vmap_size,
                      RADDBGI_DataSectionTag_GlobalVmap);
    
    RADDBGI_U64 thread_variables_size =
      sizeof(*symbol_data->thread_variables)*symbol_data->thread_variable_count;
    raddbgic_dsection(arena, &dss, symbol_data->thread_variables, thread_variables_size,
                      RADDBGI_DataSectionTag_ThreadVariables);
    
    RADDBGI_U64 procedures_size = sizeof(*symbol_data->procedures)*symbol_data->procedure_count;
    raddbgic_dsection(arena, &dss, symbol_data->procedures, procedures_size,
                      RADDBGI_DataSectionTag_Procedures);
    
    RADDBGI_U64 scopes_size = sizeof(*symbol_data->scopes)*symbol_data->scope_count;
    raddbgic_dsection(arena, &dss, symbol_data->scopes, scopes_size, RADDBGI_DataSectionTag_Scopes);
    
    RADDBGI_U64 scope_voffs_size = sizeof(*symbol_data->scope_voffs)*symbol_data->scope_voff_count;
    raddbgic_dsection(arena, &dss, symbol_data->scope_voffs, scope_voffs_size,
                      RADDBGI_DataSectionTag_ScopeVoffData);
    
    RADDBGIC_VMap *scope_vmap = symbol_data->scope_vmap;
    RADDBGI_U64 scope_vmap_size = sizeof(*scope_vmap->vmap)*(scope_vmap->count + 1);
    raddbgic_dsection(arena, &dss, scope_vmap->vmap, scope_vmap_size, RADDBGI_DataSectionTag_ScopeVmap);
    
    RADDBGI_U64 local_size = sizeof(*symbol_data->locals)*symbol_data->local_count;
    raddbgic_dsection(arena, &dss, symbol_data->locals, local_size, RADDBGI_DataSectionTag_Locals);
    
    RADDBGI_U64 location_blocks_size =
      sizeof(*symbol_data->location_blocks)*symbol_data->location_block_count;
    raddbgic_dsection(arena, &dss, symbol_data->location_blocks, location_blocks_size,
                      RADDBGI_DataSectionTag_LocationBlocks);
    
    RADDBGI_U64 location_data_size = symbol_data->location_data_size;
    raddbgic_dsection(arena, &dss, symbol_data->location_data, location_data_size,
                      RADDBGI_DataSectionTag_LocationData);
  }
  
  // name map baking
  {
    RADDBGI_U32 name_map_count = 0;
    for(RADDBGI_U32 i = 0; i < RADDBGI_NameMapKind_COUNT; i += 1){
      if(root->name_maps[i] != 0){
        name_map_count += 1;
      }
    }
    
    RADDBGI_NameMap *name_maps = raddbgic_push_array(arena, RADDBGI_NameMap, name_map_count);
    
    RADDBGI_NameMap *name_map_ptr = name_maps;
    for(RADDBGI_U32 i = 0; i < RADDBGI_NameMapKind_COUNT; i += 1){
      RADDBGIC_NameMap *map = root->name_maps[i];
      if(map != 0){
        RADDBGIC_NameMapBaked *baked = raddbgic_name_map_bake(arena, root, bctx, map);
        
        name_map_ptr->kind = i;
        name_map_ptr->bucket_data_idx =
          raddbgic_dsection(arena, &dss, baked->buckets, sizeof(*baked->buckets)*baked->bucket_count,
                            RADDBGI_DataSectionTag_NameMapBuckets);
        name_map_ptr->node_data_idx =
          raddbgic_dsection(arena, &dss, baked->nodes, sizeof(*baked->nodes)*baked->node_count,
                            RADDBGI_DataSectionTag_NameMapNodes);
        name_map_ptr += 1;
      }
    }
    
    raddbgic_dsection(arena, &dss, name_maps, sizeof(*name_maps)*name_map_count,
                      RADDBGI_DataSectionTag_NameMaps);
  }
  
  ////////////////////////////////
  // LATE PART: baking loose structures and creating final layout
  
  // generate data sections for file paths
  {
    RADDBGI_U32 count = bctx->tree->count;
    RADDBGI_FilePathNode *nodes = raddbgic_push_array(arena, RADDBGI_FilePathNode, count);
    
    RADDBGI_FilePathNode *out_node = nodes;
    for(RADDBGIC_PathNode *node = bctx->tree->first;
        node != 0;
        node = node->next_order, out_node += 1){
      out_node->name_string_idx = raddbgic_string(bctx, node->name);
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
    
    raddbgic_dsection(arena, &dss, nodes, sizeof(*nodes)*count, RADDBGI_DataSectionTag_FilePathNodes);
  }
  
  // generate data sections for files
  {
    RADDBGI_U32 count = bctx->tree->src_count;
    RADDBGI_SourceFile *src_files = raddbgic_push_array(arena, RADDBGI_SourceFile, count);
    
    RADDBGI_SourceFile *out_src_file = src_files;
    for(RADDBGIC_SrcNode *node = bctx->tree->src_first;
        node != 0;
        node = node->next, out_src_file += 1){
      out_src_file->file_path_node_idx = node->path_node->idx;
      out_src_file->normal_full_path_string_idx = raddbgic_string(bctx, node->normal_full_path);
      out_src_file->line_map_nums_data_idx = node->line_map_nums_data_idx;
      out_src_file->line_map_range_data_idx = node->line_map_range_data_idx;
      out_src_file->line_map_count = node->line_map_count;
      out_src_file->line_map_voff_data_idx = node->line_map_voff_data_idx;
    }
    
    raddbgic_dsection(arena, &dss, src_files, sizeof(*src_files)*count, RADDBGI_DataSectionTag_SourceFiles);
  }
  
  // generate data sections for strings
  {
    RADDBGI_U32 *str_offs = push_array_no_zero(arena, RADDBGI_U32, bctx->strs.count + 1);
    
    RADDBGI_U32 off_cursor = 0;
    {
      RADDBGI_U32 *off_ptr = str_offs;
      *off_ptr = 0;
      off_ptr += 1;
      for(RADDBGIC_StringNode *node = bctx->strs.order_first;
          node != 0;
          node = node->order_next){
        off_cursor += node->str.size;
        *off_ptr = off_cursor;
        off_ptr += 1;
      }
    }
    
    RADDBGI_U8 *buf = raddbgic_push_array(arena, RADDBGI_U8, off_cursor);
    {
      RADDBGI_U8 *ptr = buf;
      for(RADDBGIC_StringNode *node = bctx->strs.order_first;
          node != 0;
          node = node->order_next){
        MemoryCopy(ptr, node->str.str, node->str.size);
        ptr += node->str.size;
      }
    }
    
    raddbgic_dsection(arena, &dss, str_offs, sizeof(*str_offs)*(bctx->strs.count + 1),
                      RADDBGI_DataSectionTag_StringTable);
    raddbgic_dsection(arena, &dss, buf, off_cursor, RADDBGI_DataSectionTag_StringData);
  }
  
  // generate data sections for index runs
  {
    RADDBGI_U32 *idx_data = push_array_no_zero(arena, RADDBGI_U32, bctx->idxs.idx_count);
    
    {
      RADDBGI_U32 *out_ptr = idx_data;
      RADDBGI_U32 *opl = out_ptr + bctx->idxs.idx_count;
      RADDBGIC_IdxRunNode *node = bctx->idxs.order_first;
      for(;node != 0 && out_ptr < opl;
          node = node->order_next){
        MemoryCopy(out_ptr, node->idx_run, sizeof(*node->idx_run)*node->count);
        out_ptr += node->count;
      }
      Assert(out_ptr == opl);
    }
    
    raddbgic_dsection(arena, &dss, idx_data, sizeof(*idx_data)*bctx->idxs.idx_count,
                      RADDBGI_DataSectionTag_IndexRuns);
  }
  
  // layout
  // * the header and data section table have to be initialized "out of order"
  // * so that the rest of the system can avoid this tricky order-layout interdependence stuff
  RADDBGI_Header *header = raddbgic_push_array(arena, RADDBGI_Header, 1);
  RADDBGI_DataSection *dstable = raddbgic_push_array(arena, RADDBGI_DataSection, dss.count);
  str8_serial_push_align(arena, out, 8);
  RADDBGI_U64 header_off = out->total_size;
  str8_list_push(arena, out, str8_struct(header));
  str8_serial_push_align(arena, out, 8);
  RADDBGI_U64 data_section_off = out->total_size;
  str8_list_push(arena, out, str8((RADDBGI_U8 *)dstable, sizeof(*dstable)*dss.count));
  {
    header->magic = RADDBGI_MAGIC_CONSTANT;
    header->encoding_version = RADDBGI_ENCODING_VERSION;
    header->data_section_off = data_section_off;
    header->data_section_count = dss.count;
  }
  {
    RADDBGI_U64 test_dss_count = 0;
    for(RADDBGIC_DSectionNode *node = dss.first;
        node != 0;
        node = node->next){
      test_dss_count += 1;
    }
    Assert(test_dss_count == dss.count);
    
    RADDBGI_DataSection *ptr = dstable;
    for(RADDBGIC_DSectionNode *node = dss.first;
        node != 0;
        node = node->next, ptr += 1){
      RADDBGI_U64 data_section_offset = 0;
      if(node->size != 0)
      {
        str8_serial_push_align(arena, out, 8);
        data_section_offset = out->total_size;
        str8_list_push(arena, out, str8((RADDBGI_U8 *)node->data, node->size));
      }
      ptr->tag = node->tag;
      ptr->encoding = RADDBGI_DataSectionEncoding_Unpacked;
      ptr->off = data_section_offset;
      ptr->encoded_size = node->size;
      ptr->unpacked_size = node->size;
    }
    Assert(ptr == dstable + dss.count);
  }
  
  raddbgic_bake_ctx_release(bctx);
  ProfEnd();
}
