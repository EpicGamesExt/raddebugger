// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_ChunkRef
lnk_chunk_ref(U64 sect_id, U64 chunk_id)
{
  LNK_ChunkRef ref = {0};
  ref.sect_id      = sect_id;
  ref.chunk_id     = chunk_id;
  return ref;
}

internal B32
lnk_chunk_ref_is_equal(LNK_ChunkRef a, LNK_ChunkRef b)
{
  B32 is_equal = a.sect_id == b.sect_id && a.chunk_id == b.chunk_id;
  return is_equal;
}

internal LNK_ChunkNode *
lnk_chunk_list_push(Arena *arena, LNK_ChunkList *list, LNK_Chunk *chunk)
{
  LNK_ChunkNode *node = push_array_no_zero(arena, LNK_ChunkNode, 1);
  node->next = 0;
  node->data = chunk;

  SLLQueuePush(list->first, list->last, node);
  ++list->count;

  return node;
}

internal void
lnk_chunk_list_concat_in_place(LNK_ChunkList *list, LNK_ChunkList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal void
lnk_chunk_list_concat_in_place_arr(LNK_ChunkList *list, LNK_ChunkList *arr, U64 count)
{
  SLLConcatInPlaceArray(list, arr, count);
}

internal LNK_ChunkList **
lnk_make_chunk_list_arr_arr(Arena *arena, U64 slot_count, U64 per_count)
{
  LNK_ChunkList **arr_arr = push_array_no_zero(arena, LNK_ChunkList *, slot_count);
  for (U64 i = 0; i < slot_count; i += 1) {
    arr_arr[i] = push_array(arena, LNK_ChunkList, per_count);
  }
  return arr_arr;
}

internal int
lnk_chunk_sort_index_is_before(void *raw_a, void *raw_b)
{
  // Grouped Sections (PE Format)
  //  "All contributions with the same object-section name are allocated contiguously in the image,
  //  and the blocks of contributions are sorted in lexical order by object-section name." 
  LNK_ChunkPtr *a = raw_a;
  LNK_ChunkPtr *b = raw_b;
          
  // sort on section postfix
  int cmp = str8_compar_case_sensitive(&(*a)->sort_idx, &(*b)->sort_idx);

  // sort on obj position on command line
  if (cmp == 0) {
    cmp = u64_compar(&(*a)->input_idx, &(*b)->input_idx);
  }

  int is_before = cmp < 0;
  return is_before;
}

internal void
lnk_chunk_array_sort(LNK_ChunkArray arr)
{
  radsort(arr.v, arr.count, lnk_chunk_sort_index_is_before);
}

internal LNK_ChunkManager *
lnk_chunk_manager_alloc(Arena *arena, U64 id, U64 align)
{
  ProfBeginFunction();

  LNK_ChunkList temp_list = {0};

  LNK_Chunk temp_chunk = {0};
  temp_chunk.ref       = lnk_chunk_ref(id, 0);
  temp_chunk.align     = align;
  temp_chunk.type      = LNK_Chunk_List;
  temp_chunk.u.list    = &temp_list;

  LNK_ChunkManager *cman  = push_array_no_zero(arena, LNK_ChunkManager, 1);
  cman->total_chunk_count = 1; // null chunk
  cman->root              = 0;
  cman->root              = lnk_chunk_push_list(arena, cman, &temp_chunk, str8(0,0));
  cman->root->align       = align;
  
  ProfEnd();
  return cman;
}

internal LNK_Chunk *
lnk_chunk_push_(Arena *arena, LNK_Chunk *parent, U64 chunk_id, String8 sort_index)
{
  ProfBeginFunction();

  Assert(parent->type == LNK_Chunk_List);
  LNK_ChunkList *list = parent->u.list;

  LNK_Chunk *chunk    = push_array_no_zero(arena, LNK_Chunk, 1);
  chunk->ref          = lnk_chunk_ref(parent->ref.sect_id, chunk_id);
  chunk->align        = 1;
  chunk->is_discarded = 0;
  chunk->sort_chunk   = 1;
  chunk->type         = LNK_Chunk_Null;
  chunk->sort_idx     = push_str8_copy(arena, sort_index);
  chunk->input_idx    = list->count;
  chunk->flags        = 0;
  chunk->associate    = 0;

  lnk_chunk_list_push(arena, list, chunk);

  ProfEnd();
  return chunk;
}

internal LNK_Chunk *
lnk_chunk_push(Arena *arena, LNK_ChunkManager *cman, LNK_Chunk *parent, String8 sort_index)
{
  U64 chunk_id = cman->total_chunk_count;
  ++cman->total_chunk_count;
  LNK_Chunk *chunk = lnk_chunk_push_(arena, parent, chunk_id, sort_index);
  return chunk;
}

internal LNK_Chunk *
lnk_chunk_push_leaf(Arena *arena, LNK_ChunkManager *cman, LNK_Chunk *parent, String8 sort_index, void *raw_ptr, U64 raw_size)
{
  LNK_Chunk *chunk = lnk_chunk_push(arena, cman, parent, sort_index);
  chunk->type      = LNK_Chunk_Leaf;
  chunk->u.leaf    = str8((U8 *)raw_ptr, raw_size);
  return chunk;
}

internal LNK_Chunk *
lnk_chunk_push_list(Arena *arena, LNK_ChunkManager *cman, LNK_Chunk *parent, String8 sort_index)
{
  LNK_Chunk *chunk = lnk_chunk_push(arena, cman, parent, sort_index);
  chunk->type      = LNK_Chunk_List;
  chunk->u.list    = push_array(arena, LNK_ChunkList, 1);
  return chunk;
}

internal LNK_ChunkNode *
lnk_chunk_deep_copy(Arena *arena, LNK_Chunk *chunk)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  LNK_ChunkNode *dst_root_node = push_array_no_zero(arena, LNK_ChunkNode, 1);
  LNK_ChunkNode *src_root_node = push_array_no_zero(scratch.arena, LNK_ChunkNode, 1);
  src_root_node->next = 0;
  src_root_node->data = chunk;
  
  struct Stack {
    struct Stack  *next;
    LNK_ChunkNode *src_node;
    LNK_ChunkNode *dst_node;
  };
  struct Stack *stack = push_array_no_zero(scratch.arena, struct Stack, 1);
  stack->next         = 0;
  stack->src_node     = src_root_node;
  stack->dst_node     = dst_root_node;
  
  while (stack) {
    while (stack->src_node) {
      LNK_Chunk *src = stack->src_node->data;
      LNK_Chunk *dst = stack->dst_node->data;

      stack->src_node = stack->src_node->next;
      stack->dst_node = stack->dst_node->next;

      dst->ref          = src->ref;
      dst->type         = src->type;
      dst->align        = src->align;
      dst->is_discarded = src->is_discarded;
      dst->sort_chunk   = src->sort_chunk;
      dst->sort_idx     = push_str8_copy(arena, src->sort_idx);
      dst->input_idx    = src->input_idx;
      dst->flags        = src->flags;
      //dst->associate    = src->associate;
      Assert(src->associate == 0);
      lnk_chunk_set_debugf(arena, dst, "%S", src->debug);
    
      switch (src->type) {
      case LNK_Chunk_Null: break;
      case LNK_Chunk_Leaf: {
        B32 is_bss = src->u.leaf.str == 0;
        if (is_bss) {
          dst->u.leaf = src->u.leaf;
        } else {
          dst->u.leaf = push_str8_copy(arena, src->u.leaf);
        }
      } break;
      case LNK_Chunk_List: {
        LNK_ChunkNode *chain = 0;
        LNK_ChunkNode *curr  = 0;
        if (src->u.list->count > 0) {
          chain = push_array(arena, LNK_ChunkNode, src->u.list->count);
          curr = chain;
          for (U64 i = 1; i < src->u.list->count; ++i) {
            curr->next = &chain[i];
            curr = curr->next;
          }
          curr->next = 0;
        }

        dst->u.list        = push_array_no_zero(arena, LNK_ChunkList, 1);
        dst->u.list->count = src->u.list->count;
        dst->u.list->first = chain;
        dst->u.list->last  = curr;

        struct Stack *frame = push_array_no_zero(scratch.arena, struct Stack, 1);
        frame->next         = 0;
        frame->src_node     = src->u.list->first;
        frame->dst_node     = dst->u.list->first;
        SLLStackPush(stack, frame);
      } break;
      default: InvalidPath; break;
      }
    }
    
    SLLStackPop(stack);
  }
  
  scratch_end(scratch);
  ProfEnd();
  return dst_root_node;
}

