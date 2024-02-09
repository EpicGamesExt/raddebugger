// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_CONS_H
#define RADDBG_CONS_H

////////////////////////////////
//- "Public Facing" Cons API

// frequently used opaque types
typedef struct CONS_Root CONS_Root;
typedef struct CONS_Unit CONS_Unit;
typedef struct CONS_Type CONS_Type;
typedef struct CONS_Reservation CONS_Reservation;
typedef struct CONS_Symbol CONS_Symbol;
typedef struct CONS_Scope CONS_Scope;
typedef struct CONS_Local CONS_Local;
typedef struct CONS_LocationSet CONS_LocationSet;
typedef struct CONS_Location CONS_Location;

typedef struct CONS_TypeNode{
  struct CONS_TypeNode *next;
  CONS_Type *type;
} CONS_TypeNode;

typedef struct CONS_TypeList{
  CONS_TypeNode *first;
  CONS_TypeNode *last;
  U64 count;
} CONS_TypeList;

typedef struct CONS_EvalBytecodeOp{
  struct CONS_EvalBytecodeOp *next;
  RADDBG_EvalOp op;
  U32 p_size;
  U64 p;
} CONS_EvalBytecodeOp;

typedef struct CONS_EvalBytecode{
  CONS_EvalBytecodeOp *first_op;
  CONS_EvalBytecodeOp *last_op;
  U32 op_count;
  U32 encoded_size;
} CONS_EvalBytecode;

//- init

typedef struct CONS_RootParams{
  // important to set this correctly so pointer
  //  types get the correct sizes
  U64 addr_size;
  
  // options to optimize map bucket counts
  // * if these are left as zeros a modest default is used instead
  U32 bucket_count_units;
  U32 bucket_count_symbols;
  U32 bucket_count_scopes;
  U32 bucket_count_locals;
  U32 bucket_count_types;
  U64 bucket_count_type_constructs;
} CONS_RootParams;

static CONS_Root* cons_root_new(CONS_RootParams *params);
static void       cons_root_release(CONS_Root *root);


//- baking
static void cons_bake_file(Arena *arena, CONS_Root *root, String8List *out);


//- errors
typedef struct CONS_Error{
  struct CONS_Error *next;
  String8 msg;
} CONS_Error;

static void cons_errorf(CONS_Root *root, char *fmt, ...);

static CONS_Error* cons_get_first_error(CONS_Root *root);



//- information declaration

// top level info
typedef struct CONS_TopLevelInfo{
  RADDBG_Arch architecture;
  String8 exe_name;
  U64 exe_hash;
  U64 voff_max;
} CONS_TopLevelInfo;

static void cons_set_top_level_info(CONS_Root *root, CONS_TopLevelInfo *tli);


// binary sections
static void cons_add_binary_section(CONS_Root *root,
                                    String8 name, RADDBG_BinarySectionFlags flags,
                                    U64 voff_first, U64 voff_opl, U64 foff_first,
                                    U64 foff_opl);


// units
static CONS_Unit* cons_unit_handle_from_user_id(CONS_Root *root, U64 unit_user_id);

typedef struct CONS_UnitInfo{
  String8 unit_name;
  String8 compiler_name;
  String8 source_file;
  String8 object_file;
  String8 archive_file;
  String8 build_path;
  RADDBG_Language language;
} CONS_UnitInfo;

static void cons_unit_set_info(CONS_Root *root, CONS_Unit *unit, CONS_UnitInfo *info);

typedef struct CONS_LineSequence{
  String8 file_name;
  U64 *voffs;     // [line_count + 1] (sorted)
  U32 *line_nums; // [line_count]
  U16 *col_nums;  // [2*line_count]
  U64 line_count;
} CONS_LineSequence;

static void cons_unit_add_line_sequence(CONS_Root *root, CONS_Unit *unit,
                                        CONS_LineSequence *line_sequence);

static void cons_unit_vmap_add_range(CONS_Root *root, CONS_Unit *unit, U64 first, U64 opl);


// types
static CONS_Type*        cons_type_from_id(CONS_Root *root, U64 type_user_id);
static CONS_Reservation* cons_type_reserve_id(CONS_Root *root, U64 type_user_id);
static void              cons_type_fill_id(CONS_Root *root, CONS_Reservation *res, CONS_Type *type);

static B32        cons_type_is_unhandled_nil(CONS_Root *root, CONS_Type *type);

