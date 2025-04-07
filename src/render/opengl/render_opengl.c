////////////////////////////////
//~ dan: Generated Code

#include "generated/render_opengl.meta.c"


////////////////////////////////
//~ dan: Helper function for rounding a float to an integer
internal S32
round_f32_s32(F32 v)
{
    return (S32)round_f32(v);
}

////////////////////////////////
//~ dan: Helper for initializing 4x4 matrices with 16 values
internal Mat4x4F32 
m4x4f32(F32 m00, F32 m01, F32 m02, F32 m03,
        F32 m10, F32 m11, F32 m12, F32 m13,
        F32 m20, F32 m21, F32 m22, F32 m23,
        F32 m30, F32 m31, F32 m32, F32 m33)
{
    Mat4x4F32 result = {
        {{m00, m01, m02, m03},
         {m10, m11, m12, m13},
         {m20, m21, m22, m23},
         {m30, m31, m32, m33}}
    };
    return result;
}

////////////////////////////////

////////////////////////////////
//~ dan: Helpers

internal R_GL_Window *
r_gl_window_from_handle(R_Handle handle)
{
  // Simple check: lowest bit is 1 for windows
  if((handle.u64[0] & 1) == 1)
  {
    // Ensure the pointer part is non-NULL before dereferencing potentially
    R_GL_Window* potential_window = (R_GL_Window *)(handle.u64[0] & ~0x3ull);
    if (potential_window != 0 && potential_window->next != potential_window) // Check against self-referencing nil handle
    {
       return potential_window;
    }
  }
  return &r_gl_window_nil;
}

internal R_Handle
r_gl_handle_from_window(R_GL_Window *window)
{
  R_Handle result = {0};
  if (window != 0 && window != &r_gl_window_nil)
  {
  result.u64[0] = (U64)window | 0x1;
  }
  return result;
}

internal R_GL_Tex2D *
r_gl_tex2d_from_handle(R_Handle handle)
{
  // Simple check: lowest two bits are 10 for textures
  if((handle.u64[0] & 0x3) == 0x2)
  {
    R_GL_Tex2D* potential_tex = (R_GL_Tex2D *)(handle.u64[0] & ~0x3ull);
    if (potential_tex != 0 && potential_tex->next != potential_tex) // Check against self-referencing nil handle
    {
        return potential_tex;
    }
  }
  return &r_gl_tex2d_nil;
}

internal R_Handle
r_gl_handle_from_tex2d(R_GL_Tex2D *texture)
{
  R_Handle result = {0};
  if (texture != 0 && texture != &r_gl_tex2d_nil)
  {
  result.u64[0] = (U64)texture | 0x2;
  }
  return result;
}

internal R_GL_Buffer *
r_gl_buffer_from_handle(R_Handle handle)
{
  // Simple check: lowest two bits are 11 for buffers
  if((handle.u64[0] & 0x3) == 0x3)
  {
    R_GL_Buffer* potential_buf = (R_GL_Buffer *)(handle.u64[0] & ~0x3ull);
    if (potential_buf != 0 && potential_buf->next != potential_buf) // Check against self-referencing nil handle
    {
        return potential_buf;
    }
  }
  return &r_gl_buffer_nil;
}

internal R_Handle
r_gl_handle_from_buffer(R_GL_Buffer *buffer)
{
  R_Handle result = {0};
   if (buffer != 0 && buffer != &r_gl_buffer_nil)
   {
  result.u64[0] = (U64)buffer | 0x3;
   }
  return result;
}

internal GLenum
r_gl_usage_from_resource_kind(R_ResourceKind kind)
{
  GLenum result = GL_STATIC_DRAW;
  switch(kind)
  {
    case R_ResourceKind_Static:  result = GL_STATIC_DRAW;  break;
    case R_ResourceKind_Dynamic: result = GL_DYNAMIC_DRAW; break;
    case R_ResourceKind_Stream:  result = GL_STREAM_DRAW;  break;
    default: break; // Should not happen
  }
  return result;
}

internal GLenum
r_gl_format_from_tex2d_format(R_Tex2DFormat format)
{
  GLenum result = GL_RGBA;
  switch(format)
  {
    case R_Tex2DFormat_R8:     result = GL_RED;  break;
    case R_Tex2DFormat_RG8:    result = GL_RG;   break;
    case R_Tex2DFormat_RGBA8:  result = GL_RGBA; break;
    case R_Tex2DFormat_BGRA8:  result = GL_BGRA; break;
    case R_Tex2DFormat_R16:    result = GL_RED;  break; // Note: type will be GL_UNSIGNED_SHORT or similar
    case R_Tex2DFormat_RGBA16: result = GL_RGBA; break; // Note: type will be GL_UNSIGNED_SHORT or similar
    case R_Tex2DFormat_R32:    result = GL_RED;  break; // Note: type will be GL_FLOAT
    case R_Tex2DFormat_RG32:   result = GL_RG;   break; // Note: type will be GL_FLOAT
    case R_Tex2DFormat_RGBA32: result = GL_RGBA; break; // Note: type will be GL_FLOAT
    default: break;
  }
  return result;
}

internal GLint
r_gl_internal_format_from_tex2d_format(R_Tex2DFormat format)
{
  GLint result = GL_RGBA8;
  switch(format)
  {
    case R_Tex2DFormat_R8:     result = GL_R8;       break;
    case R_Tex2DFormat_RG8:    result = GL_RG8;      break;
    case R_Tex2DFormat_RGBA8:  result = GL_RGBA8;    break;
    case R_Tex2DFormat_BGRA8:  result = GL_RGBA8;    break;
    case R_Tex2DFormat_R16:    result = GL_R16;      break; // Use GL_R16UI or GL_R16I for integer formats if needed
    case R_Tex2DFormat_RGBA16: result = GL_RGBA16;   break; // Use GL_RGBA16UI or GL_RGBA16I for integer formats
    case R_Tex2DFormat_R32:    result = GL_R32F;     break;
    case R_Tex2DFormat_RG32:   result = GL_RG32F;    break;
    case R_Tex2DFormat_RGBA32: result = GL_RGBA32F;  break;
    default: break;
  }
  return result;
}

internal GLenum
r_gl_type_from_tex2d_format(R_Tex2DFormat format)
{
  GLenum result = GL_UNSIGNED_BYTE;
  switch(format)
  {
    case R_Tex2DFormat_R8:
    case R_Tex2DFormat_RG8:
    case R_Tex2DFormat_RGBA8:
    case R_Tex2DFormat_BGRA8:  result = GL_UNSIGNED_BYTE; break;
    case R_Tex2DFormat_R16:    result = GL_UNSIGNED_SHORT; break; // Or GL_HALF_FLOAT if using FP16
    case R_Tex2DFormat_RGBA16: result = GL_UNSIGNED_SHORT; break; // Or GL_HALF_FLOAT
    case R_Tex2DFormat_R32:
    case R_Tex2DFormat_RG32:
    case R_Tex2DFormat_RGBA32: result = GL_FLOAT; break;
    default: break;
  }
  return result;
}

internal GLint
r_gl_filter_from_sample_kind(R_Tex2DSampleKind kind)
{
  GLint result = GL_LINEAR;
  switch(kind)
  {
    case R_Tex2DSampleKind_Nearest: result = GL_NEAREST; break;
    case R_Tex2DSampleKind_Linear:  result = GL_LINEAR;  break;
    default: break;
  }
  return result;
}

////////////////////////////////
//~ dan: Buffer Update Helpers

internal void
r_gl_buffer_update_sub_data(R_Handle handle, U64 offset, U64 size, void *data)
{
  R_GL_Buffer *buffer = r_gl_buffer_from_handle(handle);
  if (buffer == &r_gl_buffer_nil || buffer->buffer_id == 0 || size == 0)
  {
    return;
  }

  Assert(buffer->kind == R_ResourceKind_Dynamic || buffer->kind == R_ResourceKind_Stream);
  Assert(offset + size <= buffer->size);
  Assert(buffer->target != 0); // Target should have been set on alloc

  GLenum target = buffer->target; // Use stored target

  glBindBuffer(target, buffer->buffer_id);
  r_gl_check_error("glBindBuffer UpdateSubData");

  glBufferSubData(target, (GLintptr)offset, (GLsizeiptr)size, data);
  r_gl_check_error("glBufferSubData");

  glBindBuffer(target, 0);
}

internal void*
r_gl_buffer_map_range(R_Handle handle, U64 offset, U64 size, GLbitfield access_flags)
{
  R_GL_Buffer *buffer = r_gl_buffer_from_handle(handle);
  if (buffer == &r_gl_buffer_nil || buffer->buffer_id == 0 || size == 0)
  {
    return NULL;
  }

  Assert(buffer->kind == R_ResourceKind_Stream || buffer->kind == R_ResourceKind_Dynamic); // Map usually for Stream/Dynamic
  Assert(offset + size <= buffer->size);
  Assert(buffer->target != 0);

  GLenum target = buffer->target; // Use stored target

  glBindBuffer(target, buffer->buffer_id);
  r_gl_check_error("glBindBuffer MapRange");

  void *ptr = glMapBufferRange(target, (GLintptr)offset, (GLsizeiptr)size, access_flags);
  r_gl_check_error("glMapBufferRange");

  // Do not unbind here, caller must call unmap which implies binding.
  // glBindBuffer(target, 0);

  return ptr;
}

internal B32
r_gl_buffer_unmap(R_Handle handle)
{
  R_GL_Buffer *buffer = r_gl_buffer_from_handle(handle);
  if (buffer == &r_gl_buffer_nil || buffer->buffer_id == 0)
  {
    return 0; // Indicate failure or no-op
  }
  Assert(buffer->target != 0);

  GLenum target = buffer->target; // Use stored target

  glBindBuffer(target, buffer->buffer_id);
  r_gl_check_error("glBindBuffer Unmap");

  GLboolean result = glUnmapBuffer(target);
  r_gl_check_error("glUnmapBuffer");

  glBindBuffer(target, 0);

  // glUnmapBuffer returns GL_TRUE unless the data store contents have become corrupt.
  return (result == GL_TRUE);
}

////////////////////////////////
//~ dan: Basic OpenGL Error Checking

internal void
r_gl_check_error_line_file(const char *op, int line, char *file)
{
  if (!r_gl_state->has_valid_context)
  {
    fprintf(stderr, "Warning: OpenGL error check with no valid context for operation '%s' at %s:%d\n", 
            op, file, line);
    return;
  }

  GLenum err = glGetError();
  if(err != GL_NO_ERROR)
  {
    // Map error code to a readable string
    const char* error_str = "Unknown Error";
    switch(err)
    {
      case GL_INVALID_ENUM:      error_str = "GL_INVALID_ENUM"; break;
      case GL_INVALID_VALUE:     error_str = "GL_INVALID_VALUE"; break;
      case GL_INVALID_OPERATION: error_str = "GL_INVALID_OPERATION"; break;
      case GL_STACK_OVERFLOW:    error_str = "GL_STACK_OVERFLOW"; break;
      case GL_STACK_UNDERFLOW:   error_str = "GL_STACK_UNDERFLOW"; break;
      case GL_OUT_OF_MEMORY:     error_str = "GL_OUT_OF_MEMORY"; break;
    }
    fprintf(stderr, "OpenGL Error 0x%x (%s) for operation '%s' at %s:%d\n", 
            err, error_str, op, file, line);
  }
}
#define r_gl_check_error(op) r_gl_check_error_line_file(op, __LINE__, __FILE__)

////////////////////////////////
//~ dan: Shader Compilation Helpers

internal GLuint
r_gl_compile_shader(GLenum type, const char *source, String8 name)
{
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  r_gl_check_error("glShaderSource");
  glCompileShader(shader);
  r_gl_check_error("glCompileShader");
  
  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    GLint log_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    char *info_log = malloc(log_length); // Use temp arena if available
    if (info_log) {
      glGetShaderInfoLog(shader, log_length, NULL, info_log);
      fprintf(stderr, "Shader Compilation Failure (%s) '%.*s':\n%s\n",
              type == GL_VERTEX_SHADER ? "VS" : "FS", str8_varg(name), info_log);
      free(info_log);
    } else {
      fprintf(stderr, "Shader Compilation Failure (%s) '%.*s': Could not allocate memory for log.\n",
              type == GL_VERTEX_SHADER ? "VS" : "FS", str8_varg(name));
    }
    glDeleteShader(shader);
    return 0;
  }
  
  return shader;
}