internal LNK_ChunkNode *
lnk_merge_chunks(Arena *arena, LNK_ChunkManager *dst_cman, LNK_Chunk *dst, LNK_Chunk *src, U64 *id_map_out, U64 id_map_max)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 0);

  Assert(src->ref.sect_id != dst->ref.sect_id);
  Assert(dst->type == LNK_Chunk_List);
  Assert(src->type != LNK_Chunk_Null);
  
  LNK_ChunkNode *src_node = push_array(arena, LNK_ChunkNode, 1);
  src_node->data = src;
  
  struct Stack {
    struct Stack *next;
    LNK_ChunkNode *node;
  };
  struct Stack *stack = push_array_no_zero(scratch.arena, struct Stack, 1);
  stack->next         = 0;
  stack->node         = src_node;
  
  while (stack) {
    while (stack->node) {
      LNK_Chunk *chunk = stack->node->data;
      
      // advance node
      stack->node = stack->node->next;
      
      // allocate id
      U64 new_id = dst_cman->total_chunk_count++;
      
      // write id map
      Assert(chunk->ref.chunk_id < id_map_max);
      id_map_out[chunk->ref.chunk_id] = new_id;
      
      // update id
      chunk->ref = lnk_chunk_ref(dst->ref.sect_id, new_id);
      
      // recurse down on lists
      if (chunk->type == LNK_Chunk_List) {
        struct Stack *frame = push_array_no_zero(scratch.arena, struct Stack, 1);
        frame->next         = 0;
        frame->node         = chunk->u.list->first;
        SLLStackPush(stack, frame);
      }
    }
    
    // reached end of chunk list, pop frame
    SLLStackPop(stack);
  }
  
  // move source root copy to destination section
  LNK_ChunkList *list = dst->u.list;
  ++list->count;
  SLLQueuePush(list->first, list->last, src_node);
  
  scratch_end(scratch);
  ProfEnd();
  return src_node;
}

