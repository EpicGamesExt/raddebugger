// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ Dwarf Decode Helpers

static U64
dwarf_leb128_decode_U64(U8 *ptr, U8 *opl){
  U64 r = 0;
  switch (opl - ptr){
    case 10: r |= ((U64)(ptr[9]&0x7F) << 63);
    case  9: r |= ((U64)(ptr[8]&0x7F) << 56);
    case  8: r |= ((U64)(ptr[7]&0x7F) << 49);
    case  7: r |= ((U64)(ptr[6]&0x7F) << 42);
    case  6: r |= ((U64)(ptr[5]&0x7F) << 35);
    case  5: r |= ((U64)(ptr[4]&0x7F) << 28);
    case  4: r |= ((U64)(ptr[3]&0x7F) << 21);
    case  3: r |= ((U64)(ptr[2]&0x7F) << 14);
    case  2: r |= ((U64)(ptr[1]&0x7F) <<  7);
    case  1: r |= ((U64)(ptr[0]&0x7F)      );
    case  0: default: break;
  }
  return(r);
}

static S64
dwarf_leb128_decode_S64(U8 *ptr, U8 *opl){
  U64 u = dwarf_leb128_decode_U32(ptr, opl);
  U64 s = (U64)(opl - ptr)*7;
  B32 neg = ((u & (1llu << s)) != 0);
  if (neg){
    switch (opl - ptr){
      case 9: u |= ~0x7FFFFFFFFFFFFFFFllu; break;
      case 8: u |= ~0x00FFFFFFFFFFFFFFllu; break;
      case 7: u |= ~  0x01FFFFFFFFFFFFllu; break;
      case 6: u |= ~    0x03FFFFFFFFFFllu; break;
      case 5: u |= ~      0x07FFFFFFFFllu; break;
      case 4: u |= ~        0x0FFFFFFFllu; break;
      case 3: u |= ~          0x1FFFFFllu; break;
      case 2: u |= ~            0x3FFFllu; break;
      case 1: u |= ~              0x7Fllu; break;
    }
  }
  S64 r = (S64)(u);
  return(r);
}

static U32
dwarf_leb128_decode_U32(U8 *ptr, U8 *opl){
  U32 r = 0;
  switch (opl - ptr){
    case 5: r |= ((U32)(ptr[4]&0x7F) << 28);
    case 4: r |= ((U32)(ptr[3]&0x7F) << 21);
    case 3: r |= ((U32)(ptr[2]&0x7F) << 14);
    case 2: r |= ((U32)(ptr[1]&0x7F) <<  7);
    case 1: r |= ((U32)(ptr[0]&0x7F)      );
    case 0: default: break;
  }
  return(r);
}


////////////////////////////////
//~ Dwarf Parser Functions

static DWARF_Parsed*
dwarf_parsed_from_elf(Arena *arena, ELF_Parsed *elf){
  DWARF_Parsed *result = 0;
  
  if (elf != 0){
    //- extract debug info
    U32 debug_section_idx[DWARF_SectionCode_COUNT] = {0};
    String8 debug_section_name[DWARF_SectionCode_COUNT] = {0};
    String8 debug_data[DWARF_SectionCode_COUNT] = {0};
    for (U64 i = 1; i < DWARF_SectionCode_COUNT; i += 1){
      DWARF_SectionNameRow *row = dwarf_section_name_table + i;
      U32 idx = 0;
      for (U32 j = 0; idx == 0 && j < DWARF_SECTION_NAME_VARIANT_COUNT; j += 1){
        idx = elf_section_idx_from_name(elf, row->name[j]);
      }
      debug_section_idx[i] = idx;
      debug_section_name[i] = elf_section_name_from_idx(elf, idx);
      debug_data[i] = elf_section_data_from_idx(elf, idx);
    }
    
    //- fill result
    {
      result = push_array(arena, DWARF_Parsed, 1);
      result->elf = elf;
      MemoryCopyArray(result->debug_section_idx, debug_section_idx);
      MemoryCopyArray(result->debug_section_name, debug_section_name);
      MemoryCopyArray(result->debug_data, debug_data);
    }
  }
  
  return(result);
}

static DWARF_IndexParsed*
dwarf_index_from_data(Arena *arena, String8 data){
  DWARF_IndexParsed *result = 0;
  // TODO(allen): 
  return(result);
}

static DWARF_SupParsed*
dwarf_sup_from_data(Arena *arena, String8 data){
  DWARF_SupParsed *result = 0;
  // TODO(allen): 
  return(result);
}

static DWARF_InfoParsed*
dwarf_info_from_data(Arena *arena, String8 data){
  // supported version numbers: 4,5
  
  
  // empty unit list
  DWARF_InfoUnit *first = 0;
  DWARF_InfoUnit *last = 0;
  U64 count = 0;
  
  // whole section loop
  U64 unit_idx = 0;
  U8 *ptr = data.str;
  U8 *opl = data.str + data.size;
  for (;ptr < opl; unit_idx += 1){
    
    // remember header offset
    U64 hdr_off = (ptr - data.str);
    
    // initial length
    U8 *unit_opl = 0;
    B32 is_64bit = 0;
    dwarf__initial_length(data, &ptr, &unit_opl, &is_64bit);
    
    // version
    U8 version = MemoryConsume(U16, ptr, unit_opl);
    
    // rest of header depends on version
    U64 abbrev_off = 0;
    U8  address_size = 0;
    U8  unit_type = 0;
    U64 unit_dwo_id = 0;
    U64 unit_type_signature = 0;
    U64 unit_type_offset = 0;
    switch (version){
      case 4:
      {
        // abbrev_off
        if (is_64bit){
          abbrev_off = MemoryConsume(U64, ptr, unit_opl);
        }
        else{
          abbrev_off = MemoryConsume(U32, ptr, unit_opl);
        }
        
        // address_size
        address_size = MemoryConsume(U8, ptr, unit_opl);
      }break;
      
      case 5:
      {
        // unit_type
        unit_type = (DWARF_UnitType)MemoryConsume(U8, ptr, unit_opl);
        
        // address_size
        address_size = MemoryConsume(U8, ptr, unit_opl);
        
        // abbrev_off
        if (is_64bit){
          abbrev_off = MemoryConsume(U64, ptr, unit_opl);
        }
        else{
          abbrev_off = MemoryConsume(U32, ptr, unit_opl);
        }
        
        // rest of header depends on unit_type
        switch (unit_type){
          case DWARF_UnitType_skeleton: case DWARF_UnitType_split_compile:
          {
            unit_dwo_id = MemoryConsume(U64, ptr, unit_opl);
          }break;
          case DWARF_UnitType_type: case DWARF_UnitType_split_type:
          {
            unit_type_signature = MemoryConsume(U64, ptr, unit_opl);
            if (is_64bit){
              unit_type_offset = MemoryConsume(U64, ptr, unit_opl);
            }
            else{
              unit_type_offset = MemoryConsume(U32, ptr, unit_opl);
            }
          }break;
        }
      }break;
    }
    
    // offset size
    U8 offset_size = is_64bit?8:4;
    
    // unit offsets
    U64 base_off = (ptr - data.str);
    U64 opl_off = (unit_opl - data.str);
    
    // emit unit
    DWARF_InfoUnit *unit = push_array(arena, DWARF_InfoUnit, 1);
    SLLQueuePush(first, last, unit);
    count += 1;
    
    unit->hdr_off      = hdr_off;
    unit->base_off     = base_off;
    unit->opl_off      = opl_off;
    
    unit->offset_size  = offset_size;
    unit->version      = version;
    unit->unit_type    = unit_type;
    unit->address_size = address_size;
    unit->abbrev_off   = abbrev_off;
    
    switch (unit_type){
      case DWARF_UnitType_skeleton: case DWARF_UnitType_split_compile:
      {
        unit->dwo_id = unit_dwo_id;
      }break;
      case DWARF_UnitType_type: case DWARF_UnitType_split_type:
      {
        unit->type_signature = unit_type_signature;
        unit->type_offset = unit_type_offset;
      }break;
    }
    
    // advance to end of unit
    ptr = unit_opl;
  }
  
  // fill result
  DWARF_InfoParsed *result = push_array(arena, DWARF_InfoParsed, 1);
  result->unit_first = first;
  result->unit_last = last;
  result->unit_count = count;
  return(result);
}

