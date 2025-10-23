// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal GNU_LinkMap64
gnu_linkmap64_from_linkmap32(GNU_LinkMap32 linkmap32)
{
  GNU_LinkMap64 linkmap64 = {0};
  linkmap64.addr_vaddr = linkmap32.addr_vaddr;
  linkmap64.name_vaddr = linkmap32.name_vaddr;
  linkmap64.ld_vaddr   = linkmap32.ld_vaddr;
  linkmap64.next_vaddr = linkmap32.next_vaddr;
  linkmap64.prev_vaddr = linkmap32.prev_vaddr;
  return linkmap64;
}

internal U64
gnu_rdebug_info_size_from_arch(Arch arch)
{
  U64 size = 0;
  switch (byte_size_from_arch(arch)) {
  case 0: break;
  case 4: size = sizeof(GNU_RDebugInfo32); break;
  case 8: size = sizeof(GNU_RDebugInfo64); break;
  default: InvalidPath; break;
  }
  return size;
}

internal U64
gnu_r_brk_offset_from_arch(Arch arch)
{
  U64 offset = 0;
  switch (gnu_rdebug_info_size_from_arch(arch)) {
  case 0: offset = 0; break;
  case sizeof(GNU_RDebugInfo32): offset = OffsetOf(GNU_RDebugInfo32, r_brk); break;
  case sizeof(GNU_RDebugInfo64): offset = OffsetOf(GNU_RDebugInfo64, r_brk); break;
  default: InvalidPath; break;
  }
  return offset;
}

internal GNU_RDebugInfo64
gnu_rdebug_info64_from_rdebug_info32(GNU_RDebugInfo32 rdebug32)
{
  GNU_RDebugInfo64 result = {0};
  result.r_version = rdebug32.r_version;
  result.r_map     = rdebug32.r_map;
  result.r_brk     = rdebug32.r_brk;
  result.r_state   = rdebug32.r_state;
  result.r_ldbase  = rdebug32.r_ldbase;
  return result;
}

