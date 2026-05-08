// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef X64_META_H
#define X64_META_H

typedef enum X64_RegCode
{
X64_RegCode_nil,
X64_RegCode_rax,
X64_RegCode_rcx,
X64_RegCode_rdx,
X64_RegCode_rbx,
X64_RegCode_rsp,
X64_RegCode_rbp,
X64_RegCode_rsi,
X64_RegCode_rdi,
X64_RegCode_r8,
X64_RegCode_r9,
X64_RegCode_r10,
X64_RegCode_r11,
X64_RegCode_r12,
X64_RegCode_r13,
X64_RegCode_r14,
X64_RegCode_r15,
X64_RegCode_fsbase,
X64_RegCode_gsbase,
X64_RegCode_rip,
X64_RegCode_rflags,
X64_RegCode_dr0,
X64_RegCode_dr1,
X64_RegCode_dr2,
X64_RegCode_dr3,
X64_RegCode_dr4,
X64_RegCode_dr5,
X64_RegCode_dr6,
X64_RegCode_dr7,
X64_RegCode_st0,
X64_RegCode_st1,
X64_RegCode_st2,
X64_RegCode_st3,
X64_RegCode_st4,
X64_RegCode_st5,
X64_RegCode_st6,
X64_RegCode_st7,
X64_RegCode_fcw,
X64_RegCode_fsw,
X64_RegCode_ftw,
X64_RegCode_fop,
X64_RegCode_fcs,
X64_RegCode_fds,
X64_RegCode_fip,
X64_RegCode_fdp,
X64_RegCode_mxcsr,
X64_RegCode_mxcsr_mask,
X64_RegCode_ss,
X64_RegCode_cs,
X64_RegCode_ds,
X64_RegCode_es,
X64_RegCode_fs,
X64_RegCode_gs,
X64_RegCode_zmm0,
X64_RegCode_zmm1,
X64_RegCode_zmm2,
X64_RegCode_zmm3,
X64_RegCode_zmm4,
X64_RegCode_zmm5,
X64_RegCode_zmm6,
X64_RegCode_zmm7,
X64_RegCode_zmm8,
X64_RegCode_zmm9,
X64_RegCode_zmm10,
X64_RegCode_zmm11,
X64_RegCode_zmm12,
X64_RegCode_zmm13,
X64_RegCode_zmm14,
X64_RegCode_zmm15,
X64_RegCode_zmm16,
X64_RegCode_zmm17,
X64_RegCode_zmm18,
X64_RegCode_zmm19,
X64_RegCode_zmm20,
X64_RegCode_zmm21,
X64_RegCode_zmm22,
X64_RegCode_zmm23,
X64_RegCode_zmm24,
X64_RegCode_zmm25,
X64_RegCode_zmm26,
X64_RegCode_zmm27,
X64_RegCode_zmm28,
X64_RegCode_zmm29,
X64_RegCode_zmm30,
X64_RegCode_zmm31,
X64_RegCode_k0,
X64_RegCode_k1,
X64_RegCode_k2,
X64_RegCode_k3,
X64_RegCode_k4,
X64_RegCode_k5,
X64_RegCode_k6,
X64_RegCode_k7,
X64_RegCode_cetmsr,
X64_RegCode_cetssp,
X64_RegCode_eax,
X64_RegCode_ecx,
X64_RegCode_edx,
X64_RegCode_ebx,
X64_RegCode_esp,
X64_RegCode_ebp,
X64_RegCode_esi,
X64_RegCode_edi,
X64_RegCode_r8d,
X64_RegCode_r9d,
X64_RegCode_r10d,
X64_RegCode_r11d,
X64_RegCode_r12d,
X64_RegCode_r13d,
X64_RegCode_r14d,
X64_RegCode_r15d,
X64_RegCode_eflags,
X64_RegCode_ax,
X64_RegCode_cx,
X64_RegCode_dx,
X64_RegCode_bx,
X64_RegCode_si,
X64_RegCode_di,
X64_RegCode_sp,
X64_RegCode_bp,
X64_RegCode_ip,
X64_RegCode_r8w,
X64_RegCode_r9w,
X64_RegCode_r10w,
X64_RegCode_r11w,
X64_RegCode_r12w,
X64_RegCode_r13w,
X64_RegCode_r14w,
X64_RegCode_r15w,
X64_RegCode_al,
X64_RegCode_cl,
X64_RegCode_dl,
X64_RegCode_bl,
X64_RegCode_sil,
X64_RegCode_dil,
X64_RegCode_bpl,
X64_RegCode_spl,
X64_RegCode_r8b,
X64_RegCode_r9b,
X64_RegCode_r10b,
X64_RegCode_r11b,
X64_RegCode_r12b,
X64_RegCode_r13b,
X64_RegCode_r14b,
X64_RegCode_r15b,
X64_RegCode_ah,
X64_RegCode_ch,
X64_RegCode_dh,
X64_RegCode_bh,
X64_RegCode_xmm0,
X64_RegCode_xmm1,
X64_RegCode_xmm2,
X64_RegCode_xmm3,
X64_RegCode_xmm4,
X64_RegCode_xmm5,
X64_RegCode_xmm6,
X64_RegCode_xmm7,
X64_RegCode_xmm8,
X64_RegCode_xmm9,
X64_RegCode_xmm10,
X64_RegCode_xmm11,
X64_RegCode_xmm12,
X64_RegCode_xmm13,
X64_RegCode_xmm14,
X64_RegCode_xmm15,
X64_RegCode_ymm0,
X64_RegCode_ymm1,
X64_RegCode_ymm2,
X64_RegCode_ymm3,
X64_RegCode_ymm4,
X64_RegCode_ymm5,
X64_RegCode_ymm6,
X64_RegCode_ymm7,
X64_RegCode_ymm8,
X64_RegCode_ymm9,
X64_RegCode_ymm10,
X64_RegCode_ymm11,
X64_RegCode_ymm12,
X64_RegCode_ymm13,
X64_RegCode_ymm14,
X64_RegCode_ymm15,
X64_RegCode_mm0,
X64_RegCode_mm1,
X64_RegCode_mm2,
X64_RegCode_mm3,
X64_RegCode_mm4,
X64_RegCode_mm5,
X64_RegCode_mm6,
X64_RegCode_mm7,
X64_RegCode_COUNT,
} X64_RegCode;

