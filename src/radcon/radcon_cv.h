#pragma once

////////////////////////////////
//~ rjf: CodeView => RDI Canonical Conversions

internal RDI_Arch     cv2r_rdi_arch_from_cv_arch(CV_Arch arch);
internal RDI_RegCode  cv2r_rdi_reg_code_from_cv_reg_code(RDI_Arch arch, CV_Reg reg_code);
internal RDI_Language cv2r_rdi_language_from_cv_language(CV_Language language);
internal RDI_RegCode  cv2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg);
internal RDI_TypeKind cv2r_rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type);
