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
//~ rjf: Entity Kind Flags

typedef U32 DF_EntityKindFlags;
enum
{
  //- rjf: allowed operations
  DF_EntityKindFlag_CanDelete                = (1<<8),
  DF_EntityKindFlag_CanFreeze                = (1<<9),
  DF_EntityKindFlag_CanEdit                  = (1<<10),
  DF_EntityKindFlag_CanRename                = (1<<11),
  DF_EntityKindFlag_CanEnable                = (1<<12),
  DF_EntityKindFlag_CanCondition             = (1<<13),
  DF_EntityKindFlag_CanDuplicate             = (1<<14),
  
  //- rjf: mutation -> cascading effects
  DF_EntityKindFlag_LeafMutUserConfig        = (1<<0),
  DF_EntityKindFlag_TreeMutUserConfig        = (1<<1),
  DF_EntityKindFlag_LeafMutProjectConfig     = (1<<2),
  DF_EntityKindFlag_TreeMutProjectConfig     = (1<<3),
  DF_EntityKindFlag_LeafMutSoftHalt          = (1<<4),
  DF_EntityKindFlag_TreeMutSoftHalt          = (1<<5),
  DF_EntityKindFlag_LeafMutDebugInfoMap      = (1<<6),
  DF_EntityKindFlag_TreeMutDebugInfoMap      = (1<<7),
  
  //- rjf: name categorization
  DF_EntityKindFlag_NameIsCode               = (1<<15),
  DF_EntityKindFlag_NameIsPath               = (1<<16),
  
  //- rjf: lifetime categorization
  DF_EntityKindFlag_UserDefinedLifetime      = (1<<17),
  
  //- rjf: serialization
  DF_EntityKindFlag_IsSerializedToConfig     = (1<<18),
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
  DASM_InstFlags inst_flags;
};

typedef struct DF_CtrlFlowPointNode DF_CtrlFlowPointNode;
struct DF_CtrlFlowPointNode
{
  DF_CtrlFlowPointNode *next;
  DF_CtrlFlowPoint v;
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
};

////////////////////////////////
//~ rjf: View Rule Hook Types

typedef struct DF_CfgTree DF_CfgTree;
typedef struct DF_CfgVal DF_CfgVal;
typedef struct DF_CfgTable DF_CfgTable;
typedef struct DF_EvalView DF_EvalView;
typedef struct DF_EvalVizBlockList DF_EvalVizBlockList;
#define DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_SIG(name) E_Expr *name(Arena *arena, E_Expr *expr, MD_Node *params)
#define DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_NAME(name) df_core_view_rule_expr_resolution__##name
#define DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(name) internal DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_SIG(DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_NAME(name))
#define DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_SIG(name) void name(Arena *arena,                                    \
DF_EvalView *eval_view,                          \
DF_ExpandKey parent_key,                         \
DF_ExpandKey key,                                \
DF_ExpandNode *expand_node,                      \
String8 string,                                  \
E_Expr *expr,                                    \
DF_CfgTable *cfg_table,                          \
S32 depth,                                       \
MD_Node *params,                                 \
struct DF_EvalVizBlockList *out)
#define DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_NAME(name) df_core_view_rule_viz_block_prod__##name
#define DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(name) internal DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_SIG(DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_NAME(name))
typedef DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_SIG(DF_CoreViewRuleExprResolutionHookFunctionType);
typedef DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_SIG(DF_CoreViewRuleVizBlockProdHookFunctionType);

////////////////////////////////
//~ rjf: Generated Code

#include "df/core/generated/df_core.meta.h"

////////////////////////////////
//~ rjf: Config Types

typedef struct DF_CfgTree DF_CfgTree;
struct DF_CfgTree
{
  DF_CfgTree *next;
  DF_CfgSrc source;
  MD_Node *root;
};

typedef struct DF_CfgVal DF_CfgVal;
struct DF_CfgVal
{
  DF_CfgVal *hash_next;
  DF_CfgVal *linear_next;
  DF_CfgTree *first;
  DF_CfgTree *last;
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
  DF_CoreViewRuleSpecInfoFlag_ExprResolution = (1<<2),
  DF_CoreViewRuleSpecInfoFlag_VizBlockProd   = (1<<3),
};

typedef struct DF_CoreViewRuleSpecInfo DF_CoreViewRuleSpecInfo;
struct DF_CoreViewRuleSpecInfo
{
  String8 string;
  String8 display_string;
  String8 schema;
  String8 description;
  DF_CoreViewRuleSpecInfoFlags flags;
  DF_CoreViewRuleExprResolutionHookFunctionType *expr_resolution;
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

typedef U32 DF_EntityFlags;
enum
{
  //- rjf: allocationless, simple equipment
  DF_EntityFlag_HasTextPoint      = (1<<0),
  DF_EntityFlag_HasEntityHandle   = (1<<2),
  DF_EntityFlag_HasU64            = (1<<4),
  DF_EntityFlag_HasColor          = (1<<6),
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
  DF_EntityFlags flags;
  DF_EntityID id;
  U64 gen;
  U64 alloc_time_us;
  F32 alive_t;
  
