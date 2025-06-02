// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- Hasher

internal U64
cv_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal U64
cv_hash_from_item_id(CV_ItemId item_id)
{
  U64 result = cv_hash_from_string(str8_struct(&item_id));
  return result;
}

//- Numeric Decoder

internal CV_NumericParsed
cv_numeric_from_data_range(U8 *first, U8 *opl)
{
  CV_NumericParsed result = {0};
  if(first + 2 <= opl)
  {
    U16 x = *(U16*)first;
    if(x < 0x8000)
    {
      result.kind = CV_NumericKind_USHORT;
      result.val = first;
      result.encoded_size = 2;
    }
    else
    {
      U64 val_size = 0;
      switch(x)
      {
        case CV_NumericKind_CHAR:      val_size = 1; break;
        case CV_NumericKind_SHORT:
        case CV_NumericKind_USHORT:    val_size = 2; break;
        case CV_NumericKind_LONG:
        case CV_NumericKind_ULONG:     val_size = 4; break;
        case CV_NumericKind_FLOAT32:   val_size = 4; break;
        case CV_NumericKind_FLOAT64:   val_size = 8; break;
        case CV_NumericKind_FLOAT80:   val_size = 10; break;
        case CV_NumericKind_FLOAT128:  val_size = 16; break;
        case CV_NumericKind_QUADWORD:
        case CV_NumericKind_UQUADWORD: val_size = 8; break;
        case CV_NumericKind_FLOAT48:   val_size = 6; break;
        case CV_NumericKind_COMPLEX32: val_size = 8; break;
        case CV_NumericKind_COMPLEX64: val_size = 16; break;
        case CV_NumericKind_COMPLEX80: val_size = 20; break;
        case CV_NumericKind_COMPLEX128:val_size = 32; break;
        case CV_NumericKind_VARSTRING: val_size = 0; break; // TODO: ???
        case CV_NumericKind_OCTWORD:
        case CV_NumericKind_UOCTWORD:  val_size = 16; break;
        case CV_NumericKind_DECIMAL:   val_size = 0; break; // TODO: ???
        case CV_NumericKind_DATE:      val_size = 0; break; // TODO: ???
        case CV_NumericKind_UTF8STRING:val_size = 0; break; // TODO: ???
        case CV_NumericKind_FLOAT16:   val_size = 2; break;
      }
      if(first + 2 + val_size <= opl)
      {
        result.kind = x;
        result.val = (first + 2);
        result.encoded_size = 2 + val_size;
      }
    }
  }
  return result;
}

internal U64
cv_read_numeric(String8 data, U64 offset, CV_NumericParsed *out)
{
  *out = cv_numeric_from_data_range(data.str + offset, data.str + data.size);
  return out->encoded_size;
}

internal B32
cv_numeric_fits_in_u64(CV_NumericParsed *num)
{
  B32 result = 0;
  switch(num->kind)
  {
    case CV_NumericKind_USHORT:
    case CV_NumericKind_ULONG:
    case CV_NumericKind_UQUADWORD:
    {
      result = 1;
    }break;
  }
  return result;
}

internal B32
cv_numeric_fits_in_s64(CV_NumericParsed *num)
{
  B32 result = 0;
  switch(num->kind)
  {
    case CV_NumericKind_CHAR:
    case CV_NumericKind_SHORT:
    case CV_NumericKind_LONG:
    case CV_NumericKind_QUADWORD:
    {
      result = 1;
    }break;
  }
  return result;
}

internal B32
cv_numeric_fits_in_f64(CV_NumericParsed *num)
{
  B32 result = 0;
  switch(num->kind)
  {
    case CV_NumericKind_FLOAT32:
    case CV_NumericKind_FLOAT64:
    {
      result = 1;
    }break;
  }
  return result;
}

internal U64
cv_u64_from_numeric(CV_NumericParsed *num)
{
  U64 result = 0;
  switch(num->kind)
  {
    case CV_NumericKind_CHAR:     {result = (U64)(S64)*(S8*)num->val;}break;
    case CV_NumericKind_SHORT:    {result = (U64)(S64)*(S16*)num->val;}break;
    case CV_NumericKind_LONG:     {result = (U64)(S64)*(S32*)num->val;}break;
    case CV_NumericKind_QUADWORD: {result = (U64)(S64)*(S64*)num->val;}break;
    case CV_NumericKind_USHORT:   {result = *(U16*)num->val;}break;
    case CV_NumericKind_ULONG:    {result = *(U32*)num->val;}break;
    case CV_NumericKind_UQUADWORD:{result = *(U64*)num->val;}break;
  }
  return result;
}

internal S64
cv_s64_from_numeric(CV_NumericParsed *num)
{
  S64 result = 0;
  switch(num->kind)
  {
    case CV_NumericKind_CHAR:     {result = *(S8*)num->val;}break;
    case CV_NumericKind_SHORT:    {result = *(S16*)num->val;}break;
    case CV_NumericKind_LONG:     {result = *(S32*)num->val;}break;
    case CV_NumericKind_QUADWORD: {result = *(S64*)num->val;}break;
  }
  return(result);
}

internal F64
cv_f64_from_numeric(CV_NumericParsed *num)
{
  F64 result = 0;
  switch(num->kind)
  {
    case CV_NumericKind_FLOAT32:{result = *(F32*)num->val;}break;
    case CV_NumericKind_FLOAT64:{result = *(F64*)num->val;}break;
  }
  return(result);
}

//- Inline Site Binary Annot Decoder

internal U64
cv_decode_inline_annot_u32(String8 data, U64 offset, U32 *out_value)
{
  U64 cursor = offset;
  
  // rjf: read header
  U8 header = 0;
  cursor += str8_deserial_read_struct(data, cursor, &header);
  
  // rjf: decode value
  U32 value = 0;
  {
    // 1 byte
    if((header & 0x80) == 0)
    {
      value = header;
    }
    
    // 2 bytes
    else if((header & 0xC0) == 0x80 && cursor+1 <= data.size)
    {
      U8 second_byte;
      cursor += str8_deserial_read_struct(data, cursor, &second_byte);
      value = ((header & 0x3F) << 8) | second_byte;
    }
    
    // 4 bytes
    else if((header & 0xE0) == 0xC0 && cursor+3 <= data.size)
    {
      U8 second_byte, third_byte, fourth_byte;
      cursor += str8_deserial_read_struct(data, cursor, &second_byte);
      cursor += str8_deserial_read_struct(data, cursor, &third_byte);
      cursor += str8_deserial_read_struct(data, cursor, &fourth_byte);
      value = (((U32)header & 0x1F) << 24) | ((U32)second_byte << 16) | ((U32)third_byte << 8) | (U32)fourth_byte;
    }
    
    // bad encode
    else if((header & 0xE0) == 0xE0)
    {
      value = max_U32;
    }
  }
  
  // rjf: output results
  if(out_value)
  {
    *out_value = value;
  }
  
  U64 read_size = cursor - offset;
  return read_size;
}

internal U64
cv_decode_inline_annot_s32(String8 data, U64 offset, S32 *out_value)
{
  U32 value;
  U64 read_size = cv_decode_inline_annot_u32(data, offset, &value);
  if(value & 1)
  {
    value = -(value >> 1);
  }
  else
  {
    value = value >> 1;
  }
  *out_value = (S32)value;
  return read_size;
}

