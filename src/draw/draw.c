// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#define D_StackPushImpl(name_upper, name_lower, type, val) \
D_Bucket *bucket = d_top_bucket();\
type old_val = bucket->top_##name_lower->v;\
D_##name_upper##Node *node = push_array(d_thread_ctx->arena, D_##name_upper##Node, 1);\
node->v = (val);\
SLLStackPush(bucket->top_##name_lower, node);\
bucket->stack_gen += 1;\
return old_val

#define D_StackPopImpl(name_upper, name_lower, type) \
D_Bucket *bucket = d_top_bucket();\
type popped_val = bucket->top_##name_lower->v;\
SLLStackPop(bucket->top_##name_lower);\
bucket->stack_gen += 1;\
return popped_val

#define D_StackTopImpl(name_upper, name_lower, type) \
D_Bucket *bucket = d_top_bucket();\
type top_val = bucket->top_##name_lower->v;\
return top_val

#include "generated/draw.meta.c"

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
d_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

////////////////////////////////
//~ rjf: Fancy String Type Functions

internal void
d_fancy_string_list_push(Arena *arena, D_FancyStringList *list, D_FancyString *str)
{
  D_FancyStringNode *n = push_array_no_zero(arena, D_FancyStringNode, 1);
  MemoryCopyStruct(&n->v, str);
  SLLQueuePush(list->first, list->last, n);
  list->node_count += 1;
  list->total_size += str->string.size;
}

internal void
d_fancy_string_list_concat_in_place(D_FancyStringList *dst, D_FancyStringList *to_push)
{
  if(dst->last != 0 && to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->node_count += to_push->node_count;
    dst->total_size += to_push->total_size;
  }
  else if(to_push->first != 0)
  {
    MemoryCopyStruct(dst, to_push);
  }
  MemoryZeroStruct(to_push);
}

internal String8
d_string_from_fancy_string_list(Arena *arena, D_FancyStringList *list)
{
  String8 result = {0};
  result.size = list->total_size;
  result.str = push_array_no_zero(arena, U8, result.size);
  U64 idx = 0;
  for(D_FancyStringNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopy(result.str+idx, n->v.string.str, n->v.string.size);
    idx += n->v.string.size;
  }
  return result;
}

internal D_FancyRunList
d_fancy_run_list_from_fancy_string_list(Arena *arena, F32 tab_size_px, F_RunFlags flags, D_FancyStringList *strs)
{
  ProfBeginFunction();
  D_FancyRunList run_list = {0};
  F32 base_align_px = 0;
  for(D_FancyStringNode *n = strs->first; n != 0; n = n->next)
  {
    D_FancyRunNode *dst_n = push_array(arena, D_FancyRunNode, 1);
    dst_n->v.run = f_push_run_from_string(arena, n->v.font, n->v.size, base_align_px, tab_size_px, flags, n->v.string);
    dst_n->v.color = n->v.color;
    dst_n->v.underline_thickness = n->v.underline_thickness;
    dst_n->v.strikethrough_thickness = n->v.strikethrough_thickness;
    SLLQueuePush(run_list.first, run_list.last, dst_n);
    run_list.node_count += 1;
    run_list.dim.x += dst_n->v.run.dim.x;
    run_list.dim.y = Max(run_list.dim.y, dst_n->v.run.dim.y);
    base_align_px += dst_n->v.run.dim.x;
  }
  ProfEnd();
  return run_list;
}

internal D_FancyRunList
d_fancy_run_list_copy(Arena *arena, D_FancyRunList *src)
{
  D_FancyRunList dst = {0};
  for(D_FancyRunNode *src_n = src->first; src_n != 0; src_n = src_n->next)
  {
    D_FancyRunNode *dst_n = push_array(arena, D_FancyRunNode, 1);
    SLLQueuePush(dst.first, dst.last, dst_n);
    MemoryCopyStruct(&dst_n->v, &src_n->v);
    dst_n->v.run.pieces = f_piece_array_copy(arena, &src_n->v.run.pieces);
    dst.node_count += 1;
  }
  dst.dim = src->dim;
  return dst;
}

////////////////////////////////
//~ rjf: Top-Level API
//
// (Frame boundaries)

internal void
d_begin_frame(void)
{
  if(d_thread_ctx == 0)
  {
    Arena *arena = arena_alloc__sized(GB(64), MB(8));
    d_thread_ctx = push_array(arena, D_ThreadCtx, 1);
    d_thread_ctx->arena = arena;
    d_thread_ctx->arena_frame_start_pos = arena_pos(arena);
  }
  arena_pop_to(d_thread_ctx->arena, d_thread_ctx->arena_frame_start_pos);
  d_thread_ctx->free_bucket_selection = 0;
  d_thread_ctx->top_bucket = 0;
}

internal void
d_submit_bucket(OS_Handle os_window, R_Handle r_window, D_Bucket *bucket)
{
  r_window_submit(os_window, r_window, &bucket->passes);
}

////////////////////////////////
//~ rjf: Bucket Construction & Selection API
//
// (Bucket: Handle to sequence of many render passes, constructed by this layer)

internal D_Bucket *
d_bucket_make(void)
{
  D_Bucket *bucket = push_array(d_thread_ctx->arena, D_Bucket, 1);
  D_BucketStackInits(bucket);
  return bucket;
}

internal void
d_push_bucket(D_Bucket *bucket)
{
  D_BucketSelectionNode *node = d_thread_ctx->free_bucket_selection;
  if(node)
  {
    SLLStackPop(d_thread_ctx->free_bucket_selection);
  }
  else
  {
    node = push_array(d_thread_ctx->arena, D_BucketSelectionNode, 1);
  }
  SLLStackPush(d_thread_ctx->top_bucket, node);
  node->bucket = bucket;
}

internal void
d_pop_bucket(void)
{
  D_BucketSelectionNode *node = d_thread_ctx->top_bucket;
  SLLStackPop(d_thread_ctx->top_bucket);
  SLLStackPush(d_thread_ctx->free_bucket_selection, node);
}

internal D_Bucket *
d_top_bucket(void)
{
  D_Bucket *bucket = 0;
  if(d_thread_ctx->top_bucket != 0)
  {
    bucket = d_thread_ctx->top_bucket->bucket;
  }
  return bucket;
}

////////////////////////////////
//~ rjf: Bucket Stacks
//
// (Pushing/popping implicit draw parameters)

// NOTE(rjf): (The implementation of the push/pop/top functions is auto-generated)

////////////////////////////////
//~ rjf: Draw Calls
//
// (Apply to the calling thread's currently selected bucket)

//- rjf: rectangles

internal inline R_Rect2DInst *
d_rect(Rng2F32 dst, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_UI);
  R_PassParams_UI *params = pass->params_ui;
  R_BatchGroup2DList *rects = &params->rects;
  R_BatchGroup2DNode *node = rects->last;
  if(node == 0 || bucket->stack_gen != bucket->last_cmd_stack_gen)
  {
    node = push_array(arena, R_BatchGroup2DNode, 1);
    SLLQueuePush(rects->first, rects->last, node);
    rects->count += 1;
    node->batches = r_batch_list_make(sizeof(R_Rect2DInst));
    node->params.tex = r_handle_zero();
    node->params.tex_sample_kind = bucket->top_tex2d_sample_kind->v;
    node->params.xform           = bucket->top_xform2d->v;
    node->params.clip            = bucket->top_clip->v;
    node->params.transparency    = bucket->top_transparency->v;
  }
  R_Rect2DInst *inst = (R_Rect2DInst *)r_batch_list_push_inst(arena, &node->batches, 256);
  inst->dst = dst;
  inst->src = r2f32p(0, 0, 0, 0);
  inst->colors[Corner_00] = color;
  inst->colors[Corner_01] = color;
  inst->colors[Corner_10] = color;
  inst->colors[Corner_11] = color;
  inst->corner_radii[Corner_00] = corner_radius;
  inst->corner_radii[Corner_01] = corner_radius;
  inst->corner_radii[Corner_10] = corner_radius;
  inst->corner_radii[Corner_11] = corner_radius;
  inst->border_thickness = border_thickness;
  inst->edge_softness = edge_softness;
  inst->white_texture_override = 1.f;
  bucket->last_cmd_stack_gen = bucket->stack_gen;
  return inst;
}

//- rjf: images

internal inline R_Rect2DInst *
d_img(Rng2F32 dst, Rng2F32 src, R_Handle texture, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_UI);
  R_PassParams_UI *params = pass->params_ui;
  R_BatchGroup2DList *rects = &params->rects;
  R_BatchGroup2DNode *node = rects->last;
  if(node != 0 && bucket->stack_gen == bucket->last_cmd_stack_gen && r_handle_match(node->params.tex, r_handle_zero()))
  {
    node->params.tex = texture;
  }
  else if(node == 0 || bucket->stack_gen != bucket->last_cmd_stack_gen || !r_handle_match(texture, node->params.tex))
  {
    node = push_array(arena, R_BatchGroup2DNode, 1);
    SLLQueuePush(rects->first, rects->last, node);
    rects->count += 1;
    node->batches = r_batch_list_make(sizeof(R_Rect2DInst));
    node->params.tex             = texture;
    node->params.tex_sample_kind = bucket->top_tex2d_sample_kind->v;
    node->params.xform           = bucket->top_xform2d->v;
    node->params.clip            = bucket->top_clip->v;
    node->params.transparency    = bucket->top_transparency->v;
  }
  R_Rect2DInst *inst = (R_Rect2DInst *)r_batch_list_push_inst(arena, &node->batches, 256);
  inst->dst = dst;
  inst->src = src;
  inst->colors[Corner_00] = color;
  inst->colors[Corner_01] = color;
  inst->colors[Corner_10] = color;
  inst->colors[Corner_11] = color;
  inst->corner_radii[Corner_00] = corner_radius;
  inst->corner_radii[Corner_01] = corner_radius;
  inst->corner_radii[Corner_10] = corner_radius;
  inst->corner_radii[Corner_11] = corner_radius;
  inst->border_thickness = border_thickness;
  inst->edge_softness = edge_softness;
  inst->white_texture_override = 0.f;
  bucket->last_cmd_stack_gen = bucket->stack_gen;
  return inst;
}