static CONS_Type* cons_type_handled_nil(CONS_Root *root);
static CONS_Type* cons_type_nil(CONS_Root *root);
static CONS_Type* cons_type_variadic(CONS_Root *root);

static CONS_Type* cons_type_basic(CONS_Root *root, RADDBG_TypeKind type_kind, String8 name);
static CONS_Type* cons_type_modifier(CONS_Root *root,
                                     CONS_Type *direct_type, RADDBG_TypeModifierFlags flags);
static CONS_Type* cons_type_bitfield(CONS_Root *root,
                                     CONS_Type *direct_type, U32 bit_off, U32 bit_count);
static CONS_Type* cons_type_pointer(CONS_Root *root,
                                    CONS_Type *direct_type, RADDBG_TypeKind ptr_type_kind);
static CONS_Type* cons_type_array(CONS_Root *root,
                                  CONS_Type *direct_type, U64 count);
static CONS_Type* cons_type_proc(CONS_Root *root,
                                 CONS_Type *return_type, struct CONS_TypeList *params);
static CONS_Type* cons_type_method(CONS_Root *root,
                                   CONS_Type *this_type, CONS_Type *return_type,
                                   struct CONS_TypeList *params);

static CONS_Type* cons_type_udt(CONS_Root *root,
                                RADDBG_TypeKind record_type_kind, String8 name, U64 size);
static CONS_Type* cons_type_enum(CONS_Root *root,
                                 CONS_Type *direct_type, String8 name);
static CONS_Type* cons_type_alias(CONS_Root *root,
                                  CONS_Type *direct_type, String8 name);
static CONS_Type* cons_type_incomplete(CONS_Root *root,
                                       RADDBG_TypeKind type_kind, String8 name);

static void cons_type_add_member_data_field(CONS_Root *root, CONS_Type *record_type,
                                            String8 name, CONS_Type *mem_type, U32 off);
static void cons_type_add_member_static_data(CONS_Root *root, CONS_Type *record_type,
                                             String8 name, CONS_Type *mem_type);
static void cons_type_add_member_method(CONS_Root *root, CONS_Type *record_type,
                                        String8 name, CONS_Type *mem_type);
static void cons_type_add_member_static_method(CONS_Root *root, CONS_Type *record_type,
                                               String8 name, CONS_Type *mem_type);
static void cons_type_add_member_virtual_method(CONS_Root *root, CONS_Type *record_type,
                                                String8 name, CONS_Type *mem_type);
static void cons_type_add_member_base(CONS_Root *root, CONS_Type *record_type,
                                      CONS_Type *base_type, U32 off);
static void cons_type_add_member_virtual_base(CONS_Root *root, CONS_Type *record_type,
                                              CONS_Type *base_type, U32 vptr_off, U32 vtable_off);
static void cons_type_add_member_nested_type(CONS_Root *root, CONS_Type *record_type,
                                             CONS_Type *nested_type);

static void cons_type_add_enum_val(CONS_Root *root, CONS_Type *enum_type, String8 name, U64 val);

static void cons_type_set_source_coordinates(CONS_Root *root, CONS_Type *defined_type,
                                             String8 source_path, U32 line, U32 col);

// type list
static void cons_type_list_push(Arena *arena, CONS_TypeList *list, CONS_Type *type);

// symbols
static CONS_Symbol* cons_symbol_handle_from_user_id(CONS_Root *root, U64 symbol_user_id);

typedef enum{
  CONS_SymbolKind_NULL,
  CONS_SymbolKind_GlobalVariable,
  CONS_SymbolKind_ThreadVariable,
  CONS_SymbolKind_Procedure,
  CONS_SymbolKind_COUNT
} CONS_SymbolKind;

typedef struct CONS_SymbolInfo{
  CONS_SymbolKind kind;
  String8 name;
  String8 link_name;
  CONS_Type *type;
  B32 is_extern;
  U64 offset;
  // TODO(allen): should this actually be "container scope"?
  CONS_Symbol *container_symbol;
  CONS_Type *container_type;
  CONS_Scope *root_scope;
} CONS_SymbolInfo;

static void cons_symbol_set_info(CONS_Root *root, CONS_Symbol *symbol, CONS_SymbolInfo *info);

// scopes
static CONS_Scope *cons_scope_handle_from_user_id(CONS_Root *root, U64 scope_user_id);

