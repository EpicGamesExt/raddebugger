internal String8
rc_data_from_file_path(Arena *arena, String8 path)
{
  String8 data = os_data_from_file_path(arena, path);
  if (data.size == 0) {
    fprintf(stderr, "error: unable to read file %.*s\n", str8_varg(path));
    os_abort(1);
  }
  return data;
}

internal RC_Context
rc_context_from_cmd_line(Arena *arena, CmdLine *cmdl)
{
  Temp scratch = scratch_begin(&arena, 1);

  if (cmdl->inputs.node_count > 2) {
    fprintf(stderr, "error: too many input files on the command line.\n");
    os_abort(1);
  }

  B32 is_pe_present        = 0;
  B32 is_pdb_present       = 0;
  B32 is_elf_present       = 0;
  B32 is_elf_debug_present = 0;
  String8 pe_name        = {0};
  String8 pe_data        = {0};
  String8 pdb_name       = {0};
  String8 pdb_data       = {0};
  String8 elf_name       = {0};
  String8 elf_data       = {0};
  String8 elf_debug_name = {0};
  String8 elf_debug_data = {0};

  //
  // Set typed inputs
  //
  if (cmd_line_has_flag(cmdl, str8_lit("pe"))) {
    pe_name       = cmd_line_string(cmdl, str8_lit("pe"));
    pe_data       = rc_data_from_file_path(arena, pe_name);
    if (!pe_check_magic(pe_data)) {
      fprintf(stderr, "error: -pe:%.*s is not of PE format\n", str8_varg(pe_name));
      os_abort(1);
    }
    is_pe_present = 1;
  }
  if (cmd_line_has_flag(cmdl, str8_lit("pdb"))) {
    pdb_name       = cmd_line_string(cmdl, str8_lit("pdb"));
    pdb_data       = rc_data_from_file_path(arena, pdb_name);
    if (!msf_check_magic_20(pdb_data) && !msf_check_magic_70(pdb_data)) {
      fprintf(stderr, "error: -pdb:%.*s is not of PDB format\n", str8_varg(pdb_name));
      os_abort(1);
    }
    is_pdb_present = 1;
  }
  if (cmd_line_has_flag(cmdl, str8_lit("elf"))) {
    elf_name       = cmd_line_string(cmdl, str8_lit("elf"));
    elf_data       = rc_data_from_file_path(arena, elf_name);
    if (!elf_check_magic(elf_data)) {
      fprintf(stderr, "error: -elf:%.*s is not of ELF format\n", str8_varg(elf_name));
      os_abort(1);
    }
    is_elf_present = 1;
  }
  if (cmd_line_has_flag(cmdl, str8_lit("elf_debug"))) {
    elf_debug_name       = cmd_line_string(cmdl, str8_lit("elf_debug"));
    elf_debug_data       = rc_data_from_file_path(arena, elf_debug_name);
    if (!elf_check_magic(elf_debug_data)) {
      fprintf(stderr, "error: -elf_debug:%.*s is not of ELF format\n", str8_varg(elf_debug_name));
      os_abort(1);
    }
    is_elf_debug_present = 1;
  }

  //
  // Pick conversion driver
  //
  RC_Driver driver = RC_Driver_Null;
  if (cmd_line_has_flag(cmdl, str8_lit("driver"))) {
    String8 driver_name = cmd_line_string(cmdl, str8_lit("driver"));
    if (str8_match(driver_name, str8_lit("dwarf"), StringMatchFlag_CaseInsensitive)) {
      driver = RC_Driver_Dwarf;
    } else if (str8_match(driver_name, str8_lit("pdb"), StringMatchFlag_CaseInsensitive)) {
      driver = RC_Driver_Pdb;
    } else {
      fprintf(stderr, "error: unknown driver \"%.*s\"\n", str8_varg(driver_name));
      os_abort(1);
    }
  }

  //
  // Load inputs
  //
  for (String8Node *input_n = cmdl->inputs.first; input_n != 0; input_n = input_n->next) {
    String8 input_data = os_data_from_file_path(arena, input_n->string);

    if (input_data.size == 0) {
      fprintf(stderr, "unable to read input %.*s\n", str8_varg(input_n->string));
      os_abort(1);
    }

    if (pe_check_magic(input_data)) {
      if (is_pe_present) {
        fprintf(stderr, "error: too many PE files are specified on the command line\n");
        fprintf(stderr, "       selected: %.*s\n", str8_varg(pe_name));
        fprintf(stderr, "       current:  %.*s\n", str8_varg(input_n->string));
        os_abort(1);
      }
      pe_data       = input_data;
      pe_name       = input_n->string;
      is_pe_present = 1;
    } else if (elf_check_magic(input_data)) {
      ELF_BinInfo elf              = elf_bin_from_data(input_data);
      B32         is_dwarf_present = dw_is_dwarf_present_elf_section_table(input_data, &elf);
      if (is_dwarf_present) {
        if (is_elf_debug_present) {
          fprintf(stderr, "error: ambiguous input, both ELFs have DWARF debug sections, please use --elf:<path> --elf_debug:<path> to clarify inputs.\n");
          os_abort(1);
        }
        elf_debug_name       = input_n->string;
        elf_debug_data       = input_data;
        is_elf_debug_present = 1;
      } else {
        elf_name       = input_n->string;
        elf_data       = input_data;
        is_elf_present = 1;
      }
    } else if (msf_check_magic_20(input_data) || msf_check_magic_70(input_data)) {
      if (is_pdb_present) {
        fprintf(stderr, "error: too many PDB files are specified on the command line\n");
        fprintf(stderr, "       selected: %.*s\n", str8_varg(pdb_name));
        fprintf(stderr, "       current:  %.*s\n", str8_varg(input_n->string));
        continue;
      }
      pdb_name       = input_n->string;
      pdb_data       = input_data;
      is_pdb_present = 1;
    } else {
      fprintf(stderr, "error: unknown file format %.*s\n", str8_varg(input_n->string));
    }
  }

  //
  // Validate input combos
  //
  if ((is_pe_present || is_pdb_present) && (is_elf_present || is_elf_debug_present)) {
    fprintf(stderr, "error: invalid combination of inputs provided, we convert only (PE|PDB) or (ELF|ELF_DEBUG) at a time.\n");
    if (is_pe_present) {
      fprintf(stderr, "       PE: %.*s\n", str8_varg(pe_name));
    }
    if (is_pdb_present) {
      fprintf(stderr, "       PDB: %.*s\n", str8_varg(pdb_name));
    }
    if (is_elf_present) {
      fprintf(stderr, "       ELF: %.*s\n", str8_varg(elf_name));
    }
    if (is_elf_debug_present) {
      fprintf(stderr, "       ELF Debug: %.*s\n", str8_varg(elf_debug_name));
    }
    os_abort(1);
  }

  if (is_pe_present && (is_elf_present || is_elf_debug_present)) {
    fprintf(stderr, "error: command line has too many image types specified.\n");
    os_abort(1);
  }


  ImageType image      = Image_Null;
  String8   image_name = {0};
  String8   image_data = {0};
  String8   debug_name = {0};
  String8   debug_data = {0};

  B32  check_guid  = 0;
  Guid pe_pdb_guid = {0};

  B32              elf_has_debug_link = 0;
  ELF_GnuDebugLink debug_link         = {0};

  //
  // Input has PE/COFF
  //
  if (is_pe_present) {
    image      = Image_CoffPe;
    image_name = pe_name;
    image_data = pe_data;

    PE_BinInfo       pe            = pe_bin_info_from_data(scratch.arena, pe_data);
    String8          raw_debug_dir = str8_substr(pe_data, pe.data_dir_franges[PE_DataDirectoryIndex_DEBUG]);
    PE_DebugInfoList debug_dir     = pe_parse_debug_directory(scratch.arena, pe_data, raw_debug_dir);
    for (PE_DebugInfoNode *debug_n = debug_dir.first; debug_n != 0; debug_n = debug_n->next) {
      PE_DebugInfo *debug = &debug_n->v;
      if (debug->header.type == PE_DebugDirectoryType_CODEVIEW) {
        if (debug->u.codeview.magic == PE_CODEVIEW_PDB70_MAGIC) {
          check_guid  = 1;
          pe_pdb_guid = debug->u.codeview.pdb70.header.guid;

          if (!is_pdb_present) {
            pdb_name       = debug->u.codeview.pdb70.path;
            pdb_data       = rc_data_from_file_path(arena, pdb_name);
            is_pdb_present = 1;
          }

          break;
        }
      }
    }

    if (driver == RC_Driver_Null || driver == RC_Driver_Dwarf) {
      PE_BinInfo          pe                = pe_bin_info_from_data(scratch.arena, pe_data);
      String8             raw_section_table = str8_substr(pe_data, pe.section_table_range);
      String8             string_table      = str8_substr(pe_data, pe.string_table_range);
      U64                 section_count     = raw_section_table.size / sizeof(COFF_SectionHeader);
      COFF_SectionHeader *section_table     = (COFF_SectionHeader *)raw_section_table.str;
      if (dw_is_dwarf_present_coff_section_table(pe_data, string_table, section_count, section_table)) {
        driver     = RC_Driver_Dwarf;
        debug_name = pe_name;
        debug_data = pe_data;
        goto driver_found;
      } else if (driver == RC_Driver_Dwarf) {
        fprintf(stderr, "error: image doesn't have DWARF debug sections.\n");
        os_abort(1);
      }
    }
  }

  if (is_elf_present || is_elf_debug_present) {
    if (driver != RC_Driver_Null && driver != RC_Driver_Dwarf) {
      fprintf(stderr, "error: ELF inputs are only supported when using DWARF driver.\n");
      os_abort(1);
    }

    //
    // Load image ELF
    //
    ELF_BinInfo elf           = elf_bin_from_data(elf_data);
    B32         has_elf_dwarf = dw_is_dwarf_present_elf_section_table(elf_data, &elf);

    // 
    // ELF doesn't have debug info and no .debug was specified on command line,
    // try to load .debug via debug link
    //
    if (is_elf_present && !is_elf_debug_present) {
      elf_has_debug_link = elf_parse_debug_link(elf_data, &elf, &debug_link);
    }
    if (elf_has_debug_link) {
      elf_debug_data       = rc_data_from_file_path(arena, debug_link.path);
      is_elf_debug_present = 1;
    }

    //
    // Load .debug ELF
    //
    ELF_BinInfo elf_debug           = elf_bin_from_data(elf_debug_data);
    B32         has_elf_debug_dwarf = dw_is_dwarf_present_elf_section_table(elf_debug_data, &elf_debug);

    //
    // Input is image ELF and .debug ELF
    //
    B32 is_split_elf = is_elf_present && is_elf_debug_present && !has_elf_dwarf && has_elf_debug_dwarf;
    if (is_split_elf) {
      driver     = RC_Driver_Dwarf;
      image      = ELF_HdrIs64Bit(elf.hdr.e_ident) ? Image_Elf64 : Image_Elf32;
      image_name = elf_name;
      image_data = elf_data;
      debug_name = elf_debug_name;
      debug_data = elf_debug_data;
      goto driver_found;
    }

    //
    // Input ELF is image with debug info
    //
    B32 is_monolithic_elf = is_elf_present && !is_elf_debug_present && has_elf_dwarf;
    if (is_monolithic_elf) {
      driver     = RC_Driver_Dwarf;
      image      = ELF_HdrIs64Bit(elf.hdr.e_ident) ? Image_Elf64 : Image_Elf32;
      image_name = elf_name;
      image_data = elf_data;
      debug_name = elf_name;
      debug_data = elf_data;
      goto driver_found;
    }

    //
    // Input ELF is .debug
    //
    B32 is_debug_elf = !is_elf_present && is_elf_debug_present && has_elf_debug_dwarf;
    if (is_debug_elf) {
      driver     = RC_Driver_Dwarf;
      image      = ELF_HdrIs64Bit(elf_debug.hdr.e_ident) ? Image_Elf64 : Image_Elf32;
      debug_name = elf_debug_name;
      debug_data = elf_debug_data;
      goto driver_found;
    }
  }

  //
  // Input is PDB
  //
  if (is_pdb_present) {
    if (driver == RC_Driver_Null || driver == RC_Driver_Pdb) {
      driver     = RC_Driver_Pdb;
      debug_name = pdb_name;
      debug_data = pdb_data;
      goto driver_found;
    } else if (driver == RC_Driver_Dwarf) {
      fprintf(stderr, "error: unable to select DWARF conversion driver because convert doesn't support PDB as input format.\n");
      os_abort(1);
    } else {
      InvalidPath;
    }
  }

  driver_found:;

  //
  // Handle -out param
  //
  String8 out_name = {0};
  if (cmd_line_has_flag(cmdl, str8_lit("out"))) {
    out_name = cmd_line_string(cmdl, str8_lit("out"));
    if (out_name.size == 0) {
      fprintf(stderr, "error: -out parameter doesn't have a value\n");
      os_abort(1);
    }
  } else {
    if (image_name.size) {
      out_name = path_replace_file_extension(arena, image_name, str8_lit("rdi"));
    } else {
      out_name = path_replace_file_extension(arena, debug_name, str8_lit("rdi"));
    }
  }


  //
  // Validate driver input
  //
  if (driver == RC_Driver_Pdb &&
      !is_pdb_present && (is_elf_present || is_elf_debug_present)) {
    fprintf(stderr, "error: DWARF is an invalid input for PDB driver\n");
    os_abort(1);
  }


  RC_Context ctx = {0};
  ctx.driver     = driver;
  ctx.image      = image;
  ctx.image_name = image_name;
  ctx.image_data = image_data;
  ctx.debug_name = debug_name;
  ctx.debug_data = debug_data;
  ctx.flags      = RC_Flag_Strings|
                   RC_Flag_IndexRuns|
                   RC_Flag_BinarySections|
                   RC_Flag_Units|
                   RC_Flag_Procedures|
                   RC_Flag_GlobalVariables|
                   RC_Flag_ThreadVariables|
                   RC_Flag_Scopes|
                   RC_Flag_Locals|
                   RC_Flag_Types|
                   RC_Flag_UDTs|
                   RC_Flag_LineInfo|
                   RC_Flag_GlobalVariableNameMap|
                   RC_Flag_ThreadVariableNameMap|
                   RC_Flag_ProcedureNameMap|
                   RC_Flag_TypeNameMap|
                   RC_Flag_LinkNameProcedureNameMap|
                   RC_Flag_NormalSourcePathNameMap;
  if (check_guid) {
    ctx.flags |= RC_Flag_CheckPdbGuid;
    ctx.guid   = pe_pdb_guid;
  }
  if (elf_has_debug_link) {
    ctx.flags     |= RC_Flag_CheckElfChecksum;
    ctx.debug_link = debug_link; 
  }
  ctx.out_name = out_name;

  scratch_end(scratch);
  return ctx;
}

