// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Tables

global read_only String8 eval_g_multichar_symbol_strings[] =
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
};

global read_only struct {EVAL_ExprKind kind; String8 string; S64 precedence;} eval_g_unary_prefix_op_table[] =
{
  // { EVAL_ExprKind_???, str8_lit_comp("+"), 2 },
  { EVAL_ExprKind_Neg,    str8_lit_comp("-"),      2 },
  { EVAL_ExprKind_LogNot, str8_lit_comp("!"),      2 },
  { EVAL_ExprKind_Deref,  str8_lit_comp("*"),      2 },
  { EVAL_ExprKind_Address,str8_lit_comp("&"),      2 },
  { EVAL_ExprKind_Sizeof, str8_lit_comp("sizeof"), 2 },
  // { EVAL_ExprKind_Alignof, str8_lit_comp("_Alignof"), 2 },
};

global read_only struct {EVAL_ExprKind kind; String8 string; S64 precedence;} eval_g_binary_op_table[] =
{
  { EVAL_ExprKind_Mul,    str8_lit_comp("*"),  3  },
  { EVAL_ExprKind_Div,    str8_lit_comp("/"),  3  },
  { EVAL_ExprKind_Mod,    str8_lit_comp("%"),  3  },
  { EVAL_ExprKind_Add,    str8_lit_comp("+"),  4  },
  { EVAL_ExprKind_Sub,    str8_lit_comp("-"),  4  },
  { EVAL_ExprKind_LShift, str8_lit_comp("<<"), 5  },
  { EVAL_ExprKind_RShift, str8_lit_comp(">>"), 5  },
  { EVAL_ExprKind_Less,   str8_lit_comp("<"),  6  },
  { EVAL_ExprKind_LsEq,   str8_lit_comp("<="), 6  },
  { EVAL_ExprKind_Grtr,   str8_lit_comp(">"),  6  },
  { EVAL_ExprKind_GrEq,   str8_lit_comp(">="), 6  },
  { EVAL_ExprKind_EqEq,   str8_lit_comp("=="), 7  },
  { EVAL_ExprKind_NtEq,   str8_lit_comp("!="), 7  },
  { EVAL_ExprKind_BitAnd, str8_lit_comp("&"),  8  },
  { EVAL_ExprKind_BitXor, str8_lit_comp("^"),  9  },
  { EVAL_ExprKind_BitOr,  str8_lit_comp("|"),  10 },
  { EVAL_ExprKind_LogAnd, str8_lit_comp("&&"), 11 },
  { EVAL_ExprKind_LogOr,  str8_lit_comp("||"), 12 },
};

global read_only S64 eval_g_max_precedence = 15;

////////////////////////////////
//~ rjf: Basic Functions

internal U64
eval_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

////////////////////////////////
//~ rjf: Map Functions

internal EVAL_String2NumMap
eval_string2num_map_make(Arena *arena, U64 slot_count)
{
  EVAL_String2NumMap map = {0};
  map.slots_count = slot_count;
  map.slots = push_array(arena, EVAL_String2NumMapSlot, map.slots_count);
  return map;
}

internal void
eval_string2num_map_insert(Arena *arena, EVAL_String2NumMap *map, String8 string, U64 num)
{
  U64 hash = eval_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  EVAL_String2NumMapNode *existing_node = 0;
  for(EVAL_String2NumMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
  {
    if(str8_match(node->string, string, 0) && node->num == num)
    {
      existing_node = node;
      break;
    }
  }
  if(existing_node == 0)
  {
    EVAL_String2NumMapNode *node = push_array(arena, EVAL_String2NumMapNode, 1);
    SLLQueuePush_N(map->slots[slot_idx].first, map->slots[slot_idx].last, node, hash_next);
    SLLQueuePush_N(map->first, map->last, node, order_next);
    node->string = push_str8_copy(arena, string);
    node->num = num;
  }
}

internal U64
eval_num_from_string(EVAL_String2NumMap *map, String8 string)
{
  U64 num = 0;
  if(map->slots_count != 0)
  {
    U64 hash = eval_hash_from_string(string);
    U64 slot_idx = hash%map->slots_count;
    EVAL_String2NumMapNode *existing_node = 0;
    for(EVAL_String2NumMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
    {
      if(str8_match(node->string, string, 0))
      {
        existing_node = node;
        break;
      }
    }
    if(existing_node != 0)
    {
      num = existing_node->num;
    }
  }
  return num;
}

////////////////////////////////
//~ rjf: Map Building Fast Paths

internal EVAL_String2NumMap *
eval_push_locals_map_from_raddbg_voff(Arena *arena, RADDBG_Parsed *rdbg, U64 voff)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: gather scopes to walk
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    RADDBG_Scope *scope;
  };
  Task *first_task = 0;
  Task *last_task = 0;
  
  //- rjf: voff -> tightest scope
  RADDBG_Scope *tightest_scope = 0;
  if(rdbg->scope_vmap != 0 && rdbg->scopes != 0)
  {
    U64 scope_idx = raddbg_vmap_idx_from_voff(rdbg->scope_vmap, rdbg->scope_vmap_count, voff);
    RADDBG_Scope *scope = &rdbg->scopes[scope_idx];
    Task *task = push_array(scratch.arena, Task, 1);
    task->scope = scope;
    SLLQueuePush(first_task, last_task, task);
    tightest_scope = scope;
  }
  
  //- rjf: voff-1 -> scope
  if(voff > 0 && rdbg->scope_vmap != 0 && rdbg->scopes != 0)
  {
    U64 scope_idx = raddbg_vmap_idx_from_voff(rdbg->scope_vmap, rdbg->scope_vmap_count, voff-1);
    RADDBG_Scope *scope = &rdbg->scopes[scope_idx];
    if(scope != tightest_scope)
    {
      Task *task = push_array(scratch.arena, Task, 1);
      task->scope = scope;
      SLLQueuePush(first_task, last_task, task);
    }
  }
  
  //- rjf: tightest scope -> walk up the tree & build tasks for each parent scope
  if(tightest_scope != 0)
  {
    for(RADDBG_Scope *scope = &rdbg->scopes[tightest_scope->parent_scope_idx];
        scope != 0 && scope != &rdbg->scopes[0];
        scope = &rdbg->scopes[scope->parent_scope_idx])
    {
      Task *task = push_array(scratch.arena, Task, 1);
      task->scope = scope;
      SLLQueuePush(first_task, last_task, task);
    }
  }
  
  //- rjf: build blank map
  EVAL_String2NumMap *map = push_array(arena, EVAL_String2NumMap, 1);
  *map = eval_string2num_map_make(arena, 1024);
  
  //- rjf: accumulate locals for all tasks
  for(Task *task = first_task; task != 0; task = task->next)
  {
    RADDBG_Scope *scope = task->scope;
    if(scope != 0)
    {
      U32 local_opl_idx = scope->local_first + scope->local_count;
      for(U32 local_idx = scope->local_first; local_idx < local_opl_idx; local_idx += 1)
      {
        RADDBG_Local *local_var = &rdbg->locals[local_idx];
        U64 local_name_size = 0;
        U8 *local_name_str = raddbg_string_from_idx(rdbg, local_var->name_string_idx, &local_name_size);
        String8 name = push_str8_copy(arena, str8(local_name_str, local_name_size));
        eval_string2num_map_insert(arena, map, name, (U64)local_idx+1);
      }
    }
  }
  
  scratch_end(scratch);
  return map;
}

