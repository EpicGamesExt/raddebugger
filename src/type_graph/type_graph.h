// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef TYPE_GRAPH_NEW_H
#define TYPE_GRAPH_NEW_H

////////////////////////////////
//~ rjf: Generated Code

#include "generated/type_graph.meta.h"

////////////////////////////////
//~ rjf: Key Types

typedef enum TG_KeyKind
{
  TG_KeyKind_Null,
  TG_KeyKind_Basic,
  TG_KeyKind_Ext,
  TG_KeyKind_Cons,
  TG_KeyKind_Reg,
  TG_KeyKind_RegAlias,
}
TG_KeyKind;

typedef struct TG_Key TG_Key;
struct TG_Key
{
  TG_KeyKind kind;
  U32 u32[1]; // basic -> type_kind; cons -> type_kind; ext -> type_kind; reg -> arch
  U64 u64[1]; // ext -> unique id; cons -> idx; reg -> code
};

typedef struct TG_KeyNode TG_KeyNode;
struct TG_KeyNode
{
  TG_KeyNode *next;
  TG_Key v;
};

typedef struct TG_KeyList TG_KeyList;
struct TG_KeyList
{
  TG_KeyNode *first;
  TG_KeyNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Graph Types

typedef struct TG_ConsType TG_ConsType;
struct TG_ConsType
{
  TG_Kind kind;
  TG_Key direct_type_key;
  U64 u64;
};

typedef struct TG_Node TG_Node;
struct TG_Node
{
  TG_Node *key_hash_next;
  TG_Node *content_hash_next;
  TG_Key key;
  TG_ConsType cons_type;
};

typedef struct TG_Slot TG_Slot;
struct TG_Slot
{
  TG_Node *first;
  TG_Node *last;
};

typedef struct TG_Graph TG_Graph;
struct TG_Graph
{
  U64 address_size;
  U64 cons_id_gen;
  U64 content_hash_slots_count;
  TG_Slot *content_hash_slots;
  U64 key_hash_slots_count;
  TG_Slot *key_hash_slots;
};

////////////////////////////////
//~ rjf: Extracted Info Types

typedef enum TG_MemberKind
{
  TG_MemberKind_Null,
  TG_MemberKind_DataField,
  TG_MemberKind_StaticData,
  TG_MemberKind_Method,
  TG_MemberKind_StaticMethod,
  TG_MemberKind_VirtualMethod,
  TG_MemberKind_VTablePtr,
  TG_MemberKind_Base,
  TG_MemberKind_VirtualBase,
  TG_MemberKind_NestedType,
  TG_MemberKind_Padding,
  TG_MemberKind_COUNT
}
TG_MemberKind;

typedef U32 TG_Flags;
enum
{
  TG_Flag_Const    = (1<<0),
  TG_Flag_Volatile = (1<<1),
};

typedef struct TG_Member TG_Member;
struct TG_Member
{
  TG_MemberKind kind;
  TG_Key type_key;
  String8 name;
  U64 off;
  TG_KeyList inheritance_key_chain;
};

typedef struct TG_MemberNode TG_MemberNode;
struct TG_MemberNode
{
  TG_MemberNode *next;
  TG_Member v;
};

typedef struct TG_MemberList TG_MemberList;
struct TG_MemberList
{
  TG_MemberNode *first;
  TG_MemberNode *last;
  U64 count;
};

typedef struct TG_MemberArray TG_MemberArray;
struct TG_MemberArray
{
  TG_Member *v;
  U64 count;
};

typedef struct TG_EnumVal TG_EnumVal;
struct TG_EnumVal
{
  String8 name;
  U64 val;
};

typedef struct TG_EnumValArray TG_EnumValArray;
struct TG_EnumValArray
{
  TG_EnumVal *v;
  U64 count;
};

typedef struct TG_Type TG_Type;
struct TG_Type
{
  TG_Kind kind;
  TG_Flags flags;
  String8 name;
  U64 byte_size;
  U64 count;
  U32 off;
  TG_Key direct_type_key;
  TG_Key owner_type_key;
  TG_Key *param_type_keys;
  TG_Member *members;
  TG_EnumVal *enum_vals;
};

////////////////////////////////
//~ rjf: Globals

global read_only TG_Type tg_type_nil =
{
  /* kind        */           TG_Kind_Null,
  /* flags       */           0,
  /* name        */           {(U8*)"<nil>",5},
};

global read_only TG_Type tg_type_variadic =
{
  /* kind        */           TG_Kind_Variadic,
  /* flags       */           0,
  /* name        */           {(U8*)"...",3},
};

thread_static Arena *tg_build_arena = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 tg_hash_from_string(U64 seed, String8 string);
internal int tg_qsort_compare_members_offset(TG_Member *a, TG_Member *b);
internal void tg_key_list_push(Arena *arena, TG_KeyList *list, TG_Key key);
internal TG_KeyList tg_key_list_copy(Arena *arena, TG_KeyList *src);

////////////////////////////////
//~ rjf: RADDBG <-> TG Enum Conversions

internal TG_Kind tg_kind_from_raddbg_type_kind(RADDBG_TypeKind kind);
internal TG_MemberKind tg_member_kind_from_raddbg_member_kind(RADDBG_MemberKind kind);

////////////////////////////////
//~ rjf: Key Type Functions

internal TG_Key tg_key_zero(void);
internal TG_Key tg_key_basic(TG_Kind kind);
internal TG_Key tg_key_ext(TG_Kind kind, U64 id);
internal TG_Key tg_key_reg(Architecture arch, REGS_RegCode code);
internal TG_Key tg_key_reg_alias(Architecture arch, REGS_AliasCode code);
internal B32 tg_key_match(TG_Key a, TG_Key b);

////////////////////////////////
//~ rjf: Graph Construction API

internal TG_Graph *tg_graph_begin(U64 address_size, U64 slot_count);
internal TG_Key tg_cons_type_make(TG_Graph *graph, TG_Kind kind, TG_Key direct_type_key, U64 u64);

////////////////////////////////
//~ rjf: Graph Introspection API

internal TG_Type *tg_type_from_graph_raddbg_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key);
internal TG_Key tg_direct_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key);
internal TG_Key tg_unwrapped_direct_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key);
internal TG_Key tg_owner_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key);
internal TG_Key tg_ptee_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key);
internal TG_Key tg_unwrapped_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key);
internal U64 tg_byte_size_from_graph_raddbg_key(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key);
internal TG_Kind tg_kind_from_key(TG_Key key);
internal TG_Member *tg_member_copy(Arena *arena, TG_Member *src);
internal TG_MemberArray tg_members_from_graph_raddbg_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key);
internal TG_MemberArray tg_data_members_from_graph_raddbg_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key);
internal void tg_lhs_string_from_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key, String8List *out, U32 prec, B32 skip_return);
internal void tg_rhs_string_from_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key, String8List *out, U32 prec);
internal String8 tg_string_from_key(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key);

#endif // TYPE_GRAPH_NEW_H
