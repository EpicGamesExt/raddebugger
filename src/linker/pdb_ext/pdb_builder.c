// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////

internal U64
pdb_hash_table_compute_load_factor(U64 count)
{
  // PDB/include/map.h:cdrLoadMax()
  U64 load_factor = count * 2/3 + 1;
  return load_factor;
}

internal void
pdb_hash_table_alloc(PDB_HashTable *ht, U32 max)
{
  ProfBeginFunction();
  ht->arena        = arena_alloc();
  ht->bucket_arr   = push_array(ht->arena, PDB_HashTableBucket, max);
  ht->present_bits = bit_array_init32(ht->arena, max);
  ht->deleted_bits = bit_array_init32(ht->arena, max);
  ht->max          = max;
  ht->count        = 0;
  ProfEnd();
}

internal void
pdb_hash_table_release(PDB_HashTable *ht)
{
  ProfBeginFunction();
  arena_release(ht->arena);
  MemoryZeroStruct(ht);
  ProfEnd();
}

internal PDB_HashTableParseError
pdb_hash_table_from_data(PDB_HashTable *ht,
                         String8 data,
                         B32 has_local_data,
                         PDB_HashTableUnpackFunc *unpack_func,
                         void *unpack_ud,
                         U64 *read_bytes_out)
{
  ProfBeginFunction();
  PDB_HashTableParseError error = PDB_HashTableParseError_OK;

  U64 cursor = 0;
  
  U32      local_data_size = 0;
  String8  local_data      = str8(0,0);
  U32      count           = 0;
  U32      max             = 0;
  U32Array present_bits    = {0};
  U32Array deleted_bits    = {0};

  do {
    error = PDB_HashTableParseError_OUT_OF_BYTES;

    if (has_local_data) {
      if (cursor + sizeof(local_data_size) > data.size) {
        break;
      }
      cursor += str8_deserial_read_struct(data, cursor, &local_data_size);
      if (cursor + local_data_size > data.size) {
        break;
      }
      cursor += str8_deserial_read_block(data, cursor, local_data_size, &local_data);
    }

    if (cursor + sizeof(count) > data.size) {
      break;
    }
    cursor += str8_deserial_read_struct(data, cursor, &count);
    if (cursor + sizeof(max) > data.size) {
      break;
    }
    cursor += str8_deserial_read_struct(data, cursor, &max);
    cursor += pdb_read_bit_vector_string(data, cursor, &present_bits);
    cursor += pdb_read_bit_vector_string(data, cursor, &deleted_bits);

    error = PDB_HashTableParseError_OK;
  } while(0);

  if (error == PDB_HashTableParseError_OK) {
    U64 load_factor = pdb_hash_table_compute_load_factor(max);
    B32 is_count_ok = count < max;
    B32 is_load_factor_ok = count < load_factor;
    B32 is_present_bits_ok = present_bits.count <= AlignPow2(max, 32);
    B32 is_deleted_bits_ok = deleted_bits.count <= AlignPow2(max, 32);
    if (is_count_ok && is_load_factor_ok && is_present_bits_ok && is_deleted_bits_ok) {
      Arena *arena = arena_alloc();
      PDB_HashTableBucket *bucket_arr = push_array_no_zero(arena, PDB_HashTableBucket, max);
      U32Array present_bits_new = bit_array_init32(arena, max);
      U32Array deleted_bits_new = bit_array_init32(arena, max);
      MemoryCopyTyped(&present_bits_new.v[0], &present_bits.v[0], present_bits.count);
      MemoryCopyTyped(&deleted_bits_new.v[0], &deleted_bits.v[0], deleted_bits.count);

      // unpack buckets
      U64 read_count = 0;
      for (U64 bucket_idx = 0; bucket_idx < max; bucket_idx += 1) {
        if (bit_array_is_bit_set(present_bits_new, bucket_idx)) {
          if (bit_array_is_bit_set(deleted_bits_new, bucket_idx)) {
            error = PDB_HashTableParseError_CORRUPTED;
            break;
          }
          if (read_count >= count) {
            error = PDB_HashTableParseError_CORRUPTED;
            break;
          }

          String8 key;
          String8 value;
          B32 has_unpack_failed = unpack_func(unpack_ud, local_data, data, &cursor, &key, &value);
          if (has_unpack_failed) {
            error = PDB_HashTableParseError_CORRUPTED;
            break;
          }
          
          bucket_arr[bucket_idx].key = key;
          bucket_arr[bucket_idx].value = value;

          read_count += 1;
        }
      }

      if (error == PDB_HashTableParseError_OK) {
        ht->arena        = arena;
        ht->bucket_arr   = bucket_arr;
        ht->present_bits = present_bits_new;
        ht->deleted_bits = deleted_bits_new;
        ht->count        = count;
        ht->max          = max;

        if (read_bytes_out) {
          // TBH data format should tell parser upfront size of the hash table
          *read_bytes_out = cursor;
        }
      } else {
        arena_release(arena);
      }
    } else {
      error = PDB_HashTableParseError_CORRUPTED;
    }
  }

  ProfEnd();
  return error;
}

internal int
pdb_hash_table_bucket_is_before(void *raw_a, void *raw_b)
{
  PDB_HashTableBucket *a = *(PDB_HashTableBucket **)raw_a, *b = *(PDB_HashTableBucket **)raw_b;
  return a->insert_idx < b->insert_idx;
}

internal String8
pdb_data_from_hash_table(Arena *arena, PDB_HashTable *ht, PDB_HashTablePackFunc *pack_func, void *pack_ud)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  String8List kv_srl = {0}; str8_serial_begin(scratch.arena, &kv_srl);
  PDB_HashTableBucket **buckets = pdb_hash_table_get_present_buckets(scratch.arena, ht);
  for EachIndex(i, ht->count) { pack_func(scratch.arena, &kv_srl, buckets[i], buckets[i]->key, buckets[i]->value, pack_ud); }

  // compute count of present words that are needed
  U64 present_word_count = 0;
  U64 present_msb = bit_array_scan_right_to_left32(ht->present_bits, 0, ht->present_bits.count*32, 1);
  if (present_msb < ht->present_bits.count*32) { present_word_count = present_msb / 32 + 1; }

  // compute count of deleted words that are needed
  U64 deleted_word_count = 0;
  U64 deleted_msb = bit_array_scan_right_to_left32(ht->deleted_bits, 0, ht->deleted_bits.count*32, 1);
  if (deleted_msb < ht->deleted_bits.count*32) { deleted_word_count = deleted_msb / 32 + 1; }

  // write hash table
  String8List ht_srl = {0}; str8_serial_begin(scratch.arena, &ht_srl);
  str8_serial_push_u32  (scratch.arena, &ht_srl, ht->count);
  str8_serial_push_u32  (scratch.arena, &ht_srl, ht->max);
  str8_serial_push_u32  (scratch.arena, &ht_srl, present_word_count);
  str8_serial_push_array(scratch.arena, &ht_srl, &ht->present_bits.v[0], present_word_count);
  str8_serial_push_u32  (scratch.arena, &ht_srl, deleted_word_count);
  str8_serial_push_array(scratch.arena, &ht_srl, &ht->deleted_bits.v[0], deleted_word_count);
  str8_list_concat_in_place(&ht_srl, &kv_srl);

  String8 result = str8_serial_end(arena, &ht_srl);
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal void
pdb_hash_table_grow(PDB_HashTable *ht, U64 new_capacity)
{
  ProfBeginFunction();
  PDB_HashTable new_ht;
  pdb_hash_table_alloc(&new_ht, new_capacity);
  for (U32 i = 0; i < ht->max; ++i) {
    if (bit_array_is_bit_set(ht->present_bits, i)) {
      PDB_HashTableBucket *bucket = &ht->bucket_arr[i];
      PDB_HashTableBucket *is_set = pdb_hash_table_try_set(&new_ht, bucket->key, bucket->value);
      is_set->insert_idx = bucket->insert_idx;
      Assert(is_set);
    }
  }
  pdb_hash_table_release(ht);
  *ht = new_ht;
  ProfEnd();
}

internal U32
pdb_hash_table_hash(String8 key)
{
  return (U16)pdb_hash_v1(key);
}

internal PDB_HashTableBucket *
pdb_hash_table_try_set(PDB_HashTable *ht, String8 key, String8 value)
{
  ProfBeginFunction();
  PDB_HashTableBucket *is_set = 0;
  U32 best_ibucket = pdb_hash_table_hash(key) % ht->max;
  U32 ibucket = best_ibucket;
  do {
    B32 is_present = pdb_hash_table_is_present(ht, ibucket);
    if ( ! is_present) {
      PDB_HashTableBucket *bucket = &ht->bucket_arr[ibucket];
      bucket->key        = push_str8_copy(ht->arena, key);
      bucket->value      = push_str8_copy(ht->arena, value);
      bucket->insert_idx = ht->count;

      bit_array_set_bit32(ht->present_bits, ibucket, 1);
      bit_array_set_bit32(ht->deleted_bits, ibucket, 0);

      ht->count += 1;
      is_set = bucket;
      break;
    }
    ibucket = (ibucket + 1) % ht->max;
  } while (ibucket != best_ibucket);
  ProfEnd();
  return is_set;
}

internal void
pdb_hash_table_set(PDB_HashTable *ht, String8 key, String8 value)
{
  ProfBeginFunction();

  // should resize?
  U64 load_factor = pdb_hash_table_compute_load_factor(ht->max);
  if (ht->count + 1 >= load_factor) {
    pdb_hash_table_grow(ht, load_factor * 2);
  }

  // set new item
  PDB_HashTableBucket *is_set = pdb_hash_table_try_set(ht, key, value);
  AssertAlways(is_set);

  ProfEnd();
}

internal B32
pdb_hash_table_get(PDB_HashTable *ht, String8 key, String8 *value_out)
{
  ProfBeginFunction();
  B32 is_get_ok = 0;
  U32 best_ibucket = pdb_hash_table_hash(key) % ht->max;
  U32 ibucket = best_ibucket;
  do {
    B32 is_present = pdb_hash_table_is_present(ht, ibucket);
    if (is_present) {
      PDB_HashTableBucket *bucket = &ht->bucket_arr[ibucket];
      B32 is_match = str8_match(bucket->key, key, 0);
      if (is_match) {
        *value_out = bucket->value;
        is_get_ok = 1;
        break;
      }
    } else {
      break;
    }
    ibucket = (ibucket + 1) % ht->max;
  } while (ibucket != best_ibucket);
  ProfEnd();
  return is_get_ok;
}

internal void
pdb_hash_table_delete(PDB_HashTable *ht, String8 key)
{
  ProfBeginFunction();
  U32 best_ibucket = pdb_hash_table_hash(key) % ht->max;
  U32 ibucket = best_ibucket;
  do {
    B32 is_present = pdb_hash_table_is_present(ht, ibucket);
    if (!is_present) {
      break;
    }
    PDB_HashTableBucket *bucket = &ht->bucket_arr[ibucket];
    int cmp = MemoryCompare(key.str, bucket->key.str, key.size);
    if (cmp == 0) {
      bit_array_set_bit32(ht->present_bits, ibucket, 0);
      bit_array_set_bit32(ht->deleted_bits, ibucket, 1);
      ht->count -= 1;
      break;
    }
    ibucket = (ibucket + 1) % ht->max;
  } while (ibucket != best_ibucket);
  ProfEnd();
}

internal B32
pdb_hash_table_is_present(PDB_HashTable *ht, U32 k)
{
  Assert(k < ht->max);
  return bit_array_is_bit_set(ht->present_bits, k);
}

internal B32
pdb_hash_table_is_deleted(PDB_HashTable *ht, U32 k)
{
  Assert(k < ht->max);
  return bit_array_is_bit_set(ht->deleted_bits, k);
}

internal PDB_HashTableBucket **
pdb_hash_table_get_present_buckets(Arena *arena, PDB_HashTable *ht)
{
  U64 result_count = 0;
  PDB_HashTableBucket **result = push_array(arena, PDB_HashTableBucket *, ht->count);
  for EachIndex(bucket_idx, ht->max) {
    if (bit_array_is_bit_set(ht->present_bits, bucket_idx)) {
      PDB_HashTableBucket *bucket = &ht->bucket_arr[bucket_idx];
      Assert(result_count < ht->count);
      result[result_count++] = bucket;
    }
  }
  return result;
}

internal void
pdb_hash_table_get_present_keys_and_values(Arena *arena, PDB_HashTable *ht, String8Array *keys_out, String8Array *values_out)
{
  *keys_out   = str8_array_reserve(arena, ht->count);
  *values_out = str8_array_reserve(arena, ht->count);
  for (U64 bucket_idx = 0; bucket_idx < ht->max; bucket_idx += 1) {
    if (bit_array_is_bit_set(ht->present_bits, bucket_idx)) {
      PDB_HashTableBucket *bucket = &ht->bucket_arr[bucket_idx];
      Assert(keys_out->count < ht->count);
      keys_out->v[keys_out->count++] = bucket->key;
      values_out->v[values_out->count++] = bucket->value;
    }
  }
}

////////////////////////////////

internal
PDB_HASH_TABLE_UNPACK_FUNC(pdb_named_stream_ht_unpack)
{
  Assert(!ud);

  U32 key_data_offset = max_U32;
  *key_value_cursor += str8_deserial_read_struct(key_value_data, *key_value_cursor, &key_data_offset);

  U8 *cstr_ptr = local_data.str + key_data_offset;
  U8 *cstr_opl = local_data.str + local_data.size;
  String8 stream_name = str8_cstring_capped(cstr_ptr, cstr_opl);

  // NOTE: stream number is U16 but in the reference they cast to U32
  String8 stream_number = {0};
  *key_value_cursor += str8_deserial_read_block(key_value_data, *key_value_cursor, sizeof(U32), &stream_number);

  *key_out   = stream_name;
  *value_out = stream_number;

  return 0;
}

internal
PDB_HASH_TABLE_UNPACK_FUNC(pdb_hash_adj_ht_unpack)
{
  Assert(local_data.size == 0);

  if (*key_value_cursor + sizeof(PDB_StringOffset) > key_value_data.size){
    return 1;
  }
  PDB_StringOffset string_offset = 0;
  *key_value_cursor += str8_deserial_read_struct(key_value_data, *key_value_cursor, &string_offset);

  if (*key_value_cursor + sizeof(CV_TypeIndex) > key_value_data.size) {
    return 1;
  }
  String8 type_index = {0};
  *key_value_cursor += str8_deserial_read_block(key_value_data, *key_value_cursor, sizeof(CV_TypeIndex), &type_index);

  PDB_StringTable *strtab = (PDB_StringTable*)ud;
  String8 type_name = pdb_strtab_string_from_offset(strtab, string_offset);

  *key_out   = type_name;
  *value_out = type_index;

  return 0;
}

internal
PDB_HASH_TABLE_UNPACK_FUNC(pdb_src_header_block_ht_unpack)
{
  if (*key_value_cursor + sizeof(PDB_StringOffset) > key_value_data.size) {
    return 1;
  }
  PDB_StringOffset path_offset = 0;
  *key_value_cursor += str8_deserial_read_struct(key_value_data, *key_value_cursor, &path_offset);

  if (path_offset + sizeof(PDB_SrcHeaderBlockEntry) > key_value_data.size) {
    return 1;
  }
  String8 src_header_block_entry = {0};
  *key_value_cursor += str8_deserial_read_block(key_value_data, *key_value_cursor, sizeof(PDB_SrcHeaderBlockEntry), &src_header_block_entry);

  PDB_StringTable *strtab = (PDB_StringTable*)ud;
  String8 path = pdb_strtab_string_from_offset(strtab, path_offset);

  *key_out   = path;
  *value_out = src_header_block_entry;

  return 0;
}

