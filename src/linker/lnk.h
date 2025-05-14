// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

#define LNK_NULL_SYMBOL_NAME                        "NULL"
#define LNK_TEXT_SYMBOL_NAME                        "TEXT"
#define LNK_DATA_SYMBOL_NAME                        "DATA"
#define LNK_RDATA_SYMBOL_NAME                       "RDATA"
#define LNK_BSS_SYMBOL_NAME                         "BSS"
#define LNK_XDATA_SYMBOL_NAME                       "XDATA"
#define LNK_PDATA_SYMBOL_NAME                       "PDATA"
#define LNK_BASE_RELOC_SYMBOL_NAME                  "BASE_RELOC"
#define LNK_EDATA_SYMBOL_NAME                       "EDATA"
#define LNK_DEBUG_DIR_SYMBOL_NAME                   "DEDIR"
#define LNK_DEBUG_DATA_SYMBOL_NAME                  "DEDAT"
#define LNK_CV_DIR_SYMBOL_NAME                      "CV_DIR"
#define LNK_CV_HEADER_PDB70_SYMBOL_NAME             "CV_HEADER_PDB70"
#define LNK_CV_HEADER_RDI_SYMBOL_NAME               "CV_HEADER_RDI"
#define LNK_CV_HEADER_GUID_SYMBOL_NAME              "CV_HEADER_GUID"
#define LNK_RSRC_SYMBOL_NAME                        "RSRC"
#define LNK_DEBUG_SYMBOL_NAME                       "DEBUG"
#define LNK_GFIDS_SYMBOL_NAME                       "GFIDS"
#define LNK_GIATS_SYMBOL_NAME                       "GIATS"
#define LNK_GLJMP_SYMBOL_NAME                       "GLJMP"
#define LNK_GEHCONT_SYMBOL_NAME                     "GEHCONT"
#define LNK_IMPORT_NAME_TABLE_SYMBOL_NAME           "IMPORT_STR"
#define LNK_IMPORT_DLL_TABLE_SYMBOL_NAME            "IDATA"
#define LNK_IMPORT_ILT_SYMBOL_NAME                  "ILT"
#define LNK_IMPORT_IAT_SYMBOL_NAME                  "IAT"
#define LNK_IMPORT_JMP_SYMBOL_NAME                  "IMPORT_THUNKS"
#define LNK_DELAYED_IMPORT_DLL_TABLE_SYMBOL_NAME    "DELAYED_IMPORT_DLL_TABLE"
#define LNK_DELAYED_IMPORT_HANDLE_TABLE_SYMBOL_NAME "DELAYED_IMPORT_HANDLE_TABLE"
#define LNK_DELAYED_IMPORT_INT_SYMBOL_NAME          "DELAYED_IMPORT_INT"
#define LNK_DELAYED_IMPORT_IAT_SYMBOL_NAME          "DELAYED_IMPORT_IAT"
#define LNK_DELAYED_IMPORT_ILT_SYMBOL_NAME          "DELAYED_IMPORT_ILT"
#define LNK_DELAYED_IMPORT_BIAT_SYMBOL_NAME         "DELAYED_IMPORT_BIAT"
#define LNK_DELAYED_IMPORT_UIAT_SYMBOL_NAME         "DELAYED_IMPORT_UIAT"
#define LNK_DELAYED_IMPORT_CODE_SYMBOL_NAME         "DELAYED_IMPORT_CODE"

#define LNK_WIN32_HEADER_SYMBOL_NAME                  "WIN32_HEADER"
#define LNK_DOS_SYMBOL_NAME                           "DOS"
#define LNK_NT_HEADERS_SYMBOL_NAME                    "NT_HEADERS"
#define LNK_PE_MAGIC_CONTAINER_SYMBOL_NAME            "PE_MAGIC_CONTAINER"
#define LNK_COFF_FILE_HEADER_CONTAINER_SYMBOL_NAME    "COFF_FILE_HEADER_CONTAINER"
#define LNK_PE_OPT_HEADER_CONTAINER_SYMBOL_NAME       "PE_OPTIONAL_HEADER_CONTAINER"
#define LNK_COFF_SECTION_HEADER_CONTAINER_SYMBOL_NAME "COFF_SECTION_HEADER_CONTAINER"

