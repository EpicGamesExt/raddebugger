// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ COFF Conversion Functions

static RADDBGI_BinarySectionFlags
raddbgi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags){
  RADDBGI_BinarySectionFlags result = 0;
  
  if (flags & COFF_SectionFlag_MEM_READ){
    result |= RADDBGI_BinarySectionFlag_Read;
  }
  if (flags & COFF_SectionFlag_MEM_WRITE){
    result |= RADDBGI_BinarySectionFlag_Write;
  }
  if (flags & COFF_SectionFlag_MEM_EXECUTE){
    result |= RADDBGI_BinarySectionFlag_Execute;
  }
  
  return(result);
}
