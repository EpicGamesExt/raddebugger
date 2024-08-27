// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef DF_CORE_META_H
#define DF_CORE_META_H

typedef enum DF_CfgSrc
{
DF_CfgSrc_User,
DF_CfgSrc_Project,
DF_CfgSrc_CommandLine,
DF_CfgSrc_Transient,
DF_CfgSrc_COUNT,
} DF_CfgSrc;

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
DF_EntityKind_EndedProcess,
DF_EntityKind_COUNT,
} DF_EntityKind;

typedef enum DF_CoreCmdKind
{
DF_CoreCmdKind_Null,
DF_CoreCmdKind_Exit,
DF_CoreCmdKind_RunCommand,
DF_CoreCmdKind_Error,
DF_CoreCmdKind_OSEvent,
DF_CoreCmdKind_LaunchAndRun,
DF_CoreCmdKind_LaunchAndInit,
DF_CoreCmdKind_Kill,
DF_CoreCmdKind_KillAll,
DF_CoreCmdKind_Detach,
DF_CoreCmdKind_Continue,
DF_CoreCmdKind_StepIntoInst,
DF_CoreCmdKind_StepOverInst,
DF_CoreCmdKind_StepIntoLine,
DF_CoreCmdKind_StepOverLine,
DF_CoreCmdKind_StepOut,
DF_CoreCmdKind_Halt,
DF_CoreCmdKind_SoftHaltRefresh,
DF_CoreCmdKind_SetThreadIP,
DF_CoreCmdKind_RunToLine,
DF_CoreCmdKind_RunToAddress,
DF_CoreCmdKind_Run,
DF_CoreCmdKind_Restart,
DF_CoreCmdKind_StepInto,
DF_CoreCmdKind_StepOver,
DF_CoreCmdKind_RunToCursor,
DF_CoreCmdKind_SetNextStatement,
DF_CoreCmdKind_SelectThread,
DF_CoreCmdKind_SelectUnwind,
DF_CoreCmdKind_UpOneFrame,
DF_CoreCmdKind_DownOneFrame,
DF_CoreCmdKind_FreezeThread,
DF_CoreCmdKind_ThawThread,
DF_CoreCmdKind_FreezeProcess,
DF_CoreCmdKind_ThawProcess,
DF_CoreCmdKind_FreezeMachine,
DF_CoreCmdKind_ThawMachine,
DF_CoreCmdKind_FreezeLocalMachine,
DF_CoreCmdKind_ThawLocalMachine,
DF_CoreCmdKind_IncUIFontScale,
DF_CoreCmdKind_DecUIFontScale,
DF_CoreCmdKind_IncCodeFontScale,
DF_CoreCmdKind_DecCodeFontScale,
DF_CoreCmdKind_OpenWindow,
DF_CoreCmdKind_CloseWindow,
DF_CoreCmdKind_ToggleFullscreen,
DF_CoreCmdKind_ConfirmAccept,
DF_CoreCmdKind_ConfirmCancel,
DF_CoreCmdKind_ResetToDefaultPanels,
DF_CoreCmdKind_ResetToCompactPanels,
DF_CoreCmdKind_NewPanelLeft,
DF_CoreCmdKind_NewPanelUp,
DF_CoreCmdKind_NewPanelRight,
DF_CoreCmdKind_NewPanelDown,
DF_CoreCmdKind_SplitPanel,
DF_CoreCmdKind_RotatePanelColumns,
DF_CoreCmdKind_NextPanel,
DF_CoreCmdKind_PrevPanel,
DF_CoreCmdKind_FocusPanel,
DF_CoreCmdKind_FocusPanelRight,
DF_CoreCmdKind_FocusPanelLeft,
DF_CoreCmdKind_FocusPanelUp,
DF_CoreCmdKind_FocusPanelDown,
DF_CoreCmdKind_Undo,
DF_CoreCmdKind_Redo,
DF_CoreCmdKind_GoBack,
DF_CoreCmdKind_GoForward,
DF_CoreCmdKind_ClosePanel,
DF_CoreCmdKind_NextTab,
DF_CoreCmdKind_PrevTab,
DF_CoreCmdKind_MoveTabRight,
DF_CoreCmdKind_MoveTabLeft,
DF_CoreCmdKind_OpenTab,
DF_CoreCmdKind_CloseTab,
DF_CoreCmdKind_MoveTab,
DF_CoreCmdKind_TabBarTop,
DF_CoreCmdKind_TabBarBottom,
DF_CoreCmdKind_SetCurrentPath,
DF_CoreCmdKind_Open,
DF_CoreCmdKind_Switch,
DF_CoreCmdKind_SwitchToPartnerFile,
DF_CoreCmdKind_GoToDisassembly,
DF_CoreCmdKind_GoToSource,
DF_CoreCmdKind_SetFileOverrideLinkSrc,
DF_CoreCmdKind_SetFileOverrideLinkDst,
DF_CoreCmdKind_SetFileReplacementPath,
DF_CoreCmdKind_SetAutoViewRuleType,
DF_CoreCmdKind_SetAutoViewRuleViewRule,
DF_CoreCmdKind_OpenUser,
DF_CoreCmdKind_OpenProject,
DF_CoreCmdKind_OpenRecentProject,
DF_CoreCmdKind_ApplyUserData,
DF_CoreCmdKind_ApplyProjectData,
DF_CoreCmdKind_WriteUserData,
DF_CoreCmdKind_WriteProjectData,
DF_CoreCmdKind_Edit,
DF_CoreCmdKind_Accept,
DF_CoreCmdKind_Cancel,
DF_CoreCmdKind_MoveLeft,
DF_CoreCmdKind_MoveRight,
DF_CoreCmdKind_MoveUp,
DF_CoreCmdKind_MoveDown,
DF_CoreCmdKind_MoveLeftSelect,
DF_CoreCmdKind_MoveRightSelect,
DF_CoreCmdKind_MoveUpSelect,
DF_CoreCmdKind_MoveDownSelect,
DF_CoreCmdKind_MoveLeftChunk,
DF_CoreCmdKind_MoveRightChunk,
DF_CoreCmdKind_MoveUpChunk,
DF_CoreCmdKind_MoveDownChunk,
DF_CoreCmdKind_MoveUpPage,
DF_CoreCmdKind_MoveDownPage,
DF_CoreCmdKind_MoveUpWhole,
DF_CoreCmdKind_MoveDownWhole,
DF_CoreCmdKind_MoveLeftChunkSelect,
DF_CoreCmdKind_MoveRightChunkSelect,
DF_CoreCmdKind_MoveUpChunkSelect,
DF_CoreCmdKind_MoveDownChunkSelect,
DF_CoreCmdKind_MoveUpPageSelect,
DF_CoreCmdKind_MoveDownPageSelect,
DF_CoreCmdKind_MoveUpWholeSelect,
DF_CoreCmdKind_MoveDownWholeSelect,
DF_CoreCmdKind_MoveUpReorder,
DF_CoreCmdKind_MoveDownReorder,
DF_CoreCmdKind_MoveHome,
DF_CoreCmdKind_MoveEnd,
DF_CoreCmdKind_MoveHomeSelect,
DF_CoreCmdKind_MoveEndSelect,
DF_CoreCmdKind_SelectAll,
DF_CoreCmdKind_DeleteSingle,
DF_CoreCmdKind_DeleteChunk,
DF_CoreCmdKind_BackspaceSingle,
DF_CoreCmdKind_BackspaceChunk,
DF_CoreCmdKind_Copy,
DF_CoreCmdKind_Cut,
DF_CoreCmdKind_Paste,
DF_CoreCmdKind_InsertText,
DF_CoreCmdKind_GoToLine,
DF_CoreCmdKind_GoToAddress,
DF_CoreCmdKind_CenterCursor,
DF_CoreCmdKind_ContainCursor,
DF_CoreCmdKind_FindTextForward,
DF_CoreCmdKind_FindTextBackward,
DF_CoreCmdKind_FindNext,
DF_CoreCmdKind_FindPrev,
DF_CoreCmdKind_FindThread,
DF_CoreCmdKind_FindSelectedThread,
DF_CoreCmdKind_GoToName,
DF_CoreCmdKind_GoToNameAtCursor,
DF_CoreCmdKind_ToggleWatchExpression,
DF_CoreCmdKind_ToggleWatchExpressionAtCursor,
DF_CoreCmdKind_ToggleWatchExpressionAtMouse,
DF_CoreCmdKind_SetColumns,
DF_CoreCmdKind_ToggleAddressVisibility,
DF_CoreCmdKind_ToggleCodeBytesVisibility,
DF_CoreCmdKind_EnableEntity,
DF_CoreCmdKind_DisableEntity,
DF_CoreCmdKind_FreezeEntity,
DF_CoreCmdKind_ThawEntity,
DF_CoreCmdKind_RemoveEntity,
DF_CoreCmdKind_NameEntity,
DF_CoreCmdKind_EditEntity,
DF_CoreCmdKind_DuplicateEntity,
DF_CoreCmdKind_RelocateEntity,
DF_CoreCmdKind_AddBreakpoint,
DF_CoreCmdKind_AddAddressBreakpoint,
DF_CoreCmdKind_AddFunctionBreakpoint,
DF_CoreCmdKind_ToggleBreakpoint,
DF_CoreCmdKind_RemoveBreakpoint,
DF_CoreCmdKind_EnableBreakpoint,
DF_CoreCmdKind_DisableBreakpoint,
DF_CoreCmdKind_AddWatchPin,
DF_CoreCmdKind_ToggleWatchPin,
DF_CoreCmdKind_ToggleBreakpointAtCursor,
DF_CoreCmdKind_ToggleWatchPinAtCursor,
DF_CoreCmdKind_AddTarget,
DF_CoreCmdKind_RemoveTarget,
DF_CoreCmdKind_EditTarget,
DF_CoreCmdKind_SelectTarget,
DF_CoreCmdKind_EnableTarget,
DF_CoreCmdKind_DisableTarget,
DF_CoreCmdKind_RetryEndedProcess,
DF_CoreCmdKind_Attach,
DF_CoreCmdKind_RegisterAsJITDebugger,
DF_CoreCmdKind_EntityRefFastPath,
DF_CoreCmdKind_SpawnEntityView,
DF_CoreCmdKind_FindCodeLocation,
DF_CoreCmdKind_Filter,
DF_CoreCmdKind_ApplyFilter,
DF_CoreCmdKind_ClearFilter,
DF_CoreCmdKind_GettingStarted,
DF_CoreCmdKind_Commands,
DF_CoreCmdKind_Target,
DF_CoreCmdKind_Targets,
DF_CoreCmdKind_FilePathMap,
DF_CoreCmdKind_AutoViewRules,
DF_CoreCmdKind_Breakpoints,
DF_CoreCmdKind_WatchPins,
DF_CoreCmdKind_Scheduler,
DF_CoreCmdKind_CallStack,
DF_CoreCmdKind_Modules,
DF_CoreCmdKind_Watch,
DF_CoreCmdKind_Locals,
DF_CoreCmdKind_Registers,
DF_CoreCmdKind_Globals,
DF_CoreCmdKind_ThreadLocals,
DF_CoreCmdKind_Types,
DF_CoreCmdKind_Procedures,
DF_CoreCmdKind_PendingFile,
DF_CoreCmdKind_Disassembly,
DF_CoreCmdKind_Output,
DF_CoreCmdKind_Memory,
DF_CoreCmdKind_ExceptionFilters,
DF_CoreCmdKind_Settings,
DF_CoreCmdKind_PickFile,
DF_CoreCmdKind_PickFolder,
DF_CoreCmdKind_PickFileOrFolder,
DF_CoreCmdKind_CompleteQuery,
DF_CoreCmdKind_CancelQuery,
DF_CoreCmdKind_ToggleDevMenu,
DF_CoreCmdKind_LogMarker,
DF_CoreCmdKind_COUNT,
} DF_CoreCmdKind;

