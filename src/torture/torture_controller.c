// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal T_ControllerResult
t_controller_result(T_ControllerResultCode code, U64 exit_code)
{
  T_ControllerResult result = {code, exit_code};
  return result;
}

internal void
t_controller_file_close(File *file)
{
  file_close(*file);
  MemoryZeroStruct(file);
}

internal U64
t_controller_endt_from_timeout(U64 timeout_us)
{
  if (timeout_us == max_U64) { return max_U64; }
  U64 now_us = now_time_us();
  return timeout_us > max_U64 - now_us ? max_U64 : now_us + timeout_us;
}

internal B32
t_controller_endt_is_expired(U64 endt_us)
{
  return endt_us != max_U64 && now_time_us() >= endt_us;
}

internal String8List *
t_controller_capture_from_stream(T_Controller *controller, T_ControllerStream stream)
{
  if (stream == T_ControllerStream_Stdout) { return &controller->stdout_capture; }
  if (stream == T_ControllerStream_Stderr) { return controller->merge_outputs ? &controller->stdout_capture : &controller->stderr_capture; }
  return 0;
}

internal U64 *
t_controller_cursor_from_stream(T_Controller *controller, T_ControllerStream stream)
{
  if (stream == T_ControllerStream_Stdout) { return &controller->stdout_cursor; }
  if (stream == T_ControllerStream_Stderr) { return &controller->stderr_cursor; }
  return 0;
}

internal B32
t_controller_capture_has(Arena *arena, String8List *capture, U64 *cursor, String8 needle)
{
  String8 output = str8_list_join(arena, capture, 0);
  U64 match_off = str8_find_needle(output, *cursor, needle, 0);
  B32 result = match_off < output.size;
  if (result) { *cursor = match_off + needle.size; }
  return result;
}

internal T_ControllerResult
t_controller_pump_pipe(T_Controller *controller, File pipe, String8List *capture)
{
  U64 available = file_pipe_bytes_available(pipe);
  if (available != 0) {
    U64 size = Min(available, KB(64));
    U8 *data = push_array(controller->arena, U8, size);
    U64 read_size = file_pipe_read(pipe, data, size);
    if (read_size == 0) { return t_controller_result(T_ControllerResultCode_IoFailed, controller->exit_code); }
    str8_list_push(controller->arena, capture, str8(data, read_size));
  }
  return t_controller_result(T_ControllerResultCode_Ok, controller->exit_code);
}

internal void
t_controller_drain_pipe(T_Controller *controller, File pipe, String8List *capture)
{
  U64 endt_us = t_controller_endt_from_timeout(10 * 1000000ull);
  for (;;) {
    t_controller_pump_pipe(controller, pipe, capture);
    if (file_pipe_is_end(pipe)) { break; }
    if (t_controller_endt_is_expired(endt_us)) { break; }
    sleep_ms(1);
  }
}

internal T_ControllerResult
t_controller_poll_exit(T_Controller *controller)
{
  if (controller->launched && !controller->exited) {
    U64 exit_code = max_U64;
    if (process_poll(controller->process, &exit_code)) {
      controller->exited = 1;
      controller->exit_code = exit_code;
    }
  }
  return t_controller_result(T_ControllerResultCode_Ok, controller->exit_code);
}