  // rjf: basic equipment
  TxtPt text_point;
  DF_Handle entity_handle;
  B32 disabled;
  U64 u64;
  Vec4F32 color_hsva;
  F32 life_left;
  DF_CfgSrc cfg_src;
  U64 timestamp;
  
  // rjf: ctrl equipment
  CTRL_MachineID ctrl_machine_id;
  DMN_Handle ctrl_handle;
  Architecture arch;
  U32 ctrl_id;
  U64 stack_base;
  Rng1U64 vaddr_rng;
  U64 vaddr;
  
  // rjf: name equipment
  String8 name;
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
//~ rjf: Entity Evaluation Types

typedef struct DF_EntityEval DF_EntityEval;
struct DF_EntityEval
{
  B64 enabled;
  U64 hit_count;
  U64 label_off;
  U64 location_off;
  U64 condition_off;
};

////////////////////////////////
//~ rjf: Entity Fuzzy Listing Types

typedef struct DF_EntityFuzzyItem DF_EntityFuzzyItem;
struct DF_EntityFuzzyItem
{
  DF_Entity *entity;
  FuzzyMatchRangeList matches;
};

typedef struct DF_EntityFuzzyItemArray DF_EntityFuzzyItemArray;
struct DF_EntityFuzzyItemArray
{
  DF_EntityFuzzyItem *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Rich (Including Inline) Unwind Types

typedef struct DF_UnwindInlineFrame DF_UnwindInlineFrame;
struct DF_UnwindInlineFrame
{
  DF_UnwindInlineFrame *next;
  DF_UnwindInlineFrame *prev;
  RDI_InlineSite *inline_site;
};

typedef struct DF_UnwindFrame DF_UnwindFrame;
struct DF_UnwindFrame
{
  DF_UnwindInlineFrame *first_inline_frame;
  DF_UnwindInlineFrame *last_inline_frame;
  U64 inline_frame_count;
  void *regs;
  RDI_Parsed *rdi;
  RDI_Procedure *procedure;
};

typedef struct DF_UnwindFrameArray DF_UnwindFrameArray;
struct DF_UnwindFrameArray
{
  DF_UnwindFrame *v;
  U64 concrete_frame_count;
  U64 inline_frame_count;
  U64 total_frame_count;
};

typedef struct DF_Unwind DF_Unwind;
struct DF_Unwind
{
  DF_UnwindFrameArray frames;
};

////////////////////////////////
//~ rjf: Line Info Types

typedef struct DF_Line DF_Line;
struct DF_Line
{
  String8 file_path;
  TxtPt pt;
  Rng1U64 voff_range;
  DI_Key dbgi_key;
};

typedef struct DF_LineNode DF_LineNode;
struct DF_LineNode
{
  DF_LineNode *next;
  DF_Line v;
};

typedef struct DF_LineList DF_LineList;
struct DF_LineList
{
  DF_LineNode *first;
  DF_LineNode *last;
  U64 count;
};

typedef struct DF_LineListArray DF_LineListArray;
struct DF_LineListArray
{
  DF_LineList *v;
  U64 count;
  DI_KeyList dbgi_keys;
};

////////////////////////////////
//~ rjf: Source <-> Disasm Types

//- rjf: debug info for mapping src -> disasm

typedef struct DF_TextLineSrc2DasmInfo DF_TextLineSrc2DasmInfo;
struct DF_TextLineSrc2DasmInfo
{
  Rng1U64 voff_range;
  S64 remap_line;
  DI_Key dbgi_key;
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
  DI_KeyList dbgi_keys;
  U64 count;
};

//- rjf: debug info for mapping disasm -> src

typedef struct DF_TextLineDasm2SrcInfo DF_TextLineDasm2SrcInfo;
struct DF_TextLineDasm2SrcInfo
{
  DI_Key dbgi_key;
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
//~ rjf: Interaction Context Register Types

typedef struct DF_InteractRegs DF_InteractRegs;
struct DF_InteractRegs
{
  DF_Handle module;
  DF_Handle process;
  DF_Handle thread;
  U64 unwind_count;
  U64 inline_depth;
  DF_Handle window;
  DF_Handle panel;
  DF_Handle view;
  String8 file_path;
  TxtPt cursor;
  TxtPt mark;
  U128 text_key;
  TXT_LangKind lang_kind;
  Rng1U64 vaddr_range;
  Rng1U64 voff_range;
  DF_LineList lines;
  DI_Key dbgi_key;
};

typedef struct DF_InteractRegsNode DF_InteractRegsNode;
struct DF_InteractRegsNode
{
  DF_InteractRegsNode *next;
  DF_InteractRegs v;
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
  DF_EvalVizBlockKind_EnumMembers,       // members of enum
  DF_EvalVizBlockKind_Elements,          // elements of array
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
  String8 string;
  E_Expr *expr;
  
