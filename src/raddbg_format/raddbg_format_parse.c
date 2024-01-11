// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ RADDBG Parse API

RADDBG_PROC RADDBG_ParseStatus
raddbg_parse(RADDBG_U8 *data, RADDBG_U64 size, RADDBG_Parsed *out){
  RADDBG_ParseStatus result = RADDBG_ParseStatus_Good;
  
  // out header
  RADDBG_Header *hdr = 0;
  {
    if (sizeof(*hdr) <= size){
      hdr = (RADDBG_Header*)data;
    }
    
    //  (errors)
    if (hdr == 0 || hdr->magic != RADDBG_MAGIC_CONSTANT){
      hdr = 0;
      result = RADDBG_ParseStatus_HeaderDoesNotMatch;
    }
    if (hdr != 0 && hdr->encoding_version != 1){
      hdr = 0;
      result = RADDBG_ParseStatus_UnsupportedVersionNumber;
    }
  }
  
  // out data sections
  RADDBG_DataSection *dsecs = 0;
  RADDBG_U32 dsec_count = 0;
  if (hdr != 0){
    RADDBG_U64 opl = (RADDBG_U64)hdr->data_section_off + (RADDBG_U64)hdr->data_section_count*sizeof(*dsecs);
    if (opl <= size){
      dsecs = (RADDBG_DataSection*)(data + hdr->data_section_off);
      dsec_count = hdr->data_section_count;
    }
    
    //  (errors)
    if (dsecs == 0){
      result = RADDBG_ParseStatus_InvalidDataSecionLayout;
    }
  }
  
  // extract primary data section indexes
  RADDBG_U32 dsec_idx[RADDBG_DataSectionTag_PRIMARY_COUNT] = {0};
  if (result == RADDBG_ParseStatus_Good){
    RADDBG_DataSection *sec_ptr = dsecs;
    for (RADDBG_U32 i = 0; i < dsec_count; i += 1, sec_ptr += 1){
      if (sec_ptr->tag < RADDBG_DataSectionTag_PRIMARY_COUNT){
        dsec_idx[sec_ptr->tag] = i;
      }
    }
  }
  
  // fill out data block (part 1)
  if (result == RADDBG_ParseStatus_Good){
    out->raw_data = data;
    out->raw_data_size = size;
    out->dsecs = dsecs;
    out->dsec_count = dsec_count;
    for (RADDBG_U32 i = 0; i < RADDBG_DataSectionTag_PRIMARY_COUNT; i += 1){
      out->dsec_idx[i] = dsec_idx[i];
    }
  }
  
  // out string table
  RADDBG_U8 *string_data = 0;
  RADDBG_U64 string_opl = 0;
  RADDBG_U32 *string_offs = 0;
  RADDBG_U64 string_count = 0;
  if (result == RADDBG_ParseStatus_Good){
    raddbg_parse__extract_primary(out, string_data, &string_opl,
                                  RADDBG_DataSectionTag_StringData);
    
    RADDBG_U64 table_entry_count = 0;
    raddbg_parse__extract_primary(out, string_offs, &table_entry_count,
                                  RADDBG_DataSectionTag_StringTable);
    if (table_entry_count > 0){
      string_count = table_entry_count - 1;
    }
    
    //  (errors)
    if (string_data == 0){
      result = RADDBG_ParseStatus_MissingStringDataSection;
    }
    else if (string_offs == 0){
      result = RADDBG_ParseStatus_MissingStringTableSection;
    }
  }
  
  // out index runs
  RADDBG_U32 *idx_run_data = 0;
  RADDBG_U64 idx_run_count = 0;
  if (result == RADDBG_ParseStatus_Good){
    raddbg_parse__extract_primary(out, idx_run_data, &idx_run_count,
                                  RADDBG_DataSectionTag_IndexRuns);
    
    //  (errors)
    if (idx_run_data == 0){
      result = RADDBG_ParseStatus_MissingIndexRunSection;
    }
  }
  
  if (result == RADDBG_ParseStatus_Good){
    // fill out primary data structures (part 2)
    out->string_data = string_data;
    out->string_offs = string_offs;
    out->string_data_size = string_opl;
    out->string_count = string_count;
    out->idx_run_data = idx_run_data;
    out->idx_run_count = idx_run_count;
    
    {
      RADDBG_TopLevelInfo *tli = 0;
      RADDBG_U64 dummy = 0;
      raddbg_parse__extract_primary(out, tli, &dummy, RADDBG_DataSectionTag_TopLevelInfo);
      if (dummy != 1){
        tli = 0;
      }
      out->top_level_info = tli;
    }
    
    raddbg_parse__extract_primary(out, out->binary_sections, &out->binary_section_count,
                                  RADDBG_DataSectionTag_BinarySections);
    
    raddbg_parse__extract_primary(out, out->file_paths, &out->file_path_count,
                                  RADDBG_DataSectionTag_FilePathNodes);
    
    raddbg_parse__extract_primary(out, out->source_files, &out->source_file_count,
                                  RADDBG_DataSectionTag_SourceFiles);
    
    raddbg_parse__extract_primary(out, out->units, &out->unit_count,
                                  RADDBG_DataSectionTag_Units);
    
    raddbg_parse__extract_primary(out, out->unit_vmap, &out->unit_vmap_count,
                                  RADDBG_DataSectionTag_UnitVmap);
    
    raddbg_parse__extract_primary(out, out->unit_vmap, &out->unit_vmap_count,
                                  RADDBG_DataSectionTag_UnitVmap);
    
    raddbg_parse__extract_primary(out, out->type_nodes, &out->type_node_count,
                                  RADDBG_DataSectionTag_TypeNodes);
    
    raddbg_parse__extract_primary(out, out->udts, &out->udt_count,
                                  RADDBG_DataSectionTag_UDTs);
    
    raddbg_parse__extract_primary(out, out->members, &out->member_count,
                                  RADDBG_DataSectionTag_Members);
    
    raddbg_parse__extract_primary(out, out->enum_members, &out->enum_member_count,
                                  RADDBG_DataSectionTag_EnumMembers);
    
    raddbg_parse__extract_primary(out, out->global_variables, &out->global_variable_count,
                                  RADDBG_DataSectionTag_GlobalVariables);
    
    raddbg_parse__extract_primary(out, out->global_vmap, &out->global_vmap_count,
                                  RADDBG_DataSectionTag_GlobalVmap);
    
    raddbg_parse__extract_primary(out, out->thread_variables, &out->thread_variable_count,
                                  RADDBG_DataSectionTag_ThreadVariables);
    
    raddbg_parse__extract_primary(out, out->procedures, &out->procedure_count,
                                  RADDBG_DataSectionTag_Procedures);
    
    raddbg_parse__extract_primary(out, out->scopes, &out->scope_count,
                                  RADDBG_DataSectionTag_Scopes);
    
    raddbg_parse__extract_primary(out, out->scope_voffs, &out->scope_voff_count,
                                  RADDBG_DataSectionTag_ScopeVoffData);
    
    raddbg_parse__extract_primary(out, out->scope_vmap, &out->scope_vmap_count,
                                  RADDBG_DataSectionTag_ScopeVmap);
    
    raddbg_parse__extract_primary(out, out->locals, &out->local_count,
                                  RADDBG_DataSectionTag_Locals);
    
    raddbg_parse__extract_primary(out, out->location_blocks, &out->location_block_count,
                                  RADDBG_DataSectionTag_LocationBlocks);
    
    raddbg_parse__extract_primary(out, out->location_data, &out->location_data_size,
                                  RADDBG_DataSectionTag_LocationData);
    
    {
      raddbg_parse__extract_primary(out, out->name_maps, &out->name_map_count,
                                    RADDBG_DataSectionTag_NameMaps);
      
      RADDBG_NameMap *name_map_ptr = out->name_maps;
      RADDBG_NameMap *name_map_opl = out->name_maps + out->name_map_count;
      for (; name_map_ptr < name_map_opl; name_map_ptr += 1){
        if (out->name_maps_by_kind[name_map_ptr->kind] == 0){
          out->name_maps_by_kind[name_map_ptr->kind] = name_map_ptr;
        }
      }
    }
    
  }
  
  return(result);
}

