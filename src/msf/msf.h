// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef MSF_H
#define MSF_H

////////////////////////////////
//~ rjf: MSF Format Types

#define MSF_INVALID_STREAM_NUMBER 0xFFFF
typedef U16 MSF_StreamNumber;

static char msf_msf20_magic[] = "Microsoft C/C++ program database 2.00\r\n\x1aJG\0\0";
static char msf_msf70_magic[] = "Microsoft C/C++ MSF 7.00\r\n\032DS\0\0";

#define MSF_MSF20_MAGIC_SIZE 44
#define MSF_MSF70_MAGIC_SIZE 32
#define MSF_MAX_MAGIC_SIZE   44

typedef struct MSF_Header20 MSF_Header20;
struct MSF_Header20
{
  U32 block_size;
  U16 free_block_map_block;
  U16 block_count;
  U32 directory_size;
  U32 unknown;
  U16 directory_map;
};

typedef struct MSF_Header70 MSF_Header70;
struct MSF_Header70
{
  U32 block_size;
  U32 free_block_map_block;
  U32 block_count;
  U32 directory_size;
  U32 unknown;
  U32 directory_super_map;
};

// magic(20) + header(20) = 44 + 20 = 64
// magic(70) + header(70) = 32 + 24 = 56

#define MSF_MIN_SIZE 64

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

////////////////////////////////
//~ rjf: MSF Parser Functions

internal MSF_Parsed* msf_parsed_from_data(Arena *arena, String8 msf_data);
internal String8     msf_data_from_stream(MSF_Parsed *msf, MSF_StreamNumber sn);

#endif // MSF_H