//- rjf: blurs

internal R_PassParams_Blur *
d_blur(Rng2F32 rect, F32 blur_size, F32 corner_radius)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Blur);
  R_PassParams_Blur *params = pass->params_blur;
  params->rect = rect;
  params->blur_size = blur_size;
  params->corner_radii[Corner_00] = corner_radius;
  params->corner_radii[Corner_01] = corner_radius;
  params->corner_radii[Corner_10] = corner_radius;
  params->corner_radii[Corner_11] = corner_radius;
  return params;
}

//- rjf: 3d rendering pass params

internal R_PassParams_Geo3D *
d_geo3d_begin(Rng2F32 viewport, Mat4x4F32 view, Mat4x4F32 projection)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo3D);
  R_PassParams_Geo3D *params = pass->params_geo3d;
  params->viewport = viewport;
  params->view = view;
  params->projection = projection;
  return params;
}

//- rjf: meshes

internal R_Mesh3DInst *
d_mesh(R_Handle mesh_vertices, R_Handle mesh_indices, R_GeoTopologyKind mesh_geo_topology, R_GeoVertexFlags mesh_geo_vertex_flags, R_Handle albedo_tex, Mat4x4F32 inst_xform)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo3D);
  R_PassParams_Geo3D *params = pass->params_geo3d;
  
  // rjf: mesh batch map not made yet -> make
  if(params->mesh_batches.slots_count == 0)
  {
    params->mesh_batches.slots_count = 64;
    params->mesh_batches.slots = push_array(arena, R_BatchGroup3DMapNode *, params->mesh_batches.slots_count);
  }
  
  // rjf: hash batch group 3d params
  U64 hash = 0;
  U64 slot_idx = 0;
  {
    U64 buffer[] =
    {
      mesh_vertices.u64[0],
      mesh_vertices.u64[1],
      mesh_indices.u64[0],
      mesh_indices.u64[1],
      (U64)mesh_geo_topology,
      (U64)mesh_geo_vertex_flags,
      albedo_tex.u64[0],
      albedo_tex.u64[1],
      (U64)d_top_tex2d_sample_kind(),
    };
    hash = d_hash_from_string(str8((U8 *)buffer, sizeof(buffer)));
    slot_idx = hash%params->mesh_batches.slots_count;
  }
  
  // rjf: map hash -> existing batch group node
  R_BatchGroup3DMapNode *node = 0;
  {
    for(R_BatchGroup3DMapNode *n = params->mesh_batches.slots[slot_idx]; n != 0; n = n->next)
    {
      if(n->hash == hash)
      {
        node = n;
        break;
      }
    }
  }
  
  // rjf: no batch group node? -> make one
  if(node == 0)
  {
    node = push_array(arena, R_BatchGroup3DMapNode, 1);
    SLLStackPush(params->mesh_batches.slots[slot_idx], node);
    node->hash = hash;
    node->batches = r_batch_list_make(sizeof(R_Mesh3DInst));
    node->params.mesh_vertices = mesh_vertices;
    node->params.mesh_indices = mesh_indices;
    node->params.mesh_geo_topology = mesh_geo_topology;
    node->params.mesh_geo_vertex_flags = mesh_geo_vertex_flags;
    node->params.albedo_tex = albedo_tex;
    node->params.albedo_tex_sample_kind = d_top_tex2d_sample_kind();
    node->params.xform = mat_4x4f32(1.f);
  }
  
  // rjf: push new instance to batch group
  R_Mesh3DInst *inst = (R_Mesh3DInst *)r_batch_list_push_inst(arena, &node->batches, 256);
  inst->xform = inst_xform;
  return inst;
}

