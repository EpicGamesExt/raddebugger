// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb/stb_image_write.h"

internal FT_Library freetype_instance = NULL;
internal Arena* freetype_arena = NULL;

FreeType_FontFace
freetype_face_from_handle(FP_Handle face)
{
  FreeType_FontFace result = PtrFromInt(*face.u64);
  return result;
}
FP_Handle
freetype_handle_from_face(FreeType_FontFace face)
{
  FP_Handle result = {0};
  *result.u64 = IntFromPtr(face);
  return result;
}

fp_hook void
fp_init(void)
{
  // NOTE: There is an alternative function to manage memory manually with freetype
  freetype_arena = arena_alloc();
  FT_Init_FreeType(&freetype_instance);
}

fp_hook FP_Handle
fp_font_open(String8 path)
{
  /* NotImplemented; */
  FP_Handle result = {0};
  FT_Face face = {0};
  S64 face_index = 0x0;
  FT_Size_RequestRec sizing_request = {0};
  // Pt Fractional Scaling
  sizing_request.width = 30*64;
  sizing_request.height = 30*64;
  // DPI
  sizing_request.horiResolution = 30;
  sizing_request.vertResolution = 30;
  S32 error = FT_New_Face( freetype_instance, (char*)path.str, face_index, &face);
  B32 error_sizing = FT_Request_Size(face, &sizing_request);

  Assert(error == 0);
  Assert(error_sizing == 0);


  // TODO: DELETE
  U32 test_index =  FT_Get_Char_Index(face, (FT_ULong)'A');
  B32 error_attach = FT_Attach_File(face, (char*)path.str);
  B32 error_char = FT_Load_Char(face, (FT_ULong)'A', 0x0);
  B32 error_glyph = FT_Load_Glyph(face, test_index, FT_LOAD_DEFAULT);

  result = freetype_handle_from_face(face);
  return result;
}

fp_hook FP_Handle
fp_font_open_from_static_data_string(String8 *data_ptr)
{
  // TODO: TEST IF FINSIHED
  FP_Handle result = {0};
  FT_Face face = {0};
  FT_Open_Args args = {0};
  S64 face_index = 0x0;
  FT_Size_RequestRec sizing_request = {0};

// Pt Fractional Scaling
  sizing_request.width = 300*64;
  sizing_request.height = 300*64;
  // DPI
  sizing_request.horiResolution = 30;
  sizing_request.vertResolution = 30;

  // Set memory file bit
  args.flags = FT_OPEN_MEMORY;
  args.memory_base = data_ptr->str;
  args.memory_size = data_ptr->size;
  args.driver = NULL;           // Try every font driver

  S32 error = FT_Open_Face( freetype_instance, &args, face_index, &face);
  // Setup font scaling
  B32 error_sizing = FT_Request_Size(face, &sizing_request);

  Assert(error == 0);
  Assert(error_sizing == 0);

  result = freetype_handle_from_face(face);
  return result;
}

fp_hook void
fp_font_close(FP_Handle handle)
{
  FreeType_FontFace face = freetype_face_from_handle(handle);
  FT_Done_Face(face);
}

fp_hook FP_Metrics
fp_metrics_from_font(FP_Handle font)
{
  FP_Metrics result = {0};
  if (*font.u64 == 0x0) return result;
  FreeType_FontFace face = freetype_face_from_handle(font);

  // NOTE(mallchad): Note I don't know which is more useful so leaving OS2 header for now
  // result.design_units_per_em = face->units_per_EM;
  // result.ascent = face->ascender;
  // result.descender = face->descender;
  // result.line_gap = 0;
  // result.capital_height = face->ascender; // NOTE: It's the same thing.

  /* Get the horizontal header for horizontally written fonts
     NOTE: Should work for OTF and TrueType fonts
     NOTE: From freetype documentation: You should use the `sTypoAscender` field
     of the 'OS/2' table instead if you want the correct one.
     NOTE(mallchad): These OS2 tables are supposedly version dependet, might some
     fail? who knows.
  */
  TT_Header* header_tt = (TT_Header*)FT_Get_Sfnt_Table(face,  FT_SFNT_HEAD);
  TT_OS2* header_os2 = (TT_OS2*)FT_Get_Sfnt_Table(face,  FT_SFNT_OS2);
  AssertAlways(header_os2 != NULL);
  result.ascent = header_os2->sTypoAscender;
  result.descent = header_os2->sTypoDescender;
  result.line_gap = header_os2->sTypoLineGap;
  result.capital_height = header_os2->sCapHeight;

  return result;
}

