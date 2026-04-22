// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_ENGINE_CTRL_H
#define DBG_ENGINE_CTRL_H

////////////////////////////////
//~ rjf: Unwind Types

typedef U32 D_UnwindFlags;
enum
{
  D_UnwindFlag_Error = (1<<0),
  D_UnwindFlag_Stale = (1<<1),
};

typedef struct D_UnwindStepResult D_UnwindStepResult;
struct D_UnwindStepResult
{
  D_UnwindFlags flags;
};

typedef struct D_UnwindFrame D_UnwindFrame;
struct D_UnwindFrame
{
  void *regs;
  U64 cfa;
};

typedef struct D_UnwindFrameNode D_UnwindFrameNode;
struct D_UnwindFrameNode
{
  D_UnwindFrameNode *next;
  D_UnwindFrameNode *prev;
  D_UnwindFrame v;
};

typedef struct D_UnwindFrameArray D_UnwindFrameArray;
struct D_UnwindFrameArray
{
  D_UnwindFrame *v;
  U64 count;
};

typedef struct D_Unwind D_Unwind;
struct D_Unwind
{
  D_UnwindFrameArray frames;
  D_UnwindFlags flags;
};

typedef struct D_FrameUnwindContext D_FrameUnwindContext;
struct D_FrameUnwindContext
{
  // DWARF
  U64 cfa;
  DW_CFI_Row *cfi_row;
  U64 ret_addr_reg;
};

////////////////////////////////
//~ rjf: Call Stack Types

typedef struct D_CallStackFrame D_CallStackFrame;
struct D_CallStackFrame
{
  U64 unwind_count;
  U64 inline_depth;
  void *regs;
  U64 cfa;
};

typedef struct D_CallStack D_CallStack;
struct D_CallStack
{
  D_CallStackFrame *frames;
  U64 frames_count;
  D_CallStackFrame **concrete_frames;
  U64 concrete_frames_count;
};

////////////////////////////////
//~ rjf: Call Stack Tree Types

typedef struct D_CallStackTreeNode D_CallStackTreeNode;
struct D_CallStackTreeNode
{
  D_CallStackTreeNode *hash_next;
  D_CallStackTreeNode *first;
  D_CallStackTreeNode *last;
  D_CallStackTreeNode *next;
  D_CallStackTreeNode *parent;
  U64 child_count;
  U64 id;
  D_Handle process;
  U64 vaddr;
  U64 depth;
  D_HandleList threads;
  U64 all_descendant_threads_count;
};

typedef struct D_CallStackTree D_CallStackTree;
struct D_CallStackTree
{
  D_CallStackTreeNode *root;
  U64 slots_count;
  D_CallStackTreeNode **slots;
};

////////////////////////////////
//~ rjf: Evaluation Spaces

typedef U64 D_EvalSpaceKind;
enum
{
  D_EvalSpaceKind_Entity = E_SpaceKind_FirstUserDefined,
  D_EvalSpaceKind_FirstUserDefined,
};

////////////////////////////////
//~ rjf: Process Memory Types

typedef struct D_ProcessMemorySlice D_ProcessMemorySlice;
struct D_ProcessMemorySlice
{
  String8 data;
  U64 *byte_bad_flags;
  U64 *byte_changed_flags;
  B32 stale;
  B32 any_byte_bad;
  B32 any_byte_changed;
};

////////////////////////////////
//~ rjf: Thread Register Cache Types

typedef struct D_ThreadRegCacheNode D_ThreadRegCacheNode;
struct D_ThreadRegCacheNode
{
  D_ThreadRegCacheNode *next;
  D_ThreadRegCacheNode *prev;
  D_Handle handle;
  U64 block_size;
  void *block;
  U64 reg_gen;
};

typedef struct D_ThreadRegCacheSlot D_ThreadRegCacheSlot;
struct D_ThreadRegCacheSlot
{
  D_ThreadRegCacheNode *first;
  D_ThreadRegCacheNode *last;
};

