// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_TITLE "textperf"
#define OS_FEATURE_GRAPHICAL 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "render/render_inc.h"
#include "font_provider/font_provider_inc.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "render/render_inc.c"
#include "font_provider/font_provider_inc.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"

////////////////////////////////
//~ rjf: Globals

global OS_Handle os_window = {0};
global R_Handle r_window = {0};

////////////////////////////////
//~ rjf: Entry Points

internal B32
frame(void)
{
  ProfBeginFunction();
  B32 quit = 0;
  Temp scratch = scratch_begin(0, 0);
  OS_EventList events = os_get_events(scratch.arena, 0);
  for(OS_Event *evt = events.first; evt != 0; evt = evt->next)
  {
    if(evt->kind == OS_EventKind_WindowClose)
    {
      quit = 1;
      break;
    }
  }
  r_begin_frame();
  dr_begin_frame(fnt_tag_zero());
  r_window_begin_frame(os_window, r_window);
  DR_Bucket *bucket = dr_bucket_make();
  DR_BucketScope(bucket) ProfScope("draw")
  {
    Vec2F32 mouse = os_mouse_from_window(os_window);
    FNT_Tag font = fnt_tag_from_path(str8_lit("C:/devel/raddebugger/data/Inconsolata-Regular.ttf"));
    for(F32 x = 0; x < 500; x += 5.f)
    {
      for(F32 y = 0; y < 500; y += 5.f)
      {
        dr_text(font, 16.f, 0, 0, FNT_RasterFlag_Smooth|FNT_RasterFlag_Hinted, v2f32(30 + x + mouse.x, 30 + y + mouse.y), v4f32(1, 1, 1, 1), str8_lit("This is a test."));
      }
    }
  }
  r_window_submit(os_window, r_window, &bucket->passes);
  r_window_end_frame(os_window, r_window);
  r_end_frame();
  scratch_end(scratch);
  ProfEnd();
  return quit;
}

internal void
entry_point(CmdLine *cmdline)
{
  os_window = os_window_open(r2f32p(0, 0, 1600, 900), OS_WindowFlag_UseDefaultPosition, str8_lit("textperf"));
  r_window = r_window_equip(os_window);
  os_window_first_paint(os_window);
  for(;!update(););
}
