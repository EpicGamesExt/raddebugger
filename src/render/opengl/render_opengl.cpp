// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef RADDBG_LAYER_COLOR
#define RADDBG_LAYER_COLOR 0.80f, 0.10f, 0.20f

////////////////////////////////
//~ dmylo: Generated Code

#include "generated/render_opengl.meta.c"

////////////////////////////////
//~ dmylo: Helpers

// dmylo shortcut to access function pointers
#define gl r_ogl_state->gl_functions

internal R_OGL_Window *
r_ogl_window_from_handle(R_Handle handle)
{
  R_OGL_Window *window = (R_OGL_Window *)handle.u64[0];
  if(window->generation != handle.u64[1])
  {
    window = &r_ogl_window_nil;
  }
  return window;
}

internal R_Handle
r_ogl_handle_from_window(R_OGL_Window *window)
{
  R_Handle handle = {0};
  handle.u64[0] = (U64)window;
  handle.u64[1] = window->generation;
  return handle;
}

internal R_OGL_Tex2D *
r_ogl_tex2d_from_handle(R_Handle handle)
{
  R_OGL_Tex2D *texture = (R_OGL_Tex2D *)handle.u64[0];
  if(texture == 0 || texture->generation != handle.u64[1])
  {
    texture = &r_ogl_tex2d_nil;
  }
  return texture;
}

internal R_Handle
r_ogl_handle_from_tex2d(R_OGL_Tex2D *texture)
{
  R_Handle handle = {0};
  handle.u64[0] = (U64)texture;
  handle.u64[1] = texture->generation;
  return handle;
}

internal R_OGL_Buffer *
r_ogl_buffer_from_handle(R_Handle handle)
{
  R_OGL_Buffer *buffer = (R_OGL_Buffer *)handle.u64[0];
  if(buffer == 0 || buffer->generation != handle.u64[1])
  {
    buffer = &r_ogl_buffer_nil;
  }
  return buffer;
}

internal R_Handle
r_ogl_handle_from_buffer(R_OGL_Buffer *buffer)
{
  R_Handle handle = {0};
  handle.u64[0] = (U64)buffer;
  handle.u64[1] = buffer->generation;
  return handle;
}

internal void
r_ogl_upload_buffer(R_OGL_Buffer *buffer)
{
  GLenum usage = GL_STATIC_DRAW;
  {
    switch(buffer->kind)
    {
      default:
      case R_BufferKind_Static:
      {
        if(buffer->upload_data == 0)
        {
          usage = GL_DYNAMIC_DRAW;
        }
      }break;
      case R_BufferKind_Dynamic:
      {
        usage = GL_DYNAMIC_DRAW;
      }break;
    }
  }

  gl.GenBuffers(1, &buffer->buffer);
  gl.BindBuffer(GL_ARRAY_BUFFER, buffer->buffer);
  gl.BufferData(GL_ARRAY_BUFFER, buffer->size, buffer->upload_data, usage);
  gl.BindBuffer(GL_ARRAY_BUFFER, 0);
}

internal GLuint
r_ogl_instance_buffer_from_size(U64 size)
{
  GLuint buffer = r_ogl_state->instance_scratch_buffer_64kb;

  if(size > KB(64))
  {
    U64 flushed_buffer_size = size;
    flushed_buffer_size += MB(1)-1;
    flushed_buffer_size -= flushed_buffer_size%MB(1);

    // dmylo: build buffer
    {
      gl.GenBuffers(1, &buffer);
      gl.BindBuffer(GL_ARRAY_BUFFER, buffer);
      gl.BufferData(GL_ARRAY_BUFFER, flushed_buffer_size, 0, GL_STREAM_DRAW);
      gl.BindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // dmylo: push buffer to flush list
    R_OGL_FlushBuffer *n = push_array(r_ogl_state->buffer_flush_arena, R_OGL_FlushBuffer, 1);
    n->buffer = buffer;
    SLLQueuePush(r_ogl_state->first_buffer_to_flush, r_ogl_state->last_buffer_to_flush, n);
  }

  return buffer;
}

internal R_OGL_Tex2DFormat
r_ogl_tex2d_format(R_Tex2DFormat format)
{
  //- dmylo: format -> OpenGL format
  GLenum internal_format = GL_RGBA8;
  GLenum data_format = GL_UNSIGNED_BYTE;
  GLenum data_type = GL_UNSIGNED_BYTE;
  switch(format)
  {
    default:{}break;
    case R_Tex2DFormat_R8:     {internal_format = GL_R8;      data_format = GL_RED;  data_type = GL_UNSIGNED_BYTE; }break;
    case R_Tex2DFormat_RG8:    {internal_format = GL_RG8;     data_format = GL_RG;   data_type = GL_UNSIGNED_BYTE; }break;
    case R_Tex2DFormat_RGBA8:  {internal_format = GL_RGBA8;   data_format = GL_RGBA; data_type = GL_UNSIGNED_BYTE; }break;
    case R_Tex2DFormat_BGRA8:  {internal_format = GL_RGBA8;   data_format = GL_BGRA; data_type = GL_UNSIGNED_BYTE; }break;
    case R_Tex2DFormat_R16:    {internal_format = GL_R16;     data_format = GL_RED;  data_type = GL_UNSIGNED_SHORT;}break;
    case R_Tex2DFormat_RGBA16: {internal_format = GL_RGBA16;  data_format = GL_RGBA; data_type = GL_UNSIGNED_SHORT;}break;
    case R_Tex2DFormat_R32:    {internal_format = GL_R32F;    data_format = GL_RED;  data_type = GL_FLOAT;         }break;
    case R_Tex2DFormat_RG32:   {internal_format = GL_RG32F;   data_format = GL_RG;   data_type = GL_FLOAT;         }break;
    case R_Tex2DFormat_RGBA32: {internal_format = GL_RGBA32F; data_format = GL_RGBA; data_type = GL_FLOAT;         }break;
  }

  R_OGL_Tex2DFormat result = {0};
  result.internal_format = internal_format;
  result.data_format = data_format;
  result.data_type = data_type;

  return result;
}

internal void
r_ogl_upload_texture(R_OGL_Tex2D *texture)
{
  gl.GenTextures(1, &texture->texture);
  gl.BindTexture(GL_TEXTURE_2D, texture->texture);

  R_OGL_Tex2DFormat format = r_ogl_tex2d_format(texture->format);

  gl.TexImage2D(GL_TEXTURE_2D, 0, format.internal_format, texture->size.x, texture->size.y, 0, format.data_format, format.data_type, texture->upload_data);
  gl.BindTexture(GL_TEXTURE_2D, 0);
}

internal void
r_ogl_fill_tex2d_region(R_OGL_Tex2D* texture, Rng2S32 subrect, void *data)
{
  Vec2S32 dim = v2s32(subrect.x1 - subrect.x0, subrect.y1 - subrect.y0);

  R_OGL_Tex2DFormat format = r_ogl_tex2d_format(texture->format);

  gl.BindTexture(GL_TEXTURE_2D, texture->texture);
  gl.TexSubImage2D(GL_TEXTURE_2D, 0, subrect.x0, subrect.y0, dim.x, dim.y, format.data_format, format.data_type, data);
  gl.BindTexture(GL_TEXTURE_2D, 0);
}

internal GLuint
r_ogl_compile_shader(String8 common, String8 src, GLenum kind)
{
  GLuint shader = gl.CreateShader(kind);

  GLint src_sizes[2] = {
    (GLint)common.size,
    (GLint)src.size,
  };

  GLchar* src_ptrs[2] = {
    (GLchar*)common.str,
    (GLchar*)src.str,
  };

  gl.ShaderSource(shader, 2, src_ptrs, src_sizes);
  gl.CompileShader(shader);

  GLint success;
  gl.GetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
      char compile_log[4096];
      gl.GetShaderInfoLog(shader, ArrayCount(compile_log), 0, compile_log);
      os_graphical_message(1, str8_lit("Vertex Shader Compilation Failure"), str8_cstring(compile_log));
      return 0;
  }
  else
  {
    return shader;
  }
}

