// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/linux_shell.meta.c"

////////////////////////////////
//~ rjf: @per_os_impl Shell Operations

internal void
sh_message(B32 error, String8 title, String8 message)
{
  B32 done = 0;
  B32 text_is_long = (str8_find_needle(message, 0, s("\n"), 0) < message.size);
  
  //- rjf: try zenity
  if(!done)
  {
    Temp scratch = scratch_begin(0, 0);
    if(text_is_long)
    {
      String8 cmd = str8f(scratch.arena, "echo \"%S\" | zenity --text-info --no-markup --title=\"%S\"", escaped_from_raw_str8(scratch.arena, message), title);
      FILE *f = popen((char *)cmd.str, "r");
      done = (f != 0);
      pclose(f);
    }
    else
    {
      String8 cmd = str8f(scratch.arena, "zenity %s --no-markup --title=\"%S\" --text=\"%S\"", error ? "--error" : "--info", title, escaped_from_raw_str8(scratch.arena, message));
      FILE *f = popen((char *)cmd.str, "r");
      done = (f != 0);
      pclose(f);
    }
    scratch_end(scratch);
  }
  
  //- rjf: fall back to stderr
  if(!done)
  {
    if(error)
    {
      fprintf(stderr, "[X] ");
    }
    fprintf(stderr, "%.*s\n", str8_varg(title));
    fprintf(stderr, "%.*s\n\n", str8_varg(message));
  }
}

internal String8
sh_pick_file(Arena *arena, String8 title, String8 initial_path)
{
  String8 result = {0};
  B32 done = 0;
  
  //- rjf: try zenity
  if(!done)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 cmd = str8f(scratch.arena, "zenity --file-selection --filename=\"%S\" --title=\"%S\"", initial_path, title);
    FILE *f = popen((char *)cmd.str, "r");
    done = (f != 0);
    if(done)
    {
      U64 output_buffer_size = 4096;
      char *output_buffer = push_array(scratch.arena, char, output_buffer_size);
      char *path = fgets(output_buffer, output_buffer_size, f);
      if(path != 0)
      {
        result = str8_copy(arena, str8_chop(str8_cstring_capped(path, output_buffer+output_buffer_size), 1));
      }
    }
    pclose(f);
    scratch_end(scratch);
  }
  
  return result;
}

internal void
sh_show_in_file_browser(String8 path)
{
  B32 done = 0;
  
  //- rjf: try xdg-open
  if(!done)
  {
    Temp scratch = scratch_begin(0, 0);
    String8 cmd = str8f(scratch.arena, "xdg-open %S", str8_chop_last_slash(path));
    FILE *f = popen((char *)cmd.str, "r");
    pclose(f);
    scratch_end(scratch);
  }
  
  //- rjf: fallback -> tell user
  if(!done)
  {
    Temp scratch = scratch_begin(0, 0);
    sh_message(1, s("Error"), s("Could not find a way to pick a file using the locally installed system utilities."));
    scratch_end(scratch);
  }
}

internal void
sh_open_in_browser(String8 url)
{
  B32 done = 0;
  
  //- rjf: try xdg-open
  if(!done)
  {
    Temp scratch = scratch_begin(0, 0);
    String8 cmd = str8f(scratch.arena, "xdg-open %S", url);
    FILE *f = popen((char *)cmd.str, "r");
    done = (f != 0);
    pclose(f);
    scratch_end(scratch);
  }
  
  //- rjf: fallback -> tell user
  if(!done)
  {
    Temp scratch = scratch_begin(0, 0);
    sh_message(1, s("Error"), str8f(scratch.arena, "Could not find a way to open %S in a locally installed browser.", url));
    scratch_end(scratch);
  }
}

internal void
sh_install_or_uninstall_self(B32 install)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: get xdg local application install info path
  String8 application_name = str8_chop_last_dot(str8_skip_last_slash(get_process_info()->binary_file_path));
  char *xdg_data_home_cstring = getenv("XDG_DATA_HOME");
  String8 desktop_entries_folder_name = str8_cstring(xdg_data_home_cstring);
  if(desktop_entries_folder_name.size == 0)
  {
    char *home = getenv("HOME");
    desktop_entries_folder_name = str8f(scratch.arena, "%s/.local/share", home);
  }
  String8 desktop_file_path = str8f(scratch.arena, "%S/applications/%S.desktop", desktop_entries_folder_name, application_name);
  
  //- rjf: get icon image path
  B32 has_icon = 0;
  String8 icon_path = {0};
#if LNX_WM_ICON
  {
    has_icon = 1;
    icon_path = str8f(scratch.arena, "%S/%S.png", get_process_info()->user_program_config_data_path, application_name);
  }
#endif
  
  //- rjf: install -> write files
  if(install)
  {
    //- rjf: write icon
    write_data_to_file_path(icon_path, lnx_sh_logo_file_bytes);
    
    //- rjf: write .desktop file
    String8 desktop_file_data = str8f(scratch.arena,
                                      "[Desktop Entry]\n"
                                      "Type=Application\n"
                                      "Version=" BUILD_VERSION_STRING_LITERAL "\n"
                                      "Name=" BUILD_TITLE "\n"
                                      "Comment=" BUILD_TITLE_STRING_LITERAL "\n"
                                      "Exec=%S\n"
                                      "%s%S%s" // (optional icon)
                                      "Terminal=%s\n"
                                      "Categories=Utility;Development;",
                                      get_process_info()->binary_file_path,
                                      has_icon ? "Icon=" : "",
                                      has_icon ? icon_path : s(""),
                                      has_icon ? "\n" : "",
                                      BUILD_CONSOLE_INTERFACE ? "true" : "false");
    write_data_to_file_path(desktop_file_path, desktop_file_data);
  }
  
  //- rjf: uninstall -> delete files
  else
  {
    delete_file_at_path(icon_path);
    delete_file_at_path(desktop_file_path);
  }
  
  scratch_end(scratch);
}
