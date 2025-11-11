// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#if 0
internal void
coff_print_archive_member_header(Arena *arena, String8List *out, String8 indent, COFF_ParsedArchiveMemberHeader header, String8 long_names)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 time_stamp = coff_string_from_time_stamp(scratch.arena, header.time_stamp);
  
  rd_printf("Name      : %S"             , header.name    );
  rd_printf("Time Stamp: (%#x) %S"       , header.time_stamp, time_stamp     );
  rd_printf("User ID   : %u"             , header.user_id );
  rd_printf("Group ID  : %u"             , header.group_id);
  rd_printf("Mode      : %S"             , header.mode    );
  rd_printf("Data      : [%#llx-%#llx)", header.data_range.min, header.data_range.max);
  
  scratch_end(scratch);
}

internal void
coff_print_section_table(Arena               *arena,
                         String8List        *out,
                         String8             indent,
                         String8             string_table,
                         COFF_Symbol32Array  symbol_table,
                         U64                 section_count,
                         COFF_SectionHeader *section_table)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 *symlinks = push_array(scratch.arena, String8, section_count);
  for (U64 i = 0; i < symbol_table.count; ++i) {
    COFF_Symbol32              *symbol = symbol_table.v+i;
    COFF_SymbolValueInterpType  interp = coff_interp_symbol(symbol->section_number, symbol->value, symbol->storage_class);
    if (interp == COFF_SymbolValueInterp_Regular &&
        symbol->aux_symbol_count == 0 &&
        (symbol->storage_class == COFF_SymStorageClass_External || symbol->storage_class == COFF_SymStorageClass_Static)) {
      if (symbol->section_number > 0 && symbol->section_number <= symbol_table.count) {
        COFF_SectionHeader *header = section_table+(symbol->section_number-1);
        if (header->flags & COFF_SectionFlag_LnkCOMDAT) {
          symlinks[symbol->section_number-1] = coff_read_symbol_name(string_table, &symbol->name);
        }
      }
    }
    i += symbol->aux_symbol_count;
  }
  
  if (section_count) {
    rd_printf("# Section Table");
    rd_indent();
    
    rd_printf("%-4s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-5s %-10s %s",
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
    
    for (U64 i = 0; i < section_count; ++i) {
      COFF_SectionHeader *header = section_table+i;
      
      String8 name      = str8_cstring_capped(header->name, header->name+sizeof(header->name));
      String8 full_name = coff_name_from_section_header(string_table, header);
      
      String8 align;
      {
        U64 align_size = coff_align_size_from_section_flags(header->flags);
        align = push_str8f(scratch.arena, "%u", align_size);
      }
      
      String8 flags;
      {
        String8List mem_flags = {0};
        if (header->flags & COFF_SectionFlag_MemRead) {
          str8_list_pushf(scratch.arena, &mem_flags, "r");
        }
        if (header->flags & COFF_SectionFlag_MemWrite) {
          str8_list_pushf(scratch.arena, &mem_flags, "w");
        }
        if (header->flags & COFF_SectionFlag_MemExecute) {
          str8_list_pushf(scratch.arena, &mem_flags, "x");
        }
        
        String8List cnt_flags = {0};
        if (header->flags & COFF_SectionFlag_CntCode) {
          str8_list_pushf(scratch.arena, &cnt_flags, "c");
        }
        if (header->flags & COFF_SectionFlag_CntInitializedData) {
          str8_list_pushf(scratch.arena, &cnt_flags, "d");
        }
        if (header->flags & COFF_SectionFlag_CntUninitializedData) {
          str8_list_pushf(scratch.arena, &cnt_flags, "u");
        }
        
        String8List mem_extra_flags = {0};
        if (header->flags & COFF_SectionFlag_MemShared) {
          str8_list_pushf(scratch.arena, &mem_flags, "s");
        }
        if (header->flags & COFF_SectionFlag_Mem16Bit) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "h");
        }
        if (header->flags & COFF_SectionFlag_MemLocked) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "l");
        }
        if (header->flags & COFF_SectionFlag_MemDiscardable) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "d");
        }
        if (header->flags & COFF_SectionFlag_MemNotCached) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "c");
        }
        if (header->flags & COFF_SectionFlag_MemNotPaged) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "p");
        }
        
        String8List lnk_flags = {0};
        if (header->flags & COFF_SectionFlag_LnkRemove) {
          str8_list_pushf(scratch.arena, &lnk_flags, "r");
        }
        if (header->flags & COFF_SectionFlag_LnkCOMDAT) {
          str8_list_pushf(scratch.arena, &lnk_flags, "c");
        }
        if (header->flags & COFF_SectionFlag_LnkOther) {
          str8_list_pushf(scratch.arena, &lnk_flags, "o");
        }
        if (header->flags & COFF_SectionFlag_LnkInfo) {
          str8_list_pushf(scratch.arena, &lnk_flags, "i");
        }
        if (header->flags & COFF_SectionFlag_LnkNRelocOvfl) {
          str8_list_pushf(scratch.arena, &lnk_flags, "f");
        }
        
        String8List other_flags = {0};
        if (header->flags & COFF_SectionFlag_TypeNoPad) {
          str8_list_pushf(scratch.arena, &other_flags, "n");
        }
        if (header->flags & COFF_SectionFlag_GpRel) {
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
      str8_list_pushf(scratch.arena, &l, "%-4x",  i+1                );
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
      rd_printf("%S", line);
      
      if (full_name.size != name.size) {
        rd_indent();
        rd_printf("Full Name: %S", full_name);
        rd_unindent();
      }
    }
    
    rd_newline();
    rd_printf("Flags:");
    rd_indent();
    rd_printf("r = MemRead    w = MemWrite        x = MemExecute");
    rd_printf("c = CntCode    d = InitializedData u = UninitializedData");
    rd_printf("s = MemShared  h = Mem16bit        l = MemLocked          d = MemDiscardable c = MemNotCached  p = MemNotPaged");
    rd_printf("r = LnkRemove  c = LnkComdat       o = LnkOther           i = LnkInfo        f = LnkNRelocOvfl");
    rd_printf("g = GpRel      n = TypeNoPad");
    rd_unindent();
    
    rd_unindent();
    rd_newline();
  }
  
  scratch_end(scratch);
}