internal GLuint
r_ogl_link_shaders(GLuint vs, GLuint fs)
{
  GLuint program = gl.CreateProgram();

  gl.AttachShader(program, vs);
  gl.AttachShader(program, fs);
  gl.LinkProgram(program);

  GLint success;
  char compile_log[4096];
  gl.GetProgramiv(program, GL_LINK_STATUS, &success);
  if(!success)
  {
      char compile_log[4096];
      gl.GetProgramInfoLog(program, ArrayCount(compile_log), 0, compile_log);
      os_graphical_message(1, str8_lit("Vertex Shader Compilation Failure"), str8_cstring(compile_log));
      return 0;
  }
  else
  {
    return program;
  }
}

////////////////////////////////
//~ dmylo: Backend Hook Implementations

//- dmylo: top-level layer initialization
r_hook void
r_init(CmdLine *cmdln)
{
  Arena *arena = arena_alloc();
  r_ogl_state = push_array(arena, R_OGL_State, 1);
  r_ogl_state->arena = arena;
  r_ogl_state->device_rw_mutex = os_rw_mutex_alloc();

  r_ogl_state->initialized = false;
  r_ogl_state->gl_functions = {};

  //- dmylo: initialize buffer flush state
  {
    r_ogl_state->buffer_flush_arena = arena_alloc();
  }

  //- dmylo: initialize upload state :sync_upload
  {
    r_ogl_state->upload_arena = arena_alloc();
  }

  //- dmylo: Create a fake window to initialize an old OpenGL context
  // and retrieve the functions required to initialize a modern one.
  {
    HINSTANCE instance = GetModuleHandleA(NULL);

    WNDCLASSEXA wcex = {};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc = DefWindowProcA;
    wcex.hInstance = instance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = "DummyOpenGLWindowClass";
    RegisterClassExA(&wcex);

    HWND old_window = CreateWindowExA(0,
      "DummyOpenGLWindowClass", "old_dummy",  // window class, title
      WS_OVERLAPPEDWINDOW,                    // style
      CW_USEDEFAULT, CW_USEDEFAULT,           // position x, y
      1, 1,                                   // width, height
      NULL, NULL,                             // parent window, menu
      instance, NULL);                        // instance, param

    HDC old_dc = GetDC(old_window);

    PIXELFORMATDESCRIPTOR desired_format = {};
    desired_format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    desired_format.nVersion = 1;
    desired_format.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    desired_format.iPixelType = PFD_TYPE_RGBA;
    desired_format.cColorBits = 32;
    desired_format.cAlphaBits = 8;
    desired_format.cDepthBits = 24;
    desired_format.cStencilBits = 8;
    desired_format.iLayerType = PFD_MAIN_PLANE;

    int suggested_format_index = ChoosePixelFormat(old_dc, &desired_format);

    PIXELFORMATDESCRIPTOR suggested_format = {};
    DescribePixelFormat(old_dc, suggested_format_index, sizeof(PIXELFORMATDESCRIPTOR), &suggested_format);
    SetPixelFormat(old_dc, suggested_format_index, &suggested_format);

    HGLRC old_glrc = wglCreateContext(old_dc);
    BOOL ok = wglMakeCurrent(old_dc, old_glrc);

    r_ogl_state->wglCreateContextAttribsARB = (wgl_create_context_attribs_arb*)wglGetProcAddress("wglCreateContextAttribsARB");
    r_ogl_state->wglChoosePixelFormatARB = (wgl_choose_pixel_format_arb*)wglGetProcAddress("wglChoosePixelFormatARB");

    if(!ok) {
      char buffer[256] = {0};
      raddbg_snprintf(buffer, sizeof(buffer), "OpenGL fake context initialization failed (win32 error code: %d)", GetLastError());
      os_graphical_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
      os_exit_process(1);
    }

    // dmylo: Create an other dummy window with the modern context.
    // This will persist for the whole duration of the application.
    r_ogl_state->dummy_window = CreateWindowExA(0,
      "DummyOpenGLWindowClass", "modern_dummy", // window class, title
      WS_OVERLAPPEDWINDOW,                      // style
      CW_USEDEFAULT, CW_USEDEFAULT,             // position x, y
      1, 1,                                     // width, height
      NULL, NULL,                               // parent window, menu
      instance, NULL);                          // instance, param

    r_ogl_state->dummy_window_dc = GetDC(r_ogl_state->dummy_window);

#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013
#define WGL_COLOR_BITS_ARB                      0x2014
#define WGL_ALPHA_BITS_ARB                      0x201B
#define WGL_DEPTH_BITS_ARB                      0x2022
#define WGL_STENCIL_BITS_ARB                    0x2023
#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_SAMPLE_BUFFERS_ARB                  0x2041
#define WGL_SAMPLES_ARB                         0x2042

    // dmylo: set the pixel format of the new window, this is what
    // every future window will also use.
    const int pixel_attributes[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_ALPHA_BITS_ARB,     8,
        WGL_COLOR_BITS_ARB,     32,
        WGL_DEPTH_BITS_ARB,     24,
        WGL_STENCIL_BITS_ARB,   8,
        // WGL_SAMPLE_BUFFERS_ARB, 0, // Number of buffers
        // WGL_SAMPLES_ARB, 1,        // Number of samples
        0
    };

    UINT num_suggestions;
    r_ogl_state->wglChoosePixelFormatARB(r_ogl_state->dummy_window_dc, pixel_attributes, 0, 1, &suggested_format_index, &num_suggestions);
    DescribePixelFormat(r_ogl_state->dummy_window_dc, suggested_format_index, sizeof(PIXELFORMATDESCRIPTOR), &suggested_format);
    SetPixelFormat(r_ogl_state->dummy_window_dc, suggested_format_index, &suggested_format);

    // dmylo: save pixel format information for future windows
    r_ogl_state->pixel_format = suggested_format;
    r_ogl_state->pixel_format_index = suggested_format_index;

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x0001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

    // dmylo: create a modern context
    int attribs[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 1,
        WGL_CONTEXT_FLAGS_ARB,
        // WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB disable stuff prior to 3.0
        /*WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB |*/ WGL_CONTEXT_DEBUG_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB,
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    r_ogl_state->glrc = r_ogl_state->wglCreateContextAttribsARB(r_ogl_state->dummy_window_dc, 0, attribs);
    ok = wglMakeCurrent(r_ogl_state->dummy_window_dc, r_ogl_state->glrc);
    if(!ok) {
      char buffer[256] = {0};
      raddbg_snprintf(buffer, sizeof(buffer), "OpenGL modern context initialization failed (win32 error code: %d)", GetLastError());
      os_graphical_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
      os_exit_process(1);
    }

    //- dmylo: Now that we have a modern context, cleanup the old context and the old window.
    wglDeleteContext(old_glrc);
    ReleaseDC(old_window, old_dc);
    DestroyWindow(old_window);
  }

  //-dmylo: Load OpenGL function and initialize state for the various passes
  {
    // dmylo: Load function pointers.
    HMODULE opengl_module = LoadLibraryA("opengl32.dll");
    for (U64 i = 0; i < ArrayCount(r_ogl_g_function_names); i++)
    {
      void* ptr = wglGetProcAddress(r_ogl_g_function_names[i]);
      if(!ptr) {
        // dmylo: older functions are still in opengl32, so we also try that.
        ptr = GetProcAddress(opengl_module, r_ogl_g_function_names[i]);
      }
      if(!ptr) {
        // dmylo: if still not found error out.
        char buffer[256] = {0};
        raddbg_snprintf(buffer, sizeof(buffer), "Failed to load OpenGL function: %s", r_ogl_g_function_names[i]);
        os_graphical_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
        os_exit_process(1);
      }
      r_ogl_state->gl_functions._pointers[i] = ptr;
    }

    //- dmylo: UI pass
    {
      // buffers
      gl.GenBuffers(1, &r_ogl_state->instance_scratch_buffer_64kb);
      gl.BindBuffer(GL_ARRAY_BUFFER, r_ogl_state->instance_scratch_buffer_64kb);

      // vao
      gl.GenVertexArrays(1, &r_ogl_state->rect_vao);

      // shaders
      GLuint rect_vs = r_ogl_compile_shader(r_ogl_g_rect_common_src, r_ogl_g_rect_vs_src, GL_VERTEX_SHADER);
      GLuint rect_fs = r_ogl_compile_shader(r_ogl_g_rect_common_src, r_ogl_g_rect_fs_src, GL_FRAGMENT_SHADER);
      r_ogl_state->rect_shader = r_ogl_link_shaders(rect_vs, rect_fs);

      // uniforms
      gl.GenBuffers(1, &r_ogl_state->rect_uniform_buffer);
      gl.BindBuffer(GL_UNIFORM_BUFFER, r_ogl_state->rect_uniform_buffer);
      r_ogl_state->rect_uniform_block_index = gl.GetUniformBlockIndex(r_ogl_state->rect_shader, "Globals");
    }

    //- dmylo: Blur pass
    {
      // shaders
      GLuint blur_vs = r_ogl_compile_shader(r_ogl_g_blur_common_src, r_ogl_g_blur_vs_src, GL_VERTEX_SHADER);
      GLuint blur_fs = r_ogl_compile_shader(r_ogl_g_blur_common_src, r_ogl_g_blur_fs_src, GL_FRAGMENT_SHADER);
      r_ogl_state->blur_shader = r_ogl_link_shaders(blur_vs, blur_fs);

      // uniforms
      gl.GenBuffers(1, &r_ogl_state->blur_uniform_buffer);
      gl.BindBuffer(GL_UNIFORM_BUFFER, r_ogl_state->blur_uniform_buffer);
      r_ogl_state->blur_uniform_block_index = gl.GetUniformBlockIndex(r_ogl_state->blur_shader, "Globals");
      r_ogl_state->blur_direction_uniform_location = gl.GetUniformLocation(r_ogl_state->blur_shader, "u_direction");
    }

    //- dmylo: Geo3D pass
    {
      // vao
      gl.GenVertexArrays(1, &r_ogl_state->geo3d_vao);

      // geo shaders
      GLuint geo3d_vs = r_ogl_compile_shader(r_ogl_g_mesh_common_src, r_ogl_g_mesh_vs_src, GL_VERTEX_SHADER);
      GLuint geo3d_fs = r_ogl_compile_shader(r_ogl_g_mesh_common_src, r_ogl_g_mesh_fs_src, GL_FRAGMENT_SHADER);
      r_ogl_state->geo3d_shader = r_ogl_link_shaders(geo3d_vs, geo3d_fs);

      // uniforms
      r_ogl_state->geo3d_uniform_location = gl.GetUniformLocation(r_ogl_state->geo3d_shader, "xform");

      // composite shaders
      GLuint geo3dcomposite_vs = r_ogl_compile_shader(r_ogl_g_geo3dcomposite_common_src, r_ogl_g_geo3dcomposite_vs_src, GL_VERTEX_SHADER);
      GLuint geo3dcomposite_fs = r_ogl_compile_shader(r_ogl_g_geo3dcomposite_common_src, r_ogl_g_geo3dcomposite_fs_src, GL_FRAGMENT_SHADER);
      r_ogl_state->geo3dcomposite_shader = r_ogl_link_shaders(geo3dcomposite_vs, geo3dcomposite_fs);
    }

    //- dmylo: Finalize
    {
      GLuint finalize_vs = r_ogl_compile_shader(r_ogl_g_finalize_common_src, r_ogl_g_finalize_vs_src, GL_VERTEX_SHADER);
      GLuint finalize_fs = r_ogl_compile_shader(r_ogl_g_finalize_common_src, r_ogl_g_finalize_fs_src, GL_FRAGMENT_SHADER);
      r_ogl_state->finalize_shader = r_ogl_link_shaders(finalize_vs, finalize_fs);
    }

    //- dmylo: backup texture
    {
      U32 backup_texture_data[] =
      {
        0xff00ffff, 0x330033ff,
        0x330033ff, 0xff00ffff,
      };
      r_ogl_state->backup_texture = r_tex2d_alloc(R_Tex2DKind_Static, v2s32(2, 2), R_Tex2DFormat_RGBA8, backup_texture_data);
    }
  }
}

//- dmylo: window setup/teardown
r_hook R_Handle
r_window_equip(OS_Handle handle)
{
  ProfBeginFunction();
  R_Handle result = {0};

  bool just_initialized = false;
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    //- dmylo: allocate per-window-state
    R_OGL_Window *window = r_ogl_state->first_free_window;
    {
      if(window == 0)
      {
        window = push_array(r_ogl_state->arena, R_OGL_Window, 1);
      }
      else
      {
        U64 gen = window->generation;
        SLLStackPop(r_ogl_state->first_free_window);
        MemoryZeroStruct(window);
        window->generation = gen;
      }
      window->generation += 1;
    }

    //- dmylo: map os window handle -> hwnd
    HWND hwnd = {0};
    {
      W32_Window *w32_layer_window = w32_window_from_os_window(handle);
      hwnd = w32_hwnd_from_window(w32_layer_window);
    }

    //- dmylo: set pixel format on this window.
    window->dc = GetDC(hwnd);
    SetPixelFormat(window->dc, r_ogl_state->pixel_format_index, &r_ogl_state->pixel_format);

    result = r_ogl_handle_from_window(window);
  }

  ProfEnd();
  return result;
}