typedef struct D_ThreadRegCacheStripe D_ThreadRegCacheStripe;
struct D_ThreadRegCacheStripe
{
  Arena *arena;
  RWMutex rw_mutex;
};

typedef struct D_ThreadRegCache D_ThreadRegCache;
struct D_ThreadRegCache
{
  U64 slots_count;
  D_ThreadRegCacheSlot *slots;
  U64 stripes_count;
  D_ThreadRegCacheStripe *stripes;
};

////////////////////////////////
//~ rjf: Module Image Info Cache Types

typedef struct D_ModuleImageInfoCacheNode D_ModuleImageInfoCacheNode;
struct D_ModuleImageInfoCacheNode
{
  D_ModuleImageInfoCacheNode *next;
  D_ModuleImageInfoCacheNode *prev;
  D_Handle module;
  Arena *arena;
  PE_IntelPdata *pdatas;
  U64 pdatas_count;
  U64 cfi_rebase;
  B32 is_unwind_eh;
  String8 dwarf_unwind_data;
  EH_FrameHdr eh_frame_hdr;
  EH_PtrCtx eh_ptr_ctx;
  U64 entry_point_voff;
  String8 initial_debug_info_path;
  U64 raddbg_attached_marker_voff;
  String8 raddbg_data;
};

typedef struct D_ModuleImageInfoCacheSlot D_ModuleImageInfoCacheSlot;
struct D_ModuleImageInfoCacheSlot
{
  D_ModuleImageInfoCacheNode *first;
  D_ModuleImageInfoCacheNode *last;
};

typedef struct D_ModuleImageInfoCacheStripe D_ModuleImageInfoCacheStripe;
struct D_ModuleImageInfoCacheStripe
{
  Arena *arena;
  RWMutex rw_mutex;
};

typedef struct D_ModuleImageInfoCache D_ModuleImageInfoCache;
struct D_ModuleImageInfoCache
{
  U64 slots_count;
  D_ModuleImageInfoCacheSlot *slots;
  U64 stripes_count;
  D_ModuleImageInfoCacheStripe *stripes;
};

////////////////////////////////
//~ rjf: Touched Debug Info Directory Cache

typedef struct D_DbgDirNode D_DbgDirNode;
struct D_DbgDirNode
{
  D_DbgDirNode *first;
  D_DbgDirNode *last;
  D_DbgDirNode *next;
  D_DbgDirNode *prev;
  D_DbgDirNode *parent;
  String8 name;
  U64 search_count;
  U64 child_count;
  U64 module_direct_count;
};

////////////////////////////////
//~ rjf: Control Thread Evaluation Scopes

typedef struct D_EvalScope D_EvalScope;
struct D_EvalScope
{
  Access *access;
  E_BaseCtx base_ctx;
  E_IRCtx ir_ctx;
  E_InterpretCtx interpret_ctx;
};

////////////////////////////////
//~ rjf: Module Requirement Cache Types

typedef struct D_ModuleReqCacheNode D_ModuleReqCacheNode;
struct D_ModuleReqCacheNode
{
  D_ModuleReqCacheNode *next;
  D_Handle module;
  B32 required;
};

////////////////////////////////
//~ rjf: Wakeup Hook Function Types

#define D_WAKEUP_FUNCTION_DEF(name) void name(void)
typedef D_WAKEUP_FUNCTION_DEF(D_WakeupFunctionType);

////////////////////////////////
//~ rjf: Main State Types

typedef struct D_CtrlState D_CtrlState;
struct D_CtrlState
{
  Arena *arena;
  D_WakeupFunctionType *wakeup_hook;
  
  // rjf: name -> register/alias hash tables for eval
  E_String2NumMap arch_string2reg_tables[Arch_COUNT];
  E_String2NumMap arch_string2alias_tables[Arch_COUNT];
  
