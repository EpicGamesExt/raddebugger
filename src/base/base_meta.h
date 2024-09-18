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
  Type *type;           // pointers to this
  void *indexify_base;  // can be indexified using this
  void *offsetify_base; // can be offsetified using this
  void *nil_ptr;        // is terminal if matching 0 or this
};

typedef struct TypeSerializeParams TypeSerializeParams;
struct TypeSerializeParams
{
  TypeSerializePtrRefInfo *ptr_ref_infos;
  U64 ptr_ref_infos_count;
};

////////////////////////////////
//~ rjf: Type Name -> Type Info

#define type(T) &(T##__type)

////////////////////////////////
//~ rjf: Type Info Table Initializer Helpers

#define member_lit_comp(S, T, m, ...) {str8_lit_comp(#m), type(T), OffsetOf(S, m), __VA_ARGS__}
#define struct_members(S) read_only global Member S##__members[] =
#define struct_type(S) read_only global Type S##__type = {TypeKind_Struct, sizeof(S), &type_nil, str8_lit_comp(#S), {0}, ArrayCount(S##__members), S##__members}
#define ptr_type(name, t, ...) read_only global Type name = {TypeKind_Ptr, sizeof(void *), (t), __VA_ARGS__}

////////////////////////////////
//~ rjf: Globals

read_only global Type type_nil   = {TypeKind_Null, 0, &type_nil};
read_only global Member member_nil = {{0}, &type_nil};

////////////////////////////////
//~ rjf: Built-In Types

//- rjf: leaves
read_only global Type void__type = {TypeKind_Void, 0,           &type_nil, str8_lit_comp("void")};
read_only global Type U8__type   = {TypeKind_U8,   sizeof(U8),  &type_nil, str8_lit_comp("U8")};
read_only global Type U16__type  = {TypeKind_U16,  sizeof(U16), &type_nil, str8_lit_comp("U16")};
read_only global Type U32__type  = {TypeKind_U32,  sizeof(U32), &type_nil, str8_lit_comp("U32")};
read_only global Type U64__type  = {TypeKind_U64,  sizeof(U64), &type_nil, str8_lit_comp("U64")};
read_only global Type S8__type   = {TypeKind_S8,   sizeof(S8),  &type_nil, str8_lit_comp("S8")};
read_only global Type S16__type  = {TypeKind_S16,  sizeof(S16), &type_nil, str8_lit_comp("S16")};
read_only global Type S32__type  = {TypeKind_S32,  sizeof(S32), &type_nil, str8_lit_comp("S32")};
read_only global Type S64__type  = {TypeKind_S64,  sizeof(S64), &type_nil, str8_lit_comp("S64")};
read_only global Type B8__type   = {TypeKind_B8,   sizeof(B8),  &type_nil, str8_lit_comp("B8")};
read_only global Type B16__type  = {TypeKind_B16,  sizeof(B16), &type_nil, str8_lit_comp("B16")};
read_only global Type B32__type  = {TypeKind_B32,  sizeof(B32), &type_nil, str8_lit_comp("B32")};
read_only global Type B64__type  = {TypeKind_B64,  sizeof(B64), &type_nil, str8_lit_comp("B64")};
read_only global Type F32__type  = {TypeKind_F32,  sizeof(F32), &type_nil, str8_lit_comp("F32")};
read_only global Type F64__type  = {TypeKind_F64,  sizeof(F64), &type_nil, str8_lit_comp("F64")};

//- rjf: String8
Type String8__str_ptr_type = {TypeKind_Ptr, sizeof(void *), type(U8), {0}, str8_lit_comp("size")};
Member String8__members[] =
{
  {str8_lit_comp("str"),  &String8__str_ptr_type, OffsetOf(String8, str)},
  {str8_lit_comp("size"), type(U64),              OffsetOf(String8, size)},
};
Type String8__type =
{
  TypeKind_Struct,
  sizeof(String8),
  &type_nil,
  str8_lit_comp("String8"),
  {0},
  ArrayCount(String8__members),
  String8__members,
};

//- rjf: String8Node
extern Type String8Node__type;
Type String8Node__ptr_type = {TypeKind_Ptr, sizeof(void *), &String8Node__type};
Member String8Node__members[] =
{
  {str8_lit_comp("next"),   &String8Node__ptr_type,     OffsetOf(String8Node, next)},
  {str8_lit_comp("string"), type(String8),              OffsetOf(String8Node, string)},
};
Type String8Node__type =
{
  TypeKind_Struct,
  sizeof(String8Node),
  &type_nil,
  str8_lit_comp("String8Node"),
  {0},
  ArrayCount(String8Node__members),
  String8Node__members,
};

//- rjf: String8List
Member String8List__members[] =
{
  {str8_lit_comp("first"),      &String8Node__ptr_type,     OffsetOf(String8List, first)},
  {str8_lit_comp("last"),       &String8Node__ptr_type,     OffsetOf(String8List, last), MemberFlag_DoNotSerialize},
  {str8_lit_comp("node_count"), type(U64), OffsetOf(String8List, node_count)},
  {str8_lit_comp("total_size"), type(U64), OffsetOf(String8List, total_size)},
};
Type String8List__type =
{
  TypeKind_Struct,
  sizeof(String8List),
  &type_nil,
  str8_lit_comp("String8List"),
  {0},
  ArrayCount(String8List__members),
  String8List__members,
};

////////////////////////////////
//~ rjf: Type Info Lookups

internal Member *member_from_name(Type *type, String8 name);

////////////////////////////////
//~ rjf: Type Info * Instance Operations

internal String8 serialized_from_typed_data(Arena *arena, Type *type, String8 data, TypeSerializeParams *params);
internal String8 deserialized_from_typed_data(Arena *arena, Type *type, String8 data, TypeSerializeParams *params);
internal String8 deep_copy_from_typed_data(Arena *arena, Type *type, String8 data, TypeSerializeParams *params);
#define serialized_from_struct(arena, T, ptr, ...)         serialized_from_typed_data((arena), type(T), str8_struct(ptr), &(TypeSerializeParams){.ptr_ref_infos = 0, __VA_ARGS__})
#define struct_from_serialized(arena, T, string, ...) (T *)deserialized_from_typed_data((arena), type(T), (string), &(TypeSerializeParams){.ptr_ref_infos = 0, __VA_ARGS__}).str
#define deep_copy_from_struct(arena, T, ptr, ...)     (T *)deep_copy_from_typed_data((arena), type(T), str8_struct(ptr), &(TypeSerializeParams){.ptr_ref_infos = 0, __VA_ARGS__}).str

#endif // BASE_META_H