//- rjf: collating one pre-prepped bucket into parent bucket

internal void
d_sub_bucket(D_Bucket *bucket)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *src = bucket;
  D_Bucket *dst = d_top_bucket();
  Rng2F32 dst_clip = d_top_clip();
  B32 dst_clip_is_set = !(dst_clip.x0 == 0 && dst_clip.x1 == 0 &&
                          dst_clip.y0 == 0 && dst_clip.y1 == 0);
  for(R_PassNode *n = src->passes.first; n != 0; n = n->next)
  {
    R_Pass *src_pass = &n->v;
    R_Pass *dst_pass = r_pass_from_kind(arena, &dst->passes, src_pass->kind);
    switch(dst_pass->kind)
    {
      default:{dst_pass->params = src_pass->params;}break;
      case R_PassKind_UI:
      {
        R_PassParams_UI *src_ui = src_pass->params_ui;
        R_PassParams_UI *dst_ui = dst_pass->params_ui;
        for(R_BatchGroup2DNode *src_group_n = src_ui->rects.first;
            src_group_n != 0;
            src_group_n = src_group_n->next)
        {
          R_BatchGroup2DNode *dst_group_n = push_array(arena, R_BatchGroup2DNode, 1);
          SLLQueuePush(dst_ui->rects.first, dst_ui->rects.last, dst_group_n);
          dst_ui->rects.count += 1;
          MemoryCopyStruct(&dst_group_n->params, &src_group_n->params);
          dst_group_n->batches = src_group_n->batches;
          dst_group_n->params.xform = d_top_xform2d();
          if(dst_clip_is_set)
          {
            B32 clip_is_set = !(dst_group_n->params.clip.x0 == 0 &&
                                dst_group_n->params.clip.y0 == 0 &&
                                dst_group_n->params.clip.x1 == 0 &&
                                dst_group_n->params.clip.y1 == 0);
            dst_group_n->params.clip = clip_is_set ? intersect_2f32(dst_clip, dst_group_n->params.clip) : dst_clip;
          }
        }
      }break;
    }
  }
}

