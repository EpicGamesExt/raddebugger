// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ELF_H
#define ELF_H

typedef U8 ELF_Class;
enum
{
  ELF_Class_None  = 0,
  ELF_Class_32    = 1,
  ELF_Class_64    = 2,
  ELF_Class_Count = 3
};

typedef U8 ELF_OsAbi;
enum
{
  ELF_OsAbi_None,
  ELF_OsAbi_SYSV,
  ELF_OsAbi_HPUX,
  ELF_OsAbi_NETBSD,
  ELF_OsAbi_GNU,
  ELF_OsAbi_LINUX,
  ELF_OsAbi_SOLARIS,
  ELF_OsAbi_IRIX,
  ELF_OsAbi_FREEBSD,
  ELF_OsAbi_TRU64,
  ELF_OsAbi_ARM = 97,
  ELF_OsAbi_STANDALONE = 255,
};

typedef U8 ELF_Version;
enum
{
  ELF_Version_None,
  ELF_Version_Current,
};

typedef U16 ELF_MachineKind;
enum
{
  ELF_MachineKind_None        = 0,
  ELF_MachineKind_M32         = 1,
  ELF_MachineKind_SPARC       = 2,
  ELF_MachineKind_386         = 3,
  ELF_MachineKind_68K         = 4,
  ELF_MachineKind_88K         = 5,
  ELF_MachineKind_IAMCU       = 6,
  ELF_MachineKind_860         = 7,
  ELF_MachineKind_MIPS        = 8,
  ELF_MachineKind_S370        = 9,
  ELF_MachineKind_MIPS_RS3_LE = 10,
  //  11-14 reserved
  ELF_MachineKind_PARISC      = 15,
  //  16 reserved
  ELF_MachineKind_VPP500      = 17,
  ELF_MachineKind_SPARC32PLUS = 18,
  //  nick: Sun's "v8plus"
  ELF_MachineKind_INTEL960    = 19,
  ELF_MachineKind_PPC         = 20,
  ELF_MachineKind_PPC64       = 21,
  ELF_MachineKind_S390        = 22,
  ELF_MachineKind_SPU         = 23,
  //  24-35 reserved
  ELF_MachineKind_V800        = 36,
  ELF_MachineKind_FR20        = 37,
  ELF_MachineKind_RH32        = 38,
  ELF_MachineKind_MCORE       = 39,
  ELF_MachineKind_ARM         = 40,
  ELF_MachineKind_SH          = 42,
  ELF_MachineKind_ALPHA       = 41,
  ELF_MachineKind_SPARCV9     = 43,
  ELF_MachineKind_TRICORE     = 44,
  ELF_MachineKind_ARC         = 45,
  ELF_MachineKind_H8_300      = 46,
  ELF_MachineKind_H8_300H     = 47,
  ELF_MachineKind_H8S         = 48,
  ELF_MachineKind_H8_500      = 49,
  ELF_MachineKind_IA_64       = 50,
  ELF_MachineKind_MIPS_X      = 51,
  ELF_MachineKind_COLDFILE    = 52,
  ELF_MachineKind_68HC12      = 53,
  ELF_MachineKind_MMA         = 54,
  ELF_MachineKind_PCP         = 55,
  ELF_MachineKind_NCPU        = 56,
  ELF_MachineKind_NDR1        = 57,
  ELF_MachineKind_STARCORE    = 58,
  ELF_MachineKind_ME16        = 59,
  ELF_MachineKind_ST100       = 60,
  ELF_MachineKind_TINYJ       = 61,
  ELF_MachineKind_X86_64      = 62,
  ELF_MachineKind_AARCH64     = 183,
  ELF_MachineKind_TI_C6000    = 140,
  ELF_MachineKind_L1OM        = 180,
  ELF_MachineKind_K1OM        = 181,
  ELF_MachineKind_RISCV       = 243,
  ELF_MachineKind_S390_OLD    = 0xA390,
};

typedef U8 ELF_Data;
enum
{
  ELF_Data_None = 0,
  ELF_Data_2LSB = 1,
  ELF_Data_2MSB = 2,
};

