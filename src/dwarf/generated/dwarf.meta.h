// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef DWARF_META_H
#define DWARF_META_H

typedef U16 DW_Version;
typedef enum DW_VersionEnum
{
DW_Version_Null,
DW_Version_1,
DW_Version_2,
DW_Version_3,
DW_Version_4,
DW_Version_5,
DW_Version_Last = DW_Version_5,
} DW_VersionEnum;

typedef enum DW_Ext
{
DW_Ext_Null,
DW_Ext_GNU,
DW_Ext_LLVM,
DW_Ext_Apple,
DW_Ext_MIPS,
} DW_Ext;

typedef U32 DW_ExtFlags;
typedef enum DW_ExtFlagsEnum
{
DW_ExtFlag_GNU = (1<<DW_Ext_GNU),
DW_ExtFlag_LLVM = (1<<DW_Ext_LLVM),
DW_ExtFlag_Apple = (1<<DW_Ext_Apple),
DW_ExtFlag_MIPS = (1<<DW_Ext_MIPS),
DW_ExtFlag_All = 0xffffffffu,
} DW_ExtFlagsEnum;

typedef enum DW_Format
{
DW_Format_Null,
DW_Format_32Bit,
DW_Format_64Bit,
} DW_Format;

typedef enum DW_SectionKind
{
DW_SectionKind_Null,
DW_SectionKind_Abbrev,
DW_SectionKind_ARanges,
DW_SectionKind_Frame,
DW_SectionKind_Info,
DW_SectionKind_Line,
DW_SectionKind_Loc,
DW_SectionKind_MacInfo,
DW_SectionKind_PubNames,
DW_SectionKind_PubTypes,
DW_SectionKind_Ranges,
DW_SectionKind_Str,
DW_SectionKind_Addr,
DW_SectionKind_LocLists,
DW_SectionKind_RngLists,
DW_SectionKind_StrOffsets,
DW_SectionKind_LineStr,
DW_SectionKind_Names,
DW_SectionKind_COUNT,
} DW_SectionKind;

typedef U32 DW_SectionFlags;
typedef enum DW_SectionFlagsEnum
{
DW_SectionFlag_Null = (1 << DW_SectionKind_Null),
DW_SectionFlag_Abbrev = (1 << DW_SectionKind_Abbrev),
DW_SectionFlag_ARanges = (1 << DW_SectionKind_ARanges),
DW_SectionFlag_Frame = (1 << DW_SectionKind_Frame),
DW_SectionFlag_Info = (1 << DW_SectionKind_Info),
DW_SectionFlag_Line = (1 << DW_SectionKind_Line),
DW_SectionFlag_Loc = (1 << DW_SectionKind_Loc),
DW_SectionFlag_MacInfo = (1 << DW_SectionKind_MacInfo),
DW_SectionFlag_PubNames = (1 << DW_SectionKind_PubNames),
DW_SectionFlag_PubTypes = (1 << DW_SectionKind_PubTypes),
DW_SectionFlag_Ranges = (1 << DW_SectionKind_Ranges),
DW_SectionFlag_Str = (1 << DW_SectionKind_Str),
DW_SectionFlag_Addr = (1 << DW_SectionKind_Addr),
DW_SectionFlag_LocLists = (1 << DW_SectionKind_LocLists),
DW_SectionFlag_RngLists = (1 << DW_SectionKind_RngLists),
DW_SectionFlag_StrOffsets = (1 << DW_SectionKind_StrOffsets),
DW_SectionFlag_LineStr = (1 << DW_SectionKind_LineStr),
DW_SectionFlag_Names = (1 << DW_SectionKind_Names),
DW_SectionFlag_All = 0xffffffffu,
} DW_SectionFlagsEnum;

typedef U32 DW_Language;
typedef enum DW_LanguageEnum
{
DW_Language_Null = 0x00,
DW_Language_C89 = 0x01,
DW_Language_C = 0x02,
DW_Language_Ada83 = 0x03,
DW_Language_CPlusPlus = 0x04,
DW_Language_Cobol74 = 0x05,
DW_Language_Cobol85 = 0x06,
DW_Language_Fortran77 = 0x07,
DW_Language_Fortran90 = 0x08,
DW_Language_Pascal83 = 0x09,
DW_Language_Modula2 = 0x0A,
DW_Language_Java = 0x0B,
DW_Language_C99 = 0x0C,
DW_Language_Ada95 = 0x0D,
DW_Language_Fortran95 = 0x0E,
DW_Language_PLI = 0x0F,
DW_Language_ObjC = 0x10,
DW_Language_ObjCPlusPlus = 0x11,
DW_Language_UPC = 0x12,
DW_Language_D = 0x13,
DW_Language_Python = 0x14,
DW_Language_OpenCL = 0x15,
DW_Language_Go = 0x16,
DW_Language_Modula3 = 0x17,
DW_Language_Haskell = 0x18,
DW_Language_CPlusPlus03 = 0x19,
DW_Language_CPlusPlus11 = 0x1a,
DW_Language_OCaml = 0x1b,
DW_Language_Rust = 0x1c,
DW_Language_C11 = 0x1d,
DW_Language_Swift = 0x1e,
DW_Language_Julia = 0x1f,
DW_Language_Dylan = 0x20,
DW_Language_CPlusPlus14 = 0x21,
DW_Language_Fortran03 = 0x22,
DW_Language_Fortran08 = 0x23,
DW_Language_RenderScript = 0x24,
DW_Language_BLISS = 0x25,
DW_Language_MipsAssembler = 0x8001,
DW_Language_GoogleRenderScript = 0x8E57,
DW_Language_SunAssembler = 0x9001,
DW_Language_BorlandDelphi = 0xB000,
} DW_LanguageEnum;

typedef U32 DW_InlKind;
typedef enum DW_InlKindEnum
{
DW_InlKind_NotInlined = 0x00,
DW_InlKind_Inlined = 0x01,
DW_InlKind_DeclaredNotInlined = 0x02,
DW_InlKind_DeclaredInlined = 0x03,
} DW_InlKindEnum;

typedef U8 DW_StdOpcode;
typedef enum DW_StdOpcodeEnum
{
DW_StdOpcode_ExtendedOpcode = 0x00,
DW_StdOpcode_Copy = 0x01,
DW_StdOpcode_AdvancePc = 0x02,
DW_StdOpcode_AdvanceLine = 0x03,
DW_StdOpcode_SetFile = 0x04,
DW_StdOpcode_SetColumn = 0x05,
DW_StdOpcode_NegateStmt = 0x06,
DW_StdOpcode_SetBasicBlock = 0x07,
DW_StdOpcode_ConstAddPc = 0x08,
DW_StdOpcode_FixedAdvancePc = 0x09,
DW_StdOpcode_SetPrologueEnd = 0x0A,
DW_StdOpcode_SetEpilogueBegin = 0x0B,
DW_StdOpcode_SetIsa = 0x0C,
DW_StdOpcode_COUNT,
} DW_StdOpcodeEnum;

typedef U8 DW_ExtOpcode;
typedef enum DW_ExtOpcodeEnum
{
DW_ExtOpcode_Undefined = 0x00,
DW_ExtOpcode_EndSequence = 0x01,
DW_ExtOpcode_SetAddress = 0x02,
DW_ExtOpcode_DefineFile = 0x03,
DW_ExtOpcode_SetDiscriminator = 0x04,
DW_ExtOpcode_UserLo = 0x80,
DW_ExtOpcode_UserHi = 0xff,
} DW_ExtOpcodeEnum;

typedef U64 DW_TagKind;
typedef enum DW_TagKindEnum
{
DW_TagKind_Null = 0,
DW_TagKind_ArrayType = 0x01,
DW_TagKind_ClassType = 0x02,
DW_TagKind_EntryPoint = 0x03,
DW_TagKind_EnumerationType = 0x04,
DW_TagKind_FormalParameter = 0x05,
DW_TagKind_ImportedDeclaration = 0x08,
DW_TagKind_Label = 0x0a,
DW_TagKind_LexicalBlock = 0x0b,
DW_TagKind_Member = 0x0d,
DW_TagKind_PointerType = 0x0f,
DW_TagKind_ReferenceType = 0x10,
DW_TagKind_CompileUnit = 0x11,
DW_TagKind_StringType = 0x12,
DW_TagKind_StructureType = 0x13,
DW_TagKind_SubroutineType = 0x15,
DW_TagKind_Typedef = 0x16,
DW_TagKind_UnionType = 0x17,
DW_TagKind_UnspecifiedParameters = 0x18,
DW_TagKind_Variant = 0x19,
DW_TagKind_CommonBlock = 0x1a,
DW_TagKind_CommonInclusion = 0x1b,
DW_TagKind_Inheritance = 0x1c,
DW_TagKind_InlinedSubroutine = 0x1d,
DW_TagKind_Module = 0x1e,
DW_TagKind_PtrToMemberType = 0x1f,
DW_TagKind_SetType = 0x20,
DW_TagKind_SubrangeType = 0x21,
DW_TagKind_WithStmt = 0x22,
DW_TagKind_AccessDeclaration = 0x23,
DW_TagKind_BaseType = 0x24,
DW_TagKind_CatchBlock = 0x25,
DW_TagKind_ConstType = 0x26,
DW_TagKind_Constant = 0x27,
DW_TagKind_Enumerator = 0x28,
DW_TagKind_FileType = 0x29,
DW_TagKind_Friend = 0x2a,
DW_TagKind_NameList = 0x2b,
DW_TagKind_NameListItem = 0x2c,
DW_TagKind_PackedType = 0x2d,
DW_TagKind_SubProgram = 0x2e,
DW_TagKind_TemplateTypeParameter = 0x2f,
DW_TagKind_TemplateValueParameter = 0x30,
DW_TagKind_ThrownType = 0x31,
DW_TagKind_TryBlock = 0x32,
DW_TagKind_VariantPart = 0x33,
DW_TagKind_Variable = 0x34,
DW_TagKind_VolatileType = 0x35,
DW_TagKind_DwarfProcedure = 0x36,
DW_TagKind_RestrictType = 0x37,
DW_TagKind_InterfaceType = 0x38,
DW_TagKind_Namespace = 0x39,
DW_TagKind_ImportedModule = 0x3a,
DW_TagKind_UnspecifiedType = 0x3b,
DW_TagKind_PartialUnit = 0x3c,
DW_TagKind_ImportedUnit = 0x3d,
DW_TagKind_Condition = 0x3f,
DW_TagKind_SharedType = 0x40,
DW_TagKind_TypeUnit = 0x41,
DW_TagKind_RValueReferenceType = 0x42,
DW_TagKind_TemplateAlias = 0x43,
DW_TagKind_CoarrayType = 0x44,
DW_TagKind_GenericSubrange = 0x45,
DW_TagKind_DynamicType = 0x46,
DW_TagKind_AtomicType = 0x47,
DW_TagKind_CallSite = 0x48,
DW_TagKind_CallSiteParameter = 0x49,
DW_TagKind_SkeletonUnit = 0x4A,
DW_TagKind_ImmutableType = 0x4B,
DW_TagKind_UserLo = 0x4080,
DW_TagKind_UserHi = 0xffff,
} DW_TagKindEnum;

typedef U64 DW_GNU_TagKind;
typedef enum DW_GNU_TagKindEnum
{
DW_GNU_TagKind_CallSite = 0x4109,
DW_GNU_TagKind_CallSiteParameter = 0x410a,
} DW_GNU_TagKindEnum;

typedef U32 DW_AttribClassKind;
typedef enum DW_AttribClassKindEnum
{
DW_AttribClassKind_Null,
DW_AttribClassKind_Undefined = 1,
DW_AttribClassKind_Address = 2,
DW_AttribClassKind_Block = 3,
DW_AttribClassKind_Const = 4,
DW_AttribClassKind_ExprLoc = 5,
DW_AttribClassKind_Flag = 6,
DW_AttribClassKind_LinePtr = 7,
DW_AttribClassKind_LocListPtr = 8,
DW_AttribClassKind_MacPtr = 9,
DW_AttribClassKind_RngListPtr = 10,
DW_AttribClassKind_Reference = 11,
DW_AttribClassKind_String = 12,
DW_AttribClassKind_LocList = 13,
DW_AttribClassKind_RngList = 14,
DW_AttribClassKind_StrOffsetsPtr = 15,
DW_AttribClassKind_AddrPtr = 16,
} DW_AttribClassKindEnum;

