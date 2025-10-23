// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef X64_H
#define X64_H

////////////////////////////////
//~ RFlags

typedef U64 X64_RFlags;
enum
{
  X64_RFlag_Carry       = (1 << 0),
  X64_RFlag_Parity      = (1 << 2),
  X64_RFlag_AuxCarry    = (1 << 4),
  X64_RFlag_Zero        = (1 << 6),
  X64_RFlag_Sign        = (1 << 7),
  X64_RFlag_Trap        = (1 << 8),
  X64_RFlag_Interrupt   = (1 << 9),
  X64_RFlag_Direction   = (1 << 10),
  X64_RFlag_Overflow    = (1 << 11),
  X64_RFlag_IoPrivilege = (3 << 12),
  X64_RFlag_NT          = (1 << 14),
  X64_RFlag_MD          = (1 << 15),
  X64_RFlag_RF          = (1 << 16),
  X64_RFlag_VM          = (1 << 17),
  X64_RFlag_AC          = (1 << 18),
  X64_RFlag_VIF         = (1 << 19),
  X64_RFlag_VIP         = (1 << 20),
  X64_RFlag_AES         = (1 << 30),
  X64_RFlag_AI          = (1 << 31),
};

////////////////////////////////
//~ fxsave/xsave

typedef struct AlignType(16) X64_FXSave
{
  U16 fcw;
  U16 fsw;
  U16 ftw;
  U16 fop;
  union {
    struct {
      U64 fip;
      U64 fdp;
    } b64;
    struct {
      U32 fip;
      U16 fcs, _pad0;
      U32 fdp;
      U16 fds, _pad1;
    } b32;
  };
  U32  mxcsr;
  U32  mxcsr_mask;
  U128 st_space[8];
  U128 xmm_space[16];
  U8   padding[96];
} X64_FXSave;
StaticAssert(sizeof(X64_FXSave) == 512, g_x64_xsave_legacy_size_check);

typedef struct X64_XSaveHeader
{
  U64 xstate_bv;
  U64 xcomp_bv;
  U8  reserved[48];
} X64_XSaveHeader;
StaticAssert(sizeof(X64_XSaveHeader) == 64, g_x64_xsave_header_size_check);

typedef struct AlignType(64) X64_XSave
{
  X64_FXSave      fxsave;
  X64_XSaveHeader header;
  // U8 ext_area[0];
} X64_XSave;
StaticAssert(sizeof(X64_XSave) == 576, g_x64_xsave_size_check);

typedef U32 X64_XStateComponentIdx;
enum
{
  X64_XStateComponentIdx_FP       = 0,
  X64_XStateComponentIdx_SSE      = 1,
  X64_XStateComponentIdx_AVX      = 2,
  X64_XStateComponentIdx_BNDREGS  = 3,
  X64_XStateComponentIdx_BNDCSR   = 4,
  X64_XStateComponentIdx_OPMASK   = 5,
  X64_XStateComponentIdx_ZMM_H    = 6,
  X64_XStateComponentIdx_ZMM      = 7,
  X64_XStateComponentIdx_PT       = 8,
  X64_XStateComponentIdx_PKRU     = 9,
  X64_XStateComponentIdx_PASID    = 10,
  X64_XStateComponentIdx_CETU     = 11,
  X64_XStateComponentIdx_CETS     = 12,
  X64_XStateComponentIdx_HDC      = 13,
  X64_XStateComponentIdx_UINTR    = 14,
  X64_XStateComponentIdx_LBR      = 15,
  X64_XStateComponentIdx_HWP      = 16,
  X64_XStateComponentIdx_TILECFG  = 17,
  X64_XStateComponentIdx_TILEDATA = 18,
};

enum
{
  X64_XStateComponentFlag_FP       = (1 << X64_XStateComponentIdx_FP),
  X64_XStateComponentFlag_SSE      = (1 << X64_XStateComponentIdx_SSE),
  X64_XStateComponentFlag_AVX      = (1 << X64_XStateComponentIdx_AVX),
  X64_XStateComponentFlag_BNDREGS  = (1 << X64_XStateComponentIdx_BNDREGS),
  X64_XStateComponentFlag_BNDCSR   = (1 << X64_XStateComponentIdx_BNDCSR),
  X64_XStateComponentFlag_OPMASK   = (1 << X64_XStateComponentIdx_OPMASK),
  X64_XStateComponentFlag_ZMM_H    = (1 << X64_XStateComponentIdx_ZMM_H),
  X64_XStateComponentFlag_ZMM      = (1 << X64_XStateComponentIdx_ZMM),
  X64_XStateComponentFlag_PT       = (1 << X64_XStateComponentIdx_PT),
  X64_XStateComponentFlag_PKRU     = (1 << X64_XStateComponentIdx_PKRU),
  X64_XStateComponentFlag_PASID    = (1 << X64_XStateComponentIdx_PASID),
  X64_XStateComponentFlag_CETU     = (1 << X64_XStateComponentIdx_CETU),
  X64_XStateComponentFlag_CETS     = (1 << X64_XStateComponentIdx_CETS),
  X64_XStateComponentFlag_HDC      = (1 << X64_XStateComponentIdx_HDC),
  X64_XStateComponentFlag_UINTR    = (1 << X64_XStateComponentIdx_UINTR),
  X64_XStateComponentFlag_LBR      = (1 << X64_XStateComponentIdx_LBR),
  X64_XStateComponentFlag_HWP      = (1 << X64_XStateComponentIdx_HWP),
  X64_XStateComponentFlag_TILECFG  = (1 << X64_XStateComponentIdx_TILECFG),
  X64_XStateComponentFlag_TILEDATA = (1 << X64_XStateComponentIdx_TILEDATA),
};

