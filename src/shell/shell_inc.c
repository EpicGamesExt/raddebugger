// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "shell.c"
#if OS_WINDOWS
# include "win32/shell/win32_shell.c"
#elif OS_LINUX
# include "linux/shell/linux_shell.c"
#else
# error Shell functions not implemented for this operating system.
#endif
