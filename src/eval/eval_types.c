// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Type Kind Enum Functions

internal E_TypeKind
e_type_kind_from_base(TypeKind kind)
{
  E_TypeKind result = E_TypeKind_Null;
  switch(kind)
  {
    default:{}break;
    case TypeKind_Void:  {result = E_TypeKind_Void;}break;
    case TypeKind_U8:    {result = E_TypeKind_U8;}break;
    case TypeKind_U16:   {result = E_TypeKind_U16;}break;
    case TypeKind_U32:   {result = E_TypeKind_U32;}break;
    case TypeKind_U64:   {result = E_TypeKind_U64;}break;
    case TypeKind_S8:    {result = E_TypeKind_S8;}break;
    case TypeKind_S16:   {result = E_TypeKind_S16;}break;
    case TypeKind_S32:   {result = E_TypeKind_S32;}break;
    case TypeKind_S64:   {result = E_TypeKind_S64;}break;
    case TypeKind_B8:    {result = E_TypeKind_S8;}break;
    case TypeKind_B16:   {result = E_TypeKind_S16;}break;
    case TypeKind_B32:   {result = E_TypeKind_S32;}break;
    case TypeKind_B64:   {result = E_TypeKind_S64;}break;
    case TypeKind_F32:   {result = E_TypeKind_F32;}break;
    case TypeKind_F64:   {result = E_TypeKind_F64;}break;
    case TypeKind_Ptr:   {result = E_TypeKind_Ptr;}break;
    case TypeKind_Array: {result = E_TypeKind_Array;}break;
    case TypeKind_Struct:{result = E_TypeKind_Struct;}break;
    case TypeKind_Union: {result = E_TypeKind_Union;}break;
    case TypeKind_Enum:  {result = E_TypeKind_Enum;}break;
  }
  return result;
}

internal E_TypeKind
e_type_kind_from_rdi(RDI_TypeKind kind)
{
  E_TypeKind result = E_TypeKind_Null;
  switch(kind)
  {
    default:{}break;
    case RDI_TypeKind_Void:                   {result = E_TypeKind_Void;}break;
    case RDI_TypeKind_Handle:                 {result = E_TypeKind_Handle;}break;
    case RDI_TypeKind_HResult:                {result = E_TypeKind_HResult;}break;
    case RDI_TypeKind_Char8:                  {result = E_TypeKind_Char8;}break;
    case RDI_TypeKind_Char16:                 {result = E_TypeKind_Char16;}break;
    case RDI_TypeKind_Char32:                 {result = E_TypeKind_Char32;}break;
    case RDI_TypeKind_UChar8:                 {result = E_TypeKind_UChar8;}break;
    case RDI_TypeKind_UChar16:                {result = E_TypeKind_UChar16;}break;
    case RDI_TypeKind_UChar32:                {result = E_TypeKind_UChar32;}break;
    case RDI_TypeKind_U8:                     {result = E_TypeKind_U8;}break;
    case RDI_TypeKind_U16:                    {result = E_TypeKind_U16;}break;
    case RDI_TypeKind_U32:                    {result = E_TypeKind_U32;}break;
    case RDI_TypeKind_U64:                    {result = E_TypeKind_U64;}break;
    case RDI_TypeKind_U128:                   {result = E_TypeKind_U128;}break;
    case RDI_TypeKind_U256:                   {result = E_TypeKind_U256;}break;
    case RDI_TypeKind_U512:                   {result = E_TypeKind_U512;}break;
    case RDI_TypeKind_S8:                     {result = E_TypeKind_S8;}break;
    case RDI_TypeKind_S16:                    {result = E_TypeKind_S16;}break;
    case RDI_TypeKind_S32:                    {result = E_TypeKind_S32;}break;
    case RDI_TypeKind_S64:                    {result = E_TypeKind_S64;}break;
    case RDI_TypeKind_S128:                   {result = E_TypeKind_S128;}break;
    case RDI_TypeKind_S256:                   {result = E_TypeKind_S256;}break;
    case RDI_TypeKind_S512:                   {result = E_TypeKind_S512;}break;
    case RDI_TypeKind_Bool:                   {result = E_TypeKind_Bool;}break;
    case RDI_TypeKind_F16:                    {result = E_TypeKind_F16;}break;
    case RDI_TypeKind_F32:                    {result = E_TypeKind_F32;}break;
    case RDI_TypeKind_F32PP:                  {result = E_TypeKind_F32PP;}break;
    case RDI_TypeKind_F48:                    {result = E_TypeKind_F48;}break;
    case RDI_TypeKind_F64:                    {result = E_TypeKind_F64;}break;
    case RDI_TypeKind_F80:                    {result = E_TypeKind_F80;}break;
    case RDI_TypeKind_F128:                   {result = E_TypeKind_F128;}break;
    case RDI_TypeKind_ComplexF32:             {result = E_TypeKind_ComplexF32;}break;
    case RDI_TypeKind_ComplexF64:             {result = E_TypeKind_ComplexF64;}break;
    case RDI_TypeKind_ComplexF80:             {result = E_TypeKind_ComplexF80;}break;
    case RDI_TypeKind_ComplexF128:            {result = E_TypeKind_ComplexF128;}break;
    case RDI_TypeKind_Modifier:               {result = E_TypeKind_Modifier;}break;
    case RDI_TypeKind_Ptr:                    {result = E_TypeKind_Ptr;}break;
    case RDI_TypeKind_LRef:                   {result = E_TypeKind_LRef;}break;
    case RDI_TypeKind_RRef:                   {result = E_TypeKind_RRef;}break;
    case RDI_TypeKind_Array:                  {result = E_TypeKind_Array;}break;
    case RDI_TypeKind_Function:               {result = E_TypeKind_Function;}break;
    case RDI_TypeKind_Method:                 {result = E_TypeKind_Method;}break;
    case RDI_TypeKind_MemberPtr:              {result = E_TypeKind_MemberPtr;}break;
    case RDI_TypeKind_Struct:                 {result = E_TypeKind_Struct;}break;
    case RDI_TypeKind_Class:                  {result = E_TypeKind_Class;}break;
    case RDI_TypeKind_Union:                  {result = E_TypeKind_Union;}break;
    case RDI_TypeKind_Enum:                   {result = E_TypeKind_Enum;}break;
    case RDI_TypeKind_Alias:                  {result = E_TypeKind_Alias;}break;
    case RDI_TypeKind_IncompleteStruct:       {result = E_TypeKind_IncompleteStruct;}break;
    case RDI_TypeKind_IncompleteUnion:        {result = E_TypeKind_IncompleteUnion;}break;
    case RDI_TypeKind_IncompleteClass:        {result = E_TypeKind_IncompleteClass;}break;
    case RDI_TypeKind_IncompleteEnum:         {result = E_TypeKind_IncompleteEnum;}break;
    case RDI_TypeKind_Bitfield:               {result = E_TypeKind_Bitfield;}break;
    case RDI_TypeKind_Variadic:               {result = E_TypeKind_Variadic;}break;
  }
  return result;
}

internal E_MemberKind
e_member_kind_from_rdi(RDI_MemberKind kind)
{
  E_MemberKind result = E_MemberKind_Null;
  switch(kind)
  {
    default:{}break;
    case RDI_MemberKind_DataField:            {result = E_MemberKind_DataField;}break;
    case RDI_MemberKind_StaticData:           {result = E_MemberKind_StaticData;}break;
    case RDI_MemberKind_Method:               {result = E_MemberKind_Method;}break;
    case RDI_MemberKind_StaticMethod:         {result = E_MemberKind_StaticMethod;}break;
    case RDI_MemberKind_VirtualMethod:        {result = E_MemberKind_VirtualMethod;}break;
    case RDI_MemberKind_VTablePtr:            {result = E_MemberKind_VTablePtr;}break;
    case RDI_MemberKind_Base:                 {result = E_MemberKind_Base;}break;
    case RDI_MemberKind_VirtualBase:          {result = E_MemberKind_VirtualBase;}break;
    case RDI_MemberKind_NestedType:           {result = E_MemberKind_NestedType;}break;
  }
  return result;
}

internal RDI_EvalTypeGroup
e_type_group_from_kind(E_TypeKind kind)
{
  RDI_EvalTypeGroup result = 0;
  switch(kind)
  {
    default:{}break;
    
    case E_TypeKind_Null: case E_TypeKind_Void:
    case E_TypeKind_F16:  case E_TypeKind_F32PP: case E_TypeKind_F48:
    case E_TypeKind_F80:  case E_TypeKind_F128:
    case E_TypeKind_ComplexF32: case E_TypeKind_ComplexF64:
    case E_TypeKind_ComplexF80: case E_TypeKind_ComplexF128:
    case E_TypeKind_Modifier:   case E_TypeKind_Array:
    case E_TypeKind_Struct:     case E_TypeKind_Class: case E_TypeKind_Union:
    case E_TypeKind_Enum:       case E_TypeKind_Alias:
    case E_TypeKind_IncompleteStruct: case E_TypeKind_IncompleteClass:
    case E_TypeKind_IncompleteUnion:  case E_TypeKind_IncompleteEnum:
    case E_TypeKind_Bitfield:
    case E_TypeKind_Variadic:
    {result = RDI_EvalTypeGroup_Other;}break;
    
    case E_TypeKind_Handle:
    case E_TypeKind_UChar8: case E_TypeKind_UChar16: case E_TypeKind_UChar32:
    case E_TypeKind_U8:     case E_TypeKind_U16:     case E_TypeKind_U32:
    case E_TypeKind_U64:    case E_TypeKind_U128:    case E_TypeKind_U256:
    case E_TypeKind_U512:
    case E_TypeKind_Ptr: case E_TypeKind_LRef: case E_TypeKind_RRef:
    case E_TypeKind_Function: case E_TypeKind_Method: case E_TypeKind_MemberPtr:
    {result = RDI_EvalTypeGroup_U;}break;
    
    case E_TypeKind_Char8: case E_TypeKind_Char16: case E_TypeKind_Char32:
    case E_TypeKind_S8:    case E_TypeKind_S16:    case E_TypeKind_S32:
    case E_TypeKind_S64:   case E_TypeKind_S128:   case E_TypeKind_S256:
    case E_TypeKind_S512:
    case E_TypeKind_Bool:
    {result = RDI_EvalTypeGroup_S;}break;
    
    case E_TypeKind_F32:{result = RDI_EvalTypeGroup_F32;}break;
    case E_TypeKind_F64:{result = RDI_EvalTypeGroup_F64;}break;
  }
  return result;
}

internal B32
e_type_kind_is_integer(E_TypeKind kind)
{
  B32 result = (E_TypeKind_FirstInteger <= kind && kind <= E_TypeKind_LastInteger);
  return result;
}

internal B32
e_type_kind_is_signed(E_TypeKind kind)
{
  B32 result = ((E_TypeKind_FirstSigned1 <= kind && kind <= E_TypeKind_LastSigned1) ||
                (E_TypeKind_FirstSigned2 <= kind && kind <= E_TypeKind_LastSigned2));
  return result;
}

internal B32
e_type_kind_is_basic_or_enum(E_TypeKind kind)
{
  B32 result = ((E_TypeKind_FirstBasic <= kind && kind <= E_TypeKind_LastBasic) ||
                kind == E_TypeKind_Enum);
  return result;
}

internal B32
e_type_kind_is_pointer_or_ref(E_TypeKind kind)
{
  B32 result = (kind == E_TypeKind_Ptr || kind == E_TypeKind_LRef || kind == E_TypeKind_RRef);
  return result;
}

////////////////////////////////
//~ rjf: Member Functions

