// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DF_VIEW_RULES_H
#define DF_VIEW_RULES_H

////////////////////////////////
//~ rjf: "rgba"

typedef struct DF_VR_RGBAState DF_VR_RGBAState;
struct DF_VR_RGBAState
{
  Vec4F32 hsva;
  U64 memgen_idx;
};

internal Vec4F32 df_vr_rgba_from_eval(E_Eval eval, DF_Entity *process);
internal void df_vr_eval_commit_rgba(E_Eval eval, Vec4F32 rgba);

////////////////////////////////
//~ rjf: "disasm"

typedef struct DF_DisasmTopologyInfo DF_DisasmTopologyInfo;
struct DF_DisasmTopologyInfo
{
  Architecture arch;
  U64 size_cap;
};

typedef struct DF_VR_DisasmState DF_VR_DisasmState;
struct DF_VR_DisasmState
{
  B32 initialized;
  TxtPt cursor;
  TxtPt mark;
  S64 preferred_column;
  U64 last_open_frame_idx;
  F32 loaded_t;
};

////////////////////////////////
//~ rjf: "geo"

typedef struct DF_GeoTopologyInfo DF_GeoTopologyInfo;
struct DF_GeoTopologyInfo
{
  U64 index_count;
  Rng1U64 vertices_vaddr_range;
};

typedef struct DF_VR_GeoState DF_VR_GeoState;
struct DF_VR_GeoState
{
  B32 initialized;
  U64 last_open_frame_idx;
  F32 loaded_t;
  F32 pitch;
  F32 pitch_target;
  F32 yaw;
  F32 yaw_target;
  F32 zoom;
  F32 zoom_target;
};

typedef struct DF_VR_GeoBoxDrawData DF_VR_GeoBoxDrawData;
struct DF_VR_GeoBoxDrawData
{
  DF_ExpandKey key;
  R_Handle vertex_buffer;
  R_Handle index_buffer;
  F32 loaded_t;
};

#if 0
internal DF_GeoTopologyInfo df_vr_geo_topology_info_from_cfg(DF_CfgNode *cfg);
#endif

#endif // DF_VIEW_RULES_H
