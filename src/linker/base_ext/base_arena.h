// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

internal U64 *    push_u64           (Arena *arena, U64 value);
internal U32 *    push_array_copy_u32(Arena *arena, U32 *v, U64 count);
internal U64 *    push_array_copy_u64(Arena *arena, U64 *v, U64 count);
internal Arena ** alloc_fixed_size_arena_array(Arena *arena, U64 count, U64 res, U64 cmt);
#define alloc_arena_array(_arena_, _arena_count_, _counts_, T) alloc_arena_array_(_arena_, _arena_count_, _counts_, sizeof(T))