internal void
e_member_list_push(Arena *arena, E_MemberList *list, E_Member *member)
{
  E_MemberNode *n = push_array(arena, E_MemberNode, 1);
  MemoryCopyStruct(&n->v, member);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal E_MemberArray
e_member_array_from_list(Arena *arena, E_MemberList *list)
{
  E_MemberArray array = {0};
  array.count = list->count;
  array.v = push_array(arena, E_Member, array.count);
  {
    U64 idx = 0;
    for(E_MemberNode *n = list->first; n != 0; n = n->next, idx += 1)
    {
      MemoryCopyStruct(&array.v[idx], &n->v);
    }
  }
  return array;
}

////////////////////////////////
//~ rjf: Enum Value Functions

internal void
e_enum_val_list_push(Arena *arena, E_EnumValList *list, E_EnumVal *enum_val)
{
  E_EnumValNode *n = push_array(arena, E_EnumValNode, 1);
  MemoryCopyStruct(&n->v, enum_val);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal E_EnumValArray
e_enum_val_array_from_list(Arena *arena, E_EnumValList *list)
{
  E_EnumValArray array = {0};
  array.count = list->count;
  array.v = push_array(arena, E_EnumVal, array.count);
  {
    U64 idx = 0;
    for(E_EnumValNode *n = list->first; n != 0; n = n->next, idx += 1)
    {
      MemoryCopyStruct(&array.v[idx], &n->v);
    }
  }
  return array;
}

////////////////////////////////
//~ rjf: Type Operation Functions

//- rjf: key constructors

internal E_TypeKey
e_type_key_zero(void)
{
  E_TypeKey k = zero_struct;
  return k;
}

internal E_TypeKey
e_type_key_basic(E_TypeKind kind)
{
  E_TypeKey key = {E_TypeKeyKind_Basic};
  key.u32[0] = (U32)kind;
  return key;
}

internal E_TypeKey
e_type_key_ext(E_TypeKind kind, U32 type_idx, U32 rdi_idx)
{
  E_TypeKey key = {E_TypeKeyKind_Ext};
  key.u32[0] = (U32)kind;
  if(E_TypeKind_FirstBasic <= kind && kind <= E_TypeKind_LastBasic)
  {
    key.kind = E_TypeKeyKind_Basic;
  }
  else
  {
    key.u32[1] = type_idx;
    key.u32[2] = rdi_idx;
  }
  return key;
}

internal E_TypeKey
e_type_key_reg(Arch arch, REGS_RegCode code)
{
  E_TypeKey key = {E_TypeKeyKind_Reg};
  key.u32[0] = (U32)arch;
  key.u32[1] = (U32)code;
  return key;
}

internal E_TypeKey
e_type_key_reg_alias(Arch arch, REGS_AliasCode code)
{
  E_TypeKey key = {E_TypeKeyKind_RegAlias};
  key.u32[0] = (U32)arch;
  key.u32[1] = (U32)code;
  return key;
}

//- rjf: constructed type construction

internal U64
e_hash_from_cons_type_params(E_ConsTypeParams *params)
{
  U32 buffer[] =
  {
    (U32)params->kind,
    (U32)params->direct_key.kind,
    params->direct_key.u32[0],
    params->direct_key.u32[1],
    params->direct_key.u32[2],
    (U32)((params->count & 0x00000000ffffffffull)>> 0),
    (U32)((params->count & 0xffffffff00000000ull)>> 32),
    (U32)((params->depth & 0x00000000ffffffffull)>> 0),
    (U32)((params->depth & 0xffffffff00000000ull)>> 32),
  };
  U64 hash = e_hash_from_string(5381, str8((U8 *)buffer, sizeof(buffer)));
  hash = e_hash_from_string(hash, params->name);
  return hash;
}

internal B32
e_cons_type_params_match(E_ConsTypeParams *l, E_ConsTypeParams *r)
{
  B32 result = (l->kind != E_TypeKind_Lens &&
                l->kind == r->kind &&
                l->flags == r->flags &&
                str8_match(l->name, r->name, 0) &&
                e_type_key_match(l->direct_key, r->direct_key) &&
                l->count == r->count &&
                l->depth == r->depth);
  if(result && l->members != 0 && r->members != 0)
  {
    for(U64 idx = 0; idx < l->count; idx += 1)
    {
      if(l->members[idx].kind != r->members[idx].kind ||
         !e_type_key_match(l->members[idx].type_key, r->members[idx].type_key) ||
         !str8_match(l->members[idx].name, r->members[idx].name, 0) ||
         l->members[idx].off != r->members[idx].off)
      {
        result = 0;
        break;
      }
    }
  }
  if(result && l->enum_vals != 0 && r->enum_vals != 0)
  {
    for(U64 idx = 0; idx < l->count; idx += 1)
    {
      if(l->enum_vals[idx].val != r->enum_vals[idx].val ||
         !str8_match(l->enum_vals[idx].name, r->enum_vals[idx].name, 0))
      {
        result = 0;
        break;
      }
    }
  }
  return result;
}

internal E_TypeKey
e_type_key_cons_(E_ConsTypeParams *params)
{
  U64 content_hash = e_hash_from_cons_type_params(params);
  U64 content_slot_idx = content_hash%e_cache->cons_content_slots_count;
  E_ConsTypeSlot *content_slot = &e_cache->cons_content_slots[content_slot_idx];
  E_ConsTypeNode *node = 0;
  for(E_ConsTypeNode *n = content_slot->first; n != 0; n = n->content_next)
  {
    if(e_cons_type_params_match(params, &n->params))
    {
      node = n;
      break;
    }
  }
  E_TypeKey result = zero_struct;
  if(node == 0)
  {
    E_TypeKey key = {E_TypeKeyKind_Cons};
    key.u32[0] = (U32)params->kind;
    key.u32[1] = (U32)e_cache->cons_id_gen;
    e_cache->cons_id_gen += 1;
    U64 key_hash = e_hash_from_string(5381, str8_struct(&key));
    U64 key_slot_idx = key_hash%e_cache->cons_key_slots_count;
    E_ConsTypeSlot *key_slot = &e_cache->cons_key_slots[key_slot_idx];
    E_ConsTypeNode *node = push_array(e_cache->arena, E_ConsTypeNode, 1);
    SLLQueuePush_N(content_slot->first, content_slot->last, node, content_next);
    SLLQueuePush_N(key_slot->first, key_slot->last, node, key_next);
    node->key = key;
    MemoryCopyStruct(&node->params, params);
    node->params.name = push_str8_copy(e_cache->arena, params->name);
    if(node->params.expand.info != 0)
    {
      if(node->params.expand.range == 0)       {node->params.expand.range       = E_TYPE_EXPAND_RANGE_FUNCTION_NAME(default);}
      if(node->params.expand.id_from_num == 0) {node->params.expand.id_from_num = E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_NAME(identity);}
      if(node->params.expand.num_from_id == 0) {node->params.expand.num_from_id = E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_NAME(identity);}
    }
    if(params->members != 0)
    {
      node->params.members = push_array(e_cache->arena, E_Member, params->count);
      MemoryCopy(node->params.members, params->members, sizeof(E_Member)*params->count);
      for(U64 idx = 0; idx < node->params.count; idx += 1)
      {
        node->params.members[idx].name = push_str8_copy(e_cache->arena, node->params.members[idx].name);
        node->params.members[idx].inheritance_key_chain = e_type_key_list_copy(e_cache->arena, &node->params.members[idx].inheritance_key_chain);
        U64 opl_off = (node->params.members[idx].off + e_type_byte_size_from_key(node->params.members[idx].type_key));
        node->byte_size = Max(node->byte_size, opl_off);
      }
    }
    else if(params->enum_vals != 0)
    {
      node->params.enum_vals = push_array(e_cache->arena, E_EnumVal, params->count);
      MemoryCopy(node->params.enum_vals, params->enum_vals, sizeof(E_EnumVal)*params->count);
      for(U64 idx = 0; idx < node->params.count; idx += 1)
      {
        node->params.enum_vals[idx].name = push_str8_copy(e_cache->arena, node->params.enum_vals[idx].name);
      }
      node->byte_size = e_type_byte_size_from_key(node->params.direct_key);
    }
    else if(params->args != 0)
    {
      node->params.args = push_array(e_cache->arena, E_Expr *, params->count);
      for EachIndex(idx, params->count)
      {
        node->params.args[idx] = e_expr_copy(e_cache->arena, params->args[idx]);
      }
    }
    else switch(params->kind)
    {
      default:
      {
        node->byte_size = e_type_byte_size_from_key(node->params.direct_key);
      }break;
      case E_TypeKind_Ptr:
      {
        node->byte_size = bit_size_from_arch(node->params.arch)/8;
      }break;
      case E_TypeKind_Array:
      {
        U64 ptee_size = e_type_byte_size_from_key(node->params.direct_key);
        node->byte_size = ptee_size * node->params.count;
      }break;
    }
    result = key;
  }
  else
  {
    result = node->key;
  }
  return result;
}

//- rjf: constructed type helpers

internal E_TypeKey
e_type_key_cons_array(E_TypeKey element_type_key, U64 count, E_TypeFlags flags)
{
  E_TypeKey key = e_type_key_cons(.kind = E_TypeKind_Array, .direct_key = element_type_key, .count = count, .flags = flags);
  return key;
}

internal E_TypeKey
e_type_key_cons_ptr(Arch arch, E_TypeKey element_type_key, U64 count, E_TypeFlags flags)
{
  E_TypeKey key = e_type_key_cons(.arch = arch, .kind = E_TypeKind_Ptr, .flags = flags, .direct_key = element_type_key, .count = count);
  return key;
}

internal E_TypeKey
e_type_key_cons_meta_expr(E_TypeKey type_key, String8 expr)
{
  E_TypeKey key = e_type_key_cons(.kind = E_TypeKind_MetaExpr, .direct_key = type_key, .name = expr);
  return key;
}

internal E_TypeKey
e_type_key_cons_meta_display_name(E_TypeKey type_key, String8 name)
{
  E_TypeKey key = e_type_key_cons(.kind = E_TypeKind_MetaDisplayName, .direct_key = type_key, .name = name);
  return key;
}

internal E_TypeKey
e_type_key_cons_meta_description(E_TypeKey type_key, String8 desc)
{
  E_TypeKey key = e_type_key_cons(.kind = E_TypeKind_MetaDescription, .direct_key = type_key, .name = desc);
  return key;
}

internal E_TypeKey
e_type_key_cons_base(Type *type)
{
  E_TypeKey result = e_type_key_zero();
  switch(type->kind)
  {
    default:
    if(TypeKind_FirstLeaf <= type->kind && type->kind <= TypeKind_LastLeaf)
    {
      E_TypeKind kind = e_type_kind_from_base(type->kind);
      result = e_type_key_basic(kind);
    }break;
    case TypeKind_Ptr:
    {
      E_TypeKey direct_type = e_type_key_cons_base(type->direct);
      E_TypeFlags flags = 0;
      if(type->flags & TypeFlag_IsPlainText){ flags |= E_TypeFlag_IsPlainText; }
      if(type->flags & TypeFlag_IsCodeText) { flags |= E_TypeFlag_IsCodeText; }
      if(type->flags & TypeFlag_IsPathText) { flags |= E_TypeFlag_IsPathText; }
      result = e_type_key_cons_ptr(Arch_CURRENT, direct_type, 1, flags);
    }break;
    case TypeKind_Array:
    {
      E_TypeKey direct_type = e_type_key_cons_base(type->direct);
      result = e_type_key_cons_array(direct_type, type->count, 0);
    }break;
    case TypeKind_Struct:
    {
      Temp scratch = scratch_begin(0, 0);
      E_MemberList members = {0};
      for(U64 idx = 0; idx < type->count; idx += 1)
      {
        E_TypeKey member_type_key = e_type_key_cons_base(type->members[idx].type);
        e_member_list_push_new(scratch.arena, &members, .name = type->members[idx].name, .off = type->members[idx].value, .type_key = member_type_key);
      }
      E_MemberArray members_array = e_member_array_from_list(scratch.arena, &members);
      result = e_type_key_cons(.arch    = Arch_CURRENT,
                               .kind    = E_TypeKind_Struct,
                               .name    = type->name,
                               .members = members_array.v,
                               .count   = members_array.count);
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal E_TypeKey
e_type_key_file(void)
{
  E_TypeKey key = e_cache->file_type_key;
  return key;
}

internal E_TypeKey
e_type_key_folder(void)
{
  E_TypeKey key = e_cache->folder_type_key;
  return key;
}

//- rjf: basic type key functions

internal B32
e_type_key_match(E_TypeKey l, E_TypeKey r)
{
  B32 result = MemoryMatchStruct(&l, &r);
  return result;
}

//- rjf: type key -> info extraction

internal U64
e_hash_from_type(E_Type *type)
{
  U64 hash = 0;
  if(type != &e_type_nil)
  {
    Temp scratch = scratch_begin(0, 0);
    String8List strings = {0};
    str8_serial_begin(scratch.arena, &strings);
    str8_serial_push_struct(scratch.arena, &strings, &type->kind);
    str8_serial_push_struct(scratch.arena, &strings, &type->flags);
    str8_serial_push_string(scratch.arena, &strings, type->name);
    str8_serial_push_struct(scratch.arena, &strings, &type->byte_size);
    str8_serial_push_struct(scratch.arena, &strings, &type->count);
    str8_serial_push_struct(scratch.arena, &strings, &type->off);
    String8 direct_type_string = e_type_string_from_key(scratch.arena, type->direct_type_key);
    String8 owner_type_string = e_type_string_from_key(scratch.arena, type->owner_type_key);
    U64 direct_hash = e_hash_from_string(5381, direct_type_string);
    U64 owner_hash = e_hash_from_string(5381, owner_type_string);
    str8_serial_push_struct(scratch.arena, &strings, &direct_hash);
    str8_serial_push_struct(scratch.arena, &strings, &owner_hash);
    if(type->param_type_keys != 0)
    {
      for EachIndex(idx, type->count)
      {
        String8 param_type_string = e_type_string_from_key(scratch.arena, type->param_type_keys[idx]);
        U64 param_type_hash = e_hash_from_string(5381, param_type_string);
        str8_serial_push_struct(scratch.arena, &strings, &param_type_hash);
      }
    }
    else if(type->members != 0)
    {
      for EachIndex(idx, type->count)
      {
        String8 member_type_string = e_type_string_from_key(scratch.arena, type->members[idx].type_key);
        U64 member_type_hash = e_hash_from_string(5381, member_type_string);
        str8_serial_push_struct(scratch.arena, &strings, &type->members[idx].off);
        str8_serial_push_struct(scratch.arena, &strings, &member_type_hash);
      }
    }
    String8 string = str8_serial_end(scratch.arena, &strings);
    hash = e_hash_from_string(5381, string);
    scratch_end(scratch);
  }
  return hash;
}

internal E_TypeKind
e_type_kind_from_key(E_TypeKey key)
{
  E_TypeKind kind = E_TypeKind_Null;
  switch(key.kind)
  {
    default:{}break;
    case E_TypeKeyKind_Basic:   {kind = (E_TypeKind)key.u32[0];}break;
    case E_TypeKeyKind_Ext:     {kind = (E_TypeKind)key.u32[0];}break;
    case E_TypeKeyKind_Cons:    {kind = (E_TypeKind)key.u32[0];}break;
    case E_TypeKeyKind_Reg:     {kind = E_TypeKind_Union;}break;
    case E_TypeKeyKind_RegAlias:{kind = E_TypeKind_Union;}break;
  }
  return kind;
}

internal U64
e_type_byte_size_from_key(E_TypeKey key)
{
  ProfBeginFunction();
  U64 result = 0;
  switch(key.kind)
  {
    default:{}break;
    case E_TypeKeyKind_Basic:
    {
      E_TypeKind kind = (E_TypeKind)key.u32[0];
      result = e_type_kind_basic_byte_size_table[kind];
    }break;
    case E_TypeKeyKind_Ext:
    {
      U64 type_node_idx = key.u32[1];
      U32 rdi_idx = key.u32[2];
      RDI_Parsed *rdi = e_base_ctx->modules[rdi_idx].rdi;
      RDI_TypeNode *rdi_type = rdi_element_from_name_idx(rdi, TypeNodes, type_node_idx);
      result = rdi_type->byte_size;
    }break;
    case E_TypeKeyKind_Cons:
    {
      U64 key_hash = e_hash_from_string(5381, str8_struct(&key));
      U64 key_slot_idx = key_hash%e_cache->cons_key_slots_count;
      E_ConsTypeSlot *key_slot = &e_cache->cons_key_slots[key_slot_idx];
      for(E_ConsTypeNode *node = key_slot->first;
          node != 0;
          node = node->key_next)
      {
        if(e_type_key_match(node->key, key))
        {
          result = node->byte_size;
          break;
        }
      }
    }break;
  }
  ProfEnd();
  return result;
}

internal E_Type *
e_push_type_from_key(Arena *arena, E_TypeKey key)
{
  ProfBeginFunction();
  E_Type *type = &e_type_nil;
  U64 reg_byte_count = 0;
  {
    switch(key.kind)
    {
      default:{}break;
      
      //- rjf: basic type keys
      case E_TypeKeyKind_Basic:
      {
        E_TypeKind kind = (E_TypeKind)key.u32[0];
        if(E_TypeKind_FirstBasic <= kind && kind <= E_TypeKind_LastBasic)
        {
          type = push_array(arena, E_Type, 1);
          type->kind       = kind;
          type->name       = e_type_kind_basic_string_table[kind];
          type->byte_size  = e_type_kind_basic_byte_size_table[kind];
        }
      }break;
      
      //- rjf: constructed type keys
      case E_TypeKeyKind_Cons:
      {
        U64 key_hash = e_hash_from_string(5381, str8_struct(&key));
        U64 key_slot_idx = key_hash%e_cache->cons_key_slots_count;
        E_ConsTypeSlot *key_slot = &e_cache->cons_key_slots[key_slot_idx];
        for(E_ConsTypeNode *node = key_slot->first;
            node != 0;
            node = node->key_next)
        {
          if(e_type_key_match(node->key, key))
          {
            type = push_array(arena, E_Type, 1);
            type->kind               = e_type_kind_from_key(node->key);
            type->flags              = node->params.flags;
            type->name               = push_str8_copy(arena, node->params.name);
            type->direct_type_key    = node->params.direct_key;
            type->count              = node->params.count;
            type->depth              = node->params.depth;
            type->arch               = node->params.arch;
            type->irext              = node->params.irext;
            type->access             = node->params.access;
            type->expand             = node->params.expand;
            type->byte_size          = node->byte_size;
            switch(type->kind)
            {
              default:{}break;
              case E_TypeKind_Lens:
              {
                type->args = push_array(arena, E_Expr *, type->count);
                MemoryCopy(type->args, node->params.args, sizeof(E_Expr *)*type->count);
                for EachIndex(idx, type->count)
                {
                  type->args[idx] = e_expr_copy(arena, type->args[idx]);
                }
              }break;
              case E_TypeKind_Struct:
              case E_TypeKind_Union:
              case E_TypeKind_Class:
              {
                type->members = push_array(arena, E_Member, type->count);
                MemoryCopy(type->members, node->params.members, sizeof(E_Member)*type->count);
                for(U64 idx = 0; idx < type->count; idx += 1)
                {
                  U64 opl_byte = type->members[idx].off + e_type_byte_size_from_key(type->members[idx].type_key);
                  type->byte_size = Max(type->byte_size, opl_byte);
                }
              }break;
              case E_TypeKind_Enum:
              {
                type->enum_vals = push_array(arena, E_EnumVal, type->count);
                MemoryCopy(type->enum_vals, node->params.enum_vals, sizeof(E_EnumVal)*type->count);
              }break;
            }
          }
        }
      }break;
      
      //- rjf: external (rdi) type keys
      case E_TypeKeyKind_Ext:
      {
        U64 type_node_idx = key.u32[1];
        U32 rdi_idx = key.u32[2];
        RDI_Parsed *rdi = e_base_ctx->modules[rdi_idx].rdi;
        RDI_TypeNode *rdi_type = rdi_element_from_name_idx(rdi, TypeNodes, type_node_idx);
        if(rdi_type->kind != RDI_TypeKind_NULL)
        {
          E_TypeKind kind = e_type_kind_from_rdi(rdi_type->kind);
          
          //- rjf: record types => unpack name * members & produce
          if(RDI_TypeKind_FirstRecord <= rdi_type->kind && rdi_type->kind <= RDI_TypeKind_LastRecord)
          {
            // rjf: unpack name
            String8 name = {0};
            name.str = rdi_string_from_idx(rdi, rdi_type->user_defined.name_string_idx, &name.size);
            
            // rjf: unpack UDT info
            RDI_UDT *udt = rdi_element_from_name_idx(rdi, UDTs, rdi_type->user_defined.udt_idx);
            
            // rjf: unpack members
            E_Member *members = 0;
            U32 members_count = 0;
            {
              members_count = udt->member_count;
              members = push_array(arena, E_Member, members_count);
              if(members_count != 0)
              {
                for(U32 member_idx = udt->member_first;
                    member_idx < udt->member_first+udt->member_count;
                    member_idx += 1)
                {
                  RDI_Member *src = rdi_element_from_name_idx(rdi, Members, member_idx);
                  E_TypeKind member_type_kind = E_TypeKind_Null;
                  RDI_TypeNode *member_type = rdi_element_from_name_idx(rdi, TypeNodes, src->type_idx);
                  member_type_kind = e_type_kind_from_rdi(member_type->kind);
                  E_Member *dst = &members[member_idx-udt->member_first];
                  dst->kind     = e_member_kind_from_rdi(src->kind);
                  dst->type_key = e_type_key_ext(member_type_kind, src->type_idx, rdi_idx);
                  dst->name.str = rdi_string_from_idx(rdi, src->name_string_idx, &dst->name.size);
                  dst->off      = (U64)src->off;
                }
              }
            }
            
            // rjf: produce
            type = push_array(arena, E_Type, 1);
            type->kind       = kind;
            type->name       = push_str8_copy(arena, name);
            type->byte_size  = (U64)rdi_type->byte_size;
            type->count      = members_count;
            type->arch       = e_base_ctx->modules[rdi_idx].arch;
            type->members    = members;
          }
          
          //- rjf: enum types => unpack name * values & produce
          else if(rdi_type->kind == RDI_TypeKind_Enum)
          {
            // rjf: unpack name
            String8 name = {0};
            name.str = rdi_string_from_idx(rdi, rdi_type->user_defined.name_string_idx, &name.size);
            
            // rjf: unpack direct type
            E_TypeKey direct_type_key = zero_struct;
            if(rdi_type->user_defined.direct_type_idx < type_node_idx)
            {
              RDI_TypeNode *direct_type_node = rdi_element_from_name_idx(rdi, TypeNodes, rdi_type->user_defined.direct_type_idx);
              E_TypeKind direct_type_kind = e_type_kind_from_rdi(direct_type_node->kind);
              direct_type_key = e_type_key_ext(direct_type_kind, rdi_type->user_defined.direct_type_idx, rdi_idx);
            }
            
            // rjf: unpack members
            E_EnumVal *enum_vals = 0;
            U32 enum_vals_count = 0;
            {
              U32 udt_idx = rdi_type->user_defined.udt_idx;
              RDI_UDT *udt = rdi_element_from_name_idx(rdi, UDTs, udt_idx);
              enum_vals_count = udt->member_count;
              enum_vals = push_array(arena, E_EnumVal, enum_vals_count);
              for(U32 member_idx = udt->member_first;
                  member_idx < udt->member_first+udt->member_count;
                  member_idx += 1)
              {
                RDI_EnumMember *src = rdi_element_from_name_idx(rdi, EnumMembers, member_idx);
                E_EnumVal *dst = &enum_vals[member_idx-udt->member_first];
                dst->name.str = rdi_string_from_idx(rdi, src->name_string_idx, &dst->name.size);
                dst->val      = src->val;
              }
            }
            
            // rjf: produce
            type = push_array(arena, E_Type, 1);
            type->kind            = kind;
            type->name            = push_str8_copy(arena, name);
            type->byte_size       = (U64)rdi_type->byte_size;
            type->count           = enum_vals_count;
            type->arch            = e_base_ctx->modules[rdi_idx].arch;
            type->enum_vals       = enum_vals;
            type->direct_type_key = direct_type_key;
          }
          
          //- rjf: constructed types
          else if(RDI_TypeKind_FirstConstructed <= rdi_type->kind && rdi_type->kind <= RDI_TypeKind_LastConstructed)
          {
            // rjf: unpack direct type
            B32 direct_type_is_good = 0;
            E_TypeKey direct_type_key = zero_struct;
            U64 direct_type_byte_size = 0;
            if(rdi_type->constructed.direct_type_idx < type_node_idx)
            {
              RDI_TypeNode *direct_type_node = rdi_element_from_name_idx(rdi, TypeNodes, rdi_type->constructed.direct_type_idx);
              E_TypeKind direct_type_kind = e_type_kind_from_rdi(direct_type_node->kind);
              direct_type_key = e_type_key_ext(direct_type_kind, rdi_type->constructed.direct_type_idx, rdi_idx);
              direct_type_is_good = 1;
              direct_type_byte_size = (U64)direct_type_node->byte_size;
            }
            
            // rjf: construct based on kind
            switch(rdi_type->kind)
            {
              case RDI_TypeKind_Modifier:
              {
                E_TypeFlags flags = 0;
                if(rdi_type->flags & RDI_TypeModifierFlag_Const)
                {
                  flags |= E_TypeFlag_Const;
                }
                if(rdi_type->flags & RDI_TypeModifierFlag_Volatile)
                {
                  flags |= E_TypeFlag_Volatile;
                }
                if(rdi_type->flags & RDI_TypeModifierFlag_Restrict)
                {
                  flags |= E_TypeFlag_Restrict;
                }
                type = push_array(arena, E_Type, 1);
                type->kind            = kind;
                type->direct_type_key = direct_type_key;
                type->byte_size       = direct_type_byte_size;
                type->flags           = flags;
                type->arch            = e_base_ctx->modules[rdi_idx].arch;
              }break;
              case RDI_TypeKind_Ptr:
              case RDI_TypeKind_LRef:
              case RDI_TypeKind_RRef:
              {
                type = push_array(arena, E_Type, 1);
                type->kind            = kind;
                type->direct_type_key = direct_type_key;
                type->byte_size       = bit_size_from_arch(e_base_ctx->modules[rdi_idx].arch)/8;
                type->count           = 1;
                type->arch            = e_base_ctx->modules[rdi_idx].arch;
              }break;
              
              case RDI_TypeKind_Array:
              {
                type = push_array(arena, E_Type, 1);
                type->kind            = kind;
                type->direct_type_key = direct_type_key;
                type->count           = rdi_type->constructed.count;
                type->byte_size       = direct_type_byte_size * type->count;
                type->arch            = e_base_ctx->modules[rdi_idx].arch;
              }break;
              case RDI_TypeKind_Function:
              {
                U32 count = rdi_type->constructed.count;
                U32 idx_run_first = rdi_type->constructed.param_idx_run_first;
                U32 check_count = 0;
                U32 *idx_run = rdi_idx_run_from_first_count(rdi, idx_run_first, count, &check_count);
                if(check_count == count)
                {
                  type = push_array(arena, E_Type, 1);
                  type->kind            = kind;
                  type->byte_size       = bit_size_from_arch(e_base_ctx->modules[rdi_idx].arch)/8;
                  type->direct_type_key = direct_type_key;
                  type->count           = count;
                  type->param_type_keys = push_array(arena, E_TypeKey, type->count);
                  type->arch            = e_base_ctx->modules[rdi_idx].arch;
                  for(U32 idx = 0; idx < type->count; idx += 1)
                  {
                    U32 param_type_idx = idx_run[idx];
                    if(param_type_idx < type_node_idx)
                    {
                      RDI_TypeNode *param_type_node = rdi_element_from_name_idx(rdi, TypeNodes, param_type_idx);
                      E_TypeKind param_kind = e_type_kind_from_rdi(param_type_node->kind);
                      type->param_type_keys[idx] = e_type_key_ext(param_kind, param_type_idx, rdi_idx);
                    }
                    else
                    {
                      break;
                    }
                  }
                }
              }break;
              case RDI_TypeKind_Method:
              {
                // NOTE(rjf): for methods, the `direct` type points at the owner type.
                // the return type, instead of being encoded via the `direct` type, is
                // encoded via the first parameter.
                U32 count = rdi_type->constructed.count;
                U32 idx_run_first = rdi_type->constructed.param_idx_run_first;
                U32 check_count = 0;
                U32 *idx_run = rdi_idx_run_from_first_count(rdi, idx_run_first, count, &check_count);
                if(check_count == count)
                {
                  type = push_array(arena, E_Type, 1);
                  type->kind            = kind;
                  type->byte_size       = bit_size_from_arch(e_base_ctx->modules[rdi_idx].arch)/8;
                  type->owner_type_key  = direct_type_key;
                  type->count           = count;
                  type->param_type_keys = push_array_no_zero(arena, E_TypeKey, type->count);
                  type->arch            = e_base_ctx->modules[rdi_idx].arch;
                  for(U32 idx = 0; idx < type->count; idx += 1)
                  {
                    U32 param_type_idx = idx_run[idx];
                    if(param_type_idx < type_node_idx)
                    {
                      RDI_TypeNode *param_type_node = rdi_element_from_name_idx(rdi, TypeNodes, param_type_idx);
                      E_TypeKind param_kind = e_type_kind_from_rdi(param_type_node->kind);
                      type->param_type_keys[idx] = e_type_key_ext(param_kind, param_type_idx, rdi_idx);
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
              case RDI_TypeKind_MemberPtr:
              {
                // rjf: unpack owner type
                E_TypeKey owner_type_key = zero_struct;
                if(rdi_type->constructed.owner_type_idx < type_node_idx)
                {
                  RDI_TypeNode *owner_type_node = rdi_element_from_name_idx(rdi, TypeNodes, rdi_type->constructed.owner_type_idx);
                  E_TypeKind owner_type_kind = e_type_kind_from_rdi(owner_type_node->kind);
                  owner_type_key = e_type_key_ext(owner_type_kind, rdi_type->constructed.owner_type_idx, rdi_idx);
                }
                type = push_array(arena, E_Type, 1);
                type->kind            = kind;
                type->byte_size       = bit_size_from_arch(e_base_ctx->modules[rdi_idx].arch)/8;
                type->owner_type_key  = owner_type_key;
                type->direct_type_key = direct_type_key;
                type->arch            = e_base_ctx->modules[rdi_idx].arch;
              }break;
            }
          }
          
          //- rjf: alias types
          else if(rdi_type->kind == RDI_TypeKind_Alias)
          {
            // rjf: unpack name
            String8 name = {0};
            name.str = rdi_string_from_idx(rdi, rdi_type->user_defined.name_string_idx, &name.size);
            
            // rjf: unpack direct type
            E_TypeKey direct_type_key = zero_struct;
            U64 direct_type_byte_size = 0;
            if(rdi_type->user_defined.direct_type_idx < type_node_idx)
            {
              RDI_TypeNode *direct_type_node = rdi_element_from_name_idx(rdi, TypeNodes, rdi_type->user_defined.direct_type_idx);
              E_TypeKind direct_type_kind = e_type_kind_from_rdi(direct_type_node->kind);
              direct_type_key = e_type_key_ext(direct_type_kind, rdi_type->user_defined.direct_type_idx, rdi_idx);
              direct_type_byte_size = direct_type_node->byte_size;
            }
            
            // rjf: produce
            type = push_array(arena, E_Type, 1);
            type->kind            = kind;
            type->name            = push_str8_copy(arena, name);
            type->byte_size       = direct_type_byte_size;
            type->direct_type_key = direct_type_key;
            type->arch            = e_base_ctx->modules[rdi_idx].arch;
          }
          
          //- rjf: bitfields
          else if(RDI_TypeKind_Bitfield == rdi_type->kind)
          {
            // rjf: unpack direct type
            E_TypeKey direct_type_key = zero_struct;
            U64 direct_type_byte_size = 0;
            if(rdi_type->bitfield.direct_type_idx < type_node_idx)
            {
              RDI_TypeNode *direct_type_node = rdi_element_from_name_idx(rdi, TypeNodes, rdi_type->bitfield.direct_type_idx);
              E_TypeKind direct_type_kind = e_type_kind_from_rdi(direct_type_node->kind);
              direct_type_key = e_type_key_ext(direct_type_kind, rdi_type->bitfield.direct_type_idx, rdi_idx);
              direct_type_byte_size = direct_type_node->byte_size;
            }
            
            // rjf: produce
            type = push_array(arena, E_Type, 1);
            type->kind            = kind;
            type->byte_size       = direct_type_byte_size;
            type->direct_type_key = direct_type_key;
            type->off             = (U32)rdi_type->bitfield.off;
            type->count           = (U64)rdi_type->bitfield.size;
            type->arch            = e_base_ctx->modules[rdi_idx].arch;
          }
          
          //- rjf: incomplete types
          else if(RDI_TypeKind_FirstIncomplete <= rdi_type->kind && rdi_type->kind <= RDI_TypeKind_LastIncomplete)
          {
            // rjf: unpack name
            String8 name = {0};
            name.str = rdi_string_from_idx(rdi, rdi_type->user_defined.name_string_idx, &name.size);
            
            // rjf: produce
            type = push_array(arena, E_Type, 1);
            type->kind            = kind;
            type->name            = push_str8_copy(arena, name);
            type->arch            = e_base_ctx->modules[rdi_idx].arch;
          }
          
        }
      }break;
      
      //- rjf: reg type keys
      case E_TypeKeyKind_Reg:
      {
        Arch arch = (Arch)key.u32[0];
        REGS_RegCode code = (REGS_RegCode)key.u32[1];
        REGS_Rng rng = regs_reg_code_rng_table_from_arch(arch)[code];
        reg_byte_count = (U64)rng.byte_size;
      }goto build_reg_type;
      case E_TypeKeyKind_RegAlias:
      {
        Arch arch = (Arch)key.u32[0];
        REGS_AliasCode code = (REGS_AliasCode)key.u32[1];
        REGS_Slice slice = regs_alias_code_slice_table_from_arch(arch)[code];
        reg_byte_count = (U64)slice.byte_size;
      }goto build_reg_type;
      build_reg_type:
      {
        Temp scratch = scratch_begin(&arena, 1);
        type = push_array(arena, E_Type, 1);
        type->kind       = E_TypeKind_Union;
        type->name       = push_str8f(arena, "reg_%I64u_bit", reg_byte_count*8);
        type->byte_size  = (U64)reg_byte_count;
        type->arch       = (Arch)key.u32[0];
        
        // rjf: build register type members
        E_MemberList members = {0};
        {
          // rjf: build exact-sized members
          {
            if(type->byte_size == 16)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u128");
              mem->type_key = e_type_key_basic(E_TypeKind_U128);
            }
            if(type->byte_size == 8)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u64");
              mem->type_key = e_type_key_basic(E_TypeKind_U64);
            }
            if(type->byte_size == 4)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u32");
              mem->type_key = e_type_key_basic(E_TypeKind_U32);
            }
            if(type->byte_size == 2)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u16");
              mem->type_key = e_type_key_basic(E_TypeKind_U16);
            }
            if(type->byte_size == 1)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u8");
              mem->type_key = e_type_key_basic(E_TypeKind_U8);
            }
          }
          
          // rjf: build arrays for subdivisions
          {
            if(type->byte_size > 16 && type->byte_size%16 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u128s");
              mem->type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U128), reg_byte_count/16, 0);
            }
            if(type->byte_size > 8 && type->byte_size%8 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u64s");
              mem->type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U64), reg_byte_count/8, 0);
            }
            if(type->byte_size > 4 && type->byte_size%4 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u32s");
              mem->type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U32), reg_byte_count/4, 0);
            }
            if(type->byte_size > 2 && type->byte_size%2 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u16s");
              mem->type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U16), reg_byte_count/2, 0);
            }
            if(type->byte_size > 1)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u8s");
              mem->type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), reg_byte_count, E_TypeFlag_IsNotText);
            }
            if(type->byte_size > 4 && type->byte_size%4 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("f32s");
              mem->type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_F32), reg_byte_count/4, 0);
            }
            if(type->byte_size > 8 && type->byte_size%8 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("f64s");
              mem->type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_F64), reg_byte_count/8, 0);
            }
          }
        }
        
        // rjf: commit members
        type->count = members.count;
        type->members = push_array_no_zero(arena, E_Member, members.count);
        U64 idx = 0;
        for(E_MemberNode *n = members.first; n != 0; n = n->next, idx += 1)
        {
          MemoryCopyStruct(&type->members[idx], &n->v);
        }
        
        scratch_end(scratch);
      }break;
    }
  }
  ProfEnd();
  return type;
}

