// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_CORE_H
#define EVAL_CORE_H

////////////////////////////////
//~ rjf: Evaluation Key Type

typedef struct E_Key E_Key;
struct E_Key
{
  U64 u64;
};

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
  Rng1U64 range;
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
//~ rjf: Bytecode Operation Types

enum
{
  E_IRExtKind_Bytecode = RDI_EvalOp_COUNT,
  E_IRExtKind_SetSpace,
  E_IRExtKind_COUNT
};

typedef struct E_Op E_Op;
struct E_Op
{
  E_Op *next;
  RDI_EvalOp opcode;
  E_Value value;
  String8 string;
};

typedef struct E_OpList E_OpList;
struct E_OpList
{
  E_Op *first;
  E_Op *last;
  U64 op_count;
  U64 encoded_size;
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
  String8 chain;
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
  Rng1U64 range;
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

typedef struct E_Parse E_Parse;
struct E_Parse
{
  E_TokenArray tokens;
  E_Token *last_token;
  E_Expr *expr;
  E_Expr *last_expr;
  E_MsgList msgs;
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
  B32 auto_hook;
  E_MsgList msgs;
  E_IRTreeAndType *prev;
};

////////////////////////////////
//~ rjf: Bytecode Interpretation Types

typedef struct E_Interpretation E_Interpretation;
struct E_Interpretation
{
  E_Value value;
  E_Space space;
  E_InterpretationCode code;
};

////////////////////////////////
//~ rjf: Evaluation Artifact Bundle

typedef struct E_Eval E_Eval;
struct E_Eval
{
  E_Key key;
  E_Key parent_key;
  String8 string;
  E_Expr *expr;
  E_IRTreeAndType irtree;
  String8 bytecode;
  E_InterpretationCode code;
  E_Value value;
  E_Space space;
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
  E_TypeFlag_Const                   = (1<<0),
  E_TypeFlag_Volatile                = (1<<1),
  E_TypeFlag_IsPlainText             = (1<<2),
  E_TypeFlag_IsCodeText              = (1<<3),
  E_TypeFlag_IsPathText              = (1<<4),
  E_TypeFlag_IsNotText               = (1<<5),
  E_TypeFlag_EditableChildren        = (1<<6),
  E_TypeFlag_InheritedByMembers      = (1<<7),
  E_TypeFlag_InheritedByElements     = (1<<8),
  E_TypeFlag_ArrayLikeExpansion      = (1<<9),
  E_TypeFlag_StubSingleLineExpansion = (1<<10),
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

typedef struct E_EnumValNode E_EnumValNode;
struct E_EnumValNode
{
  E_EnumValNode *next;
  E_EnumVal v;
};

typedef struct E_EnumValList E_EnumValList;
struct E_EnumValList
{
  E_EnumValNode *first;
  E_EnumValNode *last;
  U64 count;
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

#define E_TYPE_ACCESS_FUNCTION_SIG(name) E_IRTreeAndType name(Arena *arena, E_IRTreeAndType *overridden, E_Expr *expr, E_IRTreeAndType *lhs_irtree)
#define E_TYPE_ACCESS_FUNCTION_NAME(name) e_type_access__##name
#define E_TYPE_ACCESS_FUNCTION_DEF(name) internal E_TYPE_ACCESS_FUNCTION_SIG(E_TYPE_ACCESS_FUNCTION_NAME(name))
typedef E_TYPE_ACCESS_FUNCTION_SIG(E_TypeAccessFunctionType);

#define E_TYPE_EXPAND_INFO_FUNCTION_SIG(name) E_TypeExpandInfo name(Arena *arena, E_Eval eval, String8 filter)
#define E_TYPE_EXPAND_INFO_FUNCTION_NAME(name) e_type_expand_info__##name
#define E_TYPE_EXPAND_INFO_FUNCTION_DEF(name) internal E_TYPE_EXPAND_INFO_FUNCTION_SIG(E_TYPE_EXPAND_INFO_FUNCTION_NAME(name))
typedef E_TYPE_EXPAND_INFO_FUNCTION_SIG(E_TypeExpandInfoFunctionType);

#define E_TYPE_EXPAND_RANGE_FUNCTION_SIG(name) void name(Arena *arena, void *user_data, E_Eval eval, String8 filter, Rng1U64 idx_range, E_Eval *evals_out)
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
//~ rjf: Constructed Type Types

typedef struct E_ConsTypeParams E_ConsTypeParams;
struct E_ConsTypeParams
{
  Arch arch;
  E_TypeKind kind;
  E_TypeFlags flags;
  String8 name;
  E_TypeKey direct_key;
  U64 count;
  U64 depth;
  E_Member *members;
  E_EnumVal *enum_vals;
  E_Expr **args;
  E_TypeIRExtFunctionType *irext;
  E_TypeAccessFunctionType *access;
  E_TypeExpandRule expand;
};

typedef struct E_ConsTypeNode E_ConsTypeNode;
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
//~ rjf: String -> Type Key Map Data Structure

typedef struct E_String2TypeKeyNode E_String2TypeKeyNode;
struct E_String2TypeKeyNode
{
  E_String2TypeKeyNode *next;
  String8 string;
  E_TypeKey key;
};

typedef struct E_String2TypeKeySlot E_String2TypeKeySlot;
struct E_String2TypeKeySlot
{
  E_String2TypeKeyNode *first;
  E_String2TypeKeyNode *last;
};

typedef struct E_String2TypeKeyMap E_String2TypeKeyMap;
struct E_String2TypeKeyMap
{
  U64 slots_count;
  E_String2TypeKeySlot *slots;
};

////////////////////////////////
//~ rjf: Type Pattern -> Hook Key Data Structure (Type Views)

typedef struct E_PatternPart E_PatternPart;
struct E_PatternPart
{
  E_PatternPart *next;
  String8 string;
  String8List wildcard_inst_names;
};

typedef struct E_Pattern E_Pattern;
struct E_Pattern
{
  E_PatternPart *first_part;
  E_PatternPart *last_part;
  U64 count;
};

typedef struct E_AutoHookWildcardInst E_AutoHookWildcardInst;
struct E_AutoHookWildcardInst
{
  E_AutoHookWildcardInst *next;
  String8 name;
  E_Expr *inst_expr;
};

typedef struct E_AutoHookMatch E_AutoHookMatch;
struct E_AutoHookMatch
{
  E_AutoHookMatch *next;
  E_Expr *expr;
  E_AutoHookWildcardInst *first_wildcard_inst;
  E_AutoHookWildcardInst *last_wildcard_inst;
};

typedef struct E_AutoHookMatchList E_AutoHookMatchList;
struct E_AutoHookMatchList
{
  E_AutoHookMatch *first;
  E_AutoHookMatch *last;
  U64 count;
};

typedef struct E_AutoHookNode E_AutoHookNode;
struct E_AutoHookNode
{
  E_AutoHookNode *hash_next;
  E_AutoHookNode *pattern_order_next;
  String8 type_string;
  E_Pattern type_pattern;
  String8 expr_string;
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
//~ rjf: Evaluation Context

typedef U64 E_SpaceGenFunction(void *user_data, E_Space space);
typedef B32 E_SpaceRWFunction(void *user_data, E_Space space, void *out, Rng1U64 offset_range);

//- rjf: base context

typedef struct E_BaseCtx E_BaseCtx;
struct E_BaseCtx
{
  // rjf: instruction pointer info
  U64 thread_ip_vaddr;
  U64 thread_ip_voff;
  E_Space thread_reg_space;
  Arch thread_arch;
  U64 thread_unwind_count;
  