internal
PDB_HASH_TABLE_PACK_FUNC(pdb_named_stream_ht_pack)
{
  Assert(!ud);
  Assert(value.size == sizeof(U32));
  str8_serial_push_u32(arena, key_value_srl, bucket->key_offset);
  str8_serial_push_string(arena, key_value_srl, value);
}

internal
PDB_HASH_TABLE_PACK_FUNC(pdb_hash_adj_ht_pack)
{
  Assert(value.size == sizeof(CV_TypeIndex));

  PDB_StringTable *strtab = ud;

  PDB_StringIndex string_idx = PDB_INVALID_STRING_INDEX;
  B32 is_found = pdb_strtab_search(strtab, key, &string_idx);
  Assert(is_found);

  PDB_StringOffset type_name_offset = pdb_strtab_string_to_offset(strtab, string_idx);

  str8_serial_push_struct(arena, key_value_srl, &type_name_offset);
  str8_serial_push_string(arena, key_value_srl, value);
}

internal
PDB_HASH_TABLE_PACK_FUNC(pdb_src_header_block_ht_pack)
{
  Assert(value.size == sizeof(PDB_SrcHeaderBlockEntry));

  PDB_StringTable *strtab = ud;

  PDB_StringIndex path_idx = 0;
  B32 is_found = pdb_strtab_search(strtab, key, &path_idx);
  Assert(is_found);

  PDB_StringOffset path_offset = pdb_strtab_string_to_offset(strtab, path_idx);

  str8_serial_push_struct(arena, key_value_srl, &path_offset);
  str8_serial_push_string(arena, key_value_srl, value);
}

////////////////////////////////

internal PDB_HashTableParseError
pdb_hash_adj_hash_table_from_data(PDB_HashTable *ht, String8 data, PDB_StringTable *strtab, U64 *read_bytes_out)
{
  return pdb_hash_table_from_data(ht, data, 0, pdb_hash_adj_ht_unpack, strtab, read_bytes_out);
}

internal PDB_HashTableParseError
pdb_src_header_block_ht_from_data(PDB_HashTable *ht, String8 data, PDB_StringTable *strtab, U64 *read_bytes_out)
{
  return pdb_hash_table_from_data(ht, data, 0, pdb_src_header_block_ht_unpack, strtab, read_bytes_out);
}

internal PDB_HashTableParseError
pdb_named_stream_ht_from_data(PDB_HashTable *ht, String8 data, U64 *read_bytes_out)
{
  return pdb_hash_table_from_data(ht, data, 1, pdb_named_stream_ht_unpack, 0, read_bytes_out);
}

internal String8
pdb_data_from_hash_adj_hash_table(Arena *arena, PDB_HashTable *ht, PDB_StringTable *strtab)
{
  return pdb_data_from_hash_table(arena, ht, pdb_hash_adj_ht_pack, strtab);
}

internal String8
pdb_data_from_src_header_block_ht(Arena *arena, PDB_HashTable *ht, PDB_StringTable *strtab)
{
  return pdb_data_from_hash_table(arena, ht, pdb_src_header_block_ht_pack, strtab);
}

internal String8
pdb_data_from_named_stream_ht(Arena *arena, PDB_HashTable *ht)
{
  Temp scratch = scratch_begin(&arena, 1);

  // serialize names (layout must be in insert order)
  String8List key_data_srl = {0}; str8_serial_begin(scratch.arena, &key_data_srl);
  {
    PDB_HashTableBucket **buckets = pdb_hash_table_get_present_buckets(scratch.arena, ht);
    radsort(buckets, ht->count, pdb_hash_table_bucket_is_before);
    for EachIndex(i, ht->count) {
      buckets[i]->key_offset = key_data_srl.total_size;
      str8_serial_push_cstr(scratch.arena, &key_data_srl, buckets[i]->key);
    }
  }

  // serialize hash table
  String8 ht_data = pdb_data_from_hash_table(arena, ht, pdb_named_stream_ht_pack, 0);

  // put names and hash table together
  String8List srl = {0}; str8_serial_begin(scratch.arena, &srl);
  str8_serial_push_u32(scratch.arena, &srl, safe_cast_u32(key_data_srl.total_size));
  str8_serial_push_data_list(scratch.arena, &srl, key_data_srl.first);
  str8_serial_push_string(scratch.arena, &srl, ht_data);

  String8 result = str8_serial_end(arena, &srl);

  scratch_end(scratch);
  return result;
}

////////////////////////////////

internal void
pdb_strtab_alloc(PDB_StringTable *strtab, U32 max)
{
  ProfBeginFunction();
  
  U64 bucket_max  = (U64)((F64)max * 1.3);
  bucket_max     += 1; // reserve space for null string
  
  strtab->arena         = arena_alloc();
  strtab->version       = 1;
  strtab->size          = 0;
  strtab->bucket_count  = 0;
  strtab->bucket_max    = bucket_max;
  strtab->ibucket_array = push_array(strtab->arena, U32, strtab->bucket_max);
  MemorySet(strtab->ibucket_array, 0xff, sizeof(strtab->ibucket_array[0]) * strtab->bucket_max);
  strtab->bucket_array = push_array(strtab->arena, PDB_StringTableBucket *, strtab->bucket_max);

  // string table always has a null for first entry
  pdb_strtab_add(strtab, str8_lit(""));

  ProfEnd();
}

internal void
pdb_strtab_build(PDB_StringTable *strtab, MSF_Context *msf, MSF_StreamNumber sn)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  // serialize bucket data
  U8  *string_buffer     = push_array_no_zero(scratch.arena, U8, strtab->size);
  U32 *bucket_offset_arr = push_array(scratch.arena, U32, strtab->bucket_max);

  for (U32 bucket_idx = 0; bucket_idx < strtab->bucket_max; bucket_idx += 1) {
    PDB_StringTableBucket *bucket = strtab->bucket_array[bucket_idx];
    if (bucket) {
      // store string offset
      Assert(bucket->offset + bucket->data.size <= strtab->size);
      bucket_offset_arr[bucket_idx] = bucket->offset;

      // write c string at bucket offset
      U8 *str_ptr = string_buffer + bucket->offset;
      MemoryCopy(str_ptr, bucket->data.str, bucket->data.size);
      str_ptr[bucket->data.size] = '\0';
    }
  }
  
  // fill out header
  PDB_StringTableHeader header;
  header.magic = PDB_StringTableHeader_MAGIC;
  header.version = strtab->version;

  // reserve memory for entire string table
  MSF_UInt reserve_size = sizeof(header)
                        + sizeof(strtab->size)
                        + strtab->size
                        + sizeof(bucket_offset_arr[0]) * strtab->bucket_max
                        + sizeof(strtab->bucket_count);
  msf_stream_reserve(msf, sn, reserve_size);

  // write out string table
  msf_stream_write_struct(msf, sn, &header);
  msf_stream_write_struct(msf, sn, &strtab->size);
  msf_stream_write_array (msf, sn, string_buffer, strtab->size);
  msf_stream_write_struct(msf, sn, &strtab->bucket_max);
  msf_stream_write_array (msf, sn, bucket_offset_arr, strtab->bucket_max);
  msf_stream_write_u32(msf, sn, strtab->bucket_count - 1); // 1 for null

  scratch_end(scratch);
  ProfEnd();
}

internal void
pdb_strtab_release(PDB_StringTable *strtab)
{
  ProfBeginFunction();
  arena_release(strtab->arena);
  MemoryZeroStruct(strtab);
  ProfEnd();
}

internal U32
pdb_strtab_get_serialized_size(PDB_StringTable *strtab)
{
  U32 result = 0;
  result += sizeof(PDB_StringTableHeader);
  result += sizeof(U32); // strtab size
  result += strtab->size;
  result += sizeof(U32); // bucket count
  result += sizeof(U32) * strtab->bucket_max;
  result += sizeof(U32); // string count
  return result;
}

internal U32
pdb_strtab_hash(PDB_StringTable *strtab, String8 string)
{
  U32 hash = 0;
  switch (strtab->version) {
  case 1: hash = pdb_hash_v1(string); break;
  default: NotImplemented; break;
  }
  U32 ibucket = hash % strtab->bucket_max;
  return ibucket;
}

internal B32
pdb_strtab_add_(PDB_StringTable *strtab, U64 hash, PDB_StringTableBucket *bucket)
{
  U64 best_bucket_idx = hash;
  U64 bucket_idx      = best_bucket_idx;
  do {
    if (strtab->bucket_array[bucket_idx] == 0) {
      strtab->ibucket_array[bucket->istr]  = bucket_idx;
      strtab->bucket_array[bucket_idx]     = bucket;
      strtab->size                        += bucket->data.size + /* null: */ 1;
      return 1;
    }
    bucket_idx = (bucket_idx + 1) % strtab->bucket_max;
  } while (best_bucket_idx != bucket_idx);
  return 0;
}

internal void
pdb_strtab_add_cv_string_hash_table(PDB_StringTable *strtab, CV_StringHashTable string_ht)
{
  ProfBeginFunction();

  // reserve enough slots for new strings
  pdb_strtab_grow(strtab, string_ht.total_insert_count);

  // upfront push buckets
  PDB_StringTableBucket *buckets = push_array_no_zero(strtab->arena, PDB_StringTableBucket, string_ht.total_insert_count);

  U64 base_offset = strtab->size;

  // proceed to fill out buckets & add them to the string table
  for (U64 bucket_idx = 0, string_idx = 0; bucket_idx < string_ht.bucket_cap; ++bucket_idx) {
    if (string_ht.buckets[bucket_idx] != 0) {
      PDB_StringTableBucket *dst = &buckets[string_idx++];
      dst->data                  = string_ht.buckets[bucket_idx]->string;
      dst->offset                = base_offset + string_ht.buckets[bucket_idx]->u.offset;
      dst->istr                  = strtab->bucket_count++;

      // TODO: precompute hashes in parallel
      U64 hash = pdb_strtab_hash(strtab, dst->data);
      B32 was_added = pdb_strtab_add_(strtab, hash, dst);
      Assert(was_added);
    }
  }

  ProfEnd();
}

internal B32
pdb_strtab_try_add(PDB_StringTable *strtab, String8 string, PDB_StringIndex *index_out)
{
  PDB_StringTableBucket *bucket = push_array(strtab->arena, PDB_StringTableBucket, 1);
  bucket->data                  = push_str8_copy(strtab->arena, string);
  bucket->offset                = strtab->size;
  bucket->istr                  = (PDB_StringIndex)strtab->bucket_count++;

  U32 hash      = pdb_strtab_hash(strtab, string);
  B32 was_added = pdb_strtab_add_(strtab, hash, bucket);

  *index_out = bucket->istr;

  return was_added;
}

internal void
pdb_strtab_grow(PDB_StringTable *strtab, U64 new_max)
{
  ProfBeginFunction();
  
  PDB_StringTable new_strtab;
  pdb_strtab_alloc(&new_strtab, new_max);
  
  // start with 1 because null bucket is already added during string table alloc
  for (PDB_StringIndex istr = 1; istr < strtab->bucket_max; ++istr) {
    U32 ibucket = strtab->ibucket_array[istr];
    
    B32 is_bucket_null = ibucket >= strtab->bucket_max;
    if (is_bucket_null) {
      continue;
    }
    
    PDB_StringTableBucket *bucket = strtab->bucket_array[ibucket];
    
    PDB_StringIndex new_istr;
    B32 is_bucket_pushed = pdb_strtab_try_add(&new_strtab, bucket->data, &new_istr);
    Assert(is_bucket_pushed);
    Assert(new_istr == istr);
    
    U32 new_ibucket = new_strtab.ibucket_array[new_istr];
    PDB_StringTableBucket *new_bucket = new_strtab.bucket_array[new_ibucket];
    Assert(new_bucket->offset == bucket->offset);
  }
  
  *strtab = new_strtab;

  ProfEnd();
}

internal PDB_StringIndex
pdb_strtab_add(PDB_StringTable *strtab, String8 string)
{
  PDB_StringIndex index = 0;
  B32 is_pushed = pdb_strtab_try_add(strtab, string, &index);
  if (!is_pushed) {
    // increase number of slots in the hash table
    pdb_strtab_grow(strtab, strtab->bucket_max * 2);

    // now we have enough slots for the new string
    is_pushed = pdb_strtab_try_add(strtab, string, &index);
    AssertAlways(is_pushed);
  }
  return index;
}

internal B32
pdb_strtab_search(PDB_StringTable *strtab, String8 string, PDB_StringIndex *index_out)
{
  B32 is_found = 0;
  U32 best_ibucket = pdb_strtab_hash(strtab, string);
  U32 ibucket = best_ibucket;
  do {
    PDB_StringTableBucket *bucket = strtab->bucket_array[ibucket];
    if (bucket == NULL) {
      break;
    }
    
    if (str8_match(bucket->data, string, 0)) {
      *index_out = bucket->istr;
      is_found = 1;
      break;
    }
    
    ibucket = (ibucket + 1) % strtab->bucket_max;
  } while (ibucket != best_ibucket);
  return is_found;
}

internal String8
pdb_strtab_string_from_offset(PDB_StringTable *strtab, PDB_StringOffset offset)
{
  String8 string = str8(0,0);
  for (U32 ibucket = 0; ibucket < strtab->bucket_max; ++ibucket) {
    PDB_StringTableBucket *bucket = strtab->bucket_array[ibucket];
    if (bucket) {
      if (bucket->offset == offset) {
        string = bucket->data;
        break;
      }
    }
  }
  return string;
}

internal PDB_StringOffset
pdb_strtab_string_to_offset(PDB_StringTable *strtab, PDB_StringIndex stridx)
{
  Assert(stridx < strtab->bucket_max);
  U32 ibucket = strtab->ibucket_array[stridx];
  PDB_StringOffset offset = strtab->bucket_array[ibucket]->offset;
  return offset;
}

////////////////////////////////

internal B32
pdb_extract_type_server_info(String8 raw_msf, MSF_RawStreamTable *st, MSF_StreamNumber sn, Rng1U64 *ti_range_out, Rng1U64 *leaf_range_out)
{
  Temp scratch = scratch_begin(0,0);
  B32             is_ok        = 0;
  String8         version_data = msf_data_from_stream_number_ex(scratch.arena, raw_msf, st, sn, r1u64(0, sizeof(PDB_TpiVersion)), 8);
  PDB_TpiVersion *version      = str8_deserial_get_raw_ptr(version_data, 0, sizeof(*version));
  if (version) {
    switch (*version) {
    case PDB_TpiVersion_IMPV80: {
      String8  header_size_data = msf_data_from_stream_number_ex(scratch.arena, raw_msf, st, sn, r1u64(sizeof(*version), sizeof(*version) + sizeof(U32)), 8);
      U32     *header_size      = str8_deserial_get_raw_ptr(header_size_data, 0, sizeof(*header_size));
      if (header_size && *header_size >= sizeof(PDB_TpiVersion) + sizeof(U32)) {
        String8        header_data = msf_data_from_stream_number_ex(scratch.arena, raw_msf, st, sn, r1u64(0, *header_size), 8);
        PDB_TpiHeader *header      = str8_deserial_get_raw_ptr(header_data, 0, sizeof(*header));
        if (*header_size + header->leaf_data_size <= st->streams[sn].size) {
          *ti_range_out   = r1u64(header->ti_lo, header->ti_hi);
          *leaf_range_out = r1u64(*header_size, *header_size + header->leaf_data_size);
          is_ok = 1;
        } else {
          NotImplemented; // TODO: handle error
        }
      } else {
        NotImplemented; // TODO: handle error
      }
    } break;
    default: break;
    }
  }
  scratch_end(scratch);
  return is_ok;
}

