// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////////////////////////////////////
//~ (R)AD (D)ebug (I)nfo Format Library
//
// Defines standard RDI debug information format types and
// functions.

#ifndef RDI_H
#define RDI_H

////////////////////////////////////////////////////////////////
//~ Overridable Procedure Decoration

#if !defined(RDI_PROC)
# define RDI_PROC static
#endif

////////////////////////////////////////////////////////////////
//~ Overridable Basic Integer Types

#if !defined(RDI_U8)
# define RDI_U8 RDI_U8
# define RDI_U16 RDI_U16
# define RDI_U32 RDI_U32
# define RDI_U64 RDI_U64
# define RDI_S8 RDI_S8
# define RDI_S16 RDI_S16
# define RDI_S32 RDI_S32
# define RDI_S64 RDI_S64
#include <stdint.h>
typedef uint8_t  RDI_U8;
typedef uint16_t RDI_U16;
typedef uint32_t RDI_U32;
typedef uint64_t RDI_U64;
typedef int8_t   RDI_S8;
typedef int16_t  RDI_S16;
typedef int32_t  RDI_S32;
typedef int64_t  RDI_S64;
#endif

////////////////////////////////////////////////////////////////
//~ Checksum Types

typedef union RDI_MD5 RDI_MD5;
union RDI_MD5 {RDI_U8 u8[16]; RDI_U64 u64[2];};

typedef union RDI_SHA1 RDI_SHA1;
union RDI_SHA1 {RDI_U8 u8[20];};

typedef union RDI_SHA256 RDI_SHA256;
union RDI_SHA256 {RDI_U8 u8[32]; RDI_U64 u64[4];};

typedef union RDI_GUID RDI_GUID;
union RDI_GUID {RDI_U8 u8[16]; RDI_U64 u64[2];};

////////////////////////////////////////////////////////////////
//~ Overridable Enabling/Disabling Of Table Index Typechecking

#if !defined(RDI_DISABLE_TABLE_INDEX_TYPECHECKING)
# define RDI_DISABLE_TABLE_INDEX_TYPECHECKING 0
#endif

////////////////////////////////////////////////////////////////
//~ Format Constants

// "raddbg\0\0"
#define RDI_MAGIC_CONSTANT   0x0000676264646172
#define RDI_ENCODING_VERSION 17

////////////////////////////////////////////////////////////////
//~ Format Types & Functions

typedef RDI_U32 RDI_SectionKind;
typedef enum RDI_SectionKindEnum
{
RDI_SectionKind_NULL                 = 0x0000,
RDI_SectionKind_TopLevelInfo         = 0x0001,
RDI_SectionKind_StringData           = 0x0002,
RDI_SectionKind_StringTable          = 0x0003,
RDI_SectionKind_IndexRuns            = 0x0004,
RDI_SectionKind_BinarySections       = 0x0005,
RDI_SectionKind_FilePathNodes        = 0x0006,
RDI_SectionKind_SourceFiles          = 0x0007,
RDI_SectionKind_LineTables           = 0x0008,
RDI_SectionKind_LineInfoVOffs        = 0x0009,
RDI_SectionKind_LineInfoLines        = 0x000A,
RDI_SectionKind_LineInfoColumns      = 0x000B,
RDI_SectionKind_SourceLineMaps       = 0x000C,
RDI_SectionKind_SourceLineMapNumbers = 0x000D,
RDI_SectionKind_SourceLineMapRanges  = 0x000E,
RDI_SectionKind_SourceLineMapVOffs   = 0x000F,
RDI_SectionKind_Units                = 0x0010,
RDI_SectionKind_UnitVMap             = 0x0011,
RDI_SectionKind_TypeNodes            = 0x0012,
RDI_SectionKind_UDTs                 = 0x0013,
RDI_SectionKind_Members              = 0x0014,
RDI_SectionKind_EnumMembers          = 0x0015,
RDI_SectionKind_GlobalVariables      = 0x0016,
RDI_SectionKind_GlobalVMap           = 0x0017,
RDI_SectionKind_ThreadVariables      = 0x0018,
RDI_SectionKind_Constants            = 0x0019,
RDI_SectionKind_Procedures           = 0x001A,
RDI_SectionKind_Scopes               = 0x001B,
RDI_SectionKind_ScopeVOffData        = 0x001C,
RDI_SectionKind_ScopeVMap            = 0x001D,
RDI_SectionKind_InlineSites          = 0x001E,
RDI_SectionKind_Locals               = 0x001F,
RDI_SectionKind_LocationBlocks       = 0x0020,
RDI_SectionKind_LocationData         = 0x0021,
RDI_SectionKind_ConstantValueData    = 0x0022,
RDI_SectionKind_ConstantValueTable   = 0x0023,
RDI_SectionKind_MD5Checksums         = 0x0024,
RDI_SectionKind_SHA1Checksums        = 0x0025,
RDI_SectionKind_SHA256Checksums      = 0x0026,
RDI_SectionKind_Timestamps           = 0x0027,
RDI_SectionKind_NameMaps             = 0x0028,
RDI_SectionKind_NameMapBuckets       = 0x0029,
RDI_SectionKind_NameMapNodes         = 0x002A,
RDI_SectionKind_COUNT                = 0x002B,
} RDI_SectionKindEnum;

typedef RDI_U32 RDI_SectionEncoding;
typedef enum RDI_SectionEncodingEnum
{
RDI_SectionEncoding_Unpacked   = 0,
RDI_SectionEncoding_LZB        = 1,
} RDI_SectionEncodingEnum;

typedef RDI_U32 RDI_Arch;
typedef enum RDI_ArchEnum
{
RDI_Arch_NULL       = 0,
RDI_Arch_X86        = 1,
RDI_Arch_X64        = 2,
} RDI_ArchEnum;

typedef RDI_U8 RDI_RegCode;
typedef enum RDI_RegCodeEnum
{
RDI_RegCode_nil,
} RDI_RegCodeEnum;

typedef RDI_U8 RDI_RegCodeX86;
typedef enum RDI_RegCodeX86Enum
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
RDI_RegCodeX86_tr         = 61,
RDI_RegCodeX86_ldtr       = 62,
} RDI_RegCodeX86Enum;

