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
  DLLPushBack(list->first, list->last, n);
  list->count += 1;
  n->v = node;
}

internal void
cfg_node_ptr_list_push_front(Arena *arena, CFG_NodePtrList *list, CFG_Node *node)
{
  CFG_NodePtrNode *n = push_array(arena, CFG_NodePtrNode, 1);
  DLLPushFront(list->first, list->last, n);
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
  U64 hash = u64_hash_from_str8(name);
  U64 slot_idx = hash%table->slots_count;
  CFG_SchemaNode *node = 0;
  for(CFG_SchemaNode *n = table->slots[slot_idx]; n != 0; n = n->next)
  {
    if(str8_match(n->name, name, 0))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    node = push_array(arena, CFG_SchemaNode, 1);
    node->name = str8_copy(arena, name);
    node->schema = schema;
    SLLStackPush(table->slots[slot_idx], node);
  }
}

internal MD_Node *
cfg_schema_from_name(CFG_SchemaTable *table, String8 name)
{
  MD_Node *result = &md_nil_node;
  {
    U64 hash = u64_hash_from_str8(name);
    U64 slot_idx = hash%table->slots_count;
    CFG_SchemaNode *node = 0;
    for(CFG_SchemaNode *n = table->slots[slot_idx]; n != 0; n = n->next)
    {
      if(str8_match(n->name, name, 0))
      {
        result = n->schema;
        break;
      }
    }
  }
  return result;
}

internal MD_NodePtrList
cfg_schemas_from_name(Arena *arena, CFG_SchemaTable *table, String8 name)
{
  MD_NodePtrList result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List tasks = {0};
    String8Node seed_task = {0, name};
    str8_list_push_node(&tasks, &seed_task);
    for EachNode(task, String8Node, tasks.first)
    {
      MD_Node *schema = cfg_schema_from_name(table, task->string);
      if(!md_node_is_nil(schema))
      {
        md_node_ptr_list_push_front(arena, &result, schema);
        for MD_EachNode(tag, schema->first_tag)
        {
          if(str8_match(tag->string, str8_lit("inherit"), 0))
          {
            str8_list_push(scratch.arena, &tasks, tag->first->string);
          }
        }
      }
    }
    scratch_end(scratch);
  }
  return result;
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

internal U64
cfg_change_gen(void)
{
  U64 result = 0;
  if(cfg_ctx != 0)
  {
    result = cfg_ctx->change_gen;
  }
  return result;
}

internal CFG_Node *
cfg_node_root(void)
{
  CFG_Node *result = &cfg_nil_node;
  if(cfg_ctx != 0)
  {
    result = cfg_ctx->root;
  }
  return result;
}

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
cfg_string_from_tree(Arena *arena, CFG_SchemaTable *schema_table, String8 root_path, CFG_Node *root)
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
        schemas = cfg_schemas_from_name(scratch.arena, schema_table, parent->string);
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
  state->ctx.id_slots_count = 4096;
  state->ctx.id_slots = push_array(arena, CFG_NodePtrSlot, state->ctx.id_slots_count);
  state->ctx.root = cfg_node_alloc(state);
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

//- rjf: string allocations

internal U64
cfg_string_bucket_num_from_size(U64 size)
{
  U64 bucket_num = 0;
  if(size > 0)
  {
    for EachElement(idx, cfg_string_bucket_chunk_sizes)
    {
      if(size <= cfg_string_bucket_chunk_sizes[idx])
      {
        bucket_num = idx+1;
        break;
      }
    }
  }
  return bucket_num;
}

