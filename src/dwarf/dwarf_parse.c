// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U64
dw_hash_from_string(String8 string)
{
  XXH64_hash_t hash64 = XXH3_64bits(string.str, string.size);
  return hash64;
}

internal U64
str8_deserial_read_dwarf_packed_size(String8 string, U64 off, U64 *size_out)
{
  U64 bytes_read = 0;
  U32 first_four_bytes;
  if (str8_deserial_read_struct(string, off, &first_four_bytes) == sizeof(first_four_bytes)) {
    if (first_four_bytes == max_U32) {
      if (str8_deserial_read_struct(string, off+sizeof(U32), size_out) == sizeof(U64)) {
        bytes_read = sizeof(U32) + sizeof(U64);
      }
    } else {
      *size_out = first_four_bytes;
      bytes_read = sizeof(U32);
    }
  }
  return bytes_read;
}

internal U64
str8_deserial_read_dwarf_uint(String8 string, U64 off, DW_Format format, U64 *uint_out)
{
  U64 bytes_read = 0;
  switch (format) {
    case DW_Format_Null: break;
    case DW_Format_32Bit: {
      *uint_out &= (U64)max_U32;
      bytes_read = str8_deserial_read(string, off, uint_out, sizeof(U32), sizeof(U32));
    } break;
    case DW_Format_64Bit: {
      bytes_read = str8_deserial_read_struct(string, off, uint_out);
    } break;
  }
  return bytes_read;
}

internal U64
str8_deserial_read_uleb128(String8 string, U64 off, U64 *value_out)
{
  U64 value  = 0;
  U64 shift  = 0;
  U64 cursor = off;
  for(;;)
  {
    U8  byte       = 0;
    U64 bytes_read = str8_deserial_read_struct(string, cursor, &byte);
    
    if(bytes_read != sizeof(byte))
    {
      break;
    }
    
    U8 val = byte & 0x7fu;
    value |= ((U64)val) << shift;
    
    cursor += bytes_read;
    shift += 7u;
    
    if((byte & 0x80u) == 0)
    {
      break;
    }
  }
  if(value_out != 0)
  {
    *value_out = value;
  }
  U64 bytes_read = cursor - off;
  return bytes_read;
}

internal U64
str8_deserial_read_sleb128(String8 string, U64 off, S64 *value_out)
{
  U64 value  = 0;
  U64 shift  = 0;
  U64 cursor = off;
  for(;;)
  {
    U8 byte;
    U64 bytes_read = str8_deserial_read_struct(string, cursor, &byte);
    if(bytes_read != sizeof(byte))
    {
      break;
    }
    
    U8 val = byte & 0x7fu;
    value |= ((U64)val) << shift;
    
    cursor += bytes_read;
    shift += 7u;
    
    if((byte & 0x80u) == 0)
    {
      if(shift < sizeof(value) * 8 && (byte & 0x40u) != 0)
      {
        value |= -(S64)(1ull << shift);
      }
      break;
    }
  }
  if(value_out != 0)
  {
    *value_out = value;
  }
  U64 bytes_read = cursor - off;
  return bytes_read;
}

internal U64
str8_deserial_read_uleb128_array(Arena *arena, String8 string, U64 off, U64 count, U64 **arr_out)
{
  Temp temp = temp_begin(arena);
  
  U64 *arr = push_array(arena, U64, count);
  U64 i, cursor;
  for (i = 0, cursor = off; i < count; ++i) {
    U64 read_size = str8_deserial_read_uleb128(string, cursor, &arr[i]);
    if (read_size == 0) {
      break;
    }
    cursor += read_size;
  }
  
  U64 bytes_read = 0;
  if (i == count) {
    *arr_out = arr;
    bytes_read = cursor - off;
  } else {
    temp_end(temp);
    *arr_out = 0;
  }
  
  return bytes_read;
}

internal U64
str8_deserial_read_sleb128_array(Arena *arena, String8 string, U64 off, U64 count, S64 **arr_out)
{
  Temp temp = temp_begin(arena);
  
  S64 *arr = push_array(arena, S64, count);
  U64 i, cursor;
  for (i = 0, cursor = off; i < count; ++i) {
    U64 read_size = str8_deserial_read_sleb128(string, cursor, &arr[i]);
    if (read_size == 0) {
      break;
    }
    cursor += read_size;
  }
  
  U64 bytes_read = 0;
  if (i == count) {
    *arr_out = arr;
    bytes_read = cursor - off;
  } else {
    temp_end(temp);
    *arr_out = 0;
  }
  
  return bytes_read;
}

