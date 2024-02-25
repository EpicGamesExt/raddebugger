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
