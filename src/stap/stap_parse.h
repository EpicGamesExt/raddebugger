// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef STAP_PARSE_H
#define STAP_PARSE_H

typedef enum STAP_ArgType
{
  STAP_ArgType_Null,
  STAP_ArgType_Imm,
  STAP_ArgType_Reg,
  STAP_ArgType_MemoryRef,
} STAP_ArgType;

typedef enum STAP_ArgValueType
{
  STAP_ArgValueType_Null,
  STAP_ArgValueType_U,
  STAP_ArgValueType_S,
  STAP_ArgValueType_F,
} STAP_ArgValueType;

typedef struct STAP_Arg
{
  STAP_ArgValueType value_type;
  U64               value_size;
  STAP_ArgType      type;
  union {
    U64 imm;
    struct {
      U32 reg_code;
      B8 is_alias;
    } reg;
    struct {
      S64        disp;
      struct {
        U32 reg_code;
        B8 is_alias;
      } base;
      struct {
        U32 reg_code;
        B8 is_alias;
      } index;
      U64        scale;
    } memory_ref;
  };
} STAP_Arg;

typedef struct STAP_ArgArray
{
  U64       count;
  STAP_Arg *v;
} STAP_ArgArray;

typedef struct STAP_ArgValue
{
  STAP_ArgValueType  value_type;
  U8                 value_size;
  void              *raw_ptr;
} STAP_ArgValue;

typedef struct STAP_ArgValueArray
{
  U64            count;
  STAP_ArgValue *v;
} STAP_ArgValueArray;

#define STAP_MEMORY_READ(name) B32 name(U64 addr, void *buffer, U64 read_size, void *raw_ctx)
typedef STAP_MEMORY_READ(STAP_MemoryRead);

////////////////////////////////

internal String8 stap_parse_args_x64(String8 string, STAP_Arg *arg_out);
internal STAP_ArgArray stap_arg_array_from_string(Arena *arena, Arch arch, String8 string);

internal B32 stap_read_arg(STAP_Arg arg, Arch arch, void *reg_block, STAP_MemoryRead *memory_read, void *memory_read_ctx, void *raw_value);
internal B32 stap_read_arg_u(STAP_Arg arg, Arch arch, void *reg_block, STAP_MemoryRead *memory_read, void *memory_read_ctx, U64 *u_out);
internal B32 stap_read_arg_s(STAP_Arg arg, Arch arch, void *reg_block, STAP_MemoryRead *memory_read, void *memory_read_ctx, S64 *s_out);
internal B32 stap_read_arg_f(STAP_Arg arg, Arch arch, void *reg_block, STAP_MemoryRead *memory_read, void *memory_read_ctx, F64 *f_out);

#endif // STAP_PARSE_H

