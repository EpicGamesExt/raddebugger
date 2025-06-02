// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_H
#define DWARF_H

typedef U16 DW_Version;
typedef enum DW_VersionEnum
{
  DW_Version_Null,
  DW_Version_1,
  DW_Version_2,
  DW_Version_3,
  DW_Version_4,
  DW_Version_5,
  DW_Version_Last = DW_Version_5
} DW_VersionEnum;

typedef U64 DW_Ext;
typedef enum DW_ExtEnum
{
  DW_Ext_Null,
  
  DW_Ext_GNU   = (1 << 0),
  DW_Ext_LLVM  = (1 << 1),
  DW_Ext_APPLE = (1 << 2),
  DW_Ext_MIPS  = (1 << 3),
  
  DW_Ext_All   = DW_Ext_GNU|DW_Ext_LLVM|DW_Ext_APPLE|DW_Ext_MIPS,
} DW_ExtEnum;

#define DW_FormatFromSize(size) ((size) >= max_U32 ? DW_Format_64Bit : DW_Format_32Bit)
typedef enum DW_Format
{
  DW_Format_Null,
  DW_Format_32Bit,
  DW_Format_64Bit
} DW_Format;

#define DW_SentinelFromSize(address_size) ((address_size) == 4 ? max_U32 : (address_size) == 8 ? max_U64 : 0)

#define DW_SectionKind_XList(X                                                       )\
X(Null,       "",                   "",                    ""                      )\
X(Abbrev,     ".debug_abbrev",      "__debug_abbrev",      ".debug_abbrev.dwo"     )\
X(ARanges,    ".debug_aranges",     "__debug_aranges",     ".debug_aranges.dwo"    )\
X(Frame,      ".debug_frame",       "__debug_frame",       ".debug_frame.dwo"      )\
X(Info,       ".debug_info",        "__debug_info",        ".debug_info.dwo"       )\
X(Line,       ".debug_line",        "__debug_line",        ".debug_line.dwo"       )\
X(Loc,        ".debug_loc",         "__debug_loc",         ".debug_loc.dwo"        )\
X(MacInfo,    ".debug_macinfo",     "__debug_macinfo",     ".debug_macinfo.dwo"    )\
X(PubNames,   ".debug_pubnames",    "__debug_pubnames",    ".debug_pubnames.dwo"   )\
X(PubTypes,   ".debug_pubtypes",    "__debug_pubtypes",    ".debug_pubtypes.dwo"   )\
X(Ranges,     ".debug_ranges",      "__debug_ranges",      ".debug_ranges.dwo"     )\
X(Str,        ".debug_str",         "__debug_str",         ".debug_str.dwo"        )\
X(Addr,       ".debug_addr",        "__debug_addr",        ".debug_addr.dwo"       )\
X(LocLists,   ".debug_loclists",    "__debug_loclists",    ".debug_loclists.dwo"   )\
X(RngLists,   ".debug_rnglists",    "__debug_rnglists",    ".debug_rnglists.dwo"   )\
X(StrOffsets, ".debug_str_offsets", "__debug_str_offsets", ".debug_str_offsets.dwo")\
X(LineStr,    ".debug_line_str",    "__debug_line_str",    ".debug_line_str.dwo"   )\
X(Names,      ".debug_names",       "__debug_names",       ".debug_names.dwo"      )

typedef U64 DW_SectionKind;
typedef enum DW_SectionKindEnum
{
#define X(_N,...) DW_Section_##_N,
  DW_SectionKind_XList(X)
#undef X
  DW_Section_Count
} DW_SectionKindEnum;

#define DW_Language_XList(X)     \
X(Null,                0x00)   \
X(C89,                 0x01)   \
X(C,                   0x02)   \
X(Ada83,               0x03)   \
X(CPlusPlus,           0x04)   \
X(Cobol74,             0x05)   \
X(Cobol85,             0x06)   \
X(Fortran77,           0x07)   \
X(Fortran90,           0x08)   \
X(Pascal83,            0x09)   \
X(Modula2,             0x0A)   \
X(Java,                0x0B)   \
X(C99,                 0x0C)   \
X(Ada95,               0x0D)   \
X(Fortran95,           0x0E)   \
X(PLI,                 0x0F)   \
X(ObjC,                0x10)   \
X(ObjCPlusPlus,        0x11)   \
X(UPC,                 0x12)   \
X(D,                   0x13)   \
X(Python,              0x14)   \
X(OpenCL,              0x15)   \
X(Go,                  0x16)   \
X(Modula3,             0x17)   \
X(Haskell,             0x18)   \
X(CPlusPlus03,         0x19)   \
X(CPlusPlus11,         0x1a)   \
X(OCaml,               0x1b)   \
X(Rust,                0x1c)   \
X(C11,                 0x1d)   \
X(Swift,               0x1e)   \
X(Julia,               0x1f)   \
X(Dylan,               0x20)   \
X(CPlusPlus14,         0x21)   \
X(Fortran03,           0x22)   \
X(Fortran08,           0x23)   \
X(RenderScript,        0x24)   \
X(BLISS,               0x25)   \
X(MipsAssembler,       0x8001) \
X(GoogleRenderScript,  0x8E57) \
X(SunAssembler,        0x9001) \
X(BorlandDelphi,       0xB000)

typedef U32 DW_Language;
typedef enum DW_LanguageEnum
{
#define X(_N, _ID) DW_Language_##_N = _ID,
  DW_Language_XList(X)
#undef X
  DW_Language_UserLo = 0x8000,
  DW_Language_UserHi = 0xffff,
} DW_LanguageEnum;

#define DW_Inl_XList(X)    \
X(NotInlined,         0) \
X(Inlined,            1) \
X(DeclaredNotInlined, 2) \
X(DeclaredInlined,    3)

typedef U32 DW_InlKind;
typedef enum DW_InlKindEnum
{
#define X(_N,_ID) DW_Inl_##_N = _ID,
  DW_Inl_XList(X)
#undef X
} DW_InlKindEnum;

#define DW_StdOpcode_XList(X) \
X(ExtendedOpcode,   0x00)   \
X(Copy,             0x01)   \
X(AdvancePc,        0x02)   \
X(AdvanceLine,      0x03)   \
X(SetFile,          0x04)   \
X(SetColumn,        0x05)   \
X(NegateStmt,       0x06)   \
X(SetBasicBlock,    0x07)   \
X(ConstAddPc,       0x08)   \
X(FixedAdvancePc,   0x09)   \
X(SetPrologueEnd,   0x0A)   \
X(SetEpilogueBegin, 0x0B)   \
X(SetIsa,           0x0C)   \

typedef enum DW_StdOpcode
{
#define X(_N,_ID) DW_StdOpcode_##_N = _ID,
  DW_StdOpcode_XList(X)
#undef X
} DW_StdOpcode;

#define DW_ExtOpcode_XList(X) \
X(Undefined,        0x00)   \
X(EndSequence,      0x01)   \
X(SetAddress,       0x02)   \
X(DefineFile,       0x03)   \
X(SetDiscriminator, 0x04)   \
X(UserLo,           0x80)   \
X(UserHi,           0xff)

typedef enum DW_ExtOpcode
{
#define X(_N,_ID) DW_ExtOpcode_##_N = _ID,
  DW_ExtOpcode_XList(X)
#undef X
} DW_ExtOpcode;

#define DW_IDCaseKind_XList(X) \
X(CaseSensitive,   0x00)     \
X(UpCase,          0x01)     \
X(DownCase,        0x02)     \
X(CaseInsensitive, 0x03)

typedef U64 DW_IDCaseKind;
typedef enum DW_IDCaseKindEnum
{
#define X(_N,_ID) DW_IDCase_##_N = _ID,
  DW_IDCaseKind_XList(X)
#undef X
} DW_IDCaseKindEnum;

#define DW_Tag_V3_XList(X)        \
X(ArrayType,              0x01) \
X(ClassType,              0x02) \
X(EntryPoint,             0x03) \
X(EnumerationType,        0x04) \
X(FormalParameter,        0x05) \
X(ImportedDeclaration,    0x08) \
X(Label,                  0x0a) \
X(LexicalBlock,           0x0b) \
X(Member,                 0x0d) \
X(PointerType,            0x0f) \
X(ReferenceType,          0x10) \
X(CompileUnit,            0x11) \
X(StringType,             0x12) \
X(StructureType,          0x13) \
X(SubroutineType,         0x15) \
X(Typedef,                0x16) \
X(UnionType,              0x17) \
X(UnspecifiedParameters,  0x18) \
X(Variant,                0x19) \
X(CommonBlock,            0x1a) \
X(CommonInclusion,        0x1b) \
X(Inheritance,            0x1c) \
X(InlinedSubroutine,      0x1d) \
X(Module,                 0x1e) \
X(PtrToMemberType,        0x1f) \
X(SetType,                0x20) \
X(SubrangeType,           0x21) \
X(WithStmt,               0x22) \
X(AccessDeclaration,      0x23) \
X(BaseType,               0x24) \
X(CatchBlock,             0x25) \
X(ConstType,              0x26) \
X(Constant,               0x27) \
X(Enumerator,             0x28) \
X(FileType,               0x29) \
X(Friend,                 0x2a) \
X(NameList,               0x2b) \
X(NameListItem,           0x2c) \
X(PackedType,             0x2d) \
X(SubProgram,             0x2e) \
X(TemplateTypeParameter,  0x2f) \
X(TemplateValueParameter, 0x30) \
X(ThrownType,             0x31) \
X(TryBlock,               0x32) \
X(VariantPart,            0x33) \
X(Variable,               0x34) \
X(VolatileType,           0x35) \
X(DwarfProcedure,         0x36) \
X(RestrictType,           0x37) \
X(InterfaceType,          0x38) \
X(Namespace,              0x39) \
X(ImportedModule,         0x3a) \
X(UnspecifiedType,        0x3b) \
X(PartialUnit,            0x3c) \
X(ImportedUnit,           0x3d) \
X(Condition,              0x3f) \
X(SharedType,             0x40)

#define DW_Tag_V5_XList(X)     \
X(TypeUnit,            0x41) \
X(RValueReferenceType, 0x42) \
X(TemplateAlias,       0x43) \
X(CoarrayType,         0x44) \
X(GenericSubrange,     0x45) \
X(DynamicType,         0x46) \
X(AtomicType,          0x47) \
X(CallSite,            0x48) \
X(CallSiteParameter,   0x49) \
X(SkeletonUnit,        0x4A) \
X(ImmutableType,       0x4B)

#define DW_Tag_GNU_XList(X)        \
X(GNU_CallSite,          0x4109) \
X(GNU_CallSiteParameter, 0x410a)