internal S32
cv_inline_annot_signed_from_unsigned_operand(U32 value)
{
  if(value & 1)
  {
    value = -(value >> 1);
  }
  else
  {
    value = value >> 1;
  }
  S32 result = (S32)value;
  return result;
}

internal CV_C13InlineSiteDecoder
cv_c13_inline_site_decoder_init(U32 file_off, U32 first_source_ln, U32 parent_voff)
{
  CV_C13InlineSiteDecoder decoder = {0};
  decoder.parent_voff             = parent_voff;
  decoder.file_off                = file_off;
  decoder.ln                      = (S32)first_source_ln;
  decoder.cn                      = 1;
  decoder.ln_changed              = 1;
  return decoder;
}

internal CV_C13InlineSiteDecoderStep
cv_c13_inline_site_decoder_step(CV_C13InlineSiteDecoder *decoder, String8 binary_annots)
{
  CV_C13InlineSiteDecoderStep result = {0};
  
  for (; decoder->cursor < binary_annots.size && result.flags == 0; ) {
    U32 op = CV_InlineBinaryAnnotation_Null;
    decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &op);
    
    switch (op) {
      case CV_InlineBinaryAnnotation_Null: {
        decoder->cursor              = binary_annots.size;
        // this is last run, append range with left over code bytes
        decoder->code_length         = decoder->code_offset - decoder->code_offset_lo;
        decoder->code_length_changed = 1;
      } break;
      case CV_InlineBinaryAnnotation_CodeOffset: {
        decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &decoder->code_offset);
        decoder->code_offset_changed = 1;
      } break;
      case CV_InlineBinaryAnnotation_ChangeCodeOffsetBase: {
        AssertAlways(!"TODO: test case");
        // U32 delta = 0;
        // decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &delta);
        // decoder->code_offset_base = decoder->code_offset;
        // decoder->code_offset_end  = decoder->code_offset + delta;
        // decoder->code_offset     += delta;
      } break;
      case CV_InlineBinaryAnnotation_ChangeCodeOffset: {
        U32 delta = 0;
        decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &delta);
        
        decoder->code_offset += delta;
        
        if (!decoder->code_offset_lo_changed) {
          decoder->code_offset_lo         = decoder->code_offset;
          decoder->code_offset_lo_changed = 1;
        }
        decoder->code_offset_changed = 1;
      } break;
      case CV_InlineBinaryAnnotation_ChangeCodeLength: {
        decoder->code_length = 0;
        decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &decoder->code_length);
        decoder->code_length_changed = 1;
      } break;
      case CV_InlineBinaryAnnotation_ChangeFile: {
        U32 old_file_off = decoder->file_off;
        decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &decoder->file_off);
        decoder->file_off_changed = old_file_off != decoder->file_off;
        // Compiler isn't obligated to terminate code sequence before chaning files,
        // so we have to always force emit code range on file change.
        decoder->code_length_changed = decoder->file_off_changed;
      } break;
      case CV_InlineBinaryAnnotation_ChangeLineOffset: {
        S32 delta = 0;
        decoder->cursor += cv_decode_inline_annot_s32(binary_annots, decoder->cursor, &delta);
        
        decoder->ln         += delta;
        decoder->ln_changed  = 1;
      } break;
      case CV_InlineBinaryAnnotation_ChangeLineEndDelta: {
        AssertAlways(!"TODO: test case");
        // S32 end_delta = 1;
        // decoder->cursor += cv_decode_inline_annot_s32(binary_annots, decoder->cursor, &end_delta);
        // decoder->ln += end_delta;
      } break;
      case CV_InlineBinaryAnnotation_ChangeRangeKind: {
        AssertAlways(!"TODO: test case");
        // decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &range_kind);
      } break;
      case CV_InlineBinaryAnnotation_ChangeColumnStart: {
        AssertAlways(!"TODO: test case");
        // S32 delta;
        // decoder->cursor += cv_decode_inline_annot_s32(binary_annots, decoder->cursor, &delta);
        // decoder->cn += delta;
      } break;
      case CV_InlineBinaryAnnotation_ChangeColumnEndDelta: {
        AssertAlways(!"TODO: test case");
        // S32 end_delta;
        // decoder->cursor += cv_decode_inline_annot_s32(binary_annots, decoder->cursor, &end_delta);
        // decoder->cn += end_delta;
      } break;
      case CV_InlineBinaryAnnotation_ChangeCodeOffsetAndLineOffset: {
        U32 code_offset_and_line_offset = 0;
        decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &code_offset_and_line_offset);
        
        S32 line_delta = cv_inline_annot_signed_from_unsigned_operand(code_offset_and_line_offset >> 4);
        U32 code_delta = (code_offset_and_line_offset & 0xf);
        
        decoder->code_offset += code_delta;
        decoder->ln          += line_delta;
        
        if (!decoder->code_offset_lo_changed) {
          decoder->code_offset_lo         = decoder->code_offset;
          decoder->code_offset_lo_changed = 1;
        }
        
        decoder->code_offset_changed = 1;
        decoder->ln_changed          = 1;
      } break;
      case CV_InlineBinaryAnnotation_ChangeCodeLengthAndCodeOffset: {
        U32 offset_delta = 0;
        decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &decoder->code_length);
        decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &offset_delta); 
        
        decoder->code_offset += offset_delta;
        
        if (!decoder->code_offset_lo_changed) {
          decoder->code_offset_lo         = decoder->code_offset;
          decoder->code_offset_lo_changed = 1;
        }
        
        decoder->code_offset_changed = 1;
        decoder->code_length_changed = 1;
      } break;
      case CV_InlineBinaryAnnotation_ChangeColumnEnd: {
        AssertAlways(!"TODO: test case");
        // U32 column_end = 0;
        // decoder->cursor += cv_decode_inline_annot_u32(binary_annots, decoder->cursor, &column_end);
      } break;
    }
    
    U64 line_code_offset = decoder->code_offset;
    
    if (decoder->code_length_changed) {
      // compute upper bound of the range
      U64 code_offset_hi = decoder->code_offset + decoder->code_length;
      
      // can last code range be extended to cover current sequence too?
      if (decoder->last_range.max == decoder->parent_voff + decoder->code_offset_lo) {
        decoder->last_range.max = decoder->parent_voff + code_offset_hi;
        
        result.flags |= CV_C13InlineSiteDecoderStepFlag_ExtendLastRange;
        result.range  = decoder->last_range;
      } else {
        decoder->last_range      = rng_1u64(decoder->parent_voff + decoder->code_offset_lo, decoder->parent_voff + code_offset_hi);
        decoder->file_last_range = decoder->last_range;
        
        result.flags |= CV_C13InlineSiteDecoderStepFlag_EmitRange;
        result.range  = decoder->last_range;
      }
      
      // update state
      decoder->code_offset_lo         = code_offset_hi;
      decoder->code_offset           += decoder->code_length;
      decoder->code_offset_lo_changed = 0;
      decoder->code_length_changed    = 0;
      decoder->code_length            = 0;
    }
    
    if (decoder->file_off_changed || (decoder->file_count == 0)) {
      result.flags    |= CV_C13InlineSiteDecoderStepFlag_EmitFile;
      result.file_off  = decoder->file_off;
      
      // update state
      decoder->file_last_range   = decoder->last_range;
      decoder->file_off_changed  = 0;
      decoder->file_count       += 1;
      decoder->file_line_count   = 0;
    }
    
    if (decoder->code_offset_changed && decoder->ln_changed) {
      if (decoder->file_line_count == 0 || decoder->file_last_ln != decoder->ln) {
        result.flags         |= CV_C13InlineSiteDecoderStepFlag_EmitLine;
        result.ln             = decoder->ln;
        result.cn             = decoder->cn;
        result.line_voff      = decoder->parent_voff + line_code_offset;
        result.line_voff_end  = decoder->last_range.max;
        
        // update state
        decoder->file_line_count += 1;
        decoder->file_last_ln     = decoder->ln;
      }
      
      // update state
      decoder->code_offset_changed = 0;
      decoder->ln_changed          = 0;
    }
  }
  
  return result;
}

