// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////////////////////////////////////
//~ RAD Debug Info, (R)AD(D)BG(I) Format Parsing Library
//
// Defines helper types and functions for extracting data from
// RDI files.

////////////////////////////////////////////////////////////////
//~ Usage Samples
//
#if 0
// Procedure Name -> Line
{
  RDI_Parsed *rdi = ...;
  char *name = "mule_main";
  RDI_Procedure *procedure = rdi_procedure_from_name_cstr(rdi, name);                  // 1. name -> procedure
  RDI_U64 procedure_first_voff = rdi_first_voff_from_procedure(rdi, procedure);        // 2. procedure -> virtual offset
  RDI_Line line = rdi_line_from_voff(rdi, procedure_first_voff);                       // 3. virtual offset -> line
  RDI_SourceFile *file = rdi_source_file_from_line(rdi, &line);                        // 4. line -> source file
  RDI_U64 file_path_size = 0;                                                          // 5. source file -> path
  RDI_U8 *file_path = rdi_normal_path_from_source_file(rdi, file, &file_path_size);
  printf("%s is at %.*s:%u\n", name, (int)file_path_size, file_path, line.line_num);
}

// Line -> Procedure Name
{
  RDI_Parsed *rdi = ...;
  char *path = "c:/devel/raddebugger/src/mule/mule_main.cpp";
  RDI_U32 line_num = 2557;
  RDI_SourceFile *file = rdi_source_file_from_normal_path_cstr(rdi, path);      // 1. path -> source file
  RDI_U64 voff = rdi_first_voff_from_source_file_line_num(rdi, file, line_num); // 2. (source file, line) -> virtual offset
  RDI_Procedure *procedure = rdi_procedure_from_voff(rdi, voff);                // 3. virtual offset -> procedure
  RDI_U64 name_size = 0;                                                        // 4. procedure -> name
  RDI_U8 *name = rdi_name_from_procedure(rdi, procedure, &name_size);
  printf("%s:%u is inside %.*s\n", path, line_num, (int)name_size, name);
}
#endif

#ifndef RDI_FORMAT_PARSE_H
#define RDI_FORMAT_PARSE_H

////////////////////////////////////////////////////////////////
//~ Parsed Information Types

typedef enum RDI_ParseStatus
{
  RDI_ParseStatus_Good = 0,
  RDI_ParseStatus_HeaderDoesNotMatch = 1,
  RDI_ParseStatus_UnsupportedVersionNumber = 2,
  RDI_ParseStatus_InvalidDataSecionLayout = 3,
  RDI_ParseStatus_MissingRequiredSection = 4,
}
RDI_ParseStatus;

typedef struct RDI_Parsed RDI_Parsed;
struct RDI_Parsed
{
  RDI_U8 *raw_data;
  RDI_U64 raw_data_size;
  RDI_Section *sections;
  RDI_U64 sections_count;
};

typedef struct RDI_ParsedLineTable RDI_ParsedLineTable;
struct RDI_ParsedLineTable
{
  // NOTE: Mapping VOFF -> LINE_INFO
  //
  // * [ voff[i], voff[i + 1] ) forms the voff range
  // * for the line info at lines[i] (and cols[i] if i < col_count)
  RDI_U64*    voffs; // [count + 1] sorted
  RDI_Line*   lines; // [count]
  RDI_Column* cols;  // [col_count]
  RDI_U64 count;
  RDI_U64 col_count;
};

typedef struct RDI_ParsedSourceLineMap RDI_ParsedSourceLineMap;
struct RDI_ParsedSourceLineMap
{
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
};

typedef struct RDI_ParsedNameMap RDI_ParsedNameMap;
struct RDI_ParsedNameMap
{
  RDI_NameMapBucket *buckets;
  RDI_NameMapNode *nodes;
  RDI_U64 bucket_count;
  RDI_U64 node_count;
};

////////////////////////////////
//~ Global Nils

