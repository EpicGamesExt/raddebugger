// Copyright (c) 2024 Epic Games Tools
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
REGS_RegCodeX64_fpr0,
REGS_RegCodeX64_fpr1,
REGS_RegCodeX64_fpr2,
REGS_RegCodeX64_fpr3,
REGS_RegCodeX64_fpr4,
REGS_RegCodeX64_fpr5,
REGS_RegCodeX64_fpr6,
REGS_RegCodeX64_fpr7,
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
REGS_RegCodeX64_fcs,
REGS_RegCodeX64_fds,
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
REGS_AliasCodeX64_eip,
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

typedef enum REGS_RegCodeX86
{
REGS_RegCodeX86_NULL,
REGS_RegCodeX86_eax,
REGS_RegCodeX86_ecx,
REGS_RegCodeX86_edx,
REGS_RegCodeX86_ebx,
REGS_RegCodeX86_esp,
REGS_RegCodeX86_ebp,
REGS_RegCodeX86_esi,
REGS_RegCodeX86_edi,
REGS_RegCodeX86_fsbase,
REGS_RegCodeX86_gsbase,
REGS_RegCodeX86_eflags,
REGS_RegCodeX86_eip,
REGS_RegCodeX86_dr0,
REGS_RegCodeX86_dr1,
REGS_RegCodeX86_dr2,
REGS_RegCodeX86_dr3,
REGS_RegCodeX86_dr4,
REGS_RegCodeX86_dr5,
REGS_RegCodeX86_dr6,
REGS_RegCodeX86_dr7,
REGS_RegCodeX86_fpr0,
REGS_RegCodeX86_fpr1,
REGS_RegCodeX86_fpr2,
REGS_RegCodeX86_fpr3,
REGS_RegCodeX86_fpr4,
REGS_RegCodeX86_fpr5,
REGS_RegCodeX86_fpr6,
REGS_RegCodeX86_fpr7,
REGS_RegCodeX86_st0,
REGS_RegCodeX86_st1,
REGS_RegCodeX86_st2,
REGS_RegCodeX86_st3,
REGS_RegCodeX86_st4,
REGS_RegCodeX86_st5,
REGS_RegCodeX86_st6,
REGS_RegCodeX86_st7,
REGS_RegCodeX86_fcw,
REGS_RegCodeX86_fsw,
REGS_RegCodeX86_ftw,
REGS_RegCodeX86_fop,
REGS_RegCodeX86_fcs,
REGS_RegCodeX86_fds,
REGS_RegCodeX86_fip,
REGS_RegCodeX86_fdp,
REGS_RegCodeX86_mxcsr,
REGS_RegCodeX86_mxcsr_mask,
REGS_RegCodeX86_ss,
REGS_RegCodeX86_cs,
REGS_RegCodeX86_ds,
REGS_RegCodeX86_es,
REGS_RegCodeX86_fs,
REGS_RegCodeX86_gs,
REGS_RegCodeX86_ymm0,
REGS_RegCodeX86_ymm1,
REGS_RegCodeX86_ymm2,
REGS_RegCodeX86_ymm3,
REGS_RegCodeX86_ymm4,
REGS_RegCodeX86_ymm5,
REGS_RegCodeX86_ymm6,
REGS_RegCodeX86_ymm7,
REGS_RegCodeX86_COUNT,
} REGS_RegCodeX86;

typedef enum REGS_AliasCodeX86
{
REGS_AliasCodeX86_NULL,
REGS_AliasCodeX86_ax,
REGS_AliasCodeX86_cx,
REGS_AliasCodeX86_bx,
REGS_AliasCodeX86_dx,
REGS_AliasCodeX86_sp,
REGS_AliasCodeX86_bp,
REGS_AliasCodeX86_si,
REGS_AliasCodeX86_di,
REGS_AliasCodeX86_ip,
REGS_AliasCodeX86_ah,
REGS_AliasCodeX86_ch,
REGS_AliasCodeX86_dh,
REGS_AliasCodeX86_bh,
REGS_AliasCodeX86_al,
REGS_AliasCodeX86_cl,
REGS_AliasCodeX86_dl,
REGS_AliasCodeX86_bl,
REGS_AliasCodeX86_bpl,
REGS_AliasCodeX86_spl,
REGS_AliasCodeX86_xmm0,
REGS_AliasCodeX86_xmm1,
REGS_AliasCodeX86_xmm2,
REGS_AliasCodeX86_xmm3,
REGS_AliasCodeX86_xmm4,
REGS_AliasCodeX86_xmm5,
REGS_AliasCodeX86_xmm6,
REGS_AliasCodeX86_xmm7,
REGS_AliasCodeX86_mm0,
REGS_AliasCodeX86_mm1,
REGS_AliasCodeX86_mm2,
REGS_AliasCodeX86_mm3,
REGS_AliasCodeX86_mm4,
REGS_AliasCodeX86_mm5,
REGS_AliasCodeX86_mm6,
REGS_AliasCodeX86_mm7,
REGS_AliasCodeX86_COUNT,
} REGS_AliasCodeX86;

