// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////////////////////////////////////
// RAD Debug Info Make, (R)AD(D)BG(I) (M)ake Library
//
// Library for building loose data structures which contain
// RADDBGI debug information, and baking that down into the
// proper flattened RADDBGI format.
//
// Requires prior inclusion of the RAD Debug Info, (R)AD(D)BG(I)
// Format Library, in raddbgi_format.h.

#ifndef RADDBGI_MAKE_H
#define RADDBGI_MAKE_H

////////////////////////////////
//~ rjf: Overrideable Memory Operations

// To override the slow/default memset implementation used by the library,
// do the following:
//
// #define RDIM_MEMSET_OVERRIDE
// #define rdim_memset <name of memset implementation>

#if !defined(rdim_memset)
# define rdim_memset rdim_memset_fallback
#endif

// To override the slow/default memcpy implementation used by the library,
// do the following:
//
// #define RDIM_MEMCPY_OVERRIDE
// #define rdim_memcpy <name of memcpy implementation>

#if !defined(rdim_memset)
# define rdim_memset rdim_memset_fallback
#endif

#if !defined(rdim_memcpy)
# define rdim_memcpy rdim_memcpy_fallback
#endif

////////////////////////////////
//~ rjf: Overrideable sprintf Functions

#if !defined(rdim_vsnprintf)
# include <string.h>
# define rdim_vsnprintf vsnprintf
#endif

////////////////////////////////
//~ rjf: Overrideable String View Types

// To override the string view type used by the library, do the following:
//
// #define RDIM_STRING8_OVERRIDE
// #define RDIM_String8 <name of your string type here>
// #define RDIM_String8_BaseMember <name of base pointer member>
// #define RDIM_String8_SizeMember <name of size member>

// To override the string view list type used by the library, do the following:
//
// #define RDIM_STRING8LIST_OVERRIDE
// #define RDIM_String8Node <name of your string node here>
// #define RDIM_String8_NextPtrMember <name of member encoding next pointer>
// #define RDIM_String8_StringMember <name of node member containing string view>
// #define RDIM_String8List <name of your string list here>
// #define RDIM_String8_FirstMember <name of member encoding first pointer>
// #define RDIM_String8_LastMember <name of member encoding last pointer>
// #define RDIM_String8_NodeCount <name of U64 list member containing node count>
// #define RDIM_String8_TotalSizeMember <name of U64 list member containing total joined string size>

#if !defined(RDIM_String8)
#define RDIM_String8 RDIM_String8
#define RDIM_String8_BaseMember str
#define RDIM_String8_SizeMember size
typedef struct RDIM_String8 RDIM_String8;
struct RDIM_String8
{
  RDI_U8 *str;
  RDI_U64 size;
};
#endif

#if !defined(RDIM_String8Node)
#define RDIM_String8Node RDIM_String8Node
#define RDIM_String8Node_NextPtrMember next
#define RDIM_String8Node_StringMember string
typedef struct RDIM_String8Node RDIM_String8Node;
struct RDIM_String8Node
{
  RDIM_String8Node *next;
  RDIM_String8 string;
};
#endif

#if !defined(RDIM_String8List)
#define RDIM_String8List RDIM_String8List
#define RDIM_String8List_FirstMember first
#define RDIM_String8List_LastMember last
#define RDIM_String8List_NodeCountMember node_count
#define RDIM_String8List_TotalSizeMember total_size
typedef struct RDIM_String8List RDIM_String8List;
struct RDIM_String8List
{
  RDIM_String8Node *first;
  RDIM_String8Node *last;
  RDI_U64 node_count;
  RDI_U64 total_size;
};
#endif

typedef RDI_U32 RDIM_StringMatchFlags;
enum
{
  RDIM_StringMatchFlag_CaseInsensitive = (1<<0),
};

////////////////////////////////
//~ rjf: Overrideable Arena Allocator Types

// To override the arena allocator type used by the library, do the following:
//
// #define RDIM_ARENA_OVERRIDE
// #define RDIM_Arena <name of your arena type here>
// #define rdim_arena_alloc   <name of your creation function - must be (void) -> Arena*>
// #define rdim_arena_release <name of your release function  - must be (Arena*) -> void>
// #define rdim_arena_pos     <name of your position function - must be (Arena*) -> U64>
// #define rdim_arena_push    <name of your pushing function  - must be (Arena*, U64 size) -> void*>
// #define rdim_arena_pop_to  <name of your popping function  - must be (Arena*, U64 pos) -> void>

#if !defined(RDIM_Arena)
# define RDIM_Arena RDIM_Arena 
typedef struct RDIM_Arena RDIM_Arena;
struct RDIM_Arena
{
  RDIM_Arena *prev;
  RDIM_Arena *current;
  RDI_U64 base_pos;
  RDI_U64 pos;
  RDI_U64 cmt;
  RDI_U64 res;
  RDI_U64 align;
  RDI_S8 grow;
};
#endif

#if !defined(rdim_arena_alloc)
# define rdim_arena_alloc rdim_arena_alloc_fallback
#endif
#if !defined(rdim_arena_release)
# define rdim_arena_release rdim_arena_release_fallback
#endif
#if !defined(rdim_arena_pos)
# define rdim_arena_pos rdim_arena_pos_fallback
#endif
#if !defined(rdim_arena_push)
# define rdim_arena_push rdim_arena_push_fallback
#endif

////////////////////////////////
//~ rjf: Overrideable Thread-Local Scratch Arenas

// To override the default thread-local scratch arenas used by the library,
// do the following:
//
// #define RDIM_SCRATCH_OVERRIDE
// #define RDIM_Temp <name of arena temp block type - generally struct: (Arena*, U64)
// #define rdim_temp_arena <name of temp -> arena implementation - must be (Temp) -> (Arena*)>
// #define rdim_scratch_begin <name of scratch begin implementation - must be (Arena **conflicts, U64 conflict_count) -> Temp>
// #define rdim_scratch_end <name of scratch end function - must be (Temp) -> void

#if !defined(RDIM_Temp)
# define RDIM_Temp RDIM_Temp
typedef struct RDIM_Temp RDIM_Temp;
struct RDIM_Temp
{
  RDIM_Arena *arena;
  RDI_U64 pos;
};
#define rdim_temp_arena(t) ((t).arena)
#endif

#if !defined(rdim_scratch_begin)
# define rdim_scratch_begin rdim_scratch_begin_fallback
#endif
#if !defined(rdim_scratch_end)
# define rdim_scratch_end rdim_scratch_end_fallback
#endif

////////////////////////////////
//~ rjf: Linked List Helper Macros

#define RDIM_CheckNil(nil,p) ((p) == 0 || (p) == nil)
#define RDIM_SetNil(nil,p) ((p) = nil)

