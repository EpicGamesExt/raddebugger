// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ PDB Parser Functions

internal PDB_Info*
pdb_info_from_data(Arena *arena, String8 data){
  ProfBegin("pdb_info_from_data");
  
  // get header
  PDB_InfoHeader *header = 0;
  if (data.size >= sizeof(*header)){
    header = (PDB_InfoHeader *)data.str;
  }
  
  PDB_Info *result = 0;
  if (header != 0){
    // read guid
    Guid *auth_guid = 0;
    U32 after_auth_guid_off = sizeof(*header);
    switch (header->version){
      case PDB_InfoVersion_VC70_DEP:
      case PDB_InfoVersion_VC70:
      case PDB_InfoVersion_VC80:
      case PDB_InfoVersion_VC110:
      case PDB_InfoVersion_VC140:
      {
        auth_guid = (Guid*)(data.str + after_auth_guid_off);
        after_auth_guid_off = sizeof(*header) + sizeof(*auth_guid);
      }break;
      
      default:
      {}break;
    }
    
    if (header->version != 0){
      // table layout: names
      U32 names_len_off = after_auth_guid_off;
      U32 names_len = 0;
      if (names_len_off + 4 <= data.size){
        names_len = *(U32*)(data.str + names_len_off);
      }
      
      U32 names_base_off = names_len_off + 4;
      U32 names_base_opl = names_base_off + names_len;
      
      // table layout: hash table
      U32 hash_table_count_off = names_base_opl;
      U32 hash_table_max_off = hash_table_count_off + 4;
      
      U32 hash_table_count = 0;
      U32 hash_table_max = 0;
      if (hash_table_max_off + 4 <= data.size){
        hash_table_count = *(U32*)(data.str + hash_table_count_off);
        hash_table_max = *(U32*)(data.str + hash_table_max_off);
      }
      
      // table layout: words
      U32 num_present_words_off = hash_table_max_off + 4;
      U32 num_present_words = 0;
      if (hash_table_max_off + 4 <= data.size){
        num_present_words = *(U32*)(data.str + num_present_words_off);
      }
      U32 present_words_array_off = num_present_words_off + 4;
      
      U32 num_deleted_words_off = present_words_array_off + num_present_words*sizeof(U32);
      U32 num_deleted_words = 0;
      if (num_deleted_words_off + 4 <= data.size){
        num_deleted_words = *(U32*)(data.str + num_deleted_words_off);
      }
      U32 deleted_words_array_off = num_deleted_words_off + 4;
      
      // table layout: epilogue
      U32 epilogue_base_off = deleted_words_array_off + num_deleted_words*sizeof(U32);
      
      if (epilogue_base_off <= data.size){
        U64 record_off = epilogue_base_off;
        
        // read table
        if (hash_table_count > 0) {
          PDB_InfoNode *first = 0;
          PDB_InfoNode *last = 0;
          
          for (U32 i = 0; i < hash_table_count; i += 1, record_off += 8){
            U32 *record = (U32*)(data.str + record_off);
            U32 relative_name_off = record[0];
            MSF_StreamNumber sn = (MSF_StreamNumber)record[1];
            
            U32 name_off = names_base_off + relative_name_off;
            String8 name = str8_cstring_capped((char*)(data.str + name_off),
                                               (char*)(data.str + names_base_opl));
            
            // push info node
            PDB_InfoNode *node = push_array(arena, PDB_InfoNode, 1);
            SLLQueuePush(first, last, node);
            node->string = name;
            node->sn = sn;
          }
          
          result = push_array(arena, PDB_Info, 1);
          result->first = first;
          result->last = last;
          result->auth_guid = *auth_guid;
        }
        
        // read PDB features
        PDB_FeatureFlags features = 0;
        for (; record_off + sizeof(PDB_FeatureSig) <= data.size; ) {
          PDB_FeatureSig sig = 0;
          record_off += str8_deserial_read_struct(data, record_off, &sig);
          switch (sig) {
            case PDB_FeatureSig_NULL: break;
            case PDB_FeatureSig_VC140:              features |= PDB_FeatureFlag_HAS_ID_STREAM;    break;
            case PDB_FeatureSig_NO_TYPE_MERGE:      features |= PDB_FeatureFlag_NO_TYPE_MERGE;    break;
            case PDB_FeatureSig_MINIMAL_DEBUG_INFO: features |= PDB_FeatureFlag_MINIMAL_DBG_INFO; break;
          }
        }
        result->features = features;
      }
    }
  }
  
  ProfEnd();
  
  return(result);
}