typedef U32 DW_AttribClass;
typedef enum DW_AttribClassEnum
{
DW_AttribClass_Null = 0,
DW_AttribClass_Undefined = (1 << 0),
DW_AttribClass_Address = (1 << 1),
DW_AttribClass_Block = (1 << 2),
DW_AttribClass_Const = (1 << 3),
DW_AttribClass_ExprLoc = (1 << 4),
DW_AttribClass_Flag = (1 << 5),
DW_AttribClass_LinePtr = (1 << 6),
DW_AttribClass_LocListPtr = (1 << 7),
DW_AttribClass_MacPtr = (1 << 8),
DW_AttribClass_RngListPtr = (1 << 9),
DW_AttribClass_Reference = (1 << 10),
DW_AttribClass_String = (1 << 11),
DW_AttribClass_LocList = (1 << 12),
DW_AttribClass_RngList = (1 << 13),
DW_AttribClass_StrOffsetsPtr = (1 << 14),
DW_AttribClass_AddrPtr = (1 << 15),
DW_AttribClass_AllPtrs = DW_AttribClass_AddrPtr|DW_AttribClass_LinePtr|DW_AttribClass_LocList|DW_AttribClass_LocListPtr|DW_AttribClass_MacPtr|DW_AttribClass_RngList|DW_AttribClass_RngListPtr|DW_AttribClass_StrOffsetsPtr,
} DW_AttribClassEnum;

typedef U64 DW_FormKind;
typedef enum DW_FormKindEnum
{
DW_FormKind_Null = 0,
DW_FormKind_Addr = 0x1,
DW_FormKind_Block2 = 0x3,
DW_FormKind_Block4 = 0x4,
DW_FormKind_Data2 = 0x5,
DW_FormKind_Data4 = 0x6,
DW_FormKind_Data8 = 0x7,
DW_FormKind_String = 0x8,
DW_FormKind_Block = 0x9,
DW_FormKind_Block1 = 0xa,
DW_FormKind_Data1 = 0xb,
DW_FormKind_Flag = 0xc,
DW_FormKind_SData = 0xd,
DW_FormKind_Strp = 0xe,
DW_FormKind_UData = 0xf,
DW_FormKind_RefAddr = 0x10,
DW_FormKind_Ref1 = 0x11,
DW_FormKind_Ref2 = 0x12,
DW_FormKind_Ref4 = 0x13,
DW_FormKind_Ref8 = 0x14,
DW_FormKind_RefUData = 0x15,
DW_FormKind_Indirect = 0x16,
DW_FormKind_SecOffset = 0x17,
DW_FormKind_ExprLoc = 0x18,
DW_FormKind_FlagPresent = 0x19,
DW_FormKind_RefSig8 = 0x20,
DW_FormKind_Strx = 0x1a,
DW_FormKind_Addrx = 0x1b,
DW_FormKind_RefSup4 = 0x1c,
DW_FormKind_StrpSup = 0x1d,
DW_FormKind_Data16 = 0x1e,
DW_FormKind_LineStrp = 0x1f,
DW_FormKind_ImplicitConst = 0x21,
DW_FormKind_LocListx = 0x22,
DW_FormKind_RngListx = 0x23,
DW_FormKind_RefSup8 = 0x24,
DW_FormKind_Strx1 = 0x25,
DW_FormKind_Strx2 = 0x26,
DW_FormKind_Strx3 = 0x27,
DW_FormKind_Strx4 = 0x28,
DW_FormKind_Addrx1 = 0x29,
DW_FormKind_Addrx2 = 0x2a,
DW_FormKind_Addrx3 = 0x2b,
DW_FormKind_Addrx4 = 0x2c,
} DW_FormKindEnum;

typedef U64 DW_GNU_FormKind;
typedef enum DW_GNU_FormKindEnum
{
DW_GNU_FormKind_AddrIndex = 0x1f01,
DW_GNU_FormKind_StrIndex = 0x1f02,
DW_GNU_FormKind_RefAlt = 0x1f20,
DW_GNU_FormKind_StrpAlt = 0x1f21,
} DW_GNU_FormKindEnum;

typedef U32 DW_AttribKind;
typedef enum DW_AttribKindEnum
{
DW_AttribKind_Null = 0,
DW_AttribKind_Sibling = 0x1,
DW_AttribKind_Location = 0x2,
DW_AttribKind_Name = 0x3,
DW_AttribKind_Ordering = 0x9,
DW_AttribKind_ByteSize = 0xB,
DW_AttribKind_BitOffset = 0xC,
DW_AttribKind_BitSize = 0xD,
DW_AttribKind_StmtList = 0x10,
DW_AttribKind_LowPc = 0x11,
DW_AttribKind_HighPc = 0x12,
DW_AttribKind_Language = 0x13,
DW_AttribKind_Discr = 0x15,
DW_AttribKind_DiscrValue = 0x16,
DW_AttribKind_Visibility = 0x17,
DW_AttribKind_Import = 0x18,
DW_AttribKind_StringLength = 0x19,
DW_AttribKind_CommonReference = 0x1a,
DW_AttribKind_CompDir = 0x1b,
DW_AttribKind_ConstValue = 0x1c,
DW_AttribKind_ContainingType = 0x1d,
DW_AttribKind_DefaultValue = 0x1e,
DW_AttribKind_Inline = 0x20,
DW_AttribKind_IsOptional = 0x21,
DW_AttribKind_LowerBound = 0x22,
DW_AttribKind_Producer = 0x25,
DW_AttribKind_Prototyped = 0x27,
DW_AttribKind_ReturnAddr = 0x2a,
DW_AttribKind_StartScope = 0x2c,
DW_AttribKind_BitStride = 0x2e,
DW_AttribKind_UpperBound = 0x2f,
DW_AttribKind_AbstractOrigin = 0x31,
DW_AttribKind_Accessibility = 0x32,
DW_AttribKind_AddressClass = 0x33,
DW_AttribKind_Artificial = 0x34,
DW_AttribKind_BaseTypes = 0x35,
DW_AttribKind_CallingConvention = 0x36,
DW_AttribKind_Count = 0x37,
DW_AttribKind_DataMemberLocation = 0x38,
DW_AttribKind_DeclColumn = 0x39,
DW_AttribKind_DeclFile = 0x3a,
DW_AttribKind_DeclLine = 0x3b,
DW_AttribKind_Declaration = 0x3c,
DW_AttribKind_DiscrList = 0x3d,
DW_AttribKind_Encoding = 0x3e,
DW_AttribKind_External = 0x3f,
DW_AttribKind_FrameBase = 0x40,
DW_AttribKind_Friend = 0x41,
DW_AttribKind_IdentifierCase = 0x42,
DW_AttribKind_MacroInfo = 0x43,
DW_AttribKind_NameListItem = 0x44,
DW_AttribKind_Priority = 0x45,
DW_AttribKind_Segment = 0x46,
DW_AttribKind_Specification = 0x47,
DW_AttribKind_StaticLink = 0x48,
DW_AttribKind_Type = 0x49,
DW_AttribKind_UseLocation = 0x4a,
DW_AttribKind_VariableParameter = 0x4b,
DW_AttribKind_Virtuality = 0x4c,
DW_AttribKind_VTableElemLocation = 0x4d,
DW_AttribKind_Allocated = 0x4e,
DW_AttribKind_Associated = 0x4f,
DW_AttribKind_DataLocation = 0x50,
DW_AttribKind_ByteStride = 0x51,
DW_AttribKind_EntryPc = 0x52,
DW_AttribKind_UseUtf8 = 0x53,
DW_AttribKind_Extension = 0x54,
DW_AttribKind_Ranges = 0x55,
DW_AttribKind_Trampoline = 0x56,
DW_AttribKind_CallColumn = 0x57,
DW_AttribKind_CallFile = 0x58,
DW_AttribKind_CallLine = 0x59,
DW_AttribKind_Description = 0x5a,
DW_AttribKind_BinaryScale = 0x5b,
DW_AttribKind_DecimalScale = 0x5c,
DW_AttribKind_Small = 0x5d,
DW_AttribKind_DecimalSign = 0x5e,
DW_AttribKind_DigitCount = 0x5f,
DW_AttribKind_PictureString = 0x60,
DW_AttribKind_Mutable = 0x61,
DW_AttribKind_ThreadsScaled = 0x62,
DW_AttribKind_Explicit = 0x63,
DW_AttribKind_ObjectPointer = 0x64,
DW_AttribKind_Endianity = 0x65,
DW_AttribKind_Elemental = 0x66,
DW_AttribKind_Pure = 0x67,
DW_AttribKind_Recursive = 0x68,
DW_AttribKind_Signature = 0x69,
DW_AttribKind_MainSubProgram = 0x6a,
DW_AttribKind_DataBitOffset = 0x6b,
DW_AttribKind_ConstExpr = 0x6c,
DW_AttribKind_EnumClass = 0x6d,
DW_AttribKind_LinkageName = 0x6e,
DW_AttribKind_StringLengthBitSize = 0x6f,
DW_AttribKind_StringLengthByteSize = 0x70,
DW_AttribKind_Rank = 0x71,
DW_AttribKind_StrOffsetsBase = 0x72,
DW_AttribKind_AddrBase = 0x73,
DW_AttribKind_RngListsBase = 0x74,
DW_AttribKind_DwoName = 0x76,
DW_AttribKind_Reference = 0x77,
DW_AttribKind_RValueReference = 0x78,
DW_AttribKind_Macros = 0x79,
DW_AttribKind_CallAllCalls = 0x7a,
DW_AttribKind_CallAllSourceCalls = 0x7b,
DW_AttribKind_CallAllTailCalls = 0x7c,
DW_AttribKind_CallReturnPc = 0x7d,
DW_AttribKind_CallValue = 0x7e,
DW_AttribKind_CallOrigin = 0x7f,
DW_AttribKind_CallParameter = 0x80,
DW_AttribKind_CallPc = 0x81,
DW_AttribKind_CallTailCall = 0x82,
DW_AttribKind_CallTarget = 0x83,
DW_AttribKind_CallTargetClobbered = 0x84,
DW_AttribKind_CallDataLocation = 0x85,
DW_AttribKind_CallDataValue = 0x86,
DW_AttribKind_NoReturn = 0x87,
DW_AttribKind_Alignment = 0x88,
DW_AttribKind_ExportSymbols = 0x89,
DW_AttribKind_Deleted = 0x8a,
DW_AttribKind_Defaulted = 0x8b,
DW_AttribKind_LocListsBase = 0x8c,
DW_AttribKind_UserLo = 0x2000,
DW_AttribKind_UserHi = 0x3fff,
} DW_AttribKindEnum;

typedef U32 DW_GNU_AttribKind;
typedef enum DW_GNU_AttribKindEnum
{
DW_GNU_AttribKind_Vector = 0x2107,
DW_GNU_AttribKind_GuardedBy = 0x2108,
DW_GNU_AttribKind_PtGuardedBy = 0x2109,
DW_GNU_AttribKind_Guarded = 0x210a,
DW_GNU_AttribKind_PtGuarded = 0x210b,
DW_GNU_AttribKind_LocksExcluded = 0x210c,
DW_GNU_AttribKind_ExclusiveLocksRequired = 0x210d,
DW_GNU_AttribKind_SharedLocksRequired = 0x210e,
DW_GNU_AttribKind_OdrSignature = 0x210f,
DW_GNU_AttribKind_TemplateName = 0x2110,
DW_GNU_AttribKind_CallSiteValue = 0x2111,
DW_GNU_AttribKind_CallSiteDataValue = 0x2112,
DW_GNU_AttribKind_CallSiteTarget = 0x2113,
DW_GNU_AttribKind_CallSiteTargetClobbered = 0x2114,
DW_GNU_AttribKind_TailCall = 0x2115,
DW_GNU_AttribKind_AllTailCallsSites = 0x2116,
DW_GNU_AttribKind_AllCallSites = 0x2117,
DW_GNU_AttribKind_AllSourceCallSites = 0x2118,
DW_GNU_AttribKind_Macros = 0x2119,
DW_GNU_AttribKind_Deleted = 0x211a,
DW_GNU_AttribKind_DwoName = 0x2130,
DW_GNU_AttribKind_DwoId = 0x2131,
DW_GNU_AttribKind_RangesBase = 0x2132,
DW_GNU_AttribKind_AddrBase = 0x2133,
DW_GNU_AttribKind_PubNames = 0x2134,
DW_GNU_AttribKind_PubTypes = 0x2135,
DW_GNU_AttribKind_Discriminator = 0x2136,
DW_GNU_AttribKind_LocViews = 0x2137,
DW_GNU_AttribKind_EntryView = 0x2138,
DW_GNU_AttribKind_DescriptiveType = 0x2302,
DW_GNU_AttribKind_Numerator = 0x2303,
DW_GNU_AttribKind_Denominator = 0x2304,
DW_GNU_AttribKind_Bias = 0x2305,
} DW_GNU_AttribKindEnum;

