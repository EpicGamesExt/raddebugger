// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// Build Options

#define BUILD_CONSOLE_INTERFACE 1
#define BUILD_TITLE "TORTURE"

////////////////////////////////

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "coff/coff.h"
#include "coff/coff_enum.h"
#include "coff/coff_parse.h"
#include "coff/coff_obj_writer.h"
#include "pe/pe.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "coff/coff.c"
#include "coff/coff_enum.c"
#include "coff/coff_parse.c"
#include "coff/coff_obj_writer.c"
#include "pe/pe.c"

#include "linker/lnk_cmd_line.h"
#include "linker/lnk_cmd_line.c"

////////////////////////////////

typedef enum
{
  T_Result_Fail,
  T_Result_Crash,
  T_Result_Pass,
} T_Result;

typedef T_Result (*T_Run)(void);

internal char *
t_string_from_result(T_Result v)
{
  switch (v) {
  case T_Result_Fail:  return "FAIL";
  case T_Result_Crash: return "CRASH";
  case T_Result_Pass:  return "PASS";
  }
  return 0;
}

global U64     g_linker_time_out;
global String8 g_linker;
global String8 g_wdir;
global String8 g_out = str8_lit_comp("torture_out");
global B32     g_verbose;

#define T_LINKER_TIME_OUT_EXIT_CODE 999999

typedef enum
{
  T_Linker_Null,
  T_Linker_RAD,
  T_Linker_MSVC,
  T_Linker_LLVM
} T_Linker;

internal T_Linker
t_ident_linker(void)
{
  String8 name = g_linker;
  name = str8_skip_last_slash(name);
  name = str8_chop_last_dot(name);
  if (str8_match(name, str8_lit("radlink"), StringMatchFlag_CaseInsensitive)) {
    return T_Linker_RAD;
  }
  if (str8_match(name, str8_lit("link"), StringMatchFlag_CaseInsensitive)) {
    return T_Linker_MSVC;
  }
  if (str8_match(name, str8_lit("lld-link"), StringMatchFlag_CaseInsensitive)) {
    return T_Linker_LLVM;
  }
  return T_Linker_Null;
}

internal int
t_invoke_linker_with_time_out(U64 time_out, String8 cmdline)
{
  Temp scratch = scratch_begin(0,0);

  //
  // Build Launch Options
  //
  OS_ProcessLaunchParams launch_opts = {0};
  launch_opts.path                   = g_wdir;
  launch_opts.inherit_env            = 1;
  str8_list_push(scratch.arena, &launch_opts.cmd_line, g_linker);
  str8_list_push(scratch.arena, &launch_opts.cmd_line, str8_lit("/nologo"));
  {
    String8List parsed_cmdline = lnk_arg_list_parse_windows_rules(scratch.arena, cmdline);
    str8_list_concat_in_place(&launch_opts.cmd_line, &parsed_cmdline);
  }

  //
  // Invoke Linker
  //
  int exit_code = -1;
  {
    if (g_verbose) {
      String8 full_cmd_line = str8_list_join(scratch.arena, &launch_opts.cmd_line, &(StringJoin){ .sep = str8_lit(" ") });
      fprintf(stdout, "Command Line: %.*s\n", str8_varg(full_cmd_line));
      fprintf(stdout, "Working Dir:  %.*s\n", str8_varg(g_wdir));
    }

    OS_Handle linker_handle = os_process_launch(&launch_opts);
    if (os_handle_match(linker_handle, os_handle_zero())) {
      fprintf(stderr, "unable to start process: %.*s\n", str8_varg(g_linker));
    } else {
      B32 was_joined = os_process_join_exit_code(linker_handle, time_out, &exit_code);
      if (!was_joined) {
        os_process_kill(linker_handle);
        exit_code = T_LINKER_TIME_OUT_EXIT_CODE;
      }
      os_process_detach(linker_handle);
    }
  }

  scratch_end(scratch);
  return exit_code;
}

internal int
t_invoke_linker(String8 cmdline)
{
  return t_invoke_linker_with_time_out(max_U64, cmdline);
}

internal int
t_invoke_linkerf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 cmdline = push_str8fv(scratch.arena, fmt, args);
  int exit_code = t_invoke_linker(cmdline);
  va_end(args);
  scratch_end(scratch);
  return exit_code;
}

internal int
t_invoke_linker_with_time_outf(U64 time_out, char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 cmdline = push_str8fv(scratch.arena, fmt, args);
  int exit_code = t_invoke_linker_with_time_out(time_out, cmdline);
  va_end(args);
  scratch_end(scratch);
  return exit_code;
}

internal String8
t_make_file_path(Arena *arena, String8 name)
{
  return push_str8f(arena, "%S\\%S", g_wdir, name);
}

internal B32
t_write_file(String8 name, String8 data)
{
  Temp scratch = scratch_begin(0,0);
  String8 path = t_make_file_path(scratch.arena, name);
  B32 is_written = os_write_data_to_file_path(path, data);
  scratch_end(scratch);
  return is_written;
}

internal String8
t_read_file(Arena *arena, String8 name)
{
  Temp scratch = scratch_begin(0,0);
  String8 path = t_make_file_path(scratch.arena, name);
  String8 data = os_data_from_file_path(scratch.arena, path);
  scratch_end(scratch);
  return data;
}

typedef struct
{
  T_Run run;
  T_Result result;
} T_RunCtx;

