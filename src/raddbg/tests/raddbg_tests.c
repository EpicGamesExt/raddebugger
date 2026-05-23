// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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
      Compiler cc;
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


#define T_Dbg_DefaultTimeout TIMEOUT_SEC(5)

extern B32 g_stop_on_first_fail_or_crash;
extern B32 g_build_only;

////////////////////////////////

internal void
t_errorf_md(String8 file_name, String8 source, MD_Node *n, char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 result = push_str8fv(scratch.arena, fmt, args);
  TxtPt pt = mg_txt_pt_from_string_off(source, n->src_offset);
  t_errorf("ERROR: %S:%llu%llu: %S\n", file_name, (unsigned long long)pt.line, (unsigned long long)pt.column, result);
  va_end(args);
  scratch_end(scratch);
}

////////////////////////////////
// IPC Controller

internal U32 g_dbg_pid;

internal B32
t_ipc_parse_string(MD_Node *node, String8 child_name, String8 *out)
{
  MD_Node *child = md_child_from_string(node, child_name, 0);
  if (!md_node_is_nil(child) && !md_node_is_nil(child->first))
  {
    if (out) { *out = child->first->string; }
    return 1;
  }
  return 0;
}

internal B32
t_ipc_parse_u32(MD_Node *node, String8 child_name, U32 *out)
{
  String8 value = {0};
  if (t_ipc_parse_string(node, child_name, &value))
  {
    U64 v64 = 0;
    if (try_u64_from_str8_c_rules(value, &v64))
    {
      if (out) { *out = safe_cast_u32(v64); }
      return 1;
    }
  }
  return 0;
}

internal B32
t_ipc_parse_int_(MD_Node *node, String8 child_name, U64 out_size, void *out)
{
  String8 value = {0};
  if (t_ipc_parse_string(node, child_name, &value))
  {
    U64 v64 = 0;
    if (try_u64_from_str8_c_rules(value, &v64))
    {
      if (out) { MemoryCopy(out, &v64, out_size); }
      return 1;
    }
  }
  return 0;
}

internal B32
t_ipc_parse_b32(MD_Node *node, String8 child_name, B32 *out)
{
  B32     is_ok = 0;
  String8 s     = {0};
  U64     value = 0;
  if (t_ipc_parse_string(node, child_name, &s))
  {
    if      (str8_matchi(s, str8_lit("true")))  { value = 1; is_ok = 1; }
    else if (str8_matchi(s, str8_lit("false"))) { value = 0; is_ok = 1; }
    else
    {
      is_ok = try_u64_from_str8_c_rules(s, &value);
      value = !!value;
    }
  }
  if (is_ok && out) { *out = value; }
  return is_ok;
}

internal B32
t_dbg_send_cmd(String8 cmd, U64 timeout_us, Arena *reply_arena, MD_ParseResult *reply_out)
{
  Temp scratch = scratch_begin(&reply_arena, 1);
  B32 is_sent = 0;
  
  // send command
  String8 cmdline = str8f(scratch.arena, "--gen_crash_dump --ipc --pid:%u %S", g_dbg_pid, cmd);
  if (t_invoke(t_raddbg_path(), cmdline, timeout_us) == 0) { goto exit; }
  
  B32 has_reply = 1;
  
  char *no_reply_cmds[] = {
    "add_breakpoint",
    "add_function_breakpoint", 
    "add_address_breakpoint",
    "clear_breakpoints",
  };
  for EachElement(i, no_reply_cmds) {
    if (str8_match_wildcard(cmd, str8f(scratch.arena, "%s*", no_reply_cmds[i]), 0)) {
      has_reply = 0;
      break;
    }
  }
  
  if (has_reply) {
    // parse reply
    Arena   *a          = reply_arena ? reply_arena : scratch.arena;
    String8  reply_text = str8_copy(a, g_output);
    
    t_infof("IPC-Reply: \"%S\"\n", reply_text);
    MD_ParseResult reply = md_parse_from_text(a, str8_lit("ipc_reply"), reply_text);
    
    // validate reply
    if (reply.msgs.worst_message_kind >= MD_MsgKind_Error) { goto exit; }
    if (md_node_is_nil(reply.root))                        { goto exit; }
    
    if (reply_arena && reply_out) { *reply_out = reply; }
  }
  
  is_sent = 1;
  exit:;
  scratch_end(scratch);
  return is_sent;
}

internal B32
t_dbg_send_cmdf(U64 timeout_us, Arena *reply_arena, MD_ParseResult *reply_out, char *fmt, ...)
{
  Temp scratch = scratch_begin(&reply_arena, 1);
  va_list args;
  va_start(args, fmt);
  String8 cmd = str8fv(scratch.arena, fmt, args);
  B32 is_ok = t_dbg_send_cmd(cmd, timeout_us, reply_arena, reply_out);
  va_end(args);
  scratch_end(scratch);
  return is_ok;
}

