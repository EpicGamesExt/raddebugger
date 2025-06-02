// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FONT_PROVIDER_DWRITE_H
#define FONT_PROVIDER_DWRITE_H

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "dwrite.lib")

// #include <dwrite.h>

////////////////////////////////
//~ rjf: (C) DirectWrite Definitions
//
// (courtesy of mmozeiko, Martins Mozeiko, https://github.com/mmozeiko/c_d2d_dwrite)
//
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <https://unlicense.org>

#include <combaseapi.h>
#include <dcommon.h>
#include <initguid.h>

//- rjf: enums

typedef enum DWRITE_FACTORY_TYPE {
  DWRITE_FACTORY_TYPE_SHARED   = 0,
  DWRITE_FACTORY_TYPE_ISOLATED = 1,
} DWRITE_FACTORY_TYPE;

typedef enum DWRITE_PIXEL_GEOMETRY {
  DWRITE_PIXEL_GEOMETRY_FLAT = 0,
  DWRITE_PIXEL_GEOMETRY_RGB  = 1,
  DWRITE_PIXEL_GEOMETRY_BGR  = 2,
} DWRITE_PIXEL_GEOMETRY;

typedef enum DWRITE_RENDERING_MODE {
  DWRITE_RENDERING_MODE_DEFAULT                     = 0,
  DWRITE_RENDERING_MODE_ALIASED                     = 1,
  DWRITE_RENDERING_MODE_GDI_CLASSIC                 = 2,
  DWRITE_RENDERING_MODE_GDI_NATURAL                 = 3,
  DWRITE_RENDERING_MODE_NATURAL                     = 4,
  DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC           = 5,
  DWRITE_RENDERING_MODE_OUTLINE                     = 6,
  DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC       = 2,
  DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL       = 3,
  DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL           = 4,
  DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC = 5,
} DWRITE_RENDERING_MODE;

typedef enum DWRITE_FONT_SIMULATIONS {
  DWRITE_FONT_SIMULATIONS_NONE    = 0,
  DWRITE_FONT_SIMULATIONS_BOLD    = 1,
  DWRITE_FONT_SIMULATIONS_OBLIQUE = 2,
} DWRITE_FONT_SIMULATIONS;

typedef enum DWRITE_FONT_FACE_TYPE {
  DWRITE_FONT_FACE_TYPE_CFF                 = 0,
  DWRITE_FONT_FACE_TYPE_TRUETYPE            = 1,
  DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION = 2,
  DWRITE_FONT_FACE_TYPE_TYPE1               = 3,
  DWRITE_FONT_FACE_TYPE_VECTOR              = 4,
  DWRITE_FONT_FACE_TYPE_BITMAP              = 5,
  DWRITE_FONT_FACE_TYPE_UNKNOWN             = 6,
  DWRITE_FONT_FACE_TYPE_RAW_CFF             = 7,
  DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION = 2,
} DWRITE_FONT_FACE_TYPE;

typedef enum DWRITE_GRID_FIT_MODE {
  DWRITE_GRID_FIT_MODE_DEFAULT  = 0,
  DWRITE_GRID_FIT_MODE_DISABLED = 1,
  DWRITE_GRID_FIT_MODE_ENABLED  = 2,
} DWRITE_GRID_FIT_MODE;

//- rjf: interfaces

typedef struct IDWriteFactory                  { struct { void* tbl[]; }* v; } IDWriteFactory;
typedef struct IDWriteFactory1                 { struct { void* tbl[]; }* v; } IDWriteFactory1;
typedef struct IDWriteFactory2                 { struct { void* tbl[]; }* v; } IDWriteFactory2;
typedef struct IDWriteRenderingParams          { struct { void* tbl[]; }* v; } IDWriteRenderingParams;
typedef struct IDWriteRenderingParams1         { struct { void* tbl[]; }* v; } IDWriteRenderingParams1;
typedef struct IDWriteRenderingParams2         { struct { void* tbl[]; }* v; } IDWriteRenderingParams2;
typedef struct IDWriteFontFileLoader           { struct { void* tbl[]; }* v; } IDWriteFontFileLoader;
typedef struct IDWriteFontFileStream           { struct { void* tbl[]; }* v; } IDWriteFontFileStream;
typedef struct IDWriteFontFile                 { struct { void* tbl[]; }* v; } IDWriteFontFile;
typedef struct IDWriteFontFace                 { struct { void* tbl[]; }* v; } IDWriteFontFace;
typedef struct IDWriteFontFace1                { struct { void* tbl[]; }* v; } IDWriteFontFace1;
typedef struct IDWriteFontFace2                { struct { void* tbl[]; }* v; } IDWriteFontFace2;
typedef struct IDWriteGdiInterop               { struct { void* tbl[]; }* v; } IDWriteGdiInterop;
typedef struct IDWriteBitmapRenderTarget       { struct { void* tbl[]; }* v; } IDWriteBitmapRenderTarget;
typedef struct IDWriteBitmapRenderTarget1      { struct { void* tbl[]; }* v; } IDWriteBitmapRenderTarget1;

