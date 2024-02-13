// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//- allen: Acceleration Layer Functions

//- accel helpers
internal DEMON_AccelModule*
demon_accel_module_alloc(void){
  DEMON_AccelModule *result = demon_free_module_accel;
  if (result != 0){
    SLLStackPop(demon_free_module_accel);
  }
  else{
    result = push_array_no_zero(demon_ent_arena, DEMON_AccelModule, 1);
  }
  MemoryZeroStruct(result);
  return(result);
}

internal void
demon_accel_module_free(DEMON_AccelModule *module){
  SLLStackPush(demon_free_module_accel, module);
}

internal DEMON_AccelThread*
demon_accel_thread_alloc(void){
  DEMON_AccelThread *result = demon_free_thread_accel;
  if (result != 0){
    SLLStackPop(demon_free_thread_accel);
  }
  else{
    result = push_array_no_zero(demon_ent_arena, DEMON_AccelThread, 1);
  }
  MemoryZeroStruct(result);
  return(result);
}

internal void
demon_accel_thread_free(DEMON_AccelThread *thread){
  SLLStackPush(demon_free_thread_accel, thread);
}

internal DEMON_AccelThread*
demon_accel_from_thread(DEMON_Entity *thread){
  DEMON_AccelThread *accel = (DEMON_AccelThread*)thread->accel;
  if (accel == 0){
    accel = demon_accel_thread_alloc();
    thread->accel = accel;
  }
  return(accel);
}

//- operations on demon objects
internal String8
demon_accel_full_path_from_module(Arena *arena, DEMON_Entity *module){
  DEMON_AccelModule *accel = (DEMON_AccelModule*)module->accel;
  
  String8 result = {0};
  
  // first time
  if (accel == 0){
    result = demon_os_full_path_from_module(arena, module);
    
    // build chain
    DEMON_AccelModule *last_accel = 0;
    
    U8 *ptr = result.str;
    U8 *opl = result.str + result.size;
    for (;ptr < opl;){
      U64 size = (U64)(ptr - opl);
      U64 clamped_size = ClampTop(result.size, sizeof(Member(DEMON_AccelModule, buf)));
      
      DEMON_AccelModule *node = demon_accel_module_alloc();
      SLLQueuePush(accel, last_accel, node);
      node->total_size = result.size;
      MemoryCopy(node->buf, ptr, clamped_size);
      
      ptr += clamped_size;
    }
    
    // store in module
    module->accel = accel;
  }
  
  // read from accel
  else{
    U64 size = accel->total_size;
    U8 *str = push_array_no_zero(arena, U8, size + 1);
    
    // copy chain contents to buffer
    U8 *ptr = str;
    for (DEMON_AccelModule *node = accel;
         node != 0;
         node = node->next){
      U64 total_size = node->total_size;
      U64 clamped_size = ClampTop(total_size, sizeof(node->buf));
      MemoryCopy(ptr, node->buf, clamped_size);
      ptr += clamped_size;
    }
    *ptr = 0;
    
    // fill result
    result.str = str;
    result.size = size;
  }
  
  return(result);
}

internal U64
demon_accel_stack_base_vaddr_from_thread(DEMON_Entity *thread){
  // get accel data
  DEMON_AccelThread *accel = demon_accel_from_thread(thread);
  
  // fill stack base
  if (!accel->has_stack_base){
    accel->has_stack_base = 1;
    accel->stack_base = demon_os_stack_base_vaddr_from_thread(thread);
  }
  
  return(accel->stack_base);
}

internal U64
demon_accel_tls_root_vaddr_from_thread(DEMON_Entity *thread){
  // get accel data
  DEMON_AccelThread *accel = demon_accel_from_thread(thread);
  
  // fill tls root
  if (!accel->has_tls_root){
    accel->has_tls_root = 1;
    accel->tls_root = demon_os_tls_root_vaddr_from_thread(thread);
  }
  
  return(accel->tls_root);
}

internal void*
demon_accel_read_regs(DEMON_Entity *thread){
  // get accel data
  DEMON_AccelThread *accel = demon_accel_from_thread(thread);
  
  // update reg cache
  if (accel->reg_cache_time != demon_time){
    accel->reg_cache_time = demon_time;
    B32 success = demon_os_read_regs(thread, &accel->regs);
    if (!success){
      MemoryZeroStruct(&accel->regs);
    }
  }
  
  return(&accel->regs);
}

internal void
demon_accel_write_regs(DEMON_Entity *thread, void *data){
  // get accel data
  DEMON_AccelThread *accel = demon_accel_from_thread(thread);
  
  // write
  U64 data_size = regs_block_size_from_architecture(thread->arch);
  B32 success = demon_os_write_regs(thread, data);
  
  // update cache
  if(success)
  {
    accel->reg_cache_time = demon_time;
    MemoryCopy(&accel->regs, data, data_size);
  }
}

//- entity accel free
internal void
demon_accel_free(DEMON_Entity *entity){
  switch (entity->kind){
    default:{}break;
    
    case DEMON_EntityKind_Module:
    {
      if (entity->accel != 0){
        for (DEMON_AccelModule *node = (DEMON_AccelModule*)entity->accel, *next = 0;
             node != 0;
             node = next){
          next = node->next;
          demon_accel_module_free(node);
        }
      }
    }break;
    
    case DEMON_EntityKind_Thread:
    {
      if (entity->accel != 0){
        demon_accel_thread_free((DEMON_AccelThread*)entity->accel);
      }
    }break;
  }
}
