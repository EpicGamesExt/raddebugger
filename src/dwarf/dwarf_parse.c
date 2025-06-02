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
  if (str8_deserial_read(string, off, size_out, sizeof(U32), sizeof(U32))) {
    if (*size_out == max_U32) {
      if (str8_deserial_read_struct(string, off+sizeof(U32), size_out)) {
        bytes_read = sizeof(U32) + sizeof(U64);
      }
    } else {
      *size_out &= (U64)max_U32;
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
#define X(_K,_L,_M,_W)                                        \
if (str8_match_lit(_L, string, 0)) { s = DW_Section_##_K; } \
if (str8_match_lit(_M, string, 0)) { s = DW_Section_##_K; }
  DW_SectionKind_XList(X)
#undef X
  return s;
}

internal DW_SectionKind
dw_section_dwo_kind_from_string(String8 string)
{
  DW_SectionKind s = DW_Section_Null;
#define X(_K,_L,_M,_W)                                        \
if (str8_match_lit(_W, string, 0)) { s = DW_Section_##_K; }
  DW_SectionKind_XList(X)
#undef X
  return s;
}

internal Rng1U64List
dw_unit_ranges_from_data(Arena *arena, String8 data)
{
  Rng1U64List result = {0};
  
  for (U64 cursor = 0; cursor < data.size; ) {
    // read CU size
    U64 cu_size      = 0;
    U64 cu_size_size = str8_deserial_read_dwarf_packed_size(data, cursor, &cu_size);
    
    // was read ok?
    if (cu_size_size == 0) {
      break;
    }
    
    if (cu_size > 0) {
      // push unit range
      rng1u64_list_push(arena, &result, rng_1u64(cursor, cursor+cu_size+cu_size_size));
    }
    
    // advance
    cursor += cu_size_size;
    cursor += cu_size;
  }
  
  return result;
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
        lu_out->entry_size            = dw_size_from_format(DW_FormatFromSize(unit_length));
        lu_out->entries               = str8_skip(unit_data, header_size);
      }
    }
  }
  
  return header_size;
}

internal U64
dw_read_list_unit_header_list(String8 unit_data, DW_ListUnit *lu_out)
{
  U64 header_size = 0;
  
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
            lu_out->entry_size            = dw_size_from_format(DW_FormatFromSize(unit_length));
            lu_out->entries               = str8_skip(unit_data, header_size);
          }
        }
      }
    }
  }
  
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
  
  //- rjf: parse ID
  U64 id_off       = offset;
  U64 sub_kind_off = id_off;
  U64 id           = 0;
  {
    U64 bytes_read = str8_deserial_read_uleb128(data, id_off, &id);
    sub_kind_off += bytes_read;
    total_bytes_read += bytes_read;
  }
  
  //- rjf: parse sub-kind
  U64 sub_kind = 0;
  U64 next_off = sub_kind_off;
  if(id != 0)
  {
    U64 bytes_read = str8_deserial_read_uleb128(data, sub_kind_off, &sub_kind);
    next_off         += bytes_read;
    total_bytes_read += bytes_read;
  }
  
  //- rjf: parse whether this tag has children
  U8 has_children = 0;
  if(id != 0)
  {
    total_bytes_read += str8_deserial_read_struct(data, next_off, &has_children);
  }
  
  //- rjf: fill abbrev
  if(out_abbrev != 0)
  {
    DW_Abbrev abbrev = {0};
    abbrev.kind      = DW_Abbrev_Tag;
    abbrev.sub_kind  = sub_kind;
    abbrev.id        = id;
    if(has_children)
    {
      abbrev.flags |= DW_AbbrevFlag_HasChildren;
    }
    *out_abbrev = abbrev;
  }
  
  return total_bytes_read;
}

internal U64
dw_read_abbrev_attrib(String8 data, U64 offset, DW_Abbrev *out_abbrev)
{
  U64 total_bytes_read = 0;
  
  //- rjf: parse ID
  U64 id_off       = offset;
  U64 sub_kind_off = id_off;
  U64 id           = 0;
  {
    U64 bytes_read = str8_deserial_read_uleb128(data, id_off, &id);
    sub_kind_off     += bytes_read;
    total_bytes_read += bytes_read;
  }
  
  //- rjf: parse sub-kind (form-kind)
  U64 sub_kind = 0;
  U64 next_off = sub_kind_off;
  {
    U64 bytes_read = str8_deserial_read_uleb128(data, sub_kind_off, &sub_kind);
    next_off         += bytes_read;
    total_bytes_read += bytes_read;
  }
  
  //- rjf: parse implicit const
  U64 implicit_const = 0;
  if(sub_kind == DW_Form_ImplicitConst)
  {
    U64 bytes_read = str8_deserial_read_uleb128(data, next_off, &implicit_const);
    total_bytes_read += bytes_read;
  }
  
  //- rjf: fill abbrev
  if(out_abbrev != 0)
  {
    DW_Abbrev abbrev    = {0};
    abbrev.kind         = DW_Abbrev_Attrib;
    abbrev.sub_kind     = sub_kind;
    abbrev.id           = id;
    if(sub_kind == DW_Form_ImplicitConst)
    {
      abbrev.flags       |= DW_AbbrevFlag_HasImplicitConst;
      abbrev.const_value  = implicit_const;
    }
    *out_abbrev = abbrev;
  }
  
  return total_bytes_read;
}

