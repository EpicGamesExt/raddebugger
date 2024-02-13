// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_CONS_H
#define RADDBGI_CONS_H

////////////////////////////////
//~ rjf: Overrideable Memory Operations

// To override the slow/default memset implementation used by the library,
// do the following:
//
// #define RADDBGIC_MEMSET_OVERRIDE
// #define raddbgic_memset <name of memset implementation>

#if !defined(raddbgic_memset)
# define raddbgic_memset raddbgic_memset_fallback
#endif

// To override the slow/default memcpy implementation used by the library,
// do the following:
//
// #define RADDBGIC_MEMCPY_OVERRIDE
// #define raddbgic_memcpy <name of memcpy implementation>

#if !defined(raddbgic_memset)
# define raddbgic_memset raddbgic_memset_fallback
#endif

#if !defined(raddbgic_memcpy)
# define raddbgic_memcpy raddbgic_memcpy_fallback
#endif

////////////////////////////////
//~ rjf: Overrideable String View Types

// To override the string view type used by the library, do the following:
//
// #define RADDBGIC_STRING8_OVERRIDE
// #define RADDBGIC_String8 <name of your string type here>
// #define RADDBGIC_String8_BaseMember <name of base pointer member>
// #define RADDBGIC_String8_SizeMember <name of size member>

#if !defined(RADDBGIC_String8)
#define RADDBGIC_String8_BaseMember str
#define RADDBGIC_String8_SizeMember size
typedef struct RADDBGIC_String8 RADDBGIC_String8;
struct RADDBGIC_String8
{
  RADDBGI_U8 *RADDBGIC_String8_BaseMember;
  RADDBGI_U64 RADDBGIC_String8_SizeMember;
};
#endif

#if !defined(RADDBGIC_String8Node)
typedef struct RADDBGIC_String8Node RADDBGIC_String8Node;
struct RADDBGIC_String8Node
{
  RADDBGIC_String8Node *next;
  RADDBGIC_String8 string;
};
#endif

#if !defined(RADDBGIC_String8List)
typedef struct RADDBGIC_String8List RADDBGIC_String8List;
struct RADDBGIC_String8List
{
  RADDBGIC_String8Node *first;
  RADDBGIC_String8Node *last;
  RADDBGI_U64 node_count;
  RADDBGI_U64 total_size;
};
#endif

////////////////////////////////
//~ rjf: Overrideable Arena Allocator Types

// To override the arena allocator type used by the library, do the following:
//
// #define RADDBGIC_ARENA_OVERRIDE
// #define RADDBGIC_Arena <name of your arena type here>
// #define raddbgic_arena_alloc   <name of your creation function - must be (void) -> Arena*>
// #define raddbgic_arena_release <name of your release function  - must be (Arena*) -> void>
// #define raddbgic_arena_pos     <name of your position function - must be (Arena*) -> U64>
// #define raddbgic_arena_push    <name of your pushing function  - must be (Arena*, U64 size) -> void*>
// #define raddbgic_arena_pop_to  <name of your popping function  - must be (Arena*, U64 pos) -> void>

#if !defined(RADDBGIC_Arena)
# define RADDBGIC_Arena RADDBGIC_Arena 
typedef struct RADDBGIC_Arena RADDBGIC_Arena;
struct RADDBGIC_Arena
{
  RADDBGIC_Arena *prev;
  RADDBGIC_Arena *current;
  RADDBGI_U64 base_pos;
  RADDBGI_U64 pos;
  RADDBGI_U64 cmt;
  RADDBGI_U64 res;
  RADDBGI_U64 align;
  RADDBGI_S8 grow;
};
#endif

#if !defined(raddbgic_arena_alloc)
# define raddbgic_arena_alloc raddbgic_arena_alloc_fallback
#endif
#if !defined(raddbgic_arena_release)
# define raddbgic_arena_release raddbgic_arena_release_fallback
#endif
#if !defined(raddbgic_arena_pos)
# define raddbgic_arena_pos raddbgic_arena_pos_fallback
#endif
#if !defined(raddbgic_arena_push)
# define raddbgic_arena_push raddbgic_arena_push_fallback
#endif

////////////////////////////////
//~ rjf: Overrideable Thread-Local Scratch Arenas

// To override the default thread-local scratch arenas used b yhe library,
// do the following:
//
// #define RADDBGIC_SCRATCH_OVERRIDE
// #define RADDBGIC_Temp <name of arena temp block type - generally struct: (Arena*, U64)
// #define raddbgic_temp_arena <name of temp -> arena implementation - must be (Temp) -> (Arena*)>
// #define raddbgic_scratch_begin <name of scratch begin implementation - must be (Arena **conflicts, U64 conflict_count) -> Temp>
// #define raddbgic_scratch_end <name of scratch end function - must be (Temp) -> void

#if !defined(RADDBGIC_Temp)
# define RADDBGIC_Temp RADDBGIC_Temp
typedef struct RADDBGIC_Temp RADDBGIC_Temp;
struct RADDBGIC_Temp
{
  RADDBGIC_Arena *arena;
  RADDBGI_U64 pos;
};
#define raddbgic_temp_arena(t) ((t).arena)
#endif

#if !defined(raddbgic_scratch_begin)
# define raddbgic_scratch_begin raddbgic_scratch_begin_fallback
#endif
#if !defined(raddbgic_scratch_end)
# define raddbgic_scratch_end raddbgic_scratch_end_fallback
#endif

////////////////////////////////
//~ rjf: Linked List Helpers

#define RADDBGIC_CheckNil(nil,p) ((p) == 0 || (p) == nil)
#define RADDBGIC_SetNil(nil,p) ((p) = nil)

