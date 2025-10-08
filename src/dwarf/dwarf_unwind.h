// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_UNWIND_H
#define DWARF_UNWIND_H

typedef struct DW_UnwindResult
{
  B32 is_invalid;
  B32 missed_read;
  U64 missed_read_addr;
  U64 stack_pointer;
} DW_UnwindResult;

// EH: Exception Frames

typedef struct DW_UnpackedCIENode
{
  struct DW_UnpackedCIENode *next;
  DW_UnpackedCIE             cie;
  U64                        offset;
} DW_UnpackedCIENode;

// CFI: Call Frame Information
typedef struct DW_CFIRecords
{
  B32            valid;
  DW_UnpackedCIE cie;
  DW_UnpackedFDE fde;
} DW_CFIRecords;

typedef enum DW_CFICFARule{
  DW_CFI_CFA_Rule_RegOff,
  DW_CFI_CFA_Rule_Expr,
} DW_CFICFARule;

typedef struct DW_CFICFACell
{
  DW_CFICFARule rule;
  union {
    struct {
      U64 reg_idx;
      S64 offset;
    };
    Rng1U64 expr;
  };
} DW_CFICFACell;

typedef enum DW_CFIRegisterRule
{
  DW_CFIRegisterRule_SameValue,
  DW_CFIRegisterRule_Undefined,
  DW_CFIRegisterRule_Offset,
  DW_CFIRegisterRule_ValOffset,
  DW_CFIRegisterRule_Register,
  DW_CFIRegisterRule_Expression,
  DW_CFIRegisterRule_ValExpression,
} DW_CFIRegisterRule;

typedef struct DW_CFICell
{
  DW_CFIRegisterRule rule;
  union {
    S64 n;
    Rng1U64 expr;
  };
} DW_CFICell;

typedef struct DW_CFIRow
{
  struct DW_CFIRow *next;
  DW_CFICell       *cells;
  DW_CFICFACell     cfa_cell;
} DW_CFIRow;

typedef struct DW_CFIMachine
{
  U64             cells_per_row;
  DW_UnpackedCIE *cie;
  DW_CFIRow      *initial_row;
  U64             fde_ip;
  EH_PtrCtx      *ptr_ctx;
} DW_CFIMachine;

typedef U8 DW_CFADecode;
enum
{
  DW_CFADecode_Nop     = 0x0,
  // 1,2,4,8 reserved for literal byte sizes
  DW_CFADecode_Address = 0x9,
  DW_CFADecode_ULEB128 = 0xA,
  DW_CFADecode_SLEB128 = 0xB,
};

typedef U16 DW_CFAControlBits;
enum
{
  DW_CFAControlBits_Dec1Mask = 0x00F,
  DW_CFAControlBits_Dec2Mask = 0x0F0,
  DW_CFAControlBits_IsReg0   = 0x100,
  DW_CFAControlBits_IsReg1   = 0x200,
  DW_CFAControlBits_IsReg2   = 0x400,
  DW_CFAControlBits_NewRow   = 0x800,
};

global read_only DW_CFAControlBits dw_unwind__cfa_control_bits_kind1[DW_CFA_OplKind1 + 1];
global read_only DW_CFAControlBits dw_unwind__cfa_control_bits_kind2[DW_CFA_OplKind2 + 1];

// register codes for unwinding match the DW_RegX64 register codes
#define DW_UNWIND_X64__REG_SLOT_COUNT 17

////////////////////////////////
// x64 Unwind Function

internal DW_UnwindResult
dw_unwind_x64(String8           raw_text,
              String8           raw_eh_frame,
              String8           raw_eh_frame_header,
              Rng1U64           text_vrange,
              Rng1U64           eh_frame_vrange,
              Rng1U64           eh_frame_header_vrange,
              U64               default_image_base,
              U64               image_base,
              U64               stack_pointer,
              DW_RegsX64       *regs,
              DW_ReadMemorySig *read_memory,
              void             *read_memory_ud);

internal DW_UnwindResult dw_unwind_x64__apply_frame_rules(String8 raw_eh_frame, DW_CFIRow *row, U64 text_base_vaddr, DW_ReadMemorySig *read_memory, void *read_memory_ud, U64 stack_pointer, DW_RegsX64 *regs);

////////////////////////////////
// x64 Unwind Helper Functions

internal void dw_unwind_init_x64(void);

//- eh_frame parsing
internal DW_CFIRecords dw_unwind_eh_frame_cfi_from_ip_slow_x64(String8 raw_eh_frame, EH_PtrCtx *ptr_ctx, U64 ip_voff);
internal DW_CFIRecords dw_unwind_eh_frame_hdr_from_ip_fast_x64(String8 raw_eh_frame, String8 raw_eh_frame_hdr, EH_PtrCtx *ptr_ctx, U64 ip_voff);

//- cfi machine

internal DW_CFIMachine dw_unwind_make_machine_x64(U64 cells_per_row, DW_UnpackedCIE *cie, EH_PtrCtx *ptr_ctx);
internal void          dw_unwind_machine_equip_initial_row_x64(DW_CFIMachine *machine, DW_CFIRow *initial_row);
internal void          dw_unwind_machine_equip_fde_ip_x64(DW_CFIMachine *machine, U64 fde_ip);

internal DW_CFIRow* dw_unwind_row_alloc_x64(Arena *arena, U64 cells_per_row);
internal void       dw_unwind_row_zero_x64(DW_CFIRow *row, U64 cells_per_row);
internal void       dw_unwind_row_copy_x64(DW_CFIRow *dst, DW_CFIRow *src, U64 cells_per_row);

internal B32 dw_unwind_machine_run_to_ip_x64(void *base, Rng1U64 range, DW_CFIMachine *machine, U64 target_ip, DW_CFIRow *row_out);

#endif // DWARF_UNWIND_H

