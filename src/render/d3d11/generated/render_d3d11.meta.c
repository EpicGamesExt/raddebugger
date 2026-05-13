// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

C_LINKAGE_BEGIN
String8* r_d3d11_g_vshad_kind_source_table[5] =
{
&r_d3d11_g_rect_shader_src,
&r_d3d11_g_blur_shader_src,
&r_d3d11_g_mesh_shader_src,
&r_d3d11_g_geo3dcomposite_shader_src,
&r_d3d11_g_finalize_shader_src,
};

String8 r_d3d11_g_vshad_kind_source_name_table[5] =
{
str8_lit_comp("r_d3d11_g_rect_shader_src"),
str8_lit_comp("r_d3d11_g_blur_shader_src"),
str8_lit_comp("r_d3d11_g_mesh_shader_src"),
str8_lit_comp("r_d3d11_g_geo3dcomposite_shader_src"),
str8_lit_comp("r_d3d11_g_finalize_shader_src"),
};

D3D11_INPUT_ELEMENT_DESC * r_d3d11_g_vshad_kind_elements_ptr_table[5] =
{
r_d3d11_g_rect_ilay_elements,
0,
r_d3d11_g_mesh_ilay_elements,
0,
0,
};

U64 r_d3d11_g_vshad_kind_elements_count_table[5] =
{
ArrayCount(r_d3d11_g_rect_ilay_elements) ,
 0,
ArrayCount(r_d3d11_g_mesh_ilay_elements) ,
 0,
 0,
};

String8* r_d3d11_g_pshad_kind_source_table[5] =
{
&r_d3d11_g_rect_shader_src,
&r_d3d11_g_blur_shader_src,
&r_d3d11_g_mesh_shader_src,
&r_d3d11_g_geo3dcomposite_shader_src,
&r_d3d11_g_finalize_shader_src,
};

String8 r_d3d11_g_pshad_kind_source_name_table[5] =
{
str8_lit_comp("r_d3d11_g_rect_shader_src"),
str8_lit_comp("r_d3d11_g_blur_shader_src"),
str8_lit_comp("r_d3d11_g_mesh_shader_src"),
str8_lit_comp("r_d3d11_g_geo3dcomposite_shader_src"),
str8_lit_comp("r_d3d11_g_finalize_shader_src"),
};

U64 r_d3d11_g_uniform_type_kind_size_table[3] =
{
sizeof(R_D3D11_Uniforms_Rect),
sizeof(R_D3D11_Uniforms_Blur),
sizeof(R_D3D11_Uniforms_Mesh),
};

C_LINKAGE_END

