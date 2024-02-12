// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_CONS_H
#define RADDBGI_CONS_H

////////////////////////////////
//~ rjf: Error Types

typedef struct CONS_Error CONS_Error;
struct CONS_Error
{
  CONS_Error *next;
  String8 msg;
};

typedef struct CONS_ErrorList CONS_ErrorList;
struct CONS_ErrorList
{
  CONS_Error *first;
  CONS_Error *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Auxiliary Data Structure Types

//- rjf: u64 -> pointer map

typedef struct CONS__U64ToPtrNode CONS__U64ToPtrNode;
struct CONS__U64ToPtrNode
{
  CONS__U64ToPtrNode *next;
  U64 _padding_;
  U64 key[1];
  void *ptr[1];
};

typedef struct CONS__U64ToPtrMap CONS__U64ToPtrMap;
struct CONS__U64ToPtrMap
{
  CONS__U64ToPtrNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 pair_count;
};

typedef struct CONS__U64ToPtrLookup CONS__U64ToPtrLookup;
struct CONS__U64ToPtrLookup
{
  void *match;
  CONS__U64ToPtrNode *fill_node;
  U32 fill_k;
};

//- rjf: string8 -> pointer map

typedef struct CONS__Str8ToPtrNode CONS__Str8ToPtrNode;
struct CONS__Str8ToPtrNode
{
  struct CONS__Str8ToPtrNode *next;
  String8 key;
  U64 hash;
  void *ptr;
};

typedef struct CONS__Str8ToPtrMap CONS__Str8ToPtrMap;
struct CONS__Str8ToPtrMap
{
  CONS__Str8ToPtrNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 pair_count;
};

//- rjf: sortable range data structure

typedef struct CONS__SortKey CONS__SortKey;
struct CONS__SortKey
{
  U64 key;
  void *val;
};

typedef struct CONS__OrderedRange CONS__OrderedRange;
struct CONS__OrderedRange
{
  CONS__OrderedRange *next;
  U64 first;
  U64 opl;
};

////////////////////////////////
//~ rjf: Binary Section Types

typedef struct CONS_BinarySection CONS_BinarySection;
struct CONS_BinarySection
{
  CONS_BinarySection *next;
  String8 name;
  RADDBGI_BinarySectionFlags flags;
  U64 voff_first;
  U64 voff_opl;
  U64 foff_first;
  U64 foff_opl;
};

////////////////////////////////
//~ rjf: Per-Compilation-Unit Info Types

typedef struct CONS_LineSequence CONS_LineSequence;
struct CONS_LineSequence
{
  String8 file_name;
  U64 *voffs;     // [line_count + 1] (sorted)
  U32 *line_nums; // [line_count]
  U16 *col_nums;  // [2*line_count]
  U64 line_count;
};

typedef struct CONS_LineSequenceNode CONS_LineSequenceNode;
struct CONS_LineSequenceNode
{
  CONS_LineSequenceNode *next;
  CONS_LineSequence line_seq;
};

typedef struct CONS_UnitInfo CONS_UnitInfo;
struct CONS_UnitInfo
{
  String8 unit_name;
  String8 compiler_name;
  String8 source_file;
  String8 object_file;
  String8 archive_file;
  String8 build_path;
  RADDBGI_Language language;
};

typedef struct CONS_Unit CONS_Unit;
struct CONS_Unit
{
  CONS_Unit *next_order;
  U32 idx;
  B32 info_is_set;
  String8 unit_name;
  String8 compiler_name;
  String8 source_file;
  String8 object_file;
  String8 archive_file;
  String8 build_path;
  RADDBGI_Language language;
  CONS_LineSequenceNode *line_seq_first;
  CONS_LineSequenceNode *line_seq_last;
  U64 line_seq_count;
};

typedef struct CONS_UnitVMapRange CONS_UnitVMapRange;
struct CONS_UnitVMapRange
{
  CONS_UnitVMapRange *next;
  CONS_Unit *unit;
  U64 first;
  U64 opl;
};

////////////////////////////////
//~ rjf: Type Info Types

typedef U8 CONS_TypeConstructKind;
enum
{
  CONS_TypeConstructKind_Basic,
  CONS_TypeConstructKind_Modifier,
  CONS_TypeConstructKind_Bitfield,
  CONS_TypeConstructKind_Pointer,
  CONS_TypeConstructKind_Array,
  CONS_TypeConstructKind_Procedure,
  CONS_TypeConstructKind_Method,
};

typedef struct CONS_Reservation CONS_Reservation;

typedef struct CONS_TypeMember CONS_TypeMember;
struct CONS_TypeMember
{
  CONS_TypeMember *next;
  RADDBGI_MemberKind kind;
  String8 name;
  struct CONS_Type *type;
  U32 off;
};

typedef struct CONS_TypeEnumVal CONS_TypeEnumVal;
struct CONS_TypeEnumVal
{
  CONS_TypeEnumVal *next;
  String8 name;
  U64 val;
};

typedef struct CONS_Type CONS_Type;
struct CONS_Type
{
  CONS_Type *next_order;
  RADDBGI_TypeKind kind;
  U32 idx;
  U32 byte_size;
  U32 flags;
  U32 off;
  U32 count;
  String8 name;
  CONS_Type *direct_type;
  CONS_Type **param_types;
  struct CONS_TypeUDT *udt;
};

typedef struct CONS_TypeUDT CONS_TypeUDT;
struct CONS_TypeUDT
{
  CONS_TypeUDT *next_order;
  U32 idx;
  CONS_Type *self_type;
  CONS_TypeMember *first_member;
  CONS_TypeMember *last_member;
  U64 member_count;
  CONS_TypeEnumVal *first_enum_val;
  CONS_TypeEnumVal *last_enum_val;
  U64 enum_val_count;
  String8 source_path;
  U32 line;
  U32 col;
};

typedef struct CONS_TypeNode CONS_TypeNode;
struct CONS_TypeNode
{
  CONS_TypeNode *next;
  CONS_Type *type;
};

typedef struct CONS_TypeList CONS_TypeList;
struct CONS_TypeList
{
  CONS_TypeNode *first;
  CONS_TypeNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Symbol Info Types

typedef enum CONS_SymbolKind
{
  CONS_SymbolKind_NULL,
  CONS_SymbolKind_GlobalVariable,
  CONS_SymbolKind_ThreadVariable,
  CONS_SymbolKind_Procedure,
  CONS_SymbolKind_COUNT
}
CONS_SymbolKind;

typedef struct CONS_SymbolInfo CONS_SymbolInfo;
struct CONS_SymbolInfo
{
  CONS_SymbolKind kind;
  String8 name;
  String8 link_name;
  CONS_Type *type;
  B32 is_extern;
  U64 offset;
  // TODO(allen): should this actually be "container scope"?
  struct CONS_Symbol *container_symbol;
  CONS_Type *container_type;
  struct CONS_Scope *root_scope;
};

typedef struct CONS_Symbol CONS_Symbol;
struct CONS_Symbol
{
  CONS_Symbol *next_order;
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
  struct CONS_Scope *root_scope;
};

////////////////////////////////
//~ rjf: Scope Info Types

typedef struct CONS_LocalInfo CONS_LocalInfo;
struct CONS_LocalInfo
{
  RADDBGI_LocalKind kind;
  struct CONS_Scope *scope;
  String8 name;
  CONS_Type *type;
};

typedef struct CONS_Local CONS_Local;
struct CONS_Local
{
  CONS_Local *next;
  RADDBGI_LocalKind kind;
  String8 name;
  CONS_Type *type;
  struct CONS_LocationSet *locset;
};

typedef struct CONS__VOffRange CONS__VOffRange;
struct CONS__VOffRange
{
  CONS__VOffRange *next;
  U64 voff_first;
  U64 voff_opl;
};

typedef struct CONS_Scope CONS_Scope;
struct CONS_Scope
{
  CONS_Scope *next_order;
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
};

////////////////////////////////
//~ rjf: Location Info Types

typedef struct CONS_EvalBytecodeOp CONS_EvalBytecodeOp;
struct CONS_EvalBytecodeOp
{
  CONS_EvalBytecodeOp *next;
  RADDBGI_EvalOp op;
  U32 p_size;
  U64 p;
};

typedef struct CONS_EvalBytecode CONS_EvalBytecode;
struct CONS_EvalBytecode
{
  CONS_EvalBytecodeOp *first_op;
  CONS_EvalBytecodeOp *last_op;
  U32 op_count;
  U32 encoded_size;
};

typedef struct CONS_Location CONS_Location;
struct CONS_Location
{
  RADDBGI_LocationKind kind;
  U8 register_code;
  U16 offset;
  CONS_EvalBytecode bytecode;
};

typedef struct CONS__LocationCase CONS__LocationCase;
struct CONS__LocationCase
{
  CONS__LocationCase *next;
  U64 voff_first;
  U64 voff_opl;
  CONS_Location *location;
};

typedef struct CONS_LocationSet CONS_LocationSet;
struct CONS_LocationSet
{
  CONS__LocationCase *first_location_case;
  CONS__LocationCase *last_location_case;
  U64 location_case_count;
};

////////////////////////////////
//~ rjf: Name Map Types

typedef struct CONS__NameMapIdxNode CONS__NameMapIdxNode;
struct CONS__NameMapIdxNode
{
  CONS__NameMapIdxNode *next;
  U32 idx[8];
};

typedef struct CONS__NameMapNode CONS__NameMapNode;
struct CONS__NameMapNode
{
  CONS__NameMapNode *bucket_next;
  CONS__NameMapNode *order_next;
  String8 string;
  CONS__NameMapIdxNode *idx_first;
  CONS__NameMapIdxNode *idx_last;
  U64 idx_count;
};

typedef struct CONS__NameMap CONS__NameMap;
struct CONS__NameMap
{
  CONS__NameMapNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  CONS__NameMapNode *first;
  CONS__NameMapNode *last;
  U64 name_count;
};

////////////////////////////////
//~ rjf: Top-Level Debug Info Types

typedef struct CONS_TopLevelInfo CONS_TopLevelInfo;
struct CONS_TopLevelInfo
{
  RADDBGI_Arch architecture;
  String8 exe_name;
  U64 exe_hash;
  U64 voff_max;
};

////////////////////////////////
//~ rjf: Root Construction Bundle Types

typedef struct CONS_RootParams CONS_RootParams;
struct CONS_RootParams
{
  U64 addr_size;
  U32 bucket_count_units;              // optional; default chosen if 0
  U32 bucket_count_symbols;            // optional; default chosen if 0
  U32 bucket_count_scopes;             // optional; default chosen if 0
  U32 bucket_count_locals;             // optional; default chosen if 0
  U32 bucket_count_types;              // optional; default chosen if 0
  U64 bucket_count_type_constructs;    // optional; default chosen if 0
};

typedef struct CONS_Root CONS_Root;
struct CONS_Root
{
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
  CONS__NameMap *name_maps[RADDBGI_NameMapKind_COUNT];
  
