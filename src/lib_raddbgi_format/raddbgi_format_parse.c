// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ RADDBG Parse API

RDI_PROC RDI_ParseStatus
rdi_parse(RDI_U8 *data, RDI_U64 size, RDI_Parsed *out)
{
  RDI_ParseStatus result = RDI_ParseStatus_Good;
  
  // out header
  RDI_Header *hdr = 0;
  {
    if (sizeof(*hdr) <= size){
      hdr = (RDI_Header*)data;
    }
    
    //  (errors)
    if (hdr == 0 || hdr->magic != RDI_MAGIC_CONSTANT){
      hdr = 0;
      result = RDI_ParseStatus_HeaderDoesNotMatch;
    }
    if (hdr != 0 && hdr->encoding_version != 1){
      hdr = 0;
      result = RDI_ParseStatus_UnsupportedVersionNumber;
    }
  }
  
  // out data sections
  RDI_DataSection *dsecs = 0;
  RDI_U32 dsec_count = 0;
  if (hdr != 0){
    RDI_U64 opl = (RDI_U64)hdr->data_section_off + (RDI_U64)hdr->data_section_count*sizeof(*dsecs);
    if (opl <= size){
      dsecs = (RDI_DataSection*)(data + hdr->data_section_off);
      dsec_count = hdr->data_section_count;
    }
    
    //  (errors)
    if (dsecs == 0){
      result = RDI_ParseStatus_InvalidDataSecionLayout;
    }
  }
  
  // extract primary data section indexes
  RDI_U32 dsec_idx[RDI_DataSectionTag_PRIMARY_COUNT] = {0};
  if (result == RDI_ParseStatus_Good){
    RDI_DataSection *sec_ptr = dsecs;
    for (RDI_U32 i = 0; i < dsec_count; i += 1, sec_ptr += 1){
      if (sec_ptr->tag < RDI_DataSectionTag_PRIMARY_COUNT){
        dsec_idx[sec_ptr->tag] = i;
      }
    }
  }
  
  // fill out data block (part 1)
  if (result == RDI_ParseStatus_Good){
    out->raw_data = data;
    out->raw_data_size = size;
    out->dsecs = dsecs;
    out->dsec_count = dsec_count;
    for (RDI_U32 i = 0; i < RDI_DataSectionTag_PRIMARY_COUNT; i += 1){
      out->dsec_idx[i] = dsec_idx[i];
    }
  }
  
  // out string table
  RDI_U8 *string_data = 0;
  RDI_U64 string_opl = 0;
  RDI_U32 *string_offs = 0;
  RDI_U64 string_count = 0;
  if (result == RDI_ParseStatus_Good){
    rdi_parse__extract_primary(out, string_data, &string_opl,
                               RDI_DataSectionTag_StringData);
    
    RDI_U64 table_entry_count = 0;
    rdi_parse__extract_primary(out, string_offs, &table_entry_count,
                               RDI_DataSectionTag_StringTable);
    if (table_entry_count > 0){
      string_count = table_entry_count - 1;
    }
    
    //  (errors)
    if (string_data == 0){
      result = RDI_ParseStatus_MissingStringDataSection;
    }
    else if (string_offs == 0){
      result = RDI_ParseStatus_MissingStringTableSection;
    }
  }
  
  // out index runs
  RDI_U32 *idx_run_data = 0;
  RDI_U64 idx_run_count = 0;
  if (result == RDI_ParseStatus_Good){
    rdi_parse__extract_primary(out, idx_run_data, &idx_run_count,
                               RDI_DataSectionTag_IndexRuns);
    
    //  (errors)
    if (idx_run_data == 0){
      result = RDI_ParseStatus_MissingIndexRunSection;
    }
  }
  
  if (result == RDI_ParseStatus_Good){
    // fill out primary data structures (part 2)
    out->string_data = string_data;
    out->string_offs = string_offs;
    out->string_data_size = string_opl;
    out->string_count = string_count;
    out->idx_run_data = idx_run_data;
    out->idx_run_count = idx_run_count;
    
    {
      RDI_TopLevelInfo *tli = 0;
      RDI_U64 dummy = 0;
      rdi_parse__extract_primary(out, tli, &dummy, RDI_DataSectionTag_TopLevelInfo);
      if (dummy != 1){
        tli = 0;
      }
      out->top_level_info = tli;
    }
    
    rdi_parse__extract_primary(out, out->binary_sections, &out->binary_sections_count,
                               RDI_DataSectionTag_BinarySections);
    
    rdi_parse__extract_primary(out, out->file_paths, &out->file_paths_count,
                               RDI_DataSectionTag_FilePathNodes);
    
    rdi_parse__extract_primary(out, out->source_files, &out->source_files_count,
                               RDI_DataSectionTag_SourceFiles);
    
    rdi_parse__extract_primary(out, out->units, &out->units_count,
                               RDI_DataSectionTag_Units);
    
    rdi_parse__extract_primary(out, out->unit_vmap, &out->unit_vmap_count,
                               RDI_DataSectionTag_UnitVmap);
    
    rdi_parse__extract_primary(out, out->unit_vmap, &out->unit_vmap_count,
                               RDI_DataSectionTag_UnitVmap);
    
    rdi_parse__extract_primary(out, out->type_nodes, &out->type_nodes_count,
                               RDI_DataSectionTag_TypeNodes);
    
    rdi_parse__extract_primary(out, out->udts, &out->udts_count,
                               RDI_DataSectionTag_UDTs);
    
    rdi_parse__extract_primary(out, out->members, &out->members_count,
                               RDI_DataSectionTag_Members);
    
    rdi_parse__extract_primary(out, out->enum_members, &out->enum_members_count,
                               RDI_DataSectionTag_EnumMembers);
    
    rdi_parse__extract_primary(out, out->global_variables, &out->global_variables_count,
                               RDI_DataSectionTag_GlobalVariables);
    
    rdi_parse__extract_primary(out, out->global_vmap, &out->global_vmap_count,
                               RDI_DataSectionTag_GlobalVmap);
    
    rdi_parse__extract_primary(out, out->thread_variables, &out->thread_variables_count,
                               RDI_DataSectionTag_ThreadVariables);
    
    rdi_parse__extract_primary(out, out->procedures, &out->procedures_count,
                               RDI_DataSectionTag_Procedures);
    
    rdi_parse__extract_primary(out, out->scopes, &out->scopes_count,
                               RDI_DataSectionTag_Scopes);
    
    rdi_parse__extract_primary(out, out->scope_voffs, &out->scope_voffs_count,
                               RDI_DataSectionTag_ScopeVoffData);
    
    rdi_parse__extract_primary(out, out->scope_vmap, &out->scope_vmap_count,
                               RDI_DataSectionTag_ScopeVmap);
    
    rdi_parse__extract_primary(out, out->locals, &out->locals_count,
                               RDI_DataSectionTag_Locals);
    
    rdi_parse__extract_primary(out, out->location_blocks, &out->location_blocks_count,
                               RDI_DataSectionTag_LocationBlocks);
    
    rdi_parse__extract_primary(out, out->location_data, &out->location_data_size,
                               RDI_DataSectionTag_LocationData);
    
    {
      rdi_parse__extract_primary(out, out->name_maps, &out->name_maps_count,
                                 RDI_DataSectionTag_NameMaps);
      
      RDI_NameMap *name_map_ptr = out->name_maps;
      RDI_NameMap *name_map_opl = out->name_maps + out->name_maps_count;
      for (; name_map_ptr < name_map_opl; name_map_ptr += 1){
        if (out->name_maps_by_kind[name_map_ptr->kind] == 0){
          out->name_maps_by_kind[name_map_ptr->kind] = name_map_ptr;
        }
      }
    }
  }
  
#if !defined(RDI_DISABLE_NILS)
  if(out->top_level_info == 0)                 { out->top_level_info          = &rdi_top_level_info_nil; }
  if(out->binary_sections == 0)                { out->binary_sections        = &rdi_binary_section_nil;           out->binary_sections_count = 1; }
  if(out->file_paths == 0)                     { out->file_paths             = &rdi_file_path_node_nil;           out->file_paths_count = 1; }
  if(out->source_files == 0)                   { out->source_files           = &rdi_source_file_nil;              out->source_files_count = 1; }
  if(out->units == 0)                          { out->units                  = &rdi_unit_nil;                     out->units_count = 1; }
  if(out->unit_vmap == 0)                      { out->unit_vmap              = &rdi_vmap_entry_nil;               out->unit_vmap_count = 1; }
  if(out->type_nodes == 0)                     { out->type_nodes             = &rdi_type_node_nil;                out->type_nodes_count = 1; }
  if(out->udts == 0)                           { out->udts                   = &rdi_udt_nil;                      out->udts_count = 1; }
  if(out->members == 0)                        { out->members                = &rdi_member_nil;                   out->members_count = 1; }
  if(out->enum_members == 0)                   { out->enum_members           = &rdi_enum_member_nil;              out->enum_members_count = 1; }
  if(out->global_variables == 0)               { out->global_variables       = &rdi_global_variable_nil;          out->global_variables_count = 1; }
  if(out->global_vmap == 0)                    { out->global_vmap            = &rdi_vmap_entry_nil;               out->global_vmap_count = 1; }
  if(out->thread_variables == 0)               { out->thread_variables       = &rdi_thread_variable_nil;          out->thread_variables_count = 1; }
  if(out->procedures == 0)                     { out->procedures             = &rdi_procedure_nil;                out->procedures_count = 1; }
  if(out->scopes == 0)                         { out->scopes                 = &rdi_scope_nil;                    out->scopes_count = 1; }
  if(out->scope_voffs == 0)                    { out->scope_voffs            = &rdi_voff_nil;                     out->scope_voffs_count = 1; }
  if(out->scope_vmap == 0)                     { out->scope_vmap             = &rdi_vmap_entry_nil;               out->scope_vmap_count = 1; }
  if(out->locals == 0)                         { out->locals                 = &rdi_local_nil;                    out->locals_count = 1; }
  if(out->location_blocks == 0)                { out->location_blocks        = &rdi_location_block_nil;           out->location_blocks_count = 1; }
#endif
  
  return(result);
}

