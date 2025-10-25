// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0xe34cd4ff

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
  else if(str8_match(extension, str8_lit("rs"), StringMatchFlag_CaseInsensitive))
  {
    kind = TXT_LangKind_Rust;
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
    case TXT_LangKind_Rust:            {result = str8_lit("rs");}break;
  }
  return result;
}

internal TXT_LangKind
txt_lang_kind_from_arch(Arch arch)
{
  TXT_LangKind kind = TXT_LangKind_Null;
  switch(arch)
  {
    default:{}break;
    case Arch_x64:{kind = TXT_LangKind_DisasmX64Intel;}break;
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
    case TXT_LangKind_Rust:          {fn = txt_token_array_from_string__rust;}break;
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
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  U64 chunk_size = Clamp(8, string.size/8, 4096);
  
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
          txt_token_chunk_list_push(scratch.arena, &tokens, chunk_size, &token);
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
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$' && byte < 128);
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
          txt_token_chunk_list_push(scratch.arena, &tokens, chunk_size, &token);
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
              if(str8_match(str8_substr(token_string, r1u64(off, off+c_cpp_multichar_symbol_strings[idx].size)),
                            c_cpp_multichar_symbol_strings[idx],
                            0))
              {
                found = 1;
                next_off = off + c_cpp_multichar_symbol_strings[idx].size;
                TXT_Token token = {TXT_TokenKind_Symbol, r1u64(active_token_start_idx+off, active_token_start_idx+next_off)};
                txt_token_chunk_list_push(scratch.arena, &tokens, chunk_size, &token);
                break;
              }
            }
            if(!found)
            {
              next_off = off+1;
              TXT_Token token = {TXT_TokenKind_Symbol, r1u64(active_token_start_idx+off, active_token_start_idx+next_off)};
              txt_token_chunk_list_push(scratch.arena, &tokens, chunk_size, &token);
            }
          }
        }
        
        // rjf: all other tokens
        else
        {
          txt_token_chunk_list_push(scratch.arena, &tokens, chunk_size, &token);
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
  ProfEnd();
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
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$' && byte < 128);
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
              if(str8_match(str8_substr(token_string, r1u64(off, off+odin_multichar_symbol_strings[idx].size)),
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
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$' && byte < 128);
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
              if(str8_match(str8_substr(token_string, r1u64(off, off+jai_multichar_symbol_strings[idx].size)),
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
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$' && byte < 128);
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
              if(str8_match(str8_substr(token_string, r1u64(off, off+zig_multichar_symbol_strings[idx].size)),
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
txt_token_array_from_string__rust(Arena *arena, U64 *bytes_processed_counter, String8 string)
{
  // NOTE(spey): Rust supports unicode identifiers. They are not handled in any way here,
  // but it might be worth looking into in the future.

  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: generate token list
  TXT_TokenChunkList tokens = {0};
  {
    S32 multiline_comment_nesting_level = 0;
    S32 raw_string_nesting_level = 0;
    S32 raw_string_ender_nesting_level = 0;

    // NOTE(spey): Rust's syntax is designed in such a way that we can't be sure what a token
    // is immediately from the first character, so we have to keep track of some possibilities.
    B32 token_may_be_char = 0;
    B32 token_may_be_lifetime = 0;
    B32 token_may_be_string = 0;
    
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
      U64 starter_pad = 0;

      // spey: special case of starter for nested comments
      if(active_token_kind == TXT_TokenKind_Comment)
      {
        if(byte == '/' && next_byte == '*')      { active_token_kind = TXT_TokenKind_Comment; multiline_comment_nesting_level++; starter_pad = 1; }
      }
      // spey: special case of starter for raw string literals
      else if(active_token_kind == TXT_TokenKind_Identifier && token_may_be_string)
      {
        if(0){}
        else if(byte == 'r' && next_byte == '#') {} // spey: still an identifier that may be a string (this branch triggers for raw byte/C string literals)
        else if(byte == '#' && next_byte == '"') { active_token_kind = TXT_TokenKind_String; token_may_be_string = 0; token_may_be_char = 0; raw_string_nesting_level++; starter_pad = 2; }
        else if(byte == '#' && next_byte == '#') { raw_string_nesting_level++; }
        else                                     { token_may_be_string = 0; token_may_be_char = 0; raw_string_nesting_level = 0; } // spey: confirmed raw identifier
      }
      // spey: regular cases
      else if(active_token_kind == TXT_TokenKind_Null)
      {
        // rjf: use next bytes to start a new token
        if(0){}
        else if(char_is_space(byte))             { active_token_kind = TXT_TokenKind_Whitespace; }
        else if(byte == 'r' && next_byte == '#') { active_token_kind = TXT_TokenKind_Identifier; token_may_be_string = 1; } // spey: either raw identifiers or raw string literals
        else if(char_is_digit(byte, 10) ||
                (byte == '.' &&
                 char_is_digit(next_byte, 10)))  { active_token_kind = TXT_TokenKind_Numeric; }
        else if(byte == '"')                     { active_token_kind = TXT_TokenKind_String; token_may_be_char = 0; }
        else if((byte == 'c' || byte == 'b') &&
                 next_byte == '"')               { active_token_kind = TXT_TokenKind_String; token_may_be_char = 0; starter_pad = 1; }
        else if((byte == 'c' || byte == 'b') &&
                 next_byte == 'r')               { active_token_kind = TXT_TokenKind_Identifier; token_may_be_string = 1; }
        else if(byte == '_' ||
                char_is_alpha(byte))             { active_token_kind = TXT_TokenKind_Identifier; }
        else if(byte == '/' && next_byte == '/') { active_token_kind = TXT_TokenKind_Comment; starter_pad = 1; }
        else if(byte == '/' && next_byte == '*') { active_token_kind = TXT_TokenKind_Comment; starter_pad = 1; multiline_comment_nesting_level++; }
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
        else if(byte == '\'')                    { active_token_kind = TXT_TokenKind_String; token_may_be_char = 1; token_may_be_lifetime = 1; }
        else if((byte == 'c' || byte == 'b') &&
                 next_byte == '\'')              { active_token_kind = TXT_TokenKind_String; token_may_be_char = 1; starter_pad = 1; }

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

      B32 is_on_starter = idx <= active_token_start_idx || token_may_be_string;

      // spey: advance by starter padding byte(s) and reset byte/next_byte values
      idx += starter_pad;
      byte      = (idx+0 < string.size) ? (string.str[idx+0]) : 0;
      next_byte = (idx+1 < string.size) ? (string.str[idx+1]) : 0;

      // rjf: look for ender
      U64 ender_pad = 0;
      B32 ender_found = 0;
      if(active_token_kind != TXT_TokenKind_Null && !is_on_starter)
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
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '$' && byte != '#' && byte != '!' && byte < 128);
          }break;
          case TXT_TokenKind_Numeric:
          {
            ender_found = (!char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte != '.' && byte != '\'');
          }break;
          case TXT_TokenKind_String:
          {
            if(!escaped)
            {
              if(token_may_be_char)
              {
                if(byte == '\'')
                {
                  // spey: char ending
                  ender_found = 1;
                }
                else if(token_may_be_lifetime && !char_is_alpha(byte) && !char_is_digit(byte, 10) && byte != '_' && byte < 128)
                {
                  // spey: lifetime ending
                  ender_found = 1;
                }
              }
              else
              {
                if(0){}

                // spey: regular string
                else if(raw_string_nesting_level == 0)       { ender_found = byte == '"'; }

                // spey: raw string
                else if(byte == '"' && next_byte == '#' &&
                        raw_string_ender_nesting_level == 0) { raw_string_ender_nesting_level++; }
                else if(byte == '#' && next_byte != '#' &&
                        raw_string_ender_nesting_level == raw_string_nesting_level &&
                        raw_string_ender_nesting_level >= 0) { ender_found = 1; raw_string_nesting_level = 0; raw_string_ender_nesting_level = 0; }
                else if(byte == '#' && next_byte != '#' &&
                        raw_string_ender_nesting_level >= 0) { raw_string_ender_nesting_level = 0; }
                else if(byte == '#' &&
                        raw_string_ender_nesting_level >= 0) { raw_string_ender_nesting_level++; }
              }
            }

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
            if(multiline_comment_nesting_level == 0)
            {
              ender_found = (byte == '\r' || byte == '\n');
            }
            else
            {
              if (byte == '*' && next_byte == '/')
                multiline_comment_nesting_level--;

              ender_found = (active_token_start_idx+1 < idx && multiline_comment_nesting_level == 0);
              ender_pad += 2;
            }
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
          read_only local_persist String8 rust_keywords[] =
          {
            str8_lit_comp("as"),
            str8_lit_comp("break"),
            str8_lit_comp("const"),
            str8_lit_comp("continue"),
            str8_lit_comp("crate"),
            str8_lit_comp("else"),
            str8_lit_comp("enum"),
            str8_lit_comp("extern"),
            str8_lit_comp("false"),
            str8_lit_comp("fn"),
            str8_lit_comp("for"),
            str8_lit_comp("if"),
            str8_lit_comp("impl"),
            str8_lit_comp("in"),
            str8_lit_comp("let"),
            str8_lit_comp("loop"),
            str8_lit_comp("match"),
            str8_lit_comp("mod"),
            str8_lit_comp("move"),
            str8_lit_comp("mut"),
            str8_lit_comp("pub"),
            str8_lit_comp("ref"),
            str8_lit_comp("return"),
            str8_lit_comp("self"),
            str8_lit_comp("Self"),
            str8_lit_comp("static"),
            str8_lit_comp("struct"),
            str8_lit_comp("super"),
            str8_lit_comp("trait"),
            str8_lit_comp("true"),
            str8_lit_comp("type"),
            str8_lit_comp("unsafe"),
            str8_lit_comp("use"),
            str8_lit_comp("where"),
            str8_lit_comp("while"),
            str8_lit_comp("yield"),
            str8_lit_comp("async"),
            str8_lit_comp("await"),
            str8_lit_comp("dyn"),

            // weak keywords
            str8_lit_comp("macro_rules"),
            str8_lit_comp("raw"),
            str8_lit_comp("safe"),
            str8_lit_comp("union"),
          };
          String8 token_string = str8_substr(string, r1u64(active_token_start_idx, idx+ender_pad));
          for(U64 keyword_idx = 0; keyword_idx < ArrayCount(rust_keywords); keyword_idx += 1)
          {
            if(str8_match(rust_keywords[keyword_idx], token_string, 0))
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
          read_only local_persist String8 rust_multichar_symbol_strings[] =
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
            for(U64 idx = 0; idx < ArrayCount(rust_multichar_symbol_strings); idx += 1)
            {
              if(str8_match(str8_substr(token_string, r1u64(off, off+rust_multichar_symbol_strings[idx].size)),
                            rust_multichar_symbol_strings[idx],
                            0))
              {
                found = 1;
                next_off = off + rust_multichar_symbol_strings[idx].size;
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

        // rjf: increment by starter and ender padding
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
    S32 string_tick_nest = 0;
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
          else if(byte == '`')
          {
            active_token_start_off = off;
            active_token_kind = TXT_TokenKind_String;
            advance = 1;
            string_tick_nest += 1;
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
          U8 ender_byte = (string_tick_nest > 0 ? '\'' :
                           string_is_char ? '\''
                           : '"');
          if(!escaped && byte == ender_byte)
          {
            if(string_tick_nest > 0)
            {
              string_tick_nest -= 1;
            }
            if(string_tick_nest == 0)
            {
              ender_found = 1;
              advance = 1;
            }
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
          else if(string_tick_nest > 0 && byte == '`')
          {
            string_tick_nest += 1;
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

internal TXT_ScopeNode *
txt_scope_node_from_info_num(TXT_TextInfo *info, U64 num)
{
  TXT_ScopeNode *result = &txt_scope_node_nil;
  if(1 <= num && num <= info->scope_nodes.count)
  {
    result = &info->scope_nodes.v[num-1];
  }
  return result;
}

internal TXT_ScopeNode *
txt_scope_node_from_info_off(TXT_TextInfo *info, U64 off)
{
  TXT_ScopeNode *result = &txt_scope_node_nil;
  if(info->scope_pts.count != 0)
  {
    U64 first = 0;
    U64 opl = info->scope_pts.count;
    for(;;)
    {
      U64 mid = (first + opl) / 2;
      U64 mid_off = info->tokens.v[info->scope_pts.v[mid].token_idx].range.min;
      if(mid_off < off)
      {
        first = mid;
      }
      else if(off < mid_off)
      {
        opl = mid;
      }
      else
      {
        first = mid;
        break;
      }
      if(opl - first <= 1)
      {
        break;
      }
    }
    TXT_ScopeNode *closest_node = &info->scope_nodes.v[info->scope_pts.v[first].scope_idx];
    for(TXT_ScopeNode *scope_n = closest_node;
        scope_n != &txt_scope_node_nil;
        scope_n = txt_scope_node_from_info_num(info, scope_n->parent_num))
    {
      if(info->tokens.v[scope_n->token_idx_range.min].range.min <= off && off < info->tokens.v[scope_n->token_idx_range.max].range.max)
      {
        result = scope_n;
        break;
      }
    }
  }
  return result;
}

internal TXT_ScopeNode *
txt_scope_node_from_info_pt(TXT_TextInfo *info, TxtPt pt)
{
  U64 off = txt_off_from_info_pt(info, pt);
  TXT_ScopeNode *result = txt_scope_node_from_info_off(info, off);
  return result;
}

////////////////////////////////
//~ rjf: Artifact Cache Hooks / Lookups

typedef struct TXT_Artifact TXT_Artifact;
struct TXT_Artifact
{
  Arena *arena;
  U128 data_hash;
  TXT_TextInfo info;
};

typedef struct TXT_ArtifactCreateShared TXT_ArtifactCreateShared;
struct TXT_ArtifactCreateShared
{
  Arena *arena;
  TXT_TextInfo info;
  TXT_Artifact *artifact;
};

internal AC_Artifact
txt_artifact_create(String8 key, B32 *cancel_signal, B32 *retry_out, U64 *gen_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  Access *access = access_open();
  
  //- rjf: get shared state
  TXT_ArtifactCreateShared *shared = 0;
  if(lane_idx() == 0)
  {
    shared = push_array(scratch.arena, TXT_ArtifactCreateShared, 1);
  }
  lane_sync_u64(&shared, 0);
  
  //- rjf: unpack key
  U128 hash = {0};
  TXT_LangKind lang = TXT_LangKind_Null;
  str8_deserial_read_struct(key, 0, &hash);
  str8_deserial_read_struct(key, sizeof(hash), &lang);
  String8 data = c_data_from_hash(access, hash);
  TXT_LangLexFunctionType *lex_function = txt_lex_function_from_lang_kind(lang);
  
  //- rjf: data -> text info
  if(!u128_match(hash, u128_zero()))
  {
    if(lane_idx() == 0)
    {
      shared->arena = arena_alloc();
    }
    
    //- rjf: set # of bytes to process
    //                  (line ending calc)     (line counting)    (line measuring)   (lexing)
    set_progress_target(Min(data.size, 1024) + data.size        + data.size        + data.size*(lang != TXT_LangKind_Null));
    
    //- rjf: detect line end kind
    TXT_LineEndKind line_end_kind = TXT_LineEndKind_Null;
    if(lane_idx() == 0)
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
      shared->info.line_end_kind = line_end_kind;
    }
    lane_sync();
    set_progress(Min(data.size, 1024));
    
    //- rjf: count # of lines
    U64 lane_line_count = 0;
    if(lane_idx() == 0)
    {
      lane_line_count = 1;
    }
    {
      Rng1U64 range = lane_range(data.size);
      for EachInRange(idx, range)
      {
        if(data.str[idx] == '\n')
        {
          lane_line_count += 1;
        }
        if(idx && idx%1000 == 0)
        {
          add_progress(1000);
        }
      }
    }
    ins_atomic_u64_add_eval(&shared->info.lines_count, lane_line_count);
    lane_sync();
    set_progress(Min(data.size, 1024) + data.size);
    
    //- rjf: allocate & store line ranges
    if(lane_idx() == 0)
    {
      shared->info.lines_ranges = push_array_no_zero(shared->arena, Rng1U64, shared->info.lines_count);
      U64 line_idx = 0;
      U64 line_start_idx = 0;
      for(U64 idx = 0; idx <= data.size; idx += 1)
      {
        if(idx == data.size || data.str[idx] == '\n')
        {
          Rng1U64 line_range = r1u64(line_start_idx, idx);
          if(idx > 0 && data.str[idx-1] == '\r' && line_range.max > line_range.min)
          {
            line_range.max -= 1;
          }
          U64 line_size = dim_1u64(line_range);
          shared->info.lines_ranges[line_idx] = line_range;
          shared->info.lines_max_size = Max(shared->info.lines_max_size, line_size);
          line_idx += 1;
          line_start_idx = idx+1;
        }
        if(idx && idx%1000 == 0)
        {
          add_progress(1000);
        }
      }
    }
    lane_sync();
    set_progress(Min(data.size, 1024) + data.size + data.size);
    
    //- rjf: lex function * data -> tokens
    if(lane_idx() == 0 && lex_function != 0)
    {
      shared->info.tokens = lex_function(shared->arena, 0, data);
    }
    lane_sync();
    set_progress(Min(data.size, 1024) + data.size + data.size + data.size*(lex_function != 0));
    TXT_TokenArray tokens = shared->info.tokens;
    
    //- rjf: count scope points
    {
      U64 lane_scope_pt_opener_count = 0;
      U64 lane_scope_pt_count = 0;
      Rng1U64 range = lane_range(tokens.count);
      for EachInRange(idx, range)
      {
        if(tokens.v[idx].kind == TXT_TokenKind_Symbol)
        {
          String8 token_string = str8_substr(data, tokens.v[idx].range);
          B32 is_opener = (token_string.str[0] == '{' ||
                           token_string.str[0] == '(' ||
                           token_string.str[0] == '[');
          B32 is_closer = (token_string.str[0] == '}' ||
                           token_string.str[0] == ')' ||
                           token_string.str[0] == ']');
          if(token_string.size == 1 && (is_opener || is_closer))
          {
            lane_scope_pt_count += 1;
            lane_scope_pt_opener_count += !!is_opener;
          }
        }
      }
      ins_atomic_u64_add_eval(&shared->info.scope_pts.count, lane_scope_pt_count);
      ins_atomic_u64_add_eval(&shared->info.scope_nodes.count, lane_scope_pt_opener_count);
    }
    lane_sync();
    
    //- rjf: allocate & fill scope data
    if(lane_idx() == 0)
    {
      shared->info.scope_pts.v = push_array_no_zero(shared->arena, TXT_ScopePt, shared->info.scope_pts.count);
      shared->info.scope_nodes.v = push_array_no_zero(shared->arena, TXT_ScopeNode, shared->info.scope_nodes.count);
      {
        typedef struct ScopeTask ScopeTask;
        struct ScopeTask
        {
          ScopeTask *next;
          U64 scope_idx;
        };
        Temp scratch = scratch_begin(0, 0);
        ScopeTask *top_scope_task = 0;
        ScopeTask *free_scope_task = 0;
        U64 pt_idx = 0;
        U64 scope_idx = 0;
        for EachIndex(token_idx, tokens.count)
        {
          if(tokens.v[token_idx].kind == TXT_TokenKind_Symbol)
          {
            String8 token_string = str8_substr(data, tokens.v[token_idx].range);
            B32 is_opener = (token_string.str[0] == '{' ||
                             token_string.str[0] == '(' ||
                             token_string.str[0] == '[');
            B32 is_closer = (token_string.str[0] == '}' ||
                             token_string.str[0] == ')' ||
                             token_string.str[0] == ']');
            
            // rjf: opener symbols -> push scope
            if(is_opener)
            {
              // rjf: insert into scope tree
              TXT_ScopeNode *new_scope = &shared->info.scope_nodes.v[scope_idx];
              new_scope->token_idx_range.min = token_idx;
              if(top_scope_task)
              {
                U64 new_scope_num = scope_idx+1;
                TXT_ScopeNode *parent = &shared->info.scope_nodes.v[top_scope_task->scope_idx];
                if(parent->first_num == 0)
                {
                  parent->first_num = new_scope_num;
                }
                if(parent->last_num != 0)
                {
                  TXT_ScopeNode *prev_scope = &shared->info.scope_nodes.v[parent->last_num-1];
                  prev_scope->next_num = new_scope_num;
                }
                parent->last_num = new_scope_num;
                new_scope->parent_num = top_scope_task->scope_idx+1;
              }
              
              // rjf: push onto scope stack
              ScopeTask *scope_task = free_scope_task;
              if(scope_task)
              {
                SLLStackPop(free_scope_task);
              }
              else
              {
                scope_task = push_array(scratch.arena, ScopeTask, 1);
              }
              scope_task->scope_idx = scope_idx;
              scope_idx += 1;
              SLLStackPush(top_scope_task, scope_task);
            }
            
            // rjf: opener or closer -> fill endpoint
            if(top_scope_task && (is_opener || is_closer))
            {
              shared->info.scope_pts.v[pt_idx].token_idx = token_idx;
              shared->info.scope_pts.v[pt_idx].scope_idx = top_scope_task->scope_idx;
              pt_idx += 1;
            }
            
            // rjf: closer symbols -> pop
            if(is_closer && top_scope_task != 0)
            {
              ScopeTask *popped = top_scope_task;
              shared->info.scope_nodes.v[popped->scope_idx].token_idx_range.max = token_idx;
              SLLStackPop(top_scope_task);
              SLLStackPush(free_scope_task, popped);
            }
          }
        }
        scratch_end(scratch);
      }
    }
    lane_sync();
  }
  
  //- rjf: mark dependency on hash
  c_hash_downstream_inc(hash);
  
  //- rjf: package as artifact
  if(lane_idx() == 0 && shared->arena != 0)
  {
    shared->artifact = push_array(shared->arena, TXT_Artifact, 1);
    shared->artifact->arena     = shared->arena;
    shared->artifact->data_hash = hash;
    shared->artifact->info      = shared->info;
  }
  lane_sync();
  
  access_close(access);
  scratch_end(scratch);
  ProfEnd();
  AC_Artifact result = {0};
  result.u64[0] = (U64)shared->artifact;
  return result;
}

internal void
txt_artifact_destroy(AC_Artifact artifact)
{
  TXT_Artifact *txt_artifact = (TXT_Artifact *)artifact.u64[0];
  if(txt_artifact == 0) { return; }
  c_hash_downstream_dec(txt_artifact->data_hash);
  arena_release(txt_artifact->arena);
}

internal TXT_TextInfo
txt_text_info_from_hash_lang(Access *access, U128 hash, TXT_LangKind lang)
{
#pragma pack(push, 1)
  struct
  {
    U128 hash;
    TXT_LangKind lang;
  } key = {hash, lang};
#pragma pack(pop)
  String8 key_string = str8_struct(&key);
  AC_Artifact artifact = ac_artifact_from_key(access, key_string, txt_artifact_create, txt_artifact_destroy, 0, .flags = AC_Flag_Wide);
  TXT_Artifact *txt_artifact = (TXT_Artifact *)artifact.u64[0];
  TXT_TextInfo info = {0};
  if(txt_artifact != 0)
  {
    info = txt_artifact->info;
  }
  return info;
}

internal TXT_TextInfo
txt_text_info_from_key_lang(Access *access, C_Key key, TXT_LangKind lang, U128 *hash_out)
{
  TXT_TextInfo result = {0};
  for(U64 rewind_idx = 0; rewind_idx < C_KEY_HASH_HISTORY_COUNT; rewind_idx += 1)
  {
    U128 hash = c_hash_from_key(key, rewind_idx);
    result = txt_text_info_from_hash_lang(access, hash, lang);
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
