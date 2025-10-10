// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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
  if(window == 0)
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
  return handle;
}

internal R_D3D11_Tex2D *
r_d3d11_tex2d_from_handle(R_Handle handle)
{
  R_D3D11_Tex2D *texture = (R_D3D11_Tex2D *)handle.u64[0];
  if(texture == 0)
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
  return handle;
}

internal R_D3D11_Buffer *
r_d3d11_buffer_from_handle(R_Handle handle)
{
  R_D3D11_Buffer *buffer = (R_D3D11_Buffer *)handle.u64[0];
  if(buffer == 0)
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
      HRESULT error = r_d3d11_state->device->lpVtbl->CreateBuffer(r_d3d11_state->device, &desc, 0, &buffer);
    }
    
    // rjf: push buffer to flush list
    R_D3D11_FlushBuffer *n = push_array(r_d3d11_state->buffer_flush_arena, R_D3D11_FlushBuffer, 1);
    n->buffer = buffer;
    SLLQueuePush(r_d3d11_state->first_buffer_to_flush, r_d3d11_state->last_buffer_to_flush, n);
  }
  return buffer;
}

internal void
r_usage_access_flags_from_resource_kind(R_ResourceKind kind, D3D11_USAGE *out_d3d11_usage, UINT *out_cpu_access_flags)
{
  switch(kind)
  {
    case R_ResourceKind_Static:
    {
      *out_d3d11_usage = D3D11_USAGE_IMMUTABLE;
      *out_cpu_access_flags = 0;
    }break;
    case R_ResourceKind_Dynamic:
    {
      *out_d3d11_usage = D3D11_USAGE_DEFAULT;
      *out_cpu_access_flags = 0;
    }break;
    case R_ResourceKind_Stream:
    {
      *out_d3d11_usage = D3D11_USAGE_DYNAMIC;
      *out_cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
    }break;
    default:
    {
      InvalidPath;
    }
  }
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
  r_d3d11_state->device_rw_mutex = rw_mutex_alloc();
  
  //- rjf: create base device
  ProfBegin("create base device");
  UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if BUILD_DEBUG
  if(cmd_line_has_flag(cmdln, str8_lit("d3d11_debug")))
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
    raddbg_snprintf(buffer, sizeof(buffer), "D3D11 device creation failure (%lx). The process is terminating.", error);
    os_graphical_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
    os_abort(1);
  }
  ProfEnd();
  
  //- rjf: enable break-on-error
#if BUILD_DEBUG
  if(cmd_line_has_flag(cmdln, str8_lit("d3d11_debug"))) ProfScope("enable break-on-error")
  {
    ID3D11InfoQueue *info = 0;
    error = r_d3d11_state->base_device->lpVtbl->QueryInterface(r_d3d11_state->base_device, &IID_ID3D11InfoQueue, (void **)(&info));
    if(SUCCEEDED(error))
    {
      error = info->lpVtbl->SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
      error = info->lpVtbl->SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
      info->lpVtbl->Release(info);
    }
  }
