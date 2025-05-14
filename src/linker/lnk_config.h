// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef enum
{
  LNK_CmdSwitch_Null,
  LNK_CmdSwitch_NotImplemented,

  LNK_CmdSwitch_Align,
  LNK_CmdSwitch_AllowBind,
  LNK_CmdSwitch_AllowIsolation,
  LNK_CmdSwitch_AlternateName,
  LNK_CmdSwitch_AppContainer,
  LNK_CmdSwitch_Base,
  LNK_CmdSwitch_Debug,
  LNK_CmdSwitch_DefaultLib,
  LNK_CmdSwitch_Delay,
  LNK_CmdSwitch_DelayLoad,
  LNK_CmdSwitch_Dll,
  LNK_CmdSwitch_DynamicBase,
  LNK_CmdSwitch_Dump,
  LNK_CmdSwitch_Entry,
  LNK_CmdSwitch_Export,
  LNK_CmdSwitch_FastFail,
  LNK_CmdSwitch_FileAlign,
  LNK_CmdSwitch_Fixed,
  LNK_CmdSwitch_FunctionPadMin,
  LNK_CmdSwitch_Heap,
  LNK_CmdSwitch_HighEntropyVa,
  LNK_CmdSwitch_Ignore,
  LNK_CmdSwitch_ImpLib,
  LNK_CmdSwitch_Include,
  LNK_CmdSwitch_Incremental,
  LNK_CmdSwitch_LargeAddressAware,
  LNK_CmdSwitch_Lib,
  LNK_CmdSwitch_LibPath,
  LNK_CmdSwitch_Machine,
  LNK_CmdSwitch_Manifest,
  LNK_CmdSwitch_ManifestDependency,
  LNK_CmdSwitch_ManifestFile,
  LNK_CmdSwitch_ManifestInput,
  LNK_CmdSwitch_ManifestUac,
  LNK_CmdSwitch_Merge,
  LNK_CmdSwitch_Natvis,
  LNK_CmdSwitch_NoDefaultLib,
  LNK_CmdSwitch_NoExp,
  LNK_CmdSwitch_NoImpLib,
  LNK_CmdSwitch_NoLogo,
  LNK_CmdSwitch_NxCompat,
  LNK_CmdSwitch_Opt,
  LNK_CmdSwitch_Out,
  LNK_CmdSwitch_Pdb,
  LNK_CmdSwitch_PdbAltPath,
  LNK_CmdSwitch_PdbPageSize,
  LNK_CmdSwitch_Stack,
  LNK_CmdSwitch_SubSystem,
  LNK_CmdSwitch_Time,
  LNK_CmdSwitch_TsAware,

  // -- NOT Implemented:

  LNK_CmdSwitch_AssemblyDebug,
  LNK_CmdSwitch_AssemblyLinkResource,
  LNK_CmdSwitch_AssemblyModule,
  LNK_CmdSwitch_AssemblyResource,
  LNK_CmdSwitch_ClrImageType,
  LNK_CmdSwitch_ClrLoaderOptimization,
  LNK_CmdSwitch_ClrSupportLastError,
  LNK_CmdSwitch_ClrThreadAttribute,
  LNK_CmdSwitch_ClrRunManagedCodeCheck,
  LNK_CmdSwitch_ClrUnmanagedCheck,
  LNK_CmdSwitch_Def,
  LNK_CmdSwitch_DelaySign,
  LNK_CmdSwitch_DependentLoadFlag,
  LNK_CmdSwitch_Driver,
  LNK_CmdSwitch_DisallowLib,
  LNK_CmdSwitch_EditAndContinue,
  LNK_CmdSwitch_EmitVolatileMetadata,
  LNK_CmdSwitch_ErrorReport,
  LNK_CmdSwitch_ExportAdmin,
  LNK_CmdSwitch_FastGenProfile,
  LNK_CmdSwitch_FailIfMismatch,
  LNK_CmdSwitch_Force,
  LNK_CmdSwitch_Guard,
  LNK_CmdSwitch_GuardSym,
  LNK_CmdSwitch_GenProfile,
  LNK_CmdSwitch_IdlOut,
  LNK_CmdSwitch_IgnoreIdl,
  LNK_CmdSwitch_Ilk,
  LNK_CmdSwitch_IntegrityCheck,
  LNK_CmdSwitch_InferAsanLibs,
  LNK_CmdSwitch_InferAsanLibsNo,
  LNK_CmdSwitch_Kernel,
  LNK_CmdSwitch_KeyContainer,
  LNK_CmdSwitch_KeyFile,
  LNK_CmdSwitch_LinkerRepro,
  LNK_CmdSwitch_LinkerReproTarget,
  LNK_CmdSwitch_Ltcg,
  LNK_CmdSwitch_LtcgOut,
  LNK_CmdSwitch_Map,
  LNK_CmdSwitch_MapInfo,
  LNK_CmdSwitch_Midl,
  LNK_CmdSwitch_NoAssembly,
  LNK_CmdSwitch_NoEntry,
  LNK_CmdSwitch_Order,
  LNK_CmdSwitch_PdbStripped,
  LNK_CmdSwitch_Profile,
  LNK_CmdSwitch_Release,
  LNK_CmdSwitch_SafeSeh,
  LNK_CmdSwitch_Section,
  LNK_CmdSwitch_SourceLink,
  LNK_CmdSwitch_Stub,
  LNK_CmdSwitch_SwapRun,
  LNK_CmdSwitch_TlbId,
  LNK_CmdSwitch_ThrowingNew,
  LNK_CmdSwitch_UserProfile,
  LNK_CmdSwitch_Verbose,
  LNK_CmdSwitch_Version,
  LNK_CmdSwitch_Winmd,
  LNK_CmdSwitch_WinmdDelaySign,
  LNK_CmdSwitch_WinmdKeyContainer,
  LNK_CmdSwitch_WinmdKeyFile,
  LNK_CmdSwitch_WholeArchive,
  LNK_CmdSwitch_Wx,

  LNK_CmdSwitch_Rad_Age,
  LNK_CmdSwitch_Rad_BuildInfo,
  LNK_CmdSwitch_Rad_CheckUnusedDelayLoadDll,
  LNK_CmdSwitch_Rad_ChunkMap,
  LNK_CmdSwitch_Rad_Debug,
  LNK_CmdSwitch_Rad_DebugName,
  LNK_CmdSwitch_Rad_DebugAltPath,
  LNK_CmdSwitch_Rad_DelayBind,
  LNK_CmdSwitch_Rad_DoMerge,
  LNK_CmdSwitch_Rad_EnvLib,
  LNK_CmdSwitch_Rad_Exe,
  LNK_CmdSwitch_Rad_Guid,
  LNK_CmdSwitch_Rad_LargePages,
  LNK_CmdSwitch_Rad_LinkVer, 
  LNK_CmdSwitch_Rad_Log,
  LNK_CmdSwitch_Rad_Logo,
  LNK_CmdSwitch_Rad_MtPath,
  LNK_CmdSwitch_Rad_OsVer,
  LNK_CmdSwitch_Rad_PageSize,
  LNK_CmdSwitch_Rad_PathStyle,
  LNK_CmdSwitch_Rad_PdbHashTypeNames,
  LNK_CmdSwitch_Rad_PdbHashTypeNameMap,
  LNK_CmdSwitch_Rad_PdbHashTypeNameLength,
  LNK_CmdSwitch_Rad_SectVirtOff,
  LNK_CmdSwitch_Rad_SharedThreadPool,
  LNK_CmdSwitch_Rad_SharedThreadPoolMaxWorkers,
  LNK_CmdSwitch_Rad_SuppressError,
  LNK_CmdSwitch_Rad_SymbolTableCapDefined,
  LNK_CmdSwitch_Rad_SymbolTableCapInternal,
  LNK_CmdSwitch_Rad_SymbolTableCapWeak,
  LNK_CmdSwitch_Rad_SymbolTableCapLib,
  LNK_CmdSwitch_Rad_WriteTempFiles,
  LNK_CmdSwitch_Rad_TargetOs,
  LNK_CmdSwitch_Rad_TimeStamp,
  LNK_CmdSwitch_Rad_Version,
  LNK_CmdSwitch_Rad_Workers,

  LNK_CmdSwitch_Help,

  LNK_CmdSwitch_Count
} LNK_CmdSwitchType;

