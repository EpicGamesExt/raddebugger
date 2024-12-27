// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// TODO(rjf):
//
// [ ] Any time we encode a subrange of a section inside of a
//     DW_AttribValue, we need to do that consistently, regardless of
//     whether or not it is a string, memory block, etc. We should just use
//     the DW_SectionKind and then the min/max pair.
//
// [ ] Things we are not reporting, or haven't figured out:
//   @dwarf_expr @dwarf_v5 @dw_cross_unit
//   [ ] currently, we're filtering out template arguments in the member accelerator.
//       this is because they don't correspond one-to-one with anything in PDB, but
//       they do contain useful information that we might want to expose another way
//       somehow.
//   [ ] DWARF V5 features that nobody seems to use right now
//     [ ] ref_addr_desc + next_info_ctx
//         apparently these are necessary when dereferencing some DWARF V5 ways of
//         forming references. They don't seem to come up at all for any real data
//         but might be a case somewhere.
//     [ ] case when only .debug_line and .debug_line_str is available, without
//         compilation unit debug info? do we care about this at all?
//     [ ] DW_Form_RefSig8, which requires using .debug_names
//         to do a lookup for a reference
//     [ ] DWARF V5, but also V1 & V2 for dw_range_list_from_range_offset
//   [ ] DW_AttribClass_RngList and DW_Form_RngListx
//   [ ] DW_OpCode_XDEREF_SIZE + DW_OpCode_XDEREF
//   [ ] DW_OpCode_PIECE + DW_OpCode_BIT_PIECE
//   [ ] DW_ExtOpcode_DefineFile, for line info
//   [ ] DWARF procedures in DWARF expr evaluation
//   [ ] DW_Attrib_DataMemberLocation is not being *fully* handled right
//       now; full handling requires evaluating a DWARF expression to find out the
//       offset of a member. Right now we handle the common case, which is when it
//       is encoded as a constant value.
//   [ ] inline information
//   [ ] full info we are not handling:
//     [ ] friend classes
//     [ ] DWARF macro info
//     [ ] whether or not a function is the entry point
//   [ ] attributes we are not handling that may be important:
//     [ ] DW_Attrib_AbstractOrigin
//       - ???
//     [ ] DW_Attrib_VariableParameter
//       - determines whether or not a parameter to a function is mutable, I think?
//     [ ] DW_Attrib_Mutable
//       - I think this is for specific keywords, may not be relevant to C/++
//     [ ] DW_Attrib_CallColumn
//       - column position of an inlined subroutine
//     [ ] DW_Attrib_CallFile
//       - file of inlined subroutine
//     [ ] DW_Attrib_CallLine
//       - line number of inlined subroutine
//     [ ] DW_Attrib_ConstExpr
//       - ??? maybe C++ constexpr?
//     [ ] DW_Attrib_EnumClass
//       - c++ thing that's an enum with a backing type
//     [ ] DW_Attrib_LinkageName
//       - name used to do linking

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
dw_hash_from_string(String8 string)
{
  XXH64_hash_t hash64 = XXH3_64bits(string.str, string.size);
  return hash64;
}

internal DW_AttribClass
dw_pick_attrib_value_class(DW_Version ver, DW_Ext ext, DW_Language lang, DW_AttribKind attrib_kind, DW_FormKind form_kind)
{
  // NOTE(rjf): DWARF's spec specifies two mappings:
  // (DW_AttribKind) => List(DW_AttribClass)
  // (DW_FormKind)   => List(DW_AttribClass)
  //
  // This function's purpose is to find the overlapping class between an
  // DW_AttribKind and DW_FormKind.

  DW_AttribClass attrib_class = dw_attrib_class_from_attrib_kind(ver, ext, attrib_kind);
  DW_AttribClass form_class   = dw_attrib_class_from_form_kind(ver, form_kind);

  // rust compiler is busted, it writes version 5 attributes
  if(ver == DW_Version_2 && lang == DW_Language_Rust && (attrib_class == DW_AttribClass_Null || form_class == DW_AttribClass_Null))
  {
    attrib_class = dw_attrib_class_from_attrib_kind(DW_Version_5, ext, attrib_kind);
    form_class   = dw_attrib_class_from_form_kind(DW_Version_5, form_kind);
  }

  DW_AttribClass result = DW_AttribClass_Null;
  if(attrib_class != DW_AttribClass_Null && form_class != DW_AttribClass_Null)
  {
    result = DW_AttribClass_Undefined;

    for(U32 i = 0; i < 32; ++i)
    {
      U32 n = 1u << i;
      if((attrib_class & n) != 0 && (form_class & n) != 0)
      {
        result = ((DW_AttribClass) n);
        break;
      }
    }

    Assert(result != DW_AttribClass_Undefined);
  }

  return result;
}

////////////////////////////////
//~ rjf: DWARF-Specific Based Range Reads

internal U64
based_range_read(void *base, Rng1U64 range, U64 offset, U64 size, void *out)
{
  String8 data = str8((U8*)base+range.min, dim_1u64(range));
  return str8_deserial_read(data, offset, out, size, 1);
}

#define based_range_read_struct(base, range, offset, out) based_range_read(base, range, offset, sizeof(*out), out)

internal String8
based_range_read_string(void *base, Rng1U64 range, U64 offset)
{
  String8 data = str8((U8*)base+range.min, dim_1u64(range));
  String8 result = {0};
  str8_deserial_read_cstr(data, offset, &result);
  return result;
}

internal void *
based_range_ptr(void *base, Rng1U64 range, U64 offset)
{
  Assert(offset < dim_1u64(range));
  U8 *data = (U8*)base + range.min + offset;
  return data;
}

internal U64
based_range_read_uleb128(void *base, Rng1U64 range, U64 offset, U64 *out_value)
{
  U64 value      = 0;
  U64 bytes_read = 0;
  U64 shift      = 0;
  U8  byte       = 0;
  for(U64 read_offset = offset;
      based_range_read_struct(base, range, read_offset, &byte) == 1;
      read_offset += 1)
  {
    bytes_read += 1;
    U8 val = byte & 0x7fu;
    value |= ((U64)val) << shift;
    if((byte&0x80u) == 0)
    {
      break;
    }
    shift += 7u;
  }
  if(out_value != 0)
  {
    *out_value = value;
  }
  return bytes_read;
}

internal U64
based_range_read_sleb128(void *base, Rng1U64 range, U64 offset, S64 *out_value)
{
  U64 value = 0;
  U64 bytes_read = 0;
  U64 shift = 0;
  U8 byte = 0;
  for(U64 read_offset = offset;
      based_range_read_struct(base, range, read_offset, &byte) == 1;
      read_offset += 1)
  {
    bytes_read += 1;
    U8 val = byte & 0x7fu;
    value |= ((U64)val) << shift;
    shift += 7u;
    if((byte&0x80u) == 0)
    {
      if(shift < sizeof(value) * 8 && (byte & 0x40u) != 0)
      {
        value |= -(S64)(1ull << shift);
      }
      break;
    }
  }
  if(out_value != 0)
  {
    *out_value = value;
  }
  return bytes_read;
}

////////////////////////////////

internal U64
dw_based_range_read_length(void *base, Rng1U64 range, U64 offset, U64 *out_value)
{
  U64 bytes_read = 0;
  U64 value      = 0;
  U32 first32    = 0;
  if(based_range_read_struct(base, range, offset, &first32))
  {
    // NOTE(rjf): DWARF 32-bit => use the first 32 bits as the size.
    if(first32 != max_U32)
    {
      value = (U64)first32;
      bytes_read = sizeof(U32);
    }
    // NOTE(rjf): DWARF 64-bit => first 32 are just a marker, use the next 64 bits as the size.
    else if(based_range_read_struct(base, range, offset + sizeof(U32), &value))
    {
      value = 0;
      bytes_read = sizeof(U32) + sizeof(U64);
    }
  }
  if(out_value != 0)
  {
    *out_value = value;
  }
  return bytes_read;
}

internal U64
dw_based_range_read_abbrev_tag(void *base, Rng1U64 range, U64 offset, DW_Abbrev *out_abbrev)
{
  U64 total_bytes_read = 0;
  
  //- rjf: parse ID
  U64 id_off       = offset;
  U64 sub_kind_off = id_off;
  U64 id           = 0;
  {
    U64 bytes_read = based_range_read_uleb128(base, range, id_off, &id);
    sub_kind_off += bytes_read;
    total_bytes_read += bytes_read;
  }
  
  //- rjf: parse sub-kind
  U64 sub_kind = 0;
  U64 next_off = sub_kind_off;
  if(id != 0)
  {
    U64 bytes_read = based_range_read_uleb128(base, range, sub_kind_off, &sub_kind);
    next_off += bytes_read;
    total_bytes_read += bytes_read;
  }
  
  //- rjf: parse whether this tag has children
  U8 has_children = 0;
  if(id != 0)
  {
    total_bytes_read += based_range_read_struct(base, range, next_off, &has_children);
  }
  
  //- rjf: fill abbrev
  if(out_abbrev != 0)
  {
    DW_Abbrev abbrev    = {0};
    abbrev.kind         = DW_Abbrev_Tag;
    abbrev.abbrev_range = rng_1u64(range.min+offset, range.min+offset+total_bytes_read);
    abbrev.sub_kind     = sub_kind;
    abbrev.id           = id;
    if(has_children)
    {
      abbrev.flags |= DW_AbbrevFlag_HasChildren;
    }
    *out_abbrev = abbrev;
  }
  
  return total_bytes_read;
}

internal U64
dw_based_range_read_abbrev_attrib_info(void *base, Rng1U64 range, U64 offset, DW_Abbrev *out_abbrev)
{
  U64 total_bytes_read = 0;
  
  //- rjf: parse ID
  U64 id_off       = offset;
  U64 sub_kind_off = id_off;
  U64 id           = 0;
  {
    U64 bytes_read = based_range_read_uleb128(base, range, id_off, &id);
    sub_kind_off += bytes_read;
    total_bytes_read += bytes_read;
  }
  
  //- rjf: parse sub-kind (form-kind)
  U64 sub_kind = 0;
  U64 next_off = sub_kind_off;
  {
    U64 bytes_read = based_range_read_uleb128(base, range, sub_kind_off, &sub_kind);
    next_off += bytes_read;
    total_bytes_read += bytes_read;
  }
  
  //- rjf: parse implicit const
  U64 implicit_const = 0;
  if(sub_kind == DW_Form_ImplicitConst)
  {
    U64 bytes_read = based_range_read_uleb128(base, range, next_off, &implicit_const);
    total_bytes_read += bytes_read;
  }
  
  //- rjf: fill abbrev
  if(out_abbrev != 0)
  {
    DW_Abbrev abbrev    = {0};
    abbrev.kind         = DW_Abbrev_Attrib;
    abbrev.abbrev_range = rng_1u64(offset, offset+total_bytes_read);
    abbrev.sub_kind     = sub_kind;
    abbrev.id           = id;
    if(sub_kind == DW_Form_ImplicitConst)
    {
      abbrev.flags |= DW_AbbrevFlag_HasImplicitConst;
      abbrev.const_value = implicit_const;
    }
    *out_abbrev = abbrev;
  }
  
  return total_bytes_read;
}