  // rjf: info about ranges that this block spans
  Rng1U64 visual_idx_range;
  Rng1U64 semantic_idx_range;
  
  // rjf: visualization config extensions
  DF_CfgTable *cfg_table;
  DF_EvalLinkBaseChunkList *link_bases;
  E_MemberArray members;
  E_EnumValArray enum_vals;
  RDI_SectionKind fzy_target;
  FZY_ItemArray fzy_backing_items;
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

typedef struct DF_EvalVizRow DF_EvalVizRow;
struct DF_EvalVizRow
{
  DF_EvalVizRow *next;
  
  // rjf: block hierarchy info
  S32 depth;
  DF_ExpandKey parent_key;
  DF_ExpandKey key;
  
  // rjf: row size/scroll info
  U64 size_in_rows;
  U64 skipped_size_in_rows;
  U64 chopped_size_in_rows;
  
  // rjf: evaluation expression
  String8 string;
  E_Member *member;
  E_Expr *expr;
  
  // rjf: view rule attachments
  DF_CfgTable *cfg_table;
  struct DF_GfxViewRuleSpec *expand_ui_rule_spec;
  MD_Node *expand_ui_rule_params;
  struct DF_GfxViewRuleSpec *value_ui_rule_spec;
  MD_Node *value_ui_rule_params;
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
  DF_CmdSpecFlag_ListInUI      = (1<<0),
  DF_CmdSpecFlag_ListInIPCDocs = (1<<1),
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

//- rjf: auto view rules hash table cache

typedef struct DF_AutoViewRuleNode DF_AutoViewRuleNode;
struct DF_AutoViewRuleNode
{
  DF_AutoViewRuleNode *next;
  String8 type;
  String8 view_rule;
};

typedef struct DF_AutoViewRuleSlot DF_AutoViewRuleSlot;
struct DF_AutoViewRuleSlot
{
  DF_AutoViewRuleNode *first;
  DF_AutoViewRuleNode *last;
};

typedef struct DF_AutoViewRuleMapCache DF_AutoViewRuleMapCache;
struct DF_AutoViewRuleMapCache
{
  Arena *arena;
  U64 slots_count;
  DF_AutoViewRuleSlot *slots;
};

//- rjf: per-thread unwind cache

typedef struct DF_UnwindCacheNode DF_UnwindCacheNode;
struct DF_UnwindCacheNode
{
  DF_UnwindCacheNode *next;
  DF_UnwindCacheNode *prev;
  U64 reggen;
  U64 memgen;
  Arena *arena;
  DF_Handle thread;
  CTRL_Unwind unwind;
};

typedef struct DF_UnwindCacheSlot DF_UnwindCacheSlot;
struct DF_UnwindCacheSlot
{
  DF_UnwindCacheNode *first;
  DF_UnwindCacheNode *last;
};

typedef struct DF_UnwindCache DF_UnwindCache;
struct DF_UnwindCache
{
  U64 slots_count;
  DF_UnwindCacheSlot *slots;
  DF_UnwindCacheNode *free_node;
};

//- rjf: per-run tls-base-vaddr cache

typedef struct DF_RunTLSBaseCacheNode DF_RunTLSBaseCacheNode;
struct DF_RunTLSBaseCacheNode
{
  DF_RunTLSBaseCacheNode *hash_next;
  DF_Handle process;
  U64 root_vaddr;
  U64 rip_vaddr;
  U64 tls_base_vaddr;
};

typedef struct DF_RunTLSBaseCacheSlot DF_RunTLSBaseCacheSlot;
struct DF_RunTLSBaseCacheSlot
{
  DF_RunTLSBaseCacheNode *first;
  DF_RunTLSBaseCacheNode *last;
};

typedef struct DF_RunTLSBaseCache DF_RunTLSBaseCache;
struct DF_RunTLSBaseCache
{
  Arena *arena;
  U64 slots_count;
  DF_RunTLSBaseCacheSlot *slots;
};

//- rjf: per-run locals cache

typedef struct DF_RunLocalsCacheNode DF_RunLocalsCacheNode;
struct DF_RunLocalsCacheNode
{
  DF_RunLocalsCacheNode *hash_next;
  DI_Key dbgi_key;
  U64 voff;
  E_String2NumMap *locals_map;
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

typedef struct DF_StateDeltaParams DF_StateDeltaParams;
struct DF_StateDeltaParams
{
  void *ptr;
  U64 size;
  DF_Entity *guard_entity;
};

typedef struct DF_StateDelta DF_StateDelta;
struct DF_StateDelta
{
  DF_Handle guard_entity;
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
};

typedef struct DF_StateDeltaHistory DF_StateDeltaHistory;
struct DF_StateDeltaHistory
{
  Arena *arena;
  Arena *side_arenas[Side_COUNT]; // min -> undo; max -> redo
  DF_StateDeltaBatch *side_tops[Side_COUNT];
  B32 batch_is_active;
};

////////////////////////////////
//~ rjf: Main State Types

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
  U64 frame_eval_memread_endt_us;
  F64 time_in_seconds;
  F32 dt;
  F32 seconds_til_autosave;
  
