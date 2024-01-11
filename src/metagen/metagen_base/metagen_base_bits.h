// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_BITS_H
#define BASE_BITS_H

#define ExtractBit(word, idx) (((word) >> (idx)) & 1)

internal U64 count_bits_set16(U16 val);
internal U64 count_bits_set32(U32 val);
internal U64 count_bits_set64(U64 val);

internal U64 ctz32(U32 val);
internal U64 ctz64(U64 val);
internal U64 clz32(U32 val);
internal U64 clz64(U64 val);

#endif // BASE_BITS_H
