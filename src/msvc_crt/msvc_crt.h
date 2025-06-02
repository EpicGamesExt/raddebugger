// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef MSVC_CRT
#define MSVC_CRT

////////////////////////////////

// feature flags in absolute symbol @feat.00
enum
{
  MSCRT_FeatFlag_HAS_SAFE_SEH  = (1 << 0),  // /safeseh
  MSCRT_FeatFlag_UNKNOWN_4     = (1 << 4),
  MSCRT_FeatFlag_GUARD_STACK   = (1 << 8),  // /GS
  MSCRT_FeatFlag_SDL           = (1 << 9),  // /sdl
  MSCRT_FeatFlag_GUARD_CF      = (1 << 11), // /guard:cf
  MSCRT_FeatFlag_GUARD_EH_CONT = (1 << 14), // /guard:ehcont
  MSCRT_FeatFlag_NO_RTTI       = (1 << 17), // /GR-
  MSCRT_FeatFlag_KERNEL        = (1 << 30), // /kernel
};
typedef U32 MSCRT_FeatFlags;

typedef struct MSCRT_VCFeatures
{
  U32 pre_vcpp;
  U32 c_cpp;
  U32 gs;
  U32 sdl;
  U32 guard_n;
} MSCRT_VCFeatures;

////////////////////////////////
// GS Handler

#define MSCRT_GSHandler_GetFlags(x)        (((x) & 0x00000007) >> 0)
#define MSCRT_GSHandler_GetCookieOffset(x) (((x) & 0xFFFFFFF8) >> 3)

typedef U8 MSCRT_GSHandlerFlags;
enum
{
  MSCRT_GSHandlerFlag_EHandler     = (1 << 0),
  MSCRT_GSHandlerFlag_UHandler     = (1 << 1),
  MSCRT_GSHandlerFlag_HasAlignment = (1 << 2)
};

////////////////////////////////
// Exceptions < v4

#define MSCRT_MAGIC_GET_CHECK(x) ((x)  & 0x1FFFFFFF)
#define MSCRT_MAGIC_GET_FLAGS(x) (((x) & 0xE0000000) >> 29)

// Magic numbers are incremented by one everytime there is a new version.
// Top 3 bits are reserved for flags.
enum
{
  MSCRT_Magic1     = 0x19930520,
  MSCRT_Magic2     = 0x19930521,
  MSCRT_Magic3     = 0x19930522,
  
  // pure magic indicates that exception cannot be caught in native or managed code.
  MSCRT_PureMagic1 = 0x1994000,
};
enum
{
  MSCRT_MagicFlag_EHS         = (1 << 0),
  MSCRT_MagicFlag_DYNSTALKING = (1 << 1),
  MSCRT_MagicFlag_EHNOEXCEPT  = (1 << 2)
};

typedef U32 MSCRT_Flags;
enum
{
  MSCRT_Flag_SynchronousExceptionOnly = (1 << 0),
  MSCRT_Flag_UNKNOWN                  = (1 << 1),
  MSCRT_Flag_StopUnwind               = (1 << 2), // When set unwinding can't continue.
};

enum
{
  MSCRT_CatchableType_IsSimpleType   = (1 << 0),
  MSCRT_CatchableType_ByRefOnly      = (1 << 1),
  MSCRT_CatchableType_HasVirtualBase = (1 << 2), // type is a class with virtual base
  MSCRT_CatchableType_IsWinRTHandle  = (1 << 3), // type is a WinRT handle
  MSCRT_CatchableType_IsStdBadAlloc  = (1 << 4)  // type is a std::bad_alloc
};

enum
{
  MSCRT_ThrowInfo_IsConst     = (1 << 0),
  MSCRT_ThrowInfo_IsVolatile  = (1 << 1),
  MSCRT_ThrowInfo_IsUnaligned = (1 << 2),
  MSCRT_ThrowInfo_IsPure      = (1 << 3), // thrown object is from pure module
  MSCRT_ThrowInfo_IsWinRT     = (1 << 4)  // thrown object is a WinRT exception
};

typedef U32 MSCRT_EhHandlerTypeFlags;
enum
{
  MSCRT_EhHandlerTypeFlag_IsConst     = (1 << 0), // referenced type is 'const'
  MSCRT_EhHandlerTypeFlag_IsVolatile  = (1 << 1), // referenced type is 'volatile'
  MSCRT_EhHandlerTypeFlag_IsUnaligned = (1 << 2), // referenced type is 'unaligned'
  MSCRT_EhHandlerTypeFlag_IsReference = (1 << 3), // catch type is by reference
  MSCRT_EhHandlerTypeFlag_IsResumable = (1 << 4), // catch may choose to resume
  MSCRT_EhHandlerTypeFlag_IsStdDotDot = (1 << 6), // catch(...)
  MSCRT_EhHandlerTypeFlag_IsComplusEH = (1 << 31) // is handling EH in complus
};

