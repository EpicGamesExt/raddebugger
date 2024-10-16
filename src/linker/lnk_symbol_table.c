// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

global read_only LNK_Symbol   g_null_symbol     = { str8_lit_comp("NULL"), LNK_Symbol_DefinedStatic };
global read_only LNK_Symbol  *g_null_symbol_ptr = &g_null_symbol;

internal void
lnk_init_symbol(LNK_Symbol *symbol, String8 name, LNK_SymbolType type)
{
  symbol->name = name;
  symbol->type = type;
}

internal void
lnk_init_defined_symbol(LNK_Symbol *symbol, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags)
{
  switch (visibility) {
  case LNK_DefinedSymbolVisibility_Static:   lnk_init_symbol(symbol, name, LNK_Symbol_DefinedStatic);   break;
  case LNK_DefinedSymbolVisibility_Extern:   lnk_init_symbol(symbol, name, LNK_Symbol_DefinedExtern);   break;
  case LNK_DefinedSymbolVisibility_Internal: lnk_init_symbol(symbol, name, LNK_Symbol_DefinedInternal); break;
  }
  LNK_DefinedSymbol *def = &symbol->u.defined;
  def->flags             = flags;
  def->value_type        = LNK_DefinedSymbolValue_Null;
}

internal void
lnk_init_defined_symbol_chunk(LNK_Symbol *symbol, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags, LNK_Chunk *chunk, U64 offset, COFF_ComdatSelectType selection, U32 check_sum)
{
  lnk_init_defined_symbol(symbol, name, visibility, flags);
  LNK_DefinedSymbol *def = &symbol->u.defined;
  def->value_type        = LNK_DefinedSymbolValue_Chunk;
  def->u.chunk           = chunk;
  def->u.chunk_offset    = offset;
  def->u.check_sum       = check_sum;
  def->u.selection       = selection;
}

internal void
lnk_init_defined_symbol_va(LNK_Symbol *symbol, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags, U64 va)
{
  lnk_init_defined_symbol(symbol, name, visibility, flags);
  LNK_DefinedSymbol *def = &symbol->u.defined;
  def->value_type        = LNK_DefinedSymbolValue_VA;
  def->u.va              = va;
}

internal void
lnk_init_undefined_symbol(LNK_Symbol *symbol, String8 name, LNK_SymbolScopeFlags scope_flags)
{
  lnk_init_symbol(symbol, name, LNK_Symbol_Undefined);
  symbol->u.undefined.scope_flags = scope_flags;
}

internal void
lnk_init_weak_symbol(LNK_Symbol *symbol, String8 name, COFF_WeakExtType lookup, LNK_Symbol *fallback)
{
  lnk_init_symbol(symbol, name, LNK_Symbol_Weak);
  symbol->u.weak.scope_flags     = LNK_SymbolScopeFlag_Defined;
  symbol->u.weak.lookup_type     = lookup;
  symbol->u.weak.fallback_symbol = fallback;
}

internal void
lnk_init_lazy_symbol(LNK_Symbol *symbol, String8 name, LNK_Lib *lib, U64 member_offset)
{
  lnk_init_symbol(symbol, name, LNK_Symbol_Lazy);
  symbol->u.lazy.lib           = lib;
  symbol->u.lazy.member_offset = member_offset;
}

internal LNK_Symbol *
lnk_make_defined_symbol(Arena *arena, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_defined_symbol(symbol, name, visibility, flags);
  return symbol;
}

internal LNK_Symbol * 
lnk_make_defined_symbol_chunk(Arena *arena, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags, LNK_Chunk *chunk, U64 offset, COFF_ComdatSelectType selection, U32 check_sum)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_defined_symbol_chunk(symbol, name, visibility, flags, chunk, offset, selection, check_sum);
  return symbol;
}

internal LNK_Symbol *
lnk_make_defined_symbol_va(Arena *arena, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags, U64 va)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_defined_symbol_va(symbol, name, visibility, flags, va);
  return symbol;
}

internal LNK_Symbol * 
lnk_make_undefined_symbol(Arena *arena, String8 name, LNK_SymbolScopeFlags flags)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_undefined_symbol(symbol, name, flags);
  return symbol;
}

