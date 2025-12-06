// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef REGS_META_H
#define REGS_META_H

typedef enum REGS_RegCodeX64
{
REGS_RegCodeX64_NULL,
REGS_RegCodeX64_rax,
REGS_RegCodeX64_rcx,
REGS_RegCodeX64_rdx,
REGS_RegCodeX64_rbx,
REGS_RegCodeX64_rsp,
REGS_RegCodeX64_rbp,
REGS_RegCodeX64_rsi,
REGS_RegCodeX64_rdi,
REGS_RegCodeX64_r8,
REGS_RegCodeX64_r9,
REGS_RegCodeX64_r10,
REGS_RegCodeX64_r11,
REGS_RegCodeX64_r12,
REGS_RegCodeX64_r13,
REGS_RegCodeX64_r14,
REGS_RegCodeX64_r15,
REGS_RegCodeX64_fsbase,
REGS_RegCodeX64_gsbase,
REGS_RegCodeX64_rip,
REGS_RegCodeX64_rflags,
REGS_RegCodeX64_dr0,
REGS_RegCodeX64_dr1,
REGS_RegCodeX64_dr2,
REGS_RegCodeX64_dr3,
REGS_RegCodeX64_dr4,
REGS_RegCodeX64_dr5,
REGS_RegCodeX64_dr6,
REGS_RegCodeX64_dr7,
REGS_RegCodeX64_st0,
REGS_RegCodeX64_st1,
REGS_RegCodeX64_st2,
REGS_RegCodeX64_st3,
REGS_RegCodeX64_st4,
REGS_RegCodeX64_st5,
REGS_RegCodeX64_st6,
REGS_RegCodeX64_st7,
REGS_RegCodeX64_fcw,
REGS_RegCodeX64_fsw,
REGS_RegCodeX64_ftw,
REGS_RegCodeX64_fop,
REGS_RegCodeX64_fip,
REGS_RegCodeX64_fdp,
REGS_RegCodeX64_mxcsr,
REGS_RegCodeX64_mxcsr_mask,
REGS_RegCodeX64_ss,
REGS_RegCodeX64_cs,
REGS_RegCodeX64_ds,
REGS_RegCodeX64_es,
REGS_RegCodeX64_fs,
REGS_RegCodeX64_gs,
REGS_RegCodeX64_zmm0,
REGS_RegCodeX64_zmm1,
REGS_RegCodeX64_zmm2,
REGS_RegCodeX64_zmm3,
REGS_RegCodeX64_zmm4,
REGS_RegCodeX64_zmm5,
REGS_RegCodeX64_zmm6,
REGS_RegCodeX64_zmm7,
REGS_RegCodeX64_zmm8,
REGS_RegCodeX64_zmm9,
REGS_RegCodeX64_zmm10,
REGS_RegCodeX64_zmm11,
REGS_RegCodeX64_zmm12,
REGS_RegCodeX64_zmm13,
REGS_RegCodeX64_zmm14,
REGS_RegCodeX64_zmm15,
REGS_RegCodeX64_zmm16,
REGS_RegCodeX64_zmm17,
REGS_RegCodeX64_zmm18,
REGS_RegCodeX64_zmm19,
REGS_RegCodeX64_zmm20,
REGS_RegCodeX64_zmm21,
REGS_RegCodeX64_zmm22,
REGS_RegCodeX64_zmm23,
REGS_RegCodeX64_zmm24,
REGS_RegCodeX64_zmm25,
REGS_RegCodeX64_zmm26,
REGS_RegCodeX64_zmm27,
REGS_RegCodeX64_zmm28,
REGS_RegCodeX64_zmm29,
REGS_RegCodeX64_zmm30,
REGS_RegCodeX64_zmm31,
REGS_RegCodeX64_k0,
REGS_RegCodeX64_k1,
REGS_RegCodeX64_k2,
REGS_RegCodeX64_k3,
REGS_RegCodeX64_k4,
REGS_RegCodeX64_k5,
REGS_RegCodeX64_k6,
REGS_RegCodeX64_k7,
REGS_RegCodeX64_cetmsr,
REGS_RegCodeX64_cetssp,
REGS_RegCodeX64_COUNT,
} REGS_RegCodeX64;