typedef struct
{
  U64 x87_offset;
  U64 sse_offset;
  U64 avx_offset;
  U64 bndregs_offset;
  U64 bndcfg_offset;
  U64 opmask_offset;
  U64 zmm_h_offset;
  U64 zmm_offset;
  U64 pt_offset;
  U64 pkru_offset;
  U64 pasid_offset;
  U64 cet_u_offset;
  U64 cet_s_offset;
  U64 hdc_offset;
  U64 uintr_offset;
  U64 lbr_offset;
  U64 hwp_offset;
  U64 tile_cfg_offset;
  U64 tile_data_offset;
} X64_XSaveLayout;

////////////////////////////////
//~ Debug Status Register

typedef enum
{
  X64_DebugStatusFlag_B0                  = (1 << 0),
  X64_DebugStatusFlag_B1                  = (1 << 1),
  X64_DebugStatusFlag_B2                  = (1 << 2),
  X64_DebugStatusFlag_B3                  = (1 << 3),
  X64_DebugStatusFlag_BusLock             = (1 << 11),
  X64_DebugStatusFlag_DebugRegisterAccess = (1 << 13),
  X64_DebugStatusFlag_SingleStep          = (1 << 14),
  X64_DebugStatusFlag_TaskSwitch          = (1 << 15),
  X64_DebugStatusFlag_RTM                 = (1 << 16),
} X64_DebugStatusFlags;

////////////////////////////////
//~ Debug Control Register

typedef enum X64_BreakpointType {
  X64_BreakpointType_Null,
  X64_BreakpointType_Local,
  X64_BreakpointType_Global,
} X64_BreakpointType;

typedef enum X64_DebugBreakType
{
  X64_DebugBreakType_Null,
  X64_DebugBreakType_Exec,
  X64_DebugBreakType_Write,
  X64_DebugBreakType_ReadWriteIO,
  X64_DebugBreakType_ReadWriteNoFetch,
} X64_DebugBreakType;

typedef enum X64_DebugControlFlags
{
  X64_DebugControlFlag_L0 = (1 << 0),
  X64_DebugControlFlag_L1 = (1 << 2),
  X64_DebugControlFlag_L2 = (1 << 4),
  X64_DebugControlFlag_L3 = (1 << 6),

  X64_DebugControlFlag_G0 = (1 << 1),
  X64_DebugControlFlag_G1 = (1 << 3),
  X64_DebugControlFlag_G3 = (1 << 5),
  X64_DebugControlFlag_G4 = (1 << 7),

  X64_DebugControlFlag_LE = (1 << 8),
  X64_DebugControlFlag_GE = (1 << 9),

  X64_DebugControlFlag_RTM = (1 << 11),

  X64_DebugControlFlag_GD = (1 << 13),

  X64_DebugControlFlag_RW0 = (3 << 16),
  X64_DebugControlFlag_RW1 = (3 << 20),
  X64_DebugControlFlag_RW2 = (3 << 24),
  X64_DebugControlFlag_FW3 = (3 << 26),

  X64_DebugControlFlag_LEN0 = (3 << 18),
  X64_DebugControlFlag_LEN1 = (3 << 22),
  X64_DebugControlFlag_LEN2 = (3 << 26),
  X64_DebugControlFlag_LEN3 = (3 << 30),
} X64_DebugControlFlags;

////////////////////////////////
//~ cpuid

internal void x64_cpuid(U32 leaf, U32 *eax, U32 *ebx, U32 *ecx, U32 *edx);
internal void x64_cpuid_ex(U32 leaf, U32 sub_leaf, U32 *eax, U32 *ebx, U32 *ecx, U32 *edx);

////////////////////////////////
//~ fxsave/xsave

internal B32 x64_is_xsave_supported(void);
internal U32 x64_get_xsave_size(void);
internal U16 x64_xsave_tag_word_from_real_tag_word(U16 ftw);
internal U32 x64_xsave_offset_from_feature_flag(U64 xcr0, X64_XStateComponentIdx comp_idx);
internal X64_XSaveLayout x64_get_xsave_layout(U64 xcr0);

////////////////////////////////
//~ Debug Control Register

internal void x64_set_debug_break(U64 *drs, U64 trap_idx, U64 addr, U64 size, X64_BreakpointType bp_type, X64_DebugBreakType break_type);

#endif // BASE_X64_H

