// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

// Generic subprocess transport. Suites own command formatting and response semantics.

typedef enum T_ControllerResultCode
{
  T_ControllerResultCode_Ok,
  T_ControllerResultCode_LaunchFailed,
  T_ControllerResultCode_IoFailed,
  T_ControllerResultCode_Timeout,
  T_ControllerResultCode_ProcessExited,
} T_ControllerResultCode;

typedef struct T_ControllerResult T_ControllerResult;
struct T_ControllerResult
{
  T_ControllerResultCode code;
  U64 exit_code;
};

typedef enum T_ControllerStream
{
  T_ControllerStream_Stdout,
  T_ControllerStream_Stderr,
} T_ControllerStream;

typedef struct T_ControllerLaunchParams T_ControllerLaunchParams;
struct T_ControllerLaunchParams
{
  String8List cmd_line;
  String8 path;
  String8List env;
  B32 inherit_env;
  B32 consoleless;
  B32 new_console;
  B32 merge_outputs; // Redirect stderr into stdout while preserving one ordered transcript.
};

typedef struct T_Controller T_Controller;
struct T_Controller
{
  Arena *arena;
  Process process;
  ProcessGroup process_group;

  File stdin_write;
  File stdout_read;
  File stderr_read;

  String8List stdout_capture;
  String8List stderr_capture;
  U64 stdout_cursor;
  U64 stderr_cursor;

  B32 merge_outputs;
  B32 new_console;
  B32 launched;
  B32 exited;
  B32 closed;
  U64 exit_code;
};

internal T_ControllerResult t_controller_launch(Arena *arena, T_Controller *controller, T_ControllerLaunchParams *params);
internal T_ControllerResult t_controller_pump(T_Controller *controller);
internal T_ControllerResult t_controller_send(T_Controller *controller, String8 data);
internal T_ControllerResult t_controller_send_line(T_Controller *controller, String8 line);
internal T_ControllerResult t_controller_wait(T_Controller *controller, U64 timeout_us);
internal T_ControllerResult t_controller_expect(T_Controller *controller, T_ControllerStream stream, String8 needle, U64 timeout_us);
internal T_ControllerResult t_controller_wait_until_quiet(T_Controller *controller, U64 quiet_us, U64 timeout_us);
internal T_ControllerResult t_controller_interrupt(T_Controller *controller);
internal void t_controller_close(T_Controller *controller);

internal String8 t_controller_stdout(Arena *arena, T_Controller *controller);
internal String8 t_controller_stderr(Arena *arena, T_Controller *controller);