fp_hook NO_ASAN
FP_RasterResult
fp_raster(Arena *arena,
          FP_Handle font,
          F32 size,
          FP_RasterMode mode,
          String8 string)
{
  /* NotImplemented; */
  /* NOTE(mallchad): I am assuming this function will only ever run for 1 glyph
     at a time because that is the API usage and how it works in the dwrite
     section. If anything else occurs the code may not behave properly */
  Assert(string.size <= 1);
  FP_RasterResult result = {0};
  FreeType_FontFace face = freetype_face_from_handle(font);
  if (face == 0) { return result; }

  Temp scratch = scratch_begin(0, 0);
  U32* charmap_indices = push_array(scratch.arena, U32, 4+ string.size);
  F32 win32_magic_dimensions = (96.f/72.f);
  // Error counting
  U32 errors_char = 0;
  U32 errors_glyph = 0;
  U32 errors_render = 0;
  // Last loop iteration had error
  B32 err_char = 0;
  B32 err_glyph = 0;
  B32 err_render = 0;

  FT_Matrix matrix;
  FT_Vector pen;
  F64 tau = 6.283185307;
  F64 angle = 1 * tau;
  /* set up matrix
     Magic fixed point math */
  matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
  matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
  matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
  matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );
  // Flip font y
  // matrix.yy = -matrix.yy;

  // the pen position in 26.6 cartesian space coordinates;
  // start at (300,200) relative to the upper left corner
  pen.x = 0;
  pen.y = face->ascender;
  U32 glyph_height = 60;
  U32 glyph_width = 100;
  U32 magic_dpi = 64;
  U32 total_advance = 0;
  U32 total_height = 0;
  Vec2S16 atlas_dim = {0};
  U32 glyph_count = string.size;
  /* NOTE(mallchad): 'size' is the actual internal glyph "scale" not char count
  Convert magic fixed point coordinate system to internal floating point representation */
  F32 glyph_advance_width = (face->max_advance_width * (96.f/72.f) * size) / face->units_per_EM;
  F32 glyph_advance_height = (face->max_advance_height * (96.f/72.f) * size) / face->units_per_EM;
  U8* atlas = push_array_no_zero( scratch.arena, U8, 5* glyph_advance_width * glyph_advance_height );

  U8 x_char = 0;
  U32 x_charmap = 0;
  U32 i = 0;
  for (; i < glyph_count; ++i)
  {
    x_char = string.str[i];
    x_charmap = FT_Get_Char_Index(face, (FT_ULong)x_char);
    charmap_indices[i] = x_charmap;

    // Undefined character code or NULL face
    /* err_char += FT_Load_Char(face, (FT_ULong)x_char, 0x0); */ // Load char and index in oneshot
    err_glyph += FT_Load_Glyph(face, x_charmap, FT_LOAD_DEFAULT);
    /* FT_Set_Transform( face, &matrix, &pen ); // Set transforms */
    FT_Set_Transform( face, NULL, NULL ); // Reset transforms
    err_render += FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

    Assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
    // TODO(mallchad): Untested section
    freetype_rectangle_copy( atlas,
                             face->glyph->bitmap.buffer,
                             vec_2s32(face->glyph->bitmap.pitch, face->glyph->bitmap.rows),
                             vec_2s32(0, 0),
                             vec_2s32(total_advance, total_height),
                             face->glyph->bitmap.pitch,
                             glyph_advance_width);

    // Section compied from dwrite
    total_advance += glyph_advance_width;
    total_height += glyph_advance_height;
    atlas_dim.x = Max(atlas_dim.x, (S16)(1+ total_advance));
    err_char += (errors_glyph > 0);
    err_glyph += (errors_glyph > 0);
    err_render += (errors_render > 0);
  }
  Assert(!errors_glyph);
  Assert(!errors_glyph);
  Assert(!errors_render);
  atlas_dim.x += 7;
  atlas_dim.x -= atlas_dim.x%8;
  atlas_dim.x += 4;
  atlas_dim.y += 4;
  // Fill raster basics
  result.atlas_dim.x = total_advance;
  result.atlas_dim.y = total_height;
  result.advance = glyph_advance_width;
  result.atlas = push_array(arena, U8, 4* result.atlas_dim.x*  result.atlas_dim.y);

  static U32 null_errors = 0;
  /* WTF. Even if no errors came back???
     leaving here as a guard against 3rd party NULL shenanigans */
  if (face->glyph->bitmap.buffer != NULL)
  {
    // Debug Stuff
    String8 debug_name = str8_lit("debug/test.bmp");
    freetype_write_bmp_monochrome_file(debug_name,
                                       face->glyph->bitmap.buffer,
                                       face->glyph->bitmap.pitch,
                                       face->glyph->bitmap.rows);


  } else { ++null_errors; }
  scratch_end(scratch);

  return result;
}

