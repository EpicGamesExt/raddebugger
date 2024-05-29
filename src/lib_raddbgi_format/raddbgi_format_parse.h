// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_PARSE_H
#define RDI_PARSE_H

////////////////////////////////
//~ RADDBG Parsing Helpers

typedef struct RDI_Parsed{
  // raw data & data sections (part 1)
  RDI_U8 *raw_data;
  RDI_U64 raw_data_size;
  RDI_DataSection *dsecs;
  RDI_U64 dsec_count;
  RDI_U32 dsec_idx[RDI_DataSectionTag_PRIMARY_COUNT];
  
  // primary data structures (part 2)
  
  //  handled by helper APIs
  RDI_U8*  string_data;
  RDI_U64  string_data_size;
  RDI_U32* string_offs;
  RDI_U64  string_count;
  RDI_U32* idx_run_data;
  RDI_U32  idx_run_count;
  
  //  directly readable by users
  //  (any of these may be empty and null even in a successful parse)
  RDI_TopLevelInfo* top_level_info;
  
  RDI_BinarySection*  binary_sections;
  RDI_U64             binary_sections_count;
  RDI_FilePathNode*   file_paths;
  RDI_U64             file_paths_count;
  RDI_SourceFile*     source_files;
  RDI_U64             source_files_count;
  RDI_Unit*           units;
  RDI_U64             units_count;
  RDI_VMapEntry*      unit_vmap;
  RDI_U64             unit_vmap_count;
  RDI_TypeNode*       type_nodes;
  RDI_U64             type_nodes_count;
  RDI_UDT*            udts;
  RDI_U64             udts_count;
  RDI_Member*         members;
  RDI_U64             members_count;
  RDI_EnumMember*     enum_members;
  RDI_U64             enum_members_count;
  RDI_GlobalVariable* global_variables;
  RDI_U64             global_variables_count;
  RDI_VMapEntry*      global_vmap;
  RDI_U64             global_vmap_count;
  RDI_ThreadVariable* thread_variables;
  RDI_U64             thread_variables_count;
  RDI_Procedure*      procedures;
  RDI_U64             procedures_count;
  RDI_Scope*          scopes;
  RDI_U64             scopes_count;
  RDI_U64*            scope_voffs;
  RDI_U64             scope_voffs_count;
  RDI_VMapEntry*      scope_vmap;
  RDI_U64             scope_vmap_count;
  RDI_InlineSite*     inline_sites;
  RDI_U64             inline_site_count;
  RDI_Local*          locals;
  RDI_U64             locals_count;
  RDI_LocationBlock*  location_blocks;
  RDI_U64             location_blocks_count;
  RDI_U8*             location_data;
  RDI_U64             location_data_size;
  RDI_NameMap*        name_maps;
  RDI_U64             name_maps_count;
  RDI_LineInfo       *line_info;
  RDI_U64             line_info_count;
  RDI_U64            *line_info_voffs;
  RDI_U64             line_info_voff_count;
  RDI_Line           *line_info_data;
  RDI_U64             line_info_data_count;
  RDI_Column         *line_info_cols;
  RDI_U64             line_info_col_count;
  RDI_LineNumberMap  *line_number_maps;
  RDI_U64             line_number_map_count;
  RDI_U32            *line_map_numbers;
  RDI_U64             line_map_number_count;
  RDI_U32            *line_map_ranges;
  RDI_U64             line_map_range_count;
  RDI_U64            *line_map_voffs;
  RDI_U64             line_map_voff_count;
  RDI_U8             *checksums;
  RDI_U64             checksums_size;

  
  // other helpers
  
  RDI_NameMap* name_maps_by_kind[RDI_NameMapKind_COUNT];
  
} RDI_Parsed;

typedef enum{
  RDI_ParseStatus_Good = 0,
  RDI_ParseStatus_HeaderDoesNotMatch = 1,
  RDI_ParseStatus_UnsupportedVersionNumber = 2,
  RDI_ParseStatus_InvalidDataSecionLayout = 3,
  RDI_ParseStatus_MissingStringDataSection = 4,
  RDI_ParseStatus_MissingStringTableSection = 5,
  RDI_ParseStatus_MissingIndexRunSection = 6,
} RDI_ParseStatus;

typedef struct RDI_ParsedChecksum{
  RDI_ChecksumKind kind;
  RDI_U8 size;
  RDI_U8 *data;
} RDI_ParsedChecksum;

typedef struct RDI_ParsedLineInfo{
  // NOTE: Mapping VOFF -> LINE_INFO
  //
  // * [ voff[i], voff[i + 1] ) forms the voff range
  // * for the line info at lines[i] (and cols[i] if i < col_count)
  
  RDI_U64*    voffs; // [count + 1] sorted
  RDI_Line*   lines; // [count]
  RDI_Column* cols;  // [col_count]
  RDI_U64 count;
  RDI_U64 col_count;
} RDI_ParsedLineInfo;

typedef struct RDI_ParsedLineMap{
  // NOTE: Mapping LINE_NUMBER -> VOFFs
  //
  // * nums[i] gives a line number
  // * that line number has one or more associated voffs
  //
  // * to find all associated voffs for the line number nums[i] :
  // * let k span over the range [ ranges[i], ranges[i + 1] )
  // * voffs[k] gives the associated voffs
  
  RDI_U32* nums;   // [count] sorted
  RDI_U32* ranges; // [count + 1]
  RDI_U64* voffs;  // [voff_count]
  RDI_U64 count;
  RDI_U64 voff_count;
} RDI_ParsedLineMap;


