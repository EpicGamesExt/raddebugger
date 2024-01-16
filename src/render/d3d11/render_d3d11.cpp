// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef RADDBG_LAYER_COLOR
#define RADDBG_LAYER_COLOR 0.80f, 0.60f, 0.20f

////////////////////////////////
//~ rjf: Input Layout Element Tables

global D3D11_INPUT_ELEMENT_DESC r_d3d11_g_rect_ilay_elements[] =
{
  { "POS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,                            0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "TEX",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "COL",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "COL",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "COL",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "COL",  3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "CRAD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "STY",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

global D3D11_INPUT_ELEMENT_DESC r_d3d11_g_mesh_ilay_elements[] =
{
  { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/render_d3d11.meta.c"

////////////////////////////////
//~ rjf: Helpers

internal R_D3D11_Window *
r_d3d11_window_from_handle(R_Handle handle)
{
  R_D3D11_Window *window = (R_D3D11_Window *)handle.u64[0];
  if(window->generation != handle.u64[1])
  {
    window = &r_d3d11_window_nil;
  }
  return window;
}

internal R_Handle
r_d3d11_handle_from_window(R_D3D11_Window *window)
{
  R_Handle handle = {0};
  handle.u64[0] = (U64)window;
  handle.u64[1] = window->generation;
  return handle;
}

internal R_D3D11_Tex2D *
r_d3d11_tex2d_from_handle(R_Handle handle)
{
  R_D3D11_Tex2D *texture = (R_D3D11_Tex2D *)handle.u64[0];
  if(texture == 0 || texture->generation != handle.u64[1])
  {
    texture = &r_d3d11_tex2d_nil;
  }
  return texture;
}

internal R_Handle
r_d3d11_handle_from_tex2d(R_D3D11_Tex2D *texture)
{
  R_Handle handle = {0};
  handle.u64[0] = (U64)texture;
  handle.u64[1] = texture->generation;
  return handle;
}

internal R_D3D11_Buffer *
r_d3d11_buffer_from_handle(R_Handle handle)
{
  R_D3D11_Buffer *buffer = (R_D3D11_Buffer *)handle.u64[0];
  if(buffer == 0 || buffer->generation != handle.u64[1])
  {
    buffer = &r_d3d11_buffer_nil;
  }
  return buffer;
}

internal R_Handle
r_d3d11_handle_from_buffer(R_D3D11_Buffer *buffer)
{
  R_Handle handle = {0};
  handle.u64[0] = (U64)buffer;
  handle.u64[1] = buffer->generation;
  return handle;
}

internal ID3D11Buffer *
r_d3d11_instance_buffer_from_size(U64 size)
{
  ID3D11Buffer *buffer = r_d3d11_state->instance_scratch_buffer_64kb;
  if(size > KB(64))
  {
    U64 flushed_buffer_size = size;
    flushed_buffer_size += MB(1)-1;
    flushed_buffer_size -= flushed_buffer_size%MB(1);
    
    // rjf: build buffer
    {
      D3D11_BUFFER_DESC desc = {0};
      {
        desc.ByteWidth      = flushed_buffer_size;
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      }
      HRESULT error = r_d3d11_state->device->CreateBuffer(&desc, 0, &buffer);
    }
    
    // rjf: push buffer to flush list
    R_D3D11_FlushBuffer *n = push_array(r_d3d11_state->buffer_flush_arena, R_D3D11_FlushBuffer, 1);
    n->buffer = buffer;
    SLLQueuePush(r_d3d11_state->first_buffer_to_flush, r_d3d11_state->last_buffer_to_flush, n);
  }
  return buffer;
}

////////////////////////////////
//~ rjf: Backend Hook Implementations

//- rjf: top-level layer initialization

r_hook void
r_init(CmdLine *cmdln)
{
  ProfBeginFunction();
  HRESULT error = 0;
  Arena *arena = arena_alloc();
  r_d3d11_state = push_array(arena, R_D3D11_State, 1);
  r_d3d11_state->arena = arena;
  r_d3d11_state->device_rw_mutex = os_rw_mutex_alloc();
  
  //- rjf: create base device
  UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if !defined(NDEBUG)
  if(!cmd_line_has_flag(cmdln, str8_lit("disable_d3d11_debug")))
  {
    creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
  }
#endif
  D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
  D3D_DRIVER_TYPE driver_type = D3D_DRIVER_TYPE_HARDWARE;
  if(cmd_line_has_flag(cmdln, str8_lit("force_d3d11_software")))
  {
    driver_type = D3D_DRIVER_TYPE_WARP;
  }
  error = D3D11CreateDevice(0,
                            driver_type,
                            0,
                            creation_flags,
                            feature_levels, ArrayCount(feature_levels),
                            D3D11_SDK_VERSION,
                            &r_d3d11_state->base_device, 0, &r_d3d11_state->base_device_ctx);
  if(FAILED(error) && driver_type == D3D_DRIVER_TYPE_HARDWARE)
  {
    // try with WARP driver as backup solution in case HW device is not available
    error = D3D11CreateDevice(0,
                              D3D_DRIVER_TYPE_WARP,
                              0,
                              creation_flags,
                              feature_levels, ArrayCount(feature_levels),
                              D3D11_SDK_VERSION,
                              &r_d3d11_state->base_device, 0, &r_d3d11_state->base_device_ctx);
  }

  if(FAILED(error))
  {
    char buffer[256] = {0};
    raddbg_snprintf(buffer, sizeof(buffer), "D3D11 device creation failure (%x). The process is terminating.", error);
    os_graphical_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
    os_exit_process(1);
  }
  
  //- rjf: enable break-on-error
#if !defined(NDEBUG)
  if(!cmd_line_has_flag(cmdln, str8_lit("disable_d3d11_debug")))
  {
    ID3D11InfoQueue *info = 0;
    error = r_d3d11_state->base_device->QueryInterface(__uuidof(ID3D11InfoQueue), (void **)(&info));
    if(SUCCEEDED(error))
    {
      error = info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
      error = info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
      info->Release();
    }
  }
#endif
  
  //- rjf: get main device
  error = r_d3d11_state->base_device->QueryInterface(__uuidof(ID3D11Device1), (void **)(&r_d3d11_state->device));
  error = r_d3d11_state->base_device_ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **)(&r_d3d11_state->device_ctx));
  
  //- rjf: get dxgi device/adapter/factory
  error = r_d3d11_state->device->QueryInterface(__uuidof(IDXGIDevice1), (void **)(&r_d3d11_state->dxgi_device));
  error = r_d3d11_state->dxgi_device->GetAdapter(&r_d3d11_state->dxgi_adapter);
  error = r_d3d11_state->dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), (void **)(&r_d3d11_state->dxgi_factory));
  
  //- rjf: create main rasterizer
  {
    D3D11_RASTERIZER_DESC1 desc = {D3D11_FILL_SOLID};
    {
      desc.FillMode = D3D11_FILL_SOLID;
      desc.CullMode = D3D11_CULL_BACK;
      desc.ScissorEnable = 1;
    }
    error = r_d3d11_state->device->CreateRasterizerState1(&desc, &r_d3d11_state->main_rasterizer);
  }
  
  //- rjf: create main blend state
  {
    D3D11_BLEND_DESC desc = {0};
    {
      desc.RenderTarget[0].BlendEnable            = 1;
      desc.RenderTarget[0].SrcBlend               = D3D11_BLEND_SRC_ALPHA;
      desc.RenderTarget[0].DestBlend              = D3D11_BLEND_INV_SRC_ALPHA; 
      desc.RenderTarget[0].BlendOp                = D3D11_BLEND_OP_ADD;
      desc.RenderTarget[0].SrcBlendAlpha          = D3D11_BLEND_ONE;
      desc.RenderTarget[0].DestBlendAlpha         = D3D11_BLEND_ZERO;
      desc.RenderTarget[0].BlendOpAlpha           = D3D11_BLEND_OP_ADD;
      desc.RenderTarget[0].RenderTargetWriteMask  = D3D11_COLOR_WRITE_ENABLE_ALL;
    }
    error = r_d3d11_state->device->CreateBlendState(&desc, &r_d3d11_state->main_blend_state);
  }
  
  //- rjf: create nearest-neighbor sampler
  {
    D3D11_SAMPLER_DESC desc = zero_struct;
    {
      desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
      desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    }
    error = r_d3d11_state->device->CreateSamplerState(&desc, &r_d3d11_state->samplers[R_Tex2DSampleKind_Nearest]);
  }
  
  //- rjf: create bilinear sampler
  {
    D3D11_SAMPLER_DESC desc = zero_struct;
    {
      desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    }
    error = r_d3d11_state->device->CreateSamplerState(&desc, &r_d3d11_state->samplers[R_Tex2DSampleKind_Linear]);
  }
  
  //- rjf: create noop depth/stencil state
  {
    D3D11_DEPTH_STENCIL_DESC desc = {0};
    {
      desc.DepthEnable    = FALSE;
      desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
      desc.DepthFunc      = D3D11_COMPARISON_LESS;
    }
    error = r_d3d11_state->device->CreateDepthStencilState(&desc, &r_d3d11_state->noop_depth_stencil);
  }
  
  //- rjf: create plain depth/stencil state
  {
    D3D11_DEPTH_STENCIL_DESC desc = {0};
    {
      desc.DepthEnable    = TRUE;
      desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
      desc.DepthFunc      = D3D11_COMPARISON_LESS;
    }
    error = r_d3d11_state->device->CreateDepthStencilState(&desc, &r_d3d11_state->plain_depth_stencil);
  }
  
  //- rjf: create buffers
  {
    D3D11_BUFFER_DESC desc = {0};
    {
      desc.ByteWidth      = KB(64);
      desc.Usage          = D3D11_USAGE_DYNAMIC;
      desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    error = r_d3d11_state->device->CreateBuffer(&desc, 0, &r_d3d11_state->instance_scratch_buffer_64kb);
  }
  
  //- rjf: build vertex shaders & input layouts
  for(R_D3D11_VShadKind kind = (R_D3D11_VShadKind)0;
      kind < R_D3D11_VShadKind_COUNT;
      kind = (R_D3D11_VShadKind)(kind+1))
  {
    String8 source = r_d3d11_g_vshad_kind_source_table[kind];
    String8 source_name = r_d3d11_g_vshad_kind_source_name_table[kind];
    D3D11_INPUT_ELEMENT_DESC *ilay_elements = r_d3d11_g_vshad_kind_elements_ptr_table[kind];
    U64 ilay_elements_count = r_d3d11_g_vshad_kind_elements_count_table[kind];
    
    // rjf: compile vertex shader
    ID3DBlob *vshad_source_blob = 0;
    ID3DBlob *vshad_source_errors = 0;
    ID3D11VertexShader *vshad = 0;
    {
      error = D3DCompile(source.str,
                         source.size,
                         (char *)source_name.str,
                         0,
                         0,
                         "vs_main",
                         "vs_5_0",
                         0,
                         0,
                         &vshad_source_blob,
                         &vshad_source_errors);
      String8 errors = {0};
      if(FAILED(error))
      {
        errors = str8((U8 *)vshad_source_errors->GetBufferPointer(),
                      (U64)vshad_source_errors->GetBufferSize());
        os_graphical_message(1, str8_lit("Vertex Shader Compilation Failure"), errors);
      }
      else
      {
        error = r_d3d11_state->device->CreateVertexShader(vshad_source_blob->GetBufferPointer(), vshad_source_blob->GetBufferSize(), 0, &vshad);
      }
    }
    
    // rjf: make input layout
    ID3D11InputLayout *ilay = 0;
    if(ilay_elements != 0)
    {
      error = r_d3d11_state->device->CreateInputLayout(ilay_elements, ilay_elements_count,
                                                       vshad_source_blob->GetBufferPointer(),
                                                       vshad_source_blob->GetBufferSize(),
                                                       &ilay);
    }

    vshad_source_blob->Release();
    
    // rjf: store
    r_d3d11_state->vshads[kind] = vshad;
    r_d3d11_state->ilays[kind] = ilay;
  }
  
  //- rjf: build pixel shaders
  for(R_D3D11_PShadKind kind = (R_D3D11_PShadKind)0;
      kind < R_D3D11_PShadKind_COUNT;
      kind = (R_D3D11_PShadKind)(kind+1))
  {
    String8 source = r_d3d11_g_pshad_kind_source_table[kind];
    String8 source_name = r_d3d11_g_pshad_kind_source_name_table[kind];
    
    // rjf: compile pixel shader
    ID3DBlob *pshad_source_blob = 0;
    ID3DBlob *pshad_source_errors = 0;
    ID3D11PixelShader *pshad = 0;
    {
      error = D3DCompile(source.str,
                         source.size,
                         (char *)source_name.str,
                         0,
                         0,
                         "ps_main",
                         "ps_5_0",
                         0,
                         0,
                         &pshad_source_blob,
                         &pshad_source_errors);
      String8 errors = {0};
      if(FAILED(error))
      {
        errors = str8((U8 *)pshad_source_errors->GetBufferPointer(),
                      (U64)pshad_source_errors->GetBufferSize());
        os_graphical_message(1, str8_lit("Pixel Shader Compilation Failure"), errors);
      }
      else
      {
        error = r_d3d11_state->device->CreatePixelShader(pshad_source_blob->GetBufferPointer(), pshad_source_blob->GetBufferSize(), 0, &pshad);
      }
    }

    pshad_source_blob->Release();
    
    // rjf: store
    r_d3d11_state->pshads[kind] = pshad;
  }
  
  //- rjf: build uniform type buffers
  for(R_D3D11_UniformTypeKind kind = (R_D3D11_UniformTypeKind)0;
      kind < R_D3D11_UniformTypeKind_COUNT;
      kind = (R_D3D11_UniformTypeKind)(kind+1))
  {
    ID3D11Buffer *buffer = 0;
    {
      D3D11_BUFFER_DESC desc = {0};
      {
        desc.ByteWidth = r_d3d11_g_uniform_type_kind_size_table[kind];
        desc.ByteWidth += 15;
        desc.ByteWidth -= desc.ByteWidth % 16;
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      }
      r_d3d11_state->device->CreateBuffer(&desc, 0, &buffer);
    }
    r_d3d11_state->uniform_type_kind_buffers[kind] = buffer;
  }
  
  //- rjf: create backup texture
  {
    U32 backup_texture_data[] =
    {
      0xff00ffff, 0x330033ff,
      0x330033ff, 0xff00ffff,
    };
    r_d3d11_state->backup_texture = r_tex2d_alloc(R_Tex2DKind_Static, v2s32(2, 2), R_Tex2DFormat_RGBA8, backup_texture_data);
  }
  
  //- rjf: initialize buffer flush state
  {
    r_d3d11_state->buffer_flush_arena = arena_alloc();
  }
  
  ProfEnd();
}

//- rjf: window setup/teardown

r_hook R_Handle
r_window_equip(OS_Handle handle)
{
  ProfBeginFunction();
  R_Handle result = {0};
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    //- rjf: allocate per-window-state
    R_D3D11_Window *window = r_d3d11_state->first_free_window;
    {
      if(window == 0)
      {
        window = push_array(r_d3d11_state->arena, R_D3D11_Window, 1);
      }
      else
      {
        U64 gen = window->generation;
        SLLStackPop(r_d3d11_state->first_free_window);
        MemoryZeroStruct(window);
        window->generation = gen;
      }
      window->generation += 1;
    }
    
    //- rjf: map os window handle -> hwnd
    HWND hwnd = {0};
    {
      W32_Window *w32_layer_window = w32_window_from_os_window(handle);
      hwnd = w32_hwnd_from_window(w32_layer_window);
    }
    
    //- rjf: create swapchain
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {0};
    {
      swapchain_desc.Width              = 0; // NOTE(rjf): use window width
      swapchain_desc.Height             = 0; // NOTE(rjf): use window height
      swapchain_desc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
      swapchain_desc.Stereo             = FALSE;
      swapchain_desc.SampleDesc.Count   = 1;
      swapchain_desc.SampleDesc.Quality = 0;
      swapchain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swapchain_desc.BufferCount        = 2;
      swapchain_desc.Scaling            = DXGI_SCALING_STRETCH;
      swapchain_desc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
      swapchain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
      swapchain_desc.Flags              = 0;
    }
    HRESULT error = r_d3d11_state->dxgi_factory->CreateSwapChainForHwnd(r_d3d11_state->device, hwnd, &swapchain_desc, 0, 0, &window->swapchain);
    if(FAILED(error))
    {
      char buffer[256] = {0};
      raddbg_snprintf(buffer, sizeof(buffer), "DXGI swap chain creation failure (%x). The process is terminating.", error);
      os_graphical_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
      os_exit_process(1);
    }
    
    //- rjf: create framebuffer & view
    window->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)(&window->framebuffer));
    r_d3d11_state->device->CreateRenderTargetView(window->framebuffer, 0, &window->framebuffer_rtv);
    
    result = r_d3d11_handle_from_window(window);
  }
  ProfEnd();
  return result;
}

