#define T_Group "MD" 

TEST(md_tokenizer)
{
  MD_TokenizeResult result;

  // empty
  result = md_tokenize_from_text(arena, str8_lit(""));
  T_Ok(result.tokens.count == 0);
  T_Ok(result.msgs.count == 0);

  // identifiers
  char *ids[] = {
    "a",
    "abc",
    "_",
    "_abc",
    "abc123",
    "abc_def123",
  };
  for EachElement(i, ids) {
    String8 in = str8_cstring(ids[i]);
    result = md_tokenize_from_text(arena, in);

    T_Ok(result.tokens.count == 1);
    T_Ok(result.tokens.v[0].range.min == 0);
    T_Ok(result.tokens.v[0].range.max == in.size);
    T_Ok(result.tokens.v[0].flags & MD_TokenFlag_Identifier);
    T_Ok((result.tokens.v[0].flags & ~MD_TokenFlag_Identifier) == 0);
    T_Ok(result.tokens.v[0].line_num == 1);
    T_Ok(result.tokens.v[0].col_num == 1);
    T_Ok(result.msgs.count == 0);
  }

  // numerics
  char *numerics[] = {
    "0",
    "123",
    "123.456",
    ".5",
    "-1",
    "-123.45"
    "123_abc",
    "1.2.3",
  };
  for EachElement(i, numerics) {
    String8 in = str8_cstring(numerics[i]);
    result = md_tokenize_from_text(arena, in);

    T_Ok(result.tokens.count == 1);
    T_Ok(result.tokens.v[0].range.min == 0);
    T_Ok(result.tokens.v[0].range.max == in.size);
    T_Ok(result.tokens.v[0].flags & MD_TokenFlag_Numeric);
    T_Ok((result.tokens.v[0].flags & ~MD_TokenFlag_Numeric) == 0);
    T_Ok(result.tokens.v[0].line_num == 1);
    T_Ok(result.tokens.v[0].col_num == 1);
    T_Ok(result.msgs.count == 0);
  }

  // comments
  char *good_comments[] = {
    "/**/",
    "/* */",
    "/* abc */",
    "/* ** */",
    "/**/",
    "/**************/",
    "/*abc*/",
    "//foo",
    "// bar",
    "//"
  };
  B32 good_comments_passed = 0;
  for EachElement(i, good_comments) {
    String8 in = str8_cstring(good_comments[i]);
    result = md_tokenize_from_text(arena, in);

    T_Ok(result.tokens.count == 1);
    T_Ok(dim_1u64(result.tokens.v[0].range) == in.size);
    T_Ok(result.tokens.v[0].flags & MD_TokenFlag_Comment);
    T_Ok((result.tokens.v[0].flags & ~MD_TokenFlag_Comment) == 0);
    T_Ok(result.tokens.v[0].line_num == 1);
    T_Ok(result.tokens.v[0].col_num == 1);
    T_Ok(result.msgs.count == 0);
  }

  char *broken_comments[] = {
    "/*",
    "/*/",
  };
  for EachElement(i, broken_comments) {
    String8 in = str8_cstring(broken_comments[i]);
    result = md_tokenize_from_text(arena, in);
    T_Ok(result.msgs.worst_message_kind == MD_MsgKind_Error);
    T_Ok(result.msgs.count > 0);
  }

  // unterminated string
  result = md_tokenize_from_text(arena, str8_lit("\"abc\nc"));
  T_Ok(result.msgs.worst_message_kind == MD_MsgKind_Error);
  T_Ok(result.msgs.count > 0);

  // terminated string
  //
  // TODO: strings with \n in the middle are not correctly tokenized
  result = md_tokenize_from_text(arena, str8_lit("\"abc\""));
  T_Ok(result.msgs.count == 0);

  // line column tracking
  {
    result = md_tokenize_from_text(arena, str8_lit("a\nb"));
    T_Ok(result.tokens.count == 3);

    // verify 'a'
    T_Ok(result.tokens.v[0].flags & MD_TokenFlag_Identifier);
    T_Ok(result.tokens.v[0].line_num == 1);
    T_Ok(result.tokens.v[0].col_num == 1);

    // verify '\n'
    T_Ok(result.tokens.v[1].flags & MD_TokenFlag_Newline);
    T_Ok(result.tokens.v[1].line_num == 1);
    T_Ok(result.tokens.v[1].col_num == 2);

    // verify 'b'
    T_Ok(result.tokens.v[2].flags & MD_TokenFlag_Identifier);
    T_Ok(result.tokens.v[2].line_num == 2);
    T_Ok(result.tokens.v[2].col_num == 1);
  }
  {
    result = md_tokenize_from_text(arena, str8_lit("    abc"));
    T_Ok(result.tokens.count == 2);
    T_Ok(result.msgs.count == 0);

    // verify white space
    T_Ok(result.tokens.v[0].flags & MD_TokenFlag_Whitespace);
    T_Ok(result.tokens.v[0].line_num == 1);
    T_Ok(result.tokens.v[0].col_num == 1);

    // verify identifier
    T_Ok(result.tokens.v[1].flags & MD_TokenFlag_Identifier);
    T_Ok(result.tokens.v[1].line_num == 1);
    T_Ok(result.tokens.v[1].col_num == 5);
  }
}

#undef T_Group
