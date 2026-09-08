// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include <signal.h>

internal void
t_controller_fixture(void)
{
  fprintf(stdout, "stdout:ready\n");
  fprintf(stderr, "stderr:ready\n");
  fflush(stdout);
  fflush(stderr);

  char line[4096];
  while (fgets(line, sizeof(line), stdin) != 0) {
    U64 size = strlen(line);
    while (size != 0 && (line[size - 1] == '\r' || line[size - 1] == '\n')) { line[--size] = 0; }
    if (strcmp(line, "exit") == 0) { break; }
    fprintf(stdout, "stdout:%s\n", line);
    fprintf(stderr, "stderr:%s\n", line);
    fflush(stdout);
    fflush(stderr);
  }
}

global volatile sig_atomic_t t_controller_fixture_interrupted;

internal void
t_controller_fixture_signal_handler(int signal)
{
  t_controller_fixture_interrupted = signal == SIGINT;
}

internal void
t_controller_interrupt_fixture(void)
{
  signal(SIGINT, t_controller_fixture_signal_handler);
  fprintf(stdout, "interrupt:ready\n");
  fflush(stdout);
  while (!t_controller_fixture_interrupted) { sleep_ms(1); }
  fprintf(stdout, "interrupt:received\n");
  fflush(stdout);
}

internal void
t_run_operation_fixture(B32 sleep, U64 exit_code)
{
  fprintf(stdout, "GenericRunStdout\n");
  fprintf(stderr, "GenericRunStderr\n");
  fflush(stdout);
  fflush(stderr);
  if (sleep) { sleep_ms(30000); }
  exit((int)exit_code);
}

internal T_ControllerResult
t_controller_test_launch(Arena *arena, T_Controller *controller, B32 merge_outputs)
{
  String8List cmd_line = {0};
  str8_list_push(arena, &cmd_line, get_process_info()->binary_file_path);
  str8_list_push(arena, &cmd_line, str8_lit("-controller_fixture"));
  T_ControllerLaunchParams params = {
      .cmd_line = cmd_line,
      .path = get_process_info()->binary_path,
      .inherit_env = 1,
      .consoleless = 1,
      .merge_outputs = merge_outputs,
  };
  return t_controller_launch(arena, controller, &params);
}

TEST(controller_interactive_separate_output)
{
  T_Controller controller = {0};
  B32 is_ok = 0;

  if (t_controller_test_launch(arena, &controller, 0).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_expect(&controller, T_ControllerStream_Stdout, str8_lit("stdout:ready"), TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_expect(&controller, T_ControllerStream_Stderr, str8_lit("stderr:ready"), TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_wait_until_quiet(&controller, TIMEOUT_MS(10), TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_send_line(&controller, str8_lit("hello")).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_expect(&controller, T_ControllerStream_Stdout, str8_lit("stdout:hello"), TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_expect(&controller, T_ControllerStream_Stderr, str8_lit("stderr:hello"), TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_expect(&controller, T_ControllerStream_Stdout, str8_lit("missing"), TIMEOUT_MS(10)).code != T_ControllerResultCode_Timeout) { goto exit; }
  if (t_controller_send_line(&controller, str8_lit("exit")).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_wait(&controller, TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok || controller.exit_code != 0) { goto exit; }
  is_ok = 1;

exit:;
  t_controller_close(&controller);
  if (!is_ok) {
    test_outf("stdout:\n%S\n", t_controller_stdout(arena, &controller));
    test_outf("stderr:\n%S\n", t_controller_stderr(arena, &controller));
  }
  T_Ok(is_ok);
}

TEST(controller_interactive_merged_output)
{
  T_Controller controller = {0};
  B32 is_ok = 0;

  if (t_controller_test_launch(arena, &controller, 1).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_expect(&controller, T_ControllerStream_Stdout, str8_lit("stdout:ready"), TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_expect(&controller, T_ControllerStream_Stderr, str8_lit("stderr:ready"), TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_send_line(&controller, str8_lit("merged")).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_expect(&controller, T_ControllerStream_Stdout, str8_lit("stdout:merged"), TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_send_line(&controller, str8_lit("exit")).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_wait(&controller, TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok || controller.exit_code != 0) { goto exit; }
  {
    String8 output = t_controller_stdout(arena, &controller);
    is_ok = str8_find_needle(output, 0, str8_lit("stderr:ready"), 0) < output.size && str8_find_needle(output, 0, str8_lit("stderr:merged"), 0) < output.size;
  }

exit:;
  t_controller_close(&controller);
  if (!is_ok) { test_outf("merged output:\n%S\n", t_controller_stdout(arena, &controller)); }
  T_Ok(is_ok);
}

TEST(controller_interrupt)
{
  T_Controller controller = {0};
  B32 is_ok = 0;
  String8List cmd_line = {0};
  str8_list_push(arena, &cmd_line, get_process_info()->binary_file_path);
  str8_list_push(arena, &cmd_line, str8_lit("-controller_interrupt_fixture"));
  T_ControllerLaunchParams params = {
      .cmd_line = cmd_line,
      .path = get_process_info()->binary_path,
      .inherit_env = 1,
      .new_console = 1,
      .merge_outputs = 1,
  };

  if (t_controller_launch(arena, &controller, &params).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_expect(&controller, T_ControllerStream_Stdout, str8_lit("interrupt:ready"), TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_interrupt(&controller).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_expect(&controller, T_ControllerStream_Stdout, str8_lit("interrupt:received"), TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok) { goto exit; }
  if (t_controller_wait(&controller, TIMEOUT_SEC(5)).code != T_ControllerResultCode_Ok || controller.exit_code != 0) { goto exit; }
  is_ok = 1;

exit:;
  t_controller_close(&controller);
  if (!is_ok) { test_outf("interrupt output:\n%S\n", t_controller_stdout(arena, &controller)); }
  T_Ok(is_ok);
}
