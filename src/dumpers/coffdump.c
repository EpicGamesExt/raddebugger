// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define BUILD_CONSOLE_INTERFACE 1
#define BUILD_TITLE "Epic Games Tools (R) COFF/PE Dumper"

////////////////////////////////

#include "third_party/xxHash/xxhash.c"
#include "third_party/xxHash/xxhash.h"
#include "third_party/radsort/radsort.h"

////////////////////////////////

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "path/path.h"
#include "coff/coff.h"
#include "pe/pe.h"
#include "msvc_crt/msvc_crt.h"
#include "codeview/codeview.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "path/path.c"
#include "coff/coff.c"
#include "pe/pe.c"
#include "msvc_crt/msvc_crt.c"
#include "codeview/codeview.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
 
#include "linker/base_ext/base_core.h"
#include "linker/base_ext/base_core.c"
#include "linker/path_ext/path.h"
#include "linker/path_ext/path.c"
#include "linker/hash_table.h"
#include "linker/hash_table.c"

////////////////////////////////

#define coff_printf(f, ...) str8_list_pushf(arena, out, "%S" f, indent, __VA_ARGS__)
#define coff_newline()      str8_list_pushf(arena, out, "");
#define coff_errorf(f, ...) coff_printf("ERROR: "f, __VA_ARGS__)
#define coff_indent()    indent.size += 4
#define coff_unindent()  indent.size -= 4

////////////////////////////////

typedef U64 CoffdumpOption;
enum CoffdumpOptionEnum
{
  CoffdumpOption_Help       = (1 << 0),
  CoffdumpOption_Version    = (1 << 1),
  CoffdumpOption_Headers    = (1 << 2),
  CoffdumpOption_Sections   = (1 << 3),
  CoffdumpOption_Debug      = (1 << 4),
  CoffdumpOption_Imports    = (1 << 5),
  CoffdumpOption_Exports    = (1 << 6),
  CoffdumpOption_Disasm     = (1 << 7),
  CoffdumpOption_Rawdata    = (1 << 8),
  CoffdumpOption_Tls        = (1 << 9),
  CoffdumpOption_Codeview   = (1 << 10),
  CoffdumpOption_Symbols    = (1 << 11),
  CoffdumpOption_Relocs     = (1 << 12),
  CoffdumpOption_Exceptions = (1 << 13),
  CoffdumpOption_LoadConfig = (1 << 14),
  CoffdumpOption_Resources  = (1 << 15),
  CoffdumpOption_LongNames  = (1 << 16),
};

 global read_only struct {
  CoffdumpOption  opt;
  char           *name;
  char           *help;
} g_coffdump_option_map[] = {
  { CoffdumpOption_Help,       "help",       "Print help and exit"                           },
  { CoffdumpOption_Version,    "v",          "Print version and exit"                        },
  { CoffdumpOption_Headers,    "headers",    "Dump dos header, file header, optional header" },
  { CoffdumpOption_Sections,   "sections",   "Dump section headers as table"                 },
  { CoffdumpOption_Debug,      "debug",      "Dump debug directory"                          },
  { CoffdumpOption_Imports,    "imports",    "Dump import table"                             },
  { CoffdumpOption_Exports,    "exports",    "Dump export table"                             },
  { CoffdumpOption_Disasm,     "disasm",     "Disassemble code sections"                     },
  { CoffdumpOption_Rawdata,    "rawdata",    "Dump raw section data"                         },
  { CoffdumpOption_Tls,        "tls",        "Dump Thread Local Storage directory"           },
  { CoffdumpOption_Codeview,   "cv",         "Dump relocations"                              },
  { CoffdumpOption_Symbols,    "symbols",    "Dump COFF symbol table"                        },
  { CoffdumpOption_Relocs,     "relocs",     "Dump relocations"                              },
  { CoffdumpOption_Exceptions, "exceptions", "Dump exceptions"                               },
  { CoffdumpOption_LoadConfig, "loadconfig", "Dump load config"                              },
  { CoffdumpOption_Resources,  "resources",  "Dump resource directory"                       },
  { CoffdumpOption_LongNames,  "longnames",  "COFF Archive: Dump Long Names Member"          },
};

////////////////////////////////
//~ COFF

internal String8
coff_format_time_stamp(Arena *arena, COFF_TimeStamp time_stamp)
{
  String8 result;
  if (time_stamp >= max_U32) {
    result = str8_lit("-1");
  } else {
    DateTime dt = date_time_from_unix_time(time_stamp);
    result = push_date_time_string(arena, &dt);
  }
  return result;
}

internal void
coff_format_archive_member_header(Arena *arena, String8List *out, String8 indent, COFF_ArchiveMemberHeader header, String8 long_names)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 time_stamp = coff_format_time_stamp(scratch.arena, header.time_stamp);

  coff_printf("Name:       %S"             , header.name    );
  coff_printf("Time Stamp: %S"             , time_stamp     );
  coff_printf("User ID:    %u"             , header.user_id );
  coff_printf("Group ID:   %u"             , header.group_id);
  coff_printf("Mode:       %S"             , header.mode    );
  coff_printf("Data:       [%#llx-%#llx)", header.data_range.min, header.data_range.max);

  scratch_end(scratch);
}

internal void
coff_format_section_table(Arena              *arena,
                          String8List        *out,
                          String8             indent,
                          String8             raw_data,
                          U64                 string_table_off,
                          COFF_Symbol32Array  symbols,
                          U64                 sect_count,
                          COFF_SectionHeader *sect_headers)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 *symlinks = push_array(scratch.arena, String8, sect_count);
  for (U64 i = 0; i < symbols.count; ++i) {
    COFF_Symbol32              *symbol = symbols.v+i;
    COFF_SymbolValueInterpType  interp = coff_interp_symbol(symbol->section_number, symbol->value, symbol->storage_class);
    if (interp == COFF_SymbolValueInterp_REGULAR &&
        symbol->aux_symbol_count == 0 &&
        (symbol->storage_class == COFF_SymStorageClass_EXTERNAL || symbol->storage_class == COFF_SymStorageClass_STATIC)) {
      if (symbol->section_number > 0 && symbol->section_number <= symbols.count) {
        COFF_SectionHeader *header = sect_headers+(symbol->section_number-1);
        if (header->flags & COFF_SectionFlag_LNK_COMDAT) {
          symlinks[symbol->section_number-1] = coff_read_symbol_name(raw_data, string_table_off, &symbol->name);
        }
      }
    }
    i += symbol->aux_symbol_count;
  }

  if (sect_count) {
    coff_printf("# Section Table");
    coff_indent();

    coff_printf("%-4s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-5s %-10s %s",
                "No.",
                "Name",
                "VirtSize",
                "VirtOff",
                "FileSize",
                "FileOff",
                "RelocOff",
                "LinesOff",
                "RelocCnt",
                "LineCnt",
                "Align",
                "Flags",
                "Symlink");

    for (U64 i = 0; i < sect_count; ++i) {
      COFF_SectionHeader *header = sect_headers+i;

      String8 name      = str8_cstring_capped(header->name, header->name+sizeof(header->name));
      String8 full_name = coff_name_from_section_header(header, raw_data, string_table_off);

      String8 align;
      {
        U64 align_size = coff_align_size_from_section_flags(header->flags);
        align = push_str8f(scratch.arena, "%u", align_size);
      }

      String8 flags;
      {
        String8List mem_flags = {0};
        if (header->flags & COFF_SectionFlag_MEM_READ) {
          str8_list_pushf(scratch.arena, &mem_flags, "r");
        }
        if (header->flags & COFF_SectionFlag_MEM_WRITE) {
          str8_list_pushf(scratch.arena, &mem_flags, "w");
        }
        if (header->flags & COFF_SectionFlag_MEM_EXECUTE) {
          str8_list_pushf(scratch.arena, &mem_flags, "x");
        }

        String8List cnt_flags = {0};
        if (header->flags & COFF_SectionFlag_CNT_CODE) {
          str8_list_pushf(scratch.arena, &cnt_flags, "c");
        }
        if (header->flags & COFF_SectionFlag_CNT_INITIALIZED_DATA) {
          str8_list_pushf(scratch.arena, &cnt_flags, "d");
        }
        if (header->flags & COFF_SectionFlag_CNT_UNINITIALIZED_DATA) {
          str8_list_pushf(scratch.arena, &cnt_flags, "u");
        }

        String8List mem_extra_flags = {0};
        if (header->flags & COFF_SectionFlag_MEM_SHARED) {
          str8_list_pushf(scratch.arena, &mem_flags, "s");
        }
        if (header->flags & COFF_SectionFlag_MEM_16BIT) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "h");
        }
        if (header->flags & COFF_SectionFlag_MEM_LOCKED) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "l");
        }
        if (header->flags & COFF_SectionFlag_MEM_DISCARDABLE) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "d");
        }
        if (header->flags & COFF_SectionFlag_MEM_NOT_CACHED) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "c");
        }
        if (header->flags & COFF_SectionFlag_MEM_NOT_PAGED) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "p");
        }

        String8List lnk_flags = {0};
        if (header->flags & COFF_SectionFlag_LNK_REMOVE) {
          str8_list_pushf(scratch.arena, &lnk_flags, "r");
        }
        if (header->flags & COFF_SectionFlag_LNK_COMDAT) {
          str8_list_pushf(scratch.arena, &lnk_flags, "c");
        }
        if (header->flags & COFF_SectionFlag_LNK_OTHER) {
          str8_list_pushf(scratch.arena, &lnk_flags, "o");
        }
        if (header->flags & COFF_SectionFlag_LNK_INFO) {
          str8_list_pushf(scratch.arena, &lnk_flags, "i");
        }
        if (header->flags & COFF_SectionFlag_LNK_NRELOC_OVFL) {
          str8_list_pushf(scratch.arena, &lnk_flags, "f");
        }

        String8List other_flags = {0};
        if (header->flags & COFF_SectionFlag_TYPE_NO_PAD) {
          str8_list_pushf(scratch.arena, &other_flags, "n");
        }
        if (header->flags & COFF_SectionFlag_GPREL) {
          str8_list_pushf(scratch.arena, &other_flags, "g");
        }

        String8 mem = str8_list_join(scratch.arena, &mem_flags, 0);
        String8 cnt = str8_list_join(scratch.arena, &cnt_flags, 0);
        String8 lnk = str8_list_join(scratch.arena, &lnk_flags, 0);
        String8 ext = str8_list_join(scratch.arena, &mem_extra_flags, 0);
        String8 oth = str8_list_join(scratch.arena, &other_flags, 0);

        String8List f = {0};
        str8_list_push(scratch.arena, &f, mem);
        str8_list_push(scratch.arena, &f, cnt);
        str8_list_push(scratch.arena, &f, ext);
        str8_list_push(scratch.arena, &f, lnk);
        str8_list_push(scratch.arena, &f, oth);

        flags = str8_list_join(scratch.arena, &f, &(StringJoin){ .sep = str8_lit("-") });

        if (!flags.size) {
          flags = str8_lit("none");
        }
      }

      String8List l = {0};
      str8_list_pushf(scratch.arena, &l, "%-4x",  i                  );
      str8_list_pushf(scratch.arena, &l, "%-8S",  name               );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->vsize      );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->voff       );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->fsize      );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->foff       );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->relocs_foff);
      str8_list_pushf(scratch.arena, &l, "%08x",  header->lines_foff );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->reloc_count);
      str8_list_pushf(scratch.arena, &l, "%08x",  header->line_count );
      str8_list_pushf(scratch.arena, &l, "%-5S",  align              );
      str8_list_pushf(scratch.arena, &l, "%-10S", flags              );
      if (symlinks[i].size > 0) {
        str8_list_pushf(scratch.arena, &l, "%S", symlinks[i]);
      } else {
        str8_list_pushf(scratch.arena, &l, "[no symlink]");
      }

      String8 line = str8_list_join(scratch.arena, &l, &(StringJoin){ .sep = str8_lit(" "), });
      coff_printf("%S", line);

      if (full_name.size != name.size) {
        coff_indent();
        coff_printf("Full Name: %S", full_name);
        coff_unindent();
      }
    }

    coff_newline();
    coff_printf("Flags:");
    coff_indent();
    coff_printf("r = MEM_READ    w = MEM_WRITE        x = MEM_EXECUTE");
    coff_printf("c = CNT_CODE    d = INITIALIZED_DATA u = UNINITIALIZED_DATA");
    coff_printf("s = MEM_SHARED  h = MEM_16BIT        l = MEM_LOCKED          d = MEM_DISCARDABLE c = MEM_NOT_CACHED  p = MEM_NOT_PAGED");
    coff_printf("r = LNK_REMOVE  c = LNK_COMDAT       o = LNK_OTHER           i = LNK_INFO        f = LNK_NRELOC_OVFL");
    coff_printf("g = GPREL       n = TYPE_NO_PAD");
    coff_unindent();

    coff_unindent();
    coff_newline();
  }

  scratch_end(scratch);
}

