// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

typedef struct R_GLBuffer R_GLBuffer;
struct R_GLBuffer
{
  /// Unique identifier that is stable between OS object regeneration
  OS_Guid id;
  // Internal location to storage array, may not be 100% stable at all times
  U32 index;
  // Internal OpenGL handle
  U32 handle;
  /// The Buffer Binding Target type, ie 'GL_ARRAY_BUFFER'
  U32 kind;
  /// Denotes if the object is using a valid OpenGL buffer, effectively an "active" tag
  B32 bound;
  String8 name;
};

typedef struct R_GLProgram R_GLProgram;
struct R_GLProgram
{
 /// Unique identifier that is stable between OS object regeneration
  OS_Guid id;
  // Internal location to storage array, may not be 100% stable at all times
  U32 index;
  // Internal OpenGL handle
  U32 handle;
  String8 name;
};

/// A single stage of a shader
typedef struct R_GLShader R_GLShader;;
struct R_GLShader
{
  OS_Guid id;
  U32 index;
  U32 handle;
  /// The type of shader stage, ie 'GL_FRAGMENT_SHADER'
  U32 kind;
  U32 usage_pattern;
  String8 name;
  String8 source;
  B32 source_file;
};
DeclareArray(R_GLBufferArray, R_GLBufferArray);

typedef struct R_GLVertexArray R_GLVertexArray;
struct R_GLVertexArray
{
  /// Unique identifier that is stable between OS object regeneration
  OS_Guid id;
  // Internal location to storage array, may not be 100% stable at all times
  U32 index;
  // Internal OpenGL handle
  U32 handle;
};
DeclareArray(R_GLVertexArrayArray, R_GLVertexArray);

typedef struct R_GLTexture R_GLTexture;
struct R_GLTexture
{
  OS_Guid id;
  U32 index;
  U32 handle;
  void* data;
  Vec2S32 size;
  /// How 'data' is formatted
  R_Tex2DFormat format;
  /// How to store the data on the GPU side
  R_Tex2DFormat format_internal;
  U32 usage_pattern;
  String8 name;
  String8 source;
};
DeclareArray(R_GLTextureArray, R_GLTexture);

typedef struct R_GLContext R_GLContext;
struct R_GLContext
{
  Arena* arena;
  /// Number of various ID's and array sizes to use
  U32 object_limit;
  /// Internal OpenGL generated GPU proxy buffers ID's
  U32* buffer_ids;
  /// Internal OpenGl generated texture ID's
  U32* texture_ids;
  /// Internal OpenGL generated logical vertex array ID's
  U32* vertex_ids;
  /// GPU proxy buffers
  R_GLBufferArray buffers;
  R_GLVertexArrayArray vertex_arrays;
  R_GLTextureArray textures;
};

/** Conversion table between R_OGL_Tex2DFormat to OpenGL internalformat
    I suffix -> integer
    UI suffix -> unsigned integar
    F suffix -> float 8 */
U32 rgl_texture_formats[] =
{
  GL_R8UI,
  GL_RG8UI,
  GL_RGBA8UI,
  0, // BGRA not a valid format but can be used for internal representation,
  GL_R16UI,
  GL_RGBA16UI,
  GL_R32UI,
  GL_RG32UI,
  GL_RGBA32UI
};

//- rjf: top-level layer initialization
r_hook void              r_init(CmdLine *cmdln);

//- rjf: window setup/teardown
r_hook R_Handle          r_window_equip(OS_Handle window);
r_hook void              r_window_unequip(OS_Handle window, R_Handle window_equip);

//- rjf: textures
r_hook R_Handle          r_tex2d_alloc(R_ResourceKind kind, Vec2S32 size, R_Tex2DFormat format, void *data);
r_hook void              r_tex2d_release(R_Handle texture);
r_hook R_ResourceKind    r_kind_from_tex2d(R_Handle texture);
r_hook Vec2S32           r_size_from_tex2d(R_Handle texture);
r_hook R_Tex2DFormat     r_format_from_tex2d(R_Handle texture);
r_hook void              r_fill_tex2d_region(R_Handle texture, Rng2S32 subrect, void *data);

//- rjf: buffers
r_hook R_Handle          r_buffer_alloc(R_ResourceKind kind, U64 size, void *data);
r_hook void              r_buffer_release(R_Handle buffer);

//- rjf: frame markers
r_hook void              r_begin_frame(void);
r_hook void              r_end_frame(void);
r_hook void              r_window_begin_frame(OS_Handle window, R_Handle window_equip);
r_hook void              r_window_end_frame(OS_Handle window, R_Handle window_equip);

//- rjf: render pass submission
r_hook void              r_window_submit(OS_Handle window, R_Handle window_equip, R_PassList *passes);


#endif // RENDER_OPENGL_H
