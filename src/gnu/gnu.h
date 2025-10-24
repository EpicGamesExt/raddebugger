// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef GNU_H
#define GNU_H

typedef ELF_NoteType GNU_NoteType;
enum
{
  GNU_NoteType_Abi           = 1,
  GNU_NoteType_HwCap         = 2,
  GNU_NoteType_BuildId       = 3,
  GNU_NoteType_GoldVersion   = 4,
  GNU_NoteType_PropertyType0 = 5,
};

typedef U32 GNU_Property;
enum
{
  GNU_Property_LoProc            = 0xc0000000,
  //  processor-specific range
  GNU_Property_HiProc            = 0xdfffffff,
  GNU_Property_LoUser            = 0xe0000000,
  //  application-specific range
  GNU_Property_HiUser            = 0xffffffff,
  GNU_Property_StackSize         = 1,
  GNU_Property_NoCopyOnProtected = 2,
};

typedef U32 GNU_PropertyX86Isa1;
enum
{
  GNU_PropertyX86Isa1_BaseLine = (1 << 0),
  GNU_PropertyX86Isa1_V2       = (1 << 1),
  GNU_PropertyX86Isa1_V3       = (1 << 2),
  GNU_PropertyX86Isa1_V4       = (1 << 3),
};

typedef U32 GNU_PropertyX86Compat1Isa1;
enum
{
  GNU_PropertyX86Compat1Isa1_486      = (1 << 0),
  GNU_PropertyX86Compat1Isa1_586      = (1 << 1),
  GNU_PropertyX86Compat1Isa1_686      = (1 << 2),
  GNU_PropertyX86Compat1Isa1_SSE      = (1 << 3),
  GNU_PropertyX86Compat1Isa1_SSE2     = (1 << 4),
  GNU_PropertyX86Compat1Isa1_SSE3     = (1 << 5),
  GNU_PropertyX86Compat1Isa1_SSSE3    = (1 << 6),
  GNU_PropertyX86Compat1Isa1_SSE4_1   = (1 << 7),
  GNU_PropertyX86Compat1Isa1_SSE4_2   = (1 << 8),
  GNU_PropertyX86Compat1Isa1_AVX      = (1 << 9),
  GNU_PropertyX86Compat1Isa1_AVX2     = (1 << 10),
  GNU_PropertyX86Compat1Isa1_AVX512F  = (1 << 11),
  GNU_PropertyX86Compat1Isa1_AVX512ER = (1 << 12),
  GNU_PropertyX86Compat1Isa1_AVX512PF = (1 << 13),
  GNU_PropertyX86Compat1Isa1_AVX512VL = (1 << 14),
  GNU_PropertyX86Compat1Isa1_AVX512DQ = (1 << 15),
  GNU_PropertyX86Compat1Isa1_AVX512BW = (1 << 16),
};

typedef U32 GNU_PropertyX86Compat2Isa1;
enum
{
  GNU_PropertyX86Compat2Isa1_CMOVE         = (1 << 0),
  GNU_PropertyX86Compat2Isa1_SSE           = (1 << 1),
  GNU_PropertyX86Compat2Isa1_SSE2          = (1 << 2),
  GNU_PropertyX86Compat2Isa1_SSE3          = (1 << 3),
  GNU_PropertyX86Compat2Isa1_SSE4_1        = (1 << 4),
  GNU_PropertyX86Compat2Isa1_SSE4_2        = (1 << 5),
  GNU_PropertyX86Compat2Isa1_AVX           = (1 << 6),
  GNU_PropertyX86Compat2Isa1_AVX2          = (1 << 7),
  GNU_PropertyX86Compat2Isa1_FMA           = (1 << 8),
  GNU_PropertyX86Compat2Isa1_AVX512F       = (1 << 9),
  GNU_PropertyX86Compat2Isa1_AVX512CD      = (1 << 10),
  GNU_PropertyX86Compat2Isa1_AVX512ER      = (1 << 11),
  GNU_PropertyX86Compat2Isa1_AVX512PF      = (1 << 12),
  GNU_PropertyX86Compat2Isa1_AVX512VL      = (1 << 13),
  GNU_PropertyX86Compat2Isa1_AVX512DQ      = (1 << 14),
  GNU_PropertyX86Compat2Isa1_AVX512BW      = (1 << 15),
  GNU_PropertyX86Compat2Isa1_AVX512_4FMAPS = (1 << 16),
  GNU_PropertyX86Compat2Isa1_AVX512_4VNNIW = (1 << 17),
  GNU_PropertyX86Compat2Isa1_AVX512_BITALG = (1 << 18),
  GNU_PropertyX86Compat2Isa1_AVX512_IFMA   = (1 << 19),
  GNU_PropertyX86Compat2Isa1_AVX512_VBMI   = (1 << 20),
  GNU_PropertyX86Compat2Isa1_AVX512_VBMI2  = (1 << 21),
  GNU_PropertyX86Compat2Isa1_AVX512_VNNI   = (1 << 22),
  GNU_PropertyX86Compat2Isa1_AVX512_BF16   = (1 << 23),
};