//- rjf: structs

typedef struct DWRITE_GLYPH_METRICS {
  INT32  leftSideBearing;
  UINT32 advanceWidth;
  INT32  rightSideBearing;
  INT32  topSideBearing;
  UINT32 advanceHeight;
  INT32  bottomSideBearing;
  INT32  verticalOriginY;
} DWRITE_GLYPH_METRICS;

typedef struct DWRITE_GLYPH_OFFSET {
  FLOAT advanceOffset;
  FLOAT ascenderOffset;
} DWRITE_GLYPH_OFFSET;

typedef struct DWRITE_GLYPH_RUN {
  IDWriteFontFace*     fontFace;
  FLOAT                fontEmSize;
  UINT32               glyphCount;
  UINT16*              glyphIndices;
  FLOAT*               glyphAdvances;
  DWRITE_GLYPH_OFFSET* glyphOffsets;
  BOOL                 isSideways;
  UINT32               bidiLevel;
} DWRITE_GLYPH_RUN;

typedef struct DWRITE_FONT_METRICS {
  UINT16 designUnitsPerEm;
  UINT16 ascent;
  UINT16 descent;
  INT16  lineGap;
  UINT16 capHeight;
  UINT16 xHeight;
  INT16  underlinePosition;
  UINT16 underlineThickness;
  INT16  strikethroughPosition;
  UINT16 strikethroughThickness;
} DWRITE_FONT_METRICS;

typedef struct DWRITE_MATRIX {
  FLOAT m11;
  FLOAT m12;
  FLOAT m21;
  FLOAT m22;
  FLOAT dx;
  FLOAT dy;
} DWRITE_MATRIX;

//- rjf: GUIDs

DEFINE_GUID(IID_IDWriteFactory,                  0xb859ee5a, 0xd838, 0x4b5b, 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48);
DEFINE_GUID(IID_IDWriteFactory1,                 0x30572f99, 0xdac6, 0x41db, 0xa1, 0x6e, 0x04, 0x86, 0x30, 0x7e, 0x60, 0x6a);
DEFINE_GUID(IID_IDWriteFactory2,                 0x0439fc60, 0xca44, 0x4994, 0x8d, 0xee, 0x3a, 0x9a, 0xf7, 0xb7, 0x32, 0xec);

//- rjf: functions

EXTERN_C HRESULT DECLSPEC_IMPORT WINAPI DWriteCreateFactory (DWRITE_FACTORY_TYPE factoryType, const GUID* iid, void** factory);

//- rjf: methods

