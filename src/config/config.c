// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: ID Functions

internal void
cfg_id_list_push(Arena *arena, CFG_IDList *list, CFG_ID id)
{
  CFG_IDNode *n = push_array(arena, CFG_IDNode, 1);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  n->v = id;
}

internal CFG_IDList
cfg_id_list_copy(Arena *arena, CFG_IDList *src)
{
  CFG_IDList dst = {0};
  for EachNode(n, CFG_IDNode, src->first)
  {
    cfg_id_list_push(arena, &dst, n->v);
  }
  return dst;
}

////////////////////////////////
//~ rjf: Node Pointer Data Structure Functions

internal void
cfg_node_ptr_list_push(Arena *arena, CFG_NodePtrList *list, CFG_Node *node)
{
  CFG_NodePtrNode *n = push_array(arena, CFG_NodePtrNode, 1);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  n->v = node;
}

internal void
cfg_node_ptr_list_push_front(Arena *arena, CFG_NodePtrList *list, CFG_Node *node)
{
  CFG_NodePtrNode *n = push_array(arena, CFG_NodePtrNode, 1);
  SLLQueuePushFront(list->first, list->last, n);
  list->count += 1;
  n->v = node;
}

internal CFG_NodePtrArray
cfg_node_ptr_array_from_list(Arena *arena, CFG_NodePtrList *list)
{
  CFG_NodePtrArray array = {0};
  array.count = list->count;
  array.v = push_array_no_zero(arena, CFG_Node *, array.count);
  {
    U64 idx = 0;
    for EachNode(n, CFG_NodePtrNode, list->first)
    {
      array.v[idx] = n->v;
      idx += 1;
    }
  }
  return array;
}

////////////////////////////////
//~ rjf: Schema Data Structure Functions

internal void
cfg_schema_table_insert(Arena *arena, CFG_SchemaTable *table, String8 name, MD_Node *schema)
{
  
}

internal MD_NodePtrList
cfg_schemas_from_name(Arena *arena, CFG_SchemaTable *table, String8 name)
{
  
}

////////////////////////////////
//~ rjf: Config Reading Functions

//- rjf: context selection

internal void
cfg_ctx_select(CFG_Ctx *ctx)
{
  cfg_ctx = ctx;
}

//- rjf: tree navigations

internal CFG_Node *
cfg_node_from_id(CFG_ID id)
{
  CFG_Node *result = &cfg_nil_node;
  if(id != 0 &&
     id == cfg_ctx->last_accessed_id &&
     id == cfg_ctx->last_accessed->id)
  {
    result = cfg_ctx->last_accessed;
  }
  else
  {
    U64 hash = u64_hash_from_str8(str8_struct(&id));
    U64 slot_idx = hash%cfg_ctx->id_slots_count;
    for(CFG_NodePtrNode *n = cfg_ctx->id_slots[slot_idx].first; n != 0; n = n->next)
    {
      if(n->v->id == id)
      {
        result = n->v;
        break;
      }
    }
  }
  cfg_ctx->last_accessed_id = id;
  cfg_ctx->last_accessed = result;
  return result;
}

internal CFG_Node *
cfg_node_child_from_string(CFG_Node *parent, String8 string)
{
  CFG_Node *child = &cfg_nil_node;
  if(string.size != 0)
  {
    for(CFG_Node *c = parent->first; c != &cfg_nil_node; c = c->next)
    {
      if(str8_match(c->string, string, 0))
      {
        child = c;
        break;
      }
    }
  }
  return child;
}

internal CFG_Node *
cfg_node_child_from_string_or_parent(CFG_Node *parent, String8 string)
{
  CFG_Node *result = cfg_node_child_from_string(parent, string);
  if(result == &cfg_nil_node)
  {
    result = parent;
  }
  return result;
}

internal CFG_NodePtrList
cfg_node_child_list_from_string(Arena *arena, CFG_Node *parent, String8 string)
{
  CFG_NodePtrList result = {0};
  for(CFG_Node *child = parent->first; child != &cfg_nil_node; child = child->next)
  {
    if(str8_match(child->string, string, 0))
    {
      cfg_node_ptr_list_push(arena, &result, child);
    }
  }
  return result;
}

internal CFG_NodePtrList
cfg_node_top_level_list_from_string(Arena *arena, String8 string)
{
  CFG_NodePtrList result = {0};
  for(CFG_Node *bucket = cfg_ctx->root->first; bucket != &cfg_nil_node; bucket = bucket->next)
  {
    for(CFG_Node *tln = bucket->first; tln != &cfg_nil_node; tln = tln->next)
    {
      if(str8_match(tln->string, string, 0))
      {
        cfg_node_ptr_list_push(arena, &result, tln);
      }
    }
  }
  return result;
}

