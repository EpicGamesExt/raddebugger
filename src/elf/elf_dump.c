// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8List
elf_dump_note(Arena *arena, String8 raw_notes, ELF_Class elf_class, ELF_MachineKind e_machine)
{
  Temp scratch = scratch_begin(&arena, 1);

  B32         is_bad_parse = 1;
  String8List strings      = {0};
  U64         cursor       = 0;

  for (;cursor < raw_notes.size;) {
    U32 owner_size;
    U64 owner_size_size = str8_deserial_read_struct(raw_notes, cursor, &owner_size);
    if (owner_size_size == 0) { goto exit; }
    cursor += owner_size_size;

    U32 desc_size;
    U64 desc_size_size = str8_deserial_read_struct(raw_notes, cursor, &desc_size);
    if (desc_size_size == 0) { goto exit; }
    cursor += desc_size_size;

    ELF_NoteType note_type;
    U64 type_size = str8_deserial_read_struct(raw_notes, cursor, &note_type);
    if (type_size == 0) { goto exit; }
    cursor += type_size;

    if (cursor + owner_size > raw_notes.size) { goto exit; }
    String8 owner = str8_cstring_capped(raw_notes.str + cursor, raw_notes.str + cursor + owner_size);
    cursor += owner_size;

    if (cursor + desc_size > raw_notes.size) { goto exit; }
    String8 raw_desc = str8_substr(raw_notes, r1u64(cursor, cursor + desc_size));
    cursor += desc_size;
    cursor = AlignPow2(cursor, 4);

    String8List desc_fmt      = {0};
    String8     note_type_str = {0};
    if (str8_match(owner, str8_lit("GNU"), StringMatchFlag_CaseInsensitive)) {
      // format description
      switch (note_type) {
      case GNU_NoteType_Abi: {
        U64 desc_cursor = 0;

        GNU_AbiTag os = 0;
        U64 os_size = str8_deserial_read_struct(raw_desc, desc_cursor, &os);
        if (os_size == 0) { goto exit; }
        cursor += os_size;

        U32 major = 0;
        U64 major_size = str8_deserial_read_struct(raw_desc, desc_cursor, &major);
        if (major_size == 0) { goto exit; }
        cursor += major_size;

        U32 minor = 0;
        U64 minor_size = str8_deserial_read_struct(raw_desc, desc_cursor, &minor);
        if (minor_size == 0) { goto exit; }
        cursor += minor_size;

        U32 sub_minor = 0;
        U64 sub_minor_size = str8_deserial_read_struct(raw_desc, desc_cursor, &sub_minor);
        if (sub_minor_size == 0) { goto exit; }
        cursor += sub_minor_size;

        String8 os_str = gnu_string_from_abi_tag(os);
        if (os_str.size == 0) os_str = str8f(scratch.arena, "0x%x", os);

        str8_list_pushf(scratch.arena, &desc_fmt, "OS: %S, ABI: %u.%u.%u", os_str, major, minor, sub_minor);
      } break;
      case GNU_NoteType_BuildId: {
        String8List build_id = {0};
        for EachIndex(desc_cursor, desc_size) {
          U8  v      = 0;
          U64 v_size = str8_deserial_read_struct(raw_desc, desc_cursor, &v);
          if (v_size == 0) { goto exit; }
          desc_cursor += v_size;
          str8_list_pushf(scratch.arena, &build_id, "%02x", v);
        }
        String8 build_id_str = str8_list_join(scratch.arena, &build_id, 0);
        str8_list_pushf(scratch.arena, &desc_fmt, "Build ID: %S", build_id_str);
      } break;
      case GNU_NoteType_PropertyType0: {
        U64 align = elf_class == ELF_Class_64 ? 8 : 4;
        for (U64 desc_cursor = 0; desc_cursor < raw_desc.size; ) {
          GNU_Property type = 0;
          U64 type_size = str8_deserial_read_struct(raw_desc, desc_cursor, &type);
          if (type_size == 0) { goto exit; }
          desc_cursor += type_size;

          U32 size = 0;
          U64 size_size = str8_deserial_read_struct(raw_desc, desc_cursor, &size);
          if (size_size == 0) { goto exit; }
          desc_cursor += size_size;

          U32 flags = 0;
          if (size == 4) {
            U64 flags_size = str8_deserial_read_struct(raw_desc, desc_cursor, &flags);
            if (flags_size == 0) { goto exit; }
            desc_cursor += flags_size;
          }

          switch (e_machine) {
          case ELF_MachineKind_None: break;
          case ELF_MachineKind_X86_64: {
            String8 features = gnu_string_from_property_flags_x86(scratch.arena, type, flags);
            str8_list_pushf(scratch.arena, &desc_fmt, "x86 features: %S", features);
          } break;
          default: NotImplemented; break;
          }

          desc_cursor = AlignPow2(desc_cursor, align);
        }
      } break;
      default: NotImplemented; break;
      }

      note_type_str = gnu_string_from_note_type(note_type);
    } else if (str8_match(owner, str8_lit("stapsdt"), StringMatchFlag_CaseInsensitive)) {
      if (note_type == ELF_NoteType_STapSdt) {
        U64 desc_cursor = 0;
        U64 addr_size   = elf_class == ELF_Class_64 ? 8 : 4;

        U64 pc = 0;
        U64 pc_size = str8_deserial_read(raw_desc, desc_cursor, &pc, addr_size, addr_size);
        if (pc_size == 0) { goto exit; }
        desc_cursor += pc_size;

        U64 base_addr = 0;
        U64 base_addr_size = str8_deserial_read(raw_desc, desc_cursor, &base_addr, addr_size, addr_size);
        if (base_addr_size == 0) { goto exit; }
        desc_cursor += base_addr_size;

        U64 semaphore = 0;
        U64 semaphore_size = str8_deserial_read(raw_desc, desc_cursor, &semaphore, addr_size, addr_size);
        if (semaphore_size == 0) { goto exit; }
        desc_cursor += semaphore_size;

        String8 provider = str8_cstring_capped(raw_desc.str + desc_cursor, raw_desc.str + raw_desc.size);
        desc_cursor += provider.size + 1;
        if (desc_cursor > raw_desc.size) { goto exit; }

        String8 probe = str8_cstring_capped(raw_desc.str + desc_cursor, raw_desc.str + raw_desc.size);
        desc_cursor += probe.size + 1;
        if (desc_cursor > raw_desc.size) { goto exit; }

        String8 args = str8_cstring_capped(raw_desc.str + desc_cursor, raw_desc.str + raw_desc.size);
        desc_cursor += args.size + 1;
        if (desc_cursor > raw_desc.size) { goto exit; }

        str8_list_pushf(scratch.arena, &desc_fmt, "Provider:  %S",      provider);
        str8_list_pushf(scratch.arena, &desc_fmt, "Probe:     %S",      probe);
        str8_list_pushf(scratch.arena, &desc_fmt, "PC:        0x%I64x", pc);
        str8_list_pushf(scratch.arena, &desc_fmt, "Base:      0x%I64x", base_addr);
        str8_list_pushf(scratch.arena, &desc_fmt, "Semaphore: 0x%I64x", semaphore);
        str8_list_pushf(scratch.arena, &desc_fmt, "Arguments: %S",      args);

        note_type_str = str8_lit("NT_STAPSDT");
      }
    }

    if (note_type_str.size == 0) note_type_str = str8f(scratch.arena, "0x%x", note_type);

    str8_list_pushf(arena, &strings, "{");
    str8_list_pushf(arena, &strings, "  Owner:     %S",   owner);
    str8_list_pushf(arena, &strings, "  Data Size: 0x%x", desc_size);
    str8_list_pushf(arena, &strings, "  Type:      %S",   note_type_str);
    if (desc_fmt.node_count) {
      str8_list_pushf(arena, &strings, "  Description:");
      str8_list_pushf(arena, &strings, "  {");
      for EachNode(n, String8Node, desc_fmt.first) { str8_list_pushf(arena, &strings, "    %S", n->string); }
      str8_list_pushf(arena, &strings, "  }");
    }
    str8_list_pushf(arena, &strings, "}");
  }

  is_bad_parse = 0;
exit:;
  if (is_bad_parse) {
    str8_list_pushf(arena, &strings, "ERROR: unable to parse data @ 0x%Ix64", cursor);
  }
  scratch_end(scratch);
  return strings;
}