static inline HRESULT                           IDWriteFactory_RegisterFontFileLoader                        (IDWriteFactory* this_, IDWriteFontFileLoader* fontFileLoader) { return ((HRESULT (WINAPI*)(IDWriteFactory*, IDWriteFontFileLoader*))this_->v->tbl[13])(this_, fontFileLoader); }
static inline HRESULT                           IDWriteFactory_CreateRenderingParams                         (IDWriteFactory* this_, IDWriteRenderingParams** renderingParams) { return ((HRESULT (WINAPI*)(IDWriteFactory*, IDWriteRenderingParams**))this_->v->tbl[10])(this_, renderingParams); }
static inline HRESULT                           IDWriteFactory_CreateCustomRenderingParams                   (IDWriteFactory* this_, FLOAT gamma, FLOAT enhancedContrast, FLOAT clearTypeLevel, DWRITE_PIXEL_GEOMETRY pixelGeometry, DWRITE_RENDERING_MODE renderingMode, IDWriteRenderingParams** renderingParams) { return ((HRESULT (WINAPI*)(IDWriteFactory*, FLOAT, FLOAT, FLOAT, DWRITE_PIXEL_GEOMETRY, DWRITE_RENDERING_MODE, IDWriteRenderingParams**))this_->v->tbl[12])(this_, gamma, enhancedContrast, clearTypeLevel, pixelGeometry, renderingMode, renderingParams); }
static inline HRESULT                           IDWriteFactory_GetGdiInterop                                 (IDWriteFactory* this_, IDWriteGdiInterop** gdiInterop) { return ((HRESULT (WINAPI*)(IDWriteFactory*, IDWriteGdiInterop**))this_->v->tbl[17])(this_, gdiInterop); }
static inline HRESULT                           IDWriteFactory_CreateCustomFontFileReference                 (IDWriteFactory* this_, const void* fontFileReferenceKey, UINT32 fontFileReferenceKeySize, IDWriteFontFileLoader* fontFileLoader, IDWriteFontFile** fontFile) { return ((HRESULT (WINAPI*)(IDWriteFactory*, const void*, UINT32, IDWriteFontFileLoader*, IDWriteFontFile**))this_->v->tbl[8])(this_, fontFileReferenceKey, fontFileReferenceKeySize, fontFileLoader, fontFile); }
static inline HRESULT                           IDWriteFactory_CreateFontFileReference                       (IDWriteFactory* this_, const WCHAR* filePath, const FILETIME* lastWriteTime, IDWriteFontFile** fontFile) { return ((HRESULT (WINAPI*)(IDWriteFactory*, const WCHAR*, const FILETIME*, IDWriteFontFile**))this_->v->tbl[7])(this_, filePath, lastWriteTime, fontFile); }
static inline HRESULT                           IDWriteFactory_CreateFontFace                                (IDWriteFactory* this_, DWRITE_FONT_FACE_TYPE fontFaceType, UINT32 numberOfFiles, IDWriteFontFile** fontFiles, UINT32 faceIndex, DWRITE_FONT_SIMULATIONS fontFaceSimulationFlags, IDWriteFontFace** fontFace) { return ((HRESULT (WINAPI*)(IDWriteFactory*, DWRITE_FONT_FACE_TYPE, UINT32, IDWriteFontFile**, UINT32, DWRITE_FONT_SIMULATIONS, IDWriteFontFace**))this_->v->tbl[9])(this_, fontFaceType, numberOfFiles, fontFiles, faceIndex, fontFaceSimulationFlags, fontFace); }
static inline HRESULT                           IDWriteFactory2_CreateCustomRenderingParams2                 (IDWriteFactory2* this, FLOAT gamma, FLOAT enhancedContrast, FLOAT grayscaleEnhancedContrast, FLOAT clearTypeLevel, DWRITE_PIXEL_GEOMETRY pixelGeometry, DWRITE_RENDERING_MODE renderingMode, DWRITE_GRID_FIT_MODE gridFitMode, IDWriteRenderingParams2** renderingParams) { return ((HRESULT (WINAPI*)(IDWriteFactory2*, FLOAT, FLOAT, FLOAT, FLOAT, DWRITE_PIXEL_GEOMETRY, DWRITE_RENDERING_MODE, DWRITE_GRID_FIT_MODE, IDWriteRenderingParams2**))this->v->tbl[29])(this, gamma, enhancedContrast, grayscaleEnhancedContrast, clearTypeLevel, pixelGeometry, renderingMode, gridFitMode, renderingParams); }
static inline FLOAT                             IDWriteRenderingParams_GetEnhancedContrast                   (IDWriteRenderingParams* this_) { return ((FLOAT (WINAPI*)(IDWriteRenderingParams*))this_->v->tbl[4])(this_); }
static inline FLOAT                             IDWriteRenderingParams_GetGamma                              (IDWriteRenderingParams* this_) { return ((FLOAT (WINAPI*)(IDWriteRenderingParams*))this_->v->tbl[3])(this_); }
static inline HRESULT                           IDWriteGdiInterop_CreateBitmapRenderTarget                   (IDWriteGdiInterop* this_, HDC hdc, UINT32 width, UINT32 height, IDWriteBitmapRenderTarget** renderTarget) { return ((HRESULT (WINAPI*)(IDWriteGdiInterop*, HDC, UINT32, UINT32, IDWriteBitmapRenderTarget**))this_->v->tbl[7])(this_, hdc, width, height, renderTarget); }
static inline HRESULT                           IDWriteBitmapRenderTarget_SetPixelsPerDip                    (IDWriteBitmapRenderTarget* this_, FLOAT pixelsPerDip) { return ((HRESULT (WINAPI*)(IDWriteBitmapRenderTarget*, FLOAT))this_->v->tbl[6])(this_, pixelsPerDip); }
static inline HDC                               IDWriteBitmapRenderTarget_GetMemoryDC                        (IDWriteBitmapRenderTarget* this_) { return ((HDC (WINAPI*)(IDWriteBitmapRenderTarget*))this_->v->tbl[4])(this_); }
static inline HRESULT                           IDWriteBitmapRenderTarget_DrawGlyphRun                       (IDWriteBitmapRenderTarget* this_, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode, const DWRITE_GLYPH_RUN* glyphRun, IDWriteRenderingParams* renderingParams, COLORREF textColor, RECT* blackBoxRect) { return ((HRESULT (WINAPI*)(IDWriteBitmapRenderTarget*, FLOAT, FLOAT, DWRITE_MEASURING_MODE, const DWRITE_GLYPH_RUN*, IDWriteRenderingParams*, COLORREF, RECT*))this_->v->tbl[3])(this_, baselineOriginX, baselineOriginY, measuringMode, glyphRun, renderingParams, textColor, blackBoxRect); }
static inline UINT32                            IDWriteFontFace_Release                                      (IDWriteFontFace* this_) { return ((UINT32 (WINAPI*)(IDWriteFontFace*))this_->v->tbl[2])(this_); }
static inline void                              IDWriteFontFace_GetMetrics                                   (IDWriteFontFace* this_, DWRITE_FONT_METRICS* fontFaceMetrics) { ((void (WINAPI*)(IDWriteFontFace*, DWRITE_FONT_METRICS*))this_->v->tbl[8])(this_, fontFaceMetrics); }
static inline UINT32                            IDWriteFontFile_Release                                      (IDWriteFontFile* this_) { return ((UINT32 (WINAPI*)(IDWriteFontFile*))this_->v->tbl[2])(this_); }
static inline HRESULT                           IDWriteFontFace_GetGlyphIndices                              (IDWriteFontFace* this_, const UINT32* codePoints, UINT32 codePointCount, UINT16* glyphIndices) { return ((HRESULT (WINAPI*)(IDWriteFontFace*, const UINT32*, UINT32, UINT16*))this_->v->tbl[11])(this_, codePoints, codePointCount, glyphIndices); }
static inline HRESULT                           IDWriteFontFace_GetGdiCompatibleGlyphMetrics                 (IDWriteFontFace* this_, FLOAT emSize, FLOAT pixelsPerDip, const DWRITE_MATRIX* transform, BOOL useGdiNatural, const UINT16* glyphIndices, UINT32 glyphCount, DWRITE_GLYPH_METRICS* glyphMetrics, BOOL isSideways) { return ((HRESULT (WINAPI*)(IDWriteFontFace*, FLOAT, FLOAT, const DWRITE_MATRIX*, BOOL, const UINT16*, UINT32, DWRITE_GLYPH_METRICS*, BOOL))this_->v->tbl[17])(this_, emSize, pixelsPerDip, transform, useGdiNatural, glyphIndices, glyphCount, glyphMetrics, isSideways); }
static inline UINT32                            IDWriteBitmapRenderTarget_Release                            (IDWriteBitmapRenderTarget* this_) { return ((UINT32 (WINAPI*)(IDWriteBitmapRenderTarget*))this_->v->tbl[2])(this_); }

