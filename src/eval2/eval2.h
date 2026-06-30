// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL2_H
#define EVAL2_H

////////////////////////////////
//~ rjf: Expression Parse Info Table Types

typedef enum E2_ExprParseKind
{
  E2_ExprParseKind_Null,
  E2_ExprParseKind_Prefix,
  E2_ExprParseKind_Postfix,
}
E2_ExprParseKind;

////////////////////////////////
//~ rjf: Generated Code

#include "eval2/generated/eval2.meta.h"

////////////////////////////////
//~ rjf: Extended Opcodes

enum
{
  E2_EvalOp_SetCtxID = RDI_EvalOp_COUNT,
  E2_EvalOp_LeafBytecode,
};

////////////////////////////////
//~ rjf: Parse Statuses

typedef enum E2_ParseStatus
{
  //- rjf: terminals
  E2_ParseStatus_Good,
  E2_ParseStatus_Error,
  E2_ParseStatus_FirstTerminal = E2_ParseStatus_Good,
  E2_ParseStatus_LastTerminal = E2_ParseStatus_Error,
  
  //- rjf: caller-provided info
  E2_ParseStatus_CheckIdentifierIsType,
  E2_ParseStatus_FirstCallerRequest = E2_ParseStatus_CheckIdentifierIsType,
  E2_ParseStatus_LastCallerRequest = E2_ParseStatus_CheckIdentifierIsType,
}
E2_ParseStatus;

#define e2_parse_status_is_terminal(s) (E2_ParseStatus_FirstTerminal <= (s) && (s) <= E2_ParseStatus_LastTerminal)
#define e2_parse_status_is_caller_request(s) (E2_ParseStatus_FirstCallerRequest <= (s) && (s) <= E2_ParseStatus_LastCallerRequest)

////////////////////////////////
//~ rjf: Compiler Statuses

typedef enum E2_CompileStatus
{
  //- rjf: terminals
  E2_CompileStatus_Good,
  E2_CompileStatus_Error,
  E2_CompileStatus_FirstTerminal = E2_CompileStatus_Good,
  E2_CompileStatus_LastTerminal = E2_CompileStatus_Error,
  
  //- rjf: caller-provided info
  E2_CompileStatus_MissedIdentifierResolution,
  E2_CompileStatus_NewIdentifierDefinition,
  E2_CompileStatus_NewCtxID,
  E2_CompileStatus_MemberAccess,
  E2_CompileStatus_IndexAccess,
  E2_CompileStatus_Call,
  E2_CompileStatus_CompileTimeEval,
  E2_CompileStatus_FirstCallerRequest = E2_CompileStatus_MissedIdentifierResolution,
  E2_CompileStatus_LastCallerRequest = E2_CompileStatus_CompileTimeEval,
}
E2_CompileStatus;

#define e2_compile_status_is_terminal(s) (E2_CompileStatus_FirstTerminal <= (s) && (s) <= E2_CompileStatus_LastTerminal)
#define e2_compile_status_is_caller_request(s) (E2_CompileStatus_FirstCallerRequest <= (s) && (s) <= E2_CompileStatus_LastCallerRequest)

////////////////////////////////
//~ rjf: Interpretation Statuses

typedef enum E2_InterpStatus
{
  //- rjf: terminals
  E2_InterpStatus_Good,
  E2_InterpStatus_BadRegCode,
  E2_InterpStatus_MissingCtxFlag,
  E2_InterpStatus_DivideByZero,
  E2_InterpStatus_InsufficientStackSpace,
  E2_InterpStatus_BadOpTypes,
  E2_InterpStatus_UnsupportedOp,
  E2_InterpStatus_BadOffset,
  E2_InterpStatus_Error,
  E2_InterpStatus_FirstTerminal = E2_InterpStatus_Good,
  E2_InterpStatus_LastTerminal = E2_InterpStatus_Error,
  
  //- rjf: caller-provided info
  E2_InterpStatus_MissedSpaceRead,
  E2_InterpStatus_NewCtxID,
  E2_InterpStatus_FirstCallerRequest = E2_InterpStatus_MissedSpaceRead,
  E2_InterpStatus_LastCallerRequest = E2_InterpStatus_NewCtxID,
}
E2_InterpStatus;