internal DW_SectionKind
dw_section_kind_from_string(String8 string)
{
  DW_SectionKind s = DW_Section_Null;
#define X(_K,_L,_M,_W)                                      \
if (str8_match_lit(_L, string, 0)) { s = DW_Section_##_K; } \
if (str8_match_lit(_M, string, 0)) { s = DW_Section_##_K; }
  DW_SectionKind_XList
#undef X
  return s;
}

internal DW_SectionKind
dw_section_dwo_kind_from_string(String8 string)
{
  DW_SectionKind s = DW_Section_Null;
#define X(_K,_L,_M,_W) if (str8_match_lit(_W, string, 0)) { s = DW_Section_##_K; }
  DW_SectionKind_XList
#undef X
  return s;
}

internal Rng1U64List
dw_unit_ranges_from_data(Arena *arena, String8 data)
{
  Rng1U64List result = {0};
  
  for (U64 cursor = 0; cursor < data.size; ) {
    U64 start = cursor;

    // read unit size
    U64 size;
    U64 size_size = str8_deserial_read_dwarf_packed_size(data, cursor, &size);
    if (size_size == 0) { break; }
    cursor += size_size;

    // is unit size valid?
    if (cursor + size > data.size) { break; }
    cursor += size;

    // push unit range
    if (size > 0) {
      rng1u64_list_push(arena, &result, rng_1u64(start, cursor));
    }
  }
  
  return result;
}

internal Rng1U64Array
dw_unit_ranges_from_data_arr(Arena *arena, String8 data)
{
  Temp scratch = scratch_begin(&arena, 1);
  Rng1U64List  list = dw_unit_ranges_from_data(scratch.arena, data);
  Rng1U64Array arr  = rng1u64_array_from_list(arena, &list);
  scratch_end(scratch);
  return arr;
}

internal U64
dw_read_list_unit_header_addr(String8 unit_data, DW_ListUnit *lu_out)
{
  U64 header_size = 0;
  
  U64 unit_length      = 0;
  U64 unit_length_size = str8_deserial_read_dwarf_packed_size(unit_data, 0, &unit_length);
  
  if (unit_length_size) {
    DW_Version version      = DW_Version_Null;
    U64        version_size = str8_deserial_read_struct(unit_data, unit_length_size, &version);
    
    if (version_size) {
      if (version >= DW_Version_5) {
        U8  address_size      = 0;
        U64 address_size_size = str8_deserial_read_struct(unit_data,
                                                          unit_length_size + version_size,
                                                          &address_size);
        
        if (address_size_size && address_size) {
          U8  segment_selector_size      = 0;
          U64 segment_selector_size_size = str8_deserial_read_struct(unit_data,
                                                                     unit_length_size + version_size + address_size_size,
                                                                     &segment_selector_size);
          if (segment_selector_size_size) {
            header_size = unit_length_size + version_size + address_size_size + segment_selector_size_size;
            
            lu_out->version               = version;
            lu_out->segment_selector_size = segment_selector_size;
            lu_out->address_size          = address_size;
            lu_out->entry_size            = segment_selector_size + address_size;
            lu_out->entries               = str8_skip(unit_data, header_size);
          }
        }
      }
    }
  }
  
  return header_size;
}

internal U64
dw_read_list_unit_header_str_offsets(String8 unit_data, DW_ListUnit *lu_out)
{
  U64 header_size = 0;

  U32 first_four_bytes = 0;
  if (str8_deserial_read_struct(unit_data, 0, &first_four_bytes) != sizeof(first_four_bytes)) { goto exit; }
  DW_Format format = first_four_bytes == max_U32 ? DW_Format_64Bit : DW_Format_32Bit;
  
  U64 unit_length      = 0;
  U64 unit_length_size = str8_deserial_read_dwarf_packed_size(unit_data, 0, &unit_length);
  
  if (unit_length_size) {
    DW_Version version      = DW_Version_Null;
    U64        version_size = str8_deserial_read_struct(unit_data, unit_length_size, &version);
    
    if (version >= DW_Version_5) {
      U16 padding = 0;
      U64 padding_size = str8_deserial_read_struct(unit_data, unit_length_size + version_size, &padding);
      
      if (padding_size && padding == 0) {
        header_size = unit_length_size + version_size + padding_size;
        
        lu_out->version               = version;
        lu_out->address_size          = 0;
        lu_out->segment_selector_size = 0;
        lu_out->entry_size            = dw_size_from_format(format);
        lu_out->entries               = str8_skip(unit_data, header_size);
      }
    }
  }
  
  exit:;
  return header_size;
}

internal U64
dw_read_list_unit_header_list(String8 unit_data, DW_ListUnit *lu_out)
{
  U64 header_size = 0;

  U32 first_four_bytes = 0;
  if (str8_deserial_read_struct(unit_data, 0, &first_four_bytes) != sizeof(first_four_bytes)) { goto exit; }
  DW_Format format = first_four_bytes == max_U32 ? DW_Format_64Bit : DW_Format_32Bit;
  
  U64 unit_length      = 0;
  U64 unit_length_size = str8_deserial_read_dwarf_packed_size(unit_data, 0, &unit_length);
  
  if (unit_length_size) {
    DW_Version version      = DW_Version_Null;
    U64        version_size = str8_deserial_read_struct(unit_data, unit_length_size, &version);
    
    if (version >= DW_Version_5) {
      U8  address_size      = 0;
      U64 address_size_size = str8_deserial_read_struct(unit_data, unit_length_size + version_size, &address_size);
      
      if (address_size_size && address_size > 0) {
        U8  segment_selector_size      = 0;
        U64 segment_selector_size_size = str8_deserial_read_struct(unit_data, unit_length_size + version_size + address_size_size, &segment_selector_size);
        
        if (segment_selector_size_size) {
          U32 offset_entry_count = 0;
          U64 offset_entry_count_size = str8_deserial_read_struct(unit_data, unit_length_size + version_size + address_size_size + segment_selector_size, &offset_entry_count);
          
          if (offset_entry_count_size) {
            header_size = unit_length_size + version_size + address_size_size + segment_selector_size_size + offset_entry_count_size;
            
            lu_out->version               = version;
            lu_out->address_size          = address_size;
            lu_out->segment_selector_size = segment_selector_size;
            lu_out->entry_size            = dw_size_from_format(format);
            lu_out->entries               = str8_skip(unit_data, header_size);
          }
        }
      }
    }
  }
  
  exit:;
  return header_size;
}

internal DW_ListUnitInput
dw_list_unit_input_from_input(Arena *arena, DW_Input *input)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  DW_ListUnitInput result = {0};
  
  DW_Section debug_addr = input->sec[DW_Section_Addr];
  {
    String8     data        = debug_addr.data;
    Rng1U64List unit_ranges = dw_unit_ranges_from_data(scratch.arena, data);
    
    result.addr_ranges = rng1u64_array_from_list(arena, &unit_ranges);
    result.addr_count  = unit_ranges.count;
    result.addrs       = push_array(arena, DW_ListUnit, unit_ranges.count);
    
    for (U64 unit_idx = 0; unit_idx < result.addr_ranges.count; ++unit_idx) {
      String8 unit_data = str8_substr(debug_addr.data, result.addr_ranges.v[unit_idx]);
      dw_read_list_unit_header_addr(unit_data, &result.addrs[unit_idx]);
    }
  }
  
  DW_Section debug_str_offsets = input->sec[DW_Section_StrOffsets];
  {
    String8     data        = debug_str_offsets.data;
    Rng1U64List unit_ranges = dw_unit_ranges_from_data(scratch.arena, data);
    
    result.str_offset_ranges = rng1u64_array_from_list(arena, &unit_ranges);
    result.str_offset_count  = unit_ranges.count;
    result.str_offsets       = push_array(arena, DW_ListUnit, unit_ranges.count);
    
    for (U64 unit_idx = 0; unit_idx < result.str_offset_ranges.count; ++unit_idx) {
      String8 unit_data = str8_substr(data, result.str_offset_ranges.v[unit_idx]);
      dw_read_list_unit_header_str_offsets(unit_data, &result.str_offsets[unit_idx]);
    }
  }
  
  DW_Section debug_rnglists = input->sec[DW_Section_RngLists];
  {
    String8     data        = debug_rnglists.data;
    Rng1U64List unit_ranges = dw_unit_ranges_from_data(scratch.arena, data);
    
    result.rnglist_ranges = rng1u64_array_from_list(arena, &unit_ranges);
    result.rnglist_count  = unit_ranges.count;
    result.rnglists       = push_array(arena, DW_ListUnit, unit_ranges.count);
    
    for (U64 unit_idx = 0; unit_idx < result.rnglist_ranges.count; ++unit_idx) {
      String8 unit_data = str8_substr(data, result.rnglist_ranges.v[unit_idx]);
      dw_read_list_unit_header_list(unit_data, &result.rnglists[unit_idx]);
    }
  }
  
  DW_Section debug_loclists = input->sec[DW_Section_LocLists];
  {
    String8     data        = debug_loclists.data;
    Rng1U64List unit_ranges = dw_unit_ranges_from_data(scratch.arena, data);
    
    result.loclist_ranges = rng1u64_array_from_list(arena, &unit_ranges);
    result.loclist_count  = unit_ranges.count;
    result.loclists       = push_array(arena, DW_ListUnit, unit_ranges.count);
    
    for (U64 unit_idx = 0; unit_idx < result.loclist_ranges.count; ++unit_idx) {
      String8 unit_data = str8_substr(data, result.loclist_ranges.v[unit_idx]);
      dw_read_list_unit_header_list(unit_data, &result.loclists[unit_idx]);
    }
  }
  
  scratch_end(scratch);
  return result;
}

internal U64
dw_offset_from_list_unit(DW_ListUnit *lu, U64 index)
{
  U64 offset;
  U64 entry_off = index * lu->entry_size; 
  if (entry_off + lu->entry_size <= lu->entries.size) {
    offset = 0;
    MemoryCopy(&offset, lu->entries.str + entry_off, lu->entry_size);
  } else {
    offset = max_U64;
  }
  return offset;
}

internal U64
dw_addr_from_list_unit(DW_ListUnit *lu, U64 index)
{
  U64 seg  = 0;
  U64 addr = max_U64;
  U64 entry_count = lu->entries.size / lu->entry_size;
  if (index < entry_count) {
    U64 seg_off  = lu->entry_size * index;
    U64 addr_off = seg_off + lu->segment_selector_size;
    MemoryCopy(&seg,  lu->entries.str + seg_off, lu->segment_selector_size);
    MemoryCopy(&addr, lu->entries.str + addr_off, lu->address_size);
    // TODO: segment-based addressing
    AssertAlways(seg == 0);
  } else {
    Assert(!"out of bounds index");
  }
  return addr;
}

internal U64
dw_read_abbrev_tag(String8 data, U64 offset, DW_Abbrev *out_abbrev)
{
  U64 total_bytes_read = 0;
  U64 cursor           = offset;
  
  // read tag ID and kind
  U64 id = 0;
  TryRead(str8_deserial_read_uleb128(data, cursor, &id), cursor, exit);

  // is this null terminator?
  if (id == 0) {
    total_bytes_read = cursor - offset;
    MemoryZeroStruct(out_abbrev);
    goto exit;
  }

  U64 kind = 0;
  TryRead(str8_deserial_read_uleb128(data, cursor, &kind), cursor, exit);

  // read child flag
  U8 has_children = 0;
  if (id != 0) {
    TryRead(str8_deserial_read_struct(data, cursor, &has_children), cursor, exit);
  }
  
  if (out_abbrev != 0) {
    DW_Abbrev abbrev = { .kind = DW_Abbrev_Tag, .sub_kind = kind, .id = id};
    if (has_children) {
      abbrev.flags |= DW_AbbrevFlag_HasChildren;
    }
    *out_abbrev = abbrev;
  }
  
  total_bytes_read = cursor - offset;
exit:;
  return total_bytes_read;
}

internal U64
dw_read_abbrev_attrib(String8 data, U64 offset, DW_Abbrev *out_abbrev)
{
  U64 total_bytes_read = 0;
  U64 cursor           = offset;

  // read id and kind
  U64 id, kind;
  TryRead(str8_deserial_read_uleb128(data, cursor, &id), cursor, exit);
  TryRead(str8_deserial_read_uleb128(data, cursor, &kind), cursor, exit);
  
  // read implicit const
  S64 implicit_const = 0;
  if (kind == DW_Form_ImplicitConst) {
    TryRead(str8_deserial_read_sleb128(data, cursor, &implicit_const), cursor, exit);
  }
  
  if (out_abbrev != 0) {
    DW_Abbrev abbrev = { .kind = DW_Abbrev_Attrib, .sub_kind = kind, .id = id };
    if (kind == DW_Form_ImplicitConst) {
      abbrev.flags         |= DW_AbbrevFlag_HasImplicitConst;
      abbrev.implicit_const = implicit_const;
    }
    *out_abbrev = abbrev;
  }
  
  total_bytes_read = cursor - offset;
exit:;
  return total_bytes_read;
}

internal DW_AbbrevTable
dw_make_abbrev_table(Arena *arena, String8 abbrev_data, U64 abbrev_base)
{
  Temp           temp           = temp_begin(arena);
  DW_AbbrevTable table          = {0};
  B32            failed_to_make = 1;

  // count abbrev tags
  for (U64 cursor = abbrev_base; cursor < abbrev_data.size; table.count += 1) {
    DW_Abbrev tag;
    TryRead(dw_read_abbrev_tag(abbrev_data, cursor, &tag), cursor, exit);
    if (tag.id == 0) { break; }

    for (; cursor < abbrev_data.size; ) {
      DW_Abbrev attrib;
      TryRead(dw_read_abbrev_attrib(abbrev_data, cursor, &attrib), cursor, exit);
      if(attrib.id == 0) { break; }
    }
  }

  // alloc entries
  table.entries = push_array(arena, DW_AbbrevTableEntry, table.count);
  table.count   = 0;

  // fill out the table
  for (U64 cursor = abbrev_base; cursor < abbrev_data.size;) {
    U64 tag_abbrev_off = cursor;
    
    DW_Abbrev tag = {0};
    TryRead(dw_read_abbrev_tag(abbrev_data, cursor, &tag), cursor, exit);
    if (tag.id == 0) { break; }
    
    // push new entry
    table.entries[table.count].id  = tag.id;
    table.entries[table.count].off = tag_abbrev_off;
    table.count += 1;
    
    for (; cursor < abbrev_data.size ;) {
      DW_Abbrev attrib = {0};
      TryRead(dw_read_abbrev_attrib(abbrev_data, cursor, &attrib), cursor, exit);
      if (attrib.id == 0) { break; }
    }
  }
  
  failed_to_make = 0;
exit:;
  if (failed_to_make) { 
    MemoryZeroStruct(&table);
    temp_end(temp);
  }
  return table;
}

internal U64
dw_abbrev_offset_from_abbrev_id(DW_AbbrevTable table, U64 abbrev_id)
{
  U64 abbrev_offset = max_U64;
  if (table.count > 0) {
    for (S64 l = 0, r = (S64)table.count - 1; l <= r; ) {
      S64 m = l + (r - l) / 2;
      if (abbrev_id > table.entries[m].id) {
        l = m + 1;
      } else if (abbrev_id < table.entries[m].id) {
        r = m - 1;
      } else {
        abbrev_offset = table.entries[m].off;
        break;
      }
    }
  }
  return abbrev_offset;
}

internal B32
dw_read_form(String8      data,
             DW_Version   version,
             DW_Format    unit_format,
             U64          address_size,
             DW_FormKind  form_kind,
             S64          implicit_const,
             DW_Form     *form_out,
             U64         *bytes_read_out)
{
  B32     is_ok  = 0;
  U64     cursor = 0;
  DW_Form form   = {0};
  switch (form_kind) {
    case DW_Form_Null: break;
    case DW_Form_Addr: {
      TryRead(str8_deserial_read_block(data, cursor, address_size, &form.addr), cursor, exit);
    } break;
    case DW_Form_Block2: {
      U16 size = 0;
      TryRead(str8_deserial_read_struct(data, cursor, &size), cursor, exit);
      TryRead(str8_deserial_read_block(data, cursor, size, &form.block), cursor, exit);
    } break;
    case DW_Form_Block4: {
      U32 size = 0;
      TryRead(str8_deserial_read_struct(data, cursor, &size), cursor, exit);
      TryRead(str8_deserial_read_block(data, cursor + cursor, size, &form.block), cursor, exit);
    } break;
    case DW_Form_Data2: {
      TryRead(str8_deserial_read_block(data, cursor, sizeof(U16), &form.data), cursor, exit);
    } break;
    case DW_Form_Data4: {
      TryRead(str8_deserial_read_block(data, cursor, sizeof(U32), &form.data), cursor, exit);
    } break;
    case DW_Form_Data8: {
      TryRead(str8_deserial_read_block(data, cursor, sizeof(U64), &form.data), cursor, exit);
    } break;
    case DW_Form_String: {
      TryRead(str8_deserial_read_cstr(data, cursor, &form.string), cursor, exit);
    } break;
    case DW_Form_Block: {
      U64 size = 0;
      TryRead(str8_deserial_read_uleb128(data, cursor, &size), cursor, exit);
      TryRead(str8_deserial_read_block(data, cursor, size, &form.block), cursor, exit);
    } break;
    case DW_Form_Block1: {
      U8 size = 0;
      TryRead(str8_deserial_read_struct(data, cursor, &size), cursor, exit);
      TryRead(str8_deserial_read_block(data, cursor, size, &form.block), cursor, exit);
    } break;
    case DW_Form_Data1: {
      TryRead(str8_deserial_read_block(data, cursor, sizeof(U8), &form.data), cursor, exit);
    } break;
    case DW_Form_Flag: {
      TryRead(str8_deserial_read_struct(data, cursor, &form.flag), cursor, exit);
    } break;
    case DW_Form_SData: {
      TryRead(str8_deserial_read_sleb128(data, cursor, &form.sdata), cursor, exit);
    } break;
    case DW_Form_UData: {
      TryRead(str8_deserial_read_uleb128(data, cursor, &form.udata), cursor, exit);
    } break;
    case DW_Form_RefAddr: {
      if (version < DW_Version_3) {
        TryRead(str8_deserial_read(data, cursor, &form.ref, address_size, address_size), cursor, exit);
      } else {
        TryRead(str8_deserial_read_dwarf_uint(data, cursor, unit_format, &form.ref), cursor, exit);
      }
    } break;
    case DW_Form_GNU_RefAlt: {
      TryRead(str8_deserial_read_dwarf_uint(data, cursor, unit_format, &form.ref), cursor, exit);
    } break;
    case DW_Form_Ref1: {
      TryRead(str8_deserial_read(data, cursor, &form.ref, 1, 1), cursor, exit);
    } break;
    case DW_Form_Ref2: {
      TryRead(str8_deserial_read(data, cursor, &form.ref, 2, 2), cursor, exit);
    } break;
    case DW_Form_Ref4: {
      TryRead(str8_deserial_read(data, cursor, &form.ref, 4, 4), cursor, exit);
    } break;
    case DW_Form_Ref8: {
      TryRead(str8_deserial_read(data, cursor, &form.ref, 8, 8), cursor, exit);
    } break;
    case DW_Form_RefUData: {
      TryRead(str8_deserial_read_uleb128(data, cursor, &form.ref), cursor, exit);
    } break;
    case DW_Form_SecOffset:
    case DW_Form_LineStrp:
    case DW_Form_GNU_StrpAlt:
    case DW_Form_Strp: {
      TryRead(str8_deserial_read_dwarf_uint(data, cursor, unit_format, &form.sec_offset), cursor, exit);
    } break;
    case DW_Form_ExprLoc: {
      U64 expr_size = 0;
      TryRead(str8_deserial_read_uleb128(data, cursor, &expr_size), cursor, exit);
      TryRead(str8_deserial_read_block(data, cursor, expr_size, &form.exprloc), cursor, exit);
    } break;
    case DW_Form_FlagPresent: {
      form.flag = 1;
    } break;
    case DW_Form_RefSig8: {
      //U64 ref = 0;
      //cursor = str8_deserial_read_struct(data, cursor, &ref);
      NotImplemented;
    } break;
    case DW_Form_Addrx:
    case DW_Form_RngListx:
    case DW_Form_Strx: {
      TryRead(str8_deserial_read_uleb128(data, cursor, &form.xval), cursor, exit);
    } break;
    case DW_Form_RefSup4: {
      //U32 ref_sup4 = 0;
      //cursor = str8_deserial_read_struct(data, cursor, &ref_sup4);
      NotImplemented;
    } break;
    case DW_Form_StrpSup: {
      TryRead(str8_deserial_read_dwarf_uint(data, cursor, unit_format, &form.strp_sup), cursor, exit);
    } break;
    case DW_Form_Data16: {
      cursor = str8_deserial_read_block(data, cursor, 16, &form.data);
    } break;
    case DW_Form_ImplicitConst: {
      // Special case.
      // Unlike other forms that have their values stored in the .debug_info section,
      // This one defines it's value in the .debug_abbrev section.
      form.implicit_const = implicit_const;
    } break;
    case DW_Form_LocListx: {
      TryRead(str8_deserial_read_uleb128(data, cursor, &form.xval), cursor, exit);
    } break;
    case DW_Form_RefSup8: {
      NotImplemented;
    } break;
    case DW_Form_Strx1: {
      TryRead(str8_deserial_read(data, cursor, &form.xval, 1, 1), cursor, exit);
    } break;
    case DW_Form_Strx2: {
      TryRead(str8_deserial_read(data, cursor, &form.xval, 2, 2), cursor, exit);
    } break;
    case DW_Form_Strx3: {
      TryRead(str8_deserial_read(data, cursor, &form.xval, 3, 3), cursor, exit);
    } break;
    case DW_Form_Strx4: {
      TryRead(str8_deserial_read(data, cursor, &form.xval, 4, 4), cursor, exit);
    } break;
    case DW_Form_Addrx1: {
      TryRead(str8_deserial_read(data, cursor, &form.xval, 1, 1), cursor, exit);
    } break;
    case DW_Form_Addrx2: {
      TryRead(str8_deserial_read(data, cursor, &form.xval, 2, 2), cursor, exit);
    } break;
    case DW_Form_Addrx3: {
      TryRead(str8_deserial_read(data, cursor, &form.xval, 3, 3), cursor, exit);
    } break;
    case DW_Form_Addrx4: {
      TryRead(str8_deserial_read(data, cursor, &form.xval, 4, 4), cursor, exit);
    } break;
    default: InvalidPath; break;
  }

  if (form_out) {
    form.kind = form_kind;
    *form_out = form;
  }
  if (bytes_read_out) {
    *bytes_read_out = cursor;
  }
  
  is_ok = 1;
exit:;
  Assert(is_ok);
  return is_ok;
}

internal U64
dw_read_tag(Arena          *arena,
            DW_Input       *input,
            DW_AbbrevTable  abbrev_table,
            DW_Version      version,
            DW_Format       format,
            U64             address_size,
            Rng1U64         cu_info_range,
            U64             tag_info_off,
            DW_Tag         *tag_out)
{
  U64 bytes_read = 0;

  String8 info_data   = input->sec[DW_Section_Info].data;
  String8 abbrev_data = input->sec[DW_Section_Abbrev].data;

  String8 tag_data   = str8_substr(info_data, cu_info_range);
  U64     tag_cursor = tag_info_off - cu_info_range.min;
  
  // read tag abbrev id
  U64 tag_abbrev_id = 0;
  TryRead(str8_deserial_read_uleb128(tag_data, tag_cursor, &tag_abbrev_id), tag_cursor, exit);

  B32           has_children = 0;
  DW_TagKind    tag_kind     = DW_TagKind_Null;
  DW_AttribList attribs      = {0};
  if (tag_abbrev_id > 0) {
    // map abbrev id -> .debug_abbrev offset
    U64 abbrev_cursor = dw_abbrev_offset_from_abbrev_id(abbrev_table, tag_abbrev_id);
    if (abbrev_cursor >= abbrev_data.size) { log_user_errorf("failed to find abbrev id 0x%I64x for tag 0x%I64x", tag_abbrev_id, tag_info_off); goto exit; }

    // read tag abbrev
    DW_Abbrev tag_abbrev = {0};
    TryRead(dw_read_abbrev_tag(abbrev_data, abbrev_cursor, &tag_abbrev), abbrev_cursor, exit);

    has_children = !!(tag_abbrev.flags & DW_AbbrevFlag_HasChildren);
    tag_kind     = (DW_TagKind)tag_abbrev.sub_kind;

    // read attribs
    for (; tag_cursor < tag_data.size && abbrev_cursor < abbrev_data.size; ) {
      U64 attrib_tag_cursor = tag_cursor;
      U64 attrib_abbrev_off = abbrev_cursor;

      // read attrib abbrev
      DW_Abbrev attrib_abbrev = {0};
      TryRead(dw_read_abbrev_attrib(abbrev_data, abbrev_cursor, &attrib_abbrev), abbrev_cursor, exit);

      DW_AttribKind attrib_kind = (DW_AttribKind)attrib_abbrev.id;
      DW_FormKind   form_kind   = (DW_FormKind)attrib_abbrev.sub_kind;

      // is this closing attrib?
      if (attrib_kind == DW_AttribKind_Null) { break; }

      // special case, allows producer to embed form in .debug_info
      if (form_kind == DW_Form_Indirect) {
        TryRead(str8_deserial_read_uleb128(tag_data, tag_cursor, &form_kind), tag_cursor, exit);
      }

      // read form value
      DW_Form form      = {0};
      U64     form_size = 0;
      if (!dw_read_form(str8_skip(tag_data, tag_cursor), version, format, address_size, form_kind, attrib_abbrev.implicit_const, &form, &form_size)) {
        goto exit;
      }
      tag_cursor += form_size;

      // fill out node
      DW_AttribNode *attrib_n  = push_array(arena, DW_AttribNode, 1);
      attrib_n->v.info_off     = tag_info_off + attrib_tag_cursor;
      attrib_n->v.abbrev_off   = attrib_abbrev_off;
      attrib_n->v.abbrev_id    = attrib_abbrev.id;
      attrib_n->v.attrib_kind  = attrib_kind;
      attrib_n->v.form         = form;

      // push node to list
      SLLQueuePush(attribs.first, attribs.last, attrib_n);
      attribs.count += 1;
    }
  }

  // fill out tag
  tag_out->abbrev_id    = tag_abbrev_id;
  tag_out->has_children = has_children;
  tag_out->kind         = tag_kind;
  tag_out->attribs      = attribs;
  tag_out->info_off     = tag_info_off;
  
  bytes_read = tag_cursor - (tag_info_off - cu_info_range.min);
exit:;
  return bytes_read;
}

internal U64
dw_read_tag_cu(Arena *arena, DW_Input *input, DW_CompUnit *cu, U64 info_off, DW_Tag *tag_out)
{
  return dw_read_tag(arena,
                     input,
                     cu->abbrev_table,
                     cu->version,
                     cu->format,
                     cu->address_size,
                     cu->info_range,
                     info_off,
                     tag_out);
}

internal B32
dw_try_u64_from_const_value(U64 type_byte_size, DW_ATE type_encoding, String8 const_value, U64 *value_out)
{
  B32 is_parsed = 0;
  if (const_value.size <= type_byte_size) {
    U64 value_size = Min(type_byte_size, const_value.size);
    if (value_size <= sizeof(*value_out)) {
      MemoryZeroStruct(value_out);
      MemoryCopy(value_out, const_value.str, value_size);
      if (type_encoding == DW_ATE_Signed || type_encoding == DW_ATE_SignedChar) {
        *value_out = extend_sign64(*value_out, value_size);
      }
      is_parsed = 1;
    } else {
      Assert(!"out value overflow");
    }
  }
  return is_parsed;
}

internal U64
dw_u64_from_const_value(String8 const_value)
{
  U64 result       = 0;
  B32 is_converted = dw_try_u64_from_const_value(sizeof(U64), DW_ATE_Unsigned, const_value, &result);
  Assert(is_converted); // TODO: error handling
  return result;
}

internal U64
dw_interp_sec_offset(DW_Form form)
{
  U64 sec_offset = 0;
  if (form.kind == DW_Form_SecOffset) {
    sec_offset = form.sec_offset;
  } else if (form.kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return sec_offset;
}

internal String8
dw_interp_exprloc(DW_Form form)
{
  String8 expr = {0};
  if (form.kind == DW_Form_ExprLoc) {
    expr = form.exprloc;
  } else if (form.kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return expr;
}

internal U128
dw_interp_const_u128(DW_Form form)
{
  AssertAlways(form.data.size <= sizeof(U128));
  U128 result = {0};
  MemoryCopy(&result.u64[0], form.data.str, form.data.size);
  return result;
}

internal U64
dw_interp_const64(U64 type_byte_size, DW_ATE type_encoding, DW_Form form)
{
  U64 result = max_U64;
  if (form.kind == DW_Form_Data1 || form.kind == DW_Form_Data2 || form.kind == DW_Form_Data4 || form.kind == DW_Form_Data8 || form.kind == DW_Form_Data16) {
    if (form.data.size <= sizeof(result)) {
      if (!dw_try_u64_from_const_value(type_byte_size, type_encoding, form.data, &result)) {
        Assert(!"unable to decode data");
      }
    } else {
      Assert(!"unable to cast U128 to U64");
    }
  } else if (form.kind == DW_Form_UData) {
    result = form.udata;
  } else if (form.kind == DW_Form_SData) {
    result = form.sdata;
  } else if (form.kind == DW_Form_ImplicitConst) {
    result = form.implicit_const;
  } else if (form.kind == DW_Form_Null) {
    // skip 
  } else {
    AssertAlways(!"unexpected form");
  }
  return result;
}

internal U64
dw_interp_const_u64(DW_Form form)
{
  return dw_interp_const64(sizeof(U64), DW_ATE_Unsigned, form);
}

internal U32
dw_interp_const_u32(DW_Form form)
{
  U64 const64 = dw_interp_const_u64(form);
  U32 const32 = safe_cast_u32(const64);
  return const32;
}

internal S64
dw_interp_const_s64(DW_Form form)
{
  U64 const_u64 = dw_interp_const_u64(form);
  S64 const_s64 = (S64)const_u64;
  return const_s64;
}

internal S32
dw_interp_const_s32(DW_Form form)
{
  U32 const_u32 = dw_interp_const_u32(form);
  S32 const_s32 = (S32)const_u32;
  return const_s32;
}

internal U64
dw_interp_address(U64 address_size, U64 base_addr, DW_ListUnit *addr_lu, DW_Form form)
{
  U64 address = 0;
  if (form.kind == DW_Form_Addr) {
    if (!dw_try_u64_from_const_value(address_size, DW_ATE_Address, form.addr, &address)) {
      AssertAlways(!"unable to decode address");
    }
  } else if (form.kind == DW_Form_Addrx || form.kind == DW_Form_Addrx1 || form.kind == DW_Form_Addrx2 ||
             form.kind == DW_Form_Addrx3 || form.kind == DW_Form_Addrx4) {
    address = dw_addr_from_list_unit(addr_lu, form.xval);
  } else if (form.kind == DW_Form_SecOffset) {
    if (addr_lu->segment_selector_size > 0) {
      AssertAlways(!"TODO: support for segmented address space");
    }
    if (form.sec_offset + addr_lu->segment_selector_size + addr_lu->address_size <= addr_lu->entries.size) {
      MemoryCopy(&address, addr_lu->entries.str + form.sec_offset, addr_lu->address_size);
    } else {
      Assert(!"out of bounds .debug_addr offset");
    }
  } else if (form.kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return address;
}

internal String8
dw_interp_block(DW_Input *input, DW_CompUnit *cu, DW_Form form)
{
  NotImplemented;
  return str8_zero();
}

internal String8
dw_interp_string(DW_Input    *input,
                 DW_Format    unit_format,
                 DW_ListUnit *str_offsets,
                 DW_Form      form)
{
  String8 string = {0};
  if (form.kind == DW_Form_String) {
    string = form.string;
  } else if (form.kind == DW_Form_Strp) {
    U64 bytes_read = str8_deserial_read_cstr(input->sec[DW_Section_Str].data, form.sec_offset, &string);
    Assert(bytes_read > 0);
  } else if (form.kind == DW_Form_LineStrp) {
    U64 bytes_read = str8_deserial_read_cstr(input->sec[DW_Section_LineStr].data, form.sec_offset, &string);
    Assert(bytes_read > 0);
  } else if (form.kind == DW_Form_StrpSup) {
    U64 bytes_read = str8_deserial_read_cstr(input->sec[DW_Section_Str].data, form.strp_sup, &string);
    Assert(bytes_read > 0);
  } else if (form.kind == DW_Form_Strx || form.kind == DW_Form_Strx1 ||
             form.kind == DW_Form_Strx2 || form.kind == DW_Form_Strx3 ||
             form.kind == DW_Form_Strx4) {
    U64 sec_offset = dw_offset_from_list_unit(str_offsets, form.xval);
    if (sec_offset < input->sec[DW_Section_Str].data.size) {
      U64 bytes_read = str8_deserial_read_cstr(input->sec[DW_Section_Str].data, sec_offset, &string);
      Assert(bytes_read > 0);
    } else {
      AssertAlways(!"unable to translate index to offset");
    }
  } else if (form.kind == DW_Form_GNU_StrpAlt) {
    NotImplemented;
  } else if (form.kind == DW_Form_GNU_StrIndex) {
    NotImplemented;
  } else if (form.kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return string;
}

internal String8
dw_interp_line_ptr(DW_Input *input, DW_Form form)
{
  String8 result = {0};
  if (form.kind == DW_Form_SecOffset) {
    result = str8_skip(input->sec[DW_Section_Line].data, form.sec_offset);
  } else if (form.kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return result;
}

internal DW_LineFile *
dw_interp_file(DW_LineVM *line_vm, DW_Form form)
{
  U64          file_idx = dw_interp_const_u64(form);
  DW_LineFile *result   = dw_line_vm_find_file(line_vm, file_idx);
  return result;
}

internal DW_Reference
dw_interp_ref(DW_Input *input, DW_CompUnit *cu, DW_Form form)
{
  DW_Reference ref = {0};
  if (form.kind == DW_Form_Ref1 || form.kind == DW_Form_Ref2 ||
      form.kind == DW_Form_Ref4 || form.kind == DW_Form_Ref8 ||
      form.kind == DW_Form_RefUData) {
    ref.cu = cu;
    ref.info_off = cu->info_range.min + form.ref;
  } else if (form.kind == DW_Form_RefAddr) {
    NotImplemented;
  } else if (form.kind == DW_Form_RefSig8) {
    NotImplemented;
  } else if (form.kind == DW_Form_RefSup4 || form.kind == DW_Form_RefSup8) {
    NotImplemented;
  } else if (form.kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return ref;
}

internal DW_LocList
dw_interp_loclist(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_Form form)
{
  DW_LocList loclist = {0};
  
  if (cu->version < DW_Version_5) {
    if (form.kind == DW_Form_SecOffset) {
      U64 sec_offset = max_U64;
      if (form.kind == DW_Form_SecOffset) {
        sec_offset = form.sec_offset;
      } else if (form.kind == DW_Form_Data8 || form.kind == DW_Form_Data4 ||
                 form.kind == DW_Form_Data2 || form.kind == DW_Form_Data1) {
        if (!dw_try_u64_from_const_value(form.data.size, DW_ATE_Unsigned, form.data, &sec_offset)) {
          Assert(!"unable to extract section offset");
        }
      } else if (form.kind == DW_Form_Null) {
        Assert(!"unexpected form");
      }
      
      String8 sec       = str8_skip(input->sec[DW_Section_Loc].data, sec_offset);
      U64     base_addr = cu->low_pc;
      U64     base_sel  = DW_SentinelFromSize(cu->address_size);
      for (U64 cursor = 0; cursor < sec.size; ) {
        U64 range_min      = 0;
        U64 range_min_off  = cursor;
        U64 range_min_size = str8_deserial_read(sec, range_min_off, &range_min, cu->address_size, cu->address_size);
        if (range_min_size == 0) {
          break;
        }
        U64 range_max      = 0;
        U64 range_max_off  = cursor + cu->address_size;
        U64 range_max_size = str8_deserial_read(sec, range_max_off, &range_max, cu->address_size, cu->address_size);
        if (range_max_size == 0) {
          break;
        }
        cursor += cu->address_size * 2;
        
        // series terminator
        if (range_min == 0 && range_max == 0) {
          break;
        }
        // set new base address
        else if (range_min == base_sel) {
          base_addr = range_max;
        }
        // location
        else {
          U16 expr_size      = 0;
          U64 expr_size_size = str8_deserial_read_struct(sec, cursor, &expr_size);
          if (expr_size_size == 0) {
            Assert(!"unable to read expression size");
            break;
          }
          cursor += expr_size_size;
          
          Assert(cursor + expr_size <= sec.size);
          Rng1U64 expr_range = rng_1u64(cursor, ClampTop(cursor + expr_size, sec.size));
          
          DW_LocNode *loc_n = push_array(arena, DW_LocNode, 1);
          loc_n->v.range    = rng_1u64(base_addr + range_min, base_addr + range_max);
          loc_n->v.expr     = str8_substr(sec, expr_range);
          
          SLLQueuePush(loclist.first, loclist.last, loc_n);
          ++loclist.count;
          
          // advance past expression
          cursor += expr_size;
        }
      }
    } else if (form.kind != DW_Form_Null) {
      AssertAlways(!"unexpected form");
    }
  } else {
    DW_Version version = DW_Version_Null;
    String8    raw_lle = {0};
    if (form.kind == DW_Form_SecOffset) {
      // offset is from beginning of the section
      U64 sec_offset = form.sec_offset;
      raw_lle = str8_skip(input->sec[DW_Section_LocLists].data, sec_offset);
    } else if (form.kind == DW_Form_LocListx) {
      // offset is from beginning of the entries
      U64 entries_off = dw_offset_from_list_unit(cu->loclists_lu, form.xval);
      raw_lle         = str8_skip(cu->loclists_lu->entries, entries_off);
      version         = cu->loclists_lu->version;
    } else if (form.kind != DW_Form_Null) {
      AssertAlways(!"unexpected form");
    }
    
    for (U64 cursor = 0, keep_parsing = 1, base_addr = cu->low_pc;
         cursor < raw_lle.size && keep_parsing; ) {
      DW_LLE kind = DW_LLE_EndOfList;
      cursor += str8_deserial_read_struct(raw_lle, cursor, &kind);
      
      Rng1U64 range = {0};
      switch (kind) {
        default:
        Assert(!"unknown kind");
        case DW_LLE_EndOfList: {
          keep_parsing = 0;
        } break;
        case DW_LLE_BaseAddressx: {
          if (!cu->addr_lu) {
            keep_parsing = 0;
            break;
          }
          
          U64 addrx = 0;
          U64 addrx_size = str8_deserial_read_uleb128(raw_lle, cursor, &addrx);
          if (addrx_size == 0) {
            keep_parsing = 0;
            break;
          }
          
          U64 base_addr_new = dw_addr_from_list_unit(cu->addr_lu, addrx);
          if (base_addr_new == max_U64) {
            InvalidPath;
            break;
          }
          
          base_addr = base_addr_new;
          cursor += addrx_size;
        } break;
        case DW_LLE_StartxEndx: {
          U64 start_addrx      = 0;
          U64 start_addrx_size = str8_deserial_read_uleb128(raw_lle, cursor, &start_addrx);
          if (start_addrx_size == 0) {
            keep_parsing = 0;
            break;
          }
          U64 end_addrx      = 0;
          U64 end_addrx_size = str8_deserial_read_uleb128(raw_lle, cursor + start_addrx_size, &end_addrx);
          if (end_addrx_size == 0) {
            keep_parsing = 0;
            break;
          }
          cursor += start_addrx_size;
          cursor += end_addrx_size;
          
          U64 start = dw_addr_from_list_unit(cu->addr_lu, start_addrx);
          U64 end   = dw_addr_from_list_unit(cu->addr_lu, end_addrx);
          Assert(start != max_U64);
          Assert(end   != max_U64);
          
          range = rng_1u64(start, end);
        } break;
        case DW_LLE_StartxLength: {
          U64 start_addrx      = 0;
          U64 start_addrx_size = str8_deserial_read_uleb128(raw_lle, cursor, &start_addrx);
          if (start_addrx_size == 0) {
            keep_parsing = 0;
            break;
          }
          
          // parse pre-standard & standard length
          U64 length_off = cursor + start_addrx_size;
          U64 length     = 0;
          U64 length_size = str8_deserial_read_uleb128(raw_lle, length_off, &length);
          if (length_size == 0) {
            keep_parsing = 0;
            break;
          }
          
          cursor += start_addrx_size;
          cursor += length_size;
          
          if (cu->addr_lu) {
            U64 start = dw_addr_from_list_unit(cu->addr_lu, start_addrx);
            Assert(start < max_U64);
            
            range = rng_1u64(start, start + length);
          } else {
            Assert(!".debug_addr section is missing -- unable to interpret address index");
          }
        } break;
        case DW_LLE_OffsetPair: {
          U64 start      = 0;
          U64 start_size = str8_deserial_read_uleb128(raw_lle, cursor, &start);
          if (start_size == 0) {
            keep_parsing = 0;
            break;
          }
          U64 end      = 0;
          U64 end_size = str8_deserial_read_uleb128(raw_lle, cursor + start_size, &end);
          if (end_size == 0) {
            keep_parsing = 0;
            break;
          }
          cursor += start_size;
          cursor += end_size;
          
          range = rng_1u64(base_addr + start, base_addr + end);
        } break;
        case DW_LLE_DefaultLocation: {
          // no range
          int x = 0;
        } break;
        case DW_LLE_BaseAddress: {
          U64 base_addr_size = str8_deserial_read(raw_lle, cursor, &base_addr, cu->address_size, cu->address_size);
          if (base_addr_size == 0) {
            keep_parsing = 0;
            break;
          }
          cursor += base_addr_size;
        } break;
        case DW_LLE_StartEnd: {
          U64 start      = 0;
          U64 start_size = str8_deserial_read(raw_lle, cursor, &start, cu->address_size, cu->address_size);
          if (start_size == 0) {
            keep_parsing = 0;
            break;
          }
          
          U64 end      = 0;
          U64 end_size = str8_deserial_read(raw_lle, cursor + start_size, &end, cu->address_size, cu->address_size);
          if (end_size == 0) {
            keep_parsing = 0;
            break;
          }
          cursor += start_size;
          cursor += end_size;
          
          range = rng_1u64(start, end);
        } break;
        case DW_LLE_StartLength: {
          U64 start      = 0;
          U64 start_size = str8_deserial_read(raw_lle, cursor, &start, cu->address_size, cu->address_size);
          if (start_size == 0) {
            keep_parsing = 0;
            break;
          }
          U64 length      = 0;
          U64 length_size = str8_deserial_read_uleb128(raw_lle, cursor + start_size, &length);
          if (length_size == 0) {
            keep_parsing = 0;
            break;
          }
          cursor += start_size;
          cursor += length_size;
          
          range = rng_1u64(start, start + length);
        } break;
      }
      
      B32 has_expr = keep_parsing && kind != DW_LLE_BaseAddressx && kind != DW_LLE_BaseAddress;
      if (has_expr) {
        U64 expr_size      = 0;
        U64 expr_size_size = str8_deserial_read_uleb128(raw_lle, cursor, &expr_size);
        if (expr_size_size == 0) {
          keep_parsing = 0;
          break;
        }
        
        String8 expr           = {0};
        U64     expr_read_size = str8_deserial_read_block(raw_lle, cursor + expr_size_size, expr_size, &expr);
        if (expr_read_size != expr_size) {
          keep_parsing = 0;
          break;
        }
        
        cursor += expr_size_size;
        cursor += expr_size;
        
        DW_LocNode *loc_n = push_array(arena, DW_LocNode, 1);
        loc_n->v.range    = range;
        loc_n->v.expr     = expr;
        
        SLLQueuePush(loclist.first, loclist.last, loc_n);
        ++loclist.count;
      }
    }
  }
  
  return loclist;
}

internal B32
dw_interp_flag(DW_Form form)
{
  B32 flag = 0;
  if (form.kind == DW_Form_Flag || form.kind == DW_Form_FlagPresent) {
    flag = form.flag;
  } else if (form.kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return flag;
}

internal Rng1U64List
dw_interp_rnglist(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_Form form)
{
  Rng1U64List rnglist = {0};
  
  if (cu->version < DW_Version_5) {
    // decode section offset
    U64 sec_offset = max_U64;
    if (form.kind == DW_Form_SecOffset) {
      sec_offset = form.sec_offset;
    } else if (form.kind == DW_Form_Data8 || form.kind == DW_Form_Data4 ||
               form.kind == DW_Form_Data2 || form.kind == DW_Form_Data1) {
      if (!dw_try_u64_from_const_value(form.data.size, DW_ATE_Unsigned, form.data, &sec_offset)) {
        Assert(!"unable to extract section offset");
      }
    } else if (form.kind != DW_Form_Null) {
      Assert(!"unexpected form");
    }
    
    String8 sec       = str8_skip(input->sec[DW_Section_Ranges].data, sec_offset);
    U64     base_addr = cu->low_pc;
    U64     base_sel  = DW_SentinelFromSize(cu->address_size);
    for (U64 cursor = 0; cursor < sec.size; ) {
      U64 range_min      = 0;
      U64 range_min_off  = cursor;
      U64 range_min_size = str8_deserial_read(sec, range_min_off, &range_min, cu->address_size, cu->address_size);
      if (range_min_size == 0) {
        break;
      }
      U64 range_max      = 0;
      U64 range_max_off  = cursor + cu->address_size;
      U64 range_max_size = str8_deserial_read(sec, range_max_off, &range_max, cu->address_size, cu->address_size);
      if (range_max_size == 0) {
        break;
      }
      cursor += cu->address_size * 2;
      
      // series terminator
      if (range_min == 0 && range_max == 0) {
        break;
      }
      // set new base address
      else if (range_min == base_sel) {
        base_addr = range_max;
      }
      // range
      else {
        Rng1U64 range = rng_1u64(base_addr + range_min, base_addr + range_max);
        rng1u64_list_push(arena, &rnglist, range);
      }
    }
  } else {
    String8 raw_rle = {0};
    if (form.kind == DW_Form_SecOffset) {
      // offset is from beginning of the section
      U64 sec_offset = form.sec_offset;
      raw_rle = str8_skip(input->sec[DW_Section_RngLists].data, sec_offset);
    } else if (form.kind == DW_Form_RngListx) {
      // offset is from beginning of the entries
      U64 sec_offset = dw_offset_from_list_unit(cu->rnglists_lu, form.xval);
      raw_rle        = str8_skip(cu->rnglists_lu->entries, sec_offset);
    } else if (form.kind != DW_Form_Null) {
      AssertAlways(!"unexpected form");
    }
    
    U64 rle_invalid_value = DW_SentinelFromSize(cu->address_size);
    U64 base_addr         = cu->low_pc;
    for (U64 cursor = 0, keep_parsing = 1; cursor < raw_rle.size && keep_parsing; ) {
      DW_RLE kind = DW_RLE_EndOfList;
      cursor += str8_deserial_read_struct(raw_rle, cursor, &kind);
      
      Rng1U64 range = rng_1u64(rle_invalid_value, rle_invalid_value);
      switch (kind) {
        default:
        case DW_RLE_EndOfList: {
          keep_parsing = 0;
        } break;
        case DW_RLE_BaseAddressx: {
          U64 addrx      = 0;
          U64 addrx_size = str8_deserial_read_uleb128(raw_rle, cursor, &addrx);
          if (addrx_size == 0) {
            keep_parsing = 0;
            break;
          }
          if (cu->addr_lu == 0) {
            keep_parsing = 0;
            break;
          }
          U64 base_addr_new = dw_addr_from_list_unit(cu->addr_lu, addrx);
          if (base_addr_new < max_U64) {
            base_addr = base_addr_new;
            cursor += addrx_size;
          } else {
            keep_parsing = 0;
            Assert(!"invalid addrx");
          }
        } break;
        case DW_RLE_StartxLength: {
          U64 start_addrx      = 0;
          U64 start_addrx_size = str8_deserial_read_uleb128(raw_rle, cursor, &start_addrx);
          if (start_addrx_size == 0) {
            keep_parsing = 0;
            break;
          }
          U64 length      = 0;
          U64 length_size = str8_deserial_read_uleb128(raw_rle, cursor + start_addrx_size, &length);
          if (length_size == 0) {
            keep_parsing = 0;
            break;
          }
          cursor += start_addrx_size;
          cursor += length_size;
          
          if (cu->addr_lu) {
            U64 start = dw_addr_from_list_unit(cu->addr_lu, start_addrx);
            AssertAlways(start < max_U64);
            range = rng_1u64(start, start + length);
          }
        } break;
        case DW_RLE_OffsetPair: {
          U64 offset_start, offset_end = 0;
          U64 offset_start_size = str8_deserial_read_uleb128(raw_rle, cursor, &offset_start);
          if (offset_start_size == 0) {
            keep_parsing = 0;
            break;
          }
          U64 offset_end_size = str8_deserial_read_uleb128(raw_rle, cursor + offset_start_size, &offset_end);
          if (offset_end_size == 0) {
            keep_parsing = 0;
            break;
          }
          cursor += offset_start_size;
          cursor += offset_end_size;
          
          range = rng_1u64(base_addr + offset_start, base_addr + offset_end);
        } break;
        case DW_RLE_BaseAddress: {
          U64 base_addr_size = str8_deserial_read(raw_rle, cursor, &base_addr, cu->address_size, cu->address_size);
          if (base_addr_size == 0) {
            keep_parsing = 0;
            break;
          }
          cursor += base_addr_size;
        } break;
        case DW_RLE_StartEnd: {
          U64 start = 0, end = 0;
          
          U64 start_size = str8_deserial_read(raw_rle, cursor, &start, cu->address_size, cu->address_size);
          if (start_size == 0) {
            keep_parsing = 0;
            break;
          }
          U64 end_size = str8_deserial_read(raw_rle, cursor + start_size, &end, cu->address_size, cu->address_size);
          if (end_size == 0) {
            keep_parsing = 0;
            break;
          }
          cursor += start_size;
          cursor += end_size;
          
          range = rng_1u64(start, end);
        } break;
        case DW_RLE_StartLength: {
          U64 start = 0, length = 0;
          
          U64 start_size = str8_deserial_read(raw_rle, cursor, &start, cu->address_size, cu->address_size);
          if (start_size == 0) {
            keep_parsing = 0;
            break;
          }
          U64 length_size = str8_deserial_read_uleb128(raw_rle, cursor + start_size, &length);
          if (length_size == 0) {
            keep_parsing = 0;
            break;
          }
          cursor += start_size;
          cursor += length_size;
          
          range = rng_1u64(start, start + length);
        } break;
      }
      
      if (range.min != rle_invalid_value) {
        rng1u64_list_push(arena, &rnglist, range);
      }
    }
  }
  
  return rnglist;
}

internal String8
dw_interp_secptr(DW_Input *input, DW_SectionKind section, DW_Form form)
{
  String8 secptr = {0};
  if (form.kind == DW_Form_SecOffset) {
    String8 sect  = input->sec[section].data;
    Rng1U64 range = rng_1u64(form.sec_offset, sect.size);
    secptr = str8_substr(sect, range);
  } else if (form.kind != DW_Form_Null) {
    Assert(!"unexpected form");
  }
  return secptr;
}

internal String8
dw_interp_addrptr(DW_Input *input, DW_Form form)
{
  return dw_interp_secptr(input, DW_Section_Addr, form);
}

internal String8
dw_interp_str_offsets_ptr(DW_Input *input, DW_Form form)
{
  return dw_interp_secptr(input, DW_Section_StrOffsets, form);
}

internal String8
dw_interp_rnglists_ptr(DW_Input *input, DW_Form form)
{
  return dw_interp_secptr(input, DW_Section_RngLists, form);
}

internal String8
dw_interp_loclists_ptr(DW_Input *input, DW_Form form)
{
  return dw_interp_secptr(input, DW_Section_LocLists, form);
}

internal DW_AttribClass
dw_value_class_from_attrib(DW_CompUnit *cu, DW_Attrib *attrib)
{
  return dw_pick_attrib_value_class(cu->version, cu->ext, cu->relaxed, attrib->attrib_kind, attrib->form.kind);
}

internal String8
dw_exprloc_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_ExprLoc || value_class == DW_AttribClass_Block);
  return dw_interp_exprloc(attrib->form);
}

internal U128
dw_const_u128_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_const_u128(attrib->form);
}

internal U64
dw_const_u64_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_const_u64(attrib->form);
}

internal U32
dw_const_u32_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_const_u32(attrib->form);
}

internal S64
dw_const_s64_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_const_s64(attrib->form);
}

internal S32
dw_const_s32_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_const_s32(attrib->form);
}

internal B32
dw_flag_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Flag);
  return dw_interp_flag(attrib->form);
}