internal void
coff_disasm_sections(Arena              *arena,
                     String8List        *out,
                     String8             indent,
                     String8             raw_data,
                     COFF_MachineType    machine,
                     U64                 image_base,
                     B32                 is_obj,
                     RD_MarkerArray     *section_markers,
                     U64                 section_count,
                     COFF_SectionHeader *sections)
{
  if (section_count) {
    for (U64 sect_idx = 0; sect_idx < section_count; ++sect_idx) {
      COFF_SectionHeader *sect = sections+sect_idx;
      if (sect->flags & COFF_SectionFlag_CntCode) {
        U64            sect_off    = is_obj ? sect->foff : sect->voff;
        U64            sect_size   = is_obj ? sect->fsize : sect->vsize;
        String8        raw_code    = str8_substr(raw_data, rng_1u64(sect->foff, sect->foff+sect_size));
        U64            sect_number = sect_idx+1;
        RD_MarkerArray markers     = section_markers[sect_number];
        
        rd_printf("# Disassembly [Section No. %#llx]", sect_number);
        rd_indent();
        rd_print_disasm(arena, out, indent, arch_from_coff_machine(machine), image_base, sect_off, markers.count, markers.v, raw_code);
        rd_unindent();
      }
    }
  }
}

internal void
coff_raw_data_sections(Arena              *arena,
                       String8List        *out,
                       String8             indent,
                       String8             raw_data,
                       B32                 is_obj,
                       RD_MarkerArray     *section_markers,
                       U64                 section_count,
                       COFF_SectionHeader *section_table)
{
  if (section_count) {
    for (U64 sect_idx = 0; sect_idx < section_count; ++sect_idx) {
      COFF_SectionHeader *sect = section_table+sect_idx;
      if (sect->fsize > 0) {
        U64         sect_size = is_obj ? sect->fsize : sect->vsize;
        String8     raw_sect  = str8_substr(raw_data, rng_1u64(sect->foff, sect->foff+sect_size));
        RD_MarkerArray markers   = section_markers[sect_idx];
        
        rd_printf("# Raw Data [Section No. %#llx]", (sect_idx+1));
        rd_indent();
        rd_print_raw_data(arena, out, indent, 32, markers.count, markers.v, raw_sect);
        rd_unindent();
        rd_newline();
      }
    }
  }
}