static DWARF_PubNamesParsed*
dwarf_pubnames_from_data(Arena *arena, String8 data){
  // supported version numbers: 2
  
  
  // empty unit list
  DWARF_PubNamesUnit *first = 0;
  DWARF_PubNamesUnit *last = 0;
  U64 count = 0;
  
  // whole section loop
  U64 unit_idx = 0;
  U8 *ptr = data.str;
  U8 *opl = data.str + data.size;
  for (;ptr < opl; unit_idx += 1){
    
    // remember header offset
    U64 hdr_off = (ptr - data.str);
    
    // initial length
    U8 *unit_opl = 0;
    B32 is_64bit = 0;
    dwarf__initial_length(data, &ptr, &unit_opl, &is_64bit);
    
    // version
    U8  version = MemoryConsume(U16, ptr, unit_opl);
    
    // info_off
    U64 info_off = 0;
    if (is_64bit){
      info_off = MemoryConsume(U64, ptr, unit_opl);
    }
    else{
      info_off = MemoryConsume(U32, ptr, unit_opl);
    }
    
    // info_length
    U64 info_length = 0;
    if (is_64bit){
      info_length = MemoryConsume(U64, ptr, unit_opl);
    }
    else{
      info_length = MemoryConsume(U32, ptr, unit_opl);
    }
    
    // offset size
    U8 offset_size = is_64bit?8:4;
    
    // unit offsets
    U64 base_off = (ptr - data.str);
    U64 opl_off = (unit_opl - data.str);
    
    // emit unit
    DWARF_PubNamesUnit *unit = push_array(arena, DWARF_PubNamesUnit, 1);
    SLLQueuePush(first, last, unit);
    count += 1;
    
    unit->hdr_off  = hdr_off;
    unit->base_off = base_off;
    unit->opl_off  = opl_off;
    
    unit->offset_size = offset_size;
    unit->version = version;
    unit->info_off = info_off;
    unit->info_length = info_length;
    
    // advance to end of unit
    ptr = unit_opl;
  }
  
  // fill result
  DWARF_PubNamesParsed *result = push_array(arena, DWARF_PubNamesParsed, 1);
  result->unit_first = first;
  result->unit_last  = last;
  result->unit_count = count;
  return(result);
}

static DWARF_NamesParsed*
dwarf_names_from_data(Arena *arena, String8 data){
  // supported version numbers: 5
  
  
  // empty unit list
  DWARF_NamesUnit *first = 0;
  DWARF_NamesUnit *last = 0;
  U64 count = 0;
  
  // whole section loop
  U64 unit_idx = 0;
  U8 *ptr = data.str;
  U8 *opl = data.str + data.size;
  for (;ptr < opl; unit_idx += 1){
    
    // remember header offset
    U64 hdr_off = (ptr - data.str);
    
    // initial length
    U8 *unit_opl = 0;
    B32 is_64bit = 0;
    dwarf__initial_length(data, &ptr, &unit_opl, &is_64bit);
    
    // version
    U8 version = MemoryConsume(U16, ptr, unit_opl);
    
    // *padding*
    MemoryConsume(U16, ptr, unit_opl);
    
    // comp_unit_count
    U32 comp_unit_count = MemoryConsume(U32, ptr, unit_opl);
    
    // local_type_unit_count
    U32 local_type_unit_count = MemoryConsume(U32, ptr, unit_opl);
    
    // foreign_type_unit_count
    U32 foreign_type_unit_count = MemoryConsume(U32, ptr, unit_opl);
    
    // bucket_count
    U32 bucket_count = MemoryConsume(U32, ptr, unit_opl);
    
    // name_count
    U32 name_count = MemoryConsume(U32, ptr, unit_opl);
    
    // abbrev_table_size
    U32 abbrev_table_size = MemoryConsume(U32, ptr, unit_opl);
    
    // augmentation_string_size
    U32 augmentation_string_size = MemoryConsume(U32, ptr, unit_opl);
    
    // augmentation_string
    U8 *augmentation_string = ptr;
    {
      U8 *ptr_raw = ptr + augmentation_string_size;
      ptr = ClampTop(ptr_raw, unit_opl);
    }
    
    // offset size
    U8 offset_size = is_64bit?8:4;
    
    // unit offsets
    U64 base_off = (ptr - data.str);
    U64 opl_off = (unit_opl - data.str);
    
    // emit unit
    DWARF_NamesUnit *unit = push_array(arena, DWARF_NamesUnit, 1);
    SLLQueuePush(first, last, unit);
    count += 1;
    
    unit->hdr_off  = hdr_off;
    unit->base_off = base_off;
    unit->opl_off  = opl_off;
    
    unit->version = version;
    unit->comp_unit_count = comp_unit_count;
    unit->local_type_unit_count = local_type_unit_count;
    unit->foreign_type_unit_count = foreign_type_unit_count;
    unit->bucket_count = bucket_count;
    unit->name_count = name_count;
    unit->abbrev_table_size = abbrev_table_size;
    unit->augmentation_string = str8_cstring_capped(augmentation_string, unit_opl);
    
    // advance to end of unit
    ptr = unit_opl;
  }
  
  // fill result
  DWARF_NamesParsed *result = push_array(arena, DWARF_NamesParsed, 1);
  result->unit_first = first;
  result->unit_last  = last;
  result->unit_count = count;
  return(result);
}

static DWARF_ArangesParsed*
dwarf_aranges_from_data(Arena *arena, String8 data){
  // supported version numbers: 2
  
  
  // empty unit list
  DWARF_ArangesUnit *first = 0;
  DWARF_ArangesUnit *last = 0;
  U64 count = 0;
  
  // whole section loop
  U64 unit_idx = 0;
  U8 *ptr = data.str;
  U8 *opl = data.str + data.size;
  for (;ptr < opl; unit_idx += 1){
    
    // remember header offset
    U64 hdr_off = (ptr - data.str);
    
    // initial length
    U8 *unit_opl = 0;
    B32 is_64bit = 0;
    dwarf__initial_length(data, &ptr, &unit_opl, &is_64bit);
    
    // version
    U8 version = MemoryConsume(U16, ptr, unit_opl);
    
    // info_off
    U64 info_off = 0;
    if (is_64bit){
      info_off = MemoryConsume(U64, ptr, unit_opl);
    }
    else{
      info_off = MemoryConsume(U32, ptr, unit_opl);
    }
    
    // address_size
    U8 address_size = MemoryConsume(U8, ptr, unit_opl);
    
    // segment_selector_size
    U8 segment_selector_size = MemoryConsume(U8, ptr, unit_opl);
    
    // offset size
    U8 offset_size = is_64bit?8:4;
    
    // unit offsets
    U64 base_off = (ptr - data.str);
    U64 opl_off = (unit_opl - data.str);
    
    // emit unit
    DWARF_ArangesUnit *unit = push_array(arena, DWARF_ArangesUnit, 1);
    SLLQueuePush(first, last, unit);
    count += 1;
    
    unit->hdr_off  = hdr_off;
    unit->base_off = base_off;
    unit->opl_off  = opl_off;
    
    unit->version = version;
    unit->address_size = address_size;
    unit->segment_selector_size = segment_selector_size;
    unit->offset_size = offset_size;
    unit->info_off = info_off;
    
    // advance to end of unit
    ptr = unit_opl;
  }
  
  // fill result
  DWARF_ArangesParsed *result = push_array(arena, DWARF_ArangesParsed, 1);
  result->unit_first = first;
  result->unit_last  = last;
  result->unit_count = count;
  return(result);
}

