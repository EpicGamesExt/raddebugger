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
  U32                 *symbol_indices;
  String8Array         symbol_names;
  String8              long_names;
  U64                  input_idx;
  U32                  next_whole_member;
  
  struct LNK_NameTrieChunk **search_cursor_chunks;
  U64                             *search_cursor_indices;
  struct LNK_NameTrieChunk **anti_dep_search_cursor_chunks;
  U64                             *anti_dep_search_cursor_indices;
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
  U32     member_off_idx;
} LNK_FirstMemberSortKey;

typedef struct LNK_ExpandedLibMember LNK_ExpandedLibMember;
struct LNK_ExpandedLibMember
{
  LNK_ExpandedLibMember *next;

  LNK_Lib *lib;
  U64      member_offset;

  String8  name;
  String8  data;
  String8  path;
  B32      is_thin;
};

typedef struct
{
  LNK_ExpandedLibMember *first;
  LNK_ExpandedLibMember *last;
  U64                    count;
  String8                error;
} LNK_LibExpansion;

// --- Workers Contexts --------------------------------------------------------
 
typedef struct
{
  LNK_Config         *config;
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

internal B32              lnk_lib_from_data(Arena *arena, String8 data, String8 path, U64 input_idx, B32 whole_archive, LNK_Lib *lib_out);
internal LNK_Lib **       lnk_array_from_lib_list(Arena *arena, LNK_LibList list);
internal void             lnk_lib_list_push_node(LNK_LibList *list, LNK_LibNode *node);
internal LNK_LibNodeArray lnk_lib_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_LibList *list, U64 inputs_count, struct LNK_Input **inputs);

internal force_inline B32 lnk_search_lib(LNK_Lib *lib, String8 symbol_name, U32 *member_idx_out);