// Updated to optionally take geometry shader source
internal GLuint
r_gl_create_program(const char *vs_source, const char *fs_source, const char *gs_source, String8 name, GLuint *out_vs, GLuint *out_fs, GLuint *out_gs, GLint *out_texture_location)
{
  GLuint vs = 0, fs = 0, gs = 0, program = 0;
  GLint link_success = 0;

  // Print first few lines of vertex shader for debugging
  fprintf(stderr, "Compiling shader '%.*s':\n", str8_varg(name));

  vs = r_gl_compile_shader(GL_VERTEX_SHADER, vs_source, str8_lit("vertex"));
  if (!vs) goto fail;
  fs = r_gl_compile_shader(GL_FRAGMENT_SHADER, fs_source, str8_lit("fragment"));
  if (!fs) goto fail;
  if (gs_source) {
    gs = r_gl_compile_shader(GL_GEOMETRY_SHADER, gs_source, str8_lit("geometry"));
    if (!gs) goto fail;
  }

  program = glCreateProgram();
  r_gl_check_error("glCreateProgram");
  glAttachShader(program, vs);
  r_gl_check_error("glAttachShader VS");
  glAttachShader(program, fs);
  r_gl_check_error("glAttachShader FS");
  if (gs) {
    glAttachShader(program, gs);
    r_gl_check_error("glAttachShader GS");
  }
  glLinkProgram(program);
  r_gl_check_error("glLinkProgram");

  glGetProgramiv(program, GL_LINK_STATUS, &link_success);
  if(!link_success)
  {
    GLint log_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    char *info_log = malloc(log_length); // Use temp arena if available
     if (info_log) {
       glGetProgramInfoLog(program, log_length, NULL, info_log);
       fprintf(stderr, "Shader Linking Failure '%.*s':\n%s\n", str8_varg(name), info_log);
       free(info_log);
    } else {
       fprintf(stderr, "Shader Linking Failure '%.*s': Could not allocate memory for log.\n", str8_varg(name));
    }
    goto fail; // Use goto for cleanup
  }

  // Validate the program to catch any issues
  glValidateProgram(program);
  GLint validate_status = 0;
  glGetProgramiv(program, GL_VALIDATE_STATUS, &validate_status);
  if (validate_status == GL_FALSE) {
    GLint log_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    char *info_log = malloc(log_length);
    if (info_log) {
      glGetProgramInfoLog(program, log_length, NULL, info_log);
      fprintf(stderr, "Shader Program Validation Failed '%.*s':\n%s\n", 
              str8_varg(name), info_log);
      free(info_log);
      // Program can still be used even if validation fails, we'll only issue a warning
    }
  }

  // Detach shaders after successful link (optional but good practice)
  glDetachShader(program, vs);
  glDetachShader(program, fs);
  if(gs) glDetachShader(program, gs);

  // Store shader handles if requested, otherwise delete them
  if(out_vs) *out_vs = vs; else glDeleteShader(vs);
  if(out_fs) *out_fs = fs; else glDeleteShader(fs);
  if(out_gs) *out_gs = gs; else if(gs) glDeleteShader(gs);

  // Get sampler uniform location (assuming name "main_t2d" for texture unit 0)
  if (out_texture_location) {
      *out_texture_location = glGetUniformLocation(program, "main_t2d");
      fprintf(stderr, "%.*s shader main_t2d location: %d\n", str8_varg(name), *out_texture_location);
  }

  r_gl_check_error("Shader Program Creation Success");
  return program;

fail:
  fprintf(stderr, "ERROR: Failed to create shader program '%.*s'\n", str8_varg(name));
  if (vs) glDeleteShader(vs);
  if (fs) glDeleteShader(fs);
  if (gs) glDeleteShader(gs);
  if (program) glDeleteProgram(program);
  return 0;
}

////////////////////////////////
//~ dan: VAO Helpers (Added)

// Creates a VAO suitable for drawing a fullscreen quad (no VBO needed)
internal GLuint
r_gl_vao_make_fullscreen_quad(void)
{
  GLuint vao_id;
  glGenVertexArrays(1, &vao_id);
  // Bind the VAO before checking for errors
  glBindVertexArray(vao_id);
  r_gl_check_error("Fullscreen VAO Creation");
  glBindVertexArray(0); // Unbind to be safe
  return vao_id;
}

////////////////////////////////
//~ dan: One-Time Initialization Helpers

// Initialize GLEW only - called with temp context
internal void
r_gl_init_glew_if_needed(void)
{
  if(r_gl_state->glew_initialized)
  {
    return;
  }

  // Verify we have a valid OpenGL context before trying to initialize GLEW
  GLXContext current_ctx = glXGetCurrentContext();
  if (!current_ctx)
  {
    fprintf(stderr, "ERROR: Attempting to initialize GLEW without a valid OpenGL context!\n");
    return;
  }
  
  fprintf(stderr, "Initializing GLEW with current GLX context: %p\n", (void*)current_ctx);
  
  // Initialize GLEW
  glewExperimental = GL_TRUE; // Enable GLEW experimental features for modern contexts
  GLenum err = glewInit();
  
  // GLEW initialization can sometimes report GL_INVALID_ENUM but still work correctly
  // Clear any error that might have been generated by GLEW
  glGetError();
  
  if(err != GLEW_OK)
  {
    fprintf(stderr, "GLEW initialization error: %s\n", glewGetErrorString(err));
    return; // Return without setting glew_initialized if it completely failed
  }
  
  // Mark GLEW as initialized
  r_gl_state->glew_initialized = 1;
  
  // Print OpenGL version information for debugging
  const GLubyte* vendor = glGetString(GL_VENDOR);
  const GLubyte* renderer = glGetString(GL_RENDERER);
  const GLubyte* version = glGetString(GL_VERSION);
  const GLubyte* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
  
  fprintf(stderr, "OpenGL Vendor: %s\n", vendor ? (const char*)vendor : "unknown");
  fprintf(stderr, "OpenGL Renderer: %s\n", renderer ? (const char*)renderer : "unknown");
  fprintf(stderr, "OpenGL Version: %s\n", version ? (const char*)version : "unknown");
  fprintf(stderr, "GLSL Version: %s\n", glsl_version ? (const char*)glsl_version : "unknown");
  
  // Now that GLEW is initialized and we have a valid context, mark the context as valid
  // This will trigger processing of any deferred textures
  fprintf(stderr, "Setting has_valid_context = 1\n");
  r_gl_set_has_valid_context(1);
}

