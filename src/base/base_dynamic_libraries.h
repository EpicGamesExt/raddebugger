// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_DYNAMIC_LIBARIES_H
#define BASE_DYNAMIC_LIBARIES_H

typedef struct Library Library;
struct Library
{
  U64 u64[1];
};

////////////////////////////////
//~ rjf: @per_os_impl Dynamically-Loaded Libraries

internal Library library_open(String8 path);
internal void library_close(Library lib);
internal VoidProc *library_load_proc(Library lib, String8 name);

#endif // BASE_DYNAMIC_LIBARIES_H