internal LNK_Symbol *
lnk_make_weak_symbol(Arena *arena, String8 name, COFF_WeakExtType lookup, LNK_Symbol *fallback)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_weak_symbol(symbol, name, lookup, fallback);
  return symbol;
}

internal LNK_Symbol *
lnk_make_lazy_symbol(Arena *arena, String8 name, LNK_Lib *lib, U64 member_offset)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_lazy_symbol(symbol, name, lib, member_offset);
  return symbol;
}

internal LNK_Chunk *
lnk_defined_symbol_get_chunk(LNK_DefinedSymbol *symbol)
{
  if (symbol->value_type == LNK_DefinedSymbolValue_Chunk) {
    return symbol->u.chunk;
  }
  return 0;
}

internal void
lnk_symbol_list_push_node(LNK_SymbolList *list, LNK_SymbolNode *node)
{
  DLLPushBack(list->first, list->last, node);
  list->count += 1;
}

internal LNK_SymbolNode *
lnk_symbol_list_push(Arena *arena, LNK_SymbolList *list, LNK_Symbol *symbol)
{
  LNK_SymbolNode *node = push_array(arena, LNK_SymbolNode, 1);
  node->data = symbol;
  lnk_symbol_list_push_node(list, node);
  return node;
}

internal void
lnk_symbol_list_push_list(LNK_SymbolList *list, LNK_SymbolList *to_push)
{
  if (to_push->count) {
    if (list->count) {
      list->last->next      = to_push->first;
      to_push->first->prev  = list->last;
      list->last            = to_push->last;
      list->count          += to_push->count;
    } else {
      *list = *to_push;
    }
    MemoryZeroStruct(to_push);
  }
}

internal void
lnk_symbol_list_insert_after(LNK_SymbolList *list, LNK_SymbolNode *node, LNK_SymbolNode *insert)
{
  DLLInsert(list->first, list->last, node, insert);
  list->count += 1;
}

internal LNK_SymbolNode *
lnk_symbol_list_pop_node(LNK_SymbolList *list)
{
  LNK_SymbolNode *node = 0;
  if (list->count) {
    node = list->first;
    DLLRemove(list->first, list->last, node);
    node->next = 0;
    node->prev = 0;
    list->count -= 1;
  }
  return node;
}

internal LNK_Symbol *
lnk_symbol_list_pop(LNK_SymbolList *list)
{
  LNK_SymbolNode *node = lnk_symbol_list_pop_node(list);
  return node ? node->data : 0;
}

internal void
lnk_symbol_list_remove(LNK_SymbolList *list, LNK_SymbolNode *node)
{
  Assert(list->count > 0);

  list->count -= 1;
  DLLRemove(list->first, list->last, node);

  node->next = 0;
  node->prev = 0;
}

internal void
lnk_symbol_list_concat_in_place(LNK_SymbolList *list, LNK_SymbolList *to_concat)
{
  DLLConcatInPlace(list, to_concat);
}

internal LNK_SymbolList
lnk_symbol_list_copy(Arena *arena, LNK_SymbolList list)
{
  LNK_SymbolList result = {0};
  LNK_SymbolNode *node_arr = push_array_no_zero(arena, LNK_SymbolNode, list.count);
  for (LNK_SymbolNode *i = list.first; i != 0; i = i->next) {
    Assert(result.count < list.count);
    LNK_SymbolNode *n = &node_arr[result.count++];
    n->data = i->data;
    SLLQueuePush(result.first, result.last, n);
  }
  return result;
}

internal LNK_SymbolNode *
lnk_symbol_list_search_node(LNK_SymbolList list, String8 name, StringMatchFlags flags)
{
  for (LNK_SymbolNode *node = list.first; node != 0; node = node->next) {
    if (str8_match(node->data->name, name, flags)) {
      return node;
    }
  }
  return 0;
}

internal LNK_Symbol *
lnk_symbol_list_search(LNK_SymbolList list, String8 name, StringMatchFlags flags)
{
  LNK_SymbolNode *node = lnk_symbol_list_search_node(list, name, flags);
  return node ? node->data : 0;
}

internal LNK_SymbolList
lnk_symbol_list_from_array(Arena *arena, LNK_SymbolArray arr)
{
  LNK_SymbolList list = {0};
  LNK_SymbolNode *node_arr = push_array_no_zero(arena, LNK_SymbolNode, arr.count);
  for (U64 i = 0; i < arr.count; i += 1) {
    LNK_SymbolNode *node = &node_arr[i];
    node->prev = node->next = 0;
    node->data = &arr.v[i];
    lnk_symbol_list_push_node(&list, node);
  }
  return list;
}