internal void
coff_format_relocs(Arena              *arena,
                   String8List        *out,
                   String8             indent,
                   String8             raw_data,
                   U64                 string_table_off,
                   COFF_MachineType    machine,
                   U64                 sect_count,
                   COFF_SectionHeader *sect_headers,
                   COFF_Symbol32Array  symbols)
{
  Temp scratch = scratch_begin(&arena, 1);

  B32 print_header = 1;

  for (U64 sect_idx = 0; sect_idx < sect_count; ++sect_idx) {
    COFF_SectionHeader *sect_header = sect_headers+sect_idx;
    COFF_RelocInfo      reloc_info  = coff_reloc_info_from_section_header(raw_data, sect_header);

    if (reloc_info.count) {
      if (print_header) {
        print_header = 0;
        coff_printf("# Relocations");
        coff_indent();
      }

      coff_printf("## Section %llx", sect_idx);
      coff_indent();

      coff_printf("%-4s %-8s %-16s %-16s %-8s %-7s", "No.", "Offset", "Type", "ApplyTo", "SymIdx", "SymName");

      for (U64 reloc_idx = 0; reloc_idx < reloc_info.count; ++reloc_idx) {
        COFF_Reloc *reloc      = (COFF_Reloc*)(raw_data.str + reloc_info.array_off) + reloc_idx;
        String8     type       = coff_string_from_reloc(machine, reloc->type);
        U64         apply_size = coff_apply_size_from_reloc(machine, reloc->type);

        U64 apply_foff = sect_header->foff + reloc->apply_off;
        if (apply_foff + apply_size > raw_data.size) {
          coff_errorf("out of bounds apply file offset %#llx in relocation %#llx", apply_foff, reloc_idx);
          break;
        }

        U64 raw_apply;
        AssertAlways(apply_size <= sizeof(raw_apply));
        MemoryCopy(&raw_apply, raw_data.str + apply_foff, apply_size);
        S64 apply = extend_sign64(raw_apply, apply_size);

        if (reloc->isymbol > symbols.count) {
          coff_errorf("out of bounds symbol index %u in relocation %#llx", reloc->isymbol, reloc_idx);
          break;
        }

        COFF_Symbol32 *symbol      = symbols.v+reloc->isymbol;
        String8        symbol_name = coff_read_symbol_name(raw_data, string_table_off, &symbol->name);

        String8List line = {0};
        str8_list_pushf(scratch.arena, &line, "%-4x",  reloc_idx       );
        str8_list_pushf(scratch.arena, &line, "%08x",  reloc->apply_off);
        str8_list_pushf(scratch.arena, &line, "%-16S", type            );
        str8_list_pushf(scratch.arena, &line, "%016x", apply           );
        str8_list_pushf(scratch.arena, &line, "%S",    symbol_name     );

        String8 l = str8_list_join(scratch.arena, &line, &(StringJoin){.sep=str8_lit(" ")});
        coff_printf("%S", l);
      }

      coff_unindent();
    }
  }

  if (!print_header) {
    coff_unindent();
  }
  coff_newline();

  scratch_end(scratch);
}

internal void
coff_format_symbol_table(Arena *arena, String8List *out, String8 indent, String8 raw_data, B32 is_big_obj, U64 string_table_off, COFF_Symbol32Array symbols)
{
  Temp scratch = scratch_begin(&arena, 1);

  coff_printf("# Symbol Table");
  coff_indent();

  coff_printf("%-4s %-8s %-10s %-4s %-4s %-4s %-16s %-20s", 
              "No.", "Value", "SectNum", "Aux", "Msb", "Lsb", "Storage", "Name");

  for (U64 i = 0; i < symbols.count; ++i) {
    COFF_Symbol32 *symbol        = &symbols.v[i];
    String8        name          = coff_read_symbol_name(raw_data, string_table_off, &symbol->name);
    String8        msb           = coff_string_from_sym_dtype(symbol->type.u.msb);
    String8        lsb           = coff_string_from_sym_type(symbol->type.u.lsb);
    String8        storage_class = coff_string_from_sym_storage_class(symbol->storage_class);
    String8        section_number;
    switch (symbol->section_number) {
      case COFF_SYMBOL_UNDEFINED_SECTION: section_number = str8_lit("UNDEF"); break;
      case COFF_SYMBOL_ABS_SECTION:       section_number = str8_lit("ABS");   break;
      case COFF_SYMBOL_DEBUG_SECTION:     section_number = str8_lit("DEBUG"); break;
      default:                            section_number = push_str8f(scratch.arena, "%010x", symbol->section_number); break;
    }

    String8List line = {0};
    str8_list_pushf(scratch.arena, &line, "%-4x",  i                       );
    str8_list_pushf(scratch.arena, &line, "%08x",  symbol->value           );
    str8_list_pushf(scratch.arena, &line, "%-10S", section_number          );
    str8_list_pushf(scratch.arena, &line, "%-4u",  symbol->aux_symbol_count);
    str8_list_pushf(scratch.arena, &line, "%-4S",  msb                     );
    str8_list_pushf(scratch.arena, &line, "%-4S",  lsb                     );
    str8_list_pushf(scratch.arena, &line, "%-16S", storage_class           );
    str8_list_pushf(scratch.arena, &line, "%S",    name                    );

    String8 l = str8_list_join(scratch.arena, &line, &(StringJoin){.sep = str8_lit(" ")});
    coff_printf("%S", l);

    coff_indent();
    for (U64 k=i+1, c = i+symbol->aux_symbol_count; k <= c; ++k) {
      void *raw_aux = &symbols.v[k];
      switch (symbol->storage_class) {
        case COFF_SymStorageClass_EXTERNAL: {
          COFF_SymbolFuncDef *func_def = (COFF_SymbolFuncDef*)&symbols.v[k];
          coff_printf("Tag Index %#x, Total Size %#x, Line Numbers %#x, Next Function %#x", 
                     func_def->tag_index, func_def->total_size, func_def->ptr_to_ln, func_def->ptr_to_next_func);
        } break;
        case COFF_SymStorageClass_FUNCTION: {
          COFF_SymbolFunc *func = raw_aux;
          coff_printf("Ordinal Line Number %#x, Next Function %#x", func->ln, func->ptr_to_next_func);
        } break;
        case COFF_SymStorageClass_WEAK_EXTERNAL: {
          COFF_SymbolWeakExt *weak = raw_aux;
          String8             type = coff_string_from_weak_ext_type(weak->characteristics);
          coff_printf("Tag Index %#x, Characteristics %S", weak->tag_index, type);
        } break;
        case COFF_SymStorageClass_FILE: {
          COFF_SymbolFile *file = raw_aux;
          String8          name = str8_cstring_capped(file->name, file->name+sizeof(file->name));
          coff_printf("Name %S", name);
        } break;
        case COFF_SymStorageClass_STATIC: {
          COFF_SymbolSecDef *sd        = raw_aux;
          String8            selection = coff_string_from_selection(sd->selection);
          U32 number = sd->number_lo;
          if (is_big_obj) {
            number |= (U32)sd->number_hi << 16;
          }
          if (number) {
            coff_printf("Length %x, Reloc Count %u, Line Count %u, Checksum %x, Section %x, Selection %S",
                       sd->length, sd->number_of_relocations, sd->number_of_ln, sd->check_sum, number, selection);
          } else {
            coff_printf("Length %x, Reloc Count %u, Line Count %u, Checksum %x",
                       sd->length, sd->number_of_relocations, sd->number_of_ln, sd->check_sum);
          }
        } break;
        default: {
          coff_printf("???");
        } break;
      }
    }

    i += symbol->aux_symbol_count;
    coff_unindent();
  }

  coff_unindent();
  coff_newline();

  scratch_end(scratch);
}

internal void
coff_format_big_obj_header(Arena *arena, String8List *out, String8 indent, COFF_HeaderBigObj *header)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 time_stamp = coff_format_time_stamp(scratch.arena, header->time_stamp);
  String8 machine    = coff_string_from_machine_type(header->machine);

  coff_printf("# Big Obj");
  coff_indent();

  coff_printf("Time Stamp:    %S",     time_stamp             );
  coff_printf("Machine:       %S",        machine             );
  coff_printf("Section Count: %u",  header->section_count     );
  coff_printf("Symbol Table:  %#x", header->symbol_table_foff);
  coff_printf("Symbol Count:  %u",   header->symbol_count     );

  coff_unindent();

  scratch_end(scratch);
}

internal void
coff_format_header(Arena *arena, String8List *out, String8 indent, COFF_Header *header)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 time_stamp = coff_format_time_stamp(scratch.arena, header->time_stamp);
  String8 machine    = coff_string_from_machine_type(header->machine);
  String8 flags      = coff_string_from_flags(scratch.arena, header->flags);

  coff_printf("# COFF Header");
  coff_indent();
  coff_printf("Time Stamp:           %S",   time_stamp                  );
  coff_printf("Machine:              %S",   machine                     );
  coff_printf("Section Count:        %u",   header->section_count       );
  coff_printf("Symbol Table:         %#x", header->symbol_table_foff   );
  coff_printf("Symbol Count:         %u",   header->symbol_count        );
  coff_printf("Optional Header Size: %m",   header->optional_header_size);
  coff_printf("Flags:                %S",   flags                       );
  coff_unindent();

  scratch_end(scratch);
}

internal void
coff_format_import(Arena *arena, String8List *out, String8 indent, COFF_ImportHeader *header)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 machine    = coff_string_from_machine_type(header->machine);
  String8 time_stamp = coff_format_time_stamp(scratch.arena, header->time_stamp);

  coff_printf("# Import");
  coff_indent();
  coff_printf("Version:    %u", header->version  );
  coff_printf("Machine:    %S", machine          );
  coff_printf("Time Stamp: %S", time_stamp       );
  coff_printf("Data Size:  %m", header->data_size);
  coff_printf("Hint:       %u", header->hint     );
  coff_printf("Type:       %u", header->type     );
  coff_printf("Name Type:  %u", header->name_type);
  coff_printf("Function:   %S", header->func_name);
  coff_printf("DLL:        %S", header->dll_name );
  coff_unindent();

  scratch_end(scratch);
}

internal void
coff_format_big_obj(Arena *arena, String8List *out, String8 indent, String8 raw_data, CoffdumpOption opts)
{
  Temp scratch = scratch_begin(&arena, 1);

  COFF_HeaderBigObj  *big_obj          = str8_deserial_get_raw_ptr(raw_data, 0, sizeof(COFF_HeaderBigObj));
  COFF_SectionHeader *sections         = str8_deserial_get_raw_ptr(raw_data, sizeof(COFF_HeaderBigObj), sizeof(COFF_SectionHeader)*big_obj->section_count);
  U64                 string_table_off = big_obj->symbol_table_foff + sizeof(COFF_Symbol32)*big_obj->symbol_count;
  COFF_Symbol32Array  symbols          = coff_symbol_array_from_data_32(scratch.arena, raw_data, big_obj->symbol_table_foff, big_obj->symbol_count);

  if (opts & CoffdumpOption_Headers) {
    coff_format_big_obj_header(arena, out, indent, big_obj);
    coff_newline();
  }

  if (opts & CoffdumpOption_Sections) {
    Rng1U64 sect_headers_range = rng_1u64(sizeof(*big_obj), sizeof(*big_obj) + sizeof(COFF_SectionHeader)*big_obj->section_count);
    Rng1U64 symbols_range      = rng_1u64(big_obj->symbol_table_foff, big_obj->symbol_table_foff + sizeof(COFF_Symbol32)*big_obj->symbol_count);

    if (sect_headers_range.max > raw_data.size) {
      coff_errorf("not enough bytes to read big obj section headers");
      goto exit;
    }
    if (big_obj->symbol_count) {
      if (symbols_range.max > raw_data.size) {
        coff_errorf("not enough bytes to read big obj symbol table");
        goto exit;
      }
      if (contains_1u64(symbols_range, sect_headers_range.min) ||
        contains_1u64(symbols_range, sect_headers_range.max)) {
        coff_errorf("section headers and symbol table ranges overlap");
        goto exit;
      }
    }

    coff_format_section_table(arena, out, indent, raw_data, string_table_off, symbols, big_obj->section_count, sections);
    coff_newline();
  }

  if (opts & CoffdumpOption_Relocs) {
    coff_format_relocs(arena, out, indent, raw_data, string_table_off, big_obj->machine, big_obj->section_count, sections, symbols);
    coff_newline();
  }

  if (opts & CoffdumpOption_Symbols) {
    coff_format_symbol_table(arena, out, indent, raw_data, string_table_off, 1, symbols);
    coff_newline();
  }

exit:;
  scratch_end(scratch);
}

internal void
coff_format_obj(Arena *arena, String8List *out, String8 indent, String8 raw_data, CoffdumpOption opts)
{
  Temp scratch = scratch_begin(&arena, 1);

  COFF_Header        *header           = (COFF_Header *)raw_data.str;
  COFF_SectionHeader *sections         = (COFF_SectionHeader *)(header+1);
  U64                 string_table_off = header->symbol_table_foff + sizeof(COFF_Symbol16)*header->symbol_count;
  COFF_Symbol32Array  symbols          = coff_symbol_array_from_data_16(scratch.arena, raw_data, header->symbol_table_foff, header->symbol_count);

  if (opts & CoffdumpOption_Headers) {
    coff_format_header(arena, out, indent, header);
    coff_newline();
  }

  if (opts & CoffdumpOption_Sections) {
    Rng1U64 sect_headers_range = rng_1u64(sizeof(*header), sizeof(*header) + sizeof(COFF_SectionHeader)*header->section_count);
    Rng1U64 symbols_range      = rng_1u64(header->symbol_table_foff, header->symbol_table_foff + sizeof(COFF_Symbol16)*header->symbol_count);

    if (sect_headers_range.max > raw_data.size) {
      coff_errorf("not enough bytes to read obj section headers");
      goto exit;
    }
    if (header->symbol_count) {
      if (symbols_range.max > raw_data.size) {
        coff_errorf("not enough bytes to read obj symbol table");
        goto exit;
      }
      if (contains_1u64(symbols_range, sect_headers_range.min) ||
        contains_1u64(symbols_range, sect_headers_range.max)) {
        coff_errorf("section headers and symbol table ranges overlap");
        goto exit;
      }
    }

    coff_format_section_table(arena, out, indent, raw_data, string_table_off, symbols, header->section_count, sections);
    coff_newline();
  }

  if (opts & CoffdumpOption_Relocs) {
    coff_format_relocs(arena, out, indent, raw_data, string_table_off, header->machine, header->section_count, sections, symbols);
    coff_newline();
  }

  if (opts & CoffdumpOption_Symbols) {
    coff_format_symbol_table(arena, out, indent, raw_data, 0, string_table_off, symbols);
    coff_newline();
  }

exit:;
  scratch_end(scratch);
}

