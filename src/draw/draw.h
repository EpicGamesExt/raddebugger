// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DRAW_H
#define DRAW_H

////////////////////////////////
//~ rjf: Fancy String Types

typedef struct D_FancyString D_FancyString;
struct D_FancyString
{
  F_Tag font;
  String8 string;
  Vec4F32 color;
  F32 size;
  F32 underline_thickness;
  F32 strikethrough_thickness;
};

typedef struct D_FancyStringNode D_FancyStringNode;
struct D_FancyStringNode
{
  D_FancyStringNode *next;
  D_FancyString v;
};

typedef struct D_FancyStringList D_FancyStringList;
struct D_FancyStringList
{
  D_FancyStringNode *first;
  D_FancyStringNode *last;
  U64 node_count;
  U64 total_size;
};

typedef struct D_FancyRun D_FancyRun;
struct D_FancyRun
{
  F_Run run;
  Vec4F32 color;
  F32 underline_thickness;
  F32 strikethrough_thickness;
};

typedef struct D_FancyRunNode D_FancyRunNode;
struct D_FancyRunNode
{
  D_FancyRunNode *next;
  D_FancyRun v;
};

typedef struct D_FancyRunList D_FancyRunList;
struct D_FancyRunList
{
  D_FancyRunNode *first;
  D_FancyRunNode *last;
  U64 node_count;
  Vec2F32 dim;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/draw.meta.h"

////////////////////////////////
//~ rjf: Draw Bucket Types

typedef struct D_Bucket D_Bucket;
struct D_Bucket
{
  R_PassList passes;
  U64 stack_gen;
  U64 last_cmd_stack_gen;
  D_BucketStackDecls;
};

////////////////////////////////
//~ rjf: Thread Context

typedef struct D_BucketSelectionNode D_BucketSelectionNode;
struct D_BucketSelectionNode
{
  D_BucketSelectionNode *next;
  D_Bucket *bucket;
};

typedef struct D_ThreadCtx D_ThreadCtx;
struct D_ThreadCtx
{
  Arena *arena;
  U64 arena_frame_start_pos;
  D_BucketSelectionNode *top_bucket;
  D_BucketSelectionNode *free_bucket_selection;
};

////////////////////////////////
//~ rjf: Globals

thread_static D_ThreadCtx *d_thread_ctx = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 d_hash_from_string(String8 string);

////////////////////////////////
//~ rjf: Fancy String Type Functions

internal void d_fancy_string_list_push(Arena *arena, D_FancyStringList *list, D_FancyString *str);
internal void d_fancy_string_list_concat_in_place(D_FancyStringList *dst, D_FancyStringList *to_push);
internal String8 d_string_from_fancy_string_list(Arena *arena, D_FancyStringList *list);
internal D_FancyRunList d_fancy_run_list_from_fancy_string_list(Arena *arena, F32 tab_size_px, F_RunFlags flags, D_FancyStringList *strs);
internal D_FancyRunList d_fancy_run_list_copy(Arena *arena, D_FancyRunList *src);

////////////////////////////////
//~ rjf: Top-Level API
//
// (Frame boundaries & bucket submission)

internal void d_begin_frame(void);
internal void d_submit_bucket(OS_Handle os_window, R_Handle r_window, D_Bucket *bucket);

////////////////////////////////
//~ rjf: Bucket Construction & Selection API
//
// (Bucket: Handle to sequence of many render passes, constructed by this layer)

internal D_Bucket *d_bucket_make(void);
internal void d_push_bucket(D_Bucket *bucket);
internal void d_pop_bucket(void);
internal D_Bucket *d_top_bucket(void);
#define D_BucketScope(b) DeferLoop(d_push_bucket(b), d_pop_bucket())

////////////////////////////////
//~ rjf: Bucket Stacks
//
// (Pushing/popping implicit draw parameters)

internal R_Tex2DSampleKind          d_push_tex2d_sample_kind(R_Tex2DSampleKind v);
internal Mat3x3F32                  d_push_xform2d(Mat3x3F32 v);
internal Rng2F32                    d_push_clip(Rng2F32 v);
internal F32                        d_push_transparency(F32 v);
internal R_Tex2DSampleKind          d_pop_tex2d_sample_kind(void);
internal Mat3x3F32                  d_pop_xform2d(void);
internal Rng2F32                    d_pop_clip(void);
internal F32                        d_pop_transparency(void);
internal R_Tex2DSampleKind          d_top_tex2d_sample_kind(void);
internal Mat3x3F32                  d_top_xform2d(void);
internal Rng2F32                    d_top_clip(void);
internal F32                        d_top_transparency(void);

#define D_Tex2DSampleKindScope(v)   DeferLoop(d_push_tex2d_sample_kind(v), d_pop_tex2d_sample_kind())
#define D_XForm2DScope(v)           DeferLoop(d_push_xform2d(v), d_pop_xform2d())
#define D_ClipScope(v)              DeferLoop(d_push_clip(v), d_pop_clip())
#define D_TransparencyScope(v)      DeferLoop(d_push_transparency(v), d_pop_transparency())

////////////////////////////////
//~ rjf: Core Draw Calls
//
// (Apply to the calling thread's currently selected bucket)

//- rjf: rectangles
internal inline R_Rect2DInst *d_rect(Rng2F32 dst, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness);

//- rjf: images
internal inline R_Rect2DInst *d_img(Rng2F32 dst, Rng2F32 src, R_Handle texture, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness);

//- rjf: blurs
internal R_PassParams_Blur *d_blur(Rng2F32 rect, F32 blur_size, F32 corner_radius);

//- rjf: 3d rendering pass params
internal R_PassParams_Geo3D *d_geo3d_begin(Rng2F32 viewport, Mat4x4F32 view, Mat4x4F32 projection);

//- rjf: meshes
internal R_Mesh3DInst *d_mesh(R_Handle mesh_vertices, R_Handle mesh_indices, R_GeoTopologyKind mesh_geo_topology, R_GeoVertexFlags mesh_geo_vertex_flags, R_Handle albedo_tex, Mat4x4F32 inst_xform);

//- rjf: collating one pre-prepped bucket into parent bucket
internal void d_sub_bucket(D_Bucket *bucket);

////////////////////////////////
//~ rjf: Draw Call Helpers

//- rjf: text
internal void d_truncated_fancy_run_list(Vec2F32 p, D_FancyRunList *list, F32 max_x, F_Run trailer_run);
internal void d_truncated_fancy_run_fuzzy_matches(Vec2F32 p, D_FancyRunList *list, F32 max_x, FuzzyMatchRangeList *ranges, Vec4F32 color);
internal void d_text_run(Vec2F32 p, Vec4F32 color, F_Run run);
internal void d_truncated_text_run(Vec2F32 p, Vec4F32 color, F32 max_x, F_Run text_run, F_Run trailer_run);
internal void d_text(F_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, F_RunFlags flags, Vec2F32 p, Vec4F32 color, String8 string);
internal void d_textf(F_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, F_RunFlags flags, Vec2F32 p, Vec4F32 color, char *fmt, ...);
internal void d_truncated_text(F_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, F_RunFlags flags, Vec2F32 p, Vec4F32 color, F32 max_x, String8 string);
internal void d_truncated_textf(F_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, F_RunFlags flags, Vec2F32 p, Vec4F32 color, F32 max_x, char *fmt, ...);

#endif // DRAW_H
