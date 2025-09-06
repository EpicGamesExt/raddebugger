// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

global read_only LNK_CmdSwitch g_cmd_switch_map[] =
{
  { LNK_CmdSwitch_Null,               0, "",                     "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "NOT_IMPLEMENTED",      "", ""                                                                                                      },
  { LNK_CmdSwitch_Align,              0, "ALIGN",                ":#", ""                                                                                                    },
  { LNK_CmdSwitch_AllowBind,          0, "ALLOWBIND",            "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_AllowIsolation,     0, "ALLOWISOLATION",       "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_AlternateName,      1, "ALTERNATENAME",        "Creates an a symbol alias \"FROM=TO\"."                                                                    },
  { LNK_CmdSwitch_AppContainer,       0, "APPCONTAINER",         "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_NotImplemented,     0, "ASSEMBLYDEBUG",        "", ""                                                                                                      }, // .NET
  { LNK_CmdSwitch_NotImplemented,     0, "ASSEMBLYLINKRESOURCE", "", ""                                                                                                      }, // .NET
  { LNK_CmdSwitch_NotImplemented,     0, "ASSEMBLYMODULE",       "", ""                                                                                                      }, // .NET
  { LNK_CmdSwitch_NotImplemented,     0, "ASSEMBLYRESOURCE",     "", ""                                                                                                      }, // .NET
  { LNK_CmdSwitch_Base,               0, "BASE",                 "{ADDRESS[,SIZE]|@FILENAME,KEY}", ""                                                                        },
  { LNK_CmdSwitch_NotImplemented,     0, "CLRIMAGETYPE",         "", ""                                                                                                      }, // .NET
  { LNK_CmdSwitch_NotImplemented,     0, "CLRLOADEROPTIMIZATION","", ""                                                                                                      }, // .NET
  { LNK_CmdSwitch_NotImplemented,     0, "CLRSUPPORTLASTERROR",  "", ""                                                                                                      }, // .NET
  { LNK_CmdSwitch_NotImplemented,     0, "CLRTHREADATTRIBUTE",   "", ""                                                                                                      }, // .NET
  { LNK_CmdSwitch_NotImplemented,     0, "CLRUNMANAGEDCODECHECK","", ""                                                                                                      }, // .NET
  { LNK_CmdSwitch_Debug,              0, "DEBUG",                "[:{FULL|NONE}]", ""                                                                                        },
  { LNK_CmdSwitch_Dump,               0, "DUMP",                 "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "DEF",                  ":FILENAME", ""                                                                                             },
  { LNK_CmdSwitch_DefaultLib,         1, "DEFAULTLIB",           ":LIBNAME", ""                                                                                              },
  { LNK_CmdSwitch_Delay,              0, "DELAY",                ":{NOBIND|UNLOAD}", ""                                                                                      },
  { LNK_CmdSwitch_DelayLoad,          0, "DELAYLOAD",            ":DLL", ""                                                                                                  },
  { LNK_CmdSwitch_NotImplemented,     0, "DELAYSIGN",            "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "DEPENDENTLOADFLAG",    "", ""                                                                                                      },
  { LNK_CmdSwitch_Dll,                0, "DLL",                  "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "DRIVER",               "", ""                                                                                                      },
  { LNK_CmdSwitch_DisallowLib,        1, "DISALLOWLIB",          ":LIBRARY", "",                                                                                             },
  { LNK_CmdSwitch_EditAndContinue,    1, "EDITANDCONTINUE",      "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_DynamicBase,        0, "DYNAMICBASE",          "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_NotImplemented,     0, "EMITVOLATILEMETADATA", "", ""                                                                                                      },
  { LNK_CmdSwitch_Entry,              1, "ENTRY",                ":FUNCTION", ""                                                                                             },
  { LNK_CmdSwitch_Null,               0, "ERRORREPORT",          "", "Deprecated starting Windows Vista."                                                                    },
  { LNK_CmdSwitch_Export,             1, "EXPORT",               ":SYMBOL", ""                                                                                               },
  { LNK_CmdSwitch_NotImplemented,     0, "EXPORTADMIN",          "", ""                                                                                                      },
  { LNK_CmdSwitch_FastFail,           0, "FASTFAIL",             "", "Not used."                                                                                             },
  { LNK_CmdSwitch_NotImplemented,     0, "FASTGENPROFILE",       "", ""                                                                                                      },
  { LNK_CmdSwitch_FailIfMismatch,     1, "FAILIFMISMATCH",       "", ""                                                                                                      },
  { LNK_CmdSwitch_FileAlign,          0, "FILEALIGN",            ":#", ""                                                                                                    },
  { LNK_CmdSwitch_Fixed,              0, "FIXED",                "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_NotImplemented,     0, "FORCE",                "", ""                                                                                                      },
  { LNK_CmdSwitch_FunctionPadMin,     0, "FUNCTIONPADMIN",      ":#", "Not Implemented"                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "GUARD",                "", ""                                                                                                      },
  { LNK_CmdSwitch_GuardSym,           1, "GUARDSYM",             "", "",                                                                                                     },
  { LNK_CmdSwitch_NotImplemented,     0, "GENPROFILE",           "", ""                                                                                                      },
  { LNK_CmdSwitch_Heap,               0, "HEAP",                 "RESERVE[,COMMIT]", ""                                                                                      },
  { LNK_CmdSwitch_HighEntropyVa,      0, "HIGHENTROPYVA",        "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_NotImplemented,     0, "IDLOUT",               "", ""                                                                                                      },
  { LNK_CmdSwitch_Ignore,             0, "IGNORE",               ":#", ""                                                                                                    },
  { LNK_CmdSwitch_NotImplemented,     0, "IGNOREIDL",            "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "ILK",                  "", ""                                                                                                      },
  { LNK_CmdSwitch_ImpLib,             0, "IMPLIB",               ":FILENAME", ""                                                                                             },
  { LNK_CmdSwitch_Include,            1, "INCLUDE",              "", ""                                                                                                      },
  { LNK_CmdSwitch_Incremental,        0, "INCREMENTAL",          "[:NO]", "Incremental linking is not supported."                                                            },
  { LNK_CmdSwitch_NotImplemented,     0, "INTEGRITYCHECK",       "", ""                                                                                                      },
  { LNK_CmdSwitch_InferAsanLibs,      1, "INFERASANLIBS",        "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_InferAsanLibsNo,    1, "INFERASANLIBSNO",      "", "",                                                                                                     },
  { LNK_CmdSwitch_NotImplemented,     0, "KERNEL",               "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "KEYCONTAINER",         "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "KEYFILE",              "", ""                                                                                                      },
  { LNK_CmdSwitch_LargeAddressAware,  0, "LARGEADDRESSAWARE",    "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_Lib,                0, "LIB",                  ""                                                                                                          },
  { LNK_CmdSwitch_LibPath,            0, "LIBPATH",              ":DIR", ""                                                                                                  },
  { LNK_CmdSwitch_NotImplemented,     0, "LINKERREPO",           "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "LINKERREPOTARGET",     "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "LTCG",                 "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "LTCGOUT",              "", ""                                                                                                      },
  { LNK_CmdSwitch_Machine,            0, "MACHINE",              ":{X64|X86}", ""                                                                                            },
  { LNK_CmdSwitch_Manifest,           0, "MANIFEST",             "[:{EMBED[,ID=#]|NO]", ""                                                                                   },
  { LNK_CmdSwitch_ManifestDependency, 1, "MANIFESTDEPENDENCY",   ":\"manifest dependency XML string\"", ""                                                                   },
  { LNK_CmdSwitch_ManifestFile,       0, "MANIFESTFILE",         ":FILENAME", ""                                                                                             },
  { LNK_CmdSwitch_ManifestInput,      0, "MANIFESTINPUT",        ":FILENAME", ""                                                                                             },
  { LNK_CmdSwitch_ManifestUac,        0, "MANIFESTUAC",          ":{NO|{'level'={'asInvoker'|'highestAvailable'|'requireAdministrator'} ['uiAccess'={'true'|'false'}]}}", "" },
  { LNK_CmdSwitch_NotImplemented,     0, "MAP",                  "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "MAPINFO",              "", ""                                                                                                      },
  { LNK_CmdSwitch_Merge,              1, "MERGE",                ":from=to", ""                                                                                              },
  { LNK_CmdSwitch_NotImplemented,     0, "MIDL",                 "", ""                                                                                                      },
  { LNK_CmdSwitch_Natvis,             0, "NATVIS",               ":FILENAME", ""                                                                                             },
  { LNK_CmdSwitch_NotImplemented,     0, "NOASSEMBLY",           "", ""                                                                                                      },
  { LNK_CmdSwitch_NoDefaultLib,       1, "NODEFAULTLIB",         ":LIBNAME", ""                                                                                              },
  { LNK_CmdSwitch_NoDefaultLib,       0, "NOD",                  ":LIBNAME", ""                                                                                              },
  { LNK_CmdSwitch_NotImplemented,     0, "NOENTRY",              "", ""                                                                                                      },
  { LNK_CmdSwitch_NoExp,              0, "NOEXP",                "", ".exp is not supported."                                                                                },
  { LNK_CmdSwitch_NoImpLib,           0, "NOIMPLIB",             "", ""                                                                                                      },
  { LNK_CmdSwitch_NoLogo,             0, "NOLOGO",               "", ""                                                                                                      },
  { LNK_CmdSwitch_NxCompat,           0, "NXCOMPAT",             "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_Opt,                0, "OPT",                  "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "ORDER",                "", ""                                                                                                      },
  { LNK_CmdSwitch_Out,                0, "OUT",                  ":FILENAME", ""                                                                                             },
  { LNK_CmdSwitch_Pdb,                0, "PDB",                  ":FILENAME", ""                                                                                             },
  { LNK_CmdSwitch_PdbAltPath,         0, "PDBALTPATH",           "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "PDBSTRIPPED",          "", ""                                                                                                      },
  { LNK_CmdSwitch_PdbPageSize,        0, "PDBPAGESIZE",          ":#", "Page size must be power of two"                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "PROFILE",              "", ""                                                                                                      },
  { LNK_CmdSwitch_Release,            1, "RELEASE",              "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "SAFESEH",              "", ""                                                                                                      },
  { LNK_CmdSwitch_Section,            1, "SECTION",              "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "SOURCELINK",           "", ""                                                                                                      },
  { LNK_CmdSwitch_Stack,              1, "STACK",                ":RESERVE[,COMMIT]", ""                                                                                     },
  { LNK_CmdSwitch_NotImplemented,     0, "STUB",                 "", ""                                                                                                      },
  { LNK_CmdSwitch_SubSystem,          1, "SUBSYSTEM",            ":{CONSOLE|NATIVE|WINDOWS}[,#[.##]]", ""                                                                    },
  { LNK_CmdSwitch_NotImplemented,     0, "SWAPRUN",              "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "TLBID",                "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "TLBOUT",               "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "TIME",                 "", ""                                                                                                      },
  { LNK_CmdSwitch_TsAware,            0, "TSAWARE",              "[:NO]", ""                                                                                                 },
  { LNK_CmdSwitch_ThrowingNew,        1, "THROWINGNEW",          "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "USERPROFILE",          "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "VERBOSE",              "", ""                                                                                                      },
  { LNK_CmdSwitch_Version,            0, "VERSION",              "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "WINMD",                "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "WINMDDELAYSIGN",       "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "WINMDKEYCONTAINER",    "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "WINMDKEYFILE",         "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "WHOLEARCHIVE",         "", ""                                                                                                      },
  { LNK_CmdSwitch_NotImplemented,     0, "WX",                   "", ""                                                                                                      },

  //- internal switches
  { LNK_CmdSwitch_Rad_Age,                          0, "RAD_AGE",                              ":#",        "Age embeded in EXE and PDB, used to validate incremental build. Default is 1."    },
  { LNK_CmdSwitch_Rad_BuildInfo,                    0, "RAD_BUILD_INFO",                       "",          "Print build info and exit."                                                       },
  { LNK_CmdSwitch_Rad_CheckUnusedDelayLoadDll,      0, "RAD_CHECK_UNUSED_DELAY_LOAD_DLL",      "[:NO]",     ""                                                                                 },
  { LNK_CmdSwitch_Rad_Map,                          0, "RAD_MAP",                              ":FILENAME", "Emit file with the output image's layout description."                            },
  { LNK_CmdSwitch_Rad_MapLinesForUnresolvedSymbols, 0, "RAD_MAP_LINES_FOR_UNRESOLVED_SYMBOLS", "[:NO]",     "Use debug info to print source file location for unresolved symbol"               },
  { LNK_CmdSwitch_Rad_MemoryMapFiles,               0, "RAD_MEMORY_MAP_FILES",                 "[:NO]",     "When enabled, files are memory-mapped instead of being read entirely on request." },
  { LNK_CmdSwitch_Rad_Debug,                        0, "RAD_DEBUG",                            "[:NO]",     "Emit RAD debug info file."                                                        },
  { LNK_CmdSwitch_Rad_DebugAltPath,                 0, "RAD_DEBUGALTPATH",                     "", ""                                                                                          },
  { LNK_CmdSwitch_Rad_DebugName,                    0, "RAD_DEBUG_NAME",                       ":FILENAME", "Sets file name for RAD debug info file."                                          },
  { LNK_CmdSwitch_Rad_DelayBind,                    0, "RAD_DELAY_BIND",                       "[:NO]", ""                                                                                     },
  { LNK_CmdSwitch_Rad_DoMerge,                      0, "RAD_DO_MERGE",                         "[:NO]", ""                                                                                     },
  { LNK_CmdSwitch_Rad_EnvLib,                       0, "RAD_ENV_LIB",                          "[:NO]", ""                                                                                     },
  { LNK_CmdSwitch_Rad_Exe,                          0, "RAD_EXE",                              "[:NO]", ""                                                                                     },
  { LNK_CmdSwitch_Rad_Guid,                         0, "RAD_GUID",                             ":{IMAGEBLAKE3|XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXXXXXX}", ""                                   },
  { LNK_CmdSwitch_Rad_LargePages,                   0, "RAD_LARGE_PAGES",                      "[:NO]",     "Disabled by default on Windows."                                                  },
  { LNK_CmdSwitch_Rad_LinkVer,                      0, "RAD_LINK_VER",                         ":##,##", ""                                                                                    },
  { LNK_CmdSwitch_Rad_Log,                          0, "RAD_LOG",                              ":{ALL,INPUT_OBJ,INPUT_LIB,IO,LINK_STATS,TIMERS}", ""                                           },
  { LNK_CmdSwitch_Rad_MtPath,                       0, "RAD_MT_PATH",                          ":EXEPATH",  "Exe path to manifest tool, default: " LNK_MANIFEST_MERGE_TOOL_NAME                },
  { LNK_CmdSwitch_Rad_OsVer,                        0, "RAD_OS_VER",                           ":##,##", ""                                                                                    },
  { LNK_CmdSwitch_Rad_PageSize,                     0, "RAD_PAGE_SIZE",                        ":#",        "Must be power of two."                                                            },
  { LNK_CmdSwitch_Rad_PathStyle,                    0, "RAD_PATH_STYLE",                       ":{WindowsAbsolute|UnixAbsolute}", ""                                                           },
  { LNK_CmdSwitch_Rad_PdbHashTypeNameLength,        0, "RAD_PDB_HASH_TYPE_NAME_LENGTH",        ":#",        "Number of hash bytes to use to replace type name. Default 8 bytes (Max 16)."      },
  { LNK_CmdSwitch_Rad_PdbHashTypeNameMap,           0, "RAD_PDB_HASH_TYPE_NAME_MAP",           ":FILENAME", "Produce map file with hash -> type name mappings."                                },
  { LNK_CmdSwitch_Rad_PdbHashTypeNames,             0, "RAD_PDB_HASH_TYPE_NAMES",              ":{NONE|LENIENT|FULL}", "Replace type names in LF_STRUCTURE and LF_CLASS with hashes."          },
  { LNK_CmdSwitch_Rad_RemoveSection,                0, "RAD_REMOVE_SECTION",                   ":NAME",     "Removes a section from output image."                                             },
  { LNK_CmdSwitch_Rad_SharedThreadPool,             0, "RAD_SHARED_THREAD_POOL",               "[:STRING]", "Default value \"" LNK_DEFAULT_THREAD_POOL_NAME "\""                               },
  { LNK_CmdSwitch_Rad_SharedThreadPoolMaxWorkers,   0, "RAD_SHARED_THREAD_POOL_MAX_WORKERS",   ":#",        "Sets maximum number of workers in a thread pool."                                 },
  { LNK_CmdSwitch_Rad_SuppressError,                0, "RAD_SUPPRESS_ERROR",                   ":#",        ""                                                                                 },
  { LNK_CmdSwitch_Rad_TargetOs,                     0, "RAD_TARGET_OS",                        ":{WINDOWS,LINUX,MAC}"                                                                          },
  { LNK_CmdSwitch_Rad_WriteTempFiles,               0, "RAD_WRITE_TEMP_FILES",                 "[:NO]",     "When speicifed linker writes image and debug info to temporary files and renames after link is done." },
  { LNK_CmdSwitch_Rad_TimeStamp,                    0, "RAD_TIME_STAMP",                       ":#",        "Time stamp embeded in EXE and PDB."                                               },
  { LNK_CmdSwitch_Rad_UnresolvedSymbolLimit,        0, "RAD_UNRESOLVED_SYMBOL_LIMIT",          ":#",        "Limits number of unresolved symbol errors linker reports."                        },
  { LNK_CmdSwitch_Rad_UnresolvedSymbolRefLimit,     0, "RAD_UNRESOLVED_SYMBOL_REF_LIMIT",      ":#",        "Limit number of unresolved symbol references linker reports."                     },
  { LNK_CmdSwitch_Rad_Version,                      0, "RAD_VERSION",                          "",          "Print version and exit."                                                          },
  { LNK_CmdSwitch_Rad_Workers,                      0, "RAD_WORKERS",                          ":#",        "Sets number of workers created in the pool. Number is capped at 1024. When /RAD_SHARED_THREAD_POOL is specified this number cant exceed /RAD_SHARED_THREAD_POOL_MAX_WORKERS." },

  { LNK_CmdSwitch_Help, 0, "HELP", "", "" },
  { LNK_CmdSwitch_Help, 0, "?",    "", "" },
};

global read_only struct
{
  char         *name;
  LNK_InputType type;
} g_input_type_map[] = {
  { "o",    LNK_Input_Obj },
  { "obj",  LNK_Input_Obj },
  { "lib",  LNK_Input_Lib },
  { "rlib", LNK_Input_Lib }, // rust libs
  { "res",  LNK_Input_Res },
};

global read_only struct
{
  char         *name;
  LNK_DebugMode mode;
} g_debug_mode_map[] = {
  { "null",     LNK_DebugMode_Null     },
  { "none",     LNK_DebugMode_None     },
  { "fastlink", LNK_DebugMode_FastLink },
  { "ghash",    LNK_DebugMode_GHash    },
  { "full",     LNK_DebugMode_Full     },
}; 

global read_only struct
{
   char                 *name;
   LNK_TypeNameHashMode  mode;
} g_type_name_hash_mode_map[] = {
  { "none",    LNK_TypeNameHashMode_None    },
  { "lenient", LNK_TypeNameHashMode_Lenient },
  { "full",    LNK_TypeNameHashMode_Full    }
};

internal LNK_CmdSwitchType
lnk_cmd_switch_type_from_string(String8 name)
{
  for (U64 i = 0; i < ArrayCount(g_cmd_switch_map); i += 1) {
    if (str8_match_cstr(g_cmd_switch_map[i].name, name, StringMatchFlag_CaseInsensitive)) {
      return g_cmd_switch_map[i].type;
    }
  }
  return LNK_CmdSwitch_Null;
}

internal LNK_CmdSwitch *
lnk_cmd_switch_from_string(String8 name)
{
  for (U64 i = 0; i < ArrayCount(g_cmd_switch_map); i += 1) {
    if (str8_match_cstr(g_cmd_switch_map[i].name, name, StringMatchFlag_CaseInsensitive)) {
      return &g_cmd_switch_map[i];
    }
  }
  return 0;
}

internal LNK_CmdSwitch *
lnk_cmd_switch_from_type(LNK_CmdSwitchType type)
{
  for (U64 cmd_idx = 0; cmd_idx < ArrayCount(g_cmd_switch_map); cmd_idx += 1) {
    if (g_cmd_switch_map[cmd_idx].type == type) {
      return &g_cmd_switch_map[cmd_idx];
    }
  }
  return 0;
}

internal String8
lnk_string_from_cmd_switch_type(LNK_CmdSwitchType type)
{
  LNK_CmdSwitch *cmd_switch = lnk_cmd_switch_from_type(type);
  return cmd_switch ? str8_cstring(cmd_switch->name) : str8_zero();
}

internal LNK_InputType
lnk_input_type_from_string(String8 name)
{
  for (U64 i = 0; i < ArrayCount(g_input_type_map); i += 1) {
    if (str8_match_cstr(g_input_type_map[i].name, name, StringMatchFlag_CaseInsensitive)) {
      return g_input_type_map[i].type;
    }
  }
  return LNK_Input_Null;
}

internal LNK_DebugMode
lnk_debug_mode_from_string(String8 name)
{
  for (U64 i = 0; i < ArrayCount(g_debug_mode_map); i += 1) {
    if (str8_match_cstr(g_debug_mode_map[i].name, name, StringMatchFlag_CaseInsensitive)) {
      return g_debug_mode_map[i].mode;
    }
  }
  return LNK_DebugMode_Null;
}

internal LNK_TypeNameHashMode
lnk_type_name_hash_mode_from_string(String8 name)
{
  for (U64 i = 0; i < ArrayCount(g_type_name_hash_mode_map); i += 1) {
    if (str8_match_cstr(g_type_name_hash_mode_map[i].name, name, StringMatchFlag_CaseInsensitive)) {
      return g_type_name_hash_mode_map[i].mode;
    }
  }
  return LNK_TypeNameHashMode_Null;
}

internal LNK_CmdOption *
lnk_cmd_line_push_option_if_not_presentf(Arena *arena, LNK_CmdLine *cmd_line, LNK_CmdSwitchType cmd_switch_type, char *param_fmt, ...)
{
  LNK_CmdOption *opt = 0;
  String8 cmd_switch_name = lnk_string_from_cmd_switch_type(cmd_switch_type);
  if (!lnk_cmd_line_has_option_string(*cmd_line, cmd_switch_name)) {
    va_list param_args;
    va_start(param_args, param_fmt);
    String8 param_str = push_str8fv(arena, param_fmt, param_args);
    va_end(param_args);

    opt = lnk_cmd_line_push_option_string(arena, cmd_line, cmd_switch_name, param_str);
  }
  return opt;
}

internal LNK_CmdOption *
lnk_cmd_line_push_optionf(Arena *arena, LNK_CmdLine *cmd_line, LNK_CmdSwitchType cmd_switch, char *param_fmt, ...)
{
  va_list param_args;
  va_start(param_args, param_fmt);
  String8 param_str = push_str8fv(arena, param_fmt, param_args);
  va_end(param_args);
  String8 cmd_switch_name = lnk_string_from_cmd_switch_type(cmd_switch);
  LNK_CmdOption *opt = lnk_cmd_line_push_option_string(arena, cmd_line, cmd_switch_name, param_str);
  return opt;
}

internal B32
lnk_cmd_line_has_switch(LNK_CmdLine cmd_line, LNK_CmdSwitchType cmd_switch)
{
  String8 cmd_switch_name = lnk_string_from_cmd_switch_type(cmd_switch);
  return lnk_cmd_line_has_option_string(cmd_line, cmd_switch_name);
}

internal void
lnk_error_cmd_switch(LNK_ErrorCode code, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args; va_start(args, fmt);
  String8 switch_name = lnk_string_from_cmd_switch_type(cmd_switch);
  String8 message     = push_str8fv(scratch.arena, fmt, args);
  String8 output      = push_str8f(scratch.arena, "/%S: %S", switch_name, message);
  lnk_error_obj(code, obj, "%S", output);
  va_end(args);
  scratch_end(scratch);
}

internal void
lnk_error_cmd_switch_invalid_param_count(LNK_ErrorCode code, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch)
{
  lnk_error_cmd_switch(code, obj, cmd_switch, "invalid number of parameters");
}

internal void
lnk_error_cmd_switch_invalid_param(LNK_ErrorCode code, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 param)
{
  lnk_error_cmd_switch(code, obj, cmd_switch, "invalid parameter \"%S\"", param);
}

internal String8
lnk_error_check_and_strip_quotes(LNK_ErrorCode error_code, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 string)
{
  String8 result = string;

  B32 starts_with_quote = str8_match(str8_substr(string, rng_1u64(0,1)), str8_lit("\""), 0);
  B32 ends_with_quote   = 0;
  if (string.size > 2) {
    ends_with_quote = str8_match(str8_substr(string, rng_1u64(string.size-1,string.size)), str8_lit("\""), 0);
  }

  if (starts_with_quote && ends_with_quote) {
    result = str8_skip(result, 1);
    result = str8_chop(result, 1);
  } else if (starts_with_quote && !ends_with_quote) {
    lnk_error_cmd_switch(error_code, obj, cmd_switch, "detected unmatched \" in \"%S\"", string);
  }

  return result;
}

internal void
lnk_error_invalid_uac_level_param(LNK_ErrorCode error_code, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 input)
{
  lnk_error_cmd_switch(error_code, obj, cmd_switch, "invalid param format, expected \"level={'asInvoker'|'highestAvailable'|'requireAdministrator'}\" but got \"%S\"", input);
}

internal void
lnk_error_invalid_uac_ui_access_param(LNK_ErrorCode error_code, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 input)
{
  lnk_error_cmd_switch(error_code, obj, cmd_switch, "invalid param format, expected \"uiAccess={'true'|'false'}\" but got \"%S\"", input);
}

internal B32
lnk_cmd_switch_parse_version(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, Version *ver_out)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_parsed = 0;

  if (value_strings.node_count == 1) {
    String8List split_list = str8_split_by_string_chars(scratch.arena, value_strings.first->string, str8_lit("."), StringSplitFlag_KeepEmpties);

    String8 maj_str = str8_lit("0");
    String8 min_str = str8_lit("0");
    if (split_list.node_count == 1) {
      maj_str = split_list.first->string;
    } else if (split_list.node_count == 2) {
      maj_str = split_list.first->string;
      min_str = split_list.last->string;
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid version format, too many dots, expected format: {N[.N]}");
      goto exit;
    }

    U64 maj, min;
    if (try_u64_from_str8_c_rules(maj_str, &maj)) {
      if (try_u64_from_str8_c_rules(min_str, &min)) {
        *ver_out = make_version(maj, min);
        is_parsed = 1;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse minor version");
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse major version");
    }
  } else {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
  }

exit:;
  scratch_end(scratch);
  return is_parsed;
}

internal B32
lnk_cmd_switch_parse_tuple(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, Rng1U64 *tuple_out)
{
  if (value_strings.node_count == 1) {
    U64 value;
    if (try_u64_from_str8_c_rules(value_strings.first->string, &value)) {
      tuple_out->v[0] = value;
      return 1;
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse the parameter \"%S\"", value_strings.first->string);
    }
  } else if (value_strings.node_count == 2) {
    U64 a,b;
    if (try_u64_from_str8_c_rules(value_strings.first->string, &a)) {
      if (try_u64_from_str8_c_rules(value_strings.last->string, &b)) {
        tuple_out->v[0] = a;
        tuple_out->v[1] = b;
        return 1;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable ot parse second parameter \"%S\"", value_strings.last->string);
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse first parameter \"%S\"", value_strings.first->string);
    }
  } else {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
  }
  return 0;
}

internal B32
lnk_try_parse_u64(String8 string, LNK_ParseU64Flags flags, U64 *value_out)
{
  if (try_u64_from_str8_c_rules(string, value_out)) {
    if (flags & LNK_ParseU64Flag_CheckUnder32bit) {
      if (*value_out > max_U32) {
        return 0;
      }
    }
    if (flags & LNK_ParseU64Flag_CheckPow2) {
      if (!IsPow2(*value_out)) {
        return 0;
      }
    }
  }

  return 1;
}

internal B32
lnk_cmd_switch_parse_u64(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, U64 *value_out, LNK_ParseU64Flags flags)
{
  if (value_strings.node_count != 1) {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters, exepcted integer number as input");
    return 0;
  }
  if (!lnk_try_parse_u64(value_strings.first->string, flags, value_out)) {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse string \"%S\"", value_strings.first->string);
    return 0;
  }
  return 1;
}

internal B32
lnk_cmd_switch_parse_u32(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, U32 *value_out, LNK_ParseU64Flags flags)
{
  U64 value;
  if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &value, flags | LNK_ParseU64Flag_CheckUnder32bit)) {
    *value_out = (U32)value;
    return 1;
  }
  return 0;
}

internal B32
lnk_cmd_switch_parse_u64_list(Arena *arena, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, U64List *list_out, LNK_ParseU64Flags flags)
{
  for (String8Node *string_n = value_strings.first; string_n != 0; string_n = string_n->next) {
    U64 value;
    if (!lnk_try_parse_u64(string_n->string, flags, &value)) {
      return 0;
    }
    u64_list_push(arena, list_out, value);
  }
  return 1;
}

internal B32
lnk_cmd_switch_parse_flag(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, LNK_SwitchState *value_out)
{
  B32 is_parsed = 0;
  if (value_strings.node_count > 1) {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "too many parameters");
  } else if (value_strings.node_count == 1) {
    if (str8_match_lit("no", value_strings.first->string, StringMatchFlag_CaseInsensitive)) {
      *value_out = LNK_SwitchState_No;
      is_parsed = 1;
    } else if (str8_match_lit("yes", value_strings.first->string, StringMatchFlag_CaseInsensitive)) {
      *value_out = LNK_SwitchState_Yes;
      is_parsed = 1;
    } else if (value_strings.first->string.size == 0) {
      *value_out = 1;
      is_parsed = 1;
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid parameter \"%S\"", value_strings.first->string);
    }
  } else {
    *value_out = LNK_SwitchState_Yes;
    is_parsed = 1;
  }
  return is_parsed;
}

internal void
lnk_cmd_switch_set_flag_inv_16(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, U16 *flags, U16 bits)
{
  LNK_SwitchState state;
  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value_strings, &state)) {
    switch (state) {
    case LNK_SwitchState_Null: break;
    case LNK_SwitchState_Yes : *flags |= bits;  break;
    case LNK_SwitchState_No  : *flags &= ~bits; break;
    }
  }
}

internal void
lnk_cmd_switch_set_flag_inv_64(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, U64 *flags, U64 bits)
{
  LNK_SwitchState state;
  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value_strings, &state)) {
    switch (state) {
    case LNK_SwitchState_Null: break;
    case LNK_SwitchState_Yes : *flags |= bits;  break;
    case LNK_SwitchState_No  : *flags &= ~bits; break;
    }
  }
}

internal void
lnk_cmd_switch_set_flag_16(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, U16 *flags, U16 bits)
{
  LNK_SwitchState state;
  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value_strings, &state)) {
    switch (state) {
    case LNK_SwitchState_Null: break;
    case LNK_SwitchState_Yes : *flags |= bits;  break;
    case LNK_SwitchState_No  : *flags &= ~bits; break;
    }
  }
}

internal void
lnk_cmd_switch_set_flag_32(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, U32 *flags, U32 bits)
{
  LNK_SwitchState state;
  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value_strings, &state)) {
    switch (state) {
    case LNK_SwitchState_Null: break;
    case LNK_SwitchState_Yes : *flags |= bits;  break;
    case LNK_SwitchState_No  : *flags &= ~bits; break;
    }
  }
}

internal void
lnk_cmd_switch_set_flag_64(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, U64 *flags, U64 bits)
{
  LNK_SwitchState state;
  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value_strings, &state)) {
    switch (state) {
    case LNK_SwitchState_Null: break;
    case LNK_SwitchState_Yes : *flags |= bits;  break;
    case LNK_SwitchState_No  : *flags &= ~bits; break;
    }
  }
}