internal int
e_type_qsort_compare_members_offset(E_Member *a, E_Member *b)
{
  int result = 0;
  if(a->kind < b->kind)
  {
    result = -1;
  }
  else if(a->kind > b->kind)
  {
    result = +1;
  }
  else if(a->off < b->off)
  {
    result = -1;
  }
  else if(a->off > b->off)
  {
    result = +1;
  }
  return result;
}

internal E_MemberArray
e_type_data_members_from_key(Arena *arena, E_TypeKey key)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_TypeKind root_type_kind = e_type_kind_from_key(key);
  
  //- rjf: walk type tree; gather members list
  E_MemberList members_list = {0};
  B32 members_need_offset_sort = 0;
  {
    E_Type *root_type = e_type_from_key(key);
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      U64 base_off;
      E_TypeKeyList inheritance_chain;
      E_TypeKey type_key;
      E_Type *type;
    };
    Task start_task = {0, 0, {0}, key, root_type};
    Task *first_task = &start_task;
    Task *last_task = &start_task;
    for(Task *task = first_task; task != 0; task = task->next)
    {
      E_Type *type = task->type;
      if(type->members != 0)
      {
        U64 last_member_off = 0;
        for(U64 member_idx = 0; member_idx < type->count; member_idx += 1)
        {
          if(type->members[member_idx].name.size != 0 && type->members[member_idx].kind == E_MemberKind_DataField)
          {
            E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
            MemoryCopyStruct(&n->v, &type->members[member_idx]);
            n->v.off += task->base_off;
            n->v.inheritance_key_chain = task->inheritance_chain;
            SLLQueuePush(members_list.first, members_list.last, n);
            members_list.count += 1;
            members_need_offset_sort = members_need_offset_sort || (type->members[member_idx].kind == E_MemberKind_DataField && n->v.off < last_member_off);
            members_need_offset_sort = members_need_offset_sort || (type->members[member_idx].kind != E_MemberKind_DataField);
            last_member_off = n->v.off;
          }
          else if(type->members[member_idx].kind == E_MemberKind_Base)
          {
            Task *t = push_array(scratch.arena, Task, 1);
            t->base_off = type->members[member_idx].off + task->base_off;
            t->inheritance_chain = e_type_key_list_copy(scratch.arena, &task->inheritance_chain);
            e_type_key_list_push(scratch.arena, &t->inheritance_chain, type->members[member_idx].type_key);
            t->type_key = type->members[member_idx].type_key;
            t->type = e_type_from_key(type->members[member_idx].type_key);
            SLLQueuePush(first_task, last_task, t);
            members_need_offset_sort = 1;
          }
        }
      }
    }
  }
  
  //- rjf: convert to array
  E_MemberArray members = {0};
  {
    members.count = members_list.count;
    members.v = push_array(arena, E_Member, members.count);
    U64 idx = 0;
    for(E_MemberNode *n = members_list.first; n != 0; n = n->next)
    {
      MemoryCopyStruct(&members.v[idx], &n->v);
      members.v[idx].name = push_str8_copy(arena, members.v[idx].name);
      members.v[idx].inheritance_key_chain = e_type_key_list_copy(arena, &members.v[idx].inheritance_key_chain);
      idx += 1;
    }
  }
  
  //- rjf: sort array by offset if needed
  if(members_need_offset_sort && (root_type_kind == E_TypeKind_Struct || root_type_kind == E_TypeKind_Class) && key.kind != E_TypeKeyKind_Cons)
  {
    quick_sort(members.v, members.count, sizeof(E_Member), e_type_qsort_compare_members_offset);
  }
  
  //- rjf: find all padding instances
  typedef struct PaddingNode PaddingNode;
  struct PaddingNode
  {
    PaddingNode *next;
    U64 off;
    U64 size;
    U64 prev_member_idx;
  };
  PaddingNode *first_padding = 0;
  PaddingNode *last_padding = 0;
  U64 padding_count = 0;
  if((root_type_kind == E_TypeKind_Struct || root_type_kind == E_TypeKind_Class) && key.kind != E_TypeKeyKind_Cons)
  {
    for(U64 idx = 0; idx < members.count; idx += 1)
    {
      E_Member *member = &members.v[idx];
      if(idx+1 < members.count)
      {
        U64 member_byte_size = e_type_byte_size_from_key(member->type_key);
        Rng1U64 member_byte_range = r1u64(member->off, member->off + member_byte_size);
        if(member[1].off > member_byte_range.max)
        {
          PaddingNode *n = push_array(scratch.arena, PaddingNode, 1);
          SLLQueuePush(first_padding, last_padding, n);
          n->off = member_byte_range.max;
          n->size = member[1].off - member_byte_range.max;
          n->prev_member_idx = idx;
          padding_count += 1;
        }
      }
    }
  }
  
  //- rjf: produce new members array, if we have any padding
  if(padding_count != 0)
  {
    E_MemberArray new_members = {0};
    new_members.count = members.count + padding_count;
    new_members.v = push_array(arena, E_Member, new_members.count);
    MemoryCopy(new_members.v, members.v, sizeof(E_Member)*members.count);
    U64 padding_idx = 0;
    for(PaddingNode *n = first_padding; n != 0; n = n->next)
    {
      if(members.count+padding_idx > n->prev_member_idx+1)
      {
        MemoryCopy(new_members.v + n->prev_member_idx + padding_idx + 2,
                   new_members.v + n->prev_member_idx + padding_idx + 1,
                   sizeof(E_Member) * (members.count + padding_idx - (n->prev_member_idx + padding_idx + 1)));
      }
      E_Member *padding_member = &new_members.v[n->prev_member_idx+padding_idx+1];
      MemoryZeroStruct(padding_member);
      padding_member->kind = E_MemberKind_Padding;
      padding_member->type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), n->size, E_TypeFlag_IsNotText);
      padding_member->off = n->off;
      padding_member->name = push_str8f(arena, "$padding_%I64u", padding_idx);
      padding_idx += 1;
    }
    members = new_members;
  }
  
  scratch_end(scratch);
  return members;
}

