// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_CORE_H
#define EVAL_CORE_H

////////////////////////////////
//~ rjf: Errors

typedef enum EVAL_ErrorKind
{
  EVAL_ErrorKind_Null,
  EVAL_ErrorKind_MalformedInput,
  EVAL_ErrorKind_MissingInfo,
  EVAL_ErrorKind_ResolutionFailure,
  EVAL_ErrorKind_InterpretationError,
  EVAL_ErrorKind_COUNT
}
EVAL_ErrorKind;

typedef struct EVAL_Error EVAL_Error;
struct EVAL_Error
{
  EVAL_Error *next;
  EVAL_ErrorKind kind;
  void *location;
  String8 text;
};

typedef struct EVAL_ErrorList EVAL_ErrorList;
struct EVAL_ErrorList
{
  EVAL_Error *first;
  EVAL_Error *last;
  EVAL_ErrorKind max_kind;
  U64 count;
};

////////////////////////////////
//~ rjf: Operation Types

enum
{
  EVAL_IRExtKind_Bytecode = RDI_EvalOp_COUNT,
  EVAL_IRExtKind_COUNT
};

typedef struct EVAL_Op EVAL_Op;
struct EVAL_Op
{
  EVAL_Op *next;
  RDI_EvalOp opcode;
  union
  {
    U64 p;
    String8 bytecode;
  };
};

typedef struct EVAL_OpList EVAL_OpList;
struct EVAL_OpList
{
  EVAL_Op *first_op;
  EVAL_Op *last_op;
  U32 op_count;
  U32 encoded_size;
};

////////////////////////////////
//~ rjf: Generated Code

#include "eval/generated/eval.meta.h"

////////////////////////////////
//~ rjf: Expression Tree Types

typedef enum EVAL_EvalMode
{
  EVAL_EvalMode_NULL,
  EVAL_EvalMode_Value,
  EVAL_EvalMode_Addr,
  EVAL_EvalMode_Reg
}
EVAL_EvalMode;

typedef struct EVAL_Expr EVAL_Expr;
struct EVAL_Expr
{
  EVAL_ExprKind kind;
  void *location;
  union
  {
    EVAL_Expr *children[3];
    U32 u32;
    U64 u64;
    F32 f32;
    F64 f64;
    struct
    {
      EVAL_Expr *child;
      U64 u64;
    } child_and_constant;
    String8 name;
    struct
    {
      TG_Key type_key;
      String8 bytecode;
      EVAL_EvalMode mode;
    };
  };
};

////////////////////////////////
//~ rjf: IR Tree Types

typedef struct EVAL_IRTree EVAL_IRTree;
struct EVAL_IRTree{
  RDI_EvalOp op;
  EVAL_IRTree *children[3];
  union{
    U64 p;
    String8 bytecode;
  };
};

typedef struct EVAL_IRTreeAndType EVAL_IRTreeAndType;
struct EVAL_IRTreeAndType{
  EVAL_IRTree *tree;
  TG_Key type_key;
  EVAL_EvalMode mode;
};

////////////////////////////////
//~ rjf: Map Types

//- rjf: string -> num

typedef struct EVAL_String2NumMapNode EVAL_String2NumMapNode;
struct EVAL_String2NumMapNode
{
  EVAL_String2NumMapNode *order_next;
  EVAL_String2NumMapNode *hash_next;
  String8 string;
  U64 num;
};

typedef struct EVAL_String2NumMapSlot EVAL_String2NumMapSlot;
struct EVAL_String2NumMapSlot
{
  EVAL_String2NumMapNode *first;
  EVAL_String2NumMapNode *last;
};

typedef struct EVAL_String2NumMap EVAL_String2NumMap;
struct EVAL_String2NumMap
{
  U64 slots_count;
  EVAL_String2NumMapSlot *slots;
  EVAL_String2NumMapNode *first;
  EVAL_String2NumMapNode *last;
};

//- rjf: string -> expr

typedef struct EVAL_String2ExprMapNode EVAL_String2ExprMapNode;
struct EVAL_String2ExprMapNode
{
  EVAL_String2ExprMapNode *hash_next;
  String8 string;
  EVAL_Expr *expr;
  U64 poison_count;
};

typedef struct EVAL_String2ExprMapSlot EVAL_String2ExprMapSlot;
struct EVAL_String2ExprMapSlot
{
  EVAL_String2ExprMapNode *first;
  EVAL_String2ExprMapNode *last;
};

typedef struct EVAL_String2ExprMap EVAL_String2ExprMap;
struct EVAL_String2ExprMap
{
  U64 slots_count;
  EVAL_String2ExprMapSlot *slots;
};

////////////////////////////////
//~ rjf: Globals

global read_only EVAL_Expr eval_expr_nil = {0};
global read_only EVAL_IRTree eval_irtree_nil = {0};

////////////////////////////////
//~ rjf: Basic Functions

internal U64 eval_hash_from_string(String8 string);

////////////////////////////////
//~ rjf: Error List Building Functions

internal void eval_error(Arena *arena, EVAL_ErrorList *list, EVAL_ErrorKind kind, void *location, String8 text);
internal void eval_errorf(Arena *arena, EVAL_ErrorList *list, EVAL_ErrorKind kind, void *location, char *fmt, ...);
internal void eval_error_list_concat_in_place(EVAL_ErrorList *dst, EVAL_ErrorList *to_push);

////////////////////////////////
//~ rjf: Map Functions

//- rjf: string -> num
internal EVAL_String2NumMap eval_string2num_map_make(Arena *arena, U64 slot_count);
internal void eval_string2num_map_insert(Arena *arena, EVAL_String2NumMap *map, String8 string, U64 num);
internal U64 eval_num_from_string(EVAL_String2NumMap *map, String8 string);

//- rjf: string -> expr
internal EVAL_String2ExprMap eval_string2expr_map_make(Arena *arena, U64 slot_count);
internal void eval_string2expr_map_insert(Arena *arena, EVAL_String2NumMap *map, String8 string, EVAL_Expr *expr);
internal void eval_string2expr_map_inc_poison(EVAL_String2ExprMap *map, String8 string);
internal void eval_string2expr_map_dec_poison(EVAL_String2ExprMap *map, String8 string);
internal EVAL_Expr *eval_expr_from_string(EVAL_String2ExprMap *map, String8 string);

#endif // EVAL_CORE_H
