// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ PDB Stringize Functions

internal void
pdb_stringize_tpi_hash(Arena *arena, String8List *out, PDB_TpiHashParsed *hash){
  U32 bucket_count = hash->bucket_count;
  str8_list_pushf(arena, out, "bucket_count=%u\n\n", bucket_count);
  for (U32 i = 0; i < bucket_count; i += 1){
    if (hash->buckets[i] != 0){
      str8_list_pushf(arena, out, "bucket[%u]:\n", i);
      for (PDB_TpiHashBlock *block = hash->buckets[i];
           block != 0;
           block = block->next){
        U32 local_count = block->local_count;
        CV_TypeId *itype_ptr = block->itypes;
        for (U32 j = 0; j < local_count; j += 1, itype_ptr += 1){
          str8_list_pushf(arena, out, " %u\n", *itype_ptr);
        }
      }
      str8_list_push(arena, out, str8_lit("\n"));
    }
  }
}