//- rjf: Base Doubly-Linked-List Macros
#define RDIM_DLLInsert_NPZ(nil,f,l,p,n,next,prev) (RDIM_CheckNil(nil,f) ? \
((f) = (l) = (n), RDIM_SetNil(nil,(n)->next), RDIM_SetNil(nil,(n)->prev)) :\
RDIM_CheckNil(nil,p) ? \
((n)->next = (f), (f)->prev = (n), (f) = (n), RDIM_SetNil(nil,(n)->prev)) :\
((p)==(l)) ? \
((l)->next = (n), (n)->prev = (l), (l) = (n), RDIM_SetNil(nil, (n)->next)) :\
(((!RDIM_CheckNil(nil,p) && RDIM_CheckNil(nil,(p)->next)) ? (0) : ((p)->next->prev = (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define RDIM_DLLPushBack_NPZ(nil,f,l,n,next,prev) RDIM_DLLInsert_NPZ(nil,f,l,l,n,next,prev)
#define RDIM_DLLPushFront_NPZ(nil,f,l,n,next,prev) RDIM_DLLInsert_NPZ(nil,l,f,f,n,prev,next)
#define RDIM_DLLRemove_NPZ(nil,f,l,n,next,prev) (((n) == (f) ? (f) = (n)->next : (0)),\
((n) == (l) ? (l) = (l)->prev : (0)),\
(RDIM_CheckNil(nil,(n)->prev) ? (0) :\
((n)->prev->next = (n)->next)),\
(RDIM_CheckNil(nil,(n)->next) ? (0) :\
((n)->next->prev = (n)->prev)))

//- rjf: Base Singly-Linked-List Queue Macros
#define RDIM_SLLQueuePush_NZ(nil,f,l,n,next) (RDIM_CheckNil(nil,f)?\
((f)=(l)=(n),RDIM_SetNil(nil,(n)->next)):\
((l)->next=(n),(l)=(n),RDIM_SetNil(nil,(n)->next)))
#define RDIM_SLLQueuePushFront_NZ(nil,f,l,n,next) (RDIM_CheckNil(nil,f)?\
((f)=(l)=(n),RDIM_SetNil(nil,(n)->next)):\
((n)->next=(f),(f)=(n)))
#define RDIM_SLLQueuePop_NZ(nil,f,l,next) ((f)==(l)?\
(RDIM_SetNil(nil,f), RDIM_SetNil(nil,l)):\
((f)=(f)->next))

//- rjf: Base Singly-Linked-List Stack Macros
#define RDIM_SLLStackPush_N(f,n,next) ((n)->next=(f), (f)=(n))
#define RDIM_SLLStackPop_N(f,next) ((f)=(f)->next)

////////////////////////////////
//~ rjf: Convenience Wrappers

//- rjf: Doubly-Linked-List Wrappers
#define RDIM_DLLInsert_NP(f,l,p,n,next,prev) RDIM_DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define RDIM_DLLPushBack_NP(f,l,n,next,prev) RDIM_DLLPushBack_NPZ(0,f,l,n,next,prev)
#define RDIM_DLLPushFront_NP(f,l,n,next,prev) RDIM_DLLPushFront_NPZ(0,f,l,n,next,prev)
#define RDIM_DLLRemove_NP(f,l,n,next,prev) RDIM_DLLRemove_NPZ(0,f,l,n,next,prev)
#define RDIM_DLLInsert(f,l,p,n) RDIM_DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define RDIM_DLLPushBack(f,l,n) RDIM_DLLPushBack_NPZ(0,f,l,n,next,prev)
#define RDIM_DLLPushFront(f,l,n) RDIM_DLLPushFront_NPZ(0,f,l,n,next,prev)
#define RDIM_DLLRemove(f,l,n) RDIM_DLLRemove_NPZ(0,f,l,n,next,prev)

//- rjf: Singly-Linked-List Queue Wrappers
#define RDIM_SLLQueuePush_N(f,l,n,next) RDIM_SLLQueuePush_NZ(0,f,l,n,next)
#define RDIM_SLLQueuePushFront_N(f,l,n,next) RDIM_SLLQueuePushFront_NZ(0,f,l,n,next)
#define RDIM_SLLQueuePop_N(f,l,next) RDIM_SLLQueuePop_NZ(0,f,l,next)
#define RDIM_SLLQueuePush(f,l,n) RDIM_SLLQueuePush_NZ(0,f,l,n,next)
#define RDIM_SLLQueuePushFront(f,l,n) RDIM_SLLQueuePushFront_NZ(0,f,l,n,next)
#define RDIM_SLLQueuePop(f,l) RDIM_SLLQueuePop_NZ(0,f,l,next)

//- rjf: Singly-Linked-List Stack Wrappers
#define RDIM_SLLStackPush(f,n) RDIM_SLLStackPush_N(f,n,next)
#define RDIM_SLLStackPop(f) RDIM_SLLStackPop_N(f,next)

////////////////////////////////
//~ rjf: Helper Macros

#if defined(_MSC_VER)
# define RDIM_THREAD_LOCAL __declspec(thread)
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
# define RDIM_THREAD_LOCAL __thread
#else
# error RDIM_THREAD_LOCAL not defined for this compiler.
#endif

#if defined(_MSC_VER)
# define rdim_trap() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
# define rdim_trap() __builtin_trap()
#else
# error "rdim_trap not defined for this compiler."
#endif

#define rdim_assert_always(x) do{if(!(x)) {rdim_trap();}}while(0)
#if !defined(NDEBUG)
# define rdim_assert(x) rdim_assert_always(x)
#else
# define rdim_assert(x) (void)(x)
#endif
#define rdim_noop ((void)0)

////////////////////////////////
//~ rjf: Auxiliary Data Structure Types

//- rjf: 1-dimensional U64 ranges

typedef struct RDIM_Rng1U64 RDIM_Rng1U64;
struct RDIM_Rng1U64
{
  RDI_U64 min;
  RDI_U64 max;
};

typedef struct RDIM_Rng1U64Node RDIM_Rng1U64Node;
struct RDIM_Rng1U64Node
{
  RDIM_Rng1U64Node *next;
  RDIM_Rng1U64 v;
};

typedef struct RDIM_Rng1U64List RDIM_Rng1U64List;
struct RDIM_Rng1U64List
{
  RDIM_Rng1U64Node *first;
  RDIM_Rng1U64Node *last;
  RDI_U64 count;
};

//- rjf: u64 -> pointer map

typedef struct RDIM_U64ToPtrNode RDIM_U64ToPtrNode;
struct RDIM_U64ToPtrNode
{
  RDIM_U64ToPtrNode *next;
  RDI_U64 _padding_;
  RDI_U64 key[1];
  void *ptr[1];
};

typedef struct RDIM_U64ToPtrMap RDIM_U64ToPtrMap;
struct RDIM_U64ToPtrMap
{
  RDIM_U64ToPtrNode **buckets;
  RDI_U64 buckets_count;
  RDI_U64 bucket_collision_count;
  RDI_U64 pair_count;
};

typedef struct RDIM_U64ToPtrLookup RDIM_U64ToPtrLookup;
struct RDIM_U64ToPtrLookup
{
  void *match;
  RDIM_U64ToPtrNode *fill_node;
  RDI_U32 fill_k;
};

//- rjf: string8 -> pointer map

