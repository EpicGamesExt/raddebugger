// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_CODEVIEW_CONVERSION_H
#define RADDBG_CODEVIEW_CONVERSION_H

////////////////////////////////
//~ CodeView Conversion Functions

static RADDBG_Arch     raddbg_arch_from_cv_arch(CV_Arch arch);
static RADDBG_RegisterCode raddbg_reg_code_from_cv_reg_code(RADDBG_Arch arch, CV_Reg reg_code);
static RADDBG_Language raddbg_language_from_cv_language(CV_Language language);

#endif //RADDBG_CODEVIEW_CONVERSION_H
