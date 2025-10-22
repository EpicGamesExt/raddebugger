// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FORMAT_LOCAL_H
#define RDI_FORMAT_LOCAL_H

#include "lib_rdi/rdi.h"
#include "lib_rdi/rdi_parse.h"

////////////////////////////////
//~ rjf: RDI Dumping Flags

#define RDI_DumpSubset_XList \
X(DataSections,        data_sections,               "DATA SECTIONS")\
X(TopLevelInfo,        top_level_info,              "TOP LEVEL INFO")\
X(BinarySections,      binary_sections,             "BINARY SECTIONS")\
X(FilePaths,           file_paths,                  "FILE PATHS")\
X(SourceFiles,         source_files,                "SOURCE FILES")\
X(LineTables,          line_tables,                 "LINE TABLES")\
X(SourceLineMaps,      source_line_maps,            "SOURCE LINE MAPS")\
X(Units,               units,                       "UNITS")\
X(UnitVMap,            unit_vmap,                   "UNIT VMAP")\
X(TypeNodes,           type_nodes,                  "TYPE NODES")\
X(UserDefinedTypes,    user_defined_types,          "USER DEFINED TYPES")\
X(GlobalVariables,     global_variables,            "GLOBAL VARIABLES")\
X(GlobalVariablesVMap, global_variables_vmap,       "GLOBAL VARIABLE VMAP")\
X(ThreadVariables,     thread_variables,            "THREAD VARIABLES")\
X(Constants,           constants,                   "CONSTANTS")\
X(Procedures,          procedures,                  "PROCEDURES")\
X(Scopes,              scopes,                      "SCOPES")\
X(ScopeVMap,           scope_vmap,                  "SCOPE VMAP")\
X(InlineSites,         inline_sites,                "INLINE SITES")\
X(NameMaps,            name_maps,                   "NAME MAPS")\
X(Strings,             strings,                     "STRINGS")\

typedef enum RDI_DumpSubset
{
#define X(name, name_lower, title) RDI_DumpSubset_##name,
  RDI_DumpSubset_XList
#undef X
}
RDI_DumpSubset;

typedef U32 RDI_DumpSubsetFlags;
enum
{
#define X(name, name_lower, title) RDI_DumpSubsetFlag_##name = (1<<RDI_DumpSubset_##name),
  RDI_DumpSubset_XList
#undef X
  RDI_DumpSubsetFlag_All = 0xffffffffu,
};

read_only global String8 rdi_name_lowercase_from_dump_subset_table[] =
{
#define X(name, name_lower, title) str8_lit_comp(#name_lower),
  RDI_DumpSubset_XList
#undef X
};

read_only global String8 rdi_name_title_from_dump_subset_table[] =
{
#define X(name, name_lower, title) str8_lit_comp(title),
  RDI_DumpSubset_XList
#undef X
};

////////////////////////////////
//~ rjf: RDI Enum <=> Base Enum

internal Arch arch_from_rdi_arch(RDI_Arch arch);

////////////////////////////////
//~ rjf: Lookup Helpers

internal String8 str8_from_rdi_string_idx(RDI_Parsed *rdi, U32 idx);

////////////////////////////////
//~ rjf: String <=> Enum

internal String8 rdi_string_from_data_section_kind(Arena *arena, RDI_SectionKind v);
internal String8 rdi_string_from_arch             (Arena *arena, RDI_Arch        v);
internal String8 rdi_string_from_language         (Arena *arena, RDI_Language    v);
internal String8 rdi_string_from_local_kind       (Arena *arena, RDI_LocalKind   v);
#if 0 // TODO(rjf): conflicts with RDI...
internal String8 rdi_string_from_type_kind        (Arena *arena, RDI_TypeKind    v);
#endif
internal String8 rdi_string_from_member_kind      (Arena *arena, RDI_MemberKind  v);
internal String8 rdi_string_from_name_map_kind(RDI_NameMapKind kind);

internal String8 rdi_string_from_binary_section_flags(Arena *arena, RDI_BinarySectionFlags flags);
internal String8 rdi_string_from_type_modifier       (Arena *arena, RDI_TypeModifierFlags  flags);
internal String8 rdi_string_from_udt_flags           (Arena *arena, RDI_UDTFlags           flags);
internal String8 rdi_string_from_link_flags          (Arena *arena, RDI_LinkFlags          flags);
internal String8 rdi_string_from_bytecode(Arena *arena, RDI_Arch arch, String8 bc);
internal String8List rdi_strings_from_locations(Arena *arena, RDI_Parsed *rdi, RDI_Arch arch, Rng1U64 location_block_range);

////////////////////////////////
//~ rjf: RDI Dumping

internal String8List rdi_dump_list_from_parsed(Arena *arena, RDI_Parsed *rdi, RDI_DumpSubsetFlags flags);

#endif // RDI_FORMAT_LOCAL_H
