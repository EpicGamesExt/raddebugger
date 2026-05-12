// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: OS Portion Includes

#if OS_WINDOWS
# include "render/opengl/win32/render_opengl_win32.c"
#elif OS_LINUX
# include "render/opengl/linux/render_opengl_linux.c"
#else
# error OS portion of OpenGL rendering backend not defined.
#endif

////////////////////////////////
//~ rjf: Attribute Tables

global read_only R_OGL_Attribute r_ogl_rect_input_attributes[] =
{
  {0, str8_lit_comp("c2v_dst_rect"),        GL_FLOAT,  4},
  {1, str8_lit_comp("c2v_src_rect"),        GL_FLOAT,  4},
  {2, str8_lit_comp("c2v_colors_0"),        GL_FLOAT,  4},
  {3, str8_lit_comp("c2v_colors_1"),        GL_FLOAT,  4},
  {4, str8_lit_comp("c2v_colors_2"),        GL_FLOAT,  4},
  {5, str8_lit_comp("c2v_colors_3"),        GL_FLOAT,  4},
  {6, str8_lit_comp("c2v_corner_radii"),    GL_FLOAT,  4},
  {7, str8_lit_comp("c2v_style"),           GL_FLOAT,  4},
};

global read_only R_OGL_Attribute r_ogl_single_color_output_attributes[] =
{
  {0, str8_lit_comp("final_color")},
};

////////////////////////////////
//~ rjf: Generated Code

#include "render/opengl/generated/render_opengl.meta.c"

////////////////////////////////
//~ rjf: Helpers

internal R_Handle
r_ogl_handle_from_tex2d(R_OGL_Tex2D *t)
{
  R_Handle h = {(U64)t};
  return h;
}

internal R_OGL_Tex2D *
r_ogl_tex2d_from_handle(R_Handle h)
{
  R_OGL_Tex2D *t = (R_OGL_Tex2D *)h.u64[0];
  return t;
}

internal R_OGL_FormatInfo
r_ogl_format_info_from_tex2dformat(R_Tex2DFormat fmt)
{
  R_OGL_FormatInfo result;
  result.internal_format = GL_RGBA;
  result.format = GL_RGBA;
  result.base_type = GL_UNSIGNED_BYTE;
  // TODO(rjf)
  return result;
}

internal GLuint
r_ogl_instance_buffer_from_size(U64 size)
{
  GLuint buffer = r_ogl_state->scratch_buffer_64kb;
  if(size > KB(64))
  {
    // rjf: build buffer
    U64 flushed_buffer_size = size;
    flushed_buffer_size += MB(1)-1;
    flushed_buffer_size -= flushed_buffer_size%MB(1);
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, flushed_buffer_size, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // rjf: push buffer to flush list
    R_OGL_FlushBuffer *n = push_array(r_ogl_state->buffer_flush_arena, R_OGL_FlushBuffer, 1);
    n->id = buffer;
    SLLQueuePush(r_ogl_state->first_buffer_to_flush, r_ogl_state->last_buffer_to_flush, n);
  }
  return buffer;
}

internal void
r_ogl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
  raddbg_log("[OpenGL] %.*s\n", (int)length, message);
  fprintf(stderr, "[OpenGL] %.*s\n", (int)length, message);
}

internal B32
r_ogl_scissor(Rng2F32 clip, Vec2F32 viewport_dim)
{
  S32 x0 = Clamp(0, (S32)floor_f32(clip.x0), (S32)viewport_dim.x);
  S32 y0 = Clamp(0, (S32)floor_f32(clip.y0), (S32)viewport_dim.y);
  S32 x1 = Clamp(0, (S32)ceil_f32(clip.x1), (S32)viewport_dim.x);
  S32 y1 = Clamp(0, (S32)ceil_f32(clip.y1), (S32)viewport_dim.y);
  S32 width = x1 - x0;
  S32 height = y1 - y0;
  if(width > 0 && height > 0)
  {
    glScissor(x0, (S32)viewport_dim.y - y1, width, height);
  }
  return width > 0 && height > 0;
}

////////////////////////////////
//~ rjf: Backend Hooks

//- rjf: top-level layer initialization

