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

