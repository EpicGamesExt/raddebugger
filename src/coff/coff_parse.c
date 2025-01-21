// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
coff_is_big_obj(String8 raw_coff)
{
  B32 is_big_obj = 0;
  if (raw_coff.size >= sizeof(COFF_BigObjHeader)) {
    COFF_BigObjHeader *file_header32 = (COFF_BigObjHeader*)(raw_coff.str);
    is_big_obj = file_header32->sig1 == COFF_Machine_Unknown && 
                 file_header32->sig2 == max_U16 &&
                 file_header32->version >= 2 &&
                 MemoryCompare(file_header32->magic, g_coff_big_header_magic, sizeof(file_header32->magic)) == 0;
  }
  return is_big_obj;
}

internal B32
coff_is_obj(String8 raw_coff)
{
  B32 is_obj = 0;
  
  if (raw_coff.size >= sizeof(COFF_FileHeader)) {
    COFF_FileHeader *header = (COFF_FileHeader*)(raw_coff.str);
    
    // validate machine
    B32 is_machine_type_valid = 0;
    switch (header->machine) {
      case COFF_Machine_Unknown:
      case COFF_Machine_X86:    case COFF_Machine_X64:
      case COFF_Machine_Am33:   case COFF_Machine_Arm:
      case COFF_Machine_Arm64:  case COFF_Machine_ArmNt:
      case COFF_Machine_Ebc:    case COFF_Machine_Ia64:
      case COFF_Machine_M32R:   case COFF_Machine_Mips16:
      case COFF_Machine_MipsFpu:case COFF_Machine_MipsFpu16:
      case COFF_Machine_PowerPc:case COFF_Machine_PowerPcFp:
      case COFF_Machine_R4000:  case COFF_Machine_RiscV32:
      case COFF_Machine_RiscV64:case COFF_Machine_RiscV128:
      case COFF_Machine_Sh3:    case COFF_Machine_Sh3Dsp:
      case COFF_Machine_Sh4:    case COFF_Machine_Sh5:
      case COFF_Machine_Thumb:  case COFF_Machine_WceMipsV2:
      {
        is_machine_type_valid = 1;
      }break;
    }
    
    if (is_machine_type_valid) {
      // validate section count
      U64 section_count = header->section_count;
      U64 section_hdr_opl_off = sizeof(*header) + section_count*sizeof(COFF_SectionHeader);
      if (raw_coff.size >= section_hdr_opl_off) {
        
        COFF_SectionHeader *section_hdrs = (COFF_SectionHeader*)(raw_coff.str + sizeof(*header));
        COFF_SectionHeader *section_hdr_opl = section_hdrs + section_count;
        
        // validate section ranges
        B32 is_sect_range_valid = 1;
        for (COFF_SectionHeader *sec_hdr = section_hdrs;
             sec_hdr < section_hdr_opl;
             sec_hdr += 1) {
          if (!(sec_hdr->flags & COFF_SectionFlag_CntUninitializedData)) {
            U64 min = sec_hdr->foff;
            U64 max = min + sec_hdr->fsize;
            if (sec_hdr->fsize > 0 && !(section_hdr_opl_off <= min && min <= max && max <= raw_coff.size)) {
              is_sect_range_valid = 0;
              break;
            }
          }
        }
        
        if (is_sect_range_valid) {
          // validate symbol table
          U64 symbol_table_off = header->symbol_table_foff;
          U64 symbol_table_size = sizeof(COFF_Symbol16)*header->symbol_count;
          U64 symbol_table_opl_off = symbol_table_off + symbol_table_size;
          
          // don't validate symbol table when there is none
          if (symbol_table_off == 0 && symbol_table_size == 0) {
            symbol_table_off = section_hdr_opl_off;
            symbol_table_opl_off = section_hdr_opl_off;
          }
          
          is_obj = (section_hdr_opl_off <= symbol_table_off &&
                    symbol_table_off <= symbol_table_opl_off &&
                    symbol_table_opl_off <= raw_coff.size);
        }
      }
    }
  }
  
  return is_obj;
}

