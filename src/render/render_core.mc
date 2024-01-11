// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Tables

@table(name, display_string, bytes_per_pixel)
R_Tex2DFormatTable:
{
  {R8       "R8"       1}
  {RGBA8    "RGBA8"    4}
  {BGRA8    "BGRA8"    4}
}

@table(name, display_string)
R_Tex2DKindTable:
{
  {Static   "Static" }
  {Dynamic  "Dynamic"}
}

@table(name, display_string)
R_Tex2DSampleKindTable:
{
  {Nearest   "Nearest" }
  {Linear    "Linear"  }
}

@table(name, display_string)
R_GeoTopologyKindTable:
{
  {Lines          "Lines"          }
  {LineStrip      "Line Strip"     }
  {Triangles      "Triangles"      }
  {TriangleStrip  "Triangle Strip" }
}

@table(name, display_string)
R_BufferKindTable:
{
  {Static   "Static" }
  {Dynamic  "Dynamic"}
}

@table(name, batch, display_string)
R_PassKindTable:
{
  {UI          1      "UI"    }
  {Blur        0      "Blur"  }
  {Geo3D       1      "Geo3D" }
}

////////////////////////////////
//~ rjf: Generators

@table_gen_enum R_Tex2DFormat:
{
  @expand(R_Tex2DFormatTable a) `R_Tex2DFormat_$(a.name),`;
  `R_Tex2DFormat_COUNT`;
}

@table_gen_enum R_Tex2DKind:
{
  @expand(R_Tex2DKindTable a) `R_Tex2DKind_$(a.name),`;
  `R_Tex2DKind_COUNT`;
}

@table_gen_enum R_Tex2DSampleKind:
{
  @expand(R_Tex2DSampleKindTable a) `R_Tex2DSampleKind_$(a.name),`;
  `R_Tex2DSampleKind_COUNT`;
}

@table_gen_enum R_GeoTopologyKind:
{
  @expand(R_GeoTopologyKindTable a) `R_GeoTopologyKind_$(a.name),`;
  `R_GeoTopologyKind_COUNT`;
}

@table_gen_enum R_BufferKind:
{
  @expand(R_BufferKindTable a) `R_BufferKind_$(a.name),`;
  `R_BufferKind_COUNT`;
}

@table_gen_enum R_PassKind:
{
  @expand(R_PassKindTable a) `R_PassKind_$(a.name),`;
  `R_PassKind_COUNT`;
}

@table_gen_data(type:String8) r_tex2d_format_display_string_table:
{
  @expand(R_Tex2DFormatTable a) `str8_lit_comp("$(a.display_string)"),`;
}

@table_gen_data(type:U8) r_tex2d_format_bytes_per_pixel_table:
{
  @expand(R_Tex2DFormatTable a) `$(a.bytes_per_pixel),`;
}

@table_gen_data(type:String8) r_tex2d_kind_display_string_table:
{
  @expand(R_Tex2DKindTable a) `str8_lit_comp("$(a.display_string)"),`;
}

@table_gen_data(type:String8) r_tex2d_sample_kind_display_string_table:
{
  @expand(R_Tex2DSampleKindTable a) `str8_lit_comp("$(a.display_string)"),`;
}

@table_gen_data(type:String8) r_pass_kind_display_string_table:
{
  @expand(R_PassKindTable a) `str8_lit_comp("$(a.display_string)"),`;
}

@table_gen_data(type:U8) r_pass_kind_batch_table:
{
  @expand(R_PassKindTable a) `$(a.batch),`;
}

@table_gen_data(type:U64) @c_file r_pass_kind_params_size_table:
{
  @expand(R_PassKindTable a) `sizeof(R_PassParams_$(a.name)),`;
}
