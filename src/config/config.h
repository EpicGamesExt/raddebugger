// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef CONFIG_H
#define CONFIG_H

////////////////////////////////
//~ rjf: IDs

typedef U64 CFG_ID;

typedef struct CFG_IDNode CFG_IDNode;
struct CFG_IDNode
{
  CFG_IDNode *next;
  CFG_ID v;
};

typedef struct CFG_IDList CFG_IDList;
struct CFG_IDList
{
  CFG_IDNode *first;
  CFG_IDNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Tree Types

typedef struct CFG_Node CFG_Node;
struct CFG_Node
{
  CFG_Node *first;
  CFG_Node *last;
  CFG_Node *next;
  CFG_Node *prev;
  CFG_Node *parent;
  CFG_ID id;
  String8 string;
};

typedef struct CFG_NodePtrNode CFG_NodePtrNode;
struct CFG_NodePtrNode
{
  CFG_NodePtrNode *next;
  CFG_NodePtrNode *prev;
  CFG_Node *v;
};

typedef struct CFG_NodePtrSlot CFG_NodePtrSlot;
struct CFG_NodePtrSlot
{
  CFG_NodePtrNode *first;
  CFG_NodePtrNode *last;
};

typedef struct CFG_NodePtrList CFG_NodePtrList;
struct CFG_NodePtrList
{
  CFG_NodePtrNode *first;
  CFG_NodePtrNode *last;
  U64 count;
};

typedef struct CFG_NodePtrArray CFG_NodePtrArray;
struct CFG_NodePtrArray
{
  CFG_Node **v;
  U64 count;
};

typedef struct CFG_NodeRec CFG_NodeRec;
struct CFG_NodeRec
{
  CFG_Node *next;
  S32 push_count;
  S32 pop_count;
};

////////////////////////////////
//~ rjf: Config State Bundles

typedef struct CFG_Ctx CFG_Ctx;
struct CFG_Ctx
{
  CFG_Node *root;
  U64 id_slots_count;
  CFG_NodePtrSlot *id_slots;
  U64 change_gen;
  CFG_ID last_accessed_id;
  CFG_Node *last_accessed;
};

typedef struct CFG_State CFG_State;
struct CFG_State
{
  Arena *arena;
  CFG_Node *free;
  CFG_NodePtrNode *free_id_node;
  U64 id_gen;
  CFG_Ctx ctx;
};

////////////////////////////////
//~ rjf: Schema Table

typedef struct CFG_SchemaNode CFG_SchemaNode;
struct CFG_SchemaNode
{
  CFG_SchemaNode *next;
  String8 name;
  MD_Node *schema;
};

typedef struct CFG_SchemaTable CFG_SchemaTable;
struct CFG_SchemaTable
{
  CFG_SchemaNode **slots;
  U64 slots_count;
};

////////////////////////////////
//~ rjf: Globals

read_only global CFG_Node cfg_nil_node =
{
  &cfg_nil_node,
  &cfg_nil_node,
  &cfg_nil_node,
  &cfg_nil_node,
  &cfg_nil_node,
};

thread_static CFG_Ctx *cfg_ctx = 0;

////////////////////////////////
//~ rjf: ID Functions

internal void cfg_id_list_push(Arena *arena, CFG_IDList *list, CFG_ID id);
internal CFG_IDList cfg_id_list_copy(Arena *arena, CFG_IDList *src);

////////////////////////////////
//~ rjf: Node Pointer Data Structure Functions

internal void cfg_node_ptr_list_push(Arena *arena, CFG_NodePtrList *list, CFG_Node *node);
internal void cfg_node_ptr_list_push_front(Arena *arena, CFG_NodePtrList *list, CFG_Node *node);
#define cfg_node_ptr_list_first(list) ((list)->count ? (list)->first->v : &cfg_nil_node)
#define cfg_node_ptr_list_last(list)  ((list)->count ? (list)->last->v  : &cfg_nil_node)
internal CFG_NodePtrArray cfg_node_ptr_array_from_list(Arena *arena, CFG_NodePtrList *list);

////////////////////////////////
//~ rjf: Schema Data Structure Functions

internal void cfg_schema_table_insert(Arena *arena, CFG_SchemaTable *table, String8 name, MD_Node *schema);
internal MD_NodePtrList cfg_schemas_from_name(Arena *arena, CFG_SchemaTable *table, String8 name);

////////////////////////////////
//~ rjf: Config Reading Functions

//- rjf: context selection
internal void cfg_ctx_select(CFG_Ctx *ctx);

//- rjf: tree navigations
internal CFG_Node *cfg_node_from_id(CFG_ID id);
internal CFG_Node *cfg_node_child_from_string(CFG_Node *parent, String8 string);
internal CFG_Node *cfg_node_child_from_string_or_parent(CFG_Node *parent, String8 string);
internal CFG_NodePtrList cfg_node_child_list_from_string(Arena *arena, CFG_Node *parent, String8 string);
internal CFG_NodePtrList cfg_node_top_level_list_from_string(Arena *arena, String8 string);
internal CFG_NodeRec cfg_node_rec__depth_first(CFG_Node *root, CFG_Node *node);

//- rjf: serialization
internal String8 cfg_string_from_tree(Arena *arena, String8 root_path, CFG_Node *root);

////////////////////////////////
//~ rjf: Config Writing Functions

//- rjf: state creation / destroying
internal CFG_State *cfg_state_alloc(void);
internal void cfg_state_release(CFG_State *state);

//- rjf: state -> ctx
internal CFG_Ctx *cfg_state_ctx(CFG_State *state);

//- rjf: tree building
internal CFG_Node *cfg_node_alloc(CFG_State *state);
internal void cfg_node_release(CFG_State *state, CFG_Node *node);
internal void cfg_node_release_all_children(CFG_State *state, CFG_Node *node);
internal CFG_Node *cfg_node_new(CFG_State *state, CFG_Node *parent, String8 string);
internal CFG_Node *cfg_node_newf(CFG_State *state, CFG_Node *parent, char *fmt, ...);
internal CFG_Node *cfg_node_new_replace(CFG_State *state, CFG_Node *parent, String8 string);
internal CFG_Node *cfg_node_new_replacef(CFG_State *state, CFG_Node *parent, char *fmt, ...);
internal CFG_Node *cfg_node_deep_copy(CFG_State *state, CFG_Node *src_root);
internal void cfg_node_equip_string(CFG_State *state, CFG_Node *node, String8 string);
internal void cfg_node_equip_stringf(CFG_State *state, CFG_Node *node, char *fmt, ...);
internal void cfg_node_insert_child(CFG_State *state, CFG_Node *parent, CFG_Node *prev_child, CFG_Node *new_child);
internal void cfg_node_unhook(CFG_State *state, CFG_Node *parent, CFG_Node *child);
internal CFG_Node *cfg_node_child_from_string_or_alloc(CFG_State *state, CFG_Node *parent, String8 string);

//- rjf: deserialization
internal CFG_NodePtrList cfg_node_ptr_list_from_string(Arena *arena, String8 root_path, String8 string);

#endif // CONFIG_H