internal
LNK_CHUNK_VISITOR_SIG(lnk_set_associate_on_chunks)
{
  chunk->associate = (LNK_Chunk *)ud;
  return 0;
}

internal void
lnk_chunk_associate(LNK_Chunk *head, LNK_Chunk *chunk)
{
  // for simplicity we don't support multiple associations,
  // but it's possible to craft symbol table with multiple associations
  AssertAlways(!chunk->associate);
  lnk_visit_chunks(0, chunk, lnk_set_associate_on_chunks, head);
}

internal B32
lnk_chunk_is_discarded(LNK_Chunk *chunk)
{
  B32        is_discarded = chunk->is_discarded;
  LNK_Chunk *curr         = chunk->associate;
  while (!is_discarded && curr) {
    is_discarded = curr->is_discarded;
    curr = curr->associate;
  }
  return is_discarded;
}

internal U64
lnk_chunk_get_size(LNK_Chunk *chunk)
{
  U64 result = 0;
  switch (chunk->type) {
  case LNK_Chunk_Null: break;
  case LNK_Chunk_Leaf: { 
    result = chunk->u.leaf.size;
  } break;
  case LNK_Chunk_LeafArray:
  case LNK_Chunk_List: {
    Assert(!"TODO: list size");
  } break;
  }
  return result;
}

internal U64
lnk_chunk_list_get_node_count(LNK_Chunk *chunk)
{
  Assert(chunk->type == LNK_Chunk_List);
  return chunk->u.list->count;
}

internal void
lnk_chunk_pad_array_list_push(Arena *arena, Arena *scratch, LNK_ChunkPadArrayList *list, U64 cap, U64 align_off, U64 align_size)
{
  if (align_size > 0) {
    if (list->last == 0 || list->last->data.count >= list->last->cap) {
      LNK_ChunkPadArrayNode *node = push_array(scratch, LNK_ChunkPadArrayNode, 1);
      node->cap                     = cap;
      node->data.v                  = push_array_no_zero(arena, LNK_ChunkPad, cap);

      SLLQueuePush(list->first, list->last, node);
      ++list->count;
    }

    LNK_ChunkPadArray *last_array = &list->last->data;
    LNK_ChunkPad *align = &last_array->v[last_array->count++];
    align->off            = align_off;
    align->size           = align_size;
  }
}

