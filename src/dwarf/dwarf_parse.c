// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Parsing Helpers

internal U64
dw2_read_initial_length(String8 data, U64 off, U64 *out, DW_Format *fmt_out)
{
  DW_Format format = DW_Format_32Bit;
  U64 size = 0;
  U64 size_size = 0;
  {
    U32 size_or_header = 0;
    size_size += str8_deserial_read_struct(data, off, &size_or_header);
    if(size_or_header == max_U32)
    {
      size_size += str8_deserial_read_struct(data, off + size_size, &size);
      format = DW_Format_64Bit;
    }
    else
    {
      size = (U64)size_or_header;
    }
  }
  if(out)
  {
    out[0] = size;
  }
  if(fmt_out)
  {
    fmt_out[0] = format;
  }
  return size_size;
}

internal U64
dw2_read_fmt_u64(String8 data, U64 off, DW_Format format, U64 *out)
{
  U64 start_off = off;
  switch(format)
  {
    default:
    case DW_Format_32Bit:
    {
      U32 out_32 = 0;
      off += str8_deserial_read_struct(data, off, &out_32);
      out[0] = (U64)out_32;
    }break;
    case DW_Format_64Bit:
    {
      off += str8_deserial_read_struct(data, off, out);
    }break;
  }
  U64 bytes_read = (off - start_off);
  return bytes_read;
}

internal DW_SectionKind
dw_section_kind_from_string(String8 string)
{
  // TODO(yuraiz): check if macho_string already fits to 16 bytes
  DW_SectionKind s = DW_SectionKind_Null;
  if(0){}
#define X(name, regular_string, macho_string, dwo_string) else if(str8_match(string, s(regular_string), 0) || str8_match(string, str8_prefix(s(macho_string), 16), 0) || str8_match(string, s(dwo_string), 0)) { s = DW_SectionKind_##name; }
  DW_SectionKind_XList
#undef X
  return s;
}

////////////////////////////////
//~ rjf: Unit Range Set Parsing (Top-Level .debug_info, .debug_aranges)

