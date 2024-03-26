// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Helpers

internal TXT_LangKind
txt_lang_kind_from_extension(String8 extension)
{
  TXT_LangKind kind = TXT_LangKind_Null;
  if(str8_match(extension, str8_lit("c"), 0) ||
     str8_match(extension, str8_lit("h"), 0))
  {
    kind = TXT_LangKind_C;
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
    kind = TXT_LangKind_CPlusPlus;
  }
  else if(str8_match(extension, str8_lit("odin"), StringMatchFlag_CaseInsensitive))
  {
    kind = TXT_LangKind_Odin;
  }
  return kind;
}

internal TXT_LangLexFunctionType *
txt_lex_function_from_lang_kind(TXT_LangKind kind)
{
  TXT_LangLexFunctionType *fn = 0;
  switch(kind)
  {
    default:{}break;
    case TXT_LangKind_C:           {fn = txt_token_array_from_string__c_cpp;}break;
    case TXT_LangKind_CPlusPlus:   {fn = txt_token_array_from_string__c_cpp;}break;
    case TXT_LangKind_Odin:        {fn = txt_token_array_from_string__odin;}break;
  }
  return fn;
}

////////////////////////////////
//~ rjf: Token Type Functions

internal void
txt_token_chunk_list_push(Arena *arena, TXT_TokenChunkList *list, U64 cap, TXT_Token *token)
{
  TXT_TokenChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, TXT_TokenChunkNode, 1);
    SLLQueuePush(list->first, list->last, node);
    node->cap = cap;
    node->v = push_array_no_zero(arena, TXT_Token, node->cap);
    list->chunk_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], token);
  node->count += 1;
  list->token_count += 1;
}

internal void
txt_token_list_push(Arena *arena, TXT_TokenList *list, TXT_Token *token)
{
  TXT_TokenNode *node = push_array(arena, TXT_TokenNode, 1);
  MemoryCopyStruct(&node->v, token);
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal TXT_TokenArray
txt_token_array_from_chunk_list(Arena *arena, TXT_TokenChunkList *list)
{
  TXT_TokenArray array = {0};
  array.count = list->token_count;
  array.v = push_array_no_zero(arena, TXT_Token, array.count);
  U64 idx = 0;
  for(TXT_TokenChunkNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopy(array.v+idx, n->v, n->count*sizeof(TXT_Token));
    idx += n->count;
  }
  return array;
}

internal TXT_TokenArray
txt_token_array_from_list(Arena *arena, TXT_TokenList *list)
{
  TXT_TokenArray array = {0};
  array.count = list->count;
  array.v = push_array_no_zero(arena, TXT_Token, array.count);
  U64 idx = 0;
  for(TXT_TokenNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopyStruct(array.v+idx, &n->v);
    idx += 1;
  }
  return array;
}

////////////////////////////////
//~ rjf: Lexing Functions

internal TXT_TokenArray
txt_token_array_from_string__c_cpp(Arena *arena, U64 *bytes_processed_counter, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: generate token list
  TXT_TokenChunkList tokens = {0};
  {
    B32 comment_is_single_line = 0;
    B32 string_is_char = 0;
    TXT_TokenKind active_token_kind = TXT_TokenKind_Null;
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
      if(active_token_kind == TXT_TokenKind_Null)
      {
        // rjf: use next bytes to start a new token
        if(0){}
        else if(char_is_space(byte))             { active_token_kind = TXT_TokenKind_Whitespace; }
        else if(byte == '_' ||
                byte == '$' ||
                char_is_alpha(byte))             { active_token_kind = TXT_TokenKind_Identifier; }
        else if(char_is_digit(byte, 10) ||
                (byte == '.' &&
                 char_is_digit(next_byte, 10)))  { active_token_kind = TXT_TokenKind_Numeric; }
        else if(byte == '"')                     { active_token_kind = TXT_TokenKind_String; string_is_char = 0; }
        else if(byte == '\'')                    { active_token_kind = TXT_TokenKind_String; string_is_char = 1; }
        else if(byte == '/' && next_byte == '/') { active_token_kind = TXT_TokenKind_Comment; comment_is_single_line = 1; }
        else if(byte == '/' && next_byte == '*') { active_token_kind = TXT_TokenKind_Comment; comment_is_single_line = 0; }
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
                byte == '?' || byte == '|')      { active_token_kind = TXT_TokenKind_Symbol; }
        else if(byte == '#')                     { active_token_kind = TXT_TokenKind_Meta; }
        
        // rjf: start new token
        if(active_token_kind != TXT_TokenKind_Null)
        {
          active_token_start_idx = idx;
        }
        
        // rjf: invalid token kind -> emit error
        else
        {
          TXT_Token token = {TXT_TokenKind_Error, r1u64(idx, idx+1)};
          txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
      }
      
      // rjf: look for ender
      U64 ender_pad = 0;
      B32 ender_found = 0;
      if(active_token_kind != TXT_TokenKind_Null && idx>active_token_start_idx)
      {
        if(idx == string.size)
        {
          ender_pad = 0;
          ender_found = 1;
        }
        else switch(active_token_kind)
        {
          default:break;
          case TXT_TokenKind_Whitespace:
          {
            ender_found = !char_is_space(byte);
          }break;
          case TXT_TokenKind_Identifier:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$');
          }break;
          case TXT_TokenKind_Numeric:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '.' && byte != '\'');
          }break;
          case TXT_TokenKind_String:
          {
            ender_found = (!escaped && ((!string_is_char && byte == '"') || (string_is_char && byte == '\'')));
            ender_pad += 1;
          }break;
          case TXT_TokenKind_Symbol:
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
          case TXT_TokenKind_Comment:
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
          case TXT_TokenKind_Meta:
          {
            ender_found = (!escaped && (byte == '\r' || byte == '\n'));
          }break;
        }
      }
      
      // rjf: next byte is ender => emit token
      if(ender_found)
      {
        TXT_Token token = {active_token_kind, r1u64(active_token_start_idx, idx+ender_pad)};
        active_token_kind = TXT_TokenKind_Null;
        
        // rjf: identifier -> keyword in special cases
        if(token.kind == TXT_TokenKind_Identifier)
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
              token.kind = TXT_TokenKind_Keyword;
              break;
            }
          }
        }
        
        // rjf: push
        txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        
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
  TXT_TokenArray result = txt_token_array_from_chunk_list(arena, &tokens);
  scratch_end(scratch);
  return result;
}