typedef U64 DW_TagKind;
typedef enum DW_TagKindEnum
{
  DW_Tag_Null,
#define X(_N,_ID) DW_Tag_##_N = _ID,
  DW_Tag_V3_XList(X)
    DW_Tag_V5_XList(X)
    DW_Tag_GNU_XList(X)
#undef X
  DW_Tag_UserLo = 0x4080,
  DW_Tag_UserHi = 0xffff
} DW_TagKindEnum;

//- Attrib Class Encodings

#define DW_AttribClass_V3_XList(X) \
X(Null,         0)               \
X(Undefined,    1)               \
X(Address,      2)               \
X(Block,        3)               \
X(Const,        4)               \
X(ExprLoc,      5)               \
X(Flag,         6)               \
X(LinePtr,      7)               \
X(LocListPtr,   8)               \
X(MacPtr,       9)               \
X(RngListPtr,   10)              \
X(Reference,    11)              \
X(String,       12)

#define DW_AttribClass_V4_XList(X) \
X(LocList, 13)                   \
X(RngList, 14)

#define DW_AttribClass_V5_XList(X) \
X(StrOffsetsPtr, 15)             \
X(AddrPtr,       16)

typedef U32 DW_AttribClass;
typedef enum DW_AttribClassEnum
{
#define X(_N,_ID) DW_AttribClass_##_N = (1 << _ID),
  DW_AttribClass_V3_XList(X)
    DW_AttribClass_V4_XList(X)
    DW_AttribClass_V5_XList(X)
#undef X
} DW_AttribClassEnum;

//- Form Encodings

#define DW_Form_V2_XList(X) \
X(Addr,     0x1)          \
X(Block2,   0x3)          \
X(Block4,   0x4)          \
X(Data2,    0x5)          \
X(Data4,    0x6)          \
X(Data8,    0x7)          \
X(String,   0x8)          \
X(Block,    0x9)          \
X(Block1,   0xa)          \
X(Data1,    0xb)          \
X(Flag,     0xc)          \
X(SData,    0xd)          \
X(Strp,     0xe)          \
X(UData,    0xf)          \
X(RefAddr,  0x10)         \
X(Ref1,     0x11)         \
X(Ref2,     0x12)         \
X(Ref4,     0x13)         \
X(Ref8,     0x14)         \
X(RefUData, 0x15)         \
X(Indirect, 0x16)

#define DW_Form_AttribClass_V2_XList(X)  \
X(Addr,     DW_AttribClass_Address)    \
X(Block2,   DW_AttribClass_Block)      \
X(Block4,   DW_AttribClass_Block)      \
X(Data2,    DW_AttribClass_Const)      \
X(Data4,    DW_AttribClass_Const)      \
X(Data8,    DW_AttribClass_Const)      \
X(String,   DW_AttribClass_String)     \
X(Block,    DW_AttribClass_Block)      \
X(Block1,   DW_AttribClass_Block)      \
X(Data1,    DW_AttribClass_Const)      \
X(Flag,     DW_AttribClass_Flag)       \
X(SData,    DW_AttribClass_Const)      \
X(Strp,     DW_AttribClass_String)     \
X(UData,    DW_AttribClass_Const)      \
X(RefAddr,  DW_AttribClass_Reference)  \
X(Ref1,     DW_AttribClass_Reference)  \
X(Ref2,     DW_AttribClass_Reference)  \
X(Ref4,     DW_AttribClass_Reference)  \
X(Ref8,     DW_AttribClass_Reference)  \
X(RefUData, DW_AttribClass_Reference)  \
X(Indirect, DW_AttribClass_Null)

#define DW_Form_V4_XList(X) \
X(SecOffset,   0x17)      \
X(ExprLoc,     0x18)      \
X(FlagPresent, 0x19)      \
X(RefSig8,     0x20)

#define DW_Form_AttribClass_V4_XList(X)     \
X(Addr,        DW_AttribClass_Address)    \
X(Block2,      DW_AttribClass_Block)      \
X(Block4,      DW_AttribClass_Block)      \
X(Data2,       DW_AttribClass_Const)      \
X(Data4,       DW_AttribClass_Const)      \
X(Data8,       DW_AttribClass_Const)      \
X(String,      DW_AttribClass_String)     \
X(Block,       DW_AttribClass_Block)      \
X(Block1,      DW_AttribClass_Block)      \
X(Data1,       DW_AttribClass_Const)      \
X(Flag,        DW_AttribClass_Flag)       \
X(SData,       DW_AttribClass_Const)      \
X(Strp,        DW_AttribClass_String)     \
X(UData,       DW_AttribClass_Const)      \
X(RefAddr,     DW_AttribClass_Reference)  \
X(Ref1,        DW_AttribClass_Reference)  \
X(Ref2,        DW_AttribClass_Reference)  \
X(Ref4,        DW_AttribClass_Reference)  \
X(Ref8,        DW_AttribClass_Reference)  \
X(RefUData,    DW_AttribClass_Reference)  \
X(Indirect,    DW_AttribClass_Null)       \
X(SecOffset,   DW_AttribClass_LinePtr|DW_AttribClass_LocListPtr|DW_AttribClass_MacPtr|DW_AttribClass_RngListPtr) \
X(ExprLoc,     DW_AttribClass_ExprLoc)    \
X(FlagPresent, DW_AttribClass_Flag)       \
X(RefSig8,     DW_AttribClass_Reference)

#define DW_Form_V5_XList(X) \
X(Strx,          0x1a)    \
X(Addrx,         0x1b)    \
X(RefSup4,       0x1c)    \
X(StrpSup,       0x1d)    \
X(Data16,        0x1e)    \
X(LineStrp,      0x1f)    \
X(ImplicitConst, 0x21)    \
X(LocListx,      0x22)    \
X(RngListx,      0x23)    \
X(RefSup8,       0x24)    \
X(Strx1,         0x25)    \
X(Strx2,         0x26)    \
X(Strx3,         0x27)    \
X(Strx4,         0x28)    \
X(Addrx1,        0x29)    \
X(Addrx2,        0x2a)    \
X(Addrx3,        0x2b)    \
X(Addrx4,        0x2c)

#define DW_Form_AttribClass_V5_XList(X)          \
X(Addr,          DW_AttribClass_Address)       \
X(Block2,        DW_AttribClass_Block)         \
X(Block4,        DW_AttribClass_Block)         \
X(Data2,         DW_AttribClass_Const)         \
X(Data4,         DW_AttribClass_Const)         \
X(Data8,         DW_AttribClass_Const)         \
X(String,        DW_AttribClass_String)        \
X(Block,         DW_AttribClass_Block)         \
X(Block1,        DW_AttribClass_Block)         \
X(Data1,         DW_AttribClass_Const)         \
X(Flag,          DW_AttribClass_Flag)          \
X(SData,         DW_AttribClass_Const)         \
X(Strp,          DW_AttribClass_String)        \
X(UData,         DW_AttribClass_Const)         \
X(RefAddr,       DW_AttribClass_Reference)     \
X(Ref1,          DW_AttribClass_Reference)     \
X(Ref2,          DW_AttribClass_Reference)     \
X(Ref4,          DW_AttribClass_Reference)     \
X(Ref8,          DW_AttribClass_Reference)     \
X(RefUData,      DW_AttribClass_Reference)     \
X(Indirect,      DW_AttribClass_Null)          \
X(SecOffset,     DW_AttribClass_AddrPtr|       \
DW_AttribClass_LinePtr|       \
DW_AttribClass_LocList|       \
DW_AttribClass_LocListPtr|    \
DW_AttribClass_MacPtr|        \
DW_AttribClass_RngList|       \
DW_AttribClass_RngListPtr|    \
DW_AttribClass_StrOffsetsPtr) \
X(ExprLoc,       DW_AttribClass_ExprLoc)       \
X(FlagPresent,   DW_AttribClass_Flag)          \
X(RefSig8,       DW_AttribClass_Reference)     \
X(Strx,          DW_AttribClass_String)        \
X(Addrx,         DW_AttribClass_Address)       \
X(RefSup4,       DW_AttribClass_Reference)     \
X(StrpSup,       DW_AttribClass_String)        \
X(Data16,        DW_AttribClass_Const)         \
X(LineStrp,      DW_AttribClass_String)        \
X(ImplicitConst, DW_AttribClass_Const)         \
X(LocListx,      DW_AttribClass_LocListPtr)    \
X(RngListx,      DW_AttribClass_RngList)    \
X(RefSup8,       DW_AttribClass_Reference)     \
X(Strx1,         DW_AttribClass_String)        \
X(Strx2,         DW_AttribClass_String)        \
X(Strx3,         DW_AttribClass_String)        \
X(Strx4,         DW_AttribClass_String)        \
X(Addrx1,        DW_AttribClass_Address)       \
X(Addrx2,        DW_AttribClass_Address)       \
X(Addrx3,        DW_AttribClass_Address)       \
X(Addrx4,        DW_AttribClass_Address)

#define DW_Form_GNU_XList(X) \
X(GNU_AddrIndex, 0x1f01)   \
X(GNU_StrIndex,  0x1f02)   \
X(GNU_RefAlt,    0x1f20)   \
X(GNU_StrpAlt,   0x1f21)

#define DW_Form_AttribClass_GNU_XList(X)     \
X(GNU_AddrIndex, DW_AttribClass_Undefined) \
X(GNU_StrIndex,  DW_AttribClass_Undefined) \
X(GNU_RefAlt,    DW_AttribClass_Undefined) \
X(GNU_StrpAlt,   DW_AttribClass_String)

typedef U64 DW_FormKind;
typedef enum DW_FormEnum
{
  DW_Form_Null,
#define X(_N, _ID) DW_Form_##_N = _ID,
  DW_Form_V2_XList(X)
    DW_Form_V4_XList(X)
    DW_Form_V5_XList(X)
    DW_Form_GNU_XList(X)
#undef X
} DW_FormEnum;

//- Attributes DWARF2

