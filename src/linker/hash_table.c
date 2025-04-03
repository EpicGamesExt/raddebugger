// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
bucket_list_concat_in_place(BucketList *list, BucketList *to_concat)
{
  if (to_concat->first) {
    if (list->first) {
      list->last->next = to_concat->first;
      list->last = to_concat->last;
    } else {
      list->first = to_concat->first;
      list->last = to_concat->last;
    }
    MemoryZeroStruct(to_concat);
  }
}

internal BucketNode *
bucket_list_pop(BucketList *list)
{
  BucketNode *result = list->first;
  SLLQueuePop(list->first, list->last);
  return result;
}

////////////////////////////////

internal U64
hash_table_hasher(String8 string)
{
  XXH64_hash_t hash64 = XXH3_64bits(string.str, string.size);
  return hash64;
}

internal HashTable *
hash_table_init(Arena *arena, U64 cap)
{
  HashTable *ht = push_array(arena, HashTable, 1);
  ht->cap       = cap;
  ht->buckets   = push_array(arena, BucketList, cap);
  return ht;
}

internal void
hash_table_purge(HashTable *ht)
{
  // reset key count
  ht->count = 0;

  // concat buckets
  for (U64 ibucket = 0; ibucket < ht->cap; ++ibucket) {
    bucket_list_concat_in_place(&ht->free_buckets, &ht->buckets[ibucket]);
  }
}

internal BucketNode *
hash_table_push(Arena *arena, HashTable *ht, U64 hash, KeyValuePair v)
{
  BucketNode *node;
  if (ht->free_buckets.first != 0) {
    node = bucket_list_pop(&ht->free_buckets);
  } else {
    node = push_array(arena, BucketNode, 1);
  }
  node->next = 0;
  node->v    = v;
  
  U64 ibucket = hash % ht->cap;
  SLLQueuePush(ht->buckets[ibucket].first, ht->buckets[ibucket].last, node);
  ++ht->count;
  
  return node;
}

internal BucketNode *
hash_table_push_string_string(Arena *arena, HashTable *ht, String8 key, String8 value)
{
  U64 hash = hash_table_hasher(key);
  return hash_table_push(arena, ht, hash, (KeyValuePair){ .key_string = key, .value_string = value });
}

internal BucketNode *
hash_table_push_string_raw(Arena *arena, HashTable *ht, String8 key, void *value)
{
  U64 hash = hash_table_hasher(key);
  return hash_table_push(arena, ht, hash, (KeyValuePair){ .key_string = key, .value_raw = value });
}

internal BucketNode *
hash_table_push_string_u64(Arena *arena, HashTable *ht, String8 key, U64 value)
{
  U64 hash = hash_table_hasher(key);
  return hash_table_push(arena, ht, hash, (KeyValuePair){.key_string = key, .value_u64 = value });
}

internal BucketNode *
hash_table_push_u32_raw(Arena *arena, HashTable *ht, U32 key, void *value)
{
  U64 hash = hash_table_hasher(str8_struct(&key));
  return hash_table_push(arena, ht, hash, (KeyValuePair){ .key_u32 = key, .value_raw = value });
}

internal BucketNode *
hash_table_push_u32_string(Arena *arena, HashTable *ht, U32 key, String8 value)
{
  U64 hash = hash_table_hasher(str8_struct(&key));
  return hash_table_push(arena, ht, hash, (KeyValuePair){ .key_u32 = key, .value_string = value });
}

internal BucketNode *
hash_table_push_u64_raw(Arena *arena, HashTable *ht, U64 key, void *value)
{
  U64 hash = hash_table_hasher(str8_struct(&key));
  return hash_table_push(arena, ht, hash, (KeyValuePair){ .key_u64 = key, .value_raw = value });
}

internal BucketNode *
hash_table_push_u64_string(Arena *arena, HashTable *ht, U64 key, String8 value)
{
  U64 hash = hash_table_hasher(str8_struct(&key));
  return hash_table_push(arena, ht, hash, (KeyValuePair){ .key_u64 = key, .value_string = value });
}

