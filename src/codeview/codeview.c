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

internal CV_SymParsed
cv_sym_from_data(Arena *arena, String8 sym_data, U64 sym_align)
{
  Assert(1 <= sym_align && IsPow2OrZero(sym_align));
  ProfBegin("cv_sym_from_data");
  Temp scratch = scratch_begin(&arena, 1);
  
  // gather up symbols
  CV_RecRangeStream *stream = cv_rec_range_stream_from_data(scratch.arena, sym_data, sym_align);
  
  // convert to result
  CV_SymParsed result = {0};
  result.data       = sym_data;
  result.sym_align  = sym_align;
  result.sym_ranges = cv_rec_range_array_from_stream(arena, stream);
  
  scratch_end(scratch);
  
  ProfEnd();
  return result;
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
  result.file_off   = parsed_lines.file_off;
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
cv_c13_inlinee_lines_from_sub_sections(Arena                *arena,
                                       String8               c13_data,
                                       CV_C13SubSectionList  ss_list)
{
  ProfBeginFunction();

  CV_C13InlineeLinesParsedList inlinee_lines_list = {0};

  for(CV_C13SubSectionNode *ss_node = ss_list.first; ss_node != 0; ss_node = ss_node->next)
  {
    String8 sub_sect_data = str8_substr(c13_data, ss_node->range);
    U64     cursor        = 0;

    CV_C13_InlineeLinesSig sig = 0;
    cursor += str8_deserial_read_struct(sub_sect_data, cursor, &sig);

    for(; cursor + sizeof(CV_C13_InlineeSourceLineHeader) <= sub_sect_data.size; )
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
        if(cursor + sizeof(U32) <= sub_sect_data.size)
        {
          U32 *extra_file_count_ptr = (U32 *)(sub_sect_data.str + cursor);
          cursor += sizeof(*extra_file_count_ptr);

          U32 max_extra_file_count = (sub_sect_data.size - cursor) / sizeof(U32);
          U32 extra_file_count     = Min(*extra_file_count_ptr, max_extra_file_count);
          U32 *extra_files         = (U32 *)(sub_sect_data.str + cursor);
          cursor += sizeof(*extra_files) * extra_file_count;

          inlinee_parsed->extra_file_count = extra_file_count;
          inlinee_parsed->extra_files      = extra_files;
        }
      }
    }
  }

  ProfEnd();
  return inlinee_lines_list;
}

////////////////////////////////
// $$LINES Accel

int
cv_c13_voff_map_compar(const void *a_, const void *b_)
{
  CV_C13Line *a = (CV_C13Line*)a_;
  CV_C13Line *b = (CV_C13Line*)b_;
  int cmp = a->voff < b->voff ? -1 :
            a->voff > b->voff ? +1 :
            0;
  return cmp;
}

internal CV_C13LinesAccel *
cv_c13_make_lines_accel(Arena *arena, U64 lines_count, CV_C13LineArray *lines)
{
  ProfBeginFunction();

  U64 total_voff_count = 0;
  for(U64 arr_idx = 0; arr_idx < lines_count; arr_idx += 1)
  {
    total_voff_count += lines[arr_idx].line_count + 1;
  }

  CV_C13Line *map      = push_array_no_zero(arena, CV_C13Line, total_voff_count);
  U64         map_idx  = 0;

  for(U64 line_idx = 0; line_idx < lines_count; line_idx += 1)
  {
    CV_C13LineArray *l = lines + line_idx;
    if (l->line_count > 0)
    {
      for(U64 voff_idx = 0; voff_idx < l->line_count; voff_idx += 1)
      {
        map[map_idx].voff     = l->voffs[voff_idx];
        map[map_idx].file_off = l->file_off;
        map[map_idx].line_num = l->line_nums[voff_idx];
        map[map_idx].col_num  = 0; // TODO: columns
        map_idx += 1;
      }

      map[map_idx].voff     = l->voffs[l->line_count];
      map[map_idx].file_off = l->file_off;
      map[map_idx].line_num = 0;
      map[map_idx].col_num  = 0;
      map_idx += 1;
    }
  }
  Assert(map_idx == total_voff_count);

  qsort(map, total_voff_count, sizeof(map[0]), cv_c13_voff_map_compar);

  CV_C13LinesAccel *accel = push_array(arena, CV_C13LinesAccel, 1);
  accel->map_count = total_voff_count;
  accel->map       = map;

  ProfEnd();
  return accel;
}