internal EVAL_String2NumMap *
eval_push_member_map_from_raddbg_voff(Arena *arena, RADDBG_Parsed *rdbg, U64 voff)
{
  //- rjf: voff -> tightest scope
  RADDBG_Scope *tightest_scope = 0;
  if(rdbg->scope_vmap != 0 && rdbg->scopes != 0)
  {
    U64 scope_idx = raddbg_vmap_idx_from_voff(rdbg->scope_vmap, rdbg->scope_vmap_count, voff);
    tightest_scope = &rdbg->scopes[scope_idx];
  }
  
  //- rjf: tightest scope -> procedure
  RADDBG_Procedure *procedure = 0;
  if(tightest_scope != 0 && rdbg->procedures != 0)
  {
    U32 proc_idx = tightest_scope->proc_idx;
    if(0 < proc_idx && proc_idx < rdbg->procedures_count)
    {
      procedure = &rdbg->procedures[proc_idx];
    }
  }
  
  //- rjf: procedure -> udt
  RADDBG_UDT *udt = 0;
  if(procedure != 0 && rdbg->udts != 0 && procedure->link_flags & RADDBG_LinkFlag_TypeScoped)
  {
    U32 udt_idx = procedure->container_idx;
    if(0 < udt_idx && udt_idx < rdbg->udts_count)
    {
      udt = &rdbg->udts[udt_idx];
    }
  }
  
  //- rjf: build blank map
  EVAL_String2NumMap *map = push_array(arena, EVAL_String2NumMap, 1);
  *map = eval_string2num_map_make(arena, 64);
  
  //- rjf: udt -> fill member map
  if(udt != 0 && !(udt->flags & RADDBG_UserDefinedTypeFlag_EnumMembers) && rdbg->members != 0)
  {
    U64 data_member_num = 1;
    for(U32 member_idx = udt->member_first;
        member_idx < udt->member_first+udt->member_count;
        member_idx += 1)
    {
      if(member_idx < 1 || rdbg->members_count <= member_idx)
      {
        break;
      }
      RADDBG_Member *m = &rdbg->members[member_idx];
      if(m->kind == RADDBG_MemberKind_DataField)
      {
        String8 name = {0};
        name.str = raddbg_string_from_idx(rdbg, m->name_string_idx, &name.size);
        eval_string2num_map_insert(arena, map, name, data_member_num);
        data_member_num += 1;
      }
    }
  }
  
  return map;
}

////////////////////////////////
//~ rjf: Tokenization Functions

internal EVAL_Token
eval_token_zero(void)
{
  EVAL_Token token = zero_struct;
  return token;
}

internal void
eval_token_chunk_list_push(Arena *arena, EVAL_TokenChunkList *list, U64 chunk_size, EVAL_Token *token)
{
  EVAL_TokenChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, EVAL_TokenChunkNode, 1);
    SLLQueuePush(list->first, list->last, node);
    node->cap = chunk_size;
    node->v = push_array_no_zero(arena, EVAL_Token, node->cap);
    list->node_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], token);
  node->count += 1;
  list->total_count += 1;
}

internal EVAL_TokenArray
eval_token_array_from_chunk_list(Arena *arena, EVAL_TokenChunkList *list)
{
  EVAL_TokenArray array = {0};
  array.count = list->total_count;
  array.v = push_array_no_zero(arena, EVAL_Token, array.count);
  U64 idx = 0;
  for(EVAL_TokenChunkNode *node = list->first; node != 0; node = node->next)
  {
    MemoryCopy(array.v+idx, node->v, sizeof(EVAL_Token)*node->count);
  }
  return array;
}

internal EVAL_TokenArray
eval_token_array_from_text(Arena *arena, String8 text)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: lex loop
  EVAL_TokenChunkList tokens = {0};
  U64 active_token_start_idx = 0;
  EVAL_TokenKind active_token_kind = EVAL_TokenKind_Null;
  B32 active_token_kind_started_with_tick = 0;
  B32 escaped = 0;
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
          active_token_kind = EVAL_TokenKind_Identifier;
          active_token_start_idx = idx;
          active_token_kind_started_with_tick = (byte == '`');
        }
        else if(char_is_digit(byte, 10) || (byte == '.' && char_is_digit(byte_next, 10)))
        {
          active_token_kind = EVAL_TokenKind_Numeric;
          active_token_start_idx = idx;
        }
        else if(byte == '"')
        {
          active_token_kind = EVAL_TokenKind_StringLiteral;
          active_token_start_idx = idx;
        }
        else if(byte == '\'')
        {
          active_token_kind = EVAL_TokenKind_CharLiteral;
          active_token_start_idx = idx;
        }
        else if(byte == '~' || byte == '!' || byte == '%' || byte == '^' ||
                byte == '&' || byte == '*' || byte == '(' || byte == ')' ||
                byte == '-' || byte == '=' || byte == '+' || byte == '[' ||
                byte == ']' || byte == '{' || byte == '}' || byte == ':' ||
                byte == ';' || byte == ',' || byte == '.' || byte == '<' ||
                byte == '>' || byte == '/' || byte == '?' || byte == '|')
        {
          active_token_kind = EVAL_TokenKind_Symbol;
          active_token_start_idx = idx;
        }
      }break;
      
      //- rjf: active tokens -> seek enders
      case EVAL_TokenKind_Identifier:
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
      case EVAL_TokenKind_Numeric:
      {
        if(!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '.')
        {
          advance = 0;
          token_formed = 1;
        }
      }break;
      case EVAL_TokenKind_StringLiteral:
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
      case EVAL_TokenKind_CharLiteral:
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
      case EVAL_TokenKind_Symbol:
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
      if(active_token_kind != EVAL_TokenKind_Symbol || idx==active_token_start_idx+1)
      {
        EVAL_Token token = {active_token_kind, r1u64(active_token_start_idx, idx+token_end_idx_pad)};
        eval_token_chunk_list_push(scratch.arena, &tokens, 256, &token);
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
              multichar_symbol_idx < ArrayCount(eval_g_multichar_symbol_strings);
              multichar_symbol_idx += 1)
          {
            String8 multichar_symbol_string = eval_g_multichar_symbol_strings[multichar_symbol_idx];
            String8 part_of_token = str8_substr(text, r1u64(idx2, idx2+multichar_symbol_string.size));
            if(str8_match(part_of_token, multichar_symbol_string, 0))
            {
              advance2 = multichar_symbol_string.size;
              break;
            }
          }
          EVAL_Token token = {active_token_kind, r1u64(idx2, idx2+advance2)};
          eval_token_chunk_list_push(scratch.arena, &tokens, 256, &token);
        }
      }
      
      // rjf: reset for subsequent tokens.
      active_token_kind = EVAL_TokenKind_Null;
    }
  }
  
  //- rjf: chunk list -> array & return
  EVAL_TokenArray array = eval_token_array_from_chunk_list(arena, &tokens);
  scratch_end(scratch);
  ProfEnd();
  return array;
}

