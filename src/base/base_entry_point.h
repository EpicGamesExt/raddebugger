// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_ENTRY_POINT_H
#define BASE_ENTRY_POINT_H

#if BASE_ENTRY_POINT_ARGCV
typedef void (*EntryPoint)(int argc, char **argv);
#else
typedef void (*EntryPoint)(CmdLine *cmdline);
#endif

internal void main_thread_base_entry_point(EntryPoint entry_point, int argc, char **argv);
internal void supplement_thread_base_entry_point(void (*entry_point)(void *params), void *params);
internal U64 update_tick_idx(void);
internal B32 update(void);

#endif // BASE_ENTRY_POINT_H