internal void
t_run_caller(void *raw_ctx)
{
  T_RunCtx *ctx = raw_ctx;
  ctx->result = ctx->run();
}

internal void
t_run_fail_handler(void *raw_ctx)
{
  T_RunCtx *ctx = raw_ctx;
  ctx->result = T_Result_Crash;
}

internal T_Result
t_run(T_Run run)
{
  T_RunCtx ctx = {0};
  ctx.run      = run;
  os_safe_call(t_run_caller, t_run_fail_handler, &ctx);
  return ctx.result;
}

internal COFF_SectionHeader *
t_coff_section_header_from_name(String8 string_table, COFF_SectionHeader *section_table, U64 section_count, String8 name)
{
  for (U64 sect_idx = 0; sect_idx < section_count; sect_idx += 1) {
    COFF_SectionHeader *section_header = &section_table[sect_idx];
    String8             section_name   = coff_name_from_section_header(string_table, section_header);
    if (str8_match(section_name, name, 0)) {
      return section_header;
    }
  }
  return 0;
}

internal COFF_SectionHeaderArray
t_coff_section_header_array_from_name(Arena *arena, String8 string_table, COFF_SectionHeader *section_table, U64 section_count, String8 name)
{
  U64 match_count = 0;
  for (U64 sect_idx = 0; sect_idx < section_count; sect_idx += 1) {
    COFF_SectionHeader *section_header = &section_table[sect_idx];
    String8             section_name   = coff_name_from_section_header(string_table, section_header);
    if (str8_match(section_name, name, 0)) {
      match_count += 1;
    }
  }

  COFF_SectionHeader *matches = push_array(arena, COFF_SectionHeader, match_count);
  for (U64 sect_idx = 0, match_idx = 0; sect_idx < section_count; sect_idx += 1) {
    COFF_SectionHeader *section_header = &section_table[sect_idx];
    String8             section_name   = coff_name_from_section_header(string_table, section_header);
    if (str8_match(section_name, name, 0)) {
      matches[match_idx++] = *section_header;
    }
  }

  COFF_SectionHeaderArray result = {0};
  result.count = match_count;
  result.v     = matches;

  return result;
}

////////////////////////////////////////////////////////////////

typedef enum
{
  T_MsvcLinkExitCode_UnresolvedExternals         = 1120,
  T_MsvcLinkExitCode_CorruptOrInvalidSymbolTable = 1235,
} T_MsvcLinkExitCode;

internal COFF_ObjSection *
t_push_text_section(COFF_ObjWriter *obj_writer, String8 data)
{
  return coff_obj_writer_push_section(obj_writer, str8_lit(".text"), COFF_SectionFlag_CntCode|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute|COFF_SectionFlag_Align1Bytes, data);
}

