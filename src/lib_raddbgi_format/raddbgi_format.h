// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////////////////////////////////////
// RAD Debug Info, (R)AD(D)BG(I) Format Library
//
// Defines standard RADDBGI debug information format types and
// functions.

#ifndef RADDBGI_FORMAT_H
#define RADDBGI_FORMAT_H

////////////////////////////////////////////////////////////////
// Overridable procedure decoration

#if !defined(RDI_PROC)
# define RDI_PROC static
#endif

////////////////////////////////////////////////////////////////
// Overridable integer types

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
// Architecture Constants

#define RDI_ArchXList(X)\
X(NULL, 0, 0)\
X(X86,  1, 4)\
X(X64,  2, 8)

typedef RDI_U32 RDI_Arch;
typedef enum RDI_ArchEnum{
#define X(N,C,Z) RDI_Arch_##N = C,
  RDI_ArchXList(X)
#undef X
} RDI_ArchEnum;


typedef RDI_U8 RDI_RegisterCode;

// x86 registers
#define RDI_RegisterCode_X86_XList(X) \
X(nil,             0) \
X(eax,             1) \
X(ecx,             2) \
X(edx,             3) \
X(ebx,             4) \
X(esp,             5) \
X(ebp,             6) \
X(esi,             7) \
X(edi,             8) \
X(fsbase,          9) \
X(gsbase,         10) \
X(eflags,         11) \
X(eip,            12) \
X(dr0,            13) \
X(dr1,            14) \
X(dr2,            15) \
X(dr3,            16) \
X(dr4,            17) \
X(dr5,            18) \
X(dr6,            19) \
X(dr7,            20) \
X(fpr0,           21) \
X(fpr1,           22) \
X(fpr2,           23) \
X(fpr3,           24) \
X(fpr4,           25) \
X(fpr5,           26) \
X(fpr6,           27) \
X(fpr7,           28) \
X(st0,            29) \
X(st1,            30) \
X(st2,            31) \
X(st3,            32) \
X(st4,            33) \
X(st5,            34) \
X(st6,            35) \
X(st7,            36) \
X(fcw,            37) \
X(fsw,            38) \
X(ftw,            39) \
X(fop,            40) \
X(fcs,            41) \
X(fds,            42) \
X(fip,            43) \
X(fdp,            44) \
X(mxcsr,          45) \
X(mxcsr_mask,     46) \
X(ss,             47) \
X(cs,             48) \
X(ds,             49) \
X(es,             50) \
X(fs,             51) \
X(gs,             52) \
X(ymm0,           53) \
X(ymm1,           54) \
X(ymm2,           55) \
X(ymm3,           56) \
X(ymm4,           57) \
X(ymm5,           58) \
X(ymm6,           59) \
X(ymm7,           60) \
X(COUNT,          61)

typedef enum RDI_RegisterCode_X86_Enum{
#define X(N,C) RDI_RegisterCode_X86_##N = C,
  RDI_RegisterCode_X86_XList(X)
#undef X
} RDI_RegisterCode_X86_Enum;

