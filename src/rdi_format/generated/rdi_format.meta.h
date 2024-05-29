// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef RDI_FORMAT_META_H
#define RDI_FORMAT_META_H

typedef RDI_U32 RDI_DataSectionTag;
typedef enum RDI_DataSectionTagEnum
{
RDI_DataSectionTag_NULL                 = 0x0000,
RDI_DataSectionTag_TopLevelInfo         = 0x0001,
RDI_DataSectionTag_StringData           = 0x0002,
RDI_DataSectionTag_StringTable          = 0x0003,
RDI_DataSectionTag_IndexRuns            = 0x0004,
RDI_DataSectionTag_BinarySections       = 0x0005,
RDI_DataSectionTag_FilePathNodes        = 0x0006,
RDI_DataSectionTag_SourceFiles          = 0x0007,
RDI_DataSectionTag_Units                = 0x0008,
RDI_DataSectionTag_UnitVmap             = 0x0009,
RDI_DataSectionTag_TypeNodes            = 0x000A,
RDI_DataSectionTag_UDTs                 = 0x000B,
RDI_DataSectionTag_Members              = 0x000C,
RDI_DataSectionTag_EnumMembers          = 0x000D,
RDI_DataSectionTag_GlobalVariables      = 0x000E,
RDI_DataSectionTag_GlobalVmap           = 0x000F,
RDI_DataSectionTag_ThreadVariables      = 0x0010,
RDI_DataSectionTag_Procedures           = 0x0011,
RDI_DataSectionTag_Scopes               = 0x0012,
RDI_DataSectionTag_ScopeVoffData        = 0x0013,
RDI_DataSectionTag_ScopeVmap            = 0x0014,
RDI_DataSectionTag_Locals               = 0x0015,
RDI_DataSectionTag_LocationBlocks       = 0x0016,
RDI_DataSectionTag_LocationData         = 0x0017,
RDI_DataSectionTag_NameMaps             = 0x0018,
RDI_DataSectionTag_PRIMARY_COUNT        = 0x0019,
RDI_DataSectionTag_SKIP                 = RDI_DataSectionTag_SECONDARY|0x0000,
RDI_DataSectionTag_LineInfoVoffs        = RDI_DataSectionTag_SECONDARY|0x0001,
RDI_DataSectionTag_LineInfoData         = RDI_DataSectionTag_SECONDARY|0x0002,
RDI_DataSectionTag_LineInfoColumns      = RDI_DataSectionTag_SECONDARY|0x0003,
RDI_DataSectionTag_LineMapNumbers       = RDI_DataSectionTag_SECONDARY|0x0004,
RDI_DataSectionTag_LineMapRanges        = RDI_DataSectionTag_SECONDARY|0x0005,
RDI_DataSectionTag_LineMapVoffs         = RDI_DataSectionTag_SECONDARY|0x0006,
RDI_DataSectionTag_NameMapBuckets       = RDI_DataSectionTag_SECONDARY|0x0007,
RDI_DataSectionTag_NameMapNodes         = RDI_DataSectionTag_SECONDARY|0x0008,
} RDI_DataSectionTagEnum;

typedef RDI_U32 RDI_DataSectionEncoding;
typedef enum RDI_DataSectionEncodingEnum
{
RDI_DataSectionEncoding_Unpacked   = 0,
RDI_DataSectionEncoding_LZB        = 1,
} RDI_DataSectionEncodingEnum;

typedef RDI_U32 RDI_Arch;
typedef enum RDI_ArchEnum
{
RDI_Arch_NULL       = 0,
RDI_Arch_X86        = 1,
RDI_Arch_X64        = 2,
} RDI_ArchEnum;

