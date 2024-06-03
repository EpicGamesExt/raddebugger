// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////////////////////////////////////
//~ RAD Debug Info, (R)AD(D)BG(I) Format Library
//
// Defines standard RDI debug information format types and
// functions.

#ifndef RDI_FORMAT_H
#define RDI_FORMAT_H

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
//~ Format Constants

// \"raddbg\0\0\"
#define RDI_MAGIC_CONSTANT   0x0000676264646172
#define RDI_ENCODING_VERSION 2

////////////////////////////////////////////////////////////////
//~ Format Types & Functions

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
RDI_DataSectionTag_LineTables           = 0x0008,
RDI_DataSectionTag_LineInfoVoffs        = 0x0009,
RDI_DataSectionTag_LineInfoLines        = 0x000A,
RDI_DataSectionTag_LineInfoColumns      = 0x000B,
RDI_DataSectionTag_Units                = 0x000C,
RDI_DataSectionTag_UnitVmap             = 0x000D,
RDI_DataSectionTag_TypeNodes            = 0x000E,
RDI_DataSectionTag_UDTs                 = 0x000F,
RDI_DataSectionTag_Members              = 0x0010,
RDI_DataSectionTag_EnumMembers          = 0x0011,
RDI_DataSectionTag_GlobalVariables      = 0x0012,
RDI_DataSectionTag_GlobalVmap           = 0x0013,
RDI_DataSectionTag_ThreadVariables      = 0x0014,
RDI_DataSectionTag_Procedures           = 0x0015,
RDI_DataSectionTag_Scopes               = 0x0016,
RDI_DataSectionTag_ScopeVoffData        = 0x0017,
RDI_DataSectionTag_ScopeVmap            = 0x0018,
RDI_DataSectionTag_InlineSites          = 0x0019,
RDI_DataSectionTag_Locals               = 0x001A,
RDI_DataSectionTag_LocationBlocks       = 0x001B,
RDI_DataSectionTag_LocationData         = 0x001C,
RDI_DataSectionTag_NameMaps             = 0x001D,
RDI_DataSectionTag_PRIMARY_COUNT        = 0x001E,
RDI_DataSectionTag_SECONDARY            = 0x80000000,
RDI_DataSectionTag_LineMapNumbers       = RDI_DataSectionTag_SECONDARY|0x0001,
RDI_DataSectionTag_LineMapRanges        = RDI_DataSectionTag_SECONDARY|0x0002,
RDI_DataSectionTag_LineMapVoffs         = RDI_DataSectionTag_SECONDARY|0x0003,
RDI_DataSectionTag_NameMapBuckets       = RDI_DataSectionTag_SECONDARY|0x0004,
RDI_DataSectionTag_NameMapNodes         = RDI_DataSectionTag_SECONDARY|0x0005,
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
} RDI_RegCodeX64Enum;

typedef RDI_U32 RDI_BinarySectionFlags;
typedef enum RDI_BinarySectionFlagsEnum
{
RDI_BinarySectionFlag_Read       = 1<<0,
RDI_BinarySectionFlag_Write      = 1<<1,
RDI_BinarySectionFlag_Execute    = 1<<2,
} RDI_BinarySectionFlagsEnum;

typedef RDI_U32 RDI_Language;
typedef enum RDI_LanguageEnum
{
RDI_Language_NULL       = 0,
RDI_Language_C          = 1,
RDI_Language_CPlusPlus  = 2,
RDI_Language_COUNT      = 3,
} RDI_LanguageEnum;

