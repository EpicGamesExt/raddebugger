// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_MAKE_LOCAL_H
#define RDI_MAKE_LOCAL_H

//- rjf: base layer memory ops
#define RDIM_MEMSET_OVERRIDE
#define RDIM_MEMCPY_OVERRIDE
#define rdim_memset MemorySet
#define rdim_memcpy MemoryCopy

//- rjf: base layer string overrides
#define RDI_STRING8_OVERRIDE
#define RDIM_String8            String8
#define RDIM_String8_BaseMember str
#define RDIM_String8_SizeMember size
#define RDI_STRING8LIST_OVERRIDE
#define RDIM_String8Node                 String8Node
#define RDIM_String8Node_NextPtrMember   next
#define RDIM_String8Node_StringMember    string
#define RDIM_String8List                 String8List
#define RDIM_String8List_FirstMember     first
#define RDIM_String8List_LastMember      last
#define RDIM_String8List_NodeCountMember node_count
#define RDIM_String8List_TotalSizeMember total_size

//- rjf: base layer arena overrides
#define RDIM_ARENA_OVERRIDE
#define RDIM_Arena Arena
#define rdim_arena_alloc     arena_alloc
#define rdim_arena_release   arena_release
#define rdim_arena_pos       arena_pos
#define rdim_arena_push      arena_push
#define rdim_arena_pop_to    arena_pop_to

//- rjf: base layer scratch arena overrides
#define RDIM_SCRATCH_OVERRIDE
#define RDIM_Temp Temp
#define rdim_temp_arena(t)   ((t).arena)
#define rdim_scratch_begin   scratch_begin
#define rdim_scratch_end     scratch_end

//- rjf: base layer profiling markup overrides
#define RDIM_ProfBegin(...) ProfBeginDynamic(__VA_ARGS__)
#define RDIM_ProfEnd(...) ProfEnd()

//- rjf: main library
#include "lib_rdi_make/rdi_make.h"

//- rjf: unsorted joined line table info

typedef struct RDIM_UnsortedJoinedLineTable RDIM_UnsortedJoinedLineTable;
struct RDIM_UnsortedJoinedLineTable
{
  RDI_U64 line_count;
  RDI_U64 seq_count;
  RDI_U64 key_count;
  RDIM_SortKey *line_keys;
  RDIM_LineRec *line_recs;
};

internal RDIM_DataModel rdim_data_model_from_os_arch(OperatingSystem os, RDI_Arch arch);
internal RDIM_TopLevelInfo rdim_make_top_level_info(String8 image_name, Arch arch, U64 exe_hash, RDIM_BinarySectionList sections);
internal RDIM_BakeParams rdim_loose_from_rdi(Arena *arena, RDIM_SubsetFlags subset_flags, RDI_Parsed *rdi);
internal RDIM_BakeResults rdim_bake(Arena *arena, RDIM_BakeParams *params);
internal RDIM_SerializedSectionBundle rdim_compress(Arena *arena, RDIM_SerializedSectionBundle *in);

#endif // RDI_MAKE_LOCAL_H
