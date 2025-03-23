// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
dw_is_dwarf_present_elf_section_table(String8 raw_image, ELF_BinInfo *bin)
{
  Temp scratch = scratch_begin(0,0);

  B32 is_dwarf_present = 0;

  ELF_Shdr64Array sections = elf_shdr64_array_from_bin(scratch.arena, raw_image, &bin->hdr);

  for (U64 i = 0; i < sections.count; ++i) {
    ELF_Shdr64 *shdr = &sections.v[i];
    String8     name = elf_name_from_shdr64(raw_image, &bin->hdr, bin->sh_name_range, shdr);

    if (shdr->sh_type != ELF_SectionCode_ProgBits) {
      continue;
    }

    DW_SectionKind s = dw_section_kind_from_string(name);
    if (s == DW_Section_Null) {
      s = dw_section_dwo_kind_from_string(name);
    }

    is_dwarf_present = s != DW_Section_Null;
    if (is_dwarf_present) {
      break;
    }
  }

  scratch_end(scratch);
  return is_dwarf_present;
}

internal DW_Input
dw_input_from_elf_section_table(Arena *arena, String8 raw_image, ELF_BinInfo *bin)
{
  Temp scratch = scratch_begin(&arena, 1);

  DW_Input result                              = {0};
  B32      sect_status[ArrayCount(result.sec)] = {0};

  ELF_Shdr64Array sections = elf_shdr64_array_from_bin(scratch.arena, raw_image, &bin->hdr);

  for (U64 sect_idx = 1; sect_idx < sections.count; ++sect_idx) {
    ELF_Shdr64 *shdr = &sections.v[sect_idx];

    // skip BSS sections
    if (shdr->sh_type != ELF_SectionCode_ProgBits) {
      continue;
    }

    String8 name = elf_name_from_shdr64(raw_image, &bin->hdr, bin->sh_name_range, shdr);

    DW_SectionKind s      = dw_section_kind_from_string(name);
    B32            is_dwo = 0;
    if (s == DW_Section_Null) {
      s      = dw_section_dwo_kind_from_string(name);
      is_dwo = 1;
    }

    if (s != DW_Section_Null) {
      if (sect_status[s]) {
        Assert(!"too many debug sections with identical name, picking first");
      } else {
        Rng1U64 raw_data_range = rng_1u64(shdr->sh_offset, shdr->sh_offset + shdr->sh_size);
        String8 data           = str8_substr(raw_image, raw_data_range);

        // ELF was compiled with compressed debug info
        if (shdr->sh_flags & ELF_Shf_Compressed) {
          String8 comp_data_with_header = data; 

          // read header
          ELF_Chdr64 chdr64    = {0};
          U64        chdr_size = 0;
          if (ELF_HdrIs64Bit(bin->hdr.e_ident)) {
            chdr_size = str8_deserial_read_struct(comp_data_with_header, 0, &chdr64);
            if (chdr_size != sizeof(chdr64)) {
              Assert(!"not enough bytes to read header");
            }
          } else if (ELF_HdrIs32Bit(bin->hdr.e_ident)) {
            ELF_Chdr32 chdr32 = {0};
            chdr_size = str8_deserial_read_struct(comp_data_with_header, 0, &chdr32);
            if (chdr_size == sizeof(chdr32)) {
              chdr64 = elf_chdr64_from_chdr32(chdr32);
            }
          }

          AssertAlways(IsPow2(chdr64.ch_addr_align));

          // skip header
          String8 comp_data = str8_skip(comp_data_with_header, chdr_size);

          // push buffer for the decompressor
          U8  *decomp_buffer      = push_array_no_zero_aligned(arena, U8, chdr64.ch_size, chdr64.ch_addr_align);
          U64  actual_decomp_size = 0;
          
          // decompress
          switch (chdr64.ch_type) {
          case ELF_CompressType_None: {
            AssertAlways(!"unexpected compression type");
          } break;
          case ELF_CompressType_ZLib: {
            actual_decomp_size = zsinflate(decomp_buffer, chdr64.ch_size, comp_data.str, comp_data.size);
          } break;
          case ELF_CompressType_ZStd: {
            // TODO: zstd lib
            NotImplemented;
          } break;
          default: InvalidPath; break;
          }

          // TODO: error handling
          AssertAlways(actual_decomp_size == chdr64.ch_size);

          // set decompressed section data
          data = str8(decomp_buffer, actual_decomp_size);
        }

        sect_status[s] = 1;
        DW_Section *d = &result.sec[s];
        d->name       = push_str8_copy(arena, name);
        d->data       = data;
        d->is_dwo     = is_dwo;
      }
    }
  }

  scratch_end(scratch);
  return result;
}