static DWARF_LineParsed*
dwarf_line_from_data(Arena *arena, String8 data){
  // supported version numbers: 4, 5
  
  
  // empty unit list
  DWARF_LineUnit *first = 0;
  DWARF_LineUnit *last = 0;
  U64 count = 0;
  
  // whole section loop
  U64 unit_idx = 0;
  U8 *ptr = data.str;
  U8 *opl = data.str + data.size;
  for (;ptr < opl; unit_idx += 1){
    
    // remember header offset
    U64 hdr_off = (ptr - data.str);
    
    // initial length
    U8 *unit_opl = 0;
    B32 is_64bit = 0;
    dwarf__initial_length(data, &ptr, &unit_opl, &is_64bit);
    
    // version
    U8 version = MemoryConsume(U16, ptr, unit_opl);
    
    // offset size
    U8 offset_size = is_64bit?8:4;
    
    // rest of header depends on version
    U8  minimum_instruction_length = 0;
    U8  maximum_operations_per_instruction = 0;
    U8  default_is_stmt = 0;
    S8  line_base = 0;
    U8  line_range = 0;
    U8  opcode_base = 0;
    U8 *standard_opcode_lengths = 0;
    // v4
    String8List include_directories = {0};
    DWARF_V4LineFileNamesList file_names = {0};
    // v5
    U8  address_size = 0;
    U8  segment_selector_size = 0;
    U8  directory_entry_format_count = 0;
    DWARF_V5LinePathEntryFormat *directory_entry_format = 0;
    U64 directories_count = 0;
    U8  file_name_entry_format_count = 0;
    DWARF_V5LinePathEntryFormat *file_name_entry_format = 0;
    U64 file_names_count = 0;
    
    switch (version){
      case 4:
      {
        // header_length
        U64 header_length = 0;
        if (is_64bit){
          header_length = MemoryConsume(U64, ptr, unit_opl);
        }
        else{
          header_length = MemoryConsume(U32, ptr, unit_opl);
        }
        
        // header opl
        U8 *header_opl_raw = ptr + header_length;
        U8 *header_opl = ClampTop(header_opl_raw, unit_opl);
        
        // minimum_instruction_length
        minimum_instruction_length = MemoryConsume(U8, ptr, header_opl);
        
        // maximum_operations_per_instruction
        maximum_operations_per_instruction = MemoryConsume(U8, ptr, header_opl);
        
        // default_is_stmt
        default_is_stmt = MemoryConsume(U8, ptr, header_opl);
        
        // line_base
        line_base = MemoryConsume(S8, ptr, header_opl);
        
        // line_range
        line_range = MemoryConsume(U8, ptr, header_opl);
        
        // opcode_base
        opcode_base = MemoryConsume(U8, ptr, header_opl);
        
        // standard_opcode_lengths
        if (opcode_base > 1){
          standard_opcode_lengths = ptr;
          ptr += opcode_base - 1;
        }
        
        // include_directories
        for (;ptr < header_opl;){
          // null byte ends entries
          if (*ptr == 0){
            ptr += 1;
            break;
          }
          
          // extract dir range
          U8 *dir_first = ptr;
          for (;ptr < header_opl && *ptr != 0;) ptr += 1;
          U8 *dir_opl = ptr;
          if (ptr < header_opl){
            ptr += 1;
          }
          
          // attach dir to list
          String8 dir = str8_range(dir_first, dir_opl);
          str8_list_push(arena, &include_directories, dir);
        }
        
        // file_names
        for (;ptr < header_opl;){
          // null byte ends entries
          if (*ptr == 0){
            ptr += 1;
            break;
          }
          
          // extract file_name range
          U8 *file_name_first = ptr;
          for (;ptr < header_opl && *ptr != 0;) ptr += 1;
          U8 *file_name_opl = ptr;
          if (ptr < header_opl){
            ptr += 1;
          }
          
          // extract include directory index
          U64 include_directory_idx = 0;
          DWARF_LEB128_DECODE_ADV(U64, include_directory_idx, ptr, header_opl);
          
          // extract last modified time
          U64 last_modified_time = 0;
          DWARF_LEB128_DECODE_ADV(U64, last_modified_time, ptr, header_opl);
          
          // extract file size
          U64 file_size = 0;
          DWARF_LEB128_DECODE_ADV(U64, file_size, ptr, header_opl);
          
          // emit file name entry
          DWARF_V4LineFileNamesEntry *entry = push_array(arena, DWARF_V4LineFileNamesEntry, 1);
          SLLQueuePush(file_names.first, file_names.last, entry);
          file_names.count += 1;
          entry->file_name = str8_range(file_name_first, file_name_opl);
          entry->include_directory_idx = include_directory_idx;
          entry->last_modified_time = last_modified_time;
          entry->file_size = file_size;
        }
        
        ptr = header_opl;
      }break;
      
      case 5:
      {
        // address_size
        address_size = MemoryConsume(U8, ptr, unit_opl);
        
        // segment_selector_size
        segment_selector_size = MemoryConsume(U8, ptr, unit_opl);
        
        // header_length
        U64 header_length = 0;
        if (is_64bit){
          header_length = MemoryConsume(U64, ptr, unit_opl);
        }
        else{
          header_length = MemoryConsume(U32, ptr, unit_opl);
        }
        
        // header opl
        U8 *header_opl_raw = ptr + header_length;
        U8 *header_opl = ClampTop(header_opl_raw, unit_opl);
        
        // minimum_instruction_length
        minimum_instruction_length = MemoryConsume(U8, ptr, header_opl);
        
        // maximum_operations_per_instruction
        maximum_operations_per_instruction = MemoryConsume(U8, ptr, header_opl);
        
        // default_is_stmt
        default_is_stmt = MemoryConsume(U8, ptr, header_opl);
        
        // line_base
        line_base = MemoryConsume(S8, ptr, header_opl);
        
        // line_range
        line_range = MemoryConsume(U8, ptr, header_opl);
        
        // opcode_base
        opcode_base = MemoryConsume(U8, ptr, header_opl);
        
        // standard_opcode_lengths
        if (opcode_base > 1){
          standard_opcode_lengths = ptr;
          ptr += opcode_base - 1;
        }
        
        // directory_entry_format_count
        directory_entry_format_count = MemoryConsume(U8, ptr, header_opl);
        
        // directory_entry_format
        {
          directory_entry_format = push_array(arena, DWARF_V5LinePathEntryFormat,
                                              directory_entry_format_count);
          DWARF_V5LinePathEntryFormat *entry = directory_entry_format;
          DWARF_V5LinePathEntryFormat *entry_opl =
            directory_entry_format + directory_entry_format_count;
          for (;entry < entry_opl && ptr < header_opl; entry += 1){
            DWARF_LEB128_DECODE_ADV(U64, entry->content_type, ptr, header_opl);
            DWARF_LEB128_DECODE_ADV(U64, entry->form, ptr, header_opl);
          }
        }
        
        // directories_count
        DWARF_LEB128_DECODE_ADV(U64, directories_count, ptr, header_opl);
        
        // directories
        DWARF_V5Directory *directories = push_array(arena, DWARF_V5Directory, directories_count);
        dwarf__line_v5_directories(address_size, offset_size,
                                   directory_entry_format, directory_entry_format_count,
                                   directories, directories_count,
                                   &ptr, header_opl);
        
        // file_name_entry_format_count
        file_name_entry_format_count = MemoryConsume(U8, ptr, header_opl);
        
        // file_name_entry_format
        {
          file_name_entry_format = push_array(arena, DWARF_V5LinePathEntryFormat,
                                              file_name_entry_format_count);
          DWARF_V5LinePathEntryFormat *entry = file_name_entry_format;
          for (;ptr < header_opl; entry += 1){
            DWARF_LEB128_DECODE_ADV(U64, entry->content_type, ptr, header_opl);
            DWARF_LEB128_DECODE_ADV(U64, entry->form, ptr, header_opl);
          }
        }
        
        // file_names_count
        DWARF_LEB128_DECODE_ADV(U64, file_names_count, ptr, header_opl);
        
        // file_names
        DWARF_V5Directory *file_names = push_array(arena, DWARF_V5Directory, file_names_count);
        dwarf__line_v5_directories(address_size, offset_size,
                                   directory_entry_format, directory_entry_format_count,
                                   file_names, file_names_count,
                                   &ptr, header_opl);
      }break;
    }
    
    // unit offsets
    U64 base_off = (ptr - data.str);
    U64 opl_off = (unit_opl - data.str);
    
    // emit unit
    DWARF_LineUnit *unit = push_array(arena, DWARF_LineUnit, 1);
    SLLQueuePush(first, last, unit);
    count += 1;
    
    unit->hdr_off  = hdr_off;
    unit->base_off = base_off;
    unit->opl_off  = opl_off;
    
    unit->version = version;
    
    // advance to end of unit
    ptr = unit_opl;
  }
  
  // fill result
  DWARF_LineParsed *result = push_array(arena, DWARF_LineParsed, 1);
  result->unit_first = first;
  result->unit_last  = last;
  result->unit_count = count;
  return(result);
}