internal DW_AbbrevTable
dw_make_abbrev_table(Arena *arena, String8 abbrev_data, U64 abbrev_offset)
{
  //- rjf: count the tags we have
  U64 tag_count = 0;
  for(U64 abbrev_read_off = abbrev_offset;;)
  {
    DW_Abbrev tag;
    {
      U64 bytes_read = dw_read_abbrev_tag(abbrev_data, abbrev_read_off, &tag);
      abbrev_read_off += bytes_read;
      if(bytes_read == 0 || tag.id == 0)
      {
        break;
      }
    }
    for(;;)
    {
      DW_Abbrev attrib     = {0};
      U64       bytes_read = dw_read_abbrev_attrib(abbrev_data, abbrev_read_off, &attrib);
      abbrev_read_off += bytes_read;
      if(bytes_read == 0 || attrib.id == 0)
      {
        break;
      }
    }
    tag_count += 1;
  }
  
  //- rjf: build table
  DW_AbbrevTable table = {0};
  table.count          = tag_count;
  table.entries        = push_array(arena, DW_AbbrevTableEntry, table.count);
  MemorySet(table.entries, 0, sizeof(DW_AbbrevTableEntry)*table.count);
  
  U64 tag_idx = 0;
  for(U64 abbrev_read_off = abbrev_offset;;)
  {
    U64 tag_abbrev_off = abbrev_read_off;
    
    DW_Abbrev tag;
    {
      U64 bytes_read = dw_read_abbrev_tag(abbrev_data, abbrev_read_off, &tag);
      abbrev_read_off += bytes_read;
      if(bytes_read == 0 || tag.id == 0)
      {
        break;
      }
    }
    
    // rjf: insert this tag into the table
    {
      table.entries[tag_idx].id  = tag.id;
      table.entries[tag_idx].off = tag_abbrev_off;
      tag_idx += 1;
    }
    
    for(;;)
    {
      DW_Abbrev attrib = {0};
      U64 bytes_read = dw_read_abbrev_attrib(abbrev_data, abbrev_read_off, &attrib);
      abbrev_read_off += bytes_read;
      if(bytes_read == 0 || attrib.id == 0)
      {
        break;
      }
    }
    tag_count += 1;
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

internal U64
dw_read_form(String8      data,
             U64          off,
             DW_Version   version,
             DW_Format    unit_format,
             U64          address_size,
             DW_FormKind  form_kind,
             U64          implicit_const,
             DW_Form     *form_out)
{
  U64     bytes_read   = 0;
  DW_Form form = {0};
  
  switch (form_kind) {
    case DW_Form_Null: break;
    
    case DW_Form_Addr: {
      bytes_read = str8_deserial_read_block(data, off, address_size, &form.addr);
    } break;
    case DW_Form_Block2: {
      U16 size = 0;
      U64 size_size = str8_deserial_read_struct(data, off, &size);
      if (size_size) {
        U64 block_size = str8_deserial_read_block(data, off + size_size, size, &form.block);
        if (block_size) {
          bytes_read = size_size + block_size;
        }
      }
    } break;
    case DW_Form_Block4: {
      U32 size = 0;
      U64 size_size = str8_deserial_read_struct(data, off, &size);
      if (size_size) {
        U64 block_size = str8_deserial_read_block(data, off + size_size, size, &form.block);
        if (block_size) {
          bytes_read = size_size + block_size;
        }
      }
    } break;
    case DW_Form_Data2: {
      bytes_read = str8_deserial_read_block(data, off, sizeof(U16), &form.data);
    } break;
    case DW_Form_Data4: {
      bytes_read = str8_deserial_read_block(data, off, sizeof(U32), &form.data);
    } break;
    case DW_Form_Data8: {
      bytes_read = str8_deserial_read_block(data, off, sizeof(U64), &form.data);
    } break;
    case DW_Form_String: {
      bytes_read = str8_deserial_read_cstr(data, off, &form.string);
    } break;
    case DW_Form_Block: {
      U64 size = 0;
      U64 size_size = str8_deserial_read_uleb128(data, off, &size);
      if (size_size) {
        U64 block_size = str8_deserial_read_block(data, off + size_size, size, &form.block);
        if (block_size) {
          bytes_read = size_size + block_size;
        }
      }
    } break;
    case DW_Form_Block1: {
      U8  size      = 0;
      U64 size_size = str8_deserial_read_struct(data, off, &size);
      if (size_size) {
        U64 block_size = str8_deserial_read_block(data, off, size, &form.block);
        if (block_size == size) {
          bytes_read = size_size + block_size;
        }
      }
    } break;
    case DW_Form_Data1: {
      bytes_read = str8_deserial_read_block(data, off, sizeof(U8), &form.data);
    } break;
    case DW_Form_Flag: {
      bytes_read = str8_deserial_read_struct(data, off, &form.flag);
    } break;
    case DW_Form_SData: {
      bytes_read = str8_deserial_read_sleb128(data, off, &form.sdata);
    } break;
    case DW_Form_UData: {
      bytes_read = str8_deserial_read_uleb128(data, off, &form.udata);
    } break;
    case DW_Form_RefAddr: {
      if (version < DW_Version_3) {
        bytes_read = str8_deserial_read(data, off, &form.ref, address_size, address_size);
      } else {
        bytes_read = str8_deserial_read_dwarf_uint(data, off, unit_format, &form.ref);
      }
    } break;
    case DW_Form_GNU_RefAlt: {
      bytes_read = str8_deserial_read_dwarf_uint(data, off, unit_format, &form.ref);
    } break;
    case DW_Form_Ref1: {
      bytes_read = str8_deserial_read(data, off, &form.ref, 1, 1);
    } break;
    case DW_Form_Ref2: {
      bytes_read = str8_deserial_read(data, off, &form.ref, 2, 2);
    } break;
    case DW_Form_Ref4: {
      bytes_read = str8_deserial_read(data, off, &form.ref, 4, 4);
    } break;
    case DW_Form_Ref8: {
      bytes_read = str8_deserial_read(data, off, &form.ref, 8, 8);
    } break;
    case DW_Form_RefUData: {
      bytes_read = str8_deserial_read_uleb128(data, off, &form.ref);
    } break;
    case DW_Form_SecOffset:
    case DW_Form_LineStrp:
    case DW_Form_GNU_StrpAlt:
    case DW_Form_Strp: {
      bytes_read = str8_deserial_read_dwarf_uint(data, off, unit_format, &form.sec_offset);
    } break;
    case DW_Form_ExprLoc: {
      U64 expr_size      = 0;
      U64 expr_size_size = str8_deserial_read_uleb128(data, off, &expr_size);
      if (expr_size_size) {
        if (str8_deserial_read_block(data, off + expr_size_size, expr_size, &form.exprloc)) {
          bytes_read = expr_size_size + expr_size;
        }
      }
    } break;
    case DW_Form_FlagPresent: {
      form.flag = 1;
    } break;
    case DW_Form_RefSig8: {
      //U64 ref = 0;
      //bytes_read = str8_deserial_read_struct(data, off, &ref);
      NotImplemented;
    } break;
    case DW_Form_Addrx:
    case DW_Form_RngListx:
    case DW_Form_Strx: {
      bytes_read = str8_deserial_read_uleb128(data, off, &form.xval);
    } break;
    case DW_Form_RefSup4: {
      //U32 ref_sup4 = 0;
      //bytes_read = str8_deserial_read_struct(data, off, &ref_sup4);
      NotImplemented;
    } break;
    case DW_Form_StrpSup: {
      bytes_read = str8_deserial_read_dwarf_uint(data, off, unit_format, &form.strp_sup);
    } break;
    case DW_Form_Data16: {
      bytes_read = str8_deserial_read_block(data, off, 16, &form.data);
    } break;
    case DW_Form_ImplicitConst: {
      // Special case.
      // Unlike other forms that have their values stored in the .debug_info section,
      // This one defines it's value in the .debug_abbrev section.
      form.implicit_const = implicit_const;
    } break;
    case DW_Form_LocListx: {
      bytes_read = str8_deserial_read_uleb128(data, off, &form.xval);
    } break;
    case DW_Form_RefSup8: {
      NotImplemented;
    } break;
    case DW_Form_Strx1: {
      bytes_read = str8_deserial_read(data, off, &form.xval, 1, 1);
    } break;
    case DW_Form_Strx2: {
      bytes_read = str8_deserial_read(data, off, &form.xval, 2, 2);
    } break;
    case DW_Form_Strx3: {
      bytes_read = str8_deserial_read(data, off, &form.xval, 3, 3);
    } break;
    case DW_Form_Strx4: {
      bytes_read = str8_deserial_read(data, off, &form.xval, 4, 4);
    } break;
    case DW_Form_Addrx1: {
      bytes_read = str8_deserial_read(data, off, &form.xval, 1, 1);
    } break;
    case DW_Form_Addrx2: {
      bytes_read = str8_deserial_read(data, off, &form.xval, 2, 2);
    } break;
    case DW_Form_Addrx3: {
      bytes_read = str8_deserial_read(data, off, &form.xval, 3, 3);
    } break;
    case DW_Form_Addrx4: {
      bytes_read = str8_deserial_read(data, off, &form.xval, 4, 4);
    } break;
    default: InvalidPath; break;
  }
  
  if (form_out) {
    *form_out = form;
  }
  
  return bytes_read;
}

internal U64
dw_read_tag(Arena          *arena,
            String8         tag_data,
            U64             tag_off,
            U64             tag_base,
            DW_AbbrevTable  abbrev_table,
            String8         abbrev_data,
            DW_Version      version,
            DW_Format       unit_format,
            U64             address_size,
            DW_Tag         *tag_out)
{
  U64 tag_cursor = tag_off;
  
  // read tag abbrev id
  U64 tag_abbrev_id    = 0;
  U64 tag_abbrev_id_size = str8_deserial_read_uleb128(tag_data, tag_cursor, &tag_abbrev_id);
  Assert(tag_abbrev_id_size);
  tag_cursor += tag_abbrev_id_size;
  
  // read tag abbrev
  U64       abbrev_cursor   = dw_abbrev_offset_from_abbrev_id(abbrev_table, tag_abbrev_id);
  DW_Abbrev tag_abbrev      = {0};
  U64       tag_abbrev_size = dw_read_abbrev_tag(abbrev_data, abbrev_cursor, &tag_abbrev);
  
  // read attribs
  DW_AttribList attribs = {0};
  if (tag_abbrev_size > 0) {
    abbrev_cursor += tag_abbrev_size;
    
    for (; tag_cursor < tag_data.size && abbrev_cursor < abbrev_data.size; ) {
      U64 attrib_tag_cursor = tag_cursor;
      U64 attrib_abbrev_off = abbrev_cursor;
      
      // read attrib abbrev
      DW_Abbrev attrib_abbrev = {0};
      abbrev_cursor += dw_read_abbrev_attrib(abbrev_data, abbrev_cursor, &attrib_abbrev);
      if (attrib_abbrev.id == 0) {
        break;
      }
      
      DW_AttribKind attrib_kind = (DW_AttribKind)attrib_abbrev.id;
      DW_FormKind   form_kind   = (DW_FormKind)attrib_abbrev.sub_kind;
      
      // special case, allows producer to embed form in .debug_info
      if (form_kind == DW_Form_Indirect) {
        U64 form_kind_size = str8_deserial_read_uleb128(tag_data, tag_cursor, &form_kind);
        
        if (form_kind_size == 0) {
          Assert(!"unable to read indirect form kind");
          break;
        }
        
        tag_cursor += form_kind_size;
      }
      
      // read form value
      DW_Form form = {0};
      tag_cursor += dw_read_form(tag_data, tag_cursor, version, unit_format, address_size, form_kind, attrib_abbrev.const_value, &form);
      
      // fill out node
      DW_AttribNode *attrib_n  = push_array(arena, DW_AttribNode, 1);
      attrib_n->v.info_off     = tag_base + attrib_tag_cursor;
      attrib_n->v.abbrev_off   = attrib_abbrev_off;
      attrib_n->v.abbrev_id    = attrib_abbrev.id;
      attrib_n->v.attrib_kind  = attrib_kind;
      attrib_n->v.form_kind    = form_kind;
      attrib_n->v.form         = form;
      
      // push node to list
      SLLQueuePush(attribs.first, attribs.last, attrib_n);
      ++attribs.count;
    }
  }
  
  // fill out tag
  tag_out->abbrev_id    = tag_abbrev_id;
  tag_out->has_children = !!(tag_abbrev.flags & DW_AbbrevFlag_HasChildren);
  tag_out->kind         = (DW_TagKind)tag_abbrev.sub_kind;
  tag_out->attribs      = attribs;
  tag_out->info_off     = tag_base + tag_off;
  
  U64 bytes_read = tag_cursor - tag_off;
  return bytes_read;
}

internal U64
dw_read_tag_cu(Arena *arena, DW_Input *input, DW_CompUnit *cu, U64 info_off, DW_Tag *tag_out)
{
  String8 tag_data = str8_substr(input->sec[DW_Section_Info].data, cu->info_range);
  U64     tag_off  = info_off - cu->info_range.min;
  return dw_read_tag(arena, tag_data, tag_off, cu->info_range.min, cu->abbrev_table, cu->abbrev_data, cu->version, cu->format, cu->address_size, tag_out);
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
dw_interp_sec_offset(DW_FormKind form_kind, DW_Form form)
{
  U64 sec_offset = 0;
  if (form_kind == DW_Form_SecOffset) {
    sec_offset = form.sec_offset;
  } else if (form_kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return sec_offset;
}

internal String8
dw_interp_exprloc(DW_FormKind form_kind, DW_Form form)
{
  String8 expr = {0};
  if (form_kind == DW_Form_ExprLoc) {
    expr = form.exprloc;
  } else if (form_kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return expr;
}

internal U128
dw_interp_const_u128(DW_FormKind form_kind, DW_Form form)
{
  AssertAlways(form.data.size <= sizeof(U128));
  U128 result = {0};
  MemoryCopy(&result.u64[0], form.data.str, form.data.size);
  return result;
}

internal U64
dw_interp_const64(U64 type_byte_size, DW_ATE type_encoding, DW_FormKind form_kind, DW_Form form)
{
  U64 result = max_U64;
  if (form_kind == DW_Form_Data1 || form_kind == DW_Form_Data2 || form_kind == DW_Form_Data4 || form_kind == DW_Form_Data16) {
    if (form.data.size <= sizeof(result)) {
      if (!dw_try_u64_from_const_value(type_byte_size, type_encoding, form.data, &result)) {
        Assert(!"unable to decode data");
      }
    } else {
      Assert(!"unable to cast U128 to U64");
    }
  } else if (form_kind == DW_Form_UData) {
    result = form.udata;
  } else if (form_kind == DW_Form_SData) {
    result = form.sdata;
  } else if (form_kind == DW_Form_ImplicitConst) {
    result = form.implicit_const;
  } else if (form_kind == DW_Form_Null) {
    // skip 
  } else {
    AssertAlways(!"unexpected form");
  }
  return result;
}

internal U64
dw_interp_const_u64(DW_FormKind form_kind, DW_Form form)
{
  return dw_interp_const64(DW_ATE_Unsigned, sizeof(U64), form_kind, form);
}

internal U32
dw_interp_const_u32(DW_FormKind form_kind, DW_Form form)
{
  U64 const64 = dw_interp_const_u64(form_kind, form);
  U32 const32 = safe_cast_u32(const64);
  return const32;
}

internal S64
dw_interp_const_s64(DW_FormKind form_kind, DW_Form form)
{
  U64 const_u64 = dw_interp_const_u64(form_kind, form);
  S64 const_s64 = (S64)const_u64;
  return const_s64;
}

internal S32
dw_interp_const_s32(DW_FormKind form_kind, DW_Form form)
{
  U32 const_u32 = dw_interp_const_u32(form_kind, form);
  S32 const_s32 = (S32)const_u32;
  return const_s32;
}

internal U64
dw_interp_address(U64 address_size, U64 base_addr, DW_ListUnit *addr_lu, DW_FormKind form_kind, DW_Form form)
{
  U64 address = 0;
  if (form_kind == DW_Form_Addr) {
    if (!dw_try_u64_from_const_value(address_size, DW_ATE_Address, form.addr, &address)) {
      AssertAlways(!"unable to decode address");
    }
  } else if (form_kind == DW_Form_Addrx || form_kind == DW_Form_Addrx1 || form_kind == DW_Form_Addrx2 ||
             form_kind == DW_Form_Addrx3 || form_kind == DW_Form_Addrx4) {
    address = dw_addr_from_list_unit(addr_lu, form.xval);
  } else if (form_kind == DW_Form_SecOffset) {
    if (addr_lu->segment_selector_size > 0) {
      AssertAlways(!"TODO: support for segmented address space");
    }
    if (form.sec_offset + addr_lu->segment_selector_size + addr_lu->address_size <= addr_lu->entries.size) {
      MemoryCopy(&address, addr_lu->entries.str + form.sec_offset, addr_lu->address_size);
    } else {
      Assert(!"out of bounds .debug_addr offset");
    }
  } else if (form_kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return address;
}

internal String8
dw_interp_block(DW_Input *input, DW_CompUnit *cu, DW_FormKind form_kind, DW_Form form)
{
  NotImplemented;
  return str8_zero();
}

internal String8
dw_interp_string(DW_Input    *input,
                 DW_Format    unit_format,
                 DW_ListUnit *str_offsets,
                 DW_FormKind  form_kind,
                 DW_Form      form)
{
  String8 string = {0};
  if (form_kind == DW_Form_String) {
    string = form.string;
  } else if (form_kind == DW_Form_Strp) {
    U64 bytes_read = str8_deserial_read_cstr(input->sec[DW_Section_Str].data, form.sec_offset, &string);
    Assert(bytes_read > 0);
  } else if (form_kind == DW_Form_LineStrp) {
    U64 bytes_read = str8_deserial_read_cstr(input->sec[DW_Section_LineStr].data, form.sec_offset, &string);
    Assert(bytes_read > 0);
  } else if (form_kind == DW_Form_StrpSup) {
    U64 bytes_read = str8_deserial_read_cstr(input->sec[DW_Section_Str].data, form.strp_sup, &string);
    Assert(bytes_read > 0);
  } else if (form_kind == DW_Form_Strx || form_kind == DW_Form_Strx1 ||
             form_kind == DW_Form_Strx2 || form_kind == DW_Form_Strx3 ||
             form_kind == DW_Form_Strx4) {
    U64 sec_offset = dw_offset_from_list_unit(str_offsets, form.xval);
    if (sec_offset < input->sec[DW_Section_Str].data.size) {
      U64 bytes_read = str8_deserial_read_cstr(input->sec[DW_Section_Str].data, sec_offset, &string);
      Assert(bytes_read > 0);
    } else {
      AssertAlways(!"unable to translate index to offset");
    }
  } else if (form_kind == DW_Form_GNU_StrpAlt) {
    NotImplemented;
  } else if (form_kind == DW_Form_GNU_StrIndex) {
    NotImplemented;
  } else if (form_kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return string;
}

internal String8
dw_interp_line_ptr(DW_Input *input, DW_FormKind form_kind, DW_Form form)
{
  String8 result = {0};
  if (form_kind == DW_Form_SecOffset) {
    result = str8_skip(input->sec[DW_Section_Line].data, form.sec_offset);
  } else if (form_kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return result;
}

internal DW_LineFile *
dw_interp_file(DW_LineVMHeader *line_vm, DW_FormKind form_kind, DW_Form form)
{
  DW_LineFile *result = 0;
  U64 file_idx = dw_interp_const_u64(form_kind, form);
  if (file_idx < line_vm->file_table.count) {
    result = &line_vm->file_table.v[file_idx];
  } else {
    Assert(!"out of bounds file index");
  }
  return result;
}

internal DW_Reference
dw_interp_ref(DW_Input *input, DW_CompUnit *cu, DW_FormKind form_kind, DW_Form form)
{
  DW_Reference ref = {0};
  if (form_kind == DW_Form_Ref1 || form_kind == DW_Form_Ref2 ||
      form_kind == DW_Form_Ref4 || form_kind == DW_Form_Ref8 ||
      form_kind == DW_Form_RefUData) {
    ref.cu = cu;
    ref.info_off = form.ref;
  } else if (form_kind == DW_Form_RefAddr) {
    NotImplemented;
  } else if (form_kind == DW_Form_RefSig8) {
    NotImplemented;
  } else if (form_kind == DW_Form_RefSup4 || form_kind == DW_Form_RefSup8) {
    NotImplemented;
  } else if (form_kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return ref;
}

internal DW_LocList
dw_interp_loclist(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_FormKind form_kind, DW_Form form)
{
  DW_LocList loclist = {0};
  
  if (cu->version < DW_Version_5) {
    if (form_kind == DW_Form_SecOffset) {
      U64 sec_offset = max_U64;
      if (form_kind == DW_Form_SecOffset) {
        sec_offset = form.sec_offset;
      } else if (form_kind == DW_Form_Data8 || form_kind == DW_Form_Data4 ||
                 form_kind == DW_Form_Data2 || form_kind == DW_Form_Data1) {
        if (!dw_try_u64_from_const_value(form.data.size, DW_ATE_Unsigned, form.data, &sec_offset)) {
          Assert(!"unable to extract section offset");
        }
      } else if (form_kind == DW_Form_Null) {
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
    } else if (form_kind != DW_Form_Null) {
      AssertAlways(!"unexpected form");
    }
  } else {
    DW_Version version = DW_Version_Null;
    String8    raw_lle = {0};
    if (form_kind == DW_Form_SecOffset) {
      // offset is from beginning of the section
      U64 sec_offset = form.sec_offset;
      raw_lle = str8_skip(input->sec[DW_Section_LocLists].data, sec_offset);
    } else if (form_kind == DW_Form_LocListx) {
      // offset is from beginning of the entries
      U64 entries_off = dw_offset_from_list_unit(cu->loclists_lu, form.xval);
      raw_lle         = str8_skip(cu->loclists_lu->entries, entries_off);
      version         = cu->loclists_lu->version;
    } else if (form_kind != DW_Form_Null) {
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
dw_interp_flag(DW_FormKind form_kind, DW_Form form)
{
  B32 flag = 0;
  if (form_kind == DW_Form_Flag || form_kind == DW_Form_FlagPresent) {
    flag = form.flag;
  } else if (form_kind != DW_Form_Null) {
    AssertAlways(!"unexpected form");
  }
  return flag;
}

internal Rng1U64List
dw_interp_rnglist(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_FormKind form_kind, DW_Form form)
{
  Rng1U64List rnglist = {0};
  
  if (cu->version < DW_Version_5) {
    // decode section offset
    U64 sec_offset = max_U64;
    if (form_kind == DW_Form_SecOffset) {
      sec_offset = form.sec_offset;
    } else if (form_kind == DW_Form_Data8 || form_kind == DW_Form_Data4 ||
               form_kind == DW_Form_Data2 || form_kind == DW_Form_Data1) {
      if (!dw_try_u64_from_const_value(form.data.size, DW_ATE_Unsigned, form.data, &sec_offset)) {
        Assert(!"unable to extract section offset");
      }
    } else if (form_kind != DW_Form_Null) {
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
    if (form_kind == DW_Form_SecOffset) {
      // offset is from beginning of the section
      U64 sec_offset = form.sec_offset;
      raw_rle = str8_skip(input->sec[DW_Section_RngLists].data, sec_offset);
    } else if (form_kind == DW_Form_RngListx) {
      // offset is from beginning of the entries
      U64 sec_offset = dw_offset_from_list_unit(cu->rnglists_lu, form.xval);
      raw_rle        = str8_skip(cu->rnglists_lu->entries, sec_offset);
    } else if (form_kind != DW_Form_Null) {
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
dw_interp_secptr(DW_Input *input, DW_SectionKind section, DW_FormKind form_kind, DW_Form form)
{
  String8 secptr = {0};
  if (form_kind == DW_Form_SecOffset) {
    String8 sect  = input->sec[section].data;
    Rng1U64 range = rng_1u64(form.sec_offset, sect.size);
    secptr = str8_substr(sect, range);
  } else if (form_kind != DW_Form_Null) {
    Assert(!"unexpected form");
  }
  return secptr;
}

internal String8
dw_interp_addrptr(DW_Input *input, DW_FormKind form_kind, DW_Form form)
{
  return dw_interp_secptr(input, DW_Section_Addr, form_kind, form);
}

internal String8
dw_interp_str_offsets_ptr(DW_Input *input, DW_FormKind form_kind, DW_Form form)
{
  return dw_interp_secptr(input, DW_Section_StrOffsets, form_kind, form);
}

internal String8
dw_interp_rnglists_ptr(DW_Input *input, DW_FormKind form_kind, DW_Form form)
{
  return dw_interp_secptr(input, DW_Section_RngLists, form_kind, form);
}

internal String8
dw_interp_loclists_ptr(DW_Input *input, DW_FormKind form_kind, DW_Form form)
{
  return dw_interp_secptr(input, DW_Section_LocLists, form_kind, form);
}

internal DW_AttribClass
dw_value_class_from_attrib(DW_CompUnit *cu, DW_Attrib *attrib)
{
  return dw_pick_attrib_value_class(cu->version, cu->ext, cu->relaxed, attrib->attrib_kind, attrib->form_kind);
}

internal String8
dw_exprloc_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_ExprLoc || value_class == DW_AttribClass_Block);
  return dw_interp_exprloc(attrib->form_kind, attrib->form);
}

internal U128
dw_const_u128_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_const_u128(attrib->form_kind, attrib->form);
}

internal U64
dw_const_u64_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_const_u64(attrib->form_kind, attrib->form);
}

internal U32
dw_const_u32_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_const_u32(attrib->form_kind, attrib->form);
}

internal S64
dw_const_s64_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_const_s64(attrib->form_kind, attrib->form);
}

internal S32
dw_const_s32_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_const_s32(attrib->form_kind, attrib->form);
}

internal B32
dw_flag_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Flag);
  return dw_interp_flag(attrib->form_kind, attrib->form);
}

internal U64
dw_address_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null ||
               value_class == DW_AttribClass_Address ||
               value_class == DW_AttribClass_AddrPtr);
  DW_FormKind form_kind = attrib->form_kind;
  DW_Form     form      = attrib->form;
  if (value_class == DW_AttribClass_AddrPtr) {
    
    if (attrib->form_kind == DW_Form_SecOffset) {
      
      
    } else {
      AssertAlways(!"unexpected form");
    }
    
    
    form_kind = DW_Form_Addr;
    form.addr = dw_interp_addrptr(input, attrib->form_kind, attrib->form);
  }
  return dw_interp_address(cu->address_size, cu->low_pc, cu->addr_lu, form_kind, form);
}

internal String8
dw_block_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Block);
  return dw_interp_block(input, cu, attrib->form_kind, attrib->form);
}

internal String8
dw_string_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_String || value_class == DW_AttribClass_StrOffsetsPtr);
  return dw_interp_string(input, cu->format, cu->str_offsets_lu, attrib->form_kind, attrib->form);
}

internal String8
dw_line_ptr_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_LinePtr);
  return dw_interp_line_ptr(input, attrib->form_kind, attrib->form);
}