internal PDB_TypeServer *
pdb_type_server_alloc(U64 bucket_cap)
{
  ProfBeginFunction();
  AssertAlways(0x1000 <= bucket_cap && bucket_cap <= 0x40000);

  Arena *arena = arena_alloc();
  PDB_TypeServer *ts  = push_array(arena, PDB_TypeServer, 1);
  ts->arena      = arena;
  ts->hash_sn    = MSF_INVALID_STREAM_NUMBER;
  ts->ti_lo      = CV_MinComplexTypeIndex;
  ts->bucket_cap = bucket_cap;
  ts->buckets    = push_array(arena, PDB_TypeBucket *, ts->bucket_cap);
  pdb_hash_table_alloc(&ts->hash_adj, 32);

  ProfEnd();
  return ts;
}

internal
THREAD_POOL_TASK_FUNC(pdb_write_type_to_bucket_map_32_task)
{
  PDB_WriteTypeToBucketMap *task = raw_task;

  U64 bucket_idx   = task_id;
  U32 bucket_idx32 = safe_cast_u32(bucket_idx);

  PDB_TypeServer *ts   = task->ts;
  PDB_TypeBucket *head = ts->buckets[bucket_idx];
  for (PDB_TypeBucket *bucket = head; bucket != 0; bucket = bucket->next) {
    Assert(bucket->type_index >= ts->ti_lo);
    Assert(bucket->type_index - ts->ti_lo < ts->leaf_list.node_count);
    CV_TypeIndex type_idx = bucket->type_index - ts->ti_lo;
    Assert(task->map[type_idx] == 0);
    task->map[type_idx] = bucket_idx32;
  }
}

internal PDB_TypeHashStreamInfo
pdb_type_hash_stream_build(TP_Context      *tp,
                           PDB_TypeServer  *ts,
                           PDB_StringTable *strtab,
                           MSF_Context     *msf,
                           PDB_TpiOffHint  *hint_arr,
                           U64              hint_count)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  // write (type index -> bucket index) map
  //
  // zero-out entire map so non-UDTs type indices, that are NOT in the hash table,
  // map to zero offset
  U32 *type_to_bucket_map = push_array(scratch.arena, U32, ts->leaf_list.node_count);
  {
    ProfBegin("Bucket Map");
    PDB_WriteTypeToBucketMap type_to_bucket_task;
    type_to_bucket_task.ts  = ts;
    type_to_bucket_task.map = type_to_bucket_map;
    tp_for_parallel(tp, 0, ts->bucket_cap, pdb_write_type_to_bucket_map_32_task, &type_to_bucket_task);
    ProfEnd();
  }

  // write bucket adjust info
  String8 hash_adj_data = pdb_data_from_hash_adj_hash_table(scratch.arena, &ts->hash_adj, strtab);
  
  ProfBegin("MSF Write");

  // write data to stream
  if (ts->hash_sn == MSF_INVALID_STREAM_NUMBER) {
    ts->hash_sn = msf_stream_alloc(msf);
  }
  msf_stream_seek_start(msf, ts->hash_sn);

  PDB_OffsetSize hash_vals;
  hash_vals.off  = msf_stream_get_pos(msf, ts->hash_sn);
  hash_vals.size = sizeof(type_to_bucket_map[0]) * ts->leaf_list.node_count;
  msf_stream_write(msf, ts->hash_sn, &type_to_bucket_map[0], hash_vals.size);
  
  PDB_OffsetSize hint_offs;
  hint_offs.off  = msf_stream_get_pos(msf, ts->hash_sn);
  hint_offs.size = sizeof(hint_arr[0]) * hint_count;
  msf_stream_write(msf, ts->hash_sn, &hint_arr[0], hint_offs.size);
  
  PDB_OffsetSize hash_adj;
  hash_adj.off  = msf_stream_get_pos(msf, ts->hash_sn);
  hash_adj.size = hash_adj_data.size;
  msf_stream_write_string(msf, ts->hash_sn, hash_adj_data);

  ProfEnd();
  
  // fill out result
  PDB_TypeHashStreamInfo result;
  result.hash_vals = hash_vals;
  result.ti_offs   = hint_offs;
  result.hash_adj  = hash_adj;

  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal
THREAD_POOL_TASK_FUNC(pdb_write_types_task)
{
  ProfBeginFunction();

  PDB_WriteTypesTask *task = raw_task;

  String8Node *node   = task->lf_arr[task_id];
  Rng1U64      range  = task->lf_range_arr[task_id];
  U64          cursor = task->lf_cursor_arr[task_id];

  for (U64 lf_idx = range.min; lf_idx < range.max; node = node->next, lf_idx += 1) {
    if (lf_idx % PDB_TYPE_HINT_STEP == 0) {
      U64 off_idx = lf_idx / PDB_TYPE_HINT_STEP;
      Assert(off_idx < task->hint_count);
      Assert(cursor < PDB_TYPE_OFFSET_MAX);
      task->hint_arr[off_idx].itype = task->ti_lo + lf_idx;
      task->hint_arr[off_idx].off   = (PDB_TypeOffset)cursor;
    }

    // copy leaf data
    MemoryCopy(task->lf_buf + cursor, node->string.str, node->string.size);
    cursor += node->string.size;
  }

  ProfEnd();
}

internal void
pdb_type_server_build(TP_Context *tp, PDB_TypeServer *ts, PDB_StringTable *strtab, MSF_Context *msf, MSF_StreamNumber sn)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  ProfBeginDynamic("Prepare Buffers [Leaf Count: %llu]", ts->leaf_list.node_count);

  U64             hint_count    = CeilIntegerDiv(ts->leaf_list.node_count, PDB_TYPE_HINT_STEP);
  PDB_TpiOffHint *hint_arr      = push_array_no_zero(scratch.arena, PDB_TpiOffHint, hint_count);
  String8Node   **lf_arr        = push_array_no_zero(scratch.arena, String8Node *, tp->worker_count);
  U64            *lf_cursor_arr = push_array_no_zero(scratch.arena, U64, tp->worker_count);
  Rng1U64        *lf_range_arr  = tp_divide_work(scratch.arena, ts->leaf_list.node_count, tp->worker_count);

  U64 lf_buf_size = 0;
  U64 lf_node_idx = 0;
  U64 lf_arr_idx  = 0;
  for (String8Node *lf = ts->leaf_list.first; lf != 0; lf = lf->next) {
    if (lf_node_idx == lf_range_arr[lf_arr_idx].min) { // :thread_pool_dummy_range
      lf_cursor_arr[lf_arr_idx] = lf_buf_size;
      lf_arr[lf_arr_idx]        = lf;
      lf_arr_idx += 1;
    }
    lf_buf_size += lf->string.size;
    lf_node_idx += 1;
  }

  ProfEnd();

  ProfBegin("Write Type Data & Hints");

  PDB_WriteTypesTask write_types_task;
  write_types_task.ti_lo         = ts->ti_lo;
  write_types_task.ti_hi         = ts->ti_lo + ts->leaf_list.node_count;
  write_types_task.hint_count    = hint_count;
  write_types_task.hint_arr      = hint_arr;
  write_types_task.lf_arr        = lf_arr;
  write_types_task.lf_range_arr  = lf_range_arr;
  write_types_task.lf_cursor_arr = lf_cursor_arr;
  write_types_task.lf_buf        = push_array_no_zero(scratch.arena, U8, lf_buf_size);
  write_types_task.lf_buf_size   = lf_buf_size;
  tp_for_parallel(tp, 0, tp->worker_count, pdb_write_types_task, &write_types_task);

  ProfEnd();
  
  // build type lookup accelerator
  PDB_TypeHashStreamInfo hash_stream_info = pdb_type_hash_stream_build(tp, ts, strtab, msf, hint_arr, hint_count);
  
  // fill out header
  PDB_TpiHeader header;
  header.version           = PDB_TpiVersion_IMPV80;
  header.header_size       = sizeof(header);
  header.ti_lo             = ts->ti_lo;
  header.ti_hi             = ts->ti_lo + ts->leaf_list.node_count;
  header.leaf_data_size    = safe_cast_u32(lf_buf_size);
  header.hash_sn           = ts->hash_sn;
  header.hash_sn_aux       = MSF_INVALID_STREAM_NUMBER;
  header.hash_key_size     = sizeof(U32);
  header.hash_bucket_count = ts->bucket_cap;
  header.hash_vals         = hash_stream_info.hash_vals;
  header.itype_offs        = hash_stream_info.ti_offs;
  header.hash_adj          = hash_stream_info.hash_adj;
  
  // write type server to stream
  ProfBegin("MSF Commit");
  msf_stream_seek_start(msf, sn);
  msf_stream_write_struct(msf, sn, &header);
  msf_stream_write_parallel(tp, msf, sn, write_types_task.lf_buf, lf_buf_size);
  ProfEnd();
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
pdb_type_server_release(PDB_TypeServer **ts_ptr)
{
  ProfBeginFunction();
  arena_release((*ts_ptr)->arena);
  *ts_ptr = 0;
  ProfEnd();
}

internal String8Node *
pdb_type_server_make_leaf(PDB_TypeServer *ts, CV_LeafKind kind, String8 data)
{
  ProfBeginFunction();

  String8      leaf = cv_make_leaf(ts->arena, kind, data, PDB_LEAF_ALIGN);
  String8Node *node = str8_list_push(ts->arena, &ts->leaf_list, leaf);
  
  ProfEnd();
  return node;
}

internal U32
pdb_type_server_hash(String8 data)
{
  U32 hash = pdb_hash_v1(data);
  return hash;
}

internal PDB_TypeBucket *
pdb_type_server_push_udt_arr(PDB_TypeServer *ts, U64 count, U32 *hash_arr, String8 *raw_leaf_arr)
{
  // check if type server already contains this leaf and if so move
  // it to the head of bucket list. 
#if 0
  B32 is_udt = pdb_is_udt(kind);
  if (is_udt) {
    PDB_UDTInfo udt_info = pdb_get_udt_info(kind, data);
    U32 udt_hash = pdb_hash_udt(udt_info, data) % ts->bucket_count;
    U64 match_count = 0;
    for (PDB_TypeBucket *curr = ts->bucket_table[udt_hash], *prev = NULL;
         curr != NULL;
         prev = curr, curr = curr->next) {
      if (curr->leaf->kind == kind) {
        PDB_UDTInfo this_udt_info = pdb_get_udt_info(curr->leaf->kind, curr->leaf->data);
        if (str8_match(udt_info.name, this_udt_info.name)) {
          B32 is_data_match = curr->leaf->data.size == data.size &&
            MemoryCompare(curr->leaf->data.str, data.str, data.size) == 0;
          if (is_data_match) {
            B32 is_not_head = (match_count > 0);
            if (is_not_head) {
              // move bucket to head
              prev->next = curr->next;
              curr->next = ts->bucket_table[udt_hash];
              ts->bucket_table[udt_hash] = curr;
              
              // update hash adjust
              pdb_hash_table_delete(&ts->hash_adj, udt_info.name);
              pdb_hash_table_set(&ts->hash_adj, udt_info.name, str8((U8*)&curr->leaf->type_index, sizeof(curr->leaf->type_index)));
            }
            
            return curr->leaf;
          }
          match_count += 1;
        }
      }
    }
  }
#endif
  
  PDB_TypeBucket *bucket_arr = push_array_no_zero(ts->arena, PDB_TypeBucket, count);

  for (U64 leaf_idx = 0; leaf_idx < count; leaf_idx += 1) {
    U32     hash     = hash_arr[leaf_idx];
    String8 raw_leaf = raw_leaf_arr[leaf_idx];

    CV_Leaf leaf = cv_leaf_from_string(raw_leaf);

    // make sure we push a complete UDT
    Assert(cv_is_udt(leaf.kind));
    Assert(!(cv_get_udt_info(leaf.kind, leaf.data).props & CV_TypeProp_FwdRef));

    PDB_TypeBucket *bucket = &bucket_arr[leaf_idx];
    bucket->next       = 0;
    bucket->raw_leaf   = raw_leaf;
    bucket->type_index = ts->ti_lo + ts->leaf_list.node_count + leaf_idx;

    U32 bucket_idx = hash % ts->bucket_cap;
    SLLStackPush(ts->buckets[bucket_idx], bucket);
  }

  return bucket_arr;
}

internal PDB_TypeBucket *
pdb_type_server_push_udt(PDB_TypeServer *ts, U32 hash, String8 raw_leaf)
{
  return pdb_type_server_push_udt_arr(ts, 1, &hash, &raw_leaf);
}

