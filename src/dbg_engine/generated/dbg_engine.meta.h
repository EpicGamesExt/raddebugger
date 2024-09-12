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
D_EntityKind_COUNT,
} D_EntityKind;

typedef enum D_RegSlot
{
D_RegSlot_Null,
D_RegSlot_Machine,
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
D_RegSlot_PID,
D_RegSlot_ForceConfirm,
D_RegSlot_PreferDisasm,
D_RegSlot_Dir2,
D_RegSlot_String,
D_RegSlot_ParamsTree,
D_RegSlot_COUNT,
} D_RegSlot;

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

typedef struct D_Regs D_Regs;
struct D_Regs
{
D_Handle machine;
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
U32 pid;
B32 force_confirm;
B32 prefer_disasm;
Dir2 dir2;
String8 string;
MD_Node * params_tree;
};

#define d_regs_lit_init_top \
.machine = d_regs()->machine,\
.module = d_regs()->module,\
.process = d_regs()->process,\
.thread = d_regs()->thread,\
.window = d_regs()->window,\
.panel = d_regs()->panel,\
.view = d_regs()->view,\
.prev_view = d_regs()->prev_view,\
.dst_panel = d_regs()->dst_panel,\
.entity = d_regs()->entity,\
.entity_list = d_regs()->entity_list,\
.unwind_count = d_regs()->unwind_count,\
.inline_depth = d_regs()->inline_depth,\
.file_path = d_regs()->file_path,\
.cursor = d_regs()->cursor,\
.mark = d_regs()->mark,\
.text_key = d_regs()->text_key,\
.lang_kind = d_regs()->lang_kind,\
.lines = d_regs()->lines,\
.dbgi_key = d_regs()->dbgi_key,\
.vaddr_range = d_regs()->vaddr_range,\
.voff_range = d_regs()->voff_range,\
.pid = d_regs()->pid,\
.force_confirm = d_regs()->force_confirm,\
.prefer_disasm = d_regs()->prefer_disasm,\
.dir2 = d_regs()->dir2,\
.string = d_regs()->string,\
.params_tree = d_regs()->params_tree,\

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
extern D_EntityKindFlags d_entity_kind_flags_table[30];
extern Rng1U64 d_reg_slot_range_table[29];

C_LINKAGE_END

#endif // DBG_ENGINE_META_H
