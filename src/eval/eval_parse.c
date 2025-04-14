// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Lexing/Parsing Data Tables

global read_only String8 e_multichar_symbol_strings[] =
{
  str8_lit_comp("<<"),
  str8_lit_comp(">>"),
  str8_lit_comp("->"),
  str8_lit_comp("<="),
  str8_lit_comp(">="),
  str8_lit_comp("=="),
  str8_lit_comp("!="),
  str8_lit_comp("&&"),
  str8_lit_comp("||"),
  str8_lit_comp("=>"),
};

global read_only S64 e_max_precedence = 15;

////////////////////////////////
//~ rjf: Tokenization Functions

internal E_Token
e_token_zero(void)
{
  E_Token t = zero_struct;
  return t;
}

internal void
e_token_chunk_list_push(Arena *arena, E_TokenChunkList *list, U64 chunk_size, E_Token *token)
{
  E_TokenChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, E_TokenChunkNode, 1);
    SLLQueuePush(list->first, list->last, node);
    node->cap = chunk_size;
    node->v = push_array_no_zero(arena, E_Token, node->cap);
    list->node_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], token);
  node->count += 1;
  list->total_count += 1;
}

internal E_TokenArray
e_token_array_from_chunk_list(Arena *arena, E_TokenChunkList *list)
{
  E_TokenArray array = {0};
  array.count = list->total_count;
  array.v = push_array_no_zero(arena, E_Token, array.count);
  U64 idx = 0;
  for(E_TokenChunkNode *node = list->first; node != 0; node = node->next)
  {
    MemoryCopy(array.v+idx, node->v, sizeof(E_Token)*node->count);
  }
  return array;
}

internal E_TokenArray
e_token_array_from_text(Arena *arena, String8 text)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: lex loop
  E_TokenChunkList tokens = {0};
  U64 active_token_start_idx = 0;
  E_TokenKind active_token_kind = E_TokenKind_Null;
  B32 active_token_kind_started_with_tick = 0;
  B32 escaped = 0;
  B32 exp = 0;
  for(U64 idx = 0, advance = 0; idx <= text.size; idx += advance)
  {
    U8 byte      = (idx+0 < text.size) ? text.str[idx+0] : 0;
    U8 byte_next = (idx+1 < text.size) ? text.str[idx+1] : 0;
    U8 byte_next2= (idx+2 < text.size) ? text.str[idx+2] : 0;
    advance = 1;
    B32 token_formed = 0;
    U64 token_end_idx_pad = 0;
    switch(active_token_kind)
    {
      //- rjf: no active token -> seek token starter
      default:
      {
        if(char_is_alpha(byte) || byte == '_' || byte == '`' || byte == '$')
        {
          active_token_kind = E_TokenKind_Identifier;
          active_token_start_idx = idx;
          active_token_kind_started_with_tick = (byte == '`');
        }
        else if(char_is_digit(byte, 10) || (byte == '.' && char_is_digit(byte_next, 10)))
        {
          active_token_kind = E_TokenKind_Numeric;
          active_token_start_idx = idx;
        }
        else if(byte == '"')
        {
          active_token_kind = E_TokenKind_StringLiteral;
          active_token_start_idx = idx;
        }
        else if(byte == '\'')
        {
          active_token_kind = E_TokenKind_CharLiteral;
          active_token_start_idx = idx;
        }
        else if(byte == '~' || byte == '!' || byte == '%' || byte == '^' ||
                byte == '&' || byte == '*' || byte == '(' || byte == ')' ||
                byte == '-' || byte == '=' || byte == '+' || byte == '[' ||
                byte == ']' || byte == '{' || byte == '}' || byte == ':' ||
                byte == ';' || byte == ',' || byte == '.' || byte == '<' ||
                byte == '>' || byte == '/' || byte == '?' || byte == '|')
        {
          active_token_kind = E_TokenKind_Symbol;
          active_token_start_idx = idx;
        }
      }break;
      
      //- rjf: active tokens -> seek enders
      case E_TokenKind_Identifier:
      {
        if(byte == ':' && byte_next == ':' && (char_is_alpha(byte_next2) || byte_next2 == '_' || byte_next2 == '<'))
        {
          // NOTE(rjf): encountering C++-style namespaces - skip over scope resolution symbol
          // & keep going.
          advance = 2;
        }
        else if((byte == '\'' || byte == '`') && active_token_kind_started_with_tick)
        {
          // NOTE(rjf): encountering ` -> ' or ` -> ` style identifier escapes
          active_token_kind_started_with_tick = 0;
          advance = 1;
        }
        else if(byte == '<')
        {
          // NOTE(rjf): encountering C++-style templates - try to find ender. if no ender found,
          // assume this is an operator & just consume the identifier part.
          S64 nest = 1;
          for(U64 idx2 = idx+1; idx2 <= text.size; idx2 += 1)
          {
            if(idx2 < text.size && text.str[idx2] == '<')
            {
              nest += 1;
            }
            else if(idx2 < text.size && text.str[idx2] == '>')
            {
              nest -= 1;
              if(nest == 0)
              {
                advance = (idx2+1-idx);
                break;
              }
            }
            else if(idx2 == text.size && nest != 0)
            {
              token_formed = 1;
              advance = 0;
              break;
            }
          }
        }
        else if(!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && !active_token_kind_started_with_tick && byte != '@' && byte != '$')
        {
          advance = 0;
          token_formed = 1;
        }
      }break;
      case E_TokenKind_Numeric:
      {
        if(exp && (byte == '+' || byte == '-')){}
        else if(!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '.' && byte != ':')
        {
          advance = 0;
          token_formed = 1;
        }
        else
        {
          exp = 0;
          exp = (byte == 'e');
        }
      }break;
      case E_TokenKind_StringLiteral:
      {
        if(escaped == 0 && byte == '\\')
        {
          escaped = 1;
        }
        else if(escaped)
        {
          escaped = 0;
        }
        else if(escaped == 0 && byte == '"')
        {
          advance = 1;
          token_formed = 1;
          token_end_idx_pad = 1;
        }
      }break;
      case E_TokenKind_CharLiteral:
      {
        if(escaped == 0 && byte == '\\')
        {
          escaped = 1;
        }
        else if(escaped)
        {
          escaped = 0;
        }
        else if(escaped == 0 && byte == '\'')
        {
          advance = 1;
          token_formed = 1;
          token_end_idx_pad = 1;
        }
      }break;
      case E_TokenKind_Symbol:
      {
        if(byte != '~' && byte != '!' && byte != '%' && byte != '^' &&
           byte != '&' && byte != '*' && byte != '(' && byte != ')' &&
           byte != '-' && byte != '=' && byte != '+' && byte != '[' &&
           byte != ']' && byte != '{' && byte != '}' && byte != ':' &&
           byte != ';' && byte != ',' && byte != '.' && byte != '<' &&
           byte != '>' && byte != '/' && byte != '?' && byte != '|')
        {
          advance = 0;
          token_formed = 1;
        }
      }break;
    }
    
    //- rjf: token formed -> push new formed token(s)
    if(token_formed)
    {
      // rjf: non-symbols *or* symbols of only 1-length can be immediately
      // pushed as a token
      if(active_token_kind != E_TokenKind_Symbol || idx==active_token_start_idx+1)
      {
        E_Token token = {active_token_kind, r1u64(active_token_start_idx, idx+token_end_idx_pad)};
        e_token_chunk_list_push(scratch.arena, &tokens, 256, &token);
      }
      
      // rjf: symbolic strings matching `--` mean the remainder of the string
      // is reserved for external usage. the rest of the stream should not
      // be tokenized.
      else if(idx == active_token_start_idx+2 && text.str[active_token_start_idx] == '-' && text.str[active_token_start_idx+1] == '-')
      {
        break;
      }
      
      // rjf: if we got a symbol string of N>1 characters, then we need to
      // apply the maximum-munch rule, and produce M<=N tokens, where each
      // formed token is the maximum size possible, given the legal
      // >1-length symbol strings.
      else
      {
        U64 advance2 = 0;
        for(U64 idx2 = active_token_start_idx; idx2 < idx; idx2 += advance2)
        {
          advance2 = 1;
          for(U64 multichar_symbol_idx = 0;
              multichar_symbol_idx < ArrayCount(e_multichar_symbol_strings);
              multichar_symbol_idx += 1)
          {
            String8 multichar_symbol_string = e_multichar_symbol_strings[multichar_symbol_idx];
            String8 part_of_token = str8_substr(text, r1u64(idx2, idx2+multichar_symbol_string.size));
            if(str8_match(part_of_token, multichar_symbol_string, 0))
            {
              advance2 = multichar_symbol_string.size;
              break;
            }
          }
          E_Token token = {active_token_kind, r1u64(idx2, idx2+advance2)};
          e_token_chunk_list_push(scratch.arena, &tokens, 256, &token);
        }
      }
      
      // rjf: reset for subsequent tokens.
      active_token_kind = E_TokenKind_Null;
    }
  }
  
  //- rjf: chunk list -> array & return
  E_TokenArray array = e_token_array_from_chunk_list(arena, &tokens);
  scratch_end(scratch);
  return array;
}