internal void
coff_print_relocs(Arena              *arena,
                  String8List        *out,
                  String8             indent,
                  String8             raw_data,
                  String8             string_table,
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
        rd_printf("# Relocations");
        rd_indent();
      }
      
      rd_printf("## Section %llx", sect_idx);
      rd_indent();
      
      rd_printf("%-4s %-8s %-16s %-16s %-8s %-7s", "No.", "Offset", "Type", "ApplyTo", "SymIdx", "SymName");
      
      for (U64 reloc_idx = 0; reloc_idx < reloc_info.count; ++reloc_idx) {
        COFF_Reloc *reloc      = (COFF_Reloc*)(raw_data.str + reloc_info.array_off) + reloc_idx;
        String8     type       = coff_string_from_reloc(machine, reloc->type);
        U64         apply_size = coff_apply_size_from_reloc(machine, reloc->type);
        
        U64 apply_foff = sect_header->foff + reloc->apply_off;
        if (apply_foff + apply_size > raw_data.size) {
          rd_errorf("out of bounds apply file offset %#llx in relocation %#llx", apply_foff, reloc_idx);
          break;
        }
        
        U64 raw_apply;
        AssertAlways(apply_size <= sizeof(raw_apply));
        MemoryCopy(&raw_apply, raw_data.str + apply_foff, apply_size);
        S64 apply = extend_sign64(raw_apply, apply_size);
        
        if (reloc->isymbol > symbols.count) {
          rd_errorf("out of bounds symbol index %u in relocation %#llx", reloc->isymbol, reloc_idx);
          break;
        }
        
        COFF_Symbol32 *symbol      = symbols.v+reloc->isymbol;
        String8        symbol_name = coff_read_symbol_name(string_table, &symbol->name);
        
        String8List line = {0};
        str8_list_pushf(scratch.arena, &line, "%-4x",  reloc_idx       );
        str8_list_pushf(scratch.arena, &line, "%08x",  reloc->apply_off);
        str8_list_pushf(scratch.arena, &line, "%-16S", type            );
        str8_list_pushf(scratch.arena, &line, "%016x", apply           );
        str8_list_pushf(scratch.arena, &line, "%S",    symbol_name     );
        
        String8 l = str8_list_join(scratch.arena, &line, &(StringJoin){.sep=str8_lit(" ")});
        rd_printf("%S", l);
      }
      
      rd_unindent();
    }
  }
  
  if (!print_header) {
    rd_unindent();
  }
  rd_newline();
  
  scratch_end(scratch);
}