internal T_DbgState *
t_dbg_state(Arena *arena, U64 timeout_us)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  T_DbgState *result = 0;
  
  // send status request
  MD_ParseResult reply = {0};
  if ( ! t_dbg_send_cmd(str8_lit("state"), timeout_us, arena, &reply)) { goto exit; }
  
  // parse reply
  MD_Node *state_md      = md_child_from_string(reply.root, str8_lit("state"), 0);
  MD_Node *stop_event_md = md_child_from_string(state_md, str8_lit("stop_event"), 0);
  MD_Node *threads_md    = md_child_from_string(state_md, str8_lit("threads"),    0);
  MD_Node *modules_md    = md_child_from_string(state_md, str8_lit("modules"),    0);
  T_DbgState v = {0};
  if (!t_ipc_parse_int(state_md, str8_lit("running"), &v.running)) { t_infof("INFO: 'state' is missing 'running'\n"); goto exit; }
  if (!t_ipc_parse_int(state_md, str8_lit("run_gen"), &v.run_gen)) { t_infof("INFO: 'state' is missing 'run_gen'\n"); goto exit; }
  if (!t_ipc_parse_int(state_md, str8_lit("ip"), &v.ip))           { t_infof("INFO: 'state' is missing 'ip'\n");      goto exit; }
  
  result = push_array(arena, T_DbgState, 1);
  *result = v;
  exit:;
  return result;
}

internal B32
t_dbg_src_line(Arena *arena, U64 vaddr, T_DbgLineArray *lines_out, U64 timeout_us)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  B32 is_ok = 0;
  
  // send line map request
  MD_ParseResult reply = {0};
  if(!t_dbg_send_cmdf(timeout_us, arena, &reply, "line_from_vaddr 0x%llx", vaddr)) { goto exit; }
  
  // parse reply
  MD_Node *lines_md = md_child_from_string(reply.root, str8_lit("lines"), 0);
  
  typedef struct Node { struct Node *next; T_DbgLine v; } Node;
  Node *first_line = 0, *last_line = 0;
  U64 line_count = 0;
  
  for MD_EachNode(n, lines_md->first) {
    T_DbgLine line = {0};
    if ( ! t_ipc_parse_string(n, str8_lit("file_path"),      &line.file_path))      { t_infof("INFO: 'lines' is missing 'file_path'\n");      goto exit; }
    if ( ! t_ipc_parse_int   (n, str8_lit("line_num"),       &line.pt.line))        { t_infof("INFO: 'lines' is missing 'line_num'\n");       goto exit; }
    if ( ! t_ipc_parse_int   (n, str8_lit("column_num"),     &line.pt.column))      { t_infof("INFO: 'lines' is missing 'column_num'\n");     goto exit; }
    if ( ! t_ipc_parse_int   (n, str8_lit("voff_range_min"), &line.voff_range.min)) { t_infof("INFO: 'lines' is missing 'voff_range_min'\n"); goto exit; }
    if ( ! t_ipc_parse_int   (n, str8_lit("voff_range_max"), &line.voff_range.max)) { t_infof("INFO: 'lines' is missing 'voff_range_max'\n"); goto exit; }
    
    Node *n = push_array(scratch.arena, Node, 1);
    n->v = line;
    SLLQueuePush(first_line, last_line, n);
    line_count += 1;
  }
  
  if (lines_out && line_count > 0) {
    lines_out->count = 0;
    lines_out->v     = push_array(arena, T_DbgLine, line_count);
    for EachNode(n, Node, first_line) {
      lines_out->v[lines_out->count++] = n->v;
    }
  }
  
  is_ok = 1;
  exit:;
  scratch_end(scratch);
  return is_ok;
}

internal String8
t_dbg_value_from_expr(Arena *arena, String8 expr)
{
  T_Eval eval = {0};
  if ( ! t_dbg_eval(arena, expr, &eval)) { AssertAlways("failed on eval"); }
  return eval.value;
}

internal String8
t_dbg_value_from_exprf(Arena *arena, char *fmt, ...)
{
  Temp scratch = scratch_begin(&arena, 1);
  va_list args;
  va_start(args, fmt);
  String8 expr = push_str8fv(scratch.arena, fmt, args);
  String8 result = t_dbg_value_from_expr(arena, expr);
  va_end(args);
  scratch_end(scratch);
  return result;
}