internal EVAL_TokenArray
eval_token_array_make_first_opl(EVAL_Token *first, EVAL_Token *opl)
{
  EVAL_TokenArray array = {first, (U64)(opl-first)};
  return array;
}

////////////////////////////////
//~ rjf: Parser Functions

internal TG_Key
eval_leaf_type_from_name(RADDBG_Parsed *rdbg, String8 name)
{
  TG_Key key = zero_struct;
  B32 found = 0;
  if(rdbg->type_nodes != 0)
  {
    RADDBG_NameMap *name_map = raddbg_name_map_from_kind(rdbg, RADDBG_NameMapKind_Types);
    RADDBG_ParsedNameMap parsed_name_map = {0};
    raddbg_name_map_parse(rdbg, name_map, &parsed_name_map);
    RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &parsed_name_map, name.str, name.size);
    if(node != 0)
    {
      U32 match_count = 0;
      U32 *matches = raddbg_matches_from_map_node(rdbg, node, &match_count);
      if(match_count != 0)
      {
        RADDBG_TypeNode *type_node = raddbg_element_from_idx(rdbg, type_nodes, matches[0]);
        found = type_node->kind != RADDBG_TypeKind_NULL;
        key = tg_key_ext(tg_kind_from_raddbg_type_kind(type_node->kind), (U64)matches[0]);
      }
    }
  }
  if(!found)
  {
#define Case(str) (str8_match(name, str8_lit(str), 0))
    if(Case("u8") || Case("uint8") || Case("uint8_t") || Case("uchar8") || Case("U8"))
    {
      key = tg_key_basic(TG_Kind_U8);
    }
    else if(Case("u16") || Case("uint16") || Case("uint16_t") || Case("uchar16") || Case("U16"))
    {
      key = tg_key_basic(TG_Kind_U16);
    }
    else if(Case("u32") || Case("uint32") || Case("uint32_t") || Case("uchar32") || Case("U32") || Case("uint"))
    {
      key = tg_key_basic(TG_Kind_U32);
    }
    else if(Case("u64") || Case("uint64") || Case("uint64_t") || Case("U64"))
    {
      key = tg_key_basic(TG_Kind_U64);
    }
    else if(Case("s8") || Case("b8") || Case("B8") || Case("i8") || Case("int8") || Case("int8_t") || Case("char8") || Case("S8"))
    {
      key = tg_key_basic(TG_Kind_S8);
    }
    else if(Case("s16") ||Case("b16") || Case("B16") || Case("i16") ||  Case("int16") || Case("int16_t") || Case("char16") || Case("S16"))
    {
      key = tg_key_basic(TG_Kind_S16);
    }
    else if(Case("s32") || Case("b32") || Case("B32") || Case("i32") || Case("int32") || Case("int32_t") || Case("char32") || Case("S32") || Case("int"))
    {
      key = tg_key_basic(TG_Kind_S32);
    }
    else if(Case("s64") || Case("b64") || Case("B64") || Case("i64") || Case("int64") || Case("int64_t") || Case("S64"))
    {
      key = tg_key_basic(TG_Kind_S64);
    }
    else if(Case("void"))
    {
      key = tg_key_basic(TG_Kind_Void);
    }
    else if(Case("bool"))
    {
      key = tg_key_basic(TG_Kind_Bool);
    }
    else if(Case("float") || Case("f32") || Case("F32") || Case("r32") || Case("R32"))
    {
      key = tg_key_basic(TG_Kind_F32);
    }
    else if(Case("double") || Case("f64") || Case("F64") || Case("r64") || Case("R64"))
    {
      key = tg_key_basic(TG_Kind_F64);
    }
#undef Case
  }
  return key;
}