internal void
coff_format_archive(Arena *arena, String8List *out, String8 indent, String8 raw_archive, CoffdumpOption opts)
{
  Temp scratch = scratch_begin(&arena, 1);

  COFF_ArchiveParse archive_parse = coff_archive_parse_from_data(raw_archive);

  if (archive_parse.error.size) {
    coff_errorf("%S", archive_parse.error);
    return;
  }

  COFF_ArchiveFirstMember first_member = archive_parse.first_member;
  {
    coff_printf("# First Header");
    coff_indent();

    coff_printf("Symbol Count:      %u", first_member.symbol_count);
    coff_printf("String Table Size: %M", first_member.string_table.size);

    coff_printf("Members:");
    coff_indent();

    String8List string_table = str8_split_by_string_chars(scratch.arena, first_member.string_table, str8_lit("\0"), 0);

    if (string_table.node_count == first_member.member_offset_count) {
      String8Node *string_n = string_table.first;

      for (U64 i = 0; i < string_table.node_count; ++i, string_n = string_n->next) {
        U32 offset = from_be_u32(first_member.member_offsets[i]);
        coff_printf("[%4u] 0x%08x %S", i, offset, string_n->string);
      }
    } else {
      coff_errorf("Member offset count (%llu) doesn't match string table count (%llu)", first_member.member_offset_count);
    }

    coff_unindent();
    coff_unindent();
    coff_newline();
  }

  if (archive_parse.has_second_header) {
    COFF_ArchiveSecondMember second_member = archive_parse.second_member;

    coff_printf("# Second Header");
    coff_indent();

    coff_printf("Member Count:      %u", second_member.member_count);
    coff_printf("Symbol Count:      %u", second_member.symbol_count);
    coff_printf("String Table Size: %M", second_member.string_table.size);

    String8List string_table = str8_split_by_string_chars(scratch.arena, second_member.string_table, str8_lit("\0"), 0);

    coff_printf("Members:");
    coff_indent();
    if (second_member.symbol_index_count == second_member.symbol_count) {
      String8Node *string_n = string_table.first;
      for (U64 i = 0; i < second_member.symbol_count; ++i, string_n = string_n->next) {
        U16 symbol_number = second_member.symbol_indices[i];
        if (symbol_number > 0 && symbol_number <= second_member.member_offset_count) {
          U16 symbol_idx    = symbol_number - 1;
          U32 member_offset = second_member.member_offsets[i];
          coff_printf("[%4u] 0x%08x %S", i, member_offset, string_n->string);
        } else {
          coff_errorf("[%4u] Out of bounds symbol number %u", i, symbol_number);
          break;
        }
      }
    } else {
      coff_errorf("Symbol index count %u doesn't match symbol count %u",
                           second_member.symbol_index_count, second_member.symbol_count);
    }
    coff_unindent();

    coff_unindent();
    coff_newline();
  }

  if (archive_parse.has_long_names && opts & CoffdumpOption_LongNames) {
    coff_printf("# Long Names");
    coff_indent();

    String8List long_names = str8_split_by_string_chars(scratch.arena, archive_parse.long_names, str8_lit("\0"), 0);
    U64 name_idx = 0;
    for (String8Node *name_n = long_names.first; name_n != 0; name_n = name_n->next, ++name_idx) {
      U64 offset = (U64)(name_n->string.str - archive_parse.long_names.str);
      coff_printf("[%-4u] 0x%08x %S", name_idx, offset, name_n->string);
    }

    coff_unindent();
    coff_newline();
  }

  U64  member_offset_count = 0;
  U32 *member_offsets      = 0;
  if (archive_parse.has_second_header) {
    member_offset_count = archive_parse.second_member.member_offset_count;
    member_offsets      = archive_parse.second_member.member_offsets;
  } else {
    HashTable *ht = hash_table_init(scratch.arena, 0x1000);
    for (U64 i = 0; i < archive_parse.first_member.member_offset_count; ++i) {
      U32 member_offset = from_be_u32(archive_parse.first_member.member_offsets[i]);
      if (!hash_table_search_u32(ht, member_offset)) {
        hash_table_push_u32_raw(scratch.arena, ht, member_offset, 0);
      }
    }
    member_offset_count = ht->count;
    member_offsets      = keys_from_hash_table_u32(scratch.arena, ht);
    radsort(member_offsets, member_offset_count, u32_is_before);
  }

  coff_printf("# Members");
  coff_indent();

  for (U64 i = 0; i < member_offset_count; ++i) {
    U64                next_member_offset = i+1 < member_offset_count ? member_offsets[i+1] : raw_archive.size;
    U64                member_offset      = member_offsets[i];
    String8            raw_member         = str8_substr(raw_archive, rng_1u64(member_offset, next_member_offset));
    COFF_ArchiveMember member             = coff_archive_member_from_data(raw_member);
    COFF_DataType      member_type        = coff_data_type_from_data(member.data);

    coff_printf("Member @ %#llx", member_offset);
    coff_indent();

    if (opts & CoffdumpOption_Headers) {
      coff_format_archive_member_header(arena, out, indent, member.header, archive_parse.long_names);
      coff_newline();
    }

    switch (member_type) {
      case COFF_DataType_BIG_OBJ: {
        coff_format_big_obj(arena, out, indent, member.data, opts);
      } break;
      case COFF_DataType_OBJ: {
        coff_format_obj(arena, out, indent, member.data, opts);
      } break;
      case COFF_DataType_IMPORT: {
        if (opts & CoffdumpOption_Headers) {
          COFF_ImportHeader header = {0};
          U64 parse_size = coff_parse_archive_import(member.data, 0, &header);
          if (parse_size) {
            coff_format_import(arena, out, indent, &header);
          } else {
            coff_errorf("not enough bytes to parse import header");
          }
        }
      } break;
      case COFF_DataType_NULL: {
        coff_errorf("unknown member format", member_offset);
      } break;
    }

    coff_unindent();
    coff_newline();
  }

  coff_unindent();

  scratch_end(scratch);
}

////////////////////////////////
//~ MSVC CRT

internal String8
mscrt_format_eh_adjectives(Arena *arena, MSCRT_EhHandlerTypeFlags adjectives)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List adj_list = {0};
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsConst) {
    str8_list_pushf(scratch.arena, &adj_list, "Const");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsVolatile) {
    str8_list_pushf(scratch.arena, &adj_list, "Volatile");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsUnaligned) {
    str8_list_pushf(scratch.arena, &adj_list, "Unaligned");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsReference) {
    str8_list_pushf(scratch.arena, &adj_list, "Reference");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsResumable) {
    str8_list_pushf(scratch.arena, &adj_list, "Resumable");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsStdDotDot) {
    str8_list_pushf(scratch.arena, &adj_list, "StdDotDot");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsComplusEH) {
    str8_list_pushf(scratch.arena, &adj_list, "ComplusEH");
  }
  String8 result = str8_list_join(arena, &adj_list, &(StringJoin){.sep=str8_lit(", ")});
  scratch_end(scratch);
  return result;
}

internal void
mscrt_format_eh_handler_type32(Arena *arena, String8List *out, String8 indent, MSCRT_EhHandlerType32 *handler)
{
  String8 catch_line     = str8_zero(); // TODO: syms_line_for_voff(scratch.arena, group, handler->catch_handler_voff);
  String8 adjectives_str = mscrt_format_eh_adjectives(arena, handler->adjectives);
  coff_printf("Adjectives:                %S",     adjectives_str, handler->adjectives);
  coff_printf("Descriptor:                %#x",    handler->descriptor_voff);
  coff_printf("Catch Object Frame Offset: %#x",    handler->catch_obj_frame_offset);
  coff_printf("Catch Handler:             %#x %S", handler->catch_handler_voff, catch_line);
  coff_printf("Delta to FP Handler:       %#x",    handler->fp_distance);
}

////////////////////////////////
//~ PE

internal B32
is_pe(String8 raw_data)
{
  PE_DosHeader header = {0};
  str8_deserial_read_struct(raw_data, 0, &header);
  return header.magic == PE_DOS_MAGIC;
}

internal void
pe_format_data_directory_ranges(Arena *arena, String8List *out, String8 indent, U64 count, PE_DataDirectory *dirs)
{
  Temp scratch = scratch_begin(&arena, 1);
  coff_printf("# Data Directories");
  coff_indent();
  for (U64 i = 0; i < count; ++i) {
    String8 dir_name;
    if (i < PE_DataDirectoryIndex_COUNT) {
      dir_name = pe_string_from_data_directory_index(i);
    } else {
      dir_name = push_str8f(scratch.arena, "%#x", i);
    }
    coff_printf("%-16S [%08x-%08x)", dir_name, dirs[i].virt_off, dirs[i].virt_off+dirs[i].virt_size);
  }
  coff_unindent();
  scratch_end(scratch);
}

internal void
pe_format_optional_header32(Arena *arena, String8List *out, String8 indent, PE_OptionalHeader32 *opt_header, PE_DataDirectory *dirs)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 subsystem = pe_string_from_subsystem(opt_header->subsystem);
  String8 dll_chars = pe_string_from_dll_characteristics(scratch.arena, opt_header->dll_characteristics);

  coff_printf("# PE Optional Header 32");
  coff_indent();
  coff_printf("Magic:                 %#x",   opt_header->magic);
  coff_printf("Linker version:        %u.%u",  opt_header->major_linker_version, opt_header->minor_linker_version);
  coff_printf("Size of code:          %m",     opt_header->sizeof_code);
  coff_printf("Size of inited data:   %m",     opt_header->sizeof_inited_data);
  coff_printf("Size of uninited data: %m",     opt_header->sizeof_uninited_data);
  coff_printf("Entry point:           %#x",   opt_header->entry_point_va);
  coff_printf("Code base:             %#x",   opt_header->code_base);
  coff_printf("Data base:             %#x",   opt_header->data_base);
  coff_printf("Image base:            %#x",   opt_header->image_base);
  coff_printf("Section align:         %#x",   opt_header->section_alignment);
  coff_printf("File align:            %#x",   opt_header->file_alignment);
  coff_printf("OS version:            %u.%u",  opt_header->major_os_ver, opt_header->minor_os_ver);
  coff_printf("Image Version:         %u.%u",  opt_header->major_img_ver, opt_header->minor_img_ver);
  coff_printf("Subsystem version:     %u.%u",  opt_header->major_subsystem_ver, opt_header->minor_subsystem_ver);
  coff_printf("Win32 version:         %u",     opt_header->win32_version_value);
  coff_printf("Size of image:         %m",     opt_header->sizeof_image);
  coff_printf("Size of headers:       %m",     opt_header->sizeof_headers);
  coff_printf("Checksum:              %#x",   opt_header->check_sum);
  coff_printf("Subsystem:             %S",     subsystem);
  coff_printf("DLL Characteristics:   %S",     dll_chars);
  coff_printf("Stack reserve:         %m",     opt_header->sizeof_stack_reserve);
  coff_printf("Stack commit:          %m",     opt_header->sizeof_stack_commit);
  coff_printf("Heap reserve:          %m",     opt_header->sizeof_heap_reserve);
  coff_printf("Heap commit:           %m",     opt_header->sizeof_heap_commit);
  coff_printf("Loader flags:          %#x",   opt_header->loader_flags);
  coff_printf("RVA and offset count:  %u",     opt_header->data_dir_count);
  coff_newline();

  pe_format_data_directory_ranges(arena, out, indent, opt_header->data_dir_count, dirs);
  coff_newline();

  coff_unindent();
  scratch_end(scratch);
}

internal void
pe_format_optional_header32plus(Arena *arena, String8List *out, String8 indent, PE_OptionalHeader32Plus *opt_header, PE_DataDirectory *dirs)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 subsystem = pe_string_from_subsystem(opt_header->subsystem);
  String8 dll_chars = pe_string_from_dll_characteristics(scratch.arena, opt_header->dll_characteristics);

  coff_printf("# PE Optional Header 32+");
  coff_indent();
  coff_printf("Magic:                 %#x",   opt_header->magic);
  coff_printf("Linker version:        %u.%u",  opt_header->major_linker_version, opt_header->minor_linker_version);
  coff_printf("Size of code:          %m",     opt_header->sizeof_code);
  coff_printf("Size of inited data:   %m",     opt_header->sizeof_inited_data);
  coff_printf("Size of uninited data: %m",     opt_header->sizeof_uninited_data);
  coff_printf("Entry point:           %#x",   opt_header->entry_point_va);
  coff_printf("Code base:             %#x",   opt_header->code_base);
  coff_printf("Image base:            %#llx", opt_header->image_base);
  coff_printf("Section align:         %#x",   opt_header->section_alignment);
  coff_printf("File align:            %#x",   opt_header->file_alignment);
  coff_printf("OS version:            %u.%u",  opt_header->major_os_ver, opt_header->minor_os_ver);
  coff_printf("Image Version:         %u.%u",  opt_header->major_img_ver, opt_header->minor_img_ver);
  coff_printf("Subsystem version:     %u.%u",  opt_header->major_subsystem_ver, opt_header->minor_subsystem_ver);
  coff_printf("Win32 version:         %u",     opt_header->win32_version_value);
  coff_printf("Size of image:         %m",     opt_header->sizeof_image);
  coff_printf("Size of headers:       %m",     opt_header->sizeof_headers);
  coff_printf("Checksum:              %#x",   opt_header->check_sum);
  coff_printf("Subsystem:             %S",     subsystem);
  coff_printf("DLL Characteristics:   %S",     dll_chars);
  coff_printf("Stack reserve:         %M",     opt_header->sizeof_stack_reserve);
  coff_printf("Stack commit:          %M",     opt_header->sizeof_stack_commit);
  coff_printf("Heap reserve:          %M",     opt_header->sizeof_heap_reserve);
  coff_printf("Heap commit:           %M",     opt_header->sizeof_heap_commit);
  coff_printf("Loader flags:          %#x",   opt_header->loader_flags);
  coff_printf("RVA and offset count:  %u",     opt_header->data_dir_count);
  coff_newline();

  pe_format_data_directory_ranges(arena, out, indent, opt_header->data_dir_count, dirs);
  coff_newline();

  coff_unindent();
  scratch_end(scratch);
}