typedef enum RDI_RegCodeX86
{
RDI_RegCodeX86_nil        = 0,
RDI_RegCodeX86_eax        = 1,
RDI_RegCodeX86_ecx        = 2,
RDI_RegCodeX86_edx        = 3,
RDI_RegCodeX86_ebx        = 4,
RDI_RegCodeX86_esp        = 5,
RDI_RegCodeX86_ebp        = 6,
RDI_RegCodeX86_esi        = 7,
RDI_RegCodeX86_edi        = 8,
RDI_RegCodeX86_fsbase     = 9,
RDI_RegCodeX86_gsbase     = 10,
RDI_RegCodeX86_eflags     = 11,
RDI_RegCodeX86_eip        = 12,
RDI_RegCodeX86_dr0        = 13,
RDI_RegCodeX86_dr1        = 14,
RDI_RegCodeX86_dr2        = 15,
RDI_RegCodeX86_dr3        = 16,
RDI_RegCodeX86_dr4        = 17,
RDI_RegCodeX86_dr5        = 18,
RDI_RegCodeX86_dr6        = 19,
RDI_RegCodeX86_dr7        = 20,
RDI_RegCodeX86_fpr0       = 21,
RDI_RegCodeX86_fpr1       = 22,
RDI_RegCodeX86_fpr2       = 23,
RDI_RegCodeX86_fpr3       = 24,
RDI_RegCodeX86_fpr4       = 25,
RDI_RegCodeX86_fpr5       = 26,
RDI_RegCodeX86_fpr6       = 27,
RDI_RegCodeX86_fpr7       = 28,
RDI_RegCodeX86_st0        = 29,
RDI_RegCodeX86_st1        = 30,
RDI_RegCodeX86_st2        = 31,
RDI_RegCodeX86_st3        = 32,
RDI_RegCodeX86_st4        = 33,
RDI_RegCodeX86_st5        = 34,
RDI_RegCodeX86_st6        = 35,
RDI_RegCodeX86_st7        = 36,
RDI_RegCodeX86_fcw        = 37,
RDI_RegCodeX86_fsw        = 38,
RDI_RegCodeX86_ftw        = 39,
RDI_RegCodeX86_fop        = 40,
RDI_RegCodeX86_fcs        = 41,
RDI_RegCodeX86_fds        = 42,
RDI_RegCodeX86_fip        = 43,
RDI_RegCodeX86_fdp        = 44,
RDI_RegCodeX86_mxcsr      = 45,
RDI_RegCodeX86_mxcsr_mask = 46,
RDI_RegCodeX86_ss         = 47,
RDI_RegCodeX86_cs         = 48,
RDI_RegCodeX86_ds         = 49,
RDI_RegCodeX86_es         = 50,
RDI_RegCodeX86_fs         = 51,
RDI_RegCodeX86_gs         = 52,
RDI_RegCodeX86_ymm0       = 53,
RDI_RegCodeX86_ymm1       = 54,
RDI_RegCodeX86_ymm2       = 55,
RDI_RegCodeX86_ymm3       = 56,
RDI_RegCodeX86_ymm4       = 57,
RDI_RegCodeX86_ymm5       = 58,
RDI_RegCodeX86_ymm6       = 59,
RDI_RegCodeX86_ymm7       = 60,
} RDI_RegCodeX86;