//- Symbol/Leaf Helpers

internal B32
cv_is_udt_name_anon(String8 name)
{
  // corresponds to fUDTAnon from dbi/tm.cpp:817
  B32 is_anon = str8_match_lit("<unnamed-tag>",   name, 0) ||
    str8_match_lit("__unnamed",       name, 0) ||
    str8_match_lit("::<unnamed-tag>", name, StringMatchFlag_RightSideSloppy) ||
    str8_match_lit("::__unnamed",     name, StringMatchFlag_RightSideSloppy);
  return is_anon;
}

internal B32
cv_is_udt(CV_LeafKind kind)
{
  B32 is_udt = kind == CV_LeafKind_CLASS            ||
    kind == CV_LeafKind_STRUCTURE        || 
    kind == CV_LeafKind_CLASS2           || 
    kind == CV_LeafKind_STRUCT2          || 
    kind == CV_LeafKind_INTERFACE        || 
    kind == CV_LeafKind_UNION            || 
    kind == CV_LeafKind_ENUM             || 
    kind == CV_LeafKind_UDT_MOD_SRC_LINE || 
    kind == CV_LeafKind_UDT_SRC_LINE     || 
    kind == CV_LeafKind_ALIAS;
  return is_udt;
}

internal B32
cv_is_global_symbol(CV_SymKind kind)
{
  B32 is_global_symbol = kind == CV_SymKind_CONSTANT       ||
    kind == CV_SymKind_GDATA16        ||
    kind == CV_SymKind_GDATA32_16t    ||
    kind == CV_SymKind_GDATA32_ST     ||
    kind == CV_SymKind_GDATA32        ||
    kind == CV_SymKind_GTHREAD32_16t  ||
    kind == CV_SymKind_GTHREAD32_ST   ||
    kind == CV_SymKind_GTHREAD32;
  return is_global_symbol;
}

internal B32
cv_is_typedef(CV_SymKind kind)
{
  B32 is_typedef = kind == CV_SymKind_UDT_16t ||
    kind == CV_SymKind_UDT_ST  ||
    kind == CV_SymKind_UDT;
  return is_typedef;
}

internal B32
cv_is_scope_symbol(CV_SymKind kind)
{
  B32 is_scope = kind == CV_SymKind_GPROC32     || 
    kind == CV_SymKind_LPROC32     || 
    kind == CV_SymKind_BLOCK32     || 
    kind == CV_SymKind_THUNK32     || 
    kind == CV_SymKind_INLINESITE  ||
    kind == CV_SymKind_INLINESITE2 || 
    kind == CV_SymKind_WITH32      ||
    kind == CV_SymKind_SEPCODE     ||
    kind == CV_SymKind_GPROC32_ID  ||
    kind == CV_SymKind_LPROC32_ID;
  return is_scope;
}

internal B32
cv_is_end_symbol(CV_SymKind kind)
{
  B32 is_end = kind == CV_SymKind_END         ||
    kind == CV_SymKind_PROC_ID_END ||
    kind == CV_SymKind_INLINESITE_END;
  return is_end;
}

internal B32
cv_is_leaf_type_server(CV_LeafKind kind)
{
  B32 is_type_server = kind == CV_LeafKind_TYPESERVER  ||
    kind == CV_LeafKind_TYPESERVER2 ||
    kind == CV_LeafKind_TYPESERVER_ST;
  return is_type_server;
}

internal B32
cv_is_leaf_pch(CV_LeafKind kind)
{
  B32 is_pch = kind == CV_LeafKind_PRECOMP    ||
    kind == CV_LeafKind_PRECOMP_ST ||
    kind == CV_LeafKind_PRECOMP_16t;
  return is_pch;
}

internal CV_TypeIndexSource
cv_type_index_source_from_leaf_kind(CV_LeafKind leaf_kind)
{
  CV_TypeIndexSource source;
  if(leaf_kind == CV_LeafKind_FUNC_ID      ||
     leaf_kind == CV_LeafKind_MFUNC_ID     ||
     leaf_kind == CV_LeafKind_BUILDINFO    ||
     leaf_kind == CV_LeafKind_SUBSTR_LIST  ||
     leaf_kind == CV_LeafKind_STRING_ID    ||
     leaf_kind == CV_LeafKind_UDT_SRC_LINE ||
     leaf_kind == CV_LeafKind_UDT_MOD_SRC_LINE)
  {
    source = CV_TypeIndexSource_IPI;
  }
  else if(leaf_kind == CV_LeafKind_NOTYPE)
  {
    source = CV_TypeIndexSource_NULL;
  }
  else
  {
    source = CV_TypeIndexSource_TPI;
  }
  return source;
}

internal CV_TypeIndexInfo *
cv_symbol_type_index_info_push(Arena *arena, CV_TypeIndexInfoList *list, CV_TypeIndexSource source, U64 offset)
{
  CV_TypeIndexInfo *info = push_array_no_zero(arena, CV_TypeIndexInfo, 1);
  info->next   = 0;
  info->offset = offset;
  info->source = source;
  
  SLLQueuePush(list->first, list->last, info);
  list->count += 1;
  
  return info;
}