internal String8List
rc_run(Arena *arena, RC_Context *rc)
{
  Temp scratch = scratch_begin(&arena, 1);

  ProfBegin("Convert");
  RDIM_LocalState *local_state  = rdim_local_init();
  RDIM_BakeParams *convert2bake = 0;
  switch (rc->driver) {
  case RC_Driver_Null: break;
  case RC_Driver_Dwarf: convert2bake = d2r_convert(scratch.arena, local_state, rc); break;
  case RC_Driver_Pdb:   convert2bake = p2r_convert(scratch.arena, local_state, rc); break;
  }
  ProfEnd();

  if (rc->errors.node_count) {
    NotImplemented;
  }

  ProfBegin("Bake");
  RDIM_BakeResults bake2srlz = rdim_bake(local_state, convert2bake);
  ProfEnd();

  ProfBegin("Serialize Bake");
  RDIM_SerializedSectionBundle srlz2file = rdim_serialized_section_bundle_from_bake_results(&bake2srlz);
  ProfEnd();

  RDIM_SerializedSectionBundle srlz2file_compressed = srlz2file;
  if (rc->flags & RC_Flag_Compress) {
    ProfBegin("Compress");
    srlz2file_compressed = rdim_compress(scratch.arena, &srlz2file);
    ProfEnd();
  }

  ProfBegin("Serialize");
  String8List raw_rdi = rdim_file_blobs_from_section_bundle(scratch.arena, &srlz2file_compressed);
  ProfEnd();

  scratch_end(scratch);
  return raw_rdi;
}

