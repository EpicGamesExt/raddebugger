// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON2_INC_H
#define DEMON2_INC_H

#include "demon2_core.h"

#if OS_WINDOWS
# include "win32/demon2_core_win32.h"
#else
# error Demon layer backend not defined for this operating system.
#endif

#endif // DEMON2_INC_H