#define LNK_DOS_HEADER_SYMBOL_NAME                  "DOS_HEADER"
#define LNK_DOS_PROGRAM_SYMBOL_NAME                 "DOS_PROGRAM"
#define LNK_PE_MAGIC_SYMBOL_NAME                    "PE_MAGIC"
#define LNK_COFF_FILE_HEADER_SYMBOL_NAME            "COFF_FILE_HEADER"
#define LNK_PE_DIRECTORY_ARRAY_SYMBOL_NAME          "PE_DIRECTORY_ARRAY"
#define LNK_PE_DIRECTORY_COUNT_SYMBOL_NAME          "PE_DIRECTORY_COUNT"
#define LNK_PE_OPT_HEADER_SYMBOL_NAME               "PE_OPTIONAL_HEADER"
#define LNK_COFF_SECT_HEADER_ARRAY_SYMBOL_NAME      "COFF_SECT_HEADER_ARRAY"
#define LNK_COFF_SECT_HEADER_COUNT_SYMBOL_NAME      "COFF_SECT_HEADER_COUNT"
#define LNK_PE_CHECKSUM_SYMBOL_NAME                 "PE_CHECKSUM"

// _tls_used is a special section in CRT which has format of 
// PE_TLSHeader32 or PE_TLSHeader64, according to machine type.
#define LNK_TLS_SYMBOL_NAME "_tls_used"

// _load_config_used points to SYMS_PeLoadConfig32/SYMS_PeLoadConfig64
// and symbols below are used to patch patricual fields of the struct.
#define LNK_LOAD_CONFIG_SYMBOL_NAME         "_load_config_used"
#define LNK_ENCLAVE_CONFIG_SYMBOL_NAME      "__enclave_config"
#define LNK_GUARD_FLAGS_SYMBOL_NAME         "__guard_flags"
#define LNK_GUARD_FIDS_TABLE_SYMBOL_NAME    "__guard_fids_table"
#define LNK_GUARD_FIDS_COUNT_SYMBOL_NAME    "__guard_fids_count"
#define LNK_GUARD_IAT_TABLE_SYMBOL_NAME     "__guard_iat_table"
#define LNK_GUARD_IAT_COUNT_SYMBOL_NAME     "__guard_iat_count"
#define LNK_GUARD_LONGJMP_TABLE_SYMBOL_NAME "__guard_longjmp_table"
#define LNK_GUARD_LONGJMP_COUNT_SYMBOL_NAME "__guard_longjmp_count"
#define LNK_GUARD_EHCONT_TABLE_SYMBOL_NAME  "__guard_eh_cont_table"
#define LNK_GUARD_EHCONT_COUNT_SYMBOL_NAME  "__guard_eh_cont_count"
// x86 load config fields
#define LNK_SAFE_SE_HANDLER_TABLE_SYMBOL_NAME "__safe_se_handler_table"
#define LNK_SAFE_SE_HANDLER_COUNT_SYMBOL_NAME "__safe_se_handler_count"

// load symbols from delayimp.lib
#define LNK_DELAY_LOAD_HELPER2_SYMBOL_NAME     "__delayLoadHelper2"
#define LNK_DELAY_LOAD_HELPER2_X86_SYMBOL_NAME "___delayLoadHelper2@8"

#define LNK_TEXT_SECTION_FLAGS      (COFF_SectionFlag_CntCode|COFF_SectionFlag_MemExecute|COFF_SectionFlag_MemRead)
#define LNK_DATA_SECTION_FLAGS      (COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite)
#define LNK_RDATA_SECTION_FLAGS     (COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead)
#define LNK_BSS_SECTION_FLAGS       (COFF_SectionFlag_CntUninitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite)
#define LNK_IDATA_SECTION_FLAGS     LNK_DATA_SECTION_FLAGS
#define LNK_DEBUG_DIR_SECTION_FLAGS LNK_DATA_SECTION_FLAGS
#define LNK_RSRC_SECTION_FLAGS      LNK_DATA_SECTION_FLAGS
#define LNK_XDATA_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_PDATA_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_EDATA_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_GFIDS_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_GIATS_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_GLJMP_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_GEHCONT_SECTION_FLAGS   LNK_RDATA_SECTION_FLAGS
#define LNK_RELOC_SECTION_FLAGS     (LNK_RDATA_SECTION_FLAGS | COFF_SectionFlag_MemDiscardable)
#define LNK_DEBUG_SECTION_FLAGS     (LNK_RDATA_SECTION_FLAGS | COFF_SectionFlag_MemDiscardable)

////////////////////////////////

typedef enum
{
  LNK_InputSource_CmdLine, // specified on command line
  LNK_InputSource_Default, // specified through defaultlib switch
  LNK_InputSource_Obj,     // refrenced from objects
  LNK_InputSource_Count
} LNK_InputSourceType;