internal COFF_ObjSection *
t_push_data_section(COFF_ObjWriter *obj_writer, String8 data)
{
  return coff_obj_writer_push_section(obj_writer, str8_lit(".data"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite|COFF_SectionFlag_Align1Bytes, data);
}

internal COFF_ObjSection *
t_push_rdata_section(COFF_ObjWriter *obj_writer, String8 data)
{
  return coff_obj_writer_push_section(obj_writer, str8_lit(".rdata"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_Align1Bytes, data);
}

internal T_Result
t_simple_link_test(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  U8 text_payload[] = { 0xC3 };

  String8 main_obj;
  {
    COFF_ObjWriter  *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect  = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), COFF_SectionFlag_CntCode, str8_array_fixed(text_payload));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    main_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 main_obj_name = str8_lit("main.obj");
  if (!t_write_file(main_obj_name, main_obj)) {
    goto exit;
  }

  int file_align = 512;
  int virt_align = 4096;
  String8 out_name = str8_lit("a.exe");
  int linker_exit_code = t_invoke_linkerf("/entry:my_entry /subsystem:console /fixed /filealign:%d /align:%d /out:%S %S", file_align, virt_align, out_name, main_obj_name);
  if (linker_exit_code == 0) {
    String8             exe           = t_read_file(scratch.arena, out_name);
    PE_BinInfo          pe            = pe_bin_info_from_data(scratch.arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);

    if (pe.is_pe32) {
      goto exit;
    }
    if (pe.section_count == 0) {
      goto exit;
    }
    if (pe.arch != Arch_x64) {
      goto exit;
    }
    if (pe.subsystem != PE_WindowsSubsystem_WINDOWS_CUI) {
      goto exit;
    }
    if (pe.virt_section_align != virt_align) {
      goto exit;
    }
    if (pe.file_section_align != file_align) {
      goto exit;
    }
    if (pe.symbol_count != 0) {
      goto exit;
    }

    // check section alignment
    for (U64 sect_idx = 0; sect_idx < pe.section_count; sect_idx += 1) {
      COFF_SectionHeader *sect_header = &section_table[sect_idx];
      if (AlignPadPow2(sect_header->fsize, file_align) != 0) {
        goto exit;
      }
      if (AlignPadPow2(sect_header->voff, virt_align) != 0) {
        goto exit;
      }
    }

    COFF_SectionHeader *text_section = t_coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    if (!text_section) {
      goto exit;
    }
    if (text_section->foff != file_align) { 
      goto exit;
    }

    String8 text_data = str8_substr(exe, rng_1u64(text_section->foff, text_section->foff + text_section->vsize));
    if (!str8_match(text_data, str8_array_fixed(text_payload), 0)) {
      goto exit;
    }

    result = T_Result_Pass;
  }

exit:;
  scratch_end(scratch);
  return result;
}

internal T_Result
t_abs_vs_weak(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  U32 abs_value = 0x123;
  U8 text_code[] = { 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3 };

  String8 abs_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("foo"), abs_value, COFF_SymStorageClass_External);
    abs_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 text_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    COFF_ObjSection *mydata = coff_obj_writer_push_section(obj_writer, str8_lit(".mydata"), COFF_SectionFlag_CntCode|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute|COFF_SectionFlag_Align1Bytes, str8_lit("mydata"));
    COFF_ObjSymbol  *tag    = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("mydata"), 0, mydata);
    COFF_ObjSymbol  *foo    = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("foo"), COFF_WeakExt_NoLibrary, tag);

    COFF_ObjSection *text = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), COFF_SectionFlag_CntCode|COFF_SectionFlag_MemExecute|COFF_SectionFlag_MemRead|COFF_SectionFlag_Align1Bytes, str8_array_fixed(text_code));
    coff_obj_writer_section_push_reloc(obj_writer, text, 2, foo, COFF_Reloc_X64_Addr64);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text);

    text_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  {
    B32 was_file_written = 0;
    was_file_written = t_write_file(str8_lit("abs.obj"),  abs_obj);
    if (!was_file_written) {
      goto exit;
    }
    was_file_written = t_write_file(str8_lit("text.obj"), text_obj);
    if (!was_file_written) {
      goto exit;
    }
  }

  int abs_vs_weak_exit_code = t_invoke_linker(str8_lit("/subsystem:console /entry:my_entry /out:a.exe abs.obj text.obj"));
  if (abs_vs_weak_exit_code != 0) {
    goto exit;
  }

  int weak_vs_abs_exit_code = t_invoke_linker(str8_lit("/subsystem:console /entry:my_entry /out:a.exe text.obj abs.obj"));
  if (weak_vs_abs_exit_code != 0) {
    goto exit;
  }

  String8             exe           = t_read_file(scratch.arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(scratch.arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *text_section = t_coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  if (text_section) {
    String8 text_data = str8_substr(exe, rng_1u64(text_section->foff, text_section->foff + text_section->fsize));
    String8 inst      = str8_prefix(text_data, 2);
    if (str8_match(inst, str8_array(text_code, 2), 0)) {
      String8 imm          = str8_prefix(str8_skip(text_data, 2), 8);
      U64     expected_imm = abs_value;
      if (str8_match(imm, str8_struct(&expected_imm), 0)) {
        result = T_Result_Pass;
      }
    }
  }
  
exit:;
  scratch_end(scratch);
  return result;
}

internal T_Result
t_abs_vs_regular(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  String8 shared_symbol_name = str8_lit("foo");

  U8 regular_payload[] = { 0xC0, 0xFF, 0xEE };
  String8 regular_obj_name = str8_lit("regular.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, str8_array_fixed(regular_payload));
    coff_obj_writer_push_symbol_extern(obj_writer, shared_symbol_name, 0, data_sect);
    String8 regular_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(regular_obj_name, regular_obj)) {
      goto exit;
    }
  }

  String8 abs_obj_name = str8_lit("abs.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_abs(obj_writer, shared_symbol_name, 0x1234, COFF_SymStorageClass_External);
    String8 abs_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(abs_obj_name, abs_obj)) {
      goto exit;
    }
  }

  U8 entry_text[] = { 
    0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, // mov rax, $imm
    0xC3 // ret
  };
  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    COFF_ObjSymbol *shared_symbol = coff_obj_writer_push_symbol_undef(obj_writer, shared_symbol_name);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 3, shared_symbol, COFF_Reloc_X64_Addr32Nb);
    String8 entry_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(entry_obj_name, entry_obj)) {
      goto exit;
    }
  }

  // TODO: validate that linker issues multiply defined symbol error
  int abs_vs_regular_exit_code = t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe abs.obj regular.obj entry.obj");
  if (abs_vs_regular_exit_code == 0) {
    // linker should complain about multiply defined symbol
    goto exit;
  }

  int regular_vs_abs_exit_code = t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe regular.obj abs.obj entry.obj");
  if (regular_vs_abs_exit_code == 0) {
    // linker should complain even in case regular is before abs
    goto exit;
  }

  result = T_Result_Pass;
  
  exit:;
  scratch_end(scratch);
  return result;
}

internal T_Result
t_abs_vs_common(void)
{

  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  String8 shared_symbol_name = str8_lit("foo");

  String8 common_obj_name = str8_lit("common.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_common(obj_writer, shared_symbol_name, 321);
    String8 common_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(common_obj_name, common_obj)) {
      goto exit;
    }
  }

  String8 abs_obj_name = str8_lit("abs.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_abs(obj_writer, shared_symbol_name, 0x1234, COFF_SymStorageClass_External);
    String8 abs_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(abs_obj_name, abs_obj)) {
      goto exit;
    }
  }

  U8 entry_text[] = { 0xC3 };
  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(entry_obj_name, entry_obj)) {
      goto exit;
    }
  }

  int linker_exit_code = t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe abs.obj common.obj entry.obj");
  if (linker_exit_code == 0) {
    // TODO: validate that linker issues multiply defined symbol error
    int common_vs_abs = t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe common.obj abs.obj entry.obj");
    if (common_vs_abs != 0) {
      result = T_Result_Pass;
    }
  }

