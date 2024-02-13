// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// NOTE(allen): State Safety Helper

internal B32
demon_access_begin(void){
  B32 result = 0;
  if (demon_primary_thread){
    Assert(demon_run_state);
    result = 1;
  }
  else{
    os_mutex_take(demon_state_mutex);
    if (demon_run_state){
      os_mutex_drop(demon_state_mutex);
    }
    else{
      result = 1;
    }
  }
  return(result);
}

internal void
demon_access_end(void){
  if (!demon_primary_thread){
    os_mutex_drop(demon_state_mutex);
  }
}

////////////////////////////////
// NOTE(allen): Entity System

internal void
demon_common_init(void){
  // access control mechanism
  demon_state_mutex = os_mutex_alloc();
  
  // time
  demon_time = 1;
  
  // setup arena
  demon_ent_arena = arena_alloc();
  
  // setup map
  demon_ent_map = push_array(demon_ent_arena, DEMON_Map, 1);
  demon_ent_map->bucket_count = 4093;
  demon_ent_map->buckets = push_array(demon_ent_arena, DEMON_MapSlot*, demon_ent_map->bucket_count);
  
  // setup entity memory
  U64 reserve_size_unaligned = (DEMON_ENTITY_CAP)*sizeof(DEMON_Entity);
  U64 reserve_size = AlignPow2(reserve_size_unaligned, DEMON_ENTITY_CMT_SIZE);
  demon_ent_cmt = demon_ent_pos = demon_ent_base = (DEMON_Entity*)os_reserve(reserve_size);
  demon_ent_opl = demon_ent_base + (reserve_size/sizeof(DEMON_Entity));
  
  Assert(demon_ent_base != 0);
  
  // setup root
  demon_ent_root = demon_ent_alloc();
  demon_ent_root->kind = DEMON_EntityKind_Root;
}

internal DEMON_Entity*
demon_ent_alloc(void){
  DEMON_Entity *result = demon_ent_free;
  if (result != 0){
    SLLStackPop(demon_ent_free);
  }
  else{
    if (demon_ent_pos < demon_ent_opl){
      if (ensure_commit(&demon_ent_cmt, demon_ent_pos + 1, DEMON_ENTITY_CMT_SIZE)){
        result = demon_ent_pos;
        demon_ent_pos += 1;
      }
    }
  }
  if (result != 0){
    U32 gen = result->gen;
    MemoryZeroStruct(result);
    result->gen = gen;
  }
  return(result);
}

//- handle <-> entity pointer

internal DEMON_Entity*
demon_ent_ptr_from_handle(DEMON_Handle handle){
  Assert(demon_ent_base != 0);
  DEMON_Entity *result = 0;
  U32 index = (U32)(handle & 0xFFFFFFFF);
  U64 count = (U64)(demon_ent_pos - demon_ent_base);
  if (0 < index && index < count){
    DEMON_Entity *entity = demon_ent_base + index;
    U32 gen = (U32)(handle >> 32);
    if (gen == entity->gen){
      result = entity;
    }
  }
  return(result);
}

internal DEMON_Handle
demon_ent_handle_from_ptr(DEMON_Entity *entity){
  Assert(demon_ent_base != 0);
  DEMON_Handle result = {0};
  if (demon_ent_base < entity && entity < demon_ent_pos){
    U32 index = (U32)(entity - demon_ent_base);
    U64 gen = entity->gen;
    result = (gen << 32) | index;
  }
  return(result);
}

//- high level entity alloc,init,release

internal DEMON_Entity*
demon_ent_new(DEMON_Entity *parent, DEMON_EntityKind kind, U64 id){
  Assert(demon_ent_base != 0);
  DEMON_Entity *result = demon_ent_alloc();
  if (result != 0){
    result->kind = kind;
    result->id = id;
    result->arch = parent->arch;
    result->parent = parent;
    DLLPushBack(parent->first, parent->last, result);
    demon_ent_map_save(kind, id, result);
  }
  return(result);
}

internal void
demon_ent_release_single(DEMON_Entity *entity){
  switch (entity->kind){
    default:{}break;
    case DEMON_EntityKind_Process: demon_proc_count -= 1; break;
    case DEMON_EntityKind_Thread: demon_thread_count -= 1; break;
    case DEMON_EntityKind_Module: demon_module_count -= 1; break;
  }
  demon_accel_free(entity);
  demon_os_entity_cleanup(entity);
  DEMON_MapRef ref = demon_ent_map_find(entity->kind, entity->id);
  demon_ent_map_erase(ref);
  entity->gen += 1;
}

internal void
demon_ent_release_children(DEMON_Entity *root){
  Assert(demon_ent_base != 0);
  if (root->first != 0){
    for (DEMON_Entity *node = root->first;
         node != 0;
         node = node->next){
      demon_ent_release_children(node);
      demon_ent_release_single(node);
    }
    root->last->next = demon_ent_free;
    demon_ent_free = root->first;
    root->first = 0;
    root->last = 0;
  }
}

internal void
demon_ent_release_root_and_children(DEMON_Entity *root){
  Assert(demon_ent_base != 0);
  Assert(root->parent != 0);
  
  // release children
  demon_ent_release_children(root);
  
  // release root
  DEMON_Entity *parent = root->parent;
  demon_ent_release_single(root);
  DLLRemove(parent->first, parent->last, root);
  SLLStackPush(demon_ent_free, root);
}

//- entity map

internal U64
demon_ent_map_hash(U16 kind, U64 id){
  U64 result = ((U64)kind << 32) ^ id;
  return(result);
}

internal void
demon_ent_map_save(U16 kind, U64 id, DEMON_Entity *entity){
  Assert(demon_ent_base != 0);
  DEMON_Map *map = demon_ent_map;
  
  // allocate a new slot
  DEMON_MapSlot *slot = map->free_slots;
  if (slot != 0){
    SLLStackPop(map->free_slots);
  }
  else{
    slot = push_array_no_zero(demon_ent_arena, DEMON_MapSlot, 1);
  }
  
  // fill slot
  slot->kind = kind;
  slot->id = id;
  slot->entity = entity;
  
  // insert into bucket
  U64 hash = demon_ent_map_hash(kind, id);
  U64 bucket_index = hash%map->bucket_count;
  SLLStackPush(map->buckets[bucket_index], slot);
}

internal DEMON_MapRef
demon_ent_map_find(U16 kind, U64 id){
  Assert(demon_ent_base != 0);
  DEMON_Map *map = demon_ent_map;
  
  // scan bucket
  DEMON_MapRef result = {0};
  U64 hash = demon_ent_map_hash(kind, id);
  U64 bucket_index = hash%map->bucket_count;
  for (DEMON_MapSlot **ptr = &map->buckets[bucket_index], *slot = 0;
       *ptr != 0;
       ptr = &slot->next){
    slot = *ptr;
    if (slot->kind == kind && slot->id == id){
      result.slot = slot;
      result.ptr_to_slot = ptr;
      break;
    }
  }
  
  return(result);
}

internal DEMON_Entity*
demon_ent_map_entity_from_id(U16 kind, U64 id){
  DEMON_Entity *result = 0;
  DEMON_MapRef ref = demon_ent_map_find(kind, id);
  if (ref.slot != 0){
    result = ref.slot->entity;
  }
  return(result);
}

internal void
demon_ent_map_erase(DEMON_MapRef ref){
  Assert(demon_ent_base != 0);
  DEMON_Map *map = demon_ent_map;
  
  // move slot to free list
  if (ref.slot != 0){
    *ref.ptr_to_slot = ref.slot->next;
    SLLStackPush(map->free_slots, ref.slot);
  }
}

////////////////////////////////
// NOTE(allen): Event Helpers

internal DEMON_Event*
demon_push_event(Arena *arena, DEMON_EventList *list, DEMON_EventKind kind){
  DEMON_EventNode *n = push_array(arena, DEMON_EventNode, 1);
  DEMON_Event *result = &n->v;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  result->kind = kind;
  return(result);
}

