
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

U32
rgl_clear_errors()
{
  U32 count = 0;
  while (glGetError() != GL_NO_ERROR) { count++; }
  glGetError();
  glGetError();
  glGetError();
  return count;
}

B32
rgl_check_error( String8 source_file, U32 source_line )
{
  char* error_message;
  GLenum error = glGetError();
  switch (error)
  {
    case GL_NO_ERROR:
      error_message = "GL_NO_ERROR"; break;
    case GL_INVALID_ENUM:
      error_message = "GL_INVALID_ENUM"; break;
    case GL_INVALID_VALUE:
      error_message = "GL_INVALID_VALUE"; break;
    case GL_INVALID_OPERATION:
      error_message = "GL_INVALID_OPERATION"; break;
    case GL_STACK_OVERFLOW:
      error_message = "GL_STACK_OVERFLOW"; break;
    case GL_STACK_UNDERFLOW:
      error_message = "GL_STACK_UNDERFLOW"; break;
    case GL_OUT_OF_MEMORY:
      error_message = "GL_OUT_OF_MEMORY"; break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      error_message = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
    case GL_CONTEXT_LOST:
      error_message = "GL_CONTEXT_LOST"; break;
    case GL_TABLE_TOO_LARGE:
      error_message = "GL_TABLE_TOO_LARGE"; break;
    default:
      error_message = "Unknown Error"; break;
  }
  if (error != GL_NO_ERROR)
  {
    printf("%s:%d: OpenGL Error Status: %s\n", source_file.str, source_line, error_message);
  }
  return error == GL_NO_ERROR;
}

B32
rgl_shader_init(R_GLShader* shader)
{
  return 0;
}

B32
rgl_pipeline_init(R_GLPipeline* pipeline)
{
  return 0;
}

R_GLPipeline*
rgl_pipeline_simple_create(String8 name,
                           String8 source_include,
                           String8 source_vertex,
                           String8 source_fragment)
{
  R_GLPipeline* pipeline = ArrayPushTail(&rgl.pipelines, NULL);
  pipeline->index = rgl.pipelines.head_size - 1;
  R_GLShader* shader1 = ArrayPushTail(&rgl.shaders, NULL);
  pipeline->index = rgl.shaders.head_size - 1;
  R_GLShader* shader2 = ArrayPushTail(&rgl.shaders, NULL);
  pipeline->index = rgl.shaders.head_size - 1;
  ArrayAllocate(&pipeline->attached_shaders, rgl.arena, 2);

  pipeline->name = name;
  shader1->name = name;
  shader1->source_include = source_include;
  shader1->source = source_vertex;
  shader1->kind = GL_VERTEX_SHADER;

  shader2->name = name;
  shader2->source_include = source_include;
  shader2->source = source_fragment;
  shader2->kind = GL_FRAGMENT_SHADER;

  pipeline->attached_shaders.data[0] = shader1;
  pipeline->attached_shaders.data[1] = shader2;
  return pipeline;
}

B32
rgl_pipeline_refresh(R_GLPipeline* pipeline)
{
  // -- Interface checking --
  Assert(pipeline->name.size);
  B32 pipeline_uninitialized = (pipeline->id.data1 == 0);
  if (pipeline_uninitialized)
  {
    pipeline->id = os_make_guid();
    pipeline->handle = gl.CreateProgram();
    Assert(pipeline->handle != 0); // Error creating program
    for (int i=0; i<pipeline->attached_shaders.head_size; ++i )
    {
      gl.AttachShader(pipeline->handle, pipeline->attached_shaders.data[i]->handle);
    }
    gl.LinkProgram(pipeline->handle);
  }
  return 1;
}

