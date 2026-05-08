// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_SYSTEM_H
#define BASE_SYSTEM_H

typedef struct SystemInfo SystemInfo;
struct SystemInfo
{
  U32 logical_processor_count;
  U64 page_size;
  U64 large_page_size;
  U64 allocation_granularity;
  String8 machine_name;
};

////////////////////////////////
//~ rjf: @per_os_impl System Info

internal SystemInfo *get_system_info(void);

#endif // BASE_SYSTEM_H