internal void
coff_print_symbol_table(Arena              *arena,
                        String8List        *out,
                        String8             indent,
                        String8             raw_data,
                        B32                 is_big_obj,
                        String8             string_table,
                        COFF_Symbol32Array  symbols)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  if (symbols.count) {
    rd_printf("# Symbol Table");
    rd_indent();
    
    rd_printf("%-4s %-8s %-10s %-4s %-4s %-4s %-16s %-20s", 
              "No.", "Value", "SectNum", "Aux", "Msb", "Lsb", "Storage", "Name");
    
    for (U64 i = 0; i < symbols.count; ++i) {
      COFF_Symbol32 *symbol        = &symbols.v[i];
      String8        name          = coff_read_symbol_name(string_table, &symbol->name);
      String8        msb           = coff_string_from_sym_dtype(symbol->type.u.msb);
      String8        lsb           = coff_string_from_sym_type(symbol->type.u.lsb);
      String8        storage_class = coff_string_from_sym_storage_class(symbol->storage_class);
      String8        section_number;
      switch (symbol->section_number) {
        case COFF_Symbol_UndefinedSection: section_number = str8_lit("Undef"); break;
        case COFF_Symbol_AbsSection32:     section_number = str8_lit("Abs");   break;
        case COFF_Symbol_DebugSection32:   section_number = str8_lit("Debug"); break;
        default:                           section_number = push_str8f(scratch.arena, "%010x", symbol->section_number); break;
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
      rd_printf("%S", l);
      
      rd_indent();
      for (U64 k=i+1, c = i+symbol->aux_symbol_count; k <= c; ++k) {
        void *raw_aux = &symbols.v[k];
        switch (symbol->storage_class) {
          case COFF_SymStorageClass_External: {
            COFF_SymbolFuncDef *func_def = (COFF_SymbolFuncDef*)&symbols.v[k];
            rd_printf("Tag Index %#x, Total Size %#x, Line Numbers %#x, Next Function %#x", 
                      func_def->tag_index, func_def->total_size, func_def->ptr_to_ln, func_def->ptr_to_next_func);
          } break;
          case COFF_SymStorageClass_Function: {
            COFF_SymbolFunc *func = raw_aux;
            rd_printf("Ordinal Line Number %#x, Next Function %#x", func->ln, func->ptr_to_next_func);
          } break;
          case COFF_SymStorageClass_WeakExternal: {
            COFF_SymbolWeakExt *weak = raw_aux;
            String8             type = coff_string_from_weak_ext_type(weak->characteristics);
            rd_printf("Tag Index %#x, Characteristics %S", weak->tag_index, type);
          } break;
          case COFF_SymStorageClass_File: {
            COFF_SymbolFile *file = raw_aux;
            String8          name = str8_cstring_capped(file->name, file->name+sizeof(file->name));
            rd_printf("Name %S", name);
          } break;
          case COFF_SymStorageClass_Static: {
            COFF_SymbolSecDef *sd        = raw_aux;
            String8            selection = coff_string_from_comdat_select_type(sd->selection);
            U32 number = sd->number_lo;
            if (is_big_obj) {
              number |= (U32)sd->number_hi << 16;
            }
            if (number) {
              rd_printf("Length %x, Reloc Count %u, Line Count %u, Checksum %x, Section %x, Selection %S",
                        sd->length, sd->number_of_relocations, sd->number_of_ln, sd->check_sum, number, selection);
            } else {
              rd_printf("Length %x, Reloc Count %u, Line Count %u, Checksum %x",
                        sd->length, sd->number_of_relocations, sd->number_of_ln, sd->check_sum);
            }
          } break;
          default: {
            rd_printf("???");
          } break;
        }
      }
      
      i += symbol->aux_symbol_count;
      rd_unindent();
    }
    
    rd_unindent();
    rd_newline();
  }
  
  scratch_end(scratch);
}

internal void
coff_print_big_obj_header(Arena *arena, String8List *out, String8 indent, COFF_BigObjHeader *header)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 time_stamp = coff_string_from_time_stamp(scratch.arena, header->time_stamp);
  String8 machine    = coff_string_from_machine_type(header->machine);
  
  rd_printf("# Big Obj");
  rd_indent();
  rd_printf("Time Stamp   : %#x (%S)",  header->time_stamp, time_stamp);
  rd_printf("Machine      : %#x (%S)",  header->machine, machine      );
  rd_printf("Section Count: %u",  header->section_count    );
  rd_printf("Symbol Table : %#x", header->symbol_table_foff);
  rd_printf("Symbol Count : %u",  header->symbol_count     );
  rd_unindent();
  
  scratch_end(scratch);
}

internal void
coff_print_file_header(Arena *arena, String8List *out, String8 indent, COFF_FileHeader *header)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 time_stamp = coff_string_from_time_stamp(scratch.arena, header->time_stamp);
  String8 machine    = coff_string_from_machine_type(header->machine);
  String8 flags      = coff_string_from_flags(scratch.arena, header->flags);
  
  rd_printf("# COFF File Header");
  rd_indent();
  rd_printf("Time Stamp          : %#x (%S)", header->time_stamp, time_stamp                            );
  rd_printf("Machine             : %#x %S",   header->machine, machine                                  );
  rd_printf("Section Count       : %u",       header->section_count                                     );
  rd_printf("Symbol Table        : %#x",      header->symbol_table_foff                                 );
  rd_printf("Symbol Count        : %u",       header->symbol_count                                      );
  rd_printf("Optional Header Size: %#x (%m)", header->optional_header_size, header->optional_header_size);
  rd_printf("Flags               : %#x (%S)", header->flags, flags                                      );
  rd_unindent();
  
  scratch_end(scratch);
}

