// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EH_FRAME_DUMP_H
#define EH_FRAME_DUMP_H

////////////////////////////////
//~ Dump Subset Types

#define EH_DumpSubset_XList                    \
  X(EhFrameHdr, eh_frame_hdr, ".eh_frame_hdr") \
  X(EhFrame,    eh_frame,     ".eh_frame")

typedef enum EH_DumpSubset {
#define X(name, name_lower, title) EH_DumpSubset_##name,
  EH_DumpSubset_XList
#undef X
} EH_DumpSubset;

typedef U32 EH_DumpSubsetFlags;
enum {
#define X(name, name_lower, title) EH_DumpSubsetFlag_##name = (1<<EH_DumpSubset_##name),
  EH_DumpSubset_XList
#undef X
  EH_DumpSubsetFlag_All = 0xffffffff,
};

read_only global String8 eh_name_lowercase_from_dump_subset_table[] =
{
#define X(name, name_lower, title) str8_lit_comp(#name_lower),
  EH_DumpSubset_XList
#undef X
};

read_only global String8 eh_name_title_from_dump_subset_table[] =
{
#define X(name, name_lower, title) str8_lit_comp(title),
  EH_DumpSubset_XList
#undef X
};

////////////////////////////////
//~ Dump Entry Point

internal String8List eh_dump_list_from_data(Arena *arena, Arch arch, U64 eh_frame_hdr_vaddr, U64 eh_frame_vaddr, String8 eh_frame_hdr, String8 eh_frame, EH_DumpSubsetFlags subset_flags);

#endif // EH_FRAME_DUMP_H

