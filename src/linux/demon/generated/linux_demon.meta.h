// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef LINUX_DEMON_META_H
#define LINUX_DEMON_META_H

typedef enum LNX_DMN_ProbeKind
{
LNX_DMN_ProbeKind_Null,
LNX_DMN_ProbeKind_InitStart,
LNX_DMN_ProbeKind_InitComplete,
LNX_DMN_ProbeKind_RelocStart,
LNX_DMN_ProbeKind_RelocComplete,
LNX_DMN_ProbeKind_MapStart,
LNX_DMN_ProbeKind_MapComplete,
LNX_DMN_ProbeKind_UnmapStart,
LNX_DMN_ProbeKind_UnmapComplete,
LNX_DMN_ProbeKind_LongJmp,
LNX_DMN_ProbeKind_LongJmpTarget,
LNX_DMN_ProbeKind_SetJmp,
LNX_DMN_ProbeKind_COUNT,
} LNX_DMN_ProbeKind;

C_LINKAGE_BEGIN
extern U8 lnx_dmn_probe_kind_args_count_table[12];
extern String8 lnx_dmn_probe_kind_string_table[12];

C_LINKAGE_END

#endif // LINUX_DEMON_META_H
