// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ Generated Code

#include "generated/codeview.meta.c"

////////////////////////////////
//~ Common Functions

internal CV_NumericParsed
cv_numeric_from_data_range(U8 *first, U8 *opl){
  CV_NumericParsed result = {0};
  if (first + 2 <= opl){
    U16 x = *(U16*)first;
    if (x < 0x8000){
      result.kind = CV_NumericKind_USHORT;
      result.val = first;
      result.encoded_size = 2;
    }
    else{
      U64 val_size = 0;
      switch (x){
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
      
      if (first + 2 + val_size <= opl){
        result.kind = x;
        result.val = (first + 2);
        result.encoded_size = 2 + val_size;
      }
    }
  }
  return(result);
}

internal B32
cv_numeric_fits_in_u64(CV_NumericParsed *num){
  B32 result = 0;
  switch (num->kind){
    case CV_NumericKind_USHORT:
    case CV_NumericKind_ULONG:
    case CV_NumericKind_UQUADWORD:
    {
      result = 1;
    }break;
  }
  return(result);
}

internal B32
cv_numeric_fits_in_s64(CV_NumericParsed *num){
  B32 result = 0;
  switch (num->kind){
    case CV_NumericKind_CHAR:
    case CV_NumericKind_SHORT:
    case CV_NumericKind_LONG:
    case CV_NumericKind_QUADWORD:
    {
      result = 1;
    }break;
  }
  return(result);
}

internal B32
cv_numeric_fits_in_f64(CV_NumericParsed *num){
  B32 result = 0;
  switch (num->kind){
    case CV_NumericKind_FLOAT32:
    case CV_NumericKind_FLOAT64:
    {
      result = 1;
    }break;
  }
  return(result);
}

internal U64
cv_u64_from_numeric(CV_NumericParsed *num){
  U64 result = 0;
  switch (num->kind){
    case CV_NumericKind_USHORT:
    {
      result = *(U16*)num->val;
    }break;
    
    case CV_NumericKind_ULONG:
    {
      result = *(U32*)num->val;
    }break;
    
    case CV_NumericKind_UQUADWORD:
    {
      result = *(U64*)num->val;
    }break;
  }
  return(result);
}

internal S64
cv_s64_from_numeric(CV_NumericParsed *num){
  S64 result = 0;
  switch (num->kind){
    case CV_NumericKind_CHAR:
    {
      result = *(S8*)num->val;
    }break;
    
    case CV_NumericKind_SHORT:
    {
      result = *(S16*)num->val;
    }break;
    
    case CV_NumericKind_LONG:
    {
      result = *(S32*)num->val;
    }break;
    
    case CV_NumericKind_QUADWORD:
    {
      result = *(S64*)num->val;
    }break;
  }
  return(result);
}

internal F64
cv_f64_from_numeric(CV_NumericParsed *num){
  F64 result = 0;
  switch (num->kind){
    case CV_NumericKind_FLOAT32:
    {
      result = *(F32*)num->val;
    }break;
    
    case CV_NumericKind_FLOAT64:
    {
      result = *(F64*)num->val;
    }break;
  }
  return(result);
}

////////////////////////////////
//~ Inline Binary Annotation Helpers

internal U64
cv_decode_inline_annot_u32(String8 data, U64 offset, U32 *out_value)
{
  U32 value;

  U64 cursor = offset;

  U8 header = 0;
  cursor += str8_deserial_read_struct(data, cursor, &header);

  // 1 byte
  if((header & 0x80) == 0)
  {
    value = header;
  }
  // 2 bytes
  else if((header & 0xC0) == 0x80)
  {
    Assert(cursor + sizeof(U8) * 2 <= data.size);
    U8 second_byte;
    cursor += str8_deserial_read_struct(data, cursor, &second_byte);
    value = ((header & 0x3F) << 8) | second_byte;
  }
  // 4 bytes
  else if((header & 0xE0) == 0xC0)
  {
    Assert(cursor + sizeof(U8) * 3 <= data.size);
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

  *out_value = value;

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

////////////////////////////////
//~ Sym Parser Functions

//- the first pass parser

internal CV_RecRangeStream*
cv_rec_range_stream_from_data(Arena *arena, String8 sym_data, U64 sym_align){
  Assert(1 <= sym_align && IsPow2OrZero(sym_align));
  
  CV_RecRangeStream *result = push_array(arena, CV_RecRangeStream, 1);
  
  U8 *data = sym_data.str;
  U64 cursor = 0;
  U64 cap = sym_data.size;
  for (;cursor + sizeof(CV_RecHeader) <= cap;){
    // setup a new chunk
    arena_push_align(arena, 64);
    CV_RecRangeChunk *cur_chunk = cv_rec_range_stream_push_chunk(arena, result);
    
    U64 partial_count = 0;
    for (;partial_count < CV_REC_RANGE_CHUNK_SIZE && cursor + sizeof(CV_RecHeader) <= cap;
         partial_count += 1){
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
  
  return(result);
}

//- sym

internal CV_SymParsed*
cv_sym_from_data(Arena *arena, String8 sym_data, U64 sym_align){
  Assert(1 <= sym_align && IsPow2OrZero(sym_align));
  ProfBegin("cv_sym_from_data");
  
  Temp scratch = scratch_begin(&arena, 1);
  
  // gather up symbols
  CV_RecRangeStream *stream = cv_rec_range_stream_from_data(scratch.arena, sym_data, sym_align);
  
  // convert to result
  CV_SymParsed *result = push_array(arena, CV_SymParsed, 1);
  result->data = sym_data;
  result->sym_align = sym_align;
  result->sym_ranges = cv_rec_range_array_from_stream(arena, stream);
  cv_sym_top_level_info_from_syms(arena, sym_data, &result->sym_ranges, &result->info);
  
  scratch_end(scratch);
  
  ProfEnd();
  
  return(result);
}

internal void
cv_sym_top_level_info_from_syms(Arena *arena, String8 sym_data,
                                CV_RecRangeArray *ranges,
                                CV_SymTopLevelInfo *info_out){
  MemoryZeroStruct(info_out);
  
  CV_RecRange *range = ranges->ranges;
  CV_RecRange *opl = range + ranges->count;
  for (; range < opl; range += 1){
    U8 *first = sym_data.str + range->off + 2;
    U64 cap = range->hdr.size - 2;
    
    switch (range->hdr.kind){
      case CV_SymKind_COMPILE:
      {
        if (sizeof(CV_SymCompile) <= cap){
          CV_SymCompile *compile = (CV_SymCompile*)first;
          
          String8 ver_str = str8_cstring_capped((char*)(compile + 1), (char *)(first + cap));
          
          info_out->arch = compile->machine;
          info_out->language = CV_CompileFlags_ExtractLanguage(compile->flags);;
          info_out->compiler_name = ver_str;
        }
      }break;
      
      case CV_SymKind_COMPILE2:
      {
        if (sizeof(CV_SymCompile2) <= cap){
          CV_SymCompile2 *compile2 = (CV_SymCompile2*)first;
          
          String8 ver_str = str8_cstring_capped((char*)(compile2 + 1), (char*)(first + cap));
          String8 compiler_name = push_str8f(arena, "%.*s %u.%u.%u",
                                             str8_varg(ver_str),
                                             compile2->ver_major,
                                             compile2->ver_minor,
                                             compile2->ver_build);
          
          info_out->arch = compile2->machine;
          info_out->language = CV_Compile2Flags_ExtractLanguage(compile2->flags);;
          info_out->compiler_name = compiler_name;
        }
      }break;
      
      case CV_SymKind_COMPILE3:
      {
        if (sizeof(CV_SymCompile3) <= cap){
          CV_SymCompile3 *compile3 = (CV_SymCompile3*)first;
          
          String8 ver_str = str8_cstring_capped((char*)(compile3 + 1), (char *)(first + cap));
          String8 compiler_name = push_str8f(arena, "%.*s %u.%u.%u",
                                             str8_varg(ver_str),
                                             compile3->ver_major,
                                             compile3->ver_minor,
                                             compile3->ver_build);
          
          info_out->arch = compile3->machine;
          info_out->language = CV_Compile3Flags_ExtractLanguage(compile3->flags);;
          info_out->compiler_name = compiler_name;
        }
      }break;
    }
  }
}

//- leaf

internal CV_LeafParsed*
cv_leaf_from_data(Arena *arena, String8 leaf_data, CV_TypeId itype_first){
  ProfBegin("cv_leaf_from_data");
  
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
  
  return(result);
}

//- range streams

internal CV_RecRangeChunk*
cv_rec_range_stream_push_chunk(Arena *arena, CV_RecRangeStream *stream){
  CV_RecRangeChunk *result = push_array_no_zero(arena, CV_RecRangeChunk, 1);
  SLLQueuePush(stream->first_chunk, stream->last_chunk, result);
  return(result);
}

internal CV_RecRangeArray
cv_rec_range_array_from_stream(Arena *arena, CV_RecRangeStream *stream){
  U64 total_count = stream->total_count;
  CV_RecRange *ranges = push_array_no_zero(arena, CV_RecRange, total_count);
  U64 idx = 0;
  for (CV_RecRangeChunk *chunk = stream->first_chunk;
       chunk != 0;
       chunk = chunk->next){
    U64 copy_count_raw = total_count - idx;
    U64 copy_count = ClampTop(copy_count_raw, CV_REC_RANGE_CHUNK_SIZE);
    MemoryCopy(ranges + idx, chunk->ranges, copy_count*sizeof(CV_RecRange));
    idx += copy_count;
  }
  CV_RecRangeArray result = {0};
  result.ranges = ranges;
  result.count = total_count;
  return(result);
}

////////////////////////////////
//~ C13 Parser Functions

internal void
cv_c13_sub_section_list_push_node(CV_C13SubSectionList *list, CV_C13SubSectionNode *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal CV_C13SubSectionNode *
cv_c13_sub_section_list_push(Arena *arena, CV_C13SubSectionList *list, CV_C13_SubSectionKind kind, U32 off, U32 size)
{
  CV_C13SubSectionNode *n = push_array(arena, CV_C13SubSectionNode, 1);
  n->next = 0;
  n->kind = kind;
  n->range = rng_1u64(off, off + size);

  cv_c13_sub_section_list_push_node(list, n);

  return n;
}

internal CV_C13_SubSectionIdxKind
cv_c13_sub_section_idx_from_kind(CV_C13_SubSectionKind kind)
{
  switch(kind)
  {
#define X(n,c) case CV_C13_SubSectionKind_##n: return CV_C13_SubSectionIdxKind_##n;
    CV_C13_SubSectionKindXList(X)
#undef X
  }
  return CV_C13_SubSectionIdxKind_NULL;
}

internal CV_C13SubSectionList
cv_c13_sub_section_list_from_data(Arena *arena, String8 data, U64 align)
{
  CV_C13SubSectionList list = {0};

  for(U64 cursor = 0; cursor + sizeof(CV_C13_SubSectionHeader) <= data.size; )
  {
    // read header
    CV_C13_SubSectionHeader *hdr = (CV_C13_SubSectionHeader *)(data.str + cursor);

    // get sub section info
    U32 sub_section_off                 = cursor + sizeof(*hdr);
    U32 after_sub_section_off_unclamped = sub_section_off + hdr->size;
    U32 after_sub_section_off           = ClampTop(after_sub_section_off_unclamped, data.size);
    U32 sub_section_size                = after_sub_section_off - sub_section_off;

    cv_c13_sub_section_list_push(arena, &list, hdr->kind, sub_section_off, sub_section_size);

    // move cursor
    cursor = AlignPow2(after_sub_section_off, align);
  }

  return list;
}

internal CV_C13Parsed
cv_c13_parsed_from_list(CV_C13SubSectionList *list)
{
  ProfBeginFunction();

  CV_C13Parsed ss = {0};

  for(CV_C13SubSectionNode *curr = list->first, *next = 0; curr != 0; curr = next)
  { 
    next = curr->next;

    CV_C13_SubSectionIdxKind idx;
    if(curr->kind & CV_C13_SubSectionKind_IgnoreFlag)
    {
      idx = CV_C13_SubSectionIdxKind_NULL;
    }
    else
    {
      idx = cv_c13_sub_section_idx_from_kind(curr->kind);
    }

    cv_c13_sub_section_list_push_node(&ss.v[idx], curr);
  }

  // clear input list
  MemoryZeroStruct(list);

  ProfEnd();
  return ss;
}

internal String8
cv_c13_file_chksms_from_sub_sections(String8 c13_data, CV_C13Parsed *ss)
{
  ProfBeginFunction();

  String8 file_chksms = str8(0,0);

  CV_C13SubSectionList file_chksms_list = ss->v[CV_C13_SubSectionIdxKind_FileChksms];
  if(file_chksms_list.count > 0)
  {
    Assert(file_chksms_list.count == 1);
    CV_C13SubSectionNode *file_chksms_node = file_chksms_list.first;
    Assert(file_chksms_node->kind == CV_C13_SubSectionKind_FileChksms);
    file_chksms = str8_substr(c13_data, file_chksms_node->range);
  }

  ProfEnd();
  return file_chksms;
}

internal CV_C13LinesParsedList
cv_c13_lines_from_sub_sections(Arena *arena, String8 c13_data, Rng1U64 ss_range)
{
  ProfBeginFunction();

  CV_C13LinesParsedList parsed_line_list = {0};

  String8 sub_sect_data  = str8_substr(c13_data, ss_range);

  for(U64 cursor = 0; cursor + sizeof(CV_C13_SubSecLinesHeader) <= sub_sect_data.size; )
  {
    CV_C13_SubSecLinesHeader *hdr = (CV_C13_SubSecLinesHeader *)(sub_sect_data.str + cursor);
    cursor += sizeof(*hdr);

    // read files
    for(; cursor + sizeof(CV_C13_File) <= sub_sect_data.size; )
    {
      // grab next file header
      CV_C13_File *file = (CV_C13_File *)(sub_sect_data.str + cursor);
      cursor += sizeof(CV_C13_File);

      // parse lines and columns
      //
      // TODO: export columns
      U64  max_line_count = (sub_sect_data.size - cursor) / sizeof(CV_C13_Line);
      U32  line_count     = Min(file->num_lines, max_line_count);

      // TODO(allen): check order correctness here

      U64 line_array_off = cursor;
      //U64 col_array_off  = line_array_off + line_count * sizeof(CV_C13_Line);

      // compute line entry size
      U64 line_entry_size = sizeof(CV_C13_Line);
      if(hdr->flags & CV_C13_SubSecLinesFlag_HasColumns)
      {
        line_entry_size += sizeof(CV_C13_Column);
      }

      // advance past line and column entries
      cursor += line_count * line_entry_size;

      // emit parsed lines
      CV_C13LinesParsedNode *lines_parsed_node = push_array_no_zero(arena, CV_C13LinesParsedNode, 1);
      lines_parsed_node->next = 0;

      CV_C13LinesParsed *lines_parsed = &lines_parsed_node->v;
      lines_parsed->sec_idx        = hdr->sec;
      lines_parsed->sec_off_lo     = hdr->sec_off;
      lines_parsed->sec_off_hi     = hdr->sec_off + hdr->len;
      lines_parsed->file_off       = file->file_off;
      lines_parsed->line_count     = line_count;
      lines_parsed->line_array_off = ss_range.min + line_array_off;
      lines_parsed->col_array_off  = 0;

      SLLQueuePush(parsed_line_list.first, parsed_line_list.last, lines_parsed_node);
      parsed_line_list.count += 1;
    }
  }

  ProfEnd();
  return parsed_line_list;
}

internal CV_C13LineArray
cv_c13_line_array_from_data(Arena *arena, String8 c13_data, U64 sec_base, CV_C13LinesParsed parsed_lines)
{
  CV_C13LineArray result;
  result.line_count = parsed_lines.line_count;
  result.col_count  = parsed_lines.col_count;
  result.voffs      = push_array_no_zero(arena, U64, parsed_lines.line_count + 1);
  result.line_nums  = push_array_no_zero(arena, U32, parsed_lines.line_count);
  result.col_nums   = 0;

  CV_C13_Line *raw_lines = (CV_C13_Line *)str8_deserial_get_raw_ptr(c13_data, parsed_lines.line_array_off, parsed_lines.line_count * sizeof(raw_lines[0]));

  for(U64 line_idx = 0; line_idx < parsed_lines.line_count; line_idx += 1)
  {
    CV_C13_Line line = raw_lines[line_idx];
    result.voffs[line_idx]     = sec_base + parsed_lines.sec_off_lo + line.off;
    result.line_nums[line_idx] = CV_C13_LineFlags_ExtractLineNumber(line.flags);
  }

  // emit voff ender
  result.voffs[result.line_count] = sec_base + parsed_lines.sec_off_hi;

  return result;
}

internal CV_C13InlineeLinesParsedList
cv_c13_inlinee_lines_from_sub_sections(Arena   *arena,
                                       String8  c13_data,
                                       U64      sub_sect_off,
                                       U64      sub_sect_size)
{
  ProfBeginFunction();

  CV_C13InlineeLinesParsedList inlinee_lines_list = {0};

  Rng1U64 sub_sect_range = rng_1u64(sub_sect_off, sub_sect_off + sub_sect_size);
  String8 sub_sect_data  = str8_substr(c13_data, sub_sect_range);
  U64     cursor         = 0;

  CV_C13_InlineeLinesSig sig = 0;
  cursor += str8_deserial_read_struct(sub_sect_data, cursor, &sig);

  for(; cursor + sizeof(CV_C13_InlineeSourceLineHeader) <= sub_sect_size; )
  {
    CV_C13_InlineeSourceLineHeader *hdr = (CV_C13_InlineeSourceLineHeader *)(sub_sect_data.str + cursor);
    cursor += sizeof(*hdr);

    CV_C13InlineeLinesParsedNode *inlinee_parsed_node = push_array_no_zero(arena, CV_C13InlineeLinesParsedNode, 1);
    inlinee_parsed_node->next = 0;
    SLLQueuePush(inlinee_lines_list.first, inlinee_lines_list.last, inlinee_parsed_node);
    inlinee_lines_list.count += 1;

    CV_C13InlineeLinesParsed *inlinee_parsed = &inlinee_parsed_node->v;
    inlinee_parsed->inlinee          = hdr->inlinee;
    inlinee_parsed->file_off         = hdr->file_off;
    inlinee_parsed->first_source_ln  = hdr->first_source_ln;
    inlinee_parsed->extra_file_count = 0;
    inlinee_parsed->extra_files      = 0;

    if(sig == CV_C13_InlineeLinesSig_EXTRA_FILES)
    {
      if(cursor + sizeof(U32) <= sub_sect_size)
      {
        U32 *extra_file_count_ptr = (U32 *)(sub_sect_data.str + cursor);
        cursor += sizeof(*extra_file_count_ptr);

        U32 max_extra_file_count = (sub_sect_size - cursor) / sizeof(U32);
        U32 extra_file_count     = Min(*extra_file_count_ptr, max_extra_file_count);
        U32 *extra_files         = (U32 *)(sub_sect_data.str + cursor);
        cursor += sizeof(*extra_files) * extra_file_count;

        inlinee_parsed->extra_file_count = extra_file_count;
        inlinee_parsed->extra_files      = extra_files;
      }
    }
  }

  ProfEnd();
  return inlinee_lines_list;
}

//- $$INLINEE_LINES Accel

internal U64
cv_c13_inlinee_lines_accel_hash(void *buffer, U64 size)
{
  return rdi_hash((U8 *)buffer, size);
}

internal B32
cv_c13_inlinee_lines_accel_push(CV_C13InlineeLinesAccel *accel, CV_C13InlineeLinesParsed *parsed)
{
  U64 load_factor = accel->bucket_max * 2/3 + 1;  
  if(accel->bucket_count > load_factor)
  {
    Assert("TODO: increase max count and rehash buckets");
  }

  B32 is_pushed = 0;

  U64 hash     = cv_c13_inlinee_lines_accel_hash(&parsed->inlinee, sizeof(parsed->inlinee));
  U64 best_idx = hash % accel->bucket_max;
  U64 idx      = best_idx;

  do
  {
    if(accel->buckets[idx] == 0)
    {
      accel->buckets[idx] = parsed;
      accel->bucket_count += 1;
      is_pushed = 1;
      break;
    }

    idx = (idx + 1) % accel->bucket_max;
  } while(idx != best_idx);

  return is_pushed;
}

internal CV_C13InlineeLinesParsed *
cv_c13_inlinee_lines_accel_find(CV_C13InlineeLinesAccel *accel, CV_ItemId inlinee)
{
  CV_C13InlineeLinesParsed *match = 0;

  U64 hash     = cv_c13_inlinee_lines_accel_hash(&inlinee, sizeof(inlinee));
  U64 best_idx = hash % accel->bucket_max;
  U64 idx      = best_idx;

  do
  {
    if(accel->buckets[idx] != 0)
    {
      if(accel->buckets[idx]->inlinee == inlinee)
      {
        match = accel->buckets[idx]; 
        break;
      }
    }

    idx = (idx + 1) % accel->bucket_max;
  } while(idx != best_idx);

  return match;
}

internal CV_C13InlineeLinesAccel *
cv_c13_make_inlinee_lines_accel(Arena *arena, String8 c13_data, CV_C13InlineeLinesParsedList inlinee_lines)
{
  ProfBeginFunction();

  // alloc hash table
  CV_C13InlineeLinesAccel *accel = push_array_no_zero(arena, CV_C13InlineeLinesAccel, 1);
  accel->bucket_count = 0;
  accel->bucket_max   = (U64)((F64)inlinee_lines.count * 1.3);
  accel->buckets      = push_array(arena, CV_C13InlineeLinesParsed *, accel->bucket_max);

  // push parsed inlinees
  for(CV_C13InlineeLinesParsedNode *inlinee = inlinee_lines.first; inlinee != 0; inlinee = inlinee->next)
  {
    if(cv_c13_inlinee_lines_accel_find(accel, inlinee->v.inlinee))
    {
      Assert(!"TODO: compiler/linker produced duplicate inlinees in $$INLINEE_LINES");
    }
    else
    {
      cv_c13_inlinee_lines_accel_push(accel, &inlinee->v);
    }
  }

  ProfEnd();
  return accel;
}