// x64 registers
#define RDI_RegisterCode_X64_XList(X) \
X(nil,             0) \
X(rax,             1) \
X(rcx,             2) \
X(rdx,             3) \
X(rbx,             4) \
X(rsp,             5) \
X(rbp,             6) \
X(rsi,             7) \
X(rdi,             8) \
X(r8,              9) \
X(r9,              10) \
X(r10,             11) \
X(r11,             12) \
X(r12,             13) \
X(r13,             14) \
X(r14,             15) \
X(r15,             16) \
X(es,              17) \
X(cs,              18) \
X(ss,              19) \
X(ds,              20) \
X(fs,              21) \
X(gs,              22) \
X(rip,             23) \
X(rflags,          24) \
X(dr0,             25) \
X(dr1,             26) \
X(dr2,             27) \
X(dr3,             28) \
X(dr4,             29) \
X(dr5,             30) \
X(dr6,             31) \
X(dr7,             32) \
X(st0,             33) \
X(st1,             34) \
X(st2,             35) \
X(st3,             36) \
X(st4,             37) \
X(st5,             38) \
X(st6,             39) \
X(st7,             40) \
X(fpr0,            41) \
X(fpr1,            42) \
X(fpr2,            43) \
X(fpr3,            44) \
X(fpr4,            45) \
X(fpr5,            46) \
X(fpr6,            47) \
X(fpr7,            48) \
X(ymm0,            49) \
X(ymm1,            50) \
X(ymm2,            51) \
X(ymm3,            52) \
X(ymm4,            53) \
X(ymm5,            54) \
X(ymm6,            55) \
X(ymm7,            56) \
X(ymm8,            57) \
X(ymm9,            58) \
X(ymm10,           59) \
X(ymm11,           60) \
X(ymm12,           61) \
X(ymm13,           62) \
X(ymm14,           63) \
X(ymm15,           64) \
X(mxcsr,           65) \
X(fsbase,          66) \
X(gsbase,          67) \
X(fcw,             68) \
X(fsw,             69) \
X(ftw,             70) \
X(fop,             71) \
X(fcs,             72) \
X(fds,             73) \
X(fip,             74) \
X(fdp,             75) \
X(mxcsr_mask,      76) \
X(COUNT,           77)

typedef enum RDI_RegisterCode_X64_Enum{
#define X(N,C) RDI_RegisterCode_X64_##N = C,
  RDI_RegisterCode_X64_XList(X)
#undef X
} RDI_RegisterCode_X64_Enum;


////////////////////////////////////////////////////////////////
// Format types

// "raddbg\0\0"
#define RDI_MAGIC_CONSTANT   0x0000676264646172
#define RDI_ENCODING_VERSION 1

#define RDI_LanguageXList(X) \
X(NULL,      0) \
X(C,         1) \
X(CPlusPlus, 2)

typedef RDI_U32 RDI_Language;
typedef enum RDI_LanguageEnum{
#define X(N,C) RDI_Language_##N = C,
  RDI_LanguageXList(X)
#undef X
} RDI_LanguageEnum;

typedef struct RDI_Header{
  // identification
  RDI_U64 magic;
  RDI_U32 encoding_version;
  
  // data sections
  RDI_U32 data_section_off;
  RDI_U32 data_section_count;
} RDI_Header;


//- data sections

#define RDI_DataSectionTag_SECONDARY 0x80000000

#define RDI_DataSectionTagXList(X,Y) \
X(NULL,                0x0000)\
X(TopLevelInfo,        0x0001)\
X(StringData,          0x0002)\
X(StringTable,         0x0003)\
X(IndexRuns,           0x0004)\
X(BinarySections,      0x0005)\
X(FilePathNodes,       0x0006)\
X(SourceFiles,         0x0007)\
X(Units,               0x0008)\
X(UnitVmap,            0x0009)\
X(TypeNodes,           0x000A)\
X(UDTs,                0x000B)\
X(Members,             0x000C)\
X(EnumMembers,         0x000D)\
X(GlobalVariables,     0x000E)\
X(GlobalVmap,          0x000F)\
X(ThreadVariables,     0x0010)\
X(Procedures,          0x0011)\
X(Scopes,              0x0012)\
X(ScopeVoffData,       0x0013)\
X(ScopeVmap,           0x0014)\
X(Locals,              0x0015)\
X(LocationBlocks,      0x0016)\
X(LocationData,        0x0017)\
X(NameMaps,            0x0018)\
Y(PRIMARY_COUNT)\
X(SKIP,                RDI_DataSectionTag_SECONDARY|0x0000)\
X(LineInfoVoffs,       RDI_DataSectionTag_SECONDARY|0x0001)\
X(LineInfoData,        RDI_DataSectionTag_SECONDARY|0x0002)\
X(LineInfoColumns,     RDI_DataSectionTag_SECONDARY|0x0003)\
X(LineMapNumbers,      RDI_DataSectionTag_SECONDARY|0x0004)\
X(LineMapRanges,       RDI_DataSectionTag_SECONDARY|0x0005)\
X(LineMapVoffs,        RDI_DataSectionTag_SECONDARY|0x0006)\
X(NameMapBuckets,      RDI_DataSectionTag_SECONDARY|0x0007)\
X(NameMapNodes,        RDI_DataSectionTag_SECONDARY|0x0008)