internal B32
t_dbg_send_cmd_and_wait_stop(String8 cmd, U64 timeout_us)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_stopped = 0;
  
  // snapshot state
  T_DbgState *state_before = t_dbg_state(scratch.arena, max_U64);
  if (state_before == 0) {
    t_errorf("ERROR: failed to snapshot state\n");
    goto exit;
  }
  
  // send command
  if (t_dbg_send_cmd(cmd, max_U64, 0, 0) == 0) {
    t_errorf("ERROR: failed to send command \"%S\"", cmd);
    goto exit;
  }
  
  // wait for debugger to stop
  U64 t = ENDT_US(timeout_us);
  for (;;) {
    // query debugger state
    T_DbgState *state = t_dbg_state(scratch.arena, t);
    if (state == 0) {
      t_errorf("ERROR: failed to fetch debugger state\n");
      goto exit;
    }
    
    // did state change? -> break
    if (!state->running && state->run_gen != state_before->run_gen) {
      is_stopped = 1;
      break;
    }
    
    // "solve" the wait problem
    if (now_time_us() >= t) {
      t_errorf("ERROR: command \"%S\" hit timeout\n", cmd);
      goto exit;
    }
    sleep_ms(10);
  }
  
  //--- state ---------------------
  if (0 && is_stopped) {
    T_DbgState *state = t_dbg_state(scratch.arena, T_Dbg_DefaultTimeout);
    
    String8 process_id     = str8_skip(str8_chop(t_dbg_value_from_exprf(scratch.arena, "query:current_process.id"), 2), 2);
    String8 process_label  = str8_chop(str8_skip(t_dbg_value_from_exprf(scratch.arena, "query:current_process.label"), 2), 2);
    String8 process_active = t_dbg_value_from_exprf(scratch.arena, "query:current_process.active");
    String8 thread_id      = t_dbg_value_from_exprf(scratch.arena, "query:current_thread.id");
    String8 thread_active  = t_dbg_value_from_exprf(scratch.arena, "query:current_thread.active");
    String8 thread_label   = str8_skip(str8_chop(t_dbg_value_from_exprf(scratch.arena, "query:current_thread.label"), 2), 2);
    String8 ip             = t_dbg_value_from_exprf(scratch.arena, "hex(reg:rip)");
    String8 sp             = t_dbg_value_from_exprf(scratch.arena, "hex(reg:rsp)");
    
    T_DbgState *last_stop = t_dbg_state(scratch.arena, T_Dbg_DefaultTimeout);
    if ( ! last_stop) {
      t_errorf("ERROR: debugger state fetch failed\n");
      goto exit;
    }
    
    T_DbgLineArray lines = {0};
    if  ( ! t_dbg_src_line(scratch.arena, last_stop->ip_vaddr, &lines, T_Dbg_DefaultTimeout)) {
      t_errorf("ERROR: failed t map IP(0x%llx) to a source location\n", last_stop->ip_vaddr);
      goto exit;
    }
    
    t_infof("------------------------------------------------------------------------------------------------------------------------\n");
    t_infof("  Process:    %.*s [%.*s] (Active: %.*s)\n", str8_varg(process_id), str8_varg(process_label), str8_varg(process_active));
    t_infof("  Thread:     %.*s [%.*s] (Active: %.*s)\n", str8_varg(thread_id), str8_varg(thread_label), str8_varg(thread_active));
    t_infof("  IP:         %.*s\n", str8_varg(ip));
    t_infof("  SP:         %.*s\n", str8_varg(sp));
    t_infof("  Run Gen:    %llu\n", (unsigned long long)state->run_gen);
    t_infof("  Stop Cause: \"%.*s\"\n", str8_varg(last_stop->stop_cause));
    for EachIndex(i, lines.count) {
      t_infof("  {\n");
      t_infof("    File Path:  %.*s\n", str8_varg(lines.v[i].file_path));
      t_infof("    Line:       %lld\n", (long long)lines.v[i].pt.line);
      t_infof("    Column:     %lld\n", (long long)lines.v[i].pt.column);
      t_infof("  }\n");
    }
    fflush(stdout);
  }
  //--------------------------------
  
  exit:;
  scratch_end(scratch);
  return is_stopped;
}

internal B32
t_dbg_ping(U64 timeout_us) 
{
  Temp scratch = scratch_begin(0,0);
  T_DbgState *state = t_dbg_state(scratch.arena, timeout_us);
  scratch_end(scratch);
  return state != 0;
}

internal B32
t_dbg_bp_add_line(String8 file, U64 line)
{
  return t_dbg_send_cmdf(0,0,0, "add_breakpoint \"%S\":%llu", file, line);
}

internal B32
t_dbg_bp_add_func(String8 func_name)
{
  return t_dbg_send_cmdf(0,0,0, "add_function_breakpoint %S", func_name);
}

internal B32
t_dbg_bp_add_addr(U64 addr)
{
  return t_dbg_send_cmdf(0,0,0, "add_address_breakpoint 0x%llx", addr);
}

internal B32
t_dbg_launch(String8 cmdline, U64 timeout_us)
{
  Temp scratch = scratch_begin(0, 0);
  B32 dbg_ready = 0;
  
  String8 user_path       = t_make_file_path(scratch.arena, str8_lit("test.raddbg_user"));
  String8 project_path    = t_make_file_path(scratch.arena, str8_lit("test.raddbg_project"));
  cmdline = str8f(scratch.arena, "%S --gen_crash_dump --user:\"%S\" --project:\"%S\" --logs:\"%S\" %S", t_raddbg_path(), user_path, project_path, g_wdir, cmdline);
  
  // launch debugger
  ProcessLaunchParams launch_opts = {
    .path        = g_wdir,
    .inherit_env = 1,
    .consoleless = 1,
    .cmd_line    = lnk_arg_list_parse_windows_rules(scratch.arena, cmdline),
  };
  Process dbg_handle = process_launch(&launch_opts);
  if (process_match(dbg_handle, process_zero())) {
    t_errorf("ERROR: failed to launch debugger with this CMDL: %S\n", cmdline);
    goto exit;
  }
  
#if OS_WINDOWS
  // cache debugger PID
  g_dbg_pid = GetProcessId((HANDLE)dbg_handle.u64[0]);
#elif OS_LINUX
  g_dbg_pid = (int)dbg_handle.u64[0];
#else
# error NotImplemented
#endif
  
  // close debugger handle
  process_detach(dbg_handle);
  
  // now wait for debugger to init
  U64 t = ENDT_US(timeout_us);
  for (;;) {
    // time the ping
    dbg_ready = t_dbg_ping(t);
    if (dbg_ready) { break; }
    
    // "solve" the wait problem
    if (now_time_us() >= t) {
      t_errorf("ERROR: failed to launch debugger, because operation timed out\n");
      break;
    }
    sleep_ms(10);
  }
  
  exit:;
  scratch_end(scratch);
  return dbg_ready;
}

