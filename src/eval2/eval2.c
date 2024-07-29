// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/eval2.meta.c"

////////////////////////////////
//~ rjf: Lexing/Parsing Data Tables

global read_only String8 e_multichar_symbol_strings[] =
{
  str8_lit_comp("<<"),
  str8_lit_comp(">>"),
  str8_lit_comp("->"),
  str8_lit_comp("<="),
  str8_lit_comp(">="),
  str8_lit_comp("=="),
  str8_lit_comp("!="),
  str8_lit_comp("&&"),
  str8_lit_comp("||"),
};

global read_only struct {E_ExprKind kind; String8 string; S64 precedence;} e_unary_prefix_op_table[] =
{
  // { E_ExprKind_???, str8_lit_comp("+"), 2 },
  { E_ExprKind_Neg,    str8_lit_comp("-"),      2 },
  { E_ExprKind_LogNot, str8_lit_comp("!"),      2 },
  { E_ExprKind_Deref,  str8_lit_comp("*"),      2 },
  { E_ExprKind_Address,str8_lit_comp("&"),      2 },
  { E_ExprKind_Sizeof, str8_lit_comp("sizeof"), 2 },
  // { E_ExprKind_Alignof, str8_lit_comp("_Alignof"), 2 },
};

global read_only struct {E_ExprKind kind; String8 string; S64 precedence;} e_binary_op_table[] =
{
  { E_ExprKind_Mul,    str8_lit_comp("*"),  3  },
  { E_ExprKind_Div,    str8_lit_comp("/"),  3  },
  { E_ExprKind_Mod,    str8_lit_comp("%"),  3  },
  { E_ExprKind_Add,    str8_lit_comp("+"),  4  },
  { E_ExprKind_Sub,    str8_lit_comp("-"),  4  },
  { E_ExprKind_LShift, str8_lit_comp("<<"), 5  },
  { E_ExprKind_RShift, str8_lit_comp(">>"), 5  },
  { E_ExprKind_Less,   str8_lit_comp("<"),  6  },
  { E_ExprKind_LsEq,   str8_lit_comp("<="), 6  },
  { E_ExprKind_Grtr,   str8_lit_comp(">"),  6  },
  { E_ExprKind_GrEq,   str8_lit_comp(">="), 6  },
  { E_ExprKind_EqEq,   str8_lit_comp("=="), 7  },
  { E_ExprKind_NtEq,   str8_lit_comp("!="), 7  },
  { E_ExprKind_BitAnd, str8_lit_comp("&"),  8  },
  { E_ExprKind_BitXor, str8_lit_comp("^"),  9  },
  { E_ExprKind_BitOr,  str8_lit_comp("|"),  10 },
  { E_ExprKind_LogAnd, str8_lit_comp("&&"), 11 },
  { E_ExprKind_LogOr,  str8_lit_comp("||"), 12 },
  { E_ExprKind_Define, str8_lit_comp("="),  13 },
};

global read_only S64 e_max_precedence = 15;

////////////////////////////////
//~ rjf: Basic Helper Functions

internal U64
e_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

////////////////////////////////
//~ rjf: Type Kind Enum Functions

internal E_TypeKind
e_type_kind_from_rdi(RDI_TypeKind kind)
{
  E_TypeKind result = E_TypeKind_Null;
  switch(kind)
  {
    default:{}break;
    case RDI_TypeKind_Void:                   {result = E_TypeKind_Void;}break;
    case RDI_TypeKind_Handle:                 {result = E_TypeKind_Handle;}break;
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
    
    case TG_Kind_Null: case TG_Kind_Void:
    case TG_Kind_F16:  case TG_Kind_F32PP: case TG_Kind_F48:
    case TG_Kind_F80:  case TG_Kind_F128:
    case TG_Kind_ComplexF32: case TG_Kind_ComplexF64:
    case TG_Kind_ComplexF80: case TG_Kind_ComplexF128:
    case TG_Kind_Modifier:   case TG_Kind_Array:
    case TG_Kind_Struct:     case TG_Kind_Class: case TG_Kind_Union:
    case TG_Kind_Enum:       case TG_Kind_Alias:
    case TG_Kind_IncompleteStruct: case TG_Kind_IncompleteClass:
    case TG_Kind_IncompleteUnion:  case TG_Kind_IncompleteEnum:
    case TG_Kind_Bitfield:
    case TG_Kind_Variadic:
    {result = RDI_EvalTypeGroup_Other;}break;
    
    case TG_Kind_Handle:
    case TG_Kind_UChar8: case TG_Kind_UChar16: case TG_Kind_UChar32:
    case TG_Kind_U8:     case TG_Kind_U16:     case TG_Kind_U32:
    case TG_Kind_U64:    case TG_Kind_U128:    case TG_Kind_U256:
    case TG_Kind_U512:
    case TG_Kind_Ptr: case TG_Kind_LRef: case TG_Kind_RRef:
    case TG_Kind_Function: case TG_Kind_Method: case TG_Kind_MemberPtr:
    {result = RDI_EvalTypeGroup_U;}break;
    
    case TG_Kind_Char8: case TG_Kind_Char16: case TG_Kind_Char32:
    case TG_Kind_S8:    case TG_Kind_S16:    case TG_Kind_S32:
    case TG_Kind_S64:   case TG_Kind_S128:   case TG_Kind_S256:
    case TG_Kind_S512:
    case TG_Kind_Bool:
    {result = RDI_EvalTypeGroup_S;}break;
    
    case TG_Kind_F32:{result = RDI_EvalTypeGroup_F32;}break;
    case TG_Kind_F64:{result = RDI_EvalTypeGroup_F64;}break;
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

////////////////////////////////
//~ rjf: Message Functions

internal void
e_msg(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, String8 text)
{
  E_Msg *msg = push_array(arena, E_Msg, 1);
  SLLQueuePush(msgs->first, msgs->last, msg);
  msgs->count += 1;
  msgs->max_kind = Max(kind, msgs->max_kind);
  msg->kind = kind;
  msg->location = location;
  msg->text = text;
}

internal void
e_msgf(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String8 text = push_str8fv(arena, fmt, args);
  va_end(args);
  e_msg(arena, msgs, kind, location, text);
}

internal void
e_msg_list_concat_in_place(E_MsgList *dst, E_MsgList *to_push)
{
  if(dst->last != 0 && to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->count += to_push->count;
    dst->max_kind = Max(dst->max_kind, to_push->max_kind);
  }
  else if(to_push->first != 0)
  {
    MemoryCopyStruct(dst, to_push);
  }
  MemoryZeroStruct(to_push);
}

////////////////////////////////
//~ rjf: Basic Map Functions

//- rjf: string -> num

internal E_String2NumMap
e_string2num_map_make(Arena *arena, U64 slot_count)
{
  E_String2NumMap map = {0};
  map.slots_count = slot_count;
  map.slots = push_array(arena, E_String2NumMapSlot, map.slots_count);
  return map;
}

internal void
e_string2num_map_insert(Arena *arena, E_String2NumMap *map, String8 string, U64 num)
{
  U64 hash = e_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  E_String2NumMapNode *existing_node = 0;
  for(E_String2NumMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
  {
    if(str8_match(node->string, string, 0) && node->num == num)
    {
      existing_node = node;
      break;
    }
  }
  if(existing_node == 0)
  {
    E_String2NumMapNode *node = push_array(arena, E_String2NumMapNode, 1);
    SLLQueuePush_N(map->slots[slot_idx].first, map->slots[slot_idx].last, node, hash_next);
    SLLQueuePush_N(map->first, map->last, node, order_next);
    node->string = push_str8_copy(arena, string);
    node->num = num;
    map->node_count += 1;
  }
}

internal U64
e_num_from_string(E_String2NumMap *map, String8 string)
{
  U64 num = 0;
  if(map->slots_count != 0)
  {
    U64 hash = e_hash_from_string(string);
    U64 slot_idx = hash%map->slots_count;
    E_String2NumMapNode *existing_node = 0;
    for(E_String2NumMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
    {
      if(str8_match(node->string, string, 0))
      {
        existing_node = node;
        break;
      }
    }
    if(existing_node != 0)
    {
      num = existing_node->num;
    }
  }
  return num;
}

internal E_String2NumMapNodeArray
e_string2num_map_node_array_from_map(Arena *arena, E_String2NumMap *map)
{
  E_String2NumMapNodeArray result = {0};
  result.count = map->node_count;
  result.v = push_array(arena, E_String2NumMapNode *, result.count);
  U64 idx = 0;
  for(E_String2NumMapNode *n = map->first; n != 0; n = n->order_next, idx += 1)
  {
    result.v[idx] = n;
  }
  return result;
}

internal int
e_string2num_map_node_qsort_compare__num_ascending(E_String2NumMapNode **a, E_String2NumMapNode **b)
{
  int result = 0;
  if(a[0]->num < b[0]->num)
  {
    result = -1;
  }
  else if(a[0]->num > b[0]->num)
  {
    result = +1;
  }
  return result;
}

internal void
e_string2num_map_node_array_sort__in_place(E_String2NumMapNodeArray *array)
{
  quick_sort(array->v, array->count, sizeof(array->v[0]), e_string2num_map_node_qsort_compare__num_ascending);
}

//- rjf: string -> expr

internal E_String2ExprMap
e_string2expr_map_make(Arena *arena, U64 slot_count)
{
  E_String2ExprMap map = {0};
  map.slots_count = slot_count;
  map.slots = push_array(arena, E_String2ExprMapSlot, map.slots_count);
  return map;
}

internal void
e_string2expr_map_insert(Arena *arena, E_String2ExprMap *map, String8 string, E_Expr *expr)
{
  U64 hash = e_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  E_String2ExprMapNode *existing_node = 0;
  for(E_String2ExprMapNode *node = map->slots[slot_idx].first;
      node != 0;
      node = node->hash_next)
  {
    if(str8_match(node->string, string, 0))
    {
      existing_node = node;
      break;
    }
  }
  if(existing_node == 0)
  {
    E_String2ExprMapNode *node = push_array(arena, E_String2ExprMapNode, 1);
    SLLQueuePush_N(map->slots[slot_idx].first, map->slots[slot_idx].last, node, hash_next);
    node->string = push_str8_copy(arena, string);
    existing_node = node;
  }
  existing_node->expr = expr;
}

internal void
e_string2expr_map_inc_poison(E_String2ExprMap *map, String8 string)
{
  U64 hash = e_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  for(E_String2ExprMapNode *node = map->slots[slot_idx].first;
      node != 0;
      node = node->hash_next)
  {
    if(str8_match(node->string, string, 0))
    {
      node->poison_count += 1;
      break;
    }
  }
}

internal void
e_string2expr_map_dec_poison(E_String2ExprMap *map, String8 string)
{
  U64 hash = e_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  for(E_String2ExprMapNode *node = map->slots[slot_idx].first;
      node != 0;
      node = node->hash_next)
  {
    if(str8_match(node->string, string, 0) && node->poison_count > 0)
    {
      node->poison_count -= 1;
      break;
    }
  }
}

internal E_Expr *
e_expr_from_string(E_String2ExprMap *map, String8 string)
{
  E_Expr *expr = &e_expr_nil;
  if(map->slots_count != 0)
  {
    U64 hash = e_hash_from_string(string);
    U64 slot_idx = hash%map->slots_count;
    E_String2ExprMapNode *existing_node = 0;
    for(E_String2ExprMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
    {
      if(str8_match(node->string, string, 0) && node->poison_count == 0)
      {
        existing_node = node;
        break;
      }
    }
    if(existing_node != 0)
    {
      expr = existing_node->expr;
    }
  }
  return expr;
}

////////////////////////////////
//~ rjf: Debug-Info-Driven Map Building Functions

internal E_String2NumMap *
e_push_locals_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: gather scopes to walk
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    RDI_Scope *scope;
  };
  Task *first_task = 0;
  Task *last_task = 0;
  
  //- rjf: voff -> tightest scope
  RDI_Scope *tightest_scope = 0;
  {
    U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff);
    RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
    Task *task = push_array(scratch.arena, Task, 1);
    task->scope = scope;
    SLLQueuePush(first_task, last_task, task);
    tightest_scope = scope;
  }
  
  //- rjf: voff-1 -> scope
  if(voff > 0)
  {
    U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff-1);
    RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
    if(scope != tightest_scope)
    {
      Task *task = push_array(scratch.arena, Task, 1);
      task->scope = scope;
      SLLQueuePush(first_task, last_task, task);
    }
  }
  
  //- rjf: tightest scope -> walk up the tree & build tasks for each parent scope
  if(tightest_scope != 0)
  {
    RDI_Scope *nil_scope = rdi_element_from_name_idx(rdi, Scopes, 0);
    for(RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, tightest_scope->parent_scope_idx);
        scope != 0 && scope != nil_scope;
        scope = rdi_element_from_name_idx(rdi, Scopes, scope->parent_scope_idx))
    {
      Task *task = push_array(scratch.arena, Task, 1);
      task->scope = scope;
      SLLQueuePush(first_task, last_task, task);
    }
  }
  
  //- rjf: build blank map
  E_String2NumMap *map = push_array(arena, E_String2NumMap, 1);
  *map = e_string2num_map_make(arena, 1024);
  
  //- rjf: accumulate locals for all tasks
  for(Task *task = first_task; task != 0; task = task->next)
  {
    RDI_Scope *scope = task->scope;
    if(scope != 0)
    {
      U32 local_opl_idx = scope->local_first + scope->local_count;
      for(U32 local_idx = scope->local_first; local_idx < local_opl_idx; local_idx += 1)
      {
        RDI_Local *local_var = rdi_element_from_name_idx(rdi, Locals, local_idx);
        U64 local_name_size = 0;
        U8 *local_name_str = rdi_string_from_idx(rdi, local_var->name_string_idx, &local_name_size);
        String8 name = push_str8_copy(arena, str8(local_name_str, local_name_size));
        e_string2num_map_insert(arena, map, name, (U64)local_idx+1);
      }
    }
  }
  
  scratch_end(scratch);
  return map;
}