internal LNK_SymbolNodeArray
lnk_symbol_node_array_from_list(Arena *arena, LNK_SymbolList list)
{
  LNK_SymbolNodeArray result = {0};
  result.count               = 0;
  result.v                   = push_array_no_zero(arena, LNK_SymbolNode *, list.count);
  for (LNK_SymbolNode *i = list.first; i != 0; i = i->next, ++result.count) {
    result.v[result.count] = i;
  }
  return result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_symbol_node_ptr_hasher)
{
  LNK_SymbolNodePtrHasher *hasher = raw_task;
  Rng1U64                  range  = hasher->range_arr[task_id];
  for (U64 symbol_idx = range.min; symbol_idx < range.max; symbol_idx += 1) {
    LNK_SymbolNode *symbol_node = hasher->input_arr[symbol_idx];
    symbol_node->hash = lnk_symbol_table_hash(symbol_node->data->name);
  }
}

internal void
lnk_symbol_node_ptr_array_hash(TP_Context *tp, LNK_SymbolNode **arr, U64 count)
{
  Temp scratch = scratch_begin(0, 0);
  LNK_SymbolNodePtrHasher hasher = {0};
  hasher.input_arr               = arr;
  hasher.range_arr               = tp_divide_work(scratch.arena, count, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_symbol_node_ptr_hasher, &hasher);
  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_symbol_node_hasher)
{
  LNK_SymbolNodeHasher *hasher = raw_task;
  Rng1U64               range  = hasher->range_arr[task_id];
  for (U64 symbol_idx = range.min; symbol_idx < range.max; symbol_idx += 1) {
    LNK_SymbolNode *symbol_node = &hasher->input_arr[symbol_idx];
    symbol_node->hash = lnk_symbol_table_hash(symbol_node->data->name);
  }
}

internal void
lnk_symbol_node_array_hash(TP_Context *tp, LNK_SymbolNode *arr, U64 count)
{
  Temp scratch = scratch_begin(0, 0);
  LNK_SymbolNodeHasher hasher = {0};
  hasher.input_arr            = arr;
  hasher.range_arr            = tp_divide_work(scratch.arena, count, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_symbol_node_hasher, &hasher);
  scratch_end(scratch);
}

internal LNK_SymbolArray
lnk_symbol_array_from_list(Arena *arena, LNK_SymbolList list)
{
  LNK_SymbolArray arr = {0};
  arr.count           = 0;
  arr.v               = push_array_no_zero(arena, LNK_Symbol, list.count);
  for (LNK_SymbolNode *node = list.first; node != 0; node = node->next) {
    arr.v[arr.count++] = *node->data;
  }
  return arr;
}

internal LNK_Symbol *
lnk_symbol_array_search(LNK_SymbolArray symarr, String8 name, StringMatchFlags flags)
{
  for (U64 isym = 0; isym < symarr.count; ++isym) {
    LNK_Symbol *sym = &symarr.v[isym];
    if (str8_match(sym->name, name, flags)) {
      return sym;
    }
  }
  return 0;
}

internal
THREAD_POOL_TASK_FUNC(lnk_symbol_name_hasher)
{
  LNK_SymbolNameHasher *task  = raw_task;
  Rng1U64               range = task->range_arr[task_id];
  for (U64 symbol_idx = range.min; symbol_idx < range.max; symbol_idx += 1) {
    LNK_Symbol *symbol = &task->symbol_arr[symbol_idx];
    task->hash_arr[symbol_idx] = lnk_symbol_table_hash(symbol->name);
  }
}