static DWARF_MacInfoParsed*
dwarf_mac_info_from_data(Arena *arena, String8 data){
  DWARF_MacInfoParsed *result = 0;
  // TODO(allen): 
  return(result);
}

static DWARF_MacroParsed*
dwarf_macro_from_data(Arena *arena, String8 data){
  DWARF_MacroParsed *result = 0;
  // TODO(allen): 
  return(result);
}

static DWARF_FrameParsed*
dwarf_frame_from_data(Arena *arena, String8 data){
  DWARF_FrameParsed *result = 0;
  // TODO(allen): 
  return(result);
}

static DWARF_RangesParsed*
dwarf_ranges_from_data(Arena *arena, String8 data){
  DWARF_RangesParsed *result = 0;
  // TODO(allen): 
  return(result);
}

static DWARF_StrOffsetsParsed*
dwarf_str_offsets_from_data(Arena *arena, String8 data){
  DWARF_StrOffsetsParsed *result = 0;
  // TODO(allen): 
  return(result);
}

static DWARF_AddrParsed*
dwarf_addr_from_data(Arena *arena, String8 data){
  // supported version numbers: 5
  
  
  // addr unit list
  DWARF_AddrUnit *first = 0;
  DWARF_AddrUnit *last = 0;
  U64 count = 0;
  
  // whole section loop
  U64 unit_idx  = 0;
  U8 *ptr = data.str;
  U8 *opl = data.str + data.size;
  for (;ptr < opl; unit_idx += 1){
    
    U64 hdr_off = (ptr - data.str);
    
    // initial length
    U8 *unit_opl = 0;
    B32 is_64bit = 0;
    dwarf__initial_length(data, &ptr, &unit_opl, &is_64bit);
    
    // version
    U8 version = MemoryConsume(U16, ptr, unit_opl);
    
    // address size
    U8 address_size = MemoryConsume(U8, ptr, unit_opl);
    
    // segment selector size
    U8 segment_selector_size = MemoryConsume(U8, ptr, unit_opl);
    
    // offset size
    U32 offset_size = is_64bit?8:4;
    
    // unit offsets
    U64 base_off = (ptr - data.str);
    U64 opl_off = (unit_opl - data.str);
    
    // emit addr unit
    DWARF_AddrUnit *unit = push_array(arena, DWARF_AddrUnit, 1);
    SLLQueuePush(first, last, unit);
    count += 1;
    
    unit->hdr_off  = hdr_off;
    unit->base_off = base_off;
    unit->opl_off  = opl_off;
    
    unit->offset_size = offset_size;
    unit->dwarf_version = version;
    unit->address_size = address_size;
    unit->segment_selector_size = segment_selector_size;
    
    // advance to next unit
    ptr = unit_opl;
  }
  
  // fill result
  DWARF_AddrParsed *result = push_array(arena, DWARF_AddrParsed, 1);
  result->unit_first = first;
  result->unit_last = last;
  result->unit_count = count;
  return(result);
}

static DWARF_RngListsParsed*
dwarf_rng_lists_from_data(Arena *arena, String8 data){
  DWARF_RngListsParsed *result = 0;
  // TODO(allen): 
  return(result);
}

static DWARF_LocListsParsed*
dwarf_loc_lists_from_data(Arena *arena, String8 data){
  DWARF_LocListsParsed *result = 0;
  // TODO(allen): 
  return(result);
}


// parse helpers

static void
dwarf__initial_length(String8 data, U8 **ptr_inout, U8 **unit_opl_out, B32 *is_64bit_out){
  U8 *unit_opl = 0;
  B32 is_64bit = 0;
  
  U8 *opl = data.str + data.size;
  U8 *ptr = *ptr_inout;
  {
    U64 length = 0;
    U32 m = MemoryConsume(U32, ptr, opl);
    if (m == 0xFFFFFFFF){
      is_64bit = 1;
      length = MemoryConsume(U64, ptr, opl);
    }
    else{
      length = ClampTop(m, 0xFFFFFFF0);
    }
    if (length > 0){
      U64 unit_opl_off_raw = (ptr - data.str) + length;
      U64 unit_opl_off = ClampTop(unit_opl_off_raw, data.size);
      unit_opl = data.str + unit_opl_off;
    }
    else{
      unit_opl = ptr;
    }
  }
  
  *ptr_inout = ptr;
  *unit_opl_out = unit_opl;
  *is_64bit_out = is_64bit;
}

static void
dwarf__line_v5_directories(U64 address_size, U64 offset_size,
                           DWARF_V5LinePathEntryFormat *format, U64 format_count,
                           DWARF_V5Directory *directories_out, U64 dir_count,
                           U8 **ptr_io, U8 *opl){
  
  U8 *ptr = *ptr_io;
  
  DWARF_V5Directory *directory_ptr = directories_out;
  for (U32 i = 0; i < dir_count; i += 1, directory_ptr += 1){
    DWARF_V5LinePathEntryFormat *fmt = format;
    for (U32 j = 0; j < format_count; j += 1){
      
      // form decode
      DWARF_FormDecodeRules rules =
        dwarf_form_decode_rule(fmt->form, address_size, offset_size);
      
      // execute decoding
      DWARF_FormDecoded decoded = dwarf_form_decode(&rules, &ptr, opl, 0, 0);
      
      // store to correct field
      U64 *target = 0;
      switch (fmt->content_type){
        case DWARF_LineEntryFormat_path:
        {
          if (decoded.dataptr != 0){
            directory_ptr->path_str = str8(decoded.dataptr, decoded.val);
          }
          else{
            directory_ptr->path_off = decoded.val;
            directory_ptr->path_sec_form = fmt->form;
          }
        }break;
        
        case DWARF_LineEntryFormat_directory_index:
        {
          target = &directory_ptr->directory_index;
        }goto v5_directory_u64;
        
        case DWARF_LineEntryFormat_timestamp:
        {
          target = &directory_ptr->timestamp;
        }goto v5_directory_u64;
        
        case DWARF_LineEntryFormat_size:
        {
          target = &directory_ptr->size;
        }goto v5_directory_u64;
        
        v5_directory_u64:
        {
          if (decoded.dataptr != 0){
            U64 size = ClampTop(decoded.val, 8);
            MemoryCopy(target, decoded.dataptr, size);
          }
          else{
            *target = decoded.val;
          }
        }break;
        
        case DWARF_LineEntryFormat_MD5:
        {
          if (decoded.dataptr != 0){
            U64 size = ClampTop(decoded.val, 16);
            MemoryCopy(directory_ptr->md5_checksum, decoded.dataptr, size);
          }
        }break;
      }
    }
  }
  
  *ptr_io = ptr;
}