static void cons_scope_set_parent(CONS_Root *root, CONS_Scope *scope, CONS_Scope *parent);
static void cons_scope_add_voff_range(CONS_Root *root, CONS_Scope *scope, U64 voff_first, U64 voff_opl);

// locals
static CONS_Local* cons_local_handle_from_user_id(CONS_Root *root, U64 local_user_id);

typedef struct CONS_LocalInfo{
  RADDBG_LocalKind kind;
  CONS_Scope *scope;
  String8 name;
  CONS_Type *type;
} CONS_LocalInfo;

static void cons_local_set_basic_info(CONS_Root *root, CONS_Local *local,
                                      CONS_LocalInfo *info);

static CONS_LocationSet* cons_location_set_from_local(CONS_Root *root, CONS_Local *local);

// locations
static void cons_location_set_add_case(CONS_Root *root, CONS_LocationSet *locset,
                                       U64 voff_min, U64 voff_max,
                                       CONS_Location *location);

static CONS_Location* cons_location_addr_bytecode_stream(CONS_Root *root,
                                                         struct CONS_EvalBytecode *bytecode);
static CONS_Location* cons_location_val_bytecode_stream(CONS_Root *root,
                                                        struct CONS_EvalBytecode *bytecode);
static CONS_Location* cons_location_addr_reg_plus_u16(CONS_Root *root, U8 reg_code, U16 offset);
static CONS_Location* cons_location_addr_addr_reg_plus_u16(CONS_Root *root, U8 reg_code, U16 offset);
static CONS_Location* cons_location_val_reg(CONS_Root *root, U8 reg_code);

// bytecode
static void cons_bytecode_push_op(Arena *arena, CONS_EvalBytecode *bytecode, RADDBG_EvalOp op, U64 p);
static void cons_bytecode_push_uconst(Arena *arena, CONS_EvalBytecode *bytecode, U64 x);
static void cons_bytecode_push_sconst(Arena *arena, CONS_EvalBytecode *bytecode, S64 x);
static void cons_bytecode_concat_in_place(CONS_EvalBytecode *left_dst,
                                          CONS_EvalBytecode *right_destroyed);

////////////////////////////////
//- Concrete Types & Implementation Helpers

// NOTE: the user should generally treat these as opaque

// errors
typedef struct CONS_ErrorList{
  CONS_Error *first;
  CONS_Error *last;
  U64 count;
} CONS_ErrorList;


// binary sections
typedef struct CONS_BinarySection{
  struct CONS_BinarySection *next;
  String8 name;
  RADDBG_BinarySectionFlags flags;
  U64 voff_first;
  U64 voff_opl;
  U64 foff_first;
  U64 foff_opl;
} CONS_BinarySection;


// units
typedef struct CONS_LineSequenceNode{
  struct CONS_LineSequenceNode *next;
  CONS_LineSequence line_seq;
} CONS_LineSequenceNode;

typedef struct CONS_Unit{
  struct CONS_Unit *next_order;
  
  U32 idx;
  
  B32 info_is_set;
  String8 unit_name;
  String8 compiler_name;
  String8 source_file;
  String8 object_file;
  String8 archive_file;
  String8 build_path;
  RADDBG_Language language;
  
  CONS_LineSequenceNode *line_seq_first;
  CONS_LineSequenceNode *line_seq_last;
  U64 line_seq_count;
  
} CONS_Unit;

typedef struct CONS_UnitVMapRange{
  struct CONS_UnitVMapRange *next;
  CONS_Unit *unit;
  U64 first;
  U64 opl;
} CONS_UnitVMapRange;


// types
typedef struct CONS_Type{
  struct CONS_Type *next_order;
  
  RADDBG_TypeKind kind;
  U32 idx;
  U32 byte_size;
  U32 flags;
  U32 off;
  U32 count;
  
  String8 name;
  
  struct CONS_Type *direct_type;
  struct CONS_Type **param_types;
  
  struct CONS_TypeUDT *udt;
} CONS_Type;

typedef struct CONS_TypeMember{
  struct CONS_TypeMember *next;
  
  RADDBG_MemberKind kind;
  String8 name;
  CONS_Type *type;
  U32 off;
} CONS_TypeMember;

typedef struct CONS_TypeEnumVal{
  struct CONS_TypeEnumVal *next;
  
  String8 name;
  U64 val;
} CONS_TypeEnumVal;