internal U64 *
lnk_symbol_array_hash(TP_Context *tp, Arena *arena, LNK_Symbol *arr, U64 count)
{
  Temp scratch = scratch_begin(&arena, 1);

  U64      stride    = CeilIntegerDiv(count, tp->worker_count);
  Rng1U64 *range_arr = push_array_no_zero(scratch.arena, Rng1U64, tp->worker_count); 
  for (U64 thread_idx = 0; thread_idx < tp->worker_count; thread_idx += 1) {
    Rng1U64 *range = &range_arr[thread_idx];
    range->min = Min(count, stride * thread_idx);
    range->max = Min(count, range->min + stride);
  }

  LNK_SymbolNameHasher hasher_ctx = {0};
  hasher_ctx.symbol_arr           = arr;
  hasher_ctx.range_arr            = range_arr;
  hasher_ctx.hash_arr             = push_array_no_zero(arena, U64, count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_symbol_name_hasher, &hasher_ctx);

  scratch_end(scratch);
  return hasher_ctx.hash_arr;
}

internal LNK_SymbolTable *
lnk_symbol_table_alloc(void)
{
  return lnk_symbol_table_alloc_ex(0x1000, 0x100, 0x500, 0x1000);
}

internal LNK_SymbolTable *
lnk_symbol_table_alloc_ex(U64 defined_cap, U64 internal_cap, U64 weak_cap, U64 lib_cap)
{
  ProfBeginDynamic("Alloc Symbol Table [Defined: 0x%llx, Internal: 0x%llx, Weak: 0x%llx, Lib: 0x%llx]", defined_cap, internal_cap, weak_cap, lib_cap);
  Arena *arena = arena_alloc();
  LNK_SymbolTable *symtab                             = push_array(arena, LNK_SymbolTable, 1);
  symtab->arena                                       = arena;
  symtab->bucket_count[LNK_SymbolScopeIndex_Defined]  = defined_cap;
  symtab->bucket_count[LNK_SymbolScopeIndex_Internal] = internal_cap;
  symtab->bucket_count[LNK_SymbolScopeIndex_Weak]     = weak_cap;
  symtab->bucket_count[LNK_SymbolScopeIndex_Lib]      = lib_cap;
  for (U64 iscope = 0; iscope < ArrayCount(symtab->buckets); ++iscope) {
    symtab->buckets[iscope] = push_array(symtab->arena, LNK_SymbolList, symtab->bucket_count[iscope]);
  }
  ProfEnd();
  return symtab;
}

internal void
lnk_symbol_table_release(LNK_SymbolTable **symtab)
{
  ProfBeginFunction();
  arena_release((*symtab)->arena);
  *symtab = 0;
  ProfEnd();
}

internal U64
lnk_symbol_table_hash(String8 string)
{
  return hash_from_str8(string);
}

internal LNK_SymbolNode *
lnk_symbol_table_search_bucket(LNK_SymbolTable *symtab, LNK_SymbolScopeIndex scope_idx, U64 bucket_idx, String8 name, U64 hash)
{
  for (LNK_SymbolNode *node = symtab->buckets[scope_idx][bucket_idx].first; node != 0; node = node->next) {
    if (hash == node->hash && str8_match(node->data->name, name, 0)) {
      return node;
    }
  }
  return 0;
}

internal LNK_SymbolNode *
lnk_symbol_table_search_node_hash(LNK_SymbolTable *symtab, LNK_SymbolScopeFlags scope_flags, String8 name, U64 hash)
{
  while (scope_flags) {
    LNK_SymbolScopeIndex scope_idx = ctz64(scope_flags);
    scope_flags &= scope_flags - 1;
    U64 bucket_idx = hash % symtab->bucket_count[scope_idx];
    LNK_SymbolNode *node = lnk_symbol_table_search_bucket(symtab, scope_idx, bucket_idx, name, hash);
    if (node) return node;
  }
  return 0;
}

internal LNK_SymbolNode *
lnk_symbol_table_search_node(LNK_SymbolTable *symtab, LNK_SymbolScopeFlags scope_flags, String8 name)
{
  U64 hash = lnk_symbol_table_hash(name);
  return lnk_symbol_table_search_node_hash(symtab, scope_flags, name, hash);
}

internal LNK_Symbol *
lnk_symbol_table_search(LNK_SymbolTable *symtab, LNK_SymbolScopeFlags scope_flags, String8 name)
{
  LNK_SymbolNode *node = lnk_symbol_table_search_node(symtab, scope_flags, name);
  return node ? node->data : 0;
}

internal LNK_Symbol *
lnk_symbol_table_searchf(LNK_SymbolTable *symtab, LNK_SymbolScopeFlags scope_flags, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  
  va_list args;
  va_start(args, fmt);
  String8 name = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  
  LNK_Symbol *symbol = lnk_symbol_table_search(symtab, scope_flags, name);
  scratch_end(scratch);
  return symbol;
}