typedef struct MSCRT_FuncInfo32
{
  U32         magic;
  U32         max_state;
  U32         unwind_map_voff;
  U32         try_block_map_count;
  U32         try_block_map_voff;
  U32         ip_map_count;
  U32         ip_map_voff;
  U32         frame_offset_unwind_helper;
  U32         es_type_list_voff;           // llvm emits zero, not sure what this supposed to be
  MSCRT_Flags eh_flags;
} MSCRT_FuncInfo32;

typedef struct MSCRT_IPState32
{
  U32 ip;
  S32 state;
} MSCRT_IPState32;

typedef struct MSCRT_UnwindMap32
{
  S32 next_state;
  U32 action_virt_off;
} MSCRT_UnwindMap32;

typedef struct MSCRT_EhHandlerType32
{
  MSCRT_EhHandlerTypeFlags adjectives;
  U32                      descriptor_voff;
  U32                      catch_obj_frame_offset;
  U32                      catch_handler_voff;
  U32                      fp_distance;
} MSCRT_EhHandlerType32;

typedef struct MSCRT_TryMapBlock32
{
  S32 try_low;
  S32 try_high;
  S32 catch_high;
  S32 catch_handlers_count;
  U32 catch_handlers_voff;
} MSCRT_TryMapBlock32;

typedef struct MSCRT_ExceptionSpecTypeList32
{
  S32 count;
  U32 handlers_voff;
} MSCRT_ExceptionSpecTypeList32;

typedef struct MSCRT_TryMapBlock
{
  S32                    try_low;
  S32                    try_high;
  S32                    catch_high;
  S32                    catch_handlers_count;
  MSCRT_EhHandlerType32 *catch_handlers;
} MSCRT_TryMapBlock;

typedef struct MSCRT_ExceptionSpecTypeList
{
  S32                    count;
  MSCRT_EhHandlerType32 *handlers;
} MSCRT_ExceptionSpecTypeList;

typedef struct MSCRT_FuncInfo
{
  U32                          magic;
  U32                          max_state;
  MSCRT_UnwindMap32           *unwind_map;
  U32                          try_block_map_count;
  MSCRT_TryMapBlock           *try_block_map;
  U32                          ip_map_count;
  MSCRT_IPState32             *ip_map;
  U32                          frame_offset_unwind_helper;
  MSCRT_ExceptionSpecTypeList  es_type_list;
  MSCRT_Flags                  eh_flags;
} MSCRT_FuncInfo;

////////////////////////////////
// C++ Exceptions V4

typedef U8 MSCRT_FuncInfoV4Flags;
enum
{
  MSCRT_FuncInfoV4Flag_IsCatch     = (1 << 0), // catch funclet
  MSCRT_FuncInfoV4Flag_IsSeparated = (1 << 1), // func has separate code segment
  MSCRT_FuncInfoV4Flag_IsBBT       = (1 << 2), // flags set by basic block trasformations
  MSCRT_FuncInfoV4Flag_UnwindMap   = (1 << 3), // unwind map is present
  MSCRT_FuncInfoV4Flag_TryBlockMap = (1 << 4), // try block map is present
  MSCRT_FuncInfoV4Flag_EHs         = (1 << 5),
  MSCRT_FuncInfoV4Flag_NoExcept    = (1 << 6),
  MSCRT_FuncInfoV4Flag_Reserved    = (1 << 7)
};

typedef U32 MSCRT_UnwindMapV4Type;
enum
{
  MSCRT_UnwindMapV4Type_NoUW             = 0, // no unwind action associated with this state
  MSCRT_UnwindMapV4Type_DtorWithObj      = 1, // dtor with an object offset
  MSCRT_UnwindMapV4Type_DtorWithPtrToObj = 2, // dtor with an offset that contains a pointer to the object to be destroyed
  MSCRT_UnwindMapV4Type_VOFF             = 3, // dtor  that has a direct function that is called that knows where the object is and can perform more exotic destruction
};

enum
{
  MSCRT_ContV4Type_NoMetadata     = 1, // no metadata use whatever funclet returns
  MSCRT_ContV4Type_OneFuncRelAddr = 2,
  MSCRT_ContV4Type_TwoFuncRelAddr = 3
};

#define MSCRT__EH_HANDLER_V4_FLAGS_EXTRACT_CONT_TYPE(x) (((x) & MSCRT_EhHandlerV4Flag_ContVOffMask) >> MSVC_CRTHandlerV4Flag_ContVOffShift)
typedef U8 MSCRT_EhHandlerV4Flags;
enum
{
  MSCRT_EhHandlerV4Flag_Adjectives   = (1 << 0), // set if adjectives are present
  MSCRT_EhHandlerV4Flag_DispType     = (1 << 1), // set if type descriptors are present
  MSCRT_EhHandlerV4Flag_DispCatchObj = (1 << 2), // set if catch object object is present
  MSCRT_EhHandlerV4Flag_ContIsVOff   = (1 << 3), // continuantion addresses are VOFF rather than function relative
  
