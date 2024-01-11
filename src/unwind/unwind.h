// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef UNWIND_H
#define UNWIND_H

////////////////////////////////
//~ allen: Unwind Types

// * applies to (any X,Y: unwind(X, Y))

typedef struct UNW_MemView{
  // Upgrade Path:
  //  1. A list of ranges like this one
  //  2. Binary-searchable list of ranges
  //  3. In-line growth strategy for missing pages (hardwired to source of new data)
  //  4. Abstracted source of new data
  void *data;
  U64 addr_first;
  U64 addr_opl;
} UNW_MemView;

typedef struct UNW_Result{
  B32 dead;
  B32 missed_read;
  U64 missed_read_addr;
  U64 stack_pointer;
} UNW_Result;


////////////////////////////////
//~ allen: X64 Unwind Types

// * applies to (any X: unwind(X, X64))

typedef REGS_RegBlockX64 UNW_X64_Regs;




////////////////////////////////
//~ allen: PE X64 Unwind Types

//- pe format unwind types

#define UNW_PE_OpCodeXList(X) \
X(PUSH_NONVOL    , 0) \
X(ALLOC_LARGE    , 1) \
X(ALLOC_SMALL    , 2) \
X(SET_FPREG      , 3) \
X(SAVE_NONVOL    , 4) \
X(SAVE_NONVOL_FAR, 5) \
X(EPILOG         , 6) \
X(SPARE_CODE     , 7) \
X(SAVE_XMM128    , 8) \
X(SAVE_XMM128_FAR, 9) \
X(PUSH_MACHFRAME , 10)

#define UNW_PE_CODE_FROM_FLAGS(f) ((f)&0xF)
#define UNW_PE_INFO_FROM_FLAGS(f) (((f) >> 4)&0xF)

typedef U32 UNW_PE_OpCode;
enum UNW_PE_OpCodeEnum{
#define X(N,C) UNW_PE_OpCode_##N = C,
  UNW_PE_OpCodeXList(X)
#undef X
};

typedef union UNW_PE_Code{
  struct{
    U8 off_in_prolog;
    U8 flags;
  };
  U16 u16;
} UNW_PE_Code;

typedef U8 UNW_PE_InfoFlags;
enum UNW_PE_InfoFlagsEnum{
  UNW_PE_InfoFlag_EHANDLER = (1 << 0),
  UNW_PE_InfoFlag_UHANDLER = (1 << 1),
  UNW_PE_InfoFlag_FHANDLER = 3,
  UNW_PE_InfoFlag_CHAINED  = (1 << 2),
} UNW_PE_InfoFlagsEnum;

#define UNW_PE_INFO_VERSION_FROM_HDR(x) ((x)&0x7)
#define UNW_PE_INFO_FLAGS_FROM_HDR(x)   (((x) >> 3)&0x1F)
#define UNW_PE_INFO_REG_FROM_FRAME(x)   ((x)&0xF)
#define UNW_PE_INFO_OFF_FROM_FRAME(x)   (((x) >> 4)&0xF)

typedef struct UNW_PE_Info{
  U8 header;
  U8 prolog_size;
  U8 codes_num;
  U8 frame;
} UNW_PE_Info;


////////////////////////////////
//~ allen: PE X64 Unwind Types

// * applies to (unwind(PE, X64))

typedef U8 UNW_PE_X64_GprReg;
enum{
  UNW_PE_X64_GprReg_RAX = 0,
  UNW_PE_X64_GprReg_RCX = 1,
  UNW_PE_X64_GprReg_RDX = 2,
  UNW_PE_X64_GprReg_RBX = 3,
  UNW_PE_X64_GprReg_RSP = 4,
  UNW_PE_X64_GprReg_RBP = 5,
  UNW_PE_X64_GprReg_RSI = 6,
  UNW_PE_X64_GprReg_RDI = 7,
  UNW_PE_X64_GprReg_R8  = 8,
  UNW_PE_X64_GprReg_R9  = 9,
  UNW_PE_X64_GprReg_R10 = 10,
  UNW_PE_X64_GprReg_R11 = 11,
  UNW_PE_X64_GprReg_R12 = 12,
  UNW_PE_X64_GprReg_R13 = 13,
  UNW_PE_X64_GprReg_R14 = 14,
  UNW_PE_X64_GprReg_R15 = 15,
};