internal void
lnk_symbol_table_remove(LNK_SymbolTable *symtab, LNK_SymbolScopeIndex scope, String8 name)
{
  U64 hash = lnk_symbol_table_hash(name);
  U64 ibucket = hash % symtab->bucket_count[scope];
  for (;;) {
    LNK_SymbolNode *node = lnk_symbol_table_search_bucket(symtab, scope, ibucket, name, hash);
    if (!node) {
      break;
    }
    LNK_SymbolList *bucket = &symtab->buckets[scope][ibucket];
    DLLRemove(bucket->first, bucket->last, node);
    bucket->count -= 1;
  }
}

internal LNK_SymbolList *
lnk_symbol_table_bucket_from_hash(LNK_SymbolTable *symtab, LNK_SymbolScopeIndex scope_idx, U64 hash)
{
  U64 bucket_idx = hash % symtab->bucket_count[scope_idx];
  LNK_SymbolList *bucket = &symtab->buckets[scope_idx][bucket_idx];
  return bucket;
}

internal void
lnk_symbol_table_push_(LNK_SymbolTable *symtab, LNK_SymbolScopeIndex scope_idx, LNK_SymbolNode *node, U64 hash)
{
  LNK_SymbolList *bucket = lnk_symbol_table_bucket_from_hash(symtab, scope_idx, hash);
  node->hash = hash;
  lnk_symbol_list_push_node(bucket, node);
}

internal void
lnk_symbol_table_push_node_hash(LNK_SymbolTable *symtab, LNK_SymbolNode *node, U64 hash)
{
  switch (node->data->type) {
  case LNK_Symbol_Null: break;

  case LNK_Symbol_DefinedExtern: {
    lnk_symbol_table_push_(symtab, LNK_SymbolScopeIndex_Defined, node, hash);
  } break;
  case LNK_Symbol_DefinedInternal: {
    lnk_symbol_table_push_(symtab, LNK_SymbolScopeIndex_Internal, node, hash);
  } break;
  case LNK_Symbol_Weak: {
    LNK_SymbolNode *is_strong_defn_present = lnk_symbol_table_search_node(symtab, LNK_SymbolScopeFlag_Defined, node->data->name);
    if (is_strong_defn_present) {
      break;
    }

    LNK_SymbolNode *is_weak_present = lnk_symbol_table_search_node(symtab, LNK_SymbolScopeFlag_Weak, node->data->name);
    if (is_weak_present) {
      B32 is_fallback_same = str8_match(is_weak_present->data->u.weak.fallback_symbol->name, node->data->u.weak.fallback_symbol->name, 0);
      if (!is_fallback_same) {
        lnk_error(LNK_Error_MultiplyDefinedSymbol, "Weak symbol %S conflict detected, symbol defined in:", node->data->name);
        lnk_supplement_error("%S", node->data->debug);
        lnk_supplement_error("%S", is_weak_present->data->debug);
      }
      break;
    }

    lnk_symbol_table_push_(symtab, LNK_SymbolScopeIndex_Weak, node, hash);
  } break;
  case LNK_Symbol_Lazy: {
    lnk_symbol_table_push_(symtab, LNK_SymbolScopeIndex_Lib, node, hash);
  } break;

  // symbols not supported
  case LNK_Symbol_Undefined:
  case LNK_Symbol_DefinedStatic: {
    InvalidPath;
  } break;
  }
}

internal void
lnk_symbol_table_push_node(LNK_SymbolTable *symtab, LNK_SymbolNode *node)
{
  U64 hash = lnk_symbol_table_hash(node->data->name);
  lnk_symbol_table_push_node_hash(symtab, node, hash);
}

internal LNK_SymbolNode *
lnk_symbol_table_push(LNK_SymbolTable *symtab, LNK_Symbol *symbol)
{
  LNK_SymbolNode *node = push_array(symtab->arena, LNK_SymbolNode, 1);
  node->data = symbol;
  lnk_symbol_table_push_node(symtab, node);
  return node;
}

