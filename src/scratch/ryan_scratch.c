// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "mdesk/mdesk.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "mdesk/mdesk.c"

typedef enum MG_StrExprOpKind
{
  MG_StrExprOpKind_Null,
  MG_StrExprOpKind_Prefix,
  MG_StrExprOpKind_Postfix,
  MG_StrExprOpKind_Binary,
  MG_StrExprOpKind_COUNT
}
MG_StrExprOpKind;

typedef enum MG_StrExprOp
{
  MG_StrExprOp_Null,
  
#define MG_StrExprOp_FirstString MG_StrExprOp_Dot
  MG_StrExprOp_Dot,
  MG_StrExprOp_ExpandIfTrue,
  MG_StrExprOp_Concat,
  MG_StrExprOp_BumpToColumn,
#define MG_StrExprOp_LastString MG_StrExprOp_BumpToColumn
  
#define MG_StrExprOp_FirstNumeric MG_StrExprOp_Add
  MG_StrExprOp_Add,
  MG_StrExprOp_Subtract,
  MG_StrExprOp_Multiply,
  MG_StrExprOp_Divide,
  MG_StrExprOp_Modulo,
  MG_StrExprOp_LeftShift,
  MG_StrExprOp_RightShift,
  MG_StrExprOp_BitwiseAnd,
  MG_StrExprOp_BitwiseOr,
  MG_StrExprOp_BitwiseXor,
  MG_StrExprOp_BitwiseNegate,
  MG_StrExprOp_BooleanAnd,
  MG_StrExprOp_BooleanOr,
  MG_StrExprOp_BooleanNot,
  MG_StrExprOp_Equals,
  MG_StrExprOp_DoesNotEqual,
#define MG_StrExprOp_LastNumeric MG_StrExprOp_DoesNotEqual
  
  MG_StrExprOp_COUNT,
}
MG_StrExprOp;

read_only global String8 mg_str_expr_op_symbol_string_table[MG_StrExprOp_COUNT] =
{
  str8_lit_comp(""),
  str8_lit_comp("."),  // MG_StrExprOp_Dot
  str8_lit_comp("->"), // MG_StrExprOp_ExpandIfTrue
  str8_lit_comp(".."), // MG_StrExprOp_Concat
  str8_lit_comp("=>"), // MG_StrExprOp_BumpToColumn
  str8_lit_comp("+"),  // MG_StrExprOp_Add
  str8_lit_comp("-"),  // MG_StrExprOp_Subtract
  str8_lit_comp("*"),  // MG_StrExprOp_Multiply
  str8_lit_comp("/"),  // MG_StrExprOp_Divide
  str8_lit_comp("%"),  // MG_StrExprOp_Modulo
  str8_lit_comp("<<"), // MG_StrExprOp_LeftShift
  str8_lit_comp(">>"), // MG_StrExprOp_RightShift
  str8_lit_comp("&"),  // MG_StrExprOp_BitwiseAnd
  str8_lit_comp("|"),  // MG_StrExprOp_BitwiseOr
  str8_lit_comp("^"),  // MG_StrExprOp_BitwiseXor
  str8_lit_comp("~"),  // MG_StrExprOp_BitwiseNegate
  str8_lit_comp("&&"), // MG_StrExprOp_BooleanAnd
  str8_lit_comp("||"), // MG_StrExprOp_BooleanOr
  str8_lit_comp("!"),  // MG_StrExprOp_BooleanNot
  str8_lit_comp("=="), // MG_StrExprOp_Equals
  str8_lit_comp("!="), // MG_StrExprOp_DoesNotEqual
};

read_only global S8 mg_str_expr_op_precedence_table[MG_StrExprOp_COUNT] =
{
  0,
  20, // MG_StrExprOp_Dot
  1,  // MG_StrExprOp_ExpandIfTrue
  2,  // MG_StrExprOp_Concat
  12, // MG_StrExprOp_BumpToColumn
  5,  // MG_StrExprOp_Add
  5,  // MG_StrExprOp_Subtract
  6,  // MG_StrExprOp_Multiply
  6,  // MG_StrExprOp_Divide
  6,  // MG_StrExprOp_Modulo
  7,  // MG_StrExprOp_LeftShift
  7,  // MG_StrExprOp_RightShift
  8,  // MG_StrExprOp_BitwiseAnd
  10, // MG_StrExprOp_BitwiseOr
  9,  // MG_StrExprOp_BitwiseXor
  11, // MG_StrExprOp_BitwiseNegate
  3,  // MG_StrExprOp_BooleanAnd
  3,  // MG_StrExprOp_BooleanOr
  11, // MG_StrExprOp_BooleanNot
  4,  // MG_StrExprOp_Equals
  4,  // MG_StrExprOp_DoesNotEqual
};

