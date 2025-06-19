// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ Top-Level Parsing API

RDI_PROC RDI_ParseStatus
rdi_parse(RDI_U8 *data, RDI_U64 size, RDI_Parsed *out)
{
  RDI_ParseStatus result = RDI_ParseStatus_Good;
  
  //////////////////////////////
  //- rjf: extract header
  //
  RDI_Header *hdr = 0;
  if(result == RDI_ParseStatus_Good)
  {
    if(sizeof(*hdr) <= size)
    {
      hdr = (RDI_Header*)data;
    }
    if(hdr == 0 || hdr->magic != RDI_MAGIC_CONSTANT)
    {
      hdr = 0;
      result = RDI_ParseStatus_HeaderDoesNotMatch;
    }
    if(hdr != 0 && hdr->encoding_version != RDI_ENCODING_VERSION)
    {
      hdr = 0;
      result = RDI_ParseStatus_UnsupportedVersionNumber;
    }
  }
  
  //////////////////////////////
  //- rjf: extract data sections
  //
  RDI_Section *dsecs = 0;
  RDI_U32 dsec_count = 0;
  if(result == RDI_ParseStatus_Good)
  {
    RDI_U64 opl = (RDI_U64)hdr->data_section_off + (RDI_U64)hdr->data_section_count*sizeof(*dsecs);
    if(opl <= size)
    {
      dsecs = (RDI_Section*)(data + hdr->data_section_off);
      dsec_count = hdr->data_section_count;
    }
    if(dsecs == 0)
    {
      result = RDI_ParseStatus_InvalidDataSecionLayout;
    }
  }
  
  //////////////////////////////
  //- rjf: fill result
  //
  if(result == RDI_ParseStatus_Good)
  {
    out->raw_data = data;
    out->raw_data_size = size;
    out->sections = dsecs;
    out->sections_count = dsec_count;
  }
  
  //////////////////////////////
  //- rjf: validate results
  //
  if(result == RDI_ParseStatus_Good)
  {
    for(RDI_SectionKind k = (RDI_SectionKind)(RDI_SectionKind_NULL+1); k < RDI_SectionKind_COUNT; k = (RDI_SectionKind)(k+1))
    {
      if(rdi_section_is_required_table[k])
      {
        RDI_U64 data_size = 0;
        RDI_SectionEncoding encoding = 0;
        void *data = rdi_section_raw_data_from_kind(out, k, &encoding, &data_size);
        if(data == 0 || data == &rdi_nil_element_union || data_size == 0)
        {
          result = RDI_ParseStatus_MissingRequiredSection;
          break;
        }
      }
    }
  }
  
  return result;
}

////////////////////////////////
//~ Base Parsed Info Extraction Helpers

//- section table/element raw data extraction

RDI_PROC void *
rdi_section_raw_data_from_kind(RDI_Parsed *rdi, RDI_SectionKind kind, RDI_SectionEncoding *encoding_out, RDI_U64 *size_out)
{
  void *result = 0;
#if !defined(RDI_DISABLE_NILS)
  result = &rdi_nil_element_union;
  *size_out = rdi_section_element_size_table[kind];
#endif
  if(0 <= kind && kind < rdi->sections_count &&
     rdi->sections[kind].off < rdi->raw_data_size)
  {
    result = rdi->raw_data+rdi->sections[kind].off;
    *size_out = rdi->sections[kind].encoded_size;
    *encoding_out = rdi->sections[kind].encoding;
  }
  return result;
}

RDI_PROC void *
rdi_section_raw_table_from_kind(RDI_Parsed *rdi, RDI_SectionKind kind, RDI_U64 *count_out)
{
  void *result = 0;
  RDI_U64 all_elements_size = 0;
  RDI_SectionEncoding all_elements_encoding = 0;
  void *all_elements = rdi_section_raw_data_from_kind(rdi, kind, &all_elements_encoding, &all_elements_size);
  if(all_elements_encoding == RDI_SectionEncoding_Unpacked)
  {
    RDI_U64 element_size = (RDI_U64)rdi_section_element_size_table[kind];
    RDI_U64 all_elements_count = all_elements_size/element_size;
    result = all_elements;
    *count_out = all_elements_count;
  }
  return result;
}

RDI_PROC void *
rdi_section_raw_element_from_kind_idx(RDI_Parsed *rdi, RDI_SectionKind kind, RDI_U64 idx)
{
  RDI_U64 count = 0;
  void *table = rdi_section_raw_table_from_kind(rdi, kind, &count);
  void *result = table;
  if(idx < count)
  {
    RDI_U64 element_size = (RDI_U64)rdi_section_element_size_table[kind];
    result = (RDI_U8 *)table + element_size*idx;
  }
  return result;
}

//- info about whole parse

RDI_PROC RDI_U64
rdi_decompressed_size_from_parsed(RDI_Parsed *rdi)
{
  RDI_U64 decompressed_size = rdi->raw_data_size;
  for(RDI_U64 section_idx = 0; section_idx < rdi->sections_count; section_idx += 1)
  {
    decompressed_size += (rdi->sections[section_idx].unpacked_size - rdi->sections[section_idx].encoded_size);
  }
  return decompressed_size;
}

//- strings

RDI_PROC RDI_U8 *
rdi_string_from_idx(RDI_Parsed *rdi, RDI_U32 idx, RDI_U64 *len_out)
{
  RDI_U8 *result_base = 0;
  RDI_U64 result_size = 0;
  {
    RDI_U64 string_offs_count = 0;
    RDI_U32 *string_offs = rdi_table_from_name(rdi, StringTable, &string_offs_count);
    if(idx < string_offs_count)
    {
      RDI_U64 string_data_size = 0;
      RDI_U8 *string_data = rdi_table_from_name(rdi, StringData, &string_data_size);
      RDI_U32 off_raw = string_offs[idx];
      RDI_U32 opl_raw = string_offs[idx + 1];
      RDI_U32 opl = rdi_parse__min(opl_raw, string_data_size);
      RDI_U32 off = rdi_parse__min(off_raw, opl);
      result_base = string_data + off;
      result_size = opl - off;
    }
  }
  *len_out = result_size;
  return result_base;
}

//- index runs

RDI_PROC RDI_U32*
rdi_idx_run_from_first_count(RDI_Parsed *rdi, RDI_U32 raw_first, RDI_U32 raw_count, RDI_U32 *n_out)
{
  RDI_U64 idx_run_count = 0;
  RDI_U32 *idx_run_data = rdi_table_from_name(rdi, IndexRuns, &idx_run_count);
  RDI_U32 raw_opl = raw_first + raw_count;
  RDI_U32 opl = rdi_parse__min(raw_opl, idx_run_count);
  RDI_U32 first = rdi_parse__min(raw_first, opl);
  RDI_U32 *result = 0;
  if(first < idx_run_count)
  {
    result = idx_run_data + first;
  }
  *n_out = opl - first;
  return result;
}

//- line info

RDI_PROC void
rdi_parsed_from_line_table(RDI_Parsed *rdi, RDI_LineTable *line_table, RDI_ParsedLineTable *out)
{
  //- rjf: extract top-level line info tables
  RDI_U64 all_voffs_count = 0;
  RDI_U64 *all_voffs = rdi_table_from_name(rdi, LineInfoVOffs, &all_voffs_count);
  RDI_U64 *all_voffs_opl = all_voffs + all_voffs_count;
  RDI_U64 all_lines_count = 0;
  RDI_Line *all_lines = rdi_table_from_name(rdi, LineInfoLines, &all_lines_count);
  RDI_Line *all_lines_opl = all_lines + all_lines_count;
  RDI_U64 all_cols_count = 0;
  RDI_Column *all_cols = rdi_table_from_name(rdi, LineInfoColumns, &all_cols_count);
  RDI_Column *all_cols_opl = all_cols + all_cols_count;
  
  //- rjf: extract ranges of top-level tables belonging to this line table
  RDI_U64    *lt_voffs = all_voffs + line_table->voffs_base_idx;
  RDI_Line   *lt_lines = all_lines + line_table->lines_base_idx;
  RDI_Column *lt_cols  = all_cols  + line_table->cols_base_idx;
  RDI_U64 lines_count = line_table->lines_count;
  RDI_U64 cols_count  = line_table->cols_count;
  if(lt_voffs >= all_voffs_opl) {lt_voffs = all_voffs; lines_count = 0;}
  if(lt_lines >= all_lines_opl) {lt_lines = all_lines; lines_count = 0;}
  if(lt_cols  >= all_cols_opl)  {lt_cols  = all_cols;  cols_count = 0;}
  
  //- rjf: fill result
  out->voffs     = lt_voffs;
  out->lines     = lt_lines;
  out->cols      = lt_cols;
  out->count     = lines_count;
  out->col_count = cols_count;
}

RDI_PROC RDI_U64
rdi_line_info_idx_range_from_voff(RDI_ParsedLineTable *line_info, RDI_U64 voff, RDI_U64 *n_out)
{
  RDI_U64 result = 0;
  RDI_U64 n = 0;
  if(line_info->count > 0 && line_info->voffs[0] <= voff && voff < line_info->voffs[line_info->count - 1])
  {
    //- rjf: find i such that: (vmap[i].voff <= voff) && (voff < vmap[i + 1].voff)
    // assuming: (i < j) -> (vmap[i].voff < vmap[j].voff)
    RDI_U32 first = 0;
    RDI_U32 opl   = line_info->count;
    for(;;)
    {
      RDI_U32 mid = (first + opl)/2;
      if(line_info->voffs[mid] < voff)
      {
        first = mid;
      }
      else if(line_info->voffs[mid] > voff)
      {
        opl = mid;
      }
      else
      {
        first = mid;
        break;
      }
      if(opl - first <= 1)
      {
        break;
      }
    }
    result = (RDI_U64)first;
    
    //- rjf: scan leftward, to find shallowest line info matching this voff
    for(;result != 0;)
    {
      if(line_info->voffs[result-1] == voff)
      {
        result -= 1;
      }
      else
      {
        break;
      }
    }
    
    //- rjf: scan rightward, to count # of line info with this voff
    for(RDI_U64 idx = result; idx < line_info->count; idx += 1)
    {
      if(line_info->voffs[idx] == voff)
      {
        n += 1;
      }
      else
      {
        break;
      }
    }
  }
  if(n_out)
  {
    *n_out = n;
  }
  return result;
}

