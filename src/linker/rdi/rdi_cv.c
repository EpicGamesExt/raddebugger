// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDI_Arch
rdi_arch_from_cv_arch(CV_Arch arch)
{
  switch (arch) {
  case CV_Arch_8086: return RDI_Arch_X86;
  case CV_Arch_X64:  return RDI_Arch_X64;

  case CV_Arch_8080: 
  case CV_Arch_80286: 
  case CV_Arch_80386: 
  case CV_Arch_80486: 
  case CV_Arch_PENTIUM: 
  case CV_Arch_PENTIUMII: 
  case CV_Arch_PENTIUMIII: 
  case CV_Arch_MIPS: 
  case CV_Arch_MIPS16: 
  case CV_Arch_MIPS32: 
  case CV_Arch_MIPS64: 
  case CV_Arch_MIPSI: 
  case CV_Arch_MIPSII: 
  case CV_Arch_MIPSIII: 
  case CV_Arch_MIPSIV: 
  case CV_Arch_MIPSV: 
  case CV_Arch_M68000: 
  case CV_Arch_M68010: 
  case CV_Arch_M68020: 
  case CV_Arch_M68030: 
  case CV_Arch_M68040: 
  case CV_Arch_ALPHA: 
  case CV_Arch_ALPHA_21164: 
  case CV_Arch_ALPHA_21164A: 
  case CV_Arch_ALPHA_21264: 
  case CV_Arch_ALPHA_21364: 
  case CV_Arch_PPC601: 
  case CV_Arch_PPC603: 
  case CV_Arch_PPC604: 
  case CV_Arch_PPC620: 
  case CV_Arch_PPCFP: 
  case CV_Arch_PPCBE: 
  case CV_Arch_SH3: 
  case CV_Arch_SH3E: 
  case CV_Arch_SH3DSP: 
  case CV_Arch_SH4: 
  case CV_Arch_SHMEDIA: 
  case CV_Arch_ARM3: 
  case CV_Arch_ARM4: 
  case CV_Arch_ARM4T: 
  case CV_Arch_ARM5: 
  case CV_Arch_ARM5T: 
  case CV_Arch_ARM6: 
  case CV_Arch_ARM_XMAC: 
  case CV_Arch_ARM_WMMX: 
  case CV_Arch_ARM7: 
  case CV_Arch_OMNI: 
  case CV_Arch_IA64_1: 
  case CV_Arch_IA64_2: 
  case CV_Arch_CEE: 
  case CV_Arch_AM33: 
  case CV_Arch_M32R: 
  case CV_Arch_TRICORE: 
  case CV_Arch_EBC: 
  case CV_Arch_THUMB: 
  case CV_Arch_ARMNT: 
  case CV_Arch_ARM64: 
  case CV_Arch_D3D11_SHADER: 
    NotImplemented;
  default:
    return RDI_Arch_NULL;
  }
}

internal RDI_Language
rdi_language_from_cv_language(CV_Language language)
{
  switch (language) {
  case CV_Language_C:      return RDI_Language_C;
  case CV_Language_CXX:    return RDI_Language_CPlusPlus;
  case CV_Language_MASM:   return RDI_Language_Masm;
  case CV_Language_LINK:   return RDI_Language_NULL;
  case CV_Language_CVTRES: return RDI_Language_NULL;

  case CV_Language_FORTRAN: 
  case CV_Language_PASCAL:  
  case CV_Language_BASIC:   
  case CV_Language_COBOL:   
  case CV_Language_CVTPGD:  
  case CV_Language_CSHARP:  
  case CV_Language_VB:      
  case CV_Language_ILASM:   
  case CV_Language_JAVA:    
  case CV_Language_JSCRIPT: 
  case CV_Language_MSIL:    
  case CV_Language_HLSL:    
    NotImplemented;
  default:
    return RDI_Language_NULL;
  }
}

internal RDI_TypeModifierFlags
rdi_type_modifier_flags_from_cv_modifier_flags(CV_ModifierFlags flags)
{
  RDI_TypeModifierFlags result = 0;
  if (flags & CV_ModifierFlag_Const) {
    result |= RDI_TypeModifierFlag_Const;
  }
  if (flags & CV_ModifierFlag_Volatile) {
    result |= RDI_TypeModifierFlag_Volatile;
  }
  return result;
}

internal RDI_TypeModifierFlags
rdi_type_modifier_flags_from_cv_pointer_attribs(CV_PointerAttribs attribs)
{
  RDI_TypeModifierFlags result = 0;
  if (attribs & CV_PointerAttrib_Const) {
    result |= RDI_TypeModifierFlag_Const;
  }
  if (attribs & CV_PointerAttrib_Volatile) {
    result |= RDI_TypeModifierFlag_Volatile;
  }
  return result;
}

internal RDI_TypeKind
rdi_type_kind_from_pointer(CV_PointerAttribs attribs, CV_PointerMode mode)
{
  RDI_TypeKind result = RDI_TypeKind_Ptr;
  
  if (attribs & CV_PointerAttrib_LRef) {
    result = RDI_TypeKind_LRef;
  } else if (attribs & CV_PointerAttrib_RRef) {
    result = RDI_TypeKind_RRef;
  }

  if (mode == CV_PointerMode_LRef) {
    result = RDI_TypeKind_LRef;
  } else if (mode == CV_PointerMode_RRef) {
    result = RDI_TypeKind_RRef;
  }

  return result;
}

