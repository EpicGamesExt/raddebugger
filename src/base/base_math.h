// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_MATH_H
#define BASE_MATH_H

////////////////////////////////
//~ rjf: Vector Types

//- rjf: 2-vectors

typedef union Vec2F32 Vec2F32;
union Vec2F32
{
  struct
  {
    F32 x;
    F32 y;
  };
  F32 v[2];
};

typedef union Vec2S64 Vec2S64;
union Vec2S64
{
  struct
  {
    S64 x;
    S64 y;
  };
  S64 v[2];
};

typedef union Vec2S32 Vec2S32;
union Vec2S32
{
  struct
  {
    S32 x;
    S32 y;
  };
  S32 v[2];
};

typedef union Vec2S16 Vec2S16;
union Vec2S16
{
  struct
  {
    S16 x;
    S16 y;
  };
  S16 v[2];
};

//- rjf: 3-vectors

typedef union Vec3F32 Vec3F32;
union Vec3F32
{
  struct
  {
    F32 x;
    F32 y;
    F32 z;
  };
  struct
  {
    Vec2F32 xy;
    F32 _z0;
  };
  struct
  {
    F32 _x0;
    Vec2F32 yz;
  };
  F32 v[3];
};

typedef union Vec3S32 Vec3S32;
union Vec3S32
{
  struct
  {
    S32 x;
    S32 y;
    S32 z;
  };
  struct
  {
    Vec2S32 xy;
    S32 _z0;
  };
  struct
  {
    S32 _x0;
    Vec2S32 yz;
  };
  S32 v[3];
};

//- rjf: 4-vectors

typedef union Vec4F32 Vec4F32;
union Vec4F32
{
  struct
  {
    F32 x;
    F32 y;
    F32 z;
    F32 w;
  };
  struct
  {
    Vec2F32 xy;
    Vec2F32 zw;
  };
  struct
  {
    Vec3F32 xyz;
    F32 _z0;
  };
  struct
  {
    F32 _x0;
    Vec3F32 yzw;
  };
  F32 v[4];
};

typedef union Vec4S32 Vec4S32;
union Vec4S32
{
  struct
  {
    S32 x;
    S32 y;
    S32 z;
    S32 w;
  };
  struct
  {
    Vec2S32 xy;
    Vec2S32 zw;
  };
  struct
  {
    Vec3S32 xyz;
    S32 _z0;
  };
  struct
  {
    S32 _x0;
    Vec3S32 yzw;
  };
  S32 v[4];
};

////////////////////////////////
//~ rjf: Matrix Types

typedef struct Mat3x3F32 Mat3x3F32;
struct Mat3x3F32
{
  F32 v[3][3];
};

typedef struct Mat4x4F32 Mat4x4F32;
struct Mat4x4F32
{
  F32 v[4][4];
};

////////////////////////////////
//~ rjf: Range Types

//- rjf: 1-range

typedef union Rng1U32 Rng1U32;
union Rng1U32
{
  struct
  {
    U32 min;
    U32 max;
  };
  U32 v[2];
};

typedef union Rng1S32 Rng1S32;
union Rng1S32
{
  struct
  {
    S32 min;
    S32 max;
  };
  S32 v[2];
};

typedef union Rng1U64 Rng1U64;
union Rng1U64
{
  struct
  {
    U64 min;
    U64 max;
  };
  U64 v[2];
};

typedef union Rng1S64 Rng1S64;
union Rng1S64
{
  struct
  {
    S64 min;
    S64 max;
  };
  S64 v[2];
};

typedef union Rng1F32 Rng1F32;
union Rng1F32
{
  struct
  {
    F32 min;
    F32 max;
  };
  F32 v[2];
};

//- rjf: 2-range (rectangles)

typedef union Rng2S16 Rng2S16;
union Rng2S16
{
  struct
  {
    Vec2S16 min;
    Vec2S16 max;
  };
  struct
  {
    Vec2S16 p0;
    Vec2S16 p1;
  };
  struct
  {
    S16 x0;
    S16 y0;
    S16 x1;
    S16 y1;
  };
  Vec2S16 v[2];
};