RDI_PROC RDI_U64
rdi_line_info_idx_from_voff(RDI_ParsedLineTable *line_info, RDI_U64 voff)
{
  RDI_U64 count = 0;
  RDI_U64 result = rdi_line_info_idx_range_from_voff(line_info, voff, &count);
  for(RDI_S64 idx = count-1; idx >= 0; idx -= 1)
  {
    if(result + idx < line_info->count && line_info->lines[result+idx].file_idx != 0)
    {
      result += idx;
      break;
    }
  }
  return result;
}

RDI_PROC void
rdi_parsed_from_source_line_map(RDI_Parsed *rdi, RDI_SourceLineMap *map, RDI_ParsedSourceLineMap *out)
{
  //- rjf: extract top-level line info tables
  RDI_U64 all_nums_count = 0;
  RDI_U32 *all_nums = rdi_table_from_name(rdi, SourceLineMapNumbers, &all_nums_count);
  RDI_U32 *all_nums_opl = all_nums + all_nums_count;
  RDI_U64 all_rngs_count = 0;
  RDI_U32 *all_rngs = rdi_table_from_name(rdi, SourceLineMapRanges, &all_rngs_count);
  RDI_U32 *all_rngs_opl = all_rngs + all_rngs_count;
  RDI_U64 all_voffs_count = 0;
  RDI_U64 *all_voffs = rdi_table_from_name(rdi, SourceLineMapVOffs, &all_voffs_count);
  RDI_U64 *all_voffs_opl = all_voffs + all_voffs_count;
  
  //- rjf: extract ranges of top-level tables belonging to this line map
  RDI_U32 *map_nums = all_nums + map->line_map_nums_base_idx;
  RDI_U32 *map_rngs = all_rngs + map->line_map_range_base_idx;
  RDI_U64 *map_voffs= all_voffs+ map->line_map_voff_base_idx;
  RDI_U64 lines_count = (RDI_U64)map->line_count;
  RDI_U64 voffs_count = (RDI_U64)map->voff_count;
  if(map_nums >= all_nums_opl) {map_nums = all_nums; lines_count = 0;}
  if(map_rngs >= all_rngs_opl) {map_rngs = all_rngs; lines_count = 0;}
  if(map_voffs>= all_voffs_opl){map_voffs= all_voffs;voffs_count = 0;}
  
  //- rjf: fill result
  out->nums       = map_nums;
  out->ranges     = map_rngs;
  out->voffs      = map_voffs;
  out->count      = lines_count;
  out->voff_count = voffs_count;
}

RDI_PROC RDI_U64 *
rdi_line_voffs_from_num(RDI_ParsedSourceLineMap *map, RDI_U32 linenum, RDI_U32 *n_out)
{
  RDI_U64 *result = 0;
  *n_out = 0;
  RDI_U32 closest_i = 0;
  if(map->count > 0 && map->nums[0] <= linenum)
  {
    // assuming: (i < j) -> (nums[i] < nums[j])
    // find i such that: (nums[i] <= linenum) && (linenum < nums[i + 1])
    RDI_U32 *nums = map->nums;
    RDI_U32 first = 0;
    RDI_U32 opl   = map->count;
    for(;;)
    {
      RDI_U32 mid = (first + opl)/2;
      if(nums[mid] < linenum)
      {
        first = mid;
      }
      else if(nums[mid] > linenum)
      {
        opl = mid;
      }
      else
      {
        first = mid;
        break;
      }
      if(opl - first <= 1)
      {
        break;
      }
    }
    closest_i = first;
  }
  
  // round up instead of down if possible
  if(closest_i + 1 < map->count && map->nums[closest_i] < linenum)
  {
    closest_i += 1;
  }
  
  // set result if possible
  if(closest_i < map->count)
  {
    RDI_U32 first = map->ranges[closest_i];
    RDI_U32 opl   = map->ranges[closest_i + 1];
    if(opl <= map->voff_count)
    {
      result = map->voffs + first;
      *n_out = opl - first;
    }
  }
  
  return result;
}

//- vmap lookups

RDI_PROC RDI_U64
rdi_vmap_idx_from_voff(RDI_VMapEntry *vmap, RDI_U64 vmap_count, RDI_U64 voff)
{
  RDI_U64 result = 0;
  if(vmap_count > 0 && vmap[0].voff <= voff && voff < vmap[vmap_count - 1].voff)
  {
    // assuming: (i < j) -> (vmap[i].voff < vmap[j].voff)
    // find i such that: (vmap[i].voff <= voff) && (voff < vmap[i + 1].voff)
    RDI_U32 first = 0;
    RDI_U32 opl   = vmap_count;
    for(;;)
    {
      RDI_U32 mid = (first + opl)/2;
      if(vmap[mid].voff < voff)
      {
        first = mid;
      }
      else if(vmap[mid].voff > voff)
      {
        opl = mid;
      }
      else
      {
        first = mid;
        break;
      }
      if(opl - first <= 1)
      {
        break;
      }
    }
    result = (RDI_U64)vmap[first].idx;
  }
  return result;
}

RDI_PROC RDI_U64
rdi_vmap_idx_from_section_kind_voff(RDI_Parsed *rdi, RDI_SectionKind kind, RDI_U64 voff)
{
  RDI_U64 vmaps_count = 0;
  RDI_VMapEntry *vmaps = rdi_section_raw_table_from_kind(rdi, kind, &vmaps_count);
  RDI_U64 result = rdi_vmap_idx_from_voff(vmaps, vmaps_count, voff);
  return result;
}

//- name maps

RDI_PROC void
rdi_parsed_from_name_map(RDI_Parsed *rdi, RDI_NameMap *mapptr, RDI_ParsedNameMap *out)
{
  out->buckets = 0;
  out->bucket_count = 0;
  if(mapptr != 0)
  {
    RDI_U64 all_buckets_count = 0;
    RDI_NameMapBucket *all_buckets = rdi_table_from_name(rdi, NameMapBuckets, &all_buckets_count);
    RDI_U64 all_nodes_count = 0;
    RDI_NameMapNode *all_nodes = rdi_table_from_name(rdi, NameMapNodes, &all_nodes_count);
    out->buckets = all_buckets+mapptr->bucket_base_idx;
    out->nodes = all_nodes+mapptr->node_base_idx;
    out->bucket_count = mapptr->bucket_count;
    out->node_count = mapptr->node_count;
    if(mapptr->bucket_base_idx > all_buckets_count)
    {
      out->buckets = 0;
      out->bucket_count = 0;
    }
    if(mapptr->node_base_idx > all_nodes_count)
    {
      out->nodes = 0;
      out->node_count = 0;
    }
  }
}

RDI_PROC RDI_NameMapNode*
rdi_name_map_lookup(RDI_Parsed *p, RDI_ParsedNameMap *map, RDI_U8 *str, RDI_U64 len)
{
  RDI_NameMapNode *result = 0;
  if(map->bucket_count > 0)
  {
    RDI_NameMapBucket *buckets = map->buckets;
    RDI_U64 bucket_count = map->bucket_count;
    RDI_U64 hash = rdi_hash(str, len);
    RDI_U64 bucket_index = hash%bucket_count;
    RDI_NameMapBucket *bucket = map->buckets + bucket_index;
    RDI_NameMapNode *node = map->nodes + bucket->first_node;
    RDI_NameMapNode *node_opl = node + bucket->node_count;
    for(;node < node_opl; node += 1)
    {
      // extract a string from this node
      RDI_U64 nlen = 0;
      RDI_U8 *nstr = rdi_string_from_idx(p, node->string_idx, &nlen);
      
      // compare this to the needle string
      RDI_S32 match = 0;
      if(nlen == len)
      {
        RDI_U8 *a = str;
        RDI_U8 *aopl = str + len;
        RDI_U8 *b = nstr;
        for (;a < aopl && *a == *b; a += 1, b += 1);
        match = (a == aopl);
      }
      
      // stop with a matching node in result
      if(match)
      {
        result = node;
        break;
      }
      
    }
  }
  return result;
}

RDI_PROC RDI_U32*
rdi_matches_from_map_node(RDI_Parsed *p, RDI_NameMapNode *node, RDI_U32 *n_out)
{
  RDI_U32 *result = 0;
  *n_out = 0;
  if(node != 0)
  {
    if(node->match_count == 1)
    {
      result = &node->match_idx_or_idx_run_first;
      *n_out = 1;
    }
    else
    {
      result = rdi_idx_run_from_first_count(p, node->match_idx_or_idx_run_first, node->match_count, n_out);
    }
  }
  return result;
}

////////////////////////////////
//~ High-Level Composite Lookup Functions

//- procedures

RDI_PROC RDI_Procedure *
rdi_procedure_from_name(RDI_Parsed *rdi, RDI_U8 *name, RDI_U64 name_size)
{
  RDI_NameMap *map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_Procedures);
  RDI_ParsedNameMap map_parsed = {0};
  rdi_parsed_from_name_map(rdi, map, &map_parsed);
  RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map_parsed, name, name_size);
  RDI_U32 id_count = 0;
  RDI_U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
  RDI_U32 procedure_idx = 0;
  if(id_count > 0)
  {
    procedure_idx = ids[0];
  }
  RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, procedure_idx);
  return procedure;
}

RDI_PROC RDI_Procedure *
rdi_procedure_from_name_cstr(RDI_Parsed *rdi, char *cstr)
{
  RDI_Procedure *result = rdi_procedure_from_name(rdi, (RDI_U8 *)cstr, rdi_cstring_length(cstr));
  return result;
}