internal CFG_NodeRec
cfg_node_rec__depth_first(CFG_Node *root, CFG_Node *node)
{
  CFG_NodeRec rec = {&cfg_nil_node};
  if(node->first != &cfg_nil_node)
  {
    rec.next = node->first;
    rec.push_count = 1;
  }
  else for(CFG_Node *p = node; p != root; p = p->parent, rec.pop_count += 1)
  {
    if(p->next != &cfg_nil_node)
    {
      rec.next = p->next;
      break;
    }
  }
  return rec;
}

//- rjf: serialization

internal String8
cfg_string_from_tree(Arena *arena, String8 root_path, CFG_Node *root)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strings = {0};
  {
    typedef struct NestTask NestTask;
    struct NestTask
    {
      NestTask *next;
      CFG_Node *cfg;
      MD_Node *schema;
      B32 is_simple;
    };
    NestTask *top_nest_task = 0;
    CFG_NodeRec rec = {0};
    for(CFG_Node *c = root; c != &cfg_nil_node; c = rec.next)
    {
      // rjf: look up parent's schemas
      MD_NodePtrList schemas = {0};
      if(top_nest_task != 0)
      {
        CFG_Node *parent = top_nest_task->cfg;
        schemas = rd_schemas_from_name(parent->string);
      }
      
      // rjf: look up child schema
      MD_Node *c_schema = &md_nil_node;
      for(MD_NodePtrNode *n = schemas.first; n != 0 && c_schema == &md_nil_node; n = n->next)
      {
        c_schema = md_child_from_string(n->v, c->string, 0);
      }
      
      // rjf: push name of this node
      if(c->string.size != 0 || c->first == &cfg_nil_node)
      {
        // rjf: extract the textualized form for this string (we may need to escape / relativize)
        String8 c_serialized_string = c->string;
        {
          MD_Node *c_schema = &md_nil_node;
          if(top_nest_task != 0)
          {
            c_schema = top_nest_task->schema;
          }
          
          // rjf: paths -> relativize
          if(!md_node_has_tag(c_schema->first, str8_lit("no_relativize"), 0))
          {
            if(str8_match(c_schema->first->string, str8_lit("path"), 0))
            {
              String8 path_absolute = c->string;
              String8 path_relative = path_relative_dst_from_absolute_dst_src(arena, path_absolute, root_path);
              c_serialized_string = path_relative;
            }
            else if(str8_match(c_schema->first->string, str8_lit("path_pt"), 0))
            {
              String8 value = c->string;
              String8TxtPtPair parts = str8_txt_pt_pair_from_string(value);
              String8 path_relative = path_relative_dst_from_absolute_dst_src(scratch.arena, parts.string, root_path);
              c_serialized_string = push_str8f(arena, "%S:%I64d:%I64d", path_relative, parts.pt.line, parts.pt.column);
            }
          }
          
          // rjf: all strings -> escape
          c_serialized_string = escaped_from_raw_str8(arena, c_serialized_string);
        }
        
        // rjf: generate all strings for this node's string
        String8List c_name_strings = {0};
        {
          B32 name_can_be_pushed_standalone = 0;
          {
            Temp temp = temp_begin(scratch.arena);
            MD_TokenizeResult c_name_tokenize = md_tokenize_from_text(temp.arena, c_serialized_string);
            name_can_be_pushed_standalone = (c_name_tokenize.tokens.count == 1 && c_name_tokenize.tokens.v[0].flags & (MD_TokenFlag_Identifier|
                                                                                                                       MD_TokenFlag_Numeric|
                                                                                                                       MD_TokenFlag_StringLiteral|
                                                                                                                       MD_TokenFlag_Symbol));
            temp_end(temp);
          }
          if(name_can_be_pushed_standalone)
          {
            str8_list_push(scratch.arena, &c_name_strings, c_serialized_string);
          }
          else
          {
            str8_list_push(scratch.arena, &c_name_strings, str8_lit("\""));
            str8_list_push(scratch.arena, &c_name_strings, c_serialized_string);
            str8_list_push(scratch.arena, &c_name_strings, str8_lit("\""));
          }
        }
        
        // rjf: if we're in a simple nesting task, then just break children by space
        if(top_nest_task != 0 && top_nest_task->is_simple)
        {
          str8_list_push(scratch.arena, &strings, str8_lit(" "));
        }
        
        // rjf: join c's strings with main string list
        str8_list_concat_in_place(&strings, &c_name_strings);
      }
      
      // rjf: grab next recursion
      rec = cfg_node_rec__depth_first(root, c);
      
      // rjf: push a new nesting task before descending to children
      if(c->first != &cfg_nil_node)
      {
        B32 is_simple_children_list = 1;
        for(CFG_Node *child = c->first; child != &cfg_nil_node; child = child->next)
        {
          if(child->first != &cfg_nil_node && child != c->last)
          {
            is_simple_children_list = 0;
            break;
          }
        }
        NestTask *task = push_array(scratch.arena, NestTask, 1);
        task->cfg = c;
        task->schema = c_schema;
        task->is_simple = is_simple_children_list;
        SLLStackPush(top_nest_task, task);
      }
      
      // rjf: tree navigations -> encode hierarchy
      if(rec.push_count > 0)
      {
        if(top_nest_task->is_simple && c->string.size != 0)
        {
          str8_list_push(scratch.arena, &strings, str8_lit(":"));
        }
        else
        {
          if(c->string.size != 0)
          {
            str8_list_push(scratch.arena, &strings, str8_lit(":\n"));
          }
          str8_list_push(scratch.arena, &strings, str8_lit("{"));
        }
      }
      else
      {
        for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1, SLLStackPop(top_nest_task))
        {
          if(top_nest_task->is_simple)
          {
            if(top_nest_task->cfg->string.size == 0)
            {
              str8_list_push(scratch.arena, &strings, str8_lit(" }"));
            }
          }
          else
          {
            str8_list_push(scratch.arena, &strings, str8_lit("\n}"));
          }
        }
      }
      if(!top_nest_task || top_nest_task->is_simple == 0)
      {
        str8_list_push(scratch.arena, &strings, str8_lit("\n"));
      }
    }
  }
  String8 result_unindented = str8_list_join(scratch.arena, &strings, 0);
  String8 result = indented_from_string(arena, result_unindented);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Config Writing Functions