static union
{
  RDI_TopLevelInfo top_level_info;
  RDI_BinarySection binary_section;
  RDI_FilePathNode file_path_node;
  RDI_SourceFile source_file;
  RDI_LineTable line_table;
  RDI_SourceLineMap source_line_map;
  RDI_Line line;
  RDI_Column column;
  RDI_Unit unit;
  RDI_VMapEntry vmap_entry;
  RDI_TypeNode type_node;
  RDI_UDT udt;
  RDI_Member member;
  RDI_EnumMember enum_member;
  RDI_GlobalVariable global_variable;
  RDI_ThreadVariable thread_variable;
  RDI_Procedure procedure;
  RDI_Scope scope;
  RDI_U64 voff;
  RDI_LocationBlock location_block;
  RDI_Local local;
}
rdi_nil_element_union = {0};
static RDI_Parsed rdi_parsed_nil = {0};

////////////////////////////////
//~ Top-Level Parsing API

RDI_PROC RDI_ParseStatus rdi_parse(RDI_U8 *data, RDI_U64 size, RDI_Parsed *out);

////////////////////////////////
//~ Base Parsed Info Extraction Helpers

//- section table/element raw data extraction
RDI_PROC void *rdi_section_raw_data_from_kind(RDI_Parsed *rdi, RDI_SectionKind kind, RDI_SectionEncoding *encoding_out, RDI_U64 *size_out);
RDI_PROC void *rdi_section_raw_table_from_kind(RDI_Parsed *rdi, RDI_SectionKind kind, RDI_U64 *count_out);
RDI_PROC void *rdi_section_raw_element_from_kind_idx(RDI_Parsed *rdi, RDI_SectionKind kind, RDI_U64 idx);
#define rdi_table_from_name(rdi, name, count_out) (RDI_SectionElementType_##name *)rdi_section_raw_table_from_kind((rdi), RDI_SectionKind_##name, (count_out))
#define rdi_element_from_name_idx(rdi, name, idx) (RDI_SectionElementType_##name *)rdi_section_raw_element_from_kind_idx((rdi), RDI_SectionKind_##name, (idx))

//- info about whole parse
RDI_PROC RDI_U64 rdi_decompressed_size_from_parsed(RDI_Parsed *rdi);

//- strings
RDI_PROC RDI_U8 *rdi_string_from_idx(RDI_Parsed *rdi, RDI_U32 idx, RDI_U64 *len_out);

//- index runs
RDI_PROC RDI_U32 *rdi_idx_run_from_first_count(RDI_Parsed *rdi, RDI_U32 raw_first, RDI_U32 raw_count, RDI_U32 *n_out);

//- line info
RDI_PROC void rdi_parsed_from_line_table(RDI_Parsed *rdi, RDI_LineTable *line_table, RDI_ParsedLineTable *out);
RDI_PROC RDI_U64 rdi_line_info_idx_range_from_voff(RDI_ParsedLineTable *line_info, RDI_U64 voff, RDI_U64 *n_out);
RDI_PROC RDI_U64 rdi_line_info_idx_from_voff(RDI_ParsedLineTable *line_info, RDI_U64 voff);
RDI_PROC void rdi_parsed_from_source_line_map(RDI_Parsed *rdi, RDI_SourceLineMap *map, RDI_ParsedSourceLineMap *out);
RDI_PROC RDI_U64 *rdi_line_voffs_from_num(RDI_ParsedSourceLineMap *map, RDI_U32 linenum, RDI_U32 *n_out);

//- vmap lookups
RDI_PROC RDI_U64 rdi_vmap_idx_from_voff(RDI_VMapEntry *vmap, RDI_U64 vmap_count, RDI_U64 voff);

//- name maps
RDI_PROC RDI_NameMap *rdi_name_map_from_kind(RDI_Parsed *p, RDI_NameMapKind kind);
RDI_PROC void rdi_name_map_parse(RDI_Parsed* p, RDI_NameMap *mapptr, RDI_ParsedNameMap *out);
RDI_PROC RDI_NameMapNode *rdi_name_map_lookup(RDI_Parsed *p, RDI_ParsedNameMap *map, RDI_U8 *str, RDI_U64 len);
RDI_PROC RDI_U32 *rdi_matches_from_map_node(RDI_Parsed *p, RDI_NameMapNode *node, RDI_U32 *n_out);

////////////////////////////////
//~ High-Level Composite Lookup Functions

