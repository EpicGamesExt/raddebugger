// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

internal U32Array  bit_array_init32              (Arena *arena, U64 word_count);
internal U64       bit_array_scan_left_to_right32(U32Array bit_array, U64 lo, U64 hi, B32 state);
internal U64       bit_array_scan_right_to_left32(U32Array bit_array, U64 lo, U64 hi, B32 state);
internal void      bit_array_set_bit32           (U32Array bit_array, U64 idx, B32 state);
internal U32       bit_array_get_bit32           (U32Array bit_array, U64 idx);