  // rjf: modules
  E_Module *modules;
  U64 modules_count;
  E_Module *primary_module;
  
  // rjf: space hooks
  void *space_rw_user_data;
  E_SpaceGenFunction *space_gen;
  E_SpaceRWFunction *space_read;
  E_SpaceRWFunction *space_write;
};

//- rjf: ir generation context

typedef struct E_IRCtx E_IRCtx;
struct E_IRCtx
{
  E_String2NumMap *regs_map;
  E_String2NumMap *reg_alias_map;
  E_String2NumMap *locals_map; // (within `primary_module`)
  E_String2NumMap *member_map; // (within `primary_module`)
  E_String2ExprMap *macro_map;
  E_AutoHookMap *auto_hook_map;
};

////////////////////////////////
//~ rjf: Core Evaluation Cache Types

//- rjf: unpacked type cache

typedef struct E_TypeCacheNode E_TypeCacheNode;
struct E_TypeCacheNode
{
  E_TypeCacheNode *next;
  E_TypeKey key;
  E_Type *type;
};

typedef struct E_TypeCacheSlot E_TypeCacheSlot;
struct E_TypeCacheSlot
{
  E_TypeCacheNode *first;
  E_TypeCacheNode *last;
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

typedef struct E_MemberFilterNode E_MemberFilterNode;
struct E_MemberFilterNode
{
  E_MemberFilterNode *next;
  String8 filter;
  E_MemberArray members_filtered;
};

typedef struct E_MemberFilterSlot E_MemberFilterSlot;
struct E_MemberFilterSlot
{
  E_MemberFilterNode *first;
  E_MemberFilterNode *last;
};

typedef struct E_MemberCacheNode E_MemberCacheNode;
struct E_MemberCacheNode
{
  E_MemberCacheNode *next;
  E_TypeKey key;
  E_MemberArray members;
  U64 member_hash_slots_count;
  E_MemberHashSlot *member_hash_slots;
  U64 member_filter_slots_count;
  E_MemberFilterSlot *member_filter_slots;
};

typedef struct E_MemberCacheSlot E_MemberCacheSlot;
struct E_MemberCacheSlot
{
  E_MemberCacheNode *first;
  E_MemberCacheNode *last;
};

//- rjf: enum val lookup cache types

typedef struct E_EnumValHashNode E_EnumValHashNode;
struct E_EnumValHashNode
{
  E_EnumValHashNode *next;
  U64 val_idx;
};

typedef struct E_EnumValHashSlot E_EnumValHashSlot;
struct E_EnumValHashSlot
{
  E_EnumValHashNode *first;
  E_EnumValHashNode *last;
};

typedef struct E_EnumValFilterNode E_EnumValFilterNode;
struct E_EnumValFilterNode
{
  E_EnumValFilterNode *next;
  String8 filter;
  E_EnumValArray vals_filtered;
};

typedef struct E_EnumValFilterSlot E_EnumValFilterSlot;
struct E_EnumValFilterSlot
{
  E_EnumValFilterNode *first;
  E_EnumValFilterNode *last;
};

typedef struct E_EnumValCacheNode E_EnumValCacheNode;
struct E_EnumValCacheNode
{
  E_EnumValCacheNode *next;
  E_TypeKey key;
  U64 val_hash_slots_count;
  E_EnumValHashSlot *val_hash_slots;
  U64 val_filter_slots_count;
  E_EnumValFilterSlot *val_filter_slots;
};

typedef struct E_EnumValCacheSlot E_EnumValCacheSlot;
struct E_EnumValCacheSlot
{
  E_EnumValCacheNode *first;
  E_EnumValCacheNode *last;
};

//- rjf: used expression map

typedef struct E_UsedExprNode E_UsedExprNode;
struct E_UsedExprNode
{
  E_UsedExprNode *next;
  E_UsedExprNode *prev;
  E_Expr *expr;
};

typedef struct E_UsedExprSlot E_UsedExprSlot;
struct E_UsedExprSlot
{
  E_UsedExprNode *first;
  E_UsedExprNode *last;
};

typedef struct E_UsedExprMap E_UsedExprMap;
struct E_UsedExprMap
{
  U64 slots_count;
  E_UsedExprSlot *slots;
};

//- rjf: type key -> auto hook expression list cache

typedef struct E_TypeAutoHookCacheNode E_TypeAutoHookCacheNode;
struct E_TypeAutoHookCacheNode
{
  E_TypeAutoHookCacheNode *next;
  E_TypeKey key;
  E_AutoHookMatchList matches;
};

typedef struct E_TypeAutoHookCacheSlot E_TypeAutoHookCacheSlot;
struct E_TypeAutoHookCacheSlot
{
  E_TypeAutoHookCacheNode *first;
  E_TypeAutoHookCacheNode *last;
};

typedef struct E_TypeAutoHookCacheMap E_TypeAutoHookCacheMap;
struct E_TypeAutoHookCacheMap
{
  U64 slots_count;
  E_TypeAutoHookCacheSlot *slots;
};

//- rjf: string ID cache

typedef struct E_StringIDNode E_StringIDNode;
struct E_StringIDNode
{
  E_StringIDNode *hash_next;
  E_StringIDNode *id_next;
  U64 id;
  String8 string;
};

typedef struct E_StringIDSlot E_StringIDSlot;
struct E_StringIDSlot
{
  E_StringIDNode *first;
  E_StringIDNode *last;
};

typedef struct E_StringIDMap E_StringIDMap;
struct E_StringIDMap
{
  U64 id_slots_count;
  E_StringIDSlot *id_slots;
  U64 hash_slots_count;
  E_StringIDSlot *hash_slots;
};

//- rjf: cache evaluation bundles

typedef U32 E_CacheBundleFlags;
enum
{
  E_CacheBundleFlag_Parse     = (1<<0),
  E_CacheBundleFlag_IRTree    = (1<<1),
  E_CacheBundleFlag_Bytecode  = (1<<2),
  E_CacheBundleFlag_Interpret = (1<<3),
};

typedef struct E_CacheBundle E_CacheBundle;
struct E_CacheBundle
{
  E_CacheBundleFlags flags;
  E_Key key;
  E_Key parent_key;
  String8 string;
  E_Parse parse;
  E_IRTreeAndType irtree;
  String8 bytecode;
  E_Interpretation interpretation;
  U64 space_gen;
  E_MsgList msgs;
};

typedef struct E_CacheNode E_CacheNode;
struct E_CacheNode
{
  E_CacheNode *string_next;
  E_CacheNode *key_next;
  E_CacheBundle bundle;
};

typedef struct E_CacheLookup E_CacheLookup;
struct E_CacheLookup
{
  E_CacheNode *node;
  U64 hash;
};

typedef struct E_CacheSlot E_CacheSlot;
struct E_CacheSlot
{
  E_CacheNode *first;
  E_CacheNode *last;
};

//- rjf: parent stack

typedef struct E_CacheParentNode E_CacheParentNode;
struct E_CacheParentNode
{
  E_CacheParentNode *next;
  E_Key key;
};

//- rjf: main cache state type

typedef struct E_Cache E_Cache;
struct E_Cache
{
  //- rjf: root arena
  Arena *arena;
  U64 arena_eval_start_pos;
  
