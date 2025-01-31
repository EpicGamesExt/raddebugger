// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal DW_SectionArray
dw_sections_from_coff_section_table(Arena              *arena,
                                    String8             raw_image,
                                    U64                 string_table_off,
                                    U64                 section_count,
                                    COFF_SectionHeader *sections)
{
  DW_SectionArray result                            = {0};
  B32             sect_status[ArrayCount(result.v)] = {0};

  for (U64 i = 0; i < section_count; ++i) {
    COFF_SectionHeader *header         = &sections[i];
    Rng1U64             raw_data_range = rng_1u64(header->foff, header->foff + header->fsize);
    String8             name           = coff_name_from_section_header(raw_image, header, string_table_off);

    DW_SectionKind  s      = DW_Section_Null;
    B32             is_dwo = 0;
    #define X(_K,_L,_M,_W)                                            \
      if (str8_match_lit(_L, name, 0)) { s = DW_Section_##_K; } \
      if (str8_match_lit(_M, name, 0)) { s = DW_Section_##_K; } \
      if (str8_match_lit(_W, name, 0)) { s = DW_Section_##_K; is_dwo = 1; }
      DW_SectionKind_XList(X)
    #undef X

    if (s != DW_Section_Null) {
      if (sect_status[s]) {
        Assert(!"too many debug sections with identical name, picking first");
      } else {
        sect_status[s] = 1;
        DW_Section *d = &result.v[s];
        d->name       = push_str8_copy(arena, name);
        d->data       = str8_substr(raw_image, raw_data_range);
        d->mode       = dim_1u64(raw_data_range) > max_U32 ? DW_Mode_64Bit : DW_Mode_32Bit;
        d->is_dwo     = is_dwo;
      }
    }
  }

  return result;
}