r_hook void
r_init(CmdLine *cmdln)
{
  //- rjf: do os-specific portion of work
  r_ogl_os_init(cmdln);
  
  //- rjf: top-level initialization
  Arena *arena = arena_alloc();
  r_ogl_state = push_array(arena, R_OGL_State, 1);
  r_ogl_state->arena = arena;
  
  //- rjf: load gl procedures
#define X(name, r, p) name = (name##_FunctionType *)r_ogl_os_load_procedure(#name);
  R_OGL_ProcedureXList
#undef X
  
  //- rjf: build all shaders
  for EachEnumVal(R_OGL_ShaderKind, k)
  {
    // rjf: compile
    struct {GLenum type; String8 *src; GLuint out; String8 errors;} stages[] =
    {
      {GL_VERTEX_SHADER,   r_ogl_shader_kind_vshad_src_table[k]},
      {GL_FRAGMENT_SHADER, r_ogl_shader_kind_pshad_src_table[k]},
    };
    for EachElement(idx, stages)
    {
      stages[idx].out = glCreateShader(stages[idx].type);
      GLint src_size = stages[idx].src->size;
      glShaderSource(stages[idx].out, 1, (char **)&stages[idx].src->str, &src_size);
      glCompileShader(stages[idx].out);
      GLint info_log_length = 0;
      GLint status = 0;
      glGetShaderiv(stages[idx].out, GL_COMPILE_STATUS, &status);
      glGetShaderiv(stages[idx].out, GL_INFO_LOG_LENGTH, &info_log_length);
      if(info_log_length != 0)
      {
        stages[idx].errors.str = push_array(r_ogl_state->arena, U8, info_log_length+1);
        stages[idx].errors.size = info_log_length;
        glGetShaderInfoLog(stages[idx].out, info_log_length, 0, (char *)stages[idx].errors.str);
      }
      raddbg_pin(text(stages[idx].errors.str));
    }
    
    // rjf: attach compilations to program
    GLuint program = glCreateProgram();
    for EachElement(idx, stages)
    {
      glAttachShader(program, stages[idx].out);
    }
    
    // rjf: bind inputs
    R_OGL_AttributeArray inputs = r_ogl_shader_kind_input_attributes_table[k];
    for EachIndex(idx, inputs.count)
    {
      glBindAttribLocation(program, inputs.v[idx].index, (char *)inputs.v[idx].name.str);
    }
    
    // rjf: bind outputs
    R_OGL_AttributeArray outputs = r_ogl_shader_kind_output_attributes_table[k];
    for EachIndex(idx, outputs.count)
    {
      glBindFragDataLocation(program, outputs.v[idx].index, (char *)outputs.v[idx].name.str);
    }
    
    // rjf: link / validate / store
    glLinkProgram(program);
    glValidateProgram(program);
    r_ogl_state->shaders[k] = program;
  }
  
  //- rjf: set up built-in resources
  {
    // rjf: all-purpose VAO
    glGenVertexArrays(1, &r_ogl_state->all_purpose_vao);
    
    // rjf: scratch buffer
    {
      glGenBuffers(1, &r_ogl_state->scratch_buffer_64kb);
      glBindBuffer(GL_ARRAY_BUFFER, r_ogl_state->scratch_buffer_64kb);
      glBufferData(GL_ARRAY_BUFFER, KB(64), 0, GL_DYNAMIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    // rjf: white texture
    {
      glGenTextures(1, &r_ogl_state->white_texture);
      glBindTexture(GL_TEXTURE_2D, r_ogl_state->white_texture);
      U32 white_pixel = 0xffffffff;
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white_pixel);
    }
  }
  
  //- rjf: set up options
  glEnable(GL_FRAMEBUFFER_SRGB);
  
  //- rjf: set up buffer flush state
  r_ogl_state->buffer_flush_arena = arena_alloc();
  
  //- rjf: set up debug callback
  B32 debug_mode = cmd_line_has_flag(cmdln, str8_lit("opengl_debug"));
#if BUILD_DEBUG
  debug_mode = 1;
#endif
  if(debug_mode)
  {
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(r_ogl_debug_message_callback, 0);
  }
}

//- rjf: window setup/teardown

r_hook R_Handle
r_window_equip(WM_Window window)
{
  R_OGL_Window *w = r_ogl_state->free_window;
  if(w != 0)
  {
    SLLStackPop(r_ogl_state->free_window);
  }
  else
  {
    w = push_array(r_ogl_state->arena, R_OGL_Window, 1);
  }
  MemoryZeroStruct(w);
  w->os = r_ogl_os_window_equip(window);
  R_Handle result = {(U64)w};
  return result;
}

r_hook void
r_window_unequip(WM_Window window, R_Handle window_equip)
{
  R_OGL_Window *w = (R_OGL_Window *)window_equip.u64[0];
  r_ogl_os_window_unequip(window, w->os);
  SLLStackPush(r_ogl_state->free_window, w);
}

//- rjf: textures

r_hook R_Handle
r_tex2d_alloc(R_ResourceKind kind, Vec2S32 size, R_Tex2DFormat format, void *data)
{
  //- rjf: allocate texture record
  R_OGL_Tex2D *tex2d = r_ogl_state->free_tex2d;
  if(tex2d)
  {
    SLLStackPop(r_ogl_state->free_tex2d);
  }
  else
  {
    tex2d = push_array(r_ogl_state->arena, R_OGL_Tex2D, 1);
  }
  
  //- rjf: map kind/format -> gl counterparts
  R_OGL_FormatInfo gl_fmt_info = r_ogl_format_info_from_tex2dformat(format);
  
  //- rjf: allocate GL texture
  {
    glGenTextures(1, &tex2d->id);
    glBindTexture(GL_TEXTURE_2D, tex2d->id);
    glTexImage2D(GL_TEXTURE_2D, 0, gl_fmt_info.internal_format, size.x, size.y, 0, gl_fmt_info.format, gl_fmt_info.base_type, data);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  
  //- rjf: fill
  tex2d->resource_kind = kind;
  tex2d->fmt = format;
  tex2d->size = size;
  
  //- rjf: bundle & return
  R_Handle result = r_ogl_handle_from_tex2d(tex2d);
  return result;
}

r_hook void
r_tex2d_release(R_Handle texture)
{
  R_OGL_Tex2D *t = r_ogl_tex2d_from_handle(texture);
  if(t != 0)
  {
    glDeleteTextures(1, &t->id);
    SLLStackPush(r_ogl_state->free_tex2d, t);
  }
}

r_hook R_ResourceKind
r_kind_from_tex2d(R_Handle texture)
{
  R_ResourceKind result = R_ResourceKind_Static;
  R_OGL_Tex2D *t = r_ogl_tex2d_from_handle(texture);
  if(t)
  {
    result = t->resource_kind;
  }
  return result;
}

r_hook Vec2S32
r_size_from_tex2d(R_Handle texture)
{
  Vec2S32 result = {0, 0};
  R_OGL_Tex2D *t = r_ogl_tex2d_from_handle(texture);
  if(t)
  {
    result = t->size;
  }
  return result;
}

r_hook R_Tex2DFormat
r_format_from_tex2d(R_Handle texture)
{
  R_Tex2DFormat result = R_Tex2DFormat_RGBA8;
  R_OGL_Tex2D *t = r_ogl_tex2d_from_handle(texture);
  if(t)
  {
    result = t->fmt;
  }
  return result;
}

r_hook void
r_fill_tex2d_region(R_Handle texture, Rng2S32 subrect, void *data)
{
  R_OGL_Tex2D *t = r_ogl_tex2d_from_handle(texture);
  if(t)
  {
    R_OGL_FormatInfo fmt_info = r_ogl_format_info_from_tex2dformat(t->fmt);
    glBindTexture(GL_TEXTURE_2D, t->id);
    Vec2S32 rect_size = dim_2s32(subrect);
    glTexSubImage2D(GL_TEXTURE_2D, 0, subrect.x0, subrect.y0, rect_size.x, rect_size.y, fmt_info.format, fmt_info.base_type, data);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
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
  // TODO(rjf)
}

//- rjf: frame markers

r_hook void
r_begin_frame(void)
{
  // TODO(rjf)
}

r_hook void
r_end_frame(void)
{
  for(R_OGL_FlushBuffer *flush_buffer = r_ogl_state->first_buffer_to_flush; flush_buffer != 0; flush_buffer = flush_buffer->next)
  {
    glDeleteBuffers(1, &flush_buffer->id);
  }
  arena_clear(r_ogl_state->buffer_flush_arena);
  r_ogl_state->first_buffer_to_flush = r_ogl_state->last_buffer_to_flush = 0;
}

r_hook void
r_window_begin_frame(WM_Window os, R_Handle r)
{
  r_ogl_os_select_window(os, r);
  R_OGL_Window *window = (R_OGL_Window *)r.u64[0];
  
  //- rjf: unpack window viewport info
  Rng2F32 client_rect = wm_client_rect_from_window(os);
  Vec2F32 client_rect_dim = dim_2f32(client_rect);
  
  //- rjf: set up targets if needed
  if(client_rect_dim.x != window->last_client_rect_dim.x || client_rect_dim.y != window->last_client_rect_dim.y)
  {
    window->last_client_rect_dim = client_rect_dim;
    R_OGL_RenderTarget *targets[] =
    {
      &window->stage_target,
      &window->stage_scratch_target,
    };
    for EachElement(idx, targets)
    {
      if(targets[idx]->fbo != 0)
      {
        glDeleteFramebuffers(1, &targets[idx]->fbo);
        glDeleteTextures(1, &targets[idx]->color_texture);
      }
      glGenFramebuffers(1, &targets[idx]->fbo);
      glGenTextures(1, &targets[idx]->color_texture);
      glBindFramebufferScope(GL_FRAMEBUFFER, targets[idx]->fbo)
        glBindTextureScope(GL_TEXTURE_2D, targets[idx]->color_texture)
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (S32)client_rect_dim.x, (S32)client_rect_dim.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targets[idx]->color_texture, 0);  
      }
    }
  }
  
  //- rjf: clear and reset state
  {
    GLuint fbos[] =
    {
      window->stage_target.fbo,
      window->stage_scratch_target.fbo,
      0,
    };
    for EachElement(idx, fbos)
    {
      glBindFramebufferScope(GL_FRAMEBUFFER, fbos[idx])
      {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      }
    }
  }
  glViewport(0, 0, (S32)client_rect_dim.x, (S32)client_rect_dim.y);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

r_hook void
r_window_end_frame(WM_Window os, R_Handle r)
{
  R_OGL_Window *window = (R_OGL_Window *)r.u64[0];
  
  //- rjf: blit window's main staging frame buffer to window frame buffer
  glBindFramebufferScope(GL_READ_FRAMEBUFFER, window->stage_target.fbo) glBindFramebufferScope(GL_DRAW_FRAMEBUFFER, 0)
  {
    glBlitFramebuffer(0, 0, (S32)window->last_client_rect_dim.x, (S32)window->last_client_rect_dim.y,
                      0, 0, (S32)window->last_client_rect_dim.x, (S32)window->last_client_rect_dim.y,
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
  }
  
  //- rjf: swap buffers
  r_ogl_os_window_swap(os, r);
}

//- rjf: render pass submission

r_hook void
r_window_submit(WM_Window window, R_Handle window_equip, R_PassList *passes)
{
  R_OGL_Window *w = (R_OGL_Window *)window_equip.u64[0];
  Rng2F32 viewport_rect = wm_client_rect_from_window(window);
  Vec2F32 viewport_dim = dim_2f32(viewport_rect);
  for(R_PassNode *pass_n = passes->first; pass_n != 0; pass_n = pass_n->next)
  {
    R_Pass *pass = &pass_n->v;
    switch(pass->kind)
    {
      default:{}break;
      
      ////////////////////////
      //- rjf: ui rendering pass
      //
      case R_PassKind_UI:
      {
        //- rjf: unpack params
        R_PassParams_UI *params = pass->params_ui;
        R_BatchGroup2DList *rect_batch_groups = &params->rects;
        
        //- rjf: draw each batch group
        GLuint shader = r_ogl_state->shaders[R_OGL_ShaderKind_Rect];
        glUseProgramScope(shader)
          glBindVertexArrayScope(r_ogl_state->all_purpose_vao)
          glBindFramebufferScope(GL_FRAMEBUFFER, w->stage_target.fbo)
        {
          for(R_BatchGroup2DNode *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
          {
            R_BatchList *batches = &group_n->batches;
            R_BatchGroup2DParams *group_params = &group_n->params;
            
            //- rjf: unpack texture
            R_Tex2DFormat texture_fmt = R_Tex2DFormat_RGBA8;
            GLuint texture_id = r_ogl_state->white_texture;
            {
              R_OGL_Tex2D *tex = r_ogl_tex2d_from_handle(group_params->tex);
              if(tex != 0)
              {
                texture_id = tex->id;
                texture_fmt = tex->fmt;
              }
            }
            
            //- rjf: get & fill buffer
            GLuint buffer = r_ogl_instance_buffer_from_size(batches->byte_count);
            {
              glBindBuffer(GL_ARRAY_BUFFER, buffer);
              U64 off = 0;
              for(R_BatchNode *batch_n = batches->first; batch_n != 0; batch_n = batch_n->next)
              {
                glBufferSubData(GL_ARRAY_BUFFER, off, batch_n->v.byte_count, batch_n->v.v);
                off += batch_n->v.byte_count;
              }
            }
            
            //- rjf: bind input attributes
            {
              R_OGL_AttributeArray inputs = r_ogl_shader_kind_input_attributes_table[R_OGL_ShaderKind_Rect];
              U64 off = 0;
              for EachIndex(idx, inputs.count)
              {
                glEnableVertexAttribArray(inputs.v[idx].index);
                glVertexAttribDivisor(inputs.v[idx].index, 1);
                glVertexAttribPointer(inputs.v[idx].index, inputs.v[idx].count, inputs.v[idx].type, GL_FALSE, sizeof(R_Rect2DInst), (void *)(off));
                // TODO(rjf): this is not correct if type != GL_FLOAT
                off += inputs.v[idx].count*sizeof(F32);
              }
            }
            
            //- rjf: bind texture
            {
              glActiveTexture(GL_TEXTURE0);
              glBindTexture(GL_TEXTURE_2D, texture_id);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
              switch(group_params->tex_sample_kind)
              {
                default:
                case R_Tex2DSampleKind_Nearest:
                {
                  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                }break;
                case R_Tex2DSampleKind_Linear:
                {
                  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                }break;
              }
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
              glUniform1i(glGetUniformLocation(shader, "u_tex_color"), 0);
            }
            
            //- rjf: upload misc. uniforms
            {
              Mat4x4F32 texture_sample_channel_map = r_sample_channel_map_from_tex2dformat(texture_fmt);
              glUniformMatrix4fv(glGetUniformLocation(shader, "u_texture_sample_channel_map"), 1, 0, &texture_sample_channel_map.v[0][0]);
              glUniform2f(glGetUniformLocation(shader, "u_viewport_size_px"), viewport_dim.x, viewport_dim.y);
              glUniform1f(glGetUniformLocation(shader, "u_opacity"), 1.f - group_params->transparency);
              glUniformMatrix3fv(glGetUniformLocation(shader, "u_xform"), 1, 0, &group_params->xform.v[0][0]);
            }
            
            //- rjf: set up scissor
            if(group_params->clip.x0 != 0 ||
               group_params->clip.x1 != 0 ||
               group_params->clip.y0 != 0 ||
               group_params->clip.y1 != 0)
            {
              if(r_ogl_scissor(group_params->clip, viewport_dim))
              {
                glEnable(GL_SCISSOR_TEST);
              }
            }
            
            //- rjf: draw
            {
              glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, batches->byte_count / batches->bytes_per_inst);
            }
            
            //- rjf: unset scissor
            glDisable(GL_SCISSOR_TEST);
          }
        }
      }break;
      
      ////////////////////////
      //- rjf: blur rendering pass
      //
      case R_PassKind_Blur:
      {
        R_PassParams_Blur *params = pass->params_blur;
        GLuint shader = r_ogl_state->shaders[R_OGL_ShaderKind_Blur];
        glUseProgramScope(shader)
          glBindVertexArrayScope(r_ogl_state->all_purpose_vao)
        {
          //- rjf: form blur sampling kernel
          Vec4F32 kernel[32] = {0};
          U64 blur_count = 0;
          {
            //
            // TODO(rjf): adopted from martins' d3d11 impl, should dedup in common render helpers
            //
            F32 weights[ArrayCount(kernel)*2] = {0};
            F32 blur_size = Min(params->blur_size, ArrayCount(weights));
            blur_count = (U64)round_f32(blur_size);
            F32 stdev = (blur_size-1.f)/2.f;
            F32 one_over_root_2pi_stdev2 = 1/sqrt_f32(2*pi32*stdev*stdev);
            F32 euler32 = 2.718281828459045f;
            weights[0] = 1.f;
            if(stdev > 0.f)
            {
              for(U64 idx = 0; idx < blur_count; idx += 1)
              {
                F32 kernel_x = (F32)idx;
                weights[idx] = one_over_root_2pi_stdev2*pow_f32(euler32, -kernel_x*kernel_x/(2.f*stdev*stdev)); 
              }
            }
            if(weights[0] > 1.f)
            {
              MemoryZeroArray(weights);
              weights[0] = 1.f;
            }
            else
            {
              // prepare weights & offsets for bilinear lookup
              // blur filter wants to calculate w0*pixel[pos] + w1*pixel[pos+1] + ...
              // with bilinear filter we can do this calulation by doing only w*sample(pos+t) = w*((1-t)*pixel[pos] + t*pixel[pos+1])
              // we can see w0=w*(1-t) and w1=w*t
              // thus w=w0+w1 and t=w1/w
              for(U64 idx = 1; idx < blur_count; idx += 2)
              {
                F32 w0 = weights[idx + 0];
                F32 w1 = weights[idx + 1];
                F32 w = w0 + w1;
                F32 t = w1 / w;
                
                // each kernel element is float2(weight, offset)
                // weights & offsets are adjusted for bilinear sampling
                // zw elements are not used, a bit of waste but it allows for simpler shader code
                kernel[(idx+1)/2] = v4f32(w, (F32)idx + t, 0, 0);
              }
            }
            kernel[0].x = weights[0];
          }
          
          //- rjf: set up uniforms
          glUniform4fv(glGetUniformLocation(shader, "rect"), 1, (F32 *)(&params->rect));
          glUniform4fv(glGetUniformLocation(shader, "corner_radii_px"), 1, params->corner_radii);
          glUniform2f(glGetUniformLocation(shader, "viewport_size"), viewport_dim.x, viewport_dim.y);
          glUniform1i(glGetUniformLocation(shader, "blur_count"), blur_count);
          glUniform4fv(glGetUniformLocation(shader, "kernel"), ArrayCount(kernel), (F32 *)&kernel[0]);
          glActiveTexture(GL_TEXTURE0);
          
          //- rjf: set up scissor
          if(params->clip.x0 != 0 ||
             params->clip.x1 != 0 ||
             params->clip.y0 != 0 ||
             params->clip.y1 != 0)
          {
            if(r_ogl_scissor(params->clip, viewport_dim))
            {
              glEnable(GL_SCISSOR_TEST);
            }
          }
          
          //- rjf: pass 1: stage -(horizontal)-> stage_scratch
          glBindFramebufferScope(GL_FRAMEBUFFER, w->stage_scratch_target.fbo)
            glBindTextureScope(GL_TEXTURE_2D, w->stage_target.color_texture)
          {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glUniform2f(glGetUniformLocation(shader, "direction"), 1.f/viewport_dim.x, 0);
            glUniform1i(glGetUniformLocation(shader, "tex"), 0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
          }
          
          //- rjf: pass 2: stage_scratch -(vertical)-> stage
          glBindFramebufferScope(GL_FRAMEBUFFER, w->stage_target.fbo)
            glBindTextureScope(GL_TEXTURE_2D, w->stage_scratch_target.color_texture)
          {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glUniform2f(glGetUniformLocation(shader, "direction"), 0, 1.f/viewport_dim.y);
            glUniform1i(glGetUniformLocation(shader, "tex"), 0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
          }
          
          //- rjf: unset scissor
          glDisable(GL_SCISSOR_TEST);
        }
      }break;
      
      
      ////////////////////////
      //- rjf: 3d geometry rendering pass
      //
      case R_PassKind_Geo3D:
      {
        //- rjf: unpack params
        R_PassParams_Geo3D *params = pass->params_geo3d;
        R_BatchGroup3DMap *mesh_group_map = &params->mesh_batches;
        // TODO(rjf)
      }break;
    }
  }
}
