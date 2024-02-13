// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "lib_raddbgi_format/raddbgi_format.h"
#include "lib_raddbgi_format/raddbgi_format_parse.h"
#include "lib_raddbgi_format/raddbgi_format.c"
#include "lib_raddbgi_format/raddbgi_format_parse.c"

#include "base/base_inc.h"
#include "os/os_inc.h"

#include "raddbgi_stringize.h"
#include "raddbgi_dump.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

#include "raddbgi_stringize.c"

////////////////////////////////
//~ Program Parameters Parser

static RADDBGIDUMP_Params*
raddbgidump_params_from_cmd_line(Arena *arena, CmdLine *cmdline){
  RADDBGIDUMP_Params *result = push_array(arena, RADDBGIDUMP_Params, 1);
  
  // get input raddbg
  {
    String8 input_name = cmd_line_string(cmdline, str8_lit("raddbg"));
    if (input_name.size == 0){
      str8_list_push(arena, &result->errors,
                     str8_lit("missing required parameter '--raddbg:<raddbg_file>'"));
    }
    
    if (input_name.size > 0){
      String8 input_data = os_data_from_file_path(arena, input_name);
      
      if (input_data.size == 0){
        str8_list_pushf(arena, &result->errors,
                        "could not load input file '%.*s'", str8_varg(input_name));
      }
      
      if (input_data.size != 0){
        result->input_name = input_name;
        result->input_data = input_data;
      }
    }
  }
  
  // error options
  if (cmd_line_has_flag(cmdline, str8_lit("hide_errors"))){
    String8List vals = cmd_line_strings(cmdline, str8_lit("hide_errors"));
    
    // if no values - set all to hidden
    if (vals.node_count == 0){
      B8 *ptr  = (B8*)&result->hide_errors;
      B8 *opl = ptr + sizeof(result->hide_errors);
      for (;ptr < opl; ptr += 1){
        *ptr = 1;
      }
    }
    
    // for each explicit value set the corresponding flag to hidden
    for (String8Node *node = vals.first;
         node != 0;
         node = node->next){
      if (str8_match(node->string, str8_lit("input"), 0)){
        result->hide_errors.input = 1;
      }
    }
  }
  
  // dump options
  {
    String8List vals = cmd_line_strings(cmdline, str8_lit("dump"));
    if (vals.first == 0){
      B8 *ptr = &result->dump__first;
      for (; ptr < &result->dump__last; ptr += 1){
        *ptr = 1;
      }
    }
    else{
      for (String8Node *node = vals.first;
           node != 0;
           node = node->next){
        if (str8_match(node->string, str8_lit("data_sections"), 0)){
          result->dump_data_sections = 1;
        }
        else if (str8_match(node->string, str8_lit("top_level_info"), 0)){
          result->dump_top_level_info = 1;
        }
        else if (str8_match(node->string, str8_lit("binary_sections"), 0)){
          result->dump_binary_sections = 1;
        }
        else if (str8_match(node->string, str8_lit("file_paths"), 0)){
          result->dump_file_paths = 1;
        }
        else if (str8_match(node->string, str8_lit("source_files"), 0)){
          result->dump_source_files = 1;
        }
        else if (str8_match(node->string, str8_lit("units"), 0)){
          result->dump_units = 1;
        }
        else if (str8_match(node->string, str8_lit("unit_vmap"), 0)){
          result->dump_unit_vmap = 1;
        }
        else if (str8_match(node->string, str8_lit("type_nodes"), 0)){
          result->dump_type_nodes = 1;
        }
        else if (str8_match(node->string, str8_lit("udt_data"), 0)){
          result->dump_udt_data = 1;
        }
        else if (str8_match(node->string, str8_lit("global_variables"), 0)){
          result->dump_global_variables = 1;
        }
        else if (str8_match(node->string, str8_lit("global_vmap"), 0)){
          result->dump_global_vmap = 1;
        }
        else if (str8_match(node->string, str8_lit("thread_variables"), 0)){
          result->dump_thread_variables = 1;
        }
        else if (str8_match(node->string, str8_lit("procedures"), 0)){
          result->dump_procedures = 1;
        }
        else if (str8_match(node->string, str8_lit("scopes"), 0)){
          result->dump_scopes = 1;
        }
        else if (str8_match(node->string, str8_lit("scope_vmap"), 0)){
          result->dump_scope_vmap = 1;
        }
      }
    }
  }
  
  return(result);
}

////////////////////////////////
//~ Entry Point