typedef struct CONS_TypeUDT{
  struct CONS_TypeUDT *next_order;
  
  U32 idx;
  struct CONS_Type *self_type;
  
  CONS_TypeMember *first_member;
  CONS_TypeMember *last_member;
  U64 member_count;
  
  CONS_TypeEnumVal *first_enum_val;
  CONS_TypeEnumVal *last_enum_val;
  U64 enum_val_count;
  
  String8 source_path;
  U32 line;
  U32 col;
} CONS_TypeUDT;

typedef U8 CONS_TypeConstructKind;
enum{
  CONS_TypeConstructKind_Basic,
  CONS_TypeConstructKind_Modifier,
  CONS_TypeConstructKind_Bitfield,
  CONS_TypeConstructKind_Pointer,
  CONS_TypeConstructKind_Array,
  CONS_TypeConstructKind_Procedure,
  CONS_TypeConstructKind_Method,
};

static CONS_Type*    cons__type_new(CONS_Root *root);
static CONS_TypeUDT* cons__type_udt_from_any_type(CONS_Root *root, CONS_Type *type);
static CONS_TypeUDT* cons__type_udt_from_record_type(CONS_Root *root, CONS_Type *type);


// symbols
typedef struct CONS_Symbol{
  struct CONS_Symbol *next_order;
  
  U32 idx;
  
  CONS_SymbolKind kind;
  String8 name;
  String8 link_name;
  CONS_Type *type;
  B32 is_extern;
  B8 offset_is_set;
  U64 offset;
  
  CONS_Symbol *container_symbol;
  CONS_Type *container_type;
  
  CONS_Scope *root_scope;
} CONS_Symbol;


// scopes
typedef struct CONS_Local{
  struct CONS_Local *next;
  RADDBG_LocalKind kind;
  String8 name;
  CONS_Type *type;
  CONS_LocationSet *locset;
} CONS_Local;

typedef struct CONS__VOffRange{
  struct CONS__VOffRange *next;
  U64 voff_first;
  U64 voff_opl;
} CONS__VOffRange;

typedef struct CONS_Scope{
  struct CONS_Scope *next_order;
  
  CONS_Symbol *symbol;
  CONS_Scope *parent_scope;
  CONS_Scope *first_child;
  CONS_Scope *last_child;
  CONS_Scope *next_sibling;
  
  U64 voff_base;
  CONS__VOffRange *first_range;
  CONS__VOffRange *last_range;
  U32 range_count;
  
  U32 idx;
  
  CONS_Local *first_local;
  CONS_Local *last_local;
  U32 local_count;
  
} CONS_Scope;

static void cons__scope_recursive_set_symbol(CONS_Scope *scope, CONS_Symbol *symbol);


// locations
typedef struct CONS_Location{
  RADDBG_LocationKind kind;
  U8 register_code;
  U16 offset;
  CONS_EvalBytecode bytecode;
} CONS_Location;

typedef struct CONS__LocationCase{
  struct CONS__LocationCase *next;
  U64 voff_first;
  U64 voff_opl;
  CONS_Location *location;
} CONS__LocationCase;

typedef struct CONS_LocationSet{
  CONS__LocationCase *first_location_case;
  CONS__LocationCase *last_location_case;
  U64 location_case_count;
} CONS_LocationSet;


// name maps
typedef struct CONS__NameMapIdxNode{
  struct CONS__NameMapIdxNode *next;
  U32 idx[8];
} CONS__NameMapIdxNode;

typedef struct CONS__NameMapNode{
  struct CONS__NameMapNode *bucket_next;
  struct CONS__NameMapNode *order_next;
  String8 string;
  CONS__NameMapIdxNode *idx_first;
  CONS__NameMapIdxNode *idx_last;
  U64 idx_count;
} CONS__NameMapNode;

typedef struct CONS__NameMap{
  CONS__NameMapNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  CONS__NameMapNode *first;
  CONS__NameMapNode *last;
  U64 name_count;
} CONS__NameMap;

static CONS__NameMap* cons__name_map_for_kind(CONS_Root *root, RADDBG_NameMapKind kind);
static void           cons__name_map_add_pair(CONS_Root *root, CONS__NameMap *map,
                                              String8 name, U32 idx);


// u64 to ptr map
typedef struct CONS__U64ToPtrNode{
  struct CONS__U64ToPtrNode *next;
  U64 key[3];
  void *ptr[3];
} CONS__U64ToPtrNode;