internal void
pdb_type_server_push(PDB_TypeServer *ts, String8 raw_leaf)
{
  ProfBeginFunction();

  CV_Leaf leaf;
  cv_read_leaf(raw_leaf, 0, 1, &leaf);

  if (cv_is_udt(leaf.kind)) {
    CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);
    B32 is_complete = !(udt_info.props & CV_TypeProp_FwdRef);
    if (is_complete) {
      U32 hash = pdb_hash_udt(udt_info, leaf.data);
      pdb_type_server_push_udt(ts, hash, raw_leaf);
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(pdb_count_udt_task)
{
  PDB_PushLeafTask *task  = raw_task;
  for EachInRange(leaf_idx, task->ranges[task_id]) {
    CV_Leaf leaf;
    cv_read_leaf(str8(task->leaf_arr[leaf_idx], max_U64), 0, 1, &leaf);

    if (cv_is_udt(leaf.kind)) {
      CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);
      if (~udt_info.props & CV_TypeProp_FwdRef) {
        task->udt_counts[task_id] += 1;
      }
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(pdb_push_udt_leaf_task)
{
  PDB_PushLeafTask *task          = raw_task;
  PDB_TypeServer   *type_server   = task->type_server;
  U64               bucket_cursor = task->udt_offsets[task_id];
  PDB_TypeBucket   *new_buckets   = task->udt_buckets;

  U64              type_ht_cap     = type_server->bucket_cap;
  PDB_TypeBucket **type_ht_buckets = type_server->buckets;
  U64              base_type_index = type_server->ti_lo + type_server->leaf_list.node_count;

  for EachInRange(leaf_idx, task->ranges[task_id]) {
    CV_Leaf leaf;
    cv_read_leaf(str8(task->leaf_arr[leaf_idx], max_U64), 0, 1, &leaf);

    if (cv_is_udt(leaf.kind)) {
      CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);
      if (~udt_info.props & CV_TypeProp_FwdRef) {
        // hash udt and compute bucket index
        U32 hash       = pdb_hash_udt(udt_info, leaf.data);
        U32 bucket_idx = hash % type_ht_cap;

        // fill out & insert bucket
        PDB_TypeBucket *bucket = &new_buckets[bucket_cursor++];
        bucket->raw_leaf       = leaf.data;
        bucket->type_index     = base_type_index + leaf_idx;
        bucket->next           = ins_atomic_ptr_eval_assign(&type_ht_buckets[bucket_idx], bucket);
      }
    }
  }
}

typedef struct
{
  Rng1U64      *ranges;
  U8          **leaf_arr;
  String8List  *lists;
  String8Node  *nodes;
} PDB_String8ListFromLeafArray;

internal
THREAD_POOL_TASK_FUNC(pdb_str8_list_from_leaf_array_task)
{
  PDB_String8ListFromLeafArray *task = raw_task;
  for EachInRange(leaf_idx, task->ranges[task_id]) {
    String8Node *node = &task->nodes[leaf_idx];
    node->string = cv_raw_leaf_from_ptr(task->leaf_arr[leaf_idx]);
    str8_list_push_node(&task->lists[task_id], node);
  }
}

internal void
pdb_type_server_push_parallel(TP_Context *tp, PDB_TypeServer *type_server, U64 leaf_count, U8 **leaf_arr)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);

  PDB_PushLeafTask task = {0};
  task.leaf_count       = leaf_count;
  task.leaf_arr         = leaf_arr;
  task.type_server      = type_server;
  task.ranges           = tp_divide_work(scratch.arena, leaf_count, tp->worker_count);

  ProfBegin("Count UDT");
  task.udt_counts = push_array(scratch.arena, U64, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, pdb_count_udt_task, &task);
  ProfEnd();

  ProfBegin("Push UDT Leaves");
  U64 total_udt_count = sum_array_u64(tp->worker_count, task.udt_counts);
  task.udt_offsets = offsets_from_counts_array_u64(scratch.arena, task.udt_counts, tp->worker_count);
  task.udt_buckets = push_array_no_zero(type_server->arena, PDB_TypeBucket, total_udt_count);
  tp_for_parallel(tp, 0, tp->worker_count, pdb_push_udt_leaf_task, &task);
  ProfEnd();

  ProfBegin("Append New Leaves");
  {
    PDB_String8ListFromLeafArray task = {0};
    task.leaf_arr = leaf_arr;
    task.ranges   = tp_divide_work(scratch.arena, leaf_count, tp->worker_count);
    task.lists    = push_array(scratch.arena, String8List, tp->worker_count);
    task.nodes    = push_array_no_zero(type_server->arena, String8Node, leaf_count);
    tp_for_parallel(tp, 0, tp->worker_count, pdb_str8_list_from_leaf_array_task, &task);

    // concat output lists
    String8List list = {0};
    for EachIndex(task_id, tp->worker_count) { str8_list_concat_in_place(&type_server->leaf_list, &task.lists[task_id]); }
  }
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
}

#if 0
internal CV_LeafNode *
pdb_type_server_leaf_from_string(PDB_TypeServer *ts, String8 string)
{
  ProfBeginFunction();
  U32 hash = pdb_hash_v1(string);
  U32 bucket_idx = hash % ts->bucket_count;
  PDB_TypeBucket *head_bucket = ts->bucket_table[bucket_idx];
  CV_LeafNode *result = 0;
  for (PDB_TypeBucket *i = head_bucket; i != 0; i = i->next) {
    CV_LeafNode *leaf = i->leaf_node;
    String8 leaf_name = cv_get_leaf_name(leaf->data.kind, leaf->data.data);
    if (str8_match(leaf_name, string, 0)) {
      result = leaf;
      break;
    }
  }
  ProfEnd();
  return result;
}
#endif

////////////////////////////////

#if 0
internal PDB_TypeIndexMap *
pdb_load_types_from_leaf_list(PDB_TypeServer **type_server_arr, CV_LeafList leaf_list)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  // 1. redistribute leaves in parallel
  CV_LeafList leaf_list_arr[CV_TypeIndexSource_COUNT] = {0};
  for (CV_LeafNode *curr = leaf_list.first, *next = 0; curr != 0; curr = next) {
    next = curr->next;
    curr->next = 0;
    CV_TypeIndexSource ti_source = cv_type_index_source_from_leaf_kind(curr->data.kind);
    CV_LeafList *list = &leaf_list_arr[ti_source];
    SLLQueuePush(list->first, list->last, curr);
    list->count += 1;
  }
  
  // 2. reserve type leafs on main thread
  PDB_TypeLeaf *leaf_arr_arr[CV_TypeIndexSource_COUNT];
  for (U64 source_idx = 0; source_idx < ArrayCount(leaf_list_arr); source_idx += 1) {
    PDB_TypeServer *type_server = type_server_arr[source_idx];
    CV_LeafList input_leaf_list = leaf_list_arr[source_idx];
    PDB_TypeLeaf *leaf_arr = pdb_type_server_reserve(type_server, input_leaf_list.count);
    leaf_arr_arr[source_idx] = leaf_arr;
  }
  
  // 3. populate type index map in parallel
  PDB_TypeIndexMap *ti_map = pdb_type_index_map_alloc();
  for (U64 source_idx = 0; source_idx < ArrayCount(leaf_list_arr); source_idx += 1) {
    CV_LeafList input_leaf_list = leaf_list_arr[source_idx];
    PDB_TypeLeaf *leaf_arr = leaf_arr_arr[source_idx];
    for (U64 leaf_idx = 0; leaf_idx < input_leaf_list.count; leaf_idx += 1) {
      CV_TypeIndex external_ti = ti_map->min_itype[source_idx] + leaf_idx;
      CV_TypeIndex internal_ti = leaf_arr[leaf_idx].type_index;
      pdb_type_index_map_add(ti_map, (CV_TypeIndexSource)source_idx, external_ti, internal_ti);
    }
  }
  
  // 4. patch type indices in parallel
  for (U64 source_idx = 0; source_idx < ArrayCount(leaf_list_arr); source_idx += 1) {
    CV_LeafList list = leaf_list_arr[source_idx];
    for (CV_LeafNode *node = list.first; node != 0; node = node->next) {
      Temp temp = temp_begin(scratch.arena);
      
      // get offsets for type indices in data blob
      CV_Leaf *leaf = &node->data;
      CV_TypeIndexInfoList ti_info_list = cv_get_leaf_type_index_offsets(temp.arena, leaf->kind, leaf->data);
      
      for (CV_TypeIndexInfo *ti_info = ti_info_list.first; ti_info != 0; ti_info = ti_info->next) {
        Assert(ti_info->offset + sizeof(CV_TypeIndex) <= leaf->data.size);
        CV_TypeIndex *ti_ptr = (CV_TypeIndex *)(leaf->data.str + ti_info->offset);
        CV_TypeIndex external_ti = *ti_ptr;
        
        B32 is_complex_type = external_ti >= ti_map->min_itype[ti_info->source];
        if (is_complex_type) {
          // search external type index
          CV_TypeIndex internal_tpi_idx = pdb_type_index_map_search(ti_map, CV_TypeIndexSource_TPI, external_ti);
          CV_TypeIndex internal_ipi_idx = pdb_type_index_map_search(ti_map, CV_TypeIndexSource_IPI, external_ti);
          
          // error checks
          if (internal_tpi_idx == 0 && internal_ipi_idx == 0) {
            lnk_invalid_path("unable to find match for external type index 0x%X", external_ti);
            continue;
          }
          if (internal_tpi_idx != 0 && internal_ipi_idx != 0) {
            lnk_invalid_path("both TPI and IPI matched for external type index 0x%X", external_ti);
            continue;
          }
          
          // rewrite index
          CV_TypeIndex internal_ti = internal_tpi_idx ? internal_tpi_idx : internal_ipi_idx;
          *ti_ptr = internal_ti;
        }
      }
      
      temp_end(temp);
    }
  }
  
  // 5. push types to hash table on main thread
  for (U64 source_idx = 0; source_idx < ArrayCount(leaf_list_arr); source_idx += 1) {
    PDB_TypeServer *type_server = type_server_arr[source_idx];
    CV_LeafList list = leaf_list_arr[source_idx];
    PDB_TypeLeaf *leaf_arr = leaf_arr_arr[source_idx];
    U64 leaf_idx = 0;
    for (CV_LeafNode *node = list.first; node != 0; node = node->next, leaf_idx += 1) {
      CV_Leaf *external_leaf = &node->data;
      
      // move patched type data
      PDB_TypeLeaf *internal_leaf = leaf_arr + leaf_idx;
      internal_leaf->kind = external_leaf->kind;
      internal_leaf->data = push_str8_copy(type_server->arena, external_leaf->data);
      
      // push leaf to type server
      pdb_type_server_push_(type_server, internal_leaf);
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
  return ti_map;
}
#endif

////////////////////////////////

internal PDB_InfoContext *
pdb_info_alloc(U32 age, COFF_TimeStamp time_stamp, Guid guid)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc();
  PDB_InfoContext *info = push_array(arena, PDB_InfoContext, 1);
  info->arena      = arena;
  info->flags      = PDB_FeatureFlag_HAS_ID_STREAM;
  info->time_stamp = time_stamp;
  info->age        = age;
  info->guid       = guid;
  pdb_strtab_alloc(&info->strtab, 0x3fff);
  pdb_hash_table_alloc(&info->named_stream_ht, 1);
  pdb_hash_table_alloc(&info->src_header_block_ht, 8);
  ProfEnd();
  return info;
}

internal void
pdb_info_parse_from_data(String8 data, PDB_InfoParse *parse_out)
{
  PDB_InfoVersion version = 0;
  str8_deserial_read_struct(data, 0, &version);

  switch (version) {
  case PDB_InfoVersion_VC70: {
    U64 cursor = 0;

    // read header
    PDB_InfoHeaderV70 header;
    cursor += str8_deserial_read_struct(data, cursor, &header);

    parse_out->version    = version;
    parse_out->time_stamp = header.time_stamp;
    parse_out->age        = header.age;
    parse_out->guid       = header.guid;
    parse_out->extra_info = str8_skip(data, cursor);
  } break;
  case PDB_InfoVersion_VC2:
  case PDB_InfoVersion_VC4:
  case PDB_InfoVersion_VC41:
  case PDB_InfoVersion_VC50:
  case PDB_InfoVersion_VC98:
  case PDB_InfoVersion_VC70_DEP:
  case PDB_InfoVersion_VC80:
  case PDB_InfoVersion_VC110:
  case PDB_InfoVersion_VC140: {
      NotImplemented;
  } break;
  default: Assert(!"invalid info stream version"); break;
  }
}

internal void
pdb_info_build_src_header_block(PDB_InfoContext *info, MSF_Context *msf)
{
  Temp scratch = scratch_begin(0,0);

  // was stream allocated?
  MSF_StreamNumber src_header_block_sn = pdb_find_named_stream(&info->named_stream_ht, PDB_SRC_HEADER_BLOCK_STREAM_NAME);
  if (src_header_block_sn == MSF_INVALID_STREAM_NUMBER) {
    src_header_block_sn = pdb_push_named_stream(&info->named_stream_ht, msf, PDB_SRC_HEADER_BLOCK_STREAM_NAME);
  }

  // build the hash table
  String8 hash_table_data = pdb_data_from_src_header_block_ht(scratch.arena, &info->src_header_block_ht, &info->strtab);
  AssertAlways(hash_table_data.size);

  // compute stream size 
  U64 src_header_stream_size = 0;
  src_header_stream_size += sizeof(PDB_SrcHeaderBlockHeader);
  src_header_stream_size += hash_table_data.size;

  // fill out header
  PDB_SrcHeaderBlockHeader src_header;
  src_header.version     = PDB_SRC_HEADER_BLOCK_MAGIC_V1;
  src_header.stream_size = src_header_stream_size;
  src_header.file_time   = 0;
  src_header.age         = 0;
  MemoryZeroStruct(&src_header.pad);

  // write to stream
  B32 is_header_written = msf_stream_write_struct(msf, src_header_block_sn, &src_header);
  B32 is_hash_table_written = msf_stream_write_string(msf, src_header_block_sn, hash_table_data);
  AssertAlways(is_header_written);
  AssertAlways(is_hash_table_written);
  AssertAlways(msf_stream_get_size(msf, src_header_block_sn) == src_header.stream_size);

  scratch_end(scratch);
}

internal void
pdb_info_build_link_info(PDB_InfoContext *info, MSF_Context *msf)
{
  MSF_StreamNumber linkinfo_sn = pdb_find_named_stream(&info->named_stream_ht, PDB_LINK_INFO_STREAM_NAME);
  if (linkinfo_sn == MSF_INVALID_STREAM_NUMBER) {
    linkinfo_sn = pdb_push_named_stream(&info->named_stream_ht, msf, PDB_LINK_INFO_STREAM_NAME);
  }
  // TODO: populate LINKINFO
}

internal void
pdb_info_build_names(PDB_InfoContext *info, MSF_Context *msf)
{
  MSF_StreamNumber strtab_sn = pdb_find_named_stream(&info->named_stream_ht, PDB_NAMES_STREAM_NAME);
  if (strtab_sn == MSF_INVALID_STREAM_NUMBER) {
    strtab_sn = pdb_push_named_stream(&info->named_stream_ht, msf, PDB_NAMES_STREAM_NAME);
  }
  pdb_strtab_build(&info->strtab, msf, strtab_sn);
}

internal void
pdb_info_build(PDB_InfoContext *info, MSF_Context *msf, MSF_StreamNumber sn)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  // finalize named streams
  if (info->src_header_block_ht.count) {
    pdb_info_build_src_header_block(info, msf);
  }
  pdb_info_build_link_info(info, msf);
  if (info->strtab.bucket_count > 1) {
    pdb_info_build_names(info, msf);
  }

  // serialize named streams hash table
  String8 named_stream_ht_data = pdb_data_from_named_stream_ht(scratch.arena, &info->named_stream_ht);
  
  // fill out header
  PDB_InfoHeaderV70 header;
  header.version    = PDB_InfoVersion_VC70;
  header.time_stamp = info->time_stamp;
  header.age        = info->age;
  header.guid       = info->guid;

  // layout info stream
  String8List info_srl = {0};
  str8_serial_begin(scratch.arena, &info_srl);
  str8_serial_push_struct(scratch.arena, &info_srl, &header);
  str8_serial_push_string(scratch.arena, &info_srl, named_stream_ht_data);
  str8_serial_push_u32(scratch.arena, &info_srl, 0);
  if (info->flags & PDB_FeatureFlag_HAS_ID_STREAM) {
    str8_serial_push_u32(scratch.arena, &info_srl, PDB_FeatureSig_VC140);
  }
  if (info->flags & PDB_FeatureFlag_NO_TYPE_MERGE) {
    str8_serial_push_u32(scratch.arena, &info_srl, PDB_FeatureSig_NO_TYPE_MERGE);
  }
  if (info->flags & PDB_FeatureFlag_MINIMAL_DBG_INFO) {
    str8_serial_push_u32(scratch.arena, &info_srl, PDB_FeatureSig_MINIMAL_DEBUG_INFO);
  }

  // write info to MSF
  msf_stream_seek_start(msf, sn);
  msf_stream_resize(msf, sn, info_srl.total_size);
  msf_stream_write_list(msf, sn, info_srl);
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
pdb_info_release(PDB_InfoContext **info_ptr)
{
  ProfBeginFunction();
  arena_release((*info_ptr)->arena);
  *info_ptr = NULL;
  ProfEnd();
}

internal MSF_StreamNumber
pdb_push_named_stream(PDB_HashTable *named_stream_ht, MSF_Context *msf, String8 name)
{
  ProfBeginFunction();
  MSF_StreamNumber sn = msf_stream_alloc(msf);
  U32 sn32 = (U32)sn;
  pdb_hash_table_set(named_stream_ht, name, str8_struct(&sn32));
  ProfEnd();
  return sn;
}

internal MSF_StreamNumber
pdb_find_named_stream(PDB_HashTable *named_stream_ht, String8 name)
{
  ProfBeginFunction();
  MSF_StreamNumber result = MSF_INVALID_STREAM_NUMBER;
  String8 value;
  if (pdb_hash_table_get(named_stream_ht, name, &value)) {
    Assert(value.size == sizeof(U32));
    result = *(MSF_StreamNumber*)value.str;
  }
  ProfEnd();
  return result;
}

internal PDB_SrcError
pdb_add_src(PDB_InfoContext *info, MSF_Context *msf, String8 file_path, String8 file_data, PDB_SrcCompType comp)
{
  Temp scratch = scratch_begin(0,0);
  PDB_SrcError error_status = PDB_SrcError_UNKNOWN;

  if (comp == PDB_SrcComp_NULL) {
    // process path so it passes VS validity checks
    String8 virt_path = file_path;
    String8 work_dir = os_get_current_path(scratch.arena);
    virt_path = path_absolute_dst_from_relative_dst_src(scratch.arena, virt_path, work_dir);
    virt_path = lower_from_str8(scratch.arena, virt_path);
    virt_path = path_convert_slashes(scratch.arena, virt_path, PathStyle_UnixAbsolute);

    String8 dummy_value;
    B32 is_virt_path_present = pdb_hash_table_get(&info->src_header_block_ht, virt_path, &dummy_value);
    if (!is_virt_path_present) {
      String8 stream_name = push_str8f(scratch.arena, "/src/files/%S", virt_path);
      MSF_StreamNumber sn = pdb_find_named_stream(&info->named_stream_ht, stream_name);
      B32 is_name_free = (sn == MSF_INVALID_STREAM_NUMBER);
      if (is_name_free) {
        sn = pdb_push_named_stream(&info->named_stream_ht, msf, stream_name);
        B32 is_file_data_written = msf_stream_write_string(msf, sn, file_data);
        if (is_file_data_written) {
          // add command line path
          PDB_StringIndex file_path_stridx;
          if (!pdb_strtab_search(&info->strtab, file_path, &file_path_stridx)) {
            file_path_stridx = pdb_strtab_add(&info->strtab, file_path);
          }

          // add virtual path
          PDB_StringIndex virt_path_stridx;
          if (!pdb_strtab_search(&info->strtab, virt_path, &virt_path_stridx)) {
            virt_path_stridx = pdb_strtab_add(&info->strtab, virt_path);
          }

          // string indices -> offsets
          PDB_StringOffset file_path_stroff = pdb_strtab_string_to_offset(&info->strtab, file_path_stridx);
          PDB_StringOffset virt_path_stroff = pdb_strtab_string_to_offset(&info->strtab, virt_path_stridx);

          // fill out entry
          PDB_SrcHeaderBlockEntry entry;
          entry.size      = sizeof(entry);
          entry.version   = PDB_SRC_HEADER_BLOCK_MAGIC_V1;
          entry.file_crc  = pdb_crc32_from_string(file_data);
          entry.file_size = file_data.size;
          entry.file_path = file_path_stroff;
          entry.obj       = 0; // null string offset
          entry.virt_path = virt_path_stroff;
          entry.comp      = comp;
          entry.flags     = 0;
          MemorySet(&entry.pad[0], 0, sizeof(entry.pad));
          MemorySet(&entry.reserved[0], 0, sizeof(entry.reserved));

          // add to hash table { path, entry }
          String8 key = virt_path;
          String8 val = str8_struct(&entry);
          pdb_hash_table_set(&info->src_header_block_ht, key, val);

          error_status = PDB_SrcError_OK;
        } else {
          error_status = PDB_SrcError_UNABLE_TO_WRITE_DATA;
        }
      } else {
        error_status = PDB_SrcError_DUPLICATE_NAME_STREAM;
      }
    } else {
      error_status = PDB_SrcError_DUPLICATE_ENTRY;
    }
  } else {
    error_status = PDB_SrcError_UNSUPPORTED_COMPRESSION;
  }

  scratch_end(scratch);
  return error_status;
}

////////////////////////////////

internal PDB_GsiContext *
gsi_alloc(void)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc();
  PDB_GsiContext *gsi  = push_array(arena, PDB_GsiContext, 1);
  gsi->arena        = arena;
  gsi->word_size    = PDB_GSI_V70_WORD_SIZE;
  gsi->symbol_align = PDB_GSI_V70_SYMBOL_ALIGN;
  gsi->bucket_count = PDB_GSI_V70_BUCKET_COUNT;
  gsi->bucket_arr   = push_array(arena, CV_SymbolList, gsi->bucket_count);
  ProfEnd();
  return gsi;
}