typedef RDI_U32 RDI_DataSectionTag;
typedef enum RDI_DataSectionTagEnum{
#define X(N,C) RDI_DataSectionTag_##N = C,
#define Y(N)   RDI_DataSectionTag_##N,
  RDI_DataSectionTagXList(X,Y)
#undef X
#undef Y
} RDI_DataSectionTagEnum;


#define RDI_DataSectionEncodingXList(X) \
X(Unpacked, 0)\
X(LZB, 1)

typedef RDI_U32 RDI_DataSectionEncoding;
typedef enum RDI_DataSectionEncodingEnum{
#define X(N,C) RDI_DataSectionEncoding_##N = C,
  RDI_DataSectionEncodingXList(X)
#undef X
} RDI_DataSectionEncodingEnum;

typedef struct RDI_DataSection{
  RDI_DataSectionTag tag;
  RDI_DataSectionEncoding encoding;
  RDI_U64 off;
  RDI_U64 encoded_size;
  RDI_U64 unpacked_size;
} RDI_DataSection;


//- common types
typedef struct RDI_VMapEntry{
  RDI_U64 voff;
  RDI_U64 idx;
} RDI_VMapEntry;

//- top level info
typedef struct RDI_TopLevelInfo{
  RDI_Arch architecture;
  RDI_U32 exe_name_string_idx;
  RDI_U64 exe_hash;
  RDI_U64 voff_max;
} RDI_TopLevelInfo;

//- binary sections
typedef RDI_U32 RDI_BinarySectionFlags;
typedef enum RDI_BinarySectionFlagsEnum{
  RDI_BinarySectionFlag_Read    = (1 << 0),
  RDI_BinarySectionFlag_Write   = (1 << 1),
  RDI_BinarySectionFlag_Execute = (1 << 2)
} RDI_BinarySectionFlagsEnum;

typedef struct RDI_BinarySection{
  RDI_U32 name_string_idx;
  RDI_BinarySectionFlags flags;
  RDI_U64 voff_first;
  RDI_U64 voff_opl;
  RDI_U64 foff_first;
  RDI_U64 foff_opl;
} RDI_BinarySection;

//- file & file system info
typedef struct RDI_FilePathNode{
  RDI_U32 name_string_idx;
  RDI_U32 parent_path_node;
  RDI_U32 first_child;
  RDI_U32 next_sibling;
  RDI_U32 source_file_idx;
} RDI_FilePathNode;

typedef struct RDI_SourceFile{
  RDI_U32 file_path_node_idx;
  
  RDI_U32 normal_full_path_string_idx;
  
  // usage of line map to go from a line number to an array of voffs
  //  (line_map_nums * line_number) -> (nil | index)
  //  (line_map_data * index) -> (range)
  //  (line_map_voff_data * range) -> (array(voff))
  
  RDI_U32 line_map_count;
  RDI_U32 line_map_nums_data_idx;  // U32[line_map_count] (sorted - not closed ranges)
  RDI_U32 line_map_range_data_idx; // U32[line_map_count + 1] (pairs form ranges)
  RDI_U32 line_map_voff_data_idx;  // U64[...] (idx by line_map_range_data)
} RDI_SourceFile;


//- units & line info
typedef struct RDI_Unit{
  RDI_U32 unit_name_string_idx;
  RDI_U32 compiler_name_string_idx;
  RDI_U32 source_file_path_node;
  RDI_U32 object_file_path_node;
  RDI_U32 archive_file_path_node;
  RDI_U32 build_path_node;
  RDI_Language language;
  
  // usage of line info to go from voff to file & line number:
  //  (line_info_voffs * voff) -> (nil + index)
  //  (line_info_data * index) -> (RDI_Line = (file_idx * line_number))
  
  RDI_U32 line_info_voffs_data_idx; // U64[line_info_count + 1] (sorted ranges)
  RDI_U32 line_info_data_idx;       // RDI_Line[line_info_count]
  RDI_U32 line_info_col_data_idx;   // RDI_Col[line_info_count]
  RDI_U32 line_info_count;
} RDI_Unit;