internal COFF_FileHeaderInfo
coff_file_header_info_from_data(String8 raw_coff)
{
  COFF_FileHeaderInfo info = {0};
  if (coff_is_big_obj(raw_coff)) {
    COFF_BigObjHeader *header32 = (COFF_BigObjHeader*)raw_coff.str;
    info.is_big_obj             = 1;
    info.machine                = header32->machine;
    info.header_size            = sizeof(COFF_BigObjHeader);
    info.section_array_off      = sizeof(COFF_BigObjHeader);
    info.section_count_no_null  = header32->section_count;
    info.string_table_off       = header32->symbol_table_foff + sizeof(COFF_Symbol32) * header32->symbol_count;
    info.symbol_size            = sizeof(COFF_Symbol32);
    info.symbol_off             = header32->symbol_table_foff;
    info.symbol_count           = header32->symbol_count;
  } else if (coff_is_obj(raw_coff)) {
    COFF_FileHeader *header16  = (COFF_FileHeader*)raw_coff.str;
    info.is_big_obj            = 0;
    info.machine               = header16->machine;
    info.header_size           = sizeof(COFF_FileHeader);
    info.section_array_off     = sizeof(COFF_FileHeader);
    info.section_count_no_null = header16->section_count;
    info.string_table_off      = header16->symbol_table_foff + sizeof(COFF_Symbol16) * header16->symbol_count;
    info.symbol_size           = sizeof(COFF_Symbol16);
    info.symbol_off            = header16->symbol_table_foff;
    info.symbol_count          = header16->symbol_count;
  }
  return info;
}

internal COFF_ParsedSymbol
coff_parse_symbol32(String8 raw_coff, U64 string_table_off, COFF_Symbol32 *sym32)
{
  COFF_ParsedSymbol result = {0};
  result.name              = coff_read_symbol_name(raw_coff, string_table_off, &sym32->name);
  result.value             = sym32->value;
  result.section_number    = sym32->section_number;
  result.type              = sym32->type;
  result.storage_class     = sym32->storage_class;
  result.aux_symbol_count  = sym32->aux_symbol_count;
  return result;
}

internal COFF_ParsedSymbol
coff_parse_symbol16(String8 raw_coff, U64 string_table_off, COFF_Symbol16 *sym16)
{
  COFF_ParsedSymbol result = {0};
  result.name              = coff_read_symbol_name(raw_coff, string_table_off, &sym16->name);
  result.value             = sym16->value;
  if (sym16->section_number == COFF_Symbol_DebugSection16) {
    result.section_number = COFF_Symbol_DebugSection32;
  } else if (sym16->section_number == COFF_Symbol_AbsSection16) {
    result.section_number = COFF_Symbol_AbsSection32;
  } else {
    result.section_number = (U32)sym16->section_number;
  }
  result.type             = sym16->type;
  result.storage_class    = sym16->storage_class;
  result.aux_symbol_count = sym16->aux_symbol_count;
  return result;
}

internal COFF_Symbol32Array
coff_symbol_array_from_data_16(Arena *arena, String8 raw_coff, U64 symbol_array_off, U64 symbol_count)
{
  COFF_Symbol32Array result = {0};
  result.count              = symbol_count;
  result.v                  = push_array_no_zero_aligned(arena, COFF_Symbol32, result.count, 8);
  
  Rng1U64        sym16_arr_range = rng_1u64(symbol_array_off, symbol_array_off + sizeof(COFF_Symbol16) * symbol_count);
  String8        raw_sym16_arr   = str8_substr(raw_coff, sym16_arr_range);
  COFF_Symbol16 *sym16_arr       = (COFF_Symbol16 *)raw_sym16_arr.str;

  for (U64 isymbol = 0, count = raw_sym16_arr.size / sizeof(COFF_Symbol16); isymbol < count; isymbol += 1) {
    COFF_Symbol16 *sym16 = &sym16_arr[isymbol];
    COFF_Symbol32 *sym32 = &result.v[isymbol];

    sym32->name             = sym16->name;
    sym32->value            = sym16->value;
    if (sym16->section_number == COFF_Symbol_DebugSection16) {
      sym32->section_number = COFF_Symbol_DebugSection32;
    } else if (sym16->section_number == COFF_Symbol_AbsSection16) {
      sym32->section_number = COFF_Symbol_AbsSection32;
    } else {
      sym32->section_number = (U32)sym16->section_number;
    }
    sym32->type.v           = sym16->type.v;
    sym32->storage_class    = sym16->storage_class;
    sym32->aux_symbol_count = sym16->aux_symbol_count;
    
    // copy aux symbols
    for (U64 iaux = isymbol+1, iaux_hi = Min(count, iaux+sym16->aux_symbol_count); iaux < iaux_hi; iaux += 1) {
      COFF_Symbol16 *aux16 = sym16_arr + iaux;
      COFF_Symbol32 *aux32 = result.v  + iaux;

      // 32bit COFF uses 16bit aux symbols
      MemoryCopy(aux32, aux16, sizeof(COFF_Symbol16));
      MemoryZero((U8 *)aux32 + sizeof(COFF_Symbol16), sizeof(COFF_Symbol32)-sizeof(COFF_Symbol16));
    }
    
    // take into account aux symbols
    isymbol += sym32->aux_symbol_count;
  }
  
  return result;
}