typedef RDI_U8 RDI_RegCodeX64;
typedef enum RDI_RegCodeX64Enum
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
RDI_RegCodeX64_zmm0       = 49,
RDI_RegCodeX64_zmm1       = 50,
RDI_RegCodeX64_zmm2       = 51,
RDI_RegCodeX64_zmm3       = 52,
RDI_RegCodeX64_zmm4       = 53,
RDI_RegCodeX64_zmm5       = 54,
RDI_RegCodeX64_zmm6       = 55,
RDI_RegCodeX64_zmm7       = 56,
RDI_RegCodeX64_zmm8       = 57,
RDI_RegCodeX64_zmm9       = 58,
RDI_RegCodeX64_zmm10      = 59,
RDI_RegCodeX64_zmm11      = 60,
RDI_RegCodeX64_zmm12      = 61,
RDI_RegCodeX64_zmm13      = 62,
RDI_RegCodeX64_zmm14      = 63,
RDI_RegCodeX64_zmm15      = 64,
RDI_RegCodeX64_zmm16      = 65,
RDI_RegCodeX64_zmm17      = 66,
RDI_RegCodeX64_zmm18      = 67,
RDI_RegCodeX64_zmm19      = 68,
RDI_RegCodeX64_zmm20      = 69,
RDI_RegCodeX64_zmm21      = 70,
RDI_RegCodeX64_zmm22      = 71,
RDI_RegCodeX64_zmm23      = 72,
RDI_RegCodeX64_zmm24      = 73,
RDI_RegCodeX64_zmm25      = 74,
RDI_RegCodeX64_zmm26      = 75,
RDI_RegCodeX64_zmm27      = 76,
RDI_RegCodeX64_zmm28      = 77,
RDI_RegCodeX64_zmm29      = 78,
RDI_RegCodeX64_zmm30      = 79,
RDI_RegCodeX64_zmm31      = 80,
RDI_RegCodeX64_k0         = 81,
RDI_RegCodeX64_k1         = 82,
RDI_RegCodeX64_k2         = 83,
RDI_RegCodeX64_k3         = 84,
RDI_RegCodeX64_k4         = 85,
RDI_RegCodeX64_k5         = 86,
RDI_RegCodeX64_k6         = 87,
RDI_RegCodeX64_k7         = 88,
RDI_RegCodeX64_mxcsr      = 89,
RDI_RegCodeX64_fsbase     = 90,
RDI_RegCodeX64_gsbase     = 91,
RDI_RegCodeX64_fcw        = 92,
RDI_RegCodeX64_fsw        = 93,
RDI_RegCodeX64_ftw        = 94,
RDI_RegCodeX64_fop        = 95,
RDI_RegCodeX64_fcs        = 96,
RDI_RegCodeX64_fds        = 97,
RDI_RegCodeX64_fip        = 98,
RDI_RegCodeX64_fdp        = 99,
RDI_RegCodeX64_mxcsr_mask = 100,
RDI_RegCodeX64_cetmsr     = 101,
RDI_RegCodeX64_cetssp     = 102,
RDI_RegCodeX64_tr         = 103,
RDI_RegCodeX64_ldtr       = 104,
} RDI_RegCodeX64Enum;

typedef RDI_U32 RDI_BinarySectionFlags;
typedef enum RDI_BinarySectionFlagsEnum
{
RDI_BinarySectionFlag_Read       = 1<<0,
RDI_BinarySectionFlag_Write      = 1<<1,
RDI_BinarySectionFlag_Execute    = 1<<2,
} RDI_BinarySectionFlagsEnum;

typedef RDI_U32 RDI_ChecksumKind;
typedef enum RDI_ChecksumKindEnum
{
RDI_ChecksumKind_NULL       = 0,
RDI_ChecksumKind_MD5        = 1,
RDI_ChecksumKind_SHA1       = 2,
RDI_ChecksumKind_SHA256     = 3,
RDI_ChecksumKind_Timestamp  = 4,
RDI_ChecksumKind_COUNT      = 5,
} RDI_ChecksumKindEnum;

typedef RDI_U32 RDI_Language;
typedef enum RDI_LanguageEnum
{
RDI_Language_NULL       = 0,
RDI_Language_C          = 1,
RDI_Language_CPlusPlus  = 2,
RDI_Language_Masm       = 3,
RDI_Language_COUNT      = 4,
} RDI_LanguageEnum;

typedef RDI_U16 RDI_TypeKind;
typedef enum RDI_TypeKindEnum
{
RDI_TypeKind_NULL                 = 0x0000,
RDI_TypeKind_Void                 = 0x0001,
RDI_TypeKind_Handle               = 0x0002,
RDI_TypeKind_HResult              = 0x0003,
RDI_TypeKind_Char8                = 0x0004,
RDI_TypeKind_Char16               = 0x0005,
RDI_TypeKind_Char32               = 0x0006,
RDI_TypeKind_UChar8               = 0x0007,
RDI_TypeKind_UChar16              = 0x0008,
RDI_TypeKind_UChar32              = 0x0009,
RDI_TypeKind_U8                   = 0x000A,
RDI_TypeKind_U16                  = 0x000B,
RDI_TypeKind_U32                  = 0x000C,
RDI_TypeKind_U64                  = 0x000D,
RDI_TypeKind_U128                 = 0x000E,
RDI_TypeKind_U256                 = 0x000F,
RDI_TypeKind_U512                 = 0x0010,
RDI_TypeKind_S8                   = 0x0011,
RDI_TypeKind_S16                  = 0x0012,
RDI_TypeKind_S32                  = 0x0013,
RDI_TypeKind_S64                  = 0x0014,
RDI_TypeKind_S128                 = 0x0015,
RDI_TypeKind_S256                 = 0x0016,
RDI_TypeKind_S512                 = 0x0017,
RDI_TypeKind_Bool                 = 0x0018,
RDI_TypeKind_F16                  = 0x0019,
RDI_TypeKind_F32                  = 0x001A,
RDI_TypeKind_F32PP                = 0x001B,
RDI_TypeKind_F48                  = 0x001C,
RDI_TypeKind_F64                  = 0x001D,
RDI_TypeKind_F80                  = 0x001E,
RDI_TypeKind_F128                 = 0x001F,
RDI_TypeKind_ComplexF32           = 0x0020,
RDI_TypeKind_ComplexF64           = 0x0021,
RDI_TypeKind_ComplexF80           = 0x0022,
RDI_TypeKind_ComplexF128          = 0x0023,
RDI_TypeKind_Modifier             = 0x1000,
RDI_TypeKind_Ptr                  = 0x1001,
RDI_TypeKind_LRef                 = 0x1002,
RDI_TypeKind_RRef                 = 0x1003,
RDI_TypeKind_Array                = 0x1004,
RDI_TypeKind_Function             = 0x1005,
RDI_TypeKind_Method               = 0x1006,
RDI_TypeKind_MemberPtr            = 0x1007,
RDI_TypeKind_Struct               = 0x2000,
RDI_TypeKind_Class                = 0x2001,
RDI_TypeKind_Union                = 0x2002,
RDI_TypeKind_Enum                 = 0x2003,
RDI_TypeKind_Alias                = 0x2004,
RDI_TypeKind_IncompleteStruct     = 0x2005,
RDI_TypeKind_IncompleteUnion      = 0x2006,
RDI_TypeKind_IncompleteClass      = 0x2007,
RDI_TypeKind_IncompleteEnum       = 0x2008,
RDI_TypeKind_Bitfield             = 0xF000,
RDI_TypeKind_Variadic             = 0xF001,
RDI_TypeKind_Count                = 0xF002,
RDI_TypeKind_FirstBuiltIn         = RDI_TypeKind_Void,
RDI_TypeKind_LastBuiltIn          = RDI_TypeKind_ComplexF128,
RDI_TypeKind_FirstConstructed     = RDI_TypeKind_Modifier,
RDI_TypeKind_LastConstructed      = RDI_TypeKind_MemberPtr,
RDI_TypeKind_FirstUserDefined     = RDI_TypeKind_Struct,
RDI_TypeKind_LastRecord           = RDI_TypeKind_Union,
RDI_TypeKind_FirstIncomplete      = RDI_TypeKind_IncompleteStruct,
RDI_TypeKind_LastIncomplete       = RDI_TypeKind_IncompleteEnum,
RDI_TypeKind_FirstRecord          = RDI_TypeKind_Struct,
RDI_TypeKind_LastUserDefined      = RDI_TypeKind_IncompleteEnum,
} RDI_TypeKindEnum;

typedef RDI_U16 RDI_TypeModifierFlags;
typedef enum RDI_TypeModifierFlagsEnum
{
RDI_TypeModifierFlag_Const                = 1<<0,
RDI_TypeModifierFlag_Volatile             = 1<<1,
RDI_TypeModifierFlag_Restrict             = 1<<2,
} RDI_TypeModifierFlagsEnum;

