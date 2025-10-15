// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_INTERPRET_H
#define EVAL_INTERPRET_H

////////////////////////////////
//~ rjf: Interpretation Context

typedef struct E_InterpretCtx E_InterpretCtx;
struct E_InterpretCtx
{
  E_Space primary_space;
  Arch reg_arch;
  E_Space reg_space;
  U64 reg_unwind_count;
  U64 *module_base;
  U64 *frame_base;
  U64 *tls_base;
};

////////////////////////////////
//~ rjf: Globals

thread_static E_InterpretCtx *e_interpret_ctx = 0;

////////////////////////////////
//~ rjf: Context Selection Functions (Selection Required For All Subsequent APIs)

internal void e_select_interpret_ctx(E_InterpretCtx *ctx, RDI_Parsed *primary_rdi, U64 ip_voff);

////////////////////////////////
//~ rjf: Space Reading Helpers

internal U64 e_space_gen(E_Space space);
internal B32 e_space_read(E_Space space, void *out, Rng1U64 range);
internal B32 e_space_write(E_Space space, void *in, Rng1U64 range);

////////////////////////////////
//~ rjf: Interpretation Functions

internal E_Interpretation e_interpret(String8 bytecode);

#endif // EVAL_INTERPRET_H