  MSCRT_EhHandlerV4Flag_ContVOffMask  = 0x30,
  MSCRT_EhHandlerV4Flag_ContVOffShift = 4,
};

typedef struct MSCRT_EhHandlerTypeV4
{
  MSCRT_EhHandlerV4Flags   flags;
  MSCRT_EhHandlerTypeFlags adjectives;
  S32                      type_voff;
  U32                      catch_obj_voff;
  S32                      catch_code_voff;
  U64                      catch_funclet_cont_addr[2];
  U32                      catch_funclet_cont_addr_count;
} MSCRT_EhHandlerTypeV4;

typedef struct MSCRT_EhHandlerTypeV4Array
{
  U64                    count;
  MSCRT_EhHandlerTypeV4 *v;
} MSCRT_EhHandlerTypeV4Array;

typedef struct MSCRT_TryBlockMap32V4
{
  U32 try_low;
  U32 try_high;
  U32 catch_high;
  S32 handler_array_voff;
} MSCRT_TryBlockMap32V4;

typedef struct MSCRT_IP2State32V4
{
  U32  count;
  U32 *voffs;
  S32 *states;
} MSCRT_IP2State32V4;

typedef struct MSCRT_SepIPState32V4
{
  S32 func_start_voff;
  S32 ip_map_voff;
} MSCRT_SepIPState32V4;

typedef struct MSCRT_FuncInfo32V4
{
  MSCRT_FuncInfoV4Flags header;
  U32                   bbt_flags;
  S32                   unwind_map_voff;
  S32                   try_block_map_voff;
  S32                   ip_to_state_map_voff;
  S32                   wrt_frame_establisher_voff; // used only in catch funclets
} MSCRT_FuncInfo32V4;

typedef struct MSCRT_UnwindEntryV4
{
  MSCRT_UnwindMapV4Type type;
  S32                   action;
  U32                   object;
  U32                   next_off;
} MSCRT_UnwindEntryV4;

typedef struct MSCRT_UnwindMapV4
{
  U32                  count;
  MSCRT_UnwindEntryV4 *v;
} MSCRT_UnwindMapV4;

typedef struct MSCRT_TryBlockMapV4
{
  U32                        try_low;
  U32                        try_high;
  U32                        catch_high;
  MSCRT_EhHandlerTypeV4Array handlers;
} MSCRT_TryBlockMapV4;

typedef struct MSCRT_TryBlockMapV4Array
{
  U64                  count;
  MSCRT_TryBlockMapV4 *v;
} MSCRT_TryBlockMapV4Array;

typedef struct MSCRT_ParsedFuncInfoV4
{
  MSCRT_FuncInfoV4Flags    header;
  U32                      bbt_flags;
  MSCRT_UnwindMapV4        unwind_map;
  MSCRT_TryBlockMapV4Array try_block_map;
  MSCRT_IP2State32V4       ip2state_map;
} MSCRT_ParsedFuncInfoV4;

//- Exception info < v4

internal U64 mscrt_parse_func_info(Arena *arena, String8 raw_data, U64 section_count, COFF_SectionHeader *sections, U64 off, MSCRT_FuncInfo *func_info);

//- Exception info v4

internal U64 mscrt_parse_handler_type_v4       (String8 raw_data, U64 offset, U64 func_voff, MSCRT_EhHandlerTypeV4 *handler);
internal U64 mscrt_parse_unwind_v4_entry       (String8 raw_data, U64 offset, MSCRT_UnwindEntryV4 *entry_out);
internal U64 mscrt_parse_handler_type_v4_array (Arena *arena, String8 raw_data, U64 offset, U64 func_voff, MSCRT_EhHandlerTypeV4Array *array_out);
internal U64 mscrt_parse_unwind_map_v4         (Arena *arena, String8 raw_data, U64 off, MSCRT_UnwindMapV4 *map_out);
internal U64 mscrt_parse_try_block_map_array_v4(Arena *arena, String8 raw_data, U64 off, U64 section_count, COFF_SectionHeader *sections, U64 func_voff, MSCRT_TryBlockMapV4Array *map_out);
internal U64 mscrt_parse_ip2state_map_v4       (Arena *arena, String8 raw_data, U64 off, U64 func_voff, MSCRT_IP2State32V4 *ip2state_map_out);
internal U64 mscrt_parse_func_info_v4          (Arena *arena, String8 raw_data, U64 section_count, COFF_SectionHeader *sections, U64 off, U64 func_voff, MSCRT_ParsedFuncInfoV4 *func_info_out);

#endif // MSVC_CRT