internal COFF_Symbol32Array
coff_symbol_array_from_data_32(Arena *arena, String8 data, U64 symbol_array_off, U64 symbol_count)
{
  COFF_Symbol32Array result;
  result.count = symbol_count;
  result.v     = (COFF_Symbol32 *)(data.str + symbol_array_off);
  return result;
}

internal COFF_Symbol32Array
coff_symbol_array_from_data(Arena *arena, String8 data, U64 symbol_array_off, U64 symbol_count, U64 symbol_size)
{
  COFF_Symbol32Array result = {0};
  switch (symbol_size) {
    case sizeof(COFF_Symbol16): result = coff_symbol_array_from_data_16(arena, data, symbol_array_off, symbol_count); break;
    case sizeof(COFF_Symbol32): result = coff_symbol_array_from_data_32(arena, data, symbol_array_off, symbol_count); break;
  }
  return result;
}

internal COFF_Symbol16Node *
coff_symbol16_list_push(Arena *arena, COFF_Symbol16List *list, COFF_Symbol16 symbol)
{
  COFF_Symbol16Node *node = push_array(arena, COFF_Symbol16Node, 1);
  node->next = 0;
  node->data = symbol;
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
  return node;
}

internal COFF_SymbolValueInterpType
coff_interp_symbol(U32 section_number, U32 value, COFF_SymStorageClass storage_class)
{
  if (storage_class == COFF_SymStorageClass_Section && section_number == COFF_Symbol_UndefinedSection) {
    return COFF_SymbolValueInterp_Undefined;
  }
  if (storage_class == COFF_SymStorageClass_External && value == 0 && section_number == COFF_Symbol_UndefinedSection) {
    return COFF_SymbolValueInterp_Undefined;
  }
  if (storage_class == COFF_SymStorageClass_External && value != 0 && section_number == COFF_Symbol_UndefinedSection) {
    return COFF_SymbolValueInterp_Common;
  }
  if (section_number == COFF_Symbol_AbsSection32) {
    return COFF_SymbolValueInterp_Abs;
  }
  if (section_number == COFF_Symbol_DebugSection32) {
    return COFF_SymbolValueInterp_Debug;
  }
  if (storage_class == COFF_SymStorageClass_WeakExternal) {
    return COFF_SymbolValueInterp_Weak;
  }
  return COFF_SymbolValueInterp_Regular;
}

internal COFF_RelocNode *
coff_reloc_list_push(Arena *arena, COFF_RelocList *list, COFF_Reloc reloc)
{
  COFF_RelocNode *node = push_array(arena, COFF_RelocNode, 1);
  node->data = reloc;
  SLLQueuePush(list->first, list->last, node);
  ++list->count;
  return node;
}

internal COFF_RelocInfo
coff_reloc_info_from_section_header(String8 data, COFF_SectionHeader *header)
{
  COFF_RelocInfo result = {0};
  if (header->flags & COFF_SectionFlag_LnkNRelocOvfl && header->reloc_count == max_U16) {
    COFF_Reloc counter;
    U64 read_size = str8_deserial_read_struct(data, header->relocs_foff, &counter);
    if (read_size == sizeof(counter) && counter.apply_off > 0) {
      result.array_off = header->relocs_foff + sizeof(COFF_Reloc);
      result.count     = counter.apply_off - 1; // exclude counter entry
    }
  } else {
    result.array_off = header->relocs_foff;
    result.count     = header->reloc_count;
  }
  return result;
}

internal String8
coff_resource_string_from_str16(Arena *arena, String16 string)
{
  AssertAlways(string.size <= max_U16);
  U16 size16 = (U16)string.size;

  U16 *buffer = push_array_no_zero(arena, U16, size16 + 1);
  MemoryCopy(buffer + 0, &size16,    sizeof(size16));
  MemoryCopy(buffer + 1, string.str, size16 * sizeof(string.str[0]));

  return str8_array(buffer, size16 + 1);
}

internal String8
coff_resource_string_from_str8(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String16 string16 = str16_from_8(scratch.arena, string);
  String8  result   = coff_resource_string_from_str16(arena, string16);
  scratch_end(scratch);
  return result;
}

