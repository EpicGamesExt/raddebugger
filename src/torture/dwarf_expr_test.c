// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define BUILD_CONSOLE_INFERFACE 1
#define BUILD_TITLE "DWARF Expr Test"

////////////////////////////////

#include "third_party/xxHash/xxhash.c"
#include "third_party/xxHash/xxhash.h"
#include "third_party/radsort/radsort.h"

////////////////////////////////

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "linker/hash_table.h"
#include "coff/coff.h"
#include "elf/elf.h"
#include "elf/elf_parse.h"
#include "dwarf/dwarf_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "coff/coff.c"
#include "elf/elf.c"
#include "elf/elf_parse.c"
#include "linker/hash_table.c"
#include "dwarf/dwarf_inc.c"

////////////////////////////////

internal void
entry_point(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0, 0);

  {
    DW_ExprEnc program[] = {
      DW_ExprEnc_DeclLabel("foo"),
      DW_ExprEnc_Op(BReg11), DW_ExprEnc_S64(44),
      DW_ExprEnc_Op(Skip), DW_ExprEnc_Label("foo"),
    };
    String8 expr_data = dw_encode_expr(scratch.arena, DW_Format_64Bit, sizeof(U64), program, ArrayCount(program));
  }

  scratch_end(scratch);
}

