// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_CORE_H
#define RENDER_CORE_H

#define r_hook C_LINKAGE

////////////////////////////////
//~ rjf: Generated Code

#include "generated/render.meta.h"

////////////////////////////////
//~ rjf: Enums

typedef U8 R_ChannelCode; // 3 bits
typedef enum R_ChannelCodeEnum
{
  R_ChannelCode_Null,
  R_ChannelCode_R,
  R_ChannelCode_G,
  R_ChannelCode_B,
  R_ChannelCode_A,
}
R_ChannelCodeEnum;

typedef U8 R_ChannelSizeKind; // 3 bits
typedef enum R_ChannelSizeKindEnum
{
  R_ChannelSizeKind_Null,
  R_ChannelSizeKind_2,
  R_ChannelSizeKind_8,
  R_ChannelSizeKind_10,
  R_ChannelSizeKind_11,
  R_ChannelSizeKind_16,
  R_ChannelSizeKind_24,
  R_ChannelSizeKind_32,
}
R_ChannelSizeKindEnum;

typedef U8 R_ChannelTypeKind; // 3 bits
typedef enum R_ChannelTypeKindEnum
{
  R_ChannelTypeKind_Null,
  R_ChannelTypeKind_UInt,
  R_ChannelTypeKind_SInt,
  R_ChannelTypeKind_UNorm,
  R_ChannelTypeKind_SNorm,
  R_ChannelTypeKind_Float,
}
R_ChannelTypeKindEnum;

typedef U64 R_Tex2DFmt;
//
// set of channels, each channel including {code, size, type kind}, 3 bits each:
//   [0, 3) -> channel code
//   [3, 6) -> channel size
//   [6, 9) -> channel type kind
//
// 9 bits per channel, * number of channels, e.g. 4 channels -> 36 bits

#define R_Channel(channel_idx, code_name, size_kind_name, type_kind_name) ((((U64)(R_ChannelCode_##code_name & 0x7)) | ((U64)(R_ChannelSizeKind_##size_kind_name & 0x7) << 3) | ((U64)(R_ChannelTypeKind_##type_kind_name & 0x7) << 6)) << ((channel_idx)*9))
#define r_code_from_tex2dfmt_channel(fmt, channel_idx) ((R_ChannelCode)(((fmt) & (0x7<<((channel_idx)*9))) >> ((channel_idx)*9)))
#define r_size_kind_from_tex2dfmt_channel(fmt, channel_idx) ((R_ChannelSizeKind)(((fmt) & (0x38<<((channel_idx)*9))) >> ((channel_idx)*9 + 3)))
#define r_type_kind_from_tex2dfmt_channel(fmt, channel_idx) ((R_ChannelTypeKind)(((fmt) & (0x1c0<<((channel_idx)*9))) >> ((channel_idx)*9 + 6)))

#define R_Tex2DFmt_R8    (R_Channel(0, R, 8, UInt))
#define R_Tex2DFmt_RG8   (R_Channel(0, R, 8, UInt) | R_Channel(1, G, 8, UInt))
#define R_Tex2DFmt_RGB8  (R_Channel(0, R, 8, UInt) | R_Channel(1, G, 8, UInt) | R_Channel(2, B, 8, UInt))
#define R_Tex2DFmt_RGBA8 (R_Channel(0, R, 8, UInt) | R_Channel(1, G, 8, UInt) | R_Channel(2, B, 8, UInt) | R_Channel(3, A, 8, UInt))

typedef U32 R_GeoVertexFlags;
enum
{
  R_GeoVertexFlag_TexCoord = (1<<0),
  R_GeoVertexFlag_Normals  = (1<<1),
  R_GeoVertexFlag_RGB      = (1<<2),
  R_GeoVertexFlag_RGBA     = (1<<3),
};

////////////////////////////////
//~ rjf: Handle Type

typedef union R_Handle R_Handle;
union R_Handle
{
  U64 u64[1];
  U32 u32[2];
  U16 u16[4];
};

////////////////////////////////
//~ rjf: Instance Types

typedef struct R_Rect2DInst R_Rect2DInst;
struct R_Rect2DInst
{
  Rng2F32 dst;
  Rng2F32 src;
  Vec4F32 colors[Corner_COUNT];
  F32 corner_radii[Corner_COUNT];
  F32 border_thickness;
  F32 edge_softness;
  F32 white_texture_override;
  F32 shear;
};

typedef struct R_Mesh3DInst R_Mesh3DInst;
struct R_Mesh3DInst
{
  Mat4x4F32 xform;
};

