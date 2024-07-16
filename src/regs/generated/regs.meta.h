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
REGS_RegCodeX64_ymm0,
REGS_RegCodeX64_ymm1,
REGS_RegCodeX64_ymm2,
REGS_RegCodeX64_ymm3,
REGS_RegCodeX64_ymm4,
REGS_RegCodeX64_ymm5,
REGS_RegCodeX64_ymm6,
REGS_RegCodeX64_ymm7,
REGS_RegCodeX64_ymm8,
REGS_RegCodeX64_ymm9,
REGS_RegCodeX64_ymm10,
REGS_RegCodeX64_ymm11,
REGS_RegCodeX64_ymm12,
REGS_RegCodeX64_ymm13,
REGS_RegCodeX64_ymm14,
REGS_RegCodeX64_ymm15,
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
REGS_Reg256 ymm0;
REGS_Reg256 ymm1;
REGS_Reg256 ymm2;
REGS_Reg256 ymm3;
REGS_Reg256 ymm4;
REGS_Reg256 ymm5;
REGS_Reg256 ymm6;
REGS_Reg256 ymm7;
REGS_Reg256 ymm8;
REGS_Reg256 ymm9;
REGS_Reg256 ymm10;
REGS_Reg256 ymm11;
REGS_Reg256 ymm12;
REGS_Reg256 ymm13;
REGS_Reg256 ymm14;
REGS_Reg256 ymm15;
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

C_LINKAGE_BEGIN
extern REGS_UsageKind regs_g_reg_code_x64_usage_kind_table[77];
extern REGS_UsageKind regs_g_alias_code_x64_usage_kind_table[80];
extern String8 regs_g_reg_code_x64_string_table[77];
extern String8 regs_g_alias_code_x64_string_table[80];
extern REGS_Rng regs_g_reg_code_x64_rng_table[77];
extern REGS_Slice regs_g_alias_code_x64_slice_table[80];
extern REGS_UsageKind regs_g_reg_code_x86_usage_kind_table[61];
extern REGS_UsageKind regs_g_alias_code_x86_usage_kind_table[36];
extern String8 regs_g_reg_code_x86_string_table[61];
extern String8 regs_g_alias_code_x86_string_table[36];
extern REGS_Rng regs_g_reg_code_x86_rng_table[61];
extern REGS_Slice regs_g_alias_code_x86_slice_table[36];

C_LINKAGE_END

#endif // REGS_META_H
