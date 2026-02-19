// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_DWARF_H
#define RDI_FROM_DWARF_H

////////////////////////////////
//~ Conversion

typedef struct D2R_ConvertParams
{
  String8             dbg_name;
  String8             dbg_data;
  String8             exe_name;
  String8             exe_data;
  ExecutableImageKind exe_kind;
  RDIM_SubsetFlags    subset_flags;
  B32                 deterministic;
  B32                 is_parse_relaxed;
} D2R_ConvertParams;

#define D2R_UNIT_CHUNK_CAP        512
#define D2R_UDT_CHUNK_CAP         1024
#define D2R_TYPE_CHUNK_CAP        1024
#define D2R_SRC_FILE_CAP          1024
#define D2R_LINE_TABLE_CAP        1024
#define D2R_GVAR_CHUNK_CAP        1024
#define D2R_TVAR_CHUNK_CAP        1024
#define D2R_LOCATIONS_CAP         4096
#define D2R_PROC_CHUNK_CAP        4096
#define D2R_SCOPE_CHUNK_CAP       4096
#define D2R_INLINE_SITE_CHUNK_CAP 4096

typedef struct D2R_Shared
{
  RDIM_TopLevelInfo        top_level_info;
  RDIM_BinarySectionList   binary_sections;
  RDIM_UnitChunkList       units;
  RDIM_UDTChunkList        udts;
  RDIM_TypeChunkList       types;
  RDIM_SrcFileChunkList    src_files;
  RDIM_LineTableChunkList  line_tables;
  RDIM_LocationChunkList   locations;
  RDIM_SymbolChunkList     gvars;
  RDIM_SymbolChunkList     tvars;
  RDIM_SymbolChunkList     procs;
  RDIM_ScopeChunkList      scopes;
  RDIM_InlineSiteChunkList inline_sites;
} D2R_Shared;

////////////////////////////////
//~ Value Types

typedef enum
{
  D2R_ArithmeticType_Null,
  D2R_ArithmeticType_Signed,
  D2R_ArithmeticType_Unsigned,
  D2R_ArithmeticType_Float,
} D2R_ArithmeticType;

#define D2R_ValueType_Signed_XList \
  X(S8,   Signed,   1 )            \
  X(S16,  Signed,   2 )            \
  X(S32,  Signed,   4 )            \
  X(S64,  Signed,   8 )            \
  X(S128, Signed,   16)            \
  X(S256, Signed,   32)            \
  X(S512, Signed,   64)

#define D2R_ValueType_Unsigned_XList \
  X(U8,   Unsigned, 1 )              \
  X(U16,  Unsigned, 2 )              \
  X(U32,  Unsigned, 4 )              \
  X(U64,  Unsigned, 8 )              \
  X(U128, Unsigned, 16)              \
  X(U256, Unsigned, 32)              \
  X(U512, Unsigned, 64)

#define D2R_ValueType_Float_XList \
  X(F16,  Float,    2 )           \
  X(F32,  Float,    4 )           \
  X(F48,  Float,    6 )           \
  X(F64,  Float,    8 )           \
  X(F80,  Float,    10)           \
  X(F96,  Float,    12)           \
  X(F128, Float,    16)

#define D2R_ValueType_XList      \
  X(Generic,       Null,     0 ) \
  X(ImplicitValue, Null,     0 ) \
  X(Address,       Unsigned, 0 ) \
  D2R_ValueType_Signed_XList     \
  D2R_ValueType_Unsigned_XList   \
  D2R_ValueType_Float_XList

typedef enum
{
#define X(t, ...) D2R_ValueType_##t,
  D2R_ValueType_XList
#undef X
  D2R_ValueType_Bool = D2R_ValueType_S8,
} D2R_ValueType;

typedef struct D2R_ValueTypeNode
{
  D2R_ValueType             type;
  struct D2R_ValueTypeNode *next;
} D2R_ValueTypeNode;

typedef struct D2R_ValueTypeStack
{
  U64                count;
  D2R_ValueTypeNode *top;
  D2R_ValueTypeNode *free_list;
} D2R_ValueTypeStack;

////////////////////////////////
//~ Type Table

