// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DF_CORE_H
#define DF_CORE_H

////////////////////////////////
//~ rjf: Handles

typedef struct DF_Handle DF_Handle;
struct DF_Handle
{
  U64 u64[2];
};

typedef struct DF_HandleNode DF_HandleNode;
struct DF_HandleNode
{
  DF_HandleNode *next;
  DF_HandleNode *prev;
  DF_Handle handle;
};

typedef struct DF_HandleList DF_HandleList;
struct DF_HandleList
{
  DF_HandleNode *first;
  DF_HandleNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Sparse Tree Expansion State Data Structure

typedef struct DF_ExpandKey DF_ExpandKey;
struct DF_ExpandKey
{
  U64 parent_hash;
  U64 child_num;
};

typedef struct DF_ExpandNode DF_ExpandNode;
struct DF_ExpandNode
{
  DF_ExpandNode *hash_next;
  DF_ExpandNode *hash_prev;
  DF_ExpandNode *first;
  DF_ExpandNode *last;
  DF_ExpandNode *next;
  DF_ExpandNode *prev;
  DF_ExpandNode *parent;
  DF_ExpandKey key;
  B32 expanded;
  F32 expanded_t;
};

typedef struct DF_ExpandSlot DF_ExpandSlot;
struct DF_ExpandSlot
{
  DF_ExpandNode *first;
  DF_ExpandNode *last;
};

typedef struct DF_ExpandTreeTable DF_ExpandTreeTable;
struct DF_ExpandTreeTable
{
  DF_ExpandSlot *slots;
  U64 slots_count;
  DF_ExpandNode *free_node;
};

////////////////////////////////
//~ rjf: Control Context Types

typedef struct DF_CtrlCtx DF_CtrlCtx;
struct DF_CtrlCtx
{
  DF_Handle thread;
  U64 unwind_count;
};

////////////////////////////////
//~ rjf: Entity Kind Flags

typedef U32 DF_EntityKindFlags;
enum
{
  DF_EntityKindFlag_LeafMutationUserConfig   = (1<<0),
  DF_EntityKindFlag_TreeMutationUserConfig   = (1<<1),
  DF_EntityKindFlag_LeafMutationProfileConfig= (1<<2),
  DF_EntityKindFlag_TreeMutationProfileConfig= (1<<3),
  DF_EntityKindFlag_LeafMutationSoftHalt     = (1<<4),
  DF_EntityKindFlag_TreeMutationSoftHalt     = (1<<5),
  DF_EntityKindFlag_LeafMutationDebugInfoMap = (1<<6),
  DF_EntityKindFlag_TreeMutationDebugInfoMap = (1<<7),
  DF_EntityKindFlag_NameIsCode               = (1<<8),
  DF_EntityKindFlag_UserDefinedLifetime      = (1<<9),
};

////////////////////////////////
//~ rjf: Entity Operation Flags

typedef U32 DF_EntityOpFlags;
enum
{
  DF_EntityOpFlag_Delete        = (1<<0),
  DF_EntityOpFlag_Freeze        = (1<<1),
  DF_EntityOpFlag_Edit          = (1<<2),
  DF_EntityOpFlag_Rename        = (1<<3),
  DF_EntityOpFlag_Enable        = (1<<4),
  DF_EntityOpFlag_Condition     = (1<<5),
  DF_EntityOpFlag_Duplicate     = (1<<6),
};

////////////////////////////////
//~ rjf: Entity Filesystem Lookup Flags

typedef U32 DF_EntityFromPathFlags;
enum
{
  DF_EntityFromPathFlag_AllowOverrides = (1<<0),
  DF_EntityFromPathFlag_OpenAsNeeded   = (1<<1),
  DF_EntityFromPathFlag_OpenMissing    = (1<<2),
  