internal CV_TypeIndexInfoList
cv_get_symbol_type_index_offsets(Arena *arena, CV_SymKind kind, String8 data)
{
  CV_TypeIndexInfoList list = {0};
  switch (kind) {
    case CV_SymKind_BUILDINFO: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, OffsetOf(CV_SymBuildInfo, id));
    } break;
    case CV_SymKind_GDATA32:
    case CV_SymKind_LDATA32: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymData32, itype));
    } break;
    case CV_SymKind_LPROC32_ID:
    case CV_SymKind_GPROC32_ID: 
    case CV_SymKind_LPROC32_DPC_ID: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, OffsetOf(CV_SymProc32, itype));
    } break;
    case CV_SymKind_GPROC32:
    case CV_SymKind_LPROC32: 
    case CV_SymKind_LPROC32_DPC: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymProc32, itype));
    } break;
    case CV_SymKind_UDT: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymUDT, itype));
    } break;
    case CV_SymKind_GTHREAD32: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymThread32, itype));
    } break;
    case CV_SymKind_FILESTATIC: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymFileStatic, itype));
    } break;
    case CV_SymKind_LOCAL: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymLocal, itype));
    } break;
    case CV_SymKind_REGREL32: 
    case CV_SymKind_BPREL32: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymRegrel32, itype));
    } break;
    case CV_SymKind_REGISTER: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymRegister, itype));
    } break;
    case CV_SymKind_CONSTANT: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymConstant, itype));
    } break;
    case CV_SymKind_CALLSITEINFO: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymCallSiteInfo, itype));
    } break;
    case CV_SymKind_CALLERS:
    case CV_SymKind_CALLEES:
    case CV_SymKind_INLINEES: {
      Assert(data.size >= sizeof(CV_SymFunctionList));
      CV_SymFunctionList *func_list = (CV_SymFunctionList*)data.str;
      for (U64 i = 0; i < func_list->count; ++i) {
        cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, sizeof(CV_SymFunctionList) + i * sizeof(CV_TypeIndex));
      }
    } break;
    case CV_SymKind_INLINESITE: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, OffsetOf(CV_SymInlineSite, inlinee));
    } break;
    case CV_SymKind_HEAPALLOCSITE: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_SymHeapAllocSite, itype));
    } break;
  }
  return list;
}

internal CV_TypeIndexInfoList
cv_get_leaf_type_index_offsets(Arena *arena, CV_LeafKind leaf_kind, String8 data)
{
  CV_TypeIndexInfoList list = {0};
  switch (leaf_kind) {
    case CV_LeafKind_NOTYPE:
    case CV_LeafKind_VTSHAPE:
    case CV_LeafKind_LABEL:
    case CV_LeafKind_NULL: 
    case CV_LeafKind_NOTTRAN: {
      // no type indices
    } break;
    case CV_LeafKind_MODIFIER: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafModifier, itype));
    } break;
    case CV_LeafKind_POINTER: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafPointer, itype));
      CV_LeafPointer *ptr = (CV_LeafPointer *)data.str;
      CV_PointerKind ptr_kind = CV_PointerAttribs_Extract_Kind(ptr->attribs);
      if (ptr_kind == CV_PointerKind_BaseType) {
        // TODO: add CV_LeafPointerBaseType
        cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, sizeof(CV_LeafPointer) + 0);
      } else {
        CV_PointerMode ptr_mode = CV_PointerAttribs_Extract_Mode(ptr->attribs);
        if (ptr_mode == CV_PointerMode_PtrMem || ptr_mode == CV_PointerMode_PtrMethod) {
          // TODO: add type for the CvLeafPointerMember to syms_cv.mc
          cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, sizeof(CV_LeafPointer) + 0);
        }
      }
    } break;
    case CV_LeafKind_ARRAY: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafArray, entry_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafArray, index_itype));
    } break;
    case CV_LeafKind_CLASS: 
    case CV_LeafKind_STRUCTURE:
    case CV_LeafKind_INTERFACE: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafStruct, field_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafStruct, derived_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafStruct, vshape_itype));
    } break;
    case CV_LeafKind_CLASS2:
    case CV_LeafKind_STRUCT2: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafStruct2, field_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafStruct2, derived_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafStruct2, vshape_itype));
    } break;
    case CV_LeafKind_UNION: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafUnion, field_itype));
    } break;
    case CV_LeafKind_ALIAS: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafAlias, itype));
    } break;
    case CV_LeafKind_FUNC_ID: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, OffsetOf(CV_LeafFuncId, scope_string_id));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafFuncId, itype));
    } break;
    case CV_LeafKind_MFUNC_ID: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafMFuncId, owner_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafMFuncId, itype));
    } break;
    case CV_LeafKind_STRING_ID: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, OffsetOf(CV_LeafStringId, substr_list_id));
    } break;
    case CV_LeafKind_UDT_SRC_LINE: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafUDTSrcLine, udt_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, OffsetOf(CV_LeafUDTSrcLine, src_string_id));
    } break;
    case CV_LeafKind_UDT_MOD_SRC_LINE: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafUDTModSrcLine, udt_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, OffsetOf(CV_LeafUDTModSrcLine, src_string_id));
    } break;
    case CV_LeafKind_BUILDINFO: {
      Assert(data.size >= sizeof(CV_LeafBuildInfo));
      CV_LeafBuildInfo *build_info = (CV_LeafBuildInfo *)data.str;
      for (U16 i = 0; i < build_info->count; ++i) {
        cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, sizeof(CV_LeafBuildInfo) + i * sizeof(CV_ItemId));
      }
    } break;
    case CV_LeafKind_ENUM: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafEnum, base_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafEnum, field_itype));
    } break;
    case CV_LeafKind_PROCEDURE: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafProcedure, ret_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafProcedure, arg_itype));
    } break;
    case CV_LeafKind_MFUNCTION: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafMFunction, ret_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafMFunction, class_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafMFunction, this_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafMFunction, arg_itype));
    } break;
    case CV_LeafKind_VFTABLE: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafVFTable, owner_itype));
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafVFTable, base_table_itype));
    } break;
    case CV_LeafKind_VFTPATH: {
      Assert(sizeof(CV_LeafVFPath) <= data.size);
      CV_LeafVFPath *vfpath = (CV_LeafVFPath *)data.str;
      for (U32 i = 0; i < vfpath->count; ++i) {
        cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, sizeof(CV_LeafVFPath) + i * sizeof(CV_TypeId));
      }
    } break;
    case CV_LeafKind_TYPESERVER:
    case CV_LeafKind_TYPESERVER2:
    case CV_LeafKind_TYPESERVER_ST: {
      // no type indices
    } break;
    case CV_LeafKind_SKIP: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafSkip, itype));
    } break;
    case CV_LeafKind_SUBSTR_LIST: {
      Assert(sizeof(CV_LeafArgList) <= data.size);
      CV_LeafArgList *arg_list = (CV_LeafArgList*)data.str;
      for (U32 i = 0; i < arg_list->count; ++i) {
        cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, sizeof(CV_LeafArgList) + i * sizeof(CV_TypeIndex));
      }
    } break;
    case CV_LeafKind_ARGLIST: {
      Assert(sizeof(CV_LeafArgList) <= data.size);
      CV_LeafArgList *arg_list = (CV_LeafArgList*)data.str;
      for (U32 i = 0; i < arg_list->count; ++i) {
        cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, sizeof(CV_LeafArgList) + i * sizeof(CV_TypeIndex));
      }
    } break;
    case CV_LeafKind_LIST: 
    case CV_LeafKind_FIELDLIST: {
      for (U64 cursor = 0; cursor < data.size; ) {
        CV_LeafKind list_member_kind = 0;
        U64 read_size = str8_deserial_read_struct(data, cursor, &list_member_kind);
        
        if(read_size != sizeof(list_member_kind)) {
          Assert(!"malformed LF_FIELDLIST");
          break;
        }
        cursor += read_size;
        
        switch (list_member_kind) {
          default: Assert(!"TODO: handle malformed field member"); break;
          case CV_LeafKind_INDEX: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafIndex, itype));
            cursor += sizeof(CV_LeafIndex);
          } break;
          case CV_LeafKind_MEMBER: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafMember, itype));
            cursor += sizeof(CV_LeafMember);
            
            CV_NumericParsed size;
            cursor += cv_read_numeric(data, cursor, &size);
            
            String8 name;
            cursor += str8_deserial_read_cstr(data, cursor, &name);
          } break;
          case CV_LeafKind_STMEMBER: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafStMember, itype));
            cursor += sizeof(CV_LeafStMember);
            
            String8 name;
            cursor += str8_deserial_read_cstr(data, cursor, &name);
          } break;
          case CV_LeafKind_METHOD: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafMethod, list_itype));
            cursor += sizeof(CV_LeafMethod);
            
            String8 name;
            cursor += str8_deserial_read_cstr(data, cursor, &name);
          } break;
          case CV_LeafKind_ONEMETHOD: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafOneMethod, itype));
            
            CV_LeafOneMethod onemethod;
            cursor += str8_deserial_read_struct(data, cursor, &onemethod);
            
            CV_MethodProp prop = CV_FieldAttribs_Extract_MethodProp(onemethod.attribs);
            if(prop == CV_MethodProp_PureIntro || prop == CV_MethodProp_Intro)
            {
              cursor += sizeof(U32); // virtoff
            }
            
            String8 name;
            cursor += str8_deserial_read_cstr(data, cursor, &name);
          } break;
          case CV_LeafKind_ENUMERATE: {
            // no type index
            cursor += sizeof(CV_LeafEnumerate);
            CV_NumericParsed value;
            cursor += cv_read_numeric(data, cursor, &value);
            String8 name;
            cursor += str8_deserial_read_cstr(data, cursor, &name);
          } break;
          case CV_LeafKind_NESTTYPE: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafNestType, itype));
            cursor += sizeof(CV_LeafNestType);
            
            String8 name;
            cursor += str8_deserial_read_cstr(data, cursor, &name);
          } break;
          case CV_LeafKind_NESTTYPEEX: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafNestTypeEx, itype));
            
            cursor += sizeof(CV_LeafNestTypeEx);
            String8 name;
            cursor += str8_deserial_read_cstr(data, cursor, &name);
          } break;
          case CV_LeafKind_BCLASS: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafBClass, itype));
            
            cursor += sizeof(CV_LeafBClass);
            CV_NumericParsed offset;
            cursor += cv_read_numeric(data, cursor, &offset);
          } break;
          case CV_LeafKind_VBCLASS:
          case CV_LeafKind_IVBCLASS: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafVBClass, itype));
            cursor += sizeof(CV_LeafVBClass);
            
            CV_NumericParsed virtual_base_pointer;
            cursor += cv_read_numeric(data, cursor, &virtual_base_pointer);
            
            CV_NumericParsed virtual_base_offset;
            cursor += cv_read_numeric(data, cursor, &virtual_base_offset);
          } break;
          case CV_LeafKind_VFUNCTAB: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafVFuncTab, itype));
            cursor += sizeof(CV_LeafVFuncTab);
          } break;
          case CV_LeafKind_VFUNCOFF: {
            cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafVFuncOff, itype));
            cursor += sizeof(CV_LeafVFuncOff);
          } break;
        }
        cursor = AlignPow2(cursor, 4);
      }
    } break;
    case CV_LeafKind_METHOD: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafMethod, list_itype));
    } break;
    case CV_LeafKind_METHODLIST: {
      for (U64 cursor = 0; cursor < data.size; ) {
        // read method
        CV_LeafMethodListMember method;
        U64 read_size = str8_deserial_read_struct(data, cursor, &method);
        
        // error check read
        if (read_size != sizeof(method)) {
          Assert(!"malformed LF_METHODLIST");
          break;
        }
        
        // push type index offset
        cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, cursor + OffsetOf(CV_LeafMethodListMember, itype));
        
        // take into account intro virtual offset
        CV_MethodProp mprop = CV_FieldAttribs_Extract_MethodProp(method.attribs);
        if (mprop == CV_MethodProp_Intro || mprop == CV_MethodProp_PureIntro) {
          read_size += sizeof(U32);
        }
        
        // advance
        cursor += read_size;
      }
    } break;
    case CV_LeafKind_ONEMETHOD: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafOneMethod, itype));
    } break;
    case CV_LeafKind_BITFIELD: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafBitField, itype));
    } break;
    case CV_LeafKind_PRECOMP:
    case CV_LeafKind_REFSYM: {
      // no type indices
    } break;
    case CV_LeafKind_INDEX: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafIndex, itype));
    } break;
    case CV_LeafKind_MEMBER: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafMember, itype));
    } break;
    case CV_LeafKind_VFUNCTAB: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafVFuncTab, itype));
    } break;
    case CV_LeafKind_VFUNCOFF: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafVFuncOff, itype));
    } break;
    case CV_LeafKind_NESTTYPE: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafNestType, itype));
    } break;
    case CV_LeafKind_NESTTYPEEX: {
      cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_TPI, OffsetOf(CV_LeafNestTypeEx, itype));
    } break;
    default: {
      NotImplemented;
    } break;
  }
  return list;
}

