// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FONT_PROVIDER_FREETYPE_H
#define FONT_PROVIDER_FREETYPE_H

// Breaks part of freetype headers
#undef internal
/* #include <freetype/freetype.h> */
#include <freetype/tttables.h>
#include <ft2build.h>

// Setup error strings for debugging
#undef FTERRORS_H_
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, NULL } };

const struct
{
  int          err_code;
  const char*  err_msg;
} freetype_errors[] =

#include FT_ERRORS_H

#include FT_FREETYPE_H

// Restore internal macro
#define internal static

// Types
#pragma pack(push, 1)
typedef struct FreeType_BitmapHeader FreeType_BitmapHeader;
struct FreeType_BitmapHeader {
  U16 signature;
  U32 file_size;
  U32 _reserved;
  U32 data_offset;
  // InfoHeader
  U32 info_header_size;
  U32 image_width;
  U32 image_height;
  U16 image_planes;
  U16 bit_depth;
  U32 compression_type;
  U32 compressed_image_size;
  U32 x_pixels_per_meter;
  U32 y_pixels_per_meter;
  U32 colors_used;
  U32 colors_important;
  U32 _repeating_color_table;
};
#pragma pack(pop)


// Convenience typedefs
/** Managing object for a font set */
typedef FT_Face FreeType_FontFace;

FreeType_FontFace freetype_face_from_handle(FP_Handle face);
FP_Handle freetype_handle_from_face(FreeType_FontFace face);
String8 freetype_error_string(U32 error);

B32 freetype_write_bmp_file(String8 name, U8* data, U32 width, U32 height);
B32 freetype_write_bmp_monochrome_file(String8 name, U8* data, U32 width, U32 height);
// Assumes the same size source and dest
U8* freetype_rectangle_slice( Arena* arena, U8* source, U32 x, U32 y, U32 width, U32 height);
U8* freetype_rectangle_copy(U8* dest,
                            U8* source,
                            Vec2S32 copy_size,
                            Vec2S32 src_coord,
                            Vec2S32 dest_coord,
                            U32 src_stride,
                            U32 dest_stride);

#endif // FONT_PROVIDER_FREETYPE_H
