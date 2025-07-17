// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal FP_FT_Font
fp_ft_font_from_handle(FP_Handle handle)
{
  FP_FT_Font result = {(FT_Face)handle.u64[0]};
  return result;
}

internal FP_Handle
fp_ft_handle_from_font(FP_FT_Font font)
{
  FP_Handle result = {(U64)font.face};
  return result;
}

////////////////////////////////
//~ rjf: Backend Implementations

fp_hook void
fp_init(void)
{
  Arena *arena = arena_alloc();
  fp_ft_state = push_array(arena, FP_FT_State, 1);
  fp_ft_state->arena = arena;
  FT_Init_FreeType(&fp_ft_state->library);
}

fp_hook FP_Handle
fp_font_open(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String8 path_copy = push_str8_copy(scratch.arena, path);
  FP_FT_Font font = {0};
  FT_New_Face(fp_ft_state->library, (char *)path_copy.str, 0, &font.face);
  FP_Handle handle = fp_ft_handle_from_font(font);
  scratch_end(scratch);
  return handle;
}

fp_hook FP_Handle
fp_font_open_from_static_data_string(String8 *data_ptr)
{
  FP_FT_Font font = {0};
  FT_New_Memory_Face(fp_ft_state->library, data_ptr->str, (FT_Long)data_ptr->size, 0, &font.face);
  FP_Handle handle = fp_ft_handle_from_font(font);
  return handle;
}

fp_hook void
fp_font_close(FP_Handle handle)
{
  FP_FT_Font font = fp_ft_font_from_handle(handle);
  if(font.face != 0)
  {
    FT_Done_Face(font.face);
  }
}

fp_hook FP_Metrics
fp_metrics_from_font(FP_Handle handle)
{
  FP_FT_Font font = fp_ft_font_from_handle(handle);
  FP_Metrics result = {0};
  if(font.face != 0)
  {
    result.design_units_per_em = (F32)(font.face->units_per_EM);
    result.ascent              = (F32)font.face->ascender;
    result.descent             = -(F32)font.face->descender;
    result.line_gap            = (F32)(font.face->height - font.face->ascender + font.face->descender);
    result.capital_height      = (F32)(font.face->ascender);
  }
  return result;
}

fp_hook FP_RasterResult
fp_raster(Arena *arena, FP_Handle handle, F32 size, FP_RasterFlags flags, String8 string)
{
  ProfBeginFunction();
  FP_FT_Font font = fp_ft_font_from_handle(handle);
  FP_RasterResult result = {0};
  if(font.face != 0)
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: unpack font
    FT_Face face = font.face;
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)((96.f/72.f) * size));
    S64 ascent  = face->size->metrics.ascender >> 6;
    S64 descent = abs_s64(face->size->metrics.descender >> 6);
    S64 height  = face->size->metrics.height >> 6;
    
    //- rjf: unpack string
    String32 string32 = str32_from_8(scratch.arena, string);
    
    //- rjf: measure
    S32 total_width = 0;
    for EachIndex(idx, string32.size)
    {
      FT_Load_Char(face, string32.str[idx], FT_LOAD_RENDER);
      total_width += (face->glyph->advance.x >> 6);
    }
    
    //- rjf: allocate & fill atlas w/ rasterization
    Vec2S16 dim = {(S16)total_width+1, height+1};
    U64 atlas_size = dim.x * dim.y * 4;
    U8 *atlas = push_array(arena, U8, atlas_size);
    S32 baseline = ascent;
    S32 atlas_write_x = 0;
    for EachIndex(idx, string32.size)
    {
      FT_Load_Char(face, string32.str[idx], FT_LOAD_RENDER);
      FT_Bitmap *bmp = &face->glyph->bitmap;
      S32 top = face->glyph->bitmap_top;
      S32 left = face->glyph->bitmap_left;
      for(S32 row = 0; row < (S32)bmp->rows; row += 1)
      {
        S32 y = baseline - top + row;
        for(S32 col = 0; col < (S32)bmp->width; col += 1)
        {
          S32 x = atlas_write_x + left + col;
          U64 off = (y*dim.x + x)*4;
          if(off+4 <= atlas_size)
          {
            atlas[off+0] = 255;
            atlas[off+1] = 255;
            atlas[off+2] = 255;
            atlas[off+3] = bmp->buffer[row*bmp->pitch + col];
          }
        }
      }
      atlas_write_x += (face->glyph->advance.x >> 6);
    }
    
    //- rjf: fill result
    result.atlas_dim = dim;
    result.advance   = (F32)total_width;
    result.atlas     = atlas;
    scratch_end(scratch);
  }
  ProfEnd();
  return result;
}