typedef enum DF_IconKind
{
DF_IconKind_Null,
DF_IconKind_FolderOpenOutline,
DF_IconKind_FolderClosedOutline,
DF_IconKind_FolderOpenFilled,
DF_IconKind_FolderClosedFilled,
DF_IconKind_FileOutline,
DF_IconKind_FileFilled,
DF_IconKind_Play,
DF_IconKind_PlayStepForward,
DF_IconKind_Pause,
DF_IconKind_Stop,
DF_IconKind_Info,
DF_IconKind_WarningSmall,
DF_IconKind_WarningBig,
DF_IconKind_Unlocked,
DF_IconKind_Locked,
DF_IconKind_LeftArrow,
DF_IconKind_RightArrow,
DF_IconKind_UpArrow,
DF_IconKind_DownArrow,
DF_IconKind_Gear,
DF_IconKind_Pencil,
DF_IconKind_Trash,
DF_IconKind_Pin,
DF_IconKind_RadioHollow,
DF_IconKind_RadioFilled,
DF_IconKind_CheckHollow,
DF_IconKind_CheckFilled,
DF_IconKind_LeftCaret,
DF_IconKind_RightCaret,
DF_IconKind_UpCaret,
DF_IconKind_DownCaret,
DF_IconKind_UpScroll,
DF_IconKind_DownScroll,
DF_IconKind_LeftScroll,
DF_IconKind_RightScroll,
DF_IconKind_Add,
DF_IconKind_Minus,
DF_IconKind_Thread,
DF_IconKind_Threads,
DF_IconKind_Machine,
DF_IconKind_CircleFilled,
DF_IconKind_X,
DF_IconKind_Refresh,
DF_IconKind_Undo,
DF_IconKind_Redo,
DF_IconKind_Save,
DF_IconKind_Window,
DF_IconKind_Target,
DF_IconKind_Clipboard,
DF_IconKind_Scheduler,
DF_IconKind_Module,
DF_IconKind_XSplit,
DF_IconKind_YSplit,
DF_IconKind_ClosePanel,
DF_IconKind_StepInto,
DF_IconKind_StepOver,
DF_IconKind_StepOut,
DF_IconKind_Find,
DF_IconKind_Palette,
DF_IconKind_Thumbnails,
DF_IconKind_Glasses,
DF_IconKind_Binoculars,
DF_IconKind_List,
DF_IconKind_Grid,
DF_IconKind_QuestionMark,
DF_IconKind_Person,
DF_IconKind_Briefcase,
DF_IconKind_Dot,
DF_IconKind_COUNT,
} DF_IconKind;

