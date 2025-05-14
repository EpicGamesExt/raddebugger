////////////////////////////////
//~ rjf: CodeView <-> RDI Canonical Conversions

internal RDI_Arch
cv2r_rdi_arch_from_cv_arch(CV_Arch cv_arch)
{
  RDI_Arch result = 0;
  switch(cv_arch)
  {
    case CV_Arch_8086: result = RDI_Arch_X86; break;
    case CV_Arch_X64:  result = RDI_Arch_X64; break;
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

internal RDI_RegCode
cv2r_rdi_reg_code_from_cv_reg_code(RDI_Arch arch, CV_Reg reg_code)
{
  RDI_RegCode result = 0;
  switch(arch)
  {
    case RDI_Arch_X86:
    {
      switch(reg_code)
      {
#define X(CVN,C,RDN,BP,BZ) case C: result = RDI_RegCodeX86_##RDN; break;
        CV_Reg_X86_XList(X)
#undef X
      }
    }break;
    case RDI_Arch_X64:
    {
      switch(reg_code)
      {
#define X(CVN,C,RDN,BP,BZ) case C: result = RDI_RegCodeX64_##RDN; break;
        CV_Reg_X64_XList(X)
#undef X
      }
    }break;
  }
  return(result);
}

internal RDI_Language
cv2r_rdi_language_from_cv_language(CV_Language cv_language)
{
  RDI_Language result = 0;
  switch(cv_language)
  {
    case CV_Language_C:       result = RDI_Language_C; break;
    case CV_Language_CXX:     result = RDI_Language_CPlusPlus; break;
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

internal RDI_RegCode
cv2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg)
{
  RDI_RegCode result = 0;
  switch(arch)
  {
    case RDI_Arch_X86:
    {
      switch(encoded_reg)
      {
        case CV_EncodedFramePtrReg_StackPtr:
        {
          // TODO(allen): support CV_AllReg_VFRAME
          // TODO(allen): error
        }break;
        case CV_EncodedFramePtrReg_FramePtr:
        {
          result = RDI_RegCodeX86_ebp;
        }break;
        case CV_EncodedFramePtrReg_BasePtr:
        {
          result = RDI_RegCodeX86_ebx;
        }break;
      }
    }break;
    case RDI_Arch_X64:
    {
      switch(encoded_reg)
      {
        case CV_EncodedFramePtrReg_StackPtr:
        {
          result = RDI_RegCodeX64_rsp;
        }break;
        case CV_EncodedFramePtrReg_FramePtr:
        {
          result = RDI_RegCodeX64_rbp;
        }break;
        case CV_EncodedFramePtrReg_BasePtr:
        {
          result = RDI_RegCodeX64_r13;
        }break;
      }
    }break;
  }
  return(result);
}


internal RDI_TypeKind
cv2r_rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch(basic_type)
  {
    case CV_BasicType_VOID: {result = RDI_TypeKind_Void;}break;
    case CV_BasicType_HRESULT: {result = RDI_TypeKind_HResult;}break;
    
    case CV_BasicType_RCHAR:
    case CV_BasicType_CHAR:
    case CV_BasicType_CHAR8:
    {result = RDI_TypeKind_Char8;}break;
    
    case CV_BasicType_UCHAR: {result = RDI_TypeKind_UChar8;}break;
    case CV_BasicType_WCHAR: {result = RDI_TypeKind_UChar16;}break;
    case CV_BasicType_CHAR16: {result = RDI_TypeKind_Char16;}break;
    case CV_BasicType_CHAR32: {result = RDI_TypeKind_Char32;}break;
    
    case CV_BasicType_BOOL8:
    case CV_BasicType_INT8:
    {result = RDI_TypeKind_S8;}break;
    
    case CV_BasicType_BOOL16:
    case CV_BasicType_INT16:
    case CV_BasicType_SHORT:
    {result = RDI_TypeKind_S16;}break;
    
    case CV_BasicType_BOOL32:
    case CV_BasicType_INT32:
    case CV_BasicType_LONG:
    {result = RDI_TypeKind_S32;}break;
    
    case CV_BasicType_BOOL64:
    case CV_BasicType_INT64:
    case CV_BasicType_QUAD:
    {result = RDI_TypeKind_S64;}break;
    
    case CV_BasicType_INT128:
    case CV_BasicType_OCT:
    {result = RDI_TypeKind_S128;}break;
    
    case CV_BasicType_UINT8: {result = RDI_TypeKind_U8;}break;
    
    case CV_BasicType_UINT16:
    case CV_BasicType_USHORT:
    {result = RDI_TypeKind_U16;}break;
    
    case CV_BasicType_UINT32:
    case CV_BasicType_ULONG:
    {result = RDI_TypeKind_U32;}break;
    
    case CV_BasicType_UINT64:
    case CV_BasicType_UQUAD:
    {result = RDI_TypeKind_U64;}break;
    
    case CV_BasicType_UINT128:
    case CV_BasicType_UOCT:
    {result = RDI_TypeKind_U128;}break;
    
    case CV_BasicType_FLOAT16:{result = RDI_TypeKind_F16;}break;
    case CV_BasicType_FLOAT32:{result = RDI_TypeKind_F32;}break;
    case CV_BasicType_FLOAT32PP:{result = RDI_TypeKind_F32PP;}break;
    case CV_BasicType_FLOAT48:{result = RDI_TypeKind_F48;}break;
    case CV_BasicType_FLOAT64:{result = RDI_TypeKind_F64;}break;
    case CV_BasicType_FLOAT80:{result = RDI_TypeKind_F80;}break;
    case CV_BasicType_FLOAT128:{result = RDI_TypeKind_F128;}break;
    case CV_BasicType_COMPLEX32:{result = RDI_TypeKind_ComplexF32;}break;
    case CV_BasicType_COMPLEX64:{result = RDI_TypeKind_ComplexF64;}break;
    case CV_BasicType_COMPLEX80:{result = RDI_TypeKind_ComplexF80;}break;
    case CV_BasicType_COMPLEX128:{result = RDI_TypeKind_ComplexF128;}break;
    case CV_BasicType_PTR:{result = RDI_TypeKind_Handle;}break;
  }
  return result;
}