internal String8
coff_resource_number_from_u16(Arena *arena, U16 number)
{
  U16 *buffer = push_array_no_zero(arena, U16, 2);
  buffer[0] = max_U16;
  buffer[1] = number;
  return str8_array(buffer, 2);
}

internal COFF_ResourceID
coff_utf8_resource_id_from_utf16(Arena *arena, COFF_ResourceID16 *id_16)
{
  COFF_ResourceID id = {0};
  id.type = id_16->type;
  switch (id_16->type) {
    case COFF_ResourceIDType_Null: break;
    case COFF_ResourceIDType_Number: {
      id.u.number = id_16->u.number;
    } break;
    case COFF_ResourceIDType_String: {
      id.u.string = str8_from_16(arena, id_16->u.string);
    } break;
    default: InvalidPath;
  }
  return id;
}

internal U64
coff_read_resource_id_utf16(String8 raw_res, U64 off, COFF_ResourceID16 *id_out)
{
  U64 cursor = off;
  
  U16 flag = 0;
  str8_deserial_read_struct(raw_res, cursor, &flag);
  
  if (flag == max_U16) {
    id_out->type = COFF_ResourceIDType_Number;
    cursor += sizeof(flag);
    cursor += str8_deserial_read_struct(raw_res, cursor, &id_out->u.number);
  } else {
    id_out->type = COFF_ResourceIDType_String;
    cursor += str8_deserial_read_windows_utf16_string16(raw_res, cursor, &id_out->u.string);
  }
  
  U64 read_size = cursor - off;
  read_size     = AlignPow2(read_size, COFF_ResourceAlign);
  return read_size;
}

internal U64
coff_read_resource(Arena *arena, String8 raw_res, U64 off, COFF_ParsedResource *res_out)
{
  String8 raw_header    = str8_skip(raw_res, off);
  U64     header_cursor = 0;

  // prefix
  COFF_ResourceHeaderPrefix prefix = {0};
  header_cursor += str8_deserial_read_struct(raw_header, header_cursor, &prefix);

  Assert(prefix.header_size >= sizeof(COFF_ResourceHeaderPrefix));
  raw_header = str8_prefix(raw_header, prefix.header_size);

  // header
  COFF_ResourceID16 type_16 = {0};
  COFF_ResourceID16 name_16 = {0};
  header_cursor += coff_read_resource_id_utf16(raw_header, header_cursor, &type_16);
  header_cursor += coff_read_resource_id_utf16(raw_header, header_cursor, &name_16);
  header_cursor += str8_deserial_read_struct(raw_header, header_cursor, &res_out->data_version);
  header_cursor += str8_deserial_read_struct(raw_header, header_cursor, &res_out->memory_flags);
  header_cursor += str8_deserial_read_struct(raw_header, header_cursor, &res_out->language_id);
  header_cursor += str8_deserial_read_struct(raw_header, header_cursor, &res_out->version);
  header_cursor += str8_deserial_read_struct(raw_header, header_cursor, &res_out->characteristics);
  Assert(prefix.header_size == header_cursor);

  // convert utf-16 resource ids to utf-8
  res_out->type = coff_utf8_resource_id_from_utf16(arena, &type_16);
  res_out->name = coff_utf8_resource_id_from_utf16(arena, &name_16);

  // read data
  U64 data_read_size = str8_deserial_read_block(raw_res, off + prefix.header_size, prefix.data_size, &res_out->data);
  Assert(prefix.data_size == data_read_size);
  
  // compute read size
  U64 read_size = Max(prefix.header_size, sizeof(prefix)) + AlignPow2(prefix.data_size, COFF_ResourceAlign);
  return read_size;
}

internal COFF_ParsedResourceList
coff_resource_list_from_data(Arena *arena, String8 data)
{
  COFF_ParsedResourceList list = {0};
  U64 cursor;
  for (cursor = 0 ; cursor < data.size; ) {
    COFF_ParsedResourceNode *node = push_array(arena, COFF_ParsedResourceNode, 1);
    cursor += coff_read_resource(arena, data, cursor, &node->data);
    SLLQueuePush(list.first, list.last, node);
    ++list.count;
  }
  Assert(cursor == data.size);
  return list;
}

internal String8
coff_write_resource_id(Arena *arena, COFF_ResourceID id)
{
  String8 result = str8_zero();
  switch (id.type) {
  case COFF_ResourceIDType_Null: break;
  case COFF_ResourceIDType_Number: {
    result = coff_resource_number_from_u16(arena, id.u.number);
  } break;
  case COFF_ResourceIDType_String: {
    result = coff_resource_string_from_str8(arena, id.u.string);
  } break;
  default: InvalidPath;
  }
  return result;
}