typedef struct RDIM_Str8ToPtrNode RDIM_Str8ToPtrNode;
struct RDIM_Str8ToPtrNode
{
  struct RDIM_Str8ToPtrNode *next;
  RDIM_String8 key;
  RDI_U64 hash;
  void *ptr;
};

typedef struct RDIM_Str8ToPtrMap RDIM_Str8ToPtrMap;
struct RDIM_Str8ToPtrMap
{
  RDIM_Str8ToPtrNode **buckets;
  RDI_U64 buckets_count;
  RDI_U64 bucket_collision_count;
  RDI_U64 pair_count;
};

//- rjf: sortable range data structure

typedef struct RDIM_SortKey RDIM_SortKey;
struct RDIM_SortKey
{
  RDI_U64 key;
  void *val;
};

typedef struct RDIM_OrderedRange RDIM_OrderedRange;
struct RDIM_OrderedRange
{
  RDIM_OrderedRange *next;
  RDI_U64 first;
  RDI_U64 opl;
};

////////////////////////////////
//~ rjf: Error/Warning/Note Message Types

typedef struct RDIM_Msg RDIM_Msg;
struct RDIM_Msg
{
  RDIM_Msg *next;
  RDIM_String8 string;
};

typedef struct RDIM_MsgList RDIM_MsgList;
struct RDIM_MsgList
{
  RDIM_Msg *first;
  RDIM_Msg *last;
  RDI_U64 count;
};

////////////////////////////////
//~ rjf: Top-Level Debug Info Types

typedef struct RDIM_TopLevelInfo RDIM_TopLevelInfo;
struct RDIM_TopLevelInfo
{
  RDI_Arch arch;
  RDIM_String8 exe_name;
  RDI_U64 exe_hash;
  RDI_U64 voff_max;
};

////////////////////////////////
//~ rjf: Binary Section Types

typedef struct RDIM_BinarySection RDIM_BinarySection;
struct RDIM_BinarySection
{
  RDIM_String8 name;
  RDI_BinarySectionFlags flags;
  RDI_U64 voff_first;
  RDI_U64 voff_opl;
  RDI_U64 foff_first;
  RDI_U64 foff_opl;
};

typedef struct RDIM_BinarySectionNode RDIM_BinarySectionNode;
struct RDIM_BinarySectionNode
{
  RDIM_BinarySectionNode *next;
  RDIM_BinarySection v;
};

typedef struct RDIM_BinarySectionList RDIM_BinarySectionList;
struct RDIM_BinarySectionList
{
  RDIM_BinarySectionNode *first;
  RDIM_BinarySectionNode *last;
  RDI_U64 count;
};

////////////////////////////////
//~ rjf: Per-Compilation-Unit Info Types

typedef struct RDIM_LineSequence RDIM_LineSequence;
struct RDIM_LineSequence
{
  RDIM_String8 file_name;
  RDI_U64 *voffs;     // [line_count + 1] (sorted)
  RDI_U32 *line_nums; // [line_count]
  RDI_U16 *col_nums;  // [2*line_count]
  RDI_U64 line_count;
};

typedef struct RDIM_LineSequenceNode RDIM_LineSequenceNode;
struct RDIM_LineSequenceNode
{
  RDIM_LineSequenceNode *next;
  RDIM_LineSequence v;
};

typedef struct RDIM_LineSequenceList RDIM_LineSequenceList;
struct RDIM_LineSequenceList
{
  RDIM_LineSequenceNode *first;
  RDIM_LineSequenceNode *last;
  RDI_U64 count;
};

typedef struct RDIM_Unit RDIM_Unit;
struct RDIM_Unit
{
  RDIM_String8 unit_name;
  RDIM_String8 compiler_name;
  RDIM_String8 source_file;
  RDIM_String8 object_file;
  RDIM_String8 archive_file;
  RDIM_String8 build_path;
  RDI_Language language;
  RDIM_LineSequenceList line_sequences;
  RDIM_Rng1U64List voff_ranges;
};

typedef struct RDIM_UnitChunkNode RDIM_UnitChunkNode;
struct RDIM_UnitChunkNode
{
  RDIM_UnitChunkNode *next;
  RDIM_Unit *v;
  RDI_U64 count;
  RDI_U64 cap;
};

typedef struct RDIM_UnitChunkList RDIM_UnitChunkList;
struct RDIM_UnitChunkList
{
  RDIM_UnitChunkNode *first;
  RDIM_UnitChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
};

typedef struct RDIM_UnitArray RDIM_UnitArray;
struct RDIM_UnitArray
{
  RDIM_Unit *v;
  RDI_U64 count;
};

////////////////////////////////
//~ rjf: Type System Node Types

typedef struct RDIM_Type RDIM_Type;
struct RDIM_Type
{
  RDI_TypeKind kind;
  RDI_U32 byte_size;
  RDI_U32 flags;
  RDI_U32 off;
  RDI_U32 count;
  RDIM_String8 name;
  RDIM_Type *direct_type;
  RDIM_Type **param_types;
  struct RDIM_UDT *udt;
};

typedef struct RDIM_TypeChunkNode RDIM_TypeChunkNode;
struct RDIM_TypeChunkNode
{
  RDIM_TypeChunkNode *next;
  RDIM_Type *v;
  RDI_U64 count;
  RDI_U64 cap;
};

typedef struct RDIM_TypeChunkList RDIM_TypeChunkList;
struct RDIM_TypeChunkList
{
  RDIM_TypeChunkNode *first;
  RDIM_TypeChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
};

typedef struct RDIM_TypeArray RDIM_TypeArray;
struct RDIM_TypeArray
{
  RDIM_Type *v;
  RDI_U64 count;
};

////////////////////////////////
//~ rjf: User-Defined-Type Info Types

typedef struct RDIM_UDTMember RDIM_UDTMember;
struct RDIM_UDTMember
{
  RDIM_UDTMember *next;
  RDI_MemberKind kind;
  RDIM_String8 name;
  RDIM_Type *type;
  RDI_U32 off;
};

typedef struct RDIM_UDTEnumVal RDIM_UDTEnumVal;
struct RDIM_UDTEnumVal
{
  RDIM_UDTEnumVal *next;
  RDIM_String8 name;
  RDI_U64 val;
};

typedef struct RDIM_UDT RDIM_UDT;
struct RDIM_UDT
{
  RDIM_Type *self_type;
  RDIM_UDTMember *first_member;
  RDIM_UDTMember *last_member;
  RDI_U64 member_count;
  RDIM_UDTEnumVal *first_enum_val;
  RDIM_UDTEnumVal *last_enum_val;
  RDI_U64 enum_val_count;
  RDIM_String8 source_path;
  RDI_U32 line;
  RDI_U32 col;
};

typedef struct RDIM_UDTChunkNode RDIM_UDTChunkNode;
struct RDIM_UDTChunkNode
{
  RDIM_UDTChunkNode *next;
  RDIM_UDT *v;
  RDI_U64 count;
  RDI_U64 cap;
};