r_hook void
r_window_unequip(OS_Handle handle, R_Handle equip_handle)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Window *window = r_d3d11_window_from_handle(equip_handle);
    window->stage_color_srv->Release();
    window->stage_color_rtv->Release();
    window->stage_color->Release();
    window->stage_scratch_color_srv->Release();
    window->stage_scratch_color_rtv->Release();
    window->stage_scratch_color->Release();
    window->framebuffer_rtv->Release();
    window->framebuffer->Release();
    window->swapchain->Release();
    window->generation += 1;
    SLLStackPush(r_d3d11_state->first_free_window, window);
  }
  ProfEnd();
}

//- rjf: textures

r_hook R_Handle
r_tex2d_alloc(R_Tex2DKind kind, Vec2S32 size, R_Tex2DFormat format, void *data)
{
  ProfBeginFunction();
  
  //- rjf: allocate
  R_D3D11_Tex2D *texture = 0;
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    texture = r_d3d11_state->first_free_tex2d;
    if(texture == 0)
    {
      texture = push_array(r_d3d11_state->arena, R_D3D11_Tex2D, 1);
    }
    else
    {
      U64 gen = texture->generation;
      SLLStackPop(r_d3d11_state->first_free_tex2d);
      MemoryZeroStruct(texture);
      texture->generation = gen;
    }
    texture->generation += 1;
  }
  
  //- rjf: kind * initial_data -> usage * cpu access flags
  D3D11_USAGE d3d11_usage = D3D11_USAGE_IMMUTABLE;
  UINT cpu_access_flags = 0;
  {
    switch(kind)
    {
      default:
      case R_Tex2DKind_Static:
      {
        if(data == 0)
        {
          d3d11_usage = D3D11_USAGE_DYNAMIC;
          cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
        }
      }break;
      case R_Tex2DKind_Dynamic:
      {
        d3d11_usage = D3D11_USAGE_DEFAULT;
        cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
      }break;
    }
  }
  
  //- rjf: format -> dxgi format
  DXGI_FORMAT dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
  {
    switch(format)
    {
      default:{}break;
      case R_Tex2DFormat_R8:    {dxgi_format = DXGI_FORMAT_R8_UNORM;}break;
      case R_Tex2DFormat_RG8:   {dxgi_format = DXGI_FORMAT_R8G8_UNORM;}break;
      case R_Tex2DFormat_RGBA8: {dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;}break;
      case R_Tex2DFormat_BGRA8: {dxgi_format = DXGI_FORMAT_B8G8R8A8_UNORM;}break;
      case R_Tex2DFormat_R16:   {dxgi_format = DXGI_FORMAT_R16_UNORM;}break;
      case R_Tex2DFormat_RGBA16:{dxgi_format = DXGI_FORMAT_R16G16B16A16_UNORM;}break;
      case R_Tex2DFormat_R32:   {dxgi_format = DXGI_FORMAT_R32_FLOAT;}break;
      case R_Tex2DFormat_RG32:  {dxgi_format = DXGI_FORMAT_R32G32_FLOAT;}break;
      case R_Tex2DFormat_RGBA32:{dxgi_format = DXGI_FORMAT_R32G32B32A32_FLOAT;}break;
    }
  }
  
  //- rjf: prep initial data, if passed
  D3D11_SUBRESOURCE_DATA initial_data_ = {0};
  D3D11_SUBRESOURCE_DATA *initial_data = 0;
  if(data != 0)
  {
    initial_data = &initial_data_;
    initial_data->pSysMem = data;
    initial_data->SysMemPitch = r_tex2d_format_bytes_per_pixel_table[format] * size.x;
  }
  
  //- rjf: create texture
  D3D11_TEXTURE2D_DESC texture_desc = {0};
  {
    texture_desc.Width              = size.x;
    texture_desc.Height             = size.y;
    texture_desc.MipLevels          = 1;
    texture_desc.ArraySize          = 1;
    texture_desc.Format             = dxgi_format;
    texture_desc.SampleDesc.Count   = 1;
    texture_desc.Usage              = d3d11_usage;
    texture_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags     = cpu_access_flags;
  }
  r_d3d11_state->device->CreateTexture2D(&texture_desc, initial_data, &texture->texture);
  
  //- rjf: create texture srv
  r_d3d11_state->device->CreateShaderResourceView(texture->texture, 0, &texture->view);
  
  //- rjf: fill basics
  {
    texture->kind = kind;
    texture->size = size;
    texture->format = format;
  }
  
  R_Handle result = r_d3d11_handle_from_tex2d(texture);
  ProfEnd();
  return result;
}

