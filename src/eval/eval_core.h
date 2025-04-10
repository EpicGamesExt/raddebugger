// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_CORE_H
#define EVAL_CORE_H

////////////////////////////////
//~ rjf: Messages

typedef enum E_MsgKind
{
  E_MsgKind_Null,
  E_MsgKind_MalformedInput,
  E_MsgKind_MissingInfo,
  E_MsgKind_ResolutionFailure,
  E_MsgKind_InterpretationError,
  E_MsgKind_COUNT
}
E_MsgKind;

typedef struct E_Msg E_Msg;
struct E_Msg
{
  E_Msg *next;
  E_MsgKind kind;
  void *location;
  String8 text;
};

typedef struct E_MsgList E_MsgList;
struct E_MsgList
{
  E_Msg *first;
  E_Msg *last;
  E_MsgKind max_kind;
  U64 count;
};

////////////////////////////////
//~ rjf: Register-Sized Value Type

typedef union E_Value E_Value;
union E_Value
{
  U512 u512;
  U256 u256;
  U128 u128;
  U64 u64;
  U32 u32;
  U16 u16;
  S64 s64;
  S32 s32;
  S32 s16;
  F64 f64;
  F32 f32;
};

////////////////////////////////
//~ rjf: Operator Info

typedef enum E_OpKind
{
  E_OpKind_Null,
  E_OpKind_UnaryPrefix,
  E_OpKind_Binary,
}
E_OpKind;

typedef struct E_OpInfo E_OpInfo;
struct E_OpInfo
{
  E_OpKind kind;
  S64 precedence;
  String8 pre;
  String8 sep;
  String8 post;
};

////////////////////////////////
//~ rjf: Evaluation Spaces
//
// NOTE(rjf): Evaluations occur within the context of a "space". Each "space"
// refers to a different offset/address-space, but it's a bit looser of a
// concept than just address space, since it can also refer to offsets into
// a register block, and it is also used to refer to spaces of unique IDs for
// key-value stores, e.g. for information in the debugger.
//
// Effectively, when considering the result of an evaluation, you use the
// value for understanding a key *into* a space, e.g. 1+2 -> 3, in a null
// space, or &foo, in the space of PID: 1234.

typedef U64 E_SpaceKind;
enum
{
  E_SpaceKind_Null,
  E_SpaceKind_File,
  E_SpaceKind_FileSystem,
  E_SpaceKind_HashStoreKey,
  E_SpaceKind_FirstUserDefined,
};

typedef struct E_Space E_Space;
struct E_Space
{
  E_SpaceKind kind;
  union
  {
    U64 u64s[3];
    struct
    {
      U64 u64_0;
      U128 u128;
    };
  };
};

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
//~ rjf: Generated Code

#include "generated/eval.meta.h"

////////////////////////////////
//~ rjf: Token Types

typedef struct E_Token E_Token;
struct E_Token
{
  E_TokenKind kind;
  Rng1U64 range;
};

typedef struct E_TokenChunkNode E_TokenChunkNode;
struct E_TokenChunkNode
{
  E_TokenChunkNode *next;
  E_Token *v;
  U64 count;
  U64 cap;
};

typedef struct E_TokenChunkList E_TokenChunkList;
struct E_TokenChunkList
{
  E_TokenChunkNode *first;
  E_TokenChunkNode *last;
  U64 node_count;
  U64 total_count;
};