typedef U16 ELF_Type;
enum
{
  ELF_Type_None   = 0,
  ELF_Type_Rel    = 1,
  ELF_Type_Exec   = 2,
  ELF_Type_Dyn    = 3,
  ELF_Type_Core   = 4,
  ELF_Type_LoOs   = 0xfe00,
  ELF_Type_HiOs   = 0xff00,
  ELF_Type_LoProc = 0xff00,
  ELF_Type_HiProc = 0xffff
};

typedef U32 ELF_PhdrType;
enum
{
  ELF_PhdrType_Null        = 0,
  ELF_PhdrType_Load        = 1,
  ELF_PhdrType_Dynamic     = 2,
  ELF_PhdrType_Interp      = 3,
  ELF_PhdrType_Note        = 4,
  ELF_PhdrType_ShLib       = 5,
  ELF_PhdrType_Phdr        = 6,
  ELF_PhdrType_Tls         = 7,
  ELF_PhdrType_LoOs        = 0x60000000,
  ELF_PhdrType_HiOs        = 0x6fffffff,
  
  ELF_PhdrType_LowProc     = 0x70000000,
  ELF_PhdrType_HighProc    = 0x7fffffff,
  
  ELF_PhdrType_GnuEHFrame  = ELF_PhdrType_LoOs + 0x474E550, // segment with .eh_frame_hdr
  ELF_PhdrType_GnuStack    = ELF_PhdrType_LoOs + 0x474e551, // frame unwind information
  ELF_PhdrType_GnuRelro    = ELF_PhdrType_LoOs + 0x474e552, // stack flags
  ELF_PhdrType_GnuProperty = ELF_PhdrType_LoOs + 0x474e553, // read-only after relocations
};

typedef U32 ELF_PFlag;
enum
{
  ELF_PFlag_Exec  = (1 << 0),
  ELF_PFlag_Write = (1 << 1),
  ELF_PFlag_Read  = (1 << 2),
};

typedef U32 ELF_ShType;
enum
{
  ELF_ShType_Null                   = 0,
  ELF_ShType_ProgBits               = 1,
  ELF_ShType_Symtab                 = 2,
  ELF_ShType_Strtab                 = 3,
  ELF_ShType_Rela                   = 4,
  ELF_ShType_Hash                   = 5,
  ELF_ShType_Dynamic                = 6,
  ELF_ShType_Note                   = 7,
  ELF_ShType_NoBits                 = 8,
  ELF_ShType_Rel                    = 9,
  ELF_ShType_Shlib                  = 10,
  ELF_ShType_Dynsym                 = 11,
  ELF_ShType_InitArray              = 14,
  ELF_ShType_FiniArray              = 15,         // Array of ptrs to init functions
  ELF_ShType_PreinitArray           = 16,         // Array of ptrs to finish functions
  ELF_ShType_Group                  = 17,         // Array of ptrs to pre-init funcs
  ELF_ShType_SymtabShndx            = 18,         // Section contains a section group
  
  ELF_ShType_GNU_IncrementalInputs  = 0x6fff4700, // Indices for SHN_XINDEX entries
  ELF_ShType_GNU_Attributes         = 0x6ffffff5, // Incremental build data
  ELF_ShType_GNU_Hash               = 0x6ffffff6, // Object attributes
  ELF_ShType_GNU_LibList            = 0x6ffffff7, // GNU style symbol hash table
  
  ELF_ShType_SUNW_verdef            = 0x6ffffffd,
  ELF_ShType_SUNW_verneed           = 0x6ffffffe, // Versions defined by file
  ELF_ShType_SUNW_versym            = 0x6fffffff, // Versions needed by file
  
  // Symbol versions
  ELF_ShType_GNU_verdef             = ELF_ShType_SUNW_verdef,
  ELF_ShType_GNU_verneed            = ELF_ShType_SUNW_verneed,
  ELF_ShType_GNU_versym             = ELF_ShType_SUNW_versym,
  ELF_ShType_Proc,
  ELF_ShType_User,
};

typedef U32 ELF_SectionIndex;
enum
{
  
  ELF_SectionIndex_Undef             = 0,      // Symbol with section index is undefined and must be resolved by the link editor
  ELF_SectionIndex_Abs               = 0xfff1, // Symbol has absolute value and wont change after relocations
  ELF_SectionIndex_Common            = 0xfff2, // This symbol indicates to linker to allocate the storage at address multiple of st_value
  