internal E_TokenArray
e_token_array_make_first_opl(E_Token *first, E_Token *opl)
{
  E_TokenArray array = {first, (U64)(opl-first)};
  return array;
}

////////////////////////////////
//~ rjf: Context Selection Functions (Selection Required For All Subsequent APIs)

internal E_ParseCtx *
e_selected_parse_ctx(void)
{
  return e_parse_state->ctx;
}

internal void
e_select_parse_ctx(E_ParseCtx *ctx)
{
  if(e_parse_state == 0)
  {
    Arena *arena = arena_alloc();
    e_parse_state = push_array(arena, E_ParseState, 1);
    e_parse_state->arena = arena;
    e_parse_state->arena_eval_start_pos = arena_pos(arena);
  }
  arena_pop_to(e_parse_state->arena, e_parse_state->arena_eval_start_pos);
  if(ctx->modules == 0)        { ctx->modules = &e_module_nil; }
  if(ctx->primary_module == 0) { ctx->primary_module = &e_module_nil; }
  e_parse_state->ctx = ctx;
}

internal U32
e_parse_ctx_module_idx_from_rdi(RDI_Parsed *rdi)
{
  U32 result = 0;
  for(U64 idx = 0; idx < e_parse_state->ctx->modules_count; idx += 1)
  {
    if(e_parse_state->ctx->modules[idx].rdi == rdi)
    {
      result = (U32)idx;
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Expression Tree Building Functions

internal E_Expr *
e_push_expr(Arena *arena, E_ExprKind kind, void *location)
{
  E_Expr *e = push_array(arena, E_Expr, 1);
  e->first = e->last = e->next = e->prev = e->ref = &e_expr_nil;
  e->location = location;
  e->kind = kind;
  return e;
}

internal void
e_expr_insert_child(E_Expr *parent, E_Expr *prev, E_Expr *child)
{
  DLLInsert_NPZ(&e_expr_nil, parent->first, parent->last, prev, child, next, prev);
}

internal void
e_expr_push_child(E_Expr *parent, E_Expr *child)
{
  DLLPushBack_NPZ(&e_expr_nil, parent->first, parent->last, child, next, prev);
}

internal void
e_expr_remove_child(E_Expr *parent, E_Expr *child)
{
  DLLRemove_NPZ(&e_expr_nil, parent->first, parent->last, child, next, prev);
}

internal E_Expr *
e_expr_ref(Arena *arena, E_Expr *ref)
{
  E_Expr *expr = e_push_expr(arena, E_ExprKind_Ref, 0);
  expr->ref = ref;
  return expr;
}

internal E_Expr *
e_expr_ref_deref(Arena *arena, E_Expr *rhs)
{
  E_Expr *root = e_push_expr(arena, E_ExprKind_Deref, 0);
  E_Expr *rhs_ref = e_expr_ref(arena, rhs);
  e_expr_push_child(root, rhs_ref);
  return root;
}

internal E_Expr *
e_expr_ref_cast(Arena *arena, E_TypeKey type_key, E_Expr *rhs)
{
  E_Expr *root = e_push_expr(arena, E_ExprKind_Cast, 0);
  E_Expr *lhs = e_push_expr(arena, E_ExprKind_TypeIdent, 0);
  lhs->type_key = type_key;
  E_Expr *rhs_ref = e_expr_ref(arena, rhs);
  e_expr_push_child(root, lhs);
  e_expr_push_child(root, rhs_ref);
  return root;
}

internal E_Expr *
e_expr_copy(Arena *arena, E_Expr *src)
{
  E_Expr *result = &e_expr_nil;
  Temp scratch = scratch_begin(&arena, 1);
  if(src != &e_expr_nil)
  {
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      E_Expr *dst_parent;
      E_Expr *src;
      B32 is_ref;
    };
    Task start_task = {0, &e_expr_nil, src};
    Task *first_task = &start_task;
    Task *last_task = first_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      E_Expr *dst = e_push_expr(arena, t->src->kind, t->src->location);
      dst->mode      = t->src->mode;
      dst->space     = t->src->space;
      dst->type_key  = t->src->type_key;
      dst->value     = t->src->value;
      dst->string    = push_str8_copy(arena, t->src->string);
      dst->bytecode  = push_str8_copy(arena, t->src->bytecode);
      dst->qualifier = push_str8_copy(arena, t->src->qualifier);
      if(t->dst_parent == &e_expr_nil)
      {
        result = dst;
      }
      else if(t->is_ref)
      {
        t->dst_parent->ref = dst;
      }
      else
      {
        e_expr_push_child(t->dst_parent, dst);
      }
      if(t->src->ref != &e_expr_nil)
      {
        Task *task = push_array(scratch.arena, Task, 1);
        task->dst_parent = dst;
        task->src = t->src->ref;
        task->is_ref = 1;
        SLLQueuePush(first_task, last_task, task);
      }
      for(E_Expr *src_child = t->src->first; src_child != &e_expr_nil; src_child = src_child->next)
      {
        Task *task = push_array(scratch.arena, Task, 1);
        task->dst_parent = dst;
        task->src = src_child;
        SLLQueuePush(first_task, last_task, task);
      }
    }
  }
  scratch_end(scratch);
  return result;
}

internal void
e_expr_list_push(Arena *arena, E_ExprList *list, E_Expr *expr)
{
  E_ExprNode *n = push_array(arena, E_ExprNode, 1);
  n->v = expr;
  SLLQueuePush(list->first, list->last, n);
  list->count +=1;
}

////////////////////////////////
//~ rjf: Expression Tree -> String Conversions

internal void
e_append_strings_from_expr(Arena *arena, E_Expr *expr, String8List *out)
{
  switch(expr->kind)
  {
    default:
    {
      E_OpInfo *op_info = &e_expr_kind_op_info_table[expr->kind];
      String8 seps[] =
      {
        op_info->pre,
        op_info->sep,
        op_info->post,
      };
      U64 sep_idx = 0;
      for(E_Expr *child = expr->first;; child = child->next)
      {
        if(sep_idx == ArrayCount(seps)-1 && child != &e_expr_nil)
        {
          str8_list_push(arena, out, op_info->chain);
        }
        else
        {
          str8_list_push(arena, out, seps[sep_idx]);
          sep_idx += 1;
        }
        if(child == &e_expr_nil)
        {
          break;
        }
        E_OpInfo *child_op_info = &e_expr_kind_op_info_table[child->kind];
        B32 need_parens = (child_op_info->precedence > op_info->precedence);
        if(need_parens)
        {
          str8_list_pushf(arena, out, "(");
        }
        e_append_strings_from_expr(arena, child, out);
        if(need_parens)
        {
          str8_list_pushf(arena, out, ")");
        }
      }
    }break;
    case E_ExprKind_LeafBytecode:
    case E_ExprKind_LeafIdentifier:
    {
      str8_list_push(arena, out, expr->string);
    }break;
    case E_ExprKind_LeafStringLiteral:
    {
      str8_list_pushf(arena, out, "\"%S\"", expr->string);
    }break;
    case E_ExprKind_LeafU64:
    {
      str8_list_pushf(arena, out, "%I64u", expr->value.u64);
    }break;
    case E_ExprKind_LeafOffset:
    {
      str8_list_pushf(arena, out, "0x%I64x", expr->value.u64);
    }break;
    case E_ExprKind_LeafFilePath:
    {
      str8_list_pushf(arena, out, "file:\"%S\"", escaped_from_raw_str8(arena, expr->string));
    }break;
    case E_ExprKind_LeafF64:
    {
      str8_list_pushf(arena, out, "%f", expr->value.f64);
    }break;
    case E_ExprKind_LeafF32:
    {
      str8_list_pushf(arena, out, "%f", expr->value.f32);
    }break;
    case E_ExprKind_TypeIdent:
    {
      String8 type_string = e_type_string_from_key(arena, expr->type_key);
      str8_list_push(arena, out, type_string);
    }break;
    case E_ExprKind_Ref:
    {
      e_append_strings_from_expr(arena, expr->ref, out);
    }break;
  }
}

internal String8
e_string_from_expr(Arena *arena, E_Expr *expr)
{
  String8List strings = {0};
  e_append_strings_from_expr(arena, expr, &strings);
  String8 result = str8_list_join(arena, &strings, 0);
  return result;
}

////////////////////////////////
//~ rjf: Parsing Functions

internal E_TypeKey
e_leaf_type_from_name(String8 name)
{
  E_TypeKey key = zero_struct;
  B32 found = 0;
  if(!found)
  {
#define Case(str) (str8_match(name, str8_lit(str), 0))
    if(0){}
    else if(Case("u8") || Case("uint8") || Case("uint8_t") || Case("U8"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_U8);
    }
    else if(Case("uchar8") || Case("uchar"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_UChar8);
    }
    else if(Case("u16") || Case("uint16") || Case("uint16_t") || Case("U16"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_U16);
    }
    else if(Case("uchar16"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_UChar16);
    }
    else if(Case("u32") || Case("uint32") || Case("uint32_t") || Case("U32") || Case("uint"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_U32);
    }
    else if(Case("uchar32"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_UChar32);
    }
    else if(Case("u64") || Case("uint64") || Case("uint64_t") || Case("U64") || Case("size_t"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_U64);
    }
    else if(Case("s8") || Case("b8") || Case("B8") || Case("i8") || Case("int8") || Case("int8_t") || Case("S8"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_S8);
    }
    else if(Case("char8") || Case("char"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_Char8);
    }
    else if(Case("s16") || Case("b16") || Case("B16") || Case("i16") ||  Case("int16") || Case("int16_t") || Case("S16"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_S16);
    }
    else if(Case("char16"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_Char16);
    }
    else if(Case("s32") || Case("b32") || Case("B32") || Case("i32") || Case("int32") || Case("int32_t") || Case("char32") || Case("S32") || Case("int"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_S32);
    }
    else if(Case("char32"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_Char32);
    }
    else if(Case("s64") || Case("b64") || Case("B64") || Case("i64") || Case("int64") || Case("int64_t") || Case("S64") || Case("ssize_t"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_S64);
    }
    else if(Case("void"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_Void);
    }
    else if(Case("bool"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_Bool);
    }
    else if(Case("float") || Case("float32") || Case("f32") || Case("F32") || Case("r32") || Case("R32"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_F32);
    }
    else if(Case("double") || Case("float64") || Case("f64") || Case("F64") || Case("r64") || Case("R64"))
    {
      found = 1;
      key = e_type_key_basic(E_TypeKind_F64);
    }
#undef Case
  }
  if(!found)
  {
    for(U64 module_idx = 0; module_idx < e_parse_state->ctx->modules_count; module_idx += 1)
    {
      RDI_Parsed *rdi = e_parse_state->ctx->modules[module_idx].rdi;
      RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_Types);
      RDI_ParsedNameMap parsed_name_map = {0};
      rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
      RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, name.str, name.size);
      if(node != 0)
      {
        U32 match_count = 0;
        U32 *matches = rdi_matches_from_map_node(rdi, node, &match_count);
        if(match_count != 0)
        {
          RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, matches[0]);
          found = (type_node->kind != RDI_TypeKind_NULL);
          key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), matches[0], module_idx);
          break;
        }
      }
    }
  }
  return key;
}

