// Copyright (c) Epic Games Tools
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
    idx += node->count;
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
//~ rjf: Expression Tree Building Functions

internal E_Expr *
e_push_expr(Arena *arena, E_ExprKind kind, Rng1U64 range)
{
  E_Expr *e = push_array(arena, E_Expr, 1);
  e->first = e->last = e->next = e->prev = e->ref = &e_expr_nil;
  e->range = range;
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
  E_Expr *expr = e_push_expr(arena, E_ExprKind_Ref, ref->range);
  expr->ref = ref;
  return expr;
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
      B32 is_sib;
    };
    Task start_task = {0, &e_expr_nil, src};
    Task *first_task = &start_task;
    Task *last_task = first_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      E_Expr *dst = e_push_expr(arena, t->src->kind, t->src->range);
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
      else if(t->is_sib)
      {
        t->dst_parent->next = dst;
        dst->prev = t->dst_parent;
      }
      else
      {
        e_expr_push_child(t->dst_parent, dst);
      }
      if(t->src->next != &e_expr_nil)
      {
        Task *task = push_array(scratch.arena, Task, 1);
        task->dst_parent = dst;
        task->src = t->src->next;
        task->is_sib = 1;
        SLLQueuePush(first_task, last_task, task);
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
e_append_strings_from_expr(Arena *arena, E_Expr *expr, String8 parent_expr_string, String8List *out)
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
        e_append_strings_from_expr(arena, child, parent_expr_string, out);
        if(need_parens)
        {
          str8_list_pushf(arena, out, ")");
        }
      }
    }break;
    case E_ExprKind_LeafBytecode:
    case E_ExprKind_LeafIdentifier:
    {
      if(str8_match(expr->string, str8_lit("$"), 0) && parent_expr_string.size != 0)
      {
        str8_list_push(arena, out, parent_expr_string);
      }
      else
      {
        str8_list_push(arena, out, expr->string);
      }
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
      e_append_strings_from_expr(arena, expr->ref, parent_expr_string, out);
    }break;
  }
}

internal String8
e_string_from_expr(Arena *arena, E_Expr *expr, String8 parent_expr_string)
{
  String8List strings = {0};
  e_append_strings_from_expr(arena, expr, parent_expr_string, &strings);
  String8 result = str8_list_join(arena, &strings, 0);
  return result;
}

////////////////////////////////
//~ rjf: Parsing Functions

internal E_TypeKey
e_leaf_builtin_type_key_from_name(String8 name)
{
  E_TypeKey result = {0};
  if(0){}
#define BuiltInType_XList \
BasicCase("uint8", U8)\
BasicCase("uint8_t", U8)\
BasicCase("uchar", UChar8)\
BasicCase("uchar8", UChar8)\
BasicCase("uint16", U16)\
BasicCase("uint16_t", U16)\
BasicCase("uchar16", UChar16)\
BasicCase("uint32", U32)\
BasicCase("uint32_t", U32)\
BasicCase("uchar32", UChar32)\
BasicCase("uint64", U64)\
BasicCase("uint64_t", U64)\
BasicCase("uint128", U128)\
BasicCase("uint128_t", U128)\
BasicCase("uint256", U256)\
BasicCase("uint256_t", U256)\
BasicCase("uint512", U512)\
BasicCase("uint512_t", U512)\
BasicCase("int8", S8)\
BasicCase("int8_t", S8)\
BasicCase("char", Char8)\
BasicCase("char8", Char8)\
BasicCase("int16", S16)\
BasicCase("int16_t", S16)\
BasicCase("char16", Char16)\
BasicCase("int32", S32)\
BasicCase("int32_t", S32)\
BasicCase("char32", Char32)\
BasicCase("int64", S64)\
BasicCase("int64_t", S64)\
BasicCase("int128", S128)\
BasicCase("int128_t", S128)\
BasicCase("int256", S256)\
BasicCase("int256_t", S256)\
BasicCase("int512", S512)\
BasicCase("int512_t", S512)\
BasicCase("void", Void)\
BasicCase("bool", Bool)\
BasicCase("float", F32)\
BasicCase("float32", F32)\
BasicCase("double", F64)\
BasicCase("float64", F64)
#define BasicCase(str, kind) else if(str8_match(name, str8_lit(str), 0)) {result = e_type_key_basic(E_TypeKind_##kind);}
  BuiltInType_XList
#undef BasicCase
  return result;
}