RDI_PROC RDI_U8*
rdi_string_from_idx(RDI_Parsed *parsed, RDI_U32 idx, RDI_U64 *len_out){
  RDI_U8 *result = 0;
  RDI_U64 len_result = 0;
  if (idx < parsed->string_count){
    RDI_U32 off_raw = parsed->string_offs[idx];
    RDI_U32 opl_raw = parsed->string_offs[idx + 1];
    RDI_U32 opl = rdi_parse__min(opl_raw, parsed->string_data_size);
    RDI_U32 off = rdi_parse__min(off_raw, opl);
    result = parsed->string_data + off;
    len_result = opl - off;
  }
  *len_out = len_result;
  return(result);
}

RDI_PROC RDI_U32*
rdi_idx_run_from_first_count(RDI_Parsed *parsed,
                             RDI_U32 raw_first, RDI_U32 raw_count,
                             RDI_U32 *n_out){
  RDI_U32 raw_opl = raw_first + raw_count;
  RDI_U32 opl = rdi_parse__min(raw_opl, parsed->idx_run_count);
  RDI_U32 first = rdi_parse__min(raw_first, opl);
  
  RDI_U32 *result = 0;
  if (first < parsed->idx_run_count){
    result = parsed->idx_run_data + first;
  }
  *n_out = opl - first;
  return(result);
}