// Converts RGB to BGR format
B32
freetype_write_bmp_file(String8 name, U8* data, U32 width, U32 height)
{
  if (width == 0 ||
      height == 0 ||
      data == 0)
  { return 0; }

  Temp scratch = scratch_begin(0, 0);
  U32 greedy_filesize = 4* width*height;
  U8* buffer = push_array(scratch.arena, U8, greedy_filesize);
  FreeType_BitmapHeader* header = (FreeType_BitmapHeader*)buffer;
  U32 pixbuf_offset = (sizeof(FreeType_BitmapHeader)*2);
  U8* pixbuf = buffer+ pixbuf_offset;

  MemoryCopy(&header->signature, "BM\0\0\0\0", 2);
  header->file_size = greedy_filesize;
  header->_reserved = 0;
  header->data_offset = pixbuf_offset;
  header->info_header_size = 40;
  header->image_width = width;
  header->image_height = height;
  header->image_planes = 1;
  header->bit_depth = 24;
  header->compression_type = 0x0; // No compression
  header->x_pixels_per_meter = 2835;  // Does this even matter?
  header->y_pixels_per_meter = 2835;
  header->compressed_image_size = 4* width*height;

  // Copy Data
  U8* tmp_buffer = push_array(scratch.arena, U8, width*height*4 );
  U64 bmp_stride = (width* 3);
  U64 data_stride = (width* 3);
  U32 mod4 = (bmp_stride%4);
  bmp_stride = (mod4 ? bmp_stride + (4- mod4) : bmp_stride); // Fix stride padding
  U32 x = 0;
  for (int iy=0; iy < height; ++iy)
  {
    for (int ix=0; ix < width +1; ++ix)
    {
      x = ix*3;
      // Reverse RGB to BGR Format
      tmp_buffer[2+ x + (iy* bmp_stride)] = data[ 0+ x+ (iy *data_stride) ];
      tmp_buffer[1+ x + (iy* bmp_stride)] = data[ 1+ x+ (iy *data_stride) ];
      tmp_buffer[0+ x + (iy* bmp_stride)] = data[ 2+ x+ (iy *data_stride) ];
    }
  }
  // Invert Image
  for (U64 i_height=0; i_height < height; ++i_height)
  {
    MemoryCopy(pixbuf+  ((height-i_height-1)* bmp_stride),
                  tmp_buffer+ (i_height* bmp_stride),
                  data_stride);
  }

  OS_Handle file = os_file_open(OS_AccessFlag_Write, name);
  os_file_write(file, rng_1u64(0, greedy_filesize), buffer);
  os_file_close(file);
  scratch_end(scratch);

  return (*file.u64 > 0);
}