internal String8
cfg_string_alloc(CFG_State *state, String8 string)
{
  //- rjf: allocate node
  CFG_StringChunkNode *node = 0;
  {
    U64 bucket_num = cfg_string_bucket_num_from_size(string.size);
    if(bucket_num == ArrayCount(cfg_string_bucket_chunk_sizes))
    {
      CFG_StringChunkNode *best_node = 0;
      CFG_StringChunkNode *best_node_prev = 0;
      U64 best_node_size = max_U64;
      {
        for(CFG_StringChunkNode *n = state->free_string_chunks[bucket_num-1], *prev = 0; n != 0; (prev = n, n = n->next))
        {
          if(n->size >= string.size && n->size < best_node_size)
          {
            best_node = n;
            best_node_prev = prev;
            best_node_size = n->size;
          }
        }
      }
      if(best_node != 0)
      {
        node = best_node;
        if(best_node_prev)
        {
          best_node_prev->next = best_node->next;
        }
        else
        {
          state->free_string_chunks[bucket_num-1] = best_node->next;
        }
      }
      else
      {
        U64 chunk_size = u64_up_to_pow2(string.size);
        node = (CFG_StringChunkNode *)push_array(state->arena, U8, chunk_size);
      }
    }
    else if(bucket_num != 0)
    {
      node = state->free_string_chunks[bucket_num-1];
      if(node != 0)
      {
        SLLStackPop(state->free_string_chunks[bucket_num-1]);
      }
      else
      {
        node = (CFG_StringChunkNode *)push_array(state->arena, U8, cfg_string_bucket_chunk_sizes[bucket_num-1]);
      }
    }
  }
  
  //- rjf: fill node
  String8 result = {0};
  if(node != 0)
  {
    result.str = (U8 *)node;
    result.size = string.size;
    MemoryCopy(result.str, string.str, result.size);
  }
  return result;
}

internal void
cfg_string_release(CFG_State *state, String8 string)
{
  U64 bucket_num = cfg_string_bucket_num_from_size(string.size);
  if(1 <= bucket_num && bucket_num <= ArrayCount(cfg_string_bucket_chunk_sizes))
  {
    U64 bucket_idx = bucket_num-1;
    CFG_StringChunkNode *node = (CFG_StringChunkNode *)string.str;
    SLLStackPush(state->free_string_chunks[bucket_idx], node);
    node->size = u64_up_to_pow2(string.size);
  }
}

//- rjf: tree building

internal CFG_Node *
cfg_node_alloc(CFG_State *state)
{
  state->ctx.change_gen += 1;
  
  // rjf: allocate
  CFG_Node *result = state->free;
  {
    if(result)
    {
      SLLStackPop(state->free);
    }
    else
    {
      result = push_array_no_zero(state->arena, CFG_Node, 1);
    }
  }
  
  // rjf: generate ID & fill
  state->id_gen += 1;
  MemoryZeroStruct(result);
  result->first = result->last = result->next = result->prev = result->parent = &cfg_nil_node;
  result->id = state->id_gen;
  
  // rjf: store to ID -> cfg map
  {
    CFG_NodePtrNode *cfg_id_node = state->free_id_node;
    if(cfg_id_node != 0)
    {
      SLLStackPop(state->free_id_node);
    }
    else
    {
      cfg_id_node = push_array(state->arena, CFG_NodePtrNode, 1);
    }
    U64 hash = u64_hash_from_str8(str8_struct(&result->id));
    U64 slot_idx = hash%state->ctx.id_slots_count;
    DLLPushBack(state->ctx.id_slots[slot_idx].first, state->ctx.id_slots[slot_idx].last, cfg_id_node);
    cfg_id_node->v = result;
  }
  
  return result;
}

internal void
cfg_node_release(CFG_State *state, CFG_Node *node)
{
  state->ctx.change_gen += 1;
  
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: unhook from context
  cfg_node_unhook(state, node->parent, node);
  
  // rjf: gather root & all descendants
  CFG_NodePtrList nodes = {0};
  for(CFG_Node *c = node; c != &cfg_nil_node; c = cfg_node_rec__depth_first(node, c).next)
  {
    cfg_node_ptr_list_push(scratch.arena, &nodes, c);
  }
  
  // rjf: release all nodes
  for(CFG_NodePtrNode *n = nodes.first; n != 0; n = n->next)
  {
    CFG_Node *c = n->v;
    cfg_string_release(state, c->string);
    SLLStackPush(state->free, c);
    c->first = c->last = c->prev = c->parent = 0;
    c->id = 0;
    c->string = str8_zero();
    U64 hash = u64_hash_from_str8(str8_struct(&c->id));
    U64 slot_idx = hash%state->ctx.id_slots_count;
    for(CFG_NodePtrNode *n = state->ctx.id_slots[slot_idx].first; n != 0; n = n->next)
    {
      if(n->v == c)
      {
        DLLRemove(state->ctx.id_slots[slot_idx].first, state->ctx.id_slots[slot_idx].last, n);
        SLLStackPush(state->free_id_node, n);
        break;
      }
    }
  }
  
  scratch_end(scratch);
}

