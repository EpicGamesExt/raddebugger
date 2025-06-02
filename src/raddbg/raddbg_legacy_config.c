// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RD_CfgList
rd_cfg_tree_list_from_string__pre_0_9_16(Arena *arena, String8 file_path, String8 data)
{
  RD_CfgList result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 folder_path = str8_skip_last_slash(file_path);
    MD_Node *src_root = md_parse_from_text(scratch.arena, file_path, data).root;
    {
      for MD_EachNode(tln, src_root->first)
      {
        //- rjf: targets
        if(str8_match(tln->string, str8_lit("target"), 0))
        {
          String8 disabled_string   = md_child_from_string(tln, str8_lit("disabled"), 0)->first->string;
          String8 executable        = md_child_from_string(tln, str8_lit("executable"), 0)->first->string;
          String8 arguments         = md_child_from_string(tln, str8_lit("arguments"), 0)->first->string;
          String8 working_directory = md_child_from_string(tln, str8_lit("working_directory"), 0)->first->string;
          String8 entry_point       = md_child_from_string(tln, str8_lit("entry_point"), 0)->first->string;
          String8 stdout_path       = md_child_from_string(tln, str8_lit("stdout_path"), 0)->first->string;
          String8 stderr_path       = md_child_from_string(tln, str8_lit("stderr_path"), 0)->first->string;
          String8 stdin_path        = md_child_from_string(tln, str8_lit("stdin_path"), 0)->first->string;
          String8 debug_subprocesses= md_child_from_string(tln, str8_lit("debug_subprocesses"), 0)->first->string;
          RD_Cfg *dst_root = rd_cfg_new(&rd_nil_cfg, str8_lit("target"));
          rd_cfg_list_push(arena, &result, dst_root);
          {
            if(executable.size != 0) { rd_cfg_new(rd_cfg_new(dst_root, str8_lit("executable")), path_absolute_dst_from_relative_dst_src(scratch.arena, executable, folder_path)); }
            if(arguments.size != 0) { rd_cfg_new(rd_cfg_new(dst_root, str8_lit("arguments")), raw_from_escaped_str8(scratch.arena, arguments)); }
            if(working_directory.size != 0) { rd_cfg_new(rd_cfg_new(dst_root, str8_lit("working_directory")), path_absolute_dst_from_relative_dst_src(scratch.arena, working_directory, folder_path)); }
            if(entry_point.size != 0) { rd_cfg_new(rd_cfg_new(dst_root, str8_lit("entry_point")), raw_from_escaped_str8(scratch.arena, entry_point)); }
            if(stdout_path.size != 0) { rd_cfg_new(rd_cfg_new(dst_root, str8_lit("stdout_path")), path_absolute_dst_from_relative_dst_src(scratch.arena, stdout_path, folder_path)); }
            if(stderr_path.size != 0) { rd_cfg_new(rd_cfg_new(dst_root, str8_lit("stderr_path")), path_absolute_dst_from_relative_dst_src(scratch.arena, stderr_path, folder_path)); }
            if(stdin_path.size != 0) { rd_cfg_new(rd_cfg_new(dst_root, str8_lit("stdin_path")), path_absolute_dst_from_relative_dst_src(scratch.arena, stdin_path, folder_path)); }
            if(debug_subprocesses.size != 0) { rd_cfg_new(rd_cfg_new(dst_root, str8_lit("debug_subprocesses")), raw_from_escaped_str8(scratch.arena, debug_subprocesses)); }
            if(!str8_match(disabled_string, str8_lit("1"), 0))
            {
              rd_cfg_new(rd_cfg_new(dst_root, str8_lit("enabled")), str8_lit("1"));
            }
          }
        }
        
        //- rjf: recent files / projects
        if(str8_match(tln->string, str8_lit("recent_file"), 0) ||
           str8_match(tln->string, str8_lit("recent_project"), 0))
        {
          RD_Cfg *dst_root = rd_cfg_new(&rd_nil_cfg, tln->string);
          rd_cfg_list_push(arena, &result, dst_root);
          rd_cfg_new(rd_cfg_new(dst_root, str8_lit("path")), path_absolute_dst_from_relative_dst_src(scratch.arena, tln->first->string, folder_path));
        }
        
        //- rjf: file path maps
        if(str8_match(tln->string, str8_lit("file_path_map"), 0))
        {
          String8 source = md_child_from_string(tln, str8_lit("source"), 0)->first->string;
          String8 dest = md_child_from_string(tln, str8_lit("dest"), 0)->first->string;
          RD_Cfg *dst_root = rd_cfg_new(&rd_nil_cfg, tln->string);
          rd_cfg_list_push(arena, &result, dst_root);
          rd_cfg_new(rd_cfg_new(dst_root, str8_lit("source")), path_absolute_dst_from_relative_dst_src(scratch.arena, source, folder_path));
          rd_cfg_new(rd_cfg_new(dst_root, str8_lit("dest")), path_absolute_dst_from_relative_dst_src(scratch.arena, dest, folder_path));
        }
      }
    }
    scratch_end(scratch);
  }
  return result;
}
