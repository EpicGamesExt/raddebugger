// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_COMMON_H
#define DEMON_COMMON_H

////////////////////////////////
//~ allen: DEMON Entity System

typedef enum DEMON_EntityKind
{
  DEMON_EntityKind_NULL,
  
  DEMON_EntityKind_Root,
  DEMON_EntityKind_Process,
  DEMON_EntityKind_Thread,
  DEMON_EntityKind_Module,
  
  DEMON_EntityKind_COUNT
}
DEMON_EntityKind;

typedef struct DEMON_Entity DEMON_Entity;
struct DEMON_Entity
{
  // TODO(allen): these could be U32s
  DEMON_Entity *next;
  DEMON_Entity *prev;
  DEMON_Entity *parent;
  DEMON_Entity *first;
  DEMON_Entity *last;
  
  U16 kind;
  U16 arch;
  U32 gen;
  U64 id;
  U64 addr_range_dim;
  
  // each OS backend decides how to use `ext` for each entity kind
  union{
    void *ext;
    U64 ext_u64;
  };
  
  // the accel layer attaches some extra information to some entities
  void *accel;
};

//- id -> entity map
typedef struct DEMON_MapSlot DEMON_MapSlot;
struct DEMON_MapSlot
{
  DEMON_MapSlot *next;
  U16 kind;
  U64 id;
  DEMON_Entity *entity;
};

typedef struct DEMON_Map DEMON_Map;
struct DEMON_Map
{
  DEMON_MapSlot **buckets;
  U64 bucket_count;
  DEMON_MapSlot *free_slots;
};

typedef struct DEMON_MapRef DEMON_MapRef;
struct DEMON_MapRef
{
  DEMON_MapSlot *slot;
  DEMON_MapSlot **ptr_to_slot;
};

//- rjf: entity extrusive list

typedef struct DEMON_EntityNode DEMON_EntityNode;
struct DEMON_EntityNode
{
  DEMON_EntityNode *next;
  DEMON_Entity *entity;
};

////////////////////////////////
//~ allen: Demon Globals

thread_static B32    demon_primary_thread = 0;
global B32           demon_run_state = 0;
global OS_Handle     demon_state_mutex = {0};

global U64           demon_time = 0;

global Arena        *demon_ent_arena = 0;
global DEMON_Map    *demon_ent_map = 0;

global DEMON_Entity *demon_ent_free = 0;
global DEMON_Entity *demon_ent_root = 0;

global DEMON_Entity *demon_ent_base = 0;
global DEMON_Entity *demon_ent_pos = 0;
global DEMON_Entity *demon_ent_opl = 0;
global void         *demon_ent_cmt = 0;

global U64           demon_proc_count = 0;
global U64           demon_thread_count = 0;
global U64           demon_module_count = 0;

#if !defined(DEMON_ENTITY_CMT_SIZE)
# define DEMON_ENTITY_CMT_SIZE KB(64)
#endif
#if !defined(DEMON_ENTITY_CAP)
# define DEMON_ENTITY_CAP 65536
#endif

StaticAssert(IsPow2(DEMON_ENTITY_CMT_SIZE), check_demon_entity_cmt_size);

////////////////////////////////
//~ allen: State Safety Helper

internal B32           demon_access_begin(void);
internal void          demon_access_end(void);

////////////////////////////////
//~ allen: Entity System

internal void          demon_common_init(void);
internal DEMON_Entity* demon_ent_alloc(void);

//- handle <-> entity pointer
internal DEMON_Entity* demon_ent_ptr_from_handle(DEMON_Handle handle);
internal DEMON_Handle  demon_ent_handle_from_ptr(DEMON_Entity *entity);

//- high level entity alloc,init,release
internal DEMON_Entity* demon_ent_new(DEMON_Entity *parent, DEMON_EntityKind kind, U64 id);
internal void          demon_ent_release_single(DEMON_Entity *entity);
internal void          demon_ent_release_children(DEMON_Entity *root);
internal void          demon_ent_release_root_and_children(DEMON_Entity *root);

//- entity map
internal U64           demon_ent_map_hash(U16 kind, U64 id);
internal void          demon_ent_map_save(U16 kind, U64 id, DEMON_Entity *entity);
internal DEMON_MapRef  demon_ent_map_find(U16 kind, U64 id);
internal DEMON_Entity* demon_ent_map_entity_from_id(U16 kind, U64 id);
internal void          demon_ent_map_erase(DEMON_MapRef map_ref);

////////////////////////////////
//~ allen: Event Helpers

internal DEMON_Event*  demon_push_event(Arena *arena, DEMON_EventList *list, DEMON_EventKind kind);

#endif //DEMON_COMMON_H
