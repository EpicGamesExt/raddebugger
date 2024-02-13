// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_ACCEL_H
#define DEMON_ACCEL_H

////////////////////////////////
//~ allen: Acceleration Data

typedef struct DEMON_AccelModule DEMON_AccelModule;
struct DEMON_AccelModule
{
  DEMON_AccelModule *next;
  U64 total_size;
  U8 buf[240];
};

typedef union DEMON_AccelThread DEMON_AccelThread;
union DEMON_AccelThread
{
  DEMON_AccelThread *next;
  struct{
    B32 has_stack_base;
    B32 has_tls_root;
    U64 stack_base;
    U64 tls_root;
    
    U64 reg_cache_time;
    union{
      REGS_RegBlockX64 x64;
      REGS_RegBlockX86 x86;
    } regs;
  };
};

////////////////////////////////
//~ allen: Acceleration Globals

global DEMON_AccelModule *demon_free_module_accel = 0;
global DEMON_AccelThread *demon_free_thread_accel = 0;

////////////////////////////////
//~ allen: Acceleration Layer Functions

//- accel helpers
internal DEMON_AccelModule *demon_accel_module_alloc(void);
internal void               demon_accel_module_free(DEMON_AccelModule *module);

internal DEMON_AccelThread *demon_accel_thread_alloc(void);
internal void               demon_accel_thread_free(DEMON_AccelThread *thread);
internal DEMON_AccelThread *demon_accel_from_thread(DEMON_Entity *thread);

//- operations on demon objects
internal String8      demon_accel_full_path_from_module(Arena *arena, DEMON_Entity *module);
internal U64          demon_accel_stack_base_vaddr_from_thread(DEMON_Entity *thread);
internal U64          demon_accel_tls_root_vaddr_from_thread(DEMON_Entity *thread);

internal void*        demon_accel_read_regs(DEMON_Entity *thread);
internal void         demon_accel_write_regs(DEMON_Entity *thread, void *data);

//- entity accel free
internal void demon_accel_free(DEMON_Entity *entity);

#endif //DEMON_ACCEL_H