internal
LNK_CHUNK_VISITOR_SIG(lnk_offset_chunks)
{
  LNK_OffsetChunks *offset_chunks = ud;
  U64               offset        = offset_chunks->offset;
  LNK_ChunkLayout  *layout        = offset_chunks->layout;

  layout->chunk_off_array[chunk->ref.chunk_id] += offset;

  return 0;
}

internal LNK_ChunkLayout
lnk_layout_from_chunk(Arena *arena, LNK_Chunk *root, U64 total_chunk_count)
{
  ProfBeginV("lnk_layout_from_chunk [total_chunk_count = %llu]", total_chunk_count);
  Temp scratch = scratch_begin(&arena, 1);
  
  LNK_ChunkLayout layout       = {0};
  layout.total_count           = total_chunk_count;
  layout.chunk_ptr_array       = push_array_no_zero(arena, LNK_ChunkPtr, total_chunk_count);
  layout.chunk_off_array       = push_array_no_zero(arena, U64,          total_chunk_count);
  layout.chunk_file_size_array = push_array_no_zero(arena, U64,          total_chunk_count);
  layout.chunk_virt_size_array = push_array_no_zero(arena, U64,          total_chunk_count);

  ProfBegin("Init Arrays");
  for (U64 i = 0; i < total_chunk_count; ++i) {
    layout.chunk_ptr_array[i] = &g_null_chunk;
  }
#if BUILD_DEBUG
  MemorySet(layout.chunk_off_array,       0xff, total_chunk_count * sizeof(layout.chunk_off_array));
  MemorySet(layout.chunk_file_size_array, 0xff, total_chunk_count * sizeof(layout.chunk_file_size_array));
  MemorySet(layout.chunk_virt_size_array, 0xff, total_chunk_count * sizeof(layout.chunk_virt_size_array));
#endif
  ProfEnd();

  // handle null chunk
  layout.chunk_off_array[0]       = 0;
  layout.chunk_file_size_array[0] = 0;
  layout.chunk_virt_size_array[0] = 0;

  // setup stack
  struct Stack {
    struct Stack  *next;
    LNK_ChunkArray chunk_array;
    U64            ichunk;
  };
  struct Stack *stack      = push_array(scratch.arena, struct Stack, 1);
  stack->chunk_array.count = 1;
  stack->chunk_array.v     = &root;

  U64                   pad_cap  = 4096;
  LNK_ChunkPadArrayList pad_list = {0};

  U64 cursor = 0;

  ProfBegin("Traverse chunks from root");
  for (; stack != 0; ) {
    for (; stack->ichunk < stack->chunk_array.count; ) {
      LNK_Chunk *chunk = stack->chunk_array.v[stack->ichunk++];
      
      // skip discarded chunk
      if (lnk_chunk_is_discarded(chunk)) {
        continue;
      }

      switch (chunk->type) {
      case LNK_Chunk_Leaf: {
        // push pad
        if (chunk->u.leaf.size < chunk->min_size) {
          U64 pad_size = chunk->min_size - chunk->u.leaf.size;
          lnk_chunk_pad_array_list_push(arena, scratch.arena, &pad_list, pad_cap, cursor, pad_size);
          cursor += pad_size;
        }

        // push align
        U64 align_size = AlignPadPow2(cursor, chunk->align);
        lnk_chunk_pad_array_list_push(arena, scratch.arena, &pad_list, pad_cap, cursor, align_size);
        cursor += align_size;
		
        // store id -> chunk
        Assert(chunk->ref.chunk_id < total_chunk_count);
        Assert(layout.chunk_ptr_array[chunk->ref.chunk_id] == &g_null_chunk);
        layout.chunk_ptr_array[chunk->ref.chunk_id] = chunk;

        // store id -> offset
        Assert(layout.chunk_off_array[chunk->ref.chunk_id] == max_U64);
        layout.chunk_off_array[chunk->ref.chunk_id] = cursor;

        // store id -> file size
        Assert(layout.chunk_file_size_array[chunk->ref.chunk_id] == max_U64);
        layout.chunk_file_size_array[chunk->ref.chunk_id] = chunk->u.leaf.size;
		
        // store id -> virt size
        Assert(layout.chunk_virt_size_array[chunk->ref.chunk_id] == max_U64);
        layout.chunk_virt_size_array[chunk->ref.chunk_id] = chunk->u.leaf.size;

        // advance
        cursor += chunk->u.leaf.size;
      } break;

      case LNK_Chunk_LeafArray: {
        // push align
        U64 align_size = AlignPadPow2(cursor, chunk->align);
        lnk_chunk_pad_array_list_push(arena, scratch.arena, &pad_list, pad_cap, cursor, align_size);
        cursor += align_size;

        // store id -> chunk
        Assert(chunk->ref.chunk_id < total_chunk_count);
        Assert(layout.chunk_ptr_array[chunk->ref.chunk_id] == &g_null_chunk);
        layout.chunk_ptr_array[chunk->ref.chunk_id] = chunk;

        // store id -> offset
        Assert(layout.chunk_off_array[chunk->ref.chunk_id] == max_U64);
        layout.chunk_off_array[chunk->ref.chunk_id] = cursor;

        // apply sort
        if (chunk->sort_chunk) {
          lnk_chunk_array_sort(*chunk->u.arr);
        }

        // recurse into sub chunks
        struct Stack *frame = push_array(scratch.arena, struct Stack, 1);
        frame->chunk_array  = *chunk->u.arr;
        SLLStackPush(stack, frame);
      } goto _continue;
      
      case LNK_Chunk_List: {
        // push align
        U64 align_size = AlignPadPow2(cursor, chunk->align);
        lnk_chunk_pad_array_list_push(arena, scratch.arena, &pad_list, pad_cap, cursor, align_size);
        cursor += align_size;

        // store id -> chunk
        Assert(chunk->ref.chunk_id < total_chunk_count);
        Assert(layout.chunk_ptr_array[chunk->ref.chunk_id] == &g_null_chunk);
        layout.chunk_ptr_array[chunk->ref.chunk_id] = chunk;

        // store id -> offset
        Assert(layout.chunk_off_array[chunk->ref.chunk_id] == max_U64);
        layout.chunk_off_array[chunk->ref.chunk_id] = cursor;

        // list -> array
        LNK_ChunkArray chunk_array = {0};
        chunk_array.v              = push_array_no_zero(scratch.arena, LNK_ChunkPtr, chunk->u.list->count);
        for (LNK_ChunkNode *cptr = chunk->u.list->first; cptr != 0; cptr = cptr->next) {
          chunk_array.v[chunk_array.count++] = cptr->data;
        }
        
        // apply sort
        if (chunk->sort_chunk) {
          lnk_chunk_array_sort(chunk_array);
        }
        
        // recurse into sub chunks
        struct Stack *frame = push_array(scratch.arena, struct Stack, 1);
        frame->chunk_array  = chunk_array;
        SLLStackPush(stack, frame);
      } goto _continue;
      
      case LNK_Chunk_Null: break;
      }
    }
    
    // terminate series
    if (stack->next) {
      // pop node chunk from stack
      struct Stack *prev = stack->next;

      Assert(prev->ichunk > 0);
      LNK_Chunk *chunk = prev->chunk_array.v[prev->ichunk-1];

      U64 chunk_data_off = layout.chunk_off_array[chunk->ref.chunk_id];
      Assert(chunk_data_off != max_U64);
      Assert(chunk_data_off <= cursor);

      U64 chunk_data_size = cursor - chunk_data_off;

      // store id -> virt size (no pad and align)
      Assert(layout.chunk_virt_size_array[chunk->ref.chunk_id] == max_U64);
      layout.chunk_virt_size_array[chunk->ref.chunk_id] = chunk_data_size;

      // push pad
      if (chunk_data_size < chunk->min_size) {
        U64 pad_size = chunk->min_size - chunk->u.leaf.size;
        lnk_chunk_pad_array_list_push(arena, scratch.arena, &pad_list, pad_cap, chunk_data_off, pad_size);

        LNK_OffsetChunks ud = {0};
        ud.offset           = pad_size;
        ud.layout           = &layout;
        lnk_visit_chunks(0, chunk, lnk_offset_chunks, &ud);
      }

      // align chunk end
      U64 align_size = AlignPadPow2(cursor, chunk->align);
      lnk_chunk_pad_array_list_push(arena, scratch.arena, &pad_list, pad_cap, cursor, align_size);
      cursor += align_size;

      chunk_data_size = cursor - chunk_data_off;

      // store id -> file size (pad + align)
      Assert(layout.chunk_file_size_array[chunk->ref.chunk_id] == max_U64);
      layout.chunk_file_size_array[chunk->ref.chunk_id] = chunk_data_size;
    }
    
    // move to next frame
    SLLStackPop(stack);
    
    _continue:;
  }
  ProfEnd();

  ProfBegin("Build Pad Array");
  layout.pad_array_count = 0;
  layout.pad_array       = push_array(arena, LNK_ChunkPadArray, pad_list.count);
  for (LNK_ChunkPadArrayNode *node = pad_list.first; node != 0; node = node->next) {
    layout.pad_array[layout.pad_array_count++] = node->data;
  }
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
  return layout;
}