typedef GNU_Property GNU_PropertyX86;
enum
{
  GNU_PropertyX86_Feature1And         = 0xc0000002,
  GNU_PropertyX86_Feature2Used        = 0xc0010001,
  GNU_PropertyX86_Isa1needed          = 0xc0008002,
  GNU_PropertyX86_Isa2Needed          = 0xc0008001,
  GNU_PropertyX86_Isa1Used            = 0xc0010002,
  GNU_PropertyX86_Compat_isa_1_used   = 0xc0000000,
  GNU_PropertyX86_Compat_isa_1_needed = 0xc0000001,
  GNU_PropertyX86_UInt32AndLo         = GNU_PropertyX86_Feature1And,
  GNU_PropertyX86_UInt32AndHi         = 0xc0007fff,
  GNU_PropertyX86_UInt32OrLo          = 0xc0008000,
  GNU_PropertyX86_UInt32OrHi          = 0xc000ffff,
  GNU_PropertyX86_UInt32OrAndLo       = 0xc0010000,
  GNU_PropertyX86_UInt32OrAndHi       = 0xc0017fff,
};

typedef U32 GNU_PropertyX86Feature1;
enum
{
  GNU_PropertyX86Feature1_Ibt    = (1 << 0),
  GNU_PropertyX86Feature1_Shstk  = (1 << 1),
  GNU_PropertyX86Feature1_LamU48 = (1 << 2),
  GNU_PropertyX86Feature1_LamU57 = (1 << 3),
};

typedef U32 GNU_PropertyX86Feature2;
enum
{
  GNU_PropertyX86Feature2_X86      = (1 << 0),
  GNU_PropertyX86Feature2_X87      = (1 << 1),
  GNU_PropertyX86Feature2_MMX      = (1 << 2),
  GNU_PropertyX86Feature2_XMM      = (1 << 3),
  GNU_PropertyX86Feature2_YMM      = (1 << 4),
  GNU_PropertyX86Feature2_ZMM      = (1 << 5),
  GNU_PropertyX86Feature2_FXSR     = (1 << 6),
  GNU_PropertyX86Feature2_XSAVE    = (1 << 7),
  GNU_PropertyX86Feature2_XSAVEOPT = (1 << 8),
  GNU_PropertyX86Feature2_XSAVEC   = (1 << 9),
  GNU_PropertyX86Feature2_TMM      = (1 << 10),
  GNU_PropertyX86Feature2_MASK     = (1 << 11),
};

typedef U32 GNU_AbiTag;
enum
{
  GNU_AbiTag_Linux    = 0,
  GNU_AbiTag_Hurd     = 1,
  GNU_AbiTag_Solaris  = 2,
  GNU_AbiTag_FreeBsd  = 3,
  GNU_AbiTag_NetBsd   = 4,
  GNU_AbiTag_Syllable = 5,
  GNU_AbiTag_Nacl     = 6,
};

typedef struct GNU_LinkMap64
{
  U64 addr_vaddr;
  U64 name_vaddr;
  U64 ld_vaddr;   // address of the dynamic section
  U64 next_vaddr;
  U64 prev_vaddr;
} GNU_LinkMap64;

typedef struct GNU_LinkMap32
{
  U32 addr_vaddr;
  U32 name_vaddr;
  U32 ld_vaddr;
  U32 next_vaddr;
  U32 prev_vaddr;
} GNU_LinkMap32;

typedef U32 GNU_RT;
enum
{
  GNU_RT_Consistent = 0,
  GNU_RT_Add        = 1,
  GNU_RT_Delete     = 2,
};

// struct reflects r_debug from /usr/include/link.h
typedef struct GNU_RDebugInfo64
{
  S32    r_version; // must be greater than 0
  U64    r_map;     // address of first loaded object
  U64    r_brk;     // when module is loared/unloaded DL calls this function
  GNU_RT r_state;
  U64    r_ldbase;  // base addres of dynamic linker
} GNU_RDebugInfo64;

typedef struct GNU_RDebugInfo32
{
  S32    r_version;
  U32    r_map;
  U32    r_brk;
  GNU_RT r_state;
  U32    r_ldbase;
} GNU_RDebugInfo32;

////////////////////////////////

internal GNU_LinkMap64 elf_linkmap64_from_linkmap32(GNU_LinkMap32 linkmap32);
internal U64 gnu_rdebug_info_size_from_arch(Arch arch);
internal U64 gnu_r_brk_offset_from_arch(Arch arch);

////////////////////////////////
//~ enum

internal String8 gnu_string_from_abi_tag(GNU_AbiTag abi_tag);
internal String8 gnu_string_from_note_type(GNU_NoteType note_type);
internal String8 gnu_string_from_property_x86(GNU_PropertyX86 prop);

#endif // GNU_H
