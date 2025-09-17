// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

// --- Input -------------------------------------------------------------------

typedef struct LNK_LibMemberRef
{
  LNK_Lib *lib;
  U32      member_idx;
  struct LNK_LibMemberRef *next;
} LNK_LibMemberRef;

typedef struct LNK_LibMemberRefList
{
  U64               count;
  LNK_LibMemberRef *first;
  LNK_LibMemberRef *last;
} LNK_LibMemberRefList;

typedef enum
{
  LNK_InputSource_CmdLine, // specified on command line
  LNK_InputSource_Default, // specified through defaultlib switch
  LNK_InputSource_Obj,     // refrenced from objects
  LNK_InputSource_Count
} LNK_InputSourceType;

typedef struct LNK_Input
{
  String8           path;
  String8           data;
  B32               disallow;
  B32               is_thin;
  B32               has_disk_read_failed;
  B32               exclude_from_debug_info;
  LNK_LibMemberRef *link_member;
  void             *loaded_input;

  struct LNK_Input *next;
} LNK_Input;

typedef struct LNK_InputList
{
  U64        count;
  LNK_Input *first;
  LNK_Input *last;
} LNK_InputList;

typedef struct LNK_InputPtrArray
{
  U64         count;
  LNK_Input **v;
} LNK_InputPtrArray;

typedef struct LNK_Inputer
{
  Arena *arena;

  LNK_InputList  objs;
  HashTable     *objs_ht;
  LNK_InputList  new_objs;

  HashTable     *libs_ht;
  HashTable     *missing_lib_ht;
  LNK_InputList  libs;
  LNK_InputList  new_libs[LNK_InputSource_Count];
} LNK_Inputer;

// --- Image Link -------------------------------------------------------------

#define LNK_IMPORT_STUB "*** RAD_IMPORT_STUB ***"
#define LNK_NULL_SYMBOL "*** RAD_NULL_SYMBOL ***"

#define LNK_SECTION_FLAG_LIVE    (1 << 0)
#define LNK_SECTION_FLAG_DEBUG (1 << 1)

typedef struct LNK_Link
{
  LNK_ObjList              objs;
  LNK_LibList              libs;
  LNK_ObjNode            **last_symbol_input;
  LNK_IncludeSymbolNode  **last_include;
  String8Node            **last_cmd_lib;
  String8Node            **last_default_lib;
  String8Node            **last_obj_lib;
  LNK_LibMemberRefList     imports;
  B32                      try_to_resolve_entry_point;
} LNK_Link;

// -- Image Layout ------------------------------------------------------------

#define LNK_REMOVED_SECTION_NUMBER_32 (U32)-3
#define LNK_REMOVED_SECTION_NUMBER_16 (U16)-3

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

// --- Ref ---------------------------------------------------------------------

#define PointerBitSize      64u
#define PointerFreeBitsSize 16u

#define PointerTagBitSize PointerFreeBitsSize
#define PointerTagMask    ((1ull << (PointerTagBitSize)) - 1u)
#define PointerTagShift   (PointerBitSize - PointerTagBitSize)

#define PackPointer(ptr, tag) (void *)(IntFromPtr(ptr) | (((tag) & PointerTagMask) << PointerTagShift))
#define UnpackPointerTag(ptr) ((IntFromPtr(ptr) >> PointerTagShift) & PointerTagMask)
#define UnpackPointer(ptr)    (void *)(IntFromPtr(ptr) & ~(PointerTagMask << PointerTagShift))
#define BumpPointerTag(ptr)   PackPointer(UnpackPointer(ptr), UnpackPointerTag(ptr) + 1)

#define LNK_RELOCS_PER_TASK 0x1000

typedef struct LNK_RelocRefs
{
  LNK_Obj         *obj;
  COFF_RelocArray  relocs;
} LNK_RelocRefs;

typedef struct LNK_RelocRefsNode
{
  LNK_RelocRefs *v;
  struct LNK_RelocRefsNode *next;
} LNK_RelocRefsNode;

typedef struct LNK_RelocRefsList
{
  LNK_RelocRefsNode *head;
} LNK_RelocRefsList;

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
  B32                   search_anti_deps;
  LNK_SymbolTable      *symtab;
  LNK_Lib              *lib;
  LNK_LibMemberRefList *member_ref_lists;
} LNK_SearchLibTask;

typedef struct
{
  LNK_SymbolTable  *symtab;
  U32               active_thread_count;
  LNK_RelocRefsList reloc_refs;
} LNK_OptRefTask;

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
internal String8 lnk_make_linker_obj(Arena *arena, LNK_Config *config);

