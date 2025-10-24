// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal GNU_LinkMap64
gnu_linkmap64_from_linkmap32(GNU_LinkMap32 linkmap32)
{
  GNU_LinkMap64 linkmap64 = {0};
  linkmap64.addr_vaddr = linkmap32.addr_vaddr;
  linkmap64.name_vaddr = linkmap32.name_vaddr;
  linkmap64.ld_vaddr   = linkmap32.ld_vaddr;
  linkmap64.next_vaddr = linkmap32.next_vaddr;
  linkmap64.prev_vaddr = linkmap32.prev_vaddr;
  return linkmap64;
}

internal U64
gnu_rdebug_info_size_from_arch(Arch arch)
{
  U64 size = 0;
  switch (byte_size_from_arch(arch)) {
  case 0: break;
  case 4: size = sizeof(GNU_RDebugInfo32); break;
  case 8: size = sizeof(GNU_RDebugInfo64); break;
  default: InvalidPath; break;
  }
  return size;
}

internal U64
gnu_r_brk_offset_from_arch(Arch arch)
{
  U64 offset = 0;
  switch (gnu_rdebug_info_size_from_arch(arch)) {
  case 0: offset = 0; break;
  case sizeof(GNU_RDebugInfo32): offset = OffsetOf(GNU_RDebugInfo32, r_brk); break;
  case sizeof(GNU_RDebugInfo64): offset = OffsetOf(GNU_RDebugInfo64, r_brk); break;
  default: InvalidPath; break;
  }
  return offset;
}

internal GNU_RDebugInfo64
gnu_rdebug_info64_from_rdebug_info32(GNU_RDebugInfo32 rdebug32)
{
  GNU_RDebugInfo64 result = {0};
  result.r_version = rdebug32.r_version;
  result.r_map     = rdebug32.r_map;
  result.r_brk     = rdebug32.r_brk;
  result.r_state   = rdebug32.r_state;
  result.r_ldbase  = rdebug32.r_ldbase;
  return result;
}

internal String8
gnu_string_from_abi_tag(GNU_AbiTag abi_tag)
{
  switch (abi_tag) {
  case GNU_AbiTag_Linux:    return str8_lit("Linux");
  case GNU_AbiTag_Hurd:     return str8_lit("Hurd");
  case GNU_AbiTag_Solaris:  return str8_lit("Solaris");
  case GNU_AbiTag_FreeBsd:  return str8_lit("FreeBsd");
  case GNU_AbiTag_NetBsd:   return str8_lit("NetBsd");
  case GNU_AbiTag_Syllable: return str8_lit("Syllable");
  case GNU_AbiTag_Nacl:     return str8_lit("Nacl");
  }
  return str8_zero();
}

internal String8
gnu_string_from_note_type(GNU_NoteType note_type)
{
  switch (note_type) {
  case GNU_NoteType_Abi:           return str8_lit("GNU_Abi");
  case GNU_NoteType_HwCap:         return str8_lit("GNU_HwCap");
  case GNU_NoteType_BuildId:       return str8_lit("GNU_BuildId");
  case GNU_NoteType_GoldVersion:   return str8_lit("GNU_GoldVersion");
  case GNU_NoteType_PropertyType0: return str8_lit("GNU_PropertyType0");
  }
  return str8_zero();
}

internal String8
gnu_string_from_property_x86(GNU_PropertyX86 prop)
{
  switch (prop) {
  case GNU_PropertyX86_Feature1And:         return str8_lit("Feature1And");
  case GNU_PropertyX86_Feature2Used:        return str8_lit("Feature2Used");
  case GNU_PropertyX86_Isa1needed:          return str8_lit("Isa1needed");
  case GNU_PropertyX86_Isa2Needed:          return str8_lit("Isa2Needed");
  case GNU_PropertyX86_Isa1Used:            return str8_lit("Isa1Used");
  case GNU_PropertyX86_Compat_isa_1_used:   return str8_lit("Compat_isa_1_used");
  case GNU_PropertyX86_Compat_isa_1_needed: return str8_lit("Compat_isa_1_needed");
  case GNU_PropertyX86_UInt32AndHi:         return str8_lit("UInt32AndHi");
  case GNU_PropertyX86_UInt32OrLo:          return str8_lit("UInt32OrLo");
  case GNU_PropertyX86_UInt32OrHi:          return str8_lit("UInt32OrHi");
  case GNU_PropertyX86_UInt32OrAndLo:       return str8_lit("UInt32OrAndLo");
  case GNU_PropertyX86_UInt32OrAndHi:       return str8_lit("UInt32OrAndHi");
  }
  return str8_zero();
}

