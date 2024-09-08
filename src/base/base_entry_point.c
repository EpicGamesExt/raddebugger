// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
main_thread_base_entry_point(void (*entry_point)(CmdLine *cmdline), char **arguments, U64 arguments_count)
{
  Temp scratch = scratch_begin(0, 0);
  ThreadNameF("[main thread]");
  
  //- rjf: set up telemetry
#if PROFILE_TELEMETRY
  local_persist U8 tm_data[MB(64)];
  tmLoadLibrary(TM_RELEASE);
  tmSetMaxThreadCount(256);
  tmInitialize(sizeof(tm_data), (char *)tm_data);
#endif
  
  //- rjf: parse command line
  String8List command_line_argument_strings = os_string_list_from_argcv(scratch.arena, (int)arguments_count, arguments);
  CmdLine cmdline = cmd_line_from_string_list(scratch.arena, command_line_argument_strings);
  
  //- rjf: begin captures
  B32 capture = cmd_line_has_flag(&cmdline, str8_lit("capture"));
  if(capture)
  {
    ProfBeginCapture(arguments[0]);
  }
  
  //- rjf: initialize all included layers
#if defined(TASK_SYSTEM_H) && !defined(TS_INIT_MANUAL)
  ts_init();
#endif
#if defined(HASH_STORE_H) && !defined(HS_INIT_MANUAL)
  hs_init();
#endif
#if defined(FILE_STREAM_H) && !defined(FS_INIT_MANUAL)
  fs_init();
#endif
#if defined(TEXT_CACHE_H) && !defined(TXT_INIT_MANUAL)
  txt_init();
#endif
#if defined(MUTABLE_TEXT_H) && !defined(MTX_INIT_MANUAL)
  mtx_init();
#endif
#if defined(DASM_CACHE_H) && !defined(DASM_INIT_MANUAL)
  dasm_init();
#endif
#if defined(DBGI_H) && !defined(DBGI_INIT_MANUAL)
  di_init();
#endif
#if defined(FUZZY_SEARCH_H) && !defined(FZY_INIT_MANUAL)
  fzy_init();
#endif
#if defined(DEMON_CORE_H) && !defined(DMN_INIT_MANUAL)
  dmn_init();
#endif
#if defined(CTRL_CORE_H) && !defined(CTRL_INIT_MANUAL)
  ctrl_init();
#endif
#if defined(OS_GFX_H) && !defined(OS_GFX_INIT_MANUAL)
  os_gfx_init();
#endif
#if defined(FONT_PROVIDER_H) && !defined(FP_INIT_MANUAL)
  fp_init();
#endif
#if defined(RENDER_CORE_H) && !defined(R_INIT_MANUAL)
  r_init(&cmdline);
#endif
#if defined(TEXTURE_CACHE_H) && !defined(TEX_INIT_MANUAL)
  tex_init();
#endif
#if defined(GEO_CACHE_H) && !defined(GEO_INIT_MANUAL)
  geo_init();
#endif
#if defined(FONT_CACHE_H) && !defined(FNT_INIT_MANUAL)
  fnt_init();
#endif
#if defined(DBG_ENGINE_CORE_H) && !defined(D_INIT_MANUAL)
  d_init(&cmdline);
#endif
#if defined(DBG_FRONTEND_CORE_H) && !defined(DF_INIT_MANUAL)
  df_init();
#endif
  
  //- rjf: call into entry point
  entry_point(&cmdline);
  
  //- rjf: end captures
  if(capture)
  {
    ProfEndCapture();
  }
  scratch_end(scratch);
}

internal void
supplement_thread_base_entry_point(void (*entry_point)(void *params), void *params)
{
  TCTX tctx;
  tctx_init_and_equip(&tctx);
  entry_point(params);
  tctx_release();
}

internal B32
update(void)
{
  ProfTick(0);
#if defined(TEXT_CACHE_H)
  txt_user_clock_tick();
#endif
#if defined(DASM_CACHE_H)
  dasm_user_clock_tick();
#endif
#if defined(GEO_CACHE_H)
  geo_user_clock_tick();
#endif
#if defined(TEXTURE_CACHE_H)
  tex_user_clock_tick();
#endif
  B32 result = frame();
  return result;
}
