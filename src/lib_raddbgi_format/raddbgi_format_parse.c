// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

RDI_PROC void *
rdi_extract_section_data(RDI_U8 *raw_data, RDI_U64 raw_data_size, RDI_DataSection *sections, RDI_DataSectionTag tag, RDI_U32 item_size, RDI_U64 *out_count)
{
  RDI_DataSection *sect = &sections[tag];
  void *data = raw_data + sect->off;
  *out_count = sect->unpacked_size / item_size;
  return data;
}

RDI_PROC RDI_ParseStatus
rdi_parse(RDI_U8 *data, RDI_U64 size, RDI_Parsed *out_rdi)
{
  RDI_ParseStatus parse_status = RDI_ParseStatus_Good;
  
  // parse data sections
  RDI_DataSection *sections      = 0;
  RDI_U64          section_count = 0;
  do
  {
    RDI_Header *header = (RDI_Header *)data;

    // error check header
    parse_status = RDI_ParseStatus_Unknown;
    if(sizeof(*header) > size)
    {
      parse_status = RDI_ParseStatus_OutOfData;
      break;
    }
    if(header->magic != RDI_MAGIC_CONSTANT)
    {
      parse_status = RDI_ParseStatus_HeaderDoesNotMatch;
      break;
    }
    if(header->encoding_version != RDI_ENCODING_VERSION)
    {
      parse_status = RDI_ParseStatus_UnsupportedVersionNumber;
      break;
    }

    // error check data section array
    RDI_U64 opl = (RDI_U64)header->data_section_off + (RDI_U64)header->data_section_count*sizeof(RDI_DataSection);
    if(opl > size)
    {
      parse_status = RDI_ParseStatus_InvalidDataSecionLayout;
      break;
    }

    // sanity check sections headers & distribute by tag
    parse_status  = RDI_ParseStatus_Good;
    sections      = (RDI_DataSection *)(data + header->data_section_off);
    section_count = header->data_section_count;
    for(RDI_U32 i = 0; i < section_count; i += 1)
    {
      RDI_DataSection *sect = sections + i;

      RDI_U64 encoded_opl = sect->off + sect->encoded_size;
      if(encoded_opl > size)
      {
        parse_status = RDI_ParseStatus_InvalidSectionEncoding;
        break;
      }
      if(sect->tag >= RDI_DataSectionTag_PRIMARY_COUNT)
      {
        parse_status = RDI_ParseStatus_InvalidDataSectionTag;
        break;
      }
    }
  } while(0);
  
  if(parse_status == RDI_ParseStatus_Good)
  {
    RDI_U64 top_level_count = 0;

    out_rdi->raw_data          = data;
    out_rdi->raw_data_size     = size;
    out_rdi->sections          = sections;
    out_rdi->section_count     = section_count;
    out_rdi->top_level_info    = (RDI_TopLevelInfo *)  rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_TopLevelInfo   , sizeof(out_rdi->top_level_info[0])   , &top_level_count                  );
    out_rdi->string_data       = (RDI_U8 *)            rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_StringData     , sizeof(out_rdi->string_data[0])      , &out_rdi->string_data_size        );
    out_rdi->string_offs       = (RDI_U32 *)           rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_StringTable    , sizeof(out_rdi->string_offs[0])      , &out_rdi->string_count            );
    out_rdi->idx_run_data      = (RDI_U32 *)           rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_IndexRuns      , sizeof(out_rdi->idx_run_data[0])     , &out_rdi->idx_run_count           );
    out_rdi->binary_sections   = (RDI_BinarySection *) rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_BinarySections , sizeof(out_rdi->binary_sections[0])  , &out_rdi->binary_sections_count   );
    out_rdi->file_paths        = (RDI_FilePathNode *)  rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_FilePathNodes  , sizeof(out_rdi->file_paths[0])       , &out_rdi->file_paths_count        );
    out_rdi->source_files      = (RDI_SourceFile *)    rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_SourceFiles    , sizeof(out_rdi->source_files[0])     , &out_rdi->source_files_count      );
    out_rdi->units             = (RDI_Unit *)          rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_Units          , sizeof(out_rdi->units[0])            , &out_rdi->units_count             );
    out_rdi->unit_vmap         = (RDI_VMapEntry *)     rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_UnitVmap       , sizeof(out_rdi->unit_vmap[0])        , &out_rdi->unit_vmap_count         );
    out_rdi->type_nodes        = (RDI_TypeNode *)      rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_TypeNodes      , sizeof(out_rdi->type_nodes[0])       , &out_rdi->type_nodes_count        );
    out_rdi->udts              = (RDI_UDT *)           rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_UDTs           , sizeof(out_rdi->udts[0])             , &out_rdi->udts_count              );
    out_rdi->members           = (RDI_Member *)        rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_Members        , sizeof(out_rdi->members[0])          , &out_rdi->members_count           );
    out_rdi->enum_members      = (RDI_EnumMember *)    rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_EnumMembers    , sizeof(out_rdi->enum_members[0])     , &out_rdi->enum_members_count      );
    out_rdi->global_variables  = (RDI_GlobalVariable *)rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_GlobalVariables, sizeof(out_rdi->global_variables[0]) , &out_rdi->global_variables_count  );
    out_rdi->global_vmap       = (RDI_VMapEntry *)     rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_GlobalVmap     , sizeof(out_rdi->global_vmap[0])      , &out_rdi->global_vmap_count       );
    out_rdi->thread_variables  = (RDI_ThreadVariable *)rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_ThreadVariables, sizeof(out_rdi->thread_variables[0]) , &out_rdi->thread_variables_count  );
    out_rdi->procedures        = (RDI_Procedure *)     rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_Procedures     , sizeof(out_rdi->procedures[0])       , &out_rdi->procedures_count        );
    out_rdi->scopes            = (RDI_Scope *)         rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_Scopes         , sizeof(out_rdi->scopes[0])           , &out_rdi->scopes_count            );
    out_rdi->scope_voffs       = (RDI_U64 *)           rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_ScopeVoffData  , sizeof(out_rdi->scope_voffs[0])      , &out_rdi->scope_voffs_count       );
    out_rdi->scope_vmap        = (RDI_VMapEntry *)     rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_ScopeVmap      , sizeof(out_rdi->scope_vmap[0])       , &out_rdi->scope_vmap_count        );
    out_rdi->inline_sites      = (RDI_InlineSite *)    rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_InlineSites    , sizeof(out_rdi->inline_sites[0])     , &out_rdi->inline_site_count       );
    out_rdi->locals            = (RDI_Local *)         rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_Locals         , sizeof(out_rdi->locals[0])           , &out_rdi->locals_count            );
    out_rdi->location_blocks   = (RDI_LocationBlock *) rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_LocationBlocks , sizeof(out_rdi->location_blocks[0])  , &out_rdi->location_blocks_count   );
    out_rdi->location_data     = (RDI_U8 *)            rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_LocationData   , sizeof(out_rdi->location_data[0])    , &out_rdi->location_data_size      );
    out_rdi->name_maps         = (RDI_NameMap *)       rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_NameMaps       , sizeof(out_rdi->name_maps[0])        , &out_rdi->name_maps_count         );
    out_rdi->name_maps_buckets = (RDI_NameMapBucket *) rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_NameMapsBuckets, sizeof(out_rdi->name_maps_buckets[0]), &out_rdi->name_maps_bucket_count  );
    out_rdi->name_maps_nodes   = (RDI_NameMapNode *)   rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_NameMapsNodes  , sizeof(out_rdi->name_maps_nodes[0])  , &out_rdi->name_maps_node_count    );
    out_rdi->line_info         = (RDI_LineInfo *)      rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_LineInfo       , sizeof(out_rdi->line_info[0])        , &out_rdi->line_info_count         );
    out_rdi->line_info_voffs   = (RDI_U64 *)           rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_LineInfoVoffs  , sizeof(out_rdi->line_info_voffs[0])  , &out_rdi->line_info_voff_count    );
    out_rdi->line_info_data    = (RDI_Line *)          rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_LineInfoData   , sizeof(out_rdi->line_info_data[0])   , &out_rdi->line_info_data_count    );
    out_rdi->line_info_cols    = (RDI_Column *)        rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_LineInfoColumns, sizeof(out_rdi->line_info_cols[0])   , &out_rdi->line_info_col_count     );
    out_rdi->line_number_maps  = (RDI_LineNumberMap *) rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_LineNumberMaps , sizeof(out_rdi->line_number_maps[0]) , &out_rdi->line_number_map_count   );
    out_rdi->line_map_numbers  = (RDI_U32 *)           rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_LineMapNumbers , sizeof(out_rdi->line_map_numbers[0]) , &out_rdi->line_map_number_count   );
    out_rdi->line_map_ranges   = (RDI_U32 *)           rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_LineMapRanges  , sizeof(out_rdi->line_map_ranges[0])  , &out_rdi->line_map_range_count    );
    out_rdi->line_map_voffs    = (RDI_U64 *)           rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_LineMapVoffs   , sizeof(out_rdi->line_map_voffs[0])   , &out_rdi->line_map_voff_count     );
    out_rdi->checksums         = (RDI_U8 *)            rdi_extract_section_data(data, size, out_rdi->sections_by_tag, RDI_DataSectionTag_Checksums      , sizeof(out_rdi->checksums[0])        , &out_rdi->checksums_size          );

    // TODO: why is string count off by one?
    if(out_rdi->string_count > 0)
    {
      out_rdi->string_count -= 1;
    }

    // fill out sections by tag
    for(RDI_U64 i = 0; i < section_count; i += 1)
    {
      RDI_DataSection *sect = sections + i;
      out_rdi->sections_by_tag[sect->tag] = *sect;
    }

    // fill out name maps by kind
    for(RDI_U64 nm_idx = 0; nm_idx < out_rdi->name_maps_count; nm_idx += 1)
    {
      RDI_NameMap *nm = &out_rdi->name_maps[nm_idx];
      if(out_rdi->name_maps_by_kind[nm->kind] == 0)
      {
        out_rdi->name_maps_by_kind[nm->kind] = nm;
      }
      else
      {
        out_rdi->name_maps_by_kind[nm->kind] = 0;
      }
    }
  }
  