typedef struct RDIM_UDTChunkList RDIM_UDTChunkList;
struct RDIM_UDTChunkList
{
  RDIM_UDTChunkNode *first;
  RDIM_UDTChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
};

////////////////////////////////
//~ rjf: Location Info Types

typedef struct RDIM_EvalBytecodeOp RDIM_EvalBytecodeOp;
struct RDIM_EvalBytecodeOp
{
  RDIM_EvalBytecodeOp *next;
  RDI_EvalOp op;
  RDI_U32 p_size;
  RDI_U64 p;
};

typedef struct RDIM_EvalBytecode RDIM_EvalBytecode;
struct RDIM_EvalBytecode
{
  RDIM_EvalBytecodeOp *first_op;
  RDIM_EvalBytecodeOp *last_op;
  RDI_U32 op_count;
  RDI_U32 encoded_size;
};

typedef struct RDIM_Location RDIM_Location;
struct RDIM_Location
{
  RDI_LocationKind kind;
  RDI_U8 register_code;
  RDI_U16 offset;
  RDIM_EvalBytecode bytecode;
};

typedef struct RDIM_LocationCase RDIM_LocationCase;
struct RDIM_LocationCase
{
  RDIM_LocationCase *next;
  RDI_U64 voff_first;
  RDI_U64 voff_opl;
  RDIM_Location *location;
};

typedef struct RDIM_LocationSet RDIM_LocationSet;
struct RDIM_LocationSet
{
  RDIM_LocationCase *first_location_case;
  RDIM_LocationCase *last_location_case;
  RDI_U64 location_case_count;
};

////////////////////////////////
//~ rjf: Symbol Info Types

typedef enum RDIM_SymbolKind
{
  RDIM_SymbolKind_NULL,
  RDIM_SymbolKind_GlobalVariable,
  RDIM_SymbolKind_ThreadVariable,
  RDIM_SymbolKind_Procedure,
  RDIM_SymbolKind_COUNT
}
RDIM_SymbolKind;

typedef struct RDIM_Symbol RDIM_Symbol;
struct RDIM_Symbol
{
  RDIM_SymbolKind kind;
  RDI_S32 is_extern;
  RDIM_String8 name;
  RDIM_String8 link_name;
  RDIM_Type *type;
  RDI_U64 offset;
  RDIM_Symbol *container_symbol;
  RDIM_Type *container_type;
  struct RDIM_Scope *root_scope;
};

typedef struct RDIM_SymbolChunkNode RDIM_SymbolChunkNode;
struct RDIM_SymbolChunkNode
{
  RDIM_SymbolChunkNode *next;
  RDIM_Symbol *v;
  RDI_U64 count;
  RDI_U64 cap;
};

typedef struct RDIM_SymbolChunkList RDIM_SymbolChunkList;
struct RDIM_SymbolChunkList
{
  RDIM_SymbolChunkNode *first;
  RDIM_SymbolChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
};

////////////////////////////////
//~ rjf: Scope Info Types

typedef struct RDIM_Local RDIM_Local;
struct RDIM_Local
{
  RDIM_Local *next;
  RDI_LocalKind kind;
  RDIM_String8 name;
  RDIM_Type *type;
  RDIM_LocationSet *locset;
};

typedef struct RDIM_Scope RDIM_Scope;
struct RDIM_Scope
{
  RDIM_Symbol *symbol;
  RDIM_Scope *parent_scope;
  RDIM_Scope *first_child;
  RDIM_Scope *last_child;
  RDIM_Scope *next_sibling;
  RDI_U64 voff_base;
  RDIM_Rng1U64List voff_ranges;
  RDIM_Local *first_local;
  RDIM_Local *last_local;
  RDI_U32 local_count;
};

typedef struct RDIM_ScopeChunkNode RDIM_ScopeChunkNode;
struct RDIM_ScopeChunkNode
{
  RDIM_ScopeChunkNode *next;
  RDIM_Scope *v;
  RDI_U64 count;
  RDI_U64 cap;
};

typedef struct RDIM_ScopeChunkList RDIM_ScopeChunkList;
struct RDIM_ScopeChunkList
{
  RDIM_ScopeChunkNode *first;
  RDIM_ScopeChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
};

////////////////////////////////
//~ rjf: Name Map Types

typedef struct RDIM_NameMapIdxNode RDIM_NameMapIdxNode;
struct RDIM_NameMapIdxNode
{
  RDIM_NameMapIdxNode *next;
  RDI_U32 idx[8];
};

typedef struct RDIM_NameMapNode RDIM_NameMapNode;
struct RDIM_NameMapNode
{
  RDIM_NameMapNode *bucket_next;
  RDIM_NameMapNode *order_next;
  RDIM_String8 string;
  RDIM_NameMapIdxNode *idx_first;
  RDIM_NameMapIdxNode *idx_last;
  RDI_U64 idx_count;
};

typedef struct RDIM_NameMap RDIM_NameMap;
struct RDIM_NameMap
{
  RDIM_NameMapNode **buckets;
  RDI_U64 buckets_count;
  RDI_U64 bucket_collision_count;
  RDIM_NameMapNode *first;
  RDIM_NameMapNode *last;
  RDI_U64 name_count;
};

////////////////////////////////
//~ rjf: Root Construction Bundle Types

typedef struct RDIM_RootParams RDIM_RootParams;
struct RDIM_RootParams
{
  RDI_U64 addr_size;
  RDI_U32 bucket_count_units;              // optional; default chosen if 0
  RDI_U32 bucket_count_symbols;            // optional; default chosen if 0
  RDI_U32 bucket_count_scopes;             // optional; default chosen if 0
  RDI_U32 bucket_count_locals;             // optional; default chosen if 0
  RDI_U32 bucket_count_types;              // optional; default chosen if 0
  RDI_U64 bucket_count_type_constructs;    // optional; default chosen if 0
};

typedef struct RDIM_Root RDIM_Root;
struct RDIM_Root
{
  RDIM_Arena *arena;
  RDIM_MsgList msgs;
  
  //////// Contextual Information
  
  RDI_U64 addr_size;
  
  //////// Info Declared By User
  
  RDI_U64 total_member_count;
  RDI_U64 total_enum_val_count;
  
  // symbols
  RDIM_Symbol *first_symbol;
  RDIM_Symbol *last_symbol;
  union
  {
    RDI_U64 symbol_count;
    RDI_U64 symbol_kind_counts[RDIM_SymbolKind_COUNT];
  };
  
  RDIM_Scope *first_scope;
  RDIM_Scope *last_scope;
  RDI_U64 scope_count;
  RDI_U64 scope_voff_count;
  
  RDIM_Local *first_local;
  RDIM_Local *last_local;
  RDI_U64 local_count;
  RDI_U64 location_count;
  
  // name maps
  RDIM_NameMap *name_maps[RDI_NameMapKind_COUNT];
  
  //////// Handle Relationship Maps
  
  RDIM_U64ToPtrMap unit_map;
  RDIM_U64ToPtrMap symbol_map;
  RDIM_U64ToPtrMap scope_map;
  RDIM_U64ToPtrMap local_map;
  RDIM_U64ToPtrMap type_from_id_map;
  RDIM_Str8ToPtrMap construct_map;
};