typedef enum
{
  LNK_SwitchState_Null,
  LNK_SwitchState_No,
  LNK_SwitchState_Yes
} LNK_SwitchState;

typedef enum 
{
  LNK_Input_Null,
  LNK_Input_Obj,
  LNK_Input_Lib,
  LNK_Input_Res,
  LNK_Input_Manifest,
  LNK_Input_Count
} LNK_InputType;

enum
{
  LNK_ConfigFlag_Fixed                   = (1 << 0),
  LNK_ConfigFlag_Merge                   = (1 << 1),
  LNK_ConfigFlag_EnvLib                  = (1 << 2),
  LNK_ConfigFlag_DelayUnload             = (1 << 3),
  LNK_ConfigFlag_DelayBind               = (1 << 4),
  LNK_ConfigFlag_CheckUnusedDelayLoadDll = (1 << 5),
  LNK_ConfigFlag_NoTsAware               = (1 << 6),
  LNK_ConfigFlag_WriteImageChecksum      = (1 << 8),
  LNK_ConfigFlag_ManifestEmbed           = (1 << 9),
};
typedef U64 LNK_ConfigFlags;

typedef enum
{
  LNK_DebugMode_Null,
  LNK_DebugMode_None,
  LNK_DebugMode_FastLink,
  LNK_DebugMode_GHash,
  LNK_DebugMode_Full,
} LNK_DebugMode;