#define DW_AttribKind_V2_XList(X) \
X(Sibling,            0x1)      \
X(Location,           0x2)      \
X(Name,               0x3)      \
X(Ordering,           0x9)      \
X(ByteSize,           0xB)      \
X(BitOffset,          0xC)      \
X(BitSize,            0xD)      \
X(StmtList,           0x10)     \
X(LowPc,              0x11)     \
X(HighPc,             0x12)     \
X(Language,           0x13)     \
X(Discr,              0x15)     \
X(DiscrValue,         0x16)     \
X(Visibility,         0x17)     \
X(Import,             0x18)     \
X(StringLength,       0x19)     \
X(CommonReference,    0x1a)     \
X(CompDir,            0x1b)     \
X(ConstValue,         0x1c)     \
X(ContainingType,     0x1d)     \
X(DefaultValue,       0x1e)     \
X(Inline,             0x20)     \
X(IsOptional,         0x21)     \
X(LowerBound,         0x22)     \
X(Producer,           0x25)     \
X(Prototyped,         0x27)     \
X(ReturnAddr,         0x2a)     \
X(StartScope,         0x2c)     \
X(BitStride,          0x2e)     \
X(UpperBound,         0x2f)     \
X(AbstractOrigin,     0x31)     \
X(Accessibility,      0x32)     \
X(AddressClass,       0x33)     \
X(Artificial,         0x34)     \
X(BaseTypes,          0x35)     \
X(CallingConvention,  0x36)     \
X(Count,              0x37)     \
X(DataMemberLocation, 0x38)     \
X(DeclColumn,         0x39)     \
X(DeclFile,           0x3a)     \
X(DeclLine,           0x3b)     \
X(Declaration,        0x3c)     \
X(DiscrList,          0x3d)     \
X(Encoding,           0x3e)     \
X(External,           0x3f)     \
X(FrameBase,          0x40)     \
X(Friend,             0x41)     \
X(IdentifierCase,     0x42)     \
X(MacroInfo,          0x43)     \
X(NameListItem,       0x44)     \
X(Priority,           0x45)     \
X(Segment,            0x46)     \
X(Specification,      0x47)     \
X(StaticLink,         0x48)     \
X(Type,               0x49)     \
X(UseLocation,        0x4a)     \
X(VariableParameter,  0x4b)     \
X(Virtuality,         0x4c)     \
X(VTableElemLocation, 0x4d)