internal LNK_ChunkLayout
lnk_build_chunk_layout(Arena *arena, LNK_ChunkManager *cman)
{
  ProfBeginFunction();
  LNK_ChunkLayout layout = lnk_layout_from_chunk(arena, cman->root, cman->total_chunk_count);
  ProfEnd();
  return layout;
}

internal
THREAD_POOL_TASK_FUNC(lnk_fill_chunks_task)
{
  ProfBeginFunction();

  LNK_ChunkLayoutSerializer *task   = raw_task;
  Rng1U64                    range  = task->ranges[task_id];
  LNK_ChunkLayout            layout = task->layout;
  String8                    buffer = task->buffer;

  for (U64 chunk_idx = range.min; chunk_idx < range.max; ++chunk_idx) {
    LNK_Chunk *chunk = layout.chunk_ptr_array[chunk_idx];

    if (lnk_chunk_is_discarded(chunk)) {
      continue;
    }

    if (chunk->type == LNK_Chunk_Leaf) {
      U64 off = layout.chunk_off_array[chunk->ref.chunk_id];
      Assert(off + chunk->u.leaf.size <= buffer.size);
      U8 *buffer_ptr = buffer.str + off;

      if (chunk->u.leaf.str == 0) {
        // zero out chunk bytes
        MemorySet(buffer_ptr, 0, chunk->u.leaf.size);
      } else {
        // copy chunk bytes
        MemoryCopy(buffer_ptr, chunk->u.leaf.str, chunk->u.leaf.size);
      }
    }
  }
  
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_fill_pads_task)
{
  ProfBeginFunction();

  LNK_ChunkLayoutSerializer *task      = raw_task;
  Rng1U64                    range     = task->ranges[task_id];
  LNK_ChunkLayout            layout    = task->layout;
  String8                    buffer    = task->buffer;
  U8                         fill_byte = task->fill_byte;

  for (U64 pad_array_idx = range.min; pad_array_idx < range.max; ++pad_array_idx) {
    LNK_ChunkPadArray pad_array = layout.pad_array[pad_array_idx];
    for (U64 pad_idx = 0; pad_idx < pad_array.count; ++pad_idx) {
      LNK_ChunkPad pad = pad_array.v[pad_idx];
      Assert(pad.off + pad.size <= buffer.size);
      MemorySet(buffer.str + pad.off, fill_byte, pad.size);
    }
  }

  ProfEnd();
}

