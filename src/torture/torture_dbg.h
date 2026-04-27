// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////
// Dbg Script

typedef struct T_DbgScriptFile
{
  struct T_DbgScriptFile *next;
  String8 path;
  String8 source;
  U64     line;
} T_DbgScriptFile;

typedef struct
{
  U64              count;
  T_DbgScriptFile *first;
  T_DbgScriptFile *last;
} T_DbgScriptFileList;

typedef enum
{
  T_DbgScriptCmdKind_Null,
  T_DbgScriptCmdKind_Breakpoint,
  T_DbgScriptCmdKind_ClearBreakpoints,
  T_DbgScriptCmdKind_Run,
  T_DbgScriptCmdKind_Halt,
  T_DbgScriptCmdKind_StepOver,
  T_DbgScriptCmdKind_StepInto,
  T_DbgScriptCmdKind_StepOut,
  T_DbgScriptCmdKind_StepOverInst,
  T_DbgScriptCmdKind_StepIntoInst,
  T_DbgScriptCmdKind_StepOverLine,
  T_DbgScriptCmdKind_StepIntoLine,
  T_DbgScriptCmdKind_KillAll,
  T_DbgScriptCmdKind_At,
  T_DbgScriptCmdKind_Eval,
} T_DbgScriptCmdKind;

typedef struct T_DbgScriptCmd
{
  struct T_DbgScriptCmd *next;
  T_DbgScriptCmdKind  kind;
  U64 line;
  union {
    struct {
      S64 delta; 
    } at;
  };
} T_DbgScriptCmd;

typedef struct T_DbgScriptProgram
{
  U64              line;
  U64              order;
  OperatingSystem  os;
  T_DbgScriptFile *file;
  U64              count;
  T_DbgScriptCmd  *first;
  T_DbgScriptCmd  *last;
  struct T_DbgScriptProgram *next;
} T_DbgScriptProgram;

typedef struct
{
  U64                 count;
  T_DbgScriptProgram *first;
  T_DbgScriptProgram *last;
} T_DbgScriptProgramList;

typedef enum
{
  T_DbgScriptDirectiveKind_Null,
  T_DbgScriptDirectiveKind_Compile,
  T_DbgScriptDirectiveKind_Link,
  T_DbgScriptDirectiveKind_Launch,
  T_DbgScriptDirectiveKind_Count,
} T_DbgScriptDirectiveKind;

typedef struct T_DbgScriptDirective
{
  struct T_DbgScriptDirective *next;
  T_DbgScriptDirectiveKind kind;
  U64                      line;
  String8                  args;
} T_DbgScriptDirective;

typedef struct
{
  U64                   count;
  T_DbgScriptDirective *first;
  T_DbgScriptDirective *last;
} T_DbgScriptDirectiveList;

typedef struct
{
  String8                   file_path;
  T_DbgScriptDirectiveList  directives[OperatingSystem_COUNT][T_DbgScriptDirectiveKind_Count];
  T_DbgScriptFileList       files;
  U64                       program_count;
  T_DbgScriptProgram      **programs;
} T_DbgScript;

////////////////////////////////
// IPC Controller

typedef struct
{
  B32 running;
  U64 run_gen;
} T_DbgStatus;

typedef struct
{
  U64     vaddr;
  U64     voff;
  String8 file_path;
  TxtPt   pt;
  Rng1U64 voff_range;
} T_DbgSourceLocation;

typedef struct
{
  Arch            arch;
  U64             vaddr_min;
  U64             vaddr_max;
  U64             ip_vaddr;
  U64             sp_base;
  U64             tls_root;
  U64             tls_index;
  U64             tls_offset;
  U64             timestamp;
  U64             exception_code;
  U64             bp_flags;
  String8         string;
  OperatingSystem target_os;
  U64             tls_model;
  String8         stop_cause;
} T_DbgStopEvent;

typedef struct
{
  String8 expr;
  String8 value;
  String8 type;
  String8 error;
} T_Eval;

////////////////////////////////
// Dbg Script

internal void t_dbg_register_script_tests(Arena *arena, String8 folder_path);
internal B32 t_dbg_run_script(Arena *arena, T_DbgScript *script, U64 timeout_us);

////////////////////////////////
// Dbg Tester

internal B32 t_dbg_ping          (U64 timeout_us);
internal B32 t_dbg_run           (U64 timeout_us);
internal B32 t_dbg_kill_all      (U64 timeout_us);
internal B32 t_dbg_halt          (U64 timeout_us);
internal B32 t_dbg_step_over     (U64 timeout_us);
internal B32 t_dbg_step_into     (U64 timeout_us);
internal B32 t_dbg_step_out      (U64 timeout_us);
internal B32 t_dbg_step_over_inst(U64 timeout_us);
internal B32 t_dbg_step_into_inst(U64 timeout_us);
internal B32 t_dbg_step_over_line(U64 timeout_us);
internal B32 t_dbg_step_into_line(U64 timeout_us);
internal B32 t_dbg_launch(String8 cmdline, U64 timeout_us);
internal B32 t_dbg_eval(Arena *arena, String8 expr, T_Eval *eval_out);
// TODO: need a source location query in eval
internal B32 t_dbg_src_line(Arena *arena, U64 vaddr, T_DbgSourceLocation *loc_out, U64 timeout_us);

