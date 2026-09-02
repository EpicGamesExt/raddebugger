// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

global read_only LNK_CmdSwitch g_cmd_switch_map[] =
{
  { LNK_CmdSwitch_Null,               0, LNK_CmdValueKind_Null,   "",                     "", ""                                                                                           },
  { LNK_CmdSwitch_NotImplemented,     0, LNK_CmdValueKind_Null,   "NOT_IMPLEMENTED",      "", ""                                                                                           },
  { LNK_CmdSwitch_Align,              0, LNK_CmdValueKind_Scalar, "ALIGN",                ":#",                             "Set section alignment in the virtual address space."          },
  { LNK_CmdSwitch_AllowBind,          0, LNK_CmdValueKind_Scalar, "ALLOWBIND",            "[:NO]",                          "Toggles bind bit in the image header."                        },
  { LNK_CmdSwitch_AllowIsolation,     0, LNK_CmdValueKind_Scalar, "ALLOWISOLATION",       "[:NO]",                          "Toggles isolation bit in the image header."                   },
  { LNK_CmdSwitch_AlternateName,      1, LNK_CmdValueKind_Scalar, "ALTERNATENAME",        ":FROM=TO",                       "Creates a symbol alias \"FROM=TO\"."                          },
  { LNK_CmdSwitch_AppContainer,       0, LNK_CmdValueKind_Scalar, "APPCONTAINER",         "[:NO]",                          "Toggles app container bit in the image header."               },
  { LNK_CmdSwitch_Base,               0, LNK_CmdValueKind_List,   "BASE",                 "{ADDRESS[,SIZE]|@FILENAME,KEY}", "Set default image base address."                              },
  { LNK_CmdSwitch_Brepro,             0, LNK_CmdValueKind_Null,   "BREPRO",               "",                               "No support."                                                  },
  { LNK_CmdSwitch_Debug,              0, LNK_CmdValueKind_Scalar, "DEBUG",                "[:{FULL|NONE}]",                 "Controls debug info level."                                   },
  { LNK_CmdSwitch_DefaultLib,         1, LNK_CmdValueKind_Scalar, "DEFAULTLIB",           ":LIBNAME",                       "Set default library."                                         },
  { LNK_CmdSwitch_Def,                1, LNK_CmdValueKind_Scalar, "DEF",                  ":FILENAME",                      "Read exports from a module-definition file."                   },
  { LNK_CmdSwitch_Delay,              0, LNK_CmdValueKind_Scalar, "DELAY",                ":{NOBIND|UNLOAD}",               "Controls emission of unload and bind tables."                 },
  { LNK_CmdSwitch_DelayLoad,          0, LNK_CmdValueKind_Scalar, "DELAYLOAD",            ":DLL",                           "Delay load DLL."                                              },
  { LNK_CmdSwitch_Dll,                0, LNK_CmdValueKind_Null,   "DLL",                  "",                               "Link to a DLL."                                               },
  { LNK_CmdSwitch_DisallowLib,        1, LNK_CmdValueKind_Scalar, "DISALLOWLIB",          ":LIBRARY",                       "Prevents LIBRARY from being linked.",                         },
  { LNK_CmdSwitch_DynamicBase,        0, LNK_CmdValueKind_Scalar, "DYNAMICBASE",          "[:NO]",                          "Enable random base address in the linked image."              },
  { LNK_CmdSwitch_Entry,              1, LNK_CmdValueKind_Scalar, "ENTRY",                ":FUNCTION",                      "Name of the entry point symbol."                              },
  { LNK_CmdSwitch_Export,             1, LNK_CmdValueKind_List,   "EXPORT",               ":SYMBOL",                        "Create an export entry for SYMBOL."                           },
  { LNK_CmdSwitch_FailIfMismatch,     1, LNK_CmdValueKind_Scalar, "FAILIFMISMATCH",       "{id=value}",                     "Fails to link if same ids have conflicting values."           },
  { LNK_CmdSwitch_FileAlign,          0, LNK_CmdValueKind_Scalar, "FILEALIGN",            ":#",                             "Set section alignment in the file."                           },
  { LNK_CmdSwitch_Fixed,              0, LNK_CmdValueKind_Scalar, "FIXED",                "[:NO]",                          "Load the image at the default base address."                  },
  { LNK_CmdSwitch_Force,              0, LNK_CmdValueKind_Null,   "FORCE",                "",                               "Force image output despite errors."                           },
  { LNK_CmdSwitch_FunctionPadMin,     0, LNK_CmdValueKind_Scalar, "FUNCTIONPADMIN",       ":#",                             "Minimum function byte size."                                  },
  { LNK_CmdSwitch_Guard,              0, LNK_CmdValueKind_List,   "GUARD",                ":{CF|NO|LONGJMP|EHCONT}",        "Controls Control Flow Guard metadata."                        },
  { LNK_CmdSwitch_GuardSym,           1, LNK_CmdValueKind_List,   "GUARDSYM",             ":SYMBOL,S",                      "MSVC guard symbol directive."                                 },
  { LNK_CmdSwitch_Heap,               0, LNK_CmdValueKind_List,   "HEAP",                 "RESERVE[,COMMIT]",               "Set reserve and commit size for the heap."                    },
  { LNK_CmdSwitch_HighEntropyVa,      0, LNK_CmdValueKind_Scalar, "HIGHENTROPYVA",        "[:NO]",                          "Indicate that image supports full 64-bit address space ASLR." },
  { LNK_CmdSwitch_Ignore,             0, LNK_CmdValueKind_Scalar, "IGNORE",               ":#",                             "Ignore a warning."                                            },
  { LNK_CmdSwitch_ImpLib,             0, LNK_CmdValueKind_Scalar, "IMPLIB",               ":FILENAME",                      "Set file name for the import library."                        },
  { LNK_CmdSwitch_Include,            1, LNK_CmdValueKind_Scalar, "INCLUDE",              ":SYMBOL",                        "Force a link against SYMBOL."                                 },
  { LNK_CmdSwitch_InferAsanLibs,      1, LNK_CmdValueKind_Scalar, "INFERASANLIBS",        "[:NO]",                          "No support."                                                  },
  { LNK_CmdSwitch_InferAsanLibsNo,    1, LNK_CmdValueKind_Null,   "INFERASANLIBSNO",      "",                               "No support.",                                                 },
  { LNK_CmdSwitch_LargeAddressAware,  0, LNK_CmdValueKind_Scalar, "LARGEADDRESSAWARE",    "[:NO]",                          "For images that can handle addresses > 2GiB."                 },
  { LNK_CmdSwitch_Lib,                0, LNK_CmdValueKind_Null,   "LIB",                  "",                               "Turn linker into lib.exe."                                    },
  { LNK_CmdSwitch_LibPath,            0, LNK_CmdValueKind_Scalar, "LIBPATH",              ":DIR",                           "Add DIR for the linker to search for libraries."              },
  { LNK_CmdSwitch_Machine,            0, LNK_CmdValueKind_Scalar, "MACHINE",              ":{X64|X86}",                     "Image target platform."                                       },
  { LNK_CmdSwitch_Map,                0, LNK_CmdValueKind_Scalar, "MAP",                  "[:FILENAME]",                    "Create a linker map file."                                    },
  { LNK_CmdSwitch_Manifest,           0, LNK_CmdValueKind_List,   "MANIFEST",             "[:{EMBED[,ID=#]|NO]",            "Controls whether the linker should create a side manifest."   },
  { LNK_CmdSwitch_ManifestDependency, 1, LNK_CmdValueKind_Scalar, "MANIFESTDEPENDENCY",   ":\"manifest dependency XML string\"", "Add a manifest dependency."                              },
  { LNK_CmdSwitch_ManifestFile,       0, LNK_CmdValueKind_Scalar, "MANIFESTFILE",         ":FILENAME",                      "Specifies a manifest file."                                   },
  { LNK_CmdSwitch_ManifestInput,      0, LNK_CmdValueKind_Scalar, "MANIFESTINPUT",        ":FILENAME",                      "Manifest that is embedded in the image."                      },
  { LNK_CmdSwitch_ManifestUac,        0, LNK_CmdValueKind_Scalar, "MANIFESTUAC",          ":{NO|{'level'={'asInvoker'|'highestAvailable'|'requireAdministrator'} ['uiAccess'={'true'|'false'}]}}", "Controls UAC information in the manifest." },
  { LNK_CmdSwitch_Merge,              1, LNK_CmdValueKind_Scalar, "MERGE",                ":FROM=TO",                       "Merges sections."                                             },
  { LNK_CmdSwitch_Natvis,             0, LNK_CmdValueKind_Scalar, "NATVIS",               ":FILENAME",                      "NATVIS to embed in the PDB."                                  },
  { LNK_CmdSwitch_NoDefaultLib,       1, LNK_CmdValueKind_Scalar, "NODEFAULTLIB",         ":LIBNAME",                       "Ignore a /DEFAULTLIB."                                        },
  { LNK_CmdSwitch_NoDefaultLib,       0, LNK_CmdValueKind_Scalar, "NOD",                  ":LIBNAME",                       "Alias for /NODEFAULTLIB."                                     },
  { LNK_CmdSwitch_NoExp,              0, LNK_CmdValueKind_Null,   "NOEXP",                "",                               "No support."                                                  },
  { LNK_CmdSwitch_NoImpLib,           0, LNK_CmdValueKind_Null,   "NOIMPLIB",             "",                               "Do not create the import library."                            },
  { LNK_CmdSwitch_NxCompat,           0, LNK_CmdValueKind_Scalar, "NXCOMPAT",             "[:NO]",                          "Image is compatible with data execution prevention."          },
  { LNK_CmdSwitch_Opt,                0, LNK_CmdValueKind_List,   "OPT",                  "{REF|ICF}",                      "Optimizations."                                               },
  { LNK_CmdSwitch_Out,                0, LNK_CmdValueKind_Scalar, "OUT",                  ":FILENAME",                      "File name of the output image."                               },
  { LNK_CmdSwitch_Pdb,                0, LNK_CmdValueKind_Scalar, "PDB",                  ":FILENAME",                      "File name of the output PDB."                                 },
  { LNK_CmdSwitch_PdbAltPath,         0, LNK_CmdValueKind_Scalar, "PDBALTPATH",           ":PATH",                          "Alternative output path for the PDB."                         },
  { LNK_CmdSwitch_PdbPageSize,        0, LNK_CmdValueKind_Scalar, "PDBPAGESIZE",          ":#",                             "Page size must be power of two."                              },
  { LNK_CmdSwitch_PdbStripped,        0, LNK_CmdValueKind_Scalar, "PDBSTRIPPED",          ":FILENAME",                      "Create a stripped PDB containing public symbols, a section map, and a list of object files." },
  { LNK_CmdSwitch_Release,            1, LNK_CmdValueKind_Null,   "RELEASE",              "",                               "Write image checksum."                                        },
  { LNK_CmdSwitch_Section,            1, LNK_CmdValueKind_List,   "SECTION",              ":NAME,ATTRS",                    "Set output section attributes."                              },
  { LNK_CmdSwitch_Stack,              1, LNK_CmdValueKind_List,   "STACK",                ":RESERVE[,COMMIT]",              "Set reserve and commit size for the stack."                   },
  { LNK_CmdSwitch_SubSystem,          1, LNK_CmdValueKind_List,   "SUBSYSTEM",            ":{CONSOLE|NATIVE|WINDOWS}[,#[.##]]", "Set subsystem for the image."                             },
  { LNK_CmdSwitch_TsAware,            0, LNK_CmdValueKind_Scalar, "TSAWARE",              "[:NO]",                          "Image is terminal server aware."                              },
  { LNK_CmdSwitch_Version,            0, LNK_CmdValueKind_Scalar, "VERSION",              "",                               "Image version."                                               },
  { LNK_CmdSwitch_WholeArchive,       0, LNK_CmdValueKind_Scalar, "WHOLEARCHIVE",         "[:LIBNAME]",                     "Force linker to pull in all objs from the specified lib."     },

  { LNK_CmdSwitch_Rad_Age,                          0, LNK_CmdValueKind_Scalar, "RAD_AGE",                              ":#",                   "Age embeded in EXE and PDB, used to validate incremental build. Default is 1."    },
  //{ LNK_CmdSwitch_Rad_BuildExp,                     0, LNK_CmdValueKind_Scalar, "RAD_BUILD_EXP",                        "[:NO]",     "Build export data."                                                             },
  { LNK_CmdSwitch_Rad_BuildInfo,                    0, LNK_CmdValueKind_Null,   "RAD_BUILD_INFO",                       "",                     "Print build info and exit."                                                       },
  { LNK_CmdSwitch_Rad_BuildImpLib,                  0, LNK_CmdValueKind_Scalar, "RAD_BUILD_IMPLIB",                     "[:NO]",                "Build import library."                                                            },
  { LNK_CmdSwitch_Rad_CheckUnusedDelayLoadDll,      0, LNK_CmdValueKind_Scalar, "RAD_CHECK_UNUSED_DELAY_LOAD_DLL",      "[:NO]",                "Check for unused delay load dlls."                                                },
  { LNK_CmdSwitch_Rad_DataDirCount,                 0, LNK_CmdValueKind_Scalar, "RAD_DATA_DIR_COUNT",                   ":#",                   "Internal default for PE optional header data directory count."                    },
  { LNK_CmdSwitch_Rad_MapLinesForUnresolvedSymbols, 0, LNK_CmdValueKind_Scalar, "RAD_MAP_LINES_FOR_UNRESOLVED_SYMBOLS", "[:NO]",                "Use debug info to print source file location for unresolved symbol"               },
  { LNK_CmdSwitch_Rad_MemoryMapFiles,               0, LNK_CmdValueKind_Scalar, "RAD_MEMORY_MAP_FILES",                 "[:{NO|READ_ONLY|READ_WRITE}]", "When enabled, files are memory-mapped instead of being read entirely on request." },
  { LNK_CmdSwitch_Rad_BootMode,                     0, LNK_CmdValueKind_Scalar, "RAD_BOOT_MODE",                        "[:LINKER|TYPE_SERVER]", "Overrides default boot program."                                                 },
  { LNK_CmdSwitch_Rad_Debug,                        0, LNK_CmdValueKind_Scalar, "RAD_DEBUG",                            "[:NO]",                "Emit RAD debug info file."                                                        },
  { LNK_CmdSwitch_Rad_DebugAltPath,                 0, LNK_CmdValueKind_Scalar, "RAD_DEBUGALTPATH",                     ":PATH",                "Alternative output path for the RDI."                                             },
  { LNK_CmdSwitch_Rad_DebugName,                    0, LNK_CmdValueKind_Scalar, "RAD_DEBUG_NAME",                       ":FILENAME",            "Set file name for RAD debug info file."                                           },
  { LNK_CmdSwitch_Rad_DelayBind,                    0, LNK_CmdValueKind_Scalar, "RAD_DELAY_BIND",                       "[:NO]",                "Emit bindable imports."                                                           },
  { LNK_CmdSwitch_Rad_DoMerge,                      0, LNK_CmdValueKind_Scalar, "RAD_DO_MERGE",                         "[:NO]",                "Set whether the linker should execute /MERGE."                                    },
  { LNK_CmdSwitch_Rad_EnvLib,                       0, LNK_CmdValueKind_Scalar, "RAD_ENV_LIB",                          "[:NO]",                "Collect libraries from %%LIB%% and %%LIBPATH%% varibles."                         },
  { LNK_CmdSwitch_Rad_Exe,                          0, LNK_CmdValueKind_Scalar, "RAD_EXE",                              "[:NO]",                "Set EXE bit in the image header."                                                 },
  { LNK_CmdSwitch_Rad_Guid,                         0, LNK_CmdValueKind_Scalar, "RAD_GUID",                             ":{IMAGEBLAKE3|XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXXXXXX}", "The image guid that is embeded in the debug info." },
  { LNK_CmdSwitch_Rad_LargePages,                   0, LNK_CmdValueKind_Scalar, "RAD_LARGE_PAGES",                      "[:NO]",                "Disabled by default on Windows."                                                  },
  { LNK_CmdSwitch_Rad_LinkVer,                      0, LNK_CmdValueKind_Scalar, "RAD_LINK_VER",                         ":##,##",               "Linker version."                                                                  },
  { LNK_CmdSwitch_Rad_Log,                          0, LNK_CmdValueKind_Scalar, "RAD_LOG",                              ":{ALL,INPUT_OBJ,INPUT_LIB,IO,LINK_STATS,TIMERS}", "Loggers."                                              },
  { LNK_CmdSwitch_Rad_MtPath,                       0, LNK_CmdValueKind_Scalar, "RAD_MT_PATH",                          ":EXEPATH",             "Exe path to the manifest tool (default: " LNK_MANIFEST_MERGE_TOOL_NAME ")"        },
  { LNK_CmdSwitch_Rad_OsVer,                        0, LNK_CmdValueKind_Scalar, "RAD_OS_VER",                           ":##,##",               "OS version."                                                                      },
  { LNK_CmdSwitch_Rad_PageSize,                     0, LNK_CmdValueKind_Scalar, "RAD_PAGE_SIZE",                        ":#",                   "Must be power of two."                                                            },
  { LNK_CmdSwitch_Rad_PathStyle,                    0, LNK_CmdValueKind_Scalar, "RAD_PATH_STYLE",                       ":{WindowsAbsolute|UnixAbsolute}", "Set path style in the PDB."                                            },
  { LNK_CmdSwitch_Rad_PdbHashTypeNameLength,        0, LNK_CmdValueKind_Scalar, "RAD_PDB_HASH_TYPE_NAME_LENGTH",        ":#",                   "Number of hash bytes to use to replace type name. Default 8 bytes (Max 16)."      },
  { LNK_CmdSwitch_Rad_PdbHashTypeNameMap,           0, LNK_CmdValueKind_Scalar, "RAD_PDB_HASH_TYPE_NAME_MAP",           ":FILENAME",            "Produce map file with hash -> type name mappings."                                },
  { LNK_CmdSwitch_Rad_PdbHashTypeNames,             0, LNK_CmdValueKind_Scalar, "RAD_PDB_HASH_TYPE_NAMES",              ":{NONE|LENIENT|FULL}", "Replace type names in LF_STRUCTURE and LF_CLASS with hashes."                     },
  { LNK_CmdSwitch_Rad_RemoveSection,                0, LNK_CmdValueKind_Scalar, "RAD_REMOVE_SECTION",                   ":NAME",                "Removes a section from the image."                                                },
  { LNK_CmdSwitch_Rad_SharedThreadPool,             0, LNK_CmdValueKind_Scalar, "RAD_SHARED_THREAD_POOL",               "[:STRING]",            "Default value \"" LNK_DEFAULT_THREAD_POOL_NAME "\""                               },
  { LNK_CmdSwitch_Rad_SharedThreadPoolMaxWorkers,   0, LNK_CmdValueKind_Scalar, "RAD_SHARED_THREAD_POOL_MAX_WORKERS",   ":#",                   "Set maximum number of workers in a thread pool."                                  },
  { LNK_CmdSwitch_Rad_SortImports,                  0, LNK_CmdValueKind_Scalar, "RAD_SORT_IMPORTS",                     "[:NO]",                "Sort static and delayed import tables by their order of appearance in libs, without assuming link order." },
  { LNK_CmdSwitch_Rad_IcfHashKind,                  0, LNK_CmdValueKind_Scalar, "RAD_ICF_HASH_KIND",                    "{BLAKE3|XXHASH}",      "Sets hashing algorithm for /OPT:ICF."                                             },
  { LNK_CmdSwitch_Rad_Ignore,                       0, LNK_CmdValueKind_List,   "RAD_IGNORE",                           ":#",                   "Ignore the specified RAD linker warning."                                         },
  { LNK_CmdSwitch_Rad_ImageAltPath,                 0, LNK_CmdValueKind_Scalar, "RAD_IMAGEALTPATH",                     ":FILENAME",            "Alternative name for the image"                                                   },
  { LNK_CmdSwitch_Rad_WriteTempFiles,               0, LNK_CmdValueKind_Scalar, "RAD_WRITE_TEMP_FILES",                 "[:NO]",                "When speicifed linker writes image and debug info to temporary files and renames after link is done." },
  { LNK_CmdSwitch_Rad_TimeStamp,                    0, LNK_CmdValueKind_Scalar, "RAD_TIME_STAMP",                       ":#",                   "Time stamp embeded in EXE and PDB."                                               },
  { LNK_CmdSwitch_Rad_DebugTypeHash,                0, LNK_CmdValueKind_Scalar, "RAD_DEBUG_TYPE_HASH",                  ":{BLAKE3|XXHASH}",     "Sets hashing algorithm for debug type merging."                                   },
  { LNK_CmdSwitch_Rad_DebugTypeHash,                0, LNK_CmdValueKind_Scalar, "RAD_TYPEHASHALG",                      ":{BLAKE3|XXHASH}",     "Alias of RAD_DEBUG_TYPE_HASH (spelling used by UnrealBuildTool)."                  },
  { LNK_CmdSwitch_Rad_UnresolvedSymbolLimit,        0, LNK_CmdValueKind_Scalar, "RAD_UNRESOLVED_SYMBOL_LIMIT",          ":#",                   "Limits number of unresolved symbol errors linker reports."                        },
  { LNK_CmdSwitch_Rad_UnresolvedSymbolRefLimit,     0, LNK_CmdValueKind_Scalar, "RAD_UNRESOLVED_SYMBOL_REF_LIMIT",      ":#",                   "Limit number of unresolved symbol references linker reports."                     },
  { LNK_CmdSwitch_Rad_Version,                      0, LNK_CmdValueKind_Null,   "RAD_VERSION",                          "",                     "Print version and exit."                                                          },
  { LNK_CmdSwitch_Rad_Workers,                      0, LNK_CmdValueKind_Scalar, "RAD_WORKERS",                          ":#",                   "Set number of workers created in the pool. Number is capped at 1024. When /RAD_SHARED_THREAD_POOL is specified this number cant exceed /RAD_SHARED_THREAD_POOL_MAX_WORKERS." },
  { LNK_CmdSwitch_Rad_DebugWorkers,                 0, LNK_CmdValueKind_Scalar, "RAD_DEBUG_WORKERS",                    ":#",                   "Cap concurrent workers in page-fault-bound debug-input stages (parse/prefetch). Default 20; 0 = uncapped. Output is identical either way; the cap only trades idle spinning in the kernel page-fault path for free cores." },
  { LNK_CmdSwitch_Rad_WorkDir,                      0, LNK_CmdValueKind_Scalar, "RAD_WORK_DIR",                         ":PATH",                "Working directory used for stable debug paths."                                   },

  { LNK_CmdSwitch_RadTypeServer,                   0, LNK_CmdValueKind_Scalar, "RAD_TYPE_SERVER", ":FILENAME", "Merge types and store them in the specified file. The filename must have the .rrt extension." },

  { LNK_CmdSwitch_LLVM_AddrSig, 0, LNK_CmdValueKind_Scalar, "LLVM_ADDRSIG", "[:NO]", "Use .llvm_addrsig to guide ICF." },
  { LNK_CmdSwitch_IfcMap,       1, LNK_CmdValueKind_Scalar, "IFCMAP",          ":FILENAME", "Map a header-unit module interface (.ifc) for debug-record resolution (TOML)." },
  { LNK_CmdSwitch_IfcDebugRecords, 0, LNK_CmdValueKind_Scalar, "IFCDEBUGRECORDS", "[:NO]",     "Resolve MSVC header-unit IFC debug records into real CodeView types." },

  { LNK_CmdSwitch_Help, 0, LNK_CmdValueKind_Null, "HELP", "", "" },
  { LNK_CmdSwitch_Help, 0, LNK_CmdValueKind_Null, "?",    "", "" },
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
  { "a",    LNK_Input_Lib }, // GNU lib
  { "res",  LNK_Input_Res },
  { "rrt",  LNK_Input_RRT },
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
  for EachElement(i, g_cmd_switch_map) {
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

internal String8List
lnk_cmd_line_values_from_switch(Arena *arena, LNK_CmdLine cmd_line, LNK_CmdSwitchType cmd_switch)
{
  String8List values = {0};
  String8 cmd_switch_name = lnk_string_from_cmd_switch_type(cmd_switch);
  for EachNode(cmd, LNK_CmdOption, cmd_line.first_option) {
    if (cmd->value.size > 0 && str8_matchi(cmd->string, cmd_switch_name)) {
      str8_list_push(arena, &values, push_str8_copy(arena, cmd->value));
    }
  }
  return values;
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
lnk_cmd_switch_parse_version(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value, Version *ver_out)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_parsed = 0;

  if (value.size > 0) {
    String8List split_list = str8_split_by_string_chars(scratch.arena, value, str8_lit("."), StringSplitFlag_KeepEmpties);

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
    lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
  }

exit:;
  scratch_end(scratch);
  return is_parsed;
}

internal B32
lnk_cmd_switch_parse_tuple(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List values, Rng1U64 *tuple_out)
{
  B32 is_parsed = 0;
  if (values.node_count == 1) {
    U64 v;
    if (try_u64_from_str8_c_rules(values.first->string, &v)) {
      tuple_out->v[0] = v;
      is_parsed = 1;
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse the parameter \"%S\"", values.first->string);
    }
  } else if (values.node_count == 2) {
    U64 a,b;
    if (try_u64_from_str8_c_rules(values.first->string, &a)) {
      if (try_u64_from_str8_c_rules(values.last->string, &b)) {
        tuple_out->v[0] = a;
        tuple_out->v[1] = b;
        is_parsed = 1;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse second parameter \"%S\"", values.last->string);
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse first parameter \"%S\"", values.first->string);
    }
  } else {
    lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
  }
  return is_parsed;
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

lnk_try_parse_s64(String8 string, LNK_ParseU64Flags flags, S64 *value_out)
{
  if (try_s64_from_str8_c_rules(string, value_out)) {
    if (flags & LNK_ParseU64Flag_CheckUnder32bit) {
      if (*value_out > max_S64) {
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
lnk_cmd_switch_parse_u64(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value, U64 *value_out, LNK_ParseU64Flags flags)
{
  if (value.size == 0) {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters, exepcted integer number as input");
    return 0;
  }
  if (!lnk_try_parse_u64(value, flags, value_out)) {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse string \"%S\"", value);
    return 0;
  }
  return 1;
}

internal B32
lnk_cmd_switch_parse_u32(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value_string, U32 *value_out, LNK_ParseU64Flags flags)
{
  U64 value;
  if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value_string, &value, flags | LNK_ParseU64Flag_CheckUnder32bit)) {
    *value_out = (U32)value;
    return 1;
  }
  return 0;
}

internal B32
lnk_cmd_switch_parse_u64_list(Arena *arena, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List values, U64List *list_out, LNK_ParseU64Flags flags)
{
  for (String8Node *string_n = values.first; string_n != 0; string_n = string_n->next) {
    U64 value;
    if (!lnk_try_parse_u64(string_n->string, flags, &value)) {
      return 0;
    }
    u64_list_push(arena, list_out, value);
  }
  return 1;
}

internal B32
lnk_cmd_switch_parse_s64_list(Arena *arena, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8List values, S64List *list_out, LNK_ParseU64Flags flags)
{
  for EachNode(string_n, String8Node, values.first) {
    S64 v;
    if (!lnk_try_parse_s64(string_n->string, flags, &v)) {
      return 0;
    }
    s64_list_push(arena, list_out, v);
  }
  return 1;
}

internal B32
lnk_cmd_switch_parse_flag(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value, LNK_SwitchState *value_out)
{
  B32 is_parsed = 0;
  if (value.size > 0) {
    if (str8_match_lit("no", value, StringMatchFlag_CaseInsensitive)) {
      *value_out = LNK_SwitchState_No;
      is_parsed = 1;
    } else if (str8_match_lit("yes", value, StringMatchFlag_CaseInsensitive)) {
      *value_out = LNK_SwitchState_Yes;
      is_parsed = 1;
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid parameter \"%S\"", value);
    }
  } else {
    *value_out = LNK_SwitchState_Yes;
    is_parsed = 1;
  }
  return is_parsed;
}

internal void
lnk_cmd_switch_set_flag_inv_16(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value, U16 *flags, U16 bits)
{
  LNK_SwitchState state;
  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &state)) {
    switch (state) {
    case LNK_SwitchState_Null: break;
    case LNK_SwitchState_Yes : *flags |= bits;  break;
    case LNK_SwitchState_No  : *flags &= ~bits; break;
    }
  }
}