B32
freetype_write_bmp_monochrome_file(String8 name, U8* data, U32 width, U32 height)
{
  if (width == 0 ||
      height == 0 ||
      data == 0)
  { return 0; }

  Temp scratch = scratch_begin(0, 0);
  U32 greedy_filesize = 4* width*height;
  U8* filebuf = push_array(scratch.arena, U8, greedy_filesize);
  FreeType_BitmapHeader* header = (FreeType_BitmapHeader*)filebuf;
  U32 pixbuf_offset = (sizeof(FreeType_BitmapHeader)*2);
  U8* pixbuf = filebuf+ pixbuf_offset;

  MemoryZero(filebuf, greedy_filesize);
  MemoryCopy(&header->signature, "BM\0\0\0\0", 2);
  header->file_size = greedy_filesize;
  header->_reserved = 0;
  header->data_offset = pixbuf_offset;
  header->info_header_size = 40;
  header->image_width = width;
  header->image_height = height;
  header->image_planes = 1;
  header->bit_depth = 24;
  header->compression_type = 0x0; // No compression
  header->x_pixels_per_meter = 2835;
  header->y_pixels_per_meter = 2835;
  header->compressed_image_size = 4* width*height;

  U8* buffer = push_array(scratch.arena, U8, 4* width*height);
  // Error printing
  /* MemorySet(buffer, 0xAF, width * height * 4); */
  /* MemorySet(pixbuf, 0xAF, width * height * 4); */
  U8 x_color = 0;
  U64 x = 0;
  // Monochromize
  for (U64 i=0; i<(width*(height)); ++i)
  {
    x_color = data[i];
    x = 3* i;

    buffer[0+ x] = x_color;
    buffer[1+ x] = x_color;
    buffer[2+ x] = x_color;
  }
  // Invert
  U64 bitmap_stride = (width* 3);
  U64 data_stride = (width* 3);
  U32 mod4 = (bitmap_stride) % 4;
  bitmap_stride = (mod4 ? bitmap_stride + (4- mod4) : bitmap_stride); // Fix stride padding
  for (U64 i_height=0; i_height <height; ++i_height)
  {
    Assert((S64)height - (S64)i_height-1 >= 0);
    MemoryCopy(pixbuf+  ((i_height)* bitmap_stride),
               buffer+ ((height - i_height - 1)* data_stride),
               data_stride);
  }
  MemorySet(pixbuf + (bitmap_stride * height-1), 0xA0, bitmap_stride);

  OS_Handle file = os_file_open(OS_AccessFlag_Write, name);
  os_file_write(file, rng_1u64(0, greedy_filesize), filebuf);
  os_file_close(file);
  scratch_end(scratch);

  return (*file.u64 > 0);
}

U8*
freetype_rectangle_copy(U8* dest,
                        U8* source,
                        Vec2S32 copy_size,
                        Vec2S32 src_coord,
                        Vec2S32 dest_coord,
                        U32 src_stride,
                        U32 dest_stride)
{
  // TODO(mallchad): Untested
  U8* result = (dest+ dest_coord.x + dest_stride);
  U8* copy_begin = 0x0;
  U8* result_begin = 0x0;
  for (int iy = src_coord.y; iy < copy_size.y; ++iy)
  {
    copy_begin = source+ src_coord.x + (iy* src_stride);
    result_begin = dest+ dest_coord.x + (iy* dest_stride);
    MemoryCopy(result_begin, copy_begin, copy_size.x);
  }
  return result;
}

U8*
freetype_rectangle_slice( Arena* arena, U8* source, U32 x, U32 y, U32 width, U32 height)
{
  U8* result = push_array(arena, U8, 4+ (width*height));
  U8* copy_begin;
  U8* result_begin;
  U32 amount = width;
  for (int iy = y; iy < height; ++iy)
  {
    copy_begin = source+ x + (iy* width);
    result_begin = result+ (iy* width);
    MemoryCopy(result_begin, copy_begin, amount);
  }

  return result;
}
