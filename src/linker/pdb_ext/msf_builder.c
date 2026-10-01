// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal MSF_UInt
msf_count_pages(MSF_UInt page_size, U64 data_size)
{
  return safe_cast_u32(CeilIntegerDiv(data_size, page_size));
}

internal MSF_PageBlock *
msf_block_from_index(MSF_Context *msf, U64 idx)
{
  MSF_PageBlock *block = msf->first_block;
  for (U64 i = 0; i < idx && block != 0; i += 1) { block = block->next; }
  Assert(block != 0);
  return block;
}

internal U8 **
msf_page_data_slot(MSF_Context *msf, MSF_PageNumber pn)
{
  U64 interval = (U64)msf->page_size * MSF_BITS_PER_CHAR;
  MSF_PageBlock *block = msf_block_from_index(msf, pn / interval);
  return &block->data[pn % interval];
}

internal String8
msf_data_from_pn(MSF_Context *msf, MSF_PageNumber pn)
{
  U8 *data = *msf_page_data_slot(msf, pn);
  return str8(data != 0 ? data : msf->zero_page, msf->page_size);
}

internal B32
msf_grow_page_table(MSF_Context *msf)
{
  U64 interval = (U64)msf->page_size * MSF_BITS_PER_CHAR;
  if (((U64)msf->block_count + 1) * interval > MSF_PN_MAX) { return 0; }

  MSF_PageBlock *block = push_array(msf->arena, MSF_PageBlock, 1);
  block->free_bits = push_array_no_zero(msf->arena, U32, msf->page_size / sizeof(U32));
  block->fpm_data  = push_array_no_zero(msf->arena, U8, msf->page_size);
  block->data      = push_array(msf->arena, U8 *, interval);
  MemorySet(block->free_bits, 0xff, msf->page_size);
  SLLQueuePush(msf->first_block, msf->last_block, block);
  U64 block_idx = msf->block_count++;

  // MS readers reserve slots 1 and 2 every page_size pages, even though one
  // FPM page describes page_size*8 pages; keep those slots out of streams
  U32Array bits = { .v = block->free_bits, .count = msf->page_size / sizeof(U32) };
  for (U64 i = 0; i < interval; i += msf->page_size) {
    bit_array_set_bit32(bits, i + MSF_FPM0, MSF_PAGE_STATE_ALLOC);
    bit_array_set_bit32(bits, i + MSF_FPM1, MSF_PAGE_STATE_ALLOC);
    block->data[i + MSF_FPM0] = msf->empty_fpm;
    block->data[i + MSF_FPM1] = msf->empty_fpm;
  }

  // the bitmap pages are spaced page_size pages apart. Both copies describe
  // the same fresh file; this builder does not edit existing files
  *msf_page_data_slot(msf, safe_cast_u32(block_idx * msf->page_size + MSF_FPM0)) = block->fpm_data;
  *msf_page_data_slot(msf, safe_cast_u32(block_idx * msf->page_size + MSF_FPM1)) = block->fpm_data;

  return 1;
}

internal MSF_PageNode *
msf_page_list_pop_last(MSF_PageList *list)
{
  MSF_PageNode *node = list->last;
  if (node) {
    DLLRemove(list->first, list->last, node);
    list->count -= 1;
    node->next = node->prev = 0;
  }
  return node;
}

internal void
msf_page_list_push_node(MSF_PageList *list, MSF_PageNode *node)
{
  DLLPushBack(list->first, list->last, node);
  list->count += 1;
}

