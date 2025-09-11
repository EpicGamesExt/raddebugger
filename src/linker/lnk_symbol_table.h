// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

// --- Symbol ------------------------------------------------------------------

typedef struct LNK_ObjSymbolRef
{
  struct LNK_Obj *obj;
  U32             symbol_idx;
} LNK_ObjSymbolRef;

typedef struct LNK_ObjSymbolRefNode
{
  struct LNK_ObjSymbolRefNode *next;
  LNK_ObjSymbolRef             v;
} LNK_ObjSymbolRefNode;

typedef struct LNK_Symbol
{
  String8               name;
  B8                    is_lib_member_linked;
  LNK_ObjSymbolRefNode *refs;
} LNK_Symbol;

// --- Symbol Containers -------------------------------------------------------

typedef struct LNK_SymbolNode
{
  struct LNK_SymbolNode *next;
  LNK_Symbol            *data;
} LNK_SymbolNode;

typedef struct LNK_SymbolList
{
  U64             count;
  LNK_SymbolNode *first;
  LNK_SymbolNode *last;
} LNK_SymbolList;

typedef struct LNK_SymbolArray
{
  U64         count;
  LNK_Symbol *v;
} LNK_SymbolArray;

// --- Symbol Hash Trie --------------------------------------------------------

typedef struct LNK_SymbolHashTrie
{
  String8                   *name;
  LNK_Symbol                *symbol;
  struct LNK_SymbolHashTrie *child[4];
} LNK_SymbolHashTrie;

typedef struct LNK_SymbolHashTrieChunk
{
  struct LNK_SymbolHashTrieChunk *next;
  U64                             count;
  U64                             cap;
  LNK_SymbolHashTrie             *v;
} LNK_SymbolHashTrieChunk;

typedef struct LNK_SymbolHashTrieChunkList
{
  U64                      count;
  LNK_SymbolHashTrieChunk *first;
  LNK_SymbolHashTrieChunk *last;
} LNK_SymbolHashTrieChunkList;

// --- Symbol Table ------------------------------------------------------------

typedef struct LNK_SymbolTable
{
  TP_Arena                    *arena;
  LNK_SymbolHashTrie          *root;
  LNK_SymbolHashTrieChunkList *chunks;
  LNK_SymbolHashTrieChunkList *weak_undef_chunks;
} LNK_SymbolTable;

// --- Workers Contexts --------------------------------------------------------

typedef struct
{
  LNK_SymbolTable          *symtab;
  LNK_SymbolHashTrieChunk **chunks;
} LNK_ReplaceWeakSymbolsWithDefaultSymbolTask;

// --- Symbol -----------------------------------------------------------------

internal LNK_Symbol * lnk_make_symbol(Arena *arena, String8 name, struct LNK_Obj *obj, U32 symbol_idx);

internal int lnk_obj_symbol_ref_is_before(void *raw_a, void *raw_b);
internal int lnk_obj_symbol_ref_ptr_is_before(void *raw_a, void *raw_b);
internal int lnk_symbol_is_before(void *raw_a, void *raw_b);
internal int lnk_symbol_ptr_is_before(void *raw_a, void *raw_b);

// --- Symbol Containers ------------------------------------------------------

internal void             lnk_symbol_list_push_node(LNK_SymbolList *list, LNK_SymbolNode *node);
internal LNK_SymbolNode * lnk_symbol_list_push(Arena *arena, LNK_SymbolList *list, LNK_Symbol *symbol);

// --- Symbol Hash Trie --------------------------------------------------------

internal LNK_SymbolHashTrie *       lnk_symbol_hash_tire_chunk_list_push(Arena *arena, LNK_SymbolHashTrieChunkList *list, U64 cap);
internal void                       lnk_symbol_hash_trie_chunk_list_concat_in_place(LNK_SymbolHashTrieChunkList *list, LNK_SymbolHashTrieChunkList *to_concat);
internal void                       lnk_symbol_hash_trie_insert_or_replace(Arena *arena, LNK_SymbolHashTrieChunkList *chunks, LNK_SymbolHashTrie **trie, U64 hash, LNK_Symbol *symbol);
internal LNK_SymbolHashTrie *       lnk_symbol_hash_trie_search(LNK_SymbolHashTrie *trie, U64 hash, String8 name);
internal void                       lnk_symbol_hash_trie_remove(LNK_SymbolHashTrie *trie);
internal LNK_SymbolHashTrieChunk ** lnk_array_from_symbol_hash_trie_chunk_list(Arena *arena, LNK_SymbolHashTrieChunkList *lists, U64 lists_count, U64 *count_out);

// --- Symbol Helpers ----------------------------------------------------------

internal LNK_ObjSymbolRef           lnk_ref_from_symbol(LNK_Symbol *symbol);
internal U64                        lnk_ref_count_from_symbol(LNK_Symbol *symbol);
internal COFF_ParsedSymbol          lnk_parsed_from_symbol(LNK_Symbol *symbol);
internal COFF_SymbolValueInterpType lnk_interp_from_symbol(LNK_Symbol *symbol);

// --- Symbol Table ------------------------------------------------------------

internal U64 lnk_symbol_table_hasher(String8 string);

internal LNK_SymbolTable * lnk_symbol_table_init(TP_Arena *arena);
internal void              lnk_symbol_table_push(LNK_SymbolTable *symtab, LNK_Symbol *symbol);
internal LNK_Symbol *      lnk_symbol_table_search(LNK_SymbolTable *symtab, String8 name);
internal LNK_Symbol *      lnk_symbol_table_searchf(LNK_SymbolTable *symtab, char *fmt, ...);

// --- Symbol Contrib Helpers --------------------------------------------------

internal ISectOff lnk_sc_from_symbol(LNK_Symbol *symbol);
internal U64      lnk_voff_from_symbol(COFF_SectionHeader **image_section_table, LNK_Symbol *symbol);
internal U64      lnk_foff_from_symbol(COFF_SectionHeader **image_section_table, LNK_Symbol *symbol);

// --- Weak Symbol -------------------------------------------------------------

internal B32 lnk_resolve_weak_symbol(LNK_SymbolTable *symtab, LNK_ObjSymbolRef symbol, LNK_ObjSymbolRef *resolved_symbol_out);

internal void lnk_replace_weak_with_default_symbols(TP_Context *tp, LNK_SymbolTable *symtab);

