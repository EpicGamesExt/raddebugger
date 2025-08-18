// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0xb8a06bff

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
mtx_init(void)
{
  Arena *arena = arena_alloc();
  mtx_shared = push_array(arena, MTX_Shared, 1);
  mtx_shared->arena = arena;
  mtx_shared->slots_count = 256;
  mtx_shared->stripes_count = Min(mtx_shared->slots_count, os_get_system_info()->logical_processor_count);
  mtx_shared->slots = push_array(arena, MTX_Slot, mtx_shared->slots_count);
  mtx_shared->stripes = push_array(arena, MTX_Stripe, mtx_shared->stripes_count);
  for(U64 idx = 0; idx < mtx_shared->stripes_count; idx += 1)
  {
    mtx_shared->stripes[idx].arena = arena_alloc();
    mtx_shared->stripes[idx].rw_mutex = rw_mutex_alloc();
  }
  mtx_shared->mut_threads_count = Min(os_get_system_info()->logical_processor_count, 4);
  mtx_shared->mut_threads = push_array(arena, MTX_MutThread, mtx_shared->mut_threads_count);
  for(U64 idx = 0; idx < mtx_shared->mut_threads_count; idx += 1)
  {
    mtx_shared->mut_threads[idx].ring_size = KB(64);
    mtx_shared->mut_threads[idx].ring_base = push_array_no_zero(arena, U8, mtx_shared->mut_threads[idx].ring_size);
    mtx_shared->mut_threads[idx].cv = cond_var_alloc();
    mtx_shared->mut_threads[idx].mutex = mutex_alloc();
    mtx_shared->mut_threads[idx].thread = os_thread_launch(mtx_mut_thread__entry_point, &mtx_shared->mut_threads[idx], 0);
  }
}

////////////////////////////////
//~ rjf: Buffer Operations

internal void
mtx_push_op(HS_Key buffer_key, MTX_Op op)
{
  U64 hash = hs_little_hash_from_data(str8_struct(&buffer_key));
  MTX_MutThread *thread = &mtx_shared->mut_threads[hash%mtx_shared->mut_threads_count];
  mtx_enqueue_op(thread, buffer_key, op);
}

////////////////////////////////
//~ rjf: Mutation Threads

internal void
mtx_enqueue_op(MTX_MutThread *thread, HS_Key buffer_key, MTX_Op op)
{
  // TODO(rjf): if op.replace is too big, need to split into multiple edits
  OS_MutexScope(thread->mutex) for(;;)
  {
    U64 unconsumed_size = thread->ring_write_pos - thread->ring_read_pos;
    U64 available_size = thread->ring_size - unconsumed_size;
    U64 needed_size = sizeof(buffer_key) + sizeof(op.range) + sizeof(op.replace.size) + op.replace.size;
    if(available_size >= needed_size)
    {
      thread->ring_write_pos += ring_write_struct(thread->ring_base, thread->ring_size, thread->ring_write_pos, &buffer_key);
      thread->ring_write_pos += ring_write_struct(thread->ring_base, thread->ring_size, thread->ring_write_pos, &op.range);
      thread->ring_write_pos += ring_write_struct(thread->ring_base, thread->ring_size, thread->ring_write_pos, &op.replace.size);
      thread->ring_write_pos += ring_write(thread->ring_base, thread->ring_size, thread->ring_write_pos, op.replace.str, op.replace.size);
      break;
    }
    cond_var_wait(thread->cv, thread->mutex, max_U64);
  }
  cond_var_broadcast(thread->cv);
}

internal void
mtx_dequeue_op(Arena *arena, MTX_MutThread *thread, HS_Key *buffer_key_out, MTX_Op *op_out)
{
  OS_MutexScope(thread->mutex) for(;;)
  {
    U64 unconsumed_size = thread->ring_write_pos - thread->ring_read_pos;
    if(unconsumed_size >= sizeof(*buffer_key_out) + sizeof(op_out->range) + sizeof(op_out->replace.size))
    {
      thread->ring_read_pos += ring_read_struct(thread->ring_base, thread->ring_size, thread->ring_read_pos, buffer_key_out);
      thread->ring_read_pos += ring_read_struct(thread->ring_base, thread->ring_size, thread->ring_read_pos, &op_out->range);
      thread->ring_read_pos += ring_read_struct(thread->ring_base, thread->ring_size, thread->ring_read_pos, &op_out->replace.size);
      op_out->replace.str = push_array_no_zero(arena, U8, op_out->replace.size);
      thread->ring_read_pos += ring_read(thread->ring_base, thread->ring_size, thread->ring_read_pos, op_out->replace.str, op_out->replace.size);
      break;
    }
    cond_var_wait(thread->cv, thread->mutex, max_U64);
  }
  cond_var_broadcast(thread->cv);
}

internal void
mtx_mut_thread__entry_point(void *p)
{
  MTX_MutThread *mut_thread = (MTX_MutThread *)p;
  ThreadNameF("[mtx] mut thread #%I64u", (U64)(mut_thread - mtx_shared->mut_threads));
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    HS_Scope *hs_scope = hs_scope_open();
    
    //- rjf: get next op
    HS_Key buffer_key = {0};
    MTX_Op op = {0};
    mtx_dequeue_op(scratch.arena, mut_thread, &buffer_key, &op);
    
    //- rjf: get buffer's current data
    U128 hash = hs_hash_from_key(buffer_key, 0);
    String8 data = hs_data_from_hash(hs_scope, hash);
    
    //- rjf: clamp op by data
    op.range.min = Min(op.range.min, data.size);
    op.range.max = Min(op.range.max, data.size);
    
    //- rjf: construct new buffer
    if(op.range.max != op.range.min || op.replace.size != 0)
    {
      U64 new_data_size = data.size + op.replace.size - dim_1u64(op.range);
      Arena *arena = arena_alloc(.commit_size = new_data_size + ARENA_HEADER_SIZE, .reserve_size = new_data_size + ARENA_HEADER_SIZE);
      U8 *new_data_base = push_array_no_zero(arena, U8, new_data_size);
      String8 pre_replace_data = str8_substr(data, r1u64(0, op.range.min));
      String8 post_replace_data = str8_substr(data, r1u64(op.range.max, data.size));
      if(pre_replace_data.size != 0)
      {
        MemoryCopy(new_data_base+0,                                     pre_replace_data.str, pre_replace_data.size);
      }
      if(op.replace.size != 0)
      {
        MemoryCopy(new_data_base+pre_replace_data.size,                 op.replace.str, op.replace.size);
      }
      if(post_replace_data.size != 0)
      {
        MemoryCopy(new_data_base+pre_replace_data.size+op.replace.size, post_replace_data.str, post_replace_data.size);
      }
      String8 new_data = str8(new_data_base, new_data_size);
      hs_submit_data(buffer_key, &arena, new_data);
    }
    
    hs_scope_close(hs_scope);
    scratch_end(scratch);
  }
}