internal void
lnk_cmd_switch_set_flag_inv_64(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value, U64 *flags, U64 bits)
{
  LNK_SwitchState state;
  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &state)) {
    switch (state) {
    case LNK_SwitchState_Null: break;
    case LNK_SwitchState_Yes : *flags |= bits;  break;
    case LNK_SwitchState_No  : *flags &= ~bits; break;
    }
  }
}

internal void
lnk_cmd_switch_set_flag_16(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value, U16 *flags, U16 bits)
{
  LNK_SwitchState state;
  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &state)) {
    switch (state) {
    case LNK_SwitchState_Null: break;
    case LNK_SwitchState_Yes : *flags |= bits;  break;
    case LNK_SwitchState_No  : *flags &= ~bits; break;
    }
  }
}

internal void
lnk_cmd_switch_set_flag_32(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value, U32 *flags, U32 bits)
{
  LNK_SwitchState state;
  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &state)) {
    switch (state) {
    case LNK_SwitchState_Null: break;
    case LNK_SwitchState_Yes : *flags |= bits;  break;
    case LNK_SwitchState_No  : *flags &= ~bits; break;
    }
  }
}

internal void
lnk_cmd_switch_set_flag_64(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value, U64 *flags, U64 bits)
{
  LNK_SwitchState state;
  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &state)) {
    switch (state) {
    case LNK_SwitchState_Null: break;
    case LNK_SwitchState_Yes : *flags |= bits;  break;
    case LNK_SwitchState_No  : *flags &= ~bits; break;
    }
  }
}

