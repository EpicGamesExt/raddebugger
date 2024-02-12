// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ RADDBG Parse API

RADDBGI_PROC RADDBGI_ParseStatus
raddbgi_parse(RADDBGI_U8 *data, RADDBGI_U64 size, RADDBGI_Parsed *out){
  RADDBGI_ParseStatus result = RADDBGI_ParseStatus_Good;
  
  // out header
  RADDBGI_Header *hdr = 0;
  {
    if (sizeof(*hdr) <= size){
      hdr = (RADDBGI_Header*)data;
    }
    
    //  (errors)
    if (hdr == 0 || hdr->magic != RADDBGI_MAGIC_CONSTANT){
      hdr = 0;
      result = RADDBGI_ParseStatus_HeaderDoesNotMatch;
    }
    if (hdr != 0 && hdr->encoding_version != 1){
      hdr = 0;
      result = RADDBGI_ParseStatus_UnsupportedVersionNumber;
    }
  }
  
  // out data sections
  RADDBGI_DataSection *dsecs = 0;
  RADDBGI_U32 dsec_count = 0;
  if (hdr != 0){
    RADDBGI_U64 opl = (RADDBGI_U64)hdr->data_section_off + (RADDBGI_U64)hdr->data_section_count*sizeof(*dsecs);
    if (opl <= size){
      dsecs = (RADDBGI_DataSection*)(data + hdr->data_section_off);
      dsec_count = hdr->data_section_count;
    }
    
    //  (errors)
    if (dsecs == 0){
      result = RADDBGI_ParseStatus_InvalidDataSecionLayout;
    }
  }
  
  // extract primary data section indexes
  RADDBGI_U32 dsec_idx[RADDBGI_DataSectionTag_PRIMARY_COUNT] = {0};
  if (result == RADDBGI_ParseStatus_Good){
    RADDBGI_DataSection *sec_ptr = dsecs;
    for (RADDBGI_U32 i = 0; i < dsec_count; i += 1, sec_ptr += 1){
      if (sec_ptr->tag < RADDBGI_DataSectionTag_PRIMARY_COUNT){
        dsec_idx[sec_ptr->tag] = i;
      }
    }
  }
  
  // fill out data block (part 1)
  if (result == RADDBGI_ParseStatus_Good){
    out->raw_data = data;
    out->raw_data_size = size;
    out->dsecs = dsecs;
    out->dsec_count = dsec_count;
    for (RADDBGI_U32 i = 0; i < RADDBGI_DataSectionTag_PRIMARY_COUNT; i += 1){
      out->dsec_idx[i] = dsec_idx[i];
    }
  }
  
  // out string table
  RADDBGI_U8 *string_data = 0;
  RADDBGI_U64 string_opl = 0;
  RADDBGI_U32 *string_offs = 0;
  RADDBGI_U64 string_count = 0;
  if (result == RADDBGI_ParseStatus_Good){
    raddbgi_parse__extract_primary(out, string_data, &string_opl,
                                   RADDBGI_DataSectionTag_StringData);
    
    RADDBGI_U64 table_entry_count = 0;
    raddbgi_parse__extract_primary(out, string_offs, &table_entry_count,
                                   RADDBGI_DataSectionTag_StringTable);
    if (table_entry_count > 0){
      string_count = table_entry_count - 1;
    }
    
    //  (errors)
    if (string_data == 0){
      result = RADDBGI_ParseStatus_MissingStringDataSection;
    }
    else if (string_offs == 0){
      result = RADDBGI_ParseStatus_MissingStringTableSection;
    }
  }
  
  // out index runs
  RADDBGI_U32 *idx_run_data = 0;
  RADDBGI_U64 idx_run_count = 0;
  if (result == RADDBGI_ParseStatus_Good){
    raddbgi_parse__extract_primary(out, idx_run_data, &idx_run_count,
                                   RADDBGI_DataSectionTag_IndexRuns);
    
    //  (errors)
    if (idx_run_data == 0){
      result = RADDBGI_ParseStatus_MissingIndexRunSection;
    }
  }
  
  if (result == RADDBGI_ParseStatus_Good){
    // fill out primary data structures (part 2)
    out->string_data = string_data;
    out->string_offs = string_offs;
    out->string_data_size = string_opl;
    out->string_count = string_count;
    out->idx_run_data = idx_run_data;
    out->idx_run_count = idx_run_count;
    
    {
      RADDBGI_TopLevelInfo *tli = 0;
      RADDBGI_U64 dummy = 0;
      raddbgi_parse__extract_primary(out, tli, &dummy, RADDBGI_DataSectionTag_TopLevelInfo);
      if (dummy != 1){
        tli = 0;
      }
      out->top_level_info = tli;
    }
    
    raddbgi_parse__extract_primary(out, out->binary_sections, &out->binary_sections_count,
                                   RADDBGI_DataSectionTag_BinarySections);
    
    raddbgi_parse__extract_primary(out, out->file_paths, &out->file_paths_count,
                                   RADDBGI_DataSectionTag_FilePathNodes);
    
    raddbgi_parse__extract_primary(out, out->source_files, &out->source_files_count,
                                   RADDBGI_DataSectionTag_SourceFiles);
    
    raddbgi_parse__extract_primary(out, out->units, &out->units_count,
                                   RADDBGI_DataSectionTag_Units);
    
    raddbgi_parse__extract_primary(out, out->unit_vmap, &out->unit_vmap_count,
                                   RADDBGI_DataSectionTag_UnitVmap);
    
    raddbgi_parse__extract_primary(out, out->unit_vmap, &out->unit_vmap_count,
                                   RADDBGI_DataSectionTag_UnitVmap);
    
    raddbgi_parse__extract_primary(out, out->type_nodes, &out->type_nodes_count,
                                   RADDBGI_DataSectionTag_TypeNodes);
    
    raddbgi_parse__extract_primary(out, out->udts, &out->udts_count,
                                   RADDBGI_DataSectionTag_UDTs);
    
    raddbgi_parse__extract_primary(out, out->members, &out->members_count,
                                   RADDBGI_DataSectionTag_Members);
    
    raddbgi_parse__extract_primary(out, out->enum_members, &out->enum_members_count,
                                   RADDBGI_DataSectionTag_EnumMembers);
    
    raddbgi_parse__extract_primary(out, out->global_variables, &out->global_variables_count,
                                   RADDBGI_DataSectionTag_GlobalVariables);
    
    raddbgi_parse__extract_primary(out, out->global_vmap, &out->global_vmap_count,
                                   RADDBGI_DataSectionTag_GlobalVmap);
    
    raddbgi_parse__extract_primary(out, out->thread_variables, &out->thread_variables_count,
                                   RADDBGI_DataSectionTag_ThreadVariables);
    
    raddbgi_parse__extract_primary(out, out->procedures, &out->procedures_count,
                                   RADDBGI_DataSectionTag_Procedures);
    
    raddbgi_parse__extract_primary(out, out->scopes, &out->scopes_count,
                                   RADDBGI_DataSectionTag_Scopes);
    
    raddbgi_parse__extract_primary(out, out->scope_voffs, &out->scope_voffs_count,
                                   RADDBGI_DataSectionTag_ScopeVoffData);
    
    raddbgi_parse__extract_primary(out, out->scope_vmap, &out->scope_vmap_count,
                                   RADDBGI_DataSectionTag_ScopeVmap);
    
    raddbgi_parse__extract_primary(out, out->locals, &out->locals_count,
                                   RADDBGI_DataSectionTag_Locals);
    
    raddbgi_parse__extract_primary(out, out->location_blocks, &out->location_blocks_count,
                                   RADDBGI_DataSectionTag_LocationBlocks);
    
    raddbgi_parse__extract_primary(out, out->location_data, &out->location_data_size,
                                   RADDBGI_DataSectionTag_LocationData);
    
    {
      raddbgi_parse__extract_primary(out, out->name_maps, &out->name_maps_count,
                                     RADDBGI_DataSectionTag_NameMaps);
      
      RADDBGI_NameMap *name_map_ptr = out->name_maps;
      RADDBGI_NameMap *name_map_opl = out->name_maps + out->name_maps_count;
      for (; name_map_ptr < name_map_opl; name_map_ptr += 1){
        if (out->name_maps_by_kind[name_map_ptr->kind] == 0){
          out->name_maps_by_kind[name_map_ptr->kind] = name_map_ptr;
        }
      }
    }
    
#if !defined(RADDBGI_DISABLE_NILS)
    if(out->binary_sections == 0)                { out->binary_sections        = &raddbgi_binary_section_nil;           out->binary_sections_count = 1; }
    if(out->file_paths == 0)                     { out->file_paths             = &raddbgi_file_path_node_nil;           out->file_paths_count = 1; }
    if(out->source_files == 0)                   { out->source_files           = &raddbgi_source_file_nil;              out->source_files_count = 1; }
    if(out->units == 0)                          { out->units                  = &raddbgi_unit_nil;                     out->units_count = 1; }
    if(out->unit_vmap == 0)                      { out->unit_vmap              = &raddbgi_vmap_entry_nil;               out->unit_vmap_count = 1; }
    if(out->type_nodes == 0)                     { out->type_nodes             = &raddbgi_type_node_nil;                out->type_nodes_count = 1; }
    if(out->udts == 0)                           { out->udts                   = &raddbgi_udt_nil;                      out->udts_count = 1; }
    if(out->members == 0)                        { out->members                = &raddbgi_member_nil;                   out->members_count = 1; }
    if(out->enum_members == 0)                   { out->enum_members           = &raddbgi_enum_member_nil;              out->enum_members_count = 1; }
    if(out->global_variables == 0)               { out->global_variables       = &raddbgi_global_variable_nil;          out->global_variables_count = 1; }
    if(out->global_vmap == 0)                    { out->global_vmap            = &raddbgi_vmap_entry_nil;               out->global_vmap_count = 1; }
    if(out->thread_variables == 0)               { out->thread_variables       = &raddbgi_thread_variable_nil;          out->thread_variables_count = 1; }
    if(out->procedures == 0)                     { out->procedures             = &raddbgi_procedure_nil;                out->procedures_count = 1; }
    if(out->scopes == 0)                         { out->scopes                 = &raddbgi_scope_nil;                    out->scopes_count = 1; }
    if(out->scope_voffs == 0)                    { out->scope_voffs            = &raddbgi_voff_nil;                     out->scope_voffs_count = 1; }
    if(out->scope_vmap == 0)                     { out->scope_vmap             = &raddbgi_vmap_entry_nil;               out->scope_vmap_count = 1; }
    if(out->locals == 0)                         { out->locals                 = &raddbgi_local_nil;                    out->locals_count = 1; }
    if(out->location_blocks == 0)                { out->location_blocks        = &raddbgi_location_block_nil;           out->location_blocks_count = 1; }
#endif
    
  }
  
  return(result);
}

