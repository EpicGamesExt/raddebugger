// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U64
coff_align_size_from_section_flags(COFF_SectionFlags flags)
{
  U32 align = 0;
  U32 align_index = COFF_SectionFlags_ExtractAlign(flags);
  switch (align_index) {
    default: break;
    case COFF_SectionAlign_1Bytes:    align = 1;    break;
    case COFF_SectionAlign_2Bytes:    align = 2;    break;
    case COFF_SectionAlign_4Bytes:    align = 4;    break;
    case COFF_SectionAlign_8Bytes:    align = 8;    break;
    case COFF_SectionAlign_16Bytes:   align = 16;   break;
    case COFF_SectionAlign_32Bytes:   align = 32;   break;
    case COFF_SectionAlign_64Bytes:   align = 64;   break;
    case COFF_SectionAlign_128Bytes:  align = 128;  break;
    case COFF_SectionAlign_256Bytes:  align = 256;  break;
    case COFF_SectionAlign_512Bytes:  align = 512;  break;
    case COFF_SectionAlign_1024Bytes: align = 1024; break;
    case COFF_SectionAlign_2048Bytes: align = 2048; break;
    case COFF_SectionAlign_4096Bytes: align = 4096; break;
    case COFF_SectionAlign_8192Bytes: align = 8192; break;
  }
  return align;
}

internal COFF_SectionFlags
coff_section_flag_from_align_size(U64 align)
{
  COFF_SectionFlags flags = 0;
  switch (align) {
  case 0:    flags = COFF_SectionAlign_None;      break;
  case 1:    flags = COFF_SectionAlign_1Bytes;    break;
  case 2:    flags = COFF_SectionAlign_2Bytes;    break;
  case 4:    flags = COFF_SectionAlign_4Bytes;    break;
  case 8:    flags = COFF_SectionAlign_8Bytes;    break;
  case 16:   flags = COFF_SectionAlign_16Bytes;   break;
  case 32:   flags = COFF_SectionAlign_32Bytes;   break;
  case 64:   flags = COFF_SectionAlign_64Bytes;   break;
  case 128:  flags = COFF_SectionAlign_128Bytes;  break;
  case 256:  flags = COFF_SectionAlign_256Bytes;  break;
  case 512:  flags = COFF_SectionAlign_512Bytes;  break;
  case 1024: flags = COFF_SectionAlign_1024Bytes; break;
  case 2048: flags = COFF_SectionAlign_2048Bytes; break;
  case 4096: flags = COFF_SectionAlign_4096Bytes; break;
  case 8192: flags = COFF_SectionAlign_8192Bytes; break;
  }
  flags <<= COFF_SectionFlag_AlignShift;
  return flags;
}

internal String8
coff_name_from_section_header(String8 string_table, COFF_SectionHeader *header)
{
  String8 name = str8_cstring_capped(header->name, header->name + sizeof(header->name));
  if (name.str[0] == '/') {
    String8 name_off_str = str8_skip(name, 1);
    U64     name_off     = u64_from_str8(name_off_str, 10);
    name = str8_cstring_capped(string_table.str + name_off, string_table.str+string_table.size);
  }
  return name;
}

internal void
coff_parse_section_name(String8 full_name, String8 *name_out, String8 *postfix_out)
{
  // dollar sign has multiple interpretations that depend on the type of the section.
  // 1. when section contains code/data it indicates section precedence
  // 2. when section starts with .debug it indicates type of data inside the section
  //    T: Types
  //    S: Symbols
  //    P: Precompiled types
  //    F: FPO data
  //    H: Clang extension produced with /debug:ghash, array of type hashes
  *name_out    = full_name;
  *postfix_out = str8_lit("");
  for (U64 i = 0; i < full_name.size; ++i) {
    if (full_name.str[i] == '$') {
      *name_out    = str8(full_name.str, i);
      *postfix_out = str8(full_name.str + i, full_name.size - i);
      break;
    }
  }
}

internal String8
coff_read_symbol_name(String8 string_table, COFF_SymbolName *name)
{
  String8 name_str = str8_lit("");
  if (name->long_name.zeroes == 0) {
    str8_deserial_read_cstr(string_table, name->long_name.string_table_offset, &name_str);
  } else {
    U32 i;
    for (i = 0; i < sizeof(name->short_name); ++i) {
      if (name->short_name[i] == '\0') {
        break;
      }
    }
    name_str = str8(name->short_name, i);
  }
  return name_str;
}

internal U64
coff_apply_size_from_reloc_x64(COFF_Reloc_X64 x)
{
  switch (x) {
    case COFF_Reloc_X64_Abs:      return 4;
    case COFF_Reloc_X64_Addr64:   return 8;
    case COFF_Reloc_X64_Addr32:   return 4;
    case COFF_Reloc_X64_Addr32Nb: return 4;
    case COFF_Reloc_X64_Rel32:    return 4;
    case COFF_Reloc_X64_Rel32_1:  return 4;
    case COFF_Reloc_X64_Rel32_2:  return 4;
    case COFF_Reloc_X64_Rel32_3:  return 4;
    case COFF_Reloc_X64_Rel32_4:  return 4;
    case COFF_Reloc_X64_Rel32_5:  return 4;
    case COFF_Reloc_X64_Section:  return 2;
    case COFF_Reloc_X64_SecRel:   return 4;
    case COFF_Reloc_X64_SRel32:   return 4;
    
    case COFF_Reloc_X64_SecRel7:
    case COFF_Reloc_X64_Token:
    case COFF_Reloc_X64_Pair:
    case COFF_Reloc_X64_SSpan32:
    NotImplemented;
    break;
  }
  return 0;
}