  DF_EntityFromPathFlag_All = 0xffffffff,
};

////////////////////////////////
//~ rjf: Debug Engine Control Communication Types

typedef enum DF_RunKind
{
  DF_RunKind_Run,
  DF_RunKind_SingleStep,
  DF_RunKind_Step,
  DF_RunKind_COUNT
}
DF_RunKind;

////////////////////////////////
//~ rjf: Disassembly Types

typedef U32 DF_InstFlags;
enum
{
  DF_InstFlag_Call                        = (1<<0),
  DF_InstFlag_Branch                      = (1<<1),
  DF_InstFlag_UnconditionalJump           = (1<<2),
  DF_InstFlag_Return                      = (1<<3),
  DF_InstFlag_NonFlow                     = (1<<4),
  DF_InstFlag_Repeats                     = (1<<5),
  DF_InstFlag_ChangesStackPointer         = (1<<6),
  DF_InstFlag_ChangesStackPointerVariably = (1<<7),
};

typedef struct DF_Inst DF_Inst;
struct DF_Inst
{
  DF_InstFlags flags;
  U64 size;
  String8 string;
  U64 rel_voff;
  S64 sp_delta;
};

typedef struct DF_InstNode DF_InstNode;
struct DF_InstNode
{
  DF_InstNode *next;
  DF_Inst inst;
};

typedef struct DF_InstList DF_InstList;
struct DF_InstList
{
  DF_InstNode *first;
  DF_InstNode *last;
  U64 count;
};

typedef struct DF_InstArray DF_InstArray;
struct DF_InstArray
{
  DF_InstArray *v;
  U64 count;
};

typedef struct DF_InstMemVOffTuple DF_InstMemVOffTuple;
struct DF_InstMemVOffTuple
{
  DF_Inst inst;
  String8 mem;
  U64 voff;
};

typedef struct DF_InstMemVOffTupleArray DF_InstMemVOffTupleArray;
struct DF_InstMemVOffTupleArray
{
  DF_InstMemVOffTuple *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Control Flow Analysis Types

typedef U32 DF_CtrlFlowFlags;
enum
{
  DF_CtrlFlowFlag_StackPointerChangesVariably = (1<<0),
};

typedef struct DF_CtrlFlowPoint DF_CtrlFlowPoint;
struct DF_CtrlFlowPoint
{
  U64 vaddr;
  U64 jump_dest_vaddr;
  S64 expected_sp_delta;
  DF_InstFlags inst_flags;
};

typedef struct DF_CtrlFlowPointNode DF_CtrlFlowPointNode;
struct DF_CtrlFlowPointNode
{
  DF_CtrlFlowPointNode *next;
  DF_CtrlFlowPoint point;
};

typedef struct DF_CtrlFlowPointList DF_CtrlFlowPointList;
struct DF_CtrlFlowPointList
{
  DF_CtrlFlowPointNode *first;
  DF_CtrlFlowPointNode *last;
  U64 count;
};

typedef struct DF_CtrlFlowInfo DF_CtrlFlowInfo;
struct DF_CtrlFlowInfo
{
  DF_CtrlFlowFlags flags;
  DF_CtrlFlowPointList exit_points;
  U64 total_size;
  S64 cumulative_sp_delta;
};

////////////////////////////////
//~ rjf: Unwind Types

typedef struct DF_UnwindFrame DF_UnwindFrame;
struct DF_UnwindFrame
{
  DF_UnwindFrame *next;
  U64 rip;
  void *regs;
};

typedef struct DF_Unwind DF_Unwind;
struct DF_Unwind
{
  DF_UnwindFrame *first;
  DF_UnwindFrame *last;
  U64 count;
  B32 error;
};

////////////////////////////////
//~ rjf: Evaluation Types

typedef struct DF_Eval DF_Eval;
struct DF_Eval
{
  TG_Key type_key;
  EVAL_EvalMode mode;
  U64 offset;
  union
  {
    S64 imm_s64;
    U64 imm_u64;
    F32 imm_f32;
    F64 imm_f64;
    U64 imm_u128[2];
  };
  EVAL_ErrorList errors;
};

////////////////////////////////
//~ rjf: View Rule Hook Types

#define DF_CORE_VIEW_RULE_EVAL_RESOLUTION_FUNCTION_SIG(name) DF_Eval name(Arena *arena, DBGI_Scope *dbgi_scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_Eval eval, struct DF_CfgVal *val)
#define DF_CORE_VIEW_RULE_EVAL_RESOLUTION_FUNCTION_NAME(name) df_core_view_rule_eval_resolution__##name
#define DF_CORE_VIEW_RULE_EVAL_RESOLUTION_FUNCTION_DEF(name) internal DF_CORE_VIEW_RULE_EVAL_RESOLUTION_FUNCTION_SIG(DF_CORE_VIEW_RULE_EVAL_RESOLUTION_FUNCTION_NAME(name))
#define DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_SIG(name) void name(Arena *arena, DBGI_Scope *dbgi_scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, struct DF_EvalView *eval_view, DF_Eval eval, struct DF_CfgTable *cfg_table, DF_ExpandKey parent_key, DF_ExpandKey key, S32 depth, struct DF_CfgNode *cfg, struct DF_EvalVizBlockList *out)
#define DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_NAME(name) df_core_view_rule_viz_block_prod__##name
#define DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(name) internal DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_SIG(DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_NAME(name))
typedef DF_CORE_VIEW_RULE_EVAL_RESOLUTION_FUNCTION_SIG(DF_CoreViewRuleEvalResolutionHookFunctionType);
typedef DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_SIG(DF_CoreViewRuleVizBlockProdHookFunctionType);

////////////////////////////////
//~ rjf: Generated Code

#include "df/core/generated/df_core.meta.h"

////////////////////////////////
//~ rjf: Config Types

typedef U32 DF_CfgNodeFlags;
enum
{
  DF_CfgNodeFlag_Identifier    = (1<<0),
  DF_CfgNodeFlag_Numeric       = (1<<1),
  DF_CfgNodeFlag_StringLiteral = (1<<2),
};

typedef struct DF_CfgNode DF_CfgNode;
struct DF_CfgNode
{
  DF_CfgNode *first;
  DF_CfgNode *last;
  DF_CfgNode *parent;
  DF_CfgNode *next;
  DF_CfgNodeFlags flags;
  String8 string;
  DF_CfgSrc source;
};

typedef struct DF_CfgNodeRec DF_CfgNodeRec;
struct DF_CfgNodeRec
{
  DF_CfgNode *next;
  S32 push_count;
  S32 pop_count;
};

typedef struct DF_CfgVal DF_CfgVal;
struct DF_CfgVal
{
  DF_CfgVal *hash_next;
  DF_CfgVal *linear_next;
  DF_CfgNode *first;
  DF_CfgNode *last;
  U64 insertion_stamp;
  String8 string;
};

typedef struct DF_CfgSlot DF_CfgSlot;
struct DF_CfgSlot
{
  DF_CfgVal *first;
};

typedef struct DF_CfgTable DF_CfgTable;
struct DF_CfgTable
{
  U64 slot_count;
  DF_CfgSlot *slots;
  U64 insertion_stamp_counter;
  DF_CfgVal *first_val;
  DF_CfgVal *last_val;
};

////////////////////////////////
//~ rjf: View Rules

typedef U32 DF_CoreViewRuleSpecInfoFlags; // NOTE(rjf): see @view_rule_info
enum
{
  DF_CoreViewRuleSpecInfoFlag_Inherited      = (1<<0),
  DF_CoreViewRuleSpecInfoFlag_Expandable     = (1<<1),
  DF_CoreViewRuleSpecInfoFlag_EvalResolution = (1<<2),
  DF_CoreViewRuleSpecInfoFlag_VizBlockProd   = (1<<3),
};

typedef struct DF_CoreViewRuleSpecInfo DF_CoreViewRuleSpecInfo;
struct DF_CoreViewRuleSpecInfo
{
  String8 string;
  String8 display_string;
  String8 description;
  DF_CoreViewRuleSpecInfoFlags flags;
  DF_CoreViewRuleEvalResolutionHookFunctionType *eval_resolution;
  DF_CoreViewRuleVizBlockProdHookFunctionType *viz_block_prod;
};

typedef struct DF_CoreViewRuleSpecInfoArray DF_CoreViewRuleSpecInfoArray;
struct DF_CoreViewRuleSpecInfoArray
{
  DF_CoreViewRuleSpecInfo *v;
  U64 count;
};

typedef struct DF_CoreViewRuleSpec DF_CoreViewRuleSpec;
struct DF_CoreViewRuleSpec
{
  DF_CoreViewRuleSpec *hash_next;
  DF_CoreViewRuleSpecInfo info;
};

////////////////////////////////
//~ rjf: Entity Types

typedef U32 DF_EntitySubKind;

typedef U32 DF_EntityFlags;
enum
{
  //- rjf: allocationless, simple equipment
  DF_EntityFlag_HasTextPoint      = (1<<0),
  DF_EntityFlag_HasTextPointAlt   = (1<<1),
  DF_EntityFlag_HasEntityHandle   = (1<<2),
  DF_EntityFlag_HasB32            = (1<<3),
  DF_EntityFlag_HasU64            = (1<<4),
  DF_EntityFlag_HasRng1U64        = (1<<5),
  DF_EntityFlag_HasColor          = (1<<6),
  DF_EntityFlag_DiesWithTime      = (1<<7),
  DF_EntityFlag_DiesOnRunStop     = (1<<8),
  
  //- rjf: ctrl entity equipment
  DF_EntityFlag_HasCtrlMachineID  = (1<<9),
  DF_EntityFlag_HasCtrlHandle     = (1<<10),
  DF_EntityFlag_HasArch           = (1<<11),
  DF_EntityFlag_HasCtrlID         = (1<<12),
  DF_EntityFlag_HasStackBase      = (1<<13),
  DF_EntityFlag_HasTLSRoot        = (1<<14),
  DF_EntityFlag_HasVAddrRng       = (1<<15),
  DF_EntityFlag_HasVAddr          = (1<<16),
  
  //- rjf: file properties
  DF_EntityFlag_IsFolder          = (1<<17),
  DF_EntityFlag_IsMissing         = (1<<18),
  DF_EntityFlag_Output            = (1<<19), // NOTE(rjf): might be missing, but written by us
  
  //- rjf: deletion
  DF_EntityFlag_MarkedForDeletion = (1<<31),
};

typedef U64 DF_EntityID;

typedef struct DF_Entity DF_Entity;
struct DF_Entity
{
  // rjf: tree links
  DF_Entity *first;
  DF_Entity *last;
  DF_Entity *next;
  DF_Entity *prev;
  DF_Entity *parent;
  
  // rjf: metadata
  DF_EntityKind kind;
  DF_EntitySubKind subkind;
  DF_EntityFlags flags;
  DF_EntityID id;
  U64 generation;
  B32 deleted;
  F32 alive_t;
  
  // rjf: allocationless, simple equipment
  TxtPt text_point;
  TxtPt text_point_alt;
  DF_Handle entity_handle;
  B32 b32;
  U64 u64;
  Rng1U64 rng1u64;
  Vec4F32 color_hsva;
  F32 life_left;
  DF_CfgSrc cfg_src;
  
  // rjf: ctrl entity equipment
  CTRL_MachineID ctrl_machine_id;
  CTRL_Handle ctrl_handle;
  Architecture arch;
  U32 ctrl_id;
  U64 stack_base;
  U64 tls_root;
  Rng1U64 vaddr_rng;
  U64 vaddr;
  
  // rjf: name equipment
  String8 name;
  U64 name_generation;
  
