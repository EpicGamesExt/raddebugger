
#include "generated/render_opengl.meta.h"
#include "generated/render_opengl.meta.c"

global R_GLContext rgl = {0};
global R_GLProcFunctions gl = {0};
R_GLTexture rgl_texture_stub = {0};

// NOTE(mallchad): This is Linux specific, whatever, we can think about making it agnostic later.
// Or not...

// -- Internal Functions --

R_Handle
rgl_handle_from_texture(R_GLTexture* texture)
{
  R_Handle result = {0};
  MemoryCopyStruct(&result, &texture->id);
  return result;
}

R_GLTexture*
rgl_texture_from_handle(R_Handle texture)
{
  for (int i=0; i<rgl.textures.head_size; ++i)
  {
    if (MemoryCompare(&rgl.textures.data[i].id, &texture, sizeof(OS_Guid)))
    { return i+ rgl.textures.data; }
  }
  return &rgl_texture_stub;
}

U32
rgl_internal_format_from_texture_format(R_Tex2DFormat format)
{
  return rgl_texture_formats[ format ];
}

// Convert R_Tex2DFormat
B32
rgl_read_format_from_texture_format(U32* out_format, U32* out_type, R_Tex2DFormat format)
{
  switch (format)
  {
    case R_Tex2DFormat_R8: *out_format = GL_RED;
    case R_Tex2DFormat_RG8: *out_format = GL_RG;
    case R_Tex2DFormat_RGBA8: *out_format = GL_RGBA;
    case R_Tex2DFormat_BGRA8: *out_format = GL_BGRA;
      *out_type = GL_UNSIGNED_BYTE;
      break;
    case R_Tex2DFormat_R16: *out_format = GL_RED;
    case R_Tex2DFormat_RGBA16: *out_format = GL_RGBA;
      *out_type = GL_UNSIGNED_SHORT;
      break;
    case R_Tex2DFormat_R32: *out_format = GL_RED;
    case R_Tex2DFormat_RG32: *out_format = GL_RG;
    case R_Tex2DFormat_RGBA32: *out_format = GL_RGBA;
      *out_type = GL_UNSIGNED_INT;
    default: return 0;
  }
  return 1;
}

// -- Public API Functions

//- rjf: top-level layer initialization
r_hook void
r_init(CmdLine *cmdln)
{
  // -- Initalize basics --
  rgl.arena = arena_alloc();
  rgl.object_limit = 1000;
  rgl.buffer_ids = push_array(rgl.arena, U32, rgl.object_limit);
  rgl.vertex_ids = push_array(rgl.arena, U32, rgl.object_limit);
  rgl.texture_ids = push_array(rgl.arena, U32, rgl.object_limit);
  ArrayAllocate(&rgl.buffers, rgl.arena, rgl.object_limit);
  ArrayAllocate(&rgl.vertex_arrays, rgl.arena, rgl.object_limit);
  ArrayAllocate(&rgl.textures, rgl.arena, rgl.object_limit);

  // Load dynamic function pointers
  void* func_ptr = NULL;
  for (int i=0; i<ArrayCount(rgl_function_names); ++i)
  {
#if OS_LINUX
    func_ptr = eglGetProcAddress(rgl_function_names[i]);
#elif OS_WINDOWS
    func_ptr = wglGetProcAddress(rgl_function_names[i]);
#endif // OS_LINUX
    Assert(func_ptr != NULL); // OpenGL doesn't report *at all* if it got a bad pointer but check anyway
    gl._pointers[i] = func_ptr;
  }

  // -- Initialize assorted OpenGL stuff --

  // Generate guranteed usableobject names/ID's
  gl.GenBuffers(rgl.object_limit, rgl.buffer_ids);
  gl.GenVertexArrays(rgl.object_limit, rgl.vertex_ids);
  gl.GenTextures(rgl.object_limit, rgl.texture_ids);
}

