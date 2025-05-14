// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct KeyValuePair
{
  union {
    String8 key_string;
    void   *key_raw;
    U32     key_u32;
    U64     key_u64;
  };
  union {
    String8  value_string;
    void    *value_raw;
    U32      value_u32;
    U64      value_u64;
  };
} KeyValuePair;

typedef struct BucketNode
{
  struct BucketNode *next;
  KeyValuePair       v;
} BucketNode;

typedef struct BucketList
{
  BucketNode *first;
  BucketNode *last;
} BucketList;

typedef struct HashTable
{
  U64         count;
  U64         cap;
  BucketList *buckets;
  BucketList  free_buckets;
} HashTable;

////////////////////////////////

//- bucket list helpers

internal void         bucket_list_concat_in_place(BucketList *list, BucketList *to_concat);
internal BucketNode * bucket_list_pop(BucketList *list);

//- main

internal U64         hash_table_hasher(String8 string);
internal HashTable * hash_table_init(Arena *arena, U64 cap);
internal void        hash_table_purge(HashTable *ht);

//- push

internal BucketNode * hash_table_push              (Arena *arena, HashTable *ht, U64 hash,     KeyValuePair  v);
internal BucketNode * hash_table_push_u32_string   (Arena *arena, HashTable *ht, U32     key,  String8       value);
internal BucketNode * hash_table_push_u64_string   (Arena *arena, HashTable *ht, U64     key,  String8       value);
internal BucketNode * hash_table_push_string_string(Arena *arena, HashTable *ht, String8 key,  String8       value);
internal BucketNode * hash_table_push_path_string  (Arena *arena, HashTable *ht, String8 key,  String8       value);
internal BucketNode * hash_table_push_u32_raw      (Arena *arena, HashTable *ht, U32     key,  void         *value);
internal BucketNode * hash_table_push_u64_raw      (Arena *arena, HashTable *ht, U64     key,  void         *value);
internal BucketNode * hash_table_push_path_raw     (Arena *arena, HashTable *ht, String8 path, void         *value);
internal BucketNode * hash_table_push_path_u64     (Arena *arena, HashTable *ht, String8 path, U64           value);
internal BucketNode * hash_table_push_u64_u64      (Arena *arena, HashTable *ht, U64     key,  U64           value);

//- search

internal KeyValuePair * hash_table_search_string  (HashTable *ht, String8 string);
internal KeyValuePair * hash_table_search_u32     (HashTable *ht, U32 key       );
internal KeyValuePair * hash_table_search_u64     (HashTable *ht, U64 key       );
internal KeyValuePair * hash_table_search_path    (HashTable *ht, String8 path  );
internal void *         hash_table_search_path_raw(HashTable *ht, String8 path  );

internal B32 hash_table_search_path_u64(HashTable *ht, String8 key, U64 *value_out);

//- key-value helpers

internal U32 *          keys_from_hash_table_u32       (Arena *arena, HashTable *ht);
internal U64 *          keys_from_hash_table_u64       (Arena *arena, HashTable *ht);
internal KeyValuePair * key_value_pairs_from_hash_table(Arena *arena, HashTable *ht);

internal void sort_key_value_pairs_as_u32(KeyValuePair *pairs, U64 count);
internal void sort_key_value_pairs_as_u64(KeyValuePair *pairs, U64 count);

////////////////////////////////

internal U64Array    remove_duplicates_u64_array(Arena *arena, U64Array arr);
internal String8List remove_duplicates_str8_list(Arena *arena, String8List list);