r_hook void
r_window_unequip(OS_Handle window_handle, R_Handle equip_handle)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    // dmylo: map os window handle -> hwnd
    HWND hwnd = {0};
    {
      W32_Window *w32_layer_window = w32_window_from_os_window(window_handle);
      hwnd = w32_hwnd_from_window(w32_layer_window);
    }

    R_OGL_Window *window = r_ogl_window_from_handle(equip_handle);
    window->generation += 1;

    // dmylo: free buffers
    if(window->stage_scratch_fbo)   { gl.DeleteFramebuffers(1, &window->stage_scratch_fbo); }
    if(window->stage_scratch_color) { gl.DeleteTextures(1, &window->stage_scratch_color); }
    if(window->stage_fbo)           { gl.DeleteFramebuffers(1, &window->stage_scratch_fbo); }
    if(window->stage_color)         { gl.DeleteTextures(1, &window->stage_color); }
    if(window->geo3d_fbo)           { gl.DeleteFramebuffers(1, &window->stage_scratch_fbo); }
    if(window->geo3d_color)         { gl.DeleteTextures(1, &window->geo3d_color); }
    if(window->geo3d_depth)         { gl.DeleteTextures(1, &window->geo3d_depth); }

    ReleaseDC(hwnd, window->dc);
    SLLStackPush(r_ogl_state->first_free_window, window);
  }
  ProfEnd();
}