//- line info

RDI_PROC void
rdi_parse_line_info(RDI_Parsed *rdi, RDI_U64 line_info_idx, RDI_ParsedLineInfo *out)
{
  RDI_U64 line_info_count;
  RDI_LineInfo *line_info_ptr = (RDI_LineInfo *)rdi_data_from_dsec(rdi, RDI_DataSectionTag_LineInfo, sizeof(RDI_LineInfo), RDI_DataSectionTag_LineInfo, &line_info_count);

  if(line_info_idx < line_info_count)
  {
    RDI_U64 voffs_max, lines_max, cols_max;
    RDI_U64    *voffs = (RDI_U64    *)rdi_data_from_dsec(rdi, RDI_DataSectionTag_LineInfoVoffs,   sizeof(voffs[0]), RDI_DataSectionTag_LineInfoVoffs,   &voffs_max);
    RDI_Line   *lines = (RDI_Line   *)rdi_data_from_dsec(rdi, RDI_DataSectionTag_LineInfoData,    sizeof(lines[0]), RDI_DataSectionTag_LineInfoData,    &lines_max);
    RDI_Column *cols  = (RDI_Column *)rdi_data_from_dsec(rdi, RDI_DataSectionTag_LineInfoColumns, sizeof(cols[0]),  RDI_DataSectionTag_LineInfoColumns, &cols_max);

    RDI_LineInfo line_info = line_info_ptr[line_info_idx];
    if(line_info.voff_data_idx + line_info.line_count <= voffs_max &&
       line_info.line_data_idx + line_info.line_count <= lines_max &&
       line_info.col_data_idx  + line_info.col_count  <= cols_max)
    {
      out->count     = line_info.line_count;
      out->col_count = line_info.col_count;
      out->voffs     = voffs + line_info.voff_data_idx;
      out->lines     = lines + line_info.line_data_idx;
      out->cols      = cols  + line_info.col_data_idx;
    }
  }
}

