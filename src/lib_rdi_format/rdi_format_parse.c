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
  RDI_U64 *voffs = rdi_element_from_name_idx(rdi, ScopeVOffData, scope->voff_range_opl);
  RDI_U64 result = *voffs;
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