// Create global GL resources - called with final context
internal void
r_gl_create_global_resources(void)
{
  if(r_gl_state->global_resources_initialized)
  {
    return; // Already initialized
  }

  log_infof("Creating global OpenGL resources...");

  // Ensure GLEW is initialized first
  r_gl_init_glew_if_needed();
  if (!r_gl_state->has_valid_context) {
      log_infof("Skipping global resource creation as there is no valid GL context yet.");
      return;
  }
  
  // Make sure we have a valid context before proceeding
  if(!r_gl_state->has_valid_context)
  {
    fprintf(stderr, "Error: Attempted to create OpenGL resources without a valid context.\n");
    return;
  }
  
  // Make sure GLEW is initialized
  if(!r_gl_state->glew_initialized)
  {
    fprintf(stderr, "Error: Attempted to create OpenGL resources before GLEW initialization.\n");
    return;
  }

  fprintf(stderr, "Creating OpenGL global resources...\n");
  
  // Create fullscreen quad VAO
  r_gl_state->fullscreen_vao = r_gl_vao_make_fullscreen_quad();
  
  // Create rect VAO
  glGenVertexArrays(1, &r_gl_state->rect_vao);
  glBindVertexArray(r_gl_state->rect_vao);
r_gl_check_error("UI Bind VAO");
  
  // Create shared instance VBO (moved earlier)
  glGenBuffers(1, &r_gl_state->instance_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, r_gl_state->instance_vbo);
r_gl_check_error("UI Bind Instance VBO");
  // Allocate some initial size, it will be resized later if needed. Use GL_STREAM_DRAW for frequent updates.
  glBufferData(GL_ARRAY_BUFFER, MB(1), NULL, GL_STREAM_DRAW); // Example initial size: 1MB
  r_gl_check_error("Instance VBO Creation");
  
  // Bind the instance VBO before defining attributes that use it
  glBindBuffer(GL_ARRAY_BUFFER, r_gl_state->instance_vbo);
r_gl_check_error("UI Bind Instance VBO");
  r_gl_check_error("Rect VAO Bind Instance VBO");
  
  // Setup instance attributes for the Rect shader (8 vec4s)
  // Assuming R_Rect2DInst packs these 8 vec4s contiguously.
  // Stride is sizeof(R_Rect2DInst), which we assume is 8 * 4 * sizeof(float) = 128 bytes.
  size_t rect_instance_stride = 8 * 4 * sizeof(float); 
  for (int i = 0; i < 8; i++) {
    glEnableVertexAttribArray(i);
    // Each attribute is a vec4 (4 floats)
    // The offset is i * sizeof(vec4)
    glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, (GLsizei)rect_instance_stride, (void*)(i * 4 * sizeof(float)));
    glVertexAttribDivisor(i, 1); // Advance this attribute once per instance
  }
  r_gl_check_error("Rect VAO Instance Attributes");
  
  // Clean up bindings
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0); // Unbind VAO as well
  
  // Create mesh VAO for 3D drawing
  glGenVertexArrays(1, &r_gl_state->mesh_vao);

  glBindVertexArray(r_gl_state->mesh_vao);
  {
    // Per-vertex attributes (binding index 0 implicitly uses currently bound GL_ARRAY_BUFFER)
    glEnableVertexAttribArray(0); // Position
    glEnableVertexAttribArray(1); // Normal
    glEnableVertexAttribArray(2); // TexCoord
    glEnableVertexAttribArray(3); // Color
    // These will have divisor 0 by default

    // Per-instance attributes (binding index 1 implicitly uses currently bound GL_ARRAY_BUFFER)
    // Instance Transform Matrix (mat4 = 4 x vec4)
    glEnableVertexAttribArray(4); glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5); glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(6); glVertexAttribDivisor(6, 1);
    glEnableVertexAttribArray(7); glVertexAttribDivisor(7, 1);
  }
  glBindVertexArray(0); // Unbind VAO
  r_gl_check_error("Mesh VAO Creation");
  
  // Create UBO for each uniform block type
  // Note: UBO size must be a multiple of GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT
  // For simplicity, we use sizes directly from the struct definitions.
  GLuint ubo_sizes[R_GL_UniformTypeKind_COUNT] = {
    r_gl_g_uniform_type_kind_size_table[R_GL_UniformTypeKind_Rect],  // R_GL_UniformTypeKind_Rect
    r_gl_g_uniform_type_kind_size_table[R_GL_UniformTypeKind_Blur],  // R_GL_UniformTypeKind_Blur
    r_gl_g_uniform_type_kind_size_table[R_GL_UniformTypeKind_Mesh],  // R_GL_UniformTypeKind_Mesh
  };
  
  // Store sizes for later use
  for (int i = 0; i < R_GL_UniformTypeKind_COUNT; i++) {
    r_gl_state->uniform_type_kind_sizes[i] = ubo_sizes[i];
  }
  
  glGenBuffers(R_GL_UniformTypeKind_COUNT, r_gl_state->uniform_type_kind_buffers);
  for (int i = 0; i < R_GL_UniformTypeKind_COUNT; i++) {
    glBindBuffer(GL_UNIFORM_BUFFER, r_gl_state->uniform_type_kind_buffers[i]);
    glBufferData(GL_UNIFORM_BUFFER, ubo_sizes[i], NULL, GL_DYNAMIC_DRAW);
  }
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  
  // Create samplers
  glGenSamplers(R_Tex2DSampleKind_COUNT, r_gl_state->samplers);
  for (R_Tex2DSampleKind kind = 0; kind < R_Tex2DSampleKind_COUNT; kind++) {
    GLint filter = r_gl_filter_from_sample_kind(kind);
    glSamplerParameteri(r_gl_state->samplers[kind], GL_TEXTURE_MIN_FILTER, filter);
    glSamplerParameteri(r_gl_state->samplers[kind], GL_TEXTURE_MAG_FILTER, filter);
    // Use consistent wrapping mode: CLAMP_TO_EDGE for all textures
    // This ensures consistent behavior between D3D11 and OpenGL
    glSamplerParameteri(r_gl_state->samplers[kind], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(r_gl_state->samplers[kind], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  
  // Create backup texture (1x1 white pixel)
  {
    // Create backup texture (1x1 white pixel)
    R_GL_Tex2D *tex = r_gl_state->first_free_tex2d;
    if(!tex)
    {
      tex = push_array(r_gl_state->arena, R_GL_Tex2D, 1);
    }
    else
    {
      SLLStackPop(r_gl_state->first_free_tex2d);
    }
    // Minimal reset needed after popping from free list
    MemoryZeroStruct(tex); // Ensure struct is clean
    tex->generation = (tex == r_gl_state->first_free_tex2d) ? 1 : tex->generation + 1; // Increment gen safely
    
    tex->kind = R_ResourceKind_Static;
    tex->size = v2s32(1, 1);
    tex->format = R_Tex2DFormat_RGBA8;
    
    glGenTextures(1, &tex->texture_id);
    r_gl_check_error("glGenTextures Backup"); // Added context
    glBindTexture(GL_TEXTURE_2D, tex->texture_id);
    r_gl_check_error("glBindTexture Backup");
    U8 white_pixel[4] = {255, 255, 255, 255};
    // Set pixel store alignment for the 1x1 texture
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // RGBA8 is 4 bytes
    r_gl_check_error("glPixelStorei Backup");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
    r_gl_check_error("glTexImage2D Backup");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Added wrap mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Added wrap mode
    r_gl_check_error("glTexParameteri Backup");
    glBindTexture(GL_TEXTURE_2D, 0);

    r_gl_state->backup_texture = r_gl_handle_from_tex2d(tex);
  }
  
  // NOTE: Rect shader
  {
    // Use global variables for shader source
    const char *vs_source = (const char *)r_gl_g_vshad_kind_source_ptr_table[R_GL_VShadKind_Rect]->str;
    const char *fs_source = (const char *)r_gl_g_pshad_kind_source_ptr_table[R_GL_PShadKind_Rect]->str;
      
    GLint texture_location = 0;
    r_gl_state->rect_shader.program_id = r_gl_create_program(
      vs_source, fs_source, NULL, 
      str8_lit("rect_shader"), 
      &r_gl_state->rect_shader.vertex_shader,
      &r_gl_state->rect_shader.fragment_shader,
      NULL,
      &texture_location); // Pass location pointer
      
    // If program creation failed, we'll get a zero ID
    if (r_gl_state->rect_shader.program_id == 0) {
      fprintf(stderr, "ERROR: Failed to create rect shader program\n");
  } else {
      fprintf(stderr, "Successfully created rect shader program: %u\n", r_gl_state->rect_shader.program_id);
      r_gl_state->rect_shader.main_texture_uniform_location = texture_location; // Store the location
      
      // Bind the uniform block to its index point for the rect shader
      GLuint block_index = glGetUniformBlockIndex(r_gl_state->rect_shader.program_id, "R_GL_Uniforms_Rect");
      if (block_index != GL_INVALID_INDEX) {
        glUniformBlockBinding(r_gl_state->rect_shader.program_id, block_index, R_GL_UNIFORM_BINDING_POINT_RECT);
      } else {
        fprintf(stderr, "WARNING: Could not find uniform block 'R_GL_Uniforms_Rect' in shader\n");
      }
      // Removed manual glGetUniformLocation for "main_texture"
    }
  }
  
  // NOTE: Blur shader
  {
    // Use global variables for shader source
    const char *vs_source = (const char *)r_gl_g_vshad_kind_source_ptr_table[R_GL_VShadKind_Blur]->str;
    const char *fs_source = (const char *)r_gl_g_pshad_kind_source_ptr_table[R_GL_PShadKind_Blur]->str;
      
    GLint texture_location = 0;
    r_gl_state->blur_shader.program_id = r_gl_create_program(
      vs_source, fs_source, NULL, 
      str8_lit("blur_shader"), 
      &r_gl_state->blur_shader.vertex_shader,
      &r_gl_state->blur_shader.fragment_shader,
      NULL,
      &texture_location); // Pass location pointer
      
    // Successfully created blur shader?
    if (r_gl_state->blur_shader.program_id == 0) {
      fprintf(stderr, "ERROR: Failed to create blur shader program\n");
    } else {
      fprintf(stderr, "Successfully created blur shader program: %u\n", r_gl_state->blur_shader.program_id);
      r_gl_state->blur_shader.main_texture_uniform_location = texture_location; // Store the location
      
      // Bind the uniform block to its index point
      GLuint block_index = glGetUniformBlockIndex(r_gl_state->blur_shader.program_id, "BlurUniforms");
      if (block_index != GL_INVALID_INDEX) {
        glUniformBlockBinding(r_gl_state->blur_shader.program_id, block_index, R_GL_UNIFORM_BINDING_POINT_BLUR);
        fprintf(stderr, "Blur UBO block bound to binding point %d\n", R_GL_UNIFORM_BINDING_POINT_BLUR);
      } else {
        fprintf(stderr, "WARNING: Could not find uniform block 'BlurUniforms' in shader\n");
      }
      
      // Get and store the main texture location
      // r_gl_state->blur_shader.main_texture_uniform_location = texture_location; // Already stored above
    }
  }
  
  // NOTE: Mesh shader
  {
    // Use global variables for shader source
    const char *vs_source = (const char *)r_gl_g_vshad_kind_source_ptr_table[R_GL_VShadKind_Mesh]->str;
    const char *fs_source = (const char *)r_gl_g_pshad_kind_source_ptr_table[R_GL_PShadKind_Mesh]->str;
      
    GLint texture_location = 0;
    r_gl_state->mesh_shader.program_id = r_gl_create_program(
      vs_source, fs_source, NULL, 
      str8_lit("mesh_shader"), 
      &r_gl_state->mesh_shader.vertex_shader,
      &r_gl_state->mesh_shader.fragment_shader,
      NULL,
      &texture_location); // Pass location pointer
      
    if (r_gl_state->mesh_shader.program_id == 0) {
      fprintf(stderr, "ERROR: Failed to create mesh shader program\n");
    } else {
      fprintf(stderr, "Successfully created mesh shader program: %u\n", r_gl_state->mesh_shader.program_id);
      r_gl_state->mesh_shader.main_texture_uniform_location = texture_location; // Store the location
      
      // Bind the uniform block
      GLuint block_index = glGetUniformBlockIndex(r_gl_state->mesh_shader.program_id, "R_GL_Uniforms_Mesh");
      if (block_index != GL_INVALID_INDEX) {
        glUniformBlockBinding(r_gl_state->mesh_shader.program_id, block_index, R_GL_UNIFORM_BINDING_POINT_MESH);
      } else {
        fprintf(stderr, "WARNING: Could not find uniform block 'R_GL_Uniforms_Mesh' in shader\n");
      }
      
      // Get and store the main texture location
      // r_gl_state->mesh_shader.main_texture_uniform_location = texture_location; // Already stored above
      // Removed manual glGetUniformLocation for "main_texture"
    }
  }
  
  // NOTE: Geo3D Composite shader
  {
    // Use global variable for shader source (assuming VS is simple and FS is main source)
    // NOTE: Assuming a simple pass-through VS or defining r_gl_g_geo3dcomposite_shader_vs_src
    const char *vs_source = 
      "#version 330 core\n"
      "out vec2 TexCoord;\n"
      "void main()\n"
      "{\n"
      "  vec2 pos = vec2( (gl_VertexID & 1) * 2.0 - 1.0, (gl_VertexID & 2) - 1.0 ); \n"
      "  gl_Position = vec4(pos, 0.0, 1.0);\n"
      "  TexCoord = pos * 0.5 + 0.5;\n"
      "  TexCoord.y = 1.0 - TexCoord.y;\n"
      "}\n"; // Using inline VS for now as no global was defined
    const char *fs_source = (const char *)r_gl_g_pshad_kind_source_ptr_table[R_GL_PShadKind_Geo3DComposite]->str;

    GLint texture_location = 0; // Get the location via the helper
    r_gl_state->geo3d_composite_shader.program_id = r_gl_create_program(
      vs_source, fs_source, NULL, 
      str8_lit("geo3d_composite_shader"), 
      &r_gl_state->geo3d_composite_shader.vertex_shader,
      &r_gl_state->geo3d_composite_shader.fragment_shader,
      NULL,
      &texture_location); // Pass pointer to get "main_t2d" location
      
    if (r_gl_state->geo3d_composite_shader.program_id == 0) {
      fprintf(stderr, "ERROR: Failed to create geo3d composite shader program\n");
  } else {
      fprintf(stderr, "Successfully created geo3d composite shader program: %u\n", r_gl_state->geo3d_composite_shader.program_id);
      r_gl_state->geo3d_composite_shader.main_texture_uniform_location = texture_location; // Store the location

      // Removed manual glGetUniformLocation and glUniform1i calls here
      // Shader uniforms for composite are now set in the render pass submit logic
    }
  }
  
  // NOTE: Finalize shader (simple blit to screen)
  {
     // Use global variables for shader source
    const char *vs_source = (const char *)r_gl_g_vshad_kind_source_ptr_table[R_GL_VShadKind_FullscreenQuad]->str;
    const char *fs_source = (const char *)r_gl_g_pshad_kind_source_ptr_table[R_GL_PShadKind_Finalize]->str;
      
    GLint texture_location = 0;
    r_gl_state->finalize_shader.program_id = r_gl_create_program(
      vs_source, fs_source, NULL, 
      str8_lit("finalize_shader"), 
      &r_gl_state->finalize_shader.vertex_shader,
      &r_gl_state->finalize_shader.fragment_shader,
      NULL,
      &texture_location); // Pass location pointer
      
    if (r_gl_state->finalize_shader.program_id == 0) {
      fprintf(stderr, "ERROR: Failed to create finalize shader program\n");
    } else {
      fprintf(stderr, "Successfully created finalize shader program: %u\n", r_gl_state->finalize_shader.program_id);
      
      // Get and store the main texture location
      r_gl_state->finalize_shader.main_texture_uniform_location = texture_location; // Store the location
      // Removed manual glGetUniformLocation for "main_texture"
    }
  }
  
  fprintf(stderr, "OpenGL resources created successfully.\n");

  // Mark initialization as complete
  r_gl_state->global_resources_initialized = 1;

  //- Create uniform buffer objects (UBOs)
  {
    // Define UBO sizes for each uniform type
    r_gl_state->uniform_type_kind_sizes[R_GL_UniformTypeKind_Rect] = r_gl_g_uniform_type_kind_size_table[R_GL_UniformTypeKind_Rect];
    r_gl_state->uniform_type_kind_sizes[R_GL_UniformTypeKind_Blur] = r_gl_g_uniform_type_kind_size_table[R_GL_UniformTypeKind_Blur];
    r_gl_state->uniform_type_kind_sizes[R_GL_UniformTypeKind_Mesh] = r_gl_g_uniform_type_kind_size_table[R_GL_UniformTypeKind_Mesh];

    // Create and initialize UBOs for each uniform type
    for (R_GL_UniformTypeKind kind = 0; kind < R_GL_UniformTypeKind_COUNT; kind++)
    {
      GLuint buffer = 0;
      glGenBuffers(1, &buffer);
      glBindBuffer(GL_UNIFORM_BUFFER, buffer);
      
      // Calculate size with proper alignment (multiple of 16 bytes)
      GLsizeiptr size = r_gl_state->uniform_type_kind_sizes[kind];
      size = (size + 15) & ~15; // Round up to multiple of 16
      
      // Initialize the buffer with NULL data (allocate space only)
      glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
      
      // Bind to pre-defined binding point
      glBindBufferBase(GL_UNIFORM_BUFFER, kind, buffer);
      
      // Store the buffer handle
      r_gl_state->uniform_type_kind_buffers[kind] = buffer;
      
      // Unbind the buffer
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
  }

  r_gl_check_error("Post UBO Creation"); // Check errors after UBO setup

  log_infof("Global OpenGL resources created successfully.");
  r_gl_state->global_resources_initialized = 1;

  // ADDED: Process deferred textures *after* global resources are ready
  r_gl_process_deferred_tex2d_queue();
}

// This function initializes GLEW and creates global OpenGL resources.
// It MUST be called after *some* GL context is made current for the first time.
// It's designed to be called idempotently.
internal void
r_gl_init_extensions_if_needed(void)
{
  // Call the split initialization functions for backward compatibility
  r_gl_init_glew_if_needed();
  r_gl_create_global_resources();
}

////////////////////////////////
//~ dan: Backend Hooks Implementation

r_hook void
r_init(CmdLine *cmdln)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc(GB(1)); // Consider arena params
  r_gl_state = push_array(arena, R_GL_State, 1);
  r_gl_state->arena = arena;
  r_gl_state->device_rw_mutex = os_rw_mutex_alloc();
  r_gl_state->global_resources_initialized = 0; // Initialize the flag
  r_gl_state->glew_initialized = 0; // Initialize GLEW flag
  r_gl_state->has_valid_context = 0; // No valid context yet
  
  // Initialize deferred texture allocation
  r_gl_state->deferred_tex2d_arena = arena_alloc();
  r_gl_state->first_deferred_tex2d = NULL;
  r_gl_state->last_deferred_tex2d = NULL;

  // Store UBO sizes (does not require GL context)
  r_gl_state->uniform_type_kind_sizes[R_GL_UniformTypeKind_Rect] = r_gl_g_uniform_type_kind_size_table[R_GL_UniformTypeKind_Rect];
  r_gl_state->uniform_type_kind_sizes[R_GL_UniformTypeKind_Blur] = r_gl_g_uniform_type_kind_size_table[R_GL_UniformTypeKind_Blur];
  r_gl_state->uniform_type_kind_sizes[R_GL_UniformTypeKind_Mesh] = r_gl_g_uniform_type_kind_size_table[R_GL_UniformTypeKind_Mesh];

  // NOTE: All gl* calls are removed from here and moved to r_window_equip / r_gl_create_global_resources
  ProfEnd();
}

// Add this function after r_init but before other functions
internal void 
r_gl_set_has_valid_context(B32 has_context)
{
  log_infof("Setting has_valid_context = %d", has_context);
  r_gl_state->has_valid_context = has_context;
  // Process deferred textures *only* if the context is now valid
  if(has_context)
  {
    // REMOVED: r_gl_process_deferred_tex2d_queue(); 
  }
}

// New function to process the deferred texture allocation queue
internal void
r_gl_process_deferred_tex2d_queue(void)
{
  // Add early return if we don't have a valid context or GLEW isn't initialized
  if(!r_gl_state->has_valid_context || !r_gl_state->glew_initialized)
  {
    fprintf(stderr, "Cannot process deferred textures: has_valid_context=%d, glew_initialized=%d\n",
           r_gl_state->has_valid_context, r_gl_state->glew_initialized);
    return;
  }
  
  // Check if queue is empty
  if(!r_gl_state->first_deferred_tex2d)
  {
    // No deferred textures to process
    return;
  }
  
  fprintf(stderr, "Processing deferred texture queue...\n");
  
  // Continue with normal processing now that we have a valid context
  for(R_GL_DeferredTex2D *n = r_gl_state->first_deferred_tex2d, *next = 0;
      n != 0;
      n = next)
  {
    next = n->next;
    
    // Create the texture now that we have a valid context
    R_GL_Tex2D *tex = n->texture;
    tex->kind = n->kind;
    tex->size = n->size;
    tex->format = n->format;
    
    // Generate texture - clear any previous errors first
    glGetError(); // Clear previous errors
    
    glGenTextures(1, &tex->texture_id);
    GLenum err = glGetError();
    if(err != GL_NO_ERROR)
    {
      fprintf(stderr, "Error in deferred texture creation (glGenTextures): 0x%x\n", err);
      continue; // Skip this texture and move to the next
    }
    
    glBindTexture(GL_TEXTURE_2D, tex->texture_id);
    err = glGetError();
    if(err != GL_NO_ERROR)
    {
      fprintf(stderr, "Error in deferred texture creation (glBindTexture): 0x%x\n", err);
      continue;
    }
    
    // Setup texture based on format
    GLint internal_format = r_gl_internal_format_from_tex2d_format(n->format);
    GLenum format = r_gl_format_from_tex2d_format(n->format);
    GLenum type = r_gl_type_from_tex2d_format(n->format);
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, n->size.x, n->size.y, 0, format, type, n->data);
    err = glGetError();
    if(err != GL_NO_ERROR)
    {
      fprintf(stderr, "Error in deferred texture creation (glTexImage2D): 0x%x\n", err);
      fprintf(stderr, "  format=%d, internal_format=%d, type=%d, size=%dx%d\n", 
              format, internal_format, type, n->size.x, n->size.y);
      continue;
    }
    
    // Set default parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    err = glGetError();
    if(err != GL_NO_ERROR)
    {
      fprintf(stderr, "Error in deferred texture creation (glTexParameteri): 0x%x\n", err);
    }
    
    // Free our copy of the data if we allocated one
    if(n->data_size > 0 && n->data != NULL)
    {
      // arena_pop(r_gl_state->deferred_tex2d_arena, n->data_size); // INCORRECT: Arena pop is LIFO, loop is FIFO.
    }
  }
  
  // Clear the queue
  fprintf(stderr, "Deferred texture queue processed\n");
  r_gl_state->first_deferred_tex2d = NULL;
  r_gl_state->last_deferred_tex2d = NULL;

  // Clear the entire arena now that all data has been used
  if(r_gl_state->deferred_tex2d_arena)
  {
    arena_clear(r_gl_state->deferred_tex2d_arena);
  }
}

r_hook R_Handle
r_window_equip(OS_Handle window)
{
  // This function equips subsequent windows or the first window *after* context creation.
  R_GL_Window *gl_window = 0;
  OS_LNX_Window *lnx_window = (OS_LNX_Window *)window.u64[0];

  fprintf(stderr, "r_window_equip: OS window handle %p, X11 window %lu\n", 
          (void*)lnx_window, lnx_window ? lnx_window->window : 0);
  
  OS_MutexScope(r_gl_state->device_rw_mutex)
  {
    if(r_gl_state->first_free_window)
    {
      gl_window = r_gl_state->first_free_window;
      SLLStackPop(r_gl_state->first_free_window);
      MemoryZeroStruct(gl_window); // Reset struct for reuse
    }
    else
    {
      gl_window = push_array(r_gl_state->arena, R_GL_Window, 1);
    }
    
    // Store OS_LNX_Window pointer as generation to make context current later
    gl_window->generation = (U64)lnx_window;
    gl_window->last_resolution = v2s32(0, 0); // Force resize on first frame
    
    // Store the display pointer in r_gl_state for later use
    r_gl_state->display = os_lnx_gfx_state->display;
    
    // Print OpenGL info
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    fprintf(stderr, "OpenGL Renderer: %s\n", renderer ? (const char*)renderer: "unknown");
    fprintf(stderr, "OpenGL Version: %s\n", version ? (const char*)version : "unknown");
  }

  R_Handle result = r_gl_handle_from_window(gl_window);
  return result;
}

// Helper function to delete window FBO resources
internal void
r_gl_delete_window_fbo_resources(R_GL_Window *gl_window)
{
    // Delete FBOs
    if (gl_window->stage_fbo) { glDeleteFramebuffers(1, &gl_window->stage_fbo); gl_window->stage_fbo = 0; }
    if (gl_window->stage_scratch_fbo) { glDeleteFramebuffers(1, &gl_window->stage_scratch_fbo); gl_window->stage_scratch_fbo = 0; }
    if (gl_window->geo3d_fbo) { glDeleteFramebuffers(1, &gl_window->geo3d_fbo); gl_window->geo3d_fbo = 0; }
    r_gl_check_error("FBO Deletion");

    // Delete Textures used as attachments
    if (gl_window->stage_color_texture) { glDeleteTextures(1, &gl_window->stage_color_texture); gl_window->stage_color_texture = 0; }
    if (gl_window->stage_scratch_color_texture) { glDeleteTextures(1, &gl_window->stage_scratch_color_texture); gl_window->stage_scratch_color_texture = 0; }
    if (gl_window->geo3d_color_texture) { glDeleteTextures(1, &gl_window->geo3d_color_texture); gl_window->geo3d_color_texture = 0; }
    if (gl_window->geo3d_depth_texture) { glDeleteTextures(1, &gl_window->geo3d_depth_texture); gl_window->geo3d_depth_texture = 0; }
    r_gl_check_error("FBO Texture Deletion");

    // Reset resolution to force recreation
    gl_window->last_resolution = v2s32(0, 0);
}

// Implementation of the make context current function
internal void
r_gl_make_context_current(R_GL_Window *window)
{
  if (window != NULL && window != &r_gl_window_nil) {
    OS_LNX_Window *os_window = (OS_LNX_Window *)window->generation;
    if (os_window && os_window->gl_context) {
      Display *display = os_lnx_gfx_state->display;
      
      // Check if this context is already current
      GLXContext current = glXGetCurrentContext();
      Window current_drawable = glXGetCurrentDrawable();
      
      if (current != os_window->gl_context || current_drawable != os_window->window) {
        fprintf(stderr, "Making context current: display=%p, window=%lu, context=%p\n", 
                display, os_window->window, os_window->gl_context);
                
        if (!glXMakeCurrent(display, os_window->window, os_window->gl_context)) {
          fprintf(stderr, "ERROR: Failed to make context current!\n");
          
          // Try to diagnose the error
          if (!display) {
            fprintf(stderr, "  - Display is NULL\n");
          }
          if (os_window->window == 0) {
            fprintf(stderr, "  - Window is 0 (None)\n");
          }
          if (!os_window->gl_context) {
            fprintf(stderr, "  - GL Context is NULL\n");
          }
        } else {
          // Verify the context was made current
          GLXContext new_current = glXGetCurrentContext();
          Window new_drawable = glXGetCurrentDrawable();
          
          if (new_current == os_window->gl_context && new_drawable == os_window->window) {
            fprintf(stderr, "Successfully made context current\n");
            
            // Check if we can make GL calls
            GLint max_texture_size = 0;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
            GLenum err = glGetError();
            
            if (err != GL_NO_ERROR || max_texture_size == 0) {
              fprintf(stderr, "WARNING: Context made current but GL calls failing (err=0x%x, max_tex_size=%d)\n",
                      err, max_texture_size);
            }
          } else {
            fprintf(stderr, "ERROR: Context not current after glXMakeCurrent! current=%p, drawable=%lu\n",
                    new_current, new_drawable);
          }
        }
      }
    } else {
      fprintf(stderr, "ERROR: Invalid OS window or GL context in r_gl_make_context_current\n");
      if (!os_window) {
        fprintf(stderr, "  - OS window is NULL (window->generation=%lu)\n", window->generation);
      } else if (!os_window->gl_context) {
        fprintf(stderr, "  - gl_context is NULL\n");
      }
    }
  } else {
    fprintf(stderr, "ERROR: Invalid R_GL_Window in r_gl_make_context_current\n");
  }
}

r_hook void
r_window_unequip(OS_Handle window, R_Handle window_equip)
{
  R_GL_Window *gl_window = r_gl_window_from_handle(window_equip);
  if(gl_window != &r_gl_window_nil)
  {
    // Check if we're unequipping the currently active window
    if (glXGetCurrentContext() == ((OS_LNX_Window *)gl_window->generation)->gl_context) {
      r_gl_make_context_current(NULL); // Switch to another valid context
    }
    
  r_gl_delete_window_fbo_resources(gl_window);
        SLLStackPush(r_gl_state->first_free_window, gl_window);
    }
}

r_hook R_Handle
r_tex2d_alloc(R_ResourceKind kind, Vec2S32 size, R_Tex2DFormat format, void *data)
{
  ProfBeginFunction();
  R_Handle result = {0};
  
  // Check if we need to defer allocation due to no OpenGL context
  if(!r_gl_state->has_valid_context)
  {
    // Allocate a texture object that will be populated later
    R_GL_Tex2D *texture = r_gl_state->first_free_tex2d;
    if(texture)
    {
      SLLStackPop(r_gl_state->first_free_tex2d);
    }
    else
    {
      texture = push_array(r_gl_state->arena, R_GL_Tex2D, 1);
    }
    texture->generation += 1;
    
    // Create the deferred allocation entry
    if(!r_gl_state->deferred_tex2d_arena)
    {
      r_gl_state->deferred_tex2d_arena = arena_alloc();
    }
    
    R_GL_DeferredTex2D *deferred = push_array(r_gl_state->deferred_tex2d_arena, R_GL_DeferredTex2D, 1);
    deferred->kind = kind;
    deferred->size = size;
    deferred->format = format;
    deferred->texture = texture;
    
    // Copy the data if provided
    if(data)
    {
      size_t data_size = size.x * size.y * r_tex2d_format_bytes_per_pixel_table[format];
      void *data_copy = push_array(r_gl_state->deferred_tex2d_arena, U8, data_size);
      MemoryCopy(data_copy, data, data_size);
      deferred->data = data_copy;
      deferred->data_size = data_size;
    }
    
    // Add to deferred list
    SLLQueuePush(r_gl_state->first_deferred_tex2d, r_gl_state->last_deferred_tex2d, deferred);
    
    // Return handle that will be valid when processed later
    result = r_gl_handle_from_tex2d(texture);
    
    fprintf(stderr, "Warning: r_tex2d_alloc called with no active GL context. Deferring creation.\n");
    
    ProfEnd();
    return result;
  }
  
  // Original implementation for when we have a valid context
  // Allocate the texture object
  R_GL_Tex2D *texture = r_gl_state->first_free_tex2d;
  if(texture)
  {
    SLLStackPop(r_gl_state->first_free_tex2d);
  }
  else
  {
    texture = push_array(r_gl_state->arena, R_GL_Tex2D, 1);
  }
  texture->generation += 1;
  
  // Create the OpenGL texture
  glGenTextures(1, &texture->texture_id);
  r_gl_check_error("r_tex2d_alloc - glGenTextures");
  
  glBindTexture(GL_TEXTURE_2D, texture->texture_id);
r_gl_check_error("UI Bind Texture");
  r_gl_check_error("r_tex2d_alloc - glBindTexture");
  
  // Get format information
  GLenum gl_format = r_gl_format_from_tex2d_format(format);
  GLint gl_internal_format = r_gl_internal_format_from_tex2d_format(format);
  GLenum gl_type = r_gl_type_from_tex2d_format(format);
  
  // Configure the texture
  glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, 
               size.x, size.y, 
               0, gl_format, gl_type, data);
  r_gl_check_error("r_tex2d_alloc - glTexImage2D");
  
  // Set filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  r_gl_check_error("r_tex2d_alloc - glTexParameteri");
  
  glBindTexture(GL_TEXTURE_2D, 0);
  
  // Store metadata in the texture object
  texture->kind = kind;
  texture->size = size;
  texture->format = format;
  
  result = r_gl_handle_from_tex2d(texture);
  ProfEnd();
  return result;
}

