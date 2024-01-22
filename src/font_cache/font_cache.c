// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Functions

#if !defined(BLAKE2_H)
#define HAVE_SSE2
#include "third_party/blake2/blake2.h"
#include "third_party/blake2/blake2b.c"
#endif

internal F_Hash
f_hash_from_string(String8 string)
{
  F_Hash result = {0};
  blake2b((U8 *)&result.u64[0], sizeof(result), string.str, string.size, 0, 0);
  return result;
}

internal U64
f_little_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal Vec2S32
f_vertex_from_corner(Corner corner)
{
  Vec2S32 result = {0};
  switch(corner)
  {
    default: break;
    case Corner_00:{result = v2s32(0, 0);}break;
    case Corner_01:{result = v2s32(0, 1);}break;
    case Corner_10:{result = v2s32(1, 0);}break;
    case Corner_11:{result = v2s32(1, 1);}break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Font Tags

internal F_Tag
f_tag_zero(void)
{
  F_Tag result = {0};
  return result;
}

internal B32
f_tag_match(F_Tag a, F_Tag b)
{
  return a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1];
}

internal FP_Handle
f_handle_from_tag(F_Tag tag)
{
  ProfBeginFunction();
  U64 slot_idx = tag.u64[1] % f_state->font_hash_table_size;
  F_FontHashNode *existing_node = 0;
  {
    for(F_FontHashNode *n = f_state->font_hash_table[slot_idx].first; n != 0 ; n = n->hash_next)
    {
      if(MemoryMatchStruct(&tag, &n->tag))
      {
        existing_node = n;
        break;
      }
    }
  }
  FP_Handle result = {0};
  if(existing_node != 0)
  {
    result = existing_node->handle;
  }
  ProfEnd();
  return result;
}

internal FP_Metrics
f_fp_metrics_from_tag(F_Tag tag)
{
  ProfBeginFunction();
  U64 slot_idx = tag.u64[1] % f_state->font_hash_table_size;
  F_FontHashNode *existing_node = 0;
  {
    for(F_FontHashNode *n = f_state->font_hash_table[slot_idx].first; n != 0 ; n = n->hash_next)
    {
      if(MemoryMatchStruct(&tag, &n->tag))
      {
        existing_node = n;
        break;
      }
    }
  }
  FP_Metrics result = {0};
  if(existing_node != 0)
  {
    result = existing_node->metrics;
  }
  ProfEnd();
  return result;
}

internal F_Tag
f_tag_from_path(String8 path)
{
  ProfBeginFunction();
  
  //- rjf: produce tag from hash of path
  F_Tag result = {0};
  {
    F_Hash hash = f_hash_from_string(path);
    MemoryCopy(&result, &hash, sizeof(result));
    result.u64[1] |= bit64;
  }
  
  //- rjf: tag -> slot index
  U64 slot_idx = result.u64[1] % f_state->font_hash_table_size;
  
  //- rjf: slot * tag -> existing node
  F_FontHashNode *existing_node = 0;
  {
    for(F_FontHashNode *n = f_state->font_hash_table[slot_idx].first; n != 0 ; n = n->hash_next)
    {
      if(MemoryMatchStruct(&result, &n->tag))
      {
        existing_node = n;
        break;
      }
    }
  }
  
  //- rjf: allocate & push new node if we don't have an existing one
  F_FontHashNode *new_node = 0;
  if(existing_node == 0)
  {
    F_FontHashSlot *slot = &f_state->font_hash_table[slot_idx];
    new_node = push_array(f_state->arena, F_FontHashNode, 1);
    new_node->tag = result;
    new_node->handle = fp_font_open(path);
    new_node->metrics = fp_metrics_from_font(new_node->handle);
    new_node->path = push_str8_copy(f_state->arena, path);
    SLLQueuePush_N(slot->first, slot->last, new_node, hash_next);
  }
  
  //- rjf: return
  ProfEnd();
  return result;
}

internal F_Tag
f_tag_from_static_data_string(String8 *data_ptr)
{
  ProfBeginFunction();
  
  //- rjf: produce tag hash of ptr
  F_Tag result = {0};
  {
    F_Hash hash = f_hash_from_string(str8((U8 *)&data_ptr, sizeof(String8 *)));
    MemoryCopy(&result, &hash, sizeof(result));
    result.u64[1] &= ~bit64;
  }
  
  //- rjf: tag -> slot index
  U64 slot_idx = result.u64[1] % f_state->font_hash_table_size;
  
  //- rjf: slot * tag -> existing node
  F_FontHashNode *existing_node = 0;
  {
    for(F_FontHashNode *n = f_state->font_hash_table[slot_idx].first; n != 0 ; n = n->hash_next)
    {
      if(MemoryMatchStruct(&result, &n->tag))
      {
        existing_node = n;
        break;
      }
    }
  }
  
  //- rjf: allocate & push new node if we don't have an existing one
  F_FontHashNode *new_node = 0;
  if(existing_node == 0)
  {
    F_FontHashSlot *slot = &f_state->font_hash_table[slot_idx];
    new_node = push_array(f_state->arena, F_FontHashNode, 1);
    new_node->tag = result;
    new_node->handle = fp_font_open_from_static_data_string(data_ptr);
    new_node->metrics = fp_metrics_from_font(new_node->handle);
    new_node->path = str8_lit("");
    SLLQueuePush_N(slot->first, slot->last, new_node, hash_next);
  }
  
  //- rjf: return
  ProfEnd();
  return result;
}

internal String8
f_path_from_tag(F_Tag tag)
{
  //- rjf: tag -> slot index
  U64 slot_idx = tag.u64[1] % f_state->font_hash_table_size;
  
  //- rjf: slot * tag -> existing node
  F_FontHashNode *existing_node = 0;
  {
    for(F_FontHashNode *n = f_state->font_hash_table[slot_idx].first; n != 0 ; n = n->hash_next)
    {
      if(MemoryMatchStruct(&tag, &n->tag))
      {
        existing_node = n;
        break;
      }
    }
  }
  
  //- rjf: existing node -> path
  String8 result = {0};
  if(existing_node != 0)
  {
    result = existing_node->path;
  }
  
  return result;
}

////////////////////////////////
//~ rjf: Atlas

internal Rng2S16
f_atlas_region_alloc(Arena *arena, F_Atlas *atlas, Vec2S16 needed_size)
{
  ProfBeginFunction();
  
  //- rjf: find node with best-fit size
  Vec2S16 region_p0 = {0};
  Vec2S16 region_sz = {0};
  Corner node_corner = Corner_Invalid;
  F_AtlasRegionNode *node = 0;
  {
    Vec2S16 n_supported_size = atlas->root_dim;
    for(F_AtlasRegionNode *n = atlas->root, *next = 0; n != 0; n = next, next = 0)
    {
      // rjf: we've traversed to a taken node.
      if(n->flags & F_AtlasRegionNodeFlag_Taken)
      {
        break;
      }
      
      // rjf: calculate if this node can be allocated (all children are non-allocated)
      B32 n_can_be_allocated = (n->num_allocated_descendants == 0);
      
      // rjf: fill size
      if(n_can_be_allocated)
      {
        region_sz = n_supported_size;
      }
      
      // rjf: calculate size of this node's children
      Vec2S16 child_size = v2s16(n_supported_size.x/2, n_supported_size.y/2);
      
      // rjf: find best next child
      F_AtlasRegionNode *best_child = 0;
      if(child_size.x >= needed_size.x && child_size.y >= needed_size.y)
      {
        for(Corner corner = (Corner)0; corner < Corner_COUNT; corner = (Corner)(corner+1))
        {
          if(n->children[corner] == 0)
          {
            n->children[corner] = push_array(arena, F_AtlasRegionNode, 1);
            n->children[corner]->parent = n;
            n->children[corner]->max_free_size[Corner_00] = 
              n->children[corner]->max_free_size[Corner_01] = 
              n->children[corner]->max_free_size[Corner_10] = 
              n->children[corner]->max_free_size[Corner_11] = v2s16(child_size.x/2, child_size.y/2);
          }
          if(n->max_free_size[corner].x >= needed_size.x && 
             n->max_free_size[corner].y >= needed_size.y)
          {
            best_child = n->children[corner];
            node_corner = corner;
            Vec2S32 side_vertex = f_vertex_from_corner(corner);
            region_p0.x += side_vertex.x*child_size.x;
            region_p0.y += side_vertex.y*child_size.y;
            break;
          }
        }
      }
      
      // rjf: resolve node to this node if it can be allocated and children
      // don't fit, or keep going to the next best child
      if(n_can_be_allocated && best_child == 0)
      {
        node = n;
      }
      else
      {
        next = best_child;
        n_supported_size = child_size;
      }
    }
  }
  
  //- rjf: we're taking the subtree rooted by `node`. mark up all parents
  if(node != 0 && node_corner != Corner_Invalid)
  {
    node->flags |= F_AtlasRegionNodeFlag_Taken;
    if(node->parent != 0)
    {
      MemoryZeroStruct(&node->parent->max_free_size[node_corner]);
    }
    for(F_AtlasRegionNode *p = node->parent; p != 0; p = p->parent)
    {
      p->num_allocated_descendants += 1;
      F_AtlasRegionNode *parent = p->parent;
      if(parent != 0)
      {
        Corner p_corner = (p == parent->children[Corner_00] ? Corner_00 :
                           p == parent->children[Corner_01] ? Corner_01 :
                           p == parent->children[Corner_10] ? Corner_10 :
                           p == parent->children[Corner_11] ? Corner_11 :
                           Corner_Invalid);
        if(p_corner == Corner_Invalid)
        {
          InvalidPath;
        }
        parent->max_free_size[p_corner].x = Max(Max(p->max_free_size[Corner_00].x,
                                                    p->max_free_size[Corner_01].x),
                                                Max(p->max_free_size[Corner_10].x,
                                                    p->max_free_size[Corner_11].x));
        parent->max_free_size[p_corner].y = Max(Max(p->max_free_size[Corner_00].y,
                                                    p->max_free_size[Corner_01].y),
                                                Max(p->max_free_size[Corner_10].y,
                                                    p->max_free_size[Corner_11].y));
      }
    }
  }
  
  //- rjf: fill rectangular region & return
  Rng2S16 result = {0};
  result.p0 = region_p0;
  result.p1 = add_2s16(region_p0, region_sz);
  ProfEnd();
  return result;
}

internal void
f_atlas_region_release(F_Atlas *atlas, Rng2S16 region)
{
  ProfBeginFunction();
  
  //- rjf: extract region size
  Vec2S16 region_size = v2s16(region.x1 - region.x0, region.y1 - region.y0);
  
  //- rjf: map region to associated node
  Vec2S16 calc_region_size = {0};
  F_AtlasRegionNode *node = 0;
  Corner node_corner = Corner_Invalid;
  {
    Vec2S16 n_p0 = v2s16(0, 0);
    Vec2S16 n_sz = atlas->root_dim;
    for(F_AtlasRegionNode *n = atlas->root, *next = 0; n != 0; n = next)
    {
      // rjf: is the region within this node's boundaries? (either this node, or a descendant)
      if(n_p0.x <= region.p0.x && region.p0.x < n_p0.x+n_sz.x &&
         n_p0.y <= region.p0.y && region.p0.y < n_p0.y+n_sz.y)
      {
        // rjf: check the region against this node
        if(region.p0.x == n_p0.x && region.p0.y == n_p0.y && 
           region_size.x == n_sz.x && region_size.y == n_sz.y)
        {
          node = n;
          calc_region_size = n_sz;
          break;
        }
        // rjf: check the region against children & iterate
        else
        {
          Vec2S16 r_midpoint = v2s16(region.p0.x + region_size.x/2,
                                     region.p0.y + region_size.y/2);
          Vec2S16 n_midpoint = v2s16(n_p0.x + n_sz.x/2,
                                     n_p0.y + n_sz.y/2);
          Corner next_corner = Corner_Invalid;
          if(r_midpoint.x <= n_midpoint.x && r_midpoint.y <= n_midpoint.y)
          {
            next_corner = Corner_00;
          }
          else if(r_midpoint.x <= n_midpoint.x && n_midpoint.y <= r_midpoint.y)
          {
            next_corner = Corner_01;
          }
          else if(n_midpoint.x <= r_midpoint.x && r_midpoint.y <= n_midpoint.y)
          {
            next_corner = Corner_10;
          }
          else if(n_midpoint.x <= r_midpoint.x && n_midpoint.y <= r_midpoint.y)
          {
            next_corner = Corner_11;
          }
          next = n->children[next_corner];
          node_corner = next_corner;
          n_sz.x /= 2;
          n_sz.y /= 2;
          Vec2S32 side_vertex = f_vertex_from_corner(node_corner);
          n_p0.x += side_vertex.x*n_sz.x;
          n_p0.y += side_vertex.y*n_sz.y;
        }
      }
      else
      {
        break; 
      }
    }
  }
  
  //- rjf: free node
  if(node != 0 && node_corner != Corner_Invalid)
  {
    node->flags &= ~F_AtlasRegionNodeFlag_Taken;
    if(node->parent != 0)
    {
      node->parent->max_free_size[node_corner] = calc_region_size;
    }
    for(F_AtlasRegionNode *p = node->parent; p != 0; p = p->parent)
    {
      p->num_allocated_descendants -= 1;
      F_AtlasRegionNode *parent = p->parent;
      if(parent != 0)
      {
        Corner p_corner = (p == parent->children[Corner_00] ? Corner_00 :
                           p == parent->children[Corner_01] ? Corner_01 :
                           p == parent->children[Corner_10] ? Corner_10 :
                           p == parent->children[Corner_11] ? Corner_11 :
                           Corner_Invalid);
        if(p_corner == Corner_Invalid)
        {
          InvalidPath;
        }
        parent->max_free_size[p_corner].x = Max(Max(p->max_free_size[Corner_00].x,
                                                    p->max_free_size[Corner_01].x),
                                                Max(p->max_free_size[Corner_10].x,
                                                    p->max_free_size[Corner_11].x));
        parent->max_free_size[p_corner].y = Max(Max(p->max_free_size[Corner_00].y,
                                                    p->max_free_size[Corner_01].y),
                                                Max(p->max_free_size[Corner_10].y,
                                                    p->max_free_size[Corner_11].y));
      }
    }
  }
  ProfEnd();
}

////////////////////////////////
//~ rjf: Piece Type Functions

internal F_Piece *
f_piece_chunk_list_push_new(Arena *arena, F_PieceChunkList *list, U64 cap)
{
  F_PieceChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, F_PieceChunkNode, 1);
    node->v = push_array_no_zero(arena, F_Piece, cap);
    node->cap = cap;
    SLLQueuePush(list->first, list->last, node);
    list->node_count += 1;
  }
  F_Piece *result = node->v + node->count;
  node->count += 1;
  list->total_piece_count += 1;
  return result;
}