  ELF_SectionIndex_LoReserve         = 0xff00,
  ELF_SectionIndex_HiReserve         = 0xffff,
  
  // Processor specific
  ELF_SectionIndex_LoProc            = ELF_SectionIndex_LoReserve,
  ELF_SectionIndex_HiProc            = 0xff1f,
  
  //  Reserved for OS
  ELF_SectionIndex_LoOs              = 0xff20,
  ELF_SectionIndex_HiOs              = 0xff3f,
  
  ELF_SectionIndex_IA64_ASNI_Common  = ELF_SectionIndex_LoProc,
  ELF_SectionIndex_X8664_LCommon     = 0xff02,
  ELF_SectionIndex_MIPS_SCommon      = 0xff03,
  
  ELF_SectionIndex_TIC6X_Common      = ELF_SectionIndex_LoReserve,
  ELF_SectionIndex_MIPS_SUndefined   = 0xff04,
};

typedef U32 ELF_SectionFlags;
enum
{
  ELF_Shf_Write            = (1 << 0),
  ELF_Shf_Alloc            = (1 << 1),
  ELF_Shf_ExecInstr        = (1 << 2),
  ELF_Shf_Merge            = (1 << 4),
  ELF_Shf_Strings          = (1 << 5),
  ELF_Shf_InfoLink         = (1 << 6),
  ELF_Shf_LinkOrder        = (1 << 7),
  ELF_Shf_OsNonConforming  = (1 << 8),
  ELF_Shf_Group            = (1 << 9),
  ELF_Shf_Tls              = (1 << 10),
  ELF_Shf_Compressed       = (1 << 11),
  ELF_Shf_MaskOs_Shift     = 16, ELF_Shf_MaskOs_Mask = 0xff,
  ELF_Shf_AMD64Large       = (1 << 28),
  ELF_Shf_Ordered          = (1 << 30),
  ELF_Shf_Exclude          = (1 << 31),
  ELF_Shf_MaskProc_Shift   = 28, ELF_Shf_MaskProc_Mask = 0xf,
};

#define ELF_SectionFlag_Extract_MaskOs(f)   (U8)(((f) >> ELF_SectionFlag_MaskOs_Shift)   & ELF_SectionFlag_MaskOs_Mask)
#define ELF_SectionFlag_Extract_MaskProc(f) (U8)(((f) >> ELF_SectionFlag_MaskProc_shift) & ELF_SectionFlag_MaskProc_Mask)

typedef enum
{
  ELF_DynTagValueKind_Null,
  ELF_DynTagValueKind_Value,
  ELF_DynTagValueKind_Address,
  ELF_DynTagValueKind_Special,
} ELF_DynTagValueKind;

// ELF Dynamic Tags
//
// X(name, id, kind): kind describes how d_un is interpreted.
#define ELF_DynTag_XList          \
  X(Needed,          1,  Value  ) \
  X(PltRelsz,        2,  Value  ) \
  X(PltGot,          3,  Address) \
  X(Hash,            4,  Address) \
  X(Strtab,          5,  Address) \
  X(Symtab,          6,  Address) \
  X(Rela,            7,  Address) \
  X(Relasz,          8,  Value  ) \
  X(Relaent,         9,  Value  ) \
  X(Strsz,           10, Value  ) \
  X(Syment,          11, Value  ) \
  X(Init,            12, Address) \
  X(Fini,            13, Address) \
  X(SoName,          14, Value  ) \
  X(RPath,           15, Value  ) \
  X(Symbolic,        16, Value  ) \
  X(Rel,             17, Address) \
  X(Relsz,           18, Value  ) \
  X(Relent,          19, Value  ) \
  X(Pltrel,          20, Value  ) \
  X(Debug,           21, Special) \
  X(TextRel,         22, Value  ) \
  X(JmpRel,          23, Address) \
  X(BindNow,         24, Value  ) \
  X(InitArray,       25, Address) \
  X(FiniArray,       26, Address) \
  X(InitArraysz,     27, Value  ) \
  X(FIniArraysz,     28, Value  ) \
  X(RunPath,         29, Value  ) \
  X(Flags,           30, Value  ) \
  X(PreInitArray,    32, Address) \
  X(PreInitArraysz,  33, Value  ) \
  X(SymtabShndx,     34, Address)