internal PDB_NamedStreamTable*
pdb_named_stream_table_from_info(Arena *arena, PDB_Info *info){
  ProfBegin("pdb_named_stream_table_from_info");
  
  // mapping "NamedStream" indexes to strings
  struct StreamNameIndexPair{
    PDB_NamedStream index;
    String8 name;
  };
  struct StreamNameIndexPair pairs[] = {
    {PDB_NamedStream_HeaderBlock, str8_lit("/src/headerblock")},
    {PDB_NamedStream_StringTable, str8_lit("/names")},
    {PDB_NamedStream_LinkInfo,    str8_lit("/LinkInfo")},
  };
  
  // build baked table
  PDB_NamedStreamTable *result = push_array(arena, PDB_NamedStreamTable, 1);
  struct StreamNameIndexPair *p = pairs;
  for (U64 i = 0; i < ArrayCount(pairs); i += 1, p += 1){
    String8 name = p->name;
    
    // get info node with this name
    PDB_InfoNode *match = 0;
    for (PDB_InfoNode *node = info->first;
         node != 0;
         node = node->next){
      if (str8_match(name, node->string, 0)){
        match = node;
        break;
      }
    }
    
    // if match found save stream number
    if (match != 0){
      result->sn[p->index] = match->sn;
    }
    else{
      result->sn[p->index] = 0xFFFF;
    }
  }
  
  ProfEnd();
  
  return(result);
}

internal PDB_Strtbl*
pdb_strtbl_from_data(Arena *arena, String8 data){
  ProfBegin("pdb_strtbl_from_data");
  
  // get header
  PDB_StringTableHeader *header = 0;
  if (sizeof(*header) <= data.size){
    header = (PDB_StringTableHeader *)data.str;
  }
  
  PDB_Strtbl *result = push_array(arena, PDB_Strtbl, 1);
  if (header != 0 && header->magic == PDB_StringTableHeader_MAGIC && header->version == 1){
    U32 strblock_size_off = sizeof(*header);
    U32 strblock_size = 0;
    if (strblock_size_off + 4 <= data.size){
      strblock_size = *(U32*)(data.str + strblock_size_off);
    }
    U32 strblock_off = strblock_size_off + 4;
    
    U32 bucket_count_off = strblock_off + strblock_size;
    U32 bucket_count = 0;
    if (bucket_count_off + 4 <= data.size){
      bucket_count = *(U32*)(data.str + bucket_count_off);
    }
    
    U32 bucket_array_off = bucket_count_off + 4;
    U32 bucket_array_size = bucket_count*sizeof(PDB_StringIndex);
    
    if (bucket_array_off + bucket_array_size <= data.size){
      result->data = data;
      result->bucket_count = bucket_count;
      result->strblock_min = strblock_off;
      result->strblock_max = strblock_off + strblock_size;
      result->buckets_min = bucket_array_off;
      result->buckets_max = bucket_array_off + bucket_array_size;
    }
  }
  
  ProfEnd();
  
  return(result);
}

internal PDB_DbiParsed*
pdb_dbi_from_data(Arena *arena, String8 data){
  ProfBegin("pdb_dbi_from_data");
  
  // get header
  PDB_DbiHeader *header = 0;
  if (sizeof(*header) <= data.size){
    header = (PDB_DbiHeader*)data.str;
  }
  
  PDB_DbiParsed *result = 0;
  if (header != 0 && header->sig == PDB_DbiHeaderSignature_V1){
    // extract range sizes
    U64 range_size[PDB_DbiRange_COUNT];
    range_size[PDB_DbiRange_ModuleInfo] = header->module_info_size;
    range_size[PDB_DbiRange_SecCon]     = header->sec_con_size;
    range_size[PDB_DbiRange_SecMap]     = header->sec_map_size;
    range_size[PDB_DbiRange_FileInfo]   = header->file_info_size;
    range_size[PDB_DbiRange_TSM]        = header->tsm_size;
    range_size[PDB_DbiRange_EcInfo]     = header->ec_info_size;
    range_size[PDB_DbiRange_DbgHeader]  = header->dbg_header_size;
    
    // fill result
    result = push_array(arena, PDB_DbiParsed, 1);
    result->data = data;
    result->machine_type = header->machine;
    result->gsi_sn = header->gsi_sn;
    result->psi_sn = header->psi_sn;
    result->sym_sn = header->sym_sn;
    
    
    // fill result's range offsets
    {
      U64 cursor = sizeof(*header);
      for (U64 i = 0; i < (U64)(PDB_DbiRange_COUNT); i += 1){
        result->range_off[i] = cursor;
        cursor += range_size[i];
        cursor = ClampTop(cursor, data.size);
      }
      result->range_off[PDB_DbiRange_COUNT] = cursor;
    }
    
    // fill result's debug streams
    U64 dbg_streams_min = result->range_off[PDB_DbiRange_DbgHeader];
    U64 dbg_streams_max = result->range_off[PDB_DbiRange_DbgHeader + 1];
    U64 dbg_streams_size_raw = dbg_streams_max - dbg_streams_min;
    U64 dbg_streams_size = ClampTop(dbg_streams_size_raw, sizeof(result->dbg_streams));
    MemoryCopy(result->dbg_streams, data.str + dbg_streams_min, dbg_streams_size);
    if (dbg_streams_size < sizeof(result->dbg_streams)){
      U64 filled_count = dbg_streams_size/sizeof(MSF_StreamNumber);
      MemorySet(result->dbg_streams + filled_count, 0xff,
                (ArrayCount(result->dbg_streams) - filled_count)*sizeof(MSF_StreamNumber));
    }
  }
  
  ProfEnd();
  
  return(result);
}