internal
THREAD_POOL_TASK_FUNC(lnk_lazy_symbol_inserter)
{
  LNK_LazySymbolInserter *task   = raw_task;
  LNK_SymbolTable        *symtab = task->symtab;
  Rng1U64                 range  = task->range_arr[task_id];
  for (U64 bucket_idx = range.min; bucket_idx < range.max; bucket_idx += 1) {
    LNK_SymbolList *bucket = &task->bucket_arr[bucket_idx];
    for (LNK_SymbolNode *curr = bucket->first, *next; curr != 0; curr = next) {
      next = curr->next;
      lnk_symbol_table_push_(symtab, LNK_SymbolScopeIndex_Lib, curr, curr->hash);
    }
  }
}

internal void
lnk_symbol_table_push_lazy_arr(TP_Context *tp, LNK_SymbolTable *symtab, LNK_Symbol *arr, U64 count)
{
  Temp scratch = scratch_begin(0,0);

  ProfBegin("Push Symbol Nodes");
  LNK_SymbolNode *node_arr = push_array_no_zero(symtab->arena, LNK_SymbolNode, count);
  for (U64 symbol_idx = 0; symbol_idx < count; symbol_idx += 1) {
    LNK_SymbolNode *node = &node_arr[symbol_idx];
    node->prev = node->next = 0;
    node->data = &arr[symbol_idx];
  }
  ProfEnd();

  ProfBegin("Hash Symbol Names");
  lnk_symbol_node_array_hash(tp, node_arr, count);
  ProfEnd();

  ProfBegin("Populate Buckets");
  LNK_SymbolList *bucket_arr = push_array(scratch.arena, LNK_SymbolList, symtab->bucket_count[LNK_SymbolScopeIndex_Lib]);
  for (U64 symbol_idx = 0; symbol_idx < count; symbol_idx += 1) {
    LNK_SymbolNode *symbol_node = &node_arr[symbol_idx];
    U64 bucket_idx = symbol_node->hash % symtab->bucket_count[LNK_SymbolScopeIndex_Lib];
    lnk_symbol_list_push_node(&bucket_arr[bucket_idx], symbol_node);
  }
  ProfEnd();

  ProfBegin("Update Symbol Table");
  LNK_LazySymbolInserter symbol_inserter;
  symbol_inserter.symtab     = symtab;
  symbol_inserter.bucket_arr = bucket_arr;
  symbol_inserter.range_arr  = tp_divide_work(scratch.arena, symtab->bucket_count[LNK_SymbolScopeIndex_Lib], tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_lazy_symbol_inserter, &symbol_inserter);
  ProfEnd();

  scratch_end(scratch);
}

internal void
lnk_symbol_table_push_list(LNK_SymbolTable *symtab, LNK_SymbolList *list)
{
  ProfBeginFunction();
  MemoryZeroStruct(list);
  ProfEnd();
}

internal LNK_Symbol *
lnk_resolve_symbol(LNK_SymbolTable *symtab, LNK_Symbol *resolve_symbol)
{
  LNK_Symbol *symbol = resolve_symbol;
  B32 run_resolver;
  do {
    run_resolver = 0;
    switch (symbol->type) {
    case LNK_Symbol_Null: break;
    case LNK_Symbol_Undefined: {
      LNK_UndefinedSymbol *undef_symbol = &symbol->u.undefined;
      LNK_Symbol *def = lnk_symbol_table_search(symtab, undef_symbol->scope_flags, symbol->name);
      if (def) {
        symbol = def;
        run_resolver = 1;
      }
    } break;
    case LNK_Symbol_Weak: {
      LNK_WeakSymbol *weak = &symbol->u.weak;
      LNK_Symbol *def = lnk_symbol_table_search(symtab, weak->scope_flags, symbol->name);
      if (def) {
        Assert(LNK_Symbol_IsDefined(def->type));
        symbol = def;
      } else {
        symbol = symbol->u.weak.fallback_symbol;
      } 
      run_resolver = 1;
    } break;
    case LNK_Symbol_DefinedStatic:
    case LNK_Symbol_DefinedExtern:
    case LNK_Symbol_DefinedInternal: { 
      /* resolved */
    } break;
    default: NotImplemented;
    }
  } while (run_resolver);
  return symbol;
}

