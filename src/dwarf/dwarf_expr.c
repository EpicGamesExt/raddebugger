// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal DW_PieceNode *
dw_piece_list_push(Arena *arena, DW_PieceList *list, DW_Piece v)
{
  DW_PieceNode *n = push_array(arena, DW_PieceNode, 1);
  n->v = v;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  return n;
}

internal DW_ExprValueType
dw_expr_unsigned_value_type_from_bit_size(U64 bit_size)
{
  switch (bit_size) {
  case 8:   return DW_ExprValueType_U8;
  case 16:  return DW_ExprValueType_U16;
  case 32:  return DW_ExprValueType_U32;
  case 64:  return DW_ExprValueType_U64;
  case 128: return DW_ExprValueType_U128;
  case 256: return DW_ExprValueType_U256;
  case 512: return DW_ExprValueType_U512;
  }
  AssertAlways(0 && "no suitable unsigned type was found for the specified size");
  return DW_ExprValueType_Generic;
}

internal DW_ExprValueType
dw_expr_signed_value_type_from_bit_size(U64 bit_size)
{
  switch (bit_size) {
  case 8:   return DW_ExprValueType_S8;
  case 16:  return DW_ExprValueType_S16;
  case 32:  return DW_ExprValueType_S32;
  case 64:  return DW_ExprValueType_S64;
  case 128: return DW_ExprValueType_S128;
  case 256: return DW_ExprValueType_S256;
  case 512: return DW_ExprValueType_S512;
  }
  AssertAlways(0 && "no suitable signed type was found for the specified size");
  return DW_ExprValueType_Generic;
}

internal DW_ExprValueType
dw_expr_float_type_from_bit_size(U64 bit_size)
{
  switch (bit_size) {
  case 4: return DW_ExprValueType_F32;
  case 8: return DW_ExprValueType_F64;
  }
  AssertAlways(0 && "no suitable type was found for the specified size");
  return DW_ExprValueType_Generic;
}

internal U64
dw_expr_byte_size_from_value_type(U64 addr_size, DW_ExprValueType k)
{
  switch (k) {
  default: { InvalidPath; }
  case DW_ExprValueType_Generic: return 0;
  case DW_ExprValueType_Addr:    return addr_size;
  case DW_ExprValueType_U8:      return 1;
  case DW_ExprValueType_U16:     return 2;
  case DW_ExprValueType_U32:     return 4;
  case DW_ExprValueType_U64:     return 8;
  case DW_ExprValueType_U128:    return 16;
  case DW_ExprValueType_U256:    return 32;
  case DW_ExprValueType_U512:    return 64;
  case DW_ExprValueType_S8:      return 1;
  case DW_ExprValueType_S16:     return 2;
  case DW_ExprValueType_S32:     return 4;
  case DW_ExprValueType_S64:     return 8;
  case DW_ExprValueType_S128:    return 16;
  case DW_ExprValueType_S256:    return 32;
  case DW_ExprValueType_S512:    return 64;
  case DW_ExprValueType_F32:     return 4;
  case DW_ExprValueType_F64:     return 8;
  }
}

internal DW_ExprValueType
dw_expr_pick_common_value_type(DW_ExprValueType lhs, DW_ExprValueType rhs)
{
  if (lhs == rhs) {
    return lhs;
  }
  // unsigned vs unsigned
  else if (DW_ExprValueType_IsUnsigned(lhs) && DW_ExprValueType_IsUnsigned(rhs)) {
    return Max(lhs, rhs);
  }
  // signed vs signed
  else if (DW_ExprValueType_IsSigned(lhs) && DW_ExprValueType_IsSigned(rhs)) {
    return Max(lhs, rhs);
  }
  // (unsigned vs signed) || (signed vs unsigned)
  else if ((DW_ExprValueType_IsUnsigned(lhs) && DW_ExprValueType_IsSigned(rhs)) ||
           (DW_ExprValueType_IsSigned(lhs) && DW_ExprValueType_IsUnsigned(rhs))) {
    U64 lhs_size = dw_expr_byte_size_from_value_type(0, lhs);
    U64 rhs_size = dw_expr_byte_size_from_value_type(0, rhs);
    if (lhs_size < rhs_size) {
      return rhs;
    } else if (lhs > rhs_size) {
      return lhs;
    } else {
      return dw_expr_unsigned_value_type_from_bit_size(lhs_size * 8);
    }
  }
  // float vs int
  else if (DW_ExprValueType_IsFloat(lhs) && DW_ExprValueType_IsInt(rhs)) {
    return lhs;
  }
  // int vs float
  else if (DW_ExprValueType_IsInt(lhs) && DW_ExprValueType_IsFloat(rhs)) {
    return rhs;
  }
  // float vs float
  else if (DW_ExprValueType_IsFloat(lhs) && DW_ExprValueType_IsFloat(rhs)) {
    return Max(lhs, rhs);
  }
  // address vs int
  else if (lhs == DW_ExprValueType_Addr && DW_ExprValueType_IsInt(rhs)) {
    return DW_ExprValueType_Addr;
  }
  // int vs address
  else if (DW_ExprValueType_IsInt(lhs) && rhs == DW_ExprValueType_Addr) {
    return DW_ExprValueType_Addr;
  }
  // address vs float
  else if (lhs == DW_ExprValueType_Addr && DW_ExprValueType_IsFloat(rhs)) {
    return DW_ExprValueType_Generic;
  }
  // float vs address
  else if (DW_ExprValueType_IsFloat(lhs) && rhs == DW_ExprValueType_Addr) {
    return DW_ExprValueType_Generic;
  }
  // no conversion for implicit value
  else if (lhs == DW_ExprValueType_Implicit || rhs == DW_ExprValueType_Implicit) {
    return DW_ExprValueType_Generic;
  }
  AssertAlways(!"undefined conversion case");
  return DW_ExprValueType_Generic;
}