typedef struct RDI_Line{
  RDI_U32 file_idx;
  RDI_U32 line_num;
} RDI_Line;

typedef struct RDI_Column{
  RDI_U16 col_first;
  RDI_U16 col_opl;
} RDI_Column;


//- type info

// X(name,code) - defines a primary code
// XZ(name,code size) - defines a primary code & associates a size
// Y(alias_name,name) - defines an alias for bookends
#define RDI_TypeKindXList(X,XZ,Y)\
X(NULL,          0x0000) \
\
XZ(Void,         0x0001,  0) Y(FirstBuiltIn, Void) \
XZ(Handle,       0x0002,  0xFFFFFFFF) \
XZ(Char8,        0x0003,  1)\
XZ(Char16,       0x0004,  2) \
XZ(Char32,       0x0005,  4) \
XZ(UChar8,       0x0006,  1) \
XZ(UChar16,      0x0007,  2) \
XZ(UChar32,      0x0008,  4) \
XZ(U8,           0x0009,  1) \
XZ(U16,          0x000A,  2) \
XZ(U32,          0x000B,  4) \
XZ(U64,          0x000C,  8) \
XZ(U128,         0x000D, 16) \
XZ(U256,         0x000E, 32) \
XZ(U512,         0x000F, 64) \
XZ(S8,           0x0010,  1) \
XZ(S16,          0x0011,  2) \
XZ(S32,          0x0012,  4) \
XZ(S64,          0x0013,  8) \
XZ(S128,         0x0014, 16) \
XZ(S256,         0x0015, 32) \
XZ(S512,         0x0016, 64) \
XZ(Bool,         0x0017,  1) \
XZ(F16,          0x0018,  2) \
XZ(F32,          0x0019,  4) \
XZ(F32PP,        0x001A,  4) \
XZ(F48,          0x001B,  6) \
XZ(F64,          0x001C,  8) \
XZ(F80,          0x001D, 10) \
XZ(F128,         0x001E, 16) \
XZ(ComplexF32,   0x001F,  8) \
XZ(ComplexF64,   0x0020, 16) \
XZ(ComplexF80,   0x0021, 20) \
XZ(ComplexF128,  0x0022, 32) Y(LastBuiltIn, ComplexF128) \
\
X(Modifier,     0x1000) Y(FirstConstructed, Modifier) \
X(Ptr,          0x1001) \
X(LRef,         0x1002) \
X(RRef,         0x1003) \
X(Array,        0x1004) \
X(Function,     0x1005) \
X(Method,       0x1006) \
X(MemberPtr,    0x1007) Y(LastConstructed, MemberPtr) \
\
X(Struct,            0x2000) Y(FirstUserDefined, Struct) Y(FirstRecord, Struct) \
X(Class,             0x2001) \
X(Union,             0x2002) Y(LastRecord, Union) \
X(Enum,              0x2003) \
X(Alias,             0x2004) \
X(IncompleteStruct,  0x2005) Y(FirstIncomplete, IncompleteStruct) \
X(IncompleteUnion,   0x2006) \
X(IncompleteClass,   0x2007) \
X(IncompleteEnum,    0x2008) Y(LastIncomplete, IncompleteEnum) \
Y(LastUserDefined, IncompleteEnum) \
\
X(Bitfield,     0xF000) \
X(Variadic,     0xF001)

typedef RDI_U16 RDI_TypeKind;
typedef enum RDI_TypeKindEnum{
  
#define X(name,code) RDI_TypeKind_##name = code,
#define XZ(name,code,size) X(name,code)
#define Y(alias_name,name) RDI_TypeKind_##alias_name = RDI_TypeKind_##name,
  RDI_TypeKindXList(X,XZ,Y)
#undef X
#undef XZ
#undef Y
  
} RDI_TypeKindEnum;