typedef enum RDI_RegCodeX64
{
RDI_RegCodeX64_nil        = 0,
RDI_RegCodeX64_rax        = 1,
RDI_RegCodeX64_rcx        = 2,
RDI_RegCodeX64_rdx        = 3,
RDI_RegCodeX64_rbx        = 4,
RDI_RegCodeX64_rsp        = 5,
RDI_RegCodeX64_rbp        = 6,
RDI_RegCodeX64_rsi        = 7,
RDI_RegCodeX64_rdi        = 8,
RDI_RegCodeX64_r8         = 9,
RDI_RegCodeX64_r9         = 10,
RDI_RegCodeX64_r10        = 11,
RDI_RegCodeX64_r11        = 12,
RDI_RegCodeX64_r12        = 13,
RDI_RegCodeX64_r13        = 14,
RDI_RegCodeX64_r14        = 15,
RDI_RegCodeX64_r15        = 16,
RDI_RegCodeX64_es         = 17,
RDI_RegCodeX64_cs         = 18,
RDI_RegCodeX64_ss         = 19,
RDI_RegCodeX64_ds         = 20,
RDI_RegCodeX64_fs         = 21,
RDI_RegCodeX64_gs         = 22,
RDI_RegCodeX64_rip        = 23,
RDI_RegCodeX64_rflags     = 24,
RDI_RegCodeX64_dr0        = 25,
RDI_RegCodeX64_dr1        = 26,
RDI_RegCodeX64_dr2        = 27,
RDI_RegCodeX64_dr3        = 28,
RDI_RegCodeX64_dr4        = 29,
RDI_RegCodeX64_dr5        = 30,
RDI_RegCodeX64_dr6        = 31,
RDI_RegCodeX64_dr7        = 32,
RDI_RegCodeX64_st0        = 33,
RDI_RegCodeX64_st1        = 34,
RDI_RegCodeX64_st2        = 35,
RDI_RegCodeX64_st3        = 36,
RDI_RegCodeX64_st4        = 37,
RDI_RegCodeX64_st5        = 38,
RDI_RegCodeX64_st6        = 39,
RDI_RegCodeX64_st7        = 40,
RDI_RegCodeX64_fpr0       = 41,
RDI_RegCodeX64_fpr1       = 42,
RDI_RegCodeX64_fpr2       = 43,
RDI_RegCodeX64_fpr3       = 44,
RDI_RegCodeX64_fpr4       = 45,
RDI_RegCodeX64_fpr5       = 46,
RDI_RegCodeX64_fpr6       = 47,
RDI_RegCodeX64_fpr7       = 48,
RDI_RegCodeX64_ymm0       = 49,
RDI_RegCodeX64_ymm1       = 50,
RDI_RegCodeX64_ymm2       = 51,
RDI_RegCodeX64_ymm3       = 52,
RDI_RegCodeX64_ymm4       = 53,
RDI_RegCodeX64_ymm5       = 54,
RDI_RegCodeX64_ymm6       = 55,
RDI_RegCodeX64_ymm7       = 56,
RDI_RegCodeX64_ymm8       = 57,
RDI_RegCodeX64_ymm9       = 58,
RDI_RegCodeX64_ymm10      = 59,
RDI_RegCodeX64_ymm11      = 60,
RDI_RegCodeX64_ymm12      = 61,
RDI_RegCodeX64_ymm13      = 62,
RDI_RegCodeX64_ymm14      = 63,
RDI_RegCodeX64_ymm15      = 64,
RDI_RegCodeX64_mxcsr      = 65,
RDI_RegCodeX64_fsbase     = 66,
RDI_RegCodeX64_gsbase     = 67,
RDI_RegCodeX64_fcw        = 68,
RDI_RegCodeX64_fsw        = 69,
RDI_RegCodeX64_ftw        = 70,
RDI_RegCodeX64_fop        = 71,
RDI_RegCodeX64_fcs        = 72,
RDI_RegCodeX64_fds        = 73,
RDI_RegCodeX64_fip        = 74,
RDI_RegCodeX64_fdp        = 75,
RDI_RegCodeX64_mxcsr_mask = 76,
} RDI_RegCodeX64;

typedef RDI_U32 RDI_BinarySectionFlags;
typedef enum RDI_BinarySectionFlagsEnum
{
RDI_BinarySectionFlags_Read       = 1<<0,
RDI_BinarySectionFlags_Write      = 1<<1,
RDI_BinarySectionFlags_Execute    = 1<<2,
} RDI_BinarySectionFlagsEnum;

typedef struct RDI_Header RDI_Header;
struct RDI_Header
{
RDI_U64 magic;
RDI_U32 encoding_version;
RDI_U32 data_section_off;
RDI_U32 data_section_count;
};

typedef struct RDI_DataSection RDI_DataSection;
struct RDI_DataSection
{
RDI_DataSectionTag tag;
RDI_DataSectionEncoding encoding;
};

typedef struct RDI_VMapEntry RDI_VMapEntry;
struct RDI_VMapEntry
{
RDI_U64 voff;
RDI_U64 idx;
};

typedef struct RDI_TopLevelInfo RDI_TopLevelInfo;
struct RDI_TopLevelInfo
{
RDI_Arch arch;
RDI_U32 exe_name_string_idx;
RDI_U64 exe_hash;
RDI_U64 voff_max;
};

C_LINKAGE_BEGIN
C_LINKAGE_END

#endif // RDI_FORMAT_META_H
