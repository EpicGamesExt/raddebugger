// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef ARM64_META_H
#define ARM64_META_H

typedef enum ARM64_RegCode
{
ARM64_RegCode_nil,
ARM64_RegCode_x0,
ARM64_RegCode_x1,
ARM64_RegCode_x2,
ARM64_RegCode_x3,
ARM64_RegCode_x4,
ARM64_RegCode_x5,
ARM64_RegCode_x6,
ARM64_RegCode_x7,
ARM64_RegCode_x8,
ARM64_RegCode_x9,
ARM64_RegCode_x10,
ARM64_RegCode_x11,
ARM64_RegCode_x12,
ARM64_RegCode_x13,
ARM64_RegCode_x14,
ARM64_RegCode_x15,
ARM64_RegCode_x16,
ARM64_RegCode_x17,
ARM64_RegCode_x18,
ARM64_RegCode_x19,
ARM64_RegCode_x20,
ARM64_RegCode_x21,
ARM64_RegCode_x22,
ARM64_RegCode_x23,
ARM64_RegCode_x24,
ARM64_RegCode_x25,
ARM64_RegCode_x26,
ARM64_RegCode_x27,
ARM64_RegCode_x28,
ARM64_RegCode_fp,
ARM64_RegCode_lr,
ARM64_RegCode_sp,
ARM64_RegCode_pc,
ARM64_RegCode_v0,
ARM64_RegCode_v1,
ARM64_RegCode_v2,
ARM64_RegCode_v3,
ARM64_RegCode_v4,
ARM64_RegCode_v5,
ARM64_RegCode_v6,
ARM64_RegCode_v7,
ARM64_RegCode_v8,
ARM64_RegCode_v9,
ARM64_RegCode_v10,
ARM64_RegCode_v11,
ARM64_RegCode_v12,
ARM64_RegCode_v13,
ARM64_RegCode_v14,
ARM64_RegCode_v15,
ARM64_RegCode_v16,
ARM64_RegCode_v17,
ARM64_RegCode_v18,
ARM64_RegCode_v19,
ARM64_RegCode_v21,
ARM64_RegCode_v22,
ARM64_RegCode_v23,
ARM64_RegCode_v24,
ARM64_RegCode_v25,
ARM64_RegCode_v26,
ARM64_RegCode_v27,
ARM64_RegCode_v28,
ARM64_RegCode_v29,
ARM64_RegCode_v30,
ARM64_RegCode_v31,
ARM64_RegCode_elr_mode,
ARM64_RegCode_ra_sign_state,
ARM64_RegCode_tpidrro_elo,
ARM64_RegCode_tpidr_elo,
ARM64_RegCode_tpidr_el1,
ARM64_RegCode_tpidr_el2,
ARM64_RegCode_tpidr_el3,
ARM64_RegCode_x29,
ARM64_RegCode_x30,
ARM64_RegCode_x31,
ARM64_RegCode_COUNT,
} ARM64_RegCode;

typedef struct ARM64_RegBlock ARM64_RegBlock;
struct ARM64_RegBlock
{
U64 x0;
U64 x1;
U64 x2;
U64 x3;
U64 x4;
U64 x5;
U64 x6;
U64 x7;
U64 x8;
U64 x9;
U64 x10;
U64 x11;
U64 x12;
U64 x13;
U64 x14;
U64 x15;
U64 x16;
U64 x17;
U64 x18;
U64 x19;
U64 x20;
U64 x21;
U64 x22;
U64 x23;
U64 x24;
U64 x25;
U64 x26;
U64 x27;
U64 x28;
U64 fp;
U64 lr;
U64 sp;
U64 pc;
U128 v0;
U128 v1;
U128 v2;
U128 v3;
U128 v4;
U128 v5;
U128 v6;
U128 v7;
U128 v8;
U128 v9;
U128 v10;
U128 v11;
U128 v12;
U128 v13;
U128 v14;
U128 v15;
U128 v16;
U128 v17;
U128 v18;
U128 v19;
U128 v21;
U128 v22;
U128 v23;
U128 v24;
U128 v25;
U128 v26;
U128 v27;
U128 v28;
U128 v29;
U128 v30;
U128 v31;
U64 elr_mode;
U64 ra_sign_state;
U64 tpidrro_elo;
U64 tpidr_elo;
U64 tpidr_el1;
U64 tpidr_el2;
U64 tpidr_el3;
};

C_LINKAGE_BEGIN
extern B8 arm64_reg_code_is_vector_table[75];
extern String8 arm64_reg_code_name_table[75];
extern U8 arm64_reg_code_base_table[75];
extern Rng1U16 arm64_reg_code_rng_table[75];

C_LINKAGE_END

#endif // ARM64_META_H