typedef RDI_U16 RDI_TypeModifierFlags;
enum{
  RDI_TypeModifierFlag_Const    = (1 << 0),
  RDI_TypeModifierFlag_Volatile = (1 << 1),
};

// IMPORTANT NOTE: All type nodes in a valid raddbg are *topologically sorted*.
// That means any time a type node refers to another type node, the type node
// it refers to has an index less than or equal to the index of the node that
// is doing the referring. It is never the case that a type node depends on a
// node that comes later in the type node array.
//  This restriction does not apply to the members of a type that are
// attached through a "UDT" though.

typedef struct RDI_TypeNode{
  RDI_TypeKind kind;
  // when kind is 'Modifier' -> RDI_TypeModifierFlags
  RDI_U16 flags;
  
  RDI_U32 byte_size;
  
  union{
    // kind is 'built-in'
    struct{
      RDI_U32 name_string_idx;
    } built_in;
    
    // kind is 'constructed'
    struct{
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
    struct{
      RDI_U32 name_string_idx;
      RDI_U32 direct_type_idx;
      RDI_U32 udt_idx;
    } user_defined;
    
    // (kind = Bitfield)
    struct{
      RDI_U32 direct_type_idx;
      RDI_U32 off;
      RDI_U32 size;
    } bitfield;
  };
} RDI_TypeNode;

typedef RDI_U32 RDI_UserDefinedTypeFlags;
enum{
  RDI_UserDefinedTypeFlag_EnumMembers = (1 << 0),
};

typedef struct RDI_UDT{
  RDI_U32 self_type_idx;
  RDI_UserDefinedTypeFlags flags;
  
  // when EnumMembers flag is set, indexes into enum "enum_members" instead of "members"
  RDI_U32 member_first;
  RDI_U32 member_count;
  
  RDI_U32 file_idx;
  RDI_U32 line;
  RDI_U32 col;
} RDI_UDT;

#define RDI_MemberKindXList(X) \
X(NULL,          0x0000) \
X(DataField,     0x0001) \
X(StaticData,    0x0002) \
X(Method,        0x0100) \
X(StaticMethod,  0x0101) \
X(VirtualMethod, 0x0102) \
X(VTablePtr,     0x0200) \
X(Base,          0x0201) \
X(VirtualBase,   0x0202) \
X(NestedType,    0x0300)

typedef RDI_U16 RDI_MemberKind;
typedef enum RDI_MemberKindEnum{
#define X(N,C) RDI_MemberKind_##N = C,
  RDI_MemberKindXList(X)
#undef X
} RDI_MemberKindEnum;

// TODO(allen): need a way to equip methods and some virtual methods
// with procedure symbol information. I'm thinking a seperate data
// array of (MemberIdx,ProcSymbolIdx) sorted by MemberIdx. Or just a
// parallel array. Putting them right into this struct looks like it
// would complicate the converters because we tend to want an API
// like 'associate_method_to_proc' that can be used *after* both the
// method and proc are known, rather than one that forces us to know
// the association when constructing the member data.
typedef struct RDI_Member{
  RDI_MemberKind kind;
  RDI_U16 __unused__;
  
  RDI_U32 name_string_idx;
  RDI_U32 type_idx;
  RDI_U32 off;
} RDI_Member;

typedef struct RDI_EnumMember{
  RDI_U32 name_string_idx;
  RDI_U32 __unused__;
  RDI_U64 val;
} RDI_EnumMember;


//- symbol info
typedef RDI_U32 RDI_LinkFlags;
enum{
  RDI_LinkFlag_External   = (1 << 0),
  // NOTE: Scope flags are mutually exclusive.
  //       A symbol is either global scoped, type scoped, or procedure scoped.
  RDI_LinkFlag_TypeScoped = (1 << 1),
  RDI_LinkFlag_ProcScoped = (1 << 2),
};

typedef struct RDI_GlobalVariable{
  RDI_U32 name_string_idx;
  // NOTE: "global variables" can be scoped in *any* way. The scope flags here refer to 
  //       *namespace* scoping. "global variables" are all in the data section of the
  //       final exe/dll type file, so they are "global" in the life-time sense of the
  //       word. In the namespace sense of the word, they can be scoped globally, by type,
  //       or by procedure.
  RDI_LinkFlags link_flags;
  RDI_U64 voff;
  RDI_U32 type_idx;
  
  // container_idx: UDT for "TypeScoped", Procedure for "ProcScoped"
  RDI_U32 container_idx;
} RDI_GlobalVariable;

typedef struct RDI_ThreadVariable{
  RDI_U32 name_string_idx;
  // NOTE: See the note in GlobalVariable regarding scoping. The same concept applies here.
  RDI_LinkFlags link_flags;
  RDI_U32 tls_off;
  RDI_U32 type_idx;
  
  // container_idx: UDT for "TypeScoped", Procedure for "ProcScoped"
  RDI_U32 container_idx;
} RDI_ThreadVariable;

typedef struct RDI_Procedure{
  RDI_U32 name_string_idx;
  RDI_U32 link_name_string_idx;
  // NOTE: See the note in GlobalVariable regarding scoping. The same concept applies here.
  RDI_LinkFlags link_flags;
  RDI_U32 type_idx;
  RDI_U32 root_scope_idx;
  
  // container_idx: UDT for "TypeScoped", Procedure for "ProcScoped"
  RDI_U32 container_idx;
} RDI_Procedure;

typedef struct RDI_Scope{
  RDI_U32 proc_idx;
  RDI_U32 parent_scope_idx;
  RDI_U32 first_child_scope_idx;
  RDI_U32 next_sibling_scope_idx;
  
  RDI_U32 voff_range_first;
  RDI_U32 voff_range_opl;
  
  // indexes into "locals"
  RDI_U32 local_first;
  RDI_U32 local_count;
  
  RDI_U32 static_local_idx_run_first;
  RDI_U32 static_local_count;
  
  // TODO(allen): attach less common scope-relevant info
} RDI_Scope;

typedef RDI_U32 RDI_LocalKind;
typedef enum{
  RDI_LocalKind_NULL,
  RDI_LocalKind_Parameter,
  RDI_LocalKind_Variable,
  RDI_LocalKind_COUNT
} RDI_LocalKindEnum;

typedef struct RDI_Local{
  RDI_LocalKind kind;
  RDI_U32 name_string_idx;
  RDI_U64 type_idx;
  // indexes into "location_blocks"
  RDI_U32 location_first;
  RDI_U32 location_opl;
} RDI_Local;

typedef struct RDI_LocationBlock{
  RDI_U32 scope_off_first;
  RDI_U32 scope_off_opl;
  RDI_U32 location_data_off;
} RDI_LocationBlock;

typedef RDI_U8 RDI_LocationKind;
typedef enum{
  RDI_LocationKind_NULL,
  RDI_LocationKind_AddrBytecodeStream,
  RDI_LocationKind_ValBytecodeStream,
  RDI_LocationKind_AddrRegisterPlusU16,
  RDI_LocationKind_AddrAddrRegisterPlusU16,
  RDI_LocationKind_ValRegister,
  RDI_LocationKind_COUNT
} RDI_LocationKindEnum;

typedef struct RDI_LocationBytecodeStream{
  RDI_LocationKind kind;
  // [... 0] null terminated byte sequence RDI_EvalBytecodeStream
} RDI_LocationBytecodeStream;

typedef struct RDI_LocationRegisterPlusU16{
  RDI_LocationKind kind;
  RDI_RegisterCode register_code;
  RDI_U16 offset;
} RDI_LocationRegisterPlusU16;

typedef struct RDI_LocationRegister{
  RDI_LocationKind kind;
  RDI_RegisterCode register_code;
} RDI_LocationRegister;

//- name map types
#define RDI_NameMapXList(X)\
X(NULL,            0)\
X(GlobalVariables, 1)\
X(ThreadVariables, 2)\
X(Procedures,      3)\
X(Types,           4)\
X(LinkNameProcedures, 5)\
X(NormalSourcePaths,  6)

typedef RDI_U32 RDI_NameMapKind;
typedef enum RDI_NameMapKindEnum{
#define X(N,C) RDI_NameMapKind_##N = C,
  RDI_NameMapXList(X)
#undef X
  
  RDI_NameMapKind_COUNT
} RDI_NameMapKindEnum;

// TODO(allen): documentation here for the hashing and probing strategy for this table

typedef struct RDI_NameMap{
  RDI_NameMapKind kind;
  RDI_U32 bucket_data_idx;
  RDI_U32 node_data_idx;
} RDI_NameMap;

typedef struct RDI_NameMapBucket{
  RDI_U32 first_node;
  RDI_U32 node_count;
} RDI_NameMapBucket;

typedef struct RDI_NameMapNode{
  RDI_U32 string_idx;
  RDI_U32 match_count;
  // NOTE: if (match_count == 1) then this is the index of the matching item
  //       if (match_count > 1)  then this is the first for an index run of all the matches
  RDI_U32 match_idx_or_idx_run_first;
} RDI_NameMapNode;


////////////////////////////////
// Eval Bytecode

// (Name, decodeN, popN, pushN)
#define RDI_EvalOpXList(X)\
X(Stop,        0, 0, 0)\
X(Noop,        0, 0, 0)\
X(Cond,        1, 1, 0)\
X(Skip,        1, 0, 0)\
X(MemRead,     1, 1, 1)\
X(RegRead,     4, 0, 1)\
X(RegReadDyn,  0, 1, 1)\
X(FrameOff,    1, 0, 1)\
X(ModuleOff,   4, 0, 1)\
X(TLSOff,      4, 0, 1)\
X(ObjectOff,   0, 0, 0)\
X(CFA,         0, 0, 0)\
X(ConstU8,     1, 0, 1)\
X(ConstU16,    2, 0, 1)\
X(ConstU32,    4, 0, 1)\
X(ConstU64,    8, 0, 1)\
X(Abs,         1, 1, 1)\
X(Neg,         1, 1, 1)\
X(Add,         1, 2, 1)\
X(Sub,         1, 2, 1)\
X(Mul,         1, 2, 1)\
X(Div,         1, 2, 1)\
X(Mod,         1, 2, 1)\
X(LShift,      1, 2, 1)\
X(RShift,      1, 2, 1)\
X(BitAnd,      1, 2, 1)\
X(BitOr,       1, 2, 1)\
X(BitXor,      1, 2, 1)\
X(BitNot,      1, 1, 1)\
X(LogAnd,      1, 2, 1)\
X(LogOr,       1, 2, 1)\
X(LogNot,      1, 1, 1)\
X(EqEq,        1, 2, 1)\
X(NtEq,        1, 2, 1)\
X(LsEq,        1, 2, 1)\
X(GrEq,        1, 2, 1)\
X(Less,        1, 2, 1)\
X(Grtr,        1, 2, 1)\
X(Trunc,       1, 1, 1)\
X(TruncSigned, 1, 1, 1)\
X(Convert,     2, 1, 1)\
X(Pick,        1, 0, 1)\
X(Pop,         0, 1, 0)\
X(Insert,      1, 0, 0)

// (Name)
#define RDI_EvalTypeGroupXList(X)\
X(Other)\
X(U)\
X(S)\
X(F32)\
X(F64)

// (Name, error-message)
#define RDI_EvalConversionKindXList(X)\
X(Noop,         "")\
X(Legal,        "")\
X(OtherToOther, "cannot convert between these types")\
X(ToOther,      "cannot convert to this type")\
X(FromOther,    "cannot convert this type")

// Xb(EvalTypeGroup) Y(TypeKind) Xe(EvalTypeGroup)
#define RDI_EvalTypeGroupFromKindMap(Y,Xb,Xe)\
\
Xb(U) Y(U8) Y(U16) Y(U32) Y(U64) Y(Bool) Y(Ptr) Y(Enum)\
Xe(U)\
\
Xb(S) Y(S8) Y(S16) Y(S32) Y(S64)\
Xe(S)\
\
Xb(F32) Y(F32)\
Xe(F32)\
\
Xb(F64) Y(F64)\
Xe(F64)

// Xb(EvalConversionKind) Y(EvalTypeGroup, EvalTypeGroup) Xe(EvalConversionKind)
#define RDI_EvalConversionKindFromTypeGroupPairMap(Y,Xb,Xe)\
\
Xb(Noop) Y(U, U) Y(S, S) Y(F32, F32) Y(F64, F64) Y(U, S) Y(S, U)\
Xe(Noop)\
\
Xb(Legal)\
Y(U, F32) Y(S, F32) Y(F32, U) Y(F32, S)\
Y(U, F64) Y(S, F64) Y(F64, U) Y(F64, S)\
Y(F32, F64) Y(F64, F32)\
Xe(Legal)\
\
Xb(OtherToOther) Y(Other, Other)\
Xe(OtherToOther)\
\
Xb(ToOther) Y(U, Other) Y(S, Other) Y(F32, Other) Y(F64, Other)\
Xe(ToOther)\
\
Xb(FromOther) Y(Other, U) Y(Other, S) Y(Other, F32) Y(Other, F64)\
Xe(FromOther)

// eval interpretation macros
#define RDI_EncodeRegReadParam(reg,bytesize,bytepos) ((reg)|((bytesize)<<8)|((bytepos)<<16))


// eval enums
typedef RDI_U8 RDI_EvalOp;

typedef enum RDI_EvalOpEnum{
#define X(N,dec,pop,push) RDI_EvalOp_##N,
  RDI_EvalOpXList(X)
#undef X
  
  RDI_EvalOp_COUNT
} RDI_EvalOpEnum;


typedef RDI_U8 RDI_EvalTypeGroup;

typedef enum RDI_EvalTypeGroupEnum{
#define X(N) RDI_EvalTypeGroup_##N,
  RDI_EvalTypeGroupXList(X)
#undef X
  RDI_EvalTypeGroup_COUNT,
} RDI_EvalTypeGroupEnum;


typedef RDI_U8 RDI_EvalConversionKind;

typedef enum RDI_EvalConversionKindEnum{
#define X(N,msg) RDI_EvalConversionKind_##N,
  RDI_EvalConversionKindXList(X)
#undef X
  RDI_EvalConversionKind_COUNT,
} RDI_EvalConversionKindEnum;


//- eval data tables

#define RDI_EVAL_CTRLBITS(decodeN,popN,pushN) ((decodeN) | ((popN) << 4) | ((pushN) << 6))
#define RDI_DECODEN_FROM_CTRLBITS(ctrlbits) ((ctrlbits) & 0xf)
#define RDI_POPN_FROM_CTRLBITS(ctrlbits)    (((ctrlbits) >> 4) & 0x3)
#define RDI_PUSHN_FROM_CTRLBITS(ctrlbits)   (((ctrlbits) >> 6) & 0x3)

static RDI_U8 rdi_eval_opcode_ctrlbits[] = {
#define X(Name, decodeN, popN, pushN) RDI_EVAL_CTRLBITS(decodeN,popN,pushN),
  RDI_EvalOpXList(X)
#undef X
};

////////////////////////////////
// Functions

RDI_PROC RDI_U64 rdi_hash(RDI_U8 *ptr, RDI_U64 size);
RDI_PROC RDI_U32 rdi_size_from_basic_type_kind(RDI_TypeKind kind);
RDI_PROC RDI_U32 rdi_addr_size_from_arch(RDI_Arch arch);

//- eval helpers

RDI_PROC RDI_EvalConversionKind rdi_eval_conversion_rule(RDI_EvalTypeGroup in, RDI_EvalTypeGroup out);
RDI_PROC RDI_U8* rdi_eval_conversion_message(RDI_EvalConversionKind conversion_kind, RDI_U64 *lennout);
RDI_PROC RDI_S32 rdi_eval_opcode_type_compatible(RDI_EvalOp op, RDI_EvalTypeGroup group);

#endif // RADDBGI_FORMAT_H
