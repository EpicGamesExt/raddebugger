// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////

typedef struct LNK_InputObj
{
  struct LNK_InputObj *next;
  B32                  is_thin;
  B32                  has_disk_read_failed;
  String8              dedup_id;
  String8              path;
  String8              data;
  String8              lib_path;
} LNK_InputObj;

typedef struct LNK_InputObjList
{
  U64           count;
  LNK_InputObj *first;
  LNK_InputObj *last;
} LNK_InputObjList;

////////////////////////////////

typedef struct LNK_Directive
{
  struct LNK_Directive *next;
  String8               id;
  String8List           value_list;
} LNK_Directive;

typedef struct LNK_DirectiveList
{
  U64            count;
  LNK_Directive *first;
  LNK_Directive *last;
} LNK_DirectiveList;

typedef struct LNK_DirectiveInfo
{
  LNK_DirectiveList v[LNK_CmdSwitch_Count];
} LNK_DirectiveInfo;

////////////////////////////////

#define LNK_MakeChunkInputIdx(obj_idx, sect_idx) (((U64)(obj_idx) << 32) | (U64)((sect_idx) & max_U32))

typedef struct LNK_Obj
{
  String8             data;
  String8             path;
  String8             lib_path;
  U64                 input_idx;
  U64                 common_symbol_size;
  COFF_MachineType    machine;
  U64                 chunk_count;
  U64                 sect_count;
  String8            *sect_name_arr;
  String8            *sect_sort_arr;
  LNK_RelocList      *sect_reloc_list_arr;
  LNK_ChunkPtr       *chunk_arr;
  LNK_SymbolList      symbol_list;
  LNK_DirectiveInfo   directive_info;
  LNK_ExportParseList export_parse;
  String8List         include_symbol_list;
  LNK_AltNameList     alt_name_list;
} LNK_Obj;

typedef struct LNK_ObjNode
{
  struct LNK_ObjNode *next;
  LNK_Obj             data;
} LNK_ObjNode;

typedef struct LNK_ObjList
{
  U64          count;
  LNK_ObjNode *first;
  LNK_ObjNode *last;
} LNK_ObjList;

typedef struct LNK_ObjNodeArray
{
  U64          count;
  LNK_ObjNode *v;
} LNK_ObjNodeArray;

////////////////////////////////

typedef struct LNK_SectDefn
{
  struct LNK_SectDefn *next;
  LNK_Obj             *obj;
  String8              name;
  COFF_SectionFlags    flags;
  U64                  idx;
} LNK_SectDefn;

typedef struct
{
  U64           count;
  LNK_SectDefn *first;
  LNK_SectDefn *last;
} LNK_SectDefnList;

typedef struct
{
  LNK_InputObj    **inputs;
  LNK_ObjNode      *obj_node_arr;
  U64               obj_id_base;
  LNK_SectDefnList *defn_arr;
  LNK_SectionTable *sectab;
  U64              *function_pad_min;
} LNK_ObjIniter;

typedef struct
{
  Rng1U64          *range_arr;
  LNK_ObjNode      *obj_node_arr;
  LNK_SectDefnList *defn_arr;
  LNK_SectDefnList *conf_arr;
} LNK_ObjNewSectScanner;

typedef struct
{
  LNK_SectionTable *sectab;
  LNK_ObjNode      *obj_arr;
  U64             **chunk_counts;
} LNK_ChunkCounter;

typedef struct
{
  LNK_ChunkManager *cman;
  U64              *chunk_id;
} LNK_ChunkRefAssign;

typedef struct
{
  LNK_SectionTable *sectab;
  Rng1U64          *range_arr;
  U64             **chunk_ids;
  LNK_ObjNode      *obj_arr;
  LNK_ChunkList   **nosort_chunk_list_arr_arr;
  LNK_ChunkList   **chunk_list_arr_arr;
} LNK_ChunkRefAssigner;

typedef struct
{
  LNK_SymbolType   type;
  LNK_ObjNodeArray in_arr;
  LNK_SymbolList  *out_arr;
  Rng1U64         *range_arr;
} LNK_SymbolCollector;

