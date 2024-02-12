// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_DUMP_H
#define RADDBGI_DUMP_H

////////////////////////////////
//~ Program Parameters Type

typedef struct DUMP_Params{
  String8 input_name;
  String8 input_data;
  
  struct{
    B8 input;
  } hide_errors;
  
  B8 dump__first;
  B8 dump_data_sections;
  B8 dump_top_level_info;
  B8 dump_binary_sections;
  B8 dump_file_paths;
  B8 dump_source_files;
  B8 dump_units;
  B8 dump_unit_vmap;
  B8 dump_type_nodes;
  B8 dump_udt_data;
  B8 dump_global_variables;
  B8 dump_global_vmap;
  B8 dump_thread_variables;
  B8 dump_procedures;
  B8 dump_scopes;
  B8 dump_scope_vmap;
  B8 dump_name_map;
  B8 dump__last;
  
  String8List errors;
} DUMP_Params;

////////////////////////////////
//~ Program Parameters Parser

static DUMP_Params *dump_params_from_cmd_line(Arena *arena, CmdLine *cmdline);

#endif //RADDBGI_DUMP_H
