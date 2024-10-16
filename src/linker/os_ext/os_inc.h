// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

#include "core/os_core.h"

#if OS_WINDOWS
//# include "core/win32/os_core_win32.h"
#elif OS_LINUX
//# include "core/linux/os_core_linux.h"
#else
# error no OS layer setup
#endif

