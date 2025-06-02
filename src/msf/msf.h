// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef MSF_H
#define MSF_H

////////////////////////////////
//~ rjf: MSF Format Types

#define MSF_UINT_MAX max_U32
typedef U32 MSF_UInt;
typedef S32 MSF_Int;

#define MSF_BITS_PER_CHAR 8
#define MSF_BITS_PER_WORD (sizeof(MSF_UInt) * MSF_BITS_PER_CHAR)

#define MSF_PN_MAX MSF_UINT_MAX
typedef MSF_UInt MSF_PageNumber;
#define MSF_MIN_PAGE_SIZE 512
#define MSF_MAX_PAGE_SIZE 32768

#define MSF_MAX_STREAM_SIZE       MSF_INT_MAX
#define MSF_DELETED_STREAM_STAMP  MSF_UINT_MAX
#define MSF_STREAM_NUMBER_MAX     max_U16
#define MSF_INVALID_STREAM_NUMBER MSF_STREAM_NUMBER_MAX
typedef U16 MSF_StreamNumber;

static char msf_msf20_magic[] = "Microsoft C/C++ program database 2.00\r\n\x1aJG\0\0";
static char msf_msf70_magic[] = "Microsoft C/C++ MSF 7.00\r\n\032DS\0\0";

typedef struct MSF_Header20 MSF_Header20;
struct MSF_Header20
{
  U8  magic[sizeof(msf_msf20_magic)];
  U32 page_size;
  U16 active_fpm;
  U16 page_count;
  U32 stream_table_size;
  U32 unknown;
  U16 root_pn;
};

typedef struct MSF_Header70 MSF_Header70;
struct MSF_Header70
{
  U8             magic[sizeof(msf_msf70_magic)];
  MSF_UInt       page_size;
  MSF_PageNumber active_fpm;
  MSF_PageNumber page_count;
  MSF_UInt       stream_table_size;
  MSF_UInt       unknown; // always set to zero (used to be stream table page number, see SI_PERSIST in msf.cpp)
  MSF_PageNumber root_pn;
};

////////////////////////////////

internal B32 msf_check_magic_20(String8 data);
internal B32 msf_check_magic_70(String8 data);

#endif // MSF_H