//- dmylo: textures
r_hook R_Handle
r_tex2d_alloc(R_Tex2DKind kind, Vec2S32 size, R_Tex2DFormat format, void *data)
{
  //- dmylo: allocate
  R_OGL_Tex2D *texture = 0;
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    texture = r_ogl_state->first_free_tex2d;
    if(texture == 0)
    {
      texture = push_array(r_ogl_state->arena, R_OGL_Tex2D, 1);
    }
    else
    {
      U64 gen = texture->generation;
      SLLStackPop(r_ogl_state->first_free_tex2d);
      MemoryZeroStruct(texture);
      texture->generation = gen;
    }
    texture->generation += 1;

    // dmylo: currently no multithreading support, so we just copy the data and push this
    // to a list that the main thread will upload to the gpu before rendering. :sync_upload
    if(data) {
      U64 size_in_bytes = r_tex2d_format_bytes_per_pixel_table[format] * size.x * size.y;
      U8* ptr = (U8*)arena_push(r_ogl_state->upload_arena, size_in_bytes);
      MemoryCopy(ptr, (U8 *)data, size_in_bytes);
      texture->upload_data = ptr;
    }

    // This must be set with the lock held because the main thread will read them :sync_upload
    texture->kind = kind;
    texture->size = size;
    texture->format = format;

    SLLStackPush_N(r_ogl_state->first_texture_to_upload, texture, upload_next);
  }

  R_Handle result = r_ogl_handle_from_tex2d(texture);
  ProfEnd();
  return result;
}

r_hook void
r_tex2d_release(R_Handle handle)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    R_OGL_Tex2D *texture = r_ogl_tex2d_from_handle(handle);
    SLLStackPush(r_ogl_state->first_to_free_tex2d, texture);
  }
  ProfEnd();
}

r_hook R_Tex2DKind
r_kind_from_tex2d(R_Handle handle)
{
  R_OGL_Tex2D *texture = r_ogl_tex2d_from_handle(handle);
  return texture->kind;
}

r_hook Vec2S32
r_size_from_tex2d(R_Handle handle)
{
  R_OGL_Tex2D *texture = r_ogl_tex2d_from_handle(handle);
  return texture->size;
}

r_hook R_Tex2DFormat
r_format_from_tex2d(R_Handle handle)
{
  R_OGL_Tex2D *texture = r_ogl_tex2d_from_handle(handle);
  return texture->format;
}