// debug sections

static String8
dwarf_name_from_debug_section(DWARF_Parsed *dwarf, DWARF_SectionCode sec_code){
  String8 result = str8_lit("invalid_debug_section");
  if (sec_code < DWARF_SectionCode_COUNT){
    if (dwarf->debug_section_idx[sec_code] != 0){
      result = dwarf->debug_section_name[sec_code];
    }
  }
  return(result);
}


// abbrev functions

static DWARF_AbbrevUnit*
dwarf_abbrev_unit_from_offset(DWARF_AbbrevParsed *abbrev, U64 offset){
  DWARF_AbbrevUnit *result = 0;
  for (DWARF_AbbrevUnit *unit = abbrev->unit_first;
       unit != 0;
       unit = unit->next){
    if (unit->offset == offset){
      result = unit;
      break;
    }
  }
  return(result);
}

static DWARF_AbbrevDecl*
dwarf_abbrev_decl_from_code(DWARF_AbbrevUnit *unit, U32 abbrev_code){
  DWARF_AbbrevDecl *result = 0;
  for (DWARF_AbbrevDecl *decl = unit->first;
       decl != 0;
       decl = decl->next){
    if (decl->abbrev_code == abbrev_code){
      result = decl;
      break;
    }
  }
  return(result);
}

// attribute decoding functions

static DWARF_AttributeClassFlags
dwarf_attribute_class_from_form(DWARF_AttributeForm form){
  DWARF_AttributeClassFlags result = 0;
  switch (form){
#define X(N,C,f) case C: result = DWARF_AttributeClassFlag_##f; break;
    DWARF_AttributeFormXList(X)
#undef X
  }
  return(result);
}

static DWARF_AttributeClassFlags
dwarf_attribute_class_from_name(DWARF_AttributeName name){
  DWARF_AttributeClassFlags result = 0;
  switch (name){
#define X(N,C,f1,f2,f3,f4) case C: result = 0\
|DWARF_AttributeClassFlag_##f1\
|DWARF_AttributeClassFlag_##f2\
|DWARF_AttributeClassFlag_##f3\
|DWARF_AttributeClassFlag_##f4\
;break;
    DWARF_AttributeNameXList(X)
#undef X
  }
  return(result);
}

// form decoding functions

static DWARF_FormDecodeRules
dwarf_form_decode_rule(DWARF_AttributeForm form, U64 address_size, U64 offset_size){
  DWARF_FormDecodeRules result = {0};
  switch (form){
    case DWARF_AttributeForm_null:
    case DWARF_AttributeForm_indirect:{}break;
    
    case DWARF_AttributeForm_addr: result.size = address_size; break;
    case DWARF_AttributeForm_addrx: result.uleb128 = 1; break;
    case DWARF_AttributeForm_addrx1: result.size = 1; break;
    case DWARF_AttributeForm_addrx2: result.size = 2; break;
    case DWARF_AttributeForm_addrx3: result.size = 3; break;
    case DWARF_AttributeForm_addrx4: result.size = 4; break;
    
    case DWARF_AttributeForm_sec_offset: result.size = offset_size; break;
    
    case DWARF_AttributeForm_block1: result.size = 1; result.block = 1; break;
    case DWARF_AttributeForm_block2: result.size = 2; result.block = 1; break;
    case DWARF_AttributeForm_block4: result.size = 4; result.block = 1; break;
    case DWARF_AttributeForm_block: result.uleb128 = 1; result.block = 1; break;
    
    case DWARF_AttributeForm_data1: result.size = 1; break;
    case DWARF_AttributeForm_data2: result.size = 2; break;
    case DWARF_AttributeForm_data4: result.size = 4; break;
    case DWARF_AttributeForm_data8: result.size = 8; break;
    case DWARF_AttributeForm_data16: result.size = 16; break;
    
    case DWARF_AttributeForm_sdata: result.sleb128 = 1; break;
    case DWARF_AttributeForm_udata: result.uleb128 = 1; break;
    
    case DWARF_AttributeForm_implicit_const: result.in_abbrev = 1; break;
    
    case DWARF_AttributeForm_exprloc: result.uleb128 = 1; result.block = 1; break;
    
    case DWARF_AttributeForm_flag: result.size = 1; break;
    case DWARF_AttributeForm_flag_present: result.auto_1 = 1; break;
    
    case DWARF_AttributeForm_loclistx: result.uleb128 = 1; break;
    case DWARF_AttributeForm_rnglistx: result.uleb128 = 1; break;
    
    case DWARF_AttributeForm_ref1: result.size = 1; break;
    case DWARF_AttributeForm_ref2: result.size = 2; break;
    case DWARF_AttributeForm_ref4: result.size = 4; break;
    case DWARF_AttributeForm_ref8: result.size = 8; break;
    case DWARF_AttributeForm_ref_udata: result.uleb128 = 1; break;
    
    case DWARF_AttributeForm_ref_addr: result.size = offset_size; break;
    
    case DWARF_AttributeForm_ref_sig8: result.size = 8; break;
    
    case DWARF_AttributeForm_ref_sup4: result.size = 4; break;
    case DWARF_AttributeForm_ref_sup8: result.size = 8; break;
    
    case DWARF_AttributeForm_string: result.null_terminated = 1; break;
    
    case DWARF_AttributeForm_strp: result.size = offset_size; break;
    case DWARF_AttributeForm_line_strp: result.size = offset_size; break;
    case DWARF_AttributeForm_strp_sup: result.size = offset_size; break;
    
    case DWARF_AttributeForm_strx: result.uleb128 = 1; break;
    case DWARF_AttributeForm_strx1: result.size = 1; break;
    case DWARF_AttributeForm_strx2: result.size = 2; break;
    case DWARF_AttributeForm_strx3: result.size = 3; break;
    case DWARF_AttributeForm_strx4: result.size = 4; break;
  }
  
  return(result);
}

static DWARF_FormDecoded
dwarf_form_decode(DWARF_FormDecodeRules *rules, U8 **ptr_io, U8 *opl,
                  DWARF_AbbrevDecl *abbrev_decl, U32 attrib_i){
  
  // local copy of ptr
  U8 *ptr = *ptr_io;
  
  // apply rules
  U64 val = 0;
  U8 *dataptr = 0;
  
  B32 success = 1;
  if (rules->size > 0){
    if (ptr + rules->size <= opl){
      MemoryCopy(&val, ptr, rules->size);
      ptr += rules->size;
    }
    else{
      success = 0;
    }
  }
  else if (rules->uleb128 || rules->sleb128){
    U8 *val_ptr = ptr;
    DWARF_LEB128_ADV(ptr, opl, success);
    if (success){
      if (rules->uleb128){
        val = dwarf_leb128_decode_U64(val_ptr, ptr);
      }
      else{
        val = (U64)dwarf_leb128_decode_S64(val_ptr, ptr);
      }
    }
  }
  else if (rules->in_abbrev){
    if (abbrev_decl != 0){
      if (abbrev_decl->implicit_const != 0){
        val = (U64)abbrev_decl->implicit_const[attrib_i];
      }
    }
    else{
      success = 0;
    }
  }
  else if (rules->auto_1){
    val = 1;
  }
  if (rules->block){
    dataptr = ptr;
    ptr += val;
  }
  else if (rules->null_terminated){
    dataptr = ptr;
    for (;ptr < opl && *ptr != 0;) ptr += 1;
    val = (U64)(ptr - dataptr);
    if (ptr < opl){
      ptr += 1;
    }
  }
  
  // store out ptr
  *ptr_io = ptr;
  
  // fill result
  DWARF_FormDecoded result = {0};
  result.val = val;
  result.dataptr = dataptr;
  result.error = !success;
  return(result);
}