typedef U32 DW_LLVM_AttribKind;
typedef enum DW_LLVM_AttribKindEnum
{
DW_LLVM_AttribKind_IncludePath = 0x3e00,
DW_LLVM_AttribKind_ConfigMacros = 0x3e01,
DW_LLVM_AttribKind_SysRoot = 0x3e02,
DW_LLVM_AttribKind_TagOffset = 0x3e03,
DW_LLVM_AttribKind_ApiNotes = 0x3e07,
} DW_LLVM_AttribKindEnum;

typedef U32 DW_Apple_AttribKind;
typedef enum DW_Apple_AttribKindEnum
{
DW_Apple_AttribKind_Optimized = 0x3fe1,
DW_Apple_AttribKind_Flags = 0x3fe2,
DW_Apple_AttribKind_Isa = 0x3fe3,
DW_Apple_AttribKind_Block = 0x3fe4,
DW_Apple_AttribKind_MajorRuntimeVers = 0x3fe5,
DW_Apple_AttribKind_RuntimeClass = 0x3fe6,
DW_Apple_AttribKind_OmitFramePtr = 0x3fe7,
DW_Apple_AttribKind_PropertyName = 0x3fe8,
DW_Apple_AttribKind_PropertyGetter = 0x3fe9,
DW_Apple_AttribKind_PropertySetter = 0x3fea,
DW_Apple_AttribKind_PropertyAttribute = 0x3feb,
DW_Apple_AttribKind_ObjcCompleteType = 0x3fec,
DW_Apple_AttribKind_Property = 0x3fed,
DW_Apple_AttribKind_ObjDirect = 0x3fee,
DW_Apple_AttribKind_Sdk = 0x3fef,
} DW_Apple_AttribKindEnum;

typedef U32 DW_MIPS_AttribKind;
typedef enum DW_MIPS_AttribKindEnum
{
DW_MIPS_AttribKind_Fde = 0x2001,
DW_MIPS_AttribKind_LoopBegin = 0x2002,
DW_MIPS_AttribKind_TailLoopBegin = 0x2003,
DW_MIPS_AttribKind_EpilogBegin = 0x2004,
DW_MIPS_AttribKind_LoopUnrollFactor = 0x2005,
DW_MIPS_AttribKind_SoftwarePipelineDepth = 0x2006,
DW_MIPS_AttribKind_LinkageName = 0x2007,
DW_MIPS_AttribKind_Stride = 0x2008,
DW_MIPS_AttribKind_AbstractName = 0x2009,
DW_MIPS_AttribKind_CloneOrigin = 0x200a,
DW_MIPS_AttribKind_HasInlines = 0x200b,
DW_MIPS_AttribKind_StrideByte = 0x200c,
DW_MIPS_AttribKind_StrideElem = 0x200d,
DW_MIPS_AttribKind_PtrDopeType = 0x200e,
DW_MIPS_AttribKind_AllocatableDopeType = 0x200f,
DW_MIPS_AttribKind_AssumedShapeDopeType = 0x2010,
DW_MIPS_AttribKind_AssumedSize = 0x2011,
} DW_MIPS_AttribKindEnum;

typedef U64 DW_ATE;
typedef enum DW_ATEEnum
{
DW_ATE_Null = 0x00,
DW_ATE_Address = 0x01,
DW_ATE_Boolean = 0x02,
DW_ATE_ComplexFloat = 0x03,
DW_ATE_Float = 0x04,
DW_ATE_Signed = 0x05,
DW_ATE_SignedChar = 0x06,
DW_ATE_Unsigned = 0x07,
DW_ATE_UnsignedChar = 0x08,
DW_ATE_ImaginaryFloat = 0x09,
DW_ATE_PackedDecimal = 0x0A,
DW_ATE_NumericString = 0x0B,
DW_ATE_Edited = 0x0C,
DW_ATE_SignedFixed = 0x0D,
DW_ATE_UnsignedFixed = 0x0E,
DW_ATE_DecimalFloat = 0x0F,
DW_ATE_Utf = 0x10,
DW_ATE_Ucs = 0x11,
DW_ATE_Ascii = 0x12,
} DW_ATEEnum;

typedef U64 DW_CallingConventionKind;
typedef enum DW_CallingConventionKindEnum
{
DW_CallingConventionKind_Normal = 0x0,
DW_CallingConventionKind_Program = 0x1,
DW_CallingConventionKind_NoCall = 0x3,
DW_CallingConventionKind_PassByValue = 0x4,
DW_CallingConventionKind_PassByReference = 0x5,
} DW_CallingConventionKindEnum;

typedef U64 DW_AccessKind;
typedef enum DW_AccessKindEnum
{
DW_AccessKind_Public = 0x0,
DW_AccessKind_Private = 0x1,
DW_AccessKind_Protected = 0x2,
} DW_AccessKindEnum;

typedef U64 DW_VirtualityKind;
typedef enum DW_VirtualityKindEnum
{
DW_VirtualityKind_None = 0x0,
DW_VirtualityKind_Virtual = 0x1,
DW_VirtualityKind_PureVirtual = 0x2,
} DW_VirtualityKindEnum;

typedef U64 DW_IdentifierCaseKind;
typedef enum DW_IdentifierCaseKindEnum
{
DW_IdentifierCaseKind_CaseSensitive = 0x00,
DW_IdentifierCaseKind_UpCase = 0x01,
DW_IdentifierCaseKind_DownCase = 0x02,
DW_IdentifierCaseKind_CaseInsensitive = 0x03,
} DW_IdentifierCaseKindEnum;

typedef U64 DW_VisibilityKind;
typedef enum DW_VisibilityKindEnum
{
DW_VisibilityKind_Null = 0x00,
DW_VisibilityKind_Local = 0x01,
DW_VisibilityKind_Exported = 0x02,
DW_VisibilityKind_Qualified = 0x03,
} DW_VisibilityKindEnum;

typedef U8 DW_RLE;
typedef enum DW_RLEEnum
{
DW_RLE_EndOfList = 0x00,
DW_RLE_BaseAddressx = 0x01,
DW_RLE_StartxEndx = 0x02,
DW_RLE_StartxLength = 0x03,
DW_RLE_OffsetPair = 0x04,
DW_RLE_BaseAddress = 0x05,
DW_RLE_StartEnd = 0x06,
DW_RLE_StartLength = 0x07,
} DW_RLEEnum;

typedef U8 DW_LLE;
typedef enum DW_LLEEnum
{
DW_LLE_EndOfList = 0x00,
DW_LLE_BaseAddressx = 0x01,
DW_LLE_StartxEndx = 0x02,
DW_LLE_StartxLength = 0x03,
DW_LLE_OffsetPair = 0x04,
DW_LLE_DefaultLocation = 0x05,
DW_LLE_BaseAddress = 0x06,
DW_LLE_StartEnd = 0x07,
DW_LLE_StartLength = 0x08,
} DW_LLEEnum;

typedef U64 DW_AddrClass;
typedef enum DW_AddrClassEnum
{
DW_AddrClass_None = 0,
DW_AddrClass_Near16 = 1,
DW_AddrClass_Far16 = 2,
DW_AddrClass_Huge16 = 3,
DW_AddrClass_Near32 = 4,
DW_AddrClass_Far32 = 5,
} DW_AddrClassEnum;

typedef U64 DW_CompUnitKind;
typedef enum DW_CompUnitKindEnum
{
DW_CompUnitKind_Reserved = 0,
DW_CompUnitKind_Compile = 1,
DW_CompUnitKind_Type = 2,
DW_CompUnitKind_Partial = 3,
DW_CompUnitKind_Skeleton = 4,
DW_CompUnitKind_SplitCompile = 5,
DW_CompUnitKind_SplitType = 6,
DW_CompUnitKind_UserLo = 0x80,
DW_CompUnitKind_UserHi = 0xff,
} DW_CompUnitKindEnum;

typedef U64 DW_LNCT;
typedef enum DW_LNCTEnum
{
DW_LNCT_Path = 0x1,
DW_LNCT_DirectoryIndex = 0x2,
DW_LNCT_TimeStamp = 0x3,
DW_LNCT_Size = 0x4,
DW_LNCT_MD5 = 0x5,
DW_LNCT_LLVM_Source = 0x2001,
DW_LNCT_UserLo = 0x2000,
DW_LNCT_UserHi = 0x3fff,
} DW_LNCTEnum;

typedef enum DW_CFAOperandKind
{
DW_CFAOperandKind_Null,
DW_CFAOperandKind_Value,
DW_CFAOperandKind_Register,
DW_CFAOperandKind_Expression,
} DW_CFAOperandKind;

typedef U8 DW_CFAOpCode;
typedef enum DW_CFAOpCodeEnum
{
DW_CFAOpCode_Nop = 0x0,
DW_CFAOpCode_SetLoc = 0x1,
DW_CFAOpCode_AdvanceLoc1 = 0x2,
DW_CFAOpCode_AdvanceLoc2 = 0x3,
DW_CFAOpCode_AdvanceLoc4 = 0x4,
DW_CFAOpCode_OffsetExt = 0x5,
DW_CFAOpCode_RestoreExt = 0x6,
DW_CFAOpCode_Undefined = 0x7,
DW_CFAOpCode_SameValue = 0x8,
DW_CFAOpCode_Register = 0x9,
DW_CFAOpCode_RememberState = 0xa,
DW_CFAOpCode_RestoreState = 0xb,
DW_CFAOpCode_DefCfa = 0xc,
DW_CFAOpCode_DefCfaRegister = 0xd,
DW_CFAOpCode_DefCfaOffset = 0xe,
DW_CFAOpCode_DefCfaExpr = 0xf,
DW_CFAOpCode_Expr = 0x10,
DW_CFAOpCode_OffsetExtSf = 0x11,
DW_CFAOpCode_DefCfaSf = 0x12,
DW_CFAOpCode_DefCfaOffsetSf = 0x13,
DW_CFAOpCode_ValOffset = 0x14,
DW_CFAOpCode_ValOffsetSf = 0x15,
DW_CFAOpCode_ValExpr = 0x16,
DW_CFAOpCode_AdvanceLoc = 0x40,
DW_CFAOpCode_Offset = 0x80,
DW_CFAOpCode_Restore = 0xc0,
} DW_CFAOpCodeEnum;

typedef enum DW_ExprOperandKind
{
DW_ExprOperandKind_Null,
DW_ExprOperandKind_U8,
DW_ExprOperandKind_U16,
DW_ExprOperandKind_U32,
DW_ExprOperandKind_U64,
DW_ExprOperandKind_S8,
DW_ExprOperandKind_S16,
DW_ExprOperandKind_S32,
DW_ExprOperandKind_S64,
DW_ExprOperandKind_ULEB128,
DW_ExprOperandKind_SLEB128,
DW_ExprOperandKind_Addr,
DW_ExprOperandKind_Block,
DW_ExprOperandKind_DwarfUInt,
} DW_ExprOperandKind;