// GNU Dynamic Tag Extensions
#define ELF_DynTag_GNU_XList             \
  X(PreLinked,      0x6ffffdf5, Value  ) \
  X(Conflictsz,     0x6ffffdf6, Value  ) \
  X(LibListsz,      0x6ffffdf7, Value  ) \
  X(Checksum,       0x6ffffdf8, Value  ) \
  X(Pltpadsz,       0x6ffffdf9, Value  ) \
  X(Moveent,        0x6ffffdfa, Value  ) \
  X(Movesz,         0x6ffffdfb, Value  ) \
  X(Feature,        0x6ffffdfc, Value  ) \
  X(SymInSz,        0x6ffffdfe, Value  ) \
  X(SymInEnt,       0x6ffffdff, Value  ) \
  X(Hash,           0x6ffffef5, Address) \
  X(TlsDescPlt,     0x6ffffef6, Address) \
  X(TlsDescGot,     0x6ffffef7, Address) \
  X(Conflict,       0x6ffffef8, Address) \
  X(LibList,        0x6ffffef9, Address) \
  X(Config,         0x6ffffefa, Address) \
  X(DepAudit,       0x6ffffefb, Address) \
  X(Audit,          0x6ffffefc, Address) \
  X(PltPad,         0x6ffffefd, Address) \
  X(MoveTab,        0x6ffffefe, Address) \
  X(SymInfo,        0x6ffffeff, Address) \
  X(VerSym,         0x6ffffff0, Address) \
  X(RelaCount,      0x6ffffff9, Value  ) \
  X(RelCount,       0x6ffffffa, Value  ) \
  X(VerDef,         0x6ffffffc, Address) \
  X(VerDefNum,      0x6ffffffd, Value  ) \
  X(VerNeed,        0x6ffffffe, Address) \
  X(VerNeedNum,     0x6fffffff, Value  ) \
  X(PosFlag_1,      0x6ffffdfd, Value  ) \
  X(Flags_1,        0x6ffffffb, Value  )

#define ELF_DynTag_All_XList \
  ELF_DynTag_XList           \
  ELF_DynTag_GNU_XList


typedef U32 ELF_DynTag;
enum
{
  ELF_DynTag_Null = 0,

  // ELF
#define X(n, id, ...) ELF_DynTag_##n = id,
  ELF_DynTag_XList
#undef X

  // GNU
#define X(n, id, ...) ELF_DynTag_GNU_##n = id,
  ELF_DynTag_GNU_XList
#undef X

  // OS range
  ELF_DynTag_LoOs = 0x6000000D,
  ELF_DynTag_HiOs = 0x6ffff000,

  // value fallback range
  ELF_DynTag_ValRngLo = 0x6ffffd00,
  ELF_DynTag_ValRngHi = ELF_DynTag_GNU_SymInEnt,

  // address fallback range
  ELF_DynTag_AddrRngLo = 0x6ffffe00,
  ELF_DynTag_AddrRngHi = ELF_DynTag_GNU_SymInfo,

  // processor specific range
  ELF_DynTag_LoProc = 0x70000000,
  ELF_DynTag_HiProc = 0x7fffffff,
};

typedef U32 ELF_DynFlag;
enum
{
  ELF_DynFlag_Origin    = (1 << 0),
  ELF_DynFlag_Symbolic  = (1 << 1),
  ELF_DynFlag_TextTel   = (1 << 2),
  ELF_DynFlag_BindNow   = (1 << 3),
  ELF_DynFlag_StaticTls = (1 << 4),
};

typedef U32 ELF_DynFeatureFlag;
enum
{
  ELF_DynFeatureFlag_ParInit = (1 << 0),
  ELF_DynFeatureFlag_ConfExp = (1 << 1),
};

typedef U8 ELF_SymBind;
enum
{
  //  the same name may exists in multiple files without interfering with each other. 
  ELF_SymBind_Local  = 0,
  //  Visible to all objects that are linked together. 
  ELF_SymBind_Global = 1,
  //  If there is a global symbol with identical name linker doesn't issue an error.
  ELF_SymBind_Weak   = 2,
  ELF_SymBind_LoProc = 13,
  ELF_SymBind_HiProc = 15,
};