r_hook void
r_tex2d_release(R_Handle handle)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_gl_state->device_rw_mutex)
{
  R_GL_Tex2D *texture = r_gl_tex2d_from_handle(handle);
    SLLStackPush(r_gl_state->first_to_free_tex2d, texture);
  }
  ProfEnd();
}

r_hook R_Handle
r_buffer_alloc(R_ResourceKind kind, U64 size, void *data)
{
  ProfBeginFunction();
  
  // NEW: Check if we have a valid OpenGL context
  GLXContext current_ctx = glXGetCurrentContext();
  if (!current_ctx) {
    // No valid context - can't create buffers yet
    fprintf(stderr, "Warning: r_buffer_alloc called with no active GL context. Deferring creation.\n");
    R_Handle result = {0};
    return result;
  }
  
  //- dan: allocate buffer structure
  R_GL_Buffer *buffer = 0;
  OS_MutexScopeW(r_gl_state->device_rw_mutex)
  {
    buffer = r_gl_state->first_free_buffer;
    if(buffer == 0)
    {
      buffer = push_array(r_gl_state->arena, R_GL_Buffer, 1);
    }
    else
    {
      U64 gen = buffer->generation;
      SLLStackPop(r_gl_state->first_free_buffer);
      MemoryZeroStruct(buffer);
      buffer->generation = gen;
    }
    buffer->generation += 1;
  }
  
  //- dan: verify static buffer has data
  if(kind == R_ResourceKind_Static)
  {
    Assert(data != 0 && "static buffer must have initial data provided");
  }
  
  //- dan: get buffer usage from resource kind
  GLenum usage = r_gl_usage_from_resource_kind(kind);
  
  //- dan: create buffer object
  GLuint buffer_id = 0;
  glGenBuffers(1, &buffer_id);
  r_gl_check_error("glGenBuffers");
  
  //- dan: bind to appropriate target and upload data
  // Default to vertex buffer, but support both targets
  GLenum target = GL_ARRAY_BUFFER;
  
  glBindBuffer(target, buffer_id);
  r_gl_check_error("glBindBuffer");
  
  // Upload the data or allocate space
  glBufferData(target, size, data, usage);
  r_gl_check_error("glBufferData");
  
  // Unbind the buffer
  glBindBuffer(target, 0);
  
  //- dan: fill buffer structure
  buffer->buffer_id = buffer_id;
  buffer->kind = kind;
  buffer->size = size;
  buffer->target = target;
  
  R_Handle result = r_gl_handle_from_buffer(buffer);
  ProfEnd();
  return result;
}

r_hook void
r_buffer_release(R_Handle handle)
{
  ProfBeginFunction();
    OS_MutexScopeW(r_gl_state->device_rw_mutex)
    {
    R_GL_Buffer *buffer = r_gl_buffer_from_handle(handle);
    SLLStackPush(r_gl_state->first_to_free_buffer, buffer);
  }
  ProfEnd();
}

r_hook void
r_buffer_write(R_Handle handle, Rng1U64 range, void *data)
{
  R_GL_Buffer *buffer = r_gl_buffer_from_handle(handle);
  if(buffer != 0 && buffer != &r_gl_buffer_nil)
  {
    //- dan: check writability
    Assert(buffer->kind == R_ResourceKind_Dynamic || buffer->kind == R_ResourceKind_Stream);

    //- dan: clamp range
    Rng1U64 buffer_range = r1u64(0, buffer->size);
    Rng1U64 write_range = range;
    write_range.min = ClampTop(write_range.min, buffer_range.max);
    write_range.max = ClampTop(write_range.max, buffer_range.max);
    U64 write_size = dim_1u64(write_range);

    if(write_size > 0)
    {
      //- dan: bind and upload data
      // NOTE: Binding to GL_ARRAY_BUFFER here is just for the glBufferSubData call.
      glBindBuffer(GL_ARRAY_BUFFER, buffer->buffer_id);
      r_gl_check_error("glBindBuffer Write");
      glBufferSubData(GL_ARRAY_BUFFER, write_range.min, write_size, data);
      r_gl_check_error("glBufferSubData");
      glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind
    }
  }
}

r_hook void
r_begin_frame(void)
{
  OS_MutexScopeW(r_gl_state->device_rw_mutex)
  {
    // dan: delete queued-up resources
    // ... existing code ...
  }
}