internal void
coff_print_import(Arena *arena, String8List *out, String8 indent, COFF_ParsedArchiveImportHeader *header)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 machine    = coff_string_from_machine_type(header->machine);
  String8 time_stamp = coff_string_from_time_stamp(scratch.arena, header->time_stamp);
  
  rd_printf("# Import");
  rd_indent();
  rd_printf("Version   : %u", header->version        );
  rd_printf("Machine   : %S", machine                );
  rd_printf("Time Stamp: %#x (%S)", header->time_stamp, time_stamp      );
  rd_printf("Data Size : %#x (%m)", header->data_size, header->data_size);
  rd_printf("Hint      : %u", header->hint_or_ordinal);
  rd_printf("Type      : %u", header->type           );
  rd_printf("Import By : %u", header->import_by      );
  rd_printf("Function  : %S", header->func_name      );
  rd_printf("DLL       : %S", header->dll_name       );
  rd_unindent();
  
  scratch_end(scratch);
}

internal void
coff_print_big_obj(Arena *arena, String8List *out, String8 indent, String8 raw_data, RD_Option opts)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  COFF_FileHeaderInfo header_info = coff_file_header_info_from_data(raw_data);
  
  String8 raw_header        = str8_substr(raw_data, header_info.header_range);
  String8 raw_section_table = str8_substr(raw_data, header_info.section_table_range);
  String8 raw_string_table  = str8_substr(raw_data, header_info.string_table_range);
  
  COFF_BigObjHeader  *big_obj       = (COFF_BigObjHeader *)raw_header.str;
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)raw_section_table.str;
  COFF_Symbol32Array  symbol_table  = coff_symbol_array_from_data_32(scratch.arena, raw_data, header_info.symbol_table_range.min, big_obj->symbol_count);
  
  if (opts & RD_Option_Headers) {
    coff_print_big_obj_header(arena, out, indent, big_obj);
    rd_newline();
  }
  
  if (opts & RD_Option_Sections) {
    Rng1U64 sect_headers_range = rng_1u64(sizeof(*big_obj), sizeof(*big_obj) + sizeof(COFF_SectionHeader)*big_obj->section_count);
    Rng1U64 symbols_range      = rng_1u64(big_obj->symbol_table_foff, big_obj->symbol_table_foff + sizeof(COFF_Symbol32)*big_obj->symbol_count);
    
    if (sect_headers_range.max > raw_data.size) {
      rd_errorf("not enough bytes to read big obj section headers");
      goto exit;
    }
    if (big_obj->symbol_count) {
      if (symbols_range.max > raw_data.size) {
        rd_errorf("not enough bytes to read big obj symbol table");
        goto exit;
      }
      if (contains_1u64(symbols_range, sect_headers_range.min) ||
          contains_1u64(symbols_range, sect_headers_range.max)) {
        rd_errorf("section headers and symbol table ranges overlap");
        goto exit;
      }
    }
    
    coff_print_section_table(arena, out, indent, raw_string_table, symbol_table, big_obj->section_count, section_table);
    rd_newline();
  }
  
  if (opts & RD_Option_Relocs) {
    coff_print_relocs(arena, out, indent, raw_data, raw_string_table, big_obj->machine, big_obj->section_count, section_table, symbol_table);
    rd_newline();
  }
  
  if (opts & RD_Option_Symbols) {
    coff_print_symbol_table(arena, out, indent, raw_data, 1, raw_string_table, symbol_table);
    rd_newline();
  }
  
  exit:;
  scratch_end(scratch);
}