internal CV_C13Line *
cv_c13_line_from_voff(CV_C13LinesAccel *accel, U64 voff, U64 *out_line_count)
{
  ProfBeginFunction();

  U64         voff_line_count = 0;
  CV_C13Line *lines           = 0;

  U64 map_idx = bsearch_nearest_u64(accel->map, accel->map_count, voff, sizeof(accel->map[0]), OffsetOf(CV_C13Line, voff));
  if(map_idx < accel->map_count)
  {
    U64 near_voff = accel->map[map_idx].voff;

    for (; map_idx > 0; map_idx -= 1) {
      if(accel->map[map_idx - 1].voff != near_voff)
      {
        break;
      }
    }

    lines = accel->map + map_idx;

    for(; map_idx < (accel->map_count-1); map_idx += 1)
    {
      if(accel->map[map_idx].voff != near_voff)
      {
        break;
      }
      voff_line_count += 1;
    }
  }

  *out_line_count = voff_line_count;

  ProfEnd();
  return lines;
}

////////////////////////////////
// $$INLINEE_LINES Accel

internal U64
cv_c13_inlinee_lines_accel_hash(void *buffer, U64 size)
{
  U64 result = 5381;
  for(U64 i = 0; i < size; i += 1)
  {
    result = ((result << 5) + result) + ((U8*)buffer)[i];
  }
  return result;
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
cv_c13_make_inlinee_lines_accel(Arena *arena, CV_C13InlineeLinesParsedList inlinee_lines)
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
      //Assert(!"TODO: compiler/linker produced duplicate inlinees in $$INLINEE_LINES");
    }
    else
    {
      cv_c13_inlinee_lines_accel_push(accel, &inlinee->v);
    }
  }

  ProfEnd();
  return accel;
}

////////////////////////////////
// Inline Binary Annots Parser

internal S32
cv_inline_annot_convert_to_signed_operand(U32 value)
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

