// Copyright (c) 2025 Epic Games Tools
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

typedef U32 ELF_PType;
enum
{
  ELF_PType_Null        = 0,
  ELF_PType_Load        = 1,
  ELF_PType_Dynamic     = 2,
  ELF_PType_Interp      = 3,
  ELF_PType_Note        = 4,
  ELF_PType_ShLib       = 5,
  ELF_PType_PHdr        = 6,
  ELF_PType_Tls         = 7,
  ELF_PType_LoOs        = 0x60000000,
  ELF_PType_HiOs        = 0x6fffffff,

  ELF_PType_LowProc     = 0x70000000,
  ELF_PType_HighProc    = 0x7fffffff,

  // specific to Sun
  ELF_PType_LowSunW     = 0x6ffffffa,
  ELF_PType_SunWBSS     = 0x6ffffffb,
  ELF_PType_GnuEHFrame  = 0x6474E550,
  
  ELF_PType_GnuStack    = ELF_PType_LoOs + 0x474e550, // frame unwind information
  ELF_PType_GnuRelro    = ELF_PType_LoOs + 0x474e551, // stack flags
  ELF_PType_GnuProperty = ELF_PType_LoOs + 0x474e552, // read-only after relocations
  ELF_PType_SunEHFrame  = ELF_PType_GnuEHFrame,
};

typedef U32 ELF_PFlag;
enum
{
  ELF_PFlag_Exec  = (1 << 0),
  ELF_PFlag_Write = (1 << 1),
  ELF_PFlag_Read  = (1 << 2),
};

typedef U32 ELF_SectionCode;
enum
{
  ELF_SectionCode_Null                   = 0,
  ELF_SectionCode_ProgBits               = 1,
  ELF_SectionCode_Symtab                 = 2,
  ELF_SectionCode_Strtab                 = 3,
  ELF_SectionCode_Rela                   = 4,
  ELF_SectionCode_Hash                   = 5,
  ELF_SectionCode_Dynamic                = 6,
  ELF_SectionCode_Note                   = 7,
  ELF_SectionCode_NoBits                 = 8,
  ELF_SectionCode_Rel                    = 9,
  ELF_SectionCode_Shlib                  = 10,
  ELF_SectionCode_Dynsym                 = 11,
  ELF_SectionCode_InitArray              = 14,
  ELF_SectionCode_FiniArray              = 15,         // Array of ptrs to init functions
  ELF_SectionCode_PreinitArray           = 16,         // Array of ptrs to finish functions
  ELF_SectionCode_Group                  = 17,         // Array of ptrs to pre-init funcs
  ELF_SectionCode_SymtabShndx            = 18,         // Section contains a section group
  ELF_SectionCode_GNU_IncrementalInputs  = 0x6fff4700, // Indices for SHN_XINDEX entries
  ELF_SectionCode_GNU_Attributes         = 0x6ffffff5, // Incremental build data
  ELF_SectionCode_GNU_Hash               = 0x6ffffff6, // Object attributes
  ELF_SectionCode_GNU_LibList            = 0x6ffffff7, // GNU style symbol hash table
  ELF_SectionCode_SUNW_verdef            = 0x6ffffffd,
  ELF_SectionCode_SUNW_verneed           = 0x6ffffffe, // Versions defined by file
  ELF_SectionCode_SUNW_versym            = 0x6fffffff, // Versions needed by file