internal E_TypeKey
e_leaf_type_key_from_name(String8 name)
{
  E_TypeKey key = e_leaf_builtin_type_key_from_name(name);
  if(!e_type_key_match(e_type_key_zero(), key))
  {
    DI_Match match = di_match_from_name(e_base_ctx->dbgi_match_store, name, 0);
    if(match.section == RDI_SectionKind_TypeNodes)
    {
      E_Module *module = &e_base_ctx->modules[match.dbgi_idx];
      RDI_Parsed *rdi = module->rdi;
      U32 type_idx = match.idx;
      RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, type_idx);
      key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)match.dbgi_idx);
    }
  }
  return key;
}

internal E_TypeKey
e_type_key_from_expr(E_Expr *expr)
{
  E_TypeKey result = zero_struct;
  E_ExprKind kind = expr->kind;
  switch(kind)
  {
    // TODO(rjf): do we support E_ExprKind_Func here?
    default:{}break;
    case E_ExprKind_LeafIdentifier:
    {
      result = e_leaf_type_key_from_name(expr->string);
    }break;
    case E_ExprKind_TypeIdent:
    {
      result = expr->type_key;
    }break;
    case E_ExprKind_Ptr:
    {
      E_TypeKey direct_type_key = e_type_key_from_expr(expr->first);
      result = e_type_key_cons_ptr(e_base_ctx->primary_module->arch, direct_type_key, 1, 0);
    }break;
    case E_ExprKind_Array:
    {
      E_Expr *child_expr = expr->first;
      E_TypeKey direct_type_key = e_type_key_from_expr(child_expr);
      result = e_type_key_cons_array(direct_type_key, expr->value.u64, 0);
    }break;
  }
  return result;
}

