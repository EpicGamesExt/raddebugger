// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef GNU_PARSE_H
#define GNU_PARSE_H

typedef struct GNU_LinkMapNode
{
  GNU_LinkMap64 v;
  struct GNU_LinkMapNode *next;
} GNU_LinkMapNode;

typedef struct GNU_LinkMapList
{
  U64 count;
  GNU_LinkMapNode *first;
  GNU_LinkMapNode *last;
} GNU_LinkMapList;

typedef struct GNU_RDebugInfoNode
{
  GNU_RDebugInfo64 v;
  struct GNU_RDebugInfoNode *next;
} GNU_RDebugInfoNode;

typedef struct GNU_RDebugInfoList
{
  U64 count;
  GNU_RDebugInfoNode *first;
  GNU_RDebugInfoNode *last;
} GNU_RDebugInfoList;

#define GNU_MEM_READ(name) U64 name(U64 addr, U64 size, void *buffer, void *ud)
typedef GNU_MEM_READ(GNU_MemRead);

////////////////////////////////
internal GNU_RDebugInfoList gnu_parse_rdebug(Arena *arena, B32 is_64bit, U64 first_rdebug_vaddr, GNU_MemRead *mem_read_func, void *mem_read_ud);
internal GNU_LinkMapList    gnu_parse_link_map_list(Arena *arena, B32 is_64bit, U64 first_link_map_vaddr, GNU_MemRead *mem_read_func, void *mem_read_ud);

#endif

