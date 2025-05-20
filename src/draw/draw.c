// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#define DR_StackPushImpl(name_upper, name_lower, type, val) \
DR_Bucket *bucket = dr_top_bucket();\
type old_val = bucket->top_##name_lower->v;\
DR_##name_upper##Node *node = push_array(dr_thread_ctx->arena, DR_##name_upper##Node, 1);\
node->v = (val);\
SLLStackPush(bucket->top_##name_lower, node);\
bucket->stack_gen += 1;\
return old_val

#define DR_StackPopImpl(name_upper, name_lower, type) \
DR_Bucket *bucket = dr_top_bucket();\
type popped_val = bucket->top_##name_lower->v;\
SLLStackPop(bucket->top_##name_lower);\
bucket->stack_gen += 1;\
return popped_val

#define DR_StackTopImpl(name_upper, name_lower, type) \
DR_Bucket *bucket = dr_top_bucket();\
type top_val = bucket->top_##name_lower->v;\
return top_val

#include "generated/draw.meta.c"

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
dr_hash_from_string(String8 string)
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
dr_fstrs_push(Arena *arena, DR_FStrList *list, DR_FStr *str)
{
  DR_FStrNode *n = push_array_no_zero(arena, DR_FStrNode, 1);
  MemoryCopyStruct(&n->v, str);
  SLLQueuePush(list->first, list->last, n);
  list->node_count += 1;
  list->total_size += str->string.size;
}

internal void
dr_fstrs_push_new_(Arena *arena, DR_FStrList *list, DR_FStrParams *params, DR_FStrParams *overrides, String8 string)
{
  DR_FStr fstr = {string, *params};
  if(!fnt_tag_match(fnt_tag_zero(), overrides->font))
  {
    fstr.params.font = overrides->font;
  }
  if(overrides->raster_flags != 0)
  {
    fstr.params.raster_flags = overrides->raster_flags;
  }
  if(overrides->color.x != 0 || overrides->color.y != 0 || overrides->color.z != 0 || overrides->color.w != 0)
  {
    fstr.params.color = overrides->color;
  }
  if(overrides->size != 0)
  {
    fstr.params.size = overrides->size;
  }
  if(overrides->underline_thickness != 0)
  {
    fstr.params.underline_thickness = overrides->underline_thickness;
  }
  if(overrides->strikethrough_thickness != 0)
  {
    fstr.params.strikethrough_thickness = overrides->strikethrough_thickness;
  }
  dr_fstrs_push(arena, list, &fstr);
}

internal void
dr_fstrs_concat_in_place(DR_FStrList *dst, DR_FStrList *to_push)
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

internal DR_FStrList
dr_fstrs_copy(Arena *arena, DR_FStrList *src)
{
  DR_FStrList dst = {0};
  for(DR_FStrNode *src_n = src->first; src_n != 0; src_n = src_n->next)
  {
    DR_FStr fstr = src_n->v;
    fstr.string = push_str8_copy(arena, fstr.string);
    dr_fstrs_push(arena, &dst, &fstr);
  }
  return dst;
}

internal String8
dr_string_from_fstrs(Arena *arena, DR_FStrList *list)
{
  String8 result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List parts = {0};
    for(DR_FStrNode *n = list->first; n != 0; n = n->next)
    {
      if(!fnt_tag_match(n->v.params.font, dr_thread_ctx->icon_font))
      {
        str8_list_push(scratch.arena, &parts, n->v.string);
      }
    }
    result = str8_list_join(arena, &parts, 0);
    result = str8_skip_chop_whitespace(result);
    scratch_end(scratch);
  }
  return result;
}

internal FuzzyMatchRangeList
dr_fuzzy_match_find_from_fstrs(Arena *arena, DR_FStrList *fstrs, String8 needle)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 fstrs_string = {0};
  fstrs_string.size = fstrs->total_size;
  fstrs_string.str = push_array(arena, U8, fstrs_string.size);
  {
    // TODO(rjf): the fact that we only increment on non-icon portions is super weird?
    // we are only doing that because of the rendering of the fuzzy matches, so maybe
    // once that is straightened out, we can fix the code here too...
    U64 off = 0;
    for(DR_FStrNode *n = fstrs->first; n != 0; n = n->next)
    {
      if(!fnt_tag_match(n->v.params.font, dr_thread_ctx->icon_font))
      {
        MemoryCopy(fstrs_string.str + off, n->v.string.str, n->v.string.size);
        off += n->v.string.size;
      }
    }
  }
  FuzzyMatchRangeList ranges = fuzzy_match_find(arena, needle, fstrs_string);
  scratch_end(scratch);
  return ranges;
}