exit:;
  scratch_end(scratch);
  return result;
}

internal T_Result
t_undef_weak(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  String8 entry_symbol_name = str8_lit("my_entry");
  String8 shared_symbol_name = str8_lit("foo");

  U8 weak_payload[] = { 0xDE, 0xAD, 0xBE, 0xEF };
  String8 weak_obj_name = str8_lit("weak.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *weak_sect = t_push_data_section(obj_writer, str8_array_fixed(weak_payload));
    COFF_ObjSymbol *tag = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("ptr"));
    coff_obj_writer_push_symbol_weak(obj_writer, shared_symbol_name, COFF_WeakExt_SearchAlias, tag);
    String8 weak_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(weak_obj_name, weak_obj)) {
      goto exit;
    }
  }

  String8 ptr_obj_name = str8_lit("ptr.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *tag = coff_obj_writer_push_symbol_undef(obj_writer, entry_symbol_name);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("ptr"), COFF_WeakExt_SearchAlias, tag);
    String8 ptr_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(ptr_obj_name, ptr_obj)) {
      goto exit;
    }
  }

  U8 undef_obj_payload[] = { 0x00, 0x00, 0x00, 0x00 };
  String8 undef_obj_name = str8_lit("undef.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *undef_sect = t_push_data_section(obj_writer, str8_array_fixed(undef_obj_payload));
    COFF_ObjSymbol *undef_symbol = coff_obj_writer_push_symbol_undef(obj_writer, shared_symbol_name);
    coff_obj_writer_section_push_reloc(obj_writer, undef_sect, 0, undef_symbol, COFF_Reloc_X64_Addr32Nb);
    String8 undef_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(undef_obj_name, undef_obj)) {
      goto exit;
    }
  }

  U8 entry_payload[] = {0xC3};
  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_payload));
    coff_obj_writer_push_symbol_extern(obj_writer, entry_symbol_name, 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(entry_obj_name, entry_obj)) {
      goto exit;
    }
  }

  int linker_exit_code = t_invoke_linkerf("/subsystem:console /entry:%S /out:a.exe %S %S %S %S", entry_symbol_name, weak_obj_name, entry_obj_name, ptr_obj_name, undef_obj_name);

  T_Linker link_ident = t_ident_linker();
  if (linker_exit_code != 0) {
    goto exit;
  }

  result = T_Result_Pass;

exit:;
  scratch_end(scratch);
  return result;
}

internal T_Result
t_weak_cycle(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  String8 ab_obj_name = str8_lit("ab.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *b = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("B"));
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("A"), COFF_WeakExt_SearchAlias, b);
    String8 ab_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(ab_obj_name, ab_obj)) {
      goto exit;
    }
  }

  String8 ba_obj_name = str8_lit("ba.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *a = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("A"));
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("B"), COFF_WeakExt_SearchAlias, a);
    String8 ba_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(ba_obj_name, ba_obj)) {
      goto exit;
    }
  }

  String8 entry_obj_name = str8_lit("entry.obj");
  U8 entry_payload[] = { 0xC3 };
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_payload));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(entry_obj_name, entry_obj)) {
      goto exit;
    }
  }

  U64 time_out = os_now_microseconds() + 3 * 1000 * 1000; // give a generous 3 seconds
  int linker_exit_code = t_invoke_linker_with_time_outf(time_out, "/subsystem:console /entry:my_entry %S %S %S", entry_obj_name, ab_obj_name, ba_obj_name);
  if (linker_exit_code != T_LINKER_TIME_OUT_EXIT_CODE) {
    if (t_ident_linker() == T_Linker_MSVC) {
      if (linker_exit_code == T_MsvcLinkExitCode_UnresolvedExternals) {
        result = T_Result_Pass;
      }
    } else {
      result = T_Result_Pass;
    }
  }

exit:;
  scratch_end(scratch);
  return result;
}

internal T_Result
t_weak_tag(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  U32 weak_tag_expected_value = 0x12345678;
  String8 weak_tag_obj_name = str8_lit("weak_tag.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *tag_symbol  = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("abs"), weak_tag_expected_value, COFF_SymStorageClass_Static);
    COFF_ObjSymbol *weak_first  = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("strong_first"), COFF_WeakExt_SearchAlias, tag_symbol);
    COFF_ObjSymbol *weak_second = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("strong_second"), COFF_WeakExt_SearchAlias, weak_first);

    U8 sect_data[] = { 0, 0, 0, 0 };
    COFF_ObjSection *sect = t_push_data_section(obj_writer, str8_array_fixed(sect_data));
    coff_obj_writer_section_push_reloc(obj_writer, sect, 0, weak_second, COFF_Reloc_X64_Addr32);

    String8 weak_tag_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(weak_tag_obj_name, weak_tag_obj)) {
      goto exit;
    }
  }

  String8 entry_name = str8_lit("my_entry");
  U8 entry_text[] = { 0xC3 };
  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter  *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect  = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern(obj_writer, entry_name, 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(entry_obj_name, entry_obj)) {
      goto exit;
    }
  }

  int linker_exit_code = t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe %S %S", weak_tag_obj_name, entry_obj_name);
  if (linker_exit_code == 0) {
    String8             exe           = t_read_file(scratch.arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(scratch.arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);

    COFF_SectionHeader *data_section = t_coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
    if (!data_section) {
      goto exit;
    }
    if (data_section->vsize != 4) {
      goto exit;
    }
    String8 data = str8_substr(exe, rng_1u64(data_section->foff, data_section->foff + data_section->vsize));
    if (str8_match(data, str8_struct(&weak_tag_expected_value), 0)) {
      result = T_Result_Pass;
    }
  }

exit:;
  scratch_end(scratch);
  return result;
}