internal B32
lnk_cmd_switch_parse_string(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, String8 *string_out)
{
  if (value_strings.node_count == 1) {
    if (value_strings.first->string.size > 0) {
      *string_out = value_strings.first->string;
      return 1;
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "empty string is not permitted");
    }
  } else {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
  }
  return 0;
}

internal void
lnk_cmd_switch_parse_string_copy(Arena *arena, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List value_strings, String8 *string_out)
{
  if (lnk_cmd_switch_parse_string(obj, cmd_switch, value_strings, string_out)) {
    *string_out = push_str8_copy(arena, *string_out);
  }
}

internal B32
lnk_parse_alt_name_directive(String8 string, LNK_Obj *obj, LNK_AltName *alt_out)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_parse_ok = 0;
  String8List pair = str8_split_by_string_chars(scratch.arena, string, str8_lit("="), 0);
  if (pair.node_count == 2) {
    alt_out->from = pair.first->string;
    alt_out->to   = pair.last->string;
    alt_out->obj  = obj;
    is_parse_ok = 1;
  } else {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, LNK_CmdSwitch_AlternateName, "syntax error in \"%S\", expected format \"FROM=TO\"", string);
  }
  scratch_end(scratch);
  return is_parse_ok;
}

internal B32
lnk_parse_export_directive_ex(Arena *arena, String8List directive, LNK_Obj *obj, PE_ExportParse *export_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  B32 is_parsed = 0;

  // parse "alias=name"
  String8     name  = {0};
  String8     alias = {0};
  String8List flags = {0};
  {
    String8List alias_name_split = str8_split_by_string_chars(scratch.arena, directive.first->string, str8_lit("="), 0);
    if (alias_name_split.node_count == 2) {
      alias = alias_name_split.first->string;
      name  = alias_name_split.last->string;
    } else if (alias_name_split.node_count == 1) {
      name = alias_name_split.first->string;
    } else {
      String8 d = str8_list_join(scratch.arena, &directive, &(StringJoin){.sep=str8_lit(",")});
      lnk_error_obj(LNK_Error_IllExport, obj, "invalid export directive \"/EXPORT:%S\"", d);
      goto exit;
    }

    flags = directive;
    str8_list_pop_front(&flags);
  }

  // discard alias to itself
  if (str8_match(name, alias, 0)) {
    alias = str8_zero();
  }

  // does directive have ordinal?
  COFF_ImportByType import_by = COFF_ImportBy_Name;
  U16 ordinal16 = 0;
  String8 ordinal = {0};
  String8 noname_flag = {0};
  if (str8_match(str8_prefix(str8_list_first(&flags), 1), str8_lit("@"), 0)) {
    // parse ordinal
    ordinal = str8_skip(str8_list_pop_front(&flags)->string, 1);
    if (str8_is_integer(ordinal, 10)) {
      U64 ordinal64 = u64_from_str8(ordinal, 10);
      if (ordinal64 <= max_U16) {
        ordinal16 = (U16)ordinal64;
        import_by = COFF_ImportBy_Ordinal;
      } else {
        String8 d = str8_list_join(scratch.arena, &directive, &(StringJoin){.sep=str8_lit(",")});
        lnk_error_obj(LNK_Error_IllExport, obj, "ordinal value must fit into 16-bit integer, \"/EXPORT:%S\"", d);
        goto exit;
      }
    } else {
      String8 d = str8_list_join(scratch.arena, &directive, &(StringJoin){.sep=str8_lit(",")});
      lnk_error_obj(LNK_Error_IllExport, obj, "invalid export directive \"/EXPORT:%S\"", d);
      goto exit;
    }

    // detect NONAME flag
    if (str8_match(str8_list_first(&flags), str8_lit("NONAME"), StringMatchFlag_CaseInsensitive)) {
      noname_flag = str8_list_pop_front(&flags)->string;
    }
  }

  // detect PRIVATE flag
  String8 private_flag = {0};
  if (str8_match(str8_list_first(&flags), str8_lit("PRIVATE"), StringMatchFlag_CaseInsensitive)) {
    private_flag = str8_list_pop_front(&flags)->string;
  }

  // parse export type
  COFF_ImportType type = COFF_ImportHeader_Code;
  if (flags.node_count) {
    type = coff_import_header_type_from_string(str8_list_pop_front(&flags)->string);
    if (type == COFF_ImportType_Invalid) {
      String8 d = str8_list_join(scratch.arena, &directive, &(StringJoin){.sep=str8_lit(",")});
      lnk_error_obj(LNK_Error_IllExport, obj, "invalid export directive \"/EXPORT:%S\"", d);
      goto exit;
    }
  }

  // are there leftover nodes?
  if (flags.node_count != 0) {
    String8 d = str8_list_join(scratch.arena, &directive, &(StringJoin){.sep=str8_lit(",")});
    lnk_error_obj(LNK_Error_IllExport, obj, "invalid export directive \"/EXPORT:%S\"", d);
    goto exit;
  }

  // fill out export
  export_out->obj_path            = obj ? obj->path : str8_zero();
  export_out->lib_path            = lnk_obj_get_lib_path(obj);
  export_out->name                = push_str8_copy(arena, name);
  export_out->alias               = push_str8_copy(arena, alias);
  export_out->type                = type;
  export_out->import_by           = import_by;
  export_out->ordinal             = ordinal16;
  export_out->is_ordinal_assigned = ordinal.size > 0;
  export_out->is_noname_present   = noname_flag.size > 0;
  export_out->is_private          = private_flag.size > 0;
  export_out->is_forwarder        = str8_find_needle(name, 0, str8_lit("."), 0) < name.size;

  is_parsed = 1;
  
exit:;
  scratch_end(scratch);
  ProfEnd();
  return is_parsed;
}