internal E_TypeExpandRule *
e_expand_rule_from_type_key(E_TypeKey key)
{
  E_TypeExpandRule *rule = &e_type_expand_rule__default;
  {
    E_Type *type = e_type_from_key(e_type_key_unwrap(key, E_TypeUnwrapFlag_Meta));
    if(type->expand.info != 0)
    {
      rule = &type->expand;
    }
    for(E_Type *lens_type = type;
        lens_type->kind == E_TypeKind_Lens || lens_type->kind == E_TypeKind_Set;
        lens_type = e_type_from_key(e_type_key_unwrap(lens_type->direct_type_key, E_TypeUnwrapFlag_Meta)))
    {
      if(lens_type->expand.info != 0)
      {
        rule = &lens_type->expand;
        break;
      }
    }
  }
  return rule;
}

//- rjf: type key traversal

internal E_TypeKey
e_type_key_direct(E_TypeKey key)
{
  E_TypeKey result = zero_struct;
  switch(key.kind)
  {
    default:{}break;
    case E_TypeKeyKind_Ext:
    case E_TypeKeyKind_Cons:
    {
      E_Type *type = e_type_from_key(key);
      result = type->direct_type_key;
    }break;
  }
  return result;
}

internal E_TypeKey
e_type_key_owner(E_TypeKey key)
{
  E_TypeKey result = zero_struct;
  switch(key.kind)
  {
    default:{}break;
    case E_TypeKeyKind_Ext:
    case E_TypeKeyKind_Cons:
    {
      E_Type *type = e_type_from_key(key);
      result = type->owner_type_key;
    }break;
  }
  return result;
}