typedef String8Node LNK_InputLib;
typedef String8List LNK_InputLibList;

typedef struct LNK_InputImport
{
  COFF_ParsedArchiveImportHeader       import_header;
  struct LNK_InputImport *next;
} LNK_InputImport;

typedef struct LNK_InputImportList
{
  U64              count;
  LNK_InputImport *first;
  LNK_InputImport *last;
} LNK_InputImportList;

////////////////////////////////

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

typedef struct
{
  U64                     page_size;
  LNK_Section           **sect_id_map;
  LNK_Reloc             **reloc_arr;
  Rng1U64                *range_arr;
  LNK_BaseRelocPageList  *list_arr;
  HashTable             **page_ht_arr;
  B32                     is_large_addr_aware;
} LNK_BaseRelocTask;

typedef struct
{
  Rng1U64                *ranges;
  U64                     page_size;
  LNK_Section           **sect_id_map;
  LNK_BaseRelocPageList  *list_arr;
  LNK_Obj               **obj_arr;
  HashTable             **page_ht_arr;
  B32                     is_large_addr_aware;
} LNK_ObjBaseRelocTask;

typedef struct
{
  LNK_InputObjList    input_obj_list;
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
  LNK_SymbolTable  *symtab;
  union {
    LNK_ObjNodeArray objs;
    LNK_LibNodeArray libs;
  } u;
} LNK_SymbolPusher;

typedef struct
{
  String8            image_data;
  LNK_SymbolTable   *symtab;
  LNK_SectionTable  *sectab;
  LNK_Section      **sect_id_map;
  U64                base_addr;
  LNK_Section      **sect_arr;
  Rng1U64           *range_arr;
} LNK_SectionRelocPatcher;

