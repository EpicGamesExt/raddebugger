// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Globals

global FP_DWrite_State *fp_dwrite_state = 0;
global FP_DWrite_FontFileLoaderVTable fp_dwrite_static_data_font_file_loader__vtable =
{
  fp_dwrite_iunknown_noop__query_interface,
  fp_dwrite_iunknown_noop__add_ref,
  fp_dwrite_iunknown_noop__release,
  fp_dwrite_static_font_file_loader__stream_from_key,
};
global FP_DWrite_FontFileLoader fp_dwrite_static_data_font_file_loader = {&fp_dwrite_static_data_font_file_loader__vtable};
global FP_DWrite_FontFileStreamVTable fp_dwrite_static_data_font_file_stream__vtable =
{
  fp_dwrite_iunknown_noop__query_interface,
  fp_dwrite_iunknown_noop__add_ref,
  fp_dwrite_iunknown_noop__release,
  fp_dwrite_static_font_file_stream__read_file_fragment,
  fp_dwrite_static_font_file_stream__release_file_fragment,
  fp_dwrite_static_font_file_stream__get_file_size,
  fp_dwrite_static_font_file_stream__get_last_write_time,
};

////////////////////////////////
//~ rjf: Helpers

//- rjf: handle conversion functions

internal FP_DWrite_Font
fp_dwrite_font_from_handle(FP_Handle handle)
{
  FP_DWrite_Font result = {0};
  result.file = (IDWriteFontFile *)handle.u64[0];
  result.face = (IDWriteFontFace *)handle.u64[1];
  return result;
}

internal FP_Handle
fp_dwrite_handle_from_font(FP_DWrite_Font font)
{
  FP_Handle result = {0};
  result.u64[0] = (U64)font.file;
  result.u64[1] = (U64)font.face;
  return result;
}

//- rjf: file stream allocator

internal FP_DWrite_FontFileStreamNode *
fp_dwrite_font_file_stream_node_alloc(String8 *data_ptr)
{
  FP_DWrite_FontFileStreamNode *node = 0;
  for(FP_DWrite_FontFileStreamNode *n = fp_dwrite_state->first_stream_node; n != 0; n = n->next)
  {
    if(n->stream.data == data_ptr)
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    node = fp_dwrite_state->free_stream_node;
    if(node != 0)
    {
      SLLStackPop(fp_dwrite_state->free_stream_node);
    }
    else
    {
      node = push_array_no_zero(fp_dwrite_state->arena, FP_DWrite_FontFileStreamNode, 1);
    }
    MemoryZeroStruct(node);
    node->stream.lpVtbl = &fp_dwrite_static_data_font_file_stream__vtable;
    node->stream.data = data_ptr;
    DLLPushBack(fp_dwrite_state->first_stream_node, fp_dwrite_state->last_stream_node, node);
  }
  return node;
}

internal void
fp_dwrite_font_file_stream_node_release(FP_DWrite_FontFileStreamNode *node)
{
  DLLPushBack(fp_dwrite_state->first_stream_node, fp_dwrite_state->last_stream_node, node);
  SLLStackPush(fp_dwrite_state->free_stream_node, node);
}

//- rjf: iunknown no-op helpers

internal HRESULT
fp_dwrite_iunknown_noop__query_interface(void *obj, REFIID riid, void *ptr_to_object)
{
  return E_NOINTERFACE;
}

internal ULONG
fp_dwrite_iunknown_noop__add_ref(void *obj)
{
  ULONG result = 1;
  return result;
}

internal ULONG
fp_dwrite_iunknown_noop__release(void *obj)
{
  ULONG result = 1;
  return result;
}

//- rjf: font file loader interface function implementations

internal HRESULT
fp_dwrite_static_font_file_loader__stream_from_key(FP_DWrite_FontFileLoader *obj, void const *font_file_ref_key, UINT32 font_file_ref_key_size, IDWriteFontFileStream **stream_out)
{
  HRESULT result = S_OK;
  String8 *key = *(String8 **)font_file_ref_key;
  FP_DWrite_FontFileStreamNode *node = fp_dwrite_font_file_stream_node_alloc(key);
  *stream_out = (IDWriteFontFileStream *)&node->stream;
  return result;
}

//- rjf: font file stream  interface function implementations

internal HRESULT
fp_dwrite_static_font_file_stream__read_file_fragment(FP_DWrite_FontFileStream *obj, void const **fragment_start, UINT64 file_offset, UINT64 fragment_size, void **fragment_context)
{
  HRESULT result = S_OK;
  *fragment_start = obj->data->str + file_offset;
  *fragment_context = 0;
  return result;
}

internal HRESULT
fp_dwrite_static_font_file_stream__release_file_fragment(FP_DWrite_FontFileStream *obj, void *fragment_context)
{
  HRESULT result = S_OK;
  return result;
}