internal E_TypeKey
e_type_key_promote(E_TypeKey key)
{
  E_TypeKey result = key;
  E_TypeKind kind = e_type_kind_from_key(key);
  if(kind == E_TypeKind_Bool ||
     kind == E_TypeKind_S8 ||
     kind == E_TypeKind_S16 ||
     kind == E_TypeKind_U8 ||
     kind == E_TypeKind_U16)
  {
    result = e_type_key_basic(E_TypeKind_S32);
  }
  return result;
}

internal E_TypeKey
e_type_key_unwrap(E_TypeKey key, E_TypeUnwrapFlags flags)
{
  E_TypeKey result = key;
  E_TypeKind kind = e_type_kind_from_key(result);
  B32 did_ptr = 0;
  for(;;)
  {
    B32 done = 0;
    switch(kind)
    {
      default:{done = 1;}break;
      case E_TypeKind_Modifier:  {done = !(flags & E_TypeUnwrapFlag_Modifiers);}break;
      case E_TypeKind_Lens:      {done = !(flags & E_TypeUnwrapFlag_Lenses);}break;
      case E_TypeKind_MetaDisplayName:
      case E_TypeKind_MetaDescription:
      case E_TypeKind_MetaExpr:  {done = !(flags & E_TypeUnwrapFlag_Meta);}break;
      case E_TypeKind_Enum:      {done = !(flags & E_TypeUnwrapFlag_Enums);}break;
      case E_TypeKind_Alias:     {done = !(flags & E_TypeUnwrapFlag_Aliases);}break;
      case E_TypeKind_Bitfield:  {done = !(flags & E_TypeUnwrapFlag_Bitfields);}break;
      case E_TypeKind_Array:
      case E_TypeKind_Ptr:
      case E_TypeKind_RRef:
      case E_TypeKind_LRef:
      case E_TypeKind_MemberPtr:
      {
        done = (did_ptr || !(flags & E_TypeUnwrapFlag_Pointers));
        did_ptr = 1;
      }break;
    }
    if(done)
    {
      break;
    }
    result = e_type_key_direct(result);
    kind = e_type_kind_from_key(result);
  }
  return result;
}

//- rjf: type comparisons

internal B32
e_type_match(E_TypeKey l, E_TypeKey r)
{
  // rjf: unpack parameters
  E_TypeKey lu = e_type_key_unwrap(l, E_TypeUnwrapFlag_AllDecorative);
  E_TypeKey ru = e_type_key_unwrap(r, E_TypeUnwrapFlag_AllDecorative);
  
  // rjf: exact key matches -> match
  B32 result = e_type_key_match(lu, ru);
  
  // rjf: if keys don't match, type *contents* could still match,
  // so we need to unpack the type info & compare
  if(!result)
  {
    E_TypeKind luk = e_type_kind_from_key(lu);
    E_TypeKind ruk = e_type_kind_from_key(ru);
    if(luk == ruk)
    {
      switch(luk)
      {
        default:
        {
          if(e_type_kind_is_basic_or_enum(luk))
          {
            result = 1;
          }
          else
          {
            result = 0;
          }
        }break;
        
        case E_TypeKind_Ptr:
        case E_TypeKind_LRef:
        case E_TypeKind_RRef:
        {
          E_TypeKey lud = e_type_key_direct(lu);
          E_TypeKey rud = e_type_key_direct(ru);
          result = e_type_match(lud, rud);
        }break;
        
        case E_TypeKind_MemberPtr:
        {
          E_TypeKey lud = e_type_key_direct(lu);
          E_TypeKey rud = e_type_key_direct(ru);
          E_TypeKey luo = e_type_key_owner(lu);
          E_TypeKey ruo = e_type_key_owner(ru);
          result = (e_type_match(lud, rud) && e_type_match(luo, ruo));
        }break;
        
        case E_TypeKind_Array:
        {
          E_Type *lt = e_type_from_key(l);
          E_Type *rt = e_type_from_key(r);
          if(lt->count == rt->count && e_type_match(lt->direct_type_key, rt->direct_type_key))
          {
            result = 1;
          }
        }break;
        
        case E_TypeKind_Function:
        {
          E_Type *lt = e_type_from_key(l);
          E_Type *rt = e_type_from_key(r);
          if(lt->count == rt->count && e_type_match(lt->direct_type_key, rt->direct_type_key))
          {
            B32 params_match = 1;
            E_TypeKey *lp = lt->param_type_keys;
            E_TypeKey *rp = rt->param_type_keys;
            U64 count = lt->count;
            for(U64 i = 0; i < count; i += 1, lp += 1, rp += 1)
            {
              if(!e_type_match(*lp, *rp))
              {
                params_match = 0;
                break;
              }
            }
            result = params_match;
          }
        }break;
        
        case E_TypeKind_Method:
        {
          E_Type *lt = e_type_from_key(l);
          E_Type *rt = e_type_from_key(r);
          if(lt->count == rt->count &&
             e_type_match(lt->direct_type_key, rt->direct_type_key) &&
             e_type_match(lt->owner_type_key, rt->owner_type_key))
          {
            B32 params_match = 1;
            E_TypeKey *lp = lt->param_type_keys;
            E_TypeKey *rp = rt->param_type_keys;
            U64 count = lt->count;
            for(U64 i = 0; i < count; i += 1, lp += 1, rp += 1)
            {
              if(!e_type_match(*lp, *rp))
              {
                params_match = 0;
                break;
              }
            }
            result = params_match;
          }
        }break;
      }
    }
  }
  
  return result;
}

//- rjf: key -> string

internal void
e_type_lhs_string_from_key(Arena *arena, E_TypeKey key, String8List *out, U32 prec, B32 skip_return)
{
  String8 keyword = {0};
  E_TypeKind kind = e_type_kind_from_key(key);
  switch(kind)
  {
    default:
    {
      E_Type *type = e_type_from_key(key);
      str8_list_pushf(arena, out, "%S ", type->name);
    }break;
    
    case E_TypeKind_Bitfield:
    {
      E_Type *type = e_type_from_key(key);
      e_type_lhs_string_from_key(arena, type->direct_type_key, out, prec, skip_return);
      str8_list_pushf(arena, out, ": %I64u", type->count);
    }break;
    
    case E_TypeKind_Modifier:
    {
      E_Type *type = e_type_from_key(key);
      E_TypeKey direct = type->direct_type_key;
      e_type_lhs_string_from_key(arena, direct, out, 1, skip_return);
      if(type->flags & E_TypeFlag_Const)
      {
        str8_list_push(arena, out, str8_lit("const "));
      }
      if(type->flags & E_TypeFlag_Volatile)
      {
        str8_list_push(arena, out, str8_lit("volatile "));
      }
      if(type->flags & E_TypeFlag_Restrict)
      {
        str8_list_push(arena, out, str8_lit("restrict "));
      }
    }break;
    
    case E_TypeKind_Variadic:
    {
      str8_list_push(arena, out, str8_lit("..."));
    }break;
    
    case E_TypeKind_Struct:
    case E_TypeKind_Union:
    case E_TypeKind_Enum:
    case E_TypeKind_Class:
    case E_TypeKind_Alias:
    {
      E_Type *type = e_type_from_key(key);
      str8_list_pushf(arena, out, "%S ", type->name);
    }break;
    
    case E_TypeKind_IncompleteStruct: keyword = str8_lit("struct"); goto fwd_udt;
    case E_TypeKind_IncompleteUnion:  keyword = str8_lit("union"); goto fwd_udt;
    case E_TypeKind_IncompleteEnum:   keyword = str8_lit("enum"); goto fwd_udt;
    case E_TypeKind_IncompleteClass:  keyword = str8_lit("class"); goto fwd_udt;
    fwd_udt:;
    {
      E_Type *type = e_type_from_key(key);
      str8_list_push(arena, out, keyword);
      str8_list_push(arena, out, str8_lit(" "));
      str8_list_pushf(arena, out, "%S ", type->name);
    }break;
    
    case E_TypeKind_Array:
    {
      E_TypeKey direct = e_type_key_direct(key);
      e_type_lhs_string_from_key(arena, direct, out, 2, skip_return);
      if(prec == 1)
      {
        str8_list_push(arena, out, str8_lit("("));
      }
    }break;
    
    case E_TypeKind_Function:
    {
      if(!skip_return)
      {
        E_TypeKey direct = e_type_key_direct(key);
        e_type_lhs_string_from_key(arena, direct, out, 2, 0);
      }
      if(prec == 1)
      {
        str8_list_push(arena, out, str8_lit("("));
      }
    }break;
    
    case E_TypeKind_Lens:
    {
      E_Type *type = e_type_from_key(key);
      str8_list_pushf(arena, out, "%S(", type->name);
      E_TypeKey direct = e_type_key_direct(key);
      String8 direct_string = e_type_string_from_key(arena, direct);
      str8_list_push(arena, out, direct_string);
      for EachIndex(idx, type->count)
      {
        String8 string = e_string_from_expr(arena, type->args[idx], str8_zero());
        str8_list_pushf(arena, out, ", ");
        str8_list_push(arena, out, string);
      }
      str8_list_pushf(arena, out, ")");
    }break;
    
    case E_TypeKind_Ptr:
    {
      E_TypeKey direct = e_type_key_direct(key);
      e_type_lhs_string_from_key(arena, direct, out, 1, skip_return);
      str8_list_push(arena, out, str8_lit("*"));
      E_Type *type = e_type_from_key(key);
      if(type->count != 1)
      {
        str8_list_pushf(arena, out, ".%I64u", type->count);
      }
    }break;
    
    case E_TypeKind_LRef:
    {
      E_TypeKey direct = e_type_key_direct(key);
      e_type_lhs_string_from_key(arena, direct, out, 1, skip_return);
      str8_list_push(arena, out, str8_lit("&"));
    }break;
    
    case E_TypeKind_RRef:
    {
      E_TypeKey direct = e_type_key_direct(key);
      e_type_lhs_string_from_key(arena, direct, out, 1, skip_return);
      str8_list_push(arena, out, str8_lit("&&"));
    }break;
    
    case E_TypeKind_MemberPtr:
    {
      E_Type *type = e_type_from_key(key);
      E_TypeKey direct = type->direct_type_key;
      e_type_lhs_string_from_key(arena, direct, out, 1, skip_return);
      E_Type *container = e_type_from_key(type->owner_type_key);
      if(container->kind != E_TypeKind_Null)
      {
        str8_list_push(arena, out, push_str8_copy(arena, container->name));
      }
      else
      {
        str8_list_push(arena, out, str8_lit("<unknown-class>"));
      }
      str8_list_push(arena, out, str8_lit("::*"));
    }break;
    
    case E_TypeKind_MetaExpr:
    case E_TypeKind_MetaDisplayName:
    case E_TypeKind_MetaDescription:
    {
      E_TypeKey direct = e_type_key_direct(key);
      e_type_lhs_string_from_key(arena, direct, out, prec, skip_return);
    }break;
  }
}