read_only global MG_StrExprOpKind mg_str_expr_op_kind_table[MG_StrExprOp_COUNT] =
{
  MG_StrExprOpKind_Null,
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Dot
  MG_StrExprOpKind_Binary, // MG_StrExprOp_ExpandIfTrue
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Concat
  MG_StrExprOpKind_Prefix, // MG_StrExprOp_BumpToColumn
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Add
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Subtract
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Multiply
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Divide
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Modulo
  MG_StrExprOpKind_Binary, // MG_StrExprOp_LeftShift
  MG_StrExprOpKind_Binary, // MG_StrExprOp_RightShift
  MG_StrExprOpKind_Binary, // MG_StrExprOp_BitwiseAnd
  MG_StrExprOpKind_Binary, // MG_StrExprOp_BitwiseOr
  MG_StrExprOpKind_Binary, // MG_StrExprOp_BitwiseXor
  MG_StrExprOpKind_Prefix, // MG_StrExprOp_BitwiseNegate
  MG_StrExprOpKind_Binary, // MG_StrExprOp_BooleanAnd
  MG_StrExprOpKind_Binary, // MG_StrExprOp_BooleanOr
  MG_StrExprOpKind_Prefix, // MG_StrExprOp_BooleanNot
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Equals
  MG_StrExprOpKind_Binary, // MG_StrExprOp_DoesNotEqual
};

typedef struct MG_StrExpr MG_StrExpr;
struct MG_StrExpr
{
  MG_StrExpr *parent;
  MG_StrExpr *left;
  MG_StrExpr *right;
  MG_StrExprOp op;
  MD_Node *node;
};

typedef struct MG_StrExprParseResult MG_StrExprParseResult;
struct MG_StrExprParseResult
{
  MG_StrExpr *root;
  MD_MsgList msgs;
  MD_Node *next_node;
};

global MG_StrExpr mg_str_expr_nil = {&mg_str_expr_nil, &mg_str_expr_nil, &mg_str_expr_nil};

internal MG_StrExpr *
mg_push_str_expr(Arena *arena, MG_StrExprOp op, MD_Node *node)
{
  MG_StrExpr *expr = push_array(arena, MG_StrExpr, 1);
  MemoryCopyStruct(expr, &mg_str_expr_nil);
  expr->op = op;
  expr->node = node;
  return expr;
}

