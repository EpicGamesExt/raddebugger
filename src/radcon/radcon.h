// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADCON_H
#define RADCON_H

typedef U32 RC_Flags;
enum
{
  RC_Flag_Strings                 = (1 <<  0),
  RC_Flag_IndexRuns               = (1 <<  1),
  RC_Flag_BinarySections          = (1 <<  2),
  RC_Flag_Units                   = (1 <<  3),
  RC_Flag_Procedures              = (1 <<  4),
  RC_Flag_GlobalVariables         = (1 <<  5),
  RC_Flag_ThreadVariables         = (1 <<  6),
  RC_Flag_Scopes                  = (1 <<  7),
  RC_Flag_Locals                  = (1 <<  8),
  RC_Flag_Types                   = (1 <<  9),
  RC_Flag_UDTs                    = (1 << 10),
  RC_Flag_LineInfo                = (1 << 11),
  RC_Flag_GlobalVariableNameMap   = (1 << 12),
  RC_Flag_ThreadVariableNameMap   = (1 << 13),
  RC_Flag_ProcedureNameMap        = (1 << 14),
  RC_Flag_TypeNameMap             = (1 << 15),
  RC_Flag_LinkNameProcedureNameMap= (1 << 16),
  RC_Flag_NormalSourcePathNameMap = (1 << 17),
  RC_Flag_Compress                = (1 << 18),
  RC_Flag_StrictDwarfParse        = (1 << 19),
  RC_Flag_Deterministic           = (1 << 20),
  RC_Flag_CheckPdbGuid            = (1 << 21),
  RC_Flag_CheckElfChecksum        = (1 << 22),
  RC_Flag_All = 0xffffffff,
};

typedef enum
{
  RC_Driver_Null,
  RC_Driver_Dwarf,
  RC_Driver_Pdb,
} RC_Driver;

typedef struct RC_Context
{
  ImageType        image;
  RC_Driver        driver;
  String8          image_name;
  String8          image_data;
  String8          debug_name;
  String8          debug_data;
  String8          out_name;
  RC_Flags         flags;
  Guid             guid;
  ELF_GnuDebugLink debug_link;
  String8List      errors;
} RC_Context;

////////////////////////////////

internal RC_Context  rc_context_from_cmd_line(Arena *arena, CmdLine *cmdl);
internal String8List rc_run(Arena *arena, RC_Context *rc);
internal String8     rc_rdi_from_cmd_line(Arena *arena, CmdLine *cmdl);
internal void        rc_main(CmdLine *cmdl);

#endif // RADCON_H