typedef struct CONS__U64ToPtrMap{
  CONS__U64ToPtrNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 pair_count;
} CONS__U64ToPtrMap;

typedef struct CONS__U64ToPtrLookup{
  void *match;
  CONS__U64ToPtrNode *fill_node;
  U32 fill_k;
} CONS__U64ToPtrLookup;

static void cons__u64toptr_init(Arena *arena, CONS__U64ToPtrMap *map, U64 bucket_count);

static void cons__u64toptr_lookup(CONS__U64ToPtrMap *map, U64 key, CONS__U64ToPtrLookup *lookup_out);
static void cons__u64toptr_insert(Arena *arena, CONS__U64ToPtrMap *map, U64 key,
                                  CONS__U64ToPtrLookup *lookup, void *ptr);



// str8 to ptr map
typedef struct CONS__Str8ToPtrNode{
  struct CONS__Str8ToPtrNode *next;
  String8 key;
  U64 hash;
  void *ptr;
} CONS__Str8ToPtrNode;

typedef struct CONS__Str8ToPtrMap{
  CONS__Str8ToPtrNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 pair_count;
} CONS__Str8ToPtrMap;

static void cons__str8toptr_init(Arena *arena, CONS__Str8ToPtrMap *map, U64 bucket_count);
static void*cons__str8toptr_lookup(CONS__Str8ToPtrMap *map, String8 key, U64 hash);
static void cons__str8toptr_insert(Arena *arena, CONS__Str8ToPtrMap *map,
                                   String8 key, U64 hash, void *ptr);

// root
struct CONS_Root{
  Arena *arena;
  CONS_ErrorList errors;
  
  //////// Contextual Information
  
  U64 addr_size;
  
  //////// Info Declared By User
  
  // top level info
  B32 top_level_info_is_set;
  CONS_TopLevelInfo top_level_info;
  
  // binary layout
  CONS_BinarySection *binary_section_first;
  CONS_BinarySection *binary_section_last;
  U64 binary_section_count;
  
  // compilation units
  CONS_Unit *unit_first;
  CONS_Unit *unit_last;
  U64 unit_count;
  
  CONS_UnitVMapRange *unit_vmap_range_first;
  CONS_UnitVMapRange *unit_vmap_range_last;
  U64 unit_vmap_range_count;
  
  // types
  CONS_Type *first_type;
  CONS_Type *last_type;
  U64 type_count;
  
  CONS_Type *nil_type;
  CONS_Type *variadic_type;
  
  CONS_Type handled_nil_type;
  
  CONS_TypeUDT *first_udt;
  CONS_TypeUDT *last_udt;
  U64 type_udt_count;
  
  U64 total_member_count;
  U64 total_enum_val_count;
  
  // symbols
  CONS_Symbol *first_symbol;
  CONS_Symbol *last_symbol;
  union{
    U64 symbol_count;
    U64 symbol_kind_counts[CONS_SymbolKind_COUNT];
  };
  
  CONS_Scope *first_scope;
  CONS_Scope *last_scope;
  U64 scope_count;
  U64 scope_voff_count;
  
  CONS_Local *first_local;
  CONS_Local *last_local;
  U64 local_count;
  U64 location_count;
  
  // name maps
  CONS__NameMap *name_maps[RADDBG_NameMapKind_COUNT];
  
  //////// Handle Relationship Maps
  
  CONS__U64ToPtrMap unit_map;
  CONS__U64ToPtrMap symbol_map;
  CONS__U64ToPtrMap scope_map;
  CONS__U64ToPtrMap local_map;
  CONS__U64ToPtrMap type_from_id_map;
  CONS__Str8ToPtrMap construct_map;
};


//- cons intermediate data sections
typedef struct CONS__DSectionNode{
  struct CONS__DSectionNode *next;
  void *data;
  U64 size;
  RADDBG_DataSectionTag tag;
} CONS__DSectionNode;

typedef struct CONS__DSections{
  CONS__DSectionNode *first;
  CONS__DSectionNode *last;
  U32 count;
} CONS__DSections;

//- cons intermediate strings
typedef struct CONS__StringNode{
  struct CONS__StringNode *order_next;
  struct CONS__StringNode *bucket_next;
  String8 str;
  U64 hash;
  U32 idx;
} CONS__StringNode;

typedef struct CONS__Strings{
  CONS__StringNode *order_first;
  CONS__StringNode *order_last;
  CONS__StringNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U32 count;
} CONS__Strings;