internal B32 
t_dbg_eval(Arena *arena, String8 expr, T_Eval *eval_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  MD_ParseResult reply = {0};
  String8        cmd   = str8f(scratch.arena, "eval %llu %S", /* value char cap: */ 10000, expr);
  B32            is_ok = t_dbg_send_cmd(cmd, T_Dbg_DefaultTimeout, arena, &reply);
  
  T_Eval e = {0};
  if ( ! t_ipc_parse_string(reply.root, str8_lit("expr"),  &e.expr))  { t_errorf_md(str8_lit("IPC"), str8_zero(), reply.root, "ERROR: failed to parse reply member: expr\n");  goto exit; }
  if ( ! t_ipc_parse_string(reply.root, str8_lit("value"), &e.value)) { t_errorf_md(str8_lit("IPC"), str8_zero(), reply.root, "ERROR: failed to parse reply member: value\n"); goto exit; }
  if ( ! t_ipc_parse_string(reply.root, str8_lit("type"),  &e.type))  { t_errorf_md(str8_lit("IPC"), str8_zero(), reply.root, "ERROR: failed to parse reply member: type\n");  goto exit; }
  if ( ! t_ipc_parse_string(reply.root, str8_lit("error"), &e.error)) { t_errorf_md(str8_lit("IPC"), str8_zero(), reply.root, "ERROR: failed to parse reply member: error\n"); goto exit; }
  if (eval_out) { *eval_out = e; }
  
  exit:;
  scratch_end(scratch);
  return is_ok;
}

////////////////////////////////
// Dbg Script

force_inline int
t_dbg_script_program_compar(const void *raw_a, const void *raw_b)
{
  T_DbgScriptProgram * const *a = raw_a, * const *b = raw_b;
  return u64_compar(&(*a)->order, &(*b)->order);
}

force_inline int
t_dbg_script_program_is_before(void *raw_a, void *raw_b)
{
  return t_dbg_script_program_compar(raw_a, raw_b) < 0;
}

internal String8
t_string_from_dbg_script_cmd_kind(T_DbgScriptCmdKind v)
{
  switch (v) {
#define X(n,v) case T_DbgScriptCmdKind_##n: return str8_lit(v);
    T_DbgScriptCmdKind_XList
#undef X
    case T_DbgScriptCmdKind_Null: return str8_zero();
    default: InvalidPath; break;
  }
  return str8_zero();
}

internal T_DbgScriptCmdKind
t_dbg_script_cmd_kind_from_string(String8 cmd)
{
  for EachIndex(i, T_DbgScriptCmdKind_Count) {
    if (str8_matchi(t_string_from_dbg_script_cmd_kind(i), cmd)) { return i; }
  }
  return T_DbgScriptCmdKind_Null;
}

