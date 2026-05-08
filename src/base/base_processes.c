// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Handle Type Functions

internal Process
process_zero(void)
{
  Process p = {0};
  return p;
}

internal B32
process_match(Process a, Process b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

internal void
process_list_push(Arena *arena, ProcessList *list, Process p)
{
  ProcessNode *n = push_array(arena, ProcessNode, 1);
  SLLQueuePush(list->first, list->last, n);
  n->v = p;
  list->count += 1;
}

////////////////////////////////
//~ rjf: Process Launcher Helpers

internal Process
launch_cmd_line(String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  U8 split_chars[] = {' '};
  String8List parts = str8_split(scratch.arena, string, split_chars, ArrayCount(split_chars), 0);
  Process process = {0};
  if(parts.node_count != 0)
  {
    // rjf: unpack exe part
    String8 exe = parts.first->string;
    String8 exe_folder = str8_chop_last_slash(exe);
    if(exe_folder.size == 0)
    {
      exe_folder = get_current_path(scratch.arena);
    }
    
    // rjf: find stdout delimiter
    String8Node *stdout_delimiter_n = 0;
    for(String8Node *n = parts.first; n != 0; n = n->next)
    {
      if(str8_match(n->string, str8_lit(">"), 0))
      {
        stdout_delimiter_n = n;
        break;
      }
    }
    
    // rjf: read stdout path
    String8 stdout_path = {0};
    if(stdout_delimiter_n && stdout_delimiter_n->next)
    {
      stdout_path = stdout_delimiter_n->next->string;
    }
    
    // rjf: open stdout handle
    File stdout_handle = {0};
    if(stdout_path.size != 0)
    {
      File file = file_open(AccessFlag_Write|AccessFlag_Read, stdout_path);
      file_close(file);
      stdout_handle = file_open(AccessFlag_Write|AccessFlag_Append|AccessFlag_ShareRead|AccessFlag_ShareWrite|AccessFlag_Inherited, stdout_path);
    }
    
    // rjf: form command line
    String8List cmdline = {0};
    for(String8Node *n = parts.first; n != stdout_delimiter_n && n != 0; n = n->next)
    {
      str8_list_push(scratch.arena, &cmdline, n->string);
    }
    
    // rjf: launch
    ProcessLaunchParams params = {0};
    params.cmd_line = cmdline;
    params.path = exe_folder;
    params.inherit_env = 1;
    params.stdout_file = stdout_handle;
    process = process_launch(&params);
    
    // rjf: close stdout handle
    {
      if(stdout_path.size != 0)
      {
        file_close(stdout_handle);
      }
    }
  }
  scratch_end(scratch);
  return process;
}

internal Process
launch_cmd_linef(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = str8fv(scratch.arena, fmt, args);
  Process result = launch_cmd_line(string);
  va_end(args);
  scratch_end(scratch);
  return result;
}
