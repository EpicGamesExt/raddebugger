// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef RENDER_OPENGL_META_H
#define RENDER_OPENGL_META_H

// Adapted from Dear ImGui generated OpenGL bindings
// Adapted from KHR/khrplatform.h to avoid including entire file.
#ifndef __khrplatform_h_
typedef          float         khronos_float_t;
typedef signed   char          khronos_int8_t;
typedef unsigned char          khronos_uint8_t;
typedef signed   short int     khronos_int16_t;
typedef unsigned short int     khronos_uint16_t;
#ifdef _WIN64
typedef signed   long long int khronos_intptr_t;
typedef signed   long long int khronos_ssize_t;
#else
typedef signed   long  int     khronos_intptr_t;
typedef signed   long  int     khronos_ssize_t;
#endif

#if defined(_MSC_VER) && !defined(__clang__)
typedef signed   __int64       khronos_int64_t;
typedef unsigned __int64       khronos_uint64_t;
#elif (defined(__clang__) || defined(__GNUC__)) && (__cplusplus < 201100)
#include <stdint.h>
typedef          int64_t       khronos_int64_t;
typedef          uint64_t      khronos_uint64_t;
#else
typedef signed   long long     khronos_int64_t;
typedef unsigned long long     khronos_uint64_t;
#endif
#endif  // __khrplatform_h_

typedef void GLvoid;
typedef unsigned int GLenum;
typedef khronos_float_t GLfloat;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef double GLdouble;
typedef unsigned int GLuint;
typedef unsigned char GLboolean;
typedef khronos_uint8_t GLubyte;
typedef khronos_float_t GLclampf;
typedef double GLclampd;
typedef khronos_ssize_t GLsizeiptr;
typedef khronos_intptr_t GLintptr;
typedef char GLchar;
typedef khronos_int16_t GLshort;
typedef khronos_int8_t GLbyte;
typedef khronos_uint16_t GLushort;
typedef khronos_uint16_t GLhalf;
typedef struct __GLsync *GLsync;
typedef khronos_uint64_t GLuint64;
typedef khronos_int64_t GLint64;
typedef khronos_uint64_t GLuint64EXT;
typedef khronos_int64_t GLint64EXT;