typedef RDI_U32 RDI_UDTFlags;
typedef enum RDI_UDTFlagsEnum
{
RDI_UDTFlag_EnumMembers          = 1<<0,
} RDI_UDTFlagsEnum;

typedef RDI_U16 RDI_MemberKind;
typedef enum RDI_MemberKindEnum
{
RDI_MemberKind_NULL                      = 0x0000,
RDI_MemberKind_DataField                 = 0x0001,
RDI_MemberKind_StaticData                = 0x0002,
RDI_MemberKind_Method                    = 0x0100,
RDI_MemberKind_StaticMethod              = 0x0101,
RDI_MemberKind_VirtualMethod             = 0x0102,
RDI_MemberKind_VTablePtr                 = 0x0200,
RDI_MemberKind_Base                      = 0x0201,
RDI_MemberKind_VirtualBase               = 0x0202,
RDI_MemberKind_NestedType                = 0x0300,
} RDI_MemberKindEnum;

typedef RDI_U32 RDI_LinkFlags;
typedef enum RDI_LinkFlagsEnum
{
RDI_LinkFlag_External             = 1<<0,
RDI_LinkFlag_TypeScoped           = 1<<1,
RDI_LinkFlag_ProcScoped           = 1<<2,
} RDI_LinkFlagsEnum;

typedef RDI_U32 RDI_LocalKind;
typedef enum RDI_LocalKindEnum
{
RDI_LocalKind_NULL                 = 0x0,
RDI_LocalKind_Parameter            = 0x1,
RDI_LocalKind_Variable             = 0x2,
} RDI_LocalKindEnum;

typedef RDI_U8 RDI_LocationKind;
typedef enum RDI_LocationKindEnum
{
RDI_LocationKind_NULL                 = 0x0,
RDI_LocationKind_AddrBytecodeStream   = 0x1,
RDI_LocationKind_ValBytecodeStream    = 0x2,
RDI_LocationKind_AddrRegPlusU16       = 0x3,
RDI_LocationKind_AddrAddrRegPlusU16   = 0x4,
RDI_LocationKind_ValReg               = 0x5,
} RDI_LocationKindEnum;

typedef RDI_U8 RDI_EvalOp;
typedef enum RDI_EvalOpEnum
{
RDI_EvalOp_Stop                 = 0,
RDI_EvalOp_Noop                 = 1,
RDI_EvalOp_Cond                 = 2,
RDI_EvalOp_Skip                 = 3,
RDI_EvalOp_MemRead              = 4,
RDI_EvalOp_RegRead              = 5,
RDI_EvalOp_RegReadDyn           = 6,
RDI_EvalOp_FrameOff             = 7,
RDI_EvalOp_ModuleOff            = 8,
RDI_EvalOp_TLSOff               = 9,
RDI_EvalOp_ObjectOff            = 10,
RDI_EvalOp_CFA                  = 11,
RDI_EvalOp_ConstU8              = 12,
RDI_EvalOp_ConstU16             = 13,
RDI_EvalOp_ConstU32             = 14,
RDI_EvalOp_ConstU64             = 15,
RDI_EvalOp_ConstU128            = 16,
RDI_EvalOp_ConstString          = 17,
RDI_EvalOp_Abs                  = 18,
RDI_EvalOp_Neg                  = 19,
RDI_EvalOp_Add                  = 20,
RDI_EvalOp_Sub                  = 21,
RDI_EvalOp_Mul                  = 22,
RDI_EvalOp_Div                  = 23,
RDI_EvalOp_Mod                  = 24,
RDI_EvalOp_LShift               = 25,
RDI_EvalOp_RShift               = 26,
RDI_EvalOp_BitAnd               = 27,
RDI_EvalOp_BitOr                = 28,
RDI_EvalOp_BitXor               = 29,
RDI_EvalOp_BitNot               = 30,
RDI_EvalOp_LogAnd               = 31,
RDI_EvalOp_LogOr                = 32,
RDI_EvalOp_LogNot               = 33,
RDI_EvalOp_EqEq                 = 34,
RDI_EvalOp_NtEq                 = 35,
RDI_EvalOp_LsEq                 = 36,
RDI_EvalOp_GrEq                 = 37,
RDI_EvalOp_Less                 = 38,
RDI_EvalOp_Grtr                 = 39,
RDI_EvalOp_Trunc                = 40,
RDI_EvalOp_TruncSigned          = 41,
RDI_EvalOp_Convert              = 42,
RDI_EvalOp_Pick                 = 43,
RDI_EvalOp_Pop                  = 44,
RDI_EvalOp_Insert               = 45,
RDI_EvalOp_ValueRead            = 46,
RDI_EvalOp_ByteSwap             = 47,
RDI_EvalOp_CallSiteValue        = 48,
RDI_EvalOp_PartialValue         = 49,
RDI_EvalOp_PartialValueBit      = 50,
RDI_EvalOp_Swap                 = 51,
RDI_EvalOp_COUNT                = 52,
} RDI_EvalOpEnum;

typedef RDI_U8 RDI_EvalTypeGroup;
typedef enum RDI_EvalTypeGroupEnum
{
RDI_EvalTypeGroup_Other                = 0,
RDI_EvalTypeGroup_U                    = 1,
RDI_EvalTypeGroup_S                    = 2,
RDI_EvalTypeGroup_F32                  = 3,
RDI_EvalTypeGroup_F64                  = 4,
RDI_EvalTypeGroup_COUNT                = 5,
} RDI_EvalTypeGroupEnum;

typedef RDI_U8 RDI_EvalConversionKind;
typedef enum RDI_EvalConversionKindEnum
{
RDI_EvalConversionKind_Noop                 = 0,
RDI_EvalConversionKind_Legal                = 1,
RDI_EvalConversionKind_OtherToOther         = 2,
RDI_EvalConversionKind_ToOther              = 3,
RDI_EvalConversionKind_FromOther            = 4,
RDI_EvalConversionKind_COUNT                = 5,
} RDI_EvalConversionKindEnum;

typedef RDI_U32 RDI_NameMapKind;
typedef enum RDI_NameMapKindEnum
{
RDI_NameMapKind_NULL                 = 0,
RDI_NameMapKind_GlobalVariables      = 1,
RDI_NameMapKind_ThreadVariables      = 2,
RDI_NameMapKind_Constants            = 3,
RDI_NameMapKind_Procedures           = 4,
RDI_NameMapKind_Types                = 5,
RDI_NameMapKind_LinkNameProcedures   = 6,
RDI_NameMapKind_NormalSourcePaths    = 7,
RDI_NameMapKind_COUNT                = 8,
} RDI_NameMapKindEnum;

#define RDI_Header_XList \
X(RDI_U64, magic)\
X(RDI_U32, encoding_version)\
X(RDI_U32, data_section_off)\
X(RDI_U32, data_section_count)\