internal U64
coff_apply_size_from_reloc_x86(COFF_Reloc_X86 x)
{
  switch (x) {
    case COFF_Reloc_X86_Abs:      return 4;
    case COFF_Reloc_X86_Dir16:    return 2;
    case COFF_Reloc_X86_Rel16:    return 2;
    case COFF_Reloc_X86_Dir32:    return 4;
    case COFF_Reloc_X86_Dir32Nb:  return 4;
    case COFF_Reloc_X86_Seg12:    return 0;
    case COFF_Reloc_X86_Section:  return 2;
    case COFF_Reloc_X86_SecRel:   return 4;
    case COFF_Reloc_X86_Token:    return 4;
    case COFF_Reloc_X86_SecRel7:  return 1;
    case COFF_Reloc_X86_Rel32:    return 4;
    
    case COFF_Reloc_X86_Unknown0:
    case COFF_Reloc_X86_Unknown2:
    case COFF_Reloc_X86_Unknown3:
    case COFF_Reloc_X86_Unknown4:
    case COFF_Reloc_X86_Unknown6:
    case COFF_Reloc_X86_Unknown7:
    case COFF_Reloc_X86_Unknown8:
    case COFF_Reloc_X86_Unknown9:
    NotImplemented;
    break;
  }
  return 0;
}

internal U64
coff_apply_size_from_reloc(COFF_MachineType machine, COFF_RelocType x)
{
  switch (machine) {
    case COFF_MachineType_X64: return coff_apply_size_from_reloc_x64(x);
    case COFF_MachineType_X86: return coff_apply_size_from_reloc_x86(x);
    default: NotImplemented;
  }
  return 0;
}

internal COFF_RelocValue
coff_pick_reloc_value_x64(COFF_Reloc_X64 type,
                          U64            image_base,
                          U64            reloc_virtual_offset,
                          U32            symbol_section_number,
                          U32            symbol_section_offset,
                          S64            symbol_virtual_offset)
{
  U64 reloc_value_size = 0;
  S64 reloc_value      = 0;

  switch (type) {
  case COFF_Reloc_X64_Abs: {} break;
  case COFF_Reloc_X64_Addr64: {
    reloc_value_size = 8;
    reloc_value      = symbol_virtual_offset + (S64)image_base;
  } break;
  case COFF_Reloc_X64_Addr32: {
    reloc_value_size = 4;
    reloc_value      = safe_cast_s32(symbol_virtual_offset + (S64)image_base);
  } break;
  case COFF_Reloc_X64_Addr32Nb: {
    reloc_value_size = 4;
    reloc_value      = symbol_virtual_offset;
  } break;
  case COFF_Reloc_X64_Rel32: {
    reloc_value_size = 4;
    reloc_value      = safe_cast_s32(symbol_virtual_offset - reloc_virtual_offset - (4 + 0));
  } break;
  case COFF_Reloc_X64_Rel32_1: {
    reloc_value_size = 4;
    reloc_value      = safe_cast_s32(symbol_virtual_offset - reloc_virtual_offset - (4 + 1));
  } break;
  case COFF_Reloc_X64_Rel32_2: {
    reloc_value_size = 4;
    reloc_value      = safe_cast_s32(symbol_virtual_offset - reloc_virtual_offset - (4 + 2));
  } break;
  case COFF_Reloc_X64_Rel32_3: {
    reloc_value_size = 4;
    reloc_value      = safe_cast_s32(symbol_virtual_offset - reloc_virtual_offset - (4 + 3));
  } break;
  case COFF_Reloc_X64_Rel32_4: {
    reloc_value_size = 4;
    reloc_value      = safe_cast_s32(symbol_virtual_offset - reloc_virtual_offset - (4 + 4));
  } break;
  case COFF_Reloc_X64_Rel32_5: {
    reloc_value_size = 4;
    reloc_value = safe_cast_s32(symbol_virtual_offset - reloc_virtual_offset - (4 + 5));
  } break;
  case COFF_Reloc_X64_Section: {
    reloc_value_size = 4;
    reloc_value      = symbol_section_number;
  } break;
  case COFF_Reloc_X64_SecRel: {
    reloc_value_size = 4;
    reloc_value      = symbol_section_offset;
  } break;
  case COFF_Reloc_X64_SecRel7:
  case COFF_Reloc_X64_Token:
  case COFF_Reloc_X64_SRel32:
  case COFF_Reloc_X64_Pair:
  case COFF_Reloc_X64_SSpan32:
  case COFF_Reloc_X64_Unknown_11: {
    NotImplemented;
  } break;
  }

  COFF_RelocValue result = {0};
  result.size            = reloc_value_size;
  result.value           = reloc_value;

  return result;
}

