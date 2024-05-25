// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_VERSION_MAJOR 0
#define BUILD_VERSION_MINOR 9
#define BUILD_VERSION_PATCH 10
#define BUILD_RELEASE_PHASE_STRING_LITERAL "ALPHA"
#define BUILD_TITLE "rdi_dump"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [lib]
#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "rdi_format_local/rdi_format_local.h"
#include "rdi_dump.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "rdi_format_local/rdi_format_local.c"
#include "rdi_dump.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmd_line)
{
  //////////////////////////////
  //- rjf: set up
  //
  Arena *arena = arena_alloc();
  String8List errors = {0};
  
  //////////////////////////////
  //- rjf: extract command line parameters
  //
  typedef U32 DumpFlags;
  enum
  {
    DumpFlag_DataSections       = (1<<0),
    DumpFlag_TopLevelInfo       = (1<<1),
    DumpFlag_BinarySections     = (1<<2),
    DumpFlag_FilePaths          = (1<<3),
    DumpFlag_SourceFiles        = (1<<4),
    DumpFlag_Units              = (1<<5),
    DumpFlag_UnitVMap           = (1<<6),
    DumpFlag_TypeNodes          = (1<<7),
    DumpFlag_UDTs               = (1<<8),
    DumpFlag_GlobalVariables    = (1<<9),
    DumpFlag_GlobalVMap         = (1<<10),
    DumpFlag_ThreadVariables    = (1<<11),
    DumpFlag_Procedures         = (1<<12),
    DumpFlag_Scopes             = (1<<13),
    DumpFlag_ScopeVMap          = (1<<14),
    DumpFlag_NameMaps           = (1<<15),
    DumpFlag_Strings            = (1<<16),
  };
  String8 input_name = {0};
  DumpFlags dump_flags = (U32)0xffffffff;
  {
    // rjf: extract input file path
    input_name = str8_list_first(&cmd_line->inputs);
    
    // rjf: extract dump options
    {
      String8List dump_options = cmd_line_strings(cmd_line, str8_lit("dump"));
      if(dump_options.first != 0)
      {
        dump_flags = 0;
        for(String8Node *n = dump_options.first; n != 0; n = n->next)
        {
          if(0){}
          else if(str8_match(n->string, str8_lit("data_sections"),           StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_DataSections; }
          else if(str8_match(n->string, str8_lit("top_level_info"),          StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_TopLevelInfo; }
          else if(str8_match(n->string, str8_lit("binary_sections"),         StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_BinarySections; }
          else if(str8_match(n->string, str8_lit("file_paths"),              StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_FilePaths; }
          else if(str8_match(n->string, str8_lit("source_files"),            StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_SourceFiles; }
          else if(str8_match(n->string, str8_lit("units"),                   StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_Units; }
          else if(str8_match(n->string, str8_lit("unit_vmap"),               StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_UnitVMap; }
          else if(str8_match(n->string, str8_lit("type_nodes"),              StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_TypeNodes; }
          else if(str8_match(n->string, str8_lit("udt_data"),                StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_UDTs; }
          else if(str8_match(n->string, str8_lit("global_variables"),        StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_GlobalVariables; }
          else if(str8_match(n->string, str8_lit("global_vmap"),             StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_GlobalVMap; }
          else if(str8_match(n->string, str8_lit("thread_variables"),        StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_ThreadVariables; }
          else if(str8_match(n->string, str8_lit("procedures"),              StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_Procedures; }
          else if(str8_match(n->string, str8_lit("scopes"),                  StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_Scopes; }
          else if(str8_match(n->string, str8_lit("scope_vmap"),              StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_ScopeVMap; }
          else if(str8_match(n->string, str8_lit("name_maps"),               StringMatchFlag_CaseInsensitive)) { dump_flags |= DumpFlag_NameMaps; }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: load file
  //
  String8 input_data = os_data_from_file_path(arena, input_name);
  if(input_name.size == 0)
  {
    str8_list_pushf(arena, &errors, "error (input): No input RDI file specified.");
  }
  else if(input_data.size == 0)
  {
    str8_list_pushf(arena, &errors, "error (input): No input RDI file successfully loaded; either the path or file contents are invalid.");
  }
  
  //////////////////////////////
  //- rjf: obtain initial rdi parse
  //
  RDI_Parsed rdi_ = {0};
  RDI_Parsed *rdi = &rdi_;
  RDI_ParseStatus status = rdi_parse(input_data.str, input_data.size, rdi);
  
  //////////////////////////////
  //- rjf: decompress rdi if necessary
  //
  {
    U64 decompressed_size = rdi_decompressed_size_from_parsed(rdi);
    if(decompressed_size > input_data.size)
    {
      U8 *decompressed_data = push_array_no_zero(arena, U8, decompressed_size);
      rdi_decompress_parsed(decompressed_data, decompressed_size, rdi);
      status = rdi_parse(decompressed_data, decompressed_size, rdi);
    }
  }
  
  //////////////////////////////
  //- rjf: error on bad parse status
  //
  if(status != RDI_ParseStatus_Good)
  {
    str8_list_pushf(arena, &errors, "error (input): RDI file could not be successfully decoded.");
  }
  
  //////////////////////////////
  //- rjf: output error strings to stderr
  //
  for(String8Node *n = errors.first; n != 0; n = n->next)
  {
    fwrite(n->string.str, 1, n->string.size, stderr);
    fprintf(stderr, "\n");
  }
  
  //////////////////////////////
  //- rjf: build dump strings
  //
  String8List dump = {0};
  if(errors.node_count != 0)
  {
    //- rjf: DATA SECTIONS
    if(dump_flags & DumpFlag_DataSections)
    {
      str8_list_pushf(arena, &dump, "# DATA SECTIONS:\n");
      rdi_stringize_data_sections(arena, &dump, rdi, 1);
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: TOP LEVEL INFO
    if(dump_flags & DumpFlag_TopLevelInfo)
    {
      str8_list_pushf(arena, &dump, "# TOP LEVEL INFO:\n");
      rdi_stringize_top_level_info(arena, &dump, rdi, rdi->top_level_info, 1);
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: BINARY SECTIONS
    if(dump_flags & DumpFlag_BinarySections)
    {
      str8_list_pushf(arena, &dump, "# BINARY SECTIONS:\n");
      RDI_BinarySection *ptr = rdi->binary_sections;
      for(U32 i = 0; i < rdi->binary_sections_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " section[%u]:\n", i);
        rdi_stringize_binary_section(arena, &dump, rdi, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: FILE PATHS
    if(dump_flags & DumpFlag_FilePaths)
    {
      RDI_FilePathBundle file_path_bundle = {0};
      {
        file_path_bundle.file_paths = rdi->file_paths;
        file_path_bundle.file_path_count = rdi->file_paths_count;
      }
      str8_list_pushf(arena, &dump, "# FILE PATHS\n");
      RDI_FilePathNode *ptr = rdi->file_paths;
      for(U32 i = 0; i < rdi->file_paths_count; i += 1, ptr += 1)
      {
        if(ptr->parent_path_node == 0)
        {
          rdi_stringize_file_path(arena, &dump, rdi, &file_path_bundle, ptr, 1);
        }
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: SOURCE FILES
    if(dump_flags & DumpFlag_SourceFiles)
    {
      str8_list_pushf(arena, &dump, "# SOURCE FILES\n");
      RDI_SourceFile *ptr = rdi->source_files;
      for(U32 i = 0; i < rdi->source_files_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " source_file[%u]:\n", i);
        rdi_stringize_source_file(arena, &dump, rdi, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: UNITS
    if(dump_flags & DumpFlag_Units)
    {
      str8_list_pushf(arena, &dump, "# UNITS\n");
      RDI_Unit *ptr = rdi->units;
      for (U32 i = 0; i < rdi->units_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " unit[%u]:\n", i);
        rdi_stringize_unit(arena, &dump, rdi, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: UNIT VMAP
    if(dump_flags & DumpFlag_UnitVMap)
    {
      str8_list_pushf(arena, &dump, "# UNIT VMAP\n");
      RDI_VMapEntry *ptr = rdi->unit_vmap;
      for(U32 i = 0; i < rdi->unit_vmap_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " 0x%08x: %llu\n", ptr->voff, ptr->idx);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: TYPE NODES
    if(dump_flags & DumpFlag_TypeNodes)
    {
      str8_list_pushf(arena, &dump, "# TYPE NODES:\n");
      RDI_TypeNode *ptr = rdi->type_nodes;
      for(U32 i = 0; i < rdi->type_nodes_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " type[%u]:\n", i);
        rdi_stringize_type_node(arena, &dump, rdi, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: UDT DATA
    if(dump_flags & DumpFlag_UDTs)
    {
      RDI_UDTMemberBundle member_bundle = {0};
      {
        member_bundle.members = rdi->members;
        member_bundle.enum_members = rdi->enum_members;
        member_bundle.member_count = rdi->members_count;
        member_bundle.enum_member_count = rdi->enum_members_count;
      }
      str8_list_pushf(arena, &dump, "# UDTS:\n");
      RDI_UDT *ptr = rdi->udts;
      for(U32 i = 0; i < rdi->udts_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " udt[%u]:\n", i);
        rdi_stringize_udt(arena, &dump, rdi, &member_bundle, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: GLOBAL VARIABLES
    if(dump_flags & DumpFlag_GlobalVariables)
    {
      str8_list_pushf(arena, &dump, "# GLOBAL VARIABLES:\n");
      RDI_GlobalVariable *ptr = rdi->global_variables;
      for(U32 i = 0; i < rdi->global_variables_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " global_variable[%u]:\n", i);
        rdi_stringize_global_variable(arena, &dump, rdi, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: GLOBAL VMAP
    if(dump_flags & DumpFlag_GlobalVMap)
    {
      str8_list_pushf(arena, &dump, "# GLOBAL VMAP:\n");
      RDI_VMapEntry *ptr = rdi->global_vmap;
      for(U32 i = 0; i < rdi->global_vmap_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " 0x%08x: %llu\n", ptr->voff, ptr->idx);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: THREAD LOCAL VARIABLES
    if(dump_flags & DumpFlag_ThreadVariables)
    {
      str8_list_pushf(arena, &dump, "# THREAD VARIABLES:\n");
      RDI_ThreadVariable *ptr = rdi->thread_variables;
      for(U32 i = 0; i < rdi->thread_variables_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " thread_variable[%u]:\n", i);
        rdi_stringize_thread_variable(arena, &dump, rdi, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: PROCEDURES
    if(dump_flags & DumpFlag_Procedures)
    {
      str8_list_pushf(arena, &dump, "# PROCEDURES:\n");
      RDI_Procedure *ptr = rdi->procedures;
      for(U32 i = 0; i < rdi->procedures_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " procedure[%u]:\n", i);
        rdi_stringize_procedure(arena, &dump, rdi, ptr, 2);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: SCOPES
    if(dump_flags & DumpFlag_Scopes)
    {
      RDI_ScopeBundle scope_bundle = {0};
      {
        scope_bundle.scopes = rdi->scopes;
        scope_bundle.scope_count = rdi->scopes_count;
        scope_bundle.scope_voffs = rdi->scope_voffs;
        scope_bundle.scope_voff_count = rdi->scope_voffs_count;
        scope_bundle.locals = rdi->locals;
        scope_bundle.local_count = rdi->locals_count;
        scope_bundle.location_blocks = rdi->location_blocks;
        scope_bundle.location_block_count = rdi->location_blocks_count;
        scope_bundle.location_data = rdi->location_data;
        scope_bundle.location_data_size = rdi->location_data_size;
      }
      str8_list_pushf(arena, &dump, "# SCOPES:\n");
      RDI_Scope *ptr = rdi->scopes;
      for(U32 i = 0; i < rdi->scopes_count; i += 1, ptr += 1)
      {
        if(ptr->parent_scope_idx == 0)
        {
          rdi_stringize_scope(arena, &dump, rdi, &scope_bundle, ptr, 1);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: SCOPE VMAP
    if(dump_flags & DumpFlag_ScopeVMap)
    {
      str8_list_pushf(arena, &dump, "# SCOPE VMAP:\n");
      RDI_VMapEntry *ptr = rdi->scope_vmap;
      for(U32 i = 0; i < rdi->scope_vmap_count; i += 1, ptr += 1)
      {
        str8_list_pushf(arena, &dump, " 0x%08x: %llu\n", ptr->voff, ptr->idx);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    //- rjf: NAME MAPS
    if(dump_flags & DumpFlag_NameMaps)
    {
      str8_list_pushf(arena, &dump, "# NAME MAP:\n");
      RDI_NameMap *ptr = rdi->name_maps;
      for(U32 i = 0; i < rdi->name_maps_count; i += 1, ptr += 1)
      {
        RDI_ParsedNameMap name_map = {0};
        rdi_name_map_parse(rdi, ptr, &name_map);
        str8_list_pushf(arena, &dump, " name_map[%u]:\n", i);
        RDI_NameMapBucket *bucket = name_map.buckets;
        for(U32 j = 0; j < name_map.bucket_count; j += 1, bucket += 1)
        {
          if(bucket->node_count > 0)
          {
            str8_list_pushf(arena, &dump, "  bucket[%u]:\n", j);
            RDI_NameMapNode *node = name_map.nodes + bucket->first_node;
            RDI_NameMapNode *node_opl = node + bucket->node_count;
            for(;node < node_opl; node += 1)
            {
              String8 string = {0};
              string.str = rdi_string_from_idx(rdi, node->string_idx, &string.size);
              str8_list_pushf(arena, &dump, "   match \"%.*s\": ", str8_varg(string));
              if(node->match_count == 1)
              {
                str8_list_pushf(arena, &dump, "%u", node->match_idx_or_idx_run_first);
              }
              else
              {
                RDI_U32 idx_count = 0;
                RDI_U32 *idx_run =
                  rdi_idx_run_from_first_count(rdi, node->match_idx_or_idx_run_first,
                                               node->match_count, &idx_count);
                if(idx_count > 0)
                {
                  RDI_U32 last = idx_count - 1;
                  for(U32 k = 0; k < last; k += 1)
                  {
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
    
    //- rjf: STRINGS
    if(dump_flags & DumpFlag_Strings)
    {
      str8_list_pushf(arena, &dump, "# STRINGS:\n");
      for(U64 string_idx = 0; string_idx < rdi->string_count; string_idx += 1)
      {
        String8 string = {0};
        string.str = rdi_string_from_idx(rdi, string_idx, &string.size);
        str8_list_pushf(arena, &dump, " string[%I64u]: \"%S\"\n", string_idx, string);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
  }
  
  //////////////////////////////
  //- rjf: write dump to stdout
  //
  for(String8Node *n = dump.first; n != 0; n = n->next)
  {
    fwrite(n->string.str, 1, n->string.size, stdout);
  }
}
