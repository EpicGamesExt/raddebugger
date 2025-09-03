// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDIM_LocationInfo
p2r2_location_info_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection)
{
  RDIM_LocationInfo result = {0};
  if(0 <= offset && offset <= (S64)max_U16)
  {
    if(extra_indirection)
    {
      result.kind = RDI_LocationKind_AddrAddrRegPlusU16;
      result.reg_code = reg_code;
      result.offset = offset;
    }
    else
    {
      result.kind = RDI_LocationKind_AddrRegPlusU16;
      result.reg_code = reg_code;
      result.offset = offset;
    }
  }
  else
  {
    RDIM_EvalBytecode bytecode = {0};
    U32 regread_param = RDI_EncodeRegReadParam(reg_code, reg_byte_size, reg_byte_pos);
    rdim_bytecode_push_op(arena, &bytecode, RDI_EvalOp_RegRead, regread_param);
    rdim_bytecode_push_sconst(arena, &bytecode, offset);
    rdim_bytecode_push_op(arena, &bytecode, RDI_EvalOp_Add, 0);
    if(extra_indirection)
    {
      U64 addr_size = rdi_addr_size_from_arch(arch);
      rdim_bytecode_push_op(arena, &bytecode, RDI_EvalOp_MemRead, addr_size);
    }
    result.kind = RDI_LocationKind_AddrBytecodeStream;
    result.bytecode = bytecode;
  }
  return result;
}

internal void
p2r2_location_over_lvar_addr_range(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_LocationSet *locset, RDIM_Location *location, CV_LvarAddrRange *range, COFF_SectionHeader *section, CV_LvarAddrGap *gaps, U64 gap_count)
{
  
}

