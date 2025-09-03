// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U16
safe_cast_u16x(U64 x)
{
  AssertAlways(x <= max_U16);
  return (U16)x;
}

internal U64
u128_mod64(U128 a, U64 b)
{
  return a.u64[1] % b;
}

internal Version
make_version(U64 major, U64 minor)
{
  Version version;
  version.major = major;
  version.minor = minor;
  return version;
}

internal int
version_compar(Version a, Version b)
{
  int cmp = 0;
  if (a.major < b.major) {
    cmp = -1;
  } else if (a.major > b.major) {
    cmp = +1;
  } else if (a.major == b.major) {
    if (a.minor < b.minor) {
      cmp = -1;
    } else if (a.minor > b.minor) {
      cmp = +1;
    }
  }
  return cmp;
}

internal ISectOff
isect_off(U32 isect, U32 off)
{
  ISectOff result = { isect, off };
  return result;
}

internal int
u16_compar(const void *raw_a, const void *raw_b)
{
  U16 a = *(U16*)raw_a;
  U16 b = *(U16*)raw_b;
  int result = a < b  ? -1 :
               a > b  ? +1 :
               0;
  return result;
}

internal int
u32_compar(const void *raw_a, const void *raw_b)
{
  U32 a = *(U32*)raw_a;
  U32 b = *(U32*)raw_b;
  int result = a < b  ? -1 :
  a > b  ? +1 :
  0;
  return result;
}

internal int
u64_compar(const void *raw_a, const void *raw_b)
{
  U64 a = *(const U64*)raw_a;
  U64 b = *(const U64*)raw_b;
  int result = a < b  ? -1 : a > b  ? +1 : 0;
  return result;
}

internal int
u64_compar_inv(const void *raw_a, const void *raw_b)
{
  U64 a = *(const U64*)raw_a;
  U64 b = *(const U64*)raw_b;
  int result = a < b  ? +1 : a > b  ? -1 : 0;
  return result;
}

internal int
u16_compar_is_before(void *raw_a, void *raw_b)
{
  U16 *a = (U16 *)raw_a;
  U16 *b = (U16 *)raw_b;
  int is_before = *a < *b;
  return is_before; 
}

internal int
u32_compar_is_before(void *raw_a, void *raw_b)
{
  U32 *a = (U32 *)raw_a;
  U32 *b = (U32 *)raw_b;
  int is_before = *a < *b;
  return is_before; 
}

internal int
u64_compar_is_before(void *raw_a, void *raw_b)
{
  U64 *a = (U64 *)raw_a;
  U64 *b = (U64 *)raw_b;
  int is_before = *a < *b;
  return is_before; 
}

internal int
u8_is_before(void *raw_a, void *raw_b)
{
  U8 *a = (U8 *) raw_a;
  U8 *b = (U8 *) raw_b;
  return *a < *b;
}

internal int
u16_is_before(void *raw_a, void *raw_b)
{
  U16 *a = (U16 *) raw_a;
  U16 *b = (U16 *) raw_b;
  return *a < *b;
}

internal int
u32_is_before(void *raw_a, void *raw_b)
{
  U32 *a = (U32 *) raw_a;
  U32 *b = (U32 *) raw_b;
  return *a < *b;
}

internal int
u64_is_before(void *raw_a, void *raw_b)
{
  U64 *a = (U64 *) raw_a;
  U64 *b = (U64 *) raw_b;
  return *a < *b;
}

internal int
pair_u32_is_before_v0(void *raw_a, void *raw_b)
{
  PairU32 *a = raw_a;
  PairU32 *b = raw_b;
  return a->v0 < b->v0;
}

internal int
pair_u32_is_before(void *raw_a, void *raw_b)
{
  PairU32 *a = raw_a;
  PairU32 *b = raw_b;
  return a->v1 < b->v1;
}

internal int
pair_u64_is_before_v0(void *raw_a, void *raw_b)
{
  PairU64 *a = raw_a;
  PairU64 *b = raw_b;
  return a->v0 < b->v0;
}

internal int
pair_u64_is_before_v1(void *raw_a, void *raw_b)
{
  PairU64 *a = raw_a;
  PairU64 *b = raw_b;
  return a->v1 < b->v1;
}

internal int
pair_u32_compar_v0(const void *raw_a, const void *raw_b)
{
  const PairU32 *a = raw_a;
  const PairU32 *b = raw_b;
  return u32_compar(&a->v0, &b->v0);
}

internal int
pair_u64_compar_v0(const void *raw_a, const void *raw_b)
{
  const PairU64 *a = raw_a;
  const PairU64 *b = raw_b;
  return u64_compar(&a->v0, &b->v0);
}

internal int
pair_u64_compar_v1(const void *raw_a, const void *raw_b)
{
  const PairU64 *a = raw_a;
  const PairU64 *b = raw_b;
  return u64_compar(&a->v1, &b->v1);
}

internal U64
pair_u64_nearest_v0(PairU64 *arr, U64 count, U64 v)
{
  U64 result = max_U64;

  if (count > 1 && arr[0].v0 <= v && v < arr[count-1].v0) {
    U64 l = 0;
    U64 r = count - 1;
    for (; l <= r; ) {
      U64 m = l + (r - l) / 2;
      if (arr[m].v0 == v) {
        return m;
      } else if (arr[m].v0 < v) {
        l = m + 1;
      } else {
        r = m - 1;
      }
    }
    result = l;
  } else if (count == 1 && arr[0].v0 == v) {
    result = 0;
  } else if (count > 0 && v >= arr[count-1].v0) {
    result = count-1;
  }

  return result;
}

internal void
str8_list_concat_in_place_array(String8List *list, String8List *arr, U64 count)
{
  for (U64 i = 0; i < count; ++i) {
    str8_list_concat_in_place(list, &arr[i]);
  }
}