typedef struct
{
  LNK_Obj      **obj_arr;
  String8        name;
  String8        postfix;
  B32            collect_discarded;
  LNK_ChunkList *list_arr;
} LNK_CollectObjChunksTaskData;

typedef struct
{
  Rng1U64          *range_arr;
  LNK_ObjNodeArray  in_arr;
  String8List      *out_arr;
} LNK_DefaultLibCollector;

typedef struct
{
  LNK_ObjNode  *in_arr;
  String8List  *out_arr;
  Rng1U64      *range_arr;
} LNK_ManifestDependencyCollector;

////////////////////////////////

internal void lnk_error_obj(LNK_ErrorCode code, LNK_Obj *obj, char *fmt, ...);

////////////////////////////////

internal void             lnk_input_obj_list_push_node(LNK_InputObjList *list, LNK_InputObj *node);
internal void             lnk_input_obj_list_concat_in_place(LNK_InputObjList *list, LNK_InputObjList *to_concat);
internal LNK_InputObj *   lnk_input_obj_list_push(Arena *arena, LNK_InputObjList *list);
internal LNK_InputObj **  lnk_array_from_input_obj_list(Arena *arena, LNK_InputObjList list);
internal LNK_InputObjList lnk_input_obj_list_from_string_list(Arena *arena, String8List list);
internal LNK_InputObjList lnk_list_from_input_obj_arr(LNK_InputObj **arr, U64 count);

////////////////////////////////

internal LNK_InputObjList lnk_input_obj_list_from_string_list(Arena *arena, String8List list);

internal LNK_Obj **       lnk_obj_arr_from_list(Arena *arena, LNK_ObjList list);
internal LNK_ObjNodeArray lnk_obj_list_reserve(Arena *arena, LNK_ObjList *list, U64 count);
internal LNK_ChunkList *  lnk_collect_obj_chunks(TP_Context *tp, TP_Arena *arena, U64 obj_count, LNK_Obj **obj_arr, String8 name, String8 postfix, B32 collect_discarded);
internal LNK_ObjNodeArray lnk_obj_list_push_parallel(TP_Context *tp, TP_Arena *tp_arena, LNK_ObjList *obj_list, LNK_SectionTable *sectab, U64 *function_pad_min, U64 input_count, LNK_InputObj **inputs);

internal LNK_Chunk *       lnk_sect_chunk_array_from_coff(Arena *arena, U64 obj_id, String8 obj_path, String8 coff_data, U64 sect_count, COFF_SectionHeader *coff_sect_arr, String8 *sect_name_arr, String8 *sect_postfix_arr);
internal LNK_SymbolArray lnk_symbol_array_from_coff(Arena *arena, LNK_Obj *obj, String8 obj_path, String8 lib_path, B32 is_big_obj, U64 function_pad_min, U64 sect_count, COFF_SectionHeader *section_table, U64 symbol_count, void *symbol_table, String8 string_table, LNK_ChunkPtr *chunk_table, LNK_Chunk *master_common_block);
internal LNK_RelocList     lnk_reloc_list_from_coff_reloc_array(Arena *arena, COFF_MachineType machine, LNK_Chunk *chunk, LNK_SymbolArray symbol_array, COFF_Reloc *reloc_v, U64 reloc_count);
internal LNK_RelocList *   lnk_reloc_list_array_from_coff(Arena *arena, COFF_MachineType machine, String8 coff_data, U64 sect_count, COFF_SectionHeader *coff_sect_arr, LNK_ChunkPtr *chunk_ptr_arr, LNK_SymbolArray symbol_array);
internal LNK_DirectiveInfo lnk_directive_info_from_sections(Arena *arena, String8 obj_path, String8 lib_path, U64 chunk_count, LNK_RelocList *reloc_list_arr, String8 *sect_name_arr, LNK_Chunk *chunk_arr);

internal U32 lnk_obj_get_features(LNK_Obj *obj);
internal U32 lnk_obj_get_comp_id(LNK_Obj *obj);
internal U32 lnk_obj_get_vol_md(LNK_Obj *obj);