internal void
gsi_release(PDB_GsiContext *gsi)
{
  ProfBeginFunction();
  arena_release(gsi->arena);
  ProfEnd();
}

internal void
gsi_write_build_result(TP_Context         *tp,
                       PDB_GsiBuildResult  build,
                       MSF_Context        *msf,
                       MSF_StreamNumber    gsi_sn,
                       MSF_StreamNumber    symbols_sn)
{
  ProfBeginFunction();

  U64 hash_record_arr_size       = sizeof(build.hash_record_arr[0])       * build.hash_record_count;
  U64 bitmap_size                = sizeof(build.bitmap[0])                * build.bitmap_count;
  U64 compressed_bucket_arr_size = sizeof(build.compressed_bucket_arr[0]) * build.compressed_bucket_count;
  U64 gsi_size                   = sizeof(build.header) + hash_record_arr_size + bitmap_size + compressed_bucket_arr_size;
  
  ProfBeginV("Reserve %M for GSI hash table", gsi_size);
  msf_stream_reserve(msf, gsi_sn, gsi_size);
  ProfEnd();

  ProfBeginV("Reserve %M for symbols", build.symbol_data.size);
  msf_stream_reserve(msf, symbols_sn, build.symbol_data.size);
  ProfEnd();

  ProfBegin("Write GSI header");
  msf_stream_write_struct(msf, gsi_sn, &build.header);
  ProfEnd();

  ProfBegin("Write hash records [%M]", hash_record_arr_size);
  msf_stream_write_parallel(tp, msf, gsi_sn, &build.hash_record_arr[0], hash_record_arr_size);
  ProfEnd();

  ProfBeginV("Write bucket bitmap [%M]", bitmap_size);
  msf_stream_write(msf, gsi_sn, &build.bitmap[0], bitmap_size);
  ProfEnd();

  ProfBegin("Write buckets [%M]", compressed_bucket_arr_size);
  msf_stream_write(msf, gsi_sn, &build.compressed_bucket_arr[0], compressed_bucket_arr_size);
  ProfEnd();
  
  ProfBegin("Write symbols [%M]", build.symbol_data.size);
  msf_stream_write_string_parallel(tp, msf, symbols_sn, build.symbol_data);
  ProfEnd();

  ProfEnd();
}

internal int
gsi_hash_record_compar_is_before(void *raw_a, void *raw_b)
{
  PDB_GsiSortRecord *a = raw_a;
  PDB_GsiSortRecord *b = raw_b;

  int is_before;
  if (a->name.size != b->name.size) {
    is_before = a->name.size < b->name.size;
  } else {
    int cmp = str8_compar_ignore_case(&a->name, &b->name);
    if (cmp == 0) {
      cmp = u64_compar(&a->offset, &b->offset);
    }
    is_before = cmp < 0;
  }

  return is_before;
}

internal int
psi_addr_map_compar_is_before(void *raw_a, void *raw_b)
{
  PDB_GsiSortRecord *a = raw_a;
  PDB_GsiSortRecord *b = raw_b;

  int is_before;
  if (a->isect_off.isect != b->isect_off.isect) {
    is_before = a->isect_off.isect < b->isect_off.isect;
  } else if (a->isect_off.off != b->isect_off.off) {
    is_before = a->isect_off.off < b->isect_off.off;
  } else {
    is_before = str8_compar_case_sensitive(&a->name, &b->name);
  }

  return is_before;
}

internal void
gsi_record_sort_by_name(PDB_GsiSortRecord *arr, U64 count)
{
  ProfBeginFunction();
  radsort(arr, count, gsi_hash_record_compar_is_before);
  ProfEnd();
}

internal void
gsi_record_sort_by_sc(PDB_GsiSortRecord *arr, U64 count)
{
  ProfBeginFunction();
  radsort(arr, count, psi_addr_map_compar_is_before);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(gsi_size_buckets_task)
{
  U64                          bucket_idx  = task_id;
  PDB_GsiSerializeSymbolsTask *task        = raw_task;
  CV_SymbolList               *bucket_list = &task->bucket_arr[bucket_idx];
  for (CV_SymbolNode *node = bucket_list->first; node != 0; node = node->next) {
    task->bucket_size_arr[bucket_idx] += cv_size_from_symbol(&node->data, task->symbol_align);
  }
}

force_inline int
gsi_symbol_is_before(void *raw_a, void *raw_b)
{
  CV_Symbol *a = *(CV_Symbol **)raw_a;
  CV_Symbol *b = *(CV_Symbol **)raw_b;

  String8 a_name = cv_name_from_symbol(a->kind, a->data);
  String8 b_name = cv_name_from_symbol(b->kind, b->data);

  int is_before;
  if (a_name.size != b_name.size) {
    is_before = a_name.size < b_name.size;
  } else {
    is_before = str8_compar_ignore_case(&a_name, &b_name) < 0;
  }

  return is_before;
}

internal int
gsi_pub_symbol_is_before(void *raw_a, void *raw_b)
{
  CV_Symbol *a = *(CV_Symbol **)raw_a;
  CV_Symbol *b = *(CV_Symbol **)raw_b;

  String8 a_name = cv_name_from_symbol(a->kind, a->data);
  String8 b_name = cv_name_from_symbol(b->kind, b->data);

  int is_before;
  if (a_name.size != b_name.size) {
    is_before = a_name.size < b_name.size;
  } else {
    int cmp = str8_compar_ignore_case(&a_name, &b_name);
    if (cmp == 0) {
      CV_SymPub32 *a_sym = (CV_SymPub32 *)a->data.str;
      CV_SymPub32 *b_sym = (CV_SymPub32 *)b->data.str;
      cmp = u16_compar(&a_sym->sec, &b_sym->sec);
      if (cmp == 0) {
        cmp = u32_compar(&a_sym->off, &b_sym->off);
      }
    }

    is_before = cmp < 0;
  }

  return is_before;
}

internal
THREAD_POOL_TASK_FUNC(gsi_serialize_pub32)
{
  Temp scratch = scratch_begin(&arena, 1);

  U64                          bucket_idx = task_id;
  PDB_GsiSerializeSymbolsTask *task       = raw_task;

  CV_SymbolList bucket = task->bucket_arr[bucket_idx];

  CV_Symbol **symbol_arr = push_array(scratch.arena, CV_Symbol *, bucket.count); 
  {
    U64 i = 0;
    for EachNode(n, CV_SymbolNode, bucket.first) { symbol_arr[i++] = &n->data; }
  }

  // sort symbols within bucket
  radsort(symbol_arr, bucket.count, gsi_pub_symbol_is_before);

  PDB_GsiSortRecord *sort_record_arr = task->sort_record_arr_arr[bucket_idx];
  U64                buffer_size     = task->bucket_size_arr[bucket_idx];
  U64                buffer_base     = task->bucket_off_arr[bucket_idx];
  U8                *buffer          = task->buffer + buffer_base;

  U64 sort_idx      = 0;
  U64 buffer_cursor = 0;
  for EachIndex(i, bucket.count) {
    Assert(symbol_arr[i]->kind == CV_SymKind_PUB32);

    CV_SymPub32 *pub32    = (CV_SymPub32 *)symbol_arr[i]->data.str;
    U8          *str_ptr  = (U8 *)(pub32 + 1);
    U64          str_size = symbol_arr[i]->data.size - sizeof(*pub32);
    String8      name     = str8(str_ptr, str_size);

    // init sort record
    PDB_GsiSortRecord *sr = &sort_record_arr[sort_idx];
    sr->isect_off         = isect_off(pub32->sec, pub32->off);
    sr->name              = name;
    sr->offset            = buffer_cursor;

    // serialize symbol
    U64 serial_size = cv_write_symbol(buffer, buffer_cursor, buffer_size, symbol_arr[i], task->symbol_align);

    // advance
    sort_idx      += 1;
    buffer_cursor += serial_size;
  }
  Assert(sort_idx == bucket.count);
  Assert(buffer_cursor == buffer_size);

  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(gsi_serialize_symbols_task)
{
  Temp scratch = scratch_begin(&arena, 1);

  U64                          bucket_idx  = task_id;
  PDB_GsiSerializeSymbolsTask *task        = raw_task;
  CV_SymbolList                bucket = task->bucket_arr[bucket_idx];

  CV_Symbol **symbol_arr = push_array(scratch.arena, CV_Symbol *, bucket.count); 
  {
    U64 i = 0;
    for EachNode(n, CV_SymbolNode, bucket.first) { symbol_arr[i++] = &n->data; }
  }

  // sort symbols within bucket
  radsort(symbol_arr, bucket.count, gsi_symbol_is_before);

  // symbol -> GSI sort record
  {
    PDB_GsiSortRecord   *sort_record_arr = task->sort_record_arr_arr[bucket_idx];
    U64                  buffer_size     = task->bucket_size_arr[bucket_idx];
    U64                  buffer_base     = task->bucket_off_arr[bucket_idx];
    U8                  *buffer          = task->buffer + buffer_base;

    U64 sort_idx      = 0;
    U64 buffer_cursor = 0;
    for EachIndex(i, bucket.count) {
      // init sort record
      PDB_GsiSortRecord *sr = &sort_record_arr[sort_idx];
      sr->name   = cv_name_from_symbol(symbol_arr[i]->kind, symbol_arr[i]->data);
      sr->offset = buffer_cursor;
      sort_idx += 1;

      // serialize symbol
      U64 serial_size = cv_write_symbol(buffer, buffer_cursor, buffer_size, symbol_arr[i], task->symbol_align);
      buffer_cursor += serial_size;
    }

    Assert(sort_idx == bucket.count);
    Assert(buffer_cursor == buffer_size);
  }

  scratch_end(scratch);
}

internal PDB_GsiBuildResult
gsi_build_ex(TP_Context *tp, Arena *arena, PDB_GsiContext *gsi, U64 symbol_data_base, B32 is_pub32, U64 msf_page_size)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena,1);

  ProfBegin("Serialize & Sort Symbols");

  PDB_GsiSerializeSymbolsTask serial_task;
  serial_task.symbol_align    = gsi->symbol_align;
  serial_task.bucket_arr      = gsi->bucket_arr;
  serial_task.bucket_size_arr = push_array(scratch.arena, U64, gsi->bucket_count);

  // estimate each bucket size
  tp_for_parallel(tp, 0, gsi->bucket_count, gsi_size_buckets_task, &serial_task);

  // prepare serial buffer
  U64 buffer_size = sum_array_u64(gsi->bucket_count, serial_task.bucket_size_arr);
  serial_task.buffer         = push_array_no_zero(arena, U8, buffer_size);
  serial_task.bucket_off_arr = push_array_copy_u64(scratch.arena, serial_task.bucket_size_arr, gsi->bucket_count);
  u64_array_counts_to_offsets(gsi->bucket_count, serial_task.bucket_off_arr);

  // prepare GSI records
  serial_task.sort_record_arr_arr  = push_array_no_zero(scratch.arena, PDB_GsiSortRecord *, gsi->bucket_count);
  serial_task.sort_record_arr      = push_array_no_zero(arena, PDB_GsiSortRecord, gsi->symbol_count);
  for (U64 bucket_idx = 0, cursor = 0; bucket_idx < gsi->bucket_count; bucket_idx += 1) {
    serial_task.sort_record_arr_arr[bucket_idx] = serial_task.sort_record_arr + cursor;
    cursor += gsi->bucket_arr[bucket_idx].count;
  }

  // fill out sort records & serialize symbols
  TP_TaskFunc *serial_func = is_pub32 ? gsi_serialize_pub32 : gsi_serialize_symbols_task;
  tp_for_parallel(tp, 0, gsi->bucket_count, serial_func, &serial_task);

  ProfEnd();

  U64             bitmap_count            = (gsi->bucket_count / gsi->word_size) + 1; // ms-pdb allocates extra bucket and funnels free buckets there
  U64             compressed_offset_count = 0;
  U64             hash_record_count       = gsi->symbol_count;
  U32            *bitmap                  = push_array(arena, U32, bitmap_count);
  U32            *compressed_offset_arr   = push_array_no_zero(arena, U32, gsi->bucket_count);
  PDB_GsiHashRecord *hash_record_arr      = push_array_no_zero(arena, PDB_GsiHashRecord, hash_record_count);

  // offsets for symbol stream are shifted by one to tell apart from null and zero (see GSI1::fixSymRecs) 
  U64 offset_cursor = (1 + symbol_data_base);
  U64 hash_idx = 0;

  ProfBegin("Write Bitmap & Record Offsets");
  for (U64 bucket_idx = 0; bucket_idx < gsi->bucket_count; bucket_idx += 1) {
    // set bit for each occupied bucket
    CV_SymbolList bucket_list = gsi->bucket_arr[bucket_idx];
    if (bucket_list.count) {
      U64 word_idx = bucket_idx / gsi->word_size;
      Assert(word_idx < bitmap_count);
      bitmap[word_idx] |= 1u << (bucket_idx % gsi->word_size);
      compressed_offset_arr[compressed_offset_count] = hash_idx * sizeof(PDB_GsiHashRecordOffsetCalc); // store in-memory offset for first bucket
      compressed_offset_count += 1;
    }

    // write out sorted hash records
    PDB_GsiSortRecord *sort_record_arr = serial_task.sort_record_arr_arr[bucket_idx];
    for (U64 sr_idx = 0; sr_idx < gsi->bucket_arr[bucket_idx].count; sr_idx += 1, hash_idx += 1) {
      PDB_GsiHashRecord *hr = &hash_record_arr[hash_idx]; 
      hr->symbol_off = offset_cursor + sort_record_arr[sr_idx].offset;
      hr->cref       = 1;
    }

    // advance offset cursor
    offset_cursor += serial_task.bucket_size_arr[bucket_idx];
  }
  ProfEnd();

  // fill out header
  PDB_GsiHeader header;
  header.signature            = PDB_GsiSignature_Basic;
  header.version              = PDB_GsiVersion_V70;
  header.hash_record_arr_size = sizeof(hash_record_arr[0]) * hash_record_count;
  header.bucket_data_size     = sizeof(bitmap[0]) * bitmap_count + sizeof(compressed_offset_arr[0]) * compressed_offset_count;
  
  // fill out result
  PDB_GsiBuildResult result;
  result.header                  = header;
  result.hash_record_count       = hash_record_count;
  result.hash_record_arr         = hash_record_arr;
  result.sort_record_arr         = serial_task.sort_record_arr;
  result.bitmap_count            = bitmap_count;
  result.bitmap                  = bitmap;
  result.compressed_bucket_count = compressed_offset_count;
  result.compressed_bucket_arr   = compressed_offset_arr;
  result.total_hash_size         = sizeof(header) + header.hash_record_arr_size + header.bucket_data_size;
  result.symbol_data             = str8(serial_task.buffer, buffer_size);
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal void
gsi_build(TP_Context *tp, PDB_GsiContext *gsi, MSF_Context *msf, MSF_StreamNumber sn, MSF_StreamNumber symbols_sn)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  U64 symbol_data_base = msf_stream_get_pos(msf, symbols_sn);
  PDB_GsiBuildResult build = gsi_build_ex(tp, scratch.arena, gsi, symbol_data_base, /* is_pub32: */ 0, msf->page_size);
  gsi_write_build_result(tp, build, msf, sn, symbols_sn);

  scratch_end(scratch);
  ProfEnd();
}