internal void
e_type_rhs_string_from_key(Arena *arena, E_TypeKey key, String8List *out, U32 prec)
{
  E_TypeKind kind = e_type_kind_from_key(key);
  switch(kind)
  {
    default:{}break;
    
    case E_TypeKind_Bitfield:
    {
      E_TypeKey direct = e_type_key_direct(key);
      e_type_rhs_string_from_key(arena, direct, out, prec);
    }break;
    
    case E_TypeKind_Modifier:
    case E_TypeKind_Ptr:
    case E_TypeKind_LRef:
    case E_TypeKind_RRef:
    case E_TypeKind_MemberPtr:
    {
      E_TypeKey direct = e_type_key_direct(key);
      e_type_rhs_string_from_key(arena, direct, out, 1);
    }break;
    
    case E_TypeKind_Array:
    {
      E_Type *type = e_type_from_key(key);
      if(prec == 1)
      {
        str8_list_push(arena, out, str8_lit(")"));
      }
      String8 count_str = str8_from_u64(arena, type->count, 10, 0, 0);
      str8_list_push(arena, out, str8_lit("["));
      str8_list_push(arena, out, count_str);
      str8_list_push(arena, out, str8_lit("]"));
      E_TypeKey direct = e_type_key_direct(key);
      e_type_rhs_string_from_key(arena, direct, out, 2);
    }break;
    
    case E_TypeKind_Function:
    {
      E_Type *type = e_type_from_key(key);
      if(prec == 1)
      {
        str8_list_push(arena, out, str8_lit(")"));
      }
      if(type->count == 0)
      {
        str8_list_push(arena, out, str8_lit("(void)"));
      }
      else
      {
        str8_list_push(arena, out, str8_lit("("));
        U64 param_count = type->count;
        E_TypeKey *param_type_keys = type->param_type_keys;
        for(U64 param_idx = 0; param_idx < param_count; param_idx += 1)
        {
          E_TypeKey param_type_key = param_type_keys[param_idx];
          String8 param_str = e_type_string_from_key(arena, param_type_key);
          String8 param_str_trimmed = str8_skip_chop_whitespace(param_str);
          str8_list_push(arena, out, param_str_trimmed);
          if(param_idx+1 < param_count)
          {
            str8_list_push(arena, out, str8_lit(", "));
          }
        }
        str8_list_push(arena, out, str8_lit(")"));
      }
      E_TypeKey direct = e_type_key_direct(key);
      e_type_rhs_string_from_key(arena, direct, out, 2);
    }break;
    
    case E_TypeKind_MetaExpr:
    {
      E_TypeKey direct = e_type_key_direct(key);
      e_type_rhs_string_from_key(arena, direct, out, prec);
    }break;
  }
}

internal String8
e_type_string_from_key(Arena *arena, E_TypeKey key)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  e_type_lhs_string_from_key(scratch.arena, key, &list, 0, 0);
  e_type_rhs_string_from_key(scratch.arena, key, &list, 0);
  String8 result = str8_list_join(arena, &list, 0);
  result = str8_skip_chop_whitespace(result);
  scratch_end(scratch);
  return result;
}

internal E_TypeKey
e_default_expansion_type_from_key(E_TypeKey root_key)
{
  E_TypeKey type_key = zero_struct;
  B32 hit_1ptr = 0;
  for(E_TypeKey key = root_key;
      !e_type_key_match(e_type_key_zero(), key);
      key = e_type_key_direct(key))
  {
    B32 done = 1;
    E_TypeKind kind = e_type_kind_from_key(key);
    
    //- rjf: if we have pointers which point to a single thing (count = 1),
    // or we have a lens, or we have a modifier node, then we will defer to
    // the next type in the chain.
    //
    // if this pointer points to N things (count > 1), then we can use it for
    // array-like expansion.
    //
    if(e_type_kind_is_pointer_or_ref(kind))
    {
      E_Type *type = e_type_from_key(key);
      if(!e_type_key_match(e_type_key_basic(E_TypeKind_Void), type->direct_type_key))
      {
        if(type->count == 1 && hit_1ptr)
        {
          type_key = key;
        }
        else if(type->count == 1 && !hit_1ptr)
        {
          hit_1ptr = 1;
          done = 0;
        }
        else if(type->count > 1)
        {
          type_key = key;
        }
      }
    }
    
    //- rjf: if we have lenses or modifiers in the type chain, then we will
    // defer to the next type in the chain.
    //
    // NOTE(rjf): while it may seem like a lens type needs to do something
    // different, because lenses sometimes want to define their own expansion
    // rules, they would've redirected to an entirely different expansion
    // hook. if we are in the default expansion hook, then the lenses do not
    // impact the expansion at all (e.g. they are for other cosmetic things,
    // like visualizers or integer radix changes), and so in that case we
    // want to ignore them.
    //
    else if(kind == E_TypeKind_Lens ||
            kind == E_TypeKind_Modifier)
    {
      done = 0;
    }
    
    //- rjf: if we have meta-expression tags in the type chain, defer
    // to the next type in the chain.
    else if(E_TypeKind_FirstMeta <= kind && kind <= E_TypeKind_LastMeta)
    {
      done = 0;
    }
    
    //- rjf: if we've reached a struct-like, then we can use that for
    // struct-like expansion.
    else if(kind == E_TypeKind_Struct ||
            kind == E_TypeKind_Union ||
            kind == E_TypeKind_Class ||
            kind == E_TypeKind_Set)
    {
      type_key = key;
    }
    
    //- rjf: if we've reached an enum-like, then we can use that for
    // enum-like expansion.
    else if(kind == E_TypeKind_Enum)
    {
      type_key = key;
    }
    
    //- rjf: if we've reached an array, then we can use that for array-like
    // expansion.
    else if(kind == E_TypeKind_Array)
    {
      type_key = key;
    }
    
    //- rjf: if we're done, then just break.
    if(done)
    {
      break;
    }
  }
  
  return type_key;
}

////////////////////////////////
//~ rjf: Cache Lookups

internal E_Type *
e_type_from_key(E_TypeKey key)
{
  E_Type *type = &e_type_nil;
  {
    U64 hash = e_hash_from_string(5381, str8_struct(&key));
    U64 slot_idx = hash%e_cache->type_cache_slots_count;
    E_TypeCacheNode *node = 0;
    for(E_TypeCacheNode *n = e_cache->type_cache_slots[slot_idx].first; n != 0; n = n->next)
    {
      if(e_type_key_match(key, n->key))
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      node = push_array(e_cache->arena, E_TypeCacheNode, 1);
      node->key = key;
      node->type = e_push_type_from_key(e_cache->arena, key);
      SLLQueuePush(e_cache->type_cache_slots[slot_idx].first, e_cache->type_cache_slots[slot_idx].last, node);
    }
    type = node->type;
  }
  return type;
}

//- rjf: member lookups

internal E_MemberCacheNode *
e_member_cache_node_from_type_key(E_TypeKey key)
{
  U64 hash = e_hash_from_string(5381, str8_struct(&key));
  U64 slot_idx = hash%e_cache->member_cache_slots_count;
  E_MemberCacheSlot *slot = &e_cache->member_cache_slots[slot_idx];
  E_MemberCacheNode *node = 0;
  for(E_MemberCacheNode *n = slot->first; n != 0; n = n->next)
  {
    if(e_type_key_match(n->key, key))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    node = push_array(e_cache->arena, E_MemberCacheNode, 1);
    SLLQueuePush(slot->first, slot->last, node);
    node->key = key;
    node->members = e_type_data_members_from_key(e_cache->arena, key);
    node->member_hash_slots_count = node->members.count;
    node->member_hash_slots = push_array(e_cache->arena, E_MemberHashSlot, node->member_hash_slots_count);
    node->member_filter_slots_count = 16;
    node->member_filter_slots = push_array(e_cache->arena, E_MemberFilterSlot, node->member_filter_slots_count);
    for EachIndex(idx, node->members.count)
    {
      U64 hash = e_hash_from_string(5381, node->members.v[idx].name);
      U64 slot_idx = hash%node->member_hash_slots_count;
      E_MemberHashNode *n = push_array(e_cache->arena, E_MemberHashNode, 1);
      SLLQueuePush(node->member_hash_slots[slot_idx].first, node->member_hash_slots[slot_idx].last, n);
      n->member_idx = idx;
    }
  }
  return node;
}

internal E_MemberArray
e_type_data_members_from_key_filter__cached(E_TypeKey key, String8 filter)
{
  E_MemberArray members = {0};
  E_MemberCacheNode *node = e_member_cache_node_from_type_key(key);
  if(node != 0)
  {
    if(filter.size == 0)
    {
      members = node->members;
    }
    else
    {
      U64 hash = e_hash_from_string(5381, filter);
      U64 slot_idx = hash%node->member_filter_slots_count;
      E_MemberFilterSlot *slot = &node->member_filter_slots[slot_idx];
      E_MemberFilterNode *filter_node = 0;
      for(E_MemberFilterNode *n = slot->first; n != 0; n = n->next)
      {
        if(str8_match(n->filter, filter, 0))
        {
          filter_node = n;
          break;
        }
      }
      if(filter_node == 0)
      {
        Temp scratch = scratch_begin(0, 0);
        filter_node = push_array(e_cache->arena, E_MemberFilterNode, 1);
        filter_node->filter = push_str8_copy(e_cache->arena, filter);
        E_MemberList member_list__filtered = {0};
        for EachIndex(idx, node->members.count)
        {
          E_Member *member = &node->members.v[idx];
          FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, member->name);
          if(matches.count == matches.needle_part_count)
          {
            e_member_list_push(scratch.arena, &member_list__filtered, member);
          }
        }
        filter_node->members_filtered = e_member_array_from_list(e_cache->arena, &member_list__filtered);
        scratch_end(scratch);
      }
      members = filter_node->members_filtered;
    }
  }
  return members;
}

internal E_MemberArray
e_type_data_members_from_key__cached(E_TypeKey key)
{
  E_MemberArray members = {0};
  E_MemberCacheNode *node = e_member_cache_node_from_type_key(key);
  if(node != 0)
  {
    members = node->members;
  }
  return members;
}

internal E_Member
e_type_member_from_key_name__cached(E_TypeKey key, String8 name)
{
  E_Member result = {0};
  E_MemberCacheNode *node = e_member_cache_node_from_type_key(key);
  if(node != 0 && node->member_hash_slots_count != 0)
  {
    U64 hash = e_hash_from_string(5381, name);
    U64 slot_idx = hash%node->member_hash_slots_count;
    for(E_MemberHashNode *n = node->member_hash_slots[slot_idx].first; n != 0; n = n->next)
    {
      if(str8_match(node->members.v[n->member_idx].name, name, 0))
      {
        result = node->members.v[n->member_idx];
        break;
      }
    }
  }
  return result;
}

//- rjf: enum val lookups

internal E_EnumValCacheNode *
e_enum_val_cache_node_from_type_key(E_TypeKey key)
{
  U64 hash = e_hash_from_string(5381, str8_struct(&key));
  U64 slot_idx = hash%e_cache->enum_val_cache_slots_count;
  E_EnumValCacheSlot *slot = &e_cache->enum_val_cache_slots[slot_idx];
  E_EnumValCacheNode *node = 0;
  for(E_EnumValCacheNode *n = slot->first; n != 0; n = n->next)
  {
    if(e_type_key_match(n->key, key))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    node = push_array(e_cache->arena, E_EnumValCacheNode, 1);
    SLLQueuePush(slot->first, slot->last, node);
    node->key = key;
    E_Type *type = e_type_from_key(key);
    if(type->kind == E_TypeKind_Enum)
    {
      node->val_hash_slots_count = type->count;
      node->val_hash_slots = push_array(e_cache->arena, E_EnumValHashSlot, node->val_hash_slots_count);
      node->val_filter_slots_count = 16;
      node->val_filter_slots = push_array(e_cache->arena, E_EnumValFilterSlot, node->val_filter_slots_count);
      for EachIndex(idx, type->count)
      {
        U64 hash = e_hash_from_string(5381, type->enum_vals[idx].name);
        U64 slot_idx = hash%node->val_hash_slots_count;
        E_EnumValHashNode *n = push_array(e_cache->arena, E_EnumValHashNode, 1);
        SLLQueuePush(node->val_hash_slots[slot_idx].first, node->val_hash_slots[slot_idx].last, n);
        n->val_idx = idx;
      }
    }
  }
  return node;
}

internal E_EnumValArray
e_type_enum_vals_from_key_filter__cached(E_TypeKey key, String8 filter)
{
  E_EnumValArray enum_vals = {0};
  E_EnumValCacheNode *node = e_enum_val_cache_node_from_type_key(key);
  if(node != 0)
  {
    if(filter.size == 0)
    {
      E_Type *type = e_type_from_key(key);
      if(type->kind == E_TypeKind_Enum)
      {
        enum_vals.v = type->enum_vals;
        enum_vals.count = type->count;
      }
    }
    else
    {
      U64 hash = e_hash_from_string(5381, filter);
      U64 slot_idx = hash%node->val_filter_slots_count;
      E_EnumValFilterSlot *slot = &node->val_filter_slots[slot_idx];
      E_EnumValFilterNode *filter_node = 0;
      for(E_EnumValFilterNode *n = slot->first; n != 0; n = n->next)
      {
        if(str8_match(n->filter, filter, 0))
        {
          filter_node = n;
          break;
        }
      }
      if(filter_node == 0)
      {
        Temp scratch = scratch_begin(0, 0);
        filter_node = push_array(e_cache->arena, E_EnumValFilterNode, 1);
        filter_node->filter = push_str8_copy(e_cache->arena, filter);
        E_Type *type = e_type_from_key(key);
        E_EnumValList enum_val_list__filtered = {0};
        if(type->kind == E_TypeKind_Enum)
        {
          for EachIndex(idx, type->count)
          {
            E_EnumVal *enum_val = &type->enum_vals[idx];
            FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, enum_val->name);
            if(matches.count == matches.needle_part_count)
            {
              e_enum_val_list_push(scratch.arena, &enum_val_list__filtered, enum_val);
            }
          }
        }
        filter_node->vals_filtered = e_enum_val_array_from_list(e_cache->arena, &enum_val_list__filtered);
        scratch_end(scratch);
      }
      enum_vals = filter_node->vals_filtered;
    }
  }
  return enum_vals;
}