internal T_ControllerResult
t_controller_launch(Arena *arena, T_Controller *controller, T_ControllerLaunchParams *params)
{
  MemoryZeroStruct(controller);
  controller->arena         = arena;
  controller->exit_code     = max_U64;
  controller->merge_outputs = params->merge_outputs;
  controller->new_console   = params->new_console;

  FilePair stdin_pipe  = file_pipe_make(1, 0);
  FilePair stdout_pipe = file_pipe_make(0, 1);
  FilePair stderr_pipe = {0};
  if (!params->merge_outputs) { stderr_pipe = file_pipe_make(0, 1); }

  B32 pipes_are_ok = file_pair_ok(stdin_pipe) && file_pair_ok(stdout_pipe) &&
                     (params->merge_outputs || file_pair_ok(stderr_pipe));
  if (!pipes_are_ok) { goto launch_failed; }

  controller->process_group = process_group_make(1);
  if (controller->process_group.u64[0] == 0) { goto launch_failed; }

  ProcessLaunchParams process_params = {
      .cmd_line      = params->cmd_line,
      .path          = params->path,
      .env           = params->env,
      .inherit_env   = params->inherit_env,
      .consoleless   = params->consoleless,
      .new_console   = params->new_console,
      .process_group = controller->process_group,
      .stdin_file    = stdin_pipe.read,
      .stdout_file   = stdout_pipe.write,
      .stderr_file   = params->merge_outputs ? stdout_pipe.write : stderr_pipe.write,
  };
  controller->process = process_launch(&process_params);

  if (process_match(controller->process, process_zero())) { goto launch_failed; }

  controller->stdin_write = stdin_pipe.write;
  controller->stdout_read = stdout_pipe.read;
  controller->stderr_read = stderr_pipe.read;
  controller->launched    = 1;

  MemoryZeroStruct(&stdin_pipe.write);
  MemoryZeroStruct(&stdout_pipe.read);
  MemoryZeroStruct(&stderr_pipe.read);

  file_pair_close(&stdin_pipe);
  file_pair_close(&stdout_pipe);
  file_pair_close(&stderr_pipe);

  return t_controller_result(T_ControllerResultCode_Ok, max_U64);

launch_failed:;

  file_pair_close(&stdin_pipe);
  file_pair_close(&stdout_pipe);
  file_pair_close(&stderr_pipe);

  process_group_close(controller->process_group);
  MemoryZeroStruct(&controller->process_group);
  controller->closed = 1;

  return t_controller_result(T_ControllerResultCode_LaunchFailed, max_U64);
}

internal T_ControllerResult
t_controller_pump(T_Controller *controller)
{
  T_ControllerResult result = t_controller_pump_pipe(controller, controller->stdout_read, &controller->stdout_capture);
  if (result.code == T_ControllerResultCode_Ok && !controller->merge_outputs) { result = t_controller_pump_pipe(controller, controller->stderr_read, &controller->stderr_capture); }
  if (result.code == T_ControllerResultCode_Ok) { result = t_controller_poll_exit(controller); }
  if (result.code == T_ControllerResultCode_Ok && controller->exited) {
    result = t_controller_pump_pipe(controller, controller->stdout_read, &controller->stdout_capture);
    if (result.code == T_ControllerResultCode_Ok && !controller->merge_outputs) {
      result = t_controller_pump_pipe(controller, controller->stderr_read, &controller->stderr_capture);
    }
  }
  return result;
}

internal T_ControllerResult
t_controller_send(T_Controller *controller, String8 data)
{
  if (controller->exited) { return t_controller_result(T_ControllerResultCode_ProcessExited, controller->exit_code); }

  for (U64 off = 0; off < data.size;) {
    U64 write_size = file_pipe_write(controller->stdin_write, data.str + off, data.size - off);
    if (write_size == 0) { return t_controller_result(T_ControllerResultCode_IoFailed, controller->exit_code); }
    off += write_size;
  }
  return t_controller_result(T_ControllerResultCode_Ok, controller->exit_code);
}

internal T_ControllerResult
t_controller_send_line(T_Controller *controller, String8 line)
{
  T_ControllerResult result = t_controller_send(controller, line);
  if (result.code == T_ControllerResultCode_Ok) { result = t_controller_send(controller, str8_lit("\n")); }
  return result;
}

internal T_ControllerResult
t_controller_wait(T_Controller *controller, U64 timeout_us)
{
  U64 endt_us = t_controller_endt_from_timeout(timeout_us);
  for (;;) {
    T_ControllerResult result = t_controller_pump(controller);
    if (result.code != T_ControllerResultCode_Ok) { return result; }
    if (controller->exited) { return result; }
    if (t_controller_endt_is_expired(endt_us)) { return t_controller_result(T_ControllerResultCode_Timeout, controller->exit_code); }
    sleep_ms(1);
  }
}

