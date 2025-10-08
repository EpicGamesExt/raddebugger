// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EH_FRAME_H
#define EH_FRAME_H

typedef U8 EH_PtrEnc;
enum
{
  EH_PtrEnc_Ptr     = 0x00, // Pointer sized unsigned value
  EH_PtrEnc_ULEB128 = 0x01, // Unsigned LE base-128 value
  EH_PtrEnc_UData2  = 0x02, // Unsigned 16-bit value
  EH_PtrEnc_UData4  = 0x03, // Unsigned 32-bit value
  EH_PtrEnc_UData8  = 0x04, // Unsigned 64-bit value
  EH_PtrEnc_Signed  = 0x08, // Signed pointer
  EH_PtrEnc_SLEB128 = 0x09, // Signed LE base-128 value
  EH_PtrEnc_SData2  = 0x0A, // Signed 16-bit value
  EH_PtrEnc_SData4  = 0x0B, // Signed 32-bit value
  EH_PtrEnc_SData8  = 0x0C, // Signed 64-bit value
 
  EH_PtrEnc_TypeMask = 0x0F,
};

enum
{
  EH_PtrEnc_PcRel   = 0x10, // Value is relative to the current program counter.
  EH_PtrEnc_TextRel = 0x20, // Value is relative to the .text section.
  EH_PtrEnc_DataRel = 0x30, // Value is relative to the .got or .eh_frame_hdr section.
  EH_PtrEnc_FuncRel = 0x40, // Value is relative to the function.
  EH_PtrEnc_Aligned = 0x50, // Value is aligned to an address unit sized boundary.
                                 
  EH_PtrEnc_ModifierMask = 0x70,
};

enum
{
  EH_PtrEnc_Indirect = 0x80, // Value is stored in virtual memory.
  EH_PtrEnc_Omit     = 0xFF,
};

typedef struct EH_PtrCtx
{
  U64 raw_base_vaddr; // address where pointer is being read
  U64 text_vaddr;     // base address of section with instructions (used for encoding pointer on SH and IA64)
  U64 data_vaddr;     // base address of data section (used for encoding pointer on x86-64)
  U64 func_vaddr;     // base address of function where IP is located
  U64 ptr_align;
} EH_PtrCtx;

typedef U8 EH_AugFlags;
enum
{
  EH_AugFlag_HasLSDA    = (1 << 0),
  EH_AugFlag_HasHandler = (1 << 1),
  EH_AugFlag_HasAddrEnc = (1 << 2),
};

typedef struct EH_Augmentation
{
  EH_AugFlags flags;
  U64         handler_ip;
  EH_PtrEnc   handler_encoding;
  EH_PtrEnc   lsda_encoding;
  EH_PtrEnc   addr_encoding;
} EH_Augmentation;

////////////////////////////////

internal U64 eh_read_ptr(String8 frame_base, U64 off, EH_PtrCtx *ptr_ctx, EH_PtrEnc encoding, U64 *ptr_out);
internal U64 eh_parse_aug_data(String8 aug_string, String8 aug_data, EH_PtrCtx *ptr_ctx, EH_Augmentation *aug_out);
internal U64 eh_size_from_aug_data(String8 aug_string, String8 data, EH_PtrCtx *ptr_ctx);

#endif // EH_FRAME_H

