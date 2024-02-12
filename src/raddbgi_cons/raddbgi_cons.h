// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_CONS_H
#define RADDBGI_CONS_H

////////////////////////////////
//~ rjf: Error Types

typedef struct RADDBGIC_Error RADDBGIC_Error;
struct RADDBGIC_Error
{
  RADDBGIC_Error *next;
  String8 msg;
};

typedef struct RADDBGIC_ErrorList RADDBGIC_ErrorList;
struct RADDBGIC_ErrorList
{
  RADDBGIC_Error *first;
  RADDBGIC_Error *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Auxiliary Data Structure Types

//- rjf: u64 -> pointer map

typedef struct RADDBGIC_U64ToPtrNode RADDBGIC_U64ToPtrNode;
struct RADDBGIC_U64ToPtrNode
{
  RADDBGIC_U64ToPtrNode *next;
  U64 _padding_;
  U64 key[1];
  void *ptr[1];
};

typedef struct RADDBGIC_U64ToPtrMap RADDBGIC_U64ToPtrMap;
struct RADDBGIC_U64ToPtrMap
{
  RADDBGIC_U64ToPtrNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 pair_count;
};

typedef struct RADDBGIC_U64ToPtrLookup RADDBGIC_U64ToPtrLookup;
struct RADDBGIC_U64ToPtrLookup
{
  void *match;
  RADDBGIC_U64ToPtrNode *fill_node;
  U32 fill_k;
};

//- rjf: string8 -> pointer map

typedef struct RADDBGIC_Str8ToPtrNode RADDBGIC_Str8ToPtrNode;
struct RADDBGIC_Str8ToPtrNode
{
  struct RADDBGIC_Str8ToPtrNode *next;
  String8 key;
  U64 hash;
  void *ptr;
};

typedef struct RADDBGIC_Str8ToPtrMap RADDBGIC_Str8ToPtrMap;
struct RADDBGIC_Str8ToPtrMap
{
  RADDBGIC_Str8ToPtrNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 pair_count;
};

//- rjf: sortable range data structure

typedef struct RADDBGIC_SortKey RADDBGIC_SortKey;
struct RADDBGIC_SortKey
{
  U64 key;
  void *val;
};

typedef struct RADDBGIC_OrderedRange RADDBGIC_OrderedRange;
struct RADDBGIC_OrderedRange
{
  RADDBGIC_OrderedRange *next;
  U64 first;
  U64 opl;
};

////////////////////////////////
//~ rjf: Binary Section Types

typedef struct RADDBGIC_BinarySection RADDBGIC_BinarySection;
struct RADDBGIC_BinarySection
{
  RADDBGIC_BinarySection *next;
  String8 name;
  RADDBGI_BinarySectionFlags flags;
  U64 voff_first;
  U64 voff_opl;
  U64 foff_first;
  U64 foff_opl;
};

////////////////////////////////
//~ rjf: Per-Compilation-Unit Info Types

typedef struct RADDBGIC_LineSequence RADDBGIC_LineSequence;
struct RADDBGIC_LineSequence
{
  String8 file_name;
  U64 *voffs;     // [line_count + 1] (sorted)
  U32 *line_nums; // [line_count]
  U16 *col_nums;  // [2*line_count]
  U64 line_count;
};

typedef struct RADDBGIC_LineSequenceNode RADDBGIC_LineSequenceNode;
struct RADDBGIC_LineSequenceNode
{
  RADDBGIC_LineSequenceNode *next;
  RADDBGIC_LineSequence line_seq;
};

typedef struct RADDBGIC_UnitInfo RADDBGIC_UnitInfo;
struct RADDBGIC_UnitInfo
{
  String8 unit_name;
  String8 compiler_name;
  String8 source_file;
  String8 object_file;
  String8 archive_file;
  String8 build_path;
  RADDBGI_Language language;
};

typedef struct RADDBGIC_Unit RADDBGIC_Unit;
struct RADDBGIC_Unit
{
  RADDBGIC_Unit *next_order;
  U32 idx;
  B32 info_is_set;
  String8 unit_name;
  String8 compiler_name;
  String8 source_file;
  String8 object_file;
  String8 archive_file;
  String8 build_path;
  RADDBGI_Language language;
  RADDBGIC_LineSequenceNode *line_seq_first;
  RADDBGIC_LineSequenceNode *line_seq_last;
  U64 line_seq_count;
};

typedef struct RADDBGIC_UnitVMapRange RADDBGIC_UnitVMapRange;
struct RADDBGIC_UnitVMapRange
{
  RADDBGIC_UnitVMapRange *next;
  RADDBGIC_Unit *unit;
  U64 first;
  U64 opl;
};

////////////////////////////////
//~ rjf: Type Info Types

typedef U8 RADDBGIC_TypeConstructKind;
enum
{
  RADDBGIC_TypeConstructKind_Basic,
  RADDBGIC_TypeConstructKind_Modifier,
  RADDBGIC_TypeConstructKind_Bitfield,
  RADDBGIC_TypeConstructKind_Pointer,
  RADDBGIC_TypeConstructKind_Array,
  RADDBGIC_TypeConstructKind_Procedure,
  RADDBGIC_TypeConstructKind_Method,
};

typedef struct RADDBGIC_Reservation RADDBGIC_Reservation;

typedef struct RADDBGIC_TypeMember RADDBGIC_TypeMember;
struct RADDBGIC_TypeMember
{
  RADDBGIC_TypeMember *next;
  RADDBGI_MemberKind kind;
  String8 name;
  struct RADDBGIC_Type *type;
  U32 off;
};

typedef struct RADDBGIC_TypeEnumVal RADDBGIC_TypeEnumVal;
struct RADDBGIC_TypeEnumVal
{
  RADDBGIC_TypeEnumVal *next;
  String8 name;
  U64 val;
};

typedef struct RADDBGIC_Type RADDBGIC_Type;
struct RADDBGIC_Type
{
  RADDBGIC_Type *next_order;
  RADDBGI_TypeKind kind;
  U32 idx;
  U32 byte_size;
  U32 flags;
  U32 off;
  U32 count;
  String8 name;
  RADDBGIC_Type *direct_type;
  RADDBGIC_Type **param_types;
  struct RADDBGIC_TypeUDT *udt;
};

typedef struct RADDBGIC_TypeUDT RADDBGIC_TypeUDT;
struct RADDBGIC_TypeUDT
{
  RADDBGIC_TypeUDT *next_order;
  U32 idx;
  RADDBGIC_Type *self_type;
  RADDBGIC_TypeMember *first_member;
  RADDBGIC_TypeMember *last_member;
  U64 member_count;
  RADDBGIC_TypeEnumVal *first_enum_val;
  RADDBGIC_TypeEnumVal *last_enum_val;
  U64 enum_val_count;
  String8 source_path;
  U32 line;
  U32 col;
};

typedef struct RADDBGIC_TypeNode RADDBGIC_TypeNode;
struct RADDBGIC_TypeNode
{
  RADDBGIC_TypeNode *next;
  RADDBGIC_Type *type;
};

typedef struct RADDBGIC_TypeList RADDBGIC_TypeList;
struct RADDBGIC_TypeList
{
  RADDBGIC_TypeNode *first;
  RADDBGIC_TypeNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Symbol Info Types

typedef enum RADDBGIC_SymbolKind
{
  RADDBGIC_SymbolKind_NULL,
  RADDBGIC_SymbolKind_GlobalVariable,
  RADDBGIC_SymbolKind_ThreadVariable,
  RADDBGIC_SymbolKind_Procedure,
  RADDBGIC_SymbolKind_COUNT
}
RADDBGIC_SymbolKind;

typedef struct RADDBGIC_SymbolInfo RADDBGIC_SymbolInfo;
struct RADDBGIC_SymbolInfo
{
  RADDBGIC_SymbolKind kind;
  String8 name;
  String8 link_name;
  RADDBGIC_Type *type;
  B32 is_extern;
  U64 offset;
  // TODO(allen): should this actually be "container scope"?
  struct RADDBGIC_Symbol *container_symbol;
  RADDBGIC_Type *container_type;
  struct RADDBGIC_Scope *root_scope;
};

typedef struct RADDBGIC_Symbol RADDBGIC_Symbol;
struct RADDBGIC_Symbol
{
  RADDBGIC_Symbol *next_order;
  U32 idx;
  RADDBGIC_SymbolKind kind;
  String8 name;
  String8 link_name;
  RADDBGIC_Type *type;
  B32 is_extern;
  B8 offset_is_set;
  U64 offset;
  RADDBGIC_Symbol *container_symbol;
  RADDBGIC_Type *container_type;
  struct RADDBGIC_Scope *root_scope;
};

////////////////////////////////
//~ rjf: Scope Info Types

typedef struct RADDBGIC_LocalInfo RADDBGIC_LocalInfo;
struct RADDBGIC_LocalInfo
{
  RADDBGI_LocalKind kind;
  struct RADDBGIC_Scope *scope;
  String8 name;
  RADDBGIC_Type *type;
};

typedef struct RADDBGIC_Local RADDBGIC_Local;
struct RADDBGIC_Local
{
  RADDBGIC_Local *next;
  RADDBGI_LocalKind kind;
  String8 name;
  RADDBGIC_Type *type;
  struct RADDBGIC_LocationSet *locset;
};

typedef struct RADDBGIC_VOffRange RADDBGIC_VOffRange;
struct RADDBGIC_VOffRange
{
  RADDBGIC_VOffRange *next;
  U64 voff_first;
  U64 voff_opl;
};

typedef struct RADDBGIC_Scope RADDBGIC_Scope;
struct RADDBGIC_Scope
{
  RADDBGIC_Scope *next_order;
  RADDBGIC_Symbol *symbol;
  RADDBGIC_Scope *parent_scope;
  RADDBGIC_Scope *first_child;
  RADDBGIC_Scope *last_child;
  RADDBGIC_Scope *next_sibling;
  U64 voff_base;
  RADDBGIC_VOffRange *first_range;
  RADDBGIC_VOffRange *last_range;
  U32 range_count;
  U32 idx;
  RADDBGIC_Local *first_local;
  RADDBGIC_Local *last_local;
  U32 local_count;
};

////////////////////////////////
//~ rjf: Location Info Types

typedef struct RADDBGIC_EvalBytecodeOp RADDBGIC_EvalBytecodeOp;
struct RADDBGIC_EvalBytecodeOp
{
  RADDBGIC_EvalBytecodeOp *next;
  RADDBGI_EvalOp op;
  U32 p_size;
  U64 p;
};

typedef struct RADDBGIC_EvalBytecode RADDBGIC_EvalBytecode;
struct RADDBGIC_EvalBytecode
{
  RADDBGIC_EvalBytecodeOp *first_op;
  RADDBGIC_EvalBytecodeOp *last_op;
  U32 op_count;
  U32 encoded_size;
};

typedef struct RADDBGIC_Location RADDBGIC_Location;
struct RADDBGIC_Location
{
  RADDBGI_LocationKind kind;
  U8 register_code;
  U16 offset;
  RADDBGIC_EvalBytecode bytecode;
};

typedef struct RADDBGIC_LocationCase RADDBGIC_LocationCase;
struct RADDBGIC_LocationCase
{
  RADDBGIC_LocationCase *next;
  U64 voff_first;
  U64 voff_opl;
  RADDBGIC_Location *location;
};

typedef struct RADDBGIC_LocationSet RADDBGIC_LocationSet;
struct RADDBGIC_LocationSet
{
  RADDBGIC_LocationCase *first_location_case;
  RADDBGIC_LocationCase *last_location_case;
  U64 location_case_count;
};

////////////////////////////////
//~ rjf: Name Map Types

typedef struct RADDBGIC_NameMapIdxNode RADDBGIC_NameMapIdxNode;
struct RADDBGIC_NameMapIdxNode
{
  RADDBGIC_NameMapIdxNode *next;
  U32 idx[8];
};

typedef struct RADDBGIC_NameMapNode RADDBGIC_NameMapNode;
struct RADDBGIC_NameMapNode
{
  RADDBGIC_NameMapNode *bucket_next;
  RADDBGIC_NameMapNode *order_next;
  String8 string;
  RADDBGIC_NameMapIdxNode *idx_first;
  RADDBGIC_NameMapIdxNode *idx_last;
  U64 idx_count;
};

typedef struct RADDBGIC_NameMap RADDBGIC_NameMap;
struct RADDBGIC_NameMap
{
  RADDBGIC_NameMapNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  RADDBGIC_NameMapNode *first;
  RADDBGIC_NameMapNode *last;
  U64 name_count;
};

////////////////////////////////
//~ rjf: Top-Level Debug Info Types

typedef struct RADDBGIC_TopLevelInfo RADDBGIC_TopLevelInfo;
struct RADDBGIC_TopLevelInfo
{
  RADDBGI_Arch architecture;
  String8 exe_name;
  U64 exe_hash;
  U64 voff_max;
};

////////////////////////////////
//~ rjf: Root Construction Bundle Types

typedef struct RADDBGIC_RootParams RADDBGIC_RootParams;
struct RADDBGIC_RootParams
{
  U64 addr_size;
  U32 bucket_count_units;              // optional; default chosen if 0
  U32 bucket_count_symbols;            // optional; default chosen if 0
  U32 bucket_count_scopes;             // optional; default chosen if 0
  U32 bucket_count_locals;             // optional; default chosen if 0
  U32 bucket_count_types;              // optional; default chosen if 0
  U64 bucket_count_type_constructs;    // optional; default chosen if 0
};

typedef struct RADDBGIC_Root RADDBGIC_Root;
struct RADDBGIC_Root
{
  Arena *arena;
  RADDBGIC_ErrorList errors;
  
