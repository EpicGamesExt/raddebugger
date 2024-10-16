// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once 

internal String8 make_file_name_with_ext(Arena *arena, String8 file_name, String8 ext);
internal String8 make_file_path_with_ext(Arena *arena, String8 file_name, String8 ext);
internal String8 path_convert_slashes(Arena *arena, String8 path, PathStyle path_style);
internal String8 path_canon_from_regular_path(Arena *arena, String8 path);
internal PathStyle path_style_from_string(String8 string);