enum
{
  LNK_Guard_None    = 0,
  LNK_Guard_Cf      = (1 << 0),
  LNK_Guard_LongJmp = (1 << 1),
  LNK_Guard_EhCont  = (1 << 2),
  LNK_Guard_All     = LNK_Guard_Cf | LNK_Guard_LongJmp | LNK_Guard_EhCont
};
typedef U32 LNK_GuardFlags;

typedef enum
{
  LNK_ManifestOpt_Null,
  LNK_ManifestOpt_WriteToFile,
  LNK_ManifestOpt_Embed,
  LNK_ManifestOpt_No,
} LNK_ManifestOpt;

typedef struct LNK_AltNameList
{
  String8List from_list;
  String8List to_list;
} LNK_AltNameList;

typedef struct LNK_ExportParse
{
  struct LNK_ExportParse *next;
  String8                 name;
  String8                 alias;
  String8                 type;
} LNK_ExportParse;

typedef struct LNK_ExportParseList
{
  U64              count;
  LNK_ExportParse *first;
  LNK_ExportParse *last;
} LNK_ExportParseList;

typedef struct LNK_MergeDirective
{
  String8 src;
  String8 dst;
} LNK_MergeDirective;

typedef struct LNK_MergeDirectiveNode
{
  struct LNK_MergeDirectiveNode *next;
  LNK_MergeDirective             data;
} LNK_MergeDirectiveNode;

typedef struct LNK_MergeDirectiveList
{
  U64                     count;
  LNK_MergeDirectiveNode *first;
  LNK_MergeDirectiveNode *last;
} LNK_MergeDirectiveList;



typedef enum
{
  LNK_DebugInfoGuid_Null,
  Lnk_DebugInfoGuid_ImageBlake3,
} LNK_DebugInfoGuidType;

typedef enum
{
  LNK_TypeNameHashMode_Null,
  LNK_TypeNameHashMode_None,
  LNK_TypeNameHashMode_Lenient,
  LNK_TypeNameHashMode_Full,
} LNK_TypeNameHashMode;

#if OS_WINDOWS
# define LNK_MANIFEST_MERGE_TOOL_NAME "mt.exe"
#elif OS_LINUX || OS_MAC
# define LNK_MANIFEST_MERGE_TOOL_NAME "llvm-mt"
#else
# error 
#endif