internal String8
coff_make_lib_member_header(Arena *arena, String8 name, COFF_TimeStamp time_stamp, U16 user_id, U16 group_id, U16 mode, U32 size)
{
  Assert(name.size < 16);
  Assert(user_id < 10000);
  Assert(group_id < 10000);
  Assert(mode < 10000);
  Assert(size < 1000000000);
  
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  str8_list_pushf(scratch.arena, &list, "%-16.*s", str8_varg(name));
  str8_list_pushf(scratch.arena, &list, "%-12u", time_stamp);
  str8_list_pushf(scratch.arena, &list, "%-6u", user_id);
  str8_list_pushf(scratch.arena, &list, "%-6u", group_id);
  str8_list_pushf(scratch.arena, &list, "%-8u", mode);
  str8_list_pushf(scratch.arena, &list, "%-10u", size);
  str8_list_pushf(scratch.arena, &list, "`\n");
  String8 result = str8_list_join(arena, &list, 0);

  Assert(result.size == sizeof(COFF_ArchiveMemberHeader));
  scratch_end(scratch);
  return result;
}

internal String8
coff_make_import_lookup(Arena *arena, U16 hint, String8 name)
{
  U64 buffer_size = sizeof(hint) + (name.size + 1);
  U8 *buffer = push_array(arena, U8, buffer_size);
  *(U16*)buffer = hint;
  MemoryCopy(buffer + sizeof(hint), name.str, name.size);
  buffer[buffer_size - 1] = 0;
  String8 result = str8(buffer, buffer_size);
  return result;
}

internal U32
coff_make_ordinal32(U16 hint)
{
  U32 ordinal = (1 << 31) | hint;
  return ordinal;
}

internal U64
coff_make_ordinal64(U16 hint)
{
  U64 ordinal = (1ULL << 63) | hint;
  return ordinal;
}

internal String8
coff_ordinal_data_from_hint(Arena *arena, COFF_MachineType machine, U16 hint)
{
  String8 ordinal_data = {0}; 
  switch (machine) {
  case COFF_MachineType_Unknown: break;
  case COFF_MachineType_X64: {
    U64 *ordinal = push_array(arena, U64, 1);
    *ordinal     = coff_make_ordinal64(hint);
    ordinal_data = str8_struct(ordinal);
  } break;
  case COFF_MachineType_X86: {
    U32 *ordinal = push_array(arena, U32, 1);
    *ordinal     = coff_make_ordinal32(hint);
    ordinal_data = str8_struct(ordinal);
  } break;
  default: { NotImplemented; } break;
  }
  return ordinal_data;
}

internal String8
coff_make_import_header(Arena            *arena,
                        COFF_MachineType  machine,
                        COFF_TimeStamp    time_stamp,
                        String8           dll_name,
                        COFF_ImportByType import_by,
                        String8           name,
                        U16               hint_or_ordinal,
                        COFF_ImportType   type)
{
  COFF_ImportHeaderFlags flags = 0;
  flags |= (type & COFF_ImportHeader_TypeMask) << COFF_ImportHeader_TypeShift;
  flags |= import_by << COFF_ImportHeader_ImportByShift;

  COFF_ImportHeader header = {0};
  header.sig1              = COFF_MachineType_Unknown;
  header.sig2              = max_U16;
  header.version           = 0;
  header.machine           = machine;
  header.time_stamp        = time_stamp;
  header.data_size         = safe_cast_u32(name.size + dll_name.size + 2);
  header.hint_or_ordinal   = hint_or_ordinal;
  header.flags             = flags;
  
  // alloc memory
  U64  buffer_size = sizeof(header) + header.data_size;
  U8  *buffer      = push_array_no_zero(arena, U8, buffer_size);
  
  // copy header
  MemoryCopy(buffer, &header, sizeof(header));
  
  // copy function name
  U8 *func_name = buffer + sizeof(header);
  MemoryCopy(func_name, name.str, name.size);
  func_name[name.size] = 0;
  
  // copy dll name
  U8 *dll_name_buffer = buffer + sizeof(header) + name.size + 1;
  MemoryCopy(dll_name_buffer, dll_name.str, dll_name.size);
  dll_name_buffer[dll_name.size] = 0;
  
  String8 import_data = str8(buffer, buffer_size);
  return import_data;
}

internal U8
coff_code_align_byte_from_machine(COFF_MachineType machine)
{
  U8 align_byte = 0;
  switch (machine) {
  case COFF_MachineType_Unknown: break;
  case COFF_MachineType_X64:
  case COFF_MachineType_X86: {
    align_byte = 0xCC;
  } break;
  default: { NotImplemented; } break;
  }
  return align_byte;
}

internal U16
coff_default_align_from_machine(COFF_MachineType machine)
{
  U16 align = 0;
  switch (machine) {
  case COFF_MachineType_Unknown: break;
  case COFF_MachineType_X64: {
    align = 16;
  } break;
  default: { NotImplemented; } break;
  }
  return align;
}