internal B32
lnk_parse_export_directive(Arena *arena, String8 directive, LNK_Obj *obj, PE_ExportParse *export_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List split_directive = str8_split_by_string_chars(scratch.arena, directive, str8_lit(","), 0);
  B32 is_parsed = lnk_parse_export_directive_ex(arena, split_directive, obj, export_out);
  scratch_end(scratch);
  return is_parsed;
}

internal B32
lnk_parse_merge_directive(String8 string, LNK_Obj *obj, LNK_MergeDirective *out)
{
  Temp scratch = scratch_begin(0, 0);
  B32 is_parse_ok = 0;
  String8List list = str8_split_by_string_chars(scratch.arena, string, str8_lit("="), 0);
  if (list.node_count == 2) {
    out->src = list.first->string;
    out->dst = list.last->string;
    is_parse_ok = 1;
  } else {
    lnk_error_cmd_switch(LNK_Warning_InvalidMergeDirectiveFormat, obj, LNK_CmdSwitch_Merge, "unable to parse merge directive, expected format \"/MERGE:FROM=TO\" but got \"%S\"", string);
  }
  scratch_end(scratch);
  return is_parse_ok;
}

internal LNK_AltNameNode *
lnk_alt_name_list_push(Arena *arena, LNK_AltNameList *list, LNK_AltName v)
{
  LNK_AltNameNode *node = push_array(arena, LNK_AltNameNode, 1);
  node->v = v;
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
  return node;
}