  //- rjf: key ID generation counter
  U64 key_id_gen;
  
  //- rjf: key -> bundle, string -> bundle tables
  U64 key_slots_count;
  E_CacheSlot *key_slots;
  U64 string_slots_count;
  E_CacheSlot *string_slots;
  
  //- rjf: parent stack
  E_CacheParentNode *top_parent_node;
  E_CacheParentNode *free_parent_node;
  
  //- rjf: unpacked context
  RDI_Procedure *thread_ip_procedure;
  
  //- rjf: [types] JIT-constructed types tables
  U64 cons_id_gen;
  U64 cons_content_slots_count;
  U64 cons_key_slots_count;
  E_ConsTypeSlot *cons_content_slots;
  E_ConsTypeSlot *cons_key_slots;
  
  //- rjf: [types] build-in constructed type keys
  E_TypeKey file_type_key;
  E_TypeKey folder_type_key;
  
  //- rjf: [types] member cache table
  U64 member_cache_slots_count;
  E_MemberCacheSlot *member_cache_slots;
  
  //- rjf: [types] enum val cache table
  U64 enum_val_cache_slots_count;
  E_EnumValCacheSlot *enum_val_cache_slots;
  
  //- rjf: [types] unpacked type cache
  U64 type_cache_slots_count;
  E_TypeCacheSlot *type_cache_slots;
  