internal String8
t_make_sec_defn_obj(Arena *arena, String8 payload)
{
  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
  COFF_ObjSection *mysect_section = coff_obj_writer_push_section(obj_writer, str8_lit(".mysect"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_Align1Bytes, payload);
  coff_obj_writer_push_symbol_secdef(obj_writer, mysect_section, COFF_ComdatSelect_Null);
  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);
  return obj;
}

internal T_Result
t_undef_section(void)
{
  Temp scratch = scratch_begin(0,0);
  T_Result result = T_Result_Fail;

  String8 main_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    U8 data[] = { 0, 0, 0, 0 };
    COFF_ObjSection *data_section = coff_obj_writer_push_section(obj_writer, str8_lit(".data"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite, str8_array_fixed(data));
    COFF_ObjSymbol  *foo          = coff_obj_writer_push_symbol_undef_section(obj_writer, str8_lit(".mysect"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead);
    coff_obj_writer_section_push_reloc(obj_writer, data_section, 0, foo, COFF_Reloc_X64_Addr32Nb);

    U8 text[] = { 0xC3 };
    COFF_ObjSection *text_section = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), COFF_SectionFlag_CntCode|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_section);

    main_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);

    coff_obj_writer_release(&obj_writer);
  }

  U8 payload[] = { 1, 2, 3 };
  String8 sec_defn_obj = t_make_sec_defn_obj(scratch.arena, str8_array_fixed(payload));

  t_write_file(str8_lit("main.obj"), main_obj);
  t_write_file(str8_lit("sec_defn.obj"), sec_defn_obj);

  int linker_exit_code = t_invoke_linker(str8_lit("/subsystem:console /entry:my_entry /out:a.exe main.obj sec_defn.obj"));
  if (linker_exit_code == 0) {
    String8             exe           = t_read_file(scratch.arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(scratch.arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);

    COFF_SectionHeader *data_section   = t_coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
    COFF_SectionHeader *mysect_section = t_coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".mysect"));
    if (data_section && mysect_section) {
      if (data_section->vsize == 4 && mysect_section->vsize == 3) {
        String8 addr32nb = str8_substr(exe, rng_1u64(data_section->foff, data_section->foff + data_section->vsize));
        String8 expected_voff = str8_struct(&mysect_section->voff);
        if (str8_match(addr32nb, expected_voff, 0)) {
          result = T_Result_Pass;
        }
      }
    }
  }

  scratch_end(scratch);
  return result;
}

internal T_Result
t_find_merged_pdata(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  U8 foobar_payload[] = {
    0x40, 0x57, 0x48, 0x81, 0xEC, 0x00, 0x02, 0x00, 0x00, 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00,
    0x48, 0x33, 0xC4, 0x48, 0x89, 0x84, 0x24, 0xF0, 0x01, 0x00, 0x00, 0x48, 0x8D, 0x04, 0x24, 0x48,
    0x8B, 0xF8, 0x33, 0xC0, 0xB9, 0xEC, 0x01, 0x00, 0x00, 0xF3, 0xAA, 0xB8, 0x04, 0x00, 0x00, 0x00,
    0x48, 0x6B, 0xC0, 0x02, 0x8B, 0x04, 0x04, 0x48, 0x8B, 0x8C, 0x24, 0xF0, 0x01, 0x00, 0x00, 0x48,
    0x33, 0xCC, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x81, 0xC4, 0x00, 0x02, 0x00, 0x00, 0x5F, 0xC3,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0x48, 0x83, 0xEC, 0x28, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0xC3      
  };
  U8 xdata_payload[] = {
    0x19, 0x1B, 0x03, 0x00, 0x09, 0x01, 0x40, 0x00, 0x02, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0x01, 0x00, 0x00, 0x01, 0x04, 0x01, 0x00, 0x04, 0x42, 0x00, 0x00
  };
  PE_IntelPdata intel_pdata = {0};
  U8 text_payload[]  = { 0xC3 };

  String8 main_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *xdata  = coff_obj_writer_push_section(obj_writer, str8_lit(".xdata"),  COFF_SectionFlag_MemRead|COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_Align4Bytes, str8_array_fixed(xdata_payload));
    COFF_ObjSection *pdata  = coff_obj_writer_push_section(obj_writer, str8_lit(".pdata"),  COFF_SectionFlag_MemRead|COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_Align4Bytes, str8_struct(&intel_pdata));
    COFF_ObjSection *foobar = coff_obj_writer_push_section(obj_writer, str8_lit(".foobar"), COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute|COFF_SectionFlag_CntCode|COFF_SectionFlag_Align1Bytes, str8_array_fixed(foobar_payload));
    COFF_ObjSection *text   = coff_obj_writer_push_section(obj_writer, str8_lit(".text"),   COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute|COFF_SectionFlag_CntCode|COFF_SectionFlag_Align1Bytes, str8_array_fixed(text_payload));

    COFF_ObjSymbol *foobar_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("foobar"), 0, foobar);

    coff_obj_writer_push_symbol_secdef(obj_writer, xdata, COFF_ComdatSelect_Null);
    COFF_ObjSymbol *unwind_foobar = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("$unwind$foobar"), 0, xdata);

    coff_obj_writer_push_symbol_secdef(obj_writer, pdata, COFF_ComdatSelect_Null);
    coff_obj_writer_push_symbol_static(obj_writer, str8_lit("$pdata$foobar"), 0, pdata);

    coff_obj_writer_section_push_reloc(obj_writer, pdata, OffsetOf(PE_IntelPdata, voff_unwind_info),   unwind_foobar, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, pdata, OffsetOf(PE_IntelPdata, voff_first),         foobar_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, pdata, OffsetOf(PE_IntelPdata, voff_one_past_last), foobar_symbol, COFF_Reloc_X64_Addr32Nb);

    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text);
    main_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  t_write_file(str8_lit("main.obj"), main_obj);

  int linker_exit_code = t_invoke_linker(str8_lit("/subsystem:console /entry:my_entry /out:a.exe main.obj /merge:.pdata=.rdata"));
  if (linker_exit_code == 0) {
    String8    exe = t_read_file(scratch.arena, str8_lit("a.exe"));
    PE_BinInfo pe  = pe_bin_info_from_data(scratch.arena, exe);
    if (dim_1u64(pe.data_dir_franges[PE_DataDirectoryIndex_EXCEPTIONS]) == 0xC) {
      result = T_Result_Pass;
    }
  }

  scratch_end(scratch);
  return result;
}

