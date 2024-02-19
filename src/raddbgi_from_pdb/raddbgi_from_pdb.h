// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_PDB_H
#define RDI_FROM_PDB_H

////////////////////////////////
//~ rjf: Conversion Inputs/Outputs

typedef struct P2R_ConvertIn P2R_ConvertIn;
struct P2R_ConvertIn
{
  String8 input_pdb_name;
  String8 input_pdb_data;
  String8 input_exe_name;
  String8 input_exe_data;
  String8 output_name;
  
  struct
  {
    B8 input;
    B8 output;
    B8 parsing;
    B8 converting;
  } hide_errors;
  
  B8 dump;
  B8 dump__first;
  B8 dump_coff_sections;
  B8 dump_msf;
  B8 dump_sym;
  B8 dump_tpi_hash;
  B8 dump_leaf;
  B8 dump_c13;
  B8 dump_contributions;
  B8 dump_table_diagnostics;
  B8 dump__last;
  
  String8List errors;
};

typedef struct P2R_ConvertOut P2R_ConvertOut;
struct P2R_ConvertOut
{
  RDIM_TopLevelInfo top_level_info;
  RDIM_BinarySectionList binary_sections;
  RDIM_UnitChunkList units;
  RDIM_TypeChunkList types;
  RDIM_UDTChunkList udts;
  RDIM_SymbolChunkList global_variables;
  RDIM_SymbolChunkList thread_variables;
  RDIM_SymbolChunkList procedures;
  RDIM_ScopeChunkList scopes;
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

typedef struct P2R_TPIHashParseTask P2R_TPIHashParseTask;
struct P2R_TPIHashParseTask
{
  P2R_TPIHashParseIn in;
  Arena *out_arena;
  PDB_TpiHashParsed *out;
};

//- rjf: tpi leaves parsing

typedef struct P2R_TPILeafParseIn P2R_TPILeafParseIn;
struct P2R_TPILeafParseIn
{
  String8 leaf_data;
  CV_TypeId itype_first;
};

typedef struct P2R_TPILeafParseTask P2R_TPILeafParseTask;
struct P2R_TPILeafParseTask
{
  P2R_TPILeafParseIn in;
  Arena *out_arena;
  CV_LeafParsed *out;
};

//- rjf: exe hashing

typedef struct P2R_EXEHashIn P2R_EXEHashIn;
struct P2R_EXEHashIn
{
  String8 exe_data;
};

typedef struct P2R_EXEHashTask P2R_EXEHashTask;
struct P2R_EXEHashTask
{
  P2R_EXEHashIn in;
  U64 out;
};

////////////////////////////////
//~ rjf: Conversion Data Structure Types

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

typedef struct P2R_ITypeFwdMapFillTask P2R_ITypeFwdMapFillTask;
struct P2R_ITypeFwdMapFillTask
{
  P2R_ITypeFwdMapFillIn fill_in;
};

typedef struct P2R_ITypeFwdMapFillTaskBatch P2R_ITypeFwdMapFillTaskBatch;
struct P2R_ITypeFwdMapFillTaskBatch
{
  P2R_ITypeFwdMapFillTask *tasks;
  U64 tasks_count;
  U64 *num_tasks_taken_ptr;
};

//- rjf: per-unit symbol conversion

typedef struct P2R_UnitSymbolConvertIn P2R_UnitSymbolConvertIn;
struct P2R_UnitSymbolConvertIn
{
  RDI_Arch arch;
  PDB_CoffSectionArray *coff_sections;
  PDB_TpiHashParsed *tpi_hash;
  CV_LeafParsed *tpi_leaf;
  CV_SymParsed *sym;
  CV_TypeId *itype_fwd_map;
  RDIM_Type **itype_type_ptrs;
  P2R_LinkNameMap *link_name_map;
};

typedef struct P2R_UnitSymbolConvertOut P2R_UnitSymbolConvertOut;
struct P2R_UnitSymbolConvertOut
{
  RDIM_SymbolChunkList procedures;
  RDIM_SymbolChunkList global_variables;
  RDIM_SymbolChunkList thread_variables;
  RDIM_ScopeChunkList scopes;
};

typedef struct P2R_UnitSymbolTask P2R_UnitSymbolTask;
struct P2R_UnitSymbolTask
{
  // rjf: inputs
  P2R_UnitSymbolConvertIn convert_in;
  
  // rjf: outputs
  Arena *out_arena;
  P2R_UnitSymbolConvertOut *convert_out;
};

typedef struct P2R_UnitSymbolTaskBatch P2R_UnitSymbolTaskBatch;
struct P2R_UnitSymbolTaskBatch
{
  P2R_UnitSymbolTask *tasks;
  U64 tasks_count;
  U64 *num_tasks_taken_ptr;
};

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 p2r_end_of_cplusplus_container_name(String8 str);
internal U64 p2r_hash_from_voff(U64 voff);

////////////////////////////////
//~ rjf: Command Line -> Conversion Inputs

internal P2R_ConvertIn *p2r_convert_in_from_cmd_line(Arena *arena, CmdLine *cmdline);

////////////////////////////////
//~ rjf: COFF => RADDBGI Canonical Conversions

internal RDI_BinarySectionFlags rdi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags);

////////////////////////////////
//~ rjf: CodeView => RADDBGI Canonical Conversions

internal RDI_Arch         rdi_arch_from_cv_arch(CV_Arch arch);
internal RDI_RegisterCode rdi_reg_code_from_cv_reg_code(RDI_Arch arch, CV_Reg reg_code);
internal RDI_Language     rdi_language_from_cv_language(CV_Language language);
internal RDI_TypeKind     rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type);

////////////////////////////////
//~ rjf: Location Info Building Helpers

internal RDIM_Location *p2r_location_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegisterCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection);
internal CV_EncodedFramePtrReg p2r_cv_encoded_fp_reg_from_frameproc(CV_SymFrameproc *frameproc, B32 param_base);
internal RDI_RegisterCode p2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg);
internal void p2r_location_over_lvar_addr_range(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_LocationSet *locset, RDIM_Location *location, CV_LvarAddrRange *range, COFF_SectionHeader *section, CV_LvarAddrGap *gaps, U64 gap_count);

////////////////////////////////
//~ rjf: Initial Parsing & Preparation Pass Threads

internal void p2r_tpi_hash_parse_thread__entry_point(void *p);
internal void p2r_tpi_leaf_parse_thread__entry_point(void *p);
internal void p2r_exe_hash_thread__entry_point(void *p);

////////////////////////////////
//~ rjf: Type Forward Resolution Map Build / Thread

internal void p2r_itype_fwd_map_fill(P2R_ITypeFwdMapFillIn *in);
internal void p2r_itype_fwd_map_fill_task_thread__entry_point(void *p);

////////////////////////////////
//~ rjf: Per-Unit Symbol Conversion / Thread

internal P2R_UnitSymbolConvertOut *p2r_unit_symbol_convert(Arena *arena, P2R_UnitSymbolConvertIn *in);
internal void p2r_unit_symbol_convert_task_thread__entry_point(void *p);

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal P2R_ConvertOut *p2r_convert(Arena *arena, P2R_ConvertIn *in);

#endif // RDI_FROM_PDB_H
