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
//~ rjf: "text"

typedef struct DF_TxtTopologyInfo DF_TxtTopologyInfo;
struct DF_TxtTopologyInfo
{
  TXT_LangKind lang;
};

typedef struct DF_VR_TextState DF_VR_TextState;
struct DF_VR_TextState
{
  B32 initialized;
  TxtPt cursor;
  TxtPt mark;
  S64 preferred_column;
  U64 last_open_frame_idx;
  F32 loaded_t;
};

internal DF_TxtTopologyInfo df_vr_txt_topology_info_from_cfg(DF_CfgNode *cfg);

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

internal DF_DisasmTopologyInfo df_vr_disasm_topology_info_from_cfg(DF_CfgNode *cfg);

////////////////////////////////
//~ rjf: "bitmap"

typedef struct DF_BitmapTopologyInfo DF_BitmapTopologyInfo;
struct DF_BitmapTopologyInfo
{
  U64 width;
  U64 height;
  R_Tex2DFormat fmt;
};

#if 0
typedef struct DF_BitmapViewState DF_BitmapViewState;
struct DF_BitmapViewState
{
  Vec2F32 view_center_pos;
  F32 zoom;
  DF_BitmapTopologyInfo top;
};
#endif

typedef struct DF_VR_BitmapState DF_VR_BitmapState;
struct DF_VR_BitmapState
{
  U64 last_open_frame_idx;
  F32 loaded_t;
};

typedef struct DF_VR_BitmapBoxDrawData DF_VR_BitmapBoxDrawData;
struct DF_VR_BitmapBoxDrawData
{
  Rng2F32 src;
  R_Handle texture;
  F32 loaded_t;
  B32 hovered;
  Vec2S32 mouse_px;
  F32 ui_per_bmp_px;
};

internal Vec2F32 df_bitmap_view_state__screen_from_canvas_pos(DF_BitmapViewState *bvs, Rng2F32 rect, Vec2F32 cvs);
internal Rng2F32 df_bitmap_view_state__screen_from_canvas_rect(DF_BitmapViewState *bvs, Rng2F32 rect, Rng2F32 cvs);
internal Vec2F32 df_bitmap_view_state__canvas_from_screen_pos(DF_BitmapViewState *bvs, Rng2F32 rect, Vec2F32 scr);
internal Rng2F32 df_bitmap_view_state__canvas_from_screen_rect(DF_BitmapViewState *bvs, Rng2F32 rect, Rng2F32 scr);
internal DF_BitmapTopologyInfo df_vr_bitmap_topology_info_from_cfg(DF_CfgNode *cfg);

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

internal DF_GeoTopologyInfo df_vr_geo_topology_info_from_cfg(DF_CfgNode *cfg);

#endif // DF_VIEW_RULES_H
