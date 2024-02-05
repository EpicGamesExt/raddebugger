////////////////////////////////
//~ rjf: Helpers

internal B32
demon_os_read_regs(DEMON_Entity *thread, void *dst)
{
  B32 result = 0;
  switch(thread->arch)
  {
    default:{}break;
    case Architecture_x86:{result = demon_os_read_regs_x86(thread, (REGS_RegBlockX86 *)dst);}break;
    case Architecture_x64:{result = demon_os_read_regs_x64(thread, (REGS_RegBlockX64 *)dst);}break;
  }
  return result;
}

internal B32
demon_os_write_regs(DEMON_Entity *thread, void *src)
{
  B32 result = 0;
  switch(thread->arch)
  {
    default:{}break;
    case Architecture_x86:{result = demon_os_write_regs_x86(thread, (REGS_RegBlockX86 *)src);}break;
    case Architecture_x64:{result = demon_os_write_regs_x64(thread, (REGS_RegBlockX64 *)src);}break;
  }
  return result;
}