  //////// Handle Relationship Maps
  
  CONS__U64ToPtrMap unit_map;
  CONS__U64ToPtrMap symbol_map;
  CONS__U64ToPtrMap scope_map;
  CONS__U64ToPtrMap local_map;
  CONS__U64ToPtrMap type_from_id_map;
  CONS__Str8ToPtrMap construct_map;
};

////////////////////////////////
//~ rjf: Baking Phase Types

//- rjf: bake data section data structure

typedef struct CONS__DSectionNode CONS__DSectionNode;
struct CONS__DSectionNode
{
  CONS__DSectionNode *next;
  void *data;
  U64 size;
  RADDBGI_DataSectionTag tag;
};

typedef struct CONS__DSections CONS__DSections;
struct CONS__DSections
{
  CONS__DSectionNode *first;
  CONS__DSectionNode *last;
  U32 count;
};

//- rjf: bake string data structure

typedef struct CONS__StringNode CONS__StringNode;
struct CONS__StringNode
{
  CONS__StringNode *order_next;
  CONS__StringNode *bucket_next;
  String8 str;
  U64 hash;
  U32 idx;
};

typedef struct CONS__Strings CONS__Strings;
struct CONS__Strings
{
  CONS__StringNode *order_first;
  CONS__StringNode *order_last;
  CONS__StringNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U32 count;
};

//- rjf: index run baking data structure

typedef struct CONS__IdxRunNode CONS__IdxRunNode;
struct CONS__IdxRunNode
{
  CONS__IdxRunNode *order_next;
  CONS__IdxRunNode *bucket_next;
  U32 *idx_run;
  U64 hash;
  U32 count;
  U32 first_idx;
};

typedef struct CONS__IdxRuns CONS__IdxRuns;
struct CONS__IdxRuns
{
  CONS__IdxRunNode *order_first;
  CONS__IdxRunNode *order_last;
  CONS__IdxRunNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U32 count;
  U32 idx_count;
};

//- rjf: source file & file path baking data structures

typedef struct CONS__PathNode CONS__PathNode;
struct CONS__PathNode
{
  CONS__PathNode *next_order;
  CONS__PathNode *parent;
  CONS__PathNode *first_child;
  CONS__PathNode *last_child;
  CONS__PathNode *next_sibling;
  String8 name;
  struct CONS__SrcNode *src_file;
  U32 idx;
};

typedef struct CONS__LineMapFragment CONS__LineMapFragment;
struct CONS__LineMapFragment
{
  CONS__LineMapFragment *next;
  CONS_LineSequenceNode *sequence;
};

typedef struct CONS__SrcNode CONS__SrcNode;
struct CONS__SrcNode
{
  CONS__SrcNode *next;
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
};

typedef struct CONS__PathTree CONS__PathTree;
struct CONS__PathTree
{
  CONS__PathNode *first;
  CONS__PathNode *last;
  U32 count;
  CONS__PathNode root;
  CONS__SrcNode *src_first;
  CONS__SrcNode *src_last;
  U32 src_count;
};

//- rjf: line info baking data structures

typedef struct CONS__LineRec CONS__LineRec;
struct CONS__LineRec
{
  U32 file_id;
  U32 line_num;
  U16 col_first;
  U16 col_opl;
};

typedef struct CONS__UnitLinesCombined CONS__UnitLinesCombined;
struct CONS__UnitLinesCombined
{
  U64 *voffs;
  RADDBGI_Line *lines;
  U16 *cols;
  U32 line_count;
};

typedef struct CONS__SrcLinesCombined CONS__SrcLinesCombined;
struct CONS__SrcLinesCombined
{
  U32 *line_nums;
  U32 *line_ranges;
  U64 *voffs;
  U32  line_count;
  U32  voff_count;
};

typedef struct CONS__SrcLineMapVoffBlock CONS__SrcLineMapVoffBlock;
struct CONS__SrcLineMapVoffBlock
{
  CONS__SrcLineMapVoffBlock *next;
  U64 voff;
};

typedef struct CONS__SrcLineMapBucket CONS__SrcLineMapBucket;
struct CONS__SrcLineMapBucket
{
  CONS__SrcLineMapBucket *order_next;
  CONS__SrcLineMapBucket *hash_next;
  U32 line_num;
  CONS__SrcLineMapVoffBlock *first_voff_block;
  CONS__SrcLineMapVoffBlock *last_voff_block;
  U64 voff_count;
};

//- rjf: vmap baking data structure 

typedef struct CONS__VMap CONS__VMap;
struct CONS__VMap
{
  RADDBGI_VMapEntry *vmap; // [count + 1]
  U32 count;
};

typedef struct CONS__VMapMarker CONS__VMapMarker;
struct CONS__VMapMarker
{
  U32 idx;
  U32 begin_range;
};

typedef struct CONS__VMapRangeTracker CONS__VMapRangeTracker;
struct CONS__VMapRangeTracker
{
  CONS__VMapRangeTracker *next;
  U32 idx;
};

//- rjf: type data baking types

typedef struct CONS__TypeData CONS__TypeData;
struct CONS__TypeData
{
  RADDBGI_TypeNode *type_nodes;
  U32 type_node_count;
  
