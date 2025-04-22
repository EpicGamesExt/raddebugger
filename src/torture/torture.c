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

global String8 g_linker;
global String8 g_wdir;
global String8 g_out = str8_lit_comp("torture_out");
global B32     g_verbose;

internal int
t_invoke_linker(String8 cmdline)
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
      B32 was_joined = os_process_join_exit_code(linker_handle, max_U64, &exit_code);
      Assert(was_joined);
      os_process_detach(linker_handle);
    }
  }

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

////////////////////////////////////////////////////////////////

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

  int linker_exit_code = t_invoke_linker(str8_lit("/subsystem:console /entry:my_entry /out:a.exe abs.obj text.obj"));
  if (linker_exit_code == 0) {
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
    { "undef_section",      t_undef_section     },
    { "abs_vs_weak",        t_abs_vs_weak       },
    { "find_merged_pdata",  t_find_merged_pdata },
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