////////////////////////////////
//~ rjf: Baking Phase Types

//- rjf: bake data section data structure

typedef struct RDIM_DSectionNode RDIM_DSectionNode;
struct RDIM_DSectionNode
{
  RDIM_DSectionNode *next;
  void *data;
  RDI_U64 size;
  RDI_DataSectionTag tag;
};

typedef struct RDIM_DSections RDIM_DSections;
struct RDIM_DSections
{
  RDIM_DSectionNode *first;
  RDIM_DSectionNode *last;
  RDI_U32 count;
};

//- rjf: bake string data structure

typedef struct RDIM_StringNode RDIM_StringNode;
struct RDIM_StringNode
{
  RDIM_StringNode *order_next;
  RDIM_StringNode *bucket_next;
  RDIM_String8 str;
  RDI_U64 hash;
  RDI_U32 idx;
};

typedef struct RDIM_Strings RDIM_Strings;
struct RDIM_Strings
{
  RDIM_StringNode *order_first;
  RDIM_StringNode *order_last;
  RDIM_StringNode **buckets;
  RDI_U64 buckets_count;
  RDI_U64 bucket_collision_count;
  RDI_U32 count;
};

//- rjf: index run baking data structure

typedef struct RDIM_IdxRunNode RDIM_IdxRunNode;
struct RDIM_IdxRunNode
{
  RDIM_IdxRunNode *order_next;
  RDIM_IdxRunNode *bucket_next;
  RDI_U32 *idx_run;
  RDI_U64 hash;
  RDI_U32 count;
  RDI_U32 first_idx;
};

typedef struct RDIM_IdxRuns RDIM_IdxRuns;
struct RDIM_IdxRuns
{
  RDIM_IdxRunNode *order_first;
  RDIM_IdxRunNode *order_last;
  RDIM_IdxRunNode **buckets;
  RDI_U64 buckets_count;
  RDI_U64 bucket_collision_count;
  RDI_U32 count;
  RDI_U32 idx_count;
};

//- rjf: source file & file path baking data structures

typedef struct RDIM_PathNode RDIM_PathNode;
struct RDIM_PathNode
{
  RDIM_PathNode *next_order;
  RDIM_PathNode *parent;
  RDIM_PathNode *first_child;
  RDIM_PathNode *last_child;
  RDIM_PathNode *next_sibling;
  RDIM_String8 name;
  struct RDIM_SrcNode *src_file;
  RDI_U32 idx;
};

typedef struct RDIM_LineMapFragment RDIM_LineMapFragment;
struct RDIM_LineMapFragment
{
  RDIM_LineMapFragment *next;
  RDIM_LineSequenceNode *sequence;
};

typedef struct RDIM_SrcNode RDIM_SrcNode;
struct RDIM_SrcNode
{
  RDIM_SrcNode *next;
  RDIM_PathNode *path_node;
  RDI_U32 idx;
  
  RDIM_String8 normal_full_path;
  
  // place to gather the line info attached to this src file
  RDIM_LineMapFragment *first_fragment;
  RDIM_LineMapFragment *last_fragment;
  
  // place to put the final baked version of this file's line map
  RDI_U32 line_map_nums_data_idx;
  RDI_U32 line_map_range_data_idx;
  RDI_U32 line_map_count;
  RDI_U32 line_map_voff_data_idx;
};

typedef struct RDIM_PathTree RDIM_PathTree;
struct RDIM_PathTree
{
  RDIM_PathNode *first;
  RDIM_PathNode *last;
  RDI_U32 count;
  RDIM_PathNode root;
  RDIM_SrcNode *src_first;
  RDIM_SrcNode *src_last;
  RDI_U32 src_count;
};

//- rjf: line info baking data structures

typedef struct RDIM_LineRec RDIM_LineRec;
struct RDIM_LineRec
{
  RDI_U32 file_id;
  RDI_U32 line_num;
  RDI_U16 col_first;
  RDI_U16 col_opl;
};

typedef struct RDIM_UnitLinesCombined RDIM_UnitLinesCombined;
struct RDIM_UnitLinesCombined
{
  RDI_U64 *voffs;
  RDI_Line *lines;
  RDI_U16 *cols;
  RDI_U32 line_count;
};

typedef struct RDIM_SrcLinesCombined RDIM_SrcLinesCombined;
struct RDIM_SrcLinesCombined
{
  RDI_U32 *line_nums;
  RDI_U32 *line_ranges;
  RDI_U64 *voffs;
  RDI_U32  line_count;
  RDI_U32  voff_count;
};

typedef struct RDIM_SrcLineMapVoffBlock RDIM_SrcLineMapVoffBlock;
struct RDIM_SrcLineMapVoffBlock
{
  RDIM_SrcLineMapVoffBlock *next;
  RDI_U64 voff;
};

typedef struct RDIM_SrcLineMapBucket RDIM_SrcLineMapBucket;
struct RDIM_SrcLineMapBucket
{
  RDIM_SrcLineMapBucket *order_next;
  RDIM_SrcLineMapBucket *hash_next;
  RDI_U32 line_num;
  RDIM_SrcLineMapVoffBlock *first_voff_block;
  RDIM_SrcLineMapVoffBlock *last_voff_block;
  RDI_U64 voff_count;
};

//- rjf: vmap baking data structure 

typedef struct RDIM_VMap RDIM_VMap;
struct RDIM_VMap
{
  RDI_VMapEntry *vmap; // [count + 1]
  RDI_U32 count;
};

typedef struct RDIM_VMapMarker RDIM_VMapMarker;
struct RDIM_VMapMarker
{
  RDI_U32 idx;
  RDI_U32 begin_range;
};

typedef struct RDIM_VMapRangeTracker RDIM_VMapRangeTracker;
struct RDIM_VMapRangeTracker
{
  RDIM_VMapRangeTracker *next;
  RDI_U32 idx;
};

//- rjf: type data baking types

typedef struct RDIM_TypeData RDIM_TypeData;
struct RDIM_TypeData
{
  RDI_TypeNode *type_nodes;
  RDI_U32 type_node_count;
  
  RDI_UDT *udts;
  RDI_U32 udt_count;
  
  RDI_Member *members;
  RDI_U32 member_count;
  
  RDI_EnumMember *enum_members;
  RDI_U32 enum_member_count;
};

//- rjf: symbol data baking types

typedef struct RDIM_SymbolData RDIM_SymbolData;
struct RDIM_SymbolData
{
  RDI_GlobalVariable *global_variables;
  RDI_U32 global_variable_count;
  
  RDIM_VMap *global_vmap;
  
  RDI_ThreadVariable *thread_variables;
  RDI_U32 thread_variable_count;
  
  RDI_Procedure *procedures;
  RDI_U32 procedure_count;
  
  RDI_Scope *scopes;
  RDI_U32 scope_count;
  
  RDI_U64 *scope_voffs;
  RDI_U32 scope_voff_count;
  
  RDIM_VMap *scope_vmap;
  
