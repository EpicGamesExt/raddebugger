// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_DUMP_H
#define RDI_DUMP_H

////////////////////////////////
//~ Program Parameters Type

typedef struct RADDBGIDUMP_Params{
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
} RADDBGIDUMP_Params;

////////////////////////////////
//~ Program Parameters Parser

static RADDBGIDUMP_Params *raddbgidump_params_from_cmd_line(Arena *arena, CmdLine *cmdline);

#endif //RDI_DUMP_H
