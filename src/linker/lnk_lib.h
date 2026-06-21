// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_Lib
{
  String8              path;
  String8              data;
  COFF_ArchiveType     type;
  U32                  member_count;
  U32                  symbol_count;
  U32                 *member_offsets;
  U16                 *symbol_indices;
  String8Array         symbol_names;
  String8              long_names;
  U64                  input_idx;

  // lib-search barrier elision: a re-search of this lib can only queue new members if the set of
  // undefined/weak symbols grew, or if anti-dep searching was just enabled, since the last search.
  // these record the state at the last search so identical re-searches can skip the tp dispatch.
  B32                  was_searched;
  B32                  searched_anti_deps;
  U64                  searched_symbol_count;

  // lib-search FRONTIER cursor: search_chunks is append-only across the entire lib-search loop
  // (it is only drained -> chunks AFTER the loop, in lnk_replace_weak_with_default_symbols), so an
  // already-searched (symbol,lib) pair can never resolve a new member -- searching is a pure
  // function of (lib, symbol->name) and the member queue dedup is idempotent. each worker records
  // the position in its search_chunks[worker] list where this lib's last search ended; the next
  // search rescans only the slots appended since. search_cursor_chunk[worker] == 0 means "start at
  // list first" (nothing searched yet). reset to 0 when the anti-dep mode flips (anti-dep weak
  // symbols must be re-tried under the new mode).
  struct LNK_SymbolHashTrieChunk **search_cursor_chunk; // [worker_count], 0 = unsearched
  U64                             *search_cursor_idx;    // [worker_count], intra-chunk resume index
} LNK_Lib;
 
typedef struct LNK_LibNode
{
  LNK_Lib             data;
  struct LNK_LibNode *next;
} LNK_LibNode;

typedef struct LNK_LibNodeArray
{
  U64           count;
  LNK_LibNode **v;
} LNK_LibNodeArray;

typedef struct LNK_LibList
{
  U64          count;
  LNK_LibNode *first;
  LNK_LibNode *last;
} LNK_LibList;

typedef struct LNK_FirstMemberSortKey
{
  String8 symbol_name;
  U16     member_off_idx;
} LNK_FirstMemberSortKey;

// --- Workers Contexts --------------------------------------------------------
 
typedef struct
{
  struct LNK_Input  **inputs;
  U64                 lib_id_base;
  U64                 next_free_lib_idx;
  U64                 valid_libs_count;
  U64                 invalid_libs_count;
  LNK_LibNode        *free_libs;
  LNK_LibNode       **valid_libs;
  LNK_LibNode       **invalid_libs;
} LNK_LibIniter;

// -----------------------------------------------------------------------------

internal int lnk_lib_node_is_before(void *a, void *b);
internal int lnk_lib_node_ptr_is_before(void *raw_a, void *raw_b);

internal B32              lnk_lib_from_data(Arena *arena, String8 data, String8 path, U64 input_idx, LNK_Lib *lib_out);
internal LNK_Lib **       lnk_array_from_lib_list(Arena *arena, LNK_LibList list);
internal void             lnk_lib_list_push_node(LNK_LibList *list, LNK_LibNode *node);
internal LNK_LibNodeArray lnk_lib_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_LibList *list, U64 inputs_count, struct LNK_Input **inputs);

internal force_inline B32 lnk_search_lib(LNK_Lib *lib, String8 symbol_name, U32 *member_idx_out);

