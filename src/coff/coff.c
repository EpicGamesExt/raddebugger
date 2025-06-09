// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U64
coff_align_size_from_section_flags(COFF_SectionFlags flags)
{
  U32 align = 0;
  U32 align_index = COFF_SectionFlags_ExtractAlign(flags);
  switch (align_index) {
    default: break;
    case COFF_SectionAlign_None:      align = 0;    break;
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