internal void
coff_print_obj(Arena *arena, String8List *out, String8 indent, String8 raw_data, RD_Option opts)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  COFF_FileHeaderInfo header_info = coff_file_header_info_from_data(raw_data);
  
  String8 raw_header        = str8_substr(raw_data, header_info.header_range);
  String8 raw_section_table = str8_substr(raw_data, header_info.section_table_range);
  String8 raw_string_table  = str8_substr(raw_data, header_info.string_table_range);
  
  COFF_FileHeader    *header        = (COFF_FileHeader *)raw_header.str;
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)raw_section_table.str;
  COFF_Symbol32Array  symbol_table  = coff_symbol_array_from_data_16(scratch.arena, raw_data, header_info.symbol_table_range.min, header->symbol_count);
  Arch                arch          = arch_from_coff_machine(header->machine);
  
  if (opts & RD_Option_Headers) {
    coff_print_file_header(arena, out, indent, header);
    rd_newline();
  }
  
  if (opts & RD_Option_Sections) {
    Rng1U64 sect_headers_range = rng_1u64(sizeof(*header), sizeof(*header) + sizeof(COFF_SectionHeader)*header->section_count);
    Rng1U64 symbols_range      = rng_1u64(header->symbol_table_foff, header->symbol_table_foff + sizeof(COFF_Symbol16)*header->symbol_count);
    
    if (sect_headers_range.max > raw_data.size) {
      rd_errorf("not enough bytes to read obj section headers");
      goto exit;
    }
    if (header->symbol_count) {
      if (symbols_range.max > raw_data.size) {
        rd_errorf("not enough bytes to read obj symbol table");
        goto exit;
      }
      if (contains_1u64(symbols_range, sect_headers_range.min) ||
          contains_1u64(symbols_range, sect_headers_range.max)) {
        rd_errorf("section headers and symbol table ranges overlap");
        goto exit;
      }
    }
    
    coff_print_section_table(arena, out, indent, raw_string_table, symbol_table, header->section_count, section_table);
    rd_newline();
  }
  
  if (opts & RD_Option_Relocs) {
    coff_print_relocs(arena, out, indent, raw_data, raw_string_table, header->machine, header->section_count, section_table, symbol_table);
    rd_newline();
  }
  
  if (opts & RD_Option_Symbols) {
    coff_print_symbol_table(arena, out, indent, raw_data, 0, raw_string_table, symbol_table);
    rd_newline();
  }
  
  RD_MarkerArray *section_markers = 0;
  if (opts & (RD_Option_Disasm|RD_Option_Rawdata)) {
    section_markers = rd_section_markers_from_coff_symbol_table(scratch.arena, raw_string_table, header->section_count, symbol_table);
  }
  
  if (opts & RD_Option_Rawdata) {
    coff_raw_data_sections(arena, out, indent, raw_data, 1, section_markers, header->section_count, section_table);
  }
  
  if (opts & RD_Option_Disasm) {
    coff_disasm_sections(arena, out, indent, raw_data, header->machine, 0, 1, section_markers, header->section_count, section_table);
    rd_newline();
  }
  
  if (opts & RD_Option_Codeview) {
    cv_format_debug_sections(arena, out, indent, raw_data, raw_string_table, header->section_count, section_table);
  }
  
  if (opts & RD_Option_Dwarf) {
    DW_Raw dwarf_input = dw_raw_from_coff_section_table(scratch.arena, raw_data, raw_string_table, header->section_count, section_table);
    dw_format(arena, out, indent, opts, &dwarf_input, arch, ExecutableImageKind_CoffPe);
  }
  
  exit:;
  scratch_end(scratch);
}