RDI_PROC void
rdi_line_info_from_unit(RDI_Parsed *rdi, RDI_Unit *unit, RDI_ParsedLineInfo *out)
{
  rdi_parse_line_info(rdi, unit->line_info_idx, out);
}

RDI_PROC RDI_U64
rdi_line_info_idx_from_voff(RDI_ParsedLineInfo *line_info, RDI_U64 voff)
{
  RDI_U64 result = 0;
  if (line_info->count > 0 && line_info->voffs[0] <= voff && voff < line_info->voffs[line_info->count - 1]){
    // assuming: (i < j) -> (vmap[i].voff < vmap[j].voff)
    // find i such that: (vmap[i].voff <= voff) && (voff < vmap[i + 1].voff)
    RDI_U32 first = 0;
    RDI_U32 opl   = line_info->count;
    for (;;){
      RDI_U32 mid = (first + opl)/2;
      if (line_info->voffs[mid] < voff){
        first = mid;
      }
      else if (line_info->voffs[mid] > voff){
        opl = mid;
      }
      else{
        first = mid;
        break;
      }
      if (opl - first <= 1){
        break;
      }
    }
    result = (RDI_U64)first;
  }
  return(result);
}

RDI_PROC void
rdi_line_map_from_source_file(RDI_Parsed *p, RDI_SourceFile *srcfile,
                              RDI_ParsedLineMap *out){
  RDI_U64 num_count = 0;
  RDI_U32 *nums = (RDI_U32*)
    rdi_data_from_dsec(p, srcfile->line_map_nums_data_idx, sizeof(RDI_U32),
                       RDI_DataSectionTag_LineMapNumbers,
                       &num_count);
  
  RDI_U64 range_count = 0;
  RDI_U32 *ranges = (RDI_U32*)
    rdi_data_from_dsec(p, srcfile->line_map_range_data_idx, sizeof(RDI_U32),
                       RDI_DataSectionTag_LineMapRanges,
                       &range_count);
  
  RDI_U64 voff_count = 0;
  RDI_U64 *voffs = (RDI_U64*)
    rdi_data_from_dsec(p, srcfile->line_map_voff_data_idx, sizeof(RDI_U64),
                       RDI_DataSectionTag_LineMapVoffs,
                       &voff_count);
  
  RDI_U32 count_a = (range_count > 0)?(range_count - 1):0;
  RDI_U32 count_b = rdi_parse__min(count_a, num_count);
  RDI_U32 count = rdi_parse__min(count_b, srcfile->line_map_count);
  
  out->nums = nums;
  out->ranges = ranges;
  out->voffs = voffs;
  out->count = count;
  out->voff_count = voff_count;
}