internal MSF_PageList
msf_alloc_pages(MSF_Context *msf, MSF_UInt count)
{
  MSF_PageList result = {0};
  U64 interval = (U64)msf->page_size * MSF_BITS_PER_CHAR;
  while (result.count < count) {
    U64 block_idx = msf->fpm_rover / interval;
    if (block_idx == msf->block_count && !msf_grow_page_table(msf)) {
      msf_free_pages(msf, &result);
      break;
    }

    MSF_PageBlock *block = msf_block_from_index(msf, block_idx);
    U32Array       bits  = { .v = block->free_bits, .count = msf->page_size / sizeof(U32) };
    U64            bit   = bit_array_scan_left_to_right32(bits, msf->fpm_rover % interval, interval, MSF_PAGE_STATE_FREE);
    if (bit >= interval) {
      msf->fpm_rover = safe_cast_u32((block_idx + 1) * interval);
      continue;
    }

    MSF_PageNumber pn = safe_cast_u32(block_idx * interval + bit);
    bit_array_set_bit32(bits, bit, MSF_PAGE_STATE_ALLOC);
    
    MSF_PageNode *node = msf_page_list_pop_last(&msf->page_pool);
    if (node == 0) { node = push_array(msf->arena, MSF_PageNode, 1); }

    node->pn = pn;
    msf_page_list_push_node(&result, node);
    msf->fpm_rover = pn + 1;
  }
  return result;
}

internal void
msf_free_pages(MSF_Context *msf, MSF_PageList *pages)
{
  U64 interval = (U64)msf->page_size * MSF_BITS_PER_CHAR;
  for (MSF_PageNode *node; (node = msf_page_list_pop_last(pages)) != 0; ) {
    MSF_PageBlock *block = msf_block_from_index(msf, node->pn / interval);
    U64            bit   = node->pn % interval;
    U32Array       bits  = { .v = block->free_bits, .count = msf->page_size / sizeof(U32) };
    Assert(bit_array_get_bit32(bits, bit) == MSF_PAGE_STATE_ALLOC);
    bit_array_set_bit32(bits, bit, MSF_PAGE_STATE_FREE);
    block->data[bit] = 0;
    msf->fpm_rover = Min(msf->fpm_rover, node->pn);
    msf_page_list_push_node(&msf->page_pool, node);
  }
}

////////////////////////////////

internal B32
msf_stream_reserve_capacity(MSF_Context *msf, MSF_Stream *stream, U64 size)
{
  if (size >= MSF_DELETED_STREAM_STAMP) { return 0; }

  U64      capacity   = AlignPow2(size, (U64)msf->page_size);
  MSF_UInt page_count = msf_count_pages(msf->page_size, size);
  B32      remap      = 0;

  if (page_count > stream->page_list.count) {
    MSF_UInt     count = page_count - stream->page_list.count;
    MSF_PageList pages = msf_alloc_pages(msf, count);
    if (pages.count != count) { return 0; }
    DLLConcatInPlace(&stream->page_list, &pages);
    remap = 1;
  }

  if (capacity > stream->capacity) {
    // sized callers allocate once. Incremental writers must reserve before
    // retaining views; arena growth leaves previous views obsolete
    U8 *data = push_array(msf->arena, U8, capacity);
    if (stream->size != 0) { MemoryCopy(data, stream->data, stream->size); }
    stream->data = data;
    stream->capacity = capacity;
    remap = 1;
  }

  if (remap) {
    U64 offset = 0;
    for EachNode(page, MSF_PageNode, stream->page_list.first) {
      *msf_page_data_slot(msf, page->pn) = stream->data + offset;
      offset += msf->page_size;
    }
  }

  return 1;
}

internal B32
msf_stream_resize_ex(MSF_Context *msf, MSF_Stream *stream, MSF_UInt size)
{
  if (!msf_stream_reserve_capacity(msf, stream, size)) { return 0; }

  MSF_UInt     page_count = msf_count_pages(msf->page_size, size);
  MSF_PageList removed    = {0};

  while (stream->page_list.count > page_count) {
    msf_page_list_push_node(&removed, msf_page_list_pop_last(&stream->page_list));
  }

  msf_free_pages(msf, &removed);

  if (size < stream->size) { MemoryZero(stream->data + size, stream->size - size); }

  stream->size = size;
  stream->pos = Min(stream->pos, size);

  return 1;
}