////////////////////////////////
//~ allen: ELF/DW Unwind Types

// * applies to (any X: unwind(ELF/DW, X))

// EH: Exception Frames
typedef U8 UNW_DW_EhPtrEnc;
enum{
  UNW_DW_EhPtrEnc_TYPE_MASK = 0x0F,
  UNW_DW_EhPtrEnc_PTR     = 0x00, // Pointer sized unsigned value
  UNW_DW_EhPtrEnc_ULEB128 = 0x01, // Unsigned LE base-128 value
  UNW_DW_EhPtrEnc_UDATA2  = 0x02, // Unsigned 16-bit value
  UNW_DW_EhPtrEnc_UDATA4  = 0x03, // Unsigned 32-bit value
  UNW_DW_EhPtrEnc_UDATA8  = 0x04, // Unsigned 64-bit value
  UNW_DW_EhPtrEnc_SIGNED  = 0x08, // Signed pointer
  UNW_DW_EhPtrEnc_SLEB128 = 0x09, // Signed LE base-128 value
  UNW_DW_EhPtrEnc_SDATA2  = 0x0A, // Signed 16-bit value
  UNW_DW_EhPtrEnc_SDATA4  = 0x0B, // Signed 32-bit value
  UNW_DW_EhPtrEnc_SDATA8  = 0x0C, // Signed 64-bit value
};
enum{
  UNW_DW_EhPtrEnc_MODIF_MASK = 0x70,
  UNW_DW_EhPtrEnc_PCREL   = 0x10, // Value is relative to the current program counter.
  UNW_DW_EhPtrEnc_TEXTREL = 0x20, // Value is relative to the .text section.
  UNW_DW_EhPtrEnc_DATAREL = 0x30, // Value is relative to the .got or .eh_frame_hdr section.
  UNW_DW_EhPtrEnc_FUNCREL = 0x40, // Value is relative to the function.
  UNW_DW_EhPtrEnc_ALIGNED = 0x50, // Value is aligned to an address unit sized boundary.
};
enum{
  UNW_DW_EhPtrEnc_INDIRECT = 0x80, // This flag indicates that value is stored in virtual memory.
  UNW_DW_EhPtrEnc_OMIT     = 0xFF,
};

typedef struct UNW_DW_EhPtrCtx{
  U64 raw_base_vaddr; // address where pointer is being read
  U64 text_vaddr;     // base address of section with instructions (used for encoding pointer on SH and IA64)
  U64 data_vaddr;     // base address of data section (used for encoding pointer on x86-64)
  U64 func_vaddr;     // base address of function where IP is located
} UNW_DW_EhPtrCtx;

// CIE: Common Information Entry
typedef struct UNW_DW_CIEUnpacked{
  U8 version;
  UNW_DW_EhPtrEnc lsda_encoding;
  UNW_DW_EhPtrEnc addr_encoding;
  
  B8 has_augmentation_size;
  U64 augmentation_size;
  String8 augmentation;
  
  U64 code_align_factor;
  S64 data_align_factor;
  U64 ret_addr_reg;
  
  U64 handler_ip;
  
  U64 cfi_range_min;
  U64 cfi_range_max;
} UNW_DW_CIEUnpacked;

typedef struct UNW_DW_CIEUnpackedNode{
  struct UNW_DW_CIEUnpackedNode *next;
  UNW_DW_CIEUnpacked cie;
  U64 offset;
} UNW_DW_CIEUnpackedNode;

// FDE: Frame Description Entry
typedef struct UNW_DW_FDEUnpacked{
  U64 ip_voff_min;
  U64 ip_voff_max;
  U64 lsda_ip;
  
  U64 cfi_range_min;
  U64 cfi_range_max;
} UNW_DW_FDEUnpacked;

// CFI: Call Frame Information
typedef struct UNW_DW_CFIRecords{
  B32 valid;
  UNW_DW_CIEUnpacked cie;
  UNW_DW_FDEUnpacked fde;
} UNW_DW_CFIRecords;

