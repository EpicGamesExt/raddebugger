// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal CFG_KeyMap *
cfg_key_map_from_cfg(Arena *arena)
{
  Temp scratch = scratch_begin(&arena, 1);
  CFG_KeyMap *key_map = push_array(arena, CFG_KeyMap, 1);
  {
    key_map->name_slots_count = 4096;
    key_map->name_slots = push_array(arena, CFG_KeyMapSlot, key_map->name_slots_count);
    key_map->binding_slots_count = 4096;
    key_map->binding_slots = push_array(arena, CFG_KeyMapSlot, key_map->binding_slots_count);
    
    //- rjf: gather & parse all explicitly stored keybinding sets
    CFG_NodePtrList keybindings_cfg_list = cfg_node_top_level_list_from_string(scratch.arena, str8_lit("keybindings"));
    for(CFG_NodePtrNode *n = keybindings_cfg_list.first; n != 0; n = n->next)
    {
      CFG_Node *keybindings_root = n->v;
      for(CFG_Node *keybinding = keybindings_root->first; keybinding != &cfg_nil_node; keybinding = keybinding->next)
      {
        String8 name = {0};
        CFG_Binding binding = {0};
        for(CFG_Node *child = keybinding->first; child != &cfg_nil_node; child = child->next)
        {
          if(0){}
          else if(str8_match(child->string, str8_lit("ctrl"), 0))   { binding.modifiers |= OS_Modifier_Ctrl; }
          else if(str8_match(child->string, str8_lit("alt"), 0))    { binding.modifiers |= OS_Modifier_Alt; }
          else if(str8_match(child->string, str8_lit("shift"), 0))  { binding.modifiers |= OS_Modifier_Shift; }
          else
          {
            OS_Key key = OS_Key_Null;
            for EachEnumVal(OS_Key, k)
            {
              if(str8_match(child->string, os_g_key_cfg_string_table[k], StringMatchFlag_CaseInsensitive))
              {
                key = k;
                break;
              }
            }
            if(key != OS_Key_Null)
            {
              binding.key = key;
            }
            else
            {
              name = child->string;
            }
          }
        }
        if(name.size != 0)
        {
          U64 name_hash = d_hash_from_string(name);
          U64 binding_hash = d_hash_from_string(str8_struct(&binding));
          U64 name_slot_idx = name_hash%key_map->name_slots_count;
          U64 binding_slot_idx = binding_hash%key_map->binding_slots_count;
          CFG_KeyMapNode *n = push_array(arena, CFG_KeyMapNode, 1);
          n->cfg_id = keybinding->id;
          n->name = push_str8_copy(arena, name);
          n->binding = binding;
          SLLQueuePush_N(key_map->name_slots[name_slot_idx].first, key_map->name_slots[name_slot_idx].last, n, name_hash_next);
          SLLQueuePush_N(key_map->binding_slots[binding_slot_idx].first, key_map->binding_slots[binding_slot_idx].last, n, binding_hash_next);
        }
      }
    }
  }
  scratch_end(scratch);
  return key_map;
}

internal CFG_KeyMapNodePtrList
cfg_key_map_node_ptr_list_from_name(Arena *arena, CFG_KeyMap *key_map, String8 string)
{
  CFG_KeyMapNodePtrList list = {0};
  {
    U64 hash = d_hash_from_string(string);
    U64 slot_idx = hash%key_map->name_slots_count;
    for(CFG_KeyMapNode *n = key_map->name_slots[slot_idx].first; n != 0; n = n->name_hash_next)
    {
      if(str8_match(n->name, string, 0))
      {
        CFG_KeyMapNodePtr *ptr = push_array(arena, CFG_KeyMapNodePtr, 1);
        ptr->v = n;
        SLLQueuePush(list.first, list.last, ptr);
        list.count += 1;
      }
    }
  }
  return list;
}

internal CFG_KeyMapNodePtrList
cfg_key_map_node_ptr_list_from_binding(Arena *arena, CFG_KeyMap *key_map, CFG_Binding binding)
{
  CFG_KeyMapNodePtrList list = {0};
  {
    U64 hash = d_hash_from_string(str8_struct(&binding));
    U64 slot_idx = hash%key_map->binding_slots_count;
    for(CFG_KeyMapNode *n = key_map->binding_slots[slot_idx].first; n != 0; n = n->binding_hash_next)
    {
      if(MemoryMatchStruct(&binding, &n->binding))
      {
        CFG_KeyMapNodePtr *ptr = push_array(arena, CFG_KeyMapNodePtr, 1);
        ptr->v = n;
        SLLQueuePush(list.first, list.last, ptr);
        list.count += 1;
      }
    }
  }
  return list;
}