internal void
cfg_node_release_all_children(CFG_State *state, CFG_Node *node)
{
  for(CFG_Node *child = node->first, *next = &cfg_nil_node; child != &cfg_nil_node; child = next)
  {
    next = child->next;
    cfg_node_release(state, child);
  }
}

internal CFG_Node *
cfg_node_new(CFG_State *state, CFG_Node *parent, String8 string)
{
  CFG_Node *node = cfg_node_alloc(state);
  cfg_node_insert_child(state, parent, parent->last, node);
  cfg_node_equip_string(state, node, string);
  return node;
}

internal CFG_Node *
cfg_node_newf(CFG_State *state, CFG_Node *parent, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  CFG_Node *result = cfg_node_new(state, parent, string);
  va_end(args);
  scratch_end(scratch);
  return result;
}

internal CFG_Node *
cfg_node_new_replace(CFG_State *state, CFG_Node *parent, String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  string = push_str8_copy(scratch.arena, string);
  for(CFG_Node *child = parent->first->next, *next = &cfg_nil_node; child != &cfg_nil_node; child = next)
  {
    next = child->next;
    cfg_node_release(state, child);
  }
  if(parent->first == &cfg_nil_node)
  {
    cfg_node_new(state, parent, str8_zero());
  }
  CFG_Node *child = parent->first;
  cfg_node_equip_string(state, child, string);
  scratch_end(scratch);
  return child;
}

internal CFG_Node *
cfg_node_new_replacef(CFG_State *state, CFG_Node *parent, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  CFG_Node *result = cfg_node_new_replace(state, parent, string);
  va_end(args);
  scratch_end(scratch);
  return result;
}

internal CFG_Node *
cfg_node_deep_copy(CFG_State *state, CFG_Node *src_root)
{
  CFG_NodeRec rec = {0};
  CFG_Node *dst_root = &cfg_nil_node;
  CFG_Node *dst_parent = &cfg_nil_node;
  for(CFG_Node *src = src_root; src != &cfg_nil_node; src = rec.next)
  {
    CFG_Node *dst = cfg_node_new(state, dst_parent, src->string);
    if(dst_root == &cfg_nil_node)
    {
      dst_root = dst;
    }
    rec = cfg_node_rec__depth_first(src_root, src);
    if(rec.push_count > 0)
    {
      dst_parent = dst;
    }
    else for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1)
    {
      dst_parent = dst_parent->parent;
    }
  }
  return dst_root;
}

internal void
cfg_node_equip_string(CFG_State *state, CFG_Node *node, String8 string)
{
  cfg_string_release(state, node->string);
  node->string = cfg_string_alloc(state, string);
  state->ctx.change_gen += 1;
}

internal void
cfg_node_equip_stringf(CFG_State *state, CFG_Node *node, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  cfg_node_equip_string(state, node, string);
  va_end(args);
  scratch_end(scratch);
}

internal void
cfg_node_insert_child(CFG_State *state, CFG_Node *parent, CFG_Node *prev_child, CFG_Node *new_child)
{
  if(parent != &cfg_nil_node)
  {
    if(new_child->parent != &cfg_nil_node)
    {
      cfg_node_unhook(state, new_child->parent, new_child);
    }
    DLLInsert_NPZ(&cfg_nil_node, parent->first, parent->last, prev_child, new_child, next, prev);
    new_child->parent = parent;
  }
}