internal PDB_TpiParsed*
pdb_tpi_from_data(Arena *arena, String8 data){
  ProfBegin("pdb_tpi_from_data");
  
  // get header
  PDB_TpiHeader *header = 0;
  if (sizeof(*header) <= data.size){
    header = (PDB_TpiHeader*)data.str;
  }
  
  PDB_TpiParsed *result = 0;
  if (header != 0 && header->version == PDB_TpiVersion_IMPV80){
    U64 leaf_first_raw = header->header_size;
    U64 leaf_first     = ClampTop(leaf_first_raw, data.size);
    U64 leaf_opl_raw   = leaf_first + header->leaf_data_size;
    U64 leaf_opl       = ClampTop(leaf_opl_raw, data.size);
    
    result       = push_array(arena, PDB_TpiParsed, 1);
    result->data = data;
    
    result->leaf_first  = leaf_first;
    result->leaf_opl    = leaf_opl;
    result->itype_first = header->ti_lo;
    result->itype_opl   = header->ti_hi;
    
    result->hash_sn           = header->hash_sn;
    result->hash_sn_aux       = header->hash_sn_aux;
    result->hash_key_size     = header->hash_key_size;
    result->hash_bucket_count = header->hash_bucket_count;
    result->hash_vals_off     = header->hash_vals.off;
    result->hash_vals_size    = header->hash_vals.size;
    result->itype_off         = header->itype_offs.off;
    result->itype_size        = header->itype_offs.size;
    result->hash_adj_off      = header->hash_adj.off;
    result->hash_adj_size     = header->hash_adj.size;
  }
  
  ProfEnd();
  
  return(result);
}

internal PDB_TpiHashParsed*
pdb_tpi_hash_from_data(Arena *arena, PDB_Strtbl *strtbl, PDB_TpiParsed *tpi, String8 data, String8 aux_data){
  ProfBegin("pdb_tpi_hash_from_data");
  
  PDB_TpiHashParsed *result = 0;
  
  U32 stride = tpi->hash_key_size;
  U32 bucket_count = tpi->hash_bucket_count;
  if (1 <= stride && stride <= 8 && bucket_count > 0 && data.str != 0){
    
    // allocate buckets
    PDB_TpiHashBlock **buckets = push_array(arena, PDB_TpiHashBlock*, bucket_count);
    
    // extract "hash" array
    U8 *hashes = data.str + tpi->hash_vals_off;
    U8 *hash_opl = hashes + tpi->hash_vals_size;
    
    // for each index in the array...
    CV_TypeId itype = tpi->itype_first;
    U8 *hash_cursor = hashes;
    for (;hash_cursor + stride <= hash_opl;){
      
      // read index
      U64 bucket_idx = 0;
      MemoryCopy(&bucket_idx, hash_cursor, stride);
      
      // save to map
      if (bucket_idx < bucket_count){
        PDB_TpiHashBlock *block = buckets[bucket_idx];
        if (block == 0 || block->local_count == ArrayCount(block->itypes)){
          block = push_array(arena, PDB_TpiHashBlock, 1);
          SLLStackPush(buckets[bucket_idx], block);
        }
        if(block->local_count != 0)
        {
          MemoryCopy(block->itypes+1, block->itypes, sizeof(CV_TypeId)*block->local_count);
        }
        block->itypes[0] = itype;
        block->local_count += 1;
      }
      
      // advance cursor
      hash_cursor += stride;
      itype += 1;
    }
    
    //- rjf: compute bucket mask
    U32 bucket_mask = 0;
    if(IsPow2OrZero(bucket_count))
    {
      bucket_mask = bucket_count-1;
    }
    
    //- rjf: apply hash adjustments, to pull correct type IDs to the front of
    // the chains
    if(tpi->hash_adj_size != 0)
    {
      // NOTE(rjf): this table is laid out in the following format:
      //
      // pair_count: U32 -> # of name_index/type_index pairs
      // slot_count: U32 -> # of slots in this hash table
      // present_bit_array_count: U32 -> count for next array
      // present_bit_array: U32[present_bit_array_count] -> 1 bit per slot, "is present"
      // deleted_bit_array_count: U32 -> count for next array
      // deleted_bit_array: U32[deleted_bit_array_count] -> 1 bit per slot, "is deleted"
      // (U32, U32)[pair_count] -> array of name_index/type_index pairs
      //
      U8 *adjs = data.str + tpi->hash_adj_off;
      U8 *adjs_opl = adjs + tpi->hash_adj_size;
      U8 *adjs_cursor = adjs;
      U32 pair_count = *(U32 *)adjs_cursor;
      adjs_cursor += sizeof(U32);
      U32 slot_count = *(U32 *)adjs_cursor;
      adjs_cursor += sizeof(U32);
      U32 present_bit_array_count = *(U32 *)adjs_cursor; // skip present_bit_array
      adjs_cursor += sizeof(U32);
      adjs_cursor += present_bit_array_count*sizeof(U32);
      U32 deleted_bit_array_count = *(U32 *)adjs_cursor; // skip deleted_bit_array
      adjs_cursor += sizeof(U32);
      adjs_cursor += deleted_bit_array_count*sizeof(U32);
      U32 adjs_stride = sizeof(U32)*2;
      U32 pair_idx = 0;
      for(;adjs_cursor < adjs_opl && pair_idx < pair_count;
          adjs_cursor += adjs_stride, pair_idx += 1)
      {
        U32 name_off = ((U32 *)adjs_cursor)[0];
        CV_TypeId type_id = ((CV_TypeId *)adjs_cursor)[1];
        String8 string = pdb_strtbl_string_from_off(strtbl, name_off);
        U32 hash = pdb_hash_v1(string);
        U32 bucket_idx = ((bucket_mask != 0) ? hash&bucket_mask : hash%bucket_count);
        PDB_TpiHashBlock *prev_block = 0;
        for(PDB_TpiHashBlock *block = buckets[bucket_idx];
            block != 0;
            prev_block = block, block = block->next)
        {
          for(U32 local_idx = 0;
              local_idx < block->local_count && local_idx < ArrayCount(block->itypes);
              local_idx += 1)
          {
            if(block->itypes[local_idx] == type_id)
            {
              if(prev_block != 0)
              {
                prev_block->next = block->next;
                block->next = buckets[bucket_idx];
                buckets[bucket_idx] = block;
              }
              if(local_idx != 0)
              {
                Swap(CV_TypeId, block->itypes[0], block->itypes[local_idx]);
              }
              break;
            }
          }
        }
      }
    }
    
    // fill result
    result = push_array(arena, PDB_TpiHashParsed, 1);
    result->data = data;
    result->aux_data = aux_data;
    result->buckets = buckets;
    result->bucket_count = bucket_count;
    result->bucket_mask = bucket_mask;
  }
  
  ProfEnd();
  
  return(result);
}