typedef enum REGS_RegCodeARM64
{
REGS_RegCodeARM64_NULL,
REGS_RegCodeARM64_x0,
REGS_RegCodeARM64_x1,
REGS_RegCodeARM64_x2,
REGS_RegCodeARM64_x3,
REGS_RegCodeARM64_x4,
REGS_RegCodeARM64_x5,
REGS_RegCodeARM64_x6,
REGS_RegCodeARM64_x7,
REGS_RegCodeARM64_x8,
REGS_RegCodeARM64_x9,
REGS_RegCodeARM64_x10,
REGS_RegCodeARM64_x11,
REGS_RegCodeARM64_x12,
REGS_RegCodeARM64_x13,
REGS_RegCodeARM64_x14,
REGS_RegCodeARM64_x15,
REGS_RegCodeARM64_x16,
REGS_RegCodeARM64_x17,
REGS_RegCodeARM64_x18,
REGS_RegCodeARM64_x19,
REGS_RegCodeARM64_x20,
REGS_RegCodeARM64_x21,
REGS_RegCodeARM64_x22,
REGS_RegCodeARM64_x23,
REGS_RegCodeARM64_x24,
REGS_RegCodeARM64_x25,
REGS_RegCodeARM64_x26,
REGS_RegCodeARM64_x27,
REGS_RegCodeARM64_x28,
REGS_RegCodeARM64_x29,
REGS_RegCodeARM64_x30,
REGS_RegCodeARM64_x31,
REGS_RegCodeARM64_pc,
REGS_RegCodeARM64_context_flags,
REGS_RegCodeARM64_cpsr,
REGS_RegCodeARM64_fpcr,
REGS_RegCodeARM64_fpsr,
REGS_RegCodeARM64_bcr0,
REGS_RegCodeARM64_bcr1,
REGS_RegCodeARM64_bcr2,
REGS_RegCodeARM64_bcr3,
REGS_RegCodeARM64_bcr4,
REGS_RegCodeARM64_bcr5,
REGS_RegCodeARM64_bcr6,
REGS_RegCodeARM64_bcr7,
REGS_RegCodeARM64_bvr0,
REGS_RegCodeARM64_bvr1,
REGS_RegCodeARM64_bvr2,
REGS_RegCodeARM64_bvr3,
REGS_RegCodeARM64_bvr4,
REGS_RegCodeARM64_bvr5,
REGS_RegCodeARM64_bvr6,
REGS_RegCodeARM64_bvr7,
REGS_RegCodeARM64_wcr0,
REGS_RegCodeARM64_wcr1,
REGS_RegCodeARM64_wvr0,
REGS_RegCodeARM64_wvr1,
REGS_RegCodeARM64_v0,
REGS_RegCodeARM64_v1,
REGS_RegCodeARM64_v2,
REGS_RegCodeARM64_v3,
REGS_RegCodeARM64_v4,
REGS_RegCodeARM64_v5,
REGS_RegCodeARM64_v6,
REGS_RegCodeARM64_v7,
REGS_RegCodeARM64_v8,
REGS_RegCodeARM64_v9,
REGS_RegCodeARM64_v10,
REGS_RegCodeARM64_v11,
REGS_RegCodeARM64_v12,
REGS_RegCodeARM64_v13,
REGS_RegCodeARM64_v14,
REGS_RegCodeARM64_v15,
REGS_RegCodeARM64_v16,
REGS_RegCodeARM64_v17,
REGS_RegCodeARM64_v18,
REGS_RegCodeARM64_v19,
REGS_RegCodeARM64_v20,
REGS_RegCodeARM64_v21,
REGS_RegCodeARM64_v22,
REGS_RegCodeARM64_v23,
REGS_RegCodeARM64_v24,
REGS_RegCodeARM64_v25,
REGS_RegCodeARM64_v26,
REGS_RegCodeARM64_v27,
REGS_RegCodeARM64_v28,
REGS_RegCodeARM64_v29,
REGS_RegCodeARM64_v30,
REGS_RegCodeARM64_v31,
REGS_RegCodeARM64_COUNT,
} REGS_RegCodeARM64;