internal void
f_piece_chunk_list_push(Arena *arena, F_PieceChunkList *list, U64 cap, F_Piece *piece)
{
  F_Piece *new_piece = f_piece_chunk_list_push_new(arena, list, cap);
  MemoryCopyStruct(new_piece, piece);
}

internal F_PieceArray
f_piece_array_from_chunk_list(Arena *arena, F_PieceChunkList *list)
{
  F_PieceArray array = {0};
  array.count = list->total_piece_count;
  array.v = push_array_no_zero(arena, F_Piece, array.count);
  U64 write_idx = 0;
  for(F_PieceChunkNode *node = list->first; node != 0; node = node->next)
  {
    MemoryCopy(array.v + write_idx, node->v, node->count * sizeof(F_Piece));
    write_idx += node->count;
  }
  return array;
}

////////////////////////////////
//~ rjf: Rasterization Cache

internal F_Hash2StyleRasterCacheNode *
f_hash2style_from_tag_size(F_Tag tag, F32 size)
{
  //- rjf: tag * size -> style hash
  U64 style_hash = {0};
  {
    U64 buffer[] =
    {
      tag.u64[0],
      tag.u64[1],
      (U64)round_f32(size),
    };
    style_hash = f_little_hash_from_string(str8((U8 *)buffer, sizeof(buffer)));
  }
  
  //- rjf: style hash -> style node
  F_Hash2StyleRasterCacheNode *hash2style_node = 0;
  {
    ProfBegin("style hash -> style node");
    U64 slot_idx = style_hash%f_state->hash2style_slots_count;
    F_Hash2StyleRasterCacheSlot *slot = &f_state->hash2style_slots[slot_idx];
    for(F_Hash2StyleRasterCacheNode *n = slot->first;
        n != 0;
        n = n->hash_next)
    {
      if(n->style_hash == style_hash)
      {
        hash2style_node = n;
        break;
      }
    }
    if(Unlikely(hash2style_node == 0))
    {
      F_Metrics metrics = f_metrics_from_tag_size(tag, size);
      hash2style_node = push_array(f_state->arena, F_Hash2StyleRasterCacheNode, 1);
      DLLPushBack_NP(slot->first, slot->last, hash2style_node, hash_next, hash_prev);
      hash2style_node->style_hash = style_hash;
      hash2style_node->ascent = metrics.ascent;
      hash2style_node->descent= metrics.descent;
      hash2style_node->utf8_class1_direct_map = push_array_no_zero(f_state->arena, F_RasterCacheInfo, 256);
      hash2style_node->hash2info_slots_count = 1024;
      hash2style_node->hash2info_slots = push_array(f_state->arena, F_Hash2InfoRasterCacheSlot, hash2style_node->hash2info_slots_count);
    }
    ProfEnd();
  }
  
  return hash2style_node;
}