  // rjf: timestamp
  U64 timestamp;
};

typedef struct DF_EntityNode DF_EntityNode;
struct DF_EntityNode
{
  DF_EntityNode *next;
  DF_Entity *entity;
};

typedef struct DF_EntityList DF_EntityList;
struct DF_EntityList
{
  DF_EntityNode *first;
  DF_EntityNode *last;
  U64 count;
};

typedef struct DF_EntityArray DF_EntityArray;
struct DF_EntityArray
{
  DF_Entity **v;
  U64 count;
};

typedef struct DF_EntityRec DF_EntityRec;
struct DF_EntityRec
{
  DF_Entity *next;
  S32 push_count;
  S32 pop_count;
};

////////////////////////////////
//~ rjf: Text Slices (output type from data which can be used to produce readable text)

//- rjf: text slice construction flags

typedef U32 DF_TextSliceFlags;
enum
{
  DF_TextSliceFlag_CodeBytes = (1<<0),
  DF_TextSliceFlag_Addresses = (1<<1),
  DF_TextSliceFlag_Tokens    = (1<<2),
  DF_TextSliceFlag_Src2Dasm  = (1<<3),
  DF_TextSliceFlag_Dasm2Src  = (1<<4),
  DF_TextSliceFlag_VirtualOff= (1<<5),
};

//- rjf: debug info for mapping src -> disasm

typedef struct DF_TextLineSrc2DasmInfo DF_TextLineSrc2DasmInfo;
struct DF_TextLineSrc2DasmInfo
{
  Rng1U64 voff_range;
  S64 remap_line;
  DF_Entity *binary;
};

typedef struct DF_TextLineSrc2DasmInfoNode DF_TextLineSrc2DasmInfoNode;
struct DF_TextLineSrc2DasmInfoNode
{
  DF_TextLineSrc2DasmInfoNode *next;
  DF_TextLineSrc2DasmInfo v;
};

typedef struct DF_TextLineSrc2DasmInfoList DF_TextLineSrc2DasmInfoList;
struct DF_TextLineSrc2DasmInfoList
{
  DF_TextLineSrc2DasmInfoNode *first;
  DF_TextLineSrc2DasmInfoNode *last;
  U64 count;
};

typedef struct DF_TextLineSrc2DasmInfoListArray DF_TextLineSrc2DasmInfoListArray;
struct DF_TextLineSrc2DasmInfoListArray
{
  DF_TextLineSrc2DasmInfoList *v;
  DF_EntityList binaries;
  U64 count;
};

//- rjf: debug info for mapping disasm -> src

typedef struct DF_TextLineDasm2SrcInfo DF_TextLineDasm2SrcInfo;
struct DF_TextLineDasm2SrcInfo
{
  DF_Entity *binary;
  DF_Entity *file;
  TxtPt pt;
  Rng1U64 voff_range;
};

typedef struct DF_TextLineDasm2SrcInfoNode DF_TextLineDasm2SrcInfoNode;
struct DF_TextLineDasm2SrcInfoNode
{
  DF_TextLineDasm2SrcInfoNode *next;
  DF_TextLineDasm2SrcInfo v;
};

typedef struct DF_TextLineDasm2SrcInfoList DF_TextLineDasm2SrcInfoList;
struct DF_TextLineDasm2SrcInfoList
{
  DF_TextLineDasm2SrcInfoNode *first;
  DF_TextLineDasm2SrcInfoNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Evaluation Visualization Types

//- rjf: expansion key -> view rule table

typedef struct DF_EvalViewRuleCacheNode DF_EvalViewRuleCacheNode;
struct DF_EvalViewRuleCacheNode
{
  DF_EvalViewRuleCacheNode *hash_next;
  DF_EvalViewRuleCacheNode *hash_prev;
  DF_ExpandKey key;
  U8 *buffer;
  U64 buffer_cap;
  U64 buffer_string_size;
};

typedef struct DF_EvalViewRuleCacheSlot DF_EvalViewRuleCacheSlot;
struct DF_EvalViewRuleCacheSlot
{
  DF_EvalViewRuleCacheNode *first;
  DF_EvalViewRuleCacheNode *last;
};

typedef struct DF_EvalViewRuleCacheTable DF_EvalViewRuleCacheTable;
struct DF_EvalViewRuleCacheTable
{
  U64 slot_count;
  DF_EvalViewRuleCacheSlot *slots;
};

//- rjf: 'eval view' entities for sparse-state expandable tree view cache for evaluation visualization

typedef struct DF_EvalViewKey DF_EvalViewKey;
struct DF_EvalViewKey
{
  U64 u64[2];
};

typedef struct DF_EvalView DF_EvalView;
struct DF_EvalView
{
  // rjf: links
  DF_EvalView *hash_next;
  DF_EvalView *hash_prev;
  
  // rjf: key
  DF_EvalViewKey key;
  
  // rjf: arena
  Arena *arena;
  
  // rjf: expansion state
  DF_ExpandTreeTable expand_tree_table;
  
  // rjf: key -> view rule cache
  DF_EvalViewRuleCacheTable view_rule_table;
};

typedef struct DF_EvalViewSlot DF_EvalViewSlot;
struct DF_EvalViewSlot
{
  DF_EvalView *first;
  DF_EvalView *last;
};

typedef struct DF_EvalViewCache DF_EvalViewCache;
struct DF_EvalViewCache
{
  DF_EvalViewSlot *slots;
  U64 slots_count;
};

//- rjf: eval view visualization building

typedef struct DF_EvalLinkBase DF_EvalLinkBase;
struct DF_EvalLinkBase
{
  EVAL_EvalMode mode;
  U64 offset;
};

typedef struct DF_EvalLinkBaseChunkNode DF_EvalLinkBaseChunkNode;
struct DF_EvalLinkBaseChunkNode
{
  DF_EvalLinkBaseChunkNode *next;
  DF_EvalLinkBase b[64];
  U64 count;
};

typedef struct DF_EvalLinkBaseChunkList DF_EvalLinkBaseChunkList;
struct DF_EvalLinkBaseChunkList
{
  DF_EvalLinkBaseChunkNode *first;
  DF_EvalLinkBaseChunkNode *last;
  U64 count;
};

typedef struct DF_EvalLinkBaseArray DF_EvalLinkBaseArray;
struct DF_EvalLinkBaseArray
{
  DF_EvalLinkBase *v;
  U64 count;
};

typedef enum DF_EvalVizBlockKind
{
  DF_EvalVizBlockKind_Null,              // empty
  DF_EvalVizBlockKind_Root,              // root of tree or subtree; possibly-expandable expression.
  DF_EvalVizBlockKind_Members,           // members of struct, class, union
  DF_EvalVizBlockKind_Elements,          // elements of array
  DF_EvalVizBlockKind_Links,             // flattened nodes in a linked list
  DF_EvalVizBlockKind_Canvas,            // escape hatch for arbitrary UI
  DF_EvalVizBlockKind_DebugInfoTable,    // block of filtered debug info table elements
  DF_EvalVizBlockKind_COUNT,
}
DF_EvalVizBlockKind;

typedef struct DF_EvalVizBlock DF_EvalVizBlock;
struct DF_EvalVizBlock
{
  // rjf: kind & keys
  DF_EvalVizBlockKind kind;
  DF_ExpandKey parent_key;
  DF_ExpandKey key;
  S32 depth;
  
  // rjf: evaluation info
  DF_Eval eval;
  String8 string;
  TG_Member *member;
  
  // rjf: info about ranges that this block spans
  Rng1U64 visual_idx_range;
  Rng1U64 semantic_idx_range;
  DBGI_FuzzySearchTarget dbgi_target;
  DBGI_FuzzySearchItemArray backing_search_items;
  
  // rjf: visualization config extensions
  DF_CfgTable cfg_table;
  TG_Key link_member_type_key;
  U64 link_member_off;
};

typedef struct DF_EvalVizBlockNode DF_EvalVizBlockNode;
struct DF_EvalVizBlockNode
{
  DF_EvalVizBlockNode *next;
  DF_EvalVizBlock v;
};

typedef struct DF_EvalVizBlockList DF_EvalVizBlockList;
struct DF_EvalVizBlockList
{
  DF_EvalVizBlockNode *first;
  DF_EvalVizBlockNode *last;
  U64 count;
  U64 total_visual_row_count;
  U64 total_semantic_row_count;
};

typedef struct DF_EvalVizBlockArray DF_EvalVizBlockArray;
struct DF_EvalVizBlockArray
{
  DF_EvalVizBlock *v;
  U64 count;
  U64 total_visual_row_count;
  U64 total_semantic_row_count;
};

typedef U32 DF_EvalVizStringFlags;
enum
{
  DF_EvalVizStringFlag_ReadOnlyDisplayRules = (1<<0),
};

// TODO(rjf): move viz-row stuff to gfx layer

typedef U32 DF_EvalVizRowFlags;
enum
{
  DF_EvalVizRowFlag_CanExpand    = (1<<0),
  DF_EvalVizRowFlag_CanEditValue = (1<<1),
  DF_EvalVizRowFlag_Canvas       = (1<<2),
};

typedef struct DF_EvalVizRow DF_EvalVizRow;
struct DF_EvalVizRow
{
  DF_EvalVizRow *next;
  DF_EvalVizRowFlags flags;
  