#define RDI_SectionKind_XList \
X(NULL, null, RDI_U8)\
X(TopLevelInfo, top_level_info, RDI_TopLevelInfo)\
X(StringData, string_data, RDI_U8)\
X(StringTable, string_table, RDI_U32)\
X(IndexRuns, index_runs, RDI_U32)\
X(BinarySections, binary_sections, RDI_BinarySection)\
X(FilePathNodes, file_path_nodes, RDI_FilePathNode)\
X(SourceFiles, source_files, RDI_SourceFile)\
X(LineTables, line_tables, RDI_LineTable)\
X(LineInfoVOffs, line_info_voffs, RDI_U64)\
X(LineInfoLines, line_info_lines, RDI_Line)\
X(LineInfoColumns, line_info_columns, RDI_Column)\
X(SourceLineMaps, source_line_maps, RDI_SourceLineMap)\
X(SourceLineMapNumbers, source_line_map_numbers, RDI_U32)\
X(SourceLineMapRanges, source_line_map_ranges, RDI_U32)\
X(SourceLineMapVOffs, source_line_map_voffs, RDI_U64)\
X(Units, units, RDI_Unit)\
X(UnitVMap, unit_vmap, RDI_VMapEntry)\
X(TypeNodes, type_nodes, RDI_TypeNode)\
X(UDTs, udts, RDI_UDT)\
X(Members, members, RDI_Member)\
X(EnumMembers, enum_members, RDI_EnumMember)\
X(GlobalVariables, global_variables, RDI_GlobalVariable)\
X(GlobalVMap, global_vmap, RDI_VMapEntry)\
X(ThreadVariables, thread_variables, RDI_ThreadVariable)\
X(Constants, constants, RDI_Constant)\
X(Procedures, procedures, RDI_Procedure)\
X(Scopes, scopes, RDI_Scope)\
X(ScopeVOffData, scope_voff_data, RDI_U64)\
X(ScopeVMap, scope_vmap, RDI_VMapEntry)\
X(InlineSites, inline_sites, RDI_InlineSite)\
X(Locals, locals, RDI_Local)\
X(LocationBlocks, location_blocks, RDI_LocationBlock)\
X(LocationData, location_data, RDI_U8)\
X(ConstantValueData, constant_value_data, RDI_U8)\
X(ConstantValueTable, constant_value_table, RDI_U32)\
X(MD5Checksums, md5_checksums, RDI_MD5)\
X(SHA1Checksums, sha1_checksums, RDI_SHA1)\
X(SHA256Checksums, sha256_checksums, RDI_SHA256)\
X(Timestamps, timestamps, RDI_U64)\
X(NameMaps, name_maps, RDI_NameMap)\
X(NameMapBuckets, name_map_buckets, RDI_NameMapBucket)\
X(NameMapNodes, name_map_nodes, RDI_NameMapNode)\

#define RDI_SectionEncoding_XList \
X(Unpacked)\
X(LZB)\

#define RDI_Section_XList \
X(RDI_SectionEncoding, encoding)\
X(RDI_U32, pad)\
X(RDI_U64, off)\
X(RDI_U64, encoded_size)\
X(RDI_U64, unpacked_size)\

#define RDI_VMapEntry_XList \
X(RDI_U64, voff)\
X(RDI_U64, idx)\

#define RDI_Arch_XList \
X(NULL)\
X(X86)\
X(X64)\

#define RDI_RegCodeX86_XList \
X(nil, 0)\
X(eax, 1)\
X(ecx, 2)\
X(edx, 3)\
X(ebx, 4)\
X(esp, 5)\
X(ebp, 6)\
X(esi, 7)\
X(edi, 8)\
X(fsbase, 9)\
X(gsbase, 10)\
X(eflags, 11)\
X(eip, 12)\
X(dr0, 13)\
X(dr1, 14)\
X(dr2, 15)\
X(dr3, 16)\
X(dr4, 17)\
X(dr5, 18)\
X(dr6, 19)\
X(dr7, 20)\
X(fpr0, 21)\
X(fpr1, 22)\
X(fpr2, 23)\
X(fpr3, 24)\
X(fpr4, 25)\
X(fpr5, 26)\
X(fpr6, 27)\
X(fpr7, 28)\
X(st0, 29)\
X(st1, 30)\
X(st2, 31)\
X(st3, 32)\
X(st4, 33)\
X(st5, 34)\
X(st6, 35)\
X(st7, 36)\
X(fcw, 37)\
X(fsw, 38)\
X(ftw, 39)\
X(fop, 40)\
X(fcs, 41)\
X(fds, 42)\
X(fip, 43)\
X(fdp, 44)\
X(mxcsr, 45)\
X(mxcsr_mask, 46)\
X(ss, 47)\
X(cs, 48)\
X(ds, 49)\
X(es, 50)\
X(fs, 51)\
X(gs, 52)\
X(ymm0, 53)\
X(ymm1, 54)\
X(ymm2, 55)\
X(ymm3, 56)\
X(ymm4, 57)\
X(ymm5, 58)\
X(ymm6, 59)\
X(ymm7, 60)\
X(tr, 61)\
X(ldtr, 62)\

#define RDI_RegCodeX64_XList \
X(nil, 0)\
X(rax, 1)\
X(rcx, 2)\
X(rdx, 3)\
X(rbx, 4)\
X(rsp, 5)\
X(rbp, 6)\
X(rsi, 7)\
X(rdi, 8)\
X(r8, 9)\
X(r9, 10)\
X(r10, 11)\
X(r11, 12)\
X(r12, 13)\
X(r13, 14)\
X(r14, 15)\
X(r15, 16)\
X(es, 17)\
X(cs, 18)\
X(ss, 19)\
X(ds, 20)\
X(fs, 21)\
X(gs, 22)\
X(rip, 23)\
X(rflags, 24)\
X(dr0, 25)\
X(dr1, 26)\
X(dr2, 27)\
X(dr3, 28)\
X(dr4, 29)\
X(dr5, 30)\
X(dr6, 31)\
X(dr7, 32)\
X(st0, 33)\
X(st1, 34)\
X(st2, 35)\
X(st3, 36)\
X(st4, 37)\
X(st5, 38)\
X(st6, 39)\
X(st7, 40)\
X(fpr0, 41)\
X(fpr1, 42)\
X(fpr2, 43)\
X(fpr3, 44)\
X(fpr4, 45)\
X(fpr5, 46)\
X(fpr6, 47)\
X(fpr7, 48)\
X(zmm0, 49)\
X(zmm1, 50)\
X(zmm2, 51)\
X(zmm3, 52)\
X(zmm4, 53)\
X(zmm5, 54)\
X(zmm6, 55)\
X(zmm7, 56)\
X(zmm8, 57)\
X(zmm9, 58)\
X(zmm10, 59)\
X(zmm11, 60)\
X(zmm12, 61)\
X(zmm13, 62)\
X(zmm14, 63)\
X(zmm15, 64)\
X(zmm16, 65)\
X(zmm17, 66)\
X(zmm18, 67)\
X(zmm19, 68)\
X(zmm20, 69)\
X(zmm21, 70)\
X(zmm22, 71)\
X(zmm23, 72)\
X(zmm24, 73)\
X(zmm25, 74)\
X(zmm26, 75)\
X(zmm27, 76)\
X(zmm28, 77)\
X(zmm29, 78)\
X(zmm30, 79)\
X(zmm31, 80)\
X(k0, 81)\
X(k1, 82)\
X(k2, 83)\
X(k3, 84)\
X(k4, 85)\
X(k5, 86)\
X(k6, 87)\
X(k7, 88)\
X(mxcsr, 89)\
X(fsbase, 90)\
X(gsbase, 91)\
X(fcw, 92)\
X(fsw, 93)\
X(ftw, 94)\
X(fop, 95)\
X(fcs, 96)\
X(fds, 97)\
X(fip, 98)\
X(fdp, 99)\
X(mxcsr_mask, 100)\
X(cetmsr, 101)\
X(cetssp, 102)\
X(tr, 103)\
X(ldtr, 104)\

#define RDI_TopLevelInfo_XList \
X(RDI_Arch, arch)\
X(RDI_U32, exe_name_string_idx)\
X(RDI_U64, exe_hash)\
X(RDI_U64, voff_max)\
X(RDI_GUID, guid)\
X(RDI_U32, producer_name_string_idx)\

