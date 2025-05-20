// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Relative <-> Absolute Path

internal String8
path_relative_dst_from_absolute_dst_src(Arena *arena, String8 dst, String8 src)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // rjf: gather path parts
  String8 dst_name = str8_skip_last_slash(dst);
  String8 src_folder = src;
  String8 dst_folder = str8_chop_last_slash(dst);
  String8List src_folders = str8_split_path(scratch.arena, src_folder);
  String8List dst_folders = str8_split_path(scratch.arena, dst_folder);
  
  // rjf: count # of backtracks to get from src -> dest
  U64 num_backtracks = src_folders.node_count;
  for(String8Node *src_n = src_folders.first, *bp_n = dst_folders.first;
      src_n != 0 && bp_n != 0;
      src_n = src_n->next, bp_n = bp_n->next)
  {
    if(str8_match(src_n->string, bp_n->string, path_match_flags_from_os(operating_system_from_context())))
    {
      num_backtracks -= 1;
    }
    else
    {
      break;
    }
  }
  
  // rjf: only build relative string if # of backtracks is not the entire `src`.
  // if getting to `dst` from `src` requires erasing the entire `src`, then the
  // only possible way to get to `dst` from `src` is via absolute path.
  String8 dst_path = {0};
  if(num_backtracks >= src_folders.node_count)
  {
    dst_path = dst;
  }
  else
  {
    // rjf: build backtrack parts
    String8List dst_path_strs = {0};
    for(U64 idx = 0; idx < num_backtracks; idx += 1)
    {
      str8_list_push(scratch.arena, &dst_path_strs, str8_lit(".."));
    }
    
    // rjf: build parts of dst which are unique from src
    {
      B32 unique_from_src = 0;
      for(String8Node *src_n = src_folders.first, *bp_n = dst_folders.first;
          bp_n != 0;
          bp_n = bp_n->next)
      {
        if(!unique_from_src && (src_n == 0 || !str8_match(src_n->string, bp_n->string, path_match_flags_from_os(operating_system_from_context()))))
        {
          unique_from_src = 1;
        }
        if(unique_from_src)
        {
          str8_list_push(scratch.arena, &dst_path_strs, bp_n->string);
        }
        if(src_n != 0)
        {
          src_n = src_n->next;
        }
      }
    }
    
    // rjf: build file name
    str8_list_push(scratch.arena, &dst_path_strs, dst_name);
    
    // rjf: join
    StringJoin join = {0};
    {
      join.sep = str8_lit("/");
    }
    dst_path = str8_list_join(arena, &dst_path_strs, &join);
  }
  scratch_end(scratch);
  return dst_path;
}

internal String8
path_absolute_dst_from_relative_dst_src(Arena *arena, String8 dst, String8 src)
{
  String8 result = dst;
  PathStyle dst_style = path_style_from_str8(dst);
  if(dst.size != 0 && dst_style == PathStyle_Relative)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 dst_from_src_absolute = push_str8f(scratch.arena, "%S/%S", src, dst);
    String8List dst_from_src_absolute_parts = str8_split_path(scratch.arena, dst_from_src_absolute);
    PathStyle dst_from_src_absolute_style = path_style_from_str8(src);
    str8_path_list_resolve_dots_in_place(&dst_from_src_absolute_parts, dst_from_src_absolute_style);
    result = str8_path_list_join_by_style(arena, &dst_from_src_absolute_parts, dst_from_src_absolute_style);
    scratch_end(scratch);
  }
  return result;
}

////////////////////////////////
//~ rjf: Path Normalization

internal String8List
path_normalized_list_from_string(Arena *arena, String8 path_string, PathStyle *style_out)
{
  // rjf: analyze path
  PathStyle path_style = path_style_from_str8(path_string);
  String8List path = str8_split_path(arena, path_string);
  
  // rjf: resolve dots
  str8_path_list_resolve_dots_in_place(&path, path_style);
  
  // rjf: return
  if(style_out != 0)
  {
    *style_out = path_style;
  }
  return path;
}

internal String8
path_normalized_from_string(Arena *arena, String8 path_string)
{
  Temp scratch = scratch_begin(&arena, 1);
  PathStyle style = PathStyle_Relative;
  String8List path = path_normalized_list_from_string(scratch.arena, path_string, &style);
  String8 result = str8_path_list_join_by_style(arena, &path, style);
  scratch_end(scratch);
  return result;
}

internal B32
path_match_normalized(String8 left, String8 right)
{
  Temp scratch = scratch_begin(0, 0);
  String8 left_normalized = path_normalized_from_string(scratch.arena, left);
  String8 right_normalized = path_normalized_from_string(scratch.arena, right);
  B32 result = str8_match(left_normalized, right_normalized, StringMatchFlag_CaseInsensitive);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Basic Helpers

internal PathStyle
path_style_from_string(String8 string)
{
  for (U64 i = 0; i < ArrayCount(g_path_style_map); ++i)
  {
    if(str8_match(g_path_style_map[i].string, string, StringMatchFlag_CaseInsensitive))
    {
      return g_path_style_map[i].path_style;
    }
  }
  return PathStyle_Null;
}

internal String8
string_from_path_style(PathStyle style)
{
  Assert(style < ArrayCount(g_path_style_map));
  return g_path_style_map[style].string;
}

internal String8
path_separator_string_from_style(PathStyle style)
{
  String8 result = str8_zero();
  switch (style)
  {
    case PathStyle_Null:     break;
    case PathStyle_Relative: break;
    case PathStyle_WindowsAbsolute: result = str8_lit("\\"); break;
    case PathStyle_UnixAbsolute:    result = str8_lit("/");  break;
  }
  return result;
}

internal StringMatchFlags
path_match_flags_from_os(OperatingSystem os)
{
  StringMatchFlags flags = StringMatchFlag_SlashInsensitive;
  switch(os)
  {
    default:{}break;
    case OperatingSystem_Windows:
    {
      flags |= StringMatchFlag_CaseInsensitive;
    }break;
    case OperatingSystem_Linux:
    case OperatingSystem_Mac:
    {
      // NOTE(rjf): no-op
    }break;
  }
  return flags;
}

internal String8
path_convert_slashes(Arena *arena, String8 path, PathStyle path_style)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = str8_split_path(scratch.arena, path);
  StringJoin join = {0};
  join.sep = path_separator_string_from_style(path_style);
  String8 result = str8_list_join(arena, &list, &join);
  scratch_end(scratch);
  return result;
}

internal String8
path_replace_file_extension(Arena *arena, String8 file_name, String8 ext)
{
  String8 file_name_no_ext = str8_chop_last_dot(file_name);
  String8 result           = push_str8f(arena, "%S.%S", file_name_no_ext, ext);
  return result;
}