internal DW_ExprValueType
dw_expr_pick_common_compar_value_type(DW_ExprValueType lhs, DW_ExprValueType rhs)
{
  DW_ExprValueType result;
  if (lhs == DW_ExprValueType_Generic || rhs == DW_ExprValueType_Generic) {
    result = DW_ExprValueType_S64;
  } else {
    result = dw_expr_pick_common_value_type(lhs, rhs);
  }
  return result;
}

internal DW_ExprValue
dw_expr_cast(DW_ExprValue value, DW_ExprValueType type)
{
  DW_ExprValue result = { type };

#define CastTable(f, t)                                                                                                     \
    switch (value.type) {                                                                                                   \
    case DW_ExprValueType_Generic: { MemoryCopy(&result.f, value.generic.str, Min(sizeof(t), value.generic.size)); } break; \
    case DW_ExprValueType_U8:      { result.f = (t)value.u8;  } break;                                                      \
    case DW_ExprValueType_U16:     { result.f = (t)value.u16; } break;                                                      \
    case DW_ExprValueType_U32:     { result.f = (t)value.u32; } break;                                                      \
    case DW_ExprValueType_U64:     { result.f = (t)value.u64; } break;                                                      \
    case DW_ExprValueType_U128:    { NotImplemented; } break;                                                               \
    case DW_ExprValueType_U256:    { NotImplemented; } break;                                                               \
    case DW_ExprValueType_U512:    { NotImplemented; } break;                                                               \
    case DW_ExprValueType_S8:      { result.f = (t)value.s8;  } break;                                                      \
    case DW_ExprValueType_S16:     { result.f = (t)value.s16; } break;                                                      \
    case DW_ExprValueType_S32:     { result.f = (t)value.s32; } break;                                                      \
    case DW_ExprValueType_S64:     { result.f = (t)value.s64; } break;                                                      \
    case DW_ExprValueType_S128:    { NotImplemented; } break;                                                               \
    case DW_ExprValueType_S256:    { NotImplemented; } break;                                                               \
    case DW_ExprValueType_S512:    { NotImplemented; } break;                                                               \
    case DW_ExprValueType_F32:     { result.f = (t)value.f32;  } break;                                                     \
    case DW_ExprValueType_F64:     { result.f = (t)value.f64;  } break;                                                     \
    case DW_ExprValueType_Addr:    { result.f = (t)value.addr; } break;                                                     \
    case DW_ExprValueType_Implicit: { InvalidPath; } break;                                                                 \
    case DW_ExprValueType_Bool:     { result.f = (t)value.boolean; } break;                                                 \
    default: { InvalidPath; } break;                                                                                        \
    }

  switch (type) {
  case DW_ExprValueType_Generic: {} break;
  case DW_ExprValueType_U8:       { CastTable(u8, U8);               } break;
  case DW_ExprValueType_U16:      { CastTable(u16, U16);             } break;
  case DW_ExprValueType_U32:      { CastTable(u32, U32);             } break;
  case DW_ExprValueType_U64:      { CastTable(u64, U64);             } break;
  case DW_ExprValueType_U128:     { NotImplemented;                  } break;
  case DW_ExprValueType_U256:     { NotImplemented;                  } break;
  case DW_ExprValueType_U512:     { NotImplemented;                  } break;
  case DW_ExprValueType_S8:       { CastTable(s8, S8);               } break;
  case DW_ExprValueType_S16:      { CastTable(s16, S16);             } break;
  case DW_ExprValueType_S32:      { CastTable(s32, S32);             } break;
  case DW_ExprValueType_S64:      { CastTable(s64, S64);             } break;
  case DW_ExprValueType_S128:     { NotImplemented;                  } break;
  case DW_ExprValueType_S256:     { NotImplemented;                  } break;
  case DW_ExprValueType_S512:     { NotImplemented;                  } break;
  case DW_ExprValueType_F32:      { CastTable(f32, F32);             } break;
  case DW_ExprValueType_F64:      { CastTable(f64, F64);             } break;
  case DW_ExprValueType_Addr:     { CastTable(addr, U64);            } break;
  case DW_ExprValueType_Implicit: { NotImplemented;                  } break;
  case DW_ExprValueType_Bool:     { CastTable(boolean, DW_ExprBool); } break;
  }

#undef CastTable
  return result;
}

internal DW_ExprValue
dw_expr_add(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  + common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 + common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 + common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 + common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  + common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  + common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 + common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 + common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 + common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { result.f32  = common_lhs.f32  + common_rhs.f32;  } break;
  case DW_ExprValueType_F64:  { result.f64  = common_lhs.f64  + common_rhs.f64;  } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr + common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }
  return result;
}

internal DW_ExprValue
dw_expr_minus(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  - common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 - common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 - common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 - common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  - common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  - common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 - common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 - common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 - common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { result.f32  = common_lhs.f32  - common_rhs.f32;  } break;
  case DW_ExprValueType_F64:  { result.f64  = common_lhs.f64  - common_rhs.f64;  } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr - common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }
  return result;
}

internal DW_ExprValue
dw_expr_mul(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  * common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 * common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 * common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 * common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  * common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  * common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 * common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 * common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 * common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { result.f32  = common_lhs.f32  * common_rhs.f32;  } break;
  case DW_ExprValueType_F64:  { result.f64  = common_lhs.f64  * common_rhs.f64;  } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr * common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }
  return result;
}

internal DW_ExprValue
dw_expr_div(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  / common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 / common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 / common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 / common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  / common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  / common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 / common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 / common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 / common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { result.f32  = common_lhs.f32  / common_rhs.f32;  } break;
  case DW_ExprValueType_F64:  { result.f64  = common_lhs.f64  / common_rhs.f64;  } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr / common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }
  return result;
}

internal DW_ExprValue
dw_expr_mod(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  % common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 % common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 % common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 % common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  % common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  % common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 % common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 % common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 % common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { InvalidPath; } break;
  case DW_ExprValueType_F64:  { InvalidPath; } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr % common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }
  return result;
}