internal DW_LineFile *
dw_file_from_attrib_ptr(DW_CompUnit *cu, DW_LineVMHeader *line_vm, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Const);
  return dw_interp_file(line_vm, attrib->form_kind, attrib->form);
}

internal DW_Reference
dw_ref_from_attrib_ptr(DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null || value_class == DW_AttribClass_Reference);
  return dw_interp_ref(input, cu, attrib->form_kind, attrib->form);
}

internal DW_LocList
dw_loclist_from_attrib_ptr(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  AssertAlways(value_class == DW_AttribClass_Null ||
               value_class == DW_AttribClass_LocList ||
               value_class == DW_AttribClass_LocListPtr);
  return dw_interp_loclist(arena, input, cu, attrib->form_kind, attrib->form);
}

internal Rng1U64List
dw_rnglist_from_attrib_ptr(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_Attrib *attrib)
{
  Rng1U64List rnglist = {0};
  DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
  if (value_class == DW_AttribClass_RngListPtr || value_class == DW_AttribClass_RngList) {
    rnglist = dw_interp_rnglist(arena, input, cu, attrib->form_kind, attrib->form);
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
  
  if (attrib->attrib_kind == DW_Attrib_Null) {
    if (cu && cu->tag_ht) {
      DW_Attrib *ao_attrib = dw_attrib_from_tag_(tag, DW_Attrib_AbstractOrigin);
      if (ao_attrib->attrib_kind == DW_Attrib_AbstractOrigin) {
        DW_Reference  ref     = dw_interp_ref(input, cu, ao_attrib->form_kind, ao_attrib->form);
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
  B32 has_attrib = attrib->attrib_kind != DW_Attrib_Null;
  return has_attrib;
}

internal String8
dw_exprloc_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_exprloc_from_attrib_ptr(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal String8
dw_block_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_block_from_attrib_ptr(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal U128
dw_const_u128_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_const_u128_from_attrib_ptr(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal U64
dw_const_u64_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_const_u64_from_attrib_ptr(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal U32
dw_const_u32_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_const_u32_from_attrib_ptr(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal U64
dw_address_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_address_from_attrib_ptr(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal String8
dw_string_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_string_from_attrib_ptr(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal String8
dw_line_ptr_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_line_ptr_from_attrib_ptr(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal DW_Reference
dw_ref_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_ref_from_attrib_ptr(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal DW_LocList
dw_loclist_from_attrib(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_loclist_from_attrib_ptr(arena, input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal Rng1U64List
dw_rnglist_from_attrib(Arena *arena, DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_rnglist_from_attrib_ptr(arena, input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal B32
dw_flag_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  return dw_flag_from_attrib_ptr(input, cu, dw_attrib_from_tag(input, cu, tag, kind));
}

internal DW_LineFile *
dw_file_from_attrib(DW_Input *input, DW_CompUnit *cu, DW_LineVMHeader *line_vm, DW_Tag tag, DW_AttribKind kind)
{
  return dw_file_from_attrib_ptr(cu, line_vm, dw_attrib_from_tag(input, cu, tag, kind));
}

internal B32
dw_try_byte_size_from_tag(DW_Input *input, DW_CompUnit *cu, DW_Tag tag, U64 *byte_size_out)
{
  B32 has_byte_size = dw_tag_has_attrib(input, cu, tag, DW_Attrib_ByteSize);
  B32 has_bit_size  = dw_tag_has_attrib(input, cu, tag, DW_Attrib_BitSize );
  
  if (has_byte_size && has_bit_size) {
    Assert(!"ill formated byte size");
  }
  
  if (has_byte_size) {
    *byte_size_out = dw_const_u64_from_attrib(input, cu, tag, DW_Attrib_ByteSize); 
    return 1;
  } else if (has_bit_size) {
    U64 bit_size = dw_const_u64_from_attrib(input, cu, tag, DW_Attrib_BitSize);
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
    if (dw_tag_has_attrib(input, cu, tag, DW_Attrib_Type)) {
      Temp scratch = scratch_begin(0,0);
      DW_Reference type_ref       = dw_ref_from_attrib(input, cu, tag, DW_Attrib_Type);
      DW_Tag type_tag = {0};
      dw_read_tag_cu(scratch.arena, input, type_ref.cu, type_ref.info_off, &type_tag);
      U64          type_byte_size = dw_byte_size_from_tag(input, cu, type_tag);
      DW_ATE       type_encoding  = dw_const_u64_from_attrib(input, type_ref.cu, type_tag, DW_Attrib_Encoding);
      if (type_encoding == DW_ATE_Unsigned || type_encoding == DW_ATE_UnsignedChar) {
        result = dw_interp_const64(type_byte_size, type_encoding, attrib->form_kind, attrib->form);
      }
      scratch_end(scratch);
    } else {
      result = dw_interp_const_u64(attrib->form_kind, attrib->form);
    }
  } else if (attrib_class == DW_AttribClass_Reference) {
    NotImplemented;
  } else if (attrib_class != DW_AttribClass_Null) {
    AssertAlways(!"unexpected attrib class");
  }
  return result;
}

internal DW_CompUnit
dw_cu_from_info_off(Arena *arena, DW_Input *input, DW_ListUnitInput lu_input, U64 offset, B32 relaxed)
{
  DW_CompUnit cu = {0};
  
  String8 info_data = input->sec[DW_Section_Info].data;
  
  // read unit size in bytes
  U64 length      = 0;
  U64 length_size = str8_deserial_read_dwarf_packed_size(info_data, offset, &length);
  
  if (length_size) {
    // compute unit range
    Rng1U64 range  = rng_1u64(offset, offset + length_size + length);
    String8 data   = str8_substr(info_data, range);
    U64     cursor = length_size;
    
    // read version
    DW_Version version = 0;
    U64 version_size = str8_deserial_read_struct(data, cursor, &version);
    cursor += version_size;
    
    if (version_size) {
      DW_Format       format       = DW_FormatFromSize(length);
      B32             is_header_ok = 0;
      U64             abbrev_base  = max_U64;
      U8              address_size = 0;
      DW_CompUnitKind unit_kind    = DW_CompUnitKind_Reserved;
      U64             spec_dwo_id  = max_U64;
      
      switch (version) {
        default:
        case DW_Version_Null:
        case DW_Version_1:
        break;
        case DW_Version_2: {
          U32 abbrev_base32    = 0;
          U64 abbrev_base_off  = cursor;
          U64 abbrev_base_size = str8_deserial_read_struct(data, abbrev_base_off, &abbrev_base32);
          if (!abbrev_base_size) {
            break;
          }
          
          U64 address_size_off  = abbrev_base_off + abbrev_base_size;
          U64 address_size_size = str8_deserial_read_struct(data, address_size_off, &address_size);
          if (!address_size_size) {
            break;
          }
          
          abbrev_base  = abbrev_base32;
          cursor       = address_size_off + address_size_size;
          is_header_ok = 1;
        } break;
        case DW_Version_3:
        case DW_Version_4: {
          U64 abbrev_base_off  = cursor;
          U64 abbrev_base_size = str8_deserial_read_dwarf_uint(data, abbrev_base_off, format, &abbrev_base);
          if (!abbrev_base_size) {
            break;
          }
          
          U64 address_size_off  = abbrev_base_off + abbrev_base_size;
          U64 address_size_size = str8_deserial_read_struct(data, address_size_off, &address_size);
          if (!address_size_size) {
            break;
          }
          
          cursor       = address_size_off + address_size_size;
          is_header_ok = 1;
        } break;
        case DW_Version_5: {
          U64 unit_kind_off  = cursor;
          U64 unit_kind_size = str8_deserial_read_struct(data, unit_kind_off, &unit_kind);
          if (unit_kind_size == 0) {
            break;
          }
          
          U64 address_size_off  = unit_kind_off + unit_kind_size;
          U64 address_size_size = str8_deserial_read_struct(data, address_size_off, &address_size);
          if (!address_size_size) {
            break;
          }
          
          U64 abbrev_base_off  = address_size_off + address_size_size;
          U64 abbrev_base_size = str8_deserial_read_dwarf_uint(data, abbrev_base_off, format, &abbrev_base);
          if (!abbrev_base_size) {
            break;
          }
          
          U64 spec_dwo_id_off  = abbrev_base_off + abbrev_base_size;
          U64 spec_dwo_id_size = 0;
          if (unit_kind == DW_CompUnitKind_Skeleton || input->sec[DW_Section_Info].is_dwo) {
            spec_dwo_id_size = str8_deserial_read_struct(data, spec_dwo_id_off, &spec_dwo_id);
            if (!spec_dwo_id_size) {
              break;
            }
          }
          
          cursor       = spec_dwo_id_off + spec_dwo_id_size;
          is_header_ok = 1;
        } break;
      }
      
      if (is_header_ok) {
        Temp temp = temp_begin(arena);
        
        // TODO: cache abbrev tables with identical offsets
        String8        abbrev_data  = input->sec[DW_Section_Abbrev].data;
        DW_AbbrevTable abbrev_table = dw_make_abbrev_table(arena, abbrev_data, abbrev_base);
        
        DW_Tag cu_tag = {0};
        dw_read_tag(arena, data, cursor, range.min, abbrev_table, abbrev_data, version, format, address_size, &cu_tag);
        
        // TODO: handle these unit types
        Assert(cu_tag.kind != DW_Tag_SkeletonUnit);
        Assert(cu_tag.kind != DW_Tag_TypeUnit);
        
        if (cu_tag.kind == DW_Tag_CompileUnit || cu_tag.kind == DW_Tag_PartialUnit) {
          // fetch attribs for list sections
          DW_Attrib *addr_base_attrib        = dw_attrib_from_tag(0, 0, cu_tag, DW_Attrib_AddrBase      );
          DW_Attrib *str_offsets_base_attrib = dw_attrib_from_tag(0, 0, cu_tag, DW_Attrib_StrOffsetsBase);
          DW_Attrib *rnglists_base_attrib    = dw_attrib_from_tag(0, 0, cu_tag, DW_Attrib_RngListsBase  );
          DW_Attrib *loclists_base_attrib    = dw_attrib_from_tag(0, 0, cu_tag, DW_Attrib_LocListsBase  );
          
          // interp attribs as section offsets
          U64 addr_sec_off        = dw_interp_sec_offset(addr_base_attrib->form_kind,        addr_base_attrib->form       );
          U64 str_offsets_sec_off = dw_interp_sec_offset(str_offsets_base_attrib->form_kind, str_offsets_base_attrib->form);
          U64 rnglists_sec_off    = dw_interp_sec_offset(rnglists_base_attrib->form_kind,    rnglists_base_attrib->form   );
          U64 loclists_sec_off    = dw_interp_sec_offset(loclists_base_attrib->form_kind,    loclists_base_attrib->form   );
          
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
          DW_Attrib *low_pc_attrib = dw_attrib_from_tag(0, 0, cu_tag, DW_Attrib_LowPc);
          U64        low_pc        = dw_interp_address(address_size, max_U64, addr_lu, low_pc_attrib->form_kind, low_pc_attrib->form);
          
          // fill out compile unit
          cu.relaxed            = relaxed;
          cu.ext                = DW_Ext_All;
          cu.kind               = unit_kind;
          cu.version            = version;
          cu.format             = format;
          cu.address_size       = address_size;
          cu.abbrev_off         = abbrev_base;
          cu.info_range         = range;
          cu.first_tag_info_off = range.min + cursor;
          cu.abbrev_table       = abbrev_table;
          cu.abbrev_data        = abbrev_data;
          cu.addr_lu            = addr_lu;
          cu.str_offsets_lu     = str_offsets_lu;
          cu.rnglists_lu        = rnglists_lu;
          cu.loclists_lu        = loclists_lu;
          cu.low_pc             = low_pc;
          cu.tag                = cu_tag;
        } else { 
          // unexpected tag, release memory and exit
          temp_end(temp);
        }
      }
    }
  }
  
  return cu;
}

internal void
dw_tag_tree_from_data(Arena *arena, String8 info_data, String8 abbrev_data, DW_CompUnit *cu, DW_TagNode *parent, U64 *cursor, U64 *tag_count)
{
  while (*cursor < info_data.size) {
    // read tag
    DW_Tag tag = {0};
    U64 tag_size = dw_read_tag(arena, info_data, *cursor, cu->info_range.min, cu->abbrev_table, abbrev_data, cu->version, cu->format, cu->address_size, &tag);
    if (tag_size == 0) {
      break;
    }
    *cursor += tag_size;
    
    // is this sentinel tag?
    if (tag.kind == DW_Tag_Null) {
      break;
    }
    
    // normal tag
    DW_TagNode *tag_n = push_array(arena, DW_TagNode, 1);
    tag_n->tag        = tag;
    
    SLLQueuePush_N(parent->first_child, parent->last_child, tag_n, sibling);
    
    // update tag count
    *tag_count += 1;
    
    if (tag.has_children) {
      dw_tag_tree_from_data(arena, info_data, abbrev_data, cu, tag_n, cursor, tag_count);
    }
  }
}

internal DW_TagTree
dw_tag_tree_from_cu(Arena *arena, DW_Input *input, DW_CompUnit *cu)
{
  String8    abbrev_data = input->sec[DW_Section_Abbrev].data;
  String8    info_data   = str8_substr(input->sec[DW_Section_Info].data, cu->info_range);
  DW_TagNode root        = {0};
  U64        cursor      = cu->first_tag_info_off;
  U64        tag_count   = 0;
  dw_tag_tree_from_data(arena, info_data, abbrev_data, cu, &root, &cursor, &tag_count);
  
  DW_TagTree result = {0};
  result.root       = root.first_child;
  result.tag_count  = tag_count;
  
  return result;
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

internal DW_LineVMFileArray
dw_line_vm_file_array_from_list(Arena *arena, DW_LineVMFileList list)
{
  DW_LineVMFileArray result = {0};
  result.count              = 0;
  result.v                  = push_array(arena, DW_LineFile, list.node_count);
  
  for (DW_LineVMFileNode *src = list.first; src != 0; src = src->next) {
    DW_LineFile *dst = &result.v[result.count++];
    dst->file_name   = push_str8_copy(arena, src->file.file_name);
    dst->dir_idx     = src->file.dir_idx;
    dst->modify_time = src->file.modify_time;
    dst->file_size   = src->file.file_size;
  }
  
  return result;
}

internal U64
dw_read_line_file(String8       data,
                  U64           off,
                  DW_Input     *input,
                  DW_Version    version,
                  DW_Format     format,
                  DW_Ext        ext,
                  U64           address_size,
                  DW_ListUnit  *str_offsets,
                  U64           enc_count,
                  U64          *enc_arr,
                  DW_LineFile  *line_file_out)
{
  MemoryZeroStruct(line_file_out);
  U64 cursor = off;
  for (U64 enc_idx = 0; enc_idx < enc_count; ++enc_idx) {
    DW_LNCT     lnct      = enc_arr[enc_idx*2 + 0];
    DW_FormKind form_kind = enc_arr[enc_idx*2 + 1];
    DW_Form     form      = {0};
    U64         bytes_read;
    switch (lnct) {
      case DW_LNCT_Path: {
        bytes_read = dw_read_form(data, cursor, version, format, address_size, form_kind, max_U64, &form);
        line_file_out->file_name = dw_interp_string(input, format, str_offsets, form_kind, form);
      } break;
      case DW_LNCT_DirectoryIndex: {
        bytes_read = dw_read_form(data, cursor, version, format, address_size, form_kind, max_U64, &form);
        line_file_out->dir_idx = dw_interp_const_u64(form_kind, form);
      } break;
      case DW_LNCT_TimeStamp: {
        bytes_read = dw_read_form(data, cursor, version, format, address_size, form_kind, max_U64, &form);
        line_file_out->modify_time = dw_interp_const_u64(form_kind, form);
      } break;
      case DW_LNCT_Size: {
        bytes_read = dw_read_form(data, cursor, version, format, address_size, form_kind, max_U64, &form);
        line_file_out->file_size = dw_interp_const_u64(form_kind, form);
      } break;
      case DW_LNCT_MD5: {
        bytes_read = dw_read_form(data, cursor, version, format, address_size, form_kind, max_U64, &form);
        line_file_out->md5_digest = dw_interp_const_u128(form_kind, form);
      } break;
      case DW_LNCT_LLVM_Source: {
        if (ext & DW_Ext_LLVM) {
          bytes_read = dw_read_form(data, cursor, version, format, address_size, form_kind, max_U64, &form);
          line_file_out->source = dw_interp_string(input, format, str_offsets, form_kind, form);
        } else {
          Assert(!"extension not supported");
        }
      } break;
      default: {
        bytes_read = dw_read_form(data, cursor, version, format, address_size, form_kind, max_U64, &form);
        Assert(!"unexpected LNTC encoding");
      } break;
    }
    Assert(bytes_read);
    cursor += bytes_read;
  }
  U64 bytes_read = cursor - off;
  return bytes_read;
}

internal U64
dw_read_line_file_array(Arena              *arena,
                        String8             data,
                        U64                 off,
                        DW_Input           *input,
                        DW_Version          version,
                        DW_Format           format,
                        DW_Ext              ext,
                        U64                 address_size,
                        DW_ListUnit        *str_offsets,
                        U64                 enc_count,
                        U64                *enc_arr,
                        U64                 table_count,
                        DW_LineVMFileArray *table_out)
{
  Temp temp = temp_begin(arena);
  
  table_out->count = table_count;
  table_out->v     = push_array(arena, DW_LineFile, table_count);
  
  U64 i, cursor;
  for (i = 0, cursor = off; i < table_count; ++i) {
    U64 bytes_read = dw_read_line_file(data,
                                       cursor,
                                       input,
                                       version,
                                       format,
                                       ext,
                                       address_size,
                                       str_offsets,
                                       enc_count,
                                       enc_arr,
                                       &table_out->v[i]);
    if (bytes_read == 0) {
      break;
    }
    cursor += bytes_read;
  }
  
  U64 bytes_read = 0;
  if (i == table_count) {
    bytes_read = cursor - off;
  } else {
    temp_end(temp);
    table_out->count = 0;
    table_out->v     = 0;
  }
  
  return bytes_read;
}

internal U64
dw_read_line_vm_header(Arena           *arena,
                       String8          line_data,
                       U64              line_off,
                       DW_Input        *input,
                       String8          cu_dir,
                       String8          cu_name,
                       U8               cu_address_size, 
                       DW_ListUnit     *cu_str_offsets,
                       DW_LineVMHeader *header_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  U64 bytes_read = 0;
  
  // read unit length
  U64 unit_length      = 0;
  U64 unit_length_size = str8_deserial_read_dwarf_packed_size(line_data, line_off, &unit_length);
  
  U64       unit_opl    = line_off + unit_length_size + unit_length;
  Rng1U64   unit_range  = rng_1u64(line_off, unit_opl);
  DW_Format format      = DW_FormatFromSize(unit_length);
  U64       unit_cursor = unit_length_size;
  String8   unit_data   = str8_substr(line_data, unit_range);
  
  // read unit version
  DW_Version version = DW_Version_Null;
  U64 version_size = str8_deserial_read_struct(unit_data, unit_cursor, &version);
  if (version_size == 0) {
    goto exit;
  }
  unit_cursor += version_size;
  
  // read DWARF5 address & segment selector
  U8 address_size = 0;
  U8 segsel_size  = 0;
  if (version == DW_Version_5) {
    U64 address_size_size = str8_deserial_read_struct(unit_data, unit_cursor, &address_size);
    if (address_size_size == 0) {
      goto exit;
    }
    unit_cursor += address_size_size;
    
    U64 segsel_size_size = str8_deserial_read_struct(unit_data, unit_cursor, &segsel_size);
    if (segsel_size_size == 0) {
      goto exit;
    }
    unit_cursor += segsel_size_size;
  } else {
    address_size = cu_address_size;
  }
  
  // read header length
  U64 header_length = 0;
  U64 header_length_size = str8_deserial_read_dwarf_uint(unit_data, unit_cursor, format, &header_length);
  if (header_length_size == 0) {
    goto exit;
  }
  unit_cursor += header_length_size;
  
  // read min instruction length
  U8  min_inst_len      = 0;
  U64 min_inst_len_size = str8_deserial_read_struct(unit_data, unit_cursor, &min_inst_len);
  if (min_inst_len_size == 0) {
    goto exit;
  }
  unit_cursor += min_inst_len_size;
  
  // read max operands for instruction
  U8 max_ops_for_inst = 1;
  if (version > DW_Version_3) {
    U64 max_ops_for_inst_size = str8_deserial_read_struct(unit_data, unit_cursor, &max_ops_for_inst);
    if (max_ops_for_inst_size == 0) {
      goto exit;
    }
    unit_cursor += max_ops_for_inst_size;
  }
  Assert(max_ops_for_inst > 0);
  
  U8  default_is_stmt      = 0;
  U64 default_is_stmt_size = str8_deserial_read_struct(unit_data, unit_cursor, &default_is_stmt);
  if (default_is_stmt_size == 0) {
    goto exit;
  }
  unit_cursor += default_is_stmt_size;
  
  S8  line_base      = 0;
  U64 line_base_size = str8_deserial_read_struct(unit_data, unit_cursor, &line_base);
  if (line_base_size == 0) {
    goto exit;
  }
  unit_cursor += line_base_size;
  
  U8  line_range      = 0;
  U64 line_range_size = str8_deserial_read_struct(unit_data, unit_cursor, &line_range);
  if (line_range_size == 0) {
    goto exit;
  }
  unit_cursor += line_range_size;
  
  U8  opcode_base      = 0;
  U64 opcode_base_size = str8_deserial_read_struct(unit_data, unit_cursor, &opcode_base);
  if (opcode_base_size == 0) {
    goto exit;
  }
  unit_cursor += opcode_base_size;
  
  U64 num_opcode_lens = opcode_base > 0 ? opcode_base - 1 : 0;
  U8 *opcode_lens     = str8_deserial_get_raw_ptr(unit_data, unit_cursor, num_opcode_lens * sizeof(opcode_lens[0]));
  if (opcode_lens == 0) {
    goto exit;
  }
  unit_cursor += num_opcode_lens * sizeof(opcode_lens[0]);
  
  DW_LineVMFileArray dir_table  = {0};
  DW_LineVMFileArray file_table = {0};
  if (version < DW_Version_5) {
    // read directory table
    DW_LineVMFileList dir_list = {0};
    {
      // compile directory is always first in the table
      DW_LineVMFileNode *node = push_array(scratch.arena, DW_LineVMFileNode, 1);
      node->file.file_name    = cu_dir;
      SLLQueuePush(dir_list.first, dir_list.last, node);
      ++dir_list.node_count;
    }
    
    // parse additional directories
    for (; unit_cursor < unit_data.size; ) {
      String8 dir = {0};
      unit_cursor += str8_deserial_read_cstr(unit_data, unit_cursor, &dir);
      if (dir.size == 0) {
        break;
      }
      
      DW_LineVMFileNode *node = push_array(scratch.arena, DW_LineVMFileNode, 1);
      node->file.file_name    = dir;
      SLLQueuePush(dir_list.first, dir_list.last, node);
      ++dir_list.node_count;
    }
    
    DW_LineVMFileList file_list = {0};
    {
      // compile unit name is always first in the file table
      {
        DW_LineVMFileNode *node = push_array(scratch.arena, DW_LineVMFileNode, 1);
        node->file.file_name    = cu_name;
        SLLQueuePush(file_list.first, file_list.last, node);
        ++file_list.node_count;
      }
      
      // read file table
      for (; unit_cursor < unit_data.size; ) {
        String8 file_name = {0};
        unit_cursor += str8_deserial_read_cstr(unit_data, unit_cursor, &file_name);
        if (file_name.size == 0) {
          break;
        }
        
        U64 dir_index      = 0;
        U64 dir_index_size = str8_deserial_read_uleb128(unit_data, unit_cursor, &dir_index);
        if (dir_index_size == 0) {
          goto exit;
        }
        unit_cursor += dir_index_size;
        
        U64 modify_time      = 0;
        U64 modify_time_size = str8_deserial_read_uleb128(unit_data, unit_cursor, &modify_time);
        if (modify_time_size == 0) {
          goto exit;
        }
        unit_cursor += modify_time_size;
        
        U64 file_size      = 0;
        U64 file_size_size = str8_deserial_read_uleb128(unit_data, unit_cursor, &file_size);
        if (file_size_size == 0) {
          goto exit;
        }
        unit_cursor += file_size_size;
        
        DW_LineVMFileNode *node = push_array(scratch.arena, DW_LineVMFileNode, 1);
        node->file.file_name    = file_name;
        node->file.dir_idx      = dir_index;
        node->file.modify_time  = modify_time;
        node->file.file_size    = file_size;
        
        SLLQueuePush(file_list.first, file_list.last, node);
        ++file_list.node_count;
      }
    }
    
    // list -> array
    dir_table  = dw_line_vm_file_array_from_list(arena, dir_list);
    file_table = dw_line_vm_file_array_from_list(arena, file_list);
  }
  // DWARF5
  else {
    // directory table
    {
      // read table entry encoding count
      U8  enc_count      = 0;
      U64 enc_count_size = str8_deserial_read_struct(unit_data, unit_cursor, &enc_count);
      if (enc_count_size == 0) {
        goto exit;
      }
      unit_cursor += enc_count_size;
      
      // read table entry encodings
      U64 *enc_arr      = 0;
      U64  enc_arr_size = str8_deserial_read_uleb128_array(scratch.arena, unit_data, unit_cursor, enc_count*2, &enc_arr);
      if (enc_arr_size == 0) {
        goto exit;
      }
      unit_cursor += enc_arr_size;
      
      // read table count
      U64 table_count      = 0;
      U64 table_count_size = str8_deserial_read_uleb128(unit_data, unit_cursor, &table_count);
      if (table_count_size == 0) {
        goto exit;
      }
      unit_cursor += table_count_size;
      
      // read table
      U64 table_size = dw_read_line_file_array(arena,
                                               unit_data,
                                               unit_cursor,
                                               input,
                                               version,
                                               format,
                                               DW_Ext_All,
                                               address_size,
                                               cu_str_offsets,
                                               enc_count,
                                               enc_arr,
                                               table_count,
                                               &dir_table);
      if (table_size == 0) {
        goto exit;
      }
      unit_cursor += table_size;
    }
    
    // file table
    {
      // read table entry encoding count
      U8  enc_count      = 0;
      U64 enc_count_size = str8_deserial_read_struct(unit_data, unit_cursor, &enc_count);
      if (enc_count == 0) {
        goto exit;
      }
      unit_cursor += enc_count_size;
      
      // read table entry encodings
      U64 *enc_arr = 0;
      U64  enc_arr_size = str8_deserial_read_uleb128_array(scratch.arena, unit_data, unit_cursor, enc_count*2, &enc_arr);
      if (enc_arr_size == 0) {
        goto exit;
      }
      unit_cursor += enc_arr_size;
      
      // read table count
      U64 table_count      = 0;
      U64 table_count_size = str8_deserial_read_uleb128(unit_data, unit_cursor, &table_count);
      if (table_count_size == 0) {
        goto exit;
      }
      unit_cursor += table_count_size;
      
      // read table
      U64 file_table_size = dw_read_line_file_array(arena,
                                                    unit_data,
                                                    unit_cursor,
                                                    input,
                                                    version,
                                                    format,
                                                    DW_Ext_All,
                                                    address_size,
                                                    cu_str_offsets,
                                                    enc_count,
                                                    enc_arr,
                                                    table_count,
                                                    &file_table);
      if (file_table_size == 0) {
        goto exit;
      }
      unit_cursor += file_table_size;
    }
  }
  
  if (header_out) {
    header_out->unit_range            = unit_range;
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
    header_out->opcode_lens           = opcode_lens;
    header_out->dir_table             = dir_table;
    header_out->file_table            = file_table;
  }
  
  bytes_read = unit_cursor;
  
  exit:;
  scratch_end(scratch);
  return bytes_read;
}

internal void
dw_line_vm_reset(DW_LineVMState *state, B32 default_is_stmt)
{
  state->address         = 0;
  state->op_index        = 0;
  state->file_index      = 1;
  state->line            = 1;
  state->column          = 0;
  state->is_stmt         = default_is_stmt;
  state->basic_block     = 0;
  state->prologue_end    = 0;
  state->epilogue_begin  = 0;
  state->isa             = 0;
  state->discriminator   = 0;
}

internal void
dw_line_vm_advance(DW_LineVMState *state, U64 advance, U64 min_inst_len, U64 max_ops_for_inst)
{
  U64 op_index = state->op_index + advance;
  state->address += min_inst_len*(op_index/max_ops_for_inst);
  state->op_index = op_index % max_ops_for_inst;
}

internal DW_LineSeqNode *
dw_push_line_seq(Arena* arena, DW_LineTableParseResult *parsed_tbl)
{
  DW_LineSeqNode *new_seq = push_array(arena, DW_LineSeqNode, 1);
  SLLQueuePush(parsed_tbl->first_seq, parsed_tbl->last_seq, new_seq);
  parsed_tbl->seq_count += 1;
  return new_seq;
}

internal DW_LineNode *
dw_push_line(Arena *arena, DW_LineTableParseResult *tbl, DW_LineVMState *vm_state, B32 start_of_sequence)
{
  DW_LineSeqNode *seq = tbl->last_seq;
  if(seq == 0 || start_of_sequence)
  {
    seq = dw_push_line_seq(arena, tbl);
  }
  
  DW_LineNode *n  = push_array(arena, DW_LineNode, 1);
  n->v.file_index = vm_state->file_index;
  n->v.line       = vm_state->line;
  n->v.column     = vm_state->column;
  n->v.address    = vm_state->address;
  
  SLLQueuePush(seq->first, seq->last, n);
  seq->count += 1;
  return n;
}

internal String8
dw_path_from_file(Arena *arena, DW_LineVMHeader *vm, DW_LineFile *file)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8      dir   = vm->dir_table.v[file->dir_idx].file_name;
  PathStyle    style = path_style_from_str8(dir);
  if (style == PathStyle_Null || style == PathStyle_Relative) {
    style = path_style_from_str8(file->file_name);
  }
  
  String8List path_list = {0};
  
  if (str8_match_lit("..", dir, StringMatchFlag_RightSideSloppy)) {
    String8List comp_dir_list = str8_split_path(scratch.arena, vm->dir_table.v[0].file_name);
    str8_list_concat_in_place(&path_list, &comp_dir_list);
  }
  
  String8List dir_list = str8_split_path(scratch.arena, dir);
  str8_list_concat_in_place(&path_list, &dir_list);
  
  str8_list_push(scratch.arena, &path_list, file->file_name);
  
  str8_path_list_resolve_dots_in_place(&path_list, style);
  
  String8 path = str8_path_list_join_by_style(arena, &path_list, style);
  
  scratch_end(scratch);
  return path;
}

internal String8
dw_path_from_file_idx(Arena *arena, DW_LineVMHeader *vm, U64 file_idx)
{
  return dw_path_from_file(arena, vm, &vm->file_table.v[file_idx]);
}

internal DW_LineTableParseResult
dw_parsed_line_table_from_data(Arena       *arena,
                               String8      unit_data,
                               DW_Input    *input,
                               String8      cu_dir,
                               String8      cu_name,
                               U8           cu_address_size,
                               DW_ListUnit *cu_str_offsets)
{
  DW_LineVMHeader vm_header = {0};
  U64 vm_header_size = dw_read_line_vm_header(arena, unit_data, 0, input, cu_dir, cu_name, cu_address_size, cu_str_offsets, &vm_header);
  
  U64 unit_cursor = vm_header_size;
  
  //- rjf: prep state for VM
  DW_LineVMState vm_state = {0};
  dw_line_vm_reset(&vm_state, vm_header.default_is_stmt);
  
  //- rjf: VM loop; build output list
  DW_LineTableParseResult result     = { .vm_header = vm_header };
  B32                     end_of_seq = 0;
  B32                     error      = 0;
  for (; !error && unit_cursor < unit_data.size; ) {
    //- rjf: parse opcode
    U8 opcode = 0;
    unit_cursor += str8_deserial_read_struct(unit_data, unit_cursor, &opcode);
    
    //- rjf: do opcode action
    switch (opcode) {
      default: {
        //- rjf: special opcode case
        if (opcode >= vm_header.opcode_base) {
          U32 adjusted_opcode = (U32)(opcode - vm_header.opcode_base);
          U32 op_advance      = adjusted_opcode / vm_header.line_range;
          S32 line_inc        = (S32)vm_header.line_base + ((S32)adjusted_opcode) % (S32)vm_header.line_range;
          // TODO: can we just call dw_advance_line_vm_state_pc
          U64 addr_inc        = vm_header.min_inst_len * ((vm_state.op_index+op_advance) / vm_header.max_ops_for_inst);
          
          vm_state.address        += addr_inc;
          vm_state.op_index        = (vm_state.op_index + op_advance) % vm_header.max_ops_for_inst;
          vm_state.line            = (U32)((S32)vm_state.line + line_inc);
          vm_state.basic_block     = 0;
          vm_state.prologue_end    = 0;
          vm_state.epilogue_begin  = 0;
          vm_state.discriminator   = 0;
          
          if(vm_state.is_stmt)
          {
            dw_push_line(arena, &result, &vm_state, end_of_seq);
          }
          end_of_seq = 0;
          
#if 0
          // NOTE(rjf): DWARF has dummy lines at the end of groups of line ranges, where we'd like
          // to break line info into sequences.
          if(vm_state.line == 0)
          {
            end_of_seq = 1;
          }
#endif
        }
        // Skipping unknown opcode. This is a valid case and
        // it works because compiler stores operand lengths.
        else {
          if (0 < opcode && opcode <= vm_header.num_opcode_lens) {
            U8 num_operands = vm_header.opcode_lens[opcode - 1];
            for (U8 i = 0; i < num_operands; ++i) {
              U64 operand = 0;
              unit_cursor += str8_deserial_read_uleb128(unit_data, unit_cursor, &operand);
            }
          } else {
            error = 1;
            goto exit;
          }
        }
      } break;
      
      //- Standard opcodes
      
      case DW_StdOpcode_Copy: {
        if(vm_state.is_stmt)
        {
          dw_push_line(arena, &result, &vm_state, end_of_seq);
        }
        end_of_seq = 0;
        vm_state.discriminator   = 0;
        vm_state.basic_block     = 0;
        vm_state.prologue_end    = 0;
        vm_state.epilogue_begin  = 0;
      } break;
      
      case DW_StdOpcode_AdvancePc: {
        U64 advance = 0;
        unit_cursor += str8_deserial_read_uleb128(unit_data, unit_cursor, &advance);
        dw_line_vm_advance(&vm_state, advance, vm_header.min_inst_len, vm_header.max_ops_for_inst);
      } break;
      
      case DW_StdOpcode_AdvanceLine: {
        S64 s = 0;
        unit_cursor += str8_deserial_read_sleb128(unit_data, unit_cursor, &s);
        vm_state.line += s;
      } break;
      
      case DW_StdOpcode_SetFile: {
        U64 file_index = 0;
        unit_cursor += str8_deserial_read_uleb128(unit_data, unit_cursor, &file_index);
        vm_state.file_index = file_index;
      } break;
      
      case DW_StdOpcode_SetColumn: {
        U64 column = 0;
        unit_cursor += str8_deserial_read_uleb128(unit_data, unit_cursor, &column);
        vm_state.column = column;
      } break;
      
      case DW_StdOpcode_NegateStmt: {
        vm_state.is_stmt = !vm_state.is_stmt;
      } break;
      
      case DW_StdOpcode_SetBasicBlock: {
        vm_state.basic_block = 1;
      } break;
      
      case DW_StdOpcode_ConstAddPc: {
        U64 advance = (0xffu - vm_header.opcode_base) / vm_header.line_range;
        dw_line_vm_advance(&vm_state, advance, vm_header.min_inst_len, vm_header.max_ops_for_inst);
      } break;
      
      case DW_StdOpcode_FixedAdvancePc: {
        U16 operand = 0;
        unit_cursor += str8_deserial_read_struct(unit_data, unit_cursor, &operand);
        vm_state.address += operand;
        vm_state.op_index = 0;
      } break;
      
      case DW_StdOpcode_SetPrologueEnd: {
        vm_state.prologue_end = 1;
      } break;
      
      case DW_StdOpcode_SetEpilogueBegin: {
        vm_state.epilogue_begin = 1;
      } break;
      
      case DW_StdOpcode_SetIsa: {
        U64 v = 0;
        unit_cursor += str8_deserial_read_uleb128(unit_data, unit_cursor, &v);
        vm_state.isa = v;
      } break;
      
      //- Extended opcodes
      case DW_StdOpcode_ExtendedOpcode: {
        U64 length = 0;
        unit_cursor += str8_deserial_read_uleb128(unit_data, unit_cursor, &length);
        
        U64 extended_opl    = unit_cursor + length;
        U8  extended_opcode = 0;
        unit_cursor += str8_deserial_read_struct(unit_data, unit_cursor, &extended_opcode);
        
        switch (extended_opcode) {
          case DW_ExtOpcode_EndSequence: {
            vm_state.end_sequence = 1;
            if(vm_state.is_stmt)
            {
              dw_push_line(arena, &result, &vm_state, 0);
            }
            dw_line_vm_reset(&vm_state, vm_header.default_is_stmt);
            end_of_seq = 1;
          } break;
          
          case DW_ExtOpcode_SetAddress: {
            U64 address = 0;
            unit_cursor += str8_deserial_read(unit_data, unit_cursor, &address, vm_header.address_size, vm_header.address_size);
            vm_state.address    = address;
            vm_state.op_index   = 0;
          } break;
          
          case DW_ExtOpcode_DefineFile: {
            String8 file_name   = {0};
            U64     dir_index   = 0;
            U64     modify_time = 0;
            U64     file_size   = 0;
            
            unit_cursor += str8_deserial_read_cstr(unit_data, unit_cursor, &file_name);
            unit_cursor += str8_deserial_read_uleb128(unit_data, unit_cursor, &dir_index);
            unit_cursor += str8_deserial_read_uleb128(unit_data, unit_cursor, &modify_time);
            unit_cursor += str8_deserial_read_uleb128(unit_data, unit_cursor, &file_size);
            
            // TODO(rjf): Not fully implemented. By the DWARF V4 spec, the above is
            // all that needs to be parsed, but the rest of the work that needs to
            // happen here---allowing this file to be used by further opcodes---is
            // not implemented.
            //
            // See the DWARF V4 spec (June 10, 2010), page 122.
            error = 1;
            AssertAlways(!"UNHANDLED DEFINE FILE!!!");
          } break;
          
          case DW_ExtOpcode_SetDiscriminator: {
            U64 v = 0;
            unit_cursor += str8_deserial_read_uleb128(unit_data, unit_cursor, &v);
            vm_state.discriminator = v;
          } break;
          
          default: break;
        }
        
        unit_cursor = extended_opl;
      } break;
    }
  }
  
  exit:;
  
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
    
    DW_Format format = DW_FormatFromSize(unit_length);
    
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