internal PDB_GsiParsed*
pdb_gsi_from_data(Arena *arena, String8 data){
  ProfBegin("pdb_gsi_from_data");
  
  // get header
  PDB_GsiHeader *header = 0;
  if (sizeof(*header) <= data.size){
    header = (PDB_GsiHeader*)data.str;
  }
  
  PDB_GsiParsed *result = 0;
  if (header != 0 && header->signature == PDB_GsiSignature_Basic &&
      header->version == PDB_GsiVersion_V70 && header->bucket_data_size != 0){
    Temp scratch = scratch_begin(&arena, 1);
    
    // hash offset
    U32 hash_record_array_off = sizeof(*header);
    
    // bucket count
    U32 slot_count = 4097;
    
    // array offsets
    U32 bitmask_u32_count = CeilIntegerDiv(slot_count, 32);
    U32 bitmask_byte_size = bitmask_u32_count*4;
    U32 bitmask_off = hash_record_array_off + header->hash_record_arr_size;
    U32 offsets_off = bitmask_off + bitmask_byte_size;
    
    // get bitmask & packed offset arrays
    U8 *bitmasks = 0;
    U8 *packed_offsets = 0;
    if (bitmask_off + bitmask_byte_size <= data.size){
      bitmasks = (data.str + bitmask_off);
      packed_offsets = (data.str + offsets_off);
    }
    U32 packed_offset_count = (data.size - offsets_off)/4;
    
    // unpack
    U32 *unpacked_offsets = 0;
    if (packed_offsets != 0){
      unpacked_offsets = push_array(scratch.arena, U32, slot_count);
      
      U32 *bitmask_ptr = (U32*)bitmasks;
      U32 *bitmask_opl = bitmask_ptr + bitmask_u32_count;
      U32 *src_ptr = (U32*)packed_offsets;
      U32 *src_opl = src_ptr + packed_offset_count;
      U32 *dst_ptr = unpacked_offsets;
      U32 *dst_opl = dst_ptr + slot_count;
      for (; bitmask_ptr < bitmask_opl && src_ptr < src_opl; bitmask_ptr += 1){
        U32 bits = *bitmask_ptr;
        U32 src_max = (U32)(src_opl - src_ptr);
        U32 dst_max = (U32)(dst_opl - dst_ptr);
        U32 k_max0 = ClampTop(32, dst_max);
        U32 k_max  = ClampTop(k_max0, src_max);
        for (U32 k = 0; k < k_max; k += 1){
          if ((bits & 1) == 1){
            *dst_ptr = *src_ptr;
            src_ptr += 1;
          }
          else{
            *dst_ptr = 0xFFFFFFFF;
          }
          dst_ptr += 1;
          bits >>= 1;
        }
      }
      for (; dst_ptr < dst_opl; dst_ptr += 1){
        *dst_ptr = 0xFFFFFFFF;
      }
    }
    
    // construct table
    B32 bad_table = 0;
    if (unpacked_offsets != 0){
      result = push_array(arena, PDB_GsiParsed, 1);
      
      // hash records
      PDB_GsiHashRecord *hash_records = (PDB_GsiHashRecord*)(data.str + hash_record_array_off);
      U32 hash_record_count = header->hash_record_arr_size/sizeof(PDB_GsiHashRecord);
      
      // * We unpack hash records into the the table by scanning backwards through the
      // * hash records. Neighboring values in unpacked_offsets *sort of* form counts, but we 
      // * have to skip the max-U32s (sloppy PDB nonsense).
      
      // * PDBs put one extra slot at the beginning of the encoded buckets that is mean
      // * to be padding for modifying the buffer in place. After decoding there are 4096 buckets, 
      // * in the encoded buckets there are 4097. We are meant to drop the first one.
      
      // build table
      PDB_GsiHashRecord *hash_record_ptr = hash_records + hash_record_count - 1;
      U32 prev_n = hash_record_count;
      for (U32 i = slot_count; i > 1;){
        i -= 1;
        if (unpacked_offsets[i] != 0xFFFFFFFF){
          // determine hash record range to use
          // * The "12" here is the result of some really sloppy PDB magic.
          U32 n = unpacked_offsets[i]/12;
          if (n > prev_n){
            bad_table = 1;
            break;
          }
          U32 num_steps = prev_n - n;
          
          // fill this bucket
          U32 *bucket_offs = push_array_aligned(arena, U32, num_steps, 4);
          for (U32 j = num_steps; j > 0;){
            j -= 1;
            // * The "- 1" is more sloppy PDB magic.
            bucket_offs[j] = hash_record_ptr->symbol_off - 1;
            hash_record_ptr -= 1;
          }
          PDB_GsiBucket *bucket = &result->buckets[i];
          bucket->count = num_steps;
          bucket->offs = bucket_offs;
          
          // update prev_n
          prev_n = n;
        }
      }
    }
    
    scratch_end(scratch);
  }
  
  ProfEnd();
  
  return(result);
}