internal E_TypeKey
e_type_from_expr(E_Expr *expr)
{
  E_TypeKey result = zero_struct;
  E_ExprKind kind = expr->kind;
  switch(kind)
  {
    // TODO(rjf): do we support E_ExprKind_Func here?
    default:{}break;
    case E_ExprKind_TypeIdent:
    {
      result = expr->type_key;
    }break;
    case E_ExprKind_Ptr:
    {
      E_TypeKey direct_type_key = e_type_from_expr(expr->first);
      result = e_type_key_cons_ptr(e_parse_state->ctx->primary_module->arch, direct_type_key, 1, 0);
    }break;
    case E_ExprKind_Array:
    {
      E_Expr *child_expr = expr->first;
      E_TypeKey direct_type_key = e_type_from_expr(child_expr);
      result = e_type_key_cons_array(direct_type_key, expr->value.u64, 0);
    }break;
  }
  return result;
}

internal void
e_push_leaf_ident_exprs_from_expr__in_place(Arena *arena, E_String2ExprMap *map, E_Expr *expr)
{
  switch(expr->kind)
  {
    default:
    {
      for(E_Expr *child = expr->first; child != &e_expr_nil; child = child->next)
      {
        e_push_leaf_ident_exprs_from_expr__in_place(arena, map, child);
      }
    }break;
    case E_ExprKind_Define:
    {
      E_Expr *exprl = expr->first;
      E_Expr *exprr = exprl->next;
      if(exprl->kind == E_ExprKind_LeafIdentifier)
      {
        e_string2expr_map_insert(arena, map, exprl->string, exprr);
      }
    }break;
  }
}