internal U64
coff_word_size_from_machine(COFF_MachineType machine)
{
  U64 result = 0;
  switch (machine) {
    case COFF_MachineType_X64: result = 8; break;
    case COFF_MachineType_X86: result = 4; break;
  }
  return result;
}

internal U64
coff_default_exe_base_from_machine(COFF_MachineType machine)
{
  U64 exe_base = 0;
  switch (coff_word_size_from_machine(machine)) {
    case 4: exe_base = 0x400000;    break;
    case 8: exe_base = 0x140000000; break;
  }
  return exe_base;
}

internal U64
coff_default_dll_base_from_machine(COFF_MachineType machine)
{
  U64 dll_base = 0;
  switch (coff_word_size_from_machine(machine)) {
    case 4: dll_base = 0x10000000;  break;
    case 8: dll_base = 0x180000000; break;
  }
  return dll_base;
}


internal Arch
arch_from_coff_machine(COFF_MachineType machine)
{
  Arch result = Arch_Null;
  switch (machine) {
    case COFF_MachineType_Unknown: break;
    case COFF_MachineType_X86:   result = Arch_x86;   break;
    case COFF_MachineType_X64:   result = Arch_x64;   break;
    case COFF_MachineType_Arm:   result = Arch_arm32; break;
    case COFF_MachineType_Arm64: result = Arch_arm64; break;
  }
  return result;
}

internal U64
coff_foff_from_voff(COFF_SectionHeader *sections, U64 section_count, U64 voff)
{
  U64 foff = 0;
  for(U64 sect_idx = 0; sect_idx < section_count; sect_idx += 1)
  {
    COFF_SectionHeader *sect = &sections[sect_idx];
    if(sect->voff <= voff && voff < sect->voff+sect->vsize)
    {
      if(!(sect->flags & COFF_SectionFlag_CntUninitializedData))
      {
        foff = sect->foff + (voff - sect->voff);
      }
      break;
    }
  }
  return foff;
}

////////////////////////////////
//~ rjf: Enum -> String

internal String8
coff_string_from_time_stamp(Arena *arena, COFF_TimeStamp time_stamp)
{
  String8 result;
  if (time_stamp == 0) {
    result = str8_lit("0");
  } else if (time_stamp >= max_U32) {
    result = str8_lit("-1");
  } else {
    DateTime dt = date_time_from_unix_time(time_stamp);
    result = push_date_time_string(arena, &dt);
  }
  return result;
}

read_only struct
{
  String8          string;
  COFF_MachineType machine;
} g_coff_machine_map[] = {
  { str8_lit_comp(""),          COFF_MachineType_Unknown   },
  { str8_lit_comp("X86"),       COFF_MachineType_X86       },
  { str8_lit_comp("Amd64"),     COFF_MachineType_X64       },
  { str8_lit_comp("X64"),       COFF_MachineType_X64       },
  { str8_lit_comp("Am33"),      COFF_MachineType_Am33      },
  { str8_lit_comp("Arm"),       COFF_MachineType_Arm       },
  { str8_lit_comp("Arm64"),     COFF_MachineType_Arm64     },
  { str8_lit_comp("ArmNt"),     COFF_MachineType_ArmNt     },
  { str8_lit_comp("Ebc"),       COFF_MachineType_Ebc       },
  { str8_lit_comp("Ia64"),      COFF_MachineType_Ia64      },
  { str8_lit_comp("M32r"),      COFF_MachineType_M32R      },
  { str8_lit_comp("Mips16"),    COFF_MachineType_Mips16    },
  { str8_lit_comp("MipsFpu"),   COFF_MachineType_MipsFpu   },
  { str8_lit_comp("MipsFpu16"), COFF_MachineType_MipsFpu16 },
  { str8_lit_comp("PowerPc"),   COFF_MachineType_PowerPc   },
  { str8_lit_comp("PowerPcFp"), COFF_MachineType_PowerPcFp },
  { str8_lit_comp("R4000"),     COFF_MachineType_R4000     },
  { str8_lit_comp("RiscV32"),   COFF_MachineType_RiscV32   },
  { str8_lit_comp("RiscV64"),   COFF_MachineType_RiscV64   },
  { str8_lit_comp("Sh3"),       COFF_MachineType_Sh3       },
  { str8_lit_comp("Sh3Dsp"),    COFF_MachineType_Sh3Dsp    },
  { str8_lit_comp("Sh4"),       COFF_MachineType_Sh4       },
  { str8_lit_comp("Sh5"),       COFF_MachineType_Sh5       },
  { str8_lit_comp("Thumb"),     COFF_MachineType_Thumb     },
  { str8_lit_comp("WceMipsV2"), COFF_MachineType_WceMipsV2 },
};

read_only static struct {
  char *                name;
  COFF_ImportType type;
} g_coff_import_header_type_map[] = {
  { "Code",  COFF_ImportHeader_Code  },
  { "Data",  COFF_ImportHeader_Data  },
  { "Const", COFF_ImportHeader_Const },
};

