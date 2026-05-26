// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ELF_PARSE_H
#define ELF_PARSE_H

////////////////////////////////
//~ rjf: Parsed Structure Types

typedef enum
{
  ELF_BinAddrMode_Null,
  ELF_BinAddrMode_Virtual,
  ELF_BinAddrMode_File
} ELF_BinAddrMode;

typedef struct ELF_Bin ELF_Bin;
struct ELF_Bin
{
  ELF_BinAddrMode    addr_mode;
  ELF_Hdr64          ehdr;
  MachineOp_MemRead *mem_read;
  U8                 mem_read_ud[16];
  U64                base;
  B32                is_rebased;
};

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

// image loaders
internal MachineOpResult elf_load_virtual(MachineOp_MemRead *mem_read, int memory_fd, U64 base, B32 is_rebased, ELF_Bin *elf_out);
internal MachineOpResult elf_load_file   (String8 file, ELF_Bin *elf_out);

// image address/layout helpers
internal U64     elf_rebase_vaddr           (ELF_Bin *elf, U64 vaddr);
internal U64     elf_base_addr_from_bin     (ELF_Bin *elf, ELF_Phdr64Array phdrs);
internal Rng1U64 elf_image_vrange_from_phdrs(ELF_Bin *elf, ELF_Phdr64Array phdrs);

// in-place rebasing helpers
internal void elf_rebase_phdr64_array(ELF_Phdr64Array *arr, U64 rebase);
internal void elf_rebase_dyn64_array (ELF_Dyn64Array  *arr, U64 rebase);
internal void elf_rebase_phdr64      (ELF_Phdr64      *v,   U64 rebase);
internal void elf_rebase_dyn64       (ELF_Dyn64       *v,   U64 rebase);

// low-level format parsers
internal ELF_NoteList elf_parse_note(Arena *arena, ELF_Hdr64 ehdr, String8 raw_note);

// image structure parsers (section table, program headers, and misc)
internal MachineOpResult elf_parse_shdrs         (Arena *arena, ELF_Bin *elf, Rng1U64 range, ELF_Shdr64Array *shdrs_out);
internal MachineOpResult elf_parse_phdrs         (Arena *arena, ELF_Bin *elf, Rng1U64 range, ELF_Phdr64Array *phdrs_out);
internal MachineOpResult elf_parse_dyns          (Arena *arena, ELF_Bin *elf, Rng1U64 range, ELF_Phdr64 pt_dynamic, ELF_Dyn64Array *dyns_out);
internal MachineOpResult elf_parse_shdr_name     (Arena *arena, ELF_Bin *elf, ELF_Shdr64 *shstr, ELF_Shdr64 *shdr, String8 *name_out);
internal MachineOpResult elf_parse_shdr_data     (Arena *arena, ELF_Bin *elf, ELF_Shdr64 *shdr, String8 *data_out);
internal MachineOpResult elf_parse_gnu_debug_link(Arena *arena, ELF_Bin *elf, ELF_GnuDebugLink *debug_link_out);

// lookup helpers
internal MachineOpResult elf_find_first_phdr          (ELF_Bin *elf, ELF_PhdrType phdr_type, ELF_Phdr64 *phdr_out);
internal MachineOpResult elf_find_shdr_by_name        (ELF_Bin *elf, String8 name, ELF_Shdr64 *shdr_out);
internal MachineOpResult elf_find_symbol_entry_by_name(ELF_Bin *elf, String8 symbol_name, U64 *symbol_entry_vaddr_out);
internal MachineOpResult elf_find_rdebug_vaddr        (ELF_Bin *elf, U64 *rdebug_vaddr_out);

#endif // ELF_PARSE_H
