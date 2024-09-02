// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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
  S32 error = FT_New_Face( freetype_instance, (char*)path.str, face_index, &face);
  Assert(error == 0);

  // TODO: DELETE
  U32 test_index =  FT_Get_Char_Index(face, (FT_ULong)'A');
  B32 error_sizing = FT_Set_Char_Size( face, 50 * 64, 0, 100, 0 );
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

  // Set memory file bit
  args.flags = FT_OPEN_MEMORY;
  args.memory_base = data_ptr->str;
  args.memory_size = data_ptr->size;
  args.driver = NULL;           // Try every font driver

  S32 error = FT_Open_Face( freetype_instance, &args, face_index, &face);
  Assert(error == 0);

// TODO: DELETE
  U32 test_index =  FT_Get_Char_Index(face, (FT_ULong)'A');
  B32 error_attach = FT_Attach_File(face, (char*)data_ptr->str);
  B32 error_char = FT_Load_Char(face, (FT_ULong)'A', 0x0);
  B32 error_glyph = FT_Load_Glyph(face, test_index, FT_LOAD_DEFAULT);

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

  // result.design_units_per_em = face->units_per_EM;
  result.ascent = face->ascender;
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
  NotImplemented;
  Temp scratch = scratch_begin(0, 0);
  FreeType_FontFace face = freetype_face_from_handle(font);
  U32* charmap_indices = push_array(scratch.arena, U32, 4+ string.size);
  U32 errors = 0;
  U8 x_char = 0;
  U32 i = 0;
  for (; i < string.size; ++i)
  {
    x_char = string.str[i];
    charmap_indices[i] = FT_Get_Char_Index(face, (FT_ULong)x_char);

    // Undefined character code or NULL
    if (charmap_indices == 0) { ++errors; }
  }
  scratch_end(scratch);
}
