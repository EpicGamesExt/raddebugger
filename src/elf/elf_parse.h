// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ELF_PARSE_H
#define ELF_PARSE_H

////////////////////////////////
//~ rjf: Parsed Structure Types

typedef struct ELF_Shdr64Array ELF_Shdr64Array;
struct ELF_Shdr64Array
{
  U64 count;
  ELF_Shdr64 *v;
};

typedef struct ELF_Phdr64Array ELF_Phdr64Array;
struct ELF_Phdr64Array
{
  U64 count;
  ELF_Phdr64 *v;
};

typedef struct ELF_Dyn64Array ELF_Dyn64Array;
struct ELF_Dyn64Array
{
  U64 count;
  ELF_Dyn64 *v;
};

typedef struct ELF_Bin ELF_Bin;
struct ELF_Bin
{
  ELF_Hdr64 hdr;
  Rng1U64 sh_name_range;
  ELF_Shdr64Array shdrs;
  ELF_Phdr64Array phdrs;
};

typedef struct ELF_GnuDebugLink ELF_GnuDebugLink;
struct ELF_GnuDebugLink
{
  String8 path;
  U32 checksum;
};

typedef struct ELF_Note ELF_Note;
struct ELF_Note
{
  String8 owner;
  ELF_NoteType type;
  String8 desc;
};

typedef struct ELF_NoteNode ELF_NoteNode;
struct ELF_NoteNode
{
  ELF_NoteNode *next;
  ELF_Note v;
};

typedef struct ELF_NoteList ELF_NoteList;
struct ELF_NoteList
{
  ELF_NoteNode *first;
  ELF_NoteNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Parsing Functions

//- rjf: top-level binary parsing
internal ELF_Bin elf_bin_from_data(Arena *arena, String8 data);

//- rjf: extra bin info extraction
internal B32 elf_is_dwarf_present_from_bin(String8 data, ELF_Bin *bin);
internal String8 elf_name_from_shdr64(String8 raw_data, ELF_Bin *bin, ELF_Shdr64 *shdr);
internal U64 elf_base_addr_from_bin(ELF_Bin *bin);
internal ELF_GnuDebugLink elf_gnu_debug_link_from_bin(String8 raw_data, ELF_Bin *bin);

internal ELF_NoteList elf_parse_note(Arena *arena, String8 raw_note, ELF_Class elf_class, ELF_MachineKind e_machine);

internal MachineOpResult elf_read_phdrs   (Arena *arena, ELF_Hdr64 ehdr, U64 base, Rng1U64 range, MachineOp_MemRead *mem_read, void *mem_read_ud, ELF_Phdr64Array *phdrs_out);
internal MachineOpResult elf_read_dyn_tags(Arena *arena, ELF_Hdr64 ehdr, ELF_Phdr64 pt_dynamic, Rng1U64 range, MachineOp_MemRead *mem_read, void *mem_read_ud, ELF_Dyn64Array *dyns_out);

internal MachineOpResult elf_symbol_entry_vaddr_from_memory(ELF_Hdr64 ehdr, U64 base, B32 is_rebased, MachineOp_MemRead *mem_read, void *mem_read_ud, String8 symbol_name, U64 *symbol_entry_vaddr_out);
internal MachineOpResult elf_find_first_phdr(ELF_Hdr64 ehdr, U64 base, MachineOp_MemRead *mem_read, void *mem_read_ud, ELF_PhdrType phdr_type, ELF_Phdr64 *phdr_out);

// rebase
internal void elf_rebase_phdr64_array(ELF_Phdr64Array *arr, U64 rebase);
internal void elf_rebase_phdr64(ELF_Phdr64 *phdr, U64 rebase);
internal void elf_rebase_dyn64_array(ELF_Dyn64Array *arr, U64 rebase);
internal void elf_rebase_dyn64(ELF_Dyn64 *v, U64 rebase);

#endif // ELF_PARSE_H