#if !defined(RDI_DISABLE_NILS)
  if(out_rdi->top_level_info    == 0) { out_rdi->top_level_info    = &rdi_top_level_info_nil;                                           }
  if(out_rdi->binary_sections   == 0) { out_rdi->binary_sections   = &rdi_binary_section_nil;      out_rdi->binary_sections_count  = 1; }
  if(out_rdi->file_paths        == 0) { out_rdi->file_paths        = &rdi_file_path_node_nil;      out_rdi->file_paths_count       = 1; }
  if(out_rdi->source_files      == 0) { out_rdi->source_files      = &rdi_source_file_nil;         out_rdi->source_files_count     = 1; }
  if(out_rdi->units             == 0) { out_rdi->units             = &rdi_unit_nil;                out_rdi->units_count            = 1; }
  if(out_rdi->unit_vmap         == 0) { out_rdi->unit_vmap         = &rdi_vmap_entry_nil;          out_rdi->unit_vmap_count        = 1; }
  if(out_rdi->type_nodes        == 0) { out_rdi->type_nodes        = &rdi_type_node_nil;           out_rdi->type_nodes_count       = 1; }
  if(out_rdi->udts              == 0) { out_rdi->udts              = &rdi_udt_nil;                 out_rdi->udts_count             = 1; }
  if(out_rdi->members           == 0) { out_rdi->members           = &rdi_member_nil;              out_rdi->members_count          = 1; }
  if(out_rdi->enum_members      == 0) { out_rdi->enum_members      = &rdi_enum_member_nil;         out_rdi->enum_members_count     = 1; }
  if(out_rdi->global_variables  == 0) { out_rdi->global_variables  = &rdi_global_variable_nil;     out_rdi->global_variables_count = 1; }
  if(out_rdi->global_vmap       == 0) { out_rdi->global_vmap       = &rdi_vmap_entry_nil;          out_rdi->global_vmap_count      = 1; }
  if(out_rdi->thread_variables  == 0) { out_rdi->thread_variables  = &rdi_thread_variable_nil;     out_rdi->thread_variables_count = 1; }
  if(out_rdi->procedures        == 0) { out_rdi->procedures        = &rdi_procedure_nil;           out_rdi->procedures_count       = 1; }
  if(out_rdi->scopes            == 0) { out_rdi->scopes            = &rdi_scope_nil;               out_rdi->scopes_count           = 1; }
  if(out_rdi->scope_voffs       == 0) { out_rdi->scope_voffs       = &rdi_voff_nil;                out_rdi->scope_voffs_count      = 1; }
  if(out_rdi->scope_vmap        == 0) { out_rdi->scope_vmap        = &rdi_vmap_entry_nil;          out_rdi->scope_vmap_count       = 1; }
  if(out_rdi->locals            == 0) { out_rdi->locals            = &rdi_local_nil;               out_rdi->locals_count           = 1; }
  if(out_rdi->location_blocks   == 0) { out_rdi->location_blocks   = &rdi_location_block_nil;      out_rdi->location_blocks_count  = 1; }
  if(out_rdi->name_maps         == 0) { out_rdi->name_maps         = &rdi_name_map_nil;            out_rdi->name_maps_count        = 1; }
  if(out_rdi->name_maps_buckets == 0) { out_rdi->name_maps_buckets = &rdi_name_map_bucket_nil;     out_rdi->name_maps_bucket_count = 1; }
  if(out_rdi->name_maps_nodes   == 0) { out_rdi->name_maps_nodes   = &rdi_name_map_node_nil;       out_rdi->name_maps_node_count   = 1; }
  if(out_rdi->inline_sites      == 0) { out_rdi->inline_sites      = &rdi_inline_site_nil;         out_rdi->inline_site_count      = 1; }
  if(out_rdi->line_info         == 0) { out_rdi->line_info         = &rdi_line_info_nil;           out_rdi->line_info_count        = 1; }
  if(out_rdi->line_info_voffs   == 0) { out_rdi->line_info_voffs   = &rdi_voff_nil;                out_rdi->line_info_voff_count   = 1; }
  if(out_rdi->line_info_data    == 0) { out_rdi->line_info_data    = &rdi_line_nil;                out_rdi->line_info_data_count   = 1; }
  if(out_rdi->line_info_cols    == 0) { out_rdi->line_info_cols    = &rdi_column_nil;              out_rdi->line_info_col_count    = 1; }
  if(out_rdi->checksums         == 0) { out_rdi->checksums         = (RDI_U8 *)&rdi_checksum_nil;  out_rdi->checksums_size         = sizeof(rdi_checksum_nil); }