internal MG_StrExprParseResult
mg_str_expr_parse_from_first_opl__min_prec(Arena *arena, MD_Node *first, MD_Node *opl, S8 min_prec)
{
  MG_StrExprParseResult parse = {&mg_str_expr_nil};
  {
    MD_Node *it = first;
    
    //- rjf: consume prefix operators
    MG_StrExpr *leafmost_op = &mg_str_expr_nil;
    for(;it < opl && !md_node_is_nil(it);)
    {
      MG_StrExprOp found_op = MG_StrExprOp_Null;
      for(MG_StrExprOp op = (MG_StrExprOp)(MG_StrExprOp_Null+1);
          op < MG_StrExprOp_COUNT;
          op = (MG_StrExprOp)(op+1))
      {
        if(mg_str_expr_op_kind_table[op] == MG_StrExprOpKind_Prefix &&
           str8_match(it->string, mg_str_expr_op_symbol_string_table[op], 0) &&
           mg_str_expr_op_precedence_table[op] >= min_prec)
        {
          found_op = op;
          break;
        }
      }
      if(found_op != MG_StrExprOp_Null)
      {
        MG_StrExpr *op_expr = mg_push_str_expr(arena, found_op, it);
        if(leafmost_op == &mg_str_expr_nil)
        {
          leafmost_op = op_expr;
        }
        op_expr->left = parse.root;
        parse.root = op_expr;
        it = it->next;
      }
      else
      {
        break;
      }
    }
    
    //- rjf: parse atom
    {
      MG_StrExpr *atom = &mg_str_expr_nil;
      if(it->flags & MD_NodeFlag_Identifier|MD_NodeFlag_Numeric|MD_NodeFlag_StringLiteral &&
         md_node_is_nil(it->first))
      {
        atom = mg_push_str_expr(arena, MG_StrExprOp_Null, it);
        it = it->next;
      }
      else if(!md_node_is_nil(it->first))
      {
        MG_StrExprParseResult subparse = mg_str_expr_parse_from_first_opl__min_prec(arena, it->first, &md_nil_node, 0);
        atom = subparse.root;
        md_msg_list_concat_in_place(&parse.msgs, &subparse.msgs);
        it = it->next;
      }
      if(leafmost_op != &mg_str_expr_nil)
      {
        leafmost_op->left = atom;
      }
      else
      {
        parse.root = atom;
      }
    }
    
    //- rjf: parse binary operator extensions at this precedence level
    for(;it < opl && !md_node_is_nil(it);)
    {
      // rjf: find binary op kind of `it`
      MG_StrExprOp found_op = MG_StrExprOp_Null;
      for(MG_StrExprOp op = (MG_StrExprOp)(MG_StrExprOp_Null+1);
          op < MG_StrExprOp_COUNT;
          op = (MG_StrExprOp)(op+1))
      {
        if(mg_str_expr_op_kind_table[op] == MG_StrExprOpKind_Binary &&
           str8_match(it->string, mg_str_expr_op_symbol_string_table[op], 0) &&
           mg_str_expr_op_precedence_table[op] >= min_prec)
        {
          found_op = op;
          break;
        }
      }
      
      // rjf: good found_op -> build binary expr
      if(found_op != MG_StrExprOp_Null)
      {
        MG_StrExpr *op_expr = mg_push_str_expr(arena, found_op, it);
        if(leafmost_op == &mg_str_expr_nil)
        {
          leafmost_op = op_expr;
        }
        op_expr->left = parse.root;
        parse.root = op_expr;
        it = it->next;
      }
      else
      {
        break;
      }
      
      // rjf: parse right hand side of binary operator
      MG_StrExprParseResult subparse = mg_str_expr_parse_from_first_opl__min_prec(arena, it, opl, mg_str_expr_op_precedence_table[found_op]+1);
      parse.root->right = subparse.root;
      md_msg_list_concat_in_place(&parse.msgs, &subparse.msgs);
      if(subparse.root == &mg_str_expr_nil)
      {
        md_msg_list_pushf(arena, &parse.msgs, it, MD_MsgKind_Error, "Missing right-hand-side of '%S'.", mg_str_expr_op_symbol_string_table[found_op]);
      }
      it = subparse.next_node;
    }
    
    // rjf: store next node for more caller-side parsing
    parse.next_node = it;
  }
  return parse;
}

internal MG_StrExprParseResult
mg_str_expr_parse_from_first_opl(Arena *arena, MD_Node *first, MD_Node *opl)
{
  MG_StrExprParseResult parse = mg_str_expr_parse_from_first_opl__min_prec(arena, first, opl, 0);
  return parse;
}

internal MG_StrExprParseResult
mg_str_expr_parse_from_root(Arena *arena, MD_Node *root)
{
  MG_StrExprParseResult parse = mg_str_expr_parse_from_first_opl__min_prec(arena, root->first, &md_nil_node, 0);
  return parse;
}

int main(int argument_count, char **arguments)
{
  static TCTX main_thread_tctx = {0};
  tctx_init_and_equip(&main_thread_tctx);
  os_init(argument_count, arguments);
  
  Arena *arena = arena_alloc();
  String8 text = str8_lit("(a.vr == \"x\" -> \"DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_DEF(\" .. a.name_lower .. \");\")");
  MD_TokenizeResult tokenize = md_tokenize_from_text(arena, text);
  MD_ParseResult base_parse = md_parse_from_text_tokens(arena, str8_lit(""), text, tokenize.tokens);
  MG_StrExprParseResult strexpr_parse = mg_str_expr_parse_from_root(arena, base_parse.root->first);
  
  return 0;
}