//- rjf: Base Doubly-Linked-List Macros
#define RADDBGIC_DLLInsert_NPZ(nil,f,l,p,n,next,prev) (RADDBGIC_CheckNil(nil,f) ? \
((f) = (l) = (n), RADDBGIC_SetNil(nil,(n)->next), RADDBGIC_SetNil(nil,(n)->prev)) :\
RADDBGIC_CheckNil(nil,p) ? \
((n)->next = (f), (f)->prev = (n), (f) = (n), RADDBGIC_SetNil(nil,(n)->prev)) :\
((p)==(l)) ? \
((l)->next = (n), (n)->prev = (l), (l) = (n), RADDBGIC_SetNil(nil, (n)->next)) :\
(((!RADDBGIC_CheckNil(nil,p) && RADDBGIC_CheckNil(nil,(p)->next)) ? (0) : ((p)->next->prev = (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define RADDBGIC_DLLPushBack_NPZ(nil,f,l,n,next,prev) RADDBGIC_DLLInsert_NPZ(nil,f,l,l,n,next,prev)
#define RADDBGIC_DLLPushFront_NPZ(nil,f,l,n,next,prev) RADDBGIC_DLLInsert_NPZ(nil,l,f,f,n,prev,next)
#define RADDBGIC_DLLRemove_NPZ(nil,f,l,n,next,prev) (((n) == (f) ? (f) = (n)->next : (0)),\
((n) == (l) ? (l) = (l)->prev : (0)),\
(RADDBGIC_CheckNil(nil,(n)->prev) ? (0) :\
((n)->prev->next = (n)->next)),\
(RADDBGIC_CheckNil(nil,(n)->next) ? (0) :\
((n)->next->prev = (n)->prev)))

//- rjf: Base Singly-Linked-List Queue Macros
#define RADDBGIC_SLLQueuePush_NZ(nil,f,l,n,next) (RADDBGIC_CheckNil(nil,f)?\
((f)=(l)=(n),RADDBGIC_SetNil(nil,(n)->next)):\
((l)->next=(n),(l)=(n),RADDBGIC_SetNil(nil,(n)->next)))
#define RADDBGIC_SLLQueuePushFront_NZ(nil,f,l,n,next) (RADDBGIC_CheckNil(nil,f)?\
((f)=(l)=(n),RADDBGIC_SetNil(nil,(n)->next)):\
((n)->next=(f),(f)=(n)))
#define RADDBGIC_SLLQueuePop_NZ(nil,f,l,next) ((f)==(l)?\
(RADDBGIC_SetNil(nil,f), RADDBGIC_SetNil(nil,l)):\
((f)=(f)->next))

//- rjf: Base Singly-Linked-List Stack Macros
#define RADDBGIC_SLLStackPush_N(f,n,next) ((n)->next=(f), (f)=(n))
#define RADDBGIC_SLLStackPop_N(f,next) ((f)=(f)->next)

////////////////////////////////
//~ rjf: Convenience Wrappers

//- rjf: Doubly-Linked-List Wrappers
#define RADDBGIC_DLLInsert_NP(f,l,p,n,next,prev) RADDBGIC_DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define RADDBGIC_DLLPushBack_NP(f,l,n,next,prev) RADDBGIC_DLLPushBack_NPZ(0,f,l,n,next,prev)
#define RADDBGIC_DLLPushFront_NP(f,l,n,next,prev) RADDBGIC_DLLPushFront_NPZ(0,f,l,n,next,prev)
#define RADDBGIC_DLLRemove_NP(f,l,n,next,prev) RADDBGIC_DLLRemove_NPZ(0,f,l,n,next,prev)
#define RADDBGIC_DLLInsert(f,l,p,n) RADDBGIC_DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define RADDBGIC_DLLPushBack(f,l,n) RADDBGIC_DLLPushBack_NPZ(0,f,l,n,next,prev)
#define RADDBGIC_DLLPushFront(f,l,n) RADDBGIC_DLLPushFront_NPZ(0,f,l,n,next,prev)
#define RADDBGIC_DLLRemove(f,l,n) RADDBGIC_DLLRemove_NPZ(0,f,l,n,next,prev)

//- rjf: Singly-Linked-List Queue Wrappers
#define RADDBGIC_SLLQueuePush_N(f,l,n,next) RADDBGIC_SLLQueuePush_NZ(0,f,l,n,next)
#define RADDBGIC_SLLQueuePushFront_N(f,l,n,next) RADDBGIC_SLLQueuePushFront_NZ(0,f,l,n,next)
#define RADDBGIC_SLLQueuePop_N(f,l,next) RADDBGIC_SLLQueuePop_NZ(0,f,l,next)
#define RADDBGIC_SLLQueuePush(f,l,n) RADDBGIC_SLLQueuePush_NZ(0,f,l,n,next)
#define RADDBGIC_SLLQueuePushFront(f,l,n) RADDBGIC_SLLQueuePushFront_NZ(0,f,l,n,next)
#define RADDBGIC_SLLQueuePop(f,l) RADDBGIC_SLLQueuePop_NZ(0,f,l,next)

//- rjf: Singly-Linked-List Stack Wrappers
#define RADDBGIC_SLLStackPush(f,n) RADDBGIC_SLLStackPush_N(f,n,next)
#define RADDBGIC_SLLStackPop(f) RADDBGIC_SLLStackPop_N(f,next)

////////////////////////////////
//~ rjf: Helper Macros

#if defined(_MSC_VER)
# define RADDBGIC_THREAD_LOCAL __declspec(thread)
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
# define RADDBGIC_THREAD_LOCAL __thread
#else
# error RADDBGIC_THREAD_LOCAL not defined for this compiler.
#endif

#if defined(_MSC_VER)
# define raddbgic_trap() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
# define raddbgic_trap() __builtin_trap()
#else
# error "raddbgic_trap not defined for this compiler."
#endif

#define raddbgic_assert_always(x) do{if(!(x)) {raddbgic_trap();}}while(0)
#if !defined(NDEBUG)
# define raddbgic_assert(x) raddbgic_assert_always(x)
#else
# define raddbgic_assert(x) (void)(x)
#endif
#define raddbgic_noop ((void)0)

////////////////////////////////
//~ rjf: Error Types

typedef struct RADDBGIC_Error RADDBGIC_Error;
struct RADDBGIC_Error
{
  RADDBGIC_Error *next;
  RADDBGIC_String8 msg;
};

typedef struct RADDBGIC_ErrorList RADDBGIC_ErrorList;
struct RADDBGIC_ErrorList
{
  RADDBGIC_Error *first;
  RADDBGIC_Error *last;
  RADDBGI_U64 count;
};

////////////////////////////////
//~ rjf: Auxiliary Data Structure Types

//- rjf: u64 -> pointer map

typedef struct RADDBGIC_U64ToPtrNode RADDBGIC_U64ToPtrNode;
struct RADDBGIC_U64ToPtrNode
{
  RADDBGIC_U64ToPtrNode *next;
  RADDBGI_U64 _padding_;
  RADDBGI_U64 key[1];
  void *ptr[1];
};

typedef struct RADDBGIC_U64ToPtrMap RADDBGIC_U64ToPtrMap;
struct RADDBGIC_U64ToPtrMap
{
  RADDBGIC_U64ToPtrNode **buckets;
  RADDBGI_U64 buckets_count;
  RADDBGI_U64 bucket_collision_count;
  RADDBGI_U64 pair_count;
};

typedef struct RADDBGIC_U64ToPtrLookup RADDBGIC_U64ToPtrLookup;
struct RADDBGIC_U64ToPtrLookup
{
  void *match;
  RADDBGIC_U64ToPtrNode *fill_node;
  RADDBGI_U32 fill_k;
};

//- rjf: string8 -> pointer map

typedef struct RADDBGIC_Str8ToPtrNode RADDBGIC_Str8ToPtrNode;
struct RADDBGIC_Str8ToPtrNode
{
  struct RADDBGIC_Str8ToPtrNode *next;
  RADDBGIC_String8 key;
  RADDBGI_U64 hash;
  void *ptr;
};