////////////////////////////////
//~ rjf: Font Provider Implementation Types

//- rjf: font file loader interface types

typedef struct FP_DWrite_FontFileLoader FP_DWrite_FontFileLoader;
typedef struct FP_DWrite_FontFileLoaderVTable FP_DWrite_FontFileLoaderVTable;

struct FP_DWrite_FontFileLoaderVTable
{
  HRESULT (*QueryInterface)(void *obj, REFIID riid, void *ptr_to_object);
  ULONG (*AddRef)(void *obj);
  ULONG (*Release)(void *obj);
  HRESULT (*CreateStreamFromKey)(FP_DWrite_FontFileLoader *loader, void const *font_file_ref_key, UINT32 font_file_ref_key_size, IDWriteFontFileStream **stream_out);
};

struct FP_DWrite_FontFileLoader
{
  FP_DWrite_FontFileLoaderVTable *lpVtbl;
};

//- rjf: font file stream interface types

typedef struct FP_DWrite_FontFileStream FP_DWrite_FontFileStream;
typedef struct FP_DWrite_FontFileStreamVTable FP_DWrite_FontFileStreamVTable;
typedef struct FP_DWrite_FontFileStreamNode FP_DWrite_FontFileStreamNode;

struct FP_DWrite_FontFileStreamVTable
{
  HRESULT (*QueryInterface)(void *obj, REFIID riid, void *ptr_to_object);
  ULONG (*AddRef)(void *obj);
  ULONG (*Release)(void *obj);
  HRESULT (*ReadFileFragment)(FP_DWrite_FontFileStream *obj, void const **fragment_start, UINT64 file_offset, UINT64 fragment_size, void **fragment_context);
  HRESULT (*ReleaseFileFragment)(FP_DWrite_FontFileStream *obj, void *fragment_context);
  HRESULT (*GetFileSize)(FP_DWrite_FontFileStream *obj, UINT64 *size_out);
  HRESULT (*GetLastWriteTime)(FP_DWrite_FontFileStream *obj, UINT64 *time_out);
};

