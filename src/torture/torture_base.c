// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define T_Group "Base"

TEST(str8_list_substr)
{
  String8List zero_list = {0};

  {
    String8List list = {0};
    str8_list_pushf(arena, &list, "a");
    str8_list_pushf(arena, &list, "b");
    str8_list_pushf(arena, &list, "c");
    String8List sub = str8_list_substr(arena, list, r1u64(0, 3));
    String8 result = str8_list_join(arena, &list, 0);
    T_Ok(str8_match(result, str8_lit("abc"), 0));
  }

  {
    String8List list = {0};
    str8_list_pushf(arena, &list, "a");
    str8_list_pushf(arena, &list, "b");
    str8_list_pushf(arena, &list, "c");
    String8List sub = str8_list_substr(arena, list, r1u64(0, max_U64));
    String8 result = str8_list_join(arena, &list, 0);
    T_Ok(str8_match(result, str8_lit("abc"), 0));
  }

  {

    String8List list = {0};
    str8_list_pushf(arena, &list, "a");
    str8_list_pushf(arena, &list, "bcd");
    String8List sub = str8_list_substr(arena, list, r1u64(2, 3));
    String8 result = str8_list_join(arena, &sub, 0);
    T_Ok(str8_match(result, str8_lit("c"), 0));
  }

  {

    String8List list = {0};
    str8_list_pushf(arena, &list, "a");
    str8_list_pushf(arena, &list, "bcd");
    String8List sub = str8_list_substr(arena, list, r1u64(1, 2));
    String8 result = str8_list_join(arena, &sub, 0);
    T_Ok(str8_match(result, str8_lit("b"), 0));
  }

  {
    String8List list = {0};
    str8_list_pushf(arena, &list, "ab");
    str8_list_pushf(arena, &list, "cd");
    str8_list_pushf(arena, &list, "ef");
    String8List sub = str8_list_substr(arena, list, r1u64(1, 5));
    String8 result = str8_list_join(arena, &sub, 0);
    T_Ok(str8_match(result, str8_lit("bcde"), 0));
  }

  {
    String8List list = {0};
    str8_list_pushf(arena, &list, "abc");

    String8List zero = str8_list_substr(arena, list, r1u64(0, 0));
    T_Ok(MemoryMatchStruct(&zero, &zero_list));

    String8List out_of_bounds_range = str8_list_substr(arena, list, r1u64(max_U64/2, max_U64));
    T_Ok(MemoryMatchStruct(&out_of_bounds_range, &zero_list));
  }
}

TEST(bit_array)
{
  for (U64 start=0; start<32*3; start++) {
    for (U64 end=start; end<32*3; end++) {
      U32 v[3] = { 0 };
      for (U64 i=start; i<end; i++) {
        v[i/32] |= 1 << (i%32);
      }
      for (U64 lo=0; lo<32*3; lo++) {
        for (U64 hi=0; hi<32*3; hi++) {
          U64 expected_idx = Min(hi, end) - 1;
          B32 expected_r = hi <= start || lo >= end || lo >= hi || start >= end ? 0 : 1;
          U64 idx = bit_array_scan_right_to_left32((U32Array){.v=v, .count=ArrayCount(v)}, lo, hi, 1);
          B32 r = idx < hi;
          T_Ok(r == expected_r);
          if (r) {
            T_Ok(idx == expected_idx);

          }
        }
      }
    }
  }
}

TEST(count_digits)
{
  T_Ok(count_digits_u64(0, 10) == 1);
  T_Ok(count_digits_u64(99, 10) == 2);
  T_Ok(count_digits_u64(999, 10) == 3);
  T_Ok(count_digits_u64(9999, 10) == 4);
  T_Ok(count_digits_u64(99999, 10) == 5);
  T_Ok(count_digits_u64(999999, 10) == 6);
}

