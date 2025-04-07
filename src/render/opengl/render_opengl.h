// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

////////////////////////////////
//~ dan: Generated Code

#include "generated/render_opengl.meta.h"


// Need R_GL_UniformTypeKind enum if not already included via render_core.h
// Assuming render_core.h includes generated/render.meta.h which defines R_PassKind etc.
// We will need a similar generation step for OpenGL specific types like UniformTypeKind if separate
// For now, let's manually define the enum count based on d3d11's mdesk.

// Uniform Binding Points
#define R_GL_UNIFORM_BINDING_POINT_RECT 0
#define R_GL_UNIFORM_BINDING_POINT_BLUR 1
#define R_GL_UNIFORM_BINDING_POINT_MESH 2
// ... add more if needed

//////////////////////////////////
//~ dan: C-side Shader Uniform Structures (Matching GLSL std140 layout)

// NOTE(Memory Layout): GLSL std140 layout rules:
// - Scalars (float, int, bool): aligned to 4 bytes.
// - vec2: aligned to 8 bytes.
// - vec3, vec4: aligned to 16 bytes.
// - mat3: column 0 aligned to 16, col 1 aligned to 16, col 2 aligned to 16 (treated as 3 vec4s essentially for alignment).
// - mat4: column 0 aligned to 16, col 1 aligned to 16, col 2 aligned to 16, col 3 aligned to 16.
// - Arrays: Each element aligned according to its type (as above), potentially padded up to vec4 alignment.
// - Structs: Aligned to the largest alignment of its members, potentially padded up to a multiple of 16 bytes.

// GLSL vec2 aligns to 8 bytes
// GLSL vec3/vec4 align to 16 bytes
// GLSL mat3 aligns as 3 vec4 columns (total 48 bytes)
// GLSL mat4 aligns as 4 vec4 columns (total 64 bytes)
// GLSL struct aligned to largest member alignment (often 16 bytes)

// Matches GLSL uniform block "R_GL_Uniforms_Rect"
typedef struct R_GL_Uniforms_Rect R_GL_Uniforms_Rect;
struct R_GL_Uniforms_Rect
{
  // Aligned(8)
  Vec2F32 viewport_size_px;        // offset 0, size 8
  // Aligned(4), needs padding after
  F32 opacity;                   // offset 8, size 4
  // --- Explicit std140 Padding ---
  F32 _padding0;                 // offset 12, size 4 (Ensure alignment for mat4)
  // Aligned(16) per column -> Mat4x4F32 is 4*16 = 64 bytes
  Vec4F32 texture_sample_channel_map[4]; // offset 16, size 64
  // Aligned(8)
  Vec2F32 texture_t2d_size_px;   // offset 80, size 8
  // --- Explicit std140 Padding ---
  Vec2F32 _padding1;             // offset 88, size 8 (Ensure alignment for mat3)
  // Aligned(16) per column -> Mat3x3F32 needs 3*16 = 48 bytes
  Vec4F32 xform[3];              // offset 96, size 48
  // Aligned(8)
  Vec2F32 xform_scale;           // offset 144, size 8
  // --- Explicit std140 Padding ---
  Vec2F32 _padding2;             // offset 152, size 8 (Ensure total size is multiple of 16, MUST be Vec2F32)
}; // Total size: 160 bytes
StaticAssert(sizeof(R_GL_Uniforms_Rect) == 160, R_GL_Uniforms_Rect_SizeCheck);

// Matches GLSL uniform block "R_GL_Uniforms_Blur"
typedef struct R_GL_Uniforms_Blur R_GL_Uniforms_Blur;
struct R_GL_Uniforms_Blur
{
  // Aligned(16)
  Vec4F32 rect;                  // offset 0
  // Aligned(16)
  Vec4F32 corner_radii_px;       // offset 16
  // Aligned(8)
  Vec2F32 direction;             // offset 32
  // Aligned(8)
  Vec2F32 viewport_size;         // offset 40
  // Aligned(4)
  U32 blur_count;              // offset 48 (Using U32 as GLSL bool might be int)
  // --- Explicit std140 Padding ---
  U32 _padding0[3];            // offset 52, size 12 (Ensure kernel starts at offset 64)
  // Aligned(16) per element
  Vec4F32 kernel[32];            // offset 64 (Array stride is 16)
}; // Total size: 64 + 32 * 16 = 576 bytes
StaticAssert(sizeof(R_GL_Uniforms_Blur) == 576, R_GL_Uniforms_Blur_SizeCheck);

