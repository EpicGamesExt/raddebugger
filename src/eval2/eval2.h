// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_H
#define EVAL_H

////////////////////////////////
//~ rjf: Generated Code

#include "generated/eval2.meta.h"

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
  E_Expr *first;
  E_Expr *last;
  E_Expr *next;
  void *location;
  E_ExprKind kind;
  E_Mode mode;
  E_TypeKey type_key;
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
  E_IRNode *first;
  E_IRNode *last;
  E_IRNode *next;
  RDI_EvalOp op;
  String8 bytecode;
  U64 u64;
};

typedef struct E_IRTreeAndType E_IRTreeAndType;
struct E_IRTreeAndType
{
  E_IRNode *root;
  E_TypeKey type_key;
  E_Mode mode;
  E_MsgList msgs;
};

////////////////////////////////
//~ rjf: Bytecode Operation Types

enum
{
  E_IRExtKind_Bytecode = RDI_EvalOp_COUNT,
  E_IRExtKind_COUNT
};

typedef struct E_Op E_Op;
struct E_Op
{
  E_Op *next;
  RDI_EvalOp opcode;
  union
  {
    U64 p;
    String8 bytecode;
  };
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
//~ rjf: Evaluation Types

typedef union E_Value E_Value;
union E_Value
{
  U64 u256[4];
  U64 u128[2];
  U64 u64;
  S64 s64;
  F64 f64;
  F32 f32;
};

typedef struct E_Result E_Result;
struct E_Result
{
  E_Value value;
  E_ResultCode code;
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

typedef B32 E_MemoryReadFunction(void *user_data, void *out, Rng1U64 vaddr_range);

typedef struct E_Ctx E_Ctx;
struct E_Ctx
{
  // rjf: architecture
  Architecture arch;
  
  // rjf: instruction pointer info
  U64 ip_vaddr;
  U64 ip_voff; // (within module, which uses `rdis[0]` for debug info)
  
  // rjf: debug info
  RDI_Parsed **rdis;
  U64 rdis_count;
  
  // rjf: identifier resolution maps
  E_String2NumMap *regs_map;
  E_String2NumMap *reg_alias_map;
  E_String2NumMap *locals_map; // (within `rdis[0]`)
  E_String2NumMap *member_map; // (within `rdis[0]`)
  E_String2ExprMap *macro_map;
  
  // rjf: JIT-constructed types
  U64 cons_id_gen;
  U64 cons_content_slots_count;
  U64 cons_key_slots_count;
  E_ConsTypeSlot *cons_content_slots;
  E_ConsTypeSlot *cons_key_slots;
  
  // rjf: interpretation environment info
  void *memory_read_user_data;
  E_MemoryReadFunction *memory_read;
  void *reg_data;
  U64 reg_size;
  U64 *module_base;
  U64 *frame_base;
  U64 *tls_base;
};

////////////////////////////////
//~ rjf: Parse Results

typedef struct E_Parse E_Parse;
struct E_Parse
{
  E_Token *last_token;
  E_Expr *expr;
  E_MsgList msgs;
};

////////////////////////////////
//~ rjf: Globals

global read_only E_Expr e_expr_nil = {&e_expr_nil, &e_expr_nil, &e_expr_nil};
global read_only E_Type e_type_nil = {E_TypeKind_Null};
global read_only E_IRNode e_irnode_nil = {&e_irnode_nil, &e_irnode_nil, &e_irnode_nil};
thread_static E_Ctx *e_ctx = 0;

////////////////////////////////
//~ rjf: Basic Helper Functions

internal U64 e_hash_from_string(String8 string);

////////////////////////////////
//~ rjf: Type Kind Enum Functions

internal E_TypeKind e_type_kind_from_rdi(RDI_TypeKind kind);
internal E_MemberKind e_member_kind_from_rdi(RDI_MemberKind kind);
internal RDI_EvalTypeGroup e_type_group_from_kind(E_TypeKind kind);
internal B32 e_type_kind_is_integer(E_TypeKind kind);
internal B32 e_type_kind_is_signed(E_TypeKind kind);
internal B32 e_type_kind_is_basic_or_enum(E_TypeKind kind);

////////////////////////////////
//~ rjf: Message Functions

internal void e_msg(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, String8 text);
internal void e_msgf(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, char *fmt, ...);
internal void e_msg_list_concat_in_place(E_MsgList *dst, E_MsgList *to_push);

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
internal E_Expr *e_expr_from_string(E_String2ExprMap *map, String8 string);

////////////////////////////////
//~ rjf: Debug-Info-Driven Map Building Functions

internal E_String2NumMap *e_push_locals_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff);
internal E_String2NumMap *e_push_member_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff);

