// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_PROCESSES_H
#define BASE_PROCESSES_H

typedef struct ProcessInfo ProcessInfo;
struct ProcessInfo
{
  U32 pid;
  B32 large_pages_allowed;
  String8 binary_file_path;
  String8 binary_path;
  String8 initial_path;
  String8 user_program_data_path;
  String8 symbol_cache_path;
  String8List module_load_paths;
  String8List environment;
};

typedef struct Process Process;
struct Process
{
  U64 u64[1];
};

typedef struct ProcessNode ProcessNode;
struct ProcessNode
{
  ProcessNode *next;
  Process v;
};

typedef struct ProcessList ProcessList;
struct ProcessList
{
  ProcessNode *first;
  ProcessNode *last;
  U64 count;
};

typedef struct ProcessLaunchParams ProcessLaunchParams;
struct ProcessLaunchParams
{
  String8List cmd_line;
  String8 path;
  String8List env;
  B32 inherit_env;
  B32 debug_subprocesses;
  B32 consoleless;
  File stdout_file;
  File stderr_file;
  File stdin_file;
};

////////////////////////////////
//~ rjf: Handle Type Functions

internal Process process_zero(void);
internal B32 process_match(Process a, Process b);
internal void process_list_push(Arena *arena, ProcessList *list, Process p);

////////////////////////////////
//~ rjf: Process Launcher Helpers

internal Process launch_cmd_line(String8 string);
internal Process launch_cmd_linef(char *fmt, ...);

////////////////////////////////
//~ rjf: @per_os_impl Aborting

internal void abort_self(S32 exit_code);

////////////////////////////////
//~ rjf: @per_os_impl Process Info

internal ProcessInfo *get_process_info(void);
internal String8 get_current_path(Arena *arena);
internal U32 get_process_start_time_unix(void);

////////////////////////////////
//~ rjf: @per_os_impl Child Processes

internal Process process_launch(ProcessLaunchParams *params);
internal U64 pid_from_process(Process process);
internal B32 process_join(Process process, U64 endt_us, U64 *exit_code_out);
internal void process_detach(Process process);
internal B32 process_kill(Process process);

#endif // BASE_PROCESSES_H
