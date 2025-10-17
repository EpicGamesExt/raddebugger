// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal DW_CallFrameInfo
dw_call_frame_info_from_data(Arena *arena, Arch arch, U64 rebase, String8 debug_frame)
{
  Temp scratch = scratch_begin(&arena, 1);

  // count CIE and FDE entries
  U64 cie_count = 0, fde_count = 0;
  for (U64 cursor = 0, desc_size; cursor < debug_frame.size; cursor += desc_size) {
    DW_DescriptorEntry desc = {0};
    desc_size = dw_parse_descriptor_entry_header(debug_frame, cursor, &desc);
    if (desc_size == 0) { break; }
    if (desc.type == DW_DescriptorEntryType_CIE) {
      cie_count += 1;
    } else if (desc.type == DW_DescriptorEntryType_FDE) {
      fde_count += 1;
    }
  }

  // parse CIEs and build (offset -> CIE) hash table
  HashTable *cie_ht = hash_table_init(scratch.arena, (U64)(cie_count * 1.3));
  DW_CIE *cie = push_array(arena, DW_CIE, cie_count);
  U64 parse_cie_count = 0;
  for (U64 cursor = 0, desc_size; cursor < debug_frame.size; cursor += desc_size) {
    DW_DescriptorEntry desc = {0};
    desc_size = dw_parse_descriptor_entry_header(debug_frame, cursor, &desc);
    if (desc_size == 0) { break; }
    if (desc.type == DW_DescriptorEntryType_CIE) {
      if (dw_parse_cie(str8_skip(debug_frame, cursor), desc.format, arch, &cie[parse_cie_count])) {
        hash_table_push_u64_raw(scratch.arena, cie_ht, cursor, &cie[parse_cie_count]);
        parse_cie_count += 1;
      }
    }
  }

  // parse FDEs
  DW_FDE *fde = push_array(arena, DW_FDE, fde_count);
  U64 parse_fde_count = 0;
  for (U64 cursor = 0, desc_size; cursor < debug_frame.size; cursor += desc_size) {
    DW_DescriptorEntry desc = {0};
    desc_size = dw_parse_descriptor_entry_header(debug_frame, cursor, &desc);
    if (desc_size == 0) { break; }
    if (desc.type == DW_DescriptorEntryType_FDE) {
      DW_CIE *cie = hash_table_search_u64_raw(cie_ht, desc.cie_pointer);
      if (dw_parse_fde(str8_skip(debug_frame, cursor), desc.format, cie, &fde[parse_fde_count])) {
        fde[parse_fde_count].pc_range.min += rebase;
        fde[parse_fde_count].pc_range.max += rebase;
        parse_fde_count += 1;
      }
    }
  }

  // fill out result
  DW_CallFrameInfo cfi = {0};
  cfi.cie_count = parse_cie_count;
  cfi.fde_count = parse_fde_count;
  cfi.cie       = cie;
  cfi.fde       = fde;

  scratch_end(scratch);
  return cfi;
}


