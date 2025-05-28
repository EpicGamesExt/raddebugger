// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef PE_SECTION_FLAGS_H
#define PE_SECTION_FLAGS_H

#define PE_TEXT_SECTION_FLAGS      (COFF_SectionFlag_CntCode|COFF_SectionFlag_MemExecute|COFF_SectionFlag_MemRead)
#define PE_DATA_SECTION_FLAGS      (COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite)
#define PE_RDATA_SECTION_FLAGS     (COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead)
#define PE_BSS_SECTION_FLAGS       (COFF_SectionFlag_CntUninitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite)
#define PE_IDATA_SECTION_FLAGS     PE_DATA_SECTION_FLAGS
#define PE_DEBUG_DIR_SECTION_FLAGS PE_RDATA_SECTION_FLAGS
#define PE_RSRC_SECTION_FLAGS      PE_DATA_SECTION_FLAGS
#define PE_RSRC1_SECTION_FLAGS     (PE_DATA_SECTION_FLAGS | COFF_SectionFlag_Align4Bytes)
#define PE_RSRC2_SECTION_FLAGS     (PE_DATA_SECTION_FLAGS | COFF_SectionFlag_Align4Bytes)
#define PE_XDATA_SECTION_FLAGS     PE_RDATA_SECTION_FLAGS
#define PE_PDATA_SECTION_FLAGS     PE_RDATA_SECTION_FLAGS
#define PE_EDATA_SECTION_FLAGS     PE_RDATA_SECTION_FLAGS
#define PE_GFIDS_SECTION_FLAGS     PE_RDATA_SECTION_FLAGS
#define PE_GIATS_SECTION_FLAGS     PE_RDATA_SECTION_FLAGS
#define PE_GLJMP_SECTION_FLAGS     PE_RDATA_SECTION_FLAGS
#define PE_GEHCONT_SECTION_FLAGS   PE_RDATA_SECTION_FLAGS
#define PE_RELOC_SECTION_FLAGS     (PE_RDATA_SECTION_FLAGS | COFF_SectionFlag_MemDiscardable)
#define PE_DEBUG_SECTION_FLAGS     (PE_RDATA_SECTION_FLAGS | COFF_SectionFlag_MemDiscardable)

#endif // PE_SECTION_FLAGS_H