typedef struct X64_RegBlock X64_RegBlock;
struct X64_RegBlock
{
U64 rax;
U64 rcx;
U64 rdx;
U64 rbx;
U64 rsp;
U64 rbp;
U64 rsi;
U64 rdi;
U64 r8;
U64 r9;
U64 r10;
U64 r11;
U64 r12;
U64 r13;
U64 r14;
U64 r15;
U64 fsbase;
U64 gsbase;
U64 rip;
U64 rflags;
U64 dr0;
U64 dr1;
U64 dr2;
U64 dr3;
U64 dr4;
U64 dr5;
U64 dr6;
U64 dr7;
U80 st0;
U80 st1;
U80 st2;
U80 st3;
U80 st4;
U80 st5;
U80 st6;
U80 st7;
U16 fcw;
U16 fsw;
U8 ftw;
U16 fop;
U16 fcs;
U16 fds;
U64 fip;
U64 fdp;
U32 mxcsr;
U32 mxcsr_mask;
U16 ss;
U16 cs;
U16 ds;
U16 es;
U16 fs;
U16 gs;
U512 zmm0;
U512 zmm1;
U512 zmm2;
U512 zmm3;
U512 zmm4;
U512 zmm5;
U512 zmm6;
U512 zmm7;
U512 zmm8;
U512 zmm9;
U512 zmm10;
U512 zmm11;
U512 zmm12;
U512 zmm13;
U512 zmm14;
U512 zmm15;
U512 zmm16;
U512 zmm17;
U512 zmm18;
U512 zmm19;
U512 zmm20;
U512 zmm21;
U512 zmm22;
U512 zmm23;
U512 zmm24;
U512 zmm25;
U512 zmm26;
U512 zmm27;
U512 zmm28;
U512 zmm29;
U512 zmm30;
U512 zmm31;
U64 k0;
U64 k1;
U64 k2;
U64 k3;
U64 k4;
U64 k5;
U64 k6;
U64 k7;
U64 cetmsr;
U64 cetssp;
};

C_LINKAGE_BEGIN
extern B8 x64_reg_code_is_vector_table[189];
extern String8 x64_reg_code_name_table[189];
extern U8 x64_reg_code_base_table[189];
extern Rng1U16 x64_reg_code_rng_table[189];

C_LINKAGE_END

#endif // X64_META_H
