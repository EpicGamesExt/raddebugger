// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Message Type Functions

internal void
md_msg_list_push(Arena *arena, MD_MsgList *msgs, MD_Node *node, MD_MsgKind kind, String8 string)
{
  MD_Msg *msg = push_array(arena, MD_Msg, 1);
  msg->node = node;
  msg->kind = kind;
  msg->string = string;
  SLLQueuePush(msgs->first, msgs->last, msg);
  msgs->count += 1;
  msgs->worst_message_kind = Max(kind, msgs->worst_message_kind);
}

internal void
md_msg_list_pushf(Arena *arena, MD_MsgList *msgs, MD_Node *node, MD_MsgKind kind, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(arena, fmt, args);
  md_msg_list_push(arena, msgs, node, kind, string);
  va_end(args);
}

internal void
md_msg_list_concat_in_place(MD_MsgList *dst, MD_MsgList *to_push)
{
  if(to_push->first != 0)
  {
    if(dst->last)
    {
      dst->last->next = to_push->first;
      dst->last = to_push->last;
      dst->count += to_push->count;
      dst->worst_message_kind = Max(dst->worst_message_kind, to_push->worst_message_kind);
    }
    else
    {
      MemoryCopyStruct(dst, to_push);
    }
  }
  MemoryZeroStruct(to_push);
}

////////////////////////////////
//~ rjf: Token Type Functions

internal MD_Token
md_token_make(Rng1U64 range, MD_TokenFlags flags)
{
  MD_Token token = {range, flags};
  return token;
}

internal B32
md_token_match(MD_Token a, MD_Token b)
{
  return (a.range.min == b.range.min &&
          a.range.max == b.range.max &&
          a.flags == b.flags);
}

internal String8List
md_string_list_from_token_flags(Arena *arena, MD_TokenFlags flags)
{
  String8List strs = {0};
  if(flags & MD_TokenFlag_Identifier          ){str8_list_push(arena, &strs, str8_lit("Identifier"));}
  if(flags & MD_TokenFlag_Numeric             ){str8_list_push(arena, &strs, str8_lit("Numeric"));}
  if(flags & MD_TokenFlag_StringLiteral       ){str8_list_push(arena, &strs, str8_lit("StringLiteral"));}
  if(flags & MD_TokenFlag_Symbol              ){str8_list_push(arena, &strs, str8_lit("Symbol"));}
  if(flags & MD_TokenFlag_Reserved            ){str8_list_push(arena, &strs, str8_lit("Reserved"));}
  if(flags & MD_TokenFlag_Comment             ){str8_list_push(arena, &strs, str8_lit("Comment"));}
  if(flags & MD_TokenFlag_Whitespace          ){str8_list_push(arena, &strs, str8_lit("Whitespace"));}
  if(flags & MD_TokenFlag_Newline             ){str8_list_push(arena, &strs, str8_lit("Newline"));}
  if(flags & MD_TokenFlag_BrokenComment       ){str8_list_push(arena, &strs, str8_lit("BrokenComment"));}
  if(flags & MD_TokenFlag_BrokenStringLiteral ){str8_list_push(arena, &strs, str8_lit("BrokenStringLiteral"));}
  if(flags & MD_TokenFlag_BadCharacter        ){str8_list_push(arena, &strs, str8_lit("BadCharacter"));}
  return strs;
}

internal void
md_token_chunk_list_push(Arena *arena, MD_TokenChunkList *list, U64 cap, MD_Token token)
{
  MD_TokenChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, MD_TokenChunkNode, 1);
    node->cap = cap;
    node->v = push_array_no_zero(arena, MD_Token, cap);
    SLLQueuePush(list->first, list->last, node);
    list->chunk_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], &token);
  node->count += 1;
  list->total_token_count += 1;
}

internal MD_TokenArray
md_token_array_from_chunk_list(Arena *arena, MD_TokenChunkList *chunks)
{
  MD_TokenArray result = {0};
  result.count = chunks->total_token_count;
  result.v = push_array_no_zero(arena, MD_Token, result.count);
  U64 write_idx = 0;
  for(MD_TokenChunkNode *n = chunks->first; n != 0; n = n->next)
  {
    MemoryCopy(result.v+write_idx, n->v, sizeof(MD_Token)*n->count);
    write_idx += n->count;
  }
  return result;
}

internal String8
md_content_string_from_token_flags_str8(MD_TokenFlags flags, String8 string)
{
  U64 num_chop = 0;
  U64 num_skip = 0;
  {
    num_skip += 3*!!(flags & MD_TokenFlag_StringTriplet);
    num_chop += 3*!!(flags & MD_TokenFlag_StringTriplet);
    num_skip += 1*(!(flags & MD_TokenFlag_StringTriplet) && flags & MD_TokenFlag_StringLiteral);
    num_chop += 1*(!(flags & MD_TokenFlag_StringTriplet) && flags & MD_TokenFlag_StringLiteral);
  }
  String8 result = string;
  result = str8_chop(result, num_chop);
  result = str8_skip(result, num_skip);
  return result;
}