internal U64
pdb_gsi_symbol_from_string(PDB_GsiParsed *gsi, String8 symbol_data, String8 string)
{
  U64 result = max_U64;
  
  U32           hash       = pdb_hash_v1(string);
  U32           bucket_idx = hash % ArrayCount(gsi->buckets);
  PDB_GsiBucket bucket     = gsi->buckets[bucket_idx];
  
  for(U64 i = 0; i < bucket.count; ++i)
  {
    U32 off = bucket.offs[i];
    if(off + sizeof(CV_RecHeader) <= symbol_data.size)
    {
      CV_RecHeader *sym_header = (CV_RecHeader *)(symbol_data.str + off);
      
      if(sym_header->size >= sizeof(sym_header->kind))
      {
        U64  opl_off = off + sizeof(sym_header->size) + sym_header->size;
        U8  *sym_opl = (U8*)sym_header;
        if(opl_off <= symbol_data.size)
        {
          sym_opl = symbol_data.str + opl_off;
        }
        
        Rng1U64 raw_symbol_range = rng_1u64(off + sizeof(*sym_header), off + (sym_header->size - sizeof(sym_header->kind)));
        String8 raw_symbol       = str8_substr(symbol_data, raw_symbol_range);
        String8 sym_name         = cv_name_from_symbol(sym_header->kind, raw_symbol);
        
        if(str8_match(sym_name, string, 0))
        {
          result = off;
          goto exit;
        }
      }
    }
  }
  
  exit:;
  return result;
}

internal COFF_SectionHeaderArray
pdb_coff_section_array_from_data(Arena *arena, String8 data){
  COFF_SectionHeaderArray result = {0};
  result.count = data.size/sizeof(COFF_SectionHeader);
  result.v = (COFF_SectionHeader*)data.str;
  return(result);
}