internal TXT_TokenArray
txt_token_array_from_string__odin(Arena *arena, U64 *bytes_processed_counter, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: generate token list
  TXT_TokenChunkList tokens = {0};
  {
    B32 comment_is_single_line = 0;
    B32 string_is_char = 0;
    TXT_TokenKind active_token_kind = TXT_TokenKind_Null;
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
      if(active_token_kind == TXT_TokenKind_Null)
      {
        // rjf: use next bytes to start a new token
        if(0){}
        else if(char_is_space(byte))             { active_token_kind = TXT_TokenKind_Whitespace; }
        else if(byte == '_' ||
                byte == '$' ||
                char_is_alpha(byte))             { active_token_kind = TXT_TokenKind_Identifier; }
        else if(char_is_digit(byte, 10) ||
                (byte == '.' &&
                 char_is_digit(next_byte, 10)))  { active_token_kind = TXT_TokenKind_Numeric; }
        else if(byte == '"')                     { active_token_kind = TXT_TokenKind_String; string_is_char = 0; }
        else if(byte == '\'')                    { active_token_kind = TXT_TokenKind_String; string_is_char = 1; }
        else if(byte == '/' && next_byte == '/') { active_token_kind = TXT_TokenKind_Comment; comment_is_single_line = 1; }
        else if(byte == '/' && next_byte == '*') { active_token_kind = TXT_TokenKind_Comment; comment_is_single_line = 0; }
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
                byte == '?' || byte == '|')      { active_token_kind = TXT_TokenKind_Symbol; }
        else if(byte == '#')                     { active_token_kind = TXT_TokenKind_Meta; }
        
        // rjf: start new token
        if(active_token_kind != TXT_TokenKind_Null)
        {
          active_token_start_idx = idx;
        }
        
        // rjf: invalid token kind -> emit error
        else
        {
          TXT_Token token = {TXT_TokenKind_Error, r1u64(idx, idx+1)};
          txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
      }
      
      // rjf: look for ender
      U64 ender_pad = 0;
      B32 ender_found = 0;
      if(active_token_kind != TXT_TokenKind_Null && idx>active_token_start_idx)
      {
        if(idx == string.size)
        {
          ender_pad = 0;
          ender_found = 1;
        }
        else switch(active_token_kind)
        {
          default:break;
          case TXT_TokenKind_Whitespace:
          {
            ender_found = !char_is_space(byte);
          }break;
          case TXT_TokenKind_Identifier:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$');
          }break;
          case TXT_TokenKind_Numeric:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '.' && byte != '\'');
          }break;
          case TXT_TokenKind_String:
          {
            ender_found = (!escaped && ((!string_is_char && byte == '"') || (string_is_char && byte == '\'')));
            ender_pad += 1;
          }break;
          case TXT_TokenKind_Symbol:
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
          case TXT_TokenKind_Comment:
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
          case TXT_TokenKind_Meta:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$');
          }break;
        }
      }
      
      // rjf: next byte is ender => emit token
      if(ender_found)
      {
        TXT_Token token = {active_token_kind, r1u64(active_token_start_idx, idx+ender_pad)};
        active_token_kind = TXT_TokenKind_Null;
        
        // rjf: identifier -> keyword in special cases
        if(token.kind == TXT_TokenKind_Identifier)
        {
          read_only local_persist String8 odin_keywords[] =
          {
            str8_lit_comp("asm"),
            str8_lit_comp("auto_cast"),
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
            str8_lit_comp("in"),
            str8_lit_comp("map"),
            str8_lit_comp("matrix"),
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
          for(U64 keyword_idx = 0; keyword_idx < ArrayCount(odin_keywords); keyword_idx += 1)
          {
            if(str8_match(odin_keywords[keyword_idx], token_string, 0))
            {
              token.kind = TXT_TokenKind_Keyword;
              break;
            }
          }
        }
        
        // rjf: push
        txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        
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
  TXT_TokenArray result = txt_token_array_from_chunk_list(arena, &tokens);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
txt_init(void)
{
  Arena *arena = arena_alloc();
  txt_shared = push_array(arena, TXT_Shared, 1);
  txt_shared->arena = arena;
  txt_shared->slots_count = 1024;
  txt_shared->slots = push_array(arena, TXT_Slot, txt_shared->slots_count);
  txt_shared->stripes_count = 64;
  txt_shared->stripes = push_array(arena, TXT_Stripe, txt_shared->stripes_count);
  txt_shared->stripes_free_nodes = push_array(arena, TXT_Node *, txt_shared->stripes_count);
  for(U64 idx = 0; idx < txt_shared->stripes_count; idx += 1)
  {
    txt_shared->stripes[idx].arena = arena_alloc();
    txt_shared->stripes[idx].rw_mutex = os_rw_mutex_alloc();
    txt_shared->stripes[idx].cv = os_condition_variable_alloc();
  }
  txt_shared->fallback_slots_count = 256;
  txt_shared->fallback_stripes_count = 16;
  txt_shared->fallback_slots = push_array(arena, TXT_KeyFallbackSlot, txt_shared->fallback_slots_count);
  txt_shared->fallback_stripes = push_array(arena, TXT_Stripe, txt_shared->fallback_stripes_count);
  for(U64 idx = 0; idx < txt_shared->fallback_stripes_count; idx += 1)
  {
    txt_shared->fallback_stripes[idx].arena = arena_alloc();
    txt_shared->fallback_stripes[idx].rw_mutex = os_rw_mutex_alloc();
    txt_shared->fallback_stripes[idx].cv = os_condition_variable_alloc();
  }
  txt_shared->u2p_ring_size = KB(64);
  txt_shared->u2p_ring_base = push_array_no_zero(arena, U8, txt_shared->u2p_ring_size);
  txt_shared->u2p_ring_cv = os_condition_variable_alloc();
  txt_shared->u2p_ring_mutex = os_mutex_alloc();
  txt_shared->parse_thread_count = Clamp(1, os_logical_core_count()-1, 4);
  txt_shared->parse_threads = push_array(arena, OS_Handle, txt_shared->parse_thread_count);
  for(U64 idx = 0; idx < txt_shared->parse_thread_count; idx += 1)
  {
    txt_shared->parse_threads[idx] = os_launch_thread(txt_parse_thread__entry_point, (void *)idx, 0);
  }
  txt_shared->evictor_thread = os_launch_thread(txt_evictor_thread__entry_point, 0, 0);
}

////////////////////////////////
//~ rjf: Thread Context Initialization

internal void
txt_tctx_ensure_inited(void)
{
  if(txt_tctx == 0)
  {
    Arena *arena = arena_alloc();
    txt_tctx = push_array(arena, TXT_TCTX, 1);
    txt_tctx->arena = arena;
  }
}

////////////////////////////////
//~ rjf: User Clock

internal void
txt_user_clock_tick(void)
{
  ins_atomic_u64_inc_eval(&txt_shared->user_clock_idx);
}

internal U64
txt_user_clock_idx(void)
{
  return ins_atomic_u64_eval(&txt_shared->user_clock_idx);
}

////////////////////////////////
//~ rjf: Scoped Access

internal TXT_Scope *
txt_scope_open(void)
{
  txt_tctx_ensure_inited();
  TXT_Scope *scope = txt_tctx->free_scope;
  if(scope)
  {
    SLLStackPop(txt_tctx->free_scope);
  }
  else
  {
    scope = push_array_no_zero(txt_tctx->arena, TXT_Scope, 1);
  }
  MemoryZeroStruct(scope);
  return scope;
}

internal void
txt_scope_close(TXT_Scope *scope)
{
  for(TXT_Touch *touch = scope->top_touch, *next = 0; touch != 0; touch = next)
  {
    U128 hash = touch->hash;
    next = touch->next;
    U64 slot_idx = hash.u64[1]%txt_shared->slots_count;
    U64 stripe_idx = slot_idx%txt_shared->stripes_count;
    TXT_Slot *slot = &txt_shared->slots[slot_idx];
    TXT_Stripe *stripe = &txt_shared->stripes[stripe_idx];
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(TXT_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(hash, n->hash))
        {
          ins_atomic_u64_dec_eval(&n->scope_ref_count);
          break;
        }
      }
    }
    SLLStackPush(txt_tctx->free_touch, touch);
  }
  SLLStackPush(txt_tctx->free_scope, scope);
}