  //////// Contextual Information
  
  U64 addr_size;
  
  //////// Info Declared By User
  
  // top level info
  B32 top_level_info_is_set;
  RADDBGIC_TopLevelInfo top_level_info;
  
  // binary layout
  RADDBGIC_BinarySection *binary_section_first;
  RADDBGIC_BinarySection *binary_section_last;
  U64 binary_section_count;
  
  // compilation units
  RADDBGIC_Unit *unit_first;
  RADDBGIC_Unit *unit_last;
  U64 unit_count;
  
  RADDBGIC_UnitVMapRange *unit_vmap_range_first;
  RADDBGIC_UnitVMapRange *unit_vmap_range_last;
  U64 unit_vmap_range_count;
  
  // types
  RADDBGIC_Type *first_type;
  RADDBGIC_Type *last_type;
  U64 type_count;
  
  RADDBGIC_Type *nil_type;
  RADDBGIC_Type *variadic_type;
  
  RADDBGIC_Type handled_nil_type;
  
  RADDBGIC_TypeUDT *first_udt;
  RADDBGIC_TypeUDT *last_udt;
  U64 type_udt_count;
  
  U64 total_member_count;
  U64 total_enum_val_count;
  
  // symbols
  RADDBGIC_Symbol *first_symbol;
  RADDBGIC_Symbol *last_symbol;
  union{
    U64 symbol_count;
    U64 symbol_kind_counts[RADDBGIC_SymbolKind_COUNT];
  };
  