r_hook void
r_tex2d_release(R_Handle handle)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Tex2D *texture = r_d3d11_tex2d_from_handle(handle);
    SLLStackPush(r_d3d11_state->first_to_free_tex2d, texture);
  }
  ProfEnd();
}

r_hook R_Tex2DKind
r_kind_from_tex2d(R_Handle handle)
{
  R_D3D11_Tex2D *texture = r_d3d11_tex2d_from_handle(handle);
  return texture->kind;
}

r_hook Vec2S32
r_size_from_tex2d(R_Handle handle)
{
  R_D3D11_Tex2D *texture = r_d3d11_tex2d_from_handle(handle);
  return texture->size;
}

r_hook R_Tex2DFormat
r_format_from_tex2d(R_Handle handle)
{
  R_D3D11_Tex2D *texture = r_d3d11_tex2d_from_handle(handle);
  return texture->format;
}

r_hook void
r_fill_tex2d_region(R_Handle handle, Rng2S32 subrect, void *data)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Tex2D *texture = r_d3d11_tex2d_from_handle(handle);
    U64 bytes_per_pixel = r_tex2d_format_bytes_per_pixel_table[texture->format];
    Vec2S32 dim = v2s32(subrect.x1 - subrect.x0, subrect.y1 - subrect.y0);
    D3D11_BOX dst_box =
    {
      (UINT)subrect.x0, (UINT)subrect.y0, 0,
      (UINT)subrect.x1, (UINT)subrect.y1, 1,
    };
    r_d3d11_state->device_ctx->UpdateSubresource(texture->texture, 0, &dst_box, data, dim.x*bytes_per_pixel, 0);
  }
  ProfEnd();
}

