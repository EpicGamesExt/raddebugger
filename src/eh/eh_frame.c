// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U64
eh_parse_ptr(String8 frame_base, U64 off, EH_PtrCtx *ptr_ctx, EH_PtrEnc encoding, U64 *ptr_out)
{
  U64 ptr_off = off;

  if (encoding == EH_PtrEnc_Omit) {
    return 0;
  }

  // align read offset as needed
  if (encoding == EH_PtrEnc_Aligned) {
    ptr_off  = AlignPow2(ptr_off, ptr_ctx->ptr_align);
    encoding = EH_PtrEnc_Ptr;
  }

  // decode pointer value
  U64 decode_size  = 0;
  U64 raw_ptr_size = 0;
  U64 raw_ptr      = 0;
  switch (encoding & EH_PtrEnc_TypeMask) {
  default: { InvalidPath; } break;
  
  case EH_PtrEnc_Ptr   : { raw_ptr_size = 8; } goto ufixed;
  case EH_PtrEnc_UData2: { raw_ptr_size = 2; } goto ufixed;
  case EH_PtrEnc_UData4: { raw_ptr_size = 4; } goto ufixed;
  case EH_PtrEnc_UData8: { raw_ptr_size = 8; } goto ufixed;
  ufixed: {
    decode_size += str8_deserial_read(frame_base, ptr_off, &raw_ptr, raw_ptr_size, raw_ptr_size);
  } break;
  
  // TODO: Signed is actually just a flag that indicates this int is negavite.
  // There shouldn't be a read for Signed.
  // For instance, (EH_PtrEnc_UData2 | EH_PtrEnc_Signed) == EH_PtrEnc_SData etc.
  case EH_PtrEnc_Signed: { raw_ptr_size = 8; } goto sfixed; 
  
  case EH_PtrEnc_SData2: { raw_ptr_size = 2; } goto sfixed;
  case EH_PtrEnc_SData4: { raw_ptr_size = 4; } goto sfixed;
  case EH_PtrEnc_SData8: { raw_ptr_size = 8; } goto sfixed;
  sfixed: {
    decode_size += str8_deserial_read(frame_base, ptr_off, &raw_ptr, raw_ptr_size, raw_ptr_size);
    raw_ptr = extend_sign64(raw_ptr, raw_ptr_size);
  } break;
  
  case EH_PtrEnc_ULEB128: { decode_size += str8_deserial_read_uleb128(frame_base, ptr_off, &raw_ptr);       } break;
  case EH_PtrEnc_SLEB128: { decode_size += str8_deserial_read_sleb128(frame_base, ptr_off, (S64*)&raw_ptr); } break;
  }

  // apply relative bases
  if (decode_size > 0) {
    U64 ptr = raw_ptr;
    switch (encoding & EH_PtrEnc_ModifierMask) {
    case EH_PtrEnc_PcRel:   { ptr = ptr_ctx->raw_base_vaddr + off + raw_ptr; } break;
    case EH_PtrEnc_TextRel: { ptr = ptr_ctx->text_vaddr + raw_ptr;           } break;
    case EH_PtrEnc_DataRel: { ptr = ptr_ctx->data_vaddr + raw_ptr;           } break;
    case EH_PtrEnc_FuncRel: {
      Assert(!"TODO: need a sample to verify implementation");
      ptr = ptr_ctx->func_vaddr + raw_ptr;
    } break;
    }

    if (ptr_out) {
      *ptr_out = raw_ptr;
    }
  }

  return decode_size;
}

