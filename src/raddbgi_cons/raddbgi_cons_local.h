// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_CONS_LOCAL_H
#define RADDBGI_CONS_LOCAL_H

// rjf: base layer memory ops
#define raddbgic_memset memset

// rjf: base layer string overrides
#define RADDBGIC_String8 String8
#define RADDBGIC_String8_BaseMember str
#define RADDBGIC_String8_SizeMember size
#define RADDBGIC_String8Node String8Node
#define RADDBGIC_String8List String8List

// rjf: base layer arena overrides
#define RADDBGIC_Arena Arena
#define RADDBGIC_Arena_AllocImpl arena_alloc
#define RADDBGIC_Arena_ReleaseImpl arena_release
#define RADDBGIC_Arena_PosImpl arena_pos
#define RADDBGIC_Arena_PushImpl arena_push
#define RADDBGIC_Arena_PopToImpl arena_pop_to

#include "raddbgi_cons.h"

#endif // RADDBGI_CONS_LOCAL_H
