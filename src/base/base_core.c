// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Safe Casts

internal U16
safe_cast_u16(U32 x)
{
  AssertAlways(x <= max_U16);
  U16 result = (U16)x;
  return result;
}

internal U32
safe_cast_u32(U64 x)
{
  AssertAlways(x <= max_U32);
  U32 result = (U32)x;
  return result;
}

internal S32
safe_cast_s32(S64 x)
{
  AssertAlways(x <= max_S32);
  S32 result = (S32)x;
  return result;
}

////////////////////////////////
//~ rjf: Large Base Type Functions

internal U128
u128_zero(void)
{
  U128 v = {0};
  return v;
}

internal U128
u128_make(U64 v0, U64 v1)
{
  U128 v = {v0, v1};
  return v;
}

internal B32
u128_match(U128 a, U128 b)
{
  return MemoryMatchStruct(&a, &b);
}

////////////////////////////////
//~ rjf: Bit Patterns

internal U32
u32_from_u64_saturate(U64 x){
  U32 x32 = (x > max_U32)?max_U32:(U32)x;
  return(x32);
}

internal U64
u64_up_to_pow2(U64 x){
  if (x == 0){
    x = 1;
  }
  else{
    x -= 1;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    x |= (x >> 32);
    x += 1;
  }
  return(x);
}

internal S32
extend_sign32(U32 x, U32 size){
  U32 high_bit = size * 8;
  U32 shift = 32 - high_bit;
  S32 result = ((S32)x << shift) >> shift;
  return result;
}

internal S64
extend_sign64(U64 x, U64 size){
  U64 high_bit = size * 8;
  U64 shift = 64 - high_bit;
  S64 result = ((S64)x << shift) >> shift;
  return result;
}

internal F32
inf32(void){
  union { U32 u; F32 f; } x;
  x.u = exponent32;
  return(x.f);
}

internal F32
neg_inf32(void){
  union { U32 u; F32 f; } x;
  x.u = sign32 | exponent32;
  return(x.f);
}

internal U16
bswap_u16(U16 x)
{
  U16 result = (((x & 0xFF00) >> 8) |
                ((x & 0x00FF) << 8));
  return result;
}

internal U32
bswap_u32(U32 x)
{
  U32 result = (((x & 0xFF000000) >> 24) |
                ((x & 0x00FF0000) >> 8)  |
                ((x & 0x0000FF00) << 8)  |
                ((x & 0x000000FF) << 24));
  return result;
}

internal U64
bswap_u64(U64 x)
{
  // TODO(nick): naive bswap, replace with something that is faster like an intrinsic
  U64 result = (((x & 0xFF00000000000000ULL) >> 56) |
                ((x & 0x00FF000000000000ULL) >> 40) |
                ((x & 0x0000FF0000000000ULL) >> 24) |
                ((x & 0x000000FF00000000ULL) >> 8)  |
                ((x & 0x00000000FF000000ULL) << 8)  |
                ((x & 0x0000000000FF0000ULL) << 24) |
                ((x & 0x000000000000FF00ULL) << 40) |
                ((x & 0x00000000000000FFULL) << 56));
  return result;
}

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
# error "Bit intrinsic functions not defined for this compiler."
#endif

////////////////////////////////
//~ rjf: Enum -> Sign

internal S32
sign_from_side_S32(Side side){
  return((side == Side_Min)?-1:1);
}

internal F32
sign_from_side_F32(Side side){
  return((side == Side_Min)?-1.f:1.f);
}

////////////////////////////////
//~ rjf: Memory Functions

internal B32
memory_is_zero(void *ptr, U64 size){
  B32 result = 1;
  
  // break down size
  U64 extra = (size&0x7);
  U64 count8 = (size >> 3);
  
  // check with 8-byte stride
  U64 *p64 = (U64*)ptr;
  if(result)
  {
    for (U64 i = 0; i < count8; i += 1, p64 += 1){
      if (*p64 != 0){
        result = 0;
        goto done;
      }
    }
  }
  
  // check extra
  if(result)
  {
    U8 *p8 = (U8*)p64;
    for (U64 i = 0; i < extra; i += 1, p8 += 1){
      if (*p8 != 0){
        result = 0;
        goto done;
      }
    }
  }
  
  done:;
  return(result);
}

////////////////////////////////
//~ rjf: Text 2D Coordinate/Range Functions

internal TxtPt
txt_pt(S64 line, S64 column)
{
  TxtPt p = {0};
  p.line = line;
  p.column = column;
  return p;
}

internal B32
txt_pt_match(TxtPt a, TxtPt b)
{
  return a.line == b.line && a.column == b.column;
}

internal B32
txt_pt_less_than(TxtPt a, TxtPt b)
{
  B32 result = 0;
  if(a.line < b.line)
  {
    result = 1;
  }
  else if(a.line == b.line)
  {
    result = a.column < b.column;
  }
  return result;
}

internal TxtPt
txt_pt_min(TxtPt a, TxtPt b)
{
  TxtPt result = b;
  if(txt_pt_less_than(a, b))
  {
    result = a;
  }
  return result;
}

internal TxtPt
txt_pt_max(TxtPt a, TxtPt b)
{
  TxtPt result = a;
  if(txt_pt_less_than(a, b))
  {
    result = b;
  }
  return result;
}

internal TxtRng
txt_rng(TxtPt min, TxtPt max)
{
  TxtRng range = {0};
  if(txt_pt_less_than(min, max))
  {
    range.min = min;
    range.max = max;
  }
  else
  {
    range.min = max;
    range.max = min;
  }
  return range;
}