internal U32
gsi_hash(PDB_GsiContext *gsi, String8 input)
{ (void)gsi;
  U32 hash = pdb_hash_v1(input);
  return hash;
}

internal void
gsi_push_(PDB_GsiContext *gsi, U32 hash, CV_SymbolNode *node)
{
  U64 bucket_idx = hash % gsi->bucket_count;
  CV_SymbolList *list = &gsi->bucket_arr[bucket_idx];
  cv_symbol_list_push_node(list, node);
  gsi->symbol_count += 1;
}

internal CV_SymbolNode *
gsi_push(PDB_GsiContext *gsi, CV_Symbol *symbol)
{
  String8 name = cv_name_from_symbol(symbol->kind, symbol->data);
  U32     hash = gsi_hash(gsi, name);

  CV_SymbolNode *node = push_array_no_zero(gsi->arena, CV_SymbolNode, 1);
  node->next = 0;
  node->prev = 0;
  node->data = *symbol;

  gsi_push_(gsi, hash, node);

  return node;
}

internal
THREAD_POOL_TASK_FUNC(gsi_symbol_hasher_task)
{
  ProfBeginFunction();
  GSI_SymbolHasherTask *task  = raw_task;
  Rng1U64               range = task->ranges[task_id];
  for (U64 symbol_idx = range.min; symbol_idx < range.max; ++symbol_idx) {
    CV_SymbolNode *symbol = task->symbols[symbol_idx];
    String8        name   = cv_name_from_symbol(symbol->data.kind, symbol->data.data);
    task->hashes[symbol_idx] = gsi_hash(task->gsi, name);
  }
  ProfEnd();
}

internal void
gsi_push_many_arr(TP_Context *tp, PDB_GsiContext *gsi, U64 count, CV_SymbolNode **symbols)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
 
  ProfBegin("Hash UDT Names");
  GSI_SymbolHasherTask task = {0};
  task.gsi                  = gsi;
  task.ranges               = tp_divide_work(scratch.arena, count, tp->worker_count);
  task.symbols              = symbols;
  task.hashes               = push_array_no_zero(scratch.arena, U32, count);
  tp_for_parallel(tp, 0, tp->worker_count, gsi_symbol_hasher_task, &task);
  ProfEnd();

  for (U64 i = 0; i < count; ++i) {
    gsi_push_(gsi, task.hashes[i], symbols[i]);
  }

  scratch_end(scratch);
  ProfEnd();
}

internal void
gsi_push_many_list(PDB_GsiContext *gsi, U64 count, U32 *hash_arr, CV_SymbolList *list)
{
  Assert(count == list->count);

  U64 hash_idx = 0;
  for (CV_SymbolNode *curr = list->first, *next = 0; curr != 0; curr = next, ++hash_idx) {
    next = curr->next;

    curr->prev = 0;
    curr->next = 0;

    gsi_push_(gsi, hash_arr[hash_idx], curr);
  }

  MemoryZeroStruct(list);
}

internal CV_SymbolNode *
gsi_search(PDB_GsiContext *gsi, CV_Symbol *symbol)
{
  String8 name    = cv_name_from_symbol(symbol->kind, symbol->data);
  U32     hash    = gsi_hash(gsi, name);
  U64     ibucket = hash % gsi->bucket_count;

  CV_SymbolList bucket_list = gsi->bucket_arr[ibucket];
  for (CV_SymbolNode *node = bucket_list.first; node != 0; node = node->next) {
    String8 that_name = cv_name_from_symbol(node->data.kind, node->data.data);
    if (str8_match(name, that_name, 0)) {
      return node;
    }
  }

  return NULL;
}

////////////////////////////////

internal PDB_PsiContext *
psi_alloc(void)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc();
  PDB_PsiContext *psi = push_array(arena, PDB_PsiContext, 1);
  psi->arena = arena;
  psi->gsi = gsi_alloc();
  ProfEnd();
  return psi;
}

internal void
psi_build(TP_Context *tp, PDB_PsiContext *psi, MSF_Context *msf, MSF_StreamNumber sn, MSF_StreamNumber symbols_sn)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);
  
  U64 symbol_data_base = msf_stream_get_pos(msf, symbols_sn);
  PDB_GsiBuildResult gsi_build = gsi_build_ex(tp, scratch.arena, psi->gsi, symbol_data_base, /* is_pub32: */ 1, msf->page_size);
  
  ProfBegin("Address Map");
  
  ProfBegin("Sort");
  gsi_record_sort_by_sc(gsi_build.sort_record_arr, gsi_build.hash_record_count);
  ProfEnd();
  
  ProfBegin("Offset Fill");
  U64 addr_map_count = gsi_build.hash_record_count;
  U64 addr_map_size = addr_map_count * sizeof(U32);
  U32 *addr_map     = push_array_no_zero(scratch.arena, U32, addr_map_count);
  for (U64 i = 0; i < addr_map_count; i += 1) {
    addr_map[i] = gsi_build.sort_record_arr[i].offset;
  }
  ProfEnd();

  ProfEnd();
  
  PDB_PsiHeader header;
  header.sym_hash_size       = gsi_build.total_hash_size;
  header.addr_map_size       = addr_map_size;
  header.thunk_count         = 0;
  header.thunk_size          = 0;
  header.isec_thunk_table    = 0;
  header.padding             = 0;
  header.sec_thunk_table_off = 0;
  header.sec_count           = 0;
  
  ProfBegin("MSF Write");
  msf_stream_write_struct(msf, sn, &header);
  gsi_write_build_result(tp, gsi_build, msf, sn, symbols_sn);
  msf_stream_write_array(msf, sn, &addr_map[0], addr_map_count);
  ProfEnd();
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
psi_release(PDB_PsiContext *psi)
{
  ProfBeginFunction();
  gsi_release(psi->gsi);
  arena_release(psi->arena);
  ProfEnd();
}

internal CV_SymbolNode *
psi_push(PDB_PsiContext *psi, CV_Pub32Flags flags, U32 offset, U16 isect, String8 name)
{
  CV_Symbol pub = cv_make_pub32(psi->arena, flags, offset, isect, name);
  CV_SymbolNode *node = gsi_push(psi->gsi, &pub);
  return node;
}

////////////////////////////////

internal void
dbi_sec_contrib_list_push_node(PDB_DbiSectionContribList *list, PDB_DbiSectionContribNode *node)
{
  node->next = 0;
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal PDB_DbiSectionContribNode *
dbi_sec_contrib_list_push(Arena *arena, PDB_DbiSectionContribList *list)
{
  PDB_DbiSectionContribNode *node = push_array_no_zero(arena, PDB_DbiSectionContribNode, 1);
  node->next = 0;
  dbi_sec_contrib_list_push_node(list, node);
  return node;
}

internal void
dbi_sec_list_concat_arr(PDB_DbiSectionContribList *list, U64 count, PDB_DbiSectionContribList *to_concat)
{
  SLLConcatInPlaceArray(list, to_concat, count);
}

internal PDB_DbiContext *
dbi_alloc(COFF_MachineType machine, U32 age)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc();
  PDB_DbiContext *dbi = push_array(arena, PDB_DbiContext, 1);
  dbi->arena      = arena;
  dbi->age        = age;
  dbi->machine    = machine;
  dbi->globals_sn = MSF_INVALID_STREAM_NUMBER;
  dbi->publics_sn = MSF_INVALID_STREAM_NUMBER;
  dbi->symbols_sn = MSF_INVALID_STREAM_NUMBER;
  pdb_strtab_alloc(&dbi->ec_names, 8);
  for (U64 istream = 0; istream < ArrayCount(dbi->dbg_streams); istream += 1) {
    dbi->dbg_streams[istream] = MSF_INVALID_STREAM_NUMBER;
  }
  ProfEnd();
  return dbi;
}

