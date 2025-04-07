// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FONT_PROVIDER_FREETYPE_H
#define FONT_PROVIDER_FREETYPE_H

////////////////////////////////
//~ rjf: Includes

#pragma push_macro("internal")
#undef internal
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H // For FT_Library_SetLcdFilter, FT_LCD_FILTER_DEFAULT
#include FT_SFNT_NAMES_H // For FT_Get_Sfnt_Table, FT_SFNT_OS2
#include FT_TRUETYPE_TABLES_H // For TT_OS2, FT_SFNT_OS2
#pragma pop_macro("internal")

#define internal static

////////////////////////////////
//~ rjf: Types

typedef struct FP_Freetype_State FP_Freetype_State;
struct FP_Freetype_State
{
  Arena *arena;
  FT_Library library;
};

////////////////////////////////
//~ rjf: Globals

global FP_Freetype_State *fp_freetype_state = 0;

#endif // FONT_PROVIDER_FREETYPE_H
