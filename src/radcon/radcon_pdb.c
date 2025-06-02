// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// TODO(rjf): eliminate redundant null checks, just always allocate
// empty results, and have nulls gracefully fall through
//
// (search for != 0 instances, inserted to prevent prior crashes)

global RDIM_LocalState *g_p2r_local_state = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
p2r_end_of_cplusplus_container_name(String8 str)
{
  // NOTE: This finds the index one past the last "::" contained in str.
  //       if no "::" is contained in str, then the returned index is 0.
  //       The intent is that [0,clamp_bot(0,result - 2)) gives the
  //       "container name" and [result,str.size) gives the leaf name.
  U64 result = 0;
  if(str.size >= 2)
  {
    for(U64 i = str.size; i >= 2; i -= 1)
    {
      if(str.str[i - 2] == ':' && str.str[i - 1] == ':')
      {
        result = i;
        break;
      }
    }
  }
  return(result);
}

internal U64
p2r_hash_from_voff(U64 voff)
{
  U64 hash = (voff >> 3) ^ ((7 & voff) << 6);
  return hash;
}

////////////////////////////////
//~ rjf: Location Info Building Helpers

internal RDIM_Location *
p2r_location_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection)
{
  RDIM_Location *result = 0;
  if(0 <= offset && offset <= (S64)max_U16)
  {
    if(extra_indirection)
    {
      result = rdim_push_location_addr_addr_reg_plus_u16(arena, reg_code, (U16)offset);
    }
    else
    {
      result = rdim_push_location_addr_reg_plus_u16(arena, reg_code, (U16)offset);
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
    result = rdim_push_location_addr_bytecode_stream(arena, &bytecode);
  }
  return result;
}

internal void
p2r_location_over_lvar_addr_range(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_LocationSet *locset, RDIM_Location *location, CV_LvarAddrRange *range, COFF_SectionHeader *section, CV_LvarAddrGap *gaps, U64 gap_count)
{
  //- rjf: extract range info
  U64 voff_first = 0;
  U64 voff_opl = 0;
  if(section != 0)
  {
    voff_first = section->voff + range->off;
    voff_opl = voff_first + range->len;
  }
  
  //- rjf: emit ranges
  CV_LvarAddrGap *gap_ptr = gaps;
  U64 voff_cursor = voff_first;
  for(U64 i = 0; i < gap_count; i += 1, gap_ptr += 1)
  {
    U64 voff_gap_first = voff_first + gap_ptr->off;
    U64 voff_gap_opl   = voff_gap_first + gap_ptr->len;
    if(voff_cursor < voff_gap_first)
    {
      RDIM_Rng1U64 voff_range = {voff_cursor, voff_gap_first};
      rdim_location_set_push_case(arena, scopes, locset, voff_range, location);
    }
    voff_cursor = voff_gap_opl;
  }
  
  //- rjf: emit remaining range
  if(voff_cursor < voff_opl)
  {
    RDIM_Rng1U64 voff_range = {voff_cursor, voff_opl};
    rdim_location_set_push_case(arena, scopes, locset, voff_range, location);
  }
}

////////////////////////////////
//~ rjf: Initial Parsing & Preparation Pass Tasks

ASYNC_WORK_DEF(p2r_exe_hash_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  P2R_EXEHashIn *in = (P2R_EXEHashIn *)input;
  U64 *out = push_array(arena, U64, 1);
  ProfScope("hash exe") *out = rdi_hash(in->exe_data.str, in->exe_data.size);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(p2r_tpi_hash_parse_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  P2R_TPIHashParseIn *in = (P2R_TPIHashParseIn *)input;
  void *out = 0;
  ProfScope("parse tpi hash") out = pdb_tpi_hash_from_data(arena, in->strtbl, in->tpi, in->hash_data, in->aux_data);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(p2r_tpi_leaf_parse_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  P2R_TPILeafParseIn *in = (P2R_TPILeafParseIn *)input;
  void *out = 0;
  ProfScope("parse tpi leaf") out = cv_leaf_from_data(arena, in->leaf_data, in->itype_first);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(p2r_symbol_stream_parse_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  P2R_SymbolStreamParseIn *in = (P2R_SymbolStreamParseIn *)input;
  void *out = 0;
  ProfScope("parse symbol stream") out = cv_sym_from_data(arena, in->data, 4);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(p2r_c13_stream_parse_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  P2R_C13StreamParseIn *in = (P2R_C13StreamParseIn *)input;
  void *out = 0;
  ProfScope("parse c13 stream") out = cv_c13_parsed_from_data(arena, in->data, in->strtbl, in->coff_sections);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(p2r_comp_unit_parse_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  P2R_CompUnitParseIn *in = (P2R_CompUnitParseIn *)input;
  void *out = 0;
  ProfScope("parse comp units") out = pdb_comp_unit_array_from_data(arena, in->data);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(p2r_comp_unit_contributions_parse_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  P2R_CompUnitContributionsParseIn *in = (P2R_CompUnitContributionsParseIn *)input;
  void *out = 0;
  ProfScope("parse comp unit contributions") out = pdb_comp_unit_contribution_array_from_data(arena, in->data, in->coff_sections);
  ProfEnd();
  return out;
}

////////////////////////////////
//~ rjf: Unit Conversion Tasks

ASYNC_WORK_DEF(p2r_units_convert_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  Temp scratch = scratch_begin(&arena, 1);
  P2R_UnitConvertIn *in = (P2R_UnitConvertIn *)input;
  P2R_UnitConvertOut *out = push_array(arena, P2R_UnitConvertOut, 1);
  ProfScope("build units, initial src file map, & collect unit source files")
    if(in->comp_units != 0)
  {
    U64 units_chunk_cap = in->comp_units->count;
    P2R_SrcFileMap src_file_map = {0};
    src_file_map.slots_count = 65536;
    src_file_map.slots = push_array(scratch.arena, P2R_SrcFileNode *, src_file_map.slots_count);
    
    ////////////////////////////
    //- rjf: pass 1: build per-unit info & per-unit line tables
    //
    ProfScope("pass 1: build per-unit info & per-unit line tables")
      for(U64 comp_unit_idx = 0; comp_unit_idx < in->comp_units->count; comp_unit_idx += 1)
    {
      PDB_CompUnit *pdb_unit     = in->comp_units->units[comp_unit_idx];
      CV_SymParsed *pdb_unit_sym = in->comp_unit_syms[comp_unit_idx];
      CV_C13Parsed *pdb_unit_c13 = in->comp_unit_c13s[comp_unit_idx];
      
      //- rjf: produce unit name
      String8 unit_name = pdb_unit->obj_name;
      if(unit_name.size != 0)
      {
        String8 unit_name_past_last_slash = str8_skip_last_slash(unit_name);
        if(unit_name_past_last_slash.size != 0)
        {
          unit_name = unit_name_past_last_slash;
        }
      }
      
      //- rjf: produce obj name
      String8 obj_name = pdb_unit->obj_name;
      if(str8_match(obj_name, str8_lit("* Linker *"), 0) ||
         str8_match(obj_name, str8_lit("Import:"), StringMatchFlag_RightSideSloppy))
      {
        MemoryZeroStruct(&obj_name);
      }
      
      //- rjf: build this unit's line table, fill out primary line info (inline info added after)
      RDIM_LineTable *line_table = 0;
      for(CV_C13SubSectionNode *node = pdb_unit_c13->first_sub_section;
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
            
            // rjf: file name -> normalized file path
            String8 file_path = lines->file_name;
            String8 file_path_normalized = lower_from_str8(scratch.arena, str8_skip_chop_whitespace(file_path));
            for(U64 idx = 0; idx < file_path_normalized.size; idx += 1)
            {
              if(file_path_normalized.str[idx] == '\\')
              {
                file_path_normalized.str[idx] = '/';
              }
            }
            
            // rjf: normalized file path -> source file node
            U64 file_path_normalized_hash = rdi_hash(file_path_normalized.str, file_path_normalized.size);
            U64 src_file_slot = file_path_normalized_hash%src_file_map.slots_count;
            P2R_SrcFileNode *src_file_node = 0;
            for(P2R_SrcFileNode *n = src_file_map.slots[src_file_slot]; n != 0; n = n->next)
            {
              if(str8_match(n->src_file->normal_full_path, file_path_normalized, 0))
              {
                src_file_node = n;
                break;
              }
            }
            if(src_file_node == 0)
            {
              src_file_node = push_array(scratch.arena, P2R_SrcFileNode, 1);
              SLLStackPush(src_file_map.slots[src_file_slot], src_file_node);
              src_file_node->src_file = rdim_src_file_chunk_list_push(arena, &out->src_files, 4096);
              src_file_node->src_file->normal_full_path = push_str8_copy(arena, file_path_normalized);
            }
            
            // rjf: push sequence into both line table & source file's line map
            if(lines->line_count != 0)
            {
              if(line_table == 0)
              {
                line_table = rdim_line_table_chunk_list_push(arena, &out->line_tables, 256);
              }
              RDIM_LineSequence *seq = rdim_line_table_push_sequence(arena, &out->line_tables, line_table, src_file_node->src_file, lines->voffs, lines->line_nums, lines->col_nums, lines->line_count);
              rdim_src_file_push_line_sequence(arena, &out->src_files, src_file_node->src_file, seq);
            }
          }
        }
      }
      
      //- rjf: build unit
      RDIM_Unit *dst_unit = rdim_unit_chunk_list_push(arena, &out->units, units_chunk_cap);
      dst_unit->unit_name     = unit_name;
      dst_unit->compiler_name = pdb_unit_sym->info.compiler_name;
      dst_unit->object_file   = obj_name;
      dst_unit->archive_file  = pdb_unit->group_name;
      dst_unit->language      = cv2r_rdi_language_from_cv_language(pdb_unit_sym->info.language);
      dst_unit->line_table    = line_table;
    }
    
    ////////////////////////////
    //- rjf: pass 2: build per-unit voff ranges from comp unit contributions table
    //
    PDB_CompUnitContribution *contrib_ptr = in->comp_unit_contributions->contributions;
    PDB_CompUnitContribution *contrib_opl = contrib_ptr + in->comp_unit_contributions->count;
    ProfScope("pass 2: build per-unit voff ranges from comp unit contributions table")
      for(;contrib_ptr < contrib_opl; contrib_ptr += 1)
    {
      if(contrib_ptr->mod < in->comp_units->count)
      {
        RDIM_Unit *unit = &out->units.first->v[contrib_ptr->mod];
        RDIM_Rng1U64 range = {contrib_ptr->voff_first, contrib_ptr->voff_opl};
        rdim_rng1u64_list_push(arena, &unit->voff_ranges, range);
      }
    }
    
    ////////////////////////////
    //- rjf: pass 3: parse all inlinee line tables
    //
    out->units_first_inline_site_line_tables = push_array(arena, RDIM_LineTable *, in->comp_units->count);
    ProfScope("pass 3: parse all inlinee line tables")
      for(U64 comp_unit_idx = 0; comp_unit_idx < in->comp_units->count; comp_unit_idx += 1)
    {
      CV_SymParsed *unit_sym = in->comp_unit_syms[comp_unit_idx];
      CV_C13Parsed *unit_c13 = in->comp_unit_c13s[comp_unit_idx];
      CV_RecRange *rec_ranges_first = unit_sym->sym_ranges.ranges;
      CV_RecRange *rec_ranges_opl   = rec_ranges_first+unit_sym->sym_ranges.count;
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
                if((last_file_off != max_U32 && last_file_off != curr_file_off))
                {
                  String8 seq_file_name = {0};
                  
                  if(last_file_off + sizeof(CV_C13Checksum) <= file_chksms->size)
                  {
                    CV_C13Checksum *checksum = (CV_C13Checksum*)(unit_c13->data.str + file_chksms->off + last_file_off);
                    U32             name_off = checksum->name_off;
                    seq_file_name = pdb_strtbl_string_from_off(in->pdb_strtbl, name_off);
                  }
                  
                  // rjf: file name -> normalized file path
                  String8 file_path            = seq_file_name;
                  String8 file_path_normalized = lower_from_str8(scratch.arena, str8_skip_chop_whitespace(file_path));
                  for(U64 idx = 0; idx < file_path_normalized.size; idx += 1)
                  {
                    if(file_path_normalized.str[idx] == '\\')
                    {
                      file_path_normalized.str[idx] = '/';
                    }
                  }
                  
                  // rjf: normalized file path -> source file node
                  U64              file_path_normalized_hash = rdi_hash(file_path_normalized.str, file_path_normalized.size);
                  U64              src_file_slot             = file_path_normalized_hash%src_file_map.slots_count;
                  P2R_SrcFileNode *src_file_node             = 0;
                  for(P2R_SrcFileNode *n = src_file_map.slots[src_file_slot]; n != 0; n = n->next)
                  {
                    if(str8_match(n->src_file->normal_full_path, file_path_normalized, 0))
                    {
                      src_file_node = n;
                      break;
                    }
                  }
                  if(src_file_node == 0)
                  {
                    src_file_node = push_array(scratch.arena, P2R_SrcFileNode, 1);
                    SLLStackPush(src_file_map.slots[src_file_slot], src_file_node);
                    src_file_node->src_file                   = rdim_src_file_chunk_list_push(arena, &out->src_files, 4096);
                    src_file_node->src_file->normal_full_path = push_str8_copy(arena, file_path_normalized);
                  }
                  
                  // rjf: gather all lines
                  RDI_U64 *voffs      = push_array_no_zero(arena, RDI_U64, total_line_chunk_line_count+1);
                  RDI_U32 *line_nums  = push_array_no_zero(arena, RDI_U32, total_line_chunk_line_count);
                  RDI_U64  line_count = total_line_chunk_line_count;
                  {
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
                      line_table = rdim_line_table_chunk_list_push(arena, &out->line_tables, 256);
                      if(out->units_first_inline_site_line_tables[comp_unit_idx] == 0)
                      {
                        out->units_first_inline_site_line_tables[comp_unit_idx] = line_table;
                      }
                    }
                    RDIM_LineSequence *seq = rdim_line_table_push_sequence(arena, &out->line_tables, line_table, src_file_node->src_file, voffs, line_nums, 0, line_count);
                    rdim_src_file_push_line_sequence(arena, &out->src_files, src_file_node->src_file, seq);
                  }
                  
                  // rjf: clear line chunks for subsequent sequences
                  first_line_chunk            = last_line_chunk = 0;
                  total_line_chunk_line_count = 0;
                }
                
                if(step.flags & CV_C13InlineSiteDecoderStepFlag_EmitLine)
                {
                  LineChunk *chunk = last_line_chunk;
                  if(chunk == 0 || chunk->count+1 >= chunk->cap)
                  {
                    chunk = push_array(scratch.arena, LineChunk, 1);
                    SLLQueuePush(first_line_chunk, last_line_chunk, chunk);
                    chunk->cap       = 256;
                    chunk->voffs     = push_array_no_zero(scratch.arena, U64, chunk->cap);
                    chunk->line_nums = push_array_no_zero(scratch.arena, U32, chunk->cap);
                  }
                  chunk->voffs[chunk->count]     = step.line_voff;
                  chunk->voffs[chunk->count+1]   = step.line_voff_end;
                  chunk->line_nums[chunk->count] = step.ln;
                  chunk->count                  += 1;
                  total_line_chunk_line_count   += 1;
                }
                
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
  }
  scratch_end(scratch);
  ProfEnd();
  return out;
}

////////////////////////////////
//~ rjf: Link Name Map Building Tasks

ASYNC_WORK_DEF(p2r_link_name_map_build_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  P2R_LinkNameMapBuildIn *in = (P2R_LinkNameMapBuildIn *)input;
  CV_RecRange *rec_ranges_first = in->sym->sym_ranges.ranges;
  CV_RecRange *rec_ranges_opl   = rec_ranges_first + in->sym->sym_ranges.count;
  for(CV_RecRange *rec_range = rec_ranges_first;
      rec_range < rec_ranges_opl;
      rec_range += 1)
  {
    //- rjf: unpack symbol range info
    CV_SymKind kind = rec_range->hdr.kind;
    U64 header_struct_size = cv_header_struct_size_from_sym_kind(kind);
    U8 *sym_first = in->sym->data.str + rec_range->off + 2;
    U8 *sym_opl   = sym_first + rec_range->hdr.size;
    
    //- rjf: skip bad ranges
    if(sym_opl > in->sym->data.str + in->sym->data.size || sym_first + header_struct_size > in->sym->data.str + in->sym->data.size)
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
        COFF_SectionHeader *section = (0 < pub32->sec && pub32->sec <= in->coff_sections.count) ? &in->coff_sections.v[pub32->sec-1] : 0;
        U64 voff = 0;
        if(section != 0)
        {
          voff = section->voff + pub32->off;
        }
        
        // rjf: commit to link name map
        U64 hash = p2r_hash_from_voff(voff);
        U64 bucket_idx = hash%in->link_name_map->buckets_count;
        P2R_LinkNameNode *node = push_array(arena, P2R_LinkNameNode, 1);
        SLLStackPush(in->link_name_map->buckets[bucket_idx], node);
        node->voff = voff;
        node->name = name;
        in->link_name_map->link_name_count += 1;
        in->link_name_map->bucket_collision_count += (node->next != 0);
      }break;
    }
  }
  ProfEnd();
  return 0;
}

////////////////////////////////
//~ rjf: UDT Conversion Tasks

ASYNC_WORK_DEF(p2r_udt_convert_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  P2R_UDTConvertIn *in = (P2R_UDTConvertIn *)input;
#define p2r_type_ptr_from_itype(itype) ((in->itype_type_ptrs && (itype) < in->tpi_leaf->itype_opl) ? (in->itype_type_ptrs[itype]) : 0)
  RDIM_UDTChunkList *udts = push_array(arena, RDIM_UDTChunkList, 1);
  RDI_U64 udts_chunk_cap = 1024;
  ProfScope("convert UDT info")
  {
    for(CV_TypeId itype = in->itype_first; itype < in->itype_opl; itype += 1)
    {
      //- rjf: skip basics
      if(itype < in->tpi_leaf->itype_first) { continue; }
      
      //- rjf: grab type for this itype - skip if empty
      RDIM_Type *dst_type = in->itype_type_ptrs[itype];
      if(dst_type == 0) { continue; }
      
      //- rjf: unpack itype leaf range - skip if out-of-range
      CV_RecRange *range = &in->tpi_leaf->leaf_ranges.ranges[itype-in->tpi_leaf->itype_first];
      CV_LeafKind kind = range->hdr.kind;
      U64 header_struct_size = cv_header_struct_size_from_leaf_kind(kind);
      U8 *itype_leaf_first = in->tpi_leaf->data.str + range->off+2;
      U8 *itype_leaf_opl   = itype_leaf_first + range->hdr.size-2;
      if(range->off+range->hdr.size > in->tpi_leaf->data.size ||
         range->off+2+header_struct_size > in->tpi_leaf->data.size ||
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
            if(field_list_itype < in->tpi_leaf->itype_first || in->tpi_leaf->itype_opl <= field_list_itype)
            {
              continue;
            }
            
            //- rjf: field list itype -> range
            CV_RecRange *range = &in->tpi_leaf->leaf_ranges.ranges[field_list_itype-in->tpi_leaf->itype_first];
            
            //- rjf: skip bad headers
            if(range->off+range->hdr.size > in->tpi_leaf->data.size ||
               range->hdr.size < 2 ||
               range->hdr.kind != CV_LeafKind_FIELDLIST)
            {
              continue;
            }
            
            //- rjf: loop over all fields
            {
              U8 *field_list_first = in->tpi_leaf->data.str+range->off+2;
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                    mem->kind = RDI_MemberKind_DataField;
                    mem->name = name;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
                    mem->off  = (U32)offset64;
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                    mem->kind = RDI_MemberKind_StaticData;
                    mem->name = name;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
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
                    CV_RecRange *method_list_range = &in->tpi_leaf->leaf_ranges.ranges[lf->list_itype-in->tpi_leaf->itype_first];
                    
                    //- rjf: skip bad method lists
                    if(method_list_range->off+method_list_range->hdr.size > in->tpi_leaf->data.size ||
                       method_list_range->hdr.size < 2 ||
                       method_list_range->hdr.kind != CV_LeafKind_METHODLIST)
                    {
                      break;
                    }
                    
                    //- rjf: loop through all methods & emit members
                    U8 *method_list_first = in->tpi_leaf->data.str + method_list_range->off + 2;
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
                          RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                          mem->kind = RDI_MemberKind_Method;
                          mem->name = name;
                          mem->type = method_type;
                        }break;
                        case CV_MethodProp_Static:
                        {
                          RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                          mem->kind = RDI_MemberKind_StaticMethod;
                          mem->name = name;
                          mem->type = method_type;
                        }break;
                        case CV_MethodProp_Virtual:
                        case CV_MethodProp_PureVirtual:
                        case CV_MethodProp_Intro:
                        case CV_MethodProp_PureIntro:
                        {
                          RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                          mem->kind = RDI_MemberKind_VirtualMethod;
                          mem->name = name;
                          mem->type = method_type;
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
                        RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                        mem->kind = RDI_MemberKind_Method;
                        mem->name = name;
                        mem->type = method_type;
                      }break;
                      
                      case CV_MethodProp_Static:
                      {
                        RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                        mem->kind = RDI_MemberKind_StaticMethod;
                        mem->name = name;
                        mem->type = method_type;
                      }break;
                      
                      case CV_MethodProp_Virtual:
                      case CV_MethodProp_PureVirtual:
                      case CV_MethodProp_Intro:
                      case CV_MethodProp_PureIntro:
                      {
                        RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                        mem->kind = RDI_MemberKind_VirtualMethod;
                        mem->name = name;
                        mem->type = method_type;
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                    mem->kind = RDI_MemberKind_NestedType;
                    mem->name = name;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                    mem->kind = RDI_MemberKind_NestedType;
                    mem->name = name;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                    mem->kind = RDI_MemberKind_Base;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
                    mem->off  = (U32)offset64;
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, udts, dst_udt);
                    mem->kind = RDI_MemberKind_VirtualBase;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
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
            if(field_list_itype < in->tpi_leaf->itype_first || in->tpi_leaf->itype_opl <= field_list_itype)
            {
              continue;
            }
            
            //- rjf: field list itype -> range
            CV_RecRange *range = &in->tpi_leaf->leaf_ranges.ranges[field_list_itype-in->tpi_leaf->itype_first];
            
            //- rjf: skip bad headers
            if(range->off+range->hdr.size > in->tpi_leaf->data.size ||
               range->hdr.size < 2 ||
               range->hdr.kind != CV_LeafKind_FIELDLIST)
            {
              continue;
            }
            
            //- rjf: loop over all fields
            {
              U8 *field_list_first = in->tpi_leaf->data.str+range->off+2;
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
                    RDIM_UDTEnumVal *enum_val = rdim_udt_push_enum_val(arena, udts, dst_udt);
                    enum_val->name = name;
                    enum_val->val  = val64;
                  }break;
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
  ProfEnd();
  return udts;
}

////////////////////////////////
//~ rjf: Symbol Stream Conversion Path & Thread

ASYNC_WORK_DEF(p2r_symbol_stream_convert_work)
{
  ProfBeginFunction();
  Arena *arena = g_p2r_local_state->work_thread_arenas[thread_idx];
  Temp scratch = scratch_begin(&arena, 1);
  P2R_SymbolStreamConvertIn *in = (P2R_SymbolStreamConvertIn *)input;
#define p2r_type_ptr_from_itype(itype) ((in->itype_type_ptrs && (itype) < in->tpi_leaf->itype_opl) ? (in->itype_type_ptrs[itype]) : 0)
  
  //////////////////////////
  //- rjf: set up outputs for this sym stream
  //
  U64 sym_procedures_chunk_cap       = 1024;
  U64 sym_global_variables_chunk_cap = 1024;
  U64 sym_thread_variables_chunk_cap = 1024;
  U64 sym_scopes_chunk_cap           = 1024;
  U64 sym_inline_sites_chunk_cap     = 1024;
  RDIM_SymbolChunkList     sym_procedures       = {0};
  RDIM_SymbolChunkList     sym_global_variables = {0};
  RDIM_SymbolChunkList     sym_thread_variables = {0};
  RDIM_ScopeChunkList      sym_scopes           = {0};
  RDIM_InlineSiteChunkList sym_inline_sites     = {0};
  
  //////////////////////////
  //- rjf: symbols pass 1: produce procedure frame info map (procedure -> frame info)
  //
  U64 procedure_frameprocs_count = 0;
  U64 procedure_frameprocs_cap   = (in->sym_ranges_opl - in->sym_ranges_first);
  CV_SymFrameproc **procedure_frameprocs = push_array_no_zero(scratch.arena, CV_SymFrameproc *, procedure_frameprocs_cap);
  ProfScope("symbols pass 1: produce procedure frame info map (procedure -> frame info)")
  {
    U64 procedure_num = 0;
    CV_RecRange *rec_ranges_first = in->sym->sym_ranges.ranges + in->sym_ranges_first;
    CV_RecRange *rec_ranges_opl   = in->sym->sym_ranges.ranges + in->sym_ranges_opl;
    for(CV_RecRange *rec_range = rec_ranges_first;
        rec_range < rec_ranges_opl;
        rec_range += 1)
    {
      //- rjf: rec range -> symbol info range
      U64 sym_off_first = rec_range->off + 2;
      U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
      
      //- rjf: skip invalid ranges
      if(sym_off_opl > in->sym->data.size || sym_off_first > in->sym->data.size || sym_off_first > sym_off_opl)
      {
        continue;
      }
      
      //- rjf: unpack symbol info
      CV_SymKind kind = rec_range->hdr.kind;
      U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
      void *sym_header_struct_base = in->sym->data.str + sym_off_first;
      
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
    CV_RecRange *rec_ranges_first = in->sym->sym_ranges.ranges + in->sym_ranges_first;
    CV_RecRange *rec_ranges_opl   = in->sym->sym_ranges.ranges + in->sym_ranges_opl;
    typedef struct P2R_ScopeNode P2R_ScopeNode;
    struct P2R_ScopeNode
    {
      P2R_ScopeNode *next;
      RDIM_Scope *scope;
    };
    P2R_ScopeNode *top_scope_node = 0;
    P2R_ScopeNode *free_scope_node = 0;
    RDIM_LineTable *inline_site_line_table = in->first_inline_site_line_table;
    for(CV_RecRange *rec_range = rec_ranges_first;
        rec_range < rec_ranges_opl;
        rec_range += 1)
    {
      //- rjf: rec range -> symbol info range
      U64 sym_off_first = rec_range->off + 2;
      U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
      
      //- rjf: skip invalid ranges
      if(sym_off_opl > in->sym->data.size || sym_off_first > in->sym->data.size || sym_off_first > sym_off_opl)
      {
        continue;
      }
      
      //- rjf: unpack symbol info
      CV_SymKind kind = rec_range->hdr.kind;
      U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
      void *sym_header_struct_base = in->sym->data.str + sym_off_first;
      void *sym_data_opl = in->sym->data.str + sym_off_opl;
      
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
            COFF_SectionHeader *section = (0 < block32->sec && block32->sec <= in->coff_sections.count) ? &in->coff_sections.v[block32->sec-1] : 0;
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
          COFF_SectionHeader *section = (0 < data32->sec && data32->sec <= in->coff_sections.count) ? &in->coff_sections.v[data32->sec-1] : 0;
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
              CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(in->tpi_hash, in->tpi_leaf, container_name, 0);
              container_type = p2r_type_ptr_from_itype(cv_type_id);
            }
            
            // rjf: unpack global's container symbol
            RDIM_Symbol *container_symbol = 0;
            if(container_type == 0 && top_scope_node != 0)
            {
              container_symbol = top_scope_node->scope->symbol;
            }
            
            // form a VOFF location
#if 0
            RDIM_LocationSet locset = {0};
            RDIM_Location *voff_loc = rdim_push_location_voff(arena, voff);
            rdim_location_set_push_case(arena, &locset, (RDIM_Rng1U64){0,max_U64}, voff_loc);
#endif
            
            // rjf: build symbol
            RDIM_Symbol *symbol = rdim_symbol_chunk_list_push(arena, &sym_global_variables, sym_global_variables_chunk_cap);
            symbol->is_extern        = (kind == CV_SymKind_GDATA32);
            symbol->name             = name;
            symbol->type             = type;
            //symbol->locset           = locset;
            symbol->container_symbol = container_symbol;
            symbol->container_type   = container_type;
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
          if(container_name_opl > 2 && in->tpi_hash != 0 && in->tpi_leaf != 0)
          {
            String8 container_name = str8(name.str, container_name_opl - 2);
            CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(in->tpi_hash, in->tpi_leaf, container_name, 0);
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
            COFF_SectionHeader *section = (0 < proc32->sec && proc32->sec <= in->coff_sections.count) ? &in->coff_sections.v[proc32->sec-1] : 0;
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
            U64 bucket_idx = hash%in->link_name_map->buckets_count;
            P2R_LinkNameNode *node = 0;
            for(P2R_LinkNameNode *n = in->link_name_map->buckets[bucket_idx]; n != 0; n = n->next)
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
            switch(in->arch)
            {
              default:{}break;
              case RDI_Arch_X86:{is_stack_reg = (cv_reg == CV_Regx86_ESP);}break;
              case RDI_Arch_X64:{is_stack_reg = (cv_reg == CV_Regx64_RSP);}break;
            }
            if(is_stack_reg)
            {
              U32 frame_size = 0xFFFFFFFF;
              if(procedure_num != 0 && procedure_frameprocs[procedure_num-1] != 0 && procedure_num < procedure_frameprocs_count)
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
            switch(in->arch)
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
            RDI_RegCode reg_code = cv2r_rdi_reg_code_from_cv_reg_code(in->arch, cv_reg);
            // TODO(rjf): real byte_size & byte_pos from cv_reg goes here
            U32 byte_size = 8;
            U32 byte_pos = 0;
            
            // rjf: set location case
            RDIM_Location *loc = p2r_location_from_addr_reg_off(arena, in->arch, reg_code, byte_size, byte_pos, (S64)(S32)var_off, extra_indirection_to_value);
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
            CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(in->tpi_hash, in->tpi_leaf, container_name, 0);
            container_type = p2r_type_ptr_from_itype(cv_type_id);
          }
          
          // rjf: unpack thread variable's container symbol
          RDIM_Symbol *container_symbol = 0;
          if(container_type == 0 && top_scope_node != 0)
          {
            container_symbol = top_scope_node->scope->symbol;
          }
          
          // form TLS OFF location
#if 0
          RDIM_LocationSet locset = {0};
          RDIM_Location *tls_off_loc = rdim_push_location_tls_off(arena, tls_off);
          rdim_location_set_push_case(arena, &locset, (RDIM_Rng1U64){0,max_U64}, tls_off_loc);
#endif
          
          // rjf: build symbol
          RDIM_Symbol *tvar = rdim_symbol_chunk_list_push(arena, &sym_thread_variables, sym_thread_variables_chunk_cap);
          tvar->name             = name;
          tvar->type             = type;
          tvar->is_extern        = (kind == CV_SymKind_GTHREAD32);
          //tvar->locset           = locset;
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
        
        //- rjf: DEFRANGE_REGISTESR
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
          COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= in->coff_sections.count) ? &in->coff_sections.v[range->sec-1] : 0;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register+1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          RDI_RegCode reg_code = cv2r_rdi_reg_code_from_cv_reg_code(in->arch, cv_reg);
          
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
          COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= in->coff_sections.count) ? &in->coff_sections.v[range->sec-1] : 0;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_fprel + 1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          
          // rjf: select frame pointer register
          CV_EncodedFramePtrReg encoded_fp_reg = cv_pick_fp_encoding(frameproc, defrange_target_is_param);
          RDI_RegCode fp_register_code = cv2r_reg_code_from_arch_encoded_fp_reg(in->arch, encoded_fp_reg);
          
          // rjf: build location
          B32 extra_indirection = 0;
          U32 byte_size = rdi_addr_size_from_arch(in->arch);
          U32 byte_pos = 0;
          S64 var_off = (S64)defrange_fprel->off;
          RDIM_Location *location = p2r_location_from_addr_reg_off(arena, in->arch, fp_register_code, byte_size, byte_pos, var_off, extra_indirection);
          
          // rjf: emit locations over ranges
          p2r_location_over_lvar_addr_range(arena, &sym_scopes, defrange_target, location, range, range_section, gaps, gap_count);
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
          COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= in->coff_sections.count) ? &in->coff_sections.v[range->sec-1] : 0;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_subfield_register + 1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          RDI_RegCode reg_code = cv2r_rdi_reg_code_from_cv_reg_code(in->arch, cv_reg);
          
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
          RDI_RegCode fp_register_code = cv2r_reg_code_from_arch_encoded_fp_reg(in->arch, encoded_fp_reg);
          
          // rjf: build location
          B32 extra_indirection = 0;
          U32 byte_size = rdi_addr_size_from_arch(in->arch);
          U32 byte_pos = 0;
          S64 var_off = (S64)defrange_fprel_full_scope->off;
          RDIM_Location *location = p2r_location_from_addr_reg_off(arena, in->arch, fp_register_code, byte_size, byte_pos, var_off, extra_indirection);
          
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
          RDI_RegCode reg_code = cv2r_rdi_reg_code_from_cv_reg_code(in->arch, cv_reg);
          CV_LvarAddrRange *range = &defrange_register_rel->range;
          COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= in->coff_sections.count) ? &in->coff_sections.v[range->sec-1] : 0;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register_rel + 1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          
          // rjf: build location
          // TODO(rjf): offset & size from cv_reg code
          U32 byte_size = rdi_addr_size_from_arch(in->arch);
          U32 byte_pos = 0;
          B32 extra_indirection_to_value = 0;
          S64 var_off = defrange_register_rel->reg_off;
          RDIM_Location *location = p2r_location_from_addr_reg_off(arena, in->arch, reg_code, byte_size, byte_pos, var_off, extra_indirection_to_value);
          
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
          if(in->ipi_leaf != 0 && in->ipi_leaf->itype_first <= sym->inlinee && sym->inlinee < in->ipi_leaf->itype_opl)
          {
            CV_RecRange rec_range = in->ipi_leaf->leaf_ranges.ranges[sym->inlinee - in->ipi_leaf->itype_first];
            String8     rec_data  = str8_substr(in->ipi_leaf->data, rng_1u64(rec_range.off, rec_range.off + rec_range.hdr.size));
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
      }
    }
  }
  
  //////////////////////////
  //- rjf: allocate & fill output
  //
  P2R_SymbolStreamConvertOut *out = push_array(arena, P2R_SymbolStreamConvertOut, 1);
  {
    out->procedures       = sym_procedures;
    out->global_variables = sym_global_variables;
    out->thread_variables = sym_thread_variables;
    out->scopes           = sym_scopes;
    out->inline_sites     = sym_inline_sites;
  }
  
#undef p2r_type_ptr_from_itype
  scratch_end(scratch);
  ProfEnd();
  return out;
}

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal RDIM_BakeParams *
p2r_convert(Arena *arena, RDIM_LocalState *local_state, RC_Context *in)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  g_p2r_local_state = local_state;
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse MSF structure
  //
  MSF_Parsed *msf = 0;
  if(in->debug_data.size != 0) ProfScope("parse MSF structure")
  {
    msf = msf_parsed_from_data(arena, in->debug_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse PDB auth_guid & named streams table
  //
  PDB_NamedStreamTable *named_streams = 0;
  Guid auth_guid = {0};
  if(msf != 0) ProfScope("parse PDB auth_guid & named streams table")
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 info_data = msf_data_from_stream(msf, PDB_FixedStream_Info);
    PDB_Info *info = pdb_info_from_data(scratch.arena, info_data);
    named_streams = pdb_named_stream_table_from_info(arena, info);
    MemoryCopyStruct(&auth_guid, &info->auth_guid);
    scratch_end(scratch);
    
    if (info->features & PDB_FeatureFlag_MINIMAL_DBG_INFO) {
      fprintf(stderr, "ERROR: PDB was linked with /DEBUG:FASTLINK (partial debug info is not supported). Please relink using /DEBUG:FULL.");
      os_abort(1);
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse PDB strtbl
  //
  PDB_Strtbl *strtbl = 0;
  String8 raw_strtbl = str8_zero();
  if(named_streams != 0) ProfScope("parse PDB strtbl")
  {
    MSF_StreamNumber strtbl_sn = named_streams->sn[PDB_NamedStream_StringTable];
    String8 strtbl_data = msf_data_from_stream(msf, strtbl_sn);
    strtbl = pdb_strtbl_from_data(arena, strtbl_data);
    raw_strtbl = str8_substr(strtbl_data, rng_1u64(strtbl->strblock_min, strtbl->strblock_max));
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse dbi
  //
  PDB_DbiParsed *dbi = 0;
  if(msf != 0) ProfScope("parse dbi")
  {
    String8 dbi_data = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
    dbi = pdb_dbi_from_data(arena, dbi_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse tpi
  //
  PDB_TpiParsed *tpi = 0;
  if(msf != 0) ProfScope("parse tpi")
  {
    String8 tpi_data = msf_data_from_stream(msf, PDB_FixedStream_Tpi);
    tpi = pdb_tpi_from_data(arena, tpi_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse ipi
  //
  PDB_TpiParsed *ipi = 0;
  if(msf != 0) ProfScope("parse ipi")
  {
    String8 ipi_data = msf_data_from_stream(msf, PDB_FixedStream_Ipi);
    ipi = pdb_tpi_from_data(arena, ipi_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse coff sections
  //
  COFF_SectionHeaderArray coff_sections = {0};
  if(dbi != 0) ProfScope("parse coff sections")
  {
    MSF_StreamNumber section_stream = dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER];
    String8 section_data = msf_data_from_stream(msf, section_stream);
    coff_sections = pdb_coff_section_array_from_data(arena, section_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse gsi
  //
  PDB_GsiParsed *gsi = 0;
  if(dbi != 0) ProfScope("parse gsi")
  {
    String8 gsi_data = msf_data_from_stream(msf, dbi->gsi_sn);
    gsi = pdb_gsi_from_data(arena, gsi_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse psi
  //
  PDB_GsiParsed *psi_gsi_part = 0;
  if(dbi != 0) ProfScope("parse psi")
  {
    String8 psi_data = msf_data_from_stream(msf, dbi->psi_sn);
    String8 psi_data_gsi_part = str8_range(psi_data.str + sizeof(PDB_PsiHeader), psi_data.str + psi_data.size);
    psi_gsi_part = pdb_gsi_from_data(arena, psi_data_gsi_part);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: kickoff EXE hash
  //
  P2R_EXEHashIn exe_hash_in = {in->image_data};
  ASYNC_Task *exe_hash_task = async_task_launch(scratch.arena, p2r_exe_hash_work, .input = &exe_hash_in);
  
  //////////////////////////////////////////////////////////////
  //- rjf: kickoff TPI hash parse
  //
  P2R_TPIHashParseIn tpi_hash_in = {0};
  ASYNC_Task *tpi_hash_task = 0;
  if(tpi != 0)
  {
    tpi_hash_in.strtbl    = strtbl;
    tpi_hash_in.tpi       = tpi;
    tpi_hash_in.hash_data = msf_data_from_stream(msf, tpi->hash_sn);
    tpi_hash_in.aux_data  = msf_data_from_stream(msf, tpi->hash_sn_aux);
    tpi_hash_task = async_task_launch(scratch.arena, p2r_tpi_hash_parse_work, .input = &tpi_hash_in);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: kickoff TPI leaf parse
  //
  P2R_TPILeafParseIn tpi_leaf_in = {0};
  ASYNC_Task *tpi_leaf_task = 0;
  if(tpi != 0)
  {
    tpi_leaf_in.leaf_data   = pdb_leaf_data_from_tpi(tpi);
    tpi_leaf_in.itype_first = tpi->itype_first;
    tpi_leaf_task = async_task_launch(scratch.arena, p2r_tpi_leaf_parse_work, .input = &tpi_leaf_in);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: kickoff IPI hash parse
  //
  P2R_TPIHashParseIn ipi_hash_in = {0};
  ASYNC_Task *ipi_hash_task = 0;
  if(ipi != 0)
  {
    ipi_hash_in.strtbl    = strtbl;
    ipi_hash_in.tpi       = ipi;
    ipi_hash_in.hash_data = msf_data_from_stream(msf, ipi->hash_sn);
    ipi_hash_in.aux_data  = msf_data_from_stream(msf, ipi->hash_sn_aux);
    ipi_hash_task = async_task_launch(scratch.arena, p2r_tpi_hash_parse_work, .input = &ipi_hash_in);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: kickoff IPI leaf parse
  //
  P2R_TPILeafParseIn ipi_leaf_in = {0};
  ASYNC_Task *ipi_leaf_task = 0;
  if(ipi != 0)
  {
    ipi_leaf_in.leaf_data   = pdb_leaf_data_from_tpi(ipi);
    ipi_leaf_in.itype_first = ipi->itype_first;
    ipi_leaf_task = async_task_launch(scratch.arena, p2r_tpi_leaf_parse_work, .input = &ipi_leaf_in);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: kickoff top-level global symbol stream parse
  //
  P2R_SymbolStreamParseIn sym_parse_in = {dbi ? msf_data_from_stream(msf, dbi->sym_sn) : str8_zero()};
  ASYNC_Task *sym_parse_task = !dbi ? 0 : async_task_launch(scratch.arena, p2r_symbol_stream_parse_work, .input = &sym_parse_in);
  
  //////////////////////////////////////////////////////////////
  //- rjf: kickoff compilation unit parses
  //
  P2R_CompUnitParseIn comp_unit_parse_in = {dbi ? pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo) : str8_zero()};
  P2R_CompUnitContributionsParseIn comp_unit_contributions_parse_in = {dbi ? pdb_data_from_dbi_range(dbi, PDB_DbiRange_SecCon) : str8_zero(), coff_sections};
  ASYNC_Task *comp_unit_parse_task               = !dbi ? 0 : async_task_launch(scratch.arena, p2r_comp_unit_parse_work, .input = &comp_unit_parse_in);
  ASYNC_Task *comp_unit_contributions_parse_task = !dbi ? 0 : async_task_launch(scratch.arena, p2r_comp_unit_contributions_parse_work, .input = &comp_unit_contributions_parse_in);
  
  //////////////////////////////////////////////////////////////
  //- rjf: join compilation unit parses
  //
  PDB_CompUnitArray *comp_units = 0;
  U64 comp_unit_count = 0;
  PDB_CompUnitContributionArray *comp_unit_contributions = 0;
  U64 comp_unit_contribution_count = 0;
  {
    comp_units              =  async_task_join_struct(comp_unit_parse_task,               PDB_CompUnitArray);
    comp_unit_contributions =  async_task_join_struct(comp_unit_contributions_parse_task, PDB_CompUnitContributionArray);
    comp_unit_count = comp_units ? comp_units->count : 0;
    comp_unit_contribution_count = comp_unit_contributions ? comp_unit_contributions->count : 0;
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse syms & line info for each compilation unit
  //
  CV_SymParsed **sym_for_unit = push_array(arena, CV_SymParsed *, comp_unit_count);
  CV_C13Parsed **c13_for_unit = push_array(arena, CV_C13Parsed *, comp_unit_count);
  if(comp_units != 0) ProfScope("parse syms & line info for each compilation unit")
  {
    //- rjf: kick off tasks
    P2R_SymbolStreamParseIn *sym_tasks_inputs = push_array(scratch.arena, P2R_SymbolStreamParseIn, comp_unit_count);
    ASYNC_Task **sym_tasks = push_array(scratch.arena, ASYNC_Task *, comp_unit_count);
    P2R_C13StreamParseIn *c13_tasks_inputs = push_array(scratch.arena, P2R_C13StreamParseIn, comp_unit_count);
    ASYNC_Task **c13_tasks = push_array(scratch.arena, ASYNC_Task *, comp_unit_count);
    for(U64 idx = 0; idx < comp_unit_count; idx += 1)
    {
      PDB_CompUnit *unit = comp_units->units[idx];
      sym_tasks_inputs[idx].data = pdb_data_from_unit_range(msf, unit, PDB_DbiCompUnitRange_Symbols);
      sym_tasks[idx]             = async_task_launch(scratch.arena, p2r_symbol_stream_parse_work, .input = &sym_tasks_inputs[idx]);
      c13_tasks_inputs[idx].data          = pdb_data_from_unit_range(msf, unit, PDB_DbiCompUnitRange_C13);
      c13_tasks_inputs[idx].strtbl        = raw_strtbl;
      c13_tasks_inputs[idx].coff_sections = coff_sections;
      c13_tasks[idx]                      = async_task_launch(scratch.arena, p2r_c13_stream_parse_work, .input = &c13_tasks_inputs[idx]);
    }
    
    //- rjf: join tasks
    for(U64 idx = 0; idx < comp_unit_count; idx += 1)
    {
      sym_for_unit[idx] = async_task_join_struct(sym_tasks[idx], CV_SymParsed);
      c13_for_unit[idx] = async_task_join_struct(c13_tasks[idx], CV_C13Parsed);
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: determine architecture
  //
  RDI_Arch arch           = c2r_rdi_arch_from_coff_machine(dbi->machine_type);
  U64      arch_addr_size = rdi_addr_size_from_arch(arch);
  
  //////////////////////////////////////////////////////////////
  //- rjf: join EXE hash
  //
  U64 exe_hash = *async_task_join_struct(exe_hash_task, U64);
  
  //////////////////////////////////////////////////////////////
  //- rjf: build binary sections list
  //
  RDIM_BinarySectionList binary_sections = c2r_rdi_binary_sections_from_coff_sections(arena, str8_zero(), str8_zero(), coff_sections.count, coff_sections.v);
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce top-level-info
  //
  RDIM_TopLevelInfo top_level_info = rdim_make_top_level_info(in->image_name, arch_from_coff_machine(dbi->machine_type), exe_hash, binary_sections);
  
  //////////////////////////////////////////////////////////////
  //- rjf: kick off unit conversion & source file collection
  //
  P2R_UnitConvertIn unit_convert_in = {strtbl, coff_sections, comp_units, comp_unit_contributions, sym_for_unit, c13_for_unit};
  ASYNC_Task *unit_convert_task = async_task_launch(scratch.arena, p2r_units_convert_work, .input = &unit_convert_in);
  
  //////////////////////////////////////////////////////////////
  //- rjf: join global sym stream parse
  //
  CV_SymParsed *sym = async_task_join_struct(sym_parse_task, CV_SymParsed);
  
  //////////////////////////////
  //- rjf: predict symbol count
  //
  U64 symbol_count_prediction = 0;
  ProfScope("predict symbol count")
  {
    U64 rec_range_count = 0;
    if(sym != 0)
    {
      rec_range_count += sym->sym_ranges.count;
    }
    for(U64 comp_unit_idx = 0; comp_unit_idx < comp_unit_count; comp_unit_idx += 1)
    {
      CV_SymParsed *unit_sym = sym_for_unit[comp_unit_idx];
      rec_range_count += unit_sym->sym_ranges.count;
    }
    symbol_count_prediction = rec_range_count/8;
    if(symbol_count_prediction < 256)
    {
      symbol_count_prediction = 256;
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: kick off link name map production
  //
  P2R_LinkNameMap link_name_map__in_progress = {0};
  P2R_LinkNameMapBuildIn link_name_map_build_in = {0};
  ASYNC_Task *link_name_map_task = 0;
  if(sym != 0) ProfScope("kick off link name map build task")
  {
    link_name_map__in_progress.buckets_count = symbol_count_prediction;
    link_name_map__in_progress.buckets       = push_array(arena, P2R_LinkNameNode *, link_name_map__in_progress.buckets_count);
    link_name_map_build_in.sym = sym;
    link_name_map_build_in.coff_sections = coff_sections;
    link_name_map_build_in.link_name_map = &link_name_map__in_progress;
    link_name_map_task = async_task_launch(scratch.arena, p2r_link_name_map_build_work, .input = &link_name_map_build_in);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: join ipi/tpi hash/leaf parses
  //
  PDB_TpiHashParsed *tpi_hash = 0;
  CV_LeafParsed     *tpi_leaf = 0;
  PDB_TpiHashParsed *ipi_hash = 0;
  CV_LeafParsed     *ipi_leaf = 0;
  {
    tpi_hash = async_task_join_struct(tpi_hash_task, PDB_TpiHashParsed);
    tpi_leaf = async_task_join_struct(tpi_leaf_task, CV_LeafParsed);
    ipi_hash = async_task_join_struct(ipi_hash_task, PDB_TpiHashParsed);
    ipi_leaf = async_task_join_struct(ipi_leaf_task, CV_LeafParsed);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 1: construct all types from TPI
  //
  // this doesn't gather struct/class/union/enum members, which is done by
  // subsequent passes, to build RDI "UDT" information, which is distinct
  // from regular type info.
  //
  RDIM_Type         **itype_type_ptrs = 0;
  RDIM_TypeChunkList  all_types       = rdim_init_type_chunk_list(arena, arch);
#define p2r_type_ptr_from_itype(itype) (((itype) < tpi_leaf->itype_opl) ? itype_type_ptrs[itype] : 0)
  if(in->flags & RC_Flag_Types) ProfScope("types pass 1: construct all root/stub types from TPI")
  {
    itype_type_ptrs = push_array(arena, RDIM_Type *, tpi_leaf->itype_opl);
    
    //////////////////////////
    //- build basic types
    //
    {
      RDIM_DataModel data_model      = rdim_infer_data_model(OperatingSystem_Windows, top_level_info.arch);
      RDI_TypeKind   short_type      = rdim_short_type_from_data_model(data_model);
      RDI_TypeKind   long_type       = rdim_long_type_from_data_model(data_model);
      RDI_TypeKind   long_long_type  = rdim_long_long_type_from_data_model(data_model);
      RDI_TypeKind   ushort_type     = rdim_unsigned_short_type_from_data_model(data_model);
      RDI_TypeKind   ulong_type      = rdim_unsigned_long_type_from_data_model(data_model);
      RDI_TypeKind   ulong_long_type = rdim_unsigned_long_long_type_from_data_model(data_model);
      RDI_TypeKind   ptr_type        = rdim_pointer_size_t_type_from_data_model(data_model);
      
      struct
      {
        char *       name;
        RDI_TypeKind kind_rdi;
        CV_LeafKind  kind_cv;
        B32          make_pointer_near;
        B32          make_pointer_32;
        B32          make_pointer_64;
      }
      table[] =
      {
        { ""                     , RDI_TypeKind_NULL       , CV_BasicType_NOTYPE     , 0, 0, 0 },
        { "void"                 , RDI_TypeKind_Void       , CV_BasicType_VOID       , 1, 1, 1 },
        { "HRESULT"              , RDI_TypeKind_Handle     , CV_BasicType_HRESULT    , 0, 1, 1 },
        { "signed char"          , RDI_TypeKind_Char8      , CV_BasicType_CHAR       , 1, 1, 1 },
        { "short"                , short_type              , CV_BasicType_SHORT      , 1, 1, 1 },
        { "long"                 , long_type               , CV_BasicType_LONG       , 1, 1, 1 },
        { "long long"            , long_long_type          , CV_BasicType_QUAD       , 1, 1, 1 },
        { "__int128"             , RDI_TypeKind_S128       , CV_BasicType_OCT        , 1, 1, 1 }, // Clang type
        { "unsigned char"        , RDI_TypeKind_UChar8     , CV_BasicType_UCHAR      , 1, 1, 1 },
        { "unsigned short"       , ushort_type             , CV_BasicType_USHORT     , 1, 1, 1 },
        { "unsigned long"        , ulong_type              , CV_BasicType_ULONG      , 1, 1, 1 },
        { "unsigned long long"   , ulong_long_type         , CV_BasicType_UQUAD      , 1, 1, 1 },
        { "__uint128"            , RDI_TypeKind_U128       , CV_BasicType_UOCT       , 1, 1, 1 }, // Clang type
        { "bool"                 , RDI_TypeKind_S8         , CV_BasicType_BOOL8      , 1, 1, 1 },
        { "__bool16"             , RDI_TypeKind_S16        , CV_BasicType_BOOL16     , 1, 1, 1 }, // not real C type
        { "__bool32"             , RDI_TypeKind_S32        , CV_BasicType_BOOL32     , 1, 1, 1 }, // not real C type
        { "float"                , RDI_TypeKind_F32        , CV_BasicType_FLOAT32    , 1, 1, 1 },
        { "double"               , RDI_TypeKind_F64        , CV_BasicType_FLOAT64    , 1, 1, 1 },
        { "long double"          , RDI_TypeKind_F80        , CV_BasicType_FLOAT80    , 1, 1, 1 },
        { "__float128"           , RDI_TypeKind_F128       , CV_BasicType_FLOAT128   , 1, 1, 1 }, // Clang type
        { "__float48"            , RDI_TypeKind_F48        , CV_BasicType_FLOAT48    , 1, 1, 1 }, // not real C type
        { "__float32pp"          , RDI_TypeKind_F32PP      , CV_BasicType_FLOAT32PP  , 1, 1, 1 }, // not real C type
        { "_Complex float"       , RDI_TypeKind_ComplexF32 , CV_BasicType_COMPLEX32  , 0, 0, 0 },
        { "_Complex double"      , RDI_TypeKind_ComplexF64 , CV_BasicType_COMPLEX64  , 0, 0, 0 },
        { "_Complex long double" , RDI_TypeKind_ComplexF80 , CV_BasicType_COMPLEX80  , 0, 0, 0 },
        { "_Complex __float128"  , RDI_TypeKind_ComplexF128, CV_BasicType_COMPLEX128 , 0, 0, 0 },
        { "__int8"               , RDI_TypeKind_S8         , CV_BasicType_INT8       , 1, 1, 1 },
        { "__uint8"              , RDI_TypeKind_U8         , CV_BasicType_UINT8      , 1, 1, 1 },
        { "__int16"              , RDI_TypeKind_S16        , CV_BasicType_INT16      , 1, 1, 1 },
        { "__uint16"             , RDI_TypeKind_U16        , CV_BasicType_UINT16     , 1, 1, 1 },
        { "int"                  , RDI_TypeKind_S32        , CV_BasicType_INT32      , 1, 1, 1 },
        { "unsigned int"         , RDI_TypeKind_U32        , CV_BasicType_UINT32     , 1, 1, 1 },
        { "__int64"              , RDI_TypeKind_S64        , CV_BasicType_INT64      , 1, 1, 1 },
        { "__uint64"             , RDI_TypeKind_U64        , CV_BasicType_UINT64     , 1, 1, 1 },
        { "__int128"             , RDI_TypeKind_S128       , CV_BasicType_INT128     , 1, 1, 1 },
        { "__uint128"            , RDI_TypeKind_U128       , CV_BasicType_UINT128    , 1, 1, 1 },
        { "char"                 , RDI_TypeKind_Char8      , CV_BasicType_RCHAR      , 1, 1, 1 }, // always ASCII
        { "wchar_t"              , RDI_TypeKind_UChar16    , CV_BasicType_WCHAR      , 1, 1, 1 }, // on windows always UTF-16
        { "char8_t"              , RDI_TypeKind_Char8      , CV_BasicType_CHAR8      , 1, 1, 1 }, // always UTF-8
        { "char16_t"             , RDI_TypeKind_Char16     , CV_BasicType_CHAR16     , 1, 1, 1 }, // always UTF-16
        { "char32_t"             , RDI_TypeKind_Char32     , CV_BasicType_CHAR32     , 1, 1, 1 }, // always UTF-32
        { "__pointer"            , ptr_type                , CV_BasicType_PTR        , 0, 0, 0 }
      };
      
      for(U64 i = 0; i < ArrayCount(table); i += 1)
      {
        RDIM_Type *type   = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
        type->kind        = RDI_TypeKind_Alias;
        type->name        = str8_cstring(table[i].name);
        type->direct_type = rdim_builtin_type_from_kind(all_types, table[i].kind_rdi);
        itype_type_ptrs[table[i].kind_cv] = type;
        
        if(table[i].make_pointer_near)
        {
          CV_TypeIndex near_ptr_itype = table[i].kind_cv | 0x100;
          RDIM_Type *ptr_near    = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
          ptr_near->kind         = RDI_TypeKind_Ptr;
          ptr_near->byte_size    = 2;
          ptr_near->direct_type  = type;
          itype_type_ptrs[near_ptr_itype] = ptr_near;
        }
        if(table[i].make_pointer_32)
        {
          CV_TypeIndex ptr_32_itype = table[i].kind_cv | 0x400;
          RDIM_Type *ptr_32    = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
          ptr_32->kind         = RDI_TypeKind_Ptr;
          ptr_32->byte_size    = 4;
          ptr_32->direct_type  = type;
          itype_type_ptrs[ptr_32_itype] = ptr_32;
        }
        if(table[i].make_pointer_64)
        {
          CV_TypeIndex ptr_64_itype = table[i].kind_cv | 0x600;
          RDIM_Type *ptr_64    = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
          ptr_64->kind         = RDI_TypeKind_Ptr;
          ptr_64->byte_size    = 8;
          ptr_64->direct_type  = type;
          itype_type_ptrs[ptr_64_itype] = ptr_64;
        }
      }
    }
    
    //////////////////////////
    //- rjf: build complex type
    //
    for(CV_TypeId itype = tpi_leaf->itype_first; itype < tpi_leaf->itype_opl; itype += 1)
    {
      RDIM_Type   *dst_type           = 0;
      CV_RecRange *range              = &tpi_leaf->leaf_ranges.ranges[itype-tpi_leaf->itype_first];
      CV_LeafKind  kind               = range->hdr.kind;
      U64          header_struct_size = cv_header_struct_size_from_leaf_kind(kind);
      
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
              dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
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
              RDIM_Type *pointer_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
              dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
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
              dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
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
            dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
            dst_type->kind        = RDI_TypeKind_Function;
            dst_type->byte_size   = arch_addr_size;
            dst_type->direct_type = ret_type;
            
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
            CV_LeafArgList *arglist              = (CV_LeafArgList*)arglist_first;
            CV_TypeId      *arglist_itypes_base  = (CV_TypeId *)(arglist+1);
            U32             arglist_itypes_count = arglist->count;
            
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
            dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
            dst_type->kind        = (lf->this_itype != 0) ? RDI_TypeKind_Method : RDI_TypeKind_Function;
            dst_type->byte_size   = arch_addr_size;
            dst_type->direct_type = ret_type;
            
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
            dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
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
            dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
            dst_type->kind        = RDI_TypeKind_Array;
            dst_type->direct_type = direct_type;
            dst_type->byte_size   = full_size;
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
            dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
            if(lf->props & CV_TypeProp_FwdRef)
            {
              dst_type->kind = (kind == CV_LeafKind_CLASS ? RDI_TypeKind_IncompleteClass : RDI_TypeKind_IncompleteStruct);
            }
            else
            {
              dst_type->kind = (kind == CV_LeafKind_CLASS ? RDI_TypeKind_Class : RDI_TypeKind_Struct);
            }
            
            B32 do_unique_name_lookup = (((lf->props & CV_TypeProp_Scoped) != 0) &&
                                         ((lf->props & CV_TypeProp_HasUniqueName) != 0));
            if(do_unique_name_lookup)
            {
              U8 *unique_name_ptr = name_ptr + name.size + 1;
              dst_type->link_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
            }
            
            dst_type->name      = name;
            dst_type->byte_size = safe_cast_u32(size_u64);
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
            dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
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
            
            B32 do_unique_name_lookup = (((lf->props & CV_TypeProp_Scoped) != 0) &&
                                         ((lf->props & CV_TypeProp_HasUniqueName) != 0));
            if(do_unique_name_lookup)
            {
              U8 *unique_name_ptr = name_ptr + name.size + 1;
              dst_type->link_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
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
            dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
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
            
            B32 do_unique_name_lookup = (((lf->props & CV_TypeProp_Scoped) != 0) &&
                                         ((lf->props & CV_TypeProp_HasUniqueName) != 0));
            if(do_unique_name_lookup)
            {
              U8 *unique_name_ptr = name_ptr + name.size + 1;
              dst_type->link_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
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
            dst_type = rdim_type_chunk_list_push(arena, &all_types, tpi_leaf->itype_opl);
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
            
            B32 do_unique_name_lookup = (((lf->props & CV_TypeProp_Scoped) != 0) &&
                                         ((lf->props & CV_TypeProp_HasUniqueName) != 0));
            if(do_unique_name_lookup)
            {
              U8 *unique_name_ptr = name_ptr + name.size + 1;
              dst_type->link_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
            }
          }break;
        }
      }
      
      //- rjf: store finalized type to this itype's slot
      itype_type_ptrs[itype] = dst_type;
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 2: kick off UDT build
  //
  U64 udt_task_size_itypes = 4096;
  U64 udt_tasks_count = (tpi_leaf->itype_opl+(udt_task_size_itypes-1))/udt_task_size_itypes;
  P2R_UDTConvertIn *udt_tasks_inputs = push_array(scratch.arena, P2R_UDTConvertIn, udt_tasks_count);
  ASYNC_Task **udt_tasks = push_array(scratch.arena, ASYNC_Task *, udt_tasks_count);
  if(in->flags & RC_Flag_UDTs) ProfScope("types pass 2: kick off UDT build")
  {
    for(U64 idx = 0; idx < udt_tasks_count; idx += 1)
    {
      udt_tasks_inputs[idx].tpi_leaf        = tpi_leaf;
      udt_tasks_inputs[idx].itype_first     = idx*udt_task_size_itypes;
      udt_tasks_inputs[idx].itype_opl       = udt_tasks_inputs[idx].itype_first + udt_task_size_itypes;
      udt_tasks_inputs[idx].itype_opl       = ClampTop(udt_tasks_inputs[idx].itype_opl, tpi_leaf->itype_opl);
      udt_tasks_inputs[idx].itype_type_ptrs = itype_type_ptrs;
      udt_tasks[idx] = async_task_launch(scratch.arena, p2r_udt_convert_work, .input = &udt_tasks_inputs[idx]);
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: join link name map building task
  //
  P2R_LinkNameMap *link_name_map = 0;
  ProfScope("join link name map building task")
  {
    async_task_join(link_name_map_task);
    link_name_map = &link_name_map__in_progress;
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: join unit conversion & src file & line table tasks
  //
  RDIM_UnitChunkList all_units = {0};
  RDIM_SrcFileChunkList all_src_files = {0};
  RDIM_LineTableChunkList all_line_tables = {0};
  RDIM_LineTable **units_first_inline_site_line_tables = 0;
  ProfScope("join unit conversion & src file tasks")
  {
    P2R_UnitConvertOut *out = async_task_join_struct(unit_convert_task, P2R_UnitConvertOut);
    all_units = out->units;
    all_src_files = out->src_files;
    all_line_tables = out->line_tables;
    units_first_inline_site_line_tables = out->units_first_inline_site_line_tables;
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce symbols from all streams
  //
  RDIM_SymbolChunkList all_procedures = {0};
  RDIM_SymbolChunkList all_global_variables = {0};
  RDIM_SymbolChunkList all_thread_variables = {0};
  RDIM_ScopeChunkList all_scopes = {0};
  RDIM_InlineSiteChunkList all_inline_sites = {0};
  ProfScope("produce symbols from all streams")
  {
    ////////////////////////////
    //- rjf: kick off all symbol conversion tasks
    //
    U64 global_stream_subdivision_tasks_count = sym ? (sym->sym_ranges.count+16383)/16384 : 0;
    U64 global_stream_syms_per_task = sym ? sym->sym_ranges.count/global_stream_subdivision_tasks_count : 0;
    U64 tasks_count = comp_unit_count + global_stream_subdivision_tasks_count;
    P2R_SymbolStreamConvertIn *tasks_inputs = push_array(scratch.arena, P2R_SymbolStreamConvertIn, tasks_count);
    ASYNC_Task **tasks = push_array(scratch.arena, ASYNC_Task *, tasks_count);
    ProfScope("kick off all symbol conversion tasks")
    {
      for(U64 idx = 0; idx < tasks_count; idx += 1)
      {
        tasks_inputs[idx].arch                         = arch;
        tasks_inputs[idx].coff_sections                = coff_sections;
        tasks_inputs[idx].tpi_hash                     = tpi_hash;
        tasks_inputs[idx].tpi_leaf                     = tpi_leaf;
        tasks_inputs[idx].ipi_leaf                     = ipi_leaf;
        tasks_inputs[idx].itype_type_ptrs              = itype_type_ptrs;
        tasks_inputs[idx].link_name_map                = link_name_map;
        if(idx < global_stream_subdivision_tasks_count)
        {
          tasks_inputs[idx].sym             = sym;
          tasks_inputs[idx].sym_ranges_first= idx*global_stream_syms_per_task;
          tasks_inputs[idx].sym_ranges_opl  = tasks_inputs[idx].sym_ranges_first + global_stream_syms_per_task;
          tasks_inputs[idx].sym_ranges_opl  = ClampTop(tasks_inputs[idx].sym_ranges_opl, sym->sym_ranges.count);
        }
        else
        {
          tasks_inputs[idx].sym             = sym_for_unit[idx-global_stream_subdivision_tasks_count];
          tasks_inputs[idx].sym_ranges_first= 0;
          tasks_inputs[idx].sym_ranges_opl  = sym_for_unit[idx-global_stream_subdivision_tasks_count]->sym_ranges.count;
          tasks_inputs[idx].first_inline_site_line_table = units_first_inline_site_line_tables[idx-global_stream_subdivision_tasks_count];
        }
        tasks[idx] = async_task_launch(scratch.arena, p2r_symbol_stream_convert_work, .input = &tasks_inputs[idx]);
      }
    }
    
    ////////////////////////////
    //- rjf: join tasks, merge with top-level collections
    //
    ProfScope("join tasks, merge with top-level collections")
    {
      for(U64 idx = 0; idx < tasks_count; idx += 1)
      {
        P2R_SymbolStreamConvertOut *out = async_task_join_struct(tasks[idx], P2R_SymbolStreamConvertOut);
        rdim_symbol_chunk_list_concat_in_place(&all_procedures,       &out->procedures);
        rdim_symbol_chunk_list_concat_in_place(&all_global_variables, &out->global_variables);
        rdim_symbol_chunk_list_concat_in_place(&all_thread_variables, &out->thread_variables);
        rdim_scope_chunk_list_concat_in_place(&all_scopes,            &out->scopes);
        rdim_inline_site_chunk_list_concat_in_place(&all_inline_sites,&out->inline_sites);
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 5: join UDT build tasks
  //
  RDIM_UDTChunkList all_udts = {0};
  for(U64 idx = 0; idx < udt_tasks_count; idx += 1)
  {
    RDIM_UDTChunkList *udts = async_task_join_struct(udt_tasks[idx], RDIM_UDTChunkList);
    rdim_udt_chunk_list_concat_in_place(&all_udts, udts);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: fill output
  //
  RDIM_BakeParams *out = push_array(arena, RDIM_BakeParams, 1);
  {
    out->top_level_info   = top_level_info;
    out->binary_sections  = binary_sections;
    out->units            = all_units;
    out->types            = all_types;
    out->udts             = all_udts;
    out->src_files        = all_src_files;
    out->line_tables      = all_line_tables;
    out->global_variables = all_global_variables;
    out->thread_variables = all_thread_variables;
    out->procedures       = all_procedures;
    out->scopes           = all_scopes;
    out->inline_sites     = all_inline_sites;
  }
  
  scratch_end(scratch);
  return out;
}

////////////////////////////////

internal B32
p2r_has_symbol_ref(String8 msf_data, String8List symbol_list, MSF_RawStreamTable *st)
{
  Temp scratch = scratch_begin(0,0);
  
  B32 has_ref = 0;
  
  String8        dbi_data = msf_data_from_stream_number(scratch.arena, msf_data, st, PDB_FixedStream_Dbi);
  PDB_DbiParsed *dbi      = pdb_dbi_from_data(scratch.arena, dbi_data);
  if(dbi)
  {
    String8        gsi_data   = msf_data_from_stream_number(scratch.arena, msf_data, st, dbi->gsi_sn);
    PDB_GsiParsed *gsi_parsed = pdb_gsi_from_data(scratch.arena, gsi_data);
    if(gsi_parsed)
    {
      String8 symbol_data = msf_data_from_stream_number(scratch.arena, msf_data, st, dbi->sym_sn);
      
      for(String8Node *symbol_n = symbol_list.first; symbol_n != 0; symbol_n = symbol_n->next)
      {
        U64 symbol_off = pdb_gsi_symbol_from_string(gsi_parsed, symbol_data, symbol_n->string);
        if(symbol_off < symbol_data.size)
        {
          has_ref = 1;
          break;
        }
      }
    }
  }
  
  scratch_end(scratch);
  return has_ref;
}

internal B32
p2r_has_file_ref(String8 msf_data, String8List file_list, MSF_RawStreamTable *st)
{
  Temp scratch = scratch_begin(0,0);
  
  B32 has_ref = 0;
  
  String8   info_data = msf_data_from_stream_number(scratch.arena, msf_data, st, PDB_FixedStream_Info);
  PDB_Info *info      = pdb_info_from_data(scratch.arena, info_data);
  if(info)
  {
    PDB_NamedStreamTable *named_streams = pdb_named_stream_table_from_info(scratch.arena, info);
    if(named_streams)
    {
      MSF_StreamNumber  strtbl_sn   = named_streams->sn[PDB_NamedStream_StringTable];
      String8           strtbl_data = msf_data_from_stream_number(scratch.arena, msf_data, st, strtbl_sn);
      PDB_Strtbl       *strtbl      = pdb_strtbl_from_data(scratch.arena, strtbl_data);
      if(strtbl)
      {
        for(String8Node *file_n = file_list.first; file_n != 0; file_n = file_n->next)
        {
          U32 off = pdb_strtbl_off_from_string(strtbl, file_n->string);
          if(off != max_U32)
          {
            has_ref = 1;
            break;
          }
        }
      }
    }
  }
  
  scratch_end(scratch);
  return has_ref;
}

internal B32
p2r_has_symbol_or_file_ref(String8 msf_data, String8List symbol_list, String8List file_list)
{
  Temp scratch = scratch_begin(0,0);
  
  B32 has_ref = 0;
  
  MSF_RawStreamTable *st = msf_raw_stream_table_from_data(scratch.arena, msf_data);
  
  if(!has_ref && symbol_list.node_count)
  {
    has_ref = p2r_has_symbol_ref(msf_data, symbol_list, st);
  }
  
  if(!has_ref && file_list.node_count)
  {
    has_ref = p2r_has_file_ref(msf_data, file_list, st);
  }
  
  scratch_end(scratch);
  return has_ref;
}

