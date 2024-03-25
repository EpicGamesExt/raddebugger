// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef UNWIND_H
#define UNWIND_H

////////////////////////////////
//~ rjf: Memory View Types
//
// Memory views are used to provide a slice (or, in the future, slices) of data
// required to do a proper unwind. This is generally a very small region in an
// address space, generally some things on the stack. But, some formats don't
// restrict this in an organized way, and so theoretically you might have some
// unwind information require arbitrary reads at an unknown address. So this
// "memory view" concept serves as kind of a "stand-in" for "provided memory
// info from the user". This keeps the control flow of this layer simpler, so
// we aren't calling a user-supplied hook to read memory or anything like that.

typedef struct UNW_MemView UNW_MemView;
struct UNW_MemView
{
  // Upgrade Path:
  //  1. A list of ranges like this one
  //  2. Binary-searchable list of ranges
  //  3. In-line growth strategy for missing pages (hardwired to source of new data)
  //  4. Abstracted source of new data
  void *data;
  U64 addr_first;
  U64 addr_opl;
};

////////////////////////////////
//~ rjf: Unwind Step Results

typedef struct UNW_Step UNW_Step;
struct UNW_Step
{
  B32 dead;
  B32 missed_read;
  U64 missed_read_addr;
  U64 stack_pointer;
};

////////////////////////////////
//~ rjf: Memory View Helpers

internal UNW_MemView unw_memview_from_data(String8 data, U64 base_vaddr);
internal B32 unw_memview_read(UNW_MemView *memview, U64 addr, U64 size, void *out);
#define unw_memview_read_struct(v,addr,p) unw_memview_read((v), (addr), sizeof(*(p)), (p))

////////////////////////////////
//~ rjf: PE/X64 Unwind Implementation

//- rjf: helpers
internal UNW_Step unw_pe_x64__epilog(String8 bindata, PE_BinInfo *bin, U64 base_vaddr, UNW_MemView *memview, REGS_RegBlockX64 *regs_inout);
internal B32 unw_pe_x64__voff_is_in_epilog(String8 bindata, PE_BinInfo *bin, U64 voff, PE_IntelPdata *final_pdata);
internal REGS_Reg64 *unw_pe_x64__gpr_reg(REGS_RegBlockX64 *regs, PE_UnwindGprRegX64 unw_reg);

//- rjf: unwind step
internal UNW_Step unw_unwind_pe_x64(String8 bindata, PE_BinInfo *bin, U64 base_vaddr, UNW_MemView *memview, REGS_RegBlockX64 *regs_inout);

#endif // UNWIND_H