internal String8
rc_rdi_from_cmd_line(Arena *arena, CmdLine *cmdl)
{
  Temp scratch = scratch_begin(&arena, 1);
  RC_Context  rc      = rc_context_from_cmd_line(scratch.arena, cmdl);
  String8List raw_rdi = rc_run(scratch.arena, &rc);
  String8     result  = str8_list_join(arena, &raw_rdi, 0);
  scratch_end(scratch);
  return result;
}

internal void
rc_main(CmdLine *cmdl)
{
  B32 do_help = (cmd_line_has_flag(cmdl, str8_lit("help")) ||
                 cmd_line_has_flag(cmdl, str8_lit("h")) ||
                 cmd_line_has_flag(cmdl, str8_lit("?")) ||
                 cmdl->argc == 1);
  if (do_help) {
    fprintf(stderr, "--- Help ---------------------------------------------------------------------\n");
    fprintf(stderr, " %s\n\n", BUILD_TITLE_STRING_LITERAL);
    fprintf(stderr, " Usage: radcon [Options] [Files]\n\n");
    fprintf(stderr, " Options:\n");
    fprintf(stderr, "  -pe:<path>          Path to Win32 executable image\n");
    fprintf(stderr, "  -pdb:<path>         Path to PDB\n");
    fprintf(stderr, "  -elf:<path>         Path to ELF\n");
    fprintf(stderr, "  -elf_debug:<path>   Path to ELF with debug info\n");
    fprintf(stderr, "  -out:<path>         Path at which the output RDI debug info will be written\n");
    fprintf(stderr, "  -driver:<PDB|DWARF> Sets converter for debug info\n");
  } else {
    Temp scratch = scratch_begin(0,0);

    // make converter context
    RC_Context rc = rc_context_from_cmd_line(scratch.arena, cmdl);

    // make RDI from context
    String8List raw_rdi = rc_run(scratch.arena, &rc);

    // output RDI
    if (rc.errors.node_count == 0) {
      if (!os_write_data_list_to_file_path(rc.out_name, raw_rdi)) {
        str8_list_pushf(scratch.arena, &rc.errors, "no write access to path %.*s", str8_varg(rc.out_name));
      }
    }

    // report any errors
    for (String8Node *error_n = rc.errors.first; error_n != 0; error_n = error_n->next) {
      fprintf(stderr, "error: %.*s\n", str8_varg(error_n->string));
    }

    scratch_end(scratch);
  }
}