////////////////////////////////
//~ rjf: Node Type Functions

//- rjf: flag conversions

internal MD_NodeFlags
md_node_flags_from_token_flags(MD_TokenFlags flags)
{
  MD_NodeFlags result = 0;
  result |=         MD_NodeFlag_Identifier*!!(flags&MD_TokenFlag_Identifier);
  result |=            MD_NodeFlag_Numeric*!!(flags&MD_TokenFlag_Numeric);
  result |=      MD_NodeFlag_StringLiteral*!!(flags&MD_TokenFlag_StringLiteral);
  result |=             MD_NodeFlag_Symbol*!!(flags&MD_TokenFlag_Symbol);
  result |= MD_NodeFlag_StringSingleQuote	*!!(flags&MD_TokenFlag_StringSingleQuote);
  result |= MD_NodeFlag_StringDoubleQuote	*!!(flags&MD_TokenFlag_StringDoubleQuote);
  result |=         MD_NodeFlag_StringTick*!!(flags&MD_TokenFlag_StringTick);
  result |=      MD_NodeFlag_StringTriplet*!!(flags&MD_TokenFlag_StringTriplet);
  return result;
}

//- rjf: nil

internal B32
md_node_is_nil(MD_Node *node)
{
  return (node == 0 || node == &md_nil_node || node->kind == MD_NodeKind_Nil);
}

//- rjf: iteration

internal MD_NodeRec
md_node_rec_depth_first(MD_Node *node, MD_Node *subtree_root, U64 child_off, U64 sib_off)
{
  MD_NodeRec rec = {0};
  rec.next = &md_nil_node;
  if(!md_node_is_nil(MemberFromOffset(MD_Node *, node, child_off)))
  {
    rec.next = MemberFromOffset(MD_Node *, node, child_off);
    rec.push_count = 1;
  }
  else for(MD_Node *p = node; !md_node_is_nil(p) && p != subtree_root; p = p->parent, rec.pop_count += 1)
  {
    if(!md_node_is_nil(MemberFromOffset(MD_Node *, p, sib_off)))
    {
      rec.next = MemberFromOffset(MD_Node *, p, sib_off);
      break;
    }
  }
  return rec;
}

//- rjf: tree building

internal MD_Node *
md_push_node(Arena *arena, MD_NodeKind kind, MD_NodeFlags flags, String8 string, String8 raw_string, U64 src_offset)
{
  MD_Node *node = push_array(arena, MD_Node, 1);
  node->first = node->last = node->parent = node->next = node->prev = node->first_tag = node->last_tag = &md_nil_node;
  node->kind       = kind;
  node->flags      = flags;
  node->string     = string;
  node->raw_string = raw_string;
  node->src_offset = src_offset;
  return node;
}

internal void
md_node_push_child(MD_Node *parent, MD_Node *node)
{
  node->parent = parent;
  DLLPushBack_NPZ(&md_nil_node, parent->first, parent->last, node, next, prev);
}

internal void
md_node_push_tag(MD_Node *parent, MD_Node *node)
{
  node->parent = parent;
  DLLPushBack_NPZ(&md_nil_node, parent->first_tag, parent->last_tag, node, next, prev);
}

//- rjf: tree introspection

internal MD_Node *
md_node_from_chain_string(MD_Node *first, MD_Node *opl, String8 string, StringMatchFlags flags)
{
  MD_Node *result = &md_nil_node;
  for(MD_Node *n = first; !md_node_is_nil(n) && n != opl; n = n->next)
  {
    if(str8_match(n->string, string, flags))
    {
      result = n;
      break;
    }
  }
  return result;
}

internal MD_Node *
md_node_from_chain_index(MD_Node *first, MD_Node *opl, U64 index)
{
  MD_Node *result = &md_nil_node;
  S64 idx = 0;
  for(MD_Node *n = first; !md_node_is_nil(n) && n != opl; n = n->next, idx += 1)
  {
    if(index == idx)
    {
      result = n;
      break;
    }
  }
  return result;
}

internal MD_Node *
md_node_from_chain_flags(MD_Node *first, MD_Node *opl, MD_NodeFlags flags)
{
  MD_Node *result = &md_nil_node;
  for(MD_Node *n = first; !md_node_is_nil(n) && n != opl; n = n->next)
  {
    if(n->flags & flags)
    {
      result = n;
      break;
    }
  }
  return result;
}

internal U64
md_index_from_node(MD_Node *node)
{
  U64 index = 0;
  for(MD_Node *n = node->prev; !md_node_is_nil(n); n = n->prev)
  {
    index += 1;
  }
  return index;
}

internal MD_Node *
md_root_from_node(MD_Node *node)
{
  MD_Node *result = node;
  for(MD_Node *p = node->parent; (p->kind == MD_NodeKind_Main || p->kind == MD_NodeKind_Tag) && !md_node_is_nil(p); p = p->parent)
  {
    result = p;
  }
  return result;
}

