// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_TYPES_H
#define EVAL_TYPES_H

////////////////////////////////
//~ rjf: Implicit Type Graph Key Types

typedef enum E_TypeKeyKind
{
  E_TypeKeyKind_Null,
  E_TypeKeyKind_Basic,
  E_TypeKeyKind_Ext,
  E_TypeKeyKind_Cons,
  E_TypeKeyKind_Reg,
  E_TypeKeyKind_RegAlias,
}
E_TypeKeyKind;

typedef struct E_TypeKey E_TypeKey;
struct E_TypeKey
{
  E_TypeKeyKind kind;
  U32 u32[3];
  // [0] -> E_TypeKind (Basic, Cons, Ext); Arch (Reg, RegAlias)
  // [1] -> Type Index In RDI (Ext); Code (Reg, RegAlias); Type Index In Constructed (Cons)
  // [2] -> RDI Index (Ext)
};

typedef struct E_TypeKeyNode E_TypeKeyNode;
struct E_TypeKeyNode
{
  E_TypeKeyNode *next;
  E_TypeKey v;
};

typedef struct E_TypeKeyList E_TypeKeyList;
struct E_TypeKeyList
{
  E_TypeKeyNode *first;
  E_TypeKeyNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Full Extracted Type Information Types

typedef enum E_MemberKind
{
  E_MemberKind_Null,
  E_MemberKind_DataField,
  E_MemberKind_StaticData,
  E_MemberKind_Method,
  E_MemberKind_StaticMethod,
  E_MemberKind_VirtualMethod,
  E_MemberKind_VTablePtr,
  E_MemberKind_Base,
  E_MemberKind_VirtualBase,
  E_MemberKind_NestedType,
  E_MemberKind_Padding,
  E_MemberKind_COUNT
}
E_MemberKind;

typedef U32 E_TypeFlags;
enum
{
  E_TypeFlag_Const      = (1<<0),
  E_TypeFlag_Volatile   = (1<<1),
  E_TypeFlag_External   = (1<<2),
  E_TypeFlag_IsPlainText= (1<<3),
  E_TypeFlag_IsCodeText = (1<<4),
  E_TypeFlag_IsPathText = (1<<5),
};

typedef struct E_Member E_Member;
struct E_Member
{
  E_MemberKind kind;
  E_TypeKey type_key;
  String8 name;
  String8 pretty_name;
  U64 off;
  E_TypeKeyList inheritance_key_chain;
};

typedef struct E_MemberNode E_MemberNode;
struct E_MemberNode
{
  E_MemberNode *next;
  E_Member v;
};

typedef struct E_MemberList E_MemberList;
struct E_MemberList
{
  E_MemberNode *first;
  E_MemberNode *last;
  U64 count;
};

typedef struct E_MemberArray E_MemberArray;
struct E_MemberArray
{
  E_Member *v;
  U64 count;
};

typedef struct E_EnumVal E_EnumVal;
struct E_EnumVal
{
  String8 name;
  U64 val;
};

typedef struct E_EnumValArray E_EnumValArray;
struct E_EnumValArray
{
  E_EnumVal *v;
  U64 count;
};

typedef struct E_Type E_Type;
struct E_Type
{
  E_TypeKind kind;
  E_TypeFlags flags;
  String8 name;
  U64 byte_size;
  U64 count;
  U32 off;
  E_TypeKey direct_type_key;
  E_TypeKey owner_type_key;
  E_TypeKey *param_type_keys;
  E_Member *members;
  E_EnumVal *enum_vals;
};

////////////////////////////////
//~ rjf: Evaluation Context

//- rjf: constructed type cache types

typedef struct E_ConsTypeParams E_ConsTypeParams;
struct E_ConsTypeParams
{
  Arch arch;
  E_TypeKind kind;
  E_TypeFlags flags;
  String8 name;
  E_TypeKey direct_key;
  U64 count;
  E_Member *members;
  E_EnumVal *enum_vals;
};

typedef struct E_ConsTypeNode  E_ConsTypeNode;
struct E_ConsTypeNode
{
  E_ConsTypeNode *key_next;
  E_ConsTypeNode *content_next;
  E_TypeKey key;
  E_ConsTypeParams params;
  U64 byte_size;
};

typedef struct E_ConsTypeSlot E_ConsTypeSlot;
struct E_ConsTypeSlot
{
  E_ConsTypeNode *first;
  E_ConsTypeNode *last;
};

//- rjf: member lookup cache types

typedef struct E_MemberHashNode E_MemberHashNode;
struct E_MemberHashNode
{
  E_MemberHashNode *next;
  U64 member_idx;
};

typedef struct E_MemberHashSlot E_MemberHashSlot;
struct E_MemberHashSlot
{
  E_MemberHashNode *first;
  E_MemberHashNode *last;
};

typedef struct E_MemberCacheNode E_MemberCacheNode;
struct E_MemberCacheNode
{
  E_MemberCacheNode *next;
  E_TypeKey key;
  E_MemberArray members;
  U64 member_hash_slots_count;
  E_MemberHashSlot *member_hash_slots;
};

typedef struct E_MemberCacheSlot E_MemberCacheSlot;
struct E_MemberCacheSlot
{
  E_MemberCacheNode *first;
  E_MemberCacheNode *last;
};

//- rjf: context parameterization

typedef struct E_TypeCtx E_TypeCtx;
struct E_TypeCtx
{
  // rjf: instruction pointer info
  U64 ip_vaddr;
  U64 ip_voff; // (within `primary_module`)
  