  RADDBGIC_Scope *first_scope;
  RADDBGIC_Scope *last_scope;
  U64 scope_count;
  U64 scope_voff_count;
  
  RADDBGIC_Local *first_local;
  RADDBGIC_Local *last_local;
  U64 local_count;
  U64 location_count;
  
  // name maps
  RADDBGIC_NameMap *name_maps[RADDBGI_NameMapKind_COUNT];
  
  //////// Handle Relationship Maps
  
  RADDBGIC_U64ToPtrMap unit_map;
  RADDBGIC_U64ToPtrMap symbol_map;
  RADDBGIC_U64ToPtrMap scope_map;
  RADDBGIC_U64ToPtrMap local_map;
  RADDBGIC_U64ToPtrMap type_from_id_map;
  RADDBGIC_Str8ToPtrMap construct_map;
};

////////////////////////////////
//~ rjf: Baking Phase Types

//- rjf: bake data section data structure

typedef struct RADDBGIC_DSectionNode RADDBGIC_DSectionNode;
struct RADDBGIC_DSectionNode
{
  RADDBGIC_DSectionNode *next;
  void *data;
  U64 size;
  RADDBGI_DataSectionTag tag;
};

typedef struct RADDBGIC_DSections RADDBGIC_DSections;
struct RADDBGIC_DSections
{
  RADDBGIC_DSectionNode *first;
  RADDBGIC_DSectionNode *last;
  U32 count;
};

//- rjf: bake string data structure

typedef struct RADDBGIC_StringNode RADDBGIC_StringNode;
struct RADDBGIC_StringNode
{
  RADDBGIC_StringNode *order_next;
  RADDBGIC_StringNode *bucket_next;
  String8 str;
  U64 hash;
  U32 idx;
};

typedef struct RADDBGIC_Strings RADDBGIC_Strings;
struct RADDBGIC_Strings
{
  RADDBGIC_StringNode *order_first;
  RADDBGIC_StringNode *order_last;
  RADDBGIC_StringNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U32 count;
};

//- rjf: index run baking data structure

typedef struct RADDBGIC_IdxRunNode RADDBGIC_IdxRunNode;
struct RADDBGIC_IdxRunNode
{
  RADDBGIC_IdxRunNode *order_next;
  RADDBGIC_IdxRunNode *bucket_next;
  U32 *idx_run;
  U64 hash;
  U32 count;
  U32 first_idx;
};

typedef struct RADDBGIC_IdxRuns RADDBGIC_IdxRuns;
struct RADDBGIC_IdxRuns
{
  RADDBGIC_IdxRunNode *order_first;
  RADDBGIC_IdxRunNode *order_last;
  RADDBGIC_IdxRunNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U32 count;
  U32 idx_count;
};

//- rjf: source file & file path baking data structures

typedef struct RADDBGIC_PathNode RADDBGIC_PathNode;
struct RADDBGIC_PathNode
{
  RADDBGIC_PathNode *next_order;
  RADDBGIC_PathNode *parent;
  RADDBGIC_PathNode *first_child;
  RADDBGIC_PathNode *last_child;
  RADDBGIC_PathNode *next_sibling;
  String8 name;
  struct RADDBGIC_SrcNode *src_file;
  U32 idx;
};

typedef struct RADDBGIC_LineMapFragment RADDBGIC_LineMapFragment;
struct RADDBGIC_LineMapFragment
{
  RADDBGIC_LineMapFragment *next;
  RADDBGIC_LineSequenceNode *sequence;
};

typedef struct RADDBGIC_SrcNode RADDBGIC_SrcNode;
struct RADDBGIC_SrcNode
{
  RADDBGIC_SrcNode *next;
  RADDBGIC_PathNode *path_node;
  U32 idx;
  