internal String8List *
dbi_open_file_info(Arena *arena, MSF_Context *msf, MSF_StreamNumber sn, PDB_DbiHeader *dbi_header)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  MSF_UInt file_info_pos = sizeof(PDB_DbiHeader) +
    dbi_header->module_info_size +
    dbi_header->sec_con_size +
    dbi_header->sec_map_size;
  msf_stream_seek(msf, sn, file_info_pos);
  
  U16 mod_count = msf_stream_read_u16(msf, sn);
  U16 total_file_count16 = msf_stream_read_u16(msf, sn);
  
  CV_ModIndex *imod_array = push_array(scratch.arena, CV_ModIndex, mod_count);
  msf_stream_read_array(msf, sn, &imod_array[0], mod_count);
  
  U16 *mod_file_count = push_array(scratch.arena, U16, mod_count);
  msf_stream_read_array(msf, sn, &mod_file_count[0], mod_count);
  
  U64 total_file_count = 0;
  for (U16 imod = 0; imod < mod_count; imod += 1) {
    total_file_count += mod_file_count[imod];
  }
  
  U32 *file_name_offset_array = push_array(scratch.arena, U32, total_file_count);
  msf_stream_read_array(msf, sn, &file_name_offset_array[0], total_file_count);
  
  U64 file_name_buffer_offset = sizeof(mod_count) + 
    sizeof(total_file_count16) +
    sizeof(imod_array[0]) * mod_count +
    sizeof(mod_file_count[0]) * mod_count +
    sizeof(file_name_offset_array[0]) * total_file_count;
  Assert(dbi_header->file_info_size >= file_name_buffer_offset);
  U64 file_name_buffer_size = dbi_header->file_info_size - file_name_buffer_offset;
  char *file_name_buffer = push_array(arena, char, file_name_buffer_size + 1);
  msf_stream_read_array(msf, sn, &file_name_buffer[0], file_name_buffer_size);
  
  String8List *file_info = push_array(arena, String8List, mod_count + 1);
  
  U32 *file_name_offset_ptr = &file_name_offset_array[0];
  for (U64 mod_idx = 0; mod_idx < mod_count; ++mod_idx) {
    String8List *file_list = &file_info[mod_idx];
    U16 file_count = mod_file_count[mod_idx];
    for (U16 ifile = 0; ifile < file_count; ifile += 1, file_name_offset_ptr += 1) {
      Assert(*file_name_offset_ptr <= file_name_buffer_size);
      String8 file_path = str8_cstring(file_name_buffer + *file_name_offset_ptr);
      str8_list_push(arena, file_list, file_path);
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
  return file_info;
}

internal PDB_DbiModuleList
dbi_open_module_info(Arena *arena, MSF_Context *msf, MSF_StreamNumber sn, PDB_DbiHeader *dbi_header, String8List *file_info)
{
  ProfBeginFunction();
  
  PDB_DbiModuleList list = {0};
  
  MSF_UInt module_info_pos = sizeof(PDB_DbiHeader);
  msf_stream_seek(msf, sn, module_info_pos);

  MSF_UInt module_info_opl = module_info_pos + dbi_header->module_info_size;
  while (msf_stream_get_pos(msf, sn) < module_info_opl) { 
    PDB_DbiCompUnitHeader header = {0};
    msf_stream_read_struct(msf, sn, &header);
    String8 obj_path = msf_stream_read_string(arena, msf, sn);
    String8 lib_path = msf_stream_read_string(arena, msf, sn);
    msf_stream_align(msf, sn, PDB_MODULE_ALIGN);
    
    String8List source_file_list = {0};
    if (header.contribution.base.mod != CV_ModIndex_Invalid) {
      source_file_list = file_info[header.contribution.base.mod];
    }
    
    PDB_DbiModule *mod    = push_array(arena, PDB_DbiModule, 1);
    mod->next             = 0;
    mod->sn               = header.sn;
    mod->imod             = header.contribution.base.mod;
    mod->sym_data_size    = header.symbols_size;
    mod->c11_data_size    = header.c11_lines_size;
    mod->c13_data_size    = header.c13_lines_size;
    mod->source_file_list = source_file_list;
    mod->obj_path         = obj_path;
    mod->lib_path         = lib_path;
    mod->first_sc         = header.contribution;
    
    SLLQueuePush(list.first, list.last, mod);
    list.count += 1;
  }
  
  ProfEnd();
  return list;
}

internal PDB_DbiSectionContribList
dbi_open_sec_contrib(Arena *arena, MSF_Context *msf, MSF_StreamNumber sn, PDB_DbiHeader *dbi_header)
{
  ProfBeginFunction();
  
  PDB_DbiSectionContribList sec_contrib = {0};
  
  if (dbi_header->sec_con_size > sizeof(PDB_DbiSectionContrib)) {
    Temp scratch = scratch_begin(&arena, 1);
    
    // seek to start of section contrib info
    MSF_UInt sec_con_pos = sizeof(PDB_DbiHeader) + dbi_header->module_info_size;
    msf_stream_seek(msf, sn, sec_con_pos);
    
    // read header
    PDB_DbiSectionContribVersion version = 0;
    msf_stream_read_struct(msf, sn, &version);
    
    // parse contrib items
    switch (version) {
    case PDB_DbiSectionContribVersion_1: {
      U64 contrib_count = dbi_header->sec_con_size / sizeof(PDB_DbiSectionContrib);
      PDB_DbiSectionContrib *src_contrib_array = push_array(scratch.arena, PDB_DbiSectionContrib, contrib_count);
      MSF_UInt sec_con_read = msf_stream_read_array(msf, sn, &src_contrib_array[0], contrib_count);
      Assert(sec_con_read == sizeof(src_contrib_array[0]) * contrib_count);
      
      PDB_DbiSectionContribNode *dst_contrib_array = push_array_no_zero(arena, PDB_DbiSectionContribNode, contrib_count);
      for (U64 icontrib = 0; icontrib < contrib_count; icontrib += 1) {
        dst_contrib_array[icontrib].next = 0;
        dst_contrib_array[icontrib].data = src_contrib_array[icontrib];
        dbi_sec_contrib_list_push_node(&sec_contrib, &dst_contrib_array[icontrib]);
      }
    } break;
    case PDB_DbiSectionContribVersion_2: {
      NotImplemented;
    } break;
    default: Assert(!"unknown section contrib version"); break;
    }
    
    // have we exhausted sec-con bytes?
    Assert(sec_con_pos + dbi_header->sec_con_size == msf_stream_get_pos(msf, sn));
    scratch_end(scratch);
  }
  
  ProfEnd();
  return sec_contrib;
}

internal void
dbi_build_section_header_stream(PDB_DbiContext *dbi, MSF_Context *msf, MSF_StreamNumber sn)
{
  ProfBeginFunction();
  
  U64 header_arr_size = sizeof(dbi->section_list.first->data) * dbi->section_list.count;
  msf_stream_resize(msf, sn, header_arr_size);
  msf_stream_seek(msf, sn, 0);
  
  for (PDB_DbiSectionNode *i = dbi->section_list.first; i; i = i->next) {
    msf_stream_write_struct(msf, sn, &i->data);
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(dbi_build_file_info_assign_file_offsets_task)
{
  ProfBeginFunction();

  PDB_DbiBuildFileInfoTask *task = raw_task;
  PDB_DbiModule            *mod  = task->mod_arr[task_id];

  task->imod_arr[mod->imod] = mod->imod;

  if (mod->imod != CV_ModIndex_Invalid) {
    // assign source file count
    task->source_file_name_count_arr[mod->imod] = safe_cast_u16x(mod->source_file_list.node_count);

    // assign source file offsets
    U64 source_file_idx = 0;
    for (String8Node *string_n = mod->source_file_list.first; string_n != 0; string_n = string_n->next, ++source_file_idx) {
      CV_StringBucket *string_bucket = cv_string_hash_table_lookup(task->string_ht, string_n->string);
      task->source_file_name_offset_arr[mod->imod][source_file_idx] = safe_cast_u32(string_bucket->u.offset);
    }
  } else {
    // module was deleted don't create source file info
    task->source_file_name_count_arr[mod->imod] = 0;
  }

  ProfEnd();
}

internal String8List
dbi_build_file_info(Arena *arena, TP_Context *tp, PDB_DbiModuleList mod_list, CV_StringHashTable string_ht)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  U64             total_source_file_count = 0;
  U64             mod_arr_count           = 0;
  PDB_DbiModule **mod_arr                 = push_array_no_zero(scratch.arena, PDB_DbiModule *, mod_list.count);

  for (PDB_DbiModule *mod = mod_list.first; mod != 0; mod = mod->next) {
    mod_arr[mod_arr_count++] = mod;
    if (mod->imod != CV_ModIndex_Invalid) {
      total_source_file_count += mod->source_file_list.node_count;
    }
  }

  U32 **source_file_name_offsets_arr = push_array_no_zero(scratch.arena, U32 *, mod_list.count);
  U32  *source_file_name_offsets     = push_array_no_zero(arena, U32, total_source_file_count);
  for (U64 mod_idx = 0, cursor = 0; mod_idx < mod_list.count; ++mod_idx) {
    if (mod_arr[mod_idx]->imod != CV_ModIndex_Invalid) {
      source_file_name_offsets_arr[mod_idx] = source_file_name_offsets + cursor;
      cursor += mod_arr[mod_idx]->source_file_list.node_count;
    } else {
      source_file_name_offsets_arr[mod_idx] = 0;
    }
  }

  U16 total_source_file_count16    = Min(max_U16, total_source_file_count);
  U16 mod_count16                  = Min(max_U16, mod_list.count);

  PDB_DbiBuildFileInfoTask task    = {0};
  task.string_ht                   = string_ht;
  task.mod_arr                     = mod_arr;
  task.imod_arr                    = push_array_no_zero(arena, U16, mod_count16);
  task.source_file_name_count_arr  = push_array_no_zero(arena, U16, mod_list.count);
  task.source_file_name_offset_arr = source_file_name_offsets_arr;
  tp_for_parallel(tp, 0, mod_arr_count, dbi_build_file_info_assign_file_offsets_task, &task);

  // pack strings
  String8 string_buffer = cv_pack_string_hash_table(arena, tp, string_ht);

  // layout file info sections
  String8List file_info_srl = {0};
  str8_serial_begin(arena, &file_info_srl);
  str8_serial_push_u16(arena, &file_info_srl, mod_count16);
  str8_serial_push_u16(arena, &file_info_srl, total_source_file_count16);
  str8_list_push(arena, &file_info_srl, str8_array(task.imod_arr, mod_count16));
  str8_list_push(arena, &file_info_srl, str8_array(task.source_file_name_count_arr, mod_list.count));
  str8_list_push(arena, &file_info_srl, str8_array(source_file_name_offsets, total_source_file_count));
  str8_list_push(arena, &file_info_srl, string_buffer);
  str8_serial_push_align(arena, &file_info_srl, sizeof(U32));

  scratch_end(scratch);
  ProfEnd();
  return file_info_srl;
}

internal String8List
dbi_build_module_info(Arena *arena, PDB_DbiContext *dbi, MSF_Context *msf)
{
  ProfBeginFunction();

  String8List module_info_list = {0};
  str8_serial_begin(arena, &module_info_list);
  
  for (PDB_DbiModule *mod = dbi->module_list.first; mod != 0; mod = mod->next) {
    // fill out header
    PDB_DbiCompUnitHeader *header = push_array(arena, PDB_DbiCompUnitHeader, 1);
    header->contribution          = mod->first_sc;
    // we don't use these flags right now
    // U16 is_written : 1
    // U16 unused     : 7
    // U16 tsm_index  : 8 ; index into type server map
    header->flags                = 0;
    header->sn                   = mod->sn;
    header->symbols_size         = mod->sym_data_size;
    header->c11_lines_size       = mod->c11_data_size;
    header->c13_lines_size       = mod->c13_data_size;
    header->num_contrib_files    = Min(max_U16, mod->source_file_list.node_count);
    header->file_names_offset    = 0; // TODO: fill out the offset
    // TODO: generate EC info
    header->src_file             = 0;
    header->pdb_file             = 0;
    
    // push module info
    str8_serial_push_struct(arena, &module_info_list, header);
    str8_serial_push_cstr(arena, &module_info_list, mod->obj_path);
    str8_serial_push_cstr(arena, &module_info_list, mod->lib_path);
    str8_serial_push_align(arena, &module_info_list, PDB_MODULE_ALIGN);
  }

  ProfEnd();
  return module_info_list;
}

#if 0
int 
dbi_sc_compar(const PDB_DbiSectionContrib *a, const PDB_DbiSectionContrib *b)
{
#if 0
  int cmp = 0;
  if (a->base.sec == b->base.sec) {
    if (a->base.sec_off < b->base.sec_off) {
      cmp = -1;
    } else if (a->base.sec_off > b->base.sec_off) {
      cmp = +1;
    }
  } else if (a->base.sec < b->base.sec) {
    cmp = -1;
  } else {
    cmp = +1;
  }
#else
#define MAKE_SORTER(x) (((U64)(x)->base.sec << 32) | (U64)(x)->base.sec_off)
  U64 l = MAKE_SORTER(a);
  U64 r = MAKE_SORTER(b);
  int cmp = l < r ? -1 : l > r ? + 1 : 0;
#undef MAKE_SORTER
#endif
  return cmp;
}
#endif

internal void
lnk_radix_sort_dbi_sc_array(PDB_DbiSectionContrib *arr, U64 sc_count, U64 sect_count)
{
  ProfBeginFunction();

#if 1
  // faster but uses more memory
# define RADIX_BIT_COUNT 16
# define RADIX_MAX       2
#else
  // slower but uses less memory
# define RADIX_BIT_COUNT 8
# define RADIX_MAX       4
#endif

  Temp scratch = scratch_begin(0,0);

  PDB_DbiSectionContrib *temp_arr = push_array_no_zero(scratch.arena, PDB_DbiSectionContrib, sc_count);
  PDB_DbiSectionContrib *src_arr = arr;
  PDB_DbiSectionContrib *dst_arr = temp_arr;

  ProfBegin("Count Memzero");
  U32 count_8lo[256]; MemoryZeroArray(count_8lo);
  U32 count_8hi[256]; MemoryZeroArray(count_8hi);
  U32 count_16[1 << 16]; MemoryZeroArray(count_16);
  U32 *count_arr = push_array(scratch.arena, U32, sect_count + 1);
  ProfEnd();

  ProfBegin("Histogram");
  for (U64 i = 0; i < sc_count; i += 1) {
    PDB_DbiSectionContrib *sc = src_arr + i;
    count_arr[sc->base.sec] += 1;

    U64 digit_8lo = (sc->base.sec_off >> 0) % ArrayCount(count_8lo);
    U64 digit_8hi = (sc->base.sec_off >> 8) % ArrayCount(count_8hi);
    U64 digit_16 = (sc->base.sec_off >> 16) % ArrayCount(count_16);
    count_8lo[digit_8lo] += 1;
    count_8hi[digit_8hi] += 1;
    count_16[digit_16] += 1;
  }
  ProfEnd();

  //
  // sort on section offset
  //

  ProfBegin("Offsets");
  U32 offset_8lo = 0;
  U32 offset_8hi = 0;
  for (U64 i = 1; i <= ArrayCount(count_8lo); i += 1) {
    U32 current_8lo = count_8lo[i - 1];
    U32 current_8hi = count_8hi[i - 1];
    count_8lo[i - 1] = offset_8lo;
    count_8hi[i - 1] = offset_8hi;
    offset_8lo += current_8lo;
    offset_8hi += current_8hi;
  }

  U32 offset_16 = 0;
  for (U64 i = 1; i <= ArrayCount(count_16); i += 1) {
    U32 current_16 = count_16[i - 1];
    count_16[i - 1] = offset_16;
    offset_16 += current_16;
  }
  ProfEnd();

  count_8lo[0] = 0;
  count_8hi[0] = 0;
  count_16[0] = 0;

  ProfBegin("Order 8 Lo");
  for (U64 i = 0; i < sc_count; i += 1) {
    PDB_DbiSectionContrib *sc = &src_arr[i];
    U64 digit = (sc->base.sec_off >> 0) % ArrayCount(count_8lo);
    dst_arr[count_8lo[digit]++] = *sc;
  }
  ProfEnd();

  ProfBegin("Order 8 Hi");
  for (U64 i = 0; i < sc_count; i += 1) {
    PDB_DbiSectionContrib *sc = &dst_arr[i];
    U64 digit = (sc->base.sec_off >> 8) % ArrayCount(count_8hi);
    src_arr[count_8hi[digit]++] = *sc;
  }
  ProfEnd();

  ProfBegin("Order 16");
  for (U64 i = 0; i < sc_count; i += 1) {
    PDB_DbiSectionContrib *sc = &src_arr[i];
    U64 digit = (sc->base.sec_off >> 16) % ArrayCount(count_16);
    dst_arr[count_16[digit]++] = *sc;
  }
  ProfEnd();

  //
  // sort on section index
  //

  ProfBegin("Section Indices");
  
  U32 offset = 0;
  for (U64 i = 1; i <= sect_count; i += 1) {
    U32 current = count_arr[i - 1];
    count_arr[i - 1] = offset;
    offset += current;
  }

  count_arr[0] = 0;

  for (U64 i = 0; i < sc_count; i += 1) {
    PDB_DbiSectionContrib *sc = dst_arr + i;
    src_arr[count_arr[sc->base.sec]++] = *sc;
  }

  ProfEnd();

#if 0
  for (U64 i = 1; i < sc_count; i += 1) {
    U64 a = ((U64)arr[i - 1].base.sec << 32) | arr[i - 1].base.sec_off;
    U64 b = ((U64)arr[i    ].base.sec << 32) | arr[i    ].base.sec_off;
    Assert(a <= b);
  }
#endif

  scratch_end(scratch);

#undef RADIX_BIT_COUNT
#undef RADIX_MAX

  ProfEnd();
}

internal String8List
dbi_build_sec_con(Arena *arena, PDB_DbiContext *dbi)
{
  ProfBeginFunction();

  PDB_DbiSectionContribVersion *version = push_array(arena, PDB_DbiSectionContribVersion, 1);
  *version = PDB_DbiSectionContribVersion_1;
  
  // push section contribs V1
  ProfBegin("Push sect contribs [Count %llu]", dbi->sec_contrib_list.count);
  PDB_DbiSectionContrib *sc_array = push_array_no_zero(arena, PDB_DbiSectionContrib, dbi->sec_contrib_list.count);
  PDB_DbiSectionContrib *dst = &sc_array[0];
  for (PDB_DbiSectionContribNode *src = dbi->sec_contrib_list.first; src != 0; src = src->next, dst += 1) {
    *dst = src->data;
  }
  ProfEnd();

  // sort section contribs so they are binary searchable
  lnk_radix_sort_dbi_sc_array(sc_array, dbi->sec_contrib_list.count, dbi->section_list.count + 1);
  
  // push section contrib info
  ProfBegin("List Push");
  String8List sec_con_list = {0};
  str8_list_push(arena, &sec_con_list, str8((U8*)version, sizeof(*version)));
  str8_list_push(arena, &sec_con_list, str8((U8*)sc_array, sizeof(sc_array[0])*dbi->sec_contrib_list.count));
  ProfEnd();
  
  ProfEnd();
  return sec_con_list;
}

internal String8List
dbi_build_sec_map(Arena *arena, PDB_DbiContext *dbi)
{
  ProfBeginFunction();

  U64 entry_count = dbi->section_list.count + 1;
  PDB_DbiSecMapEntry *entry_array = push_array(arena, PDB_DbiSecMapEntry, entry_count);
  U64 isect = 0;
  for (PDB_DbiSectionNode *sect = dbi->section_list.first; sect; sect = sect->next, ++isect) {
    PDB_DbiSecMapEntry *s = &entry_array[isect];
    COFF_SectionHeader *section_header = &sect->data;
    if (section_header->flags & COFF_SectionFlag_MemRead) {
      s->flags |= PDB_DbiOMF_READ;
    }
    if (section_header->flags & COFF_SectionFlag_MemWrite) {
      s->flags |= PDB_DbiOMF_WRITE;
    }
    if (section_header->flags & COFF_SectionFlag_MemExecute) {
      s->flags |= PDB_DbiOMF_EXEC;
    }
    if (~section_header->flags & COFF_SectionFlag_Mem16Bit) {
      s->flags |= PDB_DbiOMF_IS_32BIT_ADDR;
    }
    s->flags |= PDB_DbiOMF_IS_SELECTOR; // always set
    s->sec_size = section_header->vsize;
    s->frame = isect + 1;
    s->sec_name = max_U16;
    s->class_name = max_U16;
  }
  // init last entry 
  {
    PDB_DbiSecMapEntry *s = &entry_array[entry_count - 1];
    s->flags = PDB_DbiOMF_IS_32BIT_ADDR | PDB_DbiOMF_IS_ABS_ADDR;
    s->sec_size = max_U32;
    s->frame = isect + 1;
    s->sec_name = max_U16;
    s->class_name = max_U16;
  }
  
  // init header
  PDB_DbiSecMapHeader *header = push_array(arena, PDB_DbiSecMapHeader, 1);
  header->section_count = entry_count;
  header->segment_count = entry_count;
  
  // push section map info
  String8List sec_map_list = {0};
  str8_list_push(arena, &sec_map_list, str8((U8*)header, sizeof(*header)));
  str8_list_push(arena, &sec_map_list, str8((U8*)entry_array, sizeof(entry_array[0])*entry_count));
  
  ProfEnd();
  return sec_map_list;
}

internal String8List
dbi_build_dbg_header(Arena *arena, PDB_DbiContext *dbi, MSF_Context *msf)
{
  ProfBeginFunction();
  if (dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER] == MSF_INVALID_STREAM_NUMBER) {
    dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER] = msf_stream_alloc(msf);
  }
  dbi_build_section_header_stream(dbi, msf, dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER]);
  
  String8List dbg_header_srl = {0};
  str8_serial_begin(arena, &dbg_header_srl);
  str8_serial_push_array(arena, &dbg_header_srl, dbi->dbg_streams, ArrayCount(dbi->dbg_streams));
  
  ProfEnd();
  return dbg_header_srl;
}

