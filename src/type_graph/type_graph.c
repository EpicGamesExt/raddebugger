// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
tg_hash_from_string(U64 seed, String8 string)
{
  U64 result = seed;
  for(U64 idx = 0; idx < string.size; idx += 1)
  {
    result = ((result<<5)+result) + string.str[idx];
  }
  return result;
}

internal int
tg_qsort_compare_members_offset(TG_Member *a, TG_Member *b)
{
  int result = 0;
  if(a->off < b->off)
  {
    result = -1;
  }
  else if(a->off > b->off)
  {
    result = +1;
  }
  return result;
}

internal void
tg_key_list_push(Arena *arena, TG_KeyList *list, TG_Key key)
{
  TG_KeyNode *n = push_array(arena, TG_KeyNode, 1);
  n->v = key;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal TG_KeyList
tg_key_list_copy(Arena *arena, TG_KeyList *src)
{
  TG_KeyList dst = {0};
  for(TG_KeyNode *n = src->first; n != 0; n = n->next)
  {
    tg_key_list_push(arena, &dst, n->v);
  }
  return dst;
}

////////////////////////////////
//~ rjf: RADDBG <-> TG Enum Conversions

internal TG_Kind
tg_kind_from_raddbg_type_kind(RADDBG_TypeKind kind)
{
  TG_Kind result = TG_Kind_Null;
  switch(kind)
  {
    default:{}break;
    case RADDBG_TypeKind_Void:                   {result = TG_Kind_Void;}break;
    case RADDBG_TypeKind_Handle:                 {result = TG_Kind_Handle;}break;
    case RADDBG_TypeKind_Char8:                  {result = TG_Kind_Char8;}break;
    case RADDBG_TypeKind_Char16:                 {result = TG_Kind_Char16;}break;
    case RADDBG_TypeKind_Char32:                 {result = TG_Kind_Char32;}break;
    case RADDBG_TypeKind_UChar8:                 {result = TG_Kind_UChar8;}break;
    case RADDBG_TypeKind_UChar16:                {result = TG_Kind_UChar16;}break;
    case RADDBG_TypeKind_UChar32:                {result = TG_Kind_UChar32;}break;
    case RADDBG_TypeKind_U8:                     {result = TG_Kind_U8;}break;
    case RADDBG_TypeKind_U16:                    {result = TG_Kind_U16;}break;
    case RADDBG_TypeKind_U32:                    {result = TG_Kind_U32;}break;
    case RADDBG_TypeKind_U64:                    {result = TG_Kind_U64;}break;
    case RADDBG_TypeKind_U128:                   {result = TG_Kind_U128;}break;
    case RADDBG_TypeKind_U256:                   {result = TG_Kind_U256;}break;
    case RADDBG_TypeKind_U512:                   {result = TG_Kind_U512;}break;
    case RADDBG_TypeKind_S8:                     {result = TG_Kind_S8;}break;
    case RADDBG_TypeKind_S16:                    {result = TG_Kind_S16;}break;
    case RADDBG_TypeKind_S32:                    {result = TG_Kind_S32;}break;
    case RADDBG_TypeKind_S64:                    {result = TG_Kind_S64;}break;
    case RADDBG_TypeKind_S128:                   {result = TG_Kind_S128;}break;
    case RADDBG_TypeKind_S256:                   {result = TG_Kind_S256;}break;
    case RADDBG_TypeKind_S512:                   {result = TG_Kind_S512;}break;
    case RADDBG_TypeKind_Bool:                   {result = TG_Kind_Bool;}break;
    case RADDBG_TypeKind_F16:                    {result = TG_Kind_F16;}break;
    case RADDBG_TypeKind_F32:                    {result = TG_Kind_F32;}break;
    case RADDBG_TypeKind_F32PP:                  {result = TG_Kind_F32PP;}break;
    case RADDBG_TypeKind_F48:                    {result = TG_Kind_F48;}break;
    case RADDBG_TypeKind_F64:                    {result = TG_Kind_F64;}break;
    case RADDBG_TypeKind_F80:                    {result = TG_Kind_F80;}break;
    case RADDBG_TypeKind_F128:                   {result = TG_Kind_F128;}break;
    case RADDBG_TypeKind_ComplexF32:             {result = TG_Kind_ComplexF32;}break;
    case RADDBG_TypeKind_ComplexF64:             {result = TG_Kind_ComplexF64;}break;
    case RADDBG_TypeKind_ComplexF80:             {result = TG_Kind_ComplexF80;}break;
    case RADDBG_TypeKind_ComplexF128:            {result = TG_Kind_ComplexF128;}break;
    case RADDBG_TypeKind_Modifier:               {result = TG_Kind_Modifier;}break;
    case RADDBG_TypeKind_Ptr:                    {result = TG_Kind_Ptr;}break;
    case RADDBG_TypeKind_LRef:                   {result = TG_Kind_LRef;}break;
    case RADDBG_TypeKind_RRef:                   {result = TG_Kind_RRef;}break;
    case RADDBG_TypeKind_Array:                  {result = TG_Kind_Array;}break;
    case RADDBG_TypeKind_Function:               {result = TG_Kind_Function;}break;
    case RADDBG_TypeKind_Method:                 {result = TG_Kind_Method;}break;
    case RADDBG_TypeKind_MemberPtr:              {result = TG_Kind_MemberPtr;}break;
    case RADDBG_TypeKind_Struct:                 {result = TG_Kind_Struct;}break;
    case RADDBG_TypeKind_Class:                  {result = TG_Kind_Class;}break;
    case RADDBG_TypeKind_Union:                  {result = TG_Kind_Union;}break;
    case RADDBG_TypeKind_Enum:                   {result = TG_Kind_Enum;}break;
    case RADDBG_TypeKind_Alias:                  {result = TG_Kind_Alias;}break;
    case RADDBG_TypeKind_IncompleteStruct:       {result = TG_Kind_IncompleteStruct;}break;
    case RADDBG_TypeKind_IncompleteUnion:        {result = TG_Kind_IncompleteUnion;}break;
    case RADDBG_TypeKind_IncompleteClass:        {result = TG_Kind_IncompleteClass;}break;
    case RADDBG_TypeKind_IncompleteEnum:         {result = TG_Kind_IncompleteEnum;}break;
    case RADDBG_TypeKind_Bitfield:               {result = TG_Kind_Bitfield;}break;
    case RADDBG_TypeKind_Variadic:               {result = TG_Kind_Variadic;}break;
  }
  return result;
}

internal TG_MemberKind
tg_member_kind_from_raddbg_member_kind(RADDBG_MemberKind kind)
{
  TG_MemberKind result = TG_MemberKind_Null;
  switch(kind)
  {
    default:{}break;
    case RADDBG_MemberKind_DataField:            {result = TG_MemberKind_DataField;}break;
    case RADDBG_MemberKind_StaticData:           {result = TG_MemberKind_StaticData;}break;
    case RADDBG_MemberKind_Method:               {result = TG_MemberKind_Method;}break;
    case RADDBG_MemberKind_StaticMethod:         {result = TG_MemberKind_StaticMethod;}break;
    case RADDBG_MemberKind_VirtualMethod:        {result = TG_MemberKind_VirtualMethod;}break;
    case RADDBG_MemberKind_VTablePtr:            {result = TG_MemberKind_VTablePtr;}break;
    case RADDBG_MemberKind_Base:                 {result = TG_MemberKind_Base;}break;
    case RADDBG_MemberKind_VirtualBase:          {result = TG_MemberKind_VirtualBase;}break;
    case RADDBG_MemberKind_NestedType:           {result = TG_MemberKind_NestedType;}break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Key Type Functions

internal TG_Key
tg_key_zero(void)
{
  TG_Key key = zero_struct;
  return key;
}

internal TG_Key
tg_key_basic(TG_Kind kind)
{
  TG_Key key = {TG_KeyKind_Basic};
  key.u32[0] = (U32)kind;
  return key;
}

internal TG_Key
tg_key_ext(TG_Kind kind, U64 id)
{
  TG_Key key = {TG_KeyKind_Ext};
  key.u32[0] = (U32)kind;
  if(TG_Kind_FirstBasic <= kind && kind <= TG_Kind_LastBasic)
  {
    key.kind = TG_KeyKind_Basic;
  }
  else
  {
    key.u64[0] = id;
  }
  return key;
}

internal TG_Key
tg_key_reg(Architecture arch, REGS_RegCode code)
{
  TG_Key key = {TG_KeyKind_Reg};
  key.u32[0] = (U32)arch;
  key.u64[0] = (U64)code;
  return key;
}

internal TG_Key
tg_key_reg_alias(Architecture arch, REGS_AliasCode code)
{
  TG_Key key = {TG_KeyKind_RegAlias};
  key.u32[0] = (U32)arch;
  key.u64[0] = (U64)code;
  return key;
}

internal B32
tg_key_match(TG_Key a, TG_Key b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

////////////////////////////////
//~ rjf: Graph Construction API

internal TG_Graph *
tg_graph_begin(U64 address_size, U64 slot_count)
{
  if(tg_build_arena == 0)
  {
    tg_build_arena = arena_alloc();
  }
  else
  {
    arena_clear(tg_build_arena);
  }
  TG_Graph *graph = push_array(tg_build_arena, TG_Graph, 1);
  graph->address_size = address_size;
  graph->content_hash_slots_count = slot_count;
  graph->content_hash_slots = push_array(tg_build_arena, TG_Slot, graph->content_hash_slots_count);
  graph->key_hash_slots_count = slot_count;
  graph->key_hash_slots = push_array(tg_build_arena, TG_Slot, graph->key_hash_slots_count);
  return graph;
}

internal TG_Key
tg_cons_type_make(TG_Graph *graph, TG_Kind kind, TG_Key direct_type_key, U64 u64)
{
  U32 buffer[] =
  {
    (U32)kind,
    (U32)direct_type_key.kind,
    (U32)direct_type_key.u32[0],
    (U32)((direct_type_key.u64[0] & 0x00000000ffffffffull)>> 0),
    (U32)((direct_type_key.u64[0] & 0xffffffff00000000ull)>>32),
    (U32)((u64 & 0x00000000ffffffffull)>> 0),
    (U32)((u64 & 0xffffffff00000000ull)>> 32),
  };
  U64 content_hash = tg_hash_from_string(5381, str8((U8 *)buffer, sizeof(buffer)));
  U64 content_slot_idx = content_hash%graph->content_hash_slots_count;
  TG_Slot *content_slot = &graph->content_hash_slots[content_slot_idx];
  TG_Node *node = 0;
  for(TG_Node *n = content_slot->first; n != 0; n = n->content_hash_next)
  {
    if(n->cons_type.kind == kind && tg_key_match(n->cons_type.direct_type_key, direct_type_key))
    {
      node = n;
      break;
    }
  }
  TG_Key result = zero_struct;
  if(node == 0)
  {
    TG_Key key = {TG_KeyKind_Cons};
    key.u32[0] = (U32)kind;
    key.u64[0] = graph->cons_id_gen;
    U64 key_hash = tg_hash_from_string(5381, str8_struct(&key));
    U64 key_slot_idx = key_hash%graph->key_hash_slots_count;
    TG_Slot *key_slot = &graph->key_hash_slots[key_slot_idx];
    graph->cons_id_gen += 1;
    TG_Node *node = push_array(tg_build_arena, TG_Node, 1);
    SLLQueuePush_N(content_slot->first, content_slot->last, node, content_hash_next);
    SLLQueuePush_N(key_slot->first, key_slot->last, node, key_hash_next);
    node->key = key;
    node->cons_type.kind = kind;
    node->cons_type.direct_type_key = direct_type_key;
    node->cons_type.u64 = u64;
    result = key;
  }
  else
  {
    result = node->key;
  }
  return result;
}

////////////////////////////////
//~ rjf: Graph Introspection API

internal TG_Type *
tg_type_from_graph_raddbg_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  TG_Type *type = &tg_type_nil;
  U64 reg_byte_count = 0;
  {
    switch(key.kind)
    {
      default:{}break;
      
      //- rjf: basic type keys
      case TG_KeyKind_Basic:
      {
        TG_Kind kind = (TG_Kind)key.u32[0];
        if(TG_Kind_FirstBasic <= kind && kind <= TG_Kind_LastBasic)
        {
          type = push_array(arena, TG_Type, 1);
          type->kind       = kind;
          type->name       = tg_kind_basic_string_table[kind];
          type->byte_size  = tg_kind_basic_byte_size_table[kind];
        }
      }break;
      
      //- rjf: constructed type keys
      case TG_KeyKind_Cons:
      {
        U64 key_hash = tg_hash_from_string(5381, str8_struct(&key));
        U64 key_slot_idx = key_hash%graph->key_hash_slots_count;
        TG_Slot *key_slot = &graph->key_hash_slots[key_slot_idx];
        for(TG_Node *node = key_slot->first; node != 0; node = node->key_hash_next)
        {
          if(tg_key_match(node->key, key))
          {
            TG_ConsType *cons_type = &node->cons_type;
            type = push_array(arena, TG_Type, 1);
            type->kind             = cons_type->kind;
            type->direct_type_key  = cons_type->direct_type_key;
            type->count            = cons_type->u64;
            switch(type->kind)
            {
              default:
              {
                type->byte_size = graph->address_size;
              }break;
              case TG_Kind_Array:
              {
                U64 ptee_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, cons_type->direct_type_key);
                type->byte_size = ptee_size * type->count;
              }break;
            }
          }
        }
      }break;
      
      //- rjf: external (raddbg) type keys
      case TG_KeyKind_Ext:
      {
        U64 type_node_idx = key.u64[0];
        if(0 <= type_node_idx && type_node_idx < rdbg->type_nodes_count)
        {
          RADDBG_TypeNode *rdbg_type = &rdbg->type_nodes[type_node_idx];
          TG_Kind kind = tg_kind_from_raddbg_type_kind(rdbg_type->kind);
          
          //- rjf: record types => unpack name * members & produce
          if(RADDBG_TypeKind_FirstRecord <= rdbg_type->kind && rdbg_type->kind <= RADDBG_TypeKind_LastRecord)
          {
            // rjf: unpack name
            String8 name = {0};
            name.str = raddbg_string_from_idx(rdbg, rdbg_type->user_defined.name_string_idx, &name.size);
            
            // rjf: unpack members
            TG_Member *members = 0;
            U32 members_count = 0;
            {
              U32 udt_idx = rdbg_type->user_defined.udt_idx;
              if(0 <= udt_idx && udt_idx < rdbg->udts_count)
              {
                RADDBG_UDT *udt = &rdbg->udts[udt_idx];
                members_count = udt->member_count;
                members = push_array(arena, TG_Member, members_count);
                if(members_count != 0 && 0 <= udt->member_first && udt->member_first+udt->member_count <= rdbg->members_count)
                {
                  for(U32 member_idx = udt->member_first;
                      member_idx < udt->member_first+udt->member_count;
                      member_idx += 1)
                  {
                    RADDBG_Member *src = &rdbg->members[member_idx];
                    TG_Kind member_type_kind = TG_Kind_Null;
                    if(src->type_idx < rdbg->type_nodes_count)
                    {
                      RADDBG_TypeNode *member_type = &rdbg->type_nodes[src->type_idx];
                      member_type_kind = tg_kind_from_raddbg_type_kind(member_type->kind);
                    }
                    TG_Member *dst = &members[member_idx-udt->member_first];
                    dst->kind     = tg_member_kind_from_raddbg_member_kind(src->kind);
                    dst->type_key = tg_key_ext(member_type_kind, (U64)src->type_idx);
                    dst->name.str = raddbg_string_from_idx(rdbg, src->name_string_idx, &dst->name.size);
                    dst->off      = (U64)src->off;
                  }
                }
              }
            }
            
            // rjf: produce
            type = push_array(arena, TG_Type, 1);
            type->kind       = kind;
            type->name       = push_str8_copy(arena, name);
            type->byte_size  = (U64)rdbg_type->byte_size;
            type->count      = members_count;
            type->members    = members;
          }
          
          //- rjf: enum types => unpack name * values & produce
          else if(rdbg_type->kind == RADDBG_TypeKind_Enum)
          {
            // rjf: unpack name
            String8 name = {0};
            name.str = raddbg_string_from_idx(rdbg, rdbg_type->user_defined.name_string_idx, &name.size);
            
            // rjf: unpack direct type
            TG_Key direct_type_key = zero_struct;
            if(rdbg_type->user_defined.direct_type_idx < type_node_idx)
            {
              RADDBG_TypeNode *direct_type_node = &rdbg->type_nodes[rdbg_type->user_defined.direct_type_idx];
              TG_Kind direct_type_kind = tg_kind_from_raddbg_type_kind(direct_type_node->kind);
              direct_type_key = tg_key_ext(direct_type_kind, (U64)rdbg_type->user_defined.direct_type_idx);
            }
            
            // rjf: unpack members
            TG_EnumVal *enum_vals = 0;
            U32 enum_vals_count = 0;
            {
              U32 udt_idx = rdbg_type->user_defined.udt_idx;
              if(0 <= udt_idx && udt_idx < rdbg->udts_count)
              {
                RADDBG_UDT *udt = &rdbg->udts[udt_idx];
                enum_vals_count = udt->member_count;
                enum_vals = push_array(arena, TG_EnumVal, enum_vals_count);
                if(0 <= udt->member_first && udt->member_first+udt->member_count < rdbg->enum_members_count)
                {
                  for(U32 member_idx = udt->member_first;
                      member_idx < udt->member_first+udt->member_count;
                      member_idx += 1)
                  {
                    RADDBG_EnumMember *src = &rdbg->enum_members[member_idx];
                    TG_EnumVal *dst = &enum_vals[member_idx-udt->member_first];
                    dst->name.str = raddbg_string_from_idx(rdbg, src->name_string_idx, &dst->name.size);
                    dst->val      = src->val;
                  }
                }
              }
            }
            
            // rjf: produce
            type = push_array(arena, TG_Type, 1);
            type->kind            = kind;
            type->name            = push_str8_copy(arena, name);
            type->byte_size       = (U64)rdbg_type->byte_size;
            type->count           = enum_vals_count;
            type->enum_vals       = enum_vals;
            type->direct_type_key = direct_type_key;
          }
          
          //- rjf: constructed types
          if(RADDBG_TypeKind_FirstConstructed <= rdbg_type->kind && rdbg_type->kind <= RADDBG_TypeKind_LastConstructed)
          {
            // rjf: unpack direct type
            B32 direct_type_is_good = 0;
            TG_Key direct_type_key = zero_struct;
            U64 direct_type_byte_size = 0;
            if(rdbg_type->constructed.direct_type_idx < type_node_idx)
            {
              RADDBG_TypeNode *direct_type_node = &rdbg->type_nodes[rdbg_type->constructed.direct_type_idx];
              TG_Kind direct_type_kind = tg_kind_from_raddbg_type_kind(direct_type_node->kind);
              direct_type_key = tg_key_ext(direct_type_kind, (U64)rdbg_type->constructed.direct_type_idx);
              direct_type_is_good = 1;
              direct_type_byte_size = (U64)direct_type_node->byte_size;
            }
            
            // rjf: construct based on kind
            switch(rdbg_type->kind)
            {
              case RADDBG_TypeKind_Modifier:
              {
                TG_Flags flags = 0;
                if(rdbg_type->flags & RADDBG_TypeModifierFlag_Const)
                {
                  flags |= TG_Flag_Const;
                }
                if(rdbg_type->flags & RADDBG_TypeModifierFlag_Volatile)
                {
                  flags |= TG_Flag_Volatile;
                }
                type = push_array(arena, TG_Type, 1);
                type->kind            = kind;
                type->direct_type_key = direct_type_key;
                type->byte_size       = direct_type_byte_size;
                type->flags           = flags;
              }break;
              case RADDBG_TypeKind_Ptr:
              case RADDBG_TypeKind_LRef:
              case RADDBG_TypeKind_RRef:
              {
                type = push_array(arena, TG_Type, 1);
                type->kind            = kind;
                type->direct_type_key = direct_type_key;
                type->byte_size       = graph->address_size;
              }break;
              
              case RADDBG_TypeKind_Array:
              {
                type = push_array(arena, TG_Type, 1);
                type->kind            = kind;
                type->direct_type_key = direct_type_key;
                type->count           = rdbg_type->constructed.count;
                type->byte_size       = direct_type_byte_size * type->count;
              }break;
              case RADDBG_TypeKind_Function:
              {
                U32 count = rdbg_type->constructed.count;
                U32 idx_run_first = rdbg_type->constructed.param_idx_run_first;
                U32 check_count = 0;
                U32 *idx_run = raddbg_idx_run_from_first_count(rdbg, idx_run_first, count, &check_count);
                if(check_count == count)
                {
                  type = push_array(arena, TG_Type, 1);
                  type->kind            = kind;
                  type->byte_size       = graph->address_size;
                  type->direct_type_key = direct_type_key;
                  type->count           = count;
                  type->param_type_keys = push_array_no_zero(arena, TG_Key, type->count);
                  for(U32 idx = 0; idx < type->count; idx += 1)
                  {
                    U32 param_type_idx = idx_run[idx];
                    if(param_type_idx < type_node_idx)
                    {
                      RADDBG_TypeNode *param_type_node = &rdbg->type_nodes[param_type_idx];
                      TG_Kind param_kind = tg_kind_from_raddbg_type_kind(param_type_node->kind);
                      type->param_type_keys[idx] = tg_key_ext(param_kind, (U64)param_type_idx);
                    }
                    else
                    {
                      break;
                    }
                  }
                }
              }break;
              case RADDBG_TypeKind_Method:
              {
                // NOTE(rjf): for methods, the `direct` type points at the owner type.
                // the return type, instead of being encoded via the `direct` type, is
                // encoded via the first parameter.
                U32 count = rdbg_type->constructed.count;
                U32 idx_run_first = rdbg_type->constructed.param_idx_run_first;
                U32 check_count = 0;
                U32 *idx_run = raddbg_idx_run_from_first_count(rdbg, idx_run_first, count, &check_count);
                if(check_count == count)
                {
                  type = push_array(arena, TG_Type, 1);
                  type->kind            = kind;
                  type->byte_size       = graph->address_size;
                  type->owner_type_key  = direct_type_key;
                  type->count           = count;
                  type->param_type_keys = push_array_no_zero(arena, TG_Key, type->count);
                  for(U32 idx = 0; idx < type->count; idx += 1)
                  {
                    U32 param_type_idx = idx_run[idx];
                    if(param_type_idx < type_node_idx)
                    {
                      RADDBG_TypeNode *param_type_node = &rdbg->type_nodes[param_type_idx];
                      TG_Kind param_kind = tg_kind_from_raddbg_type_kind(param_type_node->kind);
                      type->param_type_keys[idx] = tg_key_ext(param_kind, (U64)param_type_idx);
                    }
                    else
                    {
                      break;
                    }
                  }
                  if(type->count > 0)
                  {
                    type->direct_type_key = type->param_type_keys[0];
                    type->count -= 1;
                    type->param_type_keys += 1;
                  }
                }
              }break;
              case RADDBG_TypeKind_MemberPtr:
              {
                // rjf: unpack owner type
                TG_Key owner_type_key = zero_struct;
                if(rdbg_type->constructed.owner_type_idx < type_node_idx)
                {
                  RADDBG_TypeNode *owner_type_node = &rdbg->type_nodes[rdbg_type->constructed.owner_type_idx];
                  TG_Kind owner_type_kind = tg_kind_from_raddbg_type_kind(owner_type_node->kind);
                  owner_type_key = tg_key_ext(owner_type_kind, (U64)rdbg_type->constructed.owner_type_idx);
                }
                type = push_array(arena, TG_Type, 1);
                type->kind            = kind;
                type->byte_size       = graph->address_size;
                type->owner_type_key  = owner_type_key;
                type->direct_type_key = direct_type_key;
              }break;
            }
          }
          
          //- rjf: alias types
          else if(rdbg_type->kind == RADDBG_TypeKind_Alias)
          {
            // rjf: unpack name
            String8 name = {0};
            name.str = raddbg_string_from_idx(rdbg, rdbg_type->user_defined.name_string_idx, &name.size);
            
            // rjf: unpack direct type
            TG_Key direct_type_key = zero_struct;
            U64 direct_type_byte_size = 0;
            if(rdbg_type->user_defined.direct_type_idx < type_node_idx)
            {
              RADDBG_TypeNode *direct_type_node = &rdbg->type_nodes[rdbg_type->user_defined.direct_type_idx];
              TG_Kind direct_type_kind = tg_kind_from_raddbg_type_kind(direct_type_node->kind);
              direct_type_key = tg_key_ext(direct_type_kind, (U64)rdbg_type->user_defined.direct_type_idx);
              direct_type_byte_size = direct_type_node->byte_size;
            }
            
            // rjf: produce
            type = push_array(arena, TG_Type, 1);
            type->kind            = kind;
            type->name            = push_str8_copy(arena, name);
            type->byte_size       = direct_type_byte_size;
            type->direct_type_key = direct_type_key;
          }
          
          //- rjf: incomplete types
          else if(RADDBG_TypeKind_FirstIncomplete <= rdbg_type->kind && rdbg_type->kind <= RADDBG_TypeKind_LastIncomplete)
          {
            // rjf: unpack name
            String8 name = {0};
            name.str = raddbg_string_from_idx(rdbg, rdbg_type->user_defined.name_string_idx, &name.size);
            
            // rjf: produce
            type = push_array(arena, TG_Type, 1);
            type->kind            = kind;
            type->name            = push_str8_copy(arena, name);
          }
          
        }
      }break;
      
      //- rjf: reg type keys
      case TG_KeyKind_Reg:
      {
        Architecture arch = (Architecture)key.u32[0];
        REGS_RegCode code = (REGS_RegCode)key.u64[0];
        REGS_Rng rng = regs_reg_code_rng_table_from_architecture(arch)[code];
        reg_byte_count = (U64)rng.byte_size;
      }goto build_reg_type;
      case TG_KeyKind_RegAlias:
      {
        Architecture arch = (Architecture)key.u32[0];
        REGS_AliasCode code = (REGS_AliasCode)key.u64[0];
        REGS_Slice slice = regs_alias_code_slice_table_from_architecture(arch)[code];
        reg_byte_count = (U64)slice.byte_size;
      }goto build_reg_type;
      build_reg_type:
      {
        Temp scratch = scratch_begin(&arena, 1);
        type = push_array(arena, TG_Type, 1);
        type->kind       = TG_Kind_Union;
        type->name       = push_str8f(arena, "reg_%I64u_bit", reg_byte_count*8);
        type->byte_size  = (U64)reg_byte_count;
        
        // rjf: build register type members
        TG_MemberList members = {0};
        {
          // rjf: build exact-sized members
          {
            if(type->byte_size == 16)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("u128");
              mem->type_key = tg_key_basic(TG_Kind_U128);
            }
            if(type->byte_size == 8)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("u64");
              mem->type_key = tg_key_basic(TG_Kind_U64);
            }
            if(type->byte_size == 4)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("u32");
              mem->type_key = tg_key_basic(TG_Kind_U32);
            }
            if(type->byte_size == 2)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("u16");
              mem->type_key = tg_key_basic(TG_Kind_U16);
            }
            if(type->byte_size == 1)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("u8");
              mem->type_key = tg_key_basic(TG_Kind_U8);
            }
          }
          
          // rjf: build arrays for subdivisions
          {
            if(type->byte_size > 16 && type->byte_size%16 == 0)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("u128s");
              mem->type_key = tg_cons_type_make(graph, TG_Kind_Array, tg_key_basic(TG_Kind_U128), reg_byte_count/16);
            }
            if(type->byte_size > 8 && type->byte_size%8 == 0)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("u64s");
              mem->type_key = tg_cons_type_make(graph, TG_Kind_Array, tg_key_basic(TG_Kind_U64), reg_byte_count/8);
            }
            if(type->byte_size > 4 && type->byte_size%4 == 0)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("u32s");
              mem->type_key = tg_cons_type_make(graph, TG_Kind_Array, tg_key_basic(TG_Kind_U32), reg_byte_count/4);
            }
            if(type->byte_size > 2 && type->byte_size%2 == 0)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("u16s");
              mem->type_key = tg_cons_type_make(graph, TG_Kind_Array, tg_key_basic(TG_Kind_U16), reg_byte_count/2);
            }
            if(type->byte_size > 1)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("u8s");
              mem->type_key = tg_cons_type_make(graph, TG_Kind_Array, tg_key_basic(TG_Kind_U8), reg_byte_count);
            }
            if(type->byte_size > 4 && type->byte_size%4 == 0)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("f32s");
              mem->type_key = tg_cons_type_make(graph, TG_Kind_Array, tg_key_basic(TG_Kind_F32), reg_byte_count/4);
            }
            if(type->byte_size > 8 && type->byte_size%8 == 0)
            {
              TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              TG_Member *mem = &n->v;
              mem->kind = TG_MemberKind_DataField;
              mem->name = str8_lit("f64s");
              mem->type_key = tg_cons_type_make(graph, TG_Kind_Array, tg_key_basic(TG_Kind_F64), reg_byte_count/8);
            }
          }
        }
        
        // rjf: commit members
        type->count = members.count;
        type->members = push_array_no_zero(arena, TG_Member, members.count);
        U64 idx = 0;
        for(TG_MemberNode *n = members.first; n != 0; n = n->next, idx += 1)
        {
          MemoryCopyStruct(&type->members[idx], &n->v);
        }
        
        scratch_end(scratch);
      }break;
    }
  }
  return type;
}

