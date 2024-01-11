// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

internal U64 regs_block_size_from_architecture(Architecture arch)
{
U64 result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:{result = sizeof(REGS_RegBlockX64);}break;
case Architecture_x86:{result = sizeof(REGS_RegBlockX86);}break;
}
return result;
}
internal U64 regs_reg_code_count_from_architecture(Architecture arch)
{
U64 result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:{result = REGS_RegCodeX64_COUNT;}break;
case Architecture_x86:{result = REGS_RegCodeX86_COUNT;}break;
}
return result;
}
internal U64 regs_alias_code_count_from_architecture(Architecture arch)
{
U64 result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:{result = REGS_AliasCodeX64_COUNT;}break;
case Architecture_x86:{result = REGS_AliasCodeX86_COUNT;}break;
}
return result;
}
internal String8 *regs_reg_code_string_table_from_architecture(Architecture arch)
{
String8 *result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:{result = regs_g_reg_code_x64_string_table;}break;
case Architecture_x86:{result = regs_g_reg_code_x86_string_table;}break;
}
return result;
}
internal String8 *regs_alias_code_string_table_from_architecture(Architecture arch)
{
String8 *result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:{result = regs_g_alias_code_x64_string_table;}break;
case Architecture_x86:{result = regs_g_alias_code_x86_string_table;}break;
}
return result;
}
internal REGS_Rng *regs_reg_code_rng_table_from_architecture(Architecture arch)
{
REGS_Rng *result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:{result = regs_g_reg_code_x64_rng_table;}break;
case Architecture_x86:{result = regs_g_reg_code_x86_rng_table;}break;
}
return result;
}
internal REGS_Slice *regs_alias_code_slice_table_from_architecture(Architecture arch)
{
REGS_Slice *result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:{result = regs_g_alias_code_x64_slice_table;}break;
case Architecture_x86:{result = regs_g_alias_code_x86_slice_table;}break;
}
return result;
}
internal REGS_UsageKind *regs_reg_code_usage_kind_table_from_architecture(Architecture arch)
{
REGS_UsageKind *result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:{result = regs_g_reg_code_x64_usage_kind_table;}break;
case Architecture_x86:{result = regs_g_reg_code_x86_usage_kind_table;}break;
}
return result;
}
internal REGS_UsageKind *regs_alias_code_usage_kind_table_from_architecture(Architecture arch)
{
REGS_UsageKind *result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:{result = regs_g_alias_code_x64_usage_kind_table;}break;
case Architecture_x86:{result = regs_g_alias_code_x86_usage_kind_table;}break;
}
return result;
}
