// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal U64
fzy_hash_from_string(String8 string, StringMatchFlags match_flags)
{
  return 0;
}

internal U64
fzy_item_num_from_array_element_idx__linear_search(FZY_ItemArray *array, U64 element_idx)
{
  return 0;
}

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
fzy_init(void)
{
  
}

////////////////////////////////
//~ rjf: Scope Functions

internal FZY_Scope *
fzy_scope_open(void)
{
  
}

internal void
fzy_scope_close(FZY_Scope *scope)
{
  
}

internal void
fzy_scope_touch_node__stripe_mutex_r_guarded(FZY_Scope *scope, FZY_Node *node)
{
  
}

////////////////////////////////
//~ rjf: Cache Lookup Functions

internal FZY_ItemArray
fzy_items_from_key_params_query(FZY_Scope *scope, U128 key, FZY_Params *params, String8 query, U64 endt_us, B32 *stale_out)
{
  
}

////////////////////////////////
//~ rjf: Searcher Threads

internal B32
fzy_u2s_enqueue_req(U128 key, U64 endt_us)
{
  
}

internal void
fzy_u2s_dequeue_req(Arena *arena, FZY_Thread *thread, U128 *key_out)
{
  
}

internal int
fzy_qsort_compare_items(FZY_Item *a, FZY_Item *b)
{
  
}

internal void
fzy_search_thread__entry_point(void *p)
{
  
}
