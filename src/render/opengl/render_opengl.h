#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

#pragma comment(lib, "opengl32")

#define IMGL3W_IMPL
#include "render_opengl_defines.h"

////////////////////////////////
//~ rjf: Generated Code

#include "generated/render_opengl.meta.h"

////////////////////////////////
//~ rjf: C-side Shader Types

struct R_OGL_Uniforms_Rect
{
  Vec2F32 viewport_size;
  F32 opacity;
  F32 _padding0;

  Vec4F32 texture_sample_channel_map[4];

  Vec2F32 texture_t2d_size;
  // Vec2F32 translate;
  F32 _padding1;
  F32 _padding2;

  Vec4F32 xform[4];

  Vec2F32 xform_scale;
  F32 _padding3;
  F32 _padding4;
};

struct R_OGL_Tex2D
{
  R_OGL_Tex2D *next;
  U64 generation;
  GLuint texture;
  R_Tex2DKind kind;
  Vec2S32 size;
  R_Tex2DFormat format;
};

struct R_OGL_Buffer
{
  R_OGL_Buffer *next;
  U64 generation;
  GLuint buffer;
  R_BufferKind kind;
  U64 size;
};

struct R_OGL_Window
{
  R_OGL_Window *next;
  U64 generation;
  HGLRC glrc;

  Vec2S32 last_resolution;
};

struct R_OGL_FlushBuffer
{
  R_OGL_FlushBuffer *next;
  GLuint buffer;
};

struct R_OGL_State
{
  // dmylo: OpenGL loaded functions
  bool initialized;
  ImGL3WProcs gl;

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

  // dmylo: shaders
  GLuint rect_shader;
  GLuint rect_vao;
  GLuint rect_uniform_buffer;
  GLuint rect_uniform_block_index;
  GLuint rect_texture_location;

  // dmylo: backups
  R_Handle backup_texture;

  // dmylo: buffers to flush at subsequent frame
  Arena *buffer_flush_arena;
  R_OGL_FlushBuffer *first_buffer_to_flush;
  R_OGL_FlushBuffer *last_buffer_to_flush;
};

////////////////////////////////
//~ dmylo: Globals
global R_OGL_State *r_ogl_state = 0;
global R_OGL_Window r_ogl_window_nil = {&r_ogl_window_nil};
global R_OGL_Tex2D r_ogl_tex2d_nil = {&r_ogl_tex2d_nil};
global R_OGL_Buffer r_ogl_buffer_nil = {&r_ogl_buffer_nil};

#endif // RENDER_OPENGL_H
