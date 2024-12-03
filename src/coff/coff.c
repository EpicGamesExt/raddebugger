// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
coff_is_big_obj(String8 data)
{
  B32 is_big_obj = 0;
  if (data.size >= sizeof(COFF_HeaderBigObj)) {
    COFF_HeaderBigObj *big_header = (COFF_HeaderBigObj*)(data.str);
    is_big_obj = big_header->sig1 == COFF_MachineType_UNKNOWN && 
      big_header->sig2 == max_U16 &&
      big_header->version >= COFF_MIN_BIG_OBJ_VERSION &&
      MemoryCompare(big_header->magic, coff_big_obj_magic, sizeof(big_header->magic)) == 0;
  }
  return is_big_obj;
}

internal B32
coff_is_obj(String8 data)
{
  B32 is_obj = 0;
  
  if (data.size >= sizeof(COFF_Header)) {
    COFF_Header *header = (COFF_Header*)(data.str);
    
    // validate machine
    B32 is_machine_type_valid = 0;
    switch (header->machine) {
      case COFF_MachineType_UNKNOWN:
      case COFF_MachineType_X86:    case COFF_MachineType_X64:
      case COFF_MachineType_AM33:   case COFF_MachineType_ARM:
      case COFF_MachineType_ARM64:  case COFF_MachineType_ARMNT:
      case COFF_MachineType_EBC:    case COFF_MachineType_IA64:
      case COFF_MachineType_M32R:   case COFF_MachineType_MIPS16:
      case COFF_MachineType_MIPSFPU:case COFF_MachineType_MIPSFPU16:
      case COFF_MachineType_POWERPC:case COFF_MachineType_POWERPCFP:
      case COFF_MachineType_R4000:  case COFF_MachineType_RISCV32:
      case COFF_MachineType_RISCV64:case COFF_MachineType_RISCV128:
      case COFF_MachineType_SH3:    case COFF_MachineType_SH3DSP:
      case COFF_MachineType_SH4:    case COFF_MachineType_SH5:
      case COFF_MachineType_THUMB:  case COFF_MachineType_WCEMIPSV2:
      {
        is_machine_type_valid = 1;
      }break;
    }
    
    if (is_machine_type_valid) {
      // validate section count
      U64 section_count = header->section_count;
      U64 section_hdr_opl_off = sizeof(*header) + section_count*sizeof(COFF_SectionHeader);
      if (data.size >= section_hdr_opl_off) {
        
        COFF_SectionHeader *section_hdrs = (COFF_SectionHeader*)(data.str + sizeof(*header));
        COFF_SectionHeader *section_hdr_opl = section_hdrs + section_count;
        
        // validate section ranges
        B32 is_sect_range_valid = 1;
        for (COFF_SectionHeader *sec_hdr = section_hdrs;
             sec_hdr < section_hdr_opl;
             sec_hdr += 1) {
          if (!(sec_hdr->flags & COFF_SectionFlag_CNT_UNINITIALIZED_DATA)) {
            U64 min = sec_hdr->foff;
            U64 max = min + sec_hdr->fsize;
            if (sec_hdr->fsize > 0 && !(section_hdr_opl_off <= min && min <= max && max <= data.size)) {
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
                    symbol_table_opl_off <= data.size);
        }
      }
    }
  }
  
  return is_obj;
}

internal COFF_HeaderInfo
coff_header_info_from_data(String8 data)
{
  COFF_HeaderInfo info = {0};
  if (coff_is_big_obj(data)) {
    COFF_HeaderBigObj *big_header = (COFF_HeaderBigObj*)data.str;
    info.type                     = COFF_DataType_BIG_OBJ;
    info.machine                  = big_header->machine;
    info.section_array_off        = sizeof(COFF_HeaderBigObj);
    info.section_count_no_null    = big_header->section_count;
    info.string_table_off         = big_header->symbol_table_foff + sizeof(COFF_Symbol32) * big_header->symbol_count;
    info.symbol_size              = sizeof(COFF_Symbol32);
    info.symbol_off               = big_header->symbol_table_foff;
    info.symbol_count             = big_header->symbol_count;
  } else if (coff_is_obj(data)) {
    COFF_Header *header        = (COFF_Header*)data.str;
    info.type                  = COFF_DataType_OBJ;
    info.machine               = header->machine;
    info.section_array_off     = sizeof(COFF_Header);
    info.section_count_no_null = header->section_count;
    info.string_table_off      = header->symbol_table_foff + sizeof(COFF_Symbol16) * header->symbol_count;
    info.symbol_size           = sizeof(COFF_Symbol16);
    info.symbol_off            = header->symbol_table_foff;
    info.symbol_count          = header->symbol_count;
  }
  return info;
}

internal U64
coff_align_size_from_section_flags(COFF_SectionFlags flags)
{
  U32 align = 0;
  U32 align_index = COFF_SectionFlags_Extract_ALIGN(flags);
  switch (align_index) {
    default: break;
    case 0:                           align = 1;    break; // alignment isn't specified, default to 1
    case COFF_SectionAlign_1BYTES:    align = 1;    break;
    case COFF_SectionAlign_2BYTES:    align = 2;    break;
    case COFF_SectionAlign_4BYTES:    align = 4;    break;
    case COFF_SectionAlign_8BYTES:    align = 8;    break;
    case COFF_SectionAlign_16BYTES:   align = 16;   break;
    case COFF_SectionAlign_32BYTES:   align = 32;   break;
    case COFF_SectionAlign_64BYTES:   align = 64;   break;
    case COFF_SectionAlign_128BYTES:  align = 128;  break;
    case COFF_SectionAlign_256BYTES:  align = 256;  break;
    case COFF_SectionAlign_512BYTES:  align = 512;  break;
    case COFF_SectionAlign_1024BYTES: align = 1024; break;
    case COFF_SectionAlign_2048BYTES: align = 2048; break;
    case COFF_SectionAlign_4096BYTES: align = 4096; break;
    case COFF_SectionAlign_8192BYTES: align = 8192; break;
  }
  return align;
}

internal COFF_SectionFlags
coff_section_flag_from_align_size(U64 align)
{
  COFF_SectionFlags flags = 0;
  switch (align) {
  case 1:    flags = COFF_SectionAlign_1BYTES;    break;
  case 2:    flags = COFF_SectionAlign_2BYTES;    break;
  case 4:    flags = COFF_SectionAlign_4BYTES;    break;
  case 8:    flags = COFF_SectionAlign_8BYTES;    break;
  case 16:   flags = COFF_SectionAlign_16BYTES;   break;
  case 32:   flags = COFF_SectionAlign_32BYTES;   break;
  case 64:   flags = COFF_SectionAlign_64BYTES;   break;
  case 128:  flags = COFF_SectionAlign_128BYTES;  break;
  case 256:  flags = COFF_SectionAlign_256BYTES;  break;
  case 512:  flags = COFF_SectionAlign_512BYTES;  break;
  case 1024: flags = COFF_SectionAlign_1024BYTES; break;
  case 2048: flags = COFF_SectionAlign_2048BYTES; break;
  case 4096: flags = COFF_SectionAlign_4096BYTES; break;
  case 8192: flags = COFF_SectionAlign_8192BYTES; break;
  }
  flags <<= COFF_SectionFlag_ALIGN_SHIFT;
  return flags;
}

internal COFF_SymbolValueInterpType
coff_interp_symbol(U32 section_number, U32 value, COFF_SymStorageClass storage_class)
{
  if (storage_class == COFF_SymStorageClass_SECTION && section_number == COFF_SYMBOL_UNDEFINED_SECTION) {
    return COFF_SymbolValueInterp_UNDEFINED;
  }
  if (storage_class == COFF_SymStorageClass_EXTERNAL && value == 0 && section_number == COFF_SYMBOL_UNDEFINED_SECTION) {
    return COFF_SymbolValueInterp_UNDEFINED;
  }
  if (storage_class == COFF_SymStorageClass_EXTERNAL && value != 0 && section_number == COFF_SYMBOL_UNDEFINED_SECTION) {
    return COFF_SymbolValueInterp_COMMON;
  }
  if (section_number == COFF_SYMBOL_ABS_SECTION) {
    return COFF_SymbolValueInterp_ABS;
  }
  if (section_number == COFF_SYMBOL_DEBUG_SECTION) {
    return COFF_SymbolValueInterp_DEBUG;
  }
  if (storage_class == COFF_SymStorageClass_WEAK_EXTERNAL) {
    return COFF_SymbolValueInterp_WEAK;
  }
  return COFF_SymbolValueInterp_REGULAR;
}

internal U64
coff_foff_from_voff(COFF_SectionHeader *sections, U64 section_count, U64 voff)
{
  U64 foff = 0;
  for(U64 sect_idx = 0; sect_idx < section_count; sect_idx += 1)
  {
    COFF_SectionHeader *sect = &sections[sect_idx];
    if(sect->voff <= voff && voff < sect->voff+sect->vsize)
    {
      if(!(sect->flags & COFF_SectionFlag_CNT_UNINITIALIZED_DATA))
      {
        foff = sect->foff + (voff - sect->voff);
      }
      break;
    }
  }
  return foff;
}

internal COFF_SectionHeader *
coff_section_header_from_num(String8 data, U64 section_headers_off, U64 n)
{
  COFF_SectionHeader *result = &coff_section_header_nil;
  if(1 <= n && section_headers_off + n*sizeof(COFF_SectionHeader) <= data.size)
  {
    result = (COFF_SectionHeader*)(data.str + section_headers_off + (n-1)*sizeof(COFF_SectionHeader));
  }
  return result;
}

internal String8
coff_section_header_get_name(COFF_SectionHeader *header, String8 coff_data, U64 string_table_base)
{
  U64 size = 0;
  for (; size < sizeof(header->name); size += 1) {
    if (header->name[size] == '\0') {
      break;
    }
  }
  String8 name = str8(header->name, size);
  
  if (name.str[0] == '/') {
    String8 ascii_name_offset = str8_skip(name, 1);
    U64 name_offset = u64_from_str8(ascii_name_offset, 10);
    
    name_offset += string_table_base;
    if (name_offset < coff_data.size) {
      char *ptr = (char *)coff_data.str + name_offset;
      name = str8_cstring(ptr);
    }
  }
  
  return name;
}

internal void
coff_parse_section_name(String8 full_name, String8 *name_out, String8 *postfix_out)
{
  // dollar sign has multiple interpretations that depend on the type of the section.
  // 1. when section contains code/data it indicates section precedence
  // 2. when section starts with .debug it indicates type of data inside the section
  //    T: Types
  //    S: Symbols
  //    P: Precompiled types
  //    F: FPO data
  //    H: Clang extension produced with /debug:ghash, array of type hashes
  *name_out = full_name;
  *postfix_out = str8_lit("");
  for (U64 i = 0; i < full_name.size; ++i) {
    if (full_name.str[i] == '$') {
      *name_out = str8(full_name.str, i);
      *postfix_out = str8(full_name.str + i + 1, full_name.size - i - 1);
      
      // TLS sections don't have a postfix but we still have to sort them based
      // on dollar sign so they are sloted between CRT's _tls_start and _tls_end sections.
      if (str8_match(*name_out, str8_lit(".tls"), 0) && postfix_out->size == 0) {
        *postfix_out = str8_lit("$");
      }
      
      break;
    }
  }
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

////////////////////////////////

internal String8
coff_read_symbol_name(String8 data, U64 string_table_base_offset, COFF_SymbolName *name)
{
  String8 name_str = str8_lit("");
  if (name->long_name.zeroes == 0) {
    U64 string_table_offset = string_table_base_offset + name->long_name.string_table_offset;
    str8_deserial_read_cstr(data, string_table_offset, &name_str);
  } else {
    U32 i;
    for (i = 0; i < sizeof(name->short_name); ++i) {
      if (name->short_name[i] == '\0') {
        break;
      }
    }
    name_str = str8(name->short_name, i);
  }
  return name_str;
}

internal void
coff_symbol32_from_coff_symbol16(COFF_Symbol32 *sym32, COFF_Symbol16 *sym16)
{
  sym32->name             = sym16->name;
  sym32->value            = sym16->value;
  if (sym16->section_number == COFF_SYMBOL_DEBUG_SECTION_16) {
    sym32->section_number = COFF_SYMBOL_DEBUG_SECTION;
  } else if (sym16->section_number == COFF_SYMBOL_ABS_SECTION_16) {
    sym32->section_number = COFF_SYMBOL_ABS_SECTION;
  } else {
    sym32->section_number = (U32)sym16->section_number;
  }
  sym32->type.v           = sym16->type.v;
  sym32->storage_class    = sym16->storage_class;
  sym32->aux_symbol_count = sym16->aux_symbol_count;
}

internal COFF_Symbol32Array
coff_symbol_array_from_data_16(Arena *arena, String8 data, U64 symbol_array_off, U64 symbol_count)
{
  COFF_Symbol32Array result;
  result.count = symbol_count;
  result.v = push_array_no_zero_aligned(arena, COFF_Symbol32, result.count, 8);
  
  COFF_Symbol16 *sym16_arr = (COFF_Symbol16 *)(data.str + symbol_array_off);
  for (U64 isymbol = 0; isymbol < symbol_count; isymbol += 1) {
    // read header symbol
    COFF_Symbol16 *sym16 = &sym16_arr[isymbol];
    
    // convert to 32bit
    COFF_Symbol32 *sym32 = &result.v[isymbol];
    coff_symbol32_from_coff_symbol16(sym32, sym16);
    
    if (isymbol + 1 + sym16->aux_symbol_count > symbol_count) {
      Assert(!"aux symbols out of bounds");
    }
    
    // copy aux symbols
    for (U64 iaux = 0; iaux < sym16->aux_symbol_count; iaux += 1) {
      COFF_Symbol16 *aux16 = sym16 + iaux + 1;
      COFF_Symbol32 *aux32 = sym32 + iaux + 1; // 32bit COFF uses 16bit aux symbols
      MemoryCopy(aux32, aux16, sizeof(COFF_Symbol16));
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
coff_symbol_array_from_data(Arena *arena, String8 data, U64 symbol_off, U64 symbol_count, U64 symbol_size)
{
  COFF_Symbol32Array result = {0};
  switch (symbol_size) {
    case sizeof(COFF_Symbol16): result = coff_symbol_array_from_data_16(arena, data, symbol_off, symbol_count); break;
    case sizeof(COFF_Symbol32): result = coff_symbol_array_from_data_32(arena, data, symbol_off, symbol_count); break;
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

internal COFF_RelocInfo
coff_reloc_info_from_section_header(String8 data, COFF_SectionHeader *header)
{
  COFF_RelocInfo result = {0};
  if (header->flags & COFF_SectionFlag_LNK_NRELOC_OVFL && header->reloc_count == max_U16) {
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

internal U64
coff_apply_size_from_reloc_x64(COFF_RelocTypeX64 x)
{
  switch (x) {
    case COFF_RelocTypeX64_ABS:      return 4;
    case COFF_RelocTypeX64_ADDR64:   return 8;
    case COFF_RelocTypeX64_ADDR32:   return 4;
    case COFF_RelocTypeX64_ADDR32NB: return 4;
    case COFF_RelocTypeX64_REL32:    return 4;
    case COFF_RelocTypeX64_REL32_1:  return 4;
    case COFF_RelocTypeX64_REL32_2:  return 4;
    case COFF_RelocTypeX64_REL32_3:  return 4;
    case COFF_RelocTypeX64_REL32_4:  return 4;
    case COFF_RelocTypeX64_REL32_5:  return 4;
    case COFF_RelocTypeX64_SECTION:  return 2;
    case COFF_RelocTypeX64_SECREL:   return 4;
    case COFF_RelocTypeX64_SREL32:   return 4;

    case COFF_RelocTypeX64_SECREL7:
    case COFF_RelocTypeX64_TOKEN:
    case COFF_RelocTypeX64_PAIR:
    case COFF_RelocTypeX64_SSPAN32:
      NotImplemented;
    break;
  }
  return 0;
}

internal U64
coff_apply_size_from_reloc_x86(COFF_RelocTypeX86 x)
{
  switch (x) {
    case COFF_RelocTypeX86_ABS:      return 4;
    case COFF_RelocTypeX86_DIR16:    return 2;
    case COFF_RelocTypeX86_REL16:    return 2;
    case COFF_RelocTypeX86_DIR32:    return 4;
    case COFF_RelocTypeX86_DIR32NB:  return 4;
    case COFF_RelocTypeX86_SEG12:    return 0;
    case COFF_RelocTypeX86_SECTION:  return 2;
    case COFF_RelocTypeX86_SECREL:   return 4;
    case COFF_RelocTypeX86_TOKEN:    return 4;
    case COFF_RelocTypeX86_SECREL7:  return 1;
    case COFF_RelocTypeX86_REL32:    return 4;

    case COFF_RelocTypeX86_UNKNOWN0:
    case COFF_RelocTypeX86_UNKNOWN2:
    case COFF_RelocTypeX86_UNKNOWN3:
    case COFF_RelocTypeX86_UNKNOWN4:
    case COFF_RelocTypeX86_UNKNOWN6:
    case COFF_RelocTypeX86_UNKNOWN7:
    case COFF_RelocTypeX86_UNKNOWN8:
    case COFF_RelocTypeX86_UNKNOWN9:
      NotImplemented;
    break;
  }
  return 0;
}

internal U64
coff_apply_size_from_reloc(COFF_MachineType machine, COFF_RelocType x)
{
  switch (machine) {
    case COFF_MachineType_X64: return coff_apply_size_from_reloc_x64(x);
    case COFF_MachineType_X86: return coff_apply_size_from_reloc_x86(x);
    default: NotImplemented;
  }
  return 0;
}

internal U64
coff_word_size_from_machine(COFF_MachineType machine)
{
  U64 result = 0;
  switch (machine) {
  case COFF_MachineType_X64: result = 8; break;
  case COFF_MachineType_X86: result = 4; break;
  }
  return result;
}

internal U64
coff_default_exe_base_from_machine(COFF_MachineType machine)
{
  U64 exe_base = 0;
  switch (coff_word_size_from_machine(machine)) {
  case 4: exe_base = 0x400000;    break;
  case 8: exe_base = 0x140000000; break;
  }
  return exe_base;
}

internal U64
coff_default_dll_base_from_machine(COFF_MachineType machine)
{
  U64 dll_base = 0;
  switch (coff_word_size_from_machine(machine)) {
  case 4: dll_base = 0x10000000;  break;
  case 8: dll_base = 0x180000000; break;
  }
  return dll_base;
}

internal String8
coff_make_import_lookup(Arena *arena, U16 hint, String8 name)
{
  U64 buffer_size = sizeof(hint) + (name.size + 1);
  U8 *buffer = push_array(arena, U8, buffer_size);
  *(U16*)buffer = hint;
  MemoryCopy(buffer + sizeof(hint), name.str, name.size);
  buffer[buffer_size - 1] = 0;
  String8 result = str8(buffer, buffer_size);
  return result;
}

internal U32
coff_make_ordinal_32(U16 hint)
{
  U32 ordinal = (1 << 31) | hint;
  return ordinal;
}

internal U64
coff_make_ordinal_64(U16 hint)
{
  U64 ordinal = (1ULL << 63) | hint;
  return ordinal;
}

////////////////////////////////

internal String8
coff_make_import_header_by_name(Arena                *arena,
                                String8               dll_name,
                                COFF_MachineType      machine,
                                COFF_TimeStamp        time_stamp,
                                String8               name,
                                U16                   hint,
                                COFF_ImportHeaderType type)
{
  struct {
    U16              sig1;
    U16              sig2;
    U16              version;
    COFF_MachineType machine;
    COFF_TimeStamp   time_stamp;
    U32              sizeof_data;
    U16              hint_ordinal;
    U16              flags;
  } import_header = {
    COFF_MachineType_UNKNOWN, // sig1
    max_U16, // sig2
    0,       // version
    machine, 
    time_stamp,
    safe_cast_u32(name.size + dll_name.size + 2), // sizeof_data
    0,       // hint_ordinal
    0,       // flags
  };
  
  import_header.flags |= (U16)(type & COFF_IMPORT_HEADER_TYPE_MASK) << COFF_IMPORT_HEADER_TYPE_SHIFT;
  import_header.flags |= COFF_ImportHeaderNameType_NAME << COFF_IMPORT_HEADER_NAME_TYPE_SHIFT;
  import_header.hint_ordinal = hint;
  
  // alloc memory
  U64 buffer_size = sizeof(import_header) + import_header.sizeof_data;
  U8 *buffer = push_array_no_zero(arena, U8, buffer_size);
  
  // copy header
  MemoryCopy(buffer, &import_header, sizeof(import_header));
  
  // copy function name
  U8 *func_name = buffer + sizeof(import_header);
  MemoryCopy(func_name, name.str, name.size);
  func_name[name.size] = 0;
  
  // copy dll name
  U8 *dll_name_buffer = buffer + sizeof(import_header) + name.size + 1;
  MemoryCopy(dll_name_buffer, dll_name.str, dll_name.size);
  dll_name_buffer[dll_name.size] = 0;
  
  String8 import_data = str8(buffer, buffer_size);
  return import_data;
}

internal String8
coff_make_import_header_by_ordinal(Arena                *arena,
                                   String8               dll_name,
                                   COFF_MachineType      machine,
                                   COFF_TimeStamp        time_stamp,
                                   U16                   ordinal,
                                   COFF_ImportHeaderType type)
{
  struct {
    U16              sig1;
    U16              sig2;
    U16              version;
    COFF_MachineType machine;
    COFF_TimeStamp   time_stamp;
    U32              sizeof_data;
    U16              hint_ordinal;
    U16              flags;
  } import_header = {
    COFF_MachineType_UNKNOWN, // sig1
    max_U16, // sig2
    0, // version
    machine, 
    time_stamp,
    safe_cast_u32(/* name.size + */ dll_name.size + 2), // sizeof_data
    0, // hint_ordinal
    0, // flags
  };
  
  import_header.flags |= (U16)(type & COFF_IMPORT_HEADER_TYPE_MASK) << COFF_IMPORT_HEADER_TYPE_SHIFT;
  import_header.flags |= COFF_ImportHeaderNameType_ORDINAL << COFF_IMPORT_HEADER_NAME_TYPE_SHIFT;
  import_header.hint_ordinal = ordinal;
  
  // alloc memory
  U64 buffer_size = sizeof(import_header) + import_header.sizeof_data;
  U8 *buffer = push_array_no_zero(arena, U8, buffer_size);
  
  // copy header
  MemoryCopyStruct(buffer, &import_header);
  
  // no function name write zero
  U8 *func_name = buffer + sizeof(import_header);
  func_name[0] = 0;
  
  // copy dll name
  U8 *dll_name_buffer = buffer + sizeof(import_header) + /* name.size */ + 1;
  MemoryCopy(dll_name_buffer, dll_name.str, dll_name.size);
  dll_name_buffer[dll_name.size] = 0;
  
  String8 import_data = str8(buffer, buffer_size);
  return import_data;
}

////////////////////////////////
//~ Resources

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
  String8 result = coff_resource_string_from_str16(arena, string16);
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

internal B32
coff_resource_id_is_equal(COFF_ResourceID a, COFF_ResourceID b)
{
  B32 is_equal = 0;
  if (a.type == b.type) {
    switch (a.type) {
      case COFF_ResourceIDType_NULL: break;
      case COFF_ResourceIDType_NUMBER: is_equal = (a.u.number == b.u.number);            break;
      case COFF_ResourceIDType_STRING: is_equal = str8_match(a.u.string, b.u.string, 0); break;
      default: Assert(!"invalid resource id type");
    }
  }
  return is_equal;
}

internal COFF_ResourceID
coff_utf8_resource_id_from_utf16(Arena *arena, COFF_ResourceID_16 *id_16)
{
  COFF_ResourceID id = {0};
  id.type = id_16->type;
  switch (id_16->type) {
    case COFF_ResourceIDType_NULL: break;
    case COFF_ResourceIDType_NUMBER: {
      id.u.number = id_16->u.number;
    } break;
    case COFF_ResourceIDType_STRING: {
      id.u.string = str8_from_16(arena, id_16->u.string);
    } break;
    default: InvalidPath;
  }
  return id;
}

internal U64
coff_read_resource_id_utf16(String8 data, U64 off, COFF_ResourceID_16 *id_out)
{
  U64 cursor = off;
  
  U16 flag = 0;
  str8_deserial_read_struct(data, cursor, &flag);
  
  if (flag == max_U16) {
    id_out->type = COFF_ResourceIDType_NUMBER;
    cursor += sizeof(flag);
    cursor += str8_deserial_read_struct(data, cursor, &id_out->u.number);
  } else {
    id_out->type = COFF_ResourceIDType_STRING;
    cursor += str8_deserial_read_windows_utf16_string16(data, cursor, &id_out->u.string);
  }
  
  U64 read_size = cursor - off;
  read_size     = AlignPow2(read_size, COFF_RES_ALIGN);
  return read_size;
}

internal U64
coff_read_resource(String8 raw_res, U64 off, Arena *arena, COFF_Resource *res_out)
{
  String8 raw_header    = str8_skip(raw_res, off);
  U64     header_cursor = 0;

  // prefix
  COFF_ResourceHeaderPrefix prefix = {0};
  header_cursor += str8_deserial_read_struct(raw_header, header_cursor, &prefix);

  Assert(prefix.header_size >= sizeof(COFF_ResourceHeaderPrefix));
  raw_header = str8_prefix(raw_header, prefix.header_size);

  // header
  COFF_ResourceID_16 type_16 = {0};
  COFF_ResourceID_16 name_16 = {0};
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
  U64 read_size = Max(prefix.header_size, sizeof(prefix)) + AlignPow2(prefix.data_size, COFF_RES_ALIGN);
  return read_size;
}

internal COFF_ResourceList
coff_resource_list_from_data(Arena *arena, String8 data)
{
  COFF_ResourceList list = {0};
  U64 cursor;
  for (cursor = 0 ; cursor < data.size; ) {
    COFF_ResourceNode *node = push_array(arena, COFF_ResourceNode, 1);
    cursor += coff_read_resource(data, cursor, arena, &node->data);
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
  case COFF_ResourceIDType_NULL: break;
  case COFF_ResourceIDType_NUMBER: {
    result = coff_resource_number_from_u16(arena, id.u.number);
  } break;
  case COFF_ResourceIDType_STRING: {
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
  U64 align_size = AlignPow2(list.total_size, COFF_RES_ALIGN) - list.total_size;
  U8 *align      = push_array(scratch.arena, U8, align_size);
  str8_list_push(scratch.arena, &list, str8(align, align_size));

  // join
  String8 res = str8_list_join(arena, &list, 0);

  scratch_end(scratch);
  return res;
}

////////////////////////////////

internal COFF_DataType
coff_data_type_from_data(String8 data)
{
  B32 is_big_obj = coff_is_big_obj(data);
  if (is_big_obj) {
    return COFF_DataType_BIG_OBJ;
  }
  B32 is_import = coff_is_import(data);
  if (is_import) {
    return COFF_DataType_IMPORT;
  }
  return COFF_DataType_OBJ;
}

internal B32
coff_is_import(String8 data)
{
  B32 is_import = 0;
  if (data.size >= sizeof(U16)*2) {
    U16 *sig1 = (U16*)data.str;
    U16 *sig2 = sig1 + 1;
    is_import = *sig1 == COFF_MachineType_UNKNOWN && *sig2 == 0xffff;
  }
  return is_import;
}

internal B32
coff_is_archive(String8 data)
{
  B32 is_archive = 0;
  U8 sig[sizeof(g_coff_archive_sig)];
  if (str8_deserial_read_struct(data, 0, &sig) == sizeof(sig)) {
    is_archive = MemoryCompare(&sig[0], &g_coff_archive_sig[0], sizeof(g_coff_archive_sig)) == 0;
  }
  return is_archive;
}

internal B32
coff_is_thin_archive(String8 data)
{
  B32 is_archive = 0;
  U8 sig[sizeof(g_coff_thin_archive_sig)];
  if (str8_deserial_read_struct(data, 0, &sig) == sizeof(sig)) {
    is_archive = MemoryCompare(&sig[0], &g_coff_thin_archive_sig[0], sizeof(g_coff_thin_archive_sig)) == 0;
  }
  return is_archive;
}

internal U64
coff_read_archive_member_header(String8 data, U64 offset, COFF_ArchiveMemberHeader *header_out)
{
#define NAME_SIZE 16
#define DATE_SIZE 12
#define USER_ID_SIZE 6
#define GROUP_ID_SIZE 6
#define MODE_SIZE 8
#define SIZE_SIZE 10
#define TOTAL_SIZE (NAME_SIZE + DATE_SIZE + USER_ID_SIZE + GROUP_ID_SIZE + MODE_SIZE + SIZE_SIZE)
  
  if (str8_deserial_get_raw_ptr(data, offset, TOTAL_SIZE) == NULL) {
    return 0;
  }
  
  U64 read_offset = offset;
  
  U8 *name = (U8 *)str8_deserial_get_raw_ptr(data, read_offset, NAME_SIZE);
  read_offset += NAME_SIZE;
  
  U8 *date = (U8 *)str8_deserial_get_raw_ptr(data, read_offset, DATE_SIZE);
  read_offset += DATE_SIZE;
  
  U8 *user_id = (U8 *)str8_deserial_get_raw_ptr(data, read_offset, USER_ID_SIZE);
  read_offset += USER_ID_SIZE;
  
  U8 *group_id = (U8 *)str8_deserial_get_raw_ptr(data, read_offset, GROUP_ID_SIZE);
  read_offset += GROUP_ID_SIZE;
  
  U8 *mode = (U8 *)str8_deserial_get_raw_ptr(data, read_offset, MODE_SIZE);
  read_offset += MODE_SIZE;
  
  U8 *size = (U8 *)str8_deserial_get_raw_ptr(data, read_offset, SIZE_SIZE);
  read_offset += SIZE_SIZE;
  
  U8 end[] = { 0, 0 };
  read_offset += str8_deserial_read_array(data, read_offset, &end[0], ArrayCount(end));
  
  U64 i;
  for (i = 0; i < NAME_SIZE; ++i) {
    if (name[i] == ' ') {
      break;
    }
  }
  header_out->name     = str8(name, i);
  header_out->date     = (U32)s64_from_str8(str8(date, DATE_SIZE), 10);
  header_out->user_id  = (U32)s64_from_str8(str8(user_id, USER_ID_SIZE), 10);
  header_out->group_id = (U32)s64_from_str8(str8(group_id, GROUP_ID_SIZE), 10);
  header_out->mode     = str8(mode, MODE_SIZE);
  for (i = 0; i < SIZE_SIZE; ++i) {
    if (size[i] == ' ') {
      break;
    }
  }
  header_out->size           = (U32)s64_from_str8(str8(size, i), 10);
  header_out->is_end_correct = (end[0] == '`' && end[1] == '\n');
  
  U64 result = (read_offset - offset);
  return result;
  
#undef NAME_SIZE
#undef DATE_SIZE
#undef USER_ID_SIZE
#undef GROUP_ID_SIZE
#undef MODE_SIZE
#undef SIZE_SIZE 
#undef TOTAL_SIZE
}

internal COFF_ArchiveMember
coff_read_archive_member(String8 data, U64 offset)
{
  COFF_ArchiveMember member; MemoryZeroStruct(&member);
  coff_archive_member_iter_next(data, &offset, &member);
  return member;
}

internal COFF_ArchiveMember
coff_archive_member_from_data(String8 data)
{
  return coff_read_archive_member(data, 0);
}

internal U64
coff_read_archive_import(String8 data, U64 offset, COFF_ImportHeader *header_out)
{
  U64 cursor = offset;
  
  cursor += str8_deserial_read_struct(data, cursor, &header_out->sig1);
  cursor += str8_deserial_read_struct(data, cursor, &header_out->sig2);
  cursor += str8_deserial_read_struct(data, cursor, &header_out->version);
  cursor += str8_deserial_read_struct(data, cursor, &header_out->machine);
  cursor += str8_deserial_read_struct(data, cursor, &header_out->time_stamp);
  cursor += str8_deserial_read_struct(data, cursor, &header_out->data_size);
  cursor += str8_deserial_read_struct(data, cursor, &header_out->hint);
  
  U16 flags = 0;
  cursor += str8_deserial_read_struct(data, cursor, &flags);
  header_out->type = COFF_IMPORT_HEADER_GET_TYPE(flags);
  header_out->name_type = COFF_IMPORT_HEADER_GET_NAME_TYPE(flags);
  
  header_out->func_name = str8(0,0);
  cursor += str8_deserial_read_cstr(data, cursor, &header_out->func_name);
  
  header_out->dll_name = str8(0,0);
  cursor += str8_deserial_read_cstr(data, cursor, &header_out->dll_name);
  
  Assert(header_out->func_name.size + header_out->dll_name.size + /* nulls */ 2 == header_out->data_size);
  
  U64 read_size = cursor - offset;
  return read_size;
}

internal COFF_ImportHeader
coff_archive_import_from_data(String8 data)
{
  COFF_ImportHeader header; MemoryZeroStruct(&header);
  coff_read_archive_import(data, 0, &header);
  return header;
}

internal String8
coff_read_archive_long_name(String8 long_names, String8 name)
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
coff_archive_member_iter_init(String8 data)
{
  U64 cursor = data.size;
  if (coff_is_archive(data)) {
    cursor = sizeof(g_coff_archive_sig);
  }
  return cursor;
}

internal B32
coff_archive_member_iter_next(String8 data, U64 *offset, COFF_ArchiveMember *member_out)
{
  B32 is_parsed;
  
  COFF_ArchiveMemberHeader header;
  U64 header_read_size = coff_read_archive_member_header(data, *offset, &header);
  
  if (header_read_size && header.is_end_correct) {
    Rng1U64 data_range = rng_1u64(*offset + header_read_size, *offset + header_read_size + header.size);
    
    member_out->header     = header;
    member_out->offset     = *offset;
    member_out->data       = str8_substr(data, data_range);
    
    *offset += header_read_size;
    *offset += member_out->header.size;
    *offset = AlignPow2(*offset, COFF_ARCHIVE_ALIGN);
    
    is_parsed = 1;
  } else {
    MemoryZeroStruct(&member_out->header);
    member_out->offset     = max_U64;
    member_out->data       = str8(0,0);
    
    is_parsed = 0;
  }
  
  return is_parsed;
}

internal B32
coff_get_first_archive_member(COFF_ArchiveMember *member, COFF_ArchiveFirstMember *first_out)
{
  B32 is_header = str8_match(member->header.name, str8_lit("/"), 0);
  if (is_header) {
    U64 cursor = 0;
    
    U32 symbol_count = 0;
    cursor += str8_deserial_read_struct(member->data, cursor, &symbol_count);
    
#if ARCH_LITTLE_ENDIAN
    symbol_count = bswap_u32(symbol_count);
#endif
    
    Rng1U64 member_offsets_range = rng_1u64(cursor, cursor + symbol_count * sizeof(U32));
    cursor += dim_1u64(member_offsets_range);
    
    Rng1U64 string_table_range = rng_1u64(cursor, member->data.size);
    cursor += dim_1u64(string_table_range);
    
    first_out->symbol_count         = symbol_count;
    first_out->member_offsets       = str8_substr(member->data, member_offsets_range);
    first_out->string_table         = str8_substr(member->data, string_table_range);
  }
  return is_header;
}

internal B32
coff_get_second_archive_member(COFF_ArchiveMember *member, COFF_ArchiveSecondMember *second_out)
{
  B32 is_header = str8_match(member->header.name, str8_lit("/"), 0);
  if (is_header) {
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
    
    second_out->member_count         = member_count;
    second_out->symbol_count         = symbol_count;
    second_out->member_offsets       = str8_substr(member->data, member_offsets_range);
    second_out->symbol_indices       = str8_substr(member->data, symbol_indices_range);
    second_out->string_table         = str8_substr(member->data, string_table_range);
  }
  return is_header;
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
  COFF_ArchiveMember first_header;      MemoryZeroStruct(&first_header);
  COFF_ArchiveMember second_header;     MemoryZeroStruct(&second_header);
  COFF_ArchiveMember long_names_member; MemoryZeroStruct(&long_names_member);
  
  if (member_list.count) {
    if (str8_match(member_list.first->data.header.name, str8_lit("/"), 0)) {
      first_header = member_list.first->data;
      SLLQueuePop(member_list.first, member_list.last);
      member_list.count -= 1;
      
      if (member_list.count && str8_match(member_list.first->data.header.name, str8_lit("/"), 0)) {
        second_header = member_list.first->data;
        SLLQueuePop(member_list.first, member_list.last);
        member_list.count -= 1;
      }
      
      if (member_list.count && str8_match(member_list.first->data.header.name, str8_lit("//"), 0)) {
        long_names_member = member_list.first->data;
        SLLQueuePop(member_list.first, member_list.last);
        member_list.count -= 1;
      }
    }
  }
  
  COFF_ArchiveFirstMember first_member; MemoryZeroStruct(&first_member);
  coff_get_first_archive_member(&first_header, &first_member);
  
  COFF_ArchiveSecondMember second_member; MemoryZeroStruct(&second_member);
  coff_get_second_archive_member(&second_header, &second_member);
  
  COFF_ArchiveParse parse; MemoryZeroStruct(&parse);
  parse.first_member  = first_member;
  parse.second_member = second_member;
  parse.long_names    = long_names_member.data;
  
  return parse;
}

internal COFF_ArchiveParse
coff_archive_from_data(Arena *arena, String8 data)
{
  COFF_ArchiveMemberList list; MemoryZeroStruct(&list);
  COFF_ArchiveMemberNode node_arr[3]; MemoryZeroStruct(&node_arr[0]);
  U64 cursor = coff_archive_member_iter_init(data);
  for (U64 i = 0; i < ArrayCount(node_arr); i += 1) {
    COFF_ArchiveMemberNode *node = &node_arr[i];
    if (!coff_archive_member_iter_next(data, &cursor, &node->data)) {
      break;
    }
    coff_archive_member_list_push_node(&list, node);
  }
  return coff_archive_parse_from_member_list(list);
}

internal U64
coff_thin_archive_member_iter_init(String8 data)
{
  U64 cursor = data.size;
  if (coff_is_thin_archive(data)) {
    cursor = sizeof(g_coff_thin_archive_sig);
  }
  return cursor;
}

internal B32
coff_thin_archive_member_iter_next(String8 data, U64 *offset, COFF_ArchiveMember *member_out)
{
  B32 is_parsed = 0;
  
  U64 header_size = coff_read_archive_member_header(data, *offset, &member_out->header);
  if (header_size) {
    member_out->offset = *offset;
    *offset += header_size;
    if (str8_match(member_out->header.name, str8_lit("/"), 0) || str8_match(member_out->header.name, str8_lit("//"), 0)) {
      Rng1U64 data_range = rng_1u64(*offset, *offset + member_out->header.size);
      member_out->data = str8_substr(data, data_range);
      *offset += member_out->header.size;
    } else {
      // size field in non-header members means size of stand-alone obj
      member_out->data = str8(0,0);
    }
    *offset = AlignPow2(*offset, COFF_ARCHIVE_ALIGN);
    is_parsed = 1;
  }
  
  return is_parsed;
}

internal COFF_ArchiveParse
coff_thin_archive_from_data(Arena *arena, String8 data)
{
  COFF_ArchiveMemberList list; MemoryZeroStruct(&list);
  COFF_ArchiveMemberNode node_arr[3]; MemoryZeroStruct(&node_arr[0]);
  U64 cursor = coff_thin_archive_member_iter_init(data);
  for (U64 i = 0; i < ArrayCount(node_arr); i += 1) {
    COFF_ArchiveMemberNode *node = &node_arr[i];
    if (!coff_thin_archive_member_iter_next(data, &cursor, &node->data)) {
      break;
    }
    coff_archive_member_list_push_node(&list, node);
  }
  return coff_archive_parse_from_member_list(list);
}

internal COFF_ArchiveType
coff_archive_type_from_data(String8 data)
{
  if (coff_is_archive(data)) {
    return COFF_Archive_Regular;
  } else if (coff_is_thin_archive(data)) {
    return COFF_Archive_Thin;
  }
  return COFF_Archive_Null;
}

internal COFF_ArchiveParse
coff_archive_parse_from_data(Arena *arena, String8 data)
{
  COFF_ArchiveType type = coff_archive_type_from_data(data);
  switch (type) {
    case COFF_Archive_Null:    break;
    case COFF_Archive_Regular: return coff_archive_from_data(arena, data);
    case COFF_Archive_Thin:    return coff_thin_archive_from_data(arena, data);
  }
  COFF_ArchiveParse null_parse; MemoryZeroStruct(&null_parse);
  return null_parse;
}

////////////////////////////////

read_only struct
{
  String8          string;
  COFF_MachineType machine;
} g_coff_machine_map[] = {
  { str8_lit_comp(""),          COFF_MachineType_UNKNOWN   },
  { str8_lit_comp("X86"),       COFF_MachineType_X86       },
  { str8_lit_comp("AMD64"),     COFF_MachineType_X64       },
  { str8_lit_comp("X64"),       COFF_MachineType_X64       },
  { str8_lit_comp("AM33"),      COFF_MachineType_AM33      },
  { str8_lit_comp("ARM"),       COFF_MachineType_ARM       },
  { str8_lit_comp("ARM64"),     COFF_MachineType_ARM64     },
  { str8_lit_comp("ARMNT"),     COFF_MachineType_ARMNT     },
  { str8_lit_comp("EBC"),       COFF_MachineType_EBC       },
  { str8_lit_comp("IA64"),      COFF_MachineType_IA64      },
  { str8_lit_comp("M32R"),      COFF_MachineType_M32R      },
  { str8_lit_comp("MIPS16"),    COFF_MachineType_MIPS16    },
  { str8_lit_comp("MIPSFPU"),   COFF_MachineType_MIPSFPU   },
  { str8_lit_comp("MIPSFPU16"), COFF_MachineType_MIPSFPU16 },
  { str8_lit_comp("POWERPC"),   COFF_MachineType_POWERPC   },
  { str8_lit_comp("POWERPCFP"), COFF_MachineType_POWERPCFP },
  { str8_lit_comp("R4000"),     COFF_MachineType_R4000     },
  { str8_lit_comp("RISCV32"),   COFF_MachineType_RISCV32   },
  { str8_lit_comp("RISCV64"),   COFF_MachineType_RISCV64   },
  { str8_lit_comp("SH3"),       COFF_MachineType_SH3       },
  { str8_lit_comp("SH3DSP"),    COFF_MachineType_SH3DSP    },
  { str8_lit_comp("SH4"),       COFF_MachineType_SH4       },
  { str8_lit_comp("SH5"),       COFF_MachineType_SH5       },
  { str8_lit_comp("THUMB"),     COFF_MachineType_THUMB     },
  { str8_lit_comp("WCEMIPSV2"), COFF_MachineType_WCEMIPSV2 },
};

read_only static struct {
  char *                name;
  COFF_ImportHeaderType type;
} g_coff_import_header_type_map[] = {
  { "CODE",  COFF_ImportHeaderType_CODE  },
  { "DATA",  COFF_ImportHeaderType_DATA  },
  { "CONST", COFF_ImportHeaderType_CONST },
};

internal String8
coff_string_from_comdat_select_type(COFF_ComdatSelectType select)
{
  String8 result = str8(0,0);
  switch (select) {
  case COFF_ComdatSelectType_NULL:         result = str8_lit("NULL");         break;
  case COFF_ComdatSelectType_NODUPLICATES: result = str8_lit("NODUPLICATES"); break;
  case COFF_ComdatSelectType_ANY:          result = str8_lit("ANY");          break;
  case COFF_ComdatSelectType_SAME_SIZE:    result = str8_lit("SAME_SIZE");    break;
  case COFF_ComdatSelectType_EXACT_MATCH:  result = str8_lit("EXACT_MATCH");  break;
  case COFF_ComdatSelectType_ASSOCIATIVE:  result = str8_lit("ASSOCIATIVE");  break;
  case COFF_ComdatSelectType_LARGEST:      result = str8_lit("LARGEST");      break;
  }
  return result;
}

internal String8
coff_string_from_machine_type(COFF_MachineType machine)
{
  for (U64 i = 0; i < ArrayCount(g_coff_machine_map); ++i) {
    if (g_coff_machine_map[i].machine == machine) {
      return g_coff_machine_map[i].string;
    }
  }
  return str8_zero();
}

internal String8
coff_string_from_section_flags(Arena *arena, COFF_SectionFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  
  if (flags & COFF_SectionFlag_TYPE_NO_PAD) {
    str8_list_pushf(scratch.arena, &list, "TYPE_NO_PAD");
  }
  if (flags & COFF_SectionFlag_CNT_CODE) {
    str8_list_pushf(scratch.arena, &list, "CNT_CODE");
  }
  if (flags & COFF_SectionFlag_CNT_INITIALIZED_DATA) {
    str8_list_pushf(scratch.arena, &list, "CNT_INITIALIZED_DATA");
  }
  if (flags & COFF_SectionFlag_CNT_UNINITIALIZED_DATA) {
    str8_list_pushf(scratch.arena, &list, "CNT_UNINITIALIZED_DATA");
  }
  if (flags & COFF_SectionFlag_LNK_OTHER) {
    str8_list_pushf(scratch.arena, &list, "LNK_OTHER");
  }
  if (flags & COFF_SectionFlag_LNK_INFO) {
    str8_list_pushf(scratch.arena, &list, "LNK_INFO");
  }
  if (flags & COFF_SectionFlag_LNK_COMDAT) {
    str8_list_pushf(scratch.arena, &list, "LNK_COMDAT");
  }
  if (flags & COFF_SectionFlag_GPREL) {
    str8_list_pushf(scratch.arena, &list, "GPREL");
  }
  if (flags & COFF_SectionFlag_MEM_16BIT) {
    str8_list_pushf(scratch.arena, &list, "MEM_16BIT");
  }
  if (flags & COFF_SectionFlag_MEM_LOCKED) {
    str8_list_pushf(scratch.arena, &list, "MEM_LOCKED");
  }
  if (flags & COFF_SectionFlag_MEM_PRELOAD) {
    str8_list_pushf(scratch.arena, &list, "MEM_PRELOAD");
  }
  if (flags & COFF_SectionFlag_LNK_NRELOC_OVFL) {
    str8_list_pushf(scratch.arena, &list, "LNK_NRELOC_OVFL");
  }
  if (flags & COFF_SectionFlag_MEM_DISCARDABLE) {
    str8_list_pushf(scratch.arena, &list, "MEM_DISCARDABLE");
  }
  if (flags & COFF_SectionFlag_MEM_NOT_CACHED) {
    str8_list_pushf(scratch.arena, &list, "MEM_NOT_CACHED");
  }
  if (flags & COFF_SectionFlag_MEM_NOT_PAGED) {
    str8_list_pushf(scratch.arena, &list, "MEM_NOT_PAGED");
  }
  if (flags & COFF_SectionFlag_MEM_SHARED) {
    str8_list_pushf(scratch.arena, &list, "MEM_SHARED");
  }
  if (flags & COFF_SectionFlag_MEM_EXECUTE) {
    str8_list_pushf(scratch.arena, &list, "MEM_EXECUTE");
  }
  if (flags & COFF_SectionFlag_MEM_READ) {
    str8_list_pushf(scratch.arena, &list, "MEM_READ");
  }
  if (flags & COFF_SectionFlag_MEM_WRITE) {
    str8_list_pushf(scratch.arena, &list, "MEM_WRITE");
  }
  
  U64 align = COFF_SectionFlags_Extract_ALIGN(flags);
  if (align) {
    str8_list_pushf(scratch.arena, &list, "ALIGN=%u", align);
  }
  
  StringJoin join = {0};
  join.sep = str8_lit(", ");
  String8 result = str8_list_join(arena, &list, &join);
  
  scratch_end(scratch);
  return result;
}

internal String8
coff_string_from_import_header_type(COFF_ImportHeaderType type)
{
  for (U64 i = 0; i < ArrayCount(g_coff_import_header_type_map); ++i) {
    if (g_coff_import_header_type_map[i].type == type) {
      return str8_cstring(g_coff_import_header_type_map[i].name);
    }
  }
  return str8(0,0);
}

internal String8
coff_string_from_sym_dtype(COFF_SymDType x)
{
  switch (x) {
    case COFF_SymDType_NULL:  break;
    case COFF_SymDType_PTR :  return str8_lit("PTR");
    case COFF_SymDType_FUNC:  return str8_lit("FUNC");
    case COFF_SymDType_ARRAY: return str8_lit("ARRAY");
  }
  return str8_zero();
}

internal String8
coff_string_from_sym_type(COFF_SymType x)
{
  switch (x) {
    case COFF_SymType_NULL:   break;
    case COFF_SymType_VOID:   return str8_lit("VOID");
    case COFF_SymType_CHAR:   return str8_lit("CHAR");
    case COFF_SymType_SHORT:  return str8_lit("SHORT");
    case COFF_SymType_INT:    return str8_lit("INT");
    case COFF_SymType_LONG:   return str8_lit("LONG");
    case COFF_SymType_FLOAT:  return str8_lit("FLOAT");
    case COFF_SymType_DOUBLE: return str8_lit("DOUBLE");
    case COFF_SymType_STRUCT: return str8_lit("STRUCT");
    case COFF_SymType_UNION:  return str8_lit("UNION");
    case COFF_SymType_ENUM:   return str8_lit("ENUM");
    case COFF_SymType_MOE:    return str8_lit("MOE");
    case COFF_SymType_BYTE:   return str8_lit("BYTE");
    case COFF_SymType_WORD:   return str8_lit("WORD");
    case COFF_SymType_UINT:   return str8_lit("UINT");
    case COFF_SymType_DWORD:  return str8_lit("DWORD");
  }
  return str8_zero();
}

internal String8
coff_string_from_sym_storage_class(COFF_SymStorageClass x)
{
  switch (x) {
    case COFF_SymStorageClass_NULL:             break;
    case COFF_SymStorageClass_END_OF_FUNCTION:  return str8_lit("EOF");
    case COFF_SymStorageClass_AUTOMATIC:        return str8_lit("AUTOMATIC");
    case COFF_SymStorageClass_EXTERNAL:         return str8_lit("EXTERNAL");
    case COFF_SymStorageClass_STATIC:           return str8_lit("STATIC");
    case COFF_SymStorageClass_REGISTER:         return str8_lit("REGISTER");
    case COFF_SymStorageClass_EXTERNAL_DEF:     return str8_lit("DEF");
    case COFF_SymStorageClass_LABEL:            return str8_lit("LABEL");
    case COFF_SymStorageClass_UNDEFINED_LABEL:  return str8_lit("LABEL");
    case COFF_SymStorageClass_MEMBER_OF_STRUCT: return str8_lit("STRUCT");
    case COFF_SymStorageClass_ARGUMENT:         return str8_lit("ARGUMENT");
    case COFF_SymStorageClass_STRUCT_TAG:       return str8_lit("TAG");
    case COFF_SymStorageClass_MEMBER_OF_UNION:  return str8_lit("UNION");
    case COFF_SymStorageClass_UNION_TAG:        return str8_lit("TAG");
    case COFF_SymStorageClass_TYPE_DEFINITION:  return str8_lit("DEFINITION");
    case COFF_SymStorageClass_UNDEFINED_STATIC: return str8_lit("STATIC");
    case COFF_SymStorageClass_ENUM_TAG:         return str8_lit("TAG");
    case COFF_SymStorageClass_MEMBER_OF_ENUM:   return str8_lit("ENUM");
    case COFF_SymStorageClass_REGISTER_PARAM:   return str8_lit("PARAM");
    case COFF_SymStorageClass_BIT_FIELD:        return str8_lit("FIELD");
    case COFF_SymStorageClass_BLOCK:            return str8_lit("BLOCK");
    case COFF_SymStorageClass_FUNCTION:         return str8_lit("FUNCTION");
    case COFF_SymStorageClass_END_OF_STRUCT:    return str8_lit("STRUCT");
    case COFF_SymStorageClass_FILE:             return str8_lit("FILE");
    case COFF_SymStorageClass_SECTION:          return str8_lit("SECTION");
    case COFF_SymStorageClass_WEAK_EXTERNAL:    return str8_lit("EXTERNAL");
    case COFF_SymStorageClass_CLR_TOKEN:        return str8_lit("TOKEN");
  }
  return str8_zero();
}

internal String8
coff_string_from_weak_ext_type(COFF_WeakExtType x)
{
  switch (x) {
    case COFF_WeakExtType_NOLIBRARY:      return str8_lit("NOLIBRARY");
    case COFF_WeakExtType_SEARCH_LIBRARY: return str8_lit("SEARCH_LIBRARY");
    case COFF_WeakExtType_SEARCH_ALIAS:   return str8_lit("SEARCH_ALIAS");
  }
  return str8_zero();
}

internal String8
coff_string_from_selection(COFF_ComdatSelectType x)
{
  switch (x) {
    case COFF_ComdatSelectType_NULL:         break;
    case COFF_ComdatSelectType_NODUPLICATES: return str8_lit("NODUPLICATES");
    case COFF_ComdatSelectType_ANY:          return str8_lit("ANY");
    case COFF_ComdatSelectType_SAME_SIZE:    return str8_lit("SIZE");
    case COFF_ComdatSelectType_EXACT_MATCH:  return str8_lit("MATCH");
    case COFF_ComdatSelectType_ASSOCIATIVE:  return str8_lit("ASSOCIATIVE");
    case COFF_ComdatSelectType_LARGEST:      return str8_lit("LARGEST");
  }
  return str8_zero();
}

internal String8
coff_string_from_reloc_x86(COFF_RelocTypeX86 x)
{
  switch (x) {
    case COFF_RelocTypeX86_ABS:      return str8_lit("ABS");
    case COFF_RelocTypeX86_DIR16:    return str8_lit("DIR16");
    case COFF_RelocTypeX86_REL16:    return str8_lit("REL16");
    case COFF_RelocTypeX86_UNKNOWN0: return str8_lit("UNKNOWN0");
    case COFF_RelocTypeX86_UNKNOWN2: return str8_lit("UNKNOWN2");
    case COFF_RelocTypeX86_UNKNOWN3: return str8_lit("UNKNOWN3");
    case COFF_RelocTypeX86_DIR32:    return str8_lit("DIR32");
    case COFF_RelocTypeX86_DIR32NB:  return str8_lit("DIR32NB");
    case COFF_RelocTypeX86_SEG12:    return str8_lit("SEG12");
    case COFF_RelocTypeX86_SECTION:  return str8_lit("SECTION");
    case COFF_RelocTypeX86_SECREL:   return str8_lit("SECREL");
    case COFF_RelocTypeX86_TOKEN:    return str8_lit("TOKEN");
    case COFF_RelocTypeX86_SECREL7:  return str8_lit("SECREL7");
    case COFF_RelocTypeX86_UNKNOWN4: return str8_lit("UNKNOWN4");
    case COFF_RelocTypeX86_UNKNOWN5: return str8_lit("UNKNOWN5");
    case COFF_RelocTypeX86_UNKNOWN6: return str8_lit("UNKNOWN6");
    case COFF_RelocTypeX86_UNKNOWN7: return str8_lit("UNKNOWN7");
    case COFF_RelocTypeX86_UNKNOWN8: return str8_lit("UNKNOWN8");
    case COFF_RelocTypeX86_UNKNOWN9: return str8_lit("UNKNOWN9");
    case COFF_RelocTypeX86_REL32:    return str8_lit("REL32");
  }
  return str8_zero();
}

internal String8
coff_string_from_reloc_x64(COFF_RelocTypeX64 x)
{
  switch (x) {
    case COFF_RelocTypeX64_ABS:      return str8_lit("ABS");
    case COFF_RelocTypeX64_ADDR64:   return str8_lit("ADDR64");
    case COFF_RelocTypeX64_ADDR32:   return str8_lit("ADDR32");
    case COFF_RelocTypeX64_ADDR32NB: return str8_lit("ADDR32NB");
    case COFF_RelocTypeX64_REL32:    return str8_lit("REL32");
    case COFF_RelocTypeX64_REL32_1:  return str8_lit("REL32_1");
    case COFF_RelocTypeX64_REL32_2:  return str8_lit("REL32_2");
    case COFF_RelocTypeX64_REL32_3:  return str8_lit("REL32_3");
    case COFF_RelocTypeX64_REL32_4:  return str8_lit("REL32_4");
    case COFF_RelocTypeX64_REL32_5:  return str8_lit("REL32_5");
    case COFF_RelocTypeX64_SECTION:  return str8_lit("SECTION");
    case COFF_RelocTypeX64_SECREL:   return str8_lit("SECREL");
    case COFF_RelocTypeX64_SECREL7:  return str8_lit("SECREL7");
    case COFF_RelocTypeX64_TOKEN:    return str8_lit("TOKEN");
    case COFF_RelocTypeX64_SREL32:   return str8_lit("SREL32");
    case COFF_RelocTypeX64_PAIR:     return str8_lit("PAIR");
    case COFF_RelocTypeX64_SSPAN32:  return str8_lit("SSPAN32");
  }
  return str8_zero();
}

internal String8
coff_string_from_reloc_arm(COFF_RelocTypeARM x)
{
  switch (x) {
    case COFF_RelocTypeARM_ABS:            return str8_lit("ABS");
    case COFF_RelocTypeARM_ADDR32:         return str8_lit("ADDR32");
    case COFF_RelocTypeARM_ADDR32NB:       return str8_lit("ADDR32NB");
    case COFF_RelocTypeARM_BRANCH24:       return str8_lit("BRANCH24");
    case COFF_RelocTypeARM_BRANCH11:       return str8_lit("BRANCH11");
    case COFF_RelocTypeARM_UNKNOWN1:       return str8_lit("UNKNOWN1");
    case COFF_RelocTypeARM_UNKNOWN2:       return str8_lit("UNKNOWN2");
    case COFF_RelocTypeARM_UNKNOWN3:       return str8_lit("UNKNOWN3");
    case COFF_RelocTypeARM_UNKNOWN4:       return str8_lit("UNKNOWN4");
    case COFF_RelocTypeARM_UNKNOWN5:       return str8_lit("UNKNOWN5");
    case COFF_RelocTypeARM_REL32:          return str8_lit("REL32");
    case COFF_RelocTypeARM_SECTION:        return str8_lit("SECTION");
    case COFF_RelocTypeARM_SECREL:         return str8_lit("SECREL");
    case COFF_RelocTypeARM_MOV32:          return str8_lit("MOV32");
    case COFF_RelocTypeARM_THUMB_MOV32:    return str8_lit("THUMB_MOV32");
    case COFF_RelocTypeARM_THUMB_BRANCH20: return str8_lit("THUMB_BRANCH20");
    case COFF_RelocTypeARM_UNUSED:         return str8_lit("UNUSED");
    case COFF_RelocTypeARM_THUMB_BRANCH24: return str8_lit("THUMB_BRANCH24");
    case COFF_RelocTypeARM_THUMB_BLX23:    return str8_lit("THUMB_BLX23");
    case COFF_RelocTypeARM_PAIR:           return str8_lit("PAIR");
  }
  return str8_zero();
}

internal String8
coff_string_from_reloc_arm64(COFF_RelocTypeARM64 x)
{
  switch (x) {
    case COFF_RelocTypeARM64_ABS:            return str8_lit("ABS");
    case COFF_RelocTypeARM64_ADDR32:         return str8_lit("ADDR32");
    case COFF_RelocTypeARM64_ADDR32NB:       return str8_lit("ADDR32NB");
    case COFF_RelocTypeARM64_BRANCH26:       return str8_lit("BRANCH26");
    case COFF_RelocTypeARM64_PAGEBASE_REL21: return str8_lit("PAGEBASE_REL21");
    case COFF_RelocTypeARM64_REL21:          return str8_lit("REL21");
    case COFF_RelocTypeARM64_PAGEOFFSET_12A: return str8_lit("PAGEOFFSET_12A");
    case COFF_RelocTypeARM64_SECREL:         return str8_lit("SECREL");
    case COFF_RelocTypeARM64_SECREL_LOW12A:  return str8_lit("SECREL_LOW12A");
    case COFF_RelocTypeARM64_SECREL_HIGH12A: return str8_lit("SECREL_HIGH12A");
    case COFF_RelocTypeARM64_SECREL_LOW12L:  return str8_lit("SECREL_LOW12L");
    case COFF_RelocTypeARM64_TOKEN:          return str8_lit("TOKEN");
    case COFF_RelocTypeARM64_SECTION:        return str8_lit("SECTION");
    case COFF_RelocTypeARM64_ADDR64:         return str8_lit("ADDR64");
    case COFF_RelocTypeARM64_BRANCH19:       return str8_lit("BRANCH19");
    case COFF_RelocTypeARM64_BRANCH14:       return str8_lit("BRANCH14");
    case COFF_RelocTypeARM64_REL32:          return str8_lit("REL32");
  }
  return str8_zero();
}

internal String8
coff_string_from_reloc(COFF_MachineType machine, COFF_RelocType x)
{
  switch (machine) {
    case COFF_MachineType_X86:   return coff_string_from_reloc_x86(x);
    case COFF_MachineType_X64:   return coff_string_from_reloc_x64(x);
    case COFF_MachineType_ARM:   return coff_string_from_reloc_arm(x);
    case COFF_MachineType_ARM64: return coff_string_from_reloc_arm64(x);
  }
  return str8_zero();
}

internal COFF_MachineType
coff_machine_from_string(String8 string)
{
  for (U64 i = 0; i < ArrayCount(g_coff_machine_map); ++i) {
    if (str8_match(g_coff_machine_map[i].string, string, StringMatchFlag_CaseInsensitive)) {
      return g_coff_machine_map[i].machine;
    }
  }
  return COFF_MachineType_UNKNOWN;
}

internal COFF_ImportHeaderType
coff_import_header_type_from_string(String8 name)
{
  for (U64 i = 0; i < ArrayCount(g_coff_import_header_type_map); ++i) {
    if (str8_match(str8_cstring(g_coff_import_header_type_map[i].name), name, StringMatchFlag_CaseInsensitive)) {
      return g_coff_import_header_type_map[i].type;
    }
  }
  return COFF_ImportHeaderType_COUNT;
}

