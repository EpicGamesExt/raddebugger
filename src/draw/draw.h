// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DRAW_H
#define DRAW_H

////////////////////////////////
//~ rjf: Fancy String Types

typedef struct DR_FancyString DR_FancyString;
struct DR_FancyString
{
  FNT_Tag font;
  String8 string;
  Vec4F32 color;
  F32 size;
  F32 underline_thickness;
  F32 strikethrough_thickness;
};

typedef struct DR_FancyStringNode DR_FancyStringNode;
struct DR_FancyStringNode
{
  DR_FancyStringNode *next;
  DR_FancyString v;
};

typedef struct DR_FancyStringList DR_FancyStringList;
struct DR_FancyStringList
{
  DR_FancyStringNode *first;
  DR_FancyStringNode *last;
  U64 node_count;
  U64 total_size;
};

typedef struct DR_FancyRun DR_FancyRun;
struct DR_FancyRun
{
  FNT_Run run;
  Vec4F32 color;
  F32 underline_thickness;
  F32 strikethrough_thickness;
};

typedef struct DR_FancyRunNode DR_FancyRunNode;
struct DR_FancyRunNode
{
  DR_FancyRunNode *next;
  DR_FancyRun v;
};

typedef struct DR_FancyRunList DR_FancyRunList;
struct DR_FancyRunList
{
  DR_FancyRunNode *first;
  DR_FancyRunNode *last;
  U64 node_count;
  Vec2F32 dim;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/draw.meta.h"

////////////////////////////////
//~ rjf: Draw Bucket Types

typedef struct DR_Bucket DR_Bucket;
struct DR_Bucket
{
  R_PassList passes;
  U64 stack_gen;
  U64 last_cmd_stack_gen;
  DR_BucketStackDecls;
};

////////////////////////////////
//~ rjf: Thread Context

typedef struct DR_BucketSelectionNode DR_BucketSelectionNode;
struct DR_BucketSelectionNode
{
  DR_BucketSelectionNode *next;
  DR_Bucket *bucket;
};

typedef struct DR_ThreadCtx DR_ThreadCtx;
struct DR_ThreadCtx
{
  Arena *arena;
  U64 arena_frame_start_pos;
  DR_BucketSelectionNode *top_bucket;
  DR_BucketSelectionNode *free_bucket_selection;
};

////////////////////////////////
//~ rjf: Globals

thread_static DR_ThreadCtx *dr_thread_ctx = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 dr_hash_from_string(String8 string);

////////////////////////////////
//~ rjf: Fancy String Type Functions

internal void dr_fancy_string_list_push(Arena *arena, DR_FancyStringList *list, DR_FancyString *str);
#define dr_fancy_string_list_push_new(arena, list, font_, size_, color_, string_, ...) dr_fancy_string_list_push((arena), (list), &(DR_FancyString){.font = (font_), .string = (string_), .color = (color_), .size = (size_), __VA_ARGS__})
internal void dr_fancy_string_list_concat_in_place(DR_FancyStringList *dst, DR_FancyStringList *to_push);
internal String8 dr_string_from_fancy_string_list(Arena *arena, DR_FancyStringList *list);
internal DR_FancyRunList dr_fancy_run_list_from_fancy_string_list(Arena *arena, F32 tab_size_px, FNT_RasterFlags flags, DR_FancyStringList *strs);
internal DR_FancyRunList dr_fancy_run_list_copy(Arena *arena, DR_FancyRunList *src);

////////////////////////////////
//~ rjf: Top-Level API
//
// (Frame boundaries & bucket submission)

internal void dr_begin_frame(void);
internal void dr_submit_bucket(OS_Handle os_window, R_Handle r_window, DR_Bucket *bucket);

////////////////////////////////
//~ rjf: Bucket Construction & Selection API
//
// (Bucket: Handle to sequence of many render passes, constructed by this layer)

internal DR_Bucket *dr_bucket_make(void);
internal void dr_push_bucket(DR_Bucket *bucket);
internal void dr_pop_bucket(void);
internal DR_Bucket *dr_top_bucket(void);
#define DR_BucketScope(b) DeferLoop(dr_push_bucket(b), dr_pop_bucket())

////////////////////////////////
//~ rjf: Bucket Stacks
//
// (Pushing/popping implicit draw parameters)

internal R_Tex2DSampleKind          dr_push_tex2d_sample_kind(R_Tex2DSampleKind v);
internal Mat3x3F32                  dr_push_xform2d(Mat3x3F32 v);
internal Rng2F32                    dr_push_clip(Rng2F32 v);
internal F32                        dr_push_transparency(F32 v);
internal R_Tex2DSampleKind          dr_pop_tex2d_sample_kind(void);
internal Mat3x3F32                  dr_pop_xform2d(void);
internal Rng2F32                    dr_pop_clip(void);
internal F32                        dr_pop_transparency(void);
internal R_Tex2DSampleKind          dr_top_tex2d_sample_kind(void);
internal Mat3x3F32                  dr_top_xform2d(void);
internal Rng2F32                    dr_top_clip(void);
internal F32                        dr_top_transparency(void);

#define DR_Tex2DSampleKindScope(v)   DeferLoop(dr_push_tex2d_sample_kind(v), dr_pop_tex2d_sample_kind())
#define DR_XForm2DScope(v)           DeferLoop(dr_push_xform2d(v), dr_pop_xform2d())
#define DR_ClipScope(v)              DeferLoop(dr_push_clip(v), dr_pop_clip())
#define DR_TransparencyScope(v)      DeferLoop(dr_push_transparency(v), dr_pop_transparency())

////////////////////////////////
//~ rjf: Core Draw Calls
//
// (Apply to the calling thread's currently selected bucket)

//- rjf: rectangles
internal inline R_Rect2DInst *dr_rect(Rng2F32 dst, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness);

//- rjf: images
internal inline R_Rect2DInst *dr_img(Rng2F32 dst, Rng2F32 src, R_Handle texture, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness);

//- rjf: blurs
internal R_PassParams_Blur *dr_blur(Rng2F32 rect, F32 blur_size, F32 corner_radius);

//- rjf: 3d rendering pass params
internal R_PassParams_Geo3D *dr_geo3d_begin(Rng2F32 viewport, Mat4x4F32 view, Mat4x4F32 projection);

//- rjf: meshes
internal R_Mesh3DInst *dr_mesh(R_Handle mesh_vertices, R_Handle mesh_indices, R_GeoTopologyKind mesh_geo_topology, R_GeoVertexFlags mesh_geo_vertex_flags, R_Handle albedo_tex, Mat4x4F32 inst_xform);

//- rjf: collating one pre-prepped bucket into parent bucket
internal void dr_sub_bucket(DR_Bucket *bucket);

////////////////////////////////
//~ rjf: Draw Call Helpers

//- rjf: text
internal void dr_truncated_fancy_run_list(Vec2F32 p, DR_FancyRunList *list, F32 max_x, FNT_Run trailer_run);
internal void dr_truncated_fancy_run_fuzzy_matches(Vec2F32 p, DR_FancyRunList *list, F32 max_x, FuzzyMatchRangeList *ranges, Vec4F32 color);
internal void dr_text_run(Vec2F32 p, Vec4F32 color, FNT_Run run);
internal void dr_text(FNT_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, FNT_RasterFlags flags, Vec2F32 p, Vec4F32 color, String8 string);

#endif // DRAW_H
