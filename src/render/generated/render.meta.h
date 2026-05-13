// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef RENDER_META_H
#define RENDER_META_H

typedef enum R_Tex2DFormat
{
R_Tex2DFormat_R8,
R_Tex2DFormat_RG8,
R_Tex2DFormat_RGBA8,
R_Tex2DFormat_BGRA8,
R_Tex2DFormat_R16,
R_Tex2DFormat_RGBA16,
R_Tex2DFormat_R32,
R_Tex2DFormat_RG32,
R_Tex2DFormat_RGBA32,
R_Tex2DFormat_COUNT,
} R_Tex2DFormat;

typedef enum R_ResourceKind
{
R_ResourceKind_Static,
R_ResourceKind_Dynamic,
R_ResourceKind_Stream,
R_ResourceKind_COUNT,
} R_ResourceKind;

typedef enum R_Tex2DSampleKind
{
R_Tex2DSampleKind_Nearest,
R_Tex2DSampleKind_Linear,
R_Tex2DSampleKind_COUNT,
} R_Tex2DSampleKind;

typedef enum R_GeoTopologyKind
{
R_GeoTopologyKind_Lines,
R_GeoTopologyKind_LineStrip,
R_GeoTopologyKind_Triangles,
R_GeoTopologyKind_TriangleStrip,
R_GeoTopologyKind_COUNT,
} R_GeoTopologyKind;

typedef enum R_PassKind
{
R_PassKind_UI,
R_PassKind_Blur,
R_PassKind_Geo3D,
R_PassKind_COUNT,
} R_PassKind;

C_LINKAGE_BEGIN
extern String8 r_tex2d_format_display_string_table[9];
extern U8 r_tex2d_format_bytes_per_pixel_table[9];
extern String8 r_resource_kind_display_string_table[3];
extern String8 r_tex2d_sample_kind_display_string_table[2];
extern String8 r_pass_kind_display_string_table[3];
extern U8 r_pass_kind_batch_table[3];

C_LINKAGE_END

#endif // RENDER_META_H
