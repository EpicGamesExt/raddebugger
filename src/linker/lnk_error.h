// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef enum
{
  LNK_Error_Null,
  
  LNK_Error_StopFirst,
  LNK_Error_Cmdl,
  LNK_Error_EndprecompNotFound,
  LNK_Error_EntryPoint,
  LNK_Error_ExternalTypeServerConflict,
  LNK_Error_FileNotFound,
  LNK_Error_IllData,
  LNK_Error_IllExport,
  LNK_Error_IncomatibleCmdOptions,
  LNK_Error_IncompatibleObj,
  LNK_Error_InvalidPrecompLeafCount,
  LNK_Error_InvalidStartIndex,
  LNK_Error_NoAccess,
  LNK_Error_NoSubsystem,
  LNK_Error_OutOfExportOrdinals,
  LNK_Error_PrecompObjNotFound,
  LNK_Error_PrecompSigMismatch,
  LNK_Error_Telemetry,
  LNK_Error_UnsupportedMachine,
  LNK_Error_Mt,
  LNK_Error_UnableToSerializeMsf,
  LNK_Error_LoadRes,
  LNK_Error_IO,
  LNK_Error_LargeAddrAwareRequired,
  LNK_Error_InvalidPath,
  LNK_Error_StopLast,
  
  LNK_Error_First,
  LNK_Error_AlreadyDefinedSymbol,
  LNK_Error_AlternateNameConflict,
  LNK_Error_CvPrecomp,
  LNK_Error_MultiplyDefinedSymbol,
  LNK_Error_Natvis,
  LNK_Error_TooManyFiles,
  LNK_Error_UndefinedSymbol,
  LNK_Error_UnresolvedSymbol,
  LNK_Error_UnableToOpenTypeServer,
  LNK_Error_UnexpectedCodePath,
  LNK_Error_CvIllSymbolData,
  LNK_Error_IllegalAlternateNameRedifine,
  LNK_Error_InvalidTypeIndex,
  LNK_Error_Last,
  
  LNK_Warning_First,
  LNK_Warning_AmbiguousMerge,
  LNK_Warning_AtypicalStartIndex,
  LNK_Warning_Cmdl,
  LNK_Warning_Directive,
  LNK_Warning_DuplicateObjPath,
  LNK_Warning_ExternalTypeServerAgeMismatch,
  LNK_Warning_FileNotFound,
  LNK_Warning_IllData,
  LNK_Warning_IllExport,
  LNK_Warning_InvalidNatvisFileExt,
  LNK_Warning_LargePages,
  LNK_Warning_LargePagesNotEnabled,
  LNK_Warning_MismatchedTypeServerSignature,
  LNK_Warning_MissingExternalTypeServer,
  LNK_Warning_MultipleDebugTAndDebugP,
  LNK_Warning_MultipleExternalTypeServers,
  LNK_Warning_MultipleLibMatch,
  LNK_Warning_MultiplyDefinedImport,
  LNK_Warning_Natvis,
  LNK_Warning_PrecompObjSymbolsNotFound,
  LNK_Warning_SectionFlagsConflict,
  LNK_Warning_Subsystem,
  LNK_Warning_UnknownDirective,
  LNK_Warning_IllegalDirective,
  LNK_Warning_UnresolvedComdat,
  LNK_Warning_UnusedDelayLoadDll,
  LNK_Warning_LongSectionName,
  LNK_Warning_UnknownSwitch,
  LNK_Warning_TLSAlign,
  LNK_Warning_DirectiveSectionWithRelocs,
  LNK_Warning_Last,
  
  LNK_Error_Count
} LNK_ErrorCode;

typedef enum
{
  LNK_ErrorMode_Ignore,
  LNK_ErrorMode_Stop,
  LNK_ErrorMode_Continue,
  LNK_ErrorMode_Warn,
} LNK_ErrorMode;

typedef enum
{
  LNK_InternalError_Null,
  LNK_InternalError_NotImplemented,
  LNK_InternalError_InvalidPath,
  LNK_InternalError_IncompleteSwitch,
  LNK_InternalError_OutOfMemory
} LNK_InternalError;

typedef enum
{
  LNK_ErrorCodeStatus_Active,
  LNK_ErrorCodeStatus_Ignore,
} LNK_ErrorCodeStatus;

internal void lnk_init_error_handler(void);
internal void lnk_errorfv(LNK_ErrorCode code, char *fmt, va_list args);
internal void lnk_error(LNK_ErrorCode code, char *fmt, ...);
internal void lnk_error_with_loc(LNK_ErrorCode code, String8 obj_path, String8 lib_path, char *fmt, ...);
internal void lnk_supplement_error(char *fmt, ...);
internal void lnk_supplement_error_list(String8List list);
internal void lnk_suppress_error(LNK_ErrorCode code);

#define lnk_is_error_code_active(code)  (lnk_get_error_code_status(code) == LNK_ErrorCodeStatus_Active)
#define lnk_is_error_code_ignored(code) (lnk_get_error_code_status(code) == LNK_ErrorCodeStatus_Ignore)
internal LNK_ErrorCodeStatus lnk_get_error_code_status(LNK_ErrorCode code);

internal void lnk_internal_error(LNK_InternalError code, char *file, int line, char *fmt, ...);
#define lnk_invalid_path(...)      lnk_internal_error(LNK_InternalError_InvalidPath, __FILE__, __LINE__, __VA_ARGS__)
#define lnk_not_implemented(...)   lnk_internal_error(LNK_InternalError_NotImplemented, __FILE__, __LINE__, __VA_ARGS__)
#define lnk_incomplete_switch(...) lnk_internal_error(LNK_InternalError_IncompleteSwitch, __FILE__, __LINE__, __VA_ARGS__)