  // rjf: frame info
  Arena *frame_arenas[2];
  DI_Scope *frame_di_scope;
  
  // rjf: interaction registers
  DF_InteractRegsNode base_interact_regs;
  DF_InteractRegsNode *top_interact_regs;
  
  // rjf: top-level command batch
  Arena *root_cmd_arena;
  DF_CmdList root_cmds;
  
  // rjf: output log key
  U128 output_log_key;
  
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
  DF_AutoViewRuleMapCache auto_view_rule_cache;
  
  // rjf: per-run caches
  DF_UnwindCache unwind_cache;
  U64 tls_base_cache_reggen_idx;
  U64 tls_base_cache_memgen_idx;
  DF_RunTLSBaseCache tls_base_caches[2];
  U64 tls_base_cache_gen;
  U64 locals_cache_reggen_idx;
  DF_RunLocalsCache locals_caches[2];
  U64 locals_cache_gen;
  U64 member_cache_reggen_idx;
  DF_RunLocalsCache member_caches[2];
  U64 member_cache_gen;
  
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
  
  // rjf: control thread user -> ctrl driving state
  Arena *ctrl_last_run_arena;
  DF_RunKind ctrl_last_run_kind;
  U64 ctrl_last_run_frame_idx;
  DF_Handle ctrl_last_run_thread;
  CTRL_RunFlags ctrl_last_run_flags;
  CTRL_TrapList ctrl_last_run_traps;
  U64 ctrl_run_gen;
  B32 ctrl_is_running;
  B32 ctrl_soft_halt_issued;
  Arena *ctrl_msg_arena;
  CTRL_MsgList ctrl_msgs;
  U64 ctrl_exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64];
  
  // rjf: control thread ctrl -> user reading state
  CTRL_EntityStore *ctrl_entity_store;
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
};

////////////////////////////////
//~ rjf: Globals

read_only global DF_CmdSpec df_g_nil_cmd_spec = {0};
read_only global DF_CoreViewRuleSpec df_g_nil_core_view_rule_spec = {0};
read_only global DF_CfgTree df_g_nil_cfg_tree = {&df_g_nil_cfg_tree, DF_CfgSrc_User, &md_nil_node};
read_only global DF_CfgVal df_g_nil_cfg_val = {&df_g_nil_cfg_val, &df_g_nil_cfg_val, &df_g_nil_cfg_tree, &df_g_nil_cfg_tree};
read_only global DF_CfgTable df_g_nil_cfg_table = {0, 0, 0, &df_g_nil_cfg_val, &df_g_nil_cfg_val};
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
  
