// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "third_party/martins_bitscan/bitscan.h"

internal U32Array
bit_array_init32(Arena *arena, U64 bit_count)
{
  U64 count = CeilIntegerDiv(bit_count, 32);
  return (U32Array){ .count = count, .v = push_array(arena, U32, count) };
}

internal U64
bit_array_scan_left_to_right32(U32Array bit_array, U64 lo, U64 hi, B32 state)
{
  return bitscan_lsb_index32(bit_array.v, lo, hi, state);
}

internal U64
bit_array_scan_right_to_left32(U32Array bit_array, U64 lo, U64 hi, B32 state)
{
  return bitscan_msb_index32(bit_array.v, lo, hi, state);
}

internal void
bit_array_set_bit32(U32Array bit_array, U64 idx, B32 state)
{
  Assert(idx < bit_array.count*32);
  U64 word_idx = idx / 32;
  U64 bit_idx = idx % 32;
  if (state) {
    bit_array.v[word_idx] |= (1u << bit_idx);
  } else {
    bit_array.v[word_idx] &= ~(1u << bit_idx);
  }
}

internal U32
bit_array_get_bit32(U32Array bit_array, U64 idx)
{
  Assert(idx < bit_array.count*32);
  U64 word_idx = idx / 32;
  U64 bit_idx = idx % 32;
  U32 bit = (bit_array.v[word_idx] & (1 << bit_idx)) >> bit_idx;
  return bit;
}

internal B32
bit_array_is_bit_set(U32Array bit_arr, U64 bit_pos)
{
  U64 word_idx = bit_pos / 32;
  Assert(word_idx < bit_arr.count);
  U32 word = bit_arr.v[word_idx];
  U64 bit_idx = bit_pos % 32;
  B32 is_set = !!(word & (1u << bit_idx));
  return is_set;
}


