// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
coff_string_from_time_stamp(Arena *arena, COFF_TimeStamp time_stamp)
{
  String8 result;
  if (time_stamp == 0) {
    result = str8_lit("0");
  } else if (time_stamp >= max_U32) {
    result = str8_lit("-1");
  } else {
    DateTime dt = date_time_from_unix_time(time_stamp);
    result = push_date_time_string(arena, &dt);
  }
  return result;
}

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
coff_string_from_flags(Arena *arena, COFF_Flags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};

  if (flags & COFF_Flag_RELOC_STRIPPED) {
    str8_list_pushf(scratch.arena, &list, "Relocs Stripped");
  }
  if (flags & COFF_Flag_EXECUTABLE_IMAGE) {
    str8_list_pushf(scratch.arena, &list, "Executable");
  }
  if (flags & COFF_Flag_LINE_NUMS_STRIPPED) {
    str8_list_pushf(scratch.arena, &list, "Line Numbers Stripped");
  }
  if (flags & COFF_Flag_SYM_STRIPPED) {
    str8_list_pushf(scratch.arena, &list, "Symbols Stripped");
  }
  if (flags & COFF_Flag_LARGE_ADDRESS_AWARE) {
    str8_list_pushf(scratch.arena, &list, "Large Address Aware");
  }
  if (flags & COFF_Flag_32BIT_MACHINE) {
    str8_list_pushf(scratch.arena, &list, "32-Bit Machine");
  }
  if (flags & COFF_Flag_DEBUG_STRIPPED) {
    str8_list_pushf(scratch.arena, &list, "Debug Stripped");
  }
  if (flags & COFF_Flag_REMOVABLE_RUN_FROM_SWAP) {
    str8_list_pushf(scratch.arena, &list, "Removeable Run From Swap");
  }
  if (flags & COFF_Flag_NET_RUN_FROM_SWAP) {
    str8_list_pushf(scratch.arena, &list, "Net Run From Swap");
  }
  if (flags & COFF_Flag_SYSTEM) {
    str8_list_pushf(scratch.arena, &list, "System");
  }
  if (flags & COFF_Flag_DLL) {
    str8_list_pushf(scratch.arena, &list, "DLL");
  }
  if (flags & COFF_Flag_UP_SYSTEM_ONLY) {
    str8_list_pushf(scratch.arena, &list, "Up System Only");
  }

  String8 result = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});

  scratch_end(scratch);
  return result;
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
  
  U64 align = coff_align_size_from_section_flags(flags);
  if (align) {
    str8_list_pushf(scratch.arena, &list, "Align=%u", align);
  }

  if (!list.node_count) {
    str8_list_pushf(scratch.arena, &list, "None");
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
    case COFF_SymDType_NULL:  return str8_lit("NULL");
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
    case COFF_SymType_NULL:   return str8_lit("NULL");
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



