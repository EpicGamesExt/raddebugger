// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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
  if(lane_idx() == 0) ProfScope("do top-level MSF/PDB extraction")
  {
    ProfScope("parse PDB info")
    {
      String8 info_data = msf_data_from_stream(p2r2_shared->msf, PDB_FixedStream_Info);
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
  if(lane_idx() == 0) ProfScope("bucket compilation unit contributions")
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
  
  //////////////////////////////////////////////////////////////
  //- rjf: gather all source file paths; build nodes
  //
  ProfScope("gather all source file paths; build nodes")
  {
    //- rjf: prep outputs
    if(lane_idx() == 0)
    {
      p2r2_shared->unit_src_file_paths = push_array(arena, String8Array, comp_units->count);
    }
    lane_sync();
    
    //- rjf: do wide gather
#if 0
    {
      Rng1U64 range = lane_range(comp_units->count);
      for EachInRange(idx, range)
      {
        PDB_CompUnit *unit = comp_units->units[idx];
        CV_SymParsed *unit_sym = sym_for_unit[idx];
        CV_C13Parsed *unit_c13 = c13_for_unit[idx];
        CV_RecRange *rec_ranges_first = unit_sym->sym_ranges.ranges;
        CV_RecRange *rec_ranges_opl   = rec_ranges_first+unit_sym->sym_ranges.count;
        String8List src_file_paths = {0};
        {
          Temp scratch = scratch_begin(&arena, 1);
          
          //- rjf: build local hash table to dedup files within this unit
          U64 hit_path_slots_count = 4096;
          String8Node **hit_path_slots = push_array(scratch.arena, String8Node *, hit_path_slots_count);
          
          //- rjf: produce obj name/path
          String8 obj_name = unit->obj_name;
          if(str8_match(obj_name, str8_lit("* Linker *"), 0) ||
             str8_match(obj_name, str8_lit("Import:"), StringMatchFlag_RightSideSloppy))
          {
            MemoryZeroStruct(&obj_name);
          }
          String8 obj_folder_path = lower_from_str8(scratch.arena, str8_chop_last_slash(obj_name));
          
          //- rjf: find all files in this unit's (non-inline) line info
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
          
          //- rjf: find all files in unit's inline line info
          ProfScope("find all files in unit's inline line info")
          {
            U64 base_voff = 0;
            for(CV_RecRange *rec_range = rec_ranges_first;
                rec_range < rec_ranges_opl;
                rec_range += 1)
            {
              //- rjf: rec range -> symbol info range
              U64 sym_off_first = rec_range->off + 2;
              U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
              
              //- rjf: skip invalid ranges
              if(sym_off_opl > pdb_unit_sym->data.size || sym_off_first > pdb_unit_sym->data.size || sym_off_first > sym_off_opl)
              {
                continue;
              }
              
              //- rjf: unpack symbol info
              CV_SymKind kind = rec_range->hdr.kind;
              U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
              void *sym_header_struct_base = pdb_unit_sym->data.str + sym_off_first;
              void *sym_data_opl = pdb_unit_sym->data.str + sym_off_opl;
              
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
                  COFF_SectionHeader *section = (0 < proc32->sec && proc32->sec <= in->coff_sections.count) ? &in->coff_sections.v[proc32->sec-1] : 0;
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
                    U64 slot_idx = hash%pdb_unit_c13->inlinee_lines_parsed_slots_count;
                    for(CV_C13InlineeLinesParsedNode *n = pdb_unit_c13->inlinee_lines_parsed_slots[slot_idx]; n != 0; n = n->hash_next)
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
                    CV_C13SubSectionNode *file_chksms = pdb_unit_c13->file_chksms_sub_section;
                    
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
                          CV_C13Checksum *checksum = (CV_C13Checksum*)(pdb_unit_c13->data.str + file_chksms->off + last_file_off);
                          U32             name_off = checksum->name_off;
                          seq_file_name = pdb_strtbl_string_from_off(in->pdb_strtbl, name_off);
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
                          str8_list_push(scratch.arena, &src_file_paths, push_str8_copy(arena, file_path_normalized));
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
          
          scratch_end(scratch);
        }
        p2r2_shared->unit_src_file_paths[idx] = str8_array_from_list(arena, &src_file_paths);
      }
    }
#endif
    
#if 0 // TODO(rjf): OLD
    U64 tasks_count = comp_unit_count;
    P2R_GatherUnitSrcFilesIn *tasks_inputs = push_array(scratch.arena, P2R_GatherUnitSrcFilesIn, tasks_count);
    P2R_GatherUnitSrcFilesOut *tasks_outputs = push_array(scratch.arena, P2R_GatherUnitSrcFilesOut, tasks_count);
    ASYNC_Task **tasks = push_array(scratch.arena, ASYNC_Task *, tasks_count);
    for EachIndex(idx, tasks_count)
    {
      tasks_inputs[idx].pdb_strtbl     = strtbl;
      tasks_inputs[idx].coff_sections  = coff_sections;
      tasks_inputs[idx].comp_unit      = comp_units->units[idx];
      tasks_inputs[idx].comp_unit_syms = sym_for_unit[idx];
      tasks_inputs[idx].comp_unit_c13s = c13_for_unit[idx];
      tasks[idx] = async_task_launch(scratch.arena, p2r_gather_unit_src_file_work, .input = &tasks_inputs[idx]);
    }
    U64 total_path_count = 0;
    for EachIndex(idx, tasks_count)
    {
      tasks_outputs[idx] = *async_task_join_struct(tasks[idx], P2R_GatherUnitSrcFilesOut);
      total_path_count += tasks_outputs[idx].src_file_paths.count;
    }
    src_file_map.slots_count = total_path_count + total_path_count/2 + 1;
    src_file_map.slots = push_array(scratch.arena, P2R_SrcFileNode *, src_file_map.slots_count);
#endif
    
    //- rjf: build src file map
#if 0
    for EachIndex(idx, tasks_count)
    {
      for EachIndex(path_idx, tasks_outputs[idx].src_file_paths.count)
      {
        String8 file_path_sanitized = tasks_outputs[idx].src_file_paths.v[path_idx];
        U64 file_path_sanitized_hash = rdi_hash(file_path_sanitized.str, file_path_sanitized.size);
        U64 src_file_slot = file_path_sanitized_hash%src_file_map.slots_count;
        P2R_SrcFileNode *src_file_node = 0;
        for(P2R_SrcFileNode *n = src_file_map.slots[src_file_slot]; n != 0; n = n->next)
        {
          if(str8_match(n->src_file->path, file_path_sanitized, 0))
          {
            src_file_node = n;
            break;
          }
        }
        if(src_file_node == 0)
        {
          src_file_node = push_array(scratch.arena, P2R_SrcFileNode, 1);
          SLLStackPush(src_file_map.slots[src_file_slot], src_file_node);
          src_file_node->src_file = rdim_src_file_chunk_list_push(arena, &all_src_files__sequenceless, total_path_count);
          src_file_node->src_file->path = push_str8_copy(arena, file_path_sanitized);
        }
      }
    }
#endif
  }
  RDIM_SrcFileChunkList all_src_files__sequenceless = {0};
  P2R_SrcFileMap src_file_map = {0};
}

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