internal U64
dw_based_range_read_attrib_form_value(void *base, Rng1U64 range, U64 offset, DW_Mode mode, U64 address_size, DW_FormKind form_kind, U64 implicit_const, DW_AttribValue *form_value_out)
{
  U64            bytes_read    = 0;
  U64            bytes_to_read = 0;
  DW_AttribValue form_value    = {0};
  
  switch(form_kind)
  {
    case DW_Form_Null: break;

    //- rjf: 1-byte uint reads
    case DW_Form_Ref1:  case DW_Form_Data1: case DW_Form_Flag:
    case DW_Form_Strx1: case DW_Form_Addrx1:
    bytes_to_read = 1; goto read_fixed_uint;
    
    //- rjf: 2-byte uint reads
    case DW_Form_Ref2: case DW_Form_Data2: case DW_Form_Strx2:
    case DW_Form_Addrx2:
    bytes_to_read = 2; goto read_fixed_uint;
    
    //- rjf: 3-byte uint reads
    case DW_Form_Strx3: case DW_Form_Addrx3:
    bytes_to_read = 3; goto read_fixed_uint;
    
    //- rjf: 4-byte uint reads
    case DW_Form_Data4: case DW_Form_Ref4: case DW_Form_RefSup4: case DW_Form_Strx4: case DW_Form_Addrx4:
    bytes_to_read = 4; goto read_fixed_uint;
    
    //- rjf: 8-byte uint reads
    case DW_Form_Data8: case DW_Form_Ref8: case DW_Form_RefSig8: case DW_Form_RefSup8:
    bytes_to_read = 8; goto read_fixed_uint;
    
    //- rjf: address-size reads
    case DW_Form_Addr:       bytes_to_read = address_size; goto read_fixed_uint;
    
    //- rjf: offset-size reads
    case DW_Form_RefAddr: case DW_Form_SecOffset: case DW_Form_LineStrp:
    case DW_Form_Strp: case DW_Form_StrpSup:
    bytes_to_read = dw_offset_size_from_mode(mode); goto read_fixed_uint;
    
    //- rjf: fixed-size uint reads
    {
      read_fixed_uint:;
      U64 value = 0;
      bytes_read = based_range_read(base, range, offset, bytes_to_read, &value);
      form_value.v[0] = value;
    } break;
    
    //- rjf: uleb128 reads
    case DW_Form_UData: case DW_Form_RefUData: case DW_Form_Strx:
    case DW_Form_Addrx: case DW_Form_LocListx:  case DW_Form_RngListx:
    {
      U64 value = 0;
      bytes_read = based_range_read_uleb128(base, range, offset, &value);
      form_value.v[0] = value;
    } break;
    
    //- rjf: sleb128 reads
    case DW_Form_SData:
    {
      S64 value = 0;
      bytes_read = based_range_read_sleb128(base, range, offset, &value);
      form_value.v[0] = value;
    } break;
    
    //- rjf: fixed-size uint read + skip
    case DW_Form_Block1: bytes_to_read = 1; goto read_fixed_uint_skip;
    case DW_Form_Block2: bytes_to_read = 2; goto read_fixed_uint_skip;
    case DW_Form_Block4: bytes_to_read = 4; goto read_fixed_uint_skip;
    {
      read_fixed_uint_skip:;
      U64 size = 0;
      bytes_read = based_range_read(base, range, offset, bytes_to_read, &size);
      form_value.v[0] = size;
      form_value.v[1] = offset;
      bytes_read += size;
    } break;
    
    //- rjf: uleb 128 read + skip
    case DW_Form_Block:
    {
      U64 size = 0;
      bytes_read = based_range_read_uleb128(base, range, offset, &size);
      form_value.v[0] = size;
      form_value.v[1] = offset;
      bytes_read += size;
    } break;
    
    //- rjf: u64 ranges
    case DW_Form_Data16:
    {
      U64 value1 = 0;
      U64 value2 = 0;
      bytes_read += based_range_read_struct(base, range, offset,               &value1);
      bytes_read += based_range_read_struct(base, range, offset + sizeof(U64), &value2);
      form_value.v[0] = value1;
      form_value.v[1] = value2;
    } break;
    
    //- rjf: strings
    case DW_Form_String:
    {
      String8 string = based_range_read_string(base, range, offset);
      bytes_read = string.size + 1;
      U64 string_offset = offset;
      U64 string_size = (offset + bytes_read) - string_offset;
      form_value.v[0] = string_offset;
      form_value.v[1] = string_offset+string_size-1;
    } break;
    
    //- rjf: implicit const
    case DW_Form_ImplicitConst:
    {
      // Special case.
      // Unlike other forms that have their values stored in the .debug_info section,
      // This one defines it's value in the .debug_abbrev section.
      form_value.v[0] = implicit_const;
    } break;
    
    //- rjf: expr loc
    case DW_Form_ExprLoc:
    {
      U64 size = 0;
      bytes_read = based_range_read_uleb128(base, range, offset, &size);
      form_value.v[0] = offset + bytes_read;
      form_value.v[1] = size;
      bytes_read += size;
    } break;
    
    //- rjf: flag present
    case DW_Form_FlagPresent:
    {
      form_value.v[0] = 1;
    } break;
    
    case DW_Form_Indirect:
    {
      InvalidPath;
    } break;
  }
  
  if(form_value_out != 0)
  {
    *form_value_out = form_value;
  }
  
  return bytes_read;
}

//- rjf: important DWARF section base/range accessors

internal DW_Mode
dw_mode_from_sec(DW_SectionArray *sections, DW_SectionKind kind)
{
  if(sections->v[kind].data.size > 0xffffffff)
  {
    return DW_Mode_64Bit;
  }
  else
  {
    return DW_Mode_32Bit;
  }
}

internal Rng1U64
dw_range_from_sec(DW_SectionArray *sections, DW_SectionKind kind)
{
  Rng1U64 result = rng_1u64(0, sections->v[kind].data.size);
  return result;
}

internal void *
dw_base_from_sec(DW_SectionArray *sections, DW_SectionKind kind)
{
  return sections->v[kind].data.str;
}

////////////////////////////////
//~ rjf: Abbrev Table