internal BucketNode *
hash_table_push_u64_u64(Arena *arena, HashTable *ht, U64 key, U64 value)
{
  U64 hash = hash_table_hasher(str8_struct(&key));
  return hash_table_push(arena, ht, hash, (KeyValuePair){ .key_u64 = key, .value_u64 = value });
}

internal String8
hash_table_normalize_path_string(Arena *arena, String8 path)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 result;
  result = lower_from_str8(scratch.arena, path);
  result = path_convert_slashes(arena, result, PathStyle_UnixAbsolute);
  scratch_end(scratch);
  return result;
}

internal BucketNode *
hash_table_push_path_string(Arena *arena, HashTable *ht, String8 path, String8 value)
{
  String8 path_canon = hash_table_normalize_path_string(arena, path); 
  return hash_table_push_string_string(arena, ht, path_canon, value);
}

internal BucketNode *
hash_table_push_path_u64(Arena *arena, HashTable *ht, String8 path, U64 value)
{
  String8 path_canon = hash_table_normalize_path_string(arena, path);
  U64 hash = hash_table_hasher(path_canon);
  return hash_table_push(arena, ht, hash, (KeyValuePair){ .key_string = path_canon, .value_u64 = value });
}

internal BucketNode *
hash_table_push_path_raw(Arena *arena, HashTable *ht, String8 path, void *value)
{
  String8 path_canon = hash_table_normalize_path_string(arena, path);
  U64 hash = hash_table_hasher(path_canon);
  return hash_table_push(arena, ht, hash, (KeyValuePair){ .key_string = path_canon, .value_raw = value });
}

////////////////////////////////

internal KeyValuePair *
hash_table_search_string(HashTable *ht, String8 key_string)
{
  U64         hash    = hash_table_hasher(key_string);
  U64         ibucket = hash % ht->cap;
  BucketList *bucket  = ht->buckets + ibucket;
  for (BucketNode *n = bucket->first; n != 0; n = n->next) {
    if (str8_match(n->v.key_string, key_string, 0)) {
      return &n->v;
    }
  }
  return 0;
}

internal KeyValuePair *
hash_table_search_u32(HashTable *ht, U32 key_u32)
{
  U64         hash    = hash_table_hasher(str8_struct(&key_u32));
  U64         ibucket = hash % ht->cap;
  BucketList *bucket  = ht->buckets + ibucket;
  for (BucketNode *n = bucket->first; n != 0; n = n->next) {
    if (n->v.key_u32 == key_u32) {
      return &n->v;
    }
  }
  return 0;
}

internal KeyValuePair *
hash_table_search_u64(HashTable *ht, U64 key_u64)
{
  U64         hash    = hash_table_hasher(str8_struct(&key_u64));
  U64         ibucket = hash % ht->cap;
  BucketList *bucket  = ht->buckets + ibucket;
  for (BucketNode *n = bucket->first; n != 0; n = n->next) {
    if (n->v.key_u64 == key_u64) {
      return &n->v;
    }
  }
  return 0;
}

internal void *
hash_table_search_u64_raw(HashTable *ht, U64 key_u64)
{
  KeyValuePair *kv = hash_table_search_u64(ht, key_u64);
  return kv ? kv->value_raw : 0;
}

internal KeyValuePair *
hash_table_search_path(HashTable *ht, String8 path)
{
  Temp scratch = scratch_begin(0,0);
  String8 path_canon = path;
  path_canon = lower_from_str8(scratch.arena, path_canon);
  path_canon = path_convert_slashes(scratch.arena, path_canon, PathStyle_UnixAbsolute);
  KeyValuePair *result = hash_table_search_string(ht, path_canon);
  scratch_end(scratch);
  return result;
}

internal void *
hash_table_search_path_raw(HashTable *ht, String8 path)
{
  KeyValuePair *kv = hash_table_search_path(ht, path);
  return kv ? kv->value_raw : 0;
}

internal void *
hash_table_(HashTable *ht, String8 path)
{
  KeyValuePair *result = hash_table_search_path(ht, path);
  return result ? result->value_raw : 0;
}

