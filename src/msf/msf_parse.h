// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef MSF_PARSE_H
#define MSF_PARSE_H

////////////////////////////////
//~ rjf: MSF Parser Helper Types

typedef struct MSF_Parsed MSF_Parsed;
struct MSF_Parsed
{
  String8 *streams;
  U64 stream_count;
  U64 block_size;
  U64 block_count;
};

#define MSF_MAX_MAGIC_SIZE Min(sizeof(msf_msf20_magic), sizeof(msf_msf70_magic))

////////////////////////////////
//~ rjf: MSF Parser Functions

internal MSF_Parsed* msf_parsed_from_data(Arena *arena, String8 msf_data);
internal String8     msf_data_from_stream(MSF_Parsed *msf, MSF_StreamNumber sn);

#endif // MSF_PARSE_H
