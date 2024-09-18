// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_META_H
#define BASE_META_H

////////////////////////////////
//~ rjf: Meta Markup Features

#define EmbedFile(name, path)
#define TweakB32(name, default)           (TWEAK_##name)
#define TweakF32(name, default, min, max) (TWEAK_##name)

////////////////////////////////
//~ rjf: Tweak Info Tables

typedef struct TweakB32Info TweakB32Info;
struct TweakB32Info
{
  String8 name;
  B32 default_value;
  B32 *value_ptr;
};

typedef struct TweakF32Info TweakF32Info;
struct TweakF32Info
{
  String8 name;
  F32 default_value;
  Rng1F32 value_range;
  F32 *value_ptr;
};

typedef struct TweakB32InfoTable TweakB32InfoTable;
struct TweakB32InfoTable
{
  TweakB32Info *v;
  U64 count;
};

typedef struct TweakF32InfoTable TweakF32InfoTable;
struct TweakF32InfoTable
{
  TweakF32Info *v;
  U64 count;
};

typedef struct EmbedInfo EmbedInfo;
struct EmbedInfo
{
  String8 name;
  String8 *data;
  U128 *hash;
};

typedef struct EmbedInfoTable EmbedInfoTable;
struct EmbedInfoTable
{
  EmbedInfo *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Type Info Types

typedef U32 TypeFlags;
enum
{
  TypeFlag_Indexified = (1<<0),
};

typedef enum TypeKind
{
  TypeKind_Null,
  
  // rjf: leaves
  TypeKind_Void, TypeKind_FirstLeaf = TypeKind_Void,
  TypeKind_U8,
  TypeKind_U16,
  TypeKind_U32,
  TypeKind_U64,
  TypeKind_S8,
  TypeKind_S16,
  TypeKind_S32,
  TypeKind_S64,
  TypeKind_B8,
  TypeKind_B16,
  TypeKind_B32,
  TypeKind_B64,
  TypeKind_F32,
  TypeKind_F64, TypeKind_LastLeaf = TypeKind_F64,
  
  // rjf: operators
  TypeKind_Ptr,
  TypeKind_Array,
  
  // rjf: user-defined-types
  TypeKind_Struct,
  TypeKind_Union,
  TypeKind_Enum,
  
  TypeKind_COUNT
}
TypeKind;

typedef U32 MemberFlags;
enum
{
  MemberFlag_DoNotSerialize = (1<<0),
};

typedef struct Type Type;
typedef struct Member Member;
struct Member
{
  String8 name;
  Type *type;
  U64 value;
  MemberFlags flags;
};

typedef struct Type Type;
struct Type
{
  TypeKind kind;
  TypeFlags flags;
  U64 size;
  Type *direct;
  String8 name;
  String8 count_delimiter_name; // gathered from surrounding members, turns *->[1] into *->[N]
  U64 count;
  Member *members;
};

////////////////////////////////
//~ rjf: Type Serialization Parameters

typedef struct TypeSerializePtrRefInfo TypeSerializePtrRefInfo;
struct TypeSerializePtrRefInfo
{
  Type *type;          // pointers to this
  void *indexify_base; // can be indexified using this
  String8 id_member;   // or ID'd via this member of the pointed-to-instance
};

typedef struct TypeSerializeParams TypeSerializeParams;
struct TypeSerializeParams
{
  TypeSerializePtrRefInfo *ptr_ref_infos;
  U64 ptr_ref_infos_count;
};

////////////////////////////////
//~ rjf: Globals

read_only global Type type_leaves[] =
{
  {TypeKind_Null, 0, 0, &type_leaves[0], str8_lit_comp("null")},
  {TypeKind_Void, 0, 0, &type_leaves[0], str8_lit_comp("void")},
  {TypeKind_U8,   0, sizeof(U8),          &type_leaves[0], str8_lit_comp("U8")},
  {TypeKind_U16,  0, sizeof(U16),         &type_leaves[0], str8_lit_comp("U16")},
  {TypeKind_U32,  0, sizeof(U32),         &type_leaves[0], str8_lit_comp("U32")},
  {TypeKind_U64,  0, sizeof(U64),         &type_leaves[0], str8_lit_comp("U64")},
  {TypeKind_S8,   0, sizeof(S8),          &type_leaves[0], str8_lit_comp("S8")},
  {TypeKind_S16,  0, sizeof(S16),         &type_leaves[0], str8_lit_comp("S16")},
  {TypeKind_S32,  0, sizeof(S32),         &type_leaves[0], str8_lit_comp("S32")},
  {TypeKind_S64,  0, sizeof(S64),         &type_leaves[0], str8_lit_comp("S64")},
  {TypeKind_B8,   0, sizeof(B8),          &type_leaves[0], str8_lit_comp("B8")},
  {TypeKind_B16,  0, sizeof(B16),         &type_leaves[0], str8_lit_comp("B16")},
  {TypeKind_B32,  0, sizeof(B32),         &type_leaves[0], str8_lit_comp("B32")},
  {TypeKind_B64,  0, sizeof(B64),         &type_leaves[0], str8_lit_comp("B64")},
  {TypeKind_F32,  0, sizeof(F32),         &type_leaves[0], str8_lit_comp("F32")},
  {TypeKind_F64,  0, sizeof(F64),         &type_leaves[0], str8_lit_comp("F64")},
};
read_only global Member member_nil = {{0}, &type_leaves[0]};

////////////////////////////////
//~ rjf: Type Info Lookups

#define type(T) &(T##__type)
internal Member *member_from_name(Type *type, String8 name);

////////////////////////////////
//~ rjf: Type Info * Instance Operations

internal String8 serialized_from_typed_data(Arena *arena, Type *type, void *ptr, TypeSerializeParams *params);
internal void *data_from_typed_serialized(Arena *arena, Type *type, String8 string, TypeSerializeParams *params);
#define serialized_from_struct(arena, T, ptr, ...)         serialized_from_typed_data((arena), type(T), (ptr), &(TypeSerializeParams){.ptr_ref_infos = 0, __VA_ARGS__})
#define struct_from_serialized(arena, T, string, ...) (T *)data_from_typed_serialized((arena), type(T), (string), &(TypeSerializeParams){.ptr_ref_infos = 0, __VA_ARGS__})

#endif // BASE_META_H
