// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_PROFILE_H
#define BASE_PROFILE_H

////////////////////////////////
//~ rjf: Zero Settings

#if !defined(PROFILE_TELEMETRY)
# define PROFILE_TELEMETRY 0
#endif

////////////////////////////////
//~ rjf: Third Party Includes

#if PROFILE_TELEMETRY
# include "rad_tm.h"
# if OS_WINDOWS
#  pragma comment(lib, "rad_tm_win64.lib")
# endif
#endif

////////////////////////////////
//~ rjf: Telemetry Profile Defines

#if PROFILE_TELEMETRY
# define ProfBegin(...)            tmEnter(0, 0, __VA_ARGS__)
# define ProfBeginDynamic(...)     (TM_API_PTR ? TM_API_PTR->_tmEnterZoneV_Core(0, 0, __FILE__, &g_telemetry_filename_id, __LINE__, __VA_ARGS__) : (void)0)
# define ProfEnd(...)              (TM_API_PTR ? TM_API_PTR->_tmLeaveZone(0) : (void)0)
# define ProfTick(...)             tmTick(0)
# define ProfIsCapturing(...)      tmRunning()
# define ProfBeginCapture(...)     tmOpen(0, __VA_ARGS__, __DATE__, "localhost", TMCT_TCP, TELEMETRY_DEFAULT_PORT, TMOF_INIT_NETWORKING|TMOF_CAPTURE_CONTEXT_SWITCHES, 100)
# define ProfEndCapture(...)       tmClose(0)
# define ProfThreadName(...)       (TM_API_PTR ? TM_API_PTR->_tmThreadName(0, 0, __VA_ARGS__) : (void)0)
# define ProfMsg(...)              (TM_API_PTR ? TM_API_PTR->_tmMessageV_Core(0, TMMF_ICON_NOTE, __FILE__, &g_telemetry_filename_id, __LINE__, __VA_ARGS__) : (void)0)
# define ProfBeginLockWait(...)    tmStartWaitForLock(0, 0, __VA_ARGS__)
# define ProfEndLockWait(...)      tmEndWaitForLock(0)
# define ProfLockTake(...)         tmAcquiredLock(0, 0, __VA_ARGS__)
# define ProfLockDrop(...)         tmReleasedLock(0, __VA_ARGS__)
# define ProfColor(color)          tmZoneColorSticky(color)
# define ProfBeginV(...)                                                           \
if (TM_API_PTR) {                                                                \
static tm_uint64 file_id = 0; TM_API_PTR->_tmStaticString(&file_id, __FILE__); \
Temp scratch = scratch_begin(0,0);                                             \
String8 string = push_str8f(scratch.arena, __VA_ARGS__);                       \
tm_uint64 hash = TM_API_PTR->_tmHash((char*)string.str, string.size);          \
hash = TM_API_PTR->_tmSendDynamicString(hash, (char*)string.str);              \
TM_API_PTR->_tmEnterZoneFast_Core(0, 0, file_id, __LINE__, hash);              \
scratch_end(scratch);                                                          \
}
# define ProfNoteV(...) 														   \
if (TM_API_PTR) {     														   \
static tm_uint64 file_id = 0; TM_API_PTR->_tmStaticString(&file_id, __FILE__); \
Temp scratch = scratch_begin(0,0);                                             \
String8 string = push_str8f(scratch.arena, __VA_ARGS__);                       \
tm_uint64 hash = TM_API_PTR->_tmHash((char*)string.str, string.size);          \
hash = TM_API_PTR->_tmSendDynamicString(hash, (char*)string.str);              \
TM_API_PTR->_tmMessageFast_Core(0, TMMF_ICON_NOTE, file_id, __LINE__, hash);   \
scratch_end(scratch);                                                          \
}
#endif

////////////////////////////////
//~ rjf: Zeroify Undefined Defines

#if !defined(ProfBegin)
# define ProfBegin(...)         (0)
# define ProfBeginDynamic(...)  (0)
# define ProfEnd(...)           (0)
# define ProfTick(...)          (0)
# define ProfIsCapturing(...)   (0)
# define ProfBeginCapture(...)  (0)
# define ProfEndCapture(...)    (0)
# define ProfThreadName(...)    (0)
# define ProfMsg(...)           (0)
# define ProfBeginLockWait(...) (0)
# define ProfEndLockWait(...)   (0)
# define ProfLockTake(...)      (0)
# define ProfLockDrop(...)      (0)
# define ProfColor(...)         (0)
# define ProfBeginV(...)        (0)
# define ProfNoteV(...)         (0)
#endif

////////////////////////////////
//~ rjf: Helper Wrappers

#define ProfBeginFunction(...) ProfBegin(this_function_name)
#define ProfScope(...) DeferLoop(ProfBeginDynamic(__VA_ARGS__), ProfEnd())

#endif // BASE_PROFILE_H
