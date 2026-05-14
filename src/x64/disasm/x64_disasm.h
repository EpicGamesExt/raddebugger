// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef X64_DISASM_H
#define X64_DISASM_H

internal DASM_Inst x64_dasm_inst_from_code(Arena *arena, U64 vaddr, String8  code, DASM_Syntax syntax);

#endif // X64_DISASM_H
