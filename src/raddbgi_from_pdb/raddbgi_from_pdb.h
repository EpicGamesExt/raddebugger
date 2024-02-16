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
//~ rjf: Top-Level Conversion Entry Point

internal P2R_ConvertOut *p2r_convert(Arena *arena, P2R_ConvertIn *in);

#endif // RDI_FROM_PDB_H