internal HRESULT
fp_dwrite_static_font_file_stream__get_file_size(FP_DWrite_FontFileStream *obj, UINT64 *size_out)
{
  HRESULT result = S_OK;
  *size_out = obj->data->size;
  return result;
}

internal HRESULT
fp_dwrite_static_font_file_stream__get_last_write_time(FP_DWrite_FontFileStream *obj, UINT64 *time_out)
{
  HRESULT result = S_OK;
  *time_out = 0;
  return result;
}

////////////////////////////////
//~ rjf: Backend Implementations

fp_hook void
fp_init(void)
{
  ProfBeginFunction();
  HRESULT error = 0;
  
  //- rjf: initialize main state
  {
    Arena *arena = arena_alloc();
    fp_dwrite_state = push_array(arena, FP_DWrite_State, 1);
    fp_dwrite_state->arena = arena;
  }
  
  //- rjf: make dwrite factory
  error = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory2, (void **)&fp_dwrite_state->factory);
  if(error == S_OK)
  {
    fp_dwrite_state->dwrite2_is_supported = 1;
  }
  else
  {
    error = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (void **)&fp_dwrite_state->factory);
  }
  
  //- rjf: register static data font "loader" interface
  error = IDWriteFactory_RegisterFontFileLoader(fp_dwrite_state->factory, (IDWriteFontFileLoader *)&fp_dwrite_static_data_font_file_loader);
  
  //- rjf: make base rendering params
  error = IDWriteFactory_CreateRenderingParams(fp_dwrite_state->factory, &fp_dwrite_state->base_rendering_params);
  
  //- rjf: make sharp rendering params
  {
    FLOAT gamma = IDWriteRenderingParams_GetGamma(fp_dwrite_state->base_rendering_params);
    FLOAT enhanced_contrast = IDWriteRenderingParams_GetEnhancedContrast(fp_dwrite_state->base_rendering_params);
    if(fp_dwrite_state->dwrite2_is_supported)
    {
      error = IDWriteFactory2_CreateCustomRenderingParams2((IDWriteFactory2 *)fp_dwrite_state->factory,
                                                           gamma,
                                                           enhanced_contrast,
                                                           enhanced_contrast,
                                                           1.f,
                                                           DWRITE_PIXEL_GEOMETRY_FLAT,
                                                           DWRITE_RENDERING_MODE_GDI_NATURAL,
                                                           DWRITE_GRID_FIT_MODE_ENABLED,
                                                           (IDWriteRenderingParams2 **)&fp_dwrite_state->rendering_params[FP_RasterMode_Sharp]);
    }
    else
    {
      error = IDWriteFactory_CreateCustomRenderingParams(fp_dwrite_state->factory,
                                                         gamma,
                                                         enhanced_contrast,
                                                         1.f,
                                                         DWRITE_PIXEL_GEOMETRY_FLAT,
                                                         DWRITE_RENDERING_MODE_GDI_NATURAL,
                                                         &fp_dwrite_state->rendering_params[FP_RasterMode_Sharp]);
    }
  }
  
  //- rjf: make smooth rendering params
  {
    FLOAT gamma = 1.f;
    FLOAT enhanced_contrast = 0.f;
    if(fp_dwrite_state->dwrite2_is_supported)
    {
      error = IDWriteFactory2_CreateCustomRenderingParams2((IDWriteFactory2 *)fp_dwrite_state->factory,
                                                           gamma,
                                                           enhanced_contrast,
                                                           enhanced_contrast,
                                                           1.f,
                                                           DWRITE_PIXEL_GEOMETRY_FLAT,
                                                           DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC,
                                                           DWRITE_GRID_FIT_MODE_DISABLED,
                                                           (IDWriteRenderingParams2 **)&fp_dwrite_state->rendering_params[FP_RasterMode_Smooth]);
    }
    else
    {
      error = IDWriteFactory_CreateCustomRenderingParams(fp_dwrite_state->factory,
                                                         gamma,
                                                         enhanced_contrast,
                                                         0.f,
                                                         DWRITE_PIXEL_GEOMETRY_FLAT,
                                                         DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
                                                         &fp_dwrite_state->rendering_params[FP_RasterMode_Smooth]);
    }
  }
  
  //- rjf: make dwrite gdi interop
  error = IDWriteFactory_GetGdiInterop(fp_dwrite_state->factory, &fp_dwrite_state->gdi_interop);
  
  //- rjf: build render target for rasterization
  fp_dwrite_state->bitmap_render_target_dim = v2s32(2048, 256);
  error = IDWriteGdiInterop_CreateBitmapRenderTarget(fp_dwrite_state->gdi_interop, 0, fp_dwrite_state->bitmap_render_target_dim.x, fp_dwrite_state->bitmap_render_target_dim.y, &fp_dwrite_state->bitmap_render_target);
  IDWriteBitmapRenderTarget_SetPixelsPerDip(fp_dwrite_state->bitmap_render_target, 1.0);
  ProfEnd();
}