  // rjf: caches
  D_ThreadRegCache thread_reg_cache;
  D_ModuleImageInfoCache module_image_info_cache;
  
  // rjf: generations
  U64 run_gen;
  U64 mem_gen;
  U64 reg_gen;
  
  // rjf: user -> ctrl msg ring buffer
  U64 u2c_ring_size;
  U8 *u2c_ring_base;
  U64 u2c_ring_write_pos;
  U64 u2c_ring_read_pos;
  Mutex u2c_ring_mutex;
  CondVar u2c_ring_cv;
  
  // rjf: ctrl -> user event ring buffer
  U64 c2u_ring_size;
  U64 c2u_ring_max_string_size;
  U8 *c2u_ring_base;
  U64 c2u_ring_write_pos;
  U64 c2u_ring_read_pos;
  Mutex c2u_ring_mutex;
  CondVar c2u_ring_cv;
  
  // rjf: ctrl thread state
  U64 ctrl_thread_run_state;
  String8 ctrl_thread_log_path;
  Thread ctrl_thread;
  Log *ctrl_thread_log;
  RWMutex ctrl_thread_entity_ctx_rw_mutex;
  D_EntityCtxRWStore *ctrl_thread_entity_store;
  E_Cache *ctrl_thread_eval_cache;
  Arena *ctrl_thread_msg_process_arena;
  Arena *dmn_event_arena;
  DMN_EventNode *first_dmn_event_node;
  DMN_EventNode *last_dmn_event_node;
  DMN_EventNode *free_dmn_event_node;
  Arena *user_entry_point_arena;
  String8List user_entry_points;
  U64 exception_code_filters[(D_ExceptionCodeKind_COUNT+63)/64];
  U64 process_counter;
  Arena *dbg_dir_arena;
  D_DbgDirNode *dbg_dir_root;
  U64 module_req_cache_slots_count;
  D_ModuleReqCacheNode **module_req_cache_slots;
  String8List msg_user_bp_touched_files;
  String8List msg_user_bp_touched_symbols;
};

////////////////////////////////
//~ rjf: Globals

global D_CtrlState *d_ctrl_state = 0;
read_only global D_Entity d_entity_nil =
{
  &d_entity_nil,
  &d_entity_nil,
  &d_entity_nil,
  &d_entity_nil,
  &d_entity_nil,
};
read_only global D_CallStackTreeNode d_call_stack_tree_node_nil =
{
  0,
  &d_call_stack_tree_node_nil,
  &d_call_stack_tree_node_nil,
  &d_call_stack_tree_node_nil,
  &d_call_stack_tree_node_nil,
};
thread_static D_EntityCtxLookupAccel *d_entity_ctx_lookup_accel = 0;

////////////////////////////////
//~ rjf: Basic Type Functions

internal U64 ctrl_hash_from_string(String8 string);
internal U64 ctrl_hash_from_handle(D_Handle handle);
internal D_EventCause ctrl_event_cause_from_dmn_event_kind(DMN_EventKind event_kind);
internal D_ExceptionKind ctrl_exception_kind_from_dmn(DMN_ExceptionKind kind);
internal D_TlsModel ctrl_dynamic_linker_type_from_dmn(DMN_TlsModel type);
internal String8 ctrl_string_from_event_kind(D_EventKind kind);
internal String8 ctrl_string_from_msg_kind(D_MsgKind kind);
internal D_EntityKind ctrl_entity_kind_from_string(String8 string);
internal DMN_TrapFlags ctrl_dmn_trap_flags_from_user_breakpoint_flags(D_BreakpointFlags flags);
internal D_BreakpointFlags ctrl_user_breakpoint_flags_from_dmn_trap_flags(DMN_TrapFlags flags);

////////////////////////////////
//~ rjf: Handle Type Functions