//- rjf: buffers

r_hook R_Handle
r_buffer_alloc(R_BufferKind kind, U64 size, void *data)
{
  ProfBeginFunction();
  
  //- rjf: allocate
  R_D3D11_Buffer *buffer = 0;
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    buffer = r_d3d11_state->first_free_buffer;
    if(buffer == 0)
    {
      buffer = push_array(r_d3d11_state->arena, R_D3D11_Buffer, 1);
    }
    else
    {
      U64 gen = buffer->generation;
      SLLStackPop(r_d3d11_state->first_free_buffer);
      MemoryZeroStruct(buffer);
      buffer->generation = gen;
    }
    buffer->generation += 1;
  }
  
  //- rjf: kind * initial_data -> usage * cpu access flags
  D3D11_USAGE d3d11_usage = D3D11_USAGE_IMMUTABLE;
  UINT cpu_access_flags = 0;
  {
    switch(kind)
    {
      default:
      case R_BufferKind_Static:
      {
        if(data == 0)
        {
          d3d11_usage = D3D11_USAGE_DYNAMIC;
          cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
        }
      }break;
      case R_BufferKind_Dynamic:
      {
        d3d11_usage = D3D11_USAGE_DEFAULT;
        cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
      }break;
    }
  }
  
  //- rjf: prep initial data, if passed
  D3D11_SUBRESOURCE_DATA initial_data_ = {0};
  D3D11_SUBRESOURCE_DATA *initial_data = 0;
  if(data != 0)
  {
    initial_data = &initial_data_;
    initial_data->pSysMem = data;
  }
  
  //- rjf: create buffer
  D3D11_BUFFER_DESC desc = {0};
  {
    desc.ByteWidth      = size;
    desc.Usage          = d3d11_usage;
    desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER|D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = cpu_access_flags;
  }
  r_d3d11_state->device->CreateBuffer(&desc, initial_data, &buffer->buffer);
  
  //- rjf: fill basics
  {
    buffer->kind = kind;
    buffer->size = size;
  }
  
  R_Handle result = r_d3d11_handle_from_buffer(buffer);
  ProfEnd();
  return result;
}

r_hook void
r_buffer_release(R_Handle handle)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Buffer *buffer = r_d3d11_buffer_from_handle(handle);
    SLLStackPush(r_d3d11_state->first_to_free_buffer, buffer);
  }
  ProfEnd();
}

//- rjf: frame markers

r_hook void
r_begin_frame(void)
{
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    // NOTE(rjf): no-op
  }
}

r_hook void
r_end_frame(void)
{
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    for(R_D3D11_FlushBuffer *buffer = r_d3d11_state->first_buffer_to_flush; buffer != 0; buffer = buffer->next)
    {
      buffer->buffer->Release();
    }
    for(R_D3D11_Tex2D *tex = r_d3d11_state->first_to_free_tex2d, *next = 0;
        tex != 0;
        tex = next)
    {
      next = tex->next;
      tex->view->Release();
      tex->texture->Release();
      tex->generation += 1;
      SLLStackPush(r_d3d11_state->first_free_tex2d, tex);
    }
    for(R_D3D11_Buffer *buf = r_d3d11_state->first_to_free_buffer, *next = 0;
        buf != 0;
        buf = next)
    {
      next = buf->next;
      buf->buffer->Release();
      buf->generation += 1;
      SLLStackPush(r_d3d11_state->first_free_buffer, buf);
    }
    arena_clear(r_d3d11_state->buffer_flush_arena);
    r_d3d11_state->first_buffer_to_flush = r_d3d11_state->last_buffer_to_flush = 0;
    r_d3d11_state->first_to_free_tex2d  = 0;
    r_d3d11_state->first_to_free_buffer = 0;
  }
}

