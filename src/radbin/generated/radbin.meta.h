// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef RADBIN_META_H
#define RADBIN_META_H

typedef enum RB_FileFormat
{
RB_FileFormat_Null,
RB_FileFormat_PDB,
RB_FileFormat_PE,
RB_FileFormat_COFF_OBJ,
RB_FileFormat_COFF_BigOBJ,
RB_FileFormat_COFF_Archive,
RB_FileFormat_COFF_ThinArchive,
RB_FileFormat_ELF32,
RB_FileFormat_ELF64,
RB_FileFormat_RDI,
RB_FileFormat_COUNT,
} RB_FileFormat;

C_LINKAGE_BEGIN
extern String8 rb_file_format_display_name_table[10];

C_LINKAGE_END

#endif // RADBIN_META_H
