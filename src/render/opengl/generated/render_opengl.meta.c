// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

C_LINKAGE_BEGIN
String8 r_ogl_shader_kind_name_table[2] =
{
str8_lit_comp("rect"),
str8_lit_comp("blur"),
};

String8 * r_ogl_shader_kind_vshad_src_table[2] =
{
&r_ogl_rect_vshad_src,
&r_ogl_blur_vshad_src,
};

String8 * r_ogl_shader_kind_pshad_src_table[2] =
{
&r_ogl_rect_pshad_src,
&r_ogl_blur_pshad_src,
};

R_OGL_AttributeArray r_ogl_shader_kind_input_attributes_table[2] =
{
{ r_ogl_rect_input_attributes, ArrayCount(r_ogl_rect_input_attributes) },
{ 0,  },
};

R_OGL_AttributeArray r_ogl_shader_kind_output_attributes_table[2] =
{
{ r_ogl_single_color_output_attributes, ArrayCount(r_ogl_single_color_output_attributes) },
{ r_ogl_single_color_output_attributes, ArrayCount(r_ogl_single_color_output_attributes) },
};

C_LINKAGE_END