r_hook void
r_end_frame(void)
{
  OS_MutexScopeW(r_gl_state->device_rw_mutex)
  {
    // Clean up textures that were marked for deletion
    for(R_GL_Tex2D *tex = r_gl_state->first_to_free_tex2d, *next = 0;
        tex != 0;
        tex = next)
    {
      next = tex->next;
      if(tex->texture_id != 0)
      {
        glDeleteTextures(1, &tex->texture_id);
        tex->texture_id = 0;
      }
      tex->generation += 1;
      SLLStackPush(r_gl_state->first_free_tex2d, tex);
    }
    r_gl_state->first_to_free_tex2d = 0;
    
    // Clean up buffers that were marked for deletion
    for(R_GL_Buffer *buf = r_gl_state->first_to_free_buffer, *next = 0;
        buf != 0;
        buf = next)
    {
      next = buf->next;
      if(buf->buffer_id != 0)
      {
        glDeleteBuffers(1, &buf->buffer_id);
        buf->buffer_id = 0;
      }
      buf->generation += 1;
      SLLStackPush(r_gl_state->first_free_buffer, buf);
    }
    r_gl_state->first_to_free_buffer = 0;
    
    // Clean up any framebuffers/renderbuffers marked for deletion
    for(R_GL_Framebuffer *fbo = r_gl_state->first_to_free_framebuffer, *fbo_next = 0;
        fbo != 0;
        fbo = fbo_next)
    {
      fbo_next = fbo->next;
      if(fbo->fbo_id != 0)
      {
        glDeleteFramebuffers(1, &fbo->fbo_id);
      }
      SLLStackPush(r_gl_state->first_free_framebuffer, fbo);
    }
    r_gl_state->first_to_free_framebuffer = 0;
    
    for(R_GL_Renderbuffer *rbo = r_gl_state->first_to_free_renderbuffer, *rbo_next = 0;
        rbo != 0;
        rbo = rbo_next)
    {
      rbo_next = rbo->next;
      if(rbo->rbo_id != 0)
      {
        glDeleteRenderbuffers(1, &rbo->rbo_id);
      }
      SLLStackPush(r_gl_state->first_free_renderbuffer, rbo);
    }
    r_gl_state->first_to_free_renderbuffer = 0;
    
    // Clear buffer flush arena if it exists
    if(r_gl_state->buffer_flush_arena)
    {
      arena_clear(r_gl_state->buffer_flush_arena);
    }
    
    r_gl_check_error("r_end_frame cleanup");
  }

  // debug_log_current_context();
}

r_hook void
r_window_begin_frame(OS_Handle window_handle, R_Handle window_equip) // Renamed OS_Handle param
{
  ProfBeginFunction();
  
    R_GL_Window *gl_window = r_gl_window_from_handle(window_equip);
  if (gl_window == &r_gl_window_nil) { return; } // Early exit if window handle is invalid

  // Ensure context is current for this window
    r_gl_make_context_current(gl_window);
  if (!r_gl_state->has_valid_context)
  {
      log_infof("r_window_begin_frame: No valid context, skipping frame for window %p", gl_window);
      return; // Cannot proceed without a valid context
  }
  
  // ADDED: Create global resources if they haven't been initialized yet.
  // This ensures resources are created *after* a context is successfully made current.
  if (!r_gl_state->global_resources_initialized)
  {
      log_infof("r_window_begin_frame: Creating global resources for the first time.");
      r_gl_create_global_resources();
      // Check if resource creation failed (might happen if context became invalid)
      if (!r_gl_state->global_resources_initialized)
      {
          log_infof("r_window_begin_frame: Failed to create global resources, skipping frame.");
          return;
      }
  }

  // Get current window dimensions
    Rng2F32 client_rect = os_client_rect_from_window(window_handle);
    Vec2S32 resolution = v2s32((S32)client_rect.x1, (S32)client_rect.y1);
    
    // Ensure minimum resolution
    if (resolution.x < 1) resolution.x = 1;
    if (resolution.y < 1) resolution.y = 1;
    
    // Debug info
    // fprintf(stderr, "Window resolution: %d x %d\\n", resolution.x, resolution.y); // Less verbose
    
    //- dan: check / allocate FBO resources if needed
    if(gl_window->last_resolution.x != resolution.x ||
       gl_window->last_resolution.y != resolution.y ||
       gl_window->stage_fbo == 0)
    {
      fprintf(stderr, "Creating/resizing FBOs to %d x %d\\n", resolution.x, resolution.y);
      
      // Delete any existing resources
      r_gl_delete_window_fbo_resources(gl_window);
      
      // Create stage FBO and attachments
      glGenTextures(1, &gl_window->stage_color_texture);
      glBindTexture(GL_TEXTURE_2D, gl_window->stage_color_texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, resolution.x, resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      // Set wrap modes explicitly (good practice)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      r_gl_check_error("stage color texture");
      
      // REMOVED: Depth texture creation for stage_fbo
      // glGenTextures(1, &gl_window->stage_depth_texture);
      // glBindTexture(GL_TEXTURE_2D, gl_window->stage_depth_texture);
      // glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, resolution.x, resolution.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
      // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      // r_gl_check_error("stage depth texture");
      
      glGenFramebuffers(1, &gl_window->stage_fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_window->stage_fbo);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_window->stage_color_texture, 0);
      // REMOVED: Depth texture attachment for stage_fbo
      // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gl_window->stage_depth_texture, 0);
      
      // Check FBO completeness
      GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      const char* status_str = "Unknown";
      switch(status) {
          case GL_FRAMEBUFFER_COMPLETE: status_str = "Complete"; break;
          case GL_FRAMEBUFFER_UNDEFINED: status_str = "Undefined"; break; // Added GL_FRAMEBUFFER_UNDEFINED
          case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: status_str = "Incomplete Attachment"; break;
          case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: status_str = "Missing Attachment"; break;
          case GL_FRAMEBUFFER_UNSUPPORTED: status_str = "Unsupported"; break;
          case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: status_str = "Incomplete Draw Buffer"; break;
          case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: status_str = "Incomplete Read Buffer"; break;
          case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: status_str = "Incomplete Multisample"; break; // Added Multisample
          case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: status_str = "Incomplete Layer Targets"; break; // Added Layer Targets
      }
      fprintf(stderr, "stage_fbo status: %s (0x%x)\\n", status_str, status);
      
      if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "ERROR: stage_fbo is not complete! Status: 0x%x\\n", status);
        // Fall back to default framebuffer or handle error appropriately
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Potentially delete the incomplete FBO resources?
        // r_gl_delete_window_fbo_resources(gl_window); // Careful not to infinite loop if creation always fails
      } else {
        fprintf(stderr, "stage_fbo is complete: %u\\n", gl_window->stage_fbo);
      }
      
      // Create scratch FBO (if needed for effects like blur)
      glGenTextures(1, &gl_window->stage_scratch_color_texture);
      glBindTexture(GL_TEXTURE_2D, gl_window->stage_scratch_color_texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, resolution.x, resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      r_gl_check_error("scratch color texture");
      
      glGenFramebuffers(1, &gl_window->stage_scratch_fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, gl_window->stage_scratch_fbo);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_window->stage_scratch_color_texture, 0);
      
      // Check scratch FBO completeness
      status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "ERROR: stage_scratch_fbo is not complete! Status: 0x%x\n", status);
    } else {
        fprintf(stderr, "stage_scratch_fbo is complete\n");
      }
      
      gl_window->last_resolution = resolution;
    }

    // Now bind the stage FBO
    GLenum bind_target = (gl_window->stage_fbo != 0) ? gl_window->stage_fbo : 0;
    glBindFramebuffer(GL_FRAMEBUFFER, bind_target);
    r_gl_check_error("binding framebuffer");
    
    // Set up viewport
    glViewport(0, 0, resolution.x, resolution.y);
    
    // Clear with a dark gray
    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable blending for UI
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    r_gl_check_error("UI Blend Func");
}

r_hook void
r_window_end_frame(OS_Handle window_handle, R_Handle window_equip) // Renamed OS_Handle param
{
  OS_MutexScope(r_gl_state->device_rw_mutex)
  {
    R_GL_Window *gl_window = r_gl_window_from_handle(window_equip);
    if(gl_window == &r_gl_window_nil) { return; }

    // Get OS window
    OS_LNX_Window *lnx_window = (OS_LNX_Window *)window_handle.u64[0];
    if (!lnx_window) {
      fprintf(stderr, "ERROR: Invalid OS window handle\n");
      return;
    }
    
    // Render final stage texture to screen using finalize shader
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind default framebuffer
        r_gl_check_error("bind default framebuffer finalize");

      Vec2S32 size = gl_window->last_resolution;
        glViewport(0, 0, size.x, size.y);
        r_gl_check_error("glViewport finalize");

        // Set pipeline state for simple blit
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
r_gl_check_error("UI Depth Test Disable");
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);
        r_gl_check_error("pipeline state finalize");

        // Bind shader and VAO
        glUseProgram(r_gl_state->finalize_shader.program_id);
        r_gl_check_error("glUseProgram finalize");
        glBindVertexArray(r_gl_state->fullscreen_vao);
        r_gl_check_error("glBindVertexArray finalize");

        // Bind texture and sampler
        glActiveTexture(GL_TEXTURE0);
r_gl_check_error("UI Active Texture");
        glBindTexture(GL_TEXTURE_2D, gl_window->stage_color_texture);
        r_gl_check_error("glBindTexture finalize");
        glBindSampler(0, r_gl_state->samplers[R_Tex2DSampleKind_Nearest]); // Nearest for final blit
        r_gl_check_error("glBindSampler finalize");

        // Set texture uniform location
        if(r_gl_state->finalize_shader.main_texture_uniform_location != -1)
        {
            glUniform1i(r_gl_state->finalize_shader.main_texture_uniform_location, 0); // Texture unit 0
            r_gl_check_error("glUniform1i finalize");
        }

        // Draw fullscreen quad
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        r_gl_check_error("glDrawArrays finalize");
    }
    
    // Swap buffers
    glXSwapBuffers(os_lnx_gfx_state->display, lnx_window->window);
    
    // Unbind anything we've bound
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

r_hook R_ResourceKind
r_kind_from_tex2d(R_Handle handle)
{
  R_GL_Tex2D *texture = r_gl_tex2d_from_handle(handle);
  return texture->kind;
}

r_hook Vec2S32
r_size_from_tex2d(R_Handle handle)
{
  R_GL_Tex2D *texture = r_gl_tex2d_from_handle(handle);
  return texture->size;
}

r_hook R_Tex2DFormat
r_format_from_tex2d(R_Handle handle)
{
  R_GL_Tex2D *texture = r_gl_tex2d_from_handle(handle);
  return texture->format;
}

r_hook void
r_fill_tex2d_region(R_Handle handle, Rng2S32 subrect, void *data)
{
  OS_MutexScopeW(r_gl_state->device_rw_mutex)
{
  R_GL_Tex2D *texture = r_gl_tex2d_from_handle(handle);
  if(texture == &r_gl_tex2d_nil || texture->texture_id == 0)
  {
    return;
  }

  // Ensure the texture kind allows updates
  // Assert(texture->kind == R_ResourceKind_Dynamic || texture->kind == R_ResourceKind_Stream);
  if (texture->kind == R_ResourceKind_Static) {
      // Cannot update static textures
      return;
  }

  Vec2S32 size = v2s32(subrect.x1 - subrect.x0, subrect.y1 - subrect.y0);
  if (size.x <= 0 || size.y <= 0) {
      return; // Invalid region
  }

  GLenum format = r_gl_format_from_tex2d_format(texture->format);
  GLenum type = r_gl_type_from_tex2d_format(texture->format);
  U64 bytes_per_pixel = r_tex2d_format_bytes_per_pixel_table[texture->format];

  // Set appropriate unpack alignment
  // Common alignments are 1, 2, 4, 8. Pixel data rows must start on multiples of this alignment.
  GLint previous_alignment;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_alignment);
  r_gl_check_error("glGetIntegerv GL_UNPACK_ALIGNMENT"); // Check error after get
  if (bytes_per_pixel == 1) {
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  }
  else if (bytes_per_pixel == 2) {
      glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
  }
  else {
      glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // Default for 3, 4, 8, 16 bytes per pixel usually safe
  }
  r_gl_check_error("glPixelStorei UNPACK");

  glBindTexture(GL_TEXTURE_2D, texture->texture_id);
r_gl_check_error("UI Bind Texture");
  r_gl_check_error("glBindTexture FillRegion");

  glTexSubImage2D(
                  GL_TEXTURE_2D,    // target
                  0,                // level (mipmap level)
                  subrect.x0,       // xoffset
                  subrect.y0,       // yoffset
                  size.x,           // width
                  size.y,           // height
                  format,           // format of pixel data
                  type,             // type of pixel data
                  data              // pointer to image data
                  );
  r_gl_check_error("glTexSubImage2D");

  glBindTexture(GL_TEXTURE_2D, 0);

  // Restore previous unpack alignment
  glPixelStorei(GL_UNPACK_ALIGNMENT, previous_alignment);
  r_gl_check_error("glPixelStorei UNPACK Restore");
  }
}

//////////////////////////////////
//~ dan: Primitive Topology Mapping

internal GLenum
r_gl_primitive_from_topology(R_GeoTopologyKind kind)
{
  GLenum result = GL_TRIANGLES; // Default
  switch(kind)
  {
    case R_GeoTopologyKind_Lines:         result = GL_LINES; break;
    case R_GeoTopologyKind_LineStrip:     result = GL_LINE_STRIP; break;
    case R_GeoTopologyKind_Triangles:     result = GL_TRIANGLES; break;
    case R_GeoTopologyKind_TriangleStrip: result = GL_TRIANGLE_STRIP; break;
    default: Assert(!"Invalid primitive topology kind"); break; // Should not happen
  }
  return result;
}

//////////////////////////////////
//~ dan: UBO Binding Points

#define R_GL_UNIFORM_BINDING_POINT_RECT 0
#define R_GL_UNIFORM_BINDING_POINT_BLUR 1
#define R_GL_UNIFORM_BINDING_POINT_MESH 2
// #define R_GL_UNIFORM_BINDING_POINT_UI 3 // Define if UI UBO is used

// Note: The UBO/Instance struct definitions are in render_opengl.h
// Do NOT redefine them here - use those definitions