RADDBGI_PROC RADDBGI_U8*
raddbgi_string_from_idx(RADDBGI_Parsed *parsed, RADDBGI_U32 idx, RADDBGI_U64 *len_out){
  RADDBGI_U8 *result = 0;
  RADDBGI_U64 len_result = 0;
  if (idx < parsed->string_count){
    RADDBGI_U32 off_raw = parsed->string_offs[idx];
    RADDBGI_U32 opl_raw = parsed->string_offs[idx + 1];
    RADDBGI_U32 opl = raddbgi_parse__min(opl_raw, parsed->string_data_size);
    RADDBGI_U32 off = raddbgi_parse__min(off_raw, opl);
    result = parsed->string_data + off;
    len_result = opl - off;
  }
  *len_out = len_result;
  return(result);
}

RADDBGI_PROC RADDBGI_U32*
raddbgi_idx_run_from_first_count(RADDBGI_Parsed *parsed,
                                 RADDBGI_U32 raw_first, RADDBGI_U32 raw_count,
                                 RADDBGI_U32 *n_out){
  RADDBGI_U32 raw_opl = raw_first + raw_count;
  RADDBGI_U32 opl = raddbgi_parse__min(raw_opl, parsed->idx_run_count);
  RADDBGI_U32 first = raddbgi_parse__min(raw_first, opl);
  
  RADDBGI_U32 *result = 0;
  if (first < parsed->idx_run_count){
    result = parsed->idx_run_data + first;
  }
  *n_out = opl - first;
  return(result);
}

