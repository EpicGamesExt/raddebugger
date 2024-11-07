// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct
{
  String8Array path_arr;
  OS_Handle   *handle_arr;
  U64         *size_arr;
} OS_DataSizeFromFilePathTask;

typedef struct
{
  String8Array data_arr;
  OS_Handle   *handle_arr;
  U64         *size_arr;
  U64         *off_arr;
  U8          *buffer;
} OS_DataFromFilePathTask;

internal String8Array os_data_from_file_path_parallel(TP_Context *tp, Arena *arena, String8Array path_arr);
internal String8List  os_file_search(Arena *arena, String8List dir_list, String8 file_path);