internal TG_Key
tg_direct_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  TG_Key result = zero_struct;
  switch(key.kind)
  {
    default:{}break;
    case TG_KeyKind_Ext:
    case TG_KeyKind_Cons:
    {
      Temp scratch = scratch_begin(0, 0);
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
      result = type->direct_type_key;
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal TG_Key
tg_unwrapped_direct_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  key = tg_unwrapped_from_graph_raddbg_key(graph, rdbg, key);
  key = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
  key = tg_unwrapped_from_graph_raddbg_key(graph, rdbg, key);
  return key;
}

internal TG_Key
tg_owner_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  TG_Key result = zero_struct;
  switch(key.kind)
  {
    default:{}break;
    case TG_KeyKind_Ext:
    case TG_KeyKind_Cons:
    {
      Temp scratch = scratch_begin(0, 0);
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
      result = type->owner_type_key;
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal TG_Key
tg_ptee_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  TG_Key result = key;
  B32 passed_ptr = 0;
  for(;;)
  {
    TG_Kind kind = tg_kind_from_key(result);
    result = tg_direct_from_graph_raddbg_key(graph, rdbg, result);
    if(kind == TG_Kind_Ptr || kind == TG_Kind_LRef || kind == TG_Kind_RRef)
    {
      passed_ptr = 1;
    }
    TG_Kind next_kind = tg_kind_from_key(result);
    if(passed_ptr &&
       next_kind != TG_Kind_IncompleteStruct &&
       next_kind != TG_Kind_IncompleteUnion &&
       next_kind != TG_Kind_IncompleteEnum &&
       next_kind != TG_Kind_IncompleteClass &&
       next_kind != TG_Kind_Alias &&
       next_kind != TG_Kind_Modifier)
    {
      break;
    }
    if(kind == TG_Kind_Null)
    {
      break;
    }
  }
  return result;
}

internal TG_Key
tg_unwrapped_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  TG_Key result = key;
  for(B32 good = 1; good;)
  {
    TG_Kind kind = tg_kind_from_key(result);
    if((TG_Kind_FirstIncomplete <= kind && kind <= TG_Kind_LastIncomplete) ||
       kind == TG_Kind_Modifier ||
       kind == TG_Kind_Alias)
    {
      result = tg_direct_from_graph_raddbg_key(graph, rdbg, result);
    }
    else
    {
      good = 0;
    }
  }
  return result;
}

internal U64
tg_byte_size_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  U64 result = 0;
  switch(key.kind)
  {
    default:{}break;
    case TG_KeyKind_Basic:
    {
      TG_Kind kind = (TG_Kind)key.u32[0];
      result = tg_kind_basic_byte_size_table[kind];
    }break;
    case TG_KeyKind_Ext:
    case TG_KeyKind_Cons:
    {
      Temp scratch = scratch_begin(0, 0);
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
      result = type->byte_size;
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal TG_Kind
tg_kind_from_key(TG_Key key)
{
  TG_Kind kind = TG_Kind_Null;
  switch(key.kind)
  {
    default:{}break;
    case TG_KeyKind_Basic:   {kind = (TG_Kind)key.u32[0];}break;
    case TG_KeyKind_Ext:     {kind = (TG_Kind)key.u32[0];}break;
    case TG_KeyKind_Cons:    {kind = (TG_Kind)key.u32[0];}break;
    case TG_KeyKind_Reg:     {kind = TG_Kind_Union;}break;
    case TG_KeyKind_RegAlias:{kind = TG_Kind_Union;}break;
  }
  return kind;
}

internal TG_MemberArray
tg_members_from_graph_raddbg_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  TG_MemberArray result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
    if(type->members != 0)
    {
      result.count = type->count;
      result.v = push_array_no_zero(arena, TG_Member, result.count);
      MemoryCopy(result.v, type->members, sizeof(TG_Member)*result.count);
      for(U64 idx = 0; idx < result.count; idx += 1)
      {
        result.v[idx].name = push_str8_copy(arena, result.v[idx].name);
      }
    }
  }
  scratch_end(scratch);
  return result;
}

internal TG_MemberArray
tg_data_members_from_graph_raddbg_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  Temp scratch = scratch_begin(&arena, 1);
  TG_MemberList members_list = {0};
  B32 members_need_offset_sort = 0;
  {
    TG_Type *root_type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      U64 base_off;
      TG_KeyList inheritance_chain;
      TG_Key type_key;
      TG_Type *type;
    };
    Task start_task = {0, 0, {0}, key, root_type};
    Task *first_task = &start_task;
    Task *last_task = &start_task;
    for(Task *task = first_task; task != 0; task = task->next)
    {
      TG_Type *type = task->type;
      if(type->members != 0)
      {
        for(U64 member_idx = 0; member_idx < type->count; member_idx += 1)
        {
          if(type->members[member_idx].kind == TG_MemberKind_DataField)
          {
            TG_MemberNode *n = push_array(scratch.arena, TG_MemberNode, 1);
            MemoryCopyStruct(&n->v, &type->members[member_idx]);
            n->v.off += task->base_off;
            n->v.inheritance_key_chain = task->inheritance_chain;
            SLLQueuePush(members_list.first, members_list.last, n);
            members_list.count += 1;
          }
          else if(type->members[member_idx].kind == TG_MemberKind_Base)
          {
            Task *t = push_array(scratch.arena, Task, 1);
            t->base_off = type->members[member_idx].off + task->base_off;
            t->inheritance_chain = tg_key_list_copy(scratch.arena, &task->inheritance_chain);
            tg_key_list_push(scratch.arena, &t->inheritance_chain, type->members[member_idx].type_key);
            t->type_key = type->members[member_idx].type_key;
            t->type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, type->members[member_idx].type_key);
            SLLQueuePush(first_task, last_task, t);
            members_need_offset_sort = 1;
          }
        }
      }
    }
  }
  TG_MemberArray members = {0};
  {
    members.count = members_list.count;
    members.v = push_array(arena, TG_Member, members.count);
    U64 idx = 0;
    for(TG_MemberNode *n = members_list.first; n != 0; n = n->next)
    {
      MemoryCopyStruct(&members.v[idx], &n->v);
      members.v[idx].name = push_str8_copy(arena, members.v[idx].name);
      members.v[idx].inheritance_key_chain = tg_key_list_copy(arena, &members.v[idx].inheritance_key_chain);
      idx += 1;
    }
  }
  if(members_need_offset_sort)
  {
    qsort(members.v, members.count, sizeof(TG_Member), (int (*)(const void *, const void *))tg_qsort_compare_members_offset);
  }
  scratch_end(scratch);
  return members;
}