TEST(match_wildcard)
{
  // empty strings
  T_Ok(str8_match_wildcard(str8_lit(""),  str8_lit(""),   0) == 1);
  T_Ok(str8_match_wildcard(str8_lit(""),  str8_lit("*"),  0) == 1);
  T_Ok(str8_match_wildcard(str8_lit(""),  str8_lit("**"), 0) == 1);
  T_Ok(str8_match_wildcard(str8_lit(""),  str8_lit("?"),  0) == 0);
  T_Ok(str8_match_wildcard(str8_lit(""),  str8_lit("*?"), 0) == 0);
  T_Ok(str8_match_wildcard(str8_lit(""),  str8_lit("?*"), 0) == 0);
  T_Ok(str8_match_wildcard(str8_lit("a"), str8_lit(""),   0) == 0);

  // exact
  T_Ok(str8_match_wildcard(str8_lit("a"), str8_lit("a"), 0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("a"), str8_lit("A"), 0) == 0);

  // ?
  T_Ok(str8_match_wildcard(str8_lit("a"),  str8_lit("?"),    0) == 1);
  T_Ok(str8_match_wildcard(str8_lit(""),   str8_lit("?"),    0) == 0);
  T_Ok(str8_match_wildcard(str8_lit("ab"), str8_lit("?"),    0) == 0);
  T_Ok(str8_match_wildcard(str8_lit("ab"), str8_lit("a?"),   0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("ab"), str8_lit("?b"),   0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("ab"), str8_lit("??"),   0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("a?c"), 0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("ab"), str8_lit("???"),  0) == 0);

  // *
  T_Ok(str8_match_wildcard(str8_lit(""),    str8_lit("*"),      0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("a"),   str8_lit("*"),      0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("*"),      0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("a*"),     0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("*c"),     0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("*b*"),    0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("a*c"),    0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("b*"),     0) == 0);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("**"),     0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("a**c"),   0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("a*b*c"),  0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("*a*d*"),  0) == 0);

  T_Ok(str8_match_wildcard(str8_lit("abcd"),              str8_lit("a*d"),        0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abefcdgiescdfimde"), str8_lit("ab*cd?i*de"), 0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("mississippi"),       str8_lit("m*iss*ppi"),  0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"),               str8_lit("*b"),         0) == 0);
  T_Ok(str8_match_wildcard(str8_lit("a"),                 str8_lit("aa"),         0) == 0);
  T_Ok(str8_match_wildcard(str8_lit("aa"),                str8_lit("a"),          0) == 0);

  // case insensitive
  T_Ok(str8_match_wildcard(str8_lit("a"),      str8_lit("A"),      StringMatchFlag_CaseInsensitive) == 1);
  T_Ok(str8_match_wildcard(str8_lit("FooBar"), str8_lit("foobar"), StringMatchFlag_CaseInsensitive) == 1);
  T_Ok(str8_match_wildcard(str8_lit("Foobar"), str8_lit("foo*"),   StringMatchFlag_CaseInsensitive) == 1);

  // right side sloppy
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("ab"), StringMatchFlag_RightSideSloppy) == 1);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit(""),   StringMatchFlag_RightSideSloppy) == 1);
  T_Ok(str8_match_wildcard(str8_lit(""),    str8_lit("a"),  StringMatchFlag_RightSideSloppy) == 0);

  // slash insensitive
  T_Ok(str8_match_wildcard(str8_lit("a/b"),   str8_lit("a\\b"),    0)                                == 0);
  T_Ok(str8_match_wildcard(str8_lit("a/b"),   str8_lit("a\\b"),    StringMatchFlag_SlashInsensitive) == 1);
  T_Ok(str8_match_wildcard(str8_lit("a/b/c"), str8_lit("a\\*\\c"), StringMatchFlag_SlashInsensitive) == 1);

  // combined
  T_Ok(str8_match_wildcard(str8_lit("Ab\\Cde"), str8_lit("ab/*e"), StringMatchFlag_CaseInsensitive|StringMatchFlag_SlashInsensitive) == 1);

  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("*?*"), 0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("a"),   str8_lit("*?*"), 0) == 1);
  T_Ok(str8_match_wildcard(str8_lit(""),    str8_lit("*?*"), 0) == 0);
  T_Ok(str8_match_wildcard(str8_lit("abc"), str8_lit("?*?"), 0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("ab"),  str8_lit("?*?"), 0) == 1);
  T_Ok(str8_match_wildcard(str8_lit("a"),   str8_lit("?*?"), 0) == 0);
}

#undef T_Group
