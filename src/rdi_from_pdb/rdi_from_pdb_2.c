// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDIM_BakeParams
p2r2_convert(Arena **thread_arenas, U64 thread_count, P2R_ConvertParams *in)
{
  RDIM_BakeParams result = {0};
  Temp scratch = scratch_begin(thread_arenas, thread_count);
  Barrier barrier = barrier_alloc(thread_count);
  {
    P2R2_ConvertThreadParams *thread_params = push_array(scratch.arena, P2R2_ConvertThreadParams, thread_count);
    OS_Handle *threads = push_array(scratch.arena, OS_Handle, thread_count);
    for EachIndex(idx, thread_count)
    {
      thread_params[idx].arena = thread_arenas[idx];
      thread_params[idx].lane_ctx.lane_idx   = idx;
      thread_params[idx].lane_ctx.lane_count = thread_count;
      thread_params[idx].lane_ctx.barrier    = barrier;
      thread_params[idx].input_exe_name      = in->input_exe_name;
      thread_params[idx].input_exe_data      = in->input_exe_data;
      thread_params[idx].input_pdb_name      = in->input_pdb_name;
      thread_params[idx].input_pdb_data      = in->input_pdb_data;
      thread_params[idx].deterministic       = in->deterministic;
    }
    for EachIndex(idx, thread_count)
    {
      threads[idx] = os_thread_launch(p2r2_convert_thread_entry_point, &thread_params[idx], 0);
    }
    for EachIndex(idx, thread_count)
    {
      os_thread_join(threads[idx], max_U64);
    }
  }
  barrier_release(barrier);
  scratch_end(scratch);
  return result;
}

