// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADCON_DWARF_H
#define RADCON_DWARF_H

typedef struct D2R_TypeTable
{
  HashTable          *ht;
  RDIM_TypeChunkList *types;
  U64                 type_chunk_cap;
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

internal RDIM_BakeParams * d2r_convert(Arena *arena, RDIM_LocalState *local_state, RC_Context *in);

////////////////////////////////

internal RDI_Language   rdi_language_from_dw_language(DW_Language v);
internal RDI_RegCodeX86 rdi_reg_from_dw_reg_x86(DW_RegX86 v);
internal B32            rdi_reg_from_dw_reg_x64(DW_RegX64 v, RDI_RegCodeX64 *code_out, U64 *off_out, U64 *size_out);
internal B32            rdi_reg_from_dw_reg(Arch arch, DW_Reg v, RDI_RegCode *code_out, U64 *off_out, U64 *size_out);

#endif // RADCON_DWARF_H