r_hook void
r_window_submit(OS_Handle window_handle, R_Handle window_equip, R_PassList *passes)
{
  OS_MutexScopeW(r_gl_state->device_rw_mutex)
  {
        //- dan: unpack arguments
        R_GL_Window *gl_window = r_gl_window_from_handle(window_equip);
        Vec2S32 resolution = gl_window->last_resolution;
        
        fprintf(stderr, "r_window_submit called - window resolution: %d x %d, FBO: %u\n", 
                resolution.x, resolution.y, gl_window->stage_fbo);
        
        // Verify we have valid FBOs
        if (gl_window->stage_fbo == 0) {
            fprintf(stderr, "ERROR: Invalid stage FBO in r_window_submit!\n");
        }

        // Ensure the correct GL context is current (should be set by r_window_begin_frame)
        // TODO: Add check if necessary: if (glXGetCurrentContext() != gl_window->context) { /* make current or error */ }

        //- dan: do passes
        for(R_PassNode *pass_n = passes->first; pass_n != 0; pass_n = pass_n->next)
        {
            R_Pass *pass = &pass_n->v;
            fprintf(stderr, "Processing pass kind: %d\n", pass->kind);
            
            switch(pass->kind)
            {
                default:{}break;

                ////////////////////////
                //- dan: ui rendering pass
                //
                case R_PassKind_UI:
                {
                    //- dan: unpack params
                    R_PassParams_UI *params = pass->params_ui;
                    R_BatchGroup2DList *rect_batch_groups = &params->rects;
                    
                    // Count the batch groups manually instead of using node_count
                    int batch_group_count = 0;
                    for(R_BatchGroup2DNode *node = rect_batch_groups->first; node != NULL; node = node->next) {
                        batch_group_count++;
                    }
                    fprintf(stderr, "UI pass has %d batch groups\n", batch_group_count);

                    //- dan: set common GL state for UI
                    glEnable(GL_BLEND);
                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

r_gl_check_error("UI Blend Func");
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
r_gl_check_error("UI Depth Test Disable");
                    glEnable(GL_SCISSOR_TEST);
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                    glFrontFace(GL_CW);
                    glViewport(0, 0, resolution.x, resolution.y); // Full window viewport for UI passes
                    
                    // Check if VAO and shader program are valid before using them
                    if (r_gl_state->rect_vao == 0 || r_gl_state->rect_shader.program_id == 0) {
                        fprintf(stderr, "Error: UI rendering resources not valid (VAO: %u, Shader: %u)\n", 
                                r_gl_state->rect_vao, r_gl_state->rect_shader.program_id);
                        break; // Skip this pass if resources are invalid
                    }
                    
                    glBindVertexArray(r_gl_state->rect_vao);
r_gl_check_error("UI Bind VAO");
                    glUseProgram(r_gl_state->rect_shader.program_id);
                    GLenum err = glGetError();
                    if (err != GL_NO_ERROR) {
                        fprintf(stderr, "GL Error after binding VAO/shader: 0x%x\n", err);
                    }

                    //- dan: draw each batch group
                    for(R_BatchGroup2DNode *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
                    {
                        R_BatchList *batches = &group_n->batches;
                        R_BatchGroup2DParams *group_params = &group_n->params;

                        // <<< ADD LOGGING HERE >>>
                        fprintf(stderr, "  UI Batch Group:\n");
                        fprintf(stderr, "    Texture Handle: %lu\n", group_params->tex.u64[0]); // Fixed format specifier
                        fprintf(stderr, "    Sample Kind: %d\n", group_params->tex_sample_kind);
                        fprintf(stderr, "    Transform: [[%.2f, %.2f, %.2f], [%.2f, %.2f, %.2f], [%.2f, %.2f, %.2f]]\n",
                                group_params->xform.v[0][0], group_params->xform.v[0][1], group_params->xform.v[0][2],
                                group_params->xform.v[1][0], group_params->xform.v[1][1], group_params->xform.v[1][2],
                                group_params->xform.v[2][0], group_params->xform.v[2][1], group_params->xform.v[2][2]);
                        fprintf(stderr, "    Clip Rect: (%.1f, %.1f) -> (%.1f, %.1f)\n",
                                group_params->clip.x0, group_params->clip.y0, group_params->clip.x1, group_params->clip.y1);
                        fprintf(stderr, "    Transparency: %.2f\n", group_params->transparency);


                        // dan: bind sampler
                        glBindSampler(0, r_gl_state->samplers[group_params->tex_sample_kind]); // Bind to texture unit 0
                        r_gl_check_error("glBindSampler UI");

                        // dan: bind texture
                        R_GL_Tex2D *texture = r_gl_tex2d_from_handle(group_params->tex);
                        if(texture == &r_gl_tex2d_nil || texture->texture_id == 0)
                        {
                            texture = r_gl_tex2d_from_handle(r_gl_state->backup_texture);
                        }
                        glActiveTexture(GL_TEXTURE0);
r_gl_check_error("UI Active Texture");
                        glBindTexture(GL_TEXTURE_2D, texture->texture_id);
r_gl_check_error("UI Bind Texture");
                        if(r_gl_state->rect_shader.main_texture_uniform_location != -1)
                        {
                            glUniform1i(r_gl_state->rect_shader.main_texture_uniform_location, 0); // Texture unit 0
                        }
                        else
                        {
                            // Try with the known uniform name if location wasn't found during initialization
                            GLint tex_loc = glGetUniformLocation(r_gl_state->rect_shader.program_id, "main_t2d");
                            if (tex_loc != -1) {
                                glUniform1i(tex_loc, 0);
                                // Cache it for future use
                                r_gl_state->rect_shader.main_texture_uniform_location = tex_loc;
                            }
                        }
                        r_gl_check_error("Texture Binding UI");


                        // dan: upload uniforms
                        {
                            R_GL_Uniforms_Rect uniforms = {0};
                            uniforms.viewport_size_px = v2f32(resolution.x, resolution.y);
                            uniforms.opacity = 1.0f - group_params->transparency;
                            uniforms.texture_t2d_size_px = v2f32(texture->size.x > 0 ? texture->size.x : 1.f,
                                                                 texture->size.y > 0 ? texture->size.y : 1.f); // Avoid divide by zero

                            // Texture channel mapping based on format
                            // DEFAULT TO IDENTITY for standard RGBA/BGRA formats.
                            Mat4x4F32 tex_map_matrix = m4x4f32(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1); // Identity

                            // EXPLICITLY handle single-channel R8 format.
                            if(texture->format == R_Tex2DFormat_R8)
                            {
                                // Map R channel -> R,G,B,A for grayscale rendering in the shader.
                                // GLSL matrix multiplication means the first column of the uniform matrix affects the result.
                                // Target: (R,R,R,R) = Sample(R,G,B,A) * MapMatrix
                                tex_map_matrix = m4x4f32(1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0);
                            }
                            // OPTIONAL: Add explicit handling for other formats like BGRA if necessary.
                            // else if (texture->format == R_Tex2DFormat_BGRA8) { ...swizzle matrix... }

                            // Transpose C row-major matrix -> GLSL std140 column-major format for the UBO.
                            Mat4x4F32 transposed_tex_map = transpose_4x4f32(tex_map_matrix);
                            // Assign the transposed matrix columns to the uniform structure fields.
                            for (int i = 0; i < 4; i++) {
                                uniforms.texture_sample_channel_map[i] = v4f32(
                                    transposed_tex_map.v[i][0],
                                    transposed_tex_map.v[i][1],
                                    transposed_tex_map.v[i][2],
                                    transposed_tex_map.v[i][3]
                                );
                            }

                            // Transform: C Mat3x3F32 (row-major) to GLSL mat3 (column-major in std140)
                            Mat3x3F32 c_xform = group_params->xform;
                            // Manually transpose and copy into the std140 padded structure
                            // Remember GLSL mat3 is represented as 3 Vec4F32 columns in std140 layout
                            uniforms.xform[0] = v4f32(c_xform.v[0][0], c_xform.v[1][0], c_xform.v[2][0], 0); // Col 0
                            uniforms.xform[1] = v4f32(c_xform.v[0][1], c_xform.v[1][1], c_xform.v[2][1], 0); // Col 1
                            uniforms.xform[2] = v4f32(c_xform.v[0][2], c_xform.v[1][2], c_xform.v[2][2], 0); // Col 2

                            // Calculate xform_scale from the GLSL-layout columns (before sending to GPU)
                            Vec2F32 gl_col0 = v2f32(uniforms.xform[0].x, uniforms.xform[0].y);
                            Vec2F32 gl_col1 = v2f32(uniforms.xform[1].x, uniforms.xform[1].y);
                            uniforms.xform_scale.x = length_2f32(gl_col0);
                            uniforms.xform_scale.y = length_2f32(gl_col1);
                            uniforms.xform_scale.x = Max(uniforms.xform_scale.x, 0.0001f); // Avoid zero scale
                            uniforms.xform_scale.y = Max(uniforms.xform_scale.y, 0.0001f);

                            // <<< ADD LOGGING HERE >>>
                            fprintf(stderr, "    Uniforms Upload:\n");
                            fprintf(stderr, "      Viewport: %.1f x %.1f\n", uniforms.viewport_size_px.x, uniforms.viewport_size_px.y);
                            fprintf(stderr, "      Opacity: %.2f\n", uniforms.opacity);
                            fprintf(stderr, "      Tex Size: %.1f x %.1f\n", uniforms.texture_t2d_size_px.x, uniforms.texture_t2d_size_px.y);
                            // Optional: Log matrices if needed, they can be verbose
                            // fprintf(stderr, "      Tex Map Matrix: ...\n");
                            // fprintf(stderr, "      Xform Matrix (ColMajor): [[%.2f, %.2f, %.2f], [%.2f, %.2f, %.2f], [%.2f, %.2f, %.2f]]\n", ...);
                            fprintf(stderr, "      Xform Scale: %.2f, %.2f\n", uniforms.xform_scale.x, uniforms.xform_scale.y);


                            // Bind and update UBO
                            glBindBufferBase(GL_UNIFORM_BUFFER, R_GL_UNIFORM_BINDING_POINT_RECT, r_gl_state->uniform_type_kind_buffers[R_GL_UniformTypeKind_Rect]);
                            glBindBuffer(GL_UNIFORM_BUFFER, r_gl_state->uniform_type_kind_buffers[R_GL_UniformTypeKind_Rect]); // Rebind for glBufferSubData
                            r_gl_check_error("glBindBufferBase Rect UBO");
                            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(uniforms), &uniforms);
                            r_gl_check_error("UI UBO SubData");
                            r_gl_check_error("glBufferSubData Rect UBO");
                            //glBindBuffer(GL_UNIFORM_BUFFER, 0); // REMOVED: Unbind UBO after update
                        }

                        // dan: upload instance data
                        U64 hardcoded_size = 0; // Size for hardcoded data
                        GLsizei instance_count = 0; // Instance count for draw call
                        U64 total_instance_bytes = batches->byte_count;
                        if (total_instance_bytes > 0 && batches->bytes_per_inst > 0) {
                             // <<< ADD LOGGING HERE (Log first instance of the first batch) >>>
                             if (batches->first && batches->first->v.byte_count >= sizeof(R_Rect2DInst) && batches->bytes_per_inst == sizeof(R_Rect2DInst)) {
                                 R_Rect2DInst *first_inst = (R_Rect2DInst *)batches->first->v.v;
                                 fprintf(stderr, "    First Instance Data:\n");
                                 fprintf(stderr, "      Dst: (%.1f, %.1f) -> (%.1f, %.1f)\n", first_inst->dst.x0, first_inst->dst.y0, first_inst->dst.x1, first_inst->dst.y1);
                                 fprintf(stderr, "      Src: (%.1f, %.1f) -> (%.1f, %.1f)\n", first_inst->src.x0, first_inst->src.y0, first_inst->src.x1, first_inst->src.y1);
                                 // Use integer indices instead of Corner_ enum names and x,y,z,w members
                                 fprintf(stderr, "      Colors[BL]: (%.2f, %.2f, %.2f, %.2f)\n", first_inst->colors[0].x, first_inst->colors[0].y, first_inst->colors[0].z, first_inst->colors[0].w);
                                 fprintf(stderr, "      Colors[TL]: (%.2f, %.2f, %.2f, %.2f)\n", first_inst->colors[1].x, first_inst->colors[1].y, first_inst->colors[1].z, first_inst->colors[1].w);
                                 fprintf(stderr, "      Colors[BR]: (%.2f, %.2f, %.2f, %.2f)\n", first_inst->colors[2].x, first_inst->colors[2].y, first_inst->colors[2].z, first_inst->colors[2].w);
                                 fprintf(stderr, "      Colors[TR]: (%.2f, %.2f, %.2f, %.2f)\n", first_inst->colors[3].x, first_inst->colors[3].y, first_inst->colors[3].z, first_inst->colors[3].w);
                                 fprintf(stderr, "      Radii[BL,TL,BR,TR]: %.1f, %.1f, %.1f, %.1f\n", first_inst->corner_radii[0], first_inst->corner_radii[1], first_inst->corner_radii[2], first_inst->corner_radii[3]);
                                 fprintf(stderr, "      Border: %.1f, Softness: %.1f, WhiteOverride: %.1f\n", first_inst->border_thickness, first_inst->edge_softness, first_inst->white_texture_override);
                             }

                            glBindBuffer(GL_ARRAY_BUFFER, r_gl_state->instance_vbo);
                            r_gl_check_error("glBindBuffer Instance VBO");

                            GLint current_vbo_size = 0;
                            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &current_vbo_size);
                            r_gl_check_error("glGetBufferParameteriv Instance VBO Size");

                            if (total_instance_bytes > (U64)current_vbo_size) {
                                glBufferData(GL_ARRAY_BUFFER, total_instance_bytes, NULL, GL_STREAM_DRAW);
                                r_gl_check_error("Instance VBO Resize (glBufferData)");
                            }
                            U64 current_offset = 0;
                            for(R_BatchNode *batch_n = batches->first; batch_n != 0; batch_n = batch_n->next)
                            {
                                U64 copy_size = batch_n->v.byte_count;
                                if (copy_size > 0) {
                                    Assert(current_offset + copy_size <= total_instance_bytes);
                                    glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)current_offset, (GLsizeiptr)copy_size, batch_n->v.v);
                                    r_gl_check_error("Instance VBO Upload (glBufferSubData)");
                                    current_offset += copy_size;
                                }
                            }
                            Assert(current_offset == total_instance_bytes);
                            instance_count = total_instance_bytes / batches->bytes_per_inst;
                        } else {
                            instance_count = 0; // No data or invalid size info
                        }

                        // dan: setup scissor rect
                        {
                            Rng2F32 clip_rect_f = group_params->clip;
                            Rng2S32 clip_rect_i = {0};
                            // Check for zero rect which means no clipping
                            if(clip_rect_f.x0 == 0 && clip_rect_f.y0 == 0 && clip_rect_f.x1 == 0 && clip_rect_f.y1 == 0)
                            {
                                clip_rect_i = r2s32p(0, 0, resolution.x, resolution.y);
                            }
                            else
                            {
                                // Clamp and round clip rect provided in top-left coordinates
                                clip_rect_i = r2s32p(round_f32_s32(clip_rect_f.x0), round_f32_s32(clip_rect_f.y0), round_f32_s32(clip_rect_f.x1), round_f32_s32(clip_rect_f.y1));
                                clip_rect_i.x0 = Clamp(0, clip_rect_i.x0, resolution.x);
                                clip_rect_i.y0 = Clamp(0, clip_rect_i.y0, resolution.y);
                                clip_rect_i.x1 = Clamp(0, clip_rect_i.x1, resolution.x);
                                clip_rect_i.y1 = Clamp(0, clip_rect_i.y1, resolution.y);
                            }
                            // GL scissor origin is bottom-left. Convert Y.
                            // y_gl = resolution.y - y_topleft
                            // height_gl = y_topleft_bottom - y_topleft_top
                            GLint y_gl = resolution.y - clip_rect_i.y1;
                            GLsizei height_gl = Max(0, clip_rect_i.y1 - clip_rect_i.y0);
                            GLsizei width_gl = Max(0, clip_rect_i.x1 - clip_rect_i.x0);
                            glScissor(clip_rect_i.x0, y_gl, width_gl, height_gl);
                            r_gl_check_error("glScissor UI");
                        }

                        // dan: bind target FBO (UI draws to stage FBO)
                        glBindFramebuffer(GL_FRAMEBUFFER, gl_window->stage_fbo);
                        GLenum err = glGetError();
                        if (err != GL_NO_ERROR) {
                            fprintf(stderr, "Error binding stage FBO for UI drawing: 0x%x\n", err);
                        }
                        
                        // Verify the FBO is actually bound
                        GLint current_fbo = 0;
                        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_fbo);
                        if (current_fbo != gl_window->stage_fbo) {
                            fprintf(stderr, "ERROR: Wrong FBO bound! Expected %u, got %d\n", 
                                   gl_window->stage_fbo, current_fbo);
                        }
                        
r_gl_check_error("glBindFramebuffer UI Stage");

                        // dan: draw
                        if (instance_count > 0) { // Use the calculated instance_count
                           // Check scissor settings before draw
                           GLint scissor[4];
                           glGetIntegerv(GL_SCISSOR_BOX, scissor);
                           // <<< ADD LOGGING HERE >>>
                           fprintf(stderr, "    Draw Call: %d instances, Scissor(%d, %d, %d, %d)\n",
                                  instance_count, scissor[0], scissor[1], scissor[2], scissor[3]);

                           // Extra verification of viewport and other states
                           GLint viewport[4];
                           glGetIntegerv(GL_VIEWPORT, viewport);
                          //  fprintf(stderr, "UI Draw: Viewport(%d, %d, %d, %d)\n", 
                          //         viewport[0], viewport[1], viewport[2], viewport[3]);
                            
                           // Verify transform feedback isn't active (can prevent drawing)
                           glDisable(GL_RASTERIZER_DISCARD);
                           
                           // Ensure blend mode is set correctly
                           glEnable(GL_BLEND);
                           glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

                          r_gl_check_error("UI Blend Func");
                           
                           // Draw the instances
                           glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instance_count); // Use calculated instance_count
                           r_gl_check_error("glDrawArraysInstanced UI");
                        }
                        
                        // Reset state to avoid affecting subsequent batches
                        glDisable(GL_SCISSOR_TEST);
                        glEnable(GL_SCISSOR_TEST);
                        r_gl_check_error("UI Draw State Reset");
                    } // end batch group loop

                    // Reset texture/sampler bindings for unit 0
                    glActiveTexture(GL_TEXTURE0);