internal MSF_StreamNode *
msf_find_stream_node(MSF_Context *msf, MSF_StreamNumber sn)
{
  return sn < msf->sectab.count ? msf->stream_from_sn[sn] : 0;
}

internal MSF_Stream *
msf_find_stream(MSF_Context *msf, MSF_StreamNumber sn)
{
  MSF_StreamNode *node = msf_find_stream_node(msf, sn);
  return node != 0 && node->data.size != MSF_DELETED_STREAM_STAMP ? &node->data : 0;
}

internal MSF_StreamNumber
msf_stream_alloc_ex(MSF_Context *msf, MSF_UInt size)
{
  if (msf->sectab.count >= MSF_STREAM_NUMBER_MAX) { return MSF_INVALID_STREAM_NUMBER; }
  MSF_StreamNode *node = push_array(msf->arena, MSF_StreamNode, 1);
  node->data.sn = safe_cast_u16(msf->sectab.count);
  if (!msf_stream_resize_ex(msf, &node->data, size)) { return MSF_INVALID_STREAM_NUMBER; }
  DLLPushBack(msf->sectab.first, msf->sectab.last, node);
  msf->stream_from_sn[node->data.sn] = node;
  msf->sectab.count += 1;
  return node->data.sn;
}

internal MSF_StreamNumber
msf_stream_alloc(MSF_Context *msf)
{
  return msf_stream_alloc_ex(msf, 0);
}

internal B32
msf_stream_free(MSF_Context *msf, MSF_StreamNumber sn)
{
  MSF_Stream *stream = msf_find_stream(msf, sn);
  if (stream == 0) { return 0; }
  msf_free_pages(msf, &stream->page_list);
  stream->size = MSF_DELETED_STREAM_STAMP;
  stream->pos  = 0;
  return 1;
}

internal B32
msf_stream_resize(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt size)
{
  MSF_Stream *stream = msf_find_stream(msf, sn);
  return stream != 0 && msf_stream_resize_ex(msf, stream, size);
}

internal MSF_UInt
msf_stream_get_size(MSF_Context *msf, MSF_StreamNumber sn)
{
  MSF_Stream *stream = msf_find_stream(msf, sn);
  return stream != 0 ? stream->size : MSF_UINT_MAX;
}

internal MSF_UInt
msf_stream_get_pos(MSF_Context *msf, MSF_StreamNumber sn)
{
  MSF_Stream *stream = msf_find_stream(msf, sn);
  return stream != 0 ? stream->pos : MSF_UINT_MAX;
}

internal B32
msf_stream_reserve(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt size)
{
  MSF_Stream *stream = msf_find_stream(msf, sn);
  return stream != 0 && msf_stream_reserve_capacity(msf, stream, (U64)stream->pos + size);
}

internal B32
msf_stream_seek(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt pos)
{
  MSF_Stream *stream = msf_find_stream(msf, sn);
  if (stream == 0) { return 0; }
  stream->pos = Min(pos, stream->size);
  return 1;
}

internal B32
msf_stream_seek_start(MSF_Context *msf, MSF_StreamNumber sn)
{
  return msf_stream_seek(msf, sn, 0);
}

internal String8
msf_stream_data(MSF_Context *msf, MSF_StreamNumber sn)
{
  MSF_Stream *stream = msf_find_stream(msf, sn);
  return stream != 0 ? str8(stream->data, stream->size) : str8_zero();
}

