// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef DBG_ENGINE_META_H
#define DBG_ENGINE_META_H

typedef enum D_CmdKind
{
D_CmdKind_Null,
D_CmdKind_LaunchAndRun,
D_CmdKind_LaunchAndStepInto,
D_CmdKind_Kill,
D_CmdKind_KillAll,
D_CmdKind_Detach,
D_CmdKind_Continue,
D_CmdKind_StepIntoInst,
D_CmdKind_StepOverInst,
D_CmdKind_StepIntoLine,
D_CmdKind_StepOverLine,
D_CmdKind_StepOut,
D_CmdKind_Halt,
D_CmdKind_SoftHaltRefresh,
D_CmdKind_SetThreadIP,
D_CmdKind_RunToLine,
D_CmdKind_Run,
D_CmdKind_Restart,
D_CmdKind_StepInto,
D_CmdKind_StepOver,
D_CmdKind_FreezeThread,
D_CmdKind_ThawThread,
D_CmdKind_FreezeProcess,
D_CmdKind_ThawProcess,
D_CmdKind_FreezeMachine,
D_CmdKind_ThawMachine,
D_CmdKind_FreezeLocalMachine,
D_CmdKind_ThawLocalMachine,
D_CmdKind_FreezeEntity,
D_CmdKind_ThawEntity,
D_CmdKind_SetEntityColor,
D_CmdKind_SetEntityName,
D_CmdKind_Attach,
D_CmdKind_COUNT,
} D_CmdKind;

typedef enum CTRL_EntityKind
{
CTRL_EntityKind_Null,
CTRL_EntityKind_Root,
CTRL_EntityKind_Machine,
CTRL_EntityKind_Process,
CTRL_EntityKind_Thread,
CTRL_EntityKind_Module,
CTRL_EntityKind_EntryPoint,
CTRL_EntityKind_DebugInfoPath,
CTRL_EntityKind_PendingThreadName,
CTRL_EntityKind_PendingThreadColor,
CTRL_EntityKind_Breakpoint,
CTRL_EntityKind_AddressRangeAnnotation,
CTRL_EntityKind_COUNT,
} CTRL_EntityKind;

typedef enum CTRL_ExceptionCodeKind
{
CTRL_ExceptionCodeKind_Null,
CTRL_ExceptionCodeKind_Win32CtrlC,
CTRL_ExceptionCodeKind_Win32CtrlBreak,
CTRL_ExceptionCodeKind_Win32WinRTOriginateError,
CTRL_ExceptionCodeKind_Win32WinRTTransformError,
CTRL_ExceptionCodeKind_Win32RPCCallCancelled,
CTRL_ExceptionCodeKind_Win32DatatypeMisalignment,
CTRL_ExceptionCodeKind_Win32AccessViolation,
CTRL_ExceptionCodeKind_Win32InPageError,
CTRL_ExceptionCodeKind_Win32InvalidHandle,
CTRL_ExceptionCodeKind_Win32NotEnoughQuota,
CTRL_ExceptionCodeKind_Win32IllegalInstruction,
CTRL_ExceptionCodeKind_Win32CannotContinueException,
CTRL_ExceptionCodeKind_Win32InvalidExceptionDisposition,
CTRL_ExceptionCodeKind_Win32ArrayBoundsExceeded,
CTRL_ExceptionCodeKind_Win32FloatingPointDenormalOperand,
CTRL_ExceptionCodeKind_Win32FloatingPointDivisionByZero,
CTRL_ExceptionCodeKind_Win32FloatingPointInexactResult,
CTRL_ExceptionCodeKind_Win32FloatingPointInvalidOperation,
CTRL_ExceptionCodeKind_Win32FloatingPointOverflow,
CTRL_ExceptionCodeKind_Win32FloatingPointStackCheck,
CTRL_ExceptionCodeKind_Win32FloatingPointUnderflow,
CTRL_ExceptionCodeKind_Win32IntegerDivisionByZero,
CTRL_ExceptionCodeKind_Win32IntegerOverflow,
CTRL_ExceptionCodeKind_Win32PrivilegedInstruction,
CTRL_ExceptionCodeKind_Win32StackOverflow,
CTRL_ExceptionCodeKind_Win32UnableToLocateDLL,
CTRL_ExceptionCodeKind_Win32OrdinalNotFound,
CTRL_ExceptionCodeKind_Win32EntryPointNotFound,
CTRL_ExceptionCodeKind_Win32DLLInitializationFailed,
CTRL_ExceptionCodeKind_Win32FloatingPointSSEMultipleFaults,
CTRL_ExceptionCodeKind_Win32FloatingPointSSEMultipleTraps,
CTRL_ExceptionCodeKind_Win32AssertionFailed,
CTRL_ExceptionCodeKind_Win32ModuleNotFound,
CTRL_ExceptionCodeKind_Win32ProcedureNotFound,
CTRL_ExceptionCodeKind_Win32SanitizerErrorDetected,
CTRL_ExceptionCodeKind_Win32SanitizerRawAccessViolation,
CTRL_ExceptionCodeKind_Win32DirectXDebugLayer,
CTRL_ExceptionCodeKind_COUNT,
} CTRL_ExceptionCodeKind;

global B32 DEV_always_refresh = 0;
global B32 DEV_simulate_lag = 0;
global B32 DEV_draw_ui_text_pos = 0;
global B32 DEV_draw_ui_focus_debug = 0;
global B32 DEV_draw_ui_box_heatmap = 0;
global B32 DEV_eval_compiler_tooltips = 0;
global B32 DEV_eval_watch_key_tooltips = 0;
global B32 DEV_cmd_context_tooltips = 0;
global B32 DEV_updating_indicator = 0;
struct {B32 *value_ptr; String8 name;} DEV_toggle_table[] =
{
{&DEV_always_refresh, str8_lit_comp("always_refresh")},
{&DEV_simulate_lag, str8_lit_comp("simulate_lag")},
{&DEV_draw_ui_text_pos, str8_lit_comp("draw_ui_text_pos")},
{&DEV_draw_ui_focus_debug, str8_lit_comp("draw_ui_focus_debug")},
{&DEV_draw_ui_box_heatmap, str8_lit_comp("draw_ui_box_heatmap")},
{&DEV_eval_compiler_tooltips, str8_lit_comp("eval_compiler_tooltips")},
{&DEV_eval_watch_key_tooltips, str8_lit_comp("eval_watch_key_tooltips")},
{&DEV_cmd_context_tooltips, str8_lit_comp("cmd_context_tooltips")},
{&DEV_updating_indicator, str8_lit_comp("updating_indicator")},
};
C_LINKAGE_BEGIN
extern String8 ctrl_entity_kind_code_name_table[12];
extern String8 ctrl_entity_kind_display_string_table[12];
extern U32 ctrl_exception_code_kind_code_table[38];
extern String8 ctrl_exception_code_kind_display_string_table[38];
extern String8 ctrl_exception_code_kind_lowercase_code_string_table[38];
extern B8 ctrl_exception_code_kind_default_enable_table[38];

C_LINKAGE_END

#endif // DBG_ENGINE_META_H
