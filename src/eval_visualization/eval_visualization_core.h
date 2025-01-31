// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_VISUALIZATION_CORE_H
#define EVAL_VISUALIZATION_CORE_H

////////////////////////////////
//~ rjf: Key Type (Uniquely Refers To One Tree Node)

typedef struct EV_Key EV_Key;
struct EV_Key
{
  U64 parent_hash;
  U64 child_id;
};

////////////////////////////////
//~ rjf: Visualization State Type

//- rjf: expand hash table & tree

typedef struct EV_ExpandNode EV_ExpandNode;
struct EV_ExpandNode
{
  EV_ExpandNode *hash_next;
  EV_ExpandNode *hash_prev;
  EV_ExpandNode *first;
  EV_ExpandNode *last;
  EV_ExpandNode *next;
  EV_ExpandNode *prev;
  EV_ExpandNode *parent;
  EV_Key key;
  B32 expanded;
};

typedef struct EV_ExpandSlot EV_ExpandSlot;
struct EV_ExpandSlot
{
  EV_ExpandNode *first;
  EV_ExpandNode *last;
};

//- rjf: hash table for view rules

typedef struct EV_KeyViewRuleNode EV_KeyViewRuleNode;
struct EV_KeyViewRuleNode
{
  EV_KeyViewRuleNode *hash_next;
  EV_KeyViewRuleNode *hash_prev;
  EV_Key key;
  U8 *buffer;
  U64 buffer_cap;
  U64 buffer_string_size;
};

typedef struct EV_KeyViewRuleSlot EV_KeyViewRuleSlot;
struct EV_KeyViewRuleSlot
{
  EV_KeyViewRuleNode *first;
  EV_KeyViewRuleNode *last;
};

//- rjf: view state bundle

typedef struct EV_View EV_View;
struct EV_View
{
  Arena *arena;
  EV_ExpandSlot *expand_slots;
  U64 expand_slots_count;
  EV_ExpandNode *free_expand_node;
  EV_KeyViewRuleSlot *key_view_rule_slots;
  U64 key_view_rule_slots_count;
  EV_KeyViewRuleNode *free_key_view_rule_node;
};

////////////////////////////////
//~ rjf: View Rule Instance Types

typedef struct EV_ViewRule EV_ViewRule;
struct EV_ViewRule
{
  MD_Node *root;
};

typedef struct EV_ViewRuleNode EV_ViewRuleNode;
struct EV_ViewRuleNode
{
  EV_ViewRuleNode *next;
  EV_ViewRule v;
};