internal void
txt_scope_touch_node__stripe_r_guarded(TXT_Scope *scope, TXT_Node *node)
{
  TXT_Touch *touch = txt_tctx->free_touch;
  ins_atomic_u64_inc_eval(&node->scope_ref_count);
  ins_atomic_u64_eval_assign(&node->last_time_touched_us, os_now_microseconds());
  ins_atomic_u64_eval_assign(&node->last_user_clock_idx_touched, txt_user_clock_idx());
  if(touch != 0)
  {
    SLLStackPop(txt_tctx->free_touch);
  }
  else
  {
    touch = push_array_no_zero(txt_tctx->arena, TXT_Touch, 1);
  }
  MemoryZeroStruct(touch);
  touch->hash = node->hash;
  SLLStackPush(scope->top_touch, touch);
}

////////////////////////////////
//~ rjf: Cache Lookups

internal TXT_TextInfo
txt_text_info_from_key_hash_lang(TXT_Scope *scope, U128 key, U128 hash, TXT_LangKind lang)
{
  TXT_TextInfo info = {0};
  if(!u128_match(hash, u128_zero()))
  {
    U64 slot_idx = hash.u64[1]%txt_shared->slots_count;
    U64 stripe_idx = slot_idx%txt_shared->stripes_count;
    TXT_Slot *slot = &txt_shared->slots[slot_idx];
    TXT_Stripe *stripe = &txt_shared->stripes[stripe_idx];
    B32 found = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(TXT_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(hash, n->hash) && n->lang == lang)
        {
          MemoryCopyStruct(&info, &n->info);
          found = 1;
          txt_scope_touch_node__stripe_r_guarded(scope, n);
          break;
        }
      }
    }
    B32 node_is_new = 0;
    if(!found)
    {
      OS_MutexScopeW(stripe->rw_mutex)
      {
        TXT_Node *node = 0;
        for(TXT_Node *n = slot->first; n != 0; n = n->next)
        {
          if(u128_match(hash, n->hash) && n->lang == lang)
          {
            node = n;
            break;
          }
        }
        if(node == 0)
        {
          node = txt_shared->stripes_free_nodes[stripe_idx];
          if(node)
          {
            SLLStackPop(txt_shared->stripes_free_nodes[stripe_idx]);
          }
          else
          {
            node = push_array_no_zero(stripe->arena, TXT_Node, 1);
          }
          MemoryZeroStruct(node);
          DLLPushBack(slot->first, slot->last, node);
          node->hash = hash;
          node->lang = lang;
          node_is_new = 1;
        }
      }
    }
    if(node_is_new)
    {
      txt_u2p_enqueue_req(key, hash, lang, max_U64);
    }
    if(!found)
    {
      U128 fallback_hash = {0};
      TXT_LangKind fallback_lang = TXT_LangKind_Null;
      U64 fallback_slot_idx = key.u64[1]%txt_shared->fallback_slots_count;
      U64 fallback_stripe_idx = fallback_slot_idx%txt_shared->fallback_stripes_count;
      TXT_KeyFallbackSlot *fallback_slot = &txt_shared->fallback_slots[fallback_slot_idx];
      TXT_Stripe *fallback_stripe = &txt_shared->fallback_stripes[fallback_stripe_idx];
      OS_MutexScopeR(fallback_stripe->rw_mutex) for(TXT_KeyFallbackNode *n = fallback_slot->first; n != 0; n = n->next)
      {
        if(u128_match(key, n->key))
        {
          fallback_hash = n->hash;
          break;
        }
      }
      if(!u128_match(fallback_hash, u128_zero()))
      {
        U64 retry_slot_idx = fallback_hash.u64[1]%txt_shared->slots_count;
        U64 retry_stripe_idx = retry_slot_idx%txt_shared->stripes_count;
        TXT_Slot *retry_slot = &txt_shared->slots[retry_slot_idx];
        TXT_Stripe *retry_stripe = &txt_shared->stripes[retry_stripe_idx];
        OS_MutexScopeR(retry_stripe->rw_mutex)
        {
          for(TXT_Node *n = retry_slot->first; n != 0; n = n->next)
          {
            if(u128_match(fallback_hash, n->hash) && fallback_lang == n->lang)
            {
              MemoryCopyStruct(&info, &n->info);
              txt_scope_touch_node__stripe_r_guarded(scope, n);
              break;
            }
          }
        }
      }
    }
  }
  return info;
}