  RADDBGI_UDT *udts;
  U32 udt_count;
  
  RADDBGI_Member *members;
  U32 member_count;
  
  RADDBGI_EnumMember *enum_members;
  U32 enum_member_count;
};

//- rjf: symbol data baking types

typedef struct CONS__SymbolData CONS__SymbolData;
struct CONS__SymbolData
{
  RADDBGI_GlobalVariable *global_variables;
  U32 global_variable_count;
  
  CONS__VMap *global_vmap;
  
  RADDBGI_ThreadVariable *thread_variables;
  U32 thread_variable_count;
  
  RADDBGI_Procedure *procedures;
  U32 procedure_count;
  
  RADDBGI_Scope *scopes;
  U32 scope_count;
  
  U64 *scope_voffs;
  U32 scope_voff_count;
  
  CONS__VMap *scope_vmap;
  
  RADDBGI_Local *locals;
  U32 local_count;
  
  RADDBGI_LocationBlock *location_blocks;
  U32 location_block_count;
  
  void *location_data;
  U32 location_data_size;
};

//- rjf: name map baking types

typedef struct CONS__NameMapSemiNode CONS__NameMapSemiNode;
struct CONS__NameMapSemiNode
{
  CONS__NameMapSemiNode *next;
  CONS__NameMapNode *node;
};

typedef struct CONS__NameMapSemiBucket CONS__NameMapSemiBucket;
struct CONS__NameMapSemiBucket
{
  CONS__NameMapSemiNode *first;
  CONS__NameMapSemiNode *last;
  U64 count;
};

typedef struct CONS__NameMapBaked CONS__NameMapBaked;
struct CONS__NameMapBaked
{
  RADDBGI_NameMapBucket *buckets;
  RADDBGI_NameMapNode *nodes;
  U32 bucket_count;
  U32 node_count;
};

//- rjf: bundle baking context type

typedef struct CONS__BakeParams CONS__BakeParams;
struct CONS__BakeParams
{
  U64 strings_bucket_count;
  U64 idx_runs_bucket_count;
};

typedef struct CONS__BakeCtx CONS__BakeCtx;
struct CONS__BakeCtx
{
  Arena *arena;
  CONS__Strings strs;
  CONS__IdxRuns idxs;
  CONS__PathTree *tree;
};

////////////////////////////////
//~ rjf: Basic Type Helpers

//- rjf: type lists
static void cons_type_list_push(Arena *arena, CONS_TypeList *list, CONS_Type *type);

//- rjf: bytecode lists
static void cons_bytecode_push_op(Arena *arena, CONS_EvalBytecode *bytecode, RADDBGI_EvalOp op, U64 p);
static void cons_bytecode_push_uconst(Arena *arena, CONS_EvalBytecode *bytecode, U64 x);
static void cons_bytecode_push_sconst(Arena *arena, CONS_EvalBytecode *bytecode, S64 x);
static void cons_bytecode_concat_in_place(CONS_EvalBytecode *left_dst, CONS_EvalBytecode *right_destroyed);

//- rjf: sortable range sorting
static CONS__SortKey* cons__sort_key_array(Arena *arena, CONS__SortKey *keys, U64 count);

////////////////////////////////
//~ rjf: Auxiliary Data Structure Functions

//- rjf: u64 -> ptr map
static void cons__u64toptr_init(Arena *arena, CONS__U64ToPtrMap *map, U64 bucket_count);
static void cons__u64toptr_lookup(CONS__U64ToPtrMap *map, U64 key, U64 hash, CONS__U64ToPtrLookup *lookup_out);
static void cons__u64toptr_insert(Arena *arena, CONS__U64ToPtrMap *map, U64 key, U64 hash, CONS__U64ToPtrLookup *lookup, void *ptr);

//- rjf: string8 -> ptr map
static void cons__str8toptr_init(Arena *arena, CONS__Str8ToPtrMap *map, U64 bucket_count);
static void*cons__str8toptr_lookup(CONS__Str8ToPtrMap *map, String8 key, U64 hash);
static void cons__str8toptr_insert(Arena *arena, CONS__Str8ToPtrMap *map, String8 key, U64 hash, void *ptr);

////////////////////////////////
//~ rjf: Loose Debug Info Construction (Anything -> Loose) Functions

//- rjf: root creation
static CONS_Root* cons_root_new(CONS_RootParams *params);
static void       cons_root_release(CONS_Root *root);

//- rjf: error accumulation
static void cons_error(CONS_Root *root, String8 string);
static void cons_errorf(CONS_Root *root, char *fmt, ...);
static CONS_Error* cons_get_first_error(CONS_Root *root);

//- rjf: top-level info specification
static void cons_set_top_level_info(CONS_Root *root, CONS_TopLevelInfo *tli);

//- rjf: binary section building
static void cons_add_binary_section(CONS_Root *root,
                                    String8 name, RADDBGI_BinarySectionFlags flags,
                                    U64 voff_first, U64 voff_opl, U64 foff_first,
                                    U64 foff_opl);

//- rjf: unit info building
static CONS_Unit* cons_unit_handle_from_user_id(CONS_Root *root, U64 unit_user_id, U64 unit_user_id_hash);
static void cons_unit_set_info(CONS_Root *root, CONS_Unit *unit, CONS_UnitInfo *info);
static void cons_unit_add_line_sequence(CONS_Root *root, CONS_Unit *unit,
                                        CONS_LineSequence *line_sequence);
static void cons_unit_vmap_add_range(CONS_Root *root, CONS_Unit *unit, U64 first, U64 opl);

//- rjf: type info lookups/reservations
static CONS_Type*        cons_type_from_id(CONS_Root *root, U64 type_user_id, U64 type_user_id_hash);
static CONS_Reservation* cons_type_reserve_id(CONS_Root *root, U64 type_user_id, U64 type_user_id_hash);
static void              cons_type_fill_id(CONS_Root *root, CONS_Reservation *res, CONS_Type *type);

//- rjf: nil/singleton types
static B32        cons_type_is_unhandled_nil(CONS_Root *root, CONS_Type *type);
static CONS_Type* cons_type_handled_nil(CONS_Root *root);
static CONS_Type* cons_type_nil(CONS_Root *root);
static CONS_Type* cons_type_variadic(CONS_Root *root);

//- rjf: base type info constructors
static CONS_Type*    cons__type_new(CONS_Root *root);
static CONS_TypeUDT* cons__type_udt_from_any_type(CONS_Root *root, CONS_Type *type);
static CONS_TypeUDT* cons__type_udt_from_record_type(CONS_Root *root, CONS_Type *type);

//- rjf: basic/operator type construction helpers
static CONS_Type* cons_type_basic(CONS_Root *root, RADDBGI_TypeKind type_kind, String8 name);
static CONS_Type* cons_type_modifier(CONS_Root *root, CONS_Type *direct_type, RADDBGI_TypeModifierFlags flags);
static CONS_Type* cons_type_bitfield(CONS_Root *root, CONS_Type *direct_type, U32 bit_off, U32 bit_count);
static CONS_Type* cons_type_pointer(CONS_Root *root, CONS_Type *direct_type, RADDBGI_TypeKind ptr_type_kind);
static CONS_Type* cons_type_array(CONS_Root *root, CONS_Type *direct_type, U64 count);
static CONS_Type* cons_type_proc(CONS_Root *root, CONS_Type *return_type, struct CONS_TypeList *params);
static CONS_Type* cons_type_method(CONS_Root *root, CONS_Type *this_type, CONS_Type *return_type, struct CONS_TypeList *params);

//- rjf: udt type constructors
static CONS_Type* cons_type_udt(CONS_Root *root, RADDBGI_TypeKind record_type_kind, String8 name, U64 size);
static CONS_Type* cons_type_enum(CONS_Root *root, CONS_Type *direct_type, String8 name);
static CONS_Type* cons_type_alias(CONS_Root *root, CONS_Type *direct_type, String8 name);
static CONS_Type* cons_type_incomplete(CONS_Root *root, RADDBGI_TypeKind type_kind, String8 name);

//- rjf: type member building
static void cons_type_add_member_data_field(CONS_Root *root, CONS_Type *record_type, String8 name, CONS_Type *mem_type, U32 off);
static void cons_type_add_member_static_data(CONS_Root *root, CONS_Type *record_type, String8 name, CONS_Type *mem_type);
static void cons_type_add_member_method(CONS_Root *root, CONS_Type *record_type, String8 name, CONS_Type *mem_type);
static void cons_type_add_member_static_method(CONS_Root *root, CONS_Type *record_type, String8 name, CONS_Type *mem_type);
static void cons_type_add_member_virtual_method(CONS_Root *root, CONS_Type *record_type, String8 name, CONS_Type *mem_type);
static void cons_type_add_member_base(CONS_Root *root, CONS_Type *record_type, CONS_Type *base_type, U32 off);
static void cons_type_add_member_virtual_base(CONS_Root *root, CONS_Type *record_type, CONS_Type *base_type, U32 vptr_off, U32 vtable_off);
static void cons_type_add_member_nested_type(CONS_Root *root, CONS_Type *record_type, CONS_Type *nested_type);
static void cons_type_add_enum_val(CONS_Root *root, CONS_Type *enum_type, String8 name, U64 val);

//- rjf: type source coordinate specifications
static void cons_type_set_source_coordinates(CONS_Root *root, CONS_Type *defined_type, String8 source_path, U32 line, U32 col);

//- rjf: symbol info building
static CONS_Symbol* cons_symbol_handle_from_user_id(CONS_Root *root, U64 symbol_user_id, U64 symbol_user_id_hash);
static void cons_symbol_set_info(CONS_Root *root, CONS_Symbol *symbol, CONS_SymbolInfo *info);

//- rjf: scope info building
static CONS_Scope *cons_scope_handle_from_user_id(CONS_Root *root, U64 scope_user_id, U64 scope_user_id_hash);
static void cons_scope_set_parent(CONS_Root *root, CONS_Scope *scope, CONS_Scope *parent);
static void cons_scope_add_voff_range(CONS_Root *root, CONS_Scope *scope, U64 voff_first, U64 voff_opl);
static void cons__scope_recursive_set_symbol(CONS_Scope *scope, CONS_Symbol *symbol);

//- rjf: local info building
static CONS_Local* cons_local_handle_from_user_id(CONS_Root *root, U64 local_user_id, U64 local_user_id_hash);
static void cons_local_set_basic_info(CONS_Root *root, CONS_Local *local, CONS_LocalInfo *info);
static CONS_LocationSet* cons_location_set_from_local(CONS_Root *root, CONS_Local *local);

//- rjf: location info building
static void cons_location_set_add_case(CONS_Root *root, CONS_LocationSet *locset, U64 voff_first, U64 voff_opl, CONS_Location *location);
static CONS_Location* cons_location_addr_bytecode_stream(CONS_Root *root, struct CONS_EvalBytecode *bytecode);
static CONS_Location* cons_location_val_bytecode_stream(CONS_Root *root, struct CONS_EvalBytecode *bytecode);
static CONS_Location* cons_location_addr_reg_plus_u16(CONS_Root *root, U8 reg_code, U16 offset);
static CONS_Location* cons_location_addr_addr_reg_plus_u16(CONS_Root *root, U8 reg_code, U16 offset);
static CONS_Location* cons_location_val_reg(CONS_Root *root, U8 reg_code);

//- rjf: name map building
static CONS__NameMap* cons__name_map_for_kind(CONS_Root *root, RADDBGI_NameMapKind kind);
static void           cons__name_map_add_pair(CONS_Root *root, CONS__NameMap *map, String8 name, U32 idx);

////////////////////////////////
//~ rjf: Debug Info Baking (Loose -> Tight) Functions

//- rjf: bake context construction
static CONS__BakeCtx* cons__bake_ctx_begin(CONS__BakeParams *params);
static void           cons__bake_ctx_release(CONS__BakeCtx *bake_ctx);

//- rjf: string baking
static U32 cons__string(CONS__BakeCtx *bctx, String8 str);

//- rjf: idx run baking
static U64 cons__idx_run_hash(U32 *idx_run, U32 count);
static U32 cons__idx_run(CONS__BakeCtx *bctx, U32 *idx_run, U32 count);

//- rjf: data section baking
static U32 cons__dsection(Arena *arena, CONS__DSections *dss, void *data, U64 size, RADDBGI_DataSectionTag tag);

//- rjf: paths baking
static String8 cons__normal_string_from_path_node(Arena *arena, CONS__PathNode *node);
static void    cons__normal_string_from_path_node_build(Arena *arena, CONS__PathNode *node, String8List *out);
static CONS__PathNode* cons__paths_new_node(CONS__BakeCtx *bctx);
static CONS__PathNode* cons__paths_sub_path(CONS__BakeCtx *bctx, CONS__PathNode *dir, String8 sub_dir);
static CONS__PathNode* cons__paths_node_from_path(CONS__BakeCtx *bctx,  String8 path);
static U32             cons__paths_idx_from_path(CONS__BakeCtx *bctx, String8 path);
static CONS__SrcNode*  cons__paths_new_src_node(CONS__BakeCtx *bctx);
static CONS__SrcNode*  cons__paths_src_node_from_path_node(CONS__BakeCtx *bctx, CONS__PathNode *path_node);

//- rjf: per-unit line info baking
static CONS__UnitLinesCombined* cons__unit_combine_lines(Arena *arena, CONS__BakeCtx *bctx, CONS_LineSequenceNode *first_seq);

//- rjf: per-src line info baking
static CONS__SrcLinesCombined* cons__source_combine_lines(Arena *arena, CONS__LineMapFragment *first);

//- rjf: vmap baking
static CONS__VMap* cons__vmap_from_markers(Arena *arena, CONS__VMapMarker *markers, CONS__SortKey *keys, U64 marker_count);
static CONS__VMap* cons__vmap_from_unit_ranges(Arena *arena, CONS_UnitVMapRange *first, U64 count);

//- rjf: type info baking
static U32* cons__idx_run_from_types(Arena *arena, CONS_Type **types, U32 count);
static CONS__TypeData* cons__type_data_combine(Arena *arena, CONS_Root *root, CONS__BakeCtx *bctx);

//- rjf: symbol data baking
static CONS__SymbolData* cons__symbol_data_combine(Arena *arena, CONS_Root *root, CONS__BakeCtx *bctx);

//- rjf: name map baking
static CONS__NameMapBaked* cons__name_map_bake(Arena *arena, CONS_Root *root, CONS__BakeCtx *bctx, CONS__NameMap *map);

//- rjf: top-level baking entry point
static void cons_bake_file(Arena *arena, CONS_Root *root, String8List *out);

#endif // RADDBGI_CONS_H