internal void
p2r2_convert_thread_entry_point(void *p)
{
  P2R2_ConvertThreadParams *params = (P2R2_ConvertThreadParams *)p;
  Arena *arena = params->arena;
  lane_ctx(params->lane_ctx);
  ThreadNameF("p2r2_convert_thread_%I64u", lane_idx());
  
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
    }
    lane_sync();
    
    // rjf: do wide fill
    {
      Rng1U64 range = lane_range(p2r2_shared->msf->stream_count);
      for EachInRange(idx, range)
      {
        p2r2_shared->msf->streams[idx] = msf_data_from_stream_number(arena, params->input_pdb_data, p2r2_shared->msf_raw_stream_table, idx);
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
    if(lane_idx() == lane_from_task_idx(5)) ProfScope("parse global symbol stream")
    {
      String8 sym_data = msf_data_from_stream(msf, dbi->sym_sn);
      p2r2_shared->sym = cv_sym_from_data(arena, sym_data, 4);
    }
    if(lane_idx() == lane_from_task_idx(6)) ProfScope("parse compilation units")
    {
      String8 comp_units_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo);
      p2r2_shared->comp_units = pdb_comp_unit_array_from_data(arena, comp_units_data);
    }
    if(lane_idx() == lane_from_task_idx(7)) ProfScope("parse compilation unit contributions")
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
  CV_SymParsed *sym = p2r2_shared->sym;
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
  //- rjf: parse syms & line info for each compilation unit
  //
  ProfScope("parse syms & line info for each compilation unit")
  {
    //- rjf: setup outputs
    if(lane_idx() == 0)
    {
      p2r2_shared->sym_for_unit = push_array(arena, CV_SymParsed *, comp_units->count);
      p2r2_shared->c13_for_unit = push_array(arena, CV_C13Parsed *, comp_units->count);
    }
    lane_sync();
    
    //- rjf: wide fill
    {
      Rng1U64 range = lane_range(comp_units->count);
      for EachInRange(idx, range)
      {
        PDB_CompUnit *unit = comp_units->units[idx];
        String8 unit_sym_data = pdb_data_from_unit_range(msf, unit, PDB_DbiCompUnitRange_Symbols);
        String8 unit_c13_data = pdb_data_from_unit_range(msf, unit, PDB_DbiCompUnitRange_C13);
        p2r2_shared->sym_for_unit[idx] = cv_sym_from_data(arena, unit_sym_data, 4);
        p2r2_shared->c13_for_unit[idx] = cv_c13_parsed_from_data(arena, unit_c13_data, raw_strtbl, coff_sections);
      }
    }
  }
  lane_sync();
  CV_SymParsed **sym_for_unit = p2r2_shared->sym_for_unit;
  CV_C13Parsed **c13_for_unit = p2r2_shared->c13_for_unit;
  
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
    for(U64 comp_unit_idx = 0; comp_unit_idx < comp_units->count; comp_unit_idx += 1)
    {
      p2r2_shared->arch = p2r_rdi_arch_from_cv_arch(sym_for_unit[comp_unit_idx]->info.arch);
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
  //- rjf: organize subsets of unit symbol streams by lane
  //
  ProfScope("organize subsets of unit symbol streams by lane")
  {
    //- rjf: set up
    ProfScope("set up") if(lane_idx() == 0)
    {
      p2r2_shared->lane_sym_blocks = push_array(arena, P2R2_UnitSymBlockList, lane_count());
      p2r2_shared->total_sym_record_count = 0;
      for EachIndex(unit_idx, comp_units->count)
      {
        p2r2_shared->total_sym_record_count += sym_for_unit[unit_idx]->sym_ranges.count;
      }
    }
    lane_sync();
    
    //- rjf: gather
    ProfScope("gather")
    {
      Rng1U64 lane_sym_range = lane_range(p2r2_shared->total_sym_record_count);
      {
        U64 scan_sym_idx = 0;
        for EachIndex(idx, comp_units->count)
        {
          Rng1U64 unit_sym_range = r1u64(scan_sym_idx, scan_sym_idx + sym_for_unit[idx]->sym_ranges.count);
          Rng1U64 sym_range_in_unit = intersect_1u64(unit_sym_range, lane_sym_range);
          if(sym_range_in_unit.max > sym_range_in_unit.min)
          {
            P2R2_UnitSymBlock *block = push_array(arena, P2R2_UnitSymBlock, 1);
            SLLQueuePush(p2r2_shared->lane_sym_blocks[lane_idx()].first, p2r2_shared->lane_sym_blocks[lane_idx()].last, block);
            block->unit_idx = idx;
            block->unit_rec_range = r1u64(sym_range_in_unit.min - scan_sym_idx, sym_range_in_unit.max - scan_sym_idx);
          }
          scan_sym_idx += sym_for_unit[idx]->sym_ranges.count;
        }
      }
    }
  }
  lane_sync();
  P2R2_UnitSymBlockList *lane_sym_blocks = p2r2_shared->lane_sym_blocks;
  
  //////////////////////////////////////////////////////////////
  //- rjf: gather all file paths
  //
  ProfScope("gather all file paths")
  {
    //- rjf: prep outputs
    ProfScope("prep outputs") if(lane_idx() == 0)
    {
      p2r2_shared->lane_file_paths = push_array(arena, String8Array, lane_count());
      p2r2_shared->lane_file_paths_hashes = push_array(arena, U64Array, lane_count());
    }
    lane_sync();
    
    //- rjf: do wide gather
    ProfScope("do wide gather")
    {
      Temp scratch = scratch_begin(&arena, 1);
      String8List src_file_paths = {0};
      
      //- rjf: build local hash table to dedup files within this lane
      U64 hit_path_slots_count = 4096;
      String8Node **hit_path_slots = push_array(scratch.arena, String8Node *, hit_path_slots_count);
      
      //- rjf: iterate lane blocks & gather inline site file names
      ProfScope("gather inline site file names from this lane's symbols")
        for(P2R2_UnitSymBlock *lane_block = lane_sym_blocks[lane_idx()].first;
            lane_block != 0;
            lane_block = lane_block->next)
      {
        //- rjf: unpack unit
        PDB_CompUnit *unit = comp_units->units[lane_block->unit_idx];
        CV_SymParsed *unit_sym = sym_for_unit[lane_block->unit_idx];
        CV_C13Parsed *unit_c13 = c13_for_unit[lane_block->unit_idx];
        CV_RecRange *rec_ranges_first = unit_sym->sym_ranges.ranges + lane_block->unit_rec_range.min;
        CV_RecRange *rec_ranges_opl   = unit_sym->sym_ranges.ranges + lane_block->unit_rec_range.max;
        
        //- rjf: produce obj name/path
        String8 obj_name = unit->obj_name;
        if(str8_match(obj_name, str8_lit("* Linker *"), 0) ||
           str8_match(obj_name, str8_lit("Import:"), StringMatchFlag_RightSideSloppy))
        {
          MemoryZeroStruct(&obj_name);
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
          if(sym_off_opl > unit_sym->data.size || sym_off_first > unit_sym->data.size || sym_off_first > sym_off_opl)
          {
            continue;
          }
          
          //- rjf: unpack symbol info
          CV_SymKind kind = rec_range->hdr.kind;
          U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
          void *sym_header_struct_base = unit_sym->data.str + sym_off_first;
          void *sym_data_opl = unit_sym->data.str + sym_off_opl;
          
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
                U64 slot_idx = hash%unit_c13->inlinee_lines_parsed_slots_count;
                for(CV_C13InlineeLinesParsedNode *n = unit_c13->inlinee_lines_parsed_slots[slot_idx]; n != 0; n = n->hash_next)
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
                CV_C13SubSectionNode *file_chksms = unit_c13->file_chksms_sub_section;
                
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
                      CV_C13Checksum *checksum = (CV_C13Checksum*)(unit_c13->data.str + file_chksms->off + last_file_off);
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
                      str8_list_push(arena, &src_file_paths, push_str8_copy(arena, file_path_normalized));
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
          CV_SymParsed *unit_sym = sym_for_unit[idx];
          CV_C13Parsed *unit_c13 = c13_for_unit[idx];
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
                  str8_list_push(scratch.arena, &src_file_paths, push_str8_copy(arena, file_path_sanitized));
                }
              }
            }
          }
        }
      }
      
      //- rjf: merge into array for this lane
      p2r2_shared->lane_file_paths[lane_idx()] = str8_array_from_list(arena, &src_file_paths);
      
      //- rjf: hash this lane's file paths
      {
        String8Array lane_paths = p2r2_shared->lane_file_paths[lane_idx()];
        U64Array lane_paths_hashes = {0};
        lane_paths_hashes.count = lane_paths.count;
        lane_paths_hashes.v = push_array(arena, U64, lane_paths_hashes.count);
        for EachIndex(idx, lane_paths.count)
        {
          lane_paths_hashes.v[idx] = rdi_hash(lane_paths.v[idx].str, lane_paths.v[idx].size);
        }
        p2r2_shared->lane_file_paths_hashes[lane_idx()] = lane_paths_hashes;
      }
      
      scratch_end(scratch);
    }
  }
  lane_sync();
  String8Array *lane_file_paths = p2r2_shared->lane_file_paths;
  U64Array *lane_file_paths_hashes = p2r2_shared->lane_file_paths_hashes;
  
  //////////////////////////////
  //- rjf: build unified collection & map for source files
  //
  {
    //- rjf: set up table
    ProfScope("set up table") if(lane_idx() == 0)
    {
      p2r2_shared->total_path_count = 0;
      for EachIndex(idx, lane_count())
      {
        p2r2_shared->total_path_count += p2r2_shared->lane_file_paths[idx].count;
      }
      p2r2_shared->src_file_map.slots_count = p2r2_shared->total_path_count + p2r2_shared->total_path_count/2 + 1;
      p2r2_shared->src_file_map.slots = push_array(arena, P2R_SrcFileNode *, p2r2_shared->src_file_map.slots_count);
    }
    lane_sync();
    
    //- rjf: fill table
    ProfScope("fill table") if(lane_idx() == 0)
    {
      for EachIndex(idx, lane_count())
      {
        for EachIndex(path_idx, p2r2_shared->lane_file_paths[idx].count)
        {
          String8 file_path_sanitized = p2r2_shared->lane_file_paths[idx].v[path_idx];
          U64 file_path_sanitized_hash = p2r2_shared->lane_file_paths_hashes[idx].v[path_idx];
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
  lane_sync();
  RDIM_SrcFileChunkList all_src_files__sequenceless = p2r2_shared->all_src_files__sequenceless;
  P2R_SrcFileMap src_file_map = p2r2_shared->src_file_map;
  
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
      p2r2_shared->units_first_inline_site_line_tables = push_array(arena, RDIM_LineTable *, comp_units->count);
      p2r2_shared->lanes_line_tables = push_array(arena, RDIM_LineTableChunkList, lane_count());
    }
    lane_sync();
    RDIM_Unit *units = p2r2_shared->all_units.first->v;
    U64 units_count = p2r2_shared->all_units.first->count;
    RDIM_LineTableChunkList *lanes_line_tables = p2r2_shared->lanes_line_tables;
    Assert(units_count == comp_units->count);
    
    //- rjf: do per-lane work
    {
      RDIM_LineTableChunkList *dst_line_tables = &lanes_line_tables[lane_idx()];
      
      //- rjf: per-unit line table conversion
      ProfScope("per-unit line table conversion")
      {
        Rng1U64 range = lane_range(units_count);
        for EachInRange(idx, range)
        {
          Temp scratch = scratch_begin(&arena, 1);
          PDB_CompUnit *src_unit     = comp_units->units[idx];
          CV_SymParsed *src_unit_sym = sym_for_unit[idx];
          CV_C13Parsed *src_unit_c13 = c13_for_unit[idx];
          RDIM_Unit *dst_unit = &units[idx];
          
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
                    line_table = rdim_line_table_chunk_list_push(arena, dst_line_tables, 256);
                  }
                  RDIM_LineSequence *seq = rdim_line_table_push_sequence(arena, dst_line_tables, line_table, src_file_node->src_file, lines->voffs, lines->line_nums, lines->col_nums, lines->line_count);
                }
              }
            }
          }
          scratch_end(scratch);
        }
      }
      
      //- rjf: build per-inline-site line tables
      ProfScope("build per-inline-site line tables")
      {
        U64 last_unit_num_started_by_this_lane = 0;
        for(P2R2_UnitSymBlock *lane_block = lane_sym_blocks[lane_idx()].first;
            lane_block != 0;
            lane_block = lane_block->next)
        {
          Temp scratch = scratch_begin(&arena, 1);
          if(lane_block->unit_rec_range.min == 0)
          {
            last_unit_num_started_by_this_lane = lane_block->unit_idx+1;
          }
          else if(last_unit_num_started_by_this_lane-1 != lane_block->unit_idx)
          {
            last_unit_num_started_by_this_lane = 0;
          }
          PDB_CompUnit *src_unit     = comp_units->units[lane_block->unit_idx];
          CV_SymParsed *src_unit_sym = sym_for_unit[lane_block->unit_idx];
          CV_C13Parsed *src_unit_c13 = c13_for_unit[lane_block->unit_idx];
          String8 obj_name = src_unit->obj_name;
          if(str8_match(obj_name, str8_lit("* Linker *"), 0) ||
             str8_match(obj_name, str8_lit("Import:"), StringMatchFlag_RightSideSloppy))
          {
            MemoryZeroStruct(&obj_name);
          }
          String8 obj_folder_path = backslashed_from_str8(scratch.arena, str8_chop_last_slash(obj_name));
          CV_RecRange *rec_ranges_first = src_unit_sym->sym_ranges.ranges + lane_block->unit_rec_range.min;
          CV_RecRange *rec_ranges_opl   = src_unit_sym->sym_ranges.ranges + lane_block->unit_rec_range.max;
          U64 base_voff = 0;
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
                          line_table = rdim_line_table_chunk_list_push(arena, dst_line_tables, 256);
                          if(last_unit_num_started_by_this_lane != 0)
                          {
                            if(p2r2_shared->units_first_inline_site_line_tables[last_unit_num_started_by_this_lane-1] == 0)
                            {
                              p2r2_shared->units_first_inline_site_line_tables[last_unit_num_started_by_this_lane-1] = line_table;
                            }
                          }
                        }
                        rdim_line_table_push_sequence(arena, dst_line_tables, line_table, src_file_node->src_file, voffs, line_nums, 0, line_count);
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
  RDIM_UnitChunkList *all_units = &p2r2_shared->all_units;
  RDIM_LineTableChunkList *lanes_line_tables = p2r2_shared->lanes_line_tables;
  RDIM_LineTable **units_first_inline_site_line_tables = p2r2_shared->units_first_inline_site_line_tables;
  
  //////////////////////////////////////////////////////////////
  //- rjf: join all line tables
  //
  ProfScope("join all line tables") if(lane_idx() == 0)
  {
    for EachIndex(idx, lane_count())
    {
      rdim_line_table_chunk_list_concat_in_place(&p2r2_shared->all_line_tables, &p2r2_shared->lanes_line_tables[idx]);
    }
  }
  lane_sync();
  RDIM_LineTableChunkList all_line_tables = p2r2_shared->all_line_tables;
  
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
  ProfScope("types pass 3: construct all root/stub types from TPI") if(lane_idx() == 0)
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
    p2r2_shared->all_types = all_types;
#undef p2r_type_ptr_from_itype
#undef p2r_builtin_type_ptr_from_kind
  }
  lane_sync();
  RDIM_Type **itype_type_ptrs = p2r2_shared->itype_type_ptrs;
  RDIM_Type **basic_type_ptrs = p2r2_shared->basic_type_ptrs;
  RDIM_TypeChunkList all_types = p2r2_shared->all_types;
  
  
  //-
  //-
  //--
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce top-level-info
  //
  RDIM_TopLevelInfo top_level_info = {0};
  {
    top_level_info.arch          = arch;
    top_level_info.exe_name      = str8_skip_last_slash(params->input_exe_name);
    top_level_info.exe_hash      = exe_hash;
    top_level_info.voff_max      = exe_voff_max;
    if(params->deterministic)
    {
      top_level_info.producer_name = str8_lit(BUILD_TITLE_STRING_LITERAL);
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: build binary sections list
  //
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
  
}