internal DR_FRunList
dr_fruns_from_fstrs(Arena *arena, F32 tab_size_px, DR_FStrList *strs)
{
  ProfBeginFunction();
  DR_FRunList run_list = {0};
  F32 base_align_px = 0;
  for(DR_FStrNode *n = strs->first; n != 0; n = n->next)
  {
    DR_FRunNode *dst_n = push_array(arena, DR_FRunNode, 1);
    dst_n->v.run = fnt_run_from_string(n->v.params.font, n->v.params.size, base_align_px, tab_size_px, n->v.params.raster_flags, n->v.string);
    dst_n->v.color = n->v.params.color;
    dst_n->v.underline_thickness = n->v.params.underline_thickness;
    dst_n->v.strikethrough_thickness = n->v.params.strikethrough_thickness;
    dst_n->v.icon = (fnt_tag_match(n->v.params.font, dr_thread_ctx->icon_font));
    SLLQueuePush(run_list.first, run_list.last, dst_n);
    run_list.node_count += 1;
    run_list.dim.x += dst_n->v.run.dim.x;
    run_list.dim.y = Max(run_list.dim.y, dst_n->v.run.dim.y);
    base_align_px += dst_n->v.run.dim.x;
  }
  ProfEnd();
  return run_list;
}

internal Vec2F32
dr_dim_from_fstrs(F32 tab_size_px, DR_FStrList *fstrs)
{
  Temp scratch = scratch_begin(0, 0);
  DR_FRunList fruns = dr_fruns_from_fstrs(scratch.arena, tab_size_px, fstrs);
  Vec2F32 dim = fruns.dim;
  scratch_end(scratch);
  return dim;
}

////////////////////////////////
//~ rjf: Top-Level API
//
// (Frame boundaries)

internal void
dr_begin_frame(FNT_Tag icon_font)
{
  if(dr_thread_ctx == 0)
  {
    Arena *arena = arena_alloc(.reserve_size = GB(64), .commit_size = MB(8));
    dr_thread_ctx = push_array(arena, DR_ThreadCtx, 1);
    dr_thread_ctx->arena = arena;
    dr_thread_ctx->arena_frame_start_pos = arena_pos(arena);
  }
  arena_pop_to(dr_thread_ctx->arena, dr_thread_ctx->arena_frame_start_pos);
  dr_thread_ctx->free_bucket_selection = 0;
  dr_thread_ctx->top_bucket = 0;
  dr_thread_ctx->icon_font = icon_font;
}

internal void
dr_submit_bucket(OS_Handle os_window, R_Handle r_window, DR_Bucket *bucket)
{
  r_window_submit(os_window, r_window, &bucket->passes);
}

////////////////////////////////
//~ rjf: Bucket Construction & Selection API
//
// (Bucket: Handle to sequence of many render passes, constructed by this layer)

internal DR_Bucket *
dr_bucket_make(void)
{
  DR_Bucket *bucket = push_array(dr_thread_ctx->arena, DR_Bucket, 1);
  DR_BucketStackInits(bucket);
  return bucket;
}

internal void
dr_push_bucket(DR_Bucket *bucket)
{
  DR_BucketSelectionNode *node = dr_thread_ctx->free_bucket_selection;
  if(node)
  {
    SLLStackPop(dr_thread_ctx->free_bucket_selection);
  }
  else
  {
    node = push_array(dr_thread_ctx->arena, DR_BucketSelectionNode, 1);
  }
  SLLStackPush(dr_thread_ctx->top_bucket, node);
  node->bucket = bucket;
}

