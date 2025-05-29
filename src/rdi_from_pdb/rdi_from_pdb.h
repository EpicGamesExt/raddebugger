// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_PDB_H
#define RDI_FROM_PDB_H

////////////////////////////////
//~ rjf: Export Artifact Flags

typedef U32 P2R_ConvertFlags;
enum
{
  P2R_ConvertFlag_Strings                 = (1<<0),
  P2R_ConvertFlag_IndexRuns               = (1<<1),
  P2R_ConvertFlag_BinarySections          = (1<<2),
  P2R_ConvertFlag_Units                   = (1<<3),
  P2R_ConvertFlag_Procedures              = (1<<4),
  P2R_ConvertFlag_GlobalVariables         = (1<<5),
  P2R_ConvertFlag_ThreadVariables         = (1<<6),
  P2R_ConvertFlag_Scopes                  = (1<<7),
  P2R_ConvertFlag_Locals                  = (1<<8),
  P2R_ConvertFlag_Types                   = (1<<9),
  P2R_ConvertFlag_UDTs                    = (1<<10),
  P2R_ConvertFlag_LineInfo                = (1<<11),
  P2R_ConvertFlag_GlobalVariableNameMap   = (1<<12),
  P2R_ConvertFlag_ThreadVariableNameMap   = (1<<13),
  P2R_ConvertFlag_ProcedureNameMap        = (1<<14),
  P2R_ConvertFlag_TypeNameMap             = (1<<15),
  P2R_ConvertFlag_LinkNameProcedureNameMap= (1<<16),
  P2R_ConvertFlag_NormalSourcePathNameMap = (1<<17),
  P2R_ConvertFlag_Deterministic           = (1<<18),
  P2R_ConvertFlag_All = 0xffffffff,
};

////////////////////////////////
//~ rjf: Conversion Stage Inputs/Outputs

typedef struct P2R_User2Convert P2R_User2Convert;
struct P2R_User2Convert
{
  String8 input_pdb_name;
  String8 input_pdb_data;
  String8 input_exe_name;
  String8 input_exe_data;
  String8 output_name;
  P2R_ConvertFlags flags;
  String8List errors;
};

typedef struct P2R_Convert2Bake P2R_Convert2Bake;
struct P2R_Convert2Bake
{
  RDIM_BakeParams bake_params;
};

typedef struct P2R_Bake2Serialize P2R_Bake2Serialize;
struct P2R_Bake2Serialize
{
  RDIM_BakeResults bake_results;
};

typedef struct P2R_Serialize2File P2R_Serialize2File;
struct P2R_Serialize2File
{
  RDIM_SerializedSectionBundle bundle;
};

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

//- rjf: type forward resolution map build

typedef struct P2R_ITypeFwdMapFillIn P2R_ITypeFwdMapFillIn;
struct P2R_ITypeFwdMapFillIn
{
  PDB_TpiHashParsed *tpi_hash;
  CV_LeafParsed *tpi_leaf;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  CV_TypeId *itype_fwd_map;
};

//- rjf: itype chain build

typedef struct P2R_TypeIdChain P2R_TypeIdChain;
struct P2R_TypeIdChain
{
  P2R_TypeIdChain *next;
  CV_TypeId itype;
};

typedef struct P2R_ITypeChainBuildIn P2R_ITypeChainBuildIn;
struct P2R_ITypeChainBuildIn
{
  CV_LeafParsed *tpi_leaf;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  CV_TypeId *itype_fwd_map;
  P2R_TypeIdChain **itype_chains;
};

//- rjf: udt conversion

typedef struct P2R_UDTConvertIn P2R_UDTConvertIn;
struct P2R_UDTConvertIn
{
  CV_LeafParsed *tpi_leaf;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  CV_TypeId *itype_fwd_map;
  RDIM_Type **itype_type_ptrs;
};

//- rjf: symbol stream conversion

typedef struct P2R_SymbolStreamConvertIn P2R_SymbolStreamConvertIn;
struct P2R_SymbolStreamConvertIn
{
  B32 parsing_global_stream;
  RDI_Arch arch;
  COFF_SectionHeaderArray coff_sections;
  PDB_TpiHashParsed *tpi_hash;
  CV_LeafParsed *tpi_leaf;
  CV_LeafParsed *ipi_leaf;
  CV_SymParsed *sym;
  U64 sym_ranges_first;
  U64 sym_ranges_opl;
  CV_TypeId *itype_fwd_map;
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
  RDIM_SymbolChunkList constants;
  RDIM_ScopeChunkList scopes;
  RDIM_InlineSiteChunkList inline_sites;
  RDIM_TypeChunkList typedefs;
};

////////////////////////////////
//~ rjf: Top-Level State

typedef struct P2R_State P2R_State;
struct P2R_State
{
  Arena *arena;
  U64 work_thread_arenas_count;
  Arena **work_thread_arenas;
};

////////////////////////////////
//~ rjf: Globals

global P2R_State *p2r_state = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 p2r_end_of_cplusplus_container_name(String8 str);
internal U64 p2r_hash_from_voff(U64 voff);

////////////////////////////////
//~ rjf: Command Line -> Conversion Inputs

internal P2R_User2Convert *p2r_user2convert_from_cmdln(Arena *arena, CmdLine *cmdline);

////////////////////////////////
//~ rjf: COFF => RDI Canonical Conversions

internal RDI_BinarySectionFlags p2r_rdi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags);

////////////////////////////////
//~ rjf: CodeView => RDI Canonical Conversions

internal RDI_Arch     p2r_rdi_arch_from_cv_arch(CV_Arch arch);
internal RDI_RegCode  p2r_rdi_reg_code_from_cv_reg_code(RDI_Arch arch, CV_Reg reg_code);
internal RDI_Language p2r_rdi_language_from_cv_language(CV_Language language);
internal RDI_TypeKind p2r_rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type);

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
//~ rjf: Type Parsing/Conversion Tasks

ASYNC_WORK_DEF(p2r_itype_fwd_map_fill_work);
ASYNC_WORK_DEF(p2r_itype_chain_build_work);

////////////////////////////////
//~ rjf: UDT Conversion Tasks

ASYNC_WORK_DEF(p2r_udt_convert_work);

////////////////////////////////
//~ rjf: Symbol Stream Conversion Tasks

ASYNC_WORK_DEF(p2r_symbol_stream_convert_work);

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal P2R_Convert2Bake *p2r_convert(Arena *arena, P2R_User2Convert *in);

////////////////////////////////
//~ rjf: Top-Level Initialization

internal void p2r_init(void);

////////////////////////////////
//~ rjf: Top-Level Baking Entry Point

internal P2R_Bake2Serialize *p2r_bake(Arena *arena, P2R_Convert2Bake *in);

////////////////////////////////
//~ rjf: Top-Level Compression Entry Point

internal P2R_Serialize2File *p2r_compress(Arena *arena, P2R_Serialize2File *in);

////////////////////////////////

internal B32 p2r_has_symbol_ref(String8 msf_data, String8List symbol_list, MSF_RawStreamTable *st);
internal B32 p2r_has_file_ref(String8 msf_data, String8List file_list, MSF_RawStreamTable *st);
internal B32 p2r_has_symbol_or_file_ref(String8 msf_data, String8List symbol_list, String8List file_list);

#endif // RDI_FROM_PDB_H
