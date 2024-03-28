// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DASM_H
#define DASM_H

////////////////////////////////
//~ rjf: Handle Type

typedef struct DASMI_Handle DASMI_Handle;
struct DASMI_Handle
{
  U64 u64[2];
};

////////////////////////////////
//~ rjf: Instruction Types

typedef struct DASMI_Inst DASMI_Inst;
struct DASMI_Inst
{
  String8 string;
  U64 off;
  U64 addr;
};

typedef struct DASMI_InstChunkNode DASMI_InstChunkNode;
struct DASMI_InstChunkNode
{
  DASMI_InstChunkNode *next;
  DASMI_Inst *v;
  U64 cap;
  U64 count;
};

typedef struct DASMI_InstChunkList DASMI_InstChunkList;
struct DASMI_InstChunkList
{
  DASMI_InstChunkNode *first;
  DASMI_InstChunkNode *last;
  U64 node_count;
  U64 inst_count;
};

typedef struct DASMI_InstArray DASMI_InstArray;
struct DASMI_InstArray
{
  DASMI_Inst *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Striped Access Types

typedef struct DASMI_Stripe DASMI_Stripe;
struct DASMI_Stripe
{
  Arena *arena;
  OS_Handle cv;
  OS_Handle rw_mutex;
};

typedef struct DASMI_StripeTable DASMI_StripeTable;
struct DASMI_StripeTable
{
  U64 count;
  DASMI_Stripe *v;
};

////////////////////////////////
//~ rjf: Entity Cache Types

typedef struct DASMI_Entity DASMI_Entity;
struct DASMI_Entity
{
  DASMI_Entity *next;
  
  // rjf: key info
  CTRL_MachineID machine_id;
  DMN_Handle process;
  Rng1U64 vaddr_range;
  Architecture arch;
  U64 id;
  
  // rjf: top-level info
  U64 last_time_sent_us;
  U64 working_count;
  U64 bytes_processed;
  U64 bytes_to_process;
  
  // rjf: decoded instruction data
  Arena *decode_inst_arena;
  Arena *decode_string_arena;
  DASMI_InstArray decode_inst_array;
};

typedef struct DASMI_EntitySlot DASMI_EntitySlot;
struct DASMI_EntitySlot
{
  DASMI_Entity *first;
  DASMI_Entity *last;
};

typedef struct DASMI_EntityMap DASMI_EntityMap;
struct DASMI_EntityMap
{
  U64 slots_count;
  DASMI_EntitySlot *slots;
};

////////////////////////////////
//~ rjf: Introspection Info Types

typedef struct DASMI_BinaryInfo DASMI_BinaryInfo;
struct DASMI_BinaryInfo
{
  CTRL_MachineID machine_id;
  DMN_Handle process;
  Rng1U64 vaddr_range;
  U64 bytes_processed;
  U64 bytes_to_process;
};

////////////////////////////////
//~ rjf: Decode Request Types

typedef struct DASMI_DecodeRequest DASMI_DecodeRequest;
struct DASMI_DecodeRequest
{
  DASMI_Handle handle;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct DASMI_Shared DASMI_Shared;
struct DASMI_Shared
{
  Arena *arena;
  
  // rjf: entity table
  DASMI_EntityMap entity_map;
  DASMI_StripeTable entity_map_stripes;
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

global DASMI_Shared *dasmi_shared = 0;

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void dasmi_init(void);

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 dasmi_hash_from_string(String8 string);

////////////////////////////////
//~ rjf: Instruction Type Functions

internal void dasmi_inst_chunk_list_push(Arena *arena, DASMI_InstChunkList *list, U64 cap, DASMI_Inst *inst);
internal DASMI_InstArray dasmi_inst_array_from_chunk_list(Arena *arena, DASMI_InstChunkList *list);
internal U64 dasmi_inst_array_idx_from_off__linear_scan(DASMI_InstArray *array, U64 off);
internal U64 dasmi_inst_array_off_from_idx(DASMI_InstArray *array, U64 idx);

////////////////////////////////
//~ rjf: Disassembly Functions

internal DASMI_InstChunkList dasmi_inst_chunk_list_from_arch_addr_data(Arena *arena, U64 *bytes_processed_counter, Architecture arch, U64 addr, String8 data);

////////////////////////////////
//~ rjf: Cache Lookups

//- rjf: opening handles & correllation with module
internal DASMI_Handle dasmi_handle_from_ctrl_process_range_arch(CTRL_MachineID machine, DMN_Handle process, Rng1U64 vaddr_range, Architecture arch);

//- rjf: asking for top-level info of a handle
internal DASMI_BinaryInfo dasmi_binary_info_from_handle(Arena *arena, DASMI_Handle handle);

//- rjf: asking for decoded instructions
internal DASMI_InstArray dasmi_inst_array_from_handle(Arena *arena, DASMI_Handle handle, U64 endt_us);

////////////////////////////////
//~ rjf: Decode Threads

internal B32 dasmi_u2d_enqueue_request(DASMI_DecodeRequest *req, U64 endt_us);
internal DASMI_DecodeRequest dasmi_u2d_dequeue_request(void);

internal void dasmi_decode_thread_entry_point(void *p);

#endif //DASM_H
