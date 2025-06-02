// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

////////////////////////////////
//~ rjf: Defines

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_TEXTURE_MAX_LEVEL              0x813D

#define GL_R8                             0x8229

#define GL_ARRAY_BUFFER                   0x8892
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_TESS_CONTROL_SHADER            0x8E88
#define GL_INFO_LOG_LENGTH                0x8B84

#define GL_TEXTURE_2D_ARRAY               0x8C1A

#define GL_COMPILE_STATUS                 0x8B81

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define GL_TEXTURE16                      0x84D0
#define GL_TEXTURE17                      0x84D1
#define GL_TEXTURE18                      0x84D2
#define GL_TEXTURE19                      0x84D3
#define GL_TEXTURE20                      0x84D4
#define GL_TEXTURE21                      0x84D5
#define GL_TEXTURE22                      0x84D6
#define GL_TEXTURE23                      0x84D7
#define GL_TEXTURE24                      0x84D8
#define GL_TEXTURE25                      0x84D9
#define GL_TEXTURE26                      0x84DA
#define GL_TEXTURE27                      0x84DB
#define GL_TEXTURE28                      0x84DC
#define GL_TEXTURE29                      0x84DD
#define GL_TEXTURE30                      0x84DE
#define GL_TEXTURE31                      0x84DF

#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242

////////////////////////////////
//~ rjf: OS Backend Includes

#if OS_WINDOWS
# include "render/opengl/win32/render_opengl_win32.h"
#elif OS_LINUX
# include "render/opengl/linux/render_opengl_linux.h"
#else
# error OS portion of OpenGL rendering backend not defined.
#endif

////////////////////////////////
//~ rjf: Shader Metadata Types

typedef struct R_OGL_Attribute R_OGL_Attribute;
struct R_OGL_Attribute
{
  U64 index;
  String8 name;
  GLenum type;
  U64 count;
};

typedef struct R_OGL_AttributeArray R_OGL_AttributeArray;
struct R_OGL_AttributeArray
{
  R_OGL_Attribute *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Generated Code

#include "render/opengl/generated/render_opengl.meta.h"

////////////////////////////////
//~ rjf: OpenGL Procedure List

#define R_OGL_ProcedureXList \
X(glGenBuffers, void, (GLsizei n, GLuint *buffers))\
X(glBindBuffer, void, (GLenum target, GLuint buffer))\
X(glDeleteBuffers, void, (GLsizei n, GLuint *buffers))\
X(glGenVertexArrays, void, (GLsizei n, GLuint *arrays))\
X(glBindVertexArray, void, (GLuint array))\
X(glCreateProgram, GLuint, (void))\
X(glCreateShader, GLuint, (GLenum type))\
X(glShaderSource, void, (GLuint shader, GLsizei count, char **string, GLint *length))\
X(glCompileShader, void, (GLuint shader))\
X(glGetShaderiv, void, (GLuint shader, GLenum pname, GLint *params))\
X(glGetShaderInfoLog, void, (GLuint shader, GLsizei bufSize, GLsizei *length, char *infoLog))\
X(glGetProgramiv, void, (GLuint program, GLenum pname, GLint *params))\
X(glGetProgramInfoLog, void, (GLuint program, GLsizei bufSize, GLsizei *length, char *infoLog))\
X(glAttachShader, void, (GLuint program, GLuint shader))\
X(glLinkProgram, void, (GLuint program))\
X(glValidateProgram, void, (GLuint program))\
X(glDeleteShader, void, (GLuint shader))\
X(glUseProgram, void, (GLuint program))\
X(glGetUniformLocation, GLint, (GLuint program, char *name))\
X(glGetAttribLocation, GLint, (GLuint program, char *name))\
X(glEnableVertexAttribArray, void, (GLuint index))\
X(glVertexAttribPointer, void, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer))\
X(glBufferData, void, (GLenum target, ptrdiff_t size, void *data, GLenum usage))\
X(glBufferSubData, void, (GLenum target, ptrdiff_t offset, ptrdiff_t size, const void *data))\
X(glBlendFuncSeparate, void, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha))\
X(glUniform1f, void, (GLint location, GLfloat v0))\
X(glUniform2f, void, (GLint location, GLfloat v0, GLfloat v1))\
X(glUniform3f, void, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2))\
X(glUniform4f, void, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3))\
X(glUniformMatrix4fv, void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))\
X(glUniform1i, void, (GLint location, GLint v0))\
X(glTexImage3D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels))\
X(glTexSubImage3D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels))\
X(glGenerateMipmap, void, (GLenum target))\
X(glBindAttribLocation, void, (GLuint programObj, GLuint index, char *name))\
X(glBindFragDataLocation, void, (GLuint program, GLuint color, char *name))\
X(glActiveTexture, void, (GLenum texture))\
X(glVertexAttribDivisor, void, (GLuint index, GLuint divisor))\
X(glDrawArraysInstanced, void, (GLenum mode, GLint first, GLsizei count, GLsizei instancecount))\
X(glDebugMessageCallback, void, (void (*)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam), void *user_data))\