// Matches GLSL uniform block "MeshUniforms"
// Name MUST match the GLSL block name for UBO binding to work.
typedef struct R_GL_Uniforms_Mesh R_GL_Uniforms_Mesh;
struct R_GL_Uniforms_Mesh
{
  // IMPORTANT: Remember layout(std140) rules for alignment/padding in GLSL.
  // Mat4x4F32 is typically 4x vec4.
  Mat4x4F32 view_proj_matrix; // Combined view * projection matrix, transposed for GLSL.
  // Instance transform is now passed via vertex attributes.
};

////////////////////////////////
//~ dan: Main State Types

typedef struct R_GL_Tex2D R_GL_Tex2D;
struct R_GL_Tex2D
{
  R_GL_Tex2D *next;
  U64 generation;
  GLuint texture_id;
  R_ResourceKind kind;
  Vec2S32 size;
  R_Tex2DFormat format;
};

typedef struct R_GL_Buffer R_GL_Buffer;
struct R_GL_Buffer
{
  R_GL_Buffer *next;
  U64 generation;
  GLuint buffer_id;
  R_ResourceKind kind;
  U64 size;
  GLenum target;
};

// Added helper structs for delayed deletion
typedef struct R_GL_Framebuffer R_GL_Framebuffer;
struct R_GL_Framebuffer
{
  R_GL_Framebuffer *next;
  GLuint fbo_id;
};

typedef struct R_GL_Renderbuffer R_GL_Renderbuffer;
struct R_GL_Renderbuffer
{
  R_GL_Renderbuffer *next;
  GLuint rbo_id;
};

typedef struct R_GL_Window R_GL_Window;
struct R_GL_Window
{
  R_GL_Window *next;
  U64 generation;

  // GL context related (might be managed by OS layer)
  // void *gl_context; // Example

  // Framebuffers (FBOs) & Attachments - Need cleanup on unequip/resize
  GLuint stage_fbo;
  GLuint stage_color_texture; // Texture used as color attachment
  GLuint stage_depth_texture; // Using texture for depth

  GLuint stage_scratch_fbo;
  GLuint stage_scratch_color_texture;

  GLuint geo3d_fbo;
  GLuint geo3d_color_texture;
  GLuint geo3d_depth_texture;

  // Last known state for resize detection
  Vec2S32 last_resolution;
};

typedef struct R_GL_ShaderProgram R_GL_ShaderProgram;
struct R_GL_ShaderProgram
{
  GLuint program_id;
  GLuint vertex_shader;
  GLuint fragment_shader;
  GLint main_texture_uniform_location; // Added for sampler uniforms
  // TODO(graphics): Add Geometry, Compute shader handles if needed
};

// Added: Structure to hold information for deferred texture allocations
typedef struct R_GL_DeferredTex2D R_GL_DeferredTex2D;
struct R_GL_DeferredTex2D
{
  R_GL_DeferredTex2D *next;
  R_ResourceKind kind;
  Vec2S32 size;
  R_Tex2DFormat format;
  void *data;          // Will need to copy this data
  U64 data_size;       // Track data size for allocation
  R_Handle result;     // Handle to return to caller early
  R_GL_Tex2D *texture; // Pre-allocated texture object
};

typedef struct R_GL_State R_GL_State;
struct R_GL_State
{
  Arena *arena;
  OS_Handle os_window_handle; // Added: Store the OS handle for context management
  Display *display; // Added for Linux: store the display pointer
  OS_Handle device_rw_mutex;

  // dan: resources
  R_GL_Window *first_free_window;
  R_GL_Tex2D *first_free_tex2d;
  R_GL_Buffer *first_free_buffer;
  R_GL_Framebuffer *first_free_framebuffer; // Added free list
  R_GL_Renderbuffer *first_free_renderbuffer; // Added free list

  // Queued Deletions (Processed in r_begin_frame)
  R_GL_Tex2D *first_to_free_tex2d;
  R_GL_Buffer *first_to_free_buffer;
  R_GL_Framebuffer *first_to_free_framebuffer; // Added deletion queue
  R_GL_Renderbuffer *first_to_free_renderbuffer; // Added deletion queue
  // Note: Window FBO attachment textures are deleted directly in resize/unequip queue logic

