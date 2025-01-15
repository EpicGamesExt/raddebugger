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
coff_name_from_section_header(COFF_SectionHeader *header, String8 coff_data, U64 string_table_base)
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

internal U64
coff_parse_archive_member_header(String8 data, U64 offset, B32 is_regular_archive, COFF_ArchiveMemberHeader *header_out)
{
  static const U64     name_size         = 16;
  static const U64     date_size         = 12;
  static const U64     user_id_size      = 6;
  static const U64     group_id_size     = 6;
  static const U64     mode_size         = 8;
  static const U64     size_size         = 10;
  static const String8 end_magic         = str8_lit_comp("`\n");
  U64 total_header_size = name_size + date_size + user_id_size + group_id_size + mode_size + size_size + end_magic.size;
  
  U64 cursor = offset;
  if (cursor + total_header_size <= data.size) {
    String8 name     = str8_zero();
    String8 date     = str8_zero();
    String8 user_id  = str8_zero();
    String8 group_id = str8_zero();
    String8 mode     = str8_zero();
    String8 size     = str8_zero();
    String8 end      = str8_zero();

    cursor += str8_deserial_read_block(data, cursor, name_size,     &name    );
    cursor += str8_deserial_read_block(data, cursor, date_size,     &date    );
    cursor += str8_deserial_read_block(data, cursor, user_id_size,  &user_id );
    cursor += str8_deserial_read_block(data, cursor, group_id_size, &group_id);
    cursor += str8_deserial_read_block(data, cursor, mode_size,     &mode    );
    cursor += str8_deserial_read_block(data, cursor, size_size,     &size    );
    cursor += str8_deserial_read_block(data, cursor, 2,             &end     );

    name     = str8_skip_chop_whitespace(name);
    date     = str8_skip_chop_whitespace(date);
    user_id  = str8_skip_chop_whitespace(user_id);
    group_id = str8_skip_chop_whitespace(group_id);
    mode     = str8_skip_chop_whitespace(mode);
    size     = str8_skip_chop_whitespace(size);

    header_out->name           = name;
    header_out->time_stamp     = u32_from_str8(date, 10);
    header_out->user_id        = u32_from_str8(user_id, 10);
    header_out->group_id       = u32_from_str8(group_id, 10);
    header_out->mode           = mode;
    header_out->is_end_correct = str8_match(end, end_magic, 0);
    header_out->data_range     = rng_1u64(cursor, cursor + u32_from_str8(size, 10));

    if (is_regular_archive) {
      cursor = header_out->data_range.max;
    }
  }

  U64 result = AlignPow2((cursor - offset), COFF_ARCHIVE_ALIGN);
  return result;
}

internal B32
coff_parse_archive_member_data(String8 data, U64 cursor, COFF_ArchiveMember *member_out)
{
  B32 is_parsed = 0;

  COFF_ArchiveMemberHeader header; header.is_end_correct = 0;
  coff_parse_archive_member_header(data, cursor, 1, &header);
  
  if (header.is_end_correct) {
    member_out->header = header;
    member_out->offset = cursor;
    member_out->data   = str8_substr(data, header.data_range);
    
    cursor = AlignPow2(header.data_range.max, COFF_ARCHIVE_ALIGN);

    is_parsed = 1;
  } else {
    MemoryZeroStruct(&member_out->header);
    member_out->offset = max_U64;
    member_out->data   = str8_zero();
  }
  
  return is_parsed;
}

