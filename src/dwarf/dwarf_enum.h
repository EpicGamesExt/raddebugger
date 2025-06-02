// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_ENUM_H
#define DWARF_ENUM_H

internal String8 dw_string_from_expr_op(Arena *arena, DW_Version ver, DW_Ext ext, DW_ExprOp op);
internal String8 dw_string_from_tag_kind(Arena *arena, DW_TagKind kind);
internal String8 dw_string_from_attrib_kind(Arena *arena, DW_Version ver, DW_Ext ext, DW_AttribKind kind);
internal String8 dw_string_from_form_kind(Arena *arena, DW_Version ver, DW_FormKind kind);
internal String8 dw_string_access_kind(Arena *arena, DW_AccessKind kind);

//internal String8 dw_string_from_register(Arena *arena, Arch arch, U64 reg_id);

#endif // DWARF_ENUM_H

