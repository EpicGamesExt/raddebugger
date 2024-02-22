// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef TASK_SYSTEM_H
#define TASK_SYSTEM_H

////////////////////////////////
//~ rjf: Task "Ticket" Type
//
// "Tickets" are opaque handles, used to refer to submitted tasks.
//

typedef struct TS_Ticket TS_Ticket;
struct TS_Ticket
{
  U64 u64[2];
};

typedef struct TS_TicketNode TS_TicketNode;
struct TS_TicketNode
{
  TS_TicketNode *next;
  TS_Ticket v;
};

typedef struct TS_TicketList TS_TicketList;
struct TS_TicketList
{
  TS_TicketNode *first;
  TS_TicketNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Task Request Types

typedef void *TS_TaskFunctionType(Arena *arena, void *user_data);

////////////////////////////////
//~ rjf: Task Artifact Cache Types

typedef struct TS_TaskArtifact TS_TaskArtifact;
struct TS_TaskArtifact
{
  TS_TaskArtifact *next;
  U64 num;
  B64 task_is_done;
  void *result;
};

typedef struct TS_TaskArtifactSlot TS_TaskArtifactSlot;
struct TS_TaskArtifactSlot
{
  TS_TaskArtifact *first;
  TS_TaskArtifact *last;
};

typedef struct TS_TaskArtifactStripe TS_TaskArtifactStripe;
struct TS_TaskArtifactStripe
{
  Arena *arena;
  OS_Handle cv;
  OS_Handle rw_mutex;
  TS_TaskArtifact *free_artifact;
};

////////////////////////////////
//~ rjf: Per-Thread State

typedef struct TS_TaskThread TS_TaskThread;
struct TS_TaskThread
{
  Arena *arena;
  OS_Handle thread;
};

////////////////////////////////
//~ rjf: Main Shared State

typedef struct TS_Shared TS_Shared;
struct TS_Shared
{
  Arena *arena;
  
  // rjf: task artifact cache
  U64 artifact_num_gen;
  U64 artifact_slots_count;
  U64 artifact_stripes_count;
  TS_TaskArtifactSlot *artifact_slots;
  TS_TaskArtifactStripe *artifact_stripes;
  
  // rjf: task ring buffer
  U64 u2t_ring_size;
  U8 *u2t_ring_base;
  U64 u2t_ring_write_pos;
  U64 u2t_ring_read_pos;
  OS_Handle u2t_ring_mutex;
  OS_Handle u2t_ring_cv;
  
  // rjf: task threads
  TS_TaskThread *task_threads;
  U64 task_threads_count;
};

////////////////////////////////
//~ rjf: Globals

global TS_Shared *ts_shared = 0;

////////////////////////////////
//~ rjf: Basic Type Functions

internal TS_Ticket ts_ticket_zero(void);
internal void ts_ticket_list_push(Arena *arena, TS_TicketList *list, TS_Ticket ticket);

////////////////////////////////
//~ rjf: Top-Level Layer Initialization

internal void ts_init(void);

////////////////////////////////
//~ rjf: High-Level Task Kickoff / Joining

internal TS_Ticket ts_kickoff(TS_TaskFunctionType *entry_point, Arena **optional_arena_ptr, void *p);
internal void *ts_join(TS_Ticket ticket, U64 endt_us);
#define ts_join_struct(ticket, endt_us, type) (type *)ts_join((ticket), (endt_us))

////////////////////////////////
//~ rjf: Task Threads

internal void ts_u2t_dequeue_task(TS_TaskFunctionType **entry_point_out, Arena **arena_out, void **p_out, TS_Ticket *ticket_out);
internal void ts_task_thread__entry_point(void *p);

#endif // TASK_SYSTEM_H