typedef U8 ELF_SymType;
enum
{
  ELF_SymType_NoType  = 0,
  //  Type is not specified.
  ELF_SymType_Object  = 1,
  //  Symbol is associated with data object, such as a variable, an array, etc.
  ELF_SymType_Func    = 2,
  //  Symbol is associated with a function.
  ELF_SymType_Section = 3,
  //  Symbol is used to relocate sections and normally have LOCAL binding.
  ELF_SymType_File    = 4,
  //  Gives name of the source file associated with object.
  ELF_SymType_Common  = 5,
  ELF_SymType_Tls     = 6,
  ELF_SymType_LoProc  = 13,
  ELF_SymType_HiProc  = 15,
};

typedef U8 ELF_SymVisibility;
enum
{
  ELF_SymVisibility_Default   = 0,
  ELF_SymVisibility_Internal  = 1,
  ELF_SymVisibility_Hidden    = 2,
  ELF_SymVisibility_Protected = 3,
};

typedef U32 ELF_RelocI386;
enum
{
  ELF_RelocI386_None           = 0,
  ELF_RelocI386_32             = 1,
  ELF_RelocI386_PC32           = 2,
  ELF_RelocI386_GOT32          = 3,
  ELF_RelocI386_PLT32          = 4,
  ELF_RelocI386_Copy           = 5,
  ELF_RelocI386_GlobDat        = 6,
  ELF_RelocI386_JumpSlot       = 7,
  ELF_RelocI386_Relative       = 8,
  ELF_RelocI386_GotOff         = 9,
  ELF_RelocI386_GotPc          = 10,
  ELF_RelocI386_32Plt          = 11,
  ELF_RelocI386_Tls_tpoff      = 14,
  ELF_RelocI386_Tls_ie         = 15,
  ELF_RelocI386_Tls_gotie      = 16,
  ELF_RelocI386_Tls_le         = 17,
  ELF_RelocI386_Tls_gd         = 18,
  ELF_RelocI386_Tls_ldm        = 19,
  ELF_RelocI386_16             = 20,
  ELF_RelocI386_PC16           = 21,
  ELF_RelocI386_8              = 22,
  ELF_RelocI386_Pc8            = 23,
  ELF_RelocI386_TlsGd32        = 24,
  ELF_RelocI386_TlsGdPush      = 25,
  ELF_RelocI386_TlsGdCall      = 26,
  ELF_RelocI386_TlsGdPop       = 27,
  ELF_RelocI386_TlsLdm32       = 28,
  ELF_RelocI386_TlsLdmPush     = 29,
  ELF_RelocI386_TlsLdmCall     = 30,
  ELF_RelocI386_TlsLdmPop      = 31,
  ELF_RelocI386_TlsLdo32       = 32,
  ELF_RelocI386_TlsIe32        = 33,
  ELF_RelocI386_TlsLe32        = 34,
  ELF_RelocI386_TlsDtpmod32    = 35,
  ELF_RelocI386_TlsDtpoff32    = 36,
  ELF_RelocI386_TlsTpoff32     = 37,
  //  38 is not taken
  ELF_RelocI386_TlsGotDesc     = 39,
  ELF_RelocI386_TlsDescCall    = 40,
  ELF_RelocI386_TlsDesc        = 41,
  ELF_RelocI386_IRelative      = 42,
  ELF_RelocI386_Gotx32x        = 43,
  ELF_RelocI386_UsedByIntel200 = 200,
  ELF_RelocI386_GNU_VTInherit  = 250,
  ELF_RelocI386_GNU_VTEntry    = 251,
};