RADDBG_PROC RADDBG_U8*
raddbg_string_from_idx(RADDBG_Parsed *parsed, RADDBG_U32 idx, RADDBG_U64 *len_out){
  RADDBG_U8 *result = 0;
  RADDBG_U64 len_result = 0;
  if (idx < parsed->string_count){
    RADDBG_U32 off_raw = parsed->string_offs[idx];
    RADDBG_U32 opl_raw = parsed->string_offs[idx + 1];
    RADDBG_U32 opl = raddbg_parse__min(opl_raw, parsed->string_data_size);
    RADDBG_U32 off = raddbg_parse__min(off_raw, opl);
    result = parsed->string_data + off;
    len_result = opl - off;
  }
  *len_out = len_result;
  return(result);
}

RADDBG_PROC RADDBG_U32*
raddbg_idx_run_from_first_count(RADDBG_Parsed *parsed,
                                RADDBG_U32 raw_first, RADDBG_U32 raw_count,
                                RADDBG_U32 *n_out){
  RADDBG_U32 raw_opl = raw_first + raw_count;
  RADDBG_U32 opl = raddbg_parse__min(raw_opl, parsed->idx_run_count);
  RADDBG_U32 first = raddbg_parse__min(raw_first, opl);
  
  RADDBG_U32 *result = 0;
  if (first < parsed->idx_run_count){
    result = parsed->idx_run_data + first;
  }
  *n_out = opl - first;
  return(result);
}