  RDI_Local *locals;
  RDI_U32 local_count;
  
  RDI_LocationBlock *location_blocks;
  RDI_U32 location_block_count;
  
  void *location_data;
  RDI_U32 location_data_size;
};

//- rjf: name map baking types

typedef struct RDIM_NameMapSemiNode RDIM_NameMapSemiNode;
struct RDIM_NameMapSemiNode
{
  RDIM_NameMapSemiNode *next;
  RDIM_NameMapNode *node;
};

typedef struct RDIM_NameMapSemiBucket RDIM_NameMapSemiBucket;
struct RDIM_NameMapSemiBucket
{
  RDIM_NameMapSemiNode *first;
  RDIM_NameMapSemiNode *last;
  RDI_U64 count;
};

typedef struct RDIM_NameMapBaked RDIM_NameMapBaked;
struct RDIM_NameMapBaked
{
  RDI_NameMapBucket *buckets;
  RDI_NameMapNode *nodes;
  RDI_U32 bucket_count;
  RDI_U32 node_count;
};

//- rjf: bundle baking context type

typedef struct RDIM_BakeParams RDIM_BakeParams;
struct RDIM_BakeParams
{
  RDI_U64 strings_bucket_count;
  RDI_U64 idx_runs_bucket_count;
};

typedef struct RDIM_BakeCtx RDIM_BakeCtx;
struct RDIM_BakeCtx
{
  RDIM_Arena *arena;
  RDIM_Strings strs;
  RDIM_IdxRuns idxs;
  RDIM_PathTree *tree;
};

////////////////////////////////
//~ rjf: Basic Helpers

//- rjf: memory operations
#if !defined(RDIM_MEMSET_OVERRIDE)
RDI_PROC void *rdim_memset_fallback(void *dst, RDI_U8 c, RDI_U64 size);
#endif
#if !defined(RDIM_MEMCPY_OVERRIDE)
RDI_PROC void *rdim_memcpy_fallback(void *dst, void *src, RDI_U64 size);
#endif
#define rdim_memzero(ptr, size) rdim_memset((ptr), 0, (size))
#define rdim_memzero_struct(ptr) rdim_memset((ptr), 0, sizeof(*(ptr)))
#define rdim_memcpy_struct(dst, src) rdim_memcpy((dst), (src), sizeof(*(dst)))

//- rjf: arenas
#if !defined(RDIM_ARENA_OVERRIDE)
RDI_PROC RDIM_Arena *rdim_arena_alloc_fallback(void);
RDI_PROC void rdim_arena_release_fallback(RDIM_Arena *arena);
RDI_PROC RDI_U64 rdim_arena_pos_fallback(RDIM_Arena *arena);
RDI_PROC void *rdim_arena_push_fallback(RDIM_Arena *arena, RDI_U64 size);
RDI_PROC void rdim_arena_pop_to_fallback(RDIM_Arena *arena, RDI_U64 pos);
#endif
#define rdim_push_array_no_zero(a,T,c) (T*)rdim_arena_push((a), sizeof(T)*(c))
#define rdim_push_array(a,T,c) (T*)rdim_memzero(rdim_push_array_no_zero(a,T,c), sizeof(T)*(c))

//- rjf: thread-local scratch arenas
#if !defined (RDIM_SCRATCH_OVERRIDE)
RDI_PROC RDIM_Temp rdim_scratch_begin_fallback(RDIM_Arena **conflicts, RDI_U64 conflicts_count);
RDI_PROC void rdim_scratch_end_fallback(RDIM_Temp temp);
#endif

//- rjf: strings
RDI_PROC RDIM_String8 rdim_str8(RDI_U8 *str, RDI_U64 size);
RDI_PROC RDIM_String8 rdim_str8_copy(RDIM_Arena *arena, RDIM_String8 src);
RDI_PROC RDIM_String8 rdim_str8f(RDIM_Arena *arena, char *fmt, ...);
RDI_PROC RDIM_String8 rdim_str8fv(RDIM_Arena *arena, char *fmt, va_list args);
RDI_PROC RDI_S32 rdim_str8_match(RDIM_String8 a, RDIM_String8 b, RDIM_StringMatchFlags flags);
#define rdim_str8_lit(S)    rdim_str8((RDI_U8*)(S), sizeof(S) - 1)
#define rdim_str8_struct(S) rdim_str8((RDI_U8*)(S), sizeof(*(S)))

//- rjf: string lists
RDI_PROC void rdim_str8_list_push(RDIM_Arena *arena, RDIM_String8List *list, RDIM_String8 string);
RDI_PROC RDIM_String8 rdim_str8_list_join(RDIM_Arena *arena, RDIM_String8List *list, RDIM_String8 sep);

//- rjf: sortable range sorting
RDI_PROC RDIM_SortKey* rdim_sort_key_array(RDIM_Arena *arena, RDIM_SortKey *keys, RDI_U64 count);

////////////////////////////////
//~ rjf: Auxiliary Data Structure Functions

//- rjf: rng1u64 list
RDI_PROC void rdim_rng1u64_list_push(RDIM_Arena *arena, RDIM_Rng1U64List *list, RDIM_Rng1U64 r);

//- rjf: u64 -> ptr map
RDI_PROC void rdim_u64toptr_map_init(RDIM_Arena *arena, RDIM_U64ToPtrMap *map, RDI_U64 bucket_count);
RDI_PROC void rdim_u64toptr_map_lookup(RDIM_U64ToPtrMap *map, RDI_U64 key, RDI_U64 hash, RDIM_U64ToPtrLookup *lookup_out);
RDI_PROC void rdim_u64toptr_map_insert(RDIM_Arena *arena, RDIM_U64ToPtrMap *map, RDI_U64 key, RDI_U64 hash, RDIM_U64ToPtrLookup *lookup, void *ptr);

//- rjf: string8 -> ptr map
RDI_PROC void rdim_str8toptr_map_init(RDIM_Arena *arena, RDIM_Str8ToPtrMap *map, RDI_U64 bucket_count);
RDI_PROC void*rdim_str8toptr_map_lookup(RDIM_Str8ToPtrMap *map, RDIM_String8 key, RDI_U64 hash);
RDI_PROC void rdim_str8toptr_map_insert(RDIM_Arena *arena, RDIM_Str8ToPtrMap *map, RDIM_String8 key, RDI_U64 hash, void *ptr);

////////////////////////////////
//~ rjf: Binary Section Info Building

RDI_PROC RDIM_BinarySection *rdim_binary_section_list_push(RDIM_Arena *arena, RDIM_BinarySectionList *list);

////////////////////////////////
//~ rjf: Unit Info Building

RDI_PROC RDIM_Unit *rdim_unit_chunk_list_push(RDIM_Arena *arena, RDIM_UnitChunkList *list);
RDI_PROC RDIM_LineSequence *rdim_line_sequence_list_push(RDIM_Arena *arena, RDIM_LineSequenceList *list);
RDI_PROC RDIM_UnitArray rdim_unit_array_from_chunk_list(RDIM_Arena *arena, RDIM_UnitChunkList *list);

