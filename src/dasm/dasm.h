// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DASM_H
#define DASM_H

////////////////////////////////
//~ rjf: Handle Type

typedef struct DASM_Handle DASM_Handle;
struct DASM_Handle
{
  U64 u64[2];
};

////////////////////////////////
//~ rjf: Instruction Types

typedef struct DASM_Inst DASM_Inst;
struct DASM_Inst
{
  String8 string;
  U64 off;
  U64 addr;
};

typedef struct DASM_InstChunkNode DASM_InstChunkNode;
struct DASM_InstChunkNode
{
  DASM_InstChunkNode *next;
  DASM_Inst *v;
  U64 cap;
  U64 count;
};

typedef struct DASM_InstChunkList DASM_InstChunkList;
struct DASM_InstChunkList
{
  DASM_InstChunkNode *first;
  DASM_InstChunkNode *last;
  U64 node_count;
  U64 inst_count;
};

typedef struct DASM_InstArray DASM_InstArray;
struct DASM_InstArray
{
  DASM_Inst *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Striped Access Types

typedef struct DASM_Stripe DASM_Stripe;
struct DASM_Stripe
{
  Arena *arena;
  OS_Handle cv;
  OS_Handle rw_mutex;
};

typedef struct DASM_StripeTable DASM_StripeTable;
struct DASM_StripeTable
{
  U64 count;
  DASM_Stripe *v;
};

////////////////////////////////
//~ rjf: Entity Cache Types

typedef struct DASM_Entity DASM_Entity;
struct DASM_Entity
{
  DASM_Entity *next;
  
  // rjf: key info
  CTRL_MachineID machine_id;
  DMN_Handle process;
  Rng1U64 vaddr_range;
  U64 id;
  
  // rjf: top-level info
  U64 last_time_sent_us;
  U64 working_count;
  U64 bytes_processed;
  U64 bytes_to_process;
  
  // rjf: decoded instruction data
  Arena *decode_inst_arena;
  Arena *decode_string_arena;
  DASM_InstArray decode_inst_array;
};

typedef struct DASM_EntitySlot DASM_EntitySlot;
struct DASM_EntitySlot
{
  DASM_Entity *first;
  DASM_Entity *last;
};

typedef struct DASM_EntityMap DASM_EntityMap;
struct DASM_EntityMap
{
  U64 slots_count;
  DASM_EntitySlot *slots;
};

////////////////////////////////
//~ rjf: Introspection Info Types

typedef struct DASM_BinaryInfo DASM_BinaryInfo;
struct DASM_BinaryInfo
{
  CTRL_MachineID machine_id;
  DMN_Handle process;
  Rng1U64 vaddr_range;
  U64 bytes_processed;
  U64 bytes_to_process;
};

////////////////////////////////
//~ rjf: Decode Request Types

typedef struct DASM_DecodeRequest DASM_DecodeRequest;
struct DASM_DecodeRequest
{
  DASM_Handle handle;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct DASM_Shared DASM_Shared;
struct DASM_Shared
{
  Arena *arena;
  
  // rjf: entity table
  DASM_EntityMap entity_map;
  DASM_StripeTable entity_map_stripes;
  U64 entity_id_gen;
  
  // rjf: user -> decode ring
  OS_Handle u2d_ring_mutex;
  OS_Handle u2d_ring_cv;
  U64 u2d_ring_size;
  U8 *u2d_ring_base;
  U64 u2d_ring_write_pos;
  U64 u2d_ring_read_pos;
  
  // rjf: decode threads
  U64 decode_thread_count;
  OS_Handle *decode_threads;
};

////////////////////////////////
//~ rjf: Globals

global DASM_Shared *dasm_shared = 0;

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void dasm_init(void);

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 dasm_hash_from_string(String8 string);

////////////////////////////////
//~ rjf: Instruction Type Functions

internal void dasm_inst_chunk_list_push(Arena *arena, DASM_InstChunkList *list, U64 cap, DASM_Inst *inst);
internal DASM_InstArray dasm_inst_array_from_chunk_list(Arena *arena, DASM_InstChunkList *list);
internal U64 dasm_inst_array_idx_from_off__linear_scan(DASM_InstArray *array, U64 off);
internal U64 dasm_inst_array_off_from_idx(DASM_InstArray *array, U64 idx);

////////////////////////////////
//~ rjf: Disassembly Functions

internal DASM_InstChunkList dasm_inst_chunk_list_from_arch_addr_data(Arena *arena, U64 *bytes_processed_counter, Architecture arch, U64 addr, String8 data);

////////////////////////////////
//~ rjf: Cache Lookups

//- rjf: opening handles & correllation with module
internal DASM_Handle dasm_handle_from_ctrl_process_range(CTRL_MachineID machine, DMN_Handle process, Rng1U64 vaddr_range);

//- rjf: asking for top-level info of a handle
internal DASM_BinaryInfo dasm_binary_info_from_handle(Arena *arena, DASM_Handle handle);

//- rjf: asking for decoded instructions
internal DASM_InstArray dasm_inst_array_from_handle(Arena *arena, DASM_Handle handle, U64 endt_us);

////////////////////////////////
//~ rjf: Decode Threads

internal B32 dasm_u2d_enqueue_request(DASM_DecodeRequest *req, U64 endt_us);
internal DASM_DecodeRequest dasm_u2d_dequeue_request(void);

internal void dasm_decode_thread_entry_point(void *p);

#endif //DASM_H
