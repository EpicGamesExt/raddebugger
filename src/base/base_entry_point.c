// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

global U64 global_update_tick_idx = 0;
global CondVar async_tick_cond_var = {0};
global Mutex async_tick_mutex = {0};

internal void
main_thread_base_entry_point(int arguments_count, char **arguments)
{
  Temp scratch = scratch_begin(0, 0);
  ThreadNameF("[main thread]");
  
  //- rjf: set up async thread group info
  async_tick_cond_var = cond_var_alloc();
  async_tick_mutex = mutex_alloc();
  
  //- rjf: set up telemetry
#if PROFILE_TELEMETRY
  local_persist char tm_data[MB(64)];
  tmLoadLibrary(TM_RELEASE);
  tmSetMaxThreadCount(256);
  tmInitialize(sizeof(tm_data), tm_data);
#endif
  
  //- rjf: set up spall
#if PROFILE_SPALL
  spall_profile = spall_init_file_ex("spall_capture", 1, 0);
#endif
  
  //- rjf: parse command line
  String8List command_line_argument_strings = os_string_list_from_argcv(scratch.arena, arguments_count, arguments);
  CmdLine cmdline = cmd_line_from_string_list(scratch.arena, command_line_argument_strings);
  
  //- rjf: begin captures
  B32 capture = cmd_line_has_flag(&cmdline, str8_lit("capture"));
  if(capture)
  {
    ProfBeginCapture(arguments[0]);
  }
  
#if PROFILE_TELEMETRY 
  tmMessage(0, TMMF_ICON_NOTE, BUILD_TITLE);
#endif
  
  //- rjf: initialize all included layers
#if defined(ASYNC_H) && !defined(ASYNC_INIT_MANUAL)
  async_init(&cmdline);
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
#if defined(DBGI_H) && !defined(DI_INIT_MANUAL)
  di_init();
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
  d_init();
#endif
#if defined(RADDBG_CORE_H) && !defined(RD_INIT_MANUAL)
  rd_init(&cmdline);
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
  TCTX *tctx = tctx_alloc();
  tctx_select(tctx);
  entry_point(params);
  tctx_release(tctx);
}

internal U64
update_tick_idx(void)
{
  U64 result = ins_atomic_u64_eval(&global_update_tick_idx);
  return result;
}

internal B32
update(void)
{
  ProfTick(0);
  ins_atomic_u64_inc_eval(&global_update_tick_idx);
#if defined(FONT_CACHE_H)
  fnt_frame();
#endif
#if OS_FEATURE_GRAPHICAL
  B32 result = frame();
#else
  B32 result = 0;
#endif
  return result;
}

internal void
async_thread_entry_point(void *params)
{
  MutexScope(async_tick_mutex) for(;;)
  {
    cond_var_wait(async_tick_cond_var, async_tick_mutex, max_U64);
#if defined(FILE_STREAM_H)
    fs_async_tick();
#endif
  }
}
