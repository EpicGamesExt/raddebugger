// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_CODEVIEW_STRINGIZE_H
#define RADDBG_CODEVIEW_STRINGIZE_H

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

static void cv_stringize_numeric(Arena *arena, String8List *out, CV_NumericParsed *num);

static void cv_stringize_lvar_addr_range(Arena *arena, String8List *out,
                                         CV_LvarAddrRange *range);
static void cv_stringize_lvar_addr_gap(Arena *arena, String8List *out, CV_LvarAddrGap *gap);
static void cv_stringize_lvar_addr_gap_list(Arena *arena, String8List *out,
                                            void *first, void *opl);

static String8 cv_string_from_sym_kind(CV_SymKind kind);
static String8 cv_string_from_basic_type(CV_BasicType basic_type);
static String8 cv_string_from_leaf_kind(CV_LeafKind kind);
static String8 cv_string_from_numeric_kind(CV_NumericKind kind);
static String8 cv_string_from_c13_sub_section_kind(CV_C13_SubSectionKind kind);
static String8 cv_string_from_machine(CV_Arch arch);
static String8 cv_string_from_reg(CV_Arch arch, CV_Reg reg);
static String8 cv_string_from_pointer_kind(CV_PointerKind ptr_kind);
static String8 cv_string_from_pointer_mode(CV_PointerMode ptr_mode);
static String8 cv_string_from_hfa_kind(CV_HFAKind hfa_kind);
static String8 cv_string_from_mo_com_udt_kind(CV_MoComUDTKind mo_com_udt_kind);

////////////////////////////////
//~ CodeView Flags Stringize Functions

static void cv_stringize_modifier_flags(Arena *arena, String8List *out,
                                        U32 indent, CV_ModifierFlags flags);

static void cv_stringize_type_props(Arena *arena, String8List *out,
                                    U32 indent, CV_TypeProps props);

static void cv_stringize_pointer_attribs(Arena *arena, String8List *out,
                                         U32 indent, CV_PointerAttribs attribs);

static void cv_stringize_local_flags(Arena *arena, String8List *out,
                                     U32 indent, CV_LocalFlags flags);

////////////////////////////////
//~ CodeView Sym Stringize Functions

static void cv_stringize_sym_parsed(Arena *arena, String8List *out, CV_SymParsed *sym);

static void cv_stringize_sym_range(Arena *arena, String8List *out,
                                   CV_RecRange *range, String8 data,
                                   CV_StringizeSymParams *p);
static void cv_stringize_sym_array(Arena *arena, String8List *out,
                                   CV_RecRangeArray *ranges, String8 data,
                                   CV_StringizeSymParams *p);

////////////////////////////////
//~ CodeView Leaf Stringize Functions

static void cv_stringize_leaf_parsed(Arena *arena, String8List *out, CV_LeafParsed *leaf);

static void cv_stringize_leaf_range(Arena *arena, String8List *out,
                                    CV_RecRange *range, CV_TypeId itype, String8 data,
                                    CV_StringizeLeafParams *p);
static void cv_stringize_leaf_array(Arena *arena, String8List *out,
                                    CV_RecRangeArray *ranges, CV_TypeId itype_first,
                                    String8 data,
                                    CV_StringizeLeafParams *p);

////////////////////////////////
//~ CodeView C13 Stringize Functions

static void cv_stringize_c13_parsed(Arena *arena, String8List *out, CV_C13Parsed *c13);

#endif //RADDBG_CODEVIEW_STRINGIZE_H