internal LNK_MergeDirectiveNode *
lnk_merge_directive_list_push(Arena *arena, LNK_MergeDirectiveList *list, LNK_MergeDirective v)
{
  LNK_MergeDirectiveNode *node = push_array_no_zero(arena, LNK_MergeDirectiveNode, 1);
  node->v = v;
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
  return node;
}

internal String8
lnk_get_image_name(LNK_Config *config)
{
  String8 image_name = config->image_name;
  image_name = str8_skip_last_slash(image_name);
  image_name = str8_chop_last_dot(image_name);
  return image_name;
}

internal U64
lnk_get_default_function_pad_min(COFF_MachineType machine)
{
  U64 function_pad_min = 0;
  switch (machine) {
    case COFF_MachineType_Unknown: break;
    case COFF_MachineType_X86: {
      function_pad_min = 5;
    } break;
    case COFF_MachineType_X64: {
      function_pad_min = 6;
    } break;
    default: {
      lnk_error_cmd_switch(LNK_Error_Cmdl, 0, LNK_CmdSwitch_FunctionPadMin, "default paramter is not defined for: %S", coff_string_from_machine_type(machine));
    } break;
  }
  return function_pad_min;
}

internal U64
lnk_get_base_addr(LNK_Config *config)
{
  U64 base_addr = config->user_base_addr;
  if (base_addr == 0) {
    if (config->file_characteristics & PE_ImageFileCharacteristic_FILE_DLL) {
      base_addr = coff_default_dll_base_from_machine(config->machine);
    } else if (config->file_characteristics & PE_ImageFileCharacteristic_EXE) {
      if ((~config->file_characteristics & PE_ImageFileCharacteristic_LARGE_ADDRESS_AWARE) && config->machine == COFF_MachineType_X64) {
        base_addr = coff_default_exe_base_from_machine(COFF_MachineType_X86);
      } else {
        base_addr = coff_default_exe_base_from_machine(config->machine);
      }
    } else {
      lnk_error(LNK_Error_Cmdl, "image type is not specified.");
    }
  }
  return base_addr;
}

internal Version
lnk_get_default_subsystem_version(PE_WindowsSubsystem subsystem, COFF_MachineType machine)
{
  Version ver = make_version(0,0);
  switch (subsystem) {
  case PE_WindowsSubsystem_WINDOWS_BOOT_APPLICATION: ver = make_version(1,0); break;

  case PE_WindowsSubsystem_WINDOWS_CUI: {
    switch (machine) {
    case COFF_MachineType_X64: 
    case COFF_MachineType_X86: ver = make_version(6,0); break;

    case COFF_MachineType_ArmNt:
    case COFF_MachineType_Arm64:
    case COFF_MachineType_Arm: ver = make_version(6,2); break;

    default: lnk_not_implemented("define subsystem(%S) version for %S", pe_string_from_subsystem(subsystem), coff_string_from_machine_type(machine)); break;
    }
  } break;

  case PE_WindowsSubsystem_WINDOWS_GUI: {
    switch (machine) {
    case COFF_MachineType_X64:
    case COFF_MachineType_X86: ver = make_version(6,0); break;

    case COFF_MachineType_ArmNt:
    case COFF_MachineType_Arm64:
    case COFF_MachineType_Arm: ver = make_version(6,2); break;

    default: lnk_not_implemented("define subsystem(%S) version for %S", pe_string_from_subsystem(subsystem), coff_string_from_machine_type(machine)); break;
    }
  } break;

  case PE_WindowsSubsystem_POSIX_CUI: ver = make_version(19,90); break;

  case PE_WindowsSubsystem_EFI_APPLICATION: 
  case PE_WindowsSubsystem_EFI_BOOT_SERVICE_DRIVER:
  case PE_WindowsSubsystem_EFI_ROM: 
  case PE_WindowsSubsystem_EFI_RUNTIME_DRIVER: ver = make_version(1,0); break;

  case PE_WindowsSubsystem_NATIVE_WINDOWS:
  case PE_WindowsSubsystem_NATIVE: lnk_not_implemented("detect -drive=WDM switch"); break;

  default: lnk_not_implemented("unknown subsystem kind %u", subsystem); break;
  }
  return ver;
}

internal Version
lnk_get_min_subsystem_version(PE_WindowsSubsystem subsystem, COFF_MachineType machine)
{
  Version ver = make_version(0,0);
  switch (subsystem) {
  case PE_WindowsSubsystem_WINDOWS_BOOT_APPLICATION: ver = make_version(1,0); break;

  case PE_WindowsSubsystem_WINDOWS_CUI: {
    switch (machine) {
    case COFF_MachineType_X86: ver = make_version(5,1); break;

    case COFF_MachineType_X64: ver = make_version(5,2); break;

    case COFF_MachineType_ArmNt:
    case COFF_MachineType_Arm64:
    case COFF_MachineType_Arm: ver = make_version(6,2); break;

    default: lnk_not_implemented("define min subsystem(%S) version for %S", pe_string_from_subsystem(subsystem), coff_string_from_machine_type(machine)); break;
    }
  } break;

  case PE_WindowsSubsystem_WINDOWS_GUI: {
    switch (machine) {
    case COFF_MachineType_X86: ver = make_version(5,1); break;

    case COFF_MachineType_X64: ver = make_version(5,2); break;

    case COFF_MachineType_ArmNt:
    case COFF_MachineType_Arm64:
    case COFF_MachineType_Arm: ver = make_version(6,2); break;

    default: lnk_not_implemented("define min subsystem(%S) version for %S", pe_string_from_subsystem(subsystem), coff_string_from_machine_type(machine)); break;
    }
  } break;

  case PE_WindowsSubsystem_POSIX_CUI: ver = make_version(1,0); break;

  case PE_WindowsSubsystem_EFI_APPLICATION: 
  case PE_WindowsSubsystem_EFI_BOOT_SERVICE_DRIVER:
  case PE_WindowsSubsystem_EFI_ROM: 
  case PE_WindowsSubsystem_EFI_RUNTIME_DRIVER: ver = make_version(1,0); break;

  case PE_WindowsSubsystem_NATIVE_WINDOWS:
  case PE_WindowsSubsystem_NATIVE: lnk_not_implemented("detect -drive=WDM switch"); break;
  
  default: lnk_not_implemented("unknown subsystem kind %u", subsystem);
  }
  return ver;
}

internal B32
lnk_do_debug_info(LNK_Config *config)
{
  B32 do_debug_info = config->rad_debug == LNK_SwitchState_Yes ||
    (config->debug_mode != LNK_DebugMode_None && config->debug_mode != LNK_DebugMode_Null);
  return do_debug_info;
}

internal B32
lnk_is_thread_pool_shared(LNK_Config *config)
{
  return config->shared_thread_pool_name.size > 0;
}

internal B32
lnk_is_section_removed(LNK_Config *config, String8 section_name)
{
  B32 is_removed = 0;
  for (String8Node *name_n = config->remove_sections.first; name_n != 0 && !is_removed; name_n = name_n->next) {
    is_removed = str8_match(section_name, name_n->string, 0);
  }
  return is_removed;
}

internal B32
lnk_is_dll_delay_load(LNK_Config *config, String8 dll_name)
{
  return hash_table_search_path_u64(config->delay_load_ht, dll_name, 0);
}

internal String8
lnk_get_lib_name(String8 path)
{
  static String8 LIB_EXT = str8_lit_comp(".LIB");
  
  // strip path
  String8 name = str8_skip_last_slash(path);
  
  // strip extension
  String8 name_ext = str8_postfix(name, LIB_EXT.size);
  if (str8_match(name_ext, LIB_EXT, StringMatchFlag_CaseInsensitive)) {
    name = str8_chop(name, LIB_EXT.size);
  }
  
  return name;
}

internal void
lnk_push_disallow_lib(LNK_Config *config, String8 path)
{
  String8 lib_name = lnk_get_lib_name(path);
  hash_table_push_path_u64(config->arena, config->disallow_lib_ht, lib_name, 0);
}

internal B32
lnk_is_lib_disallowed(LNK_Config *config, String8 path)
{
  String8 lib_name = lnk_get_lib_name(path);
  return hash_table_search_path(config->disallow_lib_ht, lib_name) != 0;
}

internal void
lnk_include_symbol(LNK_Config *config, String8 name, LNK_Obj *obj)
{
  // is this a duplicate symbol?
  if (hash_table_search_string_raw(config->include_symbol_ht, name)) {
    return;
  }

  name = push_str8_copy(config->arena, name);

  LNK_IncludeSymbolNode *node = push_array(config->arena, LNK_IncludeSymbolNode, 1);
  node->v.name = name;
  node->v.obj  = obj;

  SLLQueuePush(config->include_symbol_list.first, config->include_symbol_list.last, node);
  config->include_symbol_list.count += 1;

  hash_table_push_string_raw(config->arena, config->include_symbol_ht, name, node);
}

internal void
lnk_print_build_info()
{
  fprintf(stdout, "  Compiler: %s\n", COMPILER_STRING);
  fprintf(stdout, "  Mode    : %s\n", BUILD_MODE_STRING);
  fprintf(stdout, "  Date    : %s %s\n", __TIME__, __DATE__);
  fprintf(stdout, "  Version : %s\n", BUILD_VERSION_STRING_LITERAL);
}