internal E_Parse
e_push_type_parse_from_text_tokens(Arena *arena, String8 text, E_TokenArray tokens)
{
  E_Parse parse = {tokens, 0, &e_expr_nil, &e_expr_nil};
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
      E_TypeKey type_key = e_leaf_type_key_from_name(token_string);
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
        parse.expr = e_push_expr(arena, E_ExprKind_TypeIdent, token.range);
        parse.expr->type_key = type_key;
      }
    }
  }
  
  //- rjf: parse extensions
  if(parse.expr != &e_expr_nil)
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
        E_Expr *ptee = parse.expr;
        parse.expr = e_push_expr(arena, E_ExprKind_Ptr, token.range);
        e_expr_push_child(parse.expr, ptee);
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
e_push_parse_from_string_tokens__prec(Arena *arena, String8 text, E_TokenArray tokens, S64 max_precedence, U64 max_chain_count)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  E_Token *it = tokens.v;
  E_Token *it_opl = tokens.v + tokens.count;
  E_Parse result = {tokens, 0, &e_expr_nil, &e_expr_nil};
  
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
    //- rjf: parse atom, gather prefix unary tasks
    //
    typedef struct PrefixUnaryTask PrefixUnaryTask;
    struct PrefixUnaryTask
    {
      PrefixUnaryTask *next;
      E_ExprKind kind;
      Rng1U64 range;
      E_Expr *cast_type_expr;
    };
    PrefixUnaryTask *first_prefix_unary = 0;
    PrefixUnaryTask *last_prefix_unary = 0;
    E_Expr *atom = &e_expr_nil;
    B32 atom_is_maybe_cast = 0;
    for(B32 done = 0; !done && it < it_opl;)
    {
      //////////////////////////
      //- rjf: prefix unary operators
      //
      {
        E_Token token = e_token_at_it(it, &tokens);
        String8 token_string = str8_substr(text, token.range);
        S64 prefix_unary_precedence = 0;
        E_ExprKind prefix_unary_kind = 0;
        E_Expr *prefix_unary_cast_expr = &e_expr_nil;
        
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
        
        // rjf: if we found a symbolic prefix unary operator, but we are
        // looking for a casted expression, then we need to abort this
        // path. C-style casts are only legal in very simple and unambiguous
        // cases, e.g. (x)123, but they cannot be made legal in more
        // complex cases like (x) * y, because this is fundamentally ambiguous
        // (the meaning / tree shape / etc. is entirely different depending on
        // the type / mode of `x`).
        //
        // because of things like hover-evaluation we do actually want to
        // support basic C-style casts. but past a certain point of complexity,
        // we will simply require usage of the explicit `cast` operator.
        //
        if(prefix_unary_precedence != 0 && atom_is_maybe_cast)
        {
          break;
        }
        
        // rjf: try 'unsigned' marker
        if(str8_match(token_string, str8_lit("unsigned"), 0))
        {
          prefix_unary_kind = E_ExprKind_Unsigned;
          prefix_unary_precedence = 2;
        }
        
        // rjf: try explicit cast
        if(str8_match(token_string, str8_lit("cast"), 0))
        {
          // rjf: consume cast & open paren
          E_Token open_paren_maybe = e_token_at_it(it+1, &tokens);
          String8 open_paren_maybe_string = str8_substr(text, open_paren_maybe.range);
          if(!str8_match(open_paren_maybe_string, str8_lit("("), 0))
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Expected `(` following `cast`.");
            goto end_cast_parse;
          }
          it += 2;
          
          // rjf: parse type expression
          E_Parse type_parse = e_push_parse_from_string_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
          e_msg_list_concat_in_place(&result.msgs, &type_parse.msgs);
          it = type_parse.last_token;
          
          // rjf: expect )
          E_Token close_paren_maybe = e_token_at_it(it, &tokens);
          String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
          if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit(")"), 0))
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Missing `)`.");
          }
          
          // rjf: require type
          if(type_parse.expr == &e_expr_nil)
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Expected type in `cast(...)`.");
          }
          
          // rjf: fill prefix unary info
          else
          {
            prefix_unary_kind = E_ExprKind_Cast;
            prefix_unary_precedence = 2;
            prefix_unary_cast_expr = type_parse.expr;
          }
          end_cast_parse:;
        }
        
        // rjf: push prefix unary if we got one
        if(prefix_unary_precedence != 0)
        {
          PrefixUnaryTask *prefix_unary_task = push_array(scratch.arena, PrefixUnaryTask, 1);
          prefix_unary_task->kind = prefix_unary_kind;
          prefix_unary_task->range = token.range;
          prefix_unary_task->cast_type_expr = prefix_unary_cast_expr;
          SLLQueuePush(first_prefix_unary, last_prefix_unary, prefix_unary_task);
          it += 1;
        }
      }
      
      //////////////////////////
      //- rjf: try to parse an atom
      //
      if(atom == &e_expr_nil || atom_is_maybe_cast)
      {
        B32 got_new_atom = 0;
        E_Expr *maybe_cast = atom_is_maybe_cast ? atom : &e_expr_nil;
        atom_is_maybe_cast = 0;
        
        ////////////////////////
        //- rjf: consume resolution qualifiers
        //
        String8 resolution_qualifier = {0};
        {
          E_Token token = e_token_at_it(it, &tokens);
          String8 token_string = str8_substr(text, token.range);
          if(token.kind == E_TokenKind_Identifier)
          {
            E_Token next_token = e_token_at_it(it+1, &tokens);
            String8 next_token_string = str8_substr(text, next_token.range);
            if(next_token.range.min == token.range.max && next_token.kind == E_TokenKind_Symbol && str8_match(next_token_string, str8_lit(":"), 0))
            {
              it += 2;
              resolution_qualifier = token_string;
            }
          }
        }
        
        ////////////////////////
        //- rjf: descent to nested expression (...)
        //
        if(!got_new_atom)
        {
          E_Token token = e_token_at_it(it, &tokens);
          String8 token_string = str8_substr(text, token.range);
          if(token.kind == E_TokenKind_Symbol && str8_match(token_string, str8_lit("("), 0))
          {
            // rjf: skip (
            it += 1;
            
            // rjf: parse () contents
            E_Parse nested_parse = e_push_parse_from_string_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
            e_msg_list_concat_in_place(&result.msgs, &nested_parse.msgs);
            atom = nested_parse.expr;
            it = nested_parse.last_token;
            atom_is_maybe_cast = 1;
            got_new_atom = 1;
            
            // rjf: expect )
            E_Token close_paren_maybe = e_token_at_it(it, &tokens);
            String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
            if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit(")"), 0))
            {
              e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Missing `)`.");
            }
            
            // rjf: consume )
            else
            {
              it += 1;
            }
          }
        }
        
        ////////////////////////
        //- rjf: descent to assembly-style dereference sub-expression [...]
        //
        if(atom == &e_expr_nil && !got_new_atom)
        {
          E_Token token = e_token_at_it(it, &tokens);
          String8 token_string = str8_substr(text, token.range);
          if(token.kind == E_TokenKind_Symbol && str8_match(token_string, str8_lit("["), 0))
          {
            // rjf: skip [
            it += 1;
            
            // rjf: parse [] contents
            E_Parse nested_parse = e_push_parse_from_string_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
            e_msg_list_concat_in_place(&result.msgs, &nested_parse.msgs);
            atom = nested_parse.expr;
            it = nested_parse.last_token;
            got_new_atom = 1;
            
            // rjf: build cast-to-U64*, and dereference operators
            if(nested_parse.expr == &e_expr_nil)
            {
              e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Expected expression following `[`.");
            }
            else
            {
              E_Expr *type = e_push_expr(arena, E_ExprKind_TypeIdent, token.range);
              type->type_key = e_type_key_cons_ptr(e_base_ctx->primary_module->arch, e_type_key_basic(E_TypeKind_U64), 1, 0);
              E_Expr *casted = atom;
              E_Expr *cast = e_push_expr(arena, E_ExprKind_Cast, token.range);
              e_expr_push_child(cast, type);
              e_expr_push_child(cast, casted);
              atom = e_push_expr(arena, E_ExprKind_Deref, token.range);
              e_expr_push_child(atom, cast);
            }
            
            // rjf: expect ]
            E_Token close_paren_maybe = e_token_at_it(it, &tokens);
            String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
            if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit("]"), 0))
            {
              e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Missing `]`.");
            }
            
            // rjf: consume )
            else
            {
              it += 1;
            }
          }
        }
        
        ////////////////////////
        //- rjf: leaf identifier
        //
        if(!got_new_atom)
        {
          E_Token token = e_token_at_it(it, &tokens);
          String8 token_string = str8_substr(text, token.range);
          
          // rjf: skip no-op prefix keywords
          if(token.kind == E_TokenKind_Identifier &&
             (str8_match(token_string, str8_lit("struct"), 0) ||
              str8_match(token_string, str8_lit("union"), 0) ||
              str8_match(token_string, str8_lit("enum"), 0) ||
              str8_match(token_string, str8_lit("class"), 0) ||
              str8_match(token_string, str8_lit("typename"), 0)))
          {
            it += 1;
            token = e_token_at_it(it, &tokens);
            token_string = str8_substr(text, token.range);
          }
          
          // rjf: build identifier atom
          if(token.kind == E_TokenKind_Identifier)
          {
            String8 identifier_string = token_string;
            if(identifier_string.size >= 2 && identifier_string.str[0] == '`' && identifier_string.str[identifier_string.size-1] == '`')
            {
              identifier_string = str8_skip(str8_chop(identifier_string, 1), 1);
            }
            atom = e_push_expr(arena, E_ExprKind_LeafIdentifier, token.range);
            atom->string = identifier_string;
            it += 1;
            got_new_atom = 1;
          }
        }
        
        ////////////////////////
        //- rjf: leaf numeric
        //
        if(!got_new_atom)
        {
          E_Token token = e_token_at_it(it, &tokens);
          String8 token_string = str8_substr(text, token.range);
          if(token.kind == E_TokenKind_Numeric)
          {
            U64 dot_pos = str8_find_needle(token_string, 0, str8_lit("."), 0);
            it += 1;
            
            // rjf: no . => integral
            if(dot_pos == token_string.size)
            {
              U64 val = 0;
              try_u64_from_str8_c_rules(token_string, &val);
              atom = e_push_expr(arena, E_ExprKind_LeafU64, token.range);
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
                atom = e_push_expr(arena, E_ExprKind_LeafF32, token.range);
                atom->value.f32 = val;
              }
              
              // rjf: no f => f64
              else
              {
                atom = e_push_expr(arena, E_ExprKind_LeafF64, token.range);
                atom->value.f64 = val;
              }
            }
            
            got_new_atom = 1;
          }
        }
        
        ////////////////////////
        //- rjf: leaf char literal
        //
        if(!got_new_atom)
        {
          E_Token token = e_token_at_it(it, &tokens);
          String8 token_string = str8_substr(text, token.range);
          if(token.kind == E_TokenKind_CharLiteral)
          {
            it += 1;
            if(token_string.size > 1 && token_string.str[0] == '\'' && token_string.str[1] != '\'')
            {
              String8 char_literal_escaped = str8_skip(str8_chop(token_string, 1), 1);
              String8 char_literal_raw = raw_from_escaped_str8(scratch.arena, char_literal_escaped);
              U8 char_val = char_literal_raw.size > 0 ? char_literal_raw.str[0] : 0;
              atom = e_push_expr(arena, E_ExprKind_LeafU64, token.range);
              atom->value.u64 = (U64)char_val;
              got_new_atom = 1;
            }
            else
            {
              e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Malformed character literal.");
            }
          }
        }
        
        ////////////////////////
        //- rjf: filesystem-qualified leaf string literal
        //
        if(!got_new_atom)
        {
          E_Token token = e_token_at_it(it, &tokens);
          String8 token_string = str8_substr(text, token.range);
          if(token.kind == E_TokenKind_StringLiteral &&
             (str8_match(resolution_qualifier, str8_lit("file"), 0) ||
              str8_match(resolution_qualifier, str8_lit("folder"), 0)))
          {
            String8 string_value_escaped = str8_chop(str8_skip(token_string, 1), 1);
            String8 string_value_raw = raw_from_escaped_str8(arena, string_value_escaped);
            atom = e_push_expr(arena, E_ExprKind_LeafFilePath, token.range);
            atom->string = string_value_raw;
            it += 1;
            got_new_atom = 1;
          }
        }
        
        ////////////////////////
        //- rjf: leaf string literal
        //
        if(!got_new_atom)
        {
          E_Token token = e_token_at_it(it, &tokens);
          String8 token_string = str8_substr(text, token.range);
          if(token.kind == E_TokenKind_StringLiteral)
          {
            String8 string_value_escaped = str8_chop(str8_skip(token_string, 1), 1);
            String8 string_value_raw = raw_from_escaped_str8(arena, string_value_escaped);
            atom = e_push_expr(arena, E_ExprKind_LeafStringLiteral, token.range);
            atom->string = string_value_raw;
            it += 1;
            got_new_atom = 1;
          }
        }
        
        ////////////////////////
        //- rjf: upgrade atom w/ qualifier
        //
        if(atom != &e_expr_nil && resolution_qualifier.size != 0)
        {
          atom->qualifier = resolution_qualifier;
        }
        
        ////////////////////////
        //- rjf: got new atom, but we had a potential cast atom? -> gather cast operator
        //
        if(got_new_atom && maybe_cast != &e_expr_nil)
        {
          PrefixUnaryTask *prefix_unary_task = push_array(scratch.arena, PrefixUnaryTask, 1);
          prefix_unary_task->kind = E_ExprKind_Cast;
          prefix_unary_task->range = maybe_cast->range;
          prefix_unary_task->cast_type_expr = maybe_cast;
          SLLQueuePush(first_prefix_unary, last_prefix_unary, prefix_unary_task);
        }
      }
      
      ////////////////////////
      //- rjf: if our atom is not potentially a cast, *or* if we simply did not get an atom, 
      // then we need to stop parsing at this stage.
      //
      done = (!atom_is_maybe_cast || atom == &e_expr_nil);
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
        B32 member_name_is_good = (member_name_maybe.kind == E_TokenKind_Identifier);
        
        // rjf: build dot-operator tree
        E_Expr *lhs = atom;
        E_Expr *rhs = &e_expr_nil;
        if(member_name_is_good)
        {
          rhs = e_push_expr(arena, E_ExprKind_LeafIdentifier, member_name_maybe.range);
          rhs->string = member_name_maybe_string;
        }
        atom = e_push_expr(arena, E_ExprKind_MemberAccess, token.range);
        e_expr_push_child(atom, lhs);
        if(member_name_is_good)
        {
          e_expr_push_child(atom, rhs);
        }
        
        // rjf: no identifier after `.`? -> error
        if(member_name_is_good)
        {
          it += 1;
        }
        else
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Missing member name after `%S`.", token_string);
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
        E_Parse idx_expr_parse = e_push_parse_from_string_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
        e_msg_list_concat_in_place(&result.msgs, &idx_expr_parse.msgs);
        it = idx_expr_parse.last_token;
        
        // rjf: valid indexing expression => produce index expr
        if(idx_expr_parse.expr != &e_expr_nil)
        {
          E_Expr *array_expr = atom;
          E_Expr *index_expr = idx_expr_parse.expr;
          atom = e_push_expr(arena, E_ExprKind_ArrayIndex, token.range);
          e_expr_push_child(atom, array_expr);
          e_expr_push_child(atom, index_expr);
        }
        
        // rjf: expect ]
        {
          E_Token close_brace_maybe = e_token_at_it(it, &tokens);
          String8 close_brace_maybe_string = str8_substr(text, close_brace_maybe.range);
          if(close_brace_maybe.kind != E_TokenKind_Symbol || !str8_match(close_brace_maybe_string, str8_lit("]"), 0))
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Unclosed `[`.");
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
        E_Expr *call_expr = e_push_expr(arena, E_ExprKind_Call, token.range);
        call_expr->string = callee_expr->string;
        e_expr_push_child(call_expr, callee_expr);
        E_Parse args_parse = e_push_parse_from_string_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, max_U64);
        e_msg_list_concat_in_place(&result.msgs, &args_parse.msgs);
        it = args_parse.last_token;
        if(args_parse.expr != &e_expr_nil)
        {
          call_expr->last->next = args_parse.expr;
          args_parse.expr->prev = call_expr->last;
          for(E_Expr *arg = args_parse.expr; arg != &e_expr_nil; arg = arg->next)
          {
            call_expr->last = arg;
          }
        }
        atom = call_expr;
        
        // rjf: expect )
        {
          E_Token close_paren_maybe = e_token_at_it(it, &tokens);
          String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
          if(close_paren_maybe.kind != E_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit(")"), 0))
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Unclosed `(`.");
            call_expr->range.max = text.size;
          }
          else
          {
            call_expr->range = union_1u64(call_expr->range, close_paren_maybe.range);
            it += 1;
          }
        }
      }
      
      // rjf: "as" style casts
      if(token.kind == E_TokenKind_Identifier &&
         str8_match(token_string, str8_lit("as"), 0))
      {
        it += 1;
        
        // rjf: parse type expression
        E_Parse type_parse = e_push_parse_from_string_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
        e_msg_list_concat_in_place(&result.msgs, &type_parse.msgs);
        it = type_parse.last_token;
        
        // rjf: require type
        if(type_parse.expr == &e_expr_nil)
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Expected type following `as`.");
        }
        
        // rjf: build cast expr
        else
        {
          E_Expr *rhs = atom;
          atom = e_push_expr(arena, E_ExprKind_Cast, token.range);
          e_expr_push_child(atom, type_parse.expr);
          e_expr_push_child(atom, rhs);
        }
      }
      
      // rjf: quit if this doesn't look like any patterns of postfix unary we know
      if(!is_postfix_unary)
      {
        break;
      }
    }
    
    ////////////////////////////
    //- rjf: no atom, just single `unsigned` prefix unary? -> unsigned int type expr
    //
    if(atom == &e_expr_nil &&
       first_prefix_unary != 0 &&
       first_prefix_unary->kind == E_ExprKind_Unsigned)
    {
      atom = e_push_expr(arena, E_ExprKind_LeafIdentifier, first_prefix_unary->cast_type_expr->range);
      atom->string = str8_lit("int");
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
        if(prefix_unary->kind == E_ExprKind_Cast)
        {
          E_Expr *rhs = atom;
          atom = e_push_expr(arena, prefix_unary->kind, prefix_unary->range);
          e_expr_push_child(atom, prefix_unary->cast_type_expr);
          e_expr_push_child(atom, rhs);
        }
        else
        {
          E_Expr *rhs = atom;
          atom = e_push_expr(arena, prefix_unary->kind, prefix_unary->range);
          e_expr_push_child(atom, rhs);
        }
      }
    }
    else if(first_prefix_unary != 0)
    {
      e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, last_prefix_unary->range, "Missing expression.");
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
          E_Parse rhs_expr_parse = e_push_parse_from_string_tokens__prec(arena, text, e_token_array_make_first_opl(it+1, it_opl), binary_precedence-1, 1);
          e_msg_list_concat_in_place(&result.msgs, &rhs_expr_parse.msgs);
          E_Expr *rhs = rhs_expr_parse.expr;
          it = rhs_expr_parse.last_token;
          if(rhs == &e_expr_nil && binary_kind == E_ExprKind_Mul)
          {
            // NOTE(rjf): C-style pointer syntax is shared with multiplication.
            // carving out a special case here to allow "unfinished *s" to be
            // treated as pointers instead.
            E_Expr *ptee = atom;
            atom = e_push_expr(arena, E_ExprKind_Ptr, token.range);
            e_expr_push_child(atom, ptee);
          }
          else if(rhs == &e_expr_nil)
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Missing right-hand-side of `%S`.", token_string);
          }
          else
          {
            E_Expr *lhs = atom;
            atom = e_push_expr(arena, binary_kind, token.range);
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
          E_Parse middle_expr_parse = e_push_parse_from_string_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
          it = middle_expr_parse.last_token;
          E_Expr *middle_expr = middle_expr_parse.expr;
          e_msg_list_concat_in_place(&result.msgs, &middle_expr_parse.msgs);
          if(middle_expr_parse.expr == &e_expr_nil)
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Expected expression after `?`.");
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
              e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, token.range, "Expected `:` after `?`.");
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
          E_Parse rhs_expr_parse = e_push_parse_from_string_tokens__prec(arena, text, e_token_array_make_first_opl(it, it_opl), e_max_precedence, 1);
          if(got_colon)
          {
            it = rhs_expr_parse.last_token;
            e_msg_list_concat_in_place(&result.msgs, &rhs_expr_parse.msgs);
            if(rhs_expr_parse.expr == &e_expr_nil)
            {
              e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, colon_token.range, "Expected expression after `:`.");
            }
          }
          
          // rjf: build ternary
          if(atom != &e_expr_nil &&
             middle_expr_parse.expr != &e_expr_nil &&
             rhs_expr_parse.expr != &e_expr_nil)
          {
            E_Expr *lhs = atom;
            E_Expr *mhs = middle_expr_parse.expr;
            E_Expr *rhs = rhs_expr_parse.expr;
            atom = e_push_expr(arena, E_ExprKind_Ternary, token.range);
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
      DLLPushBack_NPZ(&e_expr_nil, result.expr, result.last_expr, atom, next, prev);
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
e_push_parse_from_string(Arena *arena, String8 text)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_TokenArray tokens = e_token_array_from_text(scratch.arena, text);
  E_Parse parse = e_push_parse_from_string_tokens__prec(arena, text, tokens, e_max_precedence, max_U64);
  scratch_end(scratch);
  return parse;
}
