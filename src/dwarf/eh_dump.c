// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8List
eh_dump_list_from_data(Arena *arena, Arch arch, U64 eh_frame_hdr_vaddr, U64 eh_frame_vaddr, String8 eh_frame_hdr, String8 eh_frame, EH_DumpSubsetFlags subset_flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strings = {0};
#define dumpf(...) str8_list_pushf(arena, &strings, __VA_ARGS__)

  EH_PtrCtx   ptr_ctx = { .data_vaddr = eh_frame_hdr_vaddr };
  EH_FrameHdr hdr     = eh_parse_frame_hdr(eh_frame_hdr, byte_size_from_arch(arch), &ptr_ctx);
  DW_Ext      ext     = DW_Ext_All;

  if (subset_flags & EH_DumpSubsetFlag_EhFrameHdr) {
    dumpf("eh_frame_hdr:\n");
    dumpf("{\n");
    dumpf("  version:          %u\n",   hdr.version);
    dumpf("  eh_frame_ptr_enc: %S\n",   eh_string_from_ptr_enc(scratch.arena, hdr.eh_frame_ptr_enc));
    dumpf("  table_enc:        %S\n",   eh_string_from_ptr_enc(scratch.arena, hdr.table_enc));
    dumpf("  fde_count:        %llu\n", hdr.fde_count);
    if (hdr.eh_frame_ptr_enc != EH_PtrEnc_Omit) {
      dumpf("  eh_frame_ptr:     0x%I64x\n", hdr.eh_frame_ptr);
    }

    dumpf("  Entries:\n");
    dumpf("  {\n");
    for (U64 cursor = 0; cursor < hdr.table.size; ) {
      U64 entry_off = cursor;

      U64 pc      = 0;
      U64 pc_size = eh_parse_ptr(hdr.table, cursor, cursor, &ptr_ctx, hdr.table_enc, &pc);
      if (pc_size == 0) { break; }
      cursor += pc_size;

      U64 fde_addr      = 0;
      U64 fde_addr_size = eh_parse_ptr(hdr.table, cursor, cursor, &ptr_ctx, hdr.table_enc, &fde_addr);
      if (fde_addr_size == 0) { break; }
      cursor += fde_addr_size;

      U64 fde_offset = fde_addr - eh_frame_vaddr;
      dumpf("    { off=0x%04I64x, pc=0x%I64x, fde=0x%I64x }\n", entry_off, pc, fde_offset);
    }
    dumpf("  }\n");

    dumpf("}\n");

  }

  if (subset_flags & EH_DumpSubsetFlag_EhFrame) {
    dumpf(".eh_frame:\n");
    dumpf("{\n");
    for (U64 cursor = 0; cursor < eh_frame.size; ) {
      DW_DescriptorEntry desc = {0};
      U64 desc_size = eh_parse_descriptor_entry_header(eh_frame, cursor, &desc);
      if (desc_size == 0) { break; }
      
      switch (desc.type) {
      case DW_DescriptorEntryType_Null: break;
      case DW_DescriptorEntryType_CIE: {
        String8 cie_data = str8_substr(eh_frame, desc.entry_range);
        DW_CIE cie = {0};
        if (eh_parse_cie(cie_data, desc.format, arch, eh_frame_vaddr + cursor, &ptr_ctx, &cie)) {
          String8List init_insts_str_list = dw_string_list_from_cfi_program(scratch.arena, 0, arch, DW_Version_5, ext, cie.format, 0, &cie, eh_decode_ptr, &ptr_ctx, cie.insts);
          dumpf("  CIE: // entry range: %r\n", desc.entry_range);
          dumpf("  {\n");
          dumpf("    Format:          %S\n",     dw_string_from_format(desc.format));
          dumpf("    Version:         %u\n",     cie.version);
          dumpf("    Aug string:      \"%S\"\n", cie.aug_string);
          dumpf("    Code align:      %I64u\n",  cie.code_align_factor);
          dumpf("    Data align:      %I64d\n",  cie.data_align_factor);
          dumpf("    Return addr reg: %u\n",     cie.ret_addr_reg);
          if (cie.version > DW_Version_3) {
            dumpf("    Address size:          %u\n", cie.address_size);
            dumpf("    Segment selector size: %u\n", cie.segment_selector_size);
          }
          dumpf("    Initial Insturction:\n");
          dumpf("    {\n");
          for EachNode(n, String8Node, init_insts_str_list.first) { dumpf("      %S\n", n->string); }
          dumpf("    }\n");
          dumpf("  }\n");
        } else {
          dumpf("ERROR: unable to parse CIE @ %I64x\n", desc.entry_range.min);
        }
      } break;
      case DW_DescriptorEntryType_FDE: {
        U64 cie_offset = desc.cie_pointer_off - desc.cie_pointer;

        DW_CIE cie = {0};
        {
          DW_DescriptorEntry cie_desc = {0};
          eh_parse_descriptor_entry_header(eh_frame, cie_offset, &cie_desc);
          if (cie_desc.type == DW_DescriptorEntryType_CIE) {
            String8 cie_data = str8_substr(eh_frame, cie_desc.entry_range);
            eh_parse_cie(cie_data, cie_desc.format, arch, eh_frame_vaddr + cie_offset, &ptr_ctx, &cie);
          }
        }

        String8 fde_raw = str8_substr(eh_frame, desc.entry_range);
        DW_FDE  fde     = {0};
        if (eh_parse_fde(fde_raw, desc.format, eh_frame_vaddr + cursor, &cie, &ptr_ctx, &fde)) {
          String8List insts_str_list = dw_string_list_from_cfi_program(scratch.arena, 0, arch, hdr.version, ext, fde.format, 0, &cie, eh_decode_ptr, &ptr_ctx, fde.insts);

          dumpf("  FDE: // entry range: %r\n", desc.entry_range);
          dumpf("  {\n");
          {
            dumpf("    Format:      %S\n",      dw_string_from_format(fde.format));
            dumpf("    CIE:         0x%I64x\n", cie_offset);
            dumpf("    PC range:    %r\n",      fde.pc_range);
            dumpf("    Instructions:\n");
            dumpf("    {\n");
            for EachNode(n, String8Node, insts_str_list.first) { dumpf("      %S\n", n->string); }
            dumpf("    }\n");

            dumpf("    Unwind:\n");
            dumpf("    {\n");
            DW_CFI_Unwind *cfi_unwind = dw_cfi_unwind_init(scratch.arena, arch, &cie, &fde, eh_decode_ptr, &ptr_ctx);
            do {
              String8 cfa_str     = dw_string_from_cfa(scratch.arena, arch, cie.address_size, hdr.version, ext, fde.format, cfi_unwind->row->cfa);
              String8 cfi_row_str = dw_string_from_cfi_row(scratch.arena, arch, cie.address_size, hdr.version, ext, fde.format, cfi_unwind->row);
              dumpf("      { PC: 0x%I64x, CFA: %-7S, Rules: { %S }\n", cfi_unwind->pc, cfa_str, cfi_row_str);
            } while (dw_cfi_next_row(scratch.arena, cfi_unwind));
          dumpf("    }\n");
          }
        } else {
          dumpf("ERROR: unable to parse FDE @ %I64x\n", desc.entry_range.min);
        }
        dumpf("  }\n");
      } break;
      }

      cursor += desc_size;
    }
    dumpf("}\n");
  }

#undef dumpf
  scratch_end(scratch);
  return strings;
}

