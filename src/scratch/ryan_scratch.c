// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_VERSION_MAJOR 0
#define BUILD_VERSION_MINOR 9
#define BUILD_VERSION_PATCH 8
#define BUILD_RELEASE_PHASE_STRING_LITERAL "ALPHA"
#define BUILD_TITLE "ryan_scratch"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "regs/regs.h"
#include "coff/coff.h"
#include "pe/pe.h"
#include "demon2/demon2_inc.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "regs/regs.c"
#include "coff/coff.c"
#include "pe/pe.c"
#include "demon2/demon2_inc.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  
}