//- line info

RADDBGI_PROC void
raddbgi_line_info_from_unit(RADDBGI_Parsed *p, RADDBGI_Unit *unit, RADDBGI_ParsedLineInfo *out){
  RADDBGI_U64 line_info_voff_count = 0;
  RADDBGI_U64 *voffs = (RADDBGI_U64*)
    raddbgi_data_from_dsec(p, unit->line_info_voffs_data_idx, sizeof(RADDBGI_U64),
                           RADDBGI_DataSectionTag_LineInfoVoffs,
                           &line_info_voff_count);
  
  RADDBGI_U64 line_info_count_raw = 0;
  RADDBGI_Line *lines = (RADDBGI_Line*)
    raddbgi_data_from_dsec(p, unit->line_info_data_idx, sizeof(RADDBGI_Line),
                           RADDBGI_DataSectionTag_LineInfoData,
                           &line_info_count_raw);
  
  RADDBGI_U64 column_info_count_raw = 0;
  RADDBGI_Column *cols = (RADDBGI_Column*)
    raddbgi_data_from_dsec(p, unit->line_info_col_data_idx, sizeof(RADDBGI_Column),
                           RADDBGI_DataSectionTag_LineInfoColumns,
                           &column_info_count_raw);
  
  RADDBGI_U32 line_info_count_a = (line_info_voff_count > 0)?line_info_voff_count - 1:0;
  RADDBGI_U32 line_info_count   = raddbgi_parse__min(line_info_count_a, line_info_count_raw);
  RADDBGI_U32 column_info_count = raddbgi_parse__min(column_info_count_raw, line_info_count);
  
  out->voffs = voffs;
  out->lines = lines;
  out->cols = cols;
  out->count = line_info_count;
  out->col_count = column_info_count;
}