#define e2_interp_status_is_terminal(s) (E2_InterpStatus_FirstTerminal <= (s) && (s) <= E2_InterpStatus_LastTerminal)
#define e2_interp_status_is_caller_request(s) (E2_InterpStatus_FirstCallerRequest <= (s) && (s) <= E2_InterpStatus_LastCallerRequest)

////////////////////////////////
//~ rjf: Type Keys

typedef enum E2_TypeKeyKind
{
  E2_TypeKeyKind_Null,
  E2_TypeKeyKind_Basic,
  E2_TypeKeyKind_DbgInfo,
  E2_TypeKeyKind_Cons,
  E2_TypeKeyKind_Reg,
}
E2_TypeKeyKind;

typedef struct E2_TypeKey E2_TypeKey;
struct E2_TypeKey
{
  E2_TypeKeyKind kind;
  U32 u32[3];
  // [0] -> E_TypeKind (Basic, Cons, DbgInfo); Arch (Reg)
  // [1] -> Debug Info Number (DbgInfo); Code (Reg); Type Index In Constructed (Cons)
  // [2] -> Type Index In Debug Info (DbgInfo)
};

typedef struct E2_TypeKeyNode E2_TypeKeyNode;
struct E2_TypeKeyNode
{
  E2_TypeKeyNode *next;
  E2_TypeKey v;
};

typedef struct E2_TypeKeyList E2_TypeKeyList;
struct E2_TypeKeyList
{
  E2_TypeKeyNode *first;
  E2_TypeKeyNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Constructed Type Cache Types

typedef U32 E2_TypeFlags;
enum
{
  E2_TypeFlag_Const    = (1<<0),
  E2_TypeFlag_Volatile = (1<<1),
};

typedef struct E2_ConsTypeParams E2_ConsTypeParams;
struct E2_ConsTypeParams
{
  Arch arch;
  E2_TypeKind kind;
  E2_TypeFlags flags;
  String8 name;
  E2_TypeKey direct;
  U64 count;
  U64 depth;
  E2_TypeKey *param_types;
  struct E2_Expr **args;
};

typedef struct E2_ConsTypeNode E2_ConsTypeNode;
struct E2_ConsTypeNode
{
  E2_ConsTypeNode *id_next;
  E2_ConsTypeNode *content_next;
  U64 id;
  E2_ConsTypeParams params;
  U64 byte_size;
};

typedef struct E2_ConsTypeSlot E2_ConsTypeSlot;
struct E2_ConsTypeSlot
{
  E2_ConsTypeNode *first;
  E2_ConsTypeNode *last;
};

typedef struct E2_ConsTypeMap E2_ConsTypeMap;
struct E2_ConsTypeMap
{
  Arena *arena;
  U64 id_gen;
  U64 table_start_arena_pos;
  U64 slots_count;
  E2_ConsTypeSlot *content_slots;
  E2_ConsTypeSlot *id_slots;
};

////////////////////////////////
//~ rjf: Evaluation Values

typedef enum E2_Mode
{
  E2_Mode_Type,     // no value content - just type content
  E2_Mode_Address,  // value is an address into a space
  E2_Mode_Value,    // value is just a value, does not refer elsewhere
  E2_Mode_COUNT
}
E2_Mode;

typedef struct E2_Val E2_Val;
struct E2_Val
{
  union
  {
    U512 u512;
    U256 u256;
    U128 u128;
    U64 u64;
    U32 u32;
    U16 u16;
    U8 u8;
    S64 s64;
    S32 s32;
    S16 s16;
    S8 s8;
    F64 f64;
    F32 f32;
  };
  String8 string;
};

////////////////////////////////
//~ rjf: Evaluation "Asset" Parameters (Debug Info / Modules)

typedef U64 E2_SpaceID;

typedef struct E2_DbgInfo E2_DbgInfo;
struct E2_DbgInfo
{
  DI_Key dbgi_key;
  RDI_Parsed *rdi;
};

typedef struct E2_Assets E2_Assets;
struct E2_Assets
{
  U32 dbg_infos_count;
  E2_DbgInfo *dbg_infos;
};

////////////////////////////////
//~ rjf: Evaluation Contexts

typedef U64 E2_CtxID;

typedef U32 E2_CtxFlags;
enum
{
  E2_CtxFlag_HasFrameBase  = (1<<0),
  E2_CtxFlag_HasCFA        = (1<<1),
  E2_CtxFlag_HasModuleBase = (1<<2),
  E2_CtxFlag_HasTLSBase    = (1<<3),
};

typedef struct E2_Ctx E2_Ctx;
struct E2_Ctx
{
  // rjf: base info
  E2_CtxID id;
  E2_SpaceID space_id;
  E2_SpaceID reg_space_id;
  Arch arch;
  U64 addr;
  
