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

typedef U8 DW_EhPtrEnc;
enum
{
  DW_EhPtrEnc_TypeMask = 0x0F,
  DW_EhPtrEnc_Ptr       = 0x00, // Pointer sized unsigned value
  DW_EhPtrEnc_ULEB128   = 0x01, // Unsigned LE base-128 value
  DW_EhPtrEnc_UData2    = 0x02, // Unsigned 16-bit value
  DW_EhPtrEnc_UData4    = 0x03, // Unsigned 32-bit value
  DW_EhPtrEnc_UData8    = 0x04, // Unsigned 64-bit value
  DW_EhPtrEnc_Signed    = 0x08, // Signed pointer
  DW_EhPtrEnc_SLEB128   = 0x09, // Signed LE base-128 value
  DW_EhPtrEnc_SData2    = 0x0A, // Signed 16-bit value
  DW_EhPtrEnc_SData4    = 0x0B, // Signed 32-bit value
  DW_EhPtrEnc_SData8    = 0x0C, // Signed 64-bit value
};

enum
{
  DW_EhPtrEnc_ModifyMask = 0x70,
  DW_EhPtrEnc_PcRel      = 0x10, // Value is relative to the current program counter.
  DW_EhPtrEnc_TextRel    = 0x20, // Value is relative to the .text section.
  DW_EhPtrEnc_DataRel    = 0x30, // Value is relative to the .got or .eh_frame_hdr section.
  DW_EhPtrEnc_FuncRel    = 0x40, // Value is relative to the function.
  DW_EhPtrEnc_Aligned    = 0x50, // Value is aligned to an address unit sized boundary.
};

enum
{
  DW_EhPtrEnc_Indirect = 0x80, // This flag indicates that value is stored in virtual memory.
  DW_EhPtrEnc_Omit     = 0xFF,
};

typedef struct DW_EhPtrCtx
{
  U64 raw_base_vaddr; // address where pointer is being read
  U64 text_vaddr;     // base address of section with instructions (used for encoding pointer on SH and IA64)
  U64 data_vaddr;     // base address of data section (used for encoding pointer on x86-64)
  U64 func_vaddr;     // base address of function where IP is located
} DW_EhPtrCtx;

// CIE: Common Information Entry
typedef struct DW_CIEUnpacked
{
  U8          version;
  DW_EhPtrEnc lsda_encoding;
  DW_EhPtrEnc addr_encoding;
  
  B32     has_augmentation_size;
  U64     augmentation_size;
  String8 augmentation;
  
  U64 code_align_factor;
  S64 data_align_factor;
  U64 ret_addr_reg;
  
  U64 handler_ip;
  
  Rng1U64 cfi_range;
} DW_CIEUnpacked;

typedef struct DW_CIEUnpackedNode
{
  struct DW_CIEUnpackedNode *next;
  DW_CIEUnpacked             cie;
  U64                        offset;
} DW_CIEUnpackedNode;

// FDE: Frame Description Entry
typedef struct DW_FDEUnpacked
{
  Rng1U64 ip_voff_range;
  U64     lsda_ip;
  Rng1U64 cfi_range;
} DW_FDEUnpacked;

// CFI: Call Frame Information
typedef struct DW_CFIRecords
{
  B32            valid;
  DW_CIEUnpacked cie;
  DW_FDEUnpacked fde;
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
  DW_CIEUnpacked *cie;
  DW_EhPtrCtx    *ptr_ctx;
  DW_CFIRow      *initial_row;
  U64             fde_ip;
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
internal U64  dw_unwind_parse_pointer_x64(void *base, Rng1U64 range, DW_EhPtrCtx *ptr_ctx, DW_EhPtrEnc ptr_enc, U64 off, U64 *ptr_out);

//- eh_frame parsing
internal void dw_unwind_parse_cie_x64(void *base,Rng1U64 range,DW_EhPtrCtx *ptr_ctx, U64 off, DW_CIEUnpacked *cie_out);
internal void dw_unwind_parse_fde_x64(void *base,Rng1U64 range,DW_EhPtrCtx *ptr_ctx, DW_CIEUnpacked *parent_cie, U64 off, DW_FDEUnpacked *fde_out);
internal DW_CFIRecords dw_unwind_eh_frame_cfi_from_ip_slow_x64(String8 raw_eh_frame, DW_EhPtrCtx *ptr_ctx, U64 ip_voff);
internal DW_CFIRecords dw_unwind_eh_frame_hdr_from_ip_fast_x64(String8 raw_eh_frame, String8 raw_eh_frame_hdr, DW_EhPtrCtx *ptr_ctx, U64 ip_voff);

//- cfi machine

internal DW_CFIMachine dw_unwind_make_machine_x64(U64 cells_per_row, DW_CIEUnpacked *cie, DW_EhPtrCtx *ptr_ctx);
internal void          dw_unwind_machine_equip_initial_row_x64(DW_CFIMachine *machine, DW_CFIRow *initial_row);
internal void          dw_unwind_machine_equip_fde_ip_x64(DW_CFIMachine *machine, U64 fde_ip);

internal DW_CFIRow* dw_unwind_row_alloc_x64(Arena *arena, U64 cells_per_row);
internal void       dw_unwind_row_zero_x64(DW_CFIRow *row, U64 cells_per_row);
internal void       dw_unwind_row_copy_x64(DW_CFIRow *dst, DW_CFIRow *src, U64 cells_per_row);

internal B32 dw_unwind_machine_run_to_ip_x64(void *base, Rng1U64 range, DW_CFIMachine *machine, U64 target_ip, DW_CFIRow *row_out);

#endif // DWARF_UNWIND_H