internal T_Result
t_section_sort(void)
{
  Temp scratch = scratch_begin(0,0);
  
  T_Result result = T_Result_Fail;

  String8 data_obj_name = str8_lit("data.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    COFF_SectionFlags data_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemRead|COFF_SectionFlag_Align1Bytes;
    coff_obj_writer_push_section(obj_writer, str8_lit(".data$z"), data_flags, str8_lit("five"));
    coff_obj_writer_push_section(obj_writer, str8_lit(".data$a"), data_flags, str8_lit("three"));
    coff_obj_writer_push_section(obj_writer, str8_lit(".data$bbbbb"), data_flags, str8_lit("four"));
    coff_obj_writer_push_section(obj_writer, str8_lit(".data$"), data_flags, str8_lit("two"));
    coff_obj_writer_push_section(obj_writer, str8_lit(".data"), data_flags, str8_lit("one"));

    String8 data_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(data_obj_name, data_obj)) {
      goto exit;
    }
  }

  String8 entry_obj_name = str8_lit("entry.obj");
  U8 entry_text[] = { 0xC3 };
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(entry_obj_name, entry_obj)) {
      goto exit;
    }
  }

  int linker_exit_code = t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe data.obj entry.obj");
  if (linker_exit_code != 0) {
    goto exit;
  }

  String8             exe           = t_read_file(scratch.arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(scratch.arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *data_section = t_coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
  if (!data_section) {
    goto exit;
  }

  String8 data = str8_substr(exe, rng_1u64(data_section->foff, data_section->foff + data_section->vsize));
  String8 expected_data = str8_lit("onetwothreefourfive");
  if (!str8_match(data, expected_data, 0)) {
    goto exit;
  }

  result = T_Result_Pass;

exit:;
  scratch_end(scratch);
  return result;
}

internal T_Result
t_flag_conf(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  COFF_SectionFlags my_sect0_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute;
  COFF_SectionFlags my_sect1_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite;
  String8 conf_obj_name = str8_lit("conf.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *a_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".mysect"), my_sect0_flags, str8_lit("one"));
    COFF_ObjSection *b_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".mysect"), my_sect1_flags, str8_lit("two"));
    String8 conf_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(conf_obj_name, conf_obj)) {
      goto exit;
    }
  }

  U8 entry_text[] = { 0xC3 };
  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(entry_obj_name, entry_obj)) {
      goto exit;
    }
  }

  int linker_exit_code = t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe data.obj conf.obj entry.obj");
  if (linker_exit_code != 0) {
    goto exit;
  }

  String8             exe           = t_read_file(scratch.arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(scratch.arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeaderArray my_sects = t_coff_section_header_array_from_name(scratch.arena, string_table, section_table, pe.section_count, str8_lit(".mysect"));

  if (my_sects.count != 2) {
    goto exit;
  }

  COFF_SectionHeader *my_sect0 = &my_sects.v[0];
  COFF_SectionHeader *my_sect1 = &my_sects.v[1];
  if (my_sect0->flags != my_sect0_flags) {
    goto exit;
  }
  if (my_sect1->flags != my_sect1_flags) {
    goto exit;
  }

  result = T_Result_Pass;

exit:;
  scratch_end(scratch);
  return result;
}

internal T_Result
t_invalid_bss(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  String8 bss_obj_name = str8_lit("bss.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_section(obj_writer, str8_lit(".bss"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead, str8_lit("Hello, World"));
    String8 bss_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(bss_obj_name, bss_obj)) {
      goto exit;
    }
  }

  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = { 0xC3 };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(entry_obj_name, entry_obj)) {
      goto exit;
    }
  }

  int linker_exit_code = t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe bss.obj entry.obj");

exit:;
  scratch_end(scratch);
  return result;
}

internal T_Result
t_common_block(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  String8 a_obj_name = str8_lit("a.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_common(obj_writer, str8_lit("A"), 3);
    U8 data[6] = { 0 };
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, str8_array_fixed(data));
    coff_obj_writer_section_push_reloc(obj_writer, data_sect, 0, symbol, COFF_Reloc_X64_Addr32);
    String8 a_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(a_obj_name, a_obj)) {
      goto exit;
    }
  }

  String8 b_obj_name = str8_lit("b.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 data[9] = { 0 };
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, str8_array_fixed(data));
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_common(obj_writer, str8_lit("B"), 6);
    coff_obj_writer_section_push_reloc(obj_writer, data_sect, 0, symbol, COFF_Reloc_X64_Addr64);
    String8 b_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(b_obj_name, b_obj)) {
      goto exit;
    }
  }

  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = { 0xC3 };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(entry_obj_name, entry_obj)) {
      goto exit;
    }
  }

  int linker_exit_code = t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe /fixed /largeaddressaware:no a.obj b.obj entry.obj");
  
  if (linker_exit_code != 0) {
    goto exit;
  }

  String8             exe           = t_read_file(scratch.arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(scratch.arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

exit:;
  scratch_end(scratch);
  return result;
}

internal T_Result
t_base_relocs(void)
{
  Temp scratch = scratch_begin(0,0);

  T_Result result = T_Result_Fail;

  // main.obj
  String8 entry_name = str8_lit("my_entry");
  U64 mov_func_name64 = 2;
  U64 mov_func_name32 = 16;
  U8 main_text[] = {
    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov rax, func_name
    0xff, 0xd0,                                                  // call rax
    0x48, 0x31, 0xc0,                                            // xor rax, rax
    0xb8, 0x00, 0x00, 0x00, 0x00,                                // mov eax, func_name
    0xff, 0xd0,                                                  // call rax
    0xc3                                                         // ret
  };

  // func.obj
  String8 func_name   = str8_lit("foo");
  U8      func_text[] = { 0xc3 };

  String8 main_obj_name = str8_lit("main.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect  = t_push_text_section(obj_writer, str8_array_fixed(main_text));
    COFF_ObjSymbol  *func_undef = coff_obj_writer_push_symbol_undef(obj_writer, func_name);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, mov_func_name64, func_undef, COFF_Reloc_X64_Addr64);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, mov_func_name32, func_undef, COFF_Reloc_X64_Addr32);

    // linker must not produce base relocations for absolute symbol
    U8 data[4] = {0};
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, str8_array_fixed(data));
    COFF_ObjSymbol *abs_symbol = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("abs"), 0x12345678, COFF_SymStorageClass_Static);
    coff_obj_writer_section_push_reloc(obj_writer, data_sect, 0, abs_symbol, COFF_Reloc_X64_Addr32);

    coff_obj_writer_push_symbol_extern(obj_writer, entry_name, 0, text_sect);
    String8 main_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(main_obj_name, main_obj)) {
      goto exit;
    }
  }

  String8 func_obj_name = str8_lit("func.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(func_text));
    coff_obj_writer_push_symbol_extern(obj_writer, func_name, 0, text_sect);
    String8 func_obj = coff_obj_writer_serialize(scratch.arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    if (!t_write_file(func_obj_name, func_obj)) {
      goto exit;
    }
  }

  String8 out_name = str8_lit("a.exe");
  int linker_exit_code = t_invoke_linkerf("/subsystem:console /entry:%S /dynamicbase /largeaddressaware:no /out:%S %S %S", entry_name, out_name, main_obj_name, func_obj_name);
  
exit:;
  scratch_end(scratch);
  return result;
}

////////////////////////////////////////////////////////////////