internal CV_TypeIndexInfoList
cv_get_inlinee_type_index_offsets(Arena *arena, String8 raw_data)
{
  CV_TypeIndexInfoList list = {0};
  
  U64 cursor = 0;
  
  // first four bytes are always signature
  CV_C13InlineeLinesSig sig = max_U32;
  cursor += str8_deserial_read_struct(raw_data, cursor, &sig);
  
  while(cursor < raw_data.size)
  {
    // read header
    CV_C13InlineeSourceLineHeader *header = (CV_C13InlineeSourceLineHeader *) str8_deserial_get_raw_ptr(raw_data, cursor, sizeof(CV_C13InlineeSourceLineHeader));
    
    // store type index offset
    cv_symbol_type_index_info_push(arena, &list, CV_TypeIndexSource_IPI, cursor + OffsetOf(CV_C13InlineeSourceLineHeader, inlinee));
    
    // advance past header
    cursor += sizeof(*header);
    
    // skip extra files
    B32 has_extra_files = (sig == CV_C13InlineeLinesSig_EXTRA_FILES);
    if (has_extra_files)
    {
      U32 file_count = 0;
      cursor += str8_deserial_read_struct(raw_data, cursor, &file_count);
      cursor += /* file id: */ sizeof(U32) * file_count;
    }
  }
  
  return list;
}

internal String8Array
cv_get_data_around_type_indices(Arena *arena, CV_TypeIndexInfoList ti_list, String8 data)
{
  String8Array result;
  if(ti_list.count > 0)
  {
    result.count = ti_list.count + 1;
    result.v = push_array_no_zero(arena, String8, result.count);
    
    U64 cursor = 0;
    U64 ti_idx = 0;
    
    for(CV_TypeIndexInfo *ti_info = ti_list.first; ti_info != 0; ti_info = ti_info->next, ++ti_idx)
    {
      result.v[ti_idx].size = ti_info->offset - cursor;
      result.v[ti_idx].str  = data.str + cursor;
      cursor = ti_info->offset + sizeof(CV_TypeIndex);
    }
    
    result.v[result.count-1].size = data.size - cursor;
    result.v[result.count-1].str  = data.str + cursor;
  }
  else
  {
    result.count = 1;
    result.v = push_array_no_zero(arena, String8, 1);
    result.v[0] = data;
  }
  return result;
}

