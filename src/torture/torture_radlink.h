// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////

typedef enum
{
  T_Linker_Null,
  T_Linker_RAD,
  T_Linker_MSVC,
  T_Linker_LLVM
} T_Linker;

typedef enum
{
  T_MsvcLinkExitCode_UnresolvedExternals                  = 1120,
  T_MsvcLinkExitCode_CorruptOrInvalidSymbolTable          = 1235,
  T_MsvcLinkExitCode_SectionsFoundWithDifferentAttributes = 4078,
} T_MsvcLinkExitCode;

////////////////////////////////

internal T_Linker t_id_linker(void);

internal COFF_ObjSection * t_push_text_section(COFF_ObjWriter *obj_writer, String8 data);
internal COFF_ObjSection * t_push_data_section(COFF_ObjWriter *obj_writer, String8 data);
internal COFF_ObjSection * t_push_rdata_section(COFF_ObjWriter *obj_writer, String8 data);

internal String8 t_make_entry_obj(Arena *arena);
internal String8 t_make_sec_defn_obj(Arena *arena, String8 payload);
internal String8 t_make_obj_with_directive(Arena *arena, String8 directive);

internal B32 t_write_entry_obj(void);