#endif
  
  return parse_status;
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
  if(line_info_idx < rdi->line_info_count)
  {
    RDI_LineInfo line_info = rdi->line_info[line_info_idx];
    if(line_info.voff_data_idx + line_info.line_count <= rdi->line_info_voff_count &&
       line_info.line_data_idx + line_info.line_count <= rdi->line_info_data_count &&
       line_info.col_data_idx  + line_info.col_count  <= rdi->line_info_col_count)
    {
      out->count     = line_info.line_count;
      out->col_count = line_info.col_count;
      out->voffs     = rdi->line_info_voffs + line_info.voff_data_idx;
      out->lines     = rdi->line_info_data  + line_info.line_data_idx;
      out->cols      = rdi->line_info_cols  + line_info.col_data_idx;
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
rdi_parse_line_number_map(RDI_Parsed *rdi, RDI_U32 line_number_map_idx, RDI_ParsedLineMap *out)
{
  if(line_number_map_idx < rdi->line_number_map_count)
  {
    RDI_LineNumberMap *map = rdi->line_number_maps + line_number_map_idx;
    if(map->line_data_idx  + map->line_count     <= rdi->line_map_number_count &&
       map->range_data_idx + map->line_count + 1 <= rdi->line_map_range_count)
    {
      out->nums       = rdi->line_map_numbers + map->line_data_idx;
      out->ranges     = rdi->line_map_ranges  + map->range_data_idx;
      out->voffs      = rdi->line_map_voffs   + map->voff_data_idx;
      out->count      = map->line_count;
      out->voff_count = map->voff_count;
    }
  }
}

RDI_PROC void
rdi_line_map_from_source_file(RDI_Parsed *rdi, RDI_SourceFile *src_file, RDI_ParsedLineMap *out)
{
  rdi_parse_line_number_map(rdi, src_file->line_number_map_idx, out);
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
rdi_name_map_from_kind(RDI_Parsed *rdi, RDI_NameMapKind kind)
{
  RDI_NameMap *nm = 0;
  if(0 < kind && kind < RDI_NameMapKind_COUNT)
  {
    nm = rdi->name_maps_by_kind[kind];
  }
  return nm;
}

RDI_PROC void
rdi_name_map_parse(RDI_Parsed *rdi, RDI_NameMap *nm, RDI_ParsedNameMap *out)
{
  out->buckets      = 0;
  out->nodes        = 0;
  out->bucket_count = 0;
  out->node_count   = 0;
  if(nm != 0)
  {
    out->buckets      = rdi->name_maps_buckets + nm->bucket_data_idx;
    out->nodes        = rdi->name_maps_nodes   + nm->node_data_idx;
    out->bucket_count = nm->bucket_count;
    out->node_count   = nm->node_count;
  }
}

RDI_PROC RDI_NameMapNode *
rdi_name_map_lookup(RDI_Parsed *rdi, RDI_ParsedNameMap *map, RDI_U8 *str, RDI_U64 len)
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
      RDI_U8 *nstr = rdi_string_from_idx(rdi, node->string_idx, &nlen);
      
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

RDI_PROC RDI_U32 *
rdi_matches_from_map_node(RDI_Parsed *rdi, RDI_NameMapNode *node, RDI_U32 *n_out)
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
      result = rdi_idx_run_from_first_count(rdi, node->match_idx_or_idx_run_first, node->match_count, n_out);
    }
  }
  return result;
}

