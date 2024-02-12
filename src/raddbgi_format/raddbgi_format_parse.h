// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_PARSE_H
#define RADDBGI_PARSE_H

////////////////////////////////
//~ RADDBG Parsing Helpers

typedef struct RADDBGI_Parsed{
  // raw data & data sections (part 1)
  RADDBGI_U8 *raw_data;
  RADDBGI_U64 raw_data_size;
  RADDBGI_DataSection *dsecs;
  RADDBGI_U64 dsec_count;
  RADDBGI_U32 dsec_idx[RADDBGI_DataSectionTag_PRIMARY_COUNT];
  
  // primary data structures (part 2)
  
  //  handled by helper APIs
  RADDBGI_U8*  string_data;
  RADDBGI_U64  string_data_size;
  RADDBGI_U32* string_offs;
  RADDBGI_U64  string_count;
  RADDBGI_U32* idx_run_data;
  RADDBGI_U32  idx_run_count;
  
  //  directly readable by users
  //  (any of these may be empty and null even in a successful parse)
  RADDBGI_TopLevelInfo* top_level_info;
  
  RADDBGI_BinarySection*  binary_sections;
  RADDBGI_U64             binary_sections_count;
  RADDBGI_FilePathNode*   file_paths;
  RADDBGI_U64             file_paths_count;
  RADDBGI_SourceFile*     source_files;
  RADDBGI_U64             source_files_count;
  RADDBGI_Unit*           units;
  RADDBGI_U64             units_count;
  RADDBGI_VMapEntry*      unit_vmap;
  RADDBGI_U64             unit_vmap_count;
  RADDBGI_TypeNode*       type_nodes;
  RADDBGI_U64             type_nodes_count;
  RADDBGI_UDT*            udts;
  RADDBGI_U64             udts_count;
  RADDBGI_Member*         members;
  RADDBGI_U64             members_count;
  RADDBGI_EnumMember*     enum_members;
  RADDBGI_U64             enum_members_count;
  RADDBGI_GlobalVariable* global_variables;
  RADDBGI_U64             global_variables_count;
  RADDBGI_VMapEntry*      global_vmap;
  RADDBGI_U64             global_vmap_count;
  RADDBGI_ThreadVariable* thread_variables;
  RADDBGI_U64             thread_variables_count;
  RADDBGI_Procedure*      procedures;
  RADDBGI_U64             procedures_count;
  RADDBGI_Scope*          scopes;
  RADDBGI_U64             scopes_count;
  RADDBGI_U64*            scope_voffs;
  RADDBGI_U64             scope_voffs_count;
  RADDBGI_VMapEntry*      scope_vmap;
  RADDBGI_U64             scope_vmap_count;
  RADDBGI_Local*          locals;
  RADDBGI_U64             locals_count;
  RADDBGI_LocationBlock*  location_blocks;
  RADDBGI_U64             location_blocks_count;
  RADDBGI_U8*             location_data;
  RADDBGI_U64             location_data_size;
  RADDBGI_NameMap*        name_maps;
  RADDBGI_U64             name_maps_count;
  
  // other helpers
  
  RADDBGI_NameMap* name_maps_by_kind[RADDBGI_NameMapKind_COUNT];
  
} RADDBGI_Parsed;

typedef enum{
  RADDBGI_ParseStatus_Good = 0,
  RADDBGI_ParseStatus_HeaderDoesNotMatch = 1,
  RADDBGI_ParseStatus_UnsupportedVersionNumber = 2,
  RADDBGI_ParseStatus_InvalidDataSecionLayout = 3,
  RADDBGI_ParseStatus_MissingStringDataSection = 4,
  RADDBGI_ParseStatus_MissingStringTableSection = 5,
  RADDBGI_ParseStatus_MissingIndexRunSection = 6,
} RADDBGI_ParseStatus;

typedef struct RADDBGI_ParsedLineInfo{
  // NOTE: Mapping VOFF -> LINE_INFO
  //
  // * [ voff[i], voff[i + 1] ) forms the voff range
  // * for the line info at lines[i] (and cols[i] if i < col_count)
  
  RADDBGI_U64*    voffs; // [count + 1] sorted
  RADDBGI_Line*   lines; // [count]
  RADDBGI_Column* cols;  // [col_count]
  RADDBGI_U64 count;
  RADDBGI_U64 col_count;
} RADDBGI_ParsedLineInfo;

typedef struct RADDBGI_ParsedLineMap{
  // NOTE: Mapping LINE_NUMBER -> VOFFs
  //
  // * nums[i] gives a line number
  // * that line number has one or more associated voffs
  //
  // * to find all associated voffs for the line number nums[i] :
  // * let k span over the range [ ranges[i], ranges[i + 1] )
  // * voffs[k] gives the associated voffs
  
  RADDBGI_U32* nums;   // [count] sorted
  RADDBGI_U32* ranges; // [count + 1]
  RADDBGI_U64* voffs;  // [voff_count]
  RADDBGI_U64 count;
  RADDBGI_U64 voff_count;
} RADDBGI_ParsedLineMap;


