// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef COFF_ENUM_H
#define COFF_ENUM_H

internal String8 coff_string_from_time_stamp(Arena *arena, COFF_TimeStamp time_stamp);
internal String8 coff_string_from_comdat_select_type(COFF_ComdatSelectType type);
internal String8 coff_string_from_machine_type(COFF_MachineType machine);
internal String8 coff_string_from_flags(Arena *arena, COFF_FileHeaderFlags flags);
internal String8 coff_string_from_section_flags(Arena *arena, COFF_SectionFlags flags);
internal String8 coff_string_from_resource_memory_flags(Arena *arena, COFF_ResourceMemoryFlags flags);
internal String8 coff_string_from_import_header_type(COFF_ImportType type);
internal String8 coff_string_from_sym_dtype(COFF_SymDType x);
internal String8 coff_string_from_sym_type(COFF_SymType x);
internal String8 coff_string_from_sym_storage_class(COFF_SymStorageClass x);
internal String8 coff_string_from_weak_ext_type(COFF_WeakExtType x);
internal String8 coff_string_from_reloc_x86(COFF_Reloc_X86 x);
internal String8 coff_string_from_reloc_x64(COFF_Reloc_X64 x);
internal String8 coff_string_from_reloc_arm(COFF_Reloc_Arm x);
internal String8 coff_string_from_reloc_arm64(COFF_Reloc_Arm64 x);
internal String8 coff_string_from_reloc(COFF_MachineType machine, COFF_RelocType x);

internal COFF_MachineType coff_machine_from_string(String8 string);
internal COFF_ImportType  coff_import_header_type_from_string(String8 name);

#endif // COFF_ENUM_H