internal F_Run
f_push_run_from_string(Arena *arena, F_Tag tag, F32 size, F_RunFlags flags, String8 string)
{
  ProfBeginFunction();
  
  //- rjf: map tag/size to style node
  F_Hash2StyleRasterCacheNode *hash2style_node = f_hash2style_from_tag_size(tag, size);
  
  //- rjf: decode string & produce run pieces
  F_PieceChunkList piece_chunks = {0};
  Vec2F32 dim = {0};
  B32 font_handle_mapped_on_miss = 0;
  FP_Handle font_handle = {0};
  U64 piece_substring_start_idx = 0;
  for(U64 idx = 0; idx < string.size;)
  {
    //- rjf: decode next codepoint & get piece substring, or continuation rule
    String8 piece_substring;
    B32 need_another_codepoint = 0;
    switch(utf8_class[string.str[idx]>>3])
    {
      case 1:
      {
        piece_substring.str = &string.str[idx];
        piece_substring.size = 1;
        idx += 1;
      }break;
      default:
      {
        UnicodeDecode decode = utf8_decode(string.str+idx, string.size-idx);
        idx += decode.inc;
        if(decode.inc == 0) { break; }
        piece_substring.str = string.str + piece_substring_start_idx;
        piece_substring.size = decode.inc;
        // NOTE(rjf): assuming 1 codepoint per piece for now.
      }break;
    }
    
    //- rjf: need another codepoint? -> continue
    if(need_another_codepoint)
    {
      continue;
    }
    
    //- rjf: do not need another codepoint? -> bump piece start idx
    {
      piece_substring_start_idx = idx;
    }
    
    //- rjf: determine if this piece is a tab - if so, use space info to draw
    B32 is_tab = (piece_substring.size == 1 && piece_substring.str[0] == '\t');
    if(is_tab)
    {
      piece_substring = str8_lit(" ");
    }
    
    //- rjf: piece substring -> raster cache info
    F_RasterCacheInfo *info = 0;
    U64 piece_hash = 0;
    {
      // rjf: fast path for utf8 class 1 -> direct map
      if(piece_substring.size == 1 && hash2style_node->utf8_class1_direct_map_mask[piece_substring.str[0]/64] & (1ull<<(piece_substring.str[0]%64)))
      {
        info = &hash2style_node->utf8_class1_direct_map[piece_substring.str[0]];
      }
      
      // rjf: more general, slower path for other glyphs
      if(piece_substring.size > 1)
      {
        piece_hash = f_little_hash_from_string(piece_substring);
        U64 slot_idx = piece_hash%hash2style_node->hash2info_slots_count;
        F_Hash2InfoRasterCacheSlot *slot = &hash2style_node->hash2info_slots[slot_idx];
        for(F_Hash2InfoRasterCacheNode *node = slot->first; node != 0; node = node->hash_next)
        {
          if(node->hash == piece_hash)
          {
            info = &node->info;
            break;
          }
        }
      }
    }
    
    //- rjf: no info found -> miss... fill this hash in the cache
    if(info == 0)
    {
      ProfBegin("no info found -> miss... fill this hash in the cache");
      Temp scratch = scratch_begin(&arena, 1);
      
      // rjf: grab font handle for this tag if we don't have one already
      if(font_handle_mapped_on_miss == 0)
      {
        font_handle_mapped_on_miss = 1;
        
        // rjf: tag -> font slot index
        U64 font_slot_idx = tag.u64[1] % f_state->font_hash_table_size;
        
        // rjf: tag * slot -> existing node
        F_FontHashNode *existing_node = 0;
        {
          for(F_FontHashNode *n = f_state->font_hash_table[font_slot_idx].first; n != 0 ; n = n->hash_next)
          {
            if(MemoryMatchStruct(&n->tag, &tag))
            {
              existing_node = n;
              break;
            }
          }
        }
        
        // rjf: existing node -> font handle
        if(existing_node != 0)
        {
          font_handle = existing_node->handle;
        }
      }
      
      // rjf: call into font provider to rasterize this substring
      FP_RasterResult raster = fp_raster(scratch.arena, font_handle, round_f32(size), FP_RasterMode_Sharp, piece_substring);
      
      // rjf: allocate portion of an atlas to upload the rasterization
      S16 chosen_atlas_num = 0;
      F_Atlas *chosen_atlas = 0;
      Rng2S16 chosen_atlas_region = {0};
      if(raster.atlas_dim.x != 0 && raster.atlas_dim.y != 0)
      {
        U64 num_atlases = 0;
        for(F_Atlas *atlas = f_state->first_atlas;; atlas = atlas->next, num_atlases += 1)
        {
          // rjf: create atlas if needed
          if(atlas == 0 && num_atlases < 16)
          {
            atlas = push_array(f_state->arena, F_Atlas, 1);
            DLLPushBack(f_state->first_atlas, f_state->last_atlas, atlas);
            atlas->root_dim = v2s16(1024, 1024);
            atlas->root = push_array(f_state->arena, F_AtlasRegionNode, 1);
            atlas->root->max_free_size[Corner_00] =
              atlas->root->max_free_size[Corner_01] =
              atlas->root->max_free_size[Corner_10] =
              atlas->root->max_free_size[Corner_11] = v2s16(atlas->root_dim.x/2, atlas->root_dim.y/2);
            atlas->texture = r_tex2d_alloc(R_Tex2DKind_Dynamic, v2s32((S32)atlas->root_dim.x, (S32)atlas->root_dim.y), R_Tex2DFormat_RGBA8, 0);
          }
          
          // rjf: allocate from atlas
          if(atlas != 0)
          {
            Vec2S16 needed_dimensions = v2s16(raster.atlas_dim.x + 2, raster.atlas_dim.y + 2);
            chosen_atlas_region = f_atlas_region_alloc(f_state->arena, atlas, needed_dimensions);
            if(chosen_atlas_region.x1 != chosen_atlas_region.x0)
            {
              chosen_atlas = atlas;
              chosen_atlas_num = (S32)num_atlases;
              break;
            }
          }
          else
          {
            break;
          }
        }
      }
      
      // rjf: upload rasterization to allocated region of atlas texture memory
      if(chosen_atlas != 0)
      {
        Rng2S32 subregion =
        {
          chosen_atlas_region.x0 + 1,
          chosen_atlas_region.y0 + 1,
          chosen_atlas_region.x0 + raster.atlas_dim.x + 1,
          chosen_atlas_region.y0 + raster.atlas_dim.y + 1
        };
        r_fill_tex2d_region(chosen_atlas->texture, subregion, raster.atlas);
      }
      
      // rjf: allocate & fill & push node
      {
        if(piece_substring.size == 1)
        {
          info = &hash2style_node->utf8_class1_direct_map[piece_substring.str[0]];
          hash2style_node->utf8_class1_direct_map_mask[piece_substring.str[0]/64] |= (1ull<<(piece_substring.str[0]%64));
        }
        else
        {
          U64 slot_idx = piece_hash%hash2style_node->hash2info_slots_count;
          F_Hash2InfoRasterCacheSlot *slot = &hash2style_node->hash2info_slots[slot_idx];
          F_Hash2InfoRasterCacheNode *node = push_array_no_zero(f_state->arena, F_Hash2InfoRasterCacheNode, 1);
          DLLPushBack_NP(slot->first, slot->last, node, hash_next, hash_prev);
          node->hash = piece_hash;
          info = &node->info;
        }
        if(info != 0)
        {
          info->subrect = chosen_atlas_region;
          info->atlas_num = chosen_atlas_num;
          info->raster_dim = raster.atlas_dim;
          info->advance = raster.advance;
        }
      }
      
      scratch_end(scratch);
      ProfEnd();
    }
    
    //- rjf: push piece for this raster portion
    if(info != 0)
    {
      // rjf: find atlas
      F_Atlas *atlas = 0;
      {
        if(info->subrect.x1 != 0 && info->subrect.y1 != 0)
        {
          S32 num = 0;
          for(F_Atlas *a = f_state->first_atlas; a != 0; a = a->next, num += 1)
          {
            if(info->atlas_num == num)
            {
              atlas = a;
              break;
            }
          }
        }
      }
      
      // rjf: push piece
      {
        F_Piece *piece = f_piece_chunk_list_push_new(arena, &piece_chunks, string.size);
        {
          piece->texture = atlas ? atlas->texture : r_handle_zero();
          piece->subrect = r2s16p(info->subrect.x0,
                                  info->subrect.y0,
                                  info->subrect.x0 + info->raster_dim.x,
                                  info->subrect.y0 + info->raster_dim.y);
          piece->advance = info->advance;
          piece->decode_size = piece_substring.size;
          piece->offset = v2s16(0, -hash2style_node->ascent - 4);
        }
        dim.x += piece->advance;
        dim.y = Max(dim.y, dim_2s16(piece->subrect).y);
      }
    }
  }
  
  //- rjf: tighten & return
  F_Run run = {0};
  {
    if(piece_chunks.node_count == 1)
    {
      run.pieces.v = piece_chunks.first->v;
      run.pieces.count = piece_chunks.first->count;
    }
    else
    {
      run.pieces = f_piece_array_from_chunk_list(arena, &piece_chunks);
    }
    run.dim = dim;
    run.ascent  = hash2style_node->ascent;
    run.descent = hash2style_node->descent;
  }
  
  ProfEnd();
  return run;
}

