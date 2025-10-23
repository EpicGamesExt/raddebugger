// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef GNU_H
#define GNU_H

typedef struct GNU_LinkMap64
{
  U64 addr_vaddr;
  U64 name_vaddr;
  U64 ld_vaddr;   // address of the dynamic section
  U64 next_vaddr;
  U64 prev_vaddr;
} GNU_LinkMap64;

typedef struct GNU_LinkMap32
{
  U32 addr_vaddr;
  U32 name_vaddr;
  U32 ld_vaddr;
  U32 next_vaddr;
  U32 prev_vaddr;
} GNU_LinkMap32;

typedef U32 GNU_RT;
enum
{
  GNU_RT_Consistent = 0,
  GNU_RT_Add        = 1,
  GNU_RT_Delete     = 2,
};

// struct reflects r_debug from /usr/include/link.h
typedef struct GNU_RDebugInfo64
{
  S32    r_version; // must be greater than 0
  U64    r_map;     // address of first loaded object
  U64    r_brk;     // when module is loared/unloaded DL calls this function
  GNU_RT r_state;
  U64    r_ldbase;  // base addres of dynamic linker
} GNU_RDebugInfo64;

typedef struct GNU_RDebugInfo32
{
  S32    r_version;
  U32    r_map;
  U32    r_brk;
  GNU_RT r_state;
  U32    r_ldbase;
} GNU_RDebugInfo32;

////////////////////////////////

internal GNU_LinkMap64 elf_linkmap64_from_linkmap32(GNU_LinkMap32 linkmap32);
internal U64 gnu_rdebug_info_size_from_arch(Arch arch);
internal U64 gnu_r_brk_offset_from_arch(Arch arch);

#endif // GNU_H