internal String8
coff_string_from_comdat_select_type(COFF_ComdatSelectType type)
{
  String8 result = str8_zero();
  switch (type) {
    case COFF_ComdatSelect_Null:         result = str8_lit("Null");         break;
    case COFF_ComdatSelect_NoDuplicates: result = str8_lit("NoDuplicates"); break;
    case COFF_ComdatSelect_Any:          result = str8_lit("Any");          break;
    case COFF_ComdatSelect_SameSize:     result = str8_lit("SameSize");     break;
    case COFF_ComdatSelect_ExactMatch:   result = str8_lit("ExactMatch");   break;
    case COFF_ComdatSelect_Associative:  result = str8_lit("Associative");  break;
    case COFF_ComdatSelect_Largest:      result = str8_lit("Largest");      break;
  }
  return result;
}

internal String8
coff_string_from_machine_type(COFF_MachineType machine)
{
  for (U64 i = 0; i < ArrayCount(g_coff_machine_map); ++i) {
    if (g_coff_machine_map[i].machine == machine) {
      return g_coff_machine_map[i].string;
    }
  }
  return str8_zero();
}

internal String8
coff_string_from_flags(Arena *arena, COFF_FileHeaderFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  
  if (flags & COFF_FileHeaderFlag_RelocStripped) {
    str8_list_pushf(scratch.arena, &list, "Relocs Stripped");
  }
  if (flags & COFF_FileHeaderFlag_ExecutableImage) {
    str8_list_pushf(scratch.arena, &list, "Executable");
  }
  if (flags & COFF_FileHeaderFlag_LineNumbersStripped) {
    str8_list_pushf(scratch.arena, &list, "Line Numbers Stripped");
  }
  if (flags & COFF_FileHeaderFlag_SymbolsStripped) {
    str8_list_pushf(scratch.arena, &list, "Symbols Stripped");
  }
  if (flags & COFF_FileHeaderFlag_LargeAddressAware) {
    str8_list_pushf(scratch.arena, &list, "Large Address Aware");
  }
  if (flags & COFF_FileHeaderFlag_32BitMachine) {
    str8_list_pushf(scratch.arena, &list, "32-Bit Machine");
  }
  if (flags & COFF_FileHeaderFlag_DebugStripped) {
    str8_list_pushf(scratch.arena, &list, "Debug Stripped");
  }
  if (flags & COFF_FileHeaderFlag_RemovableRunFromSwap) {
    str8_list_pushf(scratch.arena, &list, "Removeable Run From Swap");
  }
  if (flags & COFF_FileHeaderFlag_NetRunFromSwap) {
    str8_list_pushf(scratch.arena, &list, "Net Run From Swap");
  }
  if (flags & COFF_FileHeaderFlag_System) {
    str8_list_pushf(scratch.arena, &list, "System");
  }
  if (flags & COFF_FileHeaderFlag_Dll) {
    str8_list_pushf(scratch.arena, &list, "DLL");
  }
  if (flags & COFF_FileHeaderFlag_UpSystemOnly) {
    str8_list_pushf(scratch.arena, &list, "Up System Only");
  }
  
  String8 result = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});
  
  scratch_end(scratch);
  return result;
}

internal String8
coff_string_from_section_flags(Arena *arena, COFF_SectionFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  
  if (flags & COFF_SectionFlag_TypeNoPad) {
    str8_list_pushf(scratch.arena, &list, "TypeNoPad");
  }
  if (flags & COFF_SectionFlag_CntCode) {
    str8_list_pushf(scratch.arena, &list, "CntCode");
  }
  if (flags & COFF_SectionFlag_CntInitializedData) {
    str8_list_pushf(scratch.arena, &list, "CntInitializedData");
  }
  if (flags & COFF_SectionFlag_CntUninitializedData) {
    str8_list_pushf(scratch.arena, &list, "CntUninitializedData");
  }
  if (flags & COFF_SectionFlag_LnkOther) {
    str8_list_pushf(scratch.arena, &list, "LnkOther");
  }
  if (flags & COFF_SectionFlag_LnkInfo) {
    str8_list_pushf(scratch.arena, &list, "LnkInfo");
  }
  if (flags & COFF_SectionFlag_LnkCOMDAT) {
    str8_list_pushf(scratch.arena, &list, "LnkCOMDAT");
  }
  if (flags & COFF_SectionFlag_GpRel) {
    str8_list_pushf(scratch.arena, &list, "GpRel");
  }
  if (flags & COFF_SectionFlag_Mem16Bit) {
    str8_list_pushf(scratch.arena, &list, "Mem16Bit");
  }
  if (flags & COFF_SectionFlag_MemLocked) {
    str8_list_pushf(scratch.arena, &list, "MemLocked");
  }
  if (flags & COFF_SectionFlag_MemPreload) {
    str8_list_pushf(scratch.arena, &list, "MemPreload");
  }
  if (flags & COFF_SectionFlag_LnkNRelocOvfl) {
    str8_list_pushf(scratch.arena, &list, "LnkNRelocOvfl");
  }
  if (flags & COFF_SectionFlag_MemDiscardable) {
    str8_list_pushf(scratch.arena, &list, "MemDiscardable");
  }
  if (flags & COFF_SectionFlag_MemNotCached) {
    str8_list_pushf(scratch.arena, &list, "MemNotCached");
  }
  if (flags & COFF_SectionFlag_MemNotPaged) {
    str8_list_pushf(scratch.arena, &list, "MemNotPaged");
  }
  if (flags & COFF_SectionFlag_MemShared) {
    str8_list_pushf(scratch.arena, &list, "MemShared");
  }
  if (flags & COFF_SectionFlag_MemExecute) {
    str8_list_pushf(scratch.arena, &list, "MemExecute");
  }
  if (flags & COFF_SectionFlag_MemRead) {
    str8_list_pushf(scratch.arena, &list, "MemRead");
  }
  if (flags & COFF_SectionFlag_MemWrite) {
    str8_list_pushf(scratch.arena, &list, "MemWrite");
  }
  
  U64 align = coff_align_size_from_section_flags(flags);
  if (align) {
    str8_list_pushf(scratch.arena, &list, "Align=%u", align);
  }
  
  if (!list.node_count) {
    str8_list_pushf(scratch.arena, &list, "None");
  }
  
  StringJoin join = {0};
  join.sep = str8_lit(", ");
  String8 result = str8_list_join(arena, &list, &join);
  
  scratch_end(scratch);
  return result;
}

