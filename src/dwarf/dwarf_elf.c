// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
dw_is_dwarf_present_from_elf_bin(String8 data, ELF_Bin *bin)
{
  B32 is_dwarf_present = 0;
  for EachIndex(idx, bin->shdrs.count)
  {
    ELF_Shdr64 *shdr = &bin->shdrs.v[idx];
    if(shdr->sh_type != ELF_SectionCode_ProgBits) { continue; }
    String8 name = elf_name_from_shdr64(data, bin, shdr);
    DW_SectionKind s = dw_section_kind_from_string(name);
    if(s == DW_Section_Null)
    {
      s = dw_section_dwo_kind_from_string(name);
    }
    is_dwarf_present = (s != DW_Section_Null);
    if(is_dwarf_present)
    {
      break;
    }
  }
  return is_dwarf_present;
}

#define SINFL_IMPLEMENTATION
#include "third_party/sinfl/sinfl.h"

internal DW_Input
dw_input_from_elf_bin(Arena *arena, String8 data, ELF_Bin *bin)
{
  DW_Input result = {0};
  B32 is_section_present[ArrayCount(result.sec)] = {0};
  Temp scratch = scratch_begin(&arena, 1);
  for(U64 section_idx = 1; section_idx < bin->shdrs.count; section_idx += 1)
  {
    ELF_Shdr64 *shdr = &bin->shdrs.v[section_idx];
    if(shdr->sh_type != ELF_SectionCode_ProgBits) { continue; } // skip BSS sections
    
    //- rjf: unpack section
    String8 section_name = elf_name_from_shdr64(data, bin, shdr);
    DW_SectionKind section_kind = dw_section_kind_from_string(section_name);
    String8 section_data__maybe_compressed = str8_substr(data, r1u64(shdr->sh_offset, shdr->sh_offset + shdr->sh_size));
    B32 is_dwo = 0;
    if(section_kind == DW_Section_Null)
    {
      section_kind = dw_section_dwo_kind_from_string(section_name);
      is_dwo = 1;
    }
    
    //- rjf: decompress section data if needed
    String8 section_data__uncompressed = {0};
    if(!(shdr->sh_flags & ELF_Shf_Compressed))
    {
      section_data__uncompressed = section_data__maybe_compressed;
    }
    else
    {
      // rjf: read compressed-section header
      ELF_Chdr64 chdr64 = {0};
      U64 chdr_size = 0;
      if(ELF_HdrIs64Bit(bin->hdr.e_ident))
      {
        chdr_size = str8_deserial_read_struct(section_data__maybe_compressed, 0, &chdr64);
      }
      else if(ELF_HdrIs32Bit(bin->hdr.e_ident))
      {
        ELF_Chdr32 chdr32 = {0};
        chdr_size = str8_deserial_read_struct(section_data__maybe_compressed, 0, &chdr32);
        if(chdr_size == sizeof(chdr32))
        {
          chdr64 = elf_chdr64_from_chdr32(chdr32);
        }
      }
      
      // rjf: decompress
      {
        String8 section_data__compressed_contents = str8_skip(section_data__maybe_compressed, chdr_size);
        switch(chdr64.ch_type)
        {
          default:
          case ELF_CompressType_None:
          {
            section_data__uncompressed = section_data__compressed_contents;
          }break;
          case ELF_CompressType_ZLib:
          {
            U8 *section_data_uncompressed_buffer = push_array_no_zero_aligned(arena, U8, chdr64.ch_size, chdr64.ch_addr_align);
            U64 section_data_uncompressed_size = 0;
            section_data_uncompressed_size = zsinflate(section_data_uncompressed_buffer, chdr64.ch_size, section_data__compressed_contents.str, section_data__compressed_contents.size);
            section_data__uncompressed = str8(section_data_uncompressed_buffer, section_data_uncompressed_size);
          }break;
          case ELF_CompressType_ZStd:
          {
            NotImplemented;
          }break;
        }
      }
    }
    
    //- rjf: store
    {
      is_section_present[section_kind] = 1;
      DW_Section *d = &result.sec[section_kind];
      d->name   = push_str8_copy(arena, section_name);
      d->data   = section_data__uncompressed;
      d->is_dwo = is_dwo;
    }
  }
  scratch_end(scratch);
  return result;
}
