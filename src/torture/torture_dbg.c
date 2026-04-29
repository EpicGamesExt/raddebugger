// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define T_Group "Dbg"

////////////////////////////////
// IPC Controller

internal OS_Handle g_dbg_handle;

internal B32
t_dbg_send_cmd(String8 cmd, U64 timeout_us, Arena *reply_arena, RD_IpcReply *reply_out)
{
  Temp scratch = scratch_begin(&reply_arena, 1);
  B32 is_sent = 0;

#if OS_WINDOWS
  U32 dbg_pid = GetProcessId((HANDLE)g_dbg_handle.u64[0]);
#elif OS_LINUX
  U32 dbg_pid = safe_cast_u32(handle.u64[0]);
#else
# error NotImplemented
#endif

  // send command
  String8 cmdline = str8f(scratch.arena, "--gen_crash_dump --ipc --pid:%u %S", dbg_pid, cmd);
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
    Arena       *a          = reply_arena ? reply_arena : scratch.arena;
    String8      reply_text = str8_copy(a, g_output);
    RD_IpcReply  reply      = rd_ipc_mdesk_reply_from_string(a, reply_text);
    if (rd_ipc_reply_is_ok(&reply) == 0) { goto exit; }
    if (md_node_is_nil(reply.msg))       { goto exit; }
    if (reply_arena && reply_out) { *reply_out = reply; }
  }

  is_sent = 1;
  exit:;
  scratch_end(scratch);
  return is_sent;
}

internal B32
t_dbg_send_cmdf(U64 timeout_us, Arena *reply_arena, RD_IpcReply *reply_out, char *fmt, ...)
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

internal B32
t_dbg_status(T_DbgStatus *status_out, U64 timeout_us)
{
  Temp scratch = scratch_begin(0, 0);
  T_DbgStatus status = {0};
  B32 is_ok = 0;

  // send status request
  RD_IpcReply reply = {0};
  if ( ! t_dbg_send_cmd(str8_lit("status"), timeout_us, scratch.arena, &reply)) { goto exit; }

  // parse reply
  if ( ! rd_ipc_parse_b32(reply.msg, str8_lit("ok"),      &is_ok))          { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_b32(reply.msg, str8_lit("running"), &status.running)) { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int(reply.msg, str8_lit("run_gen"), &status.run_gen)) { AssertAlways(0); goto exit; }
  if (status_out != 0) { *status_out = status; }

  exit:;
  return is_ok;
}

internal B32
t_dbg_stop_event(Arena *arena, T_DbgStopEvent *out, U64 timeout_us)
{
  Temp scratch = scratch_begin(0, 0);

  T_DbgStopEvent v = {0};
  B32 is_ok = 0;

  // send status request
  RD_IpcReply reply = {0};
  if ( ! t_dbg_send_cmd(str8_lit("stop_event"), timeout_us, arena ? arena : scratch.arena, &reply)) { goto exit; }

  // parse reply
  if ( ! rd_ipc_parse_b32    (reply.msg, str8_lit("ok"),             &is_ok))            { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("arch"),           &v.arch))           { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("vaddr_min"),      &v.vaddr_min))      { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("vaddr_max"),      &v.vaddr_max))      { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("ip_vaddr"),       &v.ip_vaddr))       { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("sp_base"),        &v.sp_base))        { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("tls_root"),       &v.tls_root))       { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("tls_index"),      &v.tls_index))      { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("tls_offset"),     &v.tls_offset))     { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("timestamp"),      &v.timestamp))      { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("exception_code"), &v.exception_code)) { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("bp_flags"),       &v.bp_flags))       { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_string (reply.msg, str8_lit("string"),         &v.string))         { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("target_os"),      &v.target_os))      { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int    (reply.msg, str8_lit("tls_model"),      &v.tls_model))      { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_string (reply.msg, str8_lit("stop_cause"),     &v.stop_cause))     { AssertAlways(0); goto exit; }

  if (arena && out != 0) { *out = v; }

  exit:;
  return is_ok;
}

