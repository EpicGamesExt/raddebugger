// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_INC_H
#define DEMON_INC_H

#include "demon_core.h"
#include "demon_common.h"
#include "demon_accel.h"
#include "demon_os.h"

#if OS_WINDOWS
# include "win32/demon_os_win32.h"
#elif OS_LINUX
# include "linux/demon_os_linux.h"
#else
# error No Demon Implementation for This OS
#endif

#endif //DEMON_INC_H
