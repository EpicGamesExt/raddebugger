// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////
// IPC Controller

typedef struct
{
  B32             running;
  U64             run_gen;
  U64             ip;
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
} T_DbgState;

typedef struct
{
  String8 file_path;
  TxtPt   pt;
  Rng1U64 voff_range;
} T_DbgLine;

typedef struct
{
  U64        count;
  T_DbgLine *v;
} T_DbgLineArray;

typedef struct
{
  String8 expr;
  String8 value;
  String8 type;
  String8 error;
} T_Eval;

////////////////////////////////
// Dbg Script

#define T_DbgScriptCmdKind_XList          \
    X(Breakpoint,       "bp")             \
    X(ClearBreakpoints, "bp_clear")       \
    X(Halt,             "halt")           \
    X(Run,              "run")            \
    X(RunToLine,        "run_to_line")    \
    X(StepOver,         "step_over")      \
    X(StepInto,         "step_into")      \
    X(StepOut,          "step_out")       \
    X(StepOverInst,     "step_over_inst") \
    X(StepIntoInst,     "step_over_inst") \
    X(StepOverLine,     "step_over_line") \
    X(StepIntoLine,     "step_into_line") \
    X(KillAll,          "kll_all")        \
    X(At,               "at")             \
    X(Eval,             "eval")

typedef enum
{
  T_DbgScriptCmdKind_Null,
#define X(n,...) T_DbgScriptCmdKind_##n,
  T_DbgScriptCmdKind_XList
#undef X
  T_DbgScriptCmdKind_Count
} T_DbgScriptCmdKind;

#define T_DbgScriptDirectiveKind_XList \
  X(Compile, "compile")                \
  X(Link,    "link")                   \
  X(Launch,  "launch")                 \
  X(Skip,    "skip")

typedef enum
{
  T_DbgScriptDirectiveKind_Null,
#define X(q,w) T_DbgScriptDirectiveKind_##q,
  T_DbgScriptDirectiveKind_XList
#undef X
  T_DbgScriptDirectiveKind_Count,
} T_DbgScriptDirectiveKind;

typedef struct T_DbgScriptFile
{
  struct T_DbgScriptFile *next;
  String8 path;
  String8 source;
  TxtPt   pt;
} T_DbgScriptFile;

typedef struct
{
  U64              count;
  T_DbgScriptFile *first;
  T_DbgScriptFile *last;
} T_DbgScriptFileList;

typedef struct T_DbgScriptCmd
{
  struct T_DbgScriptCmd *next;
  T_DbgScriptCmdKind kind;
  TxtPt              pt;
  S64                at_delta;
} T_DbgScriptCmd;

typedef struct T_DbgScriptProgram
{
  TxtPt            pt;
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

typedef struct T_DbgScriptDirective
{
  struct T_DbgScriptDirective *next;
  T_DbgScriptDirectiveKind kind;
  TxtPt                    pt;
  String8                  args;
  union {
    struct {
      T_Compiler cc;
    } compile;
  };
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
  B32                       skip;
} T_DbgScript;


////////////////////////////////
// IPC Controller

// error helper
internal void t_errorf_md(String8 file_name, String8 source, MD_Node *n, char *fmt, ...);

// reply parse helpers
internal B32 t_ipc_parse_string(MD_Node *node, String8 child_name, String8 *out);
internal B32 t_ipc_parse_u32(MD_Node *node, String8 child_name, U32 *out);
internal B32 t_ipc_parse_int_(MD_Node *node, String8 child_name, U64 out_size, void *out);
internal B32 t_ipc_parse_b32(MD_Node *node, String8 child_name, B32 *out);
#define t_ipc_parse_int(n, c, ptr) t_ipc_parse_int_(n, c, sizeof(*ptr), ptr)

// debugger commands
internal B32          t_dbg_send_cmd(String8 cmd, U64 timeout_us, Arena *reply_arena, MD_ParseResult *reply_out);
internal B32          t_dbg_send_cmdf(U64 timeout_us, Arena *reply_arena, MD_ParseResult *reply_out, char *fmt, ...);
internal T_DbgState * t_dbg_state(Arena *arena, U64 timeout_us);
internal B32          t_dbg_src_line(Arena *arena, U64 vaddr, T_DbgLineArray *lines_out, U64 timeout_us);
internal String8      t_dbg_value_from_expr(Arena *arena, String8 expr);
internal String8      t_dbg_value_from_exprf(Arena *arena, char *fmt, ...);
internal B32          t_dbg_send_cmd_and_wait_stop(String8 cmd, U64 timeout_us);
internal B32          t_dbg_ping(U64 timeout_us);
internal B32          t_dbg_bp_add_line(String8 file, U64 line);
internal B32          t_dbg_bp_add_func(String8 func_name);
internal B32          t_dbg_bp_add_addr(U64 addr);
internal B32          t_dbg_launch(String8 cmdline, U64 timeout_us);
internal B32          t_dbg_eval(Arena *arena, String8 expr, T_Eval *eval_out);

////////////////////////////////
// Dbg Script

internal String8            t_string_from_dbg_script_cmd_kind(T_DbgScriptCmdKind v);
internal T_DbgScriptCmdKind t_dbg_script_cmd_kind_from_string(String8 cmd);
internal B32                t_dbg_parse_script(Arena *arena, String8 file_path, String8 source, T_DbgScript *script_out);
internal B32                t_dbg_script_invoke(T_DbgScript *script, U64 timeout_us);
internal void               t_dbg_register_script_tests(Arena *arena, String8 folder_path);