internal EH_FrameHdr
eh_parse_frame_hdr(String8 data, U64 address_size, EH_PtrCtx *ptr_ctx)
{
  EH_FrameHdr header = {0};
  U64 cursor = 0;

  U64 version_size = str8_deserial_read_struct(data, cursor, &header.version);
  if (version_size == 0) { goto exit; }
  cursor += version_size;

  if (header.version == 1) {
    U64 eh_frame_ptr_enc_size = str8_deserial_read_struct(data, cursor, &header.eh_frame_ptr_enc);
    if (eh_frame_ptr_enc_size == 0) { goto exit; }
    cursor += eh_frame_ptr_enc_size;

    EH_PtrEnc fde_count_enc = 0;
    U64 fde_count_enc_size = str8_deserial_read_struct(data, cursor, &fde_count_enc);
    if (fde_count_enc_size == 0) { goto exit; }
    cursor += fde_count_enc_size;

    U64 table_enc_size = str8_deserial_read_struct(data, cursor, &header.table_enc);
    if (table_enc_size == 0) { goto exit; }
    cursor += table_enc_size;

    U64 eh_frame_ptr = 0;
    cursor += eh_parse_ptr(data, cursor, ptr_ctx, header.eh_frame_ptr_enc, &eh_frame_ptr);

    cursor += eh_parse_ptr(data, cursor, ptr_ctx, header.fde_count_enc, &header.fde_count);

    switch (header.table_enc) {
    case EH_PtrEnc_Ptr:     { header.field_byte_size = address_size; } break;
    case EH_PtrEnc_ULEB128: { InvalidPath;                           } break; // TODO: when loading module convert these to UData8
    case EH_PtrEnc_UData2:  { header.field_byte_size = 2;            } break;
    case EH_PtrEnc_UData4:  { header.field_byte_size = 4;            } break;
    case EH_PtrEnc_UData8:  { header.field_byte_size = 8;            } break;
    case EH_PtrEnc_Signed:  { header.field_byte_size = address_size; } break;
    case EH_PtrEnc_SLEB128: { InvalidPath;                           } break; // TODO: when loading module convert these to UData8
    case EH_PtrEnc_SData2:  { header.field_byte_size = 2;            } break;
    case EH_PtrEnc_SData4:  { header.field_byte_size = 4;            } break;
    case EH_PtrEnc_SData8:  { header.field_byte_size = 8;            } break;
    default: { InvalidPath; } break;
    }
    header.entry_byte_size = header.field_byte_size * 2;

    header.table = str8_skip(data, cursor);
    AssertAlways(header.table.size == header.entry_byte_size * header.fde_count);
  } else {
    Assert(0 && "unknown version");
  }

exit:;
  return header;
}

internal U64
eh_parse_aug_data(String8 aug_string, String8 aug_data, EH_PtrCtx *ptr_ctx, EH_Augmentation *aug_out)
{
  // TODO: 
  // Handle "eh" param, it indicates presence of EH Data field.
  // On 32bit arch it is a 4-byte and on 64-bit 8-byte value.
  // Reference: https://refspecs.linuxfoundation.org/LSB_3.0.0/LSB-PDA/LSB-PDA/ehframechpt.html
  // Reference doc doesn't clarify structure for EH Data though

  U64 cursor = 0;

  EH_AugFlags aug_flags        = 0;
  EH_PtrEnc lsda_encoding    = EH_PtrEnc_Omit;
  EH_PtrEnc addr_encoding    = EH_PtrEnc_UData8;
  EH_PtrEnc handler_encoding = EH_PtrEnc_Omit;
  U64         handler_ip       = 0;
  if (str8_match(str8_prefix(aug_string, 1), str8_lit("z"), 0)) {
    U64 aug_cursor = 0;
    for (U8 *ptr = aug_string.str; ptr < (aug_string.str+aug_string.size); ptr += 1) {
      switch (*ptr) {
      case 'L': {
        aug_cursor += str8_deserial_read_struct(aug_data, aug_cursor, &lsda_encoding);
        aug_flags |= EH_AugFlag_HasLSDA;
      } break;
      case 'P': {
        aug_cursor += str8_deserial_read_struct(aug_data, aug_cursor, &handler_encoding);
        aug_cursor += eh_parse_ptr(aug_data, aug_cursor, ptr_ctx, handler_encoding, &handler_ip);
        aug_flags |= EH_AugFlag_HasHandler;
      } break;
      case 'R': {
        aug_cursor += str8_deserial_read_struct(aug_data, aug_cursor, &addr_encoding);
        aug_flags |= EH_AugFlag_HasAddrEnc;
      } break;
      default: { Assert(!"failed to parse augmentation string"); goto exit; } break;
      }
    }
  }

  if (aug_out) {
    aug_out->handler_ip       = handler_ip;
    aug_out->handler_encoding = handler_encoding;
    aug_out->lsda_encoding    = handler_encoding;
    aug_out->addr_encoding    = addr_encoding;
    aug_out->flags            = aug_flags;
  }

exit:;
  U64 parse_size = cursor; 
  return parse_size;
}

