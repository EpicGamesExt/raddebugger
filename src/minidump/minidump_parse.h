// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef MINIDUMP_PARSE_H
#define MINIDUMP_PARSE_H

typedef struct MDMP_ThreadArray MDMP_ThreadArray;
struct MDMP_ThreadArray
{
  MDMP_Thread *v;
  U64 count;
};

typedef struct MDMP_ModuleArray MDMP_ModuleArray;
struct MDMP_ModuleArray
{
  MDMP_Module *v;
  U64 count;
};

typedef struct MDMP_MemoryArray MDMP_MemoryArray;
struct MDMP_MemoryArray
{
  MDMP_MemoryDescriptor64 *v;
  U64 count;
};

#endif // MINIDUMP_PARSE_H
