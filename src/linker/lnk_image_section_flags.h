// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

// --- Default Section Flags --------------------------------------------------

#define LNK_TEXT_SECTION_FLAGS      (COFF_SectionFlag_CntCode|COFF_SectionFlag_MemExecute|COFF_SectionFlag_MemRead)
#define LNK_DATA_SECTION_FLAGS      (COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite)
#define LNK_RDATA_SECTION_FLAGS     (COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead)
#define LNK_BSS_SECTION_FLAGS       (COFF_SectionFlag_CntUninitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite)
#define LNK_IDATA_SECTION_FLAGS     LNK_DATA_SECTION_FLAGS
#define LNK_DEBUG_DIR_SECTION_FLAGS LNK_DATA_SECTION_FLAGS
#define LNK_RSRC_SECTION_FLAGS      LNK_DATA_SECTION_FLAGS
#define LNK_RSRC1_SECTION_FLAGS     (LNK_DATA_SECTION_FLAGS | COFF_SectionFlag_Align4Bytes)
#define LNK_RSRC2_SECTION_FLAGS     (LNK_DATA_SECTION_FLAGS | COFF_SectionFlag_Align4Bytes)
#define LNK_XDATA_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_PDATA_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_EDATA_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_GFIDS_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_GIATS_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_GLJMP_SECTION_FLAGS     LNK_RDATA_SECTION_FLAGS
#define LNK_GEHCONT_SECTION_FLAGS   LNK_RDATA_SECTION_FLAGS
#define LNK_RELOC_SECTION_FLAGS     (LNK_RDATA_SECTION_FLAGS | COFF_SectionFlag_MemDiscardable)
#define LNK_DEBUG_SECTION_FLAGS     (LNK_RDATA_SECTION_FLAGS | COFF_SectionFlag_MemDiscardable)