////////////////////////////////
//~ rjf: Transfer Threads

internal B32
txt_u2p_enqueue_req(U128 key, U128 hash, TXT_LangKind lang, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(txt_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = txt_shared->u2p_ring_write_pos - txt_shared->u2p_ring_read_pos;
    U64 available_size = txt_shared->u2p_ring_size - unconsumed_size;
    if(available_size >= sizeof(key)+sizeof(hash))
    {
      good = 1;
      txt_shared->u2p_ring_write_pos += ring_write_struct(txt_shared->u2p_ring_base, txt_shared->u2p_ring_size, txt_shared->u2p_ring_write_pos, &key);
      txt_shared->u2p_ring_write_pos += ring_write_struct(txt_shared->u2p_ring_base, txt_shared->u2p_ring_size, txt_shared->u2p_ring_write_pos, &hash);
      txt_shared->u2p_ring_write_pos += ring_write_struct(txt_shared->u2p_ring_base, txt_shared->u2p_ring_size, txt_shared->u2p_ring_write_pos, &lang);
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(txt_shared->u2p_ring_cv, txt_shared->u2p_ring_mutex, endt_us);
  }
  if(good)
  {
    os_condition_variable_broadcast(txt_shared->u2p_ring_cv);
  }
  return good;
}

internal void
txt_u2p_dequeue_req(U128 *key_out, U128 *hash_out, TXT_LangKind *lang_out)
{
  OS_MutexScope(txt_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = txt_shared->u2p_ring_write_pos - txt_shared->u2p_ring_read_pos;
    if(unconsumed_size >= sizeof(*key_out) + sizeof(*hash_out))
    {
      txt_shared->u2p_ring_read_pos += ring_read_struct(txt_shared->u2p_ring_base, txt_shared->u2p_ring_size, txt_shared->u2p_ring_read_pos, key_out);
      txt_shared->u2p_ring_read_pos += ring_read_struct(txt_shared->u2p_ring_base, txt_shared->u2p_ring_size, txt_shared->u2p_ring_read_pos, hash_out);
      txt_shared->u2p_ring_read_pos += ring_read_struct(txt_shared->u2p_ring_base, txt_shared->u2p_ring_size, txt_shared->u2p_ring_read_pos, lang_out);
      break;
    }
    os_condition_variable_wait(txt_shared->u2p_ring_cv, txt_shared->u2p_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(txt_shared->u2p_ring_cv);
}

internal void
txt_parse_thread__entry_point(void *p)
{
  for(;;)
  {
    HS_Scope *scope = hs_scope_open();
    
    //- rjf: get next key
    U128 key = {0};
    U128 hash = {0};
    TXT_LangKind lang = TXT_LangKind_Null;
    txt_u2p_dequeue_req(&key, &hash, &lang);
    
    //- rjf: unpack hash
    U64 slot_idx = hash.u64[1]%txt_shared->slots_count;
    U64 stripe_idx = slot_idx%txt_shared->stripes_count;
    TXT_Slot *slot = &txt_shared->slots[slot_idx];
    TXT_Stripe *stripe = &txt_shared->stripes[stripe_idx];
    
    //- rjf: take task
    B32 got_task = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(TXT_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, hash))
        {
          got_task = !ins_atomic_u32_eval_cond_assign(&n->is_working, 1, 0);
          break;
        }
      }
    }
    
    //- rjf: hash -> data
    String8 data = {0};
    if(got_task)
    {
      data = hs_data_from_hash(scope, hash);
    }
    
    //- rjf: data -> text info
    Arena *info_arena = 0;
    TXT_TextInfo info = {0};
    if(got_task && data.size != 0)
    {
      info_arena = arena_alloc();
      
      //- rjf: detect line end kind
      TXT_LineEndKind line_end_kind = TXT_LineEndKind_Null;
      {
        U64 lf_count = 0;
        U64 cr_count = 0;
        for(U64 idx = 0; idx < data.size && idx < 1024; idx += 1)
        {
          if(data.str[idx] == '\r')
          {
            cr_count += 1;
          }
          if(data.str[idx] == '\n')
          {
            lf_count += 1;
          }
        }
        if(cr_count >= lf_count/2 && lf_count >= 1)
        {
          line_end_kind = TXT_LineEndKind_CRLF;
        }
        else if(lf_count >= 1)
        {
          line_end_kind = TXT_LineEndKind_LF;
        }
        info.line_end_kind = line_end_kind;
      }
      
      //- rjf: count # of lines
      U64 line_count = 1;
      U64 byte_process_start_idx = 0;
      for(U64 idx = 0; idx < data.size; idx += 1)
      {
        if(data.str[idx] == '\n' || data.str[idx] == '\r')
        {
          line_count += 1;
          if(data.str[idx] == '\r')
          {
            idx += 1;
          }
        }
      }
      
      //- rjf: allocate & store line ranges
      info.lines_count = line_count;
      info.lines_ranges = push_array_no_zero(info_arena, Rng1U64, info.lines_count);
      U64 line_idx = 0;
      U64 line_start_idx = 0;
      for(U64 idx = 0; idx <= data.size; idx += 1)
      {
        if(idx == data.size || data.str[idx] == '\n' || data.str[idx] == '\r')
        {
          Rng1U64 line_range = r1u64(line_start_idx, idx);
          U64 line_size = dim_1u64(line_range);
          info.lines_ranges[line_idx] = line_range;
          info.lines_max_size = Max(info.lines_max_size, line_size);
          line_idx += 1;
          line_start_idx = idx+1;
          if(idx < data.size && data.str[idx] == '\r')
          {
            line_start_idx += 1;
            idx += 1;
          }
        }
      }
      
      //- rjf: lang -> lex function
      TXT_LangLexFunctionType *lex_function = txt_lex_function_from_lang_kind(lang);
      
      //- rjf: lex function * data -> tokens
      TXT_TokenArray tokens = {0};
      if(lex_function != 0)
      {
        tokens = lex_function(info_arena, 0, data);
      }
      info.tokens = tokens;
    }
    
    //- rjf: commit results to cache
    if(got_task) OS_MutexScopeW(stripe->rw_mutex)
    {
      for(TXT_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, hash))
        {
          n->arena = info_arena;
          MemoryCopyStruct(&n->info, &info);
          ins_atomic_u32_eval_assign(&n->is_working, 0);
          ins_atomic_u64_inc_eval(&n->load_count);
          break;
        }
      }
    }
    
    //- rjf: commit this key/hash pair to fallback cache
    if(got_task && !u128_match(key, u128_zero()) && !u128_match(hash, u128_zero()))
    {
      U64 fallback_slot_idx = key.u64[1]%txt_shared->fallback_slots_count;
      U64 fallback_stripe_idx = fallback_slot_idx%txt_shared->fallback_stripes_count;
      TXT_KeyFallbackSlot *fallback_slot = &txt_shared->fallback_slots[fallback_slot_idx];
      TXT_Stripe *fallback_stripe = &txt_shared->fallback_stripes[fallback_stripe_idx];
      OS_MutexScopeW(fallback_stripe->rw_mutex)
      {
        TXT_KeyFallbackNode *node = 0;
        for(TXT_KeyFallbackNode *n = fallback_slot->first; n != 0; n = n->next)
        {
          if(u128_match(n->key, key))
          {
            node = n;
            break;
          }
        }
        if(node == 0)
        {
          node = push_array(fallback_stripe->arena, TXT_KeyFallbackNode, 1);
          SLLQueuePush(fallback_slot->first, fallback_slot->last, node);
        }
        node->key = key;
        node->hash = hash;
      }
    }
    
    hs_scope_close(scope);
  }
}