#define RDI_BinarySectionFlags_XList \
X(Read)\
X(Write)\
X(Execute)\

#define RDI_BinarySection_XList \
X(RDI_U32, name_string_idx)\
X(RDI_BinarySectionFlags, flags)\
X(RDI_U64, voff_first)\
X(RDI_U64, voff_opl)\
X(RDI_U64, foff_first)\
X(RDI_U64, foff_opl)\

#define RDI_ChecksumKind_XList \
X(NULL, NULL)\
X(MD5, MD5Checksums)\
X(SHA1, SHA1Checksums)\
X(SHA256, SHA256Checksums)\
X(Timestamp, Timestamps)\
X(COUNT, NULL)\

#define RDI_FilePathNode_XList \
X(RDI_U32, name_string_idx)\
X(RDI_U32, parent_path_node)\
X(RDI_U32, first_child)\
X(RDI_U32, next_sibling)\
X(RDI_U32, source_file_idx)\

#define RDI_SourceFile_XList \
X(RDI_U32, file_path_node_idx)\
X(RDI_U32, normal_full_path_string_idx)\
X(RDI_U32, source_line_map_idx)\
X(RDI_ChecksumKind, checksum_kind)\
X(RDI_U32, checksum_idx)\

#define RDI_Unit_XList \
X(RDI_U32, unit_name_string_idx)\
X(RDI_U32, compiler_name_string_idx)\
X(RDI_U32, source_file_path_node)\
X(RDI_U32, object_file_path_node)\
X(RDI_U32, archive_file_path_node)\
X(RDI_U32, build_path_node)\
X(RDI_Language, language)\
X(RDI_U32, line_table_idx)\

#define RDI_LineTable_XList \
X(RDI_U32, voffs_base_idx)\
X(RDI_U32, lines_base_idx)\
X(RDI_U32, cols_base_idx)\
X(RDI_U32, lines_count)\
X(RDI_U32, cols_count)\

#define RDI_Line_XList \
X(RDI_U32, file_idx)\
X(RDI_U32, line_num)\

#define RDI_Column_XList \
X(RDI_U16, col_first)\
X(RDI_U16, col_opl)\

#define RDI_SourceLineMapMemberTable \
X(RDI_U32, line_count)\
X(RDI_U32, voff_count)\
X(RDI_U32, line_map_nums_base_idx)\
X(RDI_U32, line_map_range_base_idx)\
X(RDI_U32, line_map_voff_base_idx)\

#define RDI_Language_XList \
X(NULL)\
X(C)\
X(CPlusPlus)\
X(Masm)\
X(COUNT)\

#define RDI_TypeKind_XList \
X(NULL)\
X(Void)\
X(Handle)\
X(HResult)\
X(Char8)\
X(Char16)\
X(Char32)\
X(UChar8)\
X(UChar16)\
X(UChar32)\
X(U8)\
X(U16)\
X(U32)\
X(U64)\
X(U128)\
X(U256)\
X(U512)\
X(S8)\
X(S16)\
X(S32)\
X(S64)\
X(S128)\
X(S256)\
X(S512)\
X(Bool)\
X(F16)\
X(F32)\
X(F32PP)\
X(F48)\
X(F64)\
X(F80)\
X(F128)\
X(ComplexF32)\
X(ComplexF64)\
X(ComplexF80)\
X(ComplexF128)\
X(Modifier)\
X(Ptr)\
X(LRef)\
X(RRef)\
X(Array)\
X(Function)\
X(Method)\
X(MemberPtr)\
X(Struct)\
X(Class)\
X(Union)\
X(Enum)\
X(Alias)\
X(IncompleteStruct)\
X(IncompleteUnion)\
X(IncompleteClass)\
X(IncompleteEnum)\
X(Bitfield)\
X(Variadic)\
X(Count)\

#define RDI_TypeModifierFlags_XList \
X(Const)\
X(Volatile)\
X(Restrict)\

#define RDI_TypeNode_XList \
X(RDI_TypeKind, kind)\
X(RDI_U16, flags)\
X(RDI_U32, byte_size)\

#define RDI_UDTFlags_XList \
X(EnumMembers)\

#define RDI_UDT_XList \
X(RDI_U32, self_type_idx)\
X(RDI_UDTFlags, flags)\
X(RDI_U32, member_first)\
X(RDI_U32, member_count)\
X(RDI_U32, file_idx)\
X(RDI_U32, line)\
X(RDI_U32, col)\

#define RDI_MemberKind_XList \
X(NULL)\
X(DataField)\
X(StaticData)\
X(Method)\
X(StaticMethod)\
X(VirtualMethod)\
X(VTablePtr)\
X(Base)\
X(VirtualBase)\
X(NestedType)\

#define RDI_Member_XList \
X(RDI_MemberKind, kind)\
X(RDI_U16, pad)\
X(RDI_U32, name_string_idx)\
X(RDI_U32, type_idx)\
X(RDI_U32, off)\

#define RDI_EnumMember_XList \
X(RDI_U32, name_string_idx)\
X(RDI_U32, pad)\
X(RDI_U64, val)\

#define RDI_LinkFlags_XList \
X(External)\
X(TypeScoped)\
X(ProcScoped)\

#define RDI_LocalKind_XList \
X(NULL)\
X(Parameter)\
X(Variable)\

#define RDI_LocationKind_XList \
X(NULL)\
X(AddrBytecodeStream)\
X(ValBytecodeStream)\
X(AddrRegPlusU16)\
X(AddrAddrRegPlusU16)\
X(ValReg)\

#define RDI_GlobalVariable_XList \
X(RDI_U32, name_string_idx)\
X(RDI_LinkFlags, link_flags)\
X(RDI_U64, voff)\
X(RDI_U32, type_idx)\
X(RDI_U32, container_idx)\

#define RDI_ThreadVariable_XList \
X(type, name_string_idx)\
X(type, link_flags)\
X(type, tls_off)\
X(type, type_idx)\
X(type, container_idx)\

#define RDI_Procedure_XList \
X(RDI_U32, name_string_idx)\
X(RDI_U32, link_name_string_idx)\
X(RDI_LinkFlags, link_flags)\
X(RDI_U32, type_idx)\
X(RDI_U32, root_scope_idx)\
X(RDI_U32, container_idx)\
X(RDI_U32, frame_base_location_first)\
X(RDI_U32, frame_base_location_opl)\

#define RDI_Scope_XList \
X(RDI_U32, proc_idx)\
X(RDI_U32, parent_scope_idx)\
X(RDI_U32, first_child_scope_idx)\
X(RDI_U32, next_sibling_scope_idx)\
X(RDI_U32, voff_range_first)\
X(RDI_U32, voff_range_opl)\
X(RDI_U32, local_first)\
X(RDI_U32, local_count)\
X(RDI_U32, inline_site_idx)\

#define RDI_InlineSite_XList \
X(RDI_U32, name_string_idx)\
X(RDI_U32, type_idx)\
X(RDI_U32, owner_type_idx)\
X(RDI_U32, line_table_idx)\

#define RDI_Local_XList \
X(RDI_LocalKind, kind)\
X(RDI_U32, name_string_idx)\
X(RDI_U32, type_idx)\
X(RDI_U32, pad)\
X(RDI_U32, location_first)\
X(RDI_U32, location_opl)\

#define RDI_LocationBlock_XList \
X(RDI_U32, scope_off_first)\
X(RDI_U32, scope_off_opl)\
X(RDI_U32, location_data_off)\

#define RDI_LocationBytecodeStream_XList \
X(RDI_LocationKind, kind)\