internal U64
cv_decode_inline_annot_u32(String8 data, U64 offset, U32 *out_value)
{
  U32 value = 0;

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
    Assert(cursor + sizeof(U8) * 1 <= data.size);
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
  else
  {
    InvalidPath;
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


internal CV_InlineBinaryAnnotsParsed
cv_c13_parse_inline_binary_annots(Arena                    *arena,
                                  U64                       parent_voff,
                                  CV_C13InlineeLinesParsed *inlinee_parsed,
                                  String8                   binary_annots)
{
  Temp scratch = scratch_begin(&arena, 1);

  struct CodeRange
  {
    struct CodeRange *next;
    Rng1U64 range;
  };
  struct SourceLine
  {
    struct SourceLine *next;
    U64                voff;
    U64                length;
    U64                ln;
    U64                cn;
    CV_InlineRangeKind kind;
  };
  struct SourceFile
  {
    struct SourceFile *next;
    struct SourceLine *line_first;
    struct SourceLine *line_last;
    U64                line_count;
    U64                checksum_off;
    Rng1U64            last_code_range;
  };

  struct CodeRange *code_range_first = 0;
  struct CodeRange *code_range_last  = 0;
  U64               code_range_count = 0;

  struct SourceFile *file_first = 0;
  struct SourceFile *file_last  = 0;
  U64                file_count = 0;

  CV_InlineRangeKind range_kind             = 0;
  U32                code_length            = 0;
  U32                code_offset            = 0;
  U32                file_off               = inlinee_parsed->file_off;
  S32                ln                     = (S32)inlinee_parsed->first_source_ln;
  S32                cn                     = 1;
  U64                code_offset_lo         = 0;
  B32                code_offset_changed    = 0;
  B32                code_offset_lo_changed = 0;
  B32                code_length_changed    = 0;
  B32                ln_changed             = 1;
  B32                file_off_changed       = 0;

  for(U64 cursor = 0, keep_running = 1; cursor < binary_annots.size && keep_running; )
  {
    U32 op = CV_InlineBinaryAnnotation_Null;
    cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &op);

    switch(op)
    {
    case CV_InlineBinaryAnnotation_Null:
    {
      keep_running = 0;
      
      // this is last run, append range with left over code bytes
      code_length         = code_offset - code_offset_lo;
      code_length_changed = 1;
    }break;
    case CV_InlineBinaryAnnotation_CodeOffset:
    {
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &code_offset);
      code_offset_changed = 1;
    }break;
    case CV_InlineBinaryAnnotation_ChangeCodeOffsetBase:
    {
      AssertAlways(!"TODO: test case");
      // U32 delta = 0;
      // cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &delta);
      // code_offset_base = code_offset;
      // code_offset_end  = code_offset + delta;
      // code_offset += delta;
    }break;
    case CV_InlineBinaryAnnotation_ChangeCodeOffset:
    {
      U32 delta = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &delta);

      code_offset += delta;

      if(!code_offset_lo_changed)
      {
        code_offset_lo = code_offset;
        code_offset_lo_changed = 1;
      }
      code_offset_changed = 1;
    }break;
    case CV_InlineBinaryAnnotation_ChangeCodeLength:
    {
      code_length = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &code_length);
      code_length_changed = 1;
    }break;
    case CV_InlineBinaryAnnotation_ChangeFile:
    {
      U32 old_file_off = file_off;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &file_off);
      file_off_changed = old_file_off != file_off;
      // Compiler isn't obligated to terminate code sequence before chaning files,
      // so we have to always force emit code range on file change.
      code_length_changed = file_off_changed;
    }break;
    case CV_InlineBinaryAnnotation_ChangeLineOffset:
    {
      S32 delta = 0;
      cursor += cv_decode_inline_annot_s32(binary_annots, cursor, &delta);

      ln += delta;
      ln_changed = 1;
    }break;
    case CV_InlineBinaryAnnotation_ChangeLineEndDelta:
    {
      AssertAlways(!"TODO: test case");
      // S32 end_delta = 1;
      // cursor += cv_decode_inline_annot_s32(binary_annots, cursor, &end_delta);
      // ln += end_delta;
    }break;
    case CV_InlineBinaryAnnotation_ChangeRangeKind:
    {
      AssertAlways(!"TODO: test case");
      // cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &range_kind);
    }break;
    case CV_InlineBinaryAnnotation_ChangeColumnStart:
    {
      AssertAlways(!"TODO: test case");
      // S32 delta;
      // cursor += cv_decode_inline_annot_s32(binary_annots, cursor, &delta);
      // cn += delta;
    }break;
    case CV_InlineBinaryAnnotation_ChangeColumnEndDelta:
    {
      AssertAlways(!"TODO: test case");
      // S32 end_delta;
      // cursor += cv_decode_inline_annot_s32(binary_annots, cursor, &end_delta);
      // cn += end_delta;
    }break;
    case CV_InlineBinaryAnnotation_ChangeCodeOffsetAndLineOffset:
    {
      U32 code_offset_and_line_offset = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &code_offset_and_line_offset);

      S32 line_delta = cv_inline_annot_convert_to_signed_operand(code_offset_and_line_offset >> 4);
      U32 code_delta = (code_offset_and_line_offset & 0xf);

      code_offset += code_delta;
      ln          += line_delta;

      if(!code_offset_lo_changed)
      {
        code_offset_lo = code_offset;
        code_offset_lo_changed = 1;
      }

      code_offset_changed = 1;
      ln_changed          = 1;
    }break;
    case CV_InlineBinaryAnnotation_ChangeCodeLengthAndCodeOffset:
    {
      U32 offset_delta = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &code_length);
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &offset_delta); 

      code_offset += offset_delta;

      if(!code_offset_lo_changed)
      {
        code_offset_lo = code_offset;
        code_offset_lo_changed = 1;
      }

      code_offset_changed = 1;
      code_length_changed = 1;
    }break;
    case CV_InlineBinaryAnnotation_ChangeColumnEnd:
    {
      AssertAlways(!"TODO: test case");
      // U32 column_end = 0;
      // cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &column_end);
    }break;
    }

    U64 line_code_offset = code_offset;

    if(code_length_changed)
    {
      // compute upper bound of the range
      U64 code_offset_hi = code_offset + code_length;

      // can last code range be extended to cover current sequence too?
      if(code_range_last != 0 && code_range_last->range.max == parent_voff + code_offset_lo)
      {
        code_range_last->range.max = parent_voff + code_offset_hi;
      }
      else
      {
        // append range
        struct CodeRange *code_range = push_array(scratch.arena, struct CodeRange, 1);
        code_range->range = rng_1u64(parent_voff + code_offset_lo, parent_voff + code_offset_hi);
        SLLQueuePush(code_range_first, code_range_last, code_range);
        ++code_range_count;

        // update last code range in file
        if(file_last)
        {
          file_last->last_code_range = code_range->range;
        }
      }

      // update low offset for next range
      code_offset_lo = code_offset_hi;

      // advance code offset
      code_offset += code_length;

      // reset state
      code_offset_lo_changed = 0;
      code_length_changed    = 0;
      code_length            = 0;
    }

    if(file_off_changed || (file_first == 0))
    {
      // append file
      struct SourceFile *file = push_array(scratch.arena, struct SourceFile, 1);
      file->checksum_off = file_off;
      SLLQueuePush(file_first, file_last, file);
      ++file_count;

      // update last code range in file
      if(code_range_last)
      {
        file->last_code_range = code_range_last->range;
      }

      // reset state
      file_off_changed = 0;
    }

    if(code_offset_changed && ln_changed)
    {
      if(file_last->line_last == 0 || file_last->line_last->ln != (U64)ln)
      {
        // append line
        struct SourceLine *line = push_array(scratch.arena, struct SourceLine, 1);
        line->voff = parent_voff + line_code_offset;
        line->ln   = (U64)ln;
        line->cn   = (U64)cn;
        SLLQueuePush(file_last->line_first, file_last->line_last, line);
        ++file_last->line_count;
      }

      // reset state
      code_offset_changed = 0;
      ln_changed          = 0;
    }
  }

  Rng1U64 *code_ranges;
  {
    code_ranges = push_array_no_zero(arena, Rng1U64, code_range_count);
    U64 code_range_idx = 0;
    for(struct CodeRange *code_range = code_range_first; code_range != 0; code_range = code_range->next, ++code_range_idx)
    {
      code_ranges[code_range_idx] = code_range->range;
    }
  }

  CV_C13LineArray *lines = push_array(arena, CV_C13LineArray, file_count);
  {
    U64 lines_idx = 0;
    for(struct SourceFile *file = file_first; file != 0; file = file->next, lines_idx += 1)
    {
      CV_C13LineArray *l = lines + lines_idx;

      l->file_off   = file->checksum_off;
      l->line_count = file->line_count;
      l->col_count  = 0;

      if(file->line_count > 0)
      {
        l->voffs     = push_array_no_zero(arena, U64, file->line_count + 1);
        l->line_nums = push_array_no_zero(arena, U32, file->line_count);
        l->col_nums  = 0; // TODO: column info 

        U64 line_idx = 0;
        for(struct SourceLine *line = file->line_first; line != NULL; line = line->next, ++line_idx)
        {
          // emit line voff and line number
          l->voffs[line_idx]     = line->voff;
          l->line_nums[line_idx] = (U32)line->ln;
        }
        Assert(line_idx == file->line_count);
        l->voffs[line_idx] = file->last_code_range.max;
      }
    }
  }

  // fill out result
  CV_InlineBinaryAnnotsParsed result;
  result.lines_count      = file_count;
  result.lines            = lines;
  result.code_range_count = code_range_count;
  result.code_ranges      = code_ranges;

  scratch_end(scratch);
  return result;
}

////////////////////////////////
// Enum -> String

internal String8
cv_string_from_inline_range_kind(CV_InlineRangeKind kind)
{
  switch (kind) {
  case CV_InlineRangeKind_Expr: return str8_lit("Expr");
  case CV_InlineRangeKind_Stmt: return str8_lit("Stmt");
  }
  return str8(0,0);
}

