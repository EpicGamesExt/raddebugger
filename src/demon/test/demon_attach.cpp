// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// exe //

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "demon/demon_inc.h"
#include "syms_helpers/syms_internal_overrides.h"
#include "syms/syms_inc.h"
#include "syms_helpers/syms_helpers.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "demon/demon_inc.c"
#include "syms_helpers/syms_internal_overrides.c"
#include "syms/syms_inc.c"
#include "syms_helpers/syms_helpers.c"

int
main(int argument_count, char **arguments)
{
  os_init(argument_count, arguments);
  Arena *arena = arena_alloc();
  demon_init();
  
  //- rjf: find PID of mule_loop.exe
  String8 attach_process_name = str8_lit("mule_loop.exe");
  U32 pid = 0;
  {
    DEMON_ProcessIter it = {0};
    demon_proc_iter_begin(&it);
    for(DEMON_ProcessInfo info = {0}; demon_proc_iter_next(arena, &it, &info);)
    {
      if(str8_match(info.name, attach_process_name, 0))
      {
        pid = info.pid;
        break;
      }
    }
    demon_proc_iter_end(&it);
  }
  
  //- rjf: attach
  B32 attach_good = demon_attach_process(pid);
  
  //- rjf: get events
  DEMON_RunCtrls ctrls = {0};
  DEMON_EventList events = demon_run(arena, ctrls);
  for(DEMON_Event *event = events.first; event != 0; event = event->next)
  {
    int x = 0;
  }
  
#if 0
  //- rjf: try to break in the loop
  DEMON_RunCtrls ctrls = {0};
  DEMON_Trap trap = {0};
  {
    U64 loop_bp = 0x0000000140001074;
    ctrls.trap_count = 1;
    ctrls.traps = &trap;
  }
#endif
}