internal void
coff_print_archive(Arena *arena, String8List *out, String8 indent, String8 raw_archive, RD_Option opts)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  COFF_ArchiveParse archive_parse = coff_archive_parse_from_data(raw_archive);
  
  if (archive_parse.error.size) {
    rd_errorf("%S", archive_parse.error);
    return;
  }
  
  COFF_ArchiveFirstMember first_member = archive_parse.first_member;
  {
    rd_printf("# First Header");
    rd_indent();
    
    rd_printf("Symbol Count     : %u",         first_member.symbol_count);
    rd_printf("String Table Size: %#llx (%M)", first_member.string_table.size, first_member.string_table.size);
    
    rd_printf("Members:");
    rd_indent();
    
    String8List string_table = str8_split_by_string_chars(scratch.arena, first_member.string_table, str8_lit("\0"), 0);
    
    if (string_table.node_count == first_member.member_offset_count) {
      String8Node *string_n = string_table.first;
      
      for (U64 i = 0; i < string_table.node_count; ++i, string_n = string_n->next) {
        U32 offset = from_be_u32(first_member.member_offsets[i]);
        rd_printf("[%4u] %#08x %S", i, offset, string_n->string);
      }
    } else {
      rd_errorf("Member offset count (%llu) doesn't match string table count (%llu)", first_member.member_offset_count);
    }
    
    rd_unindent();
    rd_unindent();
    rd_newline();
  }
  
  if (archive_parse.has_second_header) {
    COFF_ArchiveSecondMember second_member = archive_parse.second_member;
    
    rd_printf("# Second Header");
    rd_indent();
    
    rd_printf("Member Count     : %u",         second_member.member_count);
    rd_printf("Symbol Count     : %u",         second_member.symbol_count);
    rd_printf("String Table Size: %#llx (%M)", second_member.string_table.size, second_member.string_table.size);
    
    String8List string_table = str8_split_by_string_chars(scratch.arena, second_member.string_table, str8_lit("\0"), 0);
    
    rd_printf("Members:");
    rd_indent();
    if (second_member.symbol_index_count == second_member.symbol_count) {
      String8Node *string_n = string_table.first;
      for (U64 i = 0; i < second_member.symbol_count; ++i, string_n = string_n->next) {
        U16 symbol_number = second_member.symbol_indices[i];
        if (symbol_number > 0 && symbol_number <= second_member.member_offset_count) {
          U16 symbol_idx    = symbol_number - 1;
          U32 member_offset = second_member.member_offsets[i];
          rd_printf("[%4u] %#08x %S", i, member_offset, string_n->string);
        } else {
          rd_errorf("[%4u] Out of bounds symbol number %u", i, symbol_number);
          break;
        }
      }
    } else {
      rd_errorf("Symbol index count %u doesn't match symbol count %u",
                second_member.symbol_index_count, second_member.symbol_count);
    }
    rd_unindent();
    
    rd_unindent();
    rd_newline();
  }
  
  if (archive_parse.has_long_names && opts & RD_Option_LongNames) {
    rd_printf("# Long Names");
    rd_indent();
    
    String8List long_names = str8_split_by_string_chars(scratch.arena, archive_parse.long_names, str8_lit("\0"), 0);
    U64 name_idx = 0;
    for (String8Node *name_n = long_names.first; name_n != 0; name_n = name_n->next, ++name_idx) {
      U64 offset = (U64)(name_n->string.str - archive_parse.long_names.str);
      rd_printf("[%-4u] %#08x %S", name_idx, offset, name_n->string);
    }
    
    rd_unindent();
    rd_newline();
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
  
  rd_printf("# Members");
  rd_indent();
  
  for (U64 i = 0; i < member_offset_count; ++i) {
    U64                next_member_offset = i+1 < member_offset_count ? member_offsets[i+1] : raw_archive.size;
    U64                member_offset      = member_offsets[i];
    String8            raw_member         = str8_substr(raw_archive, rng_1u64(member_offset, next_member_offset));
    COFF_ArchiveMember member             = coff_archive_member_from_data(raw_member);
    COFF_DataType      member_type        = coff_data_type_from_data(member.data);
    
    rd_printf("Member @ %#llx", member_offset);
    rd_indent();
    
    if (opts & RD_Option_Headers) {
      coff_print_archive_member_header(arena, out, indent, member.header, archive_parse.long_names);
      rd_newline();
    }
    
    switch (member_type) {
      case COFF_DataType_Obj: {
        coff_print_obj(arena, out, indent, member.data, opts);
      } break;
      case COFF_DataType_BigObj: {
        coff_print_big_obj(arena, out, indent, member.data, opts);
      } break;
      case COFF_DataType_Import: {
        if (opts & RD_Option_Headers) {
          COFF_ParsedArchiveImportHeader header = {0};
          U64 parse_size = coff_parse_import(member.data, 0, &header);
          if (parse_size) {
            coff_print_import(arena, out, indent, &header);
          } else {
            rd_errorf("not enough bytes to parse import header");
          }
        }
      } break;
      case COFF_DataType_Null: {
        rd_errorf("unknown member format", member_offset);
      } break;
    }
    
    rd_unindent();
    rd_newline();
  }
  
  rd_unindent();
  
  scratch_end(scratch);
}
#endif