typedef enum UNW_DW_CFICFARule{
  UNW_DW_CFICFARule_REGOFF,
  UNW_DW_CFICFARule_EXPR,
} UNW_DW_CFICFARule;

typedef struct UNW_DW_CFICFACell{
  UNW_DW_CFICFARule rule;
  union{
    struct{
      U64 reg_idx;
      S64 offset;
    };
    U64 expr_min;
    U64 expr_max;
  };
} UNW_DW_CFICFACell;

typedef enum UNW_DW_CFIRegisterRule{
  UNW_DW_CFIRegisterRule_SAME_VALUE,
  UNW_DW_CFIRegisterRule_UNDEFINED,
  UNW_DW_CFIRegisterRule_OFFSET,
  UNW_DW_CFIRegisterRule_VAL_OFFSET,
  UNW_DW_CFIRegisterRule_REGISTER,
  UNW_DW_CFIRegisterRule_EXPRESSION,
  UNW_DW_CFIRegisterRule_VAL_EXPRESSION,
} UNW_DW_CFIRegisterRule;

typedef struct UNW_DW_CFICell{
  UNW_DW_CFIRegisterRule rule;
  union{
    S64 n;
    struct{
      U64 expr_min;
      U64 expr_max;
    };
  };
} UNW_DW_CFICell;

typedef struct UNW_DW_CFIRow{
  struct UNW_DW_CFIRow *next;
  UNW_DW_CFICell *cells;
  UNW_DW_CFICFACell cfa_cell;
} UNW_DW_CFIRow;

typedef struct UNW_DW_CFIMachine{
  U64 cells_per_row;
  UNW_DW_CIEUnpacked *cie;
  UNW_DW_EhPtrCtx *ptr_ctx;
  UNW_DW_CFIRow *initial_row;
  U64 fde_ip;
} UNW_DW_CFIMachine;

typedef U8 UNW_DW_CFADecode;
enum{
  UNW_DW_CFADecode_NOP     = 0x0,
  // 1,2,4,8 reserved for literal byte sizes
  UNW_DW_CFADecode_ADDRESS = 0x9,
  UNW_DW_CFADecode_ULEB128 = 0xA,
  UNW_DW_CFADecode_SLEB128 = 0xB,
};

typedef U16 UNW_DW_CFAControlBits;
enum{
  UNW_DW_CFAControlBits_DEC1_MASK = 0x00F,
  UNW_DW_CFAControlBits_DEC2_MASK = 0x0F0,
  UNW_DW_CFAControlBits_IS_REG_0  = 0x100,
  UNW_DW_CFAControlBits_IS_REG_1  = 0x200,
  UNW_DW_CFAControlBits_IS_REG_2  = 0x400,
  UNW_DW_CFAControlBits_NEW_ROW   = 0x800,
};



////////////////////////////////
//~ allen: Unwind Functions

//- mem view construction
internal UNW_MemView unw_memview_from_data(String8 data, U64 base_vaddr);

//- mem view user face for unwind users
internal B32 unw_memview_read(UNW_MemView *memview, U64 addr, U64 size, void *out);

#define unw_memview_read_struct(v,addr,p) unw_memview_read((v), (addr), sizeof(*(p)), (p))


////////////////////////////////
//~ allen: PE X64 Unwind Functions

//- main interface
internal UNW_Result unw_pe_x64(String8 bindata, PE_BinInfo *bin,
                               U64 base_vaddr, UNW_MemView *memview,
                               UNW_X64_Regs *regs_inout);


//- pe x64 helpers
internal UNW_Result unw_pe_x64__epilog(String8 bindata, PE_BinInfo *bin,
                                       U64 base_vaddr, UNW_MemView*memview,
                                       UNW_X64_Regs *regs_inout);

internal U32 unw_pe_x64__slot_count_from_op_code(UNW_PE_OpCode opcode);
internal B32 unw_pe_x64__voff_is_in_epilog(String8 bindata, PE_BinInfo *bin,
                                           U64 voff, PE_IntelPdata *final_pdata);

internal REGS_Reg64 *unw_pe_x64__gpr_reg(UNW_X64_Regs *regs, UNW_PE_X64_GprReg unw_reg);

#endif //UNWIND_H
