// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ELF_PARSE_H
#define ELF_PARSE_H

////////////////////////////////

typedef struct ELF_BinInfo
{
  ELF_Hdr64 hdr;
  Rng1U64   sh_name_range;
} ELF_BinInfo;

typedef struct ELF_Shdr64Array
{
  U64         count;
  ELF_Shdr64 *v;
} ELF_Shdr64Array;

typedef struct ELF_GnuDebugLink
{
  String8 path;
  U32     checksum;
} ELF_GnuDebugLink;

////////////////////////////////

internal B32             elf_check_magic(String8 data);
internal ELF_BinInfo     elf_bin_from_data(String8 data);

internal ELF_Shdr64Array elf_shdr64_array_from_bin(Arena *arena, String8 raw_data, ELF_Hdr64 *hdr);
internal String8         elf_name_from_shdr64(String8 raw_data, ELF_Hdr64 *hdr, Rng1U64 sh_name_range, ELF_Shdr64 *shdr);
internal U64             elf_base_addr_from_bin(ELF_Hdr64 *hdr);

#endif // ELF_PARSE_H