internal B32
t_dbg_parse_script(Arena *arena, String8 file_path, String8 source, T_DbgScript *script_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  B32 is_ok = 0;
  
  T_DbgScript script = { .file_path = push_str8_copy(arena, file_path) };
  
  // parse out mdesk out of the comments
  MD_TokenizeResult source_tokens = md_tokenize_from_text(scratch.arena, source);
  MD_TokenChunkList annot_chunks = {0};
  for EachIndex(token_idx, source_tokens.tokens.count) {
    MD_Token source_token = source_tokens.tokens.v[token_idx];
    
    // skip non-comment tokens
    if (~source_token.flags & MD_TokenFlag_Comment) { continue; }
    
    // read token string
    String8 source_token_string = str8_substr(source, source_token.range);
    
    if (str8_match_wildcard(source_token_string, str8_lit("*///*"), 0)) {
      
      // drop comment prefix
      String8 raw_annots = str8_skip(str8_skip_chop_whitespace(source_token_string), 3);
      
      // parse annotations
      MD_TokenizeResult annot_parse = md_tokenize_from_text(scratch.arena, raw_annots);
      
      for EachIndex(i, annot_parse.tokens.count) {
        MD_Token annot_token = annot_parse.tokens.v[i];
        
        // adjust token range so they point back into the source file
        annot_token.range = shift_1u64(annot_token.range, (U64)(raw_annots.str - source.str));
        
        // append annotation token
        md_token_chunk_list_push(scratch.arena, &annot_chunks, 4096, annot_token);
      }
      
      // append new line token
      MD_Token newline_token = md_token_make(r1u64(source_token.range.max, source_token.range.max), MD_TokenFlag_Newline);
      md_token_chunk_list_push(scratch.arena, &annot_chunks, 4096, newline_token);
    }
  }
  
  // script annotations -> mdesk tree
  MD_TokenArray  annot_tokens = md_token_array_from_chunk_list(scratch.arena, &annot_chunks);
  MD_ParseResult script_parse = md_parse_from_text_tokens(scratch.arena, file_path, source, annot_tokens);
  
  // was parse ok? -> error
  if (script_parse.msgs.worst_message_kind >= MD_MsgKind_Error) {
    t_errorf("ERROR: cannot tokenize mdesk file: \"%S\"\n", file_path);
    for EachNode(msg, MD_Msg, script_parse.msgs.first) {
      String8 msg_kind_string = {0};
      switch(msg->kind)
      {
        default:{}break;
        case MD_MsgKind_Note:        {msg_kind_string = str8_lit("note");}break;
        case MD_MsgKind_Warning:     {msg_kind_string = str8_lit("warning");}break;
        case MD_MsgKind_Error:       {msg_kind_string = str8_lit("error");}break;
        case MD_MsgKind_FatalError:  {msg_kind_string = str8_lit("fatal error");}break;
      }
      TxtPt   pt  = mg_txt_pt_from_string_off(source, msg->node->src_offset);
      String8 loc = push_str8f(scratch.arena, "%S:%I64d:%I64d", file_path, pt.line, pt.column);
      t_errorf("  [%S] %S: %S\n", msg_kind_string, loc, msg->string);
    }
    goto exit;
  }
  
  // @test:
  {
    // first child node of the root must be a test header
    MD_Node *test = script_parse.root->first;
    
    // is node test? -> error
    if ( ! str8_matchi(test->string, str8_lit("test"))) {
      t_errorf("ERROR: %S: missing test header\n", file_path);
      goto exit;
    }
    
    for MD_EachNode(n, test->first) {
      OperatingSystem os = operating_system_from_string(n->string);
      
      // is OS string correct? -> error
      if (os == OperatingSystem_Null && n->string.size > 0) {
        t_errorf_md(file_path, source, n, "test is defined for unknown os: \"%S\"\n", n->string);
        goto exit;
      }
      
      for MD_EachNode(field, n->first) {
        // define table for mapping string to a directive kind
        struct {
          T_DbgScriptDirectiveKind kind;
          String8                  string;
        } dir_string_map[] = {
#define X(q,w) { T_DbgScriptDirectiveKind_##q, str8_lit(w) },
          T_DbgScriptDirectiveKind_XList
#undef X
        };
        
        // string -> directive
        T_DbgScriptDirectiveKind kind = T_DbgScriptDirectiveKind_Null;
        for EachElement(i, dir_string_map) {
          if (str8_matchi(field->string, dir_string_map[i].string))  {
            kind = dir_string_map[i].kind;
            break;
          }
        }
        
        // was directive found? -> error
        if (kind == T_DbgScriptDirectiveKind_Null) {
          t_errorf_md(file_path, source, n, "unknown field in test header \"%S\"\n", field->string);
          goto exit;
        }
        
        // alloc directive node
        T_DbgScriptDirective *dir = push_array(arena, T_DbgScriptDirective, 1);
        dir->kind = kind;
        dir->pt   = mg_txt_pt_from_string_off(source, field->src_offset);
        
        // TODO: collapse
        if (field->flags & MD_NodeFlag_HasBraceLeft) {
          // walk each sub-field of the MD node, and find nodes for the directive
          if (kind == T_DbgScriptDirectiveKind_Compile) {
            for MD_EachNode(sub_field, field->first) {
              // compiler override
              if (str8_matchi(sub_field->string, str8_lit("cc"))) {
                MD_Node *cc = sub_field->first;
                if (~cc->flags & MD_NodeFlag_StringLiteral) {
                  t_errorf_md(file_path, source, sub_field, "value of CC must be a string literal e.g. CC: \"clang\"\n");
                  goto exit;
                }
                
                if      (str8_matchi(cc->string, str8_lit("clang"))) { dir->compile.cc = Compiler_clang; }
                else if (str8_matchi(cc->string, str8_lit("cl")))    { dir->compile.cc = Compiler_msvc;    }
                else {
                  t_errorf_md(file_path, source, cc, "unknown compiler name: \"%S\"\n", sub_field->string);
                  goto exit;
                }
              }
              // commnad line for the compiler
              else if (str8_matchi(sub_field->string, str8_lit("args"))) {
                MD_Node *args = sub_field->first;
                if (~args->flags & MD_NodeFlag_StringLiteral) {
                  t_errorf_md(file_path, source, args, "value of ARGS must be a string literal\n");
                  goto exit;
                }
                dir->args = str8_copy(arena, args->string);
              }
              // unknown field
              else {
                t_errorf_md(file_path, source, sub_field, "unknown field \"%S\"\n", sub_field->string);
                goto exit;
              }
            }
          }
        }
        // MD node is a string
        else {
          // is value present? -> error
          if (md_node_is_nil(field->first)) {
            t_errorf_md(file_path, source, field, "missing value on field %S\n", field->string);
            goto exit;
          }
          
          // more than one node? -> error
          if ( ! md_node_is_nil(field->first->next)) {
            t_errorf_md(file_path, source, field, "field %S accepts only one value\n", field->string);
            goto exit;
          }
          
          // node is not a string literal? -> error
          if (~field->first->flags & MD_NodeFlag_StringLiteral) {
            t_errorf_md(file_path, source, field, "field %S accepts only strings\n", field->string);
            goto exit;
          }
          
          // copy string from MD node
          dir->args = str8_copy(arena, field->first->string);
        }
        
        // append new directive
        T_DbgScriptDirectiveList *list = &script.directives[os][kind];
        SLLQueuePush(list->first, list->last, dir);
        list->count += 1;
      }
    }
  }
  
  // @file:
  {
    // collect file nodes
    MD_NodePtrList files = {0};
    for MD_EachNode(n, script_parse.root->first->next) {
      if (str8_matchi(n->string, str8_lit("file"))) {
        
        // is file value a string? -> error
        if ( ! md_node_is_nil(n->first->next) || ! (n->first->flags & MD_NodeFlag_StringLiteral)) {
          t_errorf_md(file_path, source, n, "value of the 'file' must be a string, (e.g. file: \"main.c\")\n");
          goto exit;
        }
        
        // append file node
        md_node_ptr_list_push(scratch.arena, &files, n);
      }
    }
    
    // no nodes? -> treat whole file as a script
    if (files.count == 0) {
      MD_Node *whole_file             = push_array(scratch.arena, MD_Node, 1);
      whole_file->first               = push_array(scratch.arena, MD_Node, 1);
      whole_file->first->next         = push_array(scratch.arena, MD_Node, 1);
      whole_file->src_offset          = 0;
      whole_file->first->string = str8_skip_last_slash(file_path);
      md_node_ptr_list_push(scratch.arena, &files, whole_file);
    }
    
    HashMap files_hm = {0};
    for EachNode(n_ptr, MD_NodePtrNode, files.first) {
      MD_Node *n = n_ptr->v;
      
      // get file name
      String8 file_name = n->first->string;
      
      // was file seen? -> error
      MD_Node *is_declared = hash_map_search_string_raw(&files_hm, file_name);
      if (is_declared) {
        TxtPt pt = mg_txt_pt_from_string_off(source, is_declared->src_offset);
        t_errorf_md(file_path, source, n, "file %S is already declared at line %llu", file_name, pt.line);
        goto exit;
      }
      hash_map_push_string_raw(scratch.arena, &files_hm, file_name, n);
      
      // file ends at EOF or before the next file directive
      U64 src_opl = source.size;
      if (n_ptr->next) {
        src_opl = n_ptr->next->v->src_offset;
      }
      
      // sub-string the source file
      String8 sub_source = str8_substr(source, r1u64(n->src_offset, src_opl));
      U64     file_min   = str8_find_needle(sub_source, 0, str8_lit("\n"), 0) + 1;
      U64     file_max   = str8_find_needle_reverse(sub_source, 0, str8_lit("\n"), 0);
      sub_source = str8_substr(sub_source, r1u64(file_min, file_max));
      
      // append new file node
      T_DbgScriptFile *file = push_array(arena, T_DbgScriptFile, 1);
      file->path   = t_make_file_path(arena, file_name);
      file->source = sub_source;
      file->pt     = mg_txt_pt_from_string_off(source, n->src_offset);
      SLLQueuePush(script.files.first, script.files.last, file);
      script.files.count += 1;
    }
  }
  
  // @program:
  {
    HashMap hm = {0}; // (U64, T_DbgScriptProgram)
    for MD_EachNode(n, script_parse.root->first->next) {
      // find order number nodes
      U64 order = 0;
      if ( ! try_u64_from_str8_c_rules(n->string, &order)) {
        continue;
      }
      
      // correlate MD node to the script file
      T_DbgScriptFile *file = 0;
      for (file = script.files.first; file != 0; file = file->next) {
        if (file->source.str <= n->string.str && n->string.str < (file->source.str + file->source.size)) {
          break;
        }
      }
      
      // found file? -> error
      if (file == 0) {
        t_errorf_md(file_path, source, n, "failed to correlate MD node to the script source file\n");
        goto exit;
      }
      
      // is order number unique? -> error
      T_DbgScriptProgram *p = hash_map_search_u64_raw(&hm, order);
      if (p) {
        t_errorf_md(file_path, source, n, "duplicate order number %llu found, previous defined at %llu\n", order, p->pt.line);
        goto exit;
      }
      
      // alloc & fill out program
      p = push_array(arena, T_DbgScriptProgram, 1);
      p->pt    = mg_txt_pt_from_string_off(source, n->src_offset);
      p->order = order;
      p->os    = OperatingSystem_CURRENT;
      p->file  = file;
      hash_map_push_u64_raw(scratch.arena, &hm, order, p);
      
      for MD_EachNode(cmd_n, n->first) {
        // alloc & fill out command
        T_DbgScriptCmd *cmd = push_array(arena, T_DbgScriptCmd, 1);
        cmd->kind = t_dbg_script_cmd_kind_from_string(cmd_n->string);
        cmd->pt   = mg_txt_pt_from_string_off(source, n->src_offset);
        SLLQueuePush(p->first, p->last, cmd);
        p->count += 1;
        
        // parse cmd args
        MD_Node *cmd_arg = cmd_n->first;
        
        if (md_node_is_nil(cmd_arg)) { continue; }
        
        if (cmd->kind == T_DbgScriptCmdKind_At) {
          if ( ! try_s64_from_str8_c_rules(cmd_arg->string, &cmd->at_delta)) {
            t_errorf_md(file_path, source, cmd_arg, "failed to parse \"%S\"", cmd_arg->string);
            goto exit;
          }
        } else if (cmd->kind == T_DbgScriptCmdKind_Eval) {
          t_errorf_md(file_path, source, cmd_arg, "TODO: Eval\n");
        } else if (cmd->kind == T_DbgScriptCmdKind_Breakpoint) {
          t_errorf_md(file_path, source, cmd_arg, "TODO: Breakpoint\n");
        }
      }
    }
    
    // extract commands and sort based on order number
    script.program_count = hm.count;
    script.programs      = values_from_hash_map_raw(arena, &hm);
    radsort(script.programs, script.program_count, t_dbg_script_program_is_before);
  }
  
  is_ok = 1;
  exit:;
  if (script_out) { *script_out = script; }
  scratch_end(scratch);
  return is_ok;
}

