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
  U64 child_num;
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
//~ rjf: Blocks

typedef enum EV_BlockKind
{
  EV_BlockKind_Null,              // empty
  EV_BlockKind_Root,              // root of tree or subtree; possibly-expandable expression.
  EV_BlockKind_Members,           // members of struct, class, union
  EV_BlockKind_EnumMembers,       // members of enum
  EV_BlockKind_Elements,          // elements of array
  EV_BlockKind_Canvas,            // escape hatch for arbitrary UI
  EV_BlockKind_DebugInfoTable,    // block of filtered debug info table elements
  EV_BlockKind_COUNT,
}
EV_BlockKind;

typedef struct EV_Block EV_Block;
struct EV_Block
{
  // rjf: kind & keys
  EV_BlockKind kind;
  EV_Key parent_key;
  EV_Key key;
  S32 depth;
  
  // rjf: evaluation info
  String8 string;
  E_Expr *expr;
  
  // rjf: info about ranges that this block spans
  Rng1U64 visual_idx_range;
  Rng1U64 semantic_idx_range;
  
  // rjf: visualization info extensions
  EV_ViewRuleList *view_rules;
  E_MemberArray members;
  E_EnumValArray enum_vals;
  RDI_SectionKind fzy_target;
  FZY_ItemArray fzy_backing_items;
};

typedef struct EV_BlockNode EV_BlockNode;
struct EV_BlockNode
{
  EV_BlockNode *next;
  EV_Block v;
};

typedef struct EV_BlockList EV_BlockList;
struct EV_BlockList
{
  EV_BlockNode *first;
  EV_BlockNode *last;
  U64 count;
  U64 total_visual_row_count;
  U64 total_semantic_row_count;
};

typedef struct EV_BlockArray EV_BlockArray;
struct EV_BlockArray
{
  EV_Block *v;
  U64 count;
  U64 total_visual_row_count;
  U64 total_semantic_row_count;
};

////////////////////////////////
//~ rjf: View Rule Info Types

#define EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_SIG(name) E_Expr *name(Arena *arena, E_Expr *expr, MD_Node *params)
#define EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_NAME(name) ev_view_rule_expr_resolution__##name
#define EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(name) internal EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_SIG(EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_NAME(name))
#define EV_VIEW_RULE_BLOCK_PROD_FUNCTION_SIG(name) void name(Arena *arena,                                      \
EV_View *view,                                     \
EV_Key parent_key,                                 \
EV_Key key,                                        \
EV_ExpandNode *expand_node,                        \
String8 string,                                    \
E_Expr *expr,                                      \
EV_ViewRuleList *view_rules,                \
MD_Node *view_params,                              \
S32 depth,                                         \
struct EV_BlockList *out)
#define EV_VIEW_RULE_BLOCK_PROD_FUNCTION_NAME(name) ev_view_rule_block_prod__##name
#define EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(name) internal EV_VIEW_RULE_BLOCK_PROD_FUNCTION_SIG(EV_VIEW_RULE_BLOCK_PROD_FUNCTION_NAME(name))
typedef EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_SIG(EV_ViewRuleExprResolutionHookFunctionType);
typedef EV_VIEW_RULE_BLOCK_PROD_FUNCTION_SIG(EV_ViewRuleBlockProdHookFunctionType);

typedef U32 EV_ViewRuleInfoFlags; // NOTE(rjf): see @view_rule_info
enum
{
  EV_ViewRuleInfoFlag_Inherited      = (1<<0),
  EV_ViewRuleInfoFlag_Expandable     = (1<<1),
  EV_ViewRuleInfoFlag_ExprResolution = (1<<2),
  EV_ViewRuleInfoFlag_VizBlockProd   = (1<<3),
};

typedef struct EV_ViewRuleInfo EV_ViewRuleInfo;
struct EV_ViewRuleInfo
{
  String8 string;
  EV_ViewRuleInfoFlags flags;
  EV_ViewRuleExprResolutionHookFunctionType *expr_resolution;
  EV_ViewRuleBlockProdHookFunctionType *block_prod;
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
//~ rjf: Generated Code

#include "generated/eval_visualization.meta.h"

////////////////////////////////
//~ rjf: Rows

typedef U32 EV_StringFlags;
enum
{
  EV_StringFlag_ReadOnlyDisplayRules = (1<<0),
};

typedef struct EV_Row EV_Row;
struct EV_Row
{
  EV_Row *next;
  
  // rjf: block hierarchy info
  S32 depth;
  EV_Key parent_key;
  EV_Key key;
  
  // rjf: row size/scroll info
  U64 size_in_rows;
  U64 skipped_size_in_rows;
  U64 chopped_size_in_rows;
  
  // rjf: evaluation expression
  String8 string;
  E_Member *member;
  E_Expr *expr;
  