typedef RDI_U16 RDI_TypeKind;
typedef enum RDI_TypeKindEnum
{
RDI_TypeKind_NULL                 = 0x0000,
RDI_TypeKind_Void                 = 0x0001,
RDI_TypeKind_Handle               = 0x0002,
RDI_TypeKind_Char8                = 0x0003,
RDI_TypeKind_Char16               = 0x0004,
RDI_TypeKind_Char32               = 0x0005,
RDI_TypeKind_UChar8               = 0x0006,
RDI_TypeKind_UChar16              = 0x0007,
RDI_TypeKind_UChar32              = 0x0008,
RDI_TypeKind_U8                   = 0x0009,
RDI_TypeKind_U16                  = 0x000A,
RDI_TypeKind_U32                  = 0x000B,
RDI_TypeKind_U64                  = 0x000C,
RDI_TypeKind_U128                 = 0x000D,
RDI_TypeKind_U256                 = 0x000E,
RDI_TypeKind_U512                 = 0x000F,
RDI_TypeKind_S8                   = 0x0010,
RDI_TypeKind_S16                  = 0x0011,
RDI_TypeKind_S32                  = 0x0012,
RDI_TypeKind_S64                  = 0x0013,
RDI_TypeKind_S128                 = 0x0014,
RDI_TypeKind_S256                 = 0x0015,
RDI_TypeKind_S512                 = 0x0016,
RDI_TypeKind_Bool                 = 0x0017,
RDI_TypeKind_F16                  = 0x0018,
RDI_TypeKind_F32                  = 0x0019,
RDI_TypeKind_F32PP                = 0x001A,
RDI_TypeKind_F48                  = 0x001B,
RDI_TypeKind_F64                  = 0x001C,
RDI_TypeKind_F80                  = 0x001D,
RDI_TypeKind_F128                 = 0x001E,
RDI_TypeKind_ComplexF32           = 0x001F,
RDI_TypeKind_ComplexF64           = 0x0020,
RDI_TypeKind_ComplexF80           = 0x0021,
RDI_TypeKind_ComplexF128          = 0x0022,
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
RDI_EvalOp_Abs                  = 16,
RDI_EvalOp_Neg                  = 17,
RDI_EvalOp_Add                  = 18,
RDI_EvalOp_Sub                  = 19,
RDI_EvalOp_Mul                  = 20,
RDI_EvalOp_Div                  = 21,
RDI_EvalOp_Mod                  = 22,
RDI_EvalOp_LShift               = 23,
RDI_EvalOp_RShift               = 24,
RDI_EvalOp_BitAnd               = 25,
RDI_EvalOp_BitOr                = 26,
RDI_EvalOp_BitXor               = 27,
RDI_EvalOp_BitNot               = 28,
RDI_EvalOp_LogAnd               = 29,
RDI_EvalOp_LogOr                = 30,
RDI_EvalOp_LogNot               = 31,
RDI_EvalOp_EqEq                 = 32,
RDI_EvalOp_NtEq                 = 33,
RDI_EvalOp_LsEq                 = 34,
RDI_EvalOp_GrEq                 = 35,
RDI_EvalOp_Less                 = 36,
RDI_EvalOp_Grtr                 = 37,
RDI_EvalOp_Trunc                = 38,
RDI_EvalOp_TruncSigned          = 39,
RDI_EvalOp_Convert              = 40,
RDI_EvalOp_Pick                 = 41,
RDI_EvalOp_Pop                  = 42,
RDI_EvalOp_Insert               = 43,
RDI_EvalOp_COUNT                = 44,
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
RDI_NameMapKind_Procedures           = 3,
RDI_NameMapKind_Types                = 4,
RDI_NameMapKind_LinkNameProcedures   = 5,
RDI_NameMapKind_NormalSourcePaths    = 6,
RDI_NameMapKind_COUNT                = 7,
} RDI_NameMapKindEnum;

#define RDI_Header_XList \
X(RDI_U64, magic)\
X(RDI_U32, encoding_version)\
X(RDI_U32, data_section_off)\
X(RDI_U32, data_section_count)\

