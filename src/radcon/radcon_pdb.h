// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADCON_PDB_H
#define RADCON_PDB_H

////////////////////////////////
//~ rjf: Initial PDB Information Extraction & Conversion Preparation Task Types

//- rjf: tpi hash parsing

typedef struct P2R_TPIHashParseIn P2R_TPIHashParseIn;
struct P2R_TPIHashParseIn
{
  PDB_Strtbl *strtbl;
  PDB_TpiParsed *tpi;
  String8 hash_data;
  String8 aux_data;
};

//- rjf: tpi leaves parsing

typedef struct P2R_TPILeafParseIn P2R_TPILeafParseIn;
struct P2R_TPILeafParseIn
{
  String8 leaf_data;
  CV_TypeId itype_first;
};

//- rjf: exe hashing

typedef struct P2R_EXEHashIn P2R_EXEHashIn;
struct P2R_EXEHashIn
{
  String8 exe_data;
};

//- rjf: symbol stream parsing

typedef struct P2R_SymbolStreamParseIn P2R_SymbolStreamParseIn;
struct P2R_SymbolStreamParseIn
{
  String8 data;
};

//- rjf: c13 line info stream parsing

typedef struct P2R_C13StreamParseIn P2R_C13StreamParseIn;
struct P2R_C13StreamParseIn
{
  String8 data;
  String8 strtbl;
  COFF_SectionHeaderArray coff_sections;
};

//- rjf: comp unit parsing

typedef struct P2R_CompUnitParseIn P2R_CompUnitParseIn;
struct P2R_CompUnitParseIn
{
  String8 data;
};

//- rjf: comp unit contribution table parsing

typedef struct P2R_CompUnitContributionsParseIn P2R_CompUnitContributionsParseIn;
struct P2R_CompUnitContributionsParseIn
{
  String8 data;
  COFF_SectionHeaderArray coff_sections;
};

////////////////////////////////
//~ rjf: Conversion Data Structure & Task Types

//- rjf: link name map (voff -> string)

typedef struct P2R_LinkNameNode P2R_LinkNameNode;
struct P2R_LinkNameNode
{
  P2R_LinkNameNode *next;
  U64 voff;
  String8 name;
};

typedef struct P2R_LinkNameMap P2R_LinkNameMap;
struct P2R_LinkNameMap
{
  P2R_LinkNameNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 link_name_count;
};

//- rjf: normalized file path -> source file map

typedef struct P2R_SrcFileNode P2R_SrcFileNode;
struct P2R_SrcFileNode
{
  P2R_SrcFileNode *next;
  RDIM_SrcFile *src_file;
};

typedef struct P2R_SrcFileMap P2R_SrcFileMap;
struct P2R_SrcFileMap
{
  P2R_SrcFileNode **slots;
  U64 slots_count;
};

//- rjf: unit conversion tasks

typedef struct P2R_UnitConvertIn P2R_UnitConvertIn;
struct P2R_UnitConvertIn
{
  PDB_Strtbl *pdb_strtbl;
  COFF_SectionHeaderArray coff_sections;
  PDB_CompUnitArray *comp_units;
  PDB_CompUnitContributionArray *comp_unit_contributions;
  CV_SymParsed **comp_unit_syms;
  CV_C13Parsed **comp_unit_c13s;
};

typedef struct P2R_UnitConvertOut P2R_UnitConvertOut;
struct P2R_UnitConvertOut
{
  RDIM_UnitChunkList units;
  RDIM_SrcFileChunkList src_files;
  RDIM_LineTableChunkList line_tables;
  RDIM_LineTable **units_first_inline_site_line_tables;
};

//- rjf: link name map building tasks

typedef struct P2R_LinkNameMapBuildIn P2R_LinkNameMapBuildIn;
struct P2R_LinkNameMapBuildIn
{
  CV_SymParsed *sym;
  COFF_SectionHeaderArray coff_sections;
  P2R_LinkNameMap *link_name_map;
};

//- rjf: udt conversion

typedef struct P2R_UDTConvertIn P2R_UDTConvertIn;
struct P2R_UDTConvertIn
{
  CV_LeafParsed *tpi_leaf;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  RDIM_Type **itype_type_ptrs;
};

//- rjf: symbol stream conversion

typedef struct P2R_SymbolStreamConvertIn P2R_SymbolStreamConvertIn;
struct P2R_SymbolStreamConvertIn
{
  RDI_Arch arch;
  COFF_SectionHeaderArray coff_sections;
  PDB_TpiHashParsed *tpi_hash;
  CV_LeafParsed *tpi_leaf;
  CV_LeafParsed *ipi_leaf;
  CV_SymParsed *sym;
  U64 sym_ranges_first;
  U64 sym_ranges_opl;
  RDIM_Type **itype_type_ptrs;
  P2R_LinkNameMap *link_name_map;
  RDIM_LineTable *first_inline_site_line_table;
};

typedef struct P2R_SymbolStreamConvertOut P2R_SymbolStreamConvertOut;
struct P2R_SymbolStreamConvertOut
{
  RDIM_SymbolChunkList procedures;
  RDIM_SymbolChunkList global_variables;
  RDIM_SymbolChunkList thread_variables;
  RDIM_ScopeChunkList scopes;
  RDIM_InlineSiteChunkList inline_sites;
};

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 p2r_end_of_cplusplus_container_name(String8 str);
internal U64 p2r_hash_from_voff(U64 voff);

////////////////////////////////
//~ rjf: Location Info Building Helpers

internal RDIM_Location *p2r_location_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection);
internal RDI_RegCode p2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg);
internal void p2r_location_over_lvar_addr_range(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_LocationSet *locset, RDIM_Location *location, CV_LvarAddrRange *range, COFF_SectionHeader *section, CV_LvarAddrGap *gaps, U64 gap_count);

////////////////////////////////
//~ rjf: Initial Parsing & Preparation Pass Tasks

ASYNC_WORK_DEF(p2r_exe_hash_work);
ASYNC_WORK_DEF(p2r_tpi_hash_parse_work);
ASYNC_WORK_DEF(p2r_tpi_leaf_work);
ASYNC_WORK_DEF(p2r_symbol_stream_parse_work);
ASYNC_WORK_DEF(p2r_c13_stream_parse_work);
ASYNC_WORK_DEF(p2r_comp_unit_parse_work);
ASYNC_WORK_DEF(p2r_comp_unit_contributions_parse_work);

////////////////////////////////
//~ rjf: Unit Conversion Tasks

ASYNC_WORK_DEF(p2r_units_convert_work);

////////////////////////////////
//~ rjf: Link Name Map Building Tasks

ASYNC_WORK_DEF(p2r_link_name_map_build_work);

////////////////////////////////
//~ rjf: UDT Conversion Tasks

ASYNC_WORK_DEF(p2r_udt_convert_work);

////////////////////////////////
//~ rjf: Symbol Stream Conversion Tasks

ASYNC_WORK_DEF(p2r_symbol_stream_convert_work);

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal RDIM_BakeParams *p2r_convert(Arena *arena, RDIM_LocalState *local_state, RC_Context *in);

////////////////////////////////

internal B32 p2r_has_symbol_ref(String8 msf_data, String8List symbol_list, MSF_RawStreamTable *st);
internal B32 p2r_has_file_ref(String8 msf_data, String8List file_list, MSF_RawStreamTable *st);
internal B32 p2r_has_symbol_or_file_ref(String8 msf_data, String8List symbol_list, String8List file_list);

#endif // RADCON_PDB_H