////////////////////////////////
//~ rjf: Draw Call Helpers

//- rjf: text

internal void
d_truncated_fancy_run_list(Vec2F32 p, D_FancyRunList *list, F32 max_x, F_Run trailer_run)
{
  ProfBeginFunction();
  
  //- rjf: total advance > max? -> enable trailer
  B32 trailer_enabled = (list->dim.x >= max_x && trailer_run.dim.x < max_x);
  
  //- rjf: draw runs
  F32 advance = 0;
  B32 trailer_found = 0;
  Vec4F32 last_color = {0};
  U64 byte_off = 0;
  for(D_FancyRunNode *n = list->first; n != 0; n = n->next)
  {
    D_FancyRun *fr = &n->v;
    Rng1F32 pixel_range = {0};
    {
      pixel_range.min = 100000;
      pixel_range.max = 0;
    }
    F_Piece *piece_first = fr->run.pieces.v;
    F_Piece *piece_opl = piece_first + fr->run.pieces.count;
    F32 pre_advance = advance;
    F32 last_piece_end_pad = 0;
    last_color = fr->color;
    for(F_Piece *piece = piece_first;
        piece < piece_opl;
        piece += 1)
    {
      if(trailer_enabled && advance + piece->advance >= (max_x - trailer_run.dim.x))
      {
        trailer_found = 1;
        break;
      }
      if(!trailer_enabled && advance + piece->advance >= max_x)
      {
        goto end_draw;
      }
      R_Handle texture = piece->texture;
      Rng2F32 src = r2f32p((F32)piece->subrect.x0, (F32)piece->subrect.y0, (F32)piece->subrect.x1, (F32)piece->subrect.y1);
      Vec2F32 size = dim_2f32(src);
      Rng2F32 dst = r2f32p(p.x + piece->offset.x + advance,
                           p.y + piece->offset.y,
                           p.x + piece->offset.x + advance + size.x,
                           p.y + piece->offset.y + size.y);
      if(!r_handle_match(texture, r_handle_zero()))
      {
        d_img(dst, src, texture, fr->color, 0, 0, 0);
        //d_rect(dst, v4f32(1, 0, 0, 1), 0, 1.f, 0.f);
      }
      advance += piece->advance;
      last_piece_end_pad = ((F32)piece->offset.x+(F32)dim_2s16(piece->subrect).x) - piece->advance;
      pixel_range.min = Min(pre_advance, pixel_range.min);
      pixel_range.max = Max(advance, pixel_range.max);
    }
    if(fr->underline_thickness > 0)
    {
      d_rect(r2f32p(p.x + pixel_range.min + 1.f,
                    p.y+fr->run.descent+fr->run.descent/8,
                    p.x + pixel_range.max + last_piece_end_pad/2,
                    p.y+fr->run.descent+fr->run.descent/8+fr->underline_thickness),
             fr->color, 0, 0, 0.8f);
    }
    if(fr->strikethrough_thickness > 0)
    {
      d_rect(r2f32p(p.x+pre_advance, p.y+fr->run.descent - fr->run.ascent/2, p.x+advance, p.y+fr->run.descent - fr->run.ascent/2 + fr->strikethrough_thickness), fr->color, 0, 0, 1.f);
    }
    if(trailer_found)
    {
      break;
    }
  }
  end_draw:;
  
  //- rjf: draw trailer
  if(trailer_found)
  {
    F_Piece *piece_first = trailer_run.pieces.v;
    F_Piece *piece_opl = piece_first + trailer_run.pieces.count;
    F32 pre_advance = advance;
    Vec4F32 trailer_piece_color = last_color;
    for(F_Piece *piece = piece_first;
        piece < piece_opl;
        piece += 1)
    {
      R_Handle texture = piece->texture;
      Rng2F32 src = r2f32p((F32)piece->subrect.x0, (F32)piece->subrect.y0, (F32)piece->subrect.x1, (F32)piece->subrect.y1);
      Vec2F32 size = dim_2f32(src);
      Rng2F32 dst = r2f32p(p.x + piece->offset.x + advance,
                           p.y + piece->offset.y,
                           p.x + piece->offset.x + advance + size.x,
                           p.y + piece->offset.y + size.y);
      if(!r_handle_match(texture, r_handle_zero()))
      {
        d_img(dst, src, texture, trailer_piece_color, 0, 0, 0);
        trailer_piece_color.w *= 0.5f;
      }
      advance += piece->advance;
    }
  }
  
  ProfEnd();
}