internal String8
coff_string_from_resource_memory_flags(Arena *arena, COFF_ResourceMemoryFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8List list = {0};
  
  if (flags & COFF_ResourceMemoryFlag_Moveable) {
    flags &= COFF_ResourceMemoryFlag_Moveable;
    str8_list_pushf(scratch.arena, &list, "Moveable");
  }
  if (flags & COFF_ResourceMemoryFlag_Pure) {
    flags &= COFF_ResourceMemoryFlag_Pure;
    str8_list_pushf(scratch.arena, &list, "Pure");
  }
  if (flags & COFF_ResourceMemoryFlag_Discardable) {
    flags &= COFF_ResourceMemoryFlag_Discardable;
    str8_list_pushf(scratch.arena, &list, "Discardable");
  }
  if (flags != 0) {
    str8_list_pushf(scratch.arena, &list, "%#x", flags);
  }
  
  String8 result = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});
  
  scratch_end(scratch);
  return result;
}

internal String8
coff_string_from_import_header_type(COFF_ImportType type)
{
  for (U64 i = 0; i < ArrayCount(g_coff_import_header_type_map); ++i) {
    if (g_coff_import_header_type_map[i].type == type) {
      return str8_cstring(g_coff_import_header_type_map[i].name);
    }
  }
  return str8(0,0);
}

internal String8
coff_string_from_sym_dtype(COFF_SymDType x)
{
  switch (x) {
    case COFF_SymDType_Null:  return str8_lit("Null");
    case COFF_SymDType_Ptr :  return str8_lit("Ptr");
    case COFF_SymDType_Func:  return str8_lit("Func");
    case COFF_SymDType_Array: return str8_lit("Array");
  }
  return str8_zero();
}

internal String8
coff_string_from_sym_type(COFF_SymType x)
{
  switch (x) {
    case COFF_SymType_Null:   return str8_lit("Null");
    case COFF_SymType_Void:   return str8_lit("Void");
    case COFF_SymType_Char:   return str8_lit("Char");
    case COFF_SymType_Short:  return str8_lit("Short");
    case COFF_SymType_Int:    return str8_lit("Int");
    case COFF_SymType_Long:   return str8_lit("Long");
    case COFF_SymType_Float:  return str8_lit("Float");
    case COFF_SymType_Double: return str8_lit("Double");
    case COFF_SymType_Struct: return str8_lit("Struct");
    case COFF_SymType_Union:  return str8_lit("Union");
    case COFF_SymType_Enum:   return str8_lit("Enum");
    case COFF_SymType_MemberOfEnumeration: return str8_lit("MOE");
    case COFF_SymType_Byte:   return str8_lit("Byte");
    case COFF_SymType_Word:   return str8_lit("Word");
    case COFF_SymType_UInt:   return str8_lit("UInt");
    case COFF_SymType_DWord:  return str8_lit("DWord");
  }
  return str8_zero();
}