internal void
pe_format_load_config32(Arena *arena, String8List *out, String8 indent, PE_LoadConfig32 *lc)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 time_stamp        = coff_format_time_stamp(scratch.arena, lc->time_stamp);
  String8 global_flag_clear = pe_string_from_global_flags(scratch.arena, lc->global_flag_clear);
  String8 global_flag_set   = pe_string_from_global_flags(scratch.arena, lc->global_flag_set);

  coff_printf("# Load Config 32");
  coff_indent();

  coff_printf("Size:                          %m",       lc->size);
  coff_printf("Time stamp:                    %#x (%S)", lc->time_stamp, time_stamp);
  coff_printf("Version:                       %u.%u",    lc->major_version, lc->minor_version);
  coff_printf("Global flag clear:             %#x %S",   global_flag_clear);
  coff_printf("Global flag set:               %#x %S",   global_flag_set);
  coff_printf("Critical section timeout:      %u",       lc->critical_section_timeout);
  coff_printf("Decommit free block threshold: %#x",      lc->decommit_free_block_threshold);
  coff_printf("Decommit total free threshold: %#x",      lc->decommit_total_free_threshold);
  coff_printf("Lock prefix table:             %#x",      lc->lock_prefix_table);
  coff_printf("Maximum alloc size:            %m",       lc->maximum_allocation_size);
  coff_printf("Virtual memory threshold:      %m",       lc->virtual_memory_threshold);
  coff_printf("Process affinity mask:         %#x",      lc->process_affinity_mask);
  coff_printf("Process heap flags:            %#x",      lc->process_heap_flags);
  coff_printf("CSD version:                   %u",       lc->csd_version);
  coff_printf("Edit list:                     %#x",      lc->edit_list);
  coff_printf("Security Cookie:               %#x",      lc->security_cookie);
  if (lc->size < OffsetOf(PE_LoadConfig64, seh_handler_table)) {
    goto exit;
  }
  coff_newline();

  coff_printf("SEH Handler Table: %#x", lc->seh_handler_table);
  coff_printf("SEH Handler Count: %u",   lc->seh_handler_count);
  if (lc->size < OffsetOf(PE_LoadConfig64, guard_cf_check_func_ptr)) {
    goto exit;
  }
  coff_newline();

  coff_printf("Guard CF Check Function:    %#x", lc->guard_cf_check_func_ptr);
  coff_printf("Guard CF Dispatch Function: %#x", lc->guard_cf_dispatch_func_ptr);
  coff_printf("Guard CF Function Table:    %#x", lc->guard_cf_func_table);
  coff_printf("Guard CF Function Count:    %u",  lc->guard_cf_func_count);
  coff_printf("Guard Flags:                %#x", lc->guard_flags);
  if (lc->size < OffsetOf(PE_LoadConfig64, code_integrity)) {
    goto exit;
  }
  coff_newline();

  coff_printf("Code integrity:                        { Flags = %#x, Catalog = %#x, Catalog Offset = %#x }",
               lc->code_integrity.flags, lc->code_integrity.catalog, lc->code_integrity.catalog_offset);
  coff_printf("Guard address taken IAT entry table:   %#x", lc->guard_address_taken_iat_entry_table);
  coff_printf("Guard address taken IAT entry count:   %u",  lc->guard_address_taken_iat_entry_count);
  coff_printf("Guard long jump target table:          %#x", lc->guard_long_jump_target_table);
  coff_printf("Guard long jump target count:          %u",  lc->guard_long_jump_target_count);
  coff_printf("Dynamic value reloc table:             %#x", lc->dynamic_value_reloc_table);
  coff_printf("CHPE Metadata ptr:                     %#x", lc->chpe_metadata_ptr);
  coff_printf("Guard RF failure routine:              %#x", lc->guard_rf_failure_routine);
  coff_printf("Guard RF failure routine func ptr:     %#x", lc->guard_rf_failure_routine_func_ptr);
  coff_printf("Dynamic value reloc section:           %#x", lc->dynamic_value_reloc_table_section);
  coff_printf("Dynamic value reloc section offset:    %#x", lc->dynamic_value_reloc_table_offset);
  coff_printf("Guard RF verify SP func ptr:           %#x", lc->guard_rf_verify_stack_pointer_func_ptr);
  coff_printf("Hot patch table offset:                %#x", lc->hot_patch_table_offset);
  if (lc->size < OffsetOf(PE_LoadConfig64, enclave_config_ptr)) {
    goto exit;
  }
  coff_newline();

  coff_printf("Enclave config ptr:                    %#x", lc->enclave_config_ptr);
  coff_printf("Volatile metadata ptr:                 %#x", lc->volatile_metadata_ptr);
  coff_printf("Guard EH continuation table:           %#x", lc->guard_eh_continue_table);
  coff_printf("Guard EH continuation count:           %u",  lc->guard_eh_continue_count);
  coff_printf("Guard XFG check func ptr:              %#x", lc->guard_xfg_check_func_ptr);
  coff_printf("Guard XFG dispatch func ptr:           %#x", lc->guard_xfg_dispatch_func_ptr);
  coff_printf("Guard XFG table dispatch func ptr:     %#x", lc->guard_xfg_table_dispatch_func_ptr);
  coff_printf("Cast guard OS determined failure mode: %#x", lc->cast_guard_os_determined_failure_mode);
  coff_newline();

exit:;
  coff_unindent();
  scratch_end(scratch);
}

internal void
pe_format_load_config64(Arena *arena, String8List *out, String8 indent, PE_LoadConfig64 *lc)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 time_stamp        = coff_format_time_stamp(scratch.arena, lc->time_stamp);
  String8 global_flag_clear = pe_string_from_global_flags(scratch.arena, lc->global_flag_clear);
  String8 global_flag_set   = pe_string_from_global_flags(scratch.arena, lc->global_flag_set);

  coff_printf("# Load Config 64");
  coff_indent();

  coff_printf("Size:                          %m",       lc->size);
  coff_printf("Time stamp:                    %#x (%S)", lc->time_stamp, time_stamp);
  coff_printf("Version:                       %u.%u",    lc->major_version, lc->minor_version);
  coff_printf("Global flag clear:             %#x %S",   lc->global_flag_clear, global_flag_clear);
  coff_printf("Global flag set:               %#x %S",   lc->global_flag_set, global_flag_set);
  coff_printf("Critical section timeout:      %u",       lc->critical_section_timeout);
  coff_printf("Decommit free block threshold: %#llx",    lc->decommit_free_block_threshold);
  coff_printf("Decommit total free threshold: %#llx",    lc->decommit_total_free_threshold);
  coff_printf("Lock prefix table:             %#llx",    lc->lock_prefix_table);
  coff_printf("Maximum alloc size:            %M",       lc->maximum_allocation_size);
  coff_printf("Virtual memory threshold:      %M",       lc->virtual_memory_threshold);
  coff_printf("Process affinity mask:         %#x",      lc->process_affinity_mask);
  coff_printf("Process heap flags:            %#x",      lc->process_heap_flags);
  coff_printf("CSD version:                   %u",       lc->csd_version);
  coff_printf("Edit list:                     %#llx",    lc->edit_list);
  coff_printf("Security Cookie:               %#llx",    lc->security_cookie);
  if (lc->size < OffsetOf(PE_LoadConfig64, seh_handler_table)) {
    goto exit;
  }
  coff_newline();

  coff_printf("SEH Handler Table: %#llx", lc->seh_handler_table);
  coff_printf("SEH Handler Count: %llu",  lc->seh_handler_count);
  if (lc->size < OffsetOf(PE_LoadConfig64, guard_cf_check_func_ptr)) {
    goto exit;
  }
  coff_newline();

  coff_printf("Guard CF Check Function:    %#llx", lc->guard_cf_check_func_ptr);
  coff_printf("Guard CF Dispatch Function: %#llx", lc->guard_cf_dispatch_func_ptr);
  coff_printf("Guard CF Function Table:    %#llx", lc->guard_cf_func_table);
  coff_printf("Guard CF Function Count:    %llu",  lc->guard_cf_func_count);
  coff_printf("Guard Flags:                %#x",   lc->guard_flags);
  if (lc->size < OffsetOf(PE_LoadConfig64, code_integrity)) {
    goto exit;
  }
  coff_newline();

  coff_printf("Code integrity:                      { Flags = %#x, Catalog = %#x, Catalog Offset = %#x }",
               lc->code_integrity.flags, lc->code_integrity.catalog, lc->code_integrity.catalog_offset);
  coff_printf("Guard address taken IAT entry table: %#llx", lc->guard_address_taken_iat_entry_table);
  coff_printf("Guard address taken IAT entry count: %llu",  lc->guard_address_taken_iat_entry_count);
  coff_printf("Guard long jump target table:        %#llx", lc->guard_long_jump_target_table);
  coff_printf("Guard long jump target count:        %llu",  lc->guard_long_jump_target_count);
  coff_printf("Dynamic value reloc table:           %#llx", lc->dynamic_value_reloc_table);
  coff_printf("CHPE Metadata ptr:                   %#llx", lc->chpe_metadata_ptr);
  coff_printf("Guard RF failure routine:            %#llx", lc->guard_rf_failure_routine);
  coff_printf("Guard RF failure routine func ptr:   %#llx", lc->guard_rf_failure_routine_func_ptr);
  coff_printf("Dynamic value reloc section:         %#llx", lc->dynamic_value_reloc_table_section);
  coff_printf("Dynamic value reloc section offset:  %#llx", lc->dynamic_value_reloc_table_offset);
  coff_printf("Guard RF verify SP func ptr:         %#llx", lc->guard_rf_verify_stack_pointer_func_ptr);
  coff_printf("Hot patch table offset:              %#llx", lc->hot_patch_table_offset);
  if (lc->size < OffsetOf(PE_LoadConfig64, enclave_config_ptr)) {
    goto exit;
  }
  coff_newline();

  coff_printf("Enclave config ptr:                    %#llx", lc->enclave_config_ptr);
  coff_printf("Volatile metadata ptr:                 %#llx", lc->volatile_metadata_ptr);
  coff_printf("Guard EH continuation table:           %#llx", lc->guard_eh_continue_table);
  coff_printf("Guard EH continuation count:           %llu",  lc->guard_eh_continue_count);
  coff_printf("Guard XFG check func ptr:              %#llx", lc->guard_xfg_check_func_ptr);
  coff_printf("Guard XFG dispatch func ptr:           %#llx", lc->guard_xfg_dispatch_func_ptr);
  coff_printf("Guard XFG table dispatch func ptr:     %#llx", lc->guard_xfg_table_dispatch_func_ptr);
  coff_printf("Cast guard OS determined failure mode: %#llx", lc->cast_guard_os_determined_failure_mode);
  coff_newline();

exit:;
  coff_unindent();
  scratch_end(scratch);
}

internal void
pe_format_tls(Arena *arena, String8List *out, String8 indent, PE_ParsedTLS tls)
{
  Temp scratch = scratch_begin(&arena, 1);

  coff_printf("# TLS");
  coff_indent();

  String8 tls_chars = coff_string_from_section_flags(scratch.arena, tls.header.characteristics);
  coff_printf("Raw data start:    %#llx", tls.header.raw_data_start);
  coff_printf("Raw data end:      %#llx", tls.header.raw_data_end);
  coff_printf("Index address:     %#llx", tls.header.index_address);
  coff_printf("Callbacks address: %#llx", tls.header.callbacks_address);
  coff_printf("Zero-fill size:    %m",    tls.header.zero_fill_size);
  coff_printf("Characteristics:   %S",    tls_chars);

  if (tls.callback_count) {
    coff_newline();
    coff_printf("## Callbacks");
    coff_indent();
    for (U64 i = 0; i < tls.callback_count; ++i) {
      coff_printf("%#llx", tls.callback_addrs[i]);
    }
    coff_unindent();
  }

  coff_unindent();
  coff_newline();

  scratch_end(scratch);
}

