// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- rjf: top-level binary parsing

internal ELF_Bin
elf_bin_from_data(Arena *arena, String8 data)
{
  ELF_Bin bin = {0};
  if(str8_match(str8_prefix(data, elf_magic_string.size), elf_magic_string, 0) &&
     data.size >= ELF_Identifier_Max)
  {
    //- rjf: parse sig/header
    U8 sig[ELF_Identifier_Max] = {0};
    str8_deserial_read(data, 0, &sig[0], sizeof(sig), 1);
    switch(sig[ELF_Identifier_Class])
    {
      default:
      case ELF_Class_None:{}break;
      case ELF_Class_32:
      {
        ELF_Hdr32 hdr32 = {0};
        U64 hdr_size = str8_deserial_read_struct(data, 0, &hdr32);
        if(hdr_size == sizeof(hdr32))
        {
          bin.hdr = elf_hdr64_from_hdr32(hdr32);
          U64 shstr_off = hdr32.e_shoff + hdr32.e_shentsize*hdr32.e_shstrndx;
          ELF_Shdr32 shdr = {0};
          U64 shdr_size = str8_deserial_read_struct(data, shstr_off, &shdr);
          if(shdr_size == sizeof(shdr))
          {
            bin.sh_name_range = rng_1u64(shdr.sh_offset, shdr.sh_offset + shdr.sh_size);
          }
        }
      }break;
      case ELF_Class_64:
      {
        ELF_Hdr64 hdr64 = {0};
        U64 hdr_size = str8_deserial_read_struct(data, 0, &hdr64);
        if(hdr_size == sizeof(hdr64))
        {
          bin.hdr = hdr64;
          U64 shstr_off = hdr64.e_shoff + hdr64.e_shentsize*hdr64.e_shstrndx;
          ELF_Shdr64 shdr = {0};
          U64 shdr_size = str8_deserial_read_struct(data, shstr_off, &shdr);
          if(shdr_size == sizeof(shdr))
          {
            bin.sh_name_range = rng_1u64(shdr.sh_offset, shdr.sh_offset + shdr.sh_size);
          }
        }
      }break;
    }
    
    //- rjf: gather all shdrs
    {
      ELF_Hdr64 *hdr = &bin.hdr;
      bin.shdrs.count = hdr->e_shnum;
      bin.shdrs.v = push_array(arena, ELF_Shdr64, hdr->e_shnum);
      Rng1U64 shdr_range = rng_1u64(hdr->e_shoff, hdr->e_shoff + hdr->e_shentsize*hdr->e_shnum);
      String8 shdr_data = str8_substr(data, shdr_range);
      for EachIndex(shdr_idx, hdr->e_shnum)
      {
        switch(hdr->e_ident[ELF_Identifier_Class])
        {
          default:
          case ELF_Class_None:
          {}break;
          case ELF_Class_32:
          {
            ELF_Shdr32 shdr32 = {0};
            str8_deserial_read_struct(shdr_data, shdr_idx * sizeof(ELF_Shdr32), &shdr32);
            bin.shdrs.v[shdr_idx] = elf_shdr64_from_shdr32(shdr32);
          }break;
          case ELF_Class_64:
          {
            str8_deserial_read_struct(shdr_data, shdr_idx * sizeof(ELF_Shdr64), &bin.shdrs.v[shdr_idx]);
          }break;
        }
      }
    }
    
    //- rjf: gather all phdrs
    {
      ELF_Hdr64 *hdr = &bin.hdr;
      bin.phdrs.count = hdr->e_phnum;
      bin.phdrs.v = push_array(arena, ELF_Phdr64, hdr->e_phnum);
      Rng1U64 phdr_range = rng_1u64(hdr->e_phoff, hdr->e_phoff + hdr->e_phentsize*hdr->e_phnum);
      String8 phdr_data = str8_substr(data, phdr_range);
      for EachIndex(phdr_idx, hdr->e_phnum)
      {
        switch(hdr->e_ident[ELF_Identifier_Class])
        {
          default:
          case ELF_Class_None:
          {}break;
          case ELF_Class_32:
          {
            ELF_Phdr32 phdr32 = {0};
            str8_deserial_read_struct(phdr_data, phdr_idx * sizeof(ELF_Phdr32), &phdr32);
            bin.phdrs.v[phdr_idx] = elf_phdr64_from_phdr32(phdr32);
          }break;
          case ELF_Class_64:
          {
            str8_deserial_read_struct(phdr_data, phdr_idx * sizeof(ELF_Phdr64), &bin.phdrs.v[phdr_idx]);
          }break;
        }
      }
    }
  }
  return bin;
}

//- rjf: extra bin info extraction

internal String8
elf_name_from_shdr64(String8 data, ELF_Bin *bin, ELF_Shdr64 *shdr)
{
  String8 sh_names = str8_substr(data, bin->sh_name_range);
  String8 name = {0};
  str8_deserial_read_cstr(sh_names, shdr->sh_name, &name);
  return name;
}

internal U64
elf_base_addr_from_bin(ELF_Bin *bin)
{
  U64 base_vaddr = 0;
  for EachIndex(phdr_idx, bin->phdrs.count)
  {
    ELF_Phdr64 *phdr = &bin->phdrs.v[phdr_idx];
    if(phdr->p_type == ELF_PType_Load &&
       (base_vaddr == 0 || phdr->p_vaddr < base_vaddr))
    {
      base_vaddr = phdr->p_vaddr;
    }
  }
  return base_vaddr;
}

internal ELF_GnuDebugLink
elf_gnu_debug_link_from_bin(String8 raw_data, ELF_Bin *bin)
{
  ELF_GnuDebugLink result = {0};
  for EachIndex(idx, bin->shdrs.count)
  {
    ELF_Shdr64 *shdr = &bin->shdrs.v[idx];
    String8 name = elf_name_from_shdr64(raw_data, bin, shdr);
    if(str8_match(name, str8_lit(".gnu_debuglink"), 0))
    {
      Rng1U64 raw_data_range = rng_1u64(shdr->sh_offset, shdr->sh_offset + shdr->sh_size);
      String8 data = str8_substr(raw_data, raw_data_range);
      String8 path = {0};
      U32 checksum = 0;
      {
        U64 cursor = 0;
        cursor += str8_deserial_read_cstr(data, cursor, &path);
        cursor = AlignPow2(cursor, 4);
        cursor += str8_deserial_read_struct(data, cursor, &checksum);
      }
      result.path = path;
      result.checksum = checksum;
      break;
    }
  }
  return result;
}