internal String8
coff_string_from_sym_storage_class(COFF_SymStorageClass x)
{
  switch (x) {
    case COFF_SymStorageClass_Null:            break;
    case COFF_SymStorageClass_EndOfFunction:   return str8_lit("EndOfFunction");
    case COFF_SymStorageClass_Automatic:       return str8_lit("Automatic");
    case COFF_SymStorageClass_External:        return str8_lit("External");
    case COFF_SymStorageClass_Static:          return str8_lit("Static");
    case COFF_SymStorageClass_Register:        return str8_lit("Register");
    case COFF_SymStorageClass_ExternalDef:     return str8_lit("Def");
    case COFF_SymStorageClass_Label:           return str8_lit("Label");
    case COFF_SymStorageClass_UndefinedLabel:  return str8_lit("UndefinedLabel");
    case COFF_SymStorageClass_MemberOfStruct:  return str8_lit("Struct");
    case COFF_SymStorageClass_Argument:        return str8_lit("Argument");
    case COFF_SymStorageClass_StructTag:       return str8_lit("Tag");
    case COFF_SymStorageClass_MemberOfUnion:   return str8_lit("Union");
    case COFF_SymStorageClass_UnionTag:        return str8_lit("Tag");
    case COFF_SymStorageClass_TypeDefinition:  return str8_lit("Definition");
    case COFF_SymStorageClass_UndefinedStatic: return str8_lit("Static");
    case COFF_SymStorageClass_EnumTag:         return str8_lit("Tag");
    case COFF_SymStorageClass_MemberOfEnum:    return str8_lit("Enum");
    case COFF_SymStorageClass_RegisterParam:   return str8_lit("Param");
    case COFF_SymStorageClass_BitField:        return str8_lit("Field");
    case COFF_SymStorageClass_Block:           return str8_lit("Block");
    case COFF_SymStorageClass_Function:        return str8_lit("Function");
    case COFF_SymStorageClass_EndOfStruct:     return str8_lit("Struct");
    case COFF_SymStorageClass_File:            return str8_lit("File");
    case COFF_SymStorageClass_Section:         return str8_lit("Section");
    case COFF_SymStorageClass_WeakExternal:    return str8_lit("External");
    case COFF_SymStorageClass_CLRToken:        return str8_lit("Token");
  }
  return str8_zero();
}

internal String8
coff_string_from_weak_ext_type(COFF_WeakExtType x)
{
  switch (x) {
    case COFF_WeakExt_NoLibrary:     return str8_lit("NoLibrary");
    case COFF_WeakExt_SearchLibrary: return str8_lit("SearchLibrary");
    case COFF_WeakExt_SearchAlias:   return str8_lit("SearchAlias");
  }
  return str8_zero();
}

internal String8
coff_string_from_reloc_x86(COFF_Reloc_X86 x)
{
  switch (x) {
    case COFF_Reloc_X86_Abs:      return str8_lit("Abs");
    case COFF_Reloc_X86_Dir16:    return str8_lit("Dir16");
    case COFF_Reloc_X86_Rel16:    return str8_lit("Rel16");
    case COFF_Reloc_X86_Unknown0: return str8_lit("Unknown0");
    case COFF_Reloc_X86_Unknown2: return str8_lit("Unknown2");
    case COFF_Reloc_X86_Unknown3: return str8_lit("Unknown3");
    case COFF_Reloc_X86_Dir32:    return str8_lit("Dir32");
    case COFF_Reloc_X86_Dir32Nb:  return str8_lit("Dir32Nb");
    case COFF_Reloc_X86_Seg12:    return str8_lit("Seg12");
    case COFF_Reloc_X86_Section:  return str8_lit("Section");
    case COFF_Reloc_X86_SecRel:   return str8_lit("SecRel");
    case COFF_Reloc_X86_Token:    return str8_lit("Token");
    case COFF_Reloc_X86_SecRel7:  return str8_lit("SecRel7");
    case COFF_Reloc_X86_Unknown4: return str8_lit("Unknown4");
    case COFF_Reloc_X86_Unknown5: return str8_lit("Unknown5");
    case COFF_Reloc_X86_Unknown6: return str8_lit("Unknown6");
    case COFF_Reloc_X86_Unknown7: return str8_lit("Unknown7");
    case COFF_Reloc_X86_Unknown8: return str8_lit("Unknown8");
    case COFF_Reloc_X86_Unknown9: return str8_lit("Unknown9");
    case COFF_Reloc_X86_Rel32:    return str8_lit("Rel32");
  }
  return str8_zero();
}

internal String8
coff_string_from_reloc_x64(COFF_Reloc_X64 x)
{
  switch (x) {
    case COFF_Reloc_X64_Abs:      return str8_lit("Abs");
    case COFF_Reloc_X64_Addr64:   return str8_lit("Addr64");
    case COFF_Reloc_X64_Addr32:   return str8_lit("Addr32");
    case COFF_Reloc_X64_Addr32Nb: return str8_lit("Addr32Nb");
    case COFF_Reloc_X64_Rel32:    return str8_lit("Rel32");
    case COFF_Reloc_X64_Rel32_1:  return str8_lit("Rel32_1");
    case COFF_Reloc_X64_Rel32_2:  return str8_lit("Rel32_2");
    case COFF_Reloc_X64_Rel32_3:  return str8_lit("Rel32_3");
    case COFF_Reloc_X64_Rel32_4:  return str8_lit("Rel32_4");
    case COFF_Reloc_X64_Rel32_5:  return str8_lit("Rel32_5");
    case COFF_Reloc_X64_Section:  return str8_lit("Section");
    case COFF_Reloc_X64_SecRel:   return str8_lit("SecRel");
    case COFF_Reloc_X64_SecRel7:  return str8_lit("SecRel7");
    case COFF_Reloc_X64_Token:    return str8_lit("Token");
    case COFF_Reloc_X64_SRel32:   return str8_lit("SRel32");
    case COFF_Reloc_X64_Pair:     return str8_lit("Pair");
    case COFF_Reloc_X64_SSpan32:  return str8_lit("SSpan32");
  }
  return str8_zero();
}