internal RDIM_BakeParams
p2r2_convert(Arena *arena, P2R_ConvertParams *params)
{
  //////////////////////////////////////////////////////////////
  //- rjf: do base MSF parse
  //
  {
    // rjf: setup output buckets
    if(lane_idx() == 0)
    {
      p2r2_shared = push_array(arena, P2R2_Shared, 1);
      p2r2_shared->msf_raw_stream_table = msf_raw_stream_table_from_data(arena, params->input_pdb_data);
      p2r2_shared->msf = push_array(arena, MSF_Parsed, 1);
      p2r2_shared->msf->page_size    = p2r2_shared->msf_raw_stream_table->page_size;
      p2r2_shared->msf->page_count   = p2r2_shared->msf_raw_stream_table->total_page_count;
      p2r2_shared->msf->stream_count = p2r2_shared->msf_raw_stream_table->stream_count;
      p2r2_shared->msf->streams      = push_array(arena, String8, p2r2_shared->msf->stream_count);
      p2r2_shared->msf_stream_lane_counter = 0;
    }
    lane_sync();
    
    // rjf: do wide fill
    {
      for(;;)
      {
        U64 stream_num = ins_atomic_u64_inc_eval(&p2r2_shared->msf_stream_lane_counter);
        if(stream_num < 1 || p2r2_shared->msf->stream_count < stream_num)
        {
          break;
        }
        U64 stream_idx = stream_num-1;
        p2r2_shared->msf->streams[stream_idx] = msf_data_from_stream_number(arena, params->input_pdb_data, p2r2_shared->msf_raw_stream_table, stream_idx);
      }
    }
  }
  lane_sync();
  MSF_Parsed *msf = p2r2_shared->msf;
  
  //////////////////////////////////////////////////////////////
  //- rjf: do top-level MSF/PDB extraction
  //
  ProfScope("do top-level MSF/PDB extraction") if(lane_idx() == 0)
  {
    ProfScope("parse PDB info")
    {
      String8 info_data = msf_data_from_stream(msf, PDB_FixedStream_Info);
      p2r2_shared->pdb_info = pdb_info_from_data(arena, info_data);
      if(p2r2_shared->pdb_info->features & PDB_FeatureFlag_MINIMAL_DBG_INFO)
      {
        log_user_error(str8_lit("PDB was linked with /DEBUG:FASTLINK; partial debug info is not supported. Please relink using /DEBUG:FULL."));
      }
    }
    ProfScope("parse named streams table")
    {
      p2r2_shared->named_streams = pdb_named_stream_table_from_info(arena, p2r2_shared->pdb_info);
    }
  }
  lane_sync();
  PDB_Info *pdb_info = p2r2_shared->pdb_info;
  PDB_NamedStreamTable *named_streams = p2r2_shared->named_streams;
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse PDB strtbl & top-level streams
  //
  ProfScope("parse PDB strtbl & top-level streams")
  {
    if(lane_idx() == lane_from_task_idx(0)) ProfScope("parse PDB strtbl")
    {
      MSF_StreamNumber strtbl_sn = named_streams->sn[PDB_NamedStream_StringTable];
      String8 strtbl_data = msf_data_from_stream(msf, strtbl_sn);
      p2r2_shared->strtbl = pdb_strtbl_from_data(arena, strtbl_data);
      p2r2_shared->raw_strtbl = str8_substr(strtbl_data, rng_1u64(p2r2_shared->strtbl->strblock_min, p2r2_shared->strtbl->strblock_max));
    }
    if(lane_idx() == lane_from_task_idx(1)) ProfScope("parse DBI")
    {
      String8 dbi_data = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
      p2r2_shared->dbi = pdb_dbi_from_data(arena, dbi_data);
    }
    if(lane_idx() == lane_from_task_idx(2)) ProfScope("parse TPI")
    {
      String8 tpi_data = msf_data_from_stream(msf, PDB_FixedStream_Tpi);
      p2r2_shared->tpi = pdb_tpi_from_data(arena, tpi_data);
    }
    if(lane_idx() == lane_from_task_idx(3)) ProfScope("parse IPI")
    {
      String8 ipi_data = msf_data_from_stream(msf, PDB_FixedStream_Ipi);
      p2r2_shared->ipi = pdb_tpi_from_data(arena, ipi_data);
    }
  }
  lane_sync();
  PDB_Strtbl *strtbl = p2r2_shared->strtbl;
  String8 raw_strtbl = p2r2_shared->raw_strtbl;
  PDB_DbiParsed *dbi = p2r2_shared->dbi;
  PDB_TpiParsed *tpi = p2r2_shared->tpi;
  PDB_TpiParsed *ipi = p2r2_shared->ipi;
  
  //////////////////////////////////////////////////////////////
  //- rjf: unpack DBI
  //
  ProfScope("unpack DBI")
  {
    if(lane_idx() == lane_from_task_idx(0)) ProfScope("parse COFF sections")
    {
      MSF_StreamNumber section_stream = dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER];
      String8 section_data = msf_data_from_stream(msf, section_stream);
      p2r2_shared->coff_sections = pdb_coff_section_array_from_data(arena, section_data);
    }
    if(lane_idx() == lane_from_task_idx(1)) ProfScope("parse GSI")
    {
      String8 gsi_data = msf_data_from_stream(msf, dbi->gsi_sn);
      p2r2_shared->gsi = pdb_gsi_from_data(arena, gsi_data);
    }
    if(lane_idx() == lane_from_task_idx(2)) ProfScope("parse GSI part of PSI")
    {
      String8 psi_data = msf_data_from_stream(msf, dbi->psi_sn);
      String8 psi_data_gsi_part = str8_range(psi_data.str + sizeof(PDB_PsiHeader), psi_data.str + psi_data.size);
      p2r2_shared->psi_gsi_part = pdb_gsi_from_data(arena, psi_data_gsi_part);
    }
  }
  lane_sync();
  COFF_SectionHeaderArray coff_sections = p2r2_shared->coff_sections;
  PDB_GsiParsed *gsi = p2r2_shared->gsi;
  PDB_GsiParsed *psi_gsi_part = p2r2_shared->psi_gsi_part;
  
  //////////////////////////////////////////////////////////////
  //- rjf: hash EXE, parse TPI/IPI hash/leaf & global symbol stream & comp units
  //
  ProfScope("hash EXE, parse TPI/IPI hash/leaf & global symbol stream & comp units")
  {
    if(lane_idx() == lane_from_task_idx(0)) ProfScope("hash EXE")
    {
      p2r2_shared->exe_hash = rdi_hash(params->input_exe_data.str, params->input_exe_data.size);
    }
    if(lane_idx() == lane_from_task_idx(1)) ProfScope("parse TPI hash")
    {
      String8 hash_data = msf_data_from_stream(msf, tpi->hash_sn);
      String8 aux_data  = msf_data_from_stream(msf, tpi->hash_sn_aux);
      p2r2_shared->tpi_hash = pdb_tpi_hash_from_data(arena, strtbl, tpi, hash_data, aux_data);
    }
    if(lane_idx() == lane_from_task_idx(2)) ProfScope("parse TPI leaf")
    {
      String8 leaf_data = pdb_leaf_data_from_tpi(tpi);
      p2r2_shared->tpi_leaf = cv_leaf_from_data(arena, leaf_data, tpi->itype_first);
    }
    if(lane_idx() == lane_from_task_idx(3)) ProfScope("parse IPI hash")
    {
      String8 hash_data = msf_data_from_stream(msf, ipi->hash_sn);
      String8 aux_data  = msf_data_from_stream(msf, ipi->hash_sn_aux);
      p2r2_shared->ipi_hash = pdb_tpi_hash_from_data(arena, strtbl, ipi, hash_data, aux_data);
    }
    if(lane_idx() == lane_from_task_idx(4)) ProfScope("parse IPI leaf")
    {
      String8 leaf_data = pdb_leaf_data_from_tpi(ipi);
      p2r2_shared->ipi_leaf = cv_leaf_from_data(arena, leaf_data, ipi->itype_first);
    }
    if(lane_idx() == lane_from_task_idx(5)) ProfScope("parse compilation units")
    {
      String8 comp_units_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo);
      p2r2_shared->comp_units = pdb_comp_unit_array_from_data(arena, comp_units_data);
    }
    if(lane_idx() == lane_from_task_idx(6)) ProfScope("parse compilation unit contributions")
    {
      String8 contribs_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_SecCon);
      p2r2_shared->comp_unit_contributions = pdb_comp_unit_contribution_array_from_data(arena, contribs_data, coff_sections);
    }
  }
  lane_sync();
  U64 exe_hash = p2r2_shared->exe_hash;
  PDB_TpiHashParsed *tpi_hash = p2r2_shared->tpi_hash;
  CV_LeafParsed *tpi_leaf = p2r2_shared->tpi_leaf;
  PDB_TpiHashParsed *ipi_hash = p2r2_shared->ipi_hash;
  CV_LeafParsed *ipi_leaf = p2r2_shared->ipi_leaf;
  PDB_CompUnitArray *comp_units = p2r2_shared->comp_units;
  PDB_CompUnitContributionArray *comp_unit_contributions = p2r2_shared->comp_unit_contributions;
  
  //////////////////////////////////////////////////////////////
  //- rjf: bucket compilation unit contributions
  //
  ProfScope("bucket compilation unit contributions") if(lane_idx() == 0)
  {
    p2r2_shared->unit_ranges = push_array(arena, RDIM_Rng1U64ChunkList, comp_units->count);
    for(U64 idx = 0; idx < comp_unit_contributions->count; idx += 1)
    {
      PDB_CompUnitContribution *contribution = &comp_unit_contributions->contributions[idx];
      if(contribution->mod < comp_units->count)
      {
        RDIM_Rng1U64 r = {contribution->voff_first, contribution->voff_opl};
        rdim_rng1u64_chunk_list_push(arena, &p2r2_shared->unit_ranges[contribution->mod], 256, r);
      }
    }
  }
  lane_sync();
  RDIM_Rng1U64ChunkList *unit_ranges = p2r2_shared->unit_ranges;
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse all syms & c13 line info streams
  //
  ProfScope("parse all syms & c13 line info streams")
  {
    //- rjf: setup outputs
    if(lane_idx() == 0)
    {
      p2r2_shared->all_syms_count = comp_units->count+1; // +1 for global symbol stream from DBI
      p2r2_shared->all_syms = push_array(arena, CV_SymParsed *, p2r2_shared->all_syms_count);
      p2r2_shared->all_c13s = push_array(arena, CV_C13Parsed *, p2r2_shared->all_syms_count);
      p2r2_shared->sym_c13_unit_lane_counter = 0;
    }
    lane_sync();
    
    //- rjf: wide fill
    {
      U64 task_count = p2r2_shared->all_syms_count;
      for(;;)
      {
        U64 task_num = ins_atomic_u64_inc_eval(&p2r2_shared->sym_c13_unit_lane_counter);
        if(task_num == 0 || task_count < task_num)
        {
          break;
        }
        U64 task_idx = task_num-1;
        if(task_idx > 0)
        {
          PDB_CompUnit *unit = comp_units->units[task_idx-1];
          String8 unit_sym_data = pdb_data_from_unit_range(msf, unit, PDB_DbiCompUnitRange_Symbols);
          String8 unit_c13_data = pdb_data_from_unit_range(msf, unit, PDB_DbiCompUnitRange_C13);
          p2r2_shared->all_syms[task_idx] = cv_sym_from_data(arena, unit_sym_data, 4);
          p2r2_shared->all_c13s[task_idx] = cv_c13_parsed_from_data(arena, unit_c13_data, raw_strtbl, coff_sections);
        }
        else
        {
          String8 global_sym_data = msf_data_from_stream(msf, dbi->sym_sn);
          p2r2_shared->all_syms[task_idx] = cv_sym_from_data(arena, global_sym_data, 4);
        }
      }
    }
  }
  lane_sync();
  U64 all_syms_count = p2r2_shared->all_syms_count;
  CV_SymParsed **all_syms = p2r2_shared->all_syms;
  CV_C13Parsed **all_c13s = p2r2_shared->all_c13s;
  
  //////////////////////////////////////////////////////////////
  //- rjf: calculate EXE's max voff
  //
  if(lane_idx() == 0)
  {
    COFF_SectionHeader *coff_sec_ptr = coff_sections.v;
    COFF_SectionHeader *coff_ptr_opl = coff_sec_ptr + coff_sections.count;
    for(;coff_sec_ptr < coff_ptr_opl; coff_sec_ptr += 1)
    {
      U64 sec_voff_max = coff_sec_ptr->voff + coff_sec_ptr->vsize;
      p2r2_shared->exe_voff_max = Max(p2r2_shared->exe_voff_max, sec_voff_max);
    }
  }
  lane_sync();
  U64 exe_voff_max = p2r2_shared->exe_voff_max;
  
  //////////////////////////////////////////////////////////////
  //- rjf: determine architecture
  //
  if(lane_idx() == 0)
  {
    //
    // TODO(rjf): in some cases, the first compilation unit has a zero
    // architecture, as it's sometimes used as a "nil" unit. this causes bugs
    // in later stages of conversion - particularly, this was detected via
    // busted location info. so i've converted this to a scan-until-we-find-an-
    // architecture. however, this may still be fundamentally insufficient,
    // because Nick has informed me that x86 units can be linked with x64
    // units, meaning the appropriate architecture at any point in time is not
    // a top-level concept, and is rather dependent on to which compilation
    // unit particular symbols belong. so in the future, to support that (odd)
    // case, we'll need to not only have this be a top-level "contextual" piece
    // of info, but to use the appropriate compilation unit's architecture when
    // possible. assuming, of course, that we care about supporting that case.
    //
    for EachIndex(idx, all_syms_count)
    {
      p2r2_shared->arch = p2r_rdi_arch_from_cv_arch(all_syms[idx]->info.arch);
      if(p2r2_shared->arch != RDI_Arch_NULL)
      {
        break;
      }
    }
  }
  lane_sync();
  RDI_Arch arch = RDI_Arch_NULL;
  U64 arch_addr_size = rdi_addr_size_from_arch(arch);
  
  //////////////////////////////////////////////////////////////
  //- rjf: predict total symbol count
  //
  if(lane_idx() == 0)
  {
    U64 rec_range_count = 0;
    for EachIndex(idx, all_syms_count)
    {
      rec_range_count += all_syms[idx]->sym_ranges.count;
    }
    p2r2_shared->symbol_count_prediction = rec_range_count/8;
    p2r2_shared->symbol_count_prediction = Max(p2r2_shared->symbol_count_prediction, 256);
  }
  lane_sync();
  U64 symbol_count_prediction = p2r2_shared->symbol_count_prediction;
  
  //////////////////////////////////////////////////////////////
  //- rjf: build link name map
  //
  ProfScope("build link name map") if(lane_idx() == 0 && all_syms_count != 0)
  {
    // rjf: set up
    {
      p2r2_shared->link_name_map.buckets_count = symbol_count_prediction;
      p2r2_shared->link_name_map.buckets = push_array(arena, P2R_LinkNameNode *, p2r2_shared->link_name_map.buckets_count);
    }
    
    // rjf: fill
    {
      CV_SymParsed *sym = all_syms[0];
      CV_RecRange *rec_ranges_first = sym->sym_ranges.ranges;
      CV_RecRange *rec_ranges_opl   = rec_ranges_first + sym->sym_ranges.count;
      for(CV_RecRange *rec_range = rec_ranges_first;
          rec_range < rec_ranges_opl;
          rec_range += 1)
      {
        //- rjf: unpack symbol range info
        CV_SymKind kind = rec_range->hdr.kind;
        U64 header_struct_size = cv_header_struct_size_from_sym_kind(kind);
        U8 *sym_first = sym->data.str + rec_range->off + 2;
        U8 *sym_opl   = sym_first + rec_range->hdr.size;
        
        //- rjf: skip bad ranges
        if(sym_opl > sym->data.str + sym->data.size || sym_first + header_struct_size > sym->data.str + sym->data.size)
        {
          continue;
        }
        
        //- rjf: consume symbol
        switch(kind)
        {
          default:{}break;
          case CV_SymKind_PUB32:
          {
            // rjf: unpack sym
            CV_SymPub32 *pub32 = (CV_SymPub32 *)sym_first;
            String8 name = str8_cstring_capped(pub32+1, sym_opl);
            COFF_SectionHeader *section = (0 < pub32->sec && pub32->sec <= coff_sections.count) ? &coff_sections.v[pub32->sec-1] : 0;
            U64 voff = 0;
            if(section != 0)
            {
              voff = section->voff + pub32->off;
            }
            
            // rjf: commit to link name map
            U64 hash = p2r_hash_from_voff(voff);
            U64 bucket_idx = hash%p2r2_shared->link_name_map.buckets_count;
            P2R_LinkNameNode *node = push_array(arena, P2R_LinkNameNode, 1);
            SLLStackPush(p2r2_shared->link_name_map.buckets[bucket_idx], node);
            node->voff = voff;
            node->name = name;
          }break;
        }
      }
    }
  }
  lane_sync();
  P2R_LinkNameMap link_name_map = p2r2_shared->link_name_map;
  
  //////////////////////////////////////////////////////////////
  //- rjf: organize subsets of unit symbol streams by lane
  //
  ProfScope("organize subsets of unit symbol streams by lane")
  {
    //- rjf: set up
    ProfScope("set up") if(lane_idx() == 0)
    {
      p2r2_shared->lane_sym_blocks = push_array(arena, P2R2_SymBlockList, lane_count());
      p2r2_shared->total_sym_record_count = 0;
      for EachIndex(sym_idx, p2r2_shared->all_syms_count)
      {
        p2r2_shared->total_sym_record_count += all_syms[sym_idx]->sym_ranges.count;
      }
    }
    lane_sync();
    
    //- rjf: gather
    ProfScope("gather")
    {
      Rng1U64 lane_sym_range = lane_range(p2r2_shared->total_sym_record_count);
      {
        U64 scan_sym_idx = 0;
        for EachIndex(idx, all_syms_count)
        {
          Rng1U64 stream_sym_range = r1u64(scan_sym_idx, scan_sym_idx + all_syms[idx]->sym_ranges.count);
          Rng1U64 sym_range_in_stream = intersect_1u64(stream_sym_range, lane_sym_range);
          if(sym_range_in_stream.max > sym_range_in_stream.min)
          {
            P2R2_SymBlock *block = push_array(arena, P2R2_SymBlock, 1);
            SLLQueuePush(p2r2_shared->lane_sym_blocks[lane_idx()].first, p2r2_shared->lane_sym_blocks[lane_idx()].last, block);
            if(idx > 0)
            {
              block->unit = comp_units->units[idx-1];
            }
            else
            {
              block->unit = &pdb_comp_unit_nil;
            }
            block->sym = all_syms[idx];
            block->c13 = all_c13s[idx];
            block->sym_rec_range = r1u64(sym_range_in_stream.min - scan_sym_idx, sym_range_in_stream.max - scan_sym_idx);
          }
          scan_sym_idx += all_syms[idx]->sym_ranges.count;
        }
      }
    }
  }
  lane_sync();
  P2R2_SymBlockList *lane_sym_blocks = p2r2_shared->lane_sym_blocks;
  
  //////////////////////////////////////////////////////////////
  //- rjf: gather all file paths
  //
  ProfScope("gather all file paths")
  {
    //- rjf: prep outputs
    ProfScope("prep outputs") if(lane_idx() == 0)
    {
      p2r2_shared->lane_inline_file_paths = push_array(arena, String8Array, lane_count());
      p2r2_shared->lane_line_file_paths = push_array(arena, String8Array, lane_count());
      p2r2_shared->lane_inline_file_paths_hashes = push_array(arena, U64Array, lane_count());
      p2r2_shared->lane_line_file_paths_hashes = push_array(arena, U64Array, lane_count());
    }
    lane_sync();
    
    //- rjf: do wide gather
    ProfScope("do wide gather")
    {
      Temp scratch = scratch_begin(&arena, 1);
      String8List inline_src_file_paths = {0};
      String8List line_src_file_paths = {0};
      
      //- rjf: build local hash table to dedup files within this lane
      U64 hit_path_slots_count = 4096;
      String8Node **hit_path_slots = push_array(scratch.arena, String8Node *, hit_path_slots_count);
      
      //- rjf: iterate lane blocks & gather inline site file names
      ProfScope("gather inline site file names from this lane's symbols")
        for(P2R2_SymBlock *lane_block = lane_sym_blocks[lane_idx()].first;
            lane_block != 0;
            lane_block = lane_block->next)
      {
        //- rjf: unpack unit
        PDB_CompUnit *unit = lane_block->unit;
        CV_SymParsed *sym = lane_block->sym;
        CV_C13Parsed *c13 = lane_block->c13;
        CV_RecRange *rec_ranges_first = sym->sym_ranges.ranges + lane_block->sym_rec_range.min;
        CV_RecRange *rec_ranges_opl   = sym->sym_ranges.ranges + lane_block->sym_rec_range.max;
        
        //- rjf: produce obj name/path
        String8 obj_name = unit->obj_name;
        {
          if(str8_match(obj_name, str8_lit("* Linker *"), 0) ||
             str8_match(obj_name, str8_lit("Import:"), StringMatchFlag_RightSideSloppy))
          {
            MemoryZeroStruct(&obj_name);
          }
        }
        String8 obj_folder_path = backslashed_from_str8(scratch.arena, str8_chop_last_slash(obj_name));
        
        //- rjf: find all inline site symbols & gather filenames
        U64 base_voff = 0;
        for(CV_RecRange *rec_range = rec_ranges_first;
            rec_range < rec_ranges_opl;
            rec_range += 1)
        {
          //- rjf: rec range -> symbol info range
          U64 sym_off_first = rec_range->off + 2;
          U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
          
          //- rjf: skip invalid ranges
          if(sym_off_opl > sym->data.size || sym_off_first > sym->data.size || sym_off_first > sym_off_opl)
          {
            continue;
          }
          
          //- rjf: unpack symbol info
          CV_SymKind kind = rec_range->hdr.kind;
          U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
          void *sym_header_struct_base = sym->data.str + sym_off_first;
          void *sym_data_opl = sym->data.str + sym_off_opl;
          
          //- rjf: skip bad sizes
          if(sym_off_first + sym_header_struct_size > sym_off_opl)
          {
            continue;
          }
          
          //- rjf: process symbol
          switch(kind)
          {
            default:{}break;
            
            //- rjf: LPROC32/GPROC32 (gather base address)
            case CV_SymKind_LPROC32:
            case CV_SymKind_GPROC32:
            {
              CV_SymProc32 *proc32 = (CV_SymProc32 *)sym_header_struct_base;
              COFF_SectionHeader *section = (0 < proc32->sec && proc32->sec <= coff_sections.count) ? &coff_sections.v[proc32->sec-1] : 0;
              if(section != 0)
              {
                base_voff = section->voff + proc32->off;
              }
            }break;
            
            //- rjf: INLINESITE
            case CV_SymKind_INLINESITE:
            {
              // rjf: unpack sym
              CV_SymInlineSite *sym           = (CV_SymInlineSite *)sym_header_struct_base;
              String8           binary_annots = str8((U8 *)(sym+1), rec_range->hdr.size - sizeof(rec_range->hdr.kind) - sizeof(*sym));
              
              // rjf: map inlinee -> parsed cv c13 inlinee line info
              CV_C13InlineeLinesParsed *inlinee_lines_parsed = 0;
              {
                U64 hash = cv_hash_from_item_id(sym->inlinee);
                U64 slot_idx = hash%c13->inlinee_lines_parsed_slots_count;
                for(CV_C13InlineeLinesParsedNode *n = c13->inlinee_lines_parsed_slots[slot_idx]; n != 0; n = n->hash_next)
                {
                  if(n->v.inlinee == sym->inlinee)
                  {
                    inlinee_lines_parsed = &n->v;
                    break;
                  }
                }
              }
              
              // rjf: build line table, fill with parsed binary annotations
              if(inlinee_lines_parsed != 0)
              {
                // rjf: grab checksums sub-section
                CV_C13SubSectionNode *file_chksms = c13->file_chksms_sub_section;
                
                // rjf: gathered lines
                U32 last_file_off = max_U32;
                U32 curr_file_off = max_U32;
                U64 line_count = 0;
                CV_C13InlineSiteDecoder decoder = cv_c13_inline_site_decoder_init(inlinee_lines_parsed->file_off, inlinee_lines_parsed->first_source_ln, base_voff);
                for(;;)
                {
                  // rjf: step & update
                  CV_C13InlineSiteDecoderStep step = cv_c13_inline_site_decoder_step(&decoder, binary_annots);
                  if(step.flags & CV_C13InlineSiteDecoderStepFlag_EmitFile)
                  {
                    last_file_off = curr_file_off;
                    curr_file_off = step.file_off;
                  }
                  if(step.flags == 0 && line_count > 0)
                  {
                    last_file_off = curr_file_off;
                    curr_file_off = max_U32;
                  }
                  
                  // rjf: file updated -> gather new file name
                  if(last_file_off != max_U32 && last_file_off != curr_file_off)
                  {
                    String8 seq_file_name = {0};
                    if(last_file_off + sizeof(CV_C13Checksum) <= file_chksms->size)
                    {
                      CV_C13Checksum *checksum = (CV_C13Checksum *)(c13->data.str + file_chksms->off + last_file_off);
                      U32             name_off = checksum->name_off;
                      seq_file_name = pdb_strtbl_string_from_off(strtbl, name_off);
                    }
                    
                    // rjf: file name -> normalized file path
                    String8 file_path            = seq_file_name;
                    String8 file_path_normalized = lower_from_str8(scratch.arena, str8_skip_chop_whitespace(file_path));
                    {
                      PathStyle file_path_normalized_style = path_style_from_str8(file_path_normalized);
                      String8List file_path_normalized_parts = str8_split_path(scratch.arena, file_path_normalized);
                      if(file_path_normalized_style == PathStyle_Relative)
                      {
                        String8List obj_folder_path_parts = str8_split_path(scratch.arena, obj_folder_path);
                        str8_list_concat_in_place(&obj_folder_path_parts, &file_path_normalized_parts);
                        file_path_normalized_parts = obj_folder_path_parts;
                        file_path_normalized_style = path_style_from_str8(obj_folder_path);
                      }
                      str8_path_list_resolve_dots_in_place(&file_path_normalized_parts, file_path_normalized_style);
                      file_path_normalized = str8_path_list_join_by_style(scratch.arena, &file_path_normalized_parts, file_path_normalized_style);
                    }
                    
                    // rjf: normalized file path -> source file node
                    U64 file_path_normalized_hash = rdi_hash(file_path_normalized.str, file_path_normalized.size);
                    U64 hit_path_slot = file_path_normalized_hash%hit_path_slots_count;
                    String8Node *hit_path_node = 0;
                    for(String8Node *n = hit_path_slots[hit_path_slot]; n != 0; n = n->next)
                    {
                      if(str8_match(n->string, file_path_normalized, 0))
                      {
                        hit_path_node = n;
                        break;
                      }
                    }
                    if(hit_path_node == 0)
                    {
                      hit_path_node = push_array(scratch.arena, String8Node, 1);
                      SLLStackPush(hit_path_slots[hit_path_slot], hit_path_node);
                      hit_path_node->string = file_path_normalized;
                      str8_list_push(arena, &inline_src_file_paths, push_str8_copy(arena, file_path_normalized));
                    }
                    line_count = 0;
                  }
                  
                  // rjf: count lines
                  if(step.flags & CV_C13InlineSiteDecoderStepFlag_EmitLine)
                  {
                    line_count += 1;
                  }
                  
                  // rjf: no more flags -> done
                  if(step.flags == 0)
                  {
                    break;
                  }
                }
              }
            }break;
          }
        }
      }
      
      //- rjf: do per-unit wide gather from unit line tables
      ProfScope("do per-unit wide gather from unit line tables")
      {
        // rjf: iterate all units for this lane
        Rng1U64 range = lane_range(comp_units->count);
        for EachInRange(idx, range)
        {
          PDB_CompUnit *unit = comp_units->units[idx];
          CV_SymParsed *unit_sym = all_syms[idx+1];
          CV_C13Parsed *unit_c13 = all_c13s[idx+1];
          CV_RecRange *rec_ranges_first = unit_sym->sym_ranges.ranges;
          CV_RecRange *rec_ranges_opl   = rec_ranges_first+unit_sym->sym_ranges.count;
          
          // rjf: produce obj name/path
          String8 obj_name = unit->obj_name;
          if(str8_match(obj_name, str8_lit("* Linker *"), 0) ||
             str8_match(obj_name, str8_lit("Import:"), StringMatchFlag_RightSideSloppy))
          {
            MemoryZeroStruct(&obj_name);
          }
          String8 obj_folder_path = backslashed_from_str8(scratch.arena, str8_chop_last_slash(obj_name));
          
          // rjf: find all files in this unit's (non-inline) line info
          ProfScope("find all files in this unit's (non-inline) line info")
            for(CV_C13SubSectionNode *node = unit_c13->first_sub_section;
                node != 0;
                node = node->next)
          {
            if(node->kind == CV_C13SubSectionKind_Lines)
            {
              for(CV_C13LinesParsedNode *lines_n = node->lines_first;
                  lines_n != 0;
                  lines_n = lines_n->next)
              {
                // rjf: file name -> sanitized file path
                String8 file_path = lines_n->v.file_name;
                String8 file_path_sanitized = str8_copy(scratch.arena, str8_skip_chop_whitespace(file_path));
                {
                  PathStyle file_path_sanitized_style = path_style_from_str8(file_path_sanitized);
                  String8List file_path_sanitized_parts = str8_split_path(scratch.arena, file_path_sanitized);
                  if(file_path_sanitized_style == PathStyle_Relative)
                  {
                    String8List obj_folder_path_parts = str8_split_path(scratch.arena, obj_folder_path);
                    str8_list_concat_in_place(&obj_folder_path_parts, &file_path_sanitized_parts);
                    file_path_sanitized_parts = obj_folder_path_parts;
                    file_path_sanitized_style = path_style_from_str8(obj_folder_path);
                  }
                  str8_path_list_resolve_dots_in_place(&file_path_sanitized_parts, file_path_sanitized_style);
                  file_path_sanitized = str8_path_list_join_by_style(scratch.arena, &file_path_sanitized_parts, file_path_sanitized_style);
                }
                
                // rjf: sanitized file path -> source file node
                U64 file_path_sanitized_hash = rdi_hash(file_path_sanitized.str, file_path_sanitized.size);
                U64 hit_path_slot = file_path_sanitized_hash%hit_path_slots_count;
                String8Node *hit_path_node = 0;
                for(String8Node *n = hit_path_slots[hit_path_slot]; n != 0; n = n->next)
                {
                  if(str8_match(n->string, file_path_sanitized, 0))
                  {
                    hit_path_node = n;
                    break;
                  }
                }
                if(hit_path_node == 0)
                {
                  hit_path_node = push_array(scratch.arena, String8Node, 1);
                  SLLStackPush(hit_path_slots[hit_path_slot], hit_path_node);
                  hit_path_node->string = file_path_sanitized;
                  str8_list_push(scratch.arena, &line_src_file_paths, push_str8_copy(arena, file_path_sanitized));
                }
              }
            }
          }
        }
      }
      
      //- rjf: merge into array for this lane
      p2r2_shared->lane_inline_file_paths[lane_idx()] = str8_array_from_list(arena, &inline_src_file_paths);
      p2r2_shared->lane_line_file_paths[lane_idx()] = str8_array_from_list(arena, &line_src_file_paths);
      
      //- rjf: hash this lane's file paths
      {
        struct
        {
          String8Array paths;
          U64Array *hashes_out;
        }
        tasks[] =
        {
          {p2r2_shared->lane_inline_file_paths[lane_idx()], &p2r2_shared->lane_inline_file_paths_hashes[lane_idx()]},
          {p2r2_shared->lane_line_file_paths[lane_idx()], &p2r2_shared->lane_line_file_paths_hashes[lane_idx()]},
        };
        for EachElement(task_idx, tasks)
        {
          U64Array hashes = {0};
          hashes.count = tasks[task_idx].paths.count;
          hashes.v = push_array(arena, U64, hashes.count);
          for EachIndex(idx, tasks[task_idx].paths.count)
          {
            hashes.v[idx] = rdi_hash(tasks[task_idx].paths.v[idx].str, tasks[task_idx].paths.v[idx].size);
          }
          tasks[task_idx].hashes_out[0] = hashes;
        }
      }
      
      scratch_end(scratch);
    }
  }
  lane_sync();
  String8Array *lane_inline_file_paths = p2r2_shared->lane_inline_file_paths;
  String8Array *lane_line_file_paths = p2r2_shared->lane_line_file_paths;
  U64Array *lane_inline_file_paths_hashes = p2r2_shared->lane_inline_file_paths_hashes;
  U64Array *lane_line_file_paths_hashes = p2r2_shared->lane_line_file_paths_hashes;
  
  //////////////////////////////////////////////////////////////
  //- rjf: build unified collection & map for source files
  //
  {
    //- rjf: set up table
    ProfScope("set up table") if(lane_idx() == 0)
    {
      p2r2_shared->total_path_count = 0;
      for EachIndex(idx, lane_count())
      {
        p2r2_shared->total_path_count += lane_line_file_paths[idx].count;
        p2r2_shared->total_path_count += lane_inline_file_paths[idx].count;
      }
      p2r2_shared->src_file_map.slots_count = p2r2_shared->total_path_count + p2r2_shared->total_path_count/2 + 1;
      p2r2_shared->src_file_map.slots = push_array(arena, P2R_SrcFileNode *, p2r2_shared->src_file_map.slots_count);
    }
    lane_sync();
    
    //- rjf: fill table
    ProfScope("fill table") if(lane_idx() == 0)
    {
      struct
      {
        String8Array *lane_paths;
        U64Array *lane_hashes;
      }
      tasks[] =
      {
        {lane_inline_file_paths, lane_inline_file_paths_hashes},
        {lane_line_file_paths, lane_line_file_paths_hashes},
      };
      for EachElement(task_idx, tasks)
      {
        for EachIndex(idx, lane_count())
        {
          String8Array paths = tasks[task_idx].lane_paths[idx];
          U64Array hashes = tasks[task_idx].lane_hashes[idx];
          for EachIndex(path_idx, paths.count)
          {
            String8 file_path_sanitized = paths.v[path_idx];
            U64 file_path_sanitized_hash = hashes.v[path_idx];
            U64 src_file_slot = file_path_sanitized_hash%p2r2_shared->src_file_map.slots_count;
            P2R_SrcFileNode *src_file_node = 0;
            for(P2R_SrcFileNode *n = p2r2_shared->src_file_map.slots[src_file_slot]; n != 0; n = n->next)
            {
              if(str8_match(n->src_file->path, file_path_sanitized, 0))
              {
                src_file_node = n;
                break;
              }
            }
            if(src_file_node == 0)
            {
              src_file_node = push_array(arena, P2R_SrcFileNode, 1);
              SLLStackPush(p2r2_shared->src_file_map.slots[src_file_slot], src_file_node);
              src_file_node->src_file = rdim_src_file_chunk_list_push(arena, &p2r2_shared->all_src_files__sequenceless, p2r2_shared->total_path_count);
              src_file_node->src_file->path = push_str8_copy(arena, file_path_sanitized);
            }
          }
        }
      }
    }
  }
  lane_sync();
  RDIM_SrcFileChunkList all_src_files__sequenceless = p2r2_shared->all_src_files__sequenceless;
  P2R_SrcFileMap src_file_map = p2r2_shared->src_file_map;
  
  //////////////////////////////////////////////////////////////
  //- rjf: for each lane, figure out info for starting a sub-unit range
  //
  ProfScope("for each lane, figure out info for starting a sub-unit range")
  {
    // rjf: set up
    if(lane_idx() == 0)
    {
      p2r2_shared->lane_unit_sub_start_pt_infos = push_array(arena, P2R2_UnitSubStartPtInfo, lane_count());
    }
    lane_sync();
    
    // rjf: fill
    {
      P2R2_UnitSubStartPtInfo *out_info = &p2r2_shared->lane_unit_sub_start_pt_infos[lane_idx()];
      P2R2_SymBlock *first_block = lane_sym_blocks[lane_idx()].first;
      if(first_block != 0 && first_block->sym_rec_range.min > 0)
      {
        CV_SymParsed *sym = first_block->sym;
        CV_RecRange *rec_ranges_first = sym->sym_ranges.ranges;
        CV_RecRange *rec_ranges_opl   = sym->sym_ranges.ranges + first_block->sym_rec_range.min;
        for(CV_RecRange *rec_range = rec_ranges_first;
            rec_range < rec_ranges_opl;
            rec_range += 1)
        {
          // rjf: rec range -> symbol info range
          U64 sym_off_first = rec_range->off + 2;
          U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
          
          // rjf: skip invalid ranges
          if(sym_off_opl > sym->data.size || sym_off_first > sym->data.size || sym_off_first > sym_off_opl)
          {
            continue;
          }
          
          // rjf: unpack symbol info
          CV_SymKind kind = rec_range->hdr.kind;
          U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
          void *sym_header_struct_base = sym->data.str + sym_off_first;
          void *sym_data_opl = sym->data.str + sym_off_opl;
          
          // rjf: skip bad sizes
          if(sym_off_first + sym_header_struct_size > sym_off_opl)
          {
            continue;
          }
          
          // rjf: find range starters
          switch(kind)
          {
            default:{}break;
            case CV_SymKind_LPROC32:
            case CV_SymKind_GPROC32:
            {
              CV_SymProc32 *proc32 = (CV_SymProc32 *)sym_header_struct_base;
              COFF_SectionHeader *section = (0 < proc32->sec && proc32->sec <= coff_sections.count) ? &coff_sections.v[proc32->sec-1] : 0;
              if(section != 0)
              {
                out_info->last_proc_voff = section->voff + proc32->off;
              }
            }break;
            case CV_SymKind_FRAMEPROC:
            {
              CV_SymFrameproc *frameproc = (CV_SymFrameproc*)sym_header_struct_base;
              out_info->last_frameproc = *frameproc;
            }break;
          }
        }
      }
    }
  }
  lane_sync();
  P2R2_UnitSubStartPtInfo *lane_unit_sub_start_pt_infos = p2r2_shared->lane_unit_sub_start_pt_infos;
  
  //////////////////////////////////////////////////////////////
  //- rjf: convert unit info
  //
  ProfScope("convert unit info")
  {
    //- rjf: set up outputs
    ProfScope("set up outputs") if(lane_idx() == 0)
    {
      for EachIndex(idx, comp_units->count)
      {
        rdim_unit_chunk_list_push(arena, &p2r2_shared->all_units, comp_units->count);
      }
      p2r2_shared->lanes_main_line_tables = push_array(arena, RDIM_LineTableChunkList, lane_count());
      p2r2_shared->lanes_inline_line_tables = push_array(arena, RDIM_LineTableChunkList, lane_count());
      p2r2_shared->lanes_first_inline_site_line_tables = push_array(arena, RDIM_LineTable *, lane_count());
    }
    lane_sync();
    RDIM_Unit *units = p2r2_shared->all_units.first->v;
    U64 units_count = p2r2_shared->all_units.first->count;
    RDIM_LineTableChunkList *lanes_main_line_tables = p2r2_shared->lanes_main_line_tables;
    RDIM_LineTableChunkList *lanes_inline_line_tables = p2r2_shared->lanes_inline_line_tables;
    Assert(units_count == comp_units->count);
    
    //- rjf: do per-lane work
    {
      RDIM_LineTableChunkList *dst_main_line_tables = &lanes_main_line_tables[lane_idx()];
      RDIM_LineTableChunkList *dst_inline_line_tables = &lanes_inline_line_tables[lane_idx()];
      
      //- rjf: per-unit line table conversion
      ProfScope("per-unit line table conversion")
      {
        Rng1U64 range = lane_range(units_count);
        for EachInRange(idx, range)
        {
          Temp scratch = scratch_begin(&arena, 1);
          PDB_CompUnit *src_unit     = comp_units->units[idx];
          CV_SymParsed *src_unit_sym = all_syms[idx+1];
          CV_C13Parsed *src_unit_c13 = all_c13s[idx+1];
          RDIM_Unit *dst_unit = &units[idx];
          
          // rjf: produce unit name
          String8 unit_name = src_unit->obj_name;
          if(unit_name.size != 0)
          {
            String8 unit_name_past_last_slash = str8_skip_last_slash(unit_name);
            if(unit_name_past_last_slash.size != 0)
            {
              unit_name = unit_name_past_last_slash;
            }
          }
          
          // rjf: produce obj name/path
          String8 obj_name = src_unit->obj_name;
          if(str8_match(obj_name, str8_lit("* Linker *"), 0) ||
             str8_match(obj_name, str8_lit("Import:"), StringMatchFlag_RightSideSloppy))
          {
            MemoryZeroStruct(&obj_name);
          }
          String8 obj_folder_path = backslashed_from_str8(scratch.arena, str8_chop_last_slash(obj_name));
          
          // rjf: build this unit's line table, fill out primary line info (inline info added after)
          RDIM_LineTable *line_table = 0;
          ProfScope("build unit line table")
            for(CV_C13SubSectionNode *node = src_unit_c13->first_sub_section;
                node != 0;
                node = node->next)
          {
            if(node->kind == CV_C13SubSectionKind_Lines)
            {
              for(CV_C13LinesParsedNode *lines_n = node->lines_first;
                  lines_n != 0;
                  lines_n = lines_n->next)
              {
                CV_C13LinesParsed *lines = &lines_n->v;
                
                // rjf: file name -> sanitized file path
                String8 file_path = lines->file_name;
                String8 file_path_sanitized = str8_copy(scratch.arena, str8_skip_chop_whitespace(file_path));
                {
                  PathStyle file_path_sanitized_style = path_style_from_str8(file_path_sanitized);
                  String8List file_path_sanitized_parts = str8_split_path(scratch.arena, file_path_sanitized);
                  if(file_path_sanitized_style == PathStyle_Relative)
                  {
                    String8List obj_folder_path_parts = str8_split_path(scratch.arena, obj_folder_path);
                    str8_list_concat_in_place(&obj_folder_path_parts, &file_path_sanitized_parts);
                    file_path_sanitized_parts = obj_folder_path_parts;
                    file_path_sanitized_style = path_style_from_str8(obj_folder_path);
                  }
                  str8_path_list_resolve_dots_in_place(&file_path_sanitized_parts, file_path_sanitized_style);
                  file_path_sanitized = str8_path_list_join_by_style(scratch.arena, &file_path_sanitized_parts, file_path_sanitized_style);
                }
                
                // rjf: sanitized file path -> source file node
                U64 file_path_sanitized_hash = rdi_hash(file_path_sanitized.str, file_path_sanitized.size);
                U64 src_file_slot = file_path_sanitized_hash%src_file_map.slots_count;
                P2R_SrcFileNode *src_file_node = 0;
                if(lines->line_count != 0)
                {
                  for(P2R_SrcFileNode *n = src_file_map.slots[src_file_slot]; n != 0; n = n->next)
                  {
                    if(str8_match(n->src_file->path, file_path_sanitized, 0))
                    {
                      src_file_node = n;
                      break;
                    }
                  }
                }
                
                // rjf: push sequence into both line table & source file's line map
                if(src_file_node != 0)
                {
                  if(line_table == 0)
                  {
                    line_table = rdim_line_table_chunk_list_push(arena, dst_main_line_tables, 256);
                  }
                  RDIM_LineSequence *seq = rdim_line_table_push_sequence(arena, dst_main_line_tables, line_table, src_file_node->src_file, lines->voffs, lines->line_nums, lines->col_nums, lines->line_count);
                }
              }
            }
          }
          
          // rjf: fill unit
          dst_unit->unit_name     = unit_name;
          dst_unit->compiler_name = src_unit_sym->info.compiler_name;
          dst_unit->object_file   = obj_name;
          dst_unit->archive_file  = src_unit->group_name;
          dst_unit->language      = p2r_rdi_language_from_cv_language(src_unit_sym->info.language);
          dst_unit->line_table    = line_table;
          dst_unit->voff_ranges   = unit_ranges[idx];
          
          scratch_end(scratch);
        }
      }
      
      //- rjf: build per-inline-site line tables
      ProfScope("build per-inline-site line tables")
      {
        for(P2R2_SymBlock *lane_block = lane_sym_blocks[lane_idx()].first;
            lane_block != 0;
            lane_block = lane_block->next)
        {
          Temp scratch = scratch_begin(&arena, 1);
          PDB_CompUnit *src_unit     = lane_block->unit;
          CV_SymParsed *src_unit_sym = lane_block->sym;
          CV_C13Parsed *src_unit_c13 = lane_block->c13;
          String8 obj_name = src_unit->obj_name;
          if(str8_match(obj_name, str8_lit("* Linker *"), 0) ||
             str8_match(obj_name, str8_lit("Import:"), StringMatchFlag_RightSideSloppy))
          {
            MemoryZeroStruct(&obj_name);
          }
          String8 obj_folder_path = backslashed_from_str8(scratch.arena, str8_chop_last_slash(obj_name));
          CV_RecRange *rec_ranges_first = src_unit_sym->sym_ranges.ranges + lane_block->sym_rec_range.min;
          CV_RecRange *rec_ranges_opl   = src_unit_sym->sym_ranges.ranges + lane_block->sym_rec_range.max;
          U64 base_voff = lane_unit_sub_start_pt_infos[lane_idx()].last_proc_voff;
          for(CV_RecRange *rec_range = rec_ranges_first;
              rec_range < rec_ranges_opl;
              rec_range += 1)
          {
            //- rjf: rec range -> symbol info range
            U64 sym_off_first = rec_range->off + 2;
            U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
            
            //- rjf: skip invalid ranges
            if(sym_off_opl > src_unit_sym->data.size || sym_off_first > src_unit_sym->data.size || sym_off_first > sym_off_opl)
            {
              continue;
            }
            
            //- rjf: unpack symbol info
            CV_SymKind kind = rec_range->hdr.kind;
            U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
            void *sym_header_struct_base = src_unit_sym->data.str + sym_off_first;
            void *sym_data_opl = src_unit_sym->data.str + sym_off_opl;
            
            //- rjf: skip bad sizes
            if(sym_off_first + sym_header_struct_size > sym_off_opl)
            {
              continue;
            }
            
            //- rjf: process symbol
            switch(kind)
            {
              default:{}break;
              
              //- rjf: LPROC32/GPROC32 (gather base address)
              case CV_SymKind_LPROC32:
              case CV_SymKind_GPROC32:
              {
                CV_SymProc32 *proc32 = (CV_SymProc32 *)sym_header_struct_base;
                COFF_SectionHeader *section = (0 < proc32->sec && proc32->sec <= coff_sections.count) ? &coff_sections.v[proc32->sec-1] : 0;
                if(section != 0)
                {
                  base_voff = section->voff + proc32->off;
                }
              }break;
              
              //- rjf: INLINESITE
              case CV_SymKind_INLINESITE:
              {
                // rjf: unpack sym
                CV_SymInlineSite *sym           = (CV_SymInlineSite *)sym_header_struct_base;
                String8           binary_annots = str8((U8 *)(sym+1), rec_range->hdr.size - sizeof(rec_range->hdr.kind) - sizeof(*sym));
                
                // rjf: map inlinee -> parsed cv c13 inlinee line info
                CV_C13InlineeLinesParsed *inlinee_lines_parsed = 0;
                {
                  U64 hash = cv_hash_from_item_id(sym->inlinee);
                  U64 slot_idx = hash%src_unit_c13->inlinee_lines_parsed_slots_count;
                  for(CV_C13InlineeLinesParsedNode *n = src_unit_c13->inlinee_lines_parsed_slots[slot_idx]; n != 0; n = n->hash_next)
                  {
                    if(n->v.inlinee == sym->inlinee)
                    {
                      inlinee_lines_parsed = &n->v;
                      break;
                    }
                  }
                }
                
                // rjf: build line table, fill with parsed binary annotations
                if(inlinee_lines_parsed != 0)
                {
                  // rjf: grab checksums sub-section
                  CV_C13SubSectionNode *file_chksms = src_unit_c13->file_chksms_sub_section;
                  
                  // rjf: gathered lines
                  typedef struct LineChunk LineChunk;
                  struct LineChunk
                  {
                    LineChunk *next;
                    U64        cap;
                    U64        count;
                    U64       *voffs;     // [line_count + 1] (sorted)
                    U32       *line_nums; // [line_count]
                    U16       *col_nums;  // [2*line_count]
                  };
                  LineChunk       *first_line_chunk            = 0;
                  LineChunk       *last_line_chunk             = 0;
                  U64              total_line_chunk_line_count = 0;
                  U32              last_file_off               = max_U32;
                  U32              curr_file_off               = max_U32;
                  RDIM_LineTable*  line_table                  = 0;
                  
                  CV_C13InlineSiteDecoder decoder = cv_c13_inline_site_decoder_init(inlinee_lines_parsed->file_off, inlinee_lines_parsed->first_source_ln, base_voff);
                  for(;;)
                  {
                    // rjf: step & update
                    CV_C13InlineSiteDecoderStep step = cv_c13_inline_site_decoder_step(&decoder, binary_annots);
                    if(step.flags & CV_C13InlineSiteDecoderStepFlag_EmitFile)
                    {
                      last_file_off = curr_file_off;
                      curr_file_off = step.file_off;
                    }
                    if(step.flags == 0 && total_line_chunk_line_count > 0)
                    {
                      last_file_off = curr_file_off;
                      curr_file_off = max_U32;
                    }
                    
                    // rjf: file updated -> push line chunks gathered for this file
                    if(last_file_off != max_U32 && last_file_off != curr_file_off)
                    {
                      String8 seq_file_name = {0};
                      if(last_file_off + sizeof(CV_C13Checksum) <= file_chksms->size)
                      {
                        CV_C13Checksum *checksum = (CV_C13Checksum*)(src_unit_c13->data.str + file_chksms->off + last_file_off);
                        U32             name_off = checksum->name_off;
                        seq_file_name = pdb_strtbl_string_from_off(strtbl, name_off);
                      }
                      
                      // rjf: file name -> sanitized file path
                      String8 file_path            = seq_file_name;
                      String8 file_path_sanitized  = str8_copy(scratch.arena, str8_skip_chop_whitespace(file_path));
                      {
                        PathStyle file_path_sanitized_style = path_style_from_str8(file_path_sanitized);
                        String8List file_path_sanitized_parts = str8_split_path(scratch.arena, file_path_sanitized);
                        if(file_path_sanitized_style == PathStyle_Relative)
                        {
                          String8List obj_folder_path_parts = str8_split_path(scratch.arena, obj_folder_path);
                          str8_list_concat_in_place(&obj_folder_path_parts, &file_path_sanitized_parts);
                          file_path_sanitized_parts = obj_folder_path_parts;
                          file_path_sanitized_style = path_style_from_str8(obj_folder_path);
                        }
                        str8_path_list_resolve_dots_in_place(&file_path_sanitized_parts, file_path_sanitized_style);
                        file_path_sanitized = str8_path_list_join_by_style(scratch.arena, &file_path_sanitized_parts, file_path_sanitized_style);
                      }
                      
                      // rjf: sanitized file path -> source file node
                      U64              file_path_sanitized_hash = rdi_hash(file_path_sanitized.str, file_path_sanitized.size);
                      U64              src_file_slot            = file_path_sanitized_hash%src_file_map.slots_count;
                      P2R_SrcFileNode *src_file_node            = 0;
                      for(P2R_SrcFileNode *n = src_file_map.slots[src_file_slot]; n != 0; n = n->next)
                      {
                        if(str8_match(n->src_file->path, file_path_sanitized, 0))
                        {
                          src_file_node = n;
                          break;
                        }
                      }
                      
                      // rjf: gather all lines
                      RDI_U64 *voffs      = 0;
                      RDI_U32 *line_nums  = 0;
                      RDI_U64  line_count = 0;
                      if(src_file_node != 0)
                      {
                        voffs = push_array_no_zero(arena, RDI_U64, total_line_chunk_line_count+1);
                        line_nums = push_array_no_zero(arena, RDI_U32, total_line_chunk_line_count);
                        line_count = total_line_chunk_line_count;
                        U64 dst_idx = 0;
                        for(LineChunk *chunk = first_line_chunk; chunk != 0; chunk = chunk->next)
                        {
                          MemoryCopy(voffs+dst_idx, chunk->voffs, sizeof(U64)*(chunk->count+1));
                          MemoryCopy(line_nums+dst_idx, chunk->line_nums, sizeof(U32)*chunk->count);
                          dst_idx += chunk->count;
                        }
                      }
                      
                      // rjf: push
                      if(line_count != 0)
                      {
                        if(line_table == 0)
                        {
                          line_table = rdim_line_table_chunk_list_push(arena, dst_inline_line_tables, 256);
                          if(p2r2_shared->lanes_first_inline_site_line_tables[lane_idx()] == 0)
                          {
                            p2r2_shared->lanes_first_inline_site_line_tables[lane_idx()] = line_table;
                          }
                        }
                        rdim_line_table_push_sequence(arena, dst_inline_line_tables, line_table, src_file_node->src_file, voffs, line_nums, 0, line_count);
                      }
                      
                      // rjf: clear line chunks for subsequent sequences
                      first_line_chunk            = last_line_chunk = 0;
                      total_line_chunk_line_count = 0;
                    }
                    
                    // rjf: new line -> emit to chunk
                    if(step.flags & CV_C13InlineSiteDecoderStepFlag_EmitLine)
                    {
                      LineChunk *chunk = last_line_chunk;
                      if(chunk == 0 || chunk->count+1 >= chunk->cap)
                      {
                        chunk = push_array(scratch.arena, LineChunk, 1);
                        SLLQueuePush(first_line_chunk, last_line_chunk, chunk);
                        chunk->cap       = 8;
                        chunk->voffs     = push_array_no_zero(scratch.arena, U64, chunk->cap);
                        chunk->line_nums = push_array_no_zero(scratch.arena, U32, chunk->cap);
                      }
                      chunk->voffs[chunk->count]     = step.line_voff;
                      chunk->voffs[chunk->count+1]   = step.line_voff_end;
                      chunk->line_nums[chunk->count] = step.ln;
                      chunk->count                  += 1;
                      total_line_chunk_line_count   += 1;
                    }
                    
                    // rjf: no more flags -> done
                    if(step.flags == 0)
                    {
                      break;
                    }
                  }
                }
              }break;
            }
          }
          scratch_end(scratch);
        }
      }
    }
  }
  lane_sync();
  RDIM_UnitChunkList all_units = p2r2_shared->all_units;
  RDIM_LineTableChunkList *lanes_main_line_tables = p2r2_shared->lanes_main_line_tables;
  RDIM_LineTableChunkList *lanes_inline_line_tables = p2r2_shared->lanes_inline_line_tables;
  RDIM_LineTable **lanes_first_inline_site_line_tables = p2r2_shared->lanes_first_inline_site_line_tables;
  
  //////////////////////////////////////////////////////////////
  //- rjf: join all line tables
  //
  ProfScope("join all line tables") if(lane_idx() == 0)
  {
    for EachIndex(idx, lane_count())
    {
      rdim_line_table_chunk_list_concat_in_place(&p2r2_shared->all_line_tables, &p2r2_shared->lanes_main_line_tables[idx]);
    }
    for EachIndex(idx, lane_count())
    {
      rdim_line_table_chunk_list_concat_in_place(&p2r2_shared->all_line_tables, &p2r2_shared->lanes_inline_line_tables[idx]);
    }
  }
  lane_sync();
  RDIM_LineTableChunkList all_line_tables = p2r2_shared->all_line_tables;
  
  //////////////////////////////////////////////////////////////
  //- rjf: equip source files with line sequences
  //
  ProfScope("equip source files with line sequences") if(lane_idx() == 0)
  {
    for(RDIM_LineTableChunkNode *line_table_chunk_n = all_line_tables.first;
        line_table_chunk_n != 0;
        line_table_chunk_n = line_table_chunk_n->next)
    {
      for EachIndex(chunk_line_table_idx, line_table_chunk_n->count)
      {
        RDIM_LineTable *line_table = &line_table_chunk_n->v[chunk_line_table_idx];
        for(RDIM_LineSequenceNode *s = line_table->first_seq; s != 0; s = s->next)
        {
          rdim_src_file_push_line_sequence(arena, &p2r2_shared->all_src_files__sequenceless, s->v.src_file, &s->v);
        }
      }
    }
  }
  lane_sync();
  RDIM_SrcFileChunkList all_src_files = p2r2_shared->all_src_files__sequenceless;
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 1: produce type forward resolution map
  //
  // this map is used to resolve usage of "incomplete structs" in codeview's
  // type info. this often happens when e.g. "struct Foo" is used to refer to
  // a later-defined "Foo", which actually contains members  and so on. we want
  // to hook types up to their actual destination complete types wherever
  // possible, and so this map can be used to do that in subsequent stages.
  //
  ProfScope("types pass 1: produce type forward resolution map")
  {
    //- rjf: allocate forward resolution map
    if(lane_idx() == 0)
    {
      p2r2_shared->itype_first = tpi_leaf->itype_first;
      p2r2_shared->itype_opl = tpi_leaf->itype_opl;
      p2r2_shared->itype_fwd_map = push_array(arena, CV_TypeId, (U64)p2r2_shared->itype_opl);
    }
    lane_sync();
    
    //- rjf: do wide fill
    {
      Rng1U64 range = lane_range(p2r2_shared->itype_opl);
      for EachInRange(idx, range)
      {
        CV_TypeId itype = (CV_TypeId)idx;
        if(itype < p2r2_shared->itype_first) { continue; }
        
        //- rjf: determine if this itype resolves to another
        CV_TypeId itype_fwd = 0;
        CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[itype-tpi_leaf->itype_first];
        CV_LeafKind kind = range->hdr.kind;
        U64 header_struct_size = cv_header_struct_size_from_leaf_kind(kind);
        if(range->off+range->hdr.size <= tpi_leaf->data.size &&
           range->off+2+header_struct_size <= tpi_leaf->data.size &&
           range->hdr.size >= 2)
        {
          U8 *itype_leaf_first = tpi_leaf->data.str + range->off+2;
          U8 *itype_leaf_opl   = itype_leaf_first + range->hdr.size-2;
          switch(kind)
          {
            default:{}break;
            
            //- rjf: CLASS/STRUCTURE
            case CV_LeafKind_CLASS:
            case CV_LeafKind_STRUCTURE:
            {
              // rjf: unpack leaf header
              CV_LeafStruct *lf_struct = (CV_LeafStruct *)itype_leaf_first;
              
              // rjf: has fwd ref flag -> lookup itype that this itype resolves to
              if(lf_struct->props & CV_TypeProp_FwdRef)
              {
                // rjf: unpack rest of leaf
                U8 *numeric_ptr = (U8 *)(lf_struct + 1);
                CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
                U8 *name_ptr = numeric_ptr + size.encoded_size;
                String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
                U8 *unique_name_ptr = name_ptr + name.size + 1;
                String8 unique_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
                
                // rjf: lookup
                B32 do_unique_name_lookup = (((lf_struct->props & CV_TypeProp_Scoped) != 0) &&
                                             ((lf_struct->props & CV_TypeProp_HasUniqueName) != 0));
                itype_fwd = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
              }
            }break;
            
            //- rjf: CLASS2/STRUCT2
            case CV_LeafKind_CLASS2:
            case CV_LeafKind_STRUCT2:
            {
              // rjf: unpack leaf header
              CV_LeafStruct2 *lf_struct = (CV_LeafStruct2 *)itype_leaf_first;
              
              // rjf: has fwd ref flag -> lookup itype that this itype resolves to
              if(lf_struct->props & CV_TypeProp_FwdRef)
              {
                // rjf: unpack rest of leaf
                U8 *numeric_ptr = (U8 *)(lf_struct + 1);
                CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
                U8 *name_ptr = (U8 *)numeric_ptr + size.encoded_size;
                String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
                U8 *unique_name_ptr = name_ptr + name.size + 1;
                String8 unique_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
                
                // rjf: lookup
                B32 do_unique_name_lookup = (((lf_struct->props & CV_TypeProp_Scoped) != 0) &&
                                             ((lf_struct->props & CV_TypeProp_HasUniqueName) != 0));
                itype_fwd = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
              }
            }break;
            
            //- rjf: UNION
            case CV_LeafKind_UNION:
            {
              // rjf: unpack leaf
              CV_LeafUnion *lf_union = (CV_LeafUnion *)itype_leaf_first;
              U8 *numeric_ptr = (U8 *)(lf_union + 1);
              CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
              U8 *name_ptr = numeric_ptr + size.encoded_size;
              String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
              U8 *unique_name_ptr = name_ptr + name.size + 1;
              String8 unique_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
              
              // rjf: has fwd ref flag -> lookup itype that this itype resolves tos
              if(lf_union->props & CV_TypeProp_FwdRef)
              {
                B32 do_unique_name_lookup = (((lf_union->props & CV_TypeProp_Scoped) != 0) &&
                                             ((lf_union->props & CV_TypeProp_HasUniqueName) != 0));
                itype_fwd = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
              }
            }break;
            
            //- rjf: ENUM
            case CV_LeafKind_ENUM:
            {
              // rjf: unpack leaf
              CV_LeafEnum *lf_enum = (CV_LeafEnum*)itype_leaf_first;
              U8 *name_ptr = (U8 *)(lf_enum + 1);
              String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
              U8 *unique_name_ptr = name_ptr + name.size + 1;
              String8 unique_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
              
              // rjf: has fwd ref flag -> lookup itype that this itype resolves to
              if(lf_enum->props & CV_TypeProp_FwdRef)
              {
                B32 do_unique_name_lookup = (((lf_enum->props & CV_TypeProp_Scoped) != 0) &&
                                             ((lf_enum->props & CV_TypeProp_HasUniqueName) != 0));
                itype_fwd = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
              }
            }break;
          }
        }
        
        //- rjf: if the forwarded itype is nonzero & in TPI range -> save to map
        if(itype_fwd != 0 && itype_fwd < tpi_leaf->itype_opl)
        {
          p2r2_shared->itype_fwd_map[itype] = itype_fwd;
        }
      }
    }
  }
  lane_sync();
  CV_TypeId *itype_fwd_map = p2r2_shared->itype_fwd_map;
  CV_TypeId itype_first = p2r2_shared->itype_first;
  CV_TypeId itype_opl = p2r2_shared->itype_opl;
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 2: produce per-itype itype chain
  //
  // this pass is to ensure that subsequent passes always produce types for
  // dependent itypes first - guaranteeing rdi's "only reference backward"
  // rule (which eliminates cycles). each itype slot gets a list of itypes,
  // starting with the deepest dependency - when types are produced per-itype,
  // this chain is walked, so that deeper dependencies are built first, and
  // as such, always show up *earlier* in the actually built types.
  //
  ProfScope("types pass 2: produce per-itype itype chain (for producing dependent types first)")
  {
    //- rjf: allocate itype chain table
    if(lane_idx() == 0)
    {
      p2r2_shared->itype_chains = push_array(arena, P2R_TypeIdChain *, (U64)p2r2_shared->itype_opl);
    }
    lane_sync();
    
    //- rjf: do wide fill
    {
      Rng1U64 range = lane_range(p2r2_shared->itype_opl);
      for EachInRange(idx, range)
      {
        CV_TypeId itype = (CV_TypeId)idx;
        if(itype < p2r2_shared->itype_first) { continue; }
        
        //- rjf: push initial itype - should be final-visited-itype for this itype
        {
          P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
          c->itype = itype;
          SLLStackPush(p2r2_shared->itype_chains[itype], c);
        }
        
        //- rjf: skip basic types for dependency walk
        if(itype < tpi_leaf->itype_first)
        {
          continue;
        }
        
        //- rjf: walk dependent types, push to chain
        Temp scratch = scratch_begin(&arena, 1);
        P2R_TypeIdChain start_walk_task = {0, itype};
        P2R_TypeIdChain *first_walk_task = &start_walk_task;
        P2R_TypeIdChain *last_walk_task = &start_walk_task;
        for(P2R_TypeIdChain *walk_task = first_walk_task;
            walk_task != 0;
            walk_task = walk_task->next)
        {
          CV_TypeId walk_itype = itype_fwd_map[walk_task->itype] ? itype_fwd_map[walk_task->itype] : walk_task->itype;
          if(walk_itype < tpi_leaf->itype_first)
          {
            continue;
          }
          CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[walk_itype-tpi_leaf->itype_first];
          CV_LeafKind kind = range->hdr.kind;
          U64 header_struct_size = cv_header_struct_size_from_leaf_kind(kind);
          if(range->off+range->hdr.size <= tpi_leaf->data.size &&
             range->off+2+header_struct_size <= tpi_leaf->data.size &&
             range->hdr.size >= 2)
          {
            U8 *itype_leaf_first = tpi_leaf->data.str + range->off+2;
            U8 *itype_leaf_opl   = itype_leaf_first + range->hdr.size-2;
            switch(kind)
            {
              default:{}break;
              
              //- rjf: MODIFIER
              case CV_LeafKind_MODIFIER:
              {
                CV_LeafModifier *lf = (CV_LeafModifier *)itype_leaf_first;
                
                // rjf: push dependent itype to chain
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = lf->itype;
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                
                // rjf: push task to walk dependency itype
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = lf->itype;
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
              }break;
              
              //- rjf: POINTER
              case CV_LeafKind_POINTER:
              {
                CV_LeafModifier *lf = (CV_LeafModifier *)itype_leaf_first;
                
                // rjf: push dependent itype to chain
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = lf->itype;
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                
                // rjf: push task to walk dependency itype
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = lf->itype;
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
              }break;
              
              //- rjf: PROCEDURE
              case CV_LeafKind_PROCEDURE:
              {
                CV_LeafProcedure *lf = (CV_LeafProcedure *)itype_leaf_first;
                
                // rjf: push return itypes to chain
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = lf->ret_itype;
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                
                // rjf: push task to walk return itype
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = lf->ret_itype;
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
                
                // rjf: unpack arglist range
                CV_RecRange *arglist_range = &tpi_leaf->leaf_ranges.ranges[lf->arg_itype-tpi_leaf->itype_first];
                if(arglist_range->hdr.kind != CV_LeafKind_ARGLIST ||
                   arglist_range->hdr.size<2 ||
                   arglist_range->off + arglist_range->hdr.size > tpi_leaf->data.size)
                {
                  break;
                }
                U8 *arglist_first = tpi_leaf->data.str + arglist_range->off + 2;
                U8 *arglist_opl   = arglist_first+arglist_range->hdr.size-2;
                if(arglist_first + sizeof(CV_LeafArgList) > arglist_opl)
                {
                  break;
                }
                
                // rjf: unpack arglist info
                CV_LeafArgList *arglist = (CV_LeafArgList*)arglist_first;
                CV_TypeId *arglist_itypes_base = (CV_TypeId *)(arglist+1);
                U32 arglist_itypes_count = arglist->count;
                
                // rjf: push arg types to chain
                for(U32 idx = 0; idx < arglist_itypes_count; idx += 1)
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = arglist_itypes_base[idx];
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                
                // rjf: push task to walk arg types
                for(U32 idx = 0; idx < arglist_itypes_count; idx += 1)
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = arglist_itypes_base[idx];
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
              }break;
              
              //- rjf: MFUNCTION
              case CV_LeafKind_MFUNCTION:
              {
                CV_LeafMFunction *lf = (CV_LeafMFunction *)itype_leaf_first;
                
                // rjf: push dependent itypes to chain
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = lf->ret_itype;
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = lf->arg_itype;
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = lf->this_itype;
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                
                // rjf: push task to walk dependency itypes
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = lf->ret_itype;
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = lf->arg_itype;
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = lf->this_itype;
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
                
                // rjf: unpack arglist range
                CV_RecRange *arglist_range = &tpi_leaf->leaf_ranges.ranges[lf->arg_itype-tpi_leaf->itype_first];
                if(arglist_range->hdr.kind != CV_LeafKind_ARGLIST ||
                   arglist_range->hdr.size<2 ||
                   arglist_range->off + arglist_range->hdr.size > tpi_leaf->data.size)
                {
                  break;
                }
                U8 *arglist_first = tpi_leaf->data.str + arglist_range->off + 2;
                U8 *arglist_opl   = arglist_first+arglist_range->hdr.size-2;
                if(arglist_first + sizeof(CV_LeafArgList) > arglist_opl)
                {
                  break;
                }
                
                // rjf: unpack arglist info
                CV_LeafArgList *arglist = (CV_LeafArgList*)arglist_first;
                CV_TypeId *arglist_itypes_base = (CV_TypeId *)(arglist+1);
                U32 arglist_itypes_count = arglist->count;
                
                // rjf: push arg types to chain
                for(U32 idx = 0; idx < arglist_itypes_count; idx += 1)
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = arglist_itypes_base[idx];
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                
                // rjf: push task to walk arg types
                for(U32 idx = 0; idx < arglist_itypes_count; idx += 1)
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = arglist_itypes_base[idx];
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
              }break;
              
              //- rjf: BITFIELD
              case CV_LeafKind_BITFIELD:
              {
                CV_LeafBitField *lf = (CV_LeafBitField *)itype_leaf_first;
                
                // rjf: push dependent itype to chain
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = lf->itype;
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                
                // rjf: push task to walk dependency itype
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = lf->itype;
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
              }break;
              
              //- rjf: ARRAY
              case CV_LeafKind_ARRAY:
              {
                CV_LeafArray *lf = (CV_LeafArray *)itype_leaf_first;
                
                // rjf: push dependent itypes to chain
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = lf->entry_itype;
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = lf->index_itype;
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                
                // rjf: push task to walk dependency itypes
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = lf->entry_itype;
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = lf->index_itype;
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
              }break;
              
              //- rjf: ENUM
              case CV_LeafKind_ENUM:
              {
                CV_LeafEnum *lf = (CV_LeafEnum *)itype_leaf_first;
                
                // rjf: push dependent itypes to chain
                {
                  P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                  c->itype = lf->base_itype;
                  SLLStackPush(p2r2_shared->itype_chains[itype], c);
                }
                
                // rjf: push task to walk dependency itypes
                {
                  P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                  c->itype = lf->base_itype;
                  SLLQueuePush(first_walk_task, last_walk_task, c);
                }
              }break;
            }
          }
        }
        scratch_end(scratch);
      }
    }
  }
  lane_sync();
  P2R_TypeIdChain **itype_chains = p2r2_shared->itype_chains;
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 3: construct all types from TPI
  //
  // this doesn't gather struct/class/union/enum members, which is done by
  // subsequent passes, to build RDI "UDT" information, which is distinct
  // from regular type info.
  //
  if(lane_idx() == 0) ProfScope("types pass 3: construct all root/stub types from TPI")
  {
#define p2r_builtin_type_ptr_from_kind(kind) ((basic_type_ptrs && RDI_TypeKind_FirstBuiltIn <= (kind) && (kind) <= RDI_TypeKind_LastBuiltIn) ? (basic_type_ptrs[(kind) - RDI_TypeKind_FirstBuiltIn]) : 0)
#define p2r_type_ptr_from_itype(itype) ((itype_type_ptrs && (itype) < itype_opl) ? (itype_type_ptrs[(itype_fwd_map[(itype)] ? itype_fwd_map[(itype)] : (itype))]) : 0)
    RDIM_Type **itype_type_ptrs = push_array(arena, RDIM_Type *, (U64)(itype_opl));
    RDIM_Type **basic_type_ptrs = push_array(arena, RDIM_Type *, (RDI_TypeKind_LastBuiltIn - RDI_TypeKind_FirstBuiltIn + 1));
    RDIM_TypeChunkList all_types = {0};
    
    ////////////////////////////
    //- rjf: build basic types
    //
    {
      for(RDI_TypeKind type_kind = RDI_TypeKind_FirstBuiltIn;
          type_kind <= RDI_TypeKind_LastBuiltIn;
          type_kind += 1)
      {
        RDIM_Type *type = rdim_type_chunk_list_push(arena, &all_types, 512);
        type->name.str  = rdi_string_from_type_kind(type_kind, &type->name.size);
        type->kind      = type_kind;
        type->byte_size = rdi_size_from_basic_type_kind(type_kind);
        basic_type_ptrs[type_kind - RDI_TypeKind_FirstBuiltIn] = type;
      }
    }
    
    ////////////////////////////
    //- rjf: build basic type aliases
    //
    {
      RDIM_DataModel data_model = rdim_data_model_from_os_arch(OperatingSystem_Windows, arch);
      RDI_TypeKind short_type      = rdim_short_type_kind_from_data_model(data_model);
      RDI_TypeKind ushort_type     = rdim_unsigned_short_type_kind_from_data_model(data_model);
      RDI_TypeKind long_type       = rdim_long_type_kind_from_data_model(data_model);
      RDI_TypeKind ulong_type      = rdim_unsigned_long_type_kind_from_data_model(data_model);
      RDI_TypeKind long_long_type  = rdim_long_long_type_kind_from_data_model(data_model);
      RDI_TypeKind ulong_long_type = rdim_unsigned_long_long_type_kind_from_data_model(data_model);
      RDI_TypeKind ptr_type        = rdim_pointer_size_t_type_kind_from_data_model(data_model);
      struct
      {
        char *       name;
        RDI_TypeKind kind_rdi;
        CV_LeafKind  kind_cv;
      }
      table[] =
      {
        { "signed char"          , RDI_TypeKind_Char8      , CV_BasicType_CHAR       },
        { "short"                , short_type              , CV_BasicType_SHORT      },
        { "long"                 , long_type               , CV_BasicType_LONG       },
        { "long long"            , long_long_type          , CV_BasicType_QUAD       },
        { "__int128"             , RDI_TypeKind_S128       , CV_BasicType_OCT        }, // Clang type
        { "unsigned char"        , RDI_TypeKind_UChar8     , CV_BasicType_UCHAR      },
        { "unsigned short"       , ushort_type             , CV_BasicType_USHORT     },
        { "unsigned long"        , ulong_type              , CV_BasicType_ULONG      },
        { "unsigned long long"   , ulong_long_type         , CV_BasicType_UQUAD      },
        { "__uint128"            , RDI_TypeKind_U128       , CV_BasicType_UOCT       }, // Clang type
        { "bool"                 , RDI_TypeKind_S8         , CV_BasicType_BOOL8      },
        { "__bool16"             , RDI_TypeKind_S16        , CV_BasicType_BOOL16     }, // not real C type
        { "__bool32"             , RDI_TypeKind_S32        , CV_BasicType_BOOL32     }, // not real C type
        { "float"                , RDI_TypeKind_F32        , CV_BasicType_FLOAT32    },
        { "double"               , RDI_TypeKind_F64        , CV_BasicType_FLOAT64    },
        { "long double"          , RDI_TypeKind_F80        , CV_BasicType_FLOAT80    },
        { "__float128"           , RDI_TypeKind_F128       , CV_BasicType_FLOAT128   }, // Clang type
        { "__float48"            , RDI_TypeKind_F48        , CV_BasicType_FLOAT48    }, // not real C type
        { "__float32pp"          , RDI_TypeKind_F32PP      , CV_BasicType_FLOAT32PP  }, // not real C type
        { "__float16"            , RDI_TypeKind_F16        , CV_BasicType_FLOAT16    },
        { "_Complex float"       , RDI_TypeKind_ComplexF32 , CV_BasicType_COMPLEX32  },
        { "_Complex double"      , RDI_TypeKind_ComplexF64 , CV_BasicType_COMPLEX64  },
        { "_Complex long double" , RDI_TypeKind_ComplexF80 , CV_BasicType_COMPLEX80  },
        { "_Complex __float128"  , RDI_TypeKind_ComplexF128, CV_BasicType_COMPLEX128 },
        { "__int8"               , RDI_TypeKind_S8         , CV_BasicType_INT8       },
        { "__uint8"              , RDI_TypeKind_U8         , CV_BasicType_UINT8      },
        { "__int16"              , RDI_TypeKind_S16        , CV_BasicType_INT16      },
        { "__uint16"             , RDI_TypeKind_U16        , CV_BasicType_UINT16     },
        { "int"                  , RDI_TypeKind_S32        , CV_BasicType_INT32      },
        { "int32"                , RDI_TypeKind_S32        , CV_BasicType_INT32      },
        { "uint32"               , RDI_TypeKind_U32        , CV_BasicType_UINT32     },
        { "__int64"              , RDI_TypeKind_S64        , CV_BasicType_INT64      },
        { "__uint64"             , RDI_TypeKind_U64        , CV_BasicType_UINT64     },
        { "__int128"             , RDI_TypeKind_S128       , CV_BasicType_INT128     },
        { "__uint128"            , RDI_TypeKind_U128       , CV_BasicType_UINT128    },
        { "char"                 , RDI_TypeKind_Char8      , CV_BasicType_RCHAR      }, // always ASCII
        { "wchar_t"              , RDI_TypeKind_UChar16    , CV_BasicType_WCHAR      }, // on windows always UTF-16
        { "char8_t"              , RDI_TypeKind_Char8      , CV_BasicType_CHAR8      }, // always UTF-8
        { "char16_t"             , RDI_TypeKind_Char16     , CV_BasicType_CHAR16     }, // always UTF-16
        { "char32_t"             , RDI_TypeKind_Char32     , CV_BasicType_CHAR32     }, // always UTF-32
        { "__pointer"            , ptr_type                , CV_BasicType_PTR        }
      };
      for EachElement(idx, table)
      {
        RDIM_Type *builtin_alias   = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
        builtin_alias->kind        = RDI_TypeKind_Alias;
        builtin_alias->name        = str8_cstring(table[idx].name);
        builtin_alias->direct_type = p2r_builtin_type_ptr_from_kind(table[idx].kind_rdi);
        builtin_alias->byte_size   = rdi_size_from_basic_type_kind(table[idx].kind_rdi);
        itype_type_ptrs[table[idx].kind_cv] = builtin_alias;
      }
      itype_type_ptrs[CV_BasicType_HRESULT] = basic_type_ptrs[RDI_TypeKind_HResult - RDI_TypeKind_FirstBuiltIn];
      itype_type_ptrs[CV_BasicType_VOID]    = basic_type_ptrs[RDI_TypeKind_Void - RDI_TypeKind_FirstBuiltIn];
    }
    
    ////////////////////////////
    //- rjf: build types from TPI
    //
    for(CV_TypeId root_itype = 0; root_itype < itype_opl; root_itype += 1)
    {
      for(P2R_TypeIdChain *itype_chain = itype_chains[root_itype];
          itype_chain != 0;
          itype_chain = itype_chain->next)
      {
        CV_TypeId itype = (root_itype != itype_chain->itype && itype_chain->itype < itype_opl && itype_fwd_map[itype_chain->itype]) ? itype_fwd_map[itype_chain->itype] : itype_chain->itype;
        B32 itype_is_basic = (itype < tpi->itype_first);
        
        //////////////////////////
        //- rjf: skip forward-reference itypes - all future resolutions will
        // reference whatever this itype resolves to, and so there is no point
        // in filling out this slot
        //
        if(itype_fwd_map[root_itype] != 0)
        {
          continue;
        }
        
        //////////////////////////
        //- rjf: skip already produced dependencies
        //
        if(itype_type_ptrs[itype] != 0)
        {
          continue;
        }
        
        //////////////////////////
        //- rjf: build basic type
        //
        if(itype_is_basic)
        {
          RDIM_Type *dst_type = 0;
          
          // rjf: unpack itype
          CV_BasicPointerKind cv_basic_ptr_kind  = CV_BasicPointerKindFromTypeId(itype);
          CV_BasicType        cv_basic_type_code = CV_BasicTypeFromTypeId(itype);
          
          // rjf: get basic type slot, fill if unfilled
          RDIM_Type *basic_type = itype_type_ptrs[cv_basic_type_code];
          if(basic_type == 0)
          {
            RDI_TypeKind type_kind = p2r_rdi_type_kind_from_cv_basic_type(cv_basic_type_code);
            U32 byte_size = rdi_size_from_basic_type_kind(type_kind);
            basic_type = dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
            if(byte_size == 0xffffffff)
            {
              byte_size = arch_addr_size;
            }
            basic_type->kind      = type_kind;
            basic_type->name      = cv_type_name_from_basic_type(cv_basic_type_code);
            basic_type->byte_size = byte_size;
          }
          
          // rjf: nonzero ptr kind -> form ptr type to basic tpye
          if(cv_basic_ptr_kind != 0)
          {
            dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
            dst_type->kind        = RDI_TypeKind_Ptr;
            dst_type->byte_size   = arch_addr_size;
            dst_type->direct_type = basic_type;
          }
          
          // rjf: fill this itype's slot with the finished type
          itype_type_ptrs[itype] = dst_type;
        }
        
        //////////////////////////
        //- rjf: build non-basic type
        //
        if(!itype_is_basic && itype >= itype_first)
        {
          RDIM_Type *dst_type = 0;
          CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[itype-itype_first];
          CV_LeafKind kind = range->hdr.kind;
          U64 header_struct_size = cv_header_struct_size_from_leaf_kind(kind);
          if(range->off+range->hdr.size <= tpi_leaf->data.size &&
             range->off+2+header_struct_size <= tpi_leaf->data.size &&
             range->hdr.size >= 2)
          {
            U8 *itype_leaf_first = tpi_leaf->data.str + range->off+2;
            U8 *itype_leaf_opl   = itype_leaf_first + range->hdr.size-2;
            switch(kind)
            {
              //- rjf: MODIFIER
              case CV_LeafKind_MODIFIER:
              {
                // rjf: unpack leaf
                CV_LeafModifier *lf = (CV_LeafModifier *)itype_leaf_first;
                
                // rjf: cv -> rdi flags
                RDI_TypeModifierFlags flags = 0;
                if(lf->flags & CV_ModifierFlag_Const)    {flags |= RDI_TypeModifierFlag_Const;}
                if(lf->flags & CV_ModifierFlag_Volatile) {flags |= RDI_TypeModifierFlag_Volatile;}
                
                // rjf: fill type
                if(flags == 0)
                {
                  dst_type = p2r_type_ptr_from_itype(lf->itype);
                }
                else
                {
                  dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                  dst_type->kind        = RDI_TypeKind_Modifier;
                  dst_type->flags       = flags;
                  dst_type->direct_type = p2r_type_ptr_from_itype(lf->itype);
                  dst_type->byte_size   = dst_type->direct_type ? dst_type->direct_type->byte_size : 0;
                }
              }break;
              
              //- rjf: POINTER
              case CV_LeafKind_POINTER:
              {
                // TODO(rjf): if ptr_mode in {PtrMem, PtrMethod} then output a member pointer instead
                
                // rjf: unpack leaf
                CV_LeafPointer *lf = (CV_LeafPointer *)itype_leaf_first;
                RDIM_Type *direct_type = p2r_type_ptr_from_itype(lf->itype);
                CV_PointerKind ptr_kind = CV_PointerAttribs_Extract_Kind(lf->attribs);
                CV_PointerMode ptr_mode = CV_PointerAttribs_Extract_Mode(lf->attribs);
                U32            ptr_size = CV_PointerAttribs_Extract_Size(lf->attribs);
                
                // rjf: cv -> rdi modifier flags
                RDI_TypeModifierFlags modifier_flags = 0;
                if(lf->attribs & CV_PointerAttrib_Const)      {modifier_flags |= RDI_TypeModifierFlag_Const;}
                if(lf->attribs & CV_PointerAttrib_Volatile)   {modifier_flags |= RDI_TypeModifierFlag_Volatile;}
                if(lf->attribs & CV_PointerAttrib_Restricted) {modifier_flags |= RDI_TypeModifierFlag_Restrict;}
                
                // rjf: cv info -> rdi pointer type kind
                RDI_TypeKind type_kind = RDI_TypeKind_Ptr;
                {
                  if(lf->attribs & CV_PointerAttrib_LRef)
                  {
                    type_kind = RDI_TypeKind_LRef;
                  }
                  else if(lf->attribs & CV_PointerAttrib_RRef)
                  {
                    type_kind = RDI_TypeKind_RRef;
                  }
                  if(ptr_mode == CV_PointerMode_LRef)
                  {
                    type_kind = RDI_TypeKind_LRef;
                  }
                  else if(ptr_mode == CV_PointerMode_RRef)
                  {
                    type_kind = RDI_TypeKind_RRef;
                  }
                }
                
                // rjf: fill type
                if(modifier_flags != 0)
                {
                  RDIM_Type *pointer_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                  dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                  dst_type->kind             = RDI_TypeKind_Modifier;
                  dst_type->flags            = modifier_flags;
                  dst_type->direct_type      = pointer_type;
                  dst_type->byte_size        = arch_addr_size;
                  pointer_type->kind         = type_kind;
                  pointer_type->byte_size    = arch_addr_size;
                  pointer_type->direct_type  = direct_type;
                }
                else
                {
                  dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                  dst_type->kind        = type_kind;
                  dst_type->byte_size   = arch_addr_size;
                  dst_type->direct_type = direct_type;
                }
              }break;
              
              //- rjf: PROCEDURE
              case CV_LeafKind_PROCEDURE:
              {
                // TODO(rjf): handle call_kind & attribs
                
                // rjf: unpack leaf
                CV_LeafProcedure *lf = (CV_LeafProcedure *)itype_leaf_first;
                RDIM_Type *ret_type = p2r_type_ptr_from_itype(lf->ret_itype);
                
                // rjf: fill type's basics
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                dst_type->kind        = RDI_TypeKind_Function;
                dst_type->byte_size   = arch_addr_size;
                dst_type->direct_type = ret_type;
                
                // rjf: unpack arglist range
                CV_RecRange *arglist_range = &tpi_leaf->leaf_ranges.ranges[lf->arg_itype-itype_first];
                if(arglist_range->hdr.kind != CV_LeafKind_ARGLIST ||
                   arglist_range->hdr.size<2 ||
                   arglist_range->off + arglist_range->hdr.size > tpi_leaf->data.size)
                {
                  break;
                }
                U8 *arglist_first = tpi_leaf->data.str + arglist_range->off + 2;
                U8 *arglist_opl   = arglist_first+arglist_range->hdr.size-2;
                if(arglist_first + sizeof(CV_LeafArgList) > arglist_opl)
                {
                  break;
                }
                
                // rjf: unpack arglist info
                CV_LeafArgList *arglist = (CV_LeafArgList*)arglist_first;
                CV_TypeId *arglist_itypes_base = (CV_TypeId *)(arglist+1);
                U32 arglist_itypes_count = arglist->count;
                
                // rjf: build param type array
                RDIM_Type **params = push_array(arena, RDIM_Type *, arglist_itypes_count);
                for(U32 idx = 0; idx < arglist_itypes_count; idx += 1)
                {
                  params[idx] = p2r_type_ptr_from_itype(arglist_itypes_base[idx]);
                }
                
                // rjf: fill dst type
                dst_type->count = arglist_itypes_count;
                dst_type->param_types = params;
              }break;
              
              //- rjf: MFUNCTION
              case CV_LeafKind_MFUNCTION:
              {
                // TODO(rjf): handle call_kind & attribs
                // TODO(rjf): preserve "this_adjust"
                
                // rjf: unpack leaf
                CV_LeafMFunction *lf = (CV_LeafMFunction *)itype_leaf_first;
                RDIM_Type *ret_type  = p2r_type_ptr_from_itype(lf->ret_itype);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                dst_type->kind        = (lf->this_itype != 0) ? RDI_TypeKind_Method : RDI_TypeKind_Function;
                dst_type->byte_size   = arch_addr_size;
                dst_type->direct_type = ret_type;
                
                // rjf: unpack arglist range
                CV_RecRange *arglist_range = &tpi_leaf->leaf_ranges.ranges[lf->arg_itype-itype_first];
                if(arglist_range->hdr.kind != CV_LeafKind_ARGLIST ||
                   arglist_range->hdr.size<2 ||
                   arglist_range->off + arglist_range->hdr.size > tpi_leaf->data.size)
                {
                  break;
                }
                U8 *arglist_first = tpi_leaf->data.str + arglist_range->off + 2;
                U8 *arglist_opl   = arglist_first+arglist_range->hdr.size-2;
                if(arglist_first + sizeof(CV_LeafArgList) > arglist_opl)
                {
                  break;
                }
                
                // rjf: unpack arglist info
                CV_LeafArgList *arglist = (CV_LeafArgList*)arglist_first;
                CV_TypeId *arglist_itypes_base = (CV_TypeId *)(arglist+1);
                U32 arglist_itypes_count = arglist->count;
                
                // rjf: build param type array
                U64 num_this_extras = 1;
                if(lf->this_itype == 0)
                {
                  num_this_extras = 0;
                }
                RDIM_Type **params = push_array(arena, RDIM_Type *, arglist_itypes_count+num_this_extras);
                for(U32 idx = 0; idx < arglist_itypes_count; idx += 1)
                {
                  params[idx+num_this_extras] = p2r_type_ptr_from_itype(arglist_itypes_base[idx]);
                }
                if(lf->this_itype != 0)
                {
                  params[0] = p2r_type_ptr_from_itype(lf->this_itype);
                }
                
                // rjf: fill dst type
                dst_type->count = arglist_itypes_count+num_this_extras;
                dst_type->param_types = params;
              }break;
              
              //- rjf: BITFIELD
              case CV_LeafKind_BITFIELD:
              {
                // rjf: unpack leaf
                CV_LeafBitField *lf = (CV_LeafBitField *)itype_leaf_first;
                RDIM_Type *direct_type = p2r_type_ptr_from_itype(lf->itype);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                dst_type->kind        = RDI_TypeKind_Bitfield;
                dst_type->off         = lf->pos;
                dst_type->count       = lf->len;
                dst_type->byte_size   = direct_type?direct_type->byte_size:0;
                dst_type->direct_type = direct_type;
              }break;
              
              //- rjf: ARRAY
              case CV_LeafKind_ARRAY:
              {
                // rjf: unpack leaf
                CV_LeafArray *lf = (CV_LeafArray *)itype_leaf_first;
                RDIM_Type *direct_type = p2r_type_ptr_from_itype(lf->entry_itype);
                U8 *numeric_ptr = (U8*)(lf + 1);
                CV_NumericParsed array_count = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
                U64 full_size = cv_u64_from_numeric(&array_count);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                dst_type->kind        = RDI_TypeKind_Array;
                dst_type->direct_type = direct_type;
                dst_type->byte_size   = full_size;
                dst_type->count       = (direct_type && direct_type->byte_size) ? (dst_type->byte_size/direct_type->byte_size) : 0;
              }break;
              
              //- rjf: CLASS/STRUCTURE
              case CV_LeafKind_CLASS:
              case CV_LeafKind_STRUCTURE:
              {
                // TODO(rjf): handle props
                
                // rjf: unpack leaf
                CV_LeafStruct *lf = (CV_LeafStruct *)itype_leaf_first;
                U8 *numeric_ptr = (U8*)(lf + 1);
                CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
                U64 size_u64 = cv_u64_from_numeric(&size);
                U8 *name_ptr = numeric_ptr + size.encoded_size;
                String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                if(lf->props & CV_TypeProp_FwdRef)
                {
                  dst_type->kind = (kind == CV_LeafKind_CLASS ? RDI_TypeKind_IncompleteClass : RDI_TypeKind_IncompleteStruct);
                  dst_type->name = name;
                }
                else
                {
                  dst_type->kind      = (kind == CV_LeafKind_CLASS ? RDI_TypeKind_Class : RDI_TypeKind_Struct);
                  dst_type->byte_size = (U32)size_u64;
                  dst_type->name      = name;
                }
              }break;
              
              //- rjf: CLASS2/STRUCT2
              case CV_LeafKind_CLASS2:
              case CV_LeafKind_STRUCT2:
              {
                // TODO(rjf): handle props
                
                // rjf: unpack leaf
                CV_LeafStruct2 *lf = (CV_LeafStruct2 *)itype_leaf_first;
                U8 *numeric_ptr = (U8*)(lf + 1);
                CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
                U64 size_u64 = cv_u64_from_numeric(&size);
                U8 *name_ptr = numeric_ptr + size.encoded_size;
                String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                if(lf->props & CV_TypeProp_FwdRef)
                {
                  dst_type->kind = (kind == CV_LeafKind_CLASS2 ? RDI_TypeKind_IncompleteClass : RDI_TypeKind_IncompleteStruct);
                  dst_type->name = name;
                }
                else
                {
                  dst_type->kind      = (kind == CV_LeafKind_CLASS2 ? RDI_TypeKind_Class : RDI_TypeKind_Struct);
                  dst_type->byte_size = (U32)size_u64;
                  dst_type->name      = name;
                }
              }break;
              
              //- rjf: UNION
              case CV_LeafKind_UNION:
              {
                // TODO(rjf): handle props
                
                // rjf: unpack leaf
                CV_LeafUnion *lf = (CV_LeafUnion *)itype_leaf_first;
                U8 *numeric_ptr = (U8*)(lf + 1);
                CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
                U64 size_u64 = cv_u64_from_numeric(&size);
                U8 *name_ptr = numeric_ptr + size.encoded_size;
                String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                if(lf->props & CV_TypeProp_FwdRef)
                {
                  dst_type->kind = RDI_TypeKind_IncompleteUnion;
                  dst_type->name = name;
                }
                else
                {
                  dst_type->kind      = RDI_TypeKind_Union;
                  dst_type->byte_size = (U32)size_u64;
                  dst_type->name      = name;
                }
              }break;
              
              //- rjf: ENUM
              case CV_LeafKind_ENUM:
              {
                // TODO(rjf): handle props
                
                // rjf: unpack leaf
                CV_LeafEnum *lf = (CV_LeafEnum *)itype_leaf_first;
                RDIM_Type *direct_type = p2r_type_ptr_from_itype(lf->base_itype);
                U8 *name_ptr = (U8 *)(lf + 1);
                String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                if(lf->props & CV_TypeProp_FwdRef)
                {
                  dst_type->kind = RDI_TypeKind_IncompleteEnum;
                  dst_type->name = name;
                }
                else
                {
                  dst_type->kind        = RDI_TypeKind_Enum;
                  dst_type->direct_type = direct_type;
                  dst_type->byte_size   = direct_type ? direct_type->byte_size : 0;
                  dst_type->name        = name;
                }
              }break;
            }
          }
          
          //- rjf: store finalized type to this itype's slot
          itype_type_ptrs[itype] = dst_type;
        }
      }
    }
    p2r2_shared->itype_type_ptrs = itype_type_ptrs;
    p2r2_shared->basic_type_ptrs = basic_type_ptrs;
    p2r2_shared->all_types__pre_typedefs = all_types;
#undef p2r_type_ptr_from_itype
#undef p2r_builtin_type_ptr_from_kind
  }
  lane_sync();
  RDIM_Type **itype_type_ptrs = p2r2_shared->itype_type_ptrs;
  RDIM_Type **basic_type_ptrs = p2r2_shared->basic_type_ptrs;
  RDIM_TypeChunkList all_types__pre_typedefs = p2r2_shared->all_types__pre_typedefs;
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 4: build UDTs
  //
  ProfScope("types pass 4: build UDTs")
  {
#define p2r_type_ptr_from_itype(itype) ((itype_type_ptrs && (itype) < tpi_leaf->itype_opl) ? (itype_type_ptrs[(itype_fwd_map[(itype)] ? itype_fwd_map[(itype)] : (itype))]) : 0)
    
    //- rjf: set up
    if(lane_idx() == 0)
    {
      p2r2_shared->lanes_udts = push_array(arena, RDIM_UDTChunkList, lane_count());
      p2r2_shared->lanes_members = push_array(arena, RDIM_UDTMemberChunkList, lane_count());
      p2r2_shared->lanes_enum_vals = push_array(arena, RDIM_UDTEnumValChunkList, lane_count());
    }
    lane_sync();
    
    //- rjf: do wide fill
    {
      U64 udts_chunk_cap = 4096;
      U64 members_chunk_cap = 4096;
      U64 enum_vals_chunk_cap = 4096;
      RDIM_UDTChunkList *udts = &p2r2_shared->lanes_udts[lane_idx()];
      RDIM_UDTMemberChunkList *members = &p2r2_shared->lanes_members[lane_idx()];
      RDIM_UDTEnumValChunkList *enum_vals = &p2r2_shared->lanes_enum_vals[lane_idx()];
      Rng1U64 range = lane_range(itype_opl);
      for EachInRange(idx, range)
      {
        //- rjf: skip basics
        CV_TypeId itype = (CV_TypeId)idx;
        if(itype < itype_first) { continue; }
        
        //- rjf: grab type for this itype - skip if empty
        RDIM_Type *dst_type = itype_type_ptrs[itype];
        if(dst_type == 0) { continue; }
        
        //- rjf: unpack itype leaf range - skip if out-of-range
        CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[itype-tpi_leaf->itype_first];
        CV_LeafKind kind = range->hdr.kind;
        U64 header_struct_size = cv_header_struct_size_from_leaf_kind(kind);
        U8 *itype_leaf_first = tpi_leaf->data.str + range->off+2;
        U8 *itype_leaf_opl   = itype_leaf_first + range->hdr.size-2;
        if(range->off+range->hdr.size > tpi_leaf->data.size ||
           range->off+2+header_struct_size > tpi_leaf->data.size ||
           range->hdr.size < 2)
        {
          continue;
        }
        
        //- rjf: build UDT
        CV_TypeId field_itype = 0;
        switch(kind)
        {
          default:{}break;
          
          ////////////////////////
          //- rjf: structs/unions/classes -> equip members
          //
          case CV_LeafKind_CLASS:
          case CV_LeafKind_STRUCTURE:
          {
            CV_LeafStruct *lf = (CV_LeafStruct *)itype_leaf_first;
            if(lf->props & CV_TypeProp_FwdRef)
            {
              break;
            }
            field_itype = lf->field_itype;
          }goto equip_members;
          case CV_LeafKind_UNION:
          {
            CV_LeafUnion *lf = (CV_LeafUnion *)itype_leaf_first;
            if(lf->props & CV_TypeProp_FwdRef)
            {
              break;
            }
            field_itype = lf->field_itype;
          }goto equip_members;
          case CV_LeafKind_CLASS2:
          case CV_LeafKind_STRUCT2:
          {
            CV_LeafStruct2 *lf = (CV_LeafStruct2 *)itype_leaf_first;
            if(lf->props & CV_TypeProp_FwdRef)
            {
              break;
            }
            field_itype = lf->field_itype;
          }goto equip_members;
          equip_members:
          {
            Temp scratch = scratch_begin(&arena, 1);
            
            //- rjf: grab UDT info
            RDIM_UDT *dst_udt = dst_type->udt;
            if(dst_udt == 0)
            {
              dst_udt = dst_type->udt = rdim_udt_chunk_list_push(arena, udts, udts_chunk_cap);
              dst_udt->self_type = dst_type;
            }
            
            //- rjf: gather all fields
            typedef struct FieldListTask FieldListTask;
            struct FieldListTask
            {
              FieldListTask *next;
              CV_TypeId itype;
            };
            FieldListTask start_fl_task = {0, field_itype};
            FieldListTask *fl_todo_stack = &start_fl_task;
            FieldListTask *fl_done_stack = 0;
            for(;fl_todo_stack != 0;)
            {
              //- rjf: take & unpack task
              FieldListTask *fl_task = fl_todo_stack;
              SLLStackPop(fl_todo_stack);
              SLLStackPush(fl_done_stack, fl_task);
              CV_TypeId field_list_itype = fl_task->itype;
              
              //- rjf: skip bad itypes
              if(field_list_itype < tpi_leaf->itype_first || tpi_leaf->itype_opl <= field_list_itype)
              {
                continue;
              }
              
              //- rjf: field list itype -> range
              CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[field_list_itype-tpi_leaf->itype_first];
              
              //- rjf: skip bad headers
              if(range->off+range->hdr.size > tpi_leaf->data.size ||
                 range->hdr.size < 2 ||
                 range->hdr.kind != CV_LeafKind_FIELDLIST)
              {
                continue;
              }
              
              //- rjf: loop over all fields
              {
                U8 *field_list_first = tpi_leaf->data.str+range->off+2;
                U8 *field_list_opl = field_list_first+range->hdr.size-2;
                for(U8 *read_ptr = field_list_first, *next_read_ptr = field_list_opl;
                    read_ptr < field_list_opl;
                    read_ptr = next_read_ptr)
                {
                  // rjf: unpack field
                  CV_LeafKind field_kind = *(CV_LeafKind *)read_ptr;
                  U64 field_leaf_header_size = cv_header_struct_size_from_leaf_kind(field_kind);
                  U8 *field_leaf_first = read_ptr+2;
                  U8 *field_leaf_opl   = field_list_opl;
                  next_read_ptr = field_leaf_opl;
                  
                  // rjf: skip out-of-bounds fields
                  if(field_leaf_first+field_leaf_header_size > field_list_opl)
                  {
                    continue;
                  }
                  
                  // rjf: process field
                  RDIM_UDTMember *new_member = 0;
                  switch(field_kind)
                  {
                    //- rjf: unhandled/invalid cases
                    default:
                    {
                      // TODO(rjf): log
                    }break;
                    
                    //- rjf: INDEX
                    case CV_LeafKind_INDEX:
                    {
                      // rjf: unpack leaf
                      CV_LeafIndex *lf = (CV_LeafIndex *)field_leaf_first;
                      CV_TypeId new_itype = lf->itype;
                      
                      // rjf: bump next read pointer past header
                      next_read_ptr = (U8 *)(lf+1);
                      
                      // rjf: determine if index itype is new
                      B32 is_new = 1;
                      for(FieldListTask *t = fl_done_stack; t != 0; t = t->next)
                      {
                        if(t->itype == new_itype)
                        {
                          is_new = 0;
                          break;
                        }
                      }
                      
                      // rjf: if new -> push task to follow new itype
                      if(is_new)
                      {
                        FieldListTask *new_task = push_array(scratch.arena, FieldListTask, 1);
                        SLLStackPush(fl_todo_stack, new_task);
                        new_task->itype = new_itype;
                      }
                    }break;
                    
                    //- rjf: MEMBER
                    case CV_LeafKind_MEMBER:
                    {
                      // TODO(rjf): log on bad offset
                      
                      // rjf: unpack leaf
                      CV_LeafMember *lf = (CV_LeafMember *)field_leaf_first;
                      U8 *offset_ptr = (U8 *)(lf+1);
                      CV_NumericParsed offset = cv_numeric_from_data_range(offset_ptr, field_leaf_opl);
                      U64 offset64 = cv_u64_from_numeric(&offset);
                      U8 *name_ptr = offset_ptr + offset.encoded_size;
                      String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                      
                      // rjf: bump next read pointer past variable length parts
                      next_read_ptr = name.str+name.size+1;
                      
                      // rjf: emit member
                      RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                      mem->kind = RDI_MemberKind_DataField;
                      mem->name = name;
                      mem->type = p2r_type_ptr_from_itype(lf->itype);
                      mem->off  = (U32)offset64;
                      new_member = mem;
                    }break;
                    
                    //- rjf: STMEMBER
                    case CV_LeafKind_STMEMBER:
                    {
                      // TODO(rjf): handle attribs
                      
                      // rjf: unpack leaf
                      CV_LeafStMember *lf = (CV_LeafStMember *)field_leaf_first;
                      U8 *name_ptr = (U8 *)(lf+1);
                      String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                      
                      // rjf: bump next read pointer past variable length parts
                      next_read_ptr = name.str+name.size+1;
                      
                      // rjf: emit member
                      RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                      mem->kind = RDI_MemberKind_StaticData;
                      mem->name = name;
                      mem->type = p2r_type_ptr_from_itype(lf->itype);
                      new_member = mem;
                    }break;
                    
                    //- rjf: METHOD
                    case CV_LeafKind_METHOD:
                    {
                      // rjf: unpack leaf
                      CV_LeafMethod *lf = (CV_LeafMethod *)field_leaf_first;
                      U8 *name_ptr = (U8 *)(lf+1);
                      String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                      
                      // rjf: bump next read pointer past variable length parts
                      next_read_ptr = name.str+name.size+1;
                      
                      //- rjf: method list itype -> range
                      CV_RecRange *method_list_range = &tpi_leaf->leaf_ranges.ranges[lf->list_itype-tpi_leaf->itype_first];
                      
                      //- rjf: skip bad method lists
                      if(method_list_range->off+method_list_range->hdr.size > tpi_leaf->data.size ||
                         method_list_range->hdr.size < 2 ||
                         method_list_range->hdr.kind != CV_LeafKind_METHODLIST)
                      {
                        break;
                      }
                      
                      //- rjf: loop through all methods & emit members
                      U8 *method_list_first = tpi_leaf->data.str + method_list_range->off + 2;
                      U8 *method_list_opl   = method_list_first + method_list_range->hdr.size-2;
                      for(U8 *method_read_ptr = method_list_first, *next_method_read_ptr = method_list_opl;
                          method_read_ptr < method_list_opl;
                          method_read_ptr = next_method_read_ptr)
                      {
                        CV_LeafMethodListMember *method = (CV_LeafMethodListMember*)method_read_ptr;
                        CV_MethodProp prop = CV_FieldAttribs_Extract_MethodProp(method->attribs);
                        RDIM_Type *method_type = p2r_type_ptr_from_itype(method->itype);
                        next_method_read_ptr = (U8 *)(method+1);
                        
                        // TODO(allen): PROBLEM
                        // We only get offsets for virtual functions (the "vbaseoff") from
                        // "Intro" and "PureIntro". In C++ inheritance, when we have a chain
                        // of inheritance (let's just talk single inheritance for now) the
                        // first class in the chain that introduces a new virtual function
                        // has this "Intro" method. If a later class in the chain redefines
                        // the virtual function it only has a "Virtual" method which does
                        // not update the offset. There is a "Virtual" and "PureVirtual"
                        // variant of "Virtual". The "Pure" in either case means there
                        // is no concrete procedure. When there is no "Pure" the method
                        // should have a corresponding procedure symbol id.
                        //
                        // The issue is we will want to mark all of our virtual methods as
                        // virtual and give them an offset, but that means we have to do
                        // some extra figuring to propogate offsets from "Intro" methods
                        // to "Virtual" methods in inheritance trees. That is - IF we want
                        // to start preserving the offsets of virtuals. There is room in
                        // the method struct to make this work, but for now I've just
                        // decided to drop this information. It is not urgently useful to
                        // us and greatly complicates matters.
                        
                        // rjf: read vbaseoff
                        U32 vbaseoff = 0;
                        if(prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro)
                        {
                          if(next_method_read_ptr+4 <= method_list_opl)
                          {
                            vbaseoff = *(U32 *)next_method_read_ptr;
                          }
                          next_method_read_ptr += 4;
                        }
                        
                        // rjf: emit method
                        switch(prop)
                        {
                          default:
                          {
                            RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                            mem->kind = RDI_MemberKind_Method;
                            mem->name = name;
                            mem->type = method_type;
                            new_member = mem;
                          }break;
                          case CV_MethodProp_Static:
                          {
                            RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                            mem->kind = RDI_MemberKind_StaticMethod;
                            mem->name = name;
                            mem->type = method_type;
                            new_member = mem;
                          }break;
                          case CV_MethodProp_Virtual:
                          case CV_MethodProp_PureVirtual:
                          case CV_MethodProp_Intro:
                          case CV_MethodProp_PureIntro:
                          {
                            RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                            mem->kind = RDI_MemberKind_VirtualMethod;
                            mem->name = name;
                            mem->type = method_type;
                            new_member = mem;
                          }break;
                        }
                      }
                      
                    }break;
                    
                    //- rjf: ONEMETHOD
                    case CV_LeafKind_ONEMETHOD:
                    {
                      // TODO(rjf): handle attribs
                      
                      // rjf: unpack leaf
                      CV_LeafOneMethod *lf = (CV_LeafOneMethod *)field_leaf_first;
                      CV_MethodProp prop = CV_FieldAttribs_Extract_MethodProp(lf->attribs);
                      U8 *vbaseoff_ptr = (U8 *)(lf+1);
                      U8 *vbaseoff_opl_ptr = vbaseoff_ptr;
                      U32 vbaseoff = 0;
                      if(prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro)
                      {
                        vbaseoff = *(U32 *)(vbaseoff_ptr);
                        vbaseoff_opl_ptr += sizeof(U32);
                      }
                      U8 *name_ptr = vbaseoff_opl_ptr;
                      String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                      RDIM_Type *method_type = p2r_type_ptr_from_itype(lf->itype);
                      
                      // rjf: bump next read pointer past variable length parts
                      next_read_ptr = name.str+name.size+1;
                      
                      // rjf: emit method
                      switch(prop)
                      {
                        default:
                        {
                          RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                          mem->kind = RDI_MemberKind_Method;
                          mem->name = name;
                          mem->type = method_type;
                          new_member = mem;
                        }break;
                        case CV_MethodProp_Static:
                        {
                          RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                          mem->kind = RDI_MemberKind_StaticMethod;
                          mem->name = name;
                          mem->type = method_type;
                          new_member = mem;
                        }break;
                        case CV_MethodProp_Virtual:
                        case CV_MethodProp_PureVirtual:
                        case CV_MethodProp_Intro:
                        case CV_MethodProp_PureIntro:
                        {
                          RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                          mem->kind = RDI_MemberKind_VirtualMethod;
                          mem->name = name;
                          mem->type = method_type;
                          new_member = mem;
                        }break;
                      }
                    }break;
                    
                    //- rjf: NESTTYPE
                    case CV_LeafKind_NESTTYPE:
                    {
                      // rjf: unpack leaf
                      CV_LeafNestType *lf = (CV_LeafNestType *)field_leaf_first;
                      U8 *name_ptr = (U8 *)(lf+1);
                      String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                      
                      // rjf: bump next read pointer past variable length parts
                      next_read_ptr = name.str+name.size+1;
                      
                      // rjf: emit member
                      RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                      mem->kind = RDI_MemberKind_NestedType;
                      mem->name = name;
                      mem->type = p2r_type_ptr_from_itype(lf->itype);
                      new_member = mem;
                    }break;
                    
                    //- rjf: NESTTYPEEX
                    case CV_LeafKind_NESTTYPEEX:
                    {
                      // TODO(rjf): handle attribs
                      
                      // rjf: unpack leaf
                      CV_LeafNestTypeEx *lf = (CV_LeafNestTypeEx *)field_leaf_first;
                      U8 *name_ptr = (U8 *)(lf+1);
                      String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                      
                      // rjf: bump next read pointer past variable length parts
                      next_read_ptr = name.str+name.size+1;
                      
                      // rjf: emit member
                      RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                      mem->kind = RDI_MemberKind_NestedType;
                      mem->name = name;
                      mem->type = p2r_type_ptr_from_itype(lf->itype);
                      new_member = mem;
                    }break;
                    
                    //- rjf: BCLASS
                    case CV_LeafKind_BCLASS:
                    {
                      // TODO(rjf): log on bad offset
                      
                      // rjf: unpack leaf
                      CV_LeafBClass *lf = (CV_LeafBClass *)field_leaf_first;
                      U8 *offset_ptr = (U8 *)(lf+1);
                      CV_NumericParsed offset = cv_numeric_from_data_range(offset_ptr, field_leaf_opl);
                      U64 offset64 = cv_u64_from_numeric(&offset);
                      
                      // rjf: bump next read pointer past variable length parts
                      next_read_ptr = offset_ptr+offset.encoded_size;
                      
                      // rjf: emit member
                      RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                      mem->kind = RDI_MemberKind_Base;
                      mem->type = p2r_type_ptr_from_itype(lf->itype);
                      mem->off  = (U32)offset64;
                      new_member = mem;
                    }break;
                    
                    //- rjf: VBCLASS/IVBCLASS
                    case CV_LeafKind_VBCLASS:
                    case CV_LeafKind_IVBCLASS:
                    {
                      // TODO(rjf): log on bad offsets
                      // TODO(rjf): handle attribs
                      // TODO(rjf): offsets?
                      
                      // rjf: unpack leaf
                      CV_LeafVBClass *lf = (CV_LeafVBClass *)field_leaf_first;
                      U8 *num1_ptr = (U8 *)(lf+1);
                      CV_NumericParsed num1 = cv_numeric_from_data_range(num1_ptr, field_leaf_opl);
                      U8 *num2_ptr = num1_ptr + num1.encoded_size;
                      CV_NumericParsed num2 = cv_numeric_from_data_range(num2_ptr, field_leaf_opl);
                      
                      // rjf: bump next read pointer past header
                      next_read_ptr = (U8 *)(lf+1);
                      
                      // rjf: emit member
                      RDIM_UDTMember *mem = rdim_udt_member_chunk_list_push(arena, members, members_chunk_cap);
                      mem->kind = RDI_MemberKind_VirtualBase;
                      mem->type = p2r_type_ptr_from_itype(lf->itype);
                      new_member = mem;
                    }break;
                    
                    //- rjf: VFUNCTAB
                    case CV_LeafKind_VFUNCTAB:
                    {
                      CV_LeafVFuncTab *lf = (CV_LeafVFuncTab *)field_leaf_first;
                      
                      // rjf: bump next read pointer past header
                      next_read_ptr = (U8 *)(lf+1);
                      
                      // NOTE(rjf): currently no-op this case
                      (void)lf;
                    }break;
                  }
                  
                  // rjf: add member to UDT
                  if(new_member != 0)
                  {
                    if(dst_udt->first_member == 0)
                    {
                      dst_udt->first_member = new_member;
                    }
                    dst_udt->member_count += 1;
                  }
                  
                  // rjf: align-up next field
                  next_read_ptr = (U8 *)AlignPow2((U64)next_read_ptr, 4);
                }
              }
            }
            
            scratch_end(scratch);
          }break;
          
          ////////////////////////
          //- rjf: enums -> equip enumerates
          //
          case CV_LeafKind_ENUM:
          {
            CV_LeafEnum *lf = (CV_LeafEnum *)itype_leaf_first;
            if(lf->props & CV_TypeProp_FwdRef)
            {
              break;
            }
            field_itype = lf->field_itype;
          }goto equip_enum_vals;
          equip_enum_vals:;
          {
            Temp scratch = scratch_begin(&arena, 1);
            
            //- rjf: grab UDT info
            RDIM_UDT *dst_udt = dst_type->udt;
            if(dst_udt == 0)
            {
              dst_udt = dst_type->udt = rdim_udt_chunk_list_push(arena, udts, udts_chunk_cap);
              dst_udt->self_type = dst_type;
            }
            
            //- rjf: gather all fields
            typedef struct FieldListTask FieldListTask;
            struct FieldListTask
            {
              FieldListTask *next;
              CV_TypeId itype;
            };
            FieldListTask start_fl_task = {0, field_itype};
            FieldListTask *fl_todo_stack = &start_fl_task;
            FieldListTask *fl_done_stack = 0;
            for(;fl_todo_stack != 0;)
            {
              //- rjf: take & unpack task
              FieldListTask *fl_task = fl_todo_stack;
              SLLStackPop(fl_todo_stack);
              SLLStackPush(fl_done_stack, fl_task);
              CV_TypeId field_list_itype = fl_task->itype;
              
              //- rjf: skip bad itypes
              if(field_list_itype < tpi_leaf->itype_first || tpi_leaf->itype_opl <= field_list_itype)
              {
                continue;
              }
              
              //- rjf: field list itype -> range
              CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[field_list_itype-tpi_leaf->itype_first];
              
              //- rjf: skip bad headers
              if(range->off+range->hdr.size > tpi_leaf->data.size ||
                 range->hdr.size < 2 ||
                 range->hdr.kind != CV_LeafKind_FIELDLIST)
              {
                continue;
              }
              
              //- rjf: loop over all fields
              {
                U8 *field_list_first = tpi_leaf->data.str+range->off+2;
                U8 *field_list_opl = field_list_first+range->hdr.size-2;
                for(U8 *read_ptr = field_list_first, *next_read_ptr = field_list_opl;
                    read_ptr < field_list_opl;
                    read_ptr = next_read_ptr)
                {
                  // rjf: unpack field
                  CV_LeafKind field_kind = *(CV_LeafKind *)read_ptr;
                  U64 field_leaf_header_size = cv_header_struct_size_from_leaf_kind(field_kind);
                  U8 *field_leaf_first = read_ptr+2;
                  U8 *field_leaf_opl   = field_leaf_first+range->hdr.size-2;
                  next_read_ptr = field_leaf_opl;
                  
                  // rjf: skip out-of-bounds fields
                  if(field_leaf_first+field_leaf_header_size > field_list_opl)
                  {
                    continue;
                  }
                  
                  // rjf: process field
                  RDIM_UDTEnumVal *new_enum_val = 0;
                  switch(field_kind)
                  {
                    //- rjf: unhandled/invalid cases
                    default:
                    {
                      // TODO(rjf): log
                    }break;
                    
                    //- rjf: INDEX
                    case CV_LeafKind_INDEX:
                    {
                      // rjf: unpack leaf
                      CV_LeafIndex *lf = (CV_LeafIndex *)field_leaf_first;
                      CV_TypeId new_itype = lf->itype;
                      
                      // rjf: determine if index itype is new
                      B32 is_new = 1;
                      for(FieldListTask *t = fl_done_stack; t != 0; t = t->next)
                      {
                        if(t->itype == new_itype)
                        {
                          is_new = 0;
                          break;
                        }
                      }
                      
                      // rjf: if new -> push task to follow new itype
                      if(is_new)
                      {
                        FieldListTask *new_task = push_array(scratch.arena, FieldListTask, 1);
                        SLLStackPush(fl_todo_stack, new_task);
                        new_task->itype = new_itype;
                      }
                    }break;
                    
                    //- rjf: ENUMERATE
                    case CV_LeafKind_ENUMERATE:
                    {
                      // TODO(rjf): attribs
                      
                      // rjf: unpack leaf
                      CV_LeafEnumerate *lf = (CV_LeafEnumerate *)field_leaf_first;
                      U8 *val_ptr = (U8 *)(lf+1);
                      CV_NumericParsed val = cv_numeric_from_data_range(val_ptr, field_leaf_opl);
                      U64 val64 = cv_u64_from_numeric(&val);
                      U8 *name_ptr = val_ptr + val.encoded_size;
                      String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                      
                      // rjf: bump next read pointer past variable length parts
                      next_read_ptr = name.str+name.size+1;
                      
                      // rjf: emit member
                      RDIM_UDTEnumVal *enum_val = rdim_udt_enum_val_chunk_list_push(arena, enum_vals, enum_vals_chunk_cap);
                      enum_val->name = name;
                      enum_val->val  = val64;
                      new_enum_val = enum_val;
                    }break;
                  }
                  
                  // rjf: push new enum val to udt
                  if(new_enum_val != 0)
                  {
                    if(dst_udt->first_enum_val == 0)
                    {
                      dst_udt->first_enum_val = new_enum_val;
                    }
                    dst_udt->enum_val_count += 1;
                  }
                  
                  // rjf: align-up next field
                  next_read_ptr = (U8 *)AlignPow2((U64)next_read_ptr, 4);
                }
              }
            }
            
            scratch_end(scratch);
          }break;
        }
      }
    }
#undef p2r_type_ptr_from_itype
  }
  lane_sync();
  RDIM_UDTChunkList *lanes_udts = p2r2_shared->lanes_udts;
  RDIM_UDTMemberChunkList *lanes_members = p2r2_shared->lanes_members;
  RDIM_UDTEnumValChunkList *lanes_enum_vals = p2r2_shared->lanes_enum_vals;
  
  //////////////////////////////////////////////////////////////
  //- rjf: join all UDTs
  //
  ProfScope("join all UDTs") if(lane_idx() == 0)
  {
    for EachIndex(idx, lane_count())
    {
      rdim_udt_chunk_list_concat_in_place(&p2r2_shared->all_udts, &lanes_udts[idx]);
      rdim_udt_member_chunk_list_concat_in_place(&p2r2_shared->all_members, &lanes_members[idx]);
      rdim_udt_enum_val_chunk_list_concat_in_place(&p2r2_shared->all_enum_vals, &lanes_enum_vals[idx]);
    }
  }
  lane_sync();
  RDIM_UDTChunkList all_udts = p2r2_shared->all_udts;
  RDIM_UDTMemberChunkList all_members = p2r2_shared->all_members;
  RDIM_UDTEnumValChunkList all_enum_vals = p2r2_shared->all_enum_vals;
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce symbols from all streams
  //
  ProfScope("produce symbols from all streams")
  {
#define p2r_type_ptr_from_itype(itype) ((itype_type_ptrs && (itype) < itype_opl) ? (itype_type_ptrs[(itype_fwd_map[(itype)] ? itype_fwd_map[(itype)] : (itype))]) : 0)
    
    ////////////////////////////
    //- rjf: set up
    //
    if(lane_idx() == 0)
    {
      p2r2_shared->lanes_locations        = push_array(arena, RDIM_LocationChunkList, lane_count());
      p2r2_shared->lanes_location_cases   = push_array(arena, RDIM_LocationCaseChunkList, lane_count());
      p2r2_shared->lanes_procedures       = push_array(arena, RDIM_SymbolChunkList, lane_count());
      p2r2_shared->lanes_global_variables = push_array(arena, RDIM_SymbolChunkList, lane_count());
      p2r2_shared->lanes_thread_variables = push_array(arena, RDIM_SymbolChunkList, lane_count());
      p2r2_shared->lanes_constants        = push_array(arena, RDIM_SymbolChunkList, lane_count());
      p2r2_shared->lanes_scopes           = push_array(arena, RDIM_ScopeChunkList, lane_count());
      p2r2_shared->lanes_inline_sites     = push_array(arena, RDIM_InlineSiteChunkList, lane_count());
      p2r2_shared->lanes_typedefs         = push_array(arena, RDIM_TypeChunkList, lane_count());
    }
    lane_sync();
    
    ////////////////////////////
    //- rjf: set up outputs for this sym stream
    //
    U64 sym_locations_chunk_cap = 16384;
    U64 sym_location_cases_chunk_cap = 16384;
    U64 sym_procedures_chunk_cap = 16384;
    U64 sym_global_variables_chunk_cap = 16384;
    U64 sym_thread_variables_chunk_cap = 16384;
    U64 sym_constants_chunk_cap = 16384;
    U64 sym_scopes_chunk_cap = 16384;
    U64 sym_inline_sites_chunk_cap = 16384;
    RDIM_LocationChunkList sym_locations = {0};
    RDIM_LocationCaseChunkList sym_location_cases = {0};
    RDIM_SymbolChunkList sym_procedures = {0};
    RDIM_SymbolChunkList sym_global_variables = {0};
    RDIM_SymbolChunkList sym_thread_variables = {0};
    RDIM_SymbolChunkList sym_constants = {0};
    RDIM_ScopeChunkList sym_scopes = {0};
    RDIM_InlineSiteChunkList sym_inline_sites = {0};
    RDIM_TypeChunkList typedefs = {0};
    
    ////////////////////////////
    //- rjf: fill outputs for all unit sym blocks in this lane
    //
    for(P2R2_SymBlock *lane_block = lane_sym_blocks[lane_idx()].first;
        lane_block != 0;
        lane_block = lane_block->next)
    {
      Temp scratch = scratch_begin(&arena, 1);
      CV_SymParsed *sym = lane_block->sym;
      Rng1U64 sym_rec_range = lane_block->sym_rec_range;
      
      //////////////////////////
      //- rjf: symbols pass 1: produce procedure frame info map (procedure -> frame info)
      //
      U64 procedure_frameprocs_count = 0;
      U64 procedure_frameprocs_cap   = dim_1u64(sym_rec_range);
      CV_SymFrameproc **procedure_frameprocs = push_array_no_zero(scratch.arena, CV_SymFrameproc *, procedure_frameprocs_cap);
      ProfScope("symbols pass 1: produce procedure frame info map (procedure -> frame info)")
      {
        U64 procedure_num = 0;
        CV_RecRange *rec_ranges_first = sym->sym_ranges.ranges + sym_rec_range.min;
        CV_RecRange *rec_ranges_opl   = sym->sym_ranges.ranges + sym_rec_range.max;
        for(CV_RecRange *rec_range = rec_ranges_first;
            rec_range < rec_ranges_opl;
            rec_range += 1)
        {
          //- rjf: rec range -> symbol info range
          U64 sym_off_first = rec_range->off + 2;
          U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
          
          //- rjf: skip invalid ranges
          if(sym_off_opl > sym->data.size || sym_off_first > sym->data.size || sym_off_first > sym_off_opl)
          {
            continue;
          }
          
          //- rjf: unpack symbol info
          CV_SymKind kind = rec_range->hdr.kind;
          U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
          void *sym_header_struct_base = sym->data.str + sym_off_first;
          
          //- rjf: skip bad sizes
          if(sym_off_first + sym_header_struct_size > sym_off_opl)
          {
            continue;
          }
          
          //- rjf: consume symbol based on kind
          switch(kind)
          {
            default:{}break;
            
            //- rjf: FRAMEPROC
            case CV_SymKind_FRAMEPROC:
            {
              if(procedure_num == 0) { break; }
              if(procedure_num > procedure_frameprocs_cap) { break; }
              CV_SymFrameproc *frameproc = (CV_SymFrameproc*)sym_header_struct_base;
              procedure_frameprocs[procedure_num-1] = frameproc;
              procedure_frameprocs_count = Max(procedure_frameprocs_count, procedure_num);
            }break;
            
            //- rjf: LPROC32/GPROC32
            case CV_SymKind_LPROC32:
            case CV_SymKind_GPROC32:
            {
              procedure_num += 1;
            }break;
          }
        }
        U64 scratch_overkill = sizeof(procedure_frameprocs[0])*(procedure_frameprocs_cap-procedure_frameprocs_count);
        arena_pop(scratch.arena, scratch_overkill);
      }
      
      //////////////////////////
      //- rjf: symbols pass 2: construct all symbols, given procedure frame info map
      //
      ProfScope("symbols pass 2: construct all symbols, given procedure frame info map")
      {
        RDIM_LocationSet *defrange_target = 0;
        B32 defrange_target_is_param = 0;
        U64 procedure_num = 0;
        U64 procedure_base_voff = 0;
        CV_RecRange *rec_ranges_first = sym->sym_ranges.ranges + sym_rec_range.min;
        CV_RecRange *rec_ranges_opl   = sym->sym_ranges.ranges + sym_rec_range.max;
        typedef struct P2R_ScopeNode P2R_ScopeNode;
        struct P2R_ScopeNode
        {
          P2R_ScopeNode *next;
          RDIM_Scope *scope;
        };
        P2R_ScopeNode *top_scope_node = 0;
        P2R_ScopeNode *free_scope_node = 0;
        RDIM_LineTable *inline_site_line_table = lanes_first_inline_site_line_tables[lane_idx()];
        for(CV_RecRange *rec_range = rec_ranges_first;
            rec_range < rec_ranges_opl;
            rec_range += 1)
        {
          //- rjf: rec range -> symbol info range
          U64 sym_off_first = rec_range->off + 2;
          U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
          
          //- rjf: skip invalid ranges
          if(sym_off_opl > sym->data.size || sym_off_first > sym->data.size || sym_off_first > sym_off_opl)
          {
            continue;
          }
          
          //- rjf: unpack symbol info
          CV_SymKind kind = rec_range->hdr.kind;
          U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
          void *sym_header_struct_base = sym->data.str + sym_off_first;
          void *sym_data_opl = sym->data.str + sym_off_opl;
          
          //- rjf: skip bad sizes
          if(sym_off_first + sym_header_struct_size > sym_off_opl)
          {
            continue;
          }
          
          //- rjf: consume symbol based on kind
          switch(kind)
          {
            default:{}break;
            
            //- rjf: END
            case CV_SymKind_END:
            {
              P2R_ScopeNode *n = top_scope_node;
              if(n != 0)
              {
                SLLStackPop(top_scope_node);
                SLLStackPush(free_scope_node, n);
              }
              defrange_target = 0;
              defrange_target_is_param = 0;
            }break;
            
            //- rjf: BLOCK32
            case CV_SymKind_BLOCK32:
            {
              // rjf: unpack sym
              CV_SymBlock32 *block32 = (CV_SymBlock32 *)sym_header_struct_base;
              
              // rjf: build scope, insert into current parent scope
              RDIM_Scope *scope = rdim_scope_chunk_list_push(arena, &sym_scopes, sym_scopes_chunk_cap);
              {
                if(top_scope_node == 0)
                {
                  // TODO(rjf): log
                }
                if(top_scope_node != 0)
                {
                  RDIM_Scope *top_scope = top_scope_node->scope;
                  SLLQueuePush_N(top_scope->first_child, top_scope->last_child, scope, next_sibling);
                  scope->parent_scope = top_scope;
                  scope->symbol = top_scope->symbol;
                }
                COFF_SectionHeader *section = (0 < block32->sec && block32->sec <= coff_sections.count) ? &coff_sections.v[block32->sec-1] : 0;
                if(section != 0)
                {
                  U64 voff_first = section->voff + block32->off;
                  U64 voff_last = voff_first + block32->len;
                  RDIM_Rng1U64 voff_range = {voff_first, voff_last};
                  rdim_scope_push_voff_range(arena, &sym_scopes, scope, voff_range);
                }
              }
              
              // rjf: push this scope to scope stack
              {
                P2R_ScopeNode *node = free_scope_node;
                if(node != 0) { SLLStackPop(free_scope_node); }
                else { node = push_array_no_zero(scratch.arena, P2R_ScopeNode, 1); }
                node->scope = scope;
                SLLStackPush(top_scope_node, node);
              }
            }break;
            
            //- rjf: LDATA32/GDATA32
            case CV_SymKind_LDATA32:
            case CV_SymKind_GDATA32:
            {
              // rjf: unpack sym
              CV_SymData32 *data32 = (CV_SymData32 *)sym_header_struct_base;
              String8 name = str8_cstring_capped(data32+1, sym_data_opl);
              COFF_SectionHeader *section = (0 < data32->sec && data32->sec <= coff_sections.count) ? &coff_sections.v[data32->sec-1] : 0;
              U64 voff = (section ? section->voff : 0) + data32->off;
              
              // rjf: determine if this is an exact duplicate global
              //
              // PDB likes to have duplicates of these spread across different
              // symbol streams so we deduplicate across the entire translation
              // context.
              //
              B32 is_duplicate = 0;
              {
                // TODO(rjf): @important global symbol dedup
              }
              
              // rjf: is not duplicate -> push new global
              if(!is_duplicate)
              {
                // rjf: unpack global variable's type
                RDIM_Type *type = p2r_type_ptr_from_itype(data32->itype);
                
                // rjf: unpack global's container type
                RDIM_Type *container_type = 0;
                U64 container_name_opl = p2r_end_of_cplusplus_container_name(name);
                if(container_name_opl > 2)
                {
                  String8 container_name = str8(name.str, container_name_opl - 2);
                  CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, container_name, 0);
                  container_type = p2r_type_ptr_from_itype(cv_type_id);
                }
                
                // rjf: unpack global's container symbol
                RDIM_Symbol *container_symbol = 0;
                if(container_type == 0 && top_scope_node != 0)
                {
                  container_symbol = top_scope_node->scope->symbol;
                }
                
                // rjf: build symbol
                RDIM_Symbol *symbol = rdim_symbol_chunk_list_push(arena, &sym_global_variables, sym_global_variables_chunk_cap);
                symbol->is_extern        = (kind == CV_SymKind_GDATA32);
                symbol->name             = name;
                symbol->type             = type;
                symbol->offset           = voff;
                symbol->container_symbol = container_symbol;
                symbol->container_type   = container_type;
              }
            }break;
            
            //- rjf: UDT (typedefs)
            case CV_SymKind_UDT:
            if(sym == all_syms[0] && top_scope_node == 0)
            {
              CV_SymUDT *udt = (CV_SymUDT *)sym_header_struct_base;
              String8 name = str8_cstring_capped(udt+1, sym_data_opl);
              RDIM_Type *type   = rdim_type_chunk_list_push(arena, &typedefs, 4096);
              type->kind        = RDI_TypeKind_Alias;
              type->name        = name;
              type->direct_type = p2r_type_ptr_from_itype(udt->itype);
              if(type->direct_type != 0)
              {
                type->byte_size = type->direct_type->byte_size;
              }
            }break;
            
            //- rjf: LPROC32/GPROC32
            case CV_SymKind_LPROC32:
            case CV_SymKind_GPROC32:
            {
              // rjf: unpack sym
              CV_SymProc32 *proc32 = (CV_SymProc32 *)sym_header_struct_base;
              String8 name = str8_cstring_capped(proc32+1, sym_data_opl);
              RDIM_Type *type = p2r_type_ptr_from_itype(proc32->itype);
              
              // rjf: unpack proc's container type
              RDIM_Type *container_type = 0;
              U64 container_name_opl = p2r_end_of_cplusplus_container_name(name);
              if(container_name_opl > 2 && tpi_hash != 0 && tpi_leaf != 0)
              {
                String8 container_name = str8(name.str, container_name_opl - 2);
                CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, container_name, 0);
                container_type = p2r_type_ptr_from_itype(cv_type_id);
              }
              
              // rjf: unpack proc's container symbol
              RDIM_Symbol *container_symbol = 0;
              if(container_type == 0 && top_scope_node != 0)
              {
                container_symbol = top_scope_node->scope->symbol;
              }
              
              // rjf: build procedure's root scope
              //
              // NOTE: even if there could be a containing scope at this point (which should be
              //       illegal in C/C++ but not necessarily in another language) we would not use
              //       it here because these scopes refer to the ranges of code that make up a
              //       procedure *not* the namespaces, so a procedure's root scope always has
              //       no parent.
              RDIM_Scope *procedure_root_scope = rdim_scope_chunk_list_push(arena, &sym_scopes, sym_scopes_chunk_cap);
              {
                COFF_SectionHeader *section = (0 < proc32->sec && proc32->sec <= coff_sections.count) ? &coff_sections.v[proc32->sec-1] : 0;
                if(section != 0)
                {
                  U64 voff_first = section->voff + proc32->off;
                  U64 voff_last = voff_first + proc32->len;
                  RDIM_Rng1U64 voff_range = {voff_first, voff_last};
                  rdim_scope_push_voff_range(arena, &sym_scopes, procedure_root_scope, voff_range);
                  procedure_base_voff = voff_first;
                }
              }
              
              // rjf: root scope voff minimum range -> link name
              String8 link_name = {0};
              if(procedure_root_scope->voff_ranges.min != 0)
              {
                U64 voff = procedure_root_scope->voff_ranges.min;
                U64 hash = p2r_hash_from_voff(voff);
                U64 bucket_idx = hash%link_name_map.buckets_count;
                P2R_LinkNameNode *node = 0;
                for(P2R_LinkNameNode *n = link_name_map.buckets[bucket_idx]; n != 0; n = n->next)
                {
                  if(n->voff == voff)
                  {
                    link_name = n->name;
                    break;
                  }
                }
              }
              
              // rjf: build procedure symbol
              RDIM_Symbol *procedure_symbol = rdim_symbol_chunk_list_push(arena, &sym_procedures, sym_procedures_chunk_cap);
              procedure_symbol->is_extern        = (kind == CV_SymKind_GPROC32);
              procedure_symbol->name             = name;
              procedure_symbol->link_name        = link_name;
              procedure_symbol->type             = type;
              procedure_symbol->container_symbol = container_symbol;
              procedure_symbol->container_type   = container_type;
              procedure_symbol->root_scope       = procedure_root_scope;
              
              // rjf: fill root scope's symbol
              procedure_root_scope->symbol = procedure_symbol;
              
              // rjf: push scope to scope stack
              {
                P2R_ScopeNode *node = free_scope_node;
                if(node != 0) { SLLStackPop(free_scope_node); }
                else { node = push_array_no_zero(scratch.arena, P2R_ScopeNode, 1); }
                node->scope = procedure_root_scope;
                SLLStackPush(top_scope_node, node);
              }
              
              // rjf: increment procedure counter
              procedure_num += 1;
            }break;
            
            //- rjf: REGREL32
            case CV_SymKind_REGREL32:
            {
              // TODO(rjf): apparently some of the information here may end up being
              // redundant with "better" information from  CV_SymKind_LOCAL record.
              // we don't currently handle this, but if those cases arise then it
              // will obviously be better to prefer the better information from both
              // records.
              
              // rjf: no containing scope? -> malformed data; locals cannot be produced
              // outside of a containing scope
              if(top_scope_node == 0)
              {
                break;
              }
              
              // rjf: unpack sym
              CV_SymRegrel32 *regrel32 = (CV_SymRegrel32 *)sym_header_struct_base;
              String8 name = str8_cstring_capped(regrel32+1, sym_data_opl);
              RDIM_Type *type = p2r_type_ptr_from_itype(regrel32->itype);
              CV_Reg cv_reg = regrel32->reg;
              U32 var_off = regrel32->reg_off;
              
              // rjf: determine if this is a parameter
              RDI_LocalKind local_kind = RDI_LocalKind_Variable;
              {
                B32 is_stack_reg = 0;
                switch(arch)
                {
                  default:{}break;
                  case RDI_Arch_X86:{is_stack_reg = (cv_reg == CV_Regx86_ESP);}break;
                  case RDI_Arch_X64:{is_stack_reg = (cv_reg == CV_Regx64_RSP);}break;
                }
                if(is_stack_reg)
                {
                  U32 frame_size = 0xFFFFFFFF;
                  if(procedure_num != 0 && procedure_frameprocs[procedure_num-1] != 0 && procedure_num <= procedure_frameprocs_count)
                  {
                    CV_SymFrameproc *frameproc = procedure_frameprocs[procedure_num-1];
                    frame_size = frameproc->frame_size;
                  }
                  if(var_off > frame_size)
                  {
                    local_kind = RDI_LocalKind_Parameter;
                  }
                }
              }
              
              // TODO(rjf): is this correct?
              // rjf: redirect type, if 0, and if outside frame, to the return type of the
              // containing procedure
              if(local_kind == RDI_LocalKind_Parameter && regrel32->itype == 0 &&
                 top_scope_node->scope->symbol != 0 &&
                 top_scope_node->scope->symbol->type != 0)
              {
                type = top_scope_node->scope->symbol->type->direct_type;
              }
              
              // rjf: build local
              RDIM_Scope *scope = top_scope_node->scope;
              RDIM_Local *local = rdim_scope_push_local(arena, &sym_scopes, scope);
              local->kind = local_kind;
              local->name = name;
              local->type = type;
              
              // rjf: add location info to local
              if(type != 0)
              {
                // rjf: determine if we need an extra indirection to the value
                B32 extra_indirection_to_value = 0;
                switch(arch)
                {
                  case RDI_Arch_X86:
                  {
                    extra_indirection_to_value = (local_kind == RDI_LocalKind_Parameter && (type->byte_size > 4 || !IsPow2OrZero(type->byte_size)));
                  }break;
                  case RDI_Arch_X64:
                  {
                    extra_indirection_to_value = (local_kind == RDI_LocalKind_Parameter && (type->byte_size > 8 || !IsPow2OrZero(type->byte_size)));
                  }break;
                }
                
                // rjf: get raddbg register code
                RDI_RegCode reg_code = p2r_rdi_reg_code_from_cv_reg_code(arch, cv_reg);
                // TODO(rjf): real byte_size & byte_pos from cv_reg goes here
                U32 byte_size = 8;
                U32 byte_pos = 0;
                
                // rjf: build location
                RDIM_LocationInfo loc_info = p2r2_location_info_from_addr_reg_off(arena, arch, reg_code, byte_size, byte_pos, (S64)(S32)var_off, extra_indirection_to_value);
                RDIM_Location2 *loc2 = rdim_location_chunk_list_push_new(arena, &sym_locations, sym_locations_chunk_cap, &loc_info);
                RDIM_LocationCase2 *loc_case = rdim_location_case_chunk_list_push(arena, &sym_location_cases, sym_locations_chunk_cap);
                loc_case->location = loc2;
                loc_case->voff_range.min = 0;
                loc_case->voff_range.max = max_U64;
                
                // rjf: equip location case to local
                local->first_location_case = loc_case;
                local->location_case_count = 1;
                
                // rjf: set location case
                RDIM_Location *loc = p2r_location_from_addr_reg_off(arena, arch, reg_code, byte_size, byte_pos, (S64)(S32)var_off, extra_indirection_to_value);
                RDIM_Rng1U64 voff_range = {0, max_U64};
                rdim_location_set_push_case(arena, &sym_scopes, &local->locset, voff_range, loc);
              }
            }break;
            
            //- rjf: LTHREAD32/GTHREAD32
            case CV_SymKind_LTHREAD32:
            case CV_SymKind_GTHREAD32:
            {
              // rjf: unpack sym
              CV_SymThread32 *thread32 = (CV_SymThread32 *)sym_header_struct_base;
              String8 name = str8_cstring_capped(thread32+1, sym_data_opl);
              U32 tls_off = thread32->tls_off;
              RDIM_Type *type = p2r_type_ptr_from_itype(thread32->itype);
              
              // rjf: unpack thread variable's container type
              RDIM_Type *container_type = 0;
              U64 container_name_opl = p2r_end_of_cplusplus_container_name(name);
              if(container_name_opl > 2)
              {
                String8 container_name = str8(name.str, container_name_opl - 2);
                CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, container_name, 0);
                container_type = p2r_type_ptr_from_itype(cv_type_id);
              }
              
              // rjf: unpack thread variable's container symbol
              RDIM_Symbol *container_symbol = 0;
              if(container_type == 0 && top_scope_node != 0)
              {
                container_symbol = top_scope_node->scope->symbol;
              }
              
              // rjf: build symbol
              RDIM_Symbol *tvar = rdim_symbol_chunk_list_push(arena, &sym_thread_variables, sym_thread_variables_chunk_cap);
              tvar->name             = name;
              tvar->type             = type;
              tvar->is_extern        = (kind == CV_SymKind_GTHREAD32);
              tvar->offset           = tls_off;
              tvar->container_type   = container_type;
              tvar->container_symbol = container_symbol;
            }break;
            
            //- rjf: LOCAL
            case CV_SymKind_LOCAL:
            {
              // rjf: no containing scope? -> malformed data; locals cannot be produced
              // outside of a containing scope
              if(top_scope_node == 0)
              {
                break;
              }
              
              // rjf: unpack sym
              CV_SymLocal *slocal = (CV_SymLocal *)sym_header_struct_base;
              String8 name = str8_cstring_capped(slocal+1, sym_data_opl);
              RDIM_Type *type = p2r_type_ptr_from_itype(slocal->itype);
              
              // rjf: determine if this symbol encodes the beginning of a global modification
              B32 is_global_modification = 0;
              if((slocal->flags & CV_LocalFlag_Global) ||
                 (slocal->flags & CV_LocalFlag_Static))
              {
                is_global_modification = 1;
              }
              
              // rjf: is global modification -> emit global modification symbol
              if(is_global_modification)
              {
                // TODO(rjf): add global modification symbols
                defrange_target = 0;
                defrange_target_is_param = 0;
              }
              
              // rjf: is not a global modification -> emit a local variable
              if(!is_global_modification)
              {
                // rjf: determine local kind
                RDI_LocalKind local_kind = RDI_LocalKind_Variable;
                if(slocal->flags & CV_LocalFlag_Param)
                {
                  local_kind = RDI_LocalKind_Parameter;
                }
                
                // rjf: build local
                RDIM_Scope *scope = top_scope_node->scope;
                RDIM_Local *local = rdim_scope_push_local(arena, &sym_scopes, scope);
                local->kind = local_kind;
                local->name = name;
                local->type = type;
                
                // rjf: save defrange target, for subsequent defrange symbols
                defrange_target = &local->locset;
                defrange_target_is_param = (local_kind == RDI_LocalKind_Parameter);
              }
            }break;
            
            //- rjf: DEFRANGE_REGISTER
            case CV_SymKind_DEFRANGE_REGISTER:
            {
              // rjf: no defrange target? -> somehow we got to a defrange symbol without first seeing
              // a local - break immediately
              if(defrange_target == 0)
              {
                break;
              }
              
              // rjf: unpack sym
              CV_SymDefrangeRegister *defrange_register = (CV_SymDefrangeRegister*)sym_header_struct_base;
              CV_Reg cv_reg = defrange_register->reg;
              CV_LvarAddrRange *range = &defrange_register->range;
              COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= coff_sections.count) ? &coff_sections.v[range->sec-1] : 0;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register+1);
              U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
              RDI_RegCode reg_code = p2r_rdi_reg_code_from_cv_reg_code(arch, cv_reg);
              
              // rjf: build location
              RDIM_Location *location = rdim_push_location_val_reg(arena, reg_code);
              
              // rjf: emit locations over ranges
              p2r_location_over_lvar_addr_range(arena, &sym_scopes, defrange_target, location, range, range_section, gaps, gap_count);
            }break;
            
            //- rjf: DEFRANGE_FRAMEPOINTER_REL
            case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL:
            {
              // rjf: no defrange target? -> somehow we got to a defrange symbol without first seeing
              // a local - break immediately
              if(defrange_target == 0)
              {
                break;
              }
              
              // rjf: find current procedure's frameproc
              CV_SymFrameproc *frameproc = 0;
              if(procedure_num != 0 && procedure_num <= procedure_frameprocs_count && procedure_frameprocs[procedure_num-1] != 0)
              {
                frameproc = procedure_frameprocs[procedure_num-1];
              }
              
              // rjf: no current valid frameproc? -> somehow we got a to a framepointer-relative defrange
              // without having an actually active procedure - break
              if(frameproc == 0)
              {
                break;
              }
              
              // rjf: unpack sym
              CV_SymDefrangeFramepointerRel *defrange_fprel = (CV_SymDefrangeFramepointerRel*)sym_header_struct_base;
              CV_LvarAddrRange *range = &defrange_fprel->range;
              COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= coff_sections.count) ? &coff_sections.v[range->sec-1] : 0;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_fprel + 1);
              U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
              
              // rjf: select frame pointer register
              CV_EncodedFramePtrReg encoded_fp_reg = cv_pick_fp_encoding(frameproc, defrange_target_is_param);
              RDI_RegCode fp_register_code = p2r_reg_code_from_arch_encoded_fp_reg(arch, encoded_fp_reg);
              
              // rjf: build location
              B32 extra_indirection = 0;
              U32 byte_size = rdi_addr_size_from_arch(arch);
              U32 byte_pos = 0;
              S64 var_off = (S64)defrange_fprel->off;
              RDIM_LocationInfo location_info = p2r2_location_info_from_addr_reg_off(arena, arch, fp_register_code, byte_size, byte_pos, var_off, extra_indirection);
              RDIM_Location2 *location = rdim_location_chunk_list_push_new(arena, &sym_locations, sym_locations_chunk_cap, &location_info);
              
              // rjf: emit locations over ranges
              // TODO(rjf): p2r_location_over_lvar_addr_range(arena, &sym_scopes, defrange_target, location, range, range_section, gaps, gap_count);
            }break;
            
            //- rjf: DEFRANGE_SUBFIELD_REGISTER
            case CV_SymKind_DEFRANGE_SUBFIELD_REGISTER:
            {
              // rjf: no defrange target? -> somehow we got to a defrange symbol without first seeing
              // a local - break immediately
              if(defrange_target == 0)
              {
                break;
              }
              
              // rjf: unpack sym
              CV_SymDefrangeSubfieldRegister *defrange_subfield_register = (CV_SymDefrangeSubfieldRegister*)sym_header_struct_base;
              CV_Reg cv_reg = defrange_subfield_register->reg;
              CV_LvarAddrRange *range = &defrange_subfield_register->range;
              COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= coff_sections.count) ? &coff_sections.v[range->sec-1] : 0;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_subfield_register + 1);
              U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
              RDI_RegCode reg_code = p2r_rdi_reg_code_from_cv_reg_code(arch, cv_reg);
              
              // rjf: skip "subfield" location info - currently not supported
              if(defrange_subfield_register->field_offset != 0)
              {
                break;
              }
              
              // rjf: build location
              RDIM_Location *location = rdim_push_location_val_reg(arena, reg_code);
              
              // rjf: emit locations over ranges
              p2r_location_over_lvar_addr_range(arena, &sym_scopes, defrange_target, location, range, range_section, gaps, gap_count);
            }break;
            
            //- rjf: DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE
            case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
            {
              // rjf: no defrange target? -> somehow we got to a defrange symbol without first seeing
              // a local - break immediately
              if(defrange_target == 0)
              {
                break;
              }
              
              // rjf: find current procedure's frameproc
              CV_SymFrameproc *frameproc = 0;
              if(procedure_num != 0 && procedure_num <= procedure_frameprocs_count && procedure_frameprocs[procedure_num-1] != 0)
              {
                frameproc = procedure_frameprocs[procedure_num-1];
              }
              
              // rjf: no current valid frameproc? -> somehow we got a to a framepointer-relative defrange
              // without having an actually active procedure - break
              if(frameproc == 0)
              {
                break;
              }
              
              // rjf: unpack sym
              CV_SymDefrangeFramepointerRelFullScope *defrange_fprel_full_scope = (CV_SymDefrangeFramepointerRelFullScope*)sym_header_struct_base;
              CV_EncodedFramePtrReg encoded_fp_reg = cv_pick_fp_encoding(frameproc, defrange_target_is_param);
              RDI_RegCode fp_register_code = p2r_reg_code_from_arch_encoded_fp_reg(arch, encoded_fp_reg);
              
              // rjf: build location
              B32 extra_indirection = 0;
              U32 byte_size = rdi_addr_size_from_arch(arch);
              U32 byte_pos = 0;
              S64 var_off = (S64)defrange_fprel_full_scope->off;
              RDIM_Location *location = p2r_location_from_addr_reg_off(arena, arch, fp_register_code, byte_size, byte_pos, var_off, extra_indirection);
              
              // rjf: emit location over ranges
              RDIM_Rng1U64 voff_range = {0, max_U64};
              rdim_location_set_push_case(arena, &sym_scopes, defrange_target, voff_range, location);
            }break;
            
            //- rjf: DEFRANGE_REGISTER_REL
            case CV_SymKind_DEFRANGE_REGISTER_REL:
            {
              // rjf: no defrange target? -> somehow we got to a defrange symbol without first seeing
              // a local - break immediately
              if(defrange_target == 0)
              {
                break;
              }
              
              // rjf: unpack sym
              CV_SymDefrangeRegisterRel *defrange_register_rel = (CV_SymDefrangeRegisterRel*)sym_header_struct_base;
              CV_Reg cv_reg = defrange_register_rel->reg;
              RDI_RegCode reg_code = p2r_rdi_reg_code_from_cv_reg_code(arch, cv_reg);
              CV_LvarAddrRange *range = &defrange_register_rel->range;
              COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= coff_sections.count) ? &coff_sections.v[range->sec-1] : 0;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register_rel + 1);
              U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
              
              // rjf: build location
              // TODO(rjf): offset & size from cv_reg code
              U32 byte_size = rdi_addr_size_from_arch(arch);
              U32 byte_pos = 0;
              B32 extra_indirection_to_value = 0;
              S64 var_off = defrange_register_rel->reg_off;
              RDIM_Location *location = p2r_location_from_addr_reg_off(arena, arch, reg_code, byte_size, byte_pos, var_off, extra_indirection_to_value);
              
              // rjf: emit locations over ranges
              p2r_location_over_lvar_addr_range(arena, &sym_scopes, defrange_target, location, range, range_section, gaps, gap_count);
            }break;
            
            //- rjf: FILESTATIC
            case CV_SymKind_FILESTATIC:
            {
              CV_SymFileStatic *file_static = (CV_SymFileStatic*)sym_header_struct_base;
              String8 name = str8_cstring_capped(file_static+1, sym_data_opl);
              RDIM_Type *type = p2r_type_ptr_from_itype(file_static->itype);
              // TODO(rjf): emit a global modifier symbol
              defrange_target = 0;
              defrange_target_is_param = 0;
            }break;
            
            //- rjf: INLINESITE
            case CV_SymKind_INLINESITE:
            {
              // rjf: unpack sym
              CV_SymInlineSite *sym           = (CV_SymInlineSite *)sym_header_struct_base;
              String8           binary_annots = str8((U8 *)(sym+1), rec_range->hdr.size - sizeof(rec_range->hdr.kind) - sizeof(*sym));
              
              // rjf: extract external info about inline site
              String8    name      = str8_zero();
              RDIM_Type *type      = 0;
              RDIM_Type *owner     = 0;
              if(ipi_leaf != 0 && ipi_leaf->itype_first <= sym->inlinee && sym->inlinee < ipi_leaf->itype_opl)
              {
                CV_RecRange rec_range = ipi_leaf->leaf_ranges.ranges[sym->inlinee - ipi_leaf->itype_first];
                String8     rec_data  = str8_substr(ipi_leaf->data, rng_1u64(rec_range.off, rec_range.off + rec_range.hdr.size));
                void       *raw_leaf  = rec_data.str + sizeof(U16);
                
                // rjf: extract method inline info
                if(rec_range.hdr.kind == CV_LeafKind_MFUNC_ID &&
                   rec_range.hdr.size >= sizeof(CV_LeafMFuncId))
                {
                  CV_LeafMFuncId *mfunc_id = (CV_LeafMFuncId*)raw_leaf;
                  name  = str8_cstring_capped(mfunc_id + 1, rec_data.str + rec_data.size);
                  type  = p2r_type_ptr_from_itype(mfunc_id->itype);
                  owner = mfunc_id->owner_itype != 0 ? p2r_type_ptr_from_itype(mfunc_id->owner_itype) : 0;
                }
                
                // rjf: extract non-method function inline info
                else if(rec_range.hdr.kind == CV_LeafKind_FUNC_ID &&
                        rec_range.hdr.size >= sizeof(CV_LeafFuncId))
                {
                  CV_LeafFuncId *func_id = (CV_LeafFuncId*)raw_leaf;
                  name  = str8_cstring_capped(func_id + 1, rec_data.str + rec_data.size);
                  type  = p2r_type_ptr_from_itype(func_id->itype);
                  owner = func_id->scope_string_id != 0 ? p2r_type_ptr_from_itype(func_id->scope_string_id) : 0;
                }
              }
              
              // rjf: build inline site
              RDIM_InlineSite *inline_site = rdim_inline_site_chunk_list_push(arena, &sym_inline_sites, sym_inline_sites_chunk_cap);
              inline_site->name       = name;
              inline_site->type       = type;
              inline_site->owner      = owner;
              inline_site->line_table = inline_site_line_table;
              
              // rjf: increment to next inline site line table in this unit
              if(inline_site_line_table != 0 && inline_site_line_table->chunk != 0)
              {
                RDIM_LineTableChunkNode *chunk = inline_site_line_table->chunk;
                U64 current_idx = (U64)(inline_site_line_table - chunk->v);
                if(current_idx+1 < chunk->count)
                {
                  inline_site_line_table += 1;
                }
                else
                {
                  chunk = chunk->next;
                  inline_site_line_table = 0;
                  if(chunk != 0)
                  {
                    inline_site_line_table = chunk->v;
                  }
                }
              }
              
              // rjf: build scope
              RDIM_Scope *scope = rdim_scope_chunk_list_push(arena, &sym_scopes, sym_scopes_chunk_cap);
              scope->inline_site = inline_site;
              if(top_scope_node == 0)
              {
                // TODO(rjf): log
              }
              if(top_scope_node != 0)
              {
                RDIM_Scope *top_scope = top_scope_node->scope;
                SLLQueuePush_N(top_scope->first_child, top_scope->last_child, scope, next_sibling);
                scope->parent_scope = top_scope;
                scope->symbol = top_scope->symbol;
              }
              
              // rjf: push this scope to scope stack
              {
                P2R_ScopeNode *node = free_scope_node;
                if(node != 0) { SLLStackPop(free_scope_node); }
                else { node = push_array_no_zero(scratch.arena, P2R_ScopeNode, 1); }
                node->scope = scope;
                SLLStackPush(top_scope_node, node);
              }
              
              // rjf: parse offset ranges of this inline site - attach to scope
              {
                CV_C13InlineSiteDecoder decoder = cv_c13_inline_site_decoder_init(0, 0, procedure_base_voff);
                for(;;)
                {
                  CV_C13InlineSiteDecoderStep step = cv_c13_inline_site_decoder_step(&decoder, binary_annots);
                  
                  if(step.flags & CV_C13InlineSiteDecoderStepFlag_EmitRange)
                  {
                    // rjf: build new range & add to scope
                    RDIM_Rng1U64 voff_range = { step.range.min, step.range.max };
                    rdim_scope_push_voff_range(arena, &sym_scopes, scope, voff_range);
                  }
                  
                  if(step.flags & CV_C13InlineSiteDecoderStepFlag_ExtendLastRange)
                  {
                    if(scope->voff_ranges.last != 0) 
                    {
                      scope->voff_ranges.last->v.max = step.range.max;
                    }
                  }
                  
                  if(step.flags == 0)
                  {
                    break;
                  }
                }
              }
            }break;
            
            //- rjf: INLINESITE_END
            case CV_SymKind_INLINESITE_END:
            {
              P2R_ScopeNode *n = top_scope_node;
              if(n != 0)
              {
                SLLStackPop(top_scope_node);
                SLLStackPush(free_scope_node, n);
              }
              defrange_target = 0;
              defrange_target_is_param = 0;
            }break;
            
            //- rjf: CONSTANT
            case CV_SymKind_CONSTANT:
            {
              // rjf: unpack
              CV_SymConstant *sym = (CV_SymConstant *)sym_header_struct_base;
              RDIM_Type *type = p2r_type_ptr_from_itype(sym->itype);
              U8 *val_ptr = (U8 *)(sym+1);
              CV_NumericParsed val = cv_numeric_from_data_range(val_ptr, sym_data_opl);
              U64 val64 = cv_u64_from_numeric(&val);
              U8 *name_ptr = val_ptr + val.encoded_size;
              String8 name = str8_cstring_capped(name_ptr, sym_data_opl);
              String8 val_data = str8_struct(&val64);
              U64 container_name_opl = 0;
              if(type != 0)
              {
                container_name_opl = p2r_end_of_cplusplus_container_name(type->name);
              }
              String8 name_qualified = name;
              if(container_name_opl != 0)
              {
                name_qualified = push_str8f(arena, "%S%S", str8_prefix(type->name, container_name_opl), name);
              }
              
              // rjf: build constant symbol
              if(name_qualified.size != 0)
              {
                RDIM_Symbol *cnst = rdim_symbol_chunk_list_push(arena, &sym_constants, sym_constants_chunk_cap);
                cnst->name = name_qualified;
                cnst->type = type;
                rdim_symbol_push_value_data(arena, &sym_constants, cnst, val_data);
              }
            }break;
          }
        }
      }
      
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: output this lane's symbols
    //
    p2r2_shared->lanes_locations[lane_idx()]               = sym_locations;
    p2r2_shared->lanes_location_cases[lane_idx()]          = sym_location_cases;
    p2r2_shared->lanes_procedures[lane_idx()]              = sym_procedures;
    p2r2_shared->lanes_global_variables[lane_idx()]        = sym_global_variables;
    p2r2_shared->lanes_thread_variables[lane_idx()]        = sym_thread_variables;
    p2r2_shared->lanes_constants[lane_idx()]               = sym_constants;
    p2r2_shared->lanes_scopes[lane_idx()]                  = sym_scopes;
    p2r2_shared->lanes_inline_sites[lane_idx()]            = sym_inline_sites;
    p2r2_shared->lanes_typedefs[lane_idx()]                = typedefs;
    