internal E_String2NumMap *
e_push_member_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff)
{
  //- rjf: voff -> tightest scope
  U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff);
  RDI_Scope *tightest_scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
  
  //- rjf: tightest scope -> procedure
  U32 proc_idx = tightest_scope->proc_idx;
  RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, proc_idx);
  
  //- rjf: procedure -> udt
  U32 udt_idx = procedure->container_idx;
  RDI_UDT *udt = rdi_element_from_name_idx(rdi, UDTs, udt_idx);
  
  //- rjf: build blank map
  E_String2NumMap *map = push_array(arena, E_String2NumMap, 1);
  *map = e_string2num_map_make(arena, 64);
  
  //- rjf: udt -> fill member map
  if(!(udt->flags & RDI_UDTFlag_EnumMembers))
  {
    U64 data_member_num = 1;
    for(U32 member_idx = udt->member_first;
        member_idx < udt->member_first+udt->member_count;
        member_idx += 1)
    {
      RDI_Member *m = rdi_element_from_name_idx(rdi, Members, member_idx);
      if(m->kind == RDI_MemberKind_DataField)
      {
        String8 name = {0};
        name.str = rdi_string_from_idx(rdi, m->name_string_idx, &name.size);
        e_string2num_map_insert(arena, map, name, data_member_num);
        data_member_num += 1;
      }
    }
  }
  
  return map;
}

////////////////////////////////
//~ rjf: Context Selection Functions (Required For All Subsequent APIs)

internal void
e_select_ctx(E_Ctx *ctx)
{
  e_ctx = ctx;
}

////////////////////////////////
//~ rjf: Tokenization Functions

internal E_Token
e_token_zero(void)
{
  E_Token t = zero_struct;
  return t;
}

internal void
e_token_chunk_list_push(Arena *arena, E_TokenChunkList *list, U64 chunk_size, E_Token *token)
{
  E_TokenChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, E_TokenChunkNode, 1);
    SLLQueuePush(list->first, list->last, node);
    node->cap = chunk_size;
    node->v = push_array_no_zero(arena, E_Token, node->cap);
    list->node_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], token);
  node->count += 1;
  list->total_count += 1;
}

internal E_TokenArray
e_token_array_from_chunk_list(Arena *arena, E_TokenChunkList *list)
{
  E_TokenArray array = {0};
  array.count = list->total_count;
  array.v = push_array_no_zero(arena, E_Token, array.count);
  U64 idx = 0;
  for(E_TokenChunkNode *node = list->first; node != 0; node = node->next)
  {
    MemoryCopy(array.v+idx, node->v, sizeof(E_Token)*node->count);
  }
  return array;
}

internal E_TokenArray
e_token_array_from_text(Arena *arena, String8 text)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: lex loop
  E_TokenChunkList tokens = {0};
  U64 active_token_start_idx = 0;
  E_TokenKind active_token_kind = E_TokenKind_Null;
  B32 active_token_kind_started_with_tick = 0;
  B32 escaped = 0;
  for(U64 idx = 0, advance = 0; idx <= text.size; idx += advance)
  {
    U8 byte      = (idx+0 < text.size) ? text.str[idx+0] : 0;
    U8 byte_next = (idx+1 < text.size) ? text.str[idx+1] : 0;
    U8 byte_next2= (idx+2 < text.size) ? text.str[idx+2] : 0;
    advance = 1;
    B32 token_formed = 0;
    U64 token_end_idx_pad = 0;
    switch(active_token_kind)
    {
      //- rjf: no active token -> seek token starter
      default:
      {
        if(char_is_alpha(byte) || byte == '_' || byte == '`' || byte == '$')
        {
          active_token_kind = E_TokenKind_Identifier;
          active_token_start_idx = idx;
          active_token_kind_started_with_tick = (byte == '`');
        }
        else if(char_is_digit(byte, 10) || (byte == '.' && char_is_digit(byte_next, 10)))
        {
          active_token_kind = E_TokenKind_Numeric;
          active_token_start_idx = idx;
        }
        else if(byte == '"')
        {
          active_token_kind = E_TokenKind_StringLiteral;
          active_token_start_idx = idx;
        }
        else if(byte == '\'')
        {
          active_token_kind = E_TokenKind_CharLiteral;
          active_token_start_idx = idx;
        }
        else if(byte == '~' || byte == '!' || byte == '%' || byte == '^' ||
                byte == '&' || byte == '*' || byte == '(' || byte == ')' ||
                byte == '-' || byte == '=' || byte == '+' || byte == '[' ||
                byte == ']' || byte == '{' || byte == '}' || byte == ':' ||
                byte == ';' || byte == ',' || byte == '.' || byte == '<' ||
                byte == '>' || byte == '/' || byte == '?' || byte == '|')
        {
          active_token_kind = E_TokenKind_Symbol;
          active_token_start_idx = idx;
        }
      }break;
      
      //- rjf: active tokens -> seek enders
      case E_TokenKind_Identifier:
      {
        if(byte == ':' && byte_next == ':' && (char_is_alpha(byte_next2) || byte_next2 == '_' || byte_next2 == '<'))
        {
          // NOTE(rjf): encountering C++-style namespaces - skip over scope resolution symbol
          // & keep going.
          advance = 2;
        }
        else if((byte == '\'' || byte == '`') && active_token_kind_started_with_tick)
        {
          // NOTE(rjf): encountering ` -> ' or ` -> ` style identifier escapes
          active_token_kind_started_with_tick = 0;
          advance = 1;
        }
        else if(byte == '<')
        {
          // NOTE(rjf): encountering C++-style templates - try to find ender. if no ender found,
          // assume this is an operator & just consume the identifier part.
          S64 nest = 1;
          for(U64 idx2 = idx+1; idx2 <= text.size; idx2 += 1)
          {
            if(idx2 < text.size && text.str[idx2] == '<')
            {
              nest += 1;
            }
            else if(idx2 < text.size && text.str[idx2] == '>')
            {
              nest -= 1;
              if(nest == 0)
              {
                advance = (idx2+1-idx);
                break;
              }
            }
            else if(idx2 == text.size && nest != 0)
            {
              token_formed = 1;
              advance = 0;
              break;
            }
          }
        }
        else if(!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && !active_token_kind_started_with_tick && byte != '@' && byte != '$')
        {
          advance = 0;
          token_formed = 1;
        }
      }break;
      case E_TokenKind_Numeric:
      {
        if(!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '.')
        {
          advance = 0;
          token_formed = 1;
        }
      }break;
      case E_TokenKind_StringLiteral:
      {
        if(escaped == 0 && byte == '\\')
        {
          escaped = 1;
        }
        else if(escaped)
        {
          escaped = 0;
        }
        else if(escaped == 0 && byte == '"')
        {
          advance = 1;
          token_formed = 1;
          token_end_idx_pad = 1;
        }
      }break;
      case E_TokenKind_CharLiteral:
      {
        if(escaped == 0 && byte == '\\')
        {
          escaped = 1;
        }
        else if(escaped)
        {
          escaped = 0;
        }
        else if(escaped == 0 && byte == '\'')
        {
          advance = 1;
          token_formed = 1;
          token_end_idx_pad = 1;
        }
      }break;
      case E_TokenKind_Symbol:
      {
        if(byte != '~' && byte != '!' && byte != '%' && byte != '^' &&
           byte != '&' && byte != '*' && byte != '(' && byte != ')' &&
           byte != '-' && byte != '=' && byte != '+' && byte != '[' &&
           byte != ']' && byte != '{' && byte != '}' && byte != ':' &&
           byte != ';' && byte != ',' && byte != '.' && byte != '<' &&
           byte != '>' && byte != '/' && byte != '?' && byte != '|')
        {
          advance = 0;
          token_formed = 1;
        }
      }break;
    }
    
    //- rjf: token formed -> push new formed token(s)
    if(token_formed)
    {
      // rjf: non-symbols *or* symbols of only 1-length can be immediately
      // pushed as a token
      if(active_token_kind != E_TokenKind_Symbol || idx==active_token_start_idx+1)
      {
        E_Token token = {active_token_kind, r1u64(active_token_start_idx, idx+token_end_idx_pad)};
        e_token_chunk_list_push(scratch.arena, &tokens, 256, &token);
      }
      
      // rjf: symbolic strings matching `--` mean the remainder of the string
      // is reserved for external usage. the rest of the stream should not
      // be tokenized.
      else if(idx == active_token_start_idx+2 && text.str[active_token_start_idx] == '-' && text.str[active_token_start_idx+1] == '-')
      {
        break;
      }
      
      // rjf: if we got a symbol string of N>1 characters, then we need to
      // apply the maximum-munch rule, and produce M<=N tokens, where each
      // formed token is the maximum size possible, given the legal
      // >1-length symbol strings.
      else
      {
        U64 advance2 = 0;
        for(U64 idx2 = active_token_start_idx; idx2 < idx; idx2 += advance2)
        {
          advance2 = 1;
          for(U64 multichar_symbol_idx = 0;
              multichar_symbol_idx < ArrayCount(e_multichar_symbol_strings);
              multichar_symbol_idx += 1)
          {
            String8 multichar_symbol_string = e_multichar_symbol_strings[multichar_symbol_idx];
            String8 part_of_token = str8_substr(text, r1u64(idx2, idx2+multichar_symbol_string.size));
            if(str8_match(part_of_token, multichar_symbol_string, 0))
            {
              advance2 = multichar_symbol_string.size;
              break;
            }
          }
          E_Token token = {active_token_kind, r1u64(idx2, idx2+advance2)};
          e_token_chunk_list_push(scratch.arena, &tokens, 256, &token);
        }
      }
      
      // rjf: reset for subsequent tokens.
      active_token_kind = E_TokenKind_Null;
    }
  }
  
  //- rjf: chunk list -> array & return
  E_TokenArray array = e_token_array_from_chunk_list(arena, &tokens);
  scratch_end(scratch);
  return array;
}

internal E_TokenArray
e_token_array_make_first_opl(E_Token *first, E_Token *opl)
{
  E_TokenArray array = {first, (U64)(opl-first)};
  return array;
}

////////////////////////////////
//~ rjf: Expression Tree Building Functions

