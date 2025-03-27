// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: 32 => 64 bit conversions

internal ELF_Hdr64
elf_hdr64_from_hdr32(ELF_Hdr32 h32)
{
  ELF_Hdr64 h64  = {0};
  MemoryCopy(h64.e_ident, h32.e_ident, sizeof(h64.e_ident));
  h64.e_type      = h32.e_type;
  h64.e_machine   = h32.e_machine;
  h64.e_version   = h32.e_version;
  h64.e_entry     = (U64)h32.e_entry;
  h64.e_phoff     = (U64)h32.e_phoff;
  h64.e_shoff     = (U64)h32.e_shoff;
  h64.e_flags     = h32.e_flags;
  h64.e_ehsize    = h32.e_ehsize;
  h64.e_phentsize = h32.e_phentsize;
  h64.e_phnum     = h32.e_phnum;
  h64.e_shentsize = h32.e_shentsize;
  h64.e_shnum     = h32.e_shnum;
  h64.e_shstrndx  = h32.e_shstrndx;
  return h64;
}

internal ELF_Shdr64
elf_shdr64_from_shdr32(ELF_Shdr32 h32)
{
  ELF_Shdr64 h64   = {0};
  h64.sh_name      = h32.sh_name;
  h64.sh_type      = h32.sh_type;
  h64.sh_flags     = (U64)h32.sh_flags;
  h64.sh_addr      = (U64)h32.sh_addr;
  h64.sh_offset    = (U64)h32.sh_offset;
  h64.sh_size      = (U64)h32.sh_size;
  h64.sh_link      = h32.sh_link;
  h64.sh_info      = h32.sh_info;
  h64.sh_addralign = (U64)h32.sh_addralign;
  h64.sh_entsize   = (U64)h32.sh_entsize;
  return h64;
}

internal ELF_Phdr64
elf_phdr64_from_phdr32(ELF_Phdr32 h32)
{
  ELF_Phdr64 h64 = {0};
  h64.p_type     = h32.p_type;
  h64.p_flags    = h32.p_flags;
  h64.p_offset   = (U64)h32.p_offset;
  h64.p_vaddr    = (U64)h32.p_vaddr;
  h64.p_paddr    = (U64)h32.p_paddr;
  h64.p_filesz   = (U64)h32.p_filesz;
  h64.p_memsz    = (U64)h32.p_memsz;
  h64.p_align    = (U64)h32.p_align;
  return h64;
}

internal ELF_Dyn64
elf_dyn64_from_dyn32(ELF_Dyn32 h32)
{
  ELF_Dyn64 h64 = {0};
  h64.tag       = (U64)h32.tag;
  h64.val       = (U64)h32.val;
  return h64;
}

internal ELF_Sym64
elf_sym64_from_sym32(ELF_Sym32 sym32)
{
  ELF_Sym64 sym64 = {0};
  sym64.st_name   = sym32.st_name;
  sym64.st_value  = sym32.st_value;
  sym64.st_size   = sym32.st_size;
  sym64.st_info   = sym32.st_info;
  sym64.st_other  = sym32.st_other;
  sym64.st_shndx  = sym32.st_shndx;
  return sym64;
}

internal ELF_Rel64
elf_rel64_from_rel32(ELF_Rel32 rel32)
{
  U32 sym  = ELF32_R_SYM(rel32.r_info);
  U32 type = ELF32_R_TYPE(rel32.r_info);
  ELF_Rel64 rel64 = {0};
  rel64.r_info    = ELF64_R_INFO(sym, type);
  rel64.r_offset  = rel32.r_offset;
  return rel64;
}

internal ELF_Rela64
elf_rela64_from_rela32(ELF_Rela32 rela32)
{
  U32 sym  = ELF32_R_SYM(rela32.r_info);
  U32 type = ELF32_R_TYPE(rela32.r_info);
  ELF_Rela64 rela64 = {0};
  rela64.r_offset   = rela32.r_info;
  rela64.r_info     = ELF64_R_INFO(sym, type);
  rela64.r_addend   = rela32.r_addend;
  return rela64;
}

internal ELF_Chdr64
elf_chdr64_from_chdr32(ELF_Chdr32 chdr32)
{
  ELF_Chdr64 chdr64    = {0};
  chdr64.ch_type       = chdr32.ch_type;
  chdr64.ch_size       = chdr32.ch_size;
  chdr64.ch_addr_align = chdr32.ch_addr_align;
  return chdr64;
}

////////////////////////////////

internal String8
elf_string_from_class(Arena *arena, ELF_Class v)
{
  switch (v) {
  case ELF_Class_None: return str8_lit("None");
  case ELF_Class_32:   return str8_lit("32Bit");
  case ELF_Class_64:   return str8_lit("64Bit");
  }
  return push_str8f(arena, "%#x", v);
}

////////////////////////////////

internal Arch
arch_from_elf_machine(ELF_MachineKind e_machine)
{
  Arch arch = Arch_Null;
  switch (e_machine) {
  case ELF_MachineKind_None:    arch = Arch_Null;  break;
  case ELF_MachineKind_AARCH64: arch = Arch_arm32; break;
  case ELF_MachineKind_ARM:     arch = Arch_arm32; break;
  case ELF_MachineKind_386:     arch = Arch_x86;   break;
  case ELF_MachineKind_X86_64:  arch = Arch_x64;   break;
  default: NotImplemented; break;
  }
  return arch;
}
