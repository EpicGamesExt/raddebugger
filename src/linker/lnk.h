// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

// --- Link --------------------------------------------------------------------

typedef struct LNK_LinkContext
{
  LNK_SymbolTable   *symtab;
  U64                objs_count;
  LNK_Obj          **objs;
  LNK_LibList        lib_index[LNK_InputSource_Count];
} LNK_LinkContext;

// -- Image --------------------------------------------------------------------

#define LNK_IMPORT_STUB "*** RAD_IMPORT_STUB ***"
#define LNK_REMOVED_SECTION_NUMBER_32 (U32)-3
#define LNK_REMOVED_SECTION_NUMBER_16 (U16)-3

#define LNK_SECTION_FLAG_IS_LIVE (1 << 0)

typedef struct LNK_ImageContext
{
  String8           image_data;
  LNK_SectionTable *sectab;
} LNK_ImageContext;

typedef struct LNK_SectionDefinition
{
  String8           name;
  COFF_SectionFlags flags;
  U64               contribs_count;
  struct LNK_Obj   *obj;
  U64               obj_sect_idx;
} LNK_SectionDefinition;

typedef struct LNK_CommonBlockContrib
{
  struct LNK_Symbol *symbol;
  union {
    U32 size;
    U32 offset;
  } u;
} LNK_CommonBlockContrib;

// --- Base Reloc --------------------------------------------------------------

typedef struct LNK_BaseRelocPage
{
  U64     voff;
  U64List entries_addr32;
  U64List entries_addr64;
} LNK_BaseRelocPage;

typedef struct LNK_BaseRelocPageNode
{
  struct LNK_BaseRelocPageNode *next;
  LNK_BaseRelocPage             v;
} LNK_BaseRelocPageNode;

typedef struct LNK_BaseRelocPageList
{
  U64                    count;
  LNK_BaseRelocPageNode *first;
  LNK_BaseRelocPageNode *last;
} LNK_BaseRelocPageList;

typedef struct LNK_BaseRelocPageArray
{
  U64                count;
  LNK_BaseRelocPage *v;
} LNK_BaseRelocPageArray;

// --- Workers Contexts --------------------------------------------------------

typedef struct
{
  LNK_SymbolTable          *symtab;
  LNK_SymbolHashTrieChunk **chunks;
} LNK_ReplaceWeakSymbolsWithDefaultSymbolTask;

typedef struct
{
  LNK_SymbolTable           *symtab;
  LNK_SectionTable          *sectab;
  U64                        objs_count;
  LNK_Obj                  **objs;
  U64                        function_pad_min;
  U64                        default_align;
  LNK_SectionContrib        *null_sc;
  LNK_SectionContrib      ***sect_map;
  HashTable                 *contribs_ht;
  LNK_SectionArray           image_sects;
  union {
    struct {
      HashTable **defns;
    } gather_sects;
    struct {
      U64                    *counts;
      U64                    *offsets;
      LNK_CommonBlockContrib *contribs;
    } common_block;
    struct {
      LNK_SectionContribChunk **chunks;
    } sort_contribs;
    struct {
      B8                        **was_symbol_patched;
      LNK_Section                *common_block_sect;
      Rng1U64                    *common_block_ranges;
      LNK_CommonBlockContrib     *common_block_contribs;
      COFF_SymbolValueInterpType  fixup_type;
    } patch_symtabs;
  } u;
} LNK_BuildImageTask;

typedef struct
{
  U64                     page_size;
  Rng1U64                *range_arr;
  LNK_BaseRelocPageList  *list_arr;
  HashTable             **page_ht_arr;
  B32                     is_large_addr_aware;
} LNK_BaseRelocTask;

typedef struct
{
  Rng1U64                *ranges;
  U64                     page_size;
  LNK_BaseRelocPageList  *list_arr;
  LNK_Obj               **obj_arr;
  HashTable             **page_ht_arr;
  B32                     is_large_addr_aware;
} LNK_ObjBaseRelocTask;

typedef struct
{
  LNK_InputObjList    input_obj_list;
  U64                 input_imports_count;
  LNK_InputImport    *input_imports;
  LNK_InputImportList input_import_list;
  LNK_SymbolList      unresolved_symbol_list;
} LNK_SymbolFinderResult;

typedef struct
{
  PathStyle               path_style;
  LNK_SymbolTable        *symtab;
  LNK_SymbolNodeArray     lookup_node_arr;
  LNK_SymbolFinderResult *result_arr;
  Rng1U64                *range_arr;
} LNK_SymbolFinder;

