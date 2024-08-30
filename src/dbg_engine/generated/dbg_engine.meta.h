// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef DBG_ENGINE_META_H
#define DBG_ENGINE_META_H

typedef enum D_RegSlot
{
D_RegSlot_Module,
D_RegSlot_Process,
D_RegSlot_Thread,
D_RegSlot_Window,
D_RegSlot_Panel,
D_RegSlot_View,
D_RegSlot_PrevView,
D_RegSlot_DstPanel,
D_RegSlot_Entity,
D_RegSlot_EntityList,
D_RegSlot_UnwindCount,
D_RegSlot_InlineDepth,
D_RegSlot_FilePath,
D_RegSlot_Cursor,
D_RegSlot_Mark,
D_RegSlot_TextKey,
D_RegSlot_LangKind,
D_RegSlot_Lines,
D_RegSlot_DbgiKey,
D_RegSlot_VaddrRange,
D_RegSlot_VoffRange,
D_RegSlot_ForceConfirm,
D_RegSlot_PreferDisasm,
D_RegSlot_Dir2,
D_RegSlot_String,
D_RegSlot_ParamsTree,
} D_RegSlot;

typedef enum D_CfgSrc
{
D_CfgSrc_User,
D_CfgSrc_Project,
D_CfgSrc_CommandLine,
D_CfgSrc_Transient,
D_CfgSrc_COUNT,
} D_CfgSrc;

typedef enum D_EntityKind
{
D_EntityKind_Nil,
D_EntityKind_Root,
D_EntityKind_Machine,
D_EntityKind_File,
D_EntityKind_AutoViewRule,
D_EntityKind_FilePathMap,
D_EntityKind_WatchPin,
D_EntityKind_Watch,
D_EntityKind_ViewRule,
D_EntityKind_Breakpoint,
D_EntityKind_Condition,
D_EntityKind_Location,
D_EntityKind_Target,
D_EntityKind_Executable,
D_EntityKind_Arguments,
D_EntityKind_WorkingDirectory,
D_EntityKind_EntryPoint,
D_EntityKind_Window,
D_EntityKind_Panel,
D_EntityKind_View,
D_EntityKind_RecentProject,
D_EntityKind_Source,
D_EntityKind_Dest,
D_EntityKind_Process,
D_EntityKind_Thread,
D_EntityKind_Module,
D_EntityKind_PendingThreadName,
D_EntityKind_DebugInfoPath,
D_EntityKind_ConversionTask,
D_EntityKind_ConversionFail,
D_EntityKind_EndedProcess,
D_EntityKind_COUNT,
} D_EntityKind;