internal void
lnk_print_help(void)
{
  Temp scratch = scratch_begin(0,0);

  fprintf(stdout, "--- Help -------------------------------------------------------\n");
  fprintf(stdout, "  %s\n", BUILD_TITLE_STRING_LITERAL);
  fprintf(stdout, "\n");
  fprintf(stdout, "  Usage: radlink.exe [Options] [Files] [@rsp]\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "  Options:\n");
  for (U64 i = 0; i < ArrayCount(g_cmd_switch_map); ++i) {
    Temp temp = temp_begin(scratch.arena);

    char *name = g_cmd_switch_map[i].name;
    char *args = g_cmd_switch_map[i].args;
    char *desc = g_cmd_switch_map[i].desc;
    LNK_CmdSwitchType type = g_cmd_switch_map[i].type;

    if (strcmp(name, "") == 0 ||
        strcmp(name, "NOT_IMPLEMENTED") == 0 ||
        type == LNK_CmdSwitch_Help) {
      continue;
    }

    String8 name_args = push_str8f(temp.arena, "%s%s", name, args);

    fprintf(stdout, "   /%-32.*s %s%s\n",
            str8_varg(name_args),
            desc,
            type == LNK_CmdSwitch_NotImplemented ? "Not Implemented" : "");

    temp_end(temp);
  }

  fprintf(stdout, "\n");

  scratch_end(scratch);
}

internal String8
lnk_expand_env_vars_windows(Arena *arena, HashTable *env_vars, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List list = {0};
  for (U64 i = 0; i < string.size; ) {
    U64 open  = str8_find_needle(string, i,      str8_lit("%"), 0);
    U64 close = str8_find_needle(string, open+1, str8_lit("%"), 0);

    String8 text = str8_substr(string, rng_1u64(i, open));
    str8_list_push(scratch.arena, &list, text);
    i += text.size;

    if (open < close) {
      String8       env_var_name = str8_substr(string, rng_1u64(open+1, close));
      KeyValuePair *match        = hash_table_search_path(env_vars, env_var_name);
      if (match) {
        str8_list_push(scratch.arena, &list, match->value_string);
        i = close+1;
      } else {
        str8_list_pushf(scratch.arena, &list, "%%%S", env_var_name);
        i = close;
      }
    }
  }

  String8 result = str8_list_join(arena, &list, 0);

  scratch_end(scratch);
  return result;
}

internal String8List
lnk_unwrap_rsp(Arena *arena, String8List arg_list)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List result = {0};

  for (String8Node *curr = arg_list.first; curr != 0; curr = curr->next) {
    B32 is_rsp = str8_match_lit("@", curr->string, StringMatchFlag_RightSideSloppy);
    if (is_rsp) {
      // remove "@"
      String8 name = str8_skip(curr->string, 1);

      if (os_file_path_exists(name)) {
        // read rsp from disk
        String8 file = lnk_read_data_from_file_path(scratch.arena, 0, name);
        
        // parse rsp
        String8List rsp_args = lnk_arg_list_parse_windows_rules(scratch.arena, file);
        
        // handle case where rsp references another rsp
        String8List list = lnk_unwrap_rsp(arena, rsp_args);

        // push arguments from rsp
        list = str8_list_copy(arena, &list);
        str8_list_concat_in_place(&result, &list);
       } else {
        lnk_error(LNK_Error_Cmdl, "unable to find rsp: %S", name);
      }
    } else {
      // push regular argument
      String8 str = push_str8_copy(arena, curr->string);
      str8_list_push(arena, &result, str);
    }
  }
  
  scratch_end(scratch);
  return result;
}

