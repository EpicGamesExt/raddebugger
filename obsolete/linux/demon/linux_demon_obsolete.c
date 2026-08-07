// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNX_DMN_Process *
lnx_dmn_event_attach(Arena *arena, DMN_EventList *events, pid_t pid)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // create process
  LNX_DMN_Process *process = lnx_dmn_event_create_process(arena, events, pid, 0, LNX_DMN_CreateProcessFlag_DebugSubprocesses|LNX_DMN_CreateProcessFlag_Rebased);
  
  // extract threads from /proc/pid/task
  {
    String8 task_path = str8f(scratch.arena, "/proc/%d/task", pid);
    DIR *task_dirp = opendir((char *)task_path.str);
    if(task_dirp)
    {
      for(;;)
      {
        struct dirent *dirent = readdir(task_dirp);
        if(dirent == 0) { break; }
        
        String8 tid_str = str8_cstring_capped(dirent->d_name, dirent->d_name + NAME_MAX);
        if(str8_match(tid_str, str8_lit(".."), 0) || str8_match(tid_str, str8_lit("."), 0)) { continue; }
        U64     tid_64  = u64_from_str8(tid_str, 10);
        pid_t   tid     = (pid_t)tid_64;
        AssertAlways(tid == tid_64);
        
        if(tid == pid) { continue; } // main thread was created during create process sequence
        lnx_dmn_event_create_thread(arena, events, process, tid);
      }
      
      LNX_RETRY_ON_EINTR(closedir(task_dirp));
    }
  }
  
  // extract modules from r_debug
  {
    B32                is_64bit      = process->ctx->dl_class == ELF_Class_64;
    GNU_RDebugInfoList rdebug_list   = gnu_parse_rdebug(scratch.arena, is_64bit, process->ctx->rdebug_vaddr, lnx_dmn_machine_op_mem_read, &process->fd);
    U64                name_space_id = 0;
    for EachNode(rdebug_n, GNU_RDebugInfoNode, rdebug_list.first)
    {
      lnx_dmn_event_load_module(arena, events, process->first_thread, name_space_id, rdebug_n->v.r_map);
      name_space_id += 1;
    }
  }
  
  // handshake complete
  lnx_dmn_push_event_handshake_complete(arena, events, process);
  
  scratch_end(scratch);
  return process;
}