internal D_Handle ctrl_handle_zero(void);
internal D_Handle ctrl_handle_make(D_MachineID machine_id, DMN_Handle dmn_handle);
internal B32 ctrl_handle_match(D_Handle a, D_Handle b);
internal void ctrl_handle_list_push(Arena *arena, D_HandleList *list, D_Handle *pair);
internal D_HandleList ctrl_handle_list_copy(Arena *arena, D_HandleList *src);
internal D_HandleArray ctrl_handle_array_from_list(Arena  *arena, D_HandleList *src);
internal String8 ctrl_string_from_handle(Arena *arena, D_Handle handle);
internal D_Handle ctrl_handle_from_string(String8 string);

////////////////////////////////
//~ rjf: Trap Type Functions

internal void ctrl_trap_list_push(Arena *arena, D_TrapList *list, D_Trap *trap);
internal D_TrapList ctrl_trap_list_copy(Arena *arena, D_TrapList *src);

////////////////////////////////
//~ rjf: User Breakpoint Type Functions

internal void ctrl_user_breakpoint_list_push(Arena *arena, D_BreakpointList *list, D_Breakpoint *bp);
internal D_BreakpointList ctrl_user_breakpoint_list_copy(Arena *arena, D_BreakpointList *src);

////////////////////////////////
//~ rjf: Message Type Functions

//- rjf: deep copying
internal void ctrl_msg_deep_copy(Arena *arena, D_Msg *dst, D_Msg *src);

//- rjf: list building
internal D_Msg *ctrl_msg_list_push(Arena *arena, D_MsgList *list);
internal D_MsgList ctrl_msg_list_deep_copy(Arena *arena, D_MsgList *src);
internal void ctrl_msg_list_concat_in_place(D_MsgList *dst, D_MsgList *src);