typedef U8 DW_ExprOp;
typedef enum DW_ExprOpEnum
{
DW_ExprOp_Null = 0x00,
DW_ExprOp_Addr = 0x03,
DW_ExprOp_Deref = 0x06,
DW_ExprOp_Const1U = 0x08,
DW_ExprOp_Const1S = 0x09,
DW_ExprOp_Const2U = 0x0a,
DW_ExprOp_Const2S = 0x0b,
DW_ExprOp_Const4U = 0x0c,
DW_ExprOp_Const4S = 0x0d,
DW_ExprOp_Const8U = 0x0e,
DW_ExprOp_Const8S = 0x0f,
DW_ExprOp_ConstU = 0x10,
DW_ExprOp_ConstS = 0x11,
DW_ExprOp_Dup = 0x12,
DW_ExprOp_Drop = 0x13,
DW_ExprOp_Over = 0x14,
DW_ExprOp_Pick = 0x15,
DW_ExprOp_Swap = 0x16,
DW_ExprOp_Rot = 0x17,
DW_ExprOp_XDeref = 0x18,
DW_ExprOp_Abs = 0x19,
DW_ExprOp_And = 0x1a,
DW_ExprOp_Div = 0x1b,
DW_ExprOp_Minus = 0x1c,
DW_ExprOp_Mod = 0x1d,
DW_ExprOp_Mul = 0x1e,
DW_ExprOp_Neg = 0x1f,
DW_ExprOp_Not = 0x20,
DW_ExprOp_Or = 0x21,
DW_ExprOp_Plus = 0x22,
DW_ExprOp_PlusUConst = 0x23,
DW_ExprOp_Shl = 0x24,
DW_ExprOp_Shr = 0x25,
DW_ExprOp_Shra = 0x26,
DW_ExprOp_Xor = 0x27,
DW_ExprOp_Bra = 0x28,
DW_ExprOp_Eq = 0x29,
DW_ExprOp_Ge = 0x2a,
DW_ExprOp_Gt = 0x2b,
DW_ExprOp_Le = 0x2c,
DW_ExprOp_Lt = 0x2d,
DW_ExprOp_Ne = 0x2e,
DW_ExprOp_Skip = 0x2f,
DW_ExprOp_Lit0 = 0x30,
DW_ExprOp_Lit1 = 0x31,
DW_ExprOp_Lit2 = 0x32,
DW_ExprOp_Lit3 = 0x33,
DW_ExprOp_Lit4 = 0x34,
DW_ExprOp_Lit5 = 0x35,
DW_ExprOp_Lit6 = 0x36,
DW_ExprOp_Lit7 = 0x37,
DW_ExprOp_Lit8 = 0x38,
DW_ExprOp_Lit9 = 0x39,
DW_ExprOp_Lit10 = 0x3a,
DW_ExprOp_Lit11 = 0x3b,
DW_ExprOp_Lit12 = 0x3c,
DW_ExprOp_Lit13 = 0x3d,
DW_ExprOp_Lit14 = 0x3e,
DW_ExprOp_Lit15 = 0x3f,
DW_ExprOp_Lit16 = 0x40,
DW_ExprOp_Lit17 = 0x41,
DW_ExprOp_Lit18 = 0x42,
DW_ExprOp_Lit19 = 0x43,
DW_ExprOp_Lit20 = 0x44,
DW_ExprOp_Lit21 = 0x45,
DW_ExprOp_Lit22 = 0x46,
DW_ExprOp_Lit23 = 0x47,
DW_ExprOp_Lit24 = 0x48,
DW_ExprOp_Lit25 = 0x49,
DW_ExprOp_Lit26 = 0x4a,
DW_ExprOp_Lit27 = 0x4b,
DW_ExprOp_Lit28 = 0x4c,
DW_ExprOp_Lit29 = 0x4d,
DW_ExprOp_Lit30 = 0x4e,
DW_ExprOp_Lit31 = 0x4f,
DW_ExprOp_Reg0 = 0x50,
DW_ExprOp_Reg1 = 0x51,
DW_ExprOp_Reg2 = 0x52,
DW_ExprOp_Reg3 = 0x53,
DW_ExprOp_Reg4 = 0x54,
DW_ExprOp_Reg5 = 0x55,
DW_ExprOp_Reg6 = 0x56,
DW_ExprOp_Reg7 = 0x57,
DW_ExprOp_Reg8 = 0x58,
DW_ExprOp_Reg9 = 0x59,
DW_ExprOp_Reg10 = 0x5a,
DW_ExprOp_Reg11 = 0x5b,
DW_ExprOp_Reg12 = 0x5c,
DW_ExprOp_Reg13 = 0x5d,
DW_ExprOp_Reg14 = 0x5e,
DW_ExprOp_Reg15 = 0x5f,
DW_ExprOp_Reg16 = 0x60,
DW_ExprOp_Reg17 = 0x61,
DW_ExprOp_Reg18 = 0x62,
DW_ExprOp_Reg19 = 0x63,
DW_ExprOp_Reg20 = 0x64,
DW_ExprOp_Reg21 = 0x65,
DW_ExprOp_Reg22 = 0x66,
DW_ExprOp_Reg23 = 0x67,
DW_ExprOp_Reg24 = 0x68,
DW_ExprOp_Reg25 = 0x69,
DW_ExprOp_Reg26 = 0x6a,
DW_ExprOp_Reg27 = 0x6b,
DW_ExprOp_Reg28 = 0x6c,
DW_ExprOp_Reg29 = 0x6d,
DW_ExprOp_Reg30 = 0x6e,
DW_ExprOp_Reg31 = 0x6f,
DW_ExprOp_BReg0 = 0x70,
DW_ExprOp_BReg1 = 0x71,
DW_ExprOp_BReg2 = 0x72,
DW_ExprOp_BReg3 = 0x73,
DW_ExprOp_BReg4 = 0x74,
DW_ExprOp_BReg5 = 0x75,
DW_ExprOp_BReg6 = 0x76,
DW_ExprOp_BReg7 = 0x77,
DW_ExprOp_BReg8 = 0x78,
DW_ExprOp_BReg9 = 0x79,
DW_ExprOp_BReg10 = 0x7a,
DW_ExprOp_BReg11 = 0x7b,
DW_ExprOp_BReg12 = 0x7c,
DW_ExprOp_BReg13 = 0x7d,
DW_ExprOp_BReg14 = 0x7e,
DW_ExprOp_BReg15 = 0x7f,
DW_ExprOp_BReg16 = 0x80,
DW_ExprOp_BReg17 = 0x81,
DW_ExprOp_BReg18 = 0x82,
DW_ExprOp_BReg19 = 0x83,
DW_ExprOp_BReg20 = 0x84,
DW_ExprOp_BReg21 = 0x85,
DW_ExprOp_BReg22 = 0x86,
DW_ExprOp_BReg23 = 0x87,
DW_ExprOp_BReg24 = 0x88,
DW_ExprOp_BReg25 = 0x89,
DW_ExprOp_BReg26 = 0x8a,
DW_ExprOp_BReg27 = 0x8b,
DW_ExprOp_BReg28 = 0x8c,
DW_ExprOp_BReg29 = 0x8d,
DW_ExprOp_BReg30 = 0x8e,
DW_ExprOp_BReg31 = 0x8f,
DW_ExprOp_RegX = 0x90,
DW_ExprOp_FBReg = 0x91,
DW_ExprOp_BRegX = 0x92,
DW_ExprOp_Piece = 0x93,
DW_ExprOp_DerefSize = 0x94,
DW_ExprOp_XDerefSize = 0x95,
DW_ExprOp_Nop = 0x96,
DW_ExprOp_PushObjectAddress = 0x97,
DW_ExprOp_Call2 = 0x98,
DW_ExprOp_Call4 = 0x99,
DW_ExprOp_CallRef = 0x9a,
DW_ExprOp_FormTlsAddress = 0x9b,
DW_ExprOp_CallFrameCfa = 0x9c,
DW_ExprOp_BitPiece = 0x9d,
DW_ExprOp_ImplicitValue = 0x9e,
DW_ExprOp_StackValue = 0x9f,
DW_ExprOp_ImplicitPointer = 0xa0,
DW_ExprOp_Addrx = 0xa1,
DW_ExprOp_Constx = 0xa2,
DW_ExprOp_EntryValue = 0xa3,
DW_ExprOp_ConstType = 0xa4,
DW_ExprOp_RegvalType = 0xa5,
DW_ExprOp_DerefType = 0xa6,
DW_ExprOp_XDerefType = 0xa7,
DW_ExprOp_Convert = 0xa8,
DW_ExprOp_Reinterpret = 0xa9,
} DW_ExprOpEnum;

typedef U8 DW_GNU_ExprOp;
typedef enum DW_GNU_ExprOpEnum
{
DW_GNU_ExprOp_PushTlsAddress = 0xe0,
DW_GNU_ExprOp_UnInit = 0xf0,
DW_GNU_ExprOp_ImplicitPointer = 0xf2,
DW_GNU_ExprOp_EntryValue = 0xf3,
DW_GNU_ExprOp_ConstType = 0xf4,
DW_GNU_ExprOp_RegvalType = 0xf5,
DW_GNU_ExprOp_DerefType = 0xf6,
DW_GNU_ExprOp_Convert = 0xf7,
DW_GNU_ExprOp_ParameterRef = 0xfa,
DW_GNU_ExprOp_AddrIndex = 0xfb,
DW_GNU_ExprOp_ConstIndex = 0xfc,
} DW_GNU_ExprOpEnum;

typedef enum DW_RegCodeX64
{
DW_RegCodeX64_rax = 0,
DW_RegCodeX64_rdx = 1,
DW_RegCodeX64_rcx = 2,
DW_RegCodeX64_rbx = 3,
DW_RegCodeX64_rsi = 4,
DW_RegCodeX64_rdi = 5,
DW_RegCodeX64_rbp = 6,
DW_RegCodeX64_rsp = 7,
DW_RegCodeX64_r8 = 8,
DW_RegCodeX64_r9 = 9,
DW_RegCodeX64_r10 = 10,
DW_RegCodeX64_r11 = 11,
DW_RegCodeX64_r12 = 12,
DW_RegCodeX64_r13 = 13,
DW_RegCodeX64_r14 = 14,
DW_RegCodeX64_r15 = 15,
DW_RegCodeX64_rip = 16,
DW_RegCodeX64_zmm0 = 17,
DW_RegCodeX64_zmm1 = 18,
DW_RegCodeX64_zmm2 = 19,
DW_RegCodeX64_zmm3 = 20,
DW_RegCodeX64_zmm4 = 21,
DW_RegCodeX64_zmm5 = 22,
DW_RegCodeX64_zmm6 = 23,
DW_RegCodeX64_zmm7 = 24,
DW_RegCodeX64_zmm8 = 25,
DW_RegCodeX64_zmm9 = 26,
DW_RegCodeX64_zmm10 = 27,
DW_RegCodeX64_zmm11 = 28,
DW_RegCodeX64_zmm12 = 29,
DW_RegCodeX64_zmm13 = 30,
DW_RegCodeX64_zmm14 = 31,
DW_RegCodeX64_zmm15 = 32,
DW_RegCodeX64_st0 = 33,
DW_RegCodeX64_st1 = 34,
DW_RegCodeX64_st2 = 35,
DW_RegCodeX64_st3 = 36,
DW_RegCodeX64_st4 = 37,
DW_RegCodeX64_st5 = 38,
DW_RegCodeX64_st6 = 39,
DW_RegCodeX64_st7 = 40,
DW_RegCodeX64_mm0 = 41,
DW_RegCodeX64_mm1 = 42,
DW_RegCodeX64_mm2 = 43,
DW_RegCodeX64_mm3 = 44,
DW_RegCodeX64_mm4 = 45,
DW_RegCodeX64_mm5 = 46,
DW_RegCodeX64_mm6 = 47,
DW_RegCodeX64_mm7 = 48,
DW_RegCodeX64_rflags = 49,
DW_RegCodeX64_es = 50,
DW_RegCodeX64_cs = 51,
DW_RegCodeX64_ss = 52,
DW_RegCodeX64_ds = 53,
DW_RegCodeX64_fs = 54,
DW_RegCodeX64_gs = 55,
DW_RegCodeX64_fsbase = 58,
DW_RegCodeX64_gsbase = 59,
DW_RegCodeX64_zmm16 = 67,
DW_RegCodeX64_zmm17 = 68,
DW_RegCodeX64_zmm18 = 69,
DW_RegCodeX64_zmm19 = 70,
DW_RegCodeX64_zmm20 = 71,
DW_RegCodeX64_zmm21 = 72,
DW_RegCodeX64_zmm22 = 73,
DW_RegCodeX64_zmm23 = 74,
DW_RegCodeX64_zmm24 = 75,
DW_RegCodeX64_zmm25 = 76,
DW_RegCodeX64_zmm26 = 77,
DW_RegCodeX64_zmm27 = 78,
DW_RegCodeX64_zmm28 = 79,
DW_RegCodeX64_zmm29 = 80,
DW_RegCodeX64_zmm30 = 81,
DW_RegCodeX64_zmm31 = 82,
DW_RegCodeX64_LAST,
} DW_RegCodeX64;

