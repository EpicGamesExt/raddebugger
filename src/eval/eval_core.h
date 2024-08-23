// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_CORE_H
#define EVAL_CORE_H

////////////////////////////////
//~ rjf: Messages

typedef enum E_MsgKind
{
  E_MsgKind_Null,
  E_MsgKind_MalformedInput,
  E_MsgKind_MissingInfo,
  E_MsgKind_ResolutionFailure,
  E_MsgKind_InterpretationError,
  E_MsgKind_COUNT
}
E_MsgKind;

typedef struct E_Msg E_Msg;
struct E_Msg
{
  E_Msg *next;
  E_MsgKind kind;
  void *location;
  String8 text;
};

typedef struct E_MsgList E_MsgList;
struct E_MsgList
{
  E_Msg *first;
  E_Msg *last;
  E_MsgKind max_kind;
  U64 count;
};

////////////////////////////////
//~ rjf: Register-Sized Value Type

typedef union E_Value E_Value;
union E_Value
{
  U64 u512[8];
  U64 u256[4];
  U128 u128;
  U64 u64;
  U32 u32;
  S64 s64;
  F64 f64;
  F32 f32;
};

////////////////////////////////
//~ rjf: Operator Info

typedef enum E_OpKind
{
  E_OpKind_Null,
  E_OpKind_UnaryPrefix,
  E_OpKind_Binary,
}
E_OpKind;

typedef struct E_OpInfo E_OpInfo;
struct E_OpInfo
{
  E_OpKind kind;
  S64 precedence;
  String8 pre;
  String8 sep;
  String8 post;
};

////////////////////////////////
//~ rjf: Evaluation Spaces

typedef U128 E_Space;
//
// NOTE(rjf): Evaluations occur within the context of a "space". Each "space"
// refers to a different offset/address-space, but it's a bit looser of a
// concept than just address space, since it can also refer to offsets into
// a register block, and it is also used to refer to spaces of unique IDs for
// key-value stores, e.g. for information in the debugger.
//
// Effectively, when considering the result of an evaluation, you use the
// value for understanding a key *into* a space, e.g. 1+2 -> 3, in a null
// space, or &foo, in the space of PID: 1234.

////////////////////////////////
//~ rjf: Evaluation Modes

typedef enum E_Mode
{
  E_Mode_Null,
  E_Mode_Value,
  E_Mode_Offset,
}
E_Mode;

////////////////////////////////
//~ rjf: Modules

typedef struct E_Module E_Module;
struct E_Module
{
  RDI_Parsed *rdi;
  Rng1U64 vaddr_range;
  Architecture arch;
  E_Space space;
};

////////////////////////////////
//~ rjf: Generated Code

#include "eval/generated/eval.meta.h"

////////////////////////////////
//~ rjf: Basic Helper Functions

internal U64 e_hash_from_string(U64 seed, String8 string);
internal String8 e_raw_from_escaped_string(Arena *arena, String8 string);
#define e_value_u64(v) (E_Value){.u64 = (v)}

////////////////////////////////
//~ rjf: Message Functions

internal void e_msg(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, String8 text);
internal void e_msgf(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, char *fmt, ...);
internal void e_msg_list_concat_in_place(E_MsgList *dst, E_MsgList *to_push);

#endif // EVAL_CORE_H
