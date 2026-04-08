// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define T_Group "Base"

T_BeginTest(str8_list_substr)
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

#undef T_Group
