// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

#include <windows.h>

#pragma comment(lib, "opengl32")

// #define IMGL3W_IMPL
// #include "render_opengl_defines.h"

////////////////////////////////
//~ dmylo: Generated Code

#include "generated/render_opengl.meta.h"

////////////////////////////////
//~ dmylo: C-side Shader Types

struct R_OGL_Uniforms_Rect
{
  Vec2F32 viewport_size;
  F32 opacity;
  F32 _padding0;

  Vec4F32 texture_sample_channel_map[4];

  Vec2F32 texture_t2d_size;
  F32 _padding1;
  F32 _padding2;

  Vec4F32 xform[4];

  Vec2F32 xform_scale;
  F32 _padding3;
  F32 _padding4;
};

struct R_OGL_Uniforms_BlurPass
{
  Rng2F32 rect;
  Vec4F32 corner_radii;
  Vec2F32 viewport_size;
  U32 blur_count;
  U32 _padding;

  Vec4F32 kernel[ArrayCount(R_Blur_Kernel::weights)];
};

struct R_OGL_Uniforms_Mesh
{
  Mat4x4F32 xform;
};

////////////////////////////////
// dmylo: Main State Types

struct R_OGL_Tex2D
{
  R_OGL_Tex2D *next;
  U64 generation;
  GLuint texture;
  R_Tex2DKind kind;
  Vec2S32 size;
  R_Tex2DFormat format;

  //-dmylo: linked list of textures to upload :sync_upload
  R_OGL_Tex2D *upload_next;
  void* upload_data;
};

struct R_OGL_Buffer
{
  R_OGL_Buffer *next;

  U64 generation;
  GLuint buffer;
  R_BufferKind kind;
  U64 size;

  //-dmylo: linked list of buffers to upload :sync_upload
  R_OGL_Buffer *upload_next;
  void* upload_data;
};

struct R_OGL_Window
{
  R_OGL_Window *next;
  U64 generation;

  GLuint stage_scratch_fbo;
  GLuint stage_scratch_color;
  GLuint stage_fbo;
  GLuint stage_color;
  GLuint geo3d_fbo;
  GLuint geo3d_color;
  GLuint geo3d_depth;

  Vec2S32 last_resolution;
};

struct R_OGL_FlushBuffer
{
  R_OGL_FlushBuffer *next;
  GLuint buffer;
};

// -dmylo: structure holding data for a synchronous fill operation :sync_upload
struct R_OGL_Fill_Tex2D
{
  R_OGL_Fill_Tex2D *next;
  R_OGL_Tex2D *texture;
  Rng2S32 subrect;
  void *data;
};

struct R_OGL_Tex2DFormat {
  GLenum internal_format;
  GLenum data_format;
  GLenum data_type;
};

typedef HGLRC WINAPI wgl_create_context_attribs_arb(HDC hDC, HGLRC hShareContext,
                                                    const int *attribList);
typedef BOOL wgl_choose_pixel_format_arb(HDC hdc, const int *piAttribIList,
                                         const FLOAT *pfAttribFList, UINT nMaxFormats,
                                         int *piFormats, UINT *nNumFormats);

struct R_OGL_State
{
  // dmylo: OpenGL loaded functions
  bool initialized;
  R_OGL_Functions gl_functions;

  // dmylo: win32 OpenGL initialization stuff
  wgl_create_context_attribs_arb *wglCreateContextAttribsARB;
  wgl_choose_pixel_format_arb *wglChoosePixelFormatARB;
  HGLRC glrc;
  HWND fake_window;
  HGLRC fake_glrc;

  // dmylo: state
  Arena        *arena;
  R_OGL_Window *first_free_window;
  R_OGL_Tex2D *first_free_tex2d;
  R_OGL_Buffer *first_free_buffer;
  R_OGL_Tex2D *first_to_free_tex2d;
  R_OGL_Buffer *first_to_free_buffer;
  OS_Handle     device_rw_mutex;

  // dmylo: base OpenGL objects
  GLuint instance_scratch_buffer_64kb;

  // dmylo: rect
  GLuint rect_shader;
  GLuint rect_vao;
  GLuint rect_uniform_buffer;
  GLuint rect_uniform_block_index;

  // dmylo: blur
  GLuint blur_shader;
  GLuint blur_uniform_buffer;
  GLuint blur_uniform_block_index;
  GLuint blur_direction_uniform_location;

  // dmylo: geo3d
  GLuint geo3d_shader;
  GLuint geo3d_vao;
  GLuint geo3d_uniform_location;
  GLuint geo3dcomposite_shader;

  // dmylo: finalize
  GLuint finalize_shader;

  // dmylo: backups
  R_Handle backup_texture;

  //- dmylo: buffers to flush at subsequent frame
  Arena *buffer_flush_arena;
  R_OGL_FlushBuffer *first_buffer_to_flush;
  R_OGL_FlushBuffer *last_buffer_to_flush;

  //- dmylo: arena holding data to upload for current frame :sync_upload
  Arena *upload_arena;
  R_OGL_Buffer *first_buffer_to_upload;
  R_OGL_Tex2D *first_texture_to_upload;
  R_OGL_Fill_Tex2D *first_texture_to_fill;
};

////////////////////////////////
//~ dmylo: Globals
global R_OGL_State *r_ogl_state = 0;
global R_OGL_Window r_ogl_window_nil = {&r_ogl_window_nil};
global R_OGL_Tex2D r_ogl_tex2d_nil = {&r_ogl_tex2d_nil};
global R_OGL_Buffer r_ogl_buffer_nil = {&r_ogl_buffer_nil};

////////////////////////////////
//~ dmylo: Helpers

internal R_OGL_Window * r_ogl_window_from_handle(R_Handle handle);
internal R_Handle r_ogl_handle_from_window(R_OGL_Window *window);
internal R_OGL_Tex2D * r_ogl_tex2d_from_handle(R_Handle handle);
internal R_Handle r_ogl_handle_from_tex2d(R_OGL_Tex2D *texture);
internal R_OGL_Buffer * r_ogl_buffer_from_handle(R_Handle handle);
internal R_Handle r_ogl_handle_from_buffer(R_OGL_Buffer *buffer);
internal GLuint r_ogl_instance_buffer_from_size(U64 size);
internal GLuint r_ogl_compile_shader(String8 common, String8 src, GLenum kind);
internal GLuint r_ogl_link_shaders(GLuint vs, GLuint fs);
internal void r_ogl_initialize_window(OS_Handle handle);
internal void r_ogl_initialize(OS_Handle handle);
internal void r_ogl_upload_buffer(R_OGL_Buffer *buffer);
internal void r_ogl_upload_texture(R_OGL_Tex2D *texture);
internal void r_ogl_fill_tex2d_region(R_OGL_Tex2D *texture);
internal R_OGL_Tex2DFormat r_ogl_tex2d_format(R_Tex2DFormat format);

#endif // RENDER_OPENGL_H