internal B32
t_dbg_src_line(Arena *arena, U64 vaddr, T_DbgSourceLocation *loc_out, U64 timeout_us)
{
  Temp scratch = scratch_begin(&arena, 1);

  B32 is_ok = 0;

  RD_IpcReply reply = {0};
  if(!t_dbg_send_cmdf(timeout_us, arena, &reply, "source_location_from_address 0x%llx", vaddr)) { goto exit; }

  T_DbgSourceLocation loc = {0};

  if ( ! rd_ipc_parse_b32   (reply.msg, str8_lit("ok"),        &is_ok))             { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int   (reply.msg, str8_lit("vaddr"),     &loc.vaddr))         { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int   (reply.msg, str8_lit("voff"),      &loc.voff))          { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_string(reply.msg, str8_lit("file_path"), &loc.file_path))     { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int   (reply.msg, str8_lit("line"),      &loc.pt.line))       { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int   (reply.msg, str8_lit("column"),    &loc.pt.column))     { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int   (reply.msg, str8_lit("voff_min"),  &loc.voff_range.min)){ AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_int   (reply.msg, str8_lit("voff_max"),  &loc.voff_range.max)){ AssertAlways(0); goto exit; }

  if (loc_out != 0) { *loc_out = loc; }

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

  // snapshot status
  T_DbgStatus status_before = {0};
  if (t_dbg_status(&status_before, max_U64) == 0) { Assert(0 && "failed to snapshot status"); goto exit; }

  // send command
  if (t_dbg_send_cmd(cmd, max_U64, 0, 0) == 0) { Assert(0 && "failed to the command"); goto exit; }

  // wait for debugger to stop
  U64 t = timeout_us;
  do {
    // query debugger status
    U64 begint_us = os_now_microseconds();
    T_DbgStatus status = {0};
    if (t_dbg_status(&status, t) == 0) { Assert(0 && "failed to acquire debugger status"); goto exit; }
    U64 endt_us = os_now_microseconds();

    // did state change? -> break
    if (!status.running && status.run_gen != status_before.run_gen) {
      is_stopped = 1;
      break;
    }

    U64 dt_us = (endt_us - begint_us) + TIMEOUT_MS(10);
    if (dt_us >= t) { break; }
    t -= dt_us;

    // "solve" the wait problem
    os_sleep_milliseconds(10);
  } while (t > 0);

  //--- Status ---------------------
  if (is_stopped) {
    T_DbgStatus status = {0};
    AssertAlways(t_dbg_status(&status, TIMEOUT_SEC(5)));

    String8 process_id     = str8_skip(str8_chop(t_dbg_value_from_exprf(scratch.arena, "query:current_process.id"), 2), 2);
    String8 process_label  = str8_chop(str8_skip(t_dbg_value_from_exprf(scratch.arena, "query:current_process.label"), 2), 2);
    String8 process_active = t_dbg_value_from_exprf(scratch.arena, "query:current_process.active");
    String8 thread_id      = t_dbg_value_from_exprf(scratch.arena, "query:current_thread.id");
    String8 thread_active  = t_dbg_value_from_exprf(scratch.arena, "query:current_thread.active");
    String8 thread_label   = str8_skip(str8_chop(t_dbg_value_from_exprf(scratch.arena, "query:current_thread.label"), 2), 2);
    String8 ip             = t_dbg_value_from_exprf(scratch.arena, "hex(reg:rip)");
    String8 sp             = t_dbg_value_from_exprf(scratch.arena, "hex(reg:rsp)");

    T_DbgStopEvent last_stop = {0};
    AssertAlways(t_dbg_stop_event(scratch.arena, &last_stop, TIMEOUT_SEC(5)));

    T_DbgSourceLocation loc = {0};
    AssertAlways(t_dbg_src_line(scratch.arena, last_stop.ip_vaddr, &loc, TIMEOUT_SEC(5)));

    printf("------------------------------------------------------------------------------------------------------------------------\n");
    printf("  Process:    %.*s [%.*s] (Active: %.*s)\n", str8_varg(process_id), str8_varg(process_label), str8_varg(process_active));
    printf("  Thread:     %.*s [%.*s] (Active: %.*s)\n", str8_varg(thread_id), str8_varg(thread_label), str8_varg(thread_active));
    printf("  IP:         %.*s\n", str8_varg(ip));
    printf("  SP:         %.*s\n", str8_varg(sp));
    printf("  File Path:  %.*s\n", str8_varg(loc.file_path));
    printf("  Line:       %lld\n", loc.pt.line);
    printf("  Column:     %lld\n", loc.pt.column);
    printf("  Run Gen:    %llu\n", status.run_gen);
    printf("  Stop Cause: \"%.*s\"\n", str8_varg(last_stop.stop_cause));
    fflush(stdout);
  }
  //--------------------------------

  exit:;
  scratch_end(scratch);
  return is_stopped;
}