typedef struct EV_ViewRuleList EV_ViewRuleList;
struct EV_ViewRuleList
{
  EV_ViewRuleNode *first;
  EV_ViewRuleNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: View Rule Info Types

typedef struct EV_ExpandInfo EV_ExpandInfo;
struct EV_ExpandInfo
{
  void *user_data;
  U64 row_count;
  B32 single_item; // all rows form a single "item" - a singular, but large, row
  B32 add_new_row; // also supports an 'add new row', as the final row, within `row_count`
  B32 rows_default_expanded;
};

typedef struct EV_ExpandRangeInfo EV_ExpandRangeInfo;
struct EV_ExpandRangeInfo
{
  U64 row_exprs_count;
  String8 *row_strings;
  String8 *row_view_rules;
  E_Expr **row_exprs;
  E_Member **row_members;
};

#define EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_SIG(name) E_Expr *name(Arena *arena, E_Expr *expr, MD_Node *params)
#define EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_NAME(name) ev_view_rule_expr_resolution__##name
#define EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(name) internal EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_SIG(EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_NAME(name))

#define EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_SIG(name) EV_ExpandInfo name(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params)
#define EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_NAME(name) ev_view_rule_expr_expand_info__##name
#define EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(name) internal EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_SIG(EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_NAME(name))

#define EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_SIG(name) EV_ExpandRangeInfo name(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, Rng1U64 idx_range, void *user_data)
#define EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_NAME(name) ev_view_rule_expr_expand_range_info__##name
#define EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(name) internal EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_SIG(EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_NAME(name))

#define EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_SIG(name) U64 name(U64 num, void *user_data)
#define EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_NAME(name) ev_view_rule_expr_expand_id_from_num_##name
#define EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(name) internal EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_SIG(EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_NAME(name))

#define EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_SIG(name) U64 name(U64 id, void *user_data)
#define EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_NAME(name) ev_view_rule_expr_expand_num_from_id_##name
#define EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(name) internal EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_SIG(EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_NAME(name))

typedef EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_SIG(EV_ViewRuleExprResolutionHookFunctionType);
typedef EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_SIG(EV_ViewRuleExprExpandInfoHookFunctionType);
typedef EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_SIG(EV_ViewRuleExprExpandRangeInfoHookFunctionType);
typedef EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_SIG(EV_ViewRuleExprExpandIDFromNumHookFunctionType);
typedef EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_SIG(EV_ViewRuleExprExpandNumFromIDHookFunctionType);

typedef U32 EV_ViewRuleInfoFlags; // NOTE(rjf): see @view_rule_info
enum
{
  EV_ViewRuleInfoFlag_Inherited  = (1<<0),
  EV_ViewRuleInfoFlag_Expandable = (1<<1),
};

typedef struct EV_ViewRuleInfo EV_ViewRuleInfo;
struct EV_ViewRuleInfo
{
  String8 string;
  EV_ViewRuleInfoFlags flags;
  EV_ViewRuleExprResolutionHookFunctionType *expr_resolution;
  EV_ViewRuleExprExpandInfoHookFunctionType *expr_expand_info;
  EV_ViewRuleExprExpandRangeInfoHookFunctionType *expr_expand_range_info;
  EV_ViewRuleExprExpandIDFromNumHookFunctionType *expr_expand_id_from_num;
  EV_ViewRuleExprExpandIDFromNumHookFunctionType *expr_expand_num_from_id;
};

typedef struct EV_ViewRuleInfoNode EV_ViewRuleInfoNode;
struct EV_ViewRuleInfoNode
{
  EV_ViewRuleInfoNode *next;
  EV_ViewRuleInfo v;
};

typedef struct EV_ViewRuleInfoSlot EV_ViewRuleInfoSlot;
struct EV_ViewRuleInfoSlot
{
  EV_ViewRuleInfoNode *first;
  EV_ViewRuleInfoNode *last;
};

typedef struct EV_ViewRuleInfoTable EV_ViewRuleInfoTable;
struct EV_ViewRuleInfoTable
{
  EV_ViewRuleInfoSlot *slots;
  U64 slots_count;
};

////////////////////////////////
//~ rjf: Blocks

typedef struct EV_Block EV_Block;
struct EV_Block
{
  // rjf: links
  EV_Block *first;
  EV_Block *last;
  EV_Block *next;
  EV_Block *prev;
  EV_Block *parent;
  
  // rjf: key
  EV_Key key;
  
  // rjf: split index, relative to parent's space
  U64 split_relative_idx;
  
  // rjf: expression / visualization info
  String8 string;
  E_Expr *expr;
  EV_ViewRuleList *view_rules;
  EV_ViewRuleInfo *expand_view_rule_info;
  MD_Node *expand_view_rule_params;
  void *expand_view_rule_info_user_data;
  