internal String8
coff_write_resource(Arena          *arena,
                    COFF_ResourceID type,
                    COFF_ResourceID name,
                    U32             data_version,
                    COFF_ResourceMemoryFlags memory_flags,
                    U16             language_id,
                    U32             version,
                    U32             characteristics,
                    String8         data)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List list = {0};

  COFF_ResourceHeaderPrefix *prefix      = push_array(scratch.arena, COFF_ResourceHeaderPrefix, 1);
  String8                    packed_type = coff_write_resource_id(scratch.arena, type);
  String8                    packed_name = coff_write_resource_id(scratch.arena, name);

  // prefix + header
  str8_list_push(scratch.arena, &list, str8_struct(prefix));
  str8_list_push(scratch.arena, &list, packed_type);
  str8_list_push(scratch.arena, &list, packed_name);
  str8_list_push(scratch.arena, &list, str8_struct(&data_version));
  str8_list_push(scratch.arena, &list, str8_struct(&memory_flags));
  str8_list_push(scratch.arena, &list, str8_struct(&language_id));
  str8_list_push(scratch.arena, &list, str8_struct(&version));
  str8_list_push(scratch.arena, &list, str8_struct(&characteristics));

  prefix->data_size   = safe_cast_u32(data.size);
  prefix->header_size = safe_cast_u32(list.total_size);

  // data
  str8_list_push(scratch.arena, &list, data);

  // magic
  str8_list_push_front(scratch.arena, &list, str8_array_fixed(g_coff_res_magic));

  // align
  U64 align_size = AlignPow2(list.total_size, COFF_ResourceAlign) - list.total_size;
  U8 *align      = push_array(scratch.arena, U8, align_size);
  str8_list_push(scratch.arena, &list, str8(align, align_size));

  // join
  String8 res = str8_list_join(arena, &list, 0);

  scratch_end(scratch);
  return res;
}

internal int
coff_resource_id_compar(void *raw_a, void *raw_b)
{
  int cmp;
  COFF_ResourceID *a = raw_b;
  COFF_ResourceID *b = raw_b;
  if (a->type == b->type) {
    switch (a->type) {
      case COFF_ResourceIDType_Null: break;
      case COFF_ResourceIDType_Number: cmp = u16_compar(&a->u.number, &b->u.number);                 break;
      case COFF_ResourceIDType_String: cmp = str8_compar_case_sensitive(&a->u.string, &b->u.string); break;
      default: InvalidPath; break;
    }
  } else {
    cmp = u32_compar(&a->type, &b->type);
  }
  return cmp;
}

internal B32
coff_is_import(String8 raw_archive_member)
{
  B32 is_import = 0;
  if (raw_archive_member.size >= sizeof(U16)*2) {
    U16 *sig1 = (U16*)raw_archive_member.str;
    U16 *sig2 = sig1 + 1;
    is_import = *sig1 == COFF_Machine_Unknown && *sig2 == 0xffff;
  }
  return is_import;
}

internal COFF_DataType
coff_data_type_from_data(String8 raw_archive_member)
{
  B32 is_big_obj = coff_is_big_obj(raw_archive_member);
  if (is_big_obj) {
    return COFF_DataType_BigObj;
  }
  B32 is_import = coff_is_import(raw_archive_member);
  if (is_import) {
    return COFF_DataType_Import;
  }
  return COFF_DataType_Obj;
}

internal B32
coff_is_regular_archive(String8 raw_archive)
{
  B32 is_archive = 0;
  U8 sig[sizeof(g_coff_archive_sig)];
  if (str8_deserial_read_struct(raw_archive, 0, &sig) == sizeof(sig)) {
    is_archive = MemoryCompare(&sig[0], &g_coff_archive_sig[0], sizeof(g_coff_archive_sig)) == 0;
  }
  return is_archive;
}

internal B32
coff_is_thin_archive(String8 raw_archive)
{
  B32 is_archive = 0;
  U8 sig[sizeof(g_coff_thin_archive_sig)];
  if (str8_deserial_read_struct(raw_archive, 0, &sig) == sizeof(sig)) {
    is_archive = MemoryCompare(&sig[0], &g_coff_thin_archive_sig[0], sizeof(g_coff_thin_archive_sig)) == 0;
  }
  return is_archive;
}

internal COFF_ArchiveType
coff_archive_type_from_data(String8 raw_archive)
{
  if (coff_is_regular_archive(raw_archive)) {
    return COFF_Archive_Regular;
  } else if (coff_is_thin_archive(raw_archive)) {
    return COFF_Archive_Thin;
  }
  return COFF_Archive_Null;
}

