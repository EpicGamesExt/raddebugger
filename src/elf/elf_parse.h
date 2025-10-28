// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ELF_PARSE_H
#define ELF_PARSE_H

////////////////////////////////
//~ rjf: Parsed Structure Types

typedef struct ELF_Shdr64Array ELF_Shdr64Array;
struct ELF_Shdr64Array
{
  U64         count;
  ELF_Shdr64 *v;
};

typedef struct ELF_Phdr64Array ELF_Phdr64Array;
struct ELF_Phdr64Array
{
  U64         count;
  ELF_Phdr64 *v;
};

typedef struct ELF_Bin ELF_Bin;
struct ELF_Bin
{
  ELF_Hdr64       hdr;
  Rng1U64         sh_name_range;
  ELF_Shdr64Array shdrs;
  ELF_Phdr64Array phdrs;
};

typedef struct ELF_GnuDebugLink ELF_GnuDebugLink;
struct ELF_GnuDebugLink
{
  String8 path;
  U32     checksum;
};

typedef struct ELF_Note ELF_Note;
struct ELF_Note
{
  String8      owner;
  ELF_NoteType type;
  String8      desc;
};

typedef struct ELF_NoteNode ELF_NoteNode;
struct ELF_NoteNode
{
  ELF_Note      v;
  ELF_NoteNode *next;
};

typedef struct ELF_NoteList ELF_NoteList;
struct ELF_NoteList
{
  U64           count;
  ELF_NoteNode *first;
  ELF_NoteNode *last;
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

#endif // ELF_PARSE_H
