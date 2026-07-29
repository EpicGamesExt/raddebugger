// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "eval2/generated/eval2.meta.c"

////////////////////////////////
//~ rjf: Space -> Memory Map Helpers

internal void
e2_space_map_push(Arena *arena, E2_SpaceMap *map, E2_SpaceID space_id, Rng1U64 addr_range, void *data)
{
  if(map->slots_count == 0)
  {
    map->slots_count = 8;
    map->slots = push_array(arena, E2_SpaceMapNode *, map->slots_count);
  }
  U64 hash = u64_hash_from_str8(str8_struct(&space_id));
  U64 slot_idx = hash%map->slots_count;
  E2_SpaceMapNode *node = 0;
  for(E2_SpaceMapNode *n = map->slots[slot_idx]; n != 0; n = n->next)
  {
    if(n->space_id == space_id)
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    node = push_array(arena, E2_SpaceMapNode, 1);
    SLLStackPush(map->slots[slot_idx], node);
  }
  memory_map_push(arena, &node->memory_map, addr_range, data);
}

internal U64
e2_space_map_read(E2_SpaceMap *map, E2_SpaceID space_id, Rng1U64 addr_range, void *out)
{
  U64 result = 0;
  {
    U64 hash = u64_hash_from_str8(str8_struct(&space_id));
    U64 slot_idx = hash%map->slots_count;
    E2_SpaceMapNode *node = 0;
    for(E2_SpaceMapNode *n = map->slots[slot_idx]; n != 0; n = n->next)
    {
      if(n->space_id == space_id)
      {
        node = n;
        break;
      }
    }
    if(node != 0)
    {
      result = memory_map_read(&node->memory_map, addr_range, out);
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Name -> Expr Map Helpers

internal void
e2_expr_map_push(Arena *arena, E2_ExprMap *map, String8 name, E2_Expr *expr)
{
  if(map->slots_count == 0)
  {
    map->slots_count = 8;
    map->slots = push_array(arena, E2_ExprMapNode *, map->slots_count);
  }
  U64 hash = u64_hash_from_str8(name);
  U64 slot_idx = hash%map->slots_count;
  E2_ExprMapNode *node = 0;
  for(E2_ExprMapNode *n = map->slots[slot_idx]; n != 0; n = n->next)
  {
    if(str8_match(n->name, name, 0))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    node = push_array(arena, E2_ExprMapNode, 1);
    node->name = str8_copy(arena, name);
    node->expr = expr;
    SLLStackPush(map->slots[slot_idx], node);
  }
}

internal E2_Expr *
e2_expr_from_name(E2_ExprMap *map, String8 name)
{
  E2_Expr *expr = &e2_expr_nil;
  if(map->slots_count != 0)
  {
    U64 hash = u64_hash_from_str8(name);
    U64 slot_idx = hash%map->slots_count;
    E2_ExprMapNode *node = 0;
    for(E2_ExprMapNode *n = map->slots[slot_idx]; n != 0; n = n->next)
    {
      if(str8_match(n->name, name, 0))
      {
        node = n;
        break;
      }
    }
    if(node != 0)
    {
      expr = node->expr;
    }
  }
  return expr;
}

////////////////////////////////
//~ rjf: Identifier Map Helpers

internal void
e2_identifier_map_push(Arena *arena, E2_IdentifierMap *map, String8 name, E2_IRNode *irtree)
{
  if(map->slots_count == 0)
  {
    map->slots_count = 8;
    map->slots = push_array(arena, E2_IdentifierMapNode *, map->slots_count);
  }
  U64 hash = u64_hash_from_str8(name);
  U64 slot_idx = hash%map->slots_count;
  E2_IdentifierMapNode *node = 0;
  for(E2_IdentifierMapNode *n = map->slots[slot_idx]; n != 0; n = n->next)
  {
    if(str8_match(n->name, name, 0))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    node = push_array(arena, E2_IdentifierMapNode, 1);
    node->name = str8_copy(arena, name);
    node->irtree = irtree;
    SLLStackPush(map->slots[slot_idx], node);
  }
}

internal E2_IRNode *
e2_irtree_from_identifier(E2_IdentifierMap *map, String8 name)
{
  E2_IRNode *irtree = &e2_irnode_nil;
  if(map->slots_count != 0)
  {
    U64 hash = u64_hash_from_str8(name);
    U64 slot_idx = hash%map->slots_count;
    E2_IdentifierMapNode *node = 0;
    for(E2_IdentifierMapNode *n = map->slots[slot_idx]; n != 0; n = n->next)
    {
      if(str8_match(n->name, name, 0))
      {
        node = n;
        break;
      }
    }
    if(node != 0)
    {
      irtree = node->irtree;
    }
  }
  return irtree;
}

////////////////////////////////
//~ rjf: Messages

internal E2_Msg *
e2_msg(Arena *arena, E2_MsgList *msgs, Rng1U64 src_range, String8 string)
{
  E2_Msg *msg = push_array(arena, E2_Msg, 1);
  SLLQueuePush(msgs->first, msgs->last, msg);
  msgs->count += 1;
  msg->src_range = src_range;
  msg->string = str8_copy(arena, string);
  return msg;
}

internal E2_Msg *
e2_msgf(Arena *arena, E2_MsgList *msgs, Rng1U64 src_range, char *fmt, ...)
{
  Temp scratch = scratch_begin(&arena, 1);
  va_list args;
  va_start(args, fmt);
  String8 string = str8fv(scratch.arena, fmt, args);
  E2_Msg *msg = e2_msg(arena, msgs, src_range, string);
  va_end(args);
  scratch_end(scratch);
  return msg;
}

////////////////////////////////
//~ rjf: Assets

internal void
e2_select_assets(E2_Assets *assets)
{
  e2_assets = assets;
}

internal E2_DbgInfo *
e2_dbgi_from_num(U32 num)
{
  E2_DbgInfo *result = &e2_dbg_info_nil;
  if(0 < num && num <= e2_assets->dbg_infos_count)
  {
    result = &e2_assets->dbg_infos[num-1];
  }
  return result;
}

////////////////////////////////
//~ rjf: Constructed Types

internal E2_ConsTypeMap *
e2_cons_type_map_alloc(void)
{
  Arena *arena = arena_alloc();
  E2_ConsTypeMap *map = push_array(arena, E2_ConsTypeMap, 1);
  map->arena = arena;
  map->table_start_arena_pos = arena_pos(arena);
  return map;
}

internal void
e2_select_cons_type_map(E2_ConsTypeMap *map)
{
  arena_pop_to(map->arena, map->table_start_arena_pos);
  map->slots_count = 256;
  map->content_slots = push_array(map->arena, E2_ConsTypeSlot, map->slots_count);
  map->id_slots = push_array(map->arena, E2_ConsTypeSlot, map->slots_count);
  e2_cons_type_map = map;
}

internal B32
e2_cons_type_params_match(E2_ConsTypeParams *a, E2_ConsTypeParams *b)
{
  B32 result = (a->arch == b->arch &&
                a->kind == b->kind &&
                str8_match(a->name, b->name, 0) &&
                e2_type_key_match(a->direct, b->direct) &&
                a->count == b->count &&
                a->depth == b->depth &&
                a->args == 0 && b->args == 0);
  return result;
}

internal E2_ConsTypeNode *
e2_cons_type_node_from_params(E2_ConsTypeParams *params)
{
  E2_ConsTypeMap *map = e2_cons_type_map;
  
  // rjf: params -> hash
  U64 hash = 0;
  {
    hash = u64_hash_from_seed_str8(hash, str8_struct(&params->arch));
    hash = u64_hash_from_seed_str8(hash, str8_struct(&params->kind));
    hash = u64_hash_from_seed_str8(hash, str8_struct(&params->name));
    hash = u64_hash_from_seed_str8(hash, str8_struct(&params->direct));
    hash = u64_hash_from_seed_str8(hash, str8_struct(&params->count));
    hash = u64_hash_from_seed_str8(hash, str8_struct(&params->depth));
    if(params->param_types != 0)
    {
      for EachIndex(idx, params->count)
      {
        hash = u64_hash_from_seed_str8(hash, str8_struct(&params->param_types[idx]));
      }
    }
  }
  
  // rjf: hash -> content slot
  U64 content_slot_idx = hash%map->slots_count;
  E2_ConsTypeSlot *content_slot = &map->content_slots[content_slot_idx];
  
  // rjf: parameters -> existing node
  E2_ConsTypeNode *node = 0;
  for(E2_ConsTypeNode *n = content_slot->first; n != 0; n = n->content_next)
  {
    if(e2_cons_type_params_match(&n->params, params))
    {
      node = n;
      break;
    }
  }
  
  // rjf: if node wasn't found -> push into the map
  if(node == 0)
  {
    // rjf: form node ID
    map->id_gen += 1;
    U64 id = map->id_gen;
    
    // rjf: build the node
    node = push_array(map->arena, E2_ConsTypeNode, 1);
    SLLQueuePush_N(content_slot->first, content_slot->last, node, content_next);
    node->id = id;
    MemoryCopyStruct(&node->params, params);
    node->params.name = str8_copy(map->arena, node->params.name);
    if(node->params.param_types != 0)
    {
      node->params.param_types = push_array(map->arena, E2_TypeKey, node->params.count);
      MemoryCopy(node->params.param_types, params->param_types, sizeof(params->param_types[0])*params->count);
    }
    // TODO(rjf): double check once we're doing type expression
    // arguments that we don't need a deep copy of the args here.
    switch(node->params.kind)
    {
      default:
      {
        node->byte_size = e2_byte_size_from_type_key(node->params.direct);
      }break;
      case E2_TypeKind_Ptr:
      case E2_TypeKind_Function:
      case E2_TypeKind_Method:
      case E2_TypeKind_MemberPtr:
      case E2_TypeKind_RRef:
      case E2_TypeKind_LRef:
      {
        node->byte_size = byte_size_from_arch(node->params.arch);
      }break;
      case E2_TypeKind_Array:
      {
        node->byte_size = node->params.count * e2_byte_size_from_type_key(node->params.direct);
      }break;
    }
    
    // rjf: insert node into the ID slots
    {
      U64 id_hash = u64_hash_from_str8(str8_struct(&id));
      U64 id_slot_idx = id_hash%map->slots_count;
      E2_ConsTypeSlot *id_slot = &map->id_slots[id_slot_idx];
      SLLQueuePush_N(id_slot->first, id_slot->last, node, id_next);
    }
  }
  
  return node;
}

internal E2_ConsTypeNode *
e2_cons_type_node_from_id(U64 id)
{
  E2_ConsTypeMap *map = e2_cons_type_map;
  U64 id_hash = u64_hash_from_str8(str8_struct(&id));
  U64 id_slot_idx = id_hash%map->slots_count;
  E2_ConsTypeSlot *id_slot = &map->id_slots[id_slot_idx];
  E2_ConsTypeNode *node = &e2_cons_type_node_nil;
  for(E2_ConsTypeNode *n = id_slot->first; n != 0; n = n->id_next)
  {
    if(n->id == id)
    {
      node = n;
      break;
    }
  }
  return node;
}

internal E2_TypeKey
e2_type_key_from_cons_type_node(E2_ConsTypeNode *node)
{
  E2_TypeKey key =
  {
    .kind = E2_TypeKeyKind_Cons,
    .u32[0] = node->params.kind,
    .u32[1] = (U32)((node->id&0xffffffff00000000ull) >> 32),
    .u32[2] = node->id&0xffffffffull,
  };
  return key;
}

////////////////////////////////
//~ rjf: Type Keys

//- rjf: RDI -> eval enum conversions

internal E2_TypeKind
e2_type_kind_from_rdi(RDI_TypeKind k)
{
  E2_TypeKind result = E2_TypeKind_Null;
  switch((RDI_TypeKindEnum)k)
  {
    case RDI_TypeKind_Null:{}break;
    case RDI_TypeKind_COUNT:{}break;
    case RDI_TypeKind_Void:                   {result = E2_TypeKind_Void;}break;
    case RDI_TypeKind_Handle:                 {result = E2_TypeKind_Handle;}break;
    case RDI_TypeKind_HResult:                {result = E2_TypeKind_HResult;}break;
    case RDI_TypeKind_Char8:                  {result = E2_TypeKind_Char8;}break;
    case RDI_TypeKind_Char16:                 {result = E2_TypeKind_Char16;}break;
    case RDI_TypeKind_Char32:                 {result = E2_TypeKind_Char32;}break;
    case RDI_TypeKind_UChar8:                 {result = E2_TypeKind_UChar8;}break;
    case RDI_TypeKind_UChar16:                {result = E2_TypeKind_UChar16;}break;
    case RDI_TypeKind_UChar32:                {result = E2_TypeKind_UChar32;}break;
    case RDI_TypeKind_U8:                     {result = E2_TypeKind_U8;}break;
    case RDI_TypeKind_U16:                    {result = E2_TypeKind_U16;}break;
    case RDI_TypeKind_U32:                    {result = E2_TypeKind_U32;}break;
    case RDI_TypeKind_U64:                    {result = E2_TypeKind_U64;}break;
    case RDI_TypeKind_U128:                   {result = E2_TypeKind_U128;}break;
    case RDI_TypeKind_U256:                   {result = E2_TypeKind_U256;}break;
    case RDI_TypeKind_U512:                   {result = E2_TypeKind_U512;}break;
    case RDI_TypeKind_S8:                     {result = E2_TypeKind_S8;}break;
    case RDI_TypeKind_S16:                    {result = E2_TypeKind_S16;}break;
    case RDI_TypeKind_S32:                    {result = E2_TypeKind_S32;}break;
    case RDI_TypeKind_S64:                    {result = E2_TypeKind_S64;}break;
    case RDI_TypeKind_S128:                   {result = E2_TypeKind_S128;}break;
    case RDI_TypeKind_S256:                   {result = E2_TypeKind_S256;}break;
    case RDI_TypeKind_S512:                   {result = E2_TypeKind_S512;}break;
    case RDI_TypeKind_Bool:                   {result = E2_TypeKind_Bool;}break;
    case RDI_TypeKind_F16:                    {result = E2_TypeKind_F16;}break;
    case RDI_TypeKind_F32:                    {result = E2_TypeKind_F32;}break;
    case RDI_TypeKind_F32PP:                  {result = E2_TypeKind_F32PP;}break;
    case RDI_TypeKind_F48:                    {result = E2_TypeKind_F48;}break;
    case RDI_TypeKind_F64:                    {result = E2_TypeKind_F64;}break;
    case RDI_TypeKind_F80:                    {result = E2_TypeKind_F80;}break;
    case RDI_TypeKind_F128:                   {result = E2_TypeKind_F128;}break;
    case RDI_TypeKind_ComplexF32:             {result = E2_TypeKind_ComplexF32;}break;
    case RDI_TypeKind_ComplexF64:             {result = E2_TypeKind_ComplexF64;}break;
    case RDI_TypeKind_ComplexF80:             {result = E2_TypeKind_ComplexF80;}break;
    case RDI_TypeKind_ComplexF128:            {result = E2_TypeKind_ComplexF128;}break;
    case RDI_TypeKind_Modifier:               {result = E2_TypeKind_Modifier;}break;
    case RDI_TypeKind_Ptr:                    {result = E2_TypeKind_Ptr;}break;
    case RDI_TypeKind_LRef:                   {result = E2_TypeKind_LRef;}break;
    case RDI_TypeKind_RRef:                   {result = E2_TypeKind_RRef;}break;
    case RDI_TypeKind_Array:                  {result = E2_TypeKind_Array;}break;
    case RDI_TypeKind_Function:               {result = E2_TypeKind_Function;}break;
    case RDI_TypeKind_Method:                 {result = E2_TypeKind_Method;}break;
    case RDI_TypeKind_MemberPtr:              {result = E2_TypeKind_MemberPtr;}break;
    case RDI_TypeKind_Struct:                 {result = E2_TypeKind_Struct;}break;
    case RDI_TypeKind_Class:                  {result = E2_TypeKind_Class;}break;
    case RDI_TypeKind_Union:                  {result = E2_TypeKind_Union;}break;
    case RDI_TypeKind_Enum:                   {result = E2_TypeKind_Enum;}break;
    case RDI_TypeKind_Alias:                  {result = E2_TypeKind_Alias;}break;
    case RDI_TypeKind_IncompleteStruct:       {result = E2_TypeKind_IncompleteStruct;}break;
    case RDI_TypeKind_IncompleteUnion:        {result = E2_TypeKind_IncompleteUnion;}break;
    case RDI_TypeKind_IncompleteClass:        {result = E2_TypeKind_IncompleteClass;}break;
    case RDI_TypeKind_IncompleteEnum:         {result = E2_TypeKind_IncompleteEnum;}break;
    case RDI_TypeKind_Bitfield:               {result = E2_TypeKind_Bitfield;}break;
    case RDI_TypeKind_Variadic:               {result = E2_TypeKind_Variadic;}break;
    
    case RDI_TypeKind_BF16:
    case RDI_TypeKind_F96:
    case RDI_TypeKind_Decimal32:
    case RDI_TypeKind_Decimal64:
    case RDI_TypeKind_Decimal128:
    {
      // NOTE(rjf): currently unsupported
    }break;
  }
  return result;
}

internal RDI_EvalTypeGroup
e2_type_group_from_kind(E2_TypeKind k)
{
  RDI_EvalTypeGroup result = 0;
  switch(k)
  {
    case E2_TypeKind_COUNT:{}break;
    
    case E2_TypeKind_Null: case E2_TypeKind_Void:
    case E2_TypeKind_F16:  case E2_TypeKind_F32PP: case E2_TypeKind_F48:
    case E2_TypeKind_F80:  case E2_TypeKind_F128:
    case E2_TypeKind_ComplexF32: case E2_TypeKind_ComplexF64:
    case E2_TypeKind_ComplexF80: case E2_TypeKind_ComplexF128:
    case E2_TypeKind_Modifier:   case E2_TypeKind_Array:
    case E2_TypeKind_Struct:     case E2_TypeKind_Class: case E2_TypeKind_Union:
    case E2_TypeKind_Enum:       case E2_TypeKind_Alias:
    case E2_TypeKind_IncompleteStruct: case E2_TypeKind_IncompleteClass:
    case E2_TypeKind_IncompleteUnion:  case E2_TypeKind_IncompleteEnum:
    case E2_TypeKind_Bitfield:
    case E2_TypeKind_Variadic:
    case E2_TypeKind_HResult:
    case E2_TypeKind_Set:
    case E2_TypeKind_Lens:
    case E2_TypeKind_LensSpec:
    case E2_TypeKind_MetaExpr:
    case E2_TypeKind_MetaDisplayName:
    case E2_TypeKind_MetaDescription:
    {result = RDI_EvalTypeGroup_Other;}break;
    
    case E2_TypeKind_Handle:
    case E2_TypeKind_UChar8: case E2_TypeKind_UChar16: case E2_TypeKind_UChar32:
    case E2_TypeKind_U8:     case E2_TypeKind_U16:     case E2_TypeKind_U32:
    case E2_TypeKind_U64:    case E2_TypeKind_U128:    case E2_TypeKind_U256:
    case E2_TypeKind_U512:
    case E2_TypeKind_Ptr: case E2_TypeKind_LRef: case E2_TypeKind_RRef:
    case E2_TypeKind_Function: case E2_TypeKind_Method: case E2_TypeKind_MemberPtr:
    {result = RDI_EvalTypeGroup_U;}break;
    
    case E2_TypeKind_Char8: case E2_TypeKind_Char16: case E2_TypeKind_Char32:
    case E2_TypeKind_S8:    case E2_TypeKind_S16:    case E2_TypeKind_S32:
    case E2_TypeKind_S64:   case E2_TypeKind_S128:   case E2_TypeKind_S256:
    case E2_TypeKind_S512:
    case E2_TypeKind_Bool:
    {result = RDI_EvalTypeGroup_S;}break;
    
    case E2_TypeKind_F32:{result = RDI_EvalTypeGroup_F32;}break;
    case E2_TypeKind_F64:{result = RDI_EvalTypeGroup_F64;}break;
  }
  return result;
}

internal B32
e2_type_kind_is_ptr_or_ref(E2_TypeKind k)
{
  B32 result = (k == E2_TypeKind_Ptr || k == E2_TypeKind_LRef || k == E2_TypeKind_RRef);
  return result;
}

internal B32
e2_type_kind_is_integer(E2_TypeKind k)
{
  B32 result = (E2_TypeKind_FirstInteger <= k && k <= E2_TypeKind_LastInteger);
  return result;
}

internal B32
e2_type_kind_is_float(E2_TypeKind k)
{
  B32 result = (k == E2_TypeKind_F32 || k == E2_TypeKind_F64);
  return result;
}

internal B32
e2_type_kind_is_signed(E2_TypeKind k)
{
  B32 result = ((E2_TypeKind_FirstSigned1 <= k && k <= E2_TypeKind_LastSigned1) ||
                (E2_TypeKind_FirstSigned2 <= k && k <= E2_TypeKind_LastSigned2));
  return result;
}

internal B32
e2_type_kind_is_unsigned(E2_TypeKind k)
{
  B32 result = !e2_type_kind_is_signed(k) && e2_type_kind_is_integer(k);
  return result;
}

//- rjf: basic key constructors

internal E2_TypeKey
e2_type_key_zero(void)
{
  E2_TypeKey key = {E2_TypeKeyKind_Null};
  return key;
}

internal E2_TypeKey
e2_type_key_basic(E2_TypeKind kind)
{
  E2_TypeKey key = {E2_TypeKeyKind_Basic};
  key.u32[0] = (U32)kind;
  return key;
}

internal E2_TypeKey
e2_type_key_dbgi(E2_TypeKind kind, U32 dbg_info_num, U32 type_idx)
{
  E2_TypeKey key = {E2_TypeKeyKind_DbgInfo};
  key.u32[0] = kind;
  key.u32[1] = dbg_info_num;
  key.u32[2] = type_idx;
  return key;
}

internal E2_TypeKey
e2_type_key_reg(Arch arch, ARCH_RegCode reg_code)
{
  E2_TypeKey key = {E2_TypeKeyKind_Reg};
  key.u32[0] = arch;
  key.u32[1] = reg_code;
  return key;
}

//- rjf: constructed type constructor

internal E2_TypeKey
e2_type_key_cons_(E2_ConsTypeParams *params)
{
  E2_ConsTypeNode *node = e2_cons_type_node_from_params(params);
  E2_TypeKey result = e2_type_key_from_cons_type_node(node);
  return result;
}

//- rjf: basic type key type functions

internal B32
e2_type_key_match(E2_TypeKey a, E2_TypeKey b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

//- rjf: type key -> info

internal E2_TypeKind
e2_type_kind_from_key(E2_TypeKey k)
{
  E2_TypeKind result = E2_TypeKind_Null;
  switch(k.kind)
  {
    case E2_TypeKeyKind_Null:{}break;
    case E2_TypeKeyKind_Basic:
    case E2_TypeKeyKind_DbgInfo:
    case E2_TypeKeyKind_Cons:
    {
      result = (E2_TypeKind)k.u32[0];
    }break;
    case E2_TypeKeyKind_Reg:
    {
      result = E2_TypeKind_Union;
    }break;
  }
  return result;
}

internal U64
e2_byte_size_from_type_key(E2_TypeKey k)
{
  U64 result = 0;
  switch(k.kind)
  {
    case E2_TypeKeyKind_Null:{}break;
    case E2_TypeKeyKind_Basic:
    {
      result = e2_type_kind_basic_byte_size_table[k.u32[0]];
    }break;
    case E2_TypeKeyKind_DbgInfo:
    {
      E2_DbgInfo *dbgi = e2_dbgi_from_type_key(k);
      U32 dbgi_type_idx = e2_dbgi_type_idx_from_key(k);
      RDI_TypeNode *type_node = rdi_element_from_name_idx(dbgi->rdi, TypeNodes, dbgi_type_idx);
      result = type_node->byte_size;
    }break;
    case E2_TypeKeyKind_Cons:
    {
      U64 id = e2_cons_type_id_from_key(k);
      E2_ConsTypeNode *node = e2_cons_type_node_from_id(id);
      result = node->byte_size;
    }break;
    case E2_TypeKeyKind_Reg:
    {
      Arch arch = (Arch)k.u32[0];
      ARCH_Info *arch_info = arch_info_from_arch(arch);
      ARCH_RegCode regcode = k.u32[1];
      if(regcode < arch_info->reg_code_count)
      {
        Rng1U16 regrng = arch_info->reg_code_rng_table[regcode];
        result = (U64)dim_1u16(regrng);
      }
    }break;
  }
  return result;
}

internal U32
e2_dbgi_num_from_type_key(E2_TypeKey k)
{
  U32 result = 0;
  if(k.kind == E2_TypeKeyKind_DbgInfo)
  {
    result = k.u32[1];
  }
  return result;
}

internal E2_DbgInfo *
e2_dbgi_from_type_key(E2_TypeKey k)
{
  U32 dbgi_num = e2_dbgi_num_from_type_key(k);
  E2_DbgInfo *dbg_info = e2_dbgi_from_num(dbgi_num);
  return dbg_info;
}

internal U32
e2_dbgi_type_idx_from_key(E2_TypeKey k)
{
  U32 result = 0;
  if(k.kind == E2_TypeKeyKind_DbgInfo)
  {
    result = k.u32[2];
  }
  return result;
}

internal U64
e2_cons_type_id_from_key(E2_TypeKey k)
{
  U64 result = 0;
  if(k.kind == E2_TypeKeyKind_Cons)
  {
    result = ((U64)k.u32[1] << 32) | k.u32[2];
  }
  return result;
}

internal U64
e2_shift_from_type_key(E2_TypeKey k)
{
  U64 result = 0;
  if(e2_type_kind_from_key(k) == E2_TypeKind_Bitfield)
  {
    switch(k.kind)
    {
      case E2_TypeKeyKind_Null:{}break;
      case E2_TypeKeyKind_Basic:{}break;
      case E2_TypeKeyKind_DbgInfo:
      {
        E2_DbgInfo *dbgi = e2_dbgi_from_type_key(k);
        U32 dbgi_type_idx = e2_dbgi_type_idx_from_key(k);
        RDI_TypeNode *type_node = rdi_element_from_name_idx(dbgi->rdi, TypeNodes, dbgi_type_idx);
        result = type_node->bitfield.off;
      }break;
      case E2_TypeKeyKind_Cons:{}break;
      case E2_TypeKeyKind_Reg:{}break;
    }
  }
  return result;
}

internal U64
e2_mask_count_from_type_key(E2_TypeKey k)
{
  U64 result = 0;
  if(e2_type_kind_from_key(k) == E2_TypeKind_Bitfield)
  {
    switch(k.kind)
    {
      case E2_TypeKeyKind_Null:{}break;
      case E2_TypeKeyKind_Basic:{}break;
      case E2_TypeKeyKind_DbgInfo:
      {
        E2_DbgInfo *dbgi = e2_dbgi_from_type_key(k);
        U32 dbgi_type_idx = e2_dbgi_type_idx_from_key(k);
        RDI_TypeNode *type_node = rdi_element_from_name_idx(dbgi->rdi, TypeNodes, dbgi_type_idx);
        result = type_node->bitfield.size;
      }break;
      case E2_TypeKeyKind_Cons:{}break;
      case E2_TypeKeyKind_Reg:{}break;
    }
  }
  return result;
}

internal U64
e2_array_count_from_type_key(E2_TypeKey k)
{
  U64 result = 0;
  if(e2_type_kind_from_key(k) == E2_TypeKind_Array)
  {
    switch(k.kind)
    {
      case E2_TypeKeyKind_Null:{}break;
      case E2_TypeKeyKind_Basic:{}break;
      case E2_TypeKeyKind_DbgInfo:
      {
        E2_DbgInfo *dbgi = e2_dbgi_from_type_key(k);
        U32 dbgi_type_idx = e2_dbgi_type_idx_from_key(k);
        RDI_TypeNode *type_node = rdi_element_from_name_idx(dbgi->rdi, TypeNodes, dbgi_type_idx);
        result = type_node->constructed.count;
      }break;
      case E2_TypeKeyKind_Cons:
      {
        U64 id = e2_cons_type_id_from_key(k);
        E2_ConsTypeNode *node = e2_cons_type_node_from_id(id);
        result = node->params.count;
      }break;
      case E2_TypeKeyKind_Reg:{}break;
    }
  }
  return result;
}

internal Arch
e2_arch_from_type_key(E2_TypeKey k)
{
  Arch arch = Arch_Null;
  switch(k.kind)
  {
    case E2_TypeKeyKind_Null:{}break;
    case E2_TypeKeyKind_Basic:{}break;
    case E2_TypeKeyKind_DbgInfo:
    {
      E2_DbgInfo *dbgi = e2_dbgi_from_type_key(k);
      RDI_TopLevelInfo *tli = rdi_element_from_name_idx(dbgi->rdi, TopLevelInfo, 0);
      RDI_Arch rdi_arch = tli->arch;
      arch = arch_from_rdi_arch(rdi_arch);
    }break;
    case E2_TypeKeyKind_Cons:
    {
      U64 id = e2_cons_type_id_from_key(k);
      E2_ConsTypeNode *node = e2_cons_type_node_from_id(id);
      arch = node->params.arch;
    }break;
    case E2_TypeKeyKind_Reg:
    {
      arch = (Arch)k.u32[0];
    }break;
  }
  return arch;
}

internal String8
e2_name_from_type_key(Arena *arena, E2_TypeKey k)
{
  String8 result = {0};
  switch(k.kind)
  {
    case E2_TypeKeyKind_Null:{}break;
    case E2_TypeKeyKind_Basic:
    {
      E2_TypeKind kind = e2_type_kind_from_key(k);
      if(E2_TypeKind_FirstBasic <= kind && kind <= E2_TypeKind_LastBasic)
      {
        result = e2_type_kind_basic_string_table[kind];
      }
    }break;
    case E2_TypeKeyKind_DbgInfo:
    {
      E2_DbgInfo *dbgi = e2_dbgi_from_type_key(k);
      U32 type_idx = e2_dbgi_type_idx_from_key(k);
      RDI_TypeNode *type_node = rdi_element_from_name_idx(dbgi->rdi, TypeNodes, type_idx);
      result = fully_qualified_str8_from_rdi_type(arena, dbgi->rdi, type_node);
    }break;
    case E2_TypeKeyKind_Cons:
    {
      U64 id = e2_cons_type_id_from_key(k);
      E2_ConsTypeNode *node = e2_cons_type_node_from_id(id);
      result = node->params.name;
    }break;
    case E2_TypeKeyKind_Reg:
    {
      U64 byte_size = e2_byte_size_from_type_key(k);
      result = str8f(arena, "register_%I64u", byte_size*8);
    }break;
  }
  return result;
}

//- rjf: type key -> string

internal String8
e2_string_from_type_key(Arena *arena, E2_TypeKey key)
{
  String8 result = {0};
  {
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      E2_TypeKey key;
      U32 prec;
      B32 did_direct;
    };
    Task start_task = {0, key};
    Task *top_task = &start_task;
    Task *free_task = 0;
    Temp scratch = scratch_begin(&arena, 1);
    String8List lhs = {0};
    String8List rhs = {0};
    for(;top_task != 0;)
    {
      //- rjf: unpack type
      E2_TypeKey key = top_task->key;
      E2_TypeKind kind = e2_type_kind_from_key(key);
      E2_TypeKey direct = e2_type_key_direct(key);
      String8 name = e2_name_from_type_key(scratch.arena, key);
      U64 prec = top_task->prec;
      B32 did_direct = top_task->did_direct;
      
      //- rjf: push string for this task
      U32 next_prec = 0;
      switch(kind)
      {
        default:
        {
          String8 keyword = {0};
          if(E2_TypeKind_FirstIncomplete <= kind && kind <= E2_TypeKind_LastIncomplete)
          {
            switch(kind)
            {
              default:{}break;
              case E2_TypeKind_IncompleteStruct:{keyword = s("struct");}break;
              case E2_TypeKind_IncompleteUnion: {keyword = s("union");}break;
              case E2_TypeKind_IncompleteClass: {keyword = s("class");}break;
              case E2_TypeKind_IncompleteEnum:  {keyword = s("enum");}break;
            }
          }
          if(keyword.size != 0)
          {
            str8_list_pushf(scratch.arena, &lhs, "%S ", keyword);
          }
          str8_list_push(scratch.arena, &lhs, name);
        }break;
        case E2_TypeKind_Ptr:
        if(!did_direct)
        {
          next_prec = 1;
        }
        else if(did_direct)
        {
          str8_list_push(scratch.arena, &lhs, s("*"));
        }break;
        case E2_TypeKind_LRef:
        if(did_direct)
        {
          str8_list_push(scratch.arena, &lhs, s("&"));
        }break;
        case E2_TypeKind_RRef:
        if(did_direct)
        {
          str8_list_push(scratch.arena, &lhs, s("&&"));
        }break;
        case E2_TypeKind_Array:
        if(!did_direct)
        {
          next_prec = 2;
        }
        else
        {
          U64 count = e2_array_count_from_type_key(key);
          if(prec == 1)
          {
            str8_list_push(scratch.arena, &lhs, s("("));
            str8_list_push(scratch.arena, &rhs, s(")"));
          }
          str8_list_pushf(scratch.arena, &rhs, "[%I64u]", count);
        }break;
        case E2_TypeKind_Function:
        if(!did_direct)
        {
          next_prec = 2;
        }
        else
        {
          U64 count = e2_array_count_from_type_key(key);
          if(prec == 1)
          {
            str8_list_push(scratch.arena, &lhs, s("("));
            str8_list_push(scratch.arena, &rhs, s(")"));
          }
          // TODO(rjf): params here
          str8_list_pushf(scratch.arena, &rhs, "(...)");
        }break;
        case E2_TypeKind_Bitfield:
        if(did_direct)
        {
          U64 mask_count = e2_mask_count_from_type_key(key);
          str8_list_pushf(scratch.arena, &rhs, ": %I64u", mask_count);
        }break;
        case E2_TypeKind_Variadic:
        {
          str8_list_push(scratch.arena, &lhs, s("..."));
        }break;
      }
      
      //- rjf: did direct, or we don't have a direct key? -> we're done with this type, pop
      if(did_direct || e2_type_key_match(direct, e2_type_key_zero()))
      {
        Task *popped = top_task;
        SLLStackPop(top_task);
        SLLStackPush(free_task, popped);
      }
      
      //- rjf: didn't do direct? -> push new task for direct type
      else
      {
        top_task->did_direct = 1;
        Task *t = free_task;
        if(t != 0)
        {
          SLLStackPop(free_task);
        }
        else
        {
          t = push_array_no_zero(scratch.arena, Task, 1);
        }
        MemoryZeroStruct(t);
        SLLStackPush(top_task, t);
        t->key = direct;
        t->prec = next_prec;
      }
    }
    String8List parts = {0};
    str8_list_concat_in_place(&parts, &lhs);
    str8_list_concat_in_place(&parts, &rhs);
    result = str8_list_join(arena, &parts, 0);
    scratch_end(scratch);
  }
  return result;
}

//- rjf: type deep matches

internal B32
e2_type_deep_match(E2_TypeKey l, E2_TypeKey r)
{
  B32 result = e2_type_key_match(l, r);
  if(!result)
  {
    Temp scratch = scratch_begin(0, 0);
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      E2_TypeKey k;
      U64 member_num;
    };
    Task *free_task = 0;
    Task l_start_task = {0, l};
    Task r_start_task = {0, r};
    Task *l_top = &l_start_task;
    Task *r_top = &r_start_task;
    U64 l_hash = 0;
    U64 r_hash = 0;
    result = 1;
    for(;l_top != 0 || r_top != 0;)
    {
      // rjf: mismatched task structures -> immediately mark as a non-match & break
      if((l_top && !r_top) || (!l_top && r_top))
      {
        result = 0;
        break;
      }
      
      // rjf: mismatched hashes -> immediately mark as a non-match & break
      if(l_hash != r_hash)
      {
        result = 0;
        break;
      }
      
      // rjf: do next step of hash
      Task **task_tops[] = {&l_top, &r_top};
      E2_TypeKey keys[] = {l_top->k, r_top->k};
      U64 *hash_ptrs[] = {&l_hash, &r_hash};
      for EachElement(key_idx, keys)
      {
        Task *top = task_tops[key_idx][0];
        E2_TypeKey key = keys[key_idx];
        E2_TypeKind kind = e2_type_kind_from_key(key);
        
        // rjf: have not advanced to members -> mix in kind / byte size
        if(top->member_num == 0)
        {
          U64 byte_size = e2_byte_size_from_type_key(key);
          hash_ptrs[key_idx][0] = u64_hash_from_seed_str8(hash_ptrs[key_idx][0], str8_struct(&kind));
          hash_ptrs[key_idx][0] = u64_hash_from_seed_str8(hash_ptrs[key_idx][0], str8_struct(&byte_size));
        }
        
        // rjf: advance to next member
        B32 has_next_member = 0;
        if(kind == E2_TypeKind_Struct ||
           kind == E2_TypeKind_Union ||
           kind == E2_TypeKind_Class ||
           kind == E2_TypeKind_Enum)
        {
          U32 dbgi_num = e2_dbgi_num_from_type_key(key);
          U32 type_idx = e2_dbgi_type_idx_from_key(key);
          E2_DbgInfo *dbgi = e2_dbgi_from_num(dbgi_num);
          RDI_TypeNode *type_node = rdi_element_from_name_idx(dbgi->rdi, TypeNodes, type_idx);
          String8 name = str8_from_rdi_string_idx(dbgi->rdi, type_node->user_defined.name_string_idx);
          hash_ptrs[key_idx][0] = u64_hash_from_seed_str8(hash_ptrs[key_idx][0], name);
          RDI_UDT *udt = rdi_element_from_name_idx(dbgi->rdi, UDTs, type_node->user_defined.udt_idx);
          U64 next_member_num = top->member_num + 1;
          if(next_member_num <= udt->member_count)
          {
            has_next_member = 1;
            RDI_Member *member = rdi_element_from_name_idx(dbgi->rdi, Members, next_member_num-1);
            RDI_TypeNode *member_type_node = rdi_element_from_name_idx(dbgi->rdi, TypeNodes, member->type_idx);
            String8 member_name = str8_from_rdi_string_idx(dbgi->rdi, member->name_string_idx);
            hash_ptrs[key_idx][0] = u64_hash_from_seed_str8(hash_ptrs[key_idx][0], str8_struct(&member->off));
            hash_ptrs[key_idx][0] = u64_hash_from_seed_str8(hash_ptrs[key_idx][0], member_name);
            Task *member_task = free_task;
            if(member_task != 0)
            {
              SLLStackPop(free_task);
            }
            else
            {
              member_task = push_array_no_zero(scratch.arena, Task, 1);
            }
            MemoryZeroStruct(&member_task);
            member_task->k = e2_type_key_dbgi(e2_type_kind_from_rdi(member_type_node->kind), dbgi_num, member->type_idx);
            SLLStackPush(task_tops[key_idx][0], member_task);
          }
        }
        
        // rjf: if we don't have a subsequent member, pop this task
        if(!has_next_member)
        {
          Task *popped = task_tops[key_idx][0];
          SLLStackPop(task_tops[key_idx][0]);
          SLLStackPush(free_task, popped);
        }
      }
    }
    scratch_end(scratch);
  }
  return result;
}

//- rjf: type graph traversal primitives

internal E2_TypeKey
e2_type_key_direct(E2_TypeKey k)
{
  E2_TypeKey result = {E2_TypeKeyKind_Null};
  switch(k.kind)
  {
    case E2_TypeKeyKind_Null:{}break;
    case E2_TypeKeyKind_Basic:{}break;
    case E2_TypeKeyKind_DbgInfo:
    {
      U32 dbgi_num = e2_dbgi_num_from_type_key(k);
      U32 dbgi_type_idx = e2_dbgi_type_idx_from_key(k);
      E2_DbgInfo *dbgi = e2_dbgi_from_num(dbgi_num);
      RDI_TypeNode *type_node = rdi_element_from_name_idx(dbgi->rdi, TypeNodes, dbgi_type_idx);
      U32 direct_type_idx = 0;
      if(RDI_TypeKind_FirstConstructed <= type_node->kind && type_node->kind <= RDI_TypeKind_LastConstructed)
      {
        direct_type_idx = type_node->constructed.direct_type_idx;
      }
      else if(RDI_TypeKind_FirstUserDefined <= type_node->kind && type_node->kind <= RDI_TypeKind_LastUserDefined)
      {
        direct_type_idx = type_node->user_defined.direct_type_idx;
      }
      else if(type_node->kind == RDI_TypeKind_Bitfield)
      {
        direct_type_idx = type_node->bitfield.direct_type_idx;
      }
      result = e2_type_key_dbgi(e2_type_kind_from_rdi(type_node->kind), dbgi_num, direct_type_idx);
    }break;
    case E2_TypeKeyKind_Cons:
    {
      U64 id = e2_cons_type_id_from_key(k);
      E2_ConsTypeNode *node = e2_cons_type_node_from_id(id);
      result = node->params.direct;
    }break;
    case E2_TypeKeyKind_Reg:{}break;
  }
  return result;
}

internal E2_TypeKey
e2_type_key_owner(E2_TypeKey k)
{
  E2_TypeKey result = {E2_TypeKeyKind_Null};
  if(e2_type_kind_from_key(k) == E2_TypeKind_MemberPtr)
  {
    switch(k.kind)
    {
      case E2_TypeKeyKind_Null:{}break;
      case E2_TypeKeyKind_Basic:{}break;
      case E2_TypeKeyKind_DbgInfo:
      {
        U32 dbgi_num = e2_dbgi_num_from_type_key(k);
        U32 dbgi_type_idx = e2_dbgi_type_idx_from_key(k);
        E2_DbgInfo *dbgi = e2_dbgi_from_num(dbgi_num);
        RDI_TypeNode *type_node = rdi_element_from_name_idx(dbgi->rdi, TypeNodes, dbgi_type_idx);
        U32 owner_type_idx = type_node->constructed.owner_type_idx;
        result = e2_type_key_dbgi(e2_type_kind_from_rdi(type_node->kind), dbgi_num, owner_type_idx);
      }break;
      case E2_TypeKeyKind_Cons:{}break;
      case E2_TypeKeyKind_Reg:{}break;
    }
  }
  return result;
}

//- rjf: type unwrapping (helpers for advancing past categories of direct type chains)

internal E2_TypeKey
e2_type_key_unwrap(E2_TypeKey k, E2_TypeUnwrapFlags flags)
{
  E2_TypeKey result = k;
  E2_TypeKind kind = e2_type_kind_from_key(result);
  B32 did_ptr = 0;
  for(;;)
  {
    B32 done = 0;
    switch(kind)
    {
      default:{done = 1;}break;
      case E2_TypeKind_Modifier:  {done = !(flags & E2_TypeUnwrapFlag_Modifiers);}break;
      case E2_TypeKind_Lens:      {done = !(flags & E2_TypeUnwrapFlag_Lenses);}break;
      case E2_TypeKind_MetaDisplayName:
      case E2_TypeKind_MetaDescription:
      case E2_TypeKind_MetaExpr:  {done = !(flags & E2_TypeUnwrapFlag_Meta);}break;
      case E2_TypeKind_Enum:      {done = !(flags & E2_TypeUnwrapFlag_Enums);}break;
      case E2_TypeKind_Alias:     {done = !(flags & E2_TypeUnwrapFlag_Aliases);}break;
      case E2_TypeKind_Bitfield:  {done = !(flags & E2_TypeUnwrapFlag_Bitfields);}break;
      case E2_TypeKind_Array:
      case E2_TypeKind_Ptr:
      case E2_TypeKind_RRef:
      case E2_TypeKind_LRef:
      case E2_TypeKind_MemberPtr:
      {
        done = (did_ptr || !(flags & E2_TypeUnwrapFlag_Pointers));
        did_ptr = 1;
      }break;
    }
    if(done)
    {
      break;
    }
    result = e2_type_key_direct(result);
    kind = e2_type_kind_from_key(result);
  }
  return result;
}

//- rjf: type coercion

internal E2_TypeKey
e2_coerced_type_key_from_operands(E2_TypeKey lhs, E2_TypeKey rhs)
{
  E2_TypeKey result = lhs;
  {
    E2_TypeKind lhs_kind = e2_type_kind_from_key(lhs);
    E2_TypeKind rhs_kind = e2_type_kind_from_key(rhs);
    U64 lhs_size = e2_byte_size_from_type_key(lhs);
    U64 rhs_size = e2_byte_size_from_type_key(rhs);
    
    //- rjf: f? + f? -> pick biggest
    if(e2_type_kind_is_float(lhs_kind) && e2_type_kind_is_float(rhs_kind) && lhs_size > rhs_size)
    {
      result = lhs;
    }
    else if(e2_type_kind_is_float(lhs_kind) && e2_type_kind_is_float(rhs_kind) && lhs_size < rhs_size)
    {
      result = rhs;
    }
    
    //- rjf: f? + i? -> pick float
    else if(e2_type_kind_is_float(lhs_kind) && e2_type_kind_is_integer(rhs_kind))
    {
      result = lhs;
    }
    else if(e2_type_kind_is_integer(lhs_kind) && e2_type_kind_is_float(rhs_kind))
    {
      result = rhs;
    }
    
    //- rjf: i? + i? -> pick biggest
    else if(e2_type_kind_is_integer(lhs_kind) && e2_type_kind_is_integer(rhs_kind) && lhs_size > rhs_size)
    {
      result = lhs;
    }
    else if(e2_type_kind_is_integer(lhs_kind) && e2_type_kind_is_integer(rhs_kind) && lhs_size < rhs_size)
    {
      result = rhs;
    }
    
    //- rjf: i? + u? -> unsigned wins
    else if(e2_type_kind_is_signed(lhs_kind) && e2_type_kind_is_unsigned(rhs_kind) && lhs_size == rhs_size)
    {
      result = rhs;
    }
    else if(e2_type_kind_is_unsigned(lhs_kind) && e2_type_kind_is_signed(rhs_kind) && lhs_size == rhs_size)
    {
      result = lhs;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Expression Constructors

internal E2_Expr *
e2_expr(Arena *arena, E2_ExprKind kind)
{
  E2_Expr *expr = push_array(arena, E2_Expr, 1);
  MemoryCopyStruct(expr, &e2_expr_nil);
  expr->kind = kind;
  return expr;
}

internal void
e2_expr_push_child_node(E2_Expr *parent, E2_ExprNode *node)
{
  SLLQueuePush(parent->first_child, parent->last_child, node);
  parent->child_count += 1;
}

internal void
e2_expr_push_child(Arena *arena, E2_Expr *parent, E2_Expr *expr)
{
  E2_ExprNode *n = push_array(arena, E2_ExprNode, 1);
  n->v = expr;
  e2_expr_push_child_node(parent, n);
}

internal void
e2_expr_push_child_node_front(E2_Expr *parent, E2_ExprNode *node)
{
  SLLQueuePushFront(parent->first_child, parent->last_child, node);
  parent->child_count += 1;
}

internal void
e2_expr_push_child_front(Arena *arena, E2_Expr *parent, E2_Expr *expr)
{
  E2_ExprNode *n = push_array(arena, E2_ExprNode, 1);
  n->v = expr;
  e2_expr_push_child_node_front(parent, n);
}

////////////////////////////////
//~ rjf: Expression Constructors

internal E2_IRNode *
e2_irnode(Arena *arena)
{
  E2_IRNode *expr = push_array(arena, E2_IRNode, 1);
  MemoryCopyStruct(expr, &e2_irnode_nil);
  return expr;
}

internal E2_IRNode *
e2_irnode_const_u64_or_smaller(Arena *arena, U64 u)
{
  E2_IRNode *e = e2_irnode(arena);
  {
    e->mode = E2_Mode_Value;
    e->val.u64 = u;
    if(u <= 0x7fffffff)
    {
      e->op = RDI_EvalOp_ConstU32;
      e->type_key = e2_type_key_basic(E2_TypeKind_S32);
    }
    else if(u <= 0xffffffff)
    {
      e->op = RDI_EvalOp_ConstU32;
      e->type_key = e2_type_key_basic(E2_TypeKind_U32);
    }
    else if(u <= 0x7fffffffffffffffull)
    {
      e->op = RDI_EvalOp_ConstU64;
      e->type_key = e2_type_key_basic(E2_TypeKind_S64);
    }
    else if(u <= 0xffffffffffffffffull)
    {
      e->op = RDI_EvalOp_ConstU64;
      e->type_key = e2_type_key_basic(E2_TypeKind_U64);
    }
  }
  return e;
}

internal E2_IRNode *
e2_irnode_const_f32(Arena *arena, F32 f32)
{
  E2_IRNode *e = e2_irnode(arena);
  e->mode = E2_Mode_Value;
  e->val.f32 = f32;
  e->op = RDI_EvalOp_ConstU32;
  e->type_key = e2_type_key_basic(E2_TypeKind_F32);
  return e;
}

internal E2_IRNode *
e2_irnode_const_f64(Arena *arena, F64 f64)
{
  E2_IRNode *e = e2_irnode(arena);
  e->mode = E2_Mode_Value;
  e->val.f64 = f64;
  e->op = RDI_EvalOp_ConstU64;
  e->type_key = e2_type_key_basic(E2_TypeKind_F64);
  return e;
}

internal E2_IRNode *
e2_irnode_unary_op(Arena *arena, E2_TypeKey type_key, RDI_EvalOp op, E2_IRNode *operand)
{
  E2_IRNode *e = e2_irnode(arena);
  e->type_key = type_key;
  e->op = op;
  e->mode = E2_Mode_Value;
  e->val.u512.u8[0] = e2_type_group_from_kind(e2_type_kind_from_key(type_key));
  e->val.u512.u8[1] = 8*e2_byte_size_from_type_key(type_key);
  e2_irnode_push_child(arena, e, operand);
  return e;
}

internal E2_IRNode *
e2_irnode_binary_op(Arena *arena, E2_TypeKey type_key, RDI_EvalOp op, E2_IRNode *lhs, E2_IRNode *rhs)
{
  E2_IRNode *e = e2_irnode(arena);
  E2_TypeKey arith_type_key = type_key;
  if(RDI_EvalOp_FirstLogical <= op && op <= RDI_EvalOp_LastLogical)
  {
    arith_type_key = lhs->type_key;
  }
  e->type_key = type_key;
  e->op = op;
  e->mode = E2_Mode_Value;
  e->val.u512.u8[0] = e2_type_group_from_kind(e2_type_kind_from_key(arith_type_key));
  e->val.u512.u8[1] = 8*e2_byte_size_from_type_key(arith_type_key);
  e2_irnode_push_child(arena, e, lhs);
  e2_irnode_push_child(arena, e, rhs);
  return e;
}

internal E2_IRNode *
e2_irnode_resolve_to_value(Arena *arena, E2_IRNode *expr)
{
  E2_IRNode *result = expr;
  
  // rjf: address evaluations of arrays -> create value of pointer to first element
  if(expr->mode == E2_Mode_Address && e2_type_kind_from_key(e2_type_key_undecorate(expr->type_key)) == E2_TypeKind_Array)
  {
    result = e2_irnode(arena);
    result->type_key = e2_type_key_direct(e2_type_key_undecorate(expr->type_key));
    result->mode = E2_Mode_Value;
    e2_irnode_push_child(arena, result, expr);
  }
  
  // rjf: address evaluations -> read value from space
  else if(expr->mode == E2_Mode_Address)
  {
    U64 memread_byte_size = e2_byte_size_from_type_key(expr->type_key);
    memread_byte_size = Min(64, memread_byte_size);
    E2_IRNode *memread_expr = e2_irnode(arena);
    memread_expr->op = RDI_EvalOp_MemRead;
    memread_expr->mode = E2_Mode_Value;
    memread_expr->type_key = expr->type_key;
    memread_expr->val.u64 = memread_byte_size;
    e2_irnode_push_child(arena, memread_expr, expr);
    result = memread_expr;
  }
  
  // rjf: bitfields -> shift & mask
  E2_TypeKey core_type_key = e2_type_key_undecorate(expr->type_key);
  if(e2_type_kind_from_key(core_type_key) == E2_TypeKind_Bitfield)
  {
    // rjf: unpack bitfield params
    U64 shift = e2_shift_from_type_key(result->type_key);
    U64 mask_count = e2_mask_count_from_type_key(result->type_key);
    U64 valid_bits_mask = 0;
    for EachIndex(idx, mask_count)
    {
      valid_bits_mask |= (1ull<<idx);
    }
    
    // rjf: mask(shift(expr, count), bitmask)
    E2_IRNode *shift_expr = e2_irnode_binary_op(arena, e2_type_key_basic(E2_TypeKind_U64), RDI_EvalOp_RShift, result, e2_irnode_const_u64_or_smaller(arena, shift));
    E2_IRNode *mask_expr = e2_irnode_binary_op(arena, e2_type_key_direct(core_type_key), RDI_EvalOp_BitAnd, shift_expr, e2_irnode_const_u64_or_smaller(arena, valid_bits_mask));
    result = mask_expr;
  }
  
  return result;
}

internal E2_IRNode *
e2_irnode_truncate(Arena *arena, E2_IRNode *expr, E2_TypeKey dst_type_key)
{
  E2_IRNode *result = expr;
  E2_TypeKind dst_type_kind = e2_type_kind_from_key(dst_type_key);
  U64 dst_type_byte_size = e2_byte_size_from_type_key(dst_type_key);
  B32 dst_type_is_signed = e2_type_kind_is_signed(dst_type_kind);
  if(dst_type_byte_size < 64)
  {
    result = e2_irnode(arena);
    result->type_key = dst_type_key;
    result->op = dst_type_is_signed ? RDI_EvalOp_TruncSigned : RDI_EvalOp_Trunc;
    result->val.u64 = dst_type_byte_size*8;
    e2_irnode_push_child(arena, result, expr);
  }
  return result;
}

internal E2_IRNode *
e2_irnode_convert_if_possible(Arena *arena, E2_IRNode *expr, E2_TypeKey dst_type_key)
{
  E2_IRNode *result = expr;
  {
    // rjf: unpack src / dst types
    E2_TypeKey src_type_key = expr->type_key;
    E2_TypeKind src_type_kind = e2_type_kind_from_key(src_type_key);
    E2_TypeKind dst_type_kind = e2_type_kind_from_key(dst_type_key);
    RDI_EvalTypeGroup src_type_group = e2_type_group_from_kind(src_type_kind);
    RDI_EvalTypeGroup dst_type_group = e2_type_group_from_kind(dst_type_kind);
    U64 src_byte_size = e2_byte_size_from_type_key(src_type_key);
    U64 dst_byte_size = e2_byte_size_from_type_key(dst_type_key);
    
    // rjf: convert from src -> dst
    RDI_EvalConversionKind conversion_kind = rdi_eval_conversion_kind_from_typegroups(src_type_group, dst_type_group);
    if(conversion_kind == RDI_EvalConversionKind_Legal)
    {
      result = e2_irnode(arena);
      result->mode     = E2_Mode_Value;
      result->type_key = dst_type_key;
      result->op       = RDI_EvalOp_Convert;
      result->val.u64  = src_type_group | (dst_type_group << 8);
      e2_irnode_push_child(arena, result, expr);
    }
    
    // rjf: no-op from src -> dst
    if(conversion_kind == RDI_EvalConversionKind_Noop)
    {
      result->type_key = dst_type_key;
    }
    
    // rjf: if shrinking integer sizes, truncate
    if(dst_byte_size < src_byte_size && e2_type_kind_is_integer(dst_type_kind))
    {
      result = e2_irnode_truncate(arena, expr, dst_type_key);
    }
  }
  return result;
}

internal E2_IRNode *
e2_irnode_type(Arena *arena, E2_TypeKey type_key)
{
  E2_IRNode *expr = e2_irnode(arena);
  expr->type_key = type_key;
  expr->mode = E2_Mode_Type;
  return expr;
}

internal void
e2_irnode_push_child_node(E2_IRNode *parent, E2_IRNodePtrNode *node)
{
  SLLQueuePush(parent->first_child, parent->last_child, node);
  parent->child_count += 1;
}

internal void
e2_irnode_push_child(Arena *arena, E2_IRNode *parent, E2_IRNode *expr)
{
  E2_IRNodePtrNode *n = push_array(arena, E2_IRNodePtrNode, 1);
  n->v = expr;
  e2_irnode_push_child_node(parent, n);
}

////////////////////////////////
//~ rjf: String -> Expression

internal E2_Token
e2_token_from_string_off(E2_LangKind lang, String8 string, U64 start_off)
{
  E2_LangInfo *lang_info = &e2_lang_kind_info_table[lang];
  E2_Token token = {E2_TokenKind_Null};
  B32 identifier_tick_mode = 0;
  B32 comment_is_explicit_ender = 0;
  B32 numeric_exponent = 0;
  B32 escaped = 0;
  B32 done = 0;
  for(U64 off = start_off; !done && off <= string.size; off += 1)
  {
    U8 next_byte = (off < string.size ? string.str[off] : 0);
    U8 next2_byte = (off+1 < string.size ? string.str[off+1] : 0);
    U8 next3_byte = (off+2 < string.size ? string.str[off+2] : 0);
    switch(token.kind)
    {
      //- rjf: no set token kind => look for starters
      default:
      {
        if(next_byte == 0)
        {
          done = 1;
        }
        else if(next_byte <= 32)
        {
          token.kind = E2_TokenKind_Whitespace;
          token.range.min = token.range.max = off;
        }
        else if(next_byte == '$' || next_byte == '_' || next_byte == '`' ||
                ('A' <= next_byte && next_byte <= 'Z') ||
                ('a' <= next_byte && next_byte <= 'z'))
        {
          token.kind = E2_TokenKind_Identifier;
          token.range.min = token.range.max = off;
          identifier_tick_mode = (next_byte == '`');
        }
        else if(('0' <= next_byte && next_byte <= '9') ||
                (next_byte == '.' && '0' <= next2_byte && next2_byte <= '9'))
        {
          token.kind = E2_TokenKind_Numeric;
          token.range.min = token.range.max = off;
        }
        else if(next_byte == '\'')
        {
          token.kind = E2_TokenKind_CharLiteral;
          token.range.min = token.range.max = off;
        }
        else if(next_byte == '"')
        {
          token.kind = E2_TokenKind_StringLiteral;
          token.range.min = token.range.max = off;
        }
        else if(next_byte == '/' && next2_byte == '/')
        {
          token.kind = E2_TokenKind_Comment;
          token.range.min = token.range.max = off;
        }
        else if(next_byte == '/' && next2_byte == '*')
        {
          token.kind = E2_TokenKind_Comment;
          token.range.min = token.range.max = off;
          comment_is_explicit_ender = 1;
        }
        else if(next_byte == '~' || next_byte == '!' || next_byte == '%' ||
                next_byte == '^' || next_byte == '&' || next_byte == '*' ||
                next_byte == '(' || next_byte == ')' || next_byte == '[' ||
                next_byte == ']' || next_byte == '{' || next_byte == '}' ||
                next_byte == '-' || next_byte == '=' || next_byte == '+' ||
                next_byte == ':' || next_byte == ';' || next_byte == ',' ||
                next_byte == '.' || next_byte == '<' || next_byte == '>' ||
                next_byte == '/' || next_byte == '?' || next_byte == '#' ||
                next_byte == '@' || next_byte == '|')
        {
          token.kind = E2_TokenKind_Symbol;
          token.range.min = token.range.max = off;
        }
      }break;
      
      //- rjf: active tokens -> seek enders
      case E2_TokenKind_Whitespace:
      if(next_byte > 32)
      {
        done = 1;
        token.range.max = off;
      }break;
      case E2_TokenKind_Comment:
      if(comment_is_explicit_ender && next_byte == '*' && next2_byte == '/')
      {
        done = 1;
        token.range.max = off+2;
      }break;
      case E2_TokenKind_Identifier:
      {
        if(identifier_tick_mode && (next_byte == '`' || next_byte == '\''))
        {
          identifier_tick_mode = 0;
        }
        else if(!identifier_tick_mode &&
                (next_byte < 'a' || 'z' < next_byte) &&
                (next_byte < 'A' || 'Z' < next_byte) &&
                (next_byte < '0' || '9' < next_byte) &&
                next_byte != '$' &&
                next_byte != '_' &&
                next_byte != '@')
        {
          done = 1;
          token.range.max = off;
        }
      }break;
      case E2_TokenKind_Numeric:
      {
        if(numeric_exponent && (next_byte == '+' || next_byte == '-')){}
        else if((next_byte < 'a' || 'z' < next_byte) &&
                (next_byte < 'A' || 'Z' < next_byte) &&
                (next_byte < '0' || '9' < next_byte) &&
                next_byte != '.' &&
                next_byte != ':')
        {
          done = 1;
          token.range.max = off;
        }
        else
        {
          numeric_exponent = (next_byte == 'e');
        }
      }break;
      case E2_TokenKind_Symbol:
      if(next_byte != '~' && next_byte != '!' && next_byte != '%' &&
         next_byte != '^' && next_byte != '&' && next_byte != '*' &&
         next_byte != '(' && next_byte != ')' && next_byte != '[' &&
         next_byte != ']' && next_byte != '{' && next_byte != '}' &&
         next_byte != '-' && next_byte != '=' && next_byte != '+' &&
         next_byte != ':' && next_byte != ';' && next_byte != ',' &&
         next_byte != '.' && next_byte != '<' && next_byte != '>' &&
         next_byte != '/' && next_byte != '?' && next_byte != '#' &&
         next_byte != '@' && next_byte != '|')
      {
        done = 1;
        token.range.max = off;
        String8 token_string = str8_substr(string, token.range);
        U64 biggest_match_size = 0;
        for EachIndex(idx, lang_info->expr_kind_parse_infos_count)
        {
          String8 op_symbol = (lang_info->expr_kind_parse_infos[idx].pre.size != 0) ? lang_info->expr_kind_parse_infos[idx].pre : lang_info->expr_kind_parse_infos[idx].sep;
          if(biggest_match_size < op_symbol.size && str8_match(op_symbol, str8_prefix(token_string, op_symbol.size), 0))
          {
            biggest_match_size = op_symbol.size;
            token.range.max = token.range.min + op_symbol.size;
          }
        }
        if(biggest_match_size == 0)
        {
          token.range.max = token.range.min + 1;
        }
      }break;
      case E2_TokenKind_CharLiteral:
      case E2_TokenKind_StringLiteral:
      {
        if(!escaped && next_byte == '\\')
        {
          escaped = 1;
        }
        else if(escaped)
        {
          escaped = 0;
        }
        else if(!escaped && ((next_byte == '"' && token.kind == E2_TokenKind_StringLiteral) ||
                             (next_byte == '\'' && token.kind == E2_TokenKind_CharLiteral)))
        {
          done = 1;
          token.range.max = off+1;
        }
      }break;
    }
    if(token.range.max > off+1)
    {
      off += (token.range.max - (off+1));
    }
    if(!done && off == string.size)
    {
      done = 1;
      token.range.max = off;
    }
  }
  return token;
}

internal U64
e2_read_token(E2_LangKind lang, String8 string, U64 off, E2_Token *token_out)
{
  E2_Token token = e2_token_from_string_off(lang, string, off);
  if(token_out != 0)
  {
    token_out[0] = token;
  }
  U64 result = dim_1u64(token.range);
  return result;
}

internal B32
e2_try_token(E2_LangKind lang, String8 string, E2_TokenKind kind, String8 expected_string, U64 *off_out, E2_Token *token_out)
{
  B32 result = 0;
  U64 off = off_out[0];
  for(;;)
  {
    E2_Token next_token = {E2_TokenKind_Null};
    off += e2_read_token(lang, string, off, &next_token);
    if(next_token.kind == kind && (expected_string.size == 0 || str8_match(str8_substr(string, next_token.range), expected_string, 0)))
    {
      result = 1;
      if(token_out != 0)
      {
        token_out[0] = next_token;
      }
      break;
    }
    else if(next_token.kind != E2_TokenKind_Comment && next_token.kind != E2_TokenKind_Whitespace)
    {
      break;
    }
  }
  if(result && off_out != 0)
  {
    off_out[0] = off;
  }
  return result;
}

internal E2_Parse
e2_parse_from_string(Arena *arena, E2_ParseState *state, B32 identifier_is_type, E2_LangKind lang, String8 string)
{
  U64 off = state->string_off;
  E2_Parse parse = {E2_ParseStatus_Error, .expr = &e2_expr_nil, .params_expr = &e2_expr_nil};
  E2_LangInfo *lang_info = &e2_lang_kind_info_table[lang];
  
  //- rjf: set up initial parsing task
  if(state->top_task == 0)
  {
    state->top_task = &state->start_task;
    state->top_task->src_range = r1u64(0, string.size);
    state->top_task->child_count_target = max_U64;
    state->top_task->max_precedence = max_S64;
    state->top_task->expected_splitter = s(",");
    state->top_task->splitter_min_child_count = 0;
  }
  
  //- rjf: parse & attach to top parsing task - if we don't have a top task
  // then it is just the result
  E2_Expr *expr = &e2_expr_nil;
  for(B32 done = 0; !done;)
  {
    U64 start_off = off;
    E2_ParseTask *start_task = state->top_task;
    S64 max_precedence = state->top_task->max_precedence;
    B32 type_ancestors = state->top_task->do_type_ancestors;
    B32 need_new_expr = (expr == &e2_expr_nil && !type_ancestors && !state->caller_info_completes_task);
    E2_Token token = {0};
    
    //- rjf: skip leading whitespace / comments
    for(;;)
    {
      E2_Token next_token = {0};
      U64 next_off = off + e2_read_token(lang, string, off, &next_token);
      if(next_token.kind == E2_TokenKind_Whitespace ||
         next_token.kind == E2_TokenKind_Comment)
      {
        off = next_off;
      }
      else
      {
        break;
      }
    }
    
    //- rjf: nested sub-expressions
    if(need_new_expr && e2_try_token(lang, string, E2_TokenKind_Symbol, s("("), &off, &token))
    {
      E2_ParseTask *task = state->free_task;
      if(task != 0)
      {
        SLLStackPop(state->free_task);
      }
      else
      {
        task = push_array_no_zero(arena, E2_ParseTask, 1);
      }
      MemoryZeroStruct(task);
      task->src_range = token.range;
      task->expected_closer = s(")");
      task->child_count_target = 1;
      task->max_precedence = max_S64;
      SLLStackPush(state->top_task, task);
    }
    
    //- rjf: symbols (possible prefix unaries, *or* unexpected)
    else if(need_new_expr && e2_try_token(lang, string, E2_TokenKind_Symbol, s(""), &off, &token))
    {
      String8 token_string = str8_substr(string, token.range);
      
      // rjf: string -> operator kind
      B32 unexpected = !str8_match(token_string, state->top_task->expected_closer, 0);
      E2_ExprKind expr_kind = E2_ExprKind_Null;
      String8 closer = {0};
      S64 precedence = 0;
      {
        for EachIndex(idx, lang_info->expr_kind_parse_infos_count)
        {
          if(lang_info->expr_kind_parse_infos[idx].parse_kind == E2_ExprParseKind_Prefix &&
             str8_match(token_string, lang_info->expr_kind_parse_infos[idx].pre, 0))
          {
            unexpected = 0;
            if(lang_info->expr_kind_parse_infos[idx].precedence <= max_precedence)
            {
              expr_kind = lang_info->expr_kind_parse_infos[idx].expr_kind;
              closer = lang_info->expr_kind_parse_infos[idx].post;
              precedence = lang_info->expr_kind_parse_infos[idx].precedence;
              break;
            }
          }
        }
      }
      
      // rjf: push task for operand
      if(expr_kind != E2_ExprKind_Null)
      {
        E2_ParseTask *task = state->free_task;
        if(task != 0)
        {
          SLLStackPop(state->free_task);
        }
        else
        {
          task = push_array_no_zero(arena, E2_ParseTask, 1);
        }
        MemoryZeroStruct(task);
        task->src_range          = token.range;
        task->child_count_target = e2_expr_kind_target_operand_count_table[expr_kind];
        task->expr_kind          = expr_kind;
        task->max_precedence     = precedence;
        task->expected_closer    = closer;
        SLLStackPush(state->top_task, task);
      }
      
      // rjf: report unexpected symbols
      if(expr_kind == E2_ExprKind_Null && unexpected)
      {
        e2_msgf(arena, &parse.msgs, token.range, "Unexpected `%S`.", token_string);
      }
    }
    
    //- rjf: identifiers with an active dot op kind -> member access
    else if(need_new_expr && state->top_task->expr_kind == E2_ExprKind_Dot && e2_try_token(lang, string, E2_TokenKind_Identifier, s(""), &off, &token))
    {
      expr = e2_expr(arena, E2_ExprKind_Dot);
      expr->first_child = state->top_task->first_child;
      expr->last_child  = state->top_task->last_child;
      expr->child_count = state->top_task->child_count;
      expr->src_range   = state->top_task->src_range;
      expr->string      = str8_substr(string, token.range);
      E2_ParseTask *task = state->top_task;
      SLLStackPop(state->top_task);
      SLLStackPush(state->free_task, task);
    }
    
    //- rjf: standalone identifiers (also could be definition or operator keyword)
    else if(need_new_expr && e2_try_token(lang, string, E2_TokenKind_Identifier, s(""), &off, &token))
    {
      String8 identifier = str8_substr(string, token.range);
      B32 identifier_mapped = 0;
      B32 definitions_are_allowed = (state->top_task->expr_kind != E2_ExprKind_Macro);
      
      // rjf: first try to resolve as a definition
      if(definitions_are_allowed && !identifier_mapped && e2_try_token(lang, string, E2_TokenKind_Symbol, s("="), &off, 0))
      {
        identifier_mapped = 1;
        E2_ParseTask *task = state->free_task;
        if(task != 0)
        {
          SLLStackPop(state->free_task);
        }
        else
        {
          task = push_array_no_zero(arena, E2_ParseTask, 1);
        }
        MemoryZeroStruct(task);
        task->src_range = token.range;
        task->child_count_target = e2_expr_kind_target_operand_count_table[E2_ExprKind_Define];
        task->expr_kind = E2_ExprKind_Define;
        task->max_precedence = max_S64;
        task->identifier = identifier;
        SLLStackPush(state->top_task, task);
      }
      
      // rjf: try to resolve it as a macro definition
      if(definitions_are_allowed && !identifier_mapped)
      {
        U64 macro_off = off;
        if(e2_try_token(lang, string, E2_TokenKind_Symbol, s("("), &macro_off, 0))
        {
          U64 post_macro_paren_off = macro_off;
          B32 looks_like_macro_def = 1;
          U64 macro_arg_count = 0;
          for(;macro_off < string.size;)
          {
            if(!e2_try_token(lang, string, E2_TokenKind_Identifier, s(""), &macro_off, 0))
            {
              looks_like_macro_def = 0;
              break;
            }
            macro_arg_count += 1;
            e2_try_token(lang, string, E2_TokenKind_Symbol, s(","), &macro_off, 0);
            if(e2_try_token(lang, string, E2_TokenKind_Symbol, s(")"), &macro_off, 0))
            {
              break;
            }
          }
          if(looks_like_macro_def && e2_try_token(lang, string, E2_TokenKind_Symbol, s("="), &macro_off, 0))
          {
            // rjf: push task to fill macro definition
            identifier_mapped = 1;
            E2_ParseTask *task = state->free_task;
            if(task != 0)
            {
              SLLStackPop(state->free_task);
            }
            else
            {
              task = push_array_no_zero(arena, E2_ParseTask, 1);
            }
            MemoryZeroStruct(task);
            task->src_range = token.range;
            task->child_count_target = e2_expr_kind_target_operand_count_table[E2_ExprKind_Macro];
            task->expr_kind = E2_ExprKind_Macro;
            task->max_precedence = max_S64;
            task->identifier = identifier;
            SLLStackPush(state->top_task, task);
            
            // rjf: gather argument names
            macro_off = post_macro_paren_off;
            for(;macro_off < string.size;)
            {
              E2_Token next_token = {0};
              if(!e2_try_token(lang, string, E2_TokenKind_Identifier, s(""), &macro_off, &next_token))
              {
                break;
              }
              str8_list_push(arena, &task->macro_arg_names, str8_substr(string, next_token.range));
              e2_try_token(lang, string, E2_TokenKind_Symbol, s(","), &macro_off, 0);
              if(e2_try_token(lang, string, E2_TokenKind_Symbol, s(")"), &macro_off, 0))
              {
                break;
              }
            }
            e2_try_token(lang, string, E2_TokenKind_Symbol, s("="), &macro_off, 0);
            
            // rjf: advance offset to past '='
            off = macro_off;
          }
        }
      }
      
      // rjf: first try to resolve as an operator name
      if(!identifier_mapped)
      {
        // rjf: string -> op kind
        E2_ExprKind expr_kind = E2_ExprKind_Null;
        S64 precedence = 0;
        {
          String8 token_string = str8_substr(string, token.range);
          for EachIndex(idx, lang_info->expr_kind_parse_infos_count)
          {
            if(lang_info->expr_kind_parse_infos[idx].parse_kind == E2_ExprParseKind_Prefix &&
               lang_info->expr_kind_parse_infos[idx].precedence <= max_precedence &&
               str8_match(token_string, str8_skip_chop_whitespace(lang_info->expr_kind_parse_infos[idx].pre), 0))
            {
              expr_kind = lang_info->expr_kind_parse_infos[idx].expr_kind;
              precedence = lang_info->expr_kind_parse_infos[idx].precedence;
              break;
            }
          }
        }
        
        // rjf: push task for operand
        if(expr_kind != E2_ExprKind_Null)
        {
          identifier_mapped = 1;
          E2_ParseTask *task = state->free_task;
          if(task != 0)
          {
            SLLStackPop(state->free_task);
          }
          else
          {
            task = push_array_no_zero(arena, E2_ParseTask, 1);
          }
          MemoryZeroStruct(task);
          task->src_range = token.range;
          task->child_count_target = e2_expr_kind_target_operand_count_table[expr_kind];
          task->expr_kind = expr_kind;
          task->max_precedence = precedence;
          SLLStackPush(state->top_task, task);
        }
      }
      
      // rjf: try to resolve it as a macro argument
      if(!identifier_mapped && state->top_task->expr_kind == E2_ExprKind_Macro)
      {
        U64 macro_arg_num = 0;
        {
          U64 num = 1;
          for(String8Node *n = state->top_task->macro_arg_names.first; n != 0; n = n->next, num += 1)
          {
            if(str8_match(n->string, identifier, 0))
            {
              macro_arg_num = num;
              break;
            }
          }
        }
        if(macro_arg_num != 0)
        {
          identifier_mapped = 1;
          expr = e2_expr(arena, E2_ExprKind_MacroArg);
          expr->macro_arg_num = (U32)macro_arg_num;
          expr->string = identifier;
        }
      }
      
      // rjf: build this as a leaf identifier expression - ask the caller if it's a type
      if(!identifier_mapped)
      {
        if(state->caller_request_count == 0)
        {
          done = 1;
          parse.status = E2_ParseStatus_CheckIdentifierIsType;
          parse.identifier = identifier;
          off = start_off;
        }
        else
        {
          identifier_mapped = 1;
          expr = e2_expr(arena, identifier_is_type ? E2_ExprKind_TypeIdentifier : E2_ExprKind_Identifier);
          expr->string = str8_substr(string, token.range);
          state->caller_request_count = 0;
        }
      }
    }
    
    //- rjf: leaf numerics
    else if(need_new_expr && e2_try_token(lang, string, E2_TokenKind_Numeric, s(""), &off, &token))
    {
      expr = e2_expr(arena, E2_ExprKind_Numeric);
      expr->string = str8_substr(string, token.range);
    }
    
    //- rjf: leaf string literals
    else if(need_new_expr && e2_try_token(lang, string, E2_TokenKind_StringLiteral, s(""), &off, &token))
    {
      String8 token_string = str8_substr(string, token.range);
      String8 unquoted_string = str8_skip(str8_chop(token_string, 1), 1);
      String8 raw_string = raw_from_escaped_str8(arena, unquoted_string);
      expr = e2_expr(arena, E2_ExprKind_StringLiteral);
      expr->string = raw_string;
    }
    
    //- rjf: leaf character literals
    else if(need_new_expr && e2_try_token(lang, string, E2_TokenKind_CharLiteral, s(""), &off, &token))
    {
      String8 token_string = str8_substr(string, token.range);
      String8 unquoted_string = str8_skip(str8_chop(token_string, 1), 1);
      String8 raw_string = raw_from_escaped_str8(arena, unquoted_string);
      expr = e2_expr(arena, E2_ExprKind_CharLiteral);
      expr->string = raw_string;
    }
    
    //- rjf: parse left-hand-side type operators
    if(!state->caller_info_completes_task && (expr != &e2_expr_nil || state->top_task->do_type_ancestors))
    {
      U64 trailing_symbol_off = off;
      if(e2_try_token(lang, string, E2_TokenKind_Symbol, s(""), &trailing_symbol_off, &token))
      {
        B32 parent_looking_for_type = (state->top_task->expr_kind != E2_ExprKind_Null &&
                                       state->top_task->child_count == 0 &&
                                       e2_expr_kind_is_first_operand_type_maybe_table[state->top_task->expr_kind]);
        B32 is_expr_type = (e2_expr_kind_is_type_expr_table[expr->kind] || parent_looking_for_type);
        if(is_expr_type || state->top_task->do_type_ancestors)
        {
          // rjf: token string -> operator kind
          E2_ExprKind expr_kind = E2_ExprKind_Null;
          {
            String8 token_string = str8_substr(string, token.range);
            for EachIndex(idx, lang_info->expr_kind_parse_infos_count)
            {
              E2_ExprKindParseInfo *op_info = &lang_info->expr_kind_parse_infos[idx];
              if(e2_expr_kind_is_type_expr_table[op_info->expr_kind] &&
                 op_info->sep.size == 0 &&
                 str8_match(op_info->post, token_string, 0))
              {
                expr_kind = op_info->expr_kind;
                break;
              }
            }
          }
          
          // rjf: got an expression kind -> build new operator node
          if(expr_kind != E2_ExprKind_Null)
          {
            E2_Expr *op_expr = e2_expr(arena, expr_kind);
            if(expr != &e2_expr_nil)
            {
              e2_expr_push_child(arena, op_expr, expr);
            }
            expr = op_expr;
            off = trailing_symbol_off;
          }
        }
      }
    }
    
    //- rjf: if we're parsing a type, and we see a C-style type info recursion (to
    // express ancestors of the current type), then we need to generate a task to
    // descend and parse the ancestor type
    if(!state->caller_info_completes_task && expr != &e2_expr_nil && (state->top_task->next_type_ancestor == 0 || state->top_task->next_type_ancestor == &e2_expr_nil))
    {
      B32 parent_looking_for_type = (state->top_task->expr_kind != E2_ExprKind_Null &&
                                     state->top_task->child_count == 0 &&
                                     e2_expr_kind_is_first_operand_type_maybe_table[state->top_task->expr_kind]);
      B32 is_expr_type = (e2_expr_kind_is_type_expr_table[expr->kind] || parent_looking_for_type);
      if(is_expr_type && e2_try_token(lang, string, E2_TokenKind_Symbol, s("("), &off, &token))
      {
        E2_ParseTask *task = state->free_task;
        if(task != 0)
        {
          SLLStackPop(state->free_task);
        }
        else
        {
          task = push_array_no_zero(arena, E2_ParseTask, 1);
        }
        MemoryZeroStruct(task);
        task->src_range = token.range;
        task->expected_closer = s(")");
        task->child_count_target = 1;
        task->max_precedence = max_S64;
        task->do_type_ancestors = 1;
        task->type_lhs = expr;
        SLLStackPush(state->top_task, task);
        expr = &e2_expr_nil;
      }
    }
    
    //- rjf: extend parsed expression tree with trailing operators
    if(!state->caller_info_completes_task && expr != &e2_expr_nil)
    {
      U64 trailing_symbol_off = off;
      if(e2_try_token(lang, string, E2_TokenKind_Symbol, s(""), &trailing_symbol_off, &token) ||
         e2_try_token(lang, string, E2_TokenKind_Identifier, s(""), &trailing_symbol_off, &token))
      {
        // rjf: determine if the expression is a type
        B32 expr_is_type = e2_expr_kind_is_type_expr_table[expr->kind];
        
        // rjf: token string -> operator kind
        E2_ExprKind expr_kind = E2_ExprKind_Null;
        U64 child_count_target = 2;
        String8 splitter = {0};
        String8 closer = {0};
        B32 splitter_is_required = 0;
        S64 precedence = 0;
        B32 reverse_children = 0;
        {
          String8 token_string = str8_substr(string, token.range);
          for EachIndex(idx, lang_info->expr_kind_parse_infos_count)
          {
            E2_ExprKindParseInfo *expr_kind_info = &lang_info->expr_kind_parse_infos[idx];
            if(expr_kind_info->precedence <= max_precedence &&
               str8_match(token_string, expr_kind_info->sep, 0) &&
               (!expr_is_type || e2_expr_kind_allow_type_operands_table[expr_kind_info->expr_kind]))
            {
              splitter = expr_kind_info->chain;
              closer = expr_kind_info->post;
              expr_kind = expr_kind_info->expr_kind;
              precedence = expr_kind_info->precedence;
              reverse_children = expr_kind_info->reverse_children;
              child_count_target = e2_expr_kind_target_operand_count_table[expr_kind];
              break;
            }
          }
        }
        
        // rjf: non-null operator kind -> push task to fill other children of this operator node
        if(expr_kind != E2_ExprKind_Null)
        {
          E2_ParseTask *task = state->free_task;
          if(task != 0)
          {
            SLLStackPop(state->free_task);
          }
          else
          {
            task = push_array_no_zero(arena, E2_ParseTask, 1);
          }
          MemoryZeroStruct(task);
          task->src_range = token.range;
          task->child_count = 0;
          task->child_count_target = child_count_target;
          task->expr_kind = expr_kind;
          task->reverse_children = reverse_children;
          task->max_precedence = precedence;
          task->expected_splitter = splitter;
          task->splitter_min_child_count = 2;
          task->expected_closer = closer;
          task->splitter_is_required = splitter_is_required;
          SLLStackPush(state->top_task, task);
          off = trailing_symbol_off;
        }
      }
    }
    
    //- rjf: attach formed expressions to parent task; pop task if done.
    if(state->caller_info_completes_task || expr != &e2_expr_nil)
    {
      //- rjf: if this task has a type ancestor: our finished expression is
      // actually a descendant of that. we want to push our finished expression
      // as a child, and then use the next type ancestor as the root expression
      // which we push to our current task.
      if(state->top_task->next_type_ancestor != 0 && state->top_task->next_type_ancestor != &e2_expr_nil)
      {
        E2_Expr *type_descendant = expr;
        expr = state->top_task->next_type_ancestor;
        state->top_task->next_type_ancestor = &e2_expr_nil;
        e2_expr_push_child_front(arena, expr, type_descendant);
      }
      
      //- rjf: gather finished expression to current task; increase task child count
      if(!state->caller_info_completes_task)
      {
        E2_ExprNode *n = push_array(arena, E2_ExprNode, 1);
        if(state->top_task->reverse_children)
        {
          SLLQueuePushFront(state->top_task->first_child, state->top_task->last_child, n);
        }
        else
        {
          SLLQueuePush(state->top_task->first_child, state->top_task->last_child, n);
        }
        n->v = expr;
        state->top_task->child_count += 1;
      }
      
      //- rjf: consume expected splitters
      if(!state->caller_info_completes_task &&
         state->top_task->child_count >= state->top_task->splitter_min_child_count &&
         state->top_task->expected_splitter.size != 0 &&
         state->top_task->child_count < state->top_task->child_count_target)
      {
        if(!e2_try_token(lang, string, E2_TokenKind_Symbol, state->top_task->expected_splitter, &off, 0) && state->top_task->splitter_is_required)
        {
          e2_msgf(arena, &parse.msgs, r1u64(off, off), "Expected `%S`.", state->top_task->expected_splitter);
        }
      }
      
      //- rjf: consume expected closers - can terminate a child list early
      B32 closer_found = 0;
      if(!state->caller_info_completes_task && state->top_task->expected_closer.size != 0)
      {
        closer_found = e2_try_token(lang, string, E2_TokenKind_Symbol, state->top_task->expected_closer, &off, 0);
      }
      
      //- rjf: determine if the task above a parenthesized sub-expression is expecting a type
      B32 parent_looking_for_type = 0;
      if(!state->top_task->do_type_ancestors &&
         state->top_task->next != 0 &&
         state->top_task->next->expr_kind != E2_ExprKind_Null &&
         state->top_task->next->child_count == 0 &&
         e2_expr_kind_is_first_operand_type_maybe_table[state->top_task->next->expr_kind])
      {
        parent_looking_for_type = 1;
      }
      
      //- rjf: determine if first child of a parenthesized sub-expression is a type
      B32 parenthesized_child_is_type = 0;
      if(!state->top_task->do_type_ancestors && !parent_looking_for_type && closer_found && state->top_task->child_count == 1)
      {
        E2_Expr *expr = state->top_task->first_child->v;
        parenthesized_child_is_type = 1;
        for(E2_Expr *e = expr, *next_e; e != &e2_expr_nil && parenthesized_child_is_type; e = next_e)
        {
          next_e = &e2_expr_nil;
          if(e->kind != E2_ExprKind_TypeIdentifier &&
             e->kind != E2_ExprKind_Ptr)
          {
            parenthesized_child_is_type = 0;
          }
          if(parenthesized_child_is_type && e->child_count == 1)
          {
            next_e = e->first_child->v;
          }
          else if(e->child_count > 1)
          {
            parenthesized_child_is_type = 0;
          }
        }
      }
      
      //- rjf: closer found, completed child count == 1 & completed child is a
      // type evaluation - then this is cast. push a task to fill the operand
      B32 is_cast = parenthesized_child_is_type;
      if(!parent_looking_for_type && is_cast)
      {
        // rjf: pop the sub-task for the type expression
        E2_Expr *type_expr = state->top_task->first_child->v;
        {
          E2_ParseTask *completed_task = state->top_task;
          SLLStackPop(state->top_task);
          SLLStackPush(state->free_task, completed_task);
        }
        
        // rjf: push a new task for the castee operand
        {
          E2_ParseTask *task = state->free_task;
          if(task != 0)
          {
            SLLStackPop(state->free_task);
          }
          else
          {
            task = push_array_no_zero(arena, E2_ParseTask, 1);
          }
          MemoryZeroStruct(task);
          task->src_range = type_expr->src_range;
          task->child_count = 1;
          task->child_count_target = e2_expr_kind_target_operand_count_table[E2_ExprKind_Cast];
          task->expr_kind = E2_ExprKind_Cast;
          task->max_precedence = 1;
          SLLStackPush(state->top_task, task);
          expr = &e2_expr_nil;
          E2_ExprNode *child_n = push_array(arena, E2_ExprNode, 1);
          SLLQueuePush(task->first_child, task->last_child, child_n);
          child_n->v = type_expr;
        }
      }
      
      //- rjf: caller-provided info completes a task, *or* closer found,
      // *or* task child count hits limit -> complete this subtree
      else if(state->caller_info_completes_task || closer_found || state->top_task->child_count >= state->top_task->child_count_target)
      {
        B32 completed_with_caller_info = state->caller_info_completes_task;
        state->caller_info_completes_task = 0;
        E2_ParseTask *completed_task = state->top_task;
        
        //- rjf: produced finished expression tree, given all children
        E2_Expr *finished_root = &e2_expr_nil;
        {
          if(completed_task->expr_kind == E2_ExprKind_Null && completed_task->child_count == 1)
          {
            finished_root = completed_task->first_child->v;
          }
          else
          {
            finished_root = e2_expr(arena, completed_task->expr_kind);
            finished_root->first_child = completed_task->first_child;
            finished_root->last_child = completed_task->last_child;
            finished_root->child_count = completed_task->child_count;
            finished_root->src_range = completed_task->src_range;
          }
        }
        
        //- rjf: report missing closers
        if(!completed_with_caller_info && completed_task->expected_closer.size != 0 && !closer_found)
        {
          e2_msgf(arena, &parse.msgs, r1u64(off, off), "Expected `%S`.", completed_task->expected_closer);
        }
        
        //- rjf: if this task parsed type ancestors:
        //
        // we need to pop this task, and return to working on the original left-hand-side
        // type descendant. but, we need to remember the parsed type ancestors, so that
        // we can apply them to the type once the right-hand-side has been parsed.
        //
        // otherwise (normal case): work on the parent of the finished expression next,
        // using the finished expression as the completed child.
        //
        if(completed_task->do_type_ancestors)
        {
          expr = completed_task->type_lhs;
          completed_task->next->next_type_ancestor = finished_root;
        }
        else
        {
          expr = finished_root;
        }
        
        //- rjf: release task
        if(!done)
        {
          SLLStackPop(state->top_task);
          SLLStackPush(state->free_task, completed_task);
        }
      }
      
      //- rjf: if the top task is not done -> continue parsing.
      //
      // if we have an active op kind, reset expression, because we need to parse another.
      // if we don't, we need to look for extensions of our current expression instead.
      //
      else if(state->top_task->expr_kind != E2_ExprKind_Null)
      {
        expr = &e2_expr_nil;
      }
    }
    
    //- rjf: we're done if we couldn't make any more progress.
    if(start_task == state->top_task && (off >= string.size || (off == start_off && !e2_parse_status_is_caller_request(parse.status))))
    {
      done = 1;
      if(expr != &e2_expr_nil)
      {
        parse.expr = expr;
      }
      if(parse.status == E2_ParseStatus_Error)
      {
        parse.status = E2_ParseStatus_Good;
      }
    }
  }
  
  //- rjf: asking caller for info? -> if we already did this at this offset,
  // the user couldn't provide the info, so we just fail out.
  if(e2_parse_status_is_caller_request(parse.status))
  {
    if(state->caller_request_count > 0 && state->last_caller_request_string_off == state->string_off)
    {
      if(parse.status == E2_ParseStatus_CheckIdentifierIsType)
      {
        e2_msgf(arena, &parse.msgs, r1u64(state->string_off, state->string_off + parse.identifier.size), "`%S` couldn't be resolved.", parse.identifier);
      }
      parse.status = E2_ParseStatus_Error;
    }
    state->caller_request_count += 1;
    state->last_caller_request_string_off = state->string_off;
  }
  
  //- rjf: report incomplete tasks
  if(off >= string.size && !e2_parse_status_is_caller_request(parse.status))
  {
    for(E2_ParseTask *t = state->top_task; t != 0 && t != &state->start_task; t = t->next)
    {
      String8 symbol = {0};
      if(t->expr_kind != E2_ExprKind_Null)
      {
        for EachIndex(idx, lang_info->expr_kind_parse_infos_count)
        {
          if(lang_info->expr_kind_parse_infos[idx].expr_kind == t->expr_kind)
          {
            U64 target_operand_count = e2_expr_kind_target_operand_count_table[t->expr_kind];
            if(target_operand_count == 2)
            {
              symbol = lang_info->expr_kind_parse_infos[idx].sep;
            }
            else if(target_operand_count == 1)
            {
              symbol = lang_info->expr_kind_parse_infos[idx].pre;
            }
            break;
          }
        }
      }
      if(t->child_count == 1 && t->child_count_target == 2 && t->expr_kind == E2_ExprKind_Dot)
      {
        e2_msgf(arena, &parse.msgs, t->src_range, "Couldn't access member `%S`.", state->last_requested_member_name);
      }
      else if(t->child_count == 2 && t->child_count_target == 2 && t->expr_kind == E2_ExprKind_Index)
      {
        e2_msgf(arena, &parse.msgs, t->src_range, "Couldn't index into this type.");
      }
      else if(t->child_count >= 1 && t->expr_kind == E2_ExprKind_Call)
      {
        e2_msgf(arena, &parse.msgs, t->src_range, "Couldn't call into this type.");
      }
      else if(t->child_count == 1 && t->child_count_target == 2 && t->expr_kind != E2_ExprKind_Null && symbol.size != 0)
      {
        e2_msgf(arena, &parse.msgs, t->src_range, "Expected expression after binary operator `%S`.", symbol);
      }
      else if(t->child_count == 0 && t->child_count_target == 1 && t->expr_kind != E2_ExprKind_Null && symbol.size != 0)
      {
        e2_msgf(arena, &parse.msgs, t->src_range, "Expected expression after unary operator `%S`.", symbol);
      }
      else
      {
        e2_msgf(arena, &parse.msgs, t->src_range, "Expected expression.");
      }
    }
  }
  
  //- rjf: commit offset
  {
    state->string_off = off;
  }
  
  return parse;
}

////////////////////////////////
//~ rjf: Expression -> IR Tree

internal E2_Compile
e2_compile_from_expr(Arena *arena, E2_CompileState *state, E2_IRNode *resolve_result, E2_Val compile_time_eval_result, E2_Expr *expr)
{
  E2_Compile compile = {E2_CompileStatus_Error};
  {
    E2_IRNode *next_resolve_result = resolve_result;
    
    //- rjf: generate initial task
    if(state->top_task == 0)
    {
      E2_CompileTask *task = push_array(arena, E2_CompileTask, 1);
      SLLStackPush(state->top_task, task);
      task->expr = expr;
    }
    
    //- rjf: make progress on the compile task stack
    for(B32 done = 0; !done && state->top_task != 0;)
    {
      E2_CompileTask *task = state->top_task;
      E2_Expr *e = task->expr;
      
      //- rjf: try to push a task for the next child, if we have one
      B32 new_child_task = 0;
      {
        E2_ExprNode *next_child_node = 0;
        if(task->last_compiled_child_node != 0)
        {
          next_child_node = state->top_task->last_compiled_child_node->next;
        }
        else if(task->irtree_child_count == 0)
        {
          next_child_node = e->first_child;
        }
        if(next_child_node != 0)
        {
          new_child_task = 1;
          task->last_compiled_child_node = next_child_node;
          E2_Expr *next_child = next_child_node->v;
          E2_CompileTask *new_task = state->free_task;
          if(new_task != 0)
          {
            SLLStackPop(state->free_task);
          }
          else
          {
            new_task = push_array_no_zero(arena, E2_CompileTask, 1);
          }
          MemoryZeroStruct(new_task);
          new_task->expr = next_child;
          SLLStackPush(state->top_task, new_task);
        }
      }
      
      //- rjf: if all children are complete -> compile this node
      E2_IRNode *finished_root = &e2_irnode_nil;
      if(!new_child_task)
      {
        E2_IRNode *lhs = task->first_irtree_child ? task->first_irtree_child->v : &e2_irnode_nil;
        E2_IRNode *rhs = task->last_irtree_child ? task->last_irtree_child->v : &e2_irnode_nil;
        E2_IRNode *mhs = task->irtree_child_count == 3 ? task->first_irtree_child->next->v : &e2_irnode_nil;
        {
          RDI_EvalOp op = RDI_EvalOp_Stop;
          E2_TypeKey dst_type_key = {E2_TypeKeyKind_Null};
          switch(e->kind)
          {
            //- rjf: no op -> just a sub-expression
            default:
            {
              if(lhs != rhs)
              {
                finished_root = e2_irnode(arena);
                finished_root->first_child = task->first_irtree_child;
                finished_root->last_child = task->last_irtree_child;
                finished_root->child_count = task->irtree_child_count;
              }
              else
              {
                finished_root = lhs;
              }
            }break;
            
            //- rjf: leaf identifiers
            case E2_ExprKind_Identifier:
            case E2_ExprKind_TypeIdentifier:
            {
              if(next_resolve_result != &e2_irnode_nil)
              {
                finished_root = next_resolve_result;
                next_resolve_result = &e2_irnode_nil;
                state->caller_request_count = 0;
              }
              else
              {
                done = 1;
                compile.status = E2_CompileStatus_MissedIdentifierResolution;
                compile.identifier = e->string;
              }
            }break;
            
            //- rjf: pointer type operators
            case E2_ExprKind_Ptr:
            {
              Arch arch = state->selected_ctx.arch;
              E2_TypeKey ptee_type_key = lhs->type_key;
              E2_TypeKey ptr_type_key = e2_type_key_cons_ptr(arch, ptee_type_key);
              finished_root = e2_irnode(arena);
              finished_root->type_key = ptr_type_key;
            }break;
            
            //- rjf: array type operator
            case E2_ExprKind_Array:
            {
              if(state->caller_request_count == 0)
              {
                done = 1;
                compile.status = E2_CompileStatus_CompileTimeEval;
                compile.irtree = rhs;
              }
              else
              {
                U64 array_count = compile_time_eval_result.u64;
                E2_TypeKey element_type_key = lhs->type_key;
                E2_TypeKey array_type_key = e2_type_key_cons_array(element_type_key, array_count);
                finished_root = e2_irnode(arena);
                finished_root->type_key = array_type_key;
              }
            }break;
            
            //- rjf: function type operator
            case E2_ExprKind_Function:
            {
              Temp scratch = scratch_begin(&arena, 1);
              U64 params_count = 0;
              E2_TypeKey *param_types = 0;
              if(task->irtree_child_count != 0)
              {
                params_count = task->irtree_child_count-1;
                param_types = push_array(scratch.arena, E2_TypeKey, params_count);
                U64 idx = 0;
                for(E2_IRNodePtrNode *n = task->first_irtree_child->next; n != 0; n = n->next, idx += 1)
                {
                  param_types[idx] = n->v->type_key;
                }
              }
              E2_TypeKey fn_type_key = e2_type_key_cons(E2_TypeKind_Function, .direct = lhs->type_key, .count = params_count, .param_types = param_types);
              finished_root = e2_irnode(arena);
              finished_root->type_key = fn_type_key;
              scratch_end(scratch);
            }break;
            
            //- rjf: const type operator
            case E2_ExprKind_Const:
            {
              E2_TypeKey const_type_key = e2_type_key_cons(E2_TypeKind_Modifier, .flags = E2_TypeFlag_Const, .direct = lhs->type_key);
              finished_root = e2_irnode(arena);
              finished_root->type_key = const_type_key;
            }break;
            
            //- rjf: volatile type operator
            case E2_ExprKind_Volatile:
            {
              E2_TypeKey volatile_type_key = e2_type_key_cons(E2_TypeKind_Modifier, .flags = E2_TypeFlag_Volatile, .direct = lhs->type_key);
              finished_root = e2_irnode(arena);
              finished_root->type_key = volatile_type_key;
            }break;
            
            //- rjf: unsigned type operator
            case E2_ExprKind_Unsigned:
            {
              E2_TypeKey unsigned_type_key = lhs->type_key;
              E2_TypeKey int_type_key = e2_type_key_undecorate(lhs->type_key);
              E2_TypeKind int_type_kind = e2_type_kind_from_key(int_type_key);
              switch(int_type_kind)
              {
                default:{}break;
                case E2_TypeKind_S8:  {unsigned_type_key = e2_type_key_basic(E2_TypeKind_U8);}break;
                case E2_TypeKind_S16: {unsigned_type_key = e2_type_key_basic(E2_TypeKind_U16);}break;
                case E2_TypeKind_S32: {unsigned_type_key = e2_type_key_basic(E2_TypeKind_U32);}break;
                case E2_TypeKind_S64: {unsigned_type_key = e2_type_key_basic(E2_TypeKind_U64);}break;
                case E2_TypeKind_S128:{unsigned_type_key = e2_type_key_basic(E2_TypeKind_U128);}break;
                case E2_TypeKind_S256:{unsigned_type_key = e2_type_key_basic(E2_TypeKind_U256);}break;
                case E2_TypeKind_S512:{unsigned_type_key = e2_type_key_basic(E2_TypeKind_U512);}break;
              }
              finished_root = e2_irnode(arena);
              finished_root->type_key = unsigned_type_key;
            }break;
            
            //- rjf: signed type operator
            case E2_ExprKind_Signed:
            {
              E2_TypeKey signed_type_key = lhs->type_key;
              E2_TypeKey int_type_key = e2_type_key_undecorate(lhs->type_key);
              E2_TypeKind int_type_kind = e2_type_kind_from_key(int_type_key);
              switch(int_type_kind)
              {
                default:{}break;
                case E2_TypeKind_U8:  {signed_type_key = e2_type_key_basic(E2_TypeKind_S8);}break;
                case E2_TypeKind_U16: {signed_type_key = e2_type_key_basic(E2_TypeKind_S16);}break;
                case E2_TypeKind_U32: {signed_type_key = e2_type_key_basic(E2_TypeKind_S32);}break;
                case E2_TypeKind_U64: {signed_type_key = e2_type_key_basic(E2_TypeKind_S64);}break;
                case E2_TypeKind_U128:{signed_type_key = e2_type_key_basic(E2_TypeKind_S128);}break;
                case E2_TypeKind_U256:{signed_type_key = e2_type_key_basic(E2_TypeKind_S256);}break;
                case E2_TypeKind_U512:{signed_type_key = e2_type_key_basic(E2_TypeKind_S512);}break;
              }
              finished_root = e2_irnode(arena);
              finished_root->type_key = signed_type_key;
            }break;
            
            //- rjf: leaf numerics
            case E2_ExprKind_Numeric:
            {
              U64 u64_val = 0;
              String8 numeric_string = e->string;
              U64 dot_pos = str8_find_needle(numeric_string, 0, s("."), 0); 
              B32 f_suffix = str8_match(str8_postfix(numeric_string, 1), s("f"), 0);
              U64 colon_pos = str8_find_needle(numeric_string, 0, s(":"), 0);
              if(dot_pos < numeric_string.size && f_suffix)
              {
                finished_root = e2_irnode_const_f32(arena, (F32)f64_from_str8(numeric_string));
              }
              else if(dot_pos < numeric_string.size && !f_suffix)
              {
                finished_root = e2_irnode_const_f64(arena, f64_from_str8(numeric_string));
              }
              else if(colon_pos < numeric_string.size)
              {
                Temp scratch = scratch_begin(&arena, 1);
                String8List parts = str8_split(scratch.arena, numeric_string, (U8 *)":", 1, 0);
                U64 u64_idx = 0;
                finished_root = e2_irnode(arena);
                for EachNode(n, String8Node, parts.first)
                {
                  if(u64_idx >= ArrayCount(finished_root->val.u512.u64))
                  {
                    break;
                  }
                  try_u64_from_str8_c_rules(n->string, &finished_root->val.u512.u64[u64_idx]);
                  u64_idx += 1;
                }
                switch(u64_idx)
                {
                  case 1:{finished_root->op = RDI_EvalOp_ConstU64; finished_root->type_key = e2_type_key_basic(E2_TypeKind_U64);}break;
                  case 2:{finished_root->op = RDI_EvalOp_ConstU128; finished_root->type_key = e2_type_key_basic(E2_TypeKind_U128);}break;
                  case 4:{finished_root->op = RDI_EvalOp_ConstU256; finished_root->type_key = e2_type_key_basic(E2_TypeKind_U256);}break;
                  case 8:{finished_root->op = RDI_EvalOp_ConstU512; finished_root->type_key = e2_type_key_basic(E2_TypeKind_U512);}break;
                  default:
                  {
                    e2_msgf(arena, &compile.msgs, e->src_range, "Invalid number of numeric portions specified (%I64u; must be 2, 4, or 8).", u64_idx);
                  }break;
                }
                scratch_end(scratch);
              }
              else if(try_u64_from_str8_c_rules(numeric_string, &u64_val))
              {
                finished_root = e2_irnode_const_u64_or_smaller(arena, u64_val);
              }
            }break;
            
            //- rjf: string literals
            case E2_ExprKind_StringLiteral:
            {
              finished_root = e2_irnode(arena);
              finished_root->op = RDI_EvalOp_ConstString;
              finished_root->string = e->string;
              finished_root->type_key = e2_type_key_cons_array(e2_type_key_basic(E2_TypeKind_UChar8), e->string.size);
              finished_root->val.u64 = e->string.size;
              finished_root->mode = E2_Mode_Value;
            }break;
            
            //- rjf: character literals
            case E2_ExprKind_CharLiteral:
            {
              UnicodeDecode decode = utf8_decode(e->string.str, e->string.size);
              finished_root = e2_irnode_const_u64_or_smaller(arena, (U64)decode.codepoint);
            }break;
            
            //- rjf: member accesses -> at this point we should have a resolved access
            // expression submitted to us by the user (the parser reports it first)
            // stored as the second child in the list. we just want to use the
            // second expression - the initial child was just the accessed evaluation.
            case E2_ExprKind_Dot:
            {
              if(next_resolve_result != &e2_irnode_nil)
              {
                finished_root = next_resolve_result;
                next_resolve_result = &e2_irnode_nil;
                state->caller_request_count = 0;
              }
              else
              {
                done = 1;
                compile.status = E2_CompileStatus_MemberAccess;
                compile.irtree = lhs;
                compile.member_name = e->string;
                finished_root = rhs;
              }
            }break;
            
            //- rjf: indexes
            case E2_ExprKind_Index:
            {
              E2_TypeKey lhs_type_key = e2_type_key_undecorate(lhs->type_key);
              E2_TypeKind lhs_type_kind = e2_type_kind_from_key(lhs_type_key);
              
              // rjf: array/pointer *address* indexes
              if(lhs_type_kind == E2_TypeKind_Ptr || lhs_type_kind == E2_TypeKind_Array)
              {
                E2_TypeKey element_type_key = e2_type_key_direct(lhs_type_key);
                U64 element_byte_size = e2_byte_size_from_type_key(element_type_key);
                E2_IRNode *base_addr_value_ir = e2_irnode_resolve_to_value(arena, lhs);
                E2_IRNode *index_value_ir = e2_irnode_resolve_to_value(arena, rhs);
                E2_IRNode *offset_value_ir = index_value_ir;
                if(element_byte_size != 1)
                {
                  E2_IRNode *element_size_value_ir = e2_irnode_const_u64_or_smaller(arena, element_byte_size);
                  offset_value_ir = e2_irnode_binary_op(arena, e2_type_key_basic(E2_TypeKind_U64), RDI_EvalOp_Mul, index_value_ir, element_size_value_ir);
                }
                E2_IRNode *element_addr_value_ir = e2_irnode_binary_op(arena, e2_type_key_basic(E2_TypeKind_U64), RDI_EvalOp_Add, base_addr_value_ir, offset_value_ir);
                element_addr_value_ir->type_key = element_type_key;
                element_addr_value_ir->mode = E2_Mode_Address;
                finished_root = element_addr_value_ir;
              }
              
              // rjf: fallback case: ask the caller for fancy indexing operations
              else
              {
                if(next_resolve_result != &e2_irnode_nil)
                {
                  finished_root = next_resolve_result;
                  next_resolve_result = &e2_irnode_nil;
                  state->caller_request_count = 0;
                }
                else
                {
                  done = 1;
                  compile.status = E2_CompileStatus_IndexAccess;
                  compile.irtree = lhs;
                  compile.params_irtree = rhs;
                }
              }
            }break;
            
            //- rjf: calls
            case E2_ExprKind_Call:
            {
              // rjf: calls of types redirect to casts
              if(lhs->mode == E2_Mode_Type)
              {
                goto cast;
              }
              
              // rjf: fallback => ask the caller to resolve the call
              else
              {
                if(next_resolve_result != &e2_irnode_nil)
                {
                  finished_root = next_resolve_result;
                  next_resolve_result = &e2_irnode_nil;
                  state->caller_request_count = 0;
                }
                else
                {
                  E2_IRNode *params_irtree = e2_irnode(arena);
                  params_irtree->first_child = task->first_irtree_child->next;
                  params_irtree->last_child = task->last_irtree_child;
                  params_irtree->child_count = task->irtree_child_count-1;
                  done = 1;
                  compile.status = E2_CompileStatus_Call;
                  compile.irtree = lhs;
                  compile.params_irtree = params_irtree;
                }
              }
            }break;
            
            //- rjf: sizeof
            case E2_ExprKind_SizeOf:
            {
              E2_TypeKey rhs_type_key = e2_type_key_unwrap(rhs->type_key, E2_TypeUnwrapFlag_AllDecorative & ~E2_TypeUnwrapFlag_Enums);
              U64 rhs_size = e2_byte_size_from_type_key(rhs_type_key);
              finished_root = e2_irnode_const_u64_or_smaller(arena, rhs_size);
              finished_root->type_key = e2_type_key_basic(E2_TypeKind_U64);
            }break;
            
            //- rjf: typeof
            case E2_ExprKind_TypeOf:
            {
              E2_TypeKey rhs_type_key = e2_type_key_unwrap(rhs->type_key, E2_TypeUnwrapFlag_AllDecorative & ~E2_TypeUnwrapFlag_Enums);
              finished_root = e2_irnode(arena);
              finished_root->mode = E2_Mode_Type;
              finished_root->type_key = rhs_type_key;
            }break;
            
            //- rjf: casts
            case E2_ExprKind_Cast:
            cast:;
            {
              E2_IRNode *cast_type_ir = lhs;
              E2_TypeKey cast_type_key = cast_type_ir->type_key;
              if(e2_type_key_match(e2_type_key_zero(), cast_type_key))
              {
                e2_msgf(arena, &compile.msgs, e->src_range, "Cannot cast to this type.");
              }
              E2_IRNode *castee_ir = rhs;
              finished_root = e2_irnode_convert_if_possible(arena, castee_ir, cast_type_key);
            }break;
            
            //- rjf: dereferences
            case E2_ExprKind_Deref:
            case E2_ExprKind_DerefAsm:
            {
              // rjf: unpack operand
              E2_TypeKey rhs_type_key = e2_type_key_unwrap(rhs->type_key, E2_TypeUnwrapFlag_AllDecorative & ~E2_TypeUnwrapFlag_Enums);
              E2_TypeKind rhs_type_kind = e2_type_kind_from_key(rhs_type_key);
              E2_TypeKey dereferenced_type = e2_type_key_unwrap(rhs->type_key, E2_TypeUnwrapFlag_All & ~E2_TypeUnwrapFlag_Enums);
              U64 dereferenced_type_size = e2_byte_size_from_type_key(dereferenced_type);
              
              // rjf: collect info about malformed situations
              B32 malformed = 0;
              if(dereferenced_type_size == 0 && e2_type_kind_is_ptr_or_ref(rhs_type_kind))
              {
                malformed = 1;
                e2_msgf(arena, &compile.msgs, e->src_range, "Cannot dereference pointers to zero-sized types.");
              }
              else if(dereferenced_type_size == 0 && rhs_type_kind == E2_TypeKind_Array)
              {
                malformed = 1;
                e2_msgf(arena, &compile.msgs, e->src_range, "Cannot dereference arrays of zero-sized types.");
              }
              else if(!e2_type_kind_is_ptr_or_ref(rhs_type_kind) && rhs_type_kind != E2_TypeKind_Array &&
                      e->kind != E2_ExprKind_DerefAsm)
              {
                malformed = 1;
                e2_msgf(arena, &compile.msgs, e->src_range, "Cannot dereference this type.");
              }
              
              // rjf: asm-style deref -> go to u64 if the address expression isn't
              // an array or pointer
              //
              // TODO(rjf): probably we should get an absolute-fallback-architecture here,
              // which ultimately is sourced by the selected thread, to choose an address-sized
              // integer type.
              //
              if(!e2_type_kind_is_ptr_or_ref(rhs_type_kind) && rhs_type_kind != E2_TypeKind_Array &&
                 e->kind == E2_ExprKind_DerefAsm)
              {
                dereferenced_type = e2_type_key_basic(E2_TypeKind_U64);
              }
              
              // rjf: not malformed -> equip info
              if(!malformed)
              {
                B32 is_only_type_eval = (rhs->mode == E2_Mode_Type);
                E2_IRNode *addr_value_tree = e2_irnode_resolve_to_value(arena, rhs);
                addr_value_tree->type_key = dereferenced_type;
                if(is_only_type_eval)
                {
                  addr_value_tree->mode = E2_Mode_Type;
                }
                else
                {
                  addr_value_tree->mode = E2_Mode_Address;
                }
                finished_root = addr_value_tree;
              }
            }break;
            
            //- rjf: address-of
            case E2_ExprKind_Address:
            {
              // rjf: determine if malformed
              B32 malformed = 0;
              if(rhs->mode != E2_Mode_Address)
              {
                malformed = 1;
                e2_msgf(arena, &compile.msgs, e->src_range, "Cannot take an address of a value; it does not exist in memory.");
              }
              
              // rjf: generate
              if(!malformed)
              {
                Arch arch = e2_arch_from_type_key(rhs->type_key);
                finished_root = rhs;
                finished_root->mode = E2_Mode_Value;
                finished_root->type_key = e2_type_key_cons_ptr(arch, rhs->type_key);
              }
            }break;
            
            //- rjf: positive (no-op, just take the right-hand-side)
            case E2_ExprKind_Pos:
            {
              finished_root = rhs;
            }break;
            
            //- rjf: unary ops
            case E2_ExprKind_Neg:    {op = RDI_EvalOp_Neg;}goto unary_op;
            case E2_ExprKind_LogNot: {op = RDI_EvalOp_LogNot;}goto unary_op;
            case E2_ExprKind_BitNot: {op = RDI_EvalOp_BitNot;}goto unary_op;
            unary_op:;
            {
              // rjf: unpack operand
              E2_TypeKey rhs_type_key = e2_type_key_unwrap(rhs->type_key, E2_TypeUnwrapFlag_AllDecorative & ~E2_TypeUnwrapFlag_Enums);
              E2_TypeKind rhs_type_kind = e2_type_kind_from_key(rhs_type_key);
              RDI_EvalTypeGroup rhs_type_group = e2_type_group_from_kind(rhs_type_kind);
              
              // rjf: determine if malformed
              B32 malformed = 0;
              if(!rdi_eval_op_typegroup_are_compatible(op, rhs_type_group))
              {
                malformed = 1;
                e2_msgf(arena, &compile.msgs, e->src_range, "Cannot use this operator on this type.");
              }
              
              // rjf: generate
              if(!malformed)
              {
                E2_TypeKey dst_type = rhs_type_key;
                if(RDI_EvalOp_FirstLogical <= op && op <= RDI_EvalOp_LastLogical)
                {
                  dst_type = e2_type_key_basic(E2_TypeKind_Bool);
                }
                else if(rhs_type_kind == E2_TypeKind_Bool ||
                        rhs_type_kind == E2_TypeKind_S8 ||
                        rhs_type_kind == E2_TypeKind_S16 ||
                        rhs_type_kind == E2_TypeKind_U8 ||
                        rhs_type_kind == E2_TypeKind_U16)
                {
                  dst_type = e2_type_key_basic(E2_TypeKind_S32);
                }
                E2_IRNode *operand = e2_irnode_resolve_to_value(arena, rhs);
                E2_IRNode *operand__converted = e2_irnode_convert_if_possible(arena, operand, dst_type);
                finished_root = e2_irnode_unary_op(arena, dst_type, op, operand__converted);
              }
            }break;
            
            //- rjf: binary ops
            case E2_ExprKind_Mul:   {op = RDI_EvalOp_Mul;}goto binary_op;
            case E2_ExprKind_Div:   {op = RDI_EvalOp_Div;}goto binary_op;
            case E2_ExprKind_Mod:   {op = RDI_EvalOp_Mod;}goto binary_op;
            case E2_ExprKind_Add:   {op = RDI_EvalOp_Add;}goto binary_op;
            case E2_ExprKind_Sub:   {op = RDI_EvalOp_Sub;}goto binary_op;
            case E2_ExprKind_LShift:{op = RDI_EvalOp_LShift;}goto binary_op;
            case E2_ExprKind_RShift:{op = RDI_EvalOp_RShift;}goto binary_op;
            case E2_ExprKind_Less:  {op = RDI_EvalOp_Less;}goto binary_op;
            case E2_ExprKind_LtEq:  {op = RDI_EvalOp_LsEq;}goto binary_op;
            case E2_ExprKind_Grtr:  {op = RDI_EvalOp_Grtr;}goto binary_op;
            case E2_ExprKind_GrEq:  {op = RDI_EvalOp_GrEq;}goto binary_op;
            case E2_ExprKind_EqEq:  {op = RDI_EvalOp_EqEq;}goto binary_op;
            case E2_ExprKind_NtEq:  {op = RDI_EvalOp_NtEq;}goto binary_op;
            case E2_ExprKind_BitAnd:{op = RDI_EvalOp_BitAnd;}goto binary_op;
            case E2_ExprKind_BitXor:{op = RDI_EvalOp_BitXor;}goto binary_op;
            case E2_ExprKind_BitOr: {op = RDI_EvalOp_BitOr;}goto binary_op;
            case E2_ExprKind_LogAnd:{op = RDI_EvalOp_LogAnd;}goto binary_op;
            case E2_ExprKind_LogOr: {op = RDI_EvalOp_LogOr;}goto binary_op;
            binary_op:;
            {
              // rjf: resolve lhs / rhs to values
              E2_IRNode *lhs_value = e2_irnode_resolve_to_value(arena, lhs);
              E2_IRNode *rhs_value = e2_irnode_resolve_to_value(arena, rhs);
              
              // rjf: unpack resolved lhs/rhs
              E2_TypeKey lhs_type_key = e2_type_key_undecorate(lhs_value->type_key);
              E2_TypeKey rhs_type_key = e2_type_key_undecorate(rhs_value->type_key);
              E2_TypeKind lhs_type_kind = e2_type_kind_from_key(lhs_type_key);
              E2_TypeKind rhs_type_kind = e2_type_kind_from_key(rhs_type_key);
              RDI_EvalTypeGroup lhs_type_group = e2_type_group_from_kind(lhs_type_kind);
              RDI_EvalTypeGroup rhs_type_group = e2_type_group_from_kind(rhs_type_kind);
              U64 lhs_type_size = e2_byte_size_from_type_key(lhs_type_key);
              U64 rhs_type_size = e2_byte_size_from_type_key(rhs_type_key);
              
              // rjf: determine kind of arithmetic
              typedef enum ArithKind
              {
                ArithKind_Normal,
                ArithKind_PtrAdd,
                ArithKind_PtrSub,
                ArithKind_PtrArrayCompare,
                ArithKind_TypeCompare,
              }
              ArithKind;
              ArithKind arith_kind = ArithKind_Normal;
              {
                if(lhs_value->mode == E2_Mode_Type || rhs_value->mode == E2_Mode_Type)
                {
                  arith_kind = ArithKind_TypeCompare;
                }
                else if((op == RDI_EvalOp_Add || op == RDI_EvalOp_Sub) &&
                        ((e2_type_kind_is_ptr_or_ref(lhs_type_kind) && e2_type_kind_is_integer(rhs_type_kind)) ||
                         (e2_type_kind_is_ptr_or_ref(rhs_type_kind) && e2_type_kind_is_integer(lhs_type_kind))))
                {
                  arith_kind = ArithKind_PtrAdd;
                }
                else if(op == RDI_EvalOp_Sub && e2_type_kind_is_ptr_or_ref(lhs_type_kind) && e2_type_kind_is_ptr_or_ref(rhs_type_kind))
                {
                  arith_kind = ArithKind_PtrSub;
                }
                else if((op == RDI_EvalOp_EqEq || op == RDI_EvalOp_NtEq) &&
                        ((e2_type_kind_is_ptr_or_ref(lhs_type_kind) && rhs_type_kind == E2_TypeKind_Array) ||
                         (e2_type_kind_is_ptr_or_ref(rhs_type_kind) && lhs_type_kind == E2_TypeKind_Array)))
                {
                  arith_kind = ArithKind_PtrArrayCompare;
                }
              }
              
              // rjf: generate
              switch(arith_kind)
              {
                //- rjf: normal arithmetic
                case ArithKind_Normal:
                {
                  // rjf: check malformation
                  B32 malformed = 0;
                  if(!rdi_eval_op_typegroup_are_compatible(op, lhs_type_group) ||
                     !rdi_eval_op_typegroup_are_compatible(op, rhs_type_group))
                  {
                    malformed = 1;
                    e2_msgf(arena, &compile.msgs, e->src_range, "Cannot use this operator on this type.");
                  }
                  
                  // rjf: generate
                  if(!malformed)
                  {
                    E2_TypeKey dst_type_key = lhs_type_key;
                    E2_IRNode *lhs_converted = lhs_value;
                    E2_IRNode *rhs_converted = rhs_value;
                    if(RDI_EvalOp_FirstLogical <= op && op <= RDI_EvalOp_LastLogical)
                    {
                      dst_type_key = e2_type_key_basic(E2_TypeKind_Bool);
                      E2_TypeKey coerced_type_key = e2_coerced_type_key_from_operands(lhs_type_key, rhs_type_key);
                      lhs_converted = e2_irnode_convert_if_possible(arena, lhs_value, coerced_type_key);
                      rhs_converted = e2_irnode_convert_if_possible(arena, rhs_value, coerced_type_key);
                    }
                    else
                    {
                      dst_type_key = e2_coerced_type_key_from_operands(lhs_type_key, rhs_type_key);
                      lhs_converted = e2_irnode_convert_if_possible(arena, lhs_value, dst_type_key);
                      rhs_converted = e2_irnode_convert_if_possible(arena, rhs_value, dst_type_key);
                    }
                    finished_root = e2_irnode_binary_op(arena, dst_type_key, op, lhs_converted, rhs_converted);
                  }
                }break;
                
                //- rjf: pointer-add arithmetic
                case ArithKind_PtrAdd:
                {
                  // rjf: l/r -> ptr + int
                  E2_IRNode *ptr_ir = lhs;
                  E2_IRNode *int_ir = rhs;
                  if(e2_type_kind_is_ptr_or_ref(rhs_type_kind))
                  {
                    ptr_ir = rhs;
                    int_ir = lhs;
                  }
                  
                  // rjf: unpack ptee type
                  E2_TypeKey ptee_type_key = e2_type_key_direct(e2_type_key_undecorate(ptr_ir->type_key));
                  U64 ptee_byte_size = e2_byte_size_from_type_key(ptee_type_key);
                  
                  // rjf: form tree
                  E2_IRNode *base_addr_ir = e2_irnode_resolve_to_value(arena, ptr_ir);
                  E2_IRNode *index_ir = e2_irnode_resolve_to_value(arena, int_ir);
                  E2_IRNode *offset_ir = index_ir;
                  if(ptee_byte_size > 1)
                  {
                    E2_IRNode *size_ir = e2_irnode_const_u64_or_smaller(arena, ptee_byte_size);
                    offset_ir = e2_irnode_binary_op(arena, e2_type_key_basic(E2_TypeKind_U64), RDI_EvalOp_Mul, index_ir, size_ir);
                  }
                  E2_IRNode *addr_value_ir = e2_irnode_binary_op(arena, e2_type_key_basic(E2_TypeKind_U64), RDI_EvalOp_Add, base_addr_ir, offset_ir);
                  addr_value_ir->mode = E2_Mode_Address;
                  addr_value_ir->type_key = ptr_ir->type_key;
                  finished_root = addr_value_ir;
                }break;
                
                //- rjf: pointer-sub arithmetic
                case ArithKind_PtrSub:
                {
                  E2_IRNode *lhs_addr_value_ir = e2_irnode_resolve_to_value(arena, lhs);
                  E2_IRNode *rhs_addr_value_ir = e2_irnode_resolve_to_value(arena, rhs);
                  E2_IRNode *sub_ir = e2_irnode_binary_op(arena, e2_type_key_basic(E2_TypeKind_U64), RDI_EvalOp_Sub, lhs_addr_value_ir, rhs_addr_value_ir);
                  E2_IRNode *sub_divided_ir = sub_ir;
                  E2_TypeKey lhs_ptr_type = e2_type_key_undecorate(lhs->type_key);
                  E2_TypeKey rhs_ptr_type = e2_type_key_undecorate(rhs->type_key);
                  E2_TypeKey lhs_ptee_type = e2_type_key_direct(lhs_ptr_type);
                  E2_TypeKey rhs_ptee_type = e2_type_key_direct(rhs_ptr_type);
                  if(e2_type_deep_match(lhs_ptee_type, rhs_ptee_type))
                  {
                    U64 ptee_byte_size = e2_byte_size_from_type_key(lhs_ptee_type);
                    if(ptee_byte_size != 0)
                    {
                      sub_divided_ir = e2_irnode_binary_op(arena, e2_type_key_basic(E2_TypeKind_U64), RDI_EvalOp_Div, sub_ir, e2_irnode_const_u64_or_smaller(arena, ptee_byte_size));
                    }
                  }
                  finished_root = sub_divided_ir;
                }break;
                
                //- rjf: pointer-array comparisons
                case ArithKind_PtrArrayCompare:
                {
                  // TODO(rjf)
                }break;
                
                //- rjf: type comparisons
                case ArithKind_TypeCompare:
                {
                  if(e->kind == E2_ExprKind_EqEq ||
                     e->kind == E2_ExprKind_NtEq)
                  {
                    E2_TypeKey lhs_type_key = e2_type_key_unwrap(lhs->type_key, E2_TypeUnwrapFlag_Meta);
                    E2_TypeKey rhs_type_key = e2_type_key_unwrap(rhs->type_key, E2_TypeUnwrapFlag_Meta);
                    B32 types_match = e2_type_deep_match(lhs_type_key, rhs_type_key);
                    B32 result = types_match;
                    if(e->kind == E2_ExprKind_NtEq)
                    {
                      result = !result;
                    }
                    finished_root = e2_irnode_const_u64_or_smaller(arena, (U64)result);
                    finished_root->type_key = e2_type_key_basic(E2_TypeKind_Bool);
                  }
                  else
                  {
                    e2_msgf(arena, &compile.msgs, e->src_range, "Cannot use this operator on types.");
                  }
                }break;
              }
            }break;
            
            //- rjf: definitions
            case E2_ExprKind_Define:
            {
              if(state->caller_request_count > 0)
              {
                finished_root = lhs;
              }
              else
              {
                done = 1;
                compile.status = E2_CompileStatus_NewIdentifierDefinition;
                compile.irtree = lhs;
                compile.identifier = e->string;
              }
            }break;
            
            //- rjf: conditionals
            case E2_ExprKind_Cond:
            {
              // rjf: unpack operands
              E2_IRNode *condition_ir = lhs;
              E2_IRNode *pass_ir = mhs;
              E2_IRNode *fail_ir = rhs;
              E2_TypeKey type_key = e2_coerced_type_key_from_operands(pass_ir->type_key, fail_ir->type_key);
              
              // rjf: determine if the mhs/rhs types force us to do compile-time evaluation of the condition
              B32 compile_time_cond = 0;
              {
                RDI_EvalTypeGroup pass_type_group = e2_type_group_from_kind(e2_type_kind_from_key(pass_ir->type_key));
                RDI_EvalTypeGroup fail_type_group = e2_type_group_from_kind(e2_type_kind_from_key(fail_ir->type_key));
                RDI_EvalConversionKind conversion_kind = rdi_eval_conversion_kind_from_typegroups(fail_type_group, pass_type_group);
                if(conversion_kind != RDI_EvalConversionKind_Legal &&
                   conversion_kind != RDI_EvalConversionKind_Noop)
                {
                  compile_time_cond = 1;
                }
                if(pass_ir->mode != fail_ir->mode)
                {
                  compile_time_cond = 1;
                }
              }
              
              // rjf: compile-time conditional -> ask caller to evaluate conditional
              if(compile_time_cond)
              {
                if(state->caller_request_count == 0)
                {
                  done = 1;
                  compile.status = E2_CompileStatus_CompileTimeEval;
                  compile.irtree = lhs;
                }
                else
                {
                  if(compile_time_eval_result.u64 == 0)
                  {
                    finished_root = fail_ir;
                  }
                  else
                  {
                    finished_root = pass_ir;
                  }
                }
              }
              
              // rjf: runtime conditional -> build opcodes for evaluating condition, jumping, etc.
              else
              {
                // rjf: convert pass/fail to coerced type
                pass_ir = e2_irnode_convert_if_possible(arena, pass_ir, type_key);
                fail_ir = e2_irnode_convert_if_possible(arena, fail_ir, type_key);
                
                // rjf: form conditional tree
                E2_IRNode *cond_root = e2_irnode(arena);
                E2_IRNode *cjump_ir = e2_irnode(arena);
                E2_IRNode *jump_ir = e2_irnode(arena);
                e2_irnode_push_child(arena, cond_root, condition_ir);
                e2_irnode_push_child(arena, cond_root, cjump_ir);
                e2_irnode_push_child(arena, cond_root, fail_ir);
                e2_irnode_push_child(arena, cond_root, jump_ir);
                e2_irnode_push_child(arena, cond_root, pass_ir);
                
                // rjf: compute # of bytes for pass
                U64 pass_ir_byte_count = 0;
                {
                  Temp scratch = scratch_begin(&arena, 1);
                  String8 pass_bytecode = e2_bytecode_from_irnode(scratch.arena, pass_ir);
                  pass_ir_byte_count = pass_bytecode.size;
                  scratch_end(scratch);
                }
                
                // rjf: configure unconditional jump (fail -> end)
                jump_ir->op = RDI_EvalOp_Skip;
                jump_ir->val.u64 = pass_ir_byte_count;
                
                // rjf: compute # of bytes for fail
                U64 fail_ir_byte_count = 0;
                {
                  Temp scratch = scratch_begin(&arena, 1);
                  String8 fail_bytecode = e2_bytecode_from_irnode(scratch.arena, fail_ir);
                  fail_ir_byte_count = fail_bytecode.size;
                  scratch_end(scratch);
                }
                
                // rjf: compute # of bytes for unconditional jump
                U64 jump_ir_byte_count = 0;
                {
                  Temp scratch = scratch_begin(&arena, 1);
                  String8 jump_bytecode = e2_bytecode_from_irnode(scratch.arena, jump_ir);
                  jump_ir_byte_count = jump_bytecode.size;
                  scratch_end(scratch);
                }
                
                // rjf: configure conditional jump
                cjump_ir->op = RDI_EvalOp_Cond;
                cjump_ir->val.u64 = fail_ir_byte_count + jump_ir_byte_count;
                
                // rjf: fill conditional root type
                cond_root->type_key = type_key;
                
                // rjf: take result
                finished_root = cond_root;
                finished_root->mode = pass_ir->mode;
              }
            }break;
          }
        }
      }
      
      //- rjf: if this task is done, pop - if there is a parent, push this task's
      // IR tree as a child - otherwise this is our final compilation result
      if(!done && !new_child_task)
      {
        SLLStackPop(state->top_task);
        SLLStackPush(state->free_task, task);
        if(state->top_task != 0)
        {
          E2_IRNodePtrNode *n = push_array(arena, E2_IRNodePtrNode, 1);
          SLLQueuePush(state->top_task->first_irtree_child, state->top_task->last_irtree_child, n);
          n->v = finished_root;
          state->top_task->irtree_child_count += 1;
        }
        else
        {
          compile.status = E2_CompileStatus_Good;
          compile.irtree = finished_root;
        }
      }
    }
    
    //- rjf: asking caller for info? -> if we already did this at this task,
    // the user couldn't provide the info, so we just fail out.
    if(e2_compile_status_is_caller_request(compile.status))
    {
      if(state->caller_request_count > 0)
      {
        if(compile.status == E2_CompileStatus_MissedIdentifierResolution)
        {
          e2_msgf(arena, &compile.msgs, state->top_task->expr->src_range, "`%S` couldn't be resolved.", compile.identifier);
        }
        compile.status = E2_CompileStatus_Error;
      }
      state->caller_request_count += 1;
    }
  }
  return compile;
}

////////////////////////////////
//~ rjf: IR Tree -> Bytecode

internal String8
e2_bytecode_from_irnode(Arena *arena, E2_IRNode *irnode)
{
  String8 result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List strings = {0};
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      E2_IRNode *ir;
      E2_IRNodePtrNode *last_pushed_child_node;
      U64 pushed_child_count;
    };
    Task start_task = {0, irnode};
    Task *top_task = &start_task;
    Task *free_task = 0;
    for(Task *t = top_task; t != 0; t = top_task)
    {
      E2_IRNode *ir = t->ir;
      
      //- rjf: unpack this op
      U16 ctrlbits = 0;
      U64 child_count = 0;
      if(0 < ir->op && ir->op < RDI_EvalOp_COUNT)
      {
        ctrlbits = rdi_eval_op_ctrlbits_table[ir->op];
        child_count = RDI_POPN_FROM_CTRLBITS(ctrlbits);
      }
      else
      {
        child_count = ir->child_count;
      }
      
      //- rjf: push the next child task if we can
      if(t->pushed_child_count < child_count)
      {
        t->pushed_child_count += 1;
        E2_IRNodePtrNode *next_child_node = (t->last_pushed_child_node == 0 ? ir->first_child : t->last_pushed_child_node->next);
        Task *child_task = free_task;
        if(child_task != 0)
        {
          SLLStackPop(free_task);
        }
        else
        {
          child_task = push_array_no_zero(scratch.arena, Task, 1);
        }
        MemoryZeroStruct(child_task);
        SLLStackPush(top_task, child_task);
        child_task->ir = next_child_node ? next_child_node->v : &e2_irnode_nil;
        child_task->last_pushed_child_node = 0;
        child_task->pushed_child_count = 0;
        t->last_pushed_child_node = next_child_node;
      }
      
      //- rjf: did push of all children -> push this expr's op, pop off stack
      else
      {
        if(ir->op != 0)
        {
          U16 ctrlbits = 0;
          if(ir->op < RDI_EvalOp_COUNT)
          {
            ctrlbits = rdi_eval_op_ctrlbits_table[ir->op];
          }
          else switch(ir->op)
          {
            default:{}break;
            case E2_EvalOp_SetCtxID:
            case E2_EvalOp_LeafBytecode:
            {
              ctrlbits = RDI_EVAL_CTRLBITS(8, 0, 0);
            }break;
          }
          U64 decode_byte_count = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
          U64 op_size = 1 + decode_byte_count;
          U8 *op_buffer = push_array(scratch.arena, U8, op_size);
          op_buffer[0] = ir->op;
          MemoryCopy(op_buffer+1, &ir->val, decode_byte_count);
          str8_list_push(scratch.arena, &strings, str8(op_buffer, op_size));
          if(ir->string.size != 0)
          {
            str8_list_push(scratch.arena, &strings, ir->string);
          }
        }
        SLLStackPop(top_task);
        SLLStackPush(free_task, t);
      }
    }
    result = str8_list_join(arena, &strings, 0);
    scratch_end(scratch);
  }
  return result;
}

////////////////////////////////
//~ rjf: Bytecode -> Result

internal E2_Interp
e2_interp_from_bytecode(Arena *arena, E2_InterpState *state, E2_SpaceMap *space_map, String8 bytecode)
{
  E2_Interp interp = {E2_InterpStatus_Error};
  {
    U64 off = state->bytecode_off;
    U64 off_opl = bytecode.size;
    B32 done = 0;
    B32 good = 1;
    
    //- rjf: execute bytecode
    for(;off < off_opl && !done && good;)
    {
      Temp scratch = scratch_begin(&arena, 1);
      U64 start_off = off;
      
      //- rjf: read next opcode
      RDI_EvalOp op = 0;
      off += str8_deserial_read_struct(bytecode, off, &op);
      
      //- rjf: opcode -> ctrl bits
      U16 ctrlbits = 0;
      switch(op)
      {
        default:
        if(op < RDI_EvalOp_COUNT)
        {
          ctrlbits = rdi_eval_op_ctrlbits_table[op];
        }break;
        case E2_EvalOp_SetCtxID:{ctrlbits = RDI_EVAL_CTRLBITS(8, 0, 0);}break;
      }
      
      //- rjf: ctrlbits -> push/pop/decode count
      U64 decode_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
      U64 push_count = RDI_PUSHN_FROM_CTRLBITS(ctrlbits);
      U64 pop_count = RDI_POPN_FROM_CTRLBITS(ctrlbits);
      
      //- rjf: do extra decode
      String8 decode_data = str8_substr(bytecode, r1u64(off, off+decode_size));
      E2_Val decode_val = {0};
      MemoryCopy(&decode_val, decode_data.str, Min(sizeof(decode_val), decode_data.size));
      RDI_EvalTypeGroup type_group = (RDI_EvalTypeGroup)decode_val.u512.u8[0];
      U64 op_arithmetic_size = (U64)decode_val.u512.u8[1];
      off += decode_size;
      
      //- rjf: prepare popped stack values & to-push stack values
      E2_Val *popped_vals = push_array(scratch.arena, E2_Val, pop_count);
      E2_Val *push_vals = push_array(scratch.arena, E2_Val, push_count);
      {
        E2_InterpStackValNode *node = state->top_val;
        for(U64 pop_idx = 0; node != 0 && pop_idx < pop_count; pop_idx += 1, node = node->next)
        {
          popped_vals[pop_idx] = node->val;
        }
      }
      
      //- rjf: do opcode
      E2_CtxFlags ctx_flags = 0;
      U64 ctx_base_addr = 0;
      switch(op)
      {
        default:
        {
          good = 0;
          interp.status = E2_InterpStatus_UnsupportedOp;
        }break;
        
        case E2_EvalOp_SetCtxID:
        {
          interp.status = E2_InterpStatus_NewCtxID;
          interp.ctx_id = decode_val.u64;
        }break;
        
        case RDI_EvalOp_Stop:
        {
          done = 1;
        }break;
        case RDI_EvalOp_Noop:{}break;
        case RDI_EvalOp_Cond:
        if(popped_vals[0].u64 != 0)
        {
          off += decode_val.u64;
        }break;
        case RDI_EvalOp_Skip:
        {
          off += decode_val.u64;
        }break;
        case RDI_EvalOp_MemRead:
        {
          U64 addr = popped_vals[0].u64;
          U64 size = decode_val.u64;
          Rng1U64 space_addr_range = r1u64(addr, addr+size);
          void *read_dst = &push_vals[0];
          if(size > sizeof(push_vals[0].u512))
          {
            read_dst = push_array(arena, U8, size);
          }
          U64 read_size = e2_space_map_read(space_map, state->selected_ctx.space_id, space_addr_range, read_dst);
          arena_pop(arena, size - read_size);
          push_vals[0].string = str8(read_dst, read_size);
          if(read_size != size)
          {
            good = 0;
            interp.status = E2_InterpStatus_MissedSpaceRead;
            interp.missed_read_space_addr_range = space_addr_range;
          }
        }break;
        case RDI_EvalOp_RegRead:
        {
          U8 rdi_reg_code = (decode_val.u64&0x0000FF)>>0;
          U8 byte_size    = (decode_val.u64&0x00FF00)>>8;
          U8 byte_off     = (decode_val.u64&0xFF0000)>>16;
          Arch arch = state->selected_ctx.arch;
          ARCH_Info *arch_info = arch_info_from_arch(arch);
          ARCH_RegCode base_reg_code = arch_reg_code_from_rdi(arch, rdi_reg_code);
          if(0 <= base_reg_code && base_reg_code < arch_info->reg_code_count)
          {
            Rng1U16 reg_rng = arch_info->reg_code_rng_table[base_reg_code];
            U64 off = (U64)reg_rng.min + byte_off;
            U64 size = (U64)byte_size;
            Rng1U64 space_addr_range = r1u64(off, off+size);
            U64 read_size = e2_space_map_read(space_map, state->selected_ctx.reg_space_id, space_addr_range, &push_vals[0]);
            if(read_size != size)
            {
              good = 0;
              interp.status = E2_InterpStatus_MissedSpaceRead;
              interp.missed_read_space_addr_range = space_addr_range;
            }
          }
          else
          {
            good = 0;
            interp.status = E2_InterpStatus_BadRegCode;
          }
        }break;
        case RDI_EvalOp_FrameOff:  {ctx_flags = E2_CtxFlag_HasFrameBase;  ctx_base_addr = state->selected_ctx.frame_base_addr;}goto optional_base_off;
        case RDI_EvalOp_ModuleOff: {ctx_flags = E2_CtxFlag_HasModuleBase; ctx_base_addr = state->selected_ctx.module_base_addr;}goto optional_base_off;
        case RDI_EvalOp_TLSOff:    {ctx_flags = E2_CtxFlag_HasTLSBase;    ctx_base_addr = state->selected_ctx.tls_base_addr;}goto optional_base_off;
        case RDI_EvalOp_PushCfa:   {ctx_flags = E2_CtxFlag_HasCFA;        ctx_base_addr = state->selected_ctx.cfa_addr;}goto optional_base_off;
        optional_base_off:;
        {
          if(state->selected_ctx.flags & ctx_flags)
          {
            push_vals[0].u64 = ctx_base_addr + decode_val.u64;
          }
          else
          {
            good = 0;
            interp.status = E2_InterpStatus_MissingCtxFlag;
            interp.missing_ctx_flags |= ctx_flags;
          }
        }break;
        
        case RDI_EvalOp_ConstU8:
        case RDI_EvalOp_ConstU16:
        case RDI_EvalOp_ConstU32:
        case RDI_EvalOp_ConstU64:
        case RDI_EvalOp_ConstU128:
        {
          push_vals[0] = decode_val;
        }break;
        
        case RDI_EvalOp_ConstString:
        {
          U64 string_size = decode_val.u64;
          String8 string = str8_substr(bytecode, r1u64(off, off+string_size));
          off += string_size;
          push_vals[0].string = string;
        }break;
        
        case RDI_EvalOp_Abs:
        {
          if(0){}
#define CaseA(target_type_group, target_type) else if(type_group == (RDI_EvalTypeGroup_##target_type_group)) do{push_vals[0].target_type = (popped_vals[0].target_type < 0 ? -popped_vals[0].target_type : +popped_vals[0].target_type);}while(0)
#define CaseB(target_type_group, size, target_type) else if(type_group == (RDI_EvalTypeGroup_##target_type_group) && op_arithmetic_size == (size)) do{push_vals[0].target_type = (popped_vals[0].target_type < 0 ? -popped_vals[0].target_type : +popped_vals[0].target_type);}while(0)
          CaseA(F32, f32);
          CaseA(F64, f64);
          CaseB(S, 8,  s8);
          CaseB(S, 16, s16);
          CaseB(S, 32, s32);
          CaseB(S, 64, s64);
          else { MemoryCopyStruct(&push_vals[0], &popped_vals[0]); }
#undef CaseA
#undef CaseB
        }break;
        
        case RDI_EvalOp_Neg:
        {
          if(0){}
#define CaseA(target_type_group, target_type) else if(type_group == (RDI_EvalTypeGroup_##target_type_group)) do{push_vals[0].target_type = (-popped_vals[0].target_type);}while(0)
#define CaseB(target_type_group, size, target_type) else if(type_group == (RDI_EvalTypeGroup_##target_type_group) && op_arithmetic_size == (size)) do{push_vals[0].target_type = (-popped_vals[0].target_type);}while(0)
          CaseA(F32, f32);
          CaseA(F64, f64);
          CaseB(S, 8,  s8);
          CaseB(S, 16, s16);
          CaseB(S, 32, s32);
          CaseB(S, 64, s64);
          else { MemoryCopyStruct(&push_vals[0], &popped_vals[0]); }
#undef CaseA
#undef CaseB
        }break;
        
#define BinOp(target_type_group, dst_type, src_type, symbol) else if(type_group == (RDI_EvalTypeGroup_##target_type_group)) do{push_vals[0].dst_type = popped_vals[1].src_type symbol popped_vals[0].src_type;}while(0)
#define SizedBinOp(target_type_group, size, dst_type, src_type, symbol) else if(type_group == (RDI_EvalTypeGroup_##target_type_group) && op_arithmetic_size == (size)) do{push_vals[0].dst_type = popped_vals[1].src_type symbol popped_vals[0].src_type;}while(0)
#define BinOpDiv(target_type_group, dst_type, src_type, symbol) else if(type_group == (RDI_EvalTypeGroup_##target_type_group)) do{if(popped_vals[0].src_type != 0) { push_vals[0].dst_type = popped_vals[1].src_type symbol popped_vals[0].src_type; } else {interp.status = E2_InterpStatus_DivideByZero; good = 0;} }while(0)
#define SizedBinOpDiv(target_type_group, size, dst_type, src_type, symbol) else if(type_group == (RDI_EvalTypeGroup_##target_type_group) && op_arithmetic_size == (size)) do{if(popped_vals[0].src_type != 0) { push_vals[0].dst_type = popped_vals[1].src_type symbol popped_vals[0].src_type; } else {interp.status = E2_InterpStatus_DivideByZero; good = 0;} }while(0)
#define BinOpDivFn(target_type_group, dst_type, src_type, fn) else if(type_group == (RDI_EvalTypeGroup_##target_type_group)) do{if(popped_vals[0].src_type != 0) { push_vals[0].dst_type = fn(popped_vals[1].src_type, popped_vals[0].src_type); } else {interp.status = E2_InterpStatus_DivideByZero; good = 0;} }while(0)
#define ArithTypeCasesInt(symbol)\
SizedBinOp(S, 8,  s8,  s8,  symbol);\
SizedBinOp(S, 16, s16, s16, symbol);\
SizedBinOp(S, 32, s32, s32, symbol);\
SizedBinOp(S, 64, s64, s64, symbol);\
SizedBinOp(U, 8,  u8,  u8,  symbol);\
SizedBinOp(U, 16, u16, u16, symbol);\
SizedBinOp(U, 32, u32, u32, symbol);\
SizedBinOp(U, 64, u64, u64, symbol);
#define ArithTypeCasesIntDiv(symbol)\
SizedBinOpDiv(S, 8,  s8,  s8,  symbol);\
SizedBinOpDiv(S, 16, s16, s16, symbol);\
SizedBinOpDiv(S, 32, s32, s32, symbol);\
SizedBinOpDiv(S, 64, s64, s64, symbol);\
SizedBinOpDiv(U, 8,  u8,  u8,  symbol);\
SizedBinOpDiv(U, 16, u16, u16, symbol);\
SizedBinOpDiv(U, 32, u32, u32, symbol);\
SizedBinOpDiv(U, 64, u64, u64, symbol);
#define LogTypeCases(symbol)\
BinOp(F32, u64, f32, symbol);\
BinOp(F64, u64, f64, symbol);\
ArithTypeCasesInt(symbol)
#define ArithTypeCases(symbol)\
BinOp(F32, f32, f32, symbol);\
BinOp(F64, f64, f64, symbol);\
ArithTypeCasesInt(symbol)
#define ArithTypeCasesDiv(symbol)\
BinOpDiv(F32, f32, f32, symbol);\
BinOpDiv(F64, f64, f64, symbol);\
ArithTypeCasesIntDiv(symbol)
#define ArithTypeCasesIntAllU64(symbol)\
BinOp(S, u64, u64, symbol);\
BinOp(U, u64, u64, symbol);
#define Case(name, ...) case RDI_EvalOp_##name:{if(0){} __VA_ARGS__ else {interp.status = E2_InterpStatus_BadOpTypes; good = 0;}}break
        Case(Add, ArithTypeCases(+));
        Case(Sub, ArithTypeCases(-));
        Case(Mul, ArithTypeCases(*));
        Case(Div, ArithTypeCasesDiv(/));
        Case(Mod, ArithTypeCasesIntDiv(%) BinOpDivFn(F32, f32, f32, mod_f32); BinOpDivFn(F64, f64, f64, mod_f64););
        Case(LShift, ArithTypeCasesInt(<<));
        Case(RShift, ArithTypeCasesInt(>>));
        Case(BitAnd, ArithTypeCasesIntAllU64(&));
        Case(BitOr,  ArithTypeCasesIntAllU64(|));
        Case(BitXor, ArithTypeCasesIntAllU64(^));
        Case(LogAnd, ArithTypeCasesIntAllU64(&&));
        Case(LogOr,  ArithTypeCasesIntAllU64(||));
        Case(LsEq,   LogTypeCases(<=));
        Case(GrEq,   LogTypeCases(>=));
        Case(Less,   LogTypeCases(<));
        Case(Grtr,   LogTypeCases(>));
#undef Case
#undef BinOp
#undef SizedBinOp
#undef BinOpDiv
#undef SizedBinOpDiv
#undef BinOpDivFn
#undef ArithTypeCasesInt
#undef ArithTypeCasesIntDiv
#undef ArithTypeCases
#undef ArithTypeCasesDiv
#undef ArithTypeCasesIntAllU64
        
        case RDI_EvalOp_LogNot:
        {
          push_vals[0].u64 = !popped_vals[0].u64;
        }break;
        case RDI_EvalOp_BitNot:
        {
          push_vals[0].u64 = ~popped_vals[0].u64;
        }break;
        
        case RDI_EvalOp_EqEq:
        case RDI_EvalOp_NtEq:
        {
          if(popped_vals[0].string.size != 0 && popped_vals[1].string.size != 0)
          {
            push_vals[0].u64 = str8_match(popped_vals[0].string, popped_vals[1].string, 0);
          }
          else
          {
            push_vals[0].u64 = MemoryMatchStruct(&popped_vals[0].u512, &popped_vals[1].u512);
          }
          if(op == RDI_EvalOp_NtEq)
          {
            push_vals[0].u64 = !push_vals[0].u64;
          }
        }break;
        
        case RDI_EvalOp_Trunc:
        {
          if(0 < decode_val.u64)
          {
            U64 mask = 0;
            if(decode_val.u64 < 64)
            {
              mask = max_U64 >> (64 - decode_val.u64);
            }
            push_vals[0].u64 = popped_vals[0].u64&mask;
          }
        }break;
        
        case RDI_EvalOp_TruncSigned:
        {
          if(0 < decode_val.u64)
          {
            U64 mask = 0;
            if(decode_val.u64 < 64)
            {
              mask = max_U64 >> (64 - decode_val.u64);
            }
            U64 high = 0;
            if(popped_vals[0].u64 & (1 << (decode_val.u64 - 1)))
            {
              high = ~mask;
            }
            push_vals[0].u64 = high|(popped_vals[0].u64&mask);
          }
        }break;
        
        case RDI_EvalOp_Convert:
        {
          U32 in = decode_val.u64&0xFF;
          U32 out = (decode_val.u64 >> 8)&0xFF;
          if(in != 0)
          {
            switch(in + out*RDI_EvalTypeGroup_COUNT)
            {
              default:{good = 0; interp.status = E2_InterpStatus_BadOpTypes;}break;
#define Case(dst_tg, dst_slot, dst_type, src_tg, src_slot) case RDI_EvalTypeGroup_##src_tg + RDI_EvalTypeGroup_##dst_tg*RDI_EvalTypeGroup_COUNT:{push_vals[0].dst_slot = (dst_type)popped_vals[0].src_slot;}break
              Case(U, u64, U64, F32, f32);
              Case(U, u64, U64, F64, f64);
              Case(S, s64, S64, F32, f32);
              Case(S, s64, S64, F64, f64);
              Case(F32, f32, F32, S, s64);
              Case(F64, f64, F64, S, s64);
              Case(F32, f32, F32, U, u64);
              Case(F64, f64, F64, U, u64); 
              Case(F32, f32, F32, F64, f64);
              Case(F64, f64, F64, F32, f32);
#undef Case
            }
          }
        }break;
        
        case RDI_EvalOp_Pick:
        {
          U64 target_idx = decode_val.u64;
          U64 idx = 0;
          B32 found = 0;
          for(E2_InterpStackValNode *n = state->top_val; n != 0; n = n->next, idx += 1)
          {
            if(idx == target_idx)
            {
              found = 1;
              push_vals[0] = n->val;
              break;
            }
          }
          if(!found)
          {
            good = 0;
            interp.status = E2_InterpStatus_InsufficientStackSpace;
          }
        }break;
        
        case RDI_EvalOp_Swap:
        {
          push_vals[0] = popped_vals[1];
          push_vals[1] = popped_vals[0];
        }break;
        
        case RDI_EvalOp_Pop:{NoOp;}break;
        
        case RDI_EvalOp_ValueRead:
        {
          U64 bytes_to_read = decode_val.u64;
          U64 offset = popped_vals[0].u64;
          if(offset + bytes_to_read <= popped_vals[1].string.size)
          {
            MemoryCopy(&push_vals[0].u512, popped_vals[1].string.str + offset, bytes_to_read);
          }
          else
          {
            good = 0;
            interp.status = E2_InterpStatus_BadOffset;
          }
        }break;
        
        case RDI_EvalOp_ByteSwap:
        {
          U64 byte_size = decode_val.u64;
          switch(byte_size)
          {
            default:
            {
              good = 0;
              interp.status = E2_InterpStatus_UnsupportedOp;
            }break;
            case 2:{push_vals[0].u16 = bswap_u16(popped_vals[0].u16);}break;
            case 4:{push_vals[0].u32 = bswap_u32(popped_vals[0].u32);}break;
            case 8:{push_vals[0].u64 = bswap_u64(popped_vals[0].u64);}break;
          }
        }break;
        
        case RDI_EvalOp_CallSiteValue:
        {
          // DW_OP_entry_value: requires evaluating the embedded sub-bytecode
          // in this frame's entry-time register state (i.e. caller's state
          // at the call instruction). that state is not currently plumbed
          // into the eval context. interpreting in the current ctx gives
          // wrong values for spilled args, so report BadOp instead.
          // skip past the embedded sub-bytecode in the outer stream so the
          // bytes are not interpreted as outer ops on resume.
          good = 0;
          interp.status = E2_InterpStatus_UnsupportedOp;
        }break;
        
        case RDI_EvalOp_PartialValue:
        {
          // DW_OP_piece marker: top-of-stack is a piece of a composite value.
          // we do not assemble composites; for single-piece expressions the
          // value already on the stack is the result. for multi-piece, only
          // the first piece is returned (stack[0] is the final result).
          good = 0;
          interp.status = E2_InterpStatus_UnsupportedOp;
        }break;
        
        case RDI_EvalOp_PartialValueBit:
        {
          // DW_OP_bit_piece marker. same caveat as PartialValue.
          good = 0;
          interp.status = E2_InterpStatus_UnsupportedOp;
        }break;
      }
      
      //- rjf: if opcode successful -> do stack pops
      if(good) for EachIndex(pop_idx, pop_count)
      {
        E2_InterpStackValNode *popped = state->top_val;
        if(popped != 0)
        {
          SLLStackPop(state->top_val);
          SLLStackPush(state->free_val, popped);
        }
        else
        {
          break;
        }
      }
      
      //- rjf: if opcode successful -> do stack pushes
      if(good) for EachIndex(push_idx, push_count)
      {
        E2_InterpStackValNode *node = state->free_val;
        if(node != 0)
        {
          SLLStackPop(state->free_val);
        }
        else
        {
          node = push_array(arena, E2_InterpStackValNode, 1);
        }
        node->val = push_vals[push_idx];
        SLLStackPush(state->top_val, node);
      }
      
      //- rjf: if opcode successful -> commit new bytecode offset
      if(good)
      {
        state->bytecode_off = off;
      }
      
      scratch_end(scratch);
      if(off == start_off)
      {
        break;
      }
    }
    
    //- rjf: asking caller for info? -> if we already did this at this offset,
    // the user couldn't provide the info, so we just fail out.
    if(e2_interp_status_is_caller_request(interp.status))
    {
      if(state->caller_request_count > 0 && state->last_caller_request_bytecode_off == state->bytecode_off)
      {
        switch(interp.status)
        {
          default:{}break;
          case E2_InterpStatus_MissedSpaceRead:
          {
            e2_msgf(arena, &interp.msgs, r1u64(state->bytecode_off, state->bytecode_off+1), "Couldn't read address range [0x%I64x, 0x%I64x) in space 0x%I64x.",
                    interp.missed_read_space_addr_range.min,
                    interp.missed_read_space_addr_range.max,
                    interp.space_id);
          }break;
        }
        interp.status = E2_InterpStatus_Error;
      }
      state->caller_request_count += 1;
      state->last_caller_request_bytecode_off = state->bytecode_off;
    }
    
    //- rjf: if we're done with the bytecode stream, report success / value
    if(off == off_opl && good)
    {
      interp.status = E2_InterpStatus_Good;
      if(state->top_val != 0)
      {
        interp.val = state->top_val->val;
      }
    }
  }
  return interp;
}
