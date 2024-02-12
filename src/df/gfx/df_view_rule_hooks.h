// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DF_VIEW_RULE_HOOKS_H
#define DF_VIEW_RULE_HOOKS_H

////////////////////////////////
//~ rjf: Helper Types

typedef struct DF_BitmapTopologyInfo DF_BitmapTopologyInfo;
struct DF_BitmapTopologyInfo
{
  U64 width;
  U64 height;
  R_Tex2DFormat fmt;
};

typedef struct DF_GeoTopologyInfo DF_GeoTopologyInfo;
struct DF_GeoTopologyInfo
{
  U64 index_count;
  Rng1U64 vertices_vaddr_range;
};

typedef struct DF_TxtTopologyInfo DF_TxtTopologyInfo;
struct DF_TxtTopologyInfo
{
  TXT_LangKind lang;
  U64 size_cap;
};

////////////////////////////////
//~ rjf: Helpers

internal Vec4F32 df_view_rule_hooks__rgba_from_eval(DF_Eval eval, TG_Graph *graph, RADDBGI_Parsed *raddbg, DF_Entity *process);
internal void df_view_rule_hooks__eval_commit_rgba(DF_Eval eval, TG_Graph *graph, RADDBGI_Parsed *raddbg, DF_CtrlCtx *ctrl_ctx, Vec4F32 rgba);
internal DF_BitmapTopologyInfo df_view_rule_hooks__bitmap_topology_info_from_cfg(DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_CfgNode *cfg);
internal DF_GeoTopologyInfo df_view_rule_hooks__geo_topology_info_from_cfg(DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_CfgNode *cfg);
internal DF_TxtTopologyInfo df_view_rule_hooks__txt_topology_info_from_cfg(DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_CfgNode *cfg);

#endif //DF_VIEW_RULE_HOOKS_H
