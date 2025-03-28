// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef DEMON_META_H
#define DEMON_META_H

typedef enum DMN_EventKind
{
DMN_EventKind_Null,
DMN_EventKind_Error,
DMN_EventKind_HandshakeComplete,
DMN_EventKind_CreateProcess,
DMN_EventKind_ExitProcess,
DMN_EventKind_CreateThread,
DMN_EventKind_ExitThread,
DMN_EventKind_LoadModule,
DMN_EventKind_UnloadModule,
DMN_EventKind_Breakpoint,
DMN_EventKind_Trap,
DMN_EventKind_SingleStep,
DMN_EventKind_Exception,
DMN_EventKind_Halt,
DMN_EventKind_Memory,
DMN_EventKind_DebugString,
DMN_EventKind_SetThreadName,
DMN_EventKind_SetThreadColor,
DMN_EventKind_COUNT,
} DMN_EventKind;

typedef enum DMN_ErrorKind
{
DMN_ErrorKind_Null,
DMN_ErrorKind_NotAttached,
DMN_ErrorKind_UnexpectedFailure,
DMN_ErrorKind_InvalidHandle,
DMN_ErrorKind_COUNT,
} DMN_ErrorKind;

typedef enum DMN_MemoryEventKind
{
DMN_MemoryEventKind_Null,
DMN_MemoryEventKind_Commit,
DMN_MemoryEventKind_Reserve,
DMN_MemoryEventKind_Decommit,
DMN_MemoryEventKind_Release,
DMN_MemoryEventKind_COUNT,
} DMN_MemoryEventKind;

typedef enum DMN_ExceptionKind
{
DMN_ExceptionKind_Null,
DMN_ExceptionKind_MemoryRead,
DMN_ExceptionKind_MemoryWrite,
DMN_ExceptionKind_MemoryExecute,
DMN_ExceptionKind_CppThrow,
DMN_ExceptionKind_COUNT,
} DMN_ExceptionKind;

C_LINKAGE_BEGIN
extern String8 dmn_event_kind_string_table[18];
extern String8 dmn_exception_kind_string_table[5];

C_LINKAGE_END

#endif // DEMON_META_H