typedef enum DF_CoreViewRuleKind
{
DF_CoreViewRuleKind_Default,
DF_CoreViewRuleKind_Array,
DF_CoreViewRuleKind_Slice,
DF_CoreViewRuleKind_List,
DF_CoreViewRuleKind_ByteSwap,
DF_CoreViewRuleKind_Cast,
DF_CoreViewRuleKind_BaseDec,
DF_CoreViewRuleKind_BaseBin,
DF_CoreViewRuleKind_BaseOct,
DF_CoreViewRuleKind_BaseHex,
DF_CoreViewRuleKind_Only,
DF_CoreViewRuleKind_Omit,
DF_CoreViewRuleKind_NoAddr,
DF_CoreViewRuleKind_Checkbox,
DF_CoreViewRuleKind_RGBA,
DF_CoreViewRuleKind_Text,
DF_CoreViewRuleKind_Disasm,
DF_CoreViewRuleKind_Memory,
DF_CoreViewRuleKind_Graph,
DF_CoreViewRuleKind_Bitmap,
DF_CoreViewRuleKind_Geo,
DF_CoreViewRuleKind_COUNT,
} DF_CoreViewRuleKind;

typedef enum DF_CmdParamSlot
{
DF_CmdParamSlot_Null,
DF_CmdParamSlot_Window,
DF_CmdParamSlot_Panel,
DF_CmdParamSlot_DestPanel,
DF_CmdParamSlot_PrevView,
DF_CmdParamSlot_View,
DF_CmdParamSlot_Entity,
DF_CmdParamSlot_EntityList,
DF_CmdParamSlot_String,
DF_CmdParamSlot_FilePath,
DF_CmdParamSlot_TextPoint,
DF_CmdParamSlot_CmdSpec,
DF_CmdParamSlot_ViewSpec,
DF_CmdParamSlot_ParamsTree,
DF_CmdParamSlot_OSEvent,
DF_CmdParamSlot_VirtualAddr,
DF_CmdParamSlot_VirtualOff,
DF_CmdParamSlot_Index,
DF_CmdParamSlot_ID,
DF_CmdParamSlot_PreferDisassembly,
DF_CmdParamSlot_ForceConfirm,
DF_CmdParamSlot_Dir2,
DF_CmdParamSlot_UnwindIndex,
DF_CmdParamSlot_InlineDepth,
DF_CmdParamSlot_COUNT,
} DF_CmdParamSlot;

