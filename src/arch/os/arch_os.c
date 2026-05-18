// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
arch_os_write_reg_block_from_thread_ctx(Arch arch, OperatingSystem os, void *reg_block, void *thread_ctx)
{
  B32 result = 0;
  if(0){}
#define Case(arch_, os_, impl) else if((arch) == arch_ && (os) == os_) do {result = impl(reg_block, thread_ctx);}while(0)
#if defined(X64_H) && defined(WIN32_X64_H)
  Case(Arch_x64, OperatingSystem_Windows, w32_x64_write_reg_block_from_thread_ctx);
#endif
#undef Case
  else{}
  return result;
}
