// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_PARSE_H
#define RADDBG_PARSE_H

////////////////////////////////
//~ RADDBG Parsing Helpers

typedef struct RADDBG_Parsed{
  // raw data & data sections (part 1)
  RADDBG_U8 *raw_data;
  RADDBG_U64 raw_data_size;
  RADDBG_DataSection *dsecs;
  RADDBG_U64 dsec_count;
  RADDBG_U32 dsec_idx[RADDBG_DataSectionTag_PRIMARY_COUNT];
  
  // primary data structures (part 2)
  
  //  handled by helper APIs
  RADDBG_U8*  string_data;
  RADDBG_U64  string_data_size;
  RADDBG_U32* string_offs;
  RADDBG_U64  string_count;
  RADDBG_U32* idx_run_data;
  RADDBG_U32  idx_run_count;
  
  //  directly readable by users
  //  (any of these may be empty and null even in a successful parse)
  RADDBG_TopLevelInfo* top_level_info;
  
  RADDBG_BinarySection*  binary_sections;
  RADDBG_U64             binary_sections_count;
  RADDBG_FilePathNode*   file_paths;
  RADDBG_U64             file_paths_count;
  RADDBG_SourceFile*     source_files;
  RADDBG_U64             source_files_count;
  RADDBG_Unit*           units;
  RADDBG_U64             units_count;
  RADDBG_VMapEntry*      unit_vmap;
  RADDBG_U64             unit_vmap_count;
  RADDBG_TypeNode*       type_nodes;
  RADDBG_U64             type_nodes_count;
  RADDBG_UDT*            udts;
  RADDBG_U64             udts_count;
  RADDBG_Member*         members;
  RADDBG_U64             members_count;
  RADDBG_EnumMember*     enum_members;
  RADDBG_U64             enum_members_count;
  RADDBG_GlobalVariable* global_variables;
  RADDBG_U64             global_variables_count;
  RADDBG_VMapEntry*      global_vmap;
  RADDBG_U64             global_vmap_count;
  RADDBG_ThreadVariable* thread_variables;
  RADDBG_U64             thread_variables_count;
  RADDBG_Procedure*      procedures;
  RADDBG_U64             procedures_count;
  RADDBG_Scope*          scopes;
  RADDBG_U64             scopes_count;
  RADDBG_U64*            scope_voffs;
  RADDBG_U64             scope_voffs_count;
  RADDBG_VMapEntry*      scope_vmap;
  RADDBG_U64             scope_vmap_count;
  RADDBG_Local*          locals;
  RADDBG_U64             locals_count;
  RADDBG_LocationBlock*  location_blocks;
  RADDBG_U64             location_blocks_count;
  RADDBG_U8*             location_data;
  RADDBG_U64             location_data_size;
  RADDBG_NameMap*        name_maps;
  RADDBG_U64             name_maps_count;
  
  // other helpers
  
  RADDBG_NameMap* name_maps_by_kind[RADDBG_NameMapKind_COUNT];
  
} RADDBG_Parsed;

typedef enum{
  RADDBG_ParseStatus_Good = 0,
  RADDBG_ParseStatus_HeaderDoesNotMatch = 1,
  RADDBG_ParseStatus_UnsupportedVersionNumber = 2,
  RADDBG_ParseStatus_InvalidDataSecionLayout = 3,
  RADDBG_ParseStatus_MissingStringDataSection = 4,
  RADDBG_ParseStatus_MissingStringTableSection = 5,
  RADDBG_ParseStatus_MissingIndexRunSection = 6,
} RADDBG_ParseStatus;

typedef struct RADDBG_ParsedLineInfo{
  // NOTE: Mapping VOFF -> LINE_INFO
  //
  // * [ voff[i], voff[i + 1] ) forms the voff range
  // * for the line info at lines[i] (and cols[i] if i < col_count)
  
  RADDBG_U64*    voffs; // [count + 1] sorted
  RADDBG_Line*   lines; // [count]
  RADDBG_Column* cols;  // [col_count]
  RADDBG_U64 count;
  RADDBG_U64 col_count;
} RADDBG_ParsedLineInfo;

typedef struct RADDBG_ParsedLineMap{
  // NOTE: Mapping LINE_NUMBER -> VOFFs
  //
  // * nums[i] gives a line number
  // * that line number has one or more associated voffs
  //
  // * to find all associated voffs for the line number nums[i] :
  // * let k span over the range [ ranges[i], ranges[i + 1] )
  // * voffs[k] gives the associated voffs
  
  RADDBG_U32* nums;   // [count] sorted
  RADDBG_U32* ranges; // [count + 1]
  RADDBG_U64* voffs;  // [voff_count]
  RADDBG_U64 count;
  RADDBG_U64 voff_count;
} RADDBG_ParsedLineMap;


