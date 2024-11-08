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

      dst->ref      = src->ref;
      dst->align    = src->align;
      dst->sort_idx = push_str8_copy(arena, src->sort_idx);
      dst->type     = src->type;
      dst->flags    = src->flags;
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

internal void
lnk_chunk_associate(Arena *arena, LNK_Chunk *head, LNK_Chunk *chunk)
{
  // for simplicity we don't support multiple associations,
  // but it's possible to craft symbol table with multiple associations
  Assert(!chunk->associate);
  chunk->associate = head;
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
lnk_chunk_op_list_push_node(LNK_ChunkOpList *list, LNK_ChunkOp *op)
{
  SLLQueuePush(list->first, list->last, op);
}

internal LNK_ChunkOp *
lnk_push_chunk_op_begin(Arena *arena, U64 chunk_id)
{
  LNK_ChunkOp *begin_op = push_array_no_zero(arena, LNK_ChunkOp, 1);
  begin_op->next        = 0;
  begin_op->type        = LNK_ChunkOp_Begin;
  begin_op->u.chunk_id  = chunk_id;
  return begin_op;
}

internal LNK_ChunkOp *
lnk_push_chunk_op_end_virt(Arena *arena)
{
  LNK_ChunkOp *end_virt_op = push_array_no_zero(arena, LNK_ChunkOp, 1);
  end_virt_op->next        = 0;
  end_virt_op->type        = LNK_ChunkOp_EndVirt;
  return end_virt_op;
}

internal LNK_ChunkOp *
lnk_push_chunk_op_end_file(Arena *arena)
{
  LNK_ChunkOp *end_op = push_array_no_zero(arena, LNK_ChunkOp, 1);
  end_op->next        = 0;
  end_op->type        = LNK_ChunkOp_End;
  return end_op;
}

internal LNK_ChunkOp *
lnk_push_chunk_op_align(Arena *arena, U64 align, U64 val)
{
  LNK_ChunkOp *align_op = push_array_no_zero(arena, LNK_ChunkOp, 1);
  align_op->next        = 0;
  align_op->type        = LNK_ChunkOp_Align;
  align_op->u.align.x   = align;
  align_op->u.align.val = val;
  return align_op;
}

internal LNK_ChunkOp *
lnk_push_chunk_op_write(Arena *arena, String8 string)
{
  LNK_ChunkOp *write_op = push_array_no_zero(arena, LNK_ChunkOp, 1);
  write_op->next        = 0;
  write_op->type        = LNK_ChunkOp_WriteString;
  write_op->u.string    = string;
  return write_op;
}

internal LNK_ChunkOpList
lnk_op_list_from_chunk(Arena *arena, LNK_Chunk *root, U64 total_chunk_count, U8 align_byte)
{
  struct Stack {
    struct Stack  *next;
    LNK_ChunkArray chunk_array;
    U64            ichunk;
  };

  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  // setup stack
  struct Stack *stack      = push_array(scratch.arena, struct Stack, 1);
  stack->chunk_array.count = 1;
  stack->chunk_array.v     = &root;
  
  // setup output list
  LNK_ChunkOpList list   = {0};
  list.total_chunk_count = total_chunk_count;
  list.first             = list.last = 0;
  
  // write null
  LNK_ChunkOp *null_begin_op    = lnk_push_chunk_op_begin(arena, 0);
  LNK_ChunkOp *null_end_virt_op = lnk_push_chunk_op_end_virt(arena);
  LNK_ChunkOp *null_end_file_op = lnk_push_chunk_op_end_file(arena);;
  lnk_chunk_op_list_push_node(&list, null_begin_op);
  lnk_chunk_op_list_push_node(&list, null_end_virt_op);
  lnk_chunk_op_list_push_node(&list, null_end_file_op);
  
  // traverse chunks from root
  while (stack) {
    while (stack->ichunk < stack->chunk_array.count) {
      LNK_Chunk *chunk = stack->chunk_array.v[stack->ichunk++];
      
      // skip unused chunks
      if (lnk_chunk_is_discarded(chunk)) {
        continue;
      }
      
      switch (chunk->type) {
      case LNK_Chunk_Leaf: {
        // align start in its own begin/end block so align bytes don't contribute to chunk size
        LNK_ChunkOp *pad_begin_op    = lnk_push_chunk_op_begin(arena, list.total_chunk_count++);
        LNK_ChunkOp *pad_align_op    = lnk_push_chunk_op_align(arena, chunk->align, align_byte);
        LNK_ChunkOp *pad_end_file_op = lnk_push_chunk_op_end_file(arena);
        lnk_chunk_op_list_push_node(&list, pad_begin_op);
        lnk_chunk_op_list_push_node(&list, pad_align_op);
        lnk_chunk_op_list_push_node(&list, pad_end_file_op);
        
        // write leaf
        LNK_ChunkOp *leaf_begin_op    = lnk_push_chunk_op_begin(arena, chunk->ref.chunk_id);
        LNK_ChunkOp *leaf_write_op    = lnk_push_chunk_op_write(arena, chunk->u.leaf);
        LNK_ChunkOp *leaf_align_op    = lnk_push_chunk_op_align(arena, chunk->align, align_byte);
        LNK_ChunkOp *leaf_end_virt_op = lnk_push_chunk_op_end_virt(arena);
        LNK_ChunkOp *leaf_end_file_op = lnk_push_chunk_op_end_file(arena);
        #if LNK_DUMP_CHUNK_LAYOUT
        leaf_write_op->chunk = chunk;
        #endif
        lnk_chunk_op_list_push_node(&list, leaf_begin_op);
        lnk_chunk_op_list_push_node(&list, leaf_write_op);
        lnk_chunk_op_list_push_node(&list, leaf_align_op);
        lnk_chunk_op_list_push_node(&list, leaf_end_virt_op);
        lnk_chunk_op_list_push_node(&list, leaf_end_file_op);
      } break;

      case LNK_Chunk_LeafArray: {
        LNK_ChunkOp *begin_op = lnk_push_chunk_op_begin(arena, chunk->ref.chunk_id);
        LNK_ChunkOp *align_op = lnk_push_chunk_op_align(arena, chunk->align, align_byte);
        lnk_chunk_op_list_push_node(&list, begin_op);
        lnk_chunk_op_list_push_node(&list, align_op);

        if (chunk->sort_chunk) {
          lnk_chunk_array_sort(*chunk->u.arr);
        }

        struct Stack *frame = push_array_no_zero(scratch.arena, struct Stack, 1);
        frame->next         = 0;
        frame->chunk_array  = *chunk->u.arr;
        frame->ichunk       = 0;
        SLLStackPush(stack, frame);
      } goto _continue;
      
      case LNK_Chunk_List: { 
        // balance ops at :end_chunk_series
        LNK_ChunkOp *begin_op = lnk_push_chunk_op_begin(arena, chunk->ref.chunk_id);
        LNK_ChunkOp *align_op = lnk_push_chunk_op_align(arena, chunk->align, align_byte);
        lnk_chunk_op_list_push_node(&list, begin_op);
        lnk_chunk_op_list_push_node(&list, align_op);

        // chunk list -> chunk array
        LNK_ChunkArray chunk_array = {0};
        chunk_array.v              = push_array_no_zero(scratch.arena, LNK_ChunkPtr, chunk->u.list->count);
        for (LNK_ChunkNode *cptr = chunk->u.list->first; cptr != 0; cptr = cptr->next) {
          chunk_array.v[chunk_array.count++] = cptr->data;
        }
        
        if (chunk->sort_chunk) {
          lnk_chunk_array_sort(chunk_array);
        }
        
        // recurse into list chunk
        struct Stack *frame = push_array_no_zero(scratch.arena, struct Stack, 1);
        frame->next         = 0;
        frame->chunk_array  = chunk_array;
        frame->ichunk       = 0;
        SLLStackPush(stack, frame);
      } goto _continue;
      
      case LNK_Chunk_Null: { /* ignore */ } break;
      }
    }
    
    // terminate series
    if (stack->next) {
      struct Stack *prev = stack->next;
      Assert(prev->ichunk > 0);

      // :end_chunk_series
      LNK_ChunkOp *end_virt_op = lnk_push_chunk_op_end_virt(arena);
      LNK_ChunkOp *align_op    = lnk_push_chunk_op_align(arena, prev->chunk_array.v[prev->ichunk - 1]->align, align_byte);
      LNK_ChunkOp *end_op      = lnk_push_chunk_op_end_file(arena);
      lnk_chunk_op_list_push_node(&list, end_virt_op);
      lnk_chunk_op_list_push_node(&list, align_op);
      lnk_chunk_op_list_push_node(&list, end_op);
    }
    
    // move to next frame
    SLLStackPop(stack);
    
    _continue:;
  }
  
  scratch_end(scratch);
  ProfEnd();
  return list;
}

internal LNK_ChunkLayout
lnk_chunk_layout_from_op_list(Arena *arena, LNK_ChunkOpList op_list, B32 is_data_inited)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  // setup stack
  struct Stack {
    struct Stack *next;
    U64           chunk_id;
    U64           cursor;
  } *stack = 0;
  
  // setup state
  U64 cursor = 0;
  String8List data_list = {0};
  
  // setup output
  U64 *chunk_off_array       = push_array_no_zero(arena, U64, op_list.total_chunk_count);
  U64 *chunk_file_size_array = push_array_no_zero(arena, U64, op_list.total_chunk_count);
  U64 *chunk_virt_size_array = push_array_no_zero(arena, U64, op_list.total_chunk_count);
  
  // debug stomp so discarded chunks map to invalid offset
#if LNK_PARANOID
  MemorySet(chunk_off_array, 0xFF, sizeof(*chunk_off_array) * op_list.total_chunk_count);
  MemorySet(chunk_file_size_array, 0xFF, sizeof(*chunk_file_size_array) * op_list.total_chunk_count);
  MemorySet(chunk_virt_size_array, 0xFF, sizeof(*chunk_virt_size_array) * op_list.total_chunk_count);
#endif
  
  // execute opcodes
  for (LNK_ChunkOp *op = op_list.first; op != NULL; op = op->next) {
    switch (op->type) {
    case LNK_ChunkOp_Null: break;
    case LNK_ChunkOp_Begin: {
      struct Stack *frame = push_array(scratch.arena, struct Stack, 1);
      frame->chunk_id     = op->u.chunk_id;
      frame->cursor       = cursor;
      SLLStackPush(stack, frame);
      chunk_off_array[stack->chunk_id] = stack->cursor;
    } break;
    case LNK_ChunkOp_End: {
      chunk_file_size_array[stack->chunk_id] = cursor - stack->cursor;
      SLLStackPop(stack);
    } break;
    case LNK_ChunkOp_EndVirt: {
      chunk_virt_size_array[stack->chunk_id] = cursor - stack->cursor;
    } break;
    case LNK_ChunkOp_Align: {
      Assert(IsPow2(op->u.align.x));
      U64 size = AlignPow2(cursor, op->u.align.x) - cursor;
      
      String8 string;
      string.size = size;
      string.str  = push_array_no_zero(arena, U8, string.size);
      MemorySet(string.str, op->u.align.val, string.size);
      
      op->type     = LNK_ChunkOp_WriteString;
      op->u.string = string;
    } // fall-through
    case LNK_ChunkOp_WriteString: {
      if (is_data_inited) {
        // we allow chunks to have null for str for regions in the image that are zeroed out.
        if (op->u.string.str == 0) {
          op->u.string.str = push_array(arena, U8, op->u.string.size);
        }
        str8_list_push(scratch.arena, &data_list, op->u.string);
      }
#if LNK_DUMP_CHUNK_LAYOUT
      if (op->chunk) {
        fprintf(g_layout_file, "[%.*s] %llX %.*s\n", str8_varg(op->chunk->sort_idx), op->chunk->input_idx, str8_varg(op->chunk->debug));
      }
#endif
      // advance
      cursor += op->u.string.size;
    } break;
    }
  }
  
  // are begin/end series opcodes balanced?
  Assert(stack == 0);
  
  // fill out result
  LNK_ChunkLayout layout       = {0};
  layout.data                  = str8_list_join(arena, &data_list, 0);
  layout.chunk_off_array       = chunk_off_array;
  layout.chunk_file_size_array = chunk_file_size_array;
  layout.chunk_virt_size_array = chunk_virt_size_array;

  scratch_end(scratch);
  ProfEnd();
  return layout;
}

internal LNK_ChunkLayout
lnk_build_chunk_layout(Arena *arena, LNK_ChunkManager *cman, COFF_SectionFlags flags, U8 align_byte)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  // should we write data for chunks?
  B32 is_data_inited = !!(~flags & COFF_SectionFlag_CNT_UNINITIALIZED_DATA);
  
  // build layout
  LNK_ChunkOpList op_list = lnk_op_list_from_chunk(scratch.arena, cman->root, cman->total_chunk_count, align_byte);
  LNK_ChunkLayout layout = lnk_chunk_layout_from_op_list(arena, op_list, is_data_inited);

  scratch_end(scratch);
  ProfEnd();
  return layout;
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
  LNK_ChunkPtr *chunk_id_map = push_array_no_zero(arena, LNK_ChunkPtr, cman->total_chunk_count + 1);
  lnk_visit_chunks(0, cman->root, lnk_save_chunk_ptr, chunk_id_map);

  LNK_Chunk *null_chunk    = push_array(arena, LNK_Chunk, 1);
  null_chunk->is_discarded = 1;

  chunk_id_map[0] = null_chunk;

  return chunk_id_map;
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