internal E_Expr *
e_push_expr(Arena *arena, E_ExprKind kind, void *location)
{
  E_Expr *e = push_array(arena, E_Expr, 1);
  e->first = e->last = e->next = &e_expr_nil;
  e->location = location;
  e->kind = kind;
}

internal void
e_expr_push_child(E_Expr *parent, E_Expr *child)
{
  SLLQueuePush_NZ(&e_expr_nil, parent->first, parent->last, child, next);
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
e_type_key_ext(TG_Kind kind, U32 type_idx, U32 rdi_idx)
{
  E_TypeKey key = {E_TypeKeyKind_Ext};
  key.u32[0] = (U32)kind;
  key.u32[1] = type_idx;
  key.u32[2] = rdi_idx;
  return key;
}

internal E_TypeKey
e_type_key_reg(Architecture arch, REGS_RegCode code)
{
  E_TypeKey key = {E_TypeKeyKind_Reg};
  key.u32[0] = (U32)arch;
  key.u32[1] = (U32)code;
  return key;
}

internal E_TypeKey
e_type_key_reg_alias(Architecture arch, REGS_AliasCode code)
{
  E_TypeKey key = {E_TypeKeyKind_RegAlias};
  key.u32[0] = (U32)arch;
  key.u32[1] = (U32)code;
  return key;
}

internal E_TypeKey
e_type_key_cons(E_TypeKind kind, E_TypeKey direct_key, U64 u64)
{
  U32 buffer[] =
  {
    (U32)kind,
    (U32)direct_key.kind,
    (U32)direct_key.u32[0],
    (U32)direct_key.u32[1],
    (U32)direct_key.u32[2],
    (U32)((u64 & 0x00000000ffffffffull)>> 0),
    (U32)((u64 & 0xffffffff00000000ull)>> 32),
  };
  U64 content_hash = e_hash_from_string(str8((U8 *)buffer, sizeof(buffer)));
  U64 content_slot_idx = content_hash%e_ctx->cons_content_slots_count;
  E_ConsTypeSlot *content_slot = &e_ctx->cons_content_slots[content_slot_idx];
  E_ConsTypeNode *node = 0;
  for(E_ConsTypeNode *n = content_slot->first; n != 0; n = n->content_next)
  {
    if(e_type_kind_from_key(n->key) == kind &&
       e_type_key_match(n->direct_key, direct_key) &&
       n->u64 == u64)
    {
      node = n;
      break;
    }
  }
  E_TypeKey result = zero_struct;
  if(node == 0)
  {
    E_TypeKey key = {E_TypeKeyKind_Cons};
    key.u32[0] = (U32)kind;
    key.u32[1] = (U32)e_ctx->cons_id_gen;
    e_ctx->cons_id_gen += 1;
    U64 key_hash = e_hash_from_string(str8_struct(&key));
    U64 key_slot_idx = key_hash%e_ctx->cons_key_slots_count;
    E_ConsTypeSlot *key_slot = &e_ctx->cons_key_slots[key_slot_idx];
    E_ConsTypeNode *node = push_array(tg_build_arena, E_ConsTypeNode, 1);
    SLLQueuePush_N(content_slot->first, content_slot->last, node, content_next);
    SLLQueuePush_N(key_slot->first, key_slot->last, node, key_next);
    node->key = key;
    node->direct_key = direct_key;
    node->u64 = u64;
    result = key;
  }
  else
  {
    result = node->key;
  }
  return result;
}

//- rjf: basic type key functions

internal B32
e_type_key_match(E_TypeKey l, E_TypeKey r)
{
  B32 result = MemoryMatchStruct(&l, &r);
  return result;
}

//- rjf: key -> info extraction

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

internal E_Type *
e_type_from_key(Arena *arena, E_TypeKey key)
{
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
          type->name       = e_kind_basic_string_table[kind];
          type->byte_size  = e_kind_basic_byte_size_table[kind];
        }
      }break;
      
      //- rjf: constructed type keys
      case E_TypeKeyKind_Cons:
      {
        U64 key_hash = e_hash_from_string(str8_struct(&key));
        U64 key_slot_idx = key_hash%e_ctx->cons_key_slots_count;
        E_ConsTypeSlot *key_slot = &e_ctx->cons_key_slots[key_slot_idx];
        for(E_ConsTypeNode *node = key_slot->first;
            node != 0;
            node = node->key_next)
        {
          if(e_type_key_match(node->key, key))
          {
            type = push_array(arena, E_Type, 1);
            type->kind             = e_type_kind_from_key(node->key);
            type->direct_type_key  = node->direct_key;
            type->count            = node->u64;
            switch(type->kind)
            {
              default:
              {
                type->byte_size = bit_size_from_arch(e_ctx->arch)/8;
              }break;
              case E_TypeKind_Array:
              {
                U64 ptee_size = e_type_byte_size_from_key(node->direct_key);
                type->byte_size = ptee_size * type->count;
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
        RDI_Parsed *rdi = e_ctx->rdis[rdi_idx];
        RDI_TypeNode *rdi_type = rdi_element_from_name_idx(rdi, TypeNodes, type_node_idx);
        if(rdi_type->kind != RDI_TypeKind_NULL)
        {
          E_TypeKind kind = e_kind_from_rdi_type_kind(rdi_type->kind);
          
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
                  member_type_kind = e_kind_from_rdi_type_kind(member_type->kind);
                  E_Member *dst = &members[member_idx-udt->member_first];
                  dst->kind     = e_member_kind_from_rdi_member_kind(src->kind);
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
              E_TypeKind direct_type_kind = e_kind_from_rdi_type_kind(direct_type_node->kind);
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
              E_TypeKind direct_type_kind = e_kind_from_rdi_type_kind(direct_type_node->kind);
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
                type = push_array(arena, E_Type, 1);
                type->kind            = kind;
                type->direct_type_key = direct_type_key;
                type->byte_size       = direct_type_byte_size;
                type->flags           = flags;
              }break;
              case RDI_TypeKind_Ptr:
              case RDI_TypeKind_LRef:
              case RDI_TypeKind_RRef:
              {
                type = push_array(arena, E_Type, 1);
                type->kind            = kind;
                type->direct_type_key = direct_type_key;
                type->byte_size       = bit_size_from_arch(e_ctx->arch)/8;
              }break;
              
              case RDI_TypeKind_Array:
              {
                type = push_array(arena, E_Type, 1);
                type->kind            = kind;
                type->direct_type_key = direct_type_key;
                type->count           = rdi_type->constructed.count;
                type->byte_size       = direct_type_byte_size * type->count;
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
                  type->byte_size       = bit_size_from_arch(e_ctx->arch)/8;
                  type->direct_type_key = direct_type_key;
                  type->count           = count;
                  type->param_type_keys = push_array_no_zero(arena, E_TypeKey, type->count);
                  for(U32 idx = 0; idx < type->count; idx += 1)
                  {
                    U32 param_type_idx = idx_run[idx];
                    if(param_type_idx < type_node_idx)
                    {
                      RDI_TypeNode *param_type_node = rdi_element_from_name_idx(rdi, TypeNodes, param_type_idx);
                      E_TypeKind param_kind = e_kind_from_rdi_type_kind(param_type_node->kind);
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
                  type->byte_size       = bit_size_from_arch(e_ctx->arch)/8;
                  type->owner_type_key  = direct_type_key;
                  type->count           = count;
                  type->param_type_keys = push_array_no_zero(arena, E_TypeKey, type->count);
                  for(U32 idx = 0; idx < type->count; idx += 1)
                  {
                    U32 param_type_idx = idx_run[idx];
                    if(param_type_idx < type_node_idx)
                    {
                      RDI_TypeNode *param_type_node = rdi_element_from_name_idx(rdi, TypeNodes, param_type_idx);
                      E_TypeKind param_kind = e_kind_from_rdi_type_kind(param_type_node->kind);
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
                  E_TypeKind owner_type_kind = e_kind_from_rdi_type_kind(owner_type_node->kind);
                  owner_type_key = e_type_key_ext(owner_type_kind, rdi_type->constructed.owner_type_idx, rdi_idx);
                }
                type = push_array(arena, E_Type, 1);
                type->kind            = kind;
                type->byte_size       = bit_size_from_arch(e_ctx->arch)/8;
                type->owner_type_key  = owner_type_key;
                type->direct_type_key = direct_type_key;
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
              E_TypeKind direct_type_kind = e_kind_from_rdi_type_kind(direct_type_node->kind);
              direct_type_key = e_type_key_ext(direct_type_kind, rdi_type->user_defined.direct_type_idx, rdi_idx);
              direct_type_byte_size = direct_type_node->byte_size;
            }
            
            // rjf: produce
            type = push_array(arena, E_Type, 1);
            type->kind            = kind;
            type->name            = push_str8_copy(arena, name);
            type->byte_size       = direct_type_byte_size;
            type->direct_type_key = direct_type_key;
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
              E_TypeKind direct_type_kind = e_kind_from_rdi_type_kind(direct_type_node->kind);
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
          }
          
        }
      }break;
      
      //- rjf: reg type keys
      case E_TypeKeyKind_Reg:
      {
        Architecture arch = (Architecture)key.u32[0];
        REGS_RegCode code = (REGS_RegCode)key.u32[1];
        REGS_Rng rng = regs_reg_code_rng_table_from_architecture(arch)[code];
        reg_byte_count = (U64)rng.byte_size;
      }goto build_reg_type;
      case E_TypeKeyKind_RegAlias:
      {
        Architecture arch = (Architecture)key.u32[0];
        REGS_AliasCode code = (REGS_AliasCode)key.u32[1];
        REGS_Slice slice = regs_alias_code_slice_table_from_architecture(arch)[code];
        reg_byte_count = (U64)slice.byte_size;
      }goto build_reg_type;
      build_reg_type:
      {
        Temp scratch = scratch_begin(&arena, 1);
        type = push_array(arena, E_Type, 1);
        type->kind       = E_TypeKind_Union;
        type->name       = push_str8f(arena, "reg_%I64u_bit", reg_byte_count*8);
        type->byte_size  = (U64)reg_byte_count;
        
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
              mem->type_key = e_type_key_cons(E_TypeKind_Array, e_type_key_basic(E_TypeKind_U128), reg_byte_count/16);
            }
            if(type->byte_size > 8 && type->byte_size%8 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u64s");
              mem->type_key = e_type_key_cons(E_TypeKind_Array, e_type_key_basic(E_TypeKind_U64), reg_byte_count/8);
            }
            if(type->byte_size > 4 && type->byte_size%4 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u32s");
              mem->type_key = e_type_key_cons(E_TypeKind_Array, e_type_key_basic(E_TypeKind_U32), reg_byte_count/4);
            }
            if(type->byte_size > 2 && type->byte_size%2 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u16s");
              mem->type_key = e_type_key_cons(E_TypeKind_Array, e_type_key_basic(E_TypeKind_U16), reg_byte_count/2);
            }
            if(type->byte_size > 1)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("u8s");
              mem->type_key = e_type_key_cons(E_TypeKind_Array, e_type_key_basic(E_TypeKind_U8), reg_byte_count);
            }
            if(type->byte_size > 4 && type->byte_size%4 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("f32s");
              mem->type_key = e_type_key_cons(E_TypeKind_Array, e_type_key_basic(E_TypeKind_F32), reg_byte_count/4);
            }
            if(type->byte_size > 8 && type->byte_size%8 == 0)
            {
              E_MemberNode *n = push_array(scratch.arena, E_MemberNode, 1);
              SLLQueuePush(members.first, members.last, n);
              members.count += 1;
              E_Member *mem = &n->v;
              mem->kind = E_MemberKind_DataField;
              mem->name = str8_lit("f64s");
              mem->type_key = e_type_key_cons(E_TypeKind_Array, e_type_key_basic(E_TypeKind_F64), reg_byte_count/8);
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
  return type;
}

internal U64
e_type_byte_size_from_key(E_TypeKey key)
{
  U64 result = 0;
  switch(key.kind)
  {
    default:{}break;
    case E_TypeKeyKind_Basic:
    {
      E_TypeKind kind = (E_TypeKind)key.u32[0];
      result = e_kind_basic_byte_size_table[kind];
    }break;
    case E_TypeKeyKind_Ext:
    case E_TypeKeyKind_Cons:
    {
      Temp scratch = scratch_begin(0, 0);
      E_Type *type = e_type_from_key(scratch.arena, key);
      result = type->byte_size;
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal E_TypeKey
e_type_direct_from_key(E_TypeKey key)
{
  E_TypeKey result = zero_struct;
  switch(key.kind)
  {
    default:{}break;
    case E_TypeKeyKind_Ext:
    case E_TypeKeyKind_Cons:
    {
      Temp scratch = scratch_begin(0, 0);
      E_Type *type = e_type_from_key(scratch.arena, key);
      result = type->direct_type_key;
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal E_TypeKey
e_type_owner_from_key(E_TypeKey key)
{
  E_TypeKey result = zero_struct;
  switch(key.kind)
  {
    default:{}break;
    case E_TypeKeyKind_Ext:
    case E_TypeKeyKind_Cons:
    {
      Temp scratch = scratch_begin(0, 0);
      E_Type *type = e_type_from_key(scratch.arena, key);
      result = type->owner_type_key;
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal E_TypeKey
e_type_unwrap_enum(E_TypeKey key)
{
  E_TypeKey result = key;
  for(B32 good = 1; good;)
  {
    E_TypeKind kind = e_type_kind_from_key(result);
    if(kind == E_TypeKind_Enum)
    {
      result = e_type_direct_from_key(result);
    }
    else
    {
      good = 0;
    }
  }
  return result;
}

internal E_TypeKey
e_type_unwrap(E_TypeKey key)
{
  E_TypeKey result = key;
  for(B32 good = 1; good;)
  {
    E_TypeKind kind = e_type_kind_from_key(result);
    if((E_TypeKind_FirstIncomplete <= kind && kind <= E_TypeKind_LastIncomplete) ||
       kind == E_TypeKind_Modifier ||
       kind == E_TypeKind_Alias)
    {
      result = e_type_direct_from_key(result);
    }
    else
    {
      good = 0;
    }
  }
  return result;
}

internal E_TypeKey
e_type_promote(E_TypeKey key)
{
  E_TypeKey result = key;
  E_TypeKind kind = e_type_kind_from_key(key);
  if(kind == TG_Kind_Bool ||
     kind == TG_Kind_S8 ||
     kind == TG_Kind_S16 ||
     kind == TG_Kind_U8 ||
     kind == TG_Kind_U16)
  {
    result = e_type_key_basic(TG_Kind_S32);
  }
  return result;
}

internal B32
e_type_match(E_TypeKey l, E_TypeKey r)
{
  // rjf: unpack parameters
  E_TypeKey lu = e_type_unwrap(l);
  E_TypeKey ru = e_type_unwrap(r);
  
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
          result = 1;
        }break;
        
        case E_TypeKind_Ptr:
        case E_TypeKind_LRef:
        case E_TypeKind_RRef:
        {
          E_TypeKey lud = e_type_direct_from_key(lu);
          E_TypeKey rud = e_type_direct_from_key(ru);
          result = e_type_match(lud, rud);
        }break;
        
        case E_TypeKind_MemberPtr:
        {
          E_TypeKey lud = e_type_direct_from_key(lu);
          E_TypeKey rud = e_type_direct_from_key(ru);
          E_TypeKey luo = e_type_owner_from_key(lu);
          E_TypeKey ruo = e_type_owner_from_key(ru);
          result = (e_type_match(lud, rud) && e_type_match(luo, ruo));
        }break;
        
        case E_TypeKind_Array:
        {
          Temp scratch = scratch_begin(0, 0);
          E_Type *lt = e_type_from_key(scratch.arena, l);
          E_Type *rt = e_type_from_key(scratch.arena, r);
          if(lt->count == rt->count && e_type_match(lt->direct_type_key, rt->direct_type_key))
          {
            result = 1;
          }
          scratch_end(scratch);
        }break;
        
        case E_TypeKind_Function:
        {
          Temp scratch = scratch_begin(0, 0);
          E_Type *lt = e_type_from_key(scratch.arena, l);
          E_Type *rt = e_type_from_key(scratch.arena, r);
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
          scratch_end(scratch);
        }break;
        
        case E_TypeKind_Method:
        {
          Temp scratch = scratch_begin(0, 0);
          E_Type *lt = e_type_from_key(scratch.arena, l);
          E_Type *rt = e_type_from_key(scratch.arena, r);
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
          scratch_end(scratch);
        }break;
      }
    }
  }
  
  return result;
}

////////////////////////////////
//~ rjf: Parsing Functions

internal E_TypeKey
e_leaf_type_from_name(String8 name)
{
  E_TypeKey key = zero_struct;
  B32 found = 0;
  for(U64 rdi_idx = 0; rdi_idx < e_ctx->rdis_count; rdi_idx += 1)
  {
    RDI_Parsed *rdi = e_ctx->rdis[rdi_idx];
    RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_Types);
    RDI_ParsedNameMap parsed_name_map = {0};
    rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
    RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, name.str, name.size);
    if(node != 0)
    {
      U32 match_count = 0;
      U32 *matches = rdi_matches_from_map_node(rdi, node, &match_count);
      if(match_count != 0)
      {
        RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, matches[0]);
        found = type_node->kind != RDI_TypeKind_NULL;
        key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), matches[0], rdi_idx);
        break;
      }
    }
  }
  if(!found)
  {
#define Case(str) (str8_match(name, str8_lit(str), 0))
    if(Case("u8") || Case("uint8") || Case("uint8_t") || Case("uchar8") || Case("U8"))
    {
      key = e_type_key_basic(E_TypeKind_U8);
    }
    else if(Case("u16") || Case("uint16") || Case("uint16_t") || Case("uchar16") || Case("U16"))
    {
      key = e_type_key_basic(E_TypeKind_U16);
    }
    else if(Case("u32") || Case("uint32") || Case("uint32_t") || Case("uchar32") || Case("U32") || Case("uint"))
    {
      key = e_type_key_basic(E_TypeKind_U32);
    }
    else if(Case("u64") || Case("uint64") || Case("uint64_t") || Case("U64"))
    {
      key = e_type_key_basic(E_TypeKind_U64);
    }
    else if(Case("s8") || Case("b8") || Case("B8") || Case("i8") || Case("int8") || Case("int8_t") || Case("char8") || Case("S8"))
    {
      key = e_type_key_basic(E_TypeKind_S8);
    }
    else if(Case("s16") ||Case("b16") || Case("B16") || Case("i16") ||  Case("int16") || Case("int16_t") || Case("char16") || Case("S16"))
    {
      key = e_type_key_basic(E_TypeKind_S16);
    }
    else if(Case("s32") || Case("b32") || Case("B32") || Case("i32") || Case("int32") || Case("int32_t") || Case("char32") || Case("S32") || Case("int"))
    {
      key = e_type_key_basic(E_TypeKind_S32);
    }
    else if(Case("s64") || Case("b64") || Case("B64") || Case("i64") || Case("int64") || Case("int64_t") || Case("S64"))
    {
      key = e_type_key_basic(E_TypeKind_S64);
    }
    else if(Case("void"))
    {
      key = e_type_key_basic(E_TypeKind_Void);
    }
    else if(Case("bool"))
    {
      key = e_type_key_basic(E_TypeKind_Bool);
    }
    else if(Case("float") || Case("f32") || Case("F32") || Case("r32") || Case("R32"))
    {
      key = e_type_key_basic(E_TypeKind_F32);
    }
    else if(Case("double") || Case("f64") || Case("F64") || Case("r64") || Case("R64"))
    {
      key = e_type_key_basic(E_TypeKind_F64);
    }
#undef Case
  }
  return key;
}

internal E_Parse
e_parse_type_from_text_tokens(Arena *arena, String8 text, E_TokenArray *tokens)
{
  E_Parse parse = {0, &e_expr_nil};
  E_Token *token_it = tokens->v;
  
  //- rjf: parse unsigned marker
  B32 unsigned_marker = 0;
  {
    E_Token token = e_token_at_it(token_it, tokens);
    if(token.kind == E_TokenKind_Identifier)
    {
      String8 token_string = str8_substr(text, token.range);
      if(str8_match(token_string, str8_lit("unsigned"), 0))
      {
        token_it += 1;
        unsigned_marker = 1;
      }
    }
  }
  
  //- rjf: parse base type
  {
    E_Token token = e_token_at_it(token_it, tokens);
    if(token.kind == E_TokenKind_Identifier)
    {
      String8 token_string = str8_substr(text, token.range);
      E_TypeKey type_key = e_leaf_type_from_name(token_string);
      if(!e_type_key_match(e_type_key_zero(), type_key))
      {
        token_it += 1;
        
        // rjf: apply unsigned marker to base type
        if(unsigned_marker) switch(e_type_kind_from_key(type_key))
        {
          default:{}break;
          case E_TypeKind_Char8: {type_key = e_type_key_basic(E_TypeKind_UChar8);}break;
          case E_TypeKind_Char16:{type_key = e_type_key_basic(E_TypeKind_UChar16);}break;
          case E_TypeKind_Char32:{type_key = e_type_key_basic(E_TypeKind_UChar32);}break;
          case E_TypeKind_S8:  {type_key = e_type_key_basic(E_TypeKind_U8);}break;
          case E_TypeKind_S16: {type_key = e_type_key_basic(E_TypeKind_U16);}break;
          case E_TypeKind_S32: {type_key = e_type_key_basic(E_TypeKind_U32);}break;
          case E_TypeKind_S64: {type_key = e_type_key_basic(E_TypeKind_U64);}break;
          case E_TypeKind_S128:{type_key = e_type_key_basic(E_TypeKind_U128);}break;
          case E_TypeKind_S256:{type_key = e_type_key_basic(E_TypeKind_U256);}break;
          case E_TypeKind_S512:{type_key = e_type_key_basic(E_TypeKind_U512);}break;
        }
        
        // rjf: construct leaf type
        parse.expr = e_push_expr(arena, E_ExprKind_TypeIdent, token_string.str);
        parse.expr->type_key = type_key;
      }
    }
  }
  
  //- rjf: parse extensions
  if(parse.expr != &e_expr_nil)
  {
    for(;;)
    {
      E_Token token = e_token_at_it(token_it, tokens);
      if(token.kind != E_TokenKind_Symbol)
      {
        break;
      }
      String8 token_string = str8_substr(text, token.range);
      if(str8_match(token_string, str8_lit("*"), 0))
      {
        token_it += 1;
        E_Expr *ptee = parse.expr;
        parse.expr = e_push_expr(arena, E_ExprKind_Ptr, token_string.str);
        e_expr_push_child(parse.expr, ptee);
      }
      else
      {
        break;
      }
    }
  }
  
  //- rjf: fill parse & end
  parse.last_token = token_it;
  return parse;
}

internal E_Parse
e_parse_expr_from_text_tokens__prec(Arena *arena, String8 text, E_TokenArray *tokens, S64 max_precedence)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  E_Token *it = tokens->v;
  E_Token *it_opl = tokens->v + tokens->count;
  E_Parse result = {0, &e_expr_nil};
  
  //- rjf: parse prefix unaries
  typedef struct PrefixUnaryNode PrefixUnaryNode;
  struct PrefixUnaryNode
  {
    PrefixUnaryNode *next;
    E_ExprKind kind;
    E_Expr *cast_expr;
    void *location;
  };
  PrefixUnaryNode *first_prefix_unary = 0;
  PrefixUnaryNode *last_prefix_unary = 0;
  {
    for(;it < it_opl;)
    {
      E_Token *start_it = it;
      E_Token token = e_token_at_it(it, tokens);
      String8 token_string = str8_substr(text, token.range);
      S64 prefix_unary_precedence = 0;
      E_ExprKind prefix_unary_kind = 0;
      E_Expr *cast_expr = &e_expr_nil;
      void *location = 0;
      
      // rjf: try op table
      for(U64 idx = 0; idx < ArrayCount(e_unary_prefix_op_table); idx += 1)
      {
        if(str8_match(token_string, e_unary_prefix_op_table[idx].string, 0))
        {
          prefix_unary_precedence = e_unary_prefix_op_table[idx].precedence;
          prefix_unary_kind = e_unary_prefix_op_table[idx].kind;
          break;
        }
      }
      
      // rjf: consume valid op
      if(prefix_unary_precedence != 0)
      {
        location = token_string.str;
        it += 1;
      }
      
      // rjf: try casting expression
      if(prefix_unary_precedence == 0 && str8_match(token_string, str8_lit("("), 0))
      {
        E_Token some_type_identifier_maybe = e_token_at_it(it+1, tokens);
        String8 some_type_identifier_maybe_string = str8_substr(text, some_type_identifier_maybe.range);
        if(some_type_identifier_maybe.kind == E_TokenKind_Identifier)
        {
          E_TypeKey type_key = e_leaf_type_from_name(some_type_identifier_maybe_string);
          if(!e_type_key_match(type_key, e_type_key_zero()) || str8_match(some_type_identifier_maybe_string, str8_lit("unsigned"), 0))
          {
            // rjf: move past open paren
            it += 1;
            
            // rjf: parse type expr
            E_TokenArray type_parse_tokens = e_token_array_make_first_opl(it, it_opl);
            E_Parse type_parse = e_parse_type_from_text_tokens(arena, text, &type_parse_tokens);
            E_Expr *type = type_parse.expr;
            e_msg_list_concat_in_place(&result.msgs, &type_parse.msgs);
            it = type_parse.last_token;
            location = token_string.str;
            
            // rjf: expect )
            E_Token close_paren_maybe = e_token_at_it(it, tokens);
            String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
            if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit(")"), 0))
            {
              e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Missing ).");
            }
            
            // rjf: consume )
            else
            {
              it += 1;
            }
            
            // rjf: fill
            prefix_unary_precedence = 2;
            prefix_unary_kind = E_ExprKind_Cast;
            cast_expr = type;
          }
        }
      }
      
      // rjf: break if we got no operators
      if(prefix_unary_precedence == 0)
      {
        break;
      }
      
      // rjf: break if the token node iterator has not changed
      if(it == start_it)
      {
        break;
      }
      
      // rjf: push prefix unary if we got one
      {
        PrefixUnaryNode *op_n = push_array(scratch.arena, PrefixUnaryNode, 1);
        op_n->kind = prefix_unary_kind;
        op_n->cast_expr = cast_expr;
        op_n->location = location;
        SLLQueuePushFront(first_prefix_unary, last_prefix_unary, op_n);
      }
    }
  }
  
  //- rjf: parse atom
  E_Expr *atom = &e_expr_nil;
  String8 atom_implicit_member_name = {0};
  if(it < it_opl)
  {
    E_Token token = e_token_at_it(it, tokens);
    String8 token_string = str8_substr(text, token.range);
    
    //- rjf: descent to nested expression
    if(token.kind == E_TokenKind_Symbol && str8_match(token_string, str8_lit("("), 0))
    {
      // rjf: skip (
      it += 1;
      
      // rjf: parse () contents
      E_TokenArray nested_parse_tokens = e_token_array_make_first_opl(it, it_opl);
      E_Parse nested_parse = e_parse_expr_from_text_tokens__prec(arena, text, &nested_parse_tokens, e_max_precedence);
      e_msg_list_concat_in_place(&result.msgs, &nested_parse.msgs);
      atom = nested_parse.expr;
      it = nested_parse.last_token;
      
      // rjf: expect )
      E_Token close_paren_maybe = e_token_at_it(it, tokens);
      String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
      if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit(")"), 0))
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Missing ).");
      }
      
      // rjf: consume )
      else
      {
        it += 1;
      }
    }
    
    //- rjf: descent to assembly-style dereference sub-expression
    else if(token.kind == E_TokenKind_Symbol && str8_match(token_string, str8_lit("["), 0))
    {
      // rjf: skip [
      it += 1;
      
      // rjf: parse [] contents
      E_TokenArray nested_parse_tokens = e_token_array_make_first_opl(it, it_opl);
      E_Parse nested_parse = e_parse_expr_from_text_tokens__prec(arena, text, &nested_parse_tokens, e_max_precedence);
      e_msg_list_concat_in_place(&result.msgs, &nested_parse.msgs);
      atom = nested_parse.expr;
      it = nested_parse.last_token;
      
      // rjf: build cast-to-U64*, and dereference operators
      E_Expr *type = e_push_expr(arena, E_ExprKind_TypeIdent, token_string.str);
      type->type_key = e_type_key_cons(E_TypeKind_Ptr, e_type_key_basic(E_TypeKind_U64), 0);
      E_Expr *casted = atom;
      E_Expr *cast = e_push_expr(arena, E_ExprKind_Cast, token_string.str);
      e_expr_push_child(cast, type);
      e_expr_push_child(cast, casted);
      atom = e_push_expr(arena, E_ExprKind_Deref, token_string.str);
      e_expr_push_child(atom, cast);
      
      // rjf: expect ]
      E_Token close_paren_maybe = e_token_at_it(it, tokens);
      String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
      if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit("]"), 0))
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Missing ].");
      }
      
      // rjf: consume )
      else
      {
        it += 1;
      }
    }
    
    //- rjf: leaf (identifier, literal)
    else if(token.kind != E_TokenKind_Symbol)
    {
      switch(token.kind)
      {
        //- rjf: identifier => name resolution
        default:
        case E_TokenKind_Identifier:
        {
          B32 mapped_identifier = 0;
          B32 identifier_type_is_possibly_dynamically_overridden = 0;
          B32 identifier_looks_like_type_expr = 0;
          RDI_LocationKind        loc_kind = RDI_LocationKind_NULL;
          RDI_LocationReg         loc_reg = {0};
          RDI_LocationRegPlusU16  loc_reg_u16 = {0};
          String8                 loc_bytecode = {0};
          REGS_RegCode            reg_code = 0;
          REGS_AliasCode          alias_code = 0;
          E_TypeKey                  type_key = zero_struct;
          String8                 local_lookup_string = token_string;
          
          //- rjf: form namespaceified fallback versions of this lookup string
          String8List namespaceified_token_strings = {0};
          {
            U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(e_ctx->rdis[0], RDI_SectionKind_ScopeVMap, e_ctx->ip_voff);
            RDI_Scope *scope = rdi_element_from_name_idx(e_ctx->rdis[0], Scopes, scope_idx);
            U64 proc_idx = scope->proc_idx;
            RDI_Procedure *procedure = rdi_element_from_name_idx(e_ctx->rdis[0], Procedures, proc_idx);
            U64 name_size = 0;
            U8 *name_ptr = rdi_string_from_idx(e_ctx->rdis[0], procedure->name_string_idx, &name_size);
            String8 containing_procedure_name = str8(name_ptr, name_size);
            U64 last_past_scope_resolution_pos = 0;
            for(;;)
            {
              U64 past_next_dbl_colon_pos = str8_find_needle(containing_procedure_name, last_past_scope_resolution_pos, str8_lit("::"), 0)+2;
              U64 past_next_dot_pos = str8_find_needle(containing_procedure_name, last_past_scope_resolution_pos, str8_lit("."), 0)+1;
              U64 past_next_scope_resolution_pos = Min(past_next_dbl_colon_pos, past_next_dot_pos);
              if(past_next_scope_resolution_pos >= containing_procedure_name.size)
              {
                break;
              }
              String8 new_namespace_prefix_possibility = str8_prefix(containing_procedure_name, past_next_scope_resolution_pos);
              String8 namespaceified_token_string = push_str8f(scratch.arena, "%S%S", new_namespace_prefix_possibility, token_string);
              str8_list_push_front(scratch.arena, &namespaceified_token_strings, namespaceified_token_string);
              last_past_scope_resolution_pos = past_next_scope_resolution_pos;
            }
          }
          
          //- rjf: try members
          if(mapped_identifier == 0)
          {
            U64 data_member_num = e_num_from_string(e_ctx->member_map, token_string);
            if(data_member_num != 0)
            {
              atom_implicit_member_name = token_string;
              local_lookup_string = str8_lit("this");
            }
          }
          
          //- rjf: try locals
          if(mapped_identifier == 0)
          {
            U64 local_num = e_num_from_string(e_ctx->locals_map, local_lookup_string);
            if(local_num != 0)
            {
              mapped_identifier = 1;
              identifier_type_is_possibly_dynamically_overridden = 1;
              RDI_Local *local_var = rdi_element_from_name_idx(e_ctx->rdis[0], Locals, local_num-1);
              RDI_TypeNode *type_node = rdi_element_from_name_idx(e_ctx->rdis[0], TypeNodes, local_var->type_idx);
              type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), local_var->type_idx, 0);
              
              // rjf: grab location info
              for(U32 loc_block_idx = local_var->location_first;
                  loc_block_idx < local_var->location_opl;
                  loc_block_idx += 1)
              {
                RDI_LocationBlock *block = rdi_element_from_name_idx(e_ctx->rdis[0], LocationBlocks, loc_block_idx);
                if(block->scope_off_first <= e_ctx->ip_voff && e_ctx->ip_voff < block->scope_off_opl)
                {
                  U64 all_location_data_size = 0;
                  U8 *all_location_data = rdi_table_from_name(e_ctx->rdis[0], LocationData, &all_location_data_size);
                  loc_kind = *((RDI_LocationKind *)(all_location_data + block->location_data_off));
                  switch(loc_kind)
                  {
                    default:{mapped_identifier = 0;}break;
                    case RDI_LocationKind_AddrBytecodeStream:
                    case RDI_LocationKind_ValBytecodeStream:
                    {
                      U8 *bytecode_base = all_location_data + block->location_data_off + sizeof(RDI_LocationKind);
                      U64 bytecode_size = 0;
                      for(U64 idx = 0; idx < all_location_data_size; idx += 1)
                      {
                        U8 op = bytecode_base[idx];
                        if(op == 0)
                        {
                          break;
                        }
                        U8 ctrlbits = rdi_eval_op_ctrlbits_table[op];
                        U32 p_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
                        bytecode_size += 1+p_size;
                      }
                      loc_bytecode = str8(bytecode_base, bytecode_size);
                    }break;
                    case RDI_LocationKind_AddrRegPlusU16:
                    case RDI_LocationKind_AddrAddrRegPlusU16:
                    {
                      MemoryCopy(&loc_reg_u16, (all_location_data + block->location_data_off), sizeof(loc_reg_u16));
                    }break;
                    case RDI_LocationKind_ValReg:
                    {
                      MemoryCopy(&loc_reg, (all_location_data + block->location_data_off), sizeof(loc_reg));
                    }break;
                  }
                }
              }
            }
          }
          
          //- rjf: try registers
          if(mapped_identifier == 0)
          {
            U64 reg_num = e_num_from_string(e_ctx->regs_map, token_string);
            if(reg_num != 0)
            {
              reg_code = reg_num;
              mapped_identifier = 1;
              type_key = e_type_key_reg(e_ctx->arch, reg_code);
            }
          }
          
          //- rjf: try register aliases
          if(mapped_identifier == 0)
          {
            U64 alias_num = e_num_from_string(e_ctx->reg_alias_map, token_string);
            if(alias_num != 0)
            {
              alias_code = (REGS_AliasCode)alias_num;
              type_key = e_type_key_reg_alias(e_ctx->arch, alias_code);
              mapped_identifier = 1;
            }
          }
          
          //- rjf: try global variables
          if(mapped_identifier == 0)
          {
            for(U64 rdi_idx = 0; rdi_idx < e_ctx->rdis_count; rdi_idx += 1)
            {
              RDI_Parsed *rdi = e_ctx->rdis[rdi_idx];
              RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_GlobalVariables);
              RDI_ParsedNameMap parsed_name_map = {0};
              rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
              RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, token_string.str, token_string.size);
              U32 matches_count = 0;
              U32 *matches = rdi_matches_from_map_node(rdi, node, &matches_count);
              for(String8Node *n = namespaceified_token_strings.first;
                  n != 0 && matches_count == 0;
                  n = n->next)
              {
                node = rdi_name_map_lookup(rdi, &parsed_name_map, n->string.str, n->string.size);
                matches_count = 0;
                matches = rdi_matches_from_map_node(rdi, node, &matches_count);
              }
              if(matches_count != 0)
              {
                // NOTE(rjf): apparently, PDBs can be produced such that they
                // also keep stale *GLOBAL VARIABLE SYMBOLS* around too. I
                // don't know of a magic hash table fixup path in PDBs, so
                // in this case, I'm going to prefer the latest-added global.
                U32 match_idx = matches[matches_count-1];
                RDI_GlobalVariable *global_var = rdi_element_from_name_idx(rdi, GlobalVariables, match_idx);
                E_OpList oplist = {0};
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_ModuleOff, global_var->voff);
                loc_kind = RDI_LocationKind_AddrBytecodeStream;
                loc_bytecode = e_bytecode_from_oplist(arena, &oplist);
                U32 type_idx = global_var->type_idx;
                RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, type_idx);
                type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)rdi_idx);
                mapped_identifier = 1;
                break;
              }
            }
          }
          
          //- rjf: try thread variables
          if(mapped_identifier == 0)
          {
            for(U64 rdi_idx = 0; rdi_idx < e_ctx->rdis_count; rdi_idx += 1)
            {
              RDI_Parsed *rdi = e_ctx->rdis[rdi_idx];
              RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_ThreadVariables);
              RDI_ParsedNameMap parsed_name_map = {0};
              rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
              RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, token_string.str, token_string.size);
              U32 matches_count = 0;
              U32 *matches = rdi_matches_from_map_node(rdi, node, &matches_count);
              for(String8Node *n = namespaceified_token_strings.first;
                  n != 0 && matches_count == 0;
                  n = n->next)
              {
                node = rdi_name_map_lookup(rdi, &parsed_name_map, n->string.str, n->string.size);
                matches_count = 0;
                matches = rdi_matches_from_map_node(rdi, node, &matches_count);
              }
              if(matches_count != 0)
              {
                U32 match_idx = matches[0];
                RDI_ThreadVariable *thread_var = rdi_element_from_name_idx(rdi, ThreadVariables, match_idx);
                E_OpList oplist = {0};
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_TLSOff, thread_var->tls_off);
                loc_kind = RDI_LocationKind_AddrBytecodeStream;
                loc_bytecode = e_bytecode_from_oplist(arena, &oplist);
                U32 type_idx = thread_var->type_idx;
                RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, type_idx);
                type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)rdi_idx);
                mapped_identifier = 1;
                break;
              }
            }
          }
          
          //- rjf: try procedures
          if(mapped_identifier == 0)
          {
            for(U64 rdi_idx = 0; rdi_idx < e_ctx->rdis_count; rdi_idx += 1)
            {
              RDI_Parsed *rdi = e_ctx->rdis[rdi_idx];
              RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_Procedures);
              RDI_ParsedNameMap parsed_name_map = {0};
              rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
              RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, token_string.str, token_string.size);
              U32 matches_count = 0;
              U32 *matches = rdi_matches_from_map_node(rdi, node, &matches_count);
              for(String8Node *n = namespaceified_token_strings.first;
                  n != 0 && matches_count == 0;
                  n = n->next)
              {
                node = rdi_name_map_lookup(rdi, &parsed_name_map, n->string.str, n->string.size);
                matches_count = 0;
                matches = rdi_matches_from_map_node(rdi, node, &matches_count);
              }
              if(matches_count != 0)
              {
                U32 match_idx = matches[0];
                RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, match_idx);
                RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, procedure->root_scope_idx);
                U64 voff = *rdi_element_from_name_idx(rdi, ScopeVOffData, scope->voff_range_first);
                E_OpList oplist = {0};
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_ModuleOff, voff);
                loc_kind = RDI_LocationKind_ValBytecodeStream;
                loc_bytecode = e_bytecode_from_oplist(arena, &oplist);
                U32 type_idx = procedure->type_idx;
                RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, type_idx);
                type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)rdi_idx);
                mapped_identifier = 1;
                break;
              }
            }
          }
          
          //- rjf: try types
          if(mapped_identifier == 0)
          {
            type_key = e_leaf_type_from_name(token_string);
            if(!e_type_key_match(e_type_key_zero(), type_key))
            {
              mapped_identifier = 1;
              identifier_looks_like_type_expr = 1;
            }
          }
          
          //- rjf: attach on map
          if(mapped_identifier != 0)
          {
            it += 1;
            
            // rjf: build atom
            switch(loc_kind)
            {
              default:
              {
                if(identifier_looks_like_type_expr)
                {
                  E_TokenArray type_parse_tokens = e_token_array_make_first_opl(it-1, it_opl);
                  E_Parse type_parse = e_parse_type_from_text_tokens(arena, text, &type_parse_tokens);
                  E_Expr *type = type_parse.expr;
                  e_msg_list_concat_in_place(&result.msgs, &type_parse.msgs);
                  it = type_parse.last_token;
                  atom = type;
                }
                else if(reg_code != 0)
                {
                  REGS_Rng reg_rng = regs_reg_code_rng_table_from_architecture(e_ctx->arch)[reg_code];
                  E_OpList oplist = {0};
                  e_oplist_push_uconst(arena, &oplist, reg_rng.byte_off);
                  atom = e_push_expr(arena, E_ExprKind_LeafBytecode, token_string.str);
                  atom->mode = E_Mode_Reg;
                  atom->type_key = type_key;
                  atom->string = e_bytecode_from_oplist(arena, &oplist);
                }
                else if(alias_code != 0)
                {
                  REGS_Slice alias_slice = regs_alias_code_slice_table_from_architecture(e_ctx->arch)[alias_code];
                  REGS_Rng alias_reg_rng = regs_reg_code_rng_table_from_architecture(e_ctx->arch)[alias_slice.code];
                  E_OpList oplist = {0};
                  e_oplist_push_uconst(arena, &oplist, alias_reg_rng.byte_off + alias_slice.byte_off);
                  atom = e_push_expr(arena, E_ExprKind_LeafBytecode, token_string.str);
                  atom->mode = E_Mode_Reg;
                  atom->type_key = type_key;
                  atom->string = e_bytecode_from_oplist(arena, &oplist);
                }
                else
                {
                  e_msgf(arena, &result.msgs, E_MsgKind_MissingInfo, token_string.str, "Missing location information for \"%S\".", token_string);
                }
              }break;
              case RDI_LocationKind_AddrBytecodeStream:
              {
                atom = e_push_expr(arena, E_ExprKind_LeafBytecode, token_string.str);
                atom->mode = E_Mode_Addr;
                atom->type_key = type_key;
                atom->string = loc_bytecode;
              }break;
              case RDI_LocationKind_ValBytecodeStream:
              {
                atom = e_push_expr(arena, E_ExprKind_LeafBytecode, token_string.str);
                atom->mode = E_Mode_Value;
                atom->type_key = type_key;
                atom->string = loc_bytecode;
              }break;
              case RDI_LocationKind_AddrRegPlusU16:
              {
                E_OpList oplist = {0};
                U64 byte_size = bit_size_from_arch(e_ctx->arch)/8;
                U64 regread_param = RDI_EncodeRegReadParam(loc_reg_u16.reg_code, byte_size, 0);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_RegRead, regread_param);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_ConstU16, loc_reg_u16.offset);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_Add, 0);
                atom = e_push_expr(arena, E_ExprKind_LeafBytecode, token_string.str);
                atom->mode = E_Mode_Addr;
                atom->type_key = type_key;
                atom->string = e_bytecode_from_oplist(arena, &oplist);
              }break;
              case RDI_LocationKind_AddrAddrRegPlusU16:
              {
                E_OpList oplist = {0};
                U64 byte_size = bit_size_from_arch(e_ctx->arch)/8;
                U64 regread_param = RDI_EncodeRegReadParam(loc_reg_u16.reg_code, byte_size, 0);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_RegRead, regread_param);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_ConstU16, loc_reg_u16.offset);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_Add, 0);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_MemRead, bit_size_from_arch(e_ctx->arch)/8);
                atom = e_push_expr(arena, E_ExprKind_LeafBytecode, token_string.str);
                atom->mode = E_Mode_Addr;
                atom->type_key = type_key;
                atom->string = e_bytecode_from_oplist(arena, &oplist);
              }break;
              case RDI_LocationKind_ValReg:
              {
                REGS_RegCode regs_reg_code = regs_reg_code_from_arch_rdi_code(e_ctx->arch, loc_reg.reg_code);
                REGS_Rng reg_rng = regs_reg_code_rng_table_from_architecture(e_ctx->arch)[regs_reg_code];
                E_OpList oplist = {0};
                U64 byte_size = (U64)reg_rng.byte_size;
                U64 byte_pos = 0;
                U64 regread_param = RDI_EncodeRegReadParam(loc_reg.reg_code, byte_size, byte_pos);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_RegRead, regread_param);
                atom = e_push_expr(arena, E_ExprKind_LeafBytecode, token_string.str);
                atom->mode = E_Mode_Value;
                atom->type_key = type_key;
                atom->string = e_bytecode_from_oplist(arena, &oplist);
              }break;
            }
            
            // rjf: implicit local lookup -> attach member access node
            if(atom_implicit_member_name.size != 0)
            {
              E_Expr *member_container = atom;
              E_Expr *member_expr = e_push_expr(arena, E_ExprKind_LeafMember, atom_implicit_member_name.str);
              member_expr->string = atom_implicit_member_name;
              atom = e_push_expr(arena, E_ExprKind_MemberAccess, atom_implicit_member_name.str);
              e_expr_push_child(atom, member_container);
              e_expr_push_child(atom, member_expr);
            }
          }
          
          //- rjf: map failure -> attach as leaf identifier, to be resolved later
          if(!mapped_identifier)
          {
            atom = e_push_expr(arena, E_ExprKind_LeafIdent, token_string.str);
            atom->string = token_string;
            it += 1;
          }
        }break;
        
        //- rjf: numeric => directly extract value
        case E_TokenKind_Numeric:
        {
          U64 dot_pos = str8_find_needle(token_string, 0, str8_lit("."), 0);
          it += 1;
          
          // rjf: no . => integral
          if(dot_pos == token_string.size)
          {
            U64 val = 0;
            try_u64_from_str8_c_rules(token_string, &val);
            atom = e_push_expr(arena, E_ExprKind_LeafU64, token_string.str);
            atom->u64 = val;
            break;
          }
          
          // rjf: presence of . => double or float
          if(dot_pos < token_string.size)
          {
            F64 val = f64_from_str8(token_string);
            U64 f_pos = str8_find_needle(token_string, 0, str8_lit("f"), StringMatchFlag_CaseInsensitive);
            
            // rjf: presence of f after . => f32
            if(f_pos < token_string.size)
            {
              atom = e_push_expr(arena, E_ExprKind_LeafF32, token_string.str);
              atom->f32 = (F32)val;
            }
            
            // rjf: no f => f64
            else
            {
              atom = e_push_expr(arena, E_ExprKind_LeafF64, token_string.str);
              atom->f64 = val;
            }
          }
        }break;
        
        //- rjf: char => extract char value
        case E_TokenKind_CharLiteral:
        {
          it += 1;
          if(token_string.size > 1 && token_string.str[0] == '\'' && token_string.str[1] != '\'')
          {
            U8 char_val = token_string.str[1];
            atom = e_push_expr(arena, E_ExprKind_LeafU64, token_string.str);
            atom->u64 = (U64)char_val;
          }
          else
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Malformed character literal.");
          }
        }break;
        
        // rjf: string => invalid
        case E_TokenKind_StringLiteral:
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "String literals are not supported.");
          it += 1;
        }break;
        
      }
    }
  }
  
  //- rjf: upgrade atom w/ postfix unaries
  if(atom != &e_expr_nil) for(;it < it_opl;)
  {
    E_Token token = e_token_at_it(it, tokens);
    String8 token_string = str8_substr(text, token.range);
    B32 is_postfix_unary = 0;
    
    // rjf: dot/arrow operator
    if(token.kind == E_TokenKind_Symbol &&
       (str8_match(token_string, str8_lit("."), 0) ||
        str8_match(token_string, str8_lit("->"), 0)))
    {
      is_postfix_unary = 1;
      
      // rjf: advance past operator
      it += 1;
      
      // rjf: expect member name
      String8 member_name = {0};
      B32 good_member_name = 0;
      {
        E_Token member_name_maybe = e_token_at_it(it, tokens);
        String8 member_name_maybe_string = str8_substr(text, member_name_maybe.range);
        if(member_name_maybe.kind != E_TokenKind_Identifier)
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Expected member name after %S.", token_string);
        }
        else
        {
          member_name = member_name_maybe_string;
          good_member_name = 1;
        }
      }
      
      // rjf: produce lookup member expr
      if(good_member_name)
      {
        E_Expr *member_container = atom;
        E_Expr *member_expr = e_push_expr(arena, E_ExprKind_LeafMember, member_name.str);
        atom = e_push_expr(arena, E_ExprKind_MemberAccess, token_string.str);
        e_expr_push_child(atom, member_container);
        e_expr_push_child(atom, member_expr);
      }
      
      // rjf: increment past good member names
      if(good_member_name)
      {
        it += 1;
      }
    }
    
    // rjf: array index
    if(token.kind == E_TokenKind_Symbol &&
       str8_match(token_string, str8_lit("["), 0))
    {
      is_postfix_unary = 1;
      
      // rjf: advance past [
      it += 1;
      
      // rjf: parse indexing expression
      E_TokenArray idx_expr_parse_tokens = e_token_array_make_first_opl(it, it_opl);
      E_Parse idx_expr_parse = e_parse_expr_from_text_tokens__prec(arena, text, &idx_expr_parse_tokens, e_max_precedence);
      e_msg_list_concat_in_place(&result.msgs, &idx_expr_parse.msgs);
      it = idx_expr_parse.last_token;
      
      // rjf: valid indexing expression => produce index expr
      if(idx_expr_parse.expr != &e_expr_nil)
      {
        E_Expr *array_expr = atom;
        E_Expr *index_expr = idx_expr_parse.expr;
        atom = e_push_expr(arena, E_ExprKind_ArrayIndex, token_string.str);
        e_expr_push_child(atom, array_expr);
        e_expr_push_child(atom, index_expr);
      }
      
      // rjf: expect ]
      {
        E_Token close_brace_maybe = e_token_at_it(it, tokens);
        String8 close_brace_maybe_string = str8_substr(text, close_brace_maybe.range);
        if(close_brace_maybe.kind != E_TokenKind_Symbol || !str8_match(close_brace_maybe_string, str8_lit("]"), 0))
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Unclosed [.");
        }
        else
        {
          it += 1;
        }
      }
    }
    
    // rjf: quit if this doesn't look like any patterns of postfix unary we know
    if(!is_postfix_unary)
    {
      break;
    }
  }
  
  //- rjf: upgrade atom w/ previously parsed prefix unaries
  if(atom == &e_expr_nil && first_prefix_unary != 0 && first_prefix_unary->cast_expr != 0)
  {
    atom = first_prefix_unary->cast_expr;
    for(PrefixUnaryNode *prefix_unary = first_prefix_unary->next;
        prefix_unary != 0;
        prefix_unary = prefix_unary->next)
    {
      E_Expr *rhs = atom;
      atom = e_push_expr(arena, prefix_unary->kind, prefix_unary->location);
      if(prefix_unary->cast_expr != &e_expr_nil)
      {
        e_expr_push_child(atom, prefix_unary->cast_expr);
      }
      e_expr_push_child(atom, rhs);
    }
  }
  else if(atom == 0 && first_prefix_unary != 0)
  {
    e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, last_prefix_unary->location, "Missing expression.");
  }
  else
  {
    for(PrefixUnaryNode *prefix_unary = first_prefix_unary;
        prefix_unary != 0;
        prefix_unary = prefix_unary->next)
    {
      E_Expr *rhs = atom;
      atom = e_push_expr(arena, prefix_unary->kind, prefix_unary->location);
      if(prefix_unary->cast_expr != &e_expr_nil)
      {
        e_expr_push_child(atom, prefix_unary->cast_expr);
      }
      e_expr_push_child(atom, rhs);
    }
  }
  
  //- rjf: parse complex operators
  if(atom != &e_expr_nil) for(;it < it_opl;)
  {
    E_Token *start_it = it;
    E_Token token = e_token_at_it(it, tokens);
    String8 token_string = str8_substr(text, token.range);
    
    //- rjf: parse binaries
    {
      // rjf: first try to find a matching binary operator
      S64 binary_precedence = 0;
      E_ExprKind binary_kind = 0;
      for(U64 idx = 0; idx < ArrayCount(e_binary_op_table); idx += 1)
      {
        if(str8_match(token_string, e_binary_op_table[idx].string, 0))
        {
          binary_precedence = e_binary_op_table[idx].precedence;
          binary_kind = e_binary_op_table[idx].kind;
          break;
        }
      }
      
      // rjf: if we got a valid binary precedence, and it's not to be handled by
      // a caller, then we need to parse the right-hand-side with a tighter
      // precedence
      if(binary_precedence != 0 && binary_precedence <= max_precedence)
      {
        E_TokenArray rhs_expr_parse_tokens = e_token_array_make_first_opl(it+1, it_opl);
        E_Parse rhs_expr_parse = e_parse_expr_from_text_tokens__prec(arena, text, &rhs_expr_parse_tokens, binary_precedence-1);
        e_msg_list_concat_in_place(&result.msgs, &rhs_expr_parse.msgs);
        E_Expr *rhs = rhs_expr_parse.expr;
        it = rhs_expr_parse.last_token;
        if(rhs == &e_expr_nil)
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Missing right-hand-side of %S.", token_string);
        }
        else
        {
          E_Expr *lhs = atom;
          atom = e_push_expr(arena, binary_kind, token_string.str);
          e_expr_push_child(atom, lhs);
          e_expr_push_child(atom, rhs);
        }
      }
    }
    
    //- rjf: parse ternaries
    {
      if(token.kind == E_TokenKind_Symbol && str8_match(token_string, str8_lit("?"), 0) && 13 <= max_precedence)
      {
        // rjf: parse middle expression
        E_TokenArray middle_expr_tokens = e_token_array_make_first_opl(it, it_opl);
        E_Parse middle_expr_parse = e_parse_expr_from_text_tokens__prec(arena, text, &middle_expr_tokens, e_max_precedence);
        it = middle_expr_parse.last_token;
        E_Expr *middle_expr = middle_expr_parse.expr;
        e_msg_list_concat_in_place(&result.msgs, &middle_expr_parse.msgs);
        if(middle_expr_parse.expr == &e_expr_nil)
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Expected expression after ?.");
        }
        
        // rjf: expect :
        B32 got_colon = 0;
        E_Token colon_token = zero_struct;
        String8 colon_token_string = {0};
        {
          E_Token colon_token_maybe = e_token_at_it(it, tokens);
          String8 colon_token_maybe_string = str8_substr(text, colon_token_maybe.range);
          if(colon_token_maybe.kind != E_TokenKind_Symbol || !str8_match(colon_token_maybe_string, str8_lit(":"), 0))
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Expected : after ?.");
          }
          else
          {
            got_colon = 1;
            colon_token = colon_token_maybe;
            colon_token_string = colon_token_maybe_string;
            it += 1;
          }
        }
        
        // rjf: parse rhs
        E_TokenArray rhs_expr_parse_tokens = e_token_array_make_first_opl(it, it_opl);
        E_Parse rhs_expr_parse = e_parse_expr_from_text_tokens__prec(arena, text, &rhs_expr_parse_tokens, e_max_precedence);
        if(got_colon)
        {
          it = rhs_expr_parse.last_token;
          e_msg_list_concat_in_place(&result.msgs, &rhs_expr_parse.msgs);
          if(rhs_expr_parse.expr == &e_expr_nil)
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, colon_token_string.str, "Expected expression after :.");
          }
        }
        
        // rjf: build ternary
        {
          E_Expr *lhs = atom;
          E_Expr *mhs = middle_expr_parse.expr;
          E_Expr *rhs = rhs_expr_parse.expr;
          atom = e_push_expr(arena, E_ExprKind_Ternary, token_string.str);
          e_expr_push_child(atom, lhs);
          e_expr_push_child(atom, mhs);
          e_expr_push_child(atom, rhs);
        }
      }
    }
    
    // rjf: if we parsed nothing successfully, we're done
    if(it == start_it)
    {
      break;
    }
  }
  
  //- rjf: fill result & return
  result.last_token = it;
  result.expr = atom;
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal E_Parse
e_parse_expr_from_text_tokens(Arena *arena, String8 text, E_TokenArray *tokens)
{
  E_Parse parse = e_parse_expr_from_text_tokens__prec(arena, text, tokens, eval_g_max_precedence);
  return parse;
}