internal void
lnk_serialize_chunk_layout(TP_Context *tp, LNK_ChunkLayout layout, String8 buffer, U8 fill_byte)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  LNK_ChunkLayoutSerializer task;
  task.layout    = layout;
  task.buffer    = buffer;
  task.fill_byte = fill_byte;

  ProfBeginV("Fill Chunks [Chunk Count %llu]", layout.total_count);
  task.ranges = tp_divide_work(scratch.arena, layout.total_count, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_fill_chunks_task, &task);
  ProfEnd();

  ProfBeginV("Fill Pads [Array Count %llu]", layout.pad_array_count);
  task.ranges = tp_divide_work(scratch.arena, layout.pad_array_count, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_fill_pads_task, &task);
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
}

internal B32
lnk_visit_chunks_(U64 sect_id, LNK_Chunk *chunk, LNK_ChunkVisitorSig *cb, void *ud)
{
  // visit chunk
  B32 is_done = cb(sect_id, chunk, ud);
  if (is_done) {
    return is_done;
  }
  
  switch (chunk->type) {
  case LNK_Chunk_Null:
  case LNK_Chunk_Leaf: {
    // reached leaf
  } break;
  case LNK_Chunk_LeafArray: {
    for (U64 idx = 0; idx < chunk->u.arr->count; idx += 1) {
      is_done = lnk_visit_chunks_(sect_id, chunk->u.arr->v[idx], cb, ud);
      if (is_done) {
        break;
      }
    }
  } break;
  case LNK_Chunk_List: {
    for (LNK_ChunkNode *i = chunk->u.list->first; i != 0; i = i->next) {
      is_done = lnk_visit_chunks_(sect_id, i->data, cb, ud);
      if (is_done) {
        break;
      }
    }
  } break;
  }

  return is_done;
}

