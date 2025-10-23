// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
x64_cpuid(U32 leaf, U32 *eax, U32 *ebx, U32 *ecx, U32 *edx)
{
  U32 info[4] = {0};
#if COMPILER_MSVC
  __cpuid(info, leaf);
  if (eax) { *eax = info[0]; }
  if (ebx) { *ebx = info[1]; }
  if (ecx) { *ecx = info[2]; }
  if (edx) { *edx = info[3]; }
#elif COMPILER_CLANG || COMPILER_GCC
  if (!eax) { eax = &info[0]; }
  if (!ebx) { ebx = &info[1]; }
  if (!ecx) { ecx = &info[2]; }
  if (!edx) { edx = &info[3]; }
  __get_cpuid(leaf, eax, ebx, ecx, edx);
#else
# error "cpuid is not defined for this compiler"
#endif
}

internal void
x64_cpuid_ex(U32 leaf, U32 sub_leaf, U32 *eax, U32 *ebx, U32 *ecx, U32 *edx)
{
  U32 info[4];
#if COMPILER_MSVC
  __cpuidex(info, leaf, sub_leaf);
  if (eax) { *eax = info[0]; }
  if (ebx) { *ebx = info[1]; }
  if (ecx) { *ecx = info[2]; }
  if (edx) { *edx = info[3]; }
#elif COMPILER_CLANG || COMPILER_GCC
  if (!eax) { eax = &info[0]; }
  if (!ebx) { ebx = &info[1]; }
  if (!ecx) { ecx = &info[2]; }
  if (!edx) { edx = &info[3]; }
  __get_cpuid_count(leaf, sub_leaf, eax, ebx, ecx, edx);
#else
# error "cpuid_count is not defined for this compiler"
#endif
}

internal B32
x64_is_xsave_supported(void)
{
  U32 ecx = 0;
  x64_cpuid(1, 0, 0, &ecx, 0);
  return !!(ecx & (1 << 26));
}

internal U32
x64_get_xsave_size(void)
{
  U32 xsave_size = 0;
  x64_cpuid_ex(0xd, 0, 0, &xsave_size, 0, 0);
  return xsave_size;
}

internal U16
x64_xsave_tag_word_from_real_tag_word(U16 ftw)
{
  U16 compact = 0;
  for EachIndex(fpr, 8) {
    U32 tag = (ftw >> (fpr * 2)) & 3;
    if (tag != 3) {
      compact |= (1 << fpr);
    }
  }
  return compact;
}

internal U32
x64_xsave_offset_from_feature_flag(U64 xcr0, X64_XStateComponentIdx comp_idx)
{
  U32 offset = 0;
  if (xcr0 & (1 << comp_idx)) {
    if(comp_idx == X64_XStateComponentIdx_FP) {
      offset = 0;
    } else if(comp_idx == X64_XStateComponentIdx_SSE) {
      offset = 160;
    } else {
      x64_cpuid_ex(0xd, comp_idx, 0, &offset, 0, 0);
    }
  }
  return offset;
}

internal X64_XSaveLayout
x64_get_xsave_layout(U64 xcr0)
{
  X64_XSaveLayout xsave_layout = {0};
  xsave_layout.x87_offset       = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_FP);
  xsave_layout.sse_offset       = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_SSE);
  xsave_layout.avx_offset       = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_AVX);
  xsave_layout.bndregs_offset   = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_BNDREGS);
  xsave_layout.bndcfg_offset    = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_BNDCSR);
  xsave_layout.opmask_offset    = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_OPMASK);
  xsave_layout.zmm_h_offset     = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_ZMM_H);
  xsave_layout.zmm_offset       = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_ZMM);
  xsave_layout.pt_offset        = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_PT);
  xsave_layout.pkru_offset      = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_PKRU);
  xsave_layout.pasid_offset     = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_PASID);
  xsave_layout.cet_u_offset     = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_CETU);
  xsave_layout.cet_s_offset     = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_CETS);
  xsave_layout.hdc_offset       = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_HDC);
  xsave_layout.uintr_offset     = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_UINTR);
  xsave_layout.lbr_offset       = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_LBR);
  xsave_layout.hwp_offset       = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_HWP);
  xsave_layout.tile_cfg_offset  = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_TILECFG);
  xsave_layout.tile_data_offset = x64_xsave_offset_from_feature_flag(xcr0, X64_XStateComponentIdx_TILEDATA);
  return xsave_layout;
}

internal void
x64_set_debug_break(U64 *drs, U64 trap_idx, U64 addr, U64 size, X64_BreakpointType bp_type, X64_DebugBreakType break_type)
{
  // set breakpoint address
  switch (trap_idx) {
  case 0: { drs[0] = addr; } break;
  case 1: { drs[1] = addr; } break;
  case 2: { drs[2] = addr; } break;
  case 3: { drs[3] = addr; } break;
  default: { InvalidPath; } break;
  }

  // enable breakpoint
  drs[7] &= ~(3 << (trap_idx * 2));
  switch (bp_type)
  {
  case X64_BreakpointType_Null: break;
  case X64_BreakpointType_Local:  { drs[7] |= 1 << (trap_idx * 2); } break;
  case X64_BreakpointType_Global: { drs[7] |= 2 << (trap_idx * 2); } break;
  default: { InvalidPath; } break;
  }

  // set break access type
  switch (break_type) {
  case X64_DebugBreakType_Exec:
    Assert(size == 0);
  case X64_DebugBreakType_Null: {
    drs[7] &= ~((1u << 16) << (trap_idx * 4));
    drs[7] &= ~((1u << 17) << (trap_idx * 4));
  } break;
  case X64_DebugBreakType_Write: {
    drs[7] |=   (1u << 16) << (trap_idx * 4);
    drs[7] &= ~((1u << 17) << (trap_idx * 4));
  } break;
  case X64_DebugBreakType_ReadWriteIO: {
    drs[7] &= ~((1u << 16) << (trap_idx * 4));
    drs[7] |=   (1u << 17) << (trap_idx * 4);
  } break;
  case X64_DebugBreakType_ReadWriteNoFetch: {
    drs[7] |= (1u << 16) << (trap_idx * 4);
    drs[7] |= (1u << 17) << (trap_idx * 4);
  } break;
  default: { InvalidPath; } break;
  }

  // set breakpoint size
  switch (size) {
  case 0:
  case 1: {
    drs[7] &= ~((1u << 18) << (trap_idx * 4));
    drs[7] &= ~((1u << 19) << (trap_idx * 4));
  } break;
  case 2: {
    drs[7] |=   (1 << 18) << (trap_idx * 4);
    drs[7] &= ~((1 << 19) << (trap_idx * 4));
  } break;
  case 4: {
    drs[7] |= (1 << 18) << (trap_idx * 4);
    drs[7] |= (1 << 19) << (trap_idx * 4);
  } break;
  case 8: {
    drs[7] &= ~((1 << 18) << (trap_idx * 4));
    drs[7] |=   (1 << 19) << (trap_idx * 4);
  } break;
  default: { InvalidPath; }
  }
}