internal E_EnumValArray
e_type_enum_vals_from_key__cached(E_TypeKey key)
{
  E_EnumValArray enum_vals = e_type_enum_vals_from_key_filter__cached(key, str8_zero());
  return enum_vals;
}

internal E_EnumVal
e_type_enum_val_from_key_name__cached(E_TypeKey key, String8 name)
{
  E_EnumVal result = {0};
  E_EnumValCacheNode *node = e_enum_val_cache_node_from_type_key(key);
  if(node != 0 && node->val_hash_slots_count != 0)
  {
    Temp scratch = scratch_begin(0, 0);
    E_Type *type = e_type_from_key(key);
    String8 name_qualified_0 = push_str8f(scratch.arena, "%S%S", type->name, name);
    String8 name_qualified_1 = push_str8f(scratch.arena, "%S_%S", type->name, name);
    U64 hash = e_hash_from_string(5381, name);
    U64 slot_idx = hash%node->val_hash_slots_count;
    for(E_EnumValHashNode *n = node->val_hash_slots[slot_idx].first; n != 0; n = n->next)
    {
      if(str8_match(type->enum_vals[n->val_idx].name, name, 0) ||
         str8_match(type->enum_vals[n->val_idx].name, name_qualified_0, 0) ||
         str8_match(type->enum_vals[n->val_idx].name, name_qualified_1, 0))
      {
        result = type->enum_vals[n->val_idx];
        break;
      }
    }
    scratch_end(scratch);
  }
  return result;
}

////////////////////////////////
//~ rjf: (Built-In Type Hooks) Default Hooks

E_TYPE_EXPAND_INFO_FUNCTION_DEF(default)
{
  E_TypeExpandInfo result = {0};
  {
    //- rjf: try to extract a struct-like type key, enum-like, or array-like
    // type key, for expansion
    E_TypeKey expand_type_key = e_default_expansion_type_from_key(eval.irtree.type_key);
    
    //- rjf: struct type? -> use the struct type for expansion
    B32 did_expansion = 0;
    if(!did_expansion)
    {
      E_TypeKind struct_type_kind = e_type_kind_from_key(expand_type_key);
      if(struct_type_kind == E_TypeKind_Struct ||
         struct_type_kind == E_TypeKind_Class ||
         struct_type_kind == E_TypeKind_Union)
      {
        E_MemberArray data_members = e_type_data_members_from_key_filter__cached(expand_type_key, filter);
        result.expr_count = data_members.count;
        did_expansion = 1;
      }
    }
    
    //- rjf: array-like type? -> use the array-like for expansion
    if(!did_expansion)
    {
      E_TypeKind array_type_kind = e_type_kind_from_key(expand_type_key);
      if(array_type_kind == E_TypeKind_Array ||
         array_type_kind == E_TypeKind_Ptr ||
         array_type_kind == E_TypeKind_RRef ||
         array_type_kind == E_TypeKind_LRef)
      {
        E_Type *array_type = e_type_from_key(expand_type_key);
        result.expr_count = array_type->count;
        did_expansion = 1;
      }
    }
    
    //- rjf: enum-like type? -> use the enum-like for expansion
    if(!did_expansion)
    {
      E_TypeKind enum_type_kind = e_type_kind_from_key(expand_type_key);
      if(enum_type_kind == E_TypeKind_Enum)
      {
        E_EnumValArray enum_vals = e_type_enum_vals_from_key_filter__cached(expand_type_key, filter);
        result.expr_count = enum_vals.count;
        did_expansion = 1;
      }
    }
  }
  return result;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(default)
{
  Temp scratch = scratch_begin(&arena, 1);
  {
    //- rjf: try to extract a struct-like type key, enum-like, or array-like
    // type key, for expansion
    E_TypeKey expand_type_key = e_default_expansion_type_from_key(eval.irtree.type_key);
    E_TypeKind expand_type_kind = e_type_kind_from_key(expand_type_key);
    
    //- rjf: struct case -> the lookup-range will return a range of members
    if(expand_type_kind == E_TypeKind_Struct ||
       expand_type_kind == E_TypeKind_Class ||
       expand_type_kind == E_TypeKind_Union)
    {
      E_MemberArray data_members = e_type_data_members_from_key_filter__cached(expand_type_key, filter);
      Rng1U64 legal_idx_range = r1u64(0, data_members.count);
      Rng1U64 read_range = intersect_1u64(legal_idx_range, idx_range);
      U64 read_range_count = dim_1u64(read_range);
      for(U64 idx = 0; idx < read_range_count; idx += 1)
      {
        U64 member_idx = idx + read_range.min;
        String8 member_name = data_members.v[member_idx].name;
        evals_out[idx] = e_eval_wrapf(eval, "$.%S", member_name);
      }
    }
    
    //- rjf: enum case -> the lookup-range will return a range of enum constants
    else if(expand_type_kind == E_TypeKind_Enum)
    {
      E_Type *type = e_type_from_key(expand_type_key);
      E_EnumValArray enum_vals = e_type_enum_vals_from_key_filter__cached(expand_type_key, filter);
      Rng1U64 legal_idx_range = r1u64(0, enum_vals.count);
      Rng1U64 read_range = intersect_1u64(legal_idx_range, idx_range);
      U64 read_range_count = dim_1u64(read_range);
      for(U64 idx = 0; idx < read_range_count; idx += 1)
      {
        U64 val_idx = idx + read_range.min;
        String8 member_name = enum_vals.v[val_idx].name;
        String8 sufficient_suffix = member_name;
        if(str8_match(sufficient_suffix, type->name, StringMatchFlag_RightSideSloppy))
        {
          sufficient_suffix = str8_skip(sufficient_suffix, type->name.size);
          if(str8_match(sufficient_suffix, str8_lit("_"), StringMatchFlag_RightSideSloppy))
          {
            sufficient_suffix = str8_skip(sufficient_suffix, 1);
          }
        }
        evals_out[idx] = e_eval_wrapf(eval, "$.%S", sufficient_suffix);
      }
    }
    
    //- rjf: ptr case -> the lookup-range will return a range of dereferences
    else if(expand_type_kind == E_TypeKind_Ptr ||
            expand_type_kind == E_TypeKind_LRef ||
            expand_type_kind == E_TypeKind_RRef ||
            expand_type_kind == E_TypeKind_Array ||
            expand_type_kind == E_TypeKind_Set)
    {
      U64 read_range_count = dim_1u64(idx_range);
      for(U64 idx = 0; idx < read_range_count; idx += 1)
      {
        evals_out[idx] = e_eval_wrapf(eval, "$[%I64u]", idx_range.min + idx);
      }
    }
  }
  scratch_end(scratch);
}

E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(identity)
{
  return num;
}

E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(identity)
{
  return id;
}

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `rows` lens

typedef struct E_RowsAccel E_RowsAccel;
struct E_RowsAccel
{
  E_Eval *root_evals;
  Rng1U64 *root_evals_ranges;
};

E_TYPE_EXPAND_INFO_FUNCTION_DEF(rows)
{
  E_Type *type = e_type_from_key(eval.irtree.type_key);
  E_RowsAccel *accel = push_array(arena, E_RowsAccel, 1);
  accel->root_evals = push_array(arena, E_Eval, type->count);
  accel->root_evals_ranges = push_array(arena, Rng1U64, type->count);
  E_ParentKey(eval.key)
  {
    for EachIndex(idx, type->count)
    {
      accel->root_evals[idx] = e_eval_from_expr(type->args[idx]);
      accel->root_evals_ranges[idx] = r1u64(idx, idx+1);
    }
  }
  E_TypeExpandInfo info = {accel, type->count};
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(rows)
{
  E_RowsAccel *accel = (E_RowsAccel *)user_data;
  U64 out_idx = 0;
  for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, out_idx += 1)
  {
    evals_out[out_idx] = accel->root_evals[idx];
  }
}

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `omit` lens

E_TYPE_EXPAND_INFO_FUNCTION_DEF(omit)
{
  E_Type *type = e_type_from_key(eval.irtree.type_key);
  String8Array allowed_children_array = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List allowed_children = {0};
    {
      E_Eval eval_stripped = e_eval_wrapf(eval, "q:raw($)");
      E_TypeExpandRule *expand_rule = e_expand_rule_from_type_key(eval_stripped.irtree.type_key);
      E_TypeExpandInfo expand_info = expand_rule->info(scratch.arena, eval_stripped, filter);
      if(expand_info.expr_count < 4096)
      {
        E_Eval *evals = push_array(scratch.arena, E_Eval, expand_info.expr_count);
        for EachIndex(idx, expand_info.expr_count)
        {
          evals[idx] = e_eval_nil;
        }
        expand_rule->range(scratch.arena, expand_info.user_data, eval_stripped, filter, r1u64(0, expand_info.expr_count), evals);
        for EachIndex(idx, expand_info.expr_count)
        {
          if(evals[idx].expr->kind == E_ExprKind_MemberAccess)
          {
            String8 name = evals[idx].expr->first->next->string;
            B32 name_is_allowed = 1;
            for EachIndex(arg_idx, type->count)
            {
              if(str8_match(type->args[arg_idx]->string, name, 0))
              {
                name_is_allowed = 0;
                break;
              }
            }
            if(name_is_allowed)
            {
              str8_list_push(scratch.arena, &allowed_children, push_str8_copy(arena, name));
            }
          }
        }
      }
    }
    allowed_children_array = str8_array_from_list(arena, &allowed_children);
    scratch_end(scratch);
  }
  String8Array *ext = push_array(arena, String8Array, 1);
  *ext = allowed_children_array;
  E_TypeExpandInfo info = {ext, ext->count};
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(omit)
{
  String8Array *ext = (String8Array *)user_data;
  U64 out_idx = 0;
  for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, out_idx += 1)
  {
    String8 name = ext->v[idx];
    if(name.size != 0)
    {
      evals_out[out_idx] = e_eval_wrapf(eval, "$.%S", name);
    }
  }
}

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `sequence` lens

