// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

internal RDI_Arch              rdi_arch_from_cv_arch(CV_Arch arch);
internal RDI_Language          rdi_language_from_cv_language(CV_Language language);
internal RDI_TypeModifierFlags rdi_type_modifier_flags_from_cv_pointer_attribs(CV_PointerAttribs attribs);
internal RDI_TypeKind          rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type);
internal RDI_RegCode           rdi_reg_code_from_cv(CV_Arch arch, CV_Reg reg);

internal RDI_ChecksumKind rdi_checksum_from_cv_c13(CV_C13ChecksumKind kind);