//- cons intermediate index runs
typedef struct CONS__IdxRunNode{
  struct CONS__IdxRunNode *order_next;
  struct CONS__IdxRunNode *bucket_next;
  U32 *idx_run;
  U64 hash;
  U32 count;
  U32 first_idx;
} CONS__IdxRunNode;

typedef struct CONS__IdxRuns{
  CONS__IdxRunNode *order_first;
  CONS__IdxRunNode *order_last;
  CONS__IdxRunNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U32 count;
  U32 idx_count;
} CONS__IdxRuns;

//- cons intermediate file path tree
typedef struct CONS__PathNode{
  struct CONS__PathNode *next_order;
  struct CONS__PathNode *parent;
  struct CONS__PathNode *first_child;
  struct CONS__PathNode *last_child;
  struct CONS__PathNode *next_sibling;
  String8 name;
  struct CONS__SrcNode *src_file;
  U32 idx;
} CONS__PathNode;

typedef struct CONS__LineMapFragment{
  struct CONS__LineMapFragment *next;
  CONS_LineSequenceNode *sequence;
} CONS__LineMapFragment;

typedef struct CONS__SrcNode{
  struct CONS__SrcNode *next;
  CONS__PathNode *path_node;
  U32 idx;
  
  String8 normal_full_path;
  
  // place to gather the line info attached to this src file
  CONS__LineMapFragment *first_fragment;
  CONS__LineMapFragment *last_fragment;
  
  // place to put the final baked version of this file's line map
  U32 line_map_nums_data_idx;
  U32 line_map_range_data_idx;
  U32 line_map_count;
  U32 line_map_voff_data_idx;
} CONS__SrcNode;

typedef struct CONS__PathTree{
  CONS__PathNode *first;
  CONS__PathNode *last;
  U32 count;
  CONS__PathNode root;
  CONS__SrcNode *src_first;
  CONS__SrcNode *src_last;
  U32 src_count;
} CONS__PathTree;

typedef struct CONS__BakeCtx{
  Arena *arena;
  CONS__Strings strs;
  CONS__IdxRuns idxs;
  CONS__PathTree *tree;
} CONS__BakeCtx;

//- cons intermediate functions
static U32 cons__dsection(Arena *arena, CONS__DSections *dss,
                          void *data, U64 size, RADDBG_DataSectionTag tag);

static CONS__BakeCtx* cons__bake_ctx_begin(void);
static void           cons__bake_ctx_release(CONS__BakeCtx *bake_ctx);

static U32 cons__string(CONS__BakeCtx *bctx, String8 str);

static U64 cons__idx_run_hash(U32 *idx_run, U32 count);
static U32 cons__idx_run(CONS__BakeCtx *bctx, U32 *idx_run, U32 count);

static CONS__PathNode* cons__paths_new_node(CONS__BakeCtx *bctx);
static CONS__PathNode* cons__paths_sub_path(CONS__BakeCtx *bctx, CONS__PathNode *dir, String8 sub_dir);
static CONS__PathNode* cons__paths_node_from_path(CONS__BakeCtx *bctx,  String8 path);
static U32             cons__paths_idx_from_path(CONS__BakeCtx *bctx, String8 path);

static CONS__SrcNode*  cons__paths_new_src_node(CONS__BakeCtx *bctx);
static CONS__SrcNode*  cons__paths_src_node_from_path_node(CONS__BakeCtx *bctx, CONS__PathNode *path);

//- cons path helper
static String8 cons__normal_string_from_path_node(Arena *arena, CONS__PathNode *node);
static void    cons__normal_string_from_path_node_build(Arena *arena, CONS__PathNode *node,
                                                        String8List *out);


//- cons sort helper
typedef struct CONS__SortKey{
  U64 key;
  void *val;
} CONS__SortKey;

typedef struct CONS__OrderedRange{
  struct CONS__OrderedRange *next;
  U64 first;
  U64 opl;
} CONS__OrderedRange;

static CONS__SortKey* cons__sort_key_array(Arena *arena, CONS__SortKey *keys, U64 count);


//- cons serializer for unit line info
typedef struct CONS__LineRec{
  U32 file_id;
  U32 line_num;
  U16 col_first;
  U16 col_opl;
} CONS__LineRec;

typedef struct CONS__UnitLinesCombined{
  U64 *voffs;
  RADDBG_Line *lines;
  U16 *cols;
  U32 line_count;
} CONS__UnitLinesCombined;