B32
rgl_shader_refresh(R_GLShader* shader)
{
  // -- Interface checking --
  Assert(shader->name.size);
  Assert(shader->source.size);
  Temp scratch = scratch_begin(0, 0);
  String8 debug_name = {0};
  String8 source = push_str8_cat(scratch.arena, shader->source_include, shader->source);

  // -- New Shader Initialize Pathway --
  B32 shader_uninitialized = (shader->id.data1 == 0);
  if (shader_uninitialized)
  {
    shader->id = os_make_guid();
    RGL_CHECK_ERROR(shader->handle = gl.CreateShader(shader->kind));
    if (glIsShader(shader->handle) == 0) { return 0; }
    /* Assert(shader->handle != 0); // Error creating shader */
    switch (shader->kind)
    {
      case GL_VERTEX_SHADER:
        debug_name = push_str8_cat(scratch.arena, str8_lit("vs_"), shader->name); break;
      case GL_FRAGMENT_SHADER:
        debug_name = push_str8_cat(scratch.arena, str8_lit("fs_"), shader->name); break;
      case GL_GEOMETRY_SHADER:
        debug_name = push_str8_cat(scratch.arena, str8_lit("gs_"), shader->name); break;
      case GL_COMPUTE_SHADER:
        debug_name = push_str8_cat(scratch.arena, str8_lit("cs_"), shader->name); break;
      case GL_TESS_CONTROL_SHADER:
        debug_name = push_str8_cat(scratch.arena, str8_lit("tcs_"), shader->name); break;
      case GL_TESS_EVALUATION_SHADER:
        debug_name = push_str8_cat(scratch.arena, str8_lit("tes_"), shader->name); break;
      default:
        debug_name = push_str8_cat(scratch.arena, str8_lit("s_"), shader->name); break;
    }
    RGL_LATEST_GL( glObjectLabel(GL_SHADER, shader->handle, shader->name.size, shader->name.str) )
    S32 string_size = (S32)source.size;
    char* string_list[] = { (char*)source.str };
    gl.ShaderSource(shader->handle, 1, (const char**)string_list, &string_size);
    RGL_CHECK_ERROR(gl.CompileShader(shader->handle));
  }
  shader->ready = 1;
  scratch_end(scratch);
  return 1;
}

// -- Public API Functions