internal B32
t_dbg_script_invoke(T_DbgScript *script, U64 timeout_us)
{
  Temp scratch = scratch_begin(0,0);
  
  B32 is_ok = 0;
  
  for EachIndex(i, script->program_count) {
    T_DbgScriptProgram *program = script->programs[i];
    
    if (program->os == OperatingSystem_CURRENT) {
      for EachNode(cmd, T_DbgScriptCmd, program->first) {
        t_infof("[%llu] Command: %S:%llu %S\n", program->order, script->file_path, cmd->pt.line, t_string_from_dbg_script_cmd_kind(cmd->kind));
        
        switch (cmd->kind) {
          case T_DbgScriptCmdKind_Null:             break;
          case T_DbgScriptCmdKind_Halt:             t_dbg_send_cmd_and_wait_stop(str8_lit("halt"),           timeout_us); break; // NOTE: does not auto-magically select main thread on stop
          case T_DbgScriptCmdKind_StepOver:         t_dbg_send_cmd_and_wait_stop(str8_lit("step_over"),      timeout_us); break;
          case T_DbgScriptCmdKind_StepInto:         t_dbg_send_cmd_and_wait_stop(str8_lit("step_into"),      timeout_us); break;
          case T_DbgScriptCmdKind_StepOut:          t_dbg_send_cmd_and_wait_stop(str8_lit("step_out"),       timeout_us); break;
          case T_DbgScriptCmdKind_StepOverInst:     t_dbg_send_cmd_and_wait_stop(str8_lit("step_over_inst"), timeout_us); break;
          case T_DbgScriptCmdKind_StepIntoInst:     t_dbg_send_cmd_and_wait_stop(str8_lit("step_into_inst"), timeout_us); break;
          case T_DbgScriptCmdKind_StepOverLine:     t_dbg_send_cmd_and_wait_stop(str8_lit("step_over_line"), timeout_us); break;
          case T_DbgScriptCmdKind_StepIntoLine:     t_dbg_send_cmd_and_wait_stop(str8_lit("step_into_line"), timeout_us); break;
          case T_DbgScriptCmdKind_KillAll:          t_dbg_send_cmd_and_wait_stop(str8_lit("kill_all"),       timeout_us); break;
          case T_DbgScriptCmdKind_Breakpoint:       NotImplemented; break;
          case T_DbgScriptCmdKind_ClearBreakpoints: t_dbg_send_cmdf(0,0,0, "clear_breakpoints"); break;
          case T_DbgScriptCmdKind_Run:              t_dbg_send_cmd(str8_lit("run"),  timeout_us, 0, 0); break;
          case T_DbgScriptCmdKind_RunToLine: {
            U64 line = cmd->pt.line - program->file->pt.line;
            t_dbg_send_cmd_and_wait_stop(str8f(scratch.arena, "run_to_line \"%S:%llu\"", program->file->path, line), timeout_us);
          } break;
          case T_DbgScriptCmdKind_At: {
            // TODO: debugger does not populate eval cache with registers before first frame -- for now use lower level option
#if 0
            U64 ip = u64_from_str8(t_dbg_value_from_exprf(scratch.arena, "reg:rip"), 10);
            if (ip == 0) {
              fprintf(stderr, "ERROR: invalid IP address: 0x%llx\n", (unsigned long long)ip);
              goto exit;
            }
#else
            T_DbgState *temp_status = t_dbg_state(scratch.arena, T_Dbg_DefaultTimeout);
            if (temp_status == 0) {
              t_errorf("ERROR: %S:%llu: failed to query IP\n", script->file_path, cmd->pt.line);
              goto exit;
            }
            U64 ip = temp_status->ip;
#endif
            
            // map IP -> source location
            T_DbgLineArray lines = {0};
            if (t_dbg_src_line(scratch.arena, ip, &lines, T_Dbg_DefaultTimeout) == 0) {
              t_errorf("ERROR: %S:%llu: IP (0x%llx) does not map to a source line\n", script->file_path, cmd->pt.line, ip);
              goto exit;
            }
            
            // compute line where debugger must be
            S64 at_line_s64 = (S64)(program->pt.line - program->file->pt.line) + cmd->at_delta;
            U64 at_line_u64 = at_line_s64 >= 0 ? (U64)at_line_s64 : 0;
            AssertAlways(at_line_u64 > 0);
            
            if (lines.count == 0) {
              t_errorf("ERROR: %S:%llu:%llu: no source location maps for vaddr: 0x%llx\n", script->file_path, cmd->pt.line, cmd->pt.column, ip);
              goto exit;
            }
            
            // match expected vs current debugger locations
            for EachIndex(i, lines.count) {
              B32 mismatch = lines.v[i].pt.line != at_line_u64 || 
                !str8_match(lines.v[i].file_path, program->file->path, StringMatchFlag_CaseInsensitive|StringMatchFlag_SlashInsensitive);
              if (mismatch) {
                t_errorf("ERROR: %S:%llu: location check did not pass:\n", script->file_path, cmd->pt.line);
                t_errorf("  Expected: %S:%llu\n", program->file->path, at_line_u64);
                t_errorf("  Got     : %S:%llu\n", lines.v[i].file_path, lines.v[i].pt.line);
                t_errorf("  IP      : 0x%llx\n",  ip);
                goto exit;
              }
            }
          } break;
          case T_DbgScriptCmdKind_Eval: NotImplemented; break;
          default: { InvalidPath; }
        }
      }
    }
    
    is_ok = (i+1 == script->program_count);
  }
  
  exit:;
  scratch_end(scratch);
  return is_ok;
}