typedef struct RADDBGIC_Str8ToPtrMap RADDBGIC_Str8ToPtrMap;
struct RADDBGIC_Str8ToPtrMap
{
  RADDBGIC_Str8ToPtrNode **buckets;
  RADDBGI_U64 buckets_count;
  RADDBGI_U64 bucket_collision_count;
  RADDBGI_U64 pair_count;
};

//- rjf: sortable range data structure

typedef struct RADDBGIC_SortKey RADDBGIC_SortKey;
struct RADDBGIC_SortKey
{
  RADDBGI_U64 key;
  void *val;
};

typedef struct RADDBGIC_OrderedRange RADDBGIC_OrderedRange;
struct RADDBGIC_OrderedRange
{
  RADDBGIC_OrderedRange *next;
  RADDBGI_U64 first;
  RADDBGI_U64 opl;
};

////////////////////////////////
//~ rjf: Binary Section Types

typedef struct RADDBGIC_BinarySection RADDBGIC_BinarySection;
struct RADDBGIC_BinarySection
{
  RADDBGIC_BinarySection *next;
  RADDBGIC_String8 name;
  RADDBGI_BinarySectionFlags flags;
  RADDBGI_U64 voff_first;
  RADDBGI_U64 voff_opl;
  RADDBGI_U64 foff_first;
  RADDBGI_U64 foff_opl;
};

////////////////////////////////
//~ rjf: Per-Compilation-Unit Info Types

typedef struct RADDBGIC_LineSequence RADDBGIC_LineSequence;
struct RADDBGIC_LineSequence
{
  RADDBGIC_String8 file_name;
  RADDBGI_U64 *voffs;     // [line_count + 1] (sorted)
  RADDBGI_U32 *line_nums; // [line_count]
  RADDBGI_U16 *col_nums;  // [2*line_count]
  RADDBGI_U64 line_count;
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
  RADDBGIC_String8 unit_name;
  RADDBGIC_String8 compiler_name;
  RADDBGIC_String8 source_file;
  RADDBGIC_String8 object_file;
  RADDBGIC_String8 archive_file;
  RADDBGIC_String8 build_path;
  RADDBGI_Language language;
};

typedef struct RADDBGIC_Unit RADDBGIC_Unit;
struct RADDBGIC_Unit
{
  RADDBGIC_Unit *next_order;
  RADDBGI_U32 idx;
  RADDBGI_S32 info_is_set;
  RADDBGIC_String8 unit_name;
  RADDBGIC_String8 compiler_name;
  RADDBGIC_String8 source_file;
  RADDBGIC_String8 object_file;
  RADDBGIC_String8 archive_file;
  RADDBGIC_String8 build_path;
  RADDBGI_Language language;
  RADDBGIC_LineSequenceNode *line_seq_first;
  RADDBGIC_LineSequenceNode *line_seq_last;
  RADDBGI_U64 line_seq_count;
};

typedef struct RADDBGIC_UnitVMapRange RADDBGIC_UnitVMapRange;
struct RADDBGIC_UnitVMapRange
{
  RADDBGIC_UnitVMapRange *next;
  RADDBGIC_Unit *unit;
  RADDBGI_U64 first;
  RADDBGI_U64 opl;
};

////////////////////////////////
//~ rjf: Type Info Types

typedef RADDBGI_U8 RADDBGIC_TypeConstructKind;
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
  RADDBGIC_String8 name;
  struct RADDBGIC_Type *type;
  RADDBGI_U32 off;
};

typedef struct RADDBGIC_TypeEnumVal RADDBGIC_TypeEnumVal;
struct RADDBGIC_TypeEnumVal
{
  RADDBGIC_TypeEnumVal *next;
  RADDBGIC_String8 name;
  RADDBGI_U64 val;
};

typedef struct RADDBGIC_Type RADDBGIC_Type;
struct RADDBGIC_Type
{
  RADDBGIC_Type *next_order;
  RADDBGI_TypeKind kind;
  RADDBGI_U32 idx;
  RADDBGI_U32 byte_size;
  RADDBGI_U32 flags;
  RADDBGI_U32 off;
  RADDBGI_U32 count;
  RADDBGIC_String8 name;
  RADDBGIC_Type *direct_type;
  RADDBGIC_Type **param_types;
  struct RADDBGIC_TypeUDT *udt;
};

typedef struct RADDBGIC_TypeUDT RADDBGIC_TypeUDT;
struct RADDBGIC_TypeUDT
{
  RADDBGIC_TypeUDT *next_order;
  RADDBGI_U32 idx;
  RADDBGIC_Type *self_type;
  RADDBGIC_TypeMember *first_member;
  RADDBGIC_TypeMember *last_member;
  RADDBGI_U64 member_count;
  RADDBGIC_TypeEnumVal *first_enum_val;
  RADDBGIC_TypeEnumVal *last_enum_val;
  RADDBGI_U64 enum_val_count;
  RADDBGIC_String8 source_path;
  RADDBGI_U32 line;
  RADDBGI_U32 col;
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
  RADDBGI_U64 count;
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
  RADDBGIC_String8 name;
  RADDBGIC_String8 link_name;
  RADDBGIC_Type *type;
  RADDBGI_S32 is_extern;
  RADDBGI_U64 offset;
  // TODO(allen): should this actually be "container scope"?
  struct RADDBGIC_Symbol *container_symbol;
  RADDBGIC_Type *container_type;
  struct RADDBGIC_Scope *root_scope;
};

typedef struct RADDBGIC_Symbol RADDBGIC_Symbol;
struct RADDBGIC_Symbol
{
  RADDBGIC_Symbol *next_order;
  RADDBGI_U32 idx;
  RADDBGIC_SymbolKind kind;
  RADDBGIC_String8 name;
  RADDBGIC_String8 link_name;
  RADDBGIC_Type *type;
  RADDBGI_S32 is_extern;
  RADDBGI_S8 offset_is_set;
  RADDBGI_U64 offset;
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
  RADDBGIC_String8 name;
  RADDBGIC_Type *type;
};

typedef struct RADDBGIC_Local RADDBGIC_Local;
struct RADDBGIC_Local
{
  RADDBGIC_Local *next;
  RADDBGI_LocalKind kind;
  RADDBGIC_String8 name;
  RADDBGIC_Type *type;
  struct RADDBGIC_LocationSet *locset;
};

typedef struct RADDBGIC_VOffRange RADDBGIC_VOffRange;
struct RADDBGIC_VOffRange
{
  RADDBGIC_VOffRange *next;
  RADDBGI_U64 voff_first;
  RADDBGI_U64 voff_opl;
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
  RADDBGI_U64 voff_base;
  RADDBGIC_VOffRange *first_range;
  RADDBGIC_VOffRange *last_range;
  RADDBGI_U32 range_count;
  RADDBGI_U32 idx;
  RADDBGIC_Local *first_local;
  RADDBGIC_Local *last_local;
  RADDBGI_U32 local_count;
};

////////////////////////////////
//~ rjf: Location Info Types