  // rjf: evaluation artifacts
  DF_Eval eval;
  
  // rjf: basic visualization contents
  String8 expr;
  String8 display_value;
  String8 edit_value;
  TG_KeyList inherited_type_key_chain;
  
  // rjf: variable-size & hook info
  U64 size_in_rows;
  U64 skipped_size_in_rows;
  U64 chopped_size_in_rows;
  struct DF_GfxViewRuleSpec *expand_ui_rule_spec;
  struct DF_CfgNode *expand_ui_rule_node;
  
  // rjf: value area override view rule spec
  struct DF_GfxViewRuleSpec *value_ui_rule_spec;
  struct DF_CfgNode *value_ui_rule_node;
  
  // rjf: errors
  EVAL_ErrorList errors;
  
  // rjf: tree depth & keys
  S32 depth;
  DF_ExpandKey parent_key;
  DF_ExpandKey key;
};

typedef struct DF_EvalVizWindowedRowList DF_EvalVizWindowedRowList;
struct DF_EvalVizWindowedRowList
{
  DF_EvalVizRow *first;
  DF_EvalVizRow *last;
  U64 count;
  U64 count_before_visual;
  U64 count_before_semantic;
};

////////////////////////////////
//~ rjf: Command Specification Types

typedef U32 DF_CmdQueryFlags;
enum
{
  DF_CmdQueryFlag_AllowFiles       = (1<<0),
  DF_CmdQueryFlag_AllowFolders     = (1<<1),
  DF_CmdQueryFlag_CodeInput        = (1<<2),
  DF_CmdQueryFlag_KeepOldInput     = (1<<3),
  DF_CmdQueryFlag_SelectOldInput   = (1<<4),
  DF_CmdQueryFlag_Required         = (1<<5),
};

typedef struct DF_CmdQuery DF_CmdQuery;
struct DF_CmdQuery
{
  DF_CmdParamSlot slot;
  DF_EntityKind entity_kind;
  DF_CmdQueryFlags flags;
};

typedef U32 DF_CmdSpecFlags;
enum
{
  DF_CmdSpecFlag_OmitFromLists      = (1<<0),
};

typedef struct DF_CmdSpecInfo DF_CmdSpecInfo;
struct DF_CmdSpecInfo
{
  String8 string;
  String8 description;
  String8 search_tags;
  String8 display_name;
  DF_CmdSpecFlags flags;
  DF_CmdQuery query;
  DF_IconKind canonical_icon_kind;
};

typedef struct DF_CmdSpec DF_CmdSpec;
struct DF_CmdSpec
{
  DF_CmdSpec *hash_next;
  DF_CmdSpecInfo info;
  U64 registrar_index;
  U64 ordering_index;
  U64 run_count;
};

typedef struct DF_CmdSpecNode DF_CmdSpecNode;
struct DF_CmdSpecNode
{
  DF_CmdSpecNode *next;
  DF_CmdSpec *spec;
};

typedef struct DF_CmdSpecList DF_CmdSpecList;
struct DF_CmdSpecList
{
  DF_CmdSpecNode *first;
  DF_CmdSpecNode *last;
  U64 count;
};

typedef struct DF_CmdSpecArray DF_CmdSpecArray;
struct DF_CmdSpecArray
{
  DF_CmdSpec **v;
  U64 count;
};

typedef struct DF_CmdSpecInfoArray DF_CmdSpecInfoArray;
struct DF_CmdSpecInfoArray
{
  DF_CmdSpecInfo *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Command Types

typedef struct DF_Cmd DF_Cmd;
struct DF_Cmd
{
  DF_CmdParams params;
  DF_CmdSpec *spec;
};

typedef struct DF_CmdNode DF_CmdNode;
struct DF_CmdNode
{
  DF_CmdNode *next;
  DF_CmdNode *prev;
  DF_Cmd cmd;
};

typedef struct DF_CmdList DF_CmdList;
struct DF_CmdList
{
  DF_CmdNode *first;
  DF_CmdNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Main State Caches

//- rjf: per-entity-kind state cache

typedef struct DF_EntityListCache DF_EntityListCache;
struct DF_EntityListCache
{
  Arena *arena;
  U64 alloc_gen;
  DF_EntityList list;
};

//- rjf: per-run unwind cache

typedef struct DF_RunUnwindCacheNode DF_RunUnwindCacheNode;
struct DF_RunUnwindCacheNode
{
  DF_RunUnwindCacheNode *hash_next;
  DF_Handle thread;
  DF_Unwind unwind;
};

typedef struct DF_RunUnwindCacheSlot DF_RunUnwindCacheSlot;
struct DF_RunUnwindCacheSlot
{
  DF_RunUnwindCacheNode *first;
  DF_RunUnwindCacheNode *last;
};

typedef struct DF_RunUnwindCache DF_RunUnwindCache;
struct DF_RunUnwindCache
{
  Arena *arena;
  U64 table_size;
  DF_RunUnwindCacheSlot *table;
};

//- rjf: per-run locals cache

typedef struct DF_RunLocalsCacheNode DF_RunLocalsCacheNode;
struct DF_RunLocalsCacheNode
{
  DF_RunLocalsCacheNode *hash_next;
  DF_Handle binary;
  U64 voff;
  EVAL_String2NumMap *locals_map;
};

typedef struct DF_RunLocalsCacheSlot DF_RunLocalsCacheSlot;
struct DF_RunLocalsCacheSlot
{
  DF_RunLocalsCacheNode *first;
  DF_RunLocalsCacheNode *last;
};

typedef struct DF_RunLocalsCache DF_RunLocalsCache;
struct DF_RunLocalsCache
{
  Arena *arena;
  U64 table_size;
  DF_RunLocalsCacheSlot *table;
};

////////////////////////////////
//~ rjf: File Change Detector Shared Data Structure Types

typedef struct DF_FileScanNode DF_FileScanNode;
struct DF_FileScanNode
{
  DF_FileScanNode *next;
  String8 path;
  U64 stamp;
};

typedef struct DF_FileScanSlot DF_FileScanSlot;
struct DF_FileScanSlot
{
  DF_FileScanNode *first;
  DF_FileScanNode *last;
};

////////////////////////////////
//~ rjf: State Delta History Types

typedef struct DF_StateDelta DF_StateDelta;
struct DF_StateDelta
{
  U64 vaddr;
  String8 data;
};

typedef struct DF_StateDeltaNode DF_StateDeltaNode;
struct DF_StateDeltaNode
{
  DF_StateDeltaNode *next;
  DF_StateDelta v;
};

typedef struct DF_StateDeltaBatch DF_StateDeltaBatch;
struct DF_StateDeltaBatch
{
  DF_StateDeltaBatch *next;
  DF_StateDeltaNode *first;
  DF_StateDeltaNode *last;
  U64 gen;
  U64 gen_vaddr;
};

typedef struct DF_StateDeltaHistory DF_StateDeltaHistory;
struct DF_StateDeltaHistory
{
  Arena *arena;
  Arena *side_arenas[Side_COUNT]; // min -> undo; max -> redo
  DF_StateDeltaBatch *side_tops[Side_COUNT];
};

////////////////////////////////
//~ rjf: Main State Types

//- rjf: architecture info table types

typedef struct DF_ArchInfoNode DF_ArchInfoNode;
struct DF_ArchInfoNode
{
  DF_ArchInfoNode *hash_next;
  String8 key;
  String8 val;
};

typedef struct DF_ArchInfoSlot DF_ArchInfoSlot;
struct DF_ArchInfoSlot
{
  DF_ArchInfoNode *first;
  DF_ArchInfoNode *last;
};

//- rjf: name allocator types

typedef struct DF_NameChunkNode DF_NameChunkNode;
struct DF_NameChunkNode
{
  DF_NameChunkNode *next;
  U64 size;
};

//- rjf: core bundle state type

typedef struct DF_State DF_State;
struct DF_State
{
  // rjf: top-level state
  Arena *arena;
  U64 frame_index;
  F64 time_in_seconds;
  F32 dt;
  F32 seconds_til_autosave;
  
