// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

#define RGL_USE_LATEST_GL 0

/// Only run if condition is true
#if RGL_USE_LATEST_GL
  #define RGL_LATEST_GL(x) do { (x); } while (0);
#else
  #define RGL_LATEST_GL(x)
#endif // RGL_USE_LATEST_GL

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
DeclareArray(R_GLBufferArray, R_GLBufferArray);

/// A single stage of a shader
typedef struct R_GLShader R_GLShader;
struct R_GLShader
{
  OS_Guid id;
  U32 index;
  U32 handle;
  String8 name;
  /// The type of shader stage, ie 'GL_FRAGMENT_SHADER'
  U32 kind;
  /// A source string to include because GLSL is fun and doesn't support includes.
  String8 source_include;
  /// A source string to be compiled
  String8 source;
  /// Specifies if 'source_include' and 'source' are actually filenames
  B32 source_file;
  /// Hints if there is a newer version of this shader
  B32 stale;
  B32 ready;
};
DeclareArray(R_GLShaderArray, R_GLShader);
DeclareArray(R_GLShaderPointerArray, R_GLShader*);

/// A whole program/pipeline of linked shader units
typedef struct R_GLPipeline R_GLPipeline;
struct R_GLPipeline
{
  // Zero ID means object is uninitialized
  OS_Guid id;
  U32 index;
  U32 handle;
  String8 name;
  R_GLShaderPointerArray attached_shaders;
  /// Hints if there is a newer version of this shader pipeline
  B32 stale;
  /// If this pipeline is ready to be used in rendering
  B32 ready;
};
DeclareArray(R_GLPipelineArray, R_GLPipeline);

typedef struct R_GLVertexArray R_GLVertexArray;
struct R_GLVertexArray
{
  /// Unique identifier that is stable between OS object regeneration
  OS_Guid id;
  // Internal location to storage array, may not be 100% stable at all times
  U32 index;
  // Internal OpenGL handle
  U32 handle;
  U8* data;
  U32 vertex_components;
  U32 format;
  B32 normalized;
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
  /// The pixel formaat of 'data'
  R_Tex2DFormat format;
  /// How to store the data on the GPU side
  R_Tex2DFormat format_internal;
  // A hint to OpenGL on how the data will be accessed from the GPU side
  U32 usage_pattern;
  String8 name;
  // The name a file to read the texture from, if it exists
  String8 source_file;
};
DeclareArray(R_GLTextureArray, R_GLTexture);

typedef struct R_GLMesh R_GLMesh;
struct R_GLMesh
{
  OS_Guid id;
  U32 index;
  U32 handle;
  Vec3F32* vertecies;
  R_GLTexture texture;
  U32 gl_draw_mode;
};
DeclareArray(R_GLMeshArray, R_GLMesh);

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
  R_GLMeshArray meshes;
  R_GLShaderArray shaders;
  R_GLPipelineArray pipelines;

  R_GLPipeline* shader_rectangle;
  R_GLPipeline* shader_mesh;
  R_GLPipeline* shader_blur;
  R_GLPipeline* shader_geometry;
  R_GLPipeline* shader_composite;
  R_GLPipeline* shader_final;
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

internal B32 rgl_read_format_from_texture_format(U32* out_format, U32* out_type, R_Tex2DFormat format);
internal U32 rgl_internal_format_from_texture_format(R_Tex2DFormat format);
internal R_GLTexture* rgl_texture_from_handle(R_Handle texture);
internal R_Handle rgl_handle_from_texture(R_GLTexture* texture);
/// Clear OpenGL error list must clear errors _before calling an OpenGL function, return error count
internal U32 rgl_clear_errors();
#define RGL_CHECK_ERROR(x) \
    do { rgl_clear_errors(); (x); rgl_check_error( str8_lit(__FILE__), __LINE__ ); } while(0);

// Returns true if no errors
internal B32 rgl_check_error( String8 source_file, U32 source_line );
internal B32 rgl_shader_init(R_GLShader* shader);
internal B32 rgl_pipeline_init(R_GLPipeline* pipeline);
internal R_GLPipeline* rgl_pipeline_simple_create(String8 name, String8 source_include, String8 source_vertex, String8 source_fragment);

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