RDI_PROC RDI_U64*
rdi_line_voffs_from_num(RDI_ParsedLineMap *map, RDI_U32 linenum, RDI_U32 *n_out){
  RDI_U64 *result = 0;
  *n_out = 0;
  
  RDI_U32 closest_i = 0;
  if (map->count > 0 && map->nums[0] <= linenum){
    // assuming: (i < j) -> (nums[i] < nums[j])
    // find i such that: (nums[i] <= linenum) && (linenum < nums[i + 1])
    RDI_U32 *nums = map->nums;
    RDI_U32 first = 0;
    RDI_U32 opl   = map->count;
    for (;;){
      RDI_U32 mid = (first + opl)/2;
      if (nums[mid] < linenum){
        first = mid;
      }
      else if (nums[mid] > linenum){
        opl = mid;
      }
      else{
        first = mid;
        break;
      }
      if (opl - first <= 1){
        break;
      }
    }
    closest_i = first;
  }
  
  // round up instead of down if possible
  if (closest_i + 1 < map->count &&
      map->nums[closest_i] < linenum){
    closest_i += 1;
  }
  
  // set result if possible
  if (closest_i < map->count){
    RDI_U32 first = map->ranges[closest_i];
    RDI_U32 opl   = map->ranges[closest_i + 1];
    if (opl < map->voff_count){
      result = map->voffs + first;
      *n_out = opl - first;
    }
  }
  
  return(result);
}


//- vmaps

RDI_PROC RDI_U64
rdi_vmap_idx_from_voff(RDI_VMapEntry *vmap, RDI_U32 vmap_count, RDI_U64 voff){
  RDI_U64 result = 0;
  if (vmap_count > 0 && vmap[0].voff <= voff && voff < vmap[vmap_count - 1].voff){
    // assuming: (i < j) -> (vmap[i].voff < vmap[j].voff)
    // find i such that: (vmap[i].voff <= voff) && (voff < vmap[i + 1].voff)
    RDI_U32 first = 0;
    RDI_U32 opl   = vmap_count;
    for (;;){
      RDI_U32 mid = (first + opl)/2;
      if (vmap[mid].voff < voff){
        first = mid;
      }
      else if (vmap[mid].voff > voff){
        opl = mid;
      }
      else{
        first = mid;
        break;
      }
      if (opl - first <= 1){
        break;
      }
    }
    result = (RDI_U64)vmap[first].idx;
  }
  return(result);
}

//- name maps

RDI_PROC RDI_NameMap*
rdi_name_map_from_kind(RDI_Parsed *p, RDI_NameMapKind kind){
  RDI_NameMap *result = 0;
  if (0 < kind && kind < RDI_NameMapKind_COUNT){
    result = p->name_maps_by_kind[kind];
  }
  return(result);
}

RDI_PROC void
rdi_name_map_parse(RDI_Parsed *p, RDI_NameMap *mapptr, RDI_ParsedNameMap *out){
  out->buckets = 0;
  out->bucket_count = 0;
  if (mapptr != 0){
    out->buckets = (RDI_NameMapBucket*)
      rdi_data_from_dsec(p, mapptr->bucket_data_idx, sizeof(RDI_NameMapBucket),
                         RDI_DataSectionTag_NameMapBuckets, &out->bucket_count);
    out->nodes = (RDI_NameMapNode*)
      rdi_data_from_dsec(p, mapptr->node_data_idx, sizeof(RDI_NameMapNode),
                         RDI_DataSectionTag_NameMapNodes, &out->node_count);
  }
}