internal B32
eh_parse_cie(String8 data, DW_Format format, Arch arch, EH_PtrCtx *ptr_ctx, DW_CIE *cie_out)
{
  B32 is_parsed = 0;
  U64 cursor    = format == DW_Format_32Bit ? 4 : 12;

  U64 cie_id      = 0;
  U64 cie_id_size = str8_deserial_read_dwarf_uint(data, cursor, format, &cie_id);
  if (cie_id_size == 0) { goto exit; }
  cursor += cie_id_size;

  U8  version      = 0;
  U64 version_size = str8_deserial_read_struct(data, cursor, &version);
  if (version_size == 0) { goto exit; }
  cursor += version_size;

  String8         aug_string        = {0};
  EH_Augmentation aug               = {0};
  U64             code_align_factor = 0;
  S64             data_align_factor = 0;
  U64             ret_addr_reg      = max_U64;
  if (version == 1) {
    U64 aug_string_size = str8_deserial_read_cstr(data, cursor, &aug_string);
    if (aug_string_size == 0) { goto exit; }
    cursor += aug_string_size;

    U64 aug_data_size = eh_parse_aug_data(aug_string, str8_skip(data, cursor), ptr_ctx, &aug);
    cursor += aug_data_size;

    U64 code_align_factor_size = str8_deserial_read_uleb128(data, cursor, &code_align_factor);
    if (code_align_factor_size == 0) { goto exit; }
    cursor += code_align_factor_size;

    U64 data_align_factor_size = str8_deserial_read_sleb128(data, cursor, &data_align_factor);
    if (data_align_factor_size == 0) { goto exit; }
    cursor += data_align_factor_size;

    ret_addr_reg = 0;
    U64 ret_addr_reg_size = str8_deserial_read(data, cursor, &ret_addr_reg, sizeof(U8), sizeof(U8));
    if (ret_addr_reg_size == 0) { goto exit; }
    cursor += ret_addr_reg_size;
  }

  cie_out->insts                 = str8_skip(data, cursor);
  cie_out->aug_string            = aug_string;
  cie_out->code_align_factor     = code_align_factor;
  cie_out->data_align_factor     = data_align_factor;
  cie_out->ret_addr_reg          = ret_addr_reg;
  cie_out->format                = format;
  cie_out->version               = version;
  cie_out->address_size          = byte_size_from_arch(arch);
  cie_out->segment_selector_size = 0;

  cie_out->ext[EH_CIE_Ext_LSDAEnc]    = EH_PtrEnc_Omit;
  cie_out->ext[EH_CIE_Ext_AddrEnc]    = EH_PtrEnc_Omit;
  cie_out->ext[EH_CIE_Ext_HandlerEnc] = EH_PtrEnc_Omit;

  if (aug.flags & EH_AugFlag_HasLSDA) {
    cie_out->ext[EH_CIE_Ext_LSDAEnc] = aug.lsda_encoding;
  }
  if (aug.flags & EH_AugFlag_HasAddrEnc) {
    cie_out->ext[EH_CIE_Ext_AddrEnc] = aug.addr_encoding;
  }
  if (aug.flags & EH_AugFlag_HasHandler) {
    cie_out->ext[EH_CIE_Ext_HandlerEnc] = aug.handler_encoding;
    cie_out->ext[EH_CIE_Ext_HandlerIp ] = aug.handler_ip;
  }

  is_parsed = 1;
exit:;
  return is_parsed;
}