  Arena *buffer_flush_arena;
  R_Handle backup_texture;

  // Added: Queue for deferred texture allocations
  R_GL_DeferredTex2D *first_deferred_tex2d;
  R_GL_DeferredTex2D *last_deferred_tex2d;
  Arena *deferred_tex2d_arena;
  B32 has_valid_context;  // Flag to track if we have a valid OpenGL context

  // dan: pipeline state object cache
  GLuint rect_vao;
  GLuint mesh_vao;
  GLuint fullscreen_vao; // Added: VAO for fullscreen passes
  R_GL_ShaderProgram rect_shader;
  R_GL_ShaderProgram blur_shader;
  R_GL_ShaderProgram mesh_shader;
  R_GL_ShaderProgram geo3d_composite_shader;
  R_GL_ShaderProgram finalize_shader;
  GLuint samplers[R_Tex2DSampleKind_COUNT];
  GLuint uniform_type_kind_buffers[R_GL_UniformTypeKind_COUNT];
  U64    uniform_type_kind_sizes[R_GL_UniformTypeKind_COUNT]; // Added: store UBO sizes

  GLuint instance_vbo; // Shared VBO for instance data

  B32 glew_initialized; // Added: Track GLEW initialization separately
  B32 global_resources_initialized; // Renamed from opengl_extensions_initialized for clarity
  B32 create_context_arb_available; // Added: Flag for GLX context creation extension
};

////////////////////////////////
//~ dan: Globals

global R_GL_State *r_gl_state = 0;
// Define NIL handles robustly to avoid accidental use of address
global R_GL_Window r_gl_window_nil = { .next = &r_gl_window_nil };
global R_GL_Tex2D r_gl_tex2d_nil = { .next = &r_gl_tex2d_nil };
global R_GL_Buffer r_gl_buffer_nil = { .next = &r_gl_buffer_nil };

////////////////////////////////
//~ dan: Helpers

// Maps R_GeoTopologyKind to GLenum primitive type
internal GLenum r_gl_primitive_from_topology(R_GeoTopologyKind kind);

internal R_GL_Window *r_gl_window_from_handle(R_Handle handle);
internal R_Handle r_gl_handle_from_window(R_GL_Window *window);
internal R_GL_Tex2D *r_gl_tex2d_from_handle(R_Handle handle);
internal R_Handle r_gl_handle_from_tex2d(R_GL_Tex2D *texture);
internal R_GL_Buffer *r_gl_buffer_from_handle(R_Handle handle);
internal R_Handle r_gl_handle_from_buffer(R_GL_Buffer *buffer);
internal GLenum r_gl_usage_from_resource_kind(R_ResourceKind kind);
internal GLenum r_gl_format_from_tex2d_format(R_Tex2DFormat format);
internal GLint r_gl_internal_format_from_tex2d_format(R_Tex2DFormat format);
internal GLenum r_gl_type_from_tex2d_format(R_Tex2DFormat format); // Added
internal GLint r_gl_filter_from_sample_kind(R_Tex2DSampleKind kind);
internal void r_gl_check_error_line_file(const char *op, int line, char *file); // Added prototype
internal void r_gl_make_context_current(R_GL_Window *window); // Add this to fix declaration error
internal void r_gl_delete_window_fbo_resources(R_GL_Window *gl_window); // Add this too
internal void r_gl_debug_draw_test_pattern(void); // Add debug test pattern function
#define r_gl_check_error(op) r_gl_check_error_line_file(op, __LINE__, __FILE__) // Added define

// Buffer Update Helpers
internal void r_gl_buffer_update_sub_data(R_Handle handle, U64 offset, U64 size, void *data);
internal void* r_gl_buffer_map_range(R_Handle handle, U64 offset, U64 size, GLbitfield access_flags);
internal B32 r_gl_buffer_unmap(R_Handle handle);

// One-Time Initialization Helpers (Split)
internal void r_gl_init_glew_if_needed(void);
internal void r_gl_create_global_resources(void);
// Leave this function for backward compatibility
internal void r_gl_init_extensions_if_needed(void);

// Add new function declarations
internal void r_gl_process_deferred_tex2d_queue(void);
internal void r_gl_set_has_valid_context(B32 has_context);

// Debugging Helpers
internal void debug_log_current_context(void); // Add this
internal void r_debug_texture_info(GLuint texture_id); // Add declaration

#endif // RENDER_OPENGL_H