internal U64
cv_name_offset_from_symbol(CV_SymKind kind, String8 data)
{
  U64 offset = data.size;
  switch (kind) {
    case CV_SymKind_COMPILE: break;
    case CV_SymKind_OBJNAME: break;
    case CV_SymKind_THUNK32: {
      offset = sizeof(CV_SymThunk32); 
    } break;
    case CV_SymKind_LABEL32: {
      offset = sizeof(CV_SymLabel32); 
    } break;
    case CV_SymKind_REGISTER: {
      offset = sizeof(CV_SymRegister); 
    } break;
    case CV_SymKind_CONSTANT: {
      offset = sizeof(CV_SymConstant);
      CV_NumericParsed size;
      offset += cv_read_numeric(data, offset, &size);
    } break;
    case CV_SymKind_UDT: {
      offset = sizeof(CV_SymUDT);
    } break;
    case CV_SymKind_BPREL32: {
      offset = sizeof(CV_SymBPRel32);
    } break;
    case CV_SymKind_LDATA32:
    case CV_SymKind_GDATA32: {
      offset = sizeof(CV_SymData32);
    } break;
    case CV_SymKind_PUB32: {
      offset = sizeof(CV_SymPub32);
    } break;
    case CV_SymKind_LPROC32: 
    case CV_SymKind_GPROC32: 
    case CV_SymKind_LPROC32_ID:
    case CV_SymKind_GPROC32_ID: {
      offset = sizeof(CV_SymProc32);
    } break;
    case CV_SymKind_REGREL32: {
      offset = sizeof(CV_SymRegrel32);
    } break;
    case CV_SymKind_LTHREAD32:
    case CV_SymKind_GTHREAD32: {
      offset = sizeof(CV_SymData32);
    } break;
    case CV_SymKind_COMPILE2: break;
    case CV_SymKind_LOCALSLOT: {
      offset = sizeof(CV_SymSlot);
    } break;
    case CV_SymKind_PROCREF: 
    case CV_SymKind_LPROCREF:
    case CV_SymKind_DATAREF: {
      offset = sizeof(CV_SymRef2);
    } break;
    case CV_SymKind_TRAMPOLINE: break;
    case CV_SymKind_LOCAL: {
      offset = sizeof(CV_SymLocal);
    } break;
    default: InvalidPath;
  }
  return offset;
}

internal String8
cv_name_from_symbol(CV_SymKind kind, String8 data)
{
  U64 buf_off = cv_name_offset_from_symbol(kind, data);
  U8 *buf_ptr = data.str + buf_off;
  U8 *buf_opl = data.str + data.size;
  String8 name = str8_cstring_capped(buf_ptr, buf_opl);
  return name;
}

internal CV_UDTInfo
cv_get_udt_info(CV_LeafKind kind, String8 data)
{
  String8      name        = str8_zero();
  String8      unique_name = str8_zero();
  CV_TypeProps props       = 0;
  
  switch(kind) {
    case CV_LeafKind_CLASS:
    case CV_LeafKind_STRUCTURE:
    case CV_LeafKind_INTERFACE: {
      U64 cursor = 0;
      
      CV_LeafStruct udt;
      cursor += str8_deserial_read_struct(data, cursor, &udt);
      
      props = udt.props;
      
      CV_NumericParsed size;
      cursor += cv_read_numeric(data, cursor, &size);
      
      cursor += str8_deserial_read_cstr(data, cursor, &name);
      
      if (udt.props & CV_TypeProp_HasUniqueName) {
        cursor += str8_deserial_read_cstr(data, cursor, &unique_name);
      }
    } break;
    
    case CV_LeafKind_CLASS2:
    case CV_LeafKind_STRUCT2: {
      U64 cursor = 0;
      
      CV_LeafStruct2 udt;
      cursor += str8_deserial_read_struct(data, cursor, &udt);
      
      props = udt.props;
      
      CV_NumericParsed size;
      cursor += cv_read_numeric(data, cursor, &size);
      
      cursor += str8_deserial_read_cstr(data, cursor, &name);
      
      if (udt.props & CV_TypeProp_HasUniqueName) {
        cursor += str8_deserial_read_cstr(data, cursor, &unique_name);
      }
    } break;
    
    case CV_LeafKind_UNION: {
      U64 cursor = 0;
      
      CV_LeafUnion udt;
      cursor += str8_deserial_read_struct(data, cursor, &udt);
      
      CV_NumericParsed size;
      cursor += cv_read_numeric(data, cursor, &size);
      
      props = udt.props;
      
      cursor += str8_deserial_read_cstr(data, cursor, &name);
      
      if(udt.props & CV_TypeProp_HasUniqueName) {
        cursor += str8_deserial_read_cstr(data, cursor, &unique_name);
      }
    } break;
    
    case CV_LeafKind_ENUM: {
      U64 cursor = 0;
      
      CV_LeafEnum udt;
      cursor += str8_deserial_read_struct(data, cursor, &udt);
      
      props = udt.props;
      
      cursor += str8_deserial_read_cstr(data, cursor, &name);
      
      if(udt.props & CV_TypeProp_HasUniqueName) {
        cursor += str8_deserial_read_cstr(data, cursor, &unique_name);
      }
    } break;
    
    // dbi/tpi.cpp:1332
    case CV_LeafKind_UDT_SRC_LINE: {
      CV_LeafUDTSrcLine *src_line = str8_deserial_get_raw_ptr(data, 0, sizeof(CV_LeafUDTSrcLine));
      name = str8_struct(&src_line->udt_itype);
    } break;
    case CV_LeafKind_UDT_MOD_SRC_LINE: {
      CV_LeafUDTModSrcLine *mod_src_line = str8_deserial_get_raw_ptr(data, 0, sizeof(CV_LeafUDTModSrcLine));
      name = str8_struct(&mod_src_line->udt_itype);
    } break;
    
    case CV_LeafKind_ALIAS: {
      str8_deserial_read_cstr(data, 0, &name);
    } break;
    
    default: {
      InvalidPath;
    } break;
  }
  
  CV_UDTInfo info  = {0};
  info.name        = name;
  info.unique_name = unique_name;
  info.props       = props;
  return info;
}

internal String8
cv_name_from_udt_info(CV_UDTInfo udt_info)
{
  if (udt_info.props & CV_TypeProp_HasUniqueName) {
    return udt_info.unique_name;
  }
  return udt_info.name;
}

//- rjf: record range stream parsing

internal CV_RecRangeStream*
cv_rec_range_stream_from_data(Arena *arena, String8 sym_data, U64 sym_align)
{
  Assert(1 <= sym_align && IsPow2OrZero(sym_align));
  CV_RecRangeStream *result = push_array(arena, CV_RecRangeStream, 1);
  U8 *data = sym_data.str;
  U64 cursor = 0;
  U64 cap = sym_data.size;
  for(;cursor + sizeof(CV_RecHeader) <= cap;)
  {
    // setup a new chunk
    CV_RecRangeChunk *cur_chunk = push_array_aligned(arena, CV_RecRangeChunk, 1, 64);
    SLLQueuePush(result->first_chunk, result->last_chunk, cur_chunk);
    U64 partial_count = 0;
    for(;partial_count < CV_REC_RANGE_CHUNK_SIZE && cursor + sizeof(CV_RecHeader) <= cap; partial_count += 1)
    {
      // compute cap
      CV_RecHeader *hdr = (CV_RecHeader*)(data + cursor);
      U64 symbol_cap_unclamped = cursor + 2 + hdr->size;
      U64 symbol_cap = ClampTop(symbol_cap_unclamped, cap);
      
      // push on range
      cur_chunk->ranges[partial_count].off = cursor + 2;
      cur_chunk->ranges[partial_count].hdr = *hdr;
      
      // update cursor
      U32 next_pos = AlignPow2(symbol_cap, sym_align);
      cursor = next_pos;
    }
    result->total_count += partial_count;
  }
  return result;
}