internal B32
lnk_cmd_switch_parse_string(LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value, String8 *string_out)
{
  if (value.size) {
    *string_out = value;
    return 1;
  } else {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "empty string is not allowed");
    return 0;
  }
}

internal void
lnk_cmd_switch_parse_string_copy(Arena *arena, LNK_Obj *obj, LNK_CmdSwitchType cmd_switch, String8 value, String8 *string_out)
{
  if (lnk_cmd_switch_parse_string(obj, cmd_switch, value, string_out)) {
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
  export_out->is_forwarder        = alias.size && str8_find_needle(name, 0, str8_lit("."), 0) < name.size;

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

internal String8
lnk_text_file_string_from_data(Arena *arena, String8 data)
{
  String8 result = data;

  if (data.size >= 2 && data.str[0] == 0xff && data.str[1] == 0xfe) {
    // decode UTF-16LE BOM
    result = str8_from_16(arena, str16((U16 *)(data.str + 2), (data.size - 2) / sizeof(U16)));
  } else if (data.size >= 3 && data.str[0] == 0xef && data.str[1] == 0xbb && data.str[2] == 0xbf) {
    // strip UTF-8 BOM
    result = str8_skip(data, 3);
  }

  return result;
}

internal void
lnk_push_export_to_config(LNK_Config *config, LNK_Obj *obj, PE_ExportParse export_parse)
{
  // lookup existing export
  String8             export_name = pe_name_from_export_parse(&export_parse);
  PE_ExportParseNode *exp_n       = hash_map_search_string_raw(&config->export_ht, export_name);

  if (exp_n == 0) {
    // push new export
    if (!export_parse.is_forwarder) {
      lnk_include_symbol(config, export_parse.name, 0);
    }

    exp_n = pe_export_parse_list_push(config->arena, &config->export_symbol_list, export_parse);
    hash_map_push_string_raw(config->arena, &config->export_ht, export_name, exp_n);
  } else {
    // merge duplicate export
    PE_ExportParse *extant_export = &exp_n->data;
    B32 alias_conflict = extant_export->alias.size &&
                         export_parse.alias.size &&
                         !str8_match(extant_export->alias, export_parse.alias, 0);
    B32 ordinal_conflict = extant_export->ordinal != export_parse.ordinal;

    if (alias_conflict || ordinal_conflict) {
      lnk_error_obj(LNK_Error_IllExport, obj, "ambiguous symbol export %S", export_parse.name);
    }

    if (!alias_conflict && !ordinal_conflict && extant_export->alias.size == 0 && export_parse.alias.size != 0) {
      extant_export->alias = export_parse.alias;
    }
  }
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

typedef struct LNK_SectionDirectiveAttr
{
  U8                code;
  COFF_SectionFlags flag;
  B32               is_mem_attr;
  B32               negated_sets;
} LNK_SectionDirectiveAttr;

global read_only LNK_SectionDirectiveAttr g_section_directive_attr_map[] =
{
  { 'D', COFF_SectionFlag_MemDiscardable, 0, 0 },
  { 'E', COFF_SectionFlag_MemExecute,     1, 0 },
  { 'K', COFF_SectionFlag_MemNotCached,   0, 1 },
  { 'P', COFF_SectionFlag_MemNotPaged,    0, 1 },
  { 'R', COFF_SectionFlag_MemRead,        1, 0 },
  { 'S', COFF_SectionFlag_MemShared,      0, 0 },
  { 'W', COFF_SectionFlag_MemWrite,       1, 0 },
};

typedef struct LNK_GuardOption
{
  String8        name;
  LNK_GuardFlags set_flags;
  LNK_GuardFlags clear_flags;
} LNK_GuardOption;

global read_only LNK_GuardOption g_guard_option_table[] =
{
  { str8_lit_comp("cf"),       LNK_Guard_Cf,      0                 },
  { str8_lit_comp("nocf"),     0,                 LNK_Guard_Cf      },
  { str8_lit_comp("longjmp"),  LNK_Guard_LongJmp, 0                 },
  { str8_lit_comp("nolongjmp"), 0,                LNK_Guard_LongJmp },
  { str8_lit_comp("ehcont"),   LNK_Guard_EhCont,  0                 },
  { str8_lit_comp("noehcont"), 0,                 LNK_Guard_EhCont  },
  { str8_lit_comp("no"),       0,                 LNK_Guard_All     },
};

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

internal COFF_SectionFlags
lnk_apply_section_directives_to_flags(LNK_Config *config, String8 full_section_name, COFF_SectionFlags flags)
{
  String8 section_name = {0};
  String8 sort_idx     = {0};
  coff_parse_section_name(full_section_name, &section_name, &sort_idx);

  for EachNode(dir_n, LNK_SectionDirectiveNode, config->section_list.first) {
    LNK_SectionDirective *dir = &dir_n->v;
    if (str8_match(dir->name, section_name, 0)) {
      flags &= ~dir->clear_flags;
      flags |= dir->set_flags;
    }
  }

  return flags;
}

internal String8
lnk_get_image_name(LNK_Config *config)
{
  String8 image_path = config->image_alt_path.size ? config->image_alt_path : config->out_path;
  String8 image_name = str8_skip_last_slash(image_path);
  return image_name;
}

internal String8
lnk_get_pdb_name(LNK_Config *config)
{
  String8 pdb_path = config->pdb_alt_path.size ? config->pdb_alt_path : config->pdb_name;
  String8 pdb_name = str8_skip_last_slash(pdb_path);
  return pdb_name;
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
    if (config->file_characteristics & PE_ImageFileCharacteristic_DLL) {
      base_addr = coff_default_dll_base_from_machine(config->machine);
    } else if (config->file_characteristics & PE_ImageFileCharacteristic_EXECUTABLE_IMAGE) {
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
  return hash_map_search_path_u64(&config->delay_load_ht, dll_name) != 0;
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
  hash_map_push_path_u64(config->arena, &config->disallow_lib_ht, lib_name, 1);
}

internal B32
lnk_is_lib_disallowed(LNK_Config *config, String8 path)
{
  String8 lib_name = lnk_get_lib_name(path);
  return hash_map_search_path_u64(&config->disallow_lib_ht, lib_name) != 0;
}

internal void
lnk_include_symbol(LNK_Config *config, String8 name, LNK_Obj *obj)
{
  // is this a duplicate symbol?
  if (hash_map_search_string_raw(&config->include_symbol_ht, name)) {
    return;
  }

  name = push_str8_copy(config->arena, name);

  LNK_IncludeSymbolNode *node = push_array(config->arena, LNK_IncludeSymbolNode, 1);
  node->v.name = name;
  node->v.obj  = obj;

  SLLQueuePush(config->include_symbol_list.first, config->include_symbol_list.last, node);
  config->include_symbol_list.count += 1;

  hash_map_push_string_raw(config->arena, &config->include_symbol_ht, name, node);
}

internal void
lnk_whole_archive(LNK_Config *config, String8 lib_name)
{
  lnk_apply_cmd_option_to_config(config, str8_lit("wholearchive"), lib_name, 0);
}

internal void
lnk_print_build_info()
{
  lnk_fprintf(stdout, "  Compiler: %s\n", COMPILER_STRING);
  lnk_fprintf(stdout, "  Mode    : %s\n", BUILD_MODE_STRING);
  lnk_fprintf(stdout, "  Date    : %s %s\n", __TIME__, __DATE__);
  lnk_fprintf(stdout, "  Version : %s\n", BUILD_VERSION_STRING_LITERAL);
}

internal void
lnk_print_help(void)
{
  Temp scratch = scratch_begin(0,0);

  static char spaces[] = "                                                                                   ";
  U64 name_max_size = 0; 
  U64 args_max_size = 0;
  U64 desc_max_size = 0;
  for EachElement(i, g_cmd_switch_map) {
    name_max_size = Max(name_max_size, strlen(g_cmd_switch_map[i].name));
    args_max_size = Max(args_max_size, strlen(g_cmd_switch_map[i].args));
    desc_max_size = Max(desc_max_size, strlen(g_cmd_switch_map[i].desc));
  }

  lnk_fprintf(stdout, "--- Help -------------------------------------------------------------------------------------------------------------------\n");
  lnk_fprintf(stdout, "  %s\n", BUILD_TITLE_STRING_LITERAL);
  lnk_fprintf(stdout, "\n");
  lnk_fprintf(stdout, "  Usage: radlink.exe [Options] [Files] [@rsp]\n");
  lnk_fprintf(stdout, "\n");

  lnk_fprintf(stdout, "  Options:\n");
  U64 option_indent_size   = 4;
  U64 option_name_max_size = 60;
  for EachElement(i, g_cmd_switch_map) {
    Temp temp = temp_begin(scratch.arena);

    U64 name_size = strlen(g_cmd_switch_map[i].name);
    U64 args_size = strlen(g_cmd_switch_map[i].args);
    U64 desc_size = strlen(g_cmd_switch_map[i].desc);

    char *name = g_cmd_switch_map[i].name;
    char *args = g_cmd_switch_map[i].args;
    char *desc = g_cmd_switch_map[i].desc;
    LNK_CmdSwitchType type = g_cmd_switch_map[i].type;

    if (strcmp(name, "") == 0 || strcmp(name, "NOT_IMPLEMENTED") == 0 || type == LNK_CmdSwitch_Help) {
      continue;
    }

    String8List fmt = {0};

    str8_list_pushf(temp.arena, &fmt, "%.*s", option_indent_size, spaces);

    str8_list_pushf(temp.arena, &fmt, "/");

    B32 put_option_args_on_new_line = name_size + args_size > option_name_max_size;
    if (put_option_args_on_new_line) {
      str8_list_pushf(temp.arena, &fmt, "%s%s", name, args[0] == ':' ? ":" : "");
    } else {
      str8_list_pushf(temp.arena, &fmt, "%s%s", name, args);
    }

    U64 desc_indent_size = fmt.total_size < option_name_max_size ? option_name_max_size - fmt.total_size : 0;
    str8_list_pushf(temp.arena, &fmt, "%.*s", desc_indent_size, spaces);

    str8_list_pushf(temp.arena, &fmt, "%s", desc);
    str8_list_pushf(temp.arena, &fmt, "\n");

    if (put_option_args_on_new_line) {
      str8_list_pushf(temp.arena, &fmt, "%.*s", Min(option_name_max_size, option_indent_size + name_size + 1), spaces);
      str8_list_pushf(temp.arena, &fmt, "%s\n", args[0] == ':' ? args + 1 : args);
    }

    String8 line = str8_list_join(temp.arena, &fmt, 0);
    lnk_fprintf(stdout, "%.*s", str8_varg(line));

    temp_end(temp);
  }

  lnk_fprintf(stdout, "\n");

  scratch_end(scratch);
}

internal void
lnk_apply_write_temp_files(Arena *arena, LNK_Config *config)
{
  if (config->map_name.size) {
    config->temp_map_name = push_str8f(arena, "%S.tmp%x", config->map_name, config->time_stamp);
  }
  if (config->out_path.size) {
    config->temp_out_path = push_str8f(arena, "%S.tmp%x", config->out_path, config->time_stamp);
  }
  if (config->pdb_name.size) {
    config->temp_pdb_name = push_str8f(arena, "%S.tmp%x", config->pdb_name, config->time_stamp);
  }
  if (config->rad_debug_name.size) {
    config->temp_rad_debug_name = push_str8f(arena, "%S.tmp%x", config->rad_debug_name, config->time_stamp);
  }
  if (config->type_server_name.size) {
    config->temp_type_server_name = push_str8f(arena, "%S.tmp%x", config->type_server_name, config->time_stamp); 
  }
}

internal void lnk_apply_def_file_to_config(LNK_Config *config, String8 path, LNK_Obj *obj);

internal void
lnk_apply_cmd_option_to_config(LNK_Config *config, String8 cmd_name, String8 value, LNK_Obj *obj)
{
  Temp scratch = scratch_begin(&config->arena, 1);

  LNK_CmdSwitch     *cmd_switch_info = lnk_cmd_switch_from_string(cmd_name);
  LNK_CmdSwitchType  cmd_switch      = cmd_switch_info ? cmd_switch_info->type : LNK_CmdSwitch_Null;
  String8List        values          = {0};
  if (cmd_switch_info && cmd_switch_info->value_kind == LNK_CmdValueKind_List) {
    values = str8_split_by_string_chars(scratch.arena, value, str8_lit(","), 0);
  }

  switch (cmd_switch) {
  case LNK_CmdSwitch_Null: {
    // Unknown /RAD_* switches on the command line warn and are ignored: the
    // RAD_ namespace is owned by this linker, but newer build scripts must
    // keep working against older radlink binaries (forward compatibility),
    // so an unrecognized /RAD_* switch must not fail the link. Use
    // LNK_Warning_Cmdl so the warning stays visible even though the
    // release-default /RAD_IGNORE mutes LNK_Warning_UnknownSwitch.
    if (obj == 0 && str8_match_lit("RAD_", str8_prefix(cmd_name, 4), StringMatchFlag_CaseInsensitive)) {
      lnk_error(LNK_Warning_Cmdl, "unknown switch \"/%S%s%S\"; this radlink build does not support it -- switch ignored", cmd_name, value.size ? ":" : "", value);
    } else {
      lnk_error_obj(LNK_Warning_UnknownSwitch, obj, "unknown switch: \"/%S%s%S\"", cmd_name, value.size ? ":" : "", value);
    }
  } break;

  default: break;

  case LNK_CmdSwitch_NotImplemented: {
    lnk_not_implemented("switch \"%S\" is not implemented \"%S\"", cmd_name, value);
  } break;

  case LNK_CmdSwitch_Align: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &config->sect_align, LNK_ParseU64Flag_CheckPow2);
  } break;

  case LNK_CmdSwitch_AllowBind: {
    lnk_cmd_switch_set_flag_inv_16(obj, cmd_switch, value, &config->dll_characteristics, PE_DllCharacteristic_NO_BIND);
  } break;

  case LNK_CmdSwitch_AllowIsolation: {
    lnk_cmd_switch_set_flag_inv_16(obj, cmd_switch, value, &config->dll_characteristics, PE_DllCharacteristic_NO_ISOLATION);
  } break;

  case LNK_CmdSwitch_AlternateName: {
    if (value.size > 0) {
      LNK_AltName alt_name;
      if (lnk_parse_alt_name_directive(value, obj, &alt_name)) {
        String8 *to_extant = hash_map_search_string_string(&config->alt_name_ht, alt_name.from);
        if (to_extant) {
          if (str8_match(*to_extant, alt_name.to, 0)) {
            // ignore, duplicate
          } else {
            lnk_error_obj(LNK_Error_AlternateNameConflict, obj, "conflicting alternative name: existing '%S=%S' vs. new '%S=%S'", alt_name.from, *to_extant, alt_name.from, alt_name.to);
          }
        } else {
          alt_name.from = push_str8_copy(config->arena, alt_name.from);
          alt_name.to   = push_str8_copy(config->arena, alt_name.to);

          lnk_alt_name_list_push(config->arena, &config->alt_name_list, alt_name);
          if (str8_ends_with(alt_name.from, str8_lit("$fo$"), 0)) {
            lnk_alt_name_list_push(config->arena, &config->function_override_list, alt_name);
          }
          hash_map_push_string_string(config->arena, &config->alt_name_ht, alt_name.from, alt_name.to);
        }
      }
    } else {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
    }
  } break;

  case LNK_CmdSwitch_AppContainer: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value, &config->dll_characteristics, PE_DllCharacteristic_APPCONTAINER);
  } break;

  case LNK_CmdSwitch_Base: {
    if (values.node_count == 2) {
      String8Node *first_node = values.first;
      //String8Node *second_node = first_node->next;
      B32 is_response_file = str8_match_lit("@", first_node->string, StringMatchFlag_RightSideSloppy);
      if (is_response_file) {
        //String8 file_path = first_node->string;
        //String8 tag = second_node->string;
        lnk_not_implemented("Response files are not implemented for /BASE");
      } else {
        Rng1U64 addr_size = {0};
        if (lnk_cmd_switch_parse_tuple(obj, cmd_switch, values, &addr_size)) {
          config->user_base_addr = addr_size.v[0];
          config->max_image_size = addr_size.v[1];
        }
      }
    } else if (values.node_count == 1) {
      U64 addr;
      if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &addr, 0)) {
        config->user_base_addr = addr;
      }
    } else if (values.node_count == 0) {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "expected at least 1 parameter");
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "too many parameters");
    }
  } break;

  case LNK_CmdSwitch_Debug: {
    if (value.size == 0) {
      config->debug_mode = LNK_DebugMode_Full;
    } else {
      LNK_DebugMode debug_mode = lnk_debug_mode_from_string(value);
      if (debug_mode == LNK_DebugMode_GHash) {
        config->debug_mode = LNK_DebugMode_Full;
        config->ghash = 1;
      } else if (debug_mode == LNK_DebugMode_FastLink) {
        config->debug_mode = LNK_DebugMode_Full;
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "FASTLINK is not supported, switching to FULL");
      } else if (debug_mode != LNK_DebugMode_Null) {
        config->debug_mode = debug_mode;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid parameter \"%S\"", value);
      }
    }
  } break;

  case LNK_CmdSwitch_DefaultLib: {
    if (value.size > 0) {
      String8 default_lib = push_str8_copy(config->arena, value);
      if (obj) {
        str8_list_push(config->arena, &config->input_obj_lib_list, default_lib);
      } else {
        str8_list_push(config->arena, &config->input_default_lib_list, default_lib);
      }
    }
  } break;

  case LNK_CmdSwitch_Def: {
    String8 path = {0};
    if (lnk_cmd_switch_parse_string(obj, cmd_switch, value, &path)) {
      lnk_apply_def_file_to_config(config, path, obj);
    }
  } break;

  case LNK_CmdSwitch_Delay: {
    if (value.size == 0) {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
    } else {
      if (str8_match_lit("unload", value, StringMatchFlag_CaseInsensitive)) {
        config->import_table_emit_uiat = LNK_SwitchState_Yes;
      } else if (str8_match_lit("nobind", value, StringMatchFlag_CaseInsensitive)) {
        config->import_table_emit_biat = LNK_SwitchState_No;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown parameter \"%S\"", value);
      }
    }
  } break;

  case LNK_CmdSwitch_DelayLoad: {
    if (value.size > 0 && !hash_map_search_path_u64(&config->delay_load_ht, value)) {
      String8 name = push_str8_copy(config->arena, value);
      hash_map_push_path_u64(config->arena, &config->delay_load_ht, name, 1);
      str8_list_push(config->arena, &config->delay_load_dll_list, name);
    }
  } break;

  case LNK_CmdSwitch_Dll: {
    config->file_characteristics |= PE_ImageFileCharacteristic_DLL;
  } break;

  case LNK_CmdSwitch_DynamicBase: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value, &config->dll_characteristics, PE_DllCharacteristic_DYNAMIC_BASE);
  } break;

  case LNK_CmdSwitch_Entry: {
    String8 new_entry_point_name = {0};
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &new_entry_point_name);

    if (config->entry_point_name.size) {
      lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "unable to redefine entry point \"%S\" to \"%S\"", config->entry_point_name, new_entry_point_name);
      break;
    }

    config->entry_point_name = new_entry_point_name;
  } break;

  case LNK_CmdSwitch_Export: {
    PE_ExportParse export_parse = {0};
    if (lnk_parse_export_directive_ex(config->arena, values, obj, &export_parse)) {
      lnk_push_export_to_config(config, obj, export_parse);
    }
  } break;

  case LNK_CmdSwitch_FailIfMismatch: {
    if (value.size == 0) {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
      break;
    }

    LNK_AltName dir;
    if ( ! lnk_parse_alt_name_directive(value, obj, &dir)) {
      break;
    }

    LNK_AltName *current = hash_map_search_string_raw(&config->fail_if_mismatch_ht, dir.from);
    if (current) {
      if ( ! str8_match(current->to, dir.to, 0)) {
        lnk_error_cmd_switch(LNK_Error_FailIfMismatch, obj, cmd_switch,
                      "'%S=%S' mismatch in:\n"
                      "  %S: /FAILIFMISMATCH:%S=%S\n",
                      dir.from, dir.to,
                      lnk_loc_from_obj(scratch.arena, current->obj), current->from, current->to);
        break;
      }
    } else {
      LNK_AltName *n = push_array(config->arena, LNK_AltName, 1);
      n->from = push_str8_copy(config->arena, dir.from);
      n->to   = push_str8_copy(config->arena, dir.to);
      n->obj  = obj;
      hash_map_push_string_raw(config->arena, &config->fail_if_mismatch_ht, n->from, n);
    }
  } break;

  case LNK_CmdSwitch_FileAlign: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &config->file_align, LNK_ParseU64Flag_CheckPow2);
  } break;

  case LNK_CmdSwitch_Fixed: {
    lnk_cmd_switch_set_flag_64(obj, cmd_switch, value, &config->flags, LNK_ConfigFlag_Fixed);
  } break;

  case LNK_CmdSwitch_FunctionPadMin: {
    if (value.size == 0) {
      config->function_pad_min       = 0;
      config->infer_function_pad_min = 1;
    } else {
      lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &config->function_pad_min, LNK_ParseU64Flag_CheckUnder32bit);
    }
    config->do_function_pad_min = LNK_SwitchState_Yes;
  } break;

  case LNK_CmdSwitch_Guard: {
    for EachNode(n, String8Node, values.first) {
      LNK_GuardOption *option = 0;
      for EachElement(i, g_guard_option_table) {
        if (str8_matchi(g_guard_option_table[i].name, n->string)) {
          option = &g_guard_option_table[i];
          break;
        }
      }

      if (option != 0) {
        config->guard_flags &= ~option->clear_flags;
        config->guard_flags |= option->set_flags;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown option \"%S\"", n->string);
      }
    }
  } break;

  case LNK_CmdSwitch_Heap: {
    Rng1U64 reserve_commit;
    reserve_commit.v[0] = config->heap_reserve;
    reserve_commit.v[1] = config->heap_commit;
    if (lnk_cmd_switch_parse_tuple(obj, cmd_switch, values, &reserve_commit)) {
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
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value, &config->dll_characteristics, PE_DllCharacteristic_HIGH_ENTROPY_VA);
  } break;

  case LNK_CmdSwitch_Ignore: {
    U64 error_code;
    if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &error_code, 0)) {
      switch (error_code) {
      case LNK_MsWarningCode_UnsuedDelayLoadDll: {
        lnk_ignore_error(LNK_Warning_UnusedDelayLoadDll);
      } break;
      case LNK_MsWarningCode_MissingExternalTypeServer: {
        lnk_ignore_error(LNK_Warning_MissingExternalTypeServer);
      } break;
      case LNK_MsWarningCode_SectionFlagsConflict: {
        lnk_ignore_error(LNK_Warning_SectionFlagsConflict);
      } break;
      default: {
        #if BUILD_DEBUG
        lnk_not_implemented("TODO: /IGNORE:%llu", error_code);
        #endif
      } break;
      }
    }
  } break;

  case LNK_CmdSwitch_ImpLib: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->imp_lib_name);
  } break;

  case LNK_CmdSwitch_Include: {
    if (value.size > 0) {
      lnk_include_symbol(config, value, obj);
    }
  } break;

  case LNK_CmdSwitch_InferAsanLibs: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &config->infer_asan_libs);
  } break;

  case LNK_CmdSwitch_LargeAddressAware: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value, &config->file_characteristics, PE_ImageFileCharacteristic_LARGE_ADDRESS_AWARE);
  } break;

  case LNK_CmdSwitch_Lib: {
    lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unsupported switch; static library is created by passing /LIB to link.exe");
  } break;

  case LNK_CmdSwitch_LibPath: {
    if (value.size > 0) {
      String8 dir = push_str8_copy(config->arena, value);
      if (!folder_path_exists(dir)) {
        String8 full_path = full_path_from_path(scratch.arena, dir);
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "path doesn't exist %S", full_path);
      }
      str8_list_push(config->arena, &config->lib_dir_list, dir);
    }
  } break;

  case LNK_CmdSwitch_Machine: {
    if (value.size > 0) {
      COFF_MachineType machine = coff_machine_from_string(value);
      if (machine != COFF_MachineType_Unknown) {
        config->machine = machine;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown parameter \"%S\"", value);
      }
    } else {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
    }
  } break;

  case LNK_CmdSwitch_Manifest: {
    if (values.node_count > 0) {
      String8Array param_arr = str8_array_from_list(scratch.arena, &values);
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
    } else {
      config->manifest_opt = LNK_ManifestOpt_WriteToFile;
    }
  } break;

  case LNK_CmdSwitch_ManifestDependency: {
    if (value.size > 0) {
      str8_list_push(config->arena, &config->manifest_dependency_list, push_str8_copy(config->arena, value));
    }

    if (config->manifest_opt == LNK_ManifestOpt_Null) {
      config->manifest_opt = LNK_ManifestOpt_WriteToFile;
    }
  } break;

  case LNK_CmdSwitch_ManifestFile: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->manifest_name);
  } break;

  case LNK_CmdSwitch_ManifestInput: {
    // see :manifest_input
  } break;

  case LNK_CmdSwitch_ManifestUac: {
    if (value.size > 0) {
      String8 uac = lnk_error_check_and_strip_quotes(LNK_Error_Cmdl, obj, cmd_switch, value);
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
    if (value.size > 0) {
      LNK_MergeDirective merge = {0};
      if (lnk_parse_merge_directive(value, obj, &merge)) {
        merge.src = push_str8_copy(config->arena, merge.src);
        merge.dst = push_str8_copy(config->arena, merge.dst);
        lnk_merge_directive_list_push(config->arena, &config->merge_list, merge);
      }
    } else {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
    }
  } break;

  case LNK_CmdSwitch_Natvis: {
    if (value.size > 0) {
      // warn about invalid natvis extension
      String8 ext = str8_skip_last_dot(value);
      if (!str8_match_lit("natvis", ext, StringMatchFlag_CaseInsensitive)) {
        lnk_error_cmd_switch(LNK_Warning_InvalidNatvisFileExt, obj, cmd_switch, "Visual Studio expects .natvis extension: \"%S\"", value);
      }

      str8_list_push(config->arena, &config->natvis_list, push_str8_copy(config->arena, value));
    }
  } break;

  case LNK_CmdSwitch_DisallowLib:
  case LNK_CmdSwitch_NoDefaultLib: {
    if (value.size == 0) {
      config->no_default_libs = 1;
    } else {
      String8 lib_name = lnk_get_lib_name(value);
      if (!hash_map_search_path_u64(&config->disallow_lib_ht, lib_name)) {
        hash_map_push_path_u64(config->arena, &config->disallow_lib_ht, lib_name, 1);
      }
    }
  } break;

  case LNK_CmdSwitch_NoExp: {
    config->build_exp = 0;
  } break;

  case LNK_CmdSwitch_NoImpLib: {
    config->build_imp_lib = 0;
  } break;

  case LNK_CmdSwitch_NxCompat: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value, &config->dll_characteristics, PE_DllCharacteristic_NX_COMPAT);
  } break;

  case LNK_CmdSwitch_Opt: {
    for (String8Node *n = values.first; n != 0; n = n->next) {
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
      } else if (str8_match_lit("icfstatic", param, StringMatchFlag_CaseInsensitive)) {
        // compatibility: build systems that drove the fork's /OPT:ICFSTATIC. Internal-linkage
        // COMDATs are always fold candidates here, so this is plain ICF.
        config->opt_icf = LNK_SwitchState_Yes;
      } else if (str8_match_lit("noicf", param, StringMatchFlag_CaseInsensitive)) {
        config->opt_icf = LNK_SwitchState_No;
      } else if (str8_match_lit("lbr", param, StringMatchFlag_CaseInsensitive)) {
        config->opt_lbr = LNK_SwitchState_Yes;
      } else if (str8_match_lit("nolibr", param, StringMatchFlag_CaseInsensitive)) {
        config->opt_lbr = LNK_SwitchState_No;
      } else if (str8_match_lit("gctypes", param, StringMatchFlag_CaseInsensitive)) {
        config->opt_gc_types = LNK_SwitchState_Yes;
      } else if (str8_match_lit("nogctypes", param, StringMatchFlag_CaseInsensitive)) {
        config->opt_gc_types = LNK_SwitchState_No;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown option \"%S\"", param);
      }
    }
  } break;

  case LNK_CmdSwitch_Out: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->out_path);
  } break;

  case LNK_CmdSwitch_Pdb: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->pdb_name);
  } break;

  case LNK_CmdSwitch_PdbAltPath: {
    // see :PdbAltPath
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->pdb_alt_path);
  } break;

  case LNK_CmdSwitch_PdbPageSize: {
    U64 page_size;
    if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &page_size, LNK_ParseU64Flag_CheckPow2)) {
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

  case LNK_CmdSwitch_PdbStripped: {
    String8 file_name;
    if (lnk_cmd_switch_parse_string(obj, cmd_switch, value, &file_name)) {
      config->pdb_stripped_name = str8_copy(config->arena, file_name);
    }
  } break;

  case LNK_CmdSwitch_Release: {
    if (value.size == 0) {
      config->flags |= LNK_ConfigFlag_WriteImageChecksum;
    } else {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
    }
  } break;

  case LNK_CmdSwitch_Section: {
    LNK_SectionDirective section_dir = {0};
    B32 is_parse_ok = 1;

    if (values.node_count < 2) {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, LNK_CmdSwitch_Section, "expected section name and attributes");
      is_parse_ok = 0;
    } else {
      section_dir.name = values.first->string;

      B32 has_attr     = 0;
      B32 has_mem_attr = 0;
      for (String8Node *param_n = values.first->next; param_n != 0; param_n = param_n->next) {
        String8 param = param_n->string;

        if (str8_match_lit("ALIGN=", param, StringMatchFlag_CaseInsensitive|StringMatchFlag_RightSideSloppy)) {
          String8 align_string = str8_skip(param, sizeof("ALIGN=") - 1);
          U64 align = 0;
          if (try_u64_from_str8_c_rules(align_string, &align)) {
            COFF_SectionFlags align_flag = coff_section_flag_from_align_size(align);
            if (align_flag) {
              COFF_SectionFlags align_mask = (COFF_SectionFlag_AlignMask << COFF_SectionFlag_AlignShift);
              section_dir.clear_flags |= align_mask;
              section_dir.set_flags   &= ~align_mask;
              section_dir.set_flags   |= align_flag;
              section_dir.clear_flags &= ~align_flag;
            } else {
              lnk_error_cmd_switch(LNK_Error_Cmdl, obj, LNK_CmdSwitch_Section, "invalid alignment \"%S\"", align_string);
              is_parse_ok = 0;
            }
          } else {
            lnk_error_cmd_switch(LNK_Error_Cmdl, obj, LNK_CmdSwitch_Section, "unable to parse alignment \"%S\"", align_string);
            is_parse_ok = 0;
          }
          continue;
        }

        B32 negate = 0;
        for EachIndex(i, param.size) {
          U8 c = upper_from_char(param.str[i]);
          if (c == '!') {
            negate = 1;
            continue;
          }

          LNK_SectionDirectiveAttr *attr = 0;
          for EachElement(attr_idx, g_section_directive_attr_map) {
            if (g_section_directive_attr_map[attr_idx].code == c) {
              attr = &g_section_directive_attr_map[attr_idx];
              break;
            }
          }

          if (attr == 0) {
            lnk_error_cmd_switch(LNK_Error_Cmdl, obj, LNK_CmdSwitch_Section, "unknown section attribute '%c' in \"%S\"", c, param);
            is_parse_ok = 0;
          } else {
            has_attr     = 1;
            has_mem_attr = has_mem_attr || attr->is_mem_attr;

            COFF_SectionFlags flags = (negate == attr->negated_sets) ? attr->flag : 0;
            section_dir.clear_flags |= attr->flag;
            section_dir.set_flags   &= ~attr->flag;
            section_dir.set_flags   |= flags;
            section_dir.clear_flags &= ~flags;
          }

          negate = 0;
        }

        if (negate) {
          lnk_error_cmd_switch(LNK_Error_Cmdl, obj, LNK_CmdSwitch_Section, "dangling '!' in \"%S\"", param);
          is_parse_ok = 0;
        }
      }

      if (has_attr) {
        section_dir.clear_flags |= COFF_SectionFlag_MemDiscardable|COFF_SectionFlag_MemNotCached|COFF_SectionFlag_MemNotPaged|COFF_SectionFlag_MemShared;
        if (has_mem_attr) {
          section_dir.clear_flags |= COFF_SectionFlag_MemExecute|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite;
        }
        section_dir.clear_flags &= ~section_dir.set_flags;
      }
    }

    if (is_parse_ok) {
      section_dir.name = push_str8_copy(config->arena, section_dir.name);

      LNK_SectionDirectiveNode *node = push_array_no_zero(config->arena, LNK_SectionDirectiveNode, 1);
      node->v = section_dir;
      SLLQueuePush(config->section_list.first, config->section_list.last, node);
      config->section_list.count += 1;
    }
  } break;

  case LNK_CmdSwitch_Stack: {
    Rng1U64 reserve_commit;
    reserve_commit.v[0] = config->stack_reserve;
    reserve_commit.v[1] = config->stack_commit;
    if (lnk_cmd_switch_parse_tuple(obj, cmd_switch, values, &reserve_commit)) {
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
    if (values.node_count <= 2 && values.node_count > 0) {
      // set subsystem type
      PE_WindowsSubsystem subsystem = pe_subsystem_from_string(values.first->string);
      if (subsystem != PE_WindowsSubsystem_UNKNOWN) {
        if (config->subsystem != PE_WindowsSubsystem_UNKNOWN) {
          if (config->subsystem != subsystem) {
            lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "overriding subystem \"%S\" with \"%S\"",
                                 pe_string_from_subsystem(config->subsystem),
                                 pe_string_from_subsystem(subsystem));
          }
        }
        config->subsystem = subsystem;

        // parse version (optional)
        if (values.node_count == 2) {
          lnk_cmd_switch_parse_version(obj, cmd_switch, values.last->string, &config->subsystem_ver);
        }
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid subsystem \"%S\"", values.first->string);
      }
    } else {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
    }
  } break;

  case LNK_CmdSwitch_Time: {
  } break;

  case LNK_CmdSwitch_TsAware: {
    lnk_cmd_switch_set_flag_inv_64(obj, cmd_switch, value, &config->flags, LNK_ConfigFlag_NoTsAware);
  } break;

  case LNK_CmdSwitch_Version: {
    lnk_cmd_switch_parse_version(obj, cmd_switch, value, &config->image_ver);
  } break;

  case LNK_CmdSwitch_WholeArchive: {
    if (value.size == 0) {
      config->whole_archive_all = 1;
    } else {
      String8 lib_name;
      if (lnk_cmd_switch_parse_string(obj, cmd_switch, value, &lib_name)) {
        lib_name = str8_chop_last_dot(str8_skip_last_slash(lib_name));
        hash_map_push_path_u64(config->arena, &config->whole_archive_ht, lib_name, 1);
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_Age: {
    lnk_cmd_switch_parse_u32(obj, cmd_switch, value, &config->age, 0);
  } break;

  //case LNK_CmdSwitch_Rad_BuildExp: {
  //  LNK_SwitchState state;
  //  if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &state)) {
  //    config->build_exp = (state == LNK_SwitchState_Yes);
  //  }
  //} break;

  case LNK_CmdSwitch_Rad_BuildInfo: {
    lnk_print_build_info();
    abort_self(0);
  } break;

  case LNK_CmdSwitch_Rad_BuildImpLib: {
    LNK_SwitchState state;
    if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &state)) {
      config->build_imp_lib = (state == LNK_SwitchState_Yes);
    }
  } break;

  case LNK_CmdSwitch_Rad_CheckUnusedDelayLoadDll: {
    lnk_cmd_switch_set_flag_64(obj, cmd_switch, value, &config->flags, LNK_ConfigFlag_CheckUnusedDelayLoadDll);
  } break;

  case LNK_CmdSwitch_Rad_DataDirCount: {
    U64 data_dir_count = 0;
    if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &data_dir_count, LNK_ParseU64Flag_CheckUnder32bit)) {
      if (1 <= data_dir_count && data_dir_count <= PE_DataDirectoryIndex_COUNT) {
        config->data_dir_count = data_dir_count;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid data directory count %llu, expected 1 through %u", data_dir_count, PE_DataDirectoryIndex_COUNT);
      }
    }
  } break;

  case LNK_CmdSwitch_Map: {
    config->map = LNK_SwitchState_Yes;
    if (value.size) {
      lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->map_name);
    }
  } break;

  case LNK_CmdSwitch_Rad_MapLinesForUnresolvedSymbols: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &config->map_lines_for_unresolved_symbols);
  } break;

  case LNK_CmdSwitch_Rad_MemoryMapFiles: {
    if (value.size == 0) {
      config->io_flags &= ~LNK_IO_Flags_MemoryMapFilesReadWrite;
      config->io_flags |=  LNK_IO_Flags_MemoryMapFilesReadOnly;
    } else {
      if (str8_matchi(value, str8_lit("no"))) {
        config->io_flags &= ~(LNK_IO_Flags_MemoryMapFilesReadOnly|LNK_IO_Flags_MemoryMapFilesReadWrite);
      } else if (str8_matchi(value, str8_lit("yes")) || str8_matchi(value, str8_lit("read_only"))) {
        config->io_flags &= ~LNK_IO_Flags_MemoryMapFilesReadWrite;
        config->io_flags |=  LNK_IO_Flags_MemoryMapFilesReadOnly;
      } else if (str8_matchi(value, str8_lit("read_write"))) {
        config->io_flags &= ~LNK_IO_Flags_MemoryMapFilesReadOnly;
        config->io_flags |=  LNK_IO_Flags_MemoryMapFilesReadWrite;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid parameter: \"%S\", expected NO, READ_ONLY, or READ_WRITE", value);
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_BootMode: {
    if (value.size > 0) {
      if (str8_matchi(value, str8_lit("linker"))) {
        config->boot_mode = LNK_BootMode_Linker;
      } else if (str8_matchi(value, str8_lit("type_server"))) {
        config->boot_mode = LNK_BootMode_TypeServer;
      } else {
        lnk_error_cmd_switch(LNK_Error_Boot, obj, cmd_switch, "unknown value: \"%S\".", value);
      }
    } else {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Boot, obj, cmd_switch);
    }
  } break;

  case LNK_CmdSwitch_Rad_Debug: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &config->rad_debug);
  } break;

  case LNK_CmdSwitch_Rad_DebugName: {
    // :Rad_DebugAltPath
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->rad_debug_name);
  } break;

  case LNK_CmdSwitch_Rad_DebugAltPath: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->rad_debug_alt_path);
  } break;

  case LNK_CmdSwitch_Rad_DelayBind: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &config->import_table_emit_biat);
  } break;

  case LNK_CmdSwitch_Rad_DoMerge: {
    lnk_cmd_switch_set_flag_64(obj, cmd_switch, value, &config->flags, LNK_ConfigFlag_Merge);
  } break;

  case LNK_CmdSwitch_Rad_EnvLib: {
    lnk_cmd_switch_set_flag_64(obj, cmd_switch, value, &config->flags, LNK_ConfigFlag_EnvLib);
  } break;

  case LNK_CmdSwitch_Rad_Exe: {
    lnk_cmd_switch_set_flag_16(obj, cmd_switch, value, &config->file_characteristics, PE_ImageFileCharacteristic_EXECUTABLE_IMAGE);
  } break;

  case LNK_CmdSwitch_Rad_Guid: {
    if (value.size > 0) {
      if (str8_match_lit("imageblake3", value, StringMatchFlag_CaseInsensitive)) {
        config->guid_type = Lnk_DebugInfoGuid_ImageBlake3;
      } else if (str8_match_lit("random", value, StringMatchFlag_CaseInsensitive)) {
        config->guid = make_guid();
      } else {
        Guid guid;
        if (try_guid_from_string(value, &guid)) {
          config->guid = guid;
        } else {
          lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse \"%S\"", value);
        }
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid number of parameters, expected GUID formatted as following: \"0000000-0000-0000-0000-000000000000\"");
    }
  } break;

  case LNK_CmdSwitch_Rad_LargePages: {
    if (value.size == 0) {
      ProcessInfo *process_info = get_process_info();
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
    } else {
      if (str8_match_lit("quiet", value, StringMatchFlag_CaseInsensitive)) {
        ProcessInfo *process_info = get_process_info();
        if (process_info->large_pages_allowed) {
          arena_default_flags |= ArenaFlag_LargePages;
        }
      } else if (str8_match_lit("no", value, StringMatchFlag_CaseInsensitive)) {
        arena_default_flags &= ~ArenaFlag_LargePages;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid parameter: \"%S\", expected NO or QUIET", value);
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_LinkVer: {
    lnk_cmd_switch_parse_version(obj, cmd_switch, value, &config->link_ver);
  } break;

  case LNK_CmdSwitch_Rad_Log: {
    if (value.size > 0) {
      B32 status = 1;
      if (str8_starts_with(value, str8_lit("-"))) {
        value = str8_skip(value, 1);
        status = 0;
      }

      if (str8_match_lit("all", value, StringMatchFlag_CaseInsensitive)) {
        for (U64 ilog = 0; ilog < LNK_Log_Count; ilog += 1) {
          lnk_set_log_status((LNK_LogType)ilog, status);
        }
      } else if (str8_match_lit("io", value, StringMatchFlag_CaseInsensitive)) {
        lnk_set_log_status(LNK_Log_IO_Read, status);
        lnk_set_log_status(LNK_Log_IO_Write, status);
      } else {
        LNK_LogType log_type = lnk_log_type_from_string(value);
        if (log_type == LNK_Log_Null) {
          lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown parameter \"%S\"", value);
        } else {
          lnk_set_log_status(log_type, status);
        }
      }
    } else {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
    }
  } break;

  case LNK_CmdSwitch_Rad_MtPath: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->mt_path);
  } break;

  case LNK_CmdSwitch_Rad_OsVer: {
    lnk_cmd_switch_parse_version(obj, cmd_switch, value, &config->os_ver);
  } break;

  case LNK_CmdSwitch_Rad_PageSize: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &config->machine_page_size, 0);
  } break;

  case LNK_CmdSwitch_Rad_PathStyle: {
    if (value.size > 0) {
      PathStyle path_style = path_style_from_string(value);
      if (path_style != PathStyle_Null) {
        config->path_style = path_style;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unable to parse parameter \"%S\"", value);
      }
    } else {
      lnk_error_cmd_switch_invalid_param_count(LNK_Error_Cmdl, obj, cmd_switch);
    }
  } break;

  case LNK_CmdSwitch_Rad_PdbHashTypeNames: {
    String8 mode_string = value;

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
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->pdb_hash_type_name_map);
  } break;

  case LNK_CmdSwitch_Rad_PdbHashTypeNameLength: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &config->pdb_hash_type_name_length, 0);
  } break;

  case LNK_CmdSwitch_Rad_RemoveSection: {
    String8 sect_name = {0};
    if (lnk_cmd_switch_parse_string(obj, cmd_switch, value, &sect_name)) {
      sect_name = push_str8_copy(config->arena, sect_name);
      str8_list_push(config->arena, &config->remove_sections, sect_name);
    }
  } break;

  case LNK_CmdSwitch_Rad_SharedThreadPool: {
    if (value.size == 0) {
      config->shared_thread_pool_name = str8_lit(LNK_DEFAULT_THREAD_POOL_NAME);
    } else {
      // NOTE: must copy into the config arena -- the parsed string points into
      // response-file/cmdline scratch that is freed long before late consumers
      // (pool init, summary) read it
      lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->shared_thread_pool_name);
      if (config->shared_thread_pool_name.size == 0) {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "invalid empty string for thread pool name");
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_SharedThreadPoolMaxWorkers: {
    SystemInfo *sysinfo = get_system_info();
    if (value.size == 0) {
      config->max_worker_count = sysinfo->logical_processor_count;
    } else {
      lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &config->max_worker_count, 0);
      if (config->max_worker_count == 0) {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "number of workers must be greater than zero");
      } else if (config->max_worker_count > sysinfo->logical_processor_count) {
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "number of workers %llu exceeds processor count %llu", config->max_worker_count, sysinfo->logical_processor_count);
        config->max_worker_count = sysinfo->logical_processor_count;
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_SortImports: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &config->sort_imports);
  } break;

  case LNK_CmdSwitch_Rad_Ignore: {
    S64List error_code_list = {0};
    if ( ! lnk_cmd_switch_parse_s64_list(scratch.arena, obj, cmd_switch, values, &error_code_list, 0)) {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "failed to parse input code");
      break;
    }

    for EachNode(error_code_n, S64Node, error_code_list.first) {
      S64 code = error_code_n->v;

      if (abs_s64(code) >= LNK_Error_Count) {
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, cmd_switch, "unknown error code %llu", (U64)abs_s64(code));
        continue;
      }

      if (code > 0) {
        lnk_ignore_error(code);
      } else if (code < 0) {
        lnk_activate_error(abs_s64(code));
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_ImageAltPath: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->image_alt_path);
  } break;

  case LNK_CmdSwitch_Rad_WriteTempFiles: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &config->write_temp_files);
    if (config->write_temp_files == LNK_SwitchState_Yes) {
      lnk_apply_write_temp_files(config->arena, config);
    }
  } break;

  case LNK_CmdSwitch_Rad_TimeStamp: {
    lnk_cmd_switch_parse_u32(obj, cmd_switch, value, &config->time_stamp, 0);
  } break;

  case LNK_CmdSwitch_Rad_IcfHashKind: {
    // TODO: dedup
    String8 alg = {0};
    if (lnk_cmd_switch_parse_string(obj, cmd_switch, value, &alg)) {
      if      (str8_matchi(alg, str8_lit("BLAKE3"))) { config->icf_hash_kind = LNK_HashKind_BLAKE3; }
      else if (str8_matchi(alg, str8_lit("XXHASH"))) { config->icf_hash_kind = LNK_HashKind_XXHash; }
      else { lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown hash alg: %S", alg); }
    }
  } break;

  case LNK_CmdSwitch_Rad_DebugTypeHash: {
    String8 alg = {0};
    if (lnk_cmd_switch_parse_string(obj, cmd_switch, value, &alg)) {
      if (str8_match(alg, str8_lit("BLAKE3"), StringMatchFlag_CaseInsensitive)) {
        config->debug_types_hash = LNK_HashKind_BLAKE3;
      } else if (str8_match(alg, str8_lit("XXHASH"), StringMatchFlag_CaseInsensitive)) {
        config->debug_types_hash = LNK_HashKind_XXHash;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "unknown hash alg: %S", alg);
      }
    }
  } break;

  case LNK_CmdSwitch_Rad_UnresolvedSymbolLimit: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &config->unresolved_symbol_limit, 0);
  } break;

  case LNK_CmdSwitch_Rad_UnresolvedSymbolRefLimit: {
    lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &config->unresolved_symbol_ref_limit, 0);
  } break;

  case LNK_CmdSwitch_Rad_Version: {
    lnk_fprintf(stdout, "%s\n", BUILD_TITLE_STRING_LITERAL);
    abort_self(0);
  } break;

  case LNK_CmdSwitch_Rad_Workers: {
    U64 worker_count;
    if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &worker_count, 0)) {
      config->worker_count = worker_count;
    }
  } break;

  case LNK_CmdSwitch_Rad_DebugWorkers: {
    U64 cap;
    if (lnk_cmd_switch_parse_u64(obj, cmd_switch, value, &cap, 0)) {
      config->debug_worker_cap = cap;
    }
  } break;


  case LNK_CmdSwitch_Rad_WorkDir: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->work_dir);
  } break;

  case LNK_CmdSwitch_Help: {
    lnk_print_help();
    abort_self(0);
  } break;

  case LNK_CmdSwitch_RadTypeServer: {
    lnk_cmd_switch_parse_string_copy(config->arena, obj, cmd_switch, value, &config->type_server_name);

    if (config->type_server_name.size) {
      String8 ext = str8_postfix(config->type_server_name, s("rrt").size);
      if (str8_matchi(ext, s("rrt"))) {
        config->boot_mode = LNK_BootMode_TypeServer;
      } else {
        lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "missing .rrt file extension after type server name %S", config->type_server_name);
      }
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, obj, cmd_switch, "missing type server file path");
    }
  } break;

  case LNK_CmdSwitch_LLVM_AddrSig: {
    lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &config->llvm_addrsig);
  } break;
  case LNK_CmdSwitch_IfcMap: {
    // collect .toml paths (header-unit -> .ifc); parsed lazily during debug-info build
    String8 path = {0};
    if (lnk_cmd_switch_parse_string(obj, cmd_switch, value, &path)) {
      str8_list_push(config->arena, &config->ifc_map_list, push_str8_copy(config->arena, path));
    }
  } break;
  case LNK_CmdSwitch_IfcDebugRecords: {
    LNK_SwitchState state = LNK_SwitchState_Null;
    if (lnk_cmd_switch_parse_flag(obj, cmd_switch, value, &state)) {
      config->ifc_debug_records = state;
    }
  } break;
  }

  scratch_end(scratch);
}