typedef enum D_CmdKind
{
D_CmdKind_Null,
D_CmdKind_Exit,
D_CmdKind_RunCommand,
D_CmdKind_Error,
D_CmdKind_OSEvent,
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
D_CmdKind_RunToCursor,
D_CmdKind_SetNextStatement,
D_CmdKind_SelectThread,
D_CmdKind_SelectUnwind,
D_CmdKind_UpOneFrame,
D_CmdKind_DownOneFrame,
D_CmdKind_FreezeThread,
D_CmdKind_ThawThread,
D_CmdKind_FreezeProcess,
D_CmdKind_ThawProcess,
D_CmdKind_FreezeMachine,
D_CmdKind_ThawMachine,
D_CmdKind_FreezeLocalMachine,
D_CmdKind_ThawLocalMachine,
D_CmdKind_IncUIFontScale,
D_CmdKind_DecUIFontScale,
D_CmdKind_IncCodeFontScale,
D_CmdKind_DecCodeFontScale,
D_CmdKind_OpenWindow,
D_CmdKind_CloseWindow,
D_CmdKind_ToggleFullscreen,
D_CmdKind_ConfirmAccept,
D_CmdKind_ConfirmCancel,
D_CmdKind_ResetToDefaultPanels,
D_CmdKind_ResetToCompactPanels,
D_CmdKind_NewPanelLeft,
D_CmdKind_NewPanelUp,
D_CmdKind_NewPanelRight,
D_CmdKind_NewPanelDown,
D_CmdKind_SplitPanel,
D_CmdKind_RotatePanelColumns,
D_CmdKind_NextPanel,
D_CmdKind_PrevPanel,
D_CmdKind_FocusPanel,
D_CmdKind_FocusPanelRight,
D_CmdKind_FocusPanelLeft,
D_CmdKind_FocusPanelUp,
D_CmdKind_FocusPanelDown,
D_CmdKind_Undo,
D_CmdKind_Redo,
D_CmdKind_GoBack,
D_CmdKind_GoForward,
D_CmdKind_ClosePanel,
D_CmdKind_NextTab,
D_CmdKind_PrevTab,
D_CmdKind_MoveTabRight,
D_CmdKind_MoveTabLeft,
D_CmdKind_OpenTab,
D_CmdKind_CloseTab,
D_CmdKind_MoveTab,
D_CmdKind_TabBarTop,
D_CmdKind_TabBarBottom,
D_CmdKind_SetCurrentPath,
D_CmdKind_Open,
D_CmdKind_Switch,
D_CmdKind_SwitchToPartnerFile,
D_CmdKind_GoToDisassembly,
D_CmdKind_GoToSource,
D_CmdKind_SetFileOverrideLinkSrc,
D_CmdKind_SetFileOverrideLinkDst,
D_CmdKind_SetFileReplacementPath,
D_CmdKind_SetAutoViewRuleType,
D_CmdKind_SetAutoViewRuleViewRule,
D_CmdKind_OpenUser,
D_CmdKind_OpenProject,
D_CmdKind_OpenRecentProject,
D_CmdKind_ApplyUserData,
D_CmdKind_ApplyProjectData,
D_CmdKind_WriteUserData,
D_CmdKind_WriteProjectData,
D_CmdKind_Edit,
D_CmdKind_Accept,
D_CmdKind_Cancel,
D_CmdKind_MoveLeft,
D_CmdKind_MoveRight,
D_CmdKind_MoveUp,
D_CmdKind_MoveDown,
D_CmdKind_MoveLeftSelect,
D_CmdKind_MoveRightSelect,
D_CmdKind_MoveUpSelect,
D_CmdKind_MoveDownSelect,
D_CmdKind_MoveLeftChunk,
D_CmdKind_MoveRightChunk,
D_CmdKind_MoveUpChunk,
D_CmdKind_MoveDownChunk,
D_CmdKind_MoveUpPage,
D_CmdKind_MoveDownPage,
D_CmdKind_MoveUpWhole,
D_CmdKind_MoveDownWhole,
D_CmdKind_MoveLeftChunkSelect,
D_CmdKind_MoveRightChunkSelect,
D_CmdKind_MoveUpChunkSelect,
D_CmdKind_MoveDownChunkSelect,
D_CmdKind_MoveUpPageSelect,
D_CmdKind_MoveDownPageSelect,
D_CmdKind_MoveUpWholeSelect,
D_CmdKind_MoveDownWholeSelect,
D_CmdKind_MoveUpReorder,
D_CmdKind_MoveDownReorder,
D_CmdKind_MoveHome,
D_CmdKind_MoveEnd,
D_CmdKind_MoveHomeSelect,
D_CmdKind_MoveEndSelect,
D_CmdKind_SelectAll,
D_CmdKind_DeleteSingle,
D_CmdKind_DeleteChunk,
D_CmdKind_BackspaceSingle,
D_CmdKind_BackspaceChunk,
D_CmdKind_Copy,
D_CmdKind_Cut,
D_CmdKind_Paste,
D_CmdKind_InsertText,
D_CmdKind_GoToLine,
D_CmdKind_GoToAddress,
D_CmdKind_CenterCursor,
D_CmdKind_ContainCursor,
D_CmdKind_FindTextForward,
D_CmdKind_FindTextBackward,
D_CmdKind_FindNext,
D_CmdKind_FindPrev,
D_CmdKind_FindThread,
D_CmdKind_FindSelectedThread,
D_CmdKind_GoToName,
D_CmdKind_GoToNameAtCursor,
D_CmdKind_ToggleWatchExpression,
D_CmdKind_ToggleWatchExpressionAtCursor,
D_CmdKind_ToggleWatchExpressionAtMouse,
D_CmdKind_SetColumns,
D_CmdKind_ToggleAddressVisibility,
D_CmdKind_ToggleCodeBytesVisibility,
D_CmdKind_EnableEntity,
D_CmdKind_DisableEntity,
D_CmdKind_FreezeEntity,
D_CmdKind_ThawEntity,
D_CmdKind_RemoveEntity,
D_CmdKind_NameEntity,
D_CmdKind_EditEntity,
D_CmdKind_DuplicateEntity,
D_CmdKind_RelocateEntity,
D_CmdKind_AddBreakpoint,
D_CmdKind_AddAddressBreakpoint,
D_CmdKind_AddFunctionBreakpoint,
D_CmdKind_ToggleBreakpoint,
D_CmdKind_RemoveBreakpoint,
D_CmdKind_EnableBreakpoint,
D_CmdKind_DisableBreakpoint,
D_CmdKind_AddWatchPin,
D_CmdKind_ToggleWatchPin,
D_CmdKind_ToggleBreakpointAtCursor,
D_CmdKind_ToggleWatchPinAtCursor,
D_CmdKind_AddTarget,
D_CmdKind_RemoveTarget,
D_CmdKind_EditTarget,
D_CmdKind_SelectTarget,
D_CmdKind_EnableTarget,
D_CmdKind_DisableTarget,
D_CmdKind_RetryEndedProcess,
D_CmdKind_Attach,
D_CmdKind_RegisterAsJITDebugger,
D_CmdKind_EntityRefFastPath,
D_CmdKind_SpawnEntityView,
D_CmdKind_FindCodeLocation,
D_CmdKind_Filter,
D_CmdKind_ApplyFilter,
D_CmdKind_ClearFilter,
D_CmdKind_GettingStarted,
D_CmdKind_Commands,
D_CmdKind_Target,
D_CmdKind_Targets,
D_CmdKind_FilePathMap,
D_CmdKind_AutoViewRules,
D_CmdKind_Breakpoints,
D_CmdKind_WatchPins,
D_CmdKind_Scheduler,
D_CmdKind_CallStack,
D_CmdKind_Modules,
D_CmdKind_Watch,
D_CmdKind_Locals,
D_CmdKind_Registers,
D_CmdKind_Globals,
D_CmdKind_ThreadLocals,
D_CmdKind_Types,
D_CmdKind_Procedures,
D_CmdKind_PendingFile,
D_CmdKind_Disassembly,
D_CmdKind_Output,
D_CmdKind_Memory,
D_CmdKind_ExceptionFilters,
D_CmdKind_Settings,
D_CmdKind_PickFile,
D_CmdKind_PickFolder,
D_CmdKind_PickFileOrFolder,
D_CmdKind_CompleteQuery,
D_CmdKind_CancelQuery,
D_CmdKind_ToggleDevMenu,
D_CmdKind_LogMarker,
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

typedef enum D_CmdParamSlot
{
D_CmdParamSlot_Null,
D_CmdParamSlot_Window,
D_CmdParamSlot_Panel,
D_CmdParamSlot_DestPanel,
D_CmdParamSlot_PrevView,
D_CmdParamSlot_View,
D_CmdParamSlot_Entity,
D_CmdParamSlot_EntityList,
D_CmdParamSlot_String,
D_CmdParamSlot_FilePath,
D_CmdParamSlot_TextPoint,
D_CmdParamSlot_CmdSpec,
D_CmdParamSlot_ViewSpec,
D_CmdParamSlot_ParamsTree,
D_CmdParamSlot_OSEvent,
D_CmdParamSlot_VirtualAddr,
D_CmdParamSlot_VirtualOff,
D_CmdParamSlot_Index,
D_CmdParamSlot_ID,
D_CmdParamSlot_PreferDisassembly,
D_CmdParamSlot_ForceConfirm,
D_CmdParamSlot_Dir2,
D_CmdParamSlot_UnwindIndex,
D_CmdParamSlot_InlineDepth,
D_CmdParamSlot_COUNT,
} D_CmdParamSlot;

typedef struct D_Regs D_Regs;
struct D_Regs
{
D_Handle module;
D_Handle process;
D_Handle thread;
D_Handle window;
D_Handle panel;
D_Handle view;
D_Handle prev_view;
D_Handle dst_panel;
D_Handle entity;
D_HandleList entity_list;
U64 unwind_count;
U64 inline_depth;
String8 file_path;
TxtPt cursor;
TxtPt mark;
U128 text_key;
TXT_LangKind lang_kind;
D_LineList lines;
DI_Key dbgi_key;
Rng1U64 vaddr_range;
Rng1U64 voff_range;
B32 force_confirm;
B32 prefer_disasm;
Dir2 dir2;
String8 string;
MD_Node * params_tree;
};

typedef struct D_CmdParams D_CmdParams;
struct D_CmdParams
{
D_Handle window;
D_Handle panel;
D_Handle dest_panel;
D_Handle prev_view;
D_Handle view;
D_Handle entity;
D_HandleList entity_list;
String8 string;
String8 file_path;
TxtPt text_point;
struct D_CmdSpec * cmd_spec;
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

D_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(array);
D_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(slice);
D_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(bswap);
D_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(cast);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(default);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(list);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(only);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(omit);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(color_rgba);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(text);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(disasm);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(memory);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(graph);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(bitmap);
D_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(geo3d);
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
extern Rng1U64 d_reg_slot_range_table[26];
extern Rng1U64 d_cmd_param_slot_range_table[23];
extern String8 d_entity_kind_display_string_table[31];
extern String8 d_entity_kind_name_lower_table[31];
extern String8 d_entity_kind_name_lower_plural_table[31];
extern String8 d_entity_kind_name_label_table[31];
extern D_EntityKindFlags d_entity_kind_flags_table[31];
extern String8 d_cfg_src_string_table[4];
extern D_CmdKind d_cfg_src_load_cmd_kind_table[4];
extern D_CmdKind d_cfg_src_write_cmd_kind_table[4];
extern D_CmdKind d_cfg_src_apply_cmd_kind_table[4];

C_LINKAGE_END

#endif // DBG_ENGINE_META_H