internal void
pe_format_debug_directory(Arena *arena, String8List *out, String8 indent, String8 raw_data, String8 raw_dir)
{
  Temp scratch = scratch_begin(&arena, 1);

  coff_printf("# Debug");
  coff_indent();

  U64                entry_count = raw_dir.size / sizeof(PE_DebugDirectory);
  PE_DebugDirectory *entries      = str8_deserial_get_raw_ptr(raw_dir, 0, sizeof(*entries)*entry_count);
  for (U64 i = 0; i < entry_count; ++i) {
    PE_DebugDirectory *de = entries+i;
    coff_printf("Entry[%u]", i);
    coff_indent();

    {
      String8 time_stamp = coff_format_time_stamp(scratch.arena, de->time_stamp);
      String8 type       = pe_string_from_debug_directory_type(de->type);

      coff_printf("Characteristics: %#x",  de->characteristics);
      coff_printf("Time Stamp:      %S",    time_stamp);
      coff_printf("Version:         %u.%u", de->major_ver, de->minor_ver);
      coff_printf("Type:            %S",    type);
      coff_printf("Size:            %u",    de->size);
      coff_printf("Data virt off:   %#x",  de->voff);
      coff_printf("Data file off:   %#x",  de->foff);
      coff_newline();
    }

    String8 raw_entry = str8_substr(raw_data, rng_1u64(de->foff, de->foff+de->size));
    if (raw_entry.size != de->size) {
      coff_errorf("unable to read debug entry @ %#x", de->foff);
      break;
    }

    coff_indent();
    switch (de->type) {
      case PE_DebugDirectoryType_POGO:
      case PE_DebugDirectoryType_ILTCG:
      case PE_DebugDirectoryType_MPX:
      case PE_DebugDirectoryType_EXCEPTION:
      case PE_DebugDirectoryType_FIXUP:
      case PE_DebugDirectoryType_OMAP_TO_SRC:
      case PE_DebugDirectoryType_OMAP_FROM_SRC:
      case PE_DebugDirectoryType_BORLAND:
      case PE_DebugDirectoryType_CLSID:
      case PE_DebugDirectoryType_REPRO:
      case PE_DebugDirectoryType_VC_FEATURE:
      case PE_DebugDirectoryType_EX_DLLCHARACTERISTICS: {
        NotImplemented;
      } break;
      case PE_DebugDirectoryType_FPO: {
        PE_DebugFPO *fpo = str8_deserial_get_raw_ptr(raw_entry, 0, sizeof(PE_DebugFPO));

        U8          prolog_size     = PE_FPOEncoded_Extract_PROLOG_SIZE(fpo->flags);
        U8          saved_regs_size = PE_FPOEncoded_Extract_SAVED_REGS_SIZE(fpo->flags);
        PE_FPOType  type            = PE_FPOEncoded_Extract_FRAME_TYPE(fpo->flags);
        PE_FPOFlags flags           = PE_FPOEncoded_Extract_FLAGS(fpo->flags);

        String8 type_string  = pe_string_from_fpo_type(type);
        String8 flags_string = pe_string_from_fpo_flags(scratch.arena, flags);

        coff_printf("Function offset: %#x", fpo->func_code_off);
        coff_printf("Function size:   %#x", fpo->func_size);
        coff_printf("Locals size:     %u",   fpo->locals_size);
        coff_printf("Params size:     %u",   fpo->params_size);
        coff_printf("Prolog size:     %u",   prolog_size);
        coff_printf("Saved regs size: %u",   saved_regs_size);
        coff_printf("Type:            %S",   type_string);
        coff_printf("Flags:           %S",   flags_string);
      } break;

      case PE_DebugDirectoryType_CODEVIEW: {
        U32 magic = 0;
        str8_deserial_read_struct(raw_entry, 0, &magic);
        switch (magic) {
          case PE_CODEVIEW_PDB20_MAGIC: {
            PE_CvHeaderPDB20 *cv20 = str8_deserial_get_raw_ptr(raw_entry, 0, sizeof(*cv20));
            String8 name; str8_deserial_read_cstr(raw_entry, sizeof(*cv20), &name);

            String8 time_stamp = coff_format_time_stamp(scratch.arena, cv20->time_stamp);

            coff_printf("Time stamp: %S", time_stamp);
            coff_printf("Age:        %u", cv20->age);
            coff_printf("Name:       %S", name);
          } break;
          case PE_CODEVIEW_PDB70_MAGIC: {
            PE_CvHeaderPDB70 *cv70 = str8_deserial_get_raw_ptr(raw_entry, 0, sizeof(*cv70));
            String8 name; str8_deserial_read_cstr(raw_entry, sizeof(*cv70), &name);
            
            String8 guid = string_from_guid(scratch.arena, cv70->guid);

            coff_printf("GUID: %S", guid);
            coff_printf("Age:  %u", cv70->age);
            coff_printf("Name: %S", name);
          } break;
          default: {
            coff_errorf("unknown CodeView magic %#x", magic);
          } break;
        }
      } break;
      case PE_DebugDirectoryType_MISC: {
        PE_DebugMisc *misc = str8_deserial_get_raw_ptr(raw_entry, 0, sizeof(*misc));
        
        String8 type_string = pe_string_from_misc_type(misc->data_type);

        coff_printf("Data type: %S", type_string);
        coff_printf("Size:      %u", misc->size);
        coff_printf("Unicode:   %u", misc->unicode);

        switch (misc->data_type) {
          case PE_DebugMiscType_EXE_NAME: {
            String8 name;
            str8_deserial_read_cstr(raw_entry, sizeof(*misc), &name);
            coff_printf("Name: %S", name);
          } break;
          default: {
            coff_printf("???");
          } break;
        }
      } break;
    }
    coff_unindent();

    coff_unindent();
    coff_newline();
  }

  coff_unindent();
  coff_newline();
  scratch_end(scratch);
}

internal void
pe_format_export_table(Arena *arena, String8List *out, String8 indent, PE_ParsedExportTable exptab)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 time_stamp = coff_format_time_stamp(scratch.arena, exptab.time_stamp);

  coff_printf("# Export Table");
  coff_indent();

  coff_printf("Characteristics: %u",      exptab.flags);
  coff_printf("Time stamp:      %S",      time_stamp);
  coff_printf("Version:         %u.%02u", exptab.major_ver, exptab.minor_ver);
  coff_printf("Ordinal base:    %u",      exptab.ordinal_base);
  coff_printf("");

  coff_printf("%-4s %-8s %-8s %-8s", "No.", "Oridnal", "VOff", "Name");

  for (U64 i = 0; i < exptab.export_count; ++i) {
    PE_ParsedExport *exp = exptab.exports+i;
    if (exp->forwarder.size) {
      coff_printf("%4u %8u %8x %S (forwarded to %S)", i, exp->ordinal, exp->voff, exp->name, exp->forwarder);
    } else {
      coff_printf("%4u %8u %8x %S", i, exp->ordinal, exp->voff, exp->name);
    }
  }

  coff_unindent();
  scratch_end(scratch);
}

internal void
pe_format_static_import_table(Arena *arena, String8List *out, String8 indent, U64 image_base, PE_ParsedStaticImportTable imptab)
{
  Temp scratch = scratch_begin(&arena, 1);

  if (imptab.count) {
    coff_printf("# Import Table");
    coff_indent();
    for (U64 dll_idx = 0; dll_idx < imptab.count; ++dll_idx) {
      PE_ParsedStaticDLLImport *dll = imptab.v+dll_idx;

      coff_printf("Name:                 %S",    dll->name);
      coff_printf("Import address table: %#llx", image_base + dll->import_address_table_voff);
      coff_printf("Import name table:    %#llx", image_base + dll->import_name_table_voff);
      coff_printf("Time stamp:           %#x",   dll->time_stamp);
      coff_newline();

      if (dll->import_count) {
        coff_indent();
        for (U64 imp_idx = 0; imp_idx < dll->import_count; ++imp_idx) {
          PE_ParsedImport *imp = dll->imports+imp_idx;
          if (imp->type == PE_ParsedImport_Ordinal) {
            coff_printf("%#-6x", imp->u.ordinal);
          } else if (imp->type == PE_ParsedImport_Name) {
            coff_printf("%#-6x %S", imp->u.name.hint, imp->u.name.string);
          }
        }
        coff_unindent();
        coff_newline();
      }
    }
    coff_unindent();
  }

  scratch_end(scratch);
}

internal void
pe_format_delay_import_table(Arena *arena, String8List *out, String8 indent, U64 image_base, PE_ParsedDelayImportTable imptab)
{
  if (imptab.count) {
    Temp scratch = scratch_begin(&arena, 1);
    coff_printf("# Delay Import Table");
    coff_indent();

    for (U64 dll_idx = 0; dll_idx < imptab.count; ++dll_idx) {
      PE_ParsedDelayDLLImport *dll = imptab.v+dll_idx;

      coff_printf("Attributes:               %#08x",  dll->attributes);
      coff_printf("Name:                     %S",    dll->name);
      coff_printf("HMODULE address:          %#llx", image_base + dll->module_handle_voff);
      coff_printf("Import address table:     %#llx", image_base + dll->iat_voff);
      coff_printf("Import name table:        %#llx", image_base + dll->name_table_voff);
      coff_printf("Bound import name table:  %#llx", image_base + dll->bound_table_voff);
      coff_printf("Unload import name table: %#llx", image_base + dll->unload_table_voff);
      coff_printf("Time stamp:               %#x",   dll->time_stamp);
      coff_newline();

      coff_indent();
      for (U64 imp_idx = 0; imp_idx < dll->import_count; ++imp_idx) {
        PE_ParsedImport *imp = dll->imports+imp_idx;

        String8 bound = str8_lit("NULL");
        if (imp_idx < dll->bound_table_count) {
          U64 bound_addr = dll->bound_table[imp_idx];
          bound = push_str8f(scratch.arena, "%#llx", bound_addr);
        }

        String8 unload = str8_lit("NULL");
        if (imp_idx < dll->unload_table_count) {
          U64 unload_addr = dll->unload_table[imp_idx];
          unload = push_str8f(scratch.arena, "%#llx", unload_addr);
        }

        if (imp->type == PE_ParsedImport_Ordinal) {
          coff_printf("%-16S %-16S %#-4x", bound, unload, imp->u.ordinal);
        } else if (imp->type == PE_ParsedImport_Name) {
          coff_printf("%-16S %-16S %#-4x %S", bound, unload, imp->u.name.hint, imp->u.name.string);
        }
      }
      coff_unindent();

      coff_newline();
    }

    coff_unindent();
    scratch_end(scratch);
  }
}

internal void
pe_format_resources(Arena *arena, String8List *out, String8 indent, PE_ResourceDir *root)
{
  Temp scratch = scratch_begin(&arena, 1);

  // setup stack
  struct stack_s {
    struct stack_s  *next;
    B32              print_table;
    B32              is_named;
    PE_ResourceNode *curr_name_node;
    PE_ResourceNode *curr_id_node;
    U64              name_idx;
    U64              id_idx;
    U64              dir_idx;
    U64              dir_id;
    String8          dir_name;
    PE_ResourceDir  *table;
  } *stack = push_array(scratch.arena, struct stack_s, 1);
  stack->table          = root;
  stack->print_table    = 1;
  stack->is_named       = 1;
  stack->dir_name       = str8_lit("ROOT");
  stack->curr_name_node = root->named_list.first;
  stack->curr_id_node   = root->id_list.first;

  if (stack) {
    coff_printf("# Resources");
    coff_indent();

    // traverse resource tree
    while (stack) {
      if (stack->print_table) {
        stack->print_table = 0;
        
        if (stack->is_named) {
          coff_printf("[%u] %S { Time Stamp: %u, Version %u.%u Name Count: %u, ID Count %u, Characteristics: %u }", 
                      stack->dir_idx,
                      stack->dir_name,
                      stack->table->time_stamp, 
                      stack->table->major_version, stack->table->minor_version, 
                      stack->table->named_list.count, stack->table->id_list.count,
                      stack->table->characteristics);
        } else {
          B32 is_actually_leaf = stack->table->id_list.count == 1 && 
                                 stack->table->id_list.first->data.kind != PE_ResDataKind_DIR;
          if (is_actually_leaf) {
            coff_printf("[%u] %u { Time Stamp: %u, Version %u.%u Name Count: %u, ID Count %u, Characteristics: %u }", 
                        stack->dir_idx,
                        stack->dir_id,
                        stack->table->time_stamp, 
                        stack->table->major_version, stack->table->minor_version, 
                        stack->table->named_list.count, stack->table->id_list.count,
                        stack->table->characteristics);
          } else {
            String8 id_str = pe_resource_kind_to_string(stack->dir_id);
            coff_printf("[%u] %S { Time Stamp: %u, Version %u.%u Name Count: %u, ID Count %u, Characteristics: %u }", 
                        stack->dir_idx,
                        id_str,
                        stack->dir_id,
                        stack->table->time_stamp, 
                        stack->table->major_version, stack->table->minor_version, 
                        stack->table->named_list.count, stack->table->id_list.count,
                        stack->table->characteristics);
          }
        }
      }
      
      while (stack->curr_name_node) {
        PE_ResourceNode *named_node = stack->curr_name_node;
        stack->curr_name_node = stack->curr_name_node->next;
        U64 name_idx = stack->name_idx++;
        
        PE_Resource *res = &named_node->data;
        if (res->kind == PE_ResDataKind_DIR) {
          struct stack_s *frame = push_array(scratch.arena, struct stack_s, 1);
          frame->table          = res->u.dir;
          frame->print_table    = 1;
          frame->dir_idx        = stack->name_idx;
          frame->dir_name       = res->id.u.string;
          frame->is_named       = 1;
          frame->curr_name_node = frame->table->named_list.first;
          frame->curr_id_node   = frame->table->id_list.first;
          SLLStackPush(stack, frame);
          coff_indent();
          goto yield;
        } else if (res->kind == PE_ResDataKind_COFF_LEAF) {
          COFF_ResourceDataEntry *entry = &res->u.leaf;
          coff_printf("[%u] %S Data VOFF: %#08X, Data Size: %#08X, Code Page: %u", 
                      name_idx, res->id.u.string, entry->data_voff, entry->data_size, entry->code_page);
        } else {
          InvalidPath;
        }
      }
      
      while (stack->curr_id_node) {
        PE_ResourceNode *id_node = stack->curr_id_node;
        PE_Resource *res = &id_node->data;
        stack->curr_id_node = stack->curr_id_node->next;
        U64 id_idx = stack->id_idx++;
        
        if (res->kind == PE_ResDataKind_DIR) {
          struct stack_s *frame = push_array(scratch.arena, struct stack_s, 1);
          frame->table          = res->u.dir;
          frame->print_table    = 1;
          frame->dir_idx        = stack->table->named_list.count + id_idx;
          frame->dir_id         = res->id.u.number;
          frame->curr_name_node = frame->table->named_list.first;
          frame->curr_id_node   = frame->table->id_list.first;
          SLLStackPush(stack, frame);
          coff_indent();
          goto yield;
        } else if (res->kind == PE_ResDataKind_COFF_LEAF) {
          COFF_ResourceDataEntry *entry = &res->u.leaf;
          coff_printf("[%u] ID: %u Data VOFF: %#08X, Data Size: %#08X, Code Page: %u", id_idx, res->id.u.number, entry->data_voff, entry->data_size, entry->code_page);
        } else {
          InvalidPath;
        }
      }
      
      SLLStackPop(stack);
      coff_unindent();
      
      yield:;
    }

    coff_unindent();
  }

  coff_newline();

  scratch_end(scratch);
}

