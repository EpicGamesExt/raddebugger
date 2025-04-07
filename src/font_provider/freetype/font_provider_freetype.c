// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ dan: Backend Implementations

fp_hook void
fp_init(void)
{
  ProfBeginFunction();

  //- dan: initialize main state
  {
    Arena *arena = arena_alloc();
    fp_freetype_state = push_array(arena, FP_Freetype_State, 1);
    fp_freetype_state->arena = arena;
  }

  //- dan: initialize freetype library
  FT_Error error = FT_Init_FreeType(&fp_freetype_state->library);
  if(error)
  {
    log_infof("fp_init: FT_Init_FreeType failed with error %d", error);
  }
  // Enable LCD filtering for potentially better subpixel rendering
  FT_Library_SetLcdFilter(fp_freetype_state->library, FT_LCD_FILTER_DEFAULT);

  ProfEnd();
}

fp_hook FP_Handle
fp_font_open(String8 path)
{
  ProfBeginFunction();
  FP_Handle handle = {0};
  FT_Face face;
  Temp scratch = scratch_begin(0, 0);

  // Freetype needs a null-terminated path
  String8 path_copy = push_str8_copy(scratch.arena, path);

  FT_Error error = FT_New_Face(fp_freetype_state->library,
                               (const char *)path_copy.str,
                               0, // Use first face index
                               &face);

  if(!error)
  {
    handle.u64[0] = (U64)face;
  }
  else
  {
    log_infof("fp_font_open: FT_New_Face failed for path '%S' with error %d", path_copy, error);
  }

  scratch_end(scratch);
  ProfEnd();
  return handle;
}

fp_hook FP_Handle
fp_font_open_from_static_data_string(String8 *data_ptr)
{
  ProfBeginFunction();
  FP_Handle handle = {0};
  FT_Face face = 0; // Initialize face to 0

  if(data_ptr != 0 && data_ptr->str != 0 && data_ptr->size > 0)
  {
    // FT_New_Memory_Face requires a pointer to the data and its size.
    FT_Error error = FT_New_Memory_Face(fp_freetype_state->library,
                                        (const FT_Byte *)data_ptr->str,
                                        (FT_Long)data_ptr->size,
                                        0, // Use first face index
                                        &face);

    if(!error)
    {
      handle.u64[0] = (U64)face;
    }
    else
    {
      log_infof("fp_font_open_from_static_data_string: FT_New_Memory_Face failed with error %d", error);
      face = 0; // Ensure face is 0 on error
    }
  }
  // If data_ptr is null or invalid, handle remains zero.

  ProfEnd();
  return handle;
}

fp_hook void
fp_font_close(FP_Handle handle)
{
  ProfBeginFunction();
  FT_Face face = (FT_Face)handle.u64[0];
  if(face != 0)
  {
    FT_Error error = FT_Done_Face(face);
    if(error)
    {
      log_infof("fp_font_close: FT_Done_Face failed with error %d", error);
    }
  }
  ProfEnd();
}

fp_hook FP_Metrics
fp_metrics_from_font(FP_Handle handle)
{
  ProfBeginFunction();
  FP_Metrics result = {0};
  FT_Face face = (FT_Face)handle.u64[0];

  if(face != 0)
  {
    result.design_units_per_em = (F32)face->units_per_EM;
    result.ascent              = (F32)face->ascender;
    // FreeType descender is typically negative, FP_Metrics likely expects positive.
    result.descent             = (F32)face->descender;
    if (result.descent < 0)
    {
        result.descent = -result.descent;
    }
    // face->height is the distance between baselines. line_gap = height - (ascender - descender)
    result.line_gap            = (F32)(face->height - (face->ascender - face->descender));

    // Capital height retrieval from OS/2 table
    result.capital_height      = 0; // Default to 0
    FT_Byte* os2_table = (FT_Byte*)FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    if (os2_table)
    {
        // OS/2 table version determines where sCapHeight is located.
        // Read version (ushort at offset 0)
        // We need fields up to sCapHeight (offset 88 in version >= 2)
        // Check table size first. Minimum size for version 0/1 is 78 bytes.
        // For version >= 2, minimum size is 96 bytes.
        // FT_ULong length = 0; // FT_Get_Sfnt_Table does not provide length directly
        // Need another way to check table size or rely on FreeType's internal handling.
        // Let's assume FreeType provides a valid pointer if the table exists.
        
        FT_UShort version = (FT_UShort)((os2_table[0] << 8) | os2_table[1]);
        
        // sCapHeight is at offset 88 (bytes) in OS/2 version 2 and later.
        // It's optional and might be 0 even if present.
        // Check if version >= 2 (assuming table is large enough)
        // A more robust check would involve FT_Load_Sfnt_Table if length was needed.
        if (version >= 2)
        {
            // Read sCapHeight (short at offset 88)
            FT_Short cap_height_units = (FT_Short)((os2_table[88] << 8) | os2_table[89]);
            if (cap_height_units > 0) // Use only if positive
            {
                 result.capital_height = (F32)cap_height_units;
            }
        }
        
        // If cap height is still 0 (not found or invalid), use ascender as fallback
        if (result.capital_height == 0)
        {
            result.capital_height = (F32)face->ascender;
        }
    }
    else
    {
        // Fallback if OS/2 table doesn't exist: Use ascender
        result.capital_height = (F32)face->ascender;
    }
  }

  ProfEnd();
  return result;
}