internal DW_ExprValue
dw_expr_eq(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  == common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 == common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 == common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 == common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  == common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  == common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 == common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 == common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 == common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { result.f32  = common_lhs.f32  == common_rhs.f32;  } break;
  case DW_ExprValueType_F64:  { result.f64  = common_lhs.f64  == common_rhs.f64;  } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr == common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_ge(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  >= common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 >= common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 >= common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 >= common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  >= common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  >= common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 >= common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 >= common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 >= common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { result.f32  = common_lhs.f32  >= common_rhs.f32;  } break;
  case DW_ExprValueType_F64:  { result.f64  = common_lhs.f64  >= common_rhs.f64;  } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr >= common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_gt(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  > common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 > common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 > common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 > common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  > common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  > common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 > common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 > common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 > common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { result.f32  = common_lhs.f32  > common_rhs.f32;  } break;
  case DW_ExprValueType_F64:  { result.f64  = common_lhs.f64  > common_rhs.f64;  } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr > common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_le(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  <= common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 <= common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 <= common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 <= common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  <= common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  <= common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 <= common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 <= common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 <= common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { result.f32  = common_lhs.f32  <= common_rhs.f32;  } break;
  case DW_ExprValueType_F64:  { result.f64  = common_lhs.f64  <= common_rhs.f64;  } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr <= common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_lt(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  < common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 < common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 < common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 < common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  < common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  < common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 < common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 < common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 < common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { result.f32  = common_lhs.f32  < common_rhs.f32;  } break;
  case DW_ExprValueType_F64:  { result.f64  = common_lhs.f64  < common_rhs.f64;  } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr < common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_ne(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  != common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 != common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 != common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 != common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  != common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  != common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 != common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 != common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 != common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { result.f32  = common_lhs.f32  != common_rhs.f32;  } break;
  case DW_ExprValueType_F64:  { result.f64  = common_lhs.f64  != common_rhs.f64;  } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr != common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_xor(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  ^ common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 ^ common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 ^ common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 ^ common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  ^ common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  ^ common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 ^ common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 ^ common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 ^ common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { InvalidPath; } break;
  case DW_ExprValueType_F64:  { InvalidPath; } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr ^ common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_and(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  & common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 & common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 & common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 & common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  & common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  & common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 & common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 & common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 & common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { InvalidPath; } break;
  case DW_ExprValueType_F64:  { InvalidPath; } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr & common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_or(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  | common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 | common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 | common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 | common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  | common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  | common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 | common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 | common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 | common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { InvalidPath; } break;
  case DW_ExprValueType_F64:  { InvalidPath; } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr | common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_shl(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  << common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 << common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 << common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 << common_rhs.u64; } break;

  case DW_ExprValueType_U128:
  case DW_ExprValueType_U256:
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  << common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  << common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 << common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 << common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 << common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { InvalidPath; } break;
  case DW_ExprValueType_F64:  { InvalidPath; } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr << common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_shr(DW_ExprValue lhs, DW_ExprValue rhs)
{
  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:  { result.u8  = common_lhs.u8  >> common_rhs.u8;  } break;
  case DW_ExprValueType_U16: { result.u16 = common_lhs.u16 >> common_rhs.u16; } break;
  case DW_ExprValueType_U32: { result.u32 = common_lhs.u32 >> common_rhs.u32; } break;
  case DW_ExprValueType_U64: { result.u64 = common_lhs.u64 >> common_rhs.u64; } break;

  case DW_ExprValueType_U128: { NotImplemented; } break;
  case DW_ExprValueType_U256: { NotImplemented; } break;
  case DW_ExprValueType_U512: { NotImplemented; } break;

  case DW_ExprValueType_Bool: { InvalidPath; } break;
  case DW_ExprValueType_S8:   { InvalidPath; } break;
  case DW_ExprValueType_S16:  { InvalidPath; } break;
  case DW_ExprValueType_S32:  { InvalidPath; } break;
  case DW_ExprValueType_S64:  { InvalidPath; } break;
  case DW_ExprValueType_S128: { InvalidPath; } break;
  case DW_ExprValueType_S256: { InvalidPath; } break;
  case DW_ExprValueType_S512: { InvalidPath; } break;

  case DW_ExprValueType_F32:  { InvalidPath; } break;
  case DW_ExprValueType_F64:  { InvalidPath; } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr >> common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_shra(DW_ExprValue lhs, DW_ExprValue rhs)
{
  if (DW_ExprValueType_IsUnsigned(lhs.type)) {
    DW_ExprValueType new_type = dw_expr_signed_value_type_from_bit_size(dw_expr_byte_size_from_value_type(0, lhs.type) * 8);
    lhs = dw_expr_cast(lhs, new_type);
  }

  if (DW_ExprValueType_IsUnsigned(rhs.type)) {
    DW_ExprValueType new_type = dw_expr_signed_value_type_from_bit_size(dw_expr_byte_size_from_value_type(0, rhs.type) * 8);
    rhs = dw_expr_cast(rhs, new_type);
  }

  DW_ExprValue result = {0};
  result.type = dw_expr_pick_common_compar_value_type(lhs.type, rhs.type);

  DW_ExprValue common_lhs = dw_expr_cast(lhs, result.type);
  DW_ExprValue common_rhs = dw_expr_cast(rhs, result.type);

  switch (result.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;
                                 
  case DW_ExprValueType_U8:   { InvalidPath; } break;
  case DW_ExprValueType_U16:  { InvalidPath; } break;
  case DW_ExprValueType_U32:  { InvalidPath; } break;
  case DW_ExprValueType_U64:  { InvalidPath; } break;
  case DW_ExprValueType_U128: { InvalidPath; } break;
  case DW_ExprValueType_U256: { InvalidPath; } break;
  case DW_ExprValueType_U512: { InvalidPath; } break;

  case DW_ExprValueType_Bool: { result.s8  = common_lhs.s8  >> common_rhs.s8;  } break;
  case DW_ExprValueType_S8:   { result.s8  = common_lhs.s8  >> common_rhs.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = common_lhs.s16 >> common_rhs.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = common_lhs.s32 >> common_rhs.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = common_lhs.s64 >> common_rhs.s64; } break;

  case DW_ExprValueType_S128:
  case DW_ExprValueType_S256:
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32:  { InvalidPath; } break;
  case DW_ExprValueType_F64:  { InvalidPath; } break;
  case DW_ExprValueType_Addr: { result.addr = common_lhs.addr >> common_rhs.addr; } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;

  default: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_abs(DW_ExprValue value)
{
  DW_ExprValue result = value;

  switch (value.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;

  case DW_ExprValueType_U8:   {}; break;
  case DW_ExprValueType_U16:  {}; break;
  case DW_ExprValueType_U32:  {}; break;
  case DW_ExprValueType_U64:  {}; break;
  case DW_ExprValueType_U128: {}; break;
  case DW_ExprValueType_U256: {}; break;
  case DW_ExprValueType_U512: {}; break;

  case DW_ExprValueType_S8:   { result.s8  = abs_s64(result.s8);  } break;
  case DW_ExprValueType_S16:  { result.s16 = abs_s64(result.s16); } break;
  case DW_ExprValueType_S32:  { result.s32 = abs_s64(result.s32); } break;
  case DW_ExprValueType_S64:  { result.s64 = abs_s64(result.s64); } break;
  case DW_ExprValueType_S128: { NotImplemented; } break;
  case DW_ExprValueType_S256: { NotImplemented; } break;
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32: { result.f32 = abs_f32(result.f32); } break;
  case DW_ExprValueType_F64: { result.f64 = abs_f64(result.f64); } break;

  case DW_ExprValueType_Addr: {} break;
  case DW_ExprValueType_Bool: { result.boolean = abs_s64(result.boolean); } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_neg(DW_ExprValue value)
{
  DW_ExprValue result = value;

  switch (value.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;

  case DW_ExprValueType_U8:   { result.u8 = -result.u8; }; break;
  case DW_ExprValueType_U16:  { result.u16 = -result.u16; }; break;
  case DW_ExprValueType_U32:  { result.u32 = -result.u32; }; break;
  case DW_ExprValueType_U64:  { result.u64 = -result.u64; }; break;
  case DW_ExprValueType_U128: { NotImplemented; }; break;
  case DW_ExprValueType_U256: { NotImplemented; }; break;
  case DW_ExprValueType_U512: { NotImplemented; }; break;

  case DW_ExprValueType_S8:   { result.s8  = -result.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = -result.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = -result.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = -result.s64; } break;
  case DW_ExprValueType_S128: { NotImplemented; } break;
  case DW_ExprValueType_S256: { NotImplemented; } break;
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32: { result.f32 = -result.f32; } break;
  case DW_ExprValueType_F64: { result.f64 = -result.f64; } break;

  case DW_ExprValueType_Addr: { result.addr = -result.addr; } break;
  case DW_ExprValueType_Bool: { result.boolean = abs_s64(result.boolean); } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;
  }

  return result;
}

internal DW_ExprValue
dw_expr_not(DW_ExprValue value)
{
  DW_ExprValue result = value;

  switch (value.type) {
  case DW_ExprValueType_Generic: { InvalidPath; } break;

  case DW_ExprValueType_U8:   { result.u8  = !result.u8; }; break;
  case DW_ExprValueType_U16:  { result.u16 = !result.u16; }; break;
  case DW_ExprValueType_U32:  { result.u32 = !result.u32; }; break;
  case DW_ExprValueType_U64:  { result.u64 = !result.u64; }; break;
  case DW_ExprValueType_U128: { NotImplemented; }; break;
  case DW_ExprValueType_U256: { NotImplemented; }; break;
  case DW_ExprValueType_U512: { NotImplemented; }; break;

  case DW_ExprValueType_S8:   { result.s8  = !result.s8;  } break;
  case DW_ExprValueType_S16:  { result.s16 = !result.s16; } break;
  case DW_ExprValueType_S32:  { result.s32 = !result.s32; } break;
  case DW_ExprValueType_S64:  { result.s64 = !result.s64; } break;
  case DW_ExprValueType_S128: { NotImplemented; } break;
  case DW_ExprValueType_S256: { NotImplemented; } break;
  case DW_ExprValueType_S512: { NotImplemented; } break;

  case DW_ExprValueType_F32: { result.f32 = !result.f32; } break;
  case DW_ExprValueType_F64: { result.f64 = !result.f64; } break;

  case DW_ExprValueType_Addr: { result.addr = !result.addr; } break;
  case DW_ExprValueType_Bool: { result.boolean = abs_s64(result.boolean); } break;
  case DW_ExprValueType_Implicit: { InvalidPath; } break;
  }

  return result;
}

internal String8
dw_string_from_expr_value(Arena *arena, U64 addr_size, DW_ExprValue v)
{
  String8 s = {0};
  switch (v.type) {
  case DW_ExprValueType_Generic:  { } break;
  case DW_ExprValueType_U8:       { s = str8_struct(&v.u8);             } break;
  case DW_ExprValueType_U16:      { s = str8_struct(&v.u16);            } break;
  case DW_ExprValueType_U32:      { s = str8_struct(&v.u32);            } break;
  case DW_ExprValueType_U64:      { s = str8_struct(&v.u64);            } break;
  case DW_ExprValueType_U128:     { s = str8_struct(&v.u128);           } break;
  case DW_ExprValueType_U256:     { s = str8_struct(&v.u256);           } break;
  case DW_ExprValueType_U512:     { s = str8_struct(&v.u512);           } break;
  case DW_ExprValueType_S8:       { s = str8_struct(&v.s8);             } break;
  case DW_ExprValueType_S16:      { s = str8_struct(&v.s16);            } break;
  case DW_ExprValueType_S32:      { s = str8_struct(&v.s32);            } break;
  case DW_ExprValueType_S64:      { s = str8_struct(&v.s64);            } break;
  case DW_ExprValueType_F32:      { s = str8_struct(&v.f32);            } break;
  case DW_ExprValueType_F64:      { s = str8_struct(&v.f64);            } break;
  case DW_ExprValueType_Addr:     { s = str8((U8 *)&v.addr, addr_size); } break;
  case DW_ExprValueType_Implicit: { s = v.implicit;                     } break;
  default: { NotImplemented; } break;
  }
  return str8_copy(arena, s);
}

internal DW_ExprValueNode *
dw_expr_stack_push(Arena *arena, DW_ExprStack *stack, DW_ExprValue value)
{
  DW_ExprValueNode *n = push_array(arena, DW_ExprValueNode, 1);
  n->v = value;
  SLLStackPush(stack->top, n);
  stack->count += 1;
  return n;
}

internal DW_ExprValueNode *
dw_expr_stack_push_unsigned(Arena *arena, DW_ExprStack *stack, void *value, U64 value_size)
{
  DW_ExprValueNode *n = 0;
  switch (value_size) {
  case 0: {} break;
  case 1: { n = dw_expr_stack_push(arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U8,  .u8  = *(U8 *)value  }); } break;
  case 2: { n = dw_expr_stack_push(arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U16, .u16 = *(U16 *)value }); } break;
  case 4: { n = dw_expr_stack_push(arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U32, .u32 = *(U32 *)value }); } break;
  case 8: { n = dw_expr_stack_push(arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U64, .u64 = *(U64 *)value }); } break;
  default: { NotImplemented; } break;
  }
  return n;
}

internal DW_ExprValue
dw_expr_stack_pop(DW_ExprStack *stack)
{
  DW_ExprValueNode *n = stack->top;
  DW_ExprValue v = n->v;
  SLLStackPop(stack->top);
  return v;
}

internal DW_ExprValue
dw_expr_stack_peek(DW_ExprStack *stack)
{
  return stack->top->v;
}

internal DW_ExprValueNode *
dw_expr_stack_pick(DW_ExprStack *stack, U64 idx)
{
  DW_ExprValueNode *n = 0;
  if (idx < stack->count) {
    U64 c = idx;
    for (n = stack->top; n != 0 && c > 0; n = n->next, c -= 1);
  }
  return n;
}

internal DW_ExprInst *
dw_expr_inst_from_delta(DW_ExprInst *inst, S16 delta)
{
  B32          skip_fwd = inst->operands[0].s16 >= 0;
  DW_ExprInst *i        = inst;
  U16          u_delta  = abs_s64(inst->operands[0].s16);
  U64          cursor   = 0;
  for (i = skip_fwd ? inst : inst->prev; i != 0 && cursor < u_delta; i = skip_fwd ? inst->next : inst->prev) {
    cursor += inst->size;
  }
  if (cursor != u_delta) {
    i = 0;
  }
  return i;
}

internal DW_UnwindStatus
dw_eval_expr(Arena *arena, DW_ExprContext *ctx, DW_Expr expr, DW_RegRead *reg_read, void *reg_read_ud, DW_ExprValue *value_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  DW_UnwindStatus result = DW_UnwindStatus_Ok;
  DW_PieceList pieces = {0};
  DW_ExprStack *stack = push_array(scratch.arena, DW_ExprStack, 1);
  B32 is_ok = 1;

  for EachNode(inst, DW_ExprInst, expr.first) {
    again:;

    U64 pop_count = dw_pop_count_from_expr_op(inst->opcode);
    if (pop_count >= stack->count) {
      Assert(0 && "not enough values on the stack to evaluate the instruction");
      is_ok = 0;
      break;
    }

    switch (inst->opcode) {
    case DW_ExprOp_Nop: {} break;

    case DW_ExprOp_Lit0:  case DW_ExprOp_Lit1:  case DW_ExprOp_Lit2:
    case DW_ExprOp_Lit3:  case DW_ExprOp_Lit4:  case DW_ExprOp_Lit5:
    case DW_ExprOp_Lit6:  case DW_ExprOp_Lit7:  case DW_ExprOp_Lit8:
    case DW_ExprOp_Lit9:  case DW_ExprOp_Lit10: case DW_ExprOp_Lit11:
    case DW_ExprOp_Lit12: case DW_ExprOp_Lit13: case DW_ExprOp_Lit14:
    case DW_ExprOp_Lit15: case DW_ExprOp_Lit16: case DW_ExprOp_Lit17:
    case DW_ExprOp_Lit18: case DW_ExprOp_Lit19: case DW_ExprOp_Lit20:
    case DW_ExprOp_Lit21: case DW_ExprOp_Lit22: case DW_ExprOp_Lit23:
    case DW_ExprOp_Lit24: case DW_ExprOp_Lit25: case DW_ExprOp_Lit26:
    case DW_ExprOp_Lit27: case DW_ExprOp_Lit28: case DW_ExprOp_Lit29:
    case DW_ExprOp_Lit30: case DW_ExprOp_Lit31: {
      dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U8, .u8 = inst->opcode - DW_ExprOp_Lit0 });
    } break;

    case DW_ExprOp_Const1U: { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U8,   .u8   = inst->operands[0].u8  }); } break;
    case DW_ExprOp_Const2U: { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U16,  .u16  = inst->operands[0].u16 }); } break;
    case DW_ExprOp_Const4U: { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U32,  .u32  = inst->operands[0].u32 }); } break;
    case DW_ExprOp_Const8U: { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U64,  .u64  = inst->operands[0].u64 }); } break;
    case DW_ExprOp_Const1S: { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_S8,   .s8   = inst->operands[0].s8  }); } break;
    case DW_ExprOp_Const2S: { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_S16,  .s16  = inst->operands[0].s16 }); } break;
    case DW_ExprOp_Const4S: { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_S32,  .s32  = inst->operands[0].s32 }); } break;
    case DW_ExprOp_Const8S: { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_S64,  .s64  = inst->operands[0].s64 }); } break;
    case DW_ExprOp_ConstU:  { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U64,  .u64  = inst->operands[0].u64 }); } break;
    case DW_ExprOp_ConstS:  { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_S64,  .s64  = inst->operands[0].s64 }); } break;
    case DW_ExprOp_Addr:    { dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_Addr, .addr = inst->operands[0].u64 }); } break;

    case DW_ExprOp_Reg0:  case DW_ExprOp_Reg1:  case DW_ExprOp_Reg2:
    case DW_ExprOp_Reg3:  case DW_ExprOp_Reg4:  case DW_ExprOp_Reg5:
    case DW_ExprOp_Reg6:  case DW_ExprOp_Reg7:  case DW_ExprOp_Reg8:
    case DW_ExprOp_Reg9:  case DW_ExprOp_Reg10: case DW_ExprOp_Reg11:
    case DW_ExprOp_Reg12: case DW_ExprOp_Reg13: case DW_ExprOp_Reg14:
    case DW_ExprOp_Reg15: case DW_ExprOp_Reg16: case DW_ExprOp_Reg17:
    case DW_ExprOp_Reg18: case DW_ExprOp_Reg19: case DW_ExprOp_Reg20:
    case DW_ExprOp_Reg21: case DW_ExprOp_Reg22: case DW_ExprOp_Reg23:
    case DW_ExprOp_Reg24: case DW_ExprOp_Reg25: case DW_ExprOp_Reg26:
    case DW_ExprOp_Reg27: case DW_ExprOp_Reg28: case DW_ExprOp_Reg29:
    case DW_ExprOp_Reg30: case DW_ExprOp_Reg31: {
      DW_Reg  reg_id    = inst->opcode - DW_ExprOp_Reg0;
      U64     reg_size  = dw_reg_size_from_code(ctx->arch, reg_id);
      U8     *reg_value = push_array(scratch.arena, U8, reg_size);
      result = reg_read(reg_id, reg_value, reg_size, reg_read_ud);
      if (result != DW_UnwindStatus_Ok) { goto exit; }
      dw_expr_stack_push_unsigned(scratch.arena, stack, reg_value, reg_size);
    } break;

    case DW_ExprOp_RegX: {
      U64 reg_size  = dw_reg_size_from_code(ctx->arch, inst->operands[0].u64);
      U8 *reg_value = push_array(scratch.arena, U8, reg_size);
      result = reg_read(inst->operands[0].u64, reg_value, reg_size, reg_read_ud);
      if (result != DW_UnwindStatus_Ok) { goto exit; }
      dw_expr_stack_push_unsigned(scratch.arena, stack, reg_value, reg_size);
    } break;

    case DW_ExprOp_ImplicitValue: {
      if (inst->operands[0].block.size <= sizeof(U512)) {
        dw_expr_stack_push_unsigned(scratch.arena, stack, inst->operands[0].block.str, inst->operands[0].block.size);
      } else {
        NotImplemented;
      }
    } break;

    case DW_ExprOp_Piece: {
      if (stack->count) {
        String8 value = dw_string_from_expr_value(arena, ctx->arch, dw_expr_stack_pop(stack));
        if (inst->operands[0].u64 <= value.size) {
          dw_piece_list_push(arena, &pieces, (DW_Piece){ .kind = DW_PieceKind_Value, .value = { .ptr = value.str, .bit_size = inst->operands[0].u64 * 8 }});
        } else {
          Assert(0 && "out of bounds size in the piece opcode");
          result = DW_UnwindStatus_Fail;
        }
      } else {
        dw_piece_list_push(arena, &pieces, (DW_Piece){ .kind = DW_PieceKind_Undefined, .undef_bit_size = inst->operands[0].u64 * 8 });
      }
    } break;
    case DW_ExprOp_BitPiece: {
      if (stack->count) {
        String8 value = dw_string_from_expr_value(arena, ctx->arch, dw_expr_stack_pop(stack));
        if (inst->operands[0].u64 <= value.size * 8) {
          dw_piece_list_push(arena, &pieces, (DW_Piece){ .kind = DW_PieceKind_Value, .value = { .ptr = value.str, .bit_size = inst->operands[0].u64 }});
        } else {
          Assert(0 && "out of bounds size in the bit piece opcode");
          result = DW_UnwindStatus_Fail;
        }
      } else {
        dw_piece_list_push(arena, &pieces, (DW_Piece){ .kind = DW_PieceKind_Undefined, .undef_bit_size = inst->operands[0].u64 });
      }
    } break;

    case DW_ExprOp_Pick: {
      DW_ExprValueNode *src = dw_expr_stack_pick(stack, inst->operands[0].u8);
      if (src) {
        dw_expr_stack_push(scratch.arena, stack, src->v);
      } else {
        Assert(0 && "out of bounds stack index");
        result = DW_UnwindStatus_Fail;
      }
    } break;

    case DW_ExprOp_Over: {
      DW_ExprValueNode *src = dw_expr_stack_pick(stack, 1);
      if (src) {
        dw_expr_stack_push(scratch.arena, stack, src->v);
      } else {
        Assert(0 && "out of bounds stack index");
        result = DW_UnwindStatus_Fail;
      }
    } break;

    case DW_ExprOp_PlusUConst: {
      DW_ExprValue lhs = dw_expr_stack_pop(stack);
      DW_ExprValue rhs = { .type = DW_ExprValueType_U64, .u64 = inst->operands[0].u64 };
      DW_ExprValue sum = dw_expr_add(lhs, rhs);
      dw_expr_stack_push(scratch.arena, stack, sum);
    } break;

    case DW_ExprOp_Skip: {
      DW_ExprInst *i = dw_expr_inst_from_delta(inst, inst->operands[0].s16);

      if (i == 0) {
        Assert(0 && "seeking to an invalid offset");
        result = DW_UnwindStatus_Fail;
        break;
      }

      inst = i;
      goto again;
    }

    case DW_ExprOp_Bra: {
      DW_ExprValue cond = dw_expr_stack_pop(stack);

      if (cond.type != DW_ExprValueType_S16) {
        Assert(0 && "unexpected value");
        result = DW_UnwindStatus_Fail;
        break;
      }

      DW_ExprInst *i = dw_expr_inst_from_delta(inst, inst->operands[0].s16);

      if (i == 0) {
        Assert(0 && "seeking to an invalid offset");
        result = DW_UnwindStatus_Fail;
        break;
      }

      inst = i;
      goto again;
    }

    case DW_ExprOp_Call2:
    case DW_ExprOp_Call4:
    case DW_ExprOp_CallRef: {
      NotImplemented;
    } break;

    case DW_ExprOp_GNU_Convert:
    case DW_ExprOp_Convert: {
      if (inst->operands[0].u64 == 0) {
        DW_ExprValue value     = dw_expr_stack_pop(stack);
        DW_ExprValue new_value = dw_expr_cast(value, DW_ExprValueType_Generic);
        dw_expr_stack_push(scratch.arena, stack, new_value);
      } else {
        NotImplemented;
      }
    } break;

    case DW_ExprOp_Reinterpret: {
      if (inst->operands[0].u64 == 0) {
        DW_ExprValue value     = dw_expr_stack_pop(stack);
        DW_ExprValue new_value = dw_expr_cast(value, DW_ExprValueType_Generic);
        dw_expr_stack_push(scratch.arena, stack, new_value);
      } else {
        NotImplemented;
      }
    } break;

    case DW_ExprOp_GNU_ParameterRef: { NotImplemented; } break;
    case DW_ExprOp_GNU_DerefType:    { NotImplemented; } break;
    case DW_ExprOp_DerefType:        { NotImplemented; } break;
    case DW_ExprOp_ConstType:        { NotImplemented; } break;
    case DW_ExprOp_GNU_ConstType:    { NotImplemented; } break;
    case DW_ExprOp_RegvalType:       { NotImplemented; } break;

    case DW_ExprOp_EntryValue:
    case DW_ExprOp_GNU_EntryValue: {
      DW_Expr      entry_value_expr = dw_expr_from_data(scratch.arena, ctx->format, byte_size_from_arch(ctx->arch), inst->operands[0].block);
      DW_ExprValue entry_value;
      result = dw_eval_expr(scratch.arena, ctx, entry_value_expr, reg_read, reg_read_ud, &entry_value);
      if (result == DW_UnwindStatus_Ok) {
        dw_expr_stack_push(scratch.arena, stack, entry_value);
      }
    } break;

    case DW_ExprOp_Addrx: { NotImplemented; } break;

    case DW_ExprOp_CallFrameCfa: {
      dw_expr_stack_push(scratch.arena, stack, (DW_ExprValue){ .type = DW_ExprValueType_U64, .u64 = ctx->cfa });
    } break;

    case DW_ExprOp_PushObjectAddress: { NotImplemented; } break;

    case DW_ExprOp_Plus:  { dw_expr_stack_push(scratch.arena, stack, dw_expr_add(dw_expr_stack_pop(stack),   dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Minus: { dw_expr_stack_push(scratch.arena, stack, dw_expr_minus(dw_expr_stack_pop(stack), dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Div:   { dw_expr_stack_push(scratch.arena, stack, dw_expr_div(dw_expr_stack_pop(stack),   dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Mul:   { dw_expr_stack_push(scratch.arena, stack, dw_expr_mul(dw_expr_stack_pop(stack),   dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Mod:   { dw_expr_stack_push(scratch.arena, stack, dw_expr_mod(dw_expr_stack_pop(stack),   dw_expr_stack_pop(stack))); } break;

    case DW_ExprOp_Eq:    { dw_expr_stack_push(scratch.arena, stack, dw_expr_eq(dw_expr_stack_pop(stack),    dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Ge:    { dw_expr_stack_push(scratch.arena, stack, dw_expr_ge(dw_expr_stack_pop(stack),    dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Gt:    { dw_expr_stack_push(scratch.arena, stack, dw_expr_gt(dw_expr_stack_pop(stack),    dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Le:    { dw_expr_stack_push(scratch.arena, stack, dw_expr_le(dw_expr_stack_pop(stack),    dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Lt:    { dw_expr_stack_push(scratch.arena, stack, dw_expr_lt(dw_expr_stack_pop(stack),    dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Ne:    { dw_expr_stack_push(scratch.arena, stack, dw_expr_ne(dw_expr_stack_pop(stack),    dw_expr_stack_pop(stack))); } break;

    case DW_ExprOp_Xor:   { dw_expr_stack_push(scratch.arena, stack, dw_expr_xor(dw_expr_stack_pop(stack),   dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_And:   { dw_expr_stack_push(scratch.arena, stack, dw_expr_and(dw_expr_stack_pop(stack),   dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Or:    { dw_expr_stack_push(scratch.arena, stack, dw_expr_or(dw_expr_stack_pop(stack),    dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Shl:   { dw_expr_stack_push(scratch.arena, stack, dw_expr_shl(dw_expr_stack_pop(stack),   dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Shr:   { dw_expr_stack_push(scratch.arena, stack, dw_expr_shr(dw_expr_stack_pop(stack),   dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Shra:  { dw_expr_stack_push(scratch.arena, stack, dw_expr_shra(dw_expr_stack_pop(stack),  dw_expr_stack_pop(stack))); } break;

    case DW_ExprOp_Abs: { dw_expr_stack_push(scratch.arena, stack, dw_expr_abs(dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Neg: { dw_expr_stack_push(scratch.arena, stack, dw_expr_neg(dw_expr_stack_pop(stack))); } break;
    case DW_ExprOp_Not: { dw_expr_stack_push(scratch.arena, stack, dw_expr_not(dw_expr_stack_pop(stack))); } break;

    case DW_ExprOp_Dup: {
      dw_expr_stack_push(scratch.arena, stack, dw_expr_stack_peek(stack));
    } break;

    case DW_ExprOp_Rot: {
      DW_ExprValue first  = dw_expr_stack_pop(stack);
      DW_ExprValue second = dw_expr_stack_pop(stack);
      DW_ExprValue third  = dw_expr_stack_pop(stack);
      dw_expr_stack_push(scratch.arena, stack, first);  // -> third
      dw_expr_stack_push(scratch.arena, stack, third);  // -> second
      dw_expr_stack_push(scratch.arena, stack, second); // -> first
    } break;

    case DW_ExprOp_Swap: {
      DW_ExprValue first  = dw_expr_stack_pop(stack);
      DW_ExprValue second = dw_expr_stack_pop(stack);
      dw_expr_stack_push(scratch.arena, stack, first);  // -> second
      dw_expr_stack_push(scratch.arena, stack, second); // -> first
    } break;

    case DW_ExprOp_Drop: {
      dw_expr_stack_pop(stack);
    } break;

    case DW_ExprOp_StackValue: {
      stack->top->v.type = dw_expr_unsigned_value_type_from_bit_size(byte_size_from_arch(ctx->arch) * 8);
    } break;

    case DW_ExprOp_GNU_PushTlsAddress:
    case DW_ExprOp_FormTlsAddress: {
      DW_ExprValue tls_off  = dw_expr_stack_pop(stack);
      DW_ExprValue tls_base = { .type = DW_ExprValueType_Addr, .addr = ctx->tls_base };
      DW_ExprValue tls_addr = dw_expr_add(tls_base, tls_off);
      dw_expr_stack_push(scratch.arena, stack, tls_addr);
    } break;
    }
  }

exit:;
  scratch_end(scratch);
  return result;
}

internal String8
dw_encode_expr(Arena *arena, DW_Format format, U64 addr_size, DW_ExprEnc *encs, U64 encs_count)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List *srl = push_array(scratch.arena, String8List, 1);
  str8_serial_begin(scratch.arena, srl);

  for EachIndex(i, encs_count) {
    DW_ExprEnc *e = &encs[i];

    switch (e->type) {
    case DW_ExprEncType_Op: {
      str8_serial_push_struct(scratch.arena, srl, &e->op);
    } break;
    case DW_ExprEncType_U8: {
      str8_serial_push_struct(scratch.arena, srl, &e->u8);
    } break;
    case DW_ExprEncType_U16: {
      str8_serial_push_struct(scratch.arena, srl, &e->u16);
    } break;
    case DW_ExprEncType_U32: {
      str8_serial_push_struct(scratch.arena, srl, &e->u32);
    } break;
    case DW_ExprEncType_U64: {
      str8_serial_push_struct(scratch.arena, srl, &e->u64);
    } break;
    case DW_ExprEncType_S8: {
      str8_serial_push_struct(scratch.arena, srl, &e->s8);
    } break;
    case DW_ExprEncType_S16: {
      str8_serial_push_struct(scratch.arena, srl, &e->s16);
    } break;
    case DW_ExprEncType_S32: {
      str8_serial_push_struct(scratch.arena, srl, &e->s32);
    } break;
    case DW_ExprEncType_S64: {
      str8_serial_push_struct(scratch.arena, srl, &e->s64);
    } break;
    case DW_ExprEncType_ULEB128: {
      U64 buffer_size = 0;
      U8  buffer[10];
      for (U64 value = e->u64; value != 0; ) {
        U8 byte = value & 0x7f;
        value >>= 7;
        if (value != 0) {
          byte |= 0x80;
        }
        Assert(buffer_size < sizeof(buffer));
        buffer[buffer_size++] = byte;
      }
      str8_serial_push_string(scratch.arena, srl, str8(buffer, buffer_size));
    } break;
    case DW_ExprEncType_SLEB128: {
      U64 buffer_size = 0;
      U8  buffer[10];
      for (S64 value = e->s64, more = 1; more != 0; ) {
        U8 byte = value & 0x7f;
        value >>= 7;
        U8 sign_bit = byte & 0x40;
        if ((value == 0 && sign_bit == 0) || (value == -1 && sign_bit != 0)) {
          more = 0;
        } else {
          byte |= 0x80;
        }
        Assert(buffer_size < sizeof(buffer));
        buffer[buffer_size++] = byte;
      }
      str8_serial_push_string(scratch.arena, srl, str8(buffer, buffer_size));
    } break;
    case DW_ExprEncType_Addr: {
      Assert(addr_size <= sizeof(e->addr));
      str8_serial_push_string(scratch.arena, srl, str8((U8 *)&e->addr, addr_size));
    } break;
    case DW_ExprEncType_Block: {
      Assert(e->block.size <= max_U8);
      str8_serial_push_u8(scratch.arena, srl, (U8)e->block.size);
      str8_serial_push_string(scratch.arena, srl, e->block);
    } break;
    case DW_ExprEncType_DwarfUInt: {
      switch (format) {
      case DW_Format_Null:  {} break;
      case DW_Format_32Bit: { str8_serial_push_u32(scratch.arena, srl, e->u32); } break;
      case DW_Format_64Bit: { str8_serial_push_u64(scratch.arena, srl, e->u64); } break;
      default: { InvalidPath; } break;
      }
    } break;
    case DW_ExprEncType_Label: {
      // TODO:
    } break;
    case DW_ExprEncType_DeclLabel: {
      // TODO:
    } break;
    default: { InvalidPath; } break;
    }
  }

  String8 expr = str8_serial_end(arena, srl);
  scratch_end(scratch);
  return expr;
}