typedef struct
{
  String8              image_data;
  LNK_Obj            **objs;
  U64                  image_base;
  COFF_SectionHeader **image_section_table;
} LNK_ObjRelocPatcher;

typedef struct
{
  String8 path;
  String8 temp_path;
  String8 data;
} LNK_WriteThreadContext;

typedef struct
{
  String8  data;
  Rng1U64 *ranges;
  U128    *hashes;
} LNK_Blake3Hasher;

typedef struct
{
  LNK_SymbolTable  *symtab;
  union {
    LNK_ObjNodeArray objs;
    LNK_LibNodeArray libs;
  } u;
} LNK_SymbolPusher;

// --- Config -----------------------------------------------------------------

internal LNK_Config * lnk_config_from_argcv(Arena *arena, int argc, char **argv);

// --- Entry Point -------------------------------------------------------------

internal void lnk_run(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config);

// --- Path --------------------------------------------------------------------

internal String8 lnk_make_full_path(Arena *arena, PathStyle system_path_style, String8 work_dir, String8 path);

// --- Hasher ------------------------------------------------------------------

internal U128 lnk_blake3_hash_parallel(TP_Context *tp, U64 chunk_count, String8 data);

// --- Manifest ----------------------------------------------------------------

internal String8 lnk_make_linker_manifest(Arena *arena, B32 manifest_uac, String8 manifest_level, String8 manifest_ui_access, String8List manifest_dependency_list);
internal void    lnk_merge_manifest_files(String8 mt_path, String8 out_name, String8List manifest_path_list);
internal String8 lnk_manifest_from_inputs(Arena *arena, LNK_IO_Flags io_flags, String8 mt_path, String8 manifest_name, B32 manifest_uac, String8 manifest_level, String8 manifest_ui_access, String8List input_manifest_path_list, String8List deps_list);

// --- Internal Objs -----------------------------------------------------------

internal String8 lnk_make_null_obj(Arena *arena);
internal String8 lnk_make_res_obj(Arena *arena, String8List res_file_list, String8List res_path_list, COFF_MachineType machine, U32 time_stamp, String8 work_dir, PathStyle system_path_style, String8 obj_name);
internal String8 lnk_make_linker_coff_obj(Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 cwd_path, String8 exe_path, String8 pdb_path, String8 cmd_line, String8 obj_name);

// --- Link Context ------------------------------------------------------------

internal String8 lnk_get_lib_name(String8 path);
internal B32     lnk_is_lib_disallowed(HashTable *disallow_lib_ht, String8 path);
internal B32     lnk_is_lib_loaded(HashTable *loaded_lib_ht, String8 lib_path);
internal void    lnk_push_disallow_lib(Arena *arena, HashTable *disallow_lib_ht, String8 path);
internal void    lnk_push_loaded_lib(Arena *arena, HashTable *loaded_lib_ht, String8 path);

internal LNK_InputObjList lnk_push_linker_symbols(Arena *arena, LNK_Config *config);
internal void             lnk_queue_lib_member_input(Arena *arena, LNK_Config *config, LNK_Symbol *symbol, LNK_InputImportList *input_import_list, LNK_InputObjList *input_obj_list);

internal LNK_LinkContext lnk_build_link_context(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config);

// --- Win32 Image -------------------------------------------------------------

internal String8List      lnk_build_guard_tables(TP_Context *tp, LNK_SectionTable *sectab, LNK_SymbolTable *symtab, U64 objs_count, LNK_Obj **objs, COFF_MachineType machine, String8 entry_point_name, LNK_GuardFlags guard_flags, B32 emit_suppress_flag);
internal String8List      lnk_build_base_relocs(TP_Context *tp, TP_Arena *tp_temp, LNK_Config *config, U64 objs_count, LNK_Obj **objs);
internal String8List      lnk_build_win32_image_header(Arena *arena, LNK_SymbolTable *symtab, LNK_Config *config, LNK_SectionArray sect_arr, U64 expected_image_header_size);
internal LNK_ImageContext lnk_build_image(TP_Arena *arena, TP_Context *tp, LNK_Config *config, LNK_SymbolTable *symtab, U64 obj_count, LNK_Obj **objs);

// --- Logger ------------------------------------------------------------------

internal void lnk_log_link_stats(LNK_ObjList obj_list, LNK_LibList *lib_index, LNK_SectionTable *sectab);
internal void lnk_log_timers(void);

