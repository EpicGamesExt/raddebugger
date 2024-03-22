// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_INC_H
#define DEMON_INC_H

#include "demon_core.h"

#if OS_WINDOWS
# include "win32/demon_core_win32.h"
#else
# error Demon layer backend not defined for this operating system.
#endif

#endif // DEMON_INC_H