////////////////////////////////
//~ rjf: Evictor Threads

internal void
txt_evictor_thread__entry_point(void *p)
{
  for(;;)
  {
    U64 check_time_us = os_now_microseconds();
    U64 check_time_user_clocks = txt_user_clock_idx();
    U64 evict_threshold_us = 10*1000000;
    U64 evict_threshold_user_clocks = 10;
    for(U64 slot_idx = 0; slot_idx < txt_shared->slots_count; slot_idx += 1)
    {
      U64 stripe_idx = slot_idx%txt_shared->stripes_count;
      TXT_Slot *slot = &txt_shared->slots[slot_idx];
      TXT_Stripe *stripe = &txt_shared->stripes[stripe_idx];
      B32 slot_has_work = 0;
      OS_MutexScopeR(stripe->rw_mutex)
      {
        for(TXT_Node *n = slot->first; n != 0; n = n->next)
        {
          if(n->scope_ref_count == 0 &&
             n->last_time_touched_us+evict_threshold_us <= check_time_us &&
             n->last_user_clock_idx_touched+evict_threshold_user_clocks <= check_time_user_clocks &&
             n->load_count != 0 &&
             n->is_working == 0)
          {
            slot_has_work = 1;
            break;
          }
        }
      }
      if(slot_has_work) OS_MutexScopeW(stripe->rw_mutex)
      {
        for(TXT_Node *n = slot->first, *next = 0; n != 0; n = next)
        {
          next = n->next;
          if(n->scope_ref_count == 0 &&
             n->last_time_touched_us+evict_threshold_us <= check_time_us &&
             n->last_user_clock_idx_touched+evict_threshold_user_clocks <= check_time_user_clocks &&
             n->load_count != 0 &&
             n->is_working == 0)
          {
            DLLRemove(slot->first, slot->last, n);
            if(n->arena != 0)
            {
              arena_release(n->arena);
            }
            SLLStackPush(txt_shared->stripes_free_nodes[stripe_idx], n);
          }
        }
      }
      os_sleep_milliseconds(5);
    }
    os_sleep_milliseconds(1000);
  }
}