r_gl_check_error("UI Active Texture");
  glBindTexture(GL_TEXTURE_2D, 0);
                    glBindSampler(0, 0);
                    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind instance VBO
                    glBindVertexArray(0); // Unbind VAO
                    glUseProgram(0); // Unbind shader program

                }break;

                ////////////////////////
                //- dan: blur rendering pass
                //
                case R_PassKind_Blur:
                {
                    R_PassParams_Blur *params = pass->params_blur;

                    //- dan: skip if zero size
                    if(params->blur_size <= 0.001f)

                    //- dan: common blur state
                    glBindVertexArray(r_gl_state->fullscreen_vao); // VAO for fullscreen quad (no attributes needed)
                    glUseProgram(r_gl_state->blur_shader.program_id);
                    glBindSampler(0, r_gl_state->samplers[R_Tex2DSampleKind_Linear]); // Use linear sampling for blur
                    glDisable(GL_BLEND); // Blur passes usually overwrite destination
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
r_gl_check_error("UI Depth Test Disable");
                    glDisable(GL_SCISSOR_TEST); // Blur affects whole texture (unless masked later)
                    glDisable(GL_CULL_FACE);
                    glViewport(0, 0, resolution.x, resolution.y); // Full texture viewport
                    r_gl_check_error("Blur Pass Setup");

                    // dan: set up uniforms (calculate kernel, fill struct)
                    R_GL_Uniforms_Blur uniforms = {0};
                    {
                       // Calculate Gaussian weights
                       F32 weights[ ArrayCount(uniforms.kernel)*2 + 1 ] = {0}; // Space for center + pairs
                       U64 kernel_radius = Min((U64)ArrayCount(uniforms.kernel), (U64)ceilf((params->blur_size - 1.0f) / 2.0f));
                       U64 kernel_size = kernel_radius * 2 + 1;
                       F32 sigma = Max(0.01f, (F32)kernel_radius / 2.0f); // Sigma heuristic

                       F32 weight_sum = 0.0f;
                       for (S64 i = -(S64)kernel_radius; i <= (S64)kernel_radius; ++i) {
                           F32 w = expf(-((F32)i * (F32)i) / (2.0f * sigma * sigma));
                           weights[i + kernel_radius] = w;
                           weight_sum += w;
                       }
                       // Normalize weights
                       if (weight_sum > 0.0001f) {
                           for (U64 i = 0; i < kernel_size; ++i) {
                               weights[i] /= weight_sum;
                           }
                       } else { // Handle zero sum case (e.g., blur_size 1 -> radius 0)
                           MemoryZeroArray(weights);
                           weights[0] = 1.0f;
                           kernel_size = 1;
                           kernel_radius = 0;
                       }

                       // Prepare kernel for bilinear optimization (combine pairs of weights)
                       uniforms.kernel[0].x = weights[kernel_radius]; // Center tap weight
                       uniforms.kernel[0].y = 0.0f;                   // Center tap offset
                       uniforms.blur_count = 1;                       // Start with center tap
                       for (U64 i = 1; i <= kernel_radius; i += 2) {
                           if (uniforms.blur_count >= ArrayCount(uniforms.kernel)) break; // Check bounds
                           F32 w0 = weights[kernel_radius + i];
                           F32 w1 = (i + 1 <= kernel_radius) ? weights[kernel_radius + i + 1] : 0.0f;
                           F32 w_sum = w0 + w1;
                           if (w_sum > 1e-6f) {
                               F32 offset = (F32)i + (w1 / w_sum); // Weighted offset from center
                               uniforms.kernel[uniforms.blur_count].x = w_sum;
                               uniforms.kernel[uniforms.blur_count].y = offset;
                               uniforms.blur_count++;
                           }
                       }

                       // Common blur parameters
                       uniforms.rect = v4f32(params->rect.x0, params->rect.y0, params->rect.x1, params->rect.y1);
                       uniforms.viewport_size = v2f32(resolution.x, resolution.y);
                       MemoryCopy(&uniforms.corner_radii_px, params->corner_radii, sizeof(params->corner_radii));
                    }

                    // Bind UBO once
                    glBindBufferBase(GL_UNIFORM_BUFFER, R_GL_UNIFORM_BINDING_POINT_BLUR, r_gl_state->uniform_type_kind_buffers[R_GL_UniformTypeKind_Blur]);
                    r_gl_check_error("Blur UBO Bind");

                    glActiveTexture(GL_TEXTURE0);
r_gl_check_error("UI Active Texture");
                    if (r_gl_state->blur_shader.main_texture_uniform_location != -1) {
                        glUniform1i(r_gl_state->blur_shader.main_texture_uniform_location, 0); // Use texture unit 0
                    }

                    // Pass 1: Horizontal Blur (Stage Color -> Scratch Color)
                    {
                        glBindFramebuffer(GL_FRAMEBUFFER, gl_window->stage_scratch_fbo);
                        r_gl_check_error("glBindFramebuffer Blur Scratch");
                        uniforms.direction = v2f32(1.0f, 0.0f); // Direction (texel size applied in shader)
                        glBindBuffer(GL_UNIFORM_BUFFER, r_gl_state->uniform_type_kind_buffers[R_GL_UniformTypeKind_Blur]); // Bind for update
                        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(uniforms), &uniforms); // Update UBO
r_gl_check_error("UI UBO SubData");
                        glBindBuffer(GL_UNIFORM_BUFFER, 0); // Unbind after update
                        r_gl_check_error("glBufferSubData Blur H");
                        glBindTexture(GL_TEXTURE_2D, gl_window->stage_color_texture); // Input texture
                        r_gl_check_error("glBindTexture Blur H Input");
                        glBindVertexArray(r_gl_state->fullscreen_vao);
                        r_gl_check_error("glBindVertexArray fullscreen_vao Blur");
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Draw fullscreen quad
                        r_gl_check_error("glDrawArrays Blur H");
                        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
                        r_gl_check_error("glBindTexture Blur H Unbind");
                    }

                    // Pass 2: Vertical Blur (Scratch Color -> Stage Color)
                    {
                        glBindFramebuffer(GL_FRAMEBUFFER, gl_window->stage_fbo); // Target main stage FBO
                        r_gl_check_error("glBindFramebuffer Blur Stage");
                        uniforms.direction = v2f32(0.0f, 1.0f); // Direction
                        glBindBuffer(GL_UNIFORM_BUFFER, r_gl_state->uniform_type_kind_buffers[R_GL_UniformTypeKind_Blur]); // Explicitly bind before update
                        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(uniforms), &uniforms); // Update UBO
r_gl_check_error("UI UBO SubData");
                        glBindBuffer(GL_UNIFORM_BUFFER, 0); // Unbind after update
                        r_gl_check_error("glBufferSubData Blur V");
                        glBindTexture(GL_TEXTURE_2D, gl_window->stage_scratch_color_texture); // Input texture
                        r_gl_check_error("glBindTexture Blur V Input");
                        glBindVertexArray(r_gl_state->fullscreen_vao);
                        r_gl_check_error("glBindVertexArray fullscreen_vao Blur");
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Draw fullscreen quad
                        r_gl_check_error("glDrawArrays Blur V");
                        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
                        r_gl_check_error("glBindTexture Blur V Unbind");
                    }

                    // Cleanup blur state
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glBindSampler(0, 0);
                    glEnable(GL_BLEND); // Re-enable blend potentially needed by subsequent passes
                    glEnable(GL_SCISSOR_TEST); // Re-enable scissor

                    // Unbind the VAO after both blur passes are done
                    glBindVertexArray(0);
                    r_gl_check_error("glBindVertexArray 0 Blur End");
                }break;


                ////////////////////////
                //- dan: 3d geometry rendering pass
                //
                case R_PassKind_Geo3D:
                {
                    //- dan: unpack params
                    R_PassParams_Geo3D *params = pass->params_geo3d;
                    R_BatchGroup3DMap *mesh_group_map = &params->mesh_batches;

                    //- dan: common Geo3D state
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LESS);
                    glDepthMask(GL_TRUE);
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                    glFrontFace(GL_CW); // Standard convention
                    glEnable(GL_BLEND); // Blend fragments onto target
                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

