// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_TYPES_H
#define EVAL_TYPES_H

////////////////////////////////
//~ rjf: Type Unwrapping

typedef U32 E_TypeUnwrapFlags;
enum
{
  E_TypeUnwrapFlag_Modifiers     = (1<<0),
  E_TypeUnwrapFlag_Pointers      = (1<<1),
  E_TypeUnwrapFlag_Lenses        = (1<<2),
  E_TypeUnwrapFlag_Meta          = (1<<3),
  E_TypeUnwrapFlag_Enums         = (1<<4),
  E_TypeUnwrapFlag_Aliases       = (1<<5),
  E_TypeUnwrapFlag_All           = 0xffffffff,
  E_TypeUnwrapFlag_AllDecorative = (E_TypeUnwrapFlag_All & ~E_TypeUnwrapFlag_Pointers)
};

////////////////////////////////
//~ rjf: Globals

global read_only E_Member e_member_nil = {E_MemberKind_Null};
global read_only E_Type e_type_nil = {E_TypeKind_Null};
E_TYPE_EXPAND_INFO_FUNCTION_DEF(default);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(default);
E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(identity);
E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(identity);
global read_only E_TypeExpandRule e_type_expand_rule__default =
{
  E_TYPE_EXPAND_INFO_FUNCTION_NAME(default),
  E_TYPE_EXPAND_RANGE_FUNCTION_NAME(default),
  E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_NAME(identity),
  E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_NAME(identity),
};

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
internal E_TypeKey e_type_key_cons_array(E_TypeKey element_type_key, U64 count, E_TypeFlags flags);
internal E_TypeKey e_type_key_cons_ptr(Arch arch, E_TypeKey element_type_key, U64 count, E_TypeFlags flags);
internal E_TypeKey e_type_key_cons_meta_expr(E_TypeKey type_key, String8 expr);
internal E_TypeKey e_type_key_cons_meta_display_name(E_TypeKey type_key, String8 name);
internal E_TypeKey e_type_key_cons_meta_description(E_TypeKey type_key, String8 desc);
internal E_TypeKey e_type_key_cons_base(Type *type);
internal E_TypeKey e_type_key_file(void);
internal E_TypeKey e_type_key_folder(void);

//- rjf: basic type key functions
internal B32 e_type_key_match(E_TypeKey l, E_TypeKey r);

//- rjf: type key -> info extraction
internal U64 e_hash_from_type(E_Type *type);
internal E_TypeKind e_type_kind_from_key(E_TypeKey key);
internal U64 e_type_byte_size_from_key(E_TypeKey key);
internal E_Type *e_type_from_key(Arena *arena, E_TypeKey key);
internal int e_type_qsort_compare_members_offset(E_Member *a, E_Member *b);
internal E_MemberArray e_type_data_members_from_key(Arena *arena, E_TypeKey key);
internal E_TypeExpandRule *e_expand_rule_from_type_key(E_TypeKey key);

//- rjf: type key traversal
internal E_TypeKey e_type_key_direct(E_TypeKey key);
internal E_TypeKey e_type_key_owner(E_TypeKey key);
internal E_TypeKey e_type_key_promote(E_TypeKey key);
internal E_TypeKey e_type_key_unwrap(E_TypeKey key, E_TypeUnwrapFlags flags);

//- rjf: type comparisons
internal B32 e_type_match(E_TypeKey l, E_TypeKey r);

//- rjf: type key -> string
internal void e_type_lhs_string_from_key(Arena *arena, E_TypeKey key, String8List *out, U32 prec, B32 skip_return);
internal void e_type_rhs_string_from_key(Arena *arena, E_TypeKey key, String8List *out, U32 prec);
internal String8 e_type_string_from_key(Arena *arena, E_TypeKey key);
internal E_TypeKey e_default_expansion_type_from_key(E_TypeKey key);

////////////////////////////////
//~ rjf: Cache Lookups

internal E_Type *e_type_from_key__cached(E_TypeKey key);
internal E_MemberCacheNode *e_member_cache_node_from_type_key(E_TypeKey key);
internal E_MemberArray e_type_data_members_from_key_filter__cached(E_TypeKey key, String8 filter);
internal E_MemberArray e_type_data_members_from_key__cached(E_TypeKey key);
internal E_Member e_type_member_from_key_name__cached(E_TypeKey key, String8 name);

////////////////////////////////
//~ rjf: (Built-In Type Hooks) Default Hooks

E_TYPE_EXPAND_INFO_FUNCTION_DEF(default);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(default);
E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(identity);
E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(identity);

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `only` lens

E_TYPE_EXPAND_INFO_FUNCTION_DEF(only);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(only);

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `sequence` lens

E_TYPE_EXPAND_INFO_FUNCTION_DEF(sequence);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(sequence);

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `array` lens

E_TYPE_EXPAND_INFO_FUNCTION_DEF(array);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(array);

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `slice` lens

E_TYPE_IREXT_FUNCTION_DEF(slice);
E_TYPE_ACCESS_FUNCTION_DEF(slice);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(slice);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(slice);

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `only`, `omit` lenses

E_TYPE_EXPAND_INFO_FUNCTION_DEF(only_and_omit);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(only_and_omit);

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `folder` type

E_TYPE_EXPAND_INFO_FUNCTION_DEF(folder);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(folder);
E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(folder);
E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(folder);

////////////////////////////////
//~ rjf: (Built-In Type Hooks) `file` type

E_TYPE_IREXT_FUNCTION_DEF(file);
E_TYPE_ACCESS_FUNCTION_DEF(file);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(file);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(file);

#endif // EVAL_TYPES_H