internal DW_AbbrevTable
dw_make_abbrev_table(Arena *arena, DW_SectionArray *sections, U64 abbrev_offset)
{
  void    *file_base    = dw_base_from_sec(sections, DW_Section_Abbrev);
  Rng1U64  abbrev_range = dw_range_from_sec(sections, DW_Section_Abbrev);
  
  //- rjf: count the tags we have
  U64 tag_count = 0;
  for(U64 abbrev_read_off = abbrev_offset - abbrev_range.min;;)
  {
    DW_Abbrev tag;
    {
      U64 bytes_read = dw_based_range_read_abbrev_tag(file_base, abbrev_range, abbrev_read_off, &tag);
      abbrev_read_off += bytes_read;
      if(bytes_read == 0 || tag.id == 0)
      {
        break;
      }
    }
    for(;;)
    {
      DW_Abbrev attrib     = {0};
      U64       bytes_read = dw_based_range_read_abbrev_attrib_info(file_base, abbrev_range, abbrev_read_off, &attrib);
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
  for(U64 abbrev_read_off = abbrev_offset - abbrev_range.min;;)
  {
    DW_Abbrev tag;
    {
      U64 bytes_read = dw_based_range_read_abbrev_tag(file_base, abbrev_range, abbrev_read_off, &tag);
      abbrev_read_off += bytes_read;
      if(bytes_read == 0 || tag.id == 0)
      {
        break;
      }
    }
    
    // rjf: insert this tag into the table
    {
      table.entries[tag_idx].id  = tag.id;
      table.entries[tag_idx].off = tag.abbrev_range.min;
      tag_idx += 1;
    }
    
    for(;;)
    {
      DW_Abbrev attrib = {0};
      U64 bytes_read = dw_based_range_read_abbrev_attrib_info(file_base, abbrev_range, abbrev_read_off, &attrib);
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
  if(table.count > 0)
  {
    S64 min = 0;
    S64 max = (S64)table.count - 1;
    while(min <= max)
    {
      S64 mid = (min + max) / 2;
      if (abbrev_id > table.entries[mid].id)
      {
        min = mid + 1;
      }
      else if (abbrev_id < table.entries[mid].id)
      {
        max = mid - 1;
      }
      else
      {
        abbrev_offset = table.entries[mid].off;
        break;
      }
    }
  }
  return abbrev_offset;
}

////////////////////////////////
//~ rjf: Miscellaneous DWARF Section Parsing

//- rjf: .debug_ranges (DWARF V4)

internal Rng1U64List
dw_v4_range_list_from_range_offset(Arena *arena, DW_SectionArray *sections, U64 addr_size, U64 comp_unit_base_addr, U64 range_off)
{
  void    *base = dw_base_from_sec(sections, DW_Section_Ranges);
  Rng1U64  rng  = dw_range_from_sec(sections, DW_Section_Ranges);
  
  Rng1U64List list = {0};
  
  U64 read_off = range_off;
  U64 base_addr = comp_unit_base_addr;
  
  for(;read_off < rng.max;)
  {
    U64 v0 = 0;
    U64 v1 = 0;
    read_off += based_range_read(base, rng, read_off, addr_size, &v0);
    read_off += based_range_read(base, rng, read_off, addr_size, &v1);
    
    //- rjf: base address entry
    if((addr_size == 4 && v0 == 0xffffffff) ||
       (addr_size == 8 && v0 == 0xffffffffffffffff))
    {
      base_addr = v1;
    }
    //- rjf: end-of-list entry
    else if(v0 == 0 && v1 == 0)
    {
      break;
    }
    //- rjf: range list entry
    else
    {
      U64 min_addr = v0 + base_addr;
      U64 max_addr = v1 + base_addr;
      rng1u64_list_push(arena, &list, rng_1u64(min_addr, max_addr));
    }
  }
  
  return list;
}

//- rjf: .debug_pubtypes + .debug_pubnames (DWARF V4)

internal DW_PubStringsTable
dw_v4_pub_strings_table_from_section_kind(Arena *arena, DW_SectionArray *sections, DW_SectionKind section_kind)
{
  Temp scratch = scratch_begin(&arena, 1);

  DW_PubStringsTable names_table = {0};
  
  // TODO(rjf): Arbitrary choice.
  names_table.size    = 16384;
  names_table.buckets = push_array(arena, DW_PubStringsBucket*, names_table.size);
  
  void    *base     = dw_base_from_sec(sections, section_kind);
  Rng1U64  rng      = dw_range_from_sec(sections, section_kind);
  DW_Mode  mode     = sections->v[section_kind].mode;
  U64      off_size = dw_offset_size_from_mode(mode);
  U64      cursor   = 0;
  
  U64 table_length = 0;
  U16 unit_version = 0;
  U64 cu_info_off  = 0;
  U64 cu_info_len  = 0;
  cursor += dw_based_range_read_length(base, rng, cursor, &table_length);
  cursor += based_range_read_struct(base, rng, cursor, &unit_version);
  cursor += based_range_read(base, rng, cursor, off_size, &cu_info_off);
  cursor += dw_based_range_read_length(base, rng, cursor, &cu_info_len);
  
  for(;;)
  {
    U64 info_off = 0;
    {
      U64 bytes_read = based_range_read(base, rng, cursor, off_size, &info_off);
      cursor += bytes_read;
      if(bytes_read == 0)
      {
        break;
      }
    }
    
    //- rjf: if we got a nonzero .debug_info offset, we've found a valid entry.
    if(info_off != 0)
    {
      String8 string = based_range_read_string(base, rng, cursor);
      cursor += string.size + 1;

      U64 hash       = dw_hash_from_string(string);
      U64 bucket_idx = hash % names_table.size;
      
      DW_PubStringsBucket *bucket = push_array(arena, DW_PubStringsBucket, 1);
      bucket->next                = names_table.buckets[bucket_idx];
      bucket->string              = string;
      bucket->info_off            = info_off;
      bucket->cu_info_off         = cu_info_off;
      names_table.buckets[bucket_idx] = bucket;
    }
    
    //- rjf: if we did not read a proper entry in the table, we need to try to
    // read the header of the next table.
    else
    {
      U64 next_table_length = 0;
      {
        U64 bytes_read = dw_based_range_read_length(base, rng, cursor, &next_table_length);
        if(bytes_read == 0 || next_table_length == 0)
        {
          break;
        }
        cursor += bytes_read;
      }
      cursor += based_range_read_struct(base, rng, cursor, &unit_version);
      cursor += based_range_read(base, rng, cursor, off_size, &cu_info_off);
      cursor += dw_based_range_read_length(base, rng, cursor, &cu_info_len);
    }
  }
  
  scratch_end(scratch);
  
  return names_table;
}

//- rjf: .debug_str_offsets (DWARF V5)

internal U64
dw_v5_offset_from_offs_section_base_index(DW_SectionArray *sections, DW_SectionKind section, U64 base, U64 index)
{
  U64 result = 0;
  
  DW_Mode  mode     = sections->v[section].mode;
  void    *sec_base = dw_base_from_sec(sections, section);
  Rng1U64  rng      = dw_range_from_sec(sections, section);
  U64      cursor   = base;
  
  //- rjf: get the length of each entry
  U64 entry_len = mode == DW_Mode_64Bit ? 8 : 4;
  
  //- rjf: parse the unit's length (not including the length itself)
  U64 unit_length = 0;
  cursor += dw_based_range_read_length(sec_base, rng, cursor, &unit_length);
  
  //- rjf: parse version
  U16 version = 0;
  cursor += based_range_read_struct(sec_base, rng, cursor, &version);
  Assert(version == 5); // must be 5 as of V5.
  
  //- rjf: parse padding
  U16 padding = 0;
  cursor += based_range_read_struct(sec_base, rng, cursor, &padding);
  Assert(padding == 0); // must be 0 as of V5.
  
  //- rjf: read
  if (unit_length >= sizeof(U16)*2) 
  {
    void *entries = (U8 *)sec_base + cursor;
    U64 count = (unit_length - sizeof(U16)*2) / entry_len;
    if(0 <= index && index < count)
    {
      switch(entry_len)
      {
        default: break;
        case 4: result = ((U32 *)entries)[index]; break;
        case 8: result = ((U64 *)entries)[index]; break;
      }
    }
  }
  
  return result;
}

//- rjf: .debug_addr parsing

internal U64
dw_v5_addr_from_addrs_section_base_index(DW_SectionArray *sections, DW_SectionKind section, U64 base, U64 index)
{
  U64 result = 0;
  
  void    *sec_base = dw_base_from_sec(sections, section);
  Rng1U64  rng      = dw_range_from_sec(sections, section);
  U64      cursor   = base;
  
  //- rjf: parse the unit's length (not including the length itself)
  U64 unit_length = 0;
  cursor += dw_based_range_read_length(sec_base, rng, cursor, &unit_length);
  
  //- rjf: parse version
  U16 version = 0;
  cursor += based_range_read_struct(sec_base, rng, cursor, &version);
  Assert(version == 5); // must be 5 as of V5.
  
  //- rjf: parse address size
  U8 address_size = 0;
  cursor += based_range_read_struct(sec_base, rng, cursor, &address_size);
  
  //- rjf: parse segment selector size
  U8 segment_selector_size = 0;
  cursor += based_range_read_struct(sec_base, rng, cursor, &segment_selector_size);
  
  //- rjf: read
  U64 entry_size = address_size + segment_selector_size;
  U64 count = (unit_length - sizeof(U16)*2) / entry_size;
  if(0 <= index && index < count)
  {
    void    *entry     = (U8 *)based_range_ptr(sec_base, rng, cursor) + entry_size*index;
    Rng1U64  entry_rng = rng_1u64(0, entry_size);
    U64      segment   = 0;
    U64      addr      = 0;
    based_range_read(entry, entry_rng, 0,                     sizeof(segment), &segment);
    based_range_read(entry, entry_rng, segment_selector_size, sizeof(addr),    &addr);
    result = addr;
  }
  
  return result;
}

//- rjf: .debug_rnglists + .debug_loclists parsing

internal U64
dw_v5_sec_offset_from_rnglist_or_loclist_section_base_index(DW_SectionArray *sections, DW_SectionKind section_kind, U64 base, U64 index)
{
  //
  // NOTE(rjf): This is only appropriate to call when DW_Form_RngListx is
  // used to access a range list, *OR* when DW_Form_LocListx is used to
  // access a location list. Otherwise, DW_Form_SecOffset is required.
  //
  // See the DWARF V5 spec (February 13, 2017), page 242. (rnglists)
  // See the DWARF V5 spec (February 13, 2017), page 215. (loclists)
  //
  
  U64 result = 0;

  DW_Mode  mode     = sections->v[section_kind].mode;
  void    *sec_base = dw_base_from_sec(sections, section_kind);
  Rng1U64  rng      = dw_range_from_sec(sections, section_kind);
  U64      cursor   = base;
  
  //- rjf: get the length of each entry
  U64 entry_len = mode == DW_Mode_64Bit ? 8 : 4;
  
  //- rjf: parse the unit's length (not including the length itself)
  U64 unit_length = 0;
  cursor += dw_based_range_read_length(sec_base, rng, cursor, &unit_length);
  
  //- rjf: parse version
  U16 version = 0;
  cursor += based_range_read_struct(sec_base, rng, cursor, &version);
  Assert(version == 5); // must be 5 as of V5.
  
  //- rjf: parse address size
  U8 address_size = 0;
  cursor += based_range_read_struct(sec_base, rng, cursor, &address_size);
  
  //- rjf: parse segment selector size
  U8 segment_selector_size = 0;
  cursor += based_range_read_struct(sec_base, rng, cursor, &segment_selector_size);
  
  //- rjf: parse offset entry count
  U32 offset_entry_count = 0;
  cursor += based_range_read_struct(sec_base, rng, cursor, &offset_entry_count);
  
  //- rjf: read from offsets array
  U64 table_off = cursor;
  void *offsets_arr = based_range_ptr(sec_base, rng, cursor);
  if(0 <= index && index < (U64)offset_entry_count)
  {
    U64 rnglist_offset = 0;
    switch(entry_len)
    {
      default: break;
      case 4: rnglist_offset = ((U32 *)offsets_arr)[index]; break;
      case 8: rnglist_offset = ((U64 *)offsets_arr)[index]; break;
    }
    result = rnglist_offset+table_off;
  }
  
  return result;
}

internal Rng1U64List
dw_v5_range_list_from_rnglist_offset(Arena *arena, DW_SectionArray *sections, DW_SectionKind section, U64 addr_size, U64 addr_section_base, U64 offset)
{
  Rng1U64List list = {0};
  
  void    *base   = dw_base_from_sec(sections, section);
  Rng1U64  rng    = dw_range_from_sec(sections, section);
  U64      cursor = offset;
  
  U64 base_addr = 0;
  
  for(B32 done = 0; !done;)
  {
    U8 kind8 = 0;
    cursor += based_range_read_struct(base, rng, cursor, &kind8);
    DW_RngListEntryKind kind = (DW_RngListEntryKind)kind8;
    
    switch(kind)
    {
      //- rjf: can be used in split and non-split units:
      default:
      case DW_RngListEntryKind_EndOfList:
      {
        done = 1;
      } break;
      
      case DW_RngListEntryKind_BaseAddressX:
      {
        U64 base_addr_idx = 0;
        cursor += based_range_read_uleb128(base, rng, cursor, &base_addr_idx);
        base_addr = dw_v5_addr_from_addrs_section_base_index(sections, DW_Section_Addr, addr_section_base, base_addr_idx);
      } break;
      
      case DW_RngListEntryKind_StartxEndx:
      {
        U64 start_addr_idx = 0;
        U64 end_addr_idx   = 0;
        cursor += based_range_read_uleb128(base, rng, cursor, &start_addr_idx);
        cursor += based_range_read_uleb128(base, rng, cursor, &end_addr_idx);
        U64 start_addr = dw_v5_addr_from_addrs_section_base_index(sections, DW_Section_Addr, addr_section_base, start_addr_idx);
        U64 end_addr   = dw_v5_addr_from_addrs_section_base_index(sections, DW_Section_Addr, addr_section_base, end_addr_idx);
        rng1u64_list_push(arena, &list, rng_1u64(start_addr, end_addr));
      } break;
      
      case DW_RngListEntryKind_StartxLength:
      {
        U64 start_addr_idx = 0;
        U64 length         = 0;
        cursor += based_range_read_uleb128(base, rng, cursor, &start_addr_idx);
        cursor += based_range_read_uleb128(base, rng, cursor, &length);
        U64 start_addr = dw_v5_addr_from_addrs_section_base_index(sections, DW_Section_Addr, addr_section_base, start_addr_idx);
        U64 end_addr   = start_addr + length;
        rng1u64_list_push(arena, &list, rng_1u64(start_addr, end_addr));
      } break;
      
      case DW_RngListEntryKind_OffsetPair:
      {
        U64 start_offset = 0;
        U64 end_offset   = 0;
        cursor += based_range_read_uleb128(base, rng, cursor, &start_offset);
        cursor += based_range_read_uleb128(base, rng, cursor, &end_offset);
        rng1u64_list_push(arena, &list, rng_1u64(start_offset + base_addr, end_offset + base_addr));
      } break;
      
      //- rjf: non-split units only:
      
      case DW_RngListEntryKind_BaseAddress:
      {
        U64 new_base_addr = 0;
        cursor += based_range_read(base, rng, cursor, addr_size, &new_base_addr);
        base_addr = new_base_addr;
      } break;
      
      case DW_RngListEntryKind_StartEnd:
      {
        U64 start = 0;
        U64 end   = 0;
        cursor += based_range_read(base, rng, cursor, addr_size, &start);
        cursor += based_range_read(base, rng, cursor, addr_size, &end);
        rng1u64_list_push(arena, &list, rng_1u64(start, end));
      } break;
      
      case DW_RngListEntryKind_StartLength:
      {
        U64 start  = 0;
        U64 length = 0;
        cursor += based_range_read(base, rng, cursor, addr_size, &start);
        cursor += based_range_read_uleb128(base, rng, cursor, &length);
        rng1u64_list_push(arena, &list, rng_1u64(start, start+length));
      } break;
    }
  }
  
  return list;
}

////////////////////////////////
//~ rjf: Attrib Value Parsing

internal DW_AttribValueResolveParams
dw_attrib_value_resolve_params_from_comp_root(DW_CompRoot *root)
{
  DW_AttribValueResolveParams params = {0};
  params.version                     = root->version;
  params.language                    = root->language;
  params.addr_size                   = root->address_size;
  params.containing_unit_info_off    = root->info_off;
  params.debug_addrs_base            = root->addrs_base;
  params.debug_rnglists_base         = root->rnglist_base;
  params.debug_str_offs_base         = root->stroffs_base;
  params.debug_loclists_base         = root->loclist_base;
  return params;
}

internal DW_AttribValue
dw_attrib_value_from_form_value(DW_SectionArray             *sections,
                                DW_AttribValueResolveParams  resolve_params,
                                DW_FormKind                  form_kind,
                                DW_AttribClass               value_class,
                                DW_AttribValue               form_value)
{
  DW_AttribValue value = {0};
  
  //~ rjf: DWARF V5 value parsing
  
  //- rjf: (DWARF V5 ONLY) the form value is storing an address index (ADDRess indeX), which we
  // must resolve to an actual address using the containing comp unit's contribution to the
  // .debug_addr section.
  if(resolve_params.version >= DW_Version_5 &&
     value_class == DW_AttribClass_Address &&
     (form_kind == DW_Form_Addrx  || form_kind == DW_Form_Addrx1 ||
      form_kind == DW_Form_Addrx2 || form_kind == DW_Form_Addrx3 ||
      form_kind == DW_Form_Addrx4))
  {
    U64 addr_index = form_value.v[0];
    U64 addr = dw_v5_addr_from_addrs_section_base_index(sections, DW_Section_Addr, resolve_params.debug_addrs_base, addr_index);
    value.v[0] = addr;
  }
  //- rjf: (DWARF V5 ONLY) lookup into the .debug_loclists section via an index
  else if(resolve_params.version >= DW_Version_5 &&
          value_class == DW_AttribClass_LocList &&
          form_kind == DW_Form_LocListx)
  {
    U64 loclist_index  = form_value.v[0];
    U64 loclist_offset = dw_v5_sec_offset_from_rnglist_or_loclist_section_base_index(sections, DW_Section_LocLists, resolve_params.debug_loclists_base, loclist_index);
    value.section = DW_Section_LocLists;
    value.v[0]    = loclist_offset;
  }
  //- rjf: (DWARF V5 ONLY) lookup into the .debug_loclists section via an offset
  else if(resolve_params.version >= DW_Version_5 &&
          (value_class == DW_AttribClass_LocList || value_class == DW_AttribClass_LocListPtr) &&
          form_kind == DW_Form_SecOffset)
  {
    U64 loclist_offset = form_value.v[0];
    value.section = DW_Section_LocLists;
    value.v[0]    = loclist_offset;
  }
  //- rjf: (DWARF V5 ONLY) lookup into the .debug_rnglists section via an index
  else if(resolve_params.version >= DW_Version_5 &&
          (value_class == DW_AttribClass_RngListPtr || value_class == DW_AttribClass_RngList) &&
          form_kind == DW_Form_RngListx)
  {
    U64 rnglist_index  = form_value.v[0];
    U64 rnglist_offset = dw_v5_sec_offset_from_rnglist_or_loclist_section_base_index(sections, DW_Section_RngLists, resolve_params.debug_rnglists_base, rnglist_index);
    value.section = DW_Section_RngLists;
    value.v[0]    = rnglist_offset;
  }
  //- rjf: (DWARF V5 ONLY) lookup into the .debug_rnglists section via an offset
  else if(resolve_params.version >= DW_Version_5 &&
          (value_class == DW_AttribClass_RngListPtr || value_class == DW_AttribClass_RngList) &&
          form_kind != DW_Form_RngListx)
  {
    U64 rnglist_offset = form_value.v[0];
    value.section = DW_Section_RngLists;
    value.v[0]    = rnglist_offset;
  }
  //- rjf: (DWARF V5 ONLY) .debug_str_offsets table index, that we need to resolve
  // using the containing compilation unit's contribution to the section
  else if(resolve_params.version >= DW_Version_5 &&
          value_class == DW_AttribClass_String && 
          (form_kind == DW_Form_Strx ||
           form_kind == DW_Form_Strx1 ||
           form_kind == DW_Form_Strx2 ||
           form_kind == DW_Form_Strx3 ||
           form_kind == DW_Form_Strx4))
  {
    DW_SectionKind  section    = DW_Section_Str;
    U64             str_index  = form_value.v[0];
    U64             str_offset = dw_v5_offset_from_offs_section_base_index(sections, DW_Section_StrOffsets, resolve_params.debug_str_offs_base, str_index);
    void           *base       = dw_base_from_sec(sections, section);
    Rng1U64         range      = dw_range_from_sec(sections, section);
    String8         string     = based_range_read_string(base, range, str_offset);
    value.section = section;
    value.v[0]    = str_offset;
    value.v[1]    = value.v[0] + string.size;
  }
  //- rjf: (DWARF V5 ONLY) reference that we should resolve through ref_addr_desc
  else if(resolve_params.version >= DW_Version_5 &&
          value_class == DW_AttribClass_Reference &&
          form_kind == DW_Form_RefAddr)
  {
    // TODO(nick): DWARF 5 @dwarf_v5
  }
  //- TODO(rjf): (DWARF V5 ONLY) reference resolution using the .debug_names section
  else if(resolve_params.version >= DW_Version_5 &&
          form_kind == DW_Form_RefSig8)
  {
    // TODO(nick): DWARF 5: We need to handle .debug_names section in order to resolve this value. @dwarf_v5
    value.v[0] = max_U64;
  }
  
  //~ rjf: All other value parsing (DWARF V4 and below)
  
  //- rjf: reference to an offset relative to the compilation unit's info base
  else if (value_class == DW_AttribClass_Reference &&
           (form_kind == DW_Form_Ref1 ||
            form_kind == DW_Form_Ref2 ||
            form_kind == DW_Form_Ref4 ||
            form_kind == DW_Form_Ref8 ||
            form_kind == DW_Form_RefUData))
  {
    value.v[0] = resolve_params.containing_unit_info_off + form_value.v[0];
  }
  
  //- rjf: info-section string -- this is a string that is just pasted straight
  // into the .debug_info section
  else if(value_class == DW_AttribClass_String && form_kind == DW_Form_String)
  {
    value         = form_value;
    value.section = DW_Section_Info;
  }
  
  //- rjf: string-section string -- this is a string that's inside the .debug_str
  // section, and we've been provided an offset to it
  else if(value_class == DW_AttribClass_String && 
          (form_kind == DW_Form_Strp ||
           form_kind == DW_Form_StrpSup))
  {

    DW_SectionKind  section = DW_Section_Str;
    void           *base    = dw_base_from_sec(sections, section);
    Rng1U64         range   = dw_range_from_sec(sections, section);
    String8         string  = based_range_read_string(base, range, form_value.v[0]);
    value.section = section;
    value.v[0]    = form_value.v[0];
    value.v[1]    = value.v[0] + string.size;
  }
  //- rjf: line-string
  else if(value_class == DW_AttribClass_String && form_kind == DW_Form_LineStrp)
  {
    DW_SectionKind  section = DW_Section_LineStr;
    void           *base    = dw_base_from_sec(sections, section);
    Rng1U64         range   = dw_range_from_sec(sections, section);
    String8         string  = based_range_read_string(base, range, form_value.v[0]);
    value.section = section;
    value.v[0]    = form_value.v[0];
    value.v[1]    = value.v[0] + string.size;
  }
  //- rjf: .debug_ranges
  else if(resolve_params.version < DW_Version_5 &&
          (value_class == DW_AttribClass_RngListPtr || value_class == DW_AttribClass_RngList) &&
          (form_kind == DW_Form_SecOffset))
  {
    U64 ranges_offset = form_value.v[0];
    value.section = DW_Section_Ranges;
    value.v[0]    = ranges_offset;
  }
  //- rjf: .debug_loc
  else if(resolve_params.version < DW_Version_5 &&
          (value_class == DW_AttribClass_LocListPtr || value_class == DW_AttribClass_LocList) &&
          (form_kind == DW_Form_SecOffset))
  {
    U64 offset = form_value.v[0];
    value.section = DW_Section_Loc;
    value.v[0]    = offset;
  }
  //- rjf: invalid attribute class
  else if(value_class == 0)
  {
    Assert(!"attribute class was not resolved");
  }
  //- rjf: in all other cases, we can accept the form_value as the correct
  // representation for the parsed value, so we can just copy it over.
  else
  {
    value = form_value;
  }
  
  return value;
}

internal String8
dw_string_from_attrib_value(DW_SectionArray *sections, DW_AttribValue value)
{
  DW_SectionKind  section_kind = value.section;
  void           *base         = dw_base_from_sec(sections, section_kind);
  Rng1U64         range        = dw_range_from_sec(sections, section_kind);

  String8 string = {0};
  string.str     = (U8 *)based_range_ptr(base, range, value.v[0]);
  string.size    = value.v[1] - value.v[0];
  return string;
}

internal Rng1U64List
dw_range_list_from_high_low_pc_and_ranges_attrib_value(Arena *arena, DW_SectionArray *sections, U64 address_size, U64 comp_unit_base_addr, U64 addr_section_base, U64 low_pc, U64 high_pc, DW_AttribValue ranges_value)
{
  Rng1U64List list = {0};
  switch(ranges_value.section)
  {
    //- rjf: (DWARF V5 ONLY) .debug_rnglists offset
    case DW_Section_RngLists:
    {
      list = dw_v5_range_list_from_rnglist_offset(arena, sections, ranges_value.section, address_size, addr_section_base, ranges_value.v[0]);
    } break;
    
    //- rjf: (DWARF V4 and earlier) .debug_ranges parsing
    case DW_Section_Ranges:
    {
      list = dw_v4_range_list_from_range_offset(arena, sections, address_size, comp_unit_base_addr, ranges_value.v[0]);
    } break;
    
    //- rjf: fall back to trying to use low/high PCs
    default:
    {
      rng1u64_list_push(arena, &list, rng_1u64(low_pc, high_pc));
    } break;
  }
  return list;
}

////////////////////////////////
//~ rjf: Tag Parsing

internal DW_AttribListParseResult
dw_parse_attrib_list_from_info_abbrev_offsets(Arena           *arena,
                                              DW_SectionArray *sections,
                                              DW_Version       ver,
                                              DW_Ext           ext,
                                              DW_Language      lang,
                                              U64              address_size,
                                              U64              info_off,
                                              U64              abbrev_off)
{
  //- rjf: set up prereqs
  DW_Mode  info_mode    = sections->v[DW_Section_Info].mode;
  DW_Mode  abbrev_mode  = sections->v[DW_Section_Abbrev].mode;
  void    *info_base    = dw_base_from_sec(sections, DW_Section_Info);
  void    *abbrev_base  = dw_base_from_sec(sections, DW_Section_Abbrev);
  Rng1U64  info_range   = dw_range_from_sec(sections, DW_Section_Info);
  Rng1U64  abbrev_range = dw_range_from_sec(sections, DW_Section_Abbrev);
  
  //- rjf: set up read offsets
  U64 info_read_off   = info_off;
  U64 abbrev_read_off = abbrev_off;
  
  //- rjf: parse all attributes 
  DW_AttribListParseResult result = {0};
  for(B32 good_abbrev = 1; good_abbrev;)
  {
    U64 attrib_info_offset = info_read_off;
    
    //- rjf: parse abbrev attrib info
    DW_Abbrev abbrev = {0};
    {
      U64 bytes_read = dw_based_range_read_abbrev_attrib_info(abbrev_base, abbrev_range, abbrev_read_off, &abbrev);
      abbrev_read_off += bytes_read;
      good_abbrev = abbrev.id != 0;
    }
    
    //- rjf: extract attrib info from abbrev
    DW_AttribKind  attrib_kind  = (DW_AttribKind)abbrev.id;
    DW_FormKind    form_kind    = (DW_FormKind)abbrev.sub_kind;
    DW_AttribClass attrib_class = dw_pick_attrib_value_class(ver, ext, lang, attrib_kind, form_kind);
    
    //- rjf: parse the form value from the file
    DW_AttribValue form_value = {0};
    if(good_abbrev)
    {
      // NOTE(nick): This is a special case form. Basically it let's user to
      // define attribute form in the .debug_info.
      if(form_kind == DW_Form_Indirect)
      {
        U64 override_form_kind = 0;
        info_read_off += based_range_read_uleb128(info_base, info_range, info_read_off, &override_form_kind);
        form_kind = (DW_FormKind)override_form_kind;
      }
      U64 bytes_read = dw_based_range_read_attrib_form_value(info_base, info_range, info_read_off, info_mode, address_size,
                                                                       form_kind, abbrev.const_value, &form_value);
      info_read_off += bytes_read;
    }
    
    //- rjf: push this parsed attrib to the list
    if(good_abbrev)
    {
      DW_AttribNode *node      = push_array(arena, DW_AttribNode, 1);
      node->attrib.info_off    = attrib_info_offset;
      node->attrib.abbrev_id   = abbrev.id;
      node->attrib.attrib_kind = attrib_kind;
      node->attrib.form_kind   = form_kind;
      node->attrib.value_class = attrib_class;
      node->attrib.form_value  = form_value;
      result.attribs.count += 1;
      SLLQueuePush(result.attribs.first, result.attribs.last, node);
    }
  }
  
  result.max_info_off   = info_read_off;
  result.max_abbrev_off = abbrev_read_off;
  return result;
}

internal DW_Tag *
dw_tag_from_info_offset(Arena           *arena,
                        DW_SectionArray *sections,
                        DW_AbbrevTable   abbrev_table,
                        DW_Version       ver,
                        DW_Ext           ext,
                        DW_Language      lang,
                        U64              address_size,
                        U64              info_offset)
{
  void    *info_base    = dw_base_from_sec(sections, DW_Section_Info);
  Rng1U64  info_range   = dw_range_from_sec(sections, DW_Section_Info);
  void    *abbrev_base  = dw_base_from_sec(sections, DW_Section_Abbrev);
  Rng1U64  abbrev_range = dw_range_from_sec(sections, DW_Section_Abbrev);
  
  DW_Tag *tag = push_array(arena, DW_Tag, 1);
  
  //- rjf: calculate .debug_info read cursor, relative to info range minimum
  U64 info_read_off = info_offset - info_range.min;
  
  //- rjf: read abbrev ID
  U64 abbrev_id = 0;
  info_read_off += based_range_read_uleb128(info_base, info_range, info_read_off, &abbrev_id);
  B32 good_abbrev_id = abbrev_id != 0;
  
  //- rjf: figure out abbrev offset for this ID
  U64 abbrev_offset = 0;
  if(good_abbrev_id)
  {
    abbrev_offset = dw_abbrev_offset_from_abbrev_id(abbrev_table, abbrev_id);
  }
  
  //- rjf: calculate .debug_abbrev read cursor, relative to abbrev range minimum
  U64 abbrev_read_off = abbrev_offset - abbrev_range.min;
  
  //- rjf: parse abbrev tag info
  DW_Abbrev abbrev_tag_info = {0};
  B32       good_tag_abbrev = 0;
  if(good_abbrev_id)
  {
    abbrev_read_off += dw_based_range_read_abbrev_tag(abbrev_base, abbrev_range, abbrev_read_off, &abbrev_tag_info);
    good_tag_abbrev = 1;//abbrev_tag_info.id != 0;
  }
  
  //- rjf: parse all attributes for this tag
  U64           attribs_info_off   = 0;
  U64           attribs_abbrev_off = 0;
  DW_AttribList attribs            = {0};
  if(good_tag_abbrev)
  {
    DW_AttribListParseResult attribs_parse = dw_parse_attrib_list_from_info_abbrev_offsets(arena, sections, ver, ext, lang, address_size, info_read_off, abbrev_read_off);
    attribs_info_off   = info_read_off;
    attribs_abbrev_off = abbrev_read_off;
    info_read_off      = attribs_parse.max_info_off;
    abbrev_read_off    = attribs_parse.max_abbrev_off;
    attribs            = attribs_parse.attribs;
  }
  
  //- rjf: fill tag
  {
    tag->abbrev_id          = abbrev_id;
    tag->info_range         = rng_1u64(info_offset, info_range.min + info_read_off);
    tag->abbrev_range       = rng_1u64(abbrev_offset, abbrev_range.min + abbrev_read_off);
    tag->has_children       = !!(abbrev_tag_info.flags & DW_AbbrevFlag_HasChildren);
    tag->kind               = (DW_TagKind)abbrev_tag_info.sub_kind;
    tag->attribs_info_off   = attribs_info_off;
    tag->attribs_abbrev_off = attribs_abbrev_off;
    tag->attribs            = attribs;
  }
  
  return tag;
}

////////////////////////////////

internal U64
dw_v5_header_offset_from_table_offset(DW_SectionArray *sections, DW_SectionKind section, U64 table_off)
{
  // NOTE(rjf): From the DWARF V5 spec (February 13, 2017), page 401:
  //
  // "
  // Each skeleton compilation unit also has a DW_AT_addr_base attribute,
  // which provides the relocated offset to that compilation unit’s
  // contribution in the executable’s .debug_addr section. Unlike the
  // DW_AT_stmt_list attribute, the offset refers to the first address table
  // slot, not to the section header. In this example, we see that the first
  // address (slot 0) from demo1.o begins at offset 48. Because the
  // .debug_addr section contains an 8-byte header, the object file’s
  // contribution to the section actually begins at offset 40 (for a 64-bit
  // DWARF object, the header would be 16 bytes long, and the value for the
  // DW_AT_addr_base attribute would then be 56). All attributes in demo1.dwo
  // that use DW_FORM_addrx, DW_FORM_addrx1, DW_FORM_addrx2, DW_FORM_addrx3
  // or DW_FORM_addrx4 would then refer to address table slots relative to
  // that offset. Likewise, the .debug_addr contribution from demo2.dwo begins
  // at offset 72, and its first address slot is at offset 80. Because these
  // contributions have been processed by the linker, they contain relocated
  // values for the addresses in the program that are referred to by the
  // debug information.
  // "
  //
  // This seems to at least partially explain why the addr_base is showing up
  // 8 bytes later than we are expecting it to. We can't actually just store
  // the base that we read from the DW_Attrib_AddrBase attrib, because
  // it's showing up *after* the header, so we need to bump it back.
  
  // NOTE(rjf): From the DWARF V5 spec (February 13, 2017), page 66:
  //
  // "
  // A DW_AT_rnglists_base attribute, whose value is of class rnglistsptr. This
  // attribute points to the beginning of the offsets table (immediately
  // following the header) of the compilation unit's contribution to the
  // .debug_rnglists section. References to range lists (using DW_FORM_rnglistx)
  // within the compilation unit are interpreted relative to this base.
  // "
  //
  // Similarly, we need to figure out where to go to parse the header.
  
  U64 max_header_size = 0;
  U64 min_header_size = 0;
  switch(section)
  {
    default:
    case DW_Section_Addr:
    {
      max_header_size = 16;
      min_header_size = 8;
    } break;
    case DW_Section_StrOffsets:
    {
      max_header_size = 16;
      min_header_size = 8;
    } break;
    case DW_Section_RngLists:
    {
      max_header_size = 20;
      min_header_size = 12;
    } break;
    case DW_Section_LocLists:
    {
      // TODO(rjf)
        NotImplemented;
    } break;
  }
  
  U64      past_header = table_off;
  void    *addr_base   = dw_base_from_sec(sections, section);
  Rng1U64  addr_rng    = dw_range_from_sec(sections, section);
  
  //- rjf: figure out which sized header we have
  U64 header_size = 0;
  {
    // rjf: try max header, and if it works, the header is the max size, otherwise we will
    // need to rely on the min header size
    U32 first32 = 0;
    based_range_read_struct(addr_base, addr_rng, past_header-max_header_size, &first32);
    if(first32 == max_U32)
    {
      header_size = max_header_size;
    }
    else
    {
      header_size = min_header_size;
    }
  }
  
  return table_off - header_size;
}

internal Rng1U64List
dw_comp_unit_ranges_from_info(Arena *arena, DW_Section info)
{
  Rng1U64List  result = {0};
  void        *base   = info.data.str;
  Rng1U64      range  = rng_1u64(0, info.data.size);
  for(U64 cursor = 0; cursor < info.data.size; )
  {
    // read unit length
    U64 unit_length = 0;
    U64 bytes_read = dw_based_range_read_length(base, range, cursor, &unit_length);

    // was read ok?
    if(bytes_read == 0)
    {
      break;
    }

    // push unit range
    rng1u64_list_push(arena, &result, rng_1u64(cursor, cursor+unit_length+bytes_read));

    // advance
    cursor += unit_length+bytes_read;
  }
  return result;
}

internal DW_CompRoot
dw_comp_root_from_range(Arena *arena, DW_SectionArray *sections, Rng1U64 range)
{
  Temp scratch = scratch_begin(&arena, 1);

  void *info_base   = dw_base_from_sec(sections, DW_Section_Info);
  B32   is_info_dwo = sections->v[DW_Section_Info].is_dwo;
  
  //- rjf: up-front known parsing offsets (yep, that's right, it's only 1!)
  U64 size_off = 0;
  
  //- rjf: parse size of this compilation unit's data
  U64 size        = 0;
  U64 version_off = size_off;
  {
    U64 bytes_read = dw_based_range_read_length(info_base, range, size_off, &size);
    version_off += bytes_read;
  }
  
  //- rjf: parse version
  B32        got_version = 0;
  DW_Version version     = 0;
  U64        unit_off    = version_off;
  if(based_range_read_struct(info_base, range, version_off, &version))
  {
    unit_off += sizeof(version);
    got_version = 1;
  }
  
  //- rjf: parse unit kind, abbrev_base, address size
  B32             got_unit_kind = 0;
  U64             next_off      = unit_off;
  DW_CompUnitKind unit_kind     = DW_CompUnitKind_Reserved;
  U64             abbrev_base   = max_U64;
  U64             address_size  = 0;
  U64             spec_dwo_id   = 0;
  if(got_version)
  {
    switch(version)
    {
      default: break;
      case DW_Version_2: {
        abbrev_base = 0;
        next_off += based_range_read(info_base, range, next_off, 4, &abbrev_base);
        next_off += based_range_read(info_base, range, next_off, 1, &address_size);
        got_unit_kind = 1;
      } break;
      case DW_Version_3:
      case DW_Version_4:
      {
        next_off += dw_based_range_read_length(info_base, range, next_off, &abbrev_base);
        next_off += based_range_read(info_base, range, next_off, 1, &address_size);
        got_unit_kind = 1;
      } break;
      case DW_Version_5:
      {
        next_off += based_range_read_struct(info_base, range, next_off, &unit_kind);
        next_off += based_range_read(info_base, range, next_off, 1, &address_size);
        next_off += dw_based_range_read_length(info_base, range, next_off, &abbrev_base);
        got_unit_kind = 1;
        
        //- rjf: parse DWO ID if appropriate
        if(unit_kind == DW_CompUnitKind_Skeleton || is_info_dwo)
        {
          next_off += based_range_read(info_base, range, next_off, 8, &spec_dwo_id);
        }
      } break;
    }
  }
  
  //- rjf: build abbrev table
  DW_AbbrevTable abbrev_table = {0};
  if(got_unit_kind)
  {
    abbrev_table = dw_make_abbrev_table(arena, sections, abbrev_base);
  }
  
  //- rjf: parse compilation unit's tag
  B32     got_comp_unit_tag = 0;
  DW_Tag *comp_unit_tag     = 0;
  if(got_unit_kind)
  {
    U64 comp_root_tag_off = range.min + next_off;
    comp_unit_tag     = dw_tag_from_info_offset(scratch.arena, sections, abbrev_table, version, DW_Ext_Null, DW_Language_Null, address_size, comp_root_tag_off);
    got_comp_unit_tag = 1;
  }
  
  //- rjf: get all of the attribute values we need to start resolving attribute values
  DW_AttribValueResolveParams resolve_params = { .version = version };
  if(got_comp_unit_tag)
  {
    for(DW_AttribNode *attrib_n = comp_unit_tag->attribs.first; attrib_n; attrib_n = attrib_n->next)
    {
      DW_Attrib *attrib = &attrib_n->attrib;
      
      // NOTE(rjf): We'll have to rely on just the form value at this point,
      // since we can't use the unit yet (since we're currently in the process
      // of building it). This should always be enough, otherwise there would
      // be a cyclic dependency in the requirements of each part of the
      // compilation unit's parse. DWARF is pretty crazy, but not *that* crazy,
      // so this should be good.
      switch(attrib->attrib_kind)
      {
        default: break;
        case DW_Attrib_AddrBase:       resolve_params.debug_addrs_base     = attrib->form_value.v[0]; break;
        case DW_Attrib_StrOffsetsBase: resolve_params.debug_str_offs_base  = attrib->form_value.v[0]; break;
        case DW_Attrib_RngListsBase:   resolve_params.debug_rnglists_base  = attrib->form_value.v[0]; break;
        case DW_Attrib_LocListsBase:   resolve_params.debug_loclists_base  = attrib->form_value.v[0]; break;
      }
    }
  }
  
  //- rjf: correct table offsets to header offsets (since DWARF V5 insists on being as useless as possible)
  if(got_comp_unit_tag && version >= DW_Version_5)
  {
    resolve_params.debug_addrs_base    = dw_v5_header_offset_from_table_offset(sections, DW_Section_Addr,       resolve_params.debug_addrs_base);
    resolve_params.debug_str_offs_base = dw_v5_header_offset_from_table_offset(sections, DW_Section_StrOffsets, resolve_params.debug_str_offs_base);
    resolve_params.debug_loclists_base = dw_v5_header_offset_from_table_offset(sections, DW_Section_LocLists,   resolve_params.debug_loclists_base);
    resolve_params.debug_rnglists_base = dw_v5_header_offset_from_table_offset(sections, DW_Section_RngLists,   resolve_params.debug_rnglists_base);
  }
  
  //- rjf: parse the rest of the compilation unit tag's attributes that we'd
  // like to cache
  String8        name                  = {0};
  String8        producer              = {0};
  String8        compile_dir           = {0};
  String8        external_dwo_name     = {0};
  String8        external_gnu_dwo_name = {0};
  U64            gnu_dwo_id            = 0;
  U64            language              = 0;
  U64            name_case             = 0;
  B32            use_utf8              = 0;
  U64            low_pc                = 0;
  U64            high_pc               = 0;
  B32            high_pc_is_relative   = 0;
  DW_AttribValue ranges_attrib_value   = {DW_Section_Null};
  U64            line_base             = 0;
  if(got_comp_unit_tag)
  {
    for(DW_AttribNode *attrib_n = comp_unit_tag->attribs.first; attrib_n; attrib_n = attrib_n->next)
    {
      DW_Attrib *attrib = &attrib_n->attrib;
      
      //- rjf: form value => value
      DW_AttribValue value      = {0};
      B32            good_value = 0;
      {
        if(dw_are_attrib_class_and_form_kind_compatible(version, attrib->value_class, attrib->form_kind))
        {
          value = dw_attrib_value_from_form_value(sections, resolve_params, attrib->form_kind, attrib->value_class, attrib->form_value);
          good_value = 1;
        }
      }
      
      //- rjf: map value to extracted info
      if(good_value)
      {
        switch(attrib->attrib_kind)
        {
          case DW_Attrib_Name:           name                   = dw_string_from_attrib_value(sections, value); break;
          case DW_Attrib_Producer:       producer               = dw_string_from_attrib_value(sections, value); break;
          case DW_Attrib_CompDir:        compile_dir            = dw_string_from_attrib_value(sections, value); break;
          case DW_Attrib_DwoName:        external_dwo_name      = dw_string_from_attrib_value(sections, value); break;
          case DW_Attrib_GNU_DwoName:    external_gnu_dwo_name  = dw_string_from_attrib_value(sections, value); break;
          case DW_Attrib_GNU_DwoId:      gnu_dwo_id             = value.v[0]; break;
          case DW_Attrib_Language:       language               = value.v[0]; break;
          case DW_Attrib_IdentifierCase: name_case              = value.v[0]; break;
          case DW_Attrib_UseUtf8:        use_utf8               = (B32)value.v[0]; break;
          case DW_Attrib_LowPc:          low_pc                 = value.v[0]; break;
          case DW_Attrib_HighPc:         high_pc                = value.v[0]; high_pc_is_relative = attrib->value_class != DW_AttribClass_Address; break;
          case DW_Attrib_Ranges:         ranges_attrib_value    = value; break;
          case DW_Attrib_StmtList:       line_base              = value.v[0]; break;
          default: break;
        }
      }
    }
  }
  
  //- rjf: build+fill unit
  DW_CompRoot unit = {0};
  
  //- rjf: fill header data
  unit.size            = size;
  unit.kind            = unit_kind;
  unit.version         = version;
  unit.ext             = DW_Ext_Null;
  unit.address_size    = address_size;
  unit.abbrev_off      = abbrev_base;
  unit.info_off        = range.min;
  unit.tags_info_range = rng_1u64(range.min+next_off, range.max);
  unit.abbrev_table    = abbrev_table;
  
  //- rjf: fill out offsets we need for attrib value resolution
  unit.rnglist_base = resolve_params.debug_rnglists_base;
  unit.loclist_base = resolve_params.debug_loclists_base;
  unit.addrs_base   = resolve_params.debug_addrs_base;
  unit.stroffs_base = resolve_params.debug_str_offs_base;
  
  //- rjf: fill out general info
  unit.name          = name;
  unit.producer      = producer;
  unit.compile_dir   = compile_dir;
  unit.external_dwo_name = external_dwo_name.size != 0 ? external_dwo_name : external_gnu_dwo_name;
  if(external_dwo_name.size)
  {
    unit.dwo_id = spec_dwo_id;
  }
  else if(external_gnu_dwo_name.size)
  {
    unit.dwo_id = gnu_dwo_id;
  }
  unit.language      = (DW_Language)language;
  unit.name_case     = name_case;
  unit.use_utf8      = use_utf8;
  unit.line_off      = line_base;
  unit.low_pc        = low_pc;
  unit.high_pc       = high_pc;
  unit.ranges_attrib_value = ranges_attrib_value;
  
  //- rjf: fill fixup of low/high PC situation
  if(high_pc_is_relative)
  {
    unit.high_pc += unit.low_pc;
  }
  
  //- rjf: fill base address
  {
    unit.base_addr = unit.low_pc;
  }
  
  //- rjf: build+fill directory and file tables
  {
    DW_Mode          line_mode = dw_mode_from_sec(sections, DW_Section_Line);
    void            *line_base = dw_base_from_sec(sections, DW_Section_Line);
    Rng1U64          line_rng  = dw_range_from_sec(sections, DW_Section_Line);
    DW_LineVMHeader  vm_header = {0};
    U64 read_size = dw_read_line_vm_header(arena, line_base, line_rng, unit.line_off, line_mode, sections, &unit, &vm_header);
    if (read_size > 0) {
      unit.dir_table  = vm_header.dir_table;
      unit.file_table = vm_header.file_table;
    }
  }
  
  scratch_end(scratch);
  return unit;
}

internal DW_ExtDebugRef
dw_ext_debug_ref_from_comp_root(DW_CompRoot *root)
{
  DW_ExtDebugRef ref = {0};
  ref.dwo_path       = root->external_dwo_name;
  ref.dwo_id         = root->dwo_id;
  return ref;
}

//- rjf: line info

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
  DW_LineNode *n = 0;
  if(vm_state->busted_seq == 0)
  {
    DW_LineSeqNode *seq = tbl->last_seq;
    if(seq == 0 || start_of_sequence)
    {
      // ERROR! do not emit sequences with only one line...
      if (seq) Assert(seq->count > 1);

      seq = dw_push_line_seq(arena, tbl);
    }
    
    n               = push_array(arena, DW_LineNode, 1);
    n->v.file_index = vm_state->file_index;
    n->v.line       = vm_state->line;
    n->v.column     = vm_state->column;
    n->v.voff       = vm_state->address;

    SLLQueuePush(seq->first, seq->last, n);
    seq->count += 1;
  }
  return n;
}

internal DW_LineTableParseResult
dw_parsed_line_table_from_comp_root(Arena *arena, DW_SectionArray *sections, DW_CompRoot *root)
{
  DW_Mode  mode            = sections->v[DW_Section_Line].mode;
  void    *base            = dw_base_from_sec(sections, DW_Section_Line);
  Rng1U64  line_info_range = dw_range_from_sec(sections, DW_Section_Line);
  U64      read_off_start  = root->line_off - line_info_range.min;
  U64      cursor          = read_off_start;
  
  DW_LineVMHeader vm_header = {0};
  cursor += dw_read_line_vm_header(arena, base, line_info_range, cursor, mode, sections, root, &vm_header);
  
  //- rjf: prep state for VM
  DW_LineVMState vm_state = {0};
  dw_line_vm_reset(&vm_state, vm_header.default_is_stmt);
  
  //- rjf: VM loop; build output list
  DW_LineTableParseResult result     = {0};
  B32                     end_of_seq = 0;
  B32                     error      = 0;
  for (;!error && cursor < vm_header.unit_opl;) {
    //- rjf: parse opcode
    U8 opcode = 0;
    cursor += based_range_read_struct(base, line_info_range, cursor, &opcode);
    
    //- rjf: do opcode action
    switch (opcode) {
      default:
      {
        //- rjf: special opcode case
        if(opcode >= vm_header.opcode_base)
        {
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
          
          dw_push_line(arena, &result, &vm_state, end_of_seq);
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
        else
        {
          if(opcode > 0 && opcode <= vm_header.num_opcode_lens)
          {
            U8 num_operands = vm_header.opcode_lens[opcode - 1];
            for(U8 i = 0; i < num_operands; ++i)
            {
              U64 operand = 0;
              cursor += based_range_read_uleb128(base, line_info_range, cursor, &operand);
            }
          }
          else
          {
            error = 1;
            goto exit;
          }
        }
      } break;
      
      //- Standard opcodes
      
      case DW_StdOpcode_Copy:
      {
        dw_push_line(arena, &result, &vm_state, end_of_seq);
        end_of_seq = 0;
        vm_state.discriminator   = 0;
        vm_state.basic_block     = 0;
        vm_state.prologue_end    = 0;
        vm_state.epilogue_begin  = 0;
      } break;
      
      case DW_StdOpcode_AdvancePc:
      {
        U64 advance = 0;
        cursor += based_range_read_uleb128(base, line_info_range, cursor, &advance);
        dw_line_vm_advance(&vm_state, advance, vm_header.min_inst_len, vm_header.max_ops_for_inst);
      } break;
      
      case DW_StdOpcode_AdvanceLine:
      {
        S64 s = 0;
        cursor += based_range_read_sleb128(base, line_info_range, cursor, &s);
        vm_state.line += s;
      } break;
      
      case DW_StdOpcode_SetFile:
      {
        U64 file_index = 0;
        cursor += based_range_read_uleb128(base, line_info_range, cursor, &file_index);
        vm_state.file_index = file_index;
      } break;
      
      case DW_StdOpcode_SetColumn:
      {
        U64 column = 0;
        cursor += based_range_read_uleb128(base, line_info_range, cursor, &column);
        vm_state.column = column;
      } break;
      
      case DW_StdOpcode_NegateStmt:
      {
        vm_state.is_stmt = !vm_state.is_stmt;
      } break;
      
      case DW_StdOpcode_SetBasicBlock:
      {
        vm_state.basic_block = 1;
      } break;
      
      case DW_StdOpcode_ConstAddPc:
      {
        U64 advance = (0xffu - vm_header.opcode_base)/vm_header.line_range;
        dw_line_vm_advance(&vm_state, advance, vm_header.min_inst_len, vm_header.max_ops_for_inst);
      } break;
      
      case DW_StdOpcode_FixedAdvancePc:
      {
        U16 operand = 0;
        cursor += based_range_read_struct(base, line_info_range, cursor, &operand);
        vm_state.address += operand;
        vm_state.op_index = 0;
      } break;
      
      case DW_StdOpcode_SetPrologueEnd:
      {
        vm_state.prologue_end = 1;
      } break;
      
      case DW_StdOpcode_SetEpilogueBegin:
      {
        vm_state.epilogue_begin = 1;
      } break;
      
      case DW_StdOpcode_SetIsa:
      {
        U64 v = 0;
        cursor += based_range_read_uleb128(base, line_info_range, cursor, &v);
        vm_state.isa = v;
      } break;
      
      //- Extended opcodes
      case DW_StdOpcode_ExtendedOpcode:
      {
        U64 length = 0;
        cursor += based_range_read_uleb128(base, line_info_range, cursor, &length);
        U64 start_off       = cursor;
        U8  extended_opcode = 0;
        cursor += based_range_read_struct(base, line_info_range, cursor, &extended_opcode);
        
        switch (extended_opcode) {
          case DW_ExtOpcode_EndSequence:
          {
            vm_state.end_sequence = 1;
            dw_push_line(arena, &result, &vm_state, 0);
            dw_line_vm_reset(&vm_state, vm_header.default_is_stmt);
            end_of_seq = 1;
          } break;
          
          case DW_ExtOpcode_SetAddress:
          {
            U64 address = 0;
            cursor += based_range_read(base, line_info_range, cursor, root->address_size, &address);
            vm_state.address    = address;
            vm_state.op_index   = 0;
            vm_state.busted_seq = address != 0; // !(dbg->acceptable_vrange.min <= address && address < dbg->acceptable_vrange.max);
          } break;
          
          case DW_ExtOpcode_DefineFile:
          {
            String8 file_name   = based_range_read_string(base, line_info_range, cursor);
            U64     dir_index   = 0;
            U64     modify_time = 0;
            U64     file_size   = 0;
            cursor += file_name.size + 1;
            cursor += based_range_read_uleb128(base, line_info_range, cursor, &dir_index);
            cursor += based_range_read_uleb128(base, line_info_range, cursor, &modify_time);
            cursor += based_range_read_uleb128(base, line_info_range, cursor, &file_size);
            
            // TODO(rjf): Not fully implemented. By the DWARF V4 spec, the above is
            // all that needs to be parsed, but the rest of the work that needs to
            // happen here---allowing this file to be used by further opcodes---is
            // not implemented.
            //
            // See the DWARF V4 spec (June 10, 2010), page 122.
            error = 1;
            AssertAlways(!"UNHANDLED DEFINE FILE!!!");
          } break;
          
          case DW_ExtOpcode_SetDiscriminator:
          {
            U64 v = 0;
            cursor += based_range_read_uleb128(base, line_info_range, cursor, &v);
            vm_state.discriminator = v;
          } break;
          
          default: break;
        }
        
        U64 num_skip = cursor - (start_off + length);
        cursor += num_skip;
        if (based_range_ptr(base, line_info_range, cursor) == 0 || start_off + length > cursor) {
          error = 1;
        }
        
      } break;
    }
  }
  exit:;
  
  return result;
}

internal U64
dw_read_line_file(void *line_base, Rng1U64 line_rng, U64 line_off, DW_Mode mode, DW_SectionArray *sections, DW_CompRoot *unit, U8 address_size, U64 format_count, Rng1U64 *formats, DW_LineFile *line_file_out)
{
  MemoryZeroStruct(line_file_out);
  
  DW_AttribValueResolveParams resolve_params = dw_attrib_value_resolve_params_from_comp_root(unit);
  U64                         line_off_start = line_off;
  for (U64 format_idx = 0; format_idx < format_count; ++format_idx) 
  {
    DW_LNCT        lnct       = (DW_LNCT)formats[format_idx].min;
    DW_FormKind    form_kind  = (DW_FormKind)formats[format_idx].max;
    DW_AttribValue form_value = {0};
    line_off += dw_based_range_read_attrib_form_value(line_base, line_rng, line_off, mode, address_size, form_kind, 0, &form_value);
    switch (lnct) 
    {
      case DW_LNCT_Path: 
      {
        Assert(form_kind == DW_Form_String || form_kind == DW_Form_LineStrp ||
                             form_kind == DW_Form_Strp || form_kind == DW_Form_StrpSup ||
                             form_kind == DW_Form_Strx || form_kind == DW_Form_Strx1 ||
                             form_kind == DW_Form_Strx2 || form_kind == DW_Form_Strx3 ||
                             form_kind == DW_Form_Strx4);
        DW_AttribValue attrib_value = dw_attrib_value_from_form_value(sections, resolve_params, form_kind, DW_AttribClass_String, form_value);
        line_file_out->file_name = dw_string_from_attrib_value(sections, attrib_value);
      } break;
      
      case DW_LNCT_DirectoryIndex: 
      {
        Assert(form_kind == DW_Form_Data1 || form_kind == DW_Form_Data2 ||
                             form_kind == DW_Form_UData);
        DW_AttribValue attrib_value = dw_attrib_value_from_form_value(sections, resolve_params, form_kind, DW_AttribClass_Block, form_value);
        line_file_out->dir_idx = attrib_value.v[0];
      } break;
      
      case DW_LNCT_TimeStamp: 
      {
        Assert(form_kind == DW_Form_UData || form_kind == DW_Form_Data4 ||
                             form_kind == DW_Form_Data8 || form_kind == DW_Form_Block);
        DW_AttribValue attrib_value = dw_attrib_value_from_form_value(sections, resolve_params, form_kind, DW_AttribClass_Const, form_value);
        line_file_out->modify_time = attrib_value.v[0];
      } break;
      
      case DW_LNCT_Size: 
      {
        Assert(form_kind == DW_Form_UData || form_kind == DW_Form_Data1 ||
                             form_kind == DW_Form_Data2 || form_kind == DW_Form_Data4 ||
                             form_kind == DW_Form_Data8);
        DW_AttribValue attrib_value = dw_attrib_value_from_form_value(sections, resolve_params, form_kind, DW_AttribClass_Block, form_value);
        line_file_out->file_size = attrib_value.v[0];
      } break;
      
      case DW_LNCT_MD5: 
      {
        Assert(form_kind == DW_Form_Data16);
        DW_AttribValue attrib_value = dw_attrib_value_from_form_value(sections, resolve_params, form_kind, DW_AttribClass_Block, form_value);
        line_file_out->md5_digest[0] = attrib_value.v[0];
        line_file_out->md5_digest[1] = attrib_value.v[1];
      } break;
      
      default: 
      {
        Assert(DW_LNCT_UserLo < lnct && lnct < DW_LNCT_UserHi);
      } break;
    }
  }
  U64 result = line_off - line_off_start;
  return result;
}

internal U64
dw_read_line_vm_header(Arena           *arena,
                       void            *line_base,
                       Rng1U64          line_rng,
                       U64              line_off,
                       DW_Mode          mode,
                       DW_SectionArray *sections,
                       DW_CompRoot     *unit,
                       DW_LineVMHeader *header_out)
{
  U64 line_off_start = line_off;
  
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: parse unit length
  header_out->unit_length = 0;
  {
    line_off += dw_based_range_read_length(line_base, line_rng, line_off, &header_out->unit_length);
  }
  
  header_out->unit_opl = line_off + header_out->unit_length;
  
  //- rjf: parse version and header length
  line_off += based_range_read_struct(line_base, line_rng, line_off, &header_out->version);
  
  if (header_out->version == DW_Version_5) {
    line_off += based_range_read_struct(line_base, line_rng, line_off, &header_out->address_size);
    line_off += based_range_read_struct(line_base, line_rng, line_off, &header_out->segment_selector_size);
  }
  
  {
    U64 off_size = dw_offset_size_from_mode(mode);
    line_off += based_range_read(line_base, line_rng, line_off, off_size, &header_out->header_length);
  }
  
  //- rjf: calculate program offset
  header_out->program_off = line_off + header_out->header_length;
  
  //- rjf: parse minimum instruction length
  {
    line_off += based_range_read_struct(line_base, line_rng, line_off, &header_out->min_inst_len);
  }
  
  //- rjf: parse max ops for instruction
  switch (header_out->version) 
  {
    case DW_Version_5:
    case DW_Version_4: 
    {
      line_off += based_range_read_struct(line_base, line_rng, line_off, &header_out->max_ops_for_inst);
      Assert(header_out->max_ops_for_inst > 0);
    } break;
    case DW_Version_3:
    case DW_Version_2:
    case DW_Version_1: 
    {
      header_out->max_ops_for_inst = 1;
    } break;
    default: break;
  }
  
  //- rjf: parse rest of program info
  based_range_read_struct(line_base, line_rng, line_off + 0 * sizeof(U8), &header_out->default_is_stmt);
  based_range_read_struct(line_base, line_rng, line_off + 1 * sizeof(U8), &header_out->line_base);
  based_range_read_struct(line_base, line_rng, line_off + 2 * sizeof(U8), &header_out->line_range);
  based_range_read_struct(line_base, line_rng, line_off + 3 * sizeof(U8), &header_out->opcode_base);
  line_off += 4 * sizeof(U8);
  Assert(header_out->opcode_base != 0 && header_out->opcode_base > 0);
  
  //- rjf: calculate opcode length array
  header_out->num_opcode_lens = header_out->opcode_base - 1u;
  header_out->opcode_lens     = (U8 *)based_range_ptr(line_base, line_rng, line_off);
  line_off += header_out->num_opcode_lens * sizeof(U8);
  
  if (header_out->version == DW_Version_5) {
    //- parse directory names
    U8 directory_entry_format_count = 0;
    line_off += based_range_read_struct(line_base, line_rng, line_off, &directory_entry_format_count);
    Assert(directory_entry_format_count == 1);
    Rng1U64 *directory_entry_formats = push_array(scratch.arena, Rng1U64, directory_entry_format_count);
    for (U8 format_idx = 0; format_idx < directory_entry_format_count; ++format_idx) 
    {
      U64 content_type_code = 0, form_code = 0;
      line_off += based_range_read_uleb128(line_base, line_rng, line_off, &content_type_code);
      line_off += based_range_read_uleb128(line_base, line_rng, line_off, &form_code);
      directory_entry_formats[format_idx] = rng_1u64(content_type_code, form_code);
    }
    U64 directories_count = 0;
    line_off += based_range_read_uleb128(line_base, line_rng, line_off, &directories_count);
    header_out->dir_table.count = directories_count;
    header_out->dir_table.v     = push_array(arena, String8, header_out->dir_table.count);
    for (U64 dir_idx = 0; dir_idx < directories_count; ++dir_idx) 
    {
      DW_LineFile line_file;
      line_off += dw_read_line_file(line_base,
                                    line_rng,
                                    line_off,
                                    mode,
                                    sections,
                                    unit,
                                    header_out->address_size,
                                    directory_entry_format_count,
                                    directory_entry_formats,
                                    &line_file);
      header_out->dir_table.v[dir_idx] = push_str8_copy(arena, line_file.file_name);
    }
    //- parse file table
    U8 file_name_entry_format_count = 0;
    line_off += based_range_read_struct(line_base, line_rng, line_off, &file_name_entry_format_count);
    Rng1U64 *file_name_entry_formats = push_array(scratch.arena, Rng1U64, file_name_entry_format_count);
    for (U8 format_idx = 0; format_idx < file_name_entry_format_count; ++format_idx) 
    {
      U64 content_type_code = 0, form_code = 0;
      line_off += based_range_read_uleb128(line_base, line_rng, line_off, &content_type_code);
      line_off += based_range_read_uleb128(line_base, line_rng, line_off, &form_code);
      file_name_entry_formats[format_idx] = rng_1u64(content_type_code, form_code);
    }
    U64 file_names_count = 0;
    line_off += based_range_read_uleb128(line_base, line_rng, line_off, &file_names_count);
    header_out->file_table.count = file_names_count;
    header_out->file_table.v     = push_array(arena, DW_LineFile, header_out->file_table.count);
    for (U64 file_idx = 0; file_idx < file_names_count; ++file_idx) 
    {
      line_off += dw_read_line_file(line_base,
                                    line_rng,
                                    line_off,
                                    mode,
                                    sections,
                                    unit,
                                    header_out->address_size,
                                    file_name_entry_format_count,
                                    file_name_entry_formats,
                                    &header_out->file_table.v[file_idx]);
    }
  }
  else 
  {
    String8List dir_list = {0};
    
    str8_list_push(scratch.arena, &dir_list, unit->compile_dir);
    for (;;) 
    {
      String8 dir = based_range_read_string(line_base, line_rng, line_off);
      line_off += dir.size + 1;
      if (dir.size == 0) 
      {
        break;
      }
      str8_list_push(scratch.arena, &dir_list, dir);
    }
    
    DW_LineVMFileList file_list = {0};
    
    //- rjf: push 0-index file (compile file)
    {
      DW_LineVMFileNode *node = push_array(scratch.arena, DW_LineVMFileNode, 1);
      node->file.file_name    = unit->name;
      SLLQueuePush(file_list.first, file_list.last, node);
      file_list.node_count += 1;
    }
    
    for (;;) 
    {
      String8 file_name   = based_range_read_string(line_base, line_rng, line_off);
      U64     dir_index   = 0;
      U64     modify_time = 0;
      U64     file_size   = 0;
      line_off += file_name.size + 1;
      if (file_name.size == 0) 
      {
        break;
      }
      line_off += based_range_read_uleb128(line_base, line_rng, line_off, &dir_index);
      line_off += based_range_read_uleb128(line_base, line_rng, line_off, &modify_time);
      line_off += based_range_read_uleb128(line_base, line_rng, line_off, &file_size);
      
      DW_LineVMFileNode *node = push_array(scratch.arena, DW_LineVMFileNode, 1);
      node->file.file_name    = file_name;
      node->file.dir_idx      = dir_index;
      node->file.modify_time  = modify_time;
      node->file.file_size    = file_size;
      SLLQueuePush(file_list.first, file_list.last, node);
      file_list.node_count += 1;
    }
    
    //- rjf: build dir table
    {
      header_out->dir_table.count = dir_list.node_count;
      header_out->dir_table.v     = push_array(arena, String8, header_out->dir_table.count);
      String8Node *n = dir_list.first;
      for(U64 idx = 0; n != 0 && idx < header_out->dir_table.count; idx += 1, n = n->next) 
      {
        header_out->dir_table.v[idx] = push_str8_copy(arena, n->string);
      }
    }
    
    //- rjf: build file table
    {
      header_out->file_table.count = file_list.node_count;
      header_out->file_table.v = push_array(arena, DW_LineFile, header_out->file_table.count);
      U64                file_idx  = 0;
      DW_LineVMFileNode *file_node = file_list.first;
      for(; file_node != 0; file_idx += 1, file_node = file_node->next) 
      {
        header_out->file_table.v[file_idx].file_name   = push_str8_copy(arena, file_node->file.file_name);
        header_out->file_table.v[file_idx].dir_idx     = file_node->file.dir_idx;
        header_out->file_table.v[file_idx].modify_time = file_node->file.modify_time;
        header_out->file_table.v[file_idx].file_size   = file_node->file.file_size;
      }
    }
  }
  
  U64 result = line_off - line_off_start;

  scratch_end(scratch);
  return result;
}