RADDBGI_PROC RADDBGI_U64
raddbgi_line_info_idx_from_voff(RADDBGI_ParsedLineInfo *line_info, RADDBGI_U64 voff)
{
  RADDBGI_U64 result = 0;
  if (line_info->count > 0 && line_info->voffs[0] <= voff && voff < line_info->voffs[line_info->count - 1]){
    // assuming: (i < j) -> (vmap[i].voff < vmap[j].voff)
    // find i such that: (vmap[i].voff <= voff) && (voff < vmap[i + 1].voff)
    RADDBGI_U32 first = 0;
    RADDBGI_U32 opl   = line_info->count;
    for (;;){
      RADDBGI_U32 mid = (first + opl)/2;
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
    result = (RADDBGI_U64)first;
  }
  return(result);
}

RADDBGI_PROC void
raddbgi_line_map_from_source_file(RADDBGI_Parsed *p, RADDBGI_SourceFile *srcfile,
                                  RADDBGI_ParsedLineMap *out){
  RADDBGI_U64 num_count = 0;
  RADDBGI_U32 *nums = (RADDBGI_U32*)
    raddbgi_data_from_dsec(p, srcfile->line_map_nums_data_idx, sizeof(RADDBGI_U32),
                           RADDBGI_DataSectionTag_LineMapNumbers,
                           &num_count);
  
  RADDBGI_U64 range_count = 0;
  RADDBGI_U32 *ranges = (RADDBGI_U32*)
    raddbgi_data_from_dsec(p, srcfile->line_map_range_data_idx, sizeof(RADDBGI_U32),
                           RADDBGI_DataSectionTag_LineMapRanges,
                           &range_count);
  
  RADDBGI_U64 voff_count = 0;
  RADDBGI_U64 *voffs = (RADDBGI_U64*)
    raddbgi_data_from_dsec(p, srcfile->line_map_voff_data_idx, sizeof(RADDBGI_U64),
                           RADDBGI_DataSectionTag_LineMapVoffs,
                           &voff_count);
  
  RADDBGI_U32 count_a = (range_count > 0)?(range_count - 1):0;
  RADDBGI_U32 count_b = raddbgi_parse__min(count_a, num_count);
  RADDBGI_U32 count = raddbgi_parse__min(count_b, srcfile->line_map_count);
  
  out->nums = nums;
  out->ranges = ranges;
  out->voffs = voffs;
  out->count = count;
  out->voff_count = voff_count;
}

RADDBGI_PROC RADDBGI_U64*
raddbgi_line_voffs_from_num(RADDBGI_ParsedLineMap *map, RADDBGI_U32 linenum, RADDBGI_U32 *n_out){
  RADDBGI_U64 *result = 0;
  *n_out = 0;
  
  RADDBGI_U32 closest_i = 0;
  if (map->count > 0 && map->nums[0] <= linenum){
    // assuming: (i < j) -> (nums[i] < nums[j])
    // find i such that: (nums[i] <= linenum) && (linenum < nums[i + 1])
    RADDBGI_U32 *nums = map->nums;
    RADDBGI_U32 first = 0;
    RADDBGI_U32 opl   = map->count;
    for (;;){
      RADDBGI_U32 mid = (first + opl)/2;
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
    RADDBGI_U32 first = map->ranges[closest_i];
    RADDBGI_U32 opl   = map->ranges[closest_i + 1];
    if (opl < map->voff_count){
      result = map->voffs + first;
      *n_out = opl - first;
    }
  }
  
  return(result);
}


//- vmaps

RADDBGI_PROC RADDBGI_U64
raddbgi_vmap_idx_from_voff(RADDBGI_VMapEntry *vmap, RADDBGI_U32 vmap_count, RADDBGI_U64 voff){
  RADDBGI_U64 result = 0;
  if (vmap_count > 0 && vmap[0].voff <= voff && voff < vmap[vmap_count - 1].voff){
    // assuming: (i < j) -> (vmap[i].voff < vmap[j].voff)
    // find i such that: (vmap[i].voff <= voff) && (voff < vmap[i + 1].voff)
    RADDBGI_U32 first = 0;
    RADDBGI_U32 opl   = vmap_count;
    for (;;){
      RADDBGI_U32 mid = (first + opl)/2;
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
    result = (RADDBGI_U64)vmap[first].idx;
  }
  return(result);
}

//- name maps

RADDBGI_PROC RADDBGI_NameMap*
raddbgi_name_map_from_kind(RADDBGI_Parsed *p, RADDBGI_NameMapKind kind){
  RADDBGI_NameMap *result = 0;
  if (0 < kind && kind < RADDBGI_NameMapKind_COUNT){
    result = p->name_maps_by_kind[kind];
  }
  return(result);
}

RADDBGI_PROC void
raddbgi_name_map_parse(RADDBGI_Parsed *p, RADDBGI_NameMap *mapptr, RADDBGI_ParsedNameMap *out){
  out->buckets = 0;
  out->bucket_count = 0;
  if (mapptr != 0){
    out->buckets = (RADDBGI_NameMapBucket*)
      raddbgi_data_from_dsec(p, mapptr->bucket_data_idx, sizeof(RADDBGI_NameMapBucket),
                             RADDBGI_DataSectionTag_NameMapBuckets, &out->bucket_count);
    out->nodes = (RADDBGI_NameMapNode*)
      raddbgi_data_from_dsec(p, mapptr->node_data_idx, sizeof(RADDBGI_NameMapNode),
                             RADDBGI_DataSectionTag_NameMapNodes, &out->node_count);
  }
}

RADDBGI_PROC RADDBGI_NameMapNode*
raddbgi_name_map_lookup(RADDBGI_Parsed *p, RADDBGI_ParsedNameMap *map,
                        RADDBGI_U8 *str, RADDBGI_U64 len){
  RADDBGI_NameMapNode *result = 0;
  if (map->bucket_count > 0){
    RADDBGI_NameMapBucket *buckets = map->buckets;
    RADDBGI_U64 bucket_count = map->bucket_count;
    RADDBGI_U64 hash = raddbgi_hash(str, len);
    RADDBGI_U64 bucket_index = hash%bucket_count;
    RADDBGI_NameMapBucket *bucket = map->buckets + bucket_index;
    
    RADDBGI_NameMapNode *node = map->nodes + bucket->first_node;
    RADDBGI_NameMapNode *node_opl = node + bucket->node_count;
    for (;node < node_opl; node += 1){
      // extract a string from this node
      RADDBGI_U64 nlen = 0;
      RADDBGI_U8 *nstr = raddbgi_string_from_idx(p, node->string_idx, &nlen);
      
      // compare this to the needle string
      RADDBGI_S32 match = 0;
      if (nlen == len){
        RADDBGI_U8 *a = str;
        RADDBGI_U8 *aopl = str + len;
        RADDBGI_U8 *b = nstr;
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

RADDBGI_PROC RADDBGI_U32*
raddbgi_matches_from_map_node(RADDBGI_Parsed *p, RADDBGI_NameMapNode *node,
                              RADDBGI_U32 *n_out){
  RADDBGI_U32 *result = 0;
  *n_out = 0;
  if (node != 0){
    if (node->match_count == 1){
      result = &node->match_idx_or_idx_run_first;
      *n_out = 1;
    }
    else{
      result = raddbgi_idx_run_from_first_count(p, node->match_idx_or_idx_run_first,
                                                node->match_count, n_out);
    }
  }
  return(result);
}

//- common helpers

RADDBGI_PROC RADDBGI_U64
raddbgi_first_voff_from_proc(RADDBGI_Parsed *p, RADDBGI_U32 proc_id){
  RADDBGI_U64 result = 0;
  if (0 < proc_id && proc_id < p->procedures_count){
    RADDBGI_Procedure *proc = p->procedures + proc_id;
    RADDBGI_U32 scope_id = proc->root_scope_idx;
    if (0 < scope_id && scope_id < p->scopes_count){
      RADDBGI_Scope *scope = p->scopes + scope_id;
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

RADDBGI_PROC void*
raddbgi_data_from_dsec(RADDBGI_Parsed *parsed, RADDBGI_U32 idx, RADDBGI_U32 item_size,
                       RADDBGI_DataSectionTag expected_tag,
                       RADDBGI_U64 *count_out){
  void *result = 0;
  RADDBGI_U32 count_result = 0;
  
  // TODO(allen): need a version of this that works with encodings other than "Unpacked"
  
  if (0 < idx && idx < parsed->dsec_count){
    RADDBGI_DataSection *ds = parsed->dsecs + idx;
    if (ds->tag == expected_tag){
      RADDBGI_U64 opl = ds->off + ds->encoded_size;
      if (opl <= parsed->raw_data_size){
        count_result = ds->encoded_size/item_size;
        result = (parsed->raw_data + ds->off);
      }
    }
  }
  
  *count_out = count_result;
  return(result);
}