struct FP_DWrite_FontFileStream
{
  FP_DWrite_FontFileStreamVTable *lpVtbl;
  String8 *data;
};

struct FP_DWrite_FontFileStreamNode
{
  FP_DWrite_FontFileStreamNode *next;
  FP_DWrite_FontFileStreamNode *prev;
  FP_DWrite_FontFileStream stream;
};

//- rjf: state & underlying handle types

typedef struct FP_DWrite_State FP_DWrite_State;
struct FP_DWrite_State
{
  Arena *arena;
  B32 dwrite2_is_supported;
  IDWriteFactory *factory;
  IDWriteRenderingParams *base_rendering_params;
  IDWriteRenderingParams *rendering_params_sharp_hinted;
  IDWriteRenderingParams *rendering_params_sharp_unhinted;
  IDWriteRenderingParams *rendering_params_smooth_hinted;
  IDWriteRenderingParams *rendering_params_smooth_unhinted;
  IDWriteGdiInterop *gdi_interop;
  Vec2S32 bitmap_render_target_dim;
  IDWriteBitmapRenderTarget *bitmap_render_target;
  FP_DWrite_FontFileStreamNode *first_stream_node;
  FP_DWrite_FontFileStreamNode *last_stream_node;
  FP_DWrite_FontFileStreamNode *free_stream_node;
};

typedef struct FP_DWrite_Font FP_DWrite_Font;
struct FP_DWrite_Font
{
  IDWriteFontFile *file;
  IDWriteFontFace *face;
};

////////////////////////////////
//~ rjf: Helpers

//- rjf: handle conversion functions
internal FP_DWrite_Font fp_dwrite_font_from_handle(FP_Handle handle);
internal FP_Handle fp_dwrite_handle_from_font(FP_DWrite_Font font);

//- rjf: file stream allocator
internal FP_DWrite_FontFileStreamNode *fp_dwrite_font_file_stream_node_alloc(String8 *data_ptr);
internal void fp_dwrite_font_file_stream_node_release(FP_DWrite_FontFileStreamNode *node);

//- rjf: iunknown no-op helpers
internal HRESULT fp_dwrite_iunknown_noop__query_interface(void *obj, REFIID riid, void *ptr_to_object);
internal ULONG fp_dwrite_iunknown_noop__add_ref(void *obj);
internal ULONG fp_dwrite_iunknown_noop__release(void *obj);

//- rjf: font file loader interface function implementations
internal HRESULT fp_dwrite_static_font_file_loader__stream_from_key(FP_DWrite_FontFileLoader *obj, void const *font_file_ref_key, UINT32 font_file_ref_key_size, IDWriteFontFileStream **stream_out);

//- rjf: font file stream  interface function implementations
internal HRESULT fp_dwrite_static_font_file_stream__read_file_fragment(FP_DWrite_FontFileStream *obj, void const **fragment_start, UINT64 file_offset, UINT64 fragment_size, void **fragment_context);
internal HRESULT fp_dwrite_static_font_file_stream__release_file_fragment(FP_DWrite_FontFileStream *obj, void *fragment_context);
internal HRESULT fp_dwrite_static_font_file_stream__get_file_size(FP_DWrite_FontFileStream *obj, UINT64 *size_out);
internal HRESULT fp_dwrite_static_font_file_stream__get_last_write_time(FP_DWrite_FontFileStream *obj, UINT64 *time_out);

#endif // FONT_PROVIDER_DWRITE_H