typedef enum REGS_AliasCodeX64
{
REGS_AliasCodeX64_NULL,
REGS_AliasCodeX64_eax,
REGS_AliasCodeX64_ecx,
REGS_AliasCodeX64_edx,
REGS_AliasCodeX64_ebx,
REGS_AliasCodeX64_esp,
REGS_AliasCodeX64_ebp,
REGS_AliasCodeX64_esi,
REGS_AliasCodeX64_edi,
REGS_AliasCodeX64_r8d,
REGS_AliasCodeX64_r9d,
REGS_AliasCodeX64_r10d,
REGS_AliasCodeX64_r11d,
REGS_AliasCodeX64_r12d,
REGS_AliasCodeX64_r13d,
REGS_AliasCodeX64_r14d,
REGS_AliasCodeX64_r15d,
REGS_AliasCodeX64_eflags,
REGS_AliasCodeX64_ax,
REGS_AliasCodeX64_cx,
REGS_AliasCodeX64_dx,
REGS_AliasCodeX64_bx,
REGS_AliasCodeX64_si,
REGS_AliasCodeX64_di,
REGS_AliasCodeX64_sp,
REGS_AliasCodeX64_bp,
REGS_AliasCodeX64_ip,
REGS_AliasCodeX64_r8w,
REGS_AliasCodeX64_r9w,
REGS_AliasCodeX64_r10w,
REGS_AliasCodeX64_r11w,
REGS_AliasCodeX64_r12w,
REGS_AliasCodeX64_r13w,
REGS_AliasCodeX64_r14w,
REGS_AliasCodeX64_r15w,
REGS_AliasCodeX64_al,
REGS_AliasCodeX64_cl,
REGS_AliasCodeX64_dl,
REGS_AliasCodeX64_bl,
REGS_AliasCodeX64_sil,
REGS_AliasCodeX64_dil,
REGS_AliasCodeX64_bpl,
REGS_AliasCodeX64_spl,
REGS_AliasCodeX64_r8b,
REGS_AliasCodeX64_r9b,
REGS_AliasCodeX64_r10b,
REGS_AliasCodeX64_r11b,
REGS_AliasCodeX64_r12b,
REGS_AliasCodeX64_r13b,
REGS_AliasCodeX64_r14b,
REGS_AliasCodeX64_r15b,
REGS_AliasCodeX64_ah,
REGS_AliasCodeX64_ch,
REGS_AliasCodeX64_dh,
REGS_AliasCodeX64_bh,
REGS_AliasCodeX64_xmm0,
REGS_AliasCodeX64_xmm1,
REGS_AliasCodeX64_xmm2,
REGS_AliasCodeX64_xmm3,
REGS_AliasCodeX64_xmm4,
REGS_AliasCodeX64_xmm5,
REGS_AliasCodeX64_xmm6,
REGS_AliasCodeX64_xmm7,
REGS_AliasCodeX64_xmm8,
REGS_AliasCodeX64_xmm9,
REGS_AliasCodeX64_xmm10,
REGS_AliasCodeX64_xmm11,
REGS_AliasCodeX64_xmm12,
REGS_AliasCodeX64_xmm13,
REGS_AliasCodeX64_xmm14,
REGS_AliasCodeX64_xmm15,
REGS_AliasCodeX64_ymm0,
REGS_AliasCodeX64_ymm1,
REGS_AliasCodeX64_ymm2,
REGS_AliasCodeX64_ymm3,
REGS_AliasCodeX64_ymm4,
REGS_AliasCodeX64_ymm5,
REGS_AliasCodeX64_ymm6,
REGS_AliasCodeX64_ymm7,
REGS_AliasCodeX64_ymm8,
REGS_AliasCodeX64_ymm9,
REGS_AliasCodeX64_ymm10,
REGS_AliasCodeX64_ymm11,
REGS_AliasCodeX64_ymm12,
REGS_AliasCodeX64_ymm13,
REGS_AliasCodeX64_ymm14,
REGS_AliasCodeX64_ymm15,
REGS_AliasCodeX64_mm0,
REGS_AliasCodeX64_mm1,
REGS_AliasCodeX64_mm2,
REGS_AliasCodeX64_mm3,
REGS_AliasCodeX64_mm4,
REGS_AliasCodeX64_mm5,
REGS_AliasCodeX64_mm6,
REGS_AliasCodeX64_mm7,
REGS_AliasCodeX64_COUNT,
} REGS_AliasCodeX64;