typedef struct
{
  String8            image_data;
  LNK_SymbolTable   *symtab;
  LNK_SectionTable  *sectab;
  LNK_Section      **sect_id_map;
  U64                base_addr;
  LNK_Obj          **obj_arr;
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

////////////////////////////////

internal LNK_InputImport *   lnk_input_import_list_push(Arena *arena, LNK_InputImportList *list);
internal void                lnk_input_import_list_concat_in_place(LNK_InputImportList *list, LNK_InputImportList *to_concat);
internal LNK_InputImport **  lnk_input_import_arr_from_list(Arena *arena, LNK_InputImportList list);
internal LNK_InputImportList lnk_list_from_input_import_arr(LNK_InputImport **arr, U64 count);

////////////////////////////////
// Helpers

internal String8 lnk_make_full_path(Arena *arena, String8 work_dir, PathStyle system_path_style, String8 path);

internal String8 lnk_get_lib_name(String8 path);
internal B32     lnk_is_lib_disallowed(HashTable *disallow_lib_ht, String8 path);
internal B32     lnk_is_lib_loaded(HashTable *loaded_lib_ht, String8 lib_path);
internal void    lnk_push_disallow_lib(Arena *arena, HashTable *disallow_lib_ht, String8 path);
internal void    lnk_push_loaded_lib(Arena *arena, HashTable *loaded_lib_ht, String8 path);

////////////////////////////////
// Manifest

internal String8 lnk_make_linker_manifest(Arena *arena, B32 manifest_uac, String8 manifest_level, String8 manifest_ui_access, String8List manifest_dependency_list);
internal void    lnk_merge_manifest_files(String8 mt_path, String8 out_name, String8List manifest_path_list);

////////////////////////////////
// Resources

internal void    lnk_serialize_pe_resource_tree(LNK_SectionTable *sectab, LNK_SymbolTable *symtab, PE_ResourceDir *root_dir);
internal void    lnk_add_resource_debug_s(LNK_SectionTable *sectab, LNK_SymbolTable *symtab, String8 obj_path, String8 cwd_path, String8 exe_path, CV_Arch arch, String8List res_file_list, MD5Hash *res_hash_array);
internal String8 lnk_make_res_obj(TP_Context *tp, Arena *arena, PE_ResourceDir *root_dir, COFF_MachineType machine, COFF_TimeStamp time_stamp, String8 path, String8 cwd_path, String8 exe_path, String8List res_file_list, MD5Hash *res_hash_array);
internal String8 lnk_obj_from_res_file_list(TP_Context *tp, Arena *arena, LNK_SectionTable *sectab, LNK_SymbolTable *symtab, String8List res_file_list, String8List res_path_list, COFF_MachineType machine, U32 time_stamp, String8 work_dir, PathStyle system_path_style, String8 obj_name); 

////////////////////////////////
// Debug

internal String8 lnk_make_linker_coff_obj(TP_Context *tp, Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 cwd_path, String8 exe_path, String8 pdb_path, String8 cmd_line, String8 obj_name);

////////////////////////////////
// Win32 Image Helpers

internal void        lnk_build_debug_pdb(LNK_SectionTable *sectab, LNK_SymbolTable *symtab, LNK_Section *debug_sect, LNK_Chunk *debug_dir_array_chunk, COFF_TimeStamp time_stamp, Guid guid, U32 age, String8 pdb_path);
internal void        lnk_build_debug_rdi(LNK_SectionTable *sectab, LNK_SymbolTable *symtab, LNK_Section *debug_sect, LNK_Chunk *debug_dir_array_chunk, COFF_TimeStamp time_stamp, Guid guid, String8 rdi_path);
internal void        lnk_build_guard_tables(TP_Context *tp, LNK_SectionTable *sectab, LNK_SymbolTable *symtab, LNK_ExportTable *exptab, LNK_ObjList obj_list, COFF_MachineType machine, String8 entry_point_name, LNK_GuardFlags guard_flags, B32 emit_suppress_flag);
internal void        lnk_build_base_relocs(TP_Context *tp, TP_Arena *tp_arena, LNK_SectionTable *sectab, LNK_SymbolTable *symtab, COFF_MachineType  machine, U64 page_size, PE_ImageFileCharacteristics file_chars, LNK_ObjList obj_list);
internal LNK_Chunk * lnk_build_dos_header(LNK_SymbolTable *symtab, LNK_Section *header_sect, LNK_Chunk *parent_chunk);
internal LNK_Chunk * lnk_build_pe_magic(LNK_SymbolTable *symtab, LNK_Section *header_sect, LNK_Chunk *parent);
internal LNK_Chunk * lnk_build_coff_file_header(LNK_SymbolTable *symtab, LNK_Section *header_sect, LNK_Chunk *parent, COFF_MachineType machine, COFF_TimeStamp time_stamp, PE_ImageFileCharacteristics file_characteristics);
internal LNK_Chunk * lnk_build_pe_optional_header_x64(LNK_SymbolTable *symtab, LNK_Section *header_sect, LNK_Chunk *parent, COFF_MachineType machine, U64 base_addr, U64 sect_align, U64 file_align, Version linker_ver, Version os_ver, Version image_ver, Version subsystem_ver, PE_WindowsSubsystem subsystem, PE_DllCharacteristics dll_characteristics, U64 stack_reserve, U64 stack_commit, U64 heap_reserve, U64 heap_commit, String8 entry_point_name, LNK_SectionArray sect_arr);
internal LNK_Chunk * lnk_build_pe_directories(LNK_SymbolTable *symtab, LNK_Section *header_sect, LNK_Chunk *parent);
internal LNK_Chunk * lnk_build_coff_section_table(LNK_SymbolTable *symtab, LNK_Section *header_sect, LNK_Chunk *parent_chunk, LNK_SectionArray sect_arr);
internal LNK_Chunk * lnk_build_win32_image_header(LNK_SymbolTable *symtab, LNK_Section *header_sect, LNK_Chunk *parent_chunk, LNK_Config *config, LNK_SectionArray sect_arr);

////////////////////////////////
// Relocs

internal void lnk_patch_relocs_linker(TP_Context *tp, LNK_SymbolTable *symtab, LNK_SectionTable *sectab, LNK_Section **sect_id_map, String8 image_data, U64 base_addr);
internal void lnk_patch_relocs_obj(TP_Context *tp, LNK_ObjList obj_list, LNK_SymbolTable *symtab, LNK_SectionTable *sectab, LNK_Section **sect_id_map, String8 image_data, U64 base_addr);

internal void lnk_apply_reloc(U64 base_addr, U64 virt_align, U64 file_align, LNK_Section **sect_id_map, LNK_SymbolTable *symtab, String8 chunk_data, LNK_Reloc *reloc);

////////////////////////////////

internal void lnk_log_size_breakdown(LNK_SectionTable *sectab, LNK_SymbolTable *symtab);
internal void lnk_log_link_stats(LNK_ObjList obj_list, LNK_LibList *lib_index, LNK_SectionTable *sectab);
internal void lnk_log_timers(void);

////////////////////////////////
// Enum <-> String

internal String8 lnk_string_from_input_source(LNK_InputSourceType input_source);