internal String8
gnu_string_from_property_flags_x86(Arena *arena, GNU_PropertyX86 prop, U32 flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List fmt = {0};
  if (flags == 0) {
    str8_list_pushf(scratch.arena, &fmt, "None");
  }
  switch (prop) {
  case GNU_PropertyX86_Isa1needed:
  case GNU_PropertyX86_Isa1Used: {
    if (flags & GNU_PropertyX86Isa1_BaseLine) {
      str8_list_pushf(scratch.arena, &fmt, "BaseLine");
      flags &= ~GNU_PropertyX86Isa1_BaseLine;
    }
    if (flags & GNU_PropertyX86Isa1_V2) {
      str8_list_pushf(scratch.arena, &fmt, "V2");
      flags &= ~GNU_PropertyX86Isa1_V2;
    }
    if (flags & GNU_PropertyX86Isa1_V3) {
      str8_list_pushf(scratch.arena, &fmt, "V3");
      flags &= ~GNU_PropertyX86Isa1_V3;
    }
    if (flags & GNU_PropertyX86Isa1_V4) {
      str8_list_pushf(scratch.arena, &fmt, "V4");
      flags &= ~GNU_PropertyX86Isa1_V4;
    }
  } break;
  case GNU_PropertyX86_Feature1And: {
    if (flags & GNU_PropertyX86Feature1_Ibt) {
      str8_list_pushf(scratch.arena, &fmt, "Ibt");
      flags &= ~GNU_PropertyX86Feature1_Ibt;
    }
    if (flags & GNU_PropertyX86Feature1_Shstk) {
      str8_list_pushf(scratch.arena, &fmt, "Shstk");
      flags &= ~GNU_PropertyX86Feature1_Shstk;
    }
    if (flags & GNU_PropertyX86Feature1_LamU48) {
      str8_list_pushf(scratch.arena, &fmt, "LamU48");
      flags &= ~GNU_PropertyX86Feature1_LamU48;
    }
    if (flags & GNU_PropertyX86Feature1_LamU57) {
      str8_list_pushf(scratch.arena, &fmt, "LamU57");
      flags &= ~GNU_PropertyX86Feature1_LamU57;
    }
  } break;
  case GNU_PropertyX86_Feature2Used: {
    if (flags & GNU_PropertyX86Feature2_X86) {
      str8_list_pushf(scratch.arena, &fmt, "X86");
      flags &= ~GNU_PropertyX86Feature2_X86;
    }
    if (flags & GNU_PropertyX86Feature2_X87) {
      str8_list_pushf(scratch.arena, &fmt, "X87");
      flags &= ~GNU_PropertyX86Feature2_X87;
    }
    if (flags & GNU_PropertyX86Feature2_MMX) {
      str8_list_pushf(scratch.arena, &fmt, "MMX");
      flags &= ~GNU_PropertyX86Feature2_MMX;
    }
    if (flags & GNU_PropertyX86Feature2_XMM) {
      str8_list_pushf(scratch.arena, &fmt, "XMM");
      flags &= ~GNU_PropertyX86Feature2_XMM;
    }
    if (flags & GNU_PropertyX86Feature2_YMM) {
      str8_list_pushf(scratch.arena, &fmt, "YMM");
      flags &= ~GNU_PropertyX86Feature2_YMM;
    }
    if (flags & GNU_PropertyX86Feature2_ZMM) {
      str8_list_pushf(scratch.arena, &fmt, "ZMM");
      flags &= ~GNU_PropertyX86Feature2_ZMM;
    }
    if (flags & GNU_PropertyX86Feature2_FXSR) {
      str8_list_pushf(scratch.arena, &fmt, "FXSR");
      flags &= ~GNU_PropertyX86Feature2_FXSR;
    }
    if (flags & GNU_PropertyX86Feature2_XSAVE) {
      str8_list_pushf(scratch.arena, &fmt, "XSAVE");
      flags &= ~GNU_PropertyX86Feature2_XSAVE;
    }
    if (flags & GNU_PropertyX86Feature2_XSAVEOPT) {
      str8_list_pushf(scratch.arena, &fmt, "XSAVEOPT");
      flags &= ~GNU_PropertyX86Feature2_XSAVEOPT;
    }
    if (flags & GNU_PropertyX86Feature2_XSAVEC) {
      str8_list_pushf(scratch.arena, &fmt, "XSAVEC");
      flags &= ~GNU_PropertyX86Feature2_XSAVEC;
    }
    if (flags & GNU_PropertyX86Feature2_TMM) {
      str8_list_pushf(scratch.arena, &fmt, "TMM");
      flags &= ~GNU_PropertyX86Feature2_TMM;
    }
    if (flags & GNU_PropertyX86Feature2_MASK) {
      str8_list_pushf(scratch.arena, &fmt, "MASK");
      flags &= ~GNU_PropertyX86Feature2_MASK;
    }
  } break;
  case GNU_PropertyX86_Compat_isa_1_used:
  case GNU_PropertyX86_Compat_isa_1_needed: {
    if (flags & GNU_PropertyX86Compat1Isa1_486) {
      str8_list_pushf(scratch.arena, &fmt, "486");
      flags &= ~GNU_PropertyX86Compat1Isa1_486;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_586) {
      str8_list_pushf(scratch.arena, &fmt, "586");
      flags &= ~GNU_PropertyX86Compat1Isa1_586;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_686) {
      str8_list_pushf(scratch.arena, &fmt, "686");
      flags &= ~GNU_PropertyX86Compat1Isa1_686;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_SSE) {
      str8_list_pushf(scratch.arena, &fmt, "SSE");
      flags &= ~GNU_PropertyX86Compat1Isa1_SSE;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_SSE2) {
      str8_list_pushf(scratch.arena, &fmt, "SSE2");
      flags &= ~GNU_PropertyX86Compat1Isa1_SSE2;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_SSE3) {
      str8_list_pushf(scratch.arena, &fmt, "SSE3");
      flags &= ~GNU_PropertyX86Compat1Isa1_SSE3;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_SSSE3) {
      str8_list_pushf(scratch.arena, &fmt, "SSSE3");
      flags &= ~GNU_PropertyX86Compat1Isa1_SSSE3;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_SSE4_1) {
      str8_list_pushf(scratch.arena, &fmt, "SSE4_1");
      flags &= ~GNU_PropertyX86Compat1Isa1_SSE4_1;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_SSE4_2) {
      str8_list_pushf(scratch.arena, &fmt, "SSE4_2");
      flags &= ~GNU_PropertyX86Compat1Isa1_SSE4_2;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_AVX) {
      str8_list_pushf(scratch.arena, &fmt, "AVX");
      flags &= ~GNU_PropertyX86Compat1Isa1_AVX;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_AVX2) {
      str8_list_pushf(scratch.arena, &fmt, "AVX2");
      flags &= ~GNU_PropertyX86Compat1Isa1_AVX2;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_AVX512F) {
      str8_list_pushf(scratch.arena, &fmt, "AVX512F");
      flags &= ~GNU_PropertyX86Compat1Isa1_AVX512F;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_AVX512ER) {
      str8_list_pushf(scratch.arena, &fmt, "AVX512ER");
      flags &= ~GNU_PropertyX86Compat1Isa1_AVX512ER;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_AVX512PF) {
      str8_list_pushf(scratch.arena, &fmt, "AVX512PF");
      flags &= ~GNU_PropertyX86Compat1Isa1_AVX512PF;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_AVX512VL) {
      str8_list_pushf(scratch.arena, &fmt, "AVX512VL");
      flags &= ~GNU_PropertyX86Compat1Isa1_AVX512VL;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_AVX512DQ) {
      str8_list_pushf(scratch.arena, &fmt, "AVX512DQ");
      flags &= ~GNU_PropertyX86Compat1Isa1_AVX512DQ;
    }
    if (flags & GNU_PropertyX86Compat1Isa1_AVX512BW) {
      str8_list_pushf(scratch.arena, &fmt, "AVX512BW");
      flags &= ~GNU_PropertyX86Compat1Isa1_AVX512BW;
    }
  } break;
  }
  if (flags) {
    str8_list_pushf(scratch.arena, &fmt, "Unknown: 0x%x", flags);
  }

  String8 result = str8_list_join(arena, &fmt, &(StringJoin){.sep = str8_lit(", ")}); 
  scratch_end(scratch);
  return result;
}