typedef struct RADDBGIC_EvalBytecodeOp RADDBGIC_EvalBytecodeOp;
struct RADDBGIC_EvalBytecodeOp
{
  RADDBGIC_EvalBytecodeOp *next;
  RADDBGI_EvalOp op;
  RADDBGI_U32 p_size;
  RADDBGI_U64 p;
};

typedef struct RADDBGIC_EvalBytecode RADDBGIC_EvalBytecode;
struct RADDBGIC_EvalBytecode
{
  RADDBGIC_EvalBytecodeOp *first_op;
  RADDBGIC_EvalBytecodeOp *last_op;
  RADDBGI_U32 op_count;
  RADDBGI_U32 encoded_size;
};

typedef struct RADDBGIC_Location RADDBGIC_Location;
struct RADDBGIC_Location
{
  RADDBGI_LocationKind kind;
  RADDBGI_U8 register_code;
  RADDBGI_U16 offset;
  RADDBGIC_EvalBytecode bytecode;
};

typedef struct RADDBGIC_LocationCase RADDBGIC_LocationCase;
struct RADDBGIC_LocationCase
{
  RADDBGIC_LocationCase *next;
  RADDBGI_U64 voff_first;
  RADDBGI_U64 voff_opl;
  RADDBGIC_Location *location;
};

typedef struct RADDBGIC_LocationSet RADDBGIC_LocationSet;
struct RADDBGIC_LocationSet
{
  RADDBGIC_LocationCase *first_location_case;
  RADDBGIC_LocationCase *last_location_case;
  RADDBGI_U64 location_case_count;
};

////////////////////////////////
//~ rjf: Name Map Types

typedef struct RADDBGIC_NameMapIdxNode RADDBGIC_NameMapIdxNode;
struct RADDBGIC_NameMapIdxNode
{
  RADDBGIC_NameMapIdxNode *next;
  RADDBGI_U32 idx[8];
};

typedef struct RADDBGIC_NameMapNode RADDBGIC_NameMapNode;
struct RADDBGIC_NameMapNode
{
  RADDBGIC_NameMapNode *bucket_next;
  RADDBGIC_NameMapNode *order_next;
  RADDBGIC_String8 string;
  RADDBGIC_NameMapIdxNode *idx_first;
  RADDBGIC_NameMapIdxNode *idx_last;
  RADDBGI_U64 idx_count;
};

typedef struct RADDBGIC_NameMap RADDBGIC_NameMap;
struct RADDBGIC_NameMap
{
  RADDBGIC_NameMapNode **buckets;
  RADDBGI_U64 buckets_count;
  RADDBGI_U64 bucket_collision_count;
  RADDBGIC_NameMapNode *first;
  RADDBGIC_NameMapNode *last;
  RADDBGI_U64 name_count;
};

////////////////////////////////
//~ rjf: Top-Level Debug Info Types

typedef struct RADDBGIC_TopLevelInfo RADDBGIC_TopLevelInfo;
struct RADDBGIC_TopLevelInfo
{
  RADDBGI_Arch architecture;
  RADDBGIC_String8 exe_name;
  RADDBGI_U64 exe_hash;
  RADDBGI_U64 voff_max;
};

////////////////////////////////
//~ rjf: Root Construction Bundle Types

typedef struct RADDBGIC_RootParams RADDBGIC_RootParams;
struct RADDBGIC_RootParams
{
  RADDBGI_U64 addr_size;
  RADDBGI_U32 bucket_count_units;              // optional; default chosen if 0
  RADDBGI_U32 bucket_count_symbols;            // optional; default chosen if 0
  RADDBGI_U32 bucket_count_scopes;             // optional; default chosen if 0
  RADDBGI_U32 bucket_count_locals;             // optional; default chosen if 0
  RADDBGI_U32 bucket_count_types;              // optional; default chosen if 0
  RADDBGI_U64 bucket_count_type_constructs;    // optional; default chosen if 0
};

typedef struct RADDBGIC_Root RADDBGIC_Root;
struct RADDBGIC_Root
{
  RADDBGIC_Arena *arena;
  RADDBGIC_ErrorList errors;
  
  //////// Contextual Information
  
  RADDBGI_U64 addr_size;
  
  //////// Info Declared By User
  
  // top level info
  RADDBGI_S32 top_level_info_is_set;
  RADDBGIC_TopLevelInfo top_level_info;
  
  // binary layout
  RADDBGIC_BinarySection *binary_section_first;
  RADDBGIC_BinarySection *binary_section_last;
  RADDBGI_U64 binary_section_count;
  
  // compilation units
  RADDBGIC_Unit *unit_first;
  RADDBGIC_Unit *unit_last;
  RADDBGI_U64 unit_count;
  
  RADDBGIC_UnitVMapRange *unit_vmap_range_first;
  RADDBGIC_UnitVMapRange *unit_vmap_range_last;
  RADDBGI_U64 unit_vmap_range_count;
  
  // types
  RADDBGIC_Type *first_type;
  RADDBGIC_Type *last_type;
  RADDBGI_U64 type_count;
  
  RADDBGIC_Type *nil_type;
  RADDBGIC_Type *variadic_type;
  
  RADDBGIC_Type handled_nil_type;
  
  RADDBGIC_TypeUDT *first_udt;
  RADDBGIC_TypeUDT *last_udt;
  RADDBGI_U64 type_udt_count;
  
  RADDBGI_U64 total_member_count;
  RADDBGI_U64 total_enum_val_count;
  
  // symbols
  RADDBGIC_Symbol *first_symbol;
  RADDBGIC_Symbol *last_symbol;
  union
  {
    RADDBGI_U64 symbol_count;
    RADDBGI_U64 symbol_kind_counts[RADDBGIC_SymbolKind_COUNT];
  };
  
  RADDBGIC_Scope *first_scope;
  RADDBGIC_Scope *last_scope;
  RADDBGI_U64 scope_count;
  RADDBGI_U64 scope_voff_count;
  
  RADDBGIC_Local *first_local;
  RADDBGIC_Local *last_local;
  RADDBGI_U64 local_count;
  RADDBGI_U64 location_count;
  
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
  RADDBGI_U64 size;
  RADDBGI_DataSectionTag tag;
};

typedef struct RADDBGIC_DSections RADDBGIC_DSections;
struct RADDBGIC_DSections
{
  RADDBGIC_DSectionNode *first;
  RADDBGIC_DSectionNode *last;
  RADDBGI_U32 count;
};

//- rjf: bake string data structure

typedef struct RADDBGIC_StringNode RADDBGIC_StringNode;
struct RADDBGIC_StringNode
{
  RADDBGIC_StringNode *order_next;
  RADDBGIC_StringNode *bucket_next;
  RADDBGIC_String8 str;
  RADDBGI_U64 hash;
  RADDBGI_U32 idx;
};

