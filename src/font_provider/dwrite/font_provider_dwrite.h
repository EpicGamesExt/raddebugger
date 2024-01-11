/* date = November 2nd 2022 11:31 am */

#ifndef FONT_PROVIDER_DWRITE_H
#define FONT_PROVIDER_DWRITE_H

#include <dwrite.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "dwrite.lib")

//- rjf: font file loader interface types

struct FP_DWrite_FontFileLoaderVTable
{
  HRESULT (*QueryInterface)(void *obj, REFIID riid, void *ptr_to_object);
  ULONG (*AddRef)(void *obj);
  ULONG (*Release)(void *obj);
  HRESULT (*CreateStreamFromKey)(struct FP_DWrite_FontFileLoader *loader, void const *font_file_ref_key, UINT32 font_file_ref_key_size, IDWriteFontFileStream **stream_out);
};

struct FP_DWrite_FontFileLoader
{
  FP_DWrite_FontFileLoaderVTable *lpVtbl;
};

//- rjf: font file stream interface types

struct FP_DWrite_FontFileStreamVTable
{
  HRESULT (*QueryInterface)(void *obj, REFIID riid, void *ptr_to_object);
  ULONG (*AddRef)(void *obj);
  ULONG (*Release)(void *obj);
  HRESULT (*ReadFileFragment)(struct FP_DWrite_FontFileStream *obj, void const **fragment_start, UINT64 file_offset, UINT64 fragment_size, void **fragment_context);
  HRESULT (*ReleaseFileFragment)(struct FP_DWrite_FontFileStream *obj, void *fragment_context);
  HRESULT (*GetFileSize)(struct FP_DWrite_FontFileStream *obj, UINT64 *size_out);
  HRESULT (*GetLastWriteTime)(struct FP_DWrite_FontFileStream *obj, UINT64 *time_out);
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

struct FP_DWrite_State
{
  Arena *arena;
  IDWriteFactory *factory;
  IDWriteRenderingParams *base_rendering_params;
  IDWriteRenderingParams *rendering_params[FP_RasterMode_COUNT];
  IDWriteGdiInterop *gdi_interop;
  Vec2S32 bitmap_render_target_dim;
  IDWriteBitmapRenderTarget *bitmap_render_target;
  FP_DWrite_FontFileStreamNode *first_stream_node;
  FP_DWrite_FontFileStreamNode *last_stream_node;
  FP_DWrite_FontFileStreamNode *free_stream_node;
};

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