internal LNK_SymbolList
lnk_pop_comdat_chain(LNK_SymbolList *bucket, LNK_SymbolNode **cursor)
{
  LNK_SymbolList chain_list = {0};

  LNK_SymbolNode *leader_node = *cursor;
  *cursor = (*cursor)->next;

  lnk_symbol_list_remove(bucket, leader_node);
  lnk_symbol_list_push_node(&chain_list, leader_node);

  while (*cursor) {
    LNK_SymbolNode *next = (*cursor)->next;

    // symbols with identical names are stored in order
    if (!str8_match(leader_node->data->name, (*cursor)->data->name, 0)) {
      break;
    }

    // move node to chain list
    lnk_symbol_list_remove(bucket, *cursor);
    lnk_symbol_list_push_node(&chain_list, *cursor);

    // advance
    *cursor = next;
  }

  return chain_list;
}

internal LNK_SymbolNode *
lnk_fold_comdat_chain(LNK_SymbolList chain_list)
{
  LNK_SymbolNode *lead_node = chain_list.first;

  if (LNK_Symbol_IsDefined(lead_node->data->type)) {
    LNK_Symbol *lead = lead_node->data;
    if (lead->u.defined.value_type != LNK_DefinedSymbolValue_Chunk && chain_list.count > 1) {
      lnk_error(LNK_Error_MultiplyDefinedSymbol, "Unable to perfrom COMDAT fold on symbol %S, symbol must reference a section, defined in %S",
                lead->name, lead->debug);
      return 0;
    }
  }

  for (LNK_SymbolNode *curr_node = lead_node->next; curr_node != 0; curr_node = curr_node->next) {
    Assert(LNK_Symbol_IsDefined(lead_node->data->type));
    Assert(LNK_Symbol_IsDefined(curr_node->data->type));

    LNK_DefinedSymbol *lead_defined = &lead_node->data->u.defined;
    LNK_DefinedSymbol *curr_defined = &curr_node->data->u.defined;

    if (curr_defined->value_type != LNK_DefinedSymbolValue_Chunk) {
      lnk_error(LNK_Error_MultiplyDefinedSymbol, "Unable to perfrom COMDAT fold on symbol %S, symbol must reference a section, defined in %S",
                curr_node->data->name, curr_node->data->debug);
      return 0;
    }
    
    // There is no mentioning of this rule in PE spec, but according to comment from lld-link in 'handleComdatSelection':
    // "cl.exe picks "any" for vftabels when building with /GR- and "largest" when building /GR.". However,
    // chromium links '__src_ucrt_dll_is_in_use' from MSVCRT which is not a vftable but still requires selection override.
    if ((curr_defined->u.selection == COFF_ComdatSelectType_ANY && lead_defined->u.selection == COFF_ComdatSelectType_LARGEST) ||
        (curr_defined->u.selection == COFF_ComdatSelectType_LARGEST && lead_defined->u.selection == COFF_ComdatSelectType_ANY)) {
      lead_defined->u.selection = COFF_ComdatSelectType_LARGEST;
      curr_defined->u.selection = COFF_ComdatSelectType_LARGEST;
    }
    
    // COMDATs must have same selection rule
    if (lead_defined->u.selection != curr_defined->u.selection) {
      String8 curr_selection_str = coff_string_from_comdat_select_type(curr_defined->u.selection); 
      String8 lead_selection_str = coff_string_from_comdat_select_type(lead_defined->u.selection);
      lnk_error(LNK_Warning_UnresolvedComdat,
                "COMDAT selection conflict detected in symbol %S defined in %S (%S), leader selection %S from %S", 
                curr_node->data->name, curr_node->data->debug, curr_selection_str, lead_selection_str, lead_node->data->debug);
      return 0;
    }
    
    switch (curr_defined->u.selection) {
    case COFF_ComdatSelectType_NULL:
    case COFF_ComdatSelectType_ANY: {
      // both COMDATs are valid but to get smaller exe pick smallest
      LNK_Chunk *lead_chunk = lead_defined->u.chunk;
      LNK_Chunk *curr_chunk = curr_defined->u.chunk;
      U64 lead_chunk_size = lnk_chunk_get_size(lead_chunk);
      U64 curr_chunk_size = lnk_chunk_get_size(curr_chunk);
      if (curr_chunk_size < lead_chunk_size) {
        lead_node = curr_node;
      }
    } break;
    case COFF_ComdatSelectType_NODUPLICATES: {
      lnk_error(LNK_Error_MultiplyDefinedSymbol, "%S: error: multiply defined symbol %S in %S.", curr_node->data->debug, curr_node->data->name, lead_node->data->debug);
    } break;
    case COFF_ComdatSelectType_SAME_SIZE: {
      LNK_Chunk *lead_chunk = lead_defined->u.chunk;
      LNK_Chunk *curr_chunk = curr_defined->u.chunk;
      U64 lead_chunk_size = lnk_chunk_get_size(lead_chunk);
      U64 curr_chunk_size = lnk_chunk_get_size(curr_chunk);
      B32 is_same_size = (lead_chunk_size == curr_chunk_size);
      if (!is_same_size) {
        lnk_error(LNK_Error_MultiplyDefinedSymbol, "%S: error: multiply defined symbol %S in %S.", curr_node->data->debug, curr_node->data->name, lead_node->data->debug);
      }
    } break;
    case COFF_ComdatSelectType_EXACT_MATCH: {
      B32 is_exact_match = (lead_defined->u.check_sum == curr_defined->u.check_sum);
      if (!is_exact_match) {
        lnk_error(LNK_Error_MultiplyDefinedSymbol, "%S: error: multiply defined symbol %S in %S.", curr_node->data->debug, curr_node->data->name, lead_node->data->debug);
      }
    } break;
    case COFF_ComdatSelectType_LARGEST: {
      LNK_Chunk *lead_chunk = lead_defined->u.chunk;
      LNK_Chunk *curr_chunk = curr_defined->u.chunk;
      U64 lead_chunk_size = lnk_chunk_get_size(lead_chunk);
      U64 curr_chunk_size = lnk_chunk_get_size(curr_chunk);
      if (lead_chunk_size > curr_chunk_size) {
        lead_node = curr_node;
      }
    } break;
    case COFF_ComdatSelectType_ASSOCIATIVE: {
      // ignore
    } break;
    }
  }
  
  // rewire chunks so they point to COMDAT leader
  for (LNK_SymbolNode *curr_node = chain_list.first; curr_node != 0; curr_node = curr_node->next) {
    if (curr_node == lead_node) {
      continue;
    }

    LNK_DefinedSymbol *curr_defined = &curr_node->data->u.defined;
    LNK_Chunk         *curr_chunk   = curr_defined->u.chunk;

    // copy offset because after folding COMDATS we might end
    // up with larger sized chunk and, for instance, a vftable
    // might have a function pointer preceeding lead symbol
    curr_defined->u.chunk = lead_node->data->u.defined.u.chunk;
    curr_defined->u.chunk_offset = lead_node->data->u.defined.u.chunk_offset;
    
    // discard chunk from output
    curr_chunk->is_discarded = 1;
    
    // static symbols that are not part of obj's symbol table might point to discarded chunk
    curr_chunk->ref = lead_node->data->u.defined.u.chunk->ref;
  }

  return lead_node;
}