typedef enum REGS_AliasCodeARM64
{
REGS_AliasCodeARM64_NULL,
REGS_AliasCodeARM64_xip0,
REGS_AliasCodeARM64_xip1,
REGS_AliasCodeARM64_xpr,
REGS_AliasCodeARM64_fp,
REGS_AliasCodeARM64_lr,
REGS_AliasCodeARM64_zr,
REGS_AliasCodeARM64_sp,
REGS_AliasCodeARM64_w0,
REGS_AliasCodeARM64_w1,
REGS_AliasCodeARM64_w2,
REGS_AliasCodeARM64_w3,
REGS_AliasCodeARM64_w4,
REGS_AliasCodeARM64_w5,
REGS_AliasCodeARM64_w6,
REGS_AliasCodeARM64_w7,
REGS_AliasCodeARM64_w8,
REGS_AliasCodeARM64_w9,
REGS_AliasCodeARM64_w10,
REGS_AliasCodeARM64_w11,
REGS_AliasCodeARM64_w12,
REGS_AliasCodeARM64_w13,
REGS_AliasCodeARM64_w14,
REGS_AliasCodeARM64_w15,
REGS_AliasCodeARM64_w16,
REGS_AliasCodeARM64_w17,
REGS_AliasCodeARM64_w18,
REGS_AliasCodeARM64_w19,
REGS_AliasCodeARM64_w20,
REGS_AliasCodeARM64_w21,
REGS_AliasCodeARM64_w22,
REGS_AliasCodeARM64_w23,
REGS_AliasCodeARM64_w24,
REGS_AliasCodeARM64_w25,
REGS_AliasCodeARM64_w26,
REGS_AliasCodeARM64_w27,
REGS_AliasCodeARM64_w28,
REGS_AliasCodeARM64_w29,
REGS_AliasCodeARM64_w30,
REGS_AliasCodeARM64_w31,
REGS_AliasCodeARM64_COUNT,
} REGS_AliasCodeARM64;

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
REGS_Reg32 dr0;
REGS_Reg32 dr1;
REGS_Reg32 dr2;
REGS_Reg32 dr3;
REGS_Reg32 dr4;
REGS_Reg32 dr5;
REGS_Reg32 dr6;
REGS_Reg32 dr7;
REGS_Reg80 fpr0;
REGS_Reg80 fpr1;
REGS_Reg80 fpr2;
REGS_Reg80 fpr3;
REGS_Reg80 fpr4;
REGS_Reg80 fpr5;
REGS_Reg80 fpr6;
REGS_Reg80 fpr7;
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
REGS_Reg16 ftw;
REGS_Reg16 fop;
REGS_Reg16 fcs;
REGS_Reg16 fds;
REGS_Reg32 fip;
REGS_Reg32 fdp;
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
};

typedef struct REGS_RegBlockX86 REGS_RegBlockX86;
struct REGS_RegBlockX86
{
REGS_Reg32 eax;
REGS_Reg32 ecx;
REGS_Reg32 edx;
REGS_Reg32 ebx;
REGS_Reg32 esp;
REGS_Reg32 ebp;
REGS_Reg32 esi;
REGS_Reg32 edi;
REGS_Reg32 fsbase;
REGS_Reg32 gsbase;
REGS_Reg32 eflags;
REGS_Reg32 eip;
REGS_Reg32 dr0;
REGS_Reg32 dr1;
REGS_Reg32 dr2;
REGS_Reg32 dr3;
REGS_Reg32 dr4;
REGS_Reg32 dr5;
REGS_Reg32 dr6;
REGS_Reg32 dr7;
REGS_Reg80 fpr0;
REGS_Reg80 fpr1;
REGS_Reg80 fpr2;
REGS_Reg80 fpr3;
REGS_Reg80 fpr4;
REGS_Reg80 fpr5;
REGS_Reg80 fpr6;
REGS_Reg80 fpr7;
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
REGS_Reg16 ftw;
REGS_Reg16 fop;
REGS_Reg16 fcs;
REGS_Reg16 fds;
REGS_Reg32 fip;
REGS_Reg32 fdp;
REGS_Reg32 mxcsr;
REGS_Reg32 mxcsr_mask;
REGS_Reg16 ss;
REGS_Reg16 cs;
REGS_Reg16 ds;
REGS_Reg16 es;
REGS_Reg16 fs;
REGS_Reg16 gs;
REGS_Reg256 ymm0;
REGS_Reg256 ymm1;
REGS_Reg256 ymm2;
REGS_Reg256 ymm3;
REGS_Reg256 ymm4;
REGS_Reg256 ymm5;
REGS_Reg256 ymm6;
REGS_Reg256 ymm7;
};