E_TYPE_EXPAND_INFO_FUNCTION_DEF(sequence)
{
  E_TypeExpandInfo info = {0, e_value_eval_from_eval(eval).value.u64};
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(sequence)
{
  U64 read_range_count = dim_1u64(idx_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    evals_out[idx] = e_eval_from_stringf("%I64u", idx_range.min + idx);
  }
}

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `array` lens

E_TYPE_EXPAND_INFO_FUNCTION_DEF(array)
{
  E_Type *type = e_type_from_key(eval.irtree.type_key);
  U64 count = 1;
  if(type->args != 0 && type->count > 0) E_ParentKey(eval.key)
  {
    E_Key count_key = e_key_from_expr(type->args[0]);
    E_Value count_value = e_value_from_key(count_key);
    count = count_value.u64;
  }
  E_TypeExpandInfo info = {0, count};
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(array)
{
  U64 read_range_count = dim_1u64(idx_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    evals_out[idx] = e_eval_wrapf(eval, "$[%I64u]", idx_range.min + idx);
  }
}

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `slice` lens

typedef struct E_SliceIRExt E_SliceIRExt;
struct E_SliceIRExt
{
  E_Member *base_ptr_member;
  E_Member *opl_ptr_member;
  E_Member *count_member;
};

typedef struct E_SliceAccel E_SliceAccel;
struct E_SliceAccel
{
  Arch arch;
  U64 count;
  U64 base_ptr_vaddr;
  E_TypeKey element_type_key;
};

E_TYPE_IREXT_FUNCTION_DEF(slice)
{
  E_SliceIRExt *ext = push_array(arena, E_SliceIRExt, 1);
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    // rjf: build info from struct type
    E_TypeKey struct_type_key = e_type_key_unwrap(irtree->type_key, E_TypeUnwrapFlag_All);
    E_TypeKind type_kind = e_type_kind_from_key(struct_type_key);
    if(type_kind == E_TypeKind_Struct || type_kind == E_TypeKind_Class)
    {
      // rjf: unpack members
      E_MemberArray members = e_type_data_members_from_key__cached(struct_type_key);
      
      // rjf: choose base pointer & count members
      E_Member *base_ptr_member = 0;
      E_Member *opl_ptr_member = 0;
      E_Member *count_member = 0;
      for(U64 idx = 0; idx < members.count; idx += 1)
      {
        E_Member *member = &members.v[idx];
        E_TypeKey member_type = e_type_key_unwrap(member->type_key, E_TypeUnwrapFlag_AllDecorative);
        E_TypeKind member_type_kind = e_type_kind_from_key(member_type);
        if(count_member == 0 && e_type_kind_is_integer(member_type_kind))
        {
          count_member = member;
        }
        if(base_ptr_member == 0 && e_type_kind_is_pointer_or_ref(member_type_kind))
        {
          base_ptr_member = &members.v[idx];
        }
        else if(base_ptr_member != 0 && opl_ptr_member == 0 && e_type_kind_is_pointer_or_ref(member_type_kind))
        {
          opl_ptr_member = &members.v[idx];
        }
        if(count_member != 0 && base_ptr_member != 0)
        {
          break;
        }
        else if(base_ptr_member != 0 && opl_ptr_member != 0)
        {
          break;
        }
      }
      
      // rjf: fill extension
      ext->base_ptr_member = base_ptr_member;
      ext->opl_ptr_member = opl_ptr_member;
      ext->count_member = count_member;
    }
    scratch_end(scratch);
  }
  E_IRExt result = {ext};
  return result;
}

E_TYPE_ACCESS_FUNCTION_DEF(slice)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  E_SliceIRExt *ext = (E_SliceIRExt *)lhs_irtree->user_data;
  switch(expr->kind)
  {
    default:
    case E_ExprKind_MemberAccess:
    {
      result = E_TYPE_ACCESS_FUNCTION_NAME(default)(arena, overridden, expr, lhs_irtree);
    }break;
    case E_ExprKind_ArrayIndex:
    if(ext->base_ptr_member != 0)
    {
      Temp scratch = scratch_begin(&arena, 1);
      
      // rjf: compute ir tree for struct base
      E_IRNode *struct_base_tree = &e_irnode_nil;
      {
        E_OpList lhs_oplist = e_oplist_from_irtree(scratch.arena, lhs_irtree->root);
        String8 lhs_bytecode = e_bytecode_from_oplist(arena, &lhs_oplist);
        struct_base_tree = e_irtree_bytecode_no_copy(arena, lhs_bytecode);
        if(e_type_kind_is_pointer_or_ref(e_type_kind_from_key(e_type_key_unwrap(lhs_irtree->type_key, E_TypeUnwrapFlag_AllDecorative))))
        {
          struct_base_tree = e_irtree_resolve_to_value(arena, lhs_irtree->mode, lhs_irtree->root, lhs_irtree->type_key);
        }
      }
      
      // rjf: compute ir tree for base pointer value calculation
      E_IRNode *base_ptr_tree = &e_irnode_nil;
      if(struct_base_tree != &e_irnode_nil)
      {
        base_ptr_tree = struct_base_tree;
        if(ext->base_ptr_member->off != 0)
        {
          base_ptr_tree = e_irtree_binary_op_u(arena, RDI_EvalOp_Add, struct_base_tree, e_irtree_const_u(arena, ext->base_ptr_member->off));
        }
        base_ptr_tree = e_irtree_mem_read_type(arena, base_ptr_tree, ext->base_ptr_member->type_key);
      }
      
      // rjf: compute ir tree for adding to the base ptr member
      E_IRNode *idxed_base_tree = &e_irnode_nil;
      if(base_ptr_tree != &e_irnode_nil)
      {
        E_IRTreeAndType idx_irtree = e_push_irtree_and_type_from_expr(arena, 0, &e_default_identifier_resolution_rule, 0, 1, expr->first->next);
        E_IRNode *idx_root = e_irtree_resolve_to_value(arena, idx_irtree.mode, idx_irtree.root, idx_irtree.type_key);
        E_IRNode *off_root = e_irtree_binary_op_u(arena, RDI_EvalOp_Mul, idx_root, e_irtree_const_u(arena, e_type_byte_size_from_key(e_type_key_unwrap(ext->base_ptr_member->type_key, E_TypeUnwrapFlag_All))));
        idxed_base_tree = e_irtree_binary_op_u(arena, RDI_EvalOp_Add, base_ptr_tree, off_root);
      }
      
      // rjf: form final result
      result.root = idxed_base_tree;
      result.type_key = e_type_key_unwrap(ext->base_ptr_member->type_key, E_TypeUnwrapFlag_All);
      result.mode = E_Mode_Offset;
      
      scratch_end(scratch);
    }break;
  }
  return result;
}

E_TYPE_EXPAND_INFO_FUNCTION_DEF(slice)
{
  E_SliceIRExt *accel = (E_SliceIRExt *)eval.irtree.user_data;
  U64 count = 0;
  {
    if(accel->count_member != 0)
    {
      count = e_value_eval_from_eval(e_eval_wrapf(eval, "$.%S", accel->count_member->name)).value.u64;
    }
    else if(accel->opl_ptr_member != 0 && accel->base_ptr_member != 0)
    {
      count = e_value_eval_from_eval(e_eval_wrapf(eval, "raw($.%S) - raw($.%S)", accel->opl_ptr_member->name, accel->base_ptr_member->name)).value.u64;
    }
  }
  E_TypeExpandInfo info = {0, count};
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(slice)
{
  U64 read_range_count = dim_1u64(idx_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    evals_out[idx] = e_eval_wrapf(eval, "$[%I64u]", idx_range.min + idx);
  }
}

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `folder` type

typedef struct E_FolderAccel E_FolderAccel;
struct E_FolderAccel
{
  String8 folder_path;
  String8Array folders;
  String8Array files;
};

E_TYPE_EXPAND_INFO_FUNCTION_DEF(folder)
{
  E_TypeExpandInfo info = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: evaluate lhs file path ID
    U64 lhs_string_id = eval.value.u64;
    String8 folder_path = e_string_from_id(lhs_string_id);
    
    //- rjf: compute filter - omit common prefixes (common parent paths)
    String8 local_filter = filter;
    {
      U64 folder_pos_in_filter = str8_find_needle(filter, 0, folder_path, StringMatchFlag_CaseInsensitive|StringMatchFlag_SlashInsensitive);
      if(folder_pos_in_filter < filter.size)
      {
        local_filter = str8_skip(local_filter, folder_pos_in_filter+folder_path.size);
        local_filter = str8_skip_chop_slashes(local_filter);
      }
      else
      {
        MemoryZeroStruct(&local_filter);
      }
    }
    
    //- rjf: gather & filter files in this folder
    String8List folder_paths = {0};
    String8List file_paths = {0};
    {
      OS_FileIter *iter = os_file_iter_begin(scratch.arena, folder_path, 0);
      for(OS_FileInfo info = {0}; os_file_iter_next(scratch.arena, iter, &info);)
      {
        FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, local_filter, info.name);
        if(matches.count == matches.needle_part_count)
        {
          if(info.props.flags & FilePropertyFlag_IsFolder)
          {
            str8_list_push(scratch.arena, &folder_paths, push_str8_copy(arena, info.name));
          }
          else
          {
            str8_list_push(scratch.arena, &file_paths, push_str8_copy(arena, info.name));
          }
        }
      }
      os_file_iter_end(iter);
    }
    
    //- rjf: build accelerator
    E_FolderAccel *accel = push_array(arena, E_FolderAccel, 1);
    accel->folder_path = push_str8_copy(arena, folder_path);
    accel->folders = str8_array_from_list(arena, &folder_paths);
    accel->files = str8_array_from_list(arena, &file_paths);
    info.user_data = accel;
    info.expr_count = accel->folders.count + accel->files.count;
    scratch_end(scratch);
  }
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(folder)
{
  E_FolderAccel *accel = (E_FolderAccel *)user_data;
  U64 out_idx = 0;
  for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, out_idx += 1)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 path_expr_string = {0};
    if(0 <= idx && idx < accel->folders.count)
    {
      String8 folder_name = accel->folders.v[idx - 0];
      String8 folder_path = push_str8f(scratch.arena, "%S%s%S", accel->folder_path, accel->folder_path.size != 0 ? "/" : "", folder_name);
      path_expr_string = push_str8f(arena, "folder:\"%S/\"", escaped_from_raw_str8(scratch.arena, folder_path));
    }
    else if(accel->folders.count <= idx && idx < accel->folders.count + accel->files.count)
    {
      String8 file_name = accel->files.v[idx - accel->folders.count];
      String8 file_path = push_str8f(scratch.arena, "%S%s%S", accel->folder_path, accel->folder_path.size != 0 ? "/" : "", file_name);
      path_expr_string = push_str8f(arena, "file:\"%S\"", escaped_from_raw_str8(scratch.arena, file_path));
    }
    evals_out[out_idx] = e_eval_from_string(path_expr_string);
    scratch_end(scratch);
  }
}

E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(folder)
{
  U64 id = 0;
  E_FolderAccel *accel = (E_FolderAccel *)user_data;
  String8 name = {0};
  if(0 < num && num <= accel->folders.count)
  {
    name = accel->folders.v[num-1];
  }
  else if(accel->folders.count < num && num <= accel->folders.count+accel->files.count)
  {
    name = accel->files.v[num-accel->folders.count-1];
  }
  id = e_hash_from_string(5381, name);
  return id;
}

E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(folder)
{
  U64 num = 0;
  E_FolderAccel *accel = (E_FolderAccel *)user_data;
  for(U64 idx = 0; idx < accel->folders.count+accel->files.count; idx += 1)
  {
    String8 name = {0};
    if(0 <= idx && idx < accel->folders.count)
    {
      name = accel->folders.v[idx];
    }
    else if(accel->folders.count <= idx && idx < accel->folders.count+accel->files.count)
    {
      name = accel->files.v[idx-accel->folders.count];
    }
    U64 hash = e_hash_from_string(5381, name);
    if(hash == id)
    {
      num = idx+1;
      break;
    }
  }
  return num;
}

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `file` type

typedef struct E_FileAccel E_FileAccel;
struct E_FileAccel
{
  String8 file_path;
  FileProperties props;
  String8Array fields;
};

E_TYPE_IREXT_FUNCTION_DEF(file)
{
  E_IRTreeAndType result = *irtree;
  E_FileAccel *accel = push_array(arena, E_FileAccel, 1);
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: evaluate lhs file path ID
    E_OpList lhs_oplist = e_oplist_from_irtree(scratch.arena, irtree->root);
    String8 lhs_bytecode = e_bytecode_from_oplist(scratch.arena, &lhs_oplist);
    E_Interpretation lhs_interp = e_interpret(lhs_bytecode);
    E_Value lhs_value = lhs_interp.value;
    U64 lhs_string_id = lhs_value.u64;
    
    //- rjf: get file path
    String8 file_path = e_string_from_id(lhs_string_id);
    
    //- rjf: build field list
    String8List fields = {0};
    str8_list_pushf(arena, &fields, "size");
    str8_list_pushf(arena, &fields, "last_modified_time");
    str8_list_pushf(arena, &fields, "creation_time");
    str8_list_pushf(arena, &fields, "data");
    
    //- rjf: fill accel
    accel->file_path = push_str8_copy(arena, file_path);
    accel->props = os_properties_from_file_path(file_path);
    accel->fields = str8_array_from_list(arena, &fields);
    
    scratch_end(scratch);
  }
  E_IRExt ext = {accel};
  return ext;
}

E_TYPE_ACCESS_FUNCTION_DEF(file)
{
  E_IRTreeAndType result = {&e_irnode_nil}; 
  E_FileAccel *accel = (E_FileAccel *)lhs_irtree->user_data;
  if(expr->kind == E_ExprKind_MemberAccess)
  {
    E_Expr *rhs = expr->first->next;
    String8 member_name = rhs->string;
    if(str8_match(member_name, str8_lit("size"), 0))
    {
      E_Space space = e_space_make(E_SpaceKind_FileSystem);
      space.u64_0 = e_id_from_string(accel->file_path);
      result.root = e_irtree_set_space(arena, space, e_irtree_const_u(arena, accel->props.size));
      result.type_key = e_type_key_basic(E_TypeKind_U64);
      result.mode = E_Mode_Value;
    }
    else if(str8_match(member_name, str8_lit("last_modified_time"), 0))
    {
      E_Space space = e_space_make(E_SpaceKind_FileSystem);
      space.u64_0 = e_id_from_string(accel->file_path);
      result.root = e_irtree_set_space(arena, space, e_irtree_const_u(arena, accel->props.modified));
      result.type_key = e_type_key_basic(E_TypeKind_U64);
      result.mode = E_Mode_Value;
    }
    else if(str8_match(member_name, str8_lit("creation_time"), 0))
    {
      E_Space space = e_space_make(E_SpaceKind_FileSystem);
      space.u64_0 = e_id_from_string(accel->file_path);
      result.root = e_irtree_set_space(arena, space, e_irtree_const_u(arena, accel->props.created));
      result.type_key = e_type_key_basic(E_TypeKind_U64);
      result.mode = E_Mode_Value;
    }
    else if(str8_match(member_name, str8_lit("data"), 0))
    {
      E_Space space = e_space_make(E_SpaceKind_File);
      space.u64_0 = e_id_from_string(accel->file_path);
      result.root     = e_irtree_set_space(arena, space, e_irtree_const_u(arena, 0));
      result.type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), accel->props.size, 0);
      result.mode = E_Mode_Offset;
    }
  }
  return result;
}

E_TYPE_EXPAND_INFO_FUNCTION_DEF(file)
{
  E_FileAccel *accel = (E_FileAccel *)eval.irtree.user_data;
  E_TypeExpandInfo info = {accel, accel->fields.count};
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(file)
{
  E_FileAccel *accel = (E_FileAccel *)user_data;
  U64 out_idx = 0;
  for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, out_idx += 1)
  {
    E_Expr *expr = &e_expr_nil;
    String8 string = {0};
    if(0 <= idx && idx < accel->fields.count)
    {
      String8 name = accel->fields.v[idx];
      evals_out[out_idx] = e_eval_wrapf(eval, "$.%S", name);
    }
  }
}
