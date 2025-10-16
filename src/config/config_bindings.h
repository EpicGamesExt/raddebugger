// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef CONFIG_BINDINGS_H
#define CONFIG_BINDINGS_H

typedef struct CFG_Binding CFG_Binding;
struct CFG_Binding
{
  OS_Key key;
  OS_Modifiers modifiers;
};

typedef struct CFG_KeyMapNode CFG_KeyMapNode;
struct CFG_KeyMapNode
{
  CFG_KeyMapNode *name_hash_next;
  CFG_KeyMapNode *binding_hash_next;
  CFG_ID cfg_id;
  String8 name;
  CFG_Binding binding;
};

typedef struct CFG_KeyMapNodePtr CFG_KeyMapNodePtr;
struct CFG_KeyMapNodePtr
{
  CFG_KeyMapNodePtr *next;
  CFG_KeyMapNode *v;
};

typedef struct CFG_KeyMapNodePtrList CFG_KeyMapNodePtrList;
struct CFG_KeyMapNodePtrList
{
  CFG_KeyMapNodePtr *first;
  CFG_KeyMapNodePtr *last;
  U64 count;
};

typedef struct CFG_KeyMapSlot CFG_KeyMapSlot;
struct CFG_KeyMapSlot
{
  CFG_KeyMapNode *first;
  CFG_KeyMapNode *last;
};

typedef struct CFG_KeyMap CFG_KeyMap;
struct CFG_KeyMap
{
  U64 name_slots_count;
  CFG_KeyMapSlot *name_slots;
  U64 binding_slots_count;
  CFG_KeyMapSlot *binding_slots;
};

internal CFG_KeyMap *cfg_key_map_from_cfg(Arena *arena);
internal CFG_KeyMapNodePtrList cfg_key_map_node_ptr_list_from_name(Arena *arena, CFG_KeyMap *key_map, String8 string);
internal CFG_KeyMapNodePtrList cfg_key_map_node_ptr_list_from_binding(Arena *arena, CFG_KeyMap *key_map, CFG_Binding binding);

#endif // CONFIG_BINDINGS_H