internal PDB_CompUnitArray*
pdb_comp_unit_array_from_data(Arena *arena, String8 data){
  PDB_CompUnitNode *first = 0;
  PDB_CompUnitNode *last = 0;
  U64 count = 0;
  
  U64 cursor = 0;
  for (;cursor + sizeof(PDB_DbiCompUnitHeader) <= data.size;){
    // get header
    PDB_DbiCompUnitHeader *header = (PDB_DbiCompUnitHeader*)(data.str + cursor);
    
    // get names
    U64 name_off = cursor + sizeof(*header);
    String8 name = str8_cstring_capped((char *)(data.str + name_off), (char *)(data.str + data.size));
    
    U64 name2_off = name_off + name.size + 1;
    String8 name2 = str8_cstring_capped((char *)(data.str + name2_off), (char *)(data.str + data.size));
    
    U64 after_name2_off = name2_off + name2.size + 1;
    
    // save mod info
    PDB_CompUnitNode *node = push_array_no_zero(arena, PDB_CompUnitNode, 1);
    SLLQueuePush(first, last, node);
    count += 1;
    node->unit.sn = header->sn;
    node->unit.obj_name = name;
    node->unit.group_name = name2;
    
    // fill range offsets
    U32 *range_buf = node->unit.range_off;
    {
      // fill the buffer with size of each range
      range_buf[PDB_DbiCompUnitRange_Symbols] = header->symbols_size;
      range_buf[PDB_DbiCompUnitRange_C11] = header->c11_lines_size;
      range_buf[PDB_DbiCompUnitRange_C13] = header->c13_lines_size;
      Assert(PDB_DbiCompUnitRange_C13 + 1 == PDB_DbiCompUnitRange_COUNT);
      
      // in-place sizes -> offs conversion
      U64 i = 0;
      U32 range_cursor = 0;
      for (; i < (U64)(PDB_DbiCompUnitRange_COUNT); i += 1){
        U64 adv = range_buf[i];
        range_buf[i] = range_cursor;
        range_cursor += adv;
      }
      range_buf[i] = range_cursor;
      
      // skip 4 byte signature in symbols range
      if (range_buf[1] >= 4){
        range_buf[0] += 4;
      }
    }
    
    // update cursor
    cursor = AlignPow2(after_name2_off, 4);
  }
  
  
  // fill result
  PDB_CompUnit **units = push_array_no_zero(arena, PDB_CompUnit*, count);
  {
    U64 idx = 0;
    for (PDB_CompUnitNode *node = first;
         node != 0;
         node = node->next, idx += 1){
      units[idx] = &node->unit;
    }
  }
  
  PDB_CompUnitArray *result = push_array(arena, PDB_CompUnitArray, 1);
  result->units = units;
  result->count = count;
  
  return(result);
}

internal PDB_CompUnitContributionArray*
pdb_comp_unit_contribution_array_from_data(Arena *arena, String8 data, COFF_SectionHeaderArray sections)
{
  PDB_CompUnitContribution *contributions = 0;
  U64 count = 0;
  if (data.size >= sizeof(PDB_DbiSectionContribVersion)){
    PDB_DbiSectionContribVersion *version = (PDB_DbiSectionContribVersion*)data.str;
    
    // determine array layout from version
    U32 item_size = 0;
    U32 array_off = 0;
    switch (*version){
      default:
      {
        // TODO(allen): do we have a test case for this?
        item_size = sizeof(PDB_DbiSectionContrib40);
      }break;
      case PDB_DbiSectionContribVersion_1:
      {
        item_size = sizeof(PDB_DbiSectionContrib);
        array_off = sizeof(*version);
      }break;
      case PDB_DbiSectionContribVersion_2:
      {
        item_size = sizeof(PDB_DbiSectionContrib2);
        array_off = sizeof(*version);
      }break;
    }
    
    // allocate ranges
    U64 max_count = (data.size - array_off)/item_size;
    contributions = push_array_no_zero(arena, PDB_CompUnitContribution, max_count);
    
    // binary section info
    U64 section_count = sections.count;
    COFF_SectionHeader* section_headers = sections.v;
    
    // fill array
    PDB_CompUnitContribution *contribution_ptr = contributions;
    U64 cursor = array_off;
    for (; cursor + item_size <= data.size; cursor += item_size){
      PDB_DbiSectionContrib40 *sc = (PDB_DbiSectionContrib40*)(data.str + cursor);
      if (sc->size > 0 && 1 <= sc->sec && sc->sec <= section_count){
        U64 voff = section_headers[sc->sec - 1].voff + sc->sec_off;
        
        contribution_ptr->mod        = sc->mod;
        contribution_ptr->voff_first = voff;
        contribution_ptr->voff_opl   = voff + sc->size;
        contribution_ptr += 1;
      }
    }
    count = (U64)(contribution_ptr - contributions);
  }
  
  // fill result
  PDB_CompUnitContributionArray *result = push_array(arena, PDB_CompUnitContributionArray, 1);
  result->contributions = contributions;
  result->count = count;
  
  return(result);
}

////////////////////////////////
//~ PDB Dbi Functions

internal String8
pdb_data_from_dbi_range(PDB_DbiParsed *dbi, PDB_DbiRange range){
  String8 result = {0};
  if (range < PDB_DbiRange_COUNT){
    U64 first = dbi->range_off[range];
    U64 opl   = dbi->range_off[range + 1];
    result.str = dbi->data.str + first;
    result.size = opl - first;
  }
  return(result);
}