internal void
d_truncated_fancy_run_fuzzy_matches(Vec2F32 p, D_FancyRunList *list, F32 max_x, FuzzyMatchRangeList *ranges, Vec4F32 color)
{
  for(FuzzyMatchRangeNode *match_n = ranges->first; match_n != 0; match_n = match_n->next)
  {
    Rng1U64 byte_range = match_n->range;
    Rng1F32 pixel_range = {0};
    {
      pixel_range.min = 100000;
      pixel_range.max = 0;
    }
    F32 last_piece_end_pad = 0;
    U64 byte_off = 0;
    F32 advance = 0;
    F32 ascent = 0;
    F32 descent = 0;
    for(D_FancyRunNode *fr_n = list->first; fr_n != 0; fr_n = fr_n->next)
    {
      D_FancyRun *fr = &fr_n->v;
      F_Run *run = &fr->run;
      ascent = run->ascent;
      descent = run->descent;
      for(U64 piece_idx = 0; piece_idx < run->pieces.count; piece_idx += 1)
      {
        F_Piece *piece = &run->pieces.v[piece_idx];
        if(contains_1u64(byte_range, byte_off))
        {
          F32 pre_advance  = advance+piece->offset.x;
          F32 post_advance = advance+piece->advance;
          last_piece_end_pad = ((F32)piece->offset.x+(F32)dim_2s16(piece->subrect).x) - piece->advance;
          pixel_range.min = Min(pre_advance,  pixel_range.min);
          pixel_range.max = Max(post_advance, pixel_range.max);
        }
        byte_off += piece->decode_size;
        advance += piece->advance;
      }
    }
    if(pixel_range.min < pixel_range.max)
    {
      Rng2F32 rect = r2f32p(p.x + pixel_range.min, p.y - descent - ascent,
                            p.x + pixel_range.max + last_piece_end_pad/2, p.y - descent - ascent + list->dim.y);
      rect.x0 = Min(rect.x0, p.x+max_x);
      rect.x1 = Min(rect.x1, p.x+max_x);
      d_rect(rect, color, (descent+ascent)/4.f, 0, 1.f);
    }
  }
}