#define RDI_LocationRegPlusU16_XList \
X(RDI_LocationKind, kind)\
X(RDI_RegCode, reg_code)\
X(RDI_U16, offset)\

#define RDI_LocationReg_XList \
X(RDI_LocationKind, kind)\
X(RDI_RegCode, reg_code)\

#define RDI_EvalOp_XList \
X(Stop)\
X(Noop)\
X(Cond)\
X(Skip)\
X(MemRead)\
X(RegRead)\
X(RegReadDyn)\
X(FrameOff)\
X(ModuleOff)\
X(TLSOff)\
X(ObjectOff)\
X(CFA)\
X(ConstU8)\
X(ConstU16)\
X(ConstU32)\
X(ConstU64)\
X(ConstU128)\
X(ConstString)\
X(Abs)\
X(Neg)\
X(Add)\
X(Sub)\
X(Mul)\
X(Div)\
X(Mod)\
X(LShift)\
X(RShift)\
X(BitAnd)\
X(BitOr)\
X(BitXor)\
X(BitNot)\
X(LogAnd)\
X(LogOr)\
X(LogNot)\
X(EqEq)\
X(NtEq)\
X(LsEq)\
X(GrEq)\
X(Less)\
X(Grtr)\
X(Trunc)\
X(TruncSigned)\
X(Convert)\
X(Pick)\
X(Pop)\
X(Insert)\
X(ValueRead)\
X(ByteSwap)\
X(CallSiteValue)\
X(PartialValue)\
X(PartialValueBit)\
X(Swap)\

#define RDI_EvalTypeGroup_XList \
X(Other)\
X(U)\
X(S)\
X(F32)\
X(F64)\

#define RDI_EvalConversionKind_XList \
X(Noop)\
X(Legal)\
X(OtherToOther)\
X(ToOther)\
X(FromOther)\

#define RDI_NameMapKind_XList \
X(NULL)\
X(GlobalVariables)\
X(ThreadVariables)\
X(Constants)\
X(Procedures)\
X(Types)\
X(LinkNameProcedures)\
X(NormalSourcePaths)\

#define RDI_NameMap_XList \
X(RDI_U32, bucket_base_idx)\
X(RDI_U32, node_base_idx)\
X(RDI_U32, bucket_count)\
X(RDI_U32, node_count)\

#define RDI_NameMapBucket_XList \
X(RDI_U32, first_node)\
X(RDI_U32, node_count)\

#define RDI_NameMapNode_XList \
X(RDI_U32, string_idx)\
X(RDI_U32, match_count)\
X(RDI_U32, match_idx_or_idx_run_first)\

#if !RDI_DISABLE_TABLE_INDEX_TYPECHECKING
typedef struct RDI_U32_StringTable                 { RDI_U32 v; } RDI_U32_StringTable;
typedef struct RDI_U32_IndexRuns                   { RDI_U32 v; } RDI_U32_IndexRuns;
typedef struct RDI_U32_BinarySections              { RDI_U32 v; } RDI_U32_BinarySections;
typedef struct RDI_U32_FilePathNodes               { RDI_U32 v; } RDI_U32_FilePathNodes;
typedef struct RDI_U32_SourceFiles                 { RDI_U32 v; } RDI_U32_SourceFiles;
typedef struct RDI_U32_LineTables                  { RDI_U32 v; } RDI_U32_LineTables;
typedef struct RDI_U32_LineInfoVOffs               { RDI_U32 v; } RDI_U32_LineInfoVOffs;
typedef struct RDI_U32_LineInfoLines               { RDI_U32 v; } RDI_U32_LineInfoLines;
typedef struct RDI_U32_LineInfoColumns             { RDI_U32 v; } RDI_U32_LineInfoColumns;
typedef struct RDI_U32_SourceLineMaps              { RDI_U32 v; } RDI_U32_SourceLineMaps;
typedef struct RDI_U32_SourceLineMapNumbers        { RDI_U32 v; } RDI_U32_SourceLineMapNumbers;
typedef struct RDI_U32_SourceLineMapRanges         { RDI_U32 v; } RDI_U32_SourceLineMapRanges;
typedef struct RDI_U32_SourceLineMapVOffs          { RDI_U32 v; } RDI_U32_SourceLineMapVOffs;
typedef struct RDI_U32_Units                       { RDI_U32 v; } RDI_U32_Units;
typedef struct RDI_U32_TypeNodes                   { RDI_U32 v; } RDI_U32_TypeNodes;
typedef struct RDI_U32_UDTs                        { RDI_U32 v; } RDI_U32_UDTs;
typedef struct RDI_U32_Members                     { RDI_U32 v; } RDI_U32_Members;
typedef struct RDI_U32_EnumMembers                 { RDI_U32 v; } RDI_U32_EnumMembers;
typedef struct RDI_U32_GlobalVariables             { RDI_U32 v; } RDI_U32_GlobalVariables;
typedef struct RDI_U32_ThreadVariables             { RDI_U32 v; } RDI_U32_ThreadVariables;
typedef struct RDI_U32_Constants                   { RDI_U32 v; } RDI_U32_Constants;
typedef struct RDI_U32_Procedures                  { RDI_U32 v; } RDI_U32_Procedures;
typedef struct RDI_U32_Scopes                      { RDI_U32 v; } RDI_U32_Scopes;
typedef struct RDI_U32_ScopeVOffData               { RDI_U32 v; } RDI_U32_ScopeVOffData;
typedef struct RDI_U32_InlineSites                 { RDI_U32 v; } RDI_U32_InlineSites;
typedef struct RDI_U32_Locals                      { RDI_U32 v; } RDI_U32_Locals;
typedef struct RDI_U32_LocationBlocks              { RDI_U32 v; } RDI_U32_LocationBlocks;
typedef struct RDI_U32_LocationData                { RDI_U32 v; } RDI_U32_LocationData;
typedef struct RDI_U32_ConstantValueData           { RDI_U32 v; } RDI_U32_ConstantValueData;
typedef struct RDI_U32_ConstantValueTable          { RDI_U32 v; } RDI_U32_ConstantValueTable;
typedef struct RDI_U32_MD5Checksums                { RDI_U32 v; } RDI_U32_MD5Checksums;
typedef struct RDI_U32_SHA1Checksums               { RDI_U32 v; } RDI_U32_SHA1Checksums;
typedef struct RDI_U32_SHA256Checksums             { RDI_U32 v; } RDI_U32_SHA256Checksums;
typedef struct RDI_U32_Timestamps                  { RDI_U32 v; } RDI_U32_Timestamps;
typedef struct RDI_U32_NameMaps                    { RDI_U32 v; } RDI_U32_NameMaps;
typedef struct RDI_U32_NameMapBuckets              { RDI_U32 v; } RDI_U32_NameMapBuckets;
typedef struct RDI_U32_NameMapNodes                { RDI_U32 v; } RDI_U32_NameMapNodes;
#else
typedef struct RDI_U32_Table { RDI_U32 v; } RDI_U32_Table;
typedef struct RDI_U64_Table { RDI_U64 v; } RDI_U64_Table;
typedef RDI_U32_Table RDI_U32_StringTable;
typedef RDI_U32_Table RDI_U32_IndexRuns;
typedef RDI_U32_Table RDI_U32_BinarySections;
typedef RDI_U32_Table RDI_U32_FilePathNodes;
typedef RDI_U32_Table RDI_U32_SourceFiles;
typedef RDI_U32_Table RDI_U32_LineTables;
typedef RDI_U32_Table RDI_U32_LineInfoVOffs;
typedef RDI_U32_Table RDI_U32_LineInfoLines;
typedef RDI_U32_Table RDI_U32_LineInfoColumns;
typedef RDI_U32_Table RDI_U32_SourceLineMaps;
typedef RDI_U32_Table RDI_U32_SourceLineMapNumbers;
typedef RDI_U32_Table RDI_U32_SourceLineMapRanges;
typedef RDI_U32_Table RDI_U32_SourceLineMapVOffs;
typedef RDI_U32_Table RDI_U32_Units;
typedef RDI_U32_Table RDI_U32_TypeNodes;
typedef RDI_U32_Table RDI_U32_UDTs;
typedef RDI_U32_Table RDI_U32_Members;
typedef RDI_U32_Table RDI_U32_EnumMembers;
typedef RDI_U32_Table RDI_U32_GlobalVariables;
typedef RDI_U32_Table RDI_U32_ThreadVariables;
typedef RDI_U32_Table RDI_U32_Constants;
typedef RDI_U32_Table RDI_U32_Procedures;
typedef RDI_U32_Table RDI_U32_Scopes;
typedef RDI_U32_Table RDI_U32_ScopeVOffData;
typedef RDI_U32_Table RDI_U32_InlineSites;
typedef RDI_U32_Table RDI_U32_Locals;
typedef RDI_U32_Table RDI_U32_LocationBlocks;
typedef RDI_U32_Table RDI_U32_LocationData;
typedef RDI_U32_Table RDI_U32_ConstantValueData;
typedef RDI_U32_Table RDI_U32_ConstantValueTable;
typedef RDI_U32_Table RDI_U32_MD5Checksums;
typedef RDI_U32_Table RDI_U32_SHA1Checksums;
typedef RDI_U32_Table RDI_U32_SHA256Checksums;
typedef RDI_U32_Table RDI_U32_Timestamps;
typedef RDI_U32_Table RDI_U32_NameMaps;
typedef RDI_U32_Table RDI_U32_NameMapBuckets;
typedef RDI_U32_Table RDI_U32_NameMapNodes;
#endif

