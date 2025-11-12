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
//~ rjf: Form Value Parsing

internal U64
dw2_read_form_val(DW2_ParseCtx *ctx, String8 data, U64 off, DW_FormKind form_kind, U64 implicit_const, DW2_FormVal *out)
{
  U64 start_off = off;
  DW2_FormVal val = {0};
  {
    U64 bytes_to_read = 0;
    switch(form_kind)
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
  *out = val;
  U64 bytes_read = (off - start_off);
  return bytes_read;
}

internal String8
dw2_string_from_form_val(DW2_ParseCtx *ctx, DW2_FormVal val)
{
  String8 section_data = ctx->raw->sec[val.section_kind].data;
  Rng1U64 range = r1u64(val.u128.u64[0], val.u128.u64[1]);
  String8 string = str8_substr(section_data, range);
  return string;
}

////////////////////////////////
//~ rjf: Tag Parsing

internal U64
dw2_read_tag(Arena *arena, DW2_ParseCtx *ctx, String8 data, U64 off, DW2_Tag *tag_out)
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
    String8 abbrev_data = ctx->raw->sec[DW_Section_Abbrev].data;
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
    String8 abbrev_data = ctx->raw->sec[DW_Section_Abbrev].data;
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
      DW2_FormVal val = {0};
      if(attrib_kind != 0)
      {
        off += dw2_read_form_val(ctx, data, off, attrib_form_kind, implicit_const, &val);
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

////////////////////////////////
//~ rjf: Line Table Parsing

internal U64
dw2_read_line_table_header(Arena *arena, DW2_ParseCtx *ctx, String8 data, U64 off, DW2_LineTableHeader *out)
{
  Temp scratch = scratch_begin(&arena, 1);
  U64 start_off = 0;
  
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
  U64 header_length = 0;
  U8 min_inst_length = 0;
  U8 max_ops_per_inst = 1;
  U8 default_is_stmt = 0;
  S8 line_base = 0;
  U8 line_range = 0;
  U8 opcode_base = 0;
  {
    off += dw2_read_fmt_u64(data, off, format, &header_length);
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
      for(B32 files = 0, dirs = 1; files <= 1; files = 1, dirs = 0)
      {
        // rjf: read header
        U8 entry_formats_count = 0;
        off += str8_deserial_read_struct(data, off, &entry_formats_count);
        U64 *entry_formats = push_array(scratch.arena, U64, entry_formats_count*2);
        for EachIndex(idx, entry_formats_count)
        {
          off += str8_deserial_read_uleb128(data, off, &entry_formats[idx]);
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
              off += dw2_read_form_val(ctx, data, off, form_kind, 0, &form_val);
              switch(lnct)
              {
                default:
                {
                  log_infof("DWARF LNCT encoding not currently supported (0x%I64x).\n", (U64)lnct);
                }break;
                case DW_LNCT_Path:
                {
                  entry_out->file_name = dw2_string_from_form_val(ctx, form_val);
                }break;
                case DW_LNCT_DirectoryIndex:
                {
                  entry_out->dir_idx = form_val.u128.u64[0];
                }break;
                case DW_LNCT_TimeStamp:
                {
                  entry_out->modify_time = form_val.u128.u64[0];
                }break;
                case DW_LNCT_Size:
                {
                  entry_out->file_size = form_val.u128.u64[0];
                }break;
                case DW_LNCT_MD5:
                {
                  entry_out->md5.u128 = form_val.u128;
                }break;
                case DW_LNCT_LLVM_Source:
                {
                  entry_out->source = dw2_string_from_form_val(ctx, form_val);
                }break;
              }
            }
          }
        }
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
  }
  
  U64 bytes_read = (off - start_off);
  scratch_end(scratch);
  return bytes_read;
}