typedef enum DW_RegCodeARM64
{
DW_RegCodeARM64_x0 = 0,
DW_RegCodeARM64_x1 = 1,
DW_RegCodeARM64_x2 = 2,
DW_RegCodeARM64_x3 = 3,
DW_RegCodeARM64_x4 = 4,
DW_RegCodeARM64_x5 = 5,
DW_RegCodeARM64_x6 = 6,
DW_RegCodeARM64_x7 = 7,
DW_RegCodeARM64_x8 = 8,
DW_RegCodeARM64_x9 = 9,
DW_RegCodeARM64_x10 = 10,
DW_RegCodeARM64_x11 = 11,
DW_RegCodeARM64_x12 = 12,
DW_RegCodeARM64_x13 = 13,
DW_RegCodeARM64_x14 = 14,
DW_RegCodeARM64_x15 = 15,
DW_RegCodeARM64_x16 = 16,
DW_RegCodeARM64_x17 = 17,
DW_RegCodeARM64_x18 = 18,
DW_RegCodeARM64_x19 = 19,
DW_RegCodeARM64_x20 = 20,
DW_RegCodeARM64_x21 = 21,
DW_RegCodeARM64_x22 = 22,
DW_RegCodeARM64_x23 = 23,
DW_RegCodeARM64_x24 = 24,
DW_RegCodeARM64_x25 = 25,
DW_RegCodeARM64_x26 = 26,
DW_RegCodeARM64_x27 = 27,
DW_RegCodeARM64_x28 = 28,
DW_RegCodeARM64_fp = 29,
DW_RegCodeARM64_lr = 30,
DW_RegCodeARM64_sp = 31,
DW_RegCodeARM64_pc = 32,
DW_RegCodeARM64_elr_mode = 33,
DW_RegCodeARM64_ra_sign_state = 34,
DW_RegCodeARM64_tpidrro_elo = 35,
DW_RegCodeARM64_tpidr_elo = 36,
DW_RegCodeARM64_tpidr_el1 = 37,
DW_RegCodeARM64_tpidr_el2 = 38,
DW_RegCodeARM64_tpidr_el3 = 39,
DW_RegCodeARM64_v0 = 64,
DW_RegCodeARM64_v1 = 65,
DW_RegCodeARM64_v2 = 66,
DW_RegCodeARM64_v3 = 67,
DW_RegCodeARM64_v4 = 68,
DW_RegCodeARM64_v5 = 69,
DW_RegCodeARM64_v6 = 70,
DW_RegCodeARM64_v7 = 71,
DW_RegCodeARM64_v8 = 72,
DW_RegCodeARM64_v9 = 73,
DW_RegCodeARM64_v10 = 74,
DW_RegCodeARM64_v11 = 75,
DW_RegCodeARM64_v12 = 76,
DW_RegCodeARM64_v13 = 77,
DW_RegCodeARM64_v14 = 78,
DW_RegCodeARM64_v15 = 79,
DW_RegCodeARM64_v16 = 80,
DW_RegCodeARM64_v17 = 81,
DW_RegCodeARM64_v18 = 82,
DW_RegCodeARM64_v19 = 83,
DW_RegCodeARM64_v20 = 84,
DW_RegCodeARM64_v21 = 85,
DW_RegCodeARM64_v22 = 86,
DW_RegCodeARM64_v23 = 87,
DW_RegCodeARM64_v24 = 88,
DW_RegCodeARM64_v25 = 89,
DW_RegCodeARM64_v26 = 90,
DW_RegCodeARM64_v27 = 91,
DW_RegCodeARM64_v28 = 92,
DW_RegCodeARM64_v29 = 93,
DW_RegCodeARM64_v30 = 94,
DW_RegCodeARM64_v31 = 95,
DW_RegCodeARM64_LAST,
} DW_RegCodeARM64;

#define DW_Format_XList \
X(Null, "", 0)\
X(32Bit, "DWARF32", 4)\
X(64Bit, "DWARF64", 8)\

#define DW_SectionKind_XList \
X(Null, "", "", "")\
X(Abbrev, ".debug_abbrev", "__debug_abbrev", ".debug_abbrev.dwo")\
X(ARanges, ".debug_aranges", "__debug_aranges", ".debug_aranges.dwo")\
X(Frame, ".debug_frame", "__debug_frame", ".debug_frame.dwo")\
X(Info, ".debug_info", "__debug_info", ".debug_info.dwo")\
X(Line, ".debug_line", "__debug_line", ".debug_line.dwo")\
X(Loc, ".debug_loc", "__debug_loc", ".debug_loc.dwo")\
X(MacInfo, ".debug_macinfo", "__debug_macinfo", ".debug_macinfo.dwo")\
X(PubNames, ".debug_pubnames", "__debug_pubnames", ".debug_pubnames.dwo")\
X(PubTypes, ".debug_pubtypes", "__debug_pubtypes", ".debug_pubtypes.dwo")\
X(Ranges, ".debug_ranges", "__debug_ranges", ".debug_ranges.dwo")\
X(Str, ".debug_str", "__debug_str", ".debug_str.dwo")\
X(Addr, ".debug_addr", "__debug_addr", ".debug_addr.dwo")\
X(LocLists, ".debug_loclists", "__debug_loclists", ".debug_loclists.dwo")\
X(RngLists, ".debug_rnglists", "__debug_rnglists", ".debug_rnglists.dwo")\
X(StrOffsets, ".debug_str_offsets", "__debug_str_offsets", ".debug_str_offsets.dwo")\
X(LineStr, ".debug_line_str", "__debug_line_str", ".debug_line_str.dwo")\
X(Names, ".debug_names", "__debug_names", ".debug_names.dwo")\

#define DW_Language_XList \
X(Null, 0x00, 0)\
X(C89, 0x01, 0)\
X(C, 0x02, 0)\
X(Ada83, 0x03, 0)\
X(CPlusPlus, 0x04, 0)\
X(Cobol74, 0x05, 1)\
X(Cobol85, 0x06, 1)\
X(Fortran77, 0x07, 1)\
X(Fortran90, 0x08, 1)\
X(Pascal83, 0x09, 1)\
X(Modula2, 0x0A, 1)\
X(Java, 0x0B, 0)\
X(C99, 0x0C, 0)\
X(Ada95, 0x0D, 1)\
X(Fortran95, 0x0E, 1)\
X(PLI, 0x0F, 1)\
X(ObjC, 0x10, 0)\
X(ObjCPlusPlus, 0x11, 0)\
X(UPC, 0x12, 0)\
X(D, 0x13, 0)\
X(Python, 0x14, 0)\
X(OpenCL, 0x15, 0)\
X(Go, 0x16, 0)\
X(Modula3, 0x17, 1)\
X(Haskell, 0x18, 0)\
X(CPlusPlus03, 0x19, 0)\
X(CPlusPlus11, 0x1a, 0)\
X(OCaml, 0x1b, 0)\
X(Rust, 0x1c, 0)\
X(C11, 0x1d, 0)\
X(Swift, 0x1e, 0)\
X(Julia, 0x1f, 1)\
X(Dylan, 0x20, 0)\
X(CPlusPlus14, 0x21, 0)\
X(Fortran03, 0x22, 1)\
X(Fortran08, 0x23, 1)\
X(RenderScript, 0x24, 0)\
X(BLISS, 0x25, 0)\
X(MipsAssembler, 0x8001, 0)\
X(GoogleRenderScript, 0x8E57, 0)\
X(SunAssembler, 0x9001, 0)\
X(BorlandDelphi, 0xB000, 0)\

#define DW_InlKind_XList \
X(NotInlined, 0x00)\
X(Inlined, 0x01)\
X(DeclaredNotInlined, 0x02)\
X(DeclaredInlined, 0x03)\

#define DW_StdOpcode_XList \
X(ExtendedOpcode, 0x00, 0)\
X(Copy, 0x01, 0)\
X(AdvancePc, 0x02, 1)\
X(AdvanceLine, 0x03, 1)\
X(SetFile, 0x04, 1)\
X(SetColumn, 0x05, 1)\
X(NegateStmt, 0x06, 0)\
X(SetBasicBlock, 0x07, 0)\
X(ConstAddPc, 0x08, 0)\
X(FixedAdvancePc, 0x09, 1)\
X(SetPrologueEnd, 0x0A, 0)\
X(SetEpilogueBegin, 0x0B, 0)\
X(SetIsa, 0x0C, 1)\

#define DW_ExtOpcode_XList \
X(Undefined, 0x00)\
X(EndSequence, 0x01)\
X(SetAddress, 0x02)\
X(DefineFile, 0x03)\
X(SetDiscriminator, 0x04)\
X(UserLo, 0x80)\
X(UserHi, 0xff)\

#define DW_TagKind_XList \
X(Null, 0, DW_Version_Null)\
X(ArrayType, 0x01, DW_Version_3)\
X(ClassType, 0x02, DW_Version_3)\
X(EntryPoint, 0x03, DW_Version_3)\
X(EnumerationType, 0x04, DW_Version_3)\
X(FormalParameter, 0x05, DW_Version_3)\
X(ImportedDeclaration, 0x08, DW_Version_3)\
X(Label, 0x0a, DW_Version_3)\
X(LexicalBlock, 0x0b, DW_Version_3)\
X(Member, 0x0d, DW_Version_3)\
X(PointerType, 0x0f, DW_Version_3)\
X(ReferenceType, 0x10, DW_Version_3)\
X(CompileUnit, 0x11, DW_Version_3)\
X(StringType, 0x12, DW_Version_3)\
X(StructureType, 0x13, DW_Version_3)\
X(SubroutineType, 0x15, DW_Version_3)\
X(Typedef, 0x16, DW_Version_3)\
X(UnionType, 0x17, DW_Version_3)\
X(UnspecifiedParameters, 0x18, DW_Version_3)\
X(Variant, 0x19, DW_Version_3)\
X(CommonBlock, 0x1a, DW_Version_3)\
X(CommonInclusion, 0x1b, DW_Version_3)\
X(Inheritance, 0x1c, DW_Version_3)\
X(InlinedSubroutine, 0x1d, DW_Version_3)\
X(Module, 0x1e, DW_Version_3)\
X(PtrToMemberType, 0x1f, DW_Version_3)\
X(SetType, 0x20, DW_Version_3)\
X(SubrangeType, 0x21, DW_Version_3)\
X(WithStmt, 0x22, DW_Version_3)\
X(AccessDeclaration, 0x23, DW_Version_3)\
X(BaseType, 0x24, DW_Version_3)\
X(CatchBlock, 0x25, DW_Version_3)\
X(ConstType, 0x26, DW_Version_3)\
X(Constant, 0x27, DW_Version_3)\
X(Enumerator, 0x28, DW_Version_3)\
X(FileType, 0x29, DW_Version_3)\
X(Friend, 0x2a, DW_Version_3)\
X(NameList, 0x2b, DW_Version_3)\
X(NameListItem, 0x2c, DW_Version_3)\
X(PackedType, 0x2d, DW_Version_3)\
X(SubProgram, 0x2e, DW_Version_3)\
X(TemplateTypeParameter, 0x2f, DW_Version_3)\
X(TemplateValueParameter, 0x30, DW_Version_3)\
X(ThrownType, 0x31, DW_Version_3)\
X(TryBlock, 0x32, DW_Version_3)\
X(VariantPart, 0x33, DW_Version_3)\
X(Variable, 0x34, DW_Version_3)\
X(VolatileType, 0x35, DW_Version_3)\
X(DwarfProcedure, 0x36, DW_Version_3)\
X(RestrictType, 0x37, DW_Version_3)\
X(InterfaceType, 0x38, DW_Version_3)\
X(Namespace, 0x39, DW_Version_3)\
X(ImportedModule, 0x3a, DW_Version_3)\
X(UnspecifiedType, 0x3b, DW_Version_3)\
X(PartialUnit, 0x3c, DW_Version_3)\
X(ImportedUnit, 0x3d, DW_Version_3)\
X(Condition, 0x3f, DW_Version_3)\
X(SharedType, 0x40, DW_Version_3)\
X(TypeUnit, 0x41, DW_Version_5)\
X(RValueReferenceType, 0x42, DW_Version_5)\
X(TemplateAlias, 0x43, DW_Version_5)\
X(CoarrayType, 0x44, DW_Version_5)\
X(GenericSubrange, 0x45, DW_Version_5)\
X(DynamicType, 0x46, DW_Version_5)\
X(AtomicType, 0x47, DW_Version_5)\
X(CallSite, 0x48, DW_Version_5)\
X(CallSiteParameter, 0x49, DW_Version_5)\
X(SkeletonUnit, 0x4A, DW_Version_5)\
X(ImmutableType, 0x4B, DW_Version_5)\

#define DW_GNU_TagKind_XList \
X(CallSite, 0x4109)\
X(CallSiteParameter, 0x410a)\