////////////////////////////////
//~ rjf: Batch Types

typedef struct R_Batch R_Batch;
struct R_Batch
{
  U8 *v;
  U64 byte_count;
  U64 byte_cap;
};

typedef struct R_BatchNode R_BatchNode;
struct R_BatchNode
{
  R_BatchNode *next;
  R_Batch v;
};

typedef struct R_BatchList R_BatchList;
struct R_BatchList
{
  R_BatchNode *first;
  R_BatchNode *last;
  U64 batch_count;
  U64 byte_count;
  U64 bytes_per_inst;
};

typedef struct R_BatchGroup2DParams R_BatchGroup2DParams;
struct R_BatchGroup2DParams
{
  R_Handle tex;
  R_Tex2DSampleKind tex_sample_kind;
  Mat3x3F32 xform;
  Rng2F32 clip;
  F32 transparency;
};

typedef struct R_BatchGroup2DNode R_BatchGroup2DNode;
struct R_BatchGroup2DNode
{
  R_BatchGroup2DNode *next;
  R_BatchList batches;
  R_BatchGroup2DParams params;
};

typedef struct R_BatchGroup2DList R_BatchGroup2DList;
struct R_BatchGroup2DList
{
  R_BatchGroup2DNode *first;
  R_BatchGroup2DNode *last;
  U64 count;
};

typedef struct R_BatchGroup3DParams R_BatchGroup3DParams;
struct R_BatchGroup3DParams
{
  R_Handle mesh_vertices;
  R_Handle mesh_indices;
  R_GeoTopologyKind mesh_geo_topology;
  R_GeoVertexFlags mesh_geo_vertex_flags;
  R_Handle albedo_tex;
  R_Tex2DSampleKind albedo_tex_sample_kind;
  Mat4x4F32 xform;
};

typedef struct R_BatchGroup3DMapNode R_BatchGroup3DMapNode;
struct R_BatchGroup3DMapNode
{
  R_BatchGroup3DMapNode *next;
  U64 hash;
  R_BatchList batches;
  R_BatchGroup3DParams params;
};

typedef struct R_BatchGroup3DMap R_BatchGroup3DMap;
struct R_BatchGroup3DMap
{
  R_BatchGroup3DMapNode **slots;
  U64 slots_count;
};

////////////////////////////////
//~ rjf: Pass Types

typedef struct R_PassParams_UI R_PassParams_UI;
struct R_PassParams_UI
{
  R_BatchGroup2DList rects;
};

typedef struct R_PassParams_Blur R_PassParams_Blur;
struct R_PassParams_Blur
{
  Rng2F32 rect;
  Rng2F32 clip;
  F32 blur_size;
  F32 corner_radii[Corner_COUNT];
};

typedef struct R_PassParams_Geo3D R_PassParams_Geo3D;
struct R_PassParams_Geo3D
{
  Rng2F32 viewport;
  Rng2F32 clip;
  Mat4x4F32 view;
  Mat4x4F32 projection;
  R_BatchGroup3DMap mesh_batches;
};

typedef struct R_Pass R_Pass;
struct R_Pass
{
  R_PassKind kind;
  union
  {
    void *params;
    R_PassParams_UI *params_ui;
    R_PassParams_Blur *params_blur;
    R_PassParams_Geo3D *params_geo3d;
  };
};

typedef struct R_PassNode R_PassNode;
struct R_PassNode
{
  R_PassNode *next;
  R_Pass v;
};

typedef struct R_PassList R_PassList;
struct R_PassList
{
  R_PassNode *first;
  R_PassNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Helpers

internal U64 r_bytes_per_pixel_from_tex2dfmt(R_Tex2DFmt fmt);
internal Mat4x4F32 r_sample_channel_map_from_tex2dfmt(R_Tex2DFmt fmt);
internal Mat4x4F32 r_sample_channel_map_from_tex2dformat(R_Tex2DFormat fmt);

////////////////////////////////
//~ rjf: Handle Type Functions

internal R_Handle r_handle_zero(void);
internal B32 r_handle_match(R_Handle a, R_Handle b);

////////////////////////////////
//~ rjf: Batch Type Functions

internal R_BatchList r_batch_list_make(U64 instance_size);
internal void *r_batch_list_push_inst(Arena *arena, R_BatchList *list, U64 batch_inst_cap);

////////////////////////////////
//~ rjf: Pass Type Functions

internal R_Pass *r_pass_from_kind(Arena *arena, R_PassList *list, R_PassKind kind);

////////////////////////////////
//~ rjf: Backend Hooks

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

#endif // RENDER_CORE_H
