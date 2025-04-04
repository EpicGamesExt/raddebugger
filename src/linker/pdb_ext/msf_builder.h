// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

#define MSF_PAGE_STATE_FREE  1
#define MSF_PAGE_STATE_ALLOC 0

#define MSF_FPM0 1
#define MSF_FPM1 2

#define MSF_DEFAULT_PAGE_SIZE 4096
#define MSF_DEFAULT_FPM MSF_FPM0

typedef struct MSF_PageNumberArray
{
  U64             count;
  MSF_PageNumber *v;
} MSF_PageNumberArray;

typedef struct MSF_PageNode
{
  struct MSF_PageNode *next;
  struct MSF_PageNode *prev;
  MSF_PageNumber       pn;
} MSF_PageNode;

typedef struct MSF_PageList
{
  MSF_PageNode *first;
  MSF_PageNode *last;
  MSF_UInt      count;
} MSF_PageList;

typedef struct MSF_Stream
{
  MSF_StreamNumber sn;
  MSF_UInt         size;
  MSF_UInt         pos;
  MSF_PageNode    *pos_page;
  MSF_PageList     page_list;
} MSF_Stream;

typedef struct MSF_StreamNode
{
  struct MSF_StreamNode *next;
  struct MSF_StreamNode *prev;
  MSF_Stream             data;
} MSF_StreamNode;

typedef struct MSF_StreamList
{
  MSF_UInt        count;
  MSF_StreamNode *first;
  MSF_StreamNode *last;
} MSF_StreamList;

typedef struct MSF_PageDataNode
{
  struct MSF_PageDataNode *next;
  struct MSF_PageDataNode *prev;
  U8                      *data;
} MSF_PageDataNode;

typedef struct MSF_PageDataList
{
  MSF_PageDataNode *first;
  MSF_PageDataNode *last;
  MSF_UInt          count;
} MSF_PageDataList;

typedef struct MSF_Context
{
  Arena           *arena;
  MSF_UInt         page_size;
  MSF_UInt         active_fpm;
  MSF_UInt         fpm_rover;
  MSF_PageNumber   page_count;
  MSF_PageDataList page_data_list;
  MSF_PageDataList page_data_pool;
  MSF_PageList     header_page_list;
  MSF_PageList     root_page_list;
  MSF_PageList     st_page_list;
  MSF_PageList     page_pool;
  MSF_StreamList   sectab;
} MSF_Context;

typedef enum MSF_Error
{
  MSF_Error_OK,
  
  // if you get this error this means stream table was divided into too many
  // pages, and to fix this you need to bump up the page size
  MSF_Error_STREAM_TABLE_HAS_TOO_MANY_PAGES,
  
  MSF_OpenError_NOT_ENOUGH_BYTES_TO_READ_HEADER,
  MSF_OpenError_INVALID_MAGIC,
  MSF_OpenError_PAGE_SIZE_IS_NOT_POW2,
  MSF_OpenError_INVALID_PAGE_SIZE,
  MSF_OpenError_NOT_ENOUGH_PAGES_TO_INIT,
  MSF_OpenError_INVALID_ROOT_STREAM_PAGE_NUMBER,
  MSF_OpenError_UNABLE_TO_READ_STREAM_TABLE_PAGE_NUMBERS,
  MSF_OpenError_STREAM_COUNT_OVERFLOW,
  MSF_OpenError_UNABLE_TO_READ_STREAM_SIZES,
  MSF_OpenError_INVALID_STREAM_TABLE,
  MSF_OpenError_INVALID_ACTIVE_FPM,
  MSF_OpenError_PAGE_COUNT_DOESNT_MATCH_DATA_SIZE,
  
  MSF_BuildError_UNABLE_TO_WRITE_STREAM_TABLE,
  MSF_BuildError_UNABLE_TO_WRITE_STREAM_TABLE_PAGE_NUMBER_DIRECTORY,
  MSF_BuildError_UNABLE_TO_WRITE_ROOT_DIRECTORY,
  MSF_BuildError_UNABLE_TO_WRITE_HEADER,
} MSF_Error;