typedef struct REGS_RegBlockX64 REGS_RegBlockX64;
struct REGS_RegBlockX64
{
REGS_Reg64 rax;
REGS_Reg64 rcx;
REGS_Reg64 rdx;
REGS_Reg64 rbx;
REGS_Reg64 rsp;
REGS_Reg64 rbp;
REGS_Reg64 rsi;
REGS_Reg64 rdi;
REGS_Reg64 r8;
REGS_Reg64 r9;
REGS_Reg64 r10;
REGS_Reg64 r11;
REGS_Reg64 r12;
REGS_Reg64 r13;
REGS_Reg64 r14;
REGS_Reg64 r15;
REGS_Reg64 fsbase;
REGS_Reg64 gsbase;
REGS_Reg64 rip;
REGS_Reg64 rflags;
REGS_Reg64 dr0;
REGS_Reg64 dr1;
REGS_Reg64 dr2;
REGS_Reg64 dr3;
REGS_Reg64 dr4;
REGS_Reg64 dr5;
REGS_Reg64 dr6;
REGS_Reg64 dr7;
REGS_Reg80 st0;
REGS_Reg80 st1;
REGS_Reg80 st2;
REGS_Reg80 st3;
REGS_Reg80 st4;
REGS_Reg80 st5;
REGS_Reg80 st6;
REGS_Reg80 st7;
REGS_Reg16 fcw;
REGS_Reg16 fsw;
REGS_Reg8 ftw;
REGS_Reg16 fop;
REGS_Reg64 fip;
REGS_Reg64 fdp;
REGS_Reg32 mxcsr;
REGS_Reg32 mxcsr_mask;
REGS_Reg16 ss;
REGS_Reg16 cs;
REGS_Reg16 ds;
REGS_Reg16 es;
REGS_Reg16 fs;
REGS_Reg16 gs;
REGS_Reg512 zmm0;
REGS_Reg512 zmm1;
REGS_Reg512 zmm2;
REGS_Reg512 zmm3;
REGS_Reg512 zmm4;
REGS_Reg512 zmm5;
REGS_Reg512 zmm6;
REGS_Reg512 zmm7;
REGS_Reg512 zmm8;
REGS_Reg512 zmm9;
REGS_Reg512 zmm10;
REGS_Reg512 zmm11;
REGS_Reg512 zmm12;
REGS_Reg512 zmm13;
REGS_Reg512 zmm14;
REGS_Reg512 zmm15;
REGS_Reg512 zmm16;
REGS_Reg512 zmm17;
REGS_Reg512 zmm18;
REGS_Reg512 zmm19;
REGS_Reg512 zmm20;
REGS_Reg512 zmm21;
REGS_Reg512 zmm22;
REGS_Reg512 zmm23;
REGS_Reg512 zmm24;
REGS_Reg512 zmm25;
REGS_Reg512 zmm26;
REGS_Reg512 zmm27;
REGS_Reg512 zmm28;
REGS_Reg512 zmm29;
REGS_Reg512 zmm30;
REGS_Reg512 zmm31;
REGS_Reg64 k0;
REGS_Reg64 k1;
REGS_Reg64 k2;
REGS_Reg64 k3;
REGS_Reg64 k4;
REGS_Reg64 k5;
REGS_Reg64 k6;
REGS_Reg64 k7;
REGS_Reg64 cetmsr;
REGS_Reg64 cetssp;
};

C_LINKAGE_BEGIN
extern REGS_UsageKind regs_g_reg_code_x64_usage_kind_table[93];
extern REGS_UsageKind regs_g_alias_code_x64_usage_kind_table[95];
extern String8 regs_g_reg_code_x64_string_table[93];
extern String8 regs_g_alias_code_x64_string_table[95];
extern REGS_Rng regs_g_reg_code_x64_rng_table[93];
extern REGS_Slice regs_g_alias_code_x64_slice_table[95];

C_LINKAGE_END

#endif // REGS_META_H