  // rjf: top-level command batch
  Arena *root_cmd_arena;
  DF_CmdList root_cmds;
  
  // rjf: history cache
  DF_StateDeltaHistory *hist;
  
  // rjf: name allocator
  DF_NameChunkNode *free_name_chunks[8];
  
  // rjf: entity state
  Arena *entities_arena;
  DF_Entity *entities_base;
  U64 entities_count;
  U64 entities_id_gen;
  DF_Entity *entities_root;
  DF_Entity *entities_free[2]; // [0] -> normal lifetime, not user defined; [1] -> user defined lifetime (& thus undoable)
  U64 entities_free_count;
  U64 entities_active_count;
  B32 entities_mut_soft_halt;
  B32 entities_mut_dbg_info_map;
  
  // rjf: entity query caches
  U64 kind_alloc_gens[DF_EntityKind_COUNT];
  DF_EntityListCache kind_caches[DF_EntityKind_COUNT];
  DF_EntityListCache dbg_info_cache;
  DF_EntityListCache bin_file_cache;
  
  // rjf: per-run caches
  U64 unwind_cache_reggen_idx;
  U64 unwind_cache_memgen_idx;
  DF_RunUnwindCache unwind_cache;
  U64 locals_cache_reggen_idx;
  DF_RunLocalsCache locals_cache;
  U64 member_cache_reggen_idx;
  DF_RunLocalsCache member_cache;
  
  // rjf: eval view cache
  DF_EvalViewCache eval_view_cache;
  
  // rjf: command specification table
  U64 total_registrar_count;
  U64 cmd_spec_table_size;
  DF_CmdSpec **cmd_spec_table;
  
  // rjf: view rule specification table
  U64 view_rule_spec_table_size;
  DF_CoreViewRuleSpec **view_rule_spec_table;
  
  // rjf: freeze state
  DF_HandleList frozen_threads;
  DF_HandleNode *free_handle_node;
  
  // rjf: main control context
  DF_CtrlCtx ctrl_ctx;
  
  // rjf: control thread user -> ctrl driving state
  Arena *ctrl_last_run_arena;
  DF_RunKind ctrl_last_run_kind;
  U64 ctrl_last_run_frame_idx;
  DF_Handle ctrl_last_run_thread;
  CTRL_TrapList ctrl_last_run_traps;
  U64 ctrl_run_gen;
  B32 ctrl_is_running;
  B32 ctrl_soft_halt_issued;
  Arena *ctrl_msg_arena;
  CTRL_MsgList ctrl_msgs;
  U64 ctrl_exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64];
  B32 ctrl_solo_stepping_mode;
  
  // rjf: control thread ctrl -> user reading state
  Arena *ctrl_stop_arena;
  CTRL_Event ctrl_last_stop_event;
  
  // rjf: config reading state
  Arena *cfg_path_arenas[DF_CfgSrc_COUNT];
  String8 cfg_paths[DF_CfgSrc_COUNT];
  U64 cfg_cached_timestamp[DF_CfgSrc_COUNT];
  Arena *cfg_arena;
  DF_CfgTable cfg_table;
  
  // rjf: config writing state
  B32 cfg_write_issued[DF_CfgSrc_COUNT];
  Arena *cfg_write_arenas[DF_CfgSrc_COUNT];
  String8List cfg_write_data[DF_CfgSrc_COUNT];
  
  // rjf: current path
  Arena *current_path_arena;
  String8 current_path;
  