#define DW_FormKind_XList \
X(Null, 0, DW_Version_0, DW_AttribClass_Null)\
X(Addr, 0x1, DW_Version_2, DW_AttribClass_Address)\
X(Block2, 0x3, DW_Version_2, DW_AttribClass_Block)\
X(Block4, 0x4, DW_Version_2, DW_AttribClass_Block)\
X(Data2, 0x5, DW_Version_2, DW_AttribClass_Const)\
X(Data4, 0x6, DW_Version_2, DW_AttribClass_Const)\
X(Data8, 0x7, DW_Version_2, DW_AttribClass_Const)\
X(String, 0x8, DW_Version_2, DW_AttribClass_String)\
X(Block, 0x9, DW_Version_2, DW_AttribClass_Block)\
X(Block1, 0xa, DW_Version_2, DW_AttribClass_Block)\
X(Data1, 0xb, DW_Version_2, DW_AttribClass_Const)\
X(Flag, 0xc, DW_Version_2, DW_AttribClass_Flag)\
X(SData, 0xd, DW_Version_2, DW_AttribClass_Const)\
X(Strp, 0xe, DW_Version_2, DW_AttribClass_String)\
X(UData, 0xf, DW_Version_2, DW_AttribClass_Const)\
X(RefAddr, 0x10, DW_Version_2, DW_AttribClass_Reference)\
X(Ref1, 0x11, DW_Version_2, DW_AttribClass_Reference)\
X(Ref2, 0x12, DW_Version_2, DW_AttribClass_Reference)\
X(Ref4, 0x13, DW_Version_2, DW_AttribClass_Reference)\
X(Ref8, 0x14, DW_Version_2, DW_AttribClass_Reference)\
X(RefUData, 0x15, DW_Version_2, DW_AttribClass_Reference)\
X(Indirect, 0x16, DW_Version_2, DW_AttribClass_Null)\
X(SecOffset, 0x17, DW_Version_4, DW_AttribClass_AllPtrs)\
X(ExprLoc, 0x18, DW_Version_4, DW_AttribClass_ExprLoc)\
X(FlagPresent, 0x19, DW_Version_4, DW_AttribClass_Flag)\
X(RefSig8, 0x20, DW_Version_4, DW_AttribClass_Reference)\
X(Strx, 0x1a, DW_Version_5, DW_AttribClass_String)\
X(Addrx, 0x1b, DW_Version_5, DW_AttribClass_String)\
X(RefSup4, 0x1c, DW_Version_5, DW_AttribClass_Reference)\
X(StrpSup, 0x1d, DW_Version_5, DW_AttribClass_String)\
X(Data16, 0x1e, DW_Version_5, DW_AttribClass_Const)\
X(LineStrp, 0x1f, DW_Version_5, DW_AttribClass_String)\
X(ImplicitConst, 0x21, DW_Version_5, DW_AttribClass_Const)\
X(LocListx, 0x22, DW_Version_5, DW_AttribClass_LocListPtr)\
X(RngListx, 0x23, DW_Version_5, DW_AttribClass_RngListPtr)\
X(RefSup8, 0x24, DW_Version_5, DW_AttribClass_Reference)\
X(Strx1, 0x25, DW_Version_5, DW_AttribClass_String)\
X(Strx2, 0x26, DW_Version_5, DW_AttribClass_String)\
X(Strx3, 0x27, DW_Version_5, DW_AttribClass_String)\
X(Strx4, 0x28, DW_Version_5, DW_AttribClass_String)\
X(Addrx1, 0x29, DW_Version_5, DW_AttribClass_Address)\
X(Addrx2, 0x2a, DW_Version_5, DW_AttribClass_Address)\
X(Addrx3, 0x2b, DW_Version_5, DW_AttribClass_Address)\
X(Addrx4, 0x2c, DW_Version_5, DW_AttribClass_Address)\

#define DW_GNU_FormKind_XList \
X(AddrIndex, 0x1f01, DW_AttribClass_Undefined)\
X(StrIndex, 0x1f02, DW_AttribClass_Undefined)\
X(RefAlt, 0x1f20, DW_AttribClass_Undefined)\
X(StrpAlt, 0x1f21, DW_AttribClass_String)\

#define DW_AttribKind_XList \
X(Null, 0, DW_Version_Null, 0)\
X(Sibling, 0x1, DW_Version_2, DW_AttribClass_Reference)\
X(Location, 0x2, DW_Version_2, DW_AttribClass_Block|DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)\
X(Name, 0x3, DW_Version_2, DW_AttribClass_String)\
X(Ordering, 0x9, DW_Version_2, DW_AttribClass_Const)\
X(ByteSize, 0xB, DW_Version_2, DW_AttribClass_Const)\
X(BitOffset, 0xC, DW_Version_2, DW_AttribClass_Const)\
X(BitSize, 0xD, DW_Version_2, DW_AttribClass_Const)\
X(StmtList, 0x10, DW_Version_2, DW_AttribClass_Const)\
X(LowPc, 0x11, DW_Version_2, DW_AttribClass_Address)\
X(HighPc, 0x12, DW_Version_2, DW_AttribClass_Address)\
X(Language, 0x13, DW_Version_2, DW_AttribClass_Const)\
X(Discr, 0x15, DW_Version_2, DW_AttribClass_Reference)\
X(DiscrValue, 0x16, DW_Version_2, DW_AttribClass_Const)\
X(Visibility, 0x17, DW_Version_2, DW_AttribClass_Const)\
X(Import, 0x18, DW_Version_2, DW_AttribClass_Reference)\
X(StringLength, 0x19, DW_Version_2, DW_AttribClass_Block|DW_AttribClass_Const)\
X(CommonReference, 0x1a, DW_Version_2, DW_AttribClass_Reference)\
X(CompDir, 0x1b, DW_Version_2, DW_AttribClass_String)\
X(ConstValue, 0x1c, DW_Version_2, DW_AttribClass_String|DW_AttribClass_Const|DW_AttribClass_Block)\
X(ContainingType, 0x1d, DW_Version_2, DW_AttribClass_Reference)\
X(DefaultValue, 0x1e, DW_Version_2, DW_AttribClass_Reference)\
X(Inline, 0x20, DW_Version_2, DW_AttribClass_Const)\
X(IsOptional, 0x21, DW_Version_2, DW_AttribClass_Flag)\
X(LowerBound, 0x22, DW_Version_2, DW_AttribClass_Const|DW_AttribClass_Reference)\
X(Producer, 0x25, DW_Version_2, DW_AttribClass_String)\
X(Prototyped, 0x27, DW_Version_2, DW_AttribClass_Flag)\
X(ReturnAddr, 0x2a, DW_Version_2, DW_AttribClass_Block|DW_AttribClass_Const)\
X(StartScope, 0x2c, DW_Version_2, DW_AttribClass_Const)\
X(BitStride, 0x2e, DW_Version_2, DW_AttribClass_Const)\
X(UpperBound, 0x2f, DW_Version_2, DW_AttribClass_Const|DW_AttribClass_Reference)\
X(AbstractOrigin, 0x31, DW_Version_2, DW_AttribClass_Reference)\
X(Accessibility, 0x32, DW_Version_2, DW_AttribClass_Const)\
X(AddressClass, 0x33, DW_Version_2, DW_AttribClass_Const)\
X(Artificial, 0x34, DW_Version_2, DW_AttribClass_Flag)\
X(BaseTypes, 0x35, DW_Version_2, DW_AttribClass_Reference)\
X(CallingConvention, 0x36, DW_Version_2, DW_AttribClass_Const)\
X(Count, 0x37, DW_Version_2, DW_AttribClass_Const|DW_AttribClass_Reference)\
X(DataMemberLocation, 0x38, DW_Version_2, DW_AttribClass_Block|DW_AttribClass_Reference)\
X(DeclColumn, 0x39, DW_Version_2, DW_AttribClass_Const)\
X(DeclFile, 0x3a, DW_Version_2, DW_AttribClass_Const)\
X(DeclLine, 0x3b, DW_Version_2, DW_AttribClass_Const)\
X(Declaration, 0x3c, DW_Version_2, DW_AttribClass_Flag)\
X(DiscrList, 0x3d, DW_Version_2, DW_AttribClass_Block)\
X(Encoding, 0x3e, DW_Version_2, DW_AttribClass_Const)\
X(External, 0x3f, DW_Version_2, DW_AttribClass_Flag)\
X(FrameBase, 0x40, DW_Version_2, DW_AttribClass_Block|DW_AttribClass_Const)\
X(Friend, 0x41, DW_Version_2, DW_AttribClass_Reference)\
X(IdentifierCase, 0x42, DW_Version_2, DW_AttribClass_Const)\
X(MacroInfo, 0x43, DW_Version_2, DW_AttribClass_Const)\
X(NameListItem, 0x44, DW_Version_2, DW_AttribClass_Block)\
X(Priority, 0x45, DW_Version_2, DW_AttribClass_Reference)\
X(Segment, 0x46, DW_Version_2, DW_AttribClass_Block|DW_AttribClass_Const)\
X(Specification, 0x47, DW_Version_2, DW_AttribClass_Reference)\
X(StaticLink, 0x48, DW_Version_2, DW_AttribClass_Block|DW_AttribClass_Const)\
X(Type, 0x49, DW_Version_2, DW_AttribClass_Reference)\
X(UseLocation, 0x4a, DW_Version_2, DW_AttribClass_Block|DW_AttribClass_Const)\
X(VariableParameter, 0x4b, DW_Version_2, DW_AttribClass_Flag)\
X(Virtuality, 0x4c, DW_Version_2, DW_AttribClass_Const)\
X(VTableElemLocation, 0x4d, DW_Version_2, DW_AttribClass_Block|DW_AttribClass_Reference)\
X(Allocated, 0x4e, DW_Version_3, DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)\
X(Associated, 0x4f, DW_Version_3, DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)\
X(DataLocation, 0x50, DW_Version_3, DW_AttribClass_ExprLoc)\
X(ByteStride, 0x51, DW_Version_3, DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)\
X(EntryPc, 0x52, DW_Version_3, DW_AttribClass_Address|DW_AttribClass_Const)\
X(UseUtf8, 0x53, DW_Version_3, DW_AttribClass_Flag)\
X(Extension, 0x54, DW_Version_3, DW_AttribClass_Reference)\
X(Ranges, 0x55, DW_Version_3, DW_AttribClass_RngList)\
X(Trampoline, 0x56, DW_Version_3, DW_AttribClass_Address|DW_AttribClass_Flag|DW_AttribClass_Reference|DW_AttribClass_String)\
X(CallColumn, 0x57, DW_Version_3, DW_AttribClass_Const)\
X(CallFile, 0x58, DW_Version_3, DW_AttribClass_Const)\
X(CallLine, 0x59, DW_Version_3, DW_AttribClass_Const)\
X(Description, 0x5a, DW_Version_3, DW_AttribClass_String)\
X(BinaryScale, 0x5b, DW_Version_3, DW_AttribClass_Const)\
X(DecimalScale, 0x5c, DW_Version_3, DW_AttribClass_Const)\
X(Small, 0x5d, DW_Version_3, DW_AttribClass_Reference)\
X(DecimalSign, 0x5e, DW_Version_3, DW_AttribClass_Const)\
X(DigitCount, 0x5f, DW_Version_3, DW_AttribClass_Const)\
X(PictureString, 0x60, DW_Version_3, DW_AttribClass_String)\
X(Mutable, 0x61, DW_Version_3, DW_AttribClass_Flag)\
X(ThreadsScaled, 0x62, DW_Version_3, DW_AttribClass_Flag)\
X(Explicit, 0x63, DW_Version_3, DW_AttribClass_Flag)\
X(ObjectPointer, 0x64, DW_Version_3, DW_AttribClass_Reference)\
X(Endianity, 0x65, DW_Version_3, DW_AttribClass_Const)\
X(Elemental, 0x66, DW_Version_3, DW_AttribClass_Flag)\
X(Pure, 0x67, DW_Version_3, DW_AttribClass_Flag)\
X(Recursive, 0x68, DW_Version_3, DW_AttribClass_Flag)\
X(Signature, 0x69, DW_Version_4, DW_AttribClass_Reference)\
X(MainSubProgram, 0x6a, DW_Version_4, DW_AttribClass_Flag)\
X(DataBitOffset, 0x6b, DW_Version_4, DW_AttribClass_Const)\
X(ConstExpr, 0x6c, DW_Version_4, DW_AttribClass_Flag)\
X(EnumClass, 0x6d, DW_Version_4, DW_AttribClass_Flag)\
X(LinkageName, 0x6e, DW_Version_4, DW_AttribClass_String)\
X(StringLengthBitSize, 0x6f, DW_Version_5, DW_AttribClass_Const)\
X(StringLengthByteSize, 0x70, DW_Version_5, DW_AttribClass_Const)\
X(Rank, 0x71, DW_Version_5, DW_AttribClass_Const|DW_AttribClass_ExprLoc)\
X(StrOffsetsBase, 0x72, DW_Version_5, DW_AttribClass_StrOffsetsPtr)\
X(AddrBase, 0x73, DW_Version_5, DW_AttribClass_AddrPtr)\
X(RngListsBase, 0x74, DW_Version_5, DW_AttribClass_RngListPtr)\
X(DwoName, 0x76, DW_Version_5, DW_AttribClass_String)\
X(Reference, 0x77, DW_Version_5, DW_AttribClass_Flag)\
X(RValueReference, 0x78, DW_Version_5, DW_AttribClass_Flag)\
X(Macros, 0x79, DW_Version_5, DW_AttribClass_MacPtr)\
X(CallAllCalls, 0x7a, DW_Version_5, DW_AttribClass_Flag)\
X(CallAllSourceCalls, 0x7b, DW_Version_5, DW_AttribClass_Flag)\
X(CallAllTailCalls, 0x7c, DW_Version_5, DW_AttribClass_Flag)\
X(CallReturnPc, 0x7d, DW_Version_5, DW_AttribClass_Address)\
X(CallValue, 0x7e, DW_Version_5, DW_AttribClass_ExprLoc)\
X(CallOrigin, 0x7f, DW_Version_5, DW_AttribClass_ExprLoc)\
X(CallParameter, 0x80, DW_Version_5, DW_AttribClass_Reference)\
X(CallPc, 0x81, DW_Version_5, DW_AttribClass_Address)\
X(CallTailCall, 0x82, DW_Version_5, DW_AttribClass_Flag)\
X(CallTarget, 0x83, DW_Version_5, DW_AttribClass_ExprLoc)\
X(CallTargetClobbered, 0x84, DW_Version_5, DW_AttribClass_ExprLoc)\
X(CallDataLocation, 0x85, DW_Version_5, DW_AttribClass_ExprLoc)\
X(CallDataValue, 0x86, DW_Version_5, DW_AttribClass_ExprLoc)\
X(NoReturn, 0x87, DW_Version_5, DW_AttribClass_Flag)\
X(Alignment, 0x88, DW_Version_5, DW_AttribClass_Const)\
X(ExportSymbols, 0x89, DW_Version_5, DW_AttribClass_Flag)\
X(Deleted, 0x8a, DW_Version_5, DW_AttribClass_Flag)\
X(Defaulted, 0x8b, DW_Version_5, DW_AttribClass_Const)\
X(LocListsBase, 0x8c, DW_Version_5, DW_AttribClass_LocListPtr)\