  String8 normal_full_path;
  
  // place to gather the line info attached to this src file
  RADDBGIC_LineMapFragment *first_fragment;
  RADDBGIC_LineMapFragment *last_fragment;
  
  // place to put the final baked version of this file's line map
  U32 line_map_nums_data_idx;
  U32 line_map_range_data_idx;
  U32 line_map_count;
  U32 line_map_voff_data_idx;
};

typedef struct RADDBGIC_PathTree RADDBGIC_PathTree;
struct RADDBGIC_PathTree
{
  RADDBGIC_PathNode *first;
  RADDBGIC_PathNode *last;
  U32 count;
  RADDBGIC_PathNode root;
  RADDBGIC_SrcNode *src_first;
  RADDBGIC_SrcNode *src_last;
  U32 src_count;
};

//- rjf: line info baking data structures

typedef struct RADDBGIC_LineRec RADDBGIC_LineRec;
struct RADDBGIC_LineRec
{
  U32 file_id;
  U32 line_num;
  U16 col_first;
  U16 col_opl;
};

typedef struct RADDBGIC_UnitLinesCombined RADDBGIC_UnitLinesCombined;
struct RADDBGIC_UnitLinesCombined
{
  U64 *voffs;
  RADDBGI_Line *lines;
  U16 *cols;
  U32 line_count;
};

typedef struct RADDBGIC_SrcLinesCombined RADDBGIC_SrcLinesCombined;
struct RADDBGIC_SrcLinesCombined
{
  U32 *line_nums;
  U32 *line_ranges;
  U64 *voffs;
  U32  line_count;
  U32  voff_count;
};

typedef struct RADDBGIC_SrcLineMapVoffBlock RADDBGIC_SrcLineMapVoffBlock;
struct RADDBGIC_SrcLineMapVoffBlock
{
  RADDBGIC_SrcLineMapVoffBlock *next;
  U64 voff;
};

typedef struct RADDBGIC_SrcLineMapBucket RADDBGIC_SrcLineMapBucket;
struct RADDBGIC_SrcLineMapBucket
{
  RADDBGIC_SrcLineMapBucket *order_next;
  RADDBGIC_SrcLineMapBucket *hash_next;
  U32 line_num;
  RADDBGIC_SrcLineMapVoffBlock *first_voff_block;
  RADDBGIC_SrcLineMapVoffBlock *last_voff_block;
  U64 voff_count;
};

//- rjf: vmap baking data structure 

typedef struct RADDBGIC_VMap RADDBGIC_VMap;
struct RADDBGIC_VMap
{
  RADDBGI_VMapEntry *vmap; // [count + 1]
  U32 count;
};

typedef struct RADDBGIC_VMapMarker RADDBGIC_VMapMarker;
struct RADDBGIC_VMapMarker
{
  U32 idx;
  U32 begin_range;
};

typedef struct RADDBGIC_VMapRangeTracker RADDBGIC_VMapRangeTracker;
struct RADDBGIC_VMapRangeTracker
{
  RADDBGIC_VMapRangeTracker *next;
  U32 idx;
};

//- rjf: type data baking types

typedef struct RADDBGIC_TypeData RADDBGIC_TypeData;
struct RADDBGIC_TypeData
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

typedef struct RADDBGIC_SymbolData RADDBGIC_SymbolData;
struct RADDBGIC_SymbolData
{
  RADDBGI_GlobalVariable *global_variables;
  U32 global_variable_count;
  