typedef U32 ELF_RelocX8664;
enum
{
  ELF_RelocX8664_None           = 0,
  ELF_RelocX8664_64             = 1,
  ELF_RelocX8664_Pc32           = 2,
  ELF_RelocX8664_Got32          = 3,
  ELF_RelocX8664_Plt32          = 4,
  ELF_RelocX8664_Copy           = 5,
  ELF_RelocX8664_GlobDat        = 6,
  ELF_RelocX8664_JumpSlot       = 7,
  ELF_RelocX8664_Relative       = 8,
  ELF_RelocX8664_GotPcRel       = 9,
  ELF_RelocX8664_32             = 10,
  ELF_RelocX8664_32S            = 11,
  ELF_RelocX8664_16             = 12,
  ELF_RelocX8664_Pc16           = 13,
  ELF_RelocX8664_8              = 14,
  ELF_RelocX8664_Pc8            = 15,
  ELF_RelocX8664_DtpMod64       = 16,
  ELF_RelocX8664_DtpOff64       = 17,
  ELF_RelocX8664_TpOff64        = 18,
  ELF_RelocX8664_TlsGd          = 19,
  ELF_RelocX8664_TlsLd          = 20,
  ELF_RelocX8664_DtpOff32       = 21,
  ELF_RelocX8664_GotTpOff       = 22,
  ELF_RelocX8664_TpOff32        = 23,
  ELF_RelocX8664_Pc64           = 24,
  ELF_RelocX8664_GotOff64       = 25,
  ELF_RelocX8664_GotPc32        = 26,
  ELF_RelocX8664_Got64          = 27,
  ELF_RelocX8664_GotPcRel64     = 28,
  ELF_RelocX8664_GotPc64        = 29,
  ELF_RelocX8664_GotPlt64       = 30,
  ELF_RelocX8664_PltOff64       = 31,
  ELF_RelocX8664_Size32         = 32,
  ELF_RelocX8664_Size64         = 33,
  ELF_RelocX8664_GotPc32TlsDesc = 34,
  ELF_RelocX8664_TlsDescCall    = 35,
  ELF_RelocX8664_TlsDesc        = 36,
  ELF_RelocX8664_IRelative      = 37,
  ELF_RelocX8664_Relative64     = 38,
  ELF_RelocX8664_Pc32Bnd        = 39,
  ELF_RelocX8664_Plt32Bnd       = 40,
  ELF_RelocX8664_GotPcRelx      = 41,
  ELF_RelocX8664_RexGotPcRelx   = 42,
  ELF_RelocX8664_GNU_VTInherit  = 250,
  ELF_RelocX8664_GNU_VTEntry    = 251,
};

typedef U32 ELF_ExternalVerFlag;
enum
{
  ELF_ExternalVerFlag_Base = (1 << 0),
  ELF_ExternalVerFlag_Weak = (1 << 1),
  ELF_ExternalVerFlag_Info = (1 << 2),
};

typedef U32 ELF_NoteType;
enum
{
  ELF_NoteType_STapSdt = 3, // System Tap probes
};

#define ELF_HdrIs64Bit(e_ident) (e_ident[ELF_Identifier_Class] == ELF_Class_64)
#define ELF_HdrIs32Bit(e_ident) (e_ident[ELF_Identifier_Class] == ELF_Class_32)

typedef enum ELF_Identifier
{
  ELF_Identifier_Mag0       = 0,
  ELF_Identifier_Mag1       = 1,
  ELF_Identifier_Mag2       = 2,
  ELF_Identifier_Mag3       = 3,
  ELF_Identifier_Class      = 4,
  ELF_Identifier_Data       = 5,
  ELF_Identifier_Version    = 6,
  ELF_Identifier_OsAbi      = 7,
  ELF_Identfiier_AbiBersion = 8,
  ELF_Identifier_Max        = 16,
} ELF_Identifier;

read_only global U8 elf_magic[] = {0x7f, 'E', 'L', 'F'};
read_only global String8 elf_magic_string = {elf_magic, sizeof(elf_magic)};

typedef struct ELF_Hdr64
{
  U8              e_ident[ELF_Identifier_Max];
  ELF_Type        e_type;
  ELF_MachineKind e_machine;
  U32             e_version;
  U64             e_entry;
  U64             e_phoff;
  U64             e_shoff;
  U32             e_flags;
  U16             e_ehsize;
  U16             e_phentsize;
  U16             e_phnum;
  U16             e_shentsize;
  U16             e_shnum;
  U16             e_shstrndx;
} ELF_Hdr64;

typedef struct ELF_Hdr32
{
  U8              e_ident[ELF_Identifier_Max];
  ELF_Type        e_type;
  ELF_MachineKind e_machine;
  U32             e_version;
  U32             e_entry;
  U32             e_phoff;
  U32             e_shoff;
  U32             e_flags;
  U16             e_ehsize;
  U16             e_phentsize;
  U16             e_phnum;
  U16             e_shentsize;
  U16             e_shnum;
  U16             e_shstrndx;
} ELF_Hdr32;