// string functions

static String8
dwarf_string_from_unit_type(DWARF_UnitType type){
  String8 result = str8_lit("unrecognized_type");
  switch (type){
#define X(N,C) case C: result = str8_lit(#N); break;
    DWARF_UnitTypeXList(X)
#undef X
  }
  return(result);
}

static String8
dwarf_string_from_tag(DWARF_Tag tag){
  String8 result = str8_lit("unrecognized_tag");
  switch (tag){
#define X(N,C) case C: result = str8_lit(#N); break;
    DWARF_TagXList(X)
#undef X
  }
  return(result);
}

static String8
dwarf_string_from_attribute_name(DWARF_AttributeName name){
  String8 result = str8_lit("unrecognized_attribute_name");
  switch (name){
#define X(N,C,f1,f2,f3,f4) case C: result = str8_lit(#N); break;
    DWARF_AttributeNameXList(X)
#undef X
  }
  return(result);
}

static String8
dwarf_string_from_attribute_form(DWARF_AttributeForm form){
  String8 result = str8_lit("unrecognized_attribute_form");
  switch (form){
#define X(N,C,k) case C: result = str8_lit(#N); break;
    DWARF_AttributeFormXList(X)
#undef X
  }
  return(result);
}

static String8
dwarf_string_from_line_std_op(DWARF_LineStdOp op){
  String8 result = str8_lit("unrecognized_line_std_op");
  switch (op){
#define X(N,C) case C: result = str8_lit(#N); break;
    DWARF_LineStdOpXList(X)
#undef X
  }
  return(result);
}

static String8
dwarf_string_from_line_ext_op(DWARF_LineExtOp op){
  String8 result = str8_lit("unrecognized_line_ext_op");
  switch (op){
#define X(N,C) case C: result = str8_lit(#N); break;
    DWARF_LineExtOpXList(X)
#undef X
  }
  return(result);
}

static String8
dwarf_string_from_line_entry_format(DWARF_LineEntryFormat format){
  String8 result = str8_lit("unrecognized_line_entry_format");
  switch (format){
#define X(N,C) case C: result = str8_lit(#N); break;
    DWARF_LineEntryFormatXList(X)
#undef X
  }
  return(result);
}

static String8
dwarf_string_from_section_code(DWARF_SectionCode sec_code){
  String8 result = str8_lit("unrecognized_section_code");
  switch (sec_code){
    case DWARF_SectionCode_COUNT:{}break;
#define X(Nc,Vf,N0,N1,N2) case DWARF_SectionCode_##Nc: result = str8_lit(#Nc); break;
    DWARF_SectionNameXList(X,0,0)
#undef X
  }
  return(result);
}