#define LNK_DEFAULT_THREAD_POOL_NAME "RADLINK_THREAD_POOL"

typedef struct LNK_Config
{
  LNK_ConfigFlags             flags;
  LNK_DebugMode               debug_mode;
  LNK_SwitchState             opt_ref;
  LNK_SwitchState             opt_icf;
  LNK_SwitchState             opt_lbr;
  U64                         opt_iter_count;
  LNK_GuardFlags              guard_flags;
  LNK_DebugInfoGuidType       guid_type;
  Guid                        guid;
  COFF_TimeStamp              time_stamp;
  U32                         age;
  U64                         section_virt_off;
  U64                         file_align;
  U64                         sect_align;
  U64                         stack_reserve;
  U64                         stack_commit;
  U64                         heap_reserve;
  U64                         heap_commit;
  U64                         user_base_addr;
  U64                         max_image_size;
  U64                         page_size;
  U64                         pdb_page_size;
  U64                         worker_count;
  U64                         max_worker_count;
  String8                     shared_thread_pool_name;
  U64                        *function_pad_min;
  U64                        *manifest_resource_id;
  B32                         no_default_libs;
  Version                     link_ver;
  Version                     os_ver;
  Version                     image_ver;
  OperatingSystem             target_os;
  COFF_MachineType            machine;
  PE_WindowsSubsystem         subsystem;
  Version                     subsystem_ver;
  PE_ImageFileCharacteristics file_characteristics;
  PE_DllCharacteristics       dll_characteristics;
  String8                     entry_point_name;
  String8List                 lib_dir_list;
  PathStyle                   path_style;
  LNK_ManifestOpt             manifest_opt;
  String8                     work_dir;
  String8                     image_name;
  String8                     imp_lib_name;
  String8List                 raw_cmd_line;
  String8                     pdb_name;
  String8                     pdb_alt_path;
  String8                     mt_path;
  LNK_TypeNameHashMode        pdb_hash_type_names;
  String8                     pdb_hash_type_name_map;
  U64                         pdb_hash_type_name_length;
  LNK_ExportParseList         export_symbol_list;
  String8List                 input_list[LNK_Input_Count];
  String8List                 input_default_lib_list;
  String8List                 disallow_lib_list;
  String8List                 delay_load_dll_list;
  String8List                 natvis_list;
  String8                     manifest_name;
  B32                         manifest_uac;
  String8                     manifest_level;
  String8                     manifest_ui_access;
  String8List                 manifest_dependency_list;
  LNK_SwitchState             rad_debug;
  LNK_SwitchState             rad_chunk_map;
  String8                     rad_chunk_map_name;
  String8                     rad_debug_name;
  String8                     rad_debug_alt_path;
  String8List                 include_symbol_list;
  LNK_AltNameList             alt_name_list;
  U64                         symbol_table_cap_defined;
  U64                         symbol_table_cap_internal;
  U64                         symbol_table_cap_weak;
  U64                         symbol_table_cap_lib;
  B32                         build_imp_lib;
  B32                         build_exp;
  LNK_SwitchState             write_temp_files;
  String8                     temp_image_name;
  String8                     temp_pdb_name;
  String8                     temp_rad_debug_name;
  String8                     temp_rad_chunk_map_name;
} LNK_Config;

typedef enum
{
  LNK_ParseU64Flag_CheckUnder32bit = (1 << 0),
  LNK_ParseU64Flag_CheckPow2       = (1 << 1),
} LNK_ParseU64Flags;

////////////////////////////////

