// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ Generated Code

#include "generated/codeview.meta.c"

////////////////////////////////

internal CV_Arch
cv_arch_from_coff_machine(COFF_MachineType machine)
{
  CV_Arch arch = 0;
  switch(machine)
  {
  case COFF_MachineType_X64:       arch = CV_Arch_X64;    break;
  case COFF_MachineType_X86:       arch = CV_Arch_8086;   break;
  case COFF_MachineType_AM33:      arch = CV_Arch_AM33;   break;
  case COFF_MachineType_ARM:       NotImplemented;        break;
  case COFF_MachineType_ARM64:     arch = CV_Arch_ARM64;  break;
  case COFF_MachineType_ARMNT:     arch = CV_Arch_ARMNT;  break;
  case COFF_MachineType_EBC:       arch = CV_Arch_EBC;    break;
  case COFF_MachineType_IA64:      arch = CV_Arch_IA64;   break;
  case COFF_MachineType_M32R:      arch = CV_Arch_M32R;   break;
  case COFF_MachineType_MIPS16:    arch = CV_Arch_MIPS16; break;
  case COFF_MachineType_MIPSFPU:   NotImplemented;        break;
  case COFF_MachineType_MIPSFPU16: NotImplemented;        break;
  case COFF_MachineType_POWERPC:   NotImplemented;        break;
  case COFF_MachineType_POWERPCFP: arch = CV_Arch_PPCFP;  break;
  case COFF_MachineType_R4000:     NotImplemented;        break;
  case COFF_MachineType_RISCV32:   NotImplemented;        break;
  case COFF_MachineType_RISCV64:   NotImplemented;        break;
  case COFF_MachineType_RISCV128:  NotImplemented;        break;
  case COFF_MachineType_SH3:       arch = CV_Arch_SH3;    break;
  case COFF_MachineType_SH3DSP:    arch = CV_Arch_SH3DSP; break;
  case COFF_MachineType_SH4:       arch = CV_Arch_SH4;    break;
  case COFF_MachineType_SH5:       NotImplemented;        break;
  case COFF_MachineType_THUMB:     arch = CV_Arch_THUMB;  break;
  case COFF_MachineType_WCEMIPSV2: NotImplemented;        break;
  }
  return arch;
}

internal U64
cv_size_from_reg(CV_Arch arch, CV_Reg reg)
{
  switch(arch)
  {
  case CV_Arch_8086: return cv_size_from_reg_x86(reg);
  case CV_Arch_X64 : return cv_size_from_reg_x64(reg);
  default: NotImplemented;
  }
  return 0;
}

internal B32
cv_is_reg_sp(CV_Arch arch, CV_Reg reg)
{
  switch(arch)
  {
  case CV_Arch_8086: return reg == CV_Regx86_ESP;
  case CV_Arch_X64:  return reg == CV_Regx64_RSP;
  default: NotImplemented;
  }
  return 0;
}

internal U64
cv_size_from_reg_x86(CV_Reg reg)
{
  switch(reg)
  {
#define X(NAME, CODE, RDI_NAME, BYTE_POS, BYTE_SIZE) case CV_Regx86_##NAME: return BYTE_SIZE;
    CV_Reg_X86_XList(X)
#undef X
  }
  return 0;
}

internal U64
cv_size_from_reg_x64(CV_Reg reg)
{
  switch(reg)
  {
#define X(NAME, CODE, RDI_NAME, BYTE_POS, BYTE_SIZE) case CV_Regx64_##NAME: return BYTE_SIZE;
  CV_Reg_X64_XList(X)
#undef X 
  }
  return 0;
}

internal CV_EncodedFramePtrReg
cv_pick_fp_encoding(CV_SymFrameproc *frameproc, B32 is_local_param)
{
  CV_EncodedFramePtrReg fp_reg = 0;
  if(is_local_param)
  {
    fp_reg = CV_FrameprocFlags_ExtractParamBasePointer(frameproc->flags);
  }
  else
  {
    fp_reg = CV_FrameprocFlags_ExtractLocalBasePointer(frameproc->flags);
  }
  return fp_reg;
}

internal CV_Reg
cv_decode_fp_reg(CV_Arch arch, CV_EncodedFramePtrReg encoded_reg)
{
  CV_Reg fp_reg = 0;
  switch (arch)
  {
  case CV_Arch_8086:
  {
    switch (encoded_reg)
	  {
    case CV_EncodedFramePtrReg_None    : break;
    case CV_EncodedFramePtrReg_StackPtr: AssertAlways(!"TODO: not tested, this is a guess");
                                         fp_reg = CV_Regx86_ESP; break;
    case CV_EncodedFramePtrReg_FramePtr: fp_reg = CV_Regx86_EBP; break;
    case CV_EncodedFramePtrReg_BasePtr : fp_reg = CV_Regx86_EBX; break;
    default: InvalidPath;
    }
  } break;
  case CV_Arch_X64:
  {
    switch (encoded_reg)
	  {
    case CV_EncodedFramePtrReg_None    : break;
    case CV_EncodedFramePtrReg_StackPtr: fp_reg = CV_Regx64_RSP; break;
    case CV_EncodedFramePtrReg_FramePtr: fp_reg = CV_Regx64_RBP; break;
    case CV_EncodedFramePtrReg_BasePtr : fp_reg = CV_Regx64_R13; break;
    default: InvalidPath;
    }
  } break;
  default: NotImplemented;
  }
  return fp_reg;
}

internal U32
cv_map_encoded_base_pointer(CV_Arch arch, U32 encoded_frame_reg)
{
  U32 r = 0;
  switch (arch) {
  case CV_Arch_8086: {
    switch (encoded_frame_reg) {
    case 0: r = 0;                    break;
    case 1: r = CV_AllReg_VFRAME; break;
    case 2: r = CV_Regx86_EBP;    break;
    case 3: r = CV_Regx86_EBX;    break;
    }
  } break;
  case CV_Arch_X64: {
    switch (encoded_frame_reg) {
    case 0: r = 0; break;
    case 1: r = CV_Regx64_RSP; break;
    case 2: r = CV_Regx64_RBP; break;
    case 3: r = CV_Regx64_R13; break;
    }
  } break;
    default: NotImplemented;
  }
  return r;
}