typedef enum LNK_DefFileStmt
{
  LNK_DefFileStmt_Null,
  LNK_DefFileStmt_Description,
  LNK_DefFileStmt_Exports,
  LNK_DefFileStmt_HeapSize,
  LNK_DefFileStmt_Imports,
  LNK_DefFileStmt_Library,
  LNK_DefFileStmt_Name,
  LNK_DefFileStmt_Sections,
  LNK_DefFileStmt_Segments,
  LNK_DefFileStmt_StackSize,
  LNK_DefFileStmt_Stub,
  LNK_DefFileStmt_Version,
} LNK_DefFileStmt;

global read_only struct
{
  String8         name;
  LNK_DefFileStmt stmt;
} g_def_file_stmt_map[] =
{
  { str8_lit_comp("DESCRIPTION"), LNK_DefFileStmt_Description },
  { str8_lit_comp("EXPORTS"),     LNK_DefFileStmt_Exports     },
  { str8_lit_comp("HEAPSIZE"),    LNK_DefFileStmt_HeapSize    },
  { str8_lit_comp("IMPORTS"),     LNK_DefFileStmt_Imports     },
  { str8_lit_comp("LIBRARY"),     LNK_DefFileStmt_Library     },
  { str8_lit_comp("NAME"),        LNK_DefFileStmt_Name        },
  { str8_lit_comp("SECTIONS"),    LNK_DefFileStmt_Sections    },
  { str8_lit_comp("SEGMENTS"),    LNK_DefFileStmt_Segments    },
  { str8_lit_comp("STACKSIZE"),   LNK_DefFileStmt_StackSize   },
  { str8_lit_comp("STUB"),        LNK_DefFileStmt_Stub        },
  { str8_lit_comp("VERSION"),     LNK_DefFileStmt_Version     },
};