//- rjf: serialization
internal String8 ctrl_serialized_string_from_msg_list(Arena *arena, D_MsgList *msgs);
internal D_MsgList ctrl_msg_list_from_serialized_string(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Event Type Functions

//- rjf: list building
internal D_Event *ctrl_event_list_push(Arena *arena, D_EventList *list);
internal void ctrl_event_list_concat_in_place(D_EventList *dst, D_EventList *to_push);

//- rjf: serialization
internal String8 ctrl_serialized_string_from_event(Arena *arena, D_Event *event, U64 max);
internal D_Event ctrl_event_from_serialized_string(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Entity Type Functions

//- rjf: entity list data structures
internal void ctrl_entity_list_push(Arena *arena, D_EntityList *list, D_Entity *entity);
internal D_EntityList ctrl_entity_list_from_handle_list(Arena *arena, D_EntityCtx *ctx, D_HandleList *list);
#define ctrl_entity_list_first(list) ((list)->first ? (list)->first->v : &d_entity_nil)

//- rjf: entity array data structure
internal D_EntityArray ctrl_entity_array_from_list(Arena *arena, D_EntityList *list);
#define ctrl_entity_array_first(array) ((array)->count != 0 ? (array)->v[0] : &d_entity_nil)

//- rjf: entity context (entity group read-only) functions
internal D_Entity *ctrl_entity_from_handle(D_EntityCtx *ctx, D_Handle handle);
internal D_Entity *ctrl_entity_child_from_kind(D_Entity *parent, D_EntityKind kind);
internal D_Entity *ctrl_entity_ancestor_from_kind(D_Entity *entity, D_EntityKind kind);
internal D_Entity *ctrl_process_from_entity(D_Entity *entity);
internal D_Entity *ctrl_module_from_process_vaddr(D_Entity *process, U64 vaddr);
internal DI_Key ctrl_dbgi_key_from_module(D_Entity *module);
internal D_Entity *ctrl_module_from_thread_candidates(D_EntityCtx *ctx, D_Entity *thread, D_EntityList *candidates);
internal U64 ctrl_vaddr_from_voff(D_Entity *module, U64 voff);
internal U64 ctrl_voff_from_vaddr(D_Entity *module, U64 vaddr);
internal Rng1U64 ctrl_vaddr_range_from_voff_range(D_Entity *module, Rng1U64 voff_range);
internal Rng1U64 ctrl_voff_range_from_vaddr_range(D_Entity *module, Rng1U64 vaddr_range);
internal B32 ctrl_entity_tree_is_frozen(D_Entity *root);

//- rjf: entity tree iteration
internal D_EntityRec ctrl_entity_rec_depth_first(D_Entity *entity, D_Entity *subtree_root, U64 sib_off, U64 child_off);
#define ctrl_entity_rec_depth_first_pre(entity, subtree_root)  ctrl_entity_rec_depth_first((entity), (subtree_root), OffsetOf(D_Entity, next), OffsetOf(D_Entity, first))
#define ctrl_entity_rec_depth_first_post(entity, subtree_root) ctrl_entity_rec_depth_first((entity), (subtree_root), OffsetOf(D_Entity, prev), OffsetOf(D_Entity, last))

//- rjf: entity ctx r/w store state functions
internal D_EntityCtxRWStore *ctrl_entity_ctx_rw_store_alloc(void);
internal void ctrl_entity_ctx_rw_store_release(D_EntityCtxRWStore *store);

//- rjf: string allocation/deletion
internal U64 ctrl_name_bucket_num_from_string_size(U64 size);
internal String8 ctrl_entity_string_alloc(D_EntityCtxRWStore *store, String8 string);
internal void ctrl_entity_string_release(D_EntityCtxRWStore *store, String8 string);

//- rjf: entity construction/deletion
internal D_Entity *ctrl_entity_alloc(D_EntityCtxRWStore *store, D_Entity *parent, D_EntityKind kind, Arch arch, D_Handle handle, U64 id);
internal void ctrl_entity_release(D_EntityCtxRWStore *store, D_Entity *entity);

//- rjf: entity equipment
internal void ctrl_entity_equip_string(D_EntityCtxRWStore *store, D_Entity *entity, String8 string);

//- rjf: accelerated entity context lookups
internal D_EntityCtxLookupAccel *ctrl_thread_entity_ctx_lookup_accel(void);
internal D_EntityArray ctrl_entity_array_from_kind(D_EntityCtx *ctx, D_EntityKind kind);
internal D_EntityList ctrl_modules_from_dbgi_key(Arena *arena, D_EntityCtx *ctx, DI_Key dbgi_key);
internal D_Entity *ctrl_thread_from_id(D_EntityCtx *ctx, U64 id);

//- rjf: applying events to entity caches
internal void ctrl_entity_store_apply_events(D_EntityCtxRWStore *store, D_EventList *list);

////////////////////////////////
//~ rjf: Wakeup Callback Registration

internal void ctrl_set_wakeup_hook(D_WakeupFunctionType *wakeup_hook);

////////////////////////////////
//~ rjf: Thread Register Functions

//- rjf: thread register cache reading
internal void *ctrl_reg_block_from_thread(Arena *arena, D_EntityCtx *ctx, D_Handle handle);
internal U64 ctrl_tls_root_vaddr_from_thread(D_EntityCtx *ctx, D_Handle handle);
internal U64 ctrl_rip_from_thread(D_EntityCtx *ctx, D_Handle handle);
internal U64 ctrl_rsp_from_thread(D_EntityCtx *ctx, D_Handle handle);

//- rjf: thread register writing
internal B32 ctrl_thread_write_reg_block(D_Handle thread, void *block);

////////////////////////////////
//~ rjf: Module Image Info Functions

//- rjf: cache lookups
internal PE_IntelPdata *ctrl_intel_pdata_from_module_voff(Arena *arena, D_Handle module_handle, U64 voff);
internal U64 ctrl_entry_point_voff_from_module(D_Handle module_handle);
internal String8 ctrl_initial_debug_info_path_from_module(Arena *arena, D_Handle module_handle);
internal String8 ctrl_raddbg_data_from_module(Arena *arena, D_Handle module_handle);

////////////////////////////////
//~ Process Info Functions

internal Arch ctrl_arch_from_process_handle(D_Handle process_handle);

////////////////////////////////
//~ rjf: Unwinding Functions

//- rjf: unwind deep copier
internal D_Unwind ctrl_unwind_deep_copy(Arena *arena, Arch arch, D_Unwind *src);

//- DWARF
internal D_UnwindStepResult ctrl_establish_frame_unwind_context__dwarf(Arena *arena, D_Handle process_handle, D_Handle module_handle, Arch arch, void *regs, U64 endt_us, D_FrameUnwindContext *ctx_out);
internal D_UnwindStepResult ctrl_unwind_step__dwarf(D_Handle process_handle, Arch arch, void *regs, D_FrameUnwindContext *frame_ctx, U64 endt_us);

//- rjf: [x64]
internal REGS_Reg64 *ctrl_unwind_reg_from_pe_gpr_reg__pe_x64(REGS_RegBlockX64 *regs, PE_UnwindGprRegX64 gpr_reg);
internal D_UnwindStepResult ctrl_unwind_step__pe_x64(D_Handle process_handle, D_Handle module_handle, U64 module_base_vaddr, REGS_RegBlockX64 *regs, U64 endt_us);

//- rjf: abstracted full unwind
internal D_Unwind ctrl_unwind_from_thread(Arena *arena, D_EntityCtx *ctx, D_Handle thread, U64 endt_us);

////////////////////////////////
//~ rjf: Call Stack Building Functions

internal D_CallStack ctrl_call_stack_from_unwind(Arena *arena, D_Entity *process, D_Unwind *base_unwind);
internal D_CallStackFrame *ctrl_call_stack_frame_from_unwind_and_inline_depth(D_CallStack *call_stack, U64 unwind_count, U64 inline_depth);

////////////////////////////////
//~ rjf: Halting All Attached Processes

internal void ctrl_halt(void);

////////////////////////////////
//~ rjf: Shared Accessor Functions

//- rjf: generation counters
internal U64 ctrl_run_gen(void);
internal U64 ctrl_mem_gen(void);
internal U64 ctrl_reg_gen(void);

//- rjf: name -> register/alias hash tables, for eval
internal E_String2NumMap *ctrl_string2reg_from_arch(Arch arch);
internal E_String2NumMap *ctrl_string2alias_from_arch(Arch arch);

////////////////////////////////
//~ rjf: Control-Thread Functions

//- rjf: user -> control thread communication
internal B32 ctrl_u2c_push_msgs(D_MsgList *msgs, U64 endt_us);
internal D_MsgList ctrl_u2c_pop_msgs(Arena *arena);

//- rjf: control -> user thread communication
internal void ctrl_c2u_push_events(D_EventList *events);
internal D_EventList ctrl_c2u_pop_events(Arena *arena);

//- rjf: entry point
internal void ctrl_thread__entry_point(void *p);

//- rjf: breakpoint resolution
internal void ctrl_thread__append_resolved_module_user_bp_traps(Arena *arena, D_EvalScope *eval_scope, D_Handle process, D_Handle module, D_BreakpointList *user_bps, DMN_TrapChunkList *traps_out);
internal void ctrl_thread__append_resolved_process_user_bp_traps(Arena *arena, D_EvalScope *eval_scope, D_Handle process, D_BreakpointList *user_bps, DMN_TrapChunkList *traps_out);
internal void ctrl_thread__append_program_defined_bp_traps(Arena *arena, D_Entity *bp, DMN_TrapChunkList *traps_out);

//- rjf: module lifetime open/close work
internal void ctrl_thread__module_open(D_Handle process, D_Handle module, Rng1U64 vaddr_range, String8 path, Rng1U64 elf_phdr_vrange, U64 elf_phdr_entsize);
internal void ctrl_thread__module_close(D_Handle process, D_Handle module, Rng1U64 vaddr_range);

//- rjf: attached process running/event gathering
internal DMN_Event *ctrl_thread__next_dmn_event(Arena *arena, DMN_CtrlCtx *ctrl_ctx, D_Msg *msg, DMN_RunCtrls *run_ctrls, D_Spoof *spoof);

//- rjf: eval helpers
internal U64 ctrl_eval_space_gen(E_Space space);
internal B32 ctrl_eval_space_read(E_Space space, void *out, Rng1U64 vaddr_range);

//- rjf: control thread eval scopes
internal D_EvalScope *ctrl_thread__eval_scope_begin(Arena *arena, D_BreakpointList *user_bps, D_Entity *thread);
internal void ctrl_thread__eval_scope_end(D_EvalScope *scope);

//- rjf: log flusher
internal void ctrl_thread__end_and_flush_log(void);

//- rjf: msg kind implementations
internal void ctrl_thread__launch(DMN_CtrlCtx *ctrl_ctx, D_Msg *msg);
internal void ctrl_thread__attach(DMN_CtrlCtx *ctrl_ctx, D_Msg *msg);
internal void ctrl_thread__kill(DMN_CtrlCtx *ctrl_ctx, D_Msg *msg);
internal void ctrl_thread__kill_all(DMN_CtrlCtx *ctrl_ctx, D_Msg *msg);
internal void ctrl_thread__detach(DMN_CtrlCtx *ctrl_ctx, D_Msg *msg);
internal void ctrl_thread__run(DMN_CtrlCtx *ctrl_ctx, D_Msg *msg);
internal void ctrl_thread__single_step(DMN_CtrlCtx *ctrl_ctx, D_Msg *msg);

////////////////////////////////
//~ rjf: Process Memory Artifact Cache Hooks / Lookups

internal AC_Artifact ctrl_memory_artifact_create(String8 key, B32 *cancel_signal, B32 *retry_out, U64 *gen_out);
internal void ctrl_memory_artifact_destroy(AC_Artifact artifact);
internal C_Key ctrl_key_from_process_vaddr_range(D_Handle process, Rng1U64 vaddr_range, B32 zero_terminated, B32 wait_for_fresh, U64 endt_us, B32 *out_is_stale);

//- rjf: process memory reading helpers
internal D_ProcessMemorySlice ctrl_process_memory_slice_from_vaddr_range(Arena *arena, D_Handle process, Rng1U64 range, B32 wait_for_fresh, U64 endt_us);
internal B32 ctrl_process_memory_read(D_Handle process, Rng1U64 range, B32 *is_stale_out, void *out, U64 endt_us);
#define ctrl_process_memory_read_struct(process, vaddr, is_stale_out, ptr, endt_us) ctrl_process_memory_read((process), r1u64((vaddr), (vaddr)+(sizeof(*(ptr)))), (is_stale_out), (ptr), (endt_us))

//- rjf: process memory writing
internal B32 ctrl_process_write(D_Handle process, Rng1U64 range, void *src);

////////////////////////////////
//~ rjf: Call Stack Artifact Cache Hooks / Lookups

internal AC_Artifact ctrl_call_stack_artifact_create(String8 key, B32 *cancel_signal, B32 *retry_out, U64 *gen_out);
internal void ctrl_call_stack_artifact_destroy(AC_Artifact artifact);
internal D_CallStack ctrl_call_stack_from_thread(Access *access, D_Handle thread_handle, B32 high_priority, U64 endt_us);

////////////////////////////////
//~ rjf: Call Stack Tree Artifact Cache Hooks / Lookups

internal AC_Artifact ctrl_call_stack_tree_artifact_create(String8 key, B32 *cancel_signal, B32 *retry_out, U64 *gen_out);
internal void ctrl_call_stack_tree_artifact_destroy(AC_Artifact artifact);
internal D_CallStackTree ctrl_call_stack_tree(Access *access, U64 endt_us);

#endif // DBG_ENGINE_CTRL_H
