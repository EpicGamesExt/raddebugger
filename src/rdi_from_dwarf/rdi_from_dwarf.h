// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef U64 D2R_ConvertFlags;
enum
{
#define X(t,n,k) D2R_ConvertFlag_##t = (1ull << RDI_SectionKind_##t),
  RDI_SectionKind_XList
#undef X
  D2R_ConvertFlag_StrictParse,
};

typedef struct D2R_User2Convert
{
  String8          input_exe_name;
  String8          input_exe_data;
  String8          input_debug_name;
  String8          input_debug_data;
  String8          output_name;
  D2R_ConvertFlags flags;
  String8List      errors;
} D2R_User2Convert;

typedef struct D2R_TypeTable
{
  HashTable          *ht;
  RDIM_TypeChunkList *types;
  U64                 type_chunk_cap;
  RDIM_Type          *void_type;
  RDIM_Type          *varg_type;
} D2R_TypeTable;

typedef struct D2R_TagNode
{
  struct D2R_TagNode *next;
  DW_TagNode         *cur_node;
  RDIM_Type          *type;
  RDIM_Scope         *scope;
} D2R_TagNode;

typedef struct D2R_CompUnitContribMap
{
  U64               count;
  U64              *info_off_arr;
  RDIM_Rng1U64List *voff_range_arr;
} D2R_CompUnitContribMap;

////////////////////////////////
// Command Line -> Conversion Inputs

internal D2R_User2Convert * d2r_user2convert_from_cmdln(Arena *arena, CmdLine *cmdline);

////////////////////////////////
// Top-Level Conversion Entry Point

internal RDIM_BakeParams *            d2r_convert (Arena          *arena, D2R_User2Convert            *in);
internal RDIM_BakeResults             d2r_bake    (RDIM_LocalState *state, RDIM_BakeParams             *in);
internal RDIM_SerializedSectionBundle d2r_compress(Arena          *arena, RDIM_SerializedSectionBundle in);

////////////////////////////////
// Enum Conversion

internal RDI_Language   rdi_language_from_dw_language(DW_Language v);
internal RDI_RegCodeX86 rdi_reg_from_dw_reg_x86(DW_RegX86 v);
internal B32            rdi_reg_from_dw_reg_x64(DW_RegX64 v, RDI_RegCodeX64 *code_out, U64 *off_out, U64 *size_out);
internal B32            rdi_reg_from_dw_reg(Arch arch, DW_Reg v, RDI_RegCode *code_out, U64 *off_out, U64 *size_out);