#define RDI_DataSectionTag_XList \
X(NULL)\
X(TopLevelInfo)\
X(StringData)\
X(StringTable)\
X(IndexRuns)\
X(BinarySections)\
X(FilePathNodes)\
X(SourceFiles)\
X(LineTables)\
X(LineInfoVoffs)\
X(LineInfoLines)\
X(LineInfoColumns)\
X(Units)\
X(UnitVmap)\
X(TypeNodes)\
X(UDTs)\
X(Members)\
X(EnumMembers)\
X(GlobalVariables)\
X(GlobalVmap)\
X(ThreadVariables)\
X(Procedures)\
X(Scopes)\
X(ScopeVoffData)\
X(ScopeVmap)\
X(InlineSites)\
X(Locals)\
X(LocationBlocks)\
X(LocationData)\
X(NameMaps)\
X(PRIMARY_COUNT)\
X(SECONDARY)\
X(LineMapNumbers)\
X(LineMapRanges)\
X(LineMapVoffs)\
X(NameMapBuckets)\
X(NameMapNodes)\

#define RDI_DataSectionEncoding_XList \
X(Unpacked)\
X(LZB)\

#define RDI_DataSection_XList \
X(RDI_DataSectionTag, tag)\
X(RDI_DataSectionEncoding, encoding)\
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

#define RDI_TopLevelInfo_XList \
X(RDI_Arch, arch)\
X(RDI_U32, exe_name_string_idx)\
X(RDI_U64, exe_hash)\
X(RDI_U64, voff_max)\

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

#define RDI_FilePathNode_XList \
X(RDI_U32, name_string_idx)\
X(RDI_U32, parent_path_node)\
X(RDI_U32, first_child)\
X(RDI_U32, next_sibling)\
X(RDI_U32, source_file_idx)\

#define RDI_SourceFile_XList \
X(RDI_U32, file_path_node_idx)\
X(RDI_U32, normal_full_path_string_idx)\
X(RDI_U32, line_map_count)\
X(RDI_U32, line_map_nums_data_idx)\
X(RDI_U32, line_map_range_data_idx)\
X(RDI_U32, line_map_voff_data_idx)\

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

#define RDI_Language_XList \
X(NULL)\
X(C)\
X(CPlusPlus)\
X(COUNT)\

#define RDI_TypeKind_XList \
X(NULL)\
X(Void)\
X(Handle)\
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

#define RDI_TypeModifierFlags_XList \
X(Const)\
X(Volatile)\

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
X(RDI_U32, name_string_idx)\
X(RDI_LinkFlags, link_flags)\
X(RDI_U32, tls_off)\
X(RDI_U32, type_idx)\
X(RDI_U32, container_idx)\

#define RDI_Procedure_XList \
X(RDI_U32, name_string_idx)\
X(RDI_U32, link_name_string_idx)\
X(RDI_LinkFlags, link_flags)\
X(RDI_U32, type_idx)\
X(RDI_U32, root_scope_idx)\
X(RDI_U32, container_idx)\

#define RDI_Scope_XList \
X(RDI_U32, proc_idx)\
X(RDI_U32, parent_scope_idx)\
X(RDI_U32, first_child_scope_idx)\
X(RDI_U32, next_sibling_scope_idx)\
X(RDI_U32, voff_range_first)\
X(RDI_U32, voff_range_opl)\
X(RDI_U32, local_first)\
X(RDI_U32, local_count)\
X(RDI_U32, static_local_idx_run_first)\
X(RDI_U32, static_local_count)\

#define RDI_InlineSite_XList \
X(RDI_U32, name_string_idx)\
X(RDI_U32, call_src_file_idx)\
X(RDI_U32, call_line_num)\
X(RDI_U32, call_col_num)\
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
X(COUNT)\

#define RDI_EvalTypeGroup_XList \
X(Other)\
X(U)\
X(S)\
X(F32)\
X(F64)\
X(COUNT)\

#define RDI_EvalConversionKind_XList \
X(Noop)\
X(Legal)\
X(OtherToOther)\
X(ToOther)\
X(FromOther)\
X(COUNT)\

