// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#if COMPILER_MSVC || (COMPILER_CLANG && OS_WINDOWS)

internal U64
count_bits_set16(U16 val)
{
  return __popcnt16(val);
}

internal U64
count_bits_set32(U32 val)
{
  return __popcnt(val);
}

internal U64
count_bits_set64(U64 val)
{
  return __popcnt64(val);
}

internal U64
ctz32(U32 mask)
{
  unsigned long idx;
  _BitScanForward(&idx, mask);
  return idx;
}

internal U64
ctz64(U64 mask)
{
  unsigned long idx;
  _BitScanForward64(&idx, mask);
  return idx;
}

internal U64
clz32(U32 mask)
{
  unsigned long idx;
  _BitScanReverse(&idx, mask);
  return 31 - idx;
}

internal U64
clz64(U64 mask)
{
  unsigned long idx;
  _BitScanReverse64(&idx, mask);
  return 63 - idx;
}

#elif COMPILER_CLANG || COMPILER_GCC

internal U64
count_bits_set16(U16 val)
{
  NotImplemented;
  return 0;
}

internal U64
count_bits_set32(U32 val)
{
  NotImplemented;
  return 0;
}

internal U64
count_bits_set64(U64 val)
{
  NotImplemented;
  return 0;
}

internal U64
ctz32(U32 val)
{
  NotImplemented;
  return 0;
}

internal U64
clz32(U32 val)
{
  NotImplemented;
  return 0;
}

internal U64
clz64(U64 val)
{
  NotImplemented;
  return 0;
}

#else
# error "bits not defined for this target"
#endif

