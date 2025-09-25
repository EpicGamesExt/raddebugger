// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct D2R_ConvertParams D2R_ConvertParams;
struct D2R_ConvertParams
{
  String8 dbg_name;
  String8 dbg_data;
  String8 exe_name;
  String8 exe_data;
  ExecutableImageKind exe_kind;
  RDIM_SubsetFlags subset_flags;
  B32 deterministic;
};

typedef struct D2R_TypeTable
{
  HashTable           *ht;
  RDIM_TypeChunkList  *types;
  U64                  type_chunk_cap;
  RDIM_Type          **builtin_types;
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
  U64                    count;
  U64                   *info_off_arr;
  RDIM_Rng1U64ChunkList *voff_range_arr;
} D2R_CompUnitContribMap;

////////////////////////////////
//~ rjf: Enum Conversion Helpers

internal RDI_Language   d2r_rdi_language_from_dw_language(DW_Language v);
internal RDI_RegCodeX86 d2r_rdi_reg_code_from_dw_reg_x86(DW_RegX86 v);
internal RDI_RegCodeX64 d2r_rdi_reg_code_from_dw_reg_x64(DW_RegX64 v);
internal RDI_RegCode d2r_rdi_reg_code_from_dw_reg(Arch arch, DW_Reg v);

////////////////////////////////
//~ rjf: Type Conversion Helpers

internal RDIM_Type *d2r_create_type(Arena *arena, D2R_TypeTable *type_table);
internal RDIM_Type *d2r_find_or_create_type_from_offset(Arena *arena, D2R_TypeTable *type_table, U64 info_off);
internal RDIM_Type *d2r_type_from_attrib(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind);
internal Rng1U64List d2r_range_list_from_tag(Arena *arena, DW_Input *input, DW_CompUnit *cu, U64 image_base, DW_Tag tag);
internal RDIM_Type **d2r_collect_proc_params(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_TagNode *cur_node, U64 *param_count_out);
internal RDI_TypeKind d2r_unsigned_type_kind_from_size(U64 byte_size);
internal RDI_TypeKind d2r_signed_type_kind_from_size(U64 byte_size);
internal RDI_EvalTypeGroup d2r_type_group_from_type_kind(RDI_TypeKind x);

////////////////////////////////
//~ rjf: Bytecode Conversion Helpers

internal RDIM_EvalBytecode
d2r_bytecode_from_expression(Arena       *arena,
                             DW_Input    *input,
                             U64          image_base,
                             U64          address_size,
                             Arch         arch,
                             DW_ListUnit *addr_lu,
                             String8      expr,
                             DW_CompUnit *cu,
                             B32         *is_addr_out);
internal RDIM_Location *d2r_transpile_expression(Arena *arena, DW_Input *input, U64 image_base, U64 address_size, Arch arch, DW_ListUnit *addr_lu, DW_CompUnit *cu, String8 expr);
internal RDIM_Location *d2r_location_from_attrib(Arena *arena, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag, DW_AttribKind kind);
internal RDIM_LocationCaseList d2r_locset_from_attrib(Arena               *arena,
                                                      DW_Input            *input,
                                                      DW_CompUnit         *cu,
                                                      RDIM_ScopeChunkList *scopes,
                                                      RDIM_Scope          *curr_scope,
                                                      U64                  image_base,
                                                      Arch                 arch,
                                                      DW_Tag               tag,
                                                      DW_AttribKind        kind);
internal RDIM_LocationCaseList d2r_var_locset_from_tag(Arena               *arena,
                                                       DW_Input            *input,
                                                       DW_CompUnit         *cu,
                                                       RDIM_ScopeChunkList *scopes,
                                                       RDIM_Scope          *curr_scope,
                                                       U64                  image_base,
                                                       Arch                 arch,
                                                       DW_Tag               tag);

////////////////////////////////
//~ rjf: Compilation Unit / Scope Conversion Helpers

internal D2R_CompUnitContribMap d2r_cu_contrib_map_from_aranges(Arena *arena, DW_Input *input, U64 image_base);
internal RDIM_Rng1U64ChunkList d2r_voff_ranges_from_cu_info_off(D2R_CompUnitContribMap map, U64 info_off);

////////////////////////////////
//~ rjf: Main Conversion Entry Point

internal RDIM_BakeParams d2r_convert(Arena *arena, D2R_ConvertParams *params);