// --- Inputer -----------------------------------------------------------------

internal void              lnk_input_list_push_node(LNK_InputList *list, LNK_Input *node);
internal void              lnk_input_list_concat_in_place(LNK_InputList *list, LNK_InputList *to_concat);
internal LNK_InputPtrArray lnk_array_from_input_list(Arena *arena, LNK_InputList list);

internal LNK_Inputer * lnk_inputer_init(void);

internal LNK_Input * lnk_input_push(Arena *arena, LNK_InputList *list, String8 path, String8 data);
internal LNK_Input * lnk_inputer_push_linkgen(Arena *arena, LNK_InputList *list, String8 path, String8 data);
internal LNK_Input * lnk_inputer_push_thin(Arena *arena, LNK_InputList *list, HashTable *ht, String8 full_path);

internal LNK_Input * lnk_inputer_push_obj(LNK_Inputer *inputer, LNK_LibMemberRef *link_member, String8 path, String8 data);
internal LNK_Input * lnk_inputer_push_obj_linkgen(LNK_Inputer *inputer, LNK_LibMemberRef *link_member, String8 path, String8 data);
internal LNK_Input * lnk_inputer_push_obj_thin(LNK_Inputer *inputer, LNK_LibMemberRef *link_member, String8 path);

internal LNK_Input * lnk_inputer_push_lib(LNK_Inputer *inputer, LNK_InputSourceType input_source, String8 path, String8 data);
internal LNK_Input * lnk_inputer_push_lib_linkgen(LNK_Inputer *inputer, LNK_InputSourceType input_source, String8 path, String8 data);
internal LNK_Input * lnk_inputer_push_lib_thin(LNK_Inputer *inputer, LNK_Config *config, LNK_InputSourceType input_source, String8 lib_path);

internal B32               lnk_inputer_has_items(LNK_Inputer *inputer);
internal LNK_InputPtrArray lnk_inputer_flush(Arena *arena, TP_Context *tp, LNK_Inputer *inputer, LNK_IO_Flags io_flags, LNK_InputList *all_inputs, LNK_InputList *new_inputs);

// --- Link Context ------------------------------------------------------------

internal void                lnk_lib_member_ref_list_push_node(LNK_LibMemberRefList *list, LNK_LibMemberRef *node);
internal void                lnk_lib_member_ref_list_concat_in_place_array(LNK_LibMemberRefList *list, LNK_LibMemberRefList *to_concat_arr, U64 count);
internal int                 lnk_lib_member_ref_is_before(void *raw_a, void *raw_b);
internal LNK_LibMemberRef ** lnk_array_from_lib_member_list(Arena *arena, LNK_LibMemberRefList list);

internal LNK_ObjNode * lnk_load_objs  (TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_Inputer *inputer, LNK_SymbolTable *symtab, LNK_Link *link, U64 *objs_count_out);
internal void          lnk_load_libs  (TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_Inputer *inputer, LNK_Link *link);
internal void          lnk_link_inputs(TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_Inputer *inputer, LNK_SymbolTable *symtab, LNK_Link *link);
internal LNK_Link *    lnk_link_image (TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_Inputer *inputer, LNK_SymbolTable *symtab);

// --- Optimizations -----------------------------------------------------------

internal void lnk_opt_ref(TP_Context *tp, LNK_SymbolTable *symtab, LNK_Config *config, LNK_ObjList objs);

// --- Win32 Image -------------------------------------------------------------

internal String8List      lnk_build_guard_tables(TP_Context *tp, LNK_SectionTable *sectab, LNK_SymbolTable *symtab, U64 objs_count, LNK_Obj **objs, COFF_MachineType machine, String8 entry_point_name, LNK_GuardFlags guard_flags, B32 emit_suppress_flag);
internal String8List      lnk_build_base_relocs(TP_Context *tp, TP_Arena *tp_temp, LNK_Config *config, U64 objs_count, LNK_Obj **objs);
internal String8List      lnk_build_win32_image_header(Arena *arena, LNK_SymbolTable *symtab, LNK_Config *config, LNK_SectionArray sect_arr, U64 expected_image_header_size);
internal LNK_ImageContext lnk_build_image(TP_Arena *arena, TP_Context *tp, LNK_Config *config, LNK_SymbolTable *symtab, U64 obj_count, LNK_Obj **objs);

// --- Logger ------------------------------------------------------------------

internal void lnk_log_link_stats(LNK_ObjList obj_list, LNK_LibList *lib_index, LNK_SectionTable *sectab);
internal void lnk_log_timers(void);