r_gl_check_error("UI Blend Func");
                    glEnable(GL_SCISSOR_TEST); // Use viewport as scissor initially
                    glBindVertexArray(r_gl_state->mesh_vao); // VAO for mesh vertex & instance attributes
                    glUseProgram(r_gl_state->mesh_shader.program_id);
                    r_gl_check_error("Geo3D Pass Setup");

                    //- dan: bind & clear Geo3D FBO
                    glBindFramebuffer(GL_FRAMEBUFFER, gl_window->geo3d_fbo);
                    r_gl_check_error("glBindFramebuffer Geo3D");
                    Vec4F32 bg_color = {0.1f, 0.1f, 0.1f, 0.0f}; // Example clear color (transparent black or grey)
                    glClearBufferfv(GL_COLOR, 0, bg_color.v); // Clear color attachment 0
                    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0f, 0); // Clear depth to 1.0, stencil to 0
                    r_gl_check_error("Geo3D FBO Clear");


                    //- dan: set viewport & scissor from params
                    Rng2F32 viewport_rng_f = params->viewport; // Top-left coordinate system
                    Vec2S32 viewport_pos_tl = v2s32(round_f32_s32(viewport_rng_f.x0), round_f32_s32(viewport_rng_f.y0));
                    Vec2S32 viewport_dim = v2s32(Max(0, round_f32_s32(viewport_rng_f.x1 - viewport_rng_f.x0)),
                                                 Max(0, round_f32_s32(viewport_rng_f.y1 - viewport_rng_f.y0)));
                    viewport_pos_tl.x = Clamp(0, viewport_pos_tl.x, resolution.x);
                    viewport_pos_tl.y = Clamp(0, viewport_pos_tl.y, resolution.y);
                    // Clamp dimensions based on position
                    viewport_dim.x = Clamp(0, viewport_dim.x, resolution.x - viewport_pos_tl.x);
                    viewport_dim.y = Clamp(0, viewport_dim.y, resolution.y - viewport_pos_tl.y);

                    // GL viewport/scissor origin is bottom-left. Convert Y.
                    GLint y_gl = resolution.y - (viewport_pos_tl.y + viewport_dim.y);
                    glViewport(viewport_pos_tl.x, y_gl, viewport_dim.x, viewport_dim.y);
                    glScissor(viewport_pos_tl.x, y_gl, viewport_dim.x, viewport_dim.y);
                    r_gl_check_error("Geo3D Viewport/Scissor");


                    // dan: setup uniforms (view/projection)
                    {
                        R_GL_Uniforms_Mesh uniforms = {0};
                        // Combine and transpose projection * view matrix
                        // Flip Y in projection matrix for OpenGL coordinate system
                        Mat4x4F32 projection_gl = params->projection;
                        projection_gl.v[1][1] *= -1.0f;
                        Mat4x4F32 view_proj_c = mul_4x4f32(projection_gl, params->view);
                        uniforms.view_proj_matrix = transpose_4x4f32(view_proj_c); // Transpose for GLSL column-major

                        glBindBufferBase(GL_UNIFORM_BUFFER, R_GL_UNIFORM_BINDING_POINT_MESH, r_gl_state->uniform_type_kind_buffers[R_GL_UniformTypeKind_Mesh]);
                        r_gl_check_error("glBindBufferBase Mesh UBO");
                        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(uniforms), &uniforms);
r_gl_check_error("UI UBO SubData");
                        r_gl_check_error("glBufferSubData Mesh UBO");
                    }


                    // dan: draw mesh batches (instanced)
                    for(U64 slot_idx = 0; slot_idx < mesh_group_map->slots_count; slot_idx += 1)
                    {
                        for(R_BatchGroup3DMapNode *n = mesh_group_map->slots[slot_idx]; n != 0; n = n->next)
                        {
                            R_BatchList *batches = &n->batches;
                            R_BatchGroup3DParams *group_params = &n->params;


                            R_GL_Buffer *vtx_buffer = r_gl_buffer_from_handle(group_params->mesh_vertices);
                            R_GL_Buffer *idx_buffer = r_gl_buffer_from_handle(group_params->mesh_indices);

                            if (vtx_buffer == &r_gl_buffer_nil || idx_buffer == &r_gl_buffer_nil || vtx_buffer->buffer_id == 0 || idx_buffer->buffer_id == 0) {
                                log_info(str8_lit("Skipping Geo3D batch group due to invalid vertex or index buffer."));
                            }

                            // Bind vertex buffer and ensure VAO attributes are set correctly for it
                            // VAO should already be bound (glBindVertexArray(r_gl_state->mesh_vao))
                            glBindBuffer(GL_ARRAY_BUFFER, vtx_buffer->buffer_id);
                            r_gl_check_error("glBindBuffer Geo3D VBO");
                            // Re-specify vertex attrib pointers as VBO binding is part of VAO state in Core Profile *only if*
                            // GL_VERTEX_ARRAY_BINDING is used, which is not the case here. So we must reset them.
                            size_t mesh_vert_stride = 11 * sizeof(F32); // Pos(3), Norm(3), Tex(2), Col(3)
                            glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, mesh_vert_stride, (void*)(0 * sizeof(F32))); // Pos
                            glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, mesh_vert_stride, (void*)(3 * sizeof(F32))); // Norm
                            glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, mesh_vert_stride, (void*)(6 * sizeof(F32))); // Tex
                            glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, mesh_vert_stride, (void*)(8 * sizeof(F32))); // Col
                             // Ensure vertex attrib divisors are 0 for per-vertex data
                             glVertexAttribDivisor(0, 0);
                             glVertexAttribDivisor(1, 0);
                             glVertexAttribDivisor(2, 0);
                             glVertexAttribDivisor(3, 0);
                            r_gl_check_error("glVertexAttribPointer Geo3D Vertex");

                            // Bind index buffer
                            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx_buffer->buffer_id);
                            r_gl_check_error("glBindBuffer Geo3D IBO");

                            // Upload instance data (transforms) to the shared instance VBO
                            U64 total_instance_bytes = batches->byte_count; // Total bytes already available in batches
                            if (total_instance_bytes > 0) {
                                glBindBuffer(GL_ARRAY_BUFFER, r_gl_state->instance_vbo);
r_gl_check_error("UI Bind Instance VBO");
                                r_gl_check_error("glBindBuffer Geo3D Instance VBO");

                                // Check if VBO needs resize/reallocation
                                GLint current_vbo_size = 0;
                                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &current_vbo_size);
                                r_gl_check_error("glGetBufferParameteriv Instance VBO Size Geo3D");

                                B32 vbo_resized = 0;
                                if (total_instance_bytes > (U64)current_vbo_size) {
                                    glBufferData(GL_ARRAY_BUFFER, total_instance_bytes, NULL, GL_STREAM_DRAW); // Orphan & Resize
                                    r_gl_check_error("Instance VBO Resize (glBufferData) Geo3D");
                                    vbo_resized = 1;
                                }

                                // Map and copy instance data
                                void *instance_data_ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, total_instance_bytes, GL_MAP_WRITE_BIT | (vbo_resized ? GL_MAP_INVALIDATE_BUFFER_BIT : GL_MAP_INVALIDATE_RANGE_BIT) );
                                if (instance_data_ptr) {
                                    U64 offset = 0;
                                    for(R_BatchNode *batch_n = batches->first; batch_n != 0; batch_n = batch_n->next)
                                    {
                                         // Assuming batch_n->v.v points to R_Mesh3DInst array
                                         U64 copy_size = batch_n->v.byte_count;
                                         Assert(offset + copy_size <= total_instance_bytes);
                                         MemoryCopy((U8*)instance_data_ptr + offset, batch_n->v.v, copy_size);
                                         offset += copy_size;
                                    }
                                    Assert(offset == total_instance_bytes);
                                    glUnmapBuffer(GL_ARRAY_BUFFER);
                                    r_gl_check_error("Instance VBO Upload (glUnmapBuffer) Geo3D");
                                } else {
                                    r_gl_check_error("glMapBufferRange Failed Geo3D");
                                    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind on failure
                                    continue; // Skip draw if map failed
                                }

                                // Ensure instance attribute pointers are correctly set *after* binding the instance VBO
                                // These should point to the r_gl_state->instance_vbo
                                size_t mesh_inst_stride = sizeof(R_Mesh3DInst); // Assuming R_Mesh3DInst is the instance structure
                                size_t xform_offset = OffsetOf(R_Mesh3DInst, xform);
                                glEnableVertexAttribArray(4); glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, mesh_inst_stride, (void*)(xform_offset + 0*sizeof(Vec4F32))); glVertexAttribDivisor(4, 1);
                                glEnableVertexAttribArray(5); glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, mesh_inst_stride, (void*)(xform_offset + 1*sizeof(Vec4F32))); glVertexAttribDivisor(5, 1);
                                glEnableVertexAttribArray(6); glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, mesh_inst_stride, (void*)(xform_offset + 2*sizeof(Vec4F32))); glVertexAttribDivisor(6, 1);
                                glEnableVertexAttribArray(7); glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, mesh_inst_stride, (void*)(xform_offset + 3*sizeof(Vec4F32))); glVertexAttribDivisor(7, 1);
                                r_gl_check_error("glVertexAttribPointer Geo3D Instance");
                            } else {
                                // Disable instance attributes if not drawing instanced? No, draw with instance count 0?
                                // Or better, skip the draw call if total_inst_count is 0.
                            }

                            // Draw instanced
                            GLenum topology = r_gl_primitive_from_topology(group_params->mesh_geo_topology);
                            U64 index_count = idx_buffer->size / sizeof(U32); // Assuming U32 indices
                            if (index_count > 0 && batches->byte_count > 0 && batches->bytes_per_inst > 0) {
                               glDrawElementsInstanced(topology, index_count, GL_UNSIGNED_INT, (void*)0, batches->byte_count / batches->bytes_per_inst);
                               r_gl_check_error("glDrawElementsInstanced Geo3D");
                            }
                        }
                    } // End Geo3D group map loop

                    // Unbind element array buffer before compositing if needed
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the instance VBO now

                    //- dan: composite geo3d result into stage fbo
                    glBindFramebuffer(GL_FRAMEBUFFER, gl_window->stage_fbo); // Target main stage FBO
                    r_gl_check_error("glBindFramebuffer Geo3D Composite");
                    glUseProgram(r_gl_state->geo3d_composite_shader.program_id);
                    r_gl_check_error("glUseProgram Geo3D Composite");
                    glBindVertexArray(r_gl_state->fullscreen_vao); // Use fullscreen quad VAO
                    r_gl_check_error("glBindVertexArray Geo3D Composite");

                    // Reset viewport and scissor to cover the composite area (usually full window or target rect)
                    // Use the clip rect from the Geo3D params for compositing area? Or always full stage? Assume full stage.
                    glViewport(0, 0, resolution.x, resolution.y); // Full window viewport
                    glScissor(0, 0, resolution.x, resolution.y);
                    r_gl_check_error("glScissor");
                    glDisable(GL_SCISSOR_TEST); // Ensure scissor test is off for fullscreen blit
                    glDisable(GL_DEPTH_TEST);
                    glDisable(GL_CULL_FACE);
                    glEnable(GL_BLEND);
                    r_gl_check_error("UI Blend Func");
                    // Or potentially use GL_ONE, GL_ZERO if result is opaque and overwrites? Assume alpha blend.
                    // Bind the Geo3D color texture as input
                    glActiveTexture(GL_TEXTURE0);
r_gl_check_error("UI Active Texture");
                    glBindTexture(GL_TEXTURE_2D, gl_window->geo3d_color_texture);
                    r_gl_check_error("glBindTexture Geo3D Composite Input");
                    glBindSampler(0, r_gl_state->samplers[R_Tex2DSampleKind_Nearest]); // Nearest neighbor for composite
                    r_gl_check_error("glBindSampler Geo3D Composite");
                    if (r_gl_state->geo3d_composite_shader.main_texture_uniform_location != -1) {
                        glUniform1i(r_gl_state->geo3d_composite_shader.main_texture_uniform_location, 0); // Texture unit 0
                    }

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Draw fullscreen quad
                    r_gl_check_error("Geo3D Composite Draw");

                    // Cleanup composite state
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glBindSampler(0, 0);
                    glUseProgram(0); // Unbind shader
                    glBindVertexArray(0); // Unbind VAO
                }break;
            } // end switch pass->kind
        } // end pass loop

        //- dan: unbind resources / reset state after all passes
        glBindVertexArray(0);
        glUseProgram(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind default framebuffer
        glDisable(GL_SCISSOR_TEST); // Disable scissor if default state requires it

    } // end mutex scope
}

// Debug function to draw a simple test pattern
internal void 
r_gl_debug_draw_test_pattern(void)
{
    // Create a simple 2x2 checkerboard texture
    GLuint test_texture;
    glGenTextures(1, &test_texture);
    glBindTexture(GL_TEXTURE_2D, test_texture);
    
    // Create a black and white checkerboard
    unsigned char pattern[] = {
        255, 255, 255, 255,  0,   0,   0, 255,
          0,   0,   0, 255, 255, 255, 255, 255
    };
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pattern);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Create a simple fullscreen quad to display it
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Draw fullscreen quad
    glUseProgram(r_gl_state->finalize_shader.program_id);
    glActiveTexture(GL_TEXTURE0);
r_gl_check_error("UI Active Texture");
    glBindTexture(GL_TEXTURE_2D, test_texture);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    // Clean up
    glBindVertexArray(0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &test_texture);
    glDeleteVertexArrays(1, &vao);
}

#define r_gl_check_error(op) r_gl_check_error_line_file(op, __LINE__, __FILE__)

// Add this function to help debug context issues
internal void
debug_log_current_context(void)
{
  // IMPORTANT: Ensure a valid context exists before calling GL functions.
  if(!r_gl_state || !r_gl_state->has_valid_context)
  {
    fprintf(stderr, "Debug Log: No valid GL context detected by r_gl_state->has_valid_context.\n");
    return;
  }

  // Use the correct display handle stored in the state
  Display *display = r_gl_state->display;
  if (!display) {
      fprintf(stderr, "Debug Log: No X11 display found in r_gl_state.\n");
      return;
  }

  GLXContext current = glXGetCurrentContext();
  fprintf(stderr, "Debug Log: Current GLX context: %p (Display: %p, Window: %lu)\n",
          current, glXGetCurrentDisplay(), (unsigned long)glXGetCurrentDrawable());

  // Check OpenGL version (can help identify context switches)
  const char* version = (const char*)glGetString(GL_VERSION);
  fprintf(stderr, "Debug Log: Current OpenGL version: %s\n", version ? version : "NULL");

  // Check for any pending GL errors
  GLenum err;
  while((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "Debug Log: GL error AFTER previous operation: 0x%x\n", err);
  }
}

// Add to your texture allocation system
internal void
r_debug_texture_info(GLuint texture_id)
{
    if (!glIsTexture(texture_id))
    {
        fprintf(stderr, "Texture %u is not a valid texture object\n", texture_id);
        return;
    }

    GLint current_texture_binding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture_binding);

    GLint width = 0, height = 0, internal_format = 0;
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

    fprintf(stderr, "Texture %u: %dx%d, internal format: 0x%x\n",
           texture_id, width, height, internal_format);

    // Restore previous binding
    glBindTexture(GL_TEXTURE_2D, current_texture_binding);
}