r_hook void
r_window_begin_frame(OS_Handle window, R_Handle window_equip)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Window *wnd = r_d3d11_window_from_handle(window_equip);
    ID3D11DeviceContext1 *d_ctx = r_d3d11_state->device_ctx;
    
    //- rjf: get resolution
    Rng2F32 client_rect = os_client_rect_from_window(window);
    Vec2S32 resolution = {(S32)(client_rect.x1 - client_rect.x0), (S32)(client_rect.y1 - client_rect.y0)};
    
    //- rjf: resolution change
    if(wnd->last_resolution.x != resolution.x ||
       wnd->last_resolution.y != resolution.y)
    {
      wnd->last_resolution = resolution;
      
      // rjf: resize swapchain & main framebuffer
      wnd->framebuffer_rtv->Release();
      wnd->framebuffer->Release();
      wnd->swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
      wnd->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)(&wnd->framebuffer));
      r_d3d11_state->device->CreateRenderTargetView(wnd->framebuffer, 0, &wnd->framebuffer_rtv);
      
      // rjf: release screen-sized render target resources, if there
      if(wnd->stage_scratch_color_srv){wnd->stage_scratch_color_srv->Release();}
      if(wnd->stage_scratch_color_rtv){wnd->stage_scratch_color_rtv->Release();}
      if(wnd->stage_scratch_color)    {wnd->stage_scratch_color->Release();}
      if(wnd->stage_color_srv)        {wnd->stage_color_srv->Release();}
      if(wnd->stage_color_rtv)        {wnd->stage_color_rtv->Release();}
      if(wnd->stage_color)            {wnd->stage_color->Release();}
      if(wnd->geo3d_color_srv)        {wnd->geo3d_color_srv->Release();}
      if(wnd->geo3d_color_rtv)        {wnd->geo3d_color_rtv->Release();}
      if(wnd->geo3d_color)            {wnd->geo3d_color->Release();}
      if(wnd->geo3d_depth_srv)        {wnd->geo3d_depth_srv->Release();}
      if(wnd->geo3d_depth_dsv)        {wnd->geo3d_depth_dsv->Release();}
      if(wnd->geo3d_depth)            {wnd->geo3d_depth->Release();}
      
      // rjf: create stage color targets
      {
        D3D11_TEXTURE2D_DESC color_desc = {};
        {
          wnd->framebuffer->GetDesc(&color_desc);
          color_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
          color_desc.BindFlags = D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        }
        D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
        {
          rtv_desc.Format         = color_desc.Format;
          rtv_desc.ViewDimension  = D3D11_RTV_DIMENSION_TEXTURE2D;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        {
          srv_desc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
          srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
          srv_desc.Texture2D.MipLevels       = -1;
        }
        r_d3d11_state->device->CreateTexture2D(&color_desc, 0, &wnd->stage_color);
        r_d3d11_state->device->CreateRenderTargetView(wnd->stage_color, &rtv_desc, &wnd->stage_color_rtv);
        r_d3d11_state->device->CreateShaderResourceView(wnd->stage_color, &srv_desc, &wnd->stage_color_srv);
        r_d3d11_state->device->CreateTexture2D(&color_desc, 0, &wnd->stage_scratch_color);
        r_d3d11_state->device->CreateRenderTargetView(wnd->stage_scratch_color, &rtv_desc, &wnd->stage_scratch_color_rtv);
        r_d3d11_state->device->CreateShaderResourceView(wnd->stage_scratch_color, &srv_desc, &wnd->stage_scratch_color_srv);
      }
      
      // rjf: create geo3d targets
      {
        D3D11_TEXTURE2D_DESC color_desc = {};
        {
          wnd->framebuffer->GetDesc(&color_desc);
          color_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
          color_desc.BindFlags = D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        }
        D3D11_RENDER_TARGET_VIEW_DESC color_rtv_desc = {};
        {
          color_rtv_desc.Format         = color_desc.Format;
          color_rtv_desc.ViewDimension  = D3D11_RTV_DIMENSION_TEXTURE2D;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC color_srv_desc = {};
        {
          color_srv_desc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
          color_srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
          color_srv_desc.Texture2D.MipLevels       = -1;
        }
        D3D11_TEXTURE2D_DESC depth_desc = {};
        {
          wnd->framebuffer->GetDesc(&depth_desc);
          depth_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
          depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
        }
        D3D11_DEPTH_STENCIL_VIEW_DESC depth_dsv_desc = {};
        {
          depth_dsv_desc.Flags = 0;
          depth_dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
          depth_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
          depth_dsv_desc.Texture2D.MipSlice = 0;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC depth_srv_desc = {};
        {
          depth_srv_desc.Format                    = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
          depth_srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
          depth_srv_desc.Texture2D.MostDetailedMip = 0;
          depth_srv_desc.Texture2D.MipLevels       = -1;
        }
        r_d3d11_state->device->CreateTexture2D(&color_desc, 0, &wnd->geo3d_color);
        r_d3d11_state->device->CreateRenderTargetView(wnd->geo3d_color, &color_rtv_desc, &wnd->geo3d_color_rtv);
        r_d3d11_state->device->CreateShaderResourceView(wnd->geo3d_color, &color_srv_desc, &wnd->geo3d_color_srv);
        r_d3d11_state->device->CreateTexture2D(&depth_desc, 0, &wnd->geo3d_depth);
        r_d3d11_state->device->CreateDepthStencilView(wnd->geo3d_depth, &depth_dsv_desc, &wnd->geo3d_depth_dsv);
        r_d3d11_state->device->CreateShaderResourceView(wnd->geo3d_depth, &depth_srv_desc, &wnd->geo3d_depth_srv);
      }
    }
    
    //- rjf: clear framebuffers
    Vec4F32 clear_color = {0, 0, 0, 0};
    d_ctx->ClearRenderTargetView(wnd->framebuffer_rtv, clear_color.v);
    d_ctx->ClearRenderTargetView(wnd->stage_color_rtv, clear_color.v);
  }
  ProfEnd();
}