r_hook void
r_fill_tex2d_region(R_Handle handle, Rng2S32 subrect, void *data)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    R_OGL_Tex2D *texture = r_ogl_tex2d_from_handle(handle);

    Vec2S32 dim = v2s32(subrect.x1 - subrect.x0, subrect.y1 - subrect.y0);

    U64 size_in_bytes = r_tex2d_format_bytes_per_pixel_table[texture->format] * dim.x * dim.y;
    U8* ptr = (U8*)arena_push(r_ogl_state->upload_arena, size_in_bytes);
    MemoryCopy(ptr, (U8 *)data, size_in_bytes);

    R_OGL_Fill_Tex2D* fill = push_array(r_ogl_state->arena, R_OGL_Fill_Tex2D, 1);
    fill->texture = texture;
    fill->subrect = subrect;
    fill->data = ptr;

    SLLStackPush(r_ogl_state->first_texture_to_fill, fill);
  }
  ProfEnd();
}

//- dmylo: buffers
r_hook R_Handle
r_buffer_alloc(R_BufferKind kind, U64 size, void *data)
{
  ProfBeginFunction();

  //- dmylo: allocate
  R_OGL_Buffer *buffer = 0;
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    buffer = r_ogl_state->first_free_buffer;
    if(buffer == 0)
    {
      buffer = push_array(r_ogl_state->arena, R_OGL_Buffer, 1);
    }
    else
    {
      U64 gen = buffer->generation;
      SLLStackPop(r_ogl_state->first_free_buffer);
      MemoryZeroStruct(buffer);
      buffer->generation = gen;
    }

    buffer->generation += 1;

    // dmylo: currently no multithreading support, so we just copy the data and push this
    // to a list that the main thread will upload to the gpu before rendering. :sync_upload
    U8* ptr = (U8*)arena_push(r_ogl_state->upload_arena, size);
    MemoryCopy(ptr, (U8 *)data, size);
    buffer->upload_data = ptr;

    // This must be set with the lock held because the main thread will read them :sync_upload
    buffer->kind = kind;
    buffer->size = size;

    SLLStackPush_N(r_ogl_state->first_buffer_to_upload, buffer, upload_next);
  }

  R_Handle result = r_ogl_handle_from_buffer(buffer);
  ProfEnd();
  return result;
}

r_hook void
r_buffer_release(R_Handle handle)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    R_OGL_Buffer *buffer = r_ogl_buffer_from_handle(handle);
    SLLStackPush(r_ogl_state->first_to_free_buffer, buffer);
  }
  ProfEnd();
}


//- dmylo: frame markers
r_hook void
r_begin_frame(void)
{
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    // no-op
  }
}

r_hook void
r_end_frame(void)
{
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    // Create, fill and destroy data that can be allocated asynchronously.
    // We do all of these with the same lock held and in this order to avoid
    // race conditions where data has been freed before being allocated.

    //- dmylo: upload required buffers
    for(;r_ogl_state->first_buffer_to_upload;)
    {
      R_OGL_Buffer* buffer = r_ogl_state->first_buffer_to_upload;
      r_ogl_upload_buffer(buffer);

      SLLStackPop_N(r_ogl_state->first_buffer_to_upload, upload_next);
    }
    //- dmylo: upload required texures
    for(;r_ogl_state->first_texture_to_upload;)
    {
      R_OGL_Tex2D* texture = r_ogl_state->first_texture_to_upload;
      r_ogl_upload_texture(texture);

      SLLStackPop_N(r_ogl_state->first_texture_to_upload, upload_next);
    }

    //- dmylo: fill required textures
    for(;r_ogl_state->first_texture_to_fill;)
    {
      R_OGL_Fill_Tex2D* fill = r_ogl_state->first_texture_to_fill;
      r_ogl_fill_tex2d_region(fill->texture, fill->subrect, fill->data);

      SLLStackPop(r_ogl_state->first_texture_to_fill);
    }

    //- dmylo: free upload scratch arena.
    arena_clear(r_ogl_state->upload_arena);

    //- dmylo: free flush buffers
    for(R_OGL_FlushBuffer *buffer = r_ogl_state->first_buffer_to_flush; buffer != 0; buffer = buffer->next)
    {
      gl.DeleteBuffers(1, &buffer->buffer);
    }

    //- dmylo: free allocated textures
    for(R_OGL_Tex2D *tex = r_ogl_state->first_to_free_tex2d, *next = 0;
        tex != 0;
        tex = next)
    {
      next = tex->next;
      gl.DeleteTextures(1, &tex->texture);
      tex->generation += 1;
      SLLStackPush(r_ogl_state->first_free_tex2d, tex);
    }

    //- dmylo: free allocated buffers
    for(R_OGL_Buffer *buf = r_ogl_state->first_to_free_buffer, *next = 0;
        buf != 0;
        buf = next)
    {
      next = buf->next;
      gl.DeleteBuffers(1, &buf->buffer);
      buf->generation += 1;
      SLLStackPush(r_ogl_state->first_free_buffer, buf);
    }

    arena_clear(r_ogl_state->buffer_flush_arena);
    r_ogl_state->first_buffer_to_flush = r_ogl_state->last_buffer_to_flush = 0;
    r_ogl_state->first_to_free_tex2d  = 0;
    r_ogl_state->first_to_free_buffer = 0;
  }
}