#define X(name, r, p) typedef r name##_FunctionType p;
R_OGL_ProcedureXList
#undef X
#define X(name, r, p) global name##_FunctionType *name = 0;
R_OGL_ProcedureXList
#undef X

////////////////////////////////
//~ rjf: State Types

typedef struct R_OGL_FormatInfo R_OGL_FormatInfo;
struct R_OGL_FormatInfo
{
  GLint internal_format;
  GLenum format;
  GLenum base_type;
};

typedef struct R_OGL_Tex2D R_OGL_Tex2D;
struct R_OGL_Tex2D
{
  R_OGL_Tex2D *next;
  GLuint id;
  R_ResourceKind resource_kind;
  R_Tex2DFormat fmt;
  Vec2S32 size;
};

typedef struct R_OGL_FlushBuffer R_OGL_FlushBuffer;
struct R_OGL_FlushBuffer
{
  R_OGL_FlushBuffer *next;
  GLuint id;
};

typedef struct R_OGL_State R_OGL_State;
struct R_OGL_State
{
  Arena *arena;
  R_OGL_Tex2D *free_tex2d;
  GLuint shaders[R_OGL_ShaderKind_COUNT];
  GLuint all_purpose_vao;
  GLuint scratch_buffer_64kb;
  GLuint white_texture;
  Arena *buffer_flush_arena;
  R_OGL_FlushBuffer *first_buffer_to_flush;
  R_OGL_FlushBuffer *last_buffer_to_flush;
};

////////////////////////////////
//~ rjf: Globals

global R_OGL_State *r_ogl_state = 0;

////////////////////////////////
//~ rjf: Helpers

internal R_Handle r_ogl_handle_from_tex2d(R_OGL_Tex2D *t);
internal R_OGL_Tex2D *r_ogl_tex2d_from_handle(R_Handle h);
internal R_OGL_FormatInfo r_ogl_format_info_from_tex2dformat(R_Tex2DFormat fmt);
internal GLuint r_ogl_instance_buffer_from_size(U64 size);
internal void r_ogl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

#define glUseProgramScope(...) DeferLoop(glUseProgram(__VA_ARGS__), glUseProgram(0))
#define glBindVertexArrayScope(...) DeferLoop(glBindVertexArray(__VA_ARGS__), glBindVertexArray(0))

////////////////////////////////
//~ rjf: OS-Specific Hooks

internal VoidProc *r_ogl_os_load_procedure(char *name);
internal void r_ogl_os_init(CmdLine *cmdln);
internal R_Handle r_ogl_os_window_equip(OS_Handle window);
internal void r_ogl_os_window_unequip(OS_Handle os, R_Handle r);
internal void r_ogl_os_select_window(OS_Handle os, R_Handle r);
internal void r_ogl_os_window_swap(OS_Handle os, R_Handle r);

#endif // RENDER_OPENGL_H