internal void
dr_pop_bucket(void)
{
  DR_BucketSelectionNode *node = dr_thread_ctx->top_bucket;
  SLLStackPop(dr_thread_ctx->top_bucket);
  SLLStackPush(dr_thread_ctx->free_bucket_selection, node);
}

internal DR_Bucket *
dr_top_bucket(void)
{
  DR_Bucket *bucket = 0;
  if(dr_thread_ctx->top_bucket != 0)
  {
    bucket = dr_thread_ctx->top_bucket->bucket;
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
dr_rect(Rng2F32 dst, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
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
dr_img(Rng2F32 dst, Rng2F32 src, R_Handle texture, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
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
dr_blur(Rng2F32 rect, F32 blur_size, F32 corner_radius)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Blur);
  R_PassParams_Blur *params = pass->params_blur;
  params->rect = rect;
  params->clip = dr_top_clip();
  params->blur_size = blur_size;
  params->corner_radii[Corner_00] = corner_radius;
  params->corner_radii[Corner_01] = corner_radius;
  params->corner_radii[Corner_10] = corner_radius;
  params->corner_radii[Corner_11] = corner_radius;
  return params;
}

//- rjf: 3d rendering pass params

internal R_PassParams_Geo3D *
dr_geo3d_begin(Rng2F32 viewport, Mat4x4F32 view, Mat4x4F32 projection)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo3D);
  R_PassParams_Geo3D *params = pass->params_geo3d;
  params->viewport = viewport;
  params->view = view;
  params->projection = projection;
  return params;
}

//- rjf: meshes

internal R_Mesh3DInst *
dr_mesh(R_Handle mesh_vertices, R_Handle mesh_indices, R_GeoTopologyKind mesh_geo_topology, R_GeoVertexFlags mesh_geo_vertex_flags, R_Handle albedo_tex, Mat4x4F32 inst_xform)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
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
      (U64)dr_top_tex2d_sample_kind(),
    };
    hash = dr_hash_from_string(str8((U8 *)buffer, sizeof(buffer)));
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
    node->params.albedo_tex_sample_kind = dr_top_tex2d_sample_kind();
    node->params.xform = mat_4x4f32(1.f);
  }
  
  // rjf: push new instance to batch group
  R_Mesh3DInst *inst = (R_Mesh3DInst *)r_batch_list_push_inst(arena, &node->batches, 256);
  inst->xform = inst_xform;
  return inst;
}

//- rjf: collating one pre-prepped bucket into parent bucket