internal
THREAD_POOL_TASK_FUNC(lnk_comdat_folder)
{
  LNK_ComdatFolder *task   = raw_task;
  LNK_SymbolTable  *symtab = task->symtab;
  Rng1U64           range  = task->range_arr[task_id];
  for (U64 bucket_idx = range.min; bucket_idx < range.max; ++bucket_idx) {
    LNK_SymbolList *bucket      = &symtab->buckets[LNK_SymbolScopeIndex_Defined][bucket_idx];
    LNK_SymbolList  leader_list = {0};
    LNK_SymbolNode *curr        = bucket->first;
    while (curr) {
      LNK_SymbolList  chain_list  = lnk_pop_comdat_chain(bucket, &curr);
      LNK_SymbolNode *leader_node = lnk_fold_comdat_chain(chain_list);
      if (leader_node) {
        lnk_symbol_list_push_node(&leader_list, leader_node);
      }
    }
    Assert(bucket->count == 0);
    *bucket = leader_list;
  }
}

internal void
lnk_fold_comdat_chunks(TP_Context *tp, LNK_SymbolTable *symtab)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);

  LNK_ComdatFolder folder = {0};
  folder.symtab           = symtab;
  folder.range_arr        = tp_divide_work(scratch.arena, symtab->bucket_count[LNK_SymbolScopeIndex_Defined], tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_comdat_folder, &folder);

  scratch_end(scratch);
  ProfEnd();
}