  // rjf: view rule attachments
  EV_ViewRuleList *view_rules;
};

typedef struct EV_WindowedRowList EV_WindowedRowList;
struct EV_WindowedRowList
{
  EV_Row *first;
  EV_Row *last;
  U64 count;
  U64 count_before_visual;
  U64 count_before_semantic;
};

////////////////////////////////
//~ rjf: Globals

global read_only EV_ViewRuleInfo ev_nil_view_rule_info = {0};
thread_static EV_ViewRuleInfoTable *ev_view_rule_info_table = 0;
global read_only EV_ViewRuleList ev_nil_view_rule_list = {0};

////////////////////////////////
//~ rjf: Key Functions

internal EV_Key ev_key_make(U64 parent_hash, U64 child_num);
internal EV_Key ev_key_zero(void);
internal B32 ev_key_match(EV_Key a, EV_Key b);
internal U64 ev_hash_from_seed_string(U64 seed, String8 string);
internal U64 ev_hash_from_key(EV_Key key);

////////////////////////////////
//~ rjf: Type Info Helpers

//- rjf: type info -> expandability/editablity
internal B32 ev_type_key_is_expandable(E_TypeKey type_key);
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
//~ rjf: View Rule Instance List Building

internal void ev_view_rule_list_push_tree(Arena *arena, EV_ViewRuleList *list, MD_Node *root);
internal void ev_view_rule_list_push_string(Arena *arena, EV_ViewRuleList *list, String8 string);
internal EV_ViewRuleList *ev_view_rule_list_from_string(Arena *arena, String8 string);
internal EV_ViewRuleList *ev_view_rule_list_from_inheritance(Arena *arena, EV_ViewRuleList *src);
internal EV_ViewRuleList *ev_view_rule_list_copy(Arena *arena, EV_ViewRuleList *src);

////////////////////////////////
//~ rjf: View Rule Expression Resolution

internal E_Expr *ev_expr_from_expr_view_rules(Arena *arena, E_Expr *expr, EV_ViewRuleList *view_rules);

////////////////////////////////
//~ rjf: Block Building

internal EV_Block *ev_block_begin(Arena *arena, EV_BlockKind kind, EV_Key parent_key, EV_Key key, S32 depth);
internal EV_Block *ev_block_split_and_continue(Arena *arena, EV_BlockList *list, EV_Block *split_block, U64 split_idx);
internal void ev_block_end(EV_BlockList *list, EV_Block *block);
internal void ev_append_expr_blocks__rec(Arena *arena, EV_View *view, EV_Key parent_key, EV_Key key, String8 string, E_Expr *expr, EV_ViewRuleList *view_rules, S32 depth, EV_BlockList *list_out);
internal EV_BlockList ev_block_list_from_view_expr_keys(Arena *arena, EV_View *view, EV_ViewRuleList *view_rules, String8 expr, EV_Key parent_key, EV_Key key);
internal void ev_block_list_concat__in_place(EV_BlockList *dst, EV_BlockList *to_push);

////////////////////////////////
//~ rjf: Block List <-> Row Coordinates

internal S64 ev_row_num_from_block_list_key(EV_BlockList *blocks, EV_Key key);
internal EV_Key ev_key_from_block_list_row_num(EV_BlockList *blocks, S64 row_num);
internal EV_Key ev_parent_key_from_block_list_row_num(EV_BlockList *blocks, S64 row_num);

////////////////////////////////
//~ rjf: Block * Index -> Expressions

internal E_Expr *ev_expr_from_block_index(Arena *arena, EV_Block *block, U64 index);

////////////////////////////////
//~ rjf: Row Lists

internal EV_Row *ev_row_list_push_new(Arena *arena, EV_View *view, EV_WindowedRowList *rows, EV_Block *block, EV_Key key, E_Expr *expr);
internal EV_WindowedRowList ev_windowed_row_list_from_block_list(Arena *arena, EV_View *view, Rng1S64 visible_range, EV_BlockList *blocks);
internal String8 ev_expr_string_from_row(Arena *arena, EV_Row *row);
internal B32 ev_row_is_expandable(EV_Row *row);
internal B32 ev_row_is_editable(EV_Row *row);

////////////////////////////////
//~ rjf: Stringification

//- rjf: leaf stringification
internal String8 ev_string_from_ascii_value(Arena *arena, U8 val);
internal String8 ev_string_from_hresult_facility_code(U32 code);
internal String8 ev_string_from_hresult_code(U32 code);
internal String8 ev_string_from_simple_typed_eval(Arena *arena, EV_StringFlags flags, U32 radix, E_Eval eval);
internal String8 ev_escaped_from_raw_string(Arena *arena, String8 raw);

#endif // EVAL_VISUALIZATION_CORE_H