typedef struct LNK_DefFileLineNode LNK_DefFileLineNode;
struct LNK_DefFileLineNode
{
  LNK_DefFileStmt     stmt;
  String8             line;
  B32                 is_keyword;
  LNK_DefFileLineNode *next;
};

typedef struct LNK_DefFileLineList LNK_DefFileLineList;
struct LNK_DefFileLineList
{
  U64                  count;
  LNK_DefFileLineNode *first;
  LNK_DefFileLineNode *last;
};

internal String8List
lnk_def_file_tokenize(Arena *arena, String8 line)
{
  // tokenize with windows quoting rules
  line = push_str8_copy(arena, line);
  for EachIndex(i, line.size) {
    if (line.str[i] == '\t') {
      line.str[i] = ' ';
    }
  }

  String8List tokens = lnk_arg_list_parse_windows_rules(arena, line);

  String8List result = {0};
  for (String8Node *token_n = tokens.first; token_n != 0; token_n = token_n->next) {
    String8      token  = token_n->string;
    String8Node *next_n = token_n->next;
    if (token.size == 1 && token.str[0] == '@' && next_n != 0) {
      str8_list_push(arena, &result, str8_cat(arena, token, next_n->string));
      token_n = next_n;
    } else {
      str8_list_push(arena, &result, token);
    }
  }
  return result;
}