internal COFF_ArchiveFirstMember
coff_parse_first_archive_member(COFF_ArchiveMember *member)
{
  Assert(str8_match(member->header.name, str8_lit("/"), 0));

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

  if (str8_match(member->header.name, str8_lit("/"), 0)) {
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
    
    result.member_count             = member_count;
    result.symbol_count             = symbol_count;
    result.member_offsets           = member_offsets;
    result.member_offset_count      = member_offset_count;
    result.symbol_indices           = symbol_indices;
    result.symbol_index_count       = symbol_index_count;
    result.string_table             = str8_substr(member->data, string_table_range);
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
coff_parse_archive_import(String8 data, U64 offset, COFF_ImportHeader *header_out)
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
  header_out->type      = COFF_IMPORT_HEADER_GET_TYPE(flags);
  header_out->name_type = COFF_IMPORT_HEADER_GET_NAME_TYPE(flags);
  
  header_out->func_name = str8_zero();
  cursor += str8_deserial_read_cstr(data, cursor, &header_out->func_name);
  
  header_out->dll_name = str8_zero();
  cursor += str8_deserial_read_cstr(data, cursor, &header_out->dll_name);
  
  Assert(header_out->func_name.size + header_out->dll_name.size + /* nulls */ 2 == header_out->data_size);
  
  U64 read_size = cursor - offset;
  return read_size;
}

internal COFF_ArchiveMember
coff_archive_member_from_offset(String8 data, U64 offset)
{
  COFF_ArchiveMember member = {0};
  coff_archive_member_iter_next(data, &offset, &member);
  return member;
}

internal COFF_ArchiveMember
coff_archive_member_from_data(String8 data)
{
  return coff_archive_member_from_offset(data, 0);
}

internal COFF_ImportHeader
coff_archive_import_from_data(String8 data)
{
  COFF_ImportHeader header = {0};
  coff_parse_archive_import(data, 0, &header);
  return header;
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
  
  COFF_ArchiveMemberHeader header; header.is_end_correct = 0;
  U64 read_size = coff_parse_archive_member_header(data, *offset, 1, &header);
  
  if (header.is_end_correct) {
    member_out->header     = header;
    member_out->offset     = *offset;
    member_out->data       = str8_substr(data, header.data_range);

    *offset += read_size;
    
    is_parsed = 1;
  } else {
    MemoryZeroStruct(&member_out->header);
    member_out->offset = max_U64;
    member_out->data   = str8(0,0);

    is_parsed = 0;
  }
  
  return is_parsed;
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
  
  member_out->header.is_end_correct = 0;
  U64 header_size = coff_parse_archive_member_header(data, *offset, 0, &member_out->header);

  if (member_out->header.is_end_correct) {
    member_out->offset = *offset;
    if (str8_match(member_out->header.name, str8_lit("/"), 0) || str8_match(member_out->header.name, str8_lit("//"), 0)) {
      member_out->data = str8_substr(data, member_out->header.data_range);
    } else {
      // size field in non-header members means size of stand-alone obj
      member_out->data = str8_zero();
    }
    *offset += header_size;
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
    if (str8_match(ptr->data.header.name, str8_lit("/"), 0)) {
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
    if (str8_match(ptr->data.header.name, str8_lit("/"), 0)) {
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
    if (str8_match(ptr->data.header.name, str8_lit("//"), 0)) {
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
coff_archive_from_data(String8 data)
{
  COFF_ArchiveMemberList list = {0};
  COFF_ArchiveMemberNode node_arr[3] = {0};
  U64 cursor = coff_archive_member_iter_init(data);
  for (U64 i = 0; i < ArrayCount(node_arr); ++i) {
    COFF_ArchiveMemberNode *node = &node_arr[i];
    if (!coff_archive_member_iter_next(data, &cursor, &node->data)) {
      break;
    }
    coff_archive_member_list_push_node(&list, node);
  }
  return coff_archive_parse_from_member_list(list);
}

internal COFF_ArchiveParse
coff_thin_archive_from_data(String8 data)
{
  COFF_ArchiveMemberList list = {0};
  COFF_ArchiveMemberNode node_arr[3] = {0};
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

internal COFF_ArchiveParse
coff_archive_parse_from_data(String8 data)
{
  COFF_ArchiveType type = coff_archive_type_from_data(data);
  switch (type) {
  case COFF_Archive_Null:    break;
  case COFF_Archive_Regular: return coff_archive_from_data(data);
  case COFF_Archive_Thin:    return coff_thin_archive_from_data(data);
  }
  COFF_ArchiveParse null_parse = {0};
  return null_parse;
}

internal Arch
arch_from_coff_machine(COFF_MachineType machine)
{
  Arch result = Arch_Null;
  switch (machine) {
    case COFF_MachineType_UNKNOWN: break;
    case COFF_MachineType_X86:   result = Arch_x86;   break;
    case COFF_MachineType_X64:   result = Arch_x64;   break;
    case COFF_MachineType_ARM:   result = Arch_arm32; break;
    case COFF_MachineType_ARM64: result = Arch_arm64; break;
  }
  return result;
}