  // rjf: expansion info
  U64 row_count;
  B32 single_item;
  B32 rows_default_expanded;
};

typedef struct EV_BlockTree EV_BlockTree;
struct EV_BlockTree
{
  EV_Block *root;
  U64 total_row_count;
  U64 total_item_count;
};

typedef struct EV_BlockRange EV_BlockRange;
struct EV_BlockRange
{
  EV_Block *block;
  Rng1U64 range;
};

typedef struct EV_BlockRangeNode EV_BlockRangeNode;
struct EV_BlockRangeNode
{
  EV_BlockRangeNode *next;
  EV_BlockRange v;
};

typedef struct EV_BlockRangeList EV_BlockRangeList;
struct EV_BlockRangeList
{
  EV_BlockRangeNode *first;
  EV_BlockRangeNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Rows

typedef struct EV_Row EV_Row;
struct EV_Row
{
  EV_Block *block;
  EV_Key key;
  U64 visual_size;
  String8 string;
  E_Expr *expr;
  E_Member *member;
  EV_ViewRuleList *view_rules;
};

typedef struct EV_WindowedRowNode EV_WindowedRowNode;
struct EV_WindowedRowNode
{
  EV_WindowedRowNode *next;
  U64 visual_size_skipped;
  U64 visual_size_chopped;
  EV_Row row;
};

typedef struct EV_WindowedRowList EV_WindowedRowList;
struct EV_WindowedRowList
{
  EV_WindowedRowNode *first;
  EV_WindowedRowNode *last;
  U64 count;
  U64 count_before_visual;
  U64 count_before_semantic;
};

////////////////////////////////
//~ rjf: Automatic Type -> View Rule Map Types

typedef struct EV_AutoViewRuleNode EV_AutoViewRuleNode;
struct EV_AutoViewRuleNode
{
  EV_AutoViewRuleNode *next;
  E_TypeKey key;
  String8 view_rule;
  B32 is_required;
};

typedef struct EV_AutoViewRuleSlot EV_AutoViewRuleSlot;
struct EV_AutoViewRuleSlot
{
  EV_AutoViewRuleNode *first;
  EV_AutoViewRuleNode *last;
  U64 count;
};

typedef struct EV_AutoViewRuleTable EV_AutoViewRuleTable;
struct EV_AutoViewRuleTable
{
  EV_AutoViewRuleSlot *slots;
  U64 slots_count;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/eval_visualization.meta.h"

////////////////////////////////
//~ rjf: String Generation Types

typedef U32 EV_StringFlags;
enum
{
  EV_StringFlag_ReadOnlyDisplayRules = (1<<0),
  EV_StringFlag_PrettyNames          = (1<<1),
};

////////////////////////////////
//~ rjf: Nil/Identity View Rule Hooks

EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(identity);
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(nil);
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(nil);
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(identity);
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(identity);

////////////////////////////////
//~ rjf: Globals

global read_only EV_ViewRuleInfo ev_nil_view_rule_info =
{
  {0},
  0,
  EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_NAME(identity),
  EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_NAME(nil),
  EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_NAME(nil),
  EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_NAME(identity),
  EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_NAME(identity),
};
thread_static EV_ViewRuleInfoTable *ev_view_rule_info_table = 0;
global read_only EV_ViewRuleList ev_nil_view_rule_list = {0};
thread_static EV_AutoViewRuleTable *ev_auto_view_rule_table = 0;
global read_only EV_Block ev_nil_block = {&ev_nil_block, &ev_nil_block, &ev_nil_block, &ev_nil_block, &ev_nil_block, {0}, 0, {0}, &e_expr_nil, &ev_nil_view_rule_list, &ev_nil_view_rule_info};

////////////////////////////////
//~ rjf: Key Functions

internal EV_Key ev_key_make(U64 parent_hash, U64 child_id);
internal EV_Key ev_key_zero(void);
internal EV_Key ev_key_root(void);
internal B32 ev_key_match(EV_Key a, EV_Key b);
internal U64 ev_hash_from_seed_string(U64 seed, String8 string);
internal U64 ev_hash_from_key(EV_Key key);

////////////////////////////////
//~ rjf: Type Info Helpers

//- rjf: type info -> expandability/editablity
internal B32 ev_type_key_and_mode_is_expandable(E_TypeKey type_key, E_Mode mode);
internal B32 ev_type_key_is_editable(E_TypeKey type_key);

////////////////////////////////
//~ rjf: View Functions

//- rjf: creation / deletion
internal EV_View *ev_view_alloc(void);
internal void ev_view_release(EV_View *view);

//- rjf: lookups / mutations
internal EV_ExpandNode *ev_expand_node_from_key(EV_View *view, EV_Key key);
internal B32 ev_expansion_from_key(EV_View *view, EV_Key key);
internal String8 ev_view_rule_from_key(EV_View *view, EV_Key key);
internal void ev_key_set_expansion(EV_View *view, EV_Key parent_key, EV_Key key, B32 expanded);
internal void ev_key_set_view_rule(EV_View *view, EV_Key key, String8 view_rule_string);

////////////////////////////////
//~ rjf: View Rule Info Table Building / Selection / Lookups

internal void ev_view_rule_info_table_push(Arena *arena, EV_ViewRuleInfoTable *table, EV_ViewRuleInfo *info);
internal void ev_view_rule_info_table_push_builtins(Arena *arena, EV_ViewRuleInfoTable *table);
internal void ev_select_view_rule_info_table(EV_ViewRuleInfoTable *table);
internal EV_ViewRuleInfo *ev_view_rule_info_from_string(String8 string);

////////////////////////////////
//~ rjf: Automatic Type -> View Rule Table Building / Selection / Lookups

internal void ev_auto_view_rule_table_push_new(Arena *arena, EV_AutoViewRuleTable *table, E_TypeKey type_key, String8 view_rule, B32 is_required);
internal void ev_select_auto_view_rule_table(EV_AutoViewRuleTable *table);
internal EV_ViewRuleList *ev_auto_view_rules_from_type_key(Arena *arena, E_TypeKey type_key, B32 gather_required, B32 gather_optional);

////////////////////////////////
//~ rjf: View Rule Instance List Building

internal void ev_view_rule_list_push_tree(Arena *arena, EV_ViewRuleList *list, MD_Node *root);
internal void ev_view_rule_list_push_string(Arena *arena, EV_ViewRuleList *list, String8 string);
internal EV_ViewRuleList *ev_view_rule_list_from_string(Arena *arena, String8 string);
internal EV_ViewRuleList *ev_view_rule_list_from_expr_fastpaths(Arena *arena, String8 string);
internal EV_ViewRuleList *ev_view_rule_list_from_inheritance(Arena *arena, EV_ViewRuleList *src);
internal EV_ViewRuleList *ev_view_rule_list_copy(Arena *arena, EV_ViewRuleList *src);
internal void ev_view_rule_list_concat_in_place(EV_ViewRuleList *dst, EV_ViewRuleList **src);

////////////////////////////////
//~ rjf: Expression Resolution (Dynamic Overrides, View Rule Application)

internal E_Expr *ev_resolved_from_expr(Arena *arena, E_Expr *expr, EV_ViewRuleList *view_rules);

////////////////////////////////
//~ rjf: Block Building

internal EV_BlockTree ev_block_tree_from_expr(Arena *arena, EV_View *view, String8 filter, String8 string, E_Expr *expr, EV_ViewRuleList *view_rules);
internal EV_BlockTree ev_block_tree_from_string(Arena *arena, EV_View *view, String8 filter, String8 string, EV_ViewRuleList *view_rules);
internal U64 ev_depth_from_block(EV_Block *block);

////////////////////////////////
//~ rjf: Block Coordinate Spaces

internal EV_BlockRangeList ev_block_range_list_from_tree(Arena *arena, EV_BlockTree *block_tree);
internal EV_BlockRange ev_block_range_from_num(EV_BlockRangeList *block_ranges, U64 num);
internal EV_Key ev_key_from_num(EV_BlockRangeList *block_ranges, U64 num);
internal U64    ev_num_from_key(EV_BlockRangeList *block_ranges, EV_Key key);
internal U64    ev_vidx_from_num(EV_BlockRangeList *block_ranges, U64 num);
internal U64    ev_num_from_vidx(EV_BlockRangeList *block_ranges, U64 vidx);

////////////////////////////////
//~ rjf: Row Building

internal EV_WindowedRowList ev_windowed_row_list_from_block_range_list(Arena *arena, EV_View *view, String8 filter, EV_BlockRangeList *block_ranges, Rng1U64 visible_range);
internal EV_Row *ev_row_from_num(Arena *arena, EV_View *view, String8 filter, EV_BlockRangeList *block_ranges, U64 num);
internal EV_WindowedRowList ev_rows_from_num_range(Arena *arena, EV_View *view, String8 filter, EV_BlockRangeList *block_ranges, Rng1U64 num_range);
internal String8 ev_expr_string_from_row(Arena *arena, EV_Row *row, EV_StringFlags flags);
internal B32 ev_row_is_expandable(EV_Row *row);
internal B32 ev_row_is_editable(EV_Row *row);

////////////////////////////////
//~ rjf: Stringification

//- rjf: leaf stringification
internal String8 ev_string_from_ascii_value(Arena *arena, U8 val);
internal String8 ev_string_from_hresult_facility_code(U32 code);
internal String8 ev_string_from_hresult_code(U32 code);
internal String8 ev_string_from_simple_typed_eval(Arena *arena, EV_StringFlags flags, U32 radix, U32 min_digits, E_Eval eval);
internal String8 ev_escaped_from_raw_string(Arena *arena, String8 raw);

#endif // EVAL_VISUALIZATION_CORE_H