static CONS__UnitLinesCombined* cons__unit_combine_lines(Arena *arena, CONS__BakeCtx *bctx,
                                                         CONS_LineSequenceNode *first);

//- cons serializer for source line info
typedef struct CONS__SrcLinesCombined{
  U32 *line_nums;
  U32 *line_ranges;
  U64 *voffs;
  U32  line_count;
  U32  voff_count;
} CONS__SrcLinesCombined;

typedef struct CONS__SrcLineMapVoffBlock{
  struct CONS__SrcLineMapVoffBlock *next;
  U64 voff;
} CONS__SrcLineMapVoffBlock;

typedef struct CONS__SrcLineMapBucket{
  struct CONS__SrcLineMapBucket *order_next;
  struct CONS__SrcLineMapBucket *hash_next;
  U32 line_num;
  CONS__SrcLineMapVoffBlock *first_voff_block;
  CONS__SrcLineMapVoffBlock *last_voff_block;
  U64 voff_count;
} CONS__SrcLineMapBucket;

static CONS__SrcLinesCombined* cons__source_combine_lines(Arena *arena, CONS__LineMapFragment *first);

//- cons serializer for vmap type
typedef struct CONS__VMap{
  RADDBG_VMapEntry *vmap; // [count + 1]
  U32 count;
} CONS__VMap;

typedef struct CONS__VMapMarker{
  U32 idx;
  U32 begin_range;
} CONS__VMapMarker;

typedef struct CONS__VMapRangeTracker{
  struct CONS__VMapRangeTracker *next;
  U32 idx;
} CONS__VMapRangeTracker;

static CONS__VMap* cons__vmap_from_markers(Arena *arena, CONS__VMapMarker *markers, CONS__SortKey *keys,
                                           U64 marker_count);

//- cons serializer for unit vmap
static CONS__VMap* cons__vmap_from_unit_ranges(Arena *arena, CONS_UnitVMapRange *first, U64 count);

//- cons serializer for types
typedef struct CONS__TypeData{
  RADDBG_TypeNode *type_nodes;
  U32 type_node_count;
  
  RADDBG_UDT *udts;
  U32 udt_count;
  
  RADDBG_Member *members;
  U32 member_count;
  
  RADDBG_EnumMember *enum_members;
  U32 enum_member_count;
} CONS__TypeData;

static CONS__TypeData* cons__type_data_combine(Arena *arena, CONS_Root *root, CONS__BakeCtx *bctx);

static U32* cons__idx_run_from_types(Arena *arena, CONS_Type **types, U32 count);

//- cons serializer for symbols
typedef struct CONS__SymbolData{
  RADDBG_GlobalVariable *global_variables;
  U32 global_variable_count;
  
  CONS__VMap *global_vmap;
  
  RADDBG_ThreadVariable *thread_variables;
  U32 thread_variable_count;
  
  RADDBG_Procedure *procedures;
  U32 procedure_count;
  
  RADDBG_Scope *scopes;
  U32 scope_count;
  
  U64 *scope_voffs;
  U32 scope_voff_count;
  
  CONS__VMap *scope_vmap;
  
  RADDBG_Local *locals;
  U32 local_count;
  
  RADDBG_LocationBlock *location_blocks;
  U32 location_block_count;
  
  void *location_data;
  U32 location_data_size;
  
} CONS__SymbolData;

static CONS__SymbolData* cons__symbol_data_combine(Arena *arena, CONS_Root *root, CONS__BakeCtx *bctx);

//- cons serializer for name maps
typedef struct CONS__NameMapSemiNode{
  struct CONS__NameMapSemiNode *next;
  CONS__NameMapNode *node;
} CONS__NameMapSemiNode;

typedef struct CONS__NameMapSemiBucket{
  CONS__NameMapSemiNode *first;
  CONS__NameMapSemiNode *last;
  U64 count;
} CONS__NameMapSemiBucket;

typedef struct CONS__NameMapBaked{
  RADDBG_NameMapBucket *buckets;
  RADDBG_NameMapNode *nodes;
  U32 bucket_count;
  U32 node_count;
} CONS__NameMapBaked;

static CONS__NameMapBaked* cons__name_map_bake(Arena *arena, CONS_Root *root, CONS__BakeCtx *bctx, CONS__NameMap *map);

#endif //RADDBG_CONS_H