typedef enum
{
  LNK_MsErrorCode_Lnk1000 = 1000,
  LNK_MsErrorCode_Lnk1103 = 1103,
  LNK_MsErrorCode_Lnk1104 = 1104,
  LNK_MsErrorCode_Lnk1106 = 1106,
  LNK_MsErrorCode_Lnk1107 = 1107,
  LNK_MsErrorCode_Lnk1112 = 1112,
  LNK_MsErrorCode_Lnk1113 = 1113,
  LNK_MsErrorCode_Lnk1120 = 1120,
  LNK_MsErrorCode_Lnk1123 = 1123,
  LNK_MsErrorCode_Lnk1127 = 1127,
  LNK_MsErrorCode_Lnk1136 = 1136,
  LNK_MsErrorCode_Lnk1140 = 1140,
  LNK_MsErrorCode_Lnk1141 = 1141,
  LNK_MsErrorCode_Lnk1143 = 1143,
  LNK_MsErrorCode_Lnk1152 = 1152,
  LNK_MsErrorCode_Lnk1158 = 1158,
  LNK_MsErrorCode_Lnk1164 = 1164,
  LNK_MsErrorCode_Lnk1166 = 1166,
  LNK_MsErrorCode_Lnk1168 = 1168,
  LNK_MsErrorCode_Lnk1169 = 1169,
  LNK_MsErrorCode_Lnk1170 = 1170,
  LNK_MsErrorCode_Lnk1179 = 1179,
  LNK_MsErrorCode_Lnk1181 = 1181,
  LNK_MsErrorCode_Lnk1189 = 1189,
  LNK_MsErrorCode_Lnk1196 = 1196,
  LNK_MsErrorCode_Lnk1200 = 1200,
  LNK_MsErrorCode_Lnk1201 = 1201,
  LNK_MsErrorCode_Lnk1211 = 1211,
  LNK_MsErrorCode_Lnk1215 = 1215,
  LNK_MsErrorCode_Lnk1218 = 1218,
  LNK_MsErrorCode_Lnk1221 = 1221,
  LNK_MsErrorCode_Lnk1223 = 1223,
  LNK_MsErrorCode_Lnk1224 = 1224,
  LNK_MsErrorCode_Lnk1237 = 1237,
  LNK_MsErrorCode_Lnk1240 = 1240,
  LNK_MsErrorCode_Lnk1241 = 1241,
  LNK_MsErrorCode_Lnk1245 = 1245,
  LNK_MsErrorCode_Lnk1248 = 1248,
  LNK_MsErrorCode_Lnk1256 = 1256,
  LNK_MsErrorCode_Lnk1264 = 1264,
  LNK_MsErrorCode_Lnk1277 = 1277,
  LNK_MsErrorCode_Lnk1282 = 1282,
  LNK_MsErrorCode_Lnk1287 = 1287,
  LNK_MsErrorCode_Lnk1296 = 1296,
  LNK_MsErrorCode_Lnk1301 = 1301,
  LNK_MsErrorCode_Lnk1302 = 1302,
  LNK_MsErrorCode_Lnk1306 = 1306,
  LNK_MsErrorCode_Lnk1309 = 1309,
  LNK_MsErrorCode_Lnk1312 = 1312,
  LNK_MsErrorCode_Lnk1313 = 1313,
  LNK_MsErrorCode_Lnk1314 = 1314,
  LNK_MsErrorCode_Lnk1318 = 1318,
  LNK_MsErrorCode_Lnk1332 = 1332,
  LNK_MsErrorCode_Lnk1352 = 1352,
  LNK_MsErrorCode_Lnk1561 = 1561,
  LNK_MsErrorCode_Lnk2001 = 2001,
  LNK_MsErrorCode_Lnk2004 = 2004,
  LNK_MsErrorCode_Lnk2005 = 2005,
  LNK_MsErrorCode_Lnk2008 = 2008,
  LNK_MsErrorCode_Lnk2011 = 2011,
  LNK_MsErrorCode_Lnk2013 = 2013,
  LNK_MsErrorCode_Lnk2017 = 2017,
  LNK_MsErrorCode_Lnk2019 = 2019,
  LNK_MsErrorCode_Lnk2020 = 2020,
  LNK_MsErrorCode_Lnk2022 = 2022,
  LNK_MsErrorCode_Lnk2023 = 2023,
  LNK_MsErrorCode_Lnk2026 = 2026,
  LNK_MsErrorCode_Lnk2027 = 2027,
  LNK_MsErrorCode_Lnk2031 = 2031,
  LNK_MsErrorCode_Lnk2033 = 2033,
  LNK_MsErrorCode_Lnk2038 = 2038,
  LNK_MsErrorCode_Lnk2039 = 2039,

  LNK_MsWarningCode_Lnk4001 = 4001,
  LNK_MsWarningCode_Lnk4002 = 4002,
  LNK_MsWarningCode_Lnk4006 = 4006,
  LNK_MsWarningCode_Lnk4010 = 4010,
  LNK_MsWarningCode_Lnk4014 = 4014,
  LNK_MsWarningCode_Lnk4020 = 4020,
  LNK_MsWarningCode_Lnk4022 = 4022,
  LNK_MsWarningCode_Lnk4039 = 4039,
  LNK_MsWarningCode_Lnk4044 = 4044,
  LNK_MsWarningCode_Lnk4049 = 4049,
  LNK_MsWarningCode_Lnk4065 = 4065,
  LNK_MsWarningCode_Lnk4070 = 4070,
  LNK_MsWarningCode_Lnk4071 = 4071,
  LNK_MsWarningCode_Lnk4073 = 4073,
  LNK_MsWarningCode_Lnk4075 = 4075,
  LNK_MsWarningCode_Lnk4076 = 4076,
  LNK_MsWarningCode_SectionFlagsConflict = 4078,
  LNK_MsWarningCode_Lnk4086 = 4086,
  LNK_MsWarningCode_Lnk4092 = 4092,
  LNK_MsWarningCode_Lnk4096 = 4096,
  LNK_MsWarningCode_Lnk4098 = 4098,
  LNK_MsWarningCode_MissingExternalTypeServer = 4099,
  LNK_MsWarningCode_Lnk4102 = 4102,
  LNK_MsWarningCode_Lnk4104 = 4104,
  LNK_MsWarningCode_Lnk4105 = 4105,
  LNK_MsWarningCode_Lnk4194 = 4194,
  LNK_MsWarningCode_Lnk4197 = 4197,
  LNK_MsWarningCode_UnsuedDelayLoadDll = 4199,
  LNK_MsWarningCode_Lnk4200 = 4200,
  LNK_MsWarningCode_Lnk4204 = 4204,
  LNK_MsWarningCode_Lnk4205 = 4205,
  LNK_MsWarningCode_Lnk4206 = 4206,
  LNK_MsWarningCode_Lnk4210 = 4210,
  LNK_MsWarningCode_Lnk4216 = 4216,
  LNK_MsWarningCode_Lnk4217 = 4217,
  LNK_MsWarningCode_Lnk4219 = 4219,
  LNK_MsWarningCode_Lnk4220 = 4220,
  LNK_MsWarningCode_Lnk4221 = 4221,
  LNK_MsWarningCode_Lnk4222 = 4222,
  LNK_MsWarningCode_Lnk4224 = 4224,
  LNK_MsWarningCode_Lnk4227 = 4227,
  LNK_MsWarningCode_Lnk4229 = 4229,
  LNK_MsWarningCode_Lnk4237 = 4237,
  LNK_MsWarningCode_Lnk4247 = 4247,
  LNK_MsWarningCode_Lnk4248 = 4248,
  LNK_MsWarningCode_Lnk4253 = 4253,
  LNK_MsWarningCode_Lnk4254 = 4254,
  LNK_MsWarningCode_Lnk4286 = 4286,
} LNK_MsErrorCode;