  // Symbol versions
  ELF_SectionCode_GNU_verdef             = ELF_SectionCode_SUNW_verdef,
  ELF_SectionCode_GNU_verneed            = ELF_SectionCode_SUNW_verneed,
  ELF_SectionCode_GNU_versym             = ELF_SectionCode_SUNW_versym,
  ELF_SectionCode_Proc,
  ELF_SectionCode_User,
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

typedef U32 ELF_SectionFlag;
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
typedef U32 ELF_AuxType;
enum
{
  ELF_AuxType_Null              = 0,
  ELF_AuxType_Phdr              = 3, // program headers
  ELF_AuxType_Phent             = 4, // size of a program header
  ELF_AuxType_Phnum             = 5, // number of program headers
  ELF_AuxType_Pagesz            = 6, // system page size
  ELF_AuxType_Base              = 7, // interpreter base address
  ELF_AuxType_Flags             = 8,
  ELF_AuxType_Entry             = 9, // program entry point
  ELF_AuxType_Uid               = 11,
  ELF_AuxType_Euid              = 12,
  ELF_AuxType_Gid               = 13,
  ELF_AuxType_Egid              = 14,
  ELF_AuxType_Platform          = 15,
  ELF_AuxType_Hwcap             = 16,
  ELF_AuxType_Clktck            = 17,
  ELF_AuxType_DCacheBSize       = 19,
  ELF_AuxType_ICacheBSize       = 20,
  ELF_AuxType_UCacheBSize       = 21,
  ELF_AuxType_IgnorePPC         = 22,
  ELF_AuxType_Secure            = 23,
  ELF_AuxType_BasePlatform      = 24,
  ELF_AuxType_Random            = 25,
  ELF_AuxType_Hwcap2            = 26, // addres to 16 random bytes
  ELF_AuxType_ExecFn            = 31, 
  ELF_AuxType_SysInfo           = 32, // file name of executable
  ELF_AuxType_SysInfoEhdr       = 33,
  ELF_AuxType_L1I_CacheSize     = 40,
  ELF_AuxType_L1I_CacheGeometry = 41,
  ELF_AuxType_L1D_CacheSize     = 42,
  ELF_AuxType_L1D_CacheGeometry = 43,
  ELF_AuxType_L2_CacheSize      = 44,
  ELF_AuxType_L2_CacheGeometry  = 45,
  ELF_AuxType_L3_CacheSize      = 46,
  ELF_AuxType_L3_CacheGeometry  = 47,
};

typedef U32 ELF_DynTag;
enum
{
  ELF_DynTag_Null            = 0,

  ELF_DynTag_Needed          = 1,
  ELF_DynTag_PltRelsz        = 2,
  ELF_DynTag_PltGot          = 3,
  ELF_DynTag_Hash            = 4,
  ELF_DynTag_Strtab          = 5,
  ELF_DynTag_Symtab          = 6,
  ELF_DynTag_Rela            = 7,
  ELF_DynTag_Relasz          = 8,
  ELF_DynTag_Relaent         = 9,
  ELF_DynTag_Strsz           = 10,
  ELF_DynTag_Syment          = 11,
  ELF_DynTag_Init            = 12,
  ELF_DynTag_Fini            = 13,
  ELF_DynTag_SoName          = 14,
  ELF_DynTag_RPath           = 15,
  ELF_DynTag_Symbolic        = 16,
  ELF_DynTag_Rel             = 17,
  ELF_DynTag_Relsz           = 18,
  ELF_DynTag_Relent          = 19,
  ELF_DynTag_Pltrel          = 20,
  ELF_DynTag_Debug           = 21,
  ELF_DynTag_TextRel         = 22,
  ELF_DynTag_JmpRel          = 23,
  ELF_DynTag_BindNow         = 24,
  ELF_DynTag_InitArray       = 25,
  ELF_DynTag_FiniArray       = 26,
  ELF_DynTag_InitArraysz     = 27,
  ELF_DynTag_FIniArraysz     = 28,
  ELF_DynTag_RunPath         = 29,
  ELF_DynTag_Flags           = 30,
  ELF_DynTag_PreInitArray    = 32,
  ELF_DynTag_PreInitArraysz  = 33,
  ELF_DynTag_SymtabShndx     = 34,

  ELF_DynTag_LoOs            = 0x6000000D,
  ELF_DynTag_HiOs            = 0x6ffff000,

  ELF_DynTag_ValRngLo        = 0x6ffffd00,
  ELF_DynTag_GNU_PreLinked   = 0x6ffffdf5,
  ELF_DynTag_GNU_Conflictsz  = 0x6ffffdf6,
  ELF_DynTag_GNU_LibListsz   = 0x6ffffdf7,
  ELF_DynTag_Checksum        = 0x6ffffdf8,
  ELF_DynTag_Pltpadsz        = 0x6ffffdf9,
  ELF_DynTag_Moveent         = 0x6ffffdfa,
  ELF_DynTag_Movesz          = 0x6ffffdfb,
  ELF_DynTag_Feature         = 0x6ffffdfc,
  ELF_DynTag_PosFlag_1       = 0x6ffffdfd,
  ELF_DynTag_SymInSz         = 0x6ffffdfe,
  ELF_DynTag_SymInEnt        = 0x6ffffdff,
  ELF_DynTag_ValRngHi        = ELF_DynTag_SymInEnt,