////////////////////////////////
//~ rjf: Type Info & UDT Building

RDI_PROC RDIM_Type *rdim_type_chunk_list_push(RDIM_Arena *arena, RDIM_TypeChunkList *list, RDI_U64 cap);
RDI_PROC RDIM_UDT *rdim_udt_chunk_list_push(RDIM_Arena *arena, RDIM_UDTChunkList *list, RDI_U64 cap);
RDI_PROC RDIM_UDTMember *rdim_udt_push_member(RDIM_Arena *arena, RDIM_UDT *udt);
RDI_PROC RDIM_UDTEnumVal *rdim_udt_push_enum_val(RDIM_Arena *arena, RDIM_UDT *udt);

////////////////////////////////
//~ rjf: Location Info Building

RDI_PROC void rdim_bytecode_push_op(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_EvalOp op, RDI_U64 p);
RDI_PROC void rdim_bytecode_push_uconst(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_U64 x);
RDI_PROC void rdim_bytecode_push_sconst(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_S64 x);
RDI_PROC void rdim_bytecode_concat_in_place(RDIM_EvalBytecode *left_dst, RDIM_EvalBytecode *right_destroyed);

////////////////////////////////
//~ rjf: Symbol Info Building

RDI_PROC RDIM_Symbol *rdim_symbol_chunk_list_push(RDIM_Arena *arena, RDIM_SymbolChunkList *list, RDI_U64 cap);

////////////////////////////////
//~ rjf: Scope Info Building

RDI_PROC RDIM_Scope *rdim_scope_chunk_list_push(RDIM_Arena *arena, RDIM_ScopeChunkList *list, RDI_U64 cap);


#if 0
////////////////////////////////
//~ rjf: Loose Debug Info Construction (Anything -> Loose) Functions

//- rjf: root creation
RDI_PROC RDIM_Root* rdim_root_alloc(RDIM_RootParams *params);
RDI_PROC void       rdim_root_release(RDIM_Root *root);

//- rjf: error accumulation
RDI_PROC void rdim_push_msg(RDIM_Root *root, RDIM_String8 string);
RDI_PROC void rdim_push_msgf(RDIM_Root *root, char *fmt, ...);
RDI_PROC RDIM_Msg* rdim_first_msg_from_root(RDIM_Root *root);

//- rjf: type info lookups/reservations
RDI_PROC RDIM_Type*        rdim_type_from_id(RDIM_Root *root, RDI_U64 type_user_id, RDI_U64 type_user_id_hash);
RDI_PROC RDIM_Reservation* rdim_type_reserve_id(RDIM_Root *root, RDI_U64 type_user_id, RDI_U64 type_user_id_hash);
RDI_PROC void              rdim_type_fill_id(RDIM_Root *root, RDIM_Reservation *res, RDIM_Type *type);

//- rjf: nil/singleton types
RDI_PROC RDI_S32    rdim_type_is_unhandled_nil(RDIM_Root *root, RDIM_Type *type);
RDI_PROC RDIM_Type* rdim_type_handled_nil(RDIM_Root *root);
RDI_PROC RDIM_Type* rdim_type_nil(RDIM_Root *root);
RDI_PROC RDIM_Type* rdim_type_variadic(RDIM_Root *root);

//- rjf: base type info constructors
RDI_PROC RDIM_Type*    rdim_type_new(RDIM_Root *root);
RDI_PROC RDIM_TypeUDT* rdim_type_udt_from_any_type(RDIM_Root *root, RDIM_Type *type);
RDI_PROC RDIM_TypeUDT* rdim_type_udt_from_record_type(RDIM_Root *root, RDIM_Type *type);

//- rjf: basic/operator type construction helpers
RDI_PROC RDIM_Type* rdim_type_basic(RDIM_Root *root, RDI_TypeKind type_kind, RDIM_String8 name);
RDI_PROC RDIM_Type* rdim_type_modifier(RDIM_Root *root, RDIM_Type *direct_type, RDI_TypeModifierFlags flags);
RDI_PROC RDIM_Type* rdim_type_bitfield(RDIM_Root *root, RDIM_Type *direct_type, RDI_U32 bit_off, RDI_U32 bit_count);
RDI_PROC RDIM_Type* rdim_type_pointer(RDIM_Root *root, RDIM_Type *direct_type, RDI_TypeKind ptr_type_kind);
RDI_PROC RDIM_Type* rdim_type_array(RDIM_Root *root, RDIM_Type *direct_type, RDI_U64 count);
RDI_PROC RDIM_Type* rdim_type_proc(RDIM_Root *root, RDIM_Type *return_type, struct RDIM_TypeList *params);
RDI_PROC RDIM_Type* rdim_type_method(RDIM_Root *root, RDIM_Type *this_type, RDIM_Type *return_type, struct RDIM_TypeList *params);

//- rjf: udt type constructors
RDI_PROC RDIM_Type* rdim_type_udt(RDIM_Root *root, RDI_TypeKind record_type_kind, RDIM_String8 name, RDI_U64 size);
RDI_PROC RDIM_Type* rdim_type_enum(RDIM_Root *root, RDIM_Type *direct_type, RDIM_String8 name);
RDI_PROC RDIM_Type* rdim_type_alias(RDIM_Root *root, RDIM_Type *direct_type, RDIM_String8 name);
RDI_PROC RDIM_Type* rdim_type_incomplete(RDIM_Root *root, RDI_TypeKind type_kind, RDIM_String8 name);

//- rjf: type member building
RDI_PROC void rdim_type_add_member_data_field(RDIM_Root *root, RDIM_Type *record_type, RDIM_String8 name, RDIM_Type *mem_type, RDI_U32 off);
RDI_PROC void rdim_type_add_member_static_data(RDIM_Root *root, RDIM_Type *record_type, RDIM_String8 name, RDIM_Type *mem_type);
RDI_PROC void rdim_type_add_member_method(RDIM_Root *root, RDIM_Type *record_type, RDIM_String8 name, RDIM_Type *mem_type);
RDI_PROC void rdim_type_add_member_static_method(RDIM_Root *root, RDIM_Type *record_type, RDIM_String8 name, RDIM_Type *mem_type);
RDI_PROC void rdim_type_add_member_virtual_method(RDIM_Root *root, RDIM_Type *record_type, RDIM_String8 name, RDIM_Type *mem_type);
RDI_PROC void rdim_type_add_member_base(RDIM_Root *root, RDIM_Type *record_type, RDIM_Type *base_type, RDI_U32 off);
RDI_PROC void rdim_type_add_member_virtual_base(RDIM_Root *root, RDIM_Type *record_type, RDIM_Type *base_type, RDI_U32 vptr_off, RDI_U32 vtable_off);
RDI_PROC void rdim_type_add_member_nested_type(RDIM_Root *root, RDIM_Type *record_type, RDIM_Type *nested_type);
RDI_PROC void rdim_type_add_enum_val(RDIM_Root *root, RDIM_Type *enum_type, RDIM_String8 name, RDI_U64 val);

