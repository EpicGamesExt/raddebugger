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
//~ rjf: Expansion Rule Types

typedef struct EV_ExpandInfo EV_ExpandInfo;
struct EV_ExpandInfo
{
  void *user_data;
  U64 row_count;
  B32 single_item; // all rows form a single "item" - a singular, but large, row
  B32 add_new_row; // also supports an 'add new row', as the final row, within `row_count`
  B32 rows_default_expanded;
};

#define EV_EXPAND_RULE_INFO_FUNCTION_SIG(name) EV_ExpandInfo name(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, E_Expr *tag)
#define EV_EXPAND_RULE_INFO_FUNCTION_NAME(name) ev_expand_rule_info__##name
#define EV_EXPAND_RULE_INFO_FUNCTION_DEF(name) internal EV_EXPAND_RULE_INFO_FUNCTION_SIG(EV_EXPAND_RULE_INFO_FUNCTION_NAME(name))
typedef EV_EXPAND_RULE_INFO_FUNCTION_SIG(EV_ExpandRuleInfoHookFunctionType);

typedef struct EV_ExpandRule EV_ExpandRule;
struct EV_ExpandRule
{
  String8 string;
  EV_ExpandRuleInfoHookFunctionType *info;
};

typedef struct EV_ExpandRuleNode EV_ExpandRuleNode;
struct EV_ExpandRuleNode
{
  EV_ExpandRuleNode *next;
  EV_ExpandRule v;
};

typedef struct EV_ExpandRuleSlot EV_ExpandRuleSlot;
struct EV_ExpandRuleSlot
{
  EV_ExpandRuleNode *first;
  EV_ExpandRuleNode *last;
};

typedef struct EV_ExpandRuleTable EV_ExpandRuleTable;
struct EV_ExpandRuleTable
{
  EV_ExpandRuleSlot *slots;
  U64 slots_count;
};

typedef struct EV_ExpandRuleTagPair EV_ExpandRuleTagPair;
struct EV_ExpandRuleTagPair
{
  EV_ExpandRule *rule;
  E_Expr *tag;
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
  E_Expr *lookup_tag;
  E_Expr *expand_tag;
  E_LookupRule *lookup_rule;
  EV_ExpandRule *expand_rule;
  void *lookup_rule_user_data;
  void *expand_rule_user_data;
  
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

EV_EXPAND_RULE_INFO_FUNCTION_DEF(nil);

////////////////////////////////
//~ rjf: Globals

global read_only EV_ExpandRule ev_nil_expand_rule =
{
  {0},
  EV_EXPAND_RULE_INFO_FUNCTION_NAME(nil),
};
thread_static EV_ExpandRuleTable *ev_view_rule_info_table = 0;
thread_static EV_AutoViewRuleTable *ev_auto_view_rule_table = 0;
global read_only EV_Block ev_nil_block = {&ev_nil_block, &ev_nil_block, &ev_nil_block, &ev_nil_block, &ev_nil_block, {0}, 0, {0}, &e_expr_nil, &e_expr_nil, &e_expr_nil, &e_lookup_rule__nil, &ev_nil_expand_rule};

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

internal void ev_expand_rule_table_push(Arena *arena, EV_ExpandRuleTable *table, EV_ExpandRule *info);
#define ev_expand_rule_table_push_new(arena, table, ...) ev_expand_rule_table_push((arena), (table), &(EV_ExpandRule){__VA_ARGS__})
internal void ev_select_expand_rule_table(EV_ExpandRuleTable *table);
internal EV_ExpandRule *ev_expand_rule_from_string(String8 string);

////////////////////////////////
//~ rjf: Expression Resolution (Dynamic Overrides, View Rule Application)

#if 0 // TODO(rjf): @cfg
internal E_Expr *ev_resolved_from_expr(Arena *arena, E_Expr *expr);
#endif

////////////////////////////////
//~ rjf: Upgrading Expressions w/ Tags From All Sources

internal void ev_keyed_expr_push_tags(Arena *arena, EV_View *view, EV_Block *block, EV_Key key, E_Expr *expr);

////////////////////////////////
//~ rjf: Block Building

internal EV_BlockTree ev_block_tree_from_exprs(Arena *arena, EV_View *view, String8 filter, E_ExprChain exprs);
internal U64 ev_depth_from_block(EV_Block *block);

////////////////////////////////
//~ rjf: Block Coordinate Spaces

internal EV_BlockRangeList ev_block_range_list_from_tree(Arena *arena, EV_BlockTree *block_tree);
internal EV_BlockRange ev_block_range_from_num(EV_BlockRangeList *block_ranges, U64 num);
internal EV_Key ev_key_from_num(EV_BlockRangeList *block_ranges, U64 num);
internal U64    ev_num_from_key(EV_BlockRangeList *block_ranges, EV_Key key);
internal U64    ev_vnum_from_num(EV_BlockRangeList *block_ranges, U64 num);
internal U64    ev_num_from_vnum(EV_BlockRangeList *block_ranges, U64 vidx);

////////////////////////////////
//~ rjf: Row Building

internal EV_WindowedRowList ev_windowed_row_list_from_block_range_list(Arena *arena, EV_View *view, String8 filter, EV_BlockRangeList *block_ranges, Rng1U64 vnum_range);
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

////////////////////////////////
//~ rjf: Expression & IR-Tree => Expand Rule

internal EV_ExpandRuleTagPair ev_expand_rule_tag_pair_from_expr_irtree(E_Expr *expr, E_IRTreeAndType *irtree);

#endif // EVAL_VISUALIZATION_CORE_H