internal CV_RecRangeArray
cv_rec_range_array_from_stream(Arena *arena, CV_RecRangeStream *stream)
{
  U64 total_count = stream->total_count;
  CV_RecRange *ranges = push_array_no_zero_aligned(arena, CV_RecRange, total_count, 8);
  U64 idx = 0;
  for(CV_RecRangeChunk *chunk = stream->first_chunk; chunk != 0; chunk = chunk->next)
  {
    U64 copy_count_raw = total_count - idx;
    U64 copy_count = ClampTop(copy_count_raw, CV_REC_RANGE_CHUNK_SIZE);
    MemoryCopy(ranges + idx, chunk->ranges, copy_count*sizeof(CV_RecRange));
    idx += copy_count;
  }
  CV_RecRangeArray result = {0};
  result.ranges = ranges;
  result.count = total_count;
  return result;
}

//- rjf: sym stream parsing

internal CV_SymParsed *
cv_sym_from_data(Arena *arena, String8 sym_data, U64 sym_align)
{
  Assert(1 <= sym_align && IsPow2OrZero(sym_align));
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: gather symbols
  CV_RecRangeStream *stream = cv_rec_range_stream_from_data(scratch.arena, sym_data, sym_align);
  
  //- rjf: convert to result, fill basics
  CV_SymParsed *result = push_array(arena, CV_SymParsed, 1);
  result->data = sym_data;
  result->sym_align = sym_align;
  result->sym_ranges = cv_rec_range_array_from_stream(arena, stream);
  
  //- rjf: extract top-level-info
  {
    CV_RecRange *range = result->sym_ranges.ranges;
    CV_RecRange *opl = range + result->sym_ranges.count;
    for(;range < opl; range += 1)
    {
      U8 *first = sym_data.str + range->off + 2;
      U64 cap = range->hdr.size - 2;
      switch(range->hdr.kind)
      {
        case CV_SymKind_COMPILE:
        if(sizeof(CV_SymCompile) <= cap)
        {
          CV_SymCompile *compile = (CV_SymCompile*)first;
          String8 ver_str = str8_cstring_capped((char*)(compile + 1), (char *)(first + cap));
          result->info.arch = compile->machine;
          result->info.language = CV_CompileFlags_Extract_Language(compile->flags);;
          result->info.compiler_name = ver_str;
        }break;
        case CV_SymKind_COMPILE2:
        if(sizeof(CV_SymCompile2) <= cap)
        {
          CV_SymCompile2 *compile2 = (CV_SymCompile2*)first;
          String8 ver_str = str8_cstring_capped((char*)(compile2 + 1), (char*)(first + cap));
          String8 compiler_name = push_str8f(arena, "%.*s %u.%u.%u",
                                             str8_varg(ver_str),
                                             compile2->ver_major,
                                             compile2->ver_minor,
                                             compile2->ver_build);
          result->info.arch = compile2->machine;
          result->info.language = CV_Compile2Flags_Extract_Language(compile2->flags);;
          result->info.compiler_name = compiler_name;
        }break;
        case CV_SymKind_COMPILE3:
        if(sizeof(CV_SymCompile3) <= cap)
        {
          CV_SymCompile3 *compile3 = (CV_SymCompile3*)first;
          String8 ver_str = str8_cstring_capped((char*)(compile3 + 1), (char *)(first + cap));
          String8 compiler_name = push_str8f(arena, "%.*s %u.%u.%u",
                                             str8_varg(ver_str),
                                             compile3->ver_major,
                                             compile3->ver_minor,
                                             compile3->ver_build);
          result->info.arch = compile3->machine;
          result->info.language = CV_Compile3Flags_Extract_Language(compile3->flags);;
          result->info.compiler_name = compiler_name;
        }break;
      }
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

//- rjf: leaf stream parsing

internal CV_LeafParsed *
cv_leaf_from_data(Arena *arena, String8 leaf_data, CV_TypeId itype_first)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  // gather up symbols
  CV_RecRangeStream *stream = cv_rec_range_stream_from_data(scratch.arena, leaf_data, 1);
  
  // convert to result
  CV_LeafParsed *result = push_array(arena, CV_LeafParsed, 1);
  result->data = leaf_data;
  result->itype_first = itype_first;
  result->itype_opl = itype_first + stream->total_count;
  result->leaf_ranges = cv_rec_range_array_from_stream(arena, stream);
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal CV_C13Parsed *
cv_c13_parsed_from_data(Arena *arena, String8 c13_data, String8 strtbl, COFF_SectionHeaderArray sections)
{
  ProfBeginFunction();
  
  //////////////////////////////
  //- rjf: gather c13 sub-sections
  //
  CV_C13SubSectionNode *file_chksms = 0;
  CV_C13SubSectionNode *first = 0;
  CV_C13SubSectionNode *last = 0;
  U64 count = 0;
  {
    U32 cursor = 0;
    for(; cursor + sizeof(CV_C13SubSectionHeader) <= c13_data.size;)
    {
      // read header
      CV_C13SubSectionHeader *hdr = (CV_C13SubSectionHeader*)(c13_data.str + cursor);
      
      // get sub section info
      U32 sub_section_off = cursor + sizeof(*hdr);
      U32 sub_section_size_raw = hdr->size;
      U32 after_sub_section_off_unclamped = sub_section_off + sub_section_size_raw;
      U32 after_sub_section_off = ClampTop(after_sub_section_off_unclamped, c13_data.size);
      U32 sub_section_size = after_sub_section_off - sub_section_off;
      
      // emit sub section
      if(!(hdr->kind & CV_C13SubSectionKind_IgnoreFlag))
      {
        CV_C13SubSectionNode *node = push_array(arena, CV_C13SubSectionNode, 1);
        SLLQueuePush(first, last, node);
        count += 1;
        node->kind = hdr->kind;
        node->off = sub_section_off;
        node->size = sub_section_size;
        if(hdr->kind == CV_C13SubSectionKind_FileChksms)
        {
          file_chksms = node;
        }
      }
      
      // move cursor
      cursor = AlignPow2(after_sub_section_off, 4);
    }
  }
  
  //////////////////////////////
  //- rjf: parse each sub-section
  //
  U64 inlinee_lines_parsed_slots_count = 4096;
  CV_C13InlineeLinesParsedNode **inlinee_lines_parsed_slots = push_array(arena, CV_C13InlineeLinesParsedNode *, inlinee_lines_parsed_slots_count);
  for(CV_C13SubSectionNode *node = first;
      node != 0;
      node = node->next)
  {
    U8 *first = c13_data.str + node->off;
    U32 cap = node->size;
    switch(node->kind)
    {
      default:{}break;
      
      //////////////////////////
      //- rjf: line info sub-section
      //
      case CV_C13SubSectionKind_Lines:
      if(sizeof(CV_C13SubSecLinesHeader) <= cap)
      {
        // read header
        U32 read_off = 0;
        U64 read_off_opl = node->size;
        CV_C13SubSecLinesHeader *hdr = (CV_C13SubSecLinesHeader*)(first + read_off);
        read_off += sizeof(*hdr);
        
        // rjf: extract section index
        U32 sec_idx = hdr->sec;
        
        // rjf: bad section index -> skip
        if(sec_idx < 1 || sections.count < sec_idx)
        {
          continue;
        }
        
        // extract top level info
        B32 has_cols = !!(hdr->flags & CV_C13SubSecLinesFlag_HasColumns);
        U64 secrel_off = hdr->sec_off;
        U64 secrel_opl = secrel_off + hdr->len;
        U64 sec_base_off = sections.v[sec_idx - 1].voff;
        
        // read files
        for(;read_off+sizeof(CV_C13File) <= read_off_opl;)
        {
          // rjf: grab next file header
          CV_C13File *file = (CV_C13File*)(first + read_off);
          U32 file_off = file->file_off;
          U32 line_count_unclamped = file->num_lines;
          U32 block_size = file->block_size;
          
          // file_name from file_off
          String8 file_name = {0};
          if(file_off + sizeof(CV_C13Checksum) <= file_chksms->size)
          {
            CV_C13Checksum *checksum = (CV_C13Checksum*)(c13_data.str + file_chksms->off + file_off);
            U32 name_off = checksum->name_off;
            file_name =  str8_cstring_capped((char*)(strtbl.str + name_off),
                                             (char*)(strtbl.str + strtbl.size));
          }
          
          // array layouts
          U32 line_item_size = sizeof(CV_C13Line);
          if (has_cols){
            line_item_size += sizeof(CV_C13Column);
          }
          
          U32 line_array_off = read_off + sizeof(*file);
          U32 line_count_max = (read_off_opl - line_array_off) / line_item_size;
          U32 line_count = ClampTop(line_count_unclamped, line_count_max);
          
          U32 col_array_off = line_array_off + line_count*sizeof(CV_C13Line);
          
          // parse lines
          U64 *voffs = push_array_no_zero(arena, U64, line_count + 1);
          U32 *line_nums = push_array_no_zero(arena, U32, line_count);
          
          {
            CV_C13Line *line_ptr = (CV_C13Line*)(first + line_array_off);
            CV_C13Line *line_opl = line_ptr + line_count;
            
            // TODO(allen): check order correctness here
            
            U32 i = 0;
            for (; line_ptr < line_opl; line_ptr += 1, i += 1){
              voffs[i] = line_ptr->off + secrel_off + sec_base_off;
              line_nums[i] = CV_C13LineFlags_Extract_LineNumber(line_ptr->flags);
            }
            voffs[i] = secrel_opl + sec_base_off;
          }
          
          // emit parsed lines
          CV_C13LinesParsedNode *lines_parsed_node = push_array(arena, CV_C13LinesParsedNode, 1);
          CV_C13LinesParsed *lines_parsed = &lines_parsed_node->v;
          lines_parsed->sec_idx = sec_idx;
          lines_parsed->file_off = file_off;
          lines_parsed->secrel_base_off = secrel_off;
          lines_parsed->file_name = file_name;
          lines_parsed->voffs  = voffs;
          lines_parsed->line_nums = line_nums;
          lines_parsed->line_count = line_count;
          SLLQueuePush(node->lines_first, node->lines_last, lines_parsed_node);
          
          // rjf: advance
          read_off += sizeof(*file);
          read_off += line_item_size*line_count;
        }
      }break;
      
      //////////////////////////
      //- rjf: inlinee line info sub-section
      //
      case CV_C13SubSectionKind_InlineeLines:
      if(sizeof(CV_C13InlineeLinesSig) <= cap)
      {
        // rjf: read sig
        U32 read_off = 0;
        U64 read_off_opl = node->size;
        CV_C13InlineeLinesSig *sig = (CV_C13InlineeLinesSig *)(first + read_off);
        read_off += sizeof(*sig);
        
        // rjf: read source lines
        for(;read_off + sizeof(CV_C13InlineeSourceLineHeader) <= read_off_opl;)
        {
          // rjf: read next header
          CV_C13InlineeSourceLineHeader *hdr = (CV_C13InlineeSourceLineHeader *)(first + read_off);
          read_off += sizeof(*hdr);
          
          // rjf: file_off -> file_name
          String8 file_name = {0};
          if(hdr->file_off + sizeof(CV_C13Checksum) <= file_chksms->size)
          {
            CV_C13Checksum *checksum = (CV_C13Checksum*)(c13_data.str + file_chksms->off + hdr->file_off);
            U32 name_off = checksum->name_off;
            file_name =  str8_cstring_capped((char*)(strtbl.str + name_off),
                                             (char*)(strtbl.str + strtbl.size));
          }
          
          // rjf: parse extra files
          U32 extra_file_count = 0;
          U32 *extra_files = 0;
          if(*sig == CV_C13InlineeLinesSig_EXTRA_FILES && read_off+sizeof(U32) <= read_off_opl)
          {
            U32 *extra_file_count_ptr = (U32 *)(first + read_off);
            read_off += sizeof(*extra_file_count_ptr);
            U32 max_extra_file_count = (read_off_opl-read_off)/sizeof(U32);
            extra_file_count = Min(*extra_file_count_ptr, max_extra_file_count);
            extra_files      = (U32 *)(first + read_off);
            read_off += sizeof(*extra_files)*extra_file_count;
          }
          
          // rjf: push node for this inlinee lines parsed into this subsection's list
          CV_C13InlineeLinesParsedNode *n = push_array(arena, CV_C13InlineeLinesParsedNode, 1);
          SLLQueuePush(node->inlinee_lines_first, node->inlinee_lines_last, n);
          n->v.inlinee          = hdr->inlinee;
          n->v.file_name        = file_name;
          n->v.file_off         = hdr->file_off;
          n->v.first_source_ln  = hdr->first_source_ln;
          n->v.extra_file_count = extra_file_count;
          n->v.extra_files      = extra_files;
          
          // rjf: push node into inlinee parse hash table
          U64 hash = cv_hash_from_item_id(hdr->inlinee);
          U64 slot_idx = hash%inlinee_lines_parsed_slots_count;
          SLLStackPush_N(inlinee_lines_parsed_slots[slot_idx], n, hash_next);
        }
      }break;
    }
  }
  
  //////////////////////////////
  //- rjf: fill output
  //
  CV_C13Parsed *result = push_array(arena, CV_C13Parsed, 1);
  result->data = c13_data;
  result->first_sub_section = first;
  result->last_sub_section = last;
  result->sub_section_count = count;
  result->file_chksms_sub_section = file_chksms;
  result->inlinee_lines_parsed_slots = inlinee_lines_parsed_slots;
  result->inlinee_lines_parsed_slots_count = inlinee_lines_parsed_slots_count;
  ProfEnd();
  return result;
}