internal E_Parse
e_parse_type_from_text_tokens(Arena *arena, String8 text, E_TokenArray tokens)
{
  E_Parse parse = {0, &e_expr_nil, &e_expr_nil};
  E_Token *token_it = tokens.v;
  
  //- rjf: parse unsigned marker
  B32 unsigned_marker = 0;
  {
    E_Token token = e_token_at_it(token_it, &tokens);
    if(token.kind == E_TokenKind_Identifier)
    {
      String8 token_string = str8_substr(text, token.range);
      if(str8_match(token_string, str8_lit("unsigned"), 0))
      {
        token_it += 1;
        unsigned_marker = 1;
      }
    }
  }
  
  //- rjf: parse base type
  {
    E_Token token = e_token_at_it(token_it, &tokens);
    if(token.kind == E_TokenKind_Identifier)
    {
      String8 token_string = str8_substr(text, token.range);
      if(token_string.size >= 2 &&
         token_string.str[0] == '`' &&
         token_string.str[token_string.size-1] == '`')
      {
        token_string = str8_substr(token_string, r1u64(1, token_string.size-1));
      }
      E_TypeKey type_key = e_leaf_type_from_name(token_string);
      if(!e_type_key_match(e_type_key_zero(), type_key))
      {
        token_it += 1;
        
        // rjf: apply unsigned marker to base type
        if(unsigned_marker) switch(e_type_kind_from_key(type_key))
        {
          default:{}break;
          case E_TypeKind_Char8: {type_key = e_type_key_basic(E_TypeKind_UChar8);}break;
          case E_TypeKind_Char16:{type_key = e_type_key_basic(E_TypeKind_UChar16);}break;
          case E_TypeKind_Char32:{type_key = e_type_key_basic(E_TypeKind_UChar32);}break;
          case E_TypeKind_S8:  {type_key = e_type_key_basic(E_TypeKind_U8);}break;
          case E_TypeKind_S16: {type_key = e_type_key_basic(E_TypeKind_U16);}break;
          case E_TypeKind_S32: {type_key = e_type_key_basic(E_TypeKind_U32);}break;
          case E_TypeKind_S64: {type_key = e_type_key_basic(E_TypeKind_U64);}break;
          case E_TypeKind_S128:{type_key = e_type_key_basic(E_TypeKind_U128);}break;
          case E_TypeKind_S256:{type_key = e_type_key_basic(E_TypeKind_U256);}break;
          case E_TypeKind_S512:{type_key = e_type_key_basic(E_TypeKind_U512);}break;
        }
        
        // rjf: construct leaf type
        parse.exprs.first = parse.exprs.last = e_push_expr(arena, E_ExprKind_TypeIdent, token_string.str);
        parse.exprs.first->type_key = type_key;
      }
    }
  }
  
  //- rjf: parse extensions
  if(parse.exprs.first != &e_expr_nil)
  {
    for(;;)
    {
      E_Token token = e_token_at_it(token_it, &tokens);
      if(token.kind != E_TokenKind_Symbol)
      {
        break;
      }
      String8 token_string = str8_substr(text, token.range);
      if(str8_match(token_string, str8_lit("*"), 0))
      {
        token_it += 1;
        E_Expr *ptee = parse.exprs.first;
        parse.exprs.first = parse.exprs.last = e_push_expr(arena, E_ExprKind_Ptr, token_string.str);
        e_expr_push_child(parse.exprs.first, ptee);
      }
      else
      {
        break;
      }
    }
  }
  
  //- rjf: fill parse & end
  parse.last_token = token_it;
  return parse;
}