//- checksums

RDI_PROC RDI_U64
rdi_checksum_from_offset(RDI_Parsed *rdi, RDI_U64 checksum_offset, RDI_ParsedChecksum *out)
{
  RDI_U64 size = 0;
  if(checksum_offset + sizeof(RDI_Checksum) <= rdi->checksums_size)
  {
    RDI_Checksum *header = (RDI_Checksum *)(rdi->checksums + checksum_offset);
    if(checksum_offset + sizeof(RDI_Checksum) + header->size <= rdi->checksums_size)
    {
      out->kind = header->kind;
      out->size = header->size;
      out->data = (RDI_U8 *)(header + 1);
      size = sizeof(*header) + header->size;
    }
  }
  return size;
}

RDI_PROC RDI_U64
rdi_time_stamp_from_parsed_checksum(RDI_ParsedChecksum parsed_checksum)
{
  RDI_U64 time_stamp = 0;
  if(parsed_checksum.kind == RDI_Checksum_TimeStamp)
  {
    switch(parsed_checksum.size)
    {
    case 1: time_stamp = *(RDI_U8  *)parsed_checksum.data; break;
    case 2: time_stamp = *(RDI_U16 *)parsed_checksum.data; break;
    case 4: time_stamp = *(RDI_U32 *)parsed_checksum.data; break;
    case 8: time_stamp = *(RDI_U64 *)parsed_checksum.data; break;
    }
  }
  return time_stamp;
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