internal void
pe_format_exceptions_x8664(Arena              *arena,
                           String8List        *out,
                           String8             indent,
                           U64                 section_count,
                           COFF_SectionHeader *sections,
                           String8             raw_data,
                           Rng1U64             except_frange)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 raw_except = str8_substr(raw_data, except_frange);
  U64     count      = raw_except.size / sizeof(PE_IntelPdata);
  for (U64 i = 0; i < count; ++i) {
    Temp temp = temp_begin(scratch.arena);

    U64            pdata_offset = i * sizeof(PE_IntelPdata);
    PE_IntelPdata *pdata        = str8_deserial_get_raw_ptr(raw_except, pdata_offset, sizeof(*pdata));

    String8 pdata_name = str8_zero(); // TODO: syms_name_for_voff(arena, group, thunk_accel, thunk_table_set, pdata.voff_first);

    U64            unwind_info_offset = coff_foff_from_voff(sections, section_count, pdata->voff_unwind_info);
    PE_UnwindInfo *uwinfo             = str8_deserial_get_raw_ptr(raw_data, unwind_info_offset, sizeof(*uwinfo));

    U8 version        = PE_UNWIND_INFO_VERSION_FROM_HDR(uwinfo->header);
    U8 flags          = PE_UNWIND_INFO_FLAGS_FROM_HDR(uwinfo->header);
    U8 frame_register = PE_UNWIND_INFO_REG_FROM_FRAME(uwinfo->frame);
    U8 frame_offset   = PE_UNWIND_INFO_OFF_FROM_FRAME(uwinfo->frame);

    B32 is_chained       = (flags & PE_UnwindInfoFlag_CHAINED) != 0;
    B32 has_handler_data = !is_chained && (flags & (PE_UnwindInfoFlag_EHANDLER | PE_UnwindInfoFlag_UHANDLER)) != 0;

    String8 flags_str = str8_zero();
    {
      U64 f = flags;

      String8List flags_list = {0};
      if (f & PE_UnwindInfoFlag_EHANDLER) {
        f &= ~PE_UnwindInfoFlag_EHANDLER;
        str8_list_pushf(scratch.arena, &flags_list, "EHANDLER");
      }
      if (f & PE_UnwindInfoFlag_UHANDLER) {
        f &= ~PE_UnwindInfoFlag_UHANDLER;
        str8_list_pushf(scratch.arena, &flags_list, "UHANDLER");
      }
      if (f & PE_UnwindInfoFlag_CHAINED) {
        f &= ~PE_UnwindInfoFlag_CHAINED;
        str8_list_pushf(scratch.arena, &flags_list, "CHAINED");
      }
      if (f) {
        str8_list_pushf(scratch.arena, &flags_list, "%#llx", f);
      }
      if (flags_list.node_count == 0) {
        str8_list_pushf(scratch.arena, &flags_list, "%#llx", f);
      }
      flags_str = str8_list_join(scratch.arena, &flags_list, &(StringJoin){.sep=str8_lit(", ")});
    }

    U64            codes_offset = unwind_info_offset + sizeof(PE_UnwindInfo);
    PE_UnwindCode *code_ptr     = str8_deserial_get_raw_ptr(raw_data, codes_offset, sizeof(*code_ptr) * uwinfo->codes_num);
    PE_UnwindCode *code_opl     = code_ptr + uwinfo->codes_num;

    if (i > 0) {
      coff_newline();
    }
    coff_printf("%08x %08x %08x %08x%s%S",
                pdata_offset,
                pdata->voff_first,
                pdata->voff_one_past_last,
                pdata->voff_unwind_info,
                pdata_name.size ? " " : "", pdata_name);
    coff_printf("Version:     %u",  version);
    coff_printf("Flags:       %S",  flags_str);
    coff_printf("Prolog Size: %#x", uwinfo->prolog_size);
    coff_printf("Code Count:  %u",  uwinfo->codes_num);
    coff_printf("Frame:       %u",  uwinfo->frame);
    coff_printf("Codes:");
    coff_indent();
    for (; code_ptr < code_opl;) {
      Temp code_temp = temp_begin(scratch.arena);
      String8List code_list = {0};

      U8 operation_code = PE_UNWIND_OPCODE_FROM_FLAGS(code_ptr[0].flags);
      U8 operation_info = PE_UNWIND_INFO_FROM_FLAGS(code_ptr[0].flags);

      str8_list_pushf(code_temp.arena, &code_list, "%#04x:", code_ptr[0].off_in_prolog);
      switch (operation_code) {
        case PE_UnwindOpCode_PUSH_NONVOL: {
          String8 gpr = pe_string_from_unwind_gpr_x64(operation_info);
          str8_list_pushf(code_temp.arena, &code_list, "PUSH_NONVOL %S", gpr);
          code_ptr += 1;
        } break;
        case PE_UnwindOpCode_ALLOC_LARGE: {
          U64 size = 0;
          switch (operation_info) {
            case 0: { // 136B - 512K
              size = code_ptr[1].u16*8;
            } break;
            case 1: { // 512K - 4GB
              size = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
            } break;
            default: break;
          }
          str8_list_pushf(code_temp.arena, &code_list, "ALLOC_LARGE size=%#x", size);
          code_ptr += 2;
        } break;
        case PE_UnwindOpCode_ALLOC_SMALL: {
          U64 size = operation_info*8 + 8;
          str8_list_pushf(code_temp.arena, &code_list, "ALLOC_SMALL size=%#x", size);
          code_ptr += 1;
        } break;
        case PE_UnwindOpCode_SET_FPREG: {
          U64     offset = frame_offset*16;
          String8 gpr    = pe_string_from_unwind_gpr_x64(frame_register);
          str8_list_pushf(code_temp.arena, &code_list, "SET_FPREG %S, offset=%#x", gpr, offset);
          code_ptr += 1;
        } break;
        case PE_UnwindOpCode_SAVE_NONVOL: {
          String8 gpr             = pe_string_from_unwind_gpr_x64(operation_info);
          U64     register_offset = code_ptr[1].u16*8;
          str8_list_pushf(code_temp.arena, &code_list, "SAVE_NONVOL %S, offset=%#x", gpr, register_offset);
          code_ptr += 2;
        } break;
        case PE_UnwindOpCode_SAVE_NONVOL_FAR: {
          String8 gpr          = pe_string_from_unwind_gpr_x64(operation_info);
          U64     frame_offset = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
          str8_list_pushf(code_temp.arena, &code_list, "SAVE_NONVOL_FAR %S, offset=%#x", gpr, frame_offset);
          code_ptr += 3;
        } break;
        case PE_UnwindOpCode_EPILOG: {
          str8_list_pushf(code_temp.arena, &code_list, "EPILOG flags=%#x", code_ptr[0].flags);
          code_ptr += 1;
        } break;
        case PE_UnwindOpCode_SPARE_CODE: {
          str8_list_pushf(code_temp.arena, &code_list, "SPARE_CODE");
          code_ptr += 1;
        } break;
        case PE_UnwindOpCode_SAVE_XMM128: {
          String8 gpr             = pe_string_from_unwind_gpr_x64(operation_info);
          U64     register_offset = code_ptr[1].u16*16;
          str8_list_pushf(code_temp.arena, &code_list, "SAVE_XMM128 %S, offset=%#x", gpr, register_offset);
          code_ptr += 2;
        } break;
        case PE_UnwindOpCode_SAVE_XMM128_FAR: {
          String8 gpr          = pe_string_from_unwind_gpr_x64(operation_info);
          U64     frame_offset = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
          str8_list_pushf(code_temp.arena, &code_list, "SAVE_XMM128_FAR %S, offset=%#x", gpr, frame_offset);
          code_ptr += 3;
        } break;
        case PE_UnwindOpCode_PUSH_MACHFRAME: {
          str8_list_pushf(code_temp.arena, &code_list, "PUSH_MACHFRAME %s", operation_info == 1  ? "with error code" : "without error code");
          code_ptr += 1;
        } break;
        default: {
          str8_list_pushf(code_temp.arena, &code_list, "UNKNOWN_OPCODE %#x", operation_code);
          code_ptr += 1;
        } break;
      }

      String8 code_line = str8_list_join(code_temp.arena, &code_list, &(StringJoin){.sep=str8_lit(" ")});
      coff_printf("%S", code_line);

      temp_end(code_temp);
    }
    coff_unindent();

    if (is_chained) {
      U64            next_pdata_offset = codes_offset + sizeof(PE_UnwindCode) * AlignPow2(uwinfo->codes_num, 2);
      PE_IntelPdata *next_pdata        = str8_deserial_get_raw_ptr(raw_data, next_pdata_offset, sizeof(*next_pdata));
      coff_printf("Chained: %#08x %#08x %#08x", next_pdata->voff_first, next_pdata->voff_one_past_last, next_pdata->voff_unwind_info);
    }

    if (has_handler_data) {
#define ExceptionHandlerDataFlag_FuncInfo   (1 << 0)
#define ExceptionHandlerDataFlag_FuncInfo4  (1 << 1)
#define ExceptionHandlerDataFlag_ScopeTable (1 << 2)
#define ExceptionHandlerDataFlag_GS         (1 << 3u)

      U64 actual_code_count = PE_UNWIND_INFO_GET_CODE_COUNT(uwinfo->codes_num);
      U64 read_cursor       = codes_offset + actual_code_count * sizeof(PE_UnwindCode);

      U32 handler = 0; 
      read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &handler);

      String8 handler_name = str8_zero(); // TODO: syms_name_for_voff(scratch.arena, group, thunk_accel, thunk_table_set, handler);

      coff_printf("Handler: %#llx%s%S", handler, handler_name.size ? " " : "", handler_name);

      U32 handler_data_flags = 0;
      if (str8_match(handler_name, str8_lit("__GSHandlerCheck_EH4"), 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_FuncInfo4;
      } else if (str8_match(handler_name, str8_lit("__CxxFrameHandler4"), 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_FuncInfo4;
      } else if (str8_match(handler_name, str8_lit("__CxxFrameHandler3"), 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_FuncInfo;
      } else if (str8_match(handler_name, str8_lit("__C_specific_handler"), 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_ScopeTable;
      } else if (str8_match(handler_name, str8_lit("__GSHandlerCheck"), 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_GS;
      } else if (str8_match(handler_name, str8_lit("__GSHandlerCheck_SEH"), 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_ScopeTable|ExceptionHandlerDataFlag_GS;
      } else if (str8_match(handler_name, str8_lit("__GSHandlerCheck_EH"), 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_FuncInfo|ExceptionHandlerDataFlag_GS;
      }

      if (handler_data_flags & ExceptionHandlerDataFlag_FuncInfo) {
        MSCRT_FuncInfo func_info;
        read_cursor += mscrt_parse_func_info(arena, raw_data, section_count, sections, read_cursor, &func_info);

        coff_printf("Function Info:");
        coff_indent();
        coff_printf("Magic:                      %#x", func_info.magic);
        coff_printf("Max State:                  %u",  func_info.max_state);
        coff_printf("Try Block Count:            %u",  func_info.try_block_map_count);
        coff_printf("IP Map Count:               %u",  func_info.ip_map_count);
        coff_printf("Frame Offset Unwind Helper: %#x", func_info.frame_offset_unwind_helper);
        coff_printf("ES Flags:                   %#x", func_info.eh_flags);
        coff_unindent();

        if (func_info.ip_map_count > 0) {
          coff_printf("IP to State Map:");
          coff_indent();
          coff_printf("%8s %8s", "State", "IP");
          for (U32 i = 0; i < func_info.ip_map_count; ++i) {
            MSCRT_IPState32 state = func_info.ip_map[i];
            String8 line = str8_zero(); // TODO: syms_line_for_voff(temp.arena, group, state.ip);
            coff_printf("%8d %08x %S", state.state, state.ip, line);
          }
          coff_unindent();
        }

        if (func_info.max_state > 0) {
          coff_printf("Unwind Map:");
          coff_indent();
          coff_printf("%13s  %10s  %8s", "Current State", "Next State", "Action @");
          for (U32 i = 0; i < func_info.max_state; ++i) {
            MSCRT_UnwindMap32 map = func_info.unwind_map[i];
            String8 line = str8_zero(); // TODO: syms_line_for_voff(temp.arena, group, map.action_virt_off);
            coff_printf("%13u  %10d  %8x %S", i, map.next_state, map.action_virt_off, line);
          }
          coff_unindent();
        }

        for (U32 i = 0; i < func_info.try_block_map_count; ++i) {
          MSCRT_TryMapBlock try_block = func_info.try_block_map[i];
          coff_printf("Try Map Block #%u", i);
          coff_indent();
          coff_printf("Try State Low:    %u", try_block.try_low);
          coff_printf("Try State High:   %u", try_block.try_high);
          coff_printf("Catch State High: %u", try_block.catch_high);
          coff_printf("Catch Count:      %u", try_block.catch_handlers_count);
          coff_printf("Catches:");
          coff_indent();
          for (U32 ihandler = 0; ihandler < try_block.catch_handlers_count; ++ihandler) {
            coff_printf("Catch #%u", ihandler);
            coff_indent();
            mscrt_format_eh_handler_type32(arena, out, indent, &try_block.catch_handlers[ihandler]);
            coff_unindent();
          }
          coff_unindent();
          coff_unindent();
        }

        if (func_info.es_type_list.count) {
          coff_printf("Exception Specific Types:");
          coff_indent();
          for (U32 i = 0; i < func_info.es_type_list.count; ++i) {
            if (i > 0) {
              coff_newline();
            }
            mscrt_format_eh_handler_type32(arena, out, indent, &func_info.es_type_list.handlers[i]);
          }
          coff_unindent();
        }
      }
      if (handler_data_flags & ExceptionHandlerDataFlag_FuncInfo4) {
        U32 handler_data_voff = 0;
        read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &handler_data_voff);

        U32 unknown = 0;
        read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &unknown);

        U64                    func_info_foff = coff_foff_from_voff(sections, section_count, handler_data_voff);
        MSCRT_ParsedFuncInfoV4 func_info      = {0};
        mscrt_parse_func_info_v4(arena, raw_data, section_count, sections, func_info_foff, pdata->voff_first, &func_info);

        String8 header_str = str8_zero();
        {
          String8List header_list = {0};
          if (func_info.header & MSCRT_FuncInfoV4Flag_IsCatch) {
            str8_list_pushf(arena, &header_list, "IsCatch");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_IsSeparated) {
            str8_list_pushf(arena, &header_list, "IsSeparted");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_IsBBT) {
            str8_list_pushf(arena, &header_list, "IsBBT");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_UnwindMap) {
            str8_list_pushf(arena, &header_list, "UnwindMap");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_TryBlockMap) {
            str8_list_pushf(arena, &header_list, "TryBlockMap");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_EHs) {
            str8_list_pushf(arena, &header_list, "EHs");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_NoExcept) {
            str8_list_pushf(arena, &header_list, "NoExcept");
          }
          header_str = str8_list_join(arena, &header_list, &(StringJoin){.sep=str8_lit(", ")});
        }

        coff_printf("Function Info V4:");
        coff_indent();
        coff_printf("Header:                %#x %S", func_info.header, header_str);
        coff_printf("BBT Flags:             %#x",    func_info.bbt_flags);

        MSCRT_IP2State32V4 ip2state_map = func_info.ip2state_map;
        coff_printf("IP To State Map:");
        coff_indent();
        coff_printf("%8s %8s", "State", "IP");
        for (U32 i = 0; i < ip2state_map.count; ++i) {
          String8 line_str = str8_zero(); // TODO: syms_line_for_voff(arena, group, ip2state_map.voffs[i]);
          coff_printf("%8d %08X %S", ip2state_map.states[i], ip2state_map.voffs[i], line_str);
        }
        coff_unindent();

        if (func_info.header & MSCRT_FuncInfoV4Flag_UnwindMap) {
          MSCRT_UnwindMapV4 unwind_map = func_info.unwind_map;
          coff_printf("Unwind Map:");
          coff_indent();
          for (U32 i = 0; i < unwind_map.count; ++i) {
            MSCRT_UnwindEntryV4 *ue       = &unwind_map.v[i];
            String8                 type_str = str8_zero();
            switch (ue->type) {
              case MSCRT_UnwindMapV4Type_NoUW:             type_str = str8_lit("NoUW");             break;
              case MSCRT_UnwindMapV4Type_DtorWithObj:      type_str = str8_lit("DtorWithObj");      break;
              case MSCRT_UnwindMapV4Type_DtorWithPtrToObj: type_str = str8_lit("DtorWithPtrToObj"); break;
              case MSCRT_UnwindMapV4Type_VOFF:             type_str = str8_lit("VOFF");             break;
            }
            if (ue->type == MSCRT_UnwindMapV4Type_DtorWithObj || ue->type == MSCRT_UnwindMapV4Type_DtorWithPtrToObj) {
              coff_printf("[%2u] NextOff=%u Type=%-16S Action=%#08x Object=%#x", i, ue->next_off, type_str, ue->action, ue->object);
            } else if (ue->type == MSCRT_UnwindMapV4Type_VOFF) {
              coff_printf("[%2u] NextOff=%u Type=%-16S Action=%#08x", i, ue->next_off, type_str, ue->action);
            } else {
              coff_printf("[%2u] NextOff=%u Type=%S", i, ue->next_off, type_str);
            }
          }
          coff_unindent();
        }

        if (func_info.header & MSCRT_FuncInfoV4Flag_TryBlockMap) {
          MSCRT_TryBlockMapV4Array try_block_map = func_info.try_block_map;
          coff_printf("Try/Catch Blocks:");
          coff_indent();
          for (U32 i = 0; i < try_block_map.count; ++i) {
            MSCRT_TryBlockMapV4 *try_block = &try_block_map.v[i];
            coff_printf("[%2u] TryLow %u TryHigh %u CatchHigh %u", i, try_block->try_low, try_block->try_high, try_block->catch_high);
            if (try_block->handlers.count) {
              for (U32 k = 0; k < try_block->handlers.count; ++k) {
                MSCRT_EhHandlerTypeV4 *handler = &try_block->handlers.v[k];

                String8List line_list = {0};
                str8_list_pushf(arena, &line_list, "  ");
                str8_list_pushf(arena, &line_list, "CatchCodeVOff=0x%08X", handler->catch_code_voff);
                if (handler->flags & MSCRT_EhHandlerV4Flag_Adjectives) {
                  String8 adjectives = mscrt_format_eh_adjectives(arena, handler->adjectives);
                  str8_list_pushf(arena, &line_list, "Adjectives=%S", adjectives);
                }
                if (handler->flags & MSCRT_EhHandlerV4Flag_DispType) {
                  str8_list_pushf(arena, &line_list, "TypeVOff=%#x", handler->type_voff);
                }
                if (handler->flags & MSCRT_EhHandlerV4Flag_DispCatchObj) {
                  str8_list_pushf(arena, &line_list, "CacthObjVOff=%#x", handler->catch_obj_voff);
                }
                if (handler->flags & MSCRT_EhHandlerV4Flag_ContIsVOff) {
                  str8_list_pushf(arena, &line_list, "ContIsVOff");
                }
                for (U32 icont = 0; icont < handler->catch_funclet_cont_addr_count; ++icont) {
                  str8_list_pushf(arena, &line_list, "ContAddr[%u]=%#llx", icont, handler->catch_funclet_cont_addr[icont]);
                }

                String8 handler_str = str8_list_join(arena, &line_list, &(StringJoin){.sep=str8_lit(" ")});
                coff_printf("%S", handler_str);
              }
            }
          }
          coff_unindent();
        }
      }
      if (handler_data_flags & ExceptionHandlerDataFlag_ScopeTable) {
        U32 scope_count = 0;
        read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &scope_count);

        PE_HandlerScope *scopes = str8_deserial_get_raw_ptr(raw_data, read_cursor, sizeof(PE_HandlerScope)*scope_count); 
        read_cursor += scope_count*sizeof(scopes[0]);

        coff_printf("Count of scope table entries: %u", scope_count);
        coff_indent();
        coff_printf("%-8s %-8s %-8s %-8s", "Begin", "End", "Handler", "Target");
        for (U32 i = 0; i < scope_count; ++i) {
          PE_HandlerScope scope = scopes[i];
          coff_printf("%08x %08x %08x %08x", scope.begin, scope.end, scope.handler, scope.target);
        }
        coff_unindent();
      }
      if (handler_data_flags & ExceptionHandlerDataFlag_GS) {
        U32 gs_data = 0;
        read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &gs_data);

        U32 flags               = MSCRT_GSHandler_GetFlags(gs_data);
        U32 cookie_offset       = MSCRT_GSHandler_GetCookieOffset(gs_data);
        U32 aligned_base_offset = 0;
        U32 alignment           = 0;
        if (flags & MSCRT_GSHandlerFlag_HasAlignment) {
          read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &aligned_base_offset);
          read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &alignment);
        }

        String8 flags_str;
        {
          String8List flags_list = {0};
          if (flags & MSCRT_GSHandlerFlag_EHandler) {
            str8_list_pushf(arena, &flags_list, "EHandler");
          }
          if (flags & MSCRT_GSHandlerFlag_UHandler) {
            str8_list_pushf(arena, &flags_list, "UHandler");
          }
          if (flags & MSCRT_GSHandlerFlag_HasAlignment) {
            str8_list_pushf(arena, &flags_list, "Has Alignment");
          }
          if (flags == 0) {
            str8_list_pushf(arena, &flags_list, "None");
          }
          flags_str = str8_list_join(arena, &flags_list, &(StringJoin){.sep=str8_lit(", ")});
        }
        coff_printf("GS unwind flags:     %S", flags_str);
        coff_printf("Cookie offset:       %x", cookie_offset);
        if (flags & MSCRT_GSHandlerFlag_HasAlignment) {
          coff_printf("Aligned base offset: %x", aligned_base_offset);
          coff_printf("Alignment:           %x", alignment);
        }
      }

      #undef ExceptionHandlerDataFlag_FuncInfo
      #undef ExceptionHandlerDataFlag_ScopeTable
      #undef ExceptionHandlerDataFlag_GS
    }

    temp_end(temp);
  } 

  scratch_end(scratch);
}