internal B32
eh_parse_fde(String8               data,
             DW_Format             format,
             DW_CIEFromOffsetFunc *cie_from_offset_func,
             void                 *cie_from_offset_ud,
             EH_PtrCtx            *ptr_ctx,
             DW_FDE               *fde_out)
{
  B32 is_parsed = 0;
  U64 cursor    = format == DW_Format_32Bit ? 4 : 12;

  U64 cie_pointer      = 0;
  U64 cie_pointer_size = str8_deserial_read_dwarf_uint(data, cursor, format, &cie_pointer);
  if (cie_pointer_size == 0) { goto exit; }

  DW_CIE *cie = cie_from_offset_func(cie_from_offset_ud, cie_pointer);
  if (cie == 0) { goto exit; }

  EH_PtrEnc addr_enc = cie->ext[EH_CIE_Ext_AddrEnc];

  U64 pc_begin = 0;
  U64 pc_range = 0;
  if (addr_enc != EH_PtrEnc_Omit) {
    U64 pc_begin_size = eh_parse_ptr(data, cursor, ptr_ctx, addr_enc, &pc_begin);
    if (pc_begin_size == 0) { goto exit; }
    cursor += pc_begin_size;

    U64 pc_range_size = eh_parse_ptr(data, cursor, ptr_ctx, addr_enc, &pc_range_size);
    if (pc_range_size == 0) { goto exit; }
    cursor += pc_range_size;
  }

  if (cursor + cie->aug_data.size > data.size) { goto exit; }
  cursor += cie->aug_data.size;

  fde_out->format      = format;
  fde_out->cie_pointer = cie_pointer;
  fde_out->cie         = cie;
  fde_out->pc_range    = rng_1u64(pc_begin, pc_begin + pc_range);
  fde_out->insts       = str8_skip(data, cursor);

  is_parsed = 1;
exit:;
  return is_parsed;
}

internal
DW_CIE_FROM_OFFSET_FUNC(eh_parse_cfi_cie_from_offset)
{
  return ud;
}

internal B32
eh_parse_cfi(String8 eh_frame, U64 fde_offset, Arch arch, EH_PtrCtx *ptr_ctx, DW_CIE *cie_out, DW_FDE *fde_out)
{
  B32 is_parsed = 0;
  DW_DescriptorEntry fde_desc = {0};
  dw_parse_descriptor_entry_header(eh_frame, fde_offset, &fde_desc);
  if (fde_desc.type == DW_DescriptorEntryType_FDE) {
    U64 cie_pointer_off  = fde_desc.format == DW_Format_32Bit ? 4 : 12;
    U64 cie_pointer      = 0;
    U64 cie_pointer_size = str8_deserial_read_dwarf_uint(eh_frame, fde_offset + cie_pointer_off, fde_desc.format, &cie_pointer);
    if (cie_pointer_size) {
      DW_DescriptorEntry cie_desc = {0};
      dw_parse_descriptor_entry_header(eh_frame, cie_pointer, &cie_desc);
      if (cie_desc.type == DW_DescriptorEntryType_CIE) {
        if (eh_parse_cie(str8_substr(eh_frame, cie_desc.entry_range), cie_desc.format, arch, ptr_ctx, cie_out)) {
          if (eh_parse_fde(str8_substr(eh_frame, fde_desc.entry_range), fde_desc.format, eh_parse_cfi_cie_from_offset, cie_out, ptr_ctx, fde_out)) {
            return is_parsed;
          }
        }
      }
    }
  }
  return is_parsed;
}

internal U64
eh_find_nearest_fde(EH_FrameHdr header, EH_PtrCtx *ptr_ctx, U64 pc)
{
  U64 fde_offset = max_U64;
  U64 fde_idx    = max_U64;

  if (header.version == 1) {
    if (header.fde_count > 0) {
      U64 first = 0;
      U64 first_size = eh_parse_ptr(header.table, 0, ptr_ctx, header.table_enc, &first);
      AssertAlways(first_size);
      if (first == pc) {
        fde_idx = 0;
        goto exit;
      }
      if (first > pc) {
        goto exit;
      }

      U64 last = 0;
      U64 last_size = eh_parse_ptr(header.table, header.table.size - header.entry_byte_size, ptr_ctx, header.table_enc, &last);
      AssertAlways(last_size);
      if (last <= pc) {
        fde_idx = header.fde_count - 1;
        goto exit;
      }

      if (first <= pc && pc < last) {
        U64 l = 0;
        U64 r = header.fde_count - 1;
        while (l <= r) {
          U64 m = l + (r - l) / 2;
          U64 m_pc = 0;
          U64 m_pc_size = eh_parse_ptr(header.table, m * header.entry_byte_size, ptr_ctx, header.table_enc, &m_pc);
          Assert(m_pc_size);

          if (m_pc == pc) {
            fde_idx = m;
            goto exit;
          } else if (m_pc < pc) {
            l = m + 1;
          } else {
            r = m - 1;
          }
        }
        fde_idx = l;
      }
    }

  }

exit:;
  if (fde_idx < header.fde_count) {
    U64 fde_offset_size = eh_parse_ptr(header.table, (fde_idx * header.entry_byte_size) + header.field_byte_size, ptr_ctx, header.table_enc, &fde_offset);
    Assert(fde_offset_size);
  }
  return fde_offset;
}