typedef struct RADDBGI_ParsedNameMap{
  RADDBGI_NameMapBucket *buckets;
  RADDBGI_NameMapNode *nodes;
  RADDBGI_U64 bucket_count;
  RADDBGI_U64 node_count;
} RADDBGI_ParsedNameMap;

////////////////////////////////
//~ Global Nils

#if !defined(RADDBGI_DISABLE_NILS)
static RADDBGI_BinarySection raddbgi_binary_section_nil = {0};
static RADDBGI_FilePathNode raddbgi_file_path_node_nil = {0};
static RADDBGI_SourceFile raddbgi_source_file_nil = {0};
static RADDBGI_Unit raddbgi_unit_nil = {0};
static RADDBGI_VMapEntry raddbgi_vmap_entry_nil = {0};
static RADDBGI_TypeNode raddbgi_type_node_nil = {0};
static RADDBGI_UDT raddbgi_udt_nil = {0};
static RADDBGI_Member raddbgi_member_nil = {0};
static RADDBGI_EnumMember raddbgi_enum_member_nil = {0};
static RADDBGI_GlobalVariable raddbgi_global_variable_nil = {0};
static RADDBGI_ThreadVariable raddbgi_thread_variable_nil = {0};
static RADDBGI_Procedure raddbgi_procedure_nil = {0};
static RADDBGI_Scope raddbgi_scope_nil = {0};
static U64 raddbgi_voff_nil = 0;
static RADDBGI_LocationBlock raddbgi_location_block_nil = {0};
static RADDBGI_Local raddbgi_local_nil = {0};
#endif

////////////////////////////////
//~ RADDBG Parse API

RADDBGI_PROC RADDBGI_ParseStatus
raddbgi_parse(RADDBGI_U8 *data, RADDBGI_U64 size, RADDBGI_Parsed *out);

RADDBGI_PROC RADDBGI_U8*
raddbgi_string_from_idx(RADDBGI_Parsed *parsed, RADDBGI_U32 idx, RADDBGI_U64 *len_out);

RADDBGI_PROC RADDBGI_U32*
raddbgi_idx_run_from_first_count(RADDBGI_Parsed *parsed, RADDBGI_U32 first, RADDBGI_U32 raw_count,
                                 RADDBGI_U32 *n_out);

//- table lookups
#define raddbgi_element_from_idx(parsed, name, idx) ((0 <= (idx) && (idx) < (parsed)->name##_count) ? &(parsed)->name[idx] : (parsed)->name ? &(parsed)->name[0] : 0)

//- line info
RADDBGI_PROC void
raddbgi_line_info_from_unit(RADDBGI_Parsed *p, RADDBGI_Unit *unit, RADDBGI_ParsedLineInfo *out);

RADDBGI_PROC RADDBGI_U64
raddbgi_line_info_idx_from_voff(RADDBGI_ParsedLineInfo *line_info, RADDBGI_U64 voff);

RADDBGI_PROC void
raddbgi_line_map_from_source_file(RADDBGI_Parsed *p, RADDBGI_SourceFile *srcfile,
                                  RADDBGI_ParsedLineMap *out);

RADDBGI_PROC RADDBGI_U64*
raddbgi_line_voffs_from_num(RADDBGI_ParsedLineMap *map, RADDBGI_U32 linenum, RADDBGI_U32 *n_out);


//- vmaps
RADDBGI_PROC RADDBGI_U64
raddbgi_vmap_idx_from_voff(RADDBGI_VMapEntry *vmap, RADDBGI_U32 vmap_count, RADDBGI_U64 voff);


//- name maps
RADDBGI_PROC RADDBGI_NameMap*
raddbgi_name_map_from_kind(RADDBGI_Parsed *p, RADDBGI_NameMapKind kind);

RADDBGI_PROC void
raddbgi_name_map_parse(RADDBGI_Parsed* p, RADDBGI_NameMap *mapptr, RADDBGI_ParsedNameMap *out);

RADDBGI_PROC RADDBGI_NameMapNode*
raddbgi_name_map_lookup(RADDBGI_Parsed *p, RADDBGI_ParsedNameMap *map,
                        RADDBGI_U8 *str, RADDBGI_U64 len);

RADDBGI_PROC RADDBGI_U32*
raddbgi_matches_from_map_node(RADDBGI_Parsed *p, RADDBGI_NameMapNode *node, RADDBGI_U32 *n_out);


//- common helpers
RADDBGI_PROC RADDBGI_U64
raddbgi_first_voff_from_proc(RADDBGI_Parsed *p, RADDBGI_U32 proc_id);



////////////////////////////////
//~ RADDBG Parsing Helpers

#define raddbgi_parse__extract_primary(p,outptr,outn,pritag) \
( (*(void**)&(outptr)) = \
raddbgi_data_from_dsec((p),(p)->dsec_idx[pritag],sizeof(*(outptr)),(pritag),(outn)) )

RADDBGI_PROC void*
raddbgi_data_from_dsec(RADDBGI_Parsed *p, RADDBGI_U32 idx, RADDBGI_U32 item_size,
                       RADDBGI_DataSectionTag expected_tag, RADDBGI_U64 *n_out);

#define raddbgi_parse__min(a,b) (((a)<(b))?(a):(b))

#endif // RADDBGI_PARSE_H