internal U64
coff_parse_archive_member_header(String8 raw_archive, U64 offset, COFF_ParsedArchiveMemberHeader *header_out)
{
  COFF_ArchiveMemberHeader *header = str8_deserial_get_raw_ptr(raw_archive, offset, sizeof(*header));
  if (header) {
    String8 name     = str8_skip_chop_whitespace(str8_cstring_capped(header->name,     header->name     + sizeof(header->name)    ));
    String8 date     = str8_skip_chop_whitespace(str8_cstring_capped(header->date,     header->date     + sizeof(header->date)    ));
    String8 user_id  = str8_skip_chop_whitespace(str8_cstring_capped(header->user_id,  header->user_id  + sizeof(header->user_id) ));
    String8 group_id = str8_skip_chop_whitespace(str8_cstring_capped(header->group_id, header->group_id + sizeof(header->group_id)));
    String8 mode     = str8_skip_chop_whitespace(str8_cstring_capped(header->mode,     header->mode     + sizeof(header->mode)    ));
    String8 size     = str8_skip_chop_whitespace(str8_cstring_capped(header->size,     header->size     + sizeof(header->size)    ));
    String8 end      = str8_cstring_capped(header->end, header->end + sizeof(header->end));

    U32 data_size = u32_from_str8(size, 10);
    U64 data_off  = offset + sizeof(COFF_ArchiveMemberHeader);

    header_out->name           = name;
    header_out->time_stamp     = u32_from_str8(date, 10);
    header_out->user_id        = u32_from_str8(user_id, 10);
    header_out->group_id       = u32_from_str8(group_id, 10);
    header_out->mode           = mode;
    header_out->is_end_correct = str8_match_lit("`\n", end, 0);
    header_out->data_range     = rng_1u64(data_off, data_off + data_size);

    return sizeof(*header);
  }
  return 0;
}

internal COFF_ArchiveFirstMember
coff_parse_first_archive_member(COFF_ArchiveMember *member)
{
  Assert(str8_match_lit("/", member->header.name, 0));

  U64 cursor = 0;
  
  U32 symbol_count = 0;
  cursor += str8_deserial_read_struct(member->data, cursor, &symbol_count);
  
  symbol_count = from_be_u32(symbol_count);
  
  Rng1U64 member_offsets_range = rng_1u64(cursor, cursor + symbol_count * sizeof(U32));
  cursor += dim_1u64(member_offsets_range);
  
  Rng1U64 string_table_range = rng_1u64(cursor, member->data.size);
  cursor += dim_1u64(string_table_range);

  String8  raw_member_offsets  = str8_substr(member->data, member_offsets_range);
  U32     *member_offsets      = (U32 *)raw_member_offsets.str;
  U64      member_offset_count = raw_member_offsets.size / sizeof(member_offsets[0]);

  COFF_ArchiveFirstMember result = {0};
  result.symbol_count            = symbol_count;
  result.member_offset_count     = member_offset_count;
  result.member_offsets          = member_offsets;
  result.string_table            = str8_substr(member->data, string_table_range);

  return result;
}

internal COFF_ArchiveSecondMember
coff_parse_second_archive_member(COFF_ArchiveMember *member)
{
  COFF_ArchiveSecondMember result = {0};

  if (str8_match_lit("/", member->header.name, 0)) {
    U64 cursor = 0;
    
    U32 member_count = 0;
    cursor += str8_deserial_read_struct(member->data, cursor, &member_count);
    
    Rng1U64 member_offsets_range = rng_1u64(cursor, cursor + member_count * sizeof(U32));
    cursor += dim_1u64(member_offsets_range);
    
    U32 symbol_count = 0;
    cursor += str8_deserial_read_struct(member->data, cursor, &symbol_count);
    
    Rng1U64 symbol_indices_range = rng_1u64(cursor, cursor + symbol_count * sizeof(U16));
    cursor += dim_1u64(symbol_indices_range);
    
    Rng1U64 string_table_range = rng_1u64(cursor, member->data.size);

    String8 raw_member_offsets = str8_substr(member->data, member_offsets_range);
    String8 raw_indices        = str8_substr(member->data, symbol_indices_range);

    U32 *member_offsets      = (U32 *)raw_member_offsets.str;
    U64  member_offset_count = raw_member_offsets.size / sizeof(member_offsets[0]);

    U16 *symbol_indices     = (U16 *)raw_indices.str;
    U64  symbol_index_count = raw_indices.size / sizeof(symbol_indices[0]);
    
    result.member_count        = member_count;
    result.symbol_count        = symbol_count;
    result.member_offsets      = member_offsets;
    result.member_offset_count = member_offset_count;
    result.symbol_indices      = symbol_indices;
    result.symbol_index_count  = symbol_index_count;
    result.string_table        = str8_substr(member->data, string_table_range);
  }

  return result;
}