typedef union Rng2S32 Rng2S32;
union Rng2S32
{
  struct
  {
    Vec2S32 min;
    Vec2S32 max;
  };
  struct
  {
    Vec2S32 p0;
    Vec2S32 p1;
  };
  struct
  {
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;
  };
  Vec2S32 v[2];
};

typedef union Rng2F32 Rng2F32;
union Rng2F32
{
  struct
  {
    Vec2F32 min;
    Vec2F32 max;
  };
  struct
  {
    Vec2F32 p0;
    Vec2F32 p1;
  };
  struct
  {
    F32 x0;
    F32 y0;
    F32 x1;
    F32 y1;
  };
  Vec2F32 v[2];
};

typedef union Rng2S64 Rng2S64;
union Rng2S64
{
  struct
  {
    Vec2S64 min;
    Vec2S64 max;
  };
  struct
  {
    Vec2S64 p0;
    Vec2S64 p1;
  };
  struct
  {
    S64 x0;
    S64 y0;
    S64 x1;
    S64 y1;
  };
  Vec2S64 v[2];
};

////////////////////////////////
//~ rjf: List Types

typedef struct Rng1S64Node Rng1S64Node;
struct Rng1S64Node
{
  Rng1S64Node *next;
  Rng1S64 v;
};

typedef struct Rng1S64List Rng1S64List;
struct Rng1S64List
{
  Rng1S64Node *first;
  Rng1S64Node *last;
  U64 count;
};