typedef struct D2R_TypeTable
{
  HashTable           *ht;
  RDIM_TypeChunkList  *types;
  U64                  type_chunk_cap;
  RDIM_Type          **builtin_types;
} D2R_TypeTable;

////////////////////////////////
//~ Tag Iterator

typedef DW_TagSpare D2R_TagFlags;
enum
{
  D2R_TagFlags_TypeConverted = (1 << 0),
  D2R_TagFlags_UdtConverted  = (1 << 1),
};

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

////////////////////////////////
//~ Line Table Conversion

typedef struct D2R_SrcFileLookup
{
  DW_LineFile  *file;
  DW_LineVM    *vm;
  RDIM_SrcFile *src_file;
} D2R_SrcFileLookup;

////////////////////////////////
//~ Contrib Map

typedef struct D2R_CompUnitContribMap
{
  U64                    count;
  U64                   *info_off_arr;
  RDIM_Rng1U64ChunkList *voff_range_arr;
} D2R_CompUnitContribMap;

////////////////////////////////
//~ RDIM Bytecode Extensions

internal B32 rdim_is_eval_bytecode_static     (RDIM_EvalBytecode bc);
internal B32 rdim_static_eval_bytecode_to_voff(RDIM_EvalBytecode bc, U64 image_base, U64 *voff_out);

////////////////////////////////
//~ DWARF -> RDI Enums

internal RDI_Language   d2r_rdi_language_from_dw_language(DW_Language v);
internal RDI_RegCodeX64 d2r_rdi_reg_code_from_dw_reg_x64 (DW_RegX64 v);
internal RDI_RegCode    d2r_rdi_reg_code_from_dw_reg     (Arch arch, DW_Reg v);

////////////////////////////////
//~ Value Type

internal D2R_ValueTypeNode * d2r_value_type_stack_push(Arena *arena, D2R_ValueTypeStack *stack, D2R_ValueType type);
internal D2R_ValueType       d2r_value_type_stack_pop (D2R_ValueTypeStack *stack);
internal D2R_ValueType       d2r_value_type_stack_peek(D2R_ValueTypeStack *stack);

internal D2R_ValueType d2r_signed_value_type_from_bit_size  (U64 bit_size);
internal D2R_ValueType d2r_unsigned_value_type_from_bit_size(U64 bit_size);
internal D2R_ValueType d2r_float_type_from_bit_size         (U64 bit_size);

internal U64 d2r_size_from_value_type(U64 addr_size, D2R_ValueType value_type);
internal D2R_ArithmeticType d2r_arithmetic_type_from_value_type(D2R_ValueType v);
internal D2R_ArithmeticType d2r_arithmetic_type_from_value_type(D2R_ValueType v);
internal B32 d2r_is_value_type_signed  (D2R_ValueType v);
internal B32 d2r_is_value_type_integral(D2R_ValueType v);
internal B32 d2r_is_value_type_unsigned(D2R_ValueType v);
internal B32 d2r_is_value_type_float   (D2R_ValueType v);

internal D2R_ValueType     d2r_pick_common_value_type(D2R_ValueType lhs, D2R_ValueType rhs);
internal RDI_EvalTypeGroup d2r_value_type_to_rdi(D2R_ValueType v);

internal D2R_ValueType d2r_apply_usual_arithmetic_conversions(Arena *arena, D2R_ValueType lhs, D2R_ValueType rhs, RDIM_EvalBytecode *bc);
internal void          d2r_push_arithmetic_op(Arena *arena, D2R_ValueTypeStack *stack, RDIM_EvalBytecode *bc, RDI_EvalOp op);
internal void          d2r_push_relational_op(Arena *arena, D2R_ValueTypeStack *stack, RDIM_EvalBytecode *bc, RDI_EvalOp op);

////////////////////////////////
//~ Expression Conversion