internal void
lnk_apply_def_file_to_config(LNK_Config *config, String8 path, LNK_Obj *obj)
{
  Temp scratch = scratch_begin(&config->arena, 1);

  // load DEF file
  B8      file_read = 0;
  String8 raw_file  = lnk_read_data_from_file_path(scratch.arena, config->io_flags, path, &file_read);
  if ( ! file_read) {
    lnk_error(LNK_Error_Cmdl, "failed to read DEF file: %S", path);
    goto exit;
  }

  String8 file = lnk_text_file_string_from_data(scratch.arena, raw_file);

  // parse & collect normalized DEF lines
  LNK_DefFileLineList def_lines = {0};
  LNK_DefFileStmt active_stmt = LNK_DefFileStmt_Null;
  String8         rest        = file;
  while (rest.size != 0) {
    String8 line = str8_chop_line(&rest);

    // strip comments outside quotes
    B32 in_quote = 0;
    for EachIndex(i, line.size) {
      U8 c = line.str[i];
      if (in_quote && c == '\\') {
        i += 1;
        continue;
      }
      if (c == '"') {
        in_quote = !in_quote;
      } else if (c == ';' && !in_quote) {
        line = str8_prefix(line, i);
        break;
      }
    }

    line = str8_skip_chop_whitespace(line);
    if (line.size == 0) { continue; }

    // parse statement keyword
    String8         tail = line;
    LNK_DefFileStmt stmt = LNK_DefFileStmt_Null;
    B32             is_keyword = 0;
    if (line.size != 0 && line.str[0] != '"') {
      U64 keyword_opl = 0;
      for (; keyword_opl < line.size; keyword_opl += 1) {
        U8 c = line.str[keyword_opl];
        if (char_is_space(c) || c == ':' || c == '=') {
          break;
        }
      }

      String8 keyword = str8_prefix(line, keyword_opl);
      for EachElement(i, g_def_file_stmt_map) {
        if (str8_matchi(g_def_file_stmt_map[i].name, keyword)) {
          stmt = g_def_file_stmt_map[i].stmt;
          break;
        }
      }

      if (stmt != LNK_DefFileStmt_Null) {
        U64 tail_pos = keyword_opl;
        while (tail_pos < line.size && char_is_space(line.str[tail_pos])) {
          tail_pos += 1;
        }
        if (tail_pos < line.size && (line.str[tail_pos] == ':' || line.str[tail_pos] == '=')) {
          tail_pos += 1;
        }
        tail = str8_skip_chop_whitespace(str8_skip(line, tail_pos));
      }
    }

    if (stmt != LNK_DefFileStmt_Null) {
      B32 is_multiline = stmt == LNK_DefFileStmt_Exports ||
                         stmt == LNK_DefFileStmt_Sections ||
                         stmt == LNK_DefFileStmt_Imports ||
                         stmt == LNK_DefFileStmt_Segments;
      active_stmt = is_multiline ? stmt : LNK_DefFileStmt_Null;
      is_keyword = 1;
      line = tail;
    } else {
      stmt = active_stmt;
    }

    LNK_DefFileLineNode *node = push_array(scratch.arena, LNK_DefFileLineNode, 1);
    node->stmt       = stmt;
    node->line       = line;
    node->is_keyword = is_keyword;
    SLLQueuePush(def_lines.first, def_lines.last, node);
    def_lines.count += 1;
  }

  // apply collected DEF lines to config
  for EachNode(def_line, LNK_DefFileLineNode, def_lines.first) {
    String8         line = def_line->line;
    LNK_DefFileStmt stmt = def_line->stmt;

    switch (stmt) {
    case LNK_DefFileStmt_Description: {
      lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, LNK_CmdSwitch_Def, "DESCRIPTION is ignored in DEF files");
    } break;

    case LNK_DefFileStmt_Imports: {
      if (def_line->is_keyword) {
        lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, LNK_CmdSwitch_Def, "IMPORTS is ignored in DEF files");
      } else {
        lnk_log(LNK_Log_Debug, "%S: unsupported old-style import specifications", path);
      }
    } break;

    case LNK_DefFileStmt_Stub: {
      lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, LNK_CmdSwitch_Def, "STUB is ignored in DEF files");
    } break;

    case LNK_DefFileStmt_Segments: {
      lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, LNK_CmdSwitch_Def, "SEGMENTS is ignored in DEF files");
    } break;

    case LNK_DefFileStmt_Exports: {
      String8List tokens = lnk_def_file_tokenize(scratch.arena, line);
      if (tokens.node_count != 0) {
        PE_ExportParse export_parse = {0};
        if (lnk_parse_export_directive_ex(config->arena, tokens, obj, &export_parse)) {
          export_parse.obj_path = path;
          lnk_push_export_to_config(config, obj, export_parse);
        }
      }
    } break;

    case LNK_DefFileStmt_Library:
    case LNK_DefFileStmt_Name: {
      String8List tokens = lnk_def_file_tokenize(scratch.arena, line);
      String8     name   = {0};

      for EachNode(token_n, String8Node, tokens.first) {
        String8 token = token_n->string;
        String8 base_value = {0};
        B32     has_base   = 0;

        U64 sep_pos = str8_find_needle(token, 0, str8_lit("="), 0);
        if (sep_pos < token.size) {
          String8 key = str8_prefix(token, sep_pos);
          if (str8_matchi(key, str8_lit("BASE"))) {
            base_value = str8_skip(token, sep_pos + 1);
            has_base = 1;
          }
        } else if (str8_matchi(token, str8_lit("BASE")) ||
                   str8_match_lit("BASE:", token, StringMatchFlag_CaseInsensitive|StringMatchFlag_RightSideSloppy)) {
          lnk_error_cmd_switch(LNK_Error_Cmdl, obj, LNK_CmdSwitch_Def, "syntax error in DEF file BASE specification");
        }

        if (has_base) {
          lnk_apply_cmd_option_to_config(config, str8_lit("base"), base_value, obj);
        } else if (name.size == 0) {
          name = token;
        } else {
          lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, LNK_CmdSwitch_Def, "ignoring unexpected DEF token \"%S\"", token);
        }
      }

      if (stmt == LNK_DefFileStmt_Library) {
        config->file_characteristics |= PE_ImageFileCharacteristic_DLL;
      }

      if (name.size != 0) {
        String8 image_name = name;
        if (stmt == LNK_DefFileStmt_Library) {
          // set DLL import name
          String8 file_name = str8_skip_last_slash(name);
          B32 has_ext = (str8_skip_last_dot(file_name).size != file_name.size);
          if (!has_ext) {
            image_name = path_replace_file_extension(scratch.arena, name, str8_lit("dll"));
          }
          if (config->image_alt_path.size == 0) {
            config->image_alt_path = push_str8_copy(config->arena, image_name);
          }
        }

        if (config->out_path.size == 0) {
          config->out_path = push_str8_copy(config->arena, image_name);
        }
      }
    } break;

    case LNK_DefFileStmt_Sections: {
      String8List tokens = lnk_def_file_tokenize(scratch.arena, line);
      if (tokens.node_count != 0) {
        B32 is_parse_ok = 1;
        String8List section_values = {0};
        str8_list_push(scratch.arena, &section_values, tokens.first->string);

        for (String8Node *token_n = tokens.first->next; token_n != 0; token_n = token_n->next) {
          String8 token = token_n->string;
          if (str8_matchi(token, str8_lit("CLASS"))) {
            if (token_n->next != 0) {
              token_n = token_n->next;
            }
            continue;
          }

          String8 attr = {0};
          if (str8_matchi(token, str8_lit("EXECUTE"))) {
            attr = str8_lit("E");
          } else if (str8_matchi(token, str8_lit("READ"))) {
            attr = str8_lit("R");
          } else if (str8_matchi(token, str8_lit("SHARED"))) {
            attr = str8_lit("S");
          } else if (str8_matchi(token, str8_lit("WRITE"))) {
            attr = str8_lit("W");
          } else {
            lnk_error_cmd_switch(LNK_Error_Cmdl, obj, LNK_CmdSwitch_Def, "unknown DEF SECTIONS specifier \"%S\"", token);
            is_parse_ok = 0;
          }

          if (attr.size != 0) {
            str8_list_push(scratch.arena, &section_values, attr);
          }
        }

        if (is_parse_ok) {
          String8 value = str8_list_join(scratch.arena, &section_values, &(StringJoin){ .sep = str8_lit(",") });
          lnk_apply_cmd_option_to_config(config, str8_lit("section"), value, obj);
        }
      }
    } break;

    case LNK_DefFileStmt_StackSize: {
      String8List values = str8_split_by_string_chars(scratch.arena, line, str8_lit(" \t,"), 0);
      String8 value = str8_list_join(scratch.arena, &values, &(StringJoin){ .sep = str8_lit(",") });
      lnk_apply_cmd_option_to_config(config, str8_lit("stack"), value, obj);
    } break;

    case LNK_DefFileStmt_Version: {
      lnk_apply_cmd_option_to_config(config, str8_lit("version"), str8_skip_chop_whitespace(line), obj);
    } break;

    case LNK_DefFileStmt_HeapSize: {
      String8List values = str8_split_by_string_chars(scratch.arena, line, str8_lit(" \t,"), 0);
      String8 value = str8_list_join(scratch.arena, &values, &(StringJoin){ .sep = str8_lit(",") });
      lnk_apply_cmd_option_to_config(config, str8_lit("heap"), value, obj);
    } break;

    default: {
      lnk_error_cmd_switch(LNK_Warning_Cmdl, obj, LNK_CmdSwitch_Def, "ignoring unrecognized DEF statement \"%S\"", line);
    } break;
    }
  }

  exit:;
  scratch_end(scratch);
}

