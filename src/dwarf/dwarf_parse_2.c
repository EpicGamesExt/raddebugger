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
      
      // rjf: read tag abbrev
      U64 id = 0;
      U64 tag_kind = 0;
      U8 has_children = 0;
      {
        read_off += str8_deserial_read_uleb128(data, read_off, &id);
        if(id != 0)
        {
          read_off += str8_deserial_read_uleb128(data, read_off, &tag_kind);
          read_off += str8_deserial_read_struct(data, read_off, &has_children);
        }
      }
      
      // rjf: count tag
      if(id != 0)
      {
        tag_count += 1;
      }
      
      // rjf: read all tag attribute abbreviations
      if(id != 0)
      {
        for(;;)
        {
          U64 attrib_start_off = read_off;
          U64 attrib_kind = 0;
          U64 attrib_form_kind = 0;
          U64 implicit_const = 0;
          read_off += str8_deserial_read_uleb128(data, read_off, &attrib_kind);
          read_off += str8_deserial_read_uleb128(data, read_off, &attrib_form_kind);
          if(attrib_form_kind == DW_Form_ImplicitConst)
          {
            read_off += str8_deserial_read_uleb128(data, read_off, &implicit_const);
          }
          if(read_off == attrib_start_off || attrib_kind == 0)
          {
            break;
          }
        }
      }
      
      // rjf: if building (second loop) -> insert into map
      if(build)
      {
        U64 hash = u64_hash_from_str8(str8_struct(&id));
        U64 slot_idx = hash%map.slots_count;
        DW2_AbbrevMapNode *n = push_array(arena, DW2_AbbrevMapNode, 1);
        SLLStackPush(map.slots[slot_idx], n);
        n->id  = id;
        n->off = start_off;
      }
      
      // rjf: no tag ID, or no movement? -> exit
      if(read_off == start_off || id == 0)
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

////////////////////////////////
//~ rjf: Tag Parsing

internal U64
dw2_read_tag(Arena *arena, DW2_TagParseCtx *ctx, String8 data, U64 off, DW2_Tag *tag_out)
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
    String8 abbrev_data = ctx->abbrev_data;
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
    String8 abbrev_data = ctx->abbrev_data;
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
      if(attrib_form_kind == DW_Form_ImplicitConst)
      {
        abbrev_read_off += str8_deserial_read_uleb128(abbrev_data, abbrev_read_off, &implicit_const);
      }
      
      // rjf: indirect form -> form kind is encoded in .debug_info (because why not)
      if(attrib_form_kind == DW_Form_Indirect)
      {
        off += str8_deserial_read_uleb128(data, off, &attrib_form_kind);
      }
      
      // rjf: read attribute's value from .debug_info
      DW2_AttribVal val = {0};
      if(attrib_kind != 0)
      {
        U64 bytes_to_read = 0;
        switch(attrib_form_kind)
        {
          //- rjf: no-ops
          default:
          case DW_Form_Indirect:
          case DW_Form_Null:
          {}break;
          
          //- rjf: 1-byte uint reads
          case DW_Form_Ref1:
          case DW_Form_Data1:
          case DW_Form_Flag:
          case DW_Form_Strx1:
          case DW_Form_Addrx1:
          bytes_to_read = 1; goto read_fixed_uint;
          
          //- rjf: 2-byte uint reads
          case DW_Form_Ref2:
          case DW_Form_Data2:
          case DW_Form_Strx2:
          case DW_Form_Addrx2:
          bytes_to_read = 2; goto read_fixed_uint;
          
          //- rjf: 3-byte uint reads
          case DW_Form_Strx3:
          case DW_Form_Addrx3:
          bytes_to_read = 3; goto read_fixed_uint;
          
          //- rjf: 4-byte uint reads
          case DW_Form_Data4:
          case DW_Form_Ref4:
          case DW_Form_RefSup4:
          case DW_Form_Strx4:
          case DW_Form_Addrx4:
          bytes_to_read = 4; goto read_fixed_uint;
          
          //- rjf: 8-byte uint reads
          case DW_Form_Data8:
          case DW_Form_Ref8:
          case DW_Form_RefSig8:
          case DW_Form_RefSup8:
          bytes_to_read = 8; goto read_fixed_uint;
          
          //- rjf: 16-byte uint reads
          case DW_Form_Data16:
          bytes_to_read = 16; goto read_fixed_uint;
          
          //- rjf: address-sized reads
          case DW_Form_Addr:
          bytes_to_read = ctx->addr_size; goto read_fixed_uint;
          
          //- rjf: format-defined-size reads
          case DW_Form_RefAddr:
          case DW_Form_SecOffset:
          case DW_Form_LineStrp:
          case DW_Form_Strp:
          case DW_Form_StrpSup:
          bytes_to_read = dw_size_from_format(ctx->format); goto read_fixed_uint;
          
          //- rjf: fixed-size uint reads
          read_fixed_uint:
          {
            off += str8_deserial_read(data, off, &val.u128.u64[0], bytes_to_read, bytes_to_read);
          }break;
          
          //- rjf: uleb128 reads
          case DW_Form_UData:
          case DW_Form_RefUData:
          case DW_Form_Strx:
          case DW_Form_Addrx:
          case DW_Form_LocListx:
          case DW_Form_RngListx:
          {
            off += str8_deserial_read_uleb128(data, off, &val.u128.u64[0]);
          }break;
          
          //- rjf: sleb128 reads
          case DW_Form_SData:
          {
            off += str8_deserial_read_sleb128(data, off, &val.u128.u64[0]);
          }break;
          
          //- rjf: fixed-size uint reads / skips
          case DW_Form_Block1: bytes_to_read = 1; goto read_fixed_uint_and_skip;
          case DW_Form_Block2: bytes_to_read = 2; goto read_fixed_uint_and_skip;
          case DW_Form_Block4: bytes_to_read = 4; goto read_fixed_uint_and_skip;
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
          case DW_Form_Block:
          {
            U64 size = 0;
            U64 og_read_off = off;
            off += str8_deserial_read_uleb128(data, off, &size);
            val.u128.u64[0] = size;
            val.u128.u64[1] = og_read_off;
            off += size;
          }break;
          
          //- rjf: strings
          case DW_Form_String:
          {
            String8 string = {0};
            U64 og_read_off = off;
            off += str8_deserial_read_cstr(data, off, &string);
            val.u128.u64[0] = og_read_off;
            val.u128.u64[1] = og_read_off + string.size - 1;
          }break;
          
          //- rjf: implicit constants (no reading in .debug_info; value comes from .debug_abbrev)
          case DW_Form_ImplicitConst:
          {
            val.u128.u64[0] = implicit_const;
          }break;
          
          //- rjf: exprloc
          case DW_Form_ExprLoc:
          {
            U64 size = 0;
            U64 og_read_off = off;
            U64 bytes_read = str8_deserial_read_uleb128(data, off, &size);
            val.u128.u64[0] = og_read_off + bytes_read;
            val.u128.u64[1] = size;
            off += bytes_read;
          }break;
          
          //- rjf: flag present
          case DW_Form_FlagPresent:
          {
            val.u128.u64[0] = 1;
          }break;
        }
      }
      
      // rjf: push
      if(attrib_kind != 0)
      {
        DW2_AttribNode *n = push_array(arena, DW2_AttribNode, 1);
        SLLQueuePush(attribs.first, attribs.last, n);
        attribs.count += 1;
        n->v.attrib_kind = attrib_kind;
        n->v.form_kind   = attrib_form_kind;
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
