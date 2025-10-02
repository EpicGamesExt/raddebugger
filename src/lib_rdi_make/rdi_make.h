// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////////////////////////////////////
// RAD Debug Info Make, (R)AD(D)BG(I) (M)ake Library
//
// Library for building loose data structures which contain
// RDI debug information, and baking that down into the
// proper flattened RDI format.
//
// Requires prior inclusion of the RAD Debug Info, (R)AD(D)BG(I)
// Format Library, in rdi_format.h.

#ifndef RDI_MAKE_H
#define RDI_MAKE_H

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
// #define rdim_arena_push    <name of your pushing function  - must be (Arena*, U64 size, U64 align, B32 zero) -> void*>
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
//~ rjf: Overrideable Profile Markup

// To override the default profiling markup, do the following:
//
// #define RDIM_ProfBegin(...) <some expression, like a function call, to begin profiling some zone>
// #define RDIM_ProfEnd() <some expression, like a function call, to end profiling some zone>

#if !defined(RDIM_ProfBegin)
# define RDIM_ProfBegin(...) ((void)0)
#endif
#if !defined(RDIM_ProfEnd)
# define RDIM_ProfEnd() ((void)0)
#endif

#define RDIM_ProfScope(...) for(int _i_ = ((RDIM_ProfBegin(__VA_ARGS__)), 0); !_i_; _i_ += 1, (RDIM_ProfEnd()))

////////////////////////////////
//~ rjf: Alignment Macros

#if _MSC_VER
# define RDIM_AlignOf(T) __alignof(T)
#elif __clang__
# define RDIM_AlignOf(T) __alignof(T)
#elif __GNUC__
# define RDIM_AlignOf(T) __alignof__(T)
#else
# error [RDIM Build Error] RDIM_AlignOf(T) is not defined for this compiler.
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
//~ rjf: RDI Subsets

#define RDIM_Subset_XList \
X(BinarySections,              binary_sections)\
X(Units,                       units)\
X(Procedures,                  procedures)\
X(GlobalVariables,             global_variables)\
X(ThreadVariables,             thread_variables)\
X(Constants,                   constants)\
X(Scopes,                      scopes)\
X(Locals,                      locals)\
X(Types,                       types)\
X(UDTs,                        udts)\
X(LineInfo,                    line_info)\
X(InlineLineInfo,              inline_line_info)\
X(GlobalVariableNameMap,       global_variable_name_map)\
X(ThreadVariableNameMap,       thread_variable_name_map)\
X(ProcedureNameMap,            procedure_name_map)\
X(ConstantNameMap,             constant_name_map)\
X(TypeNameMap,                 type_name_map)\
X(LinkNameProcedureNameMap,    link_name_procedure_name_map)\
X(NormalSourcePathNameMap,     normal_source_path_name_map)\

typedef enum RDIM_Subset
{
#define X(name, name_lower) RDIM_Subset_##name,
  RDIM_Subset_XList
#undef X
}
RDIM_Subset;

typedef U32 RDIM_SubsetFlags;
enum
{
#define X(name, name_lower) RDIM_SubsetFlag_##name = (1<<RDIM_Subset_##name),
  RDIM_Subset_XList
#undef X
  RDIM_SubsetFlag_All = 0xffffffffu,
};

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
  RDI_U64 min;
};

typedef struct RDIM_Rng1U64ChunkNode RDIM_Rng1U64ChunkNode;
struct RDIM_Rng1U64ChunkNode
{
  RDIM_Rng1U64ChunkNode *next;
  RDIM_Rng1U64 *v;
  RDI_U64 count;
  RDI_U64 cap;
};