internal String8List
elf_dump(Arena *arena, String8 raw_elf, ELF_DumpSubsetFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List strings = {0};
  ELF_Bin elf = elf_bin_from_data(scratch.arena, raw_elf);

  if (flags & ELF_DumpSubsetFlag_Note) {
    for EachIndex(sect_idx, elf.shdrs.count) {
      ELF_Shdr64 *shdr = &elf.shdrs.v[sect_idx];
      if (shdr->sh_type == ELF_ShType_Note) {
        String8 raw_notes = str8_substr(raw_elf, r1u64(shdr->sh_offset, shdr->sh_offset + shdr->sh_size));
        String8 shdr_name = elf_name_from_shdr64(raw_elf, &elf, shdr);
        str8_list_pushf(scratch.arena, &strings, "//");
        str8_list_pushf(scratch.arena, &strings, "// %S", shdr_name);
        str8_list_pushf(scratch.arena, &strings, "//");
        String8List note_strings = elf_dump_note(scratch.arena, raw_notes, elf.hdr.e_ident[ELF_Identifier_Class], elf.hdr.e_machine);
        str8_list_concat_in_place(&strings, &note_strings);
      }
    }
  }

  String8 out = str8_list_join(arena, &strings, &(StringJoin){.sep=str8_lit("\n"), .post=str8_lit("\n")});
  String8List result = {0};
  str8_list_push(arena, &result, out);

  scratch_end(scratch);
  return result;
}
