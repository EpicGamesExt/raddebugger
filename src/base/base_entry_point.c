internal void
main_thread_base_entry_point(void (*entry_point)(CmdLine *cmdline), char **arguments, U64 arguments_count)
{
#if PROFILE_TELEMETRY
  local_persist U8 tm_data[MB(64)];
  tmLoadLibrary(TM_RELEASE);
  tmSetMaxThreadCount(256);
  tmInitialize(sizeof(tm_data), (char *)tm_data);
#endif
  TCTX tctx;
  tctx_init_and_equip(&tctx);
  ThreadNameF("[main thread]");
  Temp scratch = scratch_begin(0, 0);
  String8List command_line_argument_strings = os_string_list_from_argcv(scratch.arena, (int)arguments_count, arguments);
  CmdLine cmdline = cmd_line_from_string_list(scratch.arena, command_line_argument_strings);
  B32 capture = cmd_line_has_flag(&cmdline, str8_lit("capture"));
  if(capture)
  {
    ProfBeginCapture(arguments[0]);
  }
#if defined(OS_CORE_H)
  os_init();
#endif
#if defined(TASK_SYSTEM_H)
  ts_init();
#endif
#if defined(HASH_STORE_H)
  hs_init();
#endif
#if defined(FILE_STREAM_H)
  fs_init();
#endif
#if defined(TEXT_CACHE_H)
  txt_init();
#endif
#if defined(DBGI_H)
  dbgi_init();
#endif
#if defined(TXTI_H)
  txti_init();
#endif
#if defined(DEMON_CORE_H)
  demon_init();
#endif
#if defined(DEMON2_CORE_H)
  dmn_init();
#endif
#if defined(CTRL_CORE_H)
  ctrl_init();
#endif
#if defined(DASM_H)
  dasm_init();
#endif
#if defined(OS_GRAPHICAL_H)
  os_graphical_init();
#endif
#if defined(FONT_PROVIDER_H)
  fp_init();
#endif
#if defined(RENDER_CORE_H)
  r_init(&cmdline);
#endif
#if defined(TEXTURE_CACHE_H)
  tex_init();
#endif
#if defined(GEO_CACHE_H)
  geo_init();
#endif
#if defined(FONT_CACHE_H)
  f_init();
#endif
#if defined(DF_CORE_H)
  DF_StateDeltaHistory *hist = df_state_delta_history_alloc();
  df_core_init(&cmdline, hist);
#endif
#if defined(DF_GFX_H)
  df_gfx_init(update_and_render, df_state_delta_history());
#endif
  entry_point(&cmdline);
  if(capture)
  {
    ProfEndCapture();
  }
  scratch_end(scratch);
  tctx_release();
}

internal void
supplement_thread_base_entry_point(void (*entry_point)(void *params), void *params)
{
  TCTX tctx;
  tctx_init_and_equip(&tctx);
  entry_point(params);
  tctx_release();
}