//- procedures
RDI_PROC RDI_Procedure *rdi_procedure_from_name(RDI_Parsed *rdi, RDI_U8 *name, RDI_U64 name_size);
RDI_PROC RDI_Procedure *rdi_procedure_from_name_cstr(RDI_Parsed *rdi, char *cstr);
RDI_PROC RDI_U8 *rdi_name_from_procedure(RDI_Parsed *rdi, RDI_Procedure *procedure, RDI_U64 *len_out);
RDI_PROC RDI_Scope *rdi_root_scope_from_procedure(RDI_Parsed *rdi, RDI_Procedure *procedure);
RDI_PROC RDI_U64 rdi_first_voff_from_procedure(RDI_Parsed *rdi, RDI_Procedure *procedure);
RDI_PROC RDI_U64 rdi_opl_voff_from_procedure(RDI_Parsed *rdi, RDI_Procedure *procedure);
RDI_PROC RDI_Procedure *rdi_procedure_from_voff(RDI_Parsed *rdi, RDI_U64 voff);

//- scopes
RDI_PROC RDI_U64 rdi_first_voff_from_scope(RDI_Parsed *rdi, RDI_Scope *scope);
RDI_PROC RDI_U64 rdi_opl_voff_from_scope(RDI_Parsed *rdi, RDI_Scope *scope);
RDI_PROC RDI_Scope *rdi_scope_from_voff(RDI_Parsed *rdi, RDI_U64 voff);
RDI_PROC RDI_Scope *rdi_parent_from_scope(RDI_Parsed *rdi, RDI_Scope *scope);
RDI_PROC RDI_Procedure *rdi_procedure_from_scope(RDI_Parsed *rdi, RDI_Scope *scope);
RDI_PROC RDI_InlineSite *rdi_inline_site_from_scope(RDI_Parsed *rdi, RDI_Scope *scope);

//- global variables
RDI_PROC RDI_GlobalVariable *rdi_global_variable_from_voff(RDI_Parsed *rdi, RDI_U64 voff);

//- units
RDI_PROC RDI_Unit *rdi_unit_from_voff(RDI_Parsed *rdi, RDI_U64 voff);
RDI_PROC RDI_LineTable *rdi_line_table_from_unit(RDI_Parsed *rdi, RDI_Unit *unit);

//- line tables
RDI_PROC RDI_Line rdi_line_from_voff(RDI_Parsed *rdi, RDI_U64 voff);
RDI_PROC RDI_Line rdi_line_from_line_table_voff(RDI_Parsed *rdi, RDI_LineTable *line_table, RDI_U64 voff);
RDI_PROC RDI_SourceFile *rdi_source_file_from_line(RDI_Parsed *rdi, RDI_Line *line);

//- source files
RDI_PROC RDI_SourceFile *rdi_source_file_from_normal_path(RDI_Parsed *rdi, RDI_U8 *name, RDI_U64 name_size);
RDI_PROC RDI_SourceFile *rdi_source_file_from_normal_path_cstr(RDI_Parsed *rdi, char *cstr);
RDI_PROC RDI_U8 *rdi_normal_path_from_source_file(RDI_Parsed *rdi, RDI_SourceFile *src_file, RDI_U64 *len_out);
RDI_PROC RDI_FilePathNode *rdi_file_path_node_from_source_file(RDI_Parsed *rdi, RDI_SourceFile *src_file);
RDI_PROC RDI_SourceLineMap *rdi_source_line_map_from_source_file(RDI_Parsed *rdi, RDI_SourceFile *src_file);
RDI_PROC RDI_U64 rdi_first_voff_from_source_file_line_num(RDI_Parsed *rdi, RDI_SourceFile *src_file, RDI_U32 line_num);

//- source line maps
RDI_PROC RDI_U64 rdi_first_voff_from_source_line_map_num(RDI_Parsed *rdi, RDI_SourceLineMap *map, RDI_U32 line_num);

//- file path nodes
RDI_PROC RDI_FilePathNode *rdi_parent_from_file_path_node(RDI_Parsed *rdi, RDI_FilePathNode *node);
RDI_PROC RDI_U8 *rdi_name_from_file_path_node(RDI_Parsed *rdi, RDI_FilePathNode *node, RDI_U64 *len_out);

////////////////////////////////
//~ Parser Helpers

#define rdi_parse__min(a,b) (((a)<(b))?(a):(b))
RDI_PROC RDI_U64 rdi_cstring_length(char *cstr);
RDI_PROC RDI_U64 rdi_size_from_bytecode_stream(RDI_U8 *ptr, RDI_U8 *opl);

#endif // RDI_FORMAT_PARSE_H