internal B32
msf_stream_write(MSF_Context *msf, MSF_StreamNumber sn, void *buffer, MSF_UInt size)
{
  MSF_Stream *stream = msf_find_stream(msf, sn);
  if (stream == 0) { return 0; }

  U64 end = (U64)stream->pos + size;
  if (end >= MSF_DELETED_STREAM_STAMP) { return 0; }

  if (end > stream->capacity || msf_count_pages(msf->page_size, end) > stream->page_list.count) {
    if (!msf_stream_reserve_capacity(msf, stream, end)) { return 0; }
  }

  if (size != 0) {
    if (buffer != 0) { MemoryCopy(stream->data + stream->pos, buffer, size); }
    else             { MemoryZero(stream->data + stream->pos, size);         }
  }

  stream->pos  = safe_cast_u32(end);
  stream->size = Max(stream->size, stream->pos);

  return 1;
}

internal B32
msf_stream_write_string(MSF_Context *msf, MSF_StreamNumber sn, String8 string)
{
  return msf_stream_write(msf, sn, string.str, safe_cast_u32(string.size));
}

internal B32
msf_stream_write_list(MSF_Context *msf, MSF_StreamNumber sn, String8List list)
{
  if (!msf_stream_reserve(msf, sn, safe_cast_u32(list.total_size))) { return 0; }

  for EachNode(node, String8Node, list.first) {
    if (!msf_stream_write_string(msf, sn, node->string)) { return 0; }
  }

  return 1;
}

internal B32
msf_stream_write_cstr(MSF_Context *msf, MSF_StreamNumber sn, String8 string)
{
  return msf_stream_write_string(msf, sn, string) && msf_stream_write_u8(msf, sn, 0);
}

#define MSF_WRITE_SCALAR(T, suffix) \
internal B32 msf_stream_write_##suffix(MSF_Context *msf, MSF_StreamNumber sn, T value) \
{ return msf_stream_write(msf, sn, &value, sizeof(value)); }
MSF_WRITE_SCALAR(U8, u8)
MSF_WRITE_SCALAR(U16, u16)
MSF_WRITE_SCALAR(U32, u32)
MSF_WRITE_SCALAR(U64, u64)
MSF_WRITE_SCALAR(S8, s8)
MSF_WRITE_SCALAR(S16, s16)
MSF_WRITE_SCALAR(S32, s32)
MSF_WRITE_SCALAR(S64, s64)
#undef MSF_WRITE_SCALAR

internal void
msf_stream_align(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt align)
{
  MSF_UInt pos = msf_stream_get_pos(msf, sn);
  msf_stream_write(msf, sn, 0, AlignPadPow2(pos, align));
}

internal
THREAD_POOL_TASK_FUNC(msf_write_task)
{
  MSF_WriteTask *task = raw_task;
  Rng1U64 range = task->ranges[task_id];
  if (dim_1u64(range) != 0) { MemoryCopy(task->dst + range.min, task->src + range.min, dim_1u64(range)); }
}

internal B32
msf_stream_write_parallel(TP_Context *tp, MSF_Context *msf, MSF_StreamNumber sn, void *buffer, MSF_UInt size)
{
  if (buffer == 0 || size < KB(64)) { return msf_stream_write(msf, sn, buffer, size); }

  if (!msf_stream_reserve(msf, sn, size)) { return 0; }

  Temp scratch = scratch_begin(0, 0);

  MSF_Stream *stream = msf_find_stream(msf, sn);
  MSF_WriteTask task = {
    .dst    = stream->data + stream->pos,
    .src    = buffer,
    .ranges = tp_divide_work(scratch.arena, size, tp->worker_count),
  };

  tp_for_parallel(tp, 0, tp->worker_count, msf_write_task, &task);

  stream->pos += size;
  stream->size = Max(stream->size, stream->pos);

  scratch_end(scratch);
  return 1;
}

////////////////////////////////

