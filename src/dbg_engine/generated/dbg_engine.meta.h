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

typedef enum D_EntityKind
{
D_EntityKind_Null,
D_EntityKind_Root,
D_EntityKind_Machine,
D_EntityKind_Process,
D_EntityKind_Thread,
D_EntityKind_Module,
D_EntityKind_EntryPoint,
D_EntityKind_DebugInfoPath,
D_EntityKind_PendingThreadName,
D_EntityKind_PendingThreadColor,
D_EntityKind_Breakpoint,
D_EntityKind_AddressRangeAnnotation,
D_EntityKind_COUNT,
} D_EntityKind;

typedef enum D_ExceptionCodeKind
{
D_ExceptionCodeKind_Null,
D_ExceptionCodeKind_Win32CtrlC,
D_ExceptionCodeKind_Win32CtrlBreak,
D_ExceptionCodeKind_Win32WinRTOriginateError,
D_ExceptionCodeKind_Win32WinRTTransformError,
D_ExceptionCodeKind_Win32RPCCallCancelled,
D_ExceptionCodeKind_Win32DatatypeMisalignment,
D_ExceptionCodeKind_Win32AccessViolation,
D_ExceptionCodeKind_Win32InPageError,
D_ExceptionCodeKind_Win32InvalidHandle,
D_ExceptionCodeKind_Win32NotEnoughQuota,
D_ExceptionCodeKind_Win32IllegalInstruction,
D_ExceptionCodeKind_Win32CannotContinueException,
D_ExceptionCodeKind_Win32InvalidExceptionDisposition,
D_ExceptionCodeKind_Win32ArrayBoundsExceeded,
D_ExceptionCodeKind_Win32FloatingPointDenormalOperand,
D_ExceptionCodeKind_Win32FloatingPointDivisionByZero,
D_ExceptionCodeKind_Win32FloatingPointInexactResult,
D_ExceptionCodeKind_Win32FloatingPointInvalidOperation,
D_ExceptionCodeKind_Win32FloatingPointOverflow,
D_ExceptionCodeKind_Win32FloatingPointStackCheck,
D_ExceptionCodeKind_Win32FloatingPointUnderflow,
D_ExceptionCodeKind_Win32IntegerDivisionByZero,
D_ExceptionCodeKind_Win32IntegerOverflow,
D_ExceptionCodeKind_Win32PrivilegedInstruction,
D_ExceptionCodeKind_Win32StackOverflow,
D_ExceptionCodeKind_Win32UnableToLocateDLL,
D_ExceptionCodeKind_Win32OrdinalNotFound,
D_ExceptionCodeKind_Win32EntryPointNotFound,
D_ExceptionCodeKind_Win32DLLInitializationFailed,
D_ExceptionCodeKind_Win32FloatingPointSSEMultipleFaults,
D_ExceptionCodeKind_Win32FloatingPointSSEMultipleTraps,
D_ExceptionCodeKind_Win32AssertionFailed,
D_ExceptionCodeKind_Win32ModuleNotFound,
D_ExceptionCodeKind_Win32ProcedureNotFound,
D_ExceptionCodeKind_Win32SanitizerErrorDetected,
D_ExceptionCodeKind_Win32SanitizerRawAccessViolation,
D_ExceptionCodeKind_Win32DirectXDebugLayer,
D_ExceptionCodeKind_COUNT,
} D_ExceptionCodeKind;

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