//- rjf: window setup/teardown
r_hook R_Handle
r_window_equip(OS_Handle window)
{
  R_Handle result = {0};
  return result;
}
r_hook void
r_window_unequip(OS_Handle window, R_Handle window_equip)
{

}

//- rjf: textures
r_hook R_Handle
r_tex2d_alloc(R_ResourceKind kind, Vec2S32 size, R_Tex2DFormat format, void *data)
{
  R_GLTexture tex = {0};
  tex.id = os_make_guid();
  tex.index = rgl.textures.head_size;
  tex.handle = rgl.texture_ids[ tex.index ];
  tex.data = data;
  tex.size = size;
  tex.format = format;
  tex.format_internal = format;
  tex.usage_pattern = GL_STATIC_DRAW;

  U32 gl_format = 0;
  U32 gl_type = 0;
  rgl_read_format_from_texture_format(&gl_format, &gl_type, tex.format);
  gl.BindTexture(GL_TEXTURE_2D, tex.handle);
  gl.TexImage2D(GL_TEXTURE_2D,
                0,
                gl_format,
                tex.size.x,
                tex.size.y,
                0,
                rgl_internal_format_from_texture_format(tex.format_internal),
                gl_type,
                tex.data);
  // Clear binding to reduce bug severity
  gl.BindTexture(GL_TEXTURE_2D, tex.handle);

  ArrayPushTail(&rgl.textures, &tex);
  return rgl_handle_from_texture(&tex);
}

r_hook void
r_tex2d_release(R_Handle texture)
{

}

r_hook R_ResourceKind
r_kind_from_tex2d(R_Handle texture)
{
  R_ResourceKind result = 0;
  NotImplemented;
  return result;
}

r_hook Vec2S32
r_size_from_tex2d(R_Handle texture)
{
  R_GLTexture* _texture = rgl_texture_from_handle(texture);
  return _texture->size;
}

r_hook R_Tex2DFormat
r_format_from_tex2d(R_Handle texture)
{
  R_Tex2DFormat result = 0;
  NotImplemented;
  return result;
}

r_hook void
r_fill_tex2d_region(R_Handle texture, Rng2S32 subrect, void *data)
{

}

//- rjf: buffers
r_hook R_Handle
r_buffer_alloc(R_ResourceKind kind, U64 size, void *data)
{
  R_Handle result = {0};
  return result;
}

r_hook void
r_buffer_release(R_Handle buffer)
{

}

//- rjf: frame markers
r_hook void
r_begin_frame(void)
{

}

r_hook void
r_end_frame(void)
{

}

r_hook void
r_window_begin_frame(OS_Handle window, R_Handle window_equip)
{

}
r_hook void
r_window_end_frame(OS_Handle window, R_Handle window_equip)
{
  Vec4F32 dark_magenta = vec_4f32( 0.2f, 0.f, 0.2f, 1.0f );
#if OS_LINUX
  GFX_LinuxWindow* _window = gfx_window_from_handle(window);

  eglMakeCurrent(gfx_egl_display, _window->first_surface, _window->first_surface, gfx_egl_context);
  /* glBindFramebuffer(GL_FRAMEBUFFER, 0); */
  glClearColor(dark_magenta.x, dark_magenta.y, dark_magenta.z, dark_magenta.w);
  glClear( GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

  // Enable vsync
  S32 vsync_result = eglSwapInterval(gfx_egl_display, 1);
  S32 swap_result = eglSwapBuffers(gfx_egl_display, _window->first_surface);

#elif OS_WINDOWS
  /* NOTE(mallchad): You can do wglSwapBuffers or whatever is relevant for any
     other relevant paltform this isn't seperated into a platform specific
     function or file is because this part will literally be the equivilent of
     like a 3 line difference or so and I didn't want to change the API yet. */
  glClearColor(dark_magenta.x, dark_magenta.y, dark_magenta.z, dark_magenta.w);
  glClear( GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
#endif // OS_LINUX / OS_Windows
}

//- rjf: render pass submission
r_hook void
r_window_submit(OS_Handle window, R_Handle window_equip, R_PassList *passes)
{

}