typedef struct RADDBGIC_Strings RADDBGIC_Strings;
struct RADDBGIC_Strings
{
  RADDBGIC_StringNode *order_first;
  RADDBGIC_StringNode *order_last;
  RADDBGIC_StringNode **buckets;
  RADDBGI_U64 buckets_count;
  RADDBGI_U64 bucket_collision_count;
  RADDBGI_U32 count;
};

//- rjf: index run baking data structure

typedef struct RADDBGIC_IdxRunNode RADDBGIC_IdxRunNode;
struct RADDBGIC_IdxRunNode
{
  RADDBGIC_IdxRunNode *order_next;
  RADDBGIC_IdxRunNode *bucket_next;
  RADDBGI_U32 *idx_run;
  RADDBGI_U64 hash;
  RADDBGI_U32 count;
  RADDBGI_U32 first_idx;
};

typedef struct RADDBGIC_IdxRuns RADDBGIC_IdxRuns;
struct RADDBGIC_IdxRuns
{
  RADDBGIC_IdxRunNode *order_first;
  RADDBGIC_IdxRunNode *order_last;
  RADDBGIC_IdxRunNode **buckets;
  RADDBGI_U64 buckets_count;
  RADDBGI_U64 bucket_collision_count;
  RADDBGI_U32 count;
  RADDBGI_U32 idx_count;
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
  RADDBGIC_String8 name;
  struct RADDBGIC_SrcNode *src_file;
  RADDBGI_U32 idx;
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
  RADDBGI_U32 idx;
  
  RADDBGIC_String8 normal_full_path;
  
  // place to gather the line info attached to this src file
  RADDBGIC_LineMapFragment *first_fragment;
  RADDBGIC_LineMapFragment *last_fragment;
  
  // place to put the final baked version of this file's line map
  RADDBGI_U32 line_map_nums_data_idx;
  RADDBGI_U32 line_map_range_data_idx;
  RADDBGI_U32 line_map_count;
  RADDBGI_U32 line_map_voff_data_idx;
};

typedef struct RADDBGIC_PathTree RADDBGIC_PathTree;
struct RADDBGIC_PathTree
{
  RADDBGIC_PathNode *first;
  RADDBGIC_PathNode *last;
  RADDBGI_U32 count;
  RADDBGIC_PathNode root;
  RADDBGIC_SrcNode *src_first;
  RADDBGIC_SrcNode *src_last;
  RADDBGI_U32 src_count;
};

//- rjf: line info baking data structures

typedef struct RADDBGIC_LineRec RADDBGIC_LineRec;
struct RADDBGIC_LineRec
{
  RADDBGI_U32 file_id;
  RADDBGI_U32 line_num;
  RADDBGI_U16 col_first;
  RADDBGI_U16 col_opl;
};

typedef struct RADDBGIC_UnitLinesCombined RADDBGIC_UnitLinesCombined;
struct RADDBGIC_UnitLinesCombined
{
  RADDBGI_U64 *voffs;
  RADDBGI_Line *lines;
  RADDBGI_U16 *cols;
  RADDBGI_U32 line_count;
};

typedef struct RADDBGIC_SrcLinesCombined RADDBGIC_SrcLinesCombined;
struct RADDBGIC_SrcLinesCombined
{
  RADDBGI_U32 *line_nums;
  RADDBGI_U32 *line_ranges;
  RADDBGI_U64 *voffs;
  RADDBGI_U32  line_count;
  RADDBGI_U32  voff_count;
};

typedef struct RADDBGIC_SrcLineMapVoffBlock RADDBGIC_SrcLineMapVoffBlock;
struct RADDBGIC_SrcLineMapVoffBlock
{
  RADDBGIC_SrcLineMapVoffBlock *next;
  RADDBGI_U64 voff;
};

typedef struct RADDBGIC_SrcLineMapBucket RADDBGIC_SrcLineMapBucket;
struct RADDBGIC_SrcLineMapBucket
{
  RADDBGIC_SrcLineMapBucket *order_next;
  RADDBGIC_SrcLineMapBucket *hash_next;
  RADDBGI_U32 line_num;
  RADDBGIC_SrcLineMapVoffBlock *first_voff_block;
  RADDBGIC_SrcLineMapVoffBlock *last_voff_block;
  RADDBGI_U64 voff_count;
};

//- rjf: vmap baking data structure 

typedef struct RADDBGIC_VMap RADDBGIC_VMap;
struct RADDBGIC_VMap
{
  RADDBGI_VMapEntry *vmap; // [count + 1]
  RADDBGI_U32 count;
};

typedef struct RADDBGIC_VMapMarker RADDBGIC_VMapMarker;
struct RADDBGIC_VMapMarker
{
  RADDBGI_U32 idx;
  RADDBGI_U32 begin_range;
};

typedef struct RADDBGIC_VMapRangeTracker RADDBGIC_VMapRangeTracker;
struct RADDBGIC_VMapRangeTracker
{
  RADDBGIC_VMapRangeTracker *next;
  RADDBGI_U32 idx;
};

//- rjf: type data baking types

typedef struct RADDBGIC_TypeData RADDBGIC_TypeData;
struct RADDBGIC_TypeData
{
  RADDBGI_TypeNode *type_nodes;
  RADDBGI_U32 type_node_count;
  
  RADDBGI_UDT *udts;
  RADDBGI_U32 udt_count;
  
  RADDBGI_Member *members;
  RADDBGI_U32 member_count;
  
  RADDBGI_EnumMember *enum_members;
  RADDBGI_U32 enum_member_count;
};

//- rjf: symbol data baking types

typedef struct RADDBGIC_SymbolData RADDBGIC_SymbolData;
struct RADDBGIC_SymbolData
{
  RADDBGI_GlobalVariable *global_variables;
  RADDBGI_U32 global_variable_count;
  
  RADDBGIC_VMap *global_vmap;
  
  RADDBGI_ThreadVariable *thread_variables;
  RADDBGI_U32 thread_variable_count;
  
  RADDBGI_Procedure *procedures;
  RADDBGI_U32 procedure_count;
  
  RADDBGI_Scope *scopes;
  RADDBGI_U32 scope_count;
  
  RADDBGI_U64 *scope_voffs;
  RADDBGI_U32 scope_voff_count;
  
  RADDBGIC_VMap *scope_vmap;
  
  RADDBGI_Local *locals;
  RADDBGI_U32 local_count;
  
  RADDBGI_LocationBlock *location_blocks;
  RADDBGI_U32 location_block_count;
  
  void *location_data;
  RADDBGI_U32 location_data_size;
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
  RADDBGI_U64 count;
};

typedef struct RADDBGIC_NameMapBaked RADDBGIC_NameMapBaked;
struct RADDBGIC_NameMapBaked
{
  RADDBGI_NameMapBucket *buckets;
  RADDBGI_NameMapNode *nodes;
  RADDBGI_U32 bucket_count;
  RADDBGI_U32 node_count;
};

//- rjf: bundle baking context type