internal MD_Node *
md_child_from_string(MD_Node *node, String8 child_string, StringMatchFlags flags)
{
  return md_node_from_chain_string(node->first, &md_nil_node, child_string, flags);
}

internal MD_Node *
md_tag_from_string(MD_Node *node, String8 tag_string, StringMatchFlags flags)
{
  return md_node_from_chain_string(node->first_tag, &md_nil_node, tag_string, flags);
}

internal MD_Node *
md_child_from_index(MD_Node *node, U64 index)
{
  return md_node_from_chain_index(node->first, &md_nil_node, index);
}

internal MD_Node *
md_tag_from_index(MD_Node *node, U64 index)
{
  return md_node_from_chain_index(node->first_tag, &md_nil_node, index);
}

internal MD_Node *
md_tag_arg_from_index(MD_Node *node, String8 tag_string, StringMatchFlags flags, U64 index)
{
  MD_Node *tag = md_tag_from_string(node, tag_string, flags);
  return md_child_from_index(tag, index);
}

internal MD_Node *
md_tag_arg_from_string(MD_Node *node, String8 tag_string, StringMatchFlags tag_str_flags, String8 arg_string, StringMatchFlags arg_str_flags)
{
  MD_Node *tag = md_tag_from_string(node, tag_string, tag_str_flags);
  MD_Node *arg = md_child_from_string(tag, arg_string, arg_str_flags);
  return arg;
}

internal B32
md_node_has_child(MD_Node *node, String8 string, StringMatchFlags flags)
{
  return !md_node_is_nil(md_child_from_string(node, string, flags));
}

internal B32
md_node_has_tag(MD_Node *node, String8 string, StringMatchFlags flags)
{
  return !md_node_is_nil(md_tag_from_string(node, string, flags));
}

internal U64
md_child_count_from_node(MD_Node *node)
{
  U64 result = 0;
  for(MD_Node *child = node->first; !md_node_is_nil(child); child = child->next)
  {
    result += 1;
  }
  return result;
}

internal U64
md_tag_count_from_node(MD_Node *node)
{
  U64 result = 0;
  for(MD_Node *child = node->first_tag; !md_node_is_nil(child); child = child->next)
  {
    result += 1;
  }
  return result;
}

//- rjf: tree comparison

internal B32
md_node_match(MD_Node *a, MD_Node *b, StringMatchFlags flags)
{
  B32 result = 0;
  if(a->kind == b->kind && str8_match(a->string, b->string, flags))
  {
    result = 1;
    if(result)
    {
      result = result && a->flags == b->flags;
    }
    if(result && a->kind != MD_NodeKind_Tag)
    {
      for(MD_Node *a_tag = a->first_tag, *b_tag = b->first_tag;
          !md_node_is_nil(a_tag) || !md_node_is_nil(b_tag);
          a_tag = a_tag->next, b_tag = b_tag->next)
      {
        if(md_node_match(a_tag, b_tag, flags))
        {
          for(MD_Node *a_tag_arg = a_tag->first, *b_tag_arg = b_tag->first;
              !md_node_is_nil(a_tag_arg) || !md_node_is_nil(b_tag_arg);
              a_tag_arg = a_tag_arg->next, b_tag_arg = b_tag_arg->next)
          {
            if(!md_node_deep_match(a_tag_arg, b_tag_arg, flags))
            {
              result = 0;
              goto end;
            }
          }
        }
        else
        {
          result = 0;
          goto end;
        }
      }
    }
  }
  end:;
  return result;
}

internal B32
md_node_deep_match(MD_Node *a, MD_Node *b, StringMatchFlags flags)
{
  B32 result = md_node_match(a, b, flags);
  if(result)
  {
    for(MD_Node *a_child = a->first, *b_child = b->first;
        !md_node_is_nil(a_child) || !md_node_is_nil(b_child);
        a_child = a_child->next, b_child = b_child->next)
    {
      if(!md_node_deep_match(a_child, b_child, flags))
      {
        result = 0;
        goto end;
      }
    }
  }
  end:;
  return result;
}

////////////////////////////////
//~ rjf: Text -> Tokens Functions