#define DW_AttribKind_ClassFlags_V2_XList(X)                                             \
X(Sibling,            DW_AttribClass_Reference)                                        \
X(Location,           DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                \
X(Name,               DW_AttribClass_String)                                           \
X(Ordering,           DW_AttribClass_Const)                                            \
X(ByteSize,           DW_AttribClass_Const)                                            \
X(BitOffset,          DW_AttribClass_Const)                                            \
X(BitSize,            DW_AttribClass_Const)                                            \
X(StmtList,           DW_AttribClass_Const)                                            \
X(LowPc,              DW_AttribClass_Address)                                          \
X(HighPc,             DW_AttribClass_Address)                                          \
X(Language,           DW_AttribClass_Const)                                            \
X(Discr,              DW_AttribClass_Reference)                                        \
X(DiscrValue,         DW_AttribClass_Const)                                            \
X(Visibility,         DW_AttribClass_Const)                                            \
X(Import,             DW_AttribClass_Reference)                                        \
X(StringLength,       DW_AttribClass_Block|DW_AttribClass_Const)                       \
X(CommonReference,    DW_AttribClass_Reference)                                        \
X(CompDir,            DW_AttribClass_String)                                           \
X(ConstValue,         DW_AttribClass_String|DW_AttribClass_Const|DW_AttribClass_Block) \
X(ContainingType,     DW_AttribClass_Reference)                                        \
X(DefaultValue,       DW_AttribClass_Reference)                                        \
X(Inline,             DW_AttribClass_Const)                                            \
X(IsOptional,         DW_AttribClass_Flag)                                             \
X(LowerBound,         DW_AttribClass_Const|DW_AttribClass_Reference)                   \
X(Producer,           DW_AttribClass_String)                                           \
X(Prototyped,         DW_AttribClass_Flag)                                             \
X(ReturnAddr,         DW_AttribClass_Block|DW_AttribClass_Const)                       \
X(StartScope,         DW_AttribClass_Const)                                            \
X(BitStride,          DW_AttribClass_Const) /* dwarf-v1 DW_Attrib_stride_size*/        \
X(UpperBound,         DW_AttribClass_Const|DW_AttribClass_Reference)                   \
X(AbstractOrigin,     DW_AttribClass_Reference)                                        \
X(Accessibility,      DW_AttribClass_Const)                                            \
X(AddressClass,       DW_AttribClass_Const)                                            \
X(Artificial,         DW_AttribClass_Flag)                                             \
X(BaseTypes,          DW_AttribClass_Reference)                                        \
X(CallingConvention,  DW_AttribClass_Const)                                            \
X(Count,              DW_AttribClass_Const|DW_AttribClass_Reference)                   \
X(DataMemberLocation, DW_AttribClass_Block|DW_AttribClass_Reference)                   \
X(DeclColumn,         DW_AttribClass_Const)                                            \
X(DeclFile,           DW_AttribClass_Const)                                            \
X(DeclLine,           DW_AttribClass_Const)                                            \
X(Declaration,        DW_AttribClass_Flag)                                             \
X(DiscrList,          DW_AttribClass_Block)                                            \
X(Encoding,           DW_AttribClass_Const)                                            \
X(External,           DW_AttribClass_Flag)                                             \
X(FrameBase,          DW_AttribClass_Block|DW_AttribClass_Const)                       \
X(Friend,             DW_AttribClass_Reference)                                        \
X(IdentifierCase,     DW_AttribClass_Const)                                            \
X(MacroInfo,          DW_AttribClass_Const)                                            \
X(NameListItem,       DW_AttribClass_Block)                                            \
X(Priority,           DW_AttribClass_Reference)                                        \
X(Segment,            DW_AttribClass_Block|DW_AttribClass_Const)                       \
X(Specification,      DW_AttribClass_Reference)                                        \
X(StaticLink,         DW_AttribClass_Block|DW_AttribClass_Const)                       \
X(Type,               DW_AttribClass_Reference)                                        \
X(UseLocation,        DW_AttribClass_Block|DW_AttribClass_Const)                       \
X(VariableParameter,  DW_AttribClass_Flag)                                             \
X(Virtuality,         DW_AttribClass_Const)                                            \
X(VTableElemLocation, DW_AttribClass_Block|DW_AttribClass_Reference)

//- Attributes DWARF3

#define DW_AttribKind_V3_XList(X) \
X(Allocated,          0x4e)     \
X(Associated,         0x4f)     \
X(DataLocation,       0x50)     \
X(ByteStride,         0x51)     \
X(EntryPc,            0x52)     \
X(UseUtf8,            0x53)     \
X(Extension,          0x54)     \
X(Ranges,             0x55)     \
X(Trampoline,         0x56)     \
X(CallColumn,         0x57)     \
X(CallFile,           0x58)     \
X(CallLine,           0x59)     \
X(Description,        0x5a)     \
X(BinaryScale,        0x5b)     \
X(DecimalScale,       0x5c)     \
X(Small,              0x5d)     \
X(DecimalSign,        0x5e)     \
X(DigitCount,         0x5f)     \
X(PictureString,      0x60)     \
X(Mutable,            0x61)     \
X(ThreadsScaled,      0x62)     \
X(Explicit,           0x63)     \
X(ObjectPointer,      0x64)     \
X(Endianity,          0x65)     \
X(Elemental,          0x66)     \
X(Pure,               0x67)     \
X(Recursive,          0x68)

#define DW_AttribKind_ClassFlags_V3_XList(X)                                                                       \
X(Sibling,            DW_AttribClass_Reference)                                                                  \
X(Location,           DW_AttribClass_Block|DW_AttribClass_LocListPtr)                                            \
X(Name,               DW_AttribClass_String)                                                                     \
X(Ordering,           DW_AttribClass_Const)                                                                      \
X(ByteSize,           DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_Reference)                        \
X(BitOffset,          DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_Reference)                        \
X(BitSize,            DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_Reference)                        \
X(StmtList,           DW_AttribClass_LinePtr)                                                                    \
X(LowPc,              DW_AttribClass_Address)                                                                    \
X(HighPc,             DW_AttribClass_Address)                                                                    \
X(Language,           DW_AttribClass_Const)                                                                      \
X(Discr,              DW_AttribClass_Reference)                                                                  \
X(DiscrValue,         DW_AttribClass_Const)                                                                      \
X(Visibility,         DW_AttribClass_Const)                                                                      \
X(Import,             DW_AttribClass_Reference)                                                                  \
X(StringLength,       DW_AttribClass_Block|DW_AttribClass_LocListPtr)                                            \
X(CommonReference,    DW_AttribClass_Reference)                                                                  \
X(CompDir,            DW_AttribClass_String)                                                                     \
X(ConstValue,         DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_String)                           \
X(ContainingType,     DW_AttribClass_Reference)                                                                  \
X(DefaultValue,       DW_AttribClass_Reference)                                                                  \
X(Inline,             DW_AttribClass_Const)                                                                      \
X(IsOptional,         DW_AttribClass_Flag)                                                                       \
X(LowerBound,         DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_Reference)                        \
X(Producer,           DW_AttribClass_String)                                                                     \
X(Prototyped,         DW_AttribClass_Flag)                                                                       \
X(ReturnAddr,         DW_AttribClass_Block|DW_AttribClass_LocListPtr)                                            \
X(StartScope,         DW_AttribClass_Const)                                                                      \
X(BitStride,          DW_AttribClass_Const)                                                                      \
X(UpperBound,         DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_Reference)                        \
X(AbstractOrigin,     DW_AttribClass_Reference)                                                                  \
X(Accessibility,      DW_AttribClass_Const)                                                                      \
X(AddressClass,       DW_AttribClass_Const)                                                                      \
X(Artificial,         DW_AttribClass_Flag)                                                                       \
X(BaseTypes,          DW_AttribClass_Reference)                                                                  \
X(CallingConvention,  DW_AttribClass_Const)                                                                      \
X(Count,              DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_Reference)                        \
X(DataMemberLocation, DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_LocListPtr)                       \
X(DeclColumn,         DW_AttribClass_Const)                                                                      \
X(DeclFile,           DW_AttribClass_Const)                                                                      \
X(DeclLine,           DW_AttribClass_Const)                                                                      \
X(Declaration,        DW_AttribClass_Flag)                                                                       \
X(DiscrList,          DW_AttribClass_Block)                                                                      \
X(Encoding,           DW_AttribClass_Const)                                                                      \
X(External,           DW_AttribClass_Flag)                                                                       \
X(FrameBase,          DW_AttribClass_Block|DW_AttribClass_LocListPtr)                                            \
X(Friend,             DW_AttribClass_Reference)                                                                  \
X(IdentifierCase,     DW_AttribClass_Const)                                                                      \
X(MacroInfo,          DW_AttribClass_MacPtr)                                                                     \
X(NameListItem,       DW_AttribClass_Block)                                                                      \
X(Priority,           DW_AttribClass_Reference)                                                                  \
X(Segment,            DW_AttribClass_Block|DW_AttribClass_LocListPtr)                                            \
X(Specification,      DW_AttribClass_Reference)                                                                  \
X(StaticLink,         DW_AttribClass_Block|DW_AttribClass_LocListPtr)                                            \
X(Type,               DW_AttribClass_Reference)                                                                  \
X(UseLocation,        DW_AttribClass_Block|DW_AttribClass_LocListPtr)                                            \
X(VariableParameter,  DW_AttribClass_Flag)                                                                       \
X(Virtuality,         DW_AttribClass_Const)                                                                      \
X(VTableElemLocation, DW_AttribClass_Block|DW_AttribClass_LocListPtr)                                            \
X(Allocated,          DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_Reference)                        \
X(Associated,         DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_Reference)                        \
X(DataLocation,       DW_AttribClass_Block)                                                                      \
X(ByteStride,         DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_Reference)                        \
X(EntryPc,            DW_AttribClass_Address)                                                                    \
X(UseUtf8,            DW_AttribClass_Flag)                                                                       \
X(Extension,          DW_AttribClass_Reference)                                                                  \
X(Ranges,             DW_AttribClass_RngListPtr)                                                                 \
X(Trampoline,         DW_AttribClass_Address|DW_AttribClass_Flag|DW_AttribClass_Reference|DW_AttribClass_String) \
X(CallColumn,         DW_AttribClass_Const)                                                                      \
X(CallFile,           DW_AttribClass_Const)                                                                      \
X(CallLine,           DW_AttribClass_Const)                                                                      \
X(Description,        DW_AttribClass_String)                                                                     \
X(BinaryScale,        DW_AttribClass_Const)                                                                      \
X(DecimalScale,       DW_AttribClass_Const)                                                                      \
X(Small,              DW_AttribClass_Reference)                                                                  \
X(DecimalSign,        DW_AttribClass_Const)                                                                      \
X(DigitCount,         DW_AttribClass_Const)                                                                      \
X(PictureString,      DW_AttribClass_String)                                                                     \
X(Mutable,            DW_AttribClass_Flag)                                                                       \
X(ThreadsScaled,      DW_AttribClass_Flag)                                                                       \
X(Explicit,           DW_AttribClass_Flag)                                                                       \
X(ObjectPointer,      DW_AttribClass_Reference)                                                                  \
X(Endianity,          DW_AttribClass_Const)                                                                      \
X(Elemental,          DW_AttribClass_Flag)                                                                       \
X(Pure,               DW_AttribClass_Flag)                                                                       \
X(Recursive,          DW_AttribClass_Flag)

//- Attributes DWARF4

#define DW_AttribKind_V4_XList(X) \
X(Signature,      0x69)         \
X(MainSubProgram, 0x6a)         \
X(DataBitOffset,  0x6b)         \
X(ConstExpr,      0x6c)         \
X(EnumClass,      0x6d)         \
X(LinkageName,    0x6e)

#define DW_AttribKind_ClassFlags_V4_XList(X)                                                                       \
X(Sibling,            DW_AttribClass_Reference)                                                                  \
X(Location,           DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(Name,               DW_AttribClass_String)                                                                     \
X(Ordering,           DW_AttribClass_Const)                                                                      \
X(ByteSize,           DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(BitOffset,          DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(BitSize,            DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(StmtList,           DW_AttribClass_LinePtr)                                                                    \
X(LowPc,              DW_AttribClass_Address)                                                                    \
X(HighPc,             DW_AttribClass_Address|DW_AttribClass_Const)                                               \
X(Language,           DW_AttribClass_Const)                                                                      \
X(Discr,              DW_AttribClass_Reference)                                                                  \
X(DiscrValue,         DW_AttribClass_Const)                                                                      \
X(Visibility,         DW_AttribClass_Const)                                                                      \
X(Import,             DW_AttribClass_Reference)                                                                  \
X(StringLength,       DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(CommonReference,    DW_AttribClass_Reference)                                                                  \
X(CompDir,            DW_AttribClass_String)                                                                     \
X(ConstValue,         DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_String)                           \
X(ContainingType,     DW_AttribClass_Reference)                                                                  \
X(DefaultValue,       DW_AttribClass_Reference)                                                                  \
X(Inline,             DW_AttribClass_Const)                                                                      \
X(IsOptional,         DW_AttribClass_Flag)                                                                       \
X(LowerBound,         DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(Producer,           DW_AttribClass_String)                                                                     \
X(Prototyped,         DW_AttribClass_Flag)                                                                       \
X(ReturnAddr,         DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(StartScope,         DW_AttribClass_Const|DW_AttribClass_RngListPtr)                                            \
X(BitStride,          DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(UpperBound,         DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(AbstractOrigin,     DW_AttribClass_Reference)                                                                  \
X(Accessibility,      DW_AttribClass_Const)                                                                      \
X(AddressClass,       DW_AttribClass_Const)                                                                      \
X(Artificial,         DW_AttribClass_Flag)                                                                       \
X(BaseTypes,          DW_AttribClass_Reference)                                                                  \
X(CallingConvention,  DW_AttribClass_Const)                                                                      \
X(Count,              DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(DataMemberLocation, DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                     \
X(DeclColumn,         DW_AttribClass_Const)                                                                      \
X(DeclFile,           DW_AttribClass_Const)                                                                      \
X(DeclLine,           DW_AttribClass_Const)                                                                      \
X(Declaration,        DW_AttribClass_Flag)                                                                       \
X(DiscrList,          DW_AttribClass_Block)                                                                      \
X(Encoding,           DW_AttribClass_Const)                                                                      \
X(External,           DW_AttribClass_Flag)                                                                       \
X(FrameBase,          DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(Friend,             DW_AttribClass_Reference)                                                                  \
X(IdentifierCase,     DW_AttribClass_Const)                                                                      \
X(MacroInfo,          DW_AttribClass_MacPtr)                                                                     \
X(NameListItem,       DW_AttribClass_Reference)                                                                  \
X(Priority,           DW_AttribClass_Reference)                                                                  \
X(Segment,            DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(Specification,      DW_AttribClass_Reference)                                                                  \
X(StaticLink,         DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(Type,               DW_AttribClass_Reference)                                                                  \
X(UseLocation,        DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(VariableParameter,  DW_AttribClass_Flag)                                                                       \
X(Virtuality,         DW_AttribClass_Const)                                                                      \
X(VTableElemLocation, DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(Allocated,          DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(Associated,         DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(DataLocation,       DW_AttribClass_ExprLoc)                                                                    \
X(ByteStride,         DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(EntryPc,            DW_AttribClass_Address)                                                                    \
X(UseUtf8,            DW_AttribClass_Flag)                                                                       \
X(Extension,          DW_AttribClass_Reference)                                                                  \
X(Ranges,             DW_AttribClass_RngListPtr)                                                                 \
X(Trampoline,         DW_AttribClass_Address|DW_AttribClass_Flag|DW_AttribClass_Reference|DW_AttribClass_String) \
X(CallColumn,         DW_AttribClass_Const)                                                                      \
X(CallFile,           DW_AttribClass_Const)                                                                      \
X(CallLine,           DW_AttribClass_Const)                                                                      \
X(Description,        DW_AttribClass_String)                                                                     \
X(BinaryScale,        DW_AttribClass_Const)                                                                      \
X(DecimalScale,       DW_AttribClass_Const)                                                                      \
X(Small,              DW_AttribClass_Reference)                                                                  \
X(DecimalSign,        DW_AttribClass_Const)                                                                      \
X(DigitCount,         DW_AttribClass_Const)                                                                      \
X(PictureString,      DW_AttribClass_String)                                                                     \
X(Mutable,            DW_AttribClass_Flag)                                                                       \
X(ThreadsScaled,      DW_AttribClass_Flag)                                                                       \
X(Explicit,           DW_AttribClass_Flag)                                                                       \
X(ObjectPointer,      DW_AttribClass_Reference)                                                                  \
X(Endianity,          DW_AttribClass_Const)                                                                      \
X(Elemental,          DW_AttribClass_Flag)                                                                       \
X(Pure,               DW_AttribClass_Flag)                                                                       \
X(Recursive,          DW_AttribClass_Flag)                                                                       \
X(Signature,          DW_AttribClass_Reference)                                                                  \
X(MainSubProgram,     DW_AttribClass_Flag)                                                                       \
X(DataBitOffset,      DW_AttribClass_Const)                                                                      \
X(ConstExpr,          DW_AttribClass_Flag)                                                                       \
X(EnumClass,          DW_AttribClass_Flag)                                                                       \
X(LinkageName,        DW_AttribClass_String)

//- Attributes DWARF5

#define DW_AttribKind_V5_XList(X) \
X(StringLengthBitSize,  0x6f)   \
X(StringLengthByteSize, 0x70)   \
X(Rank,                 0x71)   \
X(StrOffsetsBase,       0x72)   \
X(AddrBase,             0x73)   \
X(RngListsBase,         0x74)   \
X(DwoName,              0x76)   \
X(Reference,            0x77)   \
X(RValueReference,      0x78)   \
X(Macros,               0x79)   \
X(CallAllCalls,         0x7a)   \
X(CallAllSourceCalls,   0x7b)   \
X(CallAllTailCalls,     0x7c)   \
X(CallReturnPc,         0x7d)   \
X(CallValue,            0x7e)   \
X(CallOrigin,           0x7f)   \
X(CallParameter,        0x80)   \
X(CallPc,               0x81)   \
X(CallTailCall,         0x82)   \
X(CallTarget,           0x83)   \
X(CallTargetClobbered,  0x84)   \
X(CallDataLocation,     0x85)   \
X(CallDataValue,        0x86)   \
X(NoReturn,             0x87)   \
X(Alignment,            0x88)   \
X(ExportSymbols,        0x89)   \
X(Deleted,              0x8a)   \
X(Defaulted,            0x8b)   \
X(LocListsBase,         0x8c)

#define DW_AttribKind_ClassFlags_V5_XList(X)                                                                         \
X(Sibling,              DW_AttribClass_Reference)                                                                  \
X(Location,             DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(Name,                 DW_AttribClass_String)                                                                     \
X(Ordering,             DW_AttribClass_Const)                                                                      \
X(ByteSize,             DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(BitOffset,            DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(BitSize,              DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(StmtList,             DW_AttribClass_LinePtr)                                                                    \
X(LowPc,                DW_AttribClass_Address)                                                                    \
X(HighPc,               DW_AttribClass_Address|DW_AttribClass_Const)                                               \
X(Language,             DW_AttribClass_Const)                                                                      \
X(Discr,                DW_AttribClass_Reference)                                                                  \
X(DiscrValue,           DW_AttribClass_Const)                                                                      \
X(Visibility,           DW_AttribClass_Const)                                                                      \
X(Import,               DW_AttribClass_Reference)                                                                  \
X(StringLength,         DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(CommonReference,      DW_AttribClass_Reference)                                                                  \
X(CompDir,              DW_AttribClass_String)                                                                     \
X(ConstValue,           DW_AttribClass_Block|DW_AttribClass_Const|DW_AttribClass_String)                           \
X(ContainingType,       DW_AttribClass_Reference)                                                                  \
X(DefaultValue,         DW_AttribClass_Reference)                                                                  \
X(Inline,               DW_AttribClass_Const)                                                                      \
X(IsOptional,           DW_AttribClass_Flag)                                                                       \
X(LowerBound,           DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(Producer,             DW_AttribClass_String)                                                                     \
X(Prototyped,           DW_AttribClass_Flag)                                                                       \
X(ReturnAddr,           DW_AttribClass_ExprLoc|DW_AttribClass_LocListPtr)                                          \
X(StartScope,           DW_AttribClass_Const|DW_AttribClass_RngListPtr)                                            \
X(BitStride,            DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(UpperBound,           DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(AbstractOrigin,       DW_AttribClass_Reference)                                                                  \
X(Accessibility,        DW_AttribClass_Const)                                                                      \
X(AddressClass,         DW_AttribClass_Const)                                                                      \
X(Artificial,           DW_AttribClass_Flag)                                                                       \
X(BaseTypes,            DW_AttribClass_Reference)                                                                  \
X(CallingConvention,    DW_AttribClass_Const)                                                                      \
X(Count,                DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(DataMemberLocation,   DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_LocList)                        \
X(DeclColumn,           DW_AttribClass_Const)                                                                      \
X(DeclFile,             DW_AttribClass_Const)                                                                      \
X(DeclLine,             DW_AttribClass_Const)                                                                      \
X(Declaration,          DW_AttribClass_Flag)                                                                       \
X(DiscrList,            DW_AttribClass_Block)                                                                      \
X(Encoding,             DW_AttribClass_Const)                                                                      \
X(External,             DW_AttribClass_Flag)                                                                       \
X(FrameBase,            DW_AttribClass_ExprLoc|DW_AttribClass_LocList)                                             \
X(Friend,               DW_AttribClass_Reference)                                                                  \
X(IdentifierCase,       DW_AttribClass_Const)                                                                      \
X(MacroInfo,            DW_AttribClass_MacPtr)                                                                     \
X(NameListItem,         DW_AttribClass_Reference)                                                                  \
X(Priority,             DW_AttribClass_Reference)                                                                  \
X(Segment,              DW_AttribClass_ExprLoc|DW_AttribClass_LocList)                                             \
X(Specification,        DW_AttribClass_Reference)                                                                  \
X(StaticLink,           DW_AttribClass_ExprLoc|DW_AttribClass_LocList)                                             \
X(Type,                 DW_AttribClass_Reference)                                                                  \
X(UseLocation,          DW_AttribClass_ExprLoc|DW_AttribClass_LocList)                                             \
X(VariableParameter,    DW_AttribClass_Flag)                                                                       \
X(Virtuality,           DW_AttribClass_Const)                                                                      \
X(VTableElemLocation,   DW_AttribClass_ExprLoc|DW_AttribClass_LocList)                                             \
X(Allocated,            DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(Associated,           DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(DataLocation,         DW_AttribClass_ExprLoc)                                                                    \
X(ByteStride,           DW_AttribClass_Const|DW_AttribClass_ExprLoc|DW_AttribClass_Reference)                      \
X(EntryPc,              DW_AttribClass_Address|DW_AttribClass_Const)                                               \
X(UseUtf8,              DW_AttribClass_Flag)                                                                       \
X(Extension,            DW_AttribClass_Reference)                                                                  \
X(Ranges,               DW_AttribClass_RngList)                                                                    \
X(Trampoline,           DW_AttribClass_Address|DW_AttribClass_Flag|DW_AttribClass_Reference|DW_AttribClass_String) \
X(CallColumn,           DW_AttribClass_Const)                                                                      \
X(CallFile,             DW_AttribClass_Const)                                                                      \
X(CallLine,             DW_AttribClass_Const)                                                                      \
X(Description,          DW_AttribClass_String)                                                                     \
X(BinaryScale,          DW_AttribClass_Const)                                                                      \
X(DecimalScale,         DW_AttribClass_Const)                                                                      \
X(Small,                DW_AttribClass_Reference)                                                                  \
X(DecimalSign,          DW_AttribClass_Const)                                                                      \
X(DigitCount,           DW_AttribClass_Const)                                                                      \
X(PictureString,        DW_AttribClass_String)                                                                     \
X(Mutable,              DW_AttribClass_Flag)                                                                       \
X(ThreadsScaled,        DW_AttribClass_Flag)                                                                       \
X(Explicit,             DW_AttribClass_Flag)                                                                       \
X(ObjectPointer,        DW_AttribClass_Reference)                                                                  \
X(Endianity,            DW_AttribClass_Const)                                                                      \
X(Elemental,            DW_AttribClass_Flag)                                                                       \
X(Pure,                 DW_AttribClass_Flag)                                                                       \
X(Recursive,            DW_AttribClass_Flag)                                                                       \
X(Signature,            DW_AttribClass_Reference)                                                                  \
X(MainSubProgram,       DW_AttribClass_Flag)                                                                       \
X(DataBitOffset,        DW_AttribClass_Const)                                                                      \
X(ConstExpr,            DW_AttribClass_Flag)                                                                       \
X(EnumClass,            DW_AttribClass_Flag)                                                                       \
X(LinkageName,          DW_AttribClass_String)                                                                     \
X(StringLengthBitSize,  DW_AttribClass_Const)                                                                      \
X(StringLengthByteSize, DW_AttribClass_Const)                                                                      \
X(Rank,                 DW_AttribClass_Const|DW_AttribClass_ExprLoc)                                               \
X(StrOffsetsBase,       DW_AttribClass_StrOffsetsPtr)                                                              \
X(AddrBase,             DW_AttribClass_AddrPtr)                                                                    \
X(RngListsBase,         DW_AttribClass_RngListPtr)                                                                 \
X(DwoName,              DW_AttribClass_String)                                                                     \
X(Reference,            DW_AttribClass_Flag)                                                                       \
X(RValueReference,      DW_AttribClass_Flag)                                                                       \
X(Macros,               DW_AttribClass_MacPtr)                                                                     \
X(CallAllCalls,         DW_AttribClass_Flag)                                                                       \
X(CallAllSourceCalls,   DW_AttribClass_Flag)                                                                       \
X(CallAllTailCalls,     DW_AttribClass_Flag)                                                                       \
X(CallReturnPc,         DW_AttribClass_Address)                                                                    \
X(CallValue,            DW_AttribClass_ExprLoc)                                                                    \
X(CallOrigin,           DW_AttribClass_ExprLoc)                                                                    \
X(CallParameter,        DW_AttribClass_Reference)                                                                  \
X(CallPc,               DW_AttribClass_Address)                                                                    \
X(CallTailCall,         DW_AttribClass_Flag)                                                                       \
X(CallTarget,           DW_AttribClass_ExprLoc)                                                                    \
X(CallTargetClobbered,  DW_AttribClass_ExprLoc)                                                                    \
X(CallDataLocation,     DW_AttribClass_ExprLoc)                                                                    \
X(CallDataValue,        DW_AttribClass_ExprLoc)                                                                    \
X(NoReturn,             DW_AttribClass_Flag)                                                                       \
X(Alignment,            DW_AttribClass_Const)                                                                      \
X(ExportSymbols,        DW_AttribClass_Flag)                                                                       \
X(Deleted,              DW_AttribClass_Flag)                                                                       \
X(Defaulted,            DW_AttribClass_Const)                                                                      \
X(LocListsBase,         DW_AttribClass_LocListPtr)

//- Attributes GNU

#define DW_AttribKind_GNU_XList(X)       \
X(GNU_Vector,                  0x2107) \
X(GNU_GuardedBy,               0x2108) \
X(GNU_PtGuardedBy,             0x2109) \
X(GNU_Guarded,                 0x210a) \
X(GNU_PtGuarded,               0x210b) \
X(GNU_LocksExcluded,           0x210c) \
X(GNU_ExclusiveLocksRequired,  0x210d) \
X(GNU_SharedLocksRequired,     0x210e) \
X(GNU_OdrSignature,            0x210f) \
X(GNU_TemplateName,            0x2110) \
X(GNU_CallSiteValue,           0x2111) \
X(GNU_CallSiteDataValue,       0x2112) \
X(GNU_CallSiteTarget,          0x2113) \
X(GNU_CallSiteTargetClobbered, 0x2114) \
X(GNU_TailCall,                0x2115) \
X(GNU_AllTailCallsSites,       0x2116) \
X(GNU_AllCallSites,            0x2117) \
X(GNU_AllSourceCallSites,      0x2118) \
X(GNU_Macros,                  0x2119) \
X(GNU_Deleted,                 0x211a) \
X(GNU_DwoName,                 0x2130) \
X(GNU_DwoId,                   0x2131) \
X(GNU_RangesBase,              0x2132) \
X(GNU_AddrBase,                0x2133) \
X(GNU_PubNames,                0x2134) \
X(GNU_PubTypes,                0x2135) \
X(GNU_Discriminator,           0x2136) \
X(GNU_LocViews,                0x2137) \
X(GNU_EntryView,               0x2138) \
X(GNU_DescriptiveType,         0x2302) \
X(GNU_Numerator,               0x2303) \
X(GNU_Denominator,             0x2304) \
X(GNU_Bias,                    0x2305)

#define DW_AttribKind_ClassFlags_GNU_XList(X)              \
X(GNU_Vector,                  DW_AttribClass_Flag)      \
X(GNU_GuardedBy,               DW_AttribClass_Undefined) \
X(GNU_PtGuardedBy,             DW_AttribClass_Undefined) \
X(GNU_Guarded,                 DW_AttribClass_Undefined) \
X(GNU_PtGuarded,               DW_AttribClass_Undefined) \
X(GNU_LocksExcluded,           DW_AttribClass_Undefined) \
X(GNU_ExclusiveLocksRequired,  DW_AttribClass_Undefined) \
X(GNU_SharedLocksRequired,     DW_AttribClass_Undefined) \
X(GNU_OdrSignature,            DW_AttribClass_Undefined) \
X(GNU_TemplateName,            DW_AttribClass_Undefined) \
X(GNU_CallSiteValue,           DW_AttribClass_ExprLoc)   \
X(GNU_CallSiteDataValue,       DW_AttribClass_ExprLoc)   \
X(GNU_CallSiteTarget,          DW_AttribClass_ExprLoc)   \
X(GNU_CallSiteTargetClobbered, DW_AttribClass_ExprLoc)   \
X(GNU_TailCall,                DW_AttribClass_Flag)      \
X(GNU_AllTailCallsSites,       DW_AttribClass_Flag)      \
X(GNU_AllCallSites,            DW_AttribClass_Flag)      \
X(GNU_AllSourceCallSites,      DW_AttribClass_Flag)      \
X(GNU_Macros,                  DW_AttribClass_Flag)      \
X(GNU_Deleted,                 DW_AttribClass_Undefined) \
X(GNU_DwoName,                 DW_AttribClass_String)    \
X(GNU_DwoId,                   DW_AttribClass_Const)     \
X(GNU_RangesBase,              DW_AttribClass_Undefined) \
X(GNU_AddrBase,                DW_AttribClass_AddrPtr)   \
X(GNU_PubNames,                DW_AttribClass_Flag)      \
X(GNU_PubTypes,                DW_AttribClass_Undefined) \
X(GNU_Discriminator,           DW_AttribClass_Const)     \
X(GNU_LocViews,                DW_AttribClass_Undefined) \
X(GNU_EntryView,               DW_AttribClass_Undefined) \
X(GNU_DescriptiveType,         DW_AttribClass_Undefined) \
X(GNU_Numerator,               DW_AttribClass_Undefined) \
X(GNU_Denominator,             DW_AttribClass_Undefined) \
X(GNU_Bias,                    DW_AttribClass_Undefined)

//- Attributes LLVM

#define DW_AttribKind_LLVM_XList(X) \
X(LLVM_IncludePath,  0x3e00)      \
X(LLVM_ConfigMacros, 0x3e01)      \
X(LLVM_SysRoot,      0x3e02)      \
X(LLVM_TagOffset,    0x3e03)      \
X(LLVM_ApiNotes,     0x3e07)

#define DW_AttribKind_ClassFlags_LLVM_XList(X)   \
X(LLVM_IncludePath,  DW_AttribClass_String)    \
X(LLVM_ConfigMacros, DW_AttribClass_String)    \
X(LLVM_SysRoot,      DW_AttribClass_String)    \
X(LLVM_TagOffset,    DW_AttribClass_Undefined) \
X(LLVM_ApiNotes,     DW_AttribClass_String)

//- Attributes Apple

#define DW_AttribKind_APPLE_XList(X) \
X(APPLE_Optimized,         0x3fe1) \
X(APPLE_Flags,             0x3fe2) \
X(APPLE_Isa,               0x3fe3) \
X(APPLE_Block,             0x3fe4) \
X(APPLE_MajorRuntimeVers,  0x3fe5) \
X(APPLE_RuntimeClass,      0x3fe6) \
X(APPLE_OmitFramePtr,      0x3fe7) \
X(APPLE_PropertyName,      0x3fe8) \
X(APPLE_PropertyGetter,    0x3fe9) \
X(APPLE_PropertySetter,    0x3fea) \
X(APPLE_PropertyAttribute, 0x3feb) \
X(APPLE_ObjcCompleteType,  0x3fec) \
X(APPLE_Property,          0x3fed) \
X(APPLE_ObjDirect,         0x3fee) \
X(APPLE_Sdk,               0x3fef)

#define DW_AttribKind_ClassFlags_APPLE_XList(X)        \
X(APPLE_Optimized,         DW_AttribClass_Flag)      \
X(APPLE_Flags,             DW_AttribClass_Flag)      \
X(APPLE_Isa,               DW_AttribClass_Flag)      \
X(APPLE_Block,             DW_AttribClass_Undefined) \
X(APPLE_MajorRuntimeVers,  DW_AttribClass_Undefined) \
X(APPLE_RuntimeClass,      DW_AttribClass_Undefined) \
X(APPLE_OmitFramePtr,      DW_AttribClass_Flag)      \
X(APPLE_PropertyName,      DW_AttribClass_Undefined) \
X(APPLE_PropertyGetter,    DW_AttribClass_Undefined) \
X(APPLE_PropertySetter,    DW_AttribClass_Undefined) \
X(APPLE_PropertyAttribute, DW_AttribClass_Undefined) \
X(APPLE_ObjcCompleteType,  DW_AttribClass_Undefined) \
X(APPLE_Property,          DW_AttribClass_Undefined) \
X(APPLE_ObjDirect,         DW_AttribClass_Undefined) \
X(APPLE_Sdk,               DW_AttribClass_String)

//- Attributes MIPS

#define DW_AttribKind_MIPS_XList(X)     \
X(MIPS_Fde,                   0x2001) \
X(MIPS_LoopBegin,             0x2002) \
X(MIPS_TailLoopBegin,         0x2003) \
X(MIPS_EpilogBegin,           0x2004) \
X(MIPS_LoopUnrollFactor,      0x2005) \
X(MIPS_SoftwarePipelineDepth, 0x2006) \
X(MIPS_LinkageName,           0x2007) \
X(MIPS_Stride,                0x2008) \
X(MIPS_AbstractName,          0x2009) \
X(MIPS_CloneOrigin,           0x200a) \
X(MIPS_HasInlines,            0x200b) \
X(MIPS_StrideByte,            0x200c) \
X(MIPS_StrideElem,            0x200d) \
X(MIPS_PtrDopeType,           0x200e) \
X(MIPS_AllocatableDopeType,   0x200f) \
X(MIPS_AssumedShapeDopeType,  0x2010) \
X(MIPS_AssumedSize,           0x2011)

#define DW_AttribKind_ClassFlags_MIPS_XList(X)            \
X(MIPS_Fde,                   DW_AttribClass_Block)     \
X(MIPS_LoopBegin,             DW_AttribClass_Block)     \
X(MIPS_TailLoopBegin,         DW_AttribClass_Block)     \
X(MIPS_EpilogBegin,           DW_AttribClass_Block)     \
X(MIPS_LoopUnrollFactor,      DW_AttribClass_Block)     \
X(MIPS_SoftwarePipelineDepth, DW_AttribClass_Block)     \
X(MIPS_LinkageName,           DW_AttribClass_String)    \
X(MIPS_Stride,                DW_AttribClass_Block)     \
X(MIPS_AbstractName,          DW_AttribClass_String)    \
X(MIPS_CloneOrigin,           DW_AttribClass_String)    \
X(MIPS_HasInlines,            DW_AttribClass_Reference) \
X(MIPS_StrideByte,            DW_AttribClass_Reference) \
X(MIPS_StrideElem,            DW_AttribClass_Reference) \
X(MIPS_PtrDopeType,           DW_AttribClass_Reference) \
X(MIPS_AllocatableDopeType,   DW_AttribClass_Reference) \
X(MIPS_AssumedShapeDopeType,  DW_AttribClass_Reference) \
X(MIPS_AssumedSize,           DW_AttribClass_Reference)

typedef U32 DW_AttribKind;
typedef enum DW_AttribKindEnum
{
  DW_Attrib_Null,
#define X(_N,_ID,...) DW_Attrib_##_N = _ID,
  DW_AttribKind_V2_XList(X)
    DW_AttribKind_V3_XList(X)
    DW_AttribKind_V4_XList(X)
    DW_AttribKind_V5_XList(X)
    DW_AttribKind_GNU_XList(X)
    DW_AttribKind_LLVM_XList(X)
    DW_AttribKind_APPLE_XList(X)
    DW_AttribKind_MIPS_XList(X)
#undef X
  DW_Attrib_UserLo = 0x2000,
  DW_Attrib_UserHi = 0x3fff
} DW_AttribKindEnum;

#define DW_ATE_XList(X)   \
X(Null,           0x00) \
X(Address,        0x01) \
X(Boolean,        0x02) \
X(ComplexFloat,   0x03) \
X(Float,          0x04) \
X(Signed,         0x05) \
X(SignedChar,     0x06) \
X(Unsigned,       0x07) \
X(UnsignedChar,   0x08) \
X(ImaginaryFloat, 0x09) \
X(PackedDecimal,  0x0A) \
X(NumericString,  0x0B) \
X(Edited,         0x0C) \
X(SignedFixed,    0x0D) \
X(UnsignedFixed,  0x0E) \
X(DecimalFloat,   0x0F) \
X(Utf,            0x10) \
X(Ucs,            0x11) \
X(Ascii,          0x12)

typedef U64 DW_ATE;
typedef enum DW_ATEEnum
{
#define X(_N,_ID) DW_ATE_##_N = _ID,
  DW_ATE_XList(X)
#undef X
} DW_ATEnum;

#define DW_CallingConventionKind_XList(X) \
X(Normal,          0x0)                 \
X(Program,         0x1)                 \
X(NoCall,          0x3)                 \
X(PassByValue,     0x4)                 \
X(PassByReference, 0x5)

typedef U64 DW_CallingConventionKind;
typedef enum DW_CallingConventionKindEnum
{
#define X(_N,_ID) DW_CallingConventionKind_##_N = _ID,
  DW_CallingConventionKind_XList(X)
#undef X
} DW_CallingConventionKindEnum;

#define DW_AccessKind_XList(X) \
X(Public,    0x00) \
X(Private,   0x01) \
X(Protected, 0x02)

typedef U64 DW_AccessKind;
typedef enum DW_AccessKindEnum
{
#define X(_N,_ID) DW_AccessKind_##_N = _ID,
  DW_AccessKind_XList(X)
#undef X
} DW_AccessKindEnum;

#define DW_VirtualityKind_XList(X) \
X(None,        0x00)             \
X(Virtual,     0x01)             \
X(PureVirtual, 0x02)

typedef U64 DW_VirtualityKind;
typedef enum DW_VirtualityEnum
{
#define X(_N,_ID) DW_VirtualityKind_##_N = _ID,
  DW_VirtualityKind_XList(X)
#undef X
} DW_VirtualityEnum;

#define DW_RngListEntryKind(X) \
X(EndOfList,    0x00)        \
X(BaseAddressx, 0x01)        \
X(StartxEndx,   0x02)        \
X(StartxLength, 0x03)        \
X(OffsetPair,   0x04)        \
X(BaseAddress,  0x05)        \
X(StartEnd,     0x06)        \
X(StartLength,  0x07)

typedef U8 DW_RLE;
typedef enum DW_RLE_Enum
{
#define X(_N,_ID) DW_RLE_##_N = _ID,
  DW_RngListEntryKind(X)
#undef X
} DW_RLE_Enum;

#define DW_LocListEntry_XList(X) \
X(EndOfList,       0x00)       \
X(BaseAddressx,    0x01)       \
X(StartxEndx,      0x02)       \
X(StartxLength,    0x03)       \
X(OffsetPair,      0x04)       \
X(DefaultLocation, 0x05)       \
X(BaseAddress,     0x06)       \
X(StartEnd,        0x07)       \
X(StartLength,     0x08)

#define DW_LocListEntry_GNU_XList(X) \
X(GNU_ViewPair, 0x9)

typedef U8 DW_LLE;
typedef enum DW_LLE_Enum
{
#define X(_N,_ID) DW_LLE_##_N = _ID,
  DW_LocListEntry_XList(X)
#undef X
} DW_LLEEnum;

#define DW_AddrClass_XList(X) \
X(None,   0)                \
X(Near16, 1)                \
X(Far16,  2)                \
X(Huge16, 3)                \
X(Near32, 4)                \
X(Far32,  5)

typedef U64 DW_AddrClass;
typedef enum DW_AddrClassEnum
{
#define X(_N, _ID) DW_AddrClassKind_##_N = _ID,
  DW_AddrClass_XList(X)
#undef X
} DW_AddrClassEnum;

#define DW_CompUnitKind_XList(X) \
X(Reserved,     0)             \
X(Compile,      1)             \
X(Type,         2)             \
X(Partial,      3)             \
X(Skeleton,     4)             \
X(SplitCompile, 5)             \
X(SplitType,    6)

typedef U8 DW_CompUnitKind;
typedef enum DW_CompUnitKindEnum
{
#define X(_N, _ID) DW_CompUnitKind_##_N = _ID,
  DW_CompUnitKind_XList(X)
#undef X
  DW_CompUnitKind_UserLo = 0x80,
  DW_CompUnitKind_UserHi = 0xff
} DW_CompUnitKindEnum;

#define DW_LNCT_XList(X) \
X(Path,           0x1)       \
X(DirectoryIndex, 0x2)       \
X(TimeStamp,      0x3)       \
X(Size,           0x4)       \
X(MD5,            0x5)       \
X(LLVM_Source,    0x2001)

typedef U64 DW_LNCT;
typedef enum DW_LNCTEnum
{
#define X(_N, _ID) DW_LNCT_##_N = _ID,
  DW_LNCT_XList(X)
#undef X
  DW_LNCT_UserLo = 0x2000,
  DW_LNCT_UserHi = 0x3fff
} DW_LNCTEnum;

#define DW_CFA_Kind1_XList(X) \
X(Nop,            0x0)      \
X(SetLoc,         0x1)      \
X(AdvanceLoc1,    0x2)      \
X(AdvanceLoc2,    0x3)      \
X(AdvanceLoc4,    0x4)      \
X(OffsetExt,      0x5)      \
X(RestoreExt,     0x6)      \
X(Undefined,      0x7)      \
X(SameValue,      0x8)      \
X(Register,       0x9)      \
X(RememberState,  0xA)      \
X(RestoreState,   0xB)      \
X(DefCfa,         0xC)      \
X(DefCfaRegister, 0xD)      \
X(DefCfaOffset,   0xE)      \
X(DefCfaExpr,     0xF)      \
X(Expr,           0x10)     \
X(OffsetExtSf,    0x11)     \
X(DefCfaSf,       0x12)     \
X(DefCfaOffsetSf, 0x13)     \
X(ValOffset,      0x14)     \
X(ValOffsetSf,    0x15)     \
X(ValExpr,        0x16)

#define DW_CFA_Kind2_XList(X) \
X(AdvanceLoc,        0x40)  \
X(Offset,            0x80)  \
X(Restore,           0xC0)

typedef U8 DW_CFA;
typedef enum DW_CFAEnum
{
#define X(_N, _ID) DW_CFA_##_N = _ID,
  DW_CFA_Kind1_XList(X)
    DW_CFA_Kind2_XList(X)
#undef X
  
  DW_CFA_OplKind1 = DW_CFA_ValExpr,
  DW_CFA_OplKind2 = DW_CFA_Restore,
} DW_CFAEnum;

typedef U8 DW_CFAMask;
enum
{
  //  kind1:  opcode: [0,5] zeroes:[6,7]; kind2:  operand:[0,5] opcode:[6,7] 
  DW_CFAMask_OpcodeHi = 0xC0,
  DW_CFAMask_Operand  = 0x3F,
  DW_CFAMask_Count    = 2
};

////////////////////////////////
// Expression Opcodes

#define DW_Expr_V3_XList(X)  \
X(Null,              0x00) \
X(Addr,              0x03) \
X(Deref,             0x06) \
X(Const1U,           0x08) \
X(Const1S,           0x09) \
X(Const2U,           0x0a) \
X(Const2S,           0x0b) \
X(Const4U,           0x0c) \
X(Const4S,           0x0d) \
X(Const8U,           0x0e) \
X(Const8S,           0x0f) \
X(ConstU,            0x10) \
X(ConstS,            0x11) \
X(Dup,               0x12) \
X(Drop,              0x13) \
X(Over,              0x14) \
X(Pick,              0x15) \
X(Swap,              0x16) \
X(Rot,               0x17) \
X(XDeref,            0x18) \
X(Abs,               0x19) \
X(And,               0x1a) \
X(Div,               0x1b) \
X(Minus,             0x1c) \
X(Mod,               0x1d) \
X(Mul,               0x1e) \
X(Neg,               0x1f) \
X(Not,               0x20) \
X(Or,                0x21) \
X(Plus,              0x22) \
X(PlusUConst,        0x23) \
X(Shl,               0x24) \
X(Shr,               0x25) \
X(Shra,              0x26) \
X(Xor,               0x27) \
X(Skip,              0x2f) \
X(Bra,               0x28) \
X(Eq,                0x29) \
X(Ge,                0x2a) \
X(Gt,                0x2b) \
X(Le,                0x2c) \
X(Lt,                0x2d) \
X(Ne,                0x2e) \
X(Lit0,              0x30) \
X(Lit1,              0x31) \
X(Lit2,              0x32) \
X(Lit3,              0x33) \
X(Lit4,              0x34) \
X(Lit5,              0x35) \
X(Lit6,              0x36) \
X(Lit7,              0x37) \
X(Lit8,              0x38) \
X(Lit9,              0x39) \
X(Lit10,             0x3a) \
X(Lit11,             0x3b) \
X(Lit12,             0x3c) \
X(Lit13,             0x3d) \
X(Lit14,             0x3e) \
X(Lit15,             0x3f) \
X(Lit16,             0x40) \
X(Lit17,             0x41) \
X(Lit18,             0x42) \
X(Lit19,             0x43) \
X(Lit20,             0x44) \
X(Lit21,             0x45) \
X(Lit22,             0x46) \
X(Lit23,             0x47) \
X(Lit24,             0x48) \
X(Lit25,             0x49) \
X(Lit26,             0x4a) \
X(Lit27,             0x4b) \
X(Lit28,             0x4c) \
X(Lit29,             0x4d) \
X(Lit30,             0x4e) \
X(Lit31,             0x4f) \
X(Reg0,              0x50) \
X(Reg1,              0x51) \
X(Reg2,              0x52) \
X(Reg3,              0x53) \
X(Reg4,              0x54) \
X(Reg5,              0x55) \
X(Reg6,              0x56) \
X(Reg7,              0x57) \
X(Reg8,              0x58) \
X(Reg9,              0x59) \
X(Reg10,             0x5a) \
X(Reg11,             0x5b) \
X(Reg12,             0x5c) \
X(Reg13,             0x5d) \
X(Reg14,             0x5e) \
X(Reg15,             0x5f) \
X(Reg16,             0x60) \
X(Reg17,             0x61) \
X(Reg18,             0x62) \
X(Reg19,             0x63) \
X(Reg20,             0x64) \
X(Reg21,             0x65) \
X(Reg22,             0x66) \
X(Reg23,             0x67) \
X(Reg24,             0x68) \
X(Reg25,             0x69) \
X(Reg26,             0x6a) \
X(Reg27,             0x6b) \
X(Reg28,             0x6c) \
X(Reg29,             0x6d) \
X(Reg30,             0x6e) \
X(Reg31,             0x6f) \
X(BReg0,             0x70) \
X(BReg1,             0x71) \
X(BReg2,             0x72) \
X(BReg3,             0x73) \
X(BReg4,             0x74) \
X(BReg5,             0x75) \
X(BReg6,             0x76) \
X(BReg7,             0x77) \
X(BReg8,             0x78) \
X(BReg9,             0x79) \
X(BReg10,            0x7a) \
X(BReg11,            0x7b) \
X(BReg12,            0x7c) \
X(BReg13,            0x7d) \
X(BReg14,            0x7e) \
X(BReg15,            0x7f) \
X(BReg16,            0x80) \
X(BReg17,            0x81) \
X(BReg18,            0x82) \
X(BReg19,            0x83) \
X(BReg20,            0x84) \
X(BReg21,            0x85) \
X(BReg22,            0x86) \
X(BReg23,            0x87) \
X(BReg24,            0x88) \
X(BReg25,            0x89) \
X(BReg26,            0x8a) \
X(BReg27,            0x8b) \
X(BReg28,            0x8c) \
X(BReg29,            0x8d) \
X(BReg30,            0x8e) \
X(BReg31,            0x8f) \
X(RegX,              0x90) \
X(FBReg,             0x91) \
X(BRegX,             0x92) \
X(Piece,             0x93) \
X(DerefSize,         0x94) \
X(XDerefSize,        0x95) \
X(Nop,               0x96) \
X(PushObjectAddress, 0x97) \
X(Call2,             0x98) \
X(Call4,             0x99) \
X(CallRef,           0x9a) \
X(FormTlsAddress,    0x9b) \
X(CallFrameCfa,      0x9c) \
X(BitPiece,          0x9d)

#define DW_Expr_V4_XList(X) \
X(ImplicitValue, 0x9e)    \
X(StackValue,    0x9f)

#define DW_Expr_V5_XList(X) \
X(ImplicitPointer, 0xa0)  \
X(Addrx,           0xa1)  \
X(Constx,          0xa2)  \
X(EntryValue,      0xa3)  \
X(ConstType,       0xa4)  \
X(RegvalType,      0xa5)  \
X(DerefType,       0xa6)  \
X(XderefType,      0xa7)  \
X(Convert,         0xa8)  \
X(ReInterpret,     0xa9)

#define DW_Expr_GNU_XList(X)   \
X(GNU_PushTlsAddress,  0xe0) \
X(GNU_UnInit,          0xf0) \
X(GNU_ImplicitPointer, 0xf2) \
X(GNU_EntryValue,      0xf3) \
X(GNU_ConstType,       0xf4) \
X(GNU_RegvalType,      0xf5) \
X(GNU_DerefType,       0xf6) \
X(GNU_Convert,         0xf7) \
X(GNU_ParameterRef,    0xfa) \
X(GNU_AddrIndex,       0xfb) \
X(GNU_ConstIndex,      0xfc)

typedef U64 DW_ExprOp;
typedef enum DW_ExprOpEnum
{
#define X(_N, _ID) DW_ExprOp_##_N = _ID,
  DW_Expr_V3_XList(X)
    DW_Expr_V4_XList(X)
    DW_Expr_V5_XList(X) 
    DW_Expr_GNU_XList(X)
#undef X
} DW_ExprOpEnum;

//- Regs

#define DW_Regs_X86_XList(X)   \
X(Eax,    0,  eax,    0, 4)  \
X(Ecx,    1,  ecx,    0, 4)  \
X(Edx,    2,  edx,    0, 4)  \
X(Ebx,    3,  ebx,    0, 4)  \
X(Esp,    4,  esp,    0, 4)  \
X(Ebp,    5,  ebp,    0, 4)  \
X(Esi,    6,  esi,    0, 4)  \
X(Edi,    7,  edi,    0, 4)  \
X(Eip,    8,  eip,    0, 4)  \
X(Eflags, 9,  eflags, 0, 4)  \
X(Trapno, 10, nil,    0, 0)  \
X(St0,    11, st0,    0, 10) \
X(St1,    12, st1,    0, 10) \
X(St2,    13, st2,    0, 10) \
X(St3,    14, st3,    0, 10) \
X(St4,    15, st4,    0, 10) \
X(St5,    16, st5,    0, 10) \
X(St6,    17, st6,    0, 10) \
X(St7,    18, st7,    0, 10) \
X(Xmm0,   21, ymm0,   0, 16) \
X(Xmm1,   22, ymm1,   0, 16) \
X(Xmm2,   23, ymm2,   0, 16) \
X(Xmm3,   24, ymm3,   0, 16) \
X(Xmm4,   25, ymm4,   0, 16) \
X(Xmm5,   26, ymm5,   0, 16) \
X(Xmm6,   27, ymm6,   0, 16) \
X(Xmm7,   28, ymm7,   0, 16) \
X(Mm0,    29, fpr0,   0, 8)  \
X(Mm1,    30, fpr1,   0, 8)  \
X(Mm2,    31, fpr2,   0, 8)  \
X(Mm3,    32, fpr3,   0, 8)  \
X(Mm4,    33, fpr4,   0, 8)  \
X(Mm5,    34, fpr5,   0, 8)  \
X(Mm6,    35, fpr6,   0, 8)  \
X(Mm7,    36, fpr7,   0, 8)  \
X(Fcw,    37, fcw,    0, 2)  \
X(Fsw,    38, fsw,    0, 2)  \
X(Mxcsr,  39, mxcsr,  0, 4)  \
X(Es,     40, es,     0, 2)  \
X(Cs,     41, cs,     0, 2)  \
X(Ss,     42, ss,     0, 2)  \
X(Ds,     43, ds,     0, 2)  \
X(Fs,     44, fs,     0, 2)  \
X(Gs,     45, gs,     0, 2)  \
X(Tr,     48, nil,    0, 0)  \
X(Ldtr,   49, nil,    0, 0)

#define DW_Regs_X64_XList(X)    \
X(Rax,     0,  rax,    0, 8)  \
X(Rdx,     1,  rdx,    0, 8)  \
X(Rcx,     2,  rcx,    0, 8)  \
X(Rbx,     3,  rbx,    0, 8)  \
X(Rsi,     4,  rsi,    0, 8)  \
X(Rdi,     5,  rdi,    0, 8)  \
X(Rbp,     6,  rbp,    0, 8)  \
X(Rsp,     7,  rsp,    0, 8)  \
X(R8,      8,  r8,     0, 8)  \
X(R9,      9,  r9,     0, 8)  \
X(R10,     10, r10,    0, 8)  \
X(R11,     11, r11,    0, 8)  \
X(R12,     12, r12,    0, 8)  \
X(R13,     13, r13,    0, 8)  \
X(R14,     14, r14,    0, 8)  \
X(R15,     15, r15,    0, 8)  \
X(Rip,     16, rip,    0, 8)  \
X(Xmm0,    17, zmm0,   0, 16) \
X(Xmm1,    18, zmm1,   0, 16) \
X(Xmm2,    19, zmm2,   0, 16) \
X(Xmm3,    20, zmm3,   0, 16) \
X(Xmm4,    21, zmm4,   0, 16) \
X(Xmm5,    22, zmm5,   0, 16) \
X(Xmm6,    23, zmm6,   0, 16) \
X(Xmm7,    24, zmm7,   0, 16) \
X(Xmm8,    25, zmm8,   0, 16) \
X(Xmm9,    26, zmm9,   0, 16) \
X(Xmm10,   27, zmm10,  0, 16) \
X(Xmm11,   28, zmm11,  0, 16) \
X(Xmm12,   29, zmm12,  0, 16) \
X(Xmm13,   30, zmm13,  0, 16) \
X(Xmm14,   31, zmm14,  0, 16) \
X(Xmm15,   32, zmm15,  0, 16) \
X(Xmm16,   67, zmm16,  0, 16) \
X(Xmm17,   68, zmm17,  0, 16) \
X(Xmm18,   69, zmm18,  0, 16) \
X(Xmm19,   70, zmm19,  0, 16) \
X(Xmm20,   71, zmm20,  0, 16) \
X(Xmm21,   72, zmm21,  0, 16) \
X(Xmm22,   73, zmm22,  0, 16) \
X(Xmm23,   74, zmm23,  0, 16) \
X(Xmm24,   75, zmm24,  0, 16) \
X(Xmm25,   76, zmm25,  0, 16) \
X(Xmm26,   77, zmm26,  0, 16) \
X(Xmm27,   78, zmm27,  0, 16) \
X(Xmm28,   79, zmm28,  0, 16) \
X(Xmm29,   80, zmm29,  0, 16) \
X(Xmm30,   81, zmm30,  0, 16) \
X(Xmm31,   82, zmm31,  0, 16) \
X(St0,     33, st0,    0, 10) \
X(St1,     34, st1,    0, 10) \
X(St2,     35, st2,    0, 10) \
X(St3,     36, st3,    0, 10) \
X(St4,     37, st4,    0, 10) \
X(St5,     38, st5,    0, 10) \
X(St6,     39, st6,    0, 10) \
X(St7,     40, st7,    0, 10) \
X(Mm0,     41, fpr0,   0, 8)  \
X(Mm1,     42, fpr1,   0, 8)  \
X(Mm2,     43, fpr2,   0, 8)  \
X(Mm3,     44, fpr3,   0, 8)  \
X(Mm4,     45, fpr4,   0, 8)  \
X(Mm5,     46, fpr5,   0, 8)  \
X(Mm6,     47, fpr6,   0, 8)  \
X(Mm7,     48, fpr7,   0, 8)  \
X(Rflags,  49, rflags, 0, 4)  \
X(Es,      50, es,     0, 2)  \
X(Cs,      51, cs,     0, 2)  \
X(Ss,      52, ss,     0, 2)  \
X(Ds,      53, ds,     0, 2)  \
X(Fs,      54, fs,     0, 2)  \
X(Gs,      55, gs,     0, 2)  \
X(FsBase,  58, nil,    0, 0)  \
X(GsBase,  59, nil,    0, 0)  \
X(Tr,      62, nil,    0, 0)  \
X(Ldtr,    63, nil,    0, 0)

typedef U32 DW_Reg;

typedef DW_Reg DW_RegX86;
typedef enum DW_RegX86Enum
{
#define X(_N,_ID,...) DW_RegX86_##_N = _ID,
  DW_Regs_X86_XList(X)
#undef X
} DW_RegX86Enum;

typedef DW_Reg DW_RegX64;
typedef enum DW_RegX64Enum
{
#define X(_N,_ID,...) DW_RegX64_##_N = _ID,
  DW_Regs_X64_XList(X)
#undef X
} DW_RegX64Enum;

////////////////////////////////

internal U64 dw_reg_size_from_code_x86(DW_Reg reg_code);
internal U64 dw_reg_pos_from_code_x86(DW_Reg reg_code);
internal U64 dw_reg_size_from_code_x64(DW_Reg reg_code);
internal U64 dw_reg_pos_from_code_x64(DW_Reg reg_code);
internal U64 dw_reg_size_from_code(Arch arch, DW_Reg reg_code);
internal U64 dw_reg_pos_from_code(Arch arch, DW_Reg reg_code);

//- Attrib Class Encodings

// Speced Encodings
internal DW_AttribClass dw_attrib_class_from_attrib_kind_v2(DW_AttribKind k);
internal DW_AttribClass dw_attrib_class_from_attrib_kind_v3(DW_AttribKind k);
internal DW_AttribClass dw_attrib_class_from_attrib_kind_v4(DW_AttribKind k);
internal DW_AttribClass dw_attrib_class_from_attrib_kind_v5(DW_AttribKind k);

// Extensions
internal DW_AttribClass dw_attrib_class_from_attrib_kind_gnu  (DW_AttribKind k);
internal DW_AttribClass dw_attrib_class_from_attrib_kind_llvm (DW_AttribKind k);
internal DW_AttribClass dw_attrib_class_from_attrib_kind_apple(DW_AttribKind k);
internal DW_AttribClass dw_attrib_class_from_attrib_kind_mips (DW_AttribKind k);

internal DW_AttribClass dw_attrib_class_from_attrib_kind(DW_Version ver, DW_Ext ext, DW_AttribKind v);

//- Form Class Encodings

internal DW_AttribClass dw_attrib_class_from_form_kind(DW_Version ver, DW_FormKind k);

internal B32 dw_are_attrib_class_and_form_kind_compatible(DW_Version ver, DW_AttribClass attrib_class, DW_FormKind form_kind);

//- Section Names

internal String8 dw_name_string_from_section_kind     (DW_SectionKind k);
internal String8 dw_mach_name_string_from_section_kind(DW_SectionKind k);
internal String8 dw_dwo_name_string_from_section_kind (DW_SectionKind k);

////////////////////////////////

internal U64 dw_size_from_format(DW_Format format);

////////////////////////////////

internal DW_AttribClass dw_pick_attrib_value_class(DW_Version ver, DW_Ext ext, B32 relaxed, DW_AttribKind attrib, DW_FormKind form_kind);

internal U64 dw_pick_default_lower_bound(DW_Language lang);

#endif // DWARF_H