////////////////////////////////

typedef struct
{
  MSF_UInt         page_size;
  MSF_PageDataList page_data_list;
  MSF_PageList     page_list;
  MSF_UInt         stream_pos;
  String8          data;
  Rng1U64         *range_arr;
} MSF_WriteTask;

////////////////////////////////

internal MSF_Context *    msf_alloc(MSF_UInt page_size, MSF_UInt active_fpm);
internal MSF_Error        msf_open(String8 data, MSF_Context **msf_out);
internal void             msf_release(MSF_Context **msf_ptr);
internal MSF_Error        msf_build(MSF_Context *msf);
internal U64              msf_get_save_size(MSF_Context *msf);
internal String8List      msf_get_page_data_nodes(Arena *arena, MSF_Context *msf);
internal B32              msf_save(MSF_Context *msf, void *buffer, U64 buffer_size);
internal MSF_Error        msf_save_arena(Arena *arena, MSF_Context *msf, String8 *data_out);
internal MSF_StreamNode * msf_find_stream_node(MSF_Context *msf, MSF_StreamNumber sn);
internal MSF_Stream *     msf_find_stream(MSF_Context *msf, MSF_StreamNumber sn);
internal B32              msf_grow(MSF_Context *msf, MSF_PageNumber page_count);
internal MSF_PageNumber * msf_alloc_pn_arr(Arena *arena, MSF_Context *msf, MSF_UInt alloc_count);
internal void             msf_free_pn_arr(MSF_Context *msf, MSF_PageNumber *pn_arr, MSF_UInt pn_count);
internal MSF_PageList     msf_alloc_pages(MSF_Context *msf, MSF_UInt alloc_count);
internal void             msf_free_pages(MSF_Context *msf, MSF_PageList *page_list);

internal MSF_StreamNumber msf_stream_alloc_ex(MSF_Context *msf, MSF_UInt size);
internal MSF_StreamNumber msf_stream_alloc(MSF_Context *msf);
internal B32              msf_stream_free(MSF_Context *msf, MSF_StreamNumber sn);
internal B32              msf_stream_resize(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt new_size);
internal B32              msf_stream_resize_ex(MSF_Context *msf, MSF_Stream *stream, MSF_UInt size);
internal void             msf_stream_set_size(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt size);
internal MSF_UInt         msf_stream_get_size(MSF_Context *msf, MSF_StreamNumber sn);
internal MSF_UInt         msf_stream_get_cap(MSF_Context *msf, MSF_StreamNumber);
internal MSF_UInt         msf_stream_get_pos(MSF_Context *msf, MSF_StreamNumber sn);
internal void             msf_stream_align(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt align);
internal B32              msf_stream_reserve(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt size);
internal B32              msf_stream_seek(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt new_pos);
internal B32              msf_stream_seek_start(MSF_Context *msf, MSF_StreamNumber sn);
internal B32              msf_stream_seek_end(MSF_Context *msf, MSF_StreamNumber sn);

internal MSF_UInt msf_stream_read(MSF_Context *msf, MSF_StreamNumber sn, void *dst, MSF_UInt dst_len);
internal String8  msf_stream_read_block(Arena *arena, MSF_Context *msf, MSF_StreamNumber sn, U64 block_size);
internal String8  msf_stream_read_string(Arena *arena, MSF_Context *msf, MSF_StreamNumber sn);
internal S8       msf_stream_read_s8(MSF_Context *msf, MSF_StreamNumber sn);
internal S16      msf_stream_read_s16(MSF_Context *msf, MSF_StreamNumber sn);
internal S32      msf_stream_read_s32(MSF_Context *msf, MSF_StreamNumber sn);
internal S64      msf_stream_read_s64(MSF_Context *msf, MSF_StreamNumber sn);
internal U8       msf_stream_read_u8(MSF_Context *msf, MSF_StreamNumber sn);
internal U16      msf_stream_read_u16(MSF_Context *msf, MSF_StreamNumber sn);
internal U32      msf_stream_read_u32(MSF_Context *msf, MSF_StreamNumber sn);
internal U64      msf_stream_read_u64(MSF_Context *msf, MSF_StreamNumber sn);
#define msf_stream_read_array(msf, sn, ptr, count) msf_stream_read(msf, sn, ptr, sizeof(*ptr) * (count))
#define msf_stream_read_struct(msf, sn, ptr) msf_stream_read_array(msf, sn, ptr, 1)