////////////////////////////////
// Enum <-> String

internal String8              lnk_string_cmd_switch_type(LNK_CmdSwitchType type);
internal LNK_CmdSwitchType    lnk_cmd_switch_from_string(String8 string);
internal LNK_InputType        lnk_input_type_from_string(String8 string);
internal LNK_DebugMode        lnk_debug_mode_from_string(String8 string);
internal LNK_TypeNameHashMode lnk_type_name_hash_mode_from_string(String8 string);

////////////////////////////////
// Command Line Helpers

internal LNK_CmdOption * lnk_cmd_line_push_option_if_not_presentf(Arena *arena, LNK_CmdLine *cmd_line, LNK_CmdSwitchType cmd_switch_type, char *param_fmt, ...);
internal LNK_CmdOption * lnk_cmd_line_push_optionf               (Arena *arena, LNK_CmdLine *cmd_line, LNK_CmdSwitchType cmd_switch_type, char *param_fmt, ...);
internal B32             lnk_cmd_line_has_switch(LNK_CmdLine cmd_line, LNK_CmdSwitchType cmd_switch_type);

////////////////////////////////
// Errors

internal void lnk_error_cmd_switch                    (LNK_ErrorCode code, String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, char *fmt, ...);
internal void lnk_error_cmd_switch_invalid_param_count(LNK_ErrorCode code, String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch);
internal void lnk_error_cmd_switch_invalid_param      (LNK_ErrorCode code, String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8 param);