internal String8List
f_wrapped_string_lines_from_font_size_string_max(Arena *arena, F_Tag font, F32 size, String8 string, F32 max)
{
  String8List list = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    F_Run run = f_push_run_from_string(scratch.arena, font, size, 0, string);
    F32 off_px = 0;
    U64 off_bytes = 0;
    U64 line_start_off_bytes = 0;
    U64 line_end_off_bytes = 0;
    B32 seeking_word_end = 0;
    F32 word_start_off_px = 0;
    F_Piece *last_word_start_piece = 0;
    U64 last_word_start_off_bytes = 0;
    F_Piece *pieces_first = run.pieces.v;
    F_Piece *pieces_opl = run.pieces.v + run.pieces.count;
    for(F_Piece *piece = pieces_first, *next = 0; piece != 0 && piece <= pieces_opl; piece = next)
    {
      if(piece != 0) {next = piece+1;}
      
      // rjf: gather info
      U8 byte         = off_bytes < string.size ? string.str[off_bytes] : 0;
      F32 advance     = (piece != 0) ? piece->advance : 0;
      U64 decode_size = (piece != 0) ? piece->decode_size : 0;
      
      // rjf: find start/end of words
      B32 is_first_byte_of_word = 0;
      B32 is_first_space_after_word = 0;
      if(!seeking_word_end && !char_is_space(byte))
      {
        seeking_word_end = 1;
        is_first_byte_of_word = 1;
        last_word_start_off_bytes = off_bytes;
        last_word_start_piece = piece;
        word_start_off_px = off_px;
      }
      else if(seeking_word_end && char_is_space(byte))
      {
        seeking_word_end = 0;
        is_first_space_after_word = 1;
      }
      else if(seeking_word_end && byte == 0)
      {
        is_first_space_after_word = 1;
      }
      
      // rjf: determine properties of this advance
      B32 is_illegal = (off_px >= max);
      B32 is_next_illegal = (off_px + advance >= max);
      B32 is_end = (byte == 0);
      
      // rjf: legal word end -> extend line
      if(is_first_space_after_word && !is_illegal)
      {
        line_end_off_bytes = off_bytes;
      }
      
      // rjf: illegal mid-word split -> wrap mid-word
      if(is_next_illegal && word_start_off_px == 0)
      {
        String8 line = str8(string.str + line_start_off_bytes, off_bytes - line_start_off_bytes);
        line = str8_skip_chop_whitespace(line);
        if(line.size != 0)
        {
          str8_list_push(arena, &list, line);
        }
        off_px = advance;
        line_start_off_bytes = off_bytes;
        line_end_off_bytes = off_bytes;
        word_start_off_px = 0;
        last_word_start_piece = piece;
        last_word_start_off_bytes = off_bytes;
        off_bytes += decode_size;
      }
      
      // rjf: illegal word end -> wrap line
      else if(is_first_space_after_word && (is_illegal || is_end))
      {
        String8 line = str8(string.str + line_start_off_bytes, line_end_off_bytes - line_start_off_bytes);
        line = str8_skip_chop_whitespace(line);
        if(line.size != 0)
        {
          str8_list_push(arena, &list, line);
        }
        line_start_off_bytes = line_end_off_bytes;
        if(is_illegal)
        {
          off_px = 0;
          word_start_off_px = 0;
          off_bytes = last_word_start_off_bytes;
          next = last_word_start_piece;
        }
      }
      
      // rjf: advance offsets otherwise
      else
      {
        off_px += advance;
        off_bytes += decode_size;
      }
      
      // rjf: 0 piece and 0 next -> done
      if(piece == 0 && next == 0)
      {
        break;
      }
    }
    scratch_end(scratch);
  }
  return list;
}

