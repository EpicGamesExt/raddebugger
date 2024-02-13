// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_CONS_LOCAL_H
#define RADDBGI_CONS_LOCAL_H

// rjf: base layer memory ops
#define RADDBGIC_MEMSET_OVERRIDE
#define RADDBGIC_MEMCPY_OVERRIDE
#define raddbgic_memset MemorySet
#define raddbgic_memcpy MemoryCopy

// rjf: base layer string overrides
#define RADDBGI_STRING8_OVERRIDE
#define RADDBGIC_String8 String8
#define RADDBGIC_String8_BaseMember str
#define RADDBGIC_String8_SizeMember size
#define RADDBGI_STRING8LIST_OVERRIDE
#define RADDBGIC_String8Node String8Node
#define RADDBGIC_String8Node_NextPtrMember next
#define RADDBGIC_String8Node_StringMember string
#define RADDBGIC_String8List String8List
#define RADDBGIC_String8List_FirstMember first
#define RADDBGIC_String8List_LastMember last
#define RADDBGIC_String8List_NodeCountMember node_count
#define RADDBGIC_String8List_TotalSizeMember total_size

// rjf: base layer arena overrides
#define RADDBGIC_ARENA_OVERRIDE
#define RADDBGIC_Arena Arena
#define raddbgic_arena_alloc     arena_alloc
#define raddbgic_arena_release   arena_release
#define raddbgic_arena_pos       arena_pos
#define raddbgic_arena_push      arena_push
#define raddbgic_arena_pop_to    arena_pop_to

// rjf: base layer scratch arena overrides
#define RADDBGIC_SCRATCH_OVERRIDE
#define RADDBGIC_Temp Temp
#define raddbgic_temp_arena(t)   ((t).arena)
#define raddbgic_scratch_begin   scratch_begin
#define raddbgic_scratch_end     scratch_end

#include "lib_raddbgi_cons/raddbgi_cons.h"

#endif // RADDBGI_CONS_LOCAL_H