typedef struct RADDBG_ParsedNameMap{
  RADDBG_NameMapBucket *buckets;
  RADDBG_NameMapNode *nodes;
  RADDBG_U64 bucket_count;
  RADDBG_U64 node_count;
} RADDBG_ParsedNameMap;

////////////////////////////////
//~ Global Nils

#if !defined(RADDBG_DISABLE_NILS)
static RADDBG_BinarySection raddbg_binary_section_nil = {0};
static RADDBG_FilePathNode raddbg_file_path_node_nil = {0};
static RADDBG_SourceFile raddbg_source_file_nil = {0};
static RADDBG_Unit raddbg_unit_nil = {0};
static RADDBG_VMapEntry raddbg_vmap_entry_nil = {0};
static RADDBG_TypeNode raddbg_type_node_nil = {0};
static RADDBG_UDT raddbg_udt_nil = {0};
static RADDBG_Member raddbg_member_nil = {0};
static RADDBG_EnumMember raddbg_enum_member_nil = {0};
static RADDBG_GlobalVariable raddbg_global_variable_nil = {0};
static RADDBG_ThreadVariable raddbg_thread_variable_nil = {0};
static RADDBG_Procedure raddbg_procedure_nil = {0};
static RADDBG_Scope raddbg_scope_nil = {0};
static U64 raddbg_voff_nil = 0;
static RADDBG_LocationBlock raddbg_location_block_nil = {0};
static RADDBG_Local raddbg_local_nil = {0};
#endif

////////////////////////////////
//~ RADDBG Parse API

RADDBG_PROC RADDBG_ParseStatus
raddbg_parse(RADDBG_U8 *data, RADDBG_U64 size, RADDBG_Parsed *out);

RADDBG_PROC RADDBG_U8*
raddbg_string_from_idx(RADDBG_Parsed *parsed, RADDBG_U32 idx, RADDBG_U64 *len_out);

RADDBG_PROC RADDBG_U32*
raddbg_idx_run_from_first_count(RADDBG_Parsed *parsed, RADDBG_U32 first, RADDBG_U32 raw_count,
                                RADDBG_U32 *n_out);

//- table lookups
#define raddbg_element_from_idx(parsed, name, idx) ((0 <= (idx) && (idx) < (parsed)->name##_count) ? &(parsed)->name[idx] : (parsed)->name ? &(parsed)->name[0] : 0)

//- line info
RADDBG_PROC void
raddbg_line_info_from_unit(RADDBG_Parsed *p, RADDBG_Unit *unit, RADDBG_ParsedLineInfo *out);

RADDBG_PROC RADDBG_U64
raddbg_line_info_idx_from_voff(RADDBG_ParsedLineInfo *line_info, RADDBG_U64 voff);

RADDBG_PROC void
raddbg_line_map_from_source_file(RADDBG_Parsed *p, RADDBG_SourceFile *srcfile,
                                 RADDBG_ParsedLineMap *out);

RADDBG_PROC RADDBG_U64*
raddbg_line_voffs_from_num(RADDBG_ParsedLineMap *map, RADDBG_U32 linenum, RADDBG_U32 *n_out);


//- vmaps
RADDBG_PROC RADDBG_U64
raddbg_vmap_idx_from_voff(RADDBG_VMapEntry *vmap, RADDBG_U32 vmap_count, RADDBG_U64 voff);


//- name maps
RADDBG_PROC RADDBG_NameMap*
raddbg_name_map_from_kind(RADDBG_Parsed *p, RADDBG_NameMapKind kind);

RADDBG_PROC void
raddbg_name_map_parse(RADDBG_Parsed* p, RADDBG_NameMap *mapptr, RADDBG_ParsedNameMap *out);

RADDBG_PROC RADDBG_NameMapNode*
raddbg_name_map_lookup(RADDBG_Parsed *p, RADDBG_ParsedNameMap *map,
                       RADDBG_U8 *str, RADDBG_U64 len);

RADDBG_PROC RADDBG_U32*
raddbg_matches_from_map_node(RADDBG_Parsed *p, RADDBG_NameMapNode *node, RADDBG_U32 *n_out);


//- common helpers
RADDBG_PROC RADDBG_U64
raddbg_first_voff_from_proc(RADDBG_Parsed *p, RADDBG_U32 proc_id);



////////////////////////////////
//~ RADDBG Parsing Helpers

#define raddbg_parse__extract_primary(p,outptr,outn,pritag) \
( (*(void**)&(outptr)) = \
raddbg_data_from_dsec((p),(p)->dsec_idx[pritag],sizeof(*(outptr)),(pritag),(outn)) )

RADDBG_PROC void*
raddbg_data_from_dsec(RADDBG_Parsed *p, RADDBG_U32 idx, RADDBG_U32 item_size,
                      RADDBG_DataSectionTag expected_tag, RADDBG_U64 *n_out);

#define raddbg_parse__min(a,b) (((a)<(b))?(a):(b))

#endif //RADDBG_PARSE_H