typedef struct E_TokenArray E_TokenArray;
struct E_TokenArray
{
  E_Token *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Evaluation Modes

typedef enum E_Mode
{
  E_Mode_Null,
  E_Mode_Value,
  E_Mode_Offset,
}
E_Mode;

////////////////////////////////
//~ rjf: Expression Tree Types

typedef struct E_Expr E_Expr;
struct E_Expr
{
  E_Expr *first;
  E_Expr *last;
  E_Expr *next;
  E_Expr *prev;
  E_Expr *ref;
  void *location;
  E_ExprKind kind;
  E_Mode mode;
  E_Space space;
  E_TypeKey type_key;
  E_Value value;
  String8 string;
  String8 qualifier;
  String8 bytecode;
};

typedef struct E_ExprChain E_ExprChain;
struct E_ExprChain
{
  E_Expr *first;
  E_Expr *last;
};

typedef struct E_ExprNode E_ExprNode;
struct E_ExprNode
{
  E_ExprNode *next;
  E_Expr *v;
};

typedef struct E_ExprList E_ExprList;
struct E_ExprList
{
  E_ExprNode *first;
  E_ExprNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: IR Tree Types

typedef struct E_IRNode E_IRNode;
struct E_IRNode
{
  E_IRNode *first;
  E_IRNode *last;
  E_IRNode *next;
  RDI_EvalOp op;
  E_Space space;
  String8 string;
  E_Value value;
};

typedef struct E_IRTreeAndType E_IRTreeAndType;
struct E_IRTreeAndType
{
  E_IRNode *root;
  E_TypeKey type_key;
  void *user_data;
  E_Mode mode;
  E_MsgList msgs;
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
  E_TypeFlag_Const             = (1<<0),
  E_TypeFlag_Volatile          = (1<<1),
  E_TypeFlag_IsPlainText       = (1<<2),
  E_TypeFlag_IsCodeText        = (1<<3),
  E_TypeFlag_IsPathText        = (1<<4),
  E_TypeFlag_IsNotText         = (1<<5),
  E_TypeFlag_EditableChildren  = (1<<6),
  E_TypeFlag_InheritedOnAccess = (1<<7),
};

typedef struct E_Member E_Member;
struct E_Member
{
  E_MemberKind kind;
  E_TypeKey type_key;
  String8 name;
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

typedef struct E_IRExt E_IRExt;
struct E_IRExt
{
  void *user_data;
};

typedef struct E_TypeExpandInfo E_TypeExpandInfo;
struct E_TypeExpandInfo
{
  void *user_data;
  U64 expr_count;
};

#define E_TYPE_IREXT_FUNCTION_SIG(name) E_IRExt name(Arena *arena, E_Expr *expr, E_IRTreeAndType *irtree)
#define E_TYPE_IREXT_FUNCTION_NAME(name) e_type_irext__##name
#define E_TYPE_IREXT_FUNCTION_DEF(name) internal E_TYPE_IREXT_FUNCTION_SIG(E_TYPE_IREXT_FUNCTION_NAME(name))
typedef E_TYPE_IREXT_FUNCTION_SIG(E_TypeIRExtFunctionType);

#define E_TYPE_ACCESS_FUNCTION_SIG(name) E_IRTreeAndType name(Arena *arena, E_Expr *expr, E_IRTreeAndType *lhs_irtree)
#define E_TYPE_ACCESS_FUNCTION_NAME(name) e_type_access__##name
#define E_TYPE_ACCESS_FUNCTION_DEF(name) internal E_TYPE_ACCESS_FUNCTION_SIG(E_TYPE_ACCESS_FUNCTION_NAME(name))
typedef E_TYPE_ACCESS_FUNCTION_SIG(E_TypeAccessFunctionType);

#define E_TYPE_EXPAND_INFO_FUNCTION_SIG(name) E_TypeExpandInfo name(Arena *arena, E_Expr *expr, E_IRTreeAndType *irtree, String8 filter)
#define E_TYPE_EXPAND_INFO_FUNCTION_NAME(name) e_type_expand_info__##name
#define E_TYPE_EXPAND_INFO_FUNCTION_DEF(name) internal E_TYPE_EXPAND_INFO_FUNCTION_SIG(E_TYPE_EXPAND_INFO_FUNCTION_NAME(name))
typedef E_TYPE_EXPAND_INFO_FUNCTION_SIG(E_TypeExpandInfoFunctionType);

#define E_TYPE_EXPAND_RANGE_FUNCTION_SIG(name) void name(Arena *arena, void *user_data, E_Expr *expr, E_IRTreeAndType *irtree, String8 filter, Rng1U64 idx_range, E_Expr **exprs_out, String8 *exprs_strings_out)
#define E_TYPE_EXPAND_RANGE_FUNCTION_NAME(name) e_type_expand_range__##name
#define E_TYPE_EXPAND_RANGE_FUNCTION_DEF(name) internal E_TYPE_EXPAND_RANGE_FUNCTION_SIG(E_TYPE_EXPAND_RANGE_FUNCTION_NAME(name))
typedef E_TYPE_EXPAND_RANGE_FUNCTION_SIG(E_TypeExpandRangeFunctionType);

#define E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_SIG(name) U64 name(void *user_data, U64 num)
#define E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_NAME(name) e_type_expand_id_from_num__##name
#define E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(name) internal E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_SIG(E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_NAME(name))
typedef E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_SIG(E_TypeExpandIDFromNumFunctionType);

#define E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_SIG(name) U64 name(void *user_data, U64 id)
#define E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_NAME(name) e_type_expand_num_from_id__##name
#define E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(name) internal E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_SIG(E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_NAME(name))
typedef E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_SIG(E_TypeExpandNumFromIDFunctionType);

typedef struct E_TypeExpandRule E_TypeExpandRule;
struct E_TypeExpandRule
{
  E_TypeExpandInfoFunctionType *info;
  E_TypeExpandRangeFunctionType *range;
  E_TypeExpandIDFromNumFunctionType *id_from_num;
  E_TypeExpandNumFromIDFunctionType *num_from_id;
};

typedef struct E_Type E_Type;
struct E_Type
{
  E_TypeKind kind;
  E_TypeFlags flags;
  String8 name;
  U64 byte_size;
  U64 count;
  U64 depth;
  U32 off;
  Arch arch;
  E_TypeKey direct_type_key;
  E_TypeKey owner_type_key;
  E_TypeKey *param_type_keys;
  E_Member *members;
  E_EnumVal *enum_vals;
  E_Expr **args;
  E_TypeIRExtFunctionType *irext;
  E_TypeAccessFunctionType *access;
  E_TypeExpandRule expand;
};

////////////////////////////////
//~ rjf: Modules

typedef struct E_Module E_Module;
struct E_Module
{
  RDI_Parsed *rdi;
  Rng1U64 vaddr_range;
  Arch arch;
  E_Space space;
};

////////////////////////////////
//~ rjf: String -> Num

typedef struct E_String2NumMapNode E_String2NumMapNode;
struct E_String2NumMapNode
{
  E_String2NumMapNode *order_next;
  E_String2NumMapNode *hash_next;
  String8 string;
  U64 num;
};

typedef struct E_String2NumMapNodeArray E_String2NumMapNodeArray;
struct E_String2NumMapNodeArray
{
  E_String2NumMapNode **v;
  U64 count;
};

typedef struct E_String2NumMapSlot E_String2NumMapSlot;
struct E_String2NumMapSlot
{
  E_String2NumMapNode *first;
  E_String2NumMapNode *last;
};

typedef struct E_String2NumMap E_String2NumMap;
struct E_String2NumMap
{
  U64 slots_count;
  U64 node_count;
  E_String2NumMapSlot *slots;
  E_String2NumMapNode *first;
  E_String2NumMapNode *last;
};

////////////////////////////////
//~ rjf: String -> Expr

typedef struct E_String2ExprMapNode E_String2ExprMapNode;
struct E_String2ExprMapNode
{
  E_String2ExprMapNode *hash_next;
  String8 string;
  E_Expr *expr;
  U64 poison_count;
};

typedef struct E_String2ExprMapSlot E_String2ExprMapSlot;
struct E_String2ExprMapSlot
{
  E_String2ExprMapNode *first;
  E_String2ExprMapNode *last;
};

typedef struct E_String2ExprMap E_String2ExprMap;
struct E_String2ExprMap
{
  U64 slots_count;
  E_String2ExprMapSlot *slots;
};

////////////////////////////////
//~ rjf: Type Pattern -> Hook Key Data Structure (Auto View Rules)

typedef struct E_AutoHookNode E_AutoHookNode;
struct E_AutoHookNode
{
  E_AutoHookNode *hash_next;
  E_AutoHookNode *pattern_order_next;
  String8 type_string;
  String8List type_pattern_parts;
  E_ExprChain tag_exprs;
};

typedef struct E_AutoHookSlot E_AutoHookSlot;
struct E_AutoHookSlot
{
  E_AutoHookNode *first;
  E_AutoHookNode *last;
};

typedef struct E_AutoHookMap E_AutoHookMap;
struct E_AutoHookMap
{
  U64 slots_count;
  E_AutoHookSlot *slots;
  E_AutoHookNode *first_pattern;
  E_AutoHookNode *last_pattern;
};

typedef struct E_AutoHookParams E_AutoHookParams;
struct E_AutoHookParams
{
  E_TypeKey type_key;
  String8 type_pattern;
  String8 tag_expr_string;
};

////////////////////////////////
//~ rjf: Contextual & Implicit Evaluation Parameters

typedef B32 E_SpaceRWFunction(void *user_data, E_Space space, void *out, Rng1U64 offset_range);

////////////////////////////////
//~ rjf: Generated Code

#include "eval/generated/eval.meta.h"

////////////////////////////////
//~ rjf: Globals

global read_only E_Module e_module_nil = {&rdi_parsed_nil};

////////////////////////////////
//~ rjf: Basic Helper Functions

internal U64 e_hash_from_string(U64 seed, String8 string);
#define e_value_u64(v) (E_Value){.u64 = (v)}

////////////////////////////////
//~ rjf: Message Functions

internal void e_msg(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, String8 text);
internal void e_msgf(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, char *fmt, ...);
internal void e_msg_list_concat_in_place(E_MsgList *dst, E_MsgList *to_push);

////////////////////////////////
//~ rjf: Space Functions

internal E_Space e_space_make(E_SpaceKind kind);

////////////////////////////////
//~ rjf: Basic Map Functions

//- rjf: string -> num
internal E_String2NumMap e_string2num_map_make(Arena *arena, U64 slot_count);
internal void e_string2num_map_insert(Arena *arena, E_String2NumMap *map, String8 string, U64 num);
internal U64 e_num_from_string(E_String2NumMap *map, String8 string);
internal E_String2NumMapNodeArray e_string2num_map_node_array_from_map(Arena *arena, E_String2NumMap *map);
internal int e_string2num_map_node_qsort_compare__num_ascending(E_String2NumMapNode **a, E_String2NumMapNode **b);
internal void e_string2num_map_node_array_sort__in_place(E_String2NumMapNodeArray *array);

//- rjf: string -> expr
internal E_String2ExprMap e_string2expr_map_make(Arena *arena, U64 slot_count);
internal void e_string2expr_map_insert(Arena *arena, E_String2ExprMap *map, String8 string, E_Expr *expr);
internal void e_string2expr_map_inc_poison(E_String2ExprMap *map, String8 string);
internal void e_string2expr_map_dec_poison(E_String2ExprMap *map, String8 string);
internal E_Expr *e_string2expr_lookup(E_String2ExprMap *map, String8 string);

////////////////////////////////
//~ rjf: Debug-Info-Driven Map Building Functions

internal E_String2NumMap *e_push_locals_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff);
internal E_String2NumMap *e_push_member_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff);

#endif // EVAL_CORE_H