//- rjf: state creation / destroying

internal CFG_State *
cfg_state_alloc(void)
{
  Arena *arena = arena_alloc();
  CFG_State *state = push_array(arena, CFG_State, 1);
  state->arena = arena;
  return state;
}

internal void
cfg_state_release(CFG_State *state)
{
  arena_release(state->arena);
}

//- rjf: state -> ctx

internal CFG_Ctx *
cfg_state_ctx(CFG_State *state)
{
  CFG_Ctx *ctx = &state->ctx;
  return ctx;
}

//- rjf: tree building

internal CFG_Node *
cfg_node_alloc(CFG_State *state)
{
  
}

internal void
cfg_node_release(CFG_State *state, CFG_Node *node)
{
  
}

internal void
cfg_node_release_all_children(CFG_State *state, CFG_Node *node)
{
  
}

internal CFG_Node *
cfg_node_new(CFG_State *state, CFG_Node *parent, String8 string)
{
  
}

internal CFG_Node *
cfg_node_newf(CFG_State *state, CFG_Node *parent, char *fmt, ...)
{
  
}

internal CFG_Node *
cfg_node_new_replace(CFG_State *state, CFG_Node *parent, String8 string)
{
  
}

internal CFG_Node *
cfg_node_new_replacef(CFG_State *state, CFG_Node *parent, char *fmt, ...)
{
  
}

internal CFG_Node *
cfg_node_deep_copy(CFG_State *state, CFG_Node *src_root)
{
  
}

internal void
cfg_node_equip_string(CFG_State *state, CFG_Node *node, String8 string)
{
  
}

internal void
cfg_node_equip_stringf(CFG_State *state, CFG_Node *node, char *fmt, ...)
{
  
}

internal void
cfg_node_insert_child(CFG_State *state, CFG_Node *parent, CFG_Node *prev_child, CFG_Node *new_child)
{
  
}

internal void
cfg_node_unhook(CFG_State *state, CFG_Node *parent, CFG_Node *child)
{
  
}

internal CFG_Node *
cfg_node_child_from_string_or_alloc(CFG_State *state, CFG_Node *parent, String8 string)
{
  
}

//- rjf: deserialization

internal CFG_NodePtrList
cfg_node_ptr_list_from_string(Arena *arena, String8 root_path, String8 string)
{
  
}