  // rjf: debug info
  E_Module *modules;
  U64 modules_count;
  E_Module *primary_module;
};

//- rjf: stateful machine part of context (not provided by user)

typedef struct E_TypeState E_TypeState;
struct E_TypeState
{
  Arena *arena;
  U64 arena_eval_start_pos;
  
  // rjf: evaluation context
  E_TypeCtx *ctx;
  
  // rjf: JIT-constructed types tables
  U64 cons_id_gen;
  U64 cons_content_slots_count;
  U64 cons_key_slots_count;
  E_ConsTypeSlot *cons_content_slots;
  E_ConsTypeSlot *cons_key_slots;
  
  // rjf: member cache table
  U64 member_cache_slots_count;
  E_MemberCacheSlot *member_cache_slots;
};

////////////////////////////////
//~ rjf: Globals

global read_only E_Member e_member_nil = {E_MemberKind_Null, zero_struct, {0}, {0}, 0, {0}};
global read_only E_Type e_type_nil = {E_TypeKind_Null};
thread_static E_TypeState *e_type_state = 0;

////////////////////////////////
//~ rjf: Type Kind Enum Functions

internal E_TypeKind e_type_kind_from_base(TypeKind kind);
internal E_TypeKind e_type_kind_from_rdi(RDI_TypeKind kind);
internal E_MemberKind e_member_kind_from_rdi(RDI_MemberKind kind);
internal RDI_EvalTypeGroup e_type_group_from_kind(E_TypeKind kind);
internal B32 e_type_kind_is_integer(E_TypeKind kind);
internal B32 e_type_kind_is_signed(E_TypeKind kind);
internal B32 e_type_kind_is_basic_or_enum(E_TypeKind kind);
internal B32 e_type_kind_is_pointer_or_ref(E_TypeKind kind);

////////////////////////////////
//~ rjf: Member Functions

internal void e_member_list_push(Arena *arena, E_MemberList *list, E_Member *member);
#define e_member_list_push_new(arena, list, ...) e_member_list_push((arena), (list), &(E_Member){.kind = E_MemberKind_DataField, __VA_ARGS__})
internal E_MemberArray e_member_array_from_list(Arena *arena, E_MemberList *list);

////////////////////////////////
//~ rjf: Context Selection Functions (Selection Required For All Subsequent APIs)

internal E_TypeCtx *e_selected_type_ctx(void);
internal void e_select_type_ctx(E_TypeCtx *ctx);

////////////////////////////////
//~ rjf: Type Operation Functions

//- rjf: basic key constructors
internal E_TypeKey e_type_key_zero(void);
internal E_TypeKey e_type_key_basic(E_TypeKind kind);
internal E_TypeKey e_type_key_ext(E_TypeKind kind, U32 type_idx, U32 rdi_idx);
internal E_TypeKey e_type_key_reg(Arch arch, REGS_RegCode code);
internal E_TypeKey e_type_key_reg_alias(Arch arch, REGS_AliasCode code);

//- rjf: constructed type construction
internal U64 e_hash_from_cons_type_params(E_ConsTypeParams *params);
internal B32 e_cons_type_params_match(E_ConsTypeParams *l, E_ConsTypeParams *r);
internal E_TypeKey e_type_key_cons_(E_ConsTypeParams *params);
#define e_type_key_cons(...) e_type_key_cons_(&(E_ConsTypeParams){.kind = E_TypeKind_Null, __VA_ARGS__})

//- rjf: constructed type construction helpers
internal E_TypeKey e_type_key_cons_array(E_TypeKey element_type_key, U64 count);
internal E_TypeKey e_type_key_cons_ptr(Arch arch, E_TypeKey element_type_key, E_TypeFlags flags);
internal E_TypeKey e_type_key_cons_space_ptr(E_TypeKey direct_type_key);
internal E_TypeKey e_type_key_cons_base(Type *type);

//- rjf: basic type key functions
internal B32 e_type_key_match(E_TypeKey l, E_TypeKey r);

//- rjf: key -> info extraction
internal U64 e_hash_from_type_key(E_TypeKey key);
internal E_TypeKind e_type_kind_from_key(E_TypeKey key);
internal U64 e_type_byte_size_from_key(E_TypeKey key);
internal E_Type *e_type_from_key(Arena *arena, E_TypeKey key);
internal E_TypeKey e_type_direct_from_key(E_TypeKey key);
internal E_TypeKey e_type_owner_from_key(E_TypeKey key);
internal E_TypeKey e_type_ptee_from_key(E_TypeKey key);
internal E_TypeKey e_type_unwrap_enum(E_TypeKey key);
internal E_TypeKey e_type_unwrap(E_TypeKey key);
internal E_TypeKey e_type_promote(E_TypeKey key);
internal B32 e_type_match(E_TypeKey l, E_TypeKey r);
internal E_Member *e_type_member_copy(Arena *arena, E_Member *src);
internal int e_type_qsort_compare_members_offset(E_Member *a, E_Member *b);
internal E_MemberArray e_type_data_members_from_key(Arena *arena, E_TypeKey key);
internal E_Member *e_type_member_from_array_name(E_MemberArray *members, String8 name);
internal void e_type_lhs_string_from_key(Arena *arena, E_TypeKey key, String8List *out, U32 prec, B32 skip_return);
internal void e_type_rhs_string_from_key(Arena *arena, E_TypeKey key, String8List *out, U32 prec);
internal String8 e_type_string_from_key(Arena *arena, E_TypeKey key);

//- rjf: type key data structures
internal void e_type_key_list_push(Arena *arena, E_TypeKeyList *list, E_TypeKey key);
internal E_TypeKeyList e_type_key_list_copy(Arena *arena, E_TypeKeyList *src);

////////////////////////////////
//~ rjf: Cache Lookups

internal E_MemberCacheNode *e_member_cache_node_from_type_key(E_TypeKey key);
internal E_MemberArray e_type_data_members_from_key__cached(E_TypeKey key);
internal E_Member e_type_member_from_key_name__cached(E_TypeKey key, String8 name);

#endif // EVAL_TYPES_H