internal void
d_text_run(Vec2F32 p, Vec4F32 color, F_Run run)
{
  F32 advance = 0;
  F_Piece *piece_first = run.pieces.v;
  F_Piece *piece_opl = piece_first + run.pieces.count;
  for(F_Piece *piece = piece_first;
      piece < piece_opl;
      piece += 1)
  {
    R_Handle texture = piece->texture;
    Rng2F32 src = r2f32p((F32)piece->subrect.x0, (F32)piece->subrect.y0, (F32)piece->subrect.x1, (F32)piece->subrect.y1);
    Vec2F32 size = dim_2f32(src);
    Rng2F32 dst = r2f32p(p.x + piece->offset.x + advance,
                         p.y + piece->offset.y,
                         p.x + piece->offset.x + advance + size.x,
                         p.y + piece->offset.y + size.y);
    if(size.x != 0 && size.y != 0 && !r_handle_match(texture, r_handle_zero()))
    {
      d_img(dst, src, texture, color, 0, 0, 0);
    }
    advance += piece->advance;
  }
}

internal void
d_truncated_text_run(Vec2F32 p, Vec4F32 color, F32 max_x, F_Run text_run, F_Run trailer_run)
{
  B32 truncated = 0;
  B32 set_truncation = 0;
  F32 truncation_p = p.x;
  F32 max_x_minus_ellipses = max_x - trailer_run.dim.x;
  F32 available_space = max_x - p.x;
  
  // rjf: find last piece before truncation
  B32 truncation_needed = 0;
  F_Piece *last_piece_before_truncation = 0;
  F32 truncation_offset = 0;
  if(available_space > text_run.dim.x || available_space > trailer_run.dim.x)
  {
    F32 advance = 0;
    F_Piece *text_run_first = text_run.pieces.v;
    F_Piece *text_run_opl = text_run_first + text_run.pieces.count;
    for(F_Piece *piece = text_run_first;
        piece < text_run_opl;
        piece += 1)
    {
      Rng2F32 src = r2f32p((F32)piece->subrect.x0, (F32)piece->subrect.y0, (F32)piece->subrect.x1, (F32)piece->subrect.y1);
      Vec2F32 size = dim_2f32(src);
      Rng2F32 dst = r2f32p(p.x + piece->offset.x + advance,
                           p.y + piece->offset.y,
                           p.x + piece->offset.x + advance + size.x,
                           p.y + piece->offset.y + size.y);
      advance += piece->advance;
      if(last_piece_before_truncation == 0 && p.x + advance > max_x_minus_ellipses)
      {
        truncation_offset = advance - piece->advance;
        last_piece_before_truncation = piece;
      }
      if(p.x + advance > max_x)
      {
        truncation_needed = 1;
      }
    }
  }
  
  // rjf: draw pieces
  if(available_space > text_run.dim.x || available_space > trailer_run.dim.x)
  {
    F32 advance = 0;
    F_Piece *text_run_first = text_run.pieces.v;
    F_Piece *text_run_opl = text_run_first + text_run.pieces.count;
    for(F_Piece *piece = text_run_first;
        piece < text_run_opl;
        piece += 1)
    {
      if(truncation_needed && piece == last_piece_before_truncation)
      {
        break;
      }
      R_Handle texture = piece->texture;
      Rng2F32 src = r2f32p((F32)piece->subrect.x0, (F32)piece->subrect.y0, (F32)piece->subrect.x1, (F32)piece->subrect.y1);
      Vec2F32 size = dim_2f32(src);
      Rng2F32 dst = r2f32p(p.x + piece->offset.x + advance,
                           p.y + piece->offset.y,
                           p.x + piece->offset.x + advance + size.x,
                           p.y + piece->offset.y + size.y);
      if(size.x != 0 && size.y != 0 && !r_handle_match(texture, r_handle_zero()))
      {
        d_img(dst, src, texture, color, 0, 0, 0);
      }
      advance += piece->advance;
    }
  }
  
  // rjf: draw truncation ellipses
  if(truncation_needed && last_piece_before_truncation != 0)
  {
    Vec2F32 ellipses_p = {p.x + truncation_offset, p.y};
    Vec4F32 ellipses_color = color;
    F32 advance = 0;
    F_Piece *trailer_run_first = trailer_run.pieces.v;
    F_Piece *trailer_run_opl = trailer_run_first + trailer_run.pieces.count;
    for(F_Piece *piece = trailer_run_first;
        piece < trailer_run_opl;
        piece += 1)
    {
      R_Handle texture = piece->texture;
      Rng2F32 src = r2f32p((F32)piece->subrect.x0, (F32)piece->subrect.y0, (F32)piece->subrect.x1, (F32)piece->subrect.y1);
      Vec2F32 size = dim_2f32(src);
      Rng2F32 dst = r2f32p(ellipses_p.x + piece->offset.x + advance,
                           ellipses_p.y + piece->offset.y,
                           ellipses_p.x + piece->offset.x + advance + size.x,
                           ellipses_p.y + piece->offset.y + size.y);
      if(size.x != 0 && size.y != 0 && !r_handle_match(texture, r_handle_zero()))
      {
        d_img(dst, src, texture, ellipses_color, 0, 0, 0);
      }
      ellipses_color.w *= 0.5f;
      advance += piece->advance;
    }
  }
}