#endif
  
  //- rjf: get main device
  ProfBegin("get main device");
  error = r_d3d11_state->base_device->lpVtbl->QueryInterface(r_d3d11_state->base_device, &IID_ID3D11Device1, (void **)(&r_d3d11_state->device));
  error = r_d3d11_state->base_device_ctx->lpVtbl->QueryInterface(r_d3d11_state->base_device_ctx, &IID_ID3D11DeviceContext1, (void **)(&r_d3d11_state->device_ctx));
  ProfEnd();
  
  //- rjf: get dxgi device/adapter/factory
  ProfBegin("get dxgi device/adapter/factory");
  error = r_d3d11_state->device->lpVtbl->QueryInterface(r_d3d11_state->device, &IID_IDXGIDevice1, (void **)(&r_d3d11_state->dxgi_device));
  error = r_d3d11_state->dxgi_device->lpVtbl->GetAdapter(r_d3d11_state->dxgi_device, &r_d3d11_state->dxgi_adapter);
  error = r_d3d11_state->dxgi_adapter->lpVtbl->GetParent(r_d3d11_state->dxgi_adapter, &IID_IDXGIFactory2, (void **)(&r_d3d11_state->dxgi_factory));
  error = r_d3d11_state->dxgi_device->lpVtbl->SetMaximumFrameLatency(r_d3d11_state->dxgi_device, 1);
  ProfEnd();
  
  //- rjf: create main rasterizer
  ProfScope("create main rasterizer")
  {
    D3D11_RASTERIZER_DESC1 desc = {D3D11_FILL_SOLID};
    {
      desc.FillMode = D3D11_FILL_SOLID;
      desc.CullMode = D3D11_CULL_BACK;
      desc.ScissorEnable = 1;
    }
    error = r_d3d11_state->device->lpVtbl->CreateRasterizerState1(r_d3d11_state->device, &desc, &r_d3d11_state->main_rasterizer);
  }
  
  //- rjf: create main blend state
  ProfScope("create main blend state")
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
    error = r_d3d11_state->device->lpVtbl->CreateBlendState(r_d3d11_state->device, &desc, &r_d3d11_state->main_blend_state);
  }
  
  //- rjf: create empty blend state
  ProfScope("create empty blend state")
  {
    D3D11_BLEND_DESC desc = {0};
    {
      desc.RenderTarget[0].BlendEnable           = FALSE;
      desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }
    error = r_d3d11_state->device->lpVtbl->CreateBlendState(r_d3d11_state->device, &desc, &r_d3d11_state->no_blend_state);
  }
  
  //- rjf: create nearest-neighbor sampler
  ProfScope("create nearest-neighbor sampler")
  {
    D3D11_SAMPLER_DESC desc = zero_struct;
    {
      desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
      desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    }
    error = r_d3d11_state->device->lpVtbl->CreateSamplerState(r_d3d11_state->device, &desc, &r_d3d11_state->samplers[R_Tex2DSampleKind_Nearest]);
  }
  
  //- rjf: create bilinear sampler
  ProfScope("create bilinear sampler")
  {
    D3D11_SAMPLER_DESC desc = zero_struct;
    {
      desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
      desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    }
    error = r_d3d11_state->device->lpVtbl->CreateSamplerState(r_d3d11_state->device, &desc, &r_d3d11_state->samplers[R_Tex2DSampleKind_Linear]);
  }
  
  //- rjf: create noop depth/stencil state
  ProfScope("create noop depth/stencil state")
  {
    D3D11_DEPTH_STENCIL_DESC desc = {0};
    {
      desc.DepthEnable    = FALSE;
      desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
      desc.DepthFunc      = D3D11_COMPARISON_LESS;
    }
    error = r_d3d11_state->device->lpVtbl->CreateDepthStencilState(r_d3d11_state->device, &desc, &r_d3d11_state->noop_depth_stencil);
  }
  
  //- rjf: create plain depth/stencil state
  ProfScope("create plain depth/stencil state")
  {
    D3D11_DEPTH_STENCIL_DESC desc = {0};
    {
      desc.DepthEnable    = TRUE;
      desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
      desc.DepthFunc      = D3D11_COMPARISON_LESS;
    }
    error = r_d3d11_state->device->lpVtbl->CreateDepthStencilState(r_d3d11_state->device, &desc, &r_d3d11_state->plain_depth_stencil);
  }
  
  //- rjf: create buffers
  ProfScope("create buffers")
  {
    D3D11_BUFFER_DESC desc = {0};
    {
      desc.ByteWidth      = KB(64);
      desc.Usage          = D3D11_USAGE_DYNAMIC;
      desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    error = r_d3d11_state->device->lpVtbl->CreateBuffer(r_d3d11_state->device, &desc, 0, &r_d3d11_state->instance_scratch_buffer_64kb);
  }
  
  //- rjf: build vertex shaders & input layouts
  ProfScope("build vertex shaders & input layouts")
    for(R_D3D11_VShadKind kind = (R_D3D11_VShadKind)0;
        kind < R_D3D11_VShadKind_COUNT;
        kind = (R_D3D11_VShadKind)(kind+1))
  {
    String8 source = *r_d3d11_g_vshad_kind_source_table[kind];
    String8 source_name = r_d3d11_g_vshad_kind_source_name_table[kind];
    D3D11_INPUT_ELEMENT_DESC *ilay_elements = r_d3d11_g_vshad_kind_elements_ptr_table[kind];
    U64 ilay_elements_count = r_d3d11_g_vshad_kind_elements_count_table[kind];
    
    // rjf: compile vertex shader
    ID3DBlob *vshad_source_blob = 0;
    ID3DBlob *vshad_source_errors = 0;
    ID3D11VertexShader *vshad = 0;
    ProfScope("compile vertex shader")
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
        errors = str8((U8 *)vshad_source_errors->lpVtbl->GetBufferPointer(vshad_source_errors),
                      (U64)vshad_source_errors->lpVtbl->GetBufferSize(vshad_source_errors));
        os_graphical_message(1, str8_lit("Vertex Shader Compilation Failure"), errors);
      }
      else
      {
        error = r_d3d11_state->device->lpVtbl->CreateVertexShader(r_d3d11_state->device, vshad_source_blob->lpVtbl->GetBufferPointer(vshad_source_blob), vshad_source_blob->lpVtbl->GetBufferSize(vshad_source_blob), 0, &vshad);
      }
    }
    
    // rjf: make input layout
    ID3D11InputLayout *ilay = 0;
    if(ilay_elements != 0)
    {
      error = r_d3d11_state->device->lpVtbl->CreateInputLayout(r_d3d11_state->device, ilay_elements, ilay_elements_count,
                                                               vshad_source_blob->lpVtbl->GetBufferPointer(vshad_source_blob),
                                                               vshad_source_blob->lpVtbl->GetBufferSize(vshad_source_blob),
                                                               &ilay);
    }
    
    vshad_source_blob->lpVtbl->Release(vshad_source_blob);
    
    // rjf: store
    r_d3d11_state->vshads[kind] = vshad;
    r_d3d11_state->ilays[kind] = ilay;
  }
  
  //- rjf: build pixel shaders
  for(R_D3D11_PShadKind kind = (R_D3D11_PShadKind)0;
      kind < R_D3D11_PShadKind_COUNT;
      kind = (R_D3D11_PShadKind)(kind+1))
  {
    String8 source = *r_d3d11_g_pshad_kind_source_table[kind];
    String8 source_name = r_d3d11_g_pshad_kind_source_name_table[kind];
    
    // rjf: compile pixel shader
    ID3DBlob *pshad_source_blob = 0;
    ID3DBlob *pshad_source_errors = 0;
    ID3D11PixelShader *pshad = 0;
    ProfScope("compile pixel shader")
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
        errors = str8((U8 *)pshad_source_errors->lpVtbl->GetBufferPointer(pshad_source_errors),
                      (U64)pshad_source_errors->lpVtbl->GetBufferSize(pshad_source_errors));
        os_graphical_message(1, str8_lit("Pixel Shader Compilation Failure"), errors);
      }
      else
      {
        error = r_d3d11_state->device->lpVtbl->CreatePixelShader(r_d3d11_state->device, pshad_source_blob->lpVtbl->GetBufferPointer(pshad_source_blob), pshad_source_blob->lpVtbl->GetBufferSize(pshad_source_blob), 0, &pshad);
      }
    }
    
    pshad_source_blob->lpVtbl->Release(pshad_source_blob);
    
    // rjf: store
    r_d3d11_state->pshads[kind] = pshad;
  }
  
  //- rjf: build uniform type buffers
  ProfScope("build uniform type buffers")
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
      r_d3d11_state->device->lpVtbl->CreateBuffer(r_d3d11_state->device, &desc, 0, &buffer);
    }
    r_d3d11_state->uniform_type_kind_buffers[kind] = buffer;
  }
  
  //- rjf: create backup texture
  ProfScope("create backup texture")
  {
    U32 backup_texture_data[] =
    {
      0xff00ffff, 0x330033ff,
      0x330033ff, 0xff00ffff,
    };
    r_d3d11_state->backup_texture = r_tex2d_alloc(R_ResourceKind_Static, v2s32(2, 2), R_Tex2DFormat_RGBA8, backup_texture_data);
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
  MutexScopeW(r_d3d11_state->device_rw_mutex)
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
      OS_W32_Window *w32_layer_window = os_w32_window_from_handle(handle);
      hwnd = os_w32_hwnd_from_window(w32_layer_window);
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
      swapchain_desc.Scaling            = DXGI_SCALING_NONE;
      swapchain_desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
      swapchain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
      swapchain_desc.Flags              = 0;
    }
    HRESULT error = r_d3d11_state->dxgi_factory->lpVtbl->CreateSwapChainForHwnd(r_d3d11_state->dxgi_factory, (IUnknown *)r_d3d11_state->device, hwnd, &swapchain_desc, 0, 0, &window->swapchain);
    if(FAILED(error))
    {
      char buffer[256] = {0};
      raddbg_snprintf(buffer, sizeof(buffer), "DXGI swap chain creation failure (%lx). The process is terminating.", error);
      os_graphical_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
      os_abort(1);
    }
    
    r_d3d11_state->dxgi_factory->lpVtbl->MakeWindowAssociation(r_d3d11_state->dxgi_factory, hwnd, DXGI_MWA_NO_ALT_ENTER);
    
    //- rjf: create framebuffer & view
    D3D11_RENDER_TARGET_VIEW_DESC framebuffer_rtv_desc = {0};
    framebuffer_rtv_desc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    framebuffer_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    window->swapchain->lpVtbl->GetBuffer(window->swapchain, 0, &IID_ID3D11Texture2D, (void **)(&window->framebuffer));
    r_d3d11_state->device->lpVtbl->CreateRenderTargetView(r_d3d11_state->device, (ID3D11Resource *)window->framebuffer, &framebuffer_rtv_desc, &window->framebuffer_rtv);
    
    result = r_d3d11_handle_from_window(window);
    r_d3d11_state->window_count += 1;
    r_d3d11_state->dxgi_device->lpVtbl->SetMaximumFrameLatency(r_d3d11_state->dxgi_device, Clamp(1, r_d3d11_state->window_count, 16));
  }
  ProfEnd();
  return result;
}

