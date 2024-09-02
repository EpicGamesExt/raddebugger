// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// Breaks part of freetype headers
#undef internal
#include <freetype/freetype.h>
#include <freetype/tttables.h>
// Restore internal macro
#define internal static

// Convenience typedefs
/** Managing object for a font set */
typedef FT_Face FreeType_FontFace;

FreeType_FontFace freetype_face_from_handle(FP_Handle face);
FP_Handle freetype_handle_from_face(FreeType_FontFace face);