internal RDIM_EvalBytecode     d2r_bytecode_from_expression(Arena *arena, DW_Input *input, U64 image_base, Arch arch, DW_ListUnit *addr_lu, String8 expr, DW_CompUnit *cu, D2R_ValueType *result_type_out);
internal RDIM_Location *       d2r_transpile_expression    (Arena *arena, RDIM_LocationChunkList *locations, DW_Input *input, U64 image_base, Arch arch, DW_ListUnit *addr_lu, DW_CompUnit *cu, String8 expr);
internal RDIM_Location *       d2r_location_from_attrib    (Arena *arena, RDIM_LocationChunkList *locations, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag, DW_AttribKind kind);
internal RDIM_LocationCaseList d2r_locset_from_attrib      (Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Scope *curr_scope, RDIM_LocationChunkList *locations, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag, DW_AttribKind kind);
internal RDIM_LocationCaseList d2r_var_locset_from_tag     (Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Scope *curr_scope, RDIM_LocationChunkList *locations, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag);

////////////////////////////////
//~ Type Table

internal RDIM_Type *  d2r_create_type            (Arena *arena, D2R_TypeTable *type_table);
internal RDIM_Type *  d2r_create_type_from_offset(Arena *arena, D2R_TypeTable *type_table, U64 info_off);
internal RDIM_Type *  d2r_type_from_offset(D2R_TypeTable *type_table, U64 info_off);
internal RDIM_Type *  d2r_type_from_attrib(D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind);

////////////////////////////////
//~ Tag Iterator

internal D2R_TagIterator * d2r_tag_iterator_init(Arena *arena, DW_TagNode *root);
internal void              d2r_tag_iterator_next(Arena *arena, D2R_TagIterator *iter);
internal void              d2r_tag_iterator_skip_children  (D2R_TagIterator *iter);
internal DW_TagNode *      d2r_tag_iterator_parent_tag_node(D2R_TagIterator *iter);
internal DW_Tag            d2r_tag_iterator_parent_tag     (D2R_TagIterator *iter);

////////////////////////////////
//~ Type Conversion

internal void        d2r_flag_converted_tag(DW_TagNode *tag_node);
internal B8          d2r_is_tag_converted  (DW_TagNode *tag_node);
internal RDIM_Type * d2r_find_or_convert_type(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, Arch arch, DW_Tag tag, DW_AttribKind kind);
internal void        d2r_convert_types(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, Arch arch, DW_TagNode *root);

////////////////////////////////
//~ UDT Conversion

internal B8   d2r_is_udt_tag_converted  (DW_TagNode *tag_node);
internal void d2r_flag_converted_udt_tag(DW_TagNode *tag_node);
internal void d2r_inline_anonymous_udt_member(Arena *arena, RDIM_UDT *top_udt, U64 base_off, RDIM_UDT *udt);
internal void d2r_convert_udts(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, DW_TagNode *root);

////////////////////////////////
//~ Symbol Conversion

internal RDIM_Scope * d2r_push_scope         (Arena *arena, RDIM_ScopeChunkList *scopes, U64 scope_chunk_cap, D2R_TagFrame *tag_stack, Rng1U64List ranges);
internal RDIM_Type ** d2r_collect_proc_params(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_TagNode *cur_node, U64 *param_count_out);
internal Rng1U64List  d2r_range_list_from_tag(Arena *arena, DW_Input *input, DW_CompUnit *cu, U64 image_base, DW_Tag tag);
internal void         d2r_convert_symbols(Arena *arena, D2R_TypeTable *type_table, RDIM_Scope *global_scope, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, U64 image_base, Arch arch, DW_TagNode *root);

////////////////////////////////
//~ Line Table Conversion

internal U64  d2r_hash_line_file(String8 dir_path, DW_LineFile *file);
internal void d2r_sort_ptrs(void **ptrs, U64 count, int (* is_before)(void *a, void *b));

////////////////////////////////
//~ Contrib Map

internal D2R_CompUnitContribMap d2r_cu_contrib_map_from_aranges(Arena *arena, DW_Input *input, U64 image_base);
internal RDIM_Rng1U64ChunkList  d2r_voff_ranges_from_cu_info_off(D2R_CompUnitContribMap map, U64 info_off);

////////////////////////////////
//~ Entry Point

internal RDIM_BakeParams d2r_convert(Arena *arena, D2R_ConvertParams *params);

#endif // RDI_FROM_DWARF_H