typedef struct RDI_ParsedNameMap{
  RDI_NameMapBucket *buckets;
  RDI_NameMapNode *nodes;
  RDI_U64 bucket_count;
  RDI_U64 node_count;
} RDI_ParsedNameMap;

////////////////////////////////
//~ Global Nils

#if !defined(RDI_DISABLE_NILS)
static RDI_TopLevelInfo   rdi_top_level_info_nil  = {0};
static RDI_BinarySection  rdi_binary_section_nil  = {0};
static RDI_FilePathNode   rdi_file_path_node_nil  = {0};
static RDI_SourceFile     rdi_source_file_nil     = {0};
static RDI_Unit           rdi_unit_nil            = {0};
static RDI_VMapEntry      rdi_vmap_entry_nil      = {0};
static RDI_TypeNode       rdi_type_node_nil       = {0};
static RDI_UDT            rdi_udt_nil             = {0};
static RDI_Member         rdi_member_nil          = {0};
static RDI_EnumMember     rdi_enum_member_nil     = {0};
static RDI_GlobalVariable rdi_global_variable_nil = {0};
static RDI_ThreadVariable rdi_thread_variable_nil = {0};
static RDI_Procedure      rdi_procedure_nil       = {0};
static RDI_Scope          rdi_scope_nil           = {0};
static RDI_InlineSite     rdi_inline_site_nil     = {0};
static RDI_U64            rdi_voff_nil            = 0;
static RDI_LocationBlock  rdi_location_block_nil  = {0};
static RDI_Local          rdi_local_nil           = {0};
static RDI_LineInfo       rdi_line_info_nil       = {0};
static RDI_Line           rdi_line_nil            = {0};
static RDI_Column         rdi_column_nil          = {0};
static RDI_Checksum       rdi_checksum_nil        = {0};
static RDI_LineNumberMap  rdi_line_number_map_nil = {0};
#endif

////////////////////////////////
//~ RADDBG Parse API

RDI_PROC RDI_ParseStatus
rdi_parse(RDI_U8 *data, RDI_U64 size, RDI_Parsed *out);

RDI_PROC RDI_U8*
rdi_string_from_idx(RDI_Parsed *parsed, RDI_U32 idx, RDI_U64 *len_out);

RDI_PROC RDI_U32*
rdi_idx_run_from_first_count(RDI_Parsed *parsed, RDI_U32 first, RDI_U32 raw_count,
                             RDI_U32 *n_out);

//- table lookups
#define rdi_element_from_idx(parsed, name, idx) ((0 <= (idx) && (idx) < (parsed)->name##_count) ? &(parsed)->name[idx] : (parsed)->name ? &(parsed)->name[0] : 0)

//- line info
RDI_PROC void
rdi_parse_line_info(RDI_Parsed *rdi, RDI_U64 line_info_idx, RDI_ParsedLineInfo *out);

RDI_PROC void
rdi_line_info_from_unit(RDI_Parsed *p, RDI_Unit *unit, RDI_ParsedLineInfo *out);

RDI_PROC RDI_U64
rdi_line_info_idx_from_voff(RDI_ParsedLineInfo *line_info, RDI_U64 voff);

RDI_PROC void
rdi_parse_line_number_map(RDI_Parsed *rdi, RDI_U32 line_number_map_idx, RDI_ParsedLineMap *out);

RDI_PROC void
rdi_line_map_from_source_file(RDI_Parsed *rdi, RDI_SourceFile *src_file, RDI_ParsedLineMap *out);

RDI_PROC RDI_U64*
rdi_line_voffs_from_num(RDI_ParsedLineMap *map, RDI_U32 linenum, RDI_U32 *n_out);

//- vmaps
RDI_PROC RDI_U64
rdi_vmap_idx_from_voff(RDI_VMapEntry *vmap, RDI_U32 vmap_count, RDI_U64 voff);


//- name maps
RDI_PROC RDI_NameMap*
rdi_name_map_from_kind(RDI_Parsed *p, RDI_NameMapKind kind);

RDI_PROC void
rdi_name_map_parse(RDI_Parsed* p, RDI_NameMap *mapptr, RDI_ParsedNameMap *out);

RDI_PROC RDI_NameMapNode*
rdi_name_map_lookup(RDI_Parsed *p, RDI_ParsedNameMap *map,
                    RDI_U8 *str, RDI_U64 len);

RDI_PROC RDI_U32*
rdi_matches_from_map_node(RDI_Parsed *p, RDI_NameMapNode *node, RDI_U32 *n_out);

//- checksums

RDI_PROC RDI_U64
rdi_checksum_from_offset(RDI_Parsed *p, RDI_U64 checksum_offset, RDI_ParsedChecksum *out);

RDI_PROC RDI_U64
rdi_time_stamp_from_parsed_checksum(RDI_ParsedChecksum checksum);

//- common helpers
RDI_PROC RDI_U64
rdi_first_voff_from_proc(RDI_Parsed *p, RDI_U32 proc_id);


////////////////////////////////
//~ RADDBG Parsing Helpers

#define rdi_parse__extract_primary(p,outptr,outn,pritag) \
( (*(void**)&(outptr)) = \
rdi_data_from_dsec((p),(p)->dsec_idx[pritag],sizeof(*(outptr)),(pritag),(outn)) )

RDI_PROC void*
rdi_data_from_dsec(RDI_Parsed *p, RDI_U32 idx, RDI_U32 item_size,
                   RDI_DataSectionTag expected_tag, RDI_U64 *n_out);

#define rdi_parse__min(a,b) (((a)<(b))?(a):(b))

#endif // RDI_PARSE_H
