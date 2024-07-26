// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef METAGEN_H
#define METAGEN_H

////////////////////////////////
//~ rjf: Message Type

typedef struct MG_Msg MG_Msg;
struct MG_Msg
{
  String8 location;
  String8 kind;
  String8 msg;
};

typedef struct MG_MsgNode MG_MsgNode;
struct MG_MsgNode
{
  MG_MsgNode *next;
  MG_Msg v;
};

typedef struct MG_MsgList MG_MsgList;
struct MG_MsgList
{
  MG_MsgNode *first;
  MG_MsgNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Parse Artifact Types

typedef struct MG_FileParse MG_FileParse;
struct MG_FileParse
{
  MD_Node *root;
};

typedef struct MG_FileParseNode MG_FileParseNode;
struct MG_FileParseNode
{
  MG_FileParseNode *next;
  MG_FileParse v;
};

typedef struct MG_FileParseList MG_FileParseList;
struct MG_FileParseList
{
  MG_FileParseNode *first;
  MG_FileParseNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Map Type

typedef struct MG_MapNode MG_MapNode;
struct MG_MapNode
{
  MG_MapNode *next;
  String8 key;
  void *val;
};

typedef struct MG_MapSlot MG_MapSlot;
struct MG_MapSlot
{
  MG_MapNode *first;
  MG_MapNode *last;
};

typedef struct MG_Map MG_Map;
struct MG_Map
{
  MG_MapSlot *slots;
  U64 slots_count;
};

////////////////////////////////
//~ rjf: String Expression Types

typedef enum MG_StrExprOpKind
{
  MG_StrExprOpKind_Null,
  MG_StrExprOpKind_Prefix,
  MG_StrExprOpKind_Postfix,
  MG_StrExprOpKind_Binary,
  MG_StrExprOpKind_COUNT
}
MG_StrExprOpKind;

typedef enum MG_StrExprOp
{
  MG_StrExprOp_Null,
  
#define MG_StrExprOp_FirstString MG_StrExprOp_Dot
  MG_StrExprOp_Dot,
  MG_StrExprOp_ExpandIfTrue,
  MG_StrExprOp_Concat,
  MG_StrExprOp_BumpToColumn,
#define MG_StrExprOp_LastString MG_StrExprOp_BumpToColumn
  
#define MG_StrExprOp_FirstNumeric MG_StrExprOp_Add
  MG_StrExprOp_Add,
  MG_StrExprOp_Subtract,
  MG_StrExprOp_Multiply,
  MG_StrExprOp_Divide,
  MG_StrExprOp_Modulo,
  MG_StrExprOp_LeftShift,
  MG_StrExprOp_RightShift,
  MG_StrExprOp_BitwiseAnd,
  MG_StrExprOp_BitwiseOr,
  MG_StrExprOp_BitwiseXor,
  MG_StrExprOp_BitwiseNegate,
  MG_StrExprOp_BooleanAnd,
  MG_StrExprOp_BooleanOr,
  MG_StrExprOp_BooleanNot,
  MG_StrExprOp_Equals,
  MG_StrExprOp_DoesNotEqual,
#define MG_StrExprOp_LastNumeric MG_StrExprOp_DoesNotEqual
  
  MG_StrExprOp_COUNT,
}
MG_StrExprOp;

typedef struct MG_StrExpr MG_StrExpr;
struct MG_StrExpr
{
  MG_StrExpr *parent;
  MG_StrExpr *left;
  MG_StrExpr *right;
  MG_StrExprOp op;
  MD_Node *node;
};

typedef struct MG_StrExprParseResult MG_StrExprParseResult;
struct MG_StrExprParseResult
{
  MG_StrExpr *root;
  MD_MsgList msgs;
  MD_Node *next_node;
};

////////////////////////////////
//~ rjf: Table Generation Types

typedef struct MG_NodeArray MG_NodeArray;
struct MG_NodeArray
{
  MD_Node **v;
  U64 count;
};

typedef struct MG_NodeGrid MG_NodeGrid;
struct MG_NodeGrid
{
  U64 x_stride;
  U64 y_stride;
  MG_NodeArray cells;
  MG_NodeArray row_parents;
};

typedef enum MG_ColumnKind
{
  MG_ColumnKind_DirectCell,
  MG_ColumnKind_CheckForTag,
  MG_ColumnKind_TagChild,
  MG_ColumnKind_COUNT
}
MG_ColumnKind;

typedef struct MG_ColumnDesc MG_ColumnDesc;
struct MG_ColumnDesc
{
  String8 name;
  MG_ColumnKind kind;
  String8 tag_name;
};

typedef struct MG_ColumnDescArray MG_ColumnDescArray;
struct MG_ColumnDescArray
{
  U64 count;
  MG_ColumnDesc *v;
};

typedef struct MG_TableExpandTask MG_TableExpandTask;
struct MG_TableExpandTask
{
  MG_TableExpandTask *next;
  String8 expansion_label;
  MG_NodeGrid *grid;
  MG_ColumnDescArray column_descs;
  U64 count;
  U64 idx;
};

typedef struct MG_TableExpandInfo MG_TableExpandInfo;
struct MG_TableExpandInfo
{
  MG_TableExpandTask *first_expand_task;
  String8 missing_value_fallback;
};

////////////////////////////////
//~ rjf: Main Output Path Types

typedef struct MG_Layer MG_Layer;
struct MG_Layer
{
  String8 key;
  B32 is_library;
  String8 gen_folder_name;
  String8 h_name_override;
  String8 c_name_override;
  String8List enums;
  String8List structs;
  String8List h_functions;
  String8List h_tables;
  String8List h_catchall;
  String8List h_header;
  String8List h_footer;
  String8List c_functions;
  String8List c_tables;
  String8List c_catchall;
  String8List c_header;
  String8List c_footer;
};

typedef struct MG_LayerNode MG_LayerNode;
struct MG_LayerNode
{
  MG_LayerNode *next;
  MG_Layer v;
};

typedef struct MG_LayerSlot MG_LayerSlot;
struct MG_LayerSlot
{
  MG_LayerNode *first;
  MG_LayerNode *last;
};

typedef struct MG_State MG_State;
struct MG_State
{
  U64 slots_count;
  MG_LayerSlot *slots;
};

////////////////////////////////
//~ rjf: Globals

global Arena *mg_arena = 0;
global MG_State *mg_state = 0;
read_only global MG_StrExpr mg_str_expr_nil = {&mg_str_expr_nil, &mg_str_expr_nil, &mg_str_expr_nil};

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 mg_hash_from_string(String8 string);
internal TxtPt mg_txt_pt_from_string_off(String8 string, U64 off);

////////////////////////////////
//~ rjf: Message Lists

internal void mg_msg_list_push(Arena *arena, MG_MsgList *msgs, MG_Msg *msg);

////////////////////////////////
//~ rjf: String Escaping

internal String8 mg_escaped_from_str8(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: String Wrapping

internal String8List mg_wrapped_lines_from_string(Arena *arena, String8 string, U64 first_line_max_width, U64 max_width, U64 wrap_indent);

////////////////////////////////
//~ rjf: C-String-Izing

internal String8 mg_c_string_literal_from_multiline_string(String8 string);
internal String8 mg_c_array_literal_contents_from_data(String8 data);

////////////////////////////////
//~ rjf: Map Functions

internal MG_Map mg_push_map(Arena *arena, U64 slot_count);
internal void *mg_map_ptr_from_string(MG_Map *map, String8 string);
internal void mg_map_insert_ptr(Arena *arena, MG_Map *map, String8 string, void *val);

////////////////////////////////
//~ rjf: String Expression Parsing

internal MG_StrExpr *mg_push_str_expr(Arena *arena, MG_StrExprOp op, MD_Node *node);
internal MG_StrExprParseResult mg_str_expr_parse_from_first_opl__min_prec(Arena *arena, MD_Node *first, MD_Node *opl, S8 min_prec);
internal MG_StrExprParseResult mg_str_expr_parse_from_first_opl(Arena *arena, MD_Node *first, MD_Node *opl);
internal MG_StrExprParseResult mg_str_expr_parse_from_root(Arena *arena, MD_Node *root);

////////////////////////////////
//~ rjf: Table Generation Functions

internal MG_NodeArray mg_node_array_make(Arena *arena, U64 count);
internal MG_NodeArray mg_child_array_from_node(Arena *arena, MD_Node *node);
internal MG_NodeGrid mg_node_grid_make_from_node(Arena *arena, MD_Node *root);
internal MG_NodeArray mg_row_from_index(MG_NodeGrid grid, U64 index);
internal MG_NodeArray mg_column_from_index(Arena *arena, MG_NodeGrid grid, U64 index);
internal MD_Node *mg_node_from_grid_xy(MG_NodeGrid grid, U64 x, U64 y);

internal MG_ColumnDescArray mg_column_desc_array_make(Arena *arena, U64 count, MG_ColumnDesc *descs);
internal MG_ColumnDescArray mg_column_desc_array_from_tag(Arena *arena, MD_Node *tag);
internal U64 mg_column_index_from_name(MG_ColumnDescArray descs, String8 name);
internal String8 mg_string_from_row_desc_idx(MD_Node *row_parent, MG_ColumnDescArray descs, U64 idx);

internal S64 mg_eval_table_expand_expr__numeric(MG_StrExpr *expr, MG_TableExpandInfo *info);
internal void mg_eval_table_expand_expr__string(Arena *arena, MG_StrExpr *expr, MG_TableExpandInfo *info, String8List *out);
internal void mg_loop_table_column_expansion(Arena *arena, String8 strexpr, MG_TableExpandInfo *info, MG_TableExpandTask *task, String8List *out);
internal String8List mg_string_list_from_table_gen(Arena *arena, MG_Map grid_name_map, MG_Map grid_column_desc_map, String8 fallback, MD_Node *gen);

////////////////////////////////
//~ rjf: Layer Lookup Functions

internal String8 mg_layer_key_from_path(String8 path);
internal MG_Layer *mg_layer_from_key(String8 key);

#endif //METAGEN_H