internal MSF_Context *
msf_alloc_(Arena *arena, MSF_UInt page_size, MSF_UInt active_fpm)
{
  Assert(IsPow2(page_size) && MSF_MIN_PAGE_SIZE <= page_size && page_size <= MSF_MAX_PAGE_SIZE);
  Assert(active_fpm == MSF_FPM0 || active_fpm == MSF_FPM1);

  MSF_Context *msf = push_array(arena, MSF_Context, 1);
  msf->arena          = arena;
  msf->page_size      = page_size;
  msf->active_fpm     = active_fpm;
  msf->stream_from_sn = push_array(arena, MSF_StreamNode *, MSF_STREAM_NUMBER_MAX);
  msf->zero_page      = push_array(arena, U8, page_size);
  msf->empty_fpm      = push_array_no_zero(arena, U8, page_size);
  MemorySet(msf->empty_fpm, 0xff, page_size);

  AssertAlways(msf_stream_resize_ex(msf, &msf->header, page_size));
  AssertAlways(msf_stream_resize_ex(msf, &msf->root, page_size));
  Assert(msf->header.page_list.first->pn == 0);
  Assert(msf->root.page_list.first->pn == 3);

  return msf;
}

internal MSF_Context *
msf_alloc(MSF_UInt page_size, MSF_UInt active_fpm)
{
  return msf_alloc_(arena_alloc(.name = "MSF"), page_size, active_fpm);
}

internal void
msf_release(MSF_Context *msf)
{
  arena_release(msf->arena);
}

internal U64
msf_get_save_size(MSF_Context *msf)
{
  U64 interval = (U64)msf->page_size * MSF_BITS_PER_CHAR;
  U64 max_pn   = MSF_FPM1;
  U64 base     = 0;

  for EachNode(block, MSF_PageBlock, msf->first_block) {
    U32Array bits = { .v = block->free_bits, .count = msf->page_size / sizeof(U32) };
    U64      hi   = interval;
    while (hi != 0) {
      U64 bit = bit_array_scan_right_to_left32(bits, 0, hi, MSF_PAGE_STATE_ALLOC);
      if (bit >= hi) { break; }
      U64 slot = bit % msf->page_size;
      if (slot != MSF_FPM0 && slot != MSF_FPM1) {
        max_pn = Max(max_pn, base + bit);
        break;
      }
      hi = bit;
    }
    base += interval;
  }

  return (max_pn + 1) * msf->page_size;
}

internal MSF_Error
msf_build(MSF_Context *msf)
{
  ProfBeginFunction();

  U64 table_size = sizeof(MSF_UInt) * (1 + (U64)msf->sectab.count);
  for EachNode(node, MSF_StreamNode, msf->sectab.first) {
    if (node->data.size != MSF_DELETED_STREAM_STAMP) {
      table_size += sizeof(MSF_PageNumber) * (U64)msf_count_pages(msf->page_size, node->data.size);
    }
  }

  U64 table_pages = CeilIntegerDiv(table_size, msf->page_size);
  if (table_pages > msf->page_size / sizeof(MSF_PageNumber)) {
    ProfEnd();
    return MSF_Error_StreamTableHasTooManyPages;
  }

  if (!msf_stream_resize_ex(msf, &msf->stream_table, safe_cast_u32(table_size))) {
    ProfEnd();
    return MSF_Error_FailedToWriteStreamTable;
  }

  U32 *sizes = (U32 *)msf->stream_table.data;
  *sizes++ = msf->sectab.count;

  MSF_PageNumber *pages = (MSF_PageNumber *)(sizes + msf->sectab.count);
  for EachNode(node, MSF_StreamNode, msf->sectab.first) {
    MSF_Stream *stream = &node->data;
    sizes[stream->sn] = stream->size;
    if (stream->size == MSF_DELETED_STREAM_STAMP) { continue; }
    MSF_UInt count = msf_count_pages(msf->page_size, stream->size);
    Assert(count <= stream->page_list.count);
    MSF_PageNode *page = stream->page_list.first;
    for EachIndex(i, count) {
      *pages++ = page->pn;
      page = page->next;
    }
  }

  // keep the root's allocation-time page number, including on repeated builds
  MemoryZero(msf->root.data, msf->root.capacity);
  MSF_PageNumber *root = (MSF_PageNumber *)msf->root.data;
  for EachNode(page, MSF_PageNode, msf->stream_table.page_list.first) { *root++ = page->pn; }

  MSF_Header70 header = {0};
  MemoryCopy(&header.magic[0], &msf_msf70_magic[0], sizeof(header.magic));
  header.page_size         = msf->page_size;
  header.active_fpm        = msf->active_fpm;
  header.page_count        = safe_cast_u32(msf_get_save_size(msf) / msf->page_size);
  header.stream_table_size = safe_cast_u32(table_size);
  header.root_pn           = msf->root.page_list.first->pn;
  MemoryCopy(msf->header.data, &header, sizeof(header));

  // allocation reserves future FPM slots too, but on disk all pages beyond
  // EOF must be free; MS readers validate those trailing bits
  U64 interval = (U64)msf->page_size * MSF_BITS_PER_CHAR;
  U64 base     = 0;
  for EachNode(block, MSF_PageBlock, msf->first_block) {
    MemoryCopy(block->fpm_data, block->free_bits, msf->page_size);
    U64 used_bits = header.page_count > base ? Min((U64)header.page_count - base, interval) : 0;
    U64 byte = used_bits / 8;
    if (used_bits % 8) {
      block->fpm_data[byte++] |= (U8)(0xff << (used_bits % 8));
    }
    MemorySet(block->fpm_data + byte, 0xff, msf->page_size - byte);
    base += interval;
  }

  ProfEnd();
  return MSF_Error_Ok;
}

