// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_PDB_H
#define RDI_FROM_PDB_H

////////////////////////////////
//~ rjf: Conversion Stage Inputs

typedef struct P2R_ConvertParams P2R_ConvertParams;
struct P2R_ConvertParams
{
  String8 input_pdb_name;
  String8 input_pdb_data;
  String8 input_exe_name;
  String8 input_exe_data;
  RDIM_SubsetFlags subset_flags;
  B32 deterministic;
};

////////////////////////////////
//~ rjf: Conversion Helper Types

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

typedef struct P2R_SrcFileStub P2R_SrcFileStub;
struct P2R_SrcFileStub
{
  String8 file_path;
  CV_C13ChecksumKind checksum_kind;
  String8 checksum;
};

typedef struct P2R_SrcFileStubArray P2R_SrcFileStubArray;
struct P2R_SrcFileStubArray
{
  P2R_SrcFileStub *v;
  U64 count;
};

typedef struct P2R_SrcFileStubNode P2R_SrcFileStubNode;
struct P2R_SrcFileStubNode
{
  P2R_SrcFileStubNode *next;
  P2R_SrcFileStub v;
};

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

//- rjf: itype chains

typedef struct P2R_TypeIdChain P2R_TypeIdChain;
struct P2R_TypeIdChain
{
  P2R_TypeIdChain *next;
  CV_TypeId itype;
};

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 p2r_end_of_cplusplus_container_name(String8 str);
internal U64 p2r_hash_from_voff(U64 voff);

////////////////////////////////
//~ rjf: COFF => RDI Canonical Conversions

internal RDI_BinarySectionFlags p2r_rdi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags);

////////////////////////////////
//~ rjf: CodeView => RDI Canonical Conversions

internal RDI_Arch         p2r_rdi_arch_from_cv_arch(CV_Arch arch);
internal RDI_RegCode      p2r_rdi_reg_code_from_cv_reg_code(RDI_Arch arch, CV_Reg reg_code);
internal RDI_Language     p2r_rdi_language_from_cv_language(CV_Language language);
internal RDI_TypeKind     p2r_rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type);
internal RDI_ChecksumKind p2r_rdi_from_cv_c13_checksum_kind(CV_C13ChecksumKind k);

////////////////////////////////
//~ rjf: Location Info Building Helpers

internal RDI_RegCode p2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg);
internal RDIM_LocationInfo p2r_location_info_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection);
internal void p2r_local_push_location_cases_over_lvar_addr_range(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Local *local, RDIM_Location *loc, CV_LvarAddrRange *range, COFF_SectionHeader *section, CV_LvarAddrGap *gaps, U64 gap_count);

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal RDIM_BakeParams p2r_convert(Arena *arena, P2R_ConvertParams *params);

#endif // RDI_FROM_PDB_H