internal void
pe_format_exceptions(Arena              *arena,
                     String8List        *out,
                     String8             indent,
                     COFF_MachineType    machine,
                     U64                 section_count,
                     COFF_SectionHeader *sections,
                     String8             raw_data,
                     Rng1U64             except_frange)
{
  coff_printf("# Exceptions");
  coff_indent();
  coff_printf("%-8s %-8s %-8s %-8s", "Offset", "Begin", "End", "Unwind Info");

  switch (machine) {
    case COFF_MachineType_UNKNOWN: break;
    case COFF_MachineType_X64:
    case COFF_MachineType_X86: {
      pe_format_exceptions_x8664(arena, out, indent, section_count, sections, raw_data, except_frange);
    } break;
    default: NotImplemented; break;
  }
  coff_unindent();
}

internal void
pe_format_base_relocs(Arena              *arena,
                      String8List        *out,
                      String8             indent,
                      COFF_MachineType    machine,
                      U64                 image_base,
                      U64                 section_count,
                      COFF_SectionHeader *sections,
                      String8             raw_data,
                      Rng1U64             base_reloc_franges)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8               raw_base_relocs = str8_substr(raw_data, base_reloc_franges);
  PE_BaseRelocBlockList base_relocs     = pe_base_reloc_block_list_from_data(scratch.arena, raw_base_relocs);

  if (base_relocs.count) {
    coff_printf("# Base Relocs");
    coff_indent();

    U32 addr_size = 0;
    switch (machine) {
      case COFF_MachineType_UNKNOWN: break;
      case COFF_MachineType_X86:     addr_size = 4; break;
      case COFF_MachineType_X64:     addr_size = 8; break;
      default: NotImplemented;
    }
   
    // convert blocks to string list
    U64 iblock = 0;
    for (PE_BaseRelocBlockNode *node = base_relocs.first; node != 0; node = node->next) {
      PE_BaseRelocBlock *block = &node->v;
      coff_printf("Block No. %u, Virt Off %#x, Reloc Count %u", iblock++, block->page_virt_off, block->entry_count);
      coff_indent();
      for (U64 ientry = 0; ientry < block->entry_count; ++ientry) {
        PE_BaseRelocKind type   = PE_BaseRelocKindFromEntry(block->entries[ientry]);
        U16              offset = PE_BaseRelocOffsetFromEntry(block->entries[ientry]);
        
        U64 apply_to_voff = block->page_virt_off + offset;
        U64 apply_to_foff = coff_foff_from_voff(sections, section_count, apply_to_voff);
        U64 apply_to      = 0;
        str8_deserial_read(raw_data, apply_to_foff, &apply_to, addr_size, 1);
        U64 addr = image_base + apply_to;
        
        const char *type_str = "???";
        switch (type) {
          case PE_BaseRelocKind_ABSOLUTE: type_str = "ABS";     break;
          case PE_BaseRelocKind_HIGH:     type_str = "HIGH";    break;
          case PE_BaseRelocKind_LOW:      type_str = "LOW";     break;
          case PE_BaseRelocKind_HIGHLOW:  type_str = "HIGHLOW"; break;
          case PE_BaseRelocKind_HIGHADJ:  type_str = "HIGHADJ"; break;
          case PE_BaseRelocKind_DIR64:    type_str = "DIR64";   break;
          default: {
            switch (machine) {
              case COFF_MachineType_ARM:
              case COFF_MachineType_ARM64:
              case COFF_MachineType_ARMNT: {
                switch (type) {
                  case PE_BaseRelocKind_ARM_MOV32:   type_str = "ARM_MOV32";   break;
                  case PE_BaseRelocKind_THUMB_MOV32: type_str = "THUMB_MOV32"; break;
                  default: NotImplemented;
                }
              } break;
              // TODO: mips, loong, risc-v
            }
          } break;
        }
        
        if (type == PE_BaseRelocKind_ABSOLUTE) {
          coff_printf("%-4x %-12s", offset, type_str);
        } else {
          coff_printf("%-4x %-12s %016llx", offset, type_str, apply_to);

          // TODO
          #if 0
          U64 reloc_voff = apply_to - image_base;
          SYMS_UnitID   uid = syms_group_uid_from_voff__accelerated(group, reloc_voff);
          SYMS_SymbolID sid = syms_group_proc_sid_from_uid_voff__accelerated(group, uid, reloc_voff);
          if (sid) {
            SYMS_UnitAccel *unit = syms_group_unit_from_uid(group, uid);
            SYMS_String8 name = syms_group_symbol_name_from_sid(arena, group, unit, sid);
            syms_string_list_pushf__dev(arena, list, "  %-4X %-12s %016llX %.*s", offset, type_str, apply_to, syms_expand_string(name));
          } else {
            syms_string_list_pushf__dev(arena, list, "  %-4X %-12s %016llX", offset, type_str, apply_to);
          }
          #endif
        }
      }
      coff_unindent();
      coff_newline();
    }

    coff_unindent();
  }

  scratch_end(scratch);
}