r_hook void
r_window_unequip(OS_Handle handle, R_Handle equip_handle)
{
  ProfBeginFunction();
  MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Window *window = r_d3d11_window_from_handle(equip_handle);
    window->stage_color_srv->lpVtbl->Release(window->stage_color_srv);
    window->stage_color_rtv->lpVtbl->Release(window->stage_color_rtv);
    window->stage_color->lpVtbl->Release(window->stage_color);
    window->stage_scratch_color_srv->lpVtbl->Release(window->stage_scratch_color_srv);
    window->stage_scratch_color_rtv->lpVtbl->Release(window->stage_scratch_color_rtv);
    window->stage_scratch_color->lpVtbl->Release(window->stage_scratch_color);
    window->framebuffer_rtv->lpVtbl->Release(window->framebuffer_rtv);
    window->framebuffer->lpVtbl->Release(window->framebuffer);
    window->swapchain->lpVtbl->Release(window->swapchain);
    window->generation += 1;
    SLLStackPush(r_d3d11_state->first_free_window, window);
    r_d3d11_state->window_count -= 1;
    r_d3d11_state->dxgi_device->lpVtbl->SetMaximumFrameLatency(r_d3d11_state->dxgi_device, Clamp(1, r_d3d11_state->window_count, 16));
  }
  ProfEnd();
}

//- rjf: textures

r_hook R_Handle
r_tex2d_alloc(R_ResourceKind kind, Vec2S32 size, R_Tex2DFormat format, void *data)
{
  ProfBeginFunction();
  
  //- rjf: allocate
  R_D3D11_Tex2D *texture = 0;
  MutexScopeW(r_d3d11_state->device_rw_mutex)
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
  
  D3D11_USAGE d3d11_usage = D3D11_USAGE_DEFAULT;
  UINT cpu_access_flags = 0;
  r_usage_access_flags_from_resource_kind(kind, &d3d11_usage, &cpu_access_flags);
  if (kind == R_ResourceKind_Static)
  {
    Assert(data != 0 && "static texture must have initial data provided");
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
  r_d3d11_state->device->lpVtbl->CreateTexture2D(r_d3d11_state->device, &texture_desc, initial_data, &texture->texture);
  
  //- rjf: create texture srv
  r_d3d11_state->device->lpVtbl->CreateShaderResourceView(r_d3d11_state->device, (ID3D11Resource *)texture->texture, 0, &texture->view);
  
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
  MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Tex2D *texture = r_d3d11_tex2d_from_handle(handle);
    if(texture != &r_d3d11_tex2d_nil)
    {
      SLLStackPush(r_d3d11_state->first_to_free_tex2d, texture);
    }
  }
  ProfEnd();
}

r_hook R_ResourceKind
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
  MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Tex2D *texture = r_d3d11_tex2d_from_handle(handle);
    if(texture != &r_d3d11_tex2d_nil)
    {
      Assert(texture->kind == R_ResourceKind_Dynamic && "only dynamic texture can update region");
      U64 bytes_per_pixel = r_tex2d_format_bytes_per_pixel_table[texture->format];
      Vec2S32 dim = v2s32(subrect.x1 - subrect.x0, subrect.y1 - subrect.y0);
      D3D11_BOX dst_box =
      {
        (UINT)subrect.x0, (UINT)subrect.y0, 0,
        (UINT)subrect.x1, (UINT)subrect.y1, 1,
      };
      r_d3d11_state->device_ctx->lpVtbl->UpdateSubresource(r_d3d11_state->device_ctx, (ID3D11Resource *)texture->texture, 0, &dst_box, data, dim.x*bytes_per_pixel, 0);
    }
  }
  ProfEnd();
}

//- rjf: buffers