int
main(int argc, char **argv){
  local_persist TCTX main_thread_tctx = {0};
  tctx_init_and_equip(&main_thread_tctx);
  Arena *arena = arena_alloc();
  String8List args = os_string_list_from_argcv(arena, argc, argv);
  CmdLine cmdline = cmd_line_from_string_list(arena, args);
  
  RADDBGIDUMP_Params *params = raddbgidump_params_from_cmd_line(arena, &cmdline);
  
  // show input errors
  if (params->errors.node_count > 0 &&
      !params->hide_errors.input){
    for (String8Node *node = params->errors.first;
         node != 0;
         node = node->next){
      fprintf(stderr, "error(input): %.*s\n", str8_varg(node->string));
    }
  }
  
  // will we try to parse an input file
  B32 try_parse_input = (params->errors.node_count == 0);
  
  RDI_ParseStatus parse_status = RDI_ParseStatus_Good;
  RDI_Parsed raddbg__ = {0};
  RDI_Parsed *raddbg = 0;
  if (try_parse_input){
    parse_status = rdi_parse(params->input_data.str, params->input_data.size, &raddbg__);
    if (parse_status == RDI_ParseStatus_Good){
      raddbg = &raddbg__;
    }
  }
  
  if (raddbg == 0){
    // TODO(allen): improve this by looking at parse status.
    fprintf(stderr, "error(parsing): error trying to parse the input file\n");
  }
  
  // dump
  {
    String8List dump = {0};
    
    // DATA SECTIONS
    if (raddbg->dsecs != 0 && params->dump_data_sections){
      str8_list_pushf(arena, &dump, "# DATA SECTIONS:\n");
      rdi_stringize_data_sections(arena, &dump, raddbg, 1);
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // TOP LEVEL INFO
    if (raddbg->top_level_info != 0 && params->dump_top_level_info){
      str8_list_pushf(arena, &dump, "# TOP LEVEL INFO:\n");
      rdi_stringize_top_level_info(arena, &dump, raddbg, raddbg->top_level_info, 1);
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // BINARY SECTIONS
    if (raddbg->binary_sections != 0 && params->dump_binary_sections){
      str8_list_pushf(arena, &dump, "# BINARY SECTIONS:\n");
      RDI_BinarySection *ptr = raddbg->binary_sections;
      for (U32 i = 0; i < raddbg->binary_sections_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " section[%u]:\n", i);
        rdi_stringize_binary_section(arena, &dump, raddbg, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // FILE PATHS
    if (raddbg->file_paths != 0 && params->dump_file_paths){
      RDI_FilePathBundle file_path_bundle = {0};
      {
        file_path_bundle.file_paths = raddbg->file_paths;
        file_path_bundle.file_path_count = raddbg->file_paths_count;
      }
      
      str8_list_pushf(arena, &dump, "# FILE PATHS\n");
      RDI_FilePathNode *ptr = raddbg->file_paths;
      for (U32 i = 0; i < raddbg->file_paths_count; i += 1, ptr += 1){
        if (ptr->parent_path_node == 0){
          rdi_stringize_file_path(arena, &dump, raddbg, &file_path_bundle, ptr, 1);
        }
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // SOURCE FILES
    if (raddbg->source_files != 0 && params->dump_source_files){
      str8_list_pushf(arena, &dump, "# SOURCE FILES\n");
      RDI_SourceFile *ptr = raddbg->source_files;
      for (U32 i = 0; i < raddbg->source_files_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " source_file[%u]:\n", i);
        rdi_stringize_source_file(arena, &dump, raddbg, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // UNITS
    if (raddbg->units != 0 && params->dump_units){
      str8_list_pushf(arena, &dump, "# UNITS\n");
      RDI_Unit *ptr = raddbg->units;
      for (U32 i = 0; i < raddbg->units_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " unit[%u]:\n", i);
        rdi_stringize_unit(arena, &dump, raddbg, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // UNIT VMAP
    if (raddbg->unit_vmap != 0 && params->dump_unit_vmap){
      str8_list_pushf(arena, &dump, "# UNIT VMAP\n");
      RDI_VMapEntry *ptr = raddbg->unit_vmap;
      for (U32 i = 0; i < raddbg->unit_vmap_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " 0x%08x: %llu\n", ptr->voff, ptr->idx);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // TYPE NODES
    if (raddbg->type_nodes != 0 && params->dump_type_nodes){
      str8_list_pushf(arena, &dump, "# TYPE NODES:\n");
      RDI_TypeNode *ptr = raddbg->type_nodes;
      for (U32 i = 0; i < raddbg->type_nodes_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " type[%u]:\n", i);
        rdi_stringize_type_node(arena, &dump, raddbg, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // UDT DATA
    if (raddbg->udts != 0 && params->dump_udt_data){
      RDI_UDTMemberBundle member_bundle = {0};
      {
        member_bundle.members = raddbg->members;
        member_bundle.enum_members = raddbg->enum_members;
        member_bundle.member_count = raddbg->members_count;
        member_bundle.enum_member_count = raddbg->enum_members_count;
      }
      
      str8_list_pushf(arena, &dump, "# UDTS:\n");
      RDI_UDT *ptr = raddbg->udts;
      for (U32 i = 0; i < raddbg->udts_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " udt[%u]:\n", i);
        rdi_stringize_udt(arena, &dump, raddbg, &member_bundle, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // GLOBAL VARIABLES
    if (raddbg->global_variables != 0 && params->dump_global_variables){
      str8_list_pushf(arena, &dump, "# GLOBAL VARIABLES:\n");
      RDI_GlobalVariable *ptr = raddbg->global_variables;
      for (U32 i = 0; i < raddbg->global_variables_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " global_variable[%u]:\n", i);
        rdi_stringize_global_variable(arena, &dump, raddbg, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // GLOBAL VMAP
    if (raddbg->global_vmap != 0 && params->dump_global_vmap){
      str8_list_pushf(arena, &dump, "# GLOBAL VMAP:\n");
      RDI_VMapEntry *ptr = raddbg->global_vmap;
      for (U32 i = 0; i < raddbg->global_vmap_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " 0x%08x: %llu\n", ptr->voff, ptr->idx);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // THREAD LOCAL VARIABLES
    if (raddbg->thread_variables != 0 && params->dump_thread_variables){
      str8_list_pushf(arena, &dump, "# THREAD VARIABLES:\n");
      RDI_ThreadVariable *ptr = raddbg->thread_variables;
      for (U32 i = 0; i < raddbg->thread_variables_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " thread_variable[%u]:\n", i);
        rdi_stringize_thread_variable(arena, &dump, raddbg, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // PROCEDURES
    if (raddbg->procedures != 0 && params->dump_procedures){
      str8_list_pushf(arena, &dump, "# PROCEDURES:\n");
      RDI_Procedure *ptr = raddbg->procedures;
      for (U32 i = 0; i < raddbg->procedures_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " procedure[%u]:\n", i);
        rdi_stringize_procedure(arena, &dump, raddbg, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // SCOPES
    if (raddbg->scopes != 0 && params->dump_scopes){
      RDI_ScopeBundle scope_bundle = {0};
      {
        scope_bundle.scopes = raddbg->scopes;
        scope_bundle.scope_count = raddbg->scopes_count;
        scope_bundle.scope_voffs = raddbg->scope_voffs;
        scope_bundle.scope_voff_count = raddbg->scope_voffs_count;
        scope_bundle.locals = raddbg->locals;
        scope_bundle.local_count = raddbg->locals_count;
        scope_bundle.location_blocks = raddbg->location_blocks;
        scope_bundle.location_block_count = raddbg->location_blocks_count;
        scope_bundle.location_data = raddbg->location_data;
        scope_bundle.location_data_size = raddbg->location_data_size;
      }
      
      str8_list_pushf(arena, &dump, "# SCOPES:\n");
      RDI_Scope *ptr = raddbg->scopes;
      for (U32 i = 0; i < raddbg->scopes_count; i += 1, ptr += 1){
        if (ptr->parent_scope_idx == 0){
          rdi_stringize_scope(arena, &dump, raddbg, &scope_bundle, ptr, 1);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // SCOPE VMAP
    if (raddbg->scope_vmap != 0 && params->dump_scope_vmap){
      str8_list_pushf(arena, &dump, "# SCOPE VMAP:\n");
      RDI_VMapEntry *ptr = raddbg->scope_vmap;
      for (U32 i = 0; i < raddbg->scope_vmap_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " 0x%08x: %llu\n", ptr->voff, ptr->idx);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // NAME MAPS
    if (raddbg->name_maps != 0 && params->dump_name_map){
      str8_list_pushf(arena, &dump, "# NAME MAP:\n");
      RDI_NameMap *ptr = raddbg->name_maps;
      for (U32 i = 0; i < raddbg->name_maps_count; i += 1, ptr += 1){
        str8_list_pushf(arena, &dump, " name_map[%u]:\n", i);
        
        RDI_ParsedNameMap name_map = {0};
        rdi_name_map_parse(raddbg, ptr, &name_map);
        
        RDI_NameMapBucket *bucket = name_map.buckets;
        for (U32 j = 0; j < name_map.bucket_count; j += 1, bucket += 1){
          if (bucket->node_count > 0){
            str8_list_pushf(arena, &dump, "  bucket[%u]:\n", j);
            RDI_NameMapNode *node = name_map.nodes + bucket->first_node;
            RDI_NameMapNode *node_opl = node + bucket->node_count;
            for (; node < node_opl; node += 1){
              String8 string = {0};
              string.str = rdi_string_from_idx(raddbg, node->string_idx, &string.size);
              str8_list_pushf(arena, &dump, "   match \"%.*s\": ", str8_varg(string));
              if (node->match_count == 1){
                str8_list_pushf(arena, &dump, "%u", node->match_idx_or_idx_run_first);
              }
              else{
                RDI_U32 idx_count = 0;
                RDI_U32 *idx_run =
                  rdi_idx_run_from_first_count(raddbg, node->match_idx_or_idx_run_first,
                                               node->match_count, &idx_count);
                if (idx_count > 0){
                  RDI_U32 last = idx_count - 1;
                  for (U32 k = 0; k < last; k += 1){
                    str8_list_pushf(arena, &dump, "%u, ", idx_run[k]);
                  }
                  str8_list_pushf(arena, &dump, "%u", idx_run[last]);
                }
              }
              str8_list_pushf(arena, &dump, "\n");
            }
          }
        }
        
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // print dump
    for(String8Node *node = dump.first; node != 0; node = node->next)
    {
      fwrite(node->string.str, 1, node->string.size, stdout);
    }
  }
  
  return(0);
}