#define DW_GNU_AttribKind_XList \
X(Vector, 0x2107, DW_AttribClass_Flag)\
X(GuardedBy, 0x2108, DW_AttribClass_Undefined)\
X(PtGuardedBy, 0x2109, DW_AttribClass_Undefined)\
X(Guarded, 0x210a, DW_AttribClass_Undefined)\
X(PtGuarded, 0x210b, DW_AttribClass_Undefined)\
X(LocksExcluded, 0x210c, DW_AttribClass_Undefined)\
X(ExclusiveLocksRequired, 0x210d, DW_AttribClass_Undefined)\
X(SharedLocksRequired, 0x210e, DW_AttribClass_Undefined)\
X(OdrSignature, 0x210f, DW_AttribClass_Undefined)\
X(TemplateName, 0x2110, DW_AttribClass_Undefined)\
X(CallSiteValue, 0x2111, DW_AttribClass_ExprLoc)\
X(CallSiteDataValue, 0x2112, DW_AttribClass_ExprLoc)\
X(CallSiteTarget, 0x2113, DW_AttribClass_ExprLoc)\
X(CallSiteTargetClobbered, 0x2114, DW_AttribClass_ExprLoc)\
X(TailCall, 0x2115, DW_AttribClass_Flag)\
X(AllTailCallsSites, 0x2116, DW_AttribClass_Flag)\
X(AllCallSites, 0x2117, DW_AttribClass_Flag)\
X(AllSourceCallSites, 0x2118, DW_AttribClass_Flag)\
X(Macros, 0x2119, DW_AttribClass_Flag)\
X(Deleted, 0x211a, DW_AttribClass_Undefined)\
X(DwoName, 0x2130, DW_AttribClass_String)\
X(DwoId, 0x2131, DW_AttribClass_Const)\
X(RangesBase, 0x2132, DW_AttribClass_Undefined)\
X(AddrBase, 0x2133, DW_AttribClass_AddrPtr)\
X(PubNames, 0x2134, DW_AttribClass_Flag)\
X(PubTypes, 0x2135, DW_AttribClass_Undefined)\
X(Discriminator, 0x2136, DW_AttribClass_Const)\
X(LocViews, 0x2137, DW_AttribClass_LocListPtr)\
X(EntryView, 0x2138, DW_AttribClass_Undefined)\
X(DescriptiveType, 0x2302, DW_AttribClass_Undefined)\
X(Numerator, 0x2303, DW_AttribClass_Undefined)\
X(Denominator, 0x2304, DW_AttribClass_Undefined)\
X(Bias, 0x2305, DW_AttribClass_Undefined)\

#define DW_LLVM_AttribKind_XList \
X(IncludePath, 0x3e00, DW_AttribClass_String)\
X(ConfigMacros, 0x3e01, DW_AttribClass_String)\
X(SysRoot, 0x3e02, DW_AttribClass_String)\
X(TagOffset, 0x3e03, DW_AttribClass_Undefined)\
X(ApiNotes, 0x3e07, DW_AttribClass_String)\

#define DW_Apple_AttribKind_XList \
X(Optimized, 0x3fe1, DW_AttribClass_Flag)\
X(Flags, 0x3fe2, DW_AttribClass_Flag)\
X(Isa, 0x3fe3, DW_AttribClass_Flag)\
X(Block, 0x3fe4, DW_AttribClass_Undefined)\
X(MajorRuntimeVers, 0x3fe5, DW_AttribClass_Undefined)\
X(RuntimeClass, 0x3fe6, DW_AttribClass_Undefined)\
X(OmitFramePtr, 0x3fe7, DW_AttribClass_Flag)\
X(PropertyName, 0x3fe8, DW_AttribClass_Undefined)\
X(PropertyGetter, 0x3fe9, DW_AttribClass_Undefined)\
X(PropertySetter, 0x3fea, DW_AttribClass_Undefined)\
X(PropertyAttribute, 0x3feb, DW_AttribClass_Undefined)\
X(ObjcCompleteType, 0x3fec, DW_AttribClass_Undefined)\
X(Property, 0x3fed, DW_AttribClass_Undefined)\
X(ObjDirect, 0x3fee, DW_AttribClass_Undefined)\
X(Sdk, 0x3fef, DW_AttribClass_String)\

#define DW_MIPS_AttribKind_XList \
X(Fde, 0x2001, DW_AttribClass_Block)\
X(LoopBegin, 0x2002, DW_AttribClass_Block)\
X(TailLoopBegin, 0x2003, DW_AttribClass_Block)\
X(EpilogBegin, 0x2004, DW_AttribClass_Block)\
X(LoopUnrollFactor, 0x2005, DW_AttribClass_Block)\
X(SoftwarePipelineDepth, 0x2006, DW_AttribClass_Block)\
X(LinkageName, 0x2007, DW_AttribClass_String)\
X(Stride, 0x2008, DW_AttribClass_Block)\
X(AbstractName, 0x2009, DW_AttribClass_String)\
X(CloneOrigin, 0x200a, DW_AttribClass_String)\
X(HasInlines, 0x200b, DW_AttribClass_Reference)\
X(StrideByte, 0x200c, DW_AttribClass_Reference)\
X(StrideElem, 0x200d, DW_AttribClass_Reference)\
X(PtrDopeType, 0x200e, DW_AttribClass_Reference)\
X(AllocatableDopeType, 0x200f, DW_AttribClass_Reference)\
X(AssumedShapeDopeType, 0x2010, DW_AttribClass_Reference)\
X(AssumedSize, 0x2011, DW_AttribClass_Reference)\

#define DW_ATE_XList \
X(Null, 0x00)\
X(Address, 0x01)\
X(Boolean, 0x02)\
X(ComplexFloat, 0x03)\
X(Float, 0x04)\
X(Signed, 0x05)\
X(SignedChar, 0x06)\
X(Unsigned, 0x07)\
X(UnsignedChar, 0x08)\
X(ImaginaryFloat, 0x09)\
X(PackedDecimal, 0x0A)\
X(NumericString, 0x0B)\
X(Edited, 0x0C)\
X(SignedFixed, 0x0D)\
X(UnsignedFixed, 0x0E)\
X(DecimalFloat, 0x0F)\
X(Utf, 0x10)\
X(Ucs, 0x11)\
X(Ascii, 0x12)\

#define DW_CallingConventionKind_XList \
X(Normal, 0x0)\
X(Program, 0x1)\
X(NoCall, 0x3)\
X(PassByValue, 0x4)\
X(PassByReference, 0x5)\

#define DW_AccessKind_XList \
X(Public, 0x0)\
X(Private, 0x1)\
X(Protected, 0x2)\

#define DW_VirtualityKind_XList \
X(None, 0x0)\
X(Virtual, 0x1)\
X(PureVirtual, 0x2)\

#define DW_IdentifierCaseKind_XList \
X(CaseSensitive, 0x00)\
X(UpCase, 0x01)\
X(DownCase, 0x02)\
X(CaseInsensitive, 0x03)\

#define DW_VisibilityKind_XList \
X(Null, 0x00)\
X(Local, 0x01)\
X(Exported, 0x02)\
X(Qualified, 0x03)\

#define DW_RLE_XList \
X(EndOfList, 0x00)\
X(BaseAddressx, 0x01)\
X(StartxEndx, 0x02)\
X(StartxLength, 0x03)\
X(OffsetPair, 0x04)\
X(BaseAddress, 0x05)\
X(StartEnd, 0x06)\
X(StartLength, 0x07)\

#define DW_LLE_XList \
X(EndOfList, 0x00)\
X(BaseAddressx, 0x01)\
X(StartxEndx, 0x02)\
X(StartxLength, 0x03)\
X(OffsetPair, 0x04)\
X(DefaultLocation, 0x05)\
X(BaseAddress, 0x06)\
X(StartEnd, 0x07)\
X(StartLength, 0x08)\

#define DW_CompUnitKind_XList \
X(Reserved, 0)\
X(Compile, 1)\
X(Type, 2)\
X(Partial, 3)\
X(Skeleton, 4)\
X(SplitCompile, 5)\
X(SplitType, 6)\

#define DW_CFAOpCode_XList \
X(Nop, 0x0, Null, Null, 0)\
X(SetLoc, 0x1, Value, Null, 1)\
X(AdvanceLoc1, 0x2, Value, Null, 1)\
X(AdvanceLoc2, 0x3, Value, Null, 1)\
X(AdvanceLoc4, 0x4, Value, Null, 1)\
X(OffsetExt, 0x5, Register, Value, 0)\
X(RestoreExt, 0x6, Null, Null, 0)\
X(Undefined, 0x7, Register, Null, 0)\
X(SameValue, 0x8, Register, Null, 0)\
X(Register, 0x9, Register, Null, 0)\
X(RememberState, 0xa, Null, Null, 0)\
X(RestoreState, 0xb, Null, Null, 0)\
X(DefCfa, 0xc, Register, Value, 0)\
X(DefCfaRegister, 0xd, Register, Null, 0)\
X(DefCfaOffset, 0xe, Value, Null, 0)\
X(DefCfaExpr, 0xf, Expression, Null, 0)\
X(Expr, 0x10, Expression, Null, 0)\
X(OffsetExtSf, 0x11, Register, Value, 0)\
X(DefCfaSf, 0x12, Register, Value, 0)\
X(DefCfaOffsetSf, 0x13, Value, Null, 0)\
X(ValOffset, 0x14, Register, Value, 0)\
X(ValOffsetSf, 0x15, Register, Value, 0)\
X(ValExpr, 0x16, Register, Expression, 0)\
X(AdvanceLoc, 0x40, Value, Null, 1)\
X(Offset, 0x80, Register, Value, 0)\
X(Restore, 0xc0, Register, Null, 0)\

