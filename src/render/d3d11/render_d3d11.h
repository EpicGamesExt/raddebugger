// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_D3D11_H
#define RENDER_D3D11_H

#include <combaseapi.h>
#include <dcommon.h>
#include <initguid.h>
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

////////////////////////////////
//~ rjf: Generated Code

#include "generated/render_d3d11.meta.h"

////////////////////////////////
//~ rjf: C-side Shader Types

typedef struct R_D3D11_Uniforms_Rect R_D3D11_Uniforms_Rect;
struct R_D3D11_Uniforms_Rect
{
  Vec2F32 viewport_size;
  F32 opacity;
  F32 _padding0_;
  Mat4x4F32 texture_sample_channel_map;
  Vec2F32 texture_t2d_size;
  Vec2F32 translate;
  Vec4F32 xform[3];
  Vec2F32 xform_scale;
};

typedef struct R_D3D11_Uniforms_BlurPass R_D3D11_Uniforms_BlurPass;
struct R_D3D11_Uniforms_BlurPass
{
  Rng2F32 rect;
  Vec4F32 corner_radii;
  Vec2F32 direction;
  Vec2F32 viewport_size;
  U32 blur_count;
  U8 _padding0_[204];
};
StaticAssert(sizeof(R_D3D11_Uniforms_BlurPass) % 256 == 0, NotAligned); // constant count/offset must be aligned to 256 bytes

typedef struct R_D3D11_Uniforms_Blur R_D3D11_Uniforms_Blur;
struct R_D3D11_Uniforms_Blur
{
  R_D3D11_Uniforms_BlurPass passes[Axis2_COUNT];
  Vec4F32 kernel[32];
};

typedef struct R_D3D11_Uniforms_Mesh R_D3D11_Uniforms_Mesh;
struct R_D3D11_Uniforms_Mesh
{
  Mat4x4F32 xform;
};

////////////////////////////////
//~ rjf: Main State Types

typedef struct R_D3D11_Tex2D R_D3D11_Tex2D;
struct R_D3D11_Tex2D
{
  R_D3D11_Tex2D *next;
  U64 generation;
  ID3D11Texture2D *texture;
  ID3D11ShaderResourceView *view;
  R_ResourceKind kind;
  Vec2S32 size;
  R_Tex2DFormat format;
};

typedef struct R_D3D11_Buffer R_D3D11_Buffer;
struct R_D3D11_Buffer
{
  R_D3D11_Buffer *next;
  U64 generation;
  ID3D11Buffer *buffer;
  R_ResourceKind kind;
  U64 size;
};

typedef struct R_D3D11_Window R_D3D11_Window;
struct R_D3D11_Window
{
  R_D3D11_Window *next;
  U64 generation;
  
  // rjf: swapchain/framebuffer
  IDXGISwapChain1        *swapchain;
  ID3D11Texture2D        *framebuffer;
  ID3D11RenderTargetView *framebuffer_rtv;
  
  // rjf: staging buffer
  ID3D11Texture2D *stage_color;
  ID3D11RenderTargetView *stage_color_rtv;
  ID3D11ShaderResourceView *stage_color_srv;
  ID3D11Texture2D *stage_scratch_color;
  ID3D11RenderTargetView *stage_scratch_color_rtv;
  ID3D11ShaderResourceView *stage_scratch_color_srv;
  
  // rjf: geo3d buffer
  ID3D11Texture2D *geo3d_color;
  ID3D11RenderTargetView *geo3d_color_rtv;
  ID3D11ShaderResourceView *geo3d_color_srv;
  ID3D11Texture2D *geo3d_depth;
  ID3D11DepthStencilView *geo3d_depth_dsv;
  ID3D11ShaderResourceView *geo3d_depth_srv;
  
  // rjf: last state
  Vec2S32 last_resolution;
};

typedef struct R_D3D11_FlushBuffer R_D3D11_FlushBuffer;
struct R_D3D11_FlushBuffer
{
  R_D3D11_FlushBuffer *next;
  ID3D11Buffer *buffer;
};

typedef struct R_D3D11_State R_D3D11_State;
struct R_D3D11_State
{
  // rjf: state
  Arena *arena;
  U64 window_count;
  R_D3D11_Window *first_free_window;
  R_D3D11_Tex2D *first_free_tex2d;
  R_D3D11_Buffer *first_free_buffer;
  R_D3D11_Tex2D *first_to_free_tex2d;
  R_D3D11_Buffer *first_to_free_buffer;
  RWMutex device_rw_mutex;
  
  // rjf: base d3d11 objects
  ID3D11Device            *base_device;
  ID3D11DeviceContext     *base_device_ctx;
  ID3D11Device1           *device;
  ID3D11DeviceContext1    *device_ctx;
  IDXGIDevice1            *dxgi_device;
  IDXGIAdapter            *dxgi_adapter;
  IDXGIFactory2           *dxgi_factory;
  ID3D11RasterizerState1  *main_rasterizer;
  ID3D11BlendState        *main_blend_state;
  ID3D11BlendState        *no_blend_state;
  ID3D11SamplerState      *samplers[R_Tex2DSampleKind_COUNT];
  ID3D11DepthStencilState *noop_depth_stencil;
  ID3D11DepthStencilState *plain_depth_stencil;
  ID3D11Buffer            *instance_scratch_buffer_64kb;
  
  // rjf: backups
  R_Handle backup_texture;
  
  // rjf: vertex shaders
  ID3D11VertexShader *vshads[R_D3D11_VShadKind_COUNT];
  ID3D11InputLayout *ilays[R_D3D11_VShadKind_COUNT];
  ID3D11PixelShader *pshads[R_D3D11_PShadKind_COUNT];
  ID3D11Buffer *uniform_type_kind_buffers[R_D3D11_UniformTypeKind_COUNT];
  
  // rjf: buffers to flush at subsequent frame
  Arena *buffer_flush_arena;
  R_D3D11_FlushBuffer *first_buffer_to_flush;
  R_D3D11_FlushBuffer *last_buffer_to_flush;
};

////////////////////////////////
//~ rjf: Globals

global R_D3D11_State *r_d3d11_state = 0;
global read_only R_D3D11_Window r_d3d11_window_nil = {&r_d3d11_window_nil};
global read_only R_D3D11_Tex2D r_d3d11_tex2d_nil = {&r_d3d11_tex2d_nil};
global read_only R_D3D11_Buffer r_d3d11_buffer_nil = {&r_d3d11_buffer_nil};

////////////////////////////////
//~ rjf: Helpers

internal R_D3D11_Window *r_d3d11_window_from_handle(R_Handle handle);
internal R_Handle r_d3d11_handle_from_window(R_D3D11_Window *window);
internal R_D3D11_Tex2D *r_d3d11_tex2d_from_handle(R_Handle handle);
internal R_Handle r_d3d11_handle_from_tex2d(R_D3D11_Tex2D *texture);
internal R_D3D11_Buffer *r_d3d11_buffer_from_handle(R_Handle handle);
internal R_Handle r_d3d11_handle_from_buffer(R_D3D11_Buffer *buffer);
internal ID3D11Buffer *r_d3d11_instance_buffer_from_size(U64 size);
internal void r_usage_access_flags_from_resource_kind(R_ResourceKind kind, D3D11_USAGE *out_d3d11_usage, UINT *out_cpu_access_flags);

#endif // RENDER_D3D11_H
