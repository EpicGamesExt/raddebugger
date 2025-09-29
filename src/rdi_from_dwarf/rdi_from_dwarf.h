// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct D2R_ConvertParams D2R_ConvertParams;
struct D2R_ConvertParams
{
  String8             dbg_name;
  String8             dbg_data;
  String8             exe_name;
  String8             exe_data;
  ExecutableImageKind exe_kind;
  RDIM_SubsetFlags    subset_flags;
  B32                 deterministic;
};

typedef struct D2R_TypeTable
{
  HashTable           *ht;
  RDIM_TypeChunkList  *types;
  U64                  type_chunk_cap;
  RDIM_Type          **builtin_types;
} D2R_TypeTable;

typedef struct D2R_TagFrame
{
  DW_TagNode *node;
  RDIM_Scope *scope;
  struct D2R_TagFrame *next;
} D2R_TagFrame;

typedef struct D2R_TagIterator
{
  D2R_TagFrame *free_list;
  D2R_TagFrame *stack;
  DW_TagNode   *tag_node;
  B32           visit_children;
} D2R_TagIterator;

typedef struct D2R_CompUnitContribMap
{
  U64                    count;
  U64                   *info_off_arr;
  RDIM_Rng1U64ChunkList *voff_range_arr;
} D2R_CompUnitContribMap;

////////////////////////////////
//~ rjf: Enum Conversion Helpers

internal RDI_Language   d2r_rdi_language_from_dw_language(DW_Language v);
internal RDI_RegCodeX86 d2r_rdi_reg_code_from_dw_reg_x86(DW_RegX86 v);
internal RDI_RegCodeX64 d2r_rdi_reg_code_from_dw_reg_x64(DW_RegX64 v);
internal RDI_RegCode    d2r_rdi_reg_code_from_dw_reg(Arch arch, DW_Reg v);

////////////////////////////////
//~ rjf: Type Conversion Helpers

internal RDIM_Type *       d2r_create_type(Arena *arena, D2R_TypeTable *type_table);
internal RDIM_Type *       d2r_create_type_from_offset(Arena *arena, D2R_TypeTable *type_table, U64 info_off);
internal RDIM_Type *       d2r_type_from_offset(D2R_TypeTable *type_table, U64 info_off);
internal RDIM_Type *       d2r_type_from_attrib(D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind);
internal Rng1U64List       d2r_range_list_from_tag(Arena *arena, DW_Input *input, DW_CompUnit *cu, U64 image_base, DW_Tag tag);
internal RDIM_Type **      d2r_collect_proc_params(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_TagNode *cur_node, U64 *param_count_out);
internal RDI_TypeKind      d2r_unsigned_type_kind_from_size(U64 byte_size);
internal RDI_TypeKind      d2r_signed_type_kind_from_size(U64 byte_size);
internal RDI_EvalTypeGroup d2r_type_group_from_type_kind(RDI_TypeKind x);

////////////////////////////////
//~ rjf: Bytecode Conversion Helpers

internal RDIM_EvalBytecode     d2r_bytecode_from_expression(Arena *arena, DW_Input *input, U64 image_base, U64 address_size, Arch arch, DW_ListUnit *addr_lu, String8 expr, DW_CompUnit *cu, B32 *is_addr_out);
internal RDIM_Location *       d2r_transpile_expression(Arena *arena, RDIM_LocationChunkList *locations, DW_Input *input, U64 image_base, U64 address_size, Arch arch, DW_ListUnit *addr_lu, DW_CompUnit *cu, String8 expr);
internal RDIM_Location *       d2r_location_from_attrib(Arena *arena, RDIM_LocationChunkList *locations, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag, DW_AttribKind kind);
internal RDIM_LocationCaseList d2r_locset_from_attrib(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Scope *curr_scope, RDIM_LocationChunkList *locations, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag, DW_AttribKind kind);
internal RDIM_LocationCaseList d2r_var_locset_from_tag(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Scope *curr_scope, RDIM_LocationChunkList *locations, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag);

////////////////////////////////
//~ rjf: Compilation Unit / Scope Conversion Helpers

internal D2R_CompUnitContribMap d2r_cu_contrib_map_from_aranges(Arena *arena, DW_Input *input, U64 image_base);
internal RDIM_Rng1U64ChunkList  d2r_voff_ranges_from_cu_info_off(D2R_CompUnitContribMap map, U64 info_off);

////////////////////////////////
//~ Tag Iterator

internal D2R_TagIterator * d2r_tag_iterator_init(Arena *arena, DW_TagNode *root);
internal void              d2r_tag_iterator_next(Arena *arena, D2R_TagIterator *iter);
internal void              d2r_tag_iterator_skip_children(D2R_TagIterator *iter);
internal DW_TagNode *      d2r_tag_iterator_parent_tag_node(D2R_TagIterator *iter);
internal DW_Tag            d2r_tag_iterator_parent_tag(D2R_TagIterator *iter);

////////////////////////////////
//~ Type/UDT/Symbol Conversion

internal void d2r_flag_converted_tag(DW_TagNode *tag_node);
internal B8   d2r_is_tag_converted(DW_TagNode *tag_node);

internal void d2r_convert_types(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, U64 arch_addr_size, DW_TagNode *root);
internal void d2r_convert_udts(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, U64 arch_addr_size, DW_TagNode *root);
internal void d2r_convert_symbols(Arena *arena, D2R_TypeTable *type_table, RDIM_Scope *global_scope, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, U64 arch_addr_size, U64 image_base, Arch arch, DW_TagNode *root);

////////////////////////////////
//~ rjf: Main Conversion Entry Point

internal RDIM_BakeParams d2r_convert(Arena *arena, D2R_ConvertParams *params);