internal
T_RunSig(dbg_script_runner)
{
  // read source file
  String8 source = data_from_file_path(arena, user_data);
  if (source.size == 0) {
    t_errorf("ERROR: failed to read script: \"%S\"\n", user_data);
    T_Ok(0);
  }
  
  // source -> script
  T_DbgScript script = {0};
  if ( ! t_dbg_parse_script(arena, user_data, source, &script)) {
    result_out->status = TestStatus_Fail;
    goto exit;
  }
  
  // write source files to test folder
  for EachNode(file, T_DbgScriptFile, script.files.first) {
    if (write_data_to_file_path(file->path, file->source) == 0) {
      t_errorf("ERROR: %S:%llu: failed to write: \"%S\"\n", user_data, file->pt.line, file->path);
      T_Ok(0);
    }
  }
  
  // compiler vars
  HashTable *script_vars = hash_table_init(arena, 1000);
  hash_table_push_path_string(arena, script_vars, str8_lit("FILE"), user_data);
  hash_table_push_path_string(arena, script_vars, str8_lit("CWD"), g_wdir);
  hash_table_push_path_string(arena, script_vars, str8_lit("SRC"), t_src_path());
  
  // run compilers
  for EachNode(directive, T_DbgScriptDirective, script.directives[OperatingSystem_CURRENT][T_DbgScriptDirectiveKind_Compile].first) {
    Compiler compiler = directive->compile.cc;
    
    // pick default compiler if none selected
    if (directive->compile.cc == Compiler_Null) {
      switch (OperatingSystem_CURRENT) {
        case OperatingSystem_Windows: { compiler = Compiler_msvc;    } break;
        case OperatingSystem_Linux:   { compiler = Compiler_clang; } break;
      }
    }
    
    // get compiler path
    String8 compiler_path = {0};
    switch (compiler) {
      case Compiler_Null: break;
      case Compiler_msvc:  compiler_path = t_cl_path();    break;
      case Compiler_clang: compiler_path = t_clang_path(); break;
      case Compiler_gcc:   compiler_path = t_gcc_path();   break;
    }
    
    // invoke compiler with arguments from directive
    String8 expanded_args = lnk_expand_env_vars_windows(arena, script_vars, directive->args);
    
    if (compiler == Compiler_msvc) { expanded_args = str8f(arena, "/nologo %S", expanded_args); }
    
    if (t_invoke(compiler_path, expanded_args, max_U64) == 0) {
      t_errorf("ERROR: failed to launch compiler: \"%S %S\"\n", compiler_path, expanded_args);
      T_Ok(0);
    }
    
    if (compiler == Compiler_msvc) { g_output = str8_skip(g_output, str8_chop_line(&g_output).size); } // file name print
    
    if (g_last_exit_code) {
      t_errorf("ERROR: %S:%llu: %S\n", script.file_path, (unsigned long long)directive->pt.line, g_errors);
      if (g_stop_on_first_fail_or_crash) {
        T_Ok(0);
      }
    }
  }
  
  // run linkers
  String8 linker_path = t_radlink_path();
  for EachNode(directive, T_DbgScriptDirective, script.directives[OperatingSystem_CURRENT][T_DbgScriptDirectiveKind_Link].first) {
    String8 expanded_args = lnk_expand_env_vars_windows(arena, script_vars, directive->args);
    if (t_invoke(linker_path, expanded_args, max_U64) == 0) {
      t_errorf("ERROR: failed to launch linker: \"%S %S\"\n", linker_path, expanded_args);
      T_Ok(0);
    }
    if (g_last_exit_code != 0) {
      t_errorf("ERROR: %S:%llu: %S\n", script.file_path, (unsigned long long)directive->pt.line, g_errors);
      T_Ok(0);
    }
  }
  
  // is skip flag set? -> exit
  if (g_build_only || script.directives[OperatingSystem_CURRENT][T_DbgScriptDirectiveKind_Skip].count) {
    result_out->status = TestStatus_Skip;
    goto exit;
  }
  
  // launch targets
  for EachNode(directive, T_DbgScriptDirective, script.directives[OperatingSystem_CURRENT][T_DbgScriptDirectiveKind_Launch].first) {
    String8 expanded_args = lnk_expand_env_vars_windows(arena, script_vars, directive->args);
    String8 cmdl = str8f(arena, "--user:%S.raddbg_user %S", t_make_file_path(arena, str8_lit("temp")), expanded_args);
    if (t_dbg_launch(cmdl, T_Dbg_DefaultTimeout) == 0) {
      t_errorf("ERROR: failed to launch debugger with command line \"%S %S\"; work dir \"%S\"\n", t_raddbg_path(), cmdl, g_wdir);
      T_Ok(0);
    }
  }
  
  // debugger is ready -- now invoke script
  if (t_dbg_script_invoke(&script, T_Dbg_DefaultTimeout) == 0) {
    t_errorf("ERROR: %S: failed to run to completion\n", user_data);
    T_Ok(0);
  }
  
  // clean up
#if OS_WINDOWS
  HANDLE dbg_handle = OpenProcess(PROCESS_ALL_ACCESS, 0, g_dbg_pid);
  if (dbg_handle != 0) {
    TerminateProcess(dbg_handle, 1);
    CloseHandle(dbg_handle);
  }
#else
  kill(g_dbg_pid, SIGKILL);
#endif
  
  exit:;
  g_dbg_pid = 0;
}

internal void
t_dbg_register_script_tests(Arena *arena, String8 folder_path)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  if ( ! folder_path_exists(folder_path)) {
    t_errorf("ERROR: this folder does not exists: %S\n", folder_path);
    return;
  }
  
  // gather file paths in a folder
  String8List paths = t_file_paths_from_dir(arena, folder_path);
  
  // register a test for each file
  for EachNode(n, String8Node, paths.first) {
    String8 file_path = n->string;
    
    // test files may contain dots for extensions we have to escape them when creating output folder for a test
    String8List file_name_parts   = str8_split_by_string_chars(scratch.arena, str8_skip_last_slash(file_path), str8_lit("."), 0);
    String8     file_name_escaped = str8_list_join(arena, &file_name_parts, &(StringJoin){.sep=str8_lit("-"), .post = str8_lit("\0") });
    
    g_torture_tests_[g_torture_test_count++] = (T_Test){
      .file      = "raddbg",
      .label     = (char*)file_name_escaped.str,
      .r         = t_dbg_script_runner,
      .user_data = str8_copy(arena, file_path),
    };
  }
  
  scratch_end(scratch);
}