////////////////////////////////

internal String8List
msf_get_page_data_nodes(Arena *arena, MSF_Context *msf)
{
  String8List result = {0};

  U64 page_count = msf_get_save_size(msf) / msf->page_size;
  U64 interval   = (U64)msf->page_size * MSF_BITS_PER_CHAR;
  U64 pn         = 0;

  for EachNode(block, MSF_PageBlock, msf->first_block) {
    for (U64 i = 0; i < interval && pn < page_count; i += 1, pn += 1) {
      U8 *data = block->data[i] != 0 ? block->data[i] : msf->zero_page;
      if (result.last != 0 && result.last->string.str + result.last->string.size == data) {
        result.last->string.size += msf->page_size;
        result.total_size        += msf->page_size;
      } else {
        str8_list_push(arena, &result, str8(data, msf->page_size));
      }
    }
  }

  return result;
}

internal B32
msf_save(MSF_Context *msf, void *buffer, U64 buffer_size)
{
  if (buffer_size != msf_get_save_size(msf)) { return 0; }

  Temp scratch = scratch_begin(0, 0);
  String8List spans = msf_get_page_data_nodes(scratch.arena, msf);
  U64         pos   = 0;
  for EachNode(node, String8Node, spans.first) {
    MemoryCopy((U8 *)buffer + pos, node->string.str, node->string.size);
    pos += node->string.size;
  }

  scratch_end(scratch);
  return pos == buffer_size;
}

internal MSF_Error
msf_save_arena(Arena *arena, MSF_Context *msf, String8 *data_out)
{
  MSF_Error error = msf_build(msf);
  if (error == MSF_Error_Ok) {
    U64  size = msf_get_save_size(msf);
    U8  *data = push_array_no_zero(arena, U8, size);
    if (!msf_save(msf, data, size)) { return MSF_Error_FailedToWriteHeader; }
    *data_out = str8(data, size);
  }
  return error;
}

internal char *
msf_error_to_string(MSF_Error code)
{
  switch (code) {
  case MSF_Error_Ok: return "";
  case MSF_Error_StreamTableHasTooManyPages: return "stream table exceeds page limit";
  case MSF_Error_FailedToWriteStreamTable:   return "failed to write stream table";
  case MSF_Error_FailedToWriteHeader:        return "failed to write header";
  }
  return "unknown MSF error";
}

