// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef CODEVIEW_STRINGIZE_H
#define CODEVIEW_STRINGIZE_H

////////////////////////////////
//~ CodeView Stringize Helper Types

typedef struct CV_StringizeSymParams{
  CV_Arch arch;
} CV_StringizeSymParams;

typedef struct CV_StringizeLeafParams{
  U32 dummy;
} CV_StringizeLeafParams;

////////////////////////////////
//~ CodeView Common Stringize Functions

internal void cv_stringize_numeric(Arena *arena, String8List *out, CV_NumericParsed *num);

internal void cv_stringize_lvar_addr_range(Arena *arena, String8List *out,
                                           CV_LvarAddrRange *range);
internal void cv_stringize_lvar_addr_gap(Arena *arena, String8List *out, CV_LvarAddrGap *gap);
internal void cv_stringize_lvar_addr_gap_list(Arena *arena, String8List *out,
                                              void *first, void *opl);

internal String8 cv_string_from_basic_type(CV_BasicType basic_type);
internal String8 cv_string_from_c13_sub_section_kind(CV_C13_SubSectionKind kind);
internal String8 cv_string_from_reg(CV_Arch arch, CV_Reg reg);
internal String8 cv_string_from_pointer_kind(CV_PointerKind ptr_kind);
internal String8 cv_string_from_pointer_mode(CV_PointerMode ptr_mode);
internal String8 cv_string_from_hfa_kind(CV_HFAKind hfa_kind);
internal String8 cv_string_from_mo_com_udt_kind(CV_MoComUDTKind mo_com_udt_kind);

////////////////////////////////
//~ CodeView Flags Stringize Functions

internal void cv_stringize_modifier_flags(Arena *arena, String8List *out,
                                          U32 indent, CV_ModifierFlags flags);

internal void cv_stringize_type_props(Arena *arena, String8List *out,
                                      U32 indent, CV_TypeProps props);

internal void cv_stringize_pointer_attribs(Arena *arena, String8List *out,
                                           U32 indent, CV_PointerAttribs attribs);

internal void cv_stringize_local_flags(Arena *arena, String8List *out,
                                       U32 indent, CV_LocalFlags flags);

////////////////////////////////
//~ CodeView Sym Stringize Functions

internal void cv_stringize_sym_parsed(Arena *arena, String8List *out, CV_SymParsed *sym);

internal void cv_stringize_sym_range(Arena *arena, String8List *out,
                                     CV_RecRange *range, String8 data,
                                     CV_StringizeSymParams *p);
internal void cv_stringize_sym_array(Arena *arena, String8List *out,
                                     CV_RecRangeArray *ranges, String8 data,
                                     CV_StringizeSymParams *p);

////////////////////////////////
//~ CodeView Leaf Stringize Functions

internal void cv_stringize_leaf_parsed(Arena *arena, String8List *out, CV_LeafParsed *leaf);

internal void cv_stringize_leaf_range(Arena *arena, String8List *out,
                                      CV_RecRange *range, CV_TypeId itype, String8 data,
                                      CV_StringizeLeafParams *p);
internal void cv_stringize_leaf_array(Arena *arena, String8List *out,
                                      CV_RecRangeArray *ranges, CV_TypeId itype_first,
                                      String8 data,
                                      CV_StringizeLeafParams *p);

////////////////////////////////
//~ CodeView C13 Stringize Functions

internal void cv_stringize_c13_parsed(Arena *arena, String8List *out, CV_C13Parsed *c13);

#endif // CODEVIEW_STRINGIZE_H
