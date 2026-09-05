// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_H
#define DWARF_H

////////////////////////////////
//~ rjf: Includes

#include "dwarf/generated/dwarf.meta.h"
#if defined(X64_H)
# include "dwarf/x64/dwarf_x64.h"
#endif
#if defined(ARM64_H)
# include "dwarf/arm64/dwarf_arm64.h"
#endif

////////////////////////////////
//~ rjf: Helper Types

typedef struct DW_ExprOpInfo DW_ExprOpInfo;
struct DW_ExprOpInfo
{
  U8 push_count;
  U8 pop_count;
  U8 operand_count;
  B8 valid_for_cfa_exprs;
  DW_ExprOperandKind operand_kinds[2];
  String8 name;
  DW_Version version;
};
global read_only DW_ExprOpInfo dw_expr_op_info_nil = {0};

////////////////////////////////
//~ rjf: X-List Helpers

//- rjf: format address sizes
internal U64 dw_addr_size_from_format(DW_Format fmt);

//- rjf: attribute classes
internal DW_AttribClass dw_attrib_class_from_kind(DW_Version version, DW_ExtFlags exts, DW_AttribKind k);
internal DW_AttribClass dw_attrib_class_from_form_kind(DW_Version version, DW_ExtFlags exts, DW_FormKind k);

//- rjf: expr op metadata
internal DW_ExprOpInfo *dw_info_from_expr_op(DW_Version version, DW_ExtFlags exts, DW_ExprOp op);

//- rjf: enum -> string
internal String8 dw_string_from_format(DW_Format fmt);
internal String8 dw_string_from_tag_kind(DW_Version version, DW_ExtFlags exts, DW_TagKind k);
internal String8 dw_string_from_attrib_kind(DW_Version version, DW_ExtFlags exts, DW_AttribKind k);
internal String8 dw_string_from_form_kind(DW_Version version, DW_ExtFlags exts, DW_FormKind k);
internal String8 dw_string_from_language(DW_Language k);
internal String8 dw_string_from_comp_unit_kind(DW_CompUnitKind k);
internal String8 dw_string_from_inl_kind(DW_InlKind k);
internal String8 dw_string_from_access_kind(DW_AccessKind k);
internal String8 dw_string_from_calling_convention_kind(DW_CallingConventionKind k);
internal String8 dw_string_from_ate(DW_ATE k);
internal String8 dw_string_from_visibility_kind(DW_VisibilityKind k);
internal String8 dw_string_from_std_opcode(DW_StdOpcode opcode);
internal String8 dw_string_from_ext_opcode(DW_ExtOpcode opcode);
internal String8 dw_string_from_lle(DW_LLE lle);
internal String8 dw_string_from_rle(DW_RLE rle);
internal String8 dw_regular_name_from_section_kind(DW_SectionKind k);
internal String8 dw_macho_name_from_section_kind(DW_SectionKind k);
internal String8 dw_dwo_name_from_section_kind(DW_SectionKind k);
#define dw_string_from_expr_op(version, exts, op) (dw_info_from_expr_op((version), (exts), (op))->name)

//- rjf: architecture info
internal U64 dw_reg_count_from_arch(Arch arch);

#endif // DWARF_H