internal String8
coff_parse_long_name(String8 long_names, String8 name)
{
  String8 result = name;
  if (name.size > 0 && name.str[0] == '/') {
    String8 offset_str = str8(name.str + 1, name.size - 1);
    U64 offset = u64_from_str8(offset_str, 10);
    if (offset < long_names.size) {
      U8 *ptr = long_names.str + offset;
      U8 *opl = long_names.str + long_names.size;
      for (; ptr < opl; ++ptr) {
        if (*ptr == '\0' || *ptr == '\n') {
          break;
        }
      }
      result = str8_range(long_names.str + offset, ptr);
    }
  }
  return result;
}

internal U64
coff_parse_import(String8 raw_archive_member, U64 offset, COFF_ParsedArchiveImportHeader *header_out)
{
  COFF_ImportHeader *header = str8_deserial_get_raw_ptr(raw_archive_member, offset, sizeof(*header));
  if (header) {
    Rng1U64 data_range  = rng_1u64(offset + sizeof(*header), offset + sizeof(*header) + header->data_size);
    String8 raw_data    = str8_substr(raw_archive_member, data_range);
    U64     data_cursor = 0;

    header_out->version         = header->version;
    header_out->machine         = header->machine;
    header_out->time_stamp      = header->time_stamp;
    header_out->data_size       = header->data_size;
    header_out->hint_or_ordinal = header->hint_or_ordinal;
    header_out->type            = COFF_ImportHeader_ExtractType(header->flags);
    header_out->import_by       = COFF_ImportHeader_ExtractImportBy(header->flags);
    data_cursor += str8_deserial_read_cstr(raw_data, data_cursor, &header_out->func_name);
    data_cursor += str8_deserial_read_cstr(raw_data, data_cursor, &header_out->dll_name);
    
    Assert(header_out->func_name.size + header_out->dll_name.size + /* nulls */ 2 == header_out->data_size);
    U64 read_size = sizeof(*header) + header->data_size;
    return read_size;
  }
  return 0;  
}

internal COFF_ArchiveMember
coff_archive_member_from_offset(String8 raw_archive, U64 offset)
{
  COFF_ArchiveMember member = {0};
  coff_regular_archive_member_iter_next(raw_archive, &offset, &member);
  return member;
}

internal COFF_ArchiveMember
coff_archive_member_from_data(String8 raw_archive_member)
{
  return coff_archive_member_from_offset(raw_archive_member, 0);
}

internal COFF_ParsedArchiveImportHeader
coff_archive_import_from_data(String8 raw_archive_member)
{
  COFF_ParsedArchiveImportHeader header = {0};
  coff_parse_import(raw_archive_member, 0, &header);
  return header;
}

internal U64
coff_regular_archive_member_iter_init(String8 raw_archive)
{
  U64 cursor = raw_archive.size;
  if (coff_is_regular_archive(raw_archive)) {
    cursor = sizeof(g_coff_archive_sig);
  }
  return cursor;
}

internal B32
coff_regular_archive_member_iter_next(String8 raw_archive, U64 *offset, COFF_ArchiveMember *member_out)
{
  B32 is_parsed = 0;
  
  member_out->header.is_end_correct = 0;
  U64 header_size = coff_parse_archive_member_header(raw_archive, *offset, &member_out->header);
  
  if (member_out->header.is_end_correct) {
    member_out->offset = *offset;
    member_out->data   = str8_substr(raw_archive, member_out->header.data_range);

    U64 read_size = AlignPow2(header_size + dim_1u64(member_out->header.data_range), COFF_Archive_MemberAlign);
    *offset += read_size;
    
    is_parsed = 1;
  }
  
  return is_parsed;
}

internal U64
coff_thin_archive_member_iter_init(String8 raw_archive)
{
  U64 cursor = raw_archive.size;
  if (coff_is_thin_archive(raw_archive)) {
    cursor = sizeof(g_coff_thin_archive_sig);
  }
  return cursor;
}

