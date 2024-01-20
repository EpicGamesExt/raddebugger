// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
txti_init(void)
{
  Arena *arena = arena_alloc();
  txti_state = push_array(arena, TXTI_State, 1);
  txti_state->arena = arena;
  txti_state->entity_map.slots_count = 1024;
  txti_state->entity_map.slots = push_array(txti_state->arena, TXTI_EntitySlot, txti_state->entity_map.slots_count);
  txti_state->entity_map_stripes.count = 64;
  txti_state->entity_map_stripes.v = push_array(txti_state->arena, TXTI_Stripe, txti_state->entity_map_stripes.count);
  for(U64 idx = 0; idx < txti_state->entity_map_stripes.count; idx += 1)
  {
    txti_state->entity_map_stripes.v[idx].arena = arena_alloc();
    txti_state->entity_map_stripes.v[idx].cv = os_condition_variable_alloc();
    txti_state->entity_map_stripes.v[idx].rw_mutex = os_rw_mutex_alloc();
  }
  txti_state->mut_thread_count = Min(4, os_logical_core_count());
  txti_state->mut_threads = push_array(txti_state->arena, TXTI_MutThread, txti_state->mut_thread_count);
  for(U64 idx = 0; idx < txti_state->mut_thread_count; idx += 1)
  {
    TXTI_MutThread *thread = &txti_state->mut_threads[idx];
    thread->msg_arena = arena_alloc();
    thread->msg_mutex = os_mutex_alloc();
    thread->msg_cv = os_condition_variable_alloc();
    thread->thread = os_launch_thread(txti_mut_thread_entry_point, (void *)idx, 0);
  }
  txti_state->detector_thread = os_launch_thread(txti_detector_thread_entry_point, 0, 0);
}

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
txti_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal TXTI_LangKind
txti_lang_kind_from_extension(String8 extension)
{
  TXTI_LangKind kind = TXTI_LangKind_Null;
  if(str8_match(extension, str8_lit("c"), 0) ||
     str8_match(extension, str8_lit("h"), 0))
  {
    kind = TXTI_LangKind_C;
  }
  else if(str8_match(extension, str8_lit("cpp"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("cxx"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("cc"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("c++"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("C"), 0) ||
          str8_match(extension, str8_lit("hpp"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("hxx"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("hh"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("h++"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("H"), 0))
  {
    kind = TXTI_LangKind_CPlusPlus;
  }
  else if(str8_match(extension, str8_lit("odin"), StringMatchFlag_CaseInsensitive)) {
    kind = TXTI_LangKind_Odin;
  }
  return kind;
}

////////////////////////////////
//~ rjf: Token Type Functions

internal void
txti_token_chunk_list_push(Arena *arena, TXTI_TokenChunkList *list, U64 cap, TXTI_Token *token)
{
  TXTI_TokenChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, TXTI_TokenChunkNode, 1);
    SLLQueuePush(list->first, list->last, node);
    node->cap = cap;
    node->v = push_array_no_zero(arena, TXTI_Token, node->cap);
    list->chunk_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], token);
  node->count += 1;
  list->token_count += 1;
}

internal void
txti_token_list_push(Arena *arena, TXTI_TokenList *list, TXTI_Token *token)
{
  TXTI_TokenNode *node = push_array(arena, TXTI_TokenNode, 1);
  MemoryCopyStruct(&node->v, token);
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal TXTI_TokenArray
txti_token_array_from_chunk_list(Arena *arena, TXTI_TokenChunkList *list)
{
  TXTI_TokenArray array = {0};
  array.count = list->token_count;
  array.v = push_array_no_zero(arena, TXTI_Token, array.count);
  U64 idx = 0;
  for(TXTI_TokenChunkNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopy(array.v+idx, n->v, n->count*sizeof(TXTI_Token));
    idx += n->count;
  }
  return array;
}

internal TXTI_TokenArray
txti_token_array_from_list(Arena *arena, TXTI_TokenList *list)
{
  TXTI_TokenArray array = {0};
  array.count = list->count;
  array.v = push_array_no_zero(arena, TXTI_Token, array.count);
  U64 idx = 0;
  for(TXTI_TokenNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopyStruct(array.v+idx, &n->v);
    idx += 1;
  }
  return array;
}

////////////////////////////////
//~ rjf: Lexing Functions

internal TXTI_TokenArray
txti_token_array_from_string__cpp(Arena *arena, U64 *bytes_processed_counter, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: generate token list
  TXTI_TokenChunkList tokens = {0};
  {
    B32 comment_is_single_line = 0;
    B32 string_is_char = 0;
    TXTI_TokenKind active_token_kind = TXTI_TokenKind_Null;
    U64 active_token_start_idx = 0;
    B32 escaped = 0;
    B32 next_escaped = 0;
    U64 byte_process_start_idx = 0;
    for(U64 idx = 0; idx <= string.size;)
    {
      U8 byte      = (idx+0 < string.size) ? (string.str[idx+0]) : 0;
      U8 next_byte = (idx+1 < string.size) ? (string.str[idx+1]) : 0;
      
      // rjf: update counter
      if(bytes_processed_counter != 0 && ((idx-byte_process_start_idx) >= 1000 || idx == string.size))
      {
        ins_atomic_u64_add_eval(bytes_processed_counter, (idx-byte_process_start_idx));
        byte_process_start_idx = idx;
      }
      
      // rjf: escaping
      if(escaped && (byte != '\r' && byte != '\n'))
      {
        next_escaped = 0;
      }
      else if(!escaped && byte == '\\')
      {
        next_escaped = 1;
      }
      
      // rjf: take starter, determine active token kind
      if(active_token_kind == TXTI_TokenKind_Null)
      {
        // rjf: use next bytes to start a new token
        if(0){}
        else if(char_is_space(byte))             { active_token_kind = TXTI_TokenKind_Whitespace; }
        else if(byte == '_' ||
                byte == '$' ||
                char_is_alpha(byte))             { active_token_kind = TXTI_TokenKind_Identifier; }
        else if(char_is_digit(byte, 10) ||
                (byte == '.' &&
                 char_is_digit(next_byte, 10)))  { active_token_kind = TXTI_TokenKind_Numeric; }
        else if(byte == '"')                     { active_token_kind = TXTI_TokenKind_String; string_is_char = 0; }
        else if(byte == '\'')                    { active_token_kind = TXTI_TokenKind_String; string_is_char = 1; }
        else if(byte == '/' && next_byte == '/') { active_token_kind = TXTI_TokenKind_Comment; comment_is_single_line = 1; }
        else if(byte == '/' && next_byte == '*') { active_token_kind = TXTI_TokenKind_Comment; comment_is_single_line = 0; }
        else if(byte == '~' || byte == '!' ||
                byte == '%' || byte == '^' ||
                byte == '&' || byte == '*' ||
                byte == '(' || byte == ')' ||
                byte == '-' || byte == '=' ||
                byte == '+' || byte == '[' ||
                byte == ']' || byte == '{' ||
                byte == '}' || byte == ':' ||
                byte == ';' || byte == ',' ||
                byte == '.' || byte == '<' ||
                byte == '>' || byte == '/' ||
                byte == '?' || byte == '|')      { active_token_kind = TXTI_TokenKind_Symbol; }
        else if(byte == '#')                     { active_token_kind = TXTI_TokenKind_Meta; }
        
        // rjf: start new token
        if(active_token_kind != TXTI_TokenKind_Null)
        {
          active_token_start_idx = idx;
        }
        
        // rjf: invalid token kind -> emit error
        else
        {
          TXTI_Token token = {TXTI_TokenKind_Error, r1u64(idx, idx+1)};
          txti_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
      }
      
      // rjf: look for ender
      U64 ender_pad = 0;
      B32 ender_found = 0;
      if(active_token_kind != TXTI_TokenKind_Null && idx>active_token_start_idx)
      {
        if(idx == string.size)
        {
          ender_pad = 0;
          ender_found = 1;
        }
        else switch(active_token_kind)
        {
          default:break;
          case TXTI_TokenKind_Whitespace:
          {
            ender_found = !char_is_space(byte);
          }break;
          case TXTI_TokenKind_Identifier:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$');
          }break;
          case TXTI_TokenKind_Numeric:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '.');
          }break;
          case TXTI_TokenKind_String:
          {
            ender_found = (!escaped && ((!string_is_char && byte == '"') || (string_is_char && byte == '\'')));
            ender_pad += 1;
          }break;
          case TXTI_TokenKind_Symbol:
          {
            ender_found = (byte != '~' && byte != '!' &&
                           byte != '%' && byte != '^' &&
                           byte != '&' && byte != '*' &&
                           byte != '(' && byte != ')' &&
                           byte != '-' && byte != '=' &&
                           byte != '+' && byte != '[' &&
                           byte != ']' && byte != '{' &&
                           byte != '}' && byte != ':' &&
                           byte != ';' && byte != ',' &&
                           byte != '.' && byte != '<' &&
                           byte != '>' && byte != '/' &&
                           byte != '?' && byte != '|');
          }break;
          case TXTI_TokenKind_Comment:
          {
            if(comment_is_single_line)
            {
              ender_found = (!escaped && (byte == '\r' || byte == '\n'));
            }
            else
            {
              ender_found = (active_token_start_idx+1 < idx && byte == '*' && next_byte == '/');
              ender_pad += 2;
            }
          }break;
          case TXTI_TokenKind_Meta:
          {
            ender_found = (!escaped && (byte == '\r' || byte == '\n'));
          }break;
        }
      }
      
      // rjf: next byte is ender => emit token
      if(ender_found)
      {
        TXTI_Token token = {active_token_kind, r1u64(active_token_start_idx, idx+ender_pad)};
        active_token_kind = TXTI_TokenKind_Null;
        
        // rjf: identifier -> keyword in special cases
        if(token.kind == TXTI_TokenKind_Identifier)
        {
          read_only local_persist String8 cpp_keywords[] =
          {
            str8_lit_comp("alignas"),
            str8_lit_comp("alignof"),
            str8_lit_comp("and"),
            str8_lit_comp("and_eq"),
            str8_lit_comp("asm"),
            str8_lit_comp("atomic_cancel"),
            str8_lit_comp("atomic_commit"),
            str8_lit_comp("atomic_noexcept"),
            str8_lit_comp("auto"),
            str8_lit_comp("bitand"),
            str8_lit_comp("bitor"),
            str8_lit_comp("bool"),
            str8_lit_comp("break"),
            str8_lit_comp("case"),
            str8_lit_comp("catch"),
            str8_lit_comp("char"),
            str8_lit_comp("char8_t"),
            str8_lit_comp("char16_t"),
            str8_lit_comp("char32_t"),
            str8_lit_comp("class"),
            str8_lit_comp("compl"),
            str8_lit_comp("concept"),
            str8_lit_comp("const"),
            str8_lit_comp("consteval"),
            str8_lit_comp("constexpr"),
            str8_lit_comp("constinit"),
            str8_lit_comp("const_cast"),
            str8_lit_comp("continue"),
            str8_lit_comp("co_await"),
            str8_lit_comp("co_return"),
            str8_lit_comp("co_yield"),
            str8_lit_comp("decltype"),
            str8_lit_comp("default"),
            str8_lit_comp("delete"),
            str8_lit_comp("do"),
            str8_lit_comp("double"),
            str8_lit_comp("dynamic_cast"),
            str8_lit_comp("else"),
            str8_lit_comp("enum"),
            str8_lit_comp("explicit"),
            str8_lit_comp("export"),
            str8_lit_comp("extern"),
            str8_lit_comp("false"),
            str8_lit_comp("float"),
            str8_lit_comp("for"),
            str8_lit_comp("friend"),
            str8_lit_comp("goto"),
            str8_lit_comp("if"),
            str8_lit_comp("inline"),
            str8_lit_comp("int"),
            str8_lit_comp("long"),
            str8_lit_comp("mutable"),
            str8_lit_comp("namespace"),
            str8_lit_comp("new"),
            str8_lit_comp("noexcept"),
            str8_lit_comp("not"),
            str8_lit_comp("not_eq"),
            str8_lit_comp("nullptr"),
            str8_lit_comp("operator"),
            str8_lit_comp("or"),
            str8_lit_comp("or_eq"),
            str8_lit_comp("private"),
            str8_lit_comp("protected"),
            str8_lit_comp("public"),
            str8_lit_comp("reflexpr"),
            str8_lit_comp("register"),
            str8_lit_comp("reinterpret_cast"),
            str8_lit_comp("requires"),
            str8_lit_comp("return"),
            str8_lit_comp("short"),
            str8_lit_comp("signed"),
            str8_lit_comp("sizeof"),
            str8_lit_comp("static"),
            str8_lit_comp("static_assert"),
            str8_lit_comp("static_cast"),
            str8_lit_comp("struct"),
            str8_lit_comp("switch"),
            str8_lit_comp("synchronized"),
            str8_lit_comp("template"),
            str8_lit_comp("this"),
            str8_lit_comp("thread_local"),
            str8_lit_comp("throw"),
            str8_lit_comp("true"),
            str8_lit_comp("try"),
            str8_lit_comp("typedef"),
            str8_lit_comp("typeid"),
            str8_lit_comp("typename"),
            str8_lit_comp("union"),
            str8_lit_comp("unsigned"),
            str8_lit_comp("using"),
            str8_lit_comp("virtual"),
            str8_lit_comp("void"),
            str8_lit_comp("volatile"),
            str8_lit_comp("wchar_t"),
            str8_lit_comp("while"),
            str8_lit_comp("xor"),
            str8_lit_comp("xor_eq"),
          };
          String8 token_string = str8_substr(string, r1u64(active_token_start_idx, idx+ender_pad));
          for(U64 keyword_idx = 0; keyword_idx < ArrayCount(cpp_keywords); keyword_idx += 1)
          {
            if(str8_match(cpp_keywords[keyword_idx], token_string, 0))
            {
              token.kind = TXTI_TokenKind_Keyword;
              break;
            }
          }
        }
        
        // rjf: push
        txti_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        
        // rjf: increment by ender padding
        idx += ender_pad;
      }
      
      // rjf: advance by 1 byte if we haven't found an ender
      if(!ender_found)
      {
        idx += 1;
      }
      escaped = next_escaped;
    }
  }
  
  //- rjf: token list -> token array
  TXTI_TokenArray result = txti_token_array_from_chunk_list(arena, &tokens);
  scratch_end(scratch);
  return result;
}

internal TXTI_TokenArray
txti_token_array_from_string__odin(Arena *arena, U64 *bytes_processed_counter, String8 string)
{
  //- jle: based on txti_token_array_from_string__cpp (alot of it is exactly the same)
  //
  // tokenization is a lot like C
  // unique set of keywords
  // has #tags and #directives
  //
  // tokenizer source code can be found here
  // https://github.com/odin-lang/Odin/tree/master/core/odin/tokenizer
  
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: generate token list
  TXTI_TokenChunkList tokens = {0};
  {
    B32 comment_is_single_line = 0;
    B32 string_is_char = 0;
    TXTI_TokenKind active_token_kind = TXTI_TokenKind_Null;
    U64 active_token_start_idx = 0;
    B32 escaped = 0;
    B32 next_escaped = 0;
    U64 byte_process_start_idx = 0;
    for(U64 idx = 0; idx <= string.size;)
    {
      U8 byte      = (idx+0 < string.size) ? (string.str[idx+0]) : 0;
      U8 next_byte = (idx+1 < string.size) ? (string.str[idx+1]) : 0;
      
      // rjf: update counter
      if(bytes_processed_counter != 0 && ((idx-byte_process_start_idx) >= 1000 || idx == string.size))
      {
        ins_atomic_u64_add_eval(bytes_processed_counter, (idx-byte_process_start_idx));
        byte_process_start_idx = idx;
      }
      
      // rjf: escaping
      if(escaped && (byte != '\r' && byte != '\n'))
      {
        next_escaped = 0;
      }
      else if(!escaped && byte == '\\')
      {
        next_escaped = 1;
      }
      
      // rjf: take starter, determine active token kind
      if(active_token_kind == TXTI_TokenKind_Null)
      {
        // rjf: use next bytes to start a new token
        if(0){}
        else if(char_is_space(byte))             { active_token_kind = TXTI_TokenKind_Whitespace; }
        else if(byte == '_' ||
                byte == '$' ||
                char_is_alpha(byte))             { active_token_kind = char_is_lower(byte) ? TXTI_TokenKind_Identifier: TXTI_TokenKind_Type; }
        else if(char_is_digit(byte, 10) ||
                (byte == '.' &&
                 char_is_digit(next_byte, 10)))  { active_token_kind = TXTI_TokenKind_Numeric; }
        else if(byte == '"')                     { active_token_kind = TXTI_TokenKind_String; string_is_char = 0; }
        else if(byte == '\'')                    { active_token_kind = TXTI_TokenKind_String; string_is_char = 1; }
        else if(byte == '/' && next_byte == '/') { active_token_kind = TXTI_TokenKind_Comment; comment_is_single_line = 1; }
        else if(byte == '/' && next_byte == '*') { active_token_kind = TXTI_TokenKind_Comment; comment_is_single_line = 0; }
        else if(byte == '~' || byte == '!' ||
                byte == '%' || byte == '^' ||
                byte == '&' || byte == '*' ||
                byte == '(' || byte == ')' ||
                byte == '-' || byte == '=' ||
                byte == '+' || byte == '[' ||
                byte == ']' || byte == '{' ||
                byte == '}' || byte == ':' ||
                byte == ';' || byte == ',' ||
                byte == '.' || byte == '<' ||
                byte == '>' || byte == '/' ||
                byte == '?' || byte == '|')      { active_token_kind = TXTI_TokenKind_Symbol; }
        else if(byte == '#')                     { active_token_kind = TXTI_TokenKind_Meta; }
        
        // rjf: start new token
        if(active_token_kind != TXTI_TokenKind_Null)
        {
          active_token_start_idx = idx;
        }
        
        // rjf: invalid token kind -> emit error
        else
        {
          TXTI_Token token = {TXTI_TokenKind_Error, r1u64(idx, idx+1)};
          txti_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
      }
      
      // rjf: look for ender
      U64 ender_pad = 0;
      B32 ender_found = 0;
      if(active_token_kind != TXTI_TokenKind_Null && idx>active_token_start_idx)
      {
        if(idx == string.size)
        {
          ender_pad = 0;
          ender_found = 1;
        }
        else switch(active_token_kind)
        {
          default:break;
          case TXTI_TokenKind_Whitespace:
          {
            ender_found = !char_is_space(byte);
          }break;
          case TXTI_TokenKind_Identifier:
          case TXTI_TokenKind_Type:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$');
          }break;
          case TXTI_TokenKind_Numeric:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '.');
          }break;
          case TXTI_TokenKind_String:
          {
            ender_found = (!escaped && ((!string_is_char && byte == '"') || (string_is_char && byte == '\'')));
            ender_pad += 1;
          }break;
          case TXTI_TokenKind_Symbol:
          {
            ender_found = (byte != '~' && byte != '!' &&
                           byte != '%' && byte != '^' &&
                           byte != '&' && byte != '*' &&
                           byte != '(' && byte != ')' &&
                           byte != '-' && byte != '=' &&
                           byte != '+' && byte != '[' &&
                           byte != ']' && byte != '{' &&
                           byte != '}' && byte != ':' &&
                           byte != ';' && byte != ',' &&
                           byte != '.' && byte != '<' &&
                           byte != '>' && byte != '/' &&
                           byte != '?' && byte != '|');
          }break;
          case TXTI_TokenKind_Comment:
          {
            if(comment_is_single_line)
            {
              ender_found = (!escaped && (byte == '\r' || byte == '\n'));
            }
            else
            {
              ender_found = (active_token_start_idx+1 < idx && byte == '*' && next_byte == '/');
              ender_pad += 2;
            }
          }break;
          case TXTI_TokenKind_Meta:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$');
          }break;
        }
      }
      
      // rjf: next byte is ender => emit token
      if(ender_found)
      {
        TXTI_Token token = {active_token_kind, r1u64(active_token_start_idx, idx+ender_pad)};
        active_token_kind = TXTI_TokenKind_Null;
        
        // rjf: identifier -> keyword in special cases
        if(token.kind == TXTI_TokenKind_Identifier)
        {
          read_only local_persist String8 cpp_keywords[] =
          {
            str8_lit_comp("asm"),
            str8_lit_comp("auto_case"),
            str8_lit_comp("bit_set"),
            str8_lit_comp("break"),
            str8_lit_comp("case"),
            str8_lit_comp("cast"),
            str8_lit_comp("context"),
            str8_lit_comp("continue"),
            str8_lit_comp("defer"),
            str8_lit_comp("distinct"),
            str8_lit_comp("do"),
            str8_lit_comp("dynamic"),
            str8_lit_comp("else"),
            str8_lit_comp("enum"),
            str8_lit_comp("fallthrough"),
            str8_lit_comp("for"),
            str8_lit_comp("foreign"),
            str8_lit_comp("if"),
            str8_lit_comp("if"),
            str8_lit_comp("in"),
            str8_lit_comp("inline"),
            str8_lit_comp("inline"),
            str8_lit_comp("map"),
            str8_lit_comp("matrix"),
            str8_lit_comp("no_inline"),
            str8_lit_comp("not_in"),
            str8_lit_comp("or_break"),
            str8_lit_comp("or_continue"),
            str8_lit_comp("or_else"),
            str8_lit_comp("or_return"),
            str8_lit_comp("package"),
            str8_lit_comp("proc"),
            str8_lit_comp("return"),
            str8_lit_comp("struct"),
            str8_lit_comp("switch"),
            str8_lit_comp("transmute"),
            str8_lit_comp("typeid"),
            str8_lit_comp("union"),
            str8_lit_comp("using"),
            str8_lit_comp("when"),
            str8_lit_comp("where"),
            str8_lit_comp("import"),
          };
          String8 token_string = str8_substr(string, r1u64(active_token_start_idx, idx+ender_pad));
          for(U64 keyword_idx = 0; keyword_idx < ArrayCount(cpp_keywords); keyword_idx += 1)
          {
            if(str8_match(cpp_keywords[keyword_idx], token_string, 0))
            {
              token.kind = TXTI_TokenKind_Keyword;
              break;
            }
          }
        }
        
        // rjf: push
        txti_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        
        // rjf: increment by ender padding
        idx += ender_pad;
      }
      
      // rjf: advance by 1 byte if we haven't found an ender
      if(!ender_found)
      {
        idx += 1;
      }
      escaped = next_escaped;
    }
  }
  
  //- rjf: token list -> token array
  TXTI_TokenArray result = txti_token_array_from_chunk_list(arena, &tokens);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Message Type Functions

internal void
txti_msg_list_push(Arena *arena, TXTI_MsgList *msgs, TXTI_Msg *msg)
{
  TXTI_MsgNode *node = push_array(arena, TXTI_MsgNode, 1);
  MemoryCopyStruct(&node->v, msg);
  SLLQueuePush(msgs->first, msgs->last, node);
  msgs->count += 1;
}

internal void
txti_msg_list_concat_in_place(TXTI_MsgList *dst, TXTI_MsgList *src)
{
  if(dst->last == 0)
  {
    MemoryCopyStruct(dst, src);
  }
  else if(src->first)
  {
    dst->last->next = src->first;
    dst->last = src->last;
    dst->count += src->count;
  }
  MemoryZeroStruct(src);
}

internal TXTI_MsgList
txti_msg_list_deep_copy(Arena *arena, TXTI_MsgList *src)
{
  TXTI_MsgList dst = {0};
  for(TXTI_MsgNode *src_n = src->first; src_n != 0; src_n = src_n->next)
  {
    TXTI_MsgNode *dst_n = push_array(arena, TXTI_MsgNode, 1);
    SLLQueuePush(dst.first, dst.last, dst_n);
    dst.count += 1;
    MemoryCopyStruct(&dst_n->v, &src_n->v);
    dst_n->v.string = push_str8_copy(arena, src_n->v.string);
  }
  return dst;
}

////////////////////////////////
//~ rjf: Entities API

//- rjf: opening entities & correllation w/ path

internal TXTI_Handle
txti_handle_from_path(String8 path)
{
  TXTI_Handle handle = {0};
  {
    // rjf: path -> hash * slot *stripe
    U64 hash = txti_hash_from_string(path);
    U64 slot_idx = hash%txti_state->entity_map.slots_count;
    U64 stripe_idx = slot_idx%txti_state->entity_map_stripes.count;
    TXTI_EntitySlot *slot = &txti_state->entity_map.slots[slot_idx];
    TXTI_Stripe *stripe = &txti_state->entity_map_stripes.v[stripe_idx];
    
    // rjf: determine if entity exists (shared lock)
    TXTI_Entity *found_entity = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(TXTI_Entity *entity = slot->first; entity != 0; entity = entity->next)
      {
        if(str8_match(entity->path, path, 0))
        {
          found_entity = entity;
          break;
        }
      }
      if(found_entity != 0)
      {
        handle.u64[0] = hash;
        handle.u64[1] = found_entity->id;
      }
    }
    
    // rjf: if entity does not exist -> exclusive lock & check again -- if still
    // does not exist, then build it
    if(found_entity == 0) OS_MutexScopeW(stripe->rw_mutex)
    {
      for(TXTI_Entity *entity = slot->first; entity != 0; entity = entity->next)
      {
        if(str8_match(entity->path, path, 0))
        {
          found_entity = entity;
          break;
        }
      }
      if(found_entity == 0)
      {
        TXTI_Entity *entity = push_array(stripe->arena, TXTI_Entity, 1);
        entity->path = push_str8_copy(stripe->arena, path);
        entity->id = ins_atomic_u64_inc_eval(&txti_state->entity_id_gen);
        for(U64 idx = 0; idx < TXTI_ENTITY_BUFFER_COUNT; idx += 1)
        {
          TXTI_Buffer *buffer = &entity->buffers[idx];
          buffer->data_arena = arena_alloc__sized(GB(32), KB(64));
          buffer->analysis_arena = arena_alloc__sized(GB(32), KB(64));
          buffer->data_arena->align = 1;
        }
        SLLQueuePush(slot->first, slot->last, entity);
        found_entity = entity;
      }
      handle.u64[0] = hash;
      handle.u64[1] = found_entity->id;
    }
  }
  return handle;
}

//- rjf: buffer introspection

internal TXTI_BufferInfo
txti_buffer_info_from_handle(Arena *arena, TXTI_Handle handle)
{
  TXTI_BufferInfo result = {0};
  U64 hash = handle.u64[0];
  U64 id = handle.u64[1];
  U64 slot_idx = hash%txti_state->entity_map.slots_count;
  U64 stripe_idx = slot_idx%txti_state->entity_map_stripes.count;
  TXTI_EntitySlot *slot = &txti_state->entity_map.slots[slot_idx];
  TXTI_Stripe *stripe = &txti_state->entity_map_stripes.v[stripe_idx];
  OS_MutexScopeR(stripe->rw_mutex)
  {
    TXTI_Entity *entity = 0;
    for(TXTI_Entity *e = slot->first; e != 0; e = e->next)
    {
      if(e->id == id)
      {
        entity = e;
        break;
      }
    }
    if(entity != 0)
    {
      TXTI_Buffer *buffer = &entity->buffers[entity->buffer_apply_gen%TXTI_ENTITY_BUFFER_COUNT];
      result.path             = push_str8_copy(arena, entity->path);
      result.timestamp        = entity->timestamp;
      result.line_end_kind    = entity->line_end_kind;
      result.total_line_count = buffer->lines_count;
      result.max_line_size    = buffer->lines_max_size;
      result.mut_gen          = entity->mut_gen;
      result.buffer_apply_gen = entity->buffer_apply_gen;
      result.bytes_processed  = ins_atomic_u64_eval(&entity->bytes_processed);
      result.bytes_to_process = ins_atomic_u64_eval(&entity->bytes_to_process);
    }
  }
  result.total_line_count = Max(1, result.total_line_count);
  return result;
}

internal TXTI_Slice
txti_slice_from_handle_line_range(Arena *arena, TXTI_Handle handle, Rng1S64 line_range)
{
  ProfBeginFunction();
  TXTI_Slice result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  U64 hash = handle.u64[0];
  U64 id = handle.u64[1];
  U64 slot_idx = hash%txti_state->entity_map.slots_count;
  U64 stripe_idx = slot_idx%txti_state->entity_map_stripes.count;
  TXTI_EntitySlot *slot = &txti_state->entity_map.slots[slot_idx];
  TXTI_Stripe *stripe = &txti_state->entity_map_stripes.v[stripe_idx];
  OS_MutexScopeR(stripe->rw_mutex)
  {
    TXTI_Entity *entity = 0;
    for(TXTI_Entity *e = slot->first; e != 0; e = e->next)
    {
      if(e->id == id)
      {
        entity = e;
        break;
      }
    }
    if(entity != 0)
    {
      TXTI_Buffer *buffer = &entity->buffers[entity->buffer_apply_gen%TXTI_ENTITY_BUFFER_COUNT];
      Rng1S64 line_range_clamped = r1s64(Clamp(1, line_range.min, (S64)buffer->lines_count), Clamp(1, line_range.max, (S64)buffer->lines_count));
      
      // rjf: allocate output arrays
      result.line_count = (U64)dim_1s64(line_range_clamped)+1;
      result.line_text = push_array(arena, String8, result.line_count);
      result.line_ranges = push_array(arena, Rng1U64, result.line_count);
      result.line_tokens = push_array(arena, TXTI_TokenArray, result.line_count);
      
      // rjf: fill line ranges & text
      U64 line_slice_idx = 0;
      U64 line_buffer_idx = line_range_clamped.min-1;
      ProfScope("fill line ranges & text")
        for(S64 line_num = line_range_clamped.min;
            line_num <= line_range_clamped.max && line_buffer_idx < buffer->lines_count;
            line_num += 1,
            line_slice_idx += 1,
            line_buffer_idx += 1)
      {
        Rng1U64 range = buffer->lines_ranges[line_buffer_idx];
        String8 line_text_internal = str8_substr(buffer->data, range);
        result.line_ranges[line_slice_idx] = range;
        result.line_text[line_slice_idx] = push_str8_copy(arena, line_text_internal);
      }
      
      // rjf: binary search to find first token
      TXTI_Token *tokens_first = 0;
      ProfScope("binary search to find first token")
      {
        Rng1U64 slice_range = r1u64(result.line_ranges[0].min, result.line_ranges[result.line_count-1].max);
        U64 min_idx = 0;
        U64 opl_idx = buffer->tokens.count;
        for(;;)
        {
          U64 mid_idx = (opl_idx+min_idx)/2;
          if(mid_idx >= opl_idx)
          {
            break;
          }
          TXTI_Token *mid_token = &buffer->tokens.v[mid_idx];
          if(mid_token->range.min > slice_range.max)
          {
            opl_idx = mid_idx;
          }
          else if(mid_token->range.max < slice_range.min)
          {
            min_idx = mid_idx;
          }
          else if(tokens_first == 0 || mid_token->range.min < tokens_first->range.min)
          {
            tokens_first = mid_token;
            opl_idx = mid_idx;
          }
          if(mid_idx == min_idx && mid_idx+1 == opl_idx)
          {
            break;
          }
        }
      }
      
      // rjf: grab per-line tokens
      TXTI_TokenList *line_tokens_lists = push_array(scratch.arena, TXTI_TokenList, result.line_count);
      if(tokens_first != 0) ProfScope("grab per-line tokens")
      {
        TXTI_Token *tokens_opl = buffer->tokens.v+buffer->tokens.count;
        U64 line_slice_idx = 0;
        for(TXTI_Token *token = tokens_first; token < tokens_opl && line_slice_idx < result.line_count;)
        {
          if(token->range.min < result.line_ranges[line_slice_idx].max)
          {
            if(token->range.max > result.line_ranges[line_slice_idx].min)
            {
              txti_token_list_push(scratch.arena, &line_tokens_lists[line_slice_idx], token);
            }
            B32 need_token_advance = 0;
            B32 need_line_advance = 0;
            if(token->range.max >= result.line_ranges[line_slice_idx].max)
            {
              need_line_advance = 1;
            }
            if(token->range.max <= result.line_ranges[line_slice_idx].max)
            {
              need_token_advance += 1;
            }
            if(need_line_advance) { line_slice_idx += 1; }
            if(need_token_advance) { token += 1; }
          }
          else
          {
            line_slice_idx += 1;
          }
        }
      }
      
      // rjf: bake per-line tokens to arrays
      for(U64 line_slice_idx = 0; line_slice_idx < result.line_count; line_slice_idx += 1)
      {
        result.line_tokens[line_slice_idx] = txti_token_array_from_list(arena, &line_tokens_lists[line_slice_idx]);
      }
    }
  }
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal String8
txti_string_from_handle_txt_rng(Arena *arena, TXTI_Handle handle, TxtRng range)
{
  String8 result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    Rng1S64 line_range = r1s64(ClampBot(1, range.min.line), ClampBot(1, range.max.line));
    TXTI_BufferInfo info = txti_buffer_info_from_handle(scratch.arena, handle);
    TXTI_Slice slice = txti_slice_from_handle_line_range(scratch.arena, handle, line_range);
    String8List line_strings = {0};
    for(U64 line_idx = 0; line_idx < slice.line_count; line_idx += 1)
    {
      String8 line_text = slice.line_text[line_idx];
      if(line_idx == slice.line_count-1)
      {
        line_text = str8_prefix(line_text, range.max.column-1);
      }
      if(line_idx == 0)
      {
        line_text = str8_skip(line_text, range.min.column-1);
      }
      str8_list_push(scratch.arena, &line_strings, line_text);
    }
    StringJoin join = {0};
    switch(info.line_end_kind)
    {
      default:
      case TXTI_LineEndKind_LF:{join.sep = str8_lit("\n");}break;
      case TXTI_LineEndKind_CRLF:{join.sep = str8_lit("\r\n");}break;
    }
    result = str8_list_join(arena, &line_strings, &join);
  }
  scratch_end(scratch);
  return result;
}

internal String8
txti_string_from_handle_line_num(Arena *arena, TXTI_Handle handle, S64 line_num)
{
  String8 result = {0};
  TXTI_Slice slice = txti_slice_from_handle_line_range(arena, handle, r1s64(line_num, line_num));
  if(slice.line_count != 0)
  {
    result = slice.line_text[0];
  }
  return result;
}

internal Rng1U64
txti_expr_range_from_line_off_range_string_tokens(U64 off, Rng1U64 line_range, String8 line_text, TXTI_TokenArray *line_tokens)
{
  Rng1U64 result = {0};
  Temp scratch = scratch_begin(0, 0);
  {
    // rjf: unpack line info
    TXTI_Token *line_tokens_first = line_tokens->v;
    TXTI_Token *line_tokens_opl = line_tokens->v+line_tokens->count;
    
    // rjf: find token containing `off`
    TXTI_Token *pt_token = 0;
    for(TXTI_Token *token = line_tokens_first;
        token < line_tokens_opl;
        token += 1)
    {
      if(contains_1u64(token->range, off))
      {
        Rng1U64 token_range_clamped = intersect_1u64(line_range, token->range);
        String8 token_string = str8_substr(line_text, r1u64(token_range_clamped.max - line_range.min, token_range_clamped.max - line_range.min));
        B32 token_ender = 0;
        switch(token->kind)
        {
          default:{}break;
          case TXTI_TokenKind_Symbol:
          {
            token_ender = (str8_match(token_string, str8_lit("]"), 0));
          }break;
          case TXTI_TokenKind_Identifier:
          case TXTI_TokenKind_Keyword:
          case TXTI_TokenKind_String:
          case TXTI_TokenKind_Meta:
          {
            token_ender = 1;
          }break;
        }
        if(token_ender)
        {
          pt_token = token;
        }
        break;
      }
    }
    
    // rjf: found token containing `off`? -> mark that as our initial range
    if(pt_token != 0)
    {
      result = pt_token->range;
    }
    
    // rjf: walk back from pt_token - try to find plausible start of expression
    if(pt_token != 0)
    {
      B32 walkback_done = 0;
      S32 nest = 0;
      for(TXTI_Token *wb_token = pt_token;
          wb_token >= line_tokens_first && walkback_done == 0;
          wb_token -= 1)
      {
        Rng1U64 wb_token_range_clamped = intersect_1u64(line_range, wb_token->range);
        String8 wb_token_string = str8_substr(line_text, r1u64(wb_token_range_clamped.min - line_range.min, wb_token_range_clamped.max - line_range.min));
        B32 include_wb_token = 0;
        switch(wb_token->kind)
        {
          default:{}break;
          case TXTI_TokenKind_Symbol:
          {
            B32 is_dot = str8_match(wb_token_string, str8_lit("."), 0);
            B32 is_arrow = str8_match(wb_token_string, str8_lit("->"), 0);
            B32 is_open_bracket = str8_match(wb_token_string, str8_lit("["), 0);
            B32 is_close_bracket = str8_match(wb_token_string, str8_lit("]"), 0);
            nest -= !!(is_open_bracket);
            nest += !!(is_close_bracket);
            if(is_dot ||
               is_arrow ||
               is_open_bracket||
               is_close_bracket)
            {
              include_wb_token = 1;
            }
          }break;
          case TXTI_TokenKind_Identifier:
          {
            include_wb_token = 1;
          }break;
        }
        if(include_wb_token)
        {
          result = union_1u64(result, wb_token->range);
        }
        else if(nest == 0)
        {
          walkback_done = 1;
        }
      }
    }
  }
  scratch_end(scratch);
  return result;
}

internal TxtRng
txti_expr_range_from_handle_pt(TXTI_Handle handle, TxtPt pt)
{
  TxtRng result = {0};
  Temp scratch = scratch_begin(0, 0);
  TXTI_Slice slice = txti_slice_from_handle_line_range(scratch.arena, handle, r1s64(pt.line, pt.line));
  if(slice.line_count != 0)
  {
    // rjf: unpack line info
    String8 line_text = slice.line_text[0];
    Rng1U64 line_range = slice.line_ranges[0];
    TXTI_TokenArray line_tokens = slice.line_tokens[0];
    TXTI_Token *line_tokens_first = line_tokens.v;
    TXTI_Token *line_tokens_opl = line_tokens.v+line_tokens.count;
    U64 pt_off = line_range.min + (pt.column-1);
    
    // rjf: grab offset range of expression
    Rng1U64 expr_off_rng = txti_expr_range_from_line_off_range_string_tokens(pt_off, line_range, line_text, &line_tokens);
    
    // rjf: convert offset range into text range
    result = txt_rng(txt_pt(pt.line, 1+(expr_off_rng.min-line_range.min)), txt_pt(pt.line, 1+(expr_off_rng.max-line_range.min)));
  }
  scratch_end(scratch);
  return result;
}

//- rjf: buffer mutations

internal void
txti_reload(TXTI_Handle handle, String8 path)
{
  U64 hash = handle.u64[0];
  U64 id = handle.u64[1];
  U64 mut_thread_idx = id%txti_state->mut_thread_count;
  TXTI_MutThread *mut_thread = &txti_state->mut_threads[mut_thread_idx];
  OS_MutexScope(mut_thread->msg_mutex)
  {
    TXTI_MsgNode *node = push_array(mut_thread->msg_arena, TXTI_MsgNode, 1);
    TXTI_Msg *msg = &node->v;
    msg->kind    = TXTI_MsgKind_Reload;
    msg->handle  = handle;
    msg->string  = push_str8_copy(mut_thread->msg_arena, path);
    SLLQueuePush(mut_thread->msg_list.first, mut_thread->msg_list.last, node);
    mut_thread->msg_list.count += 1;
  }
  os_condition_variable_broadcast(mut_thread->msg_cv);
}

internal void
txti_append(TXTI_Handle handle, String8 string)
{
  U64 hash = handle.u64[0];
  U64 id = handle.u64[1];
  U64 mut_thread_idx = id%txti_state->mut_thread_count;
  TXTI_MutThread *mut_thread = &txti_state->mut_threads[mut_thread_idx];
  OS_MutexScope(mut_thread->msg_mutex)
  {
    TXTI_MsgNode *node = push_array(mut_thread->msg_arena, TXTI_MsgNode, 1);
    TXTI_Msg *msg = &node->v;
    msg->kind    = TXTI_MsgKind_Append;
    msg->handle  = handle;
    msg->string  = push_str8_copy(mut_thread->msg_arena, string);
    SLLQueuePush(mut_thread->msg_list.first, mut_thread->msg_list.last, node);
    mut_thread->msg_list.count += 1;
  }
  os_condition_variable_broadcast(mut_thread->msg_cv);
}

//- rjf: buffer external change detection enabling/disabling

internal void
txti_set_external_change_detection_enabled(B32 enabled)
{
  U64 enabled_u64 = (U64)enabled;
  ins_atomic_u64_eval_assign(&txti_state->detector_thread_enabled, enabled_u64);
}

////////////////////////////////
//~ rjf: Mutator Threads

internal void
txti_mut_thread_entry_point(void *p)
{
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  
  U64 mut_thread_idx = (U64)p;
  ProfThreadName("[txti] mut #%I64u", mut_thread_idx);
  TXTI_MutThread *mut_thread = &txti_state->mut_threads[mut_thread_idx];
  for(;;)
  {
    //- rjf: begin
    Temp scratch = scratch_begin(0, 0);
    
    //- rjf: pull messages
    TXTI_MsgList msgs = {0};
    OS_MutexScope(mut_thread->msg_mutex) for(;;)
    {
      if(mut_thread->msg_list.count != 0)
      {
        msgs = txti_msg_list_deep_copy(scratch.arena, &mut_thread->msg_list);
        MemoryZeroStruct(&mut_thread->msg_list);
        arena_clear(mut_thread->msg_arena);
        break;
      }
      os_condition_variable_wait(mut_thread->msg_cv, mut_thread->msg_mutex, max_U64);
    }
    
    //- rjf: process msgs
    for(TXTI_MsgNode *msg_n = msgs.first; msg_n != 0; msg_n = msg_n->next) ProfScope("process msg")
    {
      //- rjf: unpack message
      TXTI_Msg *msg = &msg_n->v;
      U64 hash = msg->handle.u64[0];
      U64 id = msg->handle.u64[1];
      U64 slot_idx = hash%txti_state->entity_map.slots_count;
      U64 stripe_idx = slot_idx%txti_state->entity_map_stripes.count;
      TXTI_EntitySlot *slot = &txti_state->entity_map.slots[slot_idx];
      TXTI_Stripe *stripe = &txti_state->entity_map_stripes.v[stripe_idx];
      
      //- rjf: load file if we need it
      B32 load_valid = 0;
      String8 file_contents = {0};
      TXTI_LangKind lang_kind = TXTI_LangKind_Null;
      U64 timestamp = 0;
      if(msg->kind == TXTI_MsgKind_Reload)
      {
        FileProperties pre_load_props = os_properties_from_file_path(msg->string);
        OS_Handle file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite, msg->string);
        file_contents = os_string_from_file_range(scratch.arena, file, r1u64(0, pre_load_props.size));
        lang_kind = txti_lang_kind_from_extension(str8_skip_last_dot(msg->string));
        os_file_close(file);
        FileProperties post_load_props = os_properties_from_file_path(msg->string);
        load_valid = (post_load_props.modified == pre_load_props.modified);
        if(load_valid)
        {
          timestamp = pre_load_props.modified;
        }
      }
      
      //- rjf: nonzero lang kind -> unpack lang info
      TXTI_LangLexFunctionType *lex_function = 0;
      switch(lang_kind)
      {
        default:{}break;
        case TXTI_LangKind_C:
        case TXTI_LangKind_CPlusPlus:
        {
          lex_function = txti_token_array_from_string__cpp;
        }break;
        case TXTI_LangKind_Odin:
        {
          lex_function = txti_token_array_from_string__odin;
        }break;
      }
      
      //- rjf: detect line end kind
      TXTI_LineEndKind line_end_kind = TXTI_LineEndKind_Null;
      if(load_valid)
      {
        U64 lf_count = 0;
        U64 cr_count = 0;
        for(U64 idx = 0; idx < file_contents.size && idx < 1024; idx += 1)
        {
          if(file_contents.str[idx] == '\r')
          {
            cr_count += 1;
          }
          if(file_contents.str[idx] == '\n')
          {
            lf_count += 1;
          }
        }
        if(cr_count >= lf_count/2 && lf_count >= 1)
        {
          line_end_kind = TXTI_LineEndKind_CRLF;
        }
        else if(lf_count >= 1)
        {
          line_end_kind = TXTI_LineEndKind_LF;
        }
      }
      
      //- rjf: obtain initial buffer_apply_gen, reset byte processing counters
      U64 initial_buffer_apply_gen = 0;
      if(load_valid) OS_MutexScopeR(stripe->rw_mutex)
      {
        TXTI_Entity *entity = 0;
        for(TXTI_Entity *e = slot->first; e != 0; e = e->next)
        {
          if(e->id == id)
          {
            entity = e;
            break;
          }
        }
        if(entity != 0)
        {
          initial_buffer_apply_gen = entity->buffer_apply_gen;
          if(msg->kind == TXTI_MsgKind_Reload)
          {
            ins_atomic_u64_eval_assign(&entity->bytes_processed, 0);
            ins_atomic_u64_eval_assign(&entity->bytes_to_process, file_contents.size + !!lex_function*file_contents.size);
          }
        }
      }
      
      //- rjf: apply edits
      if(load_valid)
      {
        for(U64 buffer_apply_idx = 0;
            buffer_apply_idx < TXTI_ENTITY_BUFFER_COUNT;
            buffer_apply_idx += 1)
        {
          // rjf: last buffer we're going to edit? -> bump buffer_apply_gen,
          // so that before we touch this last buffer, all readers of this
          // entity will get the already-modified buffers.
          if(buffer_apply_idx == TXTI_ENTITY_BUFFER_COUNT-1)
          {
            OS_MutexScopeW(stripe->rw_mutex)
            {
              TXTI_Entity *entity = 0;
              for(TXTI_Entity *e = slot->first; e != 0; e = e->next)
              {
                if(e->id == id)
                {
                  entity = e;
                  break;
                }
              }
              if(entity != 0)
              {
                entity->buffer_apply_gen += 1;
                if(line_end_kind != TXTI_LineEndKind_Null)
                {
                  entity->line_end_kind = line_end_kind;
                }
                if(lang_kind != TXTI_LangKind_Null)
                {
                  entity->lang_kind = lang_kind;
                }
                if(timestamp != 0)
                {
                  entity->timestamp = timestamp;
                }
              }
            }
          }
          
          // rjf: apply edit to this buffer.
          //
          // NOTE(rjf): all edits can apply *with a shared mutex lock*,
          // because only the mutator thread for this buffer can touch the
          // non-currently-viewable buffers. we only need to have an
          // exclusive lock to bump the buffer_apply_gen (to change the
          // actively viewable buffer).
          //
          OS_MutexScopeR(stripe->rw_mutex)
          {
            TXTI_Entity *entity = 0;
            for(TXTI_Entity *e = slot->first; e != 0; e = e->next)
            {
              if(e->id == id)
              {
                entity = e;
                break;
              }
            }
            if(entity != 0)
            {
              TXTI_Buffer *buffer = &entity->buffers[(initial_buffer_apply_gen+1+buffer_apply_idx)%TXTI_ENTITY_BUFFER_COUNT];
              
              // rjf: clear old analysis data
              arena_clear(buffer->analysis_arena);
              buffer->lines_count = 0;
              buffer->lines_ranges = 0;
              buffer->lines_max_size = 0;
              MemoryZeroStruct(&buffer->tokens);
              
              // rjf: perform edit to buffer data
              switch(msg->kind)
              {
                default:{}break;
                
                // rjf: replace range
                case TXTI_MsgKind_Append: ProfScope("append")
                {
                  U8 *append_data_buffer = push_array_no_zero(buffer->data_arena, U8, msg->string.size);
                  MemoryCopy(append_data_buffer, msg->string.str, msg->string.size);
                  buffer->data.size += msg->string.size;
                  if(buffer->data.str == 0 && msg->string.size != 0)
                  {
                    buffer->data.str = append_data_buffer;
                  }
                }break;
                
                // rjf: reload from disk
                case TXTI_MsgKind_Reload: ProfScope("reload")
                {
                  arena_clear(buffer->data_arena);
                  if(file_contents.size != 0)
                  {
                    buffer->data = push_str8_copy(buffer->data_arena, file_contents);
                  }
                  else
                  {
                    MemoryZeroStruct(&buffer->data);
                  }
                }break;
              }
              
              // rjf: parse & store line range info
              {
                // rjf: count # of lines
                U64 line_count = 1;
                U64 byte_process_start_idx = 0;
                for(U64 idx = 0; idx < buffer->data.size; idx += 1)
                {
                  if(buffer_apply_idx == 0 && idx-byte_process_start_idx >= 1000)
                  {
                    ins_atomic_u64_add_eval(&entity->bytes_processed, (idx-byte_process_start_idx));
                    byte_process_start_idx = idx;
                  }
                  if(buffer->data.str[idx] == '\n' || buffer->data.str[idx] == '\r')
                  {
                    line_count += 1;
                    if(buffer->data.str[idx] == '\r')
                    {
                      idx += 1;
                    }
                  }
                }
                
                // rjf: allocate & store line ranges
                buffer->lines_count = line_count;
                buffer->lines_ranges = push_array_no_zero(buffer->analysis_arena, Rng1U64, buffer->lines_count);
                U64 line_idx = 0;
                U64 line_start_idx = 0;
                for(U64 idx = 0; idx <= buffer->data.size; idx += 1)
                {
                  if(idx == buffer->data.size || buffer->data.str[idx] == '\n' || buffer->data.str[idx] == '\r')
                  {
                    Rng1U64 line_range = r1u64(line_start_idx, idx);
                    U64 line_size = dim_1u64(line_range);
                    buffer->lines_ranges[line_idx] = line_range;
                    buffer->lines_max_size = Max(buffer->lines_max_size, line_size);
                    line_idx += 1;
                    line_start_idx = idx+1;
                    if(idx < buffer->data.size && buffer->data.str[idx] == '\r')
                    {
                      line_start_idx += 1;
                      idx += 1;
                    }
                  }
                }
              }
              
              // rjf: lex file contents
              if(lex_function != 0)
              {
                buffer->tokens = lex_function(buffer->analysis_arena, buffer_apply_idx == 0 ? &entity->bytes_processed : 0, buffer->data);
              }
              
              // rjf: mark final process counter
              if(buffer_apply_idx == 0)
              {
                ins_atomic_u64_eval_assign(&entity->bytes_processed, entity->bytes_to_process);
              }
              
              // rjf: mark task completion
              if(buffer_apply_idx == TXTI_ENTITY_BUFFER_COUNT-1)
              {
                ins_atomic_u64_eval_assign(&entity->working_count, 0);
              }
            }
          }
        }
      }
    }
    
    //- rjf: end
    scratch_end(scratch);
  }
}