//- line info

RADDBG_PROC void
raddbg_line_info_from_unit(RADDBG_Parsed *p, RADDBG_Unit *unit, RADDBG_ParsedLineInfo *out){
  RADDBG_U64 line_info_voff_count = 0;
  RADDBG_U64 *voffs = (RADDBG_U64*)
    raddbg_data_from_dsec(p, unit->line_info_voffs_data_idx, sizeof(RADDBG_U64),
                          RADDBG_DataSectionTag_LineInfoVoffs,
                          &line_info_voff_count);
  
  RADDBG_U64 line_info_count_raw = 0;
  RADDBG_Line *lines = (RADDBG_Line*)
    raddbg_data_from_dsec(p, unit->line_info_data_idx, sizeof(RADDBG_Line),
                          RADDBG_DataSectionTag_LineInfoData,
                          &line_info_count_raw);
  
  RADDBG_U64 column_info_count_raw = 0;
  RADDBG_Column *cols = (RADDBG_Column*)
    raddbg_data_from_dsec(p, unit->line_info_col_data_idx, sizeof(RADDBG_Column),
                          RADDBG_DataSectionTag_LineInfoColumns,
                          &column_info_count_raw);
  
  RADDBG_U32 line_info_count_a = (line_info_voff_count > 0)?line_info_voff_count - 1:0;
  RADDBG_U32 line_info_count   = raddbg_parse__min(line_info_count_a, line_info_count_raw);
  RADDBG_U32 column_info_count = raddbg_parse__min(column_info_count_raw, line_info_count);
  
  out->voffs = voffs;
  out->lines = lines;
  out->cols = cols;
  out->count = line_info_count;
  out->col_count = column_info_count;
}