r_hook void
r_window_begin_frame(OS_Handle window_handle, R_Handle window_equip)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    R_OGL_Window *window = r_ogl_window_from_handle(window_equip);

    //- dmylo: map os window handle -> hwnd
    HWND hwnd = {0};
    {
      W32_Window *w32_layer_window = w32_window_from_os_window(window_handle);
      hwnd = w32_hwnd_from_window(w32_layer_window);
    }

    //- dmylo: bind main context to the window.
    bool ok = wglMakeCurrent(window->dc, r_ogl_state->glrc);

    //- dmylo: get resolution
    Rng2F32 client_rect = os_client_rect_from_window(window_handle);
    Vec2S32 resolution = {(S32)(client_rect.x1 - client_rect.x0), (S32)(client_rect.y1 - client_rect.y0)};

    //- dmylo: resolution change
    if(window->last_resolution.x != resolution.x ||
       window->last_resolution.y != resolution.y)
    {
      window->last_resolution = resolution;

      //- dmylo: resize buffers
      if(window->stage_scratch_fbo)   { gl.DeleteFramebuffers(1, &window->stage_scratch_fbo); }
      if(window->stage_scratch_color) { gl.DeleteTextures(1, &window->stage_scratch_color); }
      if(window->stage_fbo)           { gl.DeleteFramebuffers(1, &window->stage_scratch_fbo); }
      if(window->stage_color)         { gl.DeleteTextures(1, &window->stage_color); }
      if(window->geo3d_fbo)           { gl.DeleteFramebuffers(1, &window->stage_scratch_fbo); }
      if(window->geo3d_color)         { gl.DeleteTextures(1, &window->geo3d_color); }
      if(window->geo3d_depth)         { gl.DeleteTextures(1, &window->geo3d_depth); }

      //- dmylo: create stage framebuffer
      {
        // Stage
        gl.GenFramebuffers(1, &window->stage_fbo);
        gl.BindFramebuffer(GL_FRAMEBUFFER, window->stage_fbo);
        gl.GenTextures(1, &window->stage_color);
        gl.BindTexture(GL_TEXTURE_2D, window->stage_color);
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution.x, resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, window->stage_color, 0);

        // Stage scratch
        gl.GenFramebuffers(1, &window->stage_scratch_fbo);
        gl.BindFramebuffer(GL_FRAMEBUFFER, window->stage_scratch_fbo);
        gl.GenTextures(1, &window->stage_scratch_color);
        gl.BindTexture(GL_TEXTURE_2D, window->stage_scratch_color);
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution.x, resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, window->stage_scratch_color, 0);
      }

      //- dmylo: create geo3d targets
      {
        gl.GenFramebuffers(1, &window->geo3d_fbo);
        gl.BindFramebuffer(GL_FRAMEBUFFER, window->geo3d_fbo);

        // Color
        gl.GenTextures(1, &window->geo3d_color);
        gl.BindTexture(GL_TEXTURE_2D, window->geo3d_color);
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution.x, resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, window->geo3d_color, 0);

        // Depth
        gl.GenTextures(1, &window->geo3d_depth);
        gl.BindTexture(GL_TEXTURE_2D, window->geo3d_depth);
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, resolution.x, resolution.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, window->geo3d_depth, 0);
      }

      gl.BindTexture(GL_TEXTURE_2D, 0);
    }

    gl.Disable(GL_SCISSOR_TEST);
    gl.ClearColor(0.0, 0.0, 0.0, 0.0);
    gl.ClearDepth(1.0);

    gl.BindFramebuffer(GL_FRAMEBUFFER, window->geo3d_fbo);
    gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    //- dmylo: clear stage fbo
    gl.BindFramebuffer(GL_FRAMEBUFFER, window->stage_fbo);
    gl.Clear(GL_COLOR_BUFFER_BIT);

    //- dmylo: clear window backbuffer
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl.Clear(GL_COLOR_BUFFER_BIT);
  }
  ProfEnd();
}

r_hook void
r_window_end_frame(OS_Handle window_handle, R_Handle window_equip)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    R_OGL_Window *window = r_ogl_window_from_handle(window_equip);

    ////////////////////////////
    //- dmylo: finalize, by writing staging buffer out to window framebuffer
    //
    {
      gl.BindFramebuffer(GL_FRAMEBUFFER, 0);

      gl.Disable(GL_DEPTH_TEST);
      gl.Disable(GL_STENCIL_TEST);
      gl.Disable(GL_SCISSOR_TEST);
      gl.Disable(GL_CULL_FACE);
      gl.Disable(GL_BLEND);

      Vec2S32 resolution = window->last_resolution;
      gl.Viewport(0, 0, (F32)resolution.x, (F32)resolution.y);

      gl.UseProgram(r_ogl_state->finalize_shader);

      gl.ActiveTexture(GL_TEXTURE0);
      gl.BindTexture(GL_TEXTURE_2D, window->stage_color);
      gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    SwapBuffers(window->dc);
  }
  ProfEnd();
}