internal Vec2F32
f_dim_from_tag_size_string(F_Tag tag, F32 size, String8 string)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  Vec2F32 result = {0};
  F_Run run = f_push_run_from_string(scratch.arena, tag, size, 0, string);
  result = run.dim;
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal Vec2F32
f_dim_from_tag_size_string_list(F_Tag tag, F32 size, String8List list)
{
  ProfBeginFunction();
  Vec2F32 sum = {0};
  for(String8Node *n = list.first; n != 0; n = n->next)
  {
    Vec2F32 str_dim = f_dim_from_tag_size_string(tag, size, n->string);
    sum.x += str_dim.x;
    sum.y = Max(sum.y, str_dim.y);
  }
  ProfEnd();
  return sum;
}

internal U64
f_char_pos_from_tag_size_string_p(F_Tag tag, F32 size, String8 string, F32 p)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  U64 result = 0;
  U64 best_offset = 0;
  F32 best_distance = -1.f;
  F32 x = 0;
  for(U64 char_idx = 0; char_idx <= string.size; char_idx += 1)
  {
    F32 this_char_distance = fabsf(p - x);
    if(this_char_distance < best_distance || best_distance < 0.f)
    {
      best_offset = char_idx;
      best_distance = this_char_distance;
    }
    if(char_idx < string.size)
    {
      x += f_dim_from_tag_size_string(tag, size, str8_substr(string, r1u64(char_idx, char_idx+1))).x;
    }
  }
  result = best_offset;
  scratch_end(scratch);
  ProfEnd();
  return result;
}