internal void
tg_lhs_string_from_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key, String8List *out, U32 prec, B32 skip_return)
{
  String8 keyword = {0};
  TG_Kind kind = tg_kind_from_key(key);
  switch(kind)
  {
    default:
    {
      Temp scratch = scratch_begin(&arena, 1);
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
      str8_list_push(arena, out, push_str8_copy(arena, type->name));
      str8_list_push(arena, out, str8_lit(" "));
      scratch_end(scratch);
    }break;
    
    case TG_Kind_Bitfield:
    {
      TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
      tg_lhs_string_from_key(arena, graph, rdbg, direct, out, prec, skip_return);
    }break;
    
    case TG_Kind_Modifier:
    {
      Temp scratch = scratch_begin(&arena, 1);
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
      TG_Key direct = type->direct_type_key;
      tg_lhs_string_from_key(arena, graph, rdbg, direct, out, 1, skip_return);
      if(type->flags & TG_Flag_Const)
      {
        str8_list_push(arena, out, str8_lit("const "));
      }
      if(type->flags & TG_Flag_Volatile)
      {
        str8_list_push(arena, out, str8_lit("volatile "));
      }
      scratch_end(scratch);
    }break;
    
    case TG_Kind_Variadic:
    {
      str8_list_push(arena, out, str8_lit("..."));
    }break;
    
    case TG_Kind_Struct:
    case TG_Kind_Union:
    case TG_Kind_Enum:
    case TG_Kind_Class:
    case TG_Kind_Alias:
    {
      Temp scratch = scratch_begin(&arena, 1);
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
      str8_list_push(arena, out, push_str8_copy(arena, type->name));
      str8_list_push(arena, out, str8_lit(" "));
      scratch_end(scratch);
    }break;
    
    case TG_Kind_IncompleteStruct: keyword = str8_lit("struct"); goto fwd_udt;
    case TG_Kind_IncompleteUnion:  keyword = str8_lit("union"); goto fwd_udt;
    case TG_Kind_IncompleteEnum:   keyword = str8_lit("enum"); goto fwd_udt;
    case TG_Kind_IncompleteClass:  keyword = str8_lit("class"); goto fwd_udt;
    fwd_udt:;
    {
      Temp scratch = scratch_begin(&arena, 1);
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
      str8_list_push(arena, out, keyword);
      str8_list_push(arena, out, str8_lit(" "));
      str8_list_push(arena, out, push_str8_copy(arena, type->name));
      str8_list_push(arena, out, str8_lit(" "));
      scratch_end(scratch);
    }break;
    
    case TG_Kind_Array:
    {
      TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
      tg_lhs_string_from_key(arena, graph, rdbg, direct, out, 2, skip_return);
      if(prec == 1)
      {
        str8_list_push(arena, out, str8_lit("("));
      }
    }break;
    
    case TG_Kind_Function:
    {
      if(!skip_return)
      {
        TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
        tg_lhs_string_from_key(arena, graph, rdbg, direct, out, 2, 0);
      }
      if(prec == 1)
      {
        str8_list_push(arena, out, str8_lit("("));
      }
    }break;
    
    case TG_Kind_Ptr:
    {
      TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
      tg_lhs_string_from_key(arena, graph, rdbg, direct, out, 1, skip_return);
      str8_list_push(arena, out, str8_lit("*"));
    }break;
    
    case TG_Kind_LRef:
    {
      TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
      tg_lhs_string_from_key(arena, graph, rdbg, direct, out, 1, skip_return);
      str8_list_push(arena, out, str8_lit("&"));
    }break;
    
    case TG_Kind_RRef:
    {
      TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
      tg_lhs_string_from_key(arena, graph, rdbg, direct, out, 1, skip_return);
      str8_list_push(arena, out, str8_lit("&&"));
    }break;
    
    case TG_Kind_MemberPtr:
    {
      Temp scratch = scratch_begin(&arena, 1);
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
      TG_Key direct = type->direct_type_key;
      tg_lhs_string_from_key(arena, graph, rdbg, direct, out, 1, skip_return);
      TG_Type *container = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, type->owner_type_key);
      if(container->kind != TG_Kind_Null)
      {
        str8_list_push(arena, out, push_str8_copy(arena, container->name));
      }
      else
      {
        str8_list_push(arena, out, str8_lit("<unknown-class>"));
      }
      str8_list_push(arena, out, str8_lit("::*"));
      scratch_end(scratch);
    }break;
  }
}