  // rjf: optional extensions
  E2_CtxFlags flags;
  U64 frame_base_addr;
  U64 cfa_addr;
  U64 module_base_addr;
  U64 tls_base_addr;
};

////////////////////////////////
//~ rjf: Tokens

typedef enum E2_TokenKind
{
  E2_TokenKind_Null,
  E2_TokenKind_Whitespace,
  E2_TokenKind_Comment,
  E2_TokenKind_Identifier,
  E2_TokenKind_Numeric,
  E2_TokenKind_Symbol,
  E2_TokenKind_CharLiteral,
  E2_TokenKind_StringLiteral,
  E2_TokenKind_COUNT
}
E2_TokenKind;

typedef struct E2_Token E2_Token;
struct E2_Token
{
  E2_TokenKind kind;
  U32 unused;
  Rng1U64 range;
};

////////////////////////////////
//~ rjf: Expression Tree Building

typedef struct E2_ExprNode E2_ExprNode;
struct E2_ExprNode
{
  E2_ExprNode *next;
  struct E2_Expr *v;
};

typedef struct E2_Expr E2_Expr;
struct E2_Expr
{
  E2_ExprNode *first_child;
  E2_ExprNode *last_child;
  U64 child_count;
  E2_ExprKind kind;
  U32 macro_arg_num;
  Rng1U64 src_range;
  String8 string;
};

typedef struct E2_ExprMapNode E2_ExprMapNode;
struct E2_ExprMapNode
{
  E2_ExprMapNode *next;
  String8 name;
  E2_Expr *expr;
};

typedef struct E2_ExprMap E2_ExprMap;
struct E2_ExprMap
{
  E2_ExprMapNode **slots;
  U64 slots_count;
};

typedef struct E2_ParseTask E2_ParseTask;
struct E2_ParseTask
{
  E2_ParseTask *next;
  Rng1U64 src_range;
  E2_ExprNode *first_child;
  E2_ExprNode *last_child;
  E2_Expr *next_type_ancestor;
  U64 child_count;
  U64 child_count_target;
  B32 reverse_children;
  S64 max_precedence;
  E2_ExprKind expr_kind;
  String8 identifier;
  String8 expected_closer;
  String8 expected_splitter;
  U64 splitter_min_child_count;
  B32 splitter_is_required;
  String8List macro_arg_names;
  B32 do_type_ancestors;
  E2_Expr *type_lhs;
};

typedef struct E2_ParseState E2_ParseState;
struct E2_ParseState
{
  U64 string_off;
  E2_ParseTask start_task;
  E2_ParseTask *top_task;
  E2_ParseTask *free_task;
  U64 last_caller_request_string_off;
  U64 caller_request_count;
  String8 last_requested_member_name;
  B32 caller_info_completes_task;
};

typedef struct E2_Msg E2_Msg;
struct E2_Msg
{
  E2_Msg *next;
  Rng1U64 src_range;
  String8 string;
};

typedef struct E2_MsgList E2_MsgList;
struct E2_MsgList
{
  E2_Msg *first;
  E2_Msg *last;
  U64 count;
};

typedef struct E2_Parse E2_Parse;
struct E2_Parse
{
  E2_ParseStatus status;
  String8 identifier;
  E2_Expr *expr;
  String8 member_name;
  E2_Expr *params_expr;
  E2_MsgList msgs;
};

////////////////////////////////
//~ rjf: IR Tree Building

typedef struct E2_IRNodePtrNode E2_IRNodePtrNode;
struct E2_IRNodePtrNode
{
  E2_IRNodePtrNode *next;
  struct E2_IRNode *v;
};

typedef struct E2_IRNode E2_IRNode;
struct E2_IRNode
{
  E2_IRNodePtrNode *first_child;
  E2_IRNodePtrNode *last_child;
  U64 child_count;
  RDI_EvalOp op;
  String8 string;
  E2_TypeKey type_key;
  E2_Mode mode;
  E2_Val val;
};

typedef struct E2_IdentifierMapNode E2_IdentifierMapNode;
struct E2_IdentifierMapNode
{
  E2_IdentifierMapNode *next;
  String8 name;
  E2_IRNode *irtree;
};

typedef struct E2_IdentifierMap E2_IdentifierMap;
struct E2_IdentifierMap
{
  E2_IdentifierMapNode **slots;
  U64 slots_count;
};

typedef struct E2_CompileTask E2_CompileTask;
struct E2_CompileTask
{
  E2_CompileTask *next;
  E2_Expr *expr;
  E2_ExprNode *last_compiled_child_node;
  E2_IRNodePtrNode *first_irtree_child;
  E2_IRNodePtrNode *last_irtree_child;
  U64 irtree_child_count;
};

typedef struct E2_CompileState E2_CompileState;
struct E2_CompileState
{
  E2_CompileTask *top_task;
  E2_CompileTask *free_task;
  U64 caller_request_count;
  E2_Ctx selected_ctx;
};

typedef struct E2_Compile E2_Compile;
struct E2_Compile
{
  E2_CompileStatus status;
  String8 identifier;
  E2_IRNode *irtree;
  String8 member_name;
  E2_IRNode *params_irtree;
  E2_MsgList msgs;
};

////////////////////////////////
//~ rjf: Interpretation

typedef struct E2_SpaceMapNode E2_SpaceMapNode;
struct E2_SpaceMapNode
{
  E2_SpaceMapNode *next;
  E2_SpaceID space_id;
  MemoryMap memory_map;
};

typedef struct E2_SpaceMap E2_SpaceMap;
struct E2_SpaceMap
{
  E2_SpaceMapNode **slots;
  U64 slots_count;
};

typedef struct E2_InterpStackValNode E2_InterpStackValNode;
struct E2_InterpStackValNode
{
  E2_InterpStackValNode *next;
  E2_Val val;
};

typedef struct E2_InterpState E2_InterpState;
struct E2_InterpState
{
  U64 bytecode_off;
  E2_Ctx selected_ctx;
  E2_InterpStackValNode *top_val;
  E2_InterpStackValNode *free_val;
  U64 last_caller_request_bytecode_off;
  U64 caller_request_count;
};

typedef struct E2_Interp E2_Interp;
struct E2_Interp
{
  E2_InterpStatus status;
  E2_SpaceID space_id;
  Rng1U64 missed_read_space_addr_range;
  E2_CtxID ctx_id;
  E2_CtxFlags missing_ctx_flags;
  E2_Val val;
  E2_MsgList msgs;
};

////////////////////////////////
//~ rjf: Globals

thread_static E2_Assets *e2_assets = 0;
thread_static E2_ConsTypeMap *e2_cons_type_map = 0;
read_only global E2_ConsTypeNode e2_cons_type_node_nil = {&e2_cons_type_node_nil, &e2_cons_type_node_nil}; 
read_only global E2_DbgInfo e2_dbg_info_nil = {{0}, &rdi_parsed_nil};
read_only global E2_Expr e2_expr_nil = {0};
read_only global E2_IRNode e2_irnode_nil = {0};

////////////////////////////////
//~ rjf: Space -> Memory Map Helpers

internal void e2_space_map_push(Arena *arena, E2_SpaceMap *map, E2_SpaceID space_id, Rng1U64 addr_range, void *data);
internal U64 e2_space_map_read(E2_SpaceMap *map, E2_SpaceID space_id, Rng1U64 addr_range, void *out);

////////////////////////////////
//~ rjf: Name -> Expr Map Helpers

internal void e2_expr_map_push(Arena *arena, E2_ExprMap *map, String8 name, E2_Expr *expr);
internal E2_Expr *e2_expr_from_name(E2_ExprMap *map, String8 name);

////////////////////////////////
//~ rjf: Identifier Map Helpers

internal void e2_identifier_map_push(Arena *arena, E2_IdentifierMap *map, String8 name, E2_IRNode *irtree);
internal E2_IRNode *e2_irtree_from_identifier(E2_IdentifierMap *map, String8 name);

////////////////////////////////
//~ rjf: Messages

internal E2_Msg *e2_msg(Arena *arena, E2_MsgList *msgs, Rng1U64 src_range, String8 string);
internal E2_Msg *e2_msgf(Arena *arena, E2_MsgList *msgs, Rng1U64 src_range, char *fmt, ...);

////////////////////////////////
//~ rjf: Assets

internal void e2_select_assets(E2_Assets *assets);
internal E2_DbgInfo *e2_dbgi_from_num(U32 num);

////////////////////////////////
//~ rjf: Constructed Types

internal E2_ConsTypeMap *e2_cons_type_map_alloc(void);
internal void e2_select_cons_type_map(E2_ConsTypeMap *map);
internal B32 e2_cons_type_params_match(E2_ConsTypeParams *a, E2_ConsTypeParams *b);
internal E2_ConsTypeNode *e2_cons_type_node_from_params(E2_ConsTypeParams *params);
internal E2_ConsTypeNode *e2_cons_type_node_from_id(U64 id);
internal E2_TypeKey e2_type_key_from_cons_type_node(E2_ConsTypeNode *node);

////////////////////////////////
//~ rjf: Type Keys

//- rjf: enums
internal E2_TypeKind e2_type_kind_from_rdi(RDI_TypeKind k);
internal RDI_EvalTypeGroup e2_type_group_from_kind(E2_TypeKind k);
internal B32 e2_type_kind_is_ptr_or_ref(E2_TypeKind k);
internal B32 e2_type_kind_is_integer(E2_TypeKind k);
internal B32 e2_type_kind_is_float(E2_TypeKind k);
internal B32 e2_type_kind_is_signed(E2_TypeKind k);
internal B32 e2_type_kind_is_unsigned(E2_TypeKind k);

//- rjf: basic key constructors
internal E2_TypeKey e2_type_key_zero(void);
internal E2_TypeKey e2_type_key_basic(E2_TypeKind kind);
internal E2_TypeKey e2_type_key_dbgi(E2_TypeKind kind, U32 dbg_info_num, U32 type_idx);
internal E2_TypeKey e2_type_key_reg(Arch arch, ARCH_RegCode reg_code);

//- rjf: constructed type constructor
internal E2_TypeKey e2_type_key_cons_(E2_ConsTypeParams *params);
#define e2_type_key_cons(k, ...) e2_type_key_cons_(&(E2_ConsTypeParams){.kind = (k), __VA_ARGS__})

//- rjf: constructed type constructor helpers
#define e2_type_key_cons_array(element_type_key, count_, ...) e2_type_key_cons(E2_TypeKind_Array, .direct = (element_type_key), .count = (count_), __VA_ARGS__)
#define e2_type_key_cons_ptr(arch, ptee_type_key, ...) e2_type_key_cons(E2_TypeKind_Ptr, .direct = (ptee_type_key), __VA_ARGS__)

//- rjf: basic type key type functions
internal B32 e2_type_key_match(E2_TypeKey a, E2_TypeKey b);

//- rjf: type key -> info
internal E2_TypeKind e2_type_kind_from_key(E2_TypeKey k);
internal U64 e2_byte_size_from_type_key(E2_TypeKey k);
internal U32 e2_dbgi_num_from_type_key(E2_TypeKey k);
internal E2_DbgInfo *e2_dbgi_from_type_key(E2_TypeKey k);
internal U32 e2_dbgi_type_idx_from_key(E2_TypeKey k);
internal U64 e2_cons_type_id_from_key(E2_TypeKey k);
internal U64 e2_shift_from_type_key(E2_TypeKey k);
internal U64 e2_mask_count_from_type_key(E2_TypeKey k);
internal U64 e2_array_count_from_type_key(E2_TypeKey k);
internal Arch e2_arch_from_type_key(E2_TypeKey k);
internal String8 e2_name_from_type_key(Arena *arena, E2_TypeKey k);

//- rjf: type key -> string
internal String8 e2_string_from_type_key(Arena *arena, E2_TypeKey key);

//- rjf: type deep matches
internal B32 e2_type_deep_match(E2_TypeKey l, E2_TypeKey r);

//- rjf: type graph traversal primitives
internal E2_TypeKey e2_type_key_direct(E2_TypeKey k);
internal E2_TypeKey e2_type_key_owner(E2_TypeKey k);

//- rjf: type unwrapping (helpers for advancing past categories of direct type chains)
typedef U32 E2_TypeUnwrapFlags;
enum
{
  E2_TypeUnwrapFlag_Modifiers     = (1<<0),
  E2_TypeUnwrapFlag_Pointers      = (1<<1),
  E2_TypeUnwrapFlag_Lenses        = (1<<2),
  E2_TypeUnwrapFlag_Meta          = (1<<3),
  E2_TypeUnwrapFlag_Enums         = (1<<4),
  E2_TypeUnwrapFlag_Aliases       = (1<<5),
  E2_TypeUnwrapFlag_Bitfields     = (1<<6),
  E2_TypeUnwrapFlag_All           = 0xffffffff,
  E2_TypeUnwrapFlag_AllDecorative = (E2_TypeUnwrapFlag_All & ~(E2_TypeUnwrapFlag_Pointers|E2_TypeUnwrapFlag_Bitfields))
};
internal E2_TypeKey e2_type_key_unwrap(E2_TypeKey k, E2_TypeUnwrapFlags flags);
#define e2_type_key_undecorate(k) e2_type_key_unwrap((k), E2_TypeUnwrapFlag_AllDecorative)

//- rjf: type coercion
internal E2_TypeKey e2_coerced_type_key_from_operands(E2_TypeKey lhs, E2_TypeKey rhs);

////////////////////////////////
//~ rjf: Expression Constructors

internal E2_Expr *e2_expr(Arena *arena, E2_ExprKind kind);
internal void e2_expr_push_child_node(E2_Expr *parent, E2_ExprNode *node);
internal void e2_expr_push_child(Arena *arena, E2_Expr *parent, E2_Expr *expr);
internal void e2_expr_push_child_node_front(E2_Expr *parent, E2_ExprNode *node);
internal void e2_expr_push_child_front(Arena *arena, E2_Expr *parent, E2_Expr *expr);

////////////////////////////////
//~ rjf: IR Tree Constructors

internal E2_IRNode *e2_irnode(Arena *arena);
internal E2_IRNode *e2_irnode_const_u64_or_smaller(Arena *arena, U64 u);
internal E2_IRNode *e2_irnode_const_f32(Arena *arena, F32 f32);
internal E2_IRNode *e2_irnode_const_f64(Arena *arena, F64 f64);
internal E2_IRNode *e2_irnode_unary_op(Arena *arena, E2_TypeKey type_key, RDI_EvalOp op, E2_IRNode *operand);
internal E2_IRNode *e2_irnode_binary_op(Arena *arena, E2_TypeKey type_key, RDI_EvalOp op, E2_IRNode *lhs, E2_IRNode *rhs);
internal E2_IRNode *e2_irnode_resolve_to_value(Arena *arena, E2_IRNode *expr);
internal E2_IRNode *e2_irnode_truncate(Arena *arena, E2_IRNode *expr, E2_TypeKey dst_type_key);
internal E2_IRNode *e2_irnode_convert_if_possible(Arena *arena, E2_IRNode *expr, E2_TypeKey dst_type_key);
internal E2_IRNode *e2_irnode_type(Arena *arena, E2_TypeKey type_key);
internal void e2_irnode_push_child_node(E2_IRNode *parent, E2_IRNodePtrNode *node);
internal void e2_irnode_push_child(Arena *arena, E2_IRNode *parent, E2_IRNode *expr);

////////////////////////////////
//~ rjf: String -> Expression

internal E2_Token e2_token_from_string_off(E2_LangKind lang, String8 string, U64 start_off);
internal U64 e2_read_token(E2_LangKind lang, String8 string, U64 off, E2_Token *token_out);
internal B32 e2_try_token(E2_LangKind lang, String8 string, E2_TokenKind kind, String8 expected_string, U64 *off_out, E2_Token *token_out);
internal E2_Parse e2_parse_from_string(Arena *arena, E2_ParseState *state, B32 identifier_is_type, E2_LangKind lang, String8 string);

////////////////////////////////
//~ rjf: Expression -> IR Tree

internal E2_Compile e2_compile_from_expr(Arena *arena, E2_CompileState *state, E2_IRNode *resolve_result, E2_Val compile_time_eval_result, E2_Expr *expr);

////////////////////////////////
//~ rjf: IR Tree -> Bytecode

internal String8 e2_bytecode_from_irnode(Arena *arena, E2_IRNode *irnode);

////////////////////////////////
//~ rjf: Bytecode -> Result

internal E2_Interp e2_interp_from_bytecode(Arena *arena, E2_InterpState *state, E2_SpaceMap *space_map, String8 bytecode);

#endif // EVAL2_H
