// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

internal String8 lnk_make_linker_compile3(Arena *arena, COFF_MachineType machine);
internal U64     lnk_size_stripped_symbols(String8 symbols, U64 align);
internal String8 lnk_make_debug_s(Arena *arena, String8List symbols);
