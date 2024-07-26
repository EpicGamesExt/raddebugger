// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "lib_rdi_format/rdi_format.c"
#include "lib_rdi_format/rdi_format_parse.c"

internal void
rdi_decompress_parsed(U8 *decompressed_data, U64 decompressed_size, RDI_Parsed *og_rdi)
{
  // rjf: copy header
  RDI_Header *src_header = (RDI_Header *)og_rdi->raw_data;
  RDI_Header *dst_header = (RDI_Header *)decompressed_data;
  {
    MemoryCopy(dst_header, src_header, sizeof(RDI_Header));
  }
  
  // rjf: copy & adjust sections for decompressed version
  if(og_rdi->sections_count != 0)
  {
    RDI_Section *dsec_base = (RDI_Section *)(decompressed_data + dst_header->data_section_off);
    MemoryCopy(dsec_base, (U8 *)og_rdi->raw_data + src_header->data_section_off, sizeof(RDI_Section) * og_rdi->sections_count);
    U64 off = dst_header->data_section_off + sizeof(RDI_Section) * og_rdi->sections_count;
    off += 7;
    off -= off%8;
    for(U64 idx = 0; idx < og_rdi->sections_count; idx += 1)
    {
      dsec_base[idx].encoding = RDI_SectionEncoding_Unpacked;
      dsec_base[idx].off = off;
      dsec_base[idx].encoded_size = dsec_base[idx].unpacked_size;
      off += dsec_base[idx].unpacked_size;
      off += 7;
      off -= off%8;
    }
  }
  
  // rjf: decompress sections into new decompressed file buffer
  if(og_rdi->sections_count != 0)
  {
    RDI_Section *src_first = og_rdi->sections;
    RDI_Section *dst_first = (RDI_Section *)(decompressed_data + dst_header->data_section_off);
    RDI_Section *src_opl = src_first + og_rdi->sections_count;
    RDI_Section *dst_opl = dst_first + og_rdi->sections_count;
    for(RDI_Section *src = src_first, *dst = dst_first;
        src < src_opl && dst < dst_opl;
        src += 1, dst += 1)
    {
      rr_lzb_simple_decode((U8*)og_rdi->raw_data + src->off, src->encoded_size,
                           decompressed_data     + dst->off, dst->unpacked_size);
    }
  }
}