////////////////////////////////
//~ rjf: Context Selection Functions (Required For All Subsequent APIs)

internal void e_select_ctx(E_Ctx *ctx);

////////////////////////////////
//~ rjf: Tokenization Functions

#define e_token_at_it(it, arr) (((it) < (arr)->v+(arr)->count) ? (*(it)) : e_token_zero())
internal E_Token e_token_zero(void);
internal void e_token_chunk_list_push(Arena *arena, E_TokenChunkList *list, U64 chunk_size, E_Token *token);
internal E_TokenArray e_token_array_from_chunk_list(Arena *arena, E_TokenChunkList *list);
internal E_TokenArray e_token_array_from_text(Arena *arena, String8 text);
internal E_TokenArray e_token_array_make_first_opl(E_Token *first, E_Token *opl);

////////////////////////////////
//~ rjf: Expression Tree Building Functions

internal E_Expr *e_push_expr(Arena *arena, E_ExprKind kind, void *location);
internal void e_expr_push_child(E_Expr *parent, E_Expr *child);

////////////////////////////////
//~ rjf: Type Operation Functions

//- rjf: key constructors
internal E_TypeKey e_type_key_zero(void);
internal E_TypeKey e_type_key_basic(E_TypeKind kind);
internal E_TypeKey e_type_key_ext(TG_Kind kind, U32 type_idx, U32 rdi_idx);
internal E_TypeKey e_type_key_reg(Architecture arch, REGS_RegCode code);
internal E_TypeKey e_type_key_reg_alias(Architecture arch, REGS_AliasCode code);
internal E_TypeKey e_type_key_cons(E_TypeKind kind, E_TypeKey direct_key, U64 u64);

//- rjf: basic type key functions
internal B32 e_type_key_match(E_TypeKey l, E_TypeKey r);

//- rjf: key -> info extraction
internal E_TypeKind e_type_kind_from_key(E_TypeKey key);
internal U64 e_type_byte_size_from_key(E_TypeKey key);
internal E_Type *e_type_from_key(Arena *arena, E_TypeKey key);
internal E_TypeKey e_type_direct_from_key(E_TypeKey key);
internal E_TypeKey e_type_owner_from_key(E_TypeKey key);
internal E_TypeKey e_type_unwrap_enum(E_TypeKey key);
internal E_TypeKey e_type_unwrap(E_TypeKey key);
internal E_TypeKey e_type_promote(E_TypeKey key);
internal B32 e_type_match(E_TypeKey l, E_TypeKey r);

////////////////////////////////
//~ rjf: Parsing Functions

internal E_TypeKey e_leaf_type_from_name(String8 name);
internal E_Parse e_parse_type_from_text_tokens(Arena *arena, String8 text, E_TokenArray *tokens);
internal E_Parse e_parse_expr_from_text_tokens__prec(Arena *arena, String8 text, E_TokenArray *tokens, S64 max_precedence);
internal E_Parse e_parse_expr_from_text_tokens(Arena *arena, String8 text, E_TokenArray *tokens);

////////////////////////////////
//~ rjf: IR-ization Functions

internal void e_oplist_push(Arena *arena, E_OpList *list, RDI_EvalOp opcode, U64 p);
internal E_IRTreeAndType e_irtree_and_type_from_expr(Arena *arena, EVAL_Expr *expr);
internal E_OpList e_oplist_from_irtree(Arena *arena, E_IRNode *root);
internal void e_append_oplist_from_irtree(Arena *arena, E_IRNode *root, E_OpList *out);
internal String8 e_bytecode_from_oplist(Arena *arena, E_OpList *oplist);

////////////////////////////////
//~ rjf: Interpretation Functions

internal E_Result e_interpret(String8 bytecode);

#endif // EVAL_H