typedef struct REGS_RegBlockARM64 REGS_RegBlockARM64;
struct REGS_RegBlockARM64
{
REGS_Reg64 x0;
REGS_Reg64 x1;
REGS_Reg64 x2;
REGS_Reg64 x3;
REGS_Reg64 x4;
REGS_Reg64 x5;
REGS_Reg64 x6;
REGS_Reg64 x7;
REGS_Reg64 x8;
REGS_Reg64 x9;
REGS_Reg64 x10;
REGS_Reg64 x11;
REGS_Reg64 x12;
REGS_Reg64 x13;
REGS_Reg64 x14;
REGS_Reg64 x15;
REGS_Reg64 x16;
REGS_Reg64 x17;
REGS_Reg64 x18;
REGS_Reg64 x19;
REGS_Reg64 x20;
REGS_Reg64 x21;
REGS_Reg64 x22;
REGS_Reg64 x23;
REGS_Reg64 x24;
REGS_Reg64 x25;
REGS_Reg64 x26;
REGS_Reg64 x27;
REGS_Reg64 x28;
REGS_Reg64 x29;
REGS_Reg64 x30;
REGS_Reg64 x31;
REGS_Reg64 pc;
REGS_Reg32 context_flags;
REGS_Reg32 cpsr;
REGS_Reg32 fpcr;
REGS_Reg32 fpsr;
REGS_Reg32 bcr0;
REGS_Reg32 bcr1;
REGS_Reg32 bcr2;
REGS_Reg32 bcr3;
REGS_Reg32 bcr4;
REGS_Reg32 bcr5;
REGS_Reg32 bcr6;
REGS_Reg32 bcr7;
REGS_Reg64 bvr0;
REGS_Reg64 bvr1;
REGS_Reg64 bvr2;
REGS_Reg64 bvr3;
REGS_Reg64 bvr4;
REGS_Reg64 bvr5;
REGS_Reg64 bvr6;
REGS_Reg64 bvr7;
REGS_Reg32 wcr0;
REGS_Reg32 wcr1;
REGS_Reg64 wvr0;
REGS_Reg64 wvr1;
REGS_Reg128 v0;
REGS_Reg128 v1;
REGS_Reg128 v2;
REGS_Reg128 v3;
REGS_Reg128 v4;
REGS_Reg128 v5;
REGS_Reg128 v6;
REGS_Reg128 v7;
REGS_Reg128 v8;
REGS_Reg128 v9;
REGS_Reg128 v10;
REGS_Reg128 v11;
REGS_Reg128 v12;
REGS_Reg128 v13;
REGS_Reg128 v14;
REGS_Reg128 v15;
REGS_Reg128 v16;
REGS_Reg128 v17;
REGS_Reg128 v18;
REGS_Reg128 v19;
REGS_Reg128 v20;
REGS_Reg128 v21;
REGS_Reg128 v22;
REGS_Reg128 v23;
REGS_Reg128 v24;
REGS_Reg128 v25;
REGS_Reg128 v26;
REGS_Reg128 v27;
REGS_Reg128 v28;
REGS_Reg128 v29;
REGS_Reg128 v30;
REGS_Reg128 v31;
};

C_LINKAGE_BEGIN
extern REGS_UsageKind regs_g_reg_code_x64_usage_kind_table[101];
extern REGS_UsageKind regs_g_alias_code_x64_usage_kind_table[96];
extern String8 regs_g_reg_code_x64_string_table[101];
extern String8 regs_g_alias_code_x64_string_table[96];
extern REGS_Rng regs_g_reg_code_x64_rng_table[101];
extern REGS_Slice regs_g_alias_code_x64_slice_table[96];
extern REGS_UsageKind regs_g_reg_code_x86_usage_kind_table[61];
extern REGS_UsageKind regs_g_alias_code_x86_usage_kind_table[36];
extern String8 regs_g_reg_code_x86_string_table[61];
extern String8 regs_g_alias_code_x86_string_table[36];
extern REGS_Rng regs_g_reg_code_x86_rng_table[61];
extern REGS_Slice regs_g_alias_code_x86_slice_table[36];
extern REGS_UsageKind regs_g_reg_code_arm64_usage_kind_table[90];
extern REGS_UsageKind regs_g_alias_code_arm64_usage_kind_table[40];
extern String8 regs_g_reg_code_arm64_string_table[90];
extern String8 regs_g_alias_code_arm64_string_table[40];
extern REGS_Rng regs_g_reg_code_arm64_rng_table[90];
extern REGS_Slice regs_g_alias_code_arm64_slice_table[40];

C_LINKAGE_END

#endif // REGS_META_H