  RADDBGIC_VMap *global_vmap;
  
  RADDBGI_ThreadVariable *thread_variables;
  U32 thread_variable_count;
  
  RADDBGI_Procedure *procedures;
  U32 procedure_count;
  
  RADDBGI_Scope *scopes;
  U32 scope_count;
  
  U64 *scope_voffs;
  U32 scope_voff_count;
  
  RADDBGIC_VMap *scope_vmap;
  
  RADDBGI_Local *locals;
  U32 local_count;
  
  RADDBGI_LocationBlock *location_blocks;
  U32 location_block_count;
  
  void *location_data;
  U32 location_data_size;
};

//- rjf: name map baking types

typedef struct RADDBGIC_NameMapSemiNode RADDBGIC_NameMapSemiNode;
struct RADDBGIC_NameMapSemiNode
{
  RADDBGIC_NameMapSemiNode *next;
  RADDBGIC_NameMapNode *node;
};

typedef struct RADDBGIC_NameMapSemiBucket RADDBGIC_NameMapSemiBucket;
struct RADDBGIC_NameMapSemiBucket
{
  RADDBGIC_NameMapSemiNode *first;
  RADDBGIC_NameMapSemiNode *last;
  U64 count;
};

typedef struct RADDBGIC_NameMapBaked RADDBGIC_NameMapBaked;
struct RADDBGIC_NameMapBaked
{
  RADDBGI_NameMapBucket *buckets;
  RADDBGI_NameMapNode *nodes;
  U32 bucket_count;
  U32 node_count;
};

//- rjf: bundle baking context type

typedef struct RADDBGIC_BakeParams RADDBGIC_BakeParams;
struct RADDBGIC_BakeParams
{
  U64 strings_bucket_count;
  U64 idx_runs_bucket_count;
};

typedef struct RADDBGIC_BakeCtx RADDBGIC_BakeCtx;
struct RADDBGIC_BakeCtx
{
  Arena *arena;
  RADDBGIC_Strings strs;
  RADDBGIC_IdxRuns idxs;
  RADDBGIC_PathTree *tree;
};

////////////////////////////////
//~ rjf: Basic Type Helpers

//- rjf: type lists
static void raddbgic_type_list_push(Arena *arena, RADDBGIC_TypeList *list, RADDBGIC_Type *type);

//- rjf: bytecode lists
static void raddbgic_bytecode_push_op(Arena *arena, RADDBGIC_EvalBytecode *bytecode, RADDBGI_EvalOp op, U64 p);
static void raddbgic_bytecode_push_uconst(Arena *arena, RADDBGIC_EvalBytecode *bytecode, U64 x);
static void raddbgic_bytecode_push_sconst(Arena *arena, RADDBGIC_EvalBytecode *bytecode, S64 x);
static void raddbgic_bytecode_concat_in_place(RADDBGIC_EvalBytecode *left_dst, RADDBGIC_EvalBytecode *right_destroyed);

//- rjf: sortable range sorting
static RADDBGIC_SortKey* raddbgic_sort_key_array(Arena *arena, RADDBGIC_SortKey *keys, U64 count);

////////////////////////////////
//~ rjf: Auxiliary Data Structure Functions

//- rjf: u64 -> ptr map
static void raddbgic_u64toptr_init(Arena *arena, RADDBGIC_U64ToPtrMap *map, U64 bucket_count);
static void raddbgic_u64toptr_lookup(RADDBGIC_U64ToPtrMap *map, U64 key, U64 hash, RADDBGIC_U64ToPtrLookup *lookup_out);
static void raddbgic_u64toptr_insert(Arena *arena, RADDBGIC_U64ToPtrMap *map, U64 key, U64 hash, RADDBGIC_U64ToPtrLookup *lookup, void *ptr);

//- rjf: string8 -> ptr map
static void raddbgic_str8toptr_init(Arena *arena, RADDBGIC_Str8ToPtrMap *map, U64 bucket_count);
static void*raddbgic_str8toptr_lookup(RADDBGIC_Str8ToPtrMap *map, String8 key, U64 hash);
static void raddbgic_str8toptr_insert(Arena *arena, RADDBGIC_Str8ToPtrMap *map, String8 key, U64 hash, void *ptr);

////////////////////////////////
//~ rjf: Loose Debug Info Construction (Anything -> Loose) Functions

//- rjf: root creation
static RADDBGIC_Root* raddbgic_root_new(RADDBGIC_RootParams *params);
static void       raddbgic_root_release(RADDBGIC_Root *root);

//- rjf: error accumulation
static void raddbgic_error(RADDBGIC_Root *root, String8 string);
static void raddbgic_errorf(RADDBGIC_Root *root, char *fmt, ...);
static RADDBGIC_Error* raddbgic_get_first_error(RADDBGIC_Root *root);

//- rjf: top-level info specification
static void raddbgic_set_top_level_info(RADDBGIC_Root *root, RADDBGIC_TopLevelInfo *tli);

//- rjf: binary section building
static void raddbgic_add_binary_section(RADDBGIC_Root *root,
                                        String8 name, RADDBGI_BinarySectionFlags flags,
                                        U64 voff_first, U64 voff_opl, U64 foff_first,
                                        U64 foff_opl);

//- rjf: unit info building
static RADDBGIC_Unit* raddbgic_unit_handle_from_user_id(RADDBGIC_Root *root, U64 unit_user_id, U64 unit_user_id_hash);
static void raddbgic_unit_set_info(RADDBGIC_Root *root, RADDBGIC_Unit *unit, RADDBGIC_UnitInfo *info);
static void raddbgic_unit_add_line_sequence(RADDBGIC_Root *root, RADDBGIC_Unit *unit,
                                            RADDBGIC_LineSequence *line_sequence);
static void raddbgic_unit_vmap_add_range(RADDBGIC_Root *root, RADDBGIC_Unit *unit, U64 first, U64 opl);

//- rjf: type info lookups/reservations
static RADDBGIC_Type*        raddbgic_type_from_id(RADDBGIC_Root *root, U64 type_user_id, U64 type_user_id_hash);
static RADDBGIC_Reservation* raddbgic_type_reserve_id(RADDBGIC_Root *root, U64 type_user_id, U64 type_user_id_hash);
static void              raddbgic_type_fill_id(RADDBGIC_Root *root, RADDBGIC_Reservation *res, RADDBGIC_Type *type);

//- rjf: nil/singleton types
static B32        raddbgic_type_is_unhandled_nil(RADDBGIC_Root *root, RADDBGIC_Type *type);
static RADDBGIC_Type* raddbgic_type_handled_nil(RADDBGIC_Root *root);
static RADDBGIC_Type* raddbgic_type_nil(RADDBGIC_Root *root);
static RADDBGIC_Type* raddbgic_type_variadic(RADDBGIC_Root *root);

//- rjf: base type info constructors
static RADDBGIC_Type*    raddbgic_type_new(RADDBGIC_Root *root);
static RADDBGIC_TypeUDT* raddbgic_type_udt_from_any_type(RADDBGIC_Root *root, RADDBGIC_Type *type);
static RADDBGIC_TypeUDT* raddbgic_type_udt_from_record_type(RADDBGIC_Root *root, RADDBGIC_Type *type);

//- rjf: basic/operator type construction helpers
static RADDBGIC_Type* raddbgic_type_basic(RADDBGIC_Root *root, RADDBGI_TypeKind type_kind, String8 name);
static RADDBGIC_Type* raddbgic_type_modifier(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGI_TypeModifierFlags flags);
static RADDBGIC_Type* raddbgic_type_bitfield(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, U32 bit_off, U32 bit_count);
static RADDBGIC_Type* raddbgic_type_pointer(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGI_TypeKind ptr_type_kind);
static RADDBGIC_Type* raddbgic_type_array(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, U64 count);
static RADDBGIC_Type* raddbgic_type_proc(RADDBGIC_Root *root, RADDBGIC_Type *return_type, struct RADDBGIC_TypeList *params);
static RADDBGIC_Type* raddbgic_type_method(RADDBGIC_Root *root, RADDBGIC_Type *this_type, RADDBGIC_Type *return_type, struct RADDBGIC_TypeList *params);

//- rjf: udt type constructors
static RADDBGIC_Type* raddbgic_type_udt(RADDBGIC_Root *root, RADDBGI_TypeKind record_type_kind, String8 name, U64 size);
static RADDBGIC_Type* raddbgic_type_enum(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, String8 name);
static RADDBGIC_Type* raddbgic_type_alias(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, String8 name);
static RADDBGIC_Type* raddbgic_type_incomplete(RADDBGIC_Root *root, RADDBGI_TypeKind type_kind, String8 name);

//- rjf: type member building
static void raddbgic_type_add_member_data_field(RADDBGIC_Root *root, RADDBGIC_Type *record_type, String8 name, RADDBGIC_Type *mem_type, U32 off);
static void raddbgic_type_add_member_static_data(RADDBGIC_Root *root, RADDBGIC_Type *record_type, String8 name, RADDBGIC_Type *mem_type);
static void raddbgic_type_add_member_method(RADDBGIC_Root *root, RADDBGIC_Type *record_type, String8 name, RADDBGIC_Type *mem_type);
static void raddbgic_type_add_member_static_method(RADDBGIC_Root *root, RADDBGIC_Type *record_type, String8 name, RADDBGIC_Type *mem_type);
static void raddbgic_type_add_member_virtual_method(RADDBGIC_Root *root, RADDBGIC_Type *record_type, String8 name, RADDBGIC_Type *mem_type);
static void raddbgic_type_add_member_base(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_Type *base_type, U32 off);
static void raddbgic_type_add_member_virtual_base(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_Type *base_type, U32 vptr_off, U32 vtable_off);
static void raddbgic_type_add_member_nested_type(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_Type *nested_type);
static void raddbgic_type_add_enum_val(RADDBGIC_Root *root, RADDBGIC_Type *enum_type, String8 name, U64 val);

//- rjf: type source coordinate specifications
static void raddbgic_type_set_source_coordinates(RADDBGIC_Root *root, RADDBGIC_Type *defined_type, String8 source_path, U32 line, U32 col);

//- rjf: symbol info building
static RADDBGIC_Symbol* raddbgic_symbol_handle_from_user_id(RADDBGIC_Root *root, U64 symbol_user_id, U64 symbol_user_id_hash);
static void raddbgic_symbol_set_info(RADDBGIC_Root *root, RADDBGIC_Symbol *symbol, RADDBGIC_SymbolInfo *info);

//- rjf: scope info building
static RADDBGIC_Scope *raddbgic_scope_handle_from_user_id(RADDBGIC_Root *root, U64 scope_user_id, U64 scope_user_id_hash);
static void raddbgic_scope_set_parent(RADDBGIC_Root *root, RADDBGIC_Scope *scope, RADDBGIC_Scope *parent);
static void raddbgic_scope_add_voff_range(RADDBGIC_Root *root, RADDBGIC_Scope *scope, U64 voff_first, U64 voff_opl);
static void raddbgic_scope_recursive_set_symbol(RADDBGIC_Scope *scope, RADDBGIC_Symbol *symbol);

//- rjf: local info building
static RADDBGIC_Local* raddbgic_local_handle_from_user_id(RADDBGIC_Root *root, U64 local_user_id, U64 local_user_id_hash);
static void raddbgic_local_set_basic_info(RADDBGIC_Root *root, RADDBGIC_Local *local, RADDBGIC_LocalInfo *info);
static RADDBGIC_LocationSet* raddbgic_location_set_from_local(RADDBGIC_Root *root, RADDBGIC_Local *local);

//- rjf: location info building
static void raddbgic_location_set_add_case(RADDBGIC_Root *root, RADDBGIC_LocationSet *locset, U64 voff_first, U64 voff_opl, RADDBGIC_Location *location);
static RADDBGIC_Location* raddbgic_location_addr_bytecode_stream(RADDBGIC_Root *root, struct RADDBGIC_EvalBytecode *bytecode);
static RADDBGIC_Location* raddbgic_location_val_bytecode_stream(RADDBGIC_Root *root, struct RADDBGIC_EvalBytecode *bytecode);
static RADDBGIC_Location* raddbgic_location_addr_reg_plus_u16(RADDBGIC_Root *root, U8 reg_code, U16 offset);
static RADDBGIC_Location* raddbgic_location_addr_addr_reg_plus_u16(RADDBGIC_Root *root, U8 reg_code, U16 offset);
static RADDBGIC_Location* raddbgic_location_val_reg(RADDBGIC_Root *root, U8 reg_code);

//- rjf: name map building
static RADDBGIC_NameMap* raddbgic_name_map_for_kind(RADDBGIC_Root *root, RADDBGI_NameMapKind kind);
static void          raddbgic_name_map_add_pair(RADDBGIC_Root *root, RADDBGIC_NameMap *map, String8 name, U32 idx);

////////////////////////////////
//~ rjf: Debug Info Baking (Loose -> Tight) Functions

//- rjf: bake context construction
static RADDBGIC_BakeCtx* raddbgic_bake_ctx_begin(RADDBGIC_BakeParams *params);
static void          raddbgic_bake_ctx_release(RADDBGIC_BakeCtx *bake_ctx);

//- rjf: string baking
static U32 raddbgic_string(RADDBGIC_BakeCtx *bctx, String8 str);

//- rjf: idx run baking
static U64 raddbgic_idx_run_hash(U32 *idx_run, U32 count);
static U32 raddbgic_idx_run(RADDBGIC_BakeCtx *bctx, U32 *idx_run, U32 count);

//- rjf: data section baking
static U32 raddbgic_dsection(Arena *arena, RADDBGIC_DSections *dss, void *data, U64 size, RADDBGI_DataSectionTag tag);

//- rjf: paths baking
static String8 raddbgic_normal_string_from_path_node(Arena *arena, RADDBGIC_PathNode *node);
static void    raddbgic_normal_string_from_path_node_build(Arena *arena, RADDBGIC_PathNode *node, String8List *out);
static RADDBGIC_PathNode* raddbgic_paths_new_node(RADDBGIC_BakeCtx *bctx);
static RADDBGIC_PathNode* raddbgic_paths_sub_path(RADDBGIC_BakeCtx *bctx, RADDBGIC_PathNode *dir, String8 sub_dir);
static RADDBGIC_PathNode* raddbgic_paths_node_from_path(RADDBGIC_BakeCtx *bctx,  String8 path);
static U32            raddbgic_paths_idx_from_path(RADDBGIC_BakeCtx *bctx, String8 path);
static RADDBGIC_SrcNode*  raddbgic_paths_new_src_node(RADDBGIC_BakeCtx *bctx);
static RADDBGIC_SrcNode*  raddbgic_paths_src_node_from_path_node(RADDBGIC_BakeCtx *bctx, RADDBGIC_PathNode *path_node);

//- rjf: per-unit line info baking
static RADDBGIC_UnitLinesCombined* raddbgic_unit_combine_lines(Arena *arena, RADDBGIC_BakeCtx *bctx, RADDBGIC_LineSequenceNode *first_seq);

//- rjf: per-src line info baking
static RADDBGIC_SrcLinesCombined* raddbgic_source_combine_lines(Arena *arena, RADDBGIC_LineMapFragment *first);

//- rjf: vmap baking
static RADDBGIC_VMap* raddbgic_vmap_from_markers(Arena *arena, RADDBGIC_VMapMarker *markers, RADDBGIC_SortKey *keys, U64 marker_count);
static RADDBGIC_VMap* raddbgic_vmap_from_unit_ranges(Arena *arena, RADDBGIC_UnitVMapRange *first, U64 count);

//- rjf: type info baking
static U32* raddbgic_idx_run_from_types(Arena *arena, RADDBGIC_Type **types, U32 count);
static RADDBGIC_TypeData* raddbgic_type_data_combine(Arena *arena, RADDBGIC_Root *root, RADDBGIC_BakeCtx *bctx);

//- rjf: symbol data baking
static RADDBGIC_SymbolData* raddbgic_symbol_data_combine(Arena *arena, RADDBGIC_Root *root, RADDBGIC_BakeCtx *bctx);

//- rjf: name map baking
static RADDBGIC_NameMapBaked* raddbgic_name_map_bake(Arena *arena, RADDBGIC_Root *root, RADDBGIC_BakeCtx *bctx, RADDBGIC_NameMap *map);

//- rjf: top-level baking entry point
static void raddbgic_bake_file(Arena *arena, RADDBGIC_Root *root, String8List *out);

#endif // RADDBGI_CONS_H
