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