////////////////////////////////
//~ rjf: Metrics

internal F_Metrics
f_metrics_from_tag_size(F_Tag tag, F32 size)
{
  ProfBeginFunction();
  FP_Metrics metrics = f_fp_metrics_from_tag(tag);
  F_Metrics result = {0};
  {
    result.ascent   = size * metrics.ascent / metrics.design_units_per_em;
    result.descent  = size * metrics.descent / metrics.design_units_per_em;
    result.line_gap = size * metrics.line_gap / metrics.design_units_per_em;
    result.capital_height = size * metrics.capital_height / metrics.design_units_per_em;
  }
  ProfEnd();
  return result;
}

internal F32
f_line_height_from_metrics(F_Metrics *metrics)
{
  return metrics->ascent + metrics->descent + metrics->line_gap;
}

////////////////////////////////
//~ rjf: Main Calls

internal void
f_init(void)
{
  Arena *arena = arena_alloc();
  f_state = push_array(arena, F_State, 1);
  f_state->arena = arena;
  f_state->font_hash_table_size = 64;
  f_state->font_hash_table = push_array(arena, F_FontHashSlot, f_state->font_hash_table_size);
  f_state->hash2style_slots_count = 1024;
  f_state->hash2style_slots = push_array(arena, F_Hash2StyleRasterCacheSlot, f_state->hash2style_slots_count);
}