typedef void (*PFNGL_DrawArrays) (GLenum mode, GLint first, GLsizei count);
typedef void (*PFNGL_DrawElements) (GLenum mode, GLsizei count, GLenum type, const void *indices);
typedef void (*PFNGL_GenBuffers) (GLsizei n, GLuint *buffers);
typedef void (*PFNGL_BindBuffer) (GLenum target, GLuint buffer);
typedef void (*PFNGL_BufferData) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (*PFNGL_DeleteShader) (GLuint shader);
typedef GLuint (*PFNGL_CreateShader) (GLenum type);
typedef void (*PFNGL_ShaderSource) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (*PFNGL_CompileShader) (GLuint shader);
typedef void (*PFNGL_GetShaderiv) (GLuint shader, GLenum pname, GLint *params);
typedef void (*PFNGL_GetShaderInfoLog) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint (*PFNGL_CreateProgram) (void);
typedef void (*PFNGL_GetProgramInfoLog) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (*PFNGL_AttachShader) (GLuint program, GLuint shader);
typedef void (*PFNGL_LinkProgram) (GLuint program);
typedef void (*PFNGL_GetProgramiv) (GLuint program, GLenum pname, GLint *params);
typedef void (*PFNGL_GenVertexArrays) (GLsizei n, GLuint *arrays);
typedef GLuint (*PFNGL_GetUniformBlockIndex) (GLuint program, const GLchar* unifromBlockName);
typedef void (*PFNGL_UniformBlockBinding) (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
typedef void (*PFNGL_GenTextures) (GLsizei n, GLuint *textures);
typedef void (*PFNGL_BindTexture) (GLenum target, GLuint texture);
typedef void (*PFNGL_TexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (*PFNGL_TexSubImage2D) (GLenum target, GLint level, GLint xofffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
typedef void (*PFNGL_Disable) (GLenum cap);
typedef void (*PFNGL_Enable) (GLenum cap);
typedef void (*PFNGL_Clear) (GLbitfield mask);
typedef void (*PFNGL_ClearColor) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (*PFNGL_CullFace) (GLenum mode);
typedef void (*PFNGL_FrontFace) (GLenum mode);
typedef void (*PFNGL_BlendFunc) (GLenum sfactor, GLenum dfactor);
typedef void (*PFNGL_Viewport) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (*PFNGL_UseProgram) (GLuint program);
typedef void (*PFNGL_BindVertexArray) (GLuint array);
typedef void (*PFNGL_ActiveTexture) (GLenum texture);
typedef void (*PFNGL_DeleteBuffers) (GLsizei n, const GLuint *buffers);
typedef void (*PFNGL_DeleteTextures) (GLsizei n, const GLuint *textures);
typedef void* (*PFNGL_MapBuffer) (GLenum target, GLenum access);
typedef GLboolean (*PFNGL_UnmapBuffer) (GLenum target);
typedef void (*PFNGL_EnableVertexAttribArray) (GLuint index);
typedef void (*PFNGL_VertexAttribDivisor) (GLuint index, GLuint divisor);
typedef void (*PFNGL_VertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (*PFNGL_BindBufferBase) (GLenum target, GLuint index, GLuint buffer);
typedef void (*PFNGL_TexParameteri) (GLenum target, GLenum pname, GLint param);
typedef void (*PFNGL_Scissor) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (*PFNGL_DrawArraysInstanced) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);

const char* r_ogl_g_function_names[] = 
{
  "glDrawArrays",
  "glDrawElements",
  "glGenBuffers",
  "glBindBuffer",
  "glBufferData",
  "glDeleteShader",
  "glCreateShader",
  "glShaderSource",
  "glCompileShader",
  "glGetShaderiv",
  "glGetShaderInfoLog",
  "glCreateProgram",
  "glGetProgramInfoLog",
  "glAttachShader",
  "glLinkProgram",
  "glGetProgramiv",
  "glGenVertexArrays",
  "glGetUniformBlockIndex",
  "glUniformBlockBinding",
  "glGenTextures",
  "glBindTexture",
  "glTexImage2D",
  "glTexSubImage2D",
  "glDisable",
  "glEnable",
  "glClear",
  "glClearColor",
  "glCullFace",
  "glFrontFace",
  "glBlendFunc",
  "glViewport",
  "glUseProgram",
  "glBindVertexArray",
  "glActiveTexture",
  "glDeleteBuffers",
  "glDeleteTextures",
  "glMapBuffer",
  "glUnmapBuffer",
  "glEnableVertexAttribArray",
  "glVertexAttribDivisor",
  "glVertexAttribPointer",
  "glBindBufferBase",
  "glTexParameteri",
  "glScissor",
  "glDrawArraysInstanced",
};

typedef struct R_OGL_Functions R_OGL_Functions;
struct R_OGL_Functions
{
  union
  {
    void* _pointers[ArrayCount(r_ogl_g_function_names)];
    struct
    {
      PFNGL_DrawArrays DrawArrays;
      PFNGL_DrawElements DrawElements;
      PFNGL_GenBuffers GenBuffers;
      PFNGL_BindBuffer BindBuffer;
      PFNGL_BufferData BufferData;
      PFNGL_DeleteShader DeleteShader;
      PFNGL_CreateShader CreateShader;
      PFNGL_ShaderSource ShaderSource;
      PFNGL_CompileShader CompileShader;
      PFNGL_GetShaderiv GetShaderiv;
      PFNGL_GetShaderInfoLog GetShaderInfoLog;
      PFNGL_CreateProgram CreateProgram;
      PFNGL_GetProgramInfoLog GetProgramInfoLog;
      PFNGL_AttachShader AttachShader;
      PFNGL_LinkProgram LinkProgram;
      PFNGL_GetProgramiv GetProgramiv;
      PFNGL_GenVertexArrays GenVertexArrays;
      PFNGL_GetUniformBlockIndex GetUniformBlockIndex;
      PFNGL_UniformBlockBinding UniformBlockBinding;
      PFNGL_GenTextures GenTextures;
      PFNGL_BindTexture BindTexture;
      PFNGL_TexImage2D TexImage2D;
      PFNGL_TexSubImage2D TexSubImage2D;
      PFNGL_Disable Disable;
      PFNGL_Enable Enable;
      PFNGL_Clear Clear;
      PFNGL_ClearColor ClearColor;
      PFNGL_CullFace CullFace;
      PFNGL_FrontFace FrontFace;
      PFNGL_BlendFunc BlendFunc;
      PFNGL_Viewport Viewport;
      PFNGL_UseProgram UseProgram;
      PFNGL_BindVertexArray BindVertexArray;
      PFNGL_ActiveTexture ActiveTexture;
      PFNGL_DeleteBuffers DeleteBuffers;
      PFNGL_DeleteTextures DeleteTextures;
      PFNGL_MapBuffer MapBuffer;
      PFNGL_UnmapBuffer UnmapBuffer;
      PFNGL_EnableVertexAttribArray EnableVertexAttribArray;
      PFNGL_VertexAttribDivisor VertexAttribDivisor;
      PFNGL_VertexAttribPointer VertexAttribPointer;
      PFNGL_BindBufferBase BindBufferBase;
      PFNGL_TexParameteri TexParameteri;
      PFNGL_Scissor Scissor;
      PFNGL_DrawArraysInstanced DrawArraysInstanced;
    };
  };
};

#define GL_ARRAY_BUFFER 0x8892
#define GL_UNIFORM_BUFFER 0x8A11
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_WRITE_ONLY 0x88B9
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0X8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_ONE 1
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_STENCIL_TEST 0x0B90
#define GL_VIEWPORT 0x0BA2
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_TEXTURE_2D 0X0DE1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_RGBA 0x1908
#define GL_BGRA 0x80E1
#define GL_RED 0x1903
#define GL_RG 0x8227
#define GL_R8 0x8229
#define GL_RG8 0x822B
#define GL_RGBA8 0x8058
#define GL_R16 0x822A
#define GL_RGBA16 0x805B
#define GL_R32F 0x822E
#define GL_RG32F 0x8230
#define GL_RGBA32F 0x8814
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_TEXTURE_MAG_FILTER 0X2800
#define GL_TEXTURE_MIN_FILTER 0X2801
#define GL_CW 0x0900
#define GL_TEXTURE0 0X84C0


C_LINKAGE_BEGIN
read_only global String8 r_ogl_g_rect_common_src =
str8_lit_comp(
""
"\n"
"#version 330 core\n"
"#define float2   vec2\n"
"#define float3   vec3\n"
"#define float4   vec4\n"
"#define float3x3 mat3\n"
"#define float4x4 mat4\n"
"\n"
"layout (std140) uniform Globals\n"
"{\n"
"  float2 viewport_size_px;\n"
"  float opacity;\n"
"  float _padding;\n"
"\n"
"  float4x4 texture_sample_channel_map;\n"
"\n"
"  float2 texture_t2d_size_px;\n"
"  float  _padding1;\n"
"  float  _padding2;\n"
"\n"
"  mat4x4 xform;\n"
"\n"
"  float2 xform_scale;\n"
"  float  _padding3;\n"
"  float  _padding4;\n"
"};\n"
""
);

read_only global String8 r_ogl_g_rect_vs_src =
str8_lit_comp(
""
"\n"
"layout (location=0) in float4 a_dst_rect_px;\n"
"layout (location=1) in float4 a_src_rect_px;\n"
"layout (location=2) in float4 a_color00;\n"
"layout (location=3) in float4 a_color01;\n"
"layout (location=4) in float4 a_color10;\n"
"layout (location=5) in float4 a_color11;\n"
"layout (location=6) in float4 a_corner_radii_px;\n"
"layout (location=7) in float4 a_style_params;\n"
"\n"
"out Vertex2Pixel\n"
"{\n"
"  flat float2 rect_half_size_px;\n"
"  float2 texcoord_pct;\n"
"  float2 sdf_sample_pos;\n"
"  float4 tint;\n"
"  float corner_radius_px;\n"
"  flat float border_thickness_px;\n"
"  flat float softness_px;\n"
"  flat float omit_texture;\n"
"} vertex2pixel;\n"
"\n"
"void main()\n"
"{\n"
"  //- rjf: unpack & xform rectangle src/dst vertices\n"
"  float2 dst_p0_px  = a_dst_rect_px.xy;\n"
"  float2 dst_p1_px  = a_dst_rect_px.zw;\n"
"  float2 src_p0_px  = a_src_rect_px.xy;\n"
"  float2 src_p1_px  = a_src_rect_px.zw;\n"
"  float2 dst_size_px = abs(dst_p1_px - dst_p0_px);\n"
"\n"
"  //- rjf: unpack style params\n"
"  float border_thickness_px = a_style_params.x;\n"
"  float softness_px         = a_style_params.y;\n"
"  float omit_texture        = a_style_params.z;\n"
"\n"
"  //- rjf: prep per-vertex arrays to sample from (p: position, t: texcoord, c: colorcoord, r: cornerradius)\n"
"  float2 dst_p_verts_px[4];\n"
"  dst_p_verts_px[0] = float2(dst_p0_px.x, dst_p1_px.y);\n"
"  dst_p_verts_px[1] = float2(dst_p0_px.x, dst_p0_px.y);\n"
"  dst_p_verts_px[2] = float2(dst_p1_px.x, dst_p1_px.y);\n"
"  dst_p_verts_px[3] = float2(dst_p1_px.x, dst_p0_px.y);\n"
"\n"
"  float2 src_p_verts_px[4];\n"
"  src_p_verts_px[0] = float2(src_p0_px.x, src_p1_px.y);\n"
"  src_p_verts_px[1] = float2(src_p0_px.x, src_p0_px.y);\n"
"  src_p_verts_px[2] = float2(src_p1_px.x, src_p1_px.y);\n"
"  src_p_verts_px[3] = float2(src_p1_px.x, src_p0_px.y);\n"
"\n"
"  float dst_r_verts_px[4] = float[](\n"
"    a_corner_radii_px.y,\n"
"    a_corner_radii_px.x,\n"
"    a_corner_radii_px.w,\n"
"    a_corner_radii_px.z\n"
"  );\n"
"\n"
"  float4 src_color[4];\n"
"  src_color[0] = a_color01;\n"
"  src_color[1] = a_color00;\n"
"  src_color[2] = a_color11;\n"
"  src_color[3] = a_color10;\n"
"\n"
"  int vertex_id = gl_VertexID;\n"
"  float2 dst_verts_pct = float2((vertex_id >> 1) != 0 ? 1.f : 0.f,\n"
"                                (vertex_id & 1) != 0 ? 0.f : 1.f);\n"
"\n"
"  // rjf: fill vertex -> pixel data\n"
"  {\n"
"    float2 xformed_pos = (transpose(xform) * float4(dst_p_verts_px[vertex_id], 1.f, 0.0f)).xy;\n"
"    xformed_pos.y = viewport_size_px.y - xformed_pos.y;\n"
"    gl_Position.xy                    = 2.f * xformed_pos/viewport_size_px - 1.f;\n"
"    gl_Position.z                     = 0.f;\n"
"    gl_Position.w                     = 1.f;\n"
"    vertex2pixel.rect_half_size_px    = dst_size_px / 2.f * xform_scale;\n"
"    vertex2pixel.texcoord_pct         = src_p_verts_px[vertex_id] / texture_t2d_size_px;\n"
"    vertex2pixel.sdf_sample_pos       = (2.f * dst_verts_pct - 1.f) * vertex2pixel.rect_half_size_px;\n"
"    vertex2pixel.tint                 = src_color[vertex_id];\n"
"    vertex2pixel.corner_radius_px     = dst_r_verts_px[vertex_id];\n"
"    vertex2pixel.border_thickness_px  = border_thickness_px;\n"
"    vertex2pixel.softness_px          = softness_px;\n"
"    vertex2pixel.omit_texture         = omit_texture;\n"
"  }\n"
"}\n"
""
);

read_only global String8 r_ogl_g_rect_fs_src =
str8_lit_comp(
""
"\n"
"in Vertex2Pixel\n"
"{\n"
"  flat float2 rect_half_size_px;\n"
"  float2 texcoord_pct;\n"
"  float2 sdf_sample_pos;\n"
"  float4 tint;\n"
"  float corner_radius_px;\n"
"  flat float border_thickness_px;\n"
"  flat float softness_px;\n"
"  flat float omit_texture;\n"
"} vertex2pixel;\n"
"\n"
"out float4 o_final_color;\n"
"\n"
"uniform sampler2D main_t2d;\n"
"\n"
"float rect_sdf(float2 sample_pos, float2 rect_half_size, float r)\n"
"{\n"
"  return length(max(abs(sample_pos) - rect_half_size + r, 0.0)) - r;\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"  // rjf: blend corner colors to produce final tint\n"
"  float4 tint = vertex2pixel.tint;\n"
"\n"
"  // rjf: sample texture\n"
"  float4 albedo_sample = float4(1, 1, 1, 1);\n"
"  if(vertex2pixel.omit_texture < 1)\n"
"  {\n"
"    albedo_sample = texture(main_t2d, vertex2pixel.texcoord_pct) * transpose(texture_sample_channel_map);\n"
"  }\n"
"\n"
"  // rjf: determine SDF sample position\n"
"  float2 sdf_sample_pos = vertex2pixel.sdf_sample_pos;\n"
"\n"
"  // rjf: sample for borders\n"
"  float border_sdf_t = 1;\n"
"  if(vertex2pixel.border_thickness_px > 0)\n"
"  {\n"
"    float border_sdf_s = rect_sdf(sdf_sample_pos,\n"
"                                  vertex2pixel.rect_half_size_px - float2(vertex2pixel.softness_px*2.f, vertex2pixel.softness_px*2.f) - vertex2pixel.border_thickness_px,\n"
"                                  max(vertex2pixel.corner_radius_px-vertex2pixel.border_thickness_px, 0));\n"
"    border_sdf_t = smoothstep(0, 2*vertex2pixel.softness_px, border_sdf_s);\n"
"  }\n"
"  if(border_sdf_t < 0.001f)\n"
"  {\n"
"    discard;\n"
"  }\n"
"\n"
"  // rjf: sample for corners\n"
"  float corner_sdf_t = 1;\n"
"  if(vertex2pixel.corner_radius_px > 0 || vertex2pixel.softness_px > 0.75f)\n"
"  {\n"
"    float corner_sdf_s = rect_sdf(sdf_sample_pos,\n"
"                                  vertex2pixel.rect_half_size_px - float2(vertex2pixel.softness_px*2.f, vertex2pixel.softness_px*2.f),\n"
"                                  vertex2pixel.corner_radius_px);\n"
"    corner_sdf_t = 1-smoothstep(0, 2*vertex2pixel.softness_px, corner_sdf_s);\n"
"  }\n"
"\n"
"  // rjf: form+return final color\n"
"  o_final_color = albedo_sample;\n"
"  o_final_color *= tint;\n"
"  o_final_color.a *= opacity;\n"
"  o_final_color.a *= corner_sdf_t;\n"
"  o_final_color.a *= border_sdf_t;\n"
"}\n"
""
);

C_LINKAGE_END

#endif // RENDER_OPENGL_META_H