RDI_PROC RDI_U8 *
rdi_name_from_procedure(RDI_Parsed *rdi, RDI_Procedure *procedure, RDI_U64 *len_out)
{
  return rdi_string_from_idx(rdi, procedure->name_string_idx, len_out);
}

RDI_PROC RDI_Scope *
rdi_root_scope_from_procedure(RDI_Parsed *rdi, RDI_Procedure *procedure)
{
  RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, procedure->root_scope_idx);
  return scope;
}

RDI_PROC RDI_UDT *
rdi_container_udt_from_procedure(RDI_Parsed *rdi, RDI_Procedure *procedure)
{
  RDI_U64 idx = 0;
  if(procedure->link_flags & RDI_LinkFlag_TypeScoped)
  {
    idx = procedure->container_idx;
  }
  RDI_UDT *udt = rdi_element_from_name_idx(rdi, UDTs, idx);
  return udt;
}

RDI_PROC RDI_Procedure *
rdi_container_procedure_from_procedure(RDI_Parsed *rdi, RDI_Procedure *procedure)
{
  RDI_U64 idx = 0;
  if(procedure->link_flags & RDI_LinkFlag_ProcScoped)
  {
    idx = procedure->container_idx;
  }
  RDI_Procedure *container_procedure = rdi_element_from_name_idx(rdi, Procedures, idx);
  return container_procedure;
}

RDI_PROC RDI_U64
rdi_first_voff_from_procedure(RDI_Parsed *rdi, RDI_Procedure *procedure)
{
  RDI_Scope *scope = rdi_root_scope_from_procedure(rdi, procedure);
  RDI_U64 result = rdi_first_voff_from_scope(rdi, scope);
  return result;
}

RDI_PROC RDI_U64
rdi_opl_voff_from_procedure(RDI_Parsed *rdi, RDI_Procedure *procedure)
{
  RDI_Scope *scope = rdi_root_scope_from_procedure(rdi, procedure);
  RDI_U64 result = rdi_opl_voff_from_scope(rdi, scope);
  return result;
}

RDI_PROC RDI_Procedure *
rdi_procedure_from_voff(RDI_Parsed *rdi, RDI_U64 voff)
{
  RDI_Scope *scope = rdi_scope_from_voff(rdi, voff);
  RDI_Procedure *procedure = rdi_procedure_from_scope(rdi, scope);
  return procedure;
}

//- scopes

RDI_PROC RDI_U64
rdi_first_voff_from_scope(RDI_Parsed *rdi, RDI_Scope *scope)
{
  RDI_U64 *voffs = rdi_element_from_name_idx(rdi, ScopeVOffData, scope->voff_range_first);
  RDI_U64 result = *voffs;
  return result;
}

RDI_PROC RDI_U64
rdi_opl_voff_from_scope(RDI_Parsed *rdi, RDI_Scope *scope)
{
  RDI_U64 result = 0;
  if(scope->voff_range_opl != 0)
  {
    RDI_U64 *voffs = rdi_element_from_name_idx(rdi, ScopeVOffData, scope->voff_range_opl-1);
    result = *voffs;
  }
  return result;
}

RDI_PROC RDI_Scope *
rdi_scope_from_voff(RDI_Parsed *rdi, RDI_U64 voff)
{
  RDI_U32 idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff);
  RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, idx);
  return scope;
}

RDI_PROC RDI_Scope *
rdi_parent_from_scope(RDI_Parsed *rdi, RDI_Scope *scope)
{
  RDI_Scope *parent = rdi_element_from_name_idx(rdi, Scopes, scope->parent_scope_idx);
  return parent;
}

RDI_PROC RDI_Procedure *
rdi_procedure_from_scope(RDI_Parsed *rdi, RDI_Scope *scope)
{
  RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, scope->proc_idx);
  return procedure;
}

RDI_PROC RDI_InlineSite *
rdi_inline_site_from_scope(RDI_Parsed *rdi, RDI_Scope *scope)
{
  RDI_InlineSite *inline_site = rdi_element_from_name_idx(rdi, InlineSites, scope->inline_site_idx);
  return inline_site;
}

//- global variables

RDI_PROC RDI_GlobalVariable *
rdi_global_variable_from_voff(RDI_Parsed *rdi, RDI_U64 voff)
{
  RDI_U32 idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_GlobalVMap, voff);
  RDI_GlobalVariable *gvar = rdi_element_from_name_idx(rdi, GlobalVariables, idx);
  return gvar;
}

//- units

RDI_PROC RDI_Unit *
rdi_unit_from_voff(RDI_Parsed *rdi, RDI_U64 voff)
{
  RDI_U32 unit_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_UnitVMap, voff);
  RDI_Unit *unit = rdi_element_from_name_idx(rdi, Units, unit_idx);
  return unit;
}

RDI_PROC RDI_LineTable *
rdi_line_table_from_unit(RDI_Parsed *rdi, RDI_Unit *unit)
{
  RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, unit->line_table_idx);
  return line_table;
}

//- line info

RDI_PROC RDI_Line
rdi_line_from_voff(RDI_Parsed *rdi, RDI_U64 voff)
{
  RDI_Unit *unit = rdi_unit_from_voff(rdi, voff);
  RDI_LineTable *line_table = rdi_line_table_from_unit(rdi, unit);
  RDI_Line line = rdi_line_from_line_table_voff(rdi, line_table, voff);
  return line;
}

RDI_PROC RDI_Line
rdi_line_from_line_table_voff(RDI_Parsed *rdi, RDI_LineTable *line_table, RDI_U64 voff)
{
  RDI_ParsedLineTable parsed = {0};
  rdi_parsed_from_line_table(rdi, line_table, &parsed);
  RDI_U64 line_info_idx = rdi_line_info_idx_from_voff(&parsed, voff);
  RDI_Line result = {0};
  if(line_info_idx < parsed.count)
  {
    result = parsed.lines[line_info_idx];
  }
  return result;
}

RDI_PROC RDI_SourceFile *
rdi_source_file_from_line(RDI_Parsed *rdi, RDI_Line *line)
{
  RDI_SourceFile *result = rdi_element_from_name_idx(rdi, SourceFiles, line->file_idx);
  return result;
}

//- source files

RDI_PROC RDI_SourceFile *
rdi_source_file_from_normal_path(RDI_Parsed *rdi, RDI_U8 *name, RDI_U64 name_size)
{
  RDI_NameMap *map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_NormalSourcePaths);
  RDI_ParsedNameMap map_parsed = {0};
  rdi_parsed_from_name_map(rdi, map, &map_parsed);
  RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map_parsed, name, name_size);
  RDI_U32 id_count = 0;
  RDI_U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
  RDI_U32 file_idx = 0;
  if(id_count > 0)
  {
    file_idx = ids[0];
  }
  RDI_SourceFile *file = rdi_element_from_name_idx(rdi, SourceFiles, file_idx);
  return file;
}

RDI_PROC RDI_SourceFile *
rdi_source_file_from_normal_path_cstr(RDI_Parsed *rdi, char *cstr)
{
  RDI_SourceFile *result = rdi_source_file_from_normal_path(rdi, (RDI_U8 *)cstr, rdi_cstring_length(cstr));
  return result;
}

RDI_PROC RDI_U8 *
rdi_normal_path_from_source_file(RDI_Parsed *rdi, RDI_SourceFile *src_file, RDI_U64 *len_out)
{
  return rdi_string_from_idx(rdi, src_file->normal_full_path_string_idx, len_out);
}

RDI_PROC RDI_FilePathNode *
rdi_file_path_node_from_source_file(RDI_Parsed *rdi, RDI_SourceFile *src_file)
{
  RDI_FilePathNode *result = rdi_element_from_name_idx(rdi, FilePathNodes, src_file->file_path_node_idx);
  return result;
}

RDI_PROC RDI_SourceLineMap *
rdi_source_line_map_from_source_file(RDI_Parsed *rdi, RDI_SourceFile *src_file)
{
  RDI_SourceLineMap *result = rdi_element_from_name_idx(rdi, SourceLineMaps, src_file->source_line_map_idx);
  return result;
}

RDI_PROC RDI_U64
rdi_first_voff_from_source_file_line_num(RDI_Parsed *rdi, RDI_SourceFile *src_file, RDI_U32 line_num)
{
  RDI_SourceLineMap *source_line_map = rdi_source_line_map_from_source_file(rdi, src_file);
  RDI_U64 voff = rdi_first_voff_from_source_line_map_num(rdi, source_line_map, line_num);
  return voff;
}

//- source line maps

RDI_PROC RDI_U64
rdi_first_voff_from_source_line_map_num(RDI_Parsed *rdi, RDI_SourceLineMap *map, RDI_U32 line_num)
{
  RDI_ParsedSourceLineMap parsed = {0};
  rdi_parsed_from_source_line_map(rdi, map, &parsed);
  RDI_U32 all_voffs_count = 0;
  RDI_U64 *all_voffs = rdi_line_voffs_from_num(&parsed, line_num, &all_voffs_count);
  RDI_U64 voff = 0;
  if(all_voffs_count != 0)
  {
    voff = all_voffs[0];
  }
  return voff;
}

//- file path nodes

RDI_PROC RDI_FilePathNode *
rdi_parent_from_file_path_node(RDI_Parsed *rdi, RDI_FilePathNode *node)
{
  RDI_FilePathNode *result = rdi_element_from_name_idx(rdi, FilePathNodes, node->parent_path_node);
  return result;
}

RDI_PROC RDI_U8 *
rdi_name_from_file_path_node(RDI_Parsed *rdi, RDI_FilePathNode *node, RDI_U64 *len_out)
{
  return rdi_string_from_idx(rdi, node->name_string_idx, len_out);
}

////////////////////////////////
//~ Parser Helpers

RDI_PROC RDI_U64
rdi_cstring_length(char *cstr)
{
  RDI_U64 result = 0;
  for(;cstr[result] != 0; result += 1){}
  return result;
}