internal Rng1U64Array
dw2_unit_ranges_from_data(Arena *arena, String8 data)
{
  Temp scratch = scratch_begin(&arena, 1);
  Rng1U64List unit_range_list = {0};
  for(U64 off = 0; off < data.size;)
  {
    U64 start_off = off;
    
    // rjf: read next unit size
    U64 unit_size = 0;
    U64 unit_size_size = dw2_read_initial_length(data, off, &unit_size, 0);
    
    // rjf: push
    if(unit_size > 0)
    {
      rng1u64_list_push(scratch.arena, &unit_range_list, r1u64(off, off + unit_size_size + unit_size));
    }
    
    // rjf: advance
    off += unit_size_size;
    off += unit_size;
    
    // rjf: break if no movement
    if(off == start_off)
    {
      break;
    }
  }
  Rng1U64Array result = rng1u64_array_from_list(arena, &unit_range_list);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Unit Header Parsing

internal U64
dw2_read_unit_header(String8 data, U64 off, DW2_UnitHeader *out)
{
  U64 start_off = off;
  
  // rjf: read unit's .debug_info range size / format
  U64 unit_info_size = 0;
  DW_Format format = DW_Format_32Bit;
  off += dw2_read_initial_length(data, off, &unit_info_size, &format);
  
  // rjf: read dwarf version
  DW_Version version = 0;
  off += str8_deserial_read_struct(data, off, &version);
  
  // rjf: read header properties
  U64 abbrev_off = max_U64;
  U64 addr_size = 0;
  DW_CompUnitKind kind = DW_CompUnitKind_Reserved;
  U64 dwo_id = 0;
  switch((DW_VersionEnum)version)
  {
    case DW_Version_Null:
    case DW_Version_1:
    {}break;
    
    // rjf: @dwarf2
    case DW_Version_2:
    {
      // rjf: read abbrev off
      U32 abbrev_off_32 = 0;
      off += str8_deserial_read_struct(data, off, &abbrev_off_32);
      abbrev_off = (U64)abbrev_off_32;
      
      // rjf: read address size
      U8 addr_size_8 = 0;
      off += str8_deserial_read_struct(data, off, &addr_size_8);
      addr_size = (U64)addr_size_8;
    }break;
    
    // rjf: @dwarf3 @dwarf4
    case DW_Version_3:
    case DW_Version_4:
    {
      // rjf: read abbrev off
      off += dw2_read_fmt_u64(data, off, format, &abbrev_off);
      
      // rjf: read address size
      U8 addr_size_8 = 0;
      off += str8_deserial_read_struct(data, off, &addr_size_8);
      addr_size = (U64)addr_size_8;
    }break;
    
    // rjf: @dwarf5
    case DW_Version_5:
    {
      // rjf: read unit kind
      U8 comp_unit_kind_8 = 0;
      off += str8_deserial_read_struct(data, off, &comp_unit_kind_8);
      kind = (DW_CompUnitKind)comp_unit_kind_8;
      
      // rjf: read address size
      U8 addr_size_8 = 0;
      off += str8_deserial_read_struct(data, off, &addr_size_8);
      addr_size = (U64)addr_size_8;
      
      // rjf: read abbrev off
      off += dw2_read_fmt_u64(data, off, format, &abbrev_off);
      
      // rjf: read DWO ID, if applicable
      if(kind == DW_CompUnitKind_Skeleton ||
         kind == DW_CompUnitKind_SplitCompile ||
         kind == DW_CompUnitKind_SplitType)
      {
        off += str8_deserial_read_struct(data, off, &dwo_id);
      }
    }break;
  }
  
  // rjf: fill
  out->version     = version;
  out->format      = format;
  out->abbrev_off  = abbrev_off;
  out->addr_size   = addr_size;
  out->kind        = kind;
  out->dwo_id      = dwo_id;
  
  // rjf: return # of bytes read
  U64 bytes_read = (off - start_off);
  return bytes_read;
}

////////////////////////////////
//~ rjf: Abbreviation Map Parsing

internal U64
dw2_read_abbrev(Arena *arena, String8 data, U64 off, DW2_AbbrevHeader *header_out, DW2_AbbrevAttribList *attribs_out)
{
  U64 start_off = off;
  
  // rjf: read tag abbrev
  U64 id = 0;
  U64 tag_kind = 0;
  U8 has_children = 0;
  {
    off += str8_deserial_read_uleb128(data, off, &id);
    if(id != 0)
    {
      off += str8_deserial_read_uleb128(data, off, &tag_kind);
      off += str8_deserial_read_struct(data, off, &has_children);
    }
  }
  
  // rjf: read all tag attribute abbreviations
  DW2_AbbrevAttribList attribs = {0};
  if(id != 0)
  {
    for(;;)
    {
      U64 attrib_start_off = off;
      U64 attrib_kind = 0;
      U64 attrib_form_kind = 0;
      U64 implicit_const = 0;
      off += str8_deserial_read_uleb128(data, off, &attrib_kind);
      off += str8_deserial_read_uleb128(data, off, &attrib_form_kind);
      if(attrib_form_kind == DW_FormKind_ImplicitConst)
      {
        off += str8_deserial_read_uleb128(data, off, &implicit_const);
      }
      if(attrib_kind != 0 && attribs_out != 0)
      {
        DW2_AbbrevAttribNode *n = push_array(arena, DW2_AbbrevAttribNode, 1);
        SLLQueuePush(attribs.first, attribs.last, n);
        attribs.count += 1;
        n->v.attrib_kind = attrib_kind;
        n->v.form_kind = attrib_form_kind;
        n->v.implicit_const = implicit_const;
      }
      if(off == attrib_start_off || attrib_kind == 0)
      {
        break;
      }
    }
  }
  
  // rjf: fill
  if(header_out != 0)
  {
    header_out->id           = id;
    header_out->tag_kind     = tag_kind;
    header_out->has_children = has_children;
  }
  if(attribs_out != 0)
  {
    MemoryCopyStruct(attribs_out, &attribs);
  }
  
  U64 bytes_read = (off-start_off);
  return bytes_read;
}

internal DW2_AbbrevMap
dw2_abbrev_map_from_data(Arena *arena, String8 data, U64 off)
{
  DW2_AbbrevMap map = {0};
  
  // rjf: loop - first to determine the number of tag formats encoded
  // by this abbrev table, second to actually build a hash table for
  // mapping (id -> .debug_abbrev offset)
  for(B32 build = 0; build <= 1; build += 1)
  {
    // rjf: parse whole abbrev table
    U64 tag_count = 0;
    for(U64 read_off = off;;)
    {
      U64 start_off = read_off;
      
      // rjf: read next abbrev
      DW2_AbbrevHeader header = {0};
      read_off += dw2_read_abbrev(arena, data, read_off, &header, 0);
      
      // rjf: count tag
      if(header.id != 0)
      {
        tag_count += 1;
      }
      
      // rjf: if building (second loop) -> insert into map
      if(build)
      {
        U64 hash = u64_hash_from_str8(str8_struct(&header.id));
        U64 slot_idx = hash%map.slots_count;
        DW2_AbbrevMapNode *n = push_array(arena, DW2_AbbrevMapNode, 1);
        SLLStackPush(map.slots[slot_idx], n);
        n->id  = header.id;
        n->off = start_off;
      }
      
      // rjf: no tag ID, or no movement? -> exit
      if(read_off == start_off || header.id == 0)
      {
        break;
      }
    }
    
    // rjf: on first loop iteration, given the tag format count, construct the table
    if(!build)
    {
      map.slots_count = Max(1, tag_count + tag_count/4);
      map.slots = push_array(arena, DW2_AbbrevMapNode *, map.slots_count);
    }
  }
  
  return map;
}

internal DW2_UnitAbbrevMapMap
dw2_unit_abbrev_map_map_from_data(Arena *arena, String8 data, DW2_UnitHeader *unit_headers, U64 unit_headers_count)
{
  DW2_UnitAbbrevMapMap result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: gather deduplicated .debug_abbrev offsets, representing beginnings
  // of abbreviation tables. we want to do this because many units may refer
  // to the same abbreviation table.
  //
  typedef struct AbbrevOffNode AbbrevOffNode;
  struct AbbrevOffNode
  {
    AbbrevOffNode *order_next;
    AbbrevOffNode *hash_next;
    U64 off;
    U64 idx;
  };
  U64 abbrev_off_slots_count = unit_headers_count;
  AbbrevOffNode **abbrev_off_slots = 0;
  U64 abbrev_map_count = 0;
  U64 *abbrev_map_off_from_idx_table = 0;
  DW2_AbbrevMap *abbrev_map_from_idx_table = 0;
  DW2_AbbrevMap **abbrev_map_from_unit_idx_table = 0;
  ProfScope("gather deduplicated .debug_abbrev offsets") if(lane_idx() == 0)
  {
    AbbrevOffNode *abbrev_off_first = 0;
    AbbrevOffNode *abbrev_off_last = 0;
    abbrev_off_slots = push_array(scratch.arena, AbbrevOffNode *, abbrev_off_slots_count);
    for EachIndex(unit_idx, unit_headers_count)
    {
      U64 abbrev_off = unit_headers[unit_idx].abbrev_off;
      U64 hash = u64_hash_from_str8(str8_struct(&abbrev_off));
      U64 slot_idx = hash%abbrev_off_slots_count;
      AbbrevOffNode *node = 0;
      for(AbbrevOffNode *n = abbrev_off_slots[slot_idx]; n != 0; n = n->hash_next)
      {
        if(n->off == abbrev_off)
        {
          node = n;
          break;
        }
      }
      if(node == 0)
      {
        node = push_array(arena, AbbrevOffNode, 1);
        SLLStackPush_N(abbrev_off_slots[slot_idx], node, hash_next);
        SLLQueuePush_N(abbrev_off_first, abbrev_off_last, node, order_next);
        node->off = abbrev_off;
        node->idx = abbrev_map_count;
        abbrev_map_count += 1;
      }
    }
    abbrev_map_off_from_idx_table = push_array(scratch.arena, U64, abbrev_map_count);
    abbrev_map_from_idx_table = push_array(arena, DW2_AbbrevMap, abbrev_map_count);
    abbrev_map_from_unit_idx_table = push_array(arena, DW2_AbbrevMap *, unit_headers_count);
    {
      U64 abbrev_map_idx = 0;
      for(AbbrevOffNode *n = abbrev_off_first; n != 0; n = n->order_next)
      {
        abbrev_map_off_from_idx_table[abbrev_map_idx] = n->off;
        abbrev_map_idx += 1;
      }
    }
  }
  lane_sync_u64(&abbrev_off_slots, 0);
  lane_sync_u64(&abbrev_map_count, 0);
  lane_sync_u64(&abbrev_map_off_from_idx_table, 0);
  lane_sync_u64(&abbrev_map_from_idx_table, 0);
  lane_sync_u64(&abbrev_map_from_unit_idx_table, 0);
  
  //- rjf: build all unique abbreviation maps
  ProfScope("build all unique abbreviation maps")
  {
    U64 abbrev_map_take_idx = 0;
    U64 *abbrev_map_take_idx_ptr = &abbrev_map_take_idx;
    lane_sync_u64(&abbrev_map_take_idx_ptr, 0);
    for(;;)
    {
      U64 abbrev_map_idx = ins_atomic_u64_inc_eval(abbrev_map_take_idx_ptr) - 1;
      if(abbrev_map_idx >= abbrev_map_count)
      {
        break;
      }
      abbrev_map_from_idx_table[abbrev_map_idx] = dw2_abbrev_map_from_data(arena, data, abbrev_map_off_from_idx_table[abbrev_map_idx]);
    }
    lane_sync();
  }
  
  //- rjf: build unit -> abbrev map table
  ProfScope("build unit -> abbrev map table")
  {
    Rng1U64 range = lane_range(unit_headers_count);
    for EachInRange(unit_idx, range)
    {
      U64 abbrev_off = unit_headers[unit_idx].abbrev_off;
      U64 hash = u64_hash_from_str8(str8_struct(&abbrev_off));
      U64 slot_idx = hash%abbrev_off_slots_count;
      U64 abbrev_map_idx = 0;
      for(AbbrevOffNode *n = abbrev_off_slots[slot_idx]; n != 0; n = n->hash_next)
      {
        if(n->off == abbrev_off)
        {
          abbrev_map_idx = n->idx;
          break;
        }
      }
      abbrev_map_from_unit_idx_table[unit_idx] = &abbrev_map_from_idx_table[abbrev_map_idx];
    }
    lane_sync();
  }
  
  //- rjf: fill result
  result.map_count = abbrev_map_count;
  result.map_from_idx_table = abbrev_map_from_idx_table;
  result.map_from_unit_idx_table = abbrev_map_from_unit_idx_table;
  
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Form Value Parsing

internal U64
dw2_read_form_val(DW_Raw *raw, DW2_ParseCtx *ctx, String8 data, U64 off, DW_FormKind form_kind, U64 implicit_const, DW2_FormVal *out)
{
  U64 start_off = off;
  DW2_FormVal val = {form_kind};
  {
    //- rjf: read value bytes
    U64 bytes_to_read = 0;
    switch(form_kind)
    {
      //- rjf: no-ops
      default:
      case DW_FormKind_Indirect:
      case DW_FormKind_Null:
      {}break;
      
      //- rjf: 1-byte uint reads
      case DW_FormKind_Ref1:
      case DW_FormKind_Data1:
      case DW_FormKind_Flag:
      case DW_FormKind_Strx1:
      case DW_FormKind_Addrx1:
      bytes_to_read = 1; goto read_fixed_uint;
      
      //- rjf: 2-byte uint reads
      case DW_FormKind_Ref2:
      case DW_FormKind_Data2:
      case DW_FormKind_Strx2:
      case DW_FormKind_Addrx2:
      bytes_to_read = 2; goto read_fixed_uint;
      
      //- rjf: 3-byte uint reads
      case DW_FormKind_Strx3:
      case DW_FormKind_Addrx3:
      bytes_to_read = 3; goto read_fixed_uint;
      
      //- rjf: 4-byte uint reads
      case DW_FormKind_Data4:
      case DW_FormKind_Ref4:
      case DW_FormKind_RefSup4:
      case DW_FormKind_Strx4:
      case DW_FormKind_Addrx4:
      bytes_to_read = 4; goto read_fixed_uint;
      
      //- rjf: 8-byte uint reads
      case DW_FormKind_Data8:
      case DW_FormKind_Ref8:
      case DW_FormKind_RefSig8:
      case DW_FormKind_RefSup8:
      bytes_to_read = 8; goto read_fixed_uint;
      
      //- rjf: 16-byte uint reads
      case DW_FormKind_Data16:
      bytes_to_read = 16; goto read_fixed_uint;
      
      //- rjf: address-sized reads
      case DW_FormKind_Addr:
      bytes_to_read = ctx->addr_size; goto read_fixed_uint;
      
      //- rjf: format-defined-size reads
      case DW_FormKind_RefAddr:
      case DW_FormKind_SecOffset:
      case DW_FormKind_LineStrp:
      case DW_FormKind_Strp:
      case DW_FormKind_StrpSup:
      bytes_to_read = dw_addr_size_from_format(ctx->format); goto read_fixed_uint;
      
      //- rjf: fixed-size uint reads
      read_fixed_uint:
      {
        off += str8_deserial_read(data, off, &val.u128.u64[0], bytes_to_read, bytes_to_read);
      }break;
      
      //- rjf: uleb128 reads
      case DW_FormKind_UData:
      case DW_FormKind_RefUData:
      case DW_FormKind_Strx:
      case DW_FormKind_Addrx:
      case DW_FormKind_LocListx:
      case DW_FormKind_RngListx:
      {
        off += str8_deserial_read_uleb128(data, off, &val.u128.u64[0]);
      }break;
      
      //- rjf: sleb128 reads
      case DW_FormKind_SData:
      {
        off += str8_deserial_read_sleb128(data, off, (S64 *)(&val.u128.u64[0]));
      }break;
      
      //- rjf: fixed-size uint reads / skips
      case DW_FormKind_Block1: bytes_to_read = 1; goto read_fixed_uint_and_skip;
      case DW_FormKind_Block2: bytes_to_read = 2; goto read_fixed_uint_and_skip;
      case DW_FormKind_Block4: bytes_to_read = 4; goto read_fixed_uint_and_skip;
      read_fixed_uint_and_skip:
      {
        U64 size = 0;
        U64 og_read_off = off;
        off += str8_deserial_read(data, off, &size, bytes_to_read, bytes_to_read);
        val.u128.u64[0] = size;
        val.u128.u64[1] = og_read_off;
        off += size;
      }break;
      
      //- rjf: uleb128 read & skip
      case DW_FormKind_Block:
      {
        U64 size = 0;
        U64 og_read_off = off;
        off += str8_deserial_read_uleb128(data, off, &size);
        val.string = str8_substr(data, r1u64(off, off+size));
        off += size;
      }break;
      
      //- rjf: strings
      case DW_FormKind_String:
      {
        String8 string = {0};
        U64 og_read_off = off;
        off += str8_deserial_read_cstr(data, off, &string);
        val.string = string;
      }break;
      
      //- rjf: implicit constants (no reading in .debug_info; value comes from .debug_abbrev)
      case DW_FormKind_ImplicitConst:
      {
        val.u128.u64[0] = implicit_const;
      }break;
      
      //- rjf: exprloc
      case DW_FormKind_ExprLoc:
      {
        U64 size = 0;
        U64 og_read_off = off;
        U64 bytes_read = str8_deserial_read_uleb128(data, off, &size);
        val.u128.u64[0] = og_read_off + bytes_read;
        val.u128.u64[1] = size;
        val.string = str8_substr(data, r1u64(og_read_off, og_read_off+size));
        off += size + bytes_read;
      }break;
      
      //- rjf: flag present
      case DW_FormKind_FlagPresent:
      {
        val.u128.u64[0] = 1;
      }break;
    }
    
    //- rjf: in some cases, resolve numeric values -> strings
    {
      switch(form_kind)
      {
        default:{}break;
        case DW_FormKind_Strx1:
        case DW_FormKind_Strx2:
        case DW_FormKind_Strx3:
        case DW_FormKind_Strx4:
        case DW_FormKind_Strx:
        if(ctx->str_offsets_table != 0)
        {
          U64 entry_idx = val.u128.u64[0];
          U64 string_data_off = 0;
          if(dw2_try_offset_from_table_idx(ctx->str_offsets_table, entry_idx, &string_data_off))
          {
            String8 string_section_data = raw->sec[DW_SectionKind_Str].data;
            val.string = str8_cstring_capped(string_section_data.str + string_data_off,
                                             string_section_data.str + string_section_data.size);
          }
        }break;
        case DW_FormKind_LineStrp:
        {
          String8 string_section_data = raw->sec[DW_SectionKind_LineStr].data;
          val.string = str8_cstring_capped(string_section_data.str + val.u128.u64[0],
                                           string_section_data.str + string_section_data.size);
        }break;
        case DW_FormKind_Strp:
        case DW_FormKind_StrpSup:
        {
          String8 string_section_data = raw->sec[DW_SectionKind_Str].data;
          val.string = str8_cstring_capped(string_section_data.str + val.u128.u64[0],
                                           string_section_data.str + string_section_data.size);
        }break;
      }
    }
    
    //- rjf: in some cases, resolve base numeric values -> addresses
    {
      switch(form_kind)
      {
        default:{val.addr = val.u128.u64[0];}break;
        case DW_FormKind_Addrx:
        case DW_FormKind_Addrx1:
        case DW_FormKind_Addrx2:
        case DW_FormKind_Addrx3:
        case DW_FormKind_Addrx4:
        if(ctx->addr_table != 0)
        {
          U64 addr_idx = val.u128.u64[0];
          dw2_try_offset_from_table_idx(ctx->addr_table, addr_idx, &val.addr);
        }break;
      }
    }
  }
  *out = val;
  U64 bytes_read = (off - start_off);
  return bytes_read;
}

////////////////////////////////
//~ rjf: Parse Context Construction

internal void
dw2_parse_ctx_equip_unit_header(DW2_ParseCtx *ctx_out, DW2_UnitHeader *hdr, U64 unit_base_info_off)
{
  ctx_out->version            = hdr->version;
  ctx_out->format             = hdr->format;
  ctx_out->addr_size          = hdr->addr_size;
  ctx_out->unit_base_info_off = unit_base_info_off;
}

internal void
dw2_parse_ctx_equip_unit_abbrev_map(DW2_ParseCtx *ctx_out, DW2_UnitAbbrevMapMap *map, U64 unit_idx)
{
  ctx_out->abbrev_map = map->map_from_unit_idx_table[unit_idx];
}

internal void
dw2_parse_ctx_equip_unit_root_tag(DW2_ParseCtx *ctx_out, DW2_Tag *tag, DW2_OffsetTableSet *offset_tables)
{
  // rjf: unpack attributes
  DW2_Attrib *low_pc_attrib = &dw2_attrib_nil;
  DW2_Attrib *lang_attrib = &dw2_attrib_nil;
  DW2_Attrib *rnglists_base_off_attrib = &dw2_attrib_nil;
  DW2_Attrib *str_offsets_base_off_attrib = &dw2_attrib_nil;
  DW2_Attrib *addr_base_off_attrib = &dw2_attrib_nil;
  DW2_Attrib *loclist_base_off_attrib = &dw2_attrib_nil;
  DW2_Attrib *comp_dir_attrib = &dw2_attrib_nil;
  for EachNode(n, DW2_AttribNode, tag->attribs.first)
  {
    switch(n->v.attrib_kind)
    {
      default:{}break;
#define Case(name, name_upper) case DW_AttribKind_##name_upper:{name##_attrib = &n->v;}break
      Case(low_pc,               LowPc);
      Case(lang,                 Language);
      Case(rnglists_base_off,    RngListsBase);
      Case(str_offsets_base_off, StrOffsetsBase);
      Case(addr_base_off,        AddrBase);
      Case(loclist_base_off,     LocListsBase);
      Case(comp_dir,             CompDir);
#undef Case
    }
  }
  
  // rjf: fill basics
  ctx_out->unit_base_addr = low_pc_attrib->val.addr;
  ctx_out->language = lang_attrib->val.u128.u64[0];
  ctx_out->unit_dir = comp_dir_attrib->val.string;
  
  // rjf: find offset tables
  Rng1U64Array rnglists_tables_ranges_array = {offset_tables->rnglists_tables_ranges, offset_tables->rnglists_tables_count};
  Rng1U64Array str_offsets_tables_ranges_array = {offset_tables->str_offsets_tables_ranges, offset_tables->str_offsets_tables_count};
  Rng1U64Array addr_tables_ranges_array = {offset_tables->addr_tables_ranges, offset_tables->addr_tables_count};
  Rng1U64Array loclist_tables_ranges_array = {offset_tables->loclists_tables_ranges, offset_tables->loclists_tables_count};
  {
    // rjf: find rnglists table
    {
      U64 rnglists_base_off = rnglists_base_off_attrib->val.u128.u64[0];
      U64 rnglists_table_num = rng1u64_array_num_from_value__binary_search(&rnglists_tables_ranges_array, rnglists_base_off);
      if(0 < rnglists_table_num && rnglists_table_num <= rnglists_tables_ranges_array.count)
      {
        DW2_OffsetTable *table = &offset_tables->rnglists_tables[rnglists_table_num-1];
        ctx_out->rnglists_table = table;
      }
    }
    
    // rjf: find str offsets table
    {
      U64 str_offsets_base_off = str_offsets_base_off_attrib->val.u128.u64[0];
      U64 str_offsets_table_num = rng1u64_array_num_from_value__binary_search(&str_offsets_tables_ranges_array, str_offsets_base_off);
      if(0 < str_offsets_table_num && str_offsets_table_num <= str_offsets_tables_ranges_array.count)
      {
        DW2_OffsetTable *table = &offset_tables->str_offsets_tables[str_offsets_table_num-1];
        ctx_out->str_offsets_table = table;
      }
    }
    
    // rjf: find addr table
    {
      U64 addr_base_off = addr_base_off_attrib->val.u128.u64[0];
      U64 addr_table_num = rng1u64_array_num_from_value__binary_search(&addr_tables_ranges_array, addr_base_off);
      if(0 < addr_table_num && addr_table_num <= addr_tables_ranges_array.count)
      {
        DW2_OffsetTable *table = &offset_tables->addr_tables[addr_table_num-1];
        ctx_out->addr_table = table;
      }
    }
    
    // rjf: find loclists table
    {
      U64 loclist_base_off = loclist_base_off_attrib->val.u128.u64[0];
      U64 loclist_table_num = rng1u64_array_num_from_value__binary_search(&loclist_tables_ranges_array, loclist_base_off);
      if(0 < loclist_table_num && loclist_table_num <= loclist_tables_ranges_array.count)
      {
        DW2_OffsetTable *table = &offset_tables->loclists_tables[loclist_table_num-1];
        ctx_out->loclists_table = table;
      }
    }
  }
}

////////////////////////////////
//~ rjf: Tag Parsing

internal U64
dw2_read_tag(Arena *arena, DW_Raw *raw, DW2_ParseCtx *ctx, String8 data, U64 off, DW2_Tag *tag_out)
{
  U64 start_off = off;
  
  // rjf: read tag's abbrev ID
  U64 abbrev_id = 0;
  off += str8_deserial_read_uleb128(data, off, &abbrev_id);
  
  // rjf: map abbrev ID -> abbrev offset
  U64 abbrev_off = 0;
  {
    DW2_AbbrevMap *abbrev_map = ctx->abbrev_map;
    U64 hash = u64_hash_from_str8(str8_struct(&abbrev_id));
    U64 slot_idx = hash%abbrev_map->slots_count;
    for EachNode(n, DW2_AbbrevMapNode, abbrev_map->slots[slot_idx])
    {
      if(n->id == abbrev_id)
      {
        abbrev_off = n->off;
        break;
      }
    }
  }
  
  // rjf: read abbrev header
  U64 tag_kind = 0;
  U8 has_children = 0;
  U64 attrib_abbrev_read_off = 0;
  {
    String8 abbrev_data = raw->sec[DW_SectionKind_Abbrev].data;
    U64 abbrev_read_off = abbrev_off;
    U64 id = 0;
    abbrev_read_off += str8_deserial_read_uleb128(abbrev_data, abbrev_read_off, &id);
    if(id != 0)
    {
      abbrev_read_off += str8_deserial_read_uleb128(abbrev_data, abbrev_read_off, &tag_kind);
      abbrev_read_off += str8_deserial_read_struct(abbrev_data, abbrev_read_off, &has_children);
    }
    attrib_abbrev_read_off = abbrev_read_off;
  }
  
  // rjf: walk abbrev / info in lock-step; read all tag attributes
  DW2_AttribList attribs = {0};
  if(abbrev_id != 0)
  {
    String8 abbrev_data = raw->sec[DW_SectionKind_Abbrev].data;
    U64 abbrev_read_off = attrib_abbrev_read_off;
    for(;;)
    {
      U64 abbrev_start_read_off = abbrev_read_off;
      
      // rjf: read next attribute abbreviation from .debug_abbrev
      U64 attrib_kind = 0;
      U64 attrib_form_kind = 0;
      U64 implicit_const = 0;
      abbrev_read_off += str8_deserial_read_uleb128(abbrev_data, abbrev_read_off, &attrib_kind);
      abbrev_read_off += str8_deserial_read_uleb128(abbrev_data, abbrev_read_off, &attrib_form_kind);
      if(attrib_form_kind == DW_FormKind_ImplicitConst)
      {
        abbrev_read_off += str8_deserial_read_uleb128(abbrev_data, abbrev_read_off, &implicit_const);
      }
      
      // rjf: indirect form -> form kind is encoded in .debug_info (because why not)
      if(attrib_form_kind == DW_FormKind_Indirect)
      {
        off += str8_deserial_read_uleb128(data, off, &attrib_form_kind);
      }
      
      // rjf: read attribute's value from .debug_info
      DW2_FormVal val = {0};
      if(attrib_kind != 0)
      {
        off += dw2_read_form_val(raw, ctx, data, off, attrib_form_kind, implicit_const, &val);
      }
      
      // rjf: push
      if(attrib_kind != 0)
      {
        DW2_AttribNode *n = push_array(arena, DW2_AttribNode, 1);
        SLLQueuePush(attribs.first, attribs.last, n);
        attribs.count += 1;
        n->v.attrib_kind = attrib_kind;
        n->v.val         = val;
      }
      
      // rjf: no movement, or no attrib kind -> done
      if(abbrev_read_off == abbrev_start_read_off || attrib_kind == 0)
      {
        break;
      }
    }
  }
  
  // rjf: fill output
  tag_out->kind         = (DW_TagKind)tag_kind;
  tag_out->has_children = !!has_children;
  tag_out->attribs      = attribs;
  
  // rjf: return # of bytes read
  U64 bytes_read = (off - start_off);
  return bytes_read;
}

internal DW2_Attrib *
dw2_attrib_from_kind(DW2_Tag *tag, DW_AttribKind kind)
{
  DW2_Attrib *result = &dw2_attrib_nil;
  for EachNode(n, DW2_AttribNode, tag->attribs.first)
  {
    if(n->v.attrib_kind == kind)
    {
      result = &n->v;
      break;
    }
  }
  return result;
}

internal U64
dw2_reference_info_off_from_form_val(DW2_ParseCtx *ctx, DW2_FormVal *v)
{
  U64 result = 0;
  switch(v->kind)
  {
    default:{}break;
    case DW_FormKind_Ref1:
    case DW_FormKind_Ref2:
    case DW_FormKind_Ref4:
    case DW_FormKind_Ref8:
    {
      result = ctx->unit_base_info_off + v->u128.u64[0];
    }break;
    // TODO(rjf): DW_FormKind_RefAddr, DW_FormKind_RefUData, DW_FormKind_RefSig8, DW_FormKind_RefSup8, etc.
  }
  return result;
}

////////////////////////////////
//~ rjf: Line Table Parsing

internal U64
dw2_read_line_table_header(Arena *arena, DW_Raw *raw, DW2_ParseCtx *ctx, String8 data, U64 off, DW2_LineTableHeader *out)
{
  Temp scratch = scratch_begin(&arena, 1);
  U64 start_off = off;
  
  //////////////////////////////
  //- rjf: read unit length
  //
  U64 unit_length = 0;
  DW_Format format = DW_Format_32Bit;
  off += dw2_read_initial_length(data, off, &unit_length, &format);
  U64 off_opl = off + unit_length;
  
  //////////////////////////////
  //- rjf: read version
  //
  DW_Version version = DW_Version_Null;
  off += str8_deserial_read_struct(data, off, &version);
  
  //////////////////////////////
  //- rjf: read address / segment selector sizes
  //
  U8 addr_size = 0;
  U8 segment_selector_size = 0;
  switch(version)
  {
    // NOTE(rjf): pre-dwarf5: assume from context
    default:
    {
      addr_size = ctx->addr_size;
    }break;
    
    // NOTE(rjf): @dwarf5 read address / segment selector sizes explicitly
    case DW_Version_5:
    {
      off += str8_deserial_read_struct(data, off, &addr_size);
      off += str8_deserial_read_struct(data, off, &segment_selector_size);
    }break;
  }
  
  //////////////////////////////
  //- rjf: read all remaining flat header properties
  //
  U64 opl_header_length_off = 0;
  U64 header_length = 0;
  U8 min_inst_length = 0;
  U8 max_ops_per_inst = 1;
  U8 default_is_stmt = 0;
  S8 line_base = 0;
  U8 line_range = 0;
  U8 opcode_base = 0;
  {
    off += dw2_read_fmt_u64(data, off, format, &header_length);
    opl_header_length_off = off;
    off += str8_deserial_read_struct(data, off, &min_inst_length);
    off += str8_deserial_read_struct(data, off, &max_ops_per_inst);
    off += str8_deserial_read_struct(data, off, &default_is_stmt);
    off += str8_deserial_read_struct(data, off, &line_base);
    off += str8_deserial_read_struct(data, off, &line_range);
    off += str8_deserial_read_struct(data, off, &opcode_base);
  }
  
  //////////////////////////////
  //- rjf: read opcode lengths
  //
  U64 opcode_lengths_count = (opcode_base > 0 ? opcode_base - 1 : 0);
  U8 *opcode_lengths = 0;
  if(opcode_lengths_count > 0)
  {
    opcode_lengths = str8_skip(data, off).str;
    off += sizeof(opcode_lengths[0]) * opcode_lengths_count;
  }
  
  //////////////////////////////
  //- rjf: read directory / file tables
  //
  DW2_LineTableFileArray dirs = {0};
  DW2_LineTableFileArray files = {0};
  switch(version)
  {
    ////////////////////////////
    //- rjf: pre-dwarf5: simple(r) well-defined format... still ulebs everywhere but oh well...
    // just wait until the next case...
    //
    default:
    {
      //- rjf: gather directories
      DW2_LineTableFileList dir_list = {0};
      {
        // rjf: push compile directory as first list element, always
        {
          DW2_LineTableFileNode *n = push_array(scratch.arena, DW2_LineTableFileNode, 1);
          SLLQueuePush(dir_list.first, dir_list.last, n);
          dir_list.count += 1;
          n->v.file_name = ctx->unit_dir;
        }
        
        // rjf: gather additional directories
        for(;off < off_opl;)
        {
          String8 dir = {0};
          off += str8_deserial_read_cstr(data, off, &dir);
          if(dir.size != 0)
          {
            DW2_LineTableFileNode *n = push_array(scratch.arena, DW2_LineTableFileNode, 1);
            SLLQueuePush(dir_list.first, dir_list.last, n);
            dir_list.count += 1;
            n->v.file_name = dir;
          }
          else
          {
            break;
          }
        }
      }
      
      //- rjf: gather files
      DW2_LineTableFileList file_list = {0};
      {
        // rjf: push main compilation unit file as first list element, always
        {
          DW2_LineTableFileNode *n = push_array(scratch.arena, DW2_LineTableFileNode, 1);
          SLLQueuePush(file_list.first, file_list.last, n);
          file_list.count += 1;
          n->v.file_name = ctx->unit_file;
        }
        
        // rjf: gather additional files
        for(;off < off_opl;)
        {
          U8 next_byte = 0;
          str8_deserial_read_struct(data, off, &next_byte);
          if(next_byte == 0)
          {
            break;
          }
          String8 file_name = {0};
          U64 dir_idx = 0;
          U64 modify_time = 0;
          U64 file_size = 0;
          off += str8_deserial_read_cstr(data, off, &file_name);
          off += str8_deserial_read_uleb128(data, off, &dir_idx);
          off += str8_deserial_read_uleb128(data, off, &modify_time);
          off += str8_deserial_read_uleb128(data, off, &file_size);
          if(file_name.size != 0)
          {
            DW2_LineTableFileNode *n = push_array(scratch.arena, DW2_LineTableFileNode, 1);
            SLLQueuePush(file_list.first, file_list.last, n);
            file_list.count += 1;
            n->v.file_name   = file_name;
            n->v.dir_idx     = dir_idx;
            n->v.modify_time = modify_time;
            n->v.file_size   = file_size;
          }
        }
      }
      
      //- rjf: flatten
      {
        dirs.count = dir_list.count;
        dirs.v = push_array(arena, DW2_LineTableFile, dirs.count);
        files.count = file_list.count;
        files.v = push_array(arena, DW2_LineTableFile, files.count);
        {
          U64 idx = 0;
          for EachNode(n, DW2_LineTableFileNode, dir_list.first)
          {
            MemoryCopyStruct(&dirs.v[idx], &n->v);
            idx += 1;
          }
        }
        {
          U64 idx = 0;
          for EachNode(n, DW2_LineTableFileNode, file_list.first)
          {
            MemoryCopyStruct(&files.v[idx], &n->v);
            idx += 1;
          }
        }
      }
    }break;
    
    ////////////////////////////
    //- rjf: @dwarf5: another VM! d'oh. :(
    //
    case DW_Version_5:
    {
      //- rjf: read directory & file tables
      for(B32 do_files = 0, do_dirs = 1; do_files <= 1; do_files += 1, do_dirs = 0)
      {
        // rjf: read header
        U8 entry_formats_count = 0;
        off += str8_deserial_read_struct(data, off, &entry_formats_count);
        U64 *entry_formats = push_array(scratch.arena, U64, entry_formats_count*2);
        for EachIndex(idx, entry_formats_count)
        {
          off += str8_deserial_read_uleb128(data, off, &entry_formats[idx*2 + 0]);
          off += str8_deserial_read_uleb128(data, off, &entry_formats[idx*2 + 1]);
        }
        U64 table_count = 0;
        off += str8_deserial_read_uleb128(data, off, &table_count);
        
        // rjf: read all table entries
        DW2_LineTableFileArray table = {0};
        table.count = table_count;
        table.v = push_array(arena, DW2_LineTableFile, table.count);
        {
          U64 idx = 0;
          for(;off < off_opl && idx < table.count; idx += 1)
          {
            DW2_LineTableFile *entry_out = &table.v[idx];
            for(U64 entry_format_idx = 0; entry_format_idx < entry_formats_count; entry_format_idx += 1)
            {
              DW_LNCT lnct          =     (DW_LNCT)entry_formats[entry_format_idx*2 + 0];
              DW_FormKind form_kind = (DW_FormKind)entry_formats[entry_format_idx*2 + 1];
              DW2_FormVal form_val = {0};
              off += dw2_read_form_val(raw, ctx, data, off, form_kind, 0, &form_val);
              switch(lnct)
              {
                default:
                {
                  log_infof("DWARF LNCT encoding not currently supported (0x%I64x).\n", (U64)lnct);
                }break;
                case DW_LNCT_Path:
                {
                  entry_out->file_name = form_val.string;
                }break;
                case DW_LNCT_DirectoryIndex:
                {
                  entry_out->dir_idx = form_val.u128.u64[0];
                }break;
                case DW_LNCT_TimeStamp:
                {
                  entry_out->modify_time = form_val.u128.u64[0];
                  entry_out->flags |= DW2_LineTableFileFlag_HasModifyTime;
                }break;
                case DW_LNCT_Size:
                {
                  entry_out->file_size = form_val.u128.u64[0];
                }break;
                case DW_LNCT_MD5:
                {
                  entry_out->md5.u128 = form_val.u128;
                  entry_out->flags |= DW2_LineTableFileFlag_HasMD5;
                }break;
                case DW_LNCT_LLVM_Source:
                {
                  entry_out->source = form_val.string;
                }break;
              }
            }
          }
        }
        
        // rjf: store
        if(0){}
        else if(do_files) { files = table; }
        else if(do_dirs)  { dirs = table; }
      }
      
    }break;
  }
  
  //////////////////////////////
  //- rjf: fill output
  //
  if(out)
  {
    out->unit_length           = unit_length;
    out->format                = format;
    out->version               = version;
    out->addr_size             = addr_size;
    out->segment_selector_size = segment_selector_size;
    out->header_length         = header_length;
    out->min_inst_length       = min_inst_length;
    out->max_ops_per_inst      = max_ops_per_inst;
    out->default_is_stmt       = default_is_stmt;
    out->line_base             = line_base;
    out->line_range            = line_range;
    out->opcode_base           = opcode_base;
    out->opcode_lengths_count  = opcode_lengths_count;
    out->opcode_lengths        = opcode_lengths;
    out->dirs                  = dirs;
    out->files                 = files;
    out->line_program_off      = opl_header_length_off + header_length;
    out->total_unit_data_size  = (off_opl - start_off);
  }
  
  U64 bytes_read = (off - start_off);
  scratch_end(scratch);
  return bytes_read;
}

////////////////////////////////
//~ rjf: String Offset Table Parsing (.debug_str_offsets)

internal U64
dw2_read_offset_table(String8 data, U64 off, DW2_OffsetTable *out)
{
  U64 start_off = off;
  {
    // rjf: read data length / format
    U64 unit_data_length = 0;
    DW_Format format = DW_Format_Null;
    off += dw2_read_initial_length(data, off, &unit_data_length, &format);
    U64 unit_data_off_opl = off + unit_data_length;
    
    // rjf: read version
    DW_Version version = DW_Version_Null;
    off += str8_deserial_read_struct(data, off, &version);
    
    // rjf: version 5: read rest (this section only exists in 5+)
    if(version == DW_Version_5)
    {
      // rjf: read address / segment selector size (these need to be 0 in non-address offset tables)
      U8 addr_size = 0;
      U8 segment_selector_size = 0;
      off += str8_deserial_read_struct(data, off, &addr_size);
      off += str8_deserial_read_struct(data, off, &segment_selector_size);
      
      // rjf: determine entry size
      U64 entry_size = dw_addr_size_from_format(format);
      if(addr_size != 0)
      {
        entry_size = addr_size + segment_selector_size;
      }
      
      // rjf: fill table info
      out->format                = format;
      out->version               = version;
      out->addr_size             = addr_size;
      out->segment_selector_size = segment_selector_size;
      out->entry_size            = entry_size;
      out->entries_count         = (unit_data_off_opl - off) / out->entry_size;
      out->entries               = data.str + off;
      
      // rjf: skip table
      off = unit_data_off_opl;
    }
  }
  U64 bytes_read = (off - start_off);
  return bytes_read;
}

internal DW2_OffsetTableList
dw2_offset_table_list_from_data(Arena *arena, String8 data)
{
  DW2_OffsetTableList list = {0};
  for(U64 off = 0; off < data.size;)
  {
    U64 start_off = off;
    DW2_OffsetTable table = {0};
    off += dw2_read_offset_table(data, off, &table);
    if(table.entries != 0)
    {
      DW2_OffsetTableNode *n = push_array(arena, DW2_OffsetTableNode, 1);
      SLLQueuePush(list.first, list.last, n);
      n->v = table;
      n->range = r1u64(start_off, off);
      list.count += 1;
    }
    if(off == start_off)
    {
      break;
    }
  }
  return list;
}

internal B32
dw2_try_offset_from_table_idx(DW2_OffsetTable *tbl, U64 idx, U64 *out)
{
  B32 result = 0;
  if(idx < tbl->entries_count)
  {
    U64 entry_size = tbl->entry_size;
    U64 entry_off = idx * entry_size;
    if(tbl->addr_size != 0)
    {
      U64 segment_off = entry_off;
      U64 addr_off = entry_off + tbl->segment_selector_size;
      U64 segment = 0;
      U64 addr = 0;
      MemoryCopy(&segment, (U8 *)tbl->entries + segment_off, tbl->segment_selector_size);
      MemoryCopy(&addr, (U8 *)tbl->entries + addr_off, tbl->addr_size);
      // TODO(rjf): @segment_based_addressing
      *out = addr;
    }
    else
    {
      MemoryCopy(out, (U8 *)tbl->entries + entry_off, entry_size);
    }
    result = 1;
  }
  return result;
}

internal DW2_OffsetTableSet
dw2_offset_table_set_from_raw(Arena *arena, DW_Raw *raw)
{
  DW2_OffsetTableSet result = {0};
  {
    struct
    {
      String8 data;
      U64 *tables_count_out;
      DW2_OffsetTable **tables_out;
      Rng1U64 **tables_ranges_out;
    }
    tasks[] =
    {
      {raw->sec[DW_SectionKind_StrOffsets].data, &result.str_offsets_tables_count, &result.str_offsets_tables, &result.str_offsets_tables_ranges},
      {raw->sec[DW_SectionKind_RngLists].data, &result.rnglists_tables_count, &result.rnglists_tables, &result.rnglists_tables_ranges},
      {raw->sec[DW_SectionKind_Addr].data, &result.addr_tables_count, &result.addr_tables, &result.addr_tables_ranges},
      {raw->sec[DW_SectionKind_LocLists].data, &result.loclists_tables_count, &result.loclists_tables, &result.loclists_tables_ranges},
    };
    for EachElement(task_idx, tasks)
    {
      Temp scratch = scratch_begin(&arena, 1);
      String8 data = tasks[task_idx].data;
      DW2_OffsetTableList tables = dw2_offset_table_list_from_data(scratch.arena, data);
      tasks[task_idx].tables_count_out[0] = tables.count;
      tasks[task_idx].tables_out[0] = push_array(arena, DW2_OffsetTable, tables.count);
      tasks[task_idx].tables_ranges_out[0] = push_array(arena, Rng1U64, tables.count);
      {
        U64 idx = 0;
        for EachNode(n, DW2_OffsetTableNode, tables.first)
        {
          tasks[task_idx].tables_out[0][idx] = n->v;
          tasks[task_idx].tables_ranges_out[0][idx] = n->range;
          idx += 1;
        }
      }
      scratch_end(scratch);
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Range List Parsing (.debug_rnglists)

internal Rng1U64List
dw2_rnglist_from_form_val(Arena *arena, DW2_ParseCtx *ctx, DW_Raw *raw, DW2_FormVal form_val)
{
  Rng1U64List result = {0};
  switch((DW_VersionEnum)ctx->version)
  {
    case DW_Version_Null:{}break;
    
    //- rjf: pre-dwarf5
    case DW_Version_1:
    case DW_Version_2:
    case DW_Version_3:
    case DW_Version_4:
    {
      String8 data = raw->sec[DW_SectionKind_Ranges].data;
      U64 ranges_off = ranges_off = form_val.u128.u64[0];;
      U64 base_addr = ctx->unit_base_addr;
      U64 sentinel = (ctx->addr_size == 4 ? max_U32 : max_U64);
      for(U64 off = ranges_off; off < data.size;)
      {
        U64 start_off = off;
        U64 range_min = 0;
        U64 range_opl = 0;
        off += str8_deserial_read(data, off, &range_min, ctx->addr_size, ctx->addr_size);
        off += str8_deserial_read(data, off, &range_opl, ctx->addr_size, ctx->addr_size);
        //
        // NOTE(rjf): interpreting tuples:
        // [0, 0) -> ending range list
        // [max_U32/U64, depending on addr size, X) -> set new base address to X
        // [N, M) -> new [base + N, base + M) range
        //
        if(range_min == 0 && range_opl == 0)
        {
          break;
        }
        else if(range_min == sentinel)
        {
          base_addr = range_opl;
        }
        else if(off == start_off)
        {
          break;
        }
        else
        {
          rng1u64_list_push(arena, &result, r1u64(base_addr + range_min, base_addr + range_opl));
        }
        if(off == start_off)
        {
          break;
        }
      }
    }break;
    
    //- rjf: @dwarf5
    case DW_Version_5:
    {
      String8 data = raw->sec[DW_SectionKind_RngLists].data;
      
      // rjf: determine section offset - sometimes this is encoded directly,
      // other times it is relative, based on the form kind - more variability
      // in this awful format
      U64 rnglist_off = 0;
      switch(form_val.kind)
      {
        default:{}break;
        case DW_FormKind_SecOffset:
        {
          rnglist_off = form_val.u128.u64[0];
        }break;
        case DW_FormKind_RngListx:
        if(ctx->rnglists_table != 0)
        {
          U64 rnglist_off_idx = form_val.u128.u64[0];
          dw2_try_offset_from_table_idx(ctx->rnglists_table, rnglist_off_idx, &rnglist_off);
        }break;
      }
      
      // rjf: in a HIGHLY UNEXPECTED TURN OF EVENTS, we now have to decode ANOTHER
      // OPCODE STREAM to unpack the actual list of ranges!!!
      U64 rle_sentinel = (ctx->addr_size == 4 ? max_U32 : max_U64);
      U64 base_addr = ctx->unit_base_addr;
      for(U64 off = rnglist_off; off < data.size;)
      {
        U64 start_off = off;
        
        // rjf: decode op kind
        DW_RLE rle_kind = DW_RLE_EndOfList;
        off += str8_deserial_read_struct(data, off, &rle_kind);
        
        // rjf: obtain range from op
        B32 good_range = 1;
        Rng1U64 range = {0};
        switch(rle_kind)
        {
          default:{good_range = 0;}break;
          case DW_RLE_BaseAddressx:
          {
            good_range = 0;
            U64 addr_idx = 0;
            off += str8_deserial_read_uleb128(data, off, &addr_idx);
            if(ctx->addr_table != 0)
            {
              U64 new_base_addr = 0;
              if(dw2_try_offset_from_table_idx(ctx->addr_table, addr_idx, &new_base_addr))
              {
                good_range = 1;
                base_addr = new_base_addr;
              }
            }
          }break;
          case DW_RLE_StartxEndx:
          {
            good_range = 0;
            U64 start_idx = 0;
            U64 end_idx = 0;
            off += str8_deserial_read_uleb128(data, off, &start_idx);
            off += str8_deserial_read_uleb128(data, off, &end_idx);
            if(ctx->addr_table != 0)
            {
              U64 start = 0;
              U64 end = 0;
              if(dw2_try_offset_from_table_idx(ctx->addr_table, start_idx, &start) &&
                 dw2_try_offset_from_table_idx(ctx->addr_table, end_idx, &end))
              {
                good_range = 1;
                range = r1u64(start, end);
              }
            }
          }break;
          case DW_RLE_StartxLength:
          {
            good_range = 0;
            U64 start_idx = 0;
            U64 length = 0;
            off += str8_deserial_read_uleb128(data, off, &start_idx);
            off += str8_deserial_read_uleb128(data, off, &length);
            if(ctx->addr_table != 0)
            {
              U64 start = 0;
              if(dw2_try_offset_from_table_idx(ctx->addr_table, start_idx, &start))
              {
                good_range = 1;
                range = r1u64(start, start+length);
              }
            }
          }break;
          case DW_RLE_OffsetPair:
          {
            U64 range_off_start = 0;
            U64 range_off_end = 0;
            off += str8_deserial_read_uleb128(data, off, &range_off_start);
            off += str8_deserial_read_uleb128(data, off, &range_off_end);
            range = r1u64(base_addr + range_off_start, base_addr + range_off_end);
          }break;
          case DW_RLE_BaseAddress:
          {
            U64 new_base_addr = 0;
            off += str8_deserial_read(data, off, &new_base_addr, ctx->addr_size, ctx->addr_size);
            base_addr = new_base_addr;
          }break;
          case DW_RLE_StartEnd:
          {
            U64 start = 0;
            U64 end = 0;
            off += str8_deserial_read(data, off, &start, ctx->addr_size, ctx->addr_size);
            off += str8_deserial_read(data, off, &end, ctx->addr_size, ctx->addr_size);
            range = r1u64(start, end);
          }break;
          case DW_RLE_StartLength:
          {
            U64 start = 0;
            U64 length = 0;
            off += str8_deserial_read(data, off, &start, ctx->addr_size, ctx->addr_size);
            off += str8_deserial_read_uleb128(data, off, &length);
            range = r1u64(start, start + length);
          }break;
        }
        
        // rjf: add range
        if(good_range)
        {
          rng1u64_list_push(arena, &result, range);
        }
        
        // rjf: end if no movement or end-of-list code
        if(off == start_off || rle_kind == DW_RLE_EndOfList)
        {
          break;
        }
      }
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Location List Parsing (.debug_loclists)

internal DW2_LocList
dw2_loclist_from_form_val(Arena *arena, DW2_ParseCtx *ctx, DW_Raw *raw, DW2_FormVal form_val)
{
  DW2_LocList result = {0};
  switch((DW_VersionEnum)ctx->version)
  {
    case DW_Version_Null:{}break;
    
    //- rjf: pre-dwarf5
    case DW_Version_1:
    case DW_Version_2:
    case DW_Version_3:
    case DW_Version_4:
    {
      String8 data = raw->sec[DW_SectionKind_Loc].data;
      U64 locs_off = locs_off = form_val.u128.u64[0];
      U64 base_addr = ctx->unit_base_addr;
      U64 sentinel = (ctx->addr_size == 4 ? max_U32 : max_U64);
      for(U64 off = locs_off; off < data.size;)
      {
        U64 start_off = off;
        U64 range_min = 0;
        U64 range_opl = 0;
        off += str8_deserial_read(data, off, &range_min, ctx->addr_size, ctx->addr_size);
        off += str8_deserial_read(data, off, &range_opl, ctx->addr_size, ctx->addr_size);
        //
        // NOTE(rjf): interpreting tuples:
        // [0, 0) -> ending range list
        // [max_U32/U64, depending on addr size, X) -> set new base address to X
        // [N, M) -> new [base + N, base + M) range
        //
        if(range_min == 0 && range_opl == 0)
        {
          break;
        }
        else if(range_min == sentinel)
        {
          base_addr = range_opl;
        }
        else if(off == start_off)
        {
          break;
        }
        else
        {
          U16 expr_size = 0;
          off += str8_deserial_read_struct(data, off, &expr_size);
          DW2_LocNode *n = push_array(arena, DW2_LocNode, 1);
          n->v.range = r1u64(range_min + base_addr, range_opl + base_addr);
          n->v.expr = str8_substr(data, r1u64(off, off+expr_size));
          SLLQueuePush(result.first, result.last, n);
          result.count += 1;
          off += expr_size;
        }
        if(off == start_off)
        {
          break;
        }
      }
    }break;
    
    //- rjf: @dwarf5
    case DW_Version_5:
    {
      String8 data = raw->sec[DW_SectionKind_LocLists].data;
      
      // rjf: determine section offset - sometimes this is encoded directly,
      // other times it is relative, based on the form kind - more variability
      // in this awful format
      U64 loclist_off = 0;
      switch(form_val.kind)
      {
        default:{}break;
        case DW_FormKind_SecOffset:
        {
          loclist_off = form_val.u128.u64[0];
        }break;
        case DW_FormKind_LocListx:
        if(ctx->rnglists_table != 0)
        {
          U64 loclist_off_idx = form_val.u128.u64[0];
          dw2_try_offset_from_table_idx(ctx->loclists_table, loclist_off_idx, &loclist_off);
        }break;
      }
      
      // rjf: in a HIGHLY UNEXPECTED TURN OF EVENTS, we now have to decode ANOTHER
      // OPCODE STREAM to unpack the actual list of locations!!!
      U64 rle_sentinel = (ctx->addr_size == 4 ? max_U32 : max_U64);
      U64 base_addr = ctx->unit_base_addr;
      for(U64 off = loclist_off; off < data.size;)
      {
        U64 start_off = off;
        
        // rjf: decode op kind
        DW_LLE lle_kind = DW_LLE_EndOfList;
        off += str8_deserial_read_struct(data, off, &lle_kind);
        
        // rjf: obtain range from op
        B32 good_loc = 1;
        Rng1U64 range = {0};
        switch(lle_kind)
        {
          default:{good_loc = 0;}break;
          case DW_LLE_BaseAddressx:
          {
            good_loc = 0;
            U64 addr_idx = 0;
            off += str8_deserial_read_uleb128(data, off, &addr_idx);
            if(ctx->addr_table != 0)
            {
              U64 new_base_addr = 0;
              if(dw2_try_offset_from_table_idx(ctx->addr_table, addr_idx, &new_base_addr))
              {
                good_loc = 1;
                base_addr = new_base_addr;
              }
            }
          }break;
          case DW_LLE_StartxEndx:
          {
            good_loc = 0;
            U64 start_idx = 0;
            U64 end_idx = 0;
            off += str8_deserial_read_uleb128(data, off, &start_idx);
            off += str8_deserial_read_uleb128(data, off, &end_idx);
            if(ctx->addr_table != 0)
            {
              U64 start = 0;
              U64 end = 0;
              if(dw2_try_offset_from_table_idx(ctx->addr_table, start_idx, &start) &&
                 dw2_try_offset_from_table_idx(ctx->addr_table, end_idx, &end))
              {
                good_loc = 1;
                range = r1u64(start, end);
              }
            }
          }break;
          case DW_LLE_StartxLength:
          {
            good_loc = 0;
            U64 start_idx = 0;
            U64 length = 0;
            off += str8_deserial_read_uleb128(data, off, &start_idx);
            off += str8_deserial_read_uleb128(data, off, &length);
            if(ctx->addr_table != 0)
            {
              U64 start = 0;
              if(dw2_try_offset_from_table_idx(ctx->addr_table, start_idx, &start))
              {
                good_loc = 1;
                range = r1u64(start, start+length);
              }
            }
          }break;
          case DW_LLE_OffsetPair:
          {
            U64 range_off_start = 0;
            U64 range_off_end = 0;
            off += str8_deserial_read_uleb128(data, off, &range_off_start);
            off += str8_deserial_read_uleb128(data, off, &range_off_end);
            range = r1u64(base_addr + range_off_start, base_addr + range_off_end);
          }break;
          case DW_LLE_DefaultLocation:
          {
            range = r1u64(0, max_U64);
          }break;
          case DW_LLE_BaseAddress:
          {
            U64 new_base_addr = 0;
            off += str8_deserial_read(data, off, &new_base_addr, ctx->addr_size, ctx->addr_size);
            base_addr = new_base_addr;
          }break;
          case DW_LLE_StartEnd:
          {
            U64 start = 0;
            U64 end = 0;
            off += str8_deserial_read(data, off, &start, ctx->addr_size, ctx->addr_size);
            off += str8_deserial_read(data, off, &end, ctx->addr_size, ctx->addr_size);
            range = r1u64(start, end);
          }break;
          case DW_LLE_StartLength:
          {
            U64 start = 0;
            U64 length = 0;
            off += str8_deserial_read(data, off, &start, ctx->addr_size, ctx->addr_size);
            off += str8_deserial_read_uleb128(data, off, &length);
            range = r1u64(start, start + length);
          }break;
        }
        
        // rjf: read expression
        String8 expr = {0};
        if(good_loc && lle_kind != DW_LLE_BaseAddress && lle_kind != DW_LLE_BaseAddressx)
        {
          U64 expr_size = 0;
          off += str8_deserial_read_uleb128(data, off, &expr_size);
          expr = str8_substr(data, r1u64(off, off+expr_size));
          off += expr_size;
        }
        
        // rjf: add range
        if(good_loc)
        {
          DW2_LocNode *n = push_array(arena, DW2_LocNode, 1);
          n->v.range = range;
          n->v.expr = expr;
          SLLQueuePush(result.first, result.last, n);
          result.count += 1;
        }
        
        // rjf: end if no movement or end-of-list code
        if(off == start_off || lle_kind == DW_LLE_EndOfList)
        {
          break;
        }
      }
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Unwind Info Parsing (.debug_frame)

internal U64
dw2_read_cfi_header(String8 data, U64 off, DW_CFIHeader *cfi_header_out)
{
  U64 start_off = off;
  
  // rjf: read length / format
  U64 length = 0;
  DW_Format fmt = DW_Format_Null;
  U64 length_size = dw2_read_initial_length(data, off, &length, &fmt);
  
  // rjf: find entry info, read ID
  Rng1U64 entry_range = r1u64(off, off + length_size + length);
  String8 entry_data  = str8_substr(data, entry_range);
  U64 id = 0;
  U64 id_size = dw2_read_fmt_u64(entry_data, length_size, fmt, &id);
  
  // rjf: fill
  cfi_header_out->kind            = ((fmt == DW_Format_32Bit && id == max_U32) || (fmt == DW_Format_64Bit && id == max_U64)) ? DW_CFIKind_CIE : DW_CFIKind_FDE;
  cfi_header_out->fmt             = fmt;
  cfi_header_out->entry_range     = entry_range;
  cfi_header_out->cie_pointer     = id;
  cfi_header_out->cie_pointer_off = off + length_size;
  
  U64 bytes_read = (off - start_off);
  return bytes_read;
}

internal U64
dw2_read_cie(String8 data, U64 off, DW_Format fmt, Arch arch, DW_CIE *cie_out)
{
  U64 start_off = off;
  
  // rjf: read id
  U64 id = 0;
  off += dw2_read_fmt_u64(data, off, fmt, &id);
  
  // rjf: read version
  U8 version = 0;
  off += str8_deserial_read_struct(data, off, &version);
  
  // rjf: read aug string
  String8 aug_string = {0};
  U64 aug_string_off = off;
  off += str8_deserial_read_cstr(data, off, &aug_string);
  U64 aug_string_off_opl = off;
  
  // rjf: read address/segment-selector size
  U8 address_size = 0;
  U8 segment_selector_size = 0;
  if(version >= DW_Version_4)
  {
    off += str8_deserial_read_struct(data, off, &address_size);
    off += str8_deserial_read_struct(data, off, &segment_selector_size);
  }
  else
  {
    address_size = byte_size_from_arch(arch);
  }
  
  // rjf: read alignments
  U64 code_align_factor = 0;
  U64 data_align_factor = 0;
  off += str8_deserial_read_uleb128(data, off, &code_align_factor);
  off += str8_deserial_read_uleb128(data, off, &data_align_factor);
  
  // rjf: read return address register encoding
  U64 ret_addr_reg = 0;
  switch(version)
  {
    case DW_Version_1:
    {
      off += str8_deserial_read(data, off, &ret_addr_reg, 1, 1);
    }break;
    default:
    {
      off += str8_deserial_read_uleb128(data, off, &ret_addr_reg);
    }break;
  }
  
  // rjf: fill output
  {
    cie_out->aug_string_range      = r1u64(aug_string_off, aug_string_off_opl);
    cie_out->code_align_factor     = code_align_factor;
    cie_out->data_align_factor     = data_align_factor;
    cie_out->ret_addr_reg          = ret_addr_reg;
    cie_out->format                = fmt;
    cie_out->version               = version;
    cie_out->address_size          = address_size;
    cie_out->segment_selector_size = segment_selector_size;
  }
  
  U64 bytes_read = (off - start_off);
  return bytes_read;
}

internal U64
dw2_read_fde(String8 data, U64 off, DW_Format fmt, Arch arch, DW_CIE *cie, DW_FDE *fde_out)
{
  U64 start_off = off;
  
  // rjf: skip format-dependent prefix
  off += (fmt == DW_Format_32Bit) ? 4 : 12;
  
  // rjf: read cie ptr
  U64 cie_ptr = 0;
  off += dw2_read_fmt_u64(data, off, fmt, &cie_ptr);
  
  // rjf: read address of first instruction
  U64 pc_begin = 0;
  off += str8_deserial_read(data, off, &pc_begin, cie->address_size, cie->address_size);
  
  // rjf: read instruction range size
  U64 pc_range_size = 0;
  off += str8_deserial_read(data, off, &pc_range_size, cie->address_size, cie->address_size);
  
  // rjf: skip aug data
  off += dim_1u64(cie->aug_string_range);
  
  // rjf: fill output
  fde_out->cie_pointer = cie_ptr;
  fde_out->pc_range = r1u64(pc_begin, pc_begin + pc_range_size);
  
  U64 bytes_read = (off - start_off);
  return bytes_read;
}

internal B32
dw2_try_cfi_from_data(String8 data, U64 off, Arch arch, DW_CIE *cie_out, DW_FDE *fde_out)
{
  B32 result = 0;
  {
    DW_CFIHeader fde_header = {0};
    off += dw2_read_cfi_header(data, off, &fde_header);
    if(fde_header.kind == DW_CFIKind_FDE)
    {
      DW_CFIHeader cie_header = {0};
      dw2_read_cfi_header(data, fde_header.cie_pointer, &cie_header);
      if(cie_header.kind == DW_CFIKind_CIE)
      {
        result = 1;
        dw2_read_cie(data, cie_header.entry_range.min, cie_header.fmt, arch, cie_out);
        dw2_read_fde(data, fde_header.entry_range.min, fde_header.fmt, arch, cie_out, fde_out);
      }
    }
  }
  return result;
}
