// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U64
eh_read_ptr(String8 frame_base, U64 off, EH_PtrCtx *ptr_ctx, EH_PtrEnc encoding, U64 *ptr_out)
{
  U64 ptr_off = off;

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
        aug_cursor += eh_read_ptr(aug_data, aug_cursor, ptr_ctx, handler_encoding, &handler_ip);
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

internal U64
eh_size_from_aug_data(String8 aug_string, String8 data, EH_PtrCtx *ptr_ctx)
{
  return eh_parse_aug_data(aug_string, data, ptr_ctx, 0);
}

internal U64
eh_frame_hdr_search_linear_x64(String8 raw_eh_frame_hdr, EH_PtrCtx *ptr_ctx, U64 location)
{
  // Table contains only addresses for first instruction in a function and we cannot
  // guarantee that result is FDE that corresponds to the input location. 
  // So input location must be cheked against range from FDE header again.
  
  U64 closest_location = max_U64;
  U64 closest_address  = max_U64;
  
  U64 cursor = 0;
  
  U8 version = 0;
  cursor += str8_deserial_read_struct(raw_eh_frame_hdr, cursor, &version);
  
  if (version == 1) {
#if 0
    EH_PtrCtx ptr_ctx = {0};
    // Set this to base address of .eh_frame_hdr. Entries are relative
    // to this section for some reason.
    ptr_ctx.data_vaddr = range.min;
    // If input location is VMA then set this to address of .text. 
    // Pointer parsing function will adjust "init_location" to correct VMA.
    ptr_ctx.text_vaddr = 0; 
#endif
    
    EH_PtrEnc eh_frame_ptr_enc = 0, fde_count_enc = 0, table_enc = 0;
    cursor += str8_deserial_read_struct(raw_eh_frame_hdr, cursor, &eh_frame_ptr_enc);
    cursor += str8_deserial_read_struct(raw_eh_frame_hdr, cursor, &fde_count_enc);
    cursor += str8_deserial_read_struct(raw_eh_frame_hdr, cursor, &table_enc);
    
    U64 eh_frame_ptr = 0, fde_count = 0;
    NotImplemented;
    //cursor += dw_unwind_parse_pointer_x64(raw_eh_frame_hdr.str, rng_1u64(0, raw_eh_frame_hdr.size), ptr_ctx, eh_frame_ptr_enc, cursor, &eh_frame_ptr);
    //cursor += dw_unwind_parse_pointer_x64(raw_eh_frame_hdr.str, rng_1u64(0, raw_eh_frame_hdr.size), ptr_ctx, fde_count_enc, cursor, &fde_count);
    
    for (U64 fde_idx = 0; fde_idx < fde_count; ++fde_idx) {
      U64 init_location = 0, address = 0;
      NotImplemented;
      //cursor += dw_unwind_parse_pointer_x64(raw_eh_frame_hdr.str, rng_1u64(0, raw_eh_frame_hdr.size), ptr_ctx, table_enc, cursor, &init_location);
      //cursor += dw_unwind_parse_pointer_x64(raw_eh_frame_hdr.str, rng_1u64(0, raw_eh_frame_hdr.size), ptr_ctx, table_enc, cursor, &address);
      
      S64 current_delta = (S64)(location - init_location);
      S64 closest_delta = (S64)(location - closest_location);
      if (0 <= current_delta && current_delta < closest_delta) {
        closest_location = init_location;
        closest_address  = address;
      }
    }
  }
  
  // address where to find corresponding FDE, this is an absolute offset
  // into the image file.
  return closest_address;
}

#if 0
internal DW_CFIRecords
dw_unwind_eh_frame_hdr_from_ip_fast_x64(String8 raw_eh_frame, String8 raw_eh_frame_hdr, EH_PtrCtx *ptr_ctx, U64 ip_voff)
{
  DW_CFIRecords result = {0};
  
  // find FDE offset
  void *eh_frame_hdr = raw_eh_frame.str;
  U64   fde_offset   = dw_search_eh_frame_hdr_linear_x64(raw_eh_frame_hdr, ptr_ctx, ip_voff);
  
  B32 is_fde_offset_valid = (fde_offset != max_U64);
  if (is_fde_offset_valid) {
    U64 fde_read_offset = (fde_offset - ptr_ctx->raw_base_vaddr);
    
    // read FDE size
    U64 fde_size = 0;
    fde_read_offset += str8_deserial_read_dwarf_packed_size(raw_eh_frame, fde_read_offset, &fde_size);
    
    // read FDE discriminator
    U32 fde_discrim = 0;
    fde_read_offset += str8_deserial_read_struct(raw_eh_frame, fde_read_offset, &fde_discrim);
    
    // compute parent CIE offset
    U64 cie_read_offset = fde_read_offset - (fde_discrim + sizeof(fde_discrim));
    
    // read CIE size
    U64 cie_size = 0;
    cie_read_offset += str8_deserial_read_dwarf_packed_size(raw_eh_frame, cie_read_offset, &cie_size);
    
    // read CIE discriminator
    U32 cie_discrim = max_U32;
    cie_read_offset += str8_deserial_read_struct(raw_eh_frame, cie_read_offset, &cie_discrim);
    
    B32 is_fde = (fde_discrim != 0);
    B32 is_cie = (cie_discrim == 0);
    if (is_fde && is_cie) {
      Rng1U64 cie_range = rng_1u64(0, cie_read_offset + (cie_size - sizeof(cie_discrim)));
      Rng1U64 fde_range = rng_1u64(0, fde_read_offset + (fde_size - sizeof(fde_discrim)));
      
      // parse CIE
      DW_CIE cie = {0};
      dw_unwind_parse_cie_x64(raw_eh_frame.str, cie_range, ptr_ctx, cie_read_offset, &cie);
      
      // parse FDE
      DW_FDE fde = {0};
      NotImplemented;
      //dw_unwind_parse_fde_x64(raw_eh_frame.str, fde_range, ptr_ctx, &cie, fde_read_offset, &fde);
      
      // range check instruction pointer
      if (contains_1u64(fde.pc_range, ip_voff)) {
        result.valid = 1;
        result.cie   = cie;
        result.fde   = fde;
      }
    }
  }
  
  return result;
}
#endif

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
