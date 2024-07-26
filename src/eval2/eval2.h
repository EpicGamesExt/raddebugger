// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_H
#define EVAL_H

////////////////////////////////
//~ rjf: Token Types

typedef enum E_TokenKind
{
  E_TokenKind_Null,
  E_TokenKind_Identifier,
  E_TokenKind_Numeric,
  E_TokenKind_StringLiteral,
  E_TokenKind_CharLiteral,
  E_TokenKind_Symbol,
  E_TokenKind_COUNT
}
E_TokenKind;

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
  // [0] -> E_TypeKind (Basic, Cons, Ext); Architecture (Reg, RegAlias)
  // [1] -> Type Index In RDI (Cons, Ext); Code (Reg, RegAlias)
  // [2] -> RDI Index (Cons, Ext)
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
  E_TypeFlag_Const    = (1<<0),
  E_TypeFlag_Volatile = (1<<1),
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
//~ rjf: Expression Tree Types

typedef enum E_Mode
{
  E_Mode_Null,
  E_Mode_Value,
  E_Mode_Addr,
  E_Mode_Reg,
}
E_Mode;

typedef struct E_Expr E_Expr;
struct E_Expr
{
  E_ExprKind kind;
  E_Mode mode;
  void *location;
  E_TypeKey type_key;
  E_Expr *first;
  E_Expr *last;
  E_Expr *next;
  U32 u32;
  F32 f32;
  U64 u64;
  F64 f64;
  String8 string;
};

////////////////////////////////
//~ rjf: IR Tree Types

typedef struct E_IRNode E_IRNode;
struct E_IRNode
{
  RDI_EvalOp op;
  String8 bytecode;
  U64 u64;
  E_IRNode *first;
  E_IRNode *last;
  E_IRNode *next;
};

typedef struct E_IRTreeAndType E_IRTreeAndType;
struct E_IRTreeAndType
{
  E_IRNode *root;
  E_TypeKey type_key;
  E_Mode mode;
};

////////////////////////////////
//~ rjf: Map Types

//- rjf: string -> num

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

//- rjf: string -> expr

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
//~ rjf: Evaluation Context

typedef struct E_ConsTypeNode E_ConsTypeNode;
struct E_ConsTypeNode
{
  E_ConsTypeNode *key_next;
  E_ConsTypeNode *content_next;
  E_TypeKey key;
  E_TypeKey direct_key;
  U64 u64;
};

typedef struct E_ConsTypeSlot E_ConsTypeSlot;
struct E_ConsTypeSlot
{
  E_ConsTypeNode *first;
  E_ConsTypeNode *last;
};

typedef struct E_Ctx E_Ctx;
struct E_Ctx
{
  // rjf: architecture
  Architecture arch;
  
  // rjf: evaluation instruction pointer address (selects from `rdis`, and within them)
  U64 ip_vaddr;
  
  // rjf: debug info
  RDI_Parsed **rdis;
  Rng1U64 *rdis_vaddr_ranges;
  U64 rdis_count;
  
  // rjf: identifier resolution maps
  E_String2NumMap *regs_map;
  E_String2NumMap *reg_alias_map;
  E_String2NumMap *locals_map;
  E_String2NumMap *member_map;
  E_String2ExprMap *macro_map;
  
  // rjf: JIT-constructed types
  U64 cons_id_gen;
  U64 cons_content_slots_count;
  U64 cons_key_slots_count;
  E_ConsTypeSlot *cons_content_slots;
  E_ConsTypeSlot *cons_key_slots;
};

#endif // EVAL_H