internal RDI_TypeKind
rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type)
{
  switch (basic_type) {
  case CV_BasicType_NOTYPE    : return RDI_TypeKind_NULL;
  case CV_BasicType_ABS       : return RDI_TypeKind_NULL;
  case CV_BasicType_SEGMENT   : return RDI_TypeKind_NULL;
  case CV_BasicType_VOID      : return RDI_TypeKind_Void;
  case CV_BasicType_CURRENCY  : return RDI_TypeKind_NULL;
  case CV_BasicType_NBASICSTR : return RDI_TypeKind_NULL;
  case CV_BasicType_FBASICSTR : return RDI_TypeKind_NULL;
  case CV_BasicType_HRESULT   : return RDI_TypeKind_Handle;
  case CV_BasicType_CHAR      : return RDI_TypeKind_Char8;
  case CV_BasicType_SHORT     : return RDI_TypeKind_S16;
  case CV_BasicType_LONG      : return RDI_TypeKind_S32;
  case CV_BasicType_QUAD      : return RDI_TypeKind_S64;
  case CV_BasicType_OCT       : return RDI_TypeKind_S128;
  case CV_BasicType_UCHAR     : return RDI_TypeKind_UChar8;
  case CV_BasicType_USHORT    : return RDI_TypeKind_U16;
  case CV_BasicType_ULONG     : return RDI_TypeKind_U32;
  case CV_BasicType_UQUAD     : return RDI_TypeKind_U64;
  case CV_BasicType_UOCT      : return RDI_TypeKind_U128;
  case CV_BasicType_BOOL8     : return RDI_TypeKind_S8;
  case CV_BasicType_BOOL16    : return RDI_TypeKind_S16;
  case CV_BasicType_BOOL32    : return RDI_TypeKind_S32;
  case CV_BasicType_BOOL64    : return RDI_TypeKind_S64;
  case CV_BasicType_FLOAT32   : return RDI_TypeKind_F32;
  case CV_BasicType_FLOAT64   : return RDI_TypeKind_F64;
  case CV_BasicType_FLOAT80   : return RDI_TypeKind_F80;
  case CV_BasicType_FLOAT128  : return RDI_TypeKind_F128;
  case CV_BasicType_FLOAT48   : return RDI_TypeKind_F48;
  case CV_BasicType_FLOAT32PP : return RDI_TypeKind_F32PP;
  case CV_BasicType_FLOAT16   : return RDI_TypeKind_F16;
  case CV_BasicType_COMPLEX32 : return RDI_TypeKind_ComplexF32;
  case CV_BasicType_COMPLEX64 : return RDI_TypeKind_ComplexF64;
  case CV_BasicType_COMPLEX80 : return RDI_TypeKind_ComplexF80;
  case CV_BasicType_COMPLEX128: return RDI_TypeKind_ComplexF128;
  case CV_BasicType_BIT       : return RDI_TypeKind_NULL;
  case CV_BasicType_PASCHAR   : return RDI_TypeKind_NULL;
  case CV_BasicType_BOOL32FF  : return RDI_TypeKind_NULL;
  case CV_BasicType_INT8      : return RDI_TypeKind_S8;
  case CV_BasicType_UINT8     : return RDI_TypeKind_U8;
  case CV_BasicType_RCHAR     : return RDI_TypeKind_Char8;
  case CV_BasicType_WCHAR     : return RDI_TypeKind_UChar16;
  case CV_BasicType_CHAR16    : return RDI_TypeKind_Char16;
  case CV_BasicType_CHAR32    : return RDI_TypeKind_Char32;
  case CV_BasicType_INT16     : return RDI_TypeKind_S16;
  case CV_BasicType_UINT16    : return RDI_TypeKind_U16;
  case CV_BasicType_INT32     : return RDI_TypeKind_S32;
  case CV_BasicType_UINT32    : return RDI_TypeKind_U32;
  case CV_BasicType_INT64     : return RDI_TypeKind_S64;
  case CV_BasicType_UINT64    : return RDI_TypeKind_U64;
  case CV_BasicType_INT128    : return RDI_TypeKind_S128;
  case CV_BasicType_UINT128   : return RDI_TypeKind_U128;
  case CV_BasicType_CHAR8     : return RDI_TypeKind_Char8;
  case CV_BasicType_PTR       : return RDI_TypeKind_Ptr;
  }
  return RDI_TypeKind_NULL;
}

internal RDI_RegCode
rdi_reg_code_from_cv(CV_Arch arch, CV_Reg reg)
{
  RDI_RegCode result = 0;
  switch (arch) {
  case CV_Arch_8086: {
    switch (reg) {
#define X(CVN,C,RDN,BP,BZ) case C: result = RDI_RegCodeX86_##RDN; break;
        CV_Reg_X86_XList(X)
#undef X
    }
  } break;
  case CV_Arch_X64: {
    switch (reg) {
#define X(CVN,C,RDN,BP,BZ) case C: result = RDI_RegCodeX64_##RDN; break;
        CV_Reg_X64_XList(X)
#undef X
    }
  } break;
  default: NotImplemented;
  }
  return result;
}

internal RDI_ChecksumKind
rdi_checksum_from_cv_c13(CV_C13ChecksumKind kind)
{
  switch (kind) {
  case CV_C13ChecksumKind_Null:   return RDI_Checksum_Null;
  case CV_C13ChecksumKind_MD5:    return RDI_Checksum_MD5;
  case CV_C13ChecksumKind_SHA1:   return RDI_Checksum_SHA1;
  case CV_C13ChecksumKind_SHA256: return RDI_Checksum_SHA256;
  }
  InvalidPath;
  return RDI_Checksum_Null;
}
