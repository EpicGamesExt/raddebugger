// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

#define MSF_PAGE_STATE_FREE  1
#define MSF_PAGE_STATE_ALLOC 0
#define MSF_FPM0 1
#define MSF_FPM1 2
#define MSF_DEFAULT_PAGE_SIZE 4096
#define MSF_DEFAULT_FPM MSF_FPM0

typedef struct MSF_PageNode MSF_PageNode;
struct MSF_PageNode
{
  MSF_PageNode   *next;
  MSF_PageNode   *prev;
  MSF_PageNumber  pn;
};

typedef struct MSF_PageList
{
  MSF_PageNode *first;
  MSF_PageNode *last;
  MSF_UInt      count;
} MSF_PageList;

typedef struct MSF_Stream
{
  MSF_StreamNumber  sn;
  MSF_UInt          size;
  MSF_UInt          pos;
  U64               capacity;
  U8               *data;
  MSF_PageList      page_list;
} MSF_Stream;

typedef struct MSF_StreamNode MSF_StreamNode;
struct MSF_StreamNode
{
  MSF_StreamNode *next;
  MSF_StreamNode *prev;
  MSF_Stream      data;
};

typedef struct MSF_StreamList
{
  MSF_UInt        count;
  MSF_StreamNode *first;
  MSF_StreamNode *last;
} MSF_StreamList;

typedef struct MSF_PageBlock MSF_PageBlock;
struct MSF_PageBlock
{
  MSF_PageBlock  *next;
  U32            *free_bits;
  U8             *fpm_data;
  U8            **data;
};

typedef struct MSF_Context
{
  Arena           *arena;
  MSF_UInt         page_size;
  MSF_UInt         active_fpm;
  MSF_PageNumber   fpm_rover;
  MSF_UInt         block_count;
  MSF_PageBlock   *first_block;
  MSF_PageBlock   *last_block;
  U8              *zero_page;
  U8              *empty_fpm;
  MSF_PageList     page_pool;
  MSF_Stream       header;
  MSF_Stream       root;
  MSF_Stream       stream_table;
  MSF_StreamList   sectab;
  MSF_StreamNode **stream_from_sn;
} MSF_Context;

typedef enum MSF_Error
{
  MSF_Error_Ok,
  MSF_Error_StreamTableHasTooManyPages,
  MSF_Error_FailedToWriteStreamTable,
  MSF_Error_FailedToWriteHeader,
} MSF_Error;

typedef struct MSF_WriteTask
{
  U8      *dst;
  U8      *src;
  Rng1U64 *ranges;
} MSF_WriteTask;

internal MSF_Context *msf_alloc(MSF_UInt page_size, MSF_UInt active_fpm);
internal void         msf_release(MSF_Context *msf);
internal MSF_Error    msf_build(MSF_Context *msf);
internal U64          msf_get_save_size(MSF_Context *msf);

// File-order spans borrow stream/metadata storage and remain valid until it
// is modified or released. No second copy of the stream payloads is made.
internal String8List     msf_get_page_data_nodes(Arena *arena, MSF_Context *msf);
internal String8         msf_data_from_pn(MSF_Context *msf, MSF_PageNumber pn);
internal B32             msf_save(MSF_Context *msf, void *buffer, U64 buffer_size);
internal MSF_Error       msf_save_arena(Arena *arena, MSF_Context *msf, String8 *data_out);
internal MSF_StreamNode *msf_find_stream_node(MSF_Context *msf, MSF_StreamNumber sn);
internal MSF_Stream     *msf_find_stream(MSF_Context *msf, MSF_StreamNumber sn);
internal MSF_PageList    msf_alloc_pages(MSF_Context *msf, MSF_UInt count);
internal void            msf_free_pages(MSF_Context *msf, MSF_PageList *pages);

internal MSF_StreamNumber msf_stream_alloc_ex(MSF_Context *msf, MSF_UInt size);
internal MSF_StreamNumber msf_stream_alloc(MSF_Context *msf);
internal B32              msf_stream_free(MSF_Context *msf, MSF_StreamNumber sn);
internal B32              msf_stream_resize(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt size);
internal B32              msf_stream_resize_ex(MSF_Context *msf, MSF_Stream *stream, MSF_UInt size);
internal MSF_UInt         msf_stream_get_size(MSF_Context *msf, MSF_StreamNumber sn);
internal MSF_UInt         msf_stream_get_pos(MSF_Context *msf, MSF_StreamNumber sn);
internal B32              msf_stream_reserve(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt size);
internal B32              msf_stream_seek(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt pos);
internal B32              msf_stream_seek_start(MSF_Context *msf, MSF_StreamNumber sn);
internal void             msf_stream_align(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt align);

internal String8 msf_stream_data(MSF_Context *msf, MSF_StreamNumber sn);
// Writable, contiguous construction storage. Reserve/resize before taking a
// view: growth beyond capacity invalidates views, but preserves page numbers.
// Finalized streams must not be changed while asynchronous output borrows them.

internal B32 msf_stream_write(MSF_Context *msf, MSF_StreamNumber sn, void *buffer, MSF_UInt size);
internal B32 msf_stream_write_string(MSF_Context *msf, MSF_StreamNumber sn, String8 string);
internal B32 msf_stream_write_list(MSF_Context *msf, MSF_StreamNumber sn, String8List list);
internal B32 msf_stream_write_cstr(MSF_Context *msf, MSF_StreamNumber sn, String8 string);
internal B32 msf_stream_write_u8(MSF_Context *msf, MSF_StreamNumber sn, U8 value);
internal B32 msf_stream_write_u16(MSF_Context *msf, MSF_StreamNumber sn, U16 value);
internal B32 msf_stream_write_u32(MSF_Context *msf, MSF_StreamNumber sn, U32 value);
internal B32 msf_stream_write_u64(MSF_Context *msf, MSF_StreamNumber sn, U64 value);
internal B32 msf_stream_write_s8(MSF_Context *msf, MSF_StreamNumber sn, S8 value);
internal B32 msf_stream_write_s16(MSF_Context *msf, MSF_StreamNumber sn, S16 value);
internal B32 msf_stream_write_s32(MSF_Context *msf, MSF_StreamNumber sn, S32 value);
internal B32 msf_stream_write_s64(MSF_Context *msf, MSF_StreamNumber sn, S64 value);
internal B32 msf_stream_write_parallel(TP_Context *tp, MSF_Context *msf, MSF_StreamNumber sn, void *buffer, MSF_UInt size);
#define msf_stream_write_array(m, s, v, c) msf_stream_write(m, s, (void *)(v), sizeof(*(v)) * (c))
#define msf_stream_write_struct(m, s, v)   msf_stream_write_array(m, s, v, 1)

internal MSF_UInt  msf_count_pages(MSF_UInt page_size, U64 data_size);
internal char     *msf_error_to_string(MSF_Error code);