  ELF_DynTag_AddrRngLo       = 0x6ffffe00,
  ELF_DynTag_GNU_Hash        = 0x6ffffef5,
  ELF_DynTag_TlsDescPlt      = 0x6ffffef6,
  ELF_DynTag_TlsDescGot      = 0x6ffffef7,
  ELF_DynTag_GNU_Conflict    = 0x6ffffef8,
  ELF_DynTag_GNU_LibList     = 0x6ffffef9,
  ELF_DynTag_Config          = 0x6ffffefa,
  ELF_DynTag_DepAudit        = 0x6ffffefb,
  ELF_DynTag_Audit           = 0x6ffffefc,
  ELF_DynTag_PltPad          = 0x6ffffefd,
  ELF_DynTag_MoveTab         = 0x6ffffefe,
  ELF_DynTag_SymInfo         = 0x6ffffeff,
  ELF_DynTag_AddrRngHi       = ELF_DynTag_SymInfo,

  ELF_DynTag_RelaCount       = 0x6ffffff9,
  ELF_DynTag_RelCount        = 0x6ffffffa,
  ELF_DynTag_Flags_1         = 0x6ffffffb,
  ELF_DynTag_VerDef          = 0x6ffffffc,
  ELF_DynTag_VerDefNum       = 0x6ffffffd,
  ELF_DynTag_VerNeed         = 0x6ffffffe,
  ELF_DynTag_VerNeedNum      = 0x6fffffff,
  ELF_DynTag_VerSym          = 0x6ffffff0,
  ELF_DynTag_LoProc          = 0x70000000,
  ELF_DynTag_HiProc          = 0x7fffffff,
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
  ELF_NoteType_GNU_Abi           = 1,
  ELF_NoteType_GNU_HwCap         = 2,
  ELF_NoteType_GNU_BuildId       = 3,
  ELF_NoteType_GNU_GoldVersion   = 4,
  ELF_NoteType_GNU_PropertyType0 = 5,
};

typedef U32 ELF_GnuABITag;
enum
{
  ELF_GnuABITag_Linux    = 0,
  ELF_GnuABITag_Hurd     = 1,
  ELF_GnuABITag_Solaris  = 2,
  ELF_GnuABITag_FreeBsd  = 3,
  ELF_GnuABITag_NetBsd   = 4,
  ELF_GnuABITag_Syllable = 5,
  ELF_GnuABITag_Nacl     = 6,
};

typedef S32 ELF_GnuProperty;
enum
{
  ELF_GnuProperty_LoProc            = 0xc0000000,
  //  processor-specific range
  ELF_GnuProperty_HiProc            = 0xdfffffff,
  ELF_GnuProperty_LoUser            = 0xe0000000,
  //  application-specific range
  ELF_GnuProperty_HiUser            = 0xffffffff,
  ELF_GnuProperty_StackSize         = 1,
  ELF_GnuProperty_NoCopyOnProtected = 2,
};

typedef U32 ELF_GnuPropertyX86Isa1;
enum
{
  ELF_GnuPropertyX86Isa1_BaseLine = (1 << 0),
  ELF_GnuPropertyX86Isa1_V2       = (1 << 1),
  ELF_GnuPropertyX86Isa1_V3       = (1 << 2),
  ELF_GnuPropertyX86Isa1_V4       = (1 << 3),
};

typedef U32 ELF_GnuPropertyX86Compat1Isa1;
enum
{
  ELF_GnuPropertyX86Compat1Isa1_486      = (1 << 0),
  ELF_GnuPropertyX86Compat1Isa1_586      = (1 << 1),
  ELF_GnuPropertyX86Compat1Isa1_686      = (1 << 2),
  ELF_GnuPropertyX86Compat1Isa1_SSE      = (1 << 3),
  ELF_GnuPropertyX86Compat1Isa1_SSE2     = (1 << 4),
  ELF_GnuPropertyX86Compat1Isa1_SSE3     = (1 << 5),
  ELF_GnuPropertyX86Compat1Isa1_SSSE3    = (1 << 6),
  ELF_GnuPropertyX86Compat1Isa1_SSE4_1   = (1 << 7),
  ELF_GnuPropertyX86Compat1Isa1_SSE4_2   = (1 << 8),
  ELF_GnuPropertyX86Compat1Isa1_AVX      = (1 << 9),
  ELF_GnuPropertyX86Compat1Isa1_AVX2     = (1 << 10),
  ELF_GnuPropertyX86Compat1Isa1_AVX512F  = (1 << 11),
  ELF_GnuPropertyX86Compat1Isa1_AVX512ER = (1 << 12),
  ELF_GnuPropertyX86Compat1Isa1_AVX512PF = (1 << 13),
  ELF_GnuPropertyX86Compat1Isa1_AVX512VL = (1 << 14),
  ELF_GnuPropertyX86Compat1Isa1_AVX512DQ = (1 << 15),
  ELF_GnuPropertyX86Compat1Isa1_AVX512BW = (1 << 16),
};

typedef U32 ELF_GnuPropertyX86Compat2Isa1;
enum
{
  ELF_GnuPropertyX86Compat2Isa1_CMOVE         = (1 << 0),
  ELF_GnuPropertyX86Compat2Isa1_SSE           = (1 << 1),
  ELF_GnuPropertyX86Compat2Isa1_SSE2          = (1 << 2),
  ELF_GnuPropertyX86Compat2Isa1_SSE3          = (1 << 3),
  ELF_GnuPropertyX86Compat2Isa1_SSE4_1        = (1 << 4),
  ELF_GnuPropertyX86Compat2Isa1_SSE4_2        = (1 << 5),
  ELF_GnuPropertyX86Compat2Isa1_AVX           = (1 << 6),
  ELF_GnuPropertyX86Compat2Isa1_AVX2          = (1 << 7),
  ELF_GnuPropertyX86Compat2Isa1_FMA           = (1 << 8),
  ELF_GnuPropertyX86Compat2Isa1_AVX512F       = (1 << 9),
  ELF_GnuPropertyX86Compat2Isa1_AVX512CD      = (1 << 10),
  ELF_GnuPropertyX86Compat2Isa1_AVX512ER      = (1 << 11),
  ELF_GnuPropertyX86Compat2Isa1_AVX512PF      = (1 << 12),
  ELF_GnuPropertyX86Compat2Isa1_AVX512VL      = (1 << 13),
  ELF_GnuPropertyX86Compat2Isa1_AVX512DQ      = (1 << 14),
  ELF_GnuPropertyX86Compat2Isa1_AVX512BW      = (1 << 15),
  ELF_GnuPropertyX86Compat2Isa1_AVX512_4FMAPS = (1 << 16),
  ELF_GnuPropertyX86Compat2Isa1_AVX512_4VNNIW = (1 << 17),
  ELF_GnuPropertyX86Compat2Isa1_AVX512_BITALG = (1 << 18),
  ELF_GnuPropertyX86Compat2Isa1_AVX512_IFMA   = (1 << 19),
  ELF_GnuPropertyX86Compat2Isa1_AVX512_VBMI   = (1 << 20),
  ELF_GnuPropertyX86Compat2Isa1_AVX512_VBMI2  = (1 << 21),
  ELF_GnuPropertyX86Compat2Isa1_AVX512_VNNI   = (1 << 22),
  ELF_GnuPropertyX86Compat2Isa1_AVX512_BF16   = (1 << 23),
};

typedef S32 ELF_GnuPropertyX86;
enum
{
  ELF_GnuPropertyX86_Feature1And         = 0xc0000002,
  ELF_GnuPropertyX86_Feature2Used        = 0xc0010001,
  ELF_GnuPropertyX86_Isa1needed          = 0xc0008002,
  ELF_GnuPropertyX86_Isa2Needed          = 0xc0008001,
  ELF_GnuPropertyX86_Isa1Used            = 0xc0010002,
  ELF_GnuPropertyX86_Compat_isa_1_used   = 0xc0000000,
  ELF_GnuPropertyX86_Compat_isa_1_needed = 0xc0000001,
  ELF_GnuPropertyX86_UInt32AndLo         = ELF_GnuPropertyX86_Feature1And,
  ELF_GnuPropertyX86_UInt32AndHi         = 0xc0007fff,
  ELF_GnuPropertyX86_UInt32OrLo          = 0xc0008000,
  ELF_GnuPropertyX86_UInt32OrHi          = 0xc000ffff,
  ELF_GnuPropertyX86_UInt32OrAndLo       = 0xc0010000,
  ELF_GnuPropertyX86_UInt32OrAndHi       = 0xc0017fff,
};

typedef U32 ELF_GnuPropertyX86Feature1;
enum
{
  ELF_GnuPropertyX86Feature1_Ibt    = (1 << 0),
  ELF_GnuPropertyX86Feature1_Shstk  = (1 << 1),
  ELF_GnuPropertyX86Feature1_LamU48 = (1 << 2),
  ELF_GnuPropertyX86Feature1_LamU57 = (1 << 3),
};

typedef U32 ELF_GnuPropertyX86Feature2;
enum
{
  ELF_GnuPropertyX86Feature2_X86      = (1 << 0),
  ELF_GnuPropertyX86Feature2_X87      = (1 << 1),
  ELF_GnuPropertyX86Feature2_MMX      = (1 << 2),
  ELF_GnuPropertyX86Feature2_XMM      = (1 << 3),
  ELF_GnuPropertyX86Feature2_YMM      = (1 << 4),
  ELF_GnuPropertyX86Feature2_ZMM      = (1 << 5),
  ELF_GnuPropertyX86Feature2_FXSR     = (1 << 6),
  ELF_GnuPropertyX86Feature2_XSAVE    = (1 << 7),
  ELF_GnuPropertyX86Feature2_XSAVEOPT = (1 << 8),
  ELF_GnuPropertyX86Feature2_XSAVEC   = (1 << 9),
  ELF_GnuPropertyX86Feature2_TMM      = (1 << 10),
  ELF_GnuPropertyX86Feature2_MASK     = (1 << 11),
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

typedef U16 ELF_Type;
typedef enum ELF_TypeEnum
{
  ELF_Type_None   = 0,
  ELF_Type_Rel    = 1,
  ELF_Type_Exec   = 2,
  ELF_Type_Dyn    = 3,
  ELF_Type_Core   = 4,
  ELF_Type_LoOs   = 0xfe00,
  ELF_Type_HiOs   = 0xfeff,
  ELF_Type_LoProc = 0xff00,
  ELF_Type_HiProc = 0xffff,
} ELF_TypeEnum;

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
  U32 sh_name;
  U32 sh_type;
  U64 sh_flags;
  U64 sh_addr;
  U64 sh_offset;
  U64 sh_size;
  U32 sh_link;
  U32 sh_info;
  U64 sh_addralign;
  U64 sh_entsize;
} ELF_Shdr64;

typedef struct ELF_Shdr32
{
  U32 sh_name;
  U32 sh_type;
  U32 sh_flags;
  U32 sh_addr;
  U32 sh_offset;
  U32 sh_size;
  U32 sh_link;
  U32 sh_info;
  U32 sh_addralign;
  U32 sh_entsize;
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
// Auxiliary Vectors

// these appear in /proc/<pid>/auxv of a process, they are not in elf files

typedef struct ELF_Auxv32
{
  U32 a_type;
  U32 a_val;
} ELF_Auxv32;

typedef struct ELF_Auxv64
{
  U64 a_type;
  U64 a_val;
} ELF_Auxv64;

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

typedef struct ELF_LinkMap32
{
  U32 base;
  U32 name;
  U32 ld;
  U32 next;
} ELF_LinkMap32;

typedef struct ELF_LinkMap64
{
  U64 base;
  U64 name;
  U64 ld;
  U64 next;
} ELF_LinkMap64;

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

typedef struct
{
  U32 name_size;
  U32 desc_size;
  U32 type;
  // name + desc
  // U8  data[1];
} ELF_Note;

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
  U64 ch_type;
  U64 ch_size;
  U64 ch_addr_align;
} ELF_Chdr64;

////////////////////////////////

internal ELF_Hdr64  elf_hdr64_from_ehdr32(ELF_Hdr32 h32);
internal ELF_Shdr64 elf_shdr64_from_shdr32(ELF_Shdr32 h32);
internal ELF_Phdr64 elf_phdr64_from_phdr32(ELF_Phdr32 h32);
internal ELF_Dyn64  elf_dyn64_from_dyn32  (ELF_Dyn32 h32);
internal ELF_Sym64  elf_sym64_from_sym32  (ELF_Sym32 sym32);
internal ELF_Rel64  elf_rel64_from_rel32  (ELF_Rel32 rel32);
internal ELF_Rela64 elf_rela64_from_rela32(ELF_Rela32 rela32);
internal ELF_Chdr64 elf_chdr64_from_chdr32(ELF_Chdr32 chdr32);

////////////////////////////////

internal String8 elf_string_from_class(Arena *arena, ELF_Class v);

////////////////////////////////

internal Arch arch_from_elf_machine(ELF_MachineKind machine);

#endif // ELF_H