internal U64
dw_address_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null ||
               value_class == DW_AttribClass_Address ||
               value_class == DW_AttribClass_AddrPtr);
  DW_Form form = attrib->form;
  if (value_class == DW_AttribClass_AddrPtr) {
    if (attrib->form.kind == DW_Form_SecOffset) {
      NotImplemented;
    } else {
      AssertAlways(!"unexpected form");
    }
    
    form.kind = DW_Form_Addr;
    form.addr = dw_interp_addrptr(input, attrib->form);
  }
  return dw_interp_address(cu->address_size, cu->low_pc, cu->addr_lu, form);
}

internal String8
dw_block_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Block);
  return dw_interp_block(input, cu, attrib->form);
}

internal String8
dw_string_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_String || value_class == DW_AttribClass_StrOffsetsPtr);
  return dw_interp_string(input, cu->format, cu->str_offsets_lu, attrib->form);
}

internal String8
dw_line_ptr_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_LinePtr);
  return dw_interp_line_ptr(input, attrib->form);
}

internal DW_LineFile *
dw_file_from_attrib(DW_CompUnit *cu, DW_LineVM *line_vm, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_file(line_vm, attrib->form);
}

internal DW_Reference
dw_ref_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Reference);
  return dw_interp_ref(input, cu, attrib->form);
}

