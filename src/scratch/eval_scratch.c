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
  char *indent_spaces = "                                                                                                                                ";
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
  };
  for EachElement(idx, exprs)
  {
    //- rjf: begin expression
    String8 expr_text = exprs[idx];
    raddbg_log("`%S`\n", expr_text);
    
    //- rjf: tokenize
    E_TokenArray tokens = e_token_array_from_text(arena, expr_text);
    raddbg_log("    tokens:\n");
    for EachIndex(idx, tokens.count)
    {
      E_Token token = tokens.v[idx];
      String8 token_string = str8_substr(expr_text, token.range);
      raddbg_log("        %S: `%S`\n", e_token_kind_strings[token.kind], token_string);
    }
    
    //- rjf: parse
    E_Parse parse = e_parse_expr_from_text_tokens(arena, expr_text, &tokens);
    {
      typedef struct Task Task;
      struct Task
      {
        Task *next;
        E_Expr *expr;
        S32 indent;
      };
      raddbg_log("    expr:\n");
      Task start_task = {0, parse.exprs.first, 2};
      Task *first_task = &start_task;
      for(Task *t = first_task; t != 0; t = t->next)
      {
        E_Expr *expr = t->expr;
        raddbg_log("%.*s%S", (int)t->indent*4, indent_spaces, e_expr_kind_strings[expr->kind]);
        switch(expr->kind)
        {
          default:{}break;
          case E_ExprKind_LeafU64:
          {
            raddbg_log(" (%I64u)", expr->value.u64);
          }break;
        }
        raddbg_log("\n");
        Task *last_task = t;
        for(E_Expr *child = expr->first; child != &e_expr_nil; child = child->next)
        {
          Task *task = push_array(arena, Task, 1);
          task->next = last_task->next;
          last_task->next = task;
          task->expr = child;
          task->indent = t->indent+1;
          last_task = task;
        }
      }
    }
    
    //- rjf: type
    E_IRTreeAndType irtree = e_irtree_and_type_from_expr(arena, parse.exprs.first);
    {
      raddbg_log("    type:\n");
      S32 indent = 2;
      for(E_TypeKey type_key = irtree.type_key;
          !e_type_key_match(e_type_key_zero(), type_key);
          type_key = e_type_direct_from_key(type_key),
          indent += 1)
      {
        E_Type *type = e_type_from_key(arena, type_key);
        raddbg_log("%.*s%S\n", (int)indent*4, indent_spaces, e_kind_basic_string_table[type->kind]);
      }
    }
    
    //- rjf: irtree
    {
      typedef struct Task Task;
      struct Task
      {
        Task *next;
        E_IRNode *irnode;
        S32 indent;
      };
      raddbg_log("    ir_tree:\n");
      Task start_task = {0, irtree.root, 2};
      Task *first_task = &start_task;
      for(Task *t = first_task; t != 0; t = t->next)
      {
        E_IRNode *irnode = t->irnode;
        raddbg_log("%.*s", (int)t->indent*4, indent_spaces);
        switch(irnode->op)
        {
          default:{}break;
#define X(name) case RDI_EvalOp_##name:{raddbg_log(#name);}break;
          RDI_EvalOp_XList
#undef X
        }
        if(irnode->value.u64 != 0)
        {
          raddbg_log(" (%I64u)", irnode->value.u64);
        }
        raddbg_log("\n");
        Task *last_task = t;
        for(E_IRNode *child = irnode->first; child != &e_irnode_nil; child = child->next)
        {
          Task *task = push_array(arena, Task, 1);
          task->next = last_task->next;
          last_task->next = task;
          task->irnode = child;
          task->indent = t->indent+1;
          last_task = task;
        }
      }
    }
    
  }
}