  // rjf: architecture info tables
  U64 arch_info_x64_table_size;
  DF_ArchInfoSlot *arch_info_x64_table;
};

////////////////////////////////
//~ rjf: Globals

read_only global DF_CmdSpec df_g_nil_cmd_spec = {0};
read_only global DF_CoreViewRuleSpec df_g_nil_core_view_rule_spec = {0};
read_only global DF_CfgNode df_g_nil_cfg_node = {&df_g_nil_cfg_node, &df_g_nil_cfg_node, &df_g_nil_cfg_node, &df_g_nil_cfg_node};
read_only global DF_CfgVal df_g_nil_cfg_val = {&df_g_nil_cfg_val, &df_g_nil_cfg_val, &df_g_nil_cfg_node, &df_g_nil_cfg_node};
read_only global DF_Entity df_g_nil_entity =
{
  // rjf: tree links
  &df_g_nil_entity,
  &df_g_nil_entity,
  &df_g_nil_entity,
  &df_g_nil_entity,
  &df_g_nil_entity,
  
  // rjf: metadata
  DF_EntityKind_Nil,
  0,
  0,
  0,
  0,
  0,
  0,
  
  // rjf: allocationless, simple equipment
  {0},
  {0},
  {0},
  0,
  0,
  {0},
  {0},
  0,
  DF_CfgSrc_User,
  
  // rjf: ctrl entity equipment
  0,
  {0},
  Architecture_Null,
  0,
  0,
  0,
  {0},
  0,
  
  // rjf: name equipment
  0,
  zero_struct,
  0,
  
  // rjf: timestamp
  0,
};
read_only global DF_EvalView df_g_nil_eval_view = {&df_g_nil_eval_view, &df_g_nil_eval_view};

global DF_State *df_state = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 df_hash_from_seed_string(U64 seed, String8 string);
internal U64 df_hash_from_string(String8 string);
internal U64 df_hash_from_seed_string__case_insensitive(U64 seed, String8 string);
internal U64 df_hash_from_string__case_insensitive(String8 string);

////////////////////////////////
//~ rjf: Handle Type Pure Functions

internal DF_Handle df_handle_zero(void);
internal B32 df_handle_match(DF_Handle a, DF_Handle b);
internal void df_handle_list_push_node(DF_HandleList *list, DF_HandleNode *node);
internal void df_handle_list_push(Arena *arena, DF_HandleList *list, DF_Handle handle);
internal void df_handle_list_remove(DF_HandleList *list, DF_HandleNode *node);
internal DF_HandleNode *df_handle_list_find(DF_HandleList *list, DF_Handle handle);
internal DF_HandleList df_push_handle_list_copy(Arena *arena, DF_HandleList list);

////////////////////////////////
//~ rjf: State History Data Structure

internal DF_StateDeltaHistory *df_state_delta_history_alloc(void);
internal void df_state_delta_history_release(DF_StateDeltaHistory *hist);
internal void df_state_delta_history_push_batch(DF_StateDeltaHistory *hist, U64 *optional_gen_ptr);
internal void df_state_delta_history_push_delta(DF_StateDeltaHistory *hist, void *ptr, U64 size);
#define df_state_delta_history_push_struct_delta(hist, ptr) df_state_delta_history_push_delta((hist), (ptr), sizeof(*(ptr)))
internal void df_state_delta_history_wind(DF_StateDeltaHistory *hist, Side side);

////////////////////////////////
//~ rjf: Sparse Tree Expansion State Data Structure

//- rjf: keys
internal DF_ExpandKey df_expand_key_make(U64 parent_hash, U64 child_num);
internal DF_ExpandKey df_expand_key_zero(void);
internal B32 df_expand_key_match(DF_ExpandKey a, DF_ExpandKey b);

//- rjf: table
internal void df_expand_tree_table_init(Arena *arena, DF_ExpandTreeTable *table, U64 slot_count);
internal void df_expand_tree_table_animate(DF_ExpandTreeTable *table, F32 dt);
internal DF_ExpandNode *df_expand_node_from_key(DF_ExpandTreeTable *table, DF_ExpandKey key);
internal B32 df_expand_key_is_set(DF_ExpandTreeTable *table, DF_ExpandKey key);
internal void df_expand_set_expansion(Arena *arena, DF_ExpandTreeTable *table, DF_ExpandKey parent_key, DF_ExpandKey key, B32 expanded);

////////////////////////////////
//~ rjf: Config Type Pure Functions

internal DF_CfgNode *df_cfg_tree_copy(Arena *arena, DF_CfgNode *src_root);
internal DF_CfgNodeRec df_cfg_node_rec__depth_first_pre(DF_CfgNode *node, DF_CfgNode *root);
internal void df_cfg_table_push_unparsed_string(Arena *arena, DF_CfgTable *table, String8 string, DF_CfgSrc source);
internal DF_CfgTable df_cfg_table_from_inheritance(Arena *arena, DF_CfgTable *src);
internal DF_CfgTable df_cfg_table_copy(Arena *arena, DF_CfgTable *src);
internal DF_CfgVal *df_cfg_val_from_string(DF_CfgTable *table, String8 string);
internal DF_CfgNode *df_cfg_node_child_from_string(DF_CfgNode *node, String8 string, StringMatchFlags flags);
internal DF_CfgNode *df_first_cfg_node_child_from_flags(DF_CfgNode *node, DF_CfgNodeFlags flags);
internal String8 df_string_from_cfg_node_children(Arena *arena, DF_CfgNode *node);
internal Vec4F32 df_hsva_from_cfg_node(DF_CfgNode *node);
internal String8 df_string_from_cfg_node_key(DF_CfgNode *node, String8 key, StringMatchFlags flags);

////////////////////////////////
//~ rjf: Disassembly Pure Functions

internal DF_Inst df_single_inst_from_machine_code__x64(Arena *arena, U64 start_voff, String8 string);
internal DF_Inst df_single_inst_from_machine_code(Arena *arena, Architecture arch, U64 start_voff, String8 string);

////////////////////////////////
//~ rjf: Control Flow Analysis Pure Functions

internal DF_CtrlFlowInfo df_ctrl_flow_info_from_vaddr_code__x64(Arena *arena, DF_InstFlags exit_points_mask, U64 vaddr, String8 code);
internal DF_CtrlFlowInfo df_ctrl_flow_info_from_arch_vaddr_code(Arena *arena, DF_InstFlags exit_points_mask, Architecture arch, U64 vaddr, String8 code);

////////////////////////////////
//~ rjf: Command Type Pure Functions

//- rjf: specs
internal B32 df_cmd_spec_is_nil(DF_CmdSpec *spec);
internal void df_cmd_spec_list_push(Arena *arena, DF_CmdSpecList *list, DF_CmdSpec *spec);
internal DF_CmdSpecArray df_cmd_spec_array_from_list(Arena *arena, DF_CmdSpecList list);
internal int df_qsort_compare_cmd_spec__run_counter(DF_CmdSpec **a, DF_CmdSpec **b);
internal void df_cmd_spec_array_sort_by_run_counter__in_place(DF_CmdSpecArray array);
internal DF_Handle df_handle_from_cmd_spec(DF_CmdSpec *spec);
internal DF_CmdSpec *df_cmd_spec_from_handle(DF_Handle handle);

//- rjf: string -> command parsing
internal String8 df_cmd_name_part_from_string(String8 string);
internal String8 df_cmd_arg_part_from_string(String8 string);

//- rjf: command parameter bundles
internal DF_CmdParams df_cmd_params_zero(void);
internal void df_cmd_params_mark_slot(DF_CmdParams *params, DF_CmdParamSlot slot);
internal B32 df_cmd_params_has_slot(DF_CmdParams *params, DF_CmdParamSlot slot);
internal String8 df_cmd_params_apply_spec_query(Arena *arena, DF_CtrlCtx *ctrl_ctx, DF_CmdParams *params, DF_CmdSpec *spec, String8 query);

//- rjf: command lists
internal void df_cmd_list_push(Arena *arena, DF_CmdList *cmds, DF_CmdParams *params, DF_CmdSpec *spec);

//- rjf: string -> core layer command kind
internal DF_CoreCmdKind df_core_cmd_kind_from_string(String8 string);

////////////////////////////////
//~ rjf: Entity Type Pure Functions

//- rjf: nil
internal B32 df_entity_is_nil(DF_Entity *entity);
#define df_require_entity_nonnil(entity, if_nil_stmts) do{if(df_entity_is_nil(entity)){if_nil_stmts;}}while(0)

//- rjf: handle <-> entity conversions
internal U64 df_index_from_entity(DF_Entity *entity);
internal DF_Handle df_handle_from_entity(DF_Entity *entity);
internal DF_Entity *df_entity_from_handle(DF_Handle handle);
internal DF_EntityList df_entity_list_from_handle_list(Arena *arena, DF_HandleList handles);
internal DF_HandleList df_handle_list_from_entity_list(Arena *arena, DF_EntityList entities);

//- rjf: entity recursion iterators
internal DF_EntityRec df_entity_rec_df(DF_Entity *entity, DF_Entity *subtree_root, U64 sib_off, U64 child_off);
#define df_entity_rec_df_pre(entity, subtree_root)  df_entity_rec_df((entity), (subtree_root), OffsetOf(DF_Entity, next), OffsetOf(DF_Entity, first))
#define df_entity_rec_df_post(entity, subtree_root) df_entity_rec_df((entity), (subtree_root), OffsetOf(DF_Entity, prev), OffsetOf(DF_Entity, last))

//- rjf: ancestor/child introspection
internal DF_Entity *df_entity_child_from_kind(DF_Entity *entity, DF_EntityKind kind);
internal DF_Entity *df_entity_ancestor_from_kind(DF_Entity *entity, DF_EntityKind kind);
internal DF_EntityList df_push_entity_child_list_with_kind(Arena *arena, DF_Entity *entity, DF_EntityKind kind);
internal DF_Entity *df_entity_child_from_name_and_kind(DF_Entity *parent, String8 string, DF_EntityKind kind);

//- rjf: entity list building
internal void df_entity_list_push(Arena *arena, DF_EntityList *list, DF_Entity *entity);
internal DF_EntityArray df_entity_array_from_list(Arena *arena, DF_EntityList *list);
#define df_first_entity_from_list(list) ((list)->first != 0 ? (list)->first->entity : &df_g_nil_entity)

//- rjf: entity -> text info
internal TXTI_Handle df_txti_handle_from_entity(DF_Entity *entity);

//- rjf: entity -> disasm info
internal DASM_Handle df_dasm_handle_from_process_vaddr(DF_Entity *process, U64 vaddr);

//- rjf: full path building, from file/folder entities
internal String8 df_full_path_from_entity(Arena *arena, DF_Entity *entity);

//- rjf: display string entities, for referencing entities in ui
internal String8 df_display_string_from_entity(Arena *arena, DF_Entity *entity);

//- rjf: entity -> color operations
internal Vec4F32 df_hsva_from_entity(DF_Entity *entity);
internal Vec4F32 df_rgba_from_entity(DF_Entity *entity);

////////////////////////////////
//~ rjf: Name Allocation

internal U64 df_name_bucket_idx_from_string_size(U64 size);
internal String8 df_name_alloc(DF_StateDeltaHistory *hist, String8 string);
internal void df_name_release(DF_StateDeltaHistory *hist, String8 string);

////////////////////////////////
//~ rjf: Entity Stateful Functions

//- rjf: entity mutation notification codepath
internal void df_entity_notify_mutation(DF_Entity *entity);

//- rjf: entity allocation + tree forming
internal DF_Entity *df_entity_alloc(DF_StateDeltaHistory *hist, DF_Entity *parent, DF_EntityKind kind);
internal void df_entity_mark_for_deletion(DF_Entity *entity);
internal void df_entity_release(DF_StateDeltaHistory *hist, DF_Entity *entity);
internal void df_entity_change_parent(DF_StateDeltaHistory *hist, DF_Entity *entity, DF_Entity *old_parent, DF_Entity *new_parent);

//- rjf: entity simple equipment
internal void df_entity_equip_txt_pt(DF_Entity *entity, TxtPt point);
internal void df_entity_equip_txt_pt_alt(DF_Entity *entity, TxtPt point);
internal void df_entity_equip_entity_handle(DF_Entity *entity, DF_Handle handle);
internal void df_entity_equip_b32(DF_Entity *entity, B32 b32);
internal void df_entity_equip_u64(DF_Entity *entity, U64 u64);
internal void df_entity_equip_rng1u64(DF_Entity *entity, Rng1U64 range);
internal void df_entity_equip_color_rgba(DF_Entity *entity, Vec4F32 rgba);
internal void df_entity_equip_color_hsva(DF_Entity *entity, Vec4F32 hsva);
internal void df_entity_equip_death_timer(DF_Entity *entity, F32 seconds_til_death);
internal void df_entity_equip_cfg_src(DF_Entity *entity, DF_CfgSrc cfg_src);

//- rjf: control layer correllation equipment
internal void df_entity_equip_ctrl_machine_id(DF_Entity *entity, CTRL_MachineID machine_id);
internal void df_entity_equip_ctrl_handle(DF_Entity *entity, CTRL_Handle handle);
internal void df_entity_equip_arch(DF_Entity *entity, Architecture arch);
internal void df_entity_equip_ctrl_id(DF_Entity *entity, U32 id);
internal void df_entity_equip_stack_base(DF_Entity *entity, U64 stack_base);
internal void df_entity_equip_tls_root(DF_Entity *entity, U64 tls_root);
internal void df_entity_equip_vaddr_rng(DF_Entity *entity, Rng1U64 range);
internal void df_entity_equip_vaddr(DF_Entity *entity, U64 vaddr);

//- rjf: name equipment
internal void df_entity_equip_name(DF_StateDeltaHistory *hist, DF_Entity *entity, String8 name);
internal void df_entity_equip_namef(DF_StateDeltaHistory *hist, DF_Entity *entity, char *fmt, ...);

//- rjf: opening folders/files & maintaining the entity model of the filesystem
internal DF_Entity *df_entity_from_path(String8 path, DF_EntityFromPathFlags flags);
internal DF_EntityList df_possible_overrides_from_entity(Arena *arena, DF_Entity *entity);

//- rjf: top-level state queries
internal DF_Entity *df_entity_root(void);
internal DF_EntityList df_push_entity_list_with_kind(Arena *arena, DF_EntityKind kind);
internal DF_Entity *df_entity_from_id(DF_EntityID id);
internal DF_Entity *df_machine_entity_from_machine_id(CTRL_MachineID machine_id);
internal DF_Entity *df_entity_from_ctrl_handle(CTRL_MachineID machine_id, CTRL_Handle handle);
internal DF_Entity *df_entity_from_ctrl_id(CTRL_MachineID machine_id, U32 id);
internal DF_Entity *df_entity_from_name_and_kind(String8 string, DF_EntityKind kind);
internal DF_Entity *df_entity_from_u64_and_kind(U64 u64, DF_EntityKind kind);

//- rjf: entity freezing state
internal void df_set_thread_freeze_state(DF_Entity *thread, B32 frozen);
internal B32 df_entity_is_frozen(DF_Entity *entity);

////////////////////////////////
//~ rjf: Command Stateful Functions

internal void df_register_cmd_specs(DF_CmdSpecInfoArray specs);
internal DF_CmdSpec *df_cmd_spec_from_string(String8 string);
internal DF_CmdSpec *df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind core_cmd_kind);
internal void df_cmd_spec_counter_inc(DF_CmdSpec *spec);
internal DF_CmdSpecList df_push_cmd_spec_list(Arena *arena);

////////////////////////////////
//~ rjf: View Rule Spec Stateful Functions

internal void df_register_core_view_rule_specs(DF_CoreViewRuleSpecInfoArray specs);
internal DF_CoreViewRuleSpec *df_core_view_rule_spec_from_string(String8 string);

////////////////////////////////
//~ rjf: Debug Info Mapping

internal String8 df_debug_info_path_from_module(Arena *arena, DF_Entity *module);

////////////////////////////////
//~ rjf: Stepping "Trap Net" Builders

internal CTRL_TrapList df_trap_net_from_thread__step_over_inst(Arena *arena, DF_Entity *thread);
internal CTRL_TrapList df_trap_net_from_thread__step_over_line(Arena *arena, DF_Entity *thread);
internal CTRL_TrapList df_trap_net_from_thread__step_into_line(Arena *arena, DF_Entity *thread);

////////////////////////////////
//~ rjf: Modules & Debug Info Mappings

//- rjf: module <=> binary file
internal DF_Entity *df_binary_file_from_module(DF_Entity *module);
internal DF_EntityList df_modules_from_binary_file(Arena *arena, DF_Entity *binary_info);

//- rjf: voff <=> vaddr
internal U64 df_base_vaddr_from_module(DF_Entity *module);
internal U64 df_voff_from_vaddr(DF_Entity *module, U64 vaddr);
internal U64 df_vaddr_from_voff(DF_Entity *module, U64 voff);
internal Rng1U64 df_voff_range_from_vaddr_range(DF_Entity *module, Rng1U64 vaddr_rng);
internal Rng1U64 df_vaddr_range_from_voff_range(DF_Entity *module, Rng1U64 voff_rng);

////////////////////////////////
//~ rjf: Debug Info Lookups

//- rjf: binary file -> dbgi parse
internal DBGI_Parse *df_dbgi_parse_from_binary_file(DBGI_Scope *scope, DF_Entity *binary);

//- rjf: voff|vaddr -> symbol lookups
internal String8 df_symbol_name_from_binary_voff(Arena *arena, DF_Entity *binary, U64 voff);
internal String8 df_symbol_name_from_process_vaddr(Arena *arena, DF_Entity *process, U64 vaddr);

//- rjf: src -> voff lookups
internal DF_TextLineSrc2DasmInfoListArray df_text_line_src2dasm_info_list_array_from_src_line_range(Arena *arena, DF_Entity *file, Rng1S64 line_num_range);

//- rjf: voff -> src lookups
internal DF_TextLineDasm2SrcInfo df_text_line_dasm2src_info_from_binary_voff(DF_Entity *binary, U64 voff);
internal DF_TextLineDasm2SrcInfoList df_text_line_dasm2src_info_from_voff(Arena *arena, U64 voff);

//- rjf: symbol -> voff lookups
internal U64 df_voff_from_binary_symbol_name(DF_Entity *binary, String8 symbol_name);
internal U64 df_type_num_from_binary_name(DF_Entity *binary, String8 name);

////////////////////////////////
//~ rjf: Process/Thread Info Lookups

//- rjf: thread info extraction helpers
internal DF_Entity *df_module_from_process_vaddr(DF_Entity *process, U64 vaddr);
internal DF_Entity *df_module_from_thread(DF_Entity *thread);
internal U64 df_tls_base_vaddr_from_thread(DF_Entity *thread);
internal Architecture df_architecture_from_entity(DF_Entity *entity);
internal DF_Unwind df_push_unwind_from_thread(Arena *arena, DF_Entity *thread);
internal U64 df_rip_from_thread(DF_Entity *thread);
internal U64 df_rip_from_thread_unwind(DF_Entity *thread, U64 unwind_count);
internal EVAL_String2NumMap *df_push_locals_map_from_binary_voff(Arena *arena, DBGI_Scope *scope, DF_Entity *binary, U64 voff);
internal EVAL_String2NumMap *df_push_member_map_from_binary_voff(Arena *arena, DBGI_Scope *scope, DF_Entity *binary, U64 voff);
internal B32 df_set_thread_rip(DF_Entity *thread, U64 vaddr);
internal DF_Entity *df_module_from_thread_candidates(DF_Entity *thread, DF_EntityList *candidates);

////////////////////////////////
//~ rjf: Entity -> Log Entities

internal DF_Entity *df_log_from_entity(DF_Entity *entity);

////////////////////////////////
//~ rjf: Target Controls

//- rjf: control message dispatching
internal void df_push_ctrl_msg(CTRL_Msg *msg);

//- rjf: control thread running
internal void df_ctrl_run(DF_RunKind run, DF_Entity *run_thread, CTRL_TrapList *run_traps);

//- rjf: stopped info from the control thread
internal CTRL_Event df_ctrl_last_stop_event(void);

////////////////////////////////
//~ rjf: Evaluation

internal B32 df_eval_memory_read(void *u, void *out, U64 addr, U64 size);
internal EVAL_ParseCtx df_eval_parse_ctx_from_process_vaddr(DBGI_Scope *scope, DF_Entity *process, U64 vaddr);
internal EVAL_ParseCtx df_eval_parse_ctx_from_src_loc(DBGI_Scope *scope, DF_Entity *file, TxtPt pt);
internal DF_Eval df_eval_from_string(Arena *arena, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, String8 string);
internal DF_Eval df_value_mode_eval_from_eval(TG_Graph *graph, RADDBG_Parsed *rdbg, DF_CtrlCtx *ctrl_ctx, DF_Eval eval);
internal DF_Eval df_dynamically_typed_eval_from_eval(TG_Graph *graph, RADDBG_Parsed *rdbg, DF_CtrlCtx *ctrl_ctx, DF_Eval eval);
internal DF_Eval df_eval_from_eval_cfg_table(Arena *arena, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_Eval eval, DF_CfgTable *cfg);

////////////////////////////////
//~ rjf: Evaluation Views

//- rjf: keys
internal DF_EvalViewKey df_eval_view_key_make(U64 v0, U64 v1);
internal DF_EvalViewKey df_eval_view_key_from_string(String8 string);
internal DF_EvalViewKey df_eval_view_key_from_stringf(char *fmt, ...);
internal B32 df_eval_view_key_match(DF_EvalViewKey a, DF_EvalViewKey b);

//- rjf: cache lookup
internal DF_EvalView *df_eval_view_from_key(DF_EvalViewKey key);

//- rjf: key -> view rules
internal void df_eval_view_set_key_rule(DF_EvalView *eval_view, DF_ExpandKey key, String8 view_rule_string);
internal String8 df_eval_view_rule_from_key(DF_EvalView *eval_view, DF_ExpandKey key);

////////////////////////////////
//~ rjf: Evaluation Visualization

//- rjf: evaluation value string builder helpers
internal String8 df_string_from_ascii_value(Arena *arena, U8 val);
internal String8 df_string_from_simple_typed_eval(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, DF_EvalVizStringFlags flags, U32 radix, DF_Eval eval);

//- rjf: writing values back to child processes
internal B32 df_commit_eval_value(TG_Graph *graph, RADDBG_Parsed *rdbg, DF_CtrlCtx *ctrl_ctx, DF_Eval dst_eval, DF_Eval src_eval);

//- rjf: type helpers
internal TG_MemberArray df_filtered_data_members_from_members_cfg_table(Arena *arena, TG_MemberArray members, DF_CfgTable *cfg);
internal DF_EvalLinkBaseChunkList df_eval_link_base_chunk_list_from_eval(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key link_member_type_key, U64 link_member_off, DF_CtrlCtx *ctrl_ctx, DF_Eval eval, U64 cap);
internal DF_EvalLinkBase df_eval_link_base_from_chunk_list_index(DF_EvalLinkBaseChunkList *list, U64 idx);
internal DF_EvalLinkBaseArray df_eval_link_base_array_from_chunk_list(Arena *arena, DF_EvalLinkBaseChunkList *chunks);

//- rjf: viz block collection building
internal DF_EvalVizBlock *df_eval_viz_block_begin(Arena *arena, DF_EvalVizBlockKind kind, DF_ExpandKey parent_key, DF_ExpandKey key, S32 depth);
internal DF_EvalVizBlock *df_eval_viz_block_split_and_continue(Arena *arena, DF_EvalVizBlockList *list, DF_EvalVizBlock *split_block, U64 split_idx);
internal void df_eval_viz_block_end(DF_EvalVizBlockList *list, DF_EvalVizBlock *block);
internal void df_append_viz_blocks_for_parent__rec(Arena *arena, DBGI_Scope *scope, DF_EvalView *view, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_ExpandKey parent_key, DF_ExpandKey key, String8 string, DF_Eval eval, TG_Member *opt_member, DF_CfgTable *cfg_table, S32 depth, DF_EvalVizBlockList *list_out);
internal DF_EvalVizBlockList df_eval_viz_block_list_from_eval_view_expr_num(Arena *arena, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_EvalView *eval_view, String8 expr, U64 num);
internal void df_eval_viz_block_list_concat__in_place(DF_EvalVizBlockList *dst, DF_EvalVizBlockList *to_push);

//- rjf: viz block list <-> table coordinates
internal S64 df_row_num_from_viz_block_list_key(DF_EvalVizBlockList *blocks, DF_ExpandKey key);
internal DF_ExpandKey df_key_from_viz_block_list_row_num(DF_EvalVizBlockList *blocks, S64 row_num);

////////////////////////////////
//~ rjf: Main State Accessors/Mutators

//- rjf: frame metadata
internal F32 df_dt(void);
internal U64 df_frame_index(void);
internal F64 df_time_in_seconds(void);

//- rjf: undo/redo history
internal DF_StateDeltaHistory *df_state_delta_history(void);

//- rjf: control state
internal DF_RunKind df_ctrl_last_run_kind(void);
internal U64 df_ctrl_last_run_frame_idx(void);
internal U64 df_ctrl_run_gen(void);
internal B32 df_ctrl_targets_running(void);

//- rjf: control context
internal DF_CtrlCtx df_ctrl_ctx(void);
internal void df_ctrl_ctx_apply_overrides(DF_CtrlCtx *ctx, DF_CtrlCtx *overrides);

//- rjf: config paths
internal String8 df_cfg_path_from_src(DF_CfgSrc src);

//- rjf: config state
internal DF_CfgTable *df_cfg_table(void);

//- rjf: config serialization
internal String8 df_cfg_escaped_from_raw_string(Arena *arena, String8 string);
internal String8 df_cfg_raw_from_escaped_string(Arena *arena, String8 string);
internal String8List df_cfg_strings_from_core(Arena *arena, String8 root_path, DF_CfgSrc source);
internal void df_cfg_push_write_string(DF_CfgSrc src, String8 string);

//- rjf: current path
internal String8 df_current_path(void);

//- rjf: architecture info table lookups
internal String8 df_info_summary_from_string__x64(String8 string);
internal String8 df_info_summary_from_string(Architecture arch, String8 string);

//- rjf: entity kind cache
internal DF_EntityList df_query_cached_entity_list_with_kind(DF_EntityKind kind);
internal DF_EntityList df_push_active_binary_list(Arena *arena);
internal DF_EntityList df_push_active_target_list(Arena *arena);

//- rjf: per-run caches
internal DF_Unwind df_query_cached_unwind_from_thread(DF_Entity *thread);
internal U64 df_query_cached_rip_from_thread(DF_Entity *thread);
internal U64 df_query_cached_rip_from_thread_unwind(DF_Entity *thread, U64 unwind_count);
internal EVAL_String2NumMap *df_query_cached_locals_map_from_binary_voff(DF_Entity *binary, U64 voff);
internal EVAL_String2NumMap *df_query_cached_member_map_from_binary_voff(DF_Entity *binary, U64 voff);

//- rjf: top-level command dispatch
internal void df_push_cmd__root(DF_CmdParams *params, DF_CmdSpec *spec);

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal void df_core_init(String8 user_path, String8 profile_path, DF_StateDeltaHistory *hist);
internal DF_CmdList df_core_gather_root_cmds(Arena *arena);
internal void df_core_begin_frame(Arena *arena, DF_CmdList *cmds, F32 dt);
internal void df_core_end_frame(void);

#endif // DF_CORE_H