#undef p2r_type_ptr_from_itype
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: join all lane symbols
  //
  {
    if(lane_idx() == lane_from_task_idx(0)) ProfScope("join locations")
    {
      for EachIndex(idx, lane_count())
      {
        rdim_location_chunk_list_concat_in_place(&p2r2_shared->all_locations, &p2r2_shared->lanes_locations[idx]);
      }
    }
    if(lane_idx() == lane_from_task_idx(1)) ProfScope("join location cases")
    {
      for EachIndex(idx, lane_count())
      {
        rdim_location_case_chunk_list_concat_in_place(&p2r2_shared->all_location_cases, &p2r2_shared->lanes_location_cases[idx]);
      }
    }
    if(lane_idx() == lane_from_task_idx(2)) ProfScope("join procedures")
    {
      for EachIndex(idx, lane_count())
      {
        rdim_symbol_chunk_list_concat_in_place(&p2r2_shared->all_procedures, &p2r2_shared->lanes_procedures[idx]);
      }
    }
    if(lane_idx() == lane_from_task_idx(3)) ProfScope("join global variables")
    {
      for EachIndex(idx, lane_count())
      {
        rdim_symbol_chunk_list_concat_in_place(&p2r2_shared->all_global_variables, &p2r2_shared->lanes_global_variables[idx]);
      }
    }
    if(lane_idx() == lane_from_task_idx(4)) ProfScope("join thread variables")
    {
      for EachIndex(idx, lane_count())
      {
        rdim_symbol_chunk_list_concat_in_place(&p2r2_shared->all_thread_variables, &p2r2_shared->lanes_thread_variables[idx]);
      }
    }
    if(lane_idx() == lane_from_task_idx(5)) ProfScope("join constants")
    {
      for EachIndex(idx, lane_count())
      {
        rdim_symbol_chunk_list_concat_in_place(&p2r2_shared->all_constants, &p2r2_shared->lanes_constants[idx]);
      }
    }
    if(lane_idx() == lane_from_task_idx(6)) ProfScope("join scopes")
    {
      for EachIndex(idx, lane_count())
      {
        rdim_scope_chunk_list_concat_in_place(&p2r2_shared->all_scopes, &p2r2_shared->lanes_scopes[idx]);
      }
    }
    if(lane_idx() == lane_from_task_idx(7)) ProfScope("join inline sites")
    {
      for EachIndex(idx, lane_count())
      {
        rdim_inline_site_chunk_list_concat_in_place(&p2r2_shared->all_inline_sites, &p2r2_shared->lanes_inline_sites[idx]);
      }
    }
    if(lane_idx() == lane_from_task_idx(8)) ProfScope("join typedefs")
    {
      for EachIndex(idx, lane_count())
      {
        rdim_type_chunk_list_concat_in_place(&p2r2_shared->all_types__pre_typedefs, &p2r2_shared->lanes_typedefs[idx]);
      }
      p2r2_shared->all_types = p2r2_shared->all_types__pre_typedefs;
    }
  }
  lane_sync();
  RDIM_LocationChunkList all_locations           = p2r2_shared->all_locations;
  RDIM_LocationCaseChunkList all_location_cases  = p2r2_shared->all_location_cases;
  RDIM_SymbolChunkList all_procedures            = p2r2_shared->all_procedures;
  RDIM_SymbolChunkList all_global_variables      = p2r2_shared->all_global_variables;
  RDIM_SymbolChunkList all_thread_variables      = p2r2_shared->all_thread_variables;
  RDIM_SymbolChunkList all_constants             = p2r2_shared->all_constants;
  RDIM_ScopeChunkList all_scopes                 = p2r2_shared->all_scopes;
  RDIM_InlineSiteChunkList all_inline_sites      = p2r2_shared->all_inline_sites;
  RDIM_TypeChunkList all_types                   = p2r2_shared->all_types;
  
  //////////////////////////////////////////////////////////////
  //- rjf: bundle all outputs
  //
  RDIM_BakeParams result = {0};
  {
    //- rjf: produce top-level-info
    RDIM_TopLevelInfo top_level_info = {0};
    {
      top_level_info.arch          = arch;
      top_level_info.exe_name      = str8_skip_last_slash(params->input_exe_name);
      top_level_info.exe_hash      = exe_hash;
      top_level_info.voff_max      = exe_voff_max;
      if(!params->deterministic)
      {
        top_level_info.producer_name = str8_lit(BUILD_TITLE_STRING_LITERAL);
      }
    }
    
    //- rjf: build binary sections list
    RDIM_BinarySectionList binary_sections = {0};
    ProfScope("build binary section list")
    {
      COFF_SectionHeader *coff_ptr = coff_sections.v;
      COFF_SectionHeader *coff_opl = coff_ptr + coff_sections.count;
      for(;coff_ptr < coff_opl; coff_ptr += 1)
      {
        char *name_first = (char *)coff_ptr->name;
        char *name_opl   = name_first + sizeof(coff_ptr->name);
        RDIM_BinarySection *sec = rdim_binary_section_list_push(arena, &binary_sections);
        sec->name       = str8_cstring_capped(name_first, name_opl);
        sec->flags      = p2r_rdi_binary_section_flags_from_coff_section_flags(coff_ptr->flags);
        sec->voff_first = coff_ptr->voff;
        sec->voff_opl   = coff_ptr->voff+coff_ptr->vsize;
        sec->foff_first = coff_ptr->foff;
        sec->foff_opl   = coff_ptr->foff+coff_ptr->fsize;
      }
    }
    
    //- rjf: fill
    result.top_level_info   = top_level_info;
    result.binary_sections  = binary_sections;
    result.units            = all_units;
    result.types            = all_types;
    result.udts             = all_udts;
    result.members          = all_members;
    result.enum_vals        = all_enum_vals;
    result.src_files        = all_src_files;
    result.line_tables      = all_line_tables;
    result.locations        = all_locations;
    result.location_cases   = all_location_cases;
    result.global_variables = all_global_variables;
    result.thread_variables = all_thread_variables;
    result.constants        = all_constants;
    result.procedures       = all_procedures;
    result.scopes           = all_scopes;
    result.inline_sites     = all_inline_sites;
  }
  
  return result;
}