internal EVAL_ParseResult
eval_parse_type_from_text_tokens(Arena *arena, EVAL_ParseCtx *ctx, String8 text, EVAL_TokenArray *tokens)
{
  EVAL_ParseResult parse = eval_parse_result_nil;
  EVAL_Token *token_it = tokens->v;
  
  //- rjf: parse unsigned marker
  B32 unsigned_marker = 0;
  {
    EVAL_Token token = eval_token_at_it(token_it, tokens);
    if(token.kind == EVAL_TokenKind_Identifier)
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
    EVAL_Token token = eval_token_at_it(token_it, tokens);
    if(token.kind == EVAL_TokenKind_Identifier)
    {
      String8 token_string = str8_substr(text, token.range);
      TG_Key type_key = eval_leaf_type_from_name(ctx->rdbg, token_string);
      if(!tg_key_match(tg_key_zero(), type_key))
      {
        token_it += 1;
        
        // rjf: apply unsigned marker to base type
        if(unsigned_marker) switch(tg_kind_from_key(type_key))
        {
          default:{}break;
          case TG_Kind_Char8: {type_key = tg_key_basic(TG_Kind_UChar8);}break;
          case TG_Kind_Char16:{type_key = tg_key_basic(TG_Kind_UChar16);}break;
          case TG_Kind_Char32:{type_key = tg_key_basic(TG_Kind_UChar32);}break;
          case TG_Kind_S8:  {type_key = tg_key_basic(TG_Kind_U8);}break;
          case TG_Kind_S16: {type_key = tg_key_basic(TG_Kind_U16);}break;
          case TG_Kind_S32: {type_key = tg_key_basic(TG_Kind_U32);}break;
          case TG_Kind_S64: {type_key = tg_key_basic(TG_Kind_U64);}break;
          case TG_Kind_S128:{type_key = tg_key_basic(TG_Kind_U128);}break;
          case TG_Kind_S256:{type_key = tg_key_basic(TG_Kind_U256);}break;
          case TG_Kind_S512:{type_key = tg_key_basic(TG_Kind_U512);}break;
        }
        
        // rjf: construct leaf type
        parse.expr = eval_expr_leaf_type(arena, token_string.str, type_key);
      }
    }
  }
  
  //- rjf: parse extensions
  if(parse.expr != &eval_expr_nil)
  {
    for(;;)
    {
      EVAL_Token token = eval_token_at_it(token_it, tokens);
      if(token.kind != EVAL_TokenKind_Symbol)
      {
        break;
      }
      String8 token_string = str8_substr(text, token.range);
      if(str8_match(token_string, str8_lit("*"), 0))
      {
        token_it += 1;
        parse.expr = eval_expr(arena, EVAL_ExprKind_Ptr, token_string.str, parse.expr, 0, 0);
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

internal EVAL_ParseResult
eval_parse_expr_from_text_tokens__prec(Arena *arena, EVAL_ParseCtx *ctx, String8 text, EVAL_TokenArray *tokens, S64 max_precedence)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  EVAL_Token *it = tokens->v;
  EVAL_Token *it_opl = tokens->v + tokens->count;
  EVAL_ParseResult result = eval_parse_result_nil;
  
  //- rjf: parse prefix unaries
  typedef struct PrefixUnaryNode PrefixUnaryNode;
  struct PrefixUnaryNode
  {
    PrefixUnaryNode *next;
    EVAL_ExprKind kind;
    EVAL_Expr *cast_expr;
    void *location;
  };
  PrefixUnaryNode *first_prefix_unary = 0;
  PrefixUnaryNode *last_prefix_unary = 0;
  {
    for(;it < it_opl;)
    {
      EVAL_Token *start_it = it;
      EVAL_Token token = eval_token_at_it(it, tokens);
      String8 token_string = str8_substr(text, token.range);
      S64 prefix_unary_precedence = 0;
      EVAL_ExprKind prefix_unary_kind = 0;
      EVAL_Expr *cast_expr = &eval_expr_nil;
      void *location = 0;
      
      // rjf: try op table
      for(U64 idx = 0; idx < ArrayCount(eval_g_unary_prefix_op_table); idx += 1)
      {
        if(str8_match(token_string, eval_g_unary_prefix_op_table[idx].string, 0))
        {
          prefix_unary_precedence = eval_g_unary_prefix_op_table[idx].precedence;
          prefix_unary_kind = eval_g_unary_prefix_op_table[idx].kind;
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
        EVAL_Token some_type_identifier_maybe = eval_token_at_it(it+1, tokens);
        String8 some_type_identifier_maybe_string = str8_substr(text, some_type_identifier_maybe.range);
        if(some_type_identifier_maybe.kind == EVAL_TokenKind_Identifier)
        {
          TG_Key type_key = eval_leaf_type_from_name(ctx->rdbg, some_type_identifier_maybe_string);
          if(!tg_key_match(type_key, tg_key_zero()) || str8_match(some_type_identifier_maybe_string, str8_lit("unsigned"), 0))
          {
            // rjf: move past open paren
            it += 1;
            
            // rjf: parse type expr
            EVAL_TokenArray type_parse_tokens = eval_token_array_make_first_opl(it, it_opl);
            EVAL_ParseResult type_parse = eval_parse_type_from_text_tokens(arena, ctx, text, &type_parse_tokens);
            EVAL_Expr *type = type_parse.expr;
            eval_error_list_concat_in_place(&result.errors, &type_parse.errors);
            it = type_parse.last_token;
            location = token_string.str;
            
            // rjf: expect )
            EVAL_Token close_paren_maybe = eval_token_at_it(it, tokens);
            String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
            if(close_paren_maybe.kind != EVAL_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit(")"), 0))
            {
              eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, token_string.str, "Missing ).");
            }
            
            // rjf: consume )
            else
            {
              it += 1;
            }
            
            // rjf: fill
            prefix_unary_precedence = 2;
            prefix_unary_kind = EVAL_ExprKind_Cast;
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
        PrefixUnaryNode *op_n = push_array(scratch.arena, PrefixUnaryNode, 1);
        op_n->kind = prefix_unary_kind;
        op_n->cast_expr = cast_expr;
        op_n->location = location;
        SLLQueuePushFront(first_prefix_unary, last_prefix_unary, op_n);
      }
    }
  }
  
  //- rjf: parse atom
  EVAL_Expr *atom = &eval_expr_nil;
  String8 atom_implicit_member_name = {0};
  if(it < it_opl)
  {
    EVAL_Token token = eval_token_at_it(it, tokens);
    String8 token_string = str8_substr(text, token.range);
    
    //- rjf: descent to nested expression
    if(token.kind == EVAL_TokenKind_Symbol && str8_match(token_string, str8_lit("("), 0))
    {
      // rjf: skip (
      it += 1;
      
      // rjf: parse () contents
      EVAL_TokenArray nested_parse_tokens = eval_token_array_make_first_opl(it, it_opl);
      EVAL_ParseResult nested_parse = eval_parse_expr_from_text_tokens__prec(arena, ctx, text, &nested_parse_tokens, eval_g_max_precedence);
      eval_error_list_concat_in_place(&result.errors, &nested_parse.errors);
      atom = nested_parse.expr;
      it = nested_parse.last_token;
      
      // rjf: expect )
      EVAL_Token close_paren_maybe = eval_token_at_it(it, tokens);
      String8 close_paren_maybe_string = str8_substr(text, close_paren_maybe.range);
      if(close_paren_maybe.kind != EVAL_TokenKind_Symbol || !str8_match(close_paren_maybe_string, str8_lit(")"), 0))
      {
        eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, token_string.str, "Missing ).");
      }
      
      // rjf: consume )
      else
      {
        it += 1;
      }
    }
    
    //- rjf: leaf (identifier, literal)
    else if(token.kind != EVAL_TokenKind_Symbol)
    {
      switch(token.kind)
      {
        //- rjf: identifier => name resolution
        default:
        case EVAL_TokenKind_Identifier:
        {
          B32 mapped_identifier = 0;
          B32 identifier_type_is_possibly_dynamically_overridden = 0;
          B32 identifier_looks_like_type_expr = 0;
          RADDBG_LocationKind            loc_kind = RADDBG_LocationKind_NULL;
          RADDBG_LocationRegister        loc_reg = {0};
          RADDBG_LocationRegisterPlusU16 loc_reg_u16 = {0};
          String8                        loc_bytecode = {0};
          REGS_RegCode                   reg_code = 0;
          REGS_AliasCode                 alias_code = 0;
          TG_Key                         type_key = zero_struct;
          String8                        local_lookup_string = token_string;
          
          //- rjf: form namespaceified fallback versions of this lookup string
          String8List namespaceified_token_strings = {0};
          if(ctx->rdbg->procedures != 0 && ctx->rdbg->scopes != 0 && ctx->rdbg->scope_vmap != 0)
          {
            U64 scope_idx = raddbg_vmap_idx_from_voff(ctx->rdbg->scope_vmap, ctx->rdbg->scope_vmap_count, ctx->ip_voff);
            RADDBG_Scope *scope = &ctx->rdbg->scopes[scope_idx];
            U64 proc_idx = scope->proc_idx;
            RADDBG_Procedure *procedure = &ctx->rdbg->procedures[proc_idx];
            U64 name_size = 0;
            U8 *name_ptr = raddbg_string_from_idx(ctx->rdbg, procedure->name_string_idx, &name_size);
            String8 containing_procedure_name = str8(name_ptr, name_size);
            U64 last_past_scope_resolution_pos = 0;
            for(;;)
            {
              U64 past_next_dbl_colon_pos = str8_find_needle(containing_procedure_name, last_past_scope_resolution_pos, str8_lit("::"), 0)+2;
              U64 past_next_dot_pos = str8_find_needle(containing_procedure_name, last_past_scope_resolution_pos, str8_lit("."), 0)+1;
              U64 past_next_scope_resolution_pos = Min(past_next_dbl_colon_pos, past_next_dot_pos);
              if(past_next_scope_resolution_pos >= containing_procedure_name.size)
              {
                break;
              }
              String8 new_namespace_prefix_possibility = str8_prefix(containing_procedure_name, past_next_scope_resolution_pos);
              String8 namespaceified_token_string = push_str8f(scratch.arena, "%S%S", new_namespace_prefix_possibility, token_string);
              str8_list_push_front(scratch.arena, &namespaceified_token_strings, namespaceified_token_string);
              last_past_scope_resolution_pos = past_next_scope_resolution_pos;
            }
          }
          
          //- rjf: try members
          if(mapped_identifier == 0)
          {
            U64 data_member_num = eval_num_from_string(ctx->member_map, token_string);
            if(data_member_num != 0)
            {
              atom_implicit_member_name = token_string;
              local_lookup_string = str8_lit("this");
            }
          }
          
          //- rjf: try locals
          if(mapped_identifier == 0)
          {
            U64 local_num = eval_num_from_string(ctx->locals_map, local_lookup_string);
            if(local_num != 0)
            {
              mapped_identifier = 1;
              identifier_type_is_possibly_dynamically_overridden = 1;
              RADDBG_Local *local_var = raddbg_element_from_idx(ctx->rdbg, locals, local_num-1);
              RADDBG_TypeNode *type_node = raddbg_element_from_idx(ctx->rdbg, type_nodes, local_var->type_idx);
              type_key = tg_key_ext(tg_kind_from_raddbg_type_kind(type_node->kind), (U64)local_var->type_idx);
              
              // rjf: grab location info
              for(U32 loc_block_idx = local_var->location_first;
                  loc_block_idx < local_var->location_opl;
                  loc_block_idx += 1)
              {
                RADDBG_LocationBlock *block = &ctx->rdbg->location_blocks[loc_block_idx];
                if(block->scope_off_first <= ctx->ip_voff && ctx->ip_voff < block->scope_off_opl)
                {
                  loc_kind = *((RADDBG_LocationKind *)(ctx->rdbg->location_data + block->location_data_off));
                  switch(loc_kind)
                  {
                    default:{mapped_identifier = 0;}break;
                    case RADDBG_LocationKind_AddrBytecodeStream:
                    case RADDBG_LocationKind_ValBytecodeStream:
                    {
                      U8 *bytecode_base = ctx->rdbg->location_data + block->location_data_off + sizeof(RADDBG_LocationKind);
                      U64 bytecode_size = 0;
                      for(U64 idx = 0; idx < ctx->rdbg->location_data_size; idx += 1)
                      {
                        U8 op = bytecode_base[idx];
                        if(op == 0)
                        {
                          break;
                        }
                        U8 ctrlbits = raddbg_eval_opcode_ctrlbits[op];
                        U32 p_size = RADDBG_DECODEN_FROM_CTRLBITS(ctrlbits);
                        bytecode_size += 1+p_size;
                      }
                      loc_bytecode = str8(bytecode_base, bytecode_size);
                    }break;
                    case RADDBG_LocationKind_AddrRegisterPlusU16:
                    case RADDBG_LocationKind_AddrAddrRegisterPlusU16:
                    {
                      MemoryCopy(&loc_reg_u16, (ctx->rdbg->location_data + block->location_data_off), sizeof(loc_reg_u16));
                    }break;
                    case RADDBG_LocationKind_ValRegister:
                    {
                      MemoryCopy(&loc_reg, (ctx->rdbg->location_data + block->location_data_off), sizeof(loc_reg));
                    }break;
                  }
                }
              }
            }
          }
          
          //- rjf: try registers
          if(mapped_identifier == 0)
          {
            U64 reg_num = eval_num_from_string(ctx->regs_map, token_string);
            if(reg_num != 0)
            {
              reg_code = reg_num;
              mapped_identifier = 1;
              type_key = tg_key_reg(ctx->arch, reg_code);
            }
          }
          
          //- rjf: try register aliases
          if(mapped_identifier == 0)
          {
            U64 alias_num = eval_num_from_string(ctx->reg_alias_map, token_string);
            if(alias_num != 0)
            {
              alias_code = (REGS_AliasCode)alias_num;
              type_key = tg_key_reg_alias(ctx->arch, alias_code);
              mapped_identifier = 1;
            }
          }
          
          //- rjf: try global variables
          if(mapped_identifier == 0)
          {
            RADDBG_NameMap *name_map = raddbg_name_map_from_kind(ctx->rdbg, RADDBG_NameMapKind_GlobalVariables);
            if(name_map != 0 && ctx->rdbg->global_variables != 0)
            {
              RADDBG_ParsedNameMap parsed_name_map = {0};
              raddbg_name_map_parse(ctx->rdbg, name_map, &parsed_name_map);
              RADDBG_NameMapNode *node = raddbg_name_map_lookup(ctx->rdbg, &parsed_name_map, token_string.str, token_string.size);
              U32 matches_count = 0;
              U32 *matches = raddbg_matches_from_map_node(ctx->rdbg, node, &matches_count);
              for(String8Node *n = namespaceified_token_strings.first;
                  n != 0 && matches_count == 0;
                  n = n->next)
              {
                node = raddbg_name_map_lookup(ctx->rdbg, &parsed_name_map, n->string.str, n->string.size);
                matches_count = 0;
                matches = raddbg_matches_from_map_node(ctx->rdbg, node, &matches_count);
              }
              if(matches_count != 0)
              {
                // NOTE(rjf): apparently, PDBs can be produced such that they
                // also keep stale *GLOBAL VARIABLE SYMBOLS* around too. I
                // don't know of a magic hash table fixup path in PDBs, so
                // in this case, I'm going to prefer the latest-added global.
                U32 match_idx = matches[matches_count-1];
                RADDBG_GlobalVariable *global_var = &ctx->rdbg->global_variables[match_idx];
                EVAL_OpList oplist = {0};
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_ModuleOff, global_var->voff);
                loc_kind = RADDBG_LocationKind_AddrBytecodeStream;
                loc_bytecode = eval_bytecode_from_oplist(arena, &oplist);
                U32 type_idx = global_var->type_idx;
                if(type_idx < ctx->rdbg->type_nodes_count)
                {
                  RADDBG_TypeNode *type_node = &ctx->rdbg->type_nodes[type_idx];
                  type_key = tg_key_ext(tg_kind_from_raddbg_type_kind(type_node->kind), (U64)type_idx);
                }
                mapped_identifier = 1;
              }
            }
          }
          
          //- rjf: try thread variables
          if(mapped_identifier == 0)
          {
            RADDBG_NameMap *name_map = raddbg_name_map_from_kind(ctx->rdbg, RADDBG_NameMapKind_ThreadVariables);
            if(name_map != 0 && ctx->rdbg->global_variables != 0)
            {
              RADDBG_ParsedNameMap parsed_name_map = {0};
              raddbg_name_map_parse(ctx->rdbg, name_map, &parsed_name_map);
              RADDBG_NameMapNode *node = raddbg_name_map_lookup(ctx->rdbg, &parsed_name_map, token_string.str, token_string.size);
              U32 matches_count = 0;
              U32 *matches = raddbg_matches_from_map_node(ctx->rdbg, node, &matches_count);
              for(String8Node *n = namespaceified_token_strings.first;
                  n != 0 && matches_count == 0;
                  n = n->next)
              {
                node = raddbg_name_map_lookup(ctx->rdbg, &parsed_name_map, n->string.str, n->string.size);
                matches_count = 0;
                matches = raddbg_matches_from_map_node(ctx->rdbg, node, &matches_count);
              }
              if(matches_count != 0)
              {
                U32 match_idx = matches[0];
                RADDBG_ThreadVariable *thread_var = &ctx->rdbg->thread_variables[match_idx];
                EVAL_OpList oplist = {0};
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_TLSOff, thread_var->tls_off);
                loc_kind = RADDBG_LocationKind_AddrBytecodeStream;
                loc_bytecode = eval_bytecode_from_oplist(arena, &oplist);
                U32 type_idx = thread_var->type_idx;
                if(type_idx < ctx->rdbg->type_nodes_count)
                {
                  RADDBG_TypeNode *type_node = &ctx->rdbg->type_nodes[type_idx];
                  type_key = tg_key_ext(tg_kind_from_raddbg_type_kind(type_node->kind), (U64)type_idx);
                }
                mapped_identifier = 1;
              }
            }
          }
          
          //- rjf: try procedures
          if(mapped_identifier == 0)
          {
            RADDBG_NameMap *name_map = raddbg_name_map_from_kind(ctx->rdbg, RADDBG_NameMapKind_Procedures);
            if(name_map != 0 && ctx->rdbg->procedures != 0 && ctx->rdbg->scopes != 0 && ctx->rdbg->scope_voffs)
            {
              RADDBG_ParsedNameMap parsed_name_map = {0};
              raddbg_name_map_parse(ctx->rdbg, name_map, &parsed_name_map);
              RADDBG_NameMapNode *node = raddbg_name_map_lookup(ctx->rdbg, &parsed_name_map, token_string.str, token_string.size);
              U32 matches_count = 0;
              U32 *matches = raddbg_matches_from_map_node(ctx->rdbg, node, &matches_count);
              for(String8Node *n = namespaceified_token_strings.first;
                  n != 0 && matches_count == 0;
                  n = n->next)
              {
                node = raddbg_name_map_lookup(ctx->rdbg, &parsed_name_map, n->string.str, n->string.size);
                matches_count = 0;
                matches = raddbg_matches_from_map_node(ctx->rdbg, node, &matches_count);
              }
              if(matches_count != 0)
              {
                U32 match_idx = matches[0];
                RADDBG_Procedure *procedure = &ctx->rdbg->procedures[match_idx];
                RADDBG_Scope *scope = &ctx->rdbg->scopes[procedure->root_scope_idx];
                U64 voff = ctx->rdbg->scope_voffs[scope->voff_range_first];
                EVAL_OpList oplist = {0};
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_ModuleOff, voff);
                loc_kind = RADDBG_LocationKind_ValBytecodeStream;
                loc_bytecode = eval_bytecode_from_oplist(arena, &oplist);
                U32 type_idx = procedure->type_idx;
                if(type_idx < ctx->rdbg->type_nodes_count)
                {
                  RADDBG_TypeNode *type_node = &ctx->rdbg->type_nodes[type_idx];
                  type_key = tg_key_ext(tg_kind_from_raddbg_type_kind(type_node->kind), (U64)type_idx);
                }
                mapped_identifier = 1;
              }
            }
          }
          
          //- rjf: try types
          if(mapped_identifier == 0)
          {
            type_key = eval_leaf_type_from_name(ctx->rdbg, token_string);
            if(!tg_key_match(tg_key_zero(), type_key))
            {
              mapped_identifier = 1;
              identifier_looks_like_type_expr = 1;
            }
          }
          
          //- rjf: attach on map
          if(mapped_identifier != 0)
          {
            it += 1;
            
            // rjf: build atom
            switch(loc_kind)
            {
              default:
              {
                if(identifier_looks_like_type_expr)
                {
                  EVAL_TokenArray type_parse_tokens = eval_token_array_make_first_opl(it-1, it_opl);
                  EVAL_ParseResult type_parse = eval_parse_type_from_text_tokens(arena, ctx, text, &type_parse_tokens);
                  EVAL_Expr *type = type_parse.expr;
                  eval_error_list_concat_in_place(&result.errors, &type_parse.errors);
                  it = type_parse.last_token;
                  atom = type;
                }
                else if(reg_code != 0)
                {
                  REGS_Rng reg_rng = regs_reg_code_rng_table_from_architecture(ctx->arch)[reg_code];
                  EVAL_OpList oplist = {0};
                  eval_oplist_push_uconst(arena, &oplist, reg_rng.byte_off);
                  atom = eval_expr_leaf_op_list(arena, token_string.str, type_key, &oplist, EVAL_EvalMode_Reg);
                }
                else if(alias_code != 0)
                {
                  REGS_Slice alias_slice = regs_alias_code_slice_table_from_architecture(ctx->arch)[alias_code];
                  REGS_Rng alias_reg_rng = regs_reg_code_rng_table_from_architecture(ctx->arch)[alias_slice.code];
                  EVAL_OpList oplist = {0};
                  eval_oplist_push_uconst(arena, &oplist, alias_reg_rng.byte_off + alias_slice.byte_off);
                  atom = eval_expr_leaf_op_list(arena, token_string.str, type_key, &oplist, EVAL_EvalMode_Reg);
                }
                else
                {
                  eval_errorf(arena, &result.errors, EVAL_ErrorKind_MissingInfo, token_string.str, "Missing location information for \"%S\".", token_string);
                }
              }break;
              case RADDBG_LocationKind_AddrBytecodeStream:
              {
                atom = eval_expr_leaf_bytecode(arena, token_string.str, type_key, loc_bytecode, EVAL_EvalMode_Addr);
              }break;
              case RADDBG_LocationKind_ValBytecodeStream:
              {
                atom = eval_expr_leaf_bytecode(arena, token_string.str, type_key, loc_bytecode, EVAL_EvalMode_Value);
              }break;
              case RADDBG_LocationKind_AddrRegisterPlusU16:
              {
                EVAL_OpList oplist = {0};
                U64 byte_size = bit_size_from_arch(ctx->arch)/8;
                U64 regread_param = RADDBG_EncodeRegReadParam(loc_reg_u16.register_code, byte_size, 0);
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_RegRead, regread_param);
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_ConstU16, loc_reg_u16.offset);
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_Add, 0);
                atom = eval_expr_leaf_op_list(arena, token_string.str, type_key, &oplist, EVAL_EvalMode_Addr);
              }break;
              case RADDBG_LocationKind_AddrAddrRegisterPlusU16:
              {
                EVAL_OpList oplist = {0};
                U64 byte_size = bit_size_from_arch(ctx->arch)/8;
                U64 regread_param = RADDBG_EncodeRegReadParam(loc_reg_u16.register_code, byte_size, 0);
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_RegRead, regread_param);
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_ConstU16, loc_reg_u16.offset);
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_Add, 0);
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_MemRead, bit_size_from_arch(ctx->arch)/8);
                atom = eval_expr_leaf_op_list(arena, token_string.str, type_key, &oplist, EVAL_EvalMode_Addr);
              }break;
              case RADDBG_LocationKind_ValRegister:
              {
                REGS_RegCode regs_reg_code = regs_reg_code_from_arch_raddbg_code(ctx->arch, loc_reg.register_code);
                REGS_Rng reg_rng = regs_reg_code_rng_table_from_architecture(ctx->arch)[regs_reg_code];
                EVAL_OpList oplist = {0};
                U64 byte_size = (U64)reg_rng.byte_size;
                U64 byte_pos = 0;
                U64 regread_param = RADDBG_EncodeRegReadParam(loc_reg.register_code, byte_size, byte_pos);
                eval_oplist_push_op(arena, &oplist, RADDBG_EvalOp_RegRead, regread_param);
                atom = eval_expr_leaf_op_list(arena, token_string.str, type_key, &oplist, EVAL_EvalMode_Value);
              }break;
            }
            
            // rjf: implicit local lookup -> attach member access node
            if(atom_implicit_member_name.size != 0)
            {
              EVAL_Expr *member_expr = eval_expr_leaf_member(arena, atom_implicit_member_name.str, atom_implicit_member_name);
              atom = eval_expr(arena, EVAL_ExprKind_MemberAccess, atom_implicit_member_name.str, atom, member_expr, 0);
            }
          }
          
          // rjf: error on map failure
          if(mapped_identifier == 0)
          {
            eval_errorf(arena, &result.errors, EVAL_ErrorKind_ResolutionFailure, token_string.str, "Unknown identifier \"%S\".", token_string);
            it += 1;
          }
        }break;
        
        //- rjf: numeric => directly extract value
        case EVAL_TokenKind_Numeric:
        {
          U64 dot_pos = str8_find_needle(token_string, 0, str8_lit("."), 0);
          it += 1;
          
          // rjf: no . => integral
          if(dot_pos == token_string.size)
          {
            U64 val = 0;
            try_u64_from_str8_c_rules(token_string, &val);
            atom = eval_expr_u64(arena, token_string.str, val);
            break;
          }
          
          // rjf: presence of . => double or float
          if(dot_pos < token_string.size)
          {
            F64 val = f64_from_str8(token_string);
            U64 f_pos = str8_find_needle(token_string, 0, str8_lit("f"), StringMatchFlag_CaseInsensitive);
            
            // rjf: presence of f after . => f32
            if(f_pos < token_string.size)
            {
              atom = eval_expr_f32(arena, token_string.str, (F32)val);
            }
            
            // rjf: no f => f64
            else
            {
              atom = eval_expr_f64(arena, token_string.str, val);
            }
          }
        }break;
        
        //- rjf: char => extract char value
        case EVAL_TokenKind_CharLiteral:
        {
          it += 1;
          if(token_string.size > 1 && token_string.str[0] == '\'' && token_string.str[1] != '\'')
          {
            U8 char_val = token_string.str[1];
            atom = eval_expr_u64(arena, token_string.str, char_val);
          }
          else
          {
            eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, token_string.str, "Malformed character literal.");
          }
        }break;
        
        // rjf: string => invalid
        case EVAL_TokenKind_StringLiteral:
        {
          eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, token_string.str, "String literals are not supported.");
          it += 1;
        }break;
        
      }
    }
  }
  
  //- rjf: upgrade atom w/ postfix unaries
  if(atom != &eval_expr_nil) for(;it < it_opl;)
  {
    EVAL_Token token = eval_token_at_it(it, tokens);
    String8 token_string = str8_substr(text, token.range);
    B32 is_postfix_unary = 0;
    
    // rjf: dot/arrow operator
    if(token.kind == EVAL_TokenKind_Symbol &&
       (str8_match(token_string, str8_lit("."), 0) ||
        str8_match(token_string, str8_lit("->"), 0)))
    {
      is_postfix_unary = 1;
      
      // rjf: advance past operator
      it += 1;
      
      // rjf: expect member name
      String8 member_name = {0};
      B32 good_member_name = 0;
      {
        EVAL_Token member_name_maybe = eval_token_at_it(it, tokens);
        String8 member_name_maybe_string = str8_substr(text, member_name_maybe.range);
        if(member_name_maybe.kind != EVAL_TokenKind_Identifier)
        {
          eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, token_string.str, "Expected member name after %S.", token_string);
        }
        else
        {
          member_name = member_name_maybe_string;
          good_member_name = 1;
        }
      }
      
      // rjf: produce lookup member expr
      if(good_member_name)
      {
        EVAL_Expr *member_expr = eval_expr_leaf_member(arena, member_name.str, member_name);
        atom = eval_expr(arena, EVAL_ExprKind_MemberAccess, token_string.str, atom, member_expr, 0);
      }
      
      // rjf: increment past good member names
      if(good_member_name)
      {
        it += 1;
      }
    }
    
    // rjf: array index
    if(token.kind == EVAL_TokenKind_Symbol &&
       str8_match(token_string, str8_lit("["), 0))
    {
      is_postfix_unary = 1;
      
      // rjf: advance past [
      it += 1;
      
      // rjf: parse indexing expression
      EVAL_TokenArray idx_expr_parse_tokens = eval_token_array_make_first_opl(it, it_opl);
      EVAL_ParseResult idx_expr_parse = eval_parse_expr_from_text_tokens__prec(arena, ctx, text, &idx_expr_parse_tokens, eval_g_max_precedence);
      eval_error_list_concat_in_place(&result.errors, &idx_expr_parse.errors);
      it = idx_expr_parse.last_token;
      
      // rjf: valid indexing expression => produce index expr
      if(idx_expr_parse.expr != &eval_expr_nil)
      {
        atom = eval_expr(arena, EVAL_ExprKind_ArrayIndex, token_string.str, atom, idx_expr_parse.expr, 0);
      }
      
      // rjf: expect ]
      {
        EVAL_Token close_brace_maybe = eval_token_at_it(it, tokens);
        String8 close_brace_maybe_string = str8_substr(text, close_brace_maybe.range);
        if(close_brace_maybe.kind != EVAL_TokenKind_Symbol || !str8_match(close_brace_maybe_string, str8_lit("]"), 0))
        {
          eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, token_string.str, "Unclosed [.");
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
  
  //- rjf: upgrade atom w/ previously parsed prefix unaries
  if(atom == &eval_expr_nil && first_prefix_unary != 0 && first_prefix_unary->cast_expr != 0)
  {
    atom = first_prefix_unary->cast_expr;
    for(PrefixUnaryNode *prefix_unary = first_prefix_unary->next;
        prefix_unary != 0;
        prefix_unary = prefix_unary->next)
    {
      atom = eval_expr(arena, prefix_unary->kind, prefix_unary->location,
                       prefix_unary->cast_expr != &eval_expr_nil ? prefix_unary->cast_expr : atom,
                       prefix_unary->cast_expr != &eval_expr_nil ? atom : 0, 0);
    }
  }
  else if(atom == 0 && first_prefix_unary != 0)
  {
    eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, last_prefix_unary->location, "Missing expression.");
  }
  else
  {
    for(PrefixUnaryNode *prefix_unary = first_prefix_unary; prefix_unary != 0; prefix_unary = prefix_unary->next)
    {
      atom = eval_expr(arena, prefix_unary->kind, prefix_unary->location,
                       prefix_unary->cast_expr != &eval_expr_nil ? prefix_unary->cast_expr : atom,
                       prefix_unary->cast_expr != &eval_expr_nil ? atom : 0, 0);
    }
  }
  
  //- rjf: parse complex operators
  if(atom != &eval_expr_nil) for(;it < it_opl;)
  {
    EVAL_Token *start_it = it;
    EVAL_Token token = eval_token_at_it(it, tokens);
    String8 token_string = str8_substr(text, token.range);
    
    //- rjf: parse binaries
    {
      // rjf: first try to find a matching binary operator
      S64 binary_precedence = 0;
      EVAL_ExprKind binary_kind = 0;
      for(U64 idx = 0; idx < ArrayCount(eval_g_binary_op_table); idx += 1)
      {
        if(str8_match(token_string, eval_g_binary_op_table[idx].string, 0))
        {
          binary_precedence = eval_g_binary_op_table[idx].precedence;
          binary_kind = eval_g_binary_op_table[idx].kind;
          break;
        }
      }
      
      // rjf: if we got a valid binary precedence, and it's not to be handled by
      // a caller, then we need to parse the right-hand-side with a tighter
      // precedence
      if(binary_precedence != 0 && binary_precedence <= max_precedence)
      {
        EVAL_TokenArray rhs_expr_parse_tokens = eval_token_array_make_first_opl(it+1, it_opl);
        EVAL_ParseResult rhs_expr_parse = eval_parse_expr_from_text_tokens__prec(arena, ctx, text, &rhs_expr_parse_tokens, binary_precedence-1);
        eval_error_list_concat_in_place(&result.errors, &rhs_expr_parse.errors);
        EVAL_Expr *rhs = rhs_expr_parse.expr;
        it = rhs_expr_parse.last_token;
        if(rhs == &eval_expr_nil)
        {
          eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, token_string.str, "Missing right-hand-side of %S.", token_string);
        }
        else
        {
          atom = eval_expr(arena, binary_kind, token_string.str, atom, rhs, 0);
        }
      }
    }
    
    //- rjf: parse ternaries
    {
      if(token.kind == EVAL_TokenKind_Symbol && str8_match(token_string, str8_lit("?"), 0) && 13 <= max_precedence)
      {
        // rjf: parse middle expression
        EVAL_TokenArray middle_expr_tokens = eval_token_array_make_first_opl(it, it_opl);
        EVAL_ParseResult middle_expr_parse = eval_parse_expr_from_text_tokens__prec(arena, ctx, text, &middle_expr_tokens, eval_g_max_precedence);
        it = middle_expr_parse.last_token;
        EVAL_Expr *middle_expr = middle_expr_parse.expr;
        eval_error_list_concat_in_place(&result.errors, &middle_expr_parse.errors);
        if(middle_expr_parse.expr == &eval_expr_nil)
        {
          eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, token_string.str, "Expected expression after ?.");
        }
        
        // rjf: expect :
        B32 got_colon = 0;
        EVAL_Token colon_token = zero_struct;
        String8 colon_token_string = {0};
        {
          EVAL_Token colon_token_maybe = eval_token_at_it(it, tokens);
          String8 colon_token_maybe_string = str8_substr(text, colon_token_maybe.range);
          if(colon_token_maybe.kind != EVAL_TokenKind_Symbol || !str8_match(colon_token_maybe_string, str8_lit(":"), 0))
          {
            eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, token_string.str, "Expected : after ?.");
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
        EVAL_TokenArray rhs_expr_parse_tokens = eval_token_array_make_first_opl(it, it_opl);
        EVAL_ParseResult rhs_expr_parse = eval_parse_expr_from_text_tokens__prec(arena, ctx, text, &rhs_expr_parse_tokens, eval_g_max_precedence);
        if(got_colon)
        {
          it = rhs_expr_parse.last_token;
          eval_error_list_concat_in_place(&result.errors, &rhs_expr_parse.errors);
          if(rhs_expr_parse.expr == &eval_expr_nil)
          {
            eval_errorf(arena, &result.errors, EVAL_ErrorKind_MalformedInput, colon_token_string.str, "Expected expression after :.");
          }
        }
        
        // rjf: build ternary
        atom = eval_expr(arena, EVAL_ExprKind_Ternary, token_string.str, atom, middle_expr_parse.expr, rhs_expr_parse.expr);
      }
    }
    
    // rjf: if we parsed nothing successfully, we're done
    if(it == start_it)
    {
      break;
    }
  }
  
  //- rjf: fill result & return
  result.last_token = it;
  result.expr = atom;
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal EVAL_ParseResult
eval_parse_expr_from_text_tokens(Arena *arena, EVAL_ParseCtx *ctx, String8 text, EVAL_TokenArray *tokens)
{
  EVAL_ParseResult result = eval_parse_expr_from_text_tokens__prec(arena, ctx, text, tokens, eval_g_max_precedence);
  return result;
}