  // rjf: basic equipment
  {0},
  {0},
  0,
  0,
  {0},
  0,
  DF_CfgSrc_User,
  0,
  
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
  {0},
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
internal void df_state_delta_history_batch_begin(DF_StateDeltaHistory *hist);
internal void df_state_delta_history_batch_end(DF_StateDeltaHistory *hist);
#define DF_StateDeltaHistoryBatch(hist) DeferLoop(df_state_delta_history_batch_begin(hist), df_state_delta_history_batch_end(hist))
internal void df_state_delta_history_push_delta_(DF_StateDeltaHistory *hist, DF_StateDeltaParams *params);
#define df_state_delta_history_push_delta(hist, ...) df_state_delta_history_push_delta_((hist), &(DF_StateDeltaParams){.size = 1, __VA_ARGS__})
#define df_state_delta_history_push_struct_delta(hist, sptr, ...) df_state_delta_history_push_delta((hist), .ptr = (sptr), .size = sizeof(*(sptr)), __VA_ARGS__)
internal void df_state_delta_history_wind(DF_StateDeltaHistory *hist, Side side);

////////////////////////////////
//~ rjf: Sparse Tree Expansion State Data Structure

//- rjf: keys
internal DF_ExpandKey df_expand_key_make(U64 parent_hash, U64 child_num);
internal DF_ExpandKey df_expand_key_zero(void);
internal B32 df_expand_key_match(DF_ExpandKey a, DF_ExpandKey b);

//- rjf: table
internal void df_expand_tree_table_init(Arena *arena, DF_ExpandTreeTable *table, U64 slot_count);
internal DF_ExpandNode *df_expand_node_from_key(DF_ExpandTreeTable *table, DF_ExpandKey key);
internal B32 df_expand_key_is_set(DF_ExpandTreeTable *table, DF_ExpandKey key);
internal void df_expand_set_expansion(Arena *arena, DF_ExpandTreeTable *table, DF_ExpandKey parent_key, DF_ExpandKey key, B32 expanded);

////////////////////////////////
//~ rjf: Config Type Pure Functions

internal DF_CfgTree *df_cfg_tree_copy(Arena *arena, DF_CfgTree *src);
internal void df_cfg_table_push_unparsed_string(Arena *arena, DF_CfgTable *table, String8 string, DF_CfgSrc source);
internal DF_CfgTable df_cfg_table_from_inheritance(Arena *arena, DF_CfgTable *src);
internal DF_CfgTable df_cfg_table_copy(Arena *arena, DF_CfgTable *src);
internal DF_CfgVal *df_cfg_val_from_string(DF_CfgTable *table, String8 string);

////////////////////////////////
//~ rjf: Debug Info Extraction Type Pure Functions

internal DF_LineList df_line_list_copy(Arena *arena, DF_LineList *list);

////////////////////////////////
//~ rjf: Control Flow Analysis Pure Functions

internal DF_CtrlFlowInfo df_ctrl_flow_info_from_arch_vaddr_code(Arena *arena, DASM_InstFlags exit_points_mask, Architecture arch, U64 vaddr, String8 code);

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
internal String8 df_cmd_params_apply_spec_query(Arena *arena, DF_CmdParams *params, DF_CmdSpec *spec, String8 query);

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

//- rjf: entity fuzzy list building
internal DF_EntityFuzzyItemArray df_entity_fuzzy_item_array_from_entity_list_needle(Arena *arena, DF_EntityList *list, String8 needle);
internal DF_EntityFuzzyItemArray df_entity_fuzzy_item_array_from_entity_array_needle(Arena *arena, DF_EntityArray *array, String8 needle);

//- rjf: full path building, from file/folder entities
internal String8 df_full_path_from_entity(Arena *arena, DF_Entity *entity);

//- rjf: display string entities, for referencing entities in ui
internal String8 df_display_string_from_entity(Arena *arena, DF_Entity *entity);

//- rjf: extra search tag strings for fuzzy filtering entities
internal String8 df_search_tags_from_entity(Arena *arena, DF_Entity *entity);

//- rjf: entity -> color operations
internal Vec4F32 df_hsva_from_entity(DF_Entity *entity);
internal Vec4F32 df_rgba_from_entity(DF_Entity *entity);

//- rjf: entity -> expansion tree keys
internal DF_ExpandKey df_expand_key_from_entity(DF_Entity *entity);
internal DF_ExpandKey df_parent_expand_key_from_entity(DF_Entity *entity);

//- rjf: entity -> evaluation
internal DF_EntityEval *df_eval_from_entity(Arena *arena, DF_Entity *entity);

////////////////////////////////
//~ rjf: Name Allocation

internal U64 df_name_bucket_idx_from_string_size(U64 size);
internal String8 df_name_alloc(String8 string);
internal void df_name_release(String8 string);

////////////////////////////////
//~ rjf: Entity Stateful Functions

//- rjf: entity mutation notification codepath
internal void df_entity_notify_mutation(DF_Entity *entity);

//- rjf: entity allocation + tree forming
internal DF_Entity *df_entity_alloc(DF_Entity *parent, DF_EntityKind kind);
internal void df_entity_mark_for_deletion(DF_Entity *entity);
internal void df_entity_release(DF_Entity *entity);
internal void df_entity_change_parent(DF_Entity *entity, DF_Entity *old_parent, DF_Entity *new_parent, DF_Entity *prev_child);

//- rjf: entity simple equipment
internal void df_entity_equip_txt_pt(DF_Entity *entity, TxtPt point);
internal void df_entity_equip_entity_handle(DF_Entity *entity, DF_Handle handle);
internal void df_entity_equip_disabled(DF_Entity *entity, B32 b32);
internal void df_entity_equip_u64(DF_Entity *entity, U64 u64);
internal void df_entity_equip_color_rgba(DF_Entity *entity, Vec4F32 rgba);
internal void df_entity_equip_color_hsva(DF_Entity *entity, Vec4F32 hsva);
internal void df_entity_equip_cfg_src(DF_Entity *entity, DF_CfgSrc cfg_src);
internal void df_entity_equip_timestamp(DF_Entity *entity, U64 timestamp);

//- rjf: control layer correllation equipment
internal void df_entity_equip_ctrl_machine_id(DF_Entity *entity, CTRL_MachineID machine_id);
internal void df_entity_equip_ctrl_handle(DF_Entity *entity, DMN_Handle handle);
internal void df_entity_equip_arch(DF_Entity *entity, Architecture arch);
internal void df_entity_equip_ctrl_id(DF_Entity *entity, U32 id);
internal void df_entity_equip_stack_base(DF_Entity *entity, U64 stack_base);
internal void df_entity_equip_vaddr_rng(DF_Entity *entity, Rng1U64 range);
internal void df_entity_equip_vaddr(DF_Entity *entity, U64 vaddr);

//- rjf: name equipment
internal void df_entity_equip_name(DF_Entity *entity, String8 name);
internal void df_entity_equip_namef(DF_Entity *entity, char *fmt, ...);

//- rjf: opening folders/files & maintaining the entity model of the filesystem
internal DF_Entity *df_entity_from_path(String8 path, DF_EntityFromPathFlags flags);

//- rjf: file path map override lookups
internal String8List df_possible_overrides_from_file_path(Arena *arena, String8 file_path);

//- rjf: top-level state queries
internal DF_Entity *df_entity_root(void);
internal DF_EntityList df_push_entity_list_with_kind(Arena *arena, DF_EntityKind kind);
internal DF_Entity *df_entity_from_id(DF_EntityID id);
internal DF_Entity *df_machine_entity_from_machine_id(CTRL_MachineID machine_id);
internal DF_Entity *df_entity_from_ctrl_handle(CTRL_MachineID machine_id, DMN_Handle handle);
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
//~ rjf: Stepping "Trap Net" Builders

internal CTRL_TrapList df_trap_net_from_thread__step_over_inst(Arena *arena, DF_Entity *thread);
internal CTRL_TrapList df_trap_net_from_thread__step_over_line(Arena *arena, DF_Entity *thread);
internal CTRL_TrapList df_trap_net_from_thread__step_into_line(Arena *arena, DF_Entity *thread);

////////////////////////////////
//~ rjf: Modules & Debug Info Mappings

//- rjf: module <=> debug info keys
internal DI_Key df_dbgi_key_from_module(DF_Entity *module);
internal DF_EntityList df_modules_from_dbgi_key(Arena *arena, DI_Key *dbgi_key);

//- rjf: voff <=> vaddr
internal U64 df_base_vaddr_from_module(DF_Entity *module);
internal U64 df_voff_from_vaddr(DF_Entity *module, U64 vaddr);
internal U64 df_vaddr_from_voff(DF_Entity *module, U64 voff);
internal Rng1U64 df_voff_range_from_vaddr_range(DF_Entity *module, Rng1U64 vaddr_rng);
internal Rng1U64 df_vaddr_range_from_voff_range(DF_Entity *module, Rng1U64 voff_rng);

////////////////////////////////
//~ rjf: Debug Info Lookups

//- rjf: voff|vaddr -> symbol lookups
internal String8 df_symbol_name_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff, B32 decorated);
internal String8 df_symbol_name_from_process_vaddr(Arena *arena, DF_Entity *process, U64 vaddr, B32 decorated);

//- rjf: symbol -> voff lookups
internal U64 df_voff_from_dbgi_key_symbol_name(DI_Key *dbgi_key, String8 symbol_name);
internal U64 df_type_num_from_dbgi_key_name(DI_Key *dbgi_key, String8 name);

//- rjf: voff -> line info
internal DF_LineList df_lines_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff);

//- rjf: file:line -> line info
internal DF_LineListArray df_lines_array_from_file_path_line_range(Arena *arena, String8 file_path, Rng1S64 line_num_range);
internal DF_LineList df_lines_from_file_path_line_num(Arena *arena, String8 file_path, S64 line_num);

////////////////////////////////
//~ rjf: Process/Thread/Module Info Lookups

internal DF_Entity *df_module_from_process_vaddr(DF_Entity *process, U64 vaddr);
internal DF_Entity *df_module_from_thread(DF_Entity *thread);
internal U64 df_tls_base_vaddr_from_process_root_rip(DF_Entity *process, U64 root_vaddr, U64 rip_vaddr);
internal Architecture df_architecture_from_entity(DF_Entity *entity);
internal E_String2NumMap *df_push_locals_map_from_dbgi_key_voff(Arena *arena, DI_Scope *scope, DI_Key *dbgi_key, U64 voff);
internal E_String2NumMap *df_push_member_map_from_dbgi_key_voff(Arena *arena, DI_Scope *scope, DI_Key *dbgi_key, U64 voff);
internal B32 df_set_thread_rip(DF_Entity *thread, U64 vaddr);
internal DF_Entity *df_module_from_thread_candidates(DF_Entity *thread, DF_EntityList *candidates);
internal DF_Unwind df_unwind_from_ctrl_unwind(Arena *arena, DI_Scope *di_scope, DF_Entity *process, CTRL_Unwind *base_unwind);

////////////////////////////////
//~ rjf: Target Controls

//- rjf: control message dispatching
internal void df_push_ctrl_msg(CTRL_Msg *msg);

//- rjf: control thread running
internal void df_ctrl_run(DF_RunKind run, DF_Entity *run_thread, CTRL_RunFlags flags, CTRL_TrapList *run_traps);

//- rjf: stopped info from the control thread
internal CTRL_Event df_ctrl_last_stop_event(void);

////////////////////////////////
//~ rjf: Evaluation Spaces

//- rjf: entity <-> eval space
internal DF_Entity *df_entity_from_eval_space(E_Space space);
internal E_Space df_eval_space_from_entity(DF_Entity *entity);

//- rjf: eval space reads/writes
internal B32 df_eval_space_read(void *u, E_Space space, void *out, Rng1U64 range);
internal B32 df_eval_space_write(void *u, E_Space space, void *in, Rng1U64 range);

//- rjf: asynchronous streamed reads -> hashes from spaces
internal U128 df_key_from_eval_space_range(E_Space space, Rng1U64 range, B32 zero_terminated);

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

//- rjf: expr * view rule table -> expr
internal E_Expr *df_expr_from_expr_cfg(Arena *arena, E_Expr *expr, DF_CfgTable *cfg);

//- rjf: evaluation value string builder helpers
internal String8 df_string_from_ascii_value(Arena *arena, U8 val);
internal String8 df_string_from_hresult_facility_code(U32 code);
internal String8 df_string_from_hresult_code(U32 code);
internal String8 df_string_from_simple_typed_eval(Arena *arena, DF_EvalVizStringFlags flags, U32 radix, E_Eval eval);
internal String8 df_escaped_from_raw_string(Arena *arena, String8 raw);

//- rjf: type info -> expandability/editablity
internal B32 df_type_key_is_expandable(E_TypeKey type_key);
internal B32 df_type_key_is_editable(E_TypeKey type_key);

//- rjf: writing values back to child processes
internal B32 df_commit_eval_value_string(E_Eval dst_eval, String8 string);

//- rjf: type helpers
internal E_MemberArray df_filtered_data_members_from_members_cfg_table(Arena *arena, E_MemberArray members, DF_CfgTable *cfg);
internal DF_EvalLinkBaseChunkList df_eval_link_base_chunk_list_from_eval(Arena *arena, E_TypeKey link_member_type_key, U64 link_member_off, E_Eval eval, U64 cap);
internal DF_EvalLinkBase df_eval_link_base_from_chunk_list_index(DF_EvalLinkBaseChunkList *list, U64 idx);
internal DF_EvalLinkBaseArray df_eval_link_base_array_from_chunk_list(Arena *arena, DF_EvalLinkBaseChunkList *chunks);

//- rjf: viz block collection building
internal DF_EvalVizBlock *df_eval_viz_block_begin(Arena *arena, DF_EvalVizBlockKind kind, DF_ExpandKey parent_key, DF_ExpandKey key, S32 depth);
internal DF_EvalVizBlock *df_eval_viz_block_split_and_continue(Arena *arena, DF_EvalVizBlockList *list, DF_EvalVizBlock *split_block, U64 split_idx);
internal void df_eval_viz_block_end(DF_EvalVizBlockList *list, DF_EvalVizBlock *block);
internal void df_append_expr_eval_viz_blocks__rec(Arena *arena, DF_EvalView *view, DF_ExpandKey parent_key, DF_ExpandKey key, String8 string, E_Expr *expr, DF_CfgTable *cfg_table, S32 depth, DF_EvalVizBlockList *list_out);
internal DF_EvalVizBlockList df_eval_viz_block_list_from_eval_view_expr_keys(Arena *arena, DF_EvalView *eval_view, DF_CfgTable *cfg_table, String8 expr, DF_ExpandKey parent_key, DF_ExpandKey key);
internal void df_eval_viz_block_list_concat__in_place(DF_EvalVizBlockList *dst, DF_EvalVizBlockList *to_push);

//- rjf: viz block list <-> table coordinates
internal S64 df_row_num_from_viz_block_list_key(DF_EvalVizBlockList *blocks, DF_ExpandKey key);
internal DF_ExpandKey df_key_from_viz_block_list_row_num(DF_EvalVizBlockList *blocks, S64 row_num);
internal DF_ExpandKey df_parent_key_from_viz_block_list_row_num(DF_EvalVizBlockList *blocks, S64 row_num);

//- rjf: viz block * index -> expression
internal E_Expr *df_expr_from_eval_viz_block_index(Arena *arena, DF_EvalVizBlock *block, U64 index);

//- rjf: viz row list building
internal DF_EvalVizRow *df_eval_viz_row_list_push_new(Arena *arena, DF_EvalView *eval_view, DF_EvalVizWindowedRowList *rows, DF_EvalVizBlock *block, DF_ExpandKey key, E_Expr *expr);
internal DF_EvalVizWindowedRowList df_eval_viz_windowed_row_list_from_viz_block_list(Arena *arena, DF_EvalView *eval_view, Rng1S64 visible_range, DF_EvalVizBlockList *blocks);

//- rjf: viz row -> strings
internal String8 df_expr_string_from_viz_row(Arena *arena, DF_EvalVizRow *row);

//- rjf: viz row -> expandability/editability
internal B32 df_viz_row_is_expandable(DF_EvalVizRow *row);
internal B32 df_viz_row_is_editable(DF_EvalVizRow *row);

//- rjf: eval / view rule params tree info extraction
internal U64 df_base_offset_from_eval(E_Eval eval);
internal E_Value df_value_from_params(MD_Node *params);
internal E_TypeKey df_type_key_from_params(MD_Node *params);
internal E_Value df_value_from_params_key(MD_Node *params, String8 key);
internal Rng1U64 df_range_from_eval_params(E_Eval eval, MD_Node *params);
internal TXT_LangKind df_lang_kind_from_eval_params(E_Eval eval, MD_Node *params);
internal Architecture df_architecture_from_eval_params(E_Eval eval, MD_Node *params);
internal Vec2S32 df_dim2s32_from_eval_params(E_Eval eval, MD_Node *params);
internal R_Tex2DFormat df_tex2dformat_from_eval_params(E_Eval eval, MD_Node *params);

//- rjf: eval <-> entity
internal DF_Entity *df_entity_from_eval_string(String8 string);
internal String8 df_eval_string_from_entity(Arena *arena, DF_Entity *entity);

//- rjf: eval <-> file path
internal String8 df_file_path_from_eval_string(Arena *arena, String8 string);
internal String8 df_eval_string_from_file_path(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Main State Accessors/Mutators

//- rjf: frame data
internal F32 df_dt(void);
internal U64 df_frame_index(void);
internal Arena *df_frame_arena(void);
internal F64 df_time_in_seconds(void);

//- rjf: interaction registers
internal DF_InteractRegs *df_interact_regs(void);
internal DF_InteractRegs *df_base_interact_regs(void);
internal DF_InteractRegs *df_push_interact_regs(void);
internal DF_InteractRegs *df_pop_interact_regs(void);

//- rjf: undo/redo history
internal DF_StateDeltaHistory *df_state_delta_history(void);

//- rjf: control state
internal DF_RunKind df_ctrl_last_run_kind(void);
internal U64 df_ctrl_last_run_frame_idx(void);
internal U64 df_ctrl_run_gen(void);
internal B32 df_ctrl_targets_running(void);

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

//- rjf: entity kind cache
internal DF_EntityList df_query_cached_entity_list_with_kind(DF_EntityKind kind);

//- rjf: active entity based queries
internal DI_KeyList df_push_active_dbgi_key_list(Arena *arena);
internal DF_EntityList df_push_active_target_list(Arena *arena);

//- rjf: expand key based entity queries
internal DF_Entity *df_entity_from_expand_key_and_kind(DF_ExpandKey key, DF_EntityKind kind);

//- rjf: per-run caches
internal CTRL_Unwind df_query_cached_unwind_from_thread(DF_Entity *thread);
internal U64 df_query_cached_rip_from_thread(DF_Entity *thread);
internal U64 df_query_cached_rip_from_thread_unwind(DF_Entity *thread, U64 unwind_count);
internal U64 df_query_cached_tls_base_vaddr_from_process_root_rip(DF_Entity *process, U64 root_vaddr, U64 rip_vaddr);
internal E_String2NumMap *df_query_cached_locals_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff);
internal E_String2NumMap *df_query_cached_member_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff);

//- rjf: top-level command dispatch
internal void df_push_cmd__root(DF_CmdParams *params, DF_CmdSpec *spec);

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal void df_core_init(CmdLine *cmdln, DF_StateDeltaHistory *hist);
internal DF_CmdList df_core_gather_root_cmds(Arena *arena);
internal void df_core_begin_frame(Arena *arena, DF_CmdList *cmds, F32 dt);
internal void df_core_end_frame(void);

#endif // DF_CORE_H
