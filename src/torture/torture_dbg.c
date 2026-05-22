// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define T_Group "Dbg"

#define T_Dbg_DefaultTimeout TIMEOUT_SEC(5)

extern B32 g_stop_on_first_fail_or_crash;

////////////////////////////////
// Debugger IPC Replies

typedef struct RD_IpcReply RD_IpcReply;
struct RD_IpcReply
{
  MD_ParseResult parse;
  MD_Node *root;
  MD_Node *msg;
};

internal RD_IpcReply
rd_ipc_mdesk_reply_from_string(Arena *arena, String8 string)
{
  MD_ParseResult parse = md_parse_from_text(arena, str8_lit("ipc_reply"), string);
  return (RD_IpcReply){ .parse = parse, .root = parse.root, parse.root->first };
}

internal B32
rd_ipc_parse_string(MD_Node *node, String8 child_name, String8 *out)
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
rd_ipc_parse_u32(MD_Node *node, String8 child_name, U32 *out)
{
  String8 value = {0};
  if (rd_ipc_parse_string(node, child_name, &value))
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

#define rd_ipc_parse_int(n, c, ptr) rd_ipc_parse_int_(n, c, sizeof(*ptr), ptr)
internal B32
rd_ipc_parse_int_(MD_Node *node, String8 child_name, U64 out_size, void *out)
{
  String8 value = {0};
  if (rd_ipc_parse_string(node, child_name, &value))
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
rd_ipc_parse_b32(MD_Node *node, String8 child_name, B32 *out)
{
  B32     is_ok = 0;
  String8 s     = {0};
  U64     value = 0;
  if (rd_ipc_parse_string(node, child_name, &s))
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

////////////////////////////////
// IPC Controller

internal U32 g_dbg_pid;

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
  if ( ! t_dbg_send_cmd(str8_lit("state"), timeout_us, arena ? arena : scratch.arena, &reply)) { goto exit; }

  // parse reply
  MD_Node *state_md      = md_child_from_string(reply.root, str8_lit("state"), 0);
  MD_Node *stop_event_md = md_child_from_string(state_md, str8_lit("stop_event"), 0);
  MD_Node *threads_md    = md_child_from_string(state_md, str8_lit("threads"),    0);
  MD_Node *modules_md    = md_child_from_string(state_md, str8_lit("modules"),    0);
  T_DbgState v = {0};
  if (!rd_ipc_parse_int(state_md, str8_lit("running"), &v.running)) { Assert(0); goto exit; }
  if (!rd_ipc_parse_int(state_md, str8_lit("run_gen"), &v.run_gen)) { Assert(0); goto exit; }
  if (!rd_ipc_parse_int(state_md, str8_lit("ip"), &v.ip))           { Assert(0); goto exit; }
  
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
    if ( ! rd_ipc_parse_string(lines_md, str8_lit("file_path"), &line.file_path))        { Assert(0); goto exit; }
    if ( ! rd_ipc_parse_int(lines_md, str8_lit("line_num"), &line.line_num))             { Assert(0); goto exit; }
    if ( ! rd_ipc_parse_int(lines_md, str8_lit("column_num"), &line.column_num))         { Assert(0); goto exit; }
    if ( ! rd_ipc_parse_int(lines_md, str8_lit("voff_range_min"), &line.voff_range.min)) { Assert(0); goto exit; }
    if ( ! rd_ipc_parse_int(lines_md, str8_lit("voff_range_max"), &line.voff_range.max)) { Assert(0); goto exit; }

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
  
  // snapshot status
  T_DbgState *status_before = t_dbg_state(scratch.arena, max_U64);
  if (status_before == 0) { Assert(0 && "failed to snapshot status"); goto exit; }
  
  // send command
  if (t_dbg_send_cmd(cmd, max_U64, 0, 0) == 0) { Assert(0 && "failed to the command"); goto exit; }
  
  // wait for debugger to stop
  U64 t = ENDT_US(timeout_us);
  for (;;) {
    // query debugger status
    T_DbgState *status = t_dbg_state(scratch.arena, t);
    if (status == 0) { Assert(0 && "failed to acquire debugger status"); goto exit; }
    
    // did state change? -> break
    if (!status->running && status->run_gen != status_before->run_gen) {
      is_stopped = 1;
      break;
    }
    
    // "solve" the wait problem
    if (now_time_us() >= t) { Assert(0 && "timeout"); goto exit; }
    sleep_ms(10);
  }
  
  //--- Status ---------------------
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
    AssertAlways(last_stop);
    
    T_DbgLineArray lines = {0};
    AssertAlways(t_dbg_src_line(scratch.arena, last_stop->ip_vaddr, &lines, T_Dbg_DefaultTimeout));
    
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
      t_infof("    Line:       %lld\n", (long long)lines.v[i].line_num);
      t_infof("    Column:     %lld\n", (long long)lines.v[i].column_num);
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
  B32 did_reply = state != 0;
  scratch_end(scratch);
  return did_reply;
}
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
  cmdline = str8f(scratch.arena, "%S --gen_crash_dump --user:\"%S\" --project:\"%S\" --logs:\"%S\" %S", t_raddbg_path(), user_path, project_path, g_wdir, cmdline);
  
  // launch debugger
  ProcessLaunchParams launch_opts = {
    .path        = g_wdir,
    .inherit_env = 1,
    .consoleless = 1,
    .cmd_line    = lnk_arg_list_parse_windows_rules(scratch.arena, cmdline),
  };
  Process dbg_handle = process_launch(&launch_opts);
  if (process_match(dbg_handle, process_zero())) { AssertAlways(0 && "failed to launch debugger"); goto exit; }
  
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
    if (now_time_us() >= t) { Assert(0 && "timeout"); break; }
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
  if ( ! rd_ipc_parse_string(reply.root, str8_lit("expr"),  &e.expr))  { t_errorf("ERROR: failed to parse reply member: expr\n");  Assert(0); goto exit; }
  if ( ! rd_ipc_parse_string(reply.root, str8_lit("value"), &e.value)) { t_errorf("ERROR: failed to parse reply member: value\n"); Assert(0); goto exit; }
  if ( ! rd_ipc_parse_string(reply.root, str8_lit("type"),  &e.type))  { t_errorf("ERROR: failed to parse reply member: type\n");  Assert(0); goto exit; }
  if ( ! rd_ipc_parse_string(reply.root, str8_lit("error"), &e.error)) { t_errorf("ERROR: failed to parse reply member: error\n"); Assert(0); goto exit; }
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
    case T_DbgScriptCmdKind_Null:             return str8_zero();
    case T_DbgScriptCmdKind_Breakpoint:       return str8_lit("bp");
    case T_DbgScriptCmdKind_ClearBreakpoints: return str8_lit("bp_clear");
    case T_DbgScriptCmdKind_Run:              return str8_lit("run");
    case T_DbgScriptCmdKind_Halt:             return str8_lit("halt");
    case T_DbgScriptCmdKind_StepOver:         return str8_lit("step_over");
    case T_DbgScriptCmdKind_StepInto:         return str8_lit("step_into");
    case T_DbgScriptCmdKind_StepOut:          return str8_lit("step_out");
    case T_DbgScriptCmdKind_StepOverInst:     return str8_lit("step_over_inst");
    case T_DbgScriptCmdKind_StepIntoInst:     return str8_lit("step_into_inst");
    case T_DbgScriptCmdKind_StepOverLine:     return str8_lit("step_over_line");
    case T_DbgScriptCmdKind_StepIntoLine:     return str8_lit("step_into_line");
    case T_DbgScriptCmdKind_KillAll:          return str8_lit("kill_all");
    case T_DbgScriptCmdKind_At:               return str8_lit("at");
    case T_DbgScriptCmdKind_Eval:             return str8_lit("eval");
    default: InvalidPath;
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
t_dbg_script_from_source(Arena *arena, String8 file_path, String8 source, T_DbgScript *script_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  B32 is_ok = 0;
  
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
    if (str8_matchi(test->string, str8_lit("test"))) {
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
    } else {
      t_errorf("ERROR: %S: missing test header\n", file_path);
      goto exit;
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
          t_errorf("ERROR: duplicate order number %llu found on line %llu\n", (unsigned long long)order, (unsigned long long)p->line);
        }
        
        for MD_EachNode(cmd_n, n->first) {
          // push new cmd
          T_DbgScriptCmd *cmd = push_array(arena, T_DbgScriptCmd, 1);
          cmd->kind = t_dbg_script_cmd_kind_from_string(cmd_n->string);
          cmd->line = line;
          Assert(cmd->kind != T_DbgScriptCmdKind_Null);
          SLLQueuePush(p->first, p->last, cmd);
          p->count += 1;
          
          // parse cmd args
          MD_Node *cmd_arg = cmd_n->first;
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
    }
    
    script.program_count = ht->count;
    script.programs      = values_from_hash_table_raw(arena, ht);
    radsort(script.programs, script.program_count, t_dbg_script_program_is_before);
  }
  
  is_ok = 1;
  exit:;
  if (script_out) {
    *script_out = script;
  }
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
        t_infof("[%llu] Command: %S:%llu %S\n", program->order, script->file_path, (unsigned long long)cmd->line, t_string_from_dbg_script_cmd_kind(cmd->kind));
        
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
            // TODO: debugger does not populate eval cache with registers before first frame,
            // so this racy, for now use lower level option
#if 0
            U64 ip = u64_from_str8(t_dbg_value_from_exprf(scratch.arena, "reg:rip"), 10);
            if (ip == 0) {
              fprintf(stderr, "ERROR: invalid IP address: 0x%llx\n", (unsigned long long)ip);
              goto exit;
            }
#else
            T_DbgState *temp_status = t_dbg_state(scratch.arena, T_Dbg_DefaultTimeout);
            if (temp_status == 0) {
              t_errorf("ERROR: %S:%llu: failed to query IP\n", script->file_path, (unsigned long long)cmd->line);
              goto exit;
            }
            U64 ip = temp_status->ip;
#endif
            
            // map IP -> source location
            T_DbgLineArray lines = {0};
            if (t_dbg_src_line(scratch.arena, ip, &lines, T_Dbg_DefaultTimeout) == 0) {
              t_errorf("ERROR: %S:%llu: IP (0x%llx) does not map to a source line\n", script->file_path, (unsigned long long)cmd->line, (unsigned long long)ip);
              goto exit;
            }
            
            // compute line where debugger must be
            S64 at_line_s64 = (S64)(program->line - program->file->line) + cmd->at.delta;
            U64 at_line_u64 = at_line_s64 >= 0 ? (U64)at_line_s64 : 0;
            AssertAlways(at_line_u64 > 0);
            
            // match expected vs current debugger locations

            for EachIndex(i, lines.count) {
              B32 mismatch = lines.v[i].line_num != at_line_u64 || 
                             !str8_match(lines.v[i].file_path, program->file->path, StringMatchFlag_CaseInsensitive|StringMatchFlag_SlashInsensitive);
              if (mismatch) {
                t_errorf("ERROR: %S:%llu: location check did not pass:\n", script->file_path, (unsigned long long)cmd->line);
                t_errorf("  Expected: %S:%llu\n", program->file->path, (unsigned long long)at_line_u64);
                t_errorf("  Got     : %S:%llu\n", lines.v[i].file_path, (unsigned long long)lines.v[i].line_num);
                t_errorf("  IP      : 0x%llx\n",  (unsigned long long)ip);
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
  if ( ! file_path_exists(t_raddbg_path())) {
    t_errorf("ERROR: failed to find debugger \"%S\"\n", t_raddbg_path());
    T_Ok(0);
  }
  
  // read source file
  String8 source = data_from_file_path(arena, user_data);
  if (source.size == 0) {
    t_errorf("ERROR: failed to read script: \"%S\"\n", user_data);
    T_Ok(0);
  }
  
  // source -> script
  T_DbgScript script = {0};
  if ( ! t_dbg_script_from_source(arena, user_data, source, &script)) {
    goto exit;
  }
  
  // write source files to test folder
  for EachNode(file, T_DbgScriptFile, script.files.first) {
    if (write_data_to_file_path(file->path, file->source) == 0) {
      t_errorf("ERROR: %S:%llu: failed to write: \"%S\"\n", user_data, (unsigned long long)file->line, file->path);
      T_Ok(0);
    }
  }
  
  // compiler vars
  HashTable *script_vars = hash_table_init(arena, 1000);
  hash_table_push_path_string(arena, script_vars, str8_lit("CWD"), g_wdir);
  
  // run compilers
  for EachNode(directive, T_DbgScriptDirective, script.directives[OperatingSystem_CURRENT][T_DbgScriptDirectiveKind_Compile].first) {
    T_Compiler compiler = directive->compile.compiler;
    
    // pick default compiler if none selected
    if (compiler == T_Compiler_Null) {
      switch (OperatingSystem_CURRENT) {
        case OperatingSystem_Windows: { compiler = T_Compiler_Cl;    } break;
        case OperatingSystem_Linux:   { compiler = T_Compiler_Clang; } break;
      }
    }
    
    // get compiler path
    String8 compiler_path = {0};
    switch (compiler) {
      case T_Compiler_Null: break;
      case T_Compiler_Cl:    compiler_path = t_cl_path();    break;
      case T_Compiler_Clang: compiler_path = t_clang_path(); break;
      case T_Compiler_Gcc:   compiler_path = t_gcc_path();   break;
    }
    
    // invoke compiler with arguments from directive
    String8 expanded_args = lnk_expand_env_vars_windows(arena, script_vars, directive->args);
    if (t_invoke(compiler_path, expanded_args, max_U64) == 0) {
      t_errorf("ERROR: failed to launch compiler: \"%S %S\"\n", compiler_path, expanded_args);
      T_Ok(0);
    }
    if (g_last_exit_code) {
      t_errorf("ERROR: %S:%llu: %S\n", script.file_path, (unsigned long long)directive->line, g_errors);
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
      t_errorf("ERROR: %S:%llu: %S\n", script.file_path, (unsigned long long)directive->line, g_errors);
      T_Ok(0);
    }
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
      .group     = T_Group,
      .label     = (char*)file_name_escaped.str,
      .r         = t_dbg_script_runner,
      .user_data = str8_copy(arena, file_path),
    };
  }
  
  scratch_end(scratch);
}

#undef T_Group