fp_hook NO_ASAN FP_RasterResult
fp_raster(Arena *arena, FP_Handle font_handle, F32 size, FP_RasterFlags flags, String8 string)
{
  ProfBeginFunction();
  FP_RasterResult result = {0};
  FT_Face face = (FT_Face)font_handle.u64[0];
  if (face == 0)
  {
    ProfEnd();
    return result;
  }

  Temp scratch = scratch_begin(&arena, 1);
  String32 string32 = str32_from_8(scratch.arena, string);
  if (string32.size == 0)
  {
    scratch_end(scratch);
    ProfEnd();
    return result;
  }

  //- dan: Define glyph render info storage
  typedef struct GlyphRenderInfo GlyphRenderInfo;
  struct GlyphRenderInfo {
    FT_UInt glyph_index;
    S32     render_pen_x; // Pen position before this glyph (26.6 fixed point)
    S32     advance_x;    // Advance width for this glyph (26.6 fixed point)
  };

  //- dan: Set pixel size
  FT_UInt pixel_height = (FT_UInt)(size + 0.5f);
  FT_Error error = FT_Set_Pixel_Sizes(face, 0, pixel_height);
  if (error)
  {
    log_infof("fp_raster: FT_Set_Pixel_Sizes failed for size %.2f with error %d", size, error);
    scratch_end(scratch);
    ProfEnd();
    return result;
  }

  //- dan: Determine FreeType load flags based on FP_RasterFlags
  FT_Int32 load_flags = FT_LOAD_DEFAULT | FT_LOAD_COLOR;
  FT_Render_Mode render_mode = FT_RENDER_MODE_NORMAL;
  // Determine the appropriate kerning mode based on hinting
  FT_Kerning_Mode kerning_mode = FT_KERNING_DEFAULT; // Default (scaled, hinted)

  switch (flags)
  {
    case 0: // Sharp, Unhinted
      load_flags |= FT_LOAD_NO_HINTING | FT_LOAD_TARGET_NORMAL;
      render_mode = FT_RENDER_MODE_NORMAL;
      kerning_mode = FT_KERNING_UNFITTED; // Unhinted kerning
      break;

    case FP_RasterFlag_Hinted: // Sharp, Hinted
      load_flags |= FT_LOAD_TARGET_NORMAL; // Hinting enabled
      render_mode = FT_RENDER_MODE_NORMAL;
      kerning_mode = FT_KERNING_DEFAULT; // Hinted kerning
      break;

    case FP_RasterFlag_Smooth: // Smooth, Unhinted
      load_flags |= FT_LOAD_NO_HINTING | FT_LOAD_TARGET_LCD;
      render_mode = FT_RENDER_MODE_LCD;
      kerning_mode = FT_KERNING_UNFITTED; // Unhinted kerning
      // NOTE: Current implementation converts LCD to grayscale alpha.
      break;

    case FP_RasterFlag_Smooth | FP_RasterFlag_Hinted: // Smooth, Hinted
      load_flags |= FT_LOAD_TARGET_LCD; // Hinting enabled
      render_mode = FT_RENDER_MODE_LCD;
      kerning_mode = FT_KERNING_DEFAULT; // Hinted kerning
      // NOTE: Current implementation converts LCD to grayscale alpha.
      break;

    default: // Fallback to default (Sharp, Hinted)
      load_flags |= FT_LOAD_TARGET_NORMAL;
      render_mode = FT_RENDER_MODE_NORMAL;
      kerning_mode = FT_KERNING_DEFAULT;
      break;
  }

  //- dan: First pass: Calculate layout metrics and store glyph positions
  GlyphRenderInfo *glyph_infos = push_array(scratch.arena, GlyphRenderInfo, string32.size);
  MemoryZero(glyph_infos, sizeof(GlyphRenderInfo) * string32.size);

  S32 total_width_fixed = 0; // Use 26.6 for total width calculation
  S32 max_ascent = 0;
  S32 max_descent = 0;
  S32 pen_x = 0; // Use 26.6 fixed-point
  FT_UInt prev_glyph_index = 0;

  for (U64 i = 0; i < string32.size; ++i)
  {
    FT_UInt glyph_index = FT_Get_Char_Index(face, string32.str[i]);
    glyph_infos[i].glyph_index = glyph_index;

    // Load glyph metrics *using the final load flags*
    // FT_LOAD_NO_BITMAP could optimize this, but FT_LOAD_DEFAULT is okay.
    error = FT_Load_Glyph(face, glyph_index, load_flags); // <<< USE CONSISTENT load_flags
    if (error) {
      log_infof("fp_raster: FT_Load_Glyph (metrics pass) failed for glyph index %u (char 0x%x) with error %d", glyph_index, string32.str[i], error);
      glyph_infos[i].render_pen_x = pen_x;
      glyph_infos[i].advance_x = 0;
      prev_glyph_index = glyph_index;
      continue;
    }

    FT_GlyphSlot slot = face->glyph;

    // Apply kerning *using the consistent kerning mode*
    if (prev_glyph_index && glyph_index && FT_HAS_KERNING(face))
    {
      FT_Vector delta;
      FT_Get_Kerning(face, prev_glyph_index, glyph_index, kerning_mode, &delta); // <<< USE CONSISTENT kerning_mode
      pen_x += delta.x;
    }

    glyph_infos[i].render_pen_x = pen_x; // Store pen position before advance

    // Use metrics (ascent/descent) from the loaded glyph (now potentially hinted)
    S32 glyph_ascent = slot->bitmap_top;
    S32 glyph_descent = (S32)slot->bitmap.rows - slot->bitmap_top;
    if (glyph_ascent > max_ascent)   { max_ascent = glyph_ascent; }
    if (glyph_descent > max_descent) { max_descent = glyph_descent; }

    // Use advance from the potentially hinted glyph
    pen_x += slot->advance.x;
    glyph_infos[i].advance_x = slot->advance.x; // Store potentially hinted advance

    prev_glyph_index = glyph_index;
  }

  // Calculate total width and height based on layout (using potentially hinted metrics)
  total_width_fixed = pen_x;
  S32 total_width_pixels = (total_width_fixed + 63) >> 6;

  // Calculate total height based on maximum ascent/descent values recorded during layout.
  S32 total_height = max_ascent + max_descent;

  // Handle cases like empty strings where calculated dimensions might be zero or negative.
  // Return a valid result structure but indicate zero dimensions and no atlas data.
  if (total_width_pixels <= 0 || total_height <= 0)
  {
    scratch_end(scratch); // Release temporary memory.
    ProfEnd();
    result.advance = (F32)(total_width_fixed >> 6); // Report the calculated advance.
    result.atlas_dim = v2s16(0, 0); // Atlas dimensions are zero.
    result.atlas = 0;               // No atlas buffer allocated.
    return result;
  }

  // Allocate the atlas buffer using the precise, unpadded dimensions.
  // Atlas format is RGBA8, requiring 4 bytes per pixel.
  result.atlas_dim = v2s16((S16)total_width_pixels, (S16)total_height);
  U64 atlas_bytes = (U64)total_width_pixels * (U64)total_height * 4;
  // Use push_array, which allocates and zero-initializes the memory.
  result.atlas = push_array(arena, U8, atlas_bytes);

  // Calculate row pitch for blitting operations.
  U8 *out_base = (U8 *)result.atlas;
  U64 out_pitch = (U64)total_width_pixels * 4; // Bytes per row.

  //- dan: Second pass: Render and blit glyphs using stored positions
  U64 non_empty_pixel_count = 0;

  for (U64 i = 0; i < string32.size; ++i)
  {
    FT_UInt glyph_index = glyph_infos[i].glyph_index;
    S32 render_pen_x_fixed = glyph_infos[i].render_pen_x; // Stored 26.6 pen position

    // Load glyph again (might be necessary if FT_Load_Glyph doesn't cache everything)
    // or ensure first pass loaded everything needed. Reloading is safer.
    error = FT_Load_Glyph(face, glyph_index, load_flags); // <<< Use final load_flags
    if (error) {
        // log_infof(...)
        continue;
    }

    // Render glyph if not already a bitmap
    if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
    {
        error = FT_Render_Glyph(face->glyph, render_mode); // <<< Use final render_mode
        if (error) {
            // log_infof(...)
            continue;
        }
    }

    FT_GlyphSlot slot = face->glyph;
    FT_Bitmap *bitmap = &slot->bitmap;

    // Calculate blit position using consistent metrics
    S32 blit_x = (render_pen_x_fixed >> 6) + slot->bitmap_left;
    S32 blit_y = max_ascent - slot->bitmap_top; // max_ascent also calculated consistently now

    // Blit the bitmap to the RGBA atlas
    U8 *in_row = bitmap->buffer;
    for (unsigned int y = 0; y < bitmap->rows; ++y)
    {
      S32 atlas_y = blit_y + (S32)y;
      if (atlas_y >= 0 && atlas_y < total_height) // Clip vertically
      {
        U8 *out_row = out_base + atlas_y * out_pitch;
        // Handle different pixel modes
        if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY)
        {
          U8 *in_pixel_ptr = in_row;
          for (unsigned int x = 0; x < bitmap->width; ++x)
          {
            S32 atlas_x = blit_x + (S32)x;
            if (atlas_x >= 0 && atlas_x < total_width_pixels) // Clip horizontally
            {
              U8 *out_pixel = out_row + atlas_x * 4;
              U8 in_val = *in_pixel_ptr;
              out_pixel[0] = 255;
              out_pixel[1] = 255;
              out_pixel[2] = 255;
              out_pixel[3] = in_val;
              if (in_val > 0) {
                non_empty_pixel_count++;
              }
            }
            in_pixel_ptr++;
          }
        }
        else if (bitmap->pixel_mode == FT_PIXEL_MODE_LCD)
        {
          U8 *in_pixel_rgb = in_row;
          for (unsigned int x = 0; x < bitmap->width; ++x)
          {
            S32 atlas_x = blit_x + (S32)x;
            if (atlas_x >= 0 && atlas_x < total_width_pixels) // Clip horizontally
            {
              U8 *out_pixel = out_row + atlas_x * 4;
              // Convert RGB coverage to single alpha (suboptimal for true subpixel)
              U8 r = in_pixel_rgb[0];
              U8 g = in_pixel_rgb[1];
              U8 b = in_pixel_rgb[2];
              U32 alpha_u32 = (54*r + 183*g + 19*b) >> 8;
              U8 alpha = (U8)(alpha_u32 > 255 ? 255 : alpha_u32);
              out_pixel[0] = 255;
              out_pixel[1] = 255;
              out_pixel[2] = 255;
              out_pixel[3] = alpha;
              if (alpha > 0) {
                 non_empty_pixel_count++;
              }
            }
            in_pixel_rgb += 3;
          }
        }
        else if (bitmap->pixel_mode == FT_PIXEL_MODE_BGRA) // BGRA Color Bitmap
        {
            U8 *in_pixel_bgra = in_row;
            for (unsigned int x = 0; x < bitmap->width; ++x)
            {
                S32 atlas_x = blit_x + (S32)x;
                if (atlas_x >= 0 && atlas_x < total_width_pixels) // Clip horizontally
                {
                    U8 *out_pixel = out_row + atlas_x * 4;
                    U8 b = in_pixel_bgra[0];
                    U8 g = in_pixel_bgra[1];
                    U8 r = in_pixel_bgra[2];
                    U8 a = in_pixel_bgra[3];
                    out_pixel[0] = r;
                    out_pixel[1] = g;
                    out_pixel[2] = b;
                    out_pixel[3] = a;
                    if (a > 0) {
                        non_empty_pixel_count++;
                    }
                }
                in_pixel_bgra += 4;
            }
        }
      }
      in_row += bitmap->pitch;
    }
  }

  //- dan: Finalize result
  result.advance = (F32)(total_width_fixed >> 6); // Use final fixed-point width from first pass

  // If nothing visible rendered, ensure atlas dimensions reflect that.
  if (non_empty_pixel_count == 0)
  {
      result.atlas_dim = v2s16(0, 0);
      // Consider freeing result.atlas or ensuring it's handled correctly downstream.
  }

  scratch_end(scratch);
  ProfEnd();
  return result;
}