internal B32 t_dbg_ping       (U64 timeout_us)         { return t_dbg_status(0, timeout_us); }
internal B32 t_dbg_bp_add_line(String8 file, U64 line) { return t_dbg_send_cmdf(0,0,0, "add_breakpoint \"%S\":%llu", file, line); }
internal B32 t_dbg_bp_add_func(String8 func_name)      { return t_dbg_send_cmdf(0,0,0, "add_function_breakpoint %S", func_name);  }
internal B32 t_dbg_bp_add_addr(U64 addr)               { return t_dbg_send_cmdf(0,0,0, "add_address_breakpoint 0x%llx", addr);    }

internal B32
t_dbg_launch(String8 cmdline, U64 timeout_us)
{
  Temp scratch = scratch_begin(0, 0);
  B32 dbg_ready = 0;

  String8 user_path       = t_make_file_path(scratch.arena, str8_lit("test.raddbg_user"));
  String8 project_path    = t_make_file_path(scratch.arena, str8_lit("test.raddbg_project"));
  String8 crash_dump_path = t_make_file_path(scratch.arena, str8_lit("raddbg_crash_dump.dmp"));
  cmdline = str8f(scratch.arena, "--gen_crash_dump --crash_dump_path:\"%S\" --user:\"%S\" --project:\"%S\" %S", crash_dump_path, user_path, project_path, cmdline);

  // launch debugger
  OS_ProcessLaunchParams launch_opts = {
    .path        = g_wdir,
    .inherit_env = 1,
    .cmd_line    = lnk_arg_list_parse_windows_rules(scratch.arena, cmdline),
  };
  str8_list_push_front(scratch.arena, &launch_opts.cmd_line, t_raddbg_path());
  g_dbg_handle = os_process_launch(&launch_opts);
  if (os_handle_match(g_dbg_handle, os_handle_zero())) { AssertAlways(0 && "failed to launch debugger"); goto exit; }
  os_process_join(g_dbg_handle, 0, 0);

  // now wait for debugger to init
  U64 t = timeout_us;
  do {
    // time the ping
    U64 ping_begint_us = os_now_microseconds(); 
    dbg_ready = t_dbg_ping(t);
    if (dbg_ready) { break; }
    U64 ping_endt_us = os_now_microseconds();

    // dbg did not pong -> compute remaining timeout and loop back
    U64 ping_dt_us = (ping_endt_us - ping_begint_us) + TIMEOUT_MS(10);
    if (ping_dt_us >= t) { break; }
    t -= ping_dt_us;

    // "solve" the wait problem
    os_sleep_milliseconds(10);
  } while (t > 0);

  exit:;
  scratch_end(scratch);
  return dbg_ready;
}

