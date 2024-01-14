// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// build with:
// cl /Zi /nologo look_at_raddbg.c

#include <windows.h>
#include <stdint.h>
#include "raddbg_format/raddbg_format.h"
#include "raddbg_format/raddbg_format_parse.h"
#include "raddbg_format/raddbg_format.c"
#include "raddbg_format/raddbg_format_parse.c"

typedef struct Foo Foo;
struct Foo
{
  int x;
  int y;
  int z;
};

int main(int argument_count, char **arguments)
{
  Foo foo = {123, 456, 789};
  HANDLE file = CreateFileA(arguments[1], GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  DWORD size_hi32 = 0;
  DWORD size_lo32 = GetFileSize(file, &size_hi32);
  HANDLE map = CreateFileMappingA(file, 0, PAGE_READONLY, 0, 0, 0);
  uint64_t data_size = (size_lo32 | ((uint64_t)size_hi32 << 32));
  uint8_t *data = (uint8_t *)MapViewOfFile(map, FILE_MAP_READ, 0, 0, data_size);
  RADDBG_Parsed rdbg = {0};
  RADDBG_ParseStatus parse_status = raddbg_parse(data, data_size, &rdbg);
  uint64_t foo_count = 0;
  for(uint64_t idx = 0; idx < rdbg.type_node_count; idx += 1)
  {
    RADDBG_TypeNode *type_node = &rdbg.type_nodes[idx];
    if(RADDBG_TypeKind_FirstUserDefined <= type_node->kind && type_node->kind <= RADDBG_TypeKind_LastUserDefined)
    {
      uint64_t name_size = 0;
      uint8_t *name = raddbg_string_from_idx(&rdbg, type_node->user_defined.name_string_idx, &name_size);
      if(name_size == 3 && name[0] == 'f' && name[1] == 'o' && name[2] == 'o')
      {
        foo_count += 1;
      }
    }
  }
  printf("%s -> %I64u foos\n", arguments[1], foo_count);
  return 0;
}
