// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef DBG_ENGINE_META_H
#define DBG_ENGINE_META_H

typedef enum D_CfgSrc
{
D_CfgSrc_User,
D_CfgSrc_Project,
D_CfgSrc_CommandLine,
D_CfgSrc_Transient,
D_CfgSrc_COUNT,
} D_CfgSrc;

typedef enum DF_EntityKind
{
DF_EntityKind_Nil,
DF_EntityKind_Root,
DF_EntityKind_Machine,
DF_EntityKind_File,
DF_EntityKind_AutoViewRule,
DF_EntityKind_FilePathMap,
DF_EntityKind_WatchPin,
DF_EntityKind_Watch,
DF_EntityKind_ViewRule,
DF_EntityKind_Breakpoint,
DF_EntityKind_Condition,
DF_EntityKind_Location,
DF_EntityKind_Target,
DF_EntityKind_Executable,
DF_EntityKind_Arguments,
DF_EntityKind_WorkingDirectory,
DF_EntityKind_EntryPoint,
DF_EntityKind_Window,
DF_EntityKind_Panel,
DF_EntityKind_View,
DF_EntityKind_RecentProject,
DF_EntityKind_Source,
DF_EntityKind_Dest,
DF_EntityKind_Process,
DF_EntityKind_Thread,
DF_EntityKind_Module,
DF_EntityKind_PendingThreadName,
DF_EntityKind_DebugInfoPath,
DF_EntityKind_ConversionTask,
DF_EntityKind_ConversionFail,
DF_EntityKind_COUNT,
} DF_EntityKind;

typedef enum D_CmdKind
{
D_CmdKind_Null,
D_CmdKind_LaunchAndRun,
D_CmdKind_LaunchAndInit,
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
D_CmdKind_RunToAddress,
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
D_CmdKind_Attach,
D_CmdKind_COUNT,
} D_CmdKind;

typedef enum D_ViewRuleKind
{
D_ViewRuleKind_Default,
D_ViewRuleKind_Array,
D_ViewRuleKind_Slice,
D_ViewRuleKind_List,
D_ViewRuleKind_ByteSwap,
D_ViewRuleKind_Cast,
D_ViewRuleKind_BaseDec,
D_ViewRuleKind_BaseBin,
D_ViewRuleKind_BaseOct,
D_ViewRuleKind_BaseHex,
D_ViewRuleKind_Only,
D_ViewRuleKind_Omit,
D_ViewRuleKind_NoAddr,
D_ViewRuleKind_Checkbox,
D_ViewRuleKind_ColorRGBA,
D_ViewRuleKind_Text,
D_ViewRuleKind_Disasm,
D_ViewRuleKind_Memory,
D_ViewRuleKind_Graph,
D_ViewRuleKind_Bitmap,
D_ViewRuleKind_Geo3D,
D_ViewRuleKind_COUNT,
} D_ViewRuleKind;

global B32 DEV_telemetry_capture = 0;
global B32 DEV_simulate_lag = 0;
global B32 DEV_draw_ui_text_pos = 0;
global B32 DEV_draw_ui_focus_debug = 0;
global B32 DEV_draw_ui_box_heatmap = 0;
global B32 DEV_eval_compiler_tooltips = 0;
global B32 DEV_eval_watch_key_tooltips = 0;
global B32 DEV_cmd_context_tooltips = 0;
global B32 DEV_scratch_mouse_draw = 0;
global B32 DEV_updating_indicator = 0;
struct {B32 *value_ptr; String8 name;} DEV_toggle_table[] =
{
{&DEV_telemetry_capture, str8_lit_comp("telemetry_capture")},
{&DEV_simulate_lag, str8_lit_comp("simulate_lag")},
{&DEV_draw_ui_text_pos, str8_lit_comp("draw_ui_text_pos")},
{&DEV_draw_ui_focus_debug, str8_lit_comp("draw_ui_focus_debug")},
{&DEV_draw_ui_box_heatmap, str8_lit_comp("draw_ui_box_heatmap")},
{&DEV_eval_compiler_tooltips, str8_lit_comp("eval_compiler_tooltips")},
{&DEV_eval_watch_key_tooltips, str8_lit_comp("eval_watch_key_tooltips")},
{&DEV_cmd_context_tooltips, str8_lit_comp("cmd_context_tooltips")},
{&DEV_scratch_mouse_draw, str8_lit_comp("scratch_mouse_draw")},
{&DEV_updating_indicator, str8_lit_comp("updating_indicator")},
};
C_LINKAGE_BEGIN
extern String8 d_cfg_src_string_table[4];
extern String8 d_entity_kind_display_string_table[30];
extern String8 d_entity_kind_name_lower_table[30];
extern String8 d_entity_kind_name_lower_plural_table[30];
extern String8 d_entity_kind_name_label_table[30];
extern DF_EntityKindFlags d_entity_kind_flags_table[30];

C_LINKAGE_END

#endif // DBG_ENGINE_META_H