RDI_PROC RDI_U64
rdi_size_from_bytecode_stream(RDI_U8 *ptr, RDI_U8 *opl)
{
  RDI_U64 bytecode_size = 0;
  RDI_U8 *off_first = ptr + sizeof(RDI_LocationKind);
  for(RDI_U8 *off = off_first, *next_off = opl; off < opl; off = next_off)
  {
    RDI_U8 op = *off;
    if(op == 0)
    {
      break;
    }
    
    RDI_U16 ctrlbits = rdi_eval_op_ctrlbits_table[op];
    RDI_U32 p_size   = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
    bytecode_size += (1 + p_size);
    next_off = (off + 1 + p_size);
  }
  return bytecode_size;
}

////////////////////////////////
//~ Compression/Decompression Implementation

#ifndef _RAD_LZB_SIMPLE_H_
#define _RAD_LZB_SIMPLE_H_

/*======================================================

To encode :

	Set up an rr_lzb_simple_context
	
	fill out m_tableSizeBits (14-16 is typical)
	
	allocate m_hashTable

	rr_lzb_simple_context c;
	c.m_tableSizeBits = 14;
	c.m_hashTable = OODLE_MALLOC_ARRAY(U16,RR_ONE_SA<<c.m_tableSizeBits);
	
	then call _encode

NOTE :
	compressed & raw size are not included in the encoded bytes.  You must send
	them separately.
	
NOTE :
	lzb will never expand.  comp_len is <= raw_len strictly.
	if comp_len = raw_len it indicates that the compressed bytes are just a memcpy
	of the raw bytes.  In that case you do not need to decode.
	
To decode :

	if comp_len is == raw_len, then the compressed bytes are just a copy of the 
	raw bytes and you could use them directly without calling decode.
	
	if you call rr_lzb_simple_decode in that case, then the compressed buffer will
	be memcpy'd to the raw buffer

===============================================================*/

//~ TODO(rjf): temporary glue for building this without the shared rad code:

#define __RAD64REGS__

#include <stdint.h>
typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;
typedef int64_t  S64;

typedef S64 SINTa;
typedef U64 RAD_U64;
typedef S64 RAD_S64;
typedef U32 RAD_U32;
typedef S32 RAD_S32;

#define RADINLINE __inline

#if defined(_MSC_VER)
# define RADFORCEINLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
# define RADFORCEINLINE __attribute__((always_inline))
#else
# error need force inline for this compiler
#endif

#if _MSC_VER
# define RADLZB_TRAP() __debugbreak()
#elif __clang__ || __GNUC__
# define RADLZB_TRAP() __builtin_trap()
#else
# error Unknown trap intrinsic for this compiler.
#endif

#define RR_STRING_JOIN(arg1, arg2)              RR_STRING_JOIN_DELAY(arg1, arg2)
#define RR_STRING_JOIN_DELAY(arg1, arg2)        RR_STRING_JOIN_IMMEDIATE(arg1, arg2)
#define RR_STRING_JOIN_IMMEDIATE(arg1, arg2)    arg1 ## arg2

#ifdef _MSC_VER
#define RR_NUMBERNAME(name) RR_STRING_JOIN(name,__COUNTER__)
#else
#define RR_NUMBERNAME(name) RR_STRING_JOIN(name,__LINE__)
#endif

#define RR_COMPILER_ASSERT(exp)   typedef char RR_NUMBERNAME(_dummy_array) [ (exp) ? 1 : -1 ]

#if defined(__clang__)
# define Expect(expr, val) __builtin_expect((expr), (val))
#else
# define Expect(expr, val) (expr)
#endif

#define RAD_LIKELY(expr)            Expect(expr,1)
#define RAD_UNLIKELY(expr)          Expect(expr,0)

#define __RADLITTLEENDIAN__ 1
#define RAD_PTRBYTES 8
#define RR_MIN(a,b)    ( (a) < (b) ? (a) : (b) )
#define RR_MAX(a,b)    ( (a) > (b) ? (a) : (b) )
#define RR_ASSERT_ALWAYS(c) do{if(!(c)) {RADLZB_TRAP();}}while(0)
#define RR_ASSERT(c) RR_ASSERT_ALWAYS(c)

#define RR_PUT16_LE(ptr,val)       *((U16 *)(ptr)) = (U16)(val)
#define RR_GET16_LE_UNALIGNED(ptr) *((const U16 *)(ptr))

static RADINLINE U32
rrCtzBytes32(U32 val)
{
  // Don't get fancy here. Assumes val != 0.
  if (val & 0x000000ffu) return 0;
  if (val & 0x0000ff00u) return 1;
  if (val & 0x00ff0000u) return 2;
  return 3;
}

static RADINLINE U32
rrCtzBytes64(U64 val)
{
  U32 lo = (U32) val;
  return lo ? rrCtzBytes32(lo) : 4 + rrCtzBytes32((U32) (val >> 32));
}

//~

//---------------------

typedef struct rr_lzb_simple_context rr_lzb_simple_context;
struct rr_lzb_simple_context
{
	U16	*	m_hashTable;	// must be allocated to sizeof(U16)*(1<<m_tableSizeBits)
	S32		m_tableSizeBits;
};

SINTa rr_lzb_simple_encode_fast(rr_lzb_simple_context * ctx,
                                const void * raw, SINTa rawLen, void * comp);

SINTa rr_lzb_simple_encode_veryfast(rr_lzb_simple_context * ctx,
                                    const void * raw, SINTa rawLen, void * comp);

//---------------------

// rr_lzb_simple_decode returns the number of compressed bytes consumed ( == compLen)
SINTa rr_lzb_simple_decode(const void * comp, SINTa compLen, void * raw, SINTa rawLen);

//---------------------

#endif // _RAD_LZB_SIMPLE_H_
#include <string.h>

//-------------------------------------------------
// UINTr = int the size of a register

#ifdef __RAD64REGS__

#define RAD_UINTr RAD_U64
#define RAD_SINTr RAD_S64

#define readR read64
#define writeR write64

#define rrClzBytesR rrClzBytes64
#define rrCtzBytesR rrCtzBytes64

#else

#define RAD_UINTr RAD_U32
#define RAD_SINTr RAD_S32

#define readR read32
#define writeR write32

#define rrClzBytesR rrClzBytes32
#define rrCtzBytesR rrCtzBytes32

#endif

typedef RAD_SINTr SINTr;
typedef RAD_UINTr UINTr;

#define OOINLINE	RADFORCEINLINE

#define if_unlikely(exp)	if ( RAD_UNLIKELY( exp ) )
#define if_likely(  exp)	if ( RAD_LIKELY( exp ) )

// Raw byte IO

#if defined(__RADARM__) && !defined(__RAD64__) && defined(__GNUC__)

// older GCCs don't turn the memcpy variant into loads/stores, but
// they do support this:
typedef union
{
	U16 u16;
	U32 u32; 
	U64 u64; 
} __attribute__((packed)) unaligned_type;

static inline U16 read16(const void *ptr) 		{ return ((const unaligned_type *)ptr)->u16; }
static inline void write16(void *ptr, U16 x) 	{ ((unaligned_type *)ptr)->u16 = x; }

static inline U32 read32(const void *ptr) 		{ return ((const unaligned_type *)ptr)->u32; }
static inline void write32(void *ptr, U32 x) 	{ ((unaligned_type *)ptr)->u32 = x; }

static inline U64 read64(const void *ptr) 		{ return ((const unaligned_type *)ptr)->u64; }
static inline void write64(void *ptr, U64 x) 	{ ((unaligned_type *)ptr)->u64 = x; }

#else

// most C compilers we target are smart enough to turn this into single loads/stores
static inline U16 read16(const void *ptr) 		{ U16 x; memcpy(&x, ptr, sizeof(x)); return x; }
static inline void write16(void *ptr, U16 x) 	{ memcpy(ptr, &x, sizeof(x)); }

static inline U32 read32(const void *ptr) 		{ U32 x; memcpy(&x, ptr, sizeof(x)); return x; }
static inline void write32(void *ptr, U32 x) 	{ memcpy(ptr, &x, sizeof(x)); }

static inline U64 read64(const void *ptr) 		{ U64 x; memcpy(&x, ptr, sizeof(x)); return x; }
static inline void write64(void *ptr, U64 x) 	{ memcpy(ptr, &x, sizeof(x)); }

#endif

#define RR_PUT16_LE_UNALIGNED(ptr,val)                 RR_PUT16_LE(ptr,val)
#define RR_PUT16_LE_UNALIGNED_OFFSET(ptr,val,offset)   RR_PUT16_LE_OFFSET(ptr,val,offset)

//===========================================================================

static RADINLINE SINTa rrPtrDiffV(void * end, void *start) { return (SINTa)( ((char *)(end)) - ((char *)(start)) ); }

// helper function to show I really am intending to put a pointer difference in an int :
static RADINLINE SINTa rrPtrDiff(SINTa val) { return val; }
static RADINLINE S32 rrPtrDiff32(SINTa val) { S32 ret = (S32) val; RR_ASSERT( (SINTa)ret == val ); return ret; }
static RADINLINE SINTr rrPtrDiffR(SINTa val) { SINTr ret = (SINTr) val; RR_ASSERT( (SINTa)ret == val ); return ret; }

//=================================================================

#define LZB_LRL_BITS	4
#define LZB_LRL_ESCAPE	15

#define LZB_ML_BITS		4
#define LZB_MLCONTROL_ESCAPE	15

#define LZB_SLIDING_WINDOW_POW2	16
#define LZB_SLIDING_WINDOW_SIZE	(1<<LZB_SLIDING_WINDOW_POW2)

#define LZB_MAX_OFFSET		0xFFFF

#define LZB_MML		4		// should be 3 if I had LO

#define LZB_MATCHLEN_ESCAPE		(LZB_MLCONTROL_ESCAPE+4)


#define LZB_END_WITH_LITERALS			1	// @@ ??
//#define LZB_END_WITH_LITERALS			0	// @@ ??
#define LZB_END_OF_BLOCK_NO_MATCH_ZONE	8