internal String8List
lnk_unwrap_cmd_line(Arena *arena, String8List arg_list)
{
  Temp scratch = scratch_begin(&arena, 1);

  typedef struct LNK_RspFrame LNK_RspFrame;
  struct LNK_RspFrame
  {
    LNK_RspFrame *next;
    String8       path;
    String8Node  *arg_cursor;
  };

  String8List   result   = {0};
  LNK_RspFrame *frame    = &(LNK_RspFrame){ .arg_cursor = arg_list.first, .path = str8_lit("Command Line") };

  while (frame) {
    if (frame->arg_cursor == 0) {
      frame = frame->next;
      continue;
    }

    String8Node *curr = frame->arg_cursor;
    frame->arg_cursor = curr->next;

    if (str8_starts_with(curr->string, str8_lit("@"))) {
      String8 file_name = str8_skip(curr->string, 1);

      // error check empty rsp argument
      if (file_name.size == 0) {
        lnk_error(LNK_Error_Cmdl, "RSP file name must follow '@'");
        continue;
      }

      String8 full_path = full_path_from_path(scratch.arena, file_name);

      // error check for cyclic response files
      {
        B32 is_rsp_cyclic = 0;
        for EachNode(parent, LNK_RspFrame, frame) {
          if (path_match_normalized(parent->path, full_path)) {
            is_rsp_cyclic = 1;
            break;
          }
        }
        if (is_rsp_cyclic) {
          String8List cycle_list = {0};
          for EachNode(parent, LNK_RspFrame, frame) {
            str8_list_push_front(scratch.arena, &cycle_list, parent->path);
          }
          str8_list_push(scratch.arena, &cycle_list, full_path);
          String8 cycle_string = str8_list_join(scratch.arena, &cycle_list, &(StringJoin){ .sep = str8_lit(" -> ") });
          lnk_error(LNK_Error_Cmdl, "detected a cyclic RSP: %S", cycle_string);
          continue;
        }
      }

      // read rsp from disk
      B8 is_file_read = 0;
      String8 file_data = lnk_read_data_from_file_path(scratch.arena, 0, file_name, &is_file_read);
      if ( ! is_file_read) {
        lnk_error(LNK_Error_Cmdl, "unable to find RSP: %S", file_name);
        continue;
      }

      // unapck rsp
      String8     file_text = lnk_text_file_string_from_data(scratch.arena, file_data);
      String8List file_args = lnk_arg_list_parse_windows_rules(scratch.arena, file_text);

      // push new frame with unapcked rsp
      LNK_RspFrame *new_frame = push_array(scratch.arena, LNK_RspFrame, 1);
      new_frame->path       = full_path;
      new_frame->arg_cursor = file_args.first;
      SLLStackPush(frame, new_frame);
    } else {
      // append normal argument
      str8_list_push(arena, &result, push_str8_copy(arena, curr->string));
    }
  }

  scratch_end(scratch);
  return result;
}

internal LNK_CmdLine
lnk_make_default_cmd_line(Arena *arena, LNK_CmdLine user_cmd_line)
{
  Temp scratch = scratch_begin(&arena, 1);
  LNK_CmdLine cmd_line = {0};

  char *default_opts[] = {
    "/ALIGN:4096",
    "/DEBUG:none",
    "/FILEALIGN:512",
    "/HIGHENTROPYVA",
    "/MANIFESTUAC:\"level='asInvoker' uiAccess='false'\"",
    "/NXCOMPAT",
    "/LARGEADDRESSAWARE",
    "/PDBALTPATH:%_RAD_PDB_PATH%",
    "/PDBPAGESIZE:4096",
    (char*)str8f(scratch.arena, "/HEAP:%llu,%llu", MB(1), KB(4)).str,
    (char*)str8f(scratch.arena, "/STACK:%llu,%llu", MB(1), KB(4)).str,

    "/RAD_BOOT_MODE:LINKER",
    //"/RAD_BUILD_EXP",
    "/RAD_BUILD_IMPLIB",
    "/RAD_AGE:1",
    "/RAD_CHECK_UNUSED_DELAY_LOAD_DLL",
    "/RAD_DO_MERGE",
    "/RAD_ENV_LIB",
    "/RAD_EXE",
    "/RAD_GUID:imageblake3",
    "/RAD_LARGE_PAGES:no",
    "/RAD_LINK_VER:14.0",
    "/RAD_OS_VER:6.0",
    "/RAD_PAGE_SIZE:4096",
    "/RAD_PATH_STYLE:system",
    "/RAD_PDB_HASH_TYPE_NAMES:NONE",
    "/RAD_PDB_HASH_TYPE_NAME_LENGTH:8",
    "/RAD_DEBUGALTPATH:%_RAD_RDI_PATH%",
    "/RAD_MEMORY_MAP_FILES",
    "/RAD_MAP_LINES_FOR_UNRESOLVED_SYMBOLS",
    "/RAD_UNRESOLVED_SYMBOL_LIMIT:1000",
    "/RAD_UNRESOLVED_SYMBOL_REF_LIMIT:10",
    "/RAD_SORT_IMPORTS",
    (char*)str8f(scratch.arena, "/RAD_MT_PATH:%s",        LNK_MANIFEST_MERGE_TOOL_NAME).str,
    (char*)str8f(scratch.arena, "/RAD_DATA_DIR_COUNT:%u", PE_DataDirectoryIndex_COUNT).str,

    // Set BLAKE3 as the default to match the LLVM default.
    //
    // When hash kinds conflict, radlink discards any .debug$H sections
    // whose hash kind does not match the selected default.
    "/RAD_DEBUG_TYPE_HASH:BLAKE3",

    // Use LLVM significant addresses hints for the /OPT:ICF.
    "/LLVM_ADDRSIG",

    // By default keep full type names, override when TPI/IPI streams overflow.
    "/RAD_PDB_HASH_TYPE_NAMES:NONE",

    // TODO: The ICF algorithm requires a cryptographic hash to establish
    // equivalence. With xxHash and similar non-cryptographic hashes,
    // the algorithm must compare each section property before
    // deciding whether sections are truly identical.
    "/RAD_ICF_HASH_KIND:BLAKE3",
  };

  char *push_opts[] = {
    "/MERGE:.xdata=.rdata",
    "/MERGE:.00cfg=.rdata",
    // TODO: .tls must be always first contribution in .data section because compiler generates TLS relative movs
    //"/MERGE:.tls=.data",
    "/MERGE:.idata=.data",
    "/MERGE:.didat=.data",
    "/MERGE:.edata=.rdata",
    "/MERGE:.RAD_LINK_PE_DEBUG_DIR=.rdata",
    "/MERGE:.RAD_LINK_PE_DEBUG_DATA=.rdata",

    "/RAD_REMOVE_SECTION:.debug",
    "/RAD_REMOVE_SECTION:.gehcont",
    "/RAD_REMOVE_SECTION:.gfids",
    "/RAD_REMOVE_SECTION:.gxfg",

    (char*)str8f(scratch.arena, "/RAD_WORKERS:%u", get_system_info()->logical_processor_count).str,

    // errors that are too verbose in release build
    (char*)str8f(scratch.arena, "/RAD_IGNORE:%d", LNK_Warning_UnknownSwitch    * (BUILD_DEBUG ? -1 : 1)).str,
    (char*)str8f(scratch.arena, "/RAD_IGNORE:%d", LNK_Warning_UnknownDirective * (BUILD_DEBUG ? -1 : 1)).str,
    (char*)str8f(scratch.arena, "/RAD_IGNORE:%d", LNK_Error_InvalidTypeIndex   * (BUILD_DEBUG ? -1 : 1)).str,

    #if BUILD_DEBUG
    "/RAD_LOG:debug",
    "/RAD_LOG:io_write",
    #else
    (char*)str8f(scratch.arena, "/RAD_IGNORE:%u", LNK_Error_InvalidTypeIndex).str,
    #endif
  };

#define DefaultOpt(...) do {                                                                     \
  LNK_CmdLine parsed_cmd_line = lnk_cmd_line_from_stringf_windows_rules(arena, __VA_ARGS__);     \
  for EachNode(cmd, LNK_CmdOption, parsed_cmd_line.first_option) {                               \
    if (!lnk_cmd_line_has_switch(user_cmd_line, lnk_cmd_switch_type_from_string(cmd->string))) { \
      lnk_cmd_line_push_option_string(arena, &cmd_line, cmd->string, cmd->value);                 \
    }                                                                                            \
  }                                                                                              \
} while (0)