#define DW_ExprOp_XList \
X(Null, 0x00, Null, 0, 0, 0, Null, Null, 1)\
X(Addr, 0x03, 3, 1, 0, 1, Addr, Null, 1)\
X(Deref, 0x06, 3, 0, 1, 1, Null, Null, 1)\
X(Const1U, 0x08, 3, 1, 0, 1, U8, Null, 1)\
X(Const1S, 0x09, 3, 1, 0, 1, S8, Null, 1)\
X(Const2U, 0x0a, 3, 1, 0, 1, U16, Null, 1)\
X(Const2S, 0x0b, 3, 1, 0, 1, S16, Null, 1)\
X(Const4U, 0x0c, 3, 1, 0, 1, U32, Null, 1)\
X(Const4S, 0x0d, 3, 1, 0, 1, S32, Null, 1)\
X(Const8U, 0x0e, 3, 1, 0, 1, U64, Null, 1)\
X(Const8S, 0x0f, 3, 1, 0, 1, S64, Null, 1)\
X(ConstU, 0x10, 3, 1, 0, 1, ULEB128, Null, 1)\
X(ConstS, 0x11, 3, 1, 0, 1, SLEB128, Null, 1)\
X(Dup, 0x12, 3, 0, 0, 1, Null, Null, 1)\
X(Drop, 0x13, 3, 0, 1, 0, Null, Null, 1)\
X(Over, 0x14, 3, 0, 0, 1, Null, Null, 1)\
X(Pick, 0x15, 3, 1, 0, 1, U8, Null, 1)\
X(Swap, 0x16, 3, 0, 2, 2, Null, Null, 1)\
X(Rot, 0x17, 3, 0, 3, 3, Null, Null, 1)\
X(XDeref, 0x18, 3, 0, 2, 1, Null, Null, 1)\
X(Abs, 0x19, 3, 0, 1, 1, Null, Null, 1)\
X(And, 0x1a, 3, 0, 2, 1, Null, Null, 1)\
X(Div, 0x1b, 3, 0, 2, 1, Null, Null, 1)\
X(Minus, 0x1c, 3, 0, 2, 1, Null, Null, 1)\
X(Mod, 0x1d, 3, 0, 2, 1, Null, Null, 1)\
X(Mul, 0x1e, 3, 0, 2, 1, Null, Null, 1)\
X(Neg, 0x1f, 3, 0, 1, 1, Null, Null, 1)\
X(Not, 0x20, 3, 0, 1, 1, Null, Null, 1)\
X(Or, 0x21, 3, 0, 2, 1, Null, Null, 1)\
X(Plus, 0x22, 3, 0, 2, 1, Null, Null, 1)\
X(PlusUConst, 0x23, 3, 1, 1, 1, ULEB128, Null, 1)\
X(Shl, 0x24, 3, 0, 2, 1, Null, Null, 1)\
X(Shr, 0x25, 3, 0, 2, 1, Null, Null, 1)\
X(Shra, 0x26, 3, 0, 2, 1, Null, Null, 1)\
X(Xor, 0x27, 3, 0, 2, 1, Null, Null, 1)\
X(Bra, 0x28, 3, 1, 1, 0, S16, Null, 1)\
X(Eq, 0x29, 3, 0, 2, 1, Null, Null, 1)\
X(Ge, 0x2a, 3, 0, 2, 1, Null, Null, 1)\
X(Gt, 0x2b, 3, 0, 2, 1, Null, Null, 1)\
X(Le, 0x2c, 3, 0, 2, 1, Null, Null, 1)\
X(Lt, 0x2d, 3, 0, 2, 1, Null, Null, 1)\
X(Ne, 0x2e, 3, 0, 2, 1, Null, Null, 1)\
X(Skip, 0x2f, 3, 1, 0, 0, S16, Null, 1)\
X(Lit0, 0x30, 3, 0, 0, 0, Null, Null, 1)\
X(Lit1, 0x31, 3, 0, 0, 0, Null, Null, 1)\
X(Lit2, 0x32, 3, 0, 0, 0, Null, Null, 1)\
X(Lit3, 0x33, 3, 0, 0, 0, Null, Null, 1)\
X(Lit4, 0x34, 3, 0, 0, 0, Null, Null, 1)\
X(Lit5, 0x35, 3, 0, 0, 0, Null, Null, 1)\
X(Lit6, 0x36, 3, 0, 0, 0, Null, Null, 1)\
X(Lit7, 0x37, 3, 0, 0, 0, Null, Null, 1)\
X(Lit8, 0x38, 3, 0, 0, 0, Null, Null, 1)\
X(Lit9, 0x39, 3, 0, 0, 0, Null, Null, 1)\
X(Lit10, 0x3a, 3, 0, 0, 0, Null, Null, 1)\
X(Lit11, 0x3b, 3, 0, 0, 0, Null, Null, 1)\
X(Lit12, 0x3c, 3, 0, 0, 0, Null, Null, 1)\
X(Lit13, 0x3d, 3, 0, 0, 0, Null, Null, 1)\
X(Lit14, 0x3e, 3, 0, 0, 0, Null, Null, 1)\
X(Lit15, 0x3f, 3, 0, 0, 0, Null, Null, 1)\
X(Lit16, 0x40, 3, 0, 0, 0, Null, Null, 1)\
X(Lit17, 0x41, 3, 0, 0, 0, Null, Null, 1)\
X(Lit18, 0x42, 3, 0, 0, 0, Null, Null, 1)\
X(Lit19, 0x43, 3, 0, 0, 0, Null, Null, 1)\
X(Lit20, 0x44, 3, 0, 0, 0, Null, Null, 1)\
X(Lit21, 0x45, 3, 0, 0, 0, Null, Null, 1)\
X(Lit22, 0x46, 3, 0, 0, 0, Null, Null, 1)\
X(Lit23, 0x47, 3, 0, 0, 0, Null, Null, 1)\
X(Lit24, 0x48, 3, 0, 0, 0, Null, Null, 1)\
X(Lit25, 0x49, 3, 0, 0, 0, Null, Null, 1)\
X(Lit26, 0x4a, 3, 0, 0, 0, Null, Null, 1)\
X(Lit27, 0x4b, 3, 0, 0, 0, Null, Null, 1)\
X(Lit28, 0x4c, 3, 0, 0, 0, Null, Null, 1)\
X(Lit29, 0x4d, 3, 0, 0, 0, Null, Null, 1)\
X(Lit30, 0x4e, 3, 0, 0, 0, Null, Null, 1)\
X(Lit31, 0x4f, 3, 0, 0, 0, Null, Null, 1)\
X(Reg0, 0x50, 3, 0, 0, 1, Null, Null, 1)\
X(Reg1, 0x51, 3, 0, 0, 1, Null, Null, 1)\
X(Reg2, 0x52, 3, 0, 0, 1, Null, Null, 1)\
X(Reg3, 0x53, 3, 0, 0, 1, Null, Null, 1)\
X(Reg4, 0x54, 3, 0, 0, 1, Null, Null, 1)\
X(Reg5, 0x55, 3, 0, 0, 1, Null, Null, 1)\
X(Reg6, 0x56, 3, 0, 0, 1, Null, Null, 1)\
X(Reg7, 0x57, 3, 0, 0, 1, Null, Null, 1)\
X(Reg8, 0x58, 3, 0, 0, 1, Null, Null, 1)\
X(Reg9, 0x59, 3, 0, 0, 1, Null, Null, 1)\
X(Reg10, 0x5a, 3, 0, 0, 1, Null, Null, 1)\
X(Reg11, 0x5b, 3, 0, 0, 1, Null, Null, 1)\
X(Reg12, 0x5c, 3, 0, 0, 1, Null, Null, 1)\
X(Reg13, 0x5d, 3, 0, 0, 1, Null, Null, 1)\
X(Reg14, 0x5e, 3, 0, 0, 1, Null, Null, 1)\
X(Reg15, 0x5f, 3, 0, 0, 1, Null, Null, 1)\
X(Reg16, 0x60, 3, 0, 0, 1, Null, Null, 1)\
X(Reg17, 0x61, 3, 0, 0, 1, Null, Null, 1)\
X(Reg18, 0x62, 3, 0, 0, 1, Null, Null, 1)\
X(Reg19, 0x63, 3, 0, 0, 1, Null, Null, 1)\
X(Reg20, 0x64, 3, 0, 0, 1, Null, Null, 1)\
X(Reg21, 0x65, 3, 0, 0, 1, Null, Null, 1)\
X(Reg22, 0x66, 3, 0, 0, 1, Null, Null, 1)\
X(Reg23, 0x67, 3, 0, 0, 1, Null, Null, 1)\
X(Reg24, 0x68, 3, 0, 0, 1, Null, Null, 1)\
X(Reg25, 0x69, 3, 0, 0, 1, Null, Null, 1)\
X(Reg26, 0x6a, 3, 0, 0, 1, Null, Null, 1)\
X(Reg27, 0x6b, 3, 0, 0, 1, Null, Null, 1)\
X(Reg28, 0x6c, 3, 0, 0, 1, Null, Null, 1)\
X(Reg29, 0x6d, 3, 0, 0, 1, Null, Null, 1)\
X(Reg30, 0x6e, 3, 0, 0, 1, Null, Null, 1)\
X(Reg31, 0x6f, 3, 0, 0, 1, Null, Null, 1)\
X(BReg0, 0x70, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg1, 0x71, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg2, 0x72, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg3, 0x73, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg4, 0x74, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg5, 0x75, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg6, 0x76, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg7, 0x77, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg8, 0x78, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg9, 0x79, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg10, 0x7a, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg11, 0x7b, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg12, 0x7c, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg13, 0x7d, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg14, 0x7e, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg15, 0x7f, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg16, 0x80, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg17, 0x81, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg18, 0x82, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg19, 0x83, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg20, 0x84, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg21, 0x85, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg22, 0x86, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg23, 0x87, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg24, 0x88, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg25, 0x89, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg26, 0x8a, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg27, 0x8b, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg28, 0x8c, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg29, 0x8d, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg30, 0x8e, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BReg31, 0x8f, 3, 1, 0, 1, SLEB128, Null, 1)\
X(RegX, 0x90, 3, 1, 0, 1, ULEB128, Null, 1)\
X(FBReg, 0x91, 3, 1, 0, 1, SLEB128, Null, 1)\
X(BRegX, 0x92, 3, 2, 0, 1, ULEB128, SLEB128, 1)\
X(Piece, 0x93, 3, 1, 0, 0, ULEB128, Null, 1)\
X(DerefSize, 0x94, 3, 1, 1, 1, U8, Null, 1)\
X(XDerefSize, 0x95, 3, 1, 2, 1, U8, Null, 1)\
X(Nop, 0x96, 3, 0, 0, 0, Null, Null, 1)\
X(PushObjectAddress, 0x97, 3, 0, 0, 1, Null, Null, 0)\
X(Call2, 0x98, 3, 1, 0, 0, U16, Null, 0)\
X(Call4, 0x99, 3, 1, 0, 0, U32, Null, 0)\
X(CallRef, 0x9a, 3, 1, 0, 0, DwarfUInt, Null, 0)\
X(FormTlsAddress, 0x9b, 3, 0, 0, 1, Null, Null, 1)\
X(CallFrameCfa, 0x9c, 3, 0, 0, 1, Null, Null, 0)\
X(BitPiece, 0x9d, 3, 2, 0, 0, ULEB128, ULEB128, 1)\
X(ImplicitValue, 0x9e, 4, 2, 0, 1, Block, Null, 1)\
X(StackValue, 0x9f, 4, 0, 0, 0, Null, Null, 1)\
X(ImplicitPointer, 0xa0, 5, 2, 0, 1, DwarfUInt, SLEB128, 1)\
X(Addrx, 0xa1, 5, 1, 0, 1, ULEB128, Null, 0)\
X(Constx, 0xa2, 5, 1, 0, 1, ULEB128, Null, 0)\
X(EntryValue, 0xa3, 5, 1, 0, 0, Block, Null, 1)\
X(ConstType, 0xa4, 5, 2, 0, 1, ULEB128, Block, 0)\
X(RegvalType, 0xa5, 5, 2, 0, 1, ULEB128, ULEB128, 0)\
X(DerefType, 0xa6, 5, 2, 1, 1, U8, ULEB128, 0)\
X(XDerefType, 0xa7, 5, 2, 2, 1, U8, ULEB128, 1)\
X(Convert, 0xa8, 5, 1, 1, 1, ULEB128, Null, 0)\
X(Reinterpret, 0xa9, 5, 1, 1, 1, ULEB128, Null, 0)\

#define DW_GNU_ExprOp_XList \
X(PushTlsAddress, 0xe0, 0, 0, 1, Null, Null, 1)\
X(UnInit, 0xf0, 0, 0, 0, Null, Null, 1)\
X(ImplicitPointer, 0xf2, 2, 0, 1, DwarfUInt, SLEB128, 1)\
X(EntryValue, 0xf3, 1, 0, 0, Block, Null, 1)\
X(ConstType, 0xf4, 2, 0, 1, ULEB128, Block, 1)\
X(RegvalType, 0xf5, 2, 0, 1, ULEB128, ULEB128, 1)\
X(DerefType, 0xf6, 2, 1, 1, U8, ULEB128, 1)\
X(Convert, 0xf7, 1, 1, 1, ULEB128, Null, 1)\
X(ParameterRef, 0xfa, 1, 0, 0, U32, Null, 1)\
X(AddrIndex, 0xfb, 0, 0, 1, Null, Null, 1)\
X(ConstIndex, 0xfc, 1, 0, 1, ULEB128, Null, 1)\

#define DW_CFAOperandMax 2
#define DW_CFAOpCodeMask_OpcodeHi 0xc0
#define DW_CFAOpCodeMask_Operand  0x3f
typedef U8 DW_RegCode;
C_LINKAGE_BEGIN
extern String8 dw_regular_name_from_section_kind_table[18];
extern String8 dw_macho_name_from_section_kind_table[18];
extern String8 dw_dwo_name_from_section_kind_table[18];

C_LINKAGE_END

#endif // DWARF_META_H