internal DW_LocList
dw_loclist_from_attrib(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null ||
               value_class == DW_AttribClass_LocList ||
               value_class == DW_AttribClass_LocListPtr);
  return dw_interp_loclist(arena, input, cu, attrib->form);
}

internal Rng1U64List
dw_rnglist_from_attrib(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  Rng1U64List rnglist = {0};
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  if (value_class == DW_AttribClass_RngListPtr || value_class == DW_AttribClass_RngList) {
    rnglist = dw_interp_rnglist(arena, input, cu, attrib->form);
  } else if (value_class != DW_AttribClass_Null) {
    Assert(!"unexpected value class");
  }
  return rnglist;
}

internal DW_Attrib *
dw_attrib_from_tag_(DW_Tag tag, DW_AttribKind kind)
{
  local_persist read_only DW_Attrib null_attrib;
  DW_Attrib *attrib = &null_attrib;
  for (DW_AttribNode *attrib_n = tag.attribs.first; attrib_n != 0; attrib_n = attrib_n->next) {
    if (attrib_n->v.attrib_kind == kind) {
      attrib = &attrib_n->v;
      break;
    }
  }
  return attrib;
}

internal DW_Attrib *
dw_attrib_from_tag(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  DW_Attrib *attrib = dw_attrib_from_tag_(tag, kind);
  
  if (attrib->attrib_kind == DW_AttribKind_Null) {
    if (cu && cu->tag_ht) {
      DW_Attrib *ao_attrib = dw_attrib_from_tag_(tag, DW_AttribKind_AbstractOrigin);
      if (ao_attrib->attrib_kind == DW_AttribKind_AbstractOrigin) {
        DW_Reference  ref     = dw_interp_ref(input, cu, ao_attrib->form);
        DW_TagNode   *ref_tag = dw_tag_node_from_info_off(ref.cu, ref.info_off);
        attrib = dw_attrib_from_tag_(ref_tag->tag, kind);
      }
    }
  }
  
  return attrib;
}

