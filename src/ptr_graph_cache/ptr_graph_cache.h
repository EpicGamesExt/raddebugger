// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef PTR_GRAPH_CACHE_H
#define PTR_GRAPH_CACHE_H

////////////////////////////////
//~ rjf: Graph Search Key

typedef struct PTG_Key PTG_Key;
struct PTG_Key
{
  U128 root_hash;
  U64 link_offsets[8];
  U64 link_offsets_count;
};

////////////////////////////////
//~ rjf: Cache Types

typedef struct PTG_Node PTG_Node;
struct PTG_Node
{
  U64 value;
};

typedef struct PTG_Link PTG_Link;
struct PTG_Link
{
  U32 from;
  U32 to;
};

typedef struct PTG_NodeChunkNode PTG_NodeChunkNode;
struct PTG_NodeChunkNode
{
  PTG_NodeChunkNode *next;
  PTG_Node *v;
  U64 count;
  U64 cap;
};

typedef struct PTG_NodeChunkList PTG_NodeChunkList;
struct PTG_NodeChunkList
{
  PTG_NodeChunkNode *first;
  PTG_NodeChunkNode *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct PTG_NodeArray PTG_NodeArray;
struct PTG_NodeArray
{
  PTG_Node *v;
  U64 count;
};

typedef struct PTG_LinkChunkNode PTG_LinkChunkNode;
struct PTG_LinkChunkNode
{
  PTG_LinkChunkNode *next;
  PTG_Link *v;
  U64 count;
  U64 cap;
};

typedef struct PTG_LinkChunkList PTG_LinkChunkList;
struct PTG_LinkChunkList
{
  PTG_LinkChunkNode *first;
  PTG_LinkChunkNode *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct PTG_LinkArray PTG_LinkArray;
struct PTG_LinkArray
{
  PTG_Link *v;
  U64 count;
};

typedef struct PTG_Graph PTG_Graph;
struct PTG_Graph
{
  PTG_NodeArray nodes;
  PTG_LinkArray links;
};

typedef struct PTG_GraphNode PTG_GraphNode;
struct PTG_GraphNode
{
  // rjf: links
  PTG_GraphNode *next;
  PTG_GraphNode *prev;
  
  // rjf: key
  PTG_Key key;
  
  // rjf: metadata
  U64 scope_ref_count;
  U64 last_time_touched_us;
  U64 last_user_clock_idx_touched;
  U64 load_count;
  B32 is_working;
  
  // rjf: content
  Arena *arena;
  PTG_Graph graph;
};

typedef struct PTG_GraphSlot PTG_GraphSlot;
struct PTG_GraphSlot
{
  PTG_GraphNode *first;
  PTG_GraphNode *last;
};

typedef struct PTG_GraphStripe PTG_GraphStripe;
struct PTG_GraphStripe
{
  Arena *arena;
  RWMutex rw_mutex;
  CondVar cv;
  PTG_GraphNode *free_node;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct PTG_Shared PTG_Shared;
struct PTG_Shared
{
  Arena *arena;
  
  // rjf: cache
  U64 slots_count;
  U64 stripes_count;
  PTG_GraphSlot *slots;
  PTG_GraphStripe *stripes;
  
  // rjf: user -> xfer thread
  U64 u2b_ring_size;
  U8 *u2b_ring_base;
  U64 u2b_ring_write_pos;
  U64 u2b_ring_read_pos;
  CondVar u2b_ring_cv;
  Mutex u2b_ring_mutex;
  
  // rjf: builder threads
  U64 builder_thread_count;
  Thread *builder_threads;
  
  // rjf: evictor thread
  Thread evictor_thread;
};

////////////////////////////////
//~ rjf: Globals

global PTG_Shared *ptg_shared = 0;

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void ptg_init(void);

////////////////////////////////
//~ rjf: Cache Lookups

internal PTG_Graph *ptg_graph_from_key(Access *access, PTG_Key *key);

////////////////////////////////
//~ rjf: Transfer Threads

internal B32 ptg_u2b_enqueue_req(PTG_Key *key, U64 endt_us);
internal void ptg_u2b_dequeue_req(PTG_Key *key_out);
internal void ptg_builder_thread__entry_point(void *p);

////////////////////////////////
//~ rjf: Evictor Threads

internal void ptg_evictor_thread__entry_point(void *p);

#endif // PTR_GRAPH_CACHE_H
