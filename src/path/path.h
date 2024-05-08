// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef PATH_H
#define PATH_H

typedef U32 PathKind;
enum
{
  Path_Null,
  Path_Windows,
  Path_Unix,
  PathKind_Count
};

////////////////////////////////
//~ allen: Path Helper Functions

internal StringMatchFlags path_match_flags_from_os(OperatingSystem os);
internal String8 path_relative_dst_from_absolute_dst_src(Arena *arena, String8 dst, String8 src);
internal String8 path_absolute_dst_from_relative_dst_src(Arena *arena, String8 dst, String8 src);
internal String8List path_normalized_list_from_string(Arena *arena, String8 path, PathStyle *style_out);
internal String8 path_normalized_from_string(Arena *arena, String8 path);
internal String8 path_convert_slashes(Arena *arena, String8 path, PathKind path_kind);

#endif //PATH_H