internal B32
dw_tag_has_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  DW_Attrib *attrib = dw_attrib_from_tag(input, cu, tag, kind);
  B32 has_attrib = attrib->attrib_kind != DW_AttribKind_Null;
  return has_attrib;
}

internal String8
dw_exprloc_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_exprloc_from_attrib(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal String8
dw_block_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_block_from_attrib(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal U128
dw_const_u128_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_const_u128_from_attrib(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal U64
dw_const_u64_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_const_u64_from_attrib(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal U32
dw_const_u32_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_const_u32_from_attrib(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal U64
dw_address_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_address_from_attrib(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal String8
dw_string_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_string_from_attrib(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal String8
dw_line_ptr_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_line_ptr_from_attrib(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal DW_Reference
dw_ref_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_ref_from_attrib(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal DW_LocList
dw_loclist_from_tag_attrib_kind(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_loclist_from_attrib(arena, input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal Rng1U64List
dw_rnglist_from_tag_attrib_kind(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_rnglist_from_attrib(arena, input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal B32
dw_flag_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_flag_from_attrib(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal DW_LineFile *
dw_file_from_tag_attrib_kind(DW_Input *input, DW_CompUnit *cu, DW_LineVM *line_vm, DW_Tag tag, DW_AttribKind kind)
{
  return dw_file_from_attrib(cu, line_vm, dw_attrib_from_tag(input, cu, tag, kind));
}

internal B32
dw_try_byte_size_from_tag(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, U64 *byte_size_out)
{
  B32 has_byte_size = dw_tag_has_attrib(input, cu, tag, DW_AttribKind_ByteSize);
  B32 has_bit_size  = dw_tag_has_attrib(input, cu, tag, DW_AttribKind_BitSize );
  
  if (has_byte_size && has_bit_size) {
    Assert(!"ill formated byte size");
  }
  
  if (has_byte_size) {
    *byte_size_out = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_ByteSize); 
    return 1;
  } else if (has_bit_size) {
    U64 bit_size = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_BitSize);
    *byte_size_out = bit_size / 8;
    return 1;
  }
  
  return 0;
}

internal U64
dw_byte_size_from_tag(DW_Input *input, DW_CompUnit *cu, DW_Tag tag)
{
  U64 byte_size = max_U64;
  dw_try_byte_size_from_tag(input, cu, tag, &byte_size);
  return byte_size;
}

internal U32
dw_byte_size_32_from_tag(DW_Input *input, DW_CompUnit *cu, DW_Tag tag)
{
  U32 byte_size32 = 0;
  U64 byte_size64;
  if (dw_try_byte_size_from_tag(input, cu, tag, &byte_size64)) {
    byte_size32 = safe_cast_u32(byte_size64);
  }
  return byte_size32;
}

internal U64
dw_u64_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  U64             result       = 0;
  DW_Attrib      *attrib       = dw_attrib_from_tag(input, cu, tag, kind);
  DW_AttribClass  attrib_class = dw_value_class_from_attrib(cu, attrib);
  if (attrib_class == DW_AttribClass_Const || attrib_class == DW_AttribClass_Block) {
    if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Type)) {
      Temp scratch = scratch_begin(0,0);
      DW_Reference type_ref = dw_ref_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Type);
      DW_Tag type_tag = {0};
      dw_read_tag_cu(scratch.arena, input, type_ref.cu, type_ref.info_off, &type_tag);
      U64          type_byte_size = dw_byte_size_from_tag(input, cu, type_tag);
      DW_ATE       type_encoding  = dw_const_u64_from_tag_attrib_kind(input, type_ref.cu, type_tag, DW_AttribKind_Encoding);
      if (type_encoding == DW_ATE_Unsigned || type_encoding == DW_ATE_UnsignedChar) {
        result = dw_interp_const64(type_byte_size, type_encoding, attrib->form);
      }
      scratch_end(scratch);
    } else {
      result = dw_interp_const_u64(attrib->form);
    }
  } else if (attrib_class == DW_AttribClass_Address) {
    result = dw_address_from_attrib(input, cu, attrib);
  } else if (attrib_class == DW_AttribClass_Reference) {
    NotImplemented;
  } else if (attrib_class != DW_AttribClass_Null) {
    AssertAlways(!"unexpected attrib class");
  }
  return result;
}

internal B32
dw_is_attrib_var_ref(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  B32 is_var_ref = 0;
  if (dw_is_form_kind_ref(cu->version, cu->ext, attrib->form.kind)) {
    Temp scratch = scratch_begin(0,0);
    DW_Reference ref = dw_ref_from_attrib(input, cu, attrib);
    DW_Tag ref_tag = {0};
    if (dw_read_tag_cu(scratch.arena, input, ref.cu, ref.info_off, &ref_tag)) {
      is_var_ref = ref_tag.kind == DW_TagKind_Variable; 
    }
    scratch_end(scratch);
  }
  return is_var_ref;
}

internal DW_CompUnit
dw_cu_from_info_off(Arena *arena, DW_Input *input, DW_ListUnitInput lu_input, U64 cu_header_offset, B32 relaxed)
{
  DW_CompUnit cu = {0};

  U32 first_four_bytes = 0;
  if (str8_deserial_read_struct(input->sec[DW_Section_Info].data, 0, &first_four_bytes) != sizeof(first_four_bytes)) { goto exit; }
  DW_Format format = first_four_bytes == max_U32 ? DW_Format_64Bit : DW_Format_32Bit;
  
  // read unit size in bytes
  U64 length      = 0;
  U64 length_size = str8_deserial_read_dwarf_packed_size(input->sec[DW_Section_Info].data, cu_header_offset, &length);
  if (length_size == 0) { goto exit; }
  
  // compute unit range
  Rng1U64 info_range  = rng_1u64(cu_header_offset, cu_header_offset + length_size + length);
  String8 info        = str8_substr(input->sec[DW_Section_Info].data, info_range);
  U64     info_cursor = length_size;
  
  // read version
  DW_Version version;
  TryRead(str8_deserial_read_struct(info, info_cursor, &version), info_cursor, exit);
  
  U64             abbrev_base  = max_U64;
  U8              address_size = 0;
  DW_CompUnitKind unit_kind    = DW_CompUnitKind_Reserved;
  U64             spec_dwo_id  = max_U64;
  B32             is_header_ok = 0;
  switch (version) {
    default:
    case DW_Version_Null: {
    } break;
    case DW_Version_1: {
      NotImplemented;
    } break;
    case DW_Version_2: {
      U32 abbrev_base32;
      TryRead(str8_deserial_read_struct(info, info_cursor, &abbrev_base32), info_cursor, exit);
      TryRead(str8_deserial_read_struct(info, info_cursor, &address_size),  info_cursor, exit);
      abbrev_base  = abbrev_base32;
      is_header_ok = 1;
    } break;
    case DW_Version_3:
    case DW_Version_4: {
      TryRead(str8_deserial_read_dwarf_uint(info, info_cursor, format, &abbrev_base), info_cursor, exit);
      TryRead(str8_deserial_read_struct    (info, info_cursor, &address_size),        info_cursor, exit);
      is_header_ok = 1;
    } break;
    case DW_Version_5: {
      TryRead(str8_deserial_read_struct    (info, info_cursor, &unit_kind),           info_cursor, exit);
      TryRead(str8_deserial_read_struct    (info, info_cursor, &address_size),        info_cursor, exit);
      TryRead(str8_deserial_read_dwarf_uint(info, info_cursor, format, &abbrev_base), info_cursor, exit);
      if (unit_kind == DW_CompUnitKind_Skeleton || input->sec[DW_Section_Info].is_dwo) {
        TryRead(str8_deserial_read_struct(info, info_cursor, &spec_dwo_id), info_cursor, exit);
      }
      is_header_ok = 1;
    } break;
  }

  U64 header_size = info_cursor;
  
  if (is_header_ok) {
    Temp temp = temp_begin(arena);
    
    // TODO: cache abbrev tables with identical offsets
    DW_AbbrevTable abbrev_table = dw_make_abbrev_table(arena, input->sec[DW_Section_Abbrev].data, abbrev_base);
    
    DW_Tag cu_tag = {0};
    dw_read_tag(arena, input, abbrev_table, version, format, address_size, info_range, cu_header_offset + header_size, &cu_tag);
    
    // TODO: handle these unit types
    if (cu_tag.kind == DW_TagKind_SkeletonUnit) { log_user_errorf("TODO: Skeleton Unit"); }
    if (cu_tag.kind == DW_TagKind_TypeUnit)     { log_user_errorf("TODO Type Unit");      }

    if (cu_tag.kind != DW_TagKind_CompileUnit && cu_tag.kind != DW_TagKind_PartialUnit) {
      // unexpected tag, release memory and exit
      temp_end(temp);
      goto exit;
    }

    // fetch attribs for list sections
    DW_Attrib *addr_base_attrib        = dw_attrib_from_tag(0, 0, cu_tag, DW_AttribKind_AddrBase      );
    DW_Attrib *str_offsets_base_attrib = dw_attrib_from_tag(0, 0, cu_tag, DW_AttribKind_StrOffsetsBase);
    DW_Attrib *rnglists_base_attrib    = dw_attrib_from_tag(0, 0, cu_tag, DW_AttribKind_RngListsBase  );
    DW_Attrib *loclists_base_attrib    = dw_attrib_from_tag(0, 0, cu_tag, DW_AttribKind_LocListsBase  );
    
    // interp attribs as section offsets
    U64 addr_sec_off        = dw_interp_sec_offset(addr_base_attrib->form       );
    U64 str_offsets_sec_off = dw_interp_sec_offset(str_offsets_base_attrib->form);
    U64 rnglists_sec_off    = dw_interp_sec_offset(rnglists_base_attrib->form   );
    U64 loclists_sec_off    = dw_interp_sec_offset(loclists_base_attrib->form   );
    
    // map section offset to unit index
    U64 addr_lu_idx        = rng_1u64_array_bsearch(lu_input.addr_ranges,       addr_sec_off       );
    U64 str_offsets_lu_idx = rng_1u64_array_bsearch(lu_input.str_offset_ranges, str_offsets_sec_off);
    U64 rnglists_lu_idx    = rng_1u64_array_bsearch(lu_input.rnglist_ranges,    rnglists_sec_off   );
    U64 loclists_lu_idx    = rng_1u64_array_bsearch(lu_input.loclist_ranges,    loclists_sec_off   );
    
    // map index to unit
    DW_ListUnit *addr_lu        = addr_lu_idx        < lu_input.addr_count       ? &lu_input.addrs[addr_lu_idx]              : 0;
    DW_ListUnit *str_offsets_lu = str_offsets_lu_idx < lu_input.str_offset_count ? &lu_input.str_offsets[str_offsets_lu_idx] : 0;
    DW_ListUnit *rnglists_lu    = rnglists_lu_idx    < lu_input.rnglist_count    ? &lu_input.rnglists[rnglists_lu_idx]       : 0;
    DW_ListUnit *loclists_lu    = loclists_lu_idx    < lu_input.loclist_count    ? &lu_input.loclists[loclists_lu_idx]       : 0;
    
    // find compile unit base address
    DW_Attrib *low_pc_attrib = dw_attrib_from_tag(0, 0, cu_tag, DW_AttribKind_LowPc);
    U64        low_pc        = dw_interp_address(address_size, max_U64, addr_lu, low_pc_attrib->form);
    
    // fill out compile unit
    cu.relaxed            = relaxed;
    cu.ext                = DW_Ext_All;
    cu.kind               = unit_kind;
    cu.version            = version;
    cu.format             = format;
    cu.address_size       = address_size;
    cu.abbrev_off         = abbrev_base;
    cu.info_range         = info_range;
    cu.first_tag_info_off = cu_header_offset + header_size;
    cu.abbrev_table       = abbrev_table;
    cu.addr_lu            = addr_lu;
    cu.str_offsets_lu     = str_offsets_lu;
    cu.rnglists_lu        = rnglists_lu;
    cu.loclists_lu        = loclists_lu;
    cu.low_pc             = low_pc;
    cu.tag                = cu_tag;
  }
  
exit:;
  return cu;
}

internal void
dw_tag_tree_from_data(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_TagNode *parent, U64 *cursor, U64 *tag_count)
{
  while (*cursor < cu->info_range.max) {
    // read tag
    DW_Tag tag = {0};
    TryRead(dw_read_tag(arena, input, cu->abbrev_table, cu->version, cu->format, cu->address_size, cu->info_range, *cursor, &tag), *cursor, exit);
    
    // reached of the CU?
    if (tag.kind == DW_TagKind_Null) { break; }
    
    // normal tag
    DW_TagNode *tag_n = push_array(arena, DW_TagNode, 1);
    tag_n->tag        = tag;
    
    SLLQueuePush_N(parent->first_child, parent->last_child, tag_n, sibling);
    
    // update tag count
    *tag_count += 1;
    
    if (tag.has_children) {
      dw_tag_tree_from_data(arena, input, cu, tag_n, cursor, tag_count);
    }
  }
  exit:;
}

internal DW_TagTree
dw_tag_tree_from_cu(Arena *arena, DW_Input *input, DW_CompUnit *cu)
{
  DW_TagNode root      = {0};
  U64        cursor    = cu->first_tag_info_off;
  U64        tag_count = 0;
  dw_tag_tree_from_data(arena, input, cu, &root, &cursor, &tag_count);
  Assert(cursor == cu->info_range.max);
  return (DW_TagTree){ .root = root.first_child, .tag_count = tag_count };
}

internal HashTable *
dw_make_tag_hash_table(Arena *arena, DW_TagTree tag_tree)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  struct Frame {
    struct Frame *next;
    DW_TagNode  *node;
  };
  
  struct Frame *free_frames = 0;
  struct Frame *stack       = push_array(scratch.arena, struct Frame, 1);
  stack->node               = tag_tree.root;
  
  HashTable *ht = hash_table_init(arena, (U64)((F64)tag_tree.tag_count * 1.3));
  
  while (stack) {
    while (stack->node) {
      hash_table_push_u64_raw(arena, ht, stack->node->tag.info_off, stack->node);
      
      if (stack->node->first_child) {
        struct Frame *frame = free_frames;
        if (frame) {
          SLLStackPop(free_frames);
          MemoryZeroStruct(frame);
        } else {
          frame = push_array(scratch.arena, struct Frame, 1);
        }
        frame->node = stack->node->first_child;
        SLLStackPush(stack, frame);
      } else {
        stack->node = stack->node->sibling;
      }
    }
    
    // recycle free frame
    struct Frame *frame = stack;
    SLLStackPop(stack);
    SLLStackPush(free_frames, frame);
    
    if (stack) {
      stack->node = stack->node->sibling;
    }
  }
  
  scratch_end(scratch);
  return ht;
}

internal DW_TagNode *
dw_tag_node_from_info_off(DW_CompUnit *cu, U64 info_off)
{
  DW_TagNode *tag_node = hash_table_search_u64_raw(cu->tag_ht, info_off);
  return tag_node;
}

internal U64
dw_read_line_vm_header(Arena           *arena,
                       DW_Input        *input,
                       String8          cu_stmt_list,
                       String8          cu_dir,
                       String8          cu_name,
                       U8               cu_address_size, 
                       DW_ListUnit     *cu_str_offsets,
                       DW_LineVMHeader *header_out)
{
  Temp scratch = scratch_begin(&arena, 1);

  U64 cursor = 0;

  U32 first_four_bytes = 0;
  if (str8_deserial_read_struct(cu_stmt_list, 0, &first_four_bytes) != sizeof(U32)) { goto exit; }

  // read unit length
  U64 length      = 0;
  U64 length_size = str8_deserial_read_dwarf_packed_size(cu_stmt_list, 0, &length);
  DW_Format format = first_four_bytes == max_U32 ? DW_Format_64Bit : DW_Format_32Bit;
  
  cursor = length_size;
  String8   data   = str8_substr(cu_stmt_list, r1u64(0, length + length_size));
  
  // read unit version
  DW_Version version = DW_Version_Null;
  TryRead(str8_deserial_read_struct(data, cursor, &version), cursor, exit);
  
  // read DWARF5 address & segment selector
  U8 address_size = cu_address_size;
  U8 segsel_size  = 0;
  if (version == DW_Version_5) {
    TryRead(str8_deserial_read_struct(data, cursor, &address_size), cursor, exit);
    TryRead(str8_deserial_read_struct(data, cursor, &segsel_size),  cursor, exit);
  }
  
  U64 header_length = 0;
  TryRead(str8_deserial_read_dwarf_uint(data, cursor, format, &header_length), cursor, exit);
  U64 line_program_off = cursor + header_length;
  
  // substring header so we dont overread into the line program
  data = str8_prefix(data, cursor + header_length);

  U8 min_inst_len = 0;
  TryRead(str8_deserial_read_struct(data, cursor, &min_inst_len), cursor, exit);
  
  U8 max_ops_for_inst = 1;
  if (version > DW_Version_3) {
    TryRead(str8_deserial_read_struct(data, cursor, &max_ops_for_inst), cursor, exit);
    Assert(max_ops_for_inst > 0);
  }
  
  U8 default_is_stmt = 0;
  TryRead(str8_deserial_read_struct(data, cursor, &default_is_stmt), cursor, exit);

  S8 line_base = 0;
  TryRead(str8_deserial_read_struct(data, cursor, &line_base), cursor, exit);

  U8 line_range = 0;
  TryRead(str8_deserial_read_struct(data, cursor, &line_range), cursor, exit);

  U8 opcode_base = 0;
  TryRead(str8_deserial_read_struct(data, cursor, &opcode_base), cursor, exit);

  U64 num_opcode_lens = opcode_base > 0 ? opcode_base - 1 : 0;
  String8 opcode_lens;
  TryRead(str8_deserial_read_block(data, cursor, num_opcode_lens * sizeof(U8), &opcode_lens), cursor, exit);
  
  String8Array     dir_table  = {0};
  DW_LineFileArray file_table = {0};
  if (version < DW_Version_5) {
    // read directory table
    String8List dir_list = {0};

    // compile directory is always first in the table
    str8_list_push(scratch.arena, &dir_list, cu_dir);
    
    // parse additional directories
    while (cursor < data.size) {
      String8 dir = {0};
      TryRead(str8_deserial_read_cstr(data, cursor, &dir), cursor, exit);
      if (dir.size == 0) { break; }
      str8_list_push(scratch.arena, &dir_list, dir);
    }
    
    DW_LineFileList file_list = {0};
    {
      // compile unit name is always first in the file table
      {
        DW_LineFileNode *node = push_array(scratch.arena, DW_LineFileNode, 1);
        node->file.path = cu_name;
        SLLQueuePush(file_list.first, file_list.last, node);
        file_list.node_count += 1;
      }
      
      // read file table
      while (cursor < data.size) {
        U8 first_byte = 0;
        if (str8_deserial_read_struct(data, cursor, &first_byte) == 0) { goto exit; }
        if (first_byte == 0) { break; }

        DW_LineFile file = {0};
        TryRead(str8_deserial_read_cstr   (data, cursor, &file.path),       cursor, exit);
        TryRead(str8_deserial_read_uleb128(data, cursor, &file.dir_idx),    cursor, exit);
        TryRead(str8_deserial_read_uleb128(data, cursor, &file.time_stamp), cursor, exit);
        TryRead(str8_deserial_read_uleb128(data, cursor, &file.size),       cursor, exit);
        
        DW_LineFileNode *node = push_array(scratch.arena, DW_LineFileNode, 1);
        node->file = file;
        
        SLLQueuePush(file_list.first, file_list.last, node);
        file_list.node_count += 1;
      }
    }
    
    // dir list -> array
    dir_table = str8_array_from_list(arena, &dir_list);

    // file list -> array
    file_table.count = 0;
    file_table.v     = push_array(arena, DW_LineFile, file_list.node_count);
    for EachNode(n, DW_LineFileNode, file_list.first) {
      DW_LineFile *dst = &file_table.v[file_table.count++];
      dst->path        = push_str8_copy(arena, n->file.path);
      dst->dir_idx     = n->file.dir_idx;
      dst->time_stamp  = n->file.time_stamp;
      dst->size        = n->file.size;
    }
  }
  // DWARF5
  else {
    // directory table
    {
      // read table entry encoding count
      U8 enc_count = 0;
      TryRead(str8_deserial_read_struct(data, cursor, &enc_count), cursor, exit);
      
      // read table entry encodings
      U64 *enc_arr = 0;
      TryRead(str8_deserial_read_uleb128_array(scratch.arena, data, cursor, enc_count*2, &enc_arr), cursor, exit);
      
      // read table count
      U64 table_count = 0;
      TryRead(str8_deserial_read_uleb128(data, cursor, &table_count), cursor, exit);

      // validate encoding
      if (table_count > 0) {
        if (enc_count != 1)             { goto exit; }
        if (enc_arr[0] != DW_LNCT_Path) { goto exit; }
      }
      
      // read table
      dir_table.count = table_count;
      dir_table.v     = push_array(arena, String8, table_count);
      for EachIndex(i, table_count) {
        DW_FormKind form_kind = enc_arr[1];
        DW_Form     form      = {0};
        U64         form_size = 0;
        if (!dw_read_form(str8_skip(data, cursor), version, format, address_size, form_kind, max_U64, &form, &form_size)) { goto exit; }
        cursor += form_size;
        dir_table.v[i] = dw_interp_string(input, format, cu_str_offsets, form);
      }
    }
    
    // file table
    {
      // read table entry encoding count
      U8 enc_count = 0;
      TryRead(str8_deserial_read_struct(data, cursor, &enc_count), cursor, exit);
      
      // read table entry encodings
      U64 *enc_arr = 0;
      TryRead(str8_deserial_read_uleb128_array(scratch.arena, data, cursor, enc_count*2, &enc_arr), cursor, exit);
      
      // read table count
      U64 table_count = 0;
      TryRead(str8_deserial_read_uleb128(data, cursor, &table_count), cursor, exit);

      file_table.count = table_count;
      file_table.v     = push_array(arena, DW_LineFile, table_count);

      for EachIndex(i, table_count) {
        DW_LineFile *file = &file_table.v[i];
        for EachIndex(enc_idx, enc_count) {
          DW_LNCT lnct = enc_arr[enc_idx*2 + 0];

          DW_FormKind form_kind = enc_arr[enc_idx*2 + 1];
          DW_Form     form      = {0};
          U64         form_size = 0;
          if (!dw_read_form(str8_skip(data, cursor), version, format, address_size, form_kind, max_U64, &form, &form_size)) { goto exit; }
          cursor += form_size;

          switch (lnct) {
          case DW_LNCT_Path:           { file->path       = dw_interp_string(input, format, cu_str_offsets, form); } break;
          case DW_LNCT_DirectoryIndex: { file->dir_idx    = dw_interp_const_u64(form);                             } break;
          case DW_LNCT_TimeStamp:      { file->time_stamp = dw_interp_const_u64(form);                             } break;
          case DW_LNCT_Size:           { file->size       = dw_interp_const_u64(form);                             } break;
          case DW_LNCT_MD5:            { file->md5        = dw_interp_const_u128(form);                            } break;
          case DW_LNCT_LLVM_Source:    { file->source     = dw_interp_string(input, format, cu_str_offsets, form); } break;
          default: {
            Assert(!"unexpected LNTC encoding");
          } break;
          }
        }
      }
    }
  }
  
  if (header_out) {
    header_out->unit_length           = length_size + length;
    header_out->version               = version;
    header_out->address_size          = address_size;
    header_out->segment_selector_size = segsel_size;
    header_out->header_length         = header_length;
    header_out->min_inst_len          = min_inst_len;
    header_out->max_ops_for_inst      = max_ops_for_inst;
    header_out->default_is_stmt       = default_is_stmt;
    header_out->line_base             = line_base;
    header_out->line_range            = line_range;
    header_out->opcode_base           = opcode_base;
    header_out->num_opcode_lens       = num_opcode_lens;
    header_out->opcode_lens           = opcode_lens.str;
    header_out->line_program_off      = line_program_off;
    header_out->dir_table             = dir_table;
    header_out->file_table            = file_table;
  }
  
  exit:;
  scratch_end(scratch);
  return cursor;
}

internal DW_LineVM *
dw_line_vm_init(DW_Input *input, DW_CompUnit *cu)
{
  DW_LineVM *vm = 0;

  Arena *arena = arena_alloc(.reserve_size = KB(64), .commit_size = KB(1));

  String8 cu_stmt_list = dw_line_ptr_from_tag_attrib_kind(input, cu, cu->tag, DW_AttribKind_StmtList);
  String8 cu_dir       = dw_string_from_tag_attrib_kind(input, cu, cu->tag, DW_AttribKind_CompDir);
  String8 cu_name      = dw_string_from_tag_attrib_kind(input, cu, cu->tag, DW_AttribKind_Name);

  // read the line table header
  DW_LineVMHeader header = {0};
  if (cu_stmt_list.size) {
    if ( ! dw_read_line_vm_header(arena, input, cu_stmt_list, cu_dir, cu_name, cu->address_size, cu->str_offsets_lu, &header)) {
      goto exit;
    }
  }

  vm = push_array(arena, DW_LineVM, 1);
  *vm = (DW_LineVM){
    .arena              = arena,
    .cursor             = header.line_program_off,
    .program            = str8_prefix(cu_stmt_list, header.unit_length),
    .header             = header,
    .state.end_sequence = 1,
  };

  exit:;
  if (vm == 0) { arena_release(arena); }
  return vm;
}

internal void
dw_line_vm_release(DW_LineVM *vm)
{
  arena_release(vm->arena);
}

internal void
dw_line_vm_advance(DW_LineVM *vm, U64 advance)
{
  U64 op_index = vm->state.op_index + advance;
  vm->state.address += vm->header.min_inst_len * (op_index / vm->header.max_ops_for_inst);
  vm->state.op_index = op_index % vm->header.max_ops_for_inst;
  vm->advance = advance;
}

internal B32
dw_line_vm_step(DW_LineVM *vm)
{
  B32 is_ok = 0;

  if (vm->cursor >= vm->program.size) { goto exit; }

  vm->new_line = 0;

  // reset VM state
  if (vm->state.end_sequence) {
    vm->state.address         = 0;
    vm->state.op_index        = 0;
    vm->state.file_index      = 1;
    vm->state.line            = 1;
    vm->state.column          = 0;
    vm->state.is_stmt         = vm->header.default_is_stmt;
    vm->state.basic_block     = 0;
    vm->state.end_sequence    = 0;
    vm->state.prologue_end    = 0;
    vm->state.epilogue_begin  = 0;
    vm->state.isa             = 0;
    vm->state.discriminator   = 0;
  }

  // read opcode
  TryRead(str8_deserial_read_struct(vm->program, vm->cursor, &vm->opcode), vm->cursor, exit);

  // special opcodes
  if (vm->opcode >= vm->header.opcode_base) {
    U32 adjusted_opcode = (U32)(vm->opcode - vm->header.opcode_base);
    U32 op_advance      = adjusted_opcode / vm->header.line_range;
    S32 line_advance    = (S64)vm->header.line_base + ((S64)adjusted_opcode) % (S64)vm->header.line_range;
    U64 addr_advance    = vm->header.min_inst_len * ((vm->state.op_index + op_advance) / vm->header.max_ops_for_inst);

    if (vm->header.line_range > 0 && vm->header.max_ops_for_inst > 0) {
      adjusted_opcode = (U32)(vm->opcode - vm->header.opcode_base);
      op_advance      = adjusted_opcode / vm->header.line_range;
      line_advance    = (S32)vm->header.line_base + ((S32)adjusted_opcode) % (S32)vm->header.line_range;
      addr_advance    = vm->header.min_inst_len * ((vm->state.op_index+op_advance) / vm->header.max_ops_for_inst);
    }

    vm->state.address        += addr_advance;
    vm->state.op_index        = (vm->state.op_index + op_advance) % vm->header.max_ops_for_inst;
    vm->state.line            = (U64)((S64)vm->state.line + line_advance);
    vm->state.basic_block     = 0;
    vm->state.prologue_end    = 0;
    vm->state.epilogue_begin  = 0;
    vm->state.discriminator   = 0;

    vm->line_advance = line_advance;
    vm->addr_advance = addr_advance;
    vm->new_line = 1;
  } else {
    // standard opcodes
    switch (vm->opcode) {
    case DW_StdOpcode_Copy: {
      vm->new_line = 1;

      vm->state.discriminator   = 0;
      vm->state.basic_block     = 0;
      vm->state.prologue_end    = 0;
      vm->state.epilogue_begin  = 0;
    } break;

    case DW_StdOpcode_AdvancePc: {
      TryRead(str8_deserial_read_uleb128(vm->program, vm->cursor, &vm->operands[0].u64), vm->cursor, exit);
      dw_line_vm_advance(vm, vm->operands[0].u64);
    } break;

    case DW_StdOpcode_AdvanceLine: {
      TryRead(str8_deserial_read_sleb128(vm->program, vm->cursor, &vm->operands[0].s64), vm->cursor, exit);
      vm->state.line += vm->operands[0].s64;
    } break;

    case DW_StdOpcode_SetFile: {
      TryRead(str8_deserial_read_uleb128(vm->program, vm->cursor, &vm->operands[0].u64), vm->cursor, exit);
      vm->state.file_index = vm->operands[0].u64;
    } break;

    case DW_StdOpcode_SetColumn: {
      TryRead(str8_deserial_read_uleb128(vm->program, vm->cursor, &vm->operands[0].u64), vm->cursor, exit);
      vm->state.column = vm->operands[0].u64;
    } break;

    case DW_StdOpcode_NegateStmt: {
      vm->state.is_stmt = !vm->state.is_stmt;
    } break;

    case DW_StdOpcode_SetBasicBlock: {
      vm->state.basic_block = 1;
    } break;

    case DW_StdOpcode_ConstAddPc: {
      U64 advance = (0xffu - vm->header.opcode_base) / vm->header.line_range;
      dw_line_vm_advance(vm, advance);
    } break;

    case DW_StdOpcode_FixedAdvancePc: {
      U16 fixed_advance = 0;
      TryRead(str8_deserial_read_struct(vm->program, vm->cursor, &fixed_advance), vm->cursor, exit);
      vm->operands[0].u64 = fixed_advance;
      vm->state.address += vm->operands[0].u64;
      vm->state.op_index = 0;
    } break;

    case DW_StdOpcode_SetPrologueEnd: {
      vm->state.prologue_end = 1;
    } break;

    case DW_StdOpcode_SetEpilogueBegin: {
      vm->state.epilogue_begin = 1;
    } break;

    case DW_StdOpcode_SetIsa: {
      TryRead(str8_deserial_read_uleb128(vm->program, vm->cursor, &vm->operands[0].u64), vm->cursor, exit);
      vm->state.isa = vm->operands[0].u64;
    } break;

    // extended opcodes
    case DW_StdOpcode_ExtendedOpcode: {
      TryRead(str8_deserial_read_uleb128(vm->program, vm->cursor, &vm->ext_length), vm->cursor, exit);
      String8 ext_data = str8_substr(vm->program, r1u64(vm->cursor, vm->cursor + vm->ext_length));
      vm->cursor += vm->ext_length;

      U64 ext_cursor = 0;
      TryRead(str8_deserial_read_struct(ext_data, ext_cursor, &vm->ext_opcode), ext_cursor, exit);

      switch (vm->ext_opcode) {
      case DW_ExtOpcode_EndSequence: {
        vm->new_seq  = 1;
        vm->new_line = 1;
        vm->state.end_sequence = 1;
      } break;

      case DW_ExtOpcode_SetAddress: {
        TryRead(str8_deserial_read(ext_data, ext_cursor, &vm->operands[0].u64, vm->header.address_size, vm->header.address_size), ext_cursor, exit);
        vm->state.address  = vm->operands[0].u64;
        vm->state.op_index = 0;
      } break;

      case DW_ExtOpcode_DefineFile: {
        TryRead(str8_deserial_read_cstr   (ext_data, ext_cursor, &vm->operands[0].string), ext_cursor, exit); // file name
        TryRead(str8_deserial_read_uleb128(ext_data, ext_cursor, &vm->operands[1].u64),    ext_cursor, exit); // dir index
        TryRead(str8_deserial_read_uleb128(ext_data, ext_cursor, &vm->operands[2].u64),    ext_cursor, exit); // modify time
        TryRead(str8_deserial_read_uleb128(ext_data, ext_cursor, &vm->operands[3].u64),    ext_cursor, exit); // file size

        if (vm->ext_file_ht == 0) {
          vm->ext_file_ht = hash_table_init(vm->arena, 512);
        }

        DW_LineFile *file = push_array(vm->arena, DW_LineFile, 1);
        file->path       = vm->operands[0].string;
        file->dir_idx    = vm->operands[1].u64;
        file->time_stamp = vm->operands[2].u64;
        file->size       = vm->operands[3].u64;

        if (hash_table_search_u64_raw(vm->ext_file_ht, vm->state.file_index) == 0) {
          vm->error = push_str8f(vm->arena, "file with index %I64d was already defined", vm->state.file_index);
          goto exit;
        }

        hash_table_push_u64_raw(vm->arena, vm->ext_file_ht, vm->state.file_index, file);
      } break;

      case DW_ExtOpcode_SetDiscriminator: {
        TryRead(str8_deserial_read_uleb128(ext_data, ext_cursor, &vm->operands[0].u64), ext_cursor, exit);
        vm->state.discriminator = vm->operands[0].u64;
      } break;

      default: { NotImplemented; } break;
      }
    } break;

    default: {
      // skip unknown opcode
      if (0 == vm->opcode || vm->opcode > vm->header.num_opcode_lens) { goto exit; }
      for EachIndex(i, vm->header.opcode_lens[vm->opcode - 1]) {
        U64 v = 0;
        TryRead(str8_deserial_read_uleb128(vm->program, vm->cursor, &v), vm->cursor, exit);
      }
    } break;
    }
  }

  is_ok = 1;
  exit:;
  return is_ok;
}

internal DW_LineFile *
dw_line_vm_find_file(DW_LineVM *vm, U64 file_idx)
{
  DW_LineFile *file = 0;
  if (file == 0 && file_idx < vm->header.file_table.count) {
    file = &vm->header.file_table.v[file_idx];
  } else if (vm->ext_file_ht) {
    file = hash_table_search_u64_raw(vm->ext_file_ht, file_idx);
  }
  return file;
}

internal String8
dw_path_from_file(Arena *arena, String8Array dir_table, DW_LineFile *file)
{
  Temp scratch = scratch_begin(&arena, 1);

  // directory table must contain at least compile unit directory
  Assert(dir_table.count > 0);

  // find directory and file name associated with the file index
  String8 dir  = dir_table.v[file->dir_idx];
  String8 path = file->path;

  // infer path style if directory is empty use file name
  PathStyle style = path_style_from_str8(dir);
  if (style == PathStyle_Null || style == PathStyle_Relative) {
    style = path_style_from_str8(file->path);
  }
  
  String8List path_list = {0};
  {
    // directories that start with ".." are relative to the compile unit directory
    if (str8_match_lit("..", dir, StringMatchFlag_RightSideSloppy)) {
      String8List comp_dir = str8_split_path(scratch.arena, dir_table.v[0]);
      str8_list_concat_in_place(&path_list, &comp_dir);
    }

    // push directory
    String8List dir_list = str8_split_path(scratch.arena, dir);
    str8_list_concat_in_place(&path_list, &dir_list);

    // push file name
    str8_list_push(scratch.arena, &path_list, file->path);

    // resolve dots in the path
    str8_path_list_resolve_dots_in_place(&path_list, style);
  }

  // join path
  String8 result = str8_path_list_join_by_style(arena, &path_list, style);
  
  scratch_end(scratch);
  return result;
}

internal DW_PubStringsTable
dw_v4_pub_strings_table_from_section_kind(Arena *arena, DW_Input *input, DW_SectionKind section_kind)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  DW_PubStringsTable names_table = {0};
  names_table.size               = 16384;
  names_table.buckets            = push_array(arena, DW_PubStringsBucket*, names_table.size);
  
  String8 section_data = input->sec[section_kind].data;
  for(U64 cursor = 0; cursor < section_data.size; ) {
    U32 first_four_bytes = 0;
    if (str8_deserial_read_struct(input->sec[DW_Section_Info].data, 0, &first_four_bytes) != sizeof(first_four_bytes)) { break; }
    DW_Format format = first_four_bytes == max_U32 ? DW_Format_64Bit : DW_Format_32Bit;

    U64 unit_length      = 0;
    U64 unit_length_size = str8_deserial_read_dwarf_packed_size(section_data, cursor, &unit_length);
    if (unit_length_size == 0) {
      break;
    }
    cursor += unit_length_size;
    
    U64 cursor_opl = Min(cursor + unit_length, section_data.size);
    if (cursor >= cursor_opl) {
      break;
    }
    
    DW_Version unit_version = 0;
    cursor += str8_deserial_read_struct(section_data, cursor, &unit_version);
    if (cursor >= cursor_opl) {
      break;
    }
    
    U64 debug_info_off = 0;
    cursor += str8_deserial_read_dwarf_uint(section_data, cursor, format, &debug_info_off);
    if (cursor >= cursor_opl) {
      break;
    }
    
    U64 debug_info_length = 0;
    cursor += str8_deserial_read_dwarf_packed_size(section_data, cursor, &debug_info_length);
    if (cursor >= cursor_opl) {
      break;
    }
    
    U64 off_size = dw_size_from_format(format);
    for (; (cursor + off_size) <= cursor_opl;) {
      U64 info_off      = 0;
      U64 info_off_size = str8_deserial_read_dwarf_uint(section_data, cursor, format, &info_off);
      cursor += info_off_size;
      
      if (info_off_size == 0 || info_off == 0) {
        break;
      }
      
      String8 string = {0};
      cursor += str8_deserial_read_cstr(section_data, cursor, &string);
      
      U64 hash       = dw_hash_from_string(string);
      U64 bucket_idx = hash % names_table.size;
      
      DW_PubStringsBucket *bucket = push_array(arena, DW_PubStringsBucket, 1);
      bucket->next                = names_table.buckets[bucket_idx];
      bucket->string              = string;
      bucket->info_off            = info_off;
      bucket->cu_info_off         = debug_info_off;
      names_table.buckets[bucket_idx] = bucket;
    }
  }
  
  scratch_end(scratch);
  return names_table;
}

internal DW_Expr
dw_expr_from_data(Arena *arena, DW_Format format, U64 addr_size, String8 data)
{
  DW_Expr expr = {0};
  for (U64 cursor = 0, inst_start = 0; cursor < data.size; inst_start = cursor) {
    // read opcode
    DW_ExprOp opcode = 0;
    U64 opcode_size = str8_deserial_read_struct(data, cursor, &opcode);
    if (opcode_size == 0) { break; }
    cursor += opcode_size;
    
    // read operands
    U64                 operand_count = dw_operand_count_from_expr_op(opcode);
    DW_ExprOperandType *operand_types = dw_operand_types_from_expr_opcode(opcode);
    DW_ExprOperand      operands[2]   = {0};
    for EachIndex(operand_idx, operand_count) {
      U64 operand_size = 0;
      switch (operand_types[operand_idx]) {
      case DW_ExprOperandType_Null:      { } break;
      case DW_ExprOperandType_U8:        { operand_size = str8_deserial_read_struct(data, cursor, &operands[operand_idx].u8);                 } break;
      case DW_ExprOperandType_U16:       { operand_size = str8_deserial_read_struct(data, cursor, &operands[operand_idx].u16);                } break;
      case DW_ExprOperandType_U32:       { operand_size = str8_deserial_read_struct(data, cursor, &operands[operand_idx].u32);                } break;
      case DW_ExprOperandType_U64:       { operand_size = str8_deserial_read_struct(data, cursor, &operands[operand_idx].u64);                } break;
      case DW_ExprOperandType_S8:        { operand_size = str8_deserial_read_struct(data, cursor, &operands[operand_idx].s8);                 } break;
      case DW_ExprOperandType_S16:       { operand_size = str8_deserial_read_struct(data, cursor, &operands[operand_idx].s16);                } break;
      case DW_ExprOperandType_S32:       { operand_size = str8_deserial_read_struct(data, cursor, &operands[operand_idx].s32);                } break;
      case DW_ExprOperandType_S64:       { operand_size = str8_deserial_read_struct(data, cursor, &operands[operand_idx].s64);                } break;
      case DW_ExprOperandType_ULEB128:   { operand_size = str8_deserial_read_uleb128(data, cursor, &operands[operand_idx].u64);               } break;
      case DW_ExprOperandType_SLEB128:   { operand_size = str8_deserial_read_sleb128(data, cursor, &operands[operand_idx].s64);               } break;
      case DW_ExprOperandType_Addr:      { operand_size = str8_deserial_read(data, cursor, &operands[operand_idx].u64, addr_size, addr_size); } break;
      case DW_ExprOperandType_DwarfUInt: { operand_size = str8_deserial_read_dwarf_uint(data, cursor, format, &operands[operand_idx].u64);    } break;
      case DW_ExprOperandType_Block: {
        U8 block_size;
        U64 block_size_size = str8_deserial_read_struct(data, cursor, &block_size);
        U64 block_read_size = str8_deserial_read_block(data, cursor + block_size_size, block_size, &operands[operand_idx].block);
        if(block_read_size == block_size_size) {
          operand_size = block_size_size + block_read_size;
        }
      } break;
      default: { InvalidPath; } break;
      }
      if (operand_size == 0) { goto exit; }
      cursor += operand_size;
    }

    // fill out instruction
    DW_ExprInst *inst = push_array(arena, DW_ExprInst, 1);
    inst->opcode   = opcode;
    inst->size     = cursor - inst_start;
    inst->operands = operand_count ? push_array(arena, DW_ExprOperand, operand_count) : 0;
    MemoryCopy(inst->operands, operands, operand_count * sizeof(DW_ExprOperand));

    // push instruction
    DLLPushBack(expr.first, expr.last, inst);
    expr.count += 1;
  }
exit:;
  return expr;
}

internal void
dw_cfa_inst_list_push_node(DW_CFA_InstList *list, DW_CFA_InstNode *n)
{
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal DW_CFA_InstNode *
dw_cfa_inst_list_push(Arena *arena, DW_CFA_InstList *list, DW_CFA_Inst v)
{
  DW_CFA_InstNode *n = push_array(arena, DW_CFA_InstNode, 1);
  n->v = v;
  dw_cfa_inst_list_push_node(list, n);
  return n;
}

internal U64
dw_read_debug_frame_ptr(String8 data, DW_CIE *cie, U64 *ptr_out)
{
  U64 read_size = 0;
  if (cie->segment_selector_size) {
    NotImplemented;
  } else {
    read_size = str8_deserial_read(data, 0, ptr_out, cie->address_size, cie->address_size);
  }
  return read_size;
}

internal U64
dw_parse_descriptor_entry_header(String8 data, U64 off, DW_DescriptorEntry *desc_out)
{
  U32 first_four_bytes = 0;
  str8_deserial_read_struct(data, off, &first_four_bytes);
  DW_Format format = first_four_bytes == max_U32 ? DW_Format_64Bit : DW_Format_32Bit;

  U64 length      = 0;
  U64 length_size = str8_deserial_read_dwarf_packed_size(data, off, &length);
  if (length_size == 0) { goto exit; }

  Rng1U64 entry_range = rng_1u64(off, off + length_size + length);
  String8 entry_data  = str8_substr(data, entry_range);
  U64 id = 0;
  U64 id_size = str8_deserial_read_dwarf_uint(entry_data, length_size, format, &id);
  if (id_size == 0) { goto exit; }

  U64 id_type = format == DW_Format_32Bit ? max_U32 : max_U64;
  desc_out->format          = format;
  desc_out->type            = (id == id_type) ? DW_DescriptorEntryType_CIE : DW_DescriptorEntryType_FDE;
  desc_out->entry_range     = entry_range;
  desc_out->cie_pointer     = id;
  desc_out->cie_pointer_off = length_size;

exit:;
  return length + length_size;
}

internal B32
dw_parse_cie(String8 data, DW_Format format, Arch arch, DW_CIE *cie_out)
{
  B32 is_parsed = 0;
  U64 cursor    = format == DW_Format_32Bit ? 4 : 12;

  U64 cie_id      = 0;
  TryRead(str8_deserial_read_dwarf_uint(data, cursor, format, &cie_id), cursor, exit);

  U8  version      = 0;
  TryRead(str8_deserial_read_struct(data, cursor, &version), cursor, exit);

  String8 aug_string = {0};
  TryRead(str8_deserial_read_cstr(data, cursor, &aug_string), cursor, exit);

  U8 address_size          = 0;
  U8 segment_selector_size = 0;
  if (version >= DW_Version_4) {
    TryRead(str8_deserial_read_struct(data, cursor, &address_size), cursor, exit);
    TryRead(str8_deserial_read_struct(data, cursor, &segment_selector_size), cursor, exit);
  } else {
    address_size = byte_size_from_arch(arch);
  }

  U64 code_align_factor = 0;
  TryRead(str8_deserial_read_uleb128(data, cursor, &code_align_factor), cursor, exit);

  S64 data_align_factor = 0;
  TryRead(str8_deserial_read_sleb128(data, cursor, &data_align_factor), cursor, exit);

  U64 ret_addr_reg = 0;
  U64 ret_addr_reg_size = 0;
  if (version == DW_Version_1) { TryRead(str8_deserial_read(data, cursor, &ret_addr_reg, sizeof(U8), sizeof(U8)), cursor, exit); }
  else                         { TryRead(str8_deserial_read_uleb128(data, cursor, &ret_addr_reg), cursor, exit);                 }

  if (aug_string.size > 0) { goto exit; }

  cie_out->insts                 = str8_skip(data, cursor);
  cie_out->aug_string            = aug_string;
  cie_out->code_align_factor     = code_align_factor;
  cie_out->data_align_factor     = data_align_factor;
  cie_out->ret_addr_reg          = ret_addr_reg;
  cie_out->format                = format;
  cie_out->version               = version;
  cie_out->address_size          = address_size;
  cie_out->segment_selector_size = segment_selector_size;

  is_parsed = 1;
exit:;
  return is_parsed;
}

internal B32
dw_parse_fde(String8 data, DW_Format format, DW_CIE *cie, DW_FDE *fde_out)
{
  B32 is_parsed = 0;
  U64 cursor    = format == DW_Format_32Bit ? 4 : 12;

  // extract CIE pointer
  U64 cie_pointer = 0;
  TryRead(str8_deserial_read_dwarf_uint(data, cursor, format, &cie_pointer), cursor, exit);

  // extract address of first instruction
  U64 pc_begin = 0;
  TryRead(dw_read_debug_frame_ptr(str8_skip(data, cursor), cie, &pc_begin), cursor, exit);

  // extract instruction range size
  U64 pc_range = 0;
  TryRead(dw_read_debug_frame_ptr(str8_skip(data, cursor), cie, &pc_range), cursor, exit);

  // parse augmentation data
  String8 aug_data = str8_substr(data, rng_1u64(cursor, cursor + cie->aug_data.size));
  cursor += cie->aug_data.size;

  // commit values to out
  fde_out->format      = format;
  fde_out->cie_pointer = cie_pointer;
  fde_out->pc_range    = rng_1u64(pc_begin, pc_begin + pc_range);
  fde_out->insts       = str8_skip(data, cursor);

  is_parsed = 1;
exit:;
  return is_parsed;
}

internal B32
dw_parse_cfi(String8 data, U64 fde_offset, Arch arch, DW_CIE *cie_out, DW_FDE *fde_out)
{
  B32 is_parsed = 0;
  DW_DescriptorEntry fde_desc = {0};
  dw_parse_descriptor_entry_header(data, fde_offset, &fde_desc);
  if (fde_desc.type == DW_DescriptorEntryType_FDE) {
    U64 cie_pointer_off  = fde_desc.format == DW_Format_32Bit ? 4 : 12;
    U64 cie_pointer      = 0;
    U64 cie_pointer_size = str8_deserial_read_dwarf_uint(data, fde_offset + cie_pointer_off, fde_desc.format, &cie_pointer);
    if (cie_pointer_size) {
      DW_DescriptorEntry cie_desc = {0};
      dw_parse_descriptor_entry_header(data, cie_pointer, &cie_desc);
      if (cie_desc.type == DW_DescriptorEntryType_CIE) {
        if (dw_parse_cie(str8_substr(data, cie_desc.entry_range), cie_desc.format, arch, cie_out)) {
          if (dw_parse_fde(str8_substr(data, fde_desc.entry_range), fde_desc.format, cie_out, fde_out)) {
            is_parsed = 1;
          }
        }
      }
    }
  }

  return is_parsed;
}

internal DW_CFA_ParseErrorCode
dw_parse_cfa_inst(String8        data,
                   U64           code_align_factor,
                   S64           data_align_factor,
                   DW_DecodePtr *decode_ptr_func,
                   void         *decode_ptr_ud,
                   U64          *bytes_read_out,
                   DW_CFA_Inst   *inst_out)
{
  *bytes_read_out = 0;

  DW_CFA_ParseErrorCode error_code = DW_CFA_ParseErrorCode_End;
  U64                   cursor     = 0;

  // read opcode
  DW_CFA_Opcode raw_opcode = 0;
  TryRead(str8_deserial_read_struct(data, cursor, &raw_opcode), cursor, exit);

  // decode opcode implicit operand
  U64 opcode           = raw_opcode & ~DW_CFA_Mask_OpcodeHi;
  U64 implicit_operand = 0;
  if ((raw_opcode & DW_CFA_Mask_OpcodeHi) != 0) {
    opcode           = raw_opcode & DW_CFA_Mask_OpcodeHi;
    implicit_operand = raw_opcode & DW_CFA_Mask_Operand;
  }

  // decode operands
  DW_CFA_Operand operands[DW_CFA_OperandMax] = {0};
  switch (opcode) {
  case DW_CFA_SetLoc: {
    U64 address_size = decode_ptr_func(str8_skip(data, cursor), decode_ptr_ud, &operands[0].u64);
    if (address_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += address_size;
  } break;
  case DW_CFA_AdvanceLoc: {
    operands[0].u64 = implicit_operand * code_align_factor;
  } break;
  case DW_CFA_AdvanceLoc1: {
    U8 delta = 0;
    U64 delta_size = str8_deserial_read_struct(data, cursor, &delta);
    if (delta_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += delta_size;
    operands[0].u64 = delta * code_align_factor;
  } break;
  case DW_CFA_AdvanceLoc2: {
    U16 delta = 0;
    U64 delta_size = str8_deserial_read_struct(data, cursor, &delta);
    if (delta_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += delta_size;
    operands[0].u64 = delta * code_align_factor;
  } break;
  case DW_CFA_AdvanceLoc4: {
    U32 delta = 0;
    U64 delta_size = str8_deserial_read_struct(data, cursor, &delta);
    if (delta_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += delta_size;
    operands[0].u64 = delta * code_align_factor;
  } break;
  case DW_CFA_DefCfa: {
    U64 reg = 0;
    U64 reg_size = str8_deserial_read_uleb128(data, cursor, &reg);
    if (reg_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += reg_size;

    U64 offset = 0;
    U64 offset_size = str8_deserial_read_uleb128(data, cursor, &offset);
    if (offset_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += offset_size;

    operands[0].u64 = reg;
    operands[1].u64 = offset;
  } break;
  case DW_CFA_DefCfaSf: {
    U64 reg = 0;
    U64 reg_size = str8_deserial_read_uleb128(data, cursor, &reg);
    if (reg_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += reg_size;

    S64 offset = 0;
    U64 offset_size = str8_deserial_read_sleb128(data, cursor, &offset);
    if (offset_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += offset_size;

    operands[0].u64 = reg;
    operands[1].s64 = offset * data_align_factor;
  } break;
  case DW_CFA_DefCfaRegister: {
    U64 reg = 0;
    U64 reg_size = str8_deserial_read_uleb128(data, cursor, &reg);
    if (reg_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += reg_size;

    operands[0].u64 = reg;
  } break;
  case DW_CFA_DefCfaOffset: {
    U64 offset = 0;
    U64 offset_size = str8_deserial_read_uleb128(data, cursor, &offset);
    if (offset_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += offset_size;

    operands[0].u64 = offset;
  } break;
  case DW_CFA_DefCfaOffsetSf: {
    U64 offset = 0;
    U64 offset_size = str8_deserial_read_uleb128(data, cursor, &offset);
    if (offset_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += offset_size;

    operands[0].u64 = offset * data_align_factor;
  } break;
  case DW_CFA_DefCfaExpr: {
    U64 expr_size = 0;
    U64 expr_size_size = str8_deserial_read_uleb128(data, cursor, &expr_size);
    if (expr_size_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += expr_size_size;

    if (cursor + expr_size > data.size) { goto exit; }
    String8 expr = str8_prefix(str8_skip(data, cursor), expr_size);

    operands[0].block = expr;
    cursor += expr_size;
  } break;
  case DW_CFA_Undefined: {
    U64 reg = 0;
    U64 reg_size = str8_deserial_read_uleb128(data, cursor, &reg);
    if (reg_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += reg_size;

    operands[0].u64 = reg;
  } break;
  case DW_CFA_SameValue: {
    U64 reg = 0;
    U64 reg_size = str8_deserial_read_uleb128(data, cursor, &reg);
    if (reg_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += reg_size;

    operands[0].u64 = reg;
  } break;
  case DW_CFA_Offset: {
    U64 offset = 0;
    U64 offset_size = str8_deserial_read_uleb128(data, cursor, &offset);
    if (offset_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += offset_size;

    operands[0].u64 = implicit_operand;
    operands[1].s64 = (S64)offset * data_align_factor;
  } break;
  case DW_CFA_OffsetExt: {
    U64 reg = 0;
    U64 reg_size = str8_deserial_read_uleb128(data, cursor, &reg);
    if (reg_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += reg_size;

    U64 offset = 0;
    U64 offset_size = str8_deserial_read_uleb128(data, cursor, &offset);
    if (offset_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += offset_size;

    operands[0].u64 = reg;
    operands[1].u64 = offset * data_align_factor;
  } break;
  case DW_CFA_OffsetExtSf: {
    U64 reg = 0;
    U64 reg_size = str8_deserial_read_uleb128(data, cursor, &reg);
    if (reg_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += reg_size;

    S64 offset = 0;
    U64 offset_size = str8_deserial_read_sleb128(data, cursor, &offset);
    if (offset_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += offset_size;

    operands[0].u64 = reg;
    operands[1].s64 = offset * data_align_factor;
  } break;
  case DW_CFA_ValOffset: {
    U64 val = 0;
    U64 val_size = str8_deserial_read_uleb128(data, cursor, &val);
    if (val_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += val_size;

    U64 offset = 0;
    U64 offset_size = str8_deserial_read_uleb128(data, cursor, &offset);
    if (offset_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += offset_size;

    operands[0].u64 = val;
    operands[1].u64 = offset * data_align_factor;
  } break;
  case DW_CFA_ValOffsetSf: {
    U64 val = 0;
    U64 val_size = str8_deserial_read_uleb128(data, cursor, &val);
    if (val_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += val_size;

    S64 offset = 0;
    U64 offset_size = str8_deserial_read_sleb128(data, cursor, &offset);
    if (offset_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += offset_size;

    operands[0].u64 = val;
    operands[1].s64 = offset;
  } break;
  case DW_CFA_Register: {
    U64 dst_reg = 0;
    U64 dst_reg_size = str8_deserial_read_uleb128(data, cursor, &dst_reg);
    if (dst_reg_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += dst_reg_size;

    U64 src_reg = 0;
    U64 src_reg_size = str8_deserial_read_uleb128(data, cursor, &src_reg);
    if (src_reg_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += src_reg_size;

    operands[0].u64 = dst_reg;
    operands[1].u64 = src_reg;
  } break;
  case DW_CFA_Expr: {
    U64 reg = 0;
    U64 reg_size = str8_deserial_read_uleb128(data, cursor, &reg);
    if (reg_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += reg_size;

    U64 expr_size = 0;
    U64 expr_size_size = str8_deserial_read_uleb128(data, cursor, &expr_size);
    if (expr_size_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += expr_size_size;

    if (cursor + expr_size > data.size) { goto exit; }
    String8 expr = str8_prefix(str8_skip(data, cursor), expr_size);
    cursor += expr_size;

    operands[0].u64   = reg;
    operands[1].block = expr;
  } break;
  case DW_CFA_ValExpr: {
    U64 val = 0;
    U64 val_size = str8_deserial_read_uleb128(data, cursor, &val);
    if (val_size == 0) { goto exit; }
    cursor += val_size;

    U64 expr_size = 0;
    U64 expr_size_size = str8_deserial_read_uleb128(data, cursor, &expr_size);
    if (expr_size_size == 0) { error_code = DW_CFA_ParseErrorCode_OutOfData; goto exit; }
    cursor += expr_size_size;

    if (cursor + expr_size > data.size) { goto exit; }
    String8 expr = str8_prefix(str8_skip(data, cursor), expr_size);
    cursor += expr_size;

    operands[0].u64 = val;
    operands[1].block = expr;
  } break;
  case DW_CFA_Restore: {
    operands[0].u64 = implicit_operand;
  } break;
  case DW_CFA_RestoreExt: {} break;
  case DW_CFA_RememberState: {} break;
  case DW_CFA_RestoreState: {} break;
  case DW_CFA_Nop: {} break;
  default: { NotImplemented; goto exit; } break;
  }

  // fill out output
  inst_out->opcode = opcode;
  MemoryCopyTyped(&inst_out->operands[0], &operands[0], DW_CFA_OperandMax);

  *bytes_read_out = cursor;

  error_code = DW_CFA_ParseErrorCode_NewInst;

exit:;
  return error_code;
}

internal DW_CFA_InstList
dw_parse_cfa_inst_list(Arena          *arena,
                        String8        data,
                        U64            code_align_factor,
                        S64            data_align_factor,
                        DW_DecodePtr  *decode_ptr_func,
                        void          *decode_ptr_ud)
{
  U64 pos = arena_pos(arena);
  DW_CFA_InstList list = {0};
  for (U64 cursor = 0, inst_size; cursor < data.size; cursor += inst_size) {
    DW_CFA_Inst           inst       = {0};
    DW_CFA_ParseErrorCode error_code = dw_parse_cfa_inst(str8_skip(data, cursor), code_align_factor, data_align_factor, decode_ptr_func, decode_ptr_ud, &inst_size, &inst);
    if (error_code == DW_CFA_ParseErrorCode_End) { break; }
    if (error_code != DW_CFA_ParseErrorCode_NewInst) {
      MemoryZeroStruct(&list);
      arena_pop_to(arena, pos);
      break;
    }
    dw_cfa_inst_list_push(arena, &list, inst);
  }
  return list;
}

internal
DW_DECODE_PTR(dw_decode_ptr_debug_frame)
{
  return dw_read_debug_frame_ptr(data, ud, ptr_out);
}