#define RDI_EVAL_CTRLBITS(decodeN,popN,pushN) (((decodeN) << 8) | ((popN) << 4) | ((pushN) << 0))
#define RDI_DECODEN_FROM_CTRLBITS(ctrlbits)   (((ctrlbits) >> 8) & 0xff)
#define RDI_POPN_FROM_CTRLBITS(ctrlbits)      (((ctrlbits) >> 4) & 0xf)
#define RDI_PUSHN_FROM_CTRLBITS(ctrlbits)     (((ctrlbits) >> 0) & 0xf)
#define RDI_EncodeRegReadParam(reg,bytesize,bytepos) ((reg)|((bytesize)<<8)|((bytepos)<<16))

typedef struct RDI_Header RDI_Header;
struct RDI_Header
{
RDI_U64 magic;
RDI_U32 encoding_version;
RDI_U32 data_section_off;
RDI_U32 data_section_count;
};

typedef struct RDI_Section RDI_Section;
struct RDI_Section
{
RDI_SectionEncoding encoding;
RDI_U32 pad;
RDI_U64 off;
RDI_U64 encoded_size;
RDI_U64 unpacked_size;
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
RDI_GUID guid;
RDI_U32 producer_name_string_idx;
};

typedef struct RDI_BinarySection RDI_BinarySection;
struct RDI_BinarySection
{
RDI_U32 name_string_idx;
RDI_BinarySectionFlags flags;
RDI_U64 voff_first;
RDI_U64 voff_opl;
RDI_U64 foff_first;
RDI_U64 foff_opl;
};

typedef struct RDI_FilePathNode RDI_FilePathNode;
struct RDI_FilePathNode
{
RDI_U32 name_string_idx;
RDI_U32 parent_path_node;
RDI_U32 first_child;
RDI_U32 next_sibling;
RDI_U32 source_file_idx;
};

typedef struct RDI_SourceFile RDI_SourceFile;
struct RDI_SourceFile
{
RDI_U32 file_path_node_idx;
RDI_U32 normal_full_path_string_idx;
RDI_U32 source_line_map_idx;
RDI_ChecksumKind checksum_kind;
RDI_U32 checksum_idx;
};

typedef struct RDI_Unit RDI_Unit;
struct RDI_Unit
{
RDI_U32 unit_name_string_idx;
RDI_U32 compiler_name_string_idx;
RDI_U32 source_file_path_node;
RDI_U32 object_file_path_node;
RDI_U32 archive_file_path_node;
RDI_U32 build_path_node;
RDI_Language language;
RDI_U32 line_table_idx;
};

typedef struct RDI_LineTable RDI_LineTable;
struct RDI_LineTable
{
RDI_U32 voffs_base_idx;
RDI_U32 lines_base_idx;
RDI_U32 cols_base_idx;
RDI_U32 lines_count;
RDI_U32 cols_count;
};

typedef struct RDI_Line RDI_Line;
struct RDI_Line
{
RDI_U32 file_idx;
RDI_U32 line_num;
};

typedef struct RDI_Column RDI_Column;
struct RDI_Column
{
RDI_U16 col_first;
RDI_U16 col_opl;
};

typedef struct RDI_SourceLineMap RDI_SourceLineMap;
struct RDI_SourceLineMap
{
RDI_U32 line_count;
RDI_U32 voff_count;
RDI_U32 line_map_nums_base_idx;
RDI_U32 line_map_range_base_idx;
RDI_U32 line_map_voff_base_idx;
};

typedef struct RDI_TypeNode RDI_TypeNode;
struct RDI_TypeNode
{
RDI_TypeKind kind;
RDI_U16 flags;
RDI_U32 byte_size;

    union
  {
    // kind is 'built-in'
    struct
    {
      RDI_U32 name_string_idx;
    } built_in;
    
    // kind is 'constructed'
    struct
    {
      RDI_U32 direct_type_idx;
      RDI_U32 count;
      union
      {
        // when kind is 'Function' or 'Method'
        RDI_U32 param_idx_run_first;
        // when kind is 'MemberPtr'
        RDI_U32 owner_type_idx;
      };
    }
    constructed;
    
    // kind is 'user defined'
    struct
    {
      RDI_U32 name_string_idx;
      RDI_U32 direct_type_idx;
      RDI_U32 udt_idx;
    }
    user_defined;
    
    // (kind = Bitfield)
    struct
    {
      RDI_U32 direct_type_idx;
      RDI_U32 off;
      RDI_U32 size;
    }
    bitfield;
  }
  ;
};

typedef struct RDI_UDT RDI_UDT;
struct RDI_UDT
{
RDI_U32 self_type_idx;
RDI_UDTFlags flags;
RDI_U32 member_first;
RDI_U32 member_count;
RDI_U32 file_idx;
RDI_U32 line;
RDI_U32 col;
};

typedef struct RDI_Member RDI_Member;
struct RDI_Member
{
RDI_MemberKind kind;
RDI_U16 pad;
RDI_U32 name_string_idx;
RDI_U32 type_idx;
RDI_U32 off;
};

typedef struct RDI_EnumMember RDI_EnumMember;
struct RDI_EnumMember
{
RDI_U32 name_string_idx;
RDI_U32 pad;
RDI_U64 val;
};

typedef struct RDI_GlobalVariable RDI_GlobalVariable;
struct RDI_GlobalVariable
{
RDI_U32 name_string_idx;
RDI_LinkFlags link_flags;
RDI_U64 voff;
RDI_U32 type_idx;
RDI_U32 container_idx;
};

typedef struct RDI_ThreadVariable RDI_ThreadVariable;
struct RDI_ThreadVariable
{
RDI_U32 name_string_idx;
RDI_LinkFlags link_flags;
RDI_U32 tls_off;
RDI_U32 type_idx;
RDI_U32 container_idx;
};