typedef struct RADDBGIC_BakeParams RADDBGIC_BakeParams;
struct RADDBGIC_BakeParams
{
  RADDBGI_U64 strings_bucket_count;
  RADDBGI_U64 idx_runs_bucket_count;
};

typedef struct RADDBGIC_BakeCtx RADDBGIC_BakeCtx;
struct RADDBGIC_BakeCtx
{
  RADDBGIC_Arena *arena;
  RADDBGIC_Strings strs;
  RADDBGIC_IdxRuns idxs;
  RADDBGIC_PathTree *tree;
};

////////////////////////////////
//~ rjf: Basic Helpers

//- rjf: memory operations
#if !defined(RADDBGIC_MEMSET_OVERRIDE)
RADDBGI_PROC void *raddbgic_memset_fallback(void *dst, RADDBGI_U8 c, RADDBGI_U64 size);
#endif
#if !defined(RADDBGIC_MEMCPY_OVERRIDE)
RADDBGI_PROC void *raddbgic_memcpy_fallback(void *dst, void *src, RADDBGI_U64 size);
#endif
#define raddbgic_memzero(ptr, size) raddbgic_memset((ptr), 0, (size))
#define raddbgic_memzero_struct(ptr) raddbgic_memset((ptr), 0, sizeof(*(ptr)))
#define raddbgic_memcpy_struct(dst, src) raddbgic_memcpy((dst), (src), sizeof(*(dst)))

//- rjf: arenas
#if !defined (RADDBGIC_ARENA_OVERRIDE)
RADDBGI_PROC RADDBGIC_Arena *raddbgic_arena_alloc_fallback(void);
RADDBGI_PROC void raddbgic_arena_release_fallback(RADDBGIC_Arena *arena);
RADDBGI_PROC RADDBGI_U64 raddbgic_arena_pos_fallback(RADDBGIC_Arena *arena);
RADDBGI_PROC void *raddbgic_arena_push_fallback(RADDBGIC_Arena *arena, RADDBGI_U64 size);
RADDBGI_PROC void raddbgic_arena_pop_to_fallback(RADDBGIC_Arena *arena, RADDBGI_U64 pos);
#endif
#define raddbgic_push_array_no_zero(a,T,c) (T*)raddbgic_arena_push((a), sizeof(T)*(c))
#define raddbgic_push_array(a,T,c) (T*)raddbgic_memzero(raddbgic_push_array_no_zero(a,T,c), sizeof(T)*(c))

//- rjf: thread-local scratch arenas
#if !defined (RADDBGIC_SCRATCH_OVERRIDE)
RADDBGI_PROC RADDBGIC_Temp raddbgic_scratch_begin_fallback(RADDBGIC_Arena **conflicts, RADDBGI_U64 conflicts_count);
RADDBGI_PROC void raddbgic_scratch_end_fallback(RADDBGIC_Temp temp);
#endif

//- rjf: strings
RADDBGI_PROC RADDBGIC_String8 raddbgic_str8(RADDBGI_U8 *str, RADDBGI_U64 size);
RADDBGI_PROC RADDBGIC_String8 raddbgic_str8_copy(RADDBGIC_Arena *arena, RADDBGIC_String8 src);
#define raddbgic_str8_lit(S)  raddbgic_str8((RADDBGI_U8*)(S), sizeof(S) - 1)

//- rjf: type lists
RADDBGI_PROC void raddbgic_type_list_push(RADDBGIC_Arena *arena, RADDBGIC_TypeList *list, RADDBGIC_Type *type);

//- rjf: bytecode lists
RADDBGI_PROC void raddbgic_bytecode_push_op(RADDBGIC_Arena *arena, RADDBGIC_EvalBytecode *bytecode, RADDBGI_EvalOp op, RADDBGI_U64 p);
RADDBGI_PROC void raddbgic_bytecode_push_uconst(RADDBGIC_Arena *arena, RADDBGIC_EvalBytecode *bytecode, RADDBGI_U64 x);
RADDBGI_PROC void raddbgic_bytecode_push_sconst(RADDBGIC_Arena *arena, RADDBGIC_EvalBytecode *bytecode, RADDBGI_S64 x);
RADDBGI_PROC void raddbgic_bytecode_concat_in_place(RADDBGIC_EvalBytecode *left_dst, RADDBGIC_EvalBytecode *right_destroyed);

//- rjf: sortable range sorting
RADDBGI_PROC RADDBGIC_SortKey* raddbgic_sort_key_array(RADDBGIC_Arena *arena, RADDBGIC_SortKey *keys, RADDBGI_U64 count);

////////////////////////////////
//~ rjf: Auxiliary Data Structure Functions

//- rjf: u64 -> ptr map
RADDBGI_PROC void raddbgic_u64toptr_map_init(RADDBGIC_Arena *arena, RADDBGIC_U64ToPtrMap *map, RADDBGI_U64 bucket_count);
RADDBGI_PROC void raddbgic_u64toptr_map_lookup(RADDBGIC_U64ToPtrMap *map, RADDBGI_U64 key, RADDBGI_U64 hash, RADDBGIC_U64ToPtrLookup *lookup_out);
RADDBGI_PROC void raddbgic_u64toptr_map_insert(RADDBGIC_Arena *arena, RADDBGIC_U64ToPtrMap *map, RADDBGI_U64 key, RADDBGI_U64 hash, RADDBGIC_U64ToPtrLookup *lookup, void *ptr);

//- rjf: string8 -> ptr map
RADDBGI_PROC void raddbgic_str8toptr_map_init(RADDBGIC_Arena *arena, RADDBGIC_Str8ToPtrMap *map, RADDBGI_U64 bucket_count);
RADDBGI_PROC void*raddbgic_str8toptr_map_lookup(RADDBGIC_Str8ToPtrMap *map, RADDBGIC_String8 key, RADDBGI_U64 hash);
RADDBGI_PROC void raddbgic_str8toptr_map_insert(RADDBGIC_Arena *arena, RADDBGIC_Str8ToPtrMap *map, RADDBGIC_String8 key, RADDBGI_U64 hash, void *ptr);

////////////////////////////////
//~ rjf: Loose Debug Info Construction (Anything -> Loose) Functions

//- rjf: root creation
RADDBGI_PROC RADDBGIC_Root* raddbgic_root_alloc(RADDBGIC_RootParams *params);
RADDBGI_PROC void           raddbgic_root_release(RADDBGIC_Root *root);

//- rjf: error accumulation
RADDBGI_PROC void raddbgic_push_error(RADDBGIC_Root *root, RADDBGIC_String8 string);
RADDBGI_PROC void raddbgic_push_errorf(RADDBGIC_Root *root, char *fmt, ...);
RADDBGI_PROC RADDBGIC_Error* raddbgic_first_error_from_root(RADDBGIC_Root *root);

//- rjf: top-level info specification
RADDBGI_PROC void raddbgic_set_top_level_info(RADDBGIC_Root *root, RADDBGIC_TopLevelInfo *tli);

