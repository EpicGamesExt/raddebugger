// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define BUILD_TITLE "STAP Parser Test"
#define BUILD_CONSOLE_INTERFACE 1

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "regs/regs.h"
#include "stap/stap_parse.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "regs/regs.c"
#include "stap/stap_parse.c"

internal void
entry_point(CmdLine *cmd_line)
{
  B32      passed = 0;
  STAP_Arg arg    = {0};
  String8  error  = {0};
  String8  string = {0};

  string = str8_lit("-2@$-123");
  error = stap_parse_args_x64(string, &arg);
  if (error.size != 0) { goto exit; }
  AssertAlways(arg.value_type == STAP_ArgValueType_S);
  AssertAlways(arg.value_size == 2);
  AssertAlways(arg.type == STAP_ArgType_Imm);
  AssertAlways((S64)arg.imm == -123);

  string = str8_lit("1@$43");
  error = stap_parse_args_x64(string, &arg);
  if (error.size != 0) { goto exit; }
  AssertAlways(arg.value_type == STAP_ArgValueType_U);
  AssertAlways(arg.value_size == 1);
  AssertAlways(arg.type == STAP_ArgType_Imm);
  AssertAlways((S64)arg.imm == 43);

  string = str8_lit("4f@%eax");
  error = stap_parse_args_x64(string, &arg);
  if (error.size != 0) { goto exit; }
  AssertAlways(arg.value_type == STAP_ArgValueType_F);
  AssertAlways(arg.value_size == 4);
  AssertAlways(arg.type == STAP_ArgType_Reg);
  AssertAlways(arg.reg.is_alias == 1);
  AssertAlways(arg.reg.reg_code == REGS_AliasCodeX64_eax);

  string = str8_lit("4@(%rdi)");
  error = stap_parse_args_x64(string, &arg);
  if (error.size != 0) { goto exit; }
  AssertAlways(arg.value_size == 4);
  AssertAlways(arg.value_type == STAP_ArgValueType_U);
  AssertAlways(arg.type == STAP_ArgType_MemoryRef);
  AssertAlways(arg.memory_ref.disp == 0);
  AssertAlways(arg.memory_ref.base.reg_code == REGS_RegCodeX64_rdi);
  AssertAlways(!arg.memory_ref.base.is_alias);
  AssertAlways(arg.memory_ref.index.reg_code == 0);
  AssertAlways(arg.memory_ref.index.is_alias == 0);
  AssertAlways(arg.memory_ref.scale == 1);

  string = str8_lit("4@-22(%rdi)");
  error = stap_parse_args_x64(string, &arg);
  if (error.size != 0) { goto exit; }
  AssertAlways(arg.value_size == 4);
  AssertAlways(arg.value_type == STAP_ArgValueType_U);
  AssertAlways(arg.type == STAP_ArgType_MemoryRef);
  AssertAlways(arg.memory_ref.disp == -22);
  AssertAlways(arg.memory_ref.base.reg_code == REGS_RegCodeX64_rdi);
  AssertAlways(!arg.memory_ref.base.is_alias);
  AssertAlways(arg.memory_ref.index.reg_code == 0);
  AssertAlways(arg.memory_ref.index.is_alias == 0);
  AssertAlways(arg.memory_ref.scale == 1);

  string = str8_lit("4@32(%rax,%rsi,8)");
  error = stap_parse_args_x64(string, &arg);
  if (error.size != 0) { goto exit; }
  AssertAlways(arg.value_size == 4);
  AssertAlways(arg.value_type == STAP_ArgValueType_U);
  AssertAlways(arg.memory_ref.disp == 32);
  AssertAlways(arg.memory_ref.base.reg_code == REGS_RegCodeX64_rax);
  AssertAlways(arg.memory_ref.base.is_alias == 0);
  AssertAlways(arg.memory_ref.index.reg_code == REGS_RegCodeX64_rsi);
  AssertAlways(arg.memory_ref.index.is_alias == 0);
  AssertAlways(arg.memory_ref.scale == 8);

  string = str8_lit("4@32(,%rsi,8)");
  error = stap_parse_args_x64(string, &arg);
  if (error.size != 0) { goto exit; }
  AssertAlways(arg.value_size == 4);
  AssertAlways(arg.value_type == STAP_ArgValueType_U);
  AssertAlways(arg.memory_ref.disp == 32);
  AssertAlways(arg.memory_ref.base.reg_code == 0);
  AssertAlways(arg.memory_ref.index.reg_code == REGS_RegCodeX64_rsi);
  AssertAlways(arg.memory_ref.scale == 8);

  error = stap_parse_args_x64(str8_lit("4@(,,)"), &arg);
  if (error.size == 0) { goto exit; }

  error = stap_parse_args_x64(str8_lit("4@()"), &arg);
  if (error.size == 0) { goto exit; }

  error = stap_parse_args_x64(str8_lit("4@(%rdi, %rsi, 8"), &arg);
  if (error.size == 0) { goto exit; }

  error = stap_parse_args_x64(str8_lit("4@( ,, 8"), &arg);
  if (error.size == 0) { goto exit; }

  passed = 1;
exit:;
  AssertAlways(passed);
}