////////////////////////////////
//~ rjf: Detector Thread

internal void
txti_detector_thread_entry_point(void *p)
{
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  ProfThreadName("[txti] detector");
  for(;;)
  {
    if(ins_atomic_u64_eval(&txti_state->detector_thread_enabled))
    {
      U64 slots_per_stripe = txti_state->entity_map.slots_count/txti_state->entity_map_stripes.count;
      for(U64 stripe_idx = 0; stripe_idx < txti_state->entity_map_stripes.count; stripe_idx += 1)
      {
        TXTI_Stripe *stripe = &txti_state->entity_map_stripes.v[stripe_idx];
        OS_MutexScopeR(stripe->rw_mutex) for(U64 slot_in_stripe_idx = 0; slot_in_stripe_idx < slots_per_stripe; slot_in_stripe_idx += 1)
        {
          U64 slot_idx = stripe_idx*slots_per_stripe + slot_in_stripe_idx;
          TXTI_EntitySlot *slot = &txti_state->entity_map.slots[slot_idx];
          for(TXTI_Entity *entity = slot->first; entity != 0; entity = entity->next)
          {
            FileProperties props = os_properties_from_file_path(entity->path);
            U64 entity_timestamp = entity->timestamp;
            if(props.modified != entity_timestamp && ins_atomic_u64_eval(&entity->working_count) == 0)
            {
              TXTI_Handle handle = {txti_hash_from_string(entity->path), entity->id};
              txti_reload(handle, entity->path);
              ins_atomic_u64_inc_eval(&entity->working_count);
            }
          }
        }
      }
    }
    os_sleep_milliseconds(100);
  }
}