typedef struct RDI_Constant RDI_Constant;
struct RDI_Constant
{
RDI_U32 name_string_idx;
RDI_U32 type_idx;
RDI_U32 constant_value_idx;
};

typedef struct RDI_Procedure RDI_Procedure;
struct RDI_Procedure
{
RDI_U32 name_string_idx;
RDI_U32 link_name_string_idx;
RDI_LinkFlags link_flags;
RDI_U32 type_idx;
RDI_U32 root_scope_idx;
RDI_U32 container_idx;
RDI_U32 frame_base_location_first;
RDI_U32 frame_base_location_opl;
};

typedef struct RDI_Scope RDI_Scope;
struct RDI_Scope
{
RDI_U32 proc_idx;
RDI_U32 parent_scope_idx;
RDI_U32 first_child_scope_idx;
RDI_U32 next_sibling_scope_idx;
RDI_U32 voff_range_first;
RDI_U32 voff_range_opl;
RDI_U32 local_first;
RDI_U32 local_count;
RDI_U32 inline_site_idx;
};

typedef struct RDI_InlineSite RDI_InlineSite;
struct RDI_InlineSite
{
RDI_U32 name_string_idx;
RDI_U32 type_idx;
RDI_U32 owner_type_idx;
RDI_U32 line_table_idx;
};

typedef struct RDI_Local RDI_Local;
struct RDI_Local
{
RDI_LocalKind kind;
RDI_U32 name_string_idx;
RDI_U32 type_idx;
RDI_U32 pad;
RDI_U32 location_first;
RDI_U32 location_opl;
};

typedef struct RDI_LocationBlock RDI_LocationBlock;
struct RDI_LocationBlock
{
RDI_U32 scope_off_first;
RDI_U32 scope_off_opl;
RDI_U32 location_data_off;
};

typedef struct RDI_LocationBytecodeStream RDI_LocationBytecodeStream;
struct RDI_LocationBytecodeStream
{
RDI_LocationKind kind;
};

typedef struct RDI_LocationRegPlusU16 RDI_LocationRegPlusU16;
struct RDI_LocationRegPlusU16
{
RDI_LocationKind kind;
RDI_RegCode reg_code;
RDI_U16 offset;
};

typedef struct RDI_LocationReg RDI_LocationReg;
struct RDI_LocationReg
{
RDI_LocationKind kind;
RDI_RegCode reg_code;
};

typedef struct RDI_NameMap RDI_NameMap;
struct RDI_NameMap
{
RDI_U32 bucket_base_idx;
RDI_U32 node_base_idx;
RDI_U32 bucket_count;
RDI_U32 node_count;
};

typedef struct RDI_NameMapBucket RDI_NameMapBucket;
struct RDI_NameMapBucket
{
RDI_U32 first_node;
RDI_U32 node_count;
};

typedef struct RDI_NameMapNode RDI_NameMapNode;
struct RDI_NameMapNode
{
RDI_U32 string_idx;
RDI_U32 match_count;
RDI_U32 match_idx_or_idx_run_first;
};

typedef RDI_TopLevelInfo                 RDI_SectionElementType_TopLevelInfo;
typedef RDI_U8                           RDI_SectionElementType_StringData;
typedef RDI_U32                          RDI_SectionElementType_StringTable;
typedef RDI_U32                          RDI_SectionElementType_IndexRuns;
typedef RDI_BinarySection                RDI_SectionElementType_BinarySections;
typedef RDI_FilePathNode                 RDI_SectionElementType_FilePathNodes;
typedef RDI_SourceFile                   RDI_SectionElementType_SourceFiles;
typedef RDI_LineTable                    RDI_SectionElementType_LineTables;
typedef RDI_U64                          RDI_SectionElementType_LineInfoVOffs;
typedef RDI_Line                         RDI_SectionElementType_LineInfoLines;
typedef RDI_Column                       RDI_SectionElementType_LineInfoColumns;
typedef RDI_SourceLineMap                RDI_SectionElementType_SourceLineMaps;
typedef RDI_U32                          RDI_SectionElementType_SourceLineMapNumbers;
typedef RDI_U32                          RDI_SectionElementType_SourceLineMapRanges;
typedef RDI_U64                          RDI_SectionElementType_SourceLineMapVOffs;
typedef RDI_Unit                         RDI_SectionElementType_Units;
typedef RDI_VMapEntry                    RDI_SectionElementType_UnitVMap;
typedef RDI_TypeNode                     RDI_SectionElementType_TypeNodes;
typedef RDI_UDT                          RDI_SectionElementType_UDTs;
typedef RDI_Member                       RDI_SectionElementType_Members;
typedef RDI_EnumMember                   RDI_SectionElementType_EnumMembers;
typedef RDI_GlobalVariable               RDI_SectionElementType_GlobalVariables;
typedef RDI_VMapEntry                    RDI_SectionElementType_GlobalVMap;
typedef RDI_ThreadVariable               RDI_SectionElementType_ThreadVariables;
typedef RDI_Constant                     RDI_SectionElementType_Constants;
typedef RDI_Procedure                    RDI_SectionElementType_Procedures;
typedef RDI_Scope                        RDI_SectionElementType_Scopes;
typedef RDI_U64                          RDI_SectionElementType_ScopeVOffData;
typedef RDI_VMapEntry                    RDI_SectionElementType_ScopeVMap;
typedef RDI_InlineSite                   RDI_SectionElementType_InlineSites;
typedef RDI_Local                        RDI_SectionElementType_Locals;
typedef RDI_LocationBlock                RDI_SectionElementType_LocationBlocks;
typedef RDI_U8                           RDI_SectionElementType_LocationData;
typedef RDI_U8                           RDI_SectionElementType_ConstantValueData;
typedef RDI_U32                          RDI_SectionElementType_ConstantValueTable;
typedef RDI_MD5                          RDI_SectionElementType_MD5Checksums;
typedef RDI_SHA1                         RDI_SectionElementType_SHA1Checksums;
typedef RDI_SHA256                       RDI_SectionElementType_SHA256Checksums;
typedef RDI_U64                          RDI_SectionElementType_Timestamps;
typedef RDI_NameMap                      RDI_SectionElementType_NameMaps;
typedef RDI_NameMapBucket                RDI_SectionElementType_NameMapBuckets;
typedef RDI_NameMapNode                  RDI_SectionElementType_NameMapNodes;

RDI_PROC RDI_U64 rdi_hash(RDI_U8 *ptr, RDI_U64 size);
RDI_PROC RDI_U8 *rdi_string_from_type_kind(RDI_TypeKind kind, RDI_U64 *size_out);
RDI_PROC RDI_U32 rdi_size_from_basic_type_kind(RDI_TypeKind kind);
RDI_PROC RDI_U32 rdi_addr_size_from_arch(RDI_Arch arch);
RDI_PROC RDI_EvalConversionKind rdi_eval_conversion_kind_from_typegroups(RDI_EvalTypeGroup in, RDI_EvalTypeGroup out);
RDI_PROC RDI_S32 rdi_eval_op_typegroup_are_compatible(RDI_EvalOp op, RDI_EvalTypeGroup group);
RDI_PROC RDI_U8 *rdi_explanation_string_from_eval_conversion_kind(RDI_EvalConversionKind kind, RDI_U64 *size_out);

extern RDI_U16 rdi_section_element_size_table[44];
extern RDI_U16 rdi_eval_op_ctrlbits_table[53];

#endif // RDI_H