//- dmylo: render pass submission
r_hook void
r_window_submit(OS_Handle window, R_Handle window_equip, R_PassList *passes)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    ////////////////////////////
    //- dmylo: unpack arguments
    //
    R_OGL_Window *wnd = r_ogl_window_from_handle(window_equip);

    ////////////////////////////
    //- dmylo: do passes
    //
    for(R_PassNode *pass_n = passes->first; pass_n != 0; pass_n = pass_n->next)
    {
      R_Pass *pass = &pass_n->v;
      switch(pass->kind)
      {
        default:{}break;

        case R_PassKind_UI:
        {
          // dmylo: unpack params
          R_PassParams_UI *params = pass->params_ui;
          R_BatchGroup2DList *rect_batch_groups = &params->rects;

          // dmylo: set up rasterizer
          Vec2S32 resolution = wnd->last_resolution;
          gl.Viewport(0, 0, (F32)resolution.x, (F32)resolution.y);

          // dmylo): set render target
          gl.BindFramebuffer(GL_FRAMEBUFFER, wnd->stage_fbo);

          // dmylo: culling
          gl.Enable(GL_CULL_FACE);
          gl.CullFace(GL_BACK);
          gl.FrontFace(GL_CW);

          // dmylo: depth
          gl.Disable(GL_DEPTH_TEST);
          gl.Disable(GL_STENCIL_TEST);

          // dmylo: blending
          gl.Enable(GL_BLEND);
          gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

          // dmylo: shader
          gl.UseProgram(r_ogl_state->rect_shader);

          // dmylo: draw each batch group
          for(R_BatchGroup2DNode *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
          {
            R_BatchList *batches = &group_n->batches;
            R_BatchGroup2DParams *group_params = &group_n->params;

            // dmylo: VAO
            gl.BindVertexArray(r_ogl_state->rect_vao);

            // dmylo: get & fill buffer
            GLuint buffer = r_ogl_instance_buffer_from_size(batches->byte_count);
            {
              Temp temp = temp_begin(r_ogl_state->arena);

              U8* dst_ptr = (U8*)arena_push(r_ogl_state->arena, batches->byte_count);
              U64 off = 0;
              for(R_BatchNode *batch_n = batches->first; batch_n != 0; batch_n = batch_n->next)
              {
                MemoryCopy(dst_ptr+off, batch_n->v.v, batch_n->v.byte_count);
                off += batch_n->v.byte_count;
              }
              gl.BindBuffer(GL_ARRAY_BUFFER, buffer);
              gl.BufferData(GL_ARRAY_BUFFER, batches->byte_count, dst_ptr, GL_STREAM_DRAW);

              temp_end(temp);
            }

            for(U32 i = 0; i < 8; i++) {
              gl.EnableVertexAttribArray(i);
              gl.VertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, 8 * 16, (void*)(U64)(i * 16));
              gl.VertexAttribDivisor(i, 1);
            }

            // dmylo: get texture
            R_Handle texture_handle = group_params->tex;
            if(r_handle_match(texture_handle, r_handle_zero()))
            {
              texture_handle = r_ogl_state->backup_texture;
            }
            R_OGL_Tex2D *texture = r_ogl_tex2d_from_handle(texture_handle);

            // dmylo: get texture sample map matrix, based on format
            Vec4F32 texture_sample_channel_map[] =
            {
              {1, 0, 0, 0},
              {0, 1, 0, 0},
              {0, 0, 1, 0},
              {0, 0, 0, 1},
            };
            switch(texture->format)
            {
              default: break;
              case R_Tex2DFormat_R8:
              {
                MemoryZeroArray(texture_sample_channel_map);
                texture_sample_channel_map[0] = v4f32(1, 1, 1, 1);
              }break;
            }

            // dmylo: upload uniforms
            R_OGL_Uniforms_Rect uniforms = {0};
            {
              uniforms.viewport_size             = v2f32(resolution.x, resolution.y);
              uniforms.opacity                   = 1-group_params->transparency;
              MemoryCopyArray(uniforms.texture_sample_channel_map, texture_sample_channel_map);
              uniforms.texture_t2d_size          = v2f32(texture->size.x, texture->size.y);
              uniforms.xform[0] = v4f32(group_params->xform.v[0][0], group_params->xform.v[1][0], group_params->xform.v[2][0], 0);
              uniforms.xform[1] = v4f32(group_params->xform.v[0][1], group_params->xform.v[1][1], group_params->xform.v[2][1], 0);
              uniforms.xform[2] = v4f32(group_params->xform.v[0][2], group_params->xform.v[1][2], group_params->xform.v[2][2], 0);
              uniforms.xform[3] = v4f32(0, 0, 0, 1);
              Vec2F32 xform_2x2_col0 = v2f32(uniforms.xform[0].x, uniforms.xform[1].x);
              Vec2F32 xform_2x2_col1 = v2f32(uniforms.xform[0].y, uniforms.xform[1].y);
              uniforms.xform_scale.x = length_2f32(xform_2x2_col0);
              uniforms.xform_scale.y = length_2f32(xform_2x2_col1);
            }

            // dmylo: uniform
            GLuint uniform_buffer = r_ogl_state->rect_uniform_buffer;
            {
              gl.BindBufferBase(GL_UNIFORM_BUFFER, 0, uniform_buffer);
              gl.BufferData(GL_UNIFORM_BUFFER, sizeof(uniforms), &uniforms, GL_STREAM_DRAW);
            }
            gl.UniformBlockBinding(r_ogl_state->rect_shader, r_ogl_state->rect_uniform_block_index, 0);

            // dmylo: activate and bind texture
            gl.ActiveTexture(GL_TEXTURE0);
            gl.BindTexture(GL_TEXTURE_2D, texture->texture);

            // dmylo: sampler mode
            switch(group_params->tex_sample_kind) {
              case R_Tex2DSampleKind_Nearest: {
                gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
              } break;
              case R_Tex2DSampleKind_Linear: {
                gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              }break;
            }

            // dmylo: setup scissor rect
            {
              Rng2F32 clip = group_params->clip;

              GLint x = 0, y = 0, width = 0, height = 0;
              {
                if(clip.x0 == 0 && clip.y0 == 0 && clip.x1 == 0 && clip.y1 == 0)
                {
                  x = 0;
                  width = (GLint)wnd->last_resolution.x;
                  y = 0;
                  height = (GLint)wnd->last_resolution.y;
                }
                else if(clip.x0 > clip.x1 || clip.y0 > clip.y1)
                {
                  x = 0;
                  width = 0;
                  y = 0;
                  height = 0;
                }
                else
                {
                  x = (GLint)clip.x0;
                  width = (GLint)(clip.x1 - clip.x0);
                  // dmylo: Invert y because OpenGL scissor rect starts from bottom-left instead of top-left as in d3d11.
                  y = wnd->last_resolution.y - (GLint)clip.y1;
                  height = (GLint)(clip.y1 - clip.y0);
                }
              }

              gl.Enable(GL_SCISSOR_TEST);
              gl.Scissor(x, y, width, height);
            }

            // dmylo: draw instances
            gl.DrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, batches->byte_count / batches->bytes_per_inst);
          }
        } break;

        ////////////////////////
        //- dmylo: blur rendering pass
        //
        case R_PassKind_Blur:
        {
          R_PassParams_Blur *params = pass->params_blur;

          {
            // dmylo: Common setup
            gl.Disable(GL_DEPTH_TEST);
            gl.Disable(GL_STENCIL_TEST);
            gl.Disable(GL_SCISSOR_TEST);
            gl.Disable(GL_CULL_FACE);
            gl.Disable(GL_BLEND);

            Vec2S32 resolution = wnd->last_resolution;
            gl.Viewport(0, 0, (F32)resolution.x, (F32)resolution.y);

            gl.UseProgram(r_ogl_state->blur_shader);

            // dmylo: common uniform
            R_OGL_Uniforms_BlurPass uniforms = {0};
            {
              R_Blur_Kernel kernel = {0};
              r_fill_blur_kernel(params->blur_size, &kernel);

              for(U64 i = 0; i < ArrayCount(uniforms.kernel); i++)
              {
                uniforms.kernel[i] = v4f32(kernel.weights[i].x, kernel.weights[i].y, 0, 0);
              }

              uniforms.viewport_size = v2f32(resolution.x, resolution.y);
              uniforms.rect          = params->rect;
              uniforms.blur_count    = 1 + kernel.blur_count / 2; // 2x smaller because of bilinear sampling
              MemoryCopyArray(uniforms.corner_radii.v, params->corner_radii);
            }

            GLuint uniform_buffer = r_ogl_state->blur_uniform_buffer;
            {
              gl.BindBufferBase(GL_UNIFORM_BUFFER, 0, uniform_buffer);
              gl.BufferData(GL_UNIFORM_BUFFER, sizeof(uniforms), &uniforms, GL_STREAM_DRAW);
              gl.UniformBlockBinding(r_ogl_state->blur_shader, r_ogl_state->blur_uniform_block_index, 0);
            }

            // dmylo: Horizontal pass
            gl.Uniform2f(r_ogl_state->blur_direction_uniform_location, 1.f / resolution.x, 0);
            gl.BindFramebuffer(GL_FRAMEBUFFER, wnd->stage_scratch_fbo);
            gl.ActiveTexture(GL_TEXTURE0);
            gl.BindTexture(GL_TEXTURE_2D, wnd->stage_color);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // dmylo: Vertical pass
            gl.Uniform2f(r_ogl_state->blur_direction_uniform_location, 0.0f, 1.f / resolution.y);
            gl.BindFramebuffer(GL_FRAMEBUFFER, wnd->stage_fbo);
            gl.ActiveTexture(GL_TEXTURE0);
            gl.BindTexture(GL_TEXTURE_2D, wnd->stage_scratch_color);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
          }


        }break;

        ////////////////////////
        //- dmylo: 3d geometry rendering pass
        //
        case R_PassKind_Geo3D:
        {
          //- dmylo: unpack params
          R_PassParams_Geo3D *params = pass->params_geo3d;
          R_BatchGroup3DMap *mesh_group_map = &params->mesh_batches;

          // dmylo: Common setup
          gl.Disable(GL_STENCIL_TEST);
          gl.Disable(GL_SCISSOR_TEST);
          gl.Disable(GL_CULL_FACE);
          gl.Enable(GL_DEPTH_TEST);
          gl.DepthFunc(GL_LESS);

          // dmylo: viewport, invert y because OpenGL viewport starts from bottom-left instead of top-left as in d3d11.
          Vec2F32 viewport_dim = dim_2f32(params->viewport);
          gl.Viewport(params->viewport.x0, wnd->last_resolution.y - params->viewport.y1, viewport_dim.x, viewport_dim.y);

          // dmylo: blending
          gl.Enable(GL_BLEND);
          gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

          // dmylo: shader
          gl.UseProgram(r_ogl_state->geo3d_shader);

          // dmylo: vao
          gl.BindVertexArray(r_ogl_state->geo3d_vao);

          // dmylo: framebuffer
          gl.BindFramebuffer(GL_FRAMEBUFFER, wnd->geo3d_fbo);

          {
            // dmylo: draw all batches
            for(U64 slot_idx = 0; slot_idx < mesh_group_map->slots_count; slot_idx += 1)
            {
              for(R_BatchGroup3DMapNode *n = mesh_group_map->slots[slot_idx]; n != 0; n = n->next)
              {
                // dmylo: unpack group params
                R_BatchList *batches = &n->batches;
                R_BatchGroup3DParams *group_params = &n->params;
                R_OGL_Buffer *mesh_vertices = r_ogl_buffer_from_handle(group_params->mesh_vertices);
                R_OGL_Buffer *mesh_indices = r_ogl_buffer_from_handle(group_params->mesh_indices);

                if(mesh_vertices->buffer == 0 || mesh_indices->buffer == 0) { continue; }

                // Vertex and index buffers
                gl.BindBuffer(GL_ARRAY_BUFFER, mesh_vertices->buffer);
                gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_indices->buffer);
                gl.EnableVertexAttribArray(0);
                gl.EnableVertexAttribArray(1);
                gl.EnableVertexAttribArray(2);
                gl.EnableVertexAttribArray(3);

                U32 stride = 11 * sizeof(F32);
                gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * 4, (void*)(U64)(0));
                gl.VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * 4, (void*)(U64)(3 * sizeof(F32)));
                gl.VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * 4, (void*)(U64)(6 * sizeof(F32)));
                gl.VertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * 4, (void*)(U64)(8 * sizeof(F32)));

                // dmylo: uniforms
                R_OGL_Uniforms_Mesh uniforms = {0};
                {
                  // dmylo: we are using D3D11-style z here (mapped to [0; 1]), this works fine for
                  // the OpenGL [-1; 1] mapping, but is not optimal in terms of precision.
                  uniforms.xform = mul_4x4f32(params->projection, params->view);
                }
                gl.UniformMatrix4fv(r_ogl_state->geo3d_uniform_location, 1, GL_FALSE, &uniforms.xform.v[0][0]);

                gl.DrawElements(GL_TRIANGLES, mesh_indices->size/sizeof(U32), GL_UNSIGNED_INT, 0);
              }
            }
          }

          //- dmylo: composite to main staging buffer
          {
            // dmylo: Common setup
            gl.Disable(GL_DEPTH_TEST);
            gl.Disable(GL_STENCIL_TEST);
            gl.Disable(GL_CULL_FACE);

            // dmylo: blending
            gl.Enable(GL_BLEND);
            gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            Vec2S32 resolution = wnd->last_resolution;
            gl.Viewport(0, 0, (F32)resolution.x, (F32)resolution.y);

            // dmylo: shader
            gl.UseProgram(r_ogl_state->geo3dcomposite_shader);

            // dmylo: texture
            gl.ActiveTexture(GL_TEXTURE0);
            gl.BindTexture(GL_TEXTURE_2D, wnd->geo3d_color);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            // dmylo: framebuffer
            gl.BindFramebuffer(GL_FRAMEBUFFER, wnd->stage_fbo);

            // dmylo: setup scissor rect
            {
              GLint x = 0, y = 0, width = 0, height = 0;
              Rng2F32 clip = params->clip;
              {
                if(clip.x0 == 0 && clip.y0 == 0 && clip.x1 == 0 && clip.y1 == 0)
                {
                  x = 0;
                  width = (GLint)wnd->last_resolution.x;
                  y = 0;
                  height = (GLint)wnd->last_resolution.y;
                }
                else if(clip.x0 > clip.x1 || clip.y0 > clip.y1)
                {
                  x = 0;
                  width = 0;
                  y = 0;
                  height = 0;
                }
                else
                {
                  x = (GLint)clip.x0;
                  width = (GLint)(clip.x1 - clip.x0);
                  // dmylo: Invert y because OpenGL scissor rect starts from bottom-left instead of top-left as in d3d11.
                  y = wnd->last_resolution.y - (GLint)clip.y1;
                  height = (GLint)(clip.y1 - clip.y0);
                }
              }

              gl.Enable(GL_SCISSOR_TEST);
              gl.Scissor(x, y, width, height);
            }

            gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
          }
        }break;
      }
    }
  }
  ProfEnd();
}

#undef gl