// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_CODEVIEW_CONVERSION_H
#define RADDBGI_CODEVIEW_CONVERSION_H

////////////////////////////////
//~ CodeView Conversion Functions

static RADDBGI_Arch     raddbgi_arch_from_cv_arch(CV_Arch arch);
static RADDBGI_RegisterCode raddbgi_reg_code_from_cv_reg_code(RADDBGI_Arch arch, CV_Reg reg_code);
static RADDBGI_Language raddbgi_language_from_cv_language(CV_Language language);

#endif //RADDBGI_CODEVIEW_CONVERSION_H