internal String8
coff_string_from_reloc_arm(COFF_Reloc_Arm x)
{
  switch (x) {
    case COFF_Reloc_Arm_Abs:           return str8_lit("Abs");
    case COFF_Reloc_Arm_Addr32:        return str8_lit("Addr32");
    case COFF_Reloc_Arm_Addr32Nb:      return str8_lit("Addr32Nb");
    case COFF_Reloc_Arm_Branch24:      return str8_lit("Branch24");
    case COFF_Reloc_Arm_Branch11:      return str8_lit("Branch11");
    case COFF_Reloc_Arm_Unknown1:      return str8_lit("Unknown1");
    case COFF_Reloc_Arm_Unknown2:      return str8_lit("Unknown2");
    case COFF_Reloc_Arm_Unknown3:      return str8_lit("Unknown3");
    case COFF_Reloc_Arm_Unknown4:      return str8_lit("Unknown4");
    case COFF_Reloc_Arm_Unknown5:      return str8_lit("Unknown5");
    case COFF_Reloc_Arm_Rel32:         return str8_lit("Rel32");
    case COFF_Reloc_Arm_Section:       return str8_lit("Section");
    case COFF_Reloc_Arm_SecRel:        return str8_lit("SecRel");
    case COFF_Reloc_Arm_Mov32:         return str8_lit("Mov32");
    case COFF_Reloc_Arm_ThumbMov32:    return str8_lit("ThumbMov32");
    case COFF_Reloc_Arm_ThumbBranch20: return str8_lit("ThumbBranch20");
    case COFF_Reloc_Arm_Unused:        return str8_lit("Unused");
    case COFF_Reloc_Arm_ThumbBranch24: return str8_lit("ThumbBranch24");
    case COFF_Reloc_Arm_ThumbBlx23:    return str8_lit("ThumbBlx23");
    case COFF_Reloc_Arm_Pair:          return str8_lit("Pair");
  }
  return str8_zero();
}

internal String8
coff_string_from_reloc_arm64(COFF_Reloc_Arm64 x)
{
  switch (x) {
    case COFF_Reloc_Arm64_Abs:           return str8_lit("Abs");
    case COFF_Reloc_Arm64_Addr32:        return str8_lit("Addr32");
    case COFF_Reloc_Arm64_Addr32Nb:      return str8_lit("Addr32Nb");
    case COFF_Reloc_Arm64_Branch26:      return str8_lit("Branch26");
    case COFF_Reloc_Arm64_PageBaseRel21: return str8_lit("PageBaseRel21");
    case COFF_Reloc_Arm64_Rel21:         return str8_lit("Rel21");
    case COFF_Reloc_Arm64_PageOffset12a: return str8_lit("PageOffset12a");
    case COFF_Reloc_Arm64_SecRel:        return str8_lit("SecRel");
    case COFF_Reloc_Arm64_SecRelLow12a:  return str8_lit("SecRelLow12a");
    case COFF_Reloc_Arm64_SecRelHigh12a: return str8_lit("SecRelHigh12a");
    case COFF_Reloc_Arm64_SecRelLow12l:  return str8_lit("SecRelLow12l");
    case COFF_Reloc_Arm64_Token:         return str8_lit("Token");
    case COFF_Reloc_Arm64_Section:       return str8_lit("Section");
    case COFF_Reloc_Arm64_Addr64:        return str8_lit("Addr64");
    case COFF_Reloc_Arm64_Branch19:      return str8_lit("Branch19");
    case COFF_Reloc_Arm64_Branch14:      return str8_lit("Branch14");
    case COFF_Reloc_Arm64_Rel32:         return str8_lit("Rel32");
  }
  return str8_zero();
}

internal String8
coff_string_from_reloc(COFF_MachineType machine, COFF_RelocType x)
{
  switch (machine) {
    case COFF_MachineType_X86:   return coff_string_from_reloc_x86(x);
    case COFF_MachineType_X64:   return coff_string_from_reloc_x64(x);
    case COFF_MachineType_Arm:   return coff_string_from_reloc_arm(x);
    case COFF_MachineType_Arm64: return coff_string_from_reloc_arm64(x);
  }
  return str8_zero();
}

internal COFF_MachineType
coff_machine_from_string(String8 string)
{
  for (U64 i = 0; i < ArrayCount(g_coff_machine_map); ++i) {
    if (str8_match(g_coff_machine_map[i].string, string, StringMatchFlag_CaseInsensitive)) {
      return g_coff_machine_map[i].machine;
    }
  }
  return COFF_MachineType_Unknown;
}

internal COFF_ImportType
coff_import_header_type_from_string(String8 name)
{
  for (U64 i = 0; i < ArrayCount(g_coff_import_header_type_map); ++i) {
    if (str8_match(str8_cstring(g_coff_import_header_type_map[i].name), name, StringMatchFlag_CaseInsensitive)) {
      return g_coff_import_header_type_map[i].type;
    }
  }
  return COFF_ImportType_Invalid;
}

