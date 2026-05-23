// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_INC_H
#define BASE_INC_H

////////////////////////////////
//~ rjf: Base Includes

#include "base_context_cracking.h"

#include "base_core.h"
#include "base_profile.h"
#include "base_memory.h"
#include "base_arena.h"
#include "base_math.h"
#include "base_strings.h"
#include "base_hash.h"
#include "base_system.h"
#include "base_threads.h"
#include "base_thread_context.h"
#include "base_files.h"
#include "base_shared_memory.h"
#include "base_processes.h"
#include "base_dynamic_libraries.h"
#include "base_command_line.h"
#include "base_markup.h"
#include "base_meta.h"
#include "base_log.h"
#include "base_test.h"
#include "base_entry_point.h"

#if OS_WINDOWS
# include "win32/base/win32_base.h"
#elif OS_LINUX
# include "linux/base/linux_base.h"
#else
# error Operating system backend not found for base layer.
#endif

#endif // BASE_INC_H