////////////////////////////////
//~ rjf: IR-ization Functions

internal void
e_oplist_push(Arena *arena, E_OpList *list, RDI_EvalOp opcode, U64 p)
{
  U8 ctrlbits = rdi_eval_op_ctrlbits_table[opcode];
  U32 p_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
  E_Op *node = push_array_no_zero(arena, E_Op, 1);
  node->opcode = opcode;
  node->p = p;
  SLLQueuePush(list->first, list->last, node);
  list->op_count += 1;
  list->encoded_size += 1 + p_size;
}

internal E_IRTreeAndType e_irtree_and_type_from_expr(Arena *arena, E_Expr *expr);
internal E_OpList e_oplist_from_irtree(Arena *arena, E_IRNode *root);
internal void e_append_oplist_from_irtree(Arena *arena, E_IRNode *root, E_OpList *out);

internal String8
e_bytecode_from_oplist(Arena *arena, E_OpList *oplist)
{
  // rjf: allocate buffer
  U64 size = oplist->encoded_size;
  U8 *str = push_array_no_zero(arena, U8, size);
  
  // rjf: iterate loose op nodes; fill buffer
  U8 *ptr = str;
  U8 *opl = str + size;
  for(E_Op *op = oplist->first;
      op != 0;
      op = op->next)
  {
    U32 opcode = op->opcode;
    switch(opcode)
    {
      default:
      {
        // rjf: compute bytecode advance
        U8 ctrlbits = rdi_eval_op_ctrlbits_table[opcode];
        U64 extra_byte_count = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
        U8 *next_ptr = ptr + 1 + extra_byte_count;
        Assert(next_ptr <= opl);
        
        // rjf: fill bytecode
        ptr[0] = opcode;
        MemoryCopy(ptr + 1, &op->p, extra_byte_count);
        
        // rjf: advance
        ptr = next_ptr;
      }break;
      
      case E_IRExtKind_Bytecode:
      {
        // rjf: compute bytecode advance
        U64 size = op->bytecode.size;
        U8 *next_ptr = ptr + size;
        Assert(next_ptr <= opl);
        
        // rjf: fill bytecode
        MemoryCopy(ptr, op->bytecode.str, size);
        
        // rjf: advance
        ptr = next_ptr;
      }break;
    }
  }
  
  // rjf: fill result
  String8 result = {0};
  result.size = size;
  result.str = str;
  return result;
}