internal void
entry_point(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0,0);

  //
  // Targets
  //
  static struct {
    char *label;
    T_Result (*r)(void);
  } target_array[] = {
    { "simple_link_test",   t_simple_link_test  },
    { "undef_section",      t_undef_section     },
    { "abs_vs_weak",        t_abs_vs_weak       },
    { "abs_vs_regular",     t_abs_vs_regular    },
    { "abs_vs_common",      t_abs_vs_common     },
    { "undef_weak",         t_undef_weak        },
    { "weak_cycle",         t_weak_cycle        },
    { "weak_tag",           t_weak_tag          },
    { "find_merged_pdata",  t_find_merged_pdata },
    { "section_sort",       t_section_sort      },
    { "flag_conf",          t_flag_conf         },
    { "invalid_bss",        t_invalid_bss       },
    { "common_block",       t_common_block      },
    //{ "base_relocs",        t_base_relocs       },
  };

  //
  // Handle -help
  //
  {
    B32 print_help = cmd_line_has_flag(cmdline, str8_lit("help")) ||
                     cmd_line_has_flag(cmdline, str8_lit("h")) ||
                     cmdline->argc == 1;
    if (print_help) {
      fprintf(stderr, "--- Help -----------------------------------------------------------------------\n");
      fprintf(stderr, " %s\n\n", BUILD_TITLE_STRING_LITERAL);
      fprintf(stderr, " Usage: torture [Options] [Files]\n\n");
      fprintf(stderr, " Options:\n");
      fprintf(stderr, "   -linker:{path}        Path to PE/COFF linker\n");
      fprintf(stderr, "   -target:{name[,name]} Selects targets to test\n");
      fprintf(stderr, "   -list                 Print available test targets and exit\n");
      fprintf(stderr, "   -out:{path}           Directory path for test outputs (default \"%.*s\")\n", str8_varg(g_out));
      fprintf(stderr, "   -verbose              Enable verbose mode\n");
      fprintf(stderr, "   -help                 Print help menu and exit\n");
      os_abort(0);
    }
  }

  //
  // Handle -list
  //
  {
    if (cmd_line_has_flag(cmdline, str8_lit("list"))) {
      fprintf(stdout, "--- Targets --------------------------------------------------------------------\n");
      for (U64 i = 0; i < ArrayCount(target_array); i += 1) {
        fprintf(stdout, "  %s\n", target_array[i].label);
      }
      os_abort(0);
    }
  }


  //
  // Handle -linker
  //
  {
    CmdLineOpt *linker_opt = cmd_line_opt_from_string(cmdline, str8_lit("linker"));
    if (linker_opt == 0) {
      linker_opt = cmd_line_opt_from_string(cmdline, str8_lit("l"));
    }
    if (linker_opt) {
      if (linker_opt->value_strings.node_count == 1) {
        g_linker = linker_opt->value_string;
      } else {
        fprintf(stderr, "ERROR: -linker has invalid number of arguments\n");
        os_abort(1);
      }
    } else {
      fprintf(stderr, "ERROR: missing -linker option\n");
      os_abort(1);
    }
  }

  //
  // Handle optional -target
  //
  String8List target = cmdline->inputs;
  {
    CmdLineOpt *target_opt = cmd_line_opt_from_string(cmdline, str8_lit("target"));
    if (target_opt == 0) {
      target_opt = cmd_line_opt_from_string(cmdline, str8_lit("t"));
    }
    if (target_opt) {
      if (target_opt->value_strings.node_count > 0) {
        str8_list_concat_in_place(&target, &target_opt->value_strings);
      } else {
        fprintf(stderr, "ERROR: -target has invalid number of arguments\n");
      }
    }
  }

  //
  // Handle -out
  //
  {
    CmdLineOpt *out_opt = cmd_line_opt_from_string(cmdline, str8_lit("out"));
    if (out_opt) {
      if (out_opt->value_strings.node_count == 1) {
        g_out = out_opt->value_string;
      } else {
        fprintf(stderr, "ERROR: -out invalid number of arguments");
      }
    }
  }

  //
  // Handle -verbose
  //
  {
    g_verbose = cmd_line_has_flag(cmdline, str8_lit("verbose"));
  }

  //
  // Make Output Directory
  //
  os_make_directory(g_out);
  if (!os_folder_path_exists(g_out)) {
    fprintf(stderr, "ERROR: unable to create output directory \"%.*s\"\n", str8_varg(g_out));
    os_abort(1);
  }

  //
  // Run Test Targets
  //
  {
    U64 max_label_size = 0;
    for (U64 i = 0; i < ArrayCount(target_array); i += 1) { max_label_size = Max(max_label_size, cstring8_length(target_array[i].label)); }

    U64 dots_min = 10;
    U64 dots_size = max_label_size+dots_min;
    U8 *dots      = push_array(scratch.arena, U8, dots_size);
    MemorySet(dots, '.', dots_size);

    U64  target_indices_count;
    U64 *target_indices;
    if (target.node_count == 0) {
      target_indices_count = ArrayCount(target_array);
      target_indices       = push_array(scratch.arena, U64, ArrayCount(target_array));
      for (U64 i = 0; i < target_indices_count; i += 1) { target_indices[i] = i; }
    } else {
      target_indices_count = 0;
      target_indices       = push_array(scratch.arena, U64, target.node_count);

      for (String8Node *target_n = target.first; target_n != 0; target_n = target_n->next) {
        B32 is_target_unknown = 1;
        for (U64 i = 0; i < ArrayCount(target_array); i += 1) {
          if (str8_match(str8_cstring(target_array[i].label), target_n->string, 0)) {
            target_indices[target_indices_count++] = i;
            is_target_unknown = 0;
            break;
          }
        }
        if (is_target_unknown) {
          fprintf(stderr, "ERROR: unknown target \"%.*s\"\n", str8_varg(target_n->string));
        }
      }
    }

    for (U64 i = 0; i < target_indices_count; i += 1) {
      U64 target_idx = target_indices[i];

      g_wdir = push_str8f(scratch.arena, "%S\\%s", g_out, target_array[target_idx].label);
      g_wdir = os_full_path_from_path(scratch.arena, g_wdir);
      os_make_directory(g_wdir);
      if (!os_folder_path_exists(g_out)) {
        fprintf(stderr, "ERROR: unable to create output directory for test run %.*s\n", str8_varg(g_wdir));
        continue;
      }

      T_Result result = t_run(target_array[target_idx].r);

      U64 dots_count = (max_label_size - cstring8_length(target_array[target_idx].label)) + dots_min;
      String8 msg = push_str8f(scratch.arena, "%s%.*s%s", target_array[target_idx].label, dots_count, dots, t_string_from_result(result));

      if (result == T_Result_Pass) {
        fprintf(stdout, "\x1b[32m" "%.*s" "\x1b[0m" "\n", str8_varg(msg));
      } else if (result == T_Result_Fail) {
        fprintf(stdout, "\x1b[31m" "%.*s" "\x1b[0m" "\n", str8_varg(msg));
      } else if (result == T_Result_Crash) {
        fprintf(stdout, "\x1b[33m" "%.*s" "\x1b[0m" "\n", str8_varg(msg));
      }
    }
  }

  scratch_end(scratch);
}

