// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ELF_DUMP_H
#define ELF_DUMP_H

#define ELF_DumpSubset_XList \
  X(Note, note, "NOTE")

typedef U32 ELF_DumpSubset;
enum
{
#define X(name, name_lower, title) ELF_DumpSubset_##name,
  ELF_DumpSubset_XList
#undef X
};

typedef U32 ELF_DumpSubsetFlags;
enum
{
#define X(name, name_lower, title) ELF_DumpSubsetFlag_##name = (1 << ELF_DumpSubset_##name),
  ELF_DumpSubset_XList
#undef X
  ELF_DumpSubsetFlag_All = ~0,
};

internal String8List elf_dump(Arena *arena, String8 raw_elf, ELF_DumpSubsetFlags flags);

#endif // ELF_DUMP_H