fp_hook FP_Handle
fp_font_open(String8 path)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  String16 path16 = str16_from_8(scratch.arena, path);
  FP_DWrite_Font font = {0};
  HRESULT error = 0;
  
  //- rjf: open font file reference
  error = IDWriteFactory_CreateFontFileReference(fp_dwrite_state->factory, (WCHAR *)path16.str, 0, &font.file);
  
  //- rjf: open font face
  error = IDWriteFactory_CreateFontFace(fp_dwrite_state->factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &font.file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font.face);
  
  //- rjf: handlify & return
  FP_Handle handle = fp_dwrite_handle_from_font(font);
  scratch_end(scratch);
  ProfEnd();
  return handle;
}

fp_hook FP_Handle
fp_font_open_from_static_data_string(String8 *data_ptr)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  FP_DWrite_Font font = {0};
  HRESULT error = 0;
  
  //- rjf: open font file reference
  error = IDWriteFactory_CreateCustomFontFileReference(fp_dwrite_state->factory, &data_ptr, sizeof(String8 *), (IDWriteFontFileLoader *)&fp_dwrite_static_data_font_file_loader, &font.file);
  
  //- rjf: open font face
  error = IDWriteFactory_CreateFontFace(fp_dwrite_state->factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &font.file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font.face);
  
  //- rjf: handlify & return
  FP_Handle handle = fp_dwrite_handle_from_font(font);
  scratch_end(scratch);
  ProfEnd();
  return handle;
}

fp_hook void
fp_font_close(FP_Handle handle)
{
  ProfBeginFunction();
  FP_DWrite_Font font = fp_dwrite_font_from_handle(handle);
  if(font.face != 0)
  {
    IDWriteFontFace_Release(font.face);
  }
  if(font.file != 0)
  {
    IDWriteFontFile_Release(font.file);
  }
  ProfEnd();
}

fp_hook FP_Metrics
fp_metrics_from_font(FP_Handle handle)
{
  ProfBeginFunction();
  FP_DWrite_Font font = fp_dwrite_font_from_handle(handle);
  DWRITE_FONT_METRICS metrics = {0};
  if(font.face != 0)
  {
    IDWriteFontFace_GetMetrics(font.face, &metrics);
  }
  FP_Metrics result = {0};
  {
    result.design_units_per_em = (F32)metrics.designUnitsPerEm;
    result.ascent  = (F32)metrics.ascent;
    result.descent = (F32)metrics.descent;
    result.line_gap = (F32)metrics.lineGap;
    result.capital_height = (F32)metrics.capHeight;
  }
  ProfEnd();
  return result;
}