RADDBG_PROC RADDBG_U64
raddbg_line_info_idx_from_voff(RADDBG_ParsedLineInfo *line_info, RADDBG_U64 voff)
{
  RADDBG_U64 result = 0;
  if (line_info->count > 0 && line_info->voffs[0] <= voff && voff < line_info->voffs[line_info->count - 1]){
    // assuming: (i < j) -> (vmap[i].voff < vmap[j].voff)
    // find i such that: (vmap[i].voff <= voff) && (voff < vmap[i + 1].voff)
    RADDBG_U32 first = 0;
    RADDBG_U32 opl   = line_info->count;
    for (;;){
      RADDBG_U32 mid = (first + opl)/2;
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
    result = (RADDBG_U64)first;
  }
  return(result);
}

RADDBG_PROC void
raddbg_line_map_from_source_file(RADDBG_Parsed *p, RADDBG_SourceFile *srcfile,
                                 RADDBG_ParsedLineMap *out){
  RADDBG_U64 num_count = 0;
  RADDBG_U32 *nums = (RADDBG_U32*)
    raddbg_data_from_dsec(p, srcfile->line_map_nums_data_idx, sizeof(RADDBG_U32),
                          RADDBG_DataSectionTag_LineMapNumbers,
                          &num_count);
  
  RADDBG_U64 range_count = 0;
  RADDBG_U32 *ranges = (RADDBG_U32*)
    raddbg_data_from_dsec(p, srcfile->line_map_range_data_idx, sizeof(RADDBG_U32),
                          RADDBG_DataSectionTag_LineMapRanges,
                          &range_count);
  
  RADDBG_U64 voff_count = 0;
  RADDBG_U64 *voffs = (RADDBG_U64*)
    raddbg_data_from_dsec(p, srcfile->line_map_voff_data_idx, sizeof(RADDBG_U64),
                          RADDBG_DataSectionTag_LineMapVoffs,
                          &voff_count);
  
  RADDBG_U32 count_a = (range_count > 0)?(range_count - 1):0;
  RADDBG_U32 count_b = raddbg_parse__min(count_a, num_count);
  RADDBG_U32 count = raddbg_parse__min(count_b, srcfile->line_map_count);
  
  out->nums = nums;
  out->ranges = ranges;
  out->voffs = voffs;
  out->count = count;
  out->voff_count = voff_count;
}

RADDBG_PROC RADDBG_U64*
raddbg_line_voffs_from_num(RADDBG_ParsedLineMap *map, RADDBG_U32 linenum, RADDBG_U32 *n_out){
  RADDBG_U64 *result = 0;
  *n_out = 0;
  
  RADDBG_U32 closest_i = 0;
  if (map->count > 0 && map->nums[0] <= linenum){
    // assuming: (i < j) -> (nums[i] < nums[j])
    // find i such that: (nums[i] <= linenum) && (linenum < nums[i + 1])
    RADDBG_U32 *nums = map->nums;
    RADDBG_U32 first = 0;
    RADDBG_U32 opl   = map->count;
    for (;;){
      RADDBG_U32 mid = (first + opl)/2;
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
    RADDBG_U32 first = map->ranges[closest_i];
    RADDBG_U32 opl   = map->ranges[closest_i + 1];
    if (opl < map->voff_count){
      result = map->voffs + first;
      *n_out = opl - first;
    }
  }
  
  return(result);
}


//- vmaps

RADDBG_PROC RADDBG_U64
raddbg_vmap_idx_from_voff(RADDBG_VMapEntry *vmap, RADDBG_U32 vmap_count, RADDBG_U64 voff){
  RADDBG_U64 result = 0;
  if (vmap_count > 0 && vmap[0].voff <= voff && voff < vmap[vmap_count - 1].voff){
    // assuming: (i < j) -> (vmap[i].voff < vmap[j].voff)
    // find i such that: (vmap[i].voff <= voff) && (voff < vmap[i + 1].voff)
    RADDBG_U32 first = 0;
    RADDBG_U32 opl   = vmap_count;
    for (;;){
      RADDBG_U32 mid = (first + opl)/2;
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
    result = (RADDBG_U64)vmap[first].idx;
  }
  return(result);
}

//- name maps

RADDBG_PROC RADDBG_NameMap*
raddbg_name_map_from_kind(RADDBG_Parsed *p, RADDBG_NameMapKind kind){
  RADDBG_NameMap *result = 0;
  if (0 < kind && kind < RADDBG_NameMapKind_COUNT){
    result = p->name_maps_by_kind[kind];
  }
  return(result);
}

RADDBG_PROC void
raddbg_name_map_parse(RADDBG_Parsed *p, RADDBG_NameMap *mapptr, RADDBG_ParsedNameMap *out){
  out->buckets = 0;
  out->bucket_count = 0;
  if (mapptr != 0){
    out->buckets = (RADDBG_NameMapBucket*)
      raddbg_data_from_dsec(p, mapptr->bucket_data_idx, sizeof(RADDBG_NameMapBucket),
                            RADDBG_DataSectionTag_NameMapBuckets, &out->bucket_count);
    out->nodes = (RADDBG_NameMapNode*)
      raddbg_data_from_dsec(p, mapptr->node_data_idx, sizeof(RADDBG_NameMapNode),
                            RADDBG_DataSectionTag_NameMapNodes, &out->node_count);
  }
}

RADDBG_PROC RADDBG_NameMapNode*
raddbg_name_map_lookup(RADDBG_Parsed *p, RADDBG_ParsedNameMap *map,
                       RADDBG_U8 *str, RADDBG_U64 len){
  RADDBG_NameMapNode *result = 0;
  if (map->bucket_count > 0){
    RADDBG_NameMapBucket *buckets = map->buckets;
    RADDBG_U64 bucket_count = map->bucket_count;
    RADDBG_U64 hash = raddbg_hash(str, len);
    RADDBG_U64 bucket_index = hash%bucket_count;
    RADDBG_NameMapBucket *bucket = map->buckets + bucket_index;
    
    RADDBG_NameMapNode *node = map->nodes + bucket->first_node;
    RADDBG_NameMapNode *node_opl = node + bucket->node_count;
    for (;node < node_opl; node += 1){
      // extract a string from this node
      RADDBG_U64 nlen = 0;
      RADDBG_U8 *nstr = raddbg_string_from_idx(p, node->string_idx, &nlen);
      
      // compare this to the needle string
      RADDBG_S32 match = 0;
      if (nlen == len){
        RADDBG_U8 *a = str;
        RADDBG_U8 *aopl = str + len;
        RADDBG_U8 *b = nstr;
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

RADDBG_PROC RADDBG_U32*
raddbg_matches_from_map_node(RADDBG_Parsed *p, RADDBG_NameMapNode *node,
                             RADDBG_U32 *n_out){
  RADDBG_U32 *result = 0;
  *n_out = 0;
  if (node != 0){
    if (node->match_count == 1){
      result = &node->match_idx_or_idx_run_first;
      *n_out = 1;
    }
    else{
      result = raddbg_idx_run_from_first_count(p, node->match_idx_or_idx_run_first,
                                               node->match_count, n_out);
    }
  }
  return(result);
}

//- common helpers

RADDBG_PROC RADDBG_U64
raddbg_first_voff_from_proc(RADDBG_Parsed *p, RADDBG_U32 proc_id){
  RADDBG_U64 result = 0;
  if (0 < proc_id && proc_id < p->procedure_count){
    RADDBG_Procedure *proc = p->procedures + proc_id;
    RADDBG_U32 scope_id = proc->root_scope_idx;
    if (0 < scope_id && scope_id < p->scope_count){
      RADDBG_Scope *scope = p->scopes + scope_id;
      if (scope->voff_range_first < scope->voff_range_opl &&
          scope->voff_range_first < p->scope_voff_count){
        result = p->scope_voffs[scope->voff_range_first];
      }
    }
  }
  return(result);
}

////////////////////////////////
//~ RADDBG Parsing Helpers

RADDBG_PROC void*
raddbg_data_from_dsec(RADDBG_Parsed *parsed, RADDBG_U32 idx, RADDBG_U32 item_size,
                      RADDBG_DataSectionTag expected_tag,
                      RADDBG_U64 *count_out){
  void *result = 0;
  RADDBG_U32 count_result = 0;
  
  // TODO(allen): need a version of this that works with encodings other than "Unpacked"
  
  if (0 < idx && idx < parsed->dsec_count){
    RADDBG_DataSection *ds = parsed->dsecs + idx;
    if (ds->tag == expected_tag){
      RADDBG_U64 opl = ds->off + ds->encoded_size;
      if (opl <= parsed->raw_data_size){
        count_result = ds->encoded_size/item_size;
        result = (parsed->raw_data + ds->off);
      }
    }
  }
  
  *count_out = count_result;
  return(result);
}
