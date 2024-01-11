// generated
#ifndef _REGS_RADDBG_META_C
#define _REGS_RADDBG_META_C

internal RADDBG_RegisterCode regs_raddbg_code_from_arch_reg_code(Architecture arch, REGS_RegCode code)
{
RADDBG_RegisterCode result = 0;
switch(arch)
{
case Architecture_x64:
{
switch(code)
{
default:{}break;
case REGS_RegCodeX64_rax:{result = RADDBG_RegisterCode_X64_rax;}break;
case REGS_RegCodeX64_rcx:{result = RADDBG_RegisterCode_X64_rcx;}break;
case REGS_RegCodeX64_rdx:{result = RADDBG_RegisterCode_X64_rdx;}break;
case REGS_RegCodeX64_rbx:{result = RADDBG_RegisterCode_X64_rbx;}break;
case REGS_RegCodeX64_rsp:{result = RADDBG_RegisterCode_X64_rsp;}break;
case REGS_RegCodeX64_rbp:{result = RADDBG_RegisterCode_X64_rbp;}break;
case REGS_RegCodeX64_rsi:{result = RADDBG_RegisterCode_X64_rsi;}break;
case REGS_RegCodeX64_rdi:{result = RADDBG_RegisterCode_X64_rdi;}break;
case REGS_RegCodeX64_r8:{result = RADDBG_RegisterCode_X64_r8;}break;
case REGS_RegCodeX64_r9:{result = RADDBG_RegisterCode_X64_r9;}break;
case REGS_RegCodeX64_r10:{result = RADDBG_RegisterCode_X64_r10;}break;
case REGS_RegCodeX64_r11:{result = RADDBG_RegisterCode_X64_r11;}break;
case REGS_RegCodeX64_r12:{result = RADDBG_RegisterCode_X64_r12;}break;
case REGS_RegCodeX64_r13:{result = RADDBG_RegisterCode_X64_r13;}break;
case REGS_RegCodeX64_r14:{result = RADDBG_RegisterCode_X64_r14;}break;
case REGS_RegCodeX64_r15:{result = RADDBG_RegisterCode_X64_r15;}break;
case REGS_RegCodeX64_fsbase:{result = RADDBG_RegisterCode_X64_fsbase;}break;
case REGS_RegCodeX64_gsbase:{result = RADDBG_RegisterCode_X64_gsbase;}break;
case REGS_RegCodeX64_rip:{result = RADDBG_RegisterCode_X64_rip;}break;
case REGS_RegCodeX64_rflags:{result = RADDBG_RegisterCode_X64_rflags;}break;
case REGS_RegCodeX64_dr0:{result = RADDBG_RegisterCode_X64_dr0;}break;
case REGS_RegCodeX64_dr1:{result = RADDBG_RegisterCode_X64_dr1;}break;
case REGS_RegCodeX64_dr2:{result = RADDBG_RegisterCode_X64_dr2;}break;
case REGS_RegCodeX64_dr3:{result = RADDBG_RegisterCode_X64_dr3;}break;
case REGS_RegCodeX64_dr4:{result = RADDBG_RegisterCode_X64_dr4;}break;
case REGS_RegCodeX64_dr5:{result = RADDBG_RegisterCode_X64_dr5;}break;
case REGS_RegCodeX64_dr6:{result = RADDBG_RegisterCode_X64_dr6;}break;
case REGS_RegCodeX64_dr7:{result = RADDBG_RegisterCode_X64_dr7;}break;
case REGS_RegCodeX64_fpr0:{result = RADDBG_RegisterCode_X64_fpr0;}break;
case REGS_RegCodeX64_fpr1:{result = RADDBG_RegisterCode_X64_fpr1;}break;
case REGS_RegCodeX64_fpr2:{result = RADDBG_RegisterCode_X64_fpr2;}break;
case REGS_RegCodeX64_fpr3:{result = RADDBG_RegisterCode_X64_fpr3;}break;
case REGS_RegCodeX64_fpr4:{result = RADDBG_RegisterCode_X64_fpr4;}break;
case REGS_RegCodeX64_fpr5:{result = RADDBG_RegisterCode_X64_fpr5;}break;
case REGS_RegCodeX64_fpr6:{result = RADDBG_RegisterCode_X64_fpr6;}break;
case REGS_RegCodeX64_fpr7:{result = RADDBG_RegisterCode_X64_fpr7;}break;
case REGS_RegCodeX64_st0:{result = RADDBG_RegisterCode_X64_st0;}break;
case REGS_RegCodeX64_st1:{result = RADDBG_RegisterCode_X64_st1;}break;
case REGS_RegCodeX64_st2:{result = RADDBG_RegisterCode_X64_st2;}break;
case REGS_RegCodeX64_st3:{result = RADDBG_RegisterCode_X64_st3;}break;
case REGS_RegCodeX64_st4:{result = RADDBG_RegisterCode_X64_st4;}break;
case REGS_RegCodeX64_st5:{result = RADDBG_RegisterCode_X64_st5;}break;
case REGS_RegCodeX64_st6:{result = RADDBG_RegisterCode_X64_st6;}break;
case REGS_RegCodeX64_st7:{result = RADDBG_RegisterCode_X64_st7;}break;
case REGS_RegCodeX64_fcw:{result = RADDBG_RegisterCode_X64_fcw;}break;
case REGS_RegCodeX64_fsw:{result = RADDBG_RegisterCode_X64_fsw;}break;
case REGS_RegCodeX64_ftw:{result = RADDBG_RegisterCode_X64_ftw;}break;
case REGS_RegCodeX64_fop:{result = RADDBG_RegisterCode_X64_fop;}break;
case REGS_RegCodeX64_fcs:{result = RADDBG_RegisterCode_X64_fcs;}break;
case REGS_RegCodeX64_fds:{result = RADDBG_RegisterCode_X64_fds;}break;
case REGS_RegCodeX64_fip:{result = RADDBG_RegisterCode_X64_fip;}break;
case REGS_RegCodeX64_fdp:{result = RADDBG_RegisterCode_X64_fdp;}break;
case REGS_RegCodeX64_mxcsr:{result = RADDBG_RegisterCode_X64_mxcsr;}break;
case REGS_RegCodeX64_mxcsr_mask:{result = RADDBG_RegisterCode_X64_mxcsr_mask;}break;
case REGS_RegCodeX64_ss:{result = RADDBG_RegisterCode_X64_ss;}break;
case REGS_RegCodeX64_cs:{result = RADDBG_RegisterCode_X64_cs;}break;
case REGS_RegCodeX64_ds:{result = RADDBG_RegisterCode_X64_ds;}break;
case REGS_RegCodeX64_es:{result = RADDBG_RegisterCode_X64_es;}break;
case REGS_RegCodeX64_fs:{result = RADDBG_RegisterCode_X64_fs;}break;
case REGS_RegCodeX64_gs:{result = RADDBG_RegisterCode_X64_gs;}break;
case REGS_RegCodeX64_ymm0:{result = RADDBG_RegisterCode_X64_ymm0;}break;
case REGS_RegCodeX64_ymm1:{result = RADDBG_RegisterCode_X64_ymm1;}break;
case REGS_RegCodeX64_ymm2:{result = RADDBG_RegisterCode_X64_ymm2;}break;
case REGS_RegCodeX64_ymm3:{result = RADDBG_RegisterCode_X64_ymm3;}break;
case REGS_RegCodeX64_ymm4:{result = RADDBG_RegisterCode_X64_ymm4;}break;
case REGS_RegCodeX64_ymm5:{result = RADDBG_RegisterCode_X64_ymm5;}break;
case REGS_RegCodeX64_ymm6:{result = RADDBG_RegisterCode_X64_ymm6;}break;
case REGS_RegCodeX64_ymm7:{result = RADDBG_RegisterCode_X64_ymm7;}break;
case REGS_RegCodeX64_ymm8:{result = RADDBG_RegisterCode_X64_ymm8;}break;
case REGS_RegCodeX64_ymm9:{result = RADDBG_RegisterCode_X64_ymm9;}break;
case REGS_RegCodeX64_ymm10:{result = RADDBG_RegisterCode_X64_ymm10;}break;
case REGS_RegCodeX64_ymm11:{result = RADDBG_RegisterCode_X64_ymm11;}break;
case REGS_RegCodeX64_ymm12:{result = RADDBG_RegisterCode_X64_ymm12;}break;
case REGS_RegCodeX64_ymm13:{result = RADDBG_RegisterCode_X64_ymm13;}break;
case REGS_RegCodeX64_ymm14:{result = RADDBG_RegisterCode_X64_ymm14;}break;
case REGS_RegCodeX64_ymm15:{result = RADDBG_RegisterCode_X64_ymm15;}break;
}
}break;
case Architecture_x86:
{
switch(code)
{
default:{}break;
case REGS_RegCodeX86_eax:{result = RADDBG_RegisterCode_X86_eax;}break;
case REGS_RegCodeX86_ecx:{result = RADDBG_RegisterCode_X86_ecx;}break;
case REGS_RegCodeX86_edx:{result = RADDBG_RegisterCode_X86_edx;}break;
case REGS_RegCodeX86_ebx:{result = RADDBG_RegisterCode_X86_ebx;}break;
case REGS_RegCodeX86_esp:{result = RADDBG_RegisterCode_X86_esp;}break;
case REGS_RegCodeX86_ebp:{result = RADDBG_RegisterCode_X86_ebp;}break;
case REGS_RegCodeX86_esi:{result = RADDBG_RegisterCode_X86_esi;}break;
case REGS_RegCodeX86_edi:{result = RADDBG_RegisterCode_X86_edi;}break;
case REGS_RegCodeX86_fsbase:{result = RADDBG_RegisterCode_X86_fsbase;}break;
case REGS_RegCodeX86_gsbase:{result = RADDBG_RegisterCode_X86_gsbase;}break;
case REGS_RegCodeX86_eflags:{result = RADDBG_RegisterCode_X86_eflags;}break;
case REGS_RegCodeX86_eip:{result = RADDBG_RegisterCode_X86_eip;}break;
case REGS_RegCodeX86_dr0:{result = RADDBG_RegisterCode_X86_dr0;}break;
case REGS_RegCodeX86_dr1:{result = RADDBG_RegisterCode_X86_dr1;}break;
case REGS_RegCodeX86_dr2:{result = RADDBG_RegisterCode_X86_dr2;}break;
case REGS_RegCodeX86_dr3:{result = RADDBG_RegisterCode_X86_dr3;}break;
case REGS_RegCodeX86_dr4:{result = RADDBG_RegisterCode_X86_dr4;}break;
case REGS_RegCodeX86_dr5:{result = RADDBG_RegisterCode_X86_dr5;}break;
case REGS_RegCodeX86_dr6:{result = RADDBG_RegisterCode_X86_dr6;}break;
case REGS_RegCodeX86_dr7:{result = RADDBG_RegisterCode_X86_dr7;}break;
case REGS_RegCodeX86_fpr0:{result = RADDBG_RegisterCode_X86_fpr0;}break;
case REGS_RegCodeX86_fpr1:{result = RADDBG_RegisterCode_X86_fpr1;}break;
case REGS_RegCodeX86_fpr2:{result = RADDBG_RegisterCode_X86_fpr2;}break;
case REGS_RegCodeX86_fpr3:{result = RADDBG_RegisterCode_X86_fpr3;}break;
case REGS_RegCodeX86_fpr4:{result = RADDBG_RegisterCode_X86_fpr4;}break;
case REGS_RegCodeX86_fpr5:{result = RADDBG_RegisterCode_X86_fpr5;}break;
case REGS_RegCodeX86_fpr6:{result = RADDBG_RegisterCode_X86_fpr6;}break;
case REGS_RegCodeX86_fpr7:{result = RADDBG_RegisterCode_X86_fpr7;}break;
case REGS_RegCodeX86_st0:{result = RADDBG_RegisterCode_X86_st0;}break;
case REGS_RegCodeX86_st1:{result = RADDBG_RegisterCode_X86_st1;}break;
case REGS_RegCodeX86_st2:{result = RADDBG_RegisterCode_X86_st2;}break;
case REGS_RegCodeX86_st3:{result = RADDBG_RegisterCode_X86_st3;}break;
case REGS_RegCodeX86_st4:{result = RADDBG_RegisterCode_X86_st4;}break;
case REGS_RegCodeX86_st5:{result = RADDBG_RegisterCode_X86_st5;}break;
case REGS_RegCodeX86_st6:{result = RADDBG_RegisterCode_X86_st6;}break;
case REGS_RegCodeX86_st7:{result = RADDBG_RegisterCode_X86_st7;}break;
case REGS_RegCodeX86_fcw:{result = RADDBG_RegisterCode_X86_fcw;}break;
case REGS_RegCodeX86_fsw:{result = RADDBG_RegisterCode_X86_fsw;}break;
case REGS_RegCodeX86_ftw:{result = RADDBG_RegisterCode_X86_ftw;}break;
case REGS_RegCodeX86_fop:{result = RADDBG_RegisterCode_X86_fop;}break;
case REGS_RegCodeX86_fcs:{result = RADDBG_RegisterCode_X86_fcs;}break;
case REGS_RegCodeX86_fds:{result = RADDBG_RegisterCode_X86_fds;}break;
case REGS_RegCodeX86_fip:{result = RADDBG_RegisterCode_X86_fip;}break;
case REGS_RegCodeX86_fdp:{result = RADDBG_RegisterCode_X86_fdp;}break;
case REGS_RegCodeX86_mxcsr:{result = RADDBG_RegisterCode_X86_mxcsr;}break;
case REGS_RegCodeX86_mxcsr_mask:{result = RADDBG_RegisterCode_X86_mxcsr_mask;}break;
case REGS_RegCodeX86_ss:{result = RADDBG_RegisterCode_X86_ss;}break;
case REGS_RegCodeX86_cs:{result = RADDBG_RegisterCode_X86_cs;}break;
case REGS_RegCodeX86_ds:{result = RADDBG_RegisterCode_X86_ds;}break;
case REGS_RegCodeX86_es:{result = RADDBG_RegisterCode_X86_es;}break;
case REGS_RegCodeX86_fs:{result = RADDBG_RegisterCode_X86_fs;}break;
case REGS_RegCodeX86_gs:{result = RADDBG_RegisterCode_X86_gs;}break;
case REGS_RegCodeX86_ymm0:{result = RADDBG_RegisterCode_X86_ymm0;}break;
case REGS_RegCodeX86_ymm1:{result = RADDBG_RegisterCode_X86_ymm1;}break;
case REGS_RegCodeX86_ymm2:{result = RADDBG_RegisterCode_X86_ymm2;}break;
case REGS_RegCodeX86_ymm3:{result = RADDBG_RegisterCode_X86_ymm3;}break;
case REGS_RegCodeX86_ymm4:{result = RADDBG_RegisterCode_X86_ymm4;}break;
case REGS_RegCodeX86_ymm5:{result = RADDBG_RegisterCode_X86_ymm5;}break;
case REGS_RegCodeX86_ymm6:{result = RADDBG_RegisterCode_X86_ymm6;}break;
case REGS_RegCodeX86_ymm7:{result = RADDBG_RegisterCode_X86_ymm7;}break;
}
}break;
}
return result;
}
internal REGS_RegCode regs_reg_code_from_arch_raddbg_code(Architecture arch, RADDBG_RegisterCode code)
{
REGS_RegCode result = 0;
switch(arch)
{
case Architecture_x64:
{
switch(code)
{
default:{}break;
case RADDBG_RegisterCode_X64_rax:{result = REGS_RegCodeX64_rax;}break;
case RADDBG_RegisterCode_X64_rcx:{result = REGS_RegCodeX64_rcx;}break;
case RADDBG_RegisterCode_X64_rdx:{result = REGS_RegCodeX64_rdx;}break;
case RADDBG_RegisterCode_X64_rbx:{result = REGS_RegCodeX64_rbx;}break;
case RADDBG_RegisterCode_X64_rsp:{result = REGS_RegCodeX64_rsp;}break;
case RADDBG_RegisterCode_X64_rbp:{result = REGS_RegCodeX64_rbp;}break;
case RADDBG_RegisterCode_X64_rsi:{result = REGS_RegCodeX64_rsi;}break;
case RADDBG_RegisterCode_X64_rdi:{result = REGS_RegCodeX64_rdi;}break;
case RADDBG_RegisterCode_X64_r8:{result = REGS_RegCodeX64_r8;}break;
case RADDBG_RegisterCode_X64_r9:{result = REGS_RegCodeX64_r9;}break;
case RADDBG_RegisterCode_X64_r10:{result = REGS_RegCodeX64_r10;}break;
case RADDBG_RegisterCode_X64_r11:{result = REGS_RegCodeX64_r11;}break;
case RADDBG_RegisterCode_X64_r12:{result = REGS_RegCodeX64_r12;}break;
case RADDBG_RegisterCode_X64_r13:{result = REGS_RegCodeX64_r13;}break;
case RADDBG_RegisterCode_X64_r14:{result = REGS_RegCodeX64_r14;}break;
case RADDBG_RegisterCode_X64_r15:{result = REGS_RegCodeX64_r15;}break;
case RADDBG_RegisterCode_X64_fsbase:{result = REGS_RegCodeX64_fsbase;}break;
case RADDBG_RegisterCode_X64_gsbase:{result = REGS_RegCodeX64_gsbase;}break;
case RADDBG_RegisterCode_X64_rip:{result = REGS_RegCodeX64_rip;}break;
case RADDBG_RegisterCode_X64_rflags:{result = REGS_RegCodeX64_rflags;}break;
case RADDBG_RegisterCode_X64_dr0:{result = REGS_RegCodeX64_dr0;}break;
case RADDBG_RegisterCode_X64_dr1:{result = REGS_RegCodeX64_dr1;}break;
case RADDBG_RegisterCode_X64_dr2:{result = REGS_RegCodeX64_dr2;}break;
case RADDBG_RegisterCode_X64_dr3:{result = REGS_RegCodeX64_dr3;}break;
case RADDBG_RegisterCode_X64_dr4:{result = REGS_RegCodeX64_dr4;}break;
case RADDBG_RegisterCode_X64_dr5:{result = REGS_RegCodeX64_dr5;}break;
case RADDBG_RegisterCode_X64_dr6:{result = REGS_RegCodeX64_dr6;}break;
case RADDBG_RegisterCode_X64_dr7:{result = REGS_RegCodeX64_dr7;}break;
case RADDBG_RegisterCode_X64_fpr0:{result = REGS_RegCodeX64_fpr0;}break;
case RADDBG_RegisterCode_X64_fpr1:{result = REGS_RegCodeX64_fpr1;}break;
case RADDBG_RegisterCode_X64_fpr2:{result = REGS_RegCodeX64_fpr2;}break;
case RADDBG_RegisterCode_X64_fpr3:{result = REGS_RegCodeX64_fpr3;}break;
case RADDBG_RegisterCode_X64_fpr4:{result = REGS_RegCodeX64_fpr4;}break;
case RADDBG_RegisterCode_X64_fpr5:{result = REGS_RegCodeX64_fpr5;}break;
case RADDBG_RegisterCode_X64_fpr6:{result = REGS_RegCodeX64_fpr6;}break;
case RADDBG_RegisterCode_X64_fpr7:{result = REGS_RegCodeX64_fpr7;}break;
case RADDBG_RegisterCode_X64_st0:{result = REGS_RegCodeX64_st0;}break;
case RADDBG_RegisterCode_X64_st1:{result = REGS_RegCodeX64_st1;}break;
case RADDBG_RegisterCode_X64_st2:{result = REGS_RegCodeX64_st2;}break;
case RADDBG_RegisterCode_X64_st3:{result = REGS_RegCodeX64_st3;}break;
case RADDBG_RegisterCode_X64_st4:{result = REGS_RegCodeX64_st4;}break;
case RADDBG_RegisterCode_X64_st5:{result = REGS_RegCodeX64_st5;}break;
case RADDBG_RegisterCode_X64_st6:{result = REGS_RegCodeX64_st6;}break;
case RADDBG_RegisterCode_X64_st7:{result = REGS_RegCodeX64_st7;}break;
case RADDBG_RegisterCode_X64_fcw:{result = REGS_RegCodeX64_fcw;}break;
case RADDBG_RegisterCode_X64_fsw:{result = REGS_RegCodeX64_fsw;}break;
case RADDBG_RegisterCode_X64_ftw:{result = REGS_RegCodeX64_ftw;}break;
case RADDBG_RegisterCode_X64_fop:{result = REGS_RegCodeX64_fop;}break;
case RADDBG_RegisterCode_X64_fcs:{result = REGS_RegCodeX64_fcs;}break;
case RADDBG_RegisterCode_X64_fds:{result = REGS_RegCodeX64_fds;}break;
case RADDBG_RegisterCode_X64_fip:{result = REGS_RegCodeX64_fip;}break;
case RADDBG_RegisterCode_X64_fdp:{result = REGS_RegCodeX64_fdp;}break;
case RADDBG_RegisterCode_X64_mxcsr:{result = REGS_RegCodeX64_mxcsr;}break;
case RADDBG_RegisterCode_X64_mxcsr_mask:{result = REGS_RegCodeX64_mxcsr_mask;}break;
case RADDBG_RegisterCode_X64_ss:{result = REGS_RegCodeX64_ss;}break;
case RADDBG_RegisterCode_X64_cs:{result = REGS_RegCodeX64_cs;}break;
case RADDBG_RegisterCode_X64_ds:{result = REGS_RegCodeX64_ds;}break;
case RADDBG_RegisterCode_X64_es:{result = REGS_RegCodeX64_es;}break;
case RADDBG_RegisterCode_X64_fs:{result = REGS_RegCodeX64_fs;}break;
case RADDBG_RegisterCode_X64_gs:{result = REGS_RegCodeX64_gs;}break;
case RADDBG_RegisterCode_X64_ymm0:{result = REGS_RegCodeX64_ymm0;}break;
case RADDBG_RegisterCode_X64_ymm1:{result = REGS_RegCodeX64_ymm1;}break;
case RADDBG_RegisterCode_X64_ymm2:{result = REGS_RegCodeX64_ymm2;}break;
case RADDBG_RegisterCode_X64_ymm3:{result = REGS_RegCodeX64_ymm3;}break;
case RADDBG_RegisterCode_X64_ymm4:{result = REGS_RegCodeX64_ymm4;}break;
case RADDBG_RegisterCode_X64_ymm5:{result = REGS_RegCodeX64_ymm5;}break;
case RADDBG_RegisterCode_X64_ymm6:{result = REGS_RegCodeX64_ymm6;}break;
case RADDBG_RegisterCode_X64_ymm7:{result = REGS_RegCodeX64_ymm7;}break;
case RADDBG_RegisterCode_X64_ymm8:{result = REGS_RegCodeX64_ymm8;}break;
case RADDBG_RegisterCode_X64_ymm9:{result = REGS_RegCodeX64_ymm9;}break;
case RADDBG_RegisterCode_X64_ymm10:{result = REGS_RegCodeX64_ymm10;}break;
case RADDBG_RegisterCode_X64_ymm11:{result = REGS_RegCodeX64_ymm11;}break;
case RADDBG_RegisterCode_X64_ymm12:{result = REGS_RegCodeX64_ymm12;}break;
case RADDBG_RegisterCode_X64_ymm13:{result = REGS_RegCodeX64_ymm13;}break;
case RADDBG_RegisterCode_X64_ymm14:{result = REGS_RegCodeX64_ymm14;}break;
case RADDBG_RegisterCode_X64_ymm15:{result = REGS_RegCodeX64_ymm15;}break;
}
}break;
case Architecture_x86:
{
switch(code)
{
default:{}break;
case RADDBG_RegisterCode_X86_eax:{result = REGS_RegCodeX86_eax;}break;
case RADDBG_RegisterCode_X86_ecx:{result = REGS_RegCodeX86_ecx;}break;
case RADDBG_RegisterCode_X86_edx:{result = REGS_RegCodeX86_edx;}break;
case RADDBG_RegisterCode_X86_ebx:{result = REGS_RegCodeX86_ebx;}break;
case RADDBG_RegisterCode_X86_esp:{result = REGS_RegCodeX86_esp;}break;
case RADDBG_RegisterCode_X86_ebp:{result = REGS_RegCodeX86_ebp;}break;
case RADDBG_RegisterCode_X86_esi:{result = REGS_RegCodeX86_esi;}break;
case RADDBG_RegisterCode_X86_edi:{result = REGS_RegCodeX86_edi;}break;
case RADDBG_RegisterCode_X86_fsbase:{result = REGS_RegCodeX86_fsbase;}break;
case RADDBG_RegisterCode_X86_gsbase:{result = REGS_RegCodeX86_gsbase;}break;
case RADDBG_RegisterCode_X86_eflags:{result = REGS_RegCodeX86_eflags;}break;
case RADDBG_RegisterCode_X86_eip:{result = REGS_RegCodeX86_eip;}break;
case RADDBG_RegisterCode_X86_dr0:{result = REGS_RegCodeX86_dr0;}break;
case RADDBG_RegisterCode_X86_dr1:{result = REGS_RegCodeX86_dr1;}break;
case RADDBG_RegisterCode_X86_dr2:{result = REGS_RegCodeX86_dr2;}break;
case RADDBG_RegisterCode_X86_dr3:{result = REGS_RegCodeX86_dr3;}break;
case RADDBG_RegisterCode_X86_dr4:{result = REGS_RegCodeX86_dr4;}break;
case RADDBG_RegisterCode_X86_dr5:{result = REGS_RegCodeX86_dr5;}break;
case RADDBG_RegisterCode_X86_dr6:{result = REGS_RegCodeX86_dr6;}break;
case RADDBG_RegisterCode_X86_dr7:{result = REGS_RegCodeX86_dr7;}break;
case RADDBG_RegisterCode_X86_fpr0:{result = REGS_RegCodeX86_fpr0;}break;
case RADDBG_RegisterCode_X86_fpr1:{result = REGS_RegCodeX86_fpr1;}break;
case RADDBG_RegisterCode_X86_fpr2:{result = REGS_RegCodeX86_fpr2;}break;
case RADDBG_RegisterCode_X86_fpr3:{result = REGS_RegCodeX86_fpr3;}break;
case RADDBG_RegisterCode_X86_fpr4:{result = REGS_RegCodeX86_fpr4;}break;
case RADDBG_RegisterCode_X86_fpr5:{result = REGS_RegCodeX86_fpr5;}break;
case RADDBG_RegisterCode_X86_fpr6:{result = REGS_RegCodeX86_fpr6;}break;
case RADDBG_RegisterCode_X86_fpr7:{result = REGS_RegCodeX86_fpr7;}break;
case RADDBG_RegisterCode_X86_st0:{result = REGS_RegCodeX86_st0;}break;
case RADDBG_RegisterCode_X86_st1:{result = REGS_RegCodeX86_st1;}break;
case RADDBG_RegisterCode_X86_st2:{result = REGS_RegCodeX86_st2;}break;
case RADDBG_RegisterCode_X86_st3:{result = REGS_RegCodeX86_st3;}break;
case RADDBG_RegisterCode_X86_st4:{result = REGS_RegCodeX86_st4;}break;
case RADDBG_RegisterCode_X86_st5:{result = REGS_RegCodeX86_st5;}break;
case RADDBG_RegisterCode_X86_st6:{result = REGS_RegCodeX86_st6;}break;
case RADDBG_RegisterCode_X86_st7:{result = REGS_RegCodeX86_st7;}break;
case RADDBG_RegisterCode_X86_fcw:{result = REGS_RegCodeX86_fcw;}break;
case RADDBG_RegisterCode_X86_fsw:{result = REGS_RegCodeX86_fsw;}break;
case RADDBG_RegisterCode_X86_ftw:{result = REGS_RegCodeX86_ftw;}break;
case RADDBG_RegisterCode_X86_fop:{result = REGS_RegCodeX86_fop;}break;
case RADDBG_RegisterCode_X86_fcs:{result = REGS_RegCodeX86_fcs;}break;
case RADDBG_RegisterCode_X86_fds:{result = REGS_RegCodeX86_fds;}break;
case RADDBG_RegisterCode_X86_fip:{result = REGS_RegCodeX86_fip;}break;
case RADDBG_RegisterCode_X86_fdp:{result = REGS_RegCodeX86_fdp;}break;
case RADDBG_RegisterCode_X86_mxcsr:{result = REGS_RegCodeX86_mxcsr;}break;
case RADDBG_RegisterCode_X86_mxcsr_mask:{result = REGS_RegCodeX86_mxcsr_mask;}break;
case RADDBG_RegisterCode_X86_ss:{result = REGS_RegCodeX86_ss;}break;
case RADDBG_RegisterCode_X86_cs:{result = REGS_RegCodeX86_cs;}break;
case RADDBG_RegisterCode_X86_ds:{result = REGS_RegCodeX86_ds;}break;
case RADDBG_RegisterCode_X86_es:{result = REGS_RegCodeX86_es;}break;
case RADDBG_RegisterCode_X86_fs:{result = REGS_RegCodeX86_fs;}break;
case RADDBG_RegisterCode_X86_gs:{result = REGS_RegCodeX86_gs;}break;
case RADDBG_RegisterCode_X86_ymm0:{result = REGS_RegCodeX86_ymm0;}break;
case RADDBG_RegisterCode_X86_ymm1:{result = REGS_RegCodeX86_ymm1;}break;
case RADDBG_RegisterCode_X86_ymm2:{result = REGS_RegCodeX86_ymm2;}break;
case RADDBG_RegisterCode_X86_ymm3:{result = REGS_RegCodeX86_ymm3;}break;
case RADDBG_RegisterCode_X86_ymm4:{result = REGS_RegCodeX86_ymm4;}break;
case RADDBG_RegisterCode_X86_ymm5:{result = REGS_RegCodeX86_ymm5;}break;
case RADDBG_RegisterCode_X86_ymm6:{result = REGS_RegCodeX86_ymm6;}break;
case RADDBG_RegisterCode_X86_ymm7:{result = REGS_RegCodeX86_ymm7;}break;
}
}break;
}
return result;
}

#endif