internal void
cfg_node_unhook(CFG_State *state, CFG_Node *parent, CFG_Node *child)
{
  if(child != &cfg_nil_node && parent == child->parent && parent != &cfg_nil_node)
  {
    DLLRemove_NPZ(&cfg_nil_node, parent->first, parent->last, child, next, prev);
    child->parent = &cfg_nil_node;
  }
}

internal CFG_Node *
cfg_node_child_from_string_or_alloc(CFG_State *state, CFG_Node *parent, String8 string)
{
  CFG_Node *node = cfg_node_child_from_string(parent, string);
  if(node == &cfg_nil_node)
  {
    node = cfg_node_new(state, parent, string);
  }
  return node;
}

//- rjf: deserialization

internal CFG_NodePtrList
cfg_node_ptr_list_from_string(Arena *arena, CFG_State *state, CFG_SchemaTable *schema_table, String8 root_path, String8 string)
{
  CFG_NodePtrList result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: parse the string as metadesk
  MD_Node *root = md_tree_from_string(scratch.arena, string);
  
  //- rjf: iterate the top-level metadesk trees, generate new cfg trees for each
  for MD_EachNode(tln, root->first)
  {
    CFG_Node *dst_root_n = &cfg_nil_node;
    CFG_Node *dst_active_parent_n = &cfg_nil_node;
    MD_NodeRec rec = {0};
    for(MD_Node *src_n = tln; !md_node_is_nil(src_n); src_n = rec.next)
    {
      // rjf: lookup schema for this string
      MD_Node *schema = &md_nil_node;
      {
        MD_NodePtrList schemas = cfg_schemas_from_name(scratch.arena, schema_table, dst_active_parent_n->parent->string);
        for(MD_NodePtrNode *n = schemas.first; n != 0 && schema == &md_nil_node; n = n->next)
        {
          schema = md_child_from_string(n->v, dst_active_parent_n->string, 0);
        }
      }
      
      // rjf: extract & transform metadesk node's string (it is raw textual data, so we need to
      // go escaped -> raw, and derelativize paths)
      String8 dst_n_string = {0};
      {
        String8 src_n_string = src_n->string;
        String8 src_n_string__raw = raw_from_escaped_str8(scratch.arena, src_n_string);
        if(!md_node_has_tag(schema->first, str8_lit("no_relativize"), 0))
        {
          if(str8_match(schema->first->string, str8_lit("path"), 0))
          {
            src_n_string__raw = path_absolute_dst_from_relative_dst_src(scratch.arena, src_n_string__raw, root_path);
          }
          else if(str8_match(schema->first->string, str8_lit("path_pt"), 0))
          {
            String8TxtPtPair parts = str8_txt_pt_pair_from_string(src_n_string__raw);
            src_n_string__raw = push_str8f(scratch.arena, "%S:%I64d:%I64d", path_absolute_dst_from_relative_dst_src(scratch.arena, parts.string, root_path), parts.pt.line, parts.pt.column);
          }
        }
        dst_n_string = src_n_string__raw;
      }
      
      // rjf: allocate, fill, & insert new cfg for this metadesk node
      CFG_Node *dst_n = cfg_node_alloc(state);
      cfg_node_equip_string(state, dst_n, dst_n_string);
      if(dst_active_parent_n != &cfg_nil_node)
      {
        cfg_node_insert_child(state, dst_active_parent_n, dst_active_parent_n->last, dst_n);
      }
      
      // rjf: recurse
      rec = md_node_rec_depth_first_pre(src_n, tln);
      if(dst_active_parent_n == &cfg_nil_node)
      {
        dst_root_n = dst_n;
      }
      if(rec.push_count > 0)
      {
        dst_active_parent_n = dst_n;
      }
      else for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1)
      {
        dst_active_parent_n = dst_active_parent_n->parent;
      }
    }
    cfg_node_ptr_list_push(arena, &result, dst_root_n);
  }
  scratch_end(scratch);
  return result;
}
