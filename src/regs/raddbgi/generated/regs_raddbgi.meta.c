// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

internal RDI_RegisterCode regs_rdi_code_from_arch_reg_code(Architecture arch, REGS_RegCode code)
{
RDI_RegisterCode result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:
{
switch(code)
{
default:{}break;
case REGS_RegCodeX64_rax:{result = RDI_RegisterCode_X64_rax;}break;
case REGS_RegCodeX64_rcx:{result = RDI_RegisterCode_X64_rcx;}break;
case REGS_RegCodeX64_rdx:{result = RDI_RegisterCode_X64_rdx;}break;
case REGS_RegCodeX64_rbx:{result = RDI_RegisterCode_X64_rbx;}break;
case REGS_RegCodeX64_rsp:{result = RDI_RegisterCode_X64_rsp;}break;
case REGS_RegCodeX64_rbp:{result = RDI_RegisterCode_X64_rbp;}break;
case REGS_RegCodeX64_rsi:{result = RDI_RegisterCode_X64_rsi;}break;
case REGS_RegCodeX64_rdi:{result = RDI_RegisterCode_X64_rdi;}break;
case REGS_RegCodeX64_r8:{result = RDI_RegisterCode_X64_r8;}break;
case REGS_RegCodeX64_r9:{result = RDI_RegisterCode_X64_r9;}break;
case REGS_RegCodeX64_r10:{result = RDI_RegisterCode_X64_r10;}break;
case REGS_RegCodeX64_r11:{result = RDI_RegisterCode_X64_r11;}break;
case REGS_RegCodeX64_r12:{result = RDI_RegisterCode_X64_r12;}break;
case REGS_RegCodeX64_r13:{result = RDI_RegisterCode_X64_r13;}break;
case REGS_RegCodeX64_r14:{result = RDI_RegisterCode_X64_r14;}break;
case REGS_RegCodeX64_r15:{result = RDI_RegisterCode_X64_r15;}break;
case REGS_RegCodeX64_fsbase:{result = RDI_RegisterCode_X64_fsbase;}break;
case REGS_RegCodeX64_gsbase:{result = RDI_RegisterCode_X64_gsbase;}break;
case REGS_RegCodeX64_rip:{result = RDI_RegisterCode_X64_rip;}break;
case REGS_RegCodeX64_rflags:{result = RDI_RegisterCode_X64_rflags;}break;
case REGS_RegCodeX64_dr0:{result = RDI_RegisterCode_X64_dr0;}break;
case REGS_RegCodeX64_dr1:{result = RDI_RegisterCode_X64_dr1;}break;
case REGS_RegCodeX64_dr2:{result = RDI_RegisterCode_X64_dr2;}break;
case REGS_RegCodeX64_dr3:{result = RDI_RegisterCode_X64_dr3;}break;
case REGS_RegCodeX64_dr4:{result = RDI_RegisterCode_X64_dr4;}break;
case REGS_RegCodeX64_dr5:{result = RDI_RegisterCode_X64_dr5;}break;
case REGS_RegCodeX64_dr6:{result = RDI_RegisterCode_X64_dr6;}break;
case REGS_RegCodeX64_dr7:{result = RDI_RegisterCode_X64_dr7;}break;
case REGS_RegCodeX64_fpr0:{result = RDI_RegisterCode_X64_fpr0;}break;
case REGS_RegCodeX64_fpr1:{result = RDI_RegisterCode_X64_fpr1;}break;
case REGS_RegCodeX64_fpr2:{result = RDI_RegisterCode_X64_fpr2;}break;
case REGS_RegCodeX64_fpr3:{result = RDI_RegisterCode_X64_fpr3;}break;
case REGS_RegCodeX64_fpr4:{result = RDI_RegisterCode_X64_fpr4;}break;
case REGS_RegCodeX64_fpr5:{result = RDI_RegisterCode_X64_fpr5;}break;
case REGS_RegCodeX64_fpr6:{result = RDI_RegisterCode_X64_fpr6;}break;
case REGS_RegCodeX64_fpr7:{result = RDI_RegisterCode_X64_fpr7;}break;
case REGS_RegCodeX64_st0:{result = RDI_RegisterCode_X64_st0;}break;
case REGS_RegCodeX64_st1:{result = RDI_RegisterCode_X64_st1;}break;
case REGS_RegCodeX64_st2:{result = RDI_RegisterCode_X64_st2;}break;
case REGS_RegCodeX64_st3:{result = RDI_RegisterCode_X64_st3;}break;
case REGS_RegCodeX64_st4:{result = RDI_RegisterCode_X64_st4;}break;
case REGS_RegCodeX64_st5:{result = RDI_RegisterCode_X64_st5;}break;
case REGS_RegCodeX64_st6:{result = RDI_RegisterCode_X64_st6;}break;
case REGS_RegCodeX64_st7:{result = RDI_RegisterCode_X64_st7;}break;
case REGS_RegCodeX64_fcw:{result = RDI_RegisterCode_X64_fcw;}break;
case REGS_RegCodeX64_fsw:{result = RDI_RegisterCode_X64_fsw;}break;
case REGS_RegCodeX64_ftw:{result = RDI_RegisterCode_X64_ftw;}break;
case REGS_RegCodeX64_fop:{result = RDI_RegisterCode_X64_fop;}break;
case REGS_RegCodeX64_fcs:{result = RDI_RegisterCode_X64_fcs;}break;
case REGS_RegCodeX64_fds:{result = RDI_RegisterCode_X64_fds;}break;
case REGS_RegCodeX64_fip:{result = RDI_RegisterCode_X64_fip;}break;
case REGS_RegCodeX64_fdp:{result = RDI_RegisterCode_X64_fdp;}break;
case REGS_RegCodeX64_mxcsr:{result = RDI_RegisterCode_X64_mxcsr;}break;
case REGS_RegCodeX64_mxcsr_mask:{result = RDI_RegisterCode_X64_mxcsr_mask;}break;
case REGS_RegCodeX64_ss:{result = RDI_RegisterCode_X64_ss;}break;
case REGS_RegCodeX64_cs:{result = RDI_RegisterCode_X64_cs;}break;
case REGS_RegCodeX64_ds:{result = RDI_RegisterCode_X64_ds;}break;
case REGS_RegCodeX64_es:{result = RDI_RegisterCode_X64_es;}break;
case REGS_RegCodeX64_fs:{result = RDI_RegisterCode_X64_fs;}break;
case REGS_RegCodeX64_gs:{result = RDI_RegisterCode_X64_gs;}break;
case REGS_RegCodeX64_ymm0:{result = RDI_RegisterCode_X64_ymm0;}break;
case REGS_RegCodeX64_ymm1:{result = RDI_RegisterCode_X64_ymm1;}break;
case REGS_RegCodeX64_ymm2:{result = RDI_RegisterCode_X64_ymm2;}break;
case REGS_RegCodeX64_ymm3:{result = RDI_RegisterCode_X64_ymm3;}break;
case REGS_RegCodeX64_ymm4:{result = RDI_RegisterCode_X64_ymm4;}break;
case REGS_RegCodeX64_ymm5:{result = RDI_RegisterCode_X64_ymm5;}break;
case REGS_RegCodeX64_ymm6:{result = RDI_RegisterCode_X64_ymm6;}break;
case REGS_RegCodeX64_ymm7:{result = RDI_RegisterCode_X64_ymm7;}break;
case REGS_RegCodeX64_ymm8:{result = RDI_RegisterCode_X64_ymm8;}break;
case REGS_RegCodeX64_ymm9:{result = RDI_RegisterCode_X64_ymm9;}break;
case REGS_RegCodeX64_ymm10:{result = RDI_RegisterCode_X64_ymm10;}break;
case REGS_RegCodeX64_ymm11:{result = RDI_RegisterCode_X64_ymm11;}break;
case REGS_RegCodeX64_ymm12:{result = RDI_RegisterCode_X64_ymm12;}break;
case REGS_RegCodeX64_ymm13:{result = RDI_RegisterCode_X64_ymm13;}break;
case REGS_RegCodeX64_ymm14:{result = RDI_RegisterCode_X64_ymm14;}break;
case REGS_RegCodeX64_ymm15:{result = RDI_RegisterCode_X64_ymm15;}break;
}
}break;
case Architecture_x86:
{
switch(code)
{
default:{}break;
case REGS_RegCodeX86_eax:{result = RDI_RegisterCode_X86_eax;}break;
case REGS_RegCodeX86_ecx:{result = RDI_RegisterCode_X86_ecx;}break;
case REGS_RegCodeX86_edx:{result = RDI_RegisterCode_X86_edx;}break;
case REGS_RegCodeX86_ebx:{result = RDI_RegisterCode_X86_ebx;}break;
case REGS_RegCodeX86_esp:{result = RDI_RegisterCode_X86_esp;}break;
case REGS_RegCodeX86_ebp:{result = RDI_RegisterCode_X86_ebp;}break;
case REGS_RegCodeX86_esi:{result = RDI_RegisterCode_X86_esi;}break;
case REGS_RegCodeX86_edi:{result = RDI_RegisterCode_X86_edi;}break;
case REGS_RegCodeX86_fsbase:{result = RDI_RegisterCode_X86_fsbase;}break;
case REGS_RegCodeX86_gsbase:{result = RDI_RegisterCode_X86_gsbase;}break;
case REGS_RegCodeX86_eflags:{result = RDI_RegisterCode_X86_eflags;}break;
case REGS_RegCodeX86_eip:{result = RDI_RegisterCode_X86_eip;}break;
case REGS_RegCodeX86_dr0:{result = RDI_RegisterCode_X86_dr0;}break;
case REGS_RegCodeX86_dr1:{result = RDI_RegisterCode_X86_dr1;}break;
case REGS_RegCodeX86_dr2:{result = RDI_RegisterCode_X86_dr2;}break;
case REGS_RegCodeX86_dr3:{result = RDI_RegisterCode_X86_dr3;}break;
case REGS_RegCodeX86_dr4:{result = RDI_RegisterCode_X86_dr4;}break;
case REGS_RegCodeX86_dr5:{result = RDI_RegisterCode_X86_dr5;}break;
case REGS_RegCodeX86_dr6:{result = RDI_RegisterCode_X86_dr6;}break;
case REGS_RegCodeX86_dr7:{result = RDI_RegisterCode_X86_dr7;}break;
case REGS_RegCodeX86_fpr0:{result = RDI_RegisterCode_X86_fpr0;}break;
case REGS_RegCodeX86_fpr1:{result = RDI_RegisterCode_X86_fpr1;}break;
case REGS_RegCodeX86_fpr2:{result = RDI_RegisterCode_X86_fpr2;}break;
case REGS_RegCodeX86_fpr3:{result = RDI_RegisterCode_X86_fpr3;}break;
case REGS_RegCodeX86_fpr4:{result = RDI_RegisterCode_X86_fpr4;}break;
case REGS_RegCodeX86_fpr5:{result = RDI_RegisterCode_X86_fpr5;}break;
case REGS_RegCodeX86_fpr6:{result = RDI_RegisterCode_X86_fpr6;}break;
case REGS_RegCodeX86_fpr7:{result = RDI_RegisterCode_X86_fpr7;}break;
case REGS_RegCodeX86_st0:{result = RDI_RegisterCode_X86_st0;}break;
case REGS_RegCodeX86_st1:{result = RDI_RegisterCode_X86_st1;}break;
case REGS_RegCodeX86_st2:{result = RDI_RegisterCode_X86_st2;}break;
case REGS_RegCodeX86_st3:{result = RDI_RegisterCode_X86_st3;}break;
case REGS_RegCodeX86_st4:{result = RDI_RegisterCode_X86_st4;}break;
case REGS_RegCodeX86_st5:{result = RDI_RegisterCode_X86_st5;}break;
case REGS_RegCodeX86_st6:{result = RDI_RegisterCode_X86_st6;}break;
case REGS_RegCodeX86_st7:{result = RDI_RegisterCode_X86_st7;}break;
case REGS_RegCodeX86_fcw:{result = RDI_RegisterCode_X86_fcw;}break;
case REGS_RegCodeX86_fsw:{result = RDI_RegisterCode_X86_fsw;}break;
case REGS_RegCodeX86_ftw:{result = RDI_RegisterCode_X86_ftw;}break;
case REGS_RegCodeX86_fop:{result = RDI_RegisterCode_X86_fop;}break;
case REGS_RegCodeX86_fcs:{result = RDI_RegisterCode_X86_fcs;}break;
case REGS_RegCodeX86_fds:{result = RDI_RegisterCode_X86_fds;}break;
case REGS_RegCodeX86_fip:{result = RDI_RegisterCode_X86_fip;}break;
case REGS_RegCodeX86_fdp:{result = RDI_RegisterCode_X86_fdp;}break;
case REGS_RegCodeX86_mxcsr:{result = RDI_RegisterCode_X86_mxcsr;}break;
case REGS_RegCodeX86_mxcsr_mask:{result = RDI_RegisterCode_X86_mxcsr_mask;}break;
case REGS_RegCodeX86_ss:{result = RDI_RegisterCode_X86_ss;}break;
case REGS_RegCodeX86_cs:{result = RDI_RegisterCode_X86_cs;}break;
case REGS_RegCodeX86_ds:{result = RDI_RegisterCode_X86_ds;}break;
case REGS_RegCodeX86_es:{result = RDI_RegisterCode_X86_es;}break;
case REGS_RegCodeX86_fs:{result = RDI_RegisterCode_X86_fs;}break;
case REGS_RegCodeX86_gs:{result = RDI_RegisterCode_X86_gs;}break;
case REGS_RegCodeX86_ymm0:{result = RDI_RegisterCode_X86_ymm0;}break;
case REGS_RegCodeX86_ymm1:{result = RDI_RegisterCode_X86_ymm1;}break;
case REGS_RegCodeX86_ymm2:{result = RDI_RegisterCode_X86_ymm2;}break;
case REGS_RegCodeX86_ymm3:{result = RDI_RegisterCode_X86_ymm3;}break;
case REGS_RegCodeX86_ymm4:{result = RDI_RegisterCode_X86_ymm4;}break;
case REGS_RegCodeX86_ymm5:{result = RDI_RegisterCode_X86_ymm5;}break;
case REGS_RegCodeX86_ymm6:{result = RDI_RegisterCode_X86_ymm6;}break;
case REGS_RegCodeX86_ymm7:{result = RDI_RegisterCode_X86_ymm7;}break;
}
}break;
}
return result;
}
internal REGS_RegCode regs_reg_code_from_arch_rdi_code(Architecture arch, RDI_RegisterCode code)
{
REGS_RegCode result = 0;
switch(arch)
{
default:{}break;
case Architecture_x64:
{
switch(code)
{
default:{}break;
case RDI_RegisterCode_X64_rax:{result = REGS_RegCodeX64_rax;}break;
case RDI_RegisterCode_X64_rcx:{result = REGS_RegCodeX64_rcx;}break;
case RDI_RegisterCode_X64_rdx:{result = REGS_RegCodeX64_rdx;}break;
case RDI_RegisterCode_X64_rbx:{result = REGS_RegCodeX64_rbx;}break;
case RDI_RegisterCode_X64_rsp:{result = REGS_RegCodeX64_rsp;}break;
case RDI_RegisterCode_X64_rbp:{result = REGS_RegCodeX64_rbp;}break;
case RDI_RegisterCode_X64_rsi:{result = REGS_RegCodeX64_rsi;}break;
case RDI_RegisterCode_X64_rdi:{result = REGS_RegCodeX64_rdi;}break;
case RDI_RegisterCode_X64_r8:{result = REGS_RegCodeX64_r8;}break;
case RDI_RegisterCode_X64_r9:{result = REGS_RegCodeX64_r9;}break;
case RDI_RegisterCode_X64_r10:{result = REGS_RegCodeX64_r10;}break;
case RDI_RegisterCode_X64_r11:{result = REGS_RegCodeX64_r11;}break;
case RDI_RegisterCode_X64_r12:{result = REGS_RegCodeX64_r12;}break;
case RDI_RegisterCode_X64_r13:{result = REGS_RegCodeX64_r13;}break;
case RDI_RegisterCode_X64_r14:{result = REGS_RegCodeX64_r14;}break;
case RDI_RegisterCode_X64_r15:{result = REGS_RegCodeX64_r15;}break;
case RDI_RegisterCode_X64_fsbase:{result = REGS_RegCodeX64_fsbase;}break;
case RDI_RegisterCode_X64_gsbase:{result = REGS_RegCodeX64_gsbase;}break;
case RDI_RegisterCode_X64_rip:{result = REGS_RegCodeX64_rip;}break;
case RDI_RegisterCode_X64_rflags:{result = REGS_RegCodeX64_rflags;}break;
case RDI_RegisterCode_X64_dr0:{result = REGS_RegCodeX64_dr0;}break;
case RDI_RegisterCode_X64_dr1:{result = REGS_RegCodeX64_dr1;}break;
case RDI_RegisterCode_X64_dr2:{result = REGS_RegCodeX64_dr2;}break;
case RDI_RegisterCode_X64_dr3:{result = REGS_RegCodeX64_dr3;}break;
case RDI_RegisterCode_X64_dr4:{result = REGS_RegCodeX64_dr4;}break;
case RDI_RegisterCode_X64_dr5:{result = REGS_RegCodeX64_dr5;}break;
case RDI_RegisterCode_X64_dr6:{result = REGS_RegCodeX64_dr6;}break;
case RDI_RegisterCode_X64_dr7:{result = REGS_RegCodeX64_dr7;}break;
case RDI_RegisterCode_X64_fpr0:{result = REGS_RegCodeX64_fpr0;}break;
case RDI_RegisterCode_X64_fpr1:{result = REGS_RegCodeX64_fpr1;}break;
case RDI_RegisterCode_X64_fpr2:{result = REGS_RegCodeX64_fpr2;}break;
case RDI_RegisterCode_X64_fpr3:{result = REGS_RegCodeX64_fpr3;}break;
case RDI_RegisterCode_X64_fpr4:{result = REGS_RegCodeX64_fpr4;}break;
case RDI_RegisterCode_X64_fpr5:{result = REGS_RegCodeX64_fpr5;}break;
case RDI_RegisterCode_X64_fpr6:{result = REGS_RegCodeX64_fpr6;}break;
case RDI_RegisterCode_X64_fpr7:{result = REGS_RegCodeX64_fpr7;}break;
case RDI_RegisterCode_X64_st0:{result = REGS_RegCodeX64_st0;}break;
case RDI_RegisterCode_X64_st1:{result = REGS_RegCodeX64_st1;}break;
case RDI_RegisterCode_X64_st2:{result = REGS_RegCodeX64_st2;}break;
case RDI_RegisterCode_X64_st3:{result = REGS_RegCodeX64_st3;}break;
case RDI_RegisterCode_X64_st4:{result = REGS_RegCodeX64_st4;}break;
case RDI_RegisterCode_X64_st5:{result = REGS_RegCodeX64_st5;}break;
case RDI_RegisterCode_X64_st6:{result = REGS_RegCodeX64_st6;}break;
case RDI_RegisterCode_X64_st7:{result = REGS_RegCodeX64_st7;}break;
case RDI_RegisterCode_X64_fcw:{result = REGS_RegCodeX64_fcw;}break;
case RDI_RegisterCode_X64_fsw:{result = REGS_RegCodeX64_fsw;}break;
case RDI_RegisterCode_X64_ftw:{result = REGS_RegCodeX64_ftw;}break;
case RDI_RegisterCode_X64_fop:{result = REGS_RegCodeX64_fop;}break;
case RDI_RegisterCode_X64_fcs:{result = REGS_RegCodeX64_fcs;}break;
case RDI_RegisterCode_X64_fds:{result = REGS_RegCodeX64_fds;}break;
case RDI_RegisterCode_X64_fip:{result = REGS_RegCodeX64_fip;}break;
case RDI_RegisterCode_X64_fdp:{result = REGS_RegCodeX64_fdp;}break;
case RDI_RegisterCode_X64_mxcsr:{result = REGS_RegCodeX64_mxcsr;}break;
case RDI_RegisterCode_X64_mxcsr_mask:{result = REGS_RegCodeX64_mxcsr_mask;}break;
case RDI_RegisterCode_X64_ss:{result = REGS_RegCodeX64_ss;}break;
case RDI_RegisterCode_X64_cs:{result = REGS_RegCodeX64_cs;}break;
case RDI_RegisterCode_X64_ds:{result = REGS_RegCodeX64_ds;}break;
case RDI_RegisterCode_X64_es:{result = REGS_RegCodeX64_es;}break;
case RDI_RegisterCode_X64_fs:{result = REGS_RegCodeX64_fs;}break;
case RDI_RegisterCode_X64_gs:{result = REGS_RegCodeX64_gs;}break;
case RDI_RegisterCode_X64_ymm0:{result = REGS_RegCodeX64_ymm0;}break;
case RDI_RegisterCode_X64_ymm1:{result = REGS_RegCodeX64_ymm1;}break;
case RDI_RegisterCode_X64_ymm2:{result = REGS_RegCodeX64_ymm2;}break;
case RDI_RegisterCode_X64_ymm3:{result = REGS_RegCodeX64_ymm3;}break;
case RDI_RegisterCode_X64_ymm4:{result = REGS_RegCodeX64_ymm4;}break;
case RDI_RegisterCode_X64_ymm5:{result = REGS_RegCodeX64_ymm5;}break;
case RDI_RegisterCode_X64_ymm6:{result = REGS_RegCodeX64_ymm6;}break;
case RDI_RegisterCode_X64_ymm7:{result = REGS_RegCodeX64_ymm7;}break;
case RDI_RegisterCode_X64_ymm8:{result = REGS_RegCodeX64_ymm8;}break;
case RDI_RegisterCode_X64_ymm9:{result = REGS_RegCodeX64_ymm9;}break;
case RDI_RegisterCode_X64_ymm10:{result = REGS_RegCodeX64_ymm10;}break;
case RDI_RegisterCode_X64_ymm11:{result = REGS_RegCodeX64_ymm11;}break;
case RDI_RegisterCode_X64_ymm12:{result = REGS_RegCodeX64_ymm12;}break;
case RDI_RegisterCode_X64_ymm13:{result = REGS_RegCodeX64_ymm13;}break;
case RDI_RegisterCode_X64_ymm14:{result = REGS_RegCodeX64_ymm14;}break;
case RDI_RegisterCode_X64_ymm15:{result = REGS_RegCodeX64_ymm15;}break;
}
}break;
case Architecture_x86:
{
switch(code)
{
default:{}break;
case RDI_RegisterCode_X86_eax:{result = REGS_RegCodeX86_eax;}break;
case RDI_RegisterCode_X86_ecx:{result = REGS_RegCodeX86_ecx;}break;
case RDI_RegisterCode_X86_edx:{result = REGS_RegCodeX86_edx;}break;
case RDI_RegisterCode_X86_ebx:{result = REGS_RegCodeX86_ebx;}break;
case RDI_RegisterCode_X86_esp:{result = REGS_RegCodeX86_esp;}break;
case RDI_RegisterCode_X86_ebp:{result = REGS_RegCodeX86_ebp;}break;
case RDI_RegisterCode_X86_esi:{result = REGS_RegCodeX86_esi;}break;
case RDI_RegisterCode_X86_edi:{result = REGS_RegCodeX86_edi;}break;
case RDI_RegisterCode_X86_fsbase:{result = REGS_RegCodeX86_fsbase;}break;
case RDI_RegisterCode_X86_gsbase:{result = REGS_RegCodeX86_gsbase;}break;
case RDI_RegisterCode_X86_eflags:{result = REGS_RegCodeX86_eflags;}break;
case RDI_RegisterCode_X86_eip:{result = REGS_RegCodeX86_eip;}break;
case RDI_RegisterCode_X86_dr0:{result = REGS_RegCodeX86_dr0;}break;
case RDI_RegisterCode_X86_dr1:{result = REGS_RegCodeX86_dr1;}break;
case RDI_RegisterCode_X86_dr2:{result = REGS_RegCodeX86_dr2;}break;
case RDI_RegisterCode_X86_dr3:{result = REGS_RegCodeX86_dr3;}break;
case RDI_RegisterCode_X86_dr4:{result = REGS_RegCodeX86_dr4;}break;
case RDI_RegisterCode_X86_dr5:{result = REGS_RegCodeX86_dr5;}break;
case RDI_RegisterCode_X86_dr6:{result = REGS_RegCodeX86_dr6;}break;
case RDI_RegisterCode_X86_dr7:{result = REGS_RegCodeX86_dr7;}break;
case RDI_RegisterCode_X86_fpr0:{result = REGS_RegCodeX86_fpr0;}break;
case RDI_RegisterCode_X86_fpr1:{result = REGS_RegCodeX86_fpr1;}break;
case RDI_RegisterCode_X86_fpr2:{result = REGS_RegCodeX86_fpr2;}break;
case RDI_RegisterCode_X86_fpr3:{result = REGS_RegCodeX86_fpr3;}break;
case RDI_RegisterCode_X86_fpr4:{result = REGS_RegCodeX86_fpr4;}break;
case RDI_RegisterCode_X86_fpr5:{result = REGS_RegCodeX86_fpr5;}break;
case RDI_RegisterCode_X86_fpr6:{result = REGS_RegCodeX86_fpr6;}break;
case RDI_RegisterCode_X86_fpr7:{result = REGS_RegCodeX86_fpr7;}break;
case RDI_RegisterCode_X86_st0:{result = REGS_RegCodeX86_st0;}break;
case RDI_RegisterCode_X86_st1:{result = REGS_RegCodeX86_st1;}break;
case RDI_RegisterCode_X86_st2:{result = REGS_RegCodeX86_st2;}break;
case RDI_RegisterCode_X86_st3:{result = REGS_RegCodeX86_st3;}break;
case RDI_RegisterCode_X86_st4:{result = REGS_RegCodeX86_st4;}break;
case RDI_RegisterCode_X86_st5:{result = REGS_RegCodeX86_st5;}break;
case RDI_RegisterCode_X86_st6:{result = REGS_RegCodeX86_st6;}break;
case RDI_RegisterCode_X86_st7:{result = REGS_RegCodeX86_st7;}break;
case RDI_RegisterCode_X86_fcw:{result = REGS_RegCodeX86_fcw;}break;
case RDI_RegisterCode_X86_fsw:{result = REGS_RegCodeX86_fsw;}break;
case RDI_RegisterCode_X86_ftw:{result = REGS_RegCodeX86_ftw;}break;
case RDI_RegisterCode_X86_fop:{result = REGS_RegCodeX86_fop;}break;
case RDI_RegisterCode_X86_fcs:{result = REGS_RegCodeX86_fcs;}break;
case RDI_RegisterCode_X86_fds:{result = REGS_RegCodeX86_fds;}break;
case RDI_RegisterCode_X86_fip:{result = REGS_RegCodeX86_fip;}break;
case RDI_RegisterCode_X86_fdp:{result = REGS_RegCodeX86_fdp;}break;
case RDI_RegisterCode_X86_mxcsr:{result = REGS_RegCodeX86_mxcsr;}break;
case RDI_RegisterCode_X86_mxcsr_mask:{result = REGS_RegCodeX86_mxcsr_mask;}break;
case RDI_RegisterCode_X86_ss:{result = REGS_RegCodeX86_ss;}break;
case RDI_RegisterCode_X86_cs:{result = REGS_RegCodeX86_cs;}break;
case RDI_RegisterCode_X86_ds:{result = REGS_RegCodeX86_ds;}break;
case RDI_RegisterCode_X86_es:{result = REGS_RegCodeX86_es;}break;
case RDI_RegisterCode_X86_fs:{result = REGS_RegCodeX86_fs;}break;
case RDI_RegisterCode_X86_gs:{result = REGS_RegCodeX86_gs;}break;
case RDI_RegisterCode_X86_ymm0:{result = REGS_RegCodeX86_ymm0;}break;
case RDI_RegisterCode_X86_ymm1:{result = REGS_RegCodeX86_ymm1;}break;
case RDI_RegisterCode_X86_ymm2:{result = REGS_RegCodeX86_ymm2;}break;
case RDI_RegisterCode_X86_ymm3:{result = REGS_RegCodeX86_ymm3;}break;
case RDI_RegisterCode_X86_ymm4:{result = REGS_RegCodeX86_ymm4;}break;
case RDI_RegisterCode_X86_ymm5:{result = REGS_RegCodeX86_ymm5;}break;
case RDI_RegisterCode_X86_ymm6:{result = REGS_RegCodeX86_ymm6;}break;
case RDI_RegisterCode_X86_ymm7:{result = REGS_RegCodeX86_ymm7;}break;
}
}break;
}
return result;
}
C_LINKAGE_BEGIN
C_LINKAGE_END