r_hook R_Handle
r_buffer_alloc(R_ResourceKind kind, U64 size, void *data)
{
  ProfBeginFunction();
  
  //- rjf: allocate
  R_D3D11_Buffer *buffer = 0;
  MutexScopeW(r_d3d11_state->device_rw_mutex)
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
  
  D3D11_USAGE d3d11_usage = D3D11_USAGE_DEFAULT;
  UINT cpu_access_flags = 0;
  r_usage_access_flags_from_resource_kind(kind, &d3d11_usage, &cpu_access_flags);
  if (kind == R_ResourceKind_Static)
  {
    Assert(data != 0 && "static buffer must have initial data provided");
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
  r_d3d11_state->device->lpVtbl->CreateBuffer(r_d3d11_state->device, &desc, initial_data, &buffer->buffer);
  
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
  MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Buffer *buffer = r_d3d11_buffer_from_handle(handle);
    if(buffer != &r_d3d11_buffer_nil)
    {
      SLLStackPush(r_d3d11_state->first_to_free_buffer, buffer);
    }
  }
  ProfEnd();
}

//- rjf: frame markers

r_hook void
r_begin_frame(void)
{
  MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    // NOTE(rjf): no-op
  }
}

r_hook void
r_end_frame(void)
{
  MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    for(R_D3D11_FlushBuffer *buffer = r_d3d11_state->first_buffer_to_flush; buffer != 0; buffer = buffer->next)
    {
      buffer->buffer->lpVtbl->Release(buffer->buffer);
    }
    for(R_D3D11_Tex2D *tex = r_d3d11_state->first_to_free_tex2d, *next = 0;
        tex != 0;
        tex = next)
    {
      next = tex->next;
      if(tex->view != 0)
      {
        tex->view->lpVtbl->Release(tex->view);
      }
      if(tex->texture != 0)
      {
        tex->texture->lpVtbl->Release(tex->texture);
      }
      tex->view = 0;
      tex->texture = 0;
      tex->generation += 1;
      SLLStackPush(r_d3d11_state->first_free_tex2d, tex);
    }
    for(R_D3D11_Buffer *buf = r_d3d11_state->first_to_free_buffer, *next = 0;
        buf != 0;
        buf = next)
    {
      next = buf->next;
      if(buf->buffer != 0)
      {
        buf->buffer->lpVtbl->Release(buf->buffer);
      }
      buf->generation += 1;
      buf->buffer = 0;
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
  MutexScopeW(r_d3d11_state->device_rw_mutex)
  {
    R_D3D11_Window *wnd = r_d3d11_window_from_handle(window_equip);
    ID3D11DeviceContext1 *d_ctx = r_d3d11_state->device_ctx;
    
    //- rjf: get resolution
    Rng2F32 client_rect = os_client_rect_from_window(window);
    Vec2S32 resolution = {(S32)(client_rect.x1 - client_rect.x0), (S32)(client_rect.y1 - client_rect.y0)};
    
    //- rjf: resolution change
    B32 resize_done = 0;
    if(wnd->last_resolution.x != resolution.x ||
       wnd->last_resolution.y != resolution.y)
    {
      resize_done = 1;
      wnd->last_resolution = resolution;
      
      // rjf: release screen-sized render target resources, if there
      if(wnd->stage_scratch_color_srv){wnd->stage_scratch_color_srv->lpVtbl->Release(wnd->stage_scratch_color_srv);}
      if(wnd->stage_scratch_color_rtv){wnd->stage_scratch_color_rtv->lpVtbl->Release(wnd->stage_scratch_color_rtv);}
      if(wnd->stage_scratch_color)    {wnd->stage_scratch_color->lpVtbl->Release(wnd->stage_scratch_color);}
      if(wnd->stage_color_srv)        {wnd->stage_color_srv->lpVtbl->Release(wnd->stage_color_srv);}
      if(wnd->stage_color_rtv)        {wnd->stage_color_rtv->lpVtbl->Release(wnd->stage_color_rtv);}
      if(wnd->stage_color)            {wnd->stage_color->lpVtbl->Release(wnd->stage_color);}
      if(wnd->geo3d_color_srv)        {wnd->geo3d_color_srv->lpVtbl->Release(wnd->geo3d_color_srv);}
      if(wnd->geo3d_color_rtv)        {wnd->geo3d_color_rtv->lpVtbl->Release(wnd->geo3d_color_rtv);}
      if(wnd->geo3d_color)            {wnd->geo3d_color->lpVtbl->Release(wnd->geo3d_color);}
      if(wnd->geo3d_depth_srv)        {wnd->geo3d_depth_srv->lpVtbl->Release(wnd->geo3d_depth_srv);}
      if(wnd->geo3d_depth_dsv)        {wnd->geo3d_depth_dsv->lpVtbl->Release(wnd->geo3d_depth_dsv);}
      if(wnd->geo3d_depth)            {wnd->geo3d_depth->lpVtbl->Release(wnd->geo3d_depth);}
      
      // rjf: resize swapchain & main framebuffer
      wnd->framebuffer_rtv->lpVtbl->Release(wnd->framebuffer_rtv);
      wnd->framebuffer->lpVtbl->Release(wnd->framebuffer);
      wnd->swapchain->lpVtbl->ResizeBuffers(wnd->swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
      wnd->swapchain->lpVtbl->GetBuffer(wnd->swapchain, 0, &IID_ID3D11Texture2D, (void **)(&wnd->framebuffer));
      D3D11_RENDER_TARGET_VIEW_DESC framebuffer_rtv_desc = {0};
      framebuffer_rtv_desc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
      framebuffer_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
      r_d3d11_state->device->lpVtbl->CreateRenderTargetView(r_d3d11_state->device, (ID3D11Resource *)wnd->framebuffer, &framebuffer_rtv_desc, &wnd->framebuffer_rtv);
      
      // rjf: create stage color targets
      {
        D3D11_TEXTURE2D_DESC color_desc = zero_struct;
        {
          wnd->framebuffer->lpVtbl->GetDesc(wnd->framebuffer, &color_desc);
          color_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
          color_desc.BindFlags = D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        }
        D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = zero_struct;
        {
          rtv_desc.Format         = color_desc.Format;
          rtv_desc.ViewDimension  = D3D11_RTV_DIMENSION_TEXTURE2D;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = zero_struct;
        {
          srv_desc.Format                    = DXGI_FORMAT_R16G16B16A16_FLOAT;
          srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
          srv_desc.Texture2D.MipLevels       = -1;
        }
        r_d3d11_state->device->lpVtbl->CreateTexture2D(r_d3d11_state->device, &color_desc, 0, &wnd->stage_color);
        r_d3d11_state->device->lpVtbl->CreateRenderTargetView(r_d3d11_state->device, (ID3D11Resource *)wnd->stage_color, &rtv_desc, &wnd->stage_color_rtv);
        r_d3d11_state->device->lpVtbl->CreateShaderResourceView(r_d3d11_state->device, (ID3D11Resource *)wnd->stage_color, &srv_desc, &wnd->stage_color_srv);
        r_d3d11_state->device->lpVtbl->CreateTexture2D(r_d3d11_state->device, &color_desc, 0, &wnd->stage_scratch_color);
        r_d3d11_state->device->lpVtbl->CreateRenderTargetView(r_d3d11_state->device, (ID3D11Resource *)wnd->stage_scratch_color, &rtv_desc, &wnd->stage_scratch_color_rtv);
        r_d3d11_state->device->lpVtbl->CreateShaderResourceView(r_d3d11_state->device, (ID3D11Resource *)wnd->stage_scratch_color, &srv_desc, &wnd->stage_scratch_color_srv);
      }
      
      // rjf: create geo3d targets
      {
        D3D11_TEXTURE2D_DESC color_desc = zero_struct;
        {
          wnd->framebuffer->lpVtbl->GetDesc(wnd->framebuffer, &color_desc);
          color_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
          color_desc.BindFlags = D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        }
        D3D11_RENDER_TARGET_VIEW_DESC color_rtv_desc = zero_struct;
        {
          color_rtv_desc.Format         = color_desc.Format;
          color_rtv_desc.ViewDimension  = D3D11_RTV_DIMENSION_TEXTURE2D;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC color_srv_desc = zero_struct;
        {
          color_srv_desc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
          color_srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
          color_srv_desc.Texture2D.MipLevels       = -1;
        }
        D3D11_TEXTURE2D_DESC depth_desc = zero_struct;
        {
          wnd->framebuffer->lpVtbl->GetDesc(wnd->framebuffer, &depth_desc);
          depth_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
          depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
        }
        D3D11_DEPTH_STENCIL_VIEW_DESC depth_dsv_desc = zero_struct;
        {
          depth_dsv_desc.Flags = 0;
          depth_dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
          depth_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
          depth_dsv_desc.Texture2D.MipSlice = 0;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC depth_srv_desc = zero_struct;
        {
          depth_srv_desc.Format                    = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
          depth_srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
          depth_srv_desc.Texture2D.MostDetailedMip = 0;
          depth_srv_desc.Texture2D.MipLevels       = -1;
        }
        r_d3d11_state->device->lpVtbl->CreateTexture2D(r_d3d11_state->device, &color_desc, 0, &wnd->geo3d_color);
        r_d3d11_state->device->lpVtbl->CreateRenderTargetView(r_d3d11_state->device, (ID3D11Resource *)wnd->geo3d_color, &color_rtv_desc, &wnd->geo3d_color_rtv);
        r_d3d11_state->device->lpVtbl->CreateShaderResourceView(r_d3d11_state->device, (ID3D11Resource *)wnd->geo3d_color, &color_srv_desc, &wnd->geo3d_color_srv);
        r_d3d11_state->device->lpVtbl->CreateTexture2D(r_d3d11_state->device, &depth_desc, 0, &wnd->geo3d_depth);
        r_d3d11_state->device->lpVtbl->CreateDepthStencilView(r_d3d11_state->device, (ID3D11Resource *)wnd->geo3d_depth, &depth_dsv_desc, &wnd->geo3d_depth_dsv);
        r_d3d11_state->device->lpVtbl->CreateShaderResourceView(r_d3d11_state->device, (ID3D11Resource *)wnd->geo3d_depth, &depth_srv_desc, &wnd->geo3d_depth_srv);
      }
    }
    
    //- rjf: clear framebuffers
    Vec4F32 clear_color = {0, 0, 0, 0};
    d_ctx->lpVtbl->ClearRenderTargetView(d_ctx, wnd->framebuffer_rtv, clear_color.v);
    d_ctx->lpVtbl->ClearRenderTargetView(d_ctx, wnd->stage_color_rtv, clear_color.v);
    if(resize_done)
    {
      d_ctx->lpVtbl->Flush(d_ctx);
    }
  }
  ProfEnd();
}

r_hook void
r_window_end_frame(OS_Handle window, R_Handle window_equip)
{
  ProfBeginFunction();
  MutexScopeW(r_d3d11_state->device_rw_mutex)
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
      d_ctx->lpVtbl->OMSetRenderTargets(d_ctx, 1, &wnd->framebuffer_rtv, 0);
      d_ctx->lpVtbl->OMSetDepthStencilState(d_ctx, r_d3d11_state->noop_depth_stencil, 0);
      d_ctx->lpVtbl->OMSetBlendState(d_ctx, r_d3d11_state->main_blend_state, 0, 0xffffffff);
      
      // rjf: set up rasterizer
      Vec2S32 resolution = wnd->last_resolution;
      D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (F32)resolution.x, (F32)resolution.y, 0.0f, 1.0f };
      d_ctx->lpVtbl->RSSetViewports(d_ctx, 1, &viewport);
      d_ctx->lpVtbl->RSSetState(d_ctx, (ID3D11RasterizerState *)r_d3d11_state->main_rasterizer);
      
      // rjf: setup input assembly
      d_ctx->lpVtbl->IASetPrimitiveTopology(d_ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      d_ctx->lpVtbl->IASetInputLayout(d_ctx, 0);
      
      // rjf: setup shaders
      d_ctx->lpVtbl->VSSetShader(d_ctx, vshad, 0, 0);
      d_ctx->lpVtbl->PSSetShader(d_ctx, pshad, 0, 0);
      d_ctx->lpVtbl->PSSetShaderResources(d_ctx, 0, 1, &wnd->stage_color_srv);
      d_ctx->lpVtbl->PSSetSamplers(d_ctx, 0, 1, &sampler);
      
      // rjf: setup scissor rect
      {
        D3D11_RECT rect = {0};
        rect.left = 0;
        rect.right = (LONG)wnd->last_resolution.x;
        rect.top = 0;
        rect.bottom = (LONG)wnd->last_resolution.y;
        d_ctx->lpVtbl->RSSetScissorRects(d_ctx, 1, &rect);
      }
      
      // rjf: draw
      d_ctx->lpVtbl->Draw(d_ctx, 4, 0);
    }
    
    ////////////////////////////
    //- rjf: present
    //
    HRESULT error = wnd->swapchain->lpVtbl->Present(wnd->swapchain, 1, 0);
    if(FAILED(error))
    {
      char buffer[256] = {0};
      raddbg_snprintf(buffer, sizeof(buffer), "D3D11 present failure (%lx). The process is terminating.", error);
      os_graphical_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
      os_abort(1);
    }
    d_ctx->lpVtbl->ClearState(d_ctx);
  }
  ProfEnd();
}

//- rjf: render pass submission

r_hook void
r_window_submit(OS_Handle window, R_Handle window_equip, R_PassList *passes)
{
  ProfBeginFunction();
  MutexScopeW(r_d3d11_state->device_rw_mutex)
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
          d_ctx->lpVtbl->RSSetViewports(d_ctx, 1, &viewport);
          d_ctx->lpVtbl->RSSetState(d_ctx, (ID3D11RasterizerState *)r_d3d11_state->main_rasterizer);
          
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
              d_ctx->lpVtbl->Map(d_ctx, (ID3D11Resource *)buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub_rsrc);
              U8 *dst_ptr = (U8 *)sub_rsrc.pData;
              U64 off = 0;
              for(R_BatchNode *batch_n = batches->first; batch_n != 0; batch_n = batch_n->next)
              {
                MemoryCopy(dst_ptr+off, batch_n->v.v, batch_n->v.byte_count);
                off += batch_n->v.byte_count;
              }
              d_ctx->lpVtbl->Unmap(d_ctx, (ID3D11Resource *)buffer, 0);
            }
            
            // rjf: get texture
            R_Handle texture_handle = group_params->tex;
            if(r_handle_match(texture_handle, r_handle_zero()))
            {
              texture_handle = r_d3d11_state->backup_texture;
            }
            R_D3D11_Tex2D *texture = r_d3d11_tex2d_from_handle(texture_handle);
            
            // rjf: get texture sample map matrix, based on format
            Mat4x4F32 texture_sample_channel_map = r_sample_channel_map_from_tex2dformat(texture->format);
            
            // rjf: upload uniforms
            R_D3D11_Uniforms_Rect uniforms = {0};
            {
              uniforms.viewport_size             = v2f32(resolution.x, resolution.y);
              uniforms.opacity                   = 1-group_params->transparency;
              uniforms.texture_sample_channel_map = texture_sample_channel_map;
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
              d_ctx->lpVtbl->Map(d_ctx, (ID3D11Resource *)uniforms_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub_rsrc);
              MemoryCopy((U8 *)sub_rsrc.pData, &uniforms, sizeof(uniforms));
              d_ctx->lpVtbl->Unmap(d_ctx, (ID3D11Resource *)uniforms_buffer, 0);
            }
            
            // rjf: setup output merger
            d_ctx->lpVtbl->OMSetRenderTargets(d_ctx, 1, &wnd->stage_color_rtv, 0);
            d_ctx->lpVtbl->OMSetDepthStencilState(d_ctx, r_d3d11_state->noop_depth_stencil, 0);
            d_ctx->lpVtbl->OMSetBlendState(d_ctx, r_d3d11_state->main_blend_state, 0, 0xffffffff);
            
            // rjf: setup input assembly
            U32 stride = batches->bytes_per_inst;
            U32 offset = 0;
            d_ctx->lpVtbl->IASetPrimitiveTopology(d_ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            d_ctx->lpVtbl->IASetInputLayout(d_ctx, ilay);
            d_ctx->lpVtbl->IASetVertexBuffers(d_ctx, 0, 1, &buffer, &stride, &offset);
            
            // rjf: setup shaders
            d_ctx->lpVtbl->VSSetShader(d_ctx, vshad, 0, 0);
            d_ctx->lpVtbl->VSSetConstantBuffers(d_ctx, 0, 1, &uniforms_buffer);
            d_ctx->lpVtbl->PSSetShader(d_ctx, pshad, 0, 0);
            d_ctx->lpVtbl->PSSetConstantBuffers(d_ctx, 0, 1, &uniforms_buffer);
            d_ctx->lpVtbl->PSSetShaderResources(d_ctx, 0, 1, &texture->view);
            d_ctx->lpVtbl->PSSetSamplers(d_ctx, 0, 1, &sampler);
            
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
              d_ctx->lpVtbl->RSSetScissorRects(d_ctx, 1, &rect);
            }
            
            // rjf: draw
            d_ctx->lpVtbl->DrawInstanced(d_ctx, 4, batches->byte_count / batches->bytes_per_inst, 0, 0);
          }
        }break;
        
        ////////////////////////
        //- rjf: blur rendering pass
        //
        case R_PassKind_Blur:
        {
          R_PassParams_Blur *params = pass->params_blur;
          ID3D11SamplerState *sampler   = r_d3d11_state->samplers[R_Tex2DSampleKind_Linear];
          ID3D11VertexShader *vshad     = r_d3d11_state->vshads[R_D3D11_VShadKind_Blur];
          ID3D11PixelShader *pshad      = r_d3d11_state->pshads[R_D3D11_PShadKind_Blur];
          ID3D11Buffer *uniforms_buffer = r_d3d11_state->uniform_type_kind_buffers[R_D3D11_VShadKind_Blur];
          
          // rjf: setup output merger
          d_ctx->lpVtbl->OMSetDepthStencilState(d_ctx, r_d3d11_state->noop_depth_stencil, 0);
          d_ctx->lpVtbl->OMSetBlendState(d_ctx, r_d3d11_state->no_blend_state, 0, 0xffffffff);
          
          // rjf: set up viewport
          Vec2S32 resolution = wnd->last_resolution;
          D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (F32)resolution.x, (F32)resolution.y, 0.0f, 1.0f };
          d_ctx->lpVtbl->RSSetViewports(d_ctx, 1, &viewport);
          d_ctx->lpVtbl->RSSetState(d_ctx, (ID3D11RasterizerState *)r_d3d11_state->main_rasterizer);
          
          // rjf: setup input assembly
          d_ctx->lpVtbl->IASetPrimitiveTopology(d_ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
          d_ctx->lpVtbl->IASetInputLayout(d_ctx, 0);
          
          // rjf: setup shaders
          d_ctx->lpVtbl->VSSetShader(d_ctx, vshad, 0, 0);
          d_ctx->lpVtbl->VSSetConstantBuffers(d_ctx, 0, 1, &uniforms_buffer);
          d_ctx->lpVtbl->PSSetShader(d_ctx, pshad, 0, 0);
          d_ctx->lpVtbl->PSSetSamplers(d_ctx, 0, 1, &sampler);
          
          // rjf: setup scissor rect
          {
            D3D11_RECT rect = { 0 };
            rect.left = 0;
            rect.right = (LONG)wnd->last_resolution.x;
            rect.top = 0;
            rect.bottom = (LONG)wnd->last_resolution.y;
            d_ctx->lpVtbl->RSSetScissorRects(d_ctx, 1, &rect);
          }
          
          // rjf: set up uniforms
          R_D3D11_Uniforms_Blur uniforms = { 0 };
          {
            F32 weights[ArrayCount(uniforms.kernel)*2] = {0};
            
            F32 blur_size = Min(params->blur_size, ArrayCount(weights));
            U64 blur_count = (U64)round_f32(blur_size);
            
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
              for (U64 idx = 1; idx < blur_count; idx += 2)
              {
                F32 w0 = weights[idx + 0];
                F32 w1 = weights[idx + 1];
                F32 w = w0 + w1;
                F32 t = w1 / w;
                
                // each kernel element is float2(weight, offset)
                // weights & offsets are adjusted for bilinear sampling
                // zw elements are not used, a bit of waste but it allows for simpler shader code
                uniforms.kernel[(idx+1)/2] = v4f32(w, (F32)idx + t, 0, 0);
              }
            }
            uniforms.kernel[0].x = weights[0];
            
            // technically we need just direction be different
            // but there are 256 bytes of usable space anyway for each constant buffer chunk
            
            uniforms.passes[Axis2_X].viewport_size = v2f32(resolution.x, resolution.y);
            uniforms.passes[Axis2_X].rect          = params->rect;
            uniforms.passes[Axis2_X].direction     = v2f32(1.f / resolution.x, 0);
            uniforms.passes[Axis2_X].blur_count    = 1 + blur_count / 2; // 2x smaller because of bilinear sampling
            MemoryCopyArray(uniforms.passes[Axis2_X].corner_radii.v, params->corner_radii);
            
            uniforms.passes[Axis2_Y].viewport_size = v2f32(resolution.x, resolution.y);
            uniforms.passes[Axis2_Y].rect          = params->rect;
            uniforms.passes[Axis2_Y].direction     = v2f32(0, 1.f / resolution.y);
            uniforms.passes[Axis2_Y].blur_count    = 1 + blur_count / 2; // 2x smaller because of bilinear sampling
            MemoryCopyArray(uniforms.passes[Axis2_Y].corner_radii.v, params->corner_radii);
            
            D3D11_MAPPED_SUBRESOURCE sub_rsrc = {0};
            d_ctx->lpVtbl->Map(d_ctx, (ID3D11Resource *)uniforms_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub_rsrc);
            MemoryCopy((U8 *)sub_rsrc.pData, &uniforms, sizeof(uniforms));
            d_ctx->lpVtbl->Unmap(d_ctx, (ID3D11Resource *)uniforms_buffer, 0);
          }
          
          ID3D11Buffer *uniforms_buffers[] = { uniforms_buffer, uniforms_buffer };
          
          U32 uniform_offset[Axis2_COUNT][2] =
          {
            { 0 * sizeof(R_D3D11_Uniforms_BlurPass) / 16, (U32)OffsetOf(R_D3D11_Uniforms_Blur, kernel) / 16 },
            { 1 * sizeof(R_D3D11_Uniforms_BlurPass) / 16, (U32)OffsetOf(R_D3D11_Uniforms_Blur, kernel) / 16 },
          };
          
          U32 uniform_count[Axis2_COUNT][2] =
          {
            { sizeof(R_D3D11_Uniforms_BlurPass) / 16, sizeof(uniforms.kernel) / 16 },
            { sizeof(R_D3D11_Uniforms_BlurPass) / 16, sizeof(uniforms.kernel) / 16 },
          };
          
          // rjf: setup scissor rect
          {
            Rng2F32 clip = params->clip;
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
            d_ctx->lpVtbl->RSSetScissorRects(d_ctx, 1, &rect);
          }
          
          // rjf: for unsetting srv
          ID3D11ShaderResourceView* srv = 0;
          
          // horizontal pass
          d_ctx->lpVtbl->OMSetRenderTargets(d_ctx, 1, &wnd->stage_scratch_color_rtv, 0);
          d_ctx->lpVtbl->PSSetConstantBuffers1(d_ctx, 0, ArrayCount(uniforms_buffers), uniforms_buffers, uniform_offset[Axis2_X], uniform_count[Axis2_X]);
          d_ctx->lpVtbl->PSSetShaderResources(d_ctx, 0, 1, &wnd->stage_color_srv);
          d_ctx->lpVtbl->Draw(d_ctx, 4, 0);
          d_ctx->lpVtbl->PSSetShaderResources(d_ctx, 0, 1, &srv);
          
          // vertical pass
          d_ctx->lpVtbl->OMSetRenderTargets(d_ctx, 1, &wnd->stage_color_rtv, 0);
          d_ctx->lpVtbl->PSSetConstantBuffers1(d_ctx, 0, ArrayCount(uniforms_buffers), uniforms_buffers, uniform_offset[Axis2_Y], uniform_count[Axis2_Y]);
          d_ctx->lpVtbl->PSSetShaderResources(d_ctx, 0, 1, &wnd->stage_scratch_color_srv);
          d_ctx->lpVtbl->Draw(d_ctx, 4, 0);
          d_ctx->lpVtbl->PSSetShaderResources(d_ctx, 0, 1, &srv);
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
          d_ctx->lpVtbl->RSSetViewports(d_ctx, 1, &viewport);
          d_ctx->lpVtbl->RSSetState(d_ctx, (ID3D11RasterizerState *)r_d3d11_state->main_rasterizer);
          
          //- rjf: clear render targets
          {
            Vec4F32 bg_color = v4f32(0, 0, 0, 0);
            d_ctx->lpVtbl->ClearRenderTargetView(d_ctx, wnd->geo3d_color_rtv, bg_color.v);
            d_ctx->lpVtbl->ClearDepthStencilView(d_ctx, wnd->geo3d_depth_dsv, D3D11_CLEAR_DEPTH, 1.f, 0);
          }
          
          //- rjf: draw mesh batches
          {
            // rjf: grab pipeline info
            ID3D11VertexShader *vshad     = r_d3d11_state->vshads[R_D3D11_VShadKind_Mesh];
            ID3D11InputLayout *ilay       = r_d3d11_state->ilays[R_D3D11_VShadKind_Mesh];
            ID3D11PixelShader *pshad      = r_d3d11_state->pshads[R_D3D11_PShadKind_Mesh];
            ID3D11Buffer *uniforms_buffer = r_d3d11_state->uniform_type_kind_buffers[R_D3D11_VShadKind_Mesh];
            
            // rjf: setup output merger
            d_ctx->lpVtbl->OMSetRenderTargets(d_ctx, 1, &wnd->geo3d_color_rtv, wnd->geo3d_depth_dsv);
            d_ctx->lpVtbl->OMSetDepthStencilState(d_ctx, r_d3d11_state->plain_depth_stencil, 0);
            d_ctx->lpVtbl->OMSetBlendState(d_ctx, r_d3d11_state->main_blend_state, 0, 0xffffffff);
            
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
                d_ctx->lpVtbl->IASetPrimitiveTopology(d_ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                d_ctx->lpVtbl->IASetInputLayout(d_ctx, ilay);
                d_ctx->lpVtbl->IASetVertexBuffers(d_ctx, 0, 1, &mesh_vertices->buffer, &stride, &offset);
                d_ctx->lpVtbl->IASetIndexBuffer(d_ctx, mesh_indices->buffer, DXGI_FORMAT_R32_UINT, 0);
                
                // rjf: setup uniforms buffer
                R_D3D11_Uniforms_Mesh uniforms = {0};
                {
                  uniforms.xform = mul_4x4f32(params->projection, params->view);
                }
                {
                  D3D11_MAPPED_SUBRESOURCE sub_rsrc = {0};
                  d_ctx->lpVtbl->Map(d_ctx, (ID3D11Resource *)uniforms_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub_rsrc);
                  MemoryCopy((U8 *)sub_rsrc.pData, &uniforms, sizeof(uniforms));
                  d_ctx->lpVtbl->Unmap(d_ctx, (ID3D11Resource *)uniforms_buffer, 0);
                }
                
                
                // rjf: setup shaders
                d_ctx->lpVtbl->VSSetShader(d_ctx, vshad, 0, 0);
                d_ctx->lpVtbl->VSSetConstantBuffers(d_ctx, 0, 1, &uniforms_buffer);
                d_ctx->lpVtbl->PSSetShader(d_ctx, pshad, 0, 0);
                d_ctx->lpVtbl->PSSetConstantBuffers(d_ctx, 0, 1, &uniforms_buffer);
                
                // rjf: setup scissor rect
                {
                  D3D11_RECT rect = {0};
                  {
                    rect.left = 0;
                    rect.right = (LONG)wnd->last_resolution.x;
                    rect.top = 0;
                    rect.bottom = (LONG)wnd->last_resolution.y;
                  }
                  d_ctx->lpVtbl->RSSetScissorRects(d_ctx, 1, &rect);
                }
                
                // rjf: draw
                d_ctx->lpVtbl->DrawIndexed(d_ctx, mesh_indices->size/sizeof(U32), 0, 0);
              }
            }
          }
          
          //- rjf: composite to main staging buffer
          {
            ID3D11SamplerState *sampler   = r_d3d11_state->samplers[R_Tex2DSampleKind_Nearest];
            ID3D11VertexShader *vshad     = r_d3d11_state->vshads[R_D3D11_VShadKind_Geo3DComposite];
            ID3D11PixelShader *pshad      = r_d3d11_state->pshads[R_D3D11_PShadKind_Geo3DComposite];
            
            // rjf: setup output merger
            d_ctx->lpVtbl->OMSetRenderTargets(d_ctx, 1, &wnd->stage_color_rtv, 0);
            d_ctx->lpVtbl->OMSetDepthStencilState(d_ctx, r_d3d11_state->noop_depth_stencil, 0);
            d_ctx->lpVtbl->OMSetBlendState(d_ctx, r_d3d11_state->main_blend_state, 0, 0xffffffff);
            
            // rjf: set up rasterizer
            Vec2S32 resolution = wnd->last_resolution;
            D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (F32)resolution.x, (F32)resolution.y, 0.0f, 1.0f };
            d_ctx->lpVtbl->RSSetViewports(d_ctx, 1, &viewport);
            d_ctx->lpVtbl->RSSetState(d_ctx, (ID3D11RasterizerState *)r_d3d11_state->main_rasterizer);
            
            // rjf: setup input assembly
            d_ctx->lpVtbl->IASetPrimitiveTopology(d_ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            d_ctx->lpVtbl->IASetInputLayout(d_ctx, 0);
            
            // rjf: setup shaders
            d_ctx->lpVtbl->VSSetShader(d_ctx, vshad, 0, 0);
            d_ctx->lpVtbl->PSSetShader(d_ctx, pshad, 0, 0);
            d_ctx->lpVtbl->PSSetShaderResources(d_ctx, 0, 1, &wnd->geo3d_color_srv);
            d_ctx->lpVtbl->PSSetSamplers(d_ctx, 0, 1, &sampler);
            
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
              d_ctx->lpVtbl->RSSetScissorRects(d_ctx, 1, &rect);
            }
            
            // rjf: draw
            d_ctx->lpVtbl->Draw(d_ctx, 4, 0);
          }
        }break;
      }
    }
  }
  ProfEnd();
}