internal String8
pdb_data_from_unit_range(MSF_Parsed *msf, PDB_CompUnit *unit, PDB_DbiCompUnitRange range){
  String8 result = {0};
  if (range < PDB_DbiCompUnitRange_COUNT){
    String8 full_stream_data = msf_data_from_stream(msf, unit->sn);
    
    U64 first_raw = unit->range_off[range];
    U64 opl_raw = unit->range_off[range + 1];
    U64 opl = ClampTop(opl_raw, full_stream_data.size);
    U64 first = ClampTop(first_raw, opl);
    
    result.str = full_stream_data.str + first;
    result.size = opl - first;
  }
  return(result);
}

////////////////////////////////
//~ PDB Tpi Functions

internal String8
pdb_leaf_data_from_tpi(PDB_TpiParsed *tpi){
  String8 data = tpi->data;
  U8 *first = data.str + tpi->leaf_first;
  U8 *opl   = data.str + tpi->leaf_opl;
  String8 result = str8_range(first, opl);
  return(result);
}

internal CV_TypeIdArray
pdb_tpi_itypes_from_name(Arena *arena, PDB_TpiHashParsed *tpi_hash, CV_LeafParsed *leaf,
                         String8 name, B32 compare_unique_name, U32 output_cap){
  U32 hash = pdb_hash_v1(name);
  U32 bucket_idx = ((tpi_hash->bucket_mask != 0) ?
                    hash&tpi_hash->bucket_mask :
                    hash%tpi_hash->bucket_count);
  
  CV_TypeId itype_first = leaf->itype_first;
  CV_TypeId itype_opl = leaf->itype_opl;
  String8 data = leaf->data;
  
  Temp scratch = scratch_begin(&arena, 1);
  struct Chain{
    struct Chain *next;
    CV_TypeId itype;
  };
  struct Chain *first = 0;
  struct Chain *last = 0;
  U32 count = 0;
  
  for (PDB_TpiHashBlock *block = tpi_hash->buckets[bucket_idx];
       block != 0;
       block = block->next){
    U32 local_count = block->local_count;
    CV_TypeId *itype_ptr = block->itypes;
    for (U32 i = 0; i < local_count; i += 1, itype_ptr += 1){
      
      String8 extracted_name = {0};
      
      CV_TypeId itype = *itype_ptr;
      if (itype_first <= itype && itype < itype_opl){
        CV_RecRange *range = &leaf->leaf_ranges.ranges[itype - leaf->itype_first];
        if (range->off + range->hdr.size <= data.size){
          U8 *first = data.str + range->off + 2;
          U64 cap = range->hdr.size - 2;
          
          switch (range->hdr.kind){
            default:break;
            
            case CV_LeafKind_CLASS:
            case CV_LeafKind_STRUCTURE:
            {
              if (sizeof(CV_LeafStruct) <= cap){
                CV_LeafStruct *lf_struct = (CV_LeafStruct*)first;
                
                if (!(lf_struct->props & CV_TypeProp_FwdRef)){
                  // size
                  U8 *numeric_ptr = (U8*)(lf_struct + 1);
                  CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
                  
                  // name
                  U8 *name_ptr = numeric_ptr + size.encoded_size;
                  String8 name = str8_cstring_capped((char*)name_ptr, (char *)(first + cap));
                  
                  // unique name
                  if (compare_unique_name){
                    if (lf_struct->props & CV_TypeProp_HasUniqueName) {
                      U8 *unique_name_ptr = name_ptr + name.size + 1;
                      String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, (char *)(first + cap));
                      extracted_name = unique_name;
                    }
                  }
                  else{
                    extracted_name = name;
                  }
                }
              }
            }break;
            
            case CV_LeafKind_CLASS2:
            case CV_LeafKind_STRUCT2:
            {
              if (sizeof(CV_LeafStruct2) <= cap){
                CV_LeafStruct2 *lf_struct = (CV_LeafStruct2*)first;
                
                if (!(lf_struct->props & CV_TypeProp_FwdRef)){
                  // size
                  U8 *numeric_ptr = (U8*)(lf_struct + 1);
                  CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
                  
                  // name
                  U8 *name_ptr = numeric_ptr + size.encoded_size;
                  String8 name = str8_cstring_capped((char*)name_ptr, (char *)(first + cap));
                  
                  // unique name
                  if (compare_unique_name){
                    if (lf_struct->props & CV_TypeProp_HasUniqueName) {
                      U8 *unique_name_ptr = name_ptr + name.size + 1;
                      String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, (char *)(first + cap));
                      extracted_name = unique_name;
                    }
                  }
                  else{
                    extracted_name = name;
                  }
                }
              }
            }break;
            
            case CV_LeafKind_UNION:
            {
              if (sizeof(CV_LeafUnion) <= cap){
                CV_LeafUnion *lf_union = (CV_LeafUnion*)first;
                
                if (!(lf_union->props & CV_TypeProp_FwdRef)){
                  // size
                  U8 *numeric_ptr = (U8*)(lf_union + 1);
                  CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
                  
                  // name
                  U8 *name_ptr = numeric_ptr + size.encoded_size;
                  String8 name = str8_cstring_capped((char*)name_ptr, (char *)(first + cap));
                  
                  // unique name
                  if (compare_unique_name){
                    if (lf_union->props & CV_TypeProp_HasUniqueName) {
                      U8 *unique_name_ptr = name_ptr + name.size + 1;
                      String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, (char *)(first + cap));
                      extracted_name = unique_name;
                    }
                  }
                  else{
                    extracted_name = name;
                  }
                }
              }
            }break;
            
            case CV_LeafKind_ENUM:
            {
              if (sizeof(CV_LeafEnum) <= cap){
                CV_LeafEnum *lf_enum = (CV_LeafEnum*)first;
                
                if (!(lf_enum->props & CV_TypeProp_FwdRef)){
                  // name
                  U8 *name_ptr = (U8*)(lf_enum + 1);
                  String8 name = str8_cstring_capped((char*)name_ptr, (char *)(first + cap));
                  
                  // unique name
                  if (compare_unique_name){
                    if (lf_enum->props & CV_TypeProp_HasUniqueName) {
                      U8 *unique_name_ptr = name_ptr + name.size + 1;
                      String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, (char *)(first + cap));
                      extracted_name = unique_name;
                    }
                  }
                  else{
                    extracted_name = name;
                  }
                }
              }
            }break;
          }
        }
      }
      
      if (str8_match(extracted_name, name, 0)){
        struct Chain *chain = push_array(scratch.arena, struct Chain, 1);
        SLLQueuePush(first, last, chain);
        count += 1;
        chain->itype = itype;
        if (count == output_cap){
          goto dblbreak;
        }
      }
    }
  }
  
  dblbreak:;
  
  
  // assemble result
  CV_TypeId *itypes = push_array_aligned(arena, CV_TypeId, count, 8);
  {
    CV_TypeId *itype_ptr = itypes;
    for (struct Chain *node = first;
         node != 0;
         node = node->next, itype_ptr += 1){
      *itype_ptr = node->itype;
    }
  }
  CV_TypeIdArray result = {0};
  result.itypes = itypes;
  result.count = count;
  
  scratch_end(scratch);
  
  return(result);
}

