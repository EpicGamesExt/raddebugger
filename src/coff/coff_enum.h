// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef COFF_ENUM_H
#define COFF_ENUM_H

internal String8 coff_string_from_time_stamp(Arena *arena, COFF_TimeStamp time_stamp);
internal String8 coff_string_from_comdat_select_type(COFF_ComdatSelectType select);
internal String8 coff_string_from_machine_type(COFF_MachineType machine);
internal String8 coff_string_from_flags(Arena *arena, COFF_Flags flags);
internal String8 coff_string_from_section_flags(Arena *arena, COFF_SectionFlags flags);
internal String8 coff_string_from_import_header_type(COFF_ImportHeaderType type);
internal String8 coff_string_from_sym_dtype(COFF_SymDType x);
internal String8 coff_string_from_sym_type(COFF_SymType x);
internal String8 coff_string_from_sym_storage_class(COFF_SymStorageClass x);
internal String8 coff_string_from_weak_ext_type(COFF_WeakExtType x);
internal String8 coff_string_from_selection(COFF_ComdatSelectType x);
internal String8 coff_string_from_reloc_x86(COFF_RelocTypeX86 x);
internal String8 coff_string_from_reloc_x64(COFF_RelocTypeX64 x);
internal String8 coff_string_from_reloc_arm(COFF_RelocTypeARM x);
internal String8 coff_string_from_reloc_arm64(COFF_RelocTypeARM64 x);
internal String8 coff_string_from_reloc(COFF_MachineType machine, COFF_RelocType x);

internal COFF_MachineType      coff_machine_from_string(String8 string);
internal COFF_ImportHeaderType coff_import_header_type_from_string(String8 name);

#endif // COFF_ENUM_H