  //- rjf: [ir] ir gen options
  B32 disallow_autohooks;
  B32 disallow_chained_fastpaths;
  E_AutoHookWildcardInst *first_wildcard_inst;
  E_AutoHookWildcardInst *last_wildcard_inst;
  
  //- rjf: [ir] ir caches
  E_UsedExprMap *used_expr_map;
  E_TypeAutoHookCacheMap *type_auto_hook_cache_map;
  
  //- rjf: [ir] string ID cache
  U64 string_id_gen;
  E_StringIDMap *string_id_map;
};

////////////////////////////////
//~ rjf: Generated Code

#include "eval/generated/eval.meta.h"

////////////////////////////////
//~ rjf: Globals

read_only global E_String2NumMap e_string2num_map_nil = {0};
read_only global E_String2ExprMap e_string2expr_map_nil = {0};
read_only global E_Expr e_expr_nil = {&e_expr_nil, &e_expr_nil, &e_expr_nil, &e_expr_nil, &e_expr_nil};
read_only global E_IRNode e_irnode_nil = {&e_irnode_nil, &e_irnode_nil, &e_irnode_nil};
read_only global E_Eval e_eval_nil = {{0}, {0}, {0}, &e_expr_nil, {&e_irnode_nil}};
read_only global E_Module e_module_nil = {&rdi_parsed_nil};
read_only global E_CacheBundle e_cache_bundle_nil = {0, {0}, {0}, {0}, {{0}, 0, &e_expr_nil, &e_expr_nil}, {&e_irnode_nil}};
thread_static E_BaseCtx *e_base_ctx = 0;
thread_static E_IRCtx *e_ir_ctx = 0;
thread_static E_Cache *e_cache = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 e_hash_from_string(U64 seed, String8 string);
#define e_value_u64(v) (E_Value){.u64 = (v)}

////////////////////////////////
//~ rjf: Expr Kind Enum Functions

internal RDI_EvalOp e_opcode_from_expr_kind(E_ExprKind kind);
internal B32        e_expr_kind_is_comparison(E_ExprKind kind);

////////////////////////////////
//~ rjf: Key Type Functions

internal B32 e_key_match(E_Key a, E_Key b);
internal E_Key e_key_zero(void);

////////////////////////////////
//~ rjf: Type Key Type Functions

internal void e_type_key_list_push(Arena *arena, E_TypeKeyList *list, E_TypeKey key);
internal void e_type_key_list_push_front(Arena *arena, E_TypeKeyList *list, E_TypeKey key);
internal E_TypeKeyList e_type_key_list_copy(Arena *arena, E_TypeKeyList *src);

////////////////////////////////
//~ rjf: Message Functions

internal void e_msg(Arena *arena, E_MsgList *msgs, E_MsgKind kind, Rng1U64 range, String8 text);
internal void e_msgf(Arena *arena, E_MsgList *msgs, E_MsgKind kind, Rng1U64 range, char *fmt, ...);
internal void e_msg_list_concat_in_place(E_MsgList *dst, E_MsgList *to_push);
internal E_MsgList e_msg_list_copy(Arena *arena, E_MsgList *src);

////////////////////////////////
//~ rjf: Space Functions

internal E_Space e_space_make(E_SpaceKind kind);

////////////////////////////////
//~ rjf: Map Functions

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
internal E_Expr *e_string2expr_map_lookup(E_String2ExprMap *map, String8 string);

//- rjf: string -> type-key
internal E_String2TypeKeyMap e_string2typekey_map_make(Arena *arena, U64 slots_count);
internal void e_string2typekey_map_insert(Arena *arena, E_String2TypeKeyMap *map, String8 string, E_TypeKey key);
internal E_TypeKey e_string2typekey_map_lookup(E_String2TypeKeyMap *map, String8 string);

//- rjf: auto hooks
internal E_AutoHookMap e_auto_hook_map_make(Arena *arena, U64 slots_count);
internal void e_auto_hook_map_insert_new_(Arena *arena, E_AutoHookMap *map, E_AutoHookParams *params);
#define e_auto_hook_map_insert_new(arena, map, ...) e_auto_hook_map_insert_new_((arena), (map), &(E_AutoHookParams){.type_key = zero_struct, __VA_ARGS__})

////////////////////////////////
//~ rjf: Debug-Info-Driven Map Building Functions

internal E_String2NumMap *e_push_locals_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff);
internal E_String2NumMap *e_push_member_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff);

////////////////////////////////
//~ rjf: Cache Creation & Selection

internal E_Cache *e_cache_alloc(void);
internal void e_cache_release(E_Cache *cache);
internal void e_select_cache(E_Cache *cache);

////////////////////////////////
//~ rjf: Evaluation Phase Markers

internal void e_select_base_ctx(E_BaseCtx *ctx);
internal void e_select_ir_ctx(E_IRCtx *ctx);

////////////////////////////////
//~ rjf: Base Cache Accessing Functions
//
// The cache uses a unique keying mechanism to refer to some evaluation at
// many layers of analysis.
//
//                                  key
//         ________________________________________________
//        /            /             |                     \
//     text ->   expression   ->  ir tree and type  ->  interpretation result
//
// Each one of these calls refers to one stage in this pipeline. The cache will
// only compute what is needed on-demand. If you ask for the full evaluation,
// which is a bundle of artifacts at all layers of analysis, then all stages
// will be computed.
//
// One wrinkle here is that the IR tree generation stage is implicitly
// parameterized by the "overridden" IR tree - this is to enable "parent
// expressions", e.g. `$.x`, or simply `x` assuming `foo` has such a member,
// in the context of some struct `foo` evaluates to the same thing as `foo.x`.
// So even though the primary API shape is based around singular keys, the
// "parent key stack" also implicitly parameterizes all of these (partly
// because it is not relevant in 99% of cases).

//- rjf: parent key stack
internal E_Key e_parent_key_push(E_Key key);
internal E_Key e_parent_key_pop(void);
#define E_ParentKey(key) DeferLoop(e_parent_key_push(key), e_parent_key_pop())

//- rjf: key construction
internal E_Key e_key_from_string(String8 string);
internal E_Key e_key_from_stringf(char *fmt, ...);
internal E_Key e_key_from_expr(E_Expr *expr);

//- rjf: base key -> bundle helper
internal E_CacheBundle *e_cache_bundle_from_key(E_Key key);

//- rjf: bundle -> pipeline stage outputs
internal E_Parse e_parse_from_bundle(E_CacheBundle *bundle);
internal E_IRTreeAndType e_irtree_from_bundle(E_CacheBundle *bundle);
internal String8 e_bytecode_from_bundle(E_CacheBundle *bundle);
internal E_Interpretation e_interpretation_from_bundle(E_CacheBundle *bundle);
#define e_parse_from_key(key) e_parse_from_bundle(e_cache_bundle_from_key(key))
#define e_irtree_from_key(key) e_irtree_from_bundle(e_cache_bundle_from_key(key))
#define e_bytecode_from_key(key) e_bytecode_from_bundle(e_cache_bundle_from_key(key))
#define e_interpretation_from_key(key) e_interpretation_from_bundle(e_cache_bundle_from_key(key))

//- rjf: key -> full expression string
internal String8 e_full_expr_string_from_key(Arena *arena, E_Key key);

//- rjf: comprehensive bundle
internal E_Eval e_eval_from_bundle(E_CacheBundle *bundle);
internal E_Eval e_value_eval_from_eval(E_Eval eval);
#define e_eval_from_key(key) e_eval_from_bundle(e_cache_bundle_from_key(key))
#define e_value_from_key(key) (e_value_eval_from_eval(e_eval_from_key(key)).value)

//- rjf: string-based helpers
#define e_parse_from_string(string) e_parse_from_bundle(e_cache_bundle_from_key(e_key_from_string(string)))
#define e_irtree_from_string(string) e_irtree_from_bundle(e_cache_bundle_from_key(e_key_from_string(string)))
#define e_bytecode_from_string(string) e_bytecode_from_bundle(e_cache_bundle_from_key(e_key_from_string(string)))
#define e_interpretation_from_string(string) e_interpretation_from_bundle(e_cache_bundle_from_key(e_key_from_string(string)))
#define e_eval_from_string(string) e_eval_from_key(e_key_from_string(string))
#define e_eval_from_stringf(...) e_eval_from_key(e_key_from_stringf(__VA_ARGS__))
#define e_value_from_string(string) e_value_eval_from_eval(e_eval_from_string(string)).value
#define e_value_from_stringf(...) e_value_eval_from_eval(e_eval_from_stringf(__VA_ARGS__)).value

//- rjf: expr-based helpers
#define e_eval_from_expr(expr) e_eval_from_key(e_key_from_expr(expr))
#define e_value_from_expr(expr) e_value_eval_from_eval(e_eval_from_expr(expr)).value

//- rjf: type key -> auto hooks
internal E_AutoHookMatchList e_push_auto_hook_matches_from_type_key(Arena *arena, E_TypeKey type_key);
internal E_AutoHookMatchList e_auto_hook_matches_from_type_key(E_TypeKey type_key);

//- rjf: string IDs
internal U64 e_id_from_string(String8 string);
internal String8 e_string_from_id(U64 id);

////////////////////////////////
//~ rjf: Key Extension Functions

internal E_Key e_key_wrap(E_Key key, String8 string);
internal E_Key e_key_wrapf(E_Key key, char *fmt, ...);

//- rjf: eval-based helpers
#define e_eval_wrap(eval, string) e_eval_from_key(e_key_wrap((eval).key, (string)))
#define e_eval_wrapf(eval, ...) e_eval_from_key(e_key_wrapf((eval).key, __VA_ARGS__))

////////////////////////////////
//~ rjf: Eval Info Extraction

internal U64 e_base_offset_from_eval(E_Eval eval);
internal U64 e_range_size_from_eval(E_Eval eval);

////////////////////////////////
//~ rjf: Debug Functions

internal String8 e_debug_log_from_expr_string(Arena *arena, String8 string);

#endif // EVAL_CORE_H