//- rjf: binary section building
RADDBGI_PROC void raddbgic_add_binary_section(RADDBGIC_Root *root,
                                              RADDBGIC_String8 name, RADDBGI_BinarySectionFlags flags,
                                              RADDBGI_U64 voff_first, RADDBGI_U64 voff_opl, RADDBGI_U64 foff_first,
                                              RADDBGI_U64 foff_opl);

//- rjf: unit info building
RADDBGI_PROC RADDBGIC_Unit* raddbgic_unit_handle_from_user_id(RADDBGIC_Root *root, RADDBGI_U64 unit_user_id, RADDBGI_U64 unit_user_id_hash);
RADDBGI_PROC void raddbgic_unit_set_info(RADDBGIC_Root *root, RADDBGIC_Unit *unit, RADDBGIC_UnitInfo *info);
RADDBGI_PROC void raddbgic_unit_add_line_sequence(RADDBGIC_Root *root, RADDBGIC_Unit *unit, RADDBGIC_LineSequence *line_sequence);
RADDBGI_PROC void raddbgic_unit_vmap_add_range(RADDBGIC_Root *root, RADDBGIC_Unit *unit, RADDBGI_U64 first, RADDBGI_U64 opl);

//- rjf: type info lookups/reservations
RADDBGI_PROC RADDBGIC_Type*        raddbgic_type_from_id(RADDBGIC_Root *root, RADDBGI_U64 type_user_id, RADDBGI_U64 type_user_id_hash);
RADDBGI_PROC RADDBGIC_Reservation* raddbgic_type_reserve_id(RADDBGIC_Root *root, RADDBGI_U64 type_user_id, RADDBGI_U64 type_user_id_hash);
RADDBGI_PROC void                  raddbgic_type_fill_id(RADDBGIC_Root *root, RADDBGIC_Reservation *res, RADDBGIC_Type *type);

//- rjf: nil/singleton types
RADDBGI_PROC RADDBGI_S32    raddbgic_type_is_unhandled_nil(RADDBGIC_Root *root, RADDBGIC_Type *type);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_handled_nil(RADDBGIC_Root *root);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_nil(RADDBGIC_Root *root);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_variadic(RADDBGIC_Root *root);

//- rjf: base type info constructors
RADDBGI_PROC RADDBGIC_Type*    raddbgic_type_new(RADDBGIC_Root *root);
RADDBGI_PROC RADDBGIC_TypeUDT* raddbgic_type_udt_from_any_type(RADDBGIC_Root *root, RADDBGIC_Type *type);
RADDBGI_PROC RADDBGIC_TypeUDT* raddbgic_type_udt_from_record_type(RADDBGIC_Root *root, RADDBGIC_Type *type);

//- rjf: basic/operator type construction helpers
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_basic(RADDBGIC_Root *root, RADDBGI_TypeKind type_kind, RADDBGIC_String8 name);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_modifier(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGI_TypeModifierFlags flags);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_bitfield(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGI_U32 bit_off, RADDBGI_U32 bit_count);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_pointer(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGI_TypeKind ptr_type_kind);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_array(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGI_U64 count);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_proc(RADDBGIC_Root *root, RADDBGIC_Type *return_type, struct RADDBGIC_TypeList *params);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_method(RADDBGIC_Root *root, RADDBGIC_Type *this_type, RADDBGIC_Type *return_type, struct RADDBGIC_TypeList *params);

//- rjf: udt type constructors
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_udt(RADDBGIC_Root *root, RADDBGI_TypeKind record_type_kind, RADDBGIC_String8 name, RADDBGI_U64 size);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_enum(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGIC_String8 name);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_alias(RADDBGIC_Root *root, RADDBGIC_Type *direct_type, RADDBGIC_String8 name);
RADDBGI_PROC RADDBGIC_Type* raddbgic_type_incomplete(RADDBGIC_Root *root, RADDBGI_TypeKind type_kind, RADDBGIC_String8 name);

//- rjf: type member building
RADDBGI_PROC void raddbgic_type_add_member_data_field(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_String8 name, RADDBGIC_Type *mem_type, RADDBGI_U32 off);
RADDBGI_PROC void raddbgic_type_add_member_static_data(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_String8 name, RADDBGIC_Type *mem_type);
RADDBGI_PROC void raddbgic_type_add_member_method(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_String8 name, RADDBGIC_Type *mem_type);
RADDBGI_PROC void raddbgic_type_add_member_static_method(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_String8 name, RADDBGIC_Type *mem_type);
RADDBGI_PROC void raddbgic_type_add_member_virtual_method(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_String8 name, RADDBGIC_Type *mem_type);
RADDBGI_PROC void raddbgic_type_add_member_base(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_Type *base_type, RADDBGI_U32 off);
RADDBGI_PROC void raddbgic_type_add_member_virtual_base(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_Type *base_type, RADDBGI_U32 vptr_off, RADDBGI_U32 vtable_off);
RADDBGI_PROC void raddbgic_type_add_member_nested_type(RADDBGIC_Root *root, RADDBGIC_Type *record_type, RADDBGIC_Type *nested_type);
RADDBGI_PROC void raddbgic_type_add_enum_val(RADDBGIC_Root *root, RADDBGIC_Type *enum_type, RADDBGIC_String8 name, RADDBGI_U64 val);

//- rjf: type source coordinate specifications
RADDBGI_PROC void raddbgic_type_set_source_coordinates(RADDBGIC_Root *root, RADDBGIC_Type *defined_type, RADDBGIC_String8 source_path, RADDBGI_U32 line, RADDBGI_U32 col);

//- rjf: symbol info building
RADDBGI_PROC RADDBGIC_Symbol* raddbgic_symbol_handle_from_user_id(RADDBGIC_Root *root, RADDBGI_U64 symbol_user_id, RADDBGI_U64 symbol_user_id_hash);
RADDBGI_PROC void raddbgic_symbol_set_info(RADDBGIC_Root *root, RADDBGIC_Symbol *symbol, RADDBGIC_SymbolInfo *info);

//- rjf: scope info building
RADDBGI_PROC RADDBGIC_Scope *raddbgic_scope_handle_from_user_id(RADDBGIC_Root *root, RADDBGI_U64 scope_user_id, RADDBGI_U64 scope_user_id_hash);
RADDBGI_PROC void raddbgic_scope_set_parent(RADDBGIC_Root *root, RADDBGIC_Scope *scope, RADDBGIC_Scope *parent);
RADDBGI_PROC void raddbgic_scope_add_voff_range(RADDBGIC_Root *root, RADDBGIC_Scope *scope, RADDBGI_U64 voff_first, RADDBGI_U64 voff_opl);
RADDBGI_PROC void raddbgic_scope_recursive_set_symbol(RADDBGIC_Scope *scope, RADDBGIC_Symbol *symbol);

