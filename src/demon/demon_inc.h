// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_INC_H
#define DEMON_INC_H

#include "demon/demon_core.h"

#if OS_WINDOWS
# include "win32/demon/win32_demon.h"
#elif OS_LINUX
# include "linux/demon/linux_demon.h"
#else
# error Demon layer backend not defined for this operating system.
#endif

#endif // DEMON_INC_H