////////////////////////////////
//~ rjf: Interpretation Functions

internal E_Result
e_interpret(String8 bytecode)
{
  E_Result result = {0};
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: allocate stack
  U64 stack_cap = 128; // TODO(rjf): scan bytecode; determine maximum stack depth
  E_Value *stack = push_array_no_zero(scratch.arena, E_Value, stack_cap);
  U64 stack_count = 0;
  
  //- rjf: iterate bytecode & perform ops
  U8 *ptr = bytecode.str;
  U8 *opl = bytecode.str + bytecode.size;
  for(;ptr < opl;)
  {
    // rjf: consume next opcode
    RDI_EvalOp op = (RDI_EvalOp)*ptr;
    if(op >= RDI_EvalOp_COUNT)
    {
      result.code = E_ResultCode_BadOp;
      goto done;
    }
    U8 ctrlbits = rdi_eval_op_ctrlbits_table[op];
    ptr += 1;
    
    // rjf: decode
    U64 imm = 0;
    {
      U32 decode_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
      U8 *next_ptr = ptr + decode_size;
      if(next_ptr > opl)
      {
        result.code = E_ResultCode_BadOp;
        goto done;
      }
      // TODO(rjf): guarantee 8 bytes padding after the end of serialized
      // bytecode; read 8 bytes and mask
      switch(decode_size)
      {
        case 1:{imm = *ptr;}break;
        case 2:{imm = *(U16*)ptr;}break;
        case 4:{imm = *(U32*)ptr;}break;
        case 8:{imm = *(U64*)ptr;}break;
      }
      ptr = next_ptr;
    }
    
    // rjf: pop
    E_Value *svals = 0;
    {
      U32 pop_count = RDI_POPN_FROM_CTRLBITS(ctrlbits);
      if(pop_count > stack_count)
      {
        result.code = E_ResultCode_BadOp;
        goto done;
      }
      if(pop_count <= stack_count)
      {
        stack_count -= pop_count;
        svals = stack + stack_count;
      }
    }
    
    // rjf: interpret op, given decodes/pops
    E_Value nval = {0};
    switch(op)
    {
      case RDI_EvalOp_Stop:
      {
        goto done;
      }break;
      
      case RDI_EvalOp_Noop:
      {
        // do nothing
      }break;
      
      case RDI_EvalOp_Cond:
      if(svals[0].u64)
      {
        ptr += imm;
      }break;
      
      case RDI_EvalOp_Skip:
      {
        ptr += imm;
      }break;
      
      case RDI_EvalOp_MemRead:
      {
        U64 addr = svals[0].u64;
        U64 size = imm;
        B32 good_read = 0;
        if(e_ctx->memory_read != 0 && e_ctx->memory_read(e_ctx->memory_read_user_data, &nval, r1u64(addr, addr+size)))
        {
          good_read = 1;
        }
        if(!good_read)
        {
          result.code = E_ResultCode_BadMemRead;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_RegRead:
      {
        U8 rdi_reg_code     = (imm&0x0000FF)>>0;
        U8 byte_size        = (imm&0x00FF00)>>8;
        U8 byte_off         = (imm&0xFF0000)>>16;
        REGS_RegCode base_reg_code = regs_reg_code_from_arch_rdi_code(e_ctx->arch, rdi_reg_code);
        REGS_Rng rng = regs_reg_code_rng_table_from_architecture(e_ctx->arch)[base_reg_code];
        U64 off = (U64)rng.byte_off + byte_off;
        U64 size = (U64)byte_size;
        if(off + size <= e_ctx->reg_size)
        {
          MemoryCopy(&nval, (U8*)e_ctx->reg_data + off, size);
        }
        else
        {
          result.code = E_ResultCode_BadRegRead;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_RegReadDyn:
      {
        U64 off  = svals[0].u64;
        U64 size = bit_size_from_arch(e_ctx->arch)/8;
        if(off + size <= e_ctx->reg_size)
        {
          MemoryCopy(&nval, (U8*)e_ctx->reg_data + off, size);
        }
        else
        {
          result.code = E_ResultCode_BadRegRead;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_FrameOff:
      {
        if(e_ctx->frame_base != 0)
        {
          nval.u64 = *e_ctx->frame_base + imm;
        }
        else
        {
          result.code = E_ResultCode_BadFrameBase;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_ModuleOff:
      {
        if(e_ctx->module_base != 0)
        {
          nval.u64 = *e_ctx->module_base + imm;
        }
        else
        {
          result.code = E_ResultCode_BadModuleBase;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_TLSOff:
      {
        if(e_ctx->tls_base != 0)
        {
          nval.u64 = *e_ctx->tls_base + imm;
        }
        else
        {
          result.code = E_ResultCode_BadTLSBase;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_ConstU8:
      case RDI_EvalOp_ConstU16:
      case RDI_EvalOp_ConstU32:
      case RDI_EvalOp_ConstU64:
      {
        nval.u64 = imm;
      }break;
      
      case RDI_EvalOp_Abs:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32;
          if(svals[0].f32 < 0)
          {
            nval.f32 = -svals[0].f32;
          }
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.f64 = svals[0].f64;
          if(svals[0].f64 < 0)
          {
            nval.f64 = -svals[0].f64;
          }
        }
        else
        {
          nval.s64 = svals[0].s64;
          if(svals[0].s64 < 0)
          {
            nval.s64 = -svals[0].s64;
          }
        }
      }break;
      
      case RDI_EvalOp_Neg:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = -svals[0].f32;
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.f64 = -svals[0].f64;
        }
        else
        {
          nval.u64 = (~svals[0].u64) + 1;
        }
      }break;
      
      case RDI_EvalOp_Add:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32 + svals[1].f32;
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.f64 = svals[0].f64 + svals[1].f64;
        }
        else
        {
          nval.u64 = svals[0].u64 + svals[1].u64;
        }
      }break;
      
      case RDI_EvalOp_Sub:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32 - svals[1].f32;
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.f64 = svals[0].f64 - svals[1].f64;
        }
        else
        {
          nval.u64 = svals[0].u64 - svals[1].u64;
        }
      }break;
      
      case RDI_EvalOp_Mul:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32*svals[1].f32;
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.f64 = svals[0].f64*svals[1].f64;
        }
        else
        {
          nval.u64 = svals[0].u64*svals[1].u64;
        }
      }break;
      
      case RDI_EvalOp_Div:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          if(svals[1].f32 != 0.f)
          {
            nval.f32 = svals[0].f32/svals[1].f32;
          }
          else
          {
            result.code = E_ResultCode_DivideByZero;
            goto done;
          }
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          if(svals[1].f64 != 0.)
          {
            nval.f64 = svals[0].f64/svals[1].f64;
          }
          else
          {
            result.code = E_ResultCode_DivideByZero;
            goto done;
          }
        }
        else if(imm == RDI_EvalTypeGroup_U ||
                imm == RDI_EvalTypeGroup_S)
        {
          if(svals[1].u64 != 0)
          {
            nval.u64 = svals[0].u64/svals[1].u64;
          }
          else
          {
            result.code = E_ResultCode_DivideByZero;
            goto done;
          }
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_Mod:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          if(svals[1].u64 != 0)
          {
            nval.u64 = svals[0].u64%svals[1].u64;
          }
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_LShift:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = svals[0].u64 << svals[1].u64;
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_RShift:
      {
        if(imm == RDI_EvalTypeGroup_U)
        {
          nval.u64 = svals[0].u64 >> svals[1].u64;
        }
        else if(imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = svals[0].s64 >> svals[1].u64;
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_BitAnd:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = svals[0].u64&svals[1].u64;
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_BitOr:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = svals[0].u64|svals[1].u64;
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_BitXor:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = svals[0].u64^svals[1].u64;
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_BitNot:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = ~svals[0].u64;
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_LogAnd:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].u64 && svals[1].u64);
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_LogOr:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].u64 || svals[1].u64);
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_LogNot:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (!svals[0].u64);
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_EqEq:
      {
        nval.u64 = (svals[0].u64 == svals[1].u64);
      }break;
      
      case RDI_EvalOp_NtEq:
      {
        nval.u64 = (svals[0].u64 != svals[1].u64);
      }break;
      
      case RDI_EvalOp_LsEq:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 <= svals[1].f32);
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 <= svals[1].f64);
        }
        else if(imm == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 <= svals[1].u64);
        }
        else if(imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].s64 <= svals[1].s64);
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_GrEq:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 >= svals[1].f32);
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 >= svals[1].f64);
        }
        else if(imm == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 >= svals[1].u64);
        }
        else if(imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].s64 >= svals[1].s64);
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_Less:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 < svals[1].f32);
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 < svals[1].f64);
        }
        else if(imm == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 < svals[1].u64);
        }
        else if(imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].s64 < svals[1].s64);
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_Grtr:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 > svals[1].f32);
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 > svals[1].f64);
        }
        else if(imm == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 > svals[1].u64);
        }
        else if(imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].s64 > svals[1].s64);
        }
        else
        {
          result.code = E_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_Trunc:
      {
        if(0 < imm)
        {
          U64 mask = 0;
          if(imm < 64)
          {
            mask = max_U64 >> (64 - imm);
          }
          nval.u64 = svals[0].u64&mask;
        }
      }break;
      
      case RDI_EvalOp_TruncSigned:
      {
        if(0 < imm)
        {
          U64 mask = 0;
          if(imm < 64)
          {
            mask = max_U64 >> (64 - imm);
          }
          U64 high = 0;
          if(svals[0].u64 & (1 << (imm - 1)))
          {
            high = ~mask;
          }
          nval.u64 = high|(svals[0].u64&mask);
        }
      }break;
      
      case RDI_EvalOp_Convert:
      {
        U32 in = imm&0xFF;
        U32 out = (imm >> 8)&0xFF;
        if(in != out)
        {
          switch(in + out*RDI_EvalTypeGroup_COUNT)
          {
            case RDI_EvalTypeGroup_F32 + RDI_EvalTypeGroup_U*RDI_EvalTypeGroup_COUNT:
            {
              nval.u64 = (U64)svals[0].f32;
            }break;
            case RDI_EvalTypeGroup_F64 + RDI_EvalTypeGroup_U*RDI_EvalTypeGroup_COUNT:
            {
              nval.u64 = (U64)svals[0].f64;
            }break;
            
            case RDI_EvalTypeGroup_F32 + RDI_EvalTypeGroup_S*RDI_EvalTypeGroup_COUNT:
            {
              nval.s64 = (S64)svals[0].f32;
            }break;
            case RDI_EvalTypeGroup_F64 + RDI_EvalTypeGroup_S*RDI_EvalTypeGroup_COUNT:
            {
              nval.s64 = (S64)svals[0].f64;
            }break;
            
            case RDI_EvalTypeGroup_U + RDI_EvalTypeGroup_F32*RDI_EvalTypeGroup_COUNT:
            {
              nval.f32 = (F32)svals[0].u64;
            }break;
            case RDI_EvalTypeGroup_S + RDI_EvalTypeGroup_F32*RDI_EvalTypeGroup_COUNT:
            {
              nval.f32 = (F32)svals[0].s64;
            }break;
            case RDI_EvalTypeGroup_F64 + RDI_EvalTypeGroup_F32*RDI_EvalTypeGroup_COUNT:
            {
              nval.f32 = (F32)svals[0].f64;
            }break;
            
            case RDI_EvalTypeGroup_U + RDI_EvalTypeGroup_F64*RDI_EvalTypeGroup_COUNT:
            {
              nval.f64 = (F64)svals[0].u64;
            }break;
            case RDI_EvalTypeGroup_S + RDI_EvalTypeGroup_F64*RDI_EvalTypeGroup_COUNT:
            {
              nval.f64 = (F64)svals[0].s64;
            }break;
            case RDI_EvalTypeGroup_F32 + RDI_EvalTypeGroup_F64*RDI_EvalTypeGroup_COUNT:
            {
              nval.f64 = (F64)svals[0].f32;
            }break;
          }
        }
      }break;
      
      case RDI_EvalOp_Pick:
      {
        if(stack_count > imm)
        {
          nval = stack[stack_count - imm - 1];
        }
        else
        {
          result.code = E_ResultCode_BadOp;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_Pop:
      {
        // do nothing - the pop is handled by the control bits
      }break;
      
      case RDI_EvalOp_Insert:
      {
        if(stack_count > imm)
        {
          if(imm > 0)
          {
            E_Value tval = stack[stack_count - 1];
            E_Value *dst = stack + stack_count - 1 - imm;
            E_Value *shift = dst + 1;
            MemoryCopy(shift, dst, imm*sizeof(E_Value));
            *dst = tval;
          }
        }
        else
        {
          result.code = E_ResultCode_BadOp;
          goto done;
        }
      }break;
    }
    
    // rjf: push
    {
      U64 push_count = RDI_PUSHN_FROM_CTRLBITS(ctrlbits);
      if(push_count == 1)
      {
        if(stack_count < stack_cap)
        {
          stack[stack_count] = nval;
          stack_count += 1;
        }
        else
        {
          result.code = E_ResultCode_InsufficientStackSpace;
          goto done;
        }
      }
    }
  }
  done:;
  
  if(stack_count == 1)
  {
    result.value = stack[0];
  }
  else if(result.code == E_ResultCode_Good)
  {
    result.code = E_ResultCode_MalformedBytecode;
  }
  
  scratch_end(scratch);
  return result;
}