//- rjf: local info building
RADDBGI_PROC RADDBGIC_Local* raddbgic_local_handle_from_user_id(RADDBGIC_Root *root, RADDBGI_U64 local_user_id, RADDBGI_U64 local_user_id_hash);
RADDBGI_PROC void raddbgic_local_set_basic_info(RADDBGIC_Root *root, RADDBGIC_Local *local, RADDBGIC_LocalInfo *info);
RADDBGI_PROC RADDBGIC_LocationSet* raddbgic_location_set_from_local(RADDBGIC_Root *root, RADDBGIC_Local *local);

//- rjf: location info building
RADDBGI_PROC void raddbgic_location_set_add_case(RADDBGIC_Root *root, RADDBGIC_LocationSet *locset, RADDBGI_U64 voff_first, RADDBGI_U64 voff_opl, RADDBGIC_Location *location);
RADDBGI_PROC RADDBGIC_Location* raddbgic_location_addr_bytecode_stream(RADDBGIC_Root *root, struct RADDBGIC_EvalBytecode *bytecode);
RADDBGI_PROC RADDBGIC_Location* raddbgic_location_val_bytecode_stream(RADDBGIC_Root *root, struct RADDBGIC_EvalBytecode *bytecode);
RADDBGI_PROC RADDBGIC_Location* raddbgic_location_addr_reg_plus_u16(RADDBGIC_Root *root, RADDBGI_U8 reg_code, RADDBGI_U16 offset);
RADDBGI_PROC RADDBGIC_Location* raddbgic_location_addr_addr_reg_plus_u16(RADDBGIC_Root *root, RADDBGI_U8 reg_code, RADDBGI_U16 offset);
RADDBGI_PROC RADDBGIC_Location* raddbgic_location_val_reg(RADDBGIC_Root *root, RADDBGI_U8 reg_code);

//- rjf: name map building
RADDBGI_PROC RADDBGIC_NameMap* raddbgic_name_map_for_kind(RADDBGIC_Root *root, RADDBGI_NameMapKind kind);
RADDBGI_PROC void          raddbgic_name_map_add_pair(RADDBGIC_Root *root, RADDBGIC_NameMap *map, RADDBGIC_String8 name, RADDBGI_U32 idx);

////////////////////////////////
//~ rjf: Debug Info Baking (Loose -> Tight) Functions

//- rjf: bake context construction
RADDBGI_PROC RADDBGIC_BakeCtx* raddbgic_bake_ctx_begin(RADDBGIC_BakeParams *params);
RADDBGI_PROC void              raddbgic_bake_ctx_release(RADDBGIC_BakeCtx *bake_ctx);

//- rjf: string baking
RADDBGI_PROC RADDBGI_U32 raddbgic_string(RADDBGIC_BakeCtx *bctx, RADDBGIC_String8 str);

//- rjf: idx run baking
RADDBGI_PROC RADDBGI_U64 raddbgic_idx_run_hash(RADDBGI_U32 *idx_run, RADDBGI_U32 count);
RADDBGI_PROC RADDBGI_U32 raddbgic_idx_run(RADDBGIC_BakeCtx *bctx, RADDBGI_U32 *idx_run, RADDBGI_U32 count);

//- rjf: data section baking
RADDBGI_PROC RADDBGI_U32 raddbgic_dsection(RADDBGIC_Arena *arena, RADDBGIC_DSections *dss, void *data, RADDBGI_U64 size, RADDBGI_DataSectionTag tag);

//- rjf: paths baking
RADDBGI_PROC RADDBGIC_String8   raddbgic_normal_string_from_path_node(RADDBGIC_Arena *arena, RADDBGIC_PathNode *node);
RADDBGI_PROC void               raddbgic_normal_string_from_path_node_build(RADDBGIC_Arena *arena, RADDBGIC_PathNode *node, RADDBGIC_String8List *out);
RADDBGI_PROC RADDBGIC_PathNode* raddbgic_paths_new_node(RADDBGIC_BakeCtx *bctx);
RADDBGI_PROC RADDBGIC_PathNode* raddbgic_paths_sub_path(RADDBGIC_BakeCtx *bctx, RADDBGIC_PathNode *dir, RADDBGIC_String8 sub_dir);
RADDBGI_PROC RADDBGIC_PathNode* raddbgic_paths_node_from_path(RADDBGIC_BakeCtx *bctx,  RADDBGIC_String8 path);
RADDBGI_PROC RADDBGI_U32        raddbgic_paths_idx_from_path(RADDBGIC_BakeCtx *bctx, RADDBGIC_String8 path);
RADDBGI_PROC RADDBGIC_SrcNode*  raddbgic_paths_new_src_node(RADDBGIC_BakeCtx *bctx);
RADDBGI_PROC RADDBGIC_SrcNode*  raddbgic_paths_src_node_from_path_node(RADDBGIC_BakeCtx *bctx, RADDBGIC_PathNode *path_node);

//- rjf: per-unit line info baking
RADDBGI_PROC RADDBGIC_UnitLinesCombined* raddbgic_unit_combine_lines(RADDBGIC_Arena *arena, RADDBGIC_BakeCtx *bctx, RADDBGIC_LineSequenceNode *first_seq);

//- rjf: per-src line info baking
RADDBGI_PROC RADDBGIC_SrcLinesCombined* raddbgic_source_combine_lines(RADDBGIC_Arena *arena, RADDBGIC_LineMapFragment *first);

//- rjf: vmap baking
RADDBGI_PROC RADDBGIC_VMap* raddbgic_vmap_from_markers(RADDBGIC_Arena *arena, RADDBGIC_VMapMarker *markers, RADDBGIC_SortKey *keys, RADDBGI_U64 marker_count);
RADDBGI_PROC RADDBGIC_VMap* raddbgic_vmap_from_unit_ranges(RADDBGIC_Arena *arena, RADDBGIC_UnitVMapRange *first, RADDBGI_U64 count);

//- rjf: type info baking
RADDBGI_PROC RADDBGI_U32* raddbgic_idx_run_from_types(RADDBGIC_Arena *arena, RADDBGIC_Type **types, RADDBGI_U32 count);
RADDBGI_PROC RADDBGIC_TypeData* raddbgic_type_data_combine(RADDBGIC_Arena *arena, RADDBGIC_Root *root, RADDBGIC_BakeCtx *bctx);

//- rjf: symbol data baking
RADDBGI_PROC RADDBGIC_SymbolData* raddbgic_symbol_data_combine(RADDBGIC_Arena *arena, RADDBGIC_Root *root, RADDBGIC_BakeCtx *bctx);

//- rjf: name map baking
RADDBGI_PROC RADDBGIC_NameMapBaked* raddbgic_name_map_bake(RADDBGIC_Arena *arena, RADDBGIC_Root *root, RADDBGIC_BakeCtx *bctx, RADDBGIC_NameMap *map);

//- rjf: top-level baking entry point
RADDBGI_PROC void raddbgic_bake_file(RADDBGIC_Arena *arena, RADDBGIC_Root *root, RADDBGIC_String8List *out);

#endif // RADDBGI_CONS_H
