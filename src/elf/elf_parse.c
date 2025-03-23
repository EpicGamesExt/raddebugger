// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
elf_check_magic(String8 data)
{
  U8 sig[ELF_Identifier_Max] = {0};
  str8_deserial_read(data, 0, &sig[0], sizeof(sig), 1);
  B32 is_magic_valid = (sig[ELF_Identifier_Mag0] == 0x7f && sig[ELF_Identifier_Mag1] == 'E'  && sig[ELF_Identifier_Mag2] == 'L'  && sig[ELF_Identifier_Mag3] == 'F');
  return is_magic_valid;
}

internal ELF_BinInfo
elf_bin_from_data(String8 data)
{
  ELF_Hdr64 hdr64         = {0};
  Rng1U64   sh_name_range = rng_1u64(0,0);

  if (elf_check_magic(data)) {
    U8 sig[ELF_Identifier_Max] = {0};
    str8_deserial_read(data, 0, &sig[0], sizeof(sig), 1);

    switch (sig[ELF_Identifier_Class]) {
    case ELF_Class_None: break;
    case ELF_Class_32: {
      ELF_Hdr32 hdr32    = {0};
      U64       hdr_size = str8_deserial_read_struct(data, 0, &hdr32);
      if (hdr_size == sizeof(hdr32)) {
        hdr64  = elf_hdr64_from_hdr32(hdr32);

        U64        shstr_off = hdr32.e_shoff + hdr32.e_shentsize*hdr32.e_shstrndx;
        ELF_Shdr32 shdr      = {0};
        U64        shdr_size = str8_deserial_read_struct(data, shstr_off, &shdr);

        if (shdr_size == sizeof(shdr)) {
          sh_name_range = rng_1u64(shdr.sh_offset, shdr.sh_offset + shdr.sh_size);
        }
      }
    } break;
    case ELF_Class_64: {
      U64 hdr_size = str8_deserial_read_struct(data, 0, &hdr64);
      if (hdr_size == sizeof(hdr64)) {
        U64        shstr_off = hdr64.e_shoff + hdr64.e_shentsize*hdr64.e_shstrndx;
        ELF_Shdr64 shdr      = {0};
        U64        shdr_size = str8_deserial_read_struct(data, shstr_off, &shdr);

        if (shdr_size == sizeof(shdr)) {
          sh_name_range = rng_1u64(shdr.sh_offset, shdr.sh_offset + shdr.sh_size);
        }
      }
    } break;
    default: Assert(!"invalid elf header"); break;
    } 
  }
  
  ELF_BinInfo info   = {0};
  info.hdr           = hdr64;
  info.sh_name_range = sh_name_range;
  
  return info;
}

internal ELF_Shdr64Array
elf_shdr64_array_from_bin(Arena *arena, String8 raw_data, ELF_Hdr64 *hdr)
{
  Rng1U64 shdr_range = rng_1u64(hdr->e_shoff, hdr->e_shoff + hdr->e_shentsize*hdr->e_shnum);
  String8 shdr_data  = str8_substr(raw_data, shdr_range);
  
  ELF_Shdr64Array result = {0};
  result.count           = hdr->e_shnum;
  result.v               = push_array(arena, ELF_Shdr64, hdr->e_shnum);
  
  for(U64 shdr_idx = 0; shdr_idx < hdr->e_shnum; ++shdr_idx) {
    switch (hdr->e_ident[ELF_Identifier_Class]) {
    case ELF_Class_None: break;
    case ELF_Class_32: {
      ELF_Shdr32 shdr32 = {0};
      str8_deserial_read_struct(shdr_data, shdr_idx * sizeof(ELF_Shdr32), &shdr32);
      result.v[shdr_idx] = elf_shdr64_from_shdr32(shdr32);
    } break;
    case ELF_Class_64: {
      str8_deserial_read_struct(shdr_data, shdr_idx * sizeof(ELF_Shdr64), &result.v[shdr_idx]);
    } break;
    default: InvalidPath; break;
    }
  }
  
  return result;
}

internal String8
elf_name_from_shdr64(String8 raw_data, ELF_Hdr64 *hdr, Rng1U64 sh_name_range, ELF_Shdr64 *shdr)
{
  String8 sh_names = str8_substr(raw_data, sh_name_range);
  String8 name = {0};
  str8_deserial_read_cstr(sh_names, shdr->sh_name, &name);
  return name;
}

internal U64
elf_base_addr_from_bin(ELF_Hdr64 *hdr)
{
  NotImplemented;
  return 0;
}

internal B32
elf_parse_debug_link(String8 raw_data, ELF_BinInfo *elf, ELF_GnuDebugLink *debug_link_out)
{
  Temp scratch = scratch_begin(0,0);

  B32             is_debug_link_present = 0;
  ELF_Shdr64Array sections              = elf_shdr64_array_from_bin(scratch.arena, raw_data, &elf->hdr);
  for (U64 i = 0; i < sections.count; ++i) {
    ELF_Shdr64 *shdr = &sections.v[i];
    String8     name = elf_name_from_shdr64(raw_data, &elf->hdr, elf->sh_name_range, shdr);

    if (str8_match(name, str8_lit(".gnu_debuglink"), 0)) {
      Rng1U64 raw_data_range = rng_1u64(shdr->sh_offset, shdr->sh_offset + shdr->sh_size);
      String8 data           = str8_substr(raw_data, raw_data_range);

      String8 path     = {0};
      U32     checksum = 0;
      {
        U64 cursor = 0;
        cursor += str8_deserial_read_cstr(data, cursor, &path);

        cursor = AlignPow2(cursor, 4);
        cursor += str8_deserial_read_struct(data, cursor, &checksum);
      }

      debug_link_out->path     = path;
      debug_link_out->checksum = checksum;

      is_debug_link_present = 1;
      break;
    }
  }

  scratch_end(scratch);
  return is_debug_link_present;
}

