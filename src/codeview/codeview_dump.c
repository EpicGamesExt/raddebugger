// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
cv_string_from_numeric(Arena *arena, CV_NumericParsed num)
{
  String8 result = str8_zero();
  switch (num.kind) {
    case CV_NumericKind_FLOAT16:   NotImplemented; break; // TODO: format float16
    case CV_NumericKind_FLOAT32:   result = push_str8f(arena, "%f", (F64)(*(F32*)num.val)); break;
    case CV_NumericKind_FLOAT48:   NotImplemented; break; // TODO: format float48
    case CV_NumericKind_FLOAT64:   result = push_str8f(arena, "%f", *(F64*)num.val); break;
    case CV_NumericKind_FLOAT80:   NotImplemented; break; // TODO: format float80
    case CV_NumericKind_FLOAT128:  NotImplemented; break; // TODO: format float128
    case CV_NumericKind_CHAR:      result = push_str8f(arena, "%d",   *(S8 *)num.val); break;
    case CV_NumericKind_SHORT:     result = push_str8f(arena, "%d",   *(S16*)num.val); break;
    case CV_NumericKind_LONG:      result = push_str8f(arena, "%d",   *(S32*)num.val); break;
    case CV_NumericKind_QUADWORD:  result = push_str8f(arena, "%lld", *(S64*)num.val); break;
    case CV_NumericKind_USHORT:    result = push_str8f(arena, "%u",   *(U16*)num.val); break;
    case CV_NumericKind_ULONG:     result = push_str8f(arena, "%u",   *(U32*)num.val); break;
    case CV_NumericKind_UQUADWORD: result = push_str8f(arena, "%llu", *(U64*)num.val); break;
  }
  return result;
}
