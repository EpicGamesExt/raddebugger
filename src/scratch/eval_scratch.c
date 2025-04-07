// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "rdi_format/rdi_format_local.h"
#include "regs/regs.h"
#include "regs/rdi/regs_rdi.h"
#include "eval/eval_inc.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "rdi_format/rdi_format_local.c"
#include "regs/regs.c"
#include "regs/rdi/regs_rdi.c"
#include "eval/eval_inc.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  Arena *arena = arena_alloc();
  E_TypeCtx *type_ctx = push_array(arena, E_TypeCtx, 1);
  e_select_type_ctx(type_ctx);
  E_ParseCtx *parse_ctx = push_array(arena, E_ParseCtx, 1);
  e_select_parse_ctx(parse_ctx);
  E_IRCtx *ir_ctx = push_array(arena, E_IRCtx, 1);
  e_select_ir_ctx(ir_ctx);
  E_InterpretCtx *interpret_ctx = push_array(arena, E_InterpretCtx, 1);
  e_select_interpret_ctx(interpret_ctx, 0, 0);
  String8 exprs[] =
  {
    str8_lit("123"),
    str8_lit("1 + 2"),
    str8_lit("foo"),
    str8_lit("foo(bar)"),
    str8_lit("foo(bar(baz))"),
  };
  for EachElement(idx, exprs)
  {
    String8 debug_string = e_debug_log_from_expr_string(arena, exprs[idx]);
    raddbg_log("%S", debug_string);
  }
}
