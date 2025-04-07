// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FONT_PROVIDER_H
#define FONT_PROVIDER_H

#define fp_hook C_LINKAGE

////////////////////////////////
//~ rjf: Types

typedef U32 FP_RasterFlags;
enum
{
  FP_RasterFlag_Smooth = (1<<0),
  FP_RasterFlag_Hinted = (1<<1),
};

typedef struct FP_Handle FP_Handle;
struct FP_Handle
{
  U64 u64[2];
};

typedef struct FP_Metrics FP_Metrics;
struct FP_Metrics
{
  F32 design_units_per_em;
  F32 ascent;
  F32 descent;
  F32 line_gap;
  F32 capital_height;
};

typedef struct FP_GlyphMetrics FP_GlyphMetrics;
struct FP_GlyphMetrics
{
    // Atlas Position & Size (Source Rect)
    Rng2S16 src_rect_px; // Position (x0,y0) and dimensions (x1-x0, y1-y0) within the atlas texture

    // Layout Metrics (relative to baseline & pen position)
    S16 bitmap_left;     // Horizontal offset from pen_x to bitmap's left edge
    S16 bitmap_top;      // Vertical offset from baseline up to bitmap's top edge
    F32 advance_x;       // Horizontal advance width for this glyph (in pixels)

    // Original Character Info (optional, but useful for debugging)
    U32 codepoint;
    U64 string_index;
};

typedef struct FP_RasterResult FP_RasterResult;
struct FP_RasterResult
{
    // Atlas Data
    Vec2S16 atlas_dim;       // Dimensions of the atlas texture
    void *atlas;             // Pixel data (RGBA8), aligned

    // Per-Glyph Layout Information
    U64 glyph_count;         // Number of glyphs in the metrics array
    FP_GlyphMetrics *metrics; // Array of metrics for each glyph
};

////////////////////////////////
//~ rjf: Basic Type Functions

internal FP_Handle fp_handle_zero(void);
internal B32 fp_handle_match(FP_Handle a, FP_Handle b);

////////////////////////////////
//~ rjf: Backend Hooks

fp_hook void fp_init(void);
fp_hook FP_Handle fp_font_open(String8 path);
fp_hook FP_Handle fp_font_open_from_static_data_string(String8 *data_ptr);
fp_hook void fp_font_close(FP_Handle handle);
fp_hook FP_Metrics fp_metrics_from_font(FP_Handle font);
fp_hook NO_ASAN FP_RasterResult fp_raster(Arena *arena, FP_Handle font, F32 size, FP_RasterFlags flags, String8 string);

#endif // FONT_PROVIDER_H