typedef struct RDIM_Rng1U64ChunkList RDIM_Rng1U64ChunkList;
struct RDIM_Rng1U64ChunkList
{
  RDIM_Rng1U64ChunkNode *first;
  RDIM_Rng1U64ChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
  RDI_U64 min;
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
  RDIM_String8 producer_name;
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
//~ rjf: Source File Info Types

typedef struct RDIM_SrcFileLineMapFragment RDIM_SrcFileLineMapFragment;
struct RDIM_SrcFileLineMapFragment
{
  RDIM_SrcFileLineMapFragment *next;
  struct RDIM_LineSequence *seq;
};

typedef struct RDIM_SrcFile RDIM_SrcFile;
struct RDIM_SrcFile
{
  struct RDIM_SrcFileChunkNode *chunk;
  RDIM_String8 path;
  RDIM_SrcFileLineMapFragment *first_line_map_fragment;
  RDIM_SrcFileLineMapFragment *last_line_map_fragment;
  RDI_U64 total_line_count;
  RDI_ChecksumKind checksum_kind;
  RDIM_String8 checksum;
};

typedef struct RDIM_SrcFileChunkNode RDIM_SrcFileChunkNode;
struct RDIM_SrcFileChunkNode
{
  RDIM_SrcFileChunkNode *next;
  RDIM_SrcFile *v;
  RDI_U64 count;
  RDI_U64 cap;
  RDI_U64 base_idx;
};

typedef struct  RDIM_SrcFileChunkList RDIM_SrcFileChunkList;
struct RDIM_SrcFileChunkList
{
  RDIM_SrcFileChunkNode *first;
  RDIM_SrcFileChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
  RDI_U64 source_line_map_count;
  RDI_U64 total_line_count;
};

////////////////////////////////
//~ rjf: Line Info Types

typedef struct RDIM_LineSequence RDIM_LineSequence;
struct RDIM_LineSequence
{
  RDIM_SrcFile *src_file;
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

typedef struct RDIM_LineTable RDIM_LineTable;
struct RDIM_LineTable
{
  struct RDIM_LineTableChunkNode *chunk;
  RDIM_LineSequenceNode *first_seq;
  RDIM_LineSequenceNode *last_seq;
  RDI_U64 seq_count;
  RDI_U64 line_count;
  RDI_U64 col_count;
};

typedef struct RDIM_LineTableChunkNode RDIM_LineTableChunkNode;
struct RDIM_LineTableChunkNode
{
  RDIM_LineTableChunkNode *next;
  RDIM_LineTable *v;
  RDI_U64 count;
  RDI_U64 cap;
  RDI_U64 base_idx;
};

typedef struct RDIM_LineTableChunkList RDIM_LineTableChunkList;
struct RDIM_LineTableChunkList
{
  RDIM_LineTableChunkNode *first;
  RDIM_LineTableChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
  RDI_U64 total_seq_count;
  RDI_U64 total_line_count;
  RDI_U64 total_col_count;
};

////////////////////////////////
//~ rjf: Per-Compilation-Unit Info Types

typedef struct RDIM_Unit RDIM_Unit;
struct RDIM_Unit
{
  struct RDIM_UnitChunkNode *chunk;
  RDIM_String8 unit_name;
  RDIM_String8 compiler_name;
  RDIM_String8 source_file;
  RDIM_String8 object_file;
  RDIM_String8 archive_file;
  RDIM_String8 build_path;
  RDI_Language language;
  RDIM_LineTable *line_table;
  RDIM_Rng1U64ChunkList voff_ranges;
};

typedef struct RDIM_UnitChunkNode RDIM_UnitChunkNode;
struct RDIM_UnitChunkNode
{
  RDIM_UnitChunkNode *next;
  RDIM_Unit *v;
  RDI_U64 count;
  RDI_U64 cap;
  RDI_U64 base_idx;
};

typedef struct RDIM_UnitChunkList RDIM_UnitChunkList;
struct RDIM_UnitChunkList
{
  RDIM_UnitChunkNode *first;
  RDIM_UnitChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
};

////////////////////////////////
//~ rjf: Type System Node Types

typedef RDI_U32 RDIM_DataModel;
typedef enum RDIM_DataModelEnum
{
  RDIM_DataModel_Null,
  RDIM_DataModel_ILP32,
  RDIM_DataModel_LLP64,
  RDIM_DataModel_LP64,
  RDIM_DataModel_ILP64,
  RDIM_DataModel_SILP64
}
RDIM_DataModelEnum;

typedef struct RDIM_Type RDIM_Type;
struct RDIM_Type
{
  struct RDIM_TypeChunkNode *chunk;
  RDI_TypeKind kind;
  RDI_U32 byte_size;
  RDI_U32 flags;
  RDI_U32 off;
  RDI_U32 count;
  RDIM_String8 name;
  RDIM_String8 link_name;
  RDIM_Type *direct_type;
  RDIM_Type **param_types;
  struct RDIM_UDT *udt;
};

typedef struct RDIM_TypeNode RDIM_TypeNode;
struct RDIM_TypeNode
{
  struct RDIM_TypeNode *next;
  RDIM_Type *v;
};

typedef struct RDIM_TypeList RDIM_TypeList;
struct RDIM_TypeList
{
  U64            count;
  RDIM_TypeNode *first;
  RDIM_TypeNode *last;
};

typedef struct RDIM_TypeChunkNode RDIM_TypeChunkNode;
struct RDIM_TypeChunkNode
{
  RDIM_TypeChunkNode *next;
  RDIM_Type *v;
  RDI_U64 count;
  RDI_U64 cap;
  RDI_U64 base_idx;
};

typedef struct RDIM_TypeChunkList RDIM_TypeChunkList;
struct RDIM_TypeChunkList
{
  RDIM_TypeChunkNode *first;
  RDIM_TypeChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
};

////////////////////////////////
//~ rjf: User-Defined-Type Info Types

//- rjf: UDT members

typedef struct RDIM_UDTMember RDIM_UDTMember;
struct RDIM_UDTMember
{
  RDIM_UDTMember *next;
  RDI_MemberKind kind;
  RDIM_String8 name;
  RDIM_Type *type;
  RDI_U32 off;
};

//- rjf: UDT enum values

typedef struct RDIM_UDTEnumVal RDIM_UDTEnumVal;
struct RDIM_UDTEnumVal
{
  RDIM_UDTEnumVal *next;
  RDIM_String8 name;
  RDI_U64 val;
};

//- rjf: UDTs

typedef struct RDIM_UDT RDIM_UDT;
struct RDIM_UDT
{
  struct RDIM_UDTChunkNode *chunk;
  RDIM_Type *self_type;
  RDIM_UDTMember *first_member;
  RDIM_UDTMember *last_member;
  RDIM_UDTEnumVal *first_enum_val;
  RDIM_UDTEnumVal *last_enum_val;
  RDI_U32 member_count;
  RDI_U32 enum_val_count;
  RDIM_SrcFile *src_file;
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
  RDI_U64 base_idx;
};

typedef struct RDIM_UDTChunkList RDIM_UDTChunkList;
struct RDIM_UDTChunkList
{
  RDIM_UDTChunkNode *first;
  RDIM_UDTChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
  RDI_U64 total_member_count;
  RDI_U64 total_enum_val_count;
};

////////////////////////////////
//~ rjf: Location Info Types

//- rjf: bytecode types

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

//- rjf: location types

typedef struct RDIM_LocationInfo RDIM_LocationInfo;
struct RDIM_LocationInfo
{
  RDI_LocationKind kind;
  RDI_U8 reg_code;
  RDI_U16 offset;
  RDIM_EvalBytecode bytecode;
};

typedef struct RDIM_Location RDIM_Location;
struct RDIM_Location
{
  struct RDIM_LocationChunkNode *chunk;
  RDIM_LocationInfo info;
  RDI_U64 relative_encoding_off;
};

typedef struct RDIM_LocationChunkNode RDIM_LocationChunkNode;
struct RDIM_LocationChunkNode
{
  RDIM_LocationChunkNode *next;
  RDIM_Location *v;
  RDI_U64 count;
  RDI_U64 cap;
  RDI_U64 base_idx;
  RDI_U64 base_encoding_off;
  RDI_U64 encoded_size;
};

typedef struct RDIM_LocationChunkList RDIM_LocationChunkList;
struct RDIM_LocationChunkList
{
  RDIM_LocationChunkNode *first;
  RDIM_LocationChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
  RDI_U64 total_encoded_size;
};

//- rjf: location cases

typedef struct RDIM_LocationCase RDIM_LocationCase;
struct RDIM_LocationCase
{
  RDIM_LocationCase *next;
  RDIM_Location *location;
  RDIM_Rng1U64 voff_range;
};

typedef struct RDIM_LocationCaseList RDIM_LocationCaseList;
struct RDIM_LocationCaseList
{
  RDIM_LocationCase *first;
  RDIM_LocationCase *last;
  RDI_U64 count;
};

////////////////////////////////
//~ rjf: Symbol Info Types

typedef struct RDIM_Symbol RDIM_Symbol;
struct RDIM_Symbol
{
  struct RDIM_SymbolChunkNode *chunk;
  RDI_S32 is_extern;
  RDIM_String8 name;
  RDIM_String8 link_name;
  RDIM_Type *type;
  RDI_U64 offset;
  RDIM_Symbol *container_symbol;
  RDIM_Type *container_type;
  struct RDIM_Scope *root_scope;
  RDIM_LocationCaseList location_cases;
  RDIM_String8 value_data;
};

typedef struct RDIM_SymbolChunkNode RDIM_SymbolChunkNode;
struct RDIM_SymbolChunkNode
{
  RDIM_SymbolChunkNode *next;
  RDIM_Symbol *v;
  RDI_U64 count;
  RDI_U64 cap;
  RDI_U64 base_idx;
};

typedef struct RDIM_SymbolChunkList RDIM_SymbolChunkList;
struct RDIM_SymbolChunkList
{
  RDIM_SymbolChunkNode *first;
  RDIM_SymbolChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
  RDI_U64 total_value_data_size;
};

////////////////////////////////
//~ rjf: Inline Site Info Types

typedef struct RDIM_InlineSite RDIM_InlineSite;
struct RDIM_InlineSite
{
  struct RDIM_InlineSiteChunkNode *chunk;
  RDIM_String8 name;
  RDIM_Type *type;
  RDIM_Type *owner;
  RDIM_LineTable *line_table;
};

typedef struct RDIM_InlineSiteChunkNode RDIM_InlineSiteChunkNode;
struct RDIM_InlineSiteChunkNode
{
  RDIM_InlineSiteChunkNode *next;
  RDIM_InlineSite *v;
  RDI_U64 count;
  RDI_U64 cap;
  RDI_U64 base_idx;
};

typedef struct RDIM_InlineSiteChunkList RDIM_InlineSiteChunkList;
struct RDIM_InlineSiteChunkList
{
  RDIM_InlineSiteChunkNode *first;
  RDIM_InlineSiteChunkNode *last;
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
  RDIM_LocationCaseList location_cases;
};

typedef struct RDIM_Scope RDIM_Scope;
struct RDIM_Scope
{
  struct RDIM_ScopeChunkNode *chunk;
  RDIM_Symbol *symbol;
  RDIM_Scope *parent_scope;
  RDIM_Scope *first_child;
  RDIM_Scope *last_child;
  RDIM_Scope *next_sibling;
  RDIM_Rng1U64List voff_ranges;
  RDIM_Local *first_local;
  RDIM_Local *last_local;
  RDI_U32 local_count;
  RDIM_InlineSite *inline_site;
};

typedef struct RDIM_ScopeChunkNode RDIM_ScopeChunkNode;
struct RDIM_ScopeChunkNode
{
  RDIM_ScopeChunkNode *next;
  RDIM_Scope *v;
  RDI_U64 count;
  RDI_U64 cap;
  RDI_U64 base_idx;
};

typedef struct RDIM_ScopeChunkList RDIM_ScopeChunkList;
struct RDIM_ScopeChunkList
{
  RDIM_ScopeChunkNode *first;
  RDIM_ScopeChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
  RDI_U64 scope_voff_count;
  RDI_U64 local_count;
  RDI_U64 location_case_count;
};

////////////////////////////////
//~ rjf: Baking Types

//- rjf: baking parameters

typedef struct RDIM_BakeParams RDIM_BakeParams;
struct RDIM_BakeParams
{
  RDIM_TopLevelInfo top_level_info;
  RDIM_BinarySectionList binary_sections;
  RDIM_UnitChunkList units;
  RDIM_TypeChunkList types;
  RDIM_UDTChunkList udts;
  RDIM_SrcFileChunkList src_files;
  RDIM_LineTableChunkList line_tables;
  RDIM_LocationChunkList locations;
  RDIM_SymbolChunkList global_variables;
  RDIM_SymbolChunkList thread_variables;
  RDIM_SymbolChunkList constants;
  RDIM_SymbolChunkList procedures;
  RDIM_ScopeChunkList scopes;
  RDIM_InlineSiteChunkList inline_sites;
};

//- rjf: data sections

typedef struct RDIM_BakeSection RDIM_BakeSection;
struct RDIM_BakeSection
{
  void *data;
  RDI_SectionEncoding encoding;
  RDI_U64 encoded_size;
  RDI_U64 unpacked_size;
  RDI_SectionKind tag;
  RDI_U64 tag_idx;
};

typedef struct RDIM_BakeSectionNode RDIM_BakeSectionNode;
struct RDIM_BakeSectionNode
{
  RDIM_BakeSectionNode *next;
  RDIM_BakeSection v;
};

typedef struct RDIM_BakeSectionList RDIM_BakeSectionList;
struct RDIM_BakeSectionList
{
  RDIM_BakeSectionNode *first;
  RDIM_BakeSectionNode *last;
  RDI_U64 count;
};

//- rjf: interned string type

typedef struct RDIM_BakeString RDIM_BakeString;
struct RDIM_BakeString
{
  RDI_U64 hash;
  RDIM_String8 string;
};

typedef struct RDIM_BakeStringChunkNode RDIM_BakeStringChunkNode;
struct RDIM_BakeStringChunkNode
{
  RDIM_BakeStringChunkNode *next;
  RDIM_BakeString *v;
  RDI_U64 count;
  RDI_U64 cap;
  RDI_U64 base_idx;
};

typedef struct RDIM_BakeStringChunkList RDIM_BakeStringChunkList;
struct RDIM_BakeStringChunkList
{
  RDIM_BakeStringChunkNode *first;
  RDIM_BakeStringChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
};

typedef struct RDIM_BakeStringMapTopology RDIM_BakeStringMapTopology;
struct RDIM_BakeStringMapTopology
{
  RDI_U64 slots_count;
};

typedef struct RDIM_BakeStringMapBaseIndices RDIM_BakeStringMapBaseIndices;
struct RDIM_BakeStringMapBaseIndices
{
  RDI_U64 *slots_base_idxs;
};

typedef struct RDIM_BakeStringMapLoose RDIM_BakeStringMapLoose;
struct RDIM_BakeStringMapLoose
{
  RDIM_BakeStringChunkList **slots;
};

typedef struct RDIM_BakeStringMapTight RDIM_BakeStringMapTight;
struct RDIM_BakeStringMapTight
{
  RDIM_BakeStringChunkList *slots;
  RDI_U64 *slots_base_idxs;
  RDI_U64 slots_count;
  RDI_U64 total_count;
};

//- rjf: index runs

typedef struct RDIM_BakeIdxRun RDIM_BakeIdxRun;
struct RDIM_BakeIdxRun
{
  RDI_U64 hash;
  RDI_U64 count;
  RDI_U32 *idxes;
};

typedef struct RDIM_BakeIdxRunChunkNode RDIM_BakeIdxRunChunkNode;
struct RDIM_BakeIdxRunChunkNode
{
  RDIM_BakeIdxRunChunkNode *next;
  RDIM_BakeIdxRun *v;
  RDI_U64 count;
  RDI_U64 cap;
};

typedef struct RDIM_BakeIdxRunChunkList RDIM_BakeIdxRunChunkList;
struct RDIM_BakeIdxRunChunkList
{
  RDIM_BakeIdxRunChunkNode *first;
  RDIM_BakeIdxRunChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
};

typedef struct RDIM_BakeIdxRunMapTopology RDIM_BakeIdxRunMapTopology;
struct RDIM_BakeIdxRunMapTopology
{
  RDI_U64 slots_count;
};

typedef struct RDIM_BakeIdxRunMapLoose RDIM_BakeIdxRunMapLoose;
struct RDIM_BakeIdxRunMapLoose
{
  RDIM_BakeIdxRunChunkList **slots;
  RDI_U64 *slots_idx_counts;
};

typedef struct RDIM_BakeIdxRunMap RDIM_BakeIdxRunMap;
struct RDIM_BakeIdxRunMap
{
  RDIM_BakeIdxRunChunkList *slots;
  RDI_U64 *slots_base_idxs; // NOTE(rjf): [slots_count+1], [slots_count] holds total count
  RDI_U64 slots_count;
};

//- rjf: source info & path tree

typedef struct RDIM_BakePathNode RDIM_BakePathNode;
struct RDIM_BakePathNode
{
  RDIM_BakePathNode *next_order;
  RDIM_BakePathNode *parent;
  RDIM_BakePathNode *first_child;
  RDIM_BakePathNode *last_child;
  RDIM_BakePathNode *next_sibling;
  RDIM_String8 name;
  RDIM_SrcFile *src_file;
  RDI_U32 idx;
};

typedef struct RDIM_BakeLineMapFragment RDIM_BakeLineMapFragment;
struct RDIM_BakeLineMapFragment
{
  RDIM_BakeLineMapFragment *next;
  RDIM_LineSequence *seq;
};

typedef struct RDIM_BakePathTree RDIM_BakePathTree;
struct RDIM_BakePathTree
{
  RDIM_BakePathNode root;
  RDIM_BakePathNode *first;
  RDIM_BakePathNode *last;
  RDI_U32 count;
};

//- rjf: name maps

typedef struct RDIM_BakeName RDIM_BakeName;
struct RDIM_BakeName
{
  RDIM_String8 string;
  RDI_U64 hash;
  RDI_U64 idx;
};

typedef struct RDIM_BakeNameChunkNode RDIM_BakeNameChunkNode;
struct RDIM_BakeNameChunkNode
{
  RDIM_BakeNameChunkNode *next;
  RDIM_BakeName *v;
  RDI_U64 count;
  RDI_U64 cap;
  RDI_U64 base_idx;
};

typedef struct RDIM_BakeNameChunkList RDIM_BakeNameChunkList;
struct RDIM_BakeNameChunkList
{
  RDIM_BakeNameChunkNode *first;
  RDIM_BakeNameChunkNode *last;
  RDI_U64 chunk_count;
  RDI_U64 total_count;
};

typedef struct RDIM_BakeNameMapTopology RDIM_BakeNameMapTopology;
struct RDIM_BakeNameMapTopology
{
  RDI_U64 slots_count;
};

typedef struct RDIM_BakeNameMap RDIM_BakeNameMap;
struct RDIM_BakeNameMap
{
  RDIM_BakeNameChunkList **slots;
};

//- rjf: vmaps

typedef struct RDIM_BakeVMap RDIM_BakeVMap;
struct RDIM_BakeVMap
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

//- rjf: source line maps

typedef struct RDIM_BakeSrcLineMapNode RDIM_BakeSrcLineMapNode;
struct RDIM_BakeSrcLineMapNode
{
  RDIM_BakeSrcLineMapNode *next;
  RDI_U32 line_num;
  RDIM_Rng1U64List voff_ranges;
};

typedef struct RDIM_BakeSrcLineMapSlot RDIM_BakeSrcLineMapSlot;
struct RDIM_BakeSrcLineMapSlot
{
  RDIM_BakeSrcLineMapNode *first;
  RDIM_BakeSrcLineMapNode *last;
};

typedef struct RDIM_BakeSrcLineMap RDIM_BakeSrcLineMap;
struct RDIM_BakeSrcLineMap
{
  RDIM_BakeSrcLineMapSlot *slots;
  RDI_U64 slots_count;
  RDI_U64 line_count;
  RDI_U64 voff_range_count;
};

//- rjf: line table records

typedef struct RDIM_LineRec RDIM_LineRec;
struct RDIM_LineRec
{
  RDI_U32 file_id;
  RDI_U32 line_num;
  RDI_U16 col_first;
  RDI_U16 col_opl;
};

//- rjf: baking results

typedef struct RDIM_TopLevelInfoBakeResult RDIM_TopLevelInfoBakeResult;
struct RDIM_TopLevelInfoBakeResult
{
  RDI_TopLevelInfo top_level_info;
};

typedef struct RDIM_BinarySectionBakeResult RDIM_BinarySectionBakeResult;
struct RDIM_BinarySectionBakeResult
{
  RDI_BinarySection *binary_sections;
  RDI_U64 binary_sections_count;
};

typedef struct RDIM_UnitBakeResult RDIM_UnitBakeResult;
struct RDIM_UnitBakeResult
{
  RDI_Unit *units;
  RDI_U64 units_count;
};

typedef struct RDIM_UnitVMapBakeResult RDIM_UnitVMapBakeResult;
struct RDIM_UnitVMapBakeResult
{
  RDIM_BakeVMap vmap;
};

typedef struct RDIM_SrcFileBakeResult RDIM_SrcFileBakeResult;
struct RDIM_SrcFileBakeResult
{
  RDI_SourceFile *source_files;
  RDI_U64 source_files_count;
  RDI_SourceLineMap *source_line_maps;
  RDI_U64 source_line_maps_count;
  RDI_U32 *source_line_map_nums;
  RDI_U32 *source_line_map_rngs;
  RDI_U64 *source_line_map_voffs;
  RDI_U64 source_line_map_nums_count;
  RDI_U64 source_line_map_rngs_count;
  RDI_U64 source_line_map_voffs_count;
};

typedef struct RDIM_ChecksumBakeResult RDIM_ChecksumBakeResult;
struct RDIM_ChecksumBakeResult
{
  RDI_MD5 *md5s;
  RDI_U64 md5s_count;
  RDI_SHA1 *sha1s;
  RDI_U64 sha1s_count;
  RDI_SHA256 *sha256s;
  RDI_U64 sha256s_count;
  RDI_U64 *timestamps;
  RDI_U64 timestamps_count;
};

typedef struct RDIM_LineTableBakeResult RDIM_LineTableBakeResult;
struct RDIM_LineTableBakeResult
{
  RDI_LineTable *line_tables;
  RDI_U64 line_tables_count;
  RDI_U64 *line_table_voffs;
  RDI_U64 line_table_voffs_count;
  RDI_Line *line_table_lines;
  RDI_U64 line_table_lines_count;
  RDI_Column *line_table_columns;
  RDI_U64 line_table_columns_count;
};

typedef struct RDIM_TypeNodeBakeResult RDIM_TypeNodeBakeResult;
struct RDIM_TypeNodeBakeResult
{
  RDI_TypeNode *type_nodes;
  RDI_U64 type_nodes_count;
};

typedef struct RDIM_UDTBakeResult RDIM_UDTBakeResult;
struct RDIM_UDTBakeResult
{
  RDI_UDT *udts;
  RDI_U64 udts_count;
  RDI_Member *members;
  RDI_U64 members_count;
  RDI_EnumMember *enum_members;
  RDI_U64 enum_members_count;
};

typedef struct RDIM_LocationBakeResult RDIM_LocationBakeResult;
struct RDIM_LocationBakeResult
{
  RDI_U8 *location_data;
  RDI_U64 location_data_size;
};

typedef struct RDIM_LocationBlockBakeResult RDIM_LocationBlockBakeResult;
struct RDIM_LocationBlockBakeResult
{
  RDI_LocationBlock *location_blocks;
  RDI_U64 location_blocks_count;
};

typedef struct RDIM_GlobalVariableBakeResult RDIM_GlobalVariableBakeResult;
struct RDIM_GlobalVariableBakeResult
{
  RDI_GlobalVariable *global_variables;
  RDI_U64 global_variables_count;
};

typedef struct RDIM_GlobalVMapBakeResult RDIM_GlobalVMapBakeResult;
struct RDIM_GlobalVMapBakeResult
{
  RDIM_BakeVMap vmap;
};

typedef struct RDIM_ThreadVariableBakeResult RDIM_ThreadVariableBakeResult;
struct RDIM_ThreadVariableBakeResult
{
  RDI_ThreadVariable *thread_variables;
  RDI_U64 thread_variables_count;
};

typedef struct RDIM_ConstantsBakeResult RDIM_ConstantsBakeResult;
struct RDIM_ConstantsBakeResult
{
  RDI_Constant *constants;
  RDI_U64 constants_count;
  RDI_U32 *constant_values;
  RDI_U64 constant_values_count;
  RDI_U8 *constant_value_data;
  RDI_U64 constant_value_data_size;
};

typedef struct RDIM_ProcedureBakeResult RDIM_ProcedureBakeResult;
struct RDIM_ProcedureBakeResult
{
  RDI_Procedure *procedures;
  RDI_U64 procedures_count;
};

typedef struct RDIM_ScopeBakeResult RDIM_ScopeBakeResult;
struct RDIM_ScopeBakeResult
{
  RDI_Scope *scopes;
  RDI_U64 scopes_count;
  RDI_U64 *scope_voffs;
  RDI_U64 scope_voffs_count;
  RDI_Local *locals;
  RDI_U64 locals_count;
};

typedef struct RDIM_ScopeVMapBakeResult RDIM_ScopeVMapBakeResult;
struct RDIM_ScopeVMapBakeResult
{
  RDIM_BakeVMap vmap;
};

typedef struct RDIM_InlineSiteBakeResult RDIM_InlineSiteBakeResult;
struct RDIM_InlineSiteBakeResult
{
  RDI_InlineSite *inline_sites;
  RDI_U64 inline_sites_count;
};

typedef struct RDIM_TopLevelNameMapBakeResult RDIM_TopLevelNameMapBakeResult;
struct RDIM_TopLevelNameMapBakeResult
{
  RDI_NameMap *name_maps;
  RDI_U64 name_maps_count;
};

typedef struct RDIM_NameMapBakeResult RDIM_NameMapBakeResult;
struct RDIM_NameMapBakeResult
{
  RDI_NameMapBucket *buckets;
  RDI_U64 buckets_count;
  RDI_NameMapNode *nodes;
  RDI_U64 nodes_count;
};

typedef struct RDIM_FilePathBakeResult RDIM_FilePathBakeResult;
struct RDIM_FilePathBakeResult
{
  RDI_FilePathNode *nodes;
  RDI_U64 nodes_count;
};

typedef struct RDIM_StringBakeResult RDIM_StringBakeResult;
struct RDIM_StringBakeResult
{
  RDI_U32 *string_offs;
  RDI_U64 string_offs_count;
  RDI_U8 *string_data;
  RDI_U64 string_data_size;
};

typedef struct RDIM_IndexRunBakeResult RDIM_IndexRunBakeResult;
struct RDIM_IndexRunBakeResult
{
  RDI_U32 *idx_runs;
  RDI_U64 idx_count;
};

typedef struct RDIM_BakeResults RDIM_BakeResults;
struct RDIM_BakeResults
{
  RDIM_TopLevelInfoBakeResult top_level_info;
  RDIM_BinarySectionBakeResult binary_sections;
  RDIM_UnitBakeResult units;
  RDIM_UnitVMapBakeResult unit_vmap;
  RDIM_SrcFileBakeResult src_files;
  RDIM_ChecksumBakeResult checksums;
  RDIM_LineTableBakeResult line_tables;
  RDIM_TypeNodeBakeResult type_nodes;
  RDIM_UDTBakeResult udts;
  RDIM_GlobalVariableBakeResult global_variables;
  RDIM_GlobalVMapBakeResult global_vmap;
  RDIM_ThreadVariableBakeResult thread_variables;
  RDIM_ConstantsBakeResult constants;
  RDIM_ProcedureBakeResult procedures;
  RDIM_ScopeBakeResult scopes;
  RDIM_InlineSiteBakeResult inline_sites;
  RDIM_ScopeVMapBakeResult scope_vmap;
  RDIM_TopLevelNameMapBakeResult top_level_name_maps;
  RDIM_NameMapBakeResult name_maps;
  RDIM_FilePathBakeResult file_paths;
  RDIM_StringBakeResult strings;
  RDIM_IndexRunBakeResult idx_runs;
  RDIM_LocationBakeResult locations;
  RDIM_LocationBlockBakeResult location_blocks;
};

////////////////////////////////
//~ rjf: Serialization Types

typedef struct RDIM_SerializedSection RDIM_SerializedSection;
struct RDIM_SerializedSection
{
  void *data;
  RDI_U64 encoded_size;
  RDI_U64 unpacked_size;
  RDI_SectionEncoding encoding;
};

typedef struct RDIM_SerializedSectionBundle RDIM_SerializedSectionBundle;
struct RDIM_SerializedSectionBundle
{
  RDIM_SerializedSection sections[RDI_SectionKind_COUNT];
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
RDI_PROC void *rdim_arena_push_fallback(RDIM_Arena *arena, RDI_U64 align, RDI_U64 size);
RDI_PROC void rdim_arena_pop_to_fallback(RDIM_Arena *arena, RDI_U64 pos);
#endif
#define rdim_push_array_no_zero(a,T,c) (T*)rdim_arena_push((a), sizeof(T)*(c), RDIM_AlignOf(T), (0))
#define rdim_push_array(a,T,c) (T*)rdim_arena_push((a), sizeof(T)*(c), RDIM_AlignOf(T), (1))

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
#define rdim_str8_lit(S)              rdim_str8((RDI_U8*)(S), sizeof(S) - 1)
#define rdim_str8_struct(S)           rdim_str8((RDI_U8*)(S), sizeof(*(S)))
#define rdim_str8_struct_array(S, C)  rdim_str8((RDI_U8*)(S), sizeof(*(S)) * (C))
RDI_PROC RDIM_String8 rdim_lower_from_str8(RDIM_Arena *arena, RDIM_String8 string);

//- rjf: string lists
RDI_PROC void rdim_str8_list_push(RDIM_Arena *arena, RDIM_String8List *list, RDIM_String8 string);
RDI_PROC void rdim_str8_list_push_front(RDIM_Arena *arena, RDIM_String8List *list, RDIM_String8 string);
RDI_PROC void rdim_str8_list_push_align(RDIM_Arena *arena, RDIM_String8List *list, RDI_U64 align);
RDI_PROC RDIM_String8 rdim_str8_list_join(RDIM_Arena *arena, RDIM_String8List *list, RDIM_String8 sep);

//- rjf: sortable range sorting
RDI_PROC RDIM_SortKey *rdim_sort_key_array(RDIM_Arena *arena, RDIM_SortKey *keys, RDI_U64 count);

//- rjf: rng1u64 lists
RDI_PROC void rdim_rng1u64_list_push(RDIM_Arena *arena, RDIM_Rng1U64List *list, RDIM_Rng1U64 r);
RDI_PROC void rdim_rng1u64_chunk_list_push(RDIM_Arena *arena, RDIM_Rng1U64ChunkList *list, RDI_U64 chunk_cap, RDIM_Rng1U64 r);

////////////////////////////////
//~ rjf: [Building] Data Model

RDI_PROC RDI_TypeKind rdim_short_type_kind_from_data_model(RDIM_DataModel data_model);
RDI_PROC RDI_TypeKind rdim_unsigned_short_type_kind_from_data_model(RDIM_DataModel data_model);
RDI_PROC RDI_TypeKind rdim_int_type_from_data_model(RDIM_DataModel data_model);
RDI_PROC RDI_TypeKind rdim_unsigned_int_type_from_data_model(RDIM_DataModel data_model);
RDI_PROC RDI_TypeKind rdim_long_type_kind_from_data_model(RDIM_DataModel data_model);
RDI_PROC RDI_TypeKind rdim_unsigned_long_type_kind_from_data_model(RDIM_DataModel data_model);
RDI_PROC RDI_TypeKind rdim_long_long_type_kind_from_data_model(RDIM_DataModel data_model);
RDI_PROC RDI_TypeKind rdim_unsigned_long_long_type_kind_from_data_model(RDIM_DataModel data_model);
RDI_PROC RDI_TypeKind rdim_pointer_size_t_type_kind_from_data_model(RDIM_DataModel data_model);

////////////////////////////////
//~ rjf: [Building] Binary Section Info Building

RDI_PROC RDIM_BinarySection *rdim_binary_section_list_push(RDIM_Arena *arena, RDIM_BinarySectionList *list);

////////////////////////////////
//~ rjf: [Building] Source File Info Building

RDI_PROC RDIM_SrcFile *rdim_src_file_chunk_list_push(RDIM_Arena *arena, RDIM_SrcFileChunkList *list, RDI_U64 cap);
RDI_PROC RDI_U64 rdim_idx_from_src_file(RDIM_SrcFile *src_file);
RDI_PROC void rdim_src_file_chunk_list_concat_in_place(RDIM_SrcFileChunkList *dst, RDIM_SrcFileChunkList *to_push);
RDI_PROC void rdim_src_file_push_line_sequence(RDIM_Arena *arena, RDIM_SrcFileChunkList *src_files, RDIM_SrcFile *src_file, RDIM_LineSequence *seq);

////////////////////////////////
//~ rjf: [Building] Line Info Building

RDI_PROC RDIM_LineTable *rdim_line_table_chunk_list_push(RDIM_Arena *arena, RDIM_LineTableChunkList *list, RDI_U64 cap);
RDI_PROC RDI_U64 rdim_idx_from_line_table(RDIM_LineTable *line_table);
RDI_PROC void rdim_line_table_chunk_list_concat_in_place(RDIM_LineTableChunkList *dst, RDIM_LineTableChunkList *to_push);
RDI_PROC RDIM_LineSequence *rdim_line_table_push_sequence(RDIM_Arena *arena, RDIM_LineTableChunkList *line_tables, RDIM_LineTable *line_table, RDIM_SrcFile *src_file, RDI_U64 *voffs, RDI_U32 *line_nums, RDI_U16 *col_nums, RDI_U64 line_count);

////////////////////////////////
//~ rjf: [Building] Unit Info Building

RDI_PROC RDIM_Unit *rdim_unit_chunk_list_push(RDIM_Arena *arena, RDIM_UnitChunkList *list, RDI_U64 cap);
RDI_PROC RDI_U64 rdim_idx_from_unit(RDIM_Unit *unit);
RDI_PROC void rdim_unit_chunk_list_concat_in_place(RDIM_UnitChunkList *dst, RDIM_UnitChunkList *to_push);

////////////////////////////////
//~ rjf: [Building] Type Info & UDT Building

//- rjf: type nodes
RDI_PROC RDIM_Type *rdim_type_chunk_list_push(RDIM_Arena *arena, RDIM_TypeChunkList *list, RDI_U64 cap);
RDI_PROC RDI_U64 rdim_idx_from_type(RDIM_Type *type);
RDI_PROC void rdim_type_chunk_list_concat_in_place(RDIM_TypeChunkList *dst, RDIM_TypeChunkList *to_push);

//- rjf: UDTs
RDI_PROC RDIM_UDT *rdim_udt_chunk_list_push(RDIM_Arena *arena, RDIM_UDTChunkList *list, RDI_U64 cap);
RDI_PROC RDI_U64 rdim_idx_from_udt(RDIM_UDT *udt);
RDI_PROC void rdim_udt_chunk_list_concat_in_place(RDIM_UDTChunkList *dst, RDIM_UDTChunkList *to_push);
RDI_PROC RDIM_UDTMember *rdim_udt_push_member(RDIM_Arena *arena, RDIM_UDTChunkList *list, RDIM_UDT *udt);
RDI_PROC RDIM_UDTEnumVal *rdim_udt_push_enum_val(RDIM_Arena *arena, RDIM_UDTChunkList *list, RDIM_UDT *udt);

////////////////////////////////
//~ rjf: [Building] Symbol Info Building

RDI_PROC RDIM_Symbol *rdim_symbol_chunk_list_push(RDIM_Arena *arena, RDIM_SymbolChunkList *list, RDI_U64 cap);
RDI_PROC RDI_U64 rdim_idx_from_symbol(RDIM_Symbol *symbol);
RDI_PROC void rdim_symbol_chunk_list_concat_in_place(RDIM_SymbolChunkList *dst, RDIM_SymbolChunkList *to_push);
internal void rdim_symbol_push_value_data(RDIM_Arena *arena, RDIM_SymbolChunkList *list, RDIM_Symbol *symbol, RDIM_String8 data);

////////////////////////////////
//~ rjf: [Building] Inline Site Info Building

RDI_PROC RDIM_InlineSite *rdim_inline_site_chunk_list_push(RDIM_Arena *arena, RDIM_InlineSiteChunkList *list, RDI_U64 cap);
RDI_PROC RDI_U64 rdim_idx_from_inline_site(RDIM_InlineSite *inline_site);
RDI_PROC void rdim_inline_site_chunk_list_concat_in_place(RDIM_InlineSiteChunkList *dst, RDIM_InlineSiteChunkList *to_push);

////////////////////////////////
//~ rjf: [Building] Location Info Building

//- rjf: bytecode
RDI_PROC void rdim_bytecode_push_op(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_EvalOp op, RDI_U64 p);
RDI_PROC void rdim_bytecode_push_uconst(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_U64 x);
RDI_PROC void rdim_bytecode_push_sconst(RDIM_Arena *arena, RDIM_EvalBytecode *bytecode, RDI_S64 x);
RDI_PROC void rdim_bytecode_concat_in_place(RDIM_EvalBytecode *left_dst, RDIM_EvalBytecode *right_destroyed);

//- rjf: locations
RDI_PROC RDI_U64 rdim_encoded_size_from_location_info(RDIM_LocationInfo *info);
RDI_PROC RDIM_Location *rdim_location_chunk_list_push_new(RDIM_Arena *arena, RDIM_LocationChunkList *list, RDI_U64 cap, RDIM_LocationInfo *info);
RDI_PROC RDI_U64 rdim_off_from_location(RDIM_Location *location);
RDI_PROC void rdim_location_chunk_list_concat_in_place(RDIM_LocationChunkList *dst, RDIM_LocationChunkList *to_push);

////////////////////////////////
//~ rjf: [Building] Scope Info Building

RDI_PROC RDIM_Scope *rdim_scope_chunk_list_push(RDIM_Arena *arena, RDIM_ScopeChunkList *list, RDI_U64 cap);
RDI_PROC RDI_U64 rdim_idx_from_scope(RDIM_Scope *scope);
RDI_PROC void rdim_scope_chunk_list_concat_in_place(RDIM_ScopeChunkList *dst, RDIM_ScopeChunkList *to_push);
RDI_PROC void rdim_scope_push_voff_range(RDIM_Arena *arena, RDIM_ScopeChunkList *list, RDIM_Scope *scope, RDIM_Rng1U64 range);
RDI_PROC RDIM_Local *rdim_scope_push_local(RDIM_Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Scope *scope);
RDI_PROC RDIM_LocationCase *rdim_local_push_location_case(RDIM_Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Local *local, RDIM_Location *location, RDIM_Rng1U64 voff_range);

////////////////////////////////
//~ rjf: [Baking Helpers] Deduplicated String Baking Map

//- rjf: chunk lists
RDI_PROC RDIM_BakeString *rdim_bake_string_chunk_list_push(RDIM_Arena *arena, RDIM_BakeStringChunkList *list, RDI_U64 cap);
RDI_PROC void rdim_bake_string_chunk_list_concat_in_place(RDIM_BakeStringChunkList *dst, RDIM_BakeStringChunkList *to_push);
RDI_PROC RDIM_BakeStringChunkList rdim_bake_string_chunk_list_sorted_from_unsorted(RDIM_Arena *arena, RDIM_BakeStringChunkList *src);

//- rjf: loose map
RDI_PROC RDIM_BakeStringMapLoose *rdim_bake_string_map_loose_make(RDIM_Arena *arena, RDIM_BakeStringMapTopology *top);
RDI_PROC void rdim_bake_string_map_loose_insert(RDIM_Arena *arena, RDIM_BakeStringMapTopology *map_topology, RDIM_BakeStringMapLoose *map, RDI_U64 chunk_cap, RDIM_String8 string);
RDI_PROC RDIM_BakeStringMapBaseIndices rdim_bake_string_map_base_indices_from_map_loose(RDIM_Arena *arena, RDIM_BakeStringMapTopology *map_topology, RDIM_BakeStringMapLoose *map);

//- rjf: finalized / tight map
RDI_PROC RDI_U32 rdim_bake_idx_from_string(RDIM_BakeStringMapTight *map, RDIM_String8 string);

////////////////////////////////
//~ rjf: [Baking Helpers] Deduplicated Index Run Baking Map

//- rjf: bake idx run map reading/writing
RDI_PROC RDI_U64 rdim_hash_from_idx_run(RDI_U32 *idx_run, RDI_U32 count);

//- rjf: chunk lists
RDI_PROC RDIM_BakeIdxRun *rdim_bake_idx_run_chunk_list_push(RDIM_Arena *arena, RDIM_BakeIdxRunChunkList *list, RDI_U64 cap);
RDI_PROC void rdim_bake_idx_run_chunk_list_concat_in_place(RDIM_BakeIdxRunChunkList *dst, RDIM_BakeIdxRunChunkList *to_push);
RDI_PROC RDIM_BakeIdxRunChunkList rdim_bake_idx_run_chunk_list_sorted_from_unsorted(RDIM_Arena *arena, RDIM_BakeIdxRunChunkList *src);

//- rjf: loose map
RDI_PROC RDIM_BakeIdxRunMapLoose *rdim_bake_idx_run_map_loose_make(RDIM_Arena *arena, RDIM_BakeIdxRunMapTopology *top);
RDI_PROC void rdim_bake_idx_run_map_loose_insert(RDIM_Arena *arena, RDIM_BakeIdxRunMapTopology *map_topology, RDIM_BakeIdxRunMapLoose *map, RDI_U64 chunk_cap, RDI_U32 *idxes, RDI_U32 count);

//- rjf: finalized / tight map
RDI_PROC RDI_U32 rdim_bake_idx_from_idx_run(RDIM_BakeIdxRunMap *map, RDI_U32 *idxes, RDI_U32 count);

////////////////////////////////
//~ rjf: [Baking Helpers] Deduplicated Name Map Baking Map

//- rjf: chunk lists
RDI_PROC RDIM_BakeName *rdim_bake_name_chunk_list_push(RDIM_Arena *arena, RDIM_BakeNameChunkList *list, RDI_U64 cap);
RDI_PROC void rdim_bake_name_chunk_list_concat_in_place(RDIM_BakeNameChunkList *dst, RDIM_BakeNameChunkList *to_push);
RDI_PROC RDIM_BakeNameChunkList rdim_bake_name_chunk_list_sorted_from_unsorted(RDIM_Arena *arena, RDIM_BakeNameChunkList *src);

//- rjf: bake name chunk list maps
RDI_PROC RDIM_BakeNameMap *rdim_bake_name_map_make(RDIM_Arena *arena, RDIM_BakeNameMapTopology *top);
RDI_PROC void rdim_bake_name_map_insert(RDIM_Arena *arena, RDIM_BakeNameMapTopology *map_topology, RDIM_BakeNameMap *map, RDI_U64 chunk_cap, RDIM_String8 string, RDI_U64 idx);

////////////////////////////////
//~ rjf: [Baking Helpers] Deduplicated Path Baking Tree

RDI_PROC RDIM_BakePathNode *rdim_bake_path_node_from_string(RDIM_BakePathTree *tree, RDIM_String8 string);
RDI_PROC RDI_U32 rdim_bake_path_node_idx_from_string(RDIM_BakePathTree *tree, RDIM_String8 string);
RDI_PROC RDIM_BakePathNode *rdim_bake_path_tree_insert(RDIM_Arena *arena, RDIM_BakePathTree *tree, RDIM_String8 string);

////////////////////////////////
//~ rjf: [Baking Helpers] Data Section List Building Helpers

RDI_PROC RDIM_BakeSection *rdim_bake_section_list_push(RDIM_Arena *arena, RDIM_BakeSectionList *list);
RDI_PROC RDIM_BakeSection *rdim_bake_section_list_push_new_unpacked(RDIM_Arena *arena, RDIM_BakeSectionList *list, void *data, RDI_U64 size, RDI_SectionKind tag, RDI_U64 tag_idx);
RDI_PROC void rdim_bake_section_list_concat_in_place(RDIM_BakeSectionList *dst, RDIM_BakeSectionList *to_push);

////////////////////////////////
//~ rjf: [Serializing] Bake Results -> String Blobs

RDI_PROC RDIM_SerializedSection rdim_serialized_section_make_unpacked(void *data, RDI_U64 size);
#define rdim_serialized_section_make_unpacked_struct(ptr) rdim_serialized_section_make_unpacked((ptr), sizeof(*(ptr)))
#define rdim_serialized_section_make_unpacked_array(ptr, count) rdim_serialized_section_make_unpacked((ptr), sizeof(*(ptr))*(count))
RDI_PROC RDIM_SerializedSectionBundle rdim_serialized_section_bundle_from_bake_results(RDIM_BakeResults *results);
RDI_PROC RDIM_String8List rdim_file_blobs_from_section_bundle(RDIM_Arena *arena, RDIM_SerializedSectionBundle *bundle);

#endif // RDI_MAKE_H
