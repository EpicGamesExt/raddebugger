// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// build with:
// cl /Zi /nologo look_at_raddbg.c

#include <windows.h>
#include <stdint.h>
#include "raddbgi_format/raddbgi_format.h"
#include "raddbgi_format/raddbgi_format_parse.h"
#include "raddbgi_format/raddbgi_format.c"
#include "raddbgi_format/raddbgi_format_parse.c"

int main(int argument_count, char **arguments)
{
  // map raddbg file into address space
  HANDLE file = CreateFileA("UnrealEditorFortnite.raddbg", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  DWORD size_hi32 = 0;
  DWORD size_lo32 = GetFileSize(file, &size_hi32);
  HANDLE map = CreateFileMappingA(file, 0, PAGE_READONLY, 0, 0, 0);
  uint64_t data_size = (size_lo32 | ((uint64_t)size_hi32 << 32));
  uint8_t *data = (uint8_t *)MapViewOfFile(map, FILE_MAP_READ, 0, 0, data_size);
  
  // parse raw data as raddbg
  RADDBGI_Parsed rdbg = {0};
  RADDBGI_ParseStatus parse_status = raddbgi_parse(data, data_size, &rdbg);
  
  // usage example: print out all procedure symbol names
#if 1
  for(uint64_t procedure_idx = 0; procedure_idx < rdbg.procedure_count; procedure_idx += 1)
  {
    RADDBGI_Procedure *procedure = &rdbg.procedures[procedure_idx];
    uint64_t name_size = 0;
    uint8_t *name = raddbgi_string_from_idx(&rdbg, procedure->name_string_idx, &name_size);
    printf("[%I64u] %.*s\n", procedure_idx, (int)name_size, name);
  }
#endif
  
  // usage example: print out all user-defined-type names
#if 0
  for(uint64_t udt_idx = 0; udt_idx < rdbg.udt_count; udt_idx += 1)
  {
    RADDBGI_UDT *udt = &rdbg.udts[udt_idx];
    RADDBGI_TypeNode *type = &rdbg.type_nodes[udt->self_type_idx];
    uint64_t name_size = 0;
    uint8_t *name = raddbgi_string_from_idx(&rdbg, type->user_defined.name_string_idx, &name_size);
    printf("[%I64u] %.*s\n", udt_idx, (int)name_size, name);
  }
#endif
  
  // for getting more info, look at the `RADDBGI_Parsed` structure. all data is
  // represented as a bunch of flat plain-old-data tables. data which must
  // reference other data uses indices into that other data's table. for
  // example, given a `type_idx`, I will index into `rdbg.type_nodes`. given a
  // `udt_idx`, I will index into `rdbg.udts`. given a `scope_idx`, I will
  // index into `rdbg.scopes`. and so on.
  
  return 0;
}