RDI_PROC RDI_NameMapNode*
rdi_name_map_lookup(RDI_Parsed *p, RDI_ParsedNameMap *map,
                    RDI_U8 *str, RDI_U64 len){
  RDI_NameMapNode *result = 0;
  if (map->bucket_count > 0){
    RDI_NameMapBucket *buckets = map->buckets;
    RDI_U64 bucket_count = map->bucket_count;
    RDI_U64 hash = rdi_hash(str, len);
    RDI_U64 bucket_index = hash%bucket_count;
    RDI_NameMapBucket *bucket = map->buckets + bucket_index;
    
    RDI_NameMapNode *node = map->nodes + bucket->first_node;
    RDI_NameMapNode *node_opl = node + bucket->node_count;
    for (;node < node_opl; node += 1){
      // extract a string from this node
      RDI_U64 nlen = 0;
      RDI_U8 *nstr = rdi_string_from_idx(p, node->string_idx, &nlen);
      
      // compare this to the needle string
      RDI_S32 match = 0;
      if (nlen == len){
        RDI_U8 *a = str;
        RDI_U8 *aopl = str + len;
        RDI_U8 *b = nstr;
        for (;a < aopl && *a == *b; a += 1, b += 1);
        match = (a == aopl);
      }
      
      // stop with a matching node in result
      if (match){
        result = node;
        break;
      }
      
    }
  }
  return(result);
}

RDI_PROC RDI_U32*
rdi_matches_from_map_node(RDI_Parsed *p, RDI_NameMapNode *node,
                          RDI_U32 *n_out){
  RDI_U32 *result = 0;
  *n_out = 0;
  if (node != 0){
    if (node->match_count == 1){
      result = &node->match_idx_or_idx_run_first;
      *n_out = 1;
    }
    else{
      result = rdi_idx_run_from_first_count(p, node->match_idx_or_idx_run_first,
                                            node->match_count, n_out);
    }
  }
  return(result);
}

//- common helpers

RDI_PROC RDI_U64
rdi_first_voff_from_proc(RDI_Parsed *p, RDI_U32 proc_id){
  RDI_U64 result = 0;
  if (0 < proc_id && proc_id < p->procedures_count){
    RDI_Procedure *proc = p->procedures + proc_id;
    RDI_U32 scope_id = proc->root_scope_idx;
    if (0 < scope_id && scope_id < p->scopes_count){
      RDI_Scope *scope = p->scopes + scope_id;
      if (scope->voff_range_first < scope->voff_range_opl &&
          scope->voff_range_first < p->scope_voffs_count){
        result = p->scope_voffs[scope->voff_range_first];
      }
    }
  }
  return(result);
}

////////////////////////////////
//~ RADDBG Parsing Helpers

RDI_PROC void*
rdi_data_from_dsec(RDI_Parsed *parsed, RDI_U32 idx, RDI_U32 item_size,
                   RDI_DataSectionTag expected_tag,
                   RDI_U64 *count_out)
{
  void *result = 0;
  RDI_U32 count_result = 0;
  if(0 < idx && idx < parsed->dsec_count)
  {
    RDI_DataSection *ds = parsed->dsecs + idx;
    if(ds->tag == expected_tag)
    {
      RDI_U64 encoded_opl = ds->off + ds->encoded_size;
      if(encoded_opl <= parsed->raw_data_size)
      {
        count_result = ds->unpacked_size/item_size;
        result = (parsed->raw_data + ds->off);
      }
    }
  }
  *count_out = count_result;
  return(result);
}