internal TxtRng
txt_rng_intersect(TxtRng a, TxtRng b)
{
  TxtRng result = {0};
  result.min = txt_pt_max(a.min, b.min);
  result.max = txt_pt_min(a.max, b.max);
  if(txt_pt_less_than(result.max, result.min))
  {
    MemoryZeroStruct(&result);
  }
  return result;
}

internal TxtRng
txt_rng_union(TxtRng a, TxtRng b)
{
  TxtRng result = {0};
  result.min = txt_pt_min(a.min, b.min);
  result.max = txt_pt_max(a.max, b.max);
  return result;
}

internal B32
txt_rng_contains(TxtRng r, TxtPt pt)
{
  B32 result = ((txt_pt_less_than(r.min, pt) || txt_pt_match(r.min, pt)) &&
                txt_pt_less_than(pt, r.max));
  return result;
}

////////////////////////////////
//~ rjf: Toolchain/Environment Enum Functions

internal U64
bit_size_from_arch(Architecture arch)
{
  // TODO(rjf): metacode
  U64 arch_bitsize = 0;
  switch(arch)
  {
    case Architecture_x64:   arch_bitsize = 64; break;
    case Architecture_x86:   arch_bitsize = 32; break;
    case Architecture_arm64: arch_bitsize = 64; break;
    case Architecture_arm32: arch_bitsize = 32; break;
    default: break;
  }
  return arch_bitsize;
}

internal U64
max_instruction_size_from_arch(Architecture arch)
{
  // TODO(rjf): make this real
  return 64;
}

internal OperatingSystem
operating_system_from_context(void){
  OperatingSystem os = OperatingSystem_Null;
#if OS_WINDOWS
  os = OperatingSystem_Windows;
#elif OS_LINUX
  os = OperatingSystem_Linux;
#elif OS_MAC
  os = OperatingSystem_Mac;
#endif
  return os;
}

internal Architecture
architecture_from_context(void){
  Architecture arch = Architecture_Null;
#if ARCH_X64
  arch = Architecture_x64;
#elif ARCH_X86
  arch = Architecture_x86;
#elif ARCH_ARM64
  arch = Architecture_arm64;
#elif ARCH_ARM32
  arch = Architecture_arm32;
#endif
  return arch;
}

internal Compiler
compiler_from_context(void){
  Compiler compiler = Compiler_Null;
#if COMPILER_MSVC
  compiler = Compiler_msvc;
#elif COMPILER_GCC
  compiler = Compiler_gcc;
#elif COMPILER_CLANG
  compiler = Compiler_clang;
#endif
  return compiler;
}

////////////////////////////////
//~ rjf: Time Functions

internal DenseTime
dense_time_from_date_time(DateTime date_time){
  DenseTime result = 0;
  result += date_time.year;
  result *= 12;
  result += date_time.mon;
  result *= 31;
  result += date_time.day;
  result *= 24;
  result += date_time.hour;
  result *= 60;
  result += date_time.min;
  result *= 61;
  result += date_time.sec;
  result *= 1000;
  result += date_time.msec;
  return(result);
}

internal DateTime
date_time_from_dense_time(DenseTime time){
  DateTime result = {0};
  result.msec = time%1000;
  time /= 1000;
  result.sec  = time%61;
  time /= 61;
  result.min  = time%60;
  time /= 60;
  result.hour = time%24;
  time /= 24;
  result.day  = time%31;
  time /= 31;
  result.mon  = time%12;
  time /= 12;
  Assert(time <= max_U32);
  result.year = (U32)time;
  return(result);
}

internal DateTime
date_time_from_micro_seconds(U64 time){
  DateTime result = {0};
  result.micro_sec = time%1000;
  time /= 1000;
  result.msec = time%1000;
  time /= 1000;
  result.sec = time%60;
  time /= 60;
  result.min = time%60;
  time /= 60;
  result.hour = time%24;
  time /= 24;
  result.day = time%31;
  time /= 31;
  result.mon = time%12;
  time /= 12;
  Assert(time <= max_U32);
  result.year = (U32)time;
  return(result);
}

////////////////////////////////
//~ rjf: Non-Fancy Ring Buffer Reads/Writes

internal U64
ring_write(U8 *ring_base, U64 ring_size, U64 ring_pos, void *src_data, U64 src_data_size)
{
  Assert(src_data_size <= ring_size);
  {
    U64 ring_off = ring_pos%ring_size;
    U64 bytes_before_split = ring_size-ring_off;
    U64 pre_split_bytes = Min(bytes_before_split, src_data_size);
    U64 pst_split_bytes = src_data_size-pre_split_bytes;
    void *pre_split_data = src_data;
    void *pst_split_data = ((U8 *)src_data + pre_split_bytes);
    MemoryCopy(ring_base+ring_off, pre_split_data, pre_split_bytes);
    MemoryCopy(ring_base+0, pst_split_data, pst_split_bytes);
  }
  return src_data_size;
}

internal U64
ring_read(U8 *ring_base, U64 ring_size, U64 ring_pos, void *dst_data, U64 read_size)
{
  Assert(read_size <= ring_size);
  {
    U64 ring_off = ring_pos%ring_size;
    U64 bytes_before_split = ring_size-ring_off;
    U64 pre_split_bytes = Min(bytes_before_split, read_size);
    U64 pst_split_bytes = read_size-pre_split_bytes;
    MemoryCopy(dst_data, ring_base+ring_off, pre_split_bytes);
    MemoryCopy((U8 *)dst_data + pre_split_bytes, ring_base+0, pst_split_bytes);
  }
  return read_size;
}