fp_hook NO_ASAN FP_RasterResult
fp_raster(Arena *arena, FP_Handle font_handle, F32 size, FP_RasterMode mode, String8 string)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  HRESULT error = 0;
  String32 string32 = str32_from_8(scratch.arena, string);
  FP_DWrite_Font font = fp_dwrite_font_from_handle(font_handle);
  COLORREF bg_color = RGB(0,   0,   0);
  COLORREF fg_color = RGB(255, 255, 255);
  
  //- rjf: get font metrics
  DWRITE_FONT_METRICS font_metrics = {0};
  if(font.face != 0)
  {
    IDWriteFontFace_GetMetrics(font.face, &font_metrics);
  }
  F32 design_units_per_em = (F32)font_metrics.designUnitsPerEm;
  
  //- rjf: get glyph indices
  U16 *glyph_indices = push_array_no_zero(scratch.arena, U16, string32.size);
  if(font.face != 0)
  {
    error = IDWriteFontFace_GetGlyphIndices(font.face, string32.str, string32.size, glyph_indices);
  }
  
  //- rjf: get metrics info
  U64 glyphs_count = string32.size;
  DWRITE_GLYPH_METRICS *glyphs_metrics = push_array_no_zero(scratch.arena, DWRITE_GLYPH_METRICS, glyphs_count);
  if(font.face != 0)
  {
    error = IDWriteFontFace_GetGdiCompatibleGlyphMetrics(font.face, (96.f/72.f)*size, 1.f, 0, 1, glyph_indices, glyphs_count, glyphs_metrics, 0);
  }
  
  //- rjf: derive info from metrics
  F32 advance = 0;
  Vec2S16 atlas_dim = {0};
  if(font.face != 0)
  {
    atlas_dim.y = (S16)((96.f/72.f) * size * (font_metrics.ascent + font_metrics.descent) / design_units_per_em);
    for(U64 idx = 0; idx < glyphs_count; idx += 1)
    {
      DWRITE_GLYPH_METRICS *glyph_metrics = glyphs_metrics + idx;
      F32 glyph_advance_width  = (96.f/72.f) * size * glyph_metrics->advanceWidth       / design_units_per_em;
      F32 glyph_advance_height = (96.f/72.f) * size * glyph_metrics->advanceHeight      / design_units_per_em;
      advance += glyph_advance_width;
      atlas_dim.x = Max(atlas_dim.x, (S16)(advance+1));
    }
    atlas_dim.x += 7;
    atlas_dim.x -= atlas_dim.x%8;
    atlas_dim.x += 4;
    atlas_dim.y += 4;
  }
  
  //- rjf: make dwrite bitmap for rendering
  IDWriteBitmapRenderTarget *render_target = 0;
  if(font.face != 0)
  {
    error = IDWriteGdiInterop_CreateBitmapRenderTarget(fp_dwrite_state->gdi_interop, 0, atlas_dim.x, atlas_dim.y, &render_target);
    IDWriteBitmapRenderTarget_SetPixelsPerDip(render_target, 1.f);
  }
  
  //- rjf: get bitmap & clear
  HDC dc = 0;
  if(font.face != 0)
  {
    dc = IDWriteBitmapRenderTarget_GetMemoryDC(render_target);
    HGDIOBJ original = SelectObject(dc, GetStockObject(DC_PEN));
    SetDCPenColor(dc, bg_color);
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SetDCBrushColor(dc, bg_color);
    Rectangle(dc, 0, 0, atlas_dim.x, atlas_dim.y);
    SelectObject(dc, original);
  }
  
  //- rjf: draw glyph run
  Vec2F32 draw_p = {1, (F32)atlas_dim.y - 2.f};
  if(font.face != 0)
  {
    F32 ascent = (96.f/72.f)*size * font_metrics.ascent / design_units_per_em;
    F32 descent = (96.f/72.f)*size * font_metrics.descent / design_units_per_em;
    draw_p.y -= descent;
  }
  DWRITE_GLYPH_RUN glyph_run = {0};
  if(font.face != 0)
  {
    glyph_run.fontFace = font.face;
    glyph_run.fontEmSize = size * 96.f/72.f;
    glyph_run.glyphCount = string32.size;
    glyph_run.glyphIndices = glyph_indices;
  }
  RECT bounding_box = {0};
  if(font.face != 0)
  {
    error = IDWriteBitmapRenderTarget_DrawGlyphRun(render_target, draw_p.x, draw_p.y,
                                                   DWRITE_MEASURING_MODE_NATURAL,
                                                   &glyph_run,
                                                   fp_dwrite_state->rendering_params[mode],
                                                   fg_color,
                                                   &bounding_box);
  }
  
  //- rjf: get bitmap
  DIBSECTION dib = {0};
  if(font.face != 0)
  {
    HBITMAP bitmap = (HBITMAP)GetCurrentObject(dc, OBJ_BITMAP);
    GetObject(bitmap, sizeof(dib), &dib);
  }
  
  //- rjf: fill & return
  FP_RasterResult result = {0};
  if(font.face != 0)
  {
    // rjf: fill basics
    result.atlas_dim    = atlas_dim;
    result.atlas        = push_array_no_zero(arena, U8, atlas_dim.x*atlas_dim.y*4);
    result.advance      = floor_f32(advance);
    result.bounding_box = r2s16p(bounding_box.left, bounding_box.top, bounding_box.right, bounding_box.bottom);
    
    // rjf: fill atlas
    {
      U8 *in_data   = (U8 *)dib.dsBm.bmBits;
      U64 in_pitch  = (U64)dib.dsBm.bmWidthBytes;
      U8 *out_data  = (U8 *)result.atlas;
      U64 out_pitch = atlas_dim.x * 4;
      U64 color_sum = 0;
      U8 *in_line = (U8 *)in_data;
      U8 *out_line = out_data;
      for(U64 y = 0; y < atlas_dim.y; y += 1)
      {
        U8 *in_pixel = in_line;
        U8 *out_pixel = out_line;
        for(U64 x = 0; x < atlas_dim.x; x += 1)
        {
          U8 in_pixel_byte = in_pixel[0];
          out_pixel[0] = 255;
          out_pixel[1] = 255;
          out_pixel[2] = 255;
          out_pixel[3] = in_pixel_byte;
          color_sum += in_pixel_byte;
          in_pixel += 4;
          out_pixel += 4;
        }
        in_line += in_pitch;
        out_line += out_pitch;
      }
      if(color_sum == 0)
      {
        result.atlas_dim = v2s16(0, 0);
      }
    }
    IDWriteBitmapRenderTarget_Release(render_target);
  }
  scratch_end(scratch);
  ProfEnd();
  return result;
}
