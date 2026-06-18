// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: PDB -> (Codeview -> RDI) TPI Hash Table Building

internal CV2R_TPIHash *
p2r_cv2r_tpi_hash_from_data(Arena *arena, PDB_Strtbl *strtbl, PDB_TpiParsed *tpi, String8 data)
{
  CV2R_TPIHash *result = push_array(arena, CV2R_TPIHash, 1);
  U32 stride = tpi->hash_key_size;
  U32 bucket_count = tpi->hash_bucket_count;
  if(1 <= stride && stride <= 8 && bucket_count > 0 && data.size != 0)
  {
    // allocate buckets
    CV2R_TPIHashBlock **buckets = push_array(arena, CV2R_TPIHashBlock*, bucket_count);
    
    // extract "hash" array
    U8 *hashes = data.str + tpi->hash_vals_off;
    U8 *hash_opl = hashes + tpi->hash_vals_size;
    
    // for each index in the array...
    CV_TypeId itype = tpi->itype_first;
    U8 *hash_cursor = hashes;
    for(;hash_cursor + stride <= hash_opl;)
    {
      // read index
      U64 bucket_idx = 0;
      MemoryCopy(&bucket_idx, hash_cursor, stride);
      
      // save to map
      if(bucket_idx < bucket_count)
      {
        CV2R_TPIHashBlock *block = buckets[bucket_idx];
        if(block == 0 || block->local_count == ArrayCount(block->itypes))
        {
          block = push_array(arena, CV2R_TPIHashBlock, 1);
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
        CV2R_TPIHashBlock *prev_block = 0;
        for(CV2R_TPIHashBlock *block = buckets[bucket_idx];
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
    result->data = data;
    result->buckets = buckets;
    result->bucket_count = bucket_count;
    result->bucket_mask = bucket_mask;
  }
  return result;
}

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point (Revised)

internal RDIM_BakeParams
p2r_convert2(Arena *arena, P2R_ConvertParams *params)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //////////////////////////////////////////////////////////////
  //- rjf: do base MSF parse
  //
  MSF_Parsed *msf = 0;
  ProfScope("do base MSF parse")
  {
    // rjf: setup output buckets
    MSF_RawStreamTable *msf_raw_stream_table = 0;
    U64 *msf_stream_take_counter = 0;
    if(lane_idx() == 0)
    {
      msf_raw_stream_table = msf_raw_stream_table_from_data(scratch.arena, params->input_pdb_data);
      msf = push_array(scratch.arena, MSF_Parsed, 1);
      msf->page_size = msf_raw_stream_table->page_size;
      msf->page_count = msf_raw_stream_table->total_page_count;
      msf->stream_count = msf_raw_stream_table->stream_count;
      msf->streams = push_array(scratch.arena, String8, msf->stream_count);
      msf_stream_take_counter = push_array(scratch.arena, U64, 1);
    }
    lane_sync_u64(&msf, 0);
    lane_sync_u64(&msf_raw_stream_table, 0);
    
    // rjf: do wide fill
    {
      lane_sync_u64(&msf_stream_take_counter, 0);
      for(;;)
      {
        U64 stream_idx = ins_atomic_u64_inc_eval(msf_stream_take_counter) - 1;
        if(stream_idx >= msf->stream_count)
        {
          break;
        }
        msf->streams[stream_idx] = msf_data_from_stream_number(arena, params->input_pdb_data, msf_raw_stream_table, stream_idx);
      }
    }
    lane_sync();
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: do top-level MSF/PDB extraction
  //
  PDB_Info *pdb_info = 0;
  PDB_NamedStreamTable *named_streams = 0;
  ProfScope("do top-level MSF/PDB extraction") if(lane_idx() == 0)
  {
    ProfScope("parse PDB info")
    {
      String8 info_data = msf_data_from_stream(msf, PDB_FixedStream_Info);
      pdb_info = pdb_info_from_data(scratch.arena, info_data);
      if(pdb_info->features & PDB_FeatureFlag_MINIMAL_DBG_INFO)
      {
        log_user_error(str8_lit("PDB was linked with /DEBUG:FASTLINK; partial debug info is not supported. Please relink using /DEBUG:FULL."));
      }
    }
    ProfScope("parse named streams table")
    {
      named_streams = pdb_named_stream_table_from_info(scratch.arena, pdb_info);
    }
  }
  lane_sync_u64(&pdb_info, 0);
  lane_sync_u64(&named_streams, 0);
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse PDB strtbl & top-level streams
  //
  PDB_Strtbl *strtbl = 0;
  String8 raw_strtbl = {0};
  PDB_DbiParsed *dbi = 0;
  PDB_TpiParsed *tpi = 0;
  PDB_TpiParsed *ipi = 0;
  ProfScope("parse PDB strtbl & top-level streams")
  {
    if(lane_idx() == lane_from_task_idx(0)) ProfScope("parse PDB strtbl")
    {
      MSF_StreamNumber strtbl_sn = named_streams->sn[PDB_NamedStream_StringTable];
      String8 strtbl_data = msf_data_from_stream(msf, strtbl_sn);
      strtbl = pdb_strtbl_from_data(scratch.arena, strtbl_data);
      raw_strtbl = str8_substr(strtbl_data, rng_1u64(strtbl->strblock_min, strtbl->strblock_max));
    }
    if(lane_idx() == lane_from_task_idx(1)) ProfScope("parse DBI")
    {
      String8 dbi_data = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
      dbi = pdb_dbi_from_data(scratch.arena, dbi_data);
    }
    if(lane_idx() == lane_from_task_idx(2)) ProfScope("parse TPI")
    {
      String8 tpi_data = msf_data_from_stream(msf, PDB_FixedStream_Tpi);
      tpi = pdb_tpi_from_data(scratch.arena, tpi_data);
    }
    if(lane_idx() == lane_from_task_idx(3)) ProfScope("parse IPI")
    {
      String8 ipi_data = msf_data_from_stream(msf, PDB_FixedStream_Ipi);
      ipi = pdb_tpi_from_data(scratch.arena, ipi_data);
    }
  }
  lane_sync_u64(&strtbl, lane_from_task_idx(0));
  lane_sync_u64(&raw_strtbl.size, lane_from_task_idx(0));
  lane_sync_u64(&raw_strtbl.str, lane_from_task_idx(0));
  lane_sync_u64(&dbi, lane_from_task_idx(1));
  lane_sync_u64(&tpi, lane_from_task_idx(2));
  lane_sync_u64(&ipi, lane_from_task_idx(3));
  
  //////////////////////////////////////////////////////////////
  //- rjf: unpack DBI
  //
  COFF_SectionHeaderArray coff_sections = {0};
  PDB_GsiParsed *gsi = 0;
  PDB_GsiParsed *psi_gsi_part = 0;
  ProfScope("unpack DBI")
  {
    if(lane_idx() == lane_from_task_idx(0)) ProfScope("parse COFF sections")
    {
      MSF_StreamNumber section_stream = dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER];
      String8 section_data = msf_data_from_stream(msf, section_stream);
      coff_sections = pdb_coff_section_array_from_data(scratch.arena, section_data);
    }
    if(lane_idx() == lane_from_task_idx(1)) ProfScope("parse GSI")
    {
      String8 gsi_data = msf_data_from_stream(msf, dbi->gsi_sn);
      gsi = pdb_gsi_from_data(scratch.arena, gsi_data);
    }
    if(lane_idx() == lane_from_task_idx(2)) ProfScope("parse GSI part of PSI")
    {
      String8 psi_data = msf_data_from_stream(msf, dbi->psi_sn);
      String8 psi_data_gsi_part = str8_range(psi_data.str + sizeof(PDB_PsiHeader), psi_data.str + psi_data.size);
      psi_gsi_part = pdb_gsi_from_data(scratch.arena, psi_data_gsi_part);
    }
  }
  lane_sync_u64(&coff_sections.v, lane_from_task_idx(0));
  lane_sync_u64(&coff_sections.count, lane_from_task_idx(0));
  lane_sync_u64(&gsi, lane_from_task_idx(1));
  lane_sync_u64(&psi_gsi_part, lane_from_task_idx(2));
  
  //////////////////////////////////////////////////////////////
  //- rjf: hash EXE, parse TPI/IPI hash/leaf & global symbol stream & comp units
  //
  CV2R_TPIHash *tpi_hash = 0;
  CV_LeafParsed *tpi_leaf = 0;
  CV2R_TPIHash *ipi_hash = 0;
  CV_LeafParsed *ipi_leaf = 0;
  PDB_CompUnitArray *comp_units = 0;
  PDB_CompUnitContributionArray *comp_unit_contributions = 0;
  ProfScope("hash EXE, parse TPI/IPI hash/leaf & global symbol stream & comp units")
  {
    if(lane_idx() == lane_from_task_idx(0)) ProfScope("parse TPI hash")
    {
      String8 hash_data = msf_data_from_stream(msf, tpi->hash_sn);
      if(!(params->subset_flags & (RDIM_SubsetFlag_Types|RDIM_SubsetFlag_UDTs)))
      {
        hash_data = str8_zero();
      }
      tpi_hash = p2r_cv2r_tpi_hash_from_data(scratch.arena, strtbl, tpi, hash_data);
    }
    if(lane_idx() == lane_from_task_idx(1)) ProfScope("parse TPI leaf")
    {
      String8 leaf_data = pdb_leaf_data_from_tpi(tpi);
      tpi_leaf = cv_leaf_from_data(scratch.arena, leaf_data, tpi->itype_first);
    }
    if(lane_idx() == lane_from_task_idx(2)) ProfScope("parse IPI hash")
    {
      String8 hash_data = msf_data_from_stream(msf, ipi->hash_sn);
      if(!(params->subset_flags & (RDIM_SubsetFlag_Types|RDIM_SubsetFlag_UDTs)))
      {
        hash_data = str8_zero();
      }
      ipi_hash = p2r_cv2r_tpi_hash_from_data(scratch.arena, strtbl, ipi, hash_data);
    }
    if(lane_idx() == lane_from_task_idx(3)) ProfScope("parse IPI leaf")
    {
      String8 leaf_data = pdb_leaf_data_from_tpi(ipi);
      ipi_leaf = cv_leaf_from_data(scratch.arena, leaf_data, ipi->itype_first);
    }
    if(lane_idx() == lane_from_task_idx(4)) ProfScope("parse compilation units")
    {
      String8 comp_units_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo);
      comp_units = pdb_comp_unit_array_from_data(scratch.arena, comp_units_data);
    }
    if(lane_idx() == lane_from_task_idx(5)) ProfScope("parse compilation unit contributions")
    {
      String8 contribs_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_SecCon);
      comp_unit_contributions = push_array(scratch.arena, PDB_CompUnitContributionArray, 1);
      comp_unit_contributions[0] = pdb_comp_unit_contribution_array_from_data(scratch.arena, contribs_data, coff_sections);
    }
  }
  lane_sync_u64(&tpi_hash, lane_from_task_idx(0));
  lane_sync_u64(&tpi_leaf, lane_from_task_idx(1));
  lane_sync_u64(&ipi_hash, lane_from_task_idx(2));
  lane_sync_u64(&ipi_leaf, lane_from_task_idx(3));
  lane_sync_u64(&comp_units, lane_from_task_idx(4));
  lane_sync_u64(&comp_unit_contributions, lane_from_task_idx(5));
  
  //////////////////////////////////////////////////////////////
  //- rjf: bucket compilation unit contributions
  //
  RDIM_Rng1U64ChunkList *unit_ranges = 0;
  ProfScope("bucket compilation unit contributions") if(lane_idx() == 0)
  {
    unit_ranges = push_array(scratch.arena, RDIM_Rng1U64ChunkList, comp_units->count + 1);
    for(U64 comp_unit_idx = 0; comp_unit_idx < comp_unit_contributions->count; comp_unit_idx += 1)
    {
      PDB_CompUnitContribution *contribution = &comp_unit_contributions->contributions[comp_unit_idx];
      if(contribution->mod < comp_units->count)
      {
        RDIM_Rng1U64 r = {contribution->voff_first, contribution->voff_opl};
        rdim_rng1u64_chunk_list_push(arena, &unit_ranges[contribution->mod + 1], 256, r);
      }
    }
  }
  lane_sync_u64(&unit_ranges, 0);
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse all syms & c13 line info streams
  //
  U64 all_syms_count = 0;
  CV_SymParsed **all_syms = 0;
  CV_C13Parsed **all_c13s = 0;
  ProfScope("parse all syms & c13 line info streams")
  {
    //- rjf: setup outputs
    U64 *task_counter = 0;
    if(lane_idx() == 0)
    {
      all_syms_count = comp_units->count+1; // +1 for global symbol stream from DBI
      all_syms = push_array(scratch.arena, CV_SymParsed *, all_syms_count);
      all_c13s = push_array(scratch.arena, CV_C13Parsed *, all_syms_count);
      task_counter = push_array(scratch.arena, U64, 1);
    }
    lane_sync_u64(&all_syms_count, 0);
    lane_sync_u64(&all_syms, 0);
    lane_sync_u64(&all_c13s, 0);
    
    //- rjf: wide fill
    {
      lane_sync_u64(&task_counter, 0);
      U64 task_count = all_syms_count;
      for(;;)
      {
        U64 task_idx = ins_atomic_u64_inc_eval(task_counter) - 1;
        if(task_idx >= task_count)
        {
          break;
        }
        if(task_idx > 0)
        {
          PDB_CompUnit *unit = comp_units->units[task_idx-1];
          String8 unit_sym_data = pdb_data_from_unit_range(msf, unit, PDB_DbiCompUnitRange_Symbols);
          String8 unit_c13_data = pdb_data_from_unit_range(msf, unit, PDB_DbiCompUnitRange_C13);
          all_syms[task_idx] = cv_sym_from_data(arena, unit_sym_data, 4);
          all_c13s[task_idx] = cv_c13_parsed_from_data(arena, unit_c13_data, raw_strtbl, coff_sections);
        }
        else
        {
          String8 global_sym_data = msf_data_from_stream(msf, dbi->sym_sn);
          all_syms[task_idx] = cv_sym_from_data(arena, global_sym_data, 4);
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce cv2r units from pdb's units
  //
  U64 cv2r_comp_units_count = comp_units->count + 1; // +1 for global info stream
  CV2R_CompUnit *cv2r_comp_units = 0;
  {
    if(lane_idx() == 0)
    {
      cv2r_comp_units = push_array(scratch.arena, CV2R_CompUnit, cv2r_comp_units_count);
    }
    lane_sync_u64(&cv2r_comp_units, 0);
    Rng1U64 range = lane_range(cv2r_comp_units_count);
    for EachInRange(idx, range)
    {
      if(idx > 0)
      {
        PDB_CompUnit *src_unit = comp_units->units[idx-1];
        cv2r_comp_units[idx].obj_name   = src_unit->obj_name;
        cv2r_comp_units[idx].group_name = src_unit->group_name;
      }
      else
      {
        cv2r_comp_units[idx].obj_name = s("*global");
      }
      cv2r_comp_units[idx].ranges = unit_ranges[idx];
      cv2r_comp_units[idx].sym = all_syms[idx];
      cv2r_comp_units[idx].c13 = all_c13s[idx];
    }
    lane_sync();
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce cv2r comp unit contributions
  //
  U64 cv2r_comp_unit_contributions_count = comp_unit_contributions->count;
  CV2R_CompUnitContribution *cv2r_comp_unit_contributions = 0;
  {
    if(lane_idx() == 0)
    {
      cv2r_comp_unit_contributions = push_array(scratch.arena, CV2R_CompUnitContribution, cv2r_comp_unit_contributions_count);
    }
    lane_sync_u64(&cv2r_comp_unit_contributions, 0);
    Rng1U64 range = lane_range(cv2r_comp_unit_contributions_count);
    for EachInRange(idx, range)
    {
      cv2r_comp_unit_contributions[idx].mod        = comp_unit_contributions->contributions[idx].mod;
      cv2r_comp_unit_contributions[idx].voff_first = comp_unit_contributions->contributions[idx].voff_first;
      cv2r_comp_unit_contributions[idx].voff_opl   = comp_unit_contributions->contributions[idx].voff_opl;
    }
    lane_sync();
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce cv2r sections from coff
  //
  U64 cv2r_sections_count = coff_sections.count;
  CV2R_Section *cv2r_sections = 0;
  {
    if(lane_idx() == 0)
    {
      cv2r_sections = push_array(scratch.arena, CV2R_Section, cv2r_sections_count);
    }
    lane_sync_u64(&cv2r_sections, 0);
    Rng1U64 range = lane_range(cv2r_sections_count);
    for EachInRange(idx, range)
    {
      COFF_SectionHeader *src = &coff_sections.v[idx];
      CV2R_Section *dst = &cv2r_sections[idx];
      char *name_first = (char *)src->name;
      char *name_opl   = name_first + sizeof(src->name);
      dst->name  = str8_cstring_capped(name_first, name_opl);
      dst->flags = c2r_rdi_binary_section_flags_from_coff_section_flags(src->flags);
      dst->voff  = src->voff;
      dst->vsize = src->vsize;
      dst->foff  = src->foff;
      dst->fsize = src->fsize;
    }
    lane_sync();
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce cv2r string table
  //
  CV2R_StringTable cv2r_strtbl = {0};
  {
    cv2r_strtbl.data         = strtbl->data;
    cv2r_strtbl.bucket_count = strtbl->bucket_count;
    cv2r_strtbl.strblock_min = strtbl->strblock_min;
    cv2r_strtbl.strblock_max = strtbl->strblock_max;
    cv2r_strtbl.buckets_min  = strtbl->buckets_min;
    cv2r_strtbl.buckets_max  = strtbl->buckets_max;
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: bundle codeview conversion parameters
  //
  CV2R_ConvertParams cv2r_params = {0};
  {
    cv2r_params.exe_name                      = params->input_exe_name;
    cv2r_params.exe_data                      = params->input_exe_data;
    cv2r_params.guid                          = pdb_info->auth_guid;
    cv2r_params.comp_units_count              = cv2r_comp_units_count;
    cv2r_params.comp_units                    = cv2r_comp_units;
    cv2r_params.comp_unit_contributions_count = cv2r_comp_unit_contributions_count;
    cv2r_params.comp_unit_contributions       = cv2r_comp_unit_contributions;
    cv2r_params.sections_count                = cv2r_sections_count;
    cv2r_params.sections                      = cv2r_sections;
    cv2r_params.strtbl                        = &cv2r_strtbl;
    cv2r_params.tpi_leaf                      = tpi_leaf;
    cv2r_params.ipi_leaf                      = ipi_leaf;
    cv2r_params.tpi_hash                      = tpi_hash;
    cv2r_params.ipi_hash                      = ipi_hash;
    cv2r_params.subset_flags                  = params->subset_flags;
    cv2r_params.deterministic                 = params->deterministic;
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: convert from top-level & all parsed codeview datas
  //
  RDIM_BakeParams result = cv2r_convert(arena, &cv2r_params);
  
  lane_sync();
  scratch_end(scratch);
  return result;
}
