// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_INTERPRET_H
#define EVAL_INTERPRET_H

////////////////////////////////
//~ rjf: Bytecode Interpretation Types

typedef union E_Value E_Value;
union E_Value
{
  U64 u512[8];
  U64 u256[4];
  U64 u128[2];
  U64 u64;
  S64 s64;
  F64 f64;
  F32 f32;
};

typedef struct E_Interpretation E_Interpretation;
struct E_Interpretation
{
  E_Value value;
  E_InterpretationCode code;
};

////////////////////////////////
//~ rjf: Interpretation Context

typedef B32 E_MemoryReadFunction(void *user_data, E_Space space, void *out, Rng1U64 vaddr_range);

typedef struct E_InterpretCtx E_InterpretCtx;
struct E_InterpretCtx
{
  Architecture arch;
  void *memory_read_user_data;
  E_MemoryReadFunction *memory_read;
  E_Space primary_space;
  void *reg_data;
  U64 reg_size;
  U64 *module_base;
  U64 *frame_base;
  U64 *tls_base;
};

////////////////////////////////
//~ rjf: Globals

thread_static E_InterpretCtx *e_interpret_ctx = 0;

////////////////////////////////
//~ rjf: Context Selection Functions (Selection Required For All Subsequent APIs)

internal E_InterpretCtx *e_selected_interpret_ctx(void);
internal void e_select_interpret_ctx(E_InterpretCtx *ctx);

////////////////////////////////
//~ rjf: Interpretation Functions

internal E_Interpretation e_interpret(String8 bytecode);

#endif //EVAL_INTERPRET_H