internal MD_TokenizeResult
md_tokenize_from_text(Arena *arena, String8 text)
{
  Temp scratch = scratch_begin(&arena, 1);
  MD_TokenChunkList tokens = {0};
  MD_MsgList msgs = {0};
  U8 *byte_first = text.str;
  U8 *byte_opl = byte_first + text.size;
  U8 *byte = byte_first;
  
  //- rjf: scan string & produce tokens
  for(;byte < byte_opl;)
  {
    MD_TokenFlags token_flags = 0;
    U8 *token_start = 0;
    U8 *token_opl = 0;
    
    //- rjf: whitespace
    if(token_flags == 0 && (*byte == ' ' || *byte == '\t' || *byte == '\v' || *byte == '\r'))
    {
      token_flags = MD_TokenFlag_Whitespace;
      token_start = byte;
      token_opl = byte;
      byte += 1;
      for(;byte <= byte_opl; byte += 1)
      {
        token_opl += 1;
        if(byte == byte_opl || (*byte != ' ' && *byte != '\t' && *byte != '\v' && *byte != '\r'))
        {
          break;
        }
      }
    }
    
    //- rjf: newlines
    if(token_flags == 0 && *byte == '\n')
    {
      token_flags = MD_TokenFlag_Newline;
      token_start = byte;
      token_opl = byte+1;
      byte += 1;
    }
    
    //- rjf: single-line comments
    if(token_flags == 0 && (byte+1 < byte_opl && *byte == '/' && byte[1] == '/'))
    {
      token_flags = MD_TokenFlag_Comment;
      token_start = byte;
      token_opl = byte+2;
      byte += 2;
      B32 escaped = 0;
      for(;byte <= byte_opl; byte += 1)
      {
        token_opl += 1;
        if(byte == byte_opl)
        {
          break;
        }
        if(escaped)
        {
          escaped = 0;
        }
        else
        {
          if(*byte == '\n')
          {
            break;
          }
          else if(*byte == '\\')
          {
            escaped = 1;
          }
        }
      }
    }
    
    //- rjf: multi-line comments
    if(token_flags == 0 && (byte+1 < byte_opl && *byte == '/' && byte[1] == '*'))
    {
      token_flags = MD_TokenFlag_Comment;
      token_start = byte;
      token_opl = byte+2;
      byte += 2;
      for(;byte <= byte_opl; byte += 1)
      {
        token_opl += 1;
        if(byte == byte_opl)
        {
          token_flags |= MD_TokenFlag_BrokenComment;
          break;
        }
        if(byte+1 < byte_opl && byte[0] == '*' && byte[1] == '/')
        {
          token_opl += 2;
          break;
        }
      }
    }
    
    //- rjf: identifiers
    if(token_flags == 0 && (('A' <= *byte && *byte <= 'Z') ||
                            ('a' <= *byte && *byte <= 'z') ||
                            *byte == '_' ||
                            utf8_class[*byte>>3] >= 2 ))
    {
      token_flags = MD_TokenFlag_Identifier;
      token_start = byte;
      token_opl = byte;
      byte += 1;
      for(;byte <= byte_opl; byte += 1)
      {
        token_opl += 1;
        if(byte == byte_opl ||
           (!('A' <= *byte && *byte <= 'Z') &&
            !('a' <= *byte && *byte <= 'z') &&
            !('0' <= *byte && *byte <= '9') &&
            *byte != '_' &&
            utf8_class[*byte>>3] < 2))
        {
          break;
        }
      }
    }
    
    //- rjf: numerics
    if(token_flags == 0 && (('0' <= *byte && *byte <= '9') ||
                            (*byte == '.' && byte+1 < byte_opl && '0' <= byte[1] && byte[1] <= '9') ||
                            (*byte == '-' && byte+1 < byte_opl && '0' <= byte[1] && byte[1] <= '9') ||
                            *byte == '_'))
    {
      token_flags = MD_TokenFlag_Numeric;
      token_start = byte;
      token_opl = byte;
      byte += 1;
      for(;byte <= byte_opl; byte += 1)
      {
        token_opl += 1;
        if(byte == byte_opl ||
           (!('A' <= *byte && *byte <= 'Z') &&
            !('a' <= *byte && *byte <= 'z') &&
            !('0' <= *byte && *byte <= '9') &&
            *byte != '_' &&
            *byte != '.'))
        {
          break;
        }
      }
    }
    
    //- rjf: triplet string literals
    if(token_flags == 0 && byte+2 < byte_opl &&
       ((byte[0] == '"' && byte[1] == '"' && byte[2] == '"') ||
        (byte[0] == '\''&& byte[1] == '\''&& byte[2] == '\'') ||
        (byte[0] == '`' && byte[1] == '`' && byte[2] == '`')))
    {
      U8 literal_style = byte[0];
      token_flags = MD_TokenFlag_StringLiteral|MD_TokenFlag_StringTriplet;
      token_flags |= (literal_style == '\'')*MD_TokenFlag_StringSingleQuote;
      token_flags |= (literal_style ==  '"')*MD_TokenFlag_StringDoubleQuote;
      token_flags |= (literal_style ==  '`')*MD_TokenFlag_StringTick;
      token_start = byte;
      token_opl = byte+3;
      byte += 3;
      for(;byte <= byte_opl; byte += 1)
      {
        if(byte == byte_opl)
        {
          token_flags |= MD_TokenFlag_BrokenStringLiteral;
          token_opl = byte;
          break;
        }
        if(byte+2 < byte_opl && (byte[0] == literal_style && byte[1] == literal_style && byte[2] == literal_style))
        {
          byte += 3;
          token_opl = byte;
          break;
        }
      }
    }
    
    //- rjf: singlet string literals
    if(token_flags == 0 && (byte[0] == '"' || byte[0] == '\'' || byte[0] == '`'))
    {
      U8 literal_style = byte[0];
      token_flags = MD_TokenFlag_StringLiteral;
      token_flags |= (literal_style == '\'')*MD_TokenFlag_StringSingleQuote;
      token_flags |= (literal_style ==  '"')*MD_TokenFlag_StringDoubleQuote;
      token_flags |= (literal_style ==  '`')*MD_TokenFlag_StringTick;
      token_start = byte;
      token_opl = byte+1;
      byte += 1;
      B32 escaped = 0;
      for(;byte <= byte_opl; byte += 1)
      {
        if(byte == byte_opl || *byte == '\n')
        {
          token_opl = byte;
          token_flags |= MD_TokenFlag_BrokenStringLiteral;
          break;
        }
        if(!escaped && byte[0] == '\\')
        {
          escaped = 1;
        }
        else if(!escaped && byte[0] == literal_style)
        {
          token_opl = byte+1;
          byte += 1;
          break;
        }
        else if(escaped)
        {
          escaped = 0;
        }
      }
    }
    
    //- rjf: non-reserved symbols
    if(token_flags == 0 && (*byte == '~' || *byte == '!' || *byte == '$' || *byte == '%' || *byte == '^' ||
                            *byte == '&' || *byte == '*' || *byte == '-' || *byte == '=' || *byte == '+' ||
                            *byte == '<' || *byte == '.' || *byte == '>' || *byte == '/' || *byte == '?' ||
                            *byte == '|'))
    {
      token_flags = MD_TokenFlag_Symbol;
      token_start = byte;
      token_opl = byte;
      byte += 1;
      for(;byte <= byte_opl; byte += 1)
      {
        token_opl += 1;
        if(byte == byte_opl ||
           (*byte != '~' && *byte != '!' && *byte != '$' && *byte != '%' && *byte != '^' &&
            *byte != '&' && *byte != '*' && *byte != '-' && *byte != '=' && *byte != '+' &&
            *byte != '<' && *byte != '.' && *byte != '>' && *byte != '/' && *byte != '?' &&
            *byte != '|'))
        {
          break;
        }
      }
    }
    
    //- rjf: reserved symbols
    if(token_flags == 0 && (*byte == '{' || *byte == '}' || *byte == '(' || *byte == ')' ||
                            *byte == '[' || *byte == ']' || *byte == '#' || *byte == ',' ||
                            *byte == '\\'|| *byte == ':' || *byte == ';' || *byte == '@'))
    {
      token_flags = MD_TokenFlag_Reserved;
      token_start = byte;
      token_opl = byte+1;
      byte += 1;
    }
    
    //- rjf: bad characters in all other cases
    if(token_flags == 0)
    {
      token_flags = MD_TokenFlag_BadCharacter;
      token_start = byte;
      token_opl = byte+1;
      byte += 1;
    }
    
    //- rjf; push token if formed
    if(token_flags != 0 && token_start != 0 && token_opl > token_start)
    {
      MD_Token token = {{(U64)(token_start - byte_first), (U64)(token_opl - byte_first)}, token_flags};
      md_token_chunk_list_push(scratch.arena, &tokens, 4096, token);
    }
    
    //- rjf: push errors on unterminated comments
    if(token_flags & MD_TokenFlag_BrokenComment)
    {
      MD_Node *error = md_push_node(arena, MD_NodeKind_ErrorMarker, 0, str8_lit(""), str8_lit(""), token_start - byte_first);
      String8 error_string = str8_lit("Unterminated comment.");
      md_msg_list_push(arena, &msgs, error, MD_MsgKind_Error, error_string);
    }
    
    //- rjf: push errors on unterminated strings
    if(token_flags & MD_TokenFlag_BrokenStringLiteral)
    {
      MD_Node *error = md_push_node(arena, MD_NodeKind_ErrorMarker, 0, str8_lit(""), str8_lit(""), token_start - byte_first);
      String8 error_string = str8_lit("Unterminated string literal.");
      md_msg_list_push(arena, &msgs, error, MD_MsgKind_Error, error_string);
    }
  }
  
  //- rjf: bake, fill & return
  MD_TokenizeResult result = {0};
  {
    result.tokens = md_token_array_from_chunk_list(arena, &tokens);
    result.msgs = msgs;
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Tokens -> Tree Functions

internal MD_ParseResult
md_parse_from_text_tokens(Arena *arena, String8 filename, String8 text, MD_TokenArray tokens)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: set up outputs
  MD_MsgList msgs = {0};
  MD_Node *root = md_push_node(arena, MD_NodeKind_File, 0, filename, text, 0);
  
  //- rjf: set up parse rule stack
  typedef enum MD_ParseWorkKind
  {
    MD_ParseWorkKind_Main,
    MD_ParseWorkKind_MainImplicit,
    MD_ParseWorkKind_NodeOptionalFollowUp,
    MD_ParseWorkKind_NodeChildrenStyleScan,
  }
  MD_ParseWorkKind;
  typedef struct MD_ParseWorkNode MD_ParseWorkNode;
  struct MD_ParseWorkNode
  {
    MD_ParseWorkNode *next;
    MD_ParseWorkKind kind;
    MD_Node *parent;
    MD_Node *first_gathered_tag;
    MD_Node *last_gathered_tag;
    MD_NodeFlags gathered_node_flags;
    S32 counted_newlines;
  };
  MD_ParseWorkNode first_work =
  {
    0,
    MD_ParseWorkKind_Main,
    root,
  };
  MD_ParseWorkNode broken_work = { 0, MD_ParseWorkKind_Main, root,};
  MD_ParseWorkNode *work_top = &first_work;
  MD_ParseWorkNode *work_free = 0;
#define MD_ParseWorkPush(work_kind, work_parent) do\
{\
MD_ParseWorkNode *work_node = work_free;\
if(work_node == 0) {work_node = push_array(scratch.arena, MD_ParseWorkNode, 1);}\
else { SLLStackPop(work_free); }\
work_node->kind = (work_kind);\
work_node->parent = (work_parent);\
SLLStackPush(work_top, work_node);\
}while(0)
#define MD_ParseWorkPop() do\
{\
SLLStackPop(work_top);\
if(work_top == 0) {work_top = &broken_work;}\
}while(0)
  
  //- rjf: parse
  MD_Token *tokens_first = tokens.v;
  MD_Token *tokens_opl = tokens_first + tokens.count;
  MD_Token *token = tokens_first;
  for(;token < tokens_opl;)
  {
    //- rjf: unpack token
    String8 token_string = str8_substr(text, token[0].range);
    
    //- rjf: whitespace -> always no-op & inc
    if(token->flags & MD_TokenFlag_Whitespace)
    {
      token += 1;
      goto end_consume;
    }
    
    //- rjf: comments -> always no-op & inc
    if(token->flags & MD_TokenGroup_Comment)
    {
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [node follow up] : following label -> work top parent has children. we need
    // to scan for explicit delimiters, else parse an implicitly delimited set of children
    if(work_top->kind == MD_ParseWorkKind_NodeOptionalFollowUp && str8_match(token_string, str8_lit(":"), 0))
    {
      MD_Node *parent = work_top->parent;
      MD_ParseWorkPop();
      MD_ParseWorkPush(MD_ParseWorkKind_NodeChildrenStyleScan, parent);
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [node follow up] anything but : following label -> node has no children. just
    // pop & move on
    if(work_top->kind == MD_ParseWorkKind_NodeOptionalFollowUp)
    {
      MD_ParseWorkPop();
      goto end_consume;
    }
    
    //- rjf: [main] separators -> mark & inc
    if(work_top->kind == MD_ParseWorkKind_Main && token->flags & MD_TokenFlag_Reserved &&
       (str8_match(token_string, str8_lit(","), 0) ||
        str8_match(token_string, str8_lit(";"), 0)))
    {
      MD_Node *parent = work_top->parent;
      if(!md_node_is_nil(parent->last))
      {
        parent->last->flags |=     MD_NodeFlag_IsBeforeComma*!!str8_match(token_string, str8_lit(","), 0);
        parent->last->flags |= MD_NodeFlag_IsBeforeSemicolon*!!str8_match(token_string, str8_lit(";"), 0);
        work_top->gathered_node_flags |=     MD_NodeFlag_IsAfterComma*!!str8_match(token_string, str8_lit(","), 0);
        work_top->gathered_node_flags |= MD_NodeFlag_IsAfterSemicolon*!!str8_match(token_string, str8_lit(";"), 0);
      }
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [main_implicit] separators -> pop
    if(work_top->kind == MD_ParseWorkKind_MainImplicit && token->flags & MD_TokenFlag_Reserved &&
       (str8_match(token_string, str8_lit(","), 0) ||
        str8_match(token_string, str8_lit(";"), 0)))
    {
      MD_ParseWorkPop();
      goto end_consume;
    }
    
    //- rjf: [main, main_implicit] unexpected reserved tokens
    if((work_top->kind == MD_ParseWorkKind_Main || work_top->kind == MD_ParseWorkKind_MainImplicit) &&
       token->flags & MD_TokenFlag_Reserved &&
       (str8_match(token_string, str8_lit("#"), 0) ||
        str8_match(token_string, str8_lit("\\"), 0) ||
        str8_match(token_string, str8_lit(":"), 0)))
    {
      MD_Node *error = md_push_node(arena, MD_NodeKind_ErrorMarker, 0, token_string, token_string, token->range.min);
      String8 error_string = push_str8f(arena, "Unexpected reserved symbol \"%S\".", token_string);
      md_msg_list_push(arena, &msgs, error, MD_MsgKind_Error, error_string);
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [main, main_implicit] tag signifier -> create new tag
    if((work_top->kind == MD_ParseWorkKind_Main || work_top->kind == MD_ParseWorkKind_MainImplicit) &&
       token[0].flags & MD_TokenFlag_Reserved && str8_match(token_string, str8_lit("@"), 0))
    {
      if(token+1 >= tokens_opl ||
         !(token[1].flags & MD_TokenGroup_Label))
      {
        MD_Node *error = md_push_node(arena, MD_NodeKind_ErrorMarker, 0, token_string, token_string, token->range.min);
        String8 error_string = str8_lit("Tag label expected after @ symbol.");
        md_msg_list_push(arena, &msgs, error, MD_MsgKind_Error, error_string);
        token += 1;
        goto end_consume;
      }
      else
      {
        String8 tag_name_raw = str8_substr(text, token[1].range);
        String8 tag_name = md_content_string_from_token_flags_str8(token[1].flags, tag_name_raw);
        MD_Node *node = md_push_node(arena, MD_NodeKind_Tag, md_node_flags_from_token_flags(token[1].flags), tag_name, tag_name_raw, token[0].range.min);
        DLLPushBack_NPZ(&md_nil_node, work_top->first_gathered_tag, work_top->last_gathered_tag, node, next, prev);
        if(token+2 < tokens_opl && token[2].flags & MD_TokenFlag_Reserved && str8_match(str8_substr(text, token[2].range), str8_lit("("), 0))
        {
          token += 3;
          MD_ParseWorkPush(MD_ParseWorkKind_Main, node);
        }
        else
        {
          token += 2;
        }
        goto end_consume;
      }
    }
    
    //- rjf: [main, main_implicit] label -> create new main
    if((work_top->kind == MD_ParseWorkKind_Main || work_top->kind == MD_ParseWorkKind_MainImplicit) &&
       token->flags & MD_TokenGroup_Label)
    {
      String8 node_string_raw = token_string;
      String8 node_string = md_content_string_from_token_flags_str8(token->flags, node_string_raw);
      MD_NodeFlags flags = md_node_flags_from_token_flags(token->flags)|work_top->gathered_node_flags;
      work_top->gathered_node_flags = 0;
      MD_Node *node = md_push_node(arena, MD_NodeKind_Main, flags, node_string, node_string_raw, token[0].range.min);
      node->first_tag = work_top->first_gathered_tag;
      node->last_tag = work_top->last_gathered_tag;
      for(MD_Node *tag = work_top->first_gathered_tag; !md_node_is_nil(tag); tag = tag->next)
      {
        tag->parent = node;
      }
      work_top->first_gathered_tag = work_top->last_gathered_tag = &md_nil_node;
      md_node_push_child(work_top->parent, node);
      MD_ParseWorkPush(MD_ParseWorkKind_NodeOptionalFollowUp, node);
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [main] {s, [s, and (s -> create new main
    if(work_top->kind == MD_ParseWorkKind_Main && token->flags & MD_TokenFlag_Reserved &&
       (str8_match(token_string, str8_lit("{"), 0) ||
        str8_match(token_string, str8_lit("["), 0) ||
        str8_match(token_string, str8_lit("("), 0)))
    {
      MD_NodeFlags flags = md_node_flags_from_token_flags(token->flags)|work_top->gathered_node_flags;
      flags |=   MD_NodeFlag_HasBraceLeft*!!str8_match(token_string, str8_lit("{"), 0);
      flags |= MD_NodeFlag_HasBracketLeft*!!str8_match(token_string, str8_lit("["), 0);
      flags |=   MD_NodeFlag_HasParenLeft*!!str8_match(token_string, str8_lit("("), 0);
      work_top->gathered_node_flags = 0;
      MD_Node *node = md_push_node(arena, MD_NodeKind_Main, flags, str8_lit(""), str8_lit(""), token[0].range.min);
      node->first_tag = work_top->first_gathered_tag;
      node->last_tag = work_top->last_gathered_tag;
      for(MD_Node *tag = work_top->first_gathered_tag; !md_node_is_nil(tag); tag = tag->next)
      {
        tag->parent = node;
      }
      work_top->first_gathered_tag = work_top->last_gathered_tag = &md_nil_node;
      md_node_push_child(work_top->parent, node);
      MD_ParseWorkPush(MD_ParseWorkKind_Main, node);
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [node children style scan] {s, [s, and (s -> explicitly delimited children
    if(work_top->kind == MD_ParseWorkKind_NodeChildrenStyleScan && token->flags & MD_TokenFlag_Reserved &&
       (str8_match(token_string, str8_lit("{"), 0) ||
        str8_match(token_string, str8_lit("["), 0) ||
        str8_match(token_string, str8_lit("("), 0)))
    {
      MD_Node *parent = work_top->parent;
      parent->flags |=   MD_NodeFlag_HasBraceLeft*!!str8_match(token_string, str8_lit("{"), 0);
      parent->flags |= MD_NodeFlag_HasBracketLeft*!!str8_match(token_string, str8_lit("["), 0);
      parent->flags |=   MD_NodeFlag_HasParenLeft*!!str8_match(token_string, str8_lit("("), 0);
      MD_ParseWorkPop();
      MD_ParseWorkPush(MD_ParseWorkKind_Main, parent);
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [node children style scan] count newlines
    if(work_top->kind == MD_ParseWorkKind_NodeChildrenStyleScan && token->flags & MD_TokenFlag_Newline)
    {
      work_top->counted_newlines += 1;
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [main_implicit] newline -> pop
    if(work_top->kind == MD_ParseWorkKind_MainImplicit && token->flags & MD_TokenFlag_Newline)
    {
      MD_ParseWorkPop();
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [all but main_implicit] newline -> no-op & inc
    if(work_top->kind != MD_ParseWorkKind_MainImplicit && token->flags & MD_TokenFlag_Newline)
    {
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [node children style scan] anything causing implicit set -> <2 newlines, all good,
    // >=2 newlines, houston we have a problem
    if(work_top->kind == MD_ParseWorkKind_NodeChildrenStyleScan)
    {
      if(work_top->counted_newlines >= 2)
      {
        MD_Node *node = work_top->parent;
        MD_Node *error = md_push_node(arena, MD_NodeKind_ErrorMarker, 0, token_string, token_string, token->range.min);
        String8 error_string = push_str8f(arena, "More than two newlines following \"%S\", which has implicitly-delimited children, resulting in an empty list of children.", node->string);
        md_msg_list_push(arena, &msgs, error, MD_MsgKind_Warning, error_string);
        MD_ParseWorkPop();
      }
      else
      {
        MD_Node *parent = work_top->parent;
        MD_ParseWorkPop();
        MD_ParseWorkPush(MD_ParseWorkKind_MainImplicit, parent);
      }
      goto end_consume;
    }
    
    //- rjf: [main] }s, ]s, and )s -> pop
    if(work_top->kind == MD_ParseWorkKind_Main && token->flags & MD_TokenFlag_Reserved &&
       (str8_match(token_string, str8_lit("}"), 0) ||
        str8_match(token_string, str8_lit("]"), 0) ||
        str8_match(token_string, str8_lit(")"), 0)))
    {
      MD_Node *parent = work_top->parent;
      parent->flags |=   MD_NodeFlag_HasBraceRight*!!str8_match(token_string, str8_lit("}"), 0);
      parent->flags |= MD_NodeFlag_HasBracketRight*!!str8_match(token_string, str8_lit("]"), 0);
      parent->flags |=   MD_NodeFlag_HasParenRight*!!str8_match(token_string, str8_lit(")"), 0);
      MD_ParseWorkPop();
      token += 1;
      goto end_consume;
    }
    
    //- rjf: [main implicit] }s, ]s, and )s -> pop without advancing
    if(work_top->kind == MD_ParseWorkKind_MainImplicit && token->flags & MD_TokenFlag_Reserved &&
       (str8_match(token_string, str8_lit("}"), 0) ||
        str8_match(token_string, str8_lit("]"), 0) ||
        str8_match(token_string, str8_lit(")"), 0)))
    {
      MD_ParseWorkPop();
      goto end_consume;
    }
    
    //- rjf: no consumption -> unexpected token! we don't know what to do with this.
    {
      MD_Node *error = md_push_node(arena, MD_NodeKind_ErrorMarker, 0, token_string, token_string, token->range.min);
      String8 error_string = push_str8f(arena, "Unexpected \"%S\" token.", token_string);
      md_msg_list_push(arena, &msgs, error, MD_MsgKind_Error, error_string);
      token += 1;
    }
    
    end_consume:;
  }
  
  //- rjf: fill & return
  MD_ParseResult result = {0};
  result.root = root;
  result.msgs = msgs;
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Tree -> Text Functions

internal String8List
md_debug_string_list_from_tree(Arena *arena, MD_Node *root)
{
  String8List strings = {0};
  {
    char *indentation = "                                                                                                                                ";
    S32 depth = 0;
    for(MD_Node *node = root, *next = &md_nil_node; !md_node_is_nil(node); node = next)
    {
      // rjf: get next recursion
      MD_NodeRec rec = md_node_rec_depth_first_pre(node, root);
      next = rec.next;
      
      // rjf: extract node info
      String8 kind_string = str8_lit("Unknown");
      switch(node->kind)
      {
        default:{}break;
        case MD_NodeKind_File:       {kind_string = str8_lit("File");       }break;
        case MD_NodeKind_ErrorMarker:{kind_string = str8_lit("ErrorMarker");}break;
        case MD_NodeKind_Main:       {kind_string = str8_lit("Main");       }break;
        case MD_NodeKind_Tag:        {kind_string = str8_lit("Tag");        }break;
        case MD_NodeKind_List:       {kind_string = str8_lit("List");       }break;
        case MD_NodeKind_Reference:  {kind_string = str8_lit("Reference");  }break;
      }
      
      // rjf: push node line
      str8_list_pushf(arena, &strings, "%.*s\"%S\" : %S", depth, indentation, node->string, kind_string);
      
      // rjf: children -> open brace
      if(rec.push_count != 0)
      {
        str8_list_pushf(arena, &strings, "%.*s{", depth, indentation);
      }
      
      // rjf: descend
      depth += rec.push_count;
      
      // rjf: popping -> close braces
      for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1)
      {
        str8_list_pushf(arena, &strings, "%.*s}", depth-1-pop_idx, indentation);
      }
      
      // rjf: ascend
      depth -= rec.pop_count;
    }
  }
  return strings;
}