internal void
dbi_build(TP_Context *tp, PDB_DbiContext *dbi, MSF_Context *msf, MSF_StreamNumber dbi_sn, CV_StringHashTable string_ht, B32 is_stripped)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  ProfBegin("Build");
  String8List module_info_list = dbi_build_module_info(scratch.arena, dbi, msf);
  String8List sec_con_list     = dbi_build_sec_con(scratch.arena, dbi);
  String8List sec_map_list     = dbi_build_sec_map(scratch.arena, dbi);
  String8List file_info_list   = dbi_build_file_info(scratch.arena, tp, dbi->module_list, string_ht);
  String8List dbg_header_list  = dbi_build_dbg_header(scratch.arena, dbi, msf);
  String8List tsm_list         = {0}; // TODO: TSM
  ProfEnd();
  
  PDB_DbiHeader header    = {0};
  header.sig              = PDB_DbiHeaderSignature_V1;
  header.version          = PDB_DbiVersion_70;
  header.age              = dbi->age;
  header.gsi_sn           = dbi->globals_sn;
  header.build_number     = PDB_DbiMakeBuildNumber(14, 11);
  header.psi_sn           = dbi->publics_sn;
  header.pdb_version      = 0;
  header.sym_sn           = dbi->symbols_sn;
  header.pdb_version2     = 0;
  header.module_info_size = module_info_list.total_size;
  header.sec_con_size     = sec_con_list.total_size;
  header.sec_map_size     = sec_map_list.total_size;
  header.file_info_size   = file_info_list.total_size;
  header.tsm_size         = tsm_list.total_size;
  header.mfc_index        = 0;
  header.dbg_header_size  = dbg_header_list.total_size;
  header.ec_info_size     = pdb_strtab_get_serialized_size(&dbi->ec_names);
  header.flags            = 0;
  header.machine          = dbi->machine;
  header.reserved         = 0;

  if (is_stripped) {
    header.flags |= PDB_DbiHeaderFlag_Stripped;
  }
  
  ProfBegin("MSF Write");

  U64 dbi_stream_size = sizeof(header) +
                        module_info_list.total_size +
                        sec_con_list.total_size +
                        sec_map_list.total_size +
                        file_info_list.total_size +
                        tsm_list.total_size +
                        dbg_header_list.total_size;
  msf_stream_resize(msf, dbi_sn, dbi_stream_size);
  msf_stream_seek_start(msf, dbi_sn);
  msf_stream_write(msf, dbi_sn, &header, sizeof(header));
  msf_stream_write_list(msf, dbi_sn, module_info_list);
  msf_stream_write_list(msf, dbi_sn, sec_con_list);
  msf_stream_write_list(msf, dbi_sn, sec_map_list);
  msf_stream_write_list(msf, dbi_sn, file_info_list);
  msf_stream_write_list(msf, dbi_sn, tsm_list);
  pdb_strtab_build(&dbi->ec_names, msf, dbi_sn);
  msf_stream_write_list(msf, dbi_sn, dbg_header_list);
  ProfEnd();
  
  ProfEnd();
  scratch_end(scratch);
}

internal void
dbi_release(PDB_DbiContext *dbi)
{
  ProfBeginFunction();
  arena_release(dbi->arena);
  ProfEnd();
}

internal PDB_DbiModule *
dbi_push_module(PDB_DbiContext *dbi, String8 obj_path, String8 lib_path)
{
  // init module
  PDB_DbiModule *mod = push_array(dbi->arena, PDB_DbiModule, 1);
  mod->imod          = safe_cast_u32(dbi->module_list.count);
  mod->sn            = MSF_INVALID_STREAM_NUMBER;
  mod->obj_path      = push_str8_copy(dbi->arena, obj_path);
  mod->lib_path      = push_str8_copy(dbi->arena, lib_path.size > 0 ? lib_path : obj_path);
  
  // push to list 
  SLLQueuePush(dbi->module_list.first, dbi->module_list.last, mod);
  dbi->module_list.count += 1;
  
  return mod;
}

internal void
dbi_module_push_section_contrib(PDB_DbiContext *dbi,
                                PDB_DbiModule *mod, 
                                ISectOff isect_off,
                                U32 size,  
                                U32 data_crc,
                                U32 reloc_crc, 
                                COFF_SectionFlags flags)
{
  ProfBeginFunction();

  PDB_DbiSectionContrib sc;
  sc.base.sec     = safe_cast_u16(isect_off.isect);
  sc.base.sec_off = isect_off.off;
  sc.base.size    = size;
  sc.base.flags   = flags;
  sc.base.mod     = mod->imod;
  sc.data_crc     = data_crc;
  sc.reloc_crc    = reloc_crc;

  PDB_DbiSectionContribNode *node = push_array_no_zero(dbi->arena, PDB_DbiSectionContribNode, 1);
  node->data = sc;
  dbi_sec_contrib_list_push_node(&dbi->sec_contrib_list, node);
  
  // Mod1::fUpdateSecContrib
  if (mod->first_sc.base.mod == 0) {
    if (flags & COFF_SectionFlag_CntCode) {
      mod->first_sc = sc;
    }
  }

  ProfEnd();
}

internal String8
dbi_module_read_symbol_data(Arena *arena, MSF_Context *msf, PDB_DbiModule *mod)
{
  String8 symbol_data = str8(0,0);
  if (mod->sn != MSF_INVALID_STREAM_NUMBER) {
    B32 is_seek_ok = msf_stream_seek(msf, mod->sn, 0);
    if (is_seek_ok) {
      symbol_data = msf_stream_read_block(arena, msf, mod->sn, mod->sym_data_size);
    }
  }
  return symbol_data;
}

internal String8
dbi_module_read_c11_data(Arena *arena, MSF_Context *msf, PDB_DbiModule *mod)
{
  String8 c11_data = str8(0,0);
  if (mod->sn != MSF_INVALID_STREAM_NUMBER) {
    MSF_UInt c11_data_pos = mod->sym_data_size;
    B32 is_seek_ok = msf_stream_seek(msf, mod->sn, c11_data_pos);
    if (is_seek_ok) {
      c11_data = msf_stream_read_block(arena, msf, mod->sn, mod->c13_data_size);
    }
  }
  return c11_data;
}

internal String8
dbi_module_read_c13_data(Arena *arena, MSF_Context *msf, PDB_DbiModule *mod)
{
  String8 c13_data = str8(0,0);
  if (mod->sn != MSF_INVALID_STREAM_NUMBER) {
    MSF_UInt c13_data_pos = mod->sym_data_size + mod->c11_data_size;
    B32 is_seek_ok = msf_stream_seek(msf, mod->sn, c13_data_pos);
    if (is_seek_ok) {
      c13_data = msf_stream_read_block(arena, msf, mod->sn, mod->c13_data_size);
    }
  }
  return c13_data;
}

internal void
dbi_push_section(PDB_DbiContext *dbi, COFF_SectionHeader *hdr)
{
  ProfBeginFunction();
  
  PDB_DbiSectionNode *n = push_array(dbi->arena, PDB_DbiSectionNode, 1);
  n->data = *hdr;
  n->next = 0;
  SLLQueuePush(dbi->section_list.first, dbi->section_list.last, n);
  dbi->section_list.count += 1;

  ProfEnd();
}

////////////////////////////////

internal MSF_Context *
pdb_alloc_msf(Arena *arena, U64 page_size)
{
  ProfBeginFunction();
  MSF_Context *msf = msf_alloc_(arena, page_size, MSF_DEFAULT_FPM);
  MSF_StreamNumber null_sn = msf_stream_alloc(msf);
  MSF_StreamNumber info_sn = msf_stream_alloc(msf);
  MSF_StreamNumber tpi_sn = msf_stream_alloc(msf);
  MSF_StreamNumber dbi_sn = msf_stream_alloc(msf);
  MSF_StreamNumber ipi_sn = msf_stream_alloc(msf);
  Assert(null_sn == 0);
  Assert(info_sn == PDB_FixedStream_Info);
  Assert(dbi_sn == PDB_FixedStream_Dbi);
  Assert(tpi_sn == PDB_FixedStream_Tpi);
  Assert(ipi_sn == PDB_FixedStream_Ipi);
  ProfEnd();
  return msf;
}

internal PDB_Context *
pdb_alloc_(Arena *arena, U64 page_size, COFF_MachineType machine, COFF_TimeStamp time_stamp, U32 age, Guid guid)
{
  ProfBeginFunction();
  PDB_Context *pdb = push_array(arena, PDB_Context, 1);
  pdb->arena = arena;
  pdb->msf   = pdb_alloc_msf(arena, page_size);
  pdb->info  = pdb_info_alloc(age, time_stamp, guid);
  pdb->dbi   = dbi_alloc(machine, age);
  pdb->gsi   = gsi_alloc();
  pdb->psi   = psi_alloc();
  pdb->type_servers[CV_TypeIndexSource_NULL] = push_array(arena, PDB_TypeServer, 1);
  for (U64 i = CV_TypeIndexSource_NULL + 1; i < ArrayCount(pdb->type_servers); ++i) {
    pdb->type_servers[i] = pdb_type_server_alloc(PDB_TYPE_SERVER_HASH_BUCKET_COUNT_CURRENT);
  }
  ProfEnd();
  return pdb;
}

internal PDB_Context *
pdb_alloc(U64 page_size, COFF_MachineType machine, COFF_TimeStamp time_stamp, U32 age, Guid guid)
{
  return pdb_alloc_(arena_alloc(.name = "PDB"), page_size, machine, time_stamp, age, guid);
}

internal void
pdb_release(PDB_Context *pdb)
{
  ProfBeginFunction();
  dbi_release(pdb->dbi);
  gsi_release(pdb->gsi);
  for (U64 i = 1; i < ArrayCount(pdb->type_servers); ++i) { pdb_type_server_release(&pdb->type_servers[i]); }
  arena_release(pdb->arena);
  ProfEnd();
}

internal void
pdb_set_machine(PDB_Context *pdb, COFF_MachineType machine)
{
  pdb->dbi->machine = machine;
}

internal void
pdb_set_guid(PDB_Context *pdb, Guid guid)
{
  pdb->info->guid = guid;
}

internal void
pdb_set_time_stamp(PDB_Context *pdb, COFF_TimeStamp time_stamp)
{
  pdb->info->time_stamp = time_stamp;
}

internal void
pdb_set_age(PDB_Context *pdb, U32 age)
{
  pdb->dbi->age = age;
  pdb->info->age = age;
}

internal COFF_MachineType
pdb_get_machine(PDB_Context *pdb)
{
  return pdb->dbi->machine;
}

internal COFF_TimeStamp
pdb_get_time_stamp(PDB_Context *pdb)
{
  return pdb->info->time_stamp;
}

internal U32
pdb_get_age(PDB_Context *pdb)
{
  return pdb->info->age;
}

internal Guid
pdb_get_guid(PDB_Context *pdb)
{
  return pdb->info->guid;
}

internal void
pdb_build(TP_Context *tp, TP_Arena *pool_temp, PDB_Context *pdb, CV_StringHashTable string_ht, B32 build_gsi, B32 is_stripped)
{
  ProfBeginFunction();
  
  PDB_InfoContext *info   = pdb->info;
  PDB_StringTable *strtab = &info->strtab;
  PDB_DbiContext  *dbi    = pdb->dbi;
  PDB_TypeServer  *tpi    = pdb->type_servers[CV_TypeIndexSource_TPI];
  PDB_TypeServer  *ipi    = pdb->type_servers[CV_TypeIndexSource_IPI];
  
  pdb_type_server_build(tp, tpi, strtab, pdb->msf, PDB_FixedStream_Tpi);
  if (info->flags & PDB_FeatureFlag_HAS_ID_STREAM) {
    pdb_type_server_build(tp, ipi, strtab, pdb->msf, PDB_FixedStream_Ipi);
  }

  dbi_build(tp, pdb->dbi, pdb->msf, PDB_FixedStream_Dbi, string_ht, is_stripped);
  pdb_info_build(pdb->info, pdb->msf, PDB_FixedStream_Info);

  if (build_gsi) {
    if (dbi->globals_sn == MSF_INVALID_STREAM_NUMBER) {
      dbi->globals_sn = msf_stream_alloc(pdb->msf);
    }
    if (dbi->publics_sn == MSF_INVALID_STREAM_NUMBER) {
      dbi->publics_sn = msf_stream_alloc(pdb->msf);
    }
    if (dbi->symbols_sn == MSF_INVALID_STREAM_NUMBER) {
      dbi->symbols_sn = msf_stream_alloc(pdb->msf);
    }
    psi_build(tp, pdb->psi, pdb->msf, dbi->publics_sn, dbi->symbols_sn);
    gsi_build(tp, pdb->gsi, pdb->msf, dbi->globals_sn, dbi->symbols_sn);
  }

  ProfEnd();
}

////////////////////////////////

internal String8
pdb_string_from_src_error(PDB_SrcError error)
{
  switch (error) {
  case PDB_SrcError_OK:                      return str8_lit("OK");
  case PDB_SrcError_DUPLICATE_NAME_STREAM:   return str8_lit("DUPLICATE_NAME_STREAM");
  case PDB_SrcError_DUPLICATE_ENTRY:         return str8_lit("DUPLICATE_ENTRY");
  case PDB_SrcError_UNABLE_TO_WRITE_DATA:    return str8_lit("UNABLE_TO_WRITE_DATA");
  case PDB_SrcError_UNSUPPORTED_COMPRESSION: return str8_lit("UNSUPPORTED_COMPRESSION");
  case PDB_SrcError_UNKNOWN:                 return str8_lit("UNKNOWN");
  }
  return str8(0,0);
}