internal B32
coff_thin_archive_member_iter_next(String8 raw_archive, U64 *offset, COFF_ArchiveMember *member_out)
{
  B32 is_parsed = 0;
  
  member_out->header.is_end_correct = 0;
  U64 header_size = coff_parse_archive_member_header(raw_archive, *offset, &member_out->header);

  if (member_out->header.is_end_correct) {
    Rng1U64 data_in_archive_range = {0};
    if (str8_match_lit("/", member_out->header.name, 0) || str8_match_lit("//", member_out->header.name, 0)) {
      data_in_archive_range = member_out->header.data_range;
    }

    member_out->offset = *offset;
    member_out->data   = str8_substr(raw_archive, data_in_archive_range);

    U64 read_size = AlignPow2(header_size + dim_1u64(data_in_archive_range), COFF_Archive_MemberAlign);
    *offset += read_size;

    is_parsed = 1;
  }
  
  return is_parsed;
}

internal void
coff_archive_member_list_push_node(COFF_ArchiveMemberList *list, COFF_ArchiveMemberNode *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal COFF_ArchiveParse
coff_archive_parse_from_member_list(COFF_ArchiveMemberList member_list)
{
  String8            error             = str8_zero();
  B32                has_second_header = 0;
  B32                has_long_names    = 0;
  COFF_ArchiveMember first_header      = {0};
  COFF_ArchiveMember second_header     = {0};
  COFF_ArchiveMember long_names_member = {0};

  COFF_ArchiveMemberNode *ptr = member_list.first;

  if (ptr) {
    if (str8_match_lit("/", ptr->data.header.name, 0)) {
      if (ptr->data.header.is_end_correct) {
        first_header = ptr->data;
        ptr = ptr->next;
      } else {
        error = str8_lit("first header doesn't have correct end");
      }
    }
  } else {
    error = str8_lit("missing first header");
  }

  if (!error.size && ptr) {
    if (str8_match_lit("/", ptr->data.header.name, 0)) {
      if (ptr->data.header.is_end_correct) {
        second_header = ptr->data;
        ptr = ptr->next;
        has_second_header = 1;
      } else {
        error = str8_lit("second header doesn't have correct end");
      }
    }
  }

  if (!error.size && ptr) {
    if (str8_match_lit("//", ptr->data.header.name, 0)) {
      if (ptr->data.header.is_end_correct) {
        long_names_member = ptr->data;
        ptr = ptr->next;
        has_long_names;
      } else {
        error = str8_lit("long names header doesn't have correct end");
      }
    }
  }
  
  COFF_ArchiveParse parse = {0};
  parse.has_second_header = has_second_header;
  parse.has_long_names    = has_long_names;
  parse.first_member      = coff_parse_first_archive_member(&first_header);
  parse.second_member     = coff_parse_second_archive_member(&second_header);
  parse.long_names        = long_names_member.data;
  parse.error             = error;

  return parse;
}

internal COFF_ArchiveParse
coff_regular_archive_parse_from_data(String8 raw_archive)
{
  COFF_ArchiveMemberList list = {0};
  COFF_ArchiveMemberNode node_arr[3] = {0};
  U64 cursor = coff_regular_archive_member_iter_init(raw_archive);
  for (U64 i = 0; i < ArrayCount(node_arr); ++i) {
    COFF_ArchiveMemberNode *node = &node_arr[i];
    if (!coff_regular_archive_member_iter_next(raw_archive, &cursor, &node->data)) {
      break;
    }
    coff_archive_member_list_push_node(&list, node);
  }
  return coff_archive_parse_from_member_list(list);
}

internal COFF_ArchiveParse
coff_thin_archive_parse_from_data(String8 raw_archive)
{
  COFF_ArchiveMemberList list = {0};
  COFF_ArchiveMemberNode node_arr[3] = {0};
  U64 cursor = coff_thin_archive_member_iter_init(raw_archive);
  for (U64 i = 0; i < ArrayCount(node_arr); i += 1) {
    COFF_ArchiveMemberNode *node = &node_arr[i];
    if (!coff_thin_archive_member_iter_next(raw_archive, &cursor, &node->data)) {
      break;
    }
    coff_archive_member_list_push_node(&list, node);
  }
  return coff_archive_parse_from_member_list(list);
}

internal COFF_ArchiveParse
coff_archive_parse_from_data(String8 raw_archive)
{
  COFF_ArchiveType type = coff_archive_type_from_data(raw_archive);
  switch (type) {
  case COFF_Archive_Null:    break;
  case COFF_Archive_Regular: return coff_regular_archive_parse_from_data(raw_archive);
  case COFF_Archive_Thin:    return coff_thin_archive_parse_from_data(raw_archive);
  }
  COFF_ArchiveParse null_parse = {0};
  return null_parse;
}