typedef struct ELF_Shdr64
{
  U32        sh_name;
  ELF_ShType sh_type;
  U64        sh_flags;
  U64        sh_addr;
  U64        sh_offset;
  U64        sh_size;
  U32        sh_link;
  U32        sh_info;
  U64        sh_addralign;
  U64        sh_entsize;
} ELF_Shdr64;

typedef struct ELF_Shdr32
{
  U32        sh_name;
  ELF_ShType sh_type;
  U32        sh_flags;
  U32        sh_addr;
  U32        sh_offset;
  U32        sh_size;
  U32        sh_link;
  U32        sh_info;
  U32        sh_addralign;
  U32        sh_entsize;
} ELF_Shdr32;

typedef struct ELF_Phdr64
{
  U32 p_type;
  U32 p_flags;
  U64 p_offset;
  U64 p_vaddr;
  U64 p_paddr;
  U64 p_filesz;
  U64 p_memsz;
  U64 p_align;
} ELF_Phdr64;

typedef struct ELF_Phdr32
{
  U32 p_type;
  U32 p_offset;
  U32 p_vaddr;
  U32 p_paddr;
  U32 p_filesz;
  U32 p_memsz;
  U32 p_flags;
  U32 p_align;
} ELF_Phdr32;

////////////////////////////////
// Dynamic Structures

// these appear in the virtual address space of a process, they are not in elf files

typedef struct ELF_Dyn32
{
  U32 tag;
  U32 val;
} ELF_Dyn32;

typedef struct ELF_Dyn64
{
  U64 tag;
  U64 val;
} ELF_Dyn64;

////////////////////////////////
// Imports and Exports

typedef struct 
{
  U32 st_name;  // Holds index into files string table.
  U32 st_value; // Depending on the context, this may be address, size, etc.
  U32 st_size;  // Data size in bytes. Zero when size is unknown.
  U8  st_info;  // Contains symbols type and binding.
  U8  st_other; // Reserved for future use, currenly zero.
  U16 st_shndx; // Section index to which symbol is relevant.
} ELF_Sym32;

typedef struct 
{
  U32 st_name;
  U8  st_info;
  U8  st_other;
  U16 st_shndx;
  U64 st_value;
  U64 st_size;
} ELF_Sym64;

#define ELF_ST_INFO(b,t)     (((b) << 4) + ((t) & 0xF))
#define ELF_ST_BIND(x)       ((x) >> 4)
#define ELF_ST_TYPE(x)       ((x) & 0xF)
#define ELF_ST_VISIBILITY(v) ((v) & 0x3)

typedef struct
{
  U32 r_offset;
  U32 r_info;
} ELF_Rel32;

typedef struct
{
  U32 r_offset;
  U32 r_info;
  S32 r_addend;
} ELF_Rela32;

typedef struct
{
  U64 r_offset;
  U64 r_info;
} ELF_Rel64;

typedef struct
{
  U64 r_offset;
  U64 r_info;
  S64 r_addend;
} ELF_Rela64;

#define ELF32_R_SYM(x)  ((x) >> 8)
#define ELF32_R_TYPE(x) ((x) & 0xFF)

#define ELF64_R_INFO(s,t) (((U64)(s) << 32) | (U64)t)
#define ELF64_R_SYM(x)    ((x) >> 32)
#define ELF64_R_TYPE(x)   ((x) & 0xffffffff)

// This flag is set to indicate that symbol is not available outside shared object
#define ELF_EXTERNAL_VERSYM_HIDDEN 0x8000
#define ELF_EXTERNAL_VERSYM_MASK   0x7FFF

// Appears in .gnu.verdef (SHT_GNU_verdef)
typedef struct
{
  U16 vd_version;
  U16 vd_flags;
  U16 vd_ndx;
  U16 vd_cnt;
  U32 vd_hash;
  U32 vd_aux;
  U32 vd_next;
} ELF_ExternalVerdef;

// Appears in .gnu.verdef (SHT_GNU_verdef)
typedef struct
{
  U32 vda_name;
  U32 vda_next;
} ELF_ExternalVerdaux;

