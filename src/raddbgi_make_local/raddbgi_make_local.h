// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_CONS_LOCAL_H
#define RADDBGI_CONS_LOCAL_H

// rjf: base layer memory ops
#define RDIM_MEMSET_OVERRIDE
#define RDIM_MEMCPY_OVERRIDE
#define rdim_memset MemorySet
#define rdim_memcpy MemoryCopy

// rjf: base layer string overrides
#define RADDBGI_STRING8_OVERRIDE
#define RDIM_String8 String8
#define RDIM_String8_BaseMember str
#define RDIM_String8_SizeMember size
#define RADDBGI_STRING8LIST_OVERRIDE
#define RDIM_String8Node String8Node
#define RDIM_String8Node_NextPtrMember next
#define RDIM_String8Node_StringMember string
#define RDIM_String8List String8List
#define RDIM_String8List_FirstMember first
#define RDIM_String8List_LastMember last
#define RDIM_String8List_NodeCountMember node_count
#define RDIM_String8List_TotalSizeMember total_size

// rjf: base layer arena overrides
#define RDIM_ARENA_OVERRIDE
#define RDIM_Arena Arena
#define rdim_arena_alloc     arena_alloc
#define rdim_arena_release   arena_release
#define rdim_arena_pos       arena_pos
#define rdim_arena_push      arena_push
#define rdim_arena_pop_to    arena_pop_to

// rjf: base layer scratch arena overrides
#define RDIM_SCRATCH_OVERRIDE
#define RDIM_Temp Temp
#define rdim_temp_arena(t)   ((t).arena)
#define rdim_scratch_begin   scratch_begin
#define rdim_scratch_end     scratch_end

#include "lib_raddbgi_make/raddbgi_make.h"

#endif // RADDBGI_CONS_LOCAL_H