#if 0
static DWARF_InfoParsed*
dwarf_info_from_data(Arena *arena, String8 data, DWARF_InfoParams *params,
                     DWARF_AbbrevParsed *abbrev){
  
  // unit index range to extract
  U64 unit_idx_min = params->unit_idx_min;
  U64 unit_idx_max = params->unit_idx_max;
  
  // empty unit list
  DWARF_InfoUnit *unit_first = 0;
  DWARF_InfoUnit *unit_last = 0;
  U64 unit_count = 0;
  B32 decoding_error = 0;
  
  // whole section loop
  U64 unit_idx = 0;
  U8 *ptr = data.str;
  U8 *opl = data.str + data.size;
  for (;ptr < opl; unit_idx += 1){
    
    // early escape on unit idx
    if (unit_idx > unit_idx_max){
      break;
    }
    
    // determine whether to full parse this unit
    B32 full_parse = (unit_idx_min <= unit_idx);
    
    // header fields
    U8 *unit_opl = 0;
    B32 is_64bit = 0;
    U16 version = 0;
    U64 abbrev_offset = 0;
    U32 address_size = 0;
    DWARF_UnitType unit_type = DWARF_UnitType_null;
    U64 unit_dwo_id = 0;
    U64 unit_type_signature = 0;
    U64 unit_type_offset = 0;
    
    // initial length
    dwarf__initial_length(&ptr, opl, &unit_opl, &is_64bit);
    
    // if this is not a full parse we may use
    //  unit_opl to skip to the next unit now
    if (full_parse){
      
      // version (part of header)
      version = MemoryConsume(U16, ptr, unit_opl);
      
      // rest of header depends on version
      switch (version){
        case 4:
        {
          // abbrev_offset (part of header)
          if (is_64bit){
            abbrev_offset = MemoryConsume(U64, ptr, unit_opl);
          }
          else{
            abbrev_offset = MemoryConsume(U32, ptr, unit_opl);
          }
          
          // address_size (part of header)
          address_size = MemoryConsume(U8, ptr, unit_opl);
        }break;
        
        case 5:
        {
          // unit_type (part of header)
          unit_type = (DWARF_UnitType)MemoryConsume(U8, ptr, unit_opl);
          
          // address_size (part of header)
          address_size = MemoryConsume(U8, ptr, unit_opl);
          
          // abbrev_offset (part of header)
          if (is_64bit){
            abbrev_offset = MemoryConsume(U64, ptr, unit_opl);
          }
          else{
            abbrev_offset = MemoryConsume(U32, ptr, unit_opl);
          }
          
          // rest of header depends on unit_type
          switch (unit_type){
            case DWARF_UnitType_skeleton:
            case DWARF_UnitType_split_compile:
            {
              unit_dwo_id = MemoryConsume(U64, ptr, unit_opl);
            }break;
            case DWARF_UnitType_type:
            case DWARF_UnitType_split_type:
            {
              unit_type_signature = MemoryConsume(U64, ptr, unit_opl);
              if (is_64bit){
                unit_type_offset = MemoryConsume(U64, ptr, unit_opl);
              }
              else{
                unit_type_offset = MemoryConsume(U32, ptr, unit_opl);
              }
            }break;
          }
        }break;
      }
      
      // offset size
      U32 offset_size = is_64bit?8:4;
      
      // find matching abbrev unit
      DWARF_AbbrevUnit *abbrev_unit = dwarf_abbrev_unit_from_offset(abbrev, abbrev_offset);
      if (abbrev_unit == 0){
        // TODO: preserve error info
        decoding_error = 1;
      }
      
      // consume info entries
      DWARF_InfoEntry *entry_root = 0;
      U64 entry_count = 0;
      
      DWARF_InfoEntry *entry_consptr = 0;
      if (abbrev_unit != 0){
        for (;ptr < unit_opl;){
          B32 success = 1;
          
          // mark beginning of entry
          U8 *entry_start_ptr = ptr;
          
          // extract abbrev code
          U8 *abbrev_code_ptr = ptr;
          DWARF_LEB128_ADV(ptr, unit_opl, success);
          if (!success){
            // TODO: preserve error info
            decoding_error = 1;
            goto exit_unit_loop;
          }
          
          U32 abbrev_code = dwarf_leb128_decode_U32(abbrev_code_ptr, ptr);
          
          // null abbrev code means pop
          if (abbrev_code == 0){
            if (entry_consptr == 0){
              goto exit_unit_loop;
            }
            else{
              entry_consptr = entry_consptr->parent;
              goto skip_entry;
            }
          }
          
          // get abbrev decl
          DWARF_AbbrevDecl *abbrev_decl = dwarf_abbrev_decl_from_code(abbrev_unit, abbrev_code);
          if (abbrev_decl == 0){
            // TODO: preserve error info
            decoding_error = 1;
            goto exit_unit_loop;
          }
          
          // allocate entry
          U32 attrib_count = abbrev_decl->attrib_count;
          DWARF_InfoEntry *entry = push_array(arena, DWARF_InfoEntry, 1);
          DWARF_InfoAttribVal *attrib_vals =
            push_array_no_zero(arena, DWARF_InfoAttribVal, attrib_count);
          
          // save entry offset
          entry->info_offset = (U64)(entry_start_ptr - data.str);
          
          // set root at beginning
          if (entry_root == 0){
            entry_root = entry;
          }
          
          // attribute loop
          DWARF_AbbrevAttribSpec *attrib_spec = abbrev_decl->attrib_specs;
          DWARF_InfoAttribVal *attrib_val = attrib_vals;
          for (U32 i = 0; i < attrib_count; i += 1, attrib_spec += 1, attrib_val += 1){
            
            // determine decoding rules
            U32 size = 0;
            B8 uleb128 = 0;
            B8 sleb128 = 0;
            B8 in_abbrev = 0;
            B8 auto_1 = 0;
            B8 block = 0;
            B8 null_terminated = 0;
            {
              DWARF_AttributeForm form = attrib_spec->form;
              switch (form){
                case DWARF_AttributeForm_addr: size = address_size; break;
                case DWARF_AttributeForm_addrx: uleb128 = 1; break;
                case DWARF_AttributeForm_addrx1: size = 1; break;
                case DWARF_AttributeForm_addrx2: size = 2; break;
                case DWARF_AttributeForm_addrx3: size = 3; break;
                case DWARF_AttributeForm_addrx4: size = 4; break;
                
                case DWARF_AttributeForm_sec_offset: size = offset_size; break;
                
                case DWARF_AttributeForm_block1: size = 1; block = 1; break;
                case DWARF_AttributeForm_block2: size = 2; block = 1; break;
                case DWARF_AttributeForm_block4: size = 4; block = 1; break;
                case DWARF_AttributeForm_block: uleb128 = 1; block = 1; break;
                
                case DWARF_AttributeForm_data1: size = 1; break;
                case DWARF_AttributeForm_data2: size = 2; break;
                case DWARF_AttributeForm_data4: size = 4; break;
                case DWARF_AttributeForm_data8: size = 8; break;
                case DWARF_AttributeForm_data16: size = 16; break;
                
                case DWARF_AttributeForm_sdata: sleb128 = 1; break;
                case DWARF_AttributeForm_udata: uleb128 = 1; break;
                
                case DWARF_AttributeForm_implicit_const: in_abbrev = 1; break;
                
                case DWARF_AttributeForm_exprloc: uleb128 = 1; block = 1; break;
                
                case DWARF_AttributeForm_flag: size = 1; break;
                case DWARF_AttributeForm_flag_present: auto_1 = 1; break;
                
                case DWARF_AttributeForm_loclistx: uleb128 = 1; break;
                case DWARF_AttributeForm_rnglistx: uleb128 = 1; break;
                
                case DWARF_AttributeForm_ref1: size = 1; break;
                case DWARF_AttributeForm_ref2: size = 2; break;
                case DWARF_AttributeForm_ref4: size = 4; break;
                case DWARF_AttributeForm_ref8: size = 8; break;
                case DWARF_AttributeForm_ref_udata: uleb128 = 1; break;
                
                case DWARF_AttributeForm_ref_addr: size = offset_size; break;
                
                case DWARF_AttributeForm_ref_sig8: size = 8; break;
                
                case DWARF_AttributeForm_ref_sup4: size = 4; break;
                case DWARF_AttributeForm_ref_sup8: size = 8; break;
                
                case DWARF_AttributeForm_string: null_terminated = 1; break;
                
                case DWARF_AttributeForm_strp: size = offset_size; break;
                case DWARF_AttributeForm_line_strp: size = offset_size; break;
                case DWARF_AttributeForm_strp_sup: size = offset_size; break;
                
                case DWARF_AttributeForm_strx: uleb128 = 1; break;
                case DWARF_AttributeForm_strx1: size = 1; break;
                case DWARF_AttributeForm_strx2: size = 2; break;
                case DWARF_AttributeForm_strx3: size = 3; break;
                case DWARF_AttributeForm_strx4: size = 4; break;
              }
            }
            
            // execute decoding rules
            U64 val = 0;
            U8 *dataptr = 0;
            {
              if (size > 0){
                if (ptr + size <= unit_opl){
                  MemoryCopy(&val, ptr, size);
                  ptr += size;
                }
                else{
                  // TODO: preserve error info
                  decoding_error = 1;
                  goto exit_unit_loop;
                }
              }
              else if (uleb128 || sleb128){
                U8 *val_ptr = ptr;
                DWARF_LEB128_ADV(ptr, unit_opl, success);
                if (!success){
                  // TODO: preserve error info
                  decoding_error = 1;
                  goto exit_unit_loop;
                }
                else{
                  if (uleb128){
                    val = dwarf_leb128_decode_U64(val_ptr, ptr);
                  }
                  else{
                    val = (U64)dwarf_leb128_decode_S64(val_ptr, ptr);
                  }
                }
              }
              else if (in_abbrev){
                if (abbrev_decl->implicit_const != 0){
                  val = (U64)abbrev_decl->implicit_const[i];
                }
              }
              else if (auto_1){
                val = 1;
              }
              if (block){
                dataptr = ptr;
                ptr += val;
              }
              else if (null_terminated){
                dataptr = ptr;
                for (;ptr < unit_opl && *ptr != 0;) ptr += 1;
                val = (U64)(ptr - dataptr);
              }
            }
            
            // save attribute
            attrib_val->val = val;
            attrib_val->dataptr = dataptr;
          }
          
          // emit entry
          if (entry_consptr != 0){
            SLLQueuePush_N(entry_consptr->first_child, entry_consptr->last_child,
                           entry, next_sibling);
            entry_consptr->child_count += 1;
            entry->parent = entry_consptr;
          }
          entry_count += 1;
          entry->abbrev_decl = abbrev_decl;
          entry->attrib_vals = attrib_vals;
          
          // move consptr down if has children
          if (abbrev_decl->has_children){
            entry_consptr = entry;
          }
          
          skip_entry:;
        }
      }
      exit_unit_loop:;
      
      // TODO: notice errors, emit them, and exit loop here
      if (decoding_error){
        break;
      }
      
      // extract root attributes
      U64 language = 0;
      U64 str_offsets_base = 0;
      U64 line_info_offset = 0;
      U64 vbase = 0;
      U64 addr_base = 0;
      U64 rnglists_base = 0;
      U64 loclists_base = 0;
      if (entry_root != 0){
        
        // pull out attributes
        DWARF_AbbrevDecl *root_abbrev_decl = entry_root->abbrev_decl;
        DWARF_AbbrevAttribSpec *attrib_specs = root_abbrev_decl->attrib_specs;
        DWARF_InfoAttribVal *attrib_vals = entry_root->attrib_vals;
        U32 attrib_count = root_abbrev_decl->attrib_count;
        
        // examine each attribute
        DWARF_AbbrevAttribSpec *attrib_spec = attrib_specs;
        DWARF_InfoAttribVal *attrib_val = attrib_vals;
        for (U32 i = 0; i < attrib_count; i += 1, attrib_spec += 1, attrib_val += 1){
          
          // determine if there is a root attribute to extract here
          U64 *target_u64 = 0;
          switch (attrib_spec->name){
            case DWARF_AttributeName_language:         target_u64 = &language;         break;
            case DWARF_AttributeName_str_offsets_base: target_u64 = &str_offsets_base; break;
            case DWARF_AttributeName_stmt_list:        target_u64 = &line_info_offset; break;
            case DWARF_AttributeName_low_pc:           target_u64 = &vbase;            break;
            case DWARF_AttributeName_addr_base:        target_u64 = &addr_base;        break;
            case DWARF_AttributeName_rnglists_base:    target_u64 = &rnglists_base;    break;
            case DWARF_AttributeName_loclists_base:    target_u64 = &loclists_base;    break;
          }
          
          // set target from attrib value
          if (target_u64 != 0){
            *target_u64 = attrib_val->val;
          }
        }
      }
      
      // allocate unit
      DWARF_InfoUnit *unit = push_array(arena, DWARF_InfoUnit, 1);
      
      // fill & emit unit
      SLLQueuePush(unit_first, unit_last, unit);
      unit_count += 1;
      // [header]
      unit->dwarf_version = version;
      unit->offset_size   = offset_size;
      unit->address_size  = address_size;
      // [root attributes]
      unit->language = (DWARF_Language)language;
      unit->str_offsets_base = str_offsets_base;
      unit->line_info_offset = line_info_offset;
      unit->vbase = vbase;
      unit->addr_base = addr_base;
      unit->rnglists_base = rnglists_base;
      unit->loclists_base = loclists_base;
      // [entries]
      unit->entry_root = entry_root;
      unit->entry_count = entry_count;
      
    }
    
    // set ptr to end of this unit
    ptr = unit_opl;
  }
  
  // fill result
  DWARF_InfoParsed *result = push_array(arena, DWARF_InfoParsed, 1);
  result->unit_first = unit_first;
  result->unit_last = unit_last;
  result->unit_count = unit_count;
  result->decoding_error = decoding_error;
  return(result);
}