internal void
d_text(F_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, F_RunFlags flags, Vec2F32 p, Vec4F32 color, String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  F_Run run = f_push_run_from_string(scratch.arena, font, size, base_align_px, tab_size_px, flags, string);
  d_text_run(p, color, run);
  scratch_end(scratch);
}

internal void
d_textf(F_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, F_RunFlags flags, Vec2F32 p, Vec4F32 color, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  d_text(font, size, base_align_px, tab_size_px, flags, p, color, string);
  scratch_end(scratch);
}

internal void
d_truncated_text(F_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, F_RunFlags flags, Vec2F32 p, Vec4F32 color, F32 max_x, String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  F_Run run = f_push_run_from_string(scratch.arena, font, size, base_align_px, tab_size_px, flags, string);
  F_Run ellipses_run = f_push_run_from_string(scratch.arena, font, size, base_align_px, tab_size_px, 0, str8_lit("..."));
  d_truncated_text_run(p, color, max_x, run, ellipses_run);
  scratch_end(scratch);
}

internal void
d_truncated_textf(F_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, F_RunFlags flags, Vec2F32 p, Vec4F32 color, F32 max_x, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8f(scratch.arena, fmt, args);
  d_truncated_text(font, size, base_align_px, tab_size_px, flags, p, color, max_x, string);
  va_end(args);
  scratch_end(scratch);
}
