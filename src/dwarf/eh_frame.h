// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EH_FRAME_H
#define EH_FRAME_H

////////////////////////////////
//~ Format

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
  U64 pc_vaddr;   // address where pointer is being read
  U64 text_vaddr; // base address of section with instructions (used for encoding pointer on SH and IA64)
  U64 data_vaddr; // base address of data section (used for encoding pointer on x86-64)
  U64 func_vaddr; // base address of function where IP is located
  U64 ptr_align;
} EH_PtrCtx;

typedef U8 EH_AugFlags;
enum
{
  EH_AugFlag_HasLSDA     = (1 << 0),
  EH_AugFlag_HasHandler  = (1 << 1),
  EH_AugFlag_HasAddrEnc  = (1 << 2),
  EH_AugFlag_SignalFrame = (1 << 3),
};

typedef struct EH_Augmentation
{
  EH_AugFlags flags;
  U64         handler_ip;
  EH_PtrEnc   handler_encoding;
  EH_PtrEnc   lsda_encoding;
  EH_PtrEnc   addr_encoding;
  U64         size;
} EH_Augmentation;

////////////////////////////////
//~ Parser

enum
{
  EH_CIE_Ext_AddrEnc,
  EH_CIE_Ext_LSDAEnc,
  EH_CIE_Ext_HandlerEnc,
  EH_CIE_Ext_HandlerIp,
} EH_CIE_Ext;

typedef struct EH_FrameHdrEntry
{
  U64 addr;
  U64 fde_offset;
} EH_FrameHdrEntry;

typedef struct EH_FrameHdr
{
  U8        version;
  EH_PtrEnc eh_frame_ptr_enc;
  EH_PtrEnc fde_count_enc;
  EH_PtrEnc table_enc;
  U64       field_byte_size;
  U64       entry_byte_size;
  U64       fde_count;
  U64       eh_frame_ptr;
  String8   table;
} EH_FrameHdr;

typedef struct EH_DecodePtrCtx
{
  EH_PtrEnc  addr_enc;
  EH_PtrCtx *ptr_ctx;
} EH_DecodePtrCtx;

////////////////////////////////

internal U64         eh_parse_ptr(String8 frame_base, U64 off, U64 pc, EH_PtrCtx *ptr_ctx, EH_PtrEnc encoding, U64 *ptr_out);
internal EH_FrameHdr eh_parse_frame_hdr(String8 data, U64 address_size, EH_PtrCtx *ptr_ctx);
internal U64         eh_parse_aug_data(String8 aug_string, String8 aug_data, U64 pc, EH_PtrCtx *ptr_ctx, EH_Augmentation *aug_out);
internal B32         eh_parse_cie(String8 data, DW_Format format, Arch arch, U64 pc, EH_PtrCtx *ptr_ctx, DW_CIE *cie_out);
internal B32         eh_parse_fde(String8 data, DW_Format format, U64 pc, DW_CIE *cie, EH_PtrCtx *ptr_ctx, DW_FDE *fde_out);
internal U64         eh_find_nearest_fde(EH_FrameHdr header, EH_PtrCtx *ptr_ctx, U64 pc);
internal String8     eh_frame_hdr_from_call_frame_info(Arena *arena, U64 fde_count, U64 *fde_offsets, struct DW_FDE *fde);

////////////////////////////////
//~ Enum -> String

internal String8 eh_string_from_ptr_enc_type(EH_PtrEnc type);
internal String8 eh_string_from_ptr_enc_modifier(EH_PtrEnc modifier);
internal String8 eh_string_from_ptr_enc(Arena *arena, EH_PtrEnc enc);

#endif // EH_FRAME_H