/**

NOTE ABOUT LZB_END_OF_BLOCK_NO_MATCH_ZONE

The limitation in LZB does not actually come from the 8-at-a-time match copier

it comes from the unconditional 8-byte LRL copy

that means the last 8 bytes of every block *must* be literals

(note that's *block* not quantum)

The constraint due to matches is actually weaker
(match len rounded up to next multiple of 8 must not go past block end)

**/

// decode speed on lzt99 :
// LZ4 :      1715.10235

#define LZB_FORCELASTLRL9	1

//=======================================

#define lz_copywordstep(d,s)			do { writeR(d, readR(s)); (s) += sizeof(UINTr); (d) += sizeof(UINTr); } while(0)
#define lz_copywordsteptoend(d,s,e)		do { lz_copywordstep(d,s); } while ((d)<(e))

// lz_copysteptoend_overrunok
// NOTE : unlike memcpy, adjusts dest pointer to end !
#define lz_copysteptoend_overrunok(d,s,l)	do { U8 * e=(d)+(l); lz_copywordsteptoend(d,s,e); d=e; } while(0)

//=======================================

#define LZB_PutExcessBW(cp,val)	do { \
if ( val < 192 ) *cp++ = (U8) val; \
else { val -= 192; *cp++ = 192 + (U8) ( val&0x3F); val >>= 6; \
if ( val < 128 ) *cp++ = (U8) val; \
else { val -= 128; *cp++ = 128 + (U8) ( val&0x7F); val >>= 7; \
if ( val < 128 ) *cp++ = (U8) val; \
else { val -= 128; *cp++ = 128 + (U8) ( val&0x7F); val >>= 7; \
if ( val < 128 ) *cp++ = (U8) val; \
else { val -= 128; *cp++ = 128 + (U8) ( val&0x7F); val >>= 7; *cp++ = (U8) val; } } } } \
} while(0)

// max bytes consumed: 5
#define LZB_AddExcessBW(cp,val)	do { U32 b = *cp++; \
if ( b < 192 ) val += b; \
else { val += 192; val += (b-192); b = *cp++; \
val += (b<<6); if ( b >= 128 ) { b = *cp++; \
val += (b<<13); if ( b >= 128 ) { b = *cp++; \
val += (b<<20); if ( b >= 128 ) { b = *cp++; \
val += (b<<27); } } } } \
} while(0)

#define LZB_PutExcessLRL(cp,val) LZB_PutExcessBW(cp,val)
#define LZB_PutExcessML(cp,val)  LZB_PutExcessBW(cp,val)

#define LZB_AddExcessLRL(cp,val) LZB_AddExcessBW(cp,val)
#define LZB_AddExcessML(cp,val)  LZB_AddExcessBW(cp,val)

//=============================================================================
// match copies :

// used for LRL :
static OOINLINE void copy_no_overlap_long(U8 * to, const U8 * from, SINTr length)
{
	for(int i=0;i<length;i+=8)
		write64(to+i, read64(from+i));
}

static OOINLINE void copy_no_overlap_nooverrun(U8 * to, const U8 * from, SINTr length)
{
	// used for final LRL of every block
	//  must not overrun
	memmove(to,from,(size_t)length);
}

RR_COMPILER_ASSERT( LZB_MLCONTROL_ESCAPE == 15 );
RR_COMPILER_ASSERT( LZB_MATCHLEN_ESCAPE == 19 );

static OOINLINE void copy_match_short_overlap(U8 * to, const U8 * from, SINTr ml)
{
	RR_ASSERT( ml >= LZB_MML && ml < LZB_MATCHLEN_ESCAPE );
  
	// overlap
	// @@ err not awesome
	to[0] = from[0];
	to[1] = from[1];
	to[2] = from[2];
	to[3] = from[3];
	to[4] = from[4];
	to[5] = from[5];
	to[6] = from[6];
	to[7] = from[7];
	if ( ml > 8 )
	{
		to += 8; from += 8; ml -= 8;
		// max of 10 more
		while(ml--)
		{
			*to++ = *from++;
		}
	}
}

static OOINLINE void copy_match_memset(U8 * to, int c, SINTr ml)
{
	RR_ASSERT( ml >= 4 );
	U32 four = c * 0x01010101;
	U8 * end = to + ml;
	write32(to, four); to += 4;
	while(to<end)
	{
		write32(to, four); to += 4;
	}
}

//=============================================================================

static SINTa rr_lzb_simple_decode_notexpanded(const void * comp, void * raw, SINTa rawLen)
{
	U8 * rp = (U8 *)raw;
	U8 * rpEnd = rp+rawLen;
  
	const U8 *	cp = (const U8 *)comp;
	
	for(;;)
	{
		RR_ASSERT( rp < rpEnd );
    
		// max bytes consumed (fast paths):
		// - 1 control
		// - lits:
		//   * 15 lits OR
		//   * 5 excess lrl + long lit run
		// - match:
		//   * 2 match offset (short match) OR
		//   * 1 excess code + 5 excess ML (overlap match) OR
		//   * 1 excess code + 5 excess ML (long match)
		//
		// need near-end checks mainly on long lit runs.
    
		UINTr control = *cp++;
    
		UINTr lrl = control & 0xF;
		UINTr ml_control = (control>>4);
    
		// copy 4 literals speculatively :
		write32( rp , read32(cp) );
    
		//RR_ASSERT( lrl >= 8 || ml_control >= 8 );
    
		if ( lrl > 4 )
		{
			// if lrl was <= 8 we did it, else need this :
			if_unlikely ( lrl > 8 )
			{
				if_unlikely ( lrl >= LZB_LRL_ESCAPE )
				{
					LZB_AddExcessLRL( cp, lrl );
          
					// hide the EOF check here ?
					// has to be after the GetExcess
					if_unlikely ( rp+lrl >= rpEnd )
					{	
						RR_ASSERT( rp+lrl == rpEnd );
            
						copy_no_overlap_nooverrun(rp,cp,lrl);
            
						rp += lrl;
						cp += lrl;
						break;
					}
					else
					{
						// total undo of the previous copy	
						copy_no_overlap_long(rp,cp,lrl);
					}
				}
				else // > 8 but not 0xF
				{
					// hide the EOF check here ?
					if_unlikely ( rp+lrl >= rpEnd )
					{	
						if ( lrl == 9 )
						{
							// may be a false 9
							lrl = rrPtrDiff32( rpEnd - rp );
						}
						RR_ASSERT( rp+lrl == rpEnd );
            
						copy_no_overlap_nooverrun(rp,cp,lrl);
            
						rp += lrl;
						cp += lrl;
						break;						
					}
					else
					{
						write32( rp+4 , read32(cp+4) );
						// put 8 more :
						write64( (rp+8) , read64((cp+8)) );
					}
				}
			}
			else
			{
				write32( rp+4 , read32(cp+4) );
			}
		}
    
		rp += lrl;
		cp += lrl;
    
		RR_ASSERT( rp+LZB_MML <= rpEnd );
    
		UINTr ml = ml_control + LZB_MML;
    
		// speculatively grab offset but don't advance cp yet
		UINTr off = RR_GET16_LE_UNALIGNED(cp);
    
		if ( ml_control <= 8 )
		{
			cp += 2; // consume offset
			const U8 * match = rp - off;
      
			RR_ASSERT( ml <= 12 );
      
			write64( rp , read64(match) );
			write32( rp+8 , read32(match+8) );
      
			rp += ml;
			continue;
		}
		else
		{
      
			if_likely( ml_control < LZB_MLCONTROL_ESCAPE ) // short match
			{
				cp += 2; // consume offset
				const U8 * match = rp - off;
        
				RR_ASSERT( off >= 8 || ml <= off );
        
				write64( rp , read64(match) );
				write64( rp+8 , read64(match+8) );
        
				if ( ml > 16 )
				{
					write16( rp+16, read16(match+16) );
				}
			}
			else
			{
				// get 1-byte excess code
				UINTr excesslow = off&127;
				cp++; // consume 1
        
				//if ( excess1 >= 128 )
				if ( off & 128 )
				{				
					ml_control = excesslow >> 3;
					ml = ml_control + LZB_MML;
					if ( ml_control == 0xF )
					{
						// get more ml
						LZB_AddExcessML( cp, ml );
					}	
          
					UINTr myoff = off & 7;
          
					// low offset, can't do 8-byte grabs
					if ( myoff == 1 )
					{
						int c = rp[-1];
						copy_match_memset(rp,c,ml);
					}
					else
					{
						// shit but whatever, very rare
						for(UINTr i=0;i<ml;i++)
						{
							rp[i] = rp[i-myoff];
						}
					}
				}
				else
				{
					UINTr myoff = RR_GET16_LE_UNALIGNED(cp); cp += 2;
					const U8 * match = rp - myoff;
          
					ml += excesslow;
          
					if ( excesslow == 127 )
					{
						// get more ml
						LZB_AddExcessML( cp, ml );
					}
          
					// 8-byte copier :
					copy_no_overlap_long(rp,match,ml);
				}
			}
      
			rp += ml;
		}
	}
  
	RR_ASSERT( rp == rpEnd );
  
	SINTa used = rrPtrDiff( cp - (const U8 *)comp );
	
	RR_ASSERT( used < rawLen );
	
	return used;
}

SINTa rr_lzb_simple_decode(const void * comp, SINTa compLen, void * raw, SINTa rawLen)
{
	RR_ASSERT_ALWAYS( compLen <= rawLen );
	if ( compLen == rawLen )
	{
		memcpy(raw,comp,rawLen);
		return compLen;
	}
	return rr_lzb_simple_decode_notexpanded(comp,raw,rawLen);
}

//=====================================================


static RADINLINE U32 hmf_hash4_32(U32 ptr32)
{
  U32 h = ( ptr32 * 2654435761u );
  h ^= (h>>13);
  return h;
}

#define HashMatchFinder_Hash32	hmf_hash4_32

//=================================================================================

#define LZB_Hash4	hmf_hash4_32