internal int
eh_frame_hdr_entry_sort(void *raw_a, void *raw_b)
{
  return ((EH_FrameHdrEntry *)raw_a)->addr < ((EH_FrameHdrEntry *)raw_b)->addr;
}

internal String8
eh_frame_hdr_from_call_frame_info(Arena *arena, U64 fde_count, U64 *fde_offsets, DW_FDE *fde)
{
  Temp scratch = scratch_begin(&arena, 1);

  // make .eh_frame_hdr
  String8List srl = {0};
  str8_serial_begin(scratch.arena, &srl);
  str8_serial_push_u8(scratch.arena, &srl, 1);                // version
  str8_serial_push_u8(scratch.arena, &srl, EH_PtrEnc_Omit);   // omit eh_frame_ptr field
  str8_serial_push_u8(scratch.arena, &srl, EH_PtrEnc_UData8); // fde_count encoding
  str8_serial_push_u8(scratch.arena, &srl, EH_PtrEnc_UData8); // table encoding
  str8_serial_push_u64(scratch.arena, &srl, fde_count);   // fde_count
  String8 header = str8_serial_end(scratch.arena, &srl);

  // alloc buffer for output
  U64 buffer_size = header.size + sizeof(EH_FrameHdrEntry) * fde_count;
  U8 *buffer      = push_array(arena, U8, buffer_size);

  // copy header
  MemoryCopyStr8(buffer, header);

  // write the table
  EH_FrameHdrEntry *table = (EH_FrameHdrEntry *)(buffer + header.size);
  for EachIndex(fde_idx, fde_count) {
    table[fde_idx].addr       = fde[fde_idx].pc_range.min;
    table[fde_idx].fde_offset = fde_offsets[fde_idx];
  }
  radsort(table, fde_count, eh_frame_hdr_entry_sort);

  String8 eh_frame_hdr = str8(buffer, buffer_size);
  scratch_end(scratch);
  return eh_frame_hdr;
}

internal
DW_DECODE_PTR(eh_decode_ptr)
{
  EH_DecodePtrCtx *ctx = ud;
  return eh_parse_ptr(data, 0, ctx->ptr_ctx, ctx->addr_enc, ptr_out);
}

internal String8
eh_string_from_ptr_enc_type(EH_PtrEnc type)
{
  switch (type) {
  case EH_PtrEnc_Ptr:     return str8_lit("Ptr");
  case EH_PtrEnc_ULEB128: return str8_lit("ULEB128");
  case EH_PtrEnc_UData2:  return str8_lit("UData2");
  case EH_PtrEnc_UData4:  return str8_lit("UData4");
  case EH_PtrEnc_UData8:  return str8_lit("UData8");
  case EH_PtrEnc_Signed:  return str8_lit("Signed");
  case EH_PtrEnc_SLEB128: return str8_lit("SLEB128");
  case EH_PtrEnc_SData2:  return str8_lit("SData2");
  case EH_PtrEnc_SData4:  return str8_lit("SData4");
  case EH_PtrEnc_SData8:  return str8_lit("SData8");
  }
  return str8_zero();
}

internal String8
eh_string_from_ptr_enc_modifier(EH_PtrEnc modifier)
{
  switch (modifier) {
  case EH_PtrEnc_PcRel:   return str8_lit("PcRel");
  case EH_PtrEnc_TextRel: return str8_lit("TextRel");
  case EH_PtrEnc_DataRel: return str8_lit("DataRel");
  case EH_PtrEnc_FuncRel: return str8_lit("FuncRel");
  case EH_PtrEnc_Aligned: return str8_lit("Aligned");
  }
  return str8_zero();
}

internal String8
eh_string_from_ptr_enc(Arena *arena, EH_PtrEnc enc)
{
  String8 type_str    = eh_string_from_ptr_enc_type(enc & EH_PtrEnc_TypeMask);
  String8 modifer_str = eh_string_from_ptr_enc_modifier(enc & EH_PtrEnc_ModifierMask);
  String8 indir_str   = enc & EH_PtrEnc_Indirect ? str8_lit("Indirect") : str8_zero();
  String8 result      = str8f(arena, "Type: %S, Modifier %S (%S)", type_str, modifer_str, indir_str);
  return result;
}