r_hook void
r_window_end_frame(OS_Handle window, R_Handle window_equip)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Window *wnd = r_d3d11_window_from_handle(window_equip);
    ID3D11DeviceContext1 *d_ctx = r_d3d11_state->device_ctx;
    
    ////////////////////////////
    //- rjf: finalize, by writing staging buffer out to window framebuffer
    //
    {
      ID3D11SamplerState *sampler   = r_d3d11_state->samplers[R_Tex2DSampleKind_Nearest];
      ID3D11VertexShader *vshad     = r_d3d11_state->vshads[R_D3D11_VShadKind_Finalize];
      ID3D11PixelShader *pshad      = r_d3d11_state->pshads[R_D3D11_PShadKind_Finalize];
      
      // rjf: setup output merger
      d_ctx->OMSetRenderTargets(1, &wnd->framebuffer_rtv, 0);
      d_ctx->OMSetDepthStencilState(r_d3d11_state->noop_depth_stencil, 0);
      d_ctx->OMSetBlendState(r_d3d11_state->main_blend_state, 0, 0xffffffff);
      
      // rjf: set up rasterizer
      Vec2S32 resolution = wnd->last_resolution;
      D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (F32)resolution.x, (F32)resolution.y, 0.0f, 1.0f };
      d_ctx->RSSetViewports(1, &viewport);
      d_ctx->RSSetState(r_d3d11_state->main_rasterizer);
      
      // rjf: setup input assembly
      d_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      d_ctx->IASetInputLayout(0);
      
      // rjf: setup shaders
      d_ctx->VSSetShader(vshad, 0, 0);
      d_ctx->PSSetShader(pshad, 0, 0);
      d_ctx->PSSetShaderResources(0, 1, &wnd->stage_color_srv);
      d_ctx->PSSetSamplers(0, 1, &sampler);
      
      // rjf: setup scissor rect
      {
        D3D11_RECT rect = {0};
        rect.left = 0;
        rect.right = (LONG)wnd->last_resolution.x;
        rect.top = 0;
        rect.bottom = (LONG)wnd->last_resolution.y;
        d_ctx->RSSetScissorRects(1, &rect);
      }
      
      // rjf: draw
      d_ctx->Draw(4, 0);
    }
    
    ////////////////////////////
    //- rjf: present
    //
    HRESULT error = wnd->swapchain->Present(1, 0);
    if(FAILED(error))
    {
      char buffer[256] = {0};
      raddbg_snprintf(buffer, sizeof(buffer), "D3D11 present failure (%x). The process is terminating.", error);
      os_graphical_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
      os_exit_process(1);
    }
    d_ctx->ClearState();
  }
  ProfEnd();
}

//- rjf: render pass submission

