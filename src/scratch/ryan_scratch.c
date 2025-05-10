// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_TITLE "ryan_scratch"
#define OS_FEATURE_GRAPHICAL 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "render/render_inc.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "render/render_inc.c"

////////////////////////////////
//~ rjf: Globals

global OS_Handle window_os = {0};
global R_Handle window_r = {0};

////////////////////////////////
//~ rjf: Entry Points

internal B32
frame(void)
{
  B32 quit = 0;
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: events test
  OS_EventList events = os_get_events(scratch.arena, 0);
  for(OS_Event *ev = events.first; ev != 0; ev = ev->next)
  {
    if(ev->kind != OS_EventKind_MouseMove)
    {
      String8 string = push_str8f(scratch.arena, "%S (%S)\n", os_string_from_event_kind(ev->kind), os_g_key_display_string_table[ev->key]);
      printf("%.*s", str8_varg(string));
      raddbg_log((char *)string.str);
      fflush(stdout);
    }
    if(ev->kind == OS_EventKind_Press && ev->key == OS_Key_X)
    {
      *(volatile int *)0 = 0;
    }
  }
  for(OS_Event *ev = events.first; ev != 0; ev = ev->next)
  {
    if(ev->kind == OS_EventKind_WindowClose)
    {
      quit = 1;
      break;
    }
  }
  
  //- rjf: drawing test
  r_begin_frame();
  r_window_begin_frame(window_os, window_r);
  {
    R_PassList passes = {0};
    R_Pass *pass = r_pass_from_kind(scratch.arena, &passes, R_PassKind_UI);
    R_PassParams_UI *pass_ui = pass->params_ui;
    R_BatchGroup2DNode group = {0};
    pass_ui->rects.first = pass_ui->rects.last = &group;
    pass_ui->rects.count = 1;
    group.batches = r_batch_list_make(sizeof(R_Rect2DInst));
    group.params.xform = mat_3x3f32(1.f);
    group.params.clip = os_client_rect_from_window(window_os);
    Vec2F32 mouse = os_mouse_from_window(window_os);
    R_Rect2DInst *inst = r_batch_list_push_inst(scratch.arena, &group.batches, 256);
    MemoryZeroStruct(inst);
    inst->dst = r2f32p(mouse.x+30, mouse.y+30, mouse.x+100, mouse.y+100);
    inst->src = r2f32p(0, 0, 1, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = inst->colors[Corner_10] = inst->colors[Corner_11] = v4f32(1, 0, 0, 1);
    inst->corner_radii[Corner_00] = inst->corner_radii[Corner_01] = inst->corner_radii[Corner_10] = inst->corner_radii[Corner_11] = 8.f;
    r_window_submit(window_os, window_r, &passes);
  }
  r_window_end_frame(window_os, window_r);
  r_end_frame();
  
  scratch_end(scratch);
  return quit;
}

internal void
entry_point(CmdLine *cmdline)
{
  window_os = os_window_open(r2f32p(0, 0, 1280, 720), OS_WindowFlag_UseDefaultPosition, str8_lit("Window"));
  os_window_first_paint(window_os);
  window_r = r_window_equip(window_os);
  for(;!update(););
}