// Appears in .gnu.verneed (SHT_GNU_verneed)
typedef struct
{
  U16 vn_version;
  U16 vn_cnt;
  U32 vn_file;
  U32 vn_aux;
  U32 vn_next;
} ELF_ExternalVerneed;

// Appears in .gnu.verneed (SHT_GNU_verneed)
typedef struct
{
  U32 vna_hash;
  U16 vna_flags;
  U16 vna_other;
  U32 vna_name;
  U32 vna_next;
} ELF_ExternalVernaux;

// Appears in .gnu.version (SHT_GNU_versym)
typedef struct
{
  U16 vs_vers;
} ELF_ExternalVersym;

////////////////////////////////
// Extensions

typedef U8 ELF_CompressType;
enum ELF_CompressTypeEnum
{
  ELF_CompressType_None = 0,
  ELF_CompressType_ZLib = 1,
  ELF_CompressType_ZStd = 2,
  
  ELF_CompressType_LoOs = 0x60000000,
  ELF_CompressType_HiOs = 0x6fffffff,
  
  ELF_CompressType_LoProc = 0x70000000,
  ELF_CompressType_HiProc = 0x7fffffff,
};

typedef struct ELF_Chdr32
{
  U32 ch_type;
  U32 ch_size;
  U32 ch_addr_align;
} ELF_Chdr32;

typedef struct ELF_Chdr64
{
  U32 ch_type;
  U32 ch_reserved;
  U64 ch_size;
  U64 ch_addr_align;
} ELF_Chdr64;

////////////////////////////////
// 32 -> 64 bit conversions

internal ELF_Hdr64  elf_hdr64_from_hdr32  (ELF_Hdr32 h32);
internal ELF_Shdr64 elf_shdr64_from_shdr32(ELF_Shdr32 h32);
internal ELF_Phdr64 elf_phdr64_from_phdr32(ELF_Phdr32 h32);
internal ELF_Dyn64  elf_dyn64_from_dyn32  (ELF_Dyn32 h32);
internal ELF_Sym64  elf_sym64_from_sym32  (ELF_Sym32 sym32);
internal ELF_Rel64  elf_rel64_from_rel32  (ELF_Rel32 rel32);
internal ELF_Rela64 elf_rela64_from_rela32(ELF_Rela32 rela32);
internal ELF_Chdr64 elf_chdr64_from_chdr32(ELF_Chdr32 chdr32);

////////////////////////////////
// enum -> string

internal String8 elf_string_from_class(Arena *arena, ELF_Class v);

////////////////////////////////
// Format Helpers

internal U64 elf_phdr_size_from_class(ELF_Class elf_class);
internal U64 elf_dyn_size_from_class (ELF_Class elf_class);
internal U64 elf_sym_size_from_class (ELF_Class elf_class);

internal U32 elf_hash_sysv_from_string(String8 string);
internal U32 elf_hash_gnu_from_string (String8 string);

internal ELF_DynTagValueKind elf_value_kind_from_dyn_tag(U64 tag);

////////////////////////////////
// Compat Readers

internal MachineOpResult elf_read_ehdr  (MachineOp_MemRead *mem_read, void *mem_read_ud, U64 addr, ELF_Hdr64 *ehdr_out);
internal MachineOpResult elf_read_phdr  (MachineOp_MemRead *mem_read, void *mem_read_ud, U64 addr, ELF_Class elf_class, ELF_Phdr64 *phdr_out);
internal MachineOpResult elf_read_shdr  (MachineOp_MemRead *mem_read, void *mem_read_ud, U64 addr, ELF_Class elf_class, ELF_Shdr64 *shdr_out);
internal MachineOpResult elf_read_dyn   (MachineOp_MemRead *mem_read, void *mem_read_ud, U64 addr, ELF_Class elf_class, ELF_Dyn64  *dyn_out);
internal MachineOpResult elf_read_symbol(MachineOp_MemRead *mem_read, void *mem_read_ud, U64 addr, ELF_Class elf_class, ELF_Sym64  *symbol_out);

internal B32 elf_read_ehdr_string(String8 string, ELF_Hdr64 *hdr_out);

#endif // ELF_H
