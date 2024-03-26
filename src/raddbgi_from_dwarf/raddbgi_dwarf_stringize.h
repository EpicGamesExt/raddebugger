// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_DWARF_STRINGIZE_H
#define RADDBGI_DWARF_STRINGIZE_H

////////////////////////////////
//~ DWARF Stringize Functions

static void
dwarf_stringize_info(Arena *arena, String8List *out, DWARF_InfoUnit *unit, U32 indent);

static void
dwarf_stringize_pubnames(Arena *arena, String8List *out, DWARF_PubNamesUnit *unit,
                         U32 indent);

static void
dwarf_stringize_names(Arena *arena, String8List *out, DWARF_NamesUnit *unit, U32 indent);

static void
dwarf_stringize_aranges(Arena *arena, String8List *out, DWARF_ArangesUnit *unit, U32 indent);

static void
dwarf_stringize_addr(Arena *arena, String8List *out, DWARF_AddrUnit *unit, U32 indent);



#endif //RADDBGI_DWARF_STRINGIZE_H