internal B32 msf_stream_write(MSF_Context *msf, MSF_StreamNumber sn, void *buffer, MSF_UInt buffer_size);
internal B32 msf_stream_write_string(MSF_Context *msf, MSF_StreamNumber sn, String8 string);
internal B32 msf_stream_write_list(MSF_Context *msf, MSF_StreamNumber sn, String8List list);
internal B32 msf_stream_write_uint(MSF_Context *msf, MSF_StreamNumber sn, MSF_UInt value);
internal B32 msf_stream_write_cstr(MSF_Context *msf, MSF_StreamNumber sn, String8 string);
internal B32 msf_stream_write_u8(MSF_Context *msf, MSF_StreamNumber sn, U8 value);
internal B32 msf_stream_write_u16(MSF_Context *msf, MSF_StreamNumber sn, U16 value);
internal B32 msf_stream_write_u32(MSF_Context *msf, MSF_StreamNumber sn, U32 value);
internal B32 msf_stream_write_u64(MSF_Context *msf, MSF_StreamNumber sn, U64 value);
internal B32 msf_stream_write_s8(MSF_Context *msf, MSF_StreamNumber sn, S8 value);
internal B32 msf_stream_write_s16(MSF_Context *msf, MSF_StreamNumber sn, S16 value);
internal B32 msf_stream_write_s32(MSF_Context *msf, MSF_StreamNumber sn, S32 value);
internal B32 msf_stream_write_s64(MSF_Context *msf, MSF_StreamNumber sn, S64 value);
internal B32 msf_stream_write_parallel(TP_Context *tp, MSF_Context *msf, MSF_StreamNumber sn, void *buffer, MSF_UInt buffer_size);
#define msf_stream_write_array(m, s, v, c) msf_stream_write(m, s, (void*)(v), sizeof(*(v)) * (c))
#define msf_stream_write_struct(m, s, v )  msf_stream_write_array(m, s, v, 1)

internal MSF_UInt       msf_count_pages(MSF_UInt page_size, U64 data_size);
internal MSF_PageNumber msf_get_page_count_cap(MSF_PageDataList page_data_list, MSF_UInt page_size);
internal MSF_UInt       msf_get_fpm_interval_correct(MSF_UInt page_size);
internal MSF_UInt       msf_get_fpm_interval_wrong(MSF_UInt page_size);
internal MSF_UInt       msf_get_fpm_idx_from_pn(MSF_UInt page_size, MSF_PageNumber pn);
internal MSF_UInt       msf_get_fpm_page_bit_state(MSF_PageDataList page_data_list, MSF_UInt page_size, MSF_PageNumber active_fpm, MSF_PageNumber pn);
internal void           msf_set_fpm_bit(MSF_PageDataList page_data_list, MSF_UInt page_size, MSF_PageNumber active_fpm, MSF_PageNumber pn, B32 state);
internal B32            msf_write(MSF_PageDataList page_data_list, MSF_UInt page_size, MSF_PageList page_list, MSF_UInt offset, void *buffer, MSF_UInt buffer_size);
internal MSF_UInt       msf_read(MSF_PageDataList page_data_list, MSF_UInt page_size, MSF_PageList page_list, MSF_UInt offset, void *buffer, MSF_UInt buffer_size);

internal char * msf_error_to_string(MSF_Error code);