internal void
dr_sub_bucket(DR_Bucket *bucket)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *src = bucket;
  DR_Bucket *dst = dr_top_bucket();
  Rng2F32 dst_clip = dr_top_clip();
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
          dst_group_n->params.xform = dr_top_xform2d();
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
dr_truncated_fancy_run_list(Vec2F32 p, DR_FRunList *list, F32 max_x, FNT_Run trailer_run)
{
  ProfBeginFunction();
  
  //- rjf: total advance > max? -> enable trailer
  B32 trailer_enabled = (list->dim.x > max_x && trailer_run.dim.x < max_x);
  
  //- rjf: draw runs
  F32 advance = 0;
  B32 trailer_found = 0;
  Vec4F32 last_color = {0};
  U64 byte_off = 0;
  for(DR_FRunNode *n = list->first; n != 0; n = n->next)
  {
    DR_FRun *fr = &n->v;
    Rng1F32 pixel_range = {0};
    {
      pixel_range.min = 100000;
      pixel_range.max = 0;
    }
    FNT_Piece *piece_first = fr->run.pieces.v;
    FNT_Piece *piece_opl = piece_first + fr->run.pieces.count;
    F32 pre_advance = advance;
    last_color = fr->color;
    for(FNT_Piece *piece = piece_first;
        piece < piece_opl;
        piece += 1)
    {
      if(trailer_enabled && advance + piece->advance > (max_x - trailer_run.dim.x))
      {
        trailer_found = 1;
        break;
      }
      if(!trailer_enabled && advance + piece->advance > max_x)
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
        dr_img(dst, src, texture, fr->color, 0, 0, 0);
        // dr_rect(dst, v4f32(0, 1, 0, 0.5f), 0, 1.f, 0.f);
      }
      advance += piece->advance;
      pixel_range.min = Min(pre_advance, pixel_range.min);
      pixel_range.max = Max(advance, pixel_range.max);
    }
    if(fr->underline_thickness > 0)
    {
      dr_rect(r2f32p(p.x + pixel_range.min,
                     p.y+fr->run.descent+fr->run.descent/8,
                     p.x + pixel_range.max,
                     p.y+fr->run.descent+fr->run.descent/8+fr->underline_thickness),
              fr->color, 0, 0, 0.8f);
    }
    if(fr->strikethrough_thickness > 0)
    {
      dr_rect(r2f32p(p.x+pre_advance, p.y+fr->run.descent - fr->run.ascent/2, p.x+advance, p.y+fr->run.descent - fr->run.ascent/2 + fr->strikethrough_thickness), fr->color, 0, 0, 1.f);
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
    FNT_Piece *piece_first = trailer_run.pieces.v;
    FNT_Piece *piece_opl = piece_first + trailer_run.pieces.count;
    F32 pre_advance = advance;
    Vec4F32 trailer_piece_color = last_color;
    for(FNT_Piece *piece = piece_first;
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
        dr_img(dst, src, texture, trailer_piece_color, 0, 0, 0);
        trailer_piece_color.w *= 0.5f;
      }
      advance += piece->advance;
    }
  }
  
  ProfEnd();
}

internal void
dr_truncated_fancy_run_fuzzy_matches(Vec2F32 p, DR_FRunList *list, F32 max_x, FuzzyMatchRangeList *ranges, Vec4F32 color)
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
    for(DR_FRunNode *fr_n = list->first; fr_n != 0; fr_n = fr_n->next)
    {
      DR_FRun *fr = &fr_n->v;
      FNT_Run *run = &fr->run;
      ascent = run->ascent;
      descent = run->descent;
      for(U64 piece_idx = 0; piece_idx < run->pieces.count; piece_idx += 1)
      {
        FNT_Piece *piece = &run->pieces.v[piece_idx];
        if(contains_1u64(byte_range, byte_off))
        {
          F32 pre_advance  = advance + piece->offset.x;
          F32 post_advance = advance + piece->advance;
          pixel_range.min = Min(pre_advance,  pixel_range.min);
          pixel_range.max = Max(post_advance, pixel_range.max);
        }
        if(!fr->icon)
        {
          byte_off += piece->decode_size;
        }
        advance += piece->advance;
      }
    }
    if(pixel_range.min < pixel_range.max)
    {
      Rng2F32 rect = r2f32p(p.x + pixel_range.min - ascent/4.f,
                            p.y - descent - ascent - ascent/8.f,
                            p.x + pixel_range.max + ascent/4.f,
                            p.y - descent - ascent + ascent/8.f + list->dim.y);
      rect.x0 = Min(rect.x0, p.x+max_x);
      rect.x1 = Min(rect.x1, p.x+max_x);
      dr_rect(rect, color, (descent+ascent)/4.f, 0, 1.f);
    }
  }
}

internal void
dr_text_run(Vec2F32 p, Vec4F32 color, FNT_Run run)
{
  ProfBeginFunction();
  F32 advance = 0;
  FNT_Piece *piece_first = run.pieces.v;
  FNT_Piece *piece_opl = piece_first + run.pieces.count;
  for(FNT_Piece *piece = piece_first;
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
      dr_img(dst, src, texture, color, 0, 0, 0);
    }
    advance += piece->advance;
  }
  ProfEnd();
}

internal void
dr_text(FNT_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, FNT_RasterFlags flags, Vec2F32 p, Vec4F32 color, String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  FNT_Run run = fnt_run_from_string(font, size, base_align_px, tab_size_px, flags, string);
  dr_text_run(p, color, run);
  scratch_end(scratch);
}