internal void
lnk_apply_cmd_option_to_config(LNK_Config *config, String8 cmd_name, String8List value_strings, LNK_Obj *obj)
{
  Temp scratch = scratch_begin(&config->arena, 1);

  LNK_CmdSwitchType cmd_switch = lnk_cmd_switch_type_from_string(cmd_name);

  switch (cmd_switch) {
  case LNK_CmdSwitch_Null: {
    String8 value = str8_list_join(scratch.arena, &value_strings, &(StringJoin){.sep=str8_lit_comp(",")});
    lnk_error_obj(LNK_Warning_UnknownSwitch, obj, "unknown switch: \"/%S%s%S\"", cmd_name, value.size ? ":" : "", value);
  } break;

  default: break;

  case LNK_CmdSwitch_NotImplemented: {
    String8 value = str8_list_join(scratch.arena, &value_strings, &(StringJoin){.sep=str8_lit_comp(",")});
    lnk_not_implemented("switch \"%S\" is not implemented \"%S\"", cmd_name, value);
  } break;

  case LNK_CmdSwitch_Align: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &config->sect_align, LNK_ParseU64Flag_CheckPow2);
  } break;

  case LNK_CmdSwitch_AllowBind: {
    lnk_cmd_switch_set_flag_inv_16(obj, cmd_switch, value_strings, &config->dll_characteristics, PE_DllCharacteristic_NO_BIND);
  } break;

  case LNK_CmdSwitch_AllowIsolation: {
    lnk_cmd_switch_set_flag_inv_16(obj, cmd_switch, value_strings, &config->dll_characteristics, PE_DllCharacteristic_NO_ISOLATION);
  } break;

  case LNK_CmdSwitch_AlternateName: {
    if (value_strings.node_count == 1) {
      LNK_AltName alt_name;
      if (lnk_parse_alt_name_directive(value_strings.first->string, obj, &alt_name)) {
        String8 to_extant = {0};
        if (hash_table_search_string_string(config->alt_name_ht, alt_name.from, &to_extant)) {
          if (str8_match(to_extant, alt_name.to, 0)) {
            // ignore, duplicate
          } else {
            lnk_error_obj(LNK_Error_AlternateNameConflict, obj, "conflicting alternative name: existing '%S=%S' vs. new '%S=%S'", alt_name.from, to_extant, alt_name.from, alt_name.to);
          }
        } else {
          alt_name.from = push_str8_copy(config->arena, alt_name.from);
          alt_name.to   = push_str8_copy(config->arena, alt_name.to);

          lnk_alt_name_list_push(config->arena, &config->alt_name_list, alt_name);
          hash_table_push_string_string(config->arena, config->alt_name_ht, alt_name.from, alt_name.to);
        }
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
    }
  } break;

  case LNK_CmdSwitch_AppContainer: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value_strings, &config->dll_characteristics, PE_DllCharacteristic_APPCONTAINER);
  } break;

  case LNK_CmdSwitch_Base: {
    if (value_strings.node_count == 2) {
      String8Node *first_node = value_strings.first;
      //String8Node *second_node = first_node->next;
      B32 is_response_file = str8_match_lit("@", first_node->string, StringMatchFlag_RightSideSloppy);
      if (is_response_file) {
        //String8 file_path = first_node->string;
        //String8 tag = second_node->string;
        lnk_not_implemented("Response files are not implemented for /BASE");
      } else {
        Rng1U64 addr_size = {0};
        if (lnk_cmd_switch_parse_tuple(obj, cmd_switch, value_strings, &addr_size)) {
          config->user_base_addr = addr_size.v[0];
          config->max_image_size = addr_size.v[1];
        }
      }
    } else if (value_strings.node_count == 1) {
      U64 addr;
      if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &addr, 0)) {
        config->user_base_addr = addr;
      }
    } else if (value_strings.node_count == 0) {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "expected at least 1 parameter");
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "too many parameters");
    }
  } break;

  case LNK_CmdSwitch_Debug: {
    if (value_strings.node_count == 0) {
      config->debug_mode = LNK_DebugMode_Full;
    } else if (value_strings.node_count == 1) {
      LNK_DebugMode debug_mode = lnk_debug_mode_from_string(value_strings.first->string);
      if (debug_mode == LNK_DebugMode_GHash) {
        config->debug_mode = LNK_DebugMode_Full;
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "GHASH is not supported, switching to FULL");
      } else if (debug_mode == LNK_DebugMode_FastLink) {
        config->debug_mode = LNK_DebugMode_Full;
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "FASTLINK is not supported, switching to FULL");
      } else if (debug_mode != LNK_DebugMode_Null) {
        config->debug_mode = debug_mode;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid parameter \"%S\"", value_strings.first->string);
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
    }
  } break;

  case LNK_CmdSwitch_DefaultLib: {
    String8List default_lib_list = str8_list_copy(config->arena, &value_strings);
    if (obj) {
      str8_list_concat_in_place(&config->input_obj_lib_list, &default_lib_list);
    } else {
      str8_list_concat_in_place(&config->input_default_lib_list, &default_lib_list);
    }
  } break;

  case LNK_CmdSwitch_Delay: {
    if (value_strings.node_count == 0 || value_strings.node_count > 1) {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
    } else {
      String8 value = value_strings.first->string;
      if (str8_match_lit("unload", value, StringMatchFlag_CaseInsensitive)) {
        config->import_table_emit_uiat = 1;
      } else if (str8_match_lit("nobind", value, StringMatchFlag_CaseInsensitive)) {
        config->import_table_emit_biat = 0;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown parameter \"%S\"", value);
      }
    }
  } break;

  case LNK_CmdSwitch_DelayLoad: {
    for (String8Node *name_n = value_strings.first; name_n != 0; name_n = name_n->next) {
      if (hash_table_search_path_u64(config->delay_load_ht, name_n->string, 0)) { continue; }
      String8 name = push_str8_copy(config->arena, name_n->string);
      hash_table_push_path_u64(config->arena, config->delay_load_ht, name, 0);
      str8_list_push(config->arena, &config->delay_load_dll_list, name);
    }
  } break;

  case LNK_CmdSwitch_Dll: {
    config->file_characteristics |= PE_ImageFileCharacteristic_FILE_DLL;
  } break;

  case LNK_CmdSwitch_DynamicBase: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value_strings, &config->dll_characteristics, PE_DllCharacteristic_DYNAMIC_BASE);
  } break;

  case LNK_CmdSwitch_Dump: { 
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unsupported switch; binary dump is done by passing /DUMP to link.exe");
  } break;

  case LNK_CmdSwitch_Entry: {
    String8 new_entry_point_name = {0};
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &new_entry_point_name);

    if (config->entry_point_name.size) {
      lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "unable to redefine entry point \"%S\" to \"%S\"", config->entry_point_name, new_entry_point_name);
      break;
    }

    config->entry_point_name = new_entry_point_name;
  } break;

  case LNK_CmdSwitch_Export: {
    PE_ExportParse export_parse = {0};
    if (lnk_parse_export_directive_ex(config->arena, value_strings, obj, &export_parse)) {
      String8             export_name = pe_name_from_export_parse(&export_parse);
      PE_ExportParseNode *exp_n       = hash_table_search_string_raw(config->export_ht, export_name);

      if (exp_n == 0) {
        // make sure export is defined
        if (!export_parse.is_forwarder) {
          lnk_include_symbol(config, export_parse.name, 0);
        }

        // push new export
        exp_n = pe_export_parse_list_push(config->arena, &config->export_symbol_list, export_parse);

        hash_table_push_string_raw(config->arena, config->export_ht, export_name, exp_n);
      } else {
        B32 is_ambiguous = 1;
        PE_ExportParse *extant_export = &exp_n->data;

        if (extant_export->alias.size && export_parse.alias.size && !str8_match(extant_export->alias, export_parse.alias, 0)) {
          goto report;
        }

        if (extant_export->ordinal != export_parse.ordinal) {
          goto report;
        }

        is_ambiguous = 0;

        if (extant_export->alias.size == 0 && export_parse.alias.size != 0) {
          extant_export->alias = export_parse.alias;
        }

      report:;
       if (is_ambiguous) {
         lnk_error_obj(LNK_Error_IllExport, obj, "ambiguous symbol export %S", export_parse.name);
       }
      }
    }
  } break;

  case LNK_CmdSwitch_FastFail: {
    // do nothing
  } break;

  case LNK_CmdSwitch_FileAlign: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &config->file_align, LNK_ParseU64Flag_CheckPow2);
  } break;

  case LNK_CmdSwitch_Fixed: {
    lnk_cmd_switch_set_flag_64(obj, cmd_switch, value_strings, &config->flags, LNK_ConfigFlag_Fixed);
  } break;

  case LNK_CmdSwitch_FunctionPadMin: {
    if (value_strings.node_count == 0) {
      config->function_pad_min       = 0;
      config->infer_function_pad_min = 1;
    } else {
      lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &config->function_pad_min, LNK_ParseU64Flag_CheckUnder32bit);
    }
    config->do_function_pad_min = LNK_SwitchState_Yes;
  } break;

  case LNK_CmdSwitch_Heap: {
    Rng1U64 reserve_commit;
    reserve_commit.v[0] = config->heap_reserve;
    reserve_commit.v[1] = config->heap_commit;
    if (lnk_cmd_switch_parse_tuple(obj, cmd_switch, value_strings, &reserve_commit)) {
      if (reserve_commit.v[0] >= reserve_commit.v[1]) {
        U64 reserve_aligned = AlignPow2(reserve_commit.v[0], 4);
        U64 commit_aligned = AlignPow2(reserve_commit.v[1], 4);
#if 0
        if (reserve_aligned != reserve_commit.v[0]) {
          lnk_error_cmd_switch(LNK_WARNING_CMDL, obj, cmd_switch, "reserve is not power of two, aligned to %u bytes", reserve_aligned);
        }
        if (commit_aligned != reserve_commit.v[1]) {
          lnk_error_cmd_switch(LNK_WARNING_CMDL, obj, cmd_switch, "commit is not power of two, aligned to %u bytes", commit_aligned);
        }
#endif
        config->heap_reserve = reserve_aligned;
        config->heap_commit = commit_aligned;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "commit(%llu) is greater than reserve(%llu)", reserve_commit.v[1], reserve_commit.v[0]);
      }
    }
  } break;

  case LNK_CmdSwitch_HighEntropyVa: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value_strings, &config->dll_characteristics, PE_DllCharacteristic_HIGH_ENTROPY_VA);
  } break;

  case LNK_CmdSwitch_Ignore: {
    U64 error_code;
    if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &error_code, 0)) {
      switch (error_code) {
      case LNK_MsWarningCode_UnsuedDelayLoadDll: {
        lnk_suppress_error(LNK_Warning_UnusedDelayLoadDll);
      } break;
      case LNK_MsWarningCode_MissingExternalTypeServer: {
        lnk_suppress_error(LNK_Warning_MissingExternalTypeServer);
      } break;
      case LNK_MsWarningCode_SectionFlagsConflict: {
        lnk_suppress_error(LNK_Warning_SectionFlagsConflict);
      } break;
      default: {
        lnk_not_implemented("TODO: /IGNORE:%llu", error_code);
      } break;
      }
    }
  } break;

  case LNK_CmdSwitch_ImpLib: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &config->imp_lib_name);
  } break;

  case LNK_CmdSwitch_Include: {
    for (String8Node *value_n = value_strings.first; value_n != 0; value_n = value_n->next) {
      lnk_include_symbol(config, value_n->string, obj);
    }
  } break;

  case LNK_CmdSwitch_Incremental: {
    LNK_SwitchState state;
    if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value_strings, &state)) {
      if (state == LNK_SwitchState_Yes) {
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "incremental linkage is not supported");
      }
    }
  } break;

  case LNK_CmdSwitch_LargeAddressAware: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value_strings, &config->file_characteristics, PE_ImageFileCharacteristic_LARGE_ADDRESS_AWARE);
  } break;

  case LNK_CmdSwitch_Lib: {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unsupported switch; static library is created by passing /LIB to link.exe");
  } break;

  case LNK_CmdSwitch_LibPath: {
    String8List lib_dir_list = str8_list_copy(config->arena, &value_strings);
    for (String8Node *dir_n = lib_dir_list.first; dir_n != 0; dir_n = dir_n->next) {
      if (!os_folder_path_exists(dir_n->string)) {
        String8 full_path = os_full_path_from_path(scratch.arena, dir_n->string);
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "path doesn't exist %S", full_path);
      }
    }
    str8_list_concat_in_place(&config->lib_dir_list, &lib_dir_list);
  } break;

  case LNK_CmdSwitch_Machine: {
    if (value_strings.node_count == 1) {
      COFF_MachineType machine = coff_machine_from_string(value_strings.first->string);
      if (machine != COFF_MachineType_Unknown) {
        config->machine = machine;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown parameter \"%S\"", value_strings.first->string);
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
    }
  } break;

  case LNK_CmdSwitch_Manifest: {
    if (value_strings.node_count == 1) {
      String8List  param_list = str8_split_by_string_chars(scratch.arena, value_strings.first->string, str8_lit(","), 0);
      String8Array param_arr  = str8_array_from_list(scratch.arena, &param_list);
      if (param_arr.count > 0) {
        if (str8_match_lit("embed", param_arr.v[0], StringMatchFlag_CaseInsensitive)) {
          config->manifest_opt = LNK_ManifestOpt_Embed;
          if (param_arr.count == 1) {
            config->manifest_resource_id = 0;
          } else if (param_arr.count > 1) {
            // parse resource id
            if (str8_match_lit("id=", param_arr.v[1], StringMatchFlag_RightSideSloppy|StringMatchFlag_CaseInsensitive)) {
              String8List  res_id_list = str8_split_by_string_chars(scratch.arena, param_arr.v[1], str8_lit("="), 0);
              String8Array res_id_arr  = str8_array_from_list(scratch.arena, &res_id_list);
              if (res_id_arr.count == 2) {
                U64 resource_id;
                if (try_u64_from_str8_c_rules(res_id_arr.v[1], &resource_id)) {
                  config->manifest_resource_id = push_u64(config->arena, resource_id);
                } else {
                  lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse resource_id \"%S\"", res_id_arr.v[1]);
                }
              } else {
                lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid syntax expected form ID=# but got \"%S\"", param_arr.v[1]);
              }
            } else {
              lnk_error_cmd_switch_invalid_param(LNK_Error_Cmdl, obj, cmd_switch, param_arr.v[0]);
            }
          } else {
            lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
          }
        } else if (str8_match_lit("no", param_arr.v[0], StringMatchFlag_CaseInsensitive)) {
          config->manifest_opt = LNK_ManifestOpt_No;
        } else {
          lnk_error_cmd_switch_invalid_param(LNK_Error_Cmdl, obj, cmd_switch, param_arr.v[0]);
        }
      } else {
        lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
      }
    } else if (value_strings.node_count == 0) {
      config->manifest_opt = LNK_ManifestOpt_WriteToFile;
    } else {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
    }
  } break;

  case LNK_CmdSwitch_ManifestDependency: {
    String8List manifest_dependency_list = str8_list_copy(config->arena, &value_strings);
    str8_list_concat_in_place(&config->manifest_dependency_list, &manifest_dependency_list);

    if (config->manifest_opt == LNK_ManifestOpt_Null) {
      config->manifest_opt = LNK_ManifestOpt_WriteToFile;
    }
  } break;

  case LNK_CmdSwitch_ManifestFile: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &config->manifest_name);
  } break;

  case LNK_CmdSwitch_ManifestInput: {
    // see :manifest_input
  } break;

  case LNK_CmdSwitch_ManifestUac: {
    if (value_strings.node_count == 1) {
      String8 uac = lnk_error_check_and_strip_quotes(LNK_Error_Cmdl, obj, cmd_switch, value_strings.first->string);
      String8List  param_list = str8_split_by_string_chars(scratch.arena, uac, str8_lit(" "), 0);
      String8Array param_arr  = str8_array_from_list(scratch.arena, &param_list);
      if (param_arr.count > 0) {
        if (str8_match_lit("level=", param_arr.v[0], StringMatchFlag_RightSideSloppy|StringMatchFlag_CaseInsensitive)) {
          String8 level_param = param_arr.v[0];
          String8List level_list = str8_split_by_string_chars(scratch.arena, level_param, str8_lit("="), 0);
          if (level_list.node_count == 2) {
            if (str8_match_lit("level", level_list.first->string, StringMatchFlag_CaseInsensitive)) {
              String8 level = level_list.last->string;
              if (str8_match_lit("'asInvoker'", level, 0) ||
                  str8_match_lit("'highestAvailable'", level, 0) ||
                  str8_match_lit("'requireAdministrator'", level, 0)) {
                // manifest level was parsed!
                config->manifest_uac = 1;
                config->manifest_level = push_str8_copy(config->arena, level);
                if (param_arr.count > 1) {
                  String8 ui_access_param = param_arr.v[1];
                  String8List ui_access_list = str8_split_by_string_chars(scratch.arena, ui_access_param, str8_lit("="), 0);
                  if (ui_access_list.node_count == 2) {
                    String8 ui_access = ui_access_list.last->string;
                    if (str8_match_lit("'true'", ui_access, 0) ||
                        str8_match_lit("'false'", ui_access, 0)) {
                      // ui access was parsed!
                      config->manifest_ui_access = push_str8_copy(config->arena, ui_access);
                    } else {
                      lnk_error_invalid_uac_ui_access_param(LNK_Error_Cmdl, obj, cmd_switch, ui_access_param);
                    }
                  } else {
                    lnk_error_invalid_uac_ui_access_param(LNK_Error_Cmdl, obj, cmd_switch, ui_access_param);
                  }
                }
              } else {
                lnk_error_invalid_uac_level_param(LNK_Error_Cmdl, obj, cmd_switch, level_param);
              }
            } else {
              lnk_error_invalid_uac_level_param(LNK_Error_Cmdl, obj, cmd_switch, level_param);
            }
          } else {
            lnk_error_invalid_uac_level_param(LNK_Error_Cmdl, obj, cmd_switch, level_param);
          }
        } else if (str8_match_lit("no", param_arr.v[0], StringMatchFlag_CaseInsensitive)) {
          config->manifest_uac = 0;
        } else {
          lnk_error_cmd_switch_invalid_param(LNK_Error_Cmdl, obj, cmd_switch, param_arr.v[0]);
        }
      } else {
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "empty param string");
      }
    } else {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
    }
  } break;

  case LNK_CmdSwitch_Merge: {
    if (value_strings.node_count == 1) {
      LNK_MergeDirective merge = {0};
      if (lnk_parse_merge_directive(value_strings.first->string, obj, &merge)) {
        merge.src = push_str8_copy(config->arena, merge.src);
        merge.dst = push_str8_copy(config->arena, merge.dst);
        lnk_merge_directive_list_push(config->arena, &config->merge_list, merge);
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters %d", value_strings.node_count);
    }
  } break;

  case LNK_CmdSwitch_Natvis: {
    // warn about invalid natvis extension
    for (String8Node *node = value_strings.first; node != 0; node = node->next) {
      String8 ext = str8_skip_last_dot(node->string);
      if (!str8_match_lit("natvis", ext, StringMatchFlag_CaseInsensitive)) {
        lnk_error_cmd_switch(LNK_Warning_InvalidNatvisFileExt, obj, cmd_switch, "Visual Studio expects .natvis extension: \"%S\"", node->string);
      }
    }

    String8List natvis_list = str8_list_copy(config->arena, &value_strings);
    str8_list_concat_in_place(&config->natvis_list, &natvis_list);
  } break;

  case LNK_CmdSwitch_DisallowLib:
  case LNK_CmdSwitch_NoDefaultLib: {
    if (value_strings.node_count == 0) {
      config->no_default_libs = 1;
    } else {
      for (String8Node *lib_n = value_strings.first; lib_n != 0; lib_n = lib_n->next) {
        String8 lib_name = lnk_get_lib_name(lib_n->string);
        if (hash_table_search_path_raw(config->disallow_lib_ht, lib_name)) {
          continue;
        }
        hash_table_push_path_raw(config->arena, config->disallow_lib_ht, lib_name, 0);
      }
    }
  } break;

  case LNK_CmdSwitch_NoExp: {
    config->build_exp = 0;
  } break;

  case LNK_CmdSwitch_NoImpLib: {
    config->build_imp_lib = 0;
  } break;

  case LNK_CmdSwitch_NoLogo: {
    // we don't print logo
  } break;

  case LNK_CmdSwitch_NxCompat: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value_strings, &config->dll_characteristics, PE_DllCharacteristic_NX_COMPAT);
  } break;

  case LNK_CmdSwitch_Opt: {
    for (String8Node *n = value_strings.first; n != 0; n = n->next) {
      String8 param = n->string;
      if (str8_match_lit("ref", param, StringMatchFlag_CaseInsensitive)) {
        config->opt_ref = LNK_SwitchState_Yes; 
      } else if (str8_match_lit("noref", param, StringMatchFlag_CaseInsensitive)) {
        config->opt_ref = LNK_SwitchState_No;
      } else if (str8_match_lit("icf", param, StringMatchFlag_CaseInsensitive) ||
                 str8_match_lit("icf=", param, StringMatchFlag_CaseInsensitive | StringMatchFlag_RightSideSloppy)) {
        String8List vals = str8_split_by_string_chars(scratch.arena, param, str8_lit("="), 0);
        if (vals.node_count > 2) {
          lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "too many parameters for iteration");
          continue;
        }
        if (vals.node_count == 2) {
          B32 is_parsed = try_u64_from_str8_c_rules(vals.last->string, &config->opt_iter_count);
          if (!is_parsed) {
            lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse iterations \"%S\"", vals.last->string);
            continue;
          }
        }
        config->opt_icf = LNK_SwitchState_Yes;
      } else if (str8_match_lit("noicf", param, StringMatchFlag_CaseInsensitive)) {
        config->opt_icf = LNK_SwitchState_No;
      } else if (str8_match_lit("lbr", param, StringMatchFlag_CaseInsensitive)) {
        config->opt_lbr = LNK_SwitchState_Yes;
      } else if (str8_match_lit("nolibr", param, StringMatchFlag_CaseInsensitive)) {
        config->opt_lbr = LNK_SwitchState_No;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown option \"%S\"", param);
      }
    }
  } break;

  case LNK_CmdSwitch_Out: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &config->image_name);
  } break;

  case LNK_CmdSwitch_Pdb: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &config->pdb_name);
  } break;

  case LNK_CmdSwitch_PdbAltPath: {
    // see :PdbAltPath
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &config->pdb_alt_path);
  } break;

  case LNK_CmdSwitch_PdbPageSize: {
    U64 page_size;
    if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &page_size, LNK_ParseU64Flag_CheckPow2)) {
      if (page_size >= MSF_MIN_PAGE_SIZE) {
        if (page_size < MSF_MAX_PAGE_SIZE) {
          config->pdb_page_size = page_size;
        } else {
          lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "page size must be <= %u bytes", MSF_MAX_PAGE_SIZE);
        }
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "page size must be >= %u bytes", MSF_MIN_PAGE_SIZE);
      }
    }
  } break;

  case LNK_CmdSwitch_Release: {
    if (value_strings.node_count == 0) {
      config->flags |= LNK_ConfigFlag_WriteImageChecksum;
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
    }
  } break;

  case LNK_CmdSwitch_Stack: {
    Rng1U64 reserve_commit;
    reserve_commit.v[0] = config->stack_reserve;
    reserve_commit.v[1] = config->stack_commit;
    if (lnk_cmd_switch_parse_tuple(obj, cmd_switch, value_strings, &reserve_commit)) {
      if (reserve_commit.v[0] >= reserve_commit.v[1]) {
        U64 reserve_aligned = AlignPow2(reserve_commit.v[0], 4);
        U64 commit_aligned = AlignPow2(reserve_commit.v[1], 4);
#if 0
        if (reserve_aligned != reserve_commit.v[0]) {
          lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "reserve is not power of two, aligned to %u", reserve_aligned);
        }
        if (commit_aligned != reserve_commit.v[1]) {
          lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "commit is not power of two, aligned to %u", commit_aligned);
        }
#endif
        config->stack_reserve = reserve_aligned;
        config->stack_commit = commit_aligned;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "commit(%llu) is greater than reserve(%llu)", reserve_commit.v[1], reserve_commit.v[0]);
      }
    }
  } break;

  case LNK_CmdSwitch_SubSystem: {
    if (value_strings.node_count <= 2 && value_strings.node_count > 0) {
      // set subsystem type
      PE_WindowsSubsystem subsystem = pe_subsystem_from_string(value_strings.first->string);
      if (subsystem != PE_WindowsSubsystem_UNKNOWN) {
        if (config->subsystem != PE_WindowsSubsystem_UNKNOWN) {
          lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "overriding subystem \"%S\" with \"%S\"",
                               pe_string_from_subsystem(config->subsystem),
                               pe_string_from_subsystem(subsystem));
        }
        config->subsystem = subsystem;

        // parse version (optional)
        if (value_strings.node_count == 2) {
          str8_list_pop_front(&value_strings); // pop subsystem parameter
          lnk_cmd_switch_parse_version(obj, cmd_switch, value_strings, &config->subsystem_ver);
        }
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid subsystem \"%S\"", value_strings.first->string);
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
    }
  } break;

  case LNK_CmdSwitch_Time: {
  } break;

  case LNK_CmdSwitch_TsAware: {
    lnk_cmd_switch_set_flag_inv_64(obj, cmd_switch, value_strings, &config->flags, LNK_ConfigFlag_NoTsAware);
  } break;

  case LNK_CmdSwitch_Version: {
    lnk_cmd_switch_parse_version(obj, cmd_switch, value_strings, &config->image_ver);
  } break;

  case LNK_CmdSwitch_Rad_Age: {
    lnk_cmd_switch_parse_u32(obj, cmd_switch, value_strings, &config->age, 0);
  } break;

  case LNK_CmdSwitch_Rad_BuildInfo: {
    lnk_print_build_info();
    os_abort(0);
  } break;

  case LNK_CmdSwitch_Rad_CheckUnusedDelayLoadDll: {
    lnk_cmd_switch_set_flag_64(obj, cmd_switch, value_strings, &config->flags, LNK_ConfigFlag_CheckUnusedDelayLoadDll);
  } break;

  case LNK_CmdSwitch_Rad_Map: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &config->rad_chunk_map_name);
    config->rad_chunk_map = LNK_SwitchState_Yes;
  } break;

  case LNK_CmdSwitch_Rad_MapLinesForUnresolvedSymbols: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value_strings, &config->map_lines_for_unresolved_symbols);
  } break;

  case LNK_CmdSwitch_Rad_MemoryMapFiles: {
    lnk_cmd_switch_set_flag_32(obj, cmd_switch, value_strings, &config->io_flags, LNK_IO_Flags_MemoryMapFiles);
  } break;

  case LNK_CmdSwitch_Rad_Debug: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value_strings, &config->rad_debug);
  } break;

  case LNK_CmdSwitch_Rad_DebugName: {
    // :Rad_DebugAltPath
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &config->rad_debug_name);
  } break;

  case LNK_CmdSwitch_Rad_DebugAltPath: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &config->rad_debug_alt_path);
  } break;

  case LNK_CmdSwitch_Rad_DelayBind: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value_strings, &config->import_table_emit_biat);
  } break;

  case LNK_CmdSwitch_Rad_DoMerge: {
    lnk_cmd_switch_set_flag_64(obj, cmd_switch, value_strings, &config->flags, LNK_ConfigFlag_Merge);
  } break;

  case LNK_CmdSwitch_Rad_EnvLib: {
    lnk_cmd_switch_set_flag_64(obj, cmd_switch, value_strings, &config->flags, LNK_ConfigFlag_EnvLib);
  } break;

  case LNK_CmdSwitch_Rad_Exe: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value_strings, &config->file_characteristics, PE_ImageFileCharacteristic_EXE);
  } break;

  case LNK_CmdSwitch_Rad_Guid: {
    if (value_strings.node_count == 1) {
      if (str8_match_lit("imageblake3", value_strings.first->string, StringMatchFlag_CaseInsensitive)) {
        config->guid_type = Lnk_DebugInfoGuid_ImageBlake3;
      } else if (str8_match_lit("random", value_strings.first->string, StringMatchFlag_CaseInsensitive)) {
        config->guid = os_make_guid();
      } else {
        Guid guid;
        if (try_guid_from_string(value_strings.first->string, &guid)) {
          config->guid = guid;
        } else {
          lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse \"%S\"", value_strings.first->string);
        }
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters, expected GUID formatted as following: \"0000000-0000-0000-0000-000000000000\"");
    }
  } break;

  case LNK_CmdSwitch_Rad_LargePages: {
    if (value_strings.node_count == 0) {
      OS_ProcessInfo *process_info = os_get_process_info();
      if (process_info->large_pages_allowed) {
        arena_default_flags |= ArenaFlag_LargePages;
      } else {
        lnk_error_cmd_switch(LNK_Warning_LargePages, obj, cmd_switch, "Large pages aren't enabled on this system.");
#if OS_WINDOWS
        lnk_supplement_error("To enable large pages:");
        lnk_supplement_error("\t- Press Win+R and open \"gpedit.msc\"");
        lnk_supplement_error("\t- Navigate to Local Computer Policy > Computer Configuration > Windows Settings > Security Settings > Local Policies > User Rights And Assignments");
        lnk_supplement_error("\t- Double-click on \"Lock pages in memory\"");
        lnk_supplement_error("\t- Click \"Add User or Group...\"");
        lnk_supplement_error("\t- Type in your user name");
        lnk_supplement_error("\t- Click Oks and reboot the machine");
#endif
      }
    } else if (value_strings.node_count == 1) {
      if (str8_match_lit("quiet", value_strings.first->string, StringMatchFlag_CaseInsensitive)) {
        OS_ProcessInfo *process_info = os_get_process_info();
        if (process_info->large_pages_allowed) {
          arena_default_flags |= ArenaFlag_LargePages;
        }
      } else if (str8_match_lit("no", value_strings.first->string, StringMatchFlag_CaseInsensitive)) {
        arena_default_flags &= ~ArenaFlag_LargePages;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid parameter: \"%S\", expected NO or QUIET", value_strings.first->string);
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
    }
  } break;

  case LNK_CmdSwitch_Rad_LinkVer: {
    lnk_cmd_switch_parse_version(obj, cmd_switch, value_strings, &config->link_ver);
  } break;

  case LNK_CmdSwitch_Rad_Log: {
    if (value_strings.node_count == 1) {
      if (str8_match_lit("all", value_strings.first->string, StringMatchFlag_CaseInsensitive)) {
        for (U64 ilog = 0; ilog < LNK_Log_Count; ilog += 1) {
          lnk_set_log_status((LNK_LogType)ilog, 1);
        }
      } else if (str8_match_lit("io", value_strings.first->string, StringMatchFlag_CaseInsensitive)) {
        lnk_set_log_status(LNK_Log_IO_Read, 1);
        lnk_set_log_status(LNK_Log_IO_Write, 1);
      } else {
        LNK_LogType log_type = lnk_log_type_from_string(value_strings.first->string);
        if (log_type == LNK_Log_Null) {
          lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown parameter \"%S\"", value_strings.first->string);
        } else {
          lnk_set_log_status(log_type, 1);
        }
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters, expected 1");
    }
  } break;

  case LNK_CmdSwitch_Rad_MtPath: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &config->mt_path);
  } break;

  case LNK_CmdSwitch_Rad_OsVer: {
    lnk_cmd_switch_parse_version(obj, cmd_switch, value_strings, &config->os_ver);
  } break;

  case LNK_CmdSwitch_Rad_PageSize: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &config->machine_page_size, 0);
  } break;

  case LNK_CmdSwitch_Rad_PathStyle: {
    if (value_strings.node_count == 1) {
      PathStyle path_style = path_style_from_string(str8_list_first(&value_strings));
      if (path_style != PathStyle_Null) {
        config->path_style = path_style;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse parameter \"%S\"", value_strings.first->string);
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters");
    }
  } break;

  case LNK_CmdSwitch_Rad_PdbHashTypeNames: {
    String8              mode_string = str8_list_first(&value_strings);

    LNK_TypeNameHashMode mode;
    if (mode_string.size == 0) {
      config->pdb_hash_type_names = LNK_TypeNameHashMode_Lenient;
    } else {
      mode = lnk_type_name_hash_mode_from_string(mode_string);
      if (mode == LNK_TypeNameHashMode_Null) {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown parameter: \"%S\"", mode_string);
      } else {
        config->pdb_hash_type_names = mode;
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_PdbHashTypeNameMap: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value_strings, &config->pdb_hash_type_name_map);
  } break;

  case LNK_CmdSwitch_Rad_PdbHashTypeNameLength: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &config->pdb_hash_type_name_length, 0);
  } break;

  case LNK_CmdSwitch_Rad_RemoveSection: {
    String8 sect_name = {0};
    if (lnk_cmd_switch_parse_string(obj, cmd_switch, value_strings, &sect_name)) {
      sect_name = push_str8_copy(config->arena, sect_name);
      str8_list_push(config->arena, &config->remove_sections, sect_name);
    }
  } break;

  case LNK_CmdSwitch_Rad_SharedThreadPool: {
    if (value_strings.node_count == 0) {
      config->shared_thread_pool_name = str8_lit(LNK_DEFAULT_THREAD_POOL_NAME);
    } else {
      lnk_cmd_switch_parse_string(obj, cmd_switch, value_strings, &config->shared_thread_pool_name);
      if (config->shared_thread_pool_name.size == 0) {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid empty string for thread pool name");
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_SharedThreadPoolMaxWorkers: {
    OS_SystemInfo *sysinfo = os_get_system_info();
    if (value_strings.node_count == 0) {
      config->max_worker_count = sysinfo->logical_processor_count;
    } else {
      lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &config->max_worker_count, 0);
      if (config->max_worker_count == 0) {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "number of workers must be greater than zero");
      } else if (config->max_worker_count > sysinfo->logical_processor_count) {
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "number of workers %llu exceeds processor count %llu", config->max_worker_count, sysinfo->logical_processor_count);
        config->max_worker_count = sysinfo->logical_processor_count;
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_SuppressError: {
    U64List error_code_list = {0};
    if (lnk_cmd_switch_parse_u64_list(scratch.arena, obj, cmd_switch, value_strings, &error_code_list, 0)) {
      for (U64Node *error_code_n = error_code_list.first; error_code_n != 0; error_code_n = error_code_n->next) {
        if (error_code_n->data < LNK_Error_Count) {
          lnk_suppress_error(error_code_n->data);
        } else {
          lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "unknown error code %llu", error_code_n->data);
        }
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_TargetOs: {
    if (value_strings.node_count == 1) {
      String8 os_string = str8_list_first(&value_strings);
      OperatingSystem target_os = operating_system_from_string(os_string);
      if (target_os != OperatingSystem_Null) {
        config->target_os = target_os;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown operating system type %S", os_string);
      }
    } else {
      lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "expected 1 parameter");
    }
  } break;

  case LNK_CmdSwitch_Rad_WriteTempFiles: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value_strings, &config->write_temp_files);
  } break;

  case LNK_CmdSwitch_Rad_TimeStamp: {
    lnk_cmd_switch_parse_u32(obj, cmd_switch, value_strings, &config->time_stamp, 0);
  } break;

  case LNK_CmdSwitch_Rad_UnresolvedSymbolLimit: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &config->unresolved_symbol_limit, 0);
  } break;

  case LNK_CmdSwitch_Rad_UnresolvedSymbolRefLimit: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &config->unresolved_symbol_ref_limit, 0);
  } break;

  case LNK_CmdSwitch_Rad_Version: {
    fprintf(stdout, "%s\n", BUILD_TITLE_STRING_LITERAL);
    os_abort(0);
  } break;

  case LNK_CmdSwitch_Rad_Workers: {
    U64 worker_count;
    if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value_strings, &worker_count, 0)) {
      config->worker_count = worker_count;
    }
  } break;

  case LNK_CmdSwitch_Help: {
    lnk_print_help();
    os_abort(0);
  } break;
  }

  scratch_end(scratch);
}