internal void
lnk_visit_chunks(U64 sect_id, LNK_Chunk *chunk, LNK_ChunkVisitorSig *cb, void *ud)
{
  lnk_visit_chunks_(sect_id, chunk, cb, ud);
}

LNK_CHUNK_VISITOR_SIG(lnk_save_chunk_ptr)
{
  LNK_Chunk **id_map = (LNK_Chunk **)ud;
  if (!chunk->is_discarded) {
    id_map[chunk->ref.chunk_id] = chunk;
  }
  return 0;
}

internal LNK_ChunkPtr *
lnk_make_chunk_id_map(Arena *arena, LNK_ChunkManager *cman)
{
  LNK_ChunkPtr *map = push_array_no_zero(arena, LNK_ChunkPtr, cman->total_chunk_count);
  lnk_visit_chunks(0, cman->root, lnk_save_chunk_ptr, map);
  map[0] = &g_null_chunk;
  return map;
}

internal LNK_ChunkNode *
lnk_chunk_ptr_list_reserve(Arena *arena, LNK_ChunkList *list, U64 count)
{
  LNK_ChunkNode *arr = 0;
  if (count) {
    arr = push_array(arena, LNK_ChunkNode, count);
    LNK_Chunk *chunk_arr = push_array(arena, LNK_Chunk, count);
    for (U64 i = 0; i < count; i += 1) {
      arr[i].data = &chunk_arr[i];
      SLLQueuePush(list->first, list->last, &arr[i]);
    }
    list->count += count;
  }
  return arr;
}

internal String8Array
lnk_data_arr_from_chunk_ptr_list(Arena *arena, LNK_ChunkList list)
{
  String8Array arr = {0};
  arr.v            = push_array(arena, String8, list.count);
  for (LNK_ChunkNode *n = list.first; n != 0; n = n->next) {
    LNK_ChunkPtr c = n->data;
    Assert(c->type == LNK_Chunk_Leaf);
    arr.v[arr.count] = c->u.leaf;
    arr.count += 1;
  }
  return arr;
}

internal String8Array *
lnk_data_arr_from_chunk_ptr_list_arr(Arena *arena, LNK_ChunkList *list_arr, U64 count)
{
  String8Array *result = push_array(arena, String8Array, count);
  for (U64 i = 0; i < count; i += 1) {
    result[i] = lnk_data_arr_from_chunk_ptr_list(arena, list_arr[i]);
  }
  return result;
}

internal String8
lnk_string_from_chunk_type(LNK_ChunkType type)
{
  switch (type) {
  case LNK_Chunk_Null:      return str8_lit("Null");
  case LNK_Chunk_Leaf:      return str8_lit("Leaf");
  case LNK_Chunk_LeafArray: return str8_lit("LeafArray");
  case LNK_Chunk_List:      return str8_lit("List");
  default: InvalidPath;
  }
  return str8_zero();
}
