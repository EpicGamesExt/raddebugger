// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#if !defined(ZYDIS_H)
#include "third_party/zydis/zydis.h"
#include "third_party/zydis/zydis.c"
#endif

internal DASM_Inst
x64_dasm_inst_from_code(Arena *arena, U64 vaddr, String8 code, DASM_Syntax syntax)
{
  DASM_Inst inst = {0};
  
  //////////////////////////////
  //- rjf: determine zydis formatter style
  //
  ZydisFormatterStyle style = ZYDIS_FORMATTER_STYLE_INTEL;
  switch(syntax)
  {
    default:{}break;
    case DASM_Syntax_Intel:{style = ZYDIS_FORMATTER_STYLE_INTEL;}break;
    case DASM_Syntax_ATT:  {style = ZYDIS_FORMATTER_STYLE_ATT;}break;
  }
  
  //////////////////////////////
  //- rjf: disassemble one instruction
  //
  ZydisDisassembledInstruction zinst = {0};
  ZyanStatus status = ZydisDisassemble(ZYDIS_MACHINE_MODE_LONG_64, vaddr, code.str, code.size, &zinst, style);
  
  //////////////////////////////
  //- rjf: analyze
  //
  DASM_InstFlags flags = 0;
  U64 dst_vaddr = 0;
  X64_RegCode src_reg_code = 0;
  X64_RegCode dst_reg_code = 0;
  S64 src_reg_off = 0;
  S64 dst_reg_off = 0;
  {
    //- rjf: unpack operands
    ZydisDecodedOperand *first_visible_op = (zinst.info.operand_count_visible > 0 ? &zinst.operands[0] : 0);
    ZydisDecodedOperand *first_op = (zinst.info.operand_count > 0 ? &zinst.operands[0] : 0);
    ZydisDecodedOperand *second_op = (zinst.info.operand_count > 1 ? &zinst.operands[1] : 0);
    
    //- rjf: extract registers from operands
    ZydisRegister zydis_reg_codes[2] = {0};
    if(first_op != 0 && first_op->type == ZYDIS_OPERAND_TYPE_REGISTER)
    {
      zydis_reg_codes[0] = first_op->reg.value;
    }
    if(first_op != 0 && first_op->type == ZYDIS_OPERAND_TYPE_MEMORY)
    {
      zydis_reg_codes[0] = first_op->mem.base;
    }
    if(second_op != 0 && second_op->type == ZYDIS_OPERAND_TYPE_REGISTER)
    {
      zydis_reg_codes[1] = second_op->reg.value;
    }
    if(second_op != 0 && second_op->type == ZYDIS_OPERAND_TYPE_MEMORY)
    {
      zydis_reg_codes[1] = second_op->mem.base;
    }
    
    //- rjf: map zydis registers -> x64 layer
    X64_RegCode x64_reg_codes[2] = {0};
    for EachElement(reg_code_idx, zydis_reg_codes)
    {
      ZydisRegister src_reg = zydis_reg_codes[reg_code_idx];
      X64_RegCode dst_reg = 0;
      switch(src_reg)
      {
        default:{}break;
        case ZYDIS_REGISTER_AL:{dst_reg = X64_RegCode_al;}break;
        case ZYDIS_REGISTER_CL:{dst_reg = X64_RegCode_cl;}break;
        case ZYDIS_REGISTER_DL:{dst_reg = X64_RegCode_dl;}break;
        case ZYDIS_REGISTER_BL:{dst_reg = X64_RegCode_bl;}break;
        case ZYDIS_REGISTER_AH:{dst_reg = X64_RegCode_ah;}break;
        case ZYDIS_REGISTER_CH:{dst_reg = X64_RegCode_ch;}break;
        case ZYDIS_REGISTER_DH:{dst_reg = X64_RegCode_dh;}break;
        case ZYDIS_REGISTER_BH:{dst_reg = X64_RegCode_bh;}break;
        case ZYDIS_REGISTER_SPL:{dst_reg = X64_RegCode_spl;}break;
        case ZYDIS_REGISTER_BPL:{dst_reg = X64_RegCode_bpl;}break;
        case ZYDIS_REGISTER_SIL:{dst_reg = X64_RegCode_sil;}break;
        case ZYDIS_REGISTER_DIL:{dst_reg = X64_RegCode_dil;}break;
        case ZYDIS_REGISTER_R8B:{dst_reg = X64_RegCode_r8b;}break;
        case ZYDIS_REGISTER_R9B:{dst_reg = X64_RegCode_r9b;}break;
        case ZYDIS_REGISTER_R10B:{dst_reg = X64_RegCode_r10b;}break;
        case ZYDIS_REGISTER_R11B:{dst_reg = X64_RegCode_r11b;}break;
        case ZYDIS_REGISTER_R12B:{dst_reg = X64_RegCode_r12b;}break;
        case ZYDIS_REGISTER_R13B:{dst_reg = X64_RegCode_r13b;}break;
        case ZYDIS_REGISTER_R14B:{dst_reg = X64_RegCode_r14b;}break;
        case ZYDIS_REGISTER_R15B:{dst_reg = X64_RegCode_r15b;}break;
        case ZYDIS_REGISTER_AX:{dst_reg = X64_RegCode_ax;}break;
        case ZYDIS_REGISTER_CX:{dst_reg = X64_RegCode_cx;}break;
        case ZYDIS_REGISTER_DX:{dst_reg = X64_RegCode_dx;}break;
        case ZYDIS_REGISTER_BX:{dst_reg = X64_RegCode_bx;}break;
        case ZYDIS_REGISTER_SP:{dst_reg = X64_RegCode_sp;}break;
        case ZYDIS_REGISTER_BP:{dst_reg = X64_RegCode_bp;}break;
        case ZYDIS_REGISTER_SI:{dst_reg = X64_RegCode_si;}break;
        case ZYDIS_REGISTER_DI:{dst_reg = X64_RegCode_di;}break;
        case ZYDIS_REGISTER_R8W:{dst_reg = X64_RegCode_r8w;}break;
        case ZYDIS_REGISTER_R9W:{dst_reg = X64_RegCode_r9w;}break;
        case ZYDIS_REGISTER_R10W:{dst_reg = X64_RegCode_r10w;}break;
        case ZYDIS_REGISTER_R11W:{dst_reg = X64_RegCode_r11w;}break;
        case ZYDIS_REGISTER_R12W:{dst_reg = X64_RegCode_r12w;}break;
        case ZYDIS_REGISTER_R13W:{dst_reg = X64_RegCode_r13w;}break;
        case ZYDIS_REGISTER_R14W:{dst_reg = X64_RegCode_r14w;}break;
        case ZYDIS_REGISTER_R15W:{dst_reg = X64_RegCode_r15w;}break;
        case ZYDIS_REGISTER_EAX:{dst_reg = X64_RegCode_eax;}break;
        case ZYDIS_REGISTER_ECX:{dst_reg = X64_RegCode_ecx;}break;
        case ZYDIS_REGISTER_EDX:{dst_reg = X64_RegCode_edx;}break;
        case ZYDIS_REGISTER_EBX:{dst_reg = X64_RegCode_ebx;}break;
        case ZYDIS_REGISTER_ESP:{dst_reg = X64_RegCode_esp;}break;
        case ZYDIS_REGISTER_EBP:{dst_reg = X64_RegCode_ebp;}break;
        case ZYDIS_REGISTER_ESI:{dst_reg = X64_RegCode_esi;}break;
        case ZYDIS_REGISTER_EDI:{dst_reg = X64_RegCode_edi;}break;
        case ZYDIS_REGISTER_R8D:{dst_reg = X64_RegCode_r8d;}break;
        case ZYDIS_REGISTER_R9D:{dst_reg = X64_RegCode_r9d;}break;
        case ZYDIS_REGISTER_R10D:{dst_reg = X64_RegCode_r10d;}break;
        case ZYDIS_REGISTER_R11D:{dst_reg = X64_RegCode_r11d;}break;
        case ZYDIS_REGISTER_R12D:{dst_reg = X64_RegCode_r12d;}break;
        case ZYDIS_REGISTER_R13D:{dst_reg = X64_RegCode_r13d;}break;
        case ZYDIS_REGISTER_R14D:{dst_reg = X64_RegCode_r14d;}break;
        case ZYDIS_REGISTER_R15D:{dst_reg = X64_RegCode_r15d;}break;
        case ZYDIS_REGISTER_RAX:{dst_reg = X64_RegCode_rax;}break;
        case ZYDIS_REGISTER_RCX:{dst_reg = X64_RegCode_rcx;}break;
        case ZYDIS_REGISTER_RDX:{dst_reg = X64_RegCode_rdx;}break;
        case ZYDIS_REGISTER_RBX:{dst_reg = X64_RegCode_rbx;}break;
        case ZYDIS_REGISTER_RSP:{dst_reg = X64_RegCode_rsp;}break;
        case ZYDIS_REGISTER_RBP:{dst_reg = X64_RegCode_rbp;}break;
        case ZYDIS_REGISTER_RSI:{dst_reg = X64_RegCode_rsi;}break;
        case ZYDIS_REGISTER_RDI:{dst_reg = X64_RegCode_rdi;}break;
        case ZYDIS_REGISTER_R8:{dst_reg = X64_RegCode_r8;}break;
        case ZYDIS_REGISTER_R9:{dst_reg = X64_RegCode_r9;}break;
        case ZYDIS_REGISTER_R10:{dst_reg = X64_RegCode_r10;}break;
        case ZYDIS_REGISTER_R11:{dst_reg = X64_RegCode_r11;}break;
        case ZYDIS_REGISTER_R12:{dst_reg = X64_RegCode_r12;}break;
        case ZYDIS_REGISTER_R13:{dst_reg = X64_RegCode_r13;}break;
        case ZYDIS_REGISTER_R14:{dst_reg = X64_RegCode_r14;}break;
        case ZYDIS_REGISTER_R15:{dst_reg = X64_RegCode_r15;}break;
        case ZYDIS_REGISTER_ST0:{dst_reg = X64_RegCode_st0;}break;
        case ZYDIS_REGISTER_ST1:{dst_reg = X64_RegCode_st1;}break;
        case ZYDIS_REGISTER_ST2:{dst_reg = X64_RegCode_st2;}break;
        case ZYDIS_REGISTER_ST3:{dst_reg = X64_RegCode_st3;}break;
        case ZYDIS_REGISTER_ST4:{dst_reg = X64_RegCode_st4;}break;
        case ZYDIS_REGISTER_ST5:{dst_reg = X64_RegCode_st5;}break;
        case ZYDIS_REGISTER_ST6:{dst_reg = X64_RegCode_st6;}break;
        case ZYDIS_REGISTER_ST7:{dst_reg = X64_RegCode_st7;}break;
        case ZYDIS_REGISTER_MM0:{dst_reg = X64_RegCode_mm0;}break;
        case ZYDIS_REGISTER_MM1:{dst_reg = X64_RegCode_mm1;}break;
        case ZYDIS_REGISTER_MM2:{dst_reg = X64_RegCode_mm2;}break;
        case ZYDIS_REGISTER_MM3:{dst_reg = X64_RegCode_mm3;}break;
        case ZYDIS_REGISTER_MM4:{dst_reg = X64_RegCode_mm4;}break;
        case ZYDIS_REGISTER_MM5:{dst_reg = X64_RegCode_mm5;}break;
        case ZYDIS_REGISTER_MM6:{dst_reg = X64_RegCode_mm6;}break;
        case ZYDIS_REGISTER_MM7:{dst_reg = X64_RegCode_mm7;}break;
        case ZYDIS_REGISTER_XMM0:{dst_reg = X64_RegCode_xmm0;}break;
        case ZYDIS_REGISTER_XMM1:{dst_reg = X64_RegCode_xmm1;}break;
        case ZYDIS_REGISTER_XMM2:{dst_reg = X64_RegCode_xmm2;}break;
        case ZYDIS_REGISTER_XMM3:{dst_reg = X64_RegCode_xmm3;}break;
        case ZYDIS_REGISTER_XMM4:{dst_reg = X64_RegCode_xmm4;}break;
        case ZYDIS_REGISTER_XMM5:{dst_reg = X64_RegCode_xmm5;}break;
        case ZYDIS_REGISTER_XMM6:{dst_reg = X64_RegCode_xmm6;}break;
        case ZYDIS_REGISTER_XMM7:{dst_reg = X64_RegCode_xmm7;}break;
        case ZYDIS_REGISTER_XMM8:{dst_reg = X64_RegCode_xmm8;}break;
        case ZYDIS_REGISTER_XMM9:{dst_reg = X64_RegCode_xmm9;}break;
        case ZYDIS_REGISTER_XMM10:{dst_reg = X64_RegCode_xmm10;}break;
        case ZYDIS_REGISTER_XMM11:{dst_reg = X64_RegCode_xmm11;}break;
        case ZYDIS_REGISTER_XMM12:{dst_reg = X64_RegCode_xmm12;}break;
        case ZYDIS_REGISTER_XMM13:{dst_reg = X64_RegCode_xmm13;}break;
        case ZYDIS_REGISTER_XMM14:{dst_reg = X64_RegCode_xmm14;}break;
        case ZYDIS_REGISTER_XMM15:{dst_reg = X64_RegCode_xmm15;}break;
        case ZYDIS_REGISTER_YMM0:{dst_reg = X64_RegCode_ymm0;}break;
        case ZYDIS_REGISTER_YMM1:{dst_reg = X64_RegCode_ymm1;}break;
        case ZYDIS_REGISTER_YMM2:{dst_reg = X64_RegCode_ymm2;}break;
        case ZYDIS_REGISTER_YMM3:{dst_reg = X64_RegCode_ymm3;}break;
        case ZYDIS_REGISTER_YMM4:{dst_reg = X64_RegCode_ymm4;}break;
        case ZYDIS_REGISTER_YMM5:{dst_reg = X64_RegCode_ymm5;}break;
        case ZYDIS_REGISTER_YMM6:{dst_reg = X64_RegCode_ymm6;}break;
        case ZYDIS_REGISTER_YMM7:{dst_reg = X64_RegCode_ymm7;}break;
        case ZYDIS_REGISTER_YMM8:{dst_reg = X64_RegCode_ymm8;}break;
        case ZYDIS_REGISTER_YMM9:{dst_reg = X64_RegCode_ymm9;}break;
        case ZYDIS_REGISTER_YMM10:{dst_reg = X64_RegCode_ymm10;}break;
        case ZYDIS_REGISTER_YMM11:{dst_reg = X64_RegCode_ymm11;}break;
        case ZYDIS_REGISTER_YMM12:{dst_reg = X64_RegCode_ymm12;}break;
        case ZYDIS_REGISTER_YMM13:{dst_reg = X64_RegCode_ymm13;}break;
        case ZYDIS_REGISTER_YMM14:{dst_reg = X64_RegCode_ymm14;}break;
        case ZYDIS_REGISTER_YMM15:{dst_reg = X64_RegCode_ymm15;}break;
        case ZYDIS_REGISTER_ZMM0:{dst_reg = X64_RegCode_zmm0;}break;
        case ZYDIS_REGISTER_ZMM1:{dst_reg = X64_RegCode_zmm1;}break;
        case ZYDIS_REGISTER_ZMM2:{dst_reg = X64_RegCode_zmm2;}break;
        case ZYDIS_REGISTER_ZMM3:{dst_reg = X64_RegCode_zmm3;}break;
        case ZYDIS_REGISTER_ZMM4:{dst_reg = X64_RegCode_zmm4;}break;
        case ZYDIS_REGISTER_ZMM5:{dst_reg = X64_RegCode_zmm5;}break;
        case ZYDIS_REGISTER_ZMM6:{dst_reg = X64_RegCode_zmm6;}break;
        case ZYDIS_REGISTER_ZMM7:{dst_reg = X64_RegCode_zmm7;}break;
        case ZYDIS_REGISTER_ZMM8:{dst_reg = X64_RegCode_zmm8;}break;
        case ZYDIS_REGISTER_ZMM9:{dst_reg = X64_RegCode_zmm9;}break;
        case ZYDIS_REGISTER_ZMM10:{dst_reg = X64_RegCode_zmm10;}break;
        case ZYDIS_REGISTER_ZMM11:{dst_reg = X64_RegCode_zmm11;}break;
        case ZYDIS_REGISTER_ZMM12:{dst_reg = X64_RegCode_zmm12;}break;
        case ZYDIS_REGISTER_ZMM13:{dst_reg = X64_RegCode_zmm13;}break;
        case ZYDIS_REGISTER_ZMM14:{dst_reg = X64_RegCode_zmm14;}break;
        case ZYDIS_REGISTER_ZMM15:{dst_reg = X64_RegCode_zmm15;}break;
        case ZYDIS_REGISTER_ZMM16:{dst_reg = X64_RegCode_zmm16;}break;
        case ZYDIS_REGISTER_ZMM17:{dst_reg = X64_RegCode_zmm17;}break;
        case ZYDIS_REGISTER_ZMM18:{dst_reg = X64_RegCode_zmm18;}break;
        case ZYDIS_REGISTER_ZMM19:{dst_reg = X64_RegCode_zmm19;}break;
        case ZYDIS_REGISTER_ZMM20:{dst_reg = X64_RegCode_zmm20;}break;
        case ZYDIS_REGISTER_ZMM21:{dst_reg = X64_RegCode_zmm21;}break;
        case ZYDIS_REGISTER_ZMM22:{dst_reg = X64_RegCode_zmm22;}break;
        case ZYDIS_REGISTER_ZMM23:{dst_reg = X64_RegCode_zmm23;}break;
        case ZYDIS_REGISTER_ZMM24:{dst_reg = X64_RegCode_zmm24;}break;
        case ZYDIS_REGISTER_ZMM25:{dst_reg = X64_RegCode_zmm25;}break;
        case ZYDIS_REGISTER_ZMM26:{dst_reg = X64_RegCode_zmm26;}break;
        case ZYDIS_REGISTER_ZMM27:{dst_reg = X64_RegCode_zmm27;}break;
        case ZYDIS_REGISTER_ZMM28:{dst_reg = X64_RegCode_zmm28;}break;
        case ZYDIS_REGISTER_ZMM29:{dst_reg = X64_RegCode_zmm29;}break;
        case ZYDIS_REGISTER_ZMM30:{dst_reg = X64_RegCode_zmm30;}break;
        case ZYDIS_REGISTER_ZMM31:{dst_reg = X64_RegCode_zmm31;}break;
        case ZYDIS_REGISTER_EFLAGS:{dst_reg = X64_RegCode_eflags;}break;
        case ZYDIS_REGISTER_RFLAGS:{dst_reg = X64_RegCode_rflags;}break;
        case ZYDIS_REGISTER_IP:{dst_reg = X64_RegCode_ip;}break;
        case ZYDIS_REGISTER_RIP:{dst_reg = X64_RegCode_rip;}break;
        case ZYDIS_REGISTER_ES:{dst_reg = X64_RegCode_es;}break;
        case ZYDIS_REGISTER_CS:{dst_reg = X64_RegCode_cs;}break;
        case ZYDIS_REGISTER_SS:{dst_reg = X64_RegCode_ss;}break;
        case ZYDIS_REGISTER_DS:{dst_reg = X64_RegCode_ds;}break;
        case ZYDIS_REGISTER_FS:{dst_reg = X64_RegCode_fs;}break;
        case ZYDIS_REGISTER_GS:{dst_reg = X64_RegCode_gs;}break;
        case ZYDIS_REGISTER_DR0:{dst_reg = X64_RegCode_dr0;}break;
        case ZYDIS_REGISTER_DR1:{dst_reg = X64_RegCode_dr1;}break;
        case ZYDIS_REGISTER_DR2:{dst_reg = X64_RegCode_dr2;}break;
        case ZYDIS_REGISTER_DR3:{dst_reg = X64_RegCode_dr3;}break;
        case ZYDIS_REGISTER_DR4:{dst_reg = X64_RegCode_dr4;}break;
        case ZYDIS_REGISTER_DR5:{dst_reg = X64_RegCode_dr5;}break;
        case ZYDIS_REGISTER_DR6:{dst_reg = X64_RegCode_dr6;}break;
        case ZYDIS_REGISTER_DR7:{dst_reg = X64_RegCode_dr7;}break;
        case ZYDIS_REGISTER_K0:{dst_reg = X64_RegCode_k0;}break;
        case ZYDIS_REGISTER_K1:{dst_reg = X64_RegCode_k1;}break;
        case ZYDIS_REGISTER_K2:{dst_reg = X64_RegCode_k2;}break;
        case ZYDIS_REGISTER_K3:{dst_reg = X64_RegCode_k3;}break;
        case ZYDIS_REGISTER_K4:{dst_reg = X64_RegCode_k4;}break;
        case ZYDIS_REGISTER_K5:{dst_reg = X64_RegCode_k5;}break;
        case ZYDIS_REGISTER_K6:{dst_reg = X64_RegCode_k6;}break;
        case ZYDIS_REGISTER_K7:{dst_reg = X64_RegCode_k7;}break;
        case ZYDIS_REGISTER_MXCSR:{dst_reg = X64_RegCode_mxcsr;}break;
        
        // rjf: currently unsupported by debugger:
#if 0
        case ZYDIS_REGISTER_EIP:{dst_reg = X64_RegCode_eip;}break;
        case ZYDIS_REGISTER_X87CONTROL:{dst_reg = X64_RegCode_x87control;}break;
        case ZYDIS_REGISTER_X87STATUS:{dst_reg = X64_RegCode_x87status;}break;
        case ZYDIS_REGISTER_X87TAG:{dst_reg = X64_RegCode_x87tag;}break;
        case ZYDIS_REGISTER_XMM16:{dst_reg = X64_RegCode_xmm16;}break;
        case ZYDIS_REGISTER_XMM17:{dst_reg = X64_RegCode_xmm17;}break;
        case ZYDIS_REGISTER_XMM18:{dst_reg = X64_RegCode_xmm18;}break;
        case ZYDIS_REGISTER_XMM19:{dst_reg = X64_RegCode_xmm19;}break;
        case ZYDIS_REGISTER_XMM20:{dst_reg = X64_RegCode_xmm20;}break;
        case ZYDIS_REGISTER_XMM21:{dst_reg = X64_RegCode_xmm21;}break;
        case ZYDIS_REGISTER_XMM22:{dst_reg = X64_RegCode_xmm22;}break;
        case ZYDIS_REGISTER_XMM23:{dst_reg = X64_RegCode_xmm23;}break;
        case ZYDIS_REGISTER_XMM24:{dst_reg = X64_RegCode_xmm24;}break;
        case ZYDIS_REGISTER_XMM25:{dst_reg = X64_RegCode_xmm25;}break;
        case ZYDIS_REGISTER_XMM26:{dst_reg = X64_RegCode_xmm26;}break;
        case ZYDIS_REGISTER_XMM27:{dst_reg = X64_RegCode_xmm27;}break;
        case ZYDIS_REGISTER_XMM28:{dst_reg = X64_RegCode_xmm28;}break;
        case ZYDIS_REGISTER_XMM29:{dst_reg = X64_RegCode_xmm29;}break;
        case ZYDIS_REGISTER_XMM30:{dst_reg = X64_RegCode_xmm30;}break;
        case ZYDIS_REGISTER_XMM31:{dst_reg = X64_RegCode_xmm31;}break;
        case ZYDIS_REGISTER_YMM16:{dst_reg = X64_RegCode_ymm16;}break;
        case ZYDIS_REGISTER_YMM17:{dst_reg = X64_RegCode_ymm17;}break;
        case ZYDIS_REGISTER_YMM18:{dst_reg = X64_RegCode_ymm18;}break;
        case ZYDIS_REGISTER_YMM19:{dst_reg = X64_RegCode_ymm19;}break;
        case ZYDIS_REGISTER_YMM20:{dst_reg = X64_RegCode_ymm20;}break;
        case ZYDIS_REGISTER_YMM21:{dst_reg = X64_RegCode_ymm21;}break;
        case ZYDIS_REGISTER_YMM22:{dst_reg = X64_RegCode_ymm22;}break;
        case ZYDIS_REGISTER_YMM23:{dst_reg = X64_RegCode_ymm23;}break;
        case ZYDIS_REGISTER_YMM24:{dst_reg = X64_RegCode_ymm24;}break;
        case ZYDIS_REGISTER_YMM25:{dst_reg = X64_RegCode_ymm25;}break;
        case ZYDIS_REGISTER_YMM26:{dst_reg = X64_RegCode_ymm26;}break;
        case ZYDIS_REGISTER_YMM27:{dst_reg = X64_RegCode_ymm27;}break;
        case ZYDIS_REGISTER_YMM28:{dst_reg = X64_RegCode_ymm28;}break;
        case ZYDIS_REGISTER_YMM29:{dst_reg = X64_RegCode_ymm29;}break;
        case ZYDIS_REGISTER_YMM30:{dst_reg = X64_RegCode_ymm30;}break;
        case ZYDIS_REGISTER_YMM31:{dst_reg = X64_RegCode_ymm31;}break;
        case ZYDIS_REGISTER_TMM0:{dst_reg = X64_RegCode_tmm0;}break;
        case ZYDIS_REGISTER_TMM1:{dst_reg = X64_RegCode_tmm1;}break;
        case ZYDIS_REGISTER_TMM2:{dst_reg = X64_RegCode_tmm2;}break;
        case ZYDIS_REGISTER_TMM3:{dst_reg = X64_RegCode_tmm3;}break;
        case ZYDIS_REGISTER_TMM4:{dst_reg = X64_RegCode_tmm4;}break;
        case ZYDIS_REGISTER_TMM5:{dst_reg = X64_RegCode_tmm5;}break;
        case ZYDIS_REGISTER_TMM6:{dst_reg = X64_RegCode_tmm6;}break;
        case ZYDIS_REGISTER_TMM7:{dst_reg = X64_RegCode_tmm7;}break;
        case ZYDIS_REGISTER_FLAGS:{dst_reg = X64_RegCode_flags;}break;
        case ZYDIS_REGISTER_GDTR:{dst_reg = X64_RegCode_gdtr;}break;
        case ZYDIS_REGISTER_LDTR:{dst_reg = X64_RegCode_ldtr;}break;
        case ZYDIS_REGISTER_IDTR:{dst_reg = X64_RegCode_idtr;}break;
        case ZYDIS_REGISTER_TR:{dst_reg = X64_RegCode_tr;}break;
        case ZYDIS_REGISTER_TR0:{dst_reg = X64_RegCode_tr0;}break;
        case ZYDIS_REGISTER_TR1:{dst_reg = X64_RegCode_tr1;}break;
        case ZYDIS_REGISTER_TR2:{dst_reg = X64_RegCode_tr2;}break;
        case ZYDIS_REGISTER_TR3:{dst_reg = X64_RegCode_tr3;}break;
        case ZYDIS_REGISTER_TR4:{dst_reg = X64_RegCode_tr4;}break;
        case ZYDIS_REGISTER_TR5:{dst_reg = X64_RegCode_tr5;}break;
        case ZYDIS_REGISTER_TR6:{dst_reg = X64_RegCode_tr6;}break;
        case ZYDIS_REGISTER_TR7:{dst_reg = X64_RegCode_tr7;}break;
        case ZYDIS_REGISTER_CR0:{dst_reg = X64_RegCode_cr0;}break;
        case ZYDIS_REGISTER_CR1:{dst_reg = X64_RegCode_cr1;}break;
        case ZYDIS_REGISTER_CR2:{dst_reg = X64_RegCode_cr2;}break;
        case ZYDIS_REGISTER_CR3:{dst_reg = X64_RegCode_cr3;}break;
        case ZYDIS_REGISTER_CR4:{dst_reg = X64_RegCode_cr4;}break;
        case ZYDIS_REGISTER_CR5:{dst_reg = X64_RegCode_cr5;}break;
        case ZYDIS_REGISTER_CR6:{dst_reg = X64_RegCode_cr6;}break;
        case ZYDIS_REGISTER_CR7:{dst_reg = X64_RegCode_cr7;}break;
        case ZYDIS_REGISTER_CR8:{dst_reg = X64_RegCode_cr8;}break;
        case ZYDIS_REGISTER_CR9:{dst_reg = X64_RegCode_cr9;}break;
        case ZYDIS_REGISTER_CR10:{dst_reg = X64_RegCode_cr10;}break;
        case ZYDIS_REGISTER_CR11:{dst_reg = X64_RegCode_cr11;}break;
        case ZYDIS_REGISTER_CR12:{dst_reg = X64_RegCode_cr12;}break;
        case ZYDIS_REGISTER_CR13:{dst_reg = X64_RegCode_cr13;}break;
        case ZYDIS_REGISTER_CR14:{dst_reg = X64_RegCode_cr14;}break;
        case ZYDIS_REGISTER_CR15:{dst_reg = X64_RegCode_cr15;}break;
        case ZYDIS_REGISTER_DR8:{dst_reg = X64_RegCode_dr8;}break;
        case ZYDIS_REGISTER_DR9:{dst_reg = X64_RegCode_dr9;}break;
        case ZYDIS_REGISTER_DR10:{dst_reg = X64_RegCode_dr10;}break;
        case ZYDIS_REGISTER_DR11:{dst_reg = X64_RegCode_dr11;}break;
        case ZYDIS_REGISTER_DR12:{dst_reg = X64_RegCode_dr12;}break;
        case ZYDIS_REGISTER_DR13:{dst_reg = X64_RegCode_dr13;}break;
        case ZYDIS_REGISTER_DR14:{dst_reg = X64_RegCode_dr14;}break;
        case ZYDIS_REGISTER_DR15:{dst_reg = X64_RegCode_dr15;}break;
        case ZYDIS_REGISTER_BND0:{dst_reg = X64_RegCode_bnd0;}break;
        case ZYDIS_REGISTER_BND1:{dst_reg = X64_RegCode_bnd1;}break;
        case ZYDIS_REGISTER_BND2:{dst_reg = X64_RegCode_bnd2;}break;
        case ZYDIS_REGISTER_BND3:{dst_reg = X64_RegCode_bnd3;}break;
        case ZYDIS_REGISTER_BNDCFG:{dst_reg = X64_RegCode_bndcfg;}break;
        case ZYDIS_REGISTER_BNDSTATUS:{dst_reg = X64_RegCode_bndstatus;}break;
        case ZYDIS_REGISTER_PKRU:{dst_reg = X64_RegCode_pkru;}break;
        case ZYDIS_REGISTER_XCR0:{dst_reg = X64_RegCode_xcr0;}break;
        case ZYDIS_REGISTER_UIF:{dst_reg = X64_RegCode_uif;}break;
#endif
      }
      x64_reg_codes[reg_code_idx] = dst_reg;
    }
    
    //- rjf: extract info from operands
    if(first_op != 0 &&
       first_op->actions & ZYDIS_OPERAND_ACTION_WRITE)
    {
      dst_reg_code = x64_reg_codes[0];
      if(first_op->type == ZYDIS_OPERAND_TYPE_MEMORY)
      {
        dst_reg_off = first_op->mem.disp.value;
      }
    }
    if(second_op != 0 &&
       second_op->actions & ZYDIS_OPERAND_ACTION_READ)
    {
      src_reg_code = x64_reg_codes[0];
      if(second_op->type == ZYDIS_OPERAND_TYPE_MEMORY)
      {
        src_reg_off = second_op->mem.disp.value;
      }
    }
    if(first_visible_op != 0 && 
       (first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM8 ||
        first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM16 ||
        first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM32 ||
        first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM64 ||
        first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM16_32_64 ||
        first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM32_32_64 ||
        first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM16_32_32))
    {
      ZydisCalcAbsoluteAddress(&zinst.info, first_visible_op, vaddr, &dst_vaddr);
    }
    if(first_op != 0 && second_op != 0 && first_op->type == ZYDIS_OPERAND_TYPE_REGISTER &&
       (first_op->reg.value == ZYDIS_REGISTER_RSP ||
        first_op->reg.value == ZYDIS_REGISTER_ESP ||
        first_op->reg.value == ZYDIS_REGISTER_SP))
    {
      flags |= DASM_InstFlag_ChangesStackPointer;
      if(second_op->type != ZYDIS_OPERAND_TYPE_IMMEDIATE)
      {
        flags |= DASM_InstFlag_ChangesStackPointerVariably;
      }
    }
    if(zinst.info.attributes & (ZYDIS_ATTRIB_HAS_REP|
                                ZYDIS_ATTRIB_HAS_REPE|
                                ZYDIS_ATTRIB_HAS_REPZ|
                                ZYDIS_ATTRIB_HAS_REPNZ|
                                ZYDIS_ATTRIB_HAS_REPNE))
    {
      flags |= DASM_InstFlag_Repeats;
    }
    switch(zinst.info.mnemonic)
    {
      case ZYDIS_MNEMONIC_CALL:
      {
        flags |= DASM_InstFlag_Call;
      }break;
      
      case ZYDIS_MNEMONIC_JB:
      case ZYDIS_MNEMONIC_JBE:
      case ZYDIS_MNEMONIC_JCXZ:
      case ZYDIS_MNEMONIC_JECXZ:
      case ZYDIS_MNEMONIC_JKNZD:
      case ZYDIS_MNEMONIC_JKZD:
      case ZYDIS_MNEMONIC_JL:
      case ZYDIS_MNEMONIC_JLE:
      case ZYDIS_MNEMONIC_JNB:
      case ZYDIS_MNEMONIC_JNBE:
      case ZYDIS_MNEMONIC_JNL:
      case ZYDIS_MNEMONIC_JNLE:
      case ZYDIS_MNEMONIC_JNO:
      case ZYDIS_MNEMONIC_JNP:
      case ZYDIS_MNEMONIC_JNS:
      case ZYDIS_MNEMONIC_JNZ:
      case ZYDIS_MNEMONIC_JO:
      case ZYDIS_MNEMONIC_JP:
      case ZYDIS_MNEMONIC_JRCXZ:
      case ZYDIS_MNEMONIC_JS:
      case ZYDIS_MNEMONIC_JZ:
      case ZYDIS_MNEMONIC_LOOP:
      case ZYDIS_MNEMONIC_LOOPE:
      case ZYDIS_MNEMONIC_LOOPNE:
      {
        flags |= DASM_InstFlag_Branch;
      }break;
      
      case ZYDIS_MNEMONIC_JMP:
      {
        flags |= DASM_InstFlag_UnconditionalJump;
      }break;
      
      case ZYDIS_MNEMONIC_RET:
      {
        flags |= DASM_InstFlag_Return;
      }break;
      
      case ZYDIS_MNEMONIC_PUSH:
      case ZYDIS_MNEMONIC_POP:
      {
        flags |= DASM_InstFlag_ChangesStackPointer;
      }break;
      
      default:
      {
        flags |= DASM_InstFlag_NonFlow;
      }break;
    }
  }
  
  //////////////////////////////
  //- rjf: bundle
  //
  {
    inst.flags           = flags;
    inst.size            = zinst.info.length;
    inst.string          = str8_copy(arena, str8_cstring(zinst.text));
    inst.dst_vaddr       = dst_vaddr;
    inst.dst_reg_code    = dst_reg_code;
    inst.dst_reg_off     = dst_reg_off;
    inst.src_reg_code    = src_reg_code;
    inst.src_reg_off     = src_reg_off;
  }
  
  return inst;
}