internal T_ControllerResult
t_controller_expect(T_Controller *controller, T_ControllerStream stream, String8 needle, U64 timeout_us)
{
  String8List *capture = t_controller_capture_from_stream(controller, stream);
  U64 *cursor = t_controller_cursor_from_stream(controller, stream);

  Temp scratch = scratch_begin(&controller->arena, 1);
  U64 endt_us = t_controller_endt_from_timeout(timeout_us);
  T_ControllerResult result = t_controller_result(T_ControllerResultCode_Timeout, controller->exit_code);
  for (;;) {
    result = t_controller_pump(controller);
    if (result.code != T_ControllerResultCode_Ok) { break; }
    Temp iteration = temp_begin(scratch.arena);
    B32 has_match = t_controller_capture_has(scratch.arena, capture, cursor, needle);
    temp_end(iteration);
    if (has_match) { break; }
    if (controller->exited) {
      result = t_controller_result(T_ControllerResultCode_ProcessExited, controller->exit_code);
      break;
    }
    if (t_controller_endt_is_expired(endt_us)) {
      result = t_controller_result(T_ControllerResultCode_Timeout, controller->exit_code);
      break;
    }
    sleep_ms(1);
  }
  scratch_end(scratch);
  return result;
}

internal T_ControllerResult
t_controller_wait_until_quiet(T_Controller *controller, U64 quiet_us, U64 timeout_us)
{
  U64 endt_us = t_controller_endt_from_timeout(timeout_us);
  U64 quiet_begin_us = now_time_us();
  U64 capture_size = controller->stdout_capture.total_size + controller->stderr_capture.total_size;
  for (;;) {
    T_ControllerResult result = t_controller_pump(controller);
    if (result.code != T_ControllerResultCode_Ok) { return result; }
    U64 new_capture_size = controller->stdout_capture.total_size + controller->stderr_capture.total_size;
    if (new_capture_size != capture_size) {
      capture_size = new_capture_size;
      quiet_begin_us = now_time_us();
    }
    if (controller->exited) { return t_controller_result(T_ControllerResultCode_ProcessExited, controller->exit_code); }
    U64 now_us = now_time_us();
    if (now_us - quiet_begin_us >= quiet_us) { return result; }
    if (endt_us != max_U64 && now_us >= endt_us) { return t_controller_result(T_ControllerResultCode_Timeout, controller->exit_code); }
    sleep_ms(1);
  }
}

internal T_ControllerResult
t_controller_interrupt(T_Controller *controller)
{
  T_ControllerResultCode code = process_send_ctrl_c(controller->process) ? T_ControllerResultCode_Ok : T_ControllerResultCode_IoFailed;
  return t_controller_result(code, controller->exit_code);
}

internal void
t_controller_close(T_Controller *controller)
{
  if (controller->closed) { return; }

  t_controller_file_close(&controller->stdin_write);

  if (controller->launched) {
    t_controller_pump(controller);

    process_group_close(controller->process_group);
    MemoryZeroStruct(&controller->process_group);

    if (!process_match(controller->process, process_zero())) {
      U64 exit_code = max_U64;
      U64 endt_us   = t_controller_endt_from_timeout(10 * 1000000ull);

      if (!process_join(controller->process, endt_us, &exit_code)) {
        process_kill(controller->process);
        process_join(controller->process, max_U64, &exit_code);
      }

      MemoryZeroStruct(&controller->process);
      controller->exited = 1;
      controller->exit_code = exit_code;
    }

    t_controller_drain_pipe(controller, controller->stdout_read, &controller->stdout_capture);
    if (!controller->merge_outputs) { t_controller_drain_pipe(controller, controller->stderr_read, &controller->stderr_capture); }
  }

  t_controller_file_close(&controller->stdout_read);
  t_controller_file_close(&controller->stderr_read);
  controller->closed = 1;
}

internal String8
t_controller_stdout(Arena *arena, T_Controller *controller)
{
  return str8_list_join(arena, &controller->stdout_capture, 0);
}

internal String8
t_controller_stderr(Arena *arena, T_Controller *controller)
{
  return str8_list_join(arena, &controller->stderr_capture, 0);
}