//- rjf: type source coordinate specifications
RDI_PROC void rdim_type_set_source_coordinates(RDIM_Root *root, RDIM_Type *defined_type, RDIM_String8 source_path, RDI_U32 line, RDI_U32 col);

//- rjf: symbol info building
RDI_PROC RDIM_Symbol* rdim_symbol_handle_from_user_id(RDIM_Root *root, RDI_U64 symbol_user_id, RDI_U64 symbol_user_id_hash);
RDI_PROC void rdim_symbol_set_info(RDIM_Root *root, RDIM_Symbol *symbol, RDIM_SymbolInfo *info);

//- rjf: scope info building
RDI_PROC RDIM_Scope *rdim_scope_handle_from_user_id(RDIM_Root *root, RDI_U64 scope_user_id, RDI_U64 scope_user_id_hash);
RDI_PROC void rdim_scope_set_parent(RDIM_Root *root, RDIM_Scope *scope, RDIM_Scope *parent);
RDI_PROC void rdim_scope_add_voff_range(RDIM_Root *root, RDIM_Scope *scope, RDI_U64 voff_first, RDI_U64 voff_opl);
RDI_PROC void rdim_scope_recursive_set_symbol(RDIM_Scope *scope, RDIM_Symbol *symbol);

//- rjf: local info building
RDI_PROC RDIM_Local* rdim_local_handle_from_user_id(RDIM_Root *root, RDI_U64 local_user_id, RDI_U64 local_user_id_hash);
RDI_PROC void rdim_local_set_basic_info(RDIM_Root *root, RDIM_Local *local, RDIM_LocalInfo *info);
RDI_PROC RDIM_LocationSet* rdim_location_set_from_local(RDIM_Root *root, RDIM_Local *local);

//- rjf: location info building
RDI_PROC void rdim_location_set_add_case(RDIM_Root *root, RDIM_LocationSet *locset, RDI_U64 voff_first, RDI_U64 voff_opl, RDIM_Location *location);
RDI_PROC RDIM_Location* rdim_location_addr_bytecode_stream(RDIM_Root *root, struct RDIM_EvalBytecode *bytecode);
RDI_PROC RDIM_Location* rdim_location_val_bytecode_stream(RDIM_Root *root, struct RDIM_EvalBytecode *bytecode);
RDI_PROC RDIM_Location* rdim_location_addr_reg_plus_u16(RDIM_Root *root, RDI_U8 reg_code, RDI_U16 offset);
RDI_PROC RDIM_Location* rdim_location_addr_addr_reg_plus_u16(RDIM_Root *root, RDI_U8 reg_code, RDI_U16 offset);
RDI_PROC RDIM_Location* rdim_location_val_reg(RDIM_Root *root, RDI_U8 reg_code);

//- rjf: name map building
RDI_PROC RDIM_NameMap* rdim_name_map_for_kind(RDIM_Root *root, RDI_NameMapKind kind);
RDI_PROC void          rdim_name_map_add_pair(RDIM_Root *root, RDIM_NameMap *map, RDIM_String8 name, RDI_U32 idx);

////////////////////////////////
//~ rjf: Debug Info Baking (Loose -> Tight) Functions

//- rjf: bake context construction
RDI_PROC RDIM_BakeCtx* rdim_bake_ctx_begin(RDIM_BakeParams *params);
RDI_PROC void              rdim_bake_ctx_release(RDIM_BakeCtx *bake_ctx);

//- rjf: string baking
RDI_PROC RDI_U32 rdim_string(RDIM_BakeCtx *bctx, RDIM_String8 str);

//- rjf: idx run baking
RDI_PROC RDI_U64 rdim_idx_run_hash(RDI_U32 *idx_run, RDI_U32 count);
RDI_PROC RDI_U32 rdim_idx_run(RDIM_BakeCtx *bctx, RDI_U32 *idx_run, RDI_U32 count);

//- rjf: data section baking
RDI_PROC RDI_U32 rdim_dsection(RDIM_Arena *arena, RDIM_DSections *dss, void *data, RDI_U64 size, RDI_DataSectionTag tag);

//- rjf: paths baking
RDI_PROC RDIM_String8   rdim_normal_string_from_path_node(RDIM_Arena *arena, RDIM_PathNode *node);
RDI_PROC void           rdim_normal_string_from_path_node_build(RDIM_Arena *arena, RDIM_PathNode *node, RDIM_String8List *out);
RDI_PROC RDIM_PathNode* rdim_paths_new_node(RDIM_BakeCtx *bctx);
RDI_PROC RDIM_PathNode* rdim_paths_sub_path(RDIM_BakeCtx *bctx, RDIM_PathNode *dir, RDIM_String8 sub_dir);
RDI_PROC RDIM_PathNode* rdim_paths_node_from_path(RDIM_BakeCtx *bctx,  RDIM_String8 path);
RDI_PROC RDI_U32        rdim_paths_idx_from_path(RDIM_BakeCtx *bctx, RDIM_String8 path);
RDI_PROC RDIM_SrcNode*  rdim_paths_new_src_node(RDIM_BakeCtx *bctx);
RDI_PROC RDIM_SrcNode*  rdim_paths_src_node_from_path_node(RDIM_BakeCtx *bctx, RDIM_PathNode *path_node);

//- rjf: per-unit line info baking
RDI_PROC RDIM_UnitLinesCombined* rdim_unit_combine_lines(RDIM_Arena *arena, RDIM_BakeCtx *bctx, RDIM_LineSequenceNode *first_seq);

//- rjf: per-src line info baking
RDI_PROC RDIM_SrcLinesCombined* rdim_source_combine_lines(RDIM_Arena *arena, RDIM_LineMapFragment *first);

//- rjf: vmap baking
RDI_PROC RDIM_VMap* rdim_vmap_from_markers(RDIM_Arena *arena, RDIM_VMapMarker *markers, RDIM_SortKey *keys, RDI_U64 marker_count);
RDI_PROC RDIM_VMap* rdim_vmap_from_unit_ranges(RDIM_Arena *arena, RDIM_UnitVMapRange *first, RDI_U64 count);

//- rjf: type info baking
RDI_PROC RDI_U32* rdim_idx_run_from_types(RDIM_Arena *arena, RDIM_Type **types, RDI_U32 count);
RDI_PROC RDIM_TypeData* rdim_type_data_combine(RDIM_Arena *arena, RDIM_Root *root, RDIM_BakeCtx *bctx);

//- rjf: symbol data baking
RDI_PROC RDIM_SymbolData* rdim_symbol_data_combine(RDIM_Arena *arena, RDIM_Root *root, RDIM_BakeCtx *bctx);

//- rjf: name map baking
RDI_PROC RDIM_NameMapBaked* rdim_name_map_bake(RDIM_Arena *arena, RDIM_Root *root, RDIM_BakeCtx *bctx, RDIM_NameMap *map);

//- rjf: top-level baking entry point
RDI_PROC void rdim_bake_file(RDIM_Arena *arena, RDIM_Root *root, RDIM_String8List *out);
#endif

#endif // RDI_MAKE_H
