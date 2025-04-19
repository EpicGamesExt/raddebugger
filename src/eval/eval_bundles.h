// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_BUNDLES_H
#define EVAL_BUNDLES_H

////////////////////////////////
//~ rjf: Bundled Evaluation Path Types

typedef struct E_Eval E_Eval;
struct E_Eval
{
  E_Value value;
  E_Space space;
  E_Expr *expr;
  E_IRTreeAndType irtree;
  String8 bytecode;
  E_InterpretationCode code;
  E_MsgList msgs;
};

////////////////////////////////
//~ rjf: Globals

read_only global E_Eval e_eval_nil = {zero_struct, zero_struct, &e_expr_nil, {&e_irnode_nil}};

////////////////////////////////
//~ rjf: Bundled Evaluation Functions

internal E_Eval e_eval_from_expr(Arena *arena, E_Expr *expr);
internal E_Eval e_eval_from_string(Arena *arena, String8 string);
internal E_Eval e_eval_from_stringf(Arena *arena, char *fmt, ...);
internal E_Eval e_autoresolved_eval_from_eval(E_Eval eval);
internal E_Eval e_dynamically_typed_eval_from_eval(E_Eval eval);
internal E_Eval e_value_eval_from_eval(E_Eval eval);
internal E_Value e_value_from_string(String8 string);
internal E_Value e_value_from_stringf(char *fmt, ...);
internal E_Value e_value_from_expr(E_Expr *expr);
internal E_Eval e_eval_wrap(Arena *arena, E_Eval eval, String8 string);
internal E_Eval e_eval_wrapf(Arena *arena, E_Eval eval, char *fmt, ...);;

internal U64 e_base_offset_from_eval(E_Eval eval);
internal Rng1U64 e_range_from_eval(E_Eval eval);

////////////////////////////////
//~ rjf: Debug Logging Functions

internal String8 e_debug_log_from_expr_string(Arena *arena, String8 string);

#endif // EVAL_BUNDLES_H