////////////////////////////////
// Getters

internal U64     lnk_get_default_function_pad_min(COFF_MachineType machine);
internal U64     lnk_get_base_addr(LNK_Config *config);
internal Version lnk_get_default_subsystem_version(PE_WindowsSubsystem subsystem, COFF_MachineType machine);
internal Version lnk_get_min_subsystem_version(PE_WindowsSubsystem subsystem, COFF_MachineType machine);

internal B32 lnk_do_debug_info        (LNK_Config *config);
internal B32 lnk_is_thread_pool_shared(LNK_Config *config);

////////////////////////////////
// Specialized Parsers

internal B32  lnk_cmd_switch_parse_version  (String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, Version *ver_out);
internal B32  lnk_cmd_switch_parse_tuple    (String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, Rng1U64 *tuple_out);
internal B32  lnk_cmd_switch_parse_u64      (String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, U64 *value_out, LNK_ParseU64Flags flags);
internal B32  lnk_cmd_switch_parse_u32      (String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, U32 *value_out, LNK_ParseU64Flags flags);
internal B32  lnk_cmd_switch_parse_flag     (String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, LNK_SwitchState *value_out);
internal void lnk_cmd_switch_set_flag_inv_16(String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, U16 *flags, U16 bits);
internal void lnk_cmd_switch_set_flag_inv_64(String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, U64 *flags, U64 bits);
internal void lnk_cmd_switch_set_flag_16    (String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, U16 *flags, U16 bits);
internal void lnk_cmd_switch_set_flag_32    (String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, U32 *flags, U32 bits);
internal void lnk_cmd_switch_set_flag_64    (String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, U64 *flags, U64 bits);
internal B32  lnk_cmd_switch_parse_string   (String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, String8 *string_out);
internal void lnk_cmd_switch_parse_string_copy(Arena *arena, String8 obj_path, String8 lib_path, LNK_CmdSwitchType cmd_switch, String8List value_strings, String8 *string_out);

////////////////////////////////

internal void      lnk_alt_name_list_concat_in_place(LNK_AltNameList *list, LNK_AltNameList *to_concat);
internal B32       lnk_parse_alt_name_directive     (Arena *arena, String8 input, LNK_AltNameList *list_out);
internal String8 * lnk_parse_alt_name_directive_list(Arena *arena, String8List list, LNK_AltNameList *list_out);

internal LNK_ExportParse * lnk_parse_export_directive(Arena *arena, LNK_ExportParseList *list, String8List value_list, String8 obj_path, String8 lib_path);

internal LNK_MergeDirectiveNode * lnk_merge_directive_list_push(Arena *arena, LNK_MergeDirectiveList *list, LNK_MergeDirective data);
internal B32                      lnk_parse_merge_directive(String8 string, LNK_MergeDirective *out);

////////////////////////////////

internal void lnk_apply_cmd_option_to_config(Arena *arena, LNK_Config *config, String8 name, String8List value_list, String8 obj_path, String8 lib_path);

internal LNK_Config * lnk_config_from_raw_cmd_line(Arena *arena, String8List raw_cmd_line);
internal LNK_Config * lnk_build_config            (Arena *arena, int argc, char **argv);