internal B32 
t_dbg_eval(Arena *arena, String8 expr, T_Eval *eval_out)
{
  Temp scratch = scratch_begin(&arena, 1);

  RD_IpcReply reply = {0};
  String8     cmd   = str8f(scratch.arena, "eval %llu %S", /* value char cap: */ 10000, expr);
  B32         is_ok = t_dbg_send_cmd(cmd, TIMEOUT_SEC(5), arena, &reply);

  T_Eval e = {0};
  if ( ! rd_ipc_parse_string(reply.msg, str8_lit("expr"),  &e.expr))  { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_string(reply.msg, str8_lit("value"), &e.value)) { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_string(reply.msg, str8_lit("type"),  &e.type))  { AssertAlways(0); goto exit; }
  if ( ! rd_ipc_parse_string(reply.msg, str8_lit("error"), &e.error)) { AssertAlways(0); goto exit; }
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

internal T_DbgScriptCmdKind
t_dbg_script_cmd_kind_from_string(String8 cmd)
{
  if      (str8_matchi(cmd, str8_lit("bp")))             { return T_DbgScriptCmdKind_Breakpoint;       }
  else if (str8_matchi(cmd, str8_lit("bp_clear")))       { return T_DbgScriptCmdKind_ClearBreakpoints; }
  else if (str8_matchi(cmd, str8_lit("run")))            { return T_DbgScriptCmdKind_Run;              }
  else if (str8_matchi(cmd, str8_lit("halt")))           { return T_DbgScriptCmdKind_Halt;             }
  else if (str8_matchi(cmd, str8_lit("step_over")))      { return T_DbgScriptCmdKind_StepOver;         }
  else if (str8_matchi(cmd, str8_lit("step_into")))      { return T_DbgScriptCmdKind_StepInto;         }
  else if (str8_matchi(cmd, str8_lit("step_out")))       { return T_DbgScriptCmdKind_StepOut;          }
  else if (str8_matchi(cmd, str8_lit("step_over_inst"))) { return T_DbgScriptCmdKind_StepOverInst;     }
  else if (str8_matchi(cmd, str8_lit("step_into_inst"))) { return T_DbgScriptCmdKind_StepIntoInst;     }
  else if (str8_matchi(cmd, str8_lit("step_over_line"))) { return T_DbgScriptCmdKind_StepOverLine;     }
  else if (str8_matchi(cmd, str8_lit("step_into_line"))) { return T_DbgScriptCmdKind_StepIntoLine;     }
  else if (str8_matchi(cmd, str8_lit("at")))             { return T_DbgScriptCmdKind_At;               }
  else if (str8_matchi(cmd, str8_lit("eval")))           { return T_DbgScriptCmdKind_Eval;             }
  return T_DbgScriptCmdKind_Null;
}

internal T_DbgScript
t_dbg_script_from_source(Arena *arena, String8 file_path, String8 source)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  T_DbgScript script = { .file_path = push_str8_copy(arena, file_path) };

  // scrape MD comment tokens out of source while preserving original source offsets
  MD_TokenArray script_tokens = {0};
  {
    MD_TokenizeResult source_tokens       = md_tokenize_from_text(scratch.arena, source);
    MD_TokenChunkList script_token_chunks = {0};
    for EachIndex(token_idx, source_tokens.tokens.count) {
      MD_Token token = source_tokens.tokens.v[token_idx];
      if (token.flags & MD_TokenFlag_Comment) {
        String8 token_string = str8_substr(source, token.range);
        String8 comment      = str8_skip_chop_whitespace(token_string);
        String8 prefix       = str8_lit("///");
        if (str8_matchi(str8_prefix(comment, prefix.size), prefix)) {
          String8           script_part          = str8_skip(comment, prefix.size);
          U64               script_part_base_off = (U64)(script_part.str - source.str);
          MD_TokenizeResult script_part_tokenize = md_tokenize_from_text(scratch.arena, script_part);
          for EachIndex(script_token_idx, script_part_tokenize.tokens.count) {
            MD_Token script_token = script_part_tokenize.tokens.v[script_token_idx];
            script_token.range.min += script_part_base_off;
            script_token.range.max += script_part_base_off;
            md_token_chunk_list_push(scratch.arena, &script_token_chunks, 4096, script_token);
          }
          MD_Token newline_token = md_token_make(r1u64(token.range.max, token.range.max), MD_TokenFlag_Newline);
          md_token_chunk_list_push(scratch.arena, &script_token_chunks, 4096, newline_token);
        }
      }
    }
    script_tokens = md_token_array_from_chunk_list(scratch.arena, &script_token_chunks);
  }

  // script tokens -> mdesk tree
  MD_ParseResult script_parse  = md_parse_from_text_tokens(scratch.arena, file_path, source, script_tokens);
  AssertAlways(script_parse.msgs.worst_message_kind < MD_MsgKind_Error);

  // test
  {
    MD_Node *test = script_parse.root->first;
    AssertAlways( ! md_node_is_nil(test));
    AssertAlways(str8_matchi(test->string, str8_lit("test")));

    String8 os_name = string_from_operating_system(OperatingSystem_CURRENT);

    for MD_EachNode(n, test->first) {
      OperatingSystem os = operating_system_from_string(n->string);
      AssertAlways(os != OperatingSystem_Null);

      for MD_EachNode(field, n->first) {
        T_DbgScriptDirectiveKind kind = T_DbgScriptDirectiveKind_Null;
        if      (str8_matchi(field->string, str8_lit("compile"))) { kind = T_DbgScriptDirectiveKind_Compile; }
        else if (str8_matchi(field->string, str8_lit("link")))    { kind = T_DbgScriptDirectiveKind_Link;    }
        else if (str8_matchi(field->string, str8_lit("launch")))  { kind = T_DbgScriptDirectiveKind_Launch;  }
        AssertAlways(kind != T_DbgScriptDirectiveKind_Null);

        // syntax check
        AssertAlways( !md_node_is_nil(field->first));
        AssertAlways(md_node_is_nil(field->first->next));
        Assert(field->first->flags & MD_NodeFlag_StringLiteral);

        // src_offset -> line
        //
        // TODO: super silly!! mdesk should export line numbers
        U64 line = 1;
        String8 text_before_src = str8_prefix(source, field->src_offset);
        for EachIndex(idx, text_before_src.size) { line += (text_before_src.str[idx] == '\n'); }

        T_DbgScriptDirective *n = push_array(arena, T_DbgScriptDirective, 1);
        n->kind = kind;
        n->line = line;
        n->args = str8_copy(arena, field->first->string);
        // TODO: expand % in compile: and link: to current source file name and esacpe with %%

        T_DbgScriptDirectiveList *list = &script.directives[os][kind];
        SLLQueuePush(list->first, list->last, n);
        list->count += 1;
      }
    }
  }

  // file
  {
    MD_Node *last_file = 0;
    for MD_EachNode(n, script_parse.root->first->next) {
      B32 is_end = (md_node_is_nil(n->next) && last_file != 0);
      if (str8_matchi(n->string, str8_lit("file")) || is_end) {
        if (!is_end) {
          AssertAlways( ! md_node_is_nil(n->first));
          AssertAlways(md_node_is_nil(n->first->next));
        }

        if (last_file) {
          // src_offset -> base_line
          //
          // TODO: super silly!! mdesk should export line numbers
          U64 line = 1;
          for EachIndex(idx, last_file->src_offset) { line += (source.str[idx] == '\n'); }

          String8 sub_source          = str8_substr(source, r1u64(last_file->src_offset, is_end ? source.size : n->src_offset));
          U64     file_dir_end        = str8_find_needle(sub_source, 0, str8_lit("\n"), 0);
          U64     next_file_dir_begin = str8_find_needle_reverse(sub_source, 0, str8_lit("\n"), n->src_offset);
          sub_source = str8_substr(sub_source, r1u64(file_dir_end + 1, next_file_dir_begin));

          T_DbgScriptFile *file = push_array(arena, T_DbgScriptFile, 1);
          file->path   = t_make_file_path(arena, last_file->first->string);
          file->source = sub_source;
          file->line   = line;
          SLLQueuePush(script.files.first, script.files.last, file);
          script.files.count += 1;
        }

        last_file = n;
      }
    }

    // no file directives? assume script file as main source file
    if (last_file == 0) {
      T_DbgScriptFile *file = push_array(arena, T_DbgScriptFile, 1);
      file->path = file_path;
      file->source = source;
      SLLQueuePush(script.files.first, script.files.last, file);
      script.files.count += 1;
    }
  }

  // programs
  {
    HashTable *ht = hash_table_init(scratch.arena, 256); // <U64, T_DbgScriptProgram>
    for MD_EachNode(n, script_parse.root->first->next) {
      U64 order = 0;
      if (try_u64_from_str8_c_rules(n->string, &order)) {
        T_DbgScriptFile *file = 0;
        for (file = script.files.first; file != 0; file = file->next) {
          if (file->source.str <= n->string.str && n->string.str < (file->source.str + file->source.size)) {
            break;
          }
        }
        AssertAlways(file != 0);

        // src_offset -> line
        //
        // TODO: super silly!! mdesk should export line numbers
        U64 line = 1;
        for EachIndex(i, n->src_offset) { line += (source.str[i] == '\n'); }
        
        T_DbgScriptProgram *p = hash_table_search_u64_raw(ht, order);
        if (p == 0) {
          p = push_array(arena, T_DbgScriptProgram, 1);
          p->line  = line;
          p->order = order;
          p->os    = OperatingSystem_CURRENT;
          p->file  = file;
          hash_table_push_u64_raw(scratch.arena, ht, order, p);
        } else {
          fprintf(stderr, "ERROR: duplicate order number %llu found on line %llu\n", order, p->line);
        }

        MD_Node *cmd_name = n->first;
        MD_Node *cmd_arg  = cmd_name->first;
        AssertAlways(!md_node_is_nil(cmd_name));

        // push new cmd
        T_DbgScriptCmd *cmd = push_array(arena, T_DbgScriptCmd, 1);
        cmd->kind = t_dbg_script_cmd_kind_from_string(cmd_name->string);
        AssertAlways(cmd->kind != T_DbgScriptCmdKind_Null);
        SLLQueuePush(p->first, p->first, cmd);
        p->count += 1;

        // parse cmd args
        if ( ! md_node_is_nil(cmd_arg)) {
          if (cmd->kind == T_DbgScriptCmdKind_At) {
            AssertAlways(try_s64_from_str8_c_rules(cmd_arg->string, &cmd->at.delta));
          } else if (cmd->kind == T_DbgScriptCmdKind_Eval) {
            NotImplemented;
          } else if (cmd->kind == T_DbgScriptCmdKind_Breakpoint) {
            NotImplemented;
          }
        }
      }
    }

    script.program_count = ht->count;
    script.programs      = values_from_hash_table_raw(arena, ht);
    radsort(script.programs, script.program_count, t_dbg_script_program_is_before);
  }

  scratch_end(scratch);
  return script;
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
        case T_DbgScriptCmdKind_At: {
          // map IP -> source location
          U64                 ip  = u64_from_str8(t_dbg_value_from_exprf(scratch.arena, "reg:rip"), 10);
          T_DbgSourceLocation loc = {0};
          AssertAlways(t_dbg_src_line(scratch.arena, ip, &loc, TIMEOUT_SEC(15)));

          // compute line where debugger must be
          S64 at_line_s64 = (S64)(program->line - program->file->line) + cmd->at.delta;
          U64 at_line_u64 = at_line_s64 >= 0 ? (U64)at_line_s64 : 0;
          AssertAlways(at_line_u64 > 0);

          // match expected vs current debugger locations
          B32 mismatch = loc.pt.line != at_line_u64 || 
                         !str8_match(loc.file_path, program->file->path, StringMatchFlag_CaseInsensitive|StringMatchFlag_SlashInsensitive);
          if (mismatch) {
            fprintf(stderr, "ERROR: location check did not pass:\n");
            fprintf(stderr, "  Script  : %.*s\n",      str8_varg(script->file_path));
            fprintf(stderr, "  Expected: %.*s:%llu\n", str8_varg(program->file->path), at_line_u64);
            fprintf(stderr, "  Got     : %.*s:%llu\n", str8_varg(loc.file_path), loc.pt.line);
            goto exit;
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
  t_kill_all(str8_lit("*raddbg*"));

  // read source file
  String8 source = os_data_from_file_path(arena, user_data);
  T_Ok(source.size != 0);

  // source -> script
  T_DbgScript script = t_dbg_script_from_source(arena, user_data, source);

  // write source files to test folder
  for EachNode(file, T_DbgScriptFile, script.files.first) { T_Ok(os_write_data_to_file_path(file->path, file->source)); }

  String8 compiler_path = t_cl_path();
  String8 linker_path   = t_radlink_path();

  // run compilers
  for EachNode(directive, T_DbgScriptDirective, script.directives[OperatingSystem_CURRENT][T_DbgScriptDirectiveKind_Compile].first) {
    T_Ok(t_invoke(compiler_path, directive->args, max_U64));
  }

  // run linkers
  for EachNode(directive, T_DbgScriptDirective, script.directives[OperatingSystem_CURRENT][T_DbgScriptDirectiveKind_Link].first) {
    T_Ok(t_invoke(linker_path, directive->args, max_U64));
  }

  // launch targets
  for EachNode(directive, T_DbgScriptDirective, script.directives[OperatingSystem_CURRENT][T_DbgScriptDirectiveKind_Launch].first) {
    String8 cmdl = str8f(arena, "--user:%S.raddbg_user %S", t_make_file_path(arena, str8_lit("temp")), directive->args);
    if ( ! t_dbg_launch(cmdl, ENDT_SEC(10))) {
      t_outf("failed to launch debugger with command line \"%S %S\" work dir \"%S\"\n", t_raddbg_path(), cmdl, g_wdir);
      T_Ok(0);
    }
  }

  // debugger is ready, now call script
  t_dbg_script_invoke(&script, ENDT_SEC(60*3));

  // clean up
  os_process_kill(g_dbg_handle);
}

internal void
t_dbg_register_script_tests(Arena *arena, String8 folder_path)
{
  Temp scratch = scratch_begin(&arena, 1);
  AssertAlways(os_folder_path_exists(folder_path));

  // gather file paths in a folder
  String8List paths = t_file_paths_from_dir(arena, folder_path);

  // register a test for each file
  for EachNode(n, String8Node, paths.first) {
    String8 file_path = n->string;

    // test files may contain dots for extensions we have to escape them when creating output folder for a test
    String8List file_name_parts   = str8_split(scratch.arena, str8_skip_last_slash(file_path), ".", 1, 0);
    String8     file_name_escaped = str8_list_join(arena, &file_name_parts, &(StringJoin){.sep=str8_lit("-"), .post = str8_lit("\0") });

    g_torture_tests[g_torture_test_count++] = (T_Test){
        .group     = T_Group,
        .label     = file_name_escaped.str,
        .r         = t_dbg_script_runner,
        .user_data = str8_copy(arena, file_path),
    };
  }

  scratch_end(scratch);
}

#undef T_Group