internal B32
hash_table_search_path_u64(HashTable *ht, String8 key, U64 *value_out)
{
  KeyValuePair *result = hash_table_search_path(ht, key);
  if (result != 0) {
    if (value_out != 0) {
      *value_out = result->value_u64;
    }
    return 1;
  }
  return 0;
}

internal B32
hash_table_search_string_u64(HashTable *ht, String8 key, U64 *value_out)
{
  KeyValuePair *result = hash_table_search_string(ht, key);
  if (result != 0) {
    if (value_out != 0) {
      *value_out = result->value_u64;
    }
    return 1;
  }
  return 0;
}

////////////////////////////////

internal int
key_value_pair_is_before_u32(void *raw_a, void *raw_b)
{
  KeyValuePair *a = raw_a;
  KeyValuePair *b = raw_b;
  return a->key_u32 < b->key_u32;
}

internal int
key_value_pair_is_before_u64(void *raw_a, void *raw_b)
{
  KeyValuePair *a = raw_a;
  KeyValuePair *b = raw_b;
  return a->key_u64 < b->key_u64;
}

internal U32 *
keys_from_hash_table_u32(Arena *arena, HashTable *ht)
{
  U32 *result = push_array_no_zero(arena, U32, ht->count);
  for (U64 bucket_idx = 0, cursor = 0; bucket_idx < ht->cap; ++bucket_idx) {
    for (BucketNode *n = ht->buckets[bucket_idx].first; n != 0; n = n->next) {
      Assert(cursor < ht->count);
      result[cursor++] = n->v.key_u32;
    }
  }
  return result;
}

internal U64 *
keys_from_hash_table_u64(Arena *arena, HashTable *ht)
{
  U64 *result = push_array_no_zero(arena, U64, ht->count);
  for (U64 bucket_idx = 0, cursor = 0; bucket_idx < ht->cap; ++bucket_idx) {
    for (BucketNode *n = ht->buckets[bucket_idx].first; n != 0; n = n->next) {
      Assert(cursor < ht->count);
      result[cursor++] = n->v.key_u64;
    }
  }
  return result;
}

internal KeyValuePair *
key_value_pairs_from_hash_table(Arena *arena, HashTable *ht)
{
  KeyValuePair *pairs = push_array_no_zero(arena, KeyValuePair, ht->count);
  for (U64 bucket_idx = 0, cursor = 0; bucket_idx < ht->cap; ++bucket_idx) {
    for (BucketNode *n = ht->buckets[bucket_idx].first; n != 0; n = n->next) {
      Assert(cursor < ht->count);
      pairs[cursor++] = n->v;
    }
  }
  return pairs;
}

internal void
sort_key_value_pairs_as_u32(KeyValuePair *pairs, U64 count)
{
  radsort(pairs, count, key_value_pair_is_before_u32);
}

internal void
sort_key_value_pairs_as_u64(KeyValuePair *pairs, U64 count)
{
  radsort(pairs, count, key_value_pair_is_before_u64);
}

internal U64Array
remove_duplicates_u64_array(Arena *arena, U64Array arr)
{
  Temp scratch = scratch_begin(&arena, 1);

  HashTable *ht = hash_table_init(scratch.arena, ((U64)(F64)arr.count * 0.5));

  for (U64 i = 0; i < arr.count; ++i) {
    KeyValuePair *is_present = hash_table_search_u64(ht, arr.v[i]);
    if (!is_present) {
      hash_table_push_u64_raw(scratch.arena, ht, arr.v[i], 0);
    }
  }

  U64Array result = {0};
  result.count    = ht->count;
  result.v        = keys_from_hash_table_u64(arena, ht);

  scratch_end(scratch);
  return result;
}

internal String8List
remove_duplicates_str8_list(Arena *arena, String8List list)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List  result = {0};
  HashTable   *ht     = hash_table_init(scratch.arena, list.node_count);

  for (String8Node *node = list.first; node != 0; node = node->next) {
    KeyValuePair *is_present = hash_table_search_string(ht, node->string);
    if (!is_present) {
      hash_table_push_string_raw(scratch.arena, ht, node->string, 0);
      str8_list_push(arena, &result, node->string);
    }
  }

  scratch_end(scratch);
  return result;
}