internal void
pe_format(Arena *arena, String8List *out, String8 indent, String8 raw_data, CoffdumpOption opts)
{
  Temp scratch = scratch_begin(&arena, 1);

  PE_DosHeader *dos_header = str8_deserial_get_raw_ptr(raw_data, 0, sizeof(*dos_header));
  if (!dos_header) {
    coff_errorf("not enough bytes to read DOS header");
    goto exit;
  }
  Assert(dos_header->magic == PE_DOS_MAGIC);

  U32 pe_magic = 0;
  str8_deserial_read_struct(raw_data, dos_header->coff_file_offset, &pe_magic);
  if (pe_magic != PE_MAGIC) {
    coff_errorf("PE magic check failure, input file is not of PE format");
    goto exit;
  }

  U64          coff_header_off = dos_header->coff_file_offset+sizeof(pe_magic);
  COFF_Header *coff_header     = str8_deserial_get_raw_ptr(raw_data, coff_header_off, sizeof(*coff_header));
  if (!coff_header) {
    coff_errorf("not enough bytes to read COFF header");
    goto exit;
  }

  U64 opt_header_off   = coff_header_off + sizeof(*coff_header);
  U16 opt_header_magic = 0;
  str8_deserial_read_struct(raw_data, opt_header_off, &opt_header_magic);
  if (opt_header_magic != PE_PE32_MAGIC && opt_header_magic != PE_PE32PLUS_MAGIC) {
    coff_errorf("unexpected optional header magic %#x", opt_header_magic);
    goto exit;
  }

  if (opt_header_magic == PE_PE32_MAGIC && coff_header->optional_header_size < sizeof(PE_OptionalHeader32)) {
    coff_errorf("unexpected optional header size in COFF header %m, expected at least %m", coff_header->optional_header_size, sizeof(PE_OptionalHeader32));
    goto exit;
  }

  if (opt_header_magic == PE_PE32PLUS_MAGIC && coff_header->optional_header_size < sizeof(PE_OptionalHeader32Plus)) {
    coff_errorf("unexpected optional header size %m, expected at least %m", coff_header->optional_header_size, sizeof(PE_OptionalHeader32Plus));
    goto exit;
  }

  U64                 sections_off = coff_header_off + sizeof(*coff_header) + coff_header->optional_header_size;
  COFF_SectionHeader *sections     = str8_deserial_get_raw_ptr(raw_data, sections_off, sizeof(*sections)*coff_header->section_count);
  if (!sections) {
    coff_errorf("not enough bytes to read COFF section headers");
    goto exit;
  }

  U64 string_table_off = coff_header->symbol_table_foff + sizeof(COFF_Symbol16) * coff_header->symbol_count;

  COFF_Symbol32Array symbols = coff_symbol_array_from_data_16(scratch.arena, raw_data, coff_header->symbol_table_foff, coff_header->symbol_count);

  U8 *raw_opt_header = push_array(scratch.arena, U8, coff_header->optional_header_size);
  str8_deserial_read_array(raw_data, opt_header_off, raw_opt_header, coff_header->optional_header_size);

  if (opts & CoffdumpOption_Headers) {
    coff_format_header(arena, out, indent, coff_header);
    coff_newline();
  }
  
  U64               image_base = 0;
  U64               dir_count  = 0;
  PE_DataDirectory *dirs       = 0;

  if (opt_header_magic == PE_PE32_MAGIC) {
    PE_OptionalHeader32 *opt_header = (PE_OptionalHeader32 *)raw_opt_header;
    image_base = opt_header->image_base;
    dir_count  = opt_header->data_dir_count;
    dirs       = str8_deserial_get_raw_ptr(raw_data, opt_header_off+sizeof(*opt_header), sizeof(*dirs) * opt_header->data_dir_count);
    if (!dirs) {
      coff_errorf("unable to read data directories");
      goto exit;
    }

    if (opts & CoffdumpOption_Headers) {
      pe_format_optional_header32(arena, out, indent, opt_header, dirs);
    }
  } else if (opt_header_magic == PE_PE32PLUS_MAGIC) {
    PE_OptionalHeader32Plus *opt_header = (PE_OptionalHeader32Plus *)raw_opt_header;
    image_base = opt_header->image_base;
    dir_count  = opt_header->data_dir_count;
    dirs       = str8_deserial_get_raw_ptr(raw_data, opt_header_off+sizeof(*opt_header), sizeof(*dirs) * opt_header->data_dir_count);
    if (!dirs) {
      coff_errorf("unable to read data directories");
      goto exit;
    }

    if (opts & CoffdumpOption_Headers) {
      pe_format_optional_header32plus(arena, out, indent, opt_header, dirs);
    }
  }

  // map data directory RVA to file offsets
  Rng1U64 *dirs_file_ranges = push_array(scratch.arena, Rng1U64, dir_count);
  Rng1U64 *dirs_virt_ranges = push_array(scratch.arena, Rng1U64, dir_count);
  for (U64 i = 0; i < dir_count; ++i) {
    PE_DataDirectory dir = dirs[i];
    U64 file_off = coff_foff_from_voff(sections, coff_header->section_count, dir.virt_off);
    dirs_file_ranges[i] = r1u64(file_off, file_off+dir.virt_size);
    dirs_virt_ranges[i] = r1u64(dir.virt_off, dir.virt_off+dir.virt_size);
  }

  if (opts & CoffdumpOption_Sections) {
    coff_format_section_table(arena, out, indent, raw_data, string_table_off, symbols, coff_header->section_count, sections);
  }

  if (opts & CoffdumpOption_Relocs) {
    coff_format_relocs(arena, out, indent, raw_data, string_table_off, coff_header->machine, coff_header->section_count, sections, symbols);
  }

  if (opts & CoffdumpOption_Symbols) {
    coff_format_symbol_table(arena, out, indent, raw_data, 0, string_table_off, symbols);
  }

  if (opts & CoffdumpOption_Exports) {
    PE_ParsedExportTable exptab = pe_exports_from_data(arena,
                                                       coff_header->section_count,
                                                       sections,
                                                       raw_data,
                                                       dirs_file_ranges[PE_DataDirectoryIndex_EXPORT],
                                                       dirs_virt_ranges[PE_DataDirectoryIndex_EXPORT]);
  }

  if (opts & CoffdumpOption_Imports) {
    B32                        is_pe32       = opt_header_magic == PE_PE32_MAGIC;
    PE_ParsedStaticImportTable static_imptab = pe_static_imports_from_data(arena, is_pe32, coff_header->section_count, sections, raw_data, dirs_file_ranges[PE_DataDirectoryIndex_IMPORT]);
    PE_ParsedDelayImportTable  delay_imptab  = pe_delay_imports_from_data(arena, is_pe32, coff_header->section_count, sections, raw_data, dirs_file_ranges[PE_DataDirectoryIndex_DELAY_IMPORT]);
    pe_format_static_import_table(arena, out, indent, image_base, static_imptab);
    pe_format_delay_import_table(arena, out, indent, image_base, delay_imptab);
  }

  if (opts & CoffdumpOption_Resources) {
    String8         raw_dir  = str8_substr(raw_data, dirs_file_ranges[PE_DataDirectoryIndex_RESOURCES]);
    PE_ResourceDir *dir_root = pe_resource_table_from_directory_data(scratch.arena, raw_dir);
    pe_format_resources(arena, out, indent, dir_root);
  }

  if (opts & CoffdumpOption_Exceptions) {
    pe_format_exceptions(arena, out, indent, coff_header->machine, coff_header->section_count, sections, raw_data, dirs_file_ranges[PE_DataDirectoryIndex_EXCEPTIONS]);
  }

  if (opts & CoffdumpOption_Relocs) {
    pe_format_base_relocs(arena, out, indent, coff_header->machine, image_base, coff_header->section_count, sections, raw_data, dirs_file_ranges[PE_DataDirectoryIndex_BASE_RELOC]);
  }

  if (opts & CoffdumpOption_Debug) {
    if (PE_DataDirectoryIndex_DEBUG < dir_count) {
      String8 raw_dir = str8_substr(raw_data, dirs_file_ranges[PE_DataDirectoryIndex_DEBUG]);
      pe_format_debug_directory(arena, out, indent, raw_data, raw_dir);
    }
  }

  if (opts & CoffdumpOption_Tls) {
    if (dim_1u64(dirs_file_ranges[PE_DataDirectoryIndex_TLS])) {
      PE_ParsedTLS tls = pe_tls_from_data(scratch.arena, coff_header->machine, image_base, coff_header->section_count, sections, raw_data, dirs_file_ranges[PE_DataDirectoryIndex_TLS]);
      pe_format_tls(arena, out, indent, tls);
    }
  }

  if (opts & CoffdumpOption_LoadConfig) {
    String8 raw_lc = str8_substr(raw_data, dirs_file_ranges[PE_DataDirectoryIndex_LOAD_CONFIG]);
    if (raw_lc.size) {
      switch (coff_header->machine) {
        case COFF_MachineType_UNKNOWN: break;
        case COFF_MachineType_X86: {
          PE_LoadConfig32 *lc = str8_deserial_get_raw_ptr(raw_lc, 0, sizeof(*lc));
          if (lc) {
            pe_format_load_config32(arena, out, indent, lc);
          } else {
            coff_errorf("not enough bytes to parse 32bit load config");
          }
        } break;
        case COFF_MachineType_X64: {
          PE_LoadConfig64 *lc = str8_deserial_get_raw_ptr(raw_lc, 0, sizeof(*lc));
          if (lc) {
            pe_format_load_config64(arena, out, indent, lc);
          } else {
            coff_errorf("not enough bytes to parse 64bit load config");
          }
        } break;
        default: NotImplemented;
      }
    }
  }

exit:;
  scratch_end(scratch);
}

////////////////////////////////

internal void
entry_point(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0,0);

  // parse options
  CoffdumpOption opts = 0;
  {
    for (U64 opt_idx = 0; opt_idx < ArrayCount(g_coffdump_option_map); ++opt_idx) {
      String8 opt_name = str8_cstring(g_coffdump_option_map[opt_idx].name);
      if (cmd_line_has_flag(cmdline, opt_name)) {
        opts |= g_coffdump_option_map[opt_idx].opt;
      }
    }
    if (cmd_line_has_flag(cmdline, str8_lit("all"))) {
      opts = ~0ull;
      opts &= ~(CoffdumpOption_Help|CoffdumpOption_Version);
    }
  }

  // print help
  if (opts & CoffdumpOption_Help) {
    for (U64 opt_idx = 0; opt_idx < ArrayCount(g_coffdump_option_map); ++opt_idx) {
      fprintf(stdout, "-%s %s\n", g_coffdump_option_map[opt_idx].name, g_coffdump_option_map[opt_idx].help);
    }
    os_abort(0);
  }

  // print version
  if (opts & CoffdumpOption_Version) {
    fprintf(stdout, BUILD_TITLE_STRING_LITERAL "\n");
    fprintf(stdout, "\tCOFFDUMP <OPTIONS> <INPUTS>\n");
    os_abort(0);
  }

  // input check
  if (cmdline->inputs.node_count == 0) {
    fprintf(stderr, "No input file specified\n");
    os_abort(1);
  } else if (cmdline->inputs.node_count > 1) {
    fprintf(stderr, "Too many inputs specified, expected one\n");
    os_abort(1);
  }

  // read input
  String8 file_path = str8_list_first(&cmdline->inputs);
  String8 raw_data  = os_data_from_file_path(scratch.arena, file_path);

  // is read ok?
  if (raw_data.size == 0) {
    fprintf(stderr, "Unable to read input file \"%.*s\"\n", str8_varg(file_path));
    os_abort(1);
  }

  // format input
  String8List out = {0};
  {
    String8 indent = str8_lit("                                                                   ");
    indent.size    = 0;
    if (coff_is_archive(raw_data) || coff_is_thin_archive(raw_data)) {
      coff_format_archive(scratch.arena, &out, indent, raw_data, opts);
    } else if (coff_is_big_obj(raw_data)) {
      coff_format_big_obj(scratch.arena, &out, indent, raw_data, opts);
    } else if (coff_is_obj(raw_data)) {
      coff_format_obj(scratch.arena, &out, indent, raw_data, opts);
    } else if (is_pe(raw_data)) {
      pe_format(scratch.arena, &out, indent, raw_data, opts);
    } else if (pe_is_res(raw_data)) {
      //tool_out_coff_res(stdout, file_data);
    }
  }
  
  // print formatted string
  String8 out_string = str8_list_join(scratch.arena, &out, &(StringJoin){ .sep = str8_lit("\n"), .post = str8_lit("\n") });
  fprintf(stdout, "%.*s", str8_varg(out_string));

  scratch_end(scratch);
}

