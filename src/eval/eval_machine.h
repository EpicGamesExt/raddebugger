// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL2_MACHINE_H
#define EVAL2_MACHINE_H

////////////////////////////////
//~ allen: Eval Machine Types

typedef B32 EVAL_MemoryRead(void *u, void *out, U64 addr, U64 size);

typedef struct EVAL_Machine EVAL_Machine;
struct EVAL_Machine{
  void *u;
  Architecture arch;
  EVAL_MemoryRead *memory_read;
  void *reg_data;
  U64   reg_size;
  U64 *module_base;
  U64 *frame_base;
  U64 *tls_base;
};

typedef union EVAL_Slot EVAL_Slot;
union EVAL_Slot{
  U64 u256[4];
  U64 u128[2];
  U64 u64;
  S64 s64;
  F64 f64;
  F32 f32;
};

typedef struct EVAL_Result EVAL_Result;
struct EVAL_Result{
  EVAL_Slot value;
  B32 bad_eval;
};

////////////////////////////////
//~ allen: Eval Machine Functions

internal EVAL_Result eval_interpret(EVAL_Machine *machine, String8 bytecode);

#endif //EVAL2_MACHINE_H
