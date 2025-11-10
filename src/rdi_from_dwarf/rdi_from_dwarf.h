// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_DWARF_H
#define RDI_FROM_DWARF_H

////////////////////////////////
//~ rjf: Conversion Stage Inputs

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

////////////////////////////////
//~ rjf: Conversion Helper Types

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

#define D2R_ValueType_IsSigned(x)   ((x) == D2R_ValueType_S8 || (x) == D2R_ValueType_S16 || (x) == D2R_ValueType_S32 || (x) == D2R_ValueType_S64 || (x) == D2R_ValueType_S128 || (x) == D2R_ValueType_S256 || (x) == D2R_ValueType_S512)
#define D2R_ValueType_IsUnsigned(x) ((x) == D2R_ValueType_U8 || (x) == D2R_ValueType_U16 || (x) == D2R_ValueType_U32 || (x) == D2R_ValueType_U64 || (x) == D2R_ValueType_U128 || (x) == D2R_ValueType_U256 || (x) == D2R_ValueType_U512)
#define D2R_ValueType_IsFloat(x)    ((x) == D2R_ValueType_F32 || (x) == D2R_ValueType_F64)
#define D2R_ValueType_IsInt(x)      (D2R_ValueType_IsSigned(x) || D2R_ValueType_IsUnsigned(x) || (x) == D2R_ValueType_Address)
typedef enum D2R_ValueType
{
  D2R_ValueType_Generic,
  D2R_ValueType_U8,
  D2R_ValueType_U16,
  D2R_ValueType_U32,
  D2R_ValueType_U64,
  D2R_ValueType_U128,
  D2R_ValueType_U256,
  D2R_ValueType_U512,
  D2R_ValueType_S8,
  D2R_ValueType_S16,
  D2R_ValueType_S32,
  D2R_ValueType_S64,
  D2R_ValueType_S128,
  D2R_ValueType_S256,
  D2R_ValueType_S512,
  D2R_ValueType_F32,
  D2R_ValueType_F64,
  D2R_ValueType_Address,
  D2R_ValueType_ImplicitValue,
  D2R_ValueType_Bool = D2R_ValueType_S8,
} D2R_ValueType;

typedef struct D2R_ValueTypeNode
{
  D2R_ValueType type;
  
  struct D2R_ValueTypeNode *next;
} D2R_ValueTypeNode;

typedef struct D2R_ValueTypeStack
{
  U64                count;
  D2R_ValueTypeNode *top;
  D2R_ValueTypeNode *free_list;
} D2R_ValueTypeStack;

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

////////////////////////////////
//~ RDIM Bytecode Helpers

internal B32 rdim_is_eval_bytecode_static(RDIM_EvalBytecode bc);
internal U64 rdim_virt_off_from_eval_bytecode(RDIM_EvalBytecode bc, U64 image_base);

////////////////////////////////
//~ rjf: Bytecode Conversion Helpers

internal D2R_ValueTypeNode * d2r_value_type_stack_push(Arena *arena, D2R_ValueTypeStack *stack, D2R_ValueType type);
internal D2R_ValueType       d2r_value_type_stack_pop(D2R_ValueTypeStack *stack);
internal D2R_ValueType       d2r_value_type_stack_peek(D2R_ValueTypeStack *stack);

internal D2R_ValueType     d2r_unsigned_value_type_from_bit_size(U64 bit_size);
internal D2R_ValueType     d2r_signed_value_type_from_bit_size(U64 bit_size);
internal D2R_ValueType     d2r_float_type_from_bit_size(U64 bit_size);
internal RDI_EvalTypeGroup d2r_value_type_to_rdi(D2R_ValueType v);
internal U64               d2r_size_from_value_type(U64 addr_size, D2R_ValueType value_type);
internal D2R_ValueType     d2r_pick_common_value_type(D2R_ValueType lhs, D2R_ValueType rhs);

internal D2R_ValueType d2r_apply_usual_arithmetic_conversions(Arena *arena, D2R_ValueType lhs, D2R_ValueType rhs, RDIM_EvalBytecode *bc);
internal void          d2r_push_arithmetic_op(Arena *arena, D2R_ValueTypeStack *stack, RDIM_EvalBytecode *bc, RDI_EvalOp op);
internal void          d2r_push_relational_op(Arena *arena, D2R_ValueTypeStack *stack, RDIM_EvalBytecode *bc, RDI_EvalOp op);

internal RDIM_EvalBytecode     d2r_bytecode_from_expression(Arena *arena, DW_Input *input, U64 image_base, U64 address_size, Arch arch, DW_ListUnit *addr_lu, String8 expr, DW_CompUnit *cu, D2R_ValueType *result_type_out);
internal RDIM_Location *       d2r_transpile_expression(Arena *arena, RDIM_LocationChunkList *locations, DW_Input *input, U64 image_base, U64 address_size, Arch arch, DW_ListUnit *addr_lu, DW_CompUnit *cu, String8 expr);
internal RDIM_LocationCaseList d2r_locset_from_attrib(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Scope *curr_scope, RDIM_LocationChunkList *locations, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag, DW_AttribKind kind);
internal RDIM_LocationCaseList d2r_var_locset_from_tag(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Scope *curr_scope, RDIM_LocationChunkList *locations, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag);

////////////////////////////////
//~ rjf: Compilation Unit / Scope Conversion Helpers

internal RDIM_Rng1U64ChunkList d2r_voff_ranges_from_cu_info_off(D2R_CompUnitContribMap map, U64 info_off);
internal RDIM_Scope *d2r_push_scope(Arena *arena, RDIM_ScopeChunkList *scopes, U64 scope_chunk_cap, D2R_TagFrame *tag_stack, Rng1U64List ranges);

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

internal RDIM_Type *d2r_find_or_convert_type(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, U64 arch_addr_size, DW_Tag tag, DW_AttribKind kind);

internal void d2r_convert_types(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, U64 arch_addr_size, DW_TagNode *root);
internal void d2r_convert_udts(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, U64 arch_addr_size, DW_TagNode *root);
internal void d2r_convert_symbols(Arena *arena, D2R_TypeTable *type_table, RDIM_Scope *global_scope, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, U64 arch_addr_size, U64 image_base, Arch arch, DW_TagNode *root);

////////////////////////////////
//~ rjf: Main Conversion Entry Point

internal RDIM_BakeParams d2r_convert(Arena *arena, D2R_ConvertParams *params);

#endif // RDI_FROM_DWARF_H
