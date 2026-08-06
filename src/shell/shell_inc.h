// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef SHELL_INC_H
#define SHELL_INC_H

#include "shell.h"
#if OS_WINDOWS
# include "win32/shell/win32_shell.h"
#elif OS_LINUX
# include "linux/shell/linux_shell.h"
#else
# error Shell functions not implemented for this operating system.
#endif

#endif // SHELL_INC_H
