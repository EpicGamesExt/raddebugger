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
          str8_match(extension, str8_lit("ixx"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("cxxm"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("c++m"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("ccm"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("cppm"), StringMatchFlag_CaseInsensitive) ||
          str8_match(extension, str8_lit("mpp"), StringMatchFlag_CaseInsensitive) ||
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
  else if(str8_match(extension, str8_lit("jai"), StringMatchFlag_CaseInsensitive))
  {
    kind = TXT_LangKind_Jai;
  }
  else if(str8_match(extension, str8_lit("zig"), StringMatchFlag_CaseInsensitive))
  {
    kind = TXT_LangKind_Zig;
  }
  return kind;
}

internal String8
txt_extension_from_lang_kind(TXT_LangKind kind)
{
  String8 result = {0};
  switch(kind)
  {
    case TXT_LangKind_Null:
    case TXT_LangKind_COUNT:
    case TXT_LangKind_DisasmX64Intel:
    {}break;
    case TXT_LangKind_C:               {result = str8_lit("c");}break;
    case TXT_LangKind_CPlusPlus:       {result = str8_lit("cpp");}break;
    case TXT_LangKind_Odin:            {result = str8_lit("odin");}break;
    case TXT_LangKind_Jai:             {result = str8_lit("jai");}break;
    case TXT_LangKind_Zig:             {result = str8_lit("zig");}break;
  }
  return result;
}

internal TXT_LangKind
txt_lang_kind_from_architecture(Architecture arch)
{
  TXT_LangKind kind = TXT_LangKind_Null;
  switch(arch)
  {
    default:{}break;
    case Architecture_x64:{kind = TXT_LangKind_DisasmX64Intel;}break;
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
    case TXT_LangKind_C:             {fn = txt_token_array_from_string__c_cpp;}break;
    case TXT_LangKind_CPlusPlus:     {fn = txt_token_array_from_string__c_cpp;}break;
    case TXT_LangKind_Odin:          {fn = txt_token_array_from_string__odin;}break;
    case TXT_LangKind_Jai:           {fn = txt_token_array_from_string__jai;}break;
    case TXT_LangKind_Zig:           {fn = txt_token_array_from_string__zig;}break;
    case TXT_LangKind_DisasmX64Intel:{fn = txt_token_array_from_string__disasm_x64_intel;}break;
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
          txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
        
        // rjf: split symbols by maximum-munch-rule
        else if(token.kind == TXT_TokenKind_Symbol)
        {
          read_only local_persist String8 c_cpp_multichar_symbol_strings[] =
          {
            str8_lit_comp("<<"),
            str8_lit_comp(">>"),
            str8_lit_comp("<="),
            str8_lit_comp(">="),
            str8_lit_comp("=="),
            str8_lit_comp("!="),
            str8_lit_comp("&&"),
            str8_lit_comp("||"),
            str8_lit_comp("|="),
            str8_lit_comp("&="),
            str8_lit_comp("^="),
            str8_lit_comp("~="),
            str8_lit_comp("+="),
            str8_lit_comp("-="),
            str8_lit_comp("*="),
            str8_lit_comp("/="),
            str8_lit_comp("%="),
            str8_lit_comp("<<="),
            str8_lit_comp(">>="),
            str8_lit_comp("->"),
          };
          String8 token_string = str8_substr(string, r1u64(active_token_start_idx, idx+ender_pad));
          for(U64 off = 0, next_off = token_string.size; off < token_string.size; off = next_off)
          {
            B32 found = 0;
            for(U64 idx = 0; idx < ArrayCount(c_cpp_multichar_symbol_strings); idx += 1)
            {
              if(str8_match(str8_substr(string, r1u64(off, off+c_cpp_multichar_symbol_strings[idx].size)),
                            c_cpp_multichar_symbol_strings[idx],
                            0))
              {
                found = 1;
                next_off = off + c_cpp_multichar_symbol_strings[idx].size;
                TXT_Token token = {TXT_TokenKind_Symbol, r1u64(active_token_start_idx+off, active_token_start_idx+next_off)};
                txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
                break;
              }
            }
            if(!found)
            {
              next_off = off+1;
              TXT_Token token = {TXT_TokenKind_Symbol, r1u64(active_token_start_idx+off, active_token_start_idx+next_off)};
              txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
            }
          }
        }
        
        // rjf: all other tokens
        else
        {
          txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
        
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
            str8_lit_comp("align_of"),
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
            str8_lit_comp("size_of"),
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
          txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
        
        // rjf: split symbols by maximum-munch-rule
        else if(token.kind == TXT_TokenKind_Symbol)
        {
          read_only local_persist String8 odin_multichar_symbol_strings[] =
          {
            str8_lit_comp("<<"),
            str8_lit_comp(">>"),
            str8_lit_comp("<="),
            str8_lit_comp(">="),
            str8_lit_comp("=="),
            str8_lit_comp("!="),
            str8_lit_comp("&&"),
            str8_lit_comp("||"),
            str8_lit_comp("|="),
            str8_lit_comp("&="),
            str8_lit_comp("^="),
            str8_lit_comp("~="),
            str8_lit_comp("+="),
            str8_lit_comp("-="),
            str8_lit_comp("*="),
            str8_lit_comp("/="),
            str8_lit_comp("%="),
            str8_lit_comp("<<="),
            str8_lit_comp(">>="),
            str8_lit_comp("->"),
          };
          String8 token_string = str8_substr(string, r1u64(active_token_start_idx, idx+ender_pad));
          for(U64 off = 0, next_off = token_string.size; off < token_string.size; off = next_off)
          {
            B32 found = 0;
            for(U64 idx = 0; idx < ArrayCount(odin_multichar_symbol_strings); idx += 1)
            {
              if(str8_match(str8_substr(string, r1u64(off, off+odin_multichar_symbol_strings[idx].size)),
                            odin_multichar_symbol_strings[idx],
                            0))
              {
                found = 1;
                next_off = off + odin_multichar_symbol_strings[idx].size;
                TXT_Token token = {TXT_TokenKind_Symbol, r1u64(active_token_start_idx+off, active_token_start_idx+next_off)};
                txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
                break;
              }
            }
            if(!found)
            {
              next_off = off+1;
              TXT_Token token = {TXT_TokenKind_Symbol, r1u64(active_token_start_idx+off, active_token_start_idx+next_off)};
              txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
            }
          }
        }
        
        // rjf: all other tokens
        else
        {
          txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
        
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
txt_token_array_from_string__jai(Arena *arena, U64 *bytes_processed_counter, String8 string)
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
          read_only local_persist String8 jai_keywords[] =
          {
            str8_lit_comp("bool"),
            str8_lit_comp("true"),
            str8_lit_comp("false"),
            str8_lit_comp("int"),
            str8_lit_comp("s8"),
            str8_lit_comp("u8"),
            str8_lit_comp("s16"),
            str8_lit_comp("u16"),
            str8_lit_comp("s32"),
            str8_lit_comp("u32"),
            str8_lit_comp("s64"),
            str8_lit_comp("u64"),
            str8_lit_comp("s128"),
            str8_lit_comp("u128"),
            str8_lit_comp("float"),
            str8_lit_comp("float32"),
            str8_lit_comp("float64"),
            str8_lit_comp("void"),
            str8_lit_comp("enum"),
            str8_lit_comp("enum_flags"),
            str8_lit_comp("size_of"),
            str8_lit_comp("string"),
            str8_lit_comp("type_of"),
            str8_lit_comp("cast"),
            str8_lit_comp("if"),
            str8_lit_comp("ifs"),
            str8_lit_comp("then"),
            str8_lit_comp("else"),
            str8_lit_comp("case"),
            str8_lit_comp("for"),
            str8_lit_comp("while"),
            str8_lit_comp("break"),
            str8_lit_comp("continue"),
            str8_lit_comp("remove"),
            str8_lit_comp("return"),
            str8_lit_comp("inline"),
            str8_lit_comp("null"),
            str8_lit_comp("defer"),
            str8_lit_comp("xx"),
          };
          String8 token_string = str8_substr(string, r1u64(active_token_start_idx, idx+ender_pad));
          for(U64 keyword_idx = 0; keyword_idx < ArrayCount(jai_keywords); keyword_idx += 1)
          {
            if(str8_match(jai_keywords[keyword_idx], token_string, 0))
            {
              token.kind = TXT_TokenKind_Keyword;
              break;
            }
          }
          txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
        
        // rjf: split symbols by maximum-munch-rule
        else if(token.kind == TXT_TokenKind_Symbol)
        {
          read_only local_persist String8 jai_multichar_symbol_strings[] =
          {
            str8_lit_comp("<<"),
            str8_lit_comp(">>"),
            str8_lit_comp("<="),
            str8_lit_comp(">="),
            str8_lit_comp("=="),
            str8_lit_comp("!="),
            str8_lit_comp("&&"),
            str8_lit_comp("||"),
            str8_lit_comp("|="),
            str8_lit_comp("&="),
            str8_lit_comp("^="),
            str8_lit_comp("~="),
            str8_lit_comp("+="),
            str8_lit_comp("-="),
            str8_lit_comp("*="),
            str8_lit_comp("/="),
            str8_lit_comp("%="),
            str8_lit_comp("<<="),
            str8_lit_comp(">>="),
            str8_lit_comp("->"),
          };
          String8 token_string = str8_substr(string, r1u64(active_token_start_idx, idx+ender_pad));
          for(U64 off = 0, next_off = token_string.size; off < token_string.size; off = next_off)
          {
            B32 found = 0;
            for(U64 idx = 0; idx < ArrayCount(jai_multichar_symbol_strings); idx += 1)
            {
              if(str8_match(str8_substr(string, r1u64(off, off+jai_multichar_symbol_strings[idx].size)),
                            jai_multichar_symbol_strings[idx],
                            0))
              {
                found = 1;
                next_off = off + jai_multichar_symbol_strings[idx].size;
                TXT_Token token = {TXT_TokenKind_Symbol, r1u64(active_token_start_idx+off, active_token_start_idx+next_off)};
                txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
                break;
              }
            }
            if(!found)
            {
              next_off = off+1;
              TXT_Token token = {TXT_TokenKind_Symbol, r1u64(active_token_start_idx+off, active_token_start_idx+next_off)};
              txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
            }
          }
        }
        
        // rjf: all other tokens
        else
        {
          txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
        
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
txt_token_array_from_string__zig(Arena *arena, U64 *bytes_processed_counter, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: generate token list
  TXT_TokenChunkList tokens = {0};
  {
    B32 string_is_char = 0;
    B32 string_is_line = 0;
    TXT_TokenKind active_token_kind = TXT_TokenKind_Null;
    U64 active_token_start_idx = 0;
    B32 escaped = 0;
    B32 next_escaped = 0;
    U64 byte_process_start_idx = 0;
    for(U64 idx = 0; idx <= string.size;)
    {
      U8 byte        = (idx+0 < string.size) ? (string.str[idx+0]) : 0;
      U8 next_byte   = (idx+1 < string.size) ? (string.str[idx+1]) : 0;
      
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
                char_is_alpha(byte))             { active_token_kind = TXT_TokenKind_Identifier; }
        else if(char_is_digit(byte, 10) ||
                (byte == '.' &&
                 char_is_digit(next_byte, 10)))  { active_token_kind = TXT_TokenKind_Numeric; }
        else if(byte == '"')                     { active_token_kind = TXT_TokenKind_String; string_is_char = 0; }
        else if(byte == '\'')                    { active_token_kind = TXT_TokenKind_String; string_is_char = 1; }
        else if(byte == '\\' &&
                next_byte == '\\')               { active_token_kind = TXT_TokenKind_String; string_is_line = 1; }
        else if(byte == '/' && next_byte == '/') { active_token_kind = TXT_TokenKind_Comment; }
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
                byte == '?' || byte == '|' ||
                byte == 'c')                     { active_token_kind = TXT_TokenKind_Symbol; }
        
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
            if (string_is_line)
            {
              ender_found = (!escaped && (byte == '\r' || byte == '\n'));
            }
            else
            {
              ender_found = (!escaped && ((!string_is_char && byte == '"') || (string_is_char && byte == '\'')));
              ender_pad += 1;
            }
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
                           byte != '?' && byte != '|' &&
                           byte != 'c');
          }break;
          case TXT_TokenKind_Comment:
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
          read_only local_persist String8 zig_keywords[] =
          {
            str8_lit_comp("addrspace"),
            str8_lit_comp("align"),
            str8_lit_comp("allowzero"),
            str8_lit_comp("and"),
            str8_lit_comp("anyframe"),
            str8_lit_comp("anytype"),
            str8_lit_comp("asm"),
            str8_lit_comp("async"),
            str8_lit_comp("await"),
            str8_lit_comp("break"),
            str8_lit_comp("callconv"),
            str8_lit_comp("catch"),
            str8_lit_comp("comptime"),
            str8_lit_comp("const"),
            str8_lit_comp("continue"),
            str8_lit_comp("defer"),
            str8_lit_comp("else"),
            str8_lit_comp("enum"),
            str8_lit_comp("errdefer"),
            str8_lit_comp("error"),
            str8_lit_comp("export"),
            str8_lit_comp("extern"),
            str8_lit_comp("fn"),
            str8_lit_comp("for"),
            str8_lit_comp("if"),
            str8_lit_comp("inline"),
            str8_lit_comp("noalias"),
            str8_lit_comp("nosuspend"),
            str8_lit_comp("noinline"),
            str8_lit_comp("opaque"),
            str8_lit_comp("or"),
            str8_lit_comp("orelse"),
            str8_lit_comp("packed"),
            str8_lit_comp("pub"),
            str8_lit_comp("resume"),
            str8_lit_comp("return"),
            str8_lit_comp("linksection"),
            str8_lit_comp("struct"),
            str8_lit_comp("suspend"),
            str8_lit_comp("switch"),
            str8_lit_comp("test"),
            str8_lit_comp("threadlocal"),
            str8_lit_comp("try"),
            str8_lit_comp("union"),
            str8_lit_comp("unreachable"),
            str8_lit_comp("usingnamespace"),
            str8_lit_comp("var"),
            str8_lit_comp("volatile"),
            str8_lit_comp("while"),
          };
          String8 token_string = str8_substr(string, r1u64(active_token_start_idx, idx+ender_pad));
          for(U64 keyword_idx = 0; keyword_idx < ArrayCount(zig_keywords); keyword_idx += 1)
          {
            if(str8_match(zig_keywords[keyword_idx], token_string, 0))
            {
              token.kind = TXT_TokenKind_Keyword;
              break;
            }
          }
          txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
        
        // rjf: split symbols by maximum-munch-rule
        else if(token.kind == TXT_TokenKind_Symbol)
        {
          read_only local_persist String8 zig_multichar_symbol_strings[] =
          {
            str8_lit_comp("<<"),
            str8_lit_comp(">>"),
            str8_lit_comp("<="),
            str8_lit_comp(">="),
            str8_lit_comp("=="),
            str8_lit_comp("!="),
            str8_lit_comp("&&"),
            str8_lit_comp("||"),
            str8_lit_comp("|="),
            str8_lit_comp("&="),
            str8_lit_comp("^="),
            str8_lit_comp("~="),
            str8_lit_comp("+="),
            str8_lit_comp("-="),
            str8_lit_comp("*="),
            str8_lit_comp("/="),
            str8_lit_comp("%="),
            str8_lit_comp("<<="),
            str8_lit_comp(">>="),
            str8_lit_comp("->"),
          };
          String8 token_string = str8_substr(string, r1u64(active_token_start_idx, idx+ender_pad));
          for(U64 off = 0, next_off = token_string.size; off < token_string.size; off = next_off)
          {
            B32 found = 0;
            for(U64 idx = 0; idx < ArrayCount(zig_multichar_symbol_strings); idx += 1)
            {
              if(str8_match(str8_substr(string, r1u64(off, off+zig_multichar_symbol_strings[idx].size)),
                            zig_multichar_symbol_strings[idx],
                            0))
              {
                found = 1;
                next_off = off + zig_multichar_symbol_strings[idx].size;
                TXT_Token token = {TXT_TokenKind_Symbol, r1u64(active_token_start_idx+off, active_token_start_idx+next_off)};
                txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
                break;
              }
            }
            if(!found)
            {
              next_off = off+1;
              TXT_Token token = {TXT_TokenKind_Symbol, r1u64(active_token_start_idx+off, active_token_start_idx+next_off)};
              txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
            }
          }
        }
        
        // rjf: all other tokens
        else
        {
          txt_token_chunk_list_push(scratch.arena, &tokens, 4096, &token);
        }
        
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
txt_token_array_from_string__disasm_x64_intel(Arena *arena, U64 *bytes_processed_counter, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: parse tokens
  TXT_TokenChunkList tokens = {0};
  {
    TXT_TokenKind active_token_kind = TXT_TokenKind_Null;
    U64 active_token_start_off = 0;
    U64 off = 0;
    B32 escaped = 0;
    B32 string_is_char = 0;
    S32 brace_nest = 0;
    S32 paren_nest = 0;
    for(U64 advance = 0; off <= string.size; off += advance)
    {
      U8 byte      = (off+0 < string.size) ? string.str[off+0] : 0;
      U8 next_byte = (off+1 < string.size) ? string.str[off+1] : 0;
      B32 ender_found = 0;
      advance = (active_token_kind != TXT_TokenKind_Null ? 1 : 0);
      if(off == string.size && active_token_kind != TXT_TokenKind_Null)
      {
        ender_found = 1;
        advance = 1;
      }
      switch(active_token_kind)
      {
        default:
        case TXT_TokenKind_Null:
        {
          if(byte == ' ' || byte == '\t' || byte == '\v' || byte == '\f' || byte == '\r' || byte == '\n')
          {
            active_token_start_off = off;
            active_token_kind = TXT_TokenKind_Whitespace;
            advance = 1;
          }
          else if(byte == '>' && brace_nest == 0 && paren_nest == 0)
          {
            active_token_start_off = off;
            active_token_kind = TXT_TokenKind_Comment;
            advance = 1;
          }
          else if(('a' <= byte && byte <= 'z') || ('A' <= byte && byte <= 'Z') || byte == '_')
          {
            active_token_start_off = off;
            active_token_kind = TXT_TokenKind_Keyword;
            advance = 1;
          }
          else if(byte == '\'')
          {
            active_token_start_off = off;
            active_token_kind = TXT_TokenKind_String;
            advance = 1;
            string_is_char = 1;
          }
          else if(byte == '"')
          {
            active_token_start_off = off;
            active_token_kind = TXT_TokenKind_String;
            advance = 1;
            string_is_char = 0;
          }
          else if(('0' <= byte && byte <= '9') || (byte == '.' && '0' <= next_byte && next_byte <= '9'))
          {
            active_token_start_off = off;
            active_token_kind = TXT_TokenKind_Numeric;
            advance = 1;
          }
          else if(byte == '~' || byte == '!' || byte == '%' || byte == '^' ||
                  byte == '&' || byte == '*' || byte == '(' || byte == ')' ||
                  byte == '-' || byte == '=' || byte == '+' || byte == '[' ||
                  byte == ']' || byte == '{' || byte == '}' || byte == ';' ||
                  byte == ':' || byte == '?' || byte == '/' || byte == '<' ||
                  byte == '>' || byte == ',' || byte == '.')
          {
            active_token_start_off = off;
            active_token_kind = TXT_TokenKind_Symbol;
            advance = 1;
            if(byte == '{')
            {
              brace_nest += 1;
            }
            else if(byte == '}')
            {
              brace_nest -= 1;
            }
            if(byte == '(')
            {
              paren_nest += 1;
            }
            else if(byte == ')')
            {
              paren_nest -= 1;
            }
          }
          else
          {
            active_token_start_off = off;
            active_token_kind = TXT_TokenKind_Error;
            advance = 1;
          }
        }break;
        case TXT_TokenKind_Whitespace:
        if(byte != ' ' && byte != '\t' && byte != '\v' && byte != '\f')
        {
          ender_found = 1;
          advance = 0;
        }break;
        case TXT_TokenKind_Keyword:
        if((byte < 'a' || 'z' < byte) && (byte < 'A' || 'Z' < byte) && (byte < '0' || '9' < byte) && byte != '_')
        {
          ender_found = 1;
          advance = 0;
        }break;
        case TXT_TokenKind_String:
        {
          U8 ender_byte = string_is_char ? '\'' : '"';
          if(!escaped && byte == ender_byte)
          {
            ender_found = 1;
            advance = 1;
          }
          else if(escaped)
          {
            escaped = 0;
            advance = 1;
          }
          else if(byte == '\\')
          {
            escaped = 1;
            advance = 1;
          }
          else
          {
            U8 byte_class = utf8_class[byte>>3];
            if(byte_class > 1)
            {
              advance = (U64)byte_class;
            }
          }
        }break;
        case TXT_TokenKind_Numeric:
        if((byte < 'a' || 'z' < byte) && (byte < 'A' || 'Z' < byte) && (byte < '0' || '9' < byte) && byte != '.')
        {
          ender_found = 1;
          advance = 0;
        }break;
        case TXT_TokenKind_Symbol:
        if(1)
        {
          // NOTE(rjf): avoiding maximum munch rule for now
          ender_found = 1;
          advance = 0;
        }
        else if(byte != '~' && byte != '!' && byte != '#' && byte != '%' &&
                byte != '^' && byte != '&' && byte != '*' && byte != '(' &&
                byte != ')' && byte != '-' && byte != '=' && byte != '+' &&
                byte != '[' && byte != ']' && byte != '{' && byte != '}' &&
                byte != ';' && byte != ':' && byte != '?' && byte != '/' &&
                byte != '<' && byte != '>' && byte != ',' && byte != '.')
        {
          ender_found = 1;
          advance = 0;
        }break;
        case TXT_TokenKind_Error:
        {
          ender_found = 1;
          advance = 0;
        }break;
        case TXT_TokenKind_Comment:
        if(byte == '\n')
        {
          ender_found = 1;
          advance = 1;
        }break;
      }
      if(ender_found != 0)
      {
        if(brace_nest != 0 && active_token_kind == TXT_TokenKind_Keyword)
        {
          active_token_kind = TXT_TokenKind_Numeric;
        }
        if(paren_nest != 0 && active_token_kind == TXT_TokenKind_Keyword)
        {
          active_token_kind = TXT_TokenKind_Identifier;
        }
        TXT_Token token = {active_token_kind, r1u64(active_token_start_off, off+advance)};
        txt_token_chunk_list_push(arena, &tokens, 1024, &token);
        active_token_kind = TXT_TokenKind_Null;
        active_token_start_off = token.range.max;
      }
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
  txt_shared->stripes_count = Min(txt_shared->slots_count, os_get_system_info()->logical_processor_count);
  txt_shared->stripes = push_array(arena, TXT_Stripe, txt_shared->stripes_count);
  txt_shared->stripes_free_nodes = push_array(arena, TXT_Node *, txt_shared->stripes_count);
  for(U64 idx = 0; idx < txt_shared->stripes_count; idx += 1)
  {
    txt_shared->stripes[idx].arena = arena_alloc();
    txt_shared->stripes[idx].rw_mutex = os_rw_mutex_alloc();
    txt_shared->stripes[idx].cv = os_condition_variable_alloc();
  }
  txt_shared->u2p_ring_size = KB(64);
  txt_shared->u2p_ring_base = push_array_no_zero(arena, U8, txt_shared->u2p_ring_size);
  txt_shared->u2p_ring_cv = os_condition_variable_alloc();
  txt_shared->u2p_ring_mutex = os_mutex_alloc();
  txt_shared->parse_thread_count = Clamp(1, os_get_system_info()->logical_processor_count-1, 4);
  txt_shared->parse_threads = push_array(arena, OS_Handle, txt_shared->parse_thread_count);
  for(U64 idx = 0; idx < txt_shared->parse_thread_count; idx += 1)
  {
    txt_shared->parse_threads[idx] = os_thread_launch(txt_parse_thread__entry_point, (void *)idx, 0);
  }
  txt_shared->evictor_thread = os_thread_launch(txt_evictor_thread__entry_point, 0, 0);
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
        if(u128_match(hash, n->hash) && touch->lang == n->lang)
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
  touch->lang = node->lang;
  SLLStackPush(scope->top_touch, touch);
}

////////////////////////////////
//~ rjf: Cache Lookups

internal TXT_TextInfo
txt_text_info_from_hash_lang(TXT_Scope *scope, U128 hash, TXT_LangKind lang)
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
          info.bytes_processed = ins_atomic_u64_eval(&n->info.bytes_processed);
          info.bytes_to_process = ins_atomic_u64_eval(&n->info.bytes_to_process);
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
      txt_u2p_enqueue_req(hash, lang, max_U64);
    }
  }
  return info;
}

internal TXT_TextInfo
txt_text_info_from_key_lang(TXT_Scope *scope, U128 key, TXT_LangKind lang, U128 *hash_out)
{
  TXT_TextInfo result = {0};
  for(U64 rewind_idx = 0; rewind_idx < 2; rewind_idx += 1)
  {
    U128 hash = hs_hash_from_key(key, rewind_idx);
    result = txt_text_info_from_hash_lang(scope, hash, lang);
    if(result.lines_count != 0)
    {
      if(hash_out)
      {
        *hash_out = hash;
      }
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Text Info Extractor Helpers

internal U64
txt_off_from_info_pt(TXT_TextInfo *info, TxtPt pt)
{
  U64 off = 0;
  if(1 <= pt.line && pt.line <= info->lines_count)
  {
    Rng1U64 line_range = info->lines_ranges[pt.line-1];
    off = line_range.min + (pt.column-1);
  }
  return off;
}

internal TxtPt
txt_pt_from_info_off__linear_scan(TXT_TextInfo *info, U64 off)
{
  TxtPt pt = {0};
  {
    for(U64 line_idx = 0; line_idx < info->lines_count; line_idx += 1)
    {
      if(contains_1u64(info->lines_ranges[line_idx], off))
      {
        pt.line = (S64)line_idx + 1;
        pt.column = (S64)(off - info->lines_ranges[line_idx].min) + 1;
      }
    }
  }
  return pt;
}

internal TXT_TokenArray
txt_token_array_from_info_line_num__linear_scan(TXT_TextInfo *info, S64 line_num)
{
  TXT_TokenArray line_tokens = {0};
  if(1 <= line_num && line_num <= info->lines_count)
  {
    Rng1U64 line_range = info->lines_ranges[line_num-1];
    for(U64 token_idx = 0; token_idx < info->tokens.count; token_idx += 1)
    {
      Rng1U64 token_range = info->tokens.v[token_idx].range;
      Rng1U64 token_x_line = intersect_1u64(token_range, line_range);
      if(token_x_line.max > token_x_line.min)
      {
        if(line_tokens.v == 0)
        {
          line_tokens.v = info->tokens.v+token_idx;
        }
        line_tokens.count += 1;
      }
      else if(line_tokens.v != 0)
      {
        break;
      }
    }
  }
  return line_tokens;
}

internal Rng1U64
txt_expr_off_range_from_line_off_range_string_tokens(U64 off, Rng1U64 line_range, String8 line_text, TXT_TokenArray *line_tokens)
{
  Rng1U64 result = {0};
  Temp scratch = scratch_begin(0, 0);
  {
    // rjf: unpack line info
    TXT_Token *line_tokens_first = line_tokens->v;
    TXT_Token *line_tokens_opl = line_tokens->v+line_tokens->count;
    
    // rjf: find token containing `off`
    TXT_Token *pt_token = 0;
    for(TXT_Token *token = line_tokens_first;
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
          case TXT_TokenKind_Symbol:
          {
            token_ender = (str8_match(token_string, str8_lit("]"), 0));
          }break;
          case TXT_TokenKind_Identifier:
          case TXT_TokenKind_Keyword:
          case TXT_TokenKind_String:
          case TXT_TokenKind_Meta:
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
      for(TXT_Token *wb_token = pt_token;
          wb_token >= line_tokens_first && walkback_done == 0;
          wb_token -= 1)
      {
        Rng1U64 wb_token_range_clamped = intersect_1u64(line_range, wb_token->range);
        String8 wb_token_string = str8_substr(line_text, r1u64(wb_token_range_clamped.min - line_range.min, wb_token_range_clamped.max - line_range.min));
        B32 include_wb_token = 0;
        switch(wb_token->kind)
        {
          default:{}break;
          case TXT_TokenKind_Symbol:
          {
            B32 is_scope_resolution = str8_match(wb_token_string, str8_lit("::"), 0);
            B32 is_dot = str8_match(wb_token_string, str8_lit("."), 0);
            B32 is_arrow = str8_match(wb_token_string, str8_lit("->"), 0);
            B32 is_open_bracket = str8_match(wb_token_string, str8_lit("["), 0);
            B32 is_close_bracket = str8_match(wb_token_string, str8_lit("]"), 0);
            nest -= !!(is_open_bracket);
            nest += !!(is_close_bracket);
            if(is_scope_resolution ||
               is_dot ||
               is_arrow ||
               is_open_bracket||
               is_close_bracket)
            {
              include_wb_token = 1;
            }
          }break;
          case TXT_TokenKind_Identifier:
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

internal Rng1U64
txt_expr_off_range_from_info_data_pt(TXT_TextInfo *info, String8 data, TxtPt pt)
{
  Rng1U64 result = {0};
  Temp scratch = scratch_begin(0, 0);
  if(1 <= pt.line && pt.line <= info->lines_count)
  {
    // rjf: unpack line info
    Rng1U64 line_range = info->lines_ranges[pt.line-1];
    String8 line_text = str8_substr(data, line_range);
    TXT_LineTokensSlice line_tokens_slice = txt_line_tokens_slice_from_info_data_line_range(scratch.arena, info, data, r1s64(pt.line, pt.line));
    TXT_TokenArray line_tokens = line_tokens_slice.line_tokens[0];
    TXT_Token *line_tokens_first = line_tokens.v;
    TXT_Token *line_tokens_opl = line_tokens.v+line_tokens.count;
    U64 pt_off = line_range.min + (pt.column-1);
    
    // rjf: grab offset range of expression
    result = txt_expr_off_range_from_line_off_range_string_tokens(pt_off, line_range, line_text, &line_tokens);
  }
  scratch_end(scratch);
  return result;
}

internal String8
txt_string_from_info_data_txt_rng(TXT_TextInfo *info, String8 data, TxtRng rng)
{
  Rng1U64 rng_off = r1u64(txt_off_from_info_pt(info, rng.min), txt_off_from_info_pt(info, rng.max));
  String8 result = str8_substr(data, rng_off);
  return result;
}

internal String8
txt_string_from_info_data_line_num(TXT_TextInfo *info, String8 data, S64 line_num)
{
  String8 result = {0};
  if(1 <= line_num && line_num <= info->lines_count)
  {
    result = str8_substr(data, info->lines_ranges[line_num-1]);
  }
  return result;
}

internal TXT_LineTokensSlice
txt_line_tokens_slice_from_info_data_line_range(Arena *arena, TXT_TextInfo *info, String8 data, Rng1S64 line_range)
{
  TXT_LineTokensSlice result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  if(info->lines_count != 0)
  {
    Rng1S64 line_range_clamped = r1s64(Clamp(1, line_range.min, (S64)info->lines_count), Clamp(1, line_range.max, (S64)info->lines_count));
    U64 line_count = (U64)dim_1s64(line_range_clamped)+1;
    
    // rjf: allocate output arrays
    result.line_tokens = push_array(arena, TXT_TokenArray, line_count);
    
    // rjf: binary search to find first token
    TXT_Token *tokens_first = 0;
    ProfScope("binary search to find first token")
    {
      Rng1U64 slice_range = r1u64(info->lines_ranges[line_range_clamped.min-1].min, info->lines_ranges[line_range_clamped.max-1].max);
      U64 min_idx = 0;
      U64 opl_idx = info->tokens.count;
      for(;;)
      {
        U64 mid_idx = (opl_idx+min_idx)/2;
        if(mid_idx >= opl_idx)
        {
          break;
        }
        TXT_Token *mid_token = &info->tokens.v[mid_idx];
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
    TXT_TokenList *line_tokens_lists = push_array(scratch.arena, TXT_TokenList, line_count);
    if(tokens_first != 0) ProfScope("grab per-line tokens")
    {
      TXT_Token *tokens_opl = info->tokens.v+info->tokens.count;
      U64 line_slice_idx = 0;
      for(TXT_Token *token = tokens_first; token < tokens_opl && line_slice_idx < line_count;)
      {
        if(token->range.min < info->lines_ranges[line_slice_idx+line_range.min-1].max)
        {
          if(token->range.max > info->lines_ranges[line_slice_idx+line_range.min-1].min)
          {
            txt_token_list_push(scratch.arena, &line_tokens_lists[line_slice_idx], token);
          }
          B32 need_token_advance = 0;
          B32 need_line_advance = 0;
          if(token->range.max >= info->lines_ranges[line_slice_idx+line_range.min-1].max)
          {
            need_line_advance = 1;
          }
          if(token->range.max <= info->lines_ranges[line_slice_idx+line_range.min-1].max)
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
    for(U64 line_slice_idx = 0; line_slice_idx < line_count; line_slice_idx += 1)
    {
      result.line_tokens[line_slice_idx] = txt_token_array_from_list(arena, &line_tokens_lists[line_slice_idx]);
    }
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Transfer Threads

internal B32
txt_u2p_enqueue_req(U128 hash, TXT_LangKind lang, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(txt_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = txt_shared->u2p_ring_write_pos - txt_shared->u2p_ring_read_pos;
    U64 available_size = txt_shared->u2p_ring_size - unconsumed_size;
    if(available_size >= sizeof(hash)+sizeof(lang))
    {
      good = 1;
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
txt_u2p_dequeue_req(U128 *hash_out, TXT_LangKind *lang_out)
{
  OS_MutexScope(txt_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = txt_shared->u2p_ring_write_pos - txt_shared->u2p_ring_read_pos;
    if(unconsumed_size >= sizeof(*hash_out) + sizeof(*lang_out))
    {
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
    //- rjf: get next key
    U128 hash = {0};
    TXT_LangKind lang = TXT_LangKind_Null;
    txt_u2p_dequeue_req(&hash, &lang);
    HS_Scope *scope = hs_scope_open();
    
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
        if(u128_match(n->hash, hash) && n->lang == lang)
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
      
      //- rjf: grab pointers to working counters
      U64 *bytes_processed_ptr = 0;
      U64 *bytes_to_process_ptr = 0;
      OS_MutexScopeR(stripe->rw_mutex)
      {
        for(TXT_Node *n = slot->first; n != 0; n = n->next)
        {
          if(u128_match(n->hash, hash) && n->lang == lang)
          {
            bytes_processed_ptr = &n->info.bytes_processed;
            bytes_to_process_ptr = &n->info.bytes_to_process;
          }
        }
      }
      
      //- rjf: set # of bytes to process
      if(bytes_to_process_ptr)
      {
        //                                               (line ending calc)     (line counting)    (line measuring)   (lexing)
        ins_atomic_u64_eval_assign(bytes_to_process_ptr, Min(data.size, 1024) + data.size        + data.size        + data.size*(lang != TXT_LangKind_Null));
      }
      
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
      
      //- rjf: bump progress
      if(bytes_processed_ptr)
      {
        ins_atomic_u64_eval_assign(bytes_processed_ptr, Min(data.size, 1024));
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
        if(idx && idx%1000 == 0)
        {
          ins_atomic_u64_add_eval(bytes_processed_ptr, 1000);
        }
      }
      
      //- rjf: bump progress
      if(bytes_processed_ptr)
      {
        ins_atomic_u64_eval_assign(bytes_processed_ptr, Min(data.size, 1024) + data.size);
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
        if(idx && idx%1000 == 0)
        {
          ins_atomic_u64_add_eval(bytes_processed_ptr, 1000);
        }
      }
      
      //- rjf: bump progress
      if(bytes_processed_ptr)
      {
        ins_atomic_u64_eval_assign(bytes_processed_ptr, Min(data.size, 1024) + data.size + data.size);
      }
      
      //- rjf: lang -> lex function
      TXT_LangLexFunctionType *lex_function = txt_lex_function_from_lang_kind(lang);
      
      //- rjf: lex function * data -> tokens
      TXT_TokenArray tokens = {0};
      if(lex_function != 0)
      {
        tokens = lex_function(info_arena, bytes_processed_ptr, data);
      }
      info.tokens = tokens;
      
      //- rjf: bump progress
      if(bytes_processed_ptr)
      {
        ins_atomic_u64_eval_assign(bytes_processed_ptr, Min(data.size, 1024) + data.size + data.size + data.size*(lex_function != 0));
      }
    }
    
    //- rjf: commit results to cache
    if(got_task) OS_MutexScopeW(stripe->rw_mutex)
    {
      for(TXT_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, hash) && n->lang == lang)
        {
          n->arena = info_arena;
          info.bytes_processed = n->info.bytes_processed;
          info.bytes_to_process = n->info.bytes_to_process;
          MemoryCopyStruct(&n->info, &info);
          ins_atomic_u32_eval_assign(&n->is_working, 0);
          ins_atomic_u64_inc_eval(&n->load_count);
          break;
        }
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