typedef struct Rng1S64Array Rng1S64Array;
struct Rng1S64Array
{
  Rng1S64 *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Scalar Ops

#define abs_s64(v) (S64)llabs(v)

#define sqrt_f32(v)   sqrtf(v)
#define mod_f32(a, b) fmodf((a), (b))
#define pow_f32(b, e) powf((b), (e))
#define ceil_f32(v)   ceilf(v)
#define floor_f32(v)  floorf(v)
#define round_f32(v)  roundf(v)
#define abs_f32(v)    fabsf(v)
#define radians_from_turns_f32(v) ((v)*2*3.1415926535897f)
#define turns_from_radians_f32(v) ((v)/2*3.1415926535897f)
#define degrees_from_turns_f32(v) ((v)*360.f)
#define turns_from_degrees_f32(v) ((v)/360.f)
#define degrees_from_radians_f32(v) (degrees_from_turns_f32(turns_from_radians_f32(v)))
#define radians_from_degrees_f32(v) (radians_from_turns_f32(turns_from_degrees_f32(v)))
#define sin_f32(v)    sinf(radians_from_turns_f32(v))
#define cos_f32(v)    cosf(radians_from_turns_f32(v))
#define tan_f32(v)    tanf(radians_from_turns_f32(v))

#define sqrt_f64(v)   sqrt(v)
#define mod_f64(a, b) fmod((a), (b))
#define pow_f64(b, e) pow((b), (e))
#define ceil_f64(v)   ceil(v)
#define floor_f64(v)  floor(v)
#define round_f64(v)  round(v)
#define abs_f64(v)    fabs(v)
#define radians_from_turns_f64(v) ((v)*2*3.1415926535897)
#define turns_from_radians_f64(v) ((v)/2*3.1415926535897)
#define degrees_from_turns_f64(v) ((v)*360.0)
#define turns_from_degrees_f64(v) ((v)/360.0)
#define degrees_from_radians_f64(v) (degrees_from_turns_f64(turns_from_radians_f64(v)))
#define radians_from_degrees_f64(v) (radians_from_turns_f64(turns_from_degrees_f64(v)))
#define sin_f64(v)    sin(radians_from_turns_f64(v))
#define cos_f64(v)    cos(radians_from_turns_f64(v))
#define tan_f64(v)    tan(radians_from_turns_f64(v))

internal F32 mix_1f32(F32 a, F32 b, F32 t);
internal F64 mix_1f64(F64 a, F64 b, F64 t);

////////////////////////////////
//~ rjf: Vector Ops

#define v2f32(x, y) vec_2f32((x), (y))
internal Vec2F32 vec_2f32(F32 x, F32 y);
internal Vec2F32 add_2f32(Vec2F32 a, Vec2F32 b);
internal Vec2F32 sub_2f32(Vec2F32 a, Vec2F32 b);
internal Vec2F32 mul_2f32(Vec2F32 a, Vec2F32 b);
internal Vec2F32 div_2f32(Vec2F32 a, Vec2F32 b);
internal Vec2F32 scale_2f32(Vec2F32 v, F32 s);
internal F32 dot_2f32(Vec2F32 a, Vec2F32 b);
internal F32 length_squared_2f32(Vec2F32 v);
internal F32 length_2f32(Vec2F32 v);
internal Vec2F32 normalize_2f32(Vec2F32 v);
internal Vec2F32 mix_2f32(Vec2F32 a, Vec2F32 b, F32 t);

#define v2s64(x, y) vec_2s64((x), (y))
internal Vec2S64 vec_2s64(S64 x, S64 y);
internal Vec2S64 add_2s64(Vec2S64 a, Vec2S64 b);
internal Vec2S64 sub_2s64(Vec2S64 a, Vec2S64 b);
internal Vec2S64 mul_2s64(Vec2S64 a, Vec2S64 b);
internal Vec2S64 div_2s64(Vec2S64 a, Vec2S64 b);
internal Vec2S64 scale_2s64(Vec2S64 v, S64 s);
internal S64 dot_2s64(Vec2S64 a, Vec2S64 b);
internal S64 length_squared_2s64(Vec2S64 v);
internal S64 length_2s64(Vec2S64 v);
internal Vec2S64 normalize_2s64(Vec2S64 v);
internal Vec2S64 mix_2s64(Vec2S64 a, Vec2S64 b, F32 t);

#define v2s32(x, y) vec_2s32((x), (y))
internal Vec2S32 vec_2s32(S32 x, S32 y);
internal Vec2S32 add_2s32(Vec2S32 a, Vec2S32 b);
internal Vec2S32 sub_2s32(Vec2S32 a, Vec2S32 b);
internal Vec2S32 mul_2s32(Vec2S32 a, Vec2S32 b);
internal Vec2S32 div_2s32(Vec2S32 a, Vec2S32 b);
internal Vec2S32 scale_2s32(Vec2S32 v, S32 s);
internal S32 dot_2s32(Vec2S32 a, Vec2S32 b);
internal S32 length_squared_2s32(Vec2S32 v);
internal S32 length_2s32(Vec2S32 v);
internal Vec2S32 normalize_2s32(Vec2S32 v);
internal Vec2S32 mix_2s32(Vec2S32 a, Vec2S32 b, F32 t);

#define v2s16(x, y) vec_2s16((x), (y))
internal Vec2S16 vec_2s16(S16 x, S16 y);
internal Vec2S16 add_2s16(Vec2S16 a, Vec2S16 b);
internal Vec2S16 sub_2s16(Vec2S16 a, Vec2S16 b);
internal Vec2S16 mul_2s16(Vec2S16 a, Vec2S16 b);
internal Vec2S16 div_2s16(Vec2S16 a, Vec2S16 b);
internal Vec2S16 scale_2s16(Vec2S16 v, S16 s);
internal S16 dot_2s16(Vec2S16 a, Vec2S16 b);
internal S16 length_squared_2s16(Vec2S16 v);
internal S16 length_2s16(Vec2S16 v);
internal Vec2S16 normalize_2s16(Vec2S16 v);
internal Vec2S16 mix_2s16(Vec2S16 a, Vec2S16 b, F32 t);

#define v3f32(x, y, z) vec_3f32((x), (y), (z))
internal Vec3F32 vec_3f32(F32 x, F32 y, F32 z);
internal Vec3F32 add_3f32(Vec3F32 a, Vec3F32 b);
internal Vec3F32 sub_3f32(Vec3F32 a, Vec3F32 b);
internal Vec3F32 mul_3f32(Vec3F32 a, Vec3F32 b);
internal Vec3F32 div_3f32(Vec3F32 a, Vec3F32 b);
internal Vec3F32 scale_3f32(Vec3F32 v, F32 s);
internal F32 dot_3f32(Vec3F32 a, Vec3F32 b);
internal F32 length_squared_3f32(Vec3F32 v);
internal F32 length_3f32(Vec3F32 v);
internal Vec3F32 normalize_3f32(Vec3F32 v);
internal Vec3F32 mix_3f32(Vec3F32 a, Vec3F32 b, F32 t);
internal Vec3F32 cross_3f32(Vec3F32 a, Vec3F32 b);

#define v3s32(x, y, z) vec_3s32((x), (y), (z))
internal Vec3S32 vec_3s32(S32 x, S32 y, S32 z);
internal Vec3S32 add_3s32(Vec3S32 a, Vec3S32 b);
internal Vec3S32 sub_3s32(Vec3S32 a, Vec3S32 b);
internal Vec3S32 mul_3s32(Vec3S32 a, Vec3S32 b);
internal Vec3S32 div_3s32(Vec3S32 a, Vec3S32 b);
internal Vec3S32 scale_3s32(Vec3S32 v, S32 s);
internal S32 dot_3s32(Vec3S32 a, Vec3S32 b);
internal S32 length_squared_3s32(Vec3S32 v);
internal S32 length_3s32(Vec3S32 v);
internal Vec3S32 normalize_3s32(Vec3S32 v);
internal Vec3S32 mix_3s32(Vec3S32 a, Vec3S32 b, F32 t);
internal Vec3S32 cross_3s32(Vec3S32 a, Vec3S32 b);

#define v4f32(x, y, z, w) vec_4f32((x), (y), (z), (w))
internal Vec4F32 vec_4f32(F32 x, F32 y, F32 z, F32 w);
internal Vec4F32 add_4f32(Vec4F32 a, Vec4F32 b);
internal Vec4F32 sub_4f32(Vec4F32 a, Vec4F32 b);
internal Vec4F32 mul_4f32(Vec4F32 a, Vec4F32 b);
internal Vec4F32 div_4f32(Vec4F32 a, Vec4F32 b);
internal Vec4F32 scale_4f32(Vec4F32 v, F32 s);
internal F32 dot_4f32(Vec4F32 a, Vec4F32 b);
internal F32 length_squared_4f32(Vec4F32 v);
internal F32 length_4f32(Vec4F32 v);
internal Vec4F32 normalize_4f32(Vec4F32 v);
internal Vec4F32 mix_4f32(Vec4F32 a, Vec4F32 b, F32 t);

#define v4s32(x, y, z, w) vec_4s32((x), (y), (z), (w))
internal Vec4S32 vec_4s32(S32 x, S32 y, S32 z, S32 w);
internal Vec4S32 add_4s32(Vec4S32 a, Vec4S32 b);
internal Vec4S32 sub_4s32(Vec4S32 a, Vec4S32 b);
internal Vec4S32 mul_4s32(Vec4S32 a, Vec4S32 b);
internal Vec4S32 div_4s32(Vec4S32 a, Vec4S32 b);
internal Vec4S32 scale_4s32(Vec4S32 v, S32 s);
internal S32 dot_4s32(Vec4S32 a, Vec4S32 b);
internal S32 length_squared_4s32(Vec4S32 v);
internal S32 length_4s32(Vec4S32 v);
internal Vec4S32 normalize_4s32(Vec4S32 v);
internal Vec4S32 mix_4s32(Vec4S32 a, Vec4S32 b, F32 t);

////////////////////////////////
//~ rjf: Matrix Ops

internal Mat3x3F32 mat_3x3f32(F32 diagonal);
internal Mat3x3F32 make_translate_3x3f32(Vec2F32 delta);
internal Mat3x3F32 make_scale_3x3f32(Vec2F32 scale);
internal Mat3x3F32 mul_3x3f32(Mat3x3F32 a, Mat3x3F32 b);

internal Mat4x4F32 mat_4x4f32(F32 diagonal);
internal Mat4x4F32 make_translate_4x4f32(Vec3F32 delta);
internal Mat4x4F32 make_scale_4x4f32(Vec3F32 scale);
internal Mat4x4F32 make_perspective_4x4f32(F32 fov, F32 aspect_ratio, F32 near_z, F32 far_z);
internal Mat4x4F32 make_orthographic_4x4f32(F32 left, F32 right, F32 bottom, F32 top, F32 near_z, F32 far_z);
internal Mat4x4F32 make_look_at_4x4f32(Vec3F32 eye, Vec3F32 center, Vec3F32 up);
internal Mat4x4F32 make_rotate_4x4f32(Vec3F32 axis, F32 turns);
internal Mat4x4F32 mul_4x4f32(Mat4x4F32 a, Mat4x4F32 b);
internal Mat4x4F32 scale_4x4f32(Mat4x4F32 m, F32 scale);
internal Mat4x4F32 inverse_4x4f32(Mat4x4F32 m);
internal Mat4x4F32 derotate_4x4f32(Mat4x4F32 mat);

////////////////////////////////
//~ rjf: Range Ops

#define r1u32(min, max) rng_1u32((min), (max))
internal Rng1U32 rng_1u32(U32 min, U32 max);
internal Rng1U32 shift_1u32(Rng1U32 r, U32 x);
internal Rng1U32 pad_1u32(Rng1U32 r, U32 x);
internal U32 center_1u32(Rng1U32 r);
internal B32 contains_1u32(Rng1U32 r, U32 x);
internal U32 dim_1u32(Rng1U32 r);
internal Rng1U32 union_1u32(Rng1U32 a, Rng1U32 b);
internal Rng1U32 intersect_1u32(Rng1U32 a, Rng1U32 b);
internal U32 clamp_1u32(Rng1U32 r, U32 v);

#define r1s32(min, max) rng_1s32((min), (max))
internal Rng1S32 rng_1s32(S32 min, S32 max);
internal Rng1S32 shift_1s32(Rng1S32 r, S32 x);
internal Rng1S32 pad_1s32(Rng1S32 r, S32 x);
internal S32 center_1s32(Rng1S32 r);
internal B32 contains_1s32(Rng1S32 r, S32 x);
internal S32 dim_1s32(Rng1S32 r);
internal Rng1S32 union_1s32(Rng1S32 a, Rng1S32 b);
internal Rng1S32 intersect_1s32(Rng1S32 a, Rng1S32 b);
internal S32 clamp_1s32(Rng1S32 r, S32 v);

#define r1u64(min, max) rng_1u64((min), (max))
internal Rng1U64 rng_1u64(U64 min, U64 max);
internal Rng1U64 shift_1u64(Rng1U64 r, U64 x);
internal Rng1U64 pad_1u64(Rng1U64 r, U64 x);
internal U64 center_1u64(Rng1U64 r);
internal B32 contains_1u64(Rng1U64 r, U64 x);
internal U64 dim_1u64(Rng1U64 r);
internal Rng1U64 union_1u64(Rng1U64 a, Rng1U64 b);
internal Rng1U64 intersect_1u64(Rng1U64 a, Rng1U64 b);
internal U64 clamp_1u64(Rng1U64 r, U64 v);

#define r1s64(min, max) rng_1s64((min), (max))
internal Rng1S64 rng_1s64(S64 min, S64 max);
internal Rng1S64 shift_1s64(Rng1S64 r, S64 x);
internal Rng1S64 pad_1s64(Rng1S64 r, S64 x);
internal S64 center_1s64(Rng1S64 r);
internal B32 contains_1s64(Rng1S64 r, S64 x);
internal S64 dim_1s64(Rng1S64 r);
internal Rng1S64 union_1s64(Rng1S64 a, Rng1S64 b);
internal Rng1S64 intersect_1s64(Rng1S64 a, Rng1S64 b);
internal S64 clamp_1s64(Rng1S64 r, S64 v);

#define r1f32(min, max) rng_1f32((min), (max))
internal Rng1F32 rng_1f32(F32 min, F32 max);
internal Rng1F32 shift_1f32(Rng1F32 r, F32 x);
internal Rng1F32 pad_1f32(Rng1F32 r, F32 x);
internal F32 center_1f32(Rng1F32 r);
internal B32 contains_1f32(Rng1F32 r, F32 x);
internal F32 dim_1f32(Rng1F32 r);
internal Rng1F32 union_1f32(Rng1F32 a, Rng1F32 b);
internal Rng1F32 intersect_1f32(Rng1F32 a, Rng1F32 b);
internal F32 clamp_1f32(Rng1F32 r, F32 v);

#define r2s16(min, max) rng_2s16((min), (max))
#define r2s16p(x, y, z, w) r2s16(v2s16((x), (y)), v2s16((z), (w)))
internal Rng2S16 rng_2s16(Vec2S16 min, Vec2S16 max);
internal Rng2S16 shift_2s16(Rng2S16 r, Vec2S16 x);
internal Rng2S16 pad_2s16(Rng2S16 r, S16 x);
internal Vec2S16 center_2s16(Rng2S16 r);
internal B32 contains_2s16(Rng2S16 r, Vec2S16 x);
internal Vec2S16 dim_2s16(Rng2S16 r);
internal Rng2S16 union_2s16(Rng2S16 a, Rng2S16 b);
internal Rng2S16 intersect_2s16(Rng2S16 a, Rng2S16 b);
internal Vec2S16 clamp_2s16(Rng2S16 r, Vec2S16 v);

#define r2s32(min, max) rng_2s32((min), (max))
#define r2s32p(x, y, z, w) r2s32(v2s32((x), (y)), v2s32((z), (w)))
internal Rng2S32 rng_2s32(Vec2S32 min, Vec2S32 max);
internal Rng2S32 shift_2s32(Rng2S32 r, Vec2S32 x);
internal Rng2S32 pad_2s32(Rng2S32 r, S32 x);
internal Vec2S32 center_2s32(Rng2S32 r);
internal B32 contains_2s32(Rng2S32 r, Vec2S32 x);
internal Vec2S32 dim_2s32(Rng2S32 r);
internal Rng2S32 union_2s32(Rng2S32 a, Rng2S32 b);
internal Rng2S32 intersect_2s32(Rng2S32 a, Rng2S32 b);
internal Vec2S32 clamp_2s32(Rng2S32 r, Vec2S32 v);

#define r2s64(min, max) rng_2s64((min), (max))
#define r2s64p(x, y, z, w) r2s64(v2s64((x), (y)), v2s64((z), (w)))
internal Rng2S64 rng_2s64(Vec2S64 min, Vec2S64 max);
internal Rng2S64 shift_2s64(Rng2S64 r, Vec2S64 x);
internal Rng2S64 pad_2s64(Rng2S64 r, S64 x);
internal Vec2S64 center_2s64(Rng2S64 r);
internal B32 contains_2s64(Rng2S64 r, Vec2S64 x);
internal Vec2S64 dim_2s64(Rng2S64 r);
internal Rng2S64 union_2s64(Rng2S64 a, Rng2S64 b);
internal Rng2S64 intersect_2s64(Rng2S64 a, Rng2S64 b);
internal Vec2S64 clamp_2s64(Rng2S64 r, Vec2S64 v);

#define r2f32(min, max) rng_2f32((min), (max))
#define r2f32p(x, y, z, w) r2f32(v2f32((x), (y)), v2f32((z), (w)))
internal Rng2F32 rng_2f32(Vec2F32 min, Vec2F32 max);
internal Rng2F32 shift_2f32(Rng2F32 r, Vec2F32 x);
internal Rng2F32 pad_2f32(Rng2F32 r, F32 x);
internal Vec2F32 center_2f32(Rng2F32 r);
internal B32 contains_2f32(Rng2F32 r, Vec2F32 x);
internal Vec2F32 dim_2f32(Rng2F32 r);
internal Rng2F32 union_2f32(Rng2F32 a, Rng2F32 b);
internal Rng2F32 intersect_2f32(Rng2F32 a, Rng2F32 b);
internal Vec2F32 clamp_2f32(Rng2F32 r, Vec2F32 v);

////////////////////////////////
//~ rjf: Miscellaneous Ops

internal Vec3F32 hsv_from_rgb(Vec3F32 rgb);
internal Vec3F32 rgb_from_hsv(Vec3F32 hsv);
internal Vec4F32 hsva_from_rgba(Vec4F32 rgba);
internal Vec4F32 rgba_from_hsva(Vec4F32 hsva);
internal Vec4F32 rgba_from_u32(U32 hex);
internal U32 u32_from_rgba(Vec4F32 rgba);

#define rgba_from_u32_lit_comp(h) { (((h)&0xff000000)>>24)/255.f, (((h)&0x00ff0000)>>16)/255.f, (((h)&0x0000ff00)>> 8)/255.f, (((h)&0x000000ff)>> 0)/255.f }

////////////////////////////////
//~ rjf: List Type Functions

internal void rng1s64_list_push(Arena *arena, Rng1S64List *list, Rng1S64 rng);
internal Rng1S64Array rng1s64_array_from_list(Arena *arena, Rng1S64List *list);

#endif // BASE_MATH_H