//- rjf: top-level layer initialization
r_hook void
r_init(CmdLine *cmdln)
{
  // -- Initalize basics --
  U32 rgl_pipeline_limit = 100;
  U32 rgl_shader_limit = 300;
  rgl.arena = arena_alloc();
  rgl.object_limit = 1000;
  rgl.buffer_ids = push_array(rgl.arena, U32, rgl.object_limit);
  rgl.vertex_ids = push_array(rgl.arena, U32, rgl.object_limit);
  rgl.texture_ids = push_array(rgl.arena, U32, rgl.object_limit);
  ArrayAllocate(&rgl.buffers, rgl.arena, rgl.object_limit);
  ArrayAllocate(&rgl.vertex_arrays, rgl.arena, rgl.object_limit);
  ArrayAllocate(&rgl.textures, rgl.arena, rgl.object_limit);
  ArrayAllocate(&rgl.meshes, rgl.arena, rgl.object_limit);
  ArrayAllocate(&rgl.shaders, rgl.arena, rgl_shader_limit);
  ArrayAllocate(&rgl.pipelines, rgl.arena, rgl_pipeline_limit);

  // -- Load dynamic function pointers --
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

  // -- Setup buffers, textures and shaders --
  rgl.shader_rectangle = rgl_pipeline_simple_create(
    str8_lit("generic_rectangle"), rgl_rect_common_src, rgl_rect_vs_src, rgl_rect_fs_src);
  rgl.shader_blur = rgl_pipeline_simple_create(
    str8_lit("generic_blur"), rgl_blur_common_src, rgl_blur_vs_src, rgl_blur_fs_src);
  rgl.shader_mesh = rgl_pipeline_simple_create(
    str8_lit("generic_mesh"), rgl_mesh_common_src, rgl_mesh_vs_src, rgl_mesh_fs_src);
  rgl.shader_composite = rgl_pipeline_simple_create(
    str8_lit("mesh_composite"), rgl_mesh_common_src, rgl_mesh_vs_src, rgl_mesh_fs_src);
  rgl.shader_final = rgl_pipeline_simple_create(
    str8_lit("finalize"), rgl_finalize_common_src, rgl_finalize_vs_src, rgl_finalize_fs_src);

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
#if OS_LINUX
  GFX_LinuxWindow* _window = gfx_window_from_handle(window);
  B32 switch_result = eglMakeCurrent(gfx_egl_display, _window->first_surface,
                                     _window->first_surface, gfx_egl_context);
  Assert(switch_result == EGL_TRUE);
  static B32 first_run = 1;
  if (first_run)
  {
    printf("OpenGL Implementation Vendor: %s \n\
OpenGL Renderer String: %s \n\
OpenGL Version: %s \n\
OpenGL Shading Language Version: %s \n", gl.GetString( GL_VENDOR ), glGetString(GL_RENDERER),
           glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    first_run = 0;
  }
#endif
}
r_hook void
r_window_end_frame(OS_Handle window, R_Handle window_equip)
{
  Temp scratch = scratch_begin(0,0);
  U8* buffer = push_array(scratch.arena, U8, 1920*1080*4);
  Vec4F32 dark_magenta = vec_4f32( 0.2f, 0.f, 0.2f, 1.0f );
#if OS_LINUX
  GFX_LinuxWindow* _window = gfx_window_from_handle(window);

  static B32 regenerate_objects = 1;
  if (regenerate_objects)
  {
    // Setup Objects and Compile Shaders
    for (int i=0; i<rgl.shaders.head_size; ++i)
    {
      rgl_shader_refresh(rgl.shaders.data + i);
    }
    for (int i=0; i<rgl.pipelines.head_size; ++i)
    {
      rgl_pipeline_refresh(rgl.pipelines.data + i);
    }
    regenerate_objects = 0;
  }
// TEMPORARY
  static U32 tmp_texture;
  tmp_texture = rgl.texture_ids[99];
  // Temporary
  gl.BindTexture(GL_TEXTURE_2D, tmp_texture);
  gl.TexImage2D(GL_TEXTURE_2D,
                0,
                GL_RGBA,
                1920,
                1080,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                buffer);

  U32 framebuffer;
  U32 vao;
  gl.GenFramebuffers(1, &framebuffer);
  gl.GenVertexArrays(1, &vao);
/* ld::glBufferData( GL_UNIFORM_BUFFER,
                         contents.size ,
                         contents.data(),
                         GL_STATIC_DRAW ); */
  gl.BindVertexArray(vao);
  RGL_CHECK_ERROR(gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer));
  RGL_CHECK_ERROR(gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                          tmp_texture, 0));
  unsigned int rbo;
  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1920, 1080);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  RGL_CHECK_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                            GL_RENDERBUFFER, rbo));

  B32 framebuffer_success = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

  // Clear framebuffer to global draw buffer
  gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
  MemorySet(buffer, 0xFF, 1920*1080*4);
  glClearColor(dark_magenta.x, dark_magenta.y, dark_magenta.z, dark_magenta.w);
  glClear(GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glDrawPixels(1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, buffer);


  // Clear binding to reduce bug severity
  gl.BindTexture(GL_TEXTURE_2D, 0);
  gl.UseProgram(rgl.shader_rectangle->handle);

  // Enable vsync
  S32 vsync_result = eglSwapInterval(gfx_egl_display, 1);
  S32 swap_result = eglSwapBuffers(gfx_egl_display, _window->first_surface);
  glFlush();
  glFinish();

#elif OS_WINDOWS
  /* NOTE(mallchad): You can do wglSwapBuffers or whatever is relevant for any
     other relevant paltform this isn't seperated into a platform specific
     function or file is because this part will literally be the equivilent of
     like a 3 line difference or so and I didn't want to change the API yet. */
  glClearColor(dark_magenta.x, dark_magenta.y, dark_magenta.z, dark_magenta.w);
  glClear(GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#endif // OS_LINUX / OS_Windows

  // Cleanup
  fflush(stdout);
  scratch_end(scratch);
}

//- rjf: render pass submission
r_hook void
r_window_submit(OS_Handle window, R_Handle window_equip, R_PassList *passes)
{

}
