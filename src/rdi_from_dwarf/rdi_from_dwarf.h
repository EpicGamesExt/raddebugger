// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_DWARF_H
#define RDI_FROM_DWARF_H

////////////////////////////////
//~ Program Parameters Type

typedef struct DWARFCONV_Params{
  String8 input_elf_name;
  String8 input_elf_data;
  
  String8 output_name;
  
  U64 unit_idx_min;
  U64 unit_idx_max;
  
  struct{
    B8 input;
  } hide_errors;
  
  B8 dump;
  B8 dump__first;
  B8 dump_header;
  B8 dump_sections;
  B8 dump_segments;
  B8 dump_symtab;
  B8 dump_dynsym;
  B8 dump_debug_sections;
  B8 dump_debug_info;
  B8 dump_debug_abbrev;
  B8 dump_debug_pubnames;
  B8 dump_debug_pubtypes;
  B8 dump_debug_names;
  B8 dump_debug_aranges;
  B8 dump_debug_addr;
  B8 dump__last;
  
  String8List errors;
} DWARFCONV_Params;

////////////////////////////////
//~ Program Parameters Parser

static DWARFCONV_Params *dwarf_convert_params_from_cmd_line(Arena *arena, CmdLine *cmdline);



#endif //RDI_FROM_DWARF_H
