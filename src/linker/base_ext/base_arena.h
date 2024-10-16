// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

internal U32 *   push_u32(Arena *arena, U32 value);
internal U64 *   push_u64(Arena *arena, U64 value);
internal U32 *   push_array_copy_u32(Arena *arena, U32 *v, U64 count);
internal U64 *   push_array_copy_u64(Arena *arena, U64 *v, U64 count);
internal U64 **  push_matrix_u64(Arena *arena, U64 rows, U64 columns);
internal String8 push_cstr(Arena *arena, String8 str);

internal Arena ** alloc_fixed_size_arena_array(Arena *arena, U64 count, U64 res, U64 cmt);
internal void     release_arena_array(Arena **arr);

