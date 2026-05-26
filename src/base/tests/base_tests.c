// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

Test(str8_list_substr)
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

TEST(hash_map)
{
  {
    HashMap hm = {0};
    
    U64 test_count = 256;
    for EachIndex(i, test_count) { hash_map_push_u64_u64(arena, &hm, i, i*2); }
    
    // test tombstone reuse
    for (U64 i = 10; i < test_count; ++i) {
      HashMapNode *test_node = hash_map_search(&hm, hash_map_hasher(str8_struct(&i)), (HashMapKey){ .key_u64 = i }, hash_map_match_u64);
      T_Ok(hash_map_purge_u64(&hm, i) == 1);
      T_Ok(hash_map_push_u64_u64(arena, &hm, i, 10) == test_node);
    }
    
    // delete some items and after search them
    for (U64 i = 0; i < 10; ++i) {
      T_Ok(hash_map_purge_u64(&hm, i));
    }
    T_Ok(hm.tombstone_count == 10);
    for (U64 i = 0; i < 10; ++i) {
      T_Ok(hash_map_search_u64_raw(&hm, i) == 0);
    }
    
    // extract pairs and test sorting
    HashMapKeyValue *pairs = key_value_from_hash_map(arena, &hm);
    sort_hash_map_key_value_u64(pairs, hm.count);
    for (U64 i = 1; i < hm.count; ++i) {
      T_Ok(pairs[i-1].key.key_u64 < pairs[i].key.key_u64);
    }
    
    // did purge put all the nodes on free list?
    hash_map_purge(&hm);
    U64 free_list_count = 0;
    for (HashMapNode *n = hm.free_list; n != 0; n = n->next) { free_list_count += 1; }
    T_Ok(free_list_count == test_count);
  }
  
  // test search through tombstones
  {
    HashMap hm = {0};
    
    U64 hash = 12345;
    
    hash_map_push(arena, &hm, hash, (HashMapKeyValue){ .key = { .key_u64 = 1 }, .value = { .value_u64 = 10 } }, hash_map_match_u64);
    hash_map_push(arena, &hm, hash, (HashMapKeyValue){ .key = { .key_u64 = 2 }, .value = { .value_u64 = 20 } }, hash_map_match_u64);
    hash_map_push(arena, &hm, hash, (HashMapKeyValue){ .key = { .key_u64 = 3 }, .value = { .value_u64 = 30 } }, hash_map_match_u64);
    
    T_Ok(hash_map_purge_u64(&hm, 1));
    
    T_Ok(hash_map_search(&hm, hash, (HashMapKey){ .key_u64 = 2 }, hash_map_match_u64)->v.value.value_u64 == 20);
    T_Ok(hash_map_search(&hm, hash, (HashMapKey){ .key_u64 = 3 }, hash_map_match_u64)->v.value.value_u64 == 30);
  }
  
  // interleaved purges
  {
    HashMap hm = {0};
    for EachIndex(round, 100) {
      for EachIndex(i, 256) { hash_map_push_u64_u64(arena, &hm, i, round); }
      for (U64 i = 0; i < 256; i += 2) { hash_map_purge_u64(&hm, i); }
      for (U64 i = 0; i < 256; i += 2) { hash_map_push_u64_u64(arena, &hm, i, round + 1); }
      
      T_Ok(hm.count == 256);
      T_Ok(hm.tombstone_count == 0);
    }
    
    for EachIndex(i, 100) {
      T_Ok(hash_map_purge_u64(&hm, 123123123 + i) == 0);
    }
  }
  
  // empty hash map test
  {
    HashMap hm = {0};
    
    T_Ok(hash_map_search_u64_raw(&hm, 123) == 0);
    T_Ok(hash_map_purge_u64(&hm, 123) == 0);
    
    HashMapKeyValue *pairs = key_value_from_hash_map(arena, &hm);
    T_Ok(pairs == 0 || hm.count == 0);
    
    hash_map_purge(&hm);
    T_Ok(hm.count == 0);
    T_Ok(hm.tombstone_count == 0);
  }
  
  // heavy collision
  {
    HashMap hm = {0};
    
    U64 hash = 12345;
    U64 count = 1024;
    
    for EachIndex(i, count) {
      hash_map_push(arena, &hm, hash, (HashMapKeyValue){ .key = { .key_u64 = i }, .value = { .value_u64 = i*3 } }, hash_map_match_u64);
    }
    
    for EachIndex(i, count) {
      T_Ok(hash_map_search(&hm, hash, (HashMapKey){ .key_u64 = i }, hash_map_match_u64)->v.value.value_u64 == i*3);
    }
  }
  
  // duplicate key test
  {
    HashMap hm = {0};
    
    U64 a = 1;
    U64 b = 2;
    
    HashMapNode *n0 = hash_map_push_u64_u64(arena, &hm, 1, a);
    HashMapNode *n1 = hash_map_push_u64_u64(arena, &hm, 1, b);
    
    T_Ok(hm.count == 1);
    T_Ok(n0 == n1);
    U64 test = *hash_map_search_u64_u64(&hm, 1);
    T_Ok(test != b);
  }
  
  // test paths
  {
    HashMap hm = {0};
    
    U64 foo = 0;
    U64 bar = 1;
    
    hash_map_push_path_raw(arena, &hm, str8_lit("c:/DEVEL/test"),   &foo);
    hash_map_push_path_raw(arena, &hm, str8_lit("c:\\DEVEL\\test"), &foo);
    hash_map_push_path_raw(arena, &hm, str8_lit("c:/devel\\test"),  &foo);
    
    hash_map_push_path_raw(arena, &hm, str8_lit("/mnt/devel"), &bar);
    hash_map_push_path_raw(arena, &hm, str8_lit("/MNT/devel"), &bar);
    
    T_Ok(hm.count == 2);
    
    T_Ok(hash_map_search_path_raw(&hm, str8_lit("c:/devel/test"))   == &foo);
    T_Ok(hash_map_search_path_raw(&hm, str8_lit("c:\\devel\\TEST")) == &foo);
    T_Ok(hash_map_search_path_raw(&hm, str8_lit("/mnt/devel")) == &bar);
    T_Ok(hash_map_search_path_raw(&hm, str8_lit("/MNT/DEVEL")) == &bar);
  }
  
  // test strings
  {
    HashMap hm = {0};
    
    U64 foo = 0;
    U64 bar = 1;
    
    hash_map_push_string_raw(arena, &hm, str8_lit("c:/DEVEL/test"),   &foo);
    hash_map_push_string_raw(arena, &hm, str8_lit("c:\\DEVEL\\test"), &foo);
    hash_map_push_string_raw(arena, &hm, str8_lit("c:/devel\\test"),  &foo);
    
    hash_map_push_string_raw(arena, &hm, str8_lit("/mnt/devel"), &bar);
    hash_map_push_string_raw(arena, &hm, str8_lit("/MNT/devel"), &bar);
    
    T_Ok(hm.count == 5);
    
    T_Ok(hash_map_search_string_raw(&hm, str8_lit("c:/DEVEL/test"))   == &foo);
    T_Ok(hash_map_search_string_raw(&hm, str8_lit("c:\\DEVEL\\test")) == &foo);
    T_Ok(hash_map_search_string_raw(&hm, str8_lit("c:/devel\\test"))  == &foo);
    
    T_Ok(hash_map_search_string_raw(&hm, str8_lit("/mnt/devel")) == &bar);
    T_Ok(hash_map_search_string_raw(&hm, str8_lit("/MNT/devel")) == &bar);
  }
}
