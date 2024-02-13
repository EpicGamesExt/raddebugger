// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ CodeView Conversion Functions

internal RADDBGI_Arch
raddbgi_arch_from_cv_arch(CV_Arch cv_arch){
  RADDBGI_Arch result = 0;
  switch (cv_arch){
    case CV_Arch_8086: result = RADDBGI_Arch_X86; break;
    case CV_Arch_X64:  result = RADDBGI_Arch_X64; break;
    
    //case CV_Arch_8080: break;
    //case CV_Arch_80286: break;
    //case CV_Arch_80386: break;
    //case CV_Arch_80486: break;
    //case CV_Arch_PENTIUM: break;
    //case CV_Arch_PENTIUMII: break;
    //case CV_Arch_PENTIUMIII: break;
    //case CV_Arch_MIPS: break;
    //case CV_Arch_MIPS16: break;
    //case CV_Arch_MIPS32: break;
    //case CV_Arch_MIPS64: break;
    //case CV_Arch_MIPSI: break;
    //case CV_Arch_MIPSII: break;
    //case CV_Arch_MIPSIII: break;
    //case CV_Arch_MIPSIV: break;
    //case CV_Arch_MIPSV: break;
    //case CV_Arch_M68000: break;
    //case CV_Arch_M68010: break;
    //case CV_Arch_M68020: break;
    //case CV_Arch_M68030: break;
    //case CV_Arch_M68040: break;
    //case CV_Arch_ALPHA: break;
    //case CV_Arch_ALPHA_21164: break;
    //case CV_Arch_ALPHA_21164A: break;
    //case CV_Arch_ALPHA_21264: break;
    //case CV_Arch_ALPHA_21364: break;
    //case CV_Arch_PPC601: break;
    //case CV_Arch_PPC603: break;
    //case CV_Arch_PPC604: break;
    //case CV_Arch_PPC620: break;
    //case CV_Arch_PPCFP: break;
    //case CV_Arch_PPCBE: break;
    //case CV_Arch_SH3: break;
    //case CV_Arch_SH3E: break;
    //case CV_Arch_SH3DSP: break;
    //case CV_Arch_SH4: break;
    //case CV_Arch_SHMEDIA: break;
    //case CV_Arch_ARM3: break;
    //case CV_Arch_ARM4: break;
    //case CV_Arch_ARM4T: break;
    //case CV_Arch_ARM5: break;
    //case CV_Arch_ARM5T: break;
    //case CV_Arch_ARM6: break;
    //case CV_Arch_ARM_XMAC: break;
    //case CV_Arch_ARM_WMMX: break;
    //case CV_Arch_ARM7: break;
    //case CV_Arch_OMNI: break;
    //case CV_Arch_IA64_1: break;
    //case CV_Arch_IA64_2: break;
    //case CV_Arch_CEE: break;
    //case CV_Arch_AM33: break;
    //case CV_Arch_M32R: break;
    //case CV_Arch_TRICORE: break;
    //case CV_Arch_EBC: break;
    //case CV_Arch_THUMB: break;
    //case CV_Arch_ARMNT: break;
    //case CV_Arch_ARM64: break;
    //case CV_Arch_D3D11_SHADER: break;
  }
  return(result);
}

internal RADDBGI_RegisterCode
raddbgi_reg_code_from_cv_reg_code(RADDBGI_Arch arch, CV_Reg reg_code){
  RADDBGI_RegisterCode result = 0;
  switch (arch){
    case RADDBGI_Arch_X86:
    {
      switch (reg_code){
#define X(CVN,C,RDN,BP,BZ) case C: result = RADDBGI_RegisterCode_X86_##RDN; break;
        CV_Reg_X86_XList(X)
#undef X
      }
    }break;
    
    case RADDBGI_Arch_X64:
    {
      switch (reg_code){
#define X(CVN,C,RDN,BP,BZ) case C: result = RADDBGI_RegisterCode_X64_##RDN; break;
        CV_Reg_X64_XList(X)
#undef X
      }
    }break;
  }
  return(result);
}

internal RADDBGI_Language
raddbgi_language_from_cv_language(CV_Language cv_language){
  RADDBGI_Language result = 0;
  switch (cv_language){
    case CV_Language_C:       result = RADDBGI_Language_C; break;
    case CV_Language_CXX:     result = RADDBGI_Language_CPlusPlus; break;
    //case CV_Language_FORTRAN: result = ; break;
    //case CV_Language_MASM:    result = ; break;
    //case CV_Language_PASCAL:  result = ; break;
    //case CV_Language_BASIC:   result = ; break;
    //case CV_Language_COBOL:   result = ; break;
    //case CV_Language_LINK:    result = ; break;
    //case CV_Language_CVTRES:  result = ; break;
    //case CV_Language_CVTPGD:  result = ; break;
    //case CV_Language_CSHARP:  result = ; break;
    //case CV_Language_VB:      result = ; break;
    //case CV_Language_ILASM:   result = ; break;
    //case CV_Language_JAVA:    result = ; break;
    //case CV_Language_JSCRIPT: result = ; break;
    //case CV_Language_MSIL:    result = ; break;
    //case CV_Language_HLSL:    result = ; break;
  }
  return(result);
}