internal void
tg_rhs_string_from_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key, String8List *out, U32 prec)
{
  TG_Kind kind = tg_kind_from_key(key);
  switch(kind)
  {
    default:{}break;
    
    case TG_Kind_Bitfield:
    {
      TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
      tg_rhs_string_from_key(arena, graph, rdbg, direct, out, prec);
    }break;
    
    case TG_Kind_Modifier:
    case TG_Kind_Ptr:
    case TG_Kind_LRef:
    case TG_Kind_RRef:
    case TG_Kind_MemberPtr:
    {
      TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
      tg_rhs_string_from_key(arena, graph, rdbg, direct, out, 1);
    }break;
    
    case TG_Kind_Array:
    {
      Temp scratch = scratch_begin(&arena, 1);
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
      if(prec == 1)
      {
        str8_list_push(arena, out, str8_lit(")"));
      }
      String8 count_str = str8_from_u64(arena, type->count, 10, 0, 0);
      str8_list_push(arena, out, str8_lit("["));
      str8_list_push(arena, out, count_str);
      str8_list_push(arena, out, str8_lit("]"));
      TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
      tg_rhs_string_from_key(arena, graph, rdbg, direct, out, 2);
      scratch_end(scratch);
    }break;
    
    case TG_Kind_Function:
    {
      Temp scratch = scratch_begin(&arena, 1);
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, key);
      if(prec == 1)
      {
        str8_list_push(arena, out, str8_lit(")"));
      }
      
      // parameters
      if(type->count == 0)
      {
        str8_list_push(arena, out, str8_lit("(void)"));
      }
      else
      {
        str8_list_push(arena, out, str8_lit("("));
        U64 param_count = type->count;
        TG_Key *param_type_keys = type->param_type_keys;
        for(U64 param_idx = 0; param_idx < param_count; param_idx += 1)
        {
          TG_Key param_type_key = param_type_keys[param_idx];
          String8 param_str = tg_string_from_key(arena, graph, rdbg, param_type_key);
          String8 param_str_trimmed = str8_skip_chop_whitespace(param_str);
          str8_list_push(arena, out, param_str_trimmed);
          if(param_idx+1 < param_count)
          {
            str8_list_push(arena, out, str8_lit(", "));
          }
        }
        str8_list_push(arena, out, str8_lit(")"));
      }
      TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, key);
      tg_rhs_string_from_key(arena, graph, rdbg, direct, out, 2);
      scratch_end(scratch);
    }break;
  }
}

internal String8
tg_string_from_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  tg_lhs_string_from_key(scratch.arena, graph, rdbg, key, &list, 0, 0);
  tg_rhs_string_from_key(scratch.arena, graph, rdbg, key, &list, 0);
  String8 result = str8_list_join(arena, &list, 0);
  scratch_end(scratch);
  return result;
}