typedef struct DF_CmdParams DF_CmdParams;
struct DF_CmdParams
{
U64 slot_props[(DF_CmdParamSlot_COUNT + 63) / 64];
DF_Handle window;
DF_Handle panel;
DF_Handle dest_panel;
DF_Handle prev_view;
DF_Handle view;
DF_Handle entity;
DF_HandleList entity_list;
String8 string;
String8 file_path;
TxtPt text_point;
struct DF_CmdSpec * cmd_spec;
struct DF_ViewSpec * view_spec;
MD_Node * params_tree;
struct OS_Event * os_event;
U64 vaddr;
U64 voff;
U64 index;
U64 id;
B32 prefer_dasm;
B32 force_confirm;
Dir2 dir2;
U64 unwind_index;
U64 inline_depth;
};

DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(array);
DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(slice);
DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(bswap);
DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(cast);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(default);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(list);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(only);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(omit);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(rgba);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(text);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(disasm);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(memory);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(graph);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(bitmap);
DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(geo);
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
extern Rng1U64 df_g_cmd_param_slot_range_table[24];
extern DF_IconKind df_g_entity_kind_icon_kind_table[28];
extern String8 df_g_entity_kind_display_string_table[28];
extern String8 df_g_entity_kind_name_lower_table[28];
extern String8 df_g_entity_kind_name_lower_plural_table[28];
extern String8 df_g_entity_kind_name_label_table[28];
extern DF_EntityKindFlags df_g_entity_kind_flags_table[28];
extern String8 df_g_cfg_src_string_table[4];
extern DF_CoreCmdKind df_g_cfg_src_load_cmd_kind_table[4];
extern DF_CoreCmdKind df_g_cfg_src_write_cmd_kind_table[4];
extern DF_CoreCmdKind df_g_cfg_src_apply_cmd_kind_table[4];
extern String8 df_g_icon_kind_text_table[69];

C_LINKAGE_END

#endif // DF_CORE_META_H
