// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_CONS_LOCAL_H
#define RADDBGI_CONS_LOCAL_H

// rjf: base layer memory ops
#define RADDBGIC_MEMSET_OVERRIDE
#define raddbgic_memset memset

// rjf: base layer string overrides
#define RADDBGI_STRING8_OVERRIDE
#define RADDBGIC_String8 String8
#define RADDBGIC_String8_BaseMember str
#define RADDBGIC_String8_SizeMember size
#define RADDBGIC_String8Node String8Node
#define RADDBGIC_String8List String8List

// rjf: base layer arena overrides
#define RADDBGIC_ARENA_OVERRIDE
#define RADDBGIC_Arena Arena
#define raddbgic_arena_alloc     arena_alloc
#define raddbgic_arena_release   arena_release
#define raddbgic_arena_pos       arena_pos
#define raddbgic_arena_push      arena_push
#define raddbgic_arena_pop_to    arena_pop_to

#include "raddbgi_cons.h"

#endif // RADDBGI_CONS_LOCAL_H