static DWARF_AbbrevParsed*
dwarf_abbrev_from_data(Arena *arena, String8 data, DWARF_AbbrevParams *params){
  /* .debug_abbrev
  ** Layout
  **  List(Tag)
  **  Tag       = { id:ULEB128, tag:ULEB128, has_children:B8, ListNullTerminated(Attribute) }
  **  Attribute = { name:ULEB128, form:ULEB128, (val:SLEB128)? }
  */
  
  // unit index range to extract
  U64 unit_idx_min = params->unit_idx_min;
  U64 unit_idx_max = params->unit_idx_max;
  
  // empty unit list
  DWARF_AbbrevUnit *unit_first = 0;
  DWARF_AbbrevUnit *unit_last = 0;
  U64 unit_count = 0;
  B32 decoding_error = 0;
  
  // whole section loop
  U64 unit_idx = 0;
  U8 *ptr = data.str;
  U8 *opl = data.str + data.size;
  for (;ptr < opl; unit_idx += 1){
    
    // early escape on unit idx
    if (unit_idx > unit_idx_max){
      break;
    }
    
    // determine whether to full parse this unit
    B32 full_parse = (unit_idx_min <= unit_idx);
    
    // save unit offset
    U64 abbrev_unit_offset = (U64)(ptr - data.str);
    
    // allocate unit
    DWARF_AbbrevUnit *unit = push_array(arena, DWARF_AbbrevUnit, 1);
    
    // empty abbrev list
    DWARF_AbbrevDecl *abbrev_first = 0;
    DWARF_AbbrevDecl *abbrev_last = 0;
    U64 abbrev_count = 0;
    
    // abbrev decl loop
    for (;ptr < opl;){
      B32 success = 1;
      
      // mark abbrev_code field
      U8 *abbrev_code_ptr = ptr;
      DWARF_LEB128_ADV(ptr, opl, success);
      
      // null abbrev code means end of unit
      if (success && *abbrev_code_ptr == 0){
        break;
      }
      
      // mark tag
      U8 *tag_ptr = ptr;
      DWARF_LEB128_ADV(ptr, opl, success);
      U8 *end_tag_ptr = ptr;
      
      // extract has_children
      B8 has_children = 0;
      if (ptr < opl){
        has_children = *ptr;
        ptr += 1;
      }
      else{
        success = 0;
      }
      
      // count attributes
      U8 *attrib_start_ptr = ptr;
      U32 attrib_count = 0;
      B32 has_implicit_const = 0;
      if (success){
        for (;;){
          // decode normal attribute layout
          U8 *attrib_name = ptr;
          DWARF_LEB128_ADV(ptr, opl, success);
          U8 *attrib_form = ptr;
          DWARF_LEB128_ADV(ptr, opl, success);
          
          // handle special case implicit_const
          if (success && *attrib_form == (U8)DWARF_AttributeForm_implicit_const){
            DWARF_LEB128_ADV(ptr, opl, success);
            has_implicit_const = 1;
          }
          
          // termination conditions
          if (ptr == opl ||
              (*attrib_name == 0 && *attrib_form == 0)){
            break;
          }
          
          // increment
          attrib_count += 1;
        }
      }
      
      // build the abbreviation declaration
      if (full_parse && success){
        
        // allocate abbrev
        DWARF_AbbrevDecl *abbrev = push_array(arena, DWARF_AbbrevDecl, 1);
        DWARF_AbbrevAttribSpec *attribs =
          push_array_no_zero(arena, DWARF_AbbrevAttribSpec, attrib_count);
        U64 *implicit_const = 0;
        if (has_implicit_const){
          implicit_const = push_array(arena, U64, attrib_count);
        }
        
        // extract abbrev fields
        U32 abbrev_code = dwarf_leb128_decode_U32(abbrev_code_ptr, tag_ptr);
        U32 tag = dwarf_leb128_decode_U32(tag_ptr, end_tag_ptr);
        
        U8 *attrib_ptr = attrib_start_ptr;
        DWARF_AbbrevAttribSpec *attrib = attribs;
        for (U32 i = 0; i < attrib_count; i += 1, attrib += 1){
          // mark attribute fields
          U8 *attrib_name = attrib_ptr;
          DWARF_LEB128_ADV_NOCAP(attrib_ptr);
          U8 *attrib_form = attrib_ptr;
          DWARF_LEB128_ADV_NOCAP(attrib_ptr);
          
          // extract attribute fields
          U32 name = dwarf_leb128_decode_U32(attrib_name, attrib_form);
          U32 form = dwarf_leb128_decode_U32(attrib_form, attrib_ptr);
          
          // fill attribute spec
          attrib->name = (DWARF_AttributeName)name;
          attrib->form = (DWARF_AttributeForm)form;
          
          // handle special case implicit_const
          if (form == DWARF_AttributeForm_implicit_const){
            U8 *attrib_value = attrib_ptr;
            DWARF_LEB128_ADV_NOCAP(attrib_ptr);
            S64 value = dwarf_leb128_decode_S64(attrib_form, attrib_ptr);
            implicit_const[i] = value;
          }
        }
        
        // fill abbreviation
        SLLQueuePush(abbrev_first, abbrev_last, abbrev);
        abbrev_count += 1;
        abbrev->abbrev_code = abbrev_code;
        abbrev->tag = (DWARF_Tag)tag;
        abbrev->has_children = has_children;
        abbrev->attrib_count = attrib_count;
        abbrev->attrib_specs = attribs;
        abbrev->implicit_const = implicit_const;
      }
      
      // handle failure
      if (!success){
        // TODO: emit error message
        decoding_error = 1;
        goto done_parse;
      }
    }
    
    // fill unit
    if (full_parse){
      SLLQueuePush(unit_first, unit_last, unit);
      unit_count += 1;
      unit->offset = abbrev_unit_offset;
      unit->first  = abbrev_first;
      unit->last   = abbrev_last;
      unit->count  = abbrev_count;
    }
  }
  
  done_parse:;
  
  // fill result
  DWARF_AbbrevParsed *result = push_array(arena, DWARF_AbbrevParsed, 1);
  result->unit_first = unit_first;
  result->unit_last  = unit_last;
  result->unit_count = unit_count;
  result->decoding_error = decoding_error;
  return(result);
}
#endif