internal E_Parse
e_parse_expr_from_text_tokens__prec(Arena *arena, String8 text, E_TokenArray tokens, S64 max_precedence, U64 max_chain_count)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  E_Token *it = tokens.v;
  E_Token *it_opl = tokens.v + tokens.count;
  E_Parse result = {0, &e_expr_nil, &e_expr_nil};
  
  //////////////////////////////
  //- rjf: parse chain of expressions
  //
  for(U64 chain_count = 0; it < it_opl && chain_count < max_chain_count;)
  {
    ////////////////////////////
    //- rjf: exit on symbols callers may be waiting on
    //
    {
      E_Token token = e_token_at_it(it, &tokens);
      String8 token_string = str8_substr(text, token.range);
      if(token.kind == E_TokenKind_Symbol &&
         (str8_match(token_string, str8_lit(")"), 0) ||
          str8_match(token_string, str8_lit("]"), 0) ||
          str8_match(token_string, str8_lit(":"), 0) ||
          str8_match(token_string, str8_lit("?"), 0)))
      {
        break;
      }
    }
    
    ////////////////////////////
    //- rjf: skip commas, semicolons, etc.
    //
    for(;it < it_opl;)
    {
      E_Token token = e_token_at_it(it, &tokens);
      String8 token_string = str8_substr(text, token.range);
      if(token.kind == E_TokenKind_Symbol &&
         (str8_match(token_string, str8_lit(","), 0) ||
          str8_match(token_string, str8_lit(";"), 0)))
      {
        it += 1;
      }
      else
      {
        break;
      }
    }
    
    ////////////////////////////
    //- rjf: parse prefix unaries
    //
    typedef struct PrefixUnaryTask PrefixUnaryTask;
    struct PrefixUnaryTask
    {
      PrefixUnaryTask *next;
      E_ExprKind kind;
      E_Expr *cast_expr;
      void *location;
    };
    PrefixUnaryTask *first_prefix_unary = 0;
    PrefixUnaryTask *last_prefix_unary = 0;
    {
      for(;it < it_opl;)
      {
        E_Token *start_it = it;
        E_Token token = e_token_at_it(it, &tokens);
        String8 token_string = str8_substr(text, token.range);
        S64 prefix_unary_precedence = 0;
        E_ExprKind prefix_unary_kind = 0;
        E_Expr *cast_expr = &e_expr_nil;
        void *location = 0;
        
        // rjf: try op table
        for EachNonZeroEnumVal(E_ExprKind, k)
        {
          E_OpInfo *op_info = &e_expr_kind_op_info_table[k];
          if(op_info->kind == E_OpKind_UnaryPrefix && str8_match(str8_skip_chop_whitespace(op_info->pre), token_string, 0))
          {
            prefix_unary_precedence = op_info->precedence;
            prefix_unary_kind = k;
            break;
          }
        }
        
        // rjf: consume valid op
        if(prefix_unary_precedence != 0)
        {
          location = token_string.str;
          it += 1;
        }
        
        // rjf: try casting expression
        if(prefix_unary_precedence == 0 && str8_match(token_string, str8_lit("("), 0))
        {
          E_Token some_type_identifier_maybe = e_token_at_it(it+1, &tokens);
          String8 some_type_identifier_maybe_string = str8_substr(text, some_type_identifier_maybe.range);
          if(some_type_identifier_maybe.kind == E_TokenKind_Identifier)
          {
            E_TypeKey type_key = e_leaf_type_from_name(some_type_identifier_maybe_string);
            if(!e_type_key_match(type_key, e_type_key_zero()) || str8_match(some_type_identifier_maybe_string, str8_lit("unsigned"), 0))
            {
              // rjf: move past open paren
              it += 1;
              
              // rjf: parse type expr
              E_Parse type_parse = e_parse_type_from_text_tokens(arena, text, e_token_array_make_first_opl(it, it_opl));
              E_Expr *type = type_parse.exprs.last;
              e_msg_list_concat_in_place(&result.msgs, &type_parse.msgs);
              it = type_parse.last_token;
              location = token_string.str;
              
              // rjf: expect )
              E_Token close_paren_maybe = e_token_at_it(it, &tokens);
              String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
              if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit(")"), 0))
              {
                e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Missing `)`.");
              }
              
              // rjf: consume )
              else
              {
                it += 1;
              }
              
              // rjf: fill
              prefix_unary_precedence = 2;
              prefix_unary_kind = E_ExprKind_Cast;
              cast_expr = type;
            }
          }
        }
        
        // rjf: break if we got no operators
        if(prefix_unary_precedence == 0)
        {
          break;
        }
        
        // rjf: break if the token node iterator has not changed
        if(it == start_it)
        {
          break;
        }
        
        // rjf: push prefix unary if we got one
        {
          PrefixUnaryTask *op_n = push_array(scratch.arena, PrefixUnaryTask, 1);
          op_n->kind = prefix_unary_kind;
          op_n->cast_expr = cast_expr;
          op_n->location = location;
          SLLQueuePushFront(first_prefix_unary, last_prefix_unary, op_n);
        }
      }
    }
    
    ////////////////////////////
    //- rjf: parse atom
    //
    E_Expr *atom = &e_expr_nil;
    String8 atom_implicit_member_name = {0};
    if(it < it_opl)
    {
      E_Token token = e_token_at_it(it, &tokens);
      String8 token_string = str8_substr(text, token.range);
      
      //////////////////////////
      //- rjf: consume resolution qualifiers
      //
      String8 resolution_qualifier = {0};
      if(token.kind == E_TokenKind_Identifier)
      {
        E_Token next_token = e_token_at_it(it+1, &tokens);
        String8 next_token_string = str8_substr(text, next_token.range);
        if(next_token.kind == E_TokenKind_Symbol && str8_match(next_token_string, str8_lit(":"), 0))
        {
          it += 2;
          resolution_qualifier = token_string;
          token = e_token_at_it(it, &tokens);
          token_string = str8_substr(text, token.range);
        }
      }
      
      //////////////////////////
      //- rjf: descent to nested expression (...)
      //
      if(token.kind == E_TokenKind_Symbol && str8_match(token_string, str8_lit("("), 0))
      {
        // rjf: skip (
        it += 1;
        
        // rjf: parse () contents
        E_Parse nested_parse = e_parse_expr_from_text_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
        e_msg_list_concat_in_place(&result.msgs, &nested_parse.msgs);
        atom = nested_parse.exprs.last;
        it = nested_parse.last_token;
        
        // rjf: expect )
        E_Token close_paren_maybe = e_token_at_it(it, &tokens);
        String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
        if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit(")"), 0))
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Missing `)`.");
        }
        
        // rjf: consume )
        else
        {
          it += 1;
        }
      }
      
      //////////////////////////
      //- rjf: descent to assembly-style dereference sub-expression [...]
      //
      else if(token.kind == E_TokenKind_Symbol && str8_match(token_string, str8_lit("["), 0))
      {
        // rjf: skip [
        it += 1;
        
        // rjf: parse [] contents
        E_Parse nested_parse = e_parse_expr_from_text_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
        e_msg_list_concat_in_place(&result.msgs, &nested_parse.msgs);
        atom = nested_parse.exprs.last;
        it = nested_parse.last_token;
        
        // rjf: build cast-to-U64*, and dereference operators
        if(nested_parse.exprs.last == &e_expr_nil)
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Expected expression following `[`.");
        }
        else
        {
          E_Expr *type = e_push_expr(arena, E_ExprKind_TypeIdent, token_string.str);
          type->type_key = e_type_key_cons_ptr(e_parse_state->ctx->primary_module->arch, e_type_key_basic(E_TypeKind_U64), 1, 0);
          E_Expr *casted = atom;
          E_Expr *cast = e_push_expr(arena, E_ExprKind_Cast, token_string.str);
          e_expr_push_child(cast, type);
          e_expr_push_child(cast, casted);
          atom = e_push_expr(arena, E_ExprKind_Deref, token_string.str);
          e_expr_push_child(atom, cast);
        }
        
        // rjf: expect ]
        E_Token close_paren_maybe = e_token_at_it(it, &tokens);
        String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
        if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit("]"), 0))
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Missing `]`.");
        }
        
        // rjf: consume )
        else
        {
          it += 1;
        }
      }
      
      //////////////////////////
      //- rjf: leaf identifier
      //
      else if(token.kind == E_TokenKind_Identifier)
      {
        atom = e_push_expr(arena, E_ExprKind_LeafIdentifier, token_string.str);
        atom->string = token_string;
        it += 1;
      }
      
      //////////////////////////
      //- rjf: leaf numeric
      //
      else if(token.kind == E_TokenKind_Numeric)
      {
        U64 dot_pos = str8_find_needle(token_string, 0, str8_lit("."), 0);
        it += 1;
        
        // rjf: no . => integral
        if(dot_pos == token_string.size)
        {
          U64 val = 0;
          try_u64_from_str8_c_rules(token_string, &val);
          atom = e_push_expr(arena, E_ExprKind_LeafU64, token_string.str);
          atom->value.u64 = val;
        }
        
        // rjf: presence of . => double or float
        if(dot_pos < token_string.size)
        {
          F64 val = f64_from_str8(token_string);
          U64 f_pos = str8_find_needle(token_string, 0, str8_lit("f"), StringMatchFlag_CaseInsensitive);
          
          // rjf: presence of f after . => f32
          if(f_pos < token_string.size)
          {
            atom = e_push_expr(arena, E_ExprKind_LeafF32, token_string.str);
            atom->value.f32 = val;
          }
          
          // rjf: no f => f64
          else
          {
            atom = e_push_expr(arena, E_ExprKind_LeafF64, token_string.str);
            atom->value.f64 = val;
          }
        }
      }
      
      //////////////////////////
      //- rjf: leaf char literal
      //
      else if(token.kind == E_TokenKind_CharLiteral)
      {
        it += 1;
        if(token_string.size > 1 && token_string.str[0] == '\'' && token_string.str[1] != '\'')
        {
          String8 char_literal_escaped = str8_skip(str8_chop(token_string, 1), 1);
          String8 char_literal_raw = raw_from_escaped_str8(scratch.arena, char_literal_escaped);
          U8 char_val = char_literal_raw.size > 0 ? char_literal_raw.str[0] : 0;
          atom = e_push_expr(arena, E_ExprKind_LeafU64, token_string.str);
          atom->value.u64 = (U64)char_val;
        }
        else
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Malformed character literal.");
        }
      }
      
      //////////////////////////
      //- rjf: filesystem-qualified leaf string literal
      //
      else if(token.kind == E_TokenKind_StringLiteral &&
              (str8_match(resolution_qualifier, str8_lit("file"), 0) ||
               str8_match(resolution_qualifier, str8_lit("folder"), 0)))
      {
        String8 string_value_escaped = str8_chop(str8_skip(token_string, 1), 1);
        String8 string_value_raw = raw_from_escaped_str8(arena, string_value_escaped);
        atom = e_push_expr(arena, E_ExprKind_LeafFilePath, token_string.str);
        atom->string = string_value_raw;
        it += 1;
      }
      
      //////////////////////////
      //- rjf: leaf string literal
      //
      else if(token.kind == E_TokenKind_StringLiteral)
      {
        String8 string_value_escaped = str8_chop(str8_skip(token_string, 1), 1);
        String8 string_value_raw = raw_from_escaped_str8(arena, string_value_escaped);
        atom = e_push_expr(arena, E_ExprKind_LeafStringLiteral, token_string.str);
        atom->string = string_value_raw;
        it += 1;
      }
      
      //- rjf: upgrade atom w/ qualifier
      if(atom != &e_expr_nil && resolution_qualifier.size != 0)
      {
        atom->qualifier = resolution_qualifier;
      }
    }
    
    ////////////////////////////
    //- rjf: upgrade atom w/ postfix unaries
    //
    if(atom != &e_expr_nil) for(;it < it_opl;)
    {
      E_Token token = e_token_at_it(it, &tokens);
      String8 token_string = str8_substr(text, token.range);
      B32 is_postfix_unary = 0;
      
      // rjf: dot/arrow operator
      if(max_precedence >= 1 &&
         token.kind == E_TokenKind_Symbol &&
         (str8_match(token_string, str8_lit("."), 0) ||
          str8_match(token_string, str8_lit("->"), 0)))
      {
        is_postfix_unary = 1;
        
        // rjf: advance past operator
        it += 1;
        
        // rjf: look for member name
        E_Token member_name_maybe = e_token_at_it(it, &tokens);
        String8 member_name_maybe_string = str8_substr(text, member_name_maybe.range);
        
        // rjf: if we have a member name, build dot-operator tree
        if(member_name_maybe.kind == E_TokenKind_Identifier)
        {
          it += 1;
          E_Expr *lhs = atom;
          E_Expr *rhs = e_push_expr(arena, E_ExprKind_LeafIdentifier, member_name_maybe_string.str);
          rhs->string = member_name_maybe_string;
          atom = e_push_expr(arena, E_ExprKind_MemberAccess, token_string.str);
          e_expr_push_child(atom, lhs);
          e_expr_push_child(atom, rhs);
        }
        
        // rjf: no identifier after `.`? -> error
        else
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Missing identifier after `.`.");
        }
      }
      
      // rjf: array index
      if(token.kind == E_TokenKind_Symbol &&
         str8_match(token_string, str8_lit("["), 0))
      {
        is_postfix_unary = 1;
        
        // rjf: advance past [
        it += 1;
        
        // rjf: parse indexing expression
        E_Parse idx_expr_parse = e_parse_expr_from_text_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
        e_msg_list_concat_in_place(&result.msgs, &idx_expr_parse.msgs);
        it = idx_expr_parse.last_token;
        
        // rjf: valid indexing expression => produce index expr
        if(idx_expr_parse.exprs.last != &e_expr_nil)
        {
          E_Expr *array_expr = atom;
          E_Expr *index_expr = idx_expr_parse.exprs.last;
          atom = e_push_expr(arena, E_ExprKind_ArrayIndex, token_string.str);
          e_expr_push_child(atom, array_expr);
          e_expr_push_child(atom, index_expr);
        }
        
        // rjf: expect ]
        {
          E_Token close_brace_maybe = e_token_at_it(it, &tokens);
          String8 close_brace_maybe_string = str8_substr(text, close_brace_maybe.range);
          if(close_brace_maybe.kind != E_TokenKind_Symbol || !str8_match(close_brace_maybe_string, str8_lit("]"), 0))
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Unclosed `[`.");
          }
          else
          {
            it += 1;
          }
        }
      }
      
      // rjf: calls
      if(token.kind == E_TokenKind_Symbol &&
         str8_match(token_string, str8_lit("("), 0))
      {
        is_postfix_unary = 1;
        
        // rjf: skip (
        it += 1;
        
        // rjf: parse all argument expressions
        E_Expr *callee_expr = atom;
        E_Expr *call_expr = e_push_expr(arena, E_ExprKind_Call, token_string.str);
        call_expr->string = callee_expr->string;
        e_expr_push_child(call_expr, callee_expr);
        E_Parse args_parse = e_parse_expr_from_text_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, max_U64);
        e_msg_list_concat_in_place(&result.msgs, &args_parse.msgs);
        it = args_parse.last_token;
        if(args_parse.exprs.first != &e_expr_nil)
        {
          call_expr->last->next = args_parse.exprs.first;
          args_parse.exprs.first->prev = call_expr->last;
          call_expr->last = args_parse.exprs.last;
        }
        atom = call_expr;
        
        // rjf: expect )
        {
          E_Token close_paren_maybe = e_token_at_it(it, &tokens);
          String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
          if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit(")"), 0))
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Unclosed `(`.");
          }
          else
          {
            it += 1;
          }
        }
      }
      
      // rjf: quit if this doesn't look like any patterns of postfix unary we know
      if(!is_postfix_unary)
      {
        break;
      }
    }
    
    ////////////////////////////
    //- rjf: no `atom`, but we have a cast expression? -> just evaluate the type as the atom
    //
    if(atom == &e_expr_nil && first_prefix_unary != 0 && first_prefix_unary->cast_expr != &e_expr_nil)
    {
      atom = first_prefix_unary->cast_expr;
      first_prefix_unary = first_prefix_unary->next;
    }
    
    ////////////////////////////
    //- rjf: upgrade `atom` w/ previously parsed prefix unaries
    //
    if(atom != &e_expr_nil)
    {
      for(PrefixUnaryTask *prefix_unary = first_prefix_unary;
          prefix_unary != 0;
          prefix_unary = prefix_unary->next)
      {
        E_Expr *rhs = atom;
        atom = e_push_expr(arena, prefix_unary->kind, prefix_unary->location);
        if(prefix_unary->cast_expr != &e_expr_nil)
        {
          e_expr_push_child(atom, prefix_unary->cast_expr);
        }
        e_expr_push_child(atom, rhs);
      }
    }
    else if(first_prefix_unary != 0)
    {
      e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, last_prefix_unary->location, "Missing expression.");
    }
    
    ////////////////////////////
    //- rjf: parse complex operators to further extend `atom`
    //
    if(atom != &e_expr_nil) for(;it < it_opl;)
    {
      E_Token *start_it = it;
      E_Token token = e_token_at_it(it, &tokens);
      String8 token_string = str8_substr(text, token.range);
      
      //- rjf: parse binaries
      {
        // rjf: first try to find a matching binary operator
        S64 binary_precedence = 0;
        E_ExprKind binary_kind = 0;
        for EachNonZeroEnumVal(E_ExprKind, k)
        {
          E_OpInfo *op_info = &e_expr_kind_op_info_table[k];
          if(op_info->kind == E_OpKind_Binary && str8_match(str8_skip_chop_whitespace(op_info->sep), token_string, 0))
          {
            binary_precedence = op_info->precedence;
            binary_kind = k;
            break;
          }
        }
        
        // rjf: if we got a valid binary precedence, and it's not to be handled by
        // a caller, then we need to parse the right-hand-side with a tighter
        // precedence
        if(binary_precedence != 0 && binary_precedence <= max_precedence)
        {
          E_Parse rhs_expr_parse = e_parse_expr_from_text_tokens__prec(arena, text, e_token_array_make_first_opl(it+1, it_opl), binary_precedence-1, 1);
          e_msg_list_concat_in_place(&result.msgs, &rhs_expr_parse.msgs);
          E_Expr *rhs = rhs_expr_parse.exprs.last;
          it = rhs_expr_parse.last_token;
          if(rhs == &e_expr_nil)
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Missing right-hand-side of `%S`.", token_string);
          }
          else
          {
            E_Expr *lhs = atom;
            atom = e_push_expr(arena, binary_kind, token_string.str);
            e_expr_push_child(atom, lhs);
            e_expr_push_child(atom, rhs);
          }
        }
      }
      
      //- rjf: parse ternaries
      {
        if(token.kind == E_TokenKind_Symbol && str8_match(token_string, str8_lit("?"), 0) && 13 <= max_precedence)
        {
          it += 1;
          
          // rjf: parse middle expression
          E_Parse middle_expr_parse = e_parse_expr_from_text_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
          it = middle_expr_parse.last_token;
          E_Expr *middle_expr = middle_expr_parse.exprs.last;
          e_msg_list_concat_in_place(&result.msgs, &middle_expr_parse.msgs);
          if(middle_expr_parse.exprs.last == &e_expr_nil)
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Expected expression after `?`.");
          }
          
          // rjf: expect :
          B32 got_colon = 0;
          E_Token colon_token = zero_struct;
          String8 colon_token_string = {0};
          {
            E_Token colon_token_maybe = e_token_at_it(it, &tokens);
            String8 colon_token_maybe_string = str8_substr(text, colon_token_maybe.range);
            if(colon_token_maybe.kind != E_TokenKind_Symbol || !str8_match(colon_token_maybe_string, str8_lit(":"), 0))
            {
              e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token_string.str, "Expected `:` after `?`.");
            }
            else
            {
              got_colon = 1;
              colon_token = colon_token_maybe;
              colon_token_string = colon_token_maybe_string;
              it += 1;
            }
          }
          
          // rjf: parse rhs
          E_Parse rhs_expr_parse = e_parse_expr_from_text_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
          if(got_colon)
          {
            it = rhs_expr_parse.last_token;
            e_msg_list_concat_in_place(&result.msgs, &rhs_expr_parse.msgs);
            if(rhs_expr_parse.exprs.last == &e_expr_nil)
            {
              e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, colon_token_string.str, "Expected expression after `:`.");
            }
          }
          
          // rjf: build ternary
          if(atom != &e_expr_nil &&
             middle_expr_parse.exprs.last != &e_expr_nil &&
             rhs_expr_parse.exprs.last != &e_expr_nil)
          {
            E_Expr *lhs = atom;
            E_Expr *mhs = middle_expr_parse.exprs.last;
            E_Expr *rhs = rhs_expr_parse.exprs.last;
            atom = e_push_expr(arena, E_ExprKind_Ternary, token_string.str);
            e_expr_push_child(atom, lhs);
            e_expr_push_child(atom, mhs);
            e_expr_push_child(atom, rhs);
          }
        }
      }
      
      // rjf: if we parsed nothing successfully, we're done
      if(it == start_it)
      {
        break;
      }
    }
    
    //- rjf: store parsed atom to expression chain - if we didn't get an expression, break
    if(atom != &e_expr_nil)
    {
      DLLPushBack_NPZ(&e_expr_nil, result.exprs.first, result.exprs.last, atom, next, prev);
      chain_count += 1;
    }
    else
    {
      break;
    }
  }
  
  //- rjf: fill result & return
  result.last_token = it;
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal E_Parse
e_parse_expr_from_text_tokens(Arena *arena, String8 text, E_TokenArray tokens)
{
  ProfBegin("parse '%.*s'", str8_varg(text));
  E_Parse parse = e_parse_expr_from_text_tokens__prec(arena, text, tokens, e_max_precedence, max_U64);
  ProfEnd();
  return parse;
}

internal E_Parse
e_parse_expr_from_text(Arena *arena, String8 text)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_TokenArray tokens = e_token_array_from_text(scratch.arena, text);
  E_Parse parse = e_parse_expr_from_text_tokens(arena, text, tokens);
  scratch_end(scratch);
  return parse;
}