#define RDI_NameMapKind_XList \
X(NULL)\
X(GlobalVariables)\
X(ThreadVariables)\
X(Procedures)\
X(Types)\
X(LinkNameProcedures)\
X(NormalSourcePaths)\
X(COUNT)\

#define RDI_NameMap_XList \
X(RDI_NameMapKind, kind)\
X(RDI_U32, bucket_data_idx)\
X(RDI_U32, node_data_idx)\

#define RDI_NameMapBucket_XList \
X(RDI_U32, first_node)\
X(RDI_U32, node_count)\

#define RDI_NameMapNode_XList \
X(RDI_U32, string_idx)\
X(RDI_U32, match_count)\
X(RDI_U32, match_idx_or_idx_run_first)\

#define RDI_EVAL_CTRLBITS(decodeN,popN,pushN) ((decodeN) | ((popN) << 4) | ((pushN) << 6))
#define RDI_DECODEN_FROM_CTRLBITS(ctrlbits)   ((ctrlbits) & 0xf)
#define RDI_POPN_FROM_CTRLBITS(ctrlbits)      (((ctrlbits) >> 4) & 0x3)
#define RDI_PUSHN_FROM_CTRLBITS(ctrlbits)     (((ctrlbits) >> 6) & 0x3)
#define RDI_EncodeRegReadParam(reg,bytesize,bytepos) ((reg)|((bytesize)<<8)|((bytepos)<<16))

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
RDI_U32 line_map_count;
RDI_U32 line_map_nums_data_idx;
RDI_U32 line_map_range_data_idx;
RDI_U32 line_map_voff_data_idx;
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
      union{
        // when kind is 'Function' or 'Method'
        RDI_U32 param_idx_run_first;
        // when kind is 'MemberPtr'
        RDI_U32 owner_type_idx;
      };
    } constructed;
    
    // kind is 'user defined'
    struct
    {
      RDI_U32 name_string_idx;
      RDI_U32 direct_type_idx;
      RDI_U32 udt_idx;
    } user_defined;
    
    // (kind = Bitfield)
    struct
    {
      RDI_U32 direct_type_idx;
      RDI_U32 off;
      RDI_U32 size;
    } bitfield;
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

typedef struct RDI_Procedure RDI_Procedure;
struct RDI_Procedure
{
RDI_U32 name_string_idx;
RDI_U32 link_name_string_idx;
RDI_LinkFlags link_flags;
RDI_U32 type_idx;
RDI_U32 root_scope_idx;
RDI_U32 container_idx;
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
RDI_U32 static_local_idx_run_first;
RDI_U32 static_local_count;
};

typedef struct RDI_InlineSite RDI_InlineSite;
struct RDI_InlineSite
{
RDI_U32 name_string_idx;
RDI_U32 call_src_file_idx;
RDI_U32 call_line_num;
RDI_U32 call_col_num;
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
RDI_NameMapKind kind;
RDI_U32 bucket_data_idx;
RDI_U32 node_data_idx;
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

RDI_PROC RDI_U64 rdi_hash(RDI_U8 *ptr, RDI_U64 size);
RDI_PROC RDI_U32 rdi_size_from_basic_type_kind(RDI_TypeKind kind);
RDI_PROC RDI_U32 rdi_addr_size_from_arch(RDI_Arch arch);
RDI_PROC RDI_EvalConversionKind rdi_eval_conversion_kind_from_typegroups(RDI_EvalTypeGroup in, RDI_EvalTypeGroup out);
RDI_PROC RDI_S32 rdi_eval_op_typegroup_are_compatible(RDI_EvalOp op, RDI_EvalTypeGroup group);
RDI_PROC RDI_U8 *rdi_explanation_string_from_eval_conversion_kind(RDI_EvalConversionKind kind, RDI_U64 *size_out);

extern RDI_U8 rdi_eval_op_ctrlbits_table[45];

#endif // RDI_FORMAT_H
