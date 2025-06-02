// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef MSF_PARSE_H
#define MSF_PARSE_H

////////////////////////////////
//~ rjf: MSF Parser Helper Types

typedef struct MSF_RawStream MSF_RawStream;
struct MSF_RawStream
{
  U64 size;
  U64 page_count;
  union {
    U32 *page_indices_u32;
    U16 *page_indices_u16;
  } u;
};

typedef struct MSF_RawStreamTable MSF_RawStreamTable;
struct MSF_RawStreamTable
{
  U64            total_page_count;
  U64            index_size;
  U64            page_size;
  U64            stream_count;
  MSF_RawStream *streams;
};

typedef struct MSF_Parsed MSF_Parsed;
struct MSF_Parsed
{
  String8 *streams;
  U64      stream_count;
  U64      page_size;
  U64      page_count;
};

////////////////////////////////
//~ rjf: MSF Parser Functions

internal MSF_RawStreamTable* msf_raw_stream_table_from_data(Arena *arena, String8 msf_data);
internal String8             msf_data_from_stream_number(Arena *arena, String8 msf_data, MSF_RawStreamTable *st, MSF_StreamNumber sn);
internal MSF_Parsed*         msf_parsed_from_data(Arena *arena, String8 msf_data);
internal String8             msf_data_from_stream(MSF_Parsed *msf, MSF_StreamNumber sn);

#endif // MSF_PARSE_H
