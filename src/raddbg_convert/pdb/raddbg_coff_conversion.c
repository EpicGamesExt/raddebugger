// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ COFF Conversion Functions

static RADDBG_BinarySectionFlags
raddbg_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags){
  RADDBG_BinarySectionFlags result = 0;
  
  if (flags & COFF_SectionFlag_MEM_READ){
    result |= RADDBG_BinarySectionFlag_Read;
  }
  if (flags & COFF_SectionFlag_MEM_WRITE){
    result |= RADDBG_BinarySectionFlag_Write;
  }
  if (flags & COFF_SectionFlag_MEM_EXECUTE){
    result |= RADDBG_BinarySectionFlag_Execute;
  }
  
  return(result);
}