r_hook void
r_window_submit(OS_Handle window, R_Handle window_equip, R_PassList *passes)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    ////////////////////////////
    //- rjf: unpack arguments
    //
    R_D3D11_Window *wnd = r_d3d11_window_from_handle(window_equip);
    ID3D11DeviceContext1 *d_ctx = r_d3d11_state->device_ctx;
    
    ////////////////////////////
    //- rjf: do passes
    //
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
          
          //- rjf: set up rasterizer
          Vec2S32 resolution = wnd->last_resolution;
          D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (F32)resolution.x, (F32)resolution.y, 0.0f, 1.0f };
          d_ctx->RSSetViewports(1, &viewport);
          d_ctx->RSSetState(r_d3d11_state->main_rasterizer);
          
          //- rjf: draw each batch group
          for(R_BatchGroup2DNode *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
          {
            R_BatchList *batches = &group_n->batches;
            R_BatchGroup2DParams *group_params = &group_n->params;
            
            // rjf: unpack pipeline info
            ID3D11SamplerState *sampler   = r_d3d11_state->samplers[group_params->tex_sample_kind];
            ID3D11VertexShader *vshad     = r_d3d11_state->vshads[R_D3D11_VShadKind_Rect];
            ID3D11InputLayout *ilay       = r_d3d11_state->ilays[R_D3D11_VShadKind_Rect];
            ID3D11PixelShader *pshad      = r_d3d11_state->pshads[R_D3D11_PShadKind_Rect];
            ID3D11Buffer *uniforms_buffer = r_d3d11_state->uniform_type_kind_buffers[R_D3D11_UniformTypeKind_Rect];
            
            // rjf: get & fill buffer
            ID3D11Buffer *buffer = r_d3d11_instance_buffer_from_size(batches->byte_count);
            {
              D3D11_MAPPED_SUBRESOURCE sub_rsrc = {0};
              r_d3d11_state->device_ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub_rsrc);
              U8 *dst_ptr = (U8 *)sub_rsrc.pData;
              U64 off = 0;
              for(R_BatchNode *batch_n = batches->first; batch_n != 0; batch_n = batch_n->next)
              {
                MemoryCopy(dst_ptr+off, batch_n->v.v, batch_n->v.byte_count);
                off += batch_n->v.byte_count;
              }
              r_d3d11_state->device_ctx->Unmap(buffer, 0);
            }
            
            // rjf: get texture
            R_Handle texture_handle = group_params->tex;
            if(r_handle_match(texture_handle, r_handle_zero()))
            {
              texture_handle = r_d3d11_state->backup_texture;
            }
            R_D3D11_Tex2D *texture = r_d3d11_tex2d_from_handle(texture_handle);
            
            // rjf: get texture sample map matrix, based on format
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
            
            // rjf: upload uniforms
            R_D3D11_Uniforms_Rect uniforms = {0};
            {
              uniforms.viewport_size             = v2f32(resolution.x, resolution.y);
              uniforms.opacity                   = 1-group_params->transparency;
              MemoryCopyArray(uniforms.texture_sample_channel_map, texture_sample_channel_map);
              uniforms.texture_t2d_size          = v2f32(texture->size.x, texture->size.y);
              uniforms.xform[0] = v4f32(group_params->xform.v[0][0], group_params->xform.v[1][0], group_params->xform.v[2][0], 0);
              uniforms.xform[1] = v4f32(group_params->xform.v[0][1], group_params->xform.v[1][1], group_params->xform.v[2][1], 0);
              uniforms.xform[2] = v4f32(group_params->xform.v[0][2], group_params->xform.v[1][2], group_params->xform.v[2][2], 0);
              Vec2F32 xform_2x2_col0 = v2f32(uniforms.xform[0].x, uniforms.xform[1].x);
              Vec2F32 xform_2x2_col1 = v2f32(uniforms.xform[0].y, uniforms.xform[1].y);
              uniforms.xform_scale.x = length_2f32(xform_2x2_col0);
              uniforms.xform_scale.y = length_2f32(xform_2x2_col1);
            }
            {
              D3D11_MAPPED_SUBRESOURCE sub_rsrc = {0};
              r_d3d11_state->device_ctx->Map(uniforms_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub_rsrc);
              MemoryCopy((U8 *)sub_rsrc.pData, &uniforms, sizeof(uniforms));
              r_d3d11_state->device_ctx->Unmap(uniforms_buffer, 0);
            }
            
            // rjf: setup output merger
            d_ctx->OMSetRenderTargets(1, &wnd->stage_color_rtv, 0);
            d_ctx->OMSetDepthStencilState(r_d3d11_state->noop_depth_stencil, 0);
            d_ctx->OMSetBlendState(r_d3d11_state->main_blend_state, 0, 0xffffffff);
            
            // rjf: setup input assembly
            U32 stride = batches->bytes_per_inst;
            U32 offset = 0;
            d_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            d_ctx->IASetInputLayout(ilay);
            d_ctx->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
            
            // rjf: setup shaders
            d_ctx->VSSetShader(vshad, 0, 0);
            d_ctx->VSSetConstantBuffers(0, 1, &uniforms_buffer);
            d_ctx->PSSetShader(pshad, 0, 0);
            d_ctx->PSSetConstantBuffers(0, 1, &uniforms_buffer);
            d_ctx->PSSetShaderResources(0, 1, &texture->view);
            d_ctx->PSSetSamplers(0, 1, &sampler);
            
            // rjf: setup scissor rect
            {
              Rng2F32 clip = group_params->clip;
              D3D11_RECT rect = {0};
              {
                if(clip.x0 == 0 && clip.y0 == 0 && clip.x1 == 0 && clip.y1 == 0)
                {
                  rect.left = 0;
                  rect.right = (LONG)wnd->last_resolution.x;
                  rect.top = 0;
                  rect.bottom = (LONG)wnd->last_resolution.y;
                }
                else if(clip.x0 > clip.x1 || clip.y0 > clip.y1)
                {
                  rect.left = 0;
                  rect.right = 0;
                  rect.top = 0;
                  rect.bottom = 0;
                }
                else
                {
                  rect.left = (LONG)clip.x0;
                  rect.right = (LONG)clip.x1;
                  rect.top = (LONG)clip.y0;
                  rect.bottom = (LONG)clip.y1;
                }
              }
              d_ctx->RSSetScissorRects(1, &rect);
            }
            
            // rjf: draw
            d_ctx->DrawInstanced(4, batches->byte_count / batches->bytes_per_inst, 0, 0);
          }
        }break;
        
        ////////////////////////
        //- rjf: blur rendering pass
        //
        case R_PassKind_Blur:
        {
          R_PassParams_Blur *params = pass->params_blur;
          ID3D11SamplerState *sampler   = r_d3d11_state->samplers[R_Tex2DSampleKind_Nearest];
          ID3D11VertexShader *vshad     = r_d3d11_state->vshads[R_D3D11_VShadKind_Blur];
          ID3D11PixelShader *pshad      = r_d3d11_state->pshads[R_D3D11_PShadKind_Blur];
          ID3D11Buffer *uniforms_buffer = r_d3d11_state->uniform_type_kind_buffers[R_D3D11_VShadKind_Blur];
          
          //- rjf: perform blur on each axis
          ID3D11RenderTargetView *rtvs[Axis2_COUNT] =
          {
            wnd->stage_scratch_color_rtv,
            wnd->stage_color_rtv,
          };
          ID3D11ShaderResourceView *srvs[Axis2_COUNT] =
          {
            wnd->stage_color_srv,
            wnd->stage_scratch_color_srv,
          };
          for(Axis2 axis = (Axis2)0; axis < Axis2_COUNT; axis = (Axis2)(axis+1))
          {
            // rjf: setup output merger
            d_ctx->OMSetRenderTargets(1, &rtvs[axis], 0);
            d_ctx->OMSetDepthStencilState(r_d3d11_state->noop_depth_stencil, 0);
            d_ctx->OMSetBlendState(r_d3d11_state->main_blend_state, 0, 0xffffffff);
            
            // rjf: set up viewport
            Vec2S32 resolution = wnd->last_resolution;
            D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (F32)resolution.x, (F32)resolution.y, 0.0f, 1.0f };
            d_ctx->RSSetViewports(1, &viewport);
            d_ctx->RSSetState(r_d3d11_state->main_rasterizer);
            
            // rjf: setup input assembly
            d_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            d_ctx->IASetInputLayout(0);
            
            // rjf: set up uniforms
            {
              F32 stdev = (params->blur_size-1.f)/2.f;
              F32 one_over_root_2pi_stdev2 = 1/sqrt_f32(2*pi32*stdev*stdev);
              F32 euler32 = 2.718281828459045f;
              R_D3D11_Uniforms_Blur uniforms = {0};
              uniforms.viewport_size  = v2f32(resolution.x, resolution.y);
              uniforms.rect           = params->rect;
              uniforms.blur_size      = params->blur_size;
              uniforms.is_vertical    = (F32)!!axis;
              MemoryCopyArray(uniforms.corner_radii.v, params->corner_radii);
              F32 kernel_x = 0;
              uniforms.kernel[0].v[0] = 1.f;
              if(stdev > 0.f)
              {
                for(U64 idx = 0; idx < ArrayCount(uniforms.kernel); idx += 1)
                {
                  for(U64 v_idx = 0; v_idx < ArrayCount(uniforms.kernel[idx].v); v_idx += 1)
                  {
                    uniforms.kernel[idx].v[v_idx] = one_over_root_2pi_stdev2*pow_f32(euler32, -kernel_x*kernel_x/(2.f*stdev*stdev)); 
                    kernel_x += 1;
                  }
                }
              }
              if(uniforms.kernel[0].v[0] > 1.f)
              {
                MemoryZeroArray(uniforms.kernel);
                uniforms.kernel[0].v[0] = 1.f;
              }
              D3D11_MAPPED_SUBRESOURCE sub_rsrc = {0};
              r_d3d11_state->device_ctx->Map(uniforms_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub_rsrc);
              MemoryCopy((U8 *)sub_rsrc.pData, &uniforms, sizeof(uniforms));
              r_d3d11_state->device_ctx->Unmap(uniforms_buffer, 0);
            }
            
            // rjf: setup shaders
            d_ctx->VSSetShader(vshad, 0, 0);
            d_ctx->VSSetConstantBuffers(0, 1, &uniforms_buffer);
            d_ctx->PSSetShader(pshad, 0, 0);
            d_ctx->PSSetConstantBuffers(0, 1, &uniforms_buffer);
            d_ctx->PSSetShaderResources(0, 1, &srvs[axis]);
            d_ctx->PSSetSamplers(0, 1, &sampler);
            
            // rjf: setup scissor rect
            {
              D3D11_RECT rect = {0};
              rect.left = 0;
              rect.right = (LONG)wnd->last_resolution.x;
              rect.top = 0;
              rect.bottom = (LONG)wnd->last_resolution.y;
              d_ctx->RSSetScissorRects(1, &rect);
            }
            
            // rjf: draw
            d_ctx->Draw(4, 0);
            
            // rjf: unset srv
            ID3D11ShaderResourceView *srv = 0;
            d_ctx->PSSetShaderResources(0, 1, &srv);
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
          
          //- rjf: set up rasterizer
          Vec2F32 viewport_dim = dim_2f32(params->viewport);
          D3D11_VIEWPORT viewport = { params->viewport.x0, params->viewport.y0, viewport_dim.x, viewport_dim.y, 0.f, 1.f };
          d_ctx->RSSetViewports(1, &viewport);
          d_ctx->RSSetState(r_d3d11_state->main_rasterizer);
          
          //- rjf: clear render targets
          {
            Vec4F32 bg_color = v4f32(0, 0, 0, 0);
            d_ctx->ClearRenderTargetView(wnd->geo3d_color_rtv, bg_color.v);
            d_ctx->ClearDepthStencilView(wnd->geo3d_depth_dsv, D3D11_CLEAR_DEPTH, 1.f, 0);
          }
          
          //- rjf: draw mesh batches
          {
            // rjf: grab pipeline info
            ID3D11VertexShader *vshad     = r_d3d11_state->vshads[R_D3D11_VShadKind_Mesh];
            ID3D11InputLayout *ilay       = r_d3d11_state->ilays[R_D3D11_VShadKind_Mesh];
            ID3D11PixelShader *pshad      = r_d3d11_state->pshads[R_D3D11_PShadKind_Mesh];
            ID3D11Buffer *uniforms_buffer = r_d3d11_state->uniform_type_kind_buffers[R_D3D11_VShadKind_Mesh];
            
            // rjf: setup output merger
            d_ctx->OMSetRenderTargets(1, &wnd->geo3d_color_rtv, wnd->geo3d_depth_dsv);
            d_ctx->OMSetDepthStencilState(r_d3d11_state->plain_depth_stencil, 0);
            d_ctx->OMSetBlendState(r_d3d11_state->main_blend_state, 0, 0xffffffff);
            
            // rjf: draw all batches
            for(U64 slot_idx = 0; slot_idx < mesh_group_map->slots_count; slot_idx += 1)
            {
              for(R_BatchGroup3DMapNode *n = mesh_group_map->slots[slot_idx]; n != 0; n = n->next)
              {
                // rjf: unpack group params
                R_BatchList *batches = &n->batches;
                R_BatchGroup3DParams *group_params = &n->params;
                R_D3D11_Buffer *mesh_vertices = r_d3d11_buffer_from_handle(group_params->mesh_vertices);
                R_D3D11_Buffer *mesh_indices = r_d3d11_buffer_from_handle(group_params->mesh_indices);
                
                // rjf: setup input assembly
                U32 stride = 11 * sizeof(F32);
                U32 offset = 0;
                d_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                d_ctx->IASetInputLayout(ilay);
                d_ctx->IASetVertexBuffers(0, 1, &mesh_vertices->buffer, &stride, &offset);
                d_ctx->IASetIndexBuffer(mesh_indices->buffer, DXGI_FORMAT_R32_UINT, 0);
                
                // rjf: setup uniforms buffer
                R_D3D11_Uniforms_Mesh uniforms = {0};
                {
                  uniforms.xform = mul_4x4f32(params->projection, params->view);
                }
                {
                  D3D11_MAPPED_SUBRESOURCE sub_rsrc = {0};
                  r_d3d11_state->device_ctx->Map(uniforms_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub_rsrc);
                  MemoryCopy((U8 *)sub_rsrc.pData, &uniforms, sizeof(uniforms));
                  r_d3d11_state->device_ctx->Unmap(uniforms_buffer, 0);
                }
                
                
                // rjf: setup shaders
                d_ctx->VSSetShader(vshad, 0, 0);
                d_ctx->VSSetConstantBuffers(0, 1, &uniforms_buffer);
                d_ctx->PSSetShader(pshad, 0, 0);
                d_ctx->PSSetConstantBuffers(0, 1, &uniforms_buffer);
                
                // rjf: setup scissor rect
                {
                  D3D11_RECT rect = {0};
                  {
                    rect.left = 0;
                    rect.right = (LONG)wnd->last_resolution.x;
                    rect.top = 0;
                    rect.bottom = (LONG)wnd->last_resolution.y;
                  }
                  d_ctx->RSSetScissorRects(1, &rect);
                }
                
                // rjf: draw
                d_ctx->DrawIndexed(mesh_indices->size/sizeof(U32), 0, 0);
              }
            }
          }
          
          //- rjf: composite to main staging buffer
          {
            ID3D11SamplerState *sampler   = r_d3d11_state->samplers[R_Tex2DSampleKind_Nearest];
            ID3D11VertexShader *vshad     = r_d3d11_state->vshads[R_D3D11_VShadKind_Geo3DComposite];
            ID3D11PixelShader *pshad      = r_d3d11_state->pshads[R_D3D11_PShadKind_Geo3DComposite];
            
            // rjf: setup output merger
            d_ctx->OMSetRenderTargets(1, &wnd->stage_color_rtv, 0);
            d_ctx->OMSetDepthStencilState(r_d3d11_state->noop_depth_stencil, 0);
            d_ctx->OMSetBlendState(r_d3d11_state->main_blend_state, 0, 0xffffffff);
            
            // rjf: set up rasterizer
            Vec2S32 resolution = wnd->last_resolution;
            D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (F32)resolution.x, (F32)resolution.y, 0.0f, 1.0f };
            d_ctx->RSSetViewports(1, &viewport);
            d_ctx->RSSetState(r_d3d11_state->main_rasterizer);
            
            // rjf: setup input assembly
            d_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            d_ctx->IASetInputLayout(0);
            
            // rjf: setup shaders
            d_ctx->VSSetShader(vshad, 0, 0);
            d_ctx->PSSetShader(pshad, 0, 0);
            d_ctx->PSSetShaderResources(0, 1, &wnd->geo3d_color_srv);
            d_ctx->PSSetSamplers(0, 1, &sampler);
            
            // rjf: setup scissor rect
            {
              D3D11_RECT rect = {0};
              Rng2F32 clip = params->clip;
              if(clip.x0 == 0 && clip.y0 == 0 && clip.x1 == 0 && clip.y1 == 0)
              {
                rect.left = 0;
                rect.right = (LONG)wnd->last_resolution.x;
                rect.top = 0;
                rect.bottom = (LONG)wnd->last_resolution.y;
              }
              else if(clip.x0 > clip.x1 || clip.y0 > clip.y1)
              {
                rect.left = 0;
                rect.right = 0;
                rect.top = 0;
                rect.bottom = 0;
              }
              else
              {
                rect.left = (LONG)clip.x0;
                rect.right = (LONG)clip.x1;
                rect.top = (LONG)clip.y0;
                rect.bottom = (LONG)clip.y1;
              }
              d_ctx->RSSetScissorRects(1, &rect);
            }
            
            // rjf: draw
            d_ctx->Draw(4, 0);
          }
        }break;
      }
    }
  }
  ProfEnd();
}