static RADINLINE U32 LZB_SecondHash4(U32 be4)
{
	const U32 m = 0x5bd1e995;
  
	U32 h = be4 * m;
	h += (h>>11);
	
	return h;
}

//=============================================    

static int RADFORCEINLINE GetNumBytesZeroNeverAllR(UINTr x)
{
	RR_ASSERT( x != 0 );
  
#if defined(__RADBIGENDIAN__)
	// big endian, so earlier bytes are at the top
	int nb = (int)rrClzBytesR(x);
#elif defined(__RADLITTLEENDIAN__)
	// little endian, so earlier bytes are at the bottom
	int nb = (int)rrCtzBytesR(x);
#else
#error wtf no endian set
#endif
  
	RR_ASSERT( nb >= 0 && nb < (int)sizeof(UINTr) );
	return nb;
}

//===============================

static RADFORCEINLINE U8 * LZB_Output(U8 * cp, S32 lrl, const U8 * literals,  S32 matchlen ,  S32 mo )
{
	RR_ASSERT( lrl >= 0 );
	RR_ASSERT( matchlen >= LZB_MML );
	RR_ASSERT( mo > 0 && mo <= LZB_MAX_OFFSET );
	
	//rrprintf("[%3d][%3d][%7d]\n",lrl,ml,mo);
  
	S32 sendml = matchlen - LZB_MML;
	
	U32 ml_in_control  = RR_MIN(sendml,LZB_MLCONTROL_ESCAPE);
	
	if ( mo >= 8 ) // no overlap	
	{
		if ( lrl < LZB_LRL_ESCAPE )
		{
			U32 control = lrl | (ml_in_control<<4);
      
			*cp++ = (U8) control;
			
			write64(cp, read64(literals));
			if ( lrl > 8 )
			{
				write64(cp+8, read64(literals+8));
			}
			cp += lrl;
		}
		else
		{
			U32 control = LZB_LRL_ESCAPE | (ml_in_control<<4);
      
			*cp++ = (U8) control;
			
			U32 lrl_excess = lrl - LZB_LRL_ESCAPE;
			LZB_PutExcessLRL(cp,lrl_excess);
      
			// @@ ? is this okay for overrun ?
			lz_copysteptoend_overrunok(cp,literals,lrl);
		}
		
		if ( ml_in_control < LZB_MLCONTROL_ESCAPE )
		{
			RR_ASSERT( (U16)(mo) == mo );
			RR_PUT16_LE_UNALIGNED(cp,(U16)(mo));
			cp += 2;
		}
		else
		{
			U32 ml_excess = sendml - LZB_MLCONTROL_ESCAPE;
			
			// put special first byte, then offset, then remainder
			if ( ml_excess < 127 )
			{
				*cp++ = (U8)ml_excess;
        
				RR_ASSERT( (U16)(mo) == mo );
				RR_PUT16_LE_UNALIGNED(cp,(U16)(mo));
				cp += 2;
			}
			else
			{
				*cp++ = (U8)127;
        
				RR_ASSERT( (U16)(mo) == mo );
				RR_PUT16_LE_UNALIGNED(cp,(U16)(mo));
				cp += 2;
        
				ml_excess -= 127;
				LZB_PutExcessML(cp,ml_excess);
			}
		}
	}
	else
	{
		U32 lrl_in_control = RR_MIN(lrl,LZB_LRL_ESCAPE);
    
    // overlap case
		U32 control = (lrl_in_control) | (LZB_MLCONTROL_ESCAPE<<4);
		
		*cp++ = (U8) control;
		
		if ( lrl_in_control == LZB_LRL_ESCAPE )
		{
			U32 lrl_excess = lrl - LZB_LRL_ESCAPE;
			LZB_PutExcessLRL(cp,lrl_excess);
		}
		
		lz_copysteptoend_overrunok(cp,literals,lrl);
		//cp += lrl;
		
		// special excess1 :
		UINTr excess1 = 128 + (ml_in_control<<3) + mo;
		RR_ASSERT( excess1 < 256 );
		
		*cp++ = (U8)excess1;
		
		if ( ml_in_control == LZB_MLCONTROL_ESCAPE )
		{
			U32 ml_excess = sendml - LZB_MLCONTROL_ESCAPE;
			LZB_PutExcessML(cp,ml_excess);
		}		
	}
	
	return cp;
}

#if LZB_FORCELASTLRL9

static RADINLINE U8 * LZB_OutputLast(U8 * cp, S32 lrl, const U8 * literals )
{
	RR_ASSERT( lrl >= 0 );
	
	//U32 ml = 0;
	//U32 mo = 0;
  
	U32 lrl_in_control = RR_MIN(lrl,LZB_LRL_ESCAPE);
	
#if LZB_END_WITH_LITERALS
	// lrl_in_control must be at least 9
	lrl_in_control = RR_MAX(lrl_in_control,9);
#endif
	
	U32 control = lrl_in_control;
  
	*cp++ = (U8) control;
	
	if ( lrl_in_control == LZB_LRL_ESCAPE )
	{
		U32 lrl_excess = lrl - LZB_LRL_ESCAPE;
		LZB_PutExcessLRL(cp,lrl_excess);
	}
	
	memmove(cp,literals,lrl);
	cp += lrl;
	
	return cp;
}

#else

static RADINLINE U8 * LZB_OutputLast(U8 * cp, S32 lrl, const U8 * literals )
{
	cp = LZB_Output(cp,lrl,literals,LZB_MML,1);
	
	// remove the offset we put :
	cp -= 2;
	
	return cp;
}

#endif

//===============================================================

static void rr_lzb_simple_context_init(rr_lzb_simple_context * ctx) //, const void * base)
{
	RR_ASSERT( ctx->m_tableSizeBits >= 12 && ctx->m_tableSizeBits <= 24 );
	memset(ctx->m_hashTable,0,sizeof(U16)*((SINTa)1<<ctx->m_tableSizeBits));
}

//===============================================================

/*     
#define FAST_HASH_DEPTH_SHIFT   (1) // more depth = more & more compression,
#define DO_FAST_2ND_HASH    //  rate= 30.69 mb/s , 15451369 <- turning this off is the best way to get more speed and less compression
/*/
#define FAST_HASH_DEPTH_SHIFT   (0)
#define DO_FAST_2ND_HASH
/**/

//     lzt99,  24700820,  15475520,  16677179
//encode only      : 0.880 seconds, 1.62 b/hc, rate= 28.08 mb/s

//#define FAST_HASH_DEPTH_SHIFT   (1) // more depth = more & more compression, but slower

#define DO_FAST_UPDATE_MATCH_HASHES 1 // helps compression a lot , like 0.30
//#define DO_FAST_UPDATE_MATCH_HASHES 2 // helps compression a lot , like 0.30
#define DO_FAST_LAZY_MATCH  // also helps a lot , like 0.15
#define DO_FAST_HASH_DWORD		1

#define FAST_MULTISTEP_LITERALS_SHIFT	(5)


//-----------------------
// derived :

/*
#define FAST_HASH_BITS          (FAST_HASH_TOTAL_BITS-FAST_HASH_DEPTH_SHIFT)
#define FAST_HASH_SIZE          (1<<FAST_HASH_BITS)
#define FAST_HASH_MASK          (FAST_HASH_SIZE-1)
*/

#undef FAST_HASH_DEPTH
#define FAST_HASH_DEPTH         (1<<FAST_HASH_DEPTH_SHIFT)

/*
#if FAST_HASH_DEPTH == 1
#error nope
#endif
*/

#undef FAST_HASH_CYCLE_MASK
#define FAST_HASH_CYCLE_MASK    (FAST_HASH_DEPTH-1)

#undef FAST_HASH_INDEX
#if FAST_HASH_DEPTH > 1
#define FAST_HASH_INDEX(h,d)    ( ((h)<<FAST_HASH_DEPTH_SHIFT) + (d) )
#else
#define FAST_HASH_INDEX(h,d)    (h)
#endif

#undef FAST_HASH_FUNC
#define FAST_HASH_FUNC(ptr,dword)	( LZB_Hash4(dword) & hash_table_mask )