internal LNK_Config *
lnk_config_from_cmd_line(String8List raw_cmd_line, LNK_CmdLine cmd_line)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  Arena      *arena  = arena_alloc();
  LNK_Config *config = push_array(arena, LNK_Config, 1);
  config->arena                     = arena;
  config->raw_cmd_line              = str8_list_copy(arena, &raw_cmd_line);
  config->work_dir                  = os_get_current_path(arena);
  config->build_imp_lib             = 1;
  config->build_exp                 = 1;
  config->heap_reserve              = MB(1);
  config->heap_commit               = KB(1);
  config->stack_reserve             = MB(1);
  config->stack_commit              = KB(1);
  config->pdb_hash_type_names       = LNK_TypeNameHashMode_None;
  config->pdb_hash_type_name_length = 8;
  config->data_dir_count            = PE_DataDirectoryIndex_COUNT;
  config->export_ht                 = hash_table_init(arena, max_U16/2);
  config->alt_name_ht               = hash_table_init(arena, 0x100);
  config->include_symbol_ht         = hash_table_init(arena, 0x100);
  config->delay_load_ht             = hash_table_init(arena, 0x100);
  config->disallow_lib_ht           = hash_table_init(arena, 0x100);

  // process command line switches
  for (LNK_CmdOption *cmd = cmd_line.first_option; cmd != 0; cmd = cmd->next) {
    lnk_apply_cmd_option_to_config(config, cmd->string, cmd->value_strings, 0);
  }

  // :manifest_input
  if (lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_ManifestInput)) {
    if (config->manifest_opt == LNK_ManifestOpt_Embed) {
      for (LNK_CmdOption *cmd = cmd_line.first_option; cmd != 0; cmd = cmd->next) {
        LNK_CmdSwitchType cmd_switch = lnk_cmd_switch_type_from_string(cmd->string);
        if (cmd_switch == LNK_CmdSwitch_ManifestInput) {
          String8List manifest_list = str8_list_copy(arena, &cmd->value_strings);
          str8_list_concat_in_place(&config->input_list[LNK_Input_Manifest], &manifest_list);
        }
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, 0, LNK_CmdSwitch_ManifestInput, "missing /MANIFEST:EMBED");
    }
  }

  // set default manifest resource id
  if (config->manifest_resource_id == 0) {
    if (config->file_characteristics & PE_ImageFileCharacteristic_FILE_DLL) {
      config->manifest_resource_id = push_u64(arena, 2);
    } else {
      config->manifest_resource_id = push_u64(arena, 1);
    }
  }

  // input files
  for (String8Node *input_node = cmd_line.input_list.first; input_node != 0; input_node = input_node->next) {
    String8 path = push_str8_copy(arena, input_node->string);
    String8 ext = str8_skip_last_dot(path);

    // map file extension to input type
    LNK_InputType input_type = lnk_input_type_from_string(ext);

    // do we support this file format?
    if (input_type == LNK_Input_Null) {
      lnk_error(LNK_Error_Cmdl, "unknown file format \"%S\"", path);
      continue;
    }

    // psuh file path
    str8_list_push(arena, &config->input_list[input_type], path);
  }

  // os version and subsystem are always same?
  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Rad_OsVer)) {
    config->os_ver = config->subsystem_ver;
  }
  
  // don't emit bind table with /ALLOWBIND:NO
  if (config->dll_characteristics & PE_DllCharacteristic_NO_BIND) {
    config->import_table_emit_biat = LNK_SwitchState_No;
  }
  
  // set flags for /OPT
  {
    // these flags remove and merge inline functions and methods defined in class,
    // and makes stepping tougher, in debug mode we don't link with these optimizations
    // unless user specifically orverrides.
    if (config->debug_mode != LNK_DebugMode_None) {
      if (config->opt_ref == LNK_SwitchState_Null) {
        config->opt_ref = LNK_SwitchState_No;
      }
      if (config->opt_icf == LNK_SwitchState_Null) {
        config->opt_icf = LNK_SwitchState_No;
      }
    }
    
    // by default enable all optimizations
    if (config->opt_ref == LNK_SwitchState_Null) {
      config->opt_ref = LNK_SwitchState_Yes;
    }
    if (config->opt_icf == LNK_SwitchState_Null) {
      config->opt_icf = LNK_SwitchState_Yes;
    }
    if (config->opt_lbr == LNK_SwitchState_Null) {
      config->opt_lbr = LNK_SwitchState_Yes;
    }
  }

  // warn about unused large address aware flag
  if ((~config->file_characteristics & PE_ImageFileCharacteristic_LARGE_ADDRESS_AWARE) && (config->file_characteristics & PE_ImageFileCharacteristic_FILE_DLL)) {
    lnk_error(LNK_Warning_NoLargeAddressAwarenessForDll, "/LARGEADDRESSAWARE:NO has no effect when specified together with /DLL");
  }
  
  // error check base address flags
  if (config->flags & LNK_ConfigFlag_Fixed) {
    if (lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_DynamicBase)) {
      B32 is_dynamic_base_set = !!(config->dll_characteristics & PE_DllCharacteristic_DYNAMIC_BASE);
      if (is_dynamic_base_set) {
        lnk_error(LNK_Error_IncomatibleCmdOptions, "unable to link with /FIXED and /DYNAMICBASE at the same time");
      }
    }
  }

  if (lnk_is_thread_pool_shared(config)) {
    if (config->worker_count > config->max_worker_count) {
      config->worker_count = config->max_worker_count;
      lnk_error_cmd_switch(LNK_Warning_Cmdl, 0, LNK_CmdSwitch_Rad_Workers, "worker count %llu exceeds thread pool max worker count %llu; claping count to max", config->worker_count, config->max_worker_count);
    }
  }
  
  // set flags for /FIXED
  if (config->flags & LNK_ConfigFlag_Fixed) {
    config->file_characteristics |= PE_ImageFileCharacteristic_STRIPPED;
    config->dll_characteristics &= ~PE_DllCharacteristic_DYNAMIC_BASE;
  }
  // if we don't have a fixed image and dynamic base switch 
  // was omitted we make image with dynamic base
  else if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_DynamicBase)) {
    config->dll_characteristics |= PE_DllCharacteristic_DYNAMIC_BASE;
  }
  
  // set flag for /guard
  if (config->guard_flags != LNK_Guard_None) {
    config->dll_characteristics |= PE_DllCharacteristic_GUARD_CF;
  }

  // handle empty /OUT
  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Out)) {
    String8 name      = str8_list_first(&config->input_list[LNK_Input_Obj]);
    String8 ext       = (config->file_characteristics & PE_ImageFileCharacteristic_FILE_DLL) ? str8_lit("dll") : str8_lit("exe");
    config->image_name = path_replace_file_extension(scratch.arena, name, ext);
  }

  // handle empty /PDB
  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Pdb)) {
    config->pdb_name = path_replace_file_extension(scratch.arena, config->image_name, str8_lit("pdb"));
  }

  // handle empty /RAD_DEBUG_NAME
  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Rad_DebugName)) {
    config->rad_debug_name = path_replace_file_extension(scratch.arena, config->image_name, str8_lit("rdi"));
  }

  // handle empty /IMPLIB
  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_ImpLib)) {
    config->imp_lib_name = path_replace_file_extension(scratch.arena, config->image_name, str8_lit("lib"));
  }

  // handle empty /MANIFESTFILE
  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_ManifestFile)) {
    config->manifest_name = push_str8f(scratch.arena, "%S.manifest", config->image_name);
  }

  // convert to full paths
  config->image_name     = os_full_path_from_path(arena, config->image_name);
  config->pdb_name       = os_full_path_from_path(arena, config->pdb_name);
  config->rad_debug_name = os_full_path_from_path(arena, config->rad_debug_name);
  config->imp_lib_name   = os_full_path_from_path(arena, config->imp_lib_name);
  config->manifest_name  = os_full_path_from_path(arena, config->manifest_name);

  // collect env vars
  HashTable *env_vars = hash_table_init(scratch.arena, 512);
  {
#if OS_WINDOWS
    OS_ProcessInfo *process_info = os_get_process_info();
    for (String8Node *node = process_info->environment.first; node != 0; node = node->next) {
      String8List list = str8_split_by_string_chars(scratch.arena, node->string, str8_lit("="), 0);

      String8 key = list.first->string;
      String8 val = str8_zero();
      if (list.node_count == 2) {
        val = list.last->string;
      } else if (list.node_count > 2) {
        U64 sep_idx = str8_find_needle(node->string, node->string.size, str8_lit("="), 0);
        val = str8_skip(node->string, sep_idx+1);
      }

      hash_table_push_path_string(scratch.arena, env_vars, key, val);
    }
#endif
  }

  // define linker env vars
  hash_table_push_path_string(scratch.arena, env_vars, str8_lit("_pdb"),          str8_skip_last_slash(config->pdb_name));
  hash_table_push_path_string(scratch.arena, env_vars, str8_lit("_ext"),          str8_skip_last_dot(config->image_name));
  hash_table_push_path_string(scratch.arena, env_vars, str8_lit("_rad_pdb_path"), config->pdb_name);
  hash_table_push_path_string(scratch.arena, env_vars, str8_lit("_rad_rdi"),      str8_skip_last_slash(config->rad_debug_name));
  hash_table_push_path_string(scratch.arena, env_vars, str8_lit("_rad_rdi_path"), config->rad_debug_name);

  // collect LIB and LIBPATH
  if (config->flags & LNK_ConfigFlag_EnvLib) {
    KeyValuePair *lib = hash_table_search_path(env_vars, str8_lit("lib"));
    if (lib) {
      String8List val_list      = str8_split_by_string_chars(scratch.arena, lib->value_string, str8_lit(";"), 0);
      String8List val_list_copy = str8_list_copy(arena, &val_list);
      str8_list_concat_in_place(&config->lib_dir_list, &val_list_copy);
    }

    KeyValuePair *lib_path = hash_table_search_path(env_vars, str8_lit("libpath"));
    if (lib_path) {
      String8List val_list      = str8_split_by_string_chars(scratch.arena, lib->value_string, str8_lit(";"), 0);
      String8List val_list_copy = str8_list_copy(arena, &val_list);
      str8_list_concat_in_place(&config->lib_dir_list, &val_list_copy);
    }
  }
  
  // :PdbAltPath
  config->pdb_alt_path = lnk_expand_env_vars_windows(arena, env_vars, config->pdb_alt_path);

  // :Rad_DebugAltPath
  config->rad_debug_alt_path = lnk_expand_env_vars_windows(arena, env_vars, config->rad_debug_alt_path);

  // create temporary files names
  if (config->write_temp_files == LNK_SwitchState_Yes) {
    config->temp_rad_chunk_map_name = push_str8f(arena, "%S.tmp%x", config->rad_chunk_map_name, config->time_stamp);
    config->temp_image_name         = push_str8f(arena, "%S.tmp%x", config->image_name,         config->time_stamp);
    config->temp_pdb_name           = push_str8f(arena, "%S.tmp%x", config->pdb_name,           config->time_stamp);
    config->temp_rad_debug_name     = push_str8f(arena, "%S.tmp%x", config->rad_debug_name,     config->time_stamp);
  }

  scratch_end(scratch);
  ProfEnd();
  return config;
}