internal CV_TypeId
pdb_tpi_first_itype_from_name(PDB_TpiHashParsed *tpi_hash, CV_LeafParsed *tpi_leaf,
                              String8 name, B32 compare_unique_name){
  Temp scratch = scratch_begin(0, 0);
  CV_TypeIdArray array = pdb_tpi_itypes_from_name(scratch.arena, tpi_hash, tpi_leaf,
                                                  name, compare_unique_name, 1);
  CV_TypeId result = 0;
  if (array.count > 0){
    result = array.itypes[0];
  }
  
  scratch_end(scratch);
  return(result);
}

////////////////////////////////
//~ PDB Strtbl Functions

internal String8
pdb_strtbl_string_from_off(PDB_Strtbl *strtbl, U32 off){
  U32 strblock_max = strtbl->strblock_max;
  U32 full_off_raw = strtbl->strblock_min + off;
  U32 full_off = ClampTop(full_off_raw, strblock_max);
  String8 result = str8_cstring_capped((char*)(strtbl->data.str + full_off),
                                       (char*)(strtbl->data.str + strblock_max));
  return(result);
}

internal String8
pdb_strtbl_string_from_index(PDB_Strtbl *strtbl, PDB_StringIndex idx){
  String8 result = {0};
  if (idx < strtbl->bucket_count){
    U32 off = *(U32*)(strtbl->data.str + strtbl->buckets_min + idx*4);
    result = pdb_strtbl_string_from_off(strtbl, off);
  }
  return(result);
}

internal U32
pdb_strtbl_off_from_string(PDB_Strtbl *strtbl, String8 string)
{
  U32 result = max_U32;
  
  U32 hash            = pdb_hash_v1(string);
  U32 best_bucket_idx = hash % strtbl->bucket_count;
  U32 bucket_idx      = best_bucket_idx;
  
  do
  {
    String8 test_string = pdb_strtbl_string_from_index(strtbl, bucket_idx);
    
    if(test_string.size == 0)
    {
      break;
    }
    
    if(str8_match(test_string, string, 0))
    {
      result = bucket_idx;
      break;
    }
    
    bucket_idx = (bucket_idx+1) % strtbl->buckets_max;
  } while (bucket_idx != best_bucket_idx);
  
  return result;
}