static SINTa rr_lzb_simple_encode_fast_sub(rr_lzb_simple_context * fh,
                                           const void * raw, SINTa rawLen, void * comp)
{
	//SIMPLEPROFILE_SCOPE_N(lzbfast_sub,rawLen);
	//THREADPROFILEFUNC();
	
	U8 * cp = (U8 *)comp;
	U8 * compExpandedPtr = cp + rawLen - 8;
  
	const U8 * rp = (const U8 *)raw;
	const U8 * rpEnd = rp+rawLen;
  
	const U8 * rpMatchEnd = rpEnd - LZB_END_OF_BLOCK_NO_MATCH_ZONE;
	
	const U8 * rpEndSafe = rpMatchEnd - LZB_MML;
	
	if ( rpEndSafe <= (U8 *)raw )
	{
		// can't compress
		return rawLen+1;
	}
	
	const U8 * literals_start = rp;
  
#if FAST_HASH_DEPTH > 1
	int hashCycle = 0;
#endif
  
	U16 * hashTable16 = fh->m_hashTable;
	
	int hashTableSizeBits = fh->m_tableSizeBits;
	U32 hash_table_mask = (U32)((1UL<<(hashTableSizeBits - FAST_HASH_DEPTH_SHIFT)) - 1);
	
	const U8 * zeroPosPtr = (const U8 *)raw;
  
	// first byte is always a literal
	rp++;
	
	for(;;)
	{	
		S32 matchOff;
    
		UINTr failedMatches = (1<<FAST_MULTISTEP_LITERALS_SHIFT) + 3;
		
		U32 rp32 = read32(rp);
		U32 hash = FAST_HASH_FUNC(rp, rp32 );
		SINTa curpos;
		const U8 * hashrp;
    
#ifdef DO_FAST_2ND_HASH
		U32 hash2;
#endif
    
		// literals :
		for(;;)		
		{    				
			curpos = rrPtrDiff(rp - zeroPosPtr);	
			RR_ASSERT( curpos >= 0 );
			
#ifdef DO_FAST_2ND_HASH
			hash2 = ( LZB_SecondHash4(rp32) ) & hash_table_mask;
#endif
      
#if FAST_HASH_DEPTH > 1
			for(int d=0;d<FAST_HASH_DEPTH;d++)
#endif
			{
				U16 hashpos16 = hashTable16[ FAST_HASH_INDEX(hash,d) ];
				
				matchOff = (U16)(curpos - hashpos16);
				RR_ASSERT( matchOff >= 0 );
				
				hashrp = rp - matchOff;
        
				//if ( matchOff <= LZB_MAX_OFFSET )
				RR_ASSERT( matchOff <= LZB_MAX_OFFSET );
				{							
					const U32 hashrp32 = read32(hashrp);
          
					if ( rp32 == hashrp32 && matchOff != 0 )
					{
						goto found_match;
					}
				}
			}
      
#ifdef DO_FAST_2ND_HASH
      
#if FAST_HASH_DEPTH > 1
			for(int d=0;d<FAST_HASH_DEPTH;d++)
#endif
			{
				U16 hashpos16 = hashTable16[ FAST_HASH_INDEX(hash2,d) ];
				
				matchOff = (U16)(curpos - hashpos16);
				RR_ASSERT( matchOff >= 0 );
				
				hashrp = rp - matchOff;
        
				RR_ASSERT( matchOff <= LZB_MAX_OFFSET );
				{							
					const U32 hashrp32 = read32(hashrp);
          
					if ( rp32 == hashrp32 && matchOff != 0 )
					{
						goto found_match;
					}
				}
			} 
			
#endif
      
			//---------------------------
			// update hash :
      
			hashTable16[ FAST_HASH_INDEX(hash,hashCycle) ] = (U16) curpos;
      
#ifdef DO_FAST_2ND_HASH
			// do NOT step hashCycle !
			//hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
			hashTable16[ FAST_HASH_INDEX(hash2,hashCycle) ] = (U16) curpos;
#endif
			
#if FAST_HASH_DEPTH > 1
			hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
#endif
      
			UINTr stepLiterals = (failedMatches>>FAST_MULTISTEP_LITERALS_SHIFT);
			RR_ASSERT( stepLiterals >= 1 );
      
			++failedMatches;
      
			rp += stepLiterals;
      
			if ( rp >= rpEndSafe )
				goto done;
      
			rp32 = read32(rp);
			hash = FAST_HASH_FUNC(rp, rp32 );
      
		}
		
		//-------------------------------
		found_match:
    
		// found something
    
    //-------------------------
    // update hash now so lazy can see it :
    
#if 1 // pretty important to compression
		hashTable16[ FAST_HASH_INDEX(hash,hashCycle) ] = (U16) curpos;
    
#ifdef DO_FAST_2ND_HASH
		// do NOT step hashCycle !
		//hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
		hashTable16[ FAST_HASH_INDEX(hash2,hashCycle) ] = (U16) curpos;
#endif
		
#if FAST_HASH_DEPTH > 1
		hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
#endif
#endif
		
		//-----------------------------------
		
		const U8 * match_start = rp;
		rp += 4;
    
		while( rp < rpEndSafe )
		{
			UINTr big1 = readR(rp);
			UINTr big2 = readR(rp-matchOff);
	    
			if ( big1 == big2 )
			{
				rp += RAD_PTRBYTES;
				continue;
			}
			else
			{
				rp += GetNumBytesZeroNeverAllR(big1^big2);  
				break;
			}
		}
		rp = RR_MIN(rp,rpMatchEnd);
    
		//-------------------------------
    // rp is now at the *end* of the match
    
		//-------------------------------
		
		// check lazy match too
#ifdef DO_FAST_LAZY_MATCH
		if (rp< rpEndSafe)
		{
			const U8 * lazyrp = match_start + 1;
			//SINTa lazypos = rrPtrDiff(lazyrp - zeroPosPtr);
			SINTa lazypos = curpos + 1;
			RR_ASSERT( lazypos == rrPtrDiff(lazyrp - zeroPosPtr) );
      
			U32 lazyrp32 = read32(lazyrp);
      
			const U8 * lazyhashrp;	
			SINTa lazymatchOff;					
			
			U32 lazyHash = FAST_HASH_FUNC(lazyrp, lazyrp32 );
			
#ifdef DO_FAST_2ND_HASH
			U32 lazyhash2 = LZB_SecondHash4(lazyrp32) & hash_table_mask;
#endif
			
#if FAST_HASH_DEPTH > 1
			for(int d=0;d<FAST_HASH_DEPTH;d++)
#endif
			{			
				U16 hashpos16 = hashTable16[ FAST_HASH_INDEX(lazyHash,d) ];
				
				lazymatchOff = (U16)(lazypos - hashpos16);
				RR_ASSERT( lazymatchOff >= 0 );
				
				RR_ASSERT( lazymatchOff <= LZB_MAX_OFFSET );
				{
					lazyhashrp = lazyrp - lazymatchOff;
          
					const U32 hashrp32 = read32(lazyhashrp);
          
					if ( lazyrp32 == hashrp32 && lazymatchOff != 0 )
					{
						goto lazy_found_match;
					}
				}
			}
      
#ifdef DO_FAST_2ND_HASH
#if FAST_HASH_DEPTH > 1
			for(int d=0;d<FAST_HASH_DEPTH;d++)
#endif
			{
				U16 hashpos16 = hashTable16[ FAST_HASH_INDEX(lazyhash2,d) ];
				
				lazymatchOff = (U16)(lazypos - hashpos16);
				RR_ASSERT( lazymatchOff >= 0 );
				
				RR_ASSERT( lazymatchOff <= LZB_MAX_OFFSET );
				{
					lazyhashrp = lazyrp - lazymatchOff;
          
					const U32 hashrp32 = read32(lazyhashrp);
          
					if ( lazyrp32 == hashrp32 && lazymatchOff != 0 )
					{
						goto lazy_found_match;
					}
				}
			}  
#endif
			
			if ( 0 )
			{
				lazy_found_match:
        
				lazyrp += 4;
        
				while( lazyrp < rpEndSafe )
				{
					UINTr big1 = readR(lazyrp);
					UINTr big2 = readR(lazyrp-lazymatchOff);
			    
					if ( big1 == big2 )
					{
						lazyrp += RAD_PTRBYTES;
						continue;
					}
					else
					{
						lazyrp += GetNumBytesZeroNeverAllR(big1^big2);  
						break;
					}
				}
				lazyrp = RR_MIN(lazyrp,rpMatchEnd);
				
				//S32 lazymatchLen = rrPtrDiff32( lazyrp - (match_start+1) );
				//RR_ASSERT( lazymatchLen >= 4 );
        
				if ( lazyrp >= rp+3 )
				{
					// yes take the lazy match
					
					// put a literal :
					match_start++;
          
					// I had a bug where lazypos was set wrong for the hash fill
					// it set it to the *end* of the normal match
					// and for some reason that helped compression WTF WTF						              
					//SINTa lazypos = rrPtrDiff(rp - zeroPosPtr); // 233647528
					// with correct lazypos : 233651228	
					
					// really this shouldn't be necessary at all
					// because I do an update of hash at all positions in the match including first!
#if 1	 // with update disabled - 233690274			    
          
					hashTable16[ FAST_HASH_INDEX(lazyHash,hashCycle) ] = (U16) lazypos;
          
#ifdef DO_FAST_2ND_HASH
					// do NOT step hashCycle !
					hashTable16[ FAST_HASH_INDEX(lazyhash2,hashCycle) ] = (U16) lazypos;
#endif
					
#if FAST_HASH_DEPTH > 1
					hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
#endif
					
#endif
					
					// and then drop out and do the lazy match :
					//matchLen = lazymatchLen;
					matchOff = (S32)lazymatchOff;
					rp = lazyrp;
					hashrp = lazyhashrp;
				}	
			}
		}
#endif			  
		
		//---------------------------------------------------
    
		// back up start of match that we missed due to stepLiterals !
		// make sure we don't read off the start of the array
		
		// this costs a little speed and gains a little compression
		// 15662162 at 121.58 mb/s
		// 15776473 at 127.92 mb/s
#if 1
		/*
		lzbf : 24,700,820 ->15,963,503 =  5.170 bpb =  1.547 to 1
		encode           : 0.171 seconds, 83.60 b/kc, rate= 144.54 M/s
		decode           : 0.014 seconds, 1002.64 b/kc, rate= 1733.57 M/s
		*/
		{
			// 144 M/s
			// back up start of match that we missed
			// make sure we don't read off the start of the array
			
			const U8 * rpm1 = match_start-1;
			if ( rpm1 >= literals_start && hashrp > zeroPosPtr && rpm1[0] == hashrp[-1] )
			{
				rpm1--; hashrp-= 2;
				
				while ( rpm1 >= literals_start && hashrp >= zeroPosPtr && rpm1[0] == *hashrp )
				{
					rpm1--;
					hashrp--;
				}
				
				match_start = rpm1+1;
				//rp = RR_MAX(rp,literals_start);
				RR_ASSERT( match_start >= literals_start );
			}
		}
#endif
		
		S32 matchLen = rrPtrDiff32( rp - match_start );
		RR_ASSERT( matchLen >= 4 );
    
		//===============================================
		// chose a match
		//	output LRL (if any) and match
		
		S32 cur_lrl = rrPtrDiff32(match_start - literals_start);
    
		// catch expansion while writing :
		if_unlikely ( cp+cur_lrl >= compExpandedPtr )
		{
			return rawLen+1;
		}
    
		cp = LZB_Output(cp,cur_lrl,literals_start,matchLen,matchOff);
    
		// skip the match :
		literals_start = rp;		
		
		if ( rp >= rpEndSafe )
			break;
		
		// step & update hashes :
		//  (I already did cur pos)
#ifdef DO_FAST_UPDATE_MATCH_HASHES
		// don't bother if it takes us to the end :      
		//	(this check is not for speed it's to avoid the access violation)          
		const U8 * ptr = match_start+1;
		U16 pos16 = (U16) rrPtrDiff( ptr - zeroPosPtr );
		for(;ptr<rp;ptr++)
		{
			U32 hash_result = FAST_HASH_FUNC( ptr, read32(ptr) );
			hashTable16[ FAST_HASH_INDEX(hash_result,hashCycle) ] = pos16; pos16++;
			//hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
			// helps a bit to NOT step cycle here
			//  the hash entries that come inside a match are of much lower quality
		}
#endif
	}
  
	done:;
	
	int cur_lrl = rrPtrDiff32(rpEnd - literals_start);
#if LZB_END_WITH_LITERALS
	RR_ASSERT_ALWAYS(cur_lrl > 0 );
#endif
  
	if ( cur_lrl > 0 )
	{
		// catch expansion while writing :
		if ( cp+cur_lrl >= compExpandedPtr )
		{
			return rawLen+1;
		}
		
		cp = LZB_OutputLast(cp,cur_lrl,literals_start);
	}
  
	SINTa compLen = rrPtrDiff( cp - (U8 *)comp );
  
	return compLen;
}