#define PushOpt(...) do {                                                                    \
  LNK_CmdLine parsed_cmd_line = lnk_cmd_line_from_stringf_windows_rules(arena, __VA_ARGS__); \
  lnk_cmd_line_concat_in_place(&cmd_line, &parsed_cmd_line);                                 \
} while (0)

  if (lnk_cmd_line_has_switch(user_cmd_line, LNK_CmdSwitch_Dll)) {
    DefaultOpt("/SUBSYSTEM:%S", pe_string_from_subsystem(PE_WindowsSubsystem_WINDOWS_GUI));
  }
  if (!lnk_cmd_line_has_switch(user_cmd_line, LNK_CmdSwitch_Brepro)) {
    DefaultOpt("/RAD_TIME_STAMP:%u", get_process_start_time_unix());
  }
  for EachIndex(i, ArrayCount(default_opts)) {
    DefaultOpt("%s", default_opts[i]);
  }

  for EachIndex(i, ArrayCount(push_opts)) {
    PushOpt("%s", push_opts[i]);
  }

  // when /FORCE is specified on the command line, do not stop on these errors
  if (lnk_cmd_line_has_switch(user_cmd_line, LNK_CmdSwitch_Force)) {
    g_error_mode_arr[LNK_Error_UnresolvedSymbol] = LNK_ErrorMode_Continue;
    g_error_mode_arr[LNK_Error_RelocationAgainstRemovedSection] = LNK_ErrorMode_Continue;
  }

#undef DefaultOpt
#undef PushOpt
  scratch_end(scratch);
  return cmd_line;
}

internal LNK_Config *
lnk_config_init(U64 argc, char **argv)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  // load env vars
  HashMap env_vars = lnk_env_vars_from_process_info(scratch.arena, get_process_info(), LNK_EnvVarRule_Batch);

  // concat command line from env vars
  String8List user_args = {0};
  {
    LNK_EnvVar *link;

    if ((link = lnk_env_var_from_mapf(&env_vars, "LINK"))) {
      String8List args = lnk_arg_list_parse_windows_rules(scratch.arena, link->raw_value);
      str8_list_concat_in_place(&user_args, &args);
    }

    for (U64 i = 1; i < argc; i += 1) {
      str8_list_push(scratch.arena, &user_args, str8_cstring(argv[i]));
    }

    if ((link = lnk_env_var_from_mapf(&env_vars, "_LINK_"))) {
      String8List args = lnk_arg_list_parse_windows_rules(scratch.arena, link->raw_value);
      str8_list_concat_in_place(&user_args, &args);
    }
  }

  // concat command line string for later usages (e.g. embed in debug info)
  String8 cmd_line_string = str8_list_join(scratch.arena, &user_args, &(StringJoin){ .sep = str8_lit(" ") });

#if PROFILE_TELEMETRY
  tmMessage(0, TMMF_ICON_NOTE, "Command Line: %.*s", str8_varg(cmd_line_string));
#endif

  // concat default arguments
  LNK_CmdLine cmd_line = {0};
  {
    String8List cmd_unwrap       = lnk_unwrap_cmd_line(scratch.arena, user_args);
    LNK_CmdLine cmd_line_user    = lnk_cmd_line_parse_windows_rules(scratch.arena, cmd_unwrap);
    LNK_CmdLine cmd_line_default = lnk_make_default_cmd_line(scratch.arena, cmd_line_user);
    lnk_cmd_line_concat_in_place(&cmd_line, &cmd_line_default);
    lnk_cmd_line_concat_in_place(&cmd_line, &cmd_line_user);
  }
  
  Arena      *arena  = arena_alloc();
  LNK_Config *config = push_array(arena, LNK_Config, 1);
  config->arena        = arena;
  config->raw_cmd_line = str8_copy(arena, cmd_line_string);
  config->work_dir     = get_current_path(arena);
  config->force        = lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Force);

  // fault-bound debug-input stages spin on the kernel page-fault path past ~20
  // concurrent workers (current throughput knee); default cap trades that spin for
  // free cores, /RAD_DEBUG_WORKERS:0 restores full width
  config->debug_worker_cap = 20;

  // apply command line switches
  for EachNode(cmd, LNK_CmdOption, cmd_line.first_option) {
    lnk_apply_cmd_option_to_config(config, cmd->string, cmd->value, 0);
  }

  // in shared thread pool mode force fixed number of workers
  if (lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Rad_SharedThreadPool) &&
      !lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Rad_SharedThreadPoolMaxWorkers)) {
    config->max_worker_count = get_system_info()->logical_processor_count;
  }

  // :manifest_input
  if (lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_ManifestInput)) {
    if (config->manifest_opt == LNK_ManifestOpt_Embed) {
      String8List manifest_list = lnk_cmd_line_values_from_switch(arena, cmd_line, LNK_CmdSwitch_ManifestInput);
      str8_list_concat_in_place(&config->input_list[LNK_Input_Manifest], &manifest_list);
    } else {
      lnk_error_cmd_switch(LNK_Error_Cmdl, 0, LNK_CmdSwitch_ManifestInput, "missing /MANIFEST:EMBED");
    }
  }

  // set default manifest resource id
  if (config->manifest_resource_id == 0) {
    if (config->file_characteristics & PE_ImageFileCharacteristic_DLL) {
      config->manifest_resource_id = push_u64(arena, 2);
    } else {
      config->manifest_resource_id = push_u64(arena, 1);
    }
  }

  // input files
  for EachNode(input_node, String8Node, cmd_line.input_list.first) {
    String8 path = push_str8_copy(arena, input_node->string);
    String8 ext  = str8_skip_last_dot(path);

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

  if (config->import_table_emit_biat == LNK_SwitchState_Null) {
    config->import_table_emit_biat = LNK_SwitchState_Yes;
  }
  if (config->import_table_emit_uiat == LNK_SwitchState_Null) {
    config->import_table_emit_uiat = LNK_SwitchState_Yes;
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
  if ((~config->file_characteristics & PE_ImageFileCharacteristic_LARGE_ADDRESS_AWARE) && (config->file_characteristics & PE_ImageFileCharacteristic_DLL)) {
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
    config->file_characteristics |= PE_ImageFileCharacteristic_RELOCS_STRIPPED;
    config->dll_characteristics &= ~PE_DllCharacteristic_DYNAMIC_BASE;
  }
  // if we don't have a fixed image and dynamic base switch 
  // was omitted we make image with dynamic base
  else if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_DynamicBase)) {
    config->dll_characteristics |= PE_DllCharacteristic_DYNAMIC_BASE;
  }
  
  // TODO: Set GUARD_CF only after emitting the Guard CF function tables.
  // Marking the image without valid load-config tables makes CFG-enabled
  // executables crash on indirect calls.
  if (config->guard_flags != LNK_Guard_None) {
    // config->dll_characteristics |= PE_DllCharacteristic_GUARD_CF;
  }

  // handle empty /OUT
  if (!config->out_path.size) {
    String8 name     = str8_list_first(&config->input_list[LNK_Input_Obj]);
    String8 ext      = (config->file_characteristics & PE_ImageFileCharacteristic_DLL) ? str8_lit("dll") : str8_lit("exe");
    config->out_path = path_replace_file_extension(scratch.arena, name, ext);
  }

  // handle empty /PDB
  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Pdb)) {
    config->pdb_name = path_replace_file_extension(scratch.arena, config->out_path, str8_lit("pdb"));
  }

  // handle empty /MAP
  if (config->map == LNK_SwitchState_Yes && !config->map_name.size) {
    config->map_name = path_replace_file_extension(scratch.arena, config->out_path, str8_lit("map"));
  }

  // handle empty /RAD_DEBUG_NAME
  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Rad_DebugName)) {
    config->rad_debug_name = path_replace_file_extension(scratch.arena, config->out_path, str8_lit("rdi"));
  }

  // handle empty /IMPLIB
  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_ImpLib)) {
    config->imp_lib_name = path_replace_file_extension(scratch.arena, config->out_path, str8_lit("lib"));
  }

  // handle empty /MANIFESTFILE
  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_ManifestFile)) {
    config->manifest_name = push_str8f(scratch.arena, "%S.manifest", config->out_path);
  }

  // convert to full paths
  config->out_path       = full_path_from_path(arena, config->out_path);
  config->pdb_name       = full_path_from_path(arena, config->pdb_name);
  if (config->map_name.size) {
    config->map_name = full_path_from_path(arena, config->map_name);
  }
  config->rad_debug_name = full_path_from_path(arena, config->rad_debug_name);
  config->imp_lib_name   = full_path_from_path(arena, config->imp_lib_name);
  config->manifest_name  = full_path_from_path(arena, config->manifest_name);

  // reject output path aliases before background writers are launched
  if (config->map_name.size) {
    String8 output_paths[] = {
      config->out_path,
      config->pdb_name,
      config->rad_debug_name,
      config->imp_lib_name,
      config->manifest_name,
    };
    for EachElement(i, output_paths) {
      if (output_paths[i].size && path_match_normalized(config->map_name, output_paths[i])) {
        lnk_error(LNK_Error_Cmdl, "/MAP output path %S conflicts with another linker output", config->map_name);
        config->map = LNK_SwitchState_No;
        config->map_name = str8_zero();
        break;
      }
    }
  }

  // push linker env vars
  {
    struct { String8 key, value; } key_value_str8_table[] = {
      { str8_lit("_pdb"),           str8_skip_last_slash(config->pdb_name)       },
      { str8_lit("_ext"),           str8_skip_last_dot(config->out_path)         },
      { str8_lit("_rad_pdb_path"),  config->pdb_name                             },
      { str8_lit("_rad_rdi"),       str8_skip_last_slash(config->rad_debug_name) },
      { str8_lit("_radi_rdi_path"), config->rad_debug_name                       },
    };
    for EachElement(i, key_value_str8_table) {
      if (lnk_env_var_from_map(&env_vars, key_value_str8_table[i].key)) {
        lnk_log(LNK_Log_Debug, "Env var already exists: %S\n",key_value_str8_table[i].key);
      }
      lnk_env_var_batchf(scratch.arena, &env_vars, "%S=%S", key_value_str8_table[i].key, key_value_str8_table[i].value);
    }
  }

  // collect LIB and LIBPATH
  if (config->flags & LNK_ConfigFlag_EnvLib) {
    struct { String8List *config_list; char *key; } key_str8_list[] = {
      { &config->lib_dir_list, "lib"      },
      { &config->lib_dir_list, "lib_path" },
    };
    for EachElement(i, key_str8_list) { 
      LNK_EnvVar *var = lnk_env_var_from_mapf(&env_vars, key_str8_list[i].key);
      if (var) {
        String8List value      = lnk_value_list_from_env_var(arena, var);
        String8List value_copy = str8_list_copy(config->arena, &value);
        str8_list_concat_in_place(key_str8_list[i].config_list, &value_copy);
      }
    }
  }
  
  // :PdbAltPath
  config->pdb_alt_path = lnk_expand_env_vars_windows(arena, &env_vars, config->pdb_alt_path);

  // :Rad_DebugAltPath
  config->rad_debug_alt_path = lnk_expand_env_vars_windows(arena, &env_vars, config->rad_debug_alt_path);

  // create temporary files names
  if (config->write_temp_files == LNK_SwitchState_Yes) {
    lnk_apply_write_temp_files(arena, config);
  }

  scratch_end(scratch);
  ProfEnd();
  return config;
}