SINTa rr_lzb_simple_encode_fast(rr_lzb_simple_context * fh,
                                const void * raw, SINTa rawLen, void * comp)
{
	rr_lzb_simple_context_init(fh); //,raw);
  
	SINTa comp_len = rr_lzb_simple_encode_fast_sub(fh,raw,rawLen,comp);
	if ( comp_len >= rawLen )
	{
		memcpy(comp,raw,rawLen);
		return rawLen;
	}
	return comp_len;
}

#undef FAST_HASH_DEPTH_SHIFT

#undef DO_FAST_UPDATE_MATCH_HASHES
#undef DO_FAST_LAZY_MATCH
#undef DO_FAST_2ND_HASH  

//=====================================================

#define FAST_HASH_DEPTH_SHIFT	(0)

#undef FAST_MULTISTEP_LITERALS_SHIFT
#define FAST_MULTISTEP_LITERALS_SHIFT	(4)



//-----------------------
// derived :

RR_COMPILER_ASSERT( FAST_HASH_DEPTH_SHIFT == 0 );

#undef FAST_HASH_FUNC
//#define FAST_HASH_FUNC(ptr,dword)	( LZB_Hash4(dword) & hash_table_mask )
#define FAST_HASH_FUNC(ptr,dword)	( (((dword)*2654435761U)>>16) & hash_table_mask )


// @@@@ ????
#define LZBVF_DO_BACKUP	0
//#define LZBVF_DO_BACKUP	1


static SINTa rr_lzb_simple_encode_veryfast_sub(rr_lzb_simple_context * fh,
                                               const void * raw, SINTa rawLen, void * comp)
{
	//SIMPLEPROFILE_SCOPE_N(lzbfast_sub,rawLen);
	//THREADPROFILEFUNC();
	
	U8 * cp = (U8 *)comp;
	U8 * compExpandedPtr = cp + rawLen - 8;
  
	const U8 * rp = (const U8 *)raw;
	const U8 * rpEnd = rp+rawLen;
  
	// we can match up to rpEnd
	//	but matches can't start past rpEndSafe
	const U8 * rpMatchEnd = rpEnd - LZB_END_OF_BLOCK_NO_MATCH_ZONE;
	
	const U8 * rpEndSafe = rpMatchEnd - LZB_MML;
	
	if ( rpEndSafe <= (U8 *)raw )
	{
		// can't compress
		return rawLen+1;
	}
	
	const U8 * literals_start = rp;
  
	U16 * hashTable16 = fh->m_hashTable;
	int hashTableSizeBits = fh->m_tableSizeBits;
	U32 hash_table_mask = (U32)((1UL<<(hashTableSizeBits)) - 1);
  
	const U8 * zeroPosPtr = (const U8 *)raw;
  
	// first byte is always a literal
	rp++;
	
	for(;;)
	{   		
		U32 rp32 = read32(rp);
		U32 hash = FAST_HASH_FUNC(rp, rp32 );
		const U8 * hashrp;
		S32 matchOff;
		UINTr failedMatches;
    
		// loop while no match found :
		
		// first loop with step = 1
		// @@
		//int step1count = (1<<FAST_MULTISTEP_LITERALS_SHIFT); // full count
		int step1count = (1<<(FAST_MULTISTEP_LITERALS_SHIFT-1)); // half count
		while(step1count--)
		{			    					
			SINTa curpos = rrPtrDiff(rp - zeroPosPtr);	
			RR_ASSERT( curpos >= 0 );
			
			U16 hashpos16 = hashTable16[hash];
			hashTable16[ hash ] = (U16) curpos;
			
			matchOff = (U16)(curpos - hashpos16);
			RR_ASSERT( matchOff >= 0 && matchOff <= LZB_MAX_OFFSET );
			hashrp = rp - matchOff;
      
			const U32 hashrp32 = read32(hashrp);
			if ( rp32 == hashrp32 && matchOff != 0 )
			{
				goto found_match;
			}
      
			if ( ++rp >= rpEndSafe )
				goto done;
      
			rp32 = read32(rp);
			hash = FAST_HASH_FUNC(rp, rp32 );
		}
		
		// step starts at 2 :
		failedMatches = (2<<FAST_MULTISTEP_LITERALS_SHIFT);
    
		for(;;)		
		{			    		
			SINTa curpos = rrPtrDiff(rp - zeroPosPtr);	
			RR_ASSERT( curpos >= 0 );
			
			U16 hashpos16 = hashTable16[hash];
			hashTable16[ hash ] = (U16) curpos;
      
			matchOff = (U16)(curpos - hashpos16);
			RR_ASSERT( matchOff >= 0 && matchOff <= LZB_MAX_OFFSET );
			hashrp = rp - matchOff;
      
			const U32 hashrp32 = read32(hashrp);
      
			if ( rp32 == hashrp32 && matchOff != 0 )
			{
				goto found_match;
			}
      
			UINTr stepLiterals = (failedMatches>>FAST_MULTISTEP_LITERALS_SHIFT);
			RR_ASSERT( stepLiterals >= 1 );
      
			++failedMatches;
      
			rp += stepLiterals;
      
			if ( rp >= rpEndSafe )
				goto done;
      
			rp32 = read32(rp);
			hash = FAST_HASH_FUNC(rp, rp32 );
		}
		
		//-------------------------------
		found_match:;
    
		// found something
    
#if LZBVF_DO_BACKUP
		
		// alternative backup using counter :
		S32 cur_lrl = rrPtrDiff32(rp - literals_start);
		int neg_max_backup = - RR_MIN(cur_lrl , rrPtrDiff32(hashrp - zeroPosPtr) );
		int neg_backup = -1;
		if( neg_backup >= neg_max_backup && rp[neg_backup] == hashrp[neg_backup] )
		{
			neg_backup--;
			while( neg_backup >= neg_max_backup && rp[neg_backup] == hashrp[neg_backup] )
			{
				neg_backup--;
			}
			neg_backup++;
			rp += neg_backup;
			cur_lrl += neg_backup;
			RR_ASSERT( cur_lrl >= 0 );
			RR_ASSERT( cur_lrl == rrPtrDiff32(rp - literals_start) );
		}
		
#else
		
		S32 cur_lrl = rrPtrDiff32(rp - literals_start);
		
#endif
    
		// catch expansion while writing :
		if_unlikely ( cp+cur_lrl >= compExpandedPtr )
		{
			return rawLen+1;
		}
		
		RR_ASSERT( matchOff >= 1 );
    
		//---------------------------------------
		// find rest of match len
		// save pointer to start of match
		// walk rp ahead to end of match
		const U8 * match_start = rp;
		rp += 4;
    
		while( rp < rpEndSafe )
		{
			UINTr big1 = readR(rp);
			UINTr big2 = readR(rp-matchOff);
	    
			if ( big1 == big2 )
			{
				rp += RAD_PTRBYTES;
				continue;
			}
			else
			{
				rp += GetNumBytesZeroNeverAllR(big1^big2);  
				break;
			}
		}
		rp = RR_MIN(rp,rpMatchEnd);
		S32 matchLen = rrPtrDiff32( rp - match_start );
		
		//===============================================
		// chose a match
		//	output LRL (if any) and match
		
		cp = LZB_Output(cp,cur_lrl,literals_start,matchLen,matchOff);
    
		// skip the match :
		literals_start = rp;
		
		if ( rp >= rpEndSafe )
			goto done;	
	}
	
	done:;
	
	int cur_lrl = rrPtrDiff32(rpEnd - literals_start);
#if LZB_END_WITH_LITERALS
	RR_ASSERT_ALWAYS(cur_lrl > 0 );
#endif
  
	if ( cur_lrl > 0 )
	{
		// catch expansion while writing :
		if ( cp+cur_lrl >= compExpandedPtr )
		{
			return rawLen+1;
		}
		
		cp = LZB_OutputLast(cp,cur_lrl,literals_start);
	}
  
	SINTa compLen = rrPtrDiff( cp - (U8 *)comp );
  
	return compLen;
}

SINTa rr_lzb_simple_encode_veryfast(rr_lzb_simple_context * fh,
                                    const void * raw, SINTa rawLen, void * comp)
{
	rr_lzb_simple_context_init(fh); //,raw);
	
	SINTa comp_len = rr_lzb_simple_encode_veryfast_sub(fh,raw,rawLen,comp);
	if ( comp_len >= rawLen )
	{
		memcpy(comp,raw,rawLen);
		return rawLen;
	}
	return comp_len;
}

#undef FAST_HASH_DEPTH_SHIFT

#undef DO_FAST_UPDATE_MATCH_HASHES
#undef DO_FAST_LAZY_MATCH
#undef DO_FAST_2ND_HASH  

//=====================================================
// vim:noet:sw=4:ts=4
