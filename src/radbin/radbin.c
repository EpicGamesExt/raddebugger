// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "radbin/generated/radbin.meta.c"

////////////////////////////////
//~ rjf: Top-Level Entry Points

internal void
rb_entry_point(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0, 0);
  U64 threads_count = os_get_system_info()->logical_processor_count;
  String8 threads_count_from_cmdline_string = cmd_line_string(cmdline, str8_lit("thread_count"));
  if(threads_count_from_cmdline_string.size != 0)
  {
    U64 threads_count_from_cmdline = 0;
    if(try_u64_from_str8_c_rules(threads_count_from_cmdline_string, &threads_count_from_cmdline))
    {
      threads_count = threads_count_from_cmdline;
    }
  }
  OS_Handle *threads = push_array(scratch.arena, OS_Handle, threads_count);
  RB_ThreadParams *threads_params = push_array(scratch.arena, RB_ThreadParams, threads_count);
  Barrier barrier = barrier_alloc(threads_count);
  for EachIndex(idx, threads_count)
  {
    threads_params[idx].cmdline = cmdline;
    threads_params[idx].lane_ctx.lane_idx   = idx;
    threads_params[idx].lane_ctx.lane_count = threads_count;
    threads_params[idx].lane_ctx.barrier    = barrier;
    threads[idx] = os_thread_launch(rb_thread_entry_point, &threads_params[idx], 0);
  }
  for EachIndex(idx, threads_count)
  {
    os_thread_join(threads[idx], max_U64);
  }
  scratch_end(scratch);
}

internal void
rb_thread_entry_point(void *p)
{
  RB_ThreadParams *params = (RB_ThreadParams *)p;
  CmdLine *cmdline = params->cmdline;
  LaneCtx lctx = params->lane_ctx;
  ThreadNameF("radbin_thread_%I64u", lctx.lane_idx);
  lane_ctx(lctx);
  Arena *arena = arena_alloc();
  Log *log = log_alloc();
  log_select(log);
  log_scope_begin();
  
  //////////////////////////////
  //- rjf: set up shared state
  //
  if(lane_idx() == 0)
  {
    rb_shared = push_array(arena, RB_Shared, 1);
  }
  lane_sync();
  
  //////////////////////////////
  //- rjf: analyze & load command line input files
  //
  ProfScope("analyze & load command line input files") if(lane_idx() == 0)
  {
    String8List input_file_path_tasks = str8_list_copy(arena, &cmdline->inputs);
    for(String8Node *n = input_file_path_tasks.first; n != 0; n = n->next)
    {
      //////////////////////////
      //- rjf: do thin analysis of file
      //
      RB_FileFormat file_format = RB_FileFormat_Null;
      RB_FileFormatFlags file_format_flags = 0;
      ProfScope("do thin analysis of file")
      {
        OS_Handle file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, n->string);
        FileProperties props = os_properties_from_file(file);
        
        //- rjf: PDB magic -> PDB input
        if(file_format == RB_FileFormat_Null)
        {
          U8 msf20_magic_maybe[sizeof(msf_msf20_magic)] = {0};
          os_file_read(file, r1u64(0, sizeof(msf20_magic_maybe)), msf20_magic_maybe);
          if(MemoryMatch(msf20_magic_maybe, msf_msf20_magic, sizeof(msf20_magic_maybe)))
          {
            file_format = RB_FileFormat_PDB;
          }
        }
        if(file_format == RB_FileFormat_Null)
        {
          U8 msf70_magic_maybe[sizeof(msf_msf70_magic)] = {0};
          os_file_read(file, r1u64(0, sizeof(msf70_magic_maybe)), msf70_magic_maybe);
          if(MemoryMatch(msf70_magic_maybe, msf_msf70_magic, sizeof(msf70_magic_maybe)))
          {
            file_format = RB_FileFormat_PDB;
          }
        }
        
        //- rjf: PE magic -> PE input
        if(file_format == RB_FileFormat_Null)
        {
          PE_DosHeader dos_header_maybe = {0};
          os_file_read_struct(file, 0, &dos_header_maybe);
          if(dos_header_maybe.magic == PE_DOS_MAGIC)
          {
            U32 pe_magic_maybe = 0;
            os_file_read_struct(file, dos_header_maybe.coff_file_offset, &pe_magic_maybe);
            if(pe_magic_maybe == PE_MAGIC)
            {
              file_format = RB_FileFormat_PE;
            }
          }
        }
        
        //- rjf: COFF archive magic -> COFF archive input
        if(file_format == RB_FileFormat_Null)
        {
          U8 coff_archive_sig_maybe[sizeof(g_coff_archive_sig)] = {0};
          os_file_read(file, r1u64(0, sizeof(coff_archive_sig_maybe)), coff_archive_sig_maybe);
          if(MemoryMatch(coff_archive_sig_maybe, g_coff_archive_sig, sizeof(g_coff_archive_sig)))
          {
            file_format = RB_FileFormat_COFF_Archive;
          }
        }
        if(file_format == RB_FileFormat_Null)
        {
          U8 coff_thin_archive_sig_maybe[sizeof(g_coff_thin_archive_sig)] = {0};
          os_file_read(file, r1u64(0, sizeof(coff_thin_archive_sig_maybe)), coff_thin_archive_sig_maybe);
          if(MemoryMatch(coff_thin_archive_sig_maybe, g_coff_thin_archive_sig, sizeof(g_coff_thin_archive_sig)))
          {
            file_format = RB_FileFormat_COFF_ThinArchive;
          }
        }
        
        //- rjf: COFF obj magic -> COFF obj input
        if(file_format == RB_FileFormat_Null)
        {
          COFF_BigObjHeader header_maybe = {0};
          os_file_read_struct(file, 0, &header_maybe);
          if(header_maybe.sig1 == COFF_MachineType_Unknown &&
             header_maybe.sig2 == max_U16 &&
             header_maybe.version >= 2 &&
             MemoryMatch(header_maybe.magic, g_coff_big_header_magic, sizeof(header_maybe.magic)))
          {
            file_format = RB_FileFormat_COFF_BigOBJ;
          }
        }
        if(file_format == RB_FileFormat_Null)
        {
          Temp scratch = scratch_begin(&arena, 1);
          COFF_FileHeader header_maybe = {0};
          os_file_read_struct(file, 0, &header_maybe);
          U64 section_count = header_maybe.section_count;
          U64 section_hdr_opl_off = sizeof(header_maybe) + section_count*sizeof(COFF_SectionHeader);
          
          // rjf: check if machine type is valid
          B32 machine_type_is_valid = 0;
          switch(header_maybe.machine)
          {
            case COFF_MachineType_Unknown:
            case COFF_MachineType_X86:    case COFF_MachineType_X64:
            case COFF_MachineType_Am33:   case COFF_MachineType_Arm:
            case COFF_MachineType_Arm64:  case COFF_MachineType_ArmNt:
            case COFF_MachineType_Ebc:    case COFF_MachineType_Ia64:
            case COFF_MachineType_M32R:   case COFF_MachineType_Mips16:
            case COFF_MachineType_MipsFpu:case COFF_MachineType_MipsFpu16:
            case COFF_MachineType_PowerPc:case COFF_MachineType_PowerPcFp:
            case COFF_MachineType_R4000:  case COFF_MachineType_RiscV32:
            case COFF_MachineType_RiscV64:case COFF_MachineType_RiscV128:
            case COFF_MachineType_Sh3:    case COFF_MachineType_Sh3Dsp:
            case COFF_MachineType_Sh4:    case COFF_MachineType_Sh5:
            case COFF_MachineType_Thumb:  case COFF_MachineType_WceMipsV2:
            {
              machine_type_is_valid = 1;
            }break;
          }
          
          // rjf: check if sections are valid
          B32 sections_are_valid = 0;
          if(machine_type_is_valid)
          {
            if(props.size >= section_hdr_opl_off)
            {
              COFF_SectionHeader *section_hdrs = push_array(scratch.arena, COFF_SectionHeader, section_count);
              os_file_read(file, r1u64(sizeof(header_maybe), sizeof(header_maybe) + section_count*sizeof(COFF_SectionHeader)), section_hdrs);
              B32 section_ranges_valid = 1;
              for EachIndex(section_hdr_idx, section_count)
              {
                COFF_SectionHeader *hdr = &section_hdrs[section_hdr_idx];
                if(!(hdr->flags & COFF_SectionFlag_CntUninitializedData))
                {
                  U64 min = hdr->foff;
                  U64 max = min + hdr->fsize;
                  if(hdr->fsize > 0 && !(section_hdr_opl_off <= min && min <= max && max <= props.size))
                  {
                    section_ranges_valid = 0;
                    break;
                  }
                }
              }
              sections_are_valid = section_ranges_valid;
            }
          }
          
          // rjf: check if symbol table is valid
          B32 symbol_table_is_valid = 0;
          if(sections_are_valid)
          {
            U64 symbol_table_off = header_maybe.symbol_table_foff;
            U64 symbol_table_size = sizeof(COFF_Symbol16)*header_maybe.symbol_count;
            U64 symbol_table_opl_off = symbol_table_off+symbol_table_size;
            if(symbol_table_off == 0 && symbol_table_size == 0)
            {
              symbol_table_off = section_hdr_opl_off;
              symbol_table_opl_off = section_hdr_opl_off;
            }
            symbol_table_is_valid = (section_hdr_opl_off <= symbol_table_off &&
                                     symbol_table_off <= symbol_table_opl_off &&
                                     symbol_table_opl_off <= props.size);
          }
          
          // rjf: symbol table is valid -> is COFF OBJ
          if(symbol_table_is_valid)
          {
            file_format = RB_FileFormat_COFF_OBJ;
          }
          
          scratch_end(scratch);
        }
        
        //- rjf: ELF magic -> ELF input
        if(file_format == RB_FileFormat_Null)
        {
          U8 identifier_maybe[ELF_Identifier_Max] = {0};
          os_file_read(file, r1u64(0, sizeof(identifier_maybe)), identifier_maybe);
          B32 is_elf_magic = (identifier_maybe[ELF_Identifier_Mag0] == 0x7f &&
                              identifier_maybe[ELF_Identifier_Mag1] == 'E'  &&
                              identifier_maybe[ELF_Identifier_Mag2] == 'L'  &&
                              identifier_maybe[ELF_Identifier_Mag3] == 'F');
          if(is_elf_magic)
          {
            file_format = ELF_HdrIs64Bit(identifier_maybe) ? RB_FileFormat_ELF64 : RB_FileFormat_ELF32;
          }
        }
        
        //- rjf: RDI magic -> RDI input
        if(file_format == RB_FileFormat_Null)
        {
          RDI_Header rdi_header_maybe = {0};
          os_file_read_struct(file, 0, &rdi_header_maybe);
          if(rdi_header_maybe.magic == RDI_MAGIC_CONSTANT)
          {
            file_format = RB_FileFormat_RDI;
          }
        }
        
        os_file_close(file);
      }
      
      //////////////////////////
      //- rjf: log file recognition
      //
      if(file_format != RB_FileFormat_Null)
      {
        log_infof("%S recognized as %S\n", n->string, rb_file_format_display_name_table[file_format]);
      }
      else
      {
        log_infof("%S was not recognized as a supported format.\n", n->string);
      }
      
      //////////////////////////
      //- rjf: load recognized files
      //
      String8 file_data = {0};
      if(file_format != RB_FileFormat_Null) ProfScope("load recognized file")
      {
        file_data = os_data_from_file_path(arena, n->string);
      }
      
      //////////////////////////
      //- rjf: PE format => generate new implicit path tasks for PDBs
      //
      if(file_format == RB_FileFormat_PE) ProfScope("PE file => generate task for PDB")
      {
        Temp scratch = scratch_begin(&arena, 1);
        PE_BinInfo pe_bin_info = pe_bin_info_from_data(scratch.arena, file_data);
        String8 raw_debug_dir = str8_substr(file_data, pe_bin_info.data_dir_franges[PE_DataDirectoryIndex_DEBUG]);
        PE_DebugInfoList debug_dir = pe_debug_info_list_from_raw_debug_dir(scratch.arena, file_data, raw_debug_dir);
        for(PE_DebugInfoNode *n = debug_dir.first; n != 0; n = n->next)
        {
          if(n->v.path.size != 0)
          {
            str8_list_push(arena, &input_file_path_tasks, n->v.path);
          }
        }
        scratch_end(scratch);
      }
      
      //////////////////////////
      //- rjf: ELF format => generate new implicit path tasks for debug files
      //
      if(file_format == RB_FileFormat_ELF32 ||
         file_format == RB_FileFormat_ELF64)
      {
        Temp scratch = scratch_begin(&arena, 1);
        ELF_Bin bin = elf_bin_from_data(scratch.arena, file_data);
        ELF_GnuDebugLink debug_link = elf_gnu_debug_link_from_bin(file_data, &bin);
        if(debug_link.path.size != 0)
        {
          str8_list_push(arena, &input_file_path_tasks, debug_link.path);
        }
        scratch_end(scratch);
      }
      
      //////////////////////////
      //- rjf: PE => check if contains DWARF
      //
      if(file_format == RB_FileFormat_PE)
      {
        Temp scratch = scratch_begin(&arena, 1);
        PE_BinInfo pe_bin_info = pe_bin_info_from_data(scratch.arena, file_data);
        String8 raw_section_table = str8_substr(file_data, pe_bin_info.section_table_range);
        String8 string_table = str8_substr(file_data, pe_bin_info.string_table_range);
        U64 section_count = raw_section_table.size / sizeof(COFF_SectionHeader);
        COFF_SectionHeader *section_table = (COFF_SectionHeader *)raw_section_table.str;
        if(dw_is_dwarf_present_coff_section_table(file_data, string_table, section_count, section_table))
        {
          file_format_flags |= RB_FileFormatFlag_HasDWARF;
        }
        scratch_end(scratch);
      }
      
      //////////////////////////
      //- rjf: ELF => check if contains DWARF
      //
      if(file_format == RB_FileFormat_ELF32 ||
         file_format == RB_FileFormat_ELF64)
      {
        Temp scratch = scratch_begin(&arena, 1);
        ELF_Bin elf_bin = elf_bin_from_data(scratch.arena, file_data);
        if(dw_is_dwarf_present_from_elf_bin(file_data, &elf_bin))
        {
          file_format_flags |= RB_FileFormatFlag_HasDWARF;
        }
        scratch_end(scratch);
      }
      
      //////////////////////////
      //- rjf: push to list
      //
      {
        RB_File *f = push_array(arena, RB_File, 1);
        f->format       = file_format;
        f->format_flags = file_format_flags;
        f->path         = n->string;
        f->data         = file_data;
        RB_FileNode *file_n = push_array(arena, RB_FileNode, 1);
        file_n->v = f;
        SLLQueuePush(rb_shared->input_files.first, rb_shared->input_files.last, file_n);
        rb_shared->input_files.count += 1;
      }
    }
  }
  lane_sync();
  RB_FileList input_files = rb_shared->input_files;
  
  //////////////////////////////
  //- rjf: bucket input files by format
  //
  ProfScope("bucket input files by format") if(lane_idx() == 0)
  {
    for(RB_FileNode *n = input_files.first; n != 0; n = n->next)
    {
      RB_FileNode *file_n = push_array(arena, RB_FileNode, 1);
      file_n->v = n->v;
      SLLQueuePush(rb_shared->input_files_from_format_table[n->v->format].first, rb_shared->input_files_from_format_table[n->v->format].last, file_n);
      rb_shared->input_files_from_format_table[n->v->format].count += 1;
    }
  }
  lane_sync();
  RB_FileList *input_files_from_format_table = rb_shared->input_files_from_format_table;
  
  //////////////////////////////
  //- rjf: unpack which kind of output we're producing, and to where
  //
  typedef enum OutputKind
  {
    OutputKind_Null,
    OutputKind_RDI,
    OutputKind_Dump,
    OutputKind_Breakpad,
    OutputKind_COUNT
  }
  OutputKind;
  struct
  {
    String8 arg;
    String8 title;
  }
  output_kind_info[] =
  {
    {str8_lit_comp(""),         str8_lit_comp("")},
    {str8_lit_comp("rdi"),      str8_lit_comp("RAD Debug Info (.rdi) Conversion")},
    {str8_lit_comp("dump"),     str8_lit_comp("Textual Dumping")},
    {str8_lit_comp("breakpad"), str8_lit_comp("Breakpad Debug Info Conversion")},
  };
  OutputKind output_kind = OutputKind_Null;
  String8 output_path = cmd_line_string(cmdline, str8_lit("out"));
  {
    //- rjf: user manually specified output kind
    if(output_kind == OutputKind_Null)
    {
      for EachNonZeroEnumVal(OutputKind, k)
      {
        if(cmd_line_has_flag(cmdline, output_kind_info[k].arg))
        {
          log_infof("User specified --%S; performing `%S`\n", output_kind_info[k].arg, output_kind_info[k].title);
          output_kind = k;
          break;
        }
      }
    }
    
    //- rjf: we can infer from the user-specified output path
    if(str8_match(str8_skip_last_dot(output_path), str8_lit("rdi"), StringMatchFlag_CaseInsensitive))
    {
      output_kind = OutputKind_RDI;
      log_infof("Output path has .rdi extension; performing `%S`\n", output_kind_info[output_kind].title);
    }
    else if(str8_match(str8_skip_last_dot(output_path), str8_lit("dump"), StringMatchFlag_CaseInsensitive) ||
            str8_match(str8_skip_last_dot(output_path), str8_lit("txt"), StringMatchFlag_CaseInsensitive))
    {
      output_kind = OutputKind_Dump;
      log_infof("Output path has .dump or .txt extension; performing `%S`\n", output_kind_info[output_kind].title);
    }
    else if(str8_match(str8_skip_last_dot(output_path), str8_lit("psym"), StringMatchFlag_CaseInsensitive) ||
            str8_match(str8_skip_last_dot(output_path), str8_lit("psyms"), StringMatchFlag_CaseInsensitive))
    {
      output_kind = OutputKind_Breakpad;
      log_infof("Output path has .psym or .psyms extension; performing `%S`\n", output_kind_info[output_kind].title);
    }
  }
  
  //////////////////////////////
  //- rjf: print help preamble
  //
  if(lane_idx() == 0)
  {
    if(output_kind == OutputKind_Null || cmdline->inputs.node_count == 0)
    {
      fprintf(stderr, "%s\n", BUILD_TITLE);
      fprintf(stderr, "%s\n\n", BUILD_VERSION_STRING_LITERAL);
      if(output_kind != OutputKind_Null)
      {
        fprintf(stderr, "%.*s Help\n", str8_varg(output_kind_info[output_kind].title));
        fprintf(stderr, "To see top-level options for radbin, run the binary with no arguments.\n\n");
      }
      fprintf(stderr, "-------------------------------------------------------------------------------\n\n");
    }
  }
  lane_sync();
  
  //////////////////////////////
  //- rjf: perform operation based on output kind
  //
  String8List output_blobs = {0};
  switch(output_kind)
  {
    ////////////////////////////
    //- rjf: no output kind -> nothing to do. output help information to user
    //
    default:
    case OutputKind_Null:
    if(lane_idx() == 0)
    {
      fprintf(stderr, "USAGE EXAMPLES\n\n");
      
      fprintf(stderr, "radbin --rdi program.exe\n");
      fprintf(stderr, "Find the debug info for `program.exe`, if any, and converts it to RDI.\n\n");
      
      fprintf(stderr, "radbin program.pdb --out:program.rdi\n");
      fprintf(stderr, "Converts `program.pdb` to RDI, and stores it in `program.rdi`.\n\n");
      
      fprintf(stderr, "radbin --dump program.rdi\n");
      fprintf(stderr, "Outputs the textual dump of the debug information stored in `program.rdi`.\n\n");
      
      fprintf(stderr, "-------------------------------------------------------------------------------\n\n");
      
      fprintf(stderr, "DESCRIPTION\n\n");
      fprintf(stderr, "This utility provides a number of operations relating to executable image and\n");
      fprintf(stderr, "debug information formats. It can convert debug information to the RAD Debug\n");
      fprintf(stderr, "Info (.rdi) format. It can also parse and dump textualized contents of several\n");
      fprintf(stderr, "formats, including PDB, PE, ELF, and RDI. It also supports converting debug\n");
      fprintf(stderr, "information to the textual Breakpad format.\n\n");
      
      fprintf(stderr, "-------------------------------------------------------------------------------\n\n");
      
      fprintf(stderr, "ARGUMENTS\n\n");
      
      fprintf(stderr, "--rdi            Specifies that the utility should convert debug information\n");
      fprintf(stderr, "                 data to the RAD Debug Info (.rdi) format.\n\n");
      
      fprintf(stderr, "--dump           Specifies that the utility should dump textualized contents of\n");
      fprintf(stderr, "                 all input files.\n\n");
      
      fprintf(stderr, "--breakpad       Specifies that the utility should convert debug information\n");
      fprintf(stderr, "                 data to the textual Breakpad format.\n\n");
      
      fprintf(stderr, "--out:<path>     Specifies the path to which output data should be written. If\n");
      fprintf(stderr, "                 not specified, the utility will choose a fallback. If dumping\n");
      fprintf(stderr, "                 textual contents, the utility will write to `stdout`. If\n");
      fprintf(stderr, "                 converting to another format, the utility will form a path by\n");
      fprintf(stderr, "                 changing the extension of input files accordingly.\n\n");
      
      fprintf(stderr, "--deterministic  Turns off all sources of non-determinism in generated output,\n");
      fprintf(stderr, "                 like build names, versions, and dates.\n\n");
      
      fprintf(stderr, "--verbose        Outputs all log information collected during execution.\n\n");
      
      fprintf(stderr, "There are also operation-specific arguments. To see them, run this binary with\n");
      fprintf(stderr, "the operation selected, with no additional inputs (e.g. `radbin --rdi`).\n\n");
    }break;
    
    ////////////////////////////
    //- rjf: RDI, Breakpad -> conversion based on inputs
    //
    case OutputKind_RDI:
    case OutputKind_Breakpad:
    {
      //- rjf: no inputs => help
      if(lane_idx() == 0 && cmdline->inputs.node_count == 0) switch(output_kind)
      {
        default:
        case OutputKind_RDI:
        {
          fprintf(stderr, "ARGUMENTS\n\n");
          
          fprintf(stderr, "--compress                       Compresses the RDI file's contents.\n");
          fprintf(stderr, "\n");
          fprintf(stderr, "--only:<comma delimited names>   Specifies that only the named subsets of debug\n");
          fprintf(stderr, "                                 information should be generated. See below for\n");
          fprintf(stderr, "                                 a list of valid debug info subset names.\n");
          fprintf(stderr, "\n");
          fprintf(stderr, "--omit:<comma delimited names>   Specifies that the named subsets of debug\n");
          fprintf(stderr, "                                 information should not be generated. See below\n");
          fprintf(stderr, "                                 for a list of valid debug info subset names.\n");
          fprintf(stderr, "\n");
          
          fprintf(stderr, "-------------------------------------------------------------------------------\n\n");
          
          fprintf(stderr, "RAD DEBUG INFO SUBSET NAMES\n\n");
#define X(name, name_lower) fprintf(stderr, " - " #name_lower "\n");
          RDIM_Subset_XList
#undef X
        }break;
        case OutputKind_Breakpad:
        {
          fprintf(stderr, "All input files specified on the command line will be dumped. The following\n");
          fprintf(stderr, "formats are currently supported: PE, COFF, RDI, and ELF\n\n");
        }
      }
      
      //- rjf: unpack subset flags
      RDIM_SubsetFlags subset_flags = 0xffffffff;
      switch(output_kind)
      {
        default:{}break;
        case OutputKind_RDI:
        {
          String8List only_names = cmd_line_strings(cmdline, str8_lit("only"));
          if(only_names.node_count != 0)
          {
            subset_flags = 0;
          }
          for(String8Node *n = only_names.first; n != 0; n = n->next)
          {
            if(0){}
#define X(name, name_lower) else if(str8_match(n->string, str8_lit(#name_lower), 0)) { subset_flags |= RDIM_SubsetFlag_##name; }
            RDIM_Subset_XList
#undef X
          }
          String8List omit_names = cmd_line_strings(cmdline, str8_lit("omit"));
          for(String8Node *n = omit_names.first; n != 0; n = n->next)
          {
            if(0){}
#define X(name, name_lower) else if(str8_match(n->string, str8_lit(#name_lower), 0)) { subset_flags &= ~RDIM_SubsetFlag_##name; }
            RDIM_Subset_XList
#undef X
          }
        }break;
        case OutputKind_Breakpad:
        {
          subset_flags = RDIM_SubsetFlag_All & ~(RDIM_SubsetFlag_Types|RDIM_SubsetFlag_UDTs);
        }break;
      }
      
      //- rjf: convert inputs to RDI info
      B32 convert_done = 0;
      RDIM_BakeParams bake_params = {0};
      {
        //- rjf: PE inputs w/ DWARF, or ELF inputs => DWARF -> RDI conversion
        if(!convert_done &&
           ((input_files_from_format_table[RB_FileFormat_PE].count != 0 &&
             input_files_from_format_table[RB_FileFormat_PE].first->v->format_flags & RB_FileFormatFlag_HasDWARF) ||
            (input_files_from_format_table[RB_FileFormat_ELF32].count != 0 ||
             input_files_from_format_table[RB_FileFormat_ELF64].count != 0)))
        {
          convert_done = 1;
          log_infof("PEs w/ DWARF, or ELFs specified; producing RDI by converting DWARF data\n");
          
          // rjf: convert
          D2R_ConvertParams convert_params = {0};
          {
            B32 got_exe = 0;
            B32 got_dbg = 0;
            if(!got_exe && !got_dbg)
            {
              for(RB_FileNode *n = input_files_from_format_table[RB_FileFormat_PE].first; n != 0; n = n->next)
              {
                if(n->v->format_flags & RB_FileFormatFlag_HasDWARF)
                {
                  got_exe = 1;
                  got_dbg = 1;
                  convert_params.dbg_name = n->v->path;
                  convert_params.dbg_data = n->v->data;
                  convert_params.exe_name = n->v->path;
                  convert_params.exe_data = n->v->data;
                  convert_params.exe_kind = ExecutableImageKind_CoffPe;
                  break;
                }
              }
            }
            if(!got_exe)
            {
              for(RB_FileNode *n = input_files_from_format_table[RB_FileFormat_ELF32].first; n != 0; n = n->next)
              {
                got_exe = 1;
                convert_params.exe_name = n->v->path;
                convert_params.exe_data = n->v->data;
                convert_params.exe_kind = ExecutableImageKind_Elf32;
                if(!(n->v->format_flags & RB_FileFormatFlag_HasDWARF))
                {
                  break;
                }
              }
            }
            if(!got_exe)
            {
              for(RB_FileNode *n = input_files_from_format_table[RB_FileFormat_ELF64].first; n != 0; n = n->next)
              {
                got_exe = 1;
                convert_params.exe_name = n->v->path;
                convert_params.exe_data = n->v->data;
                convert_params.exe_kind = ExecutableImageKind_Elf64;
                if(!(n->v->format_flags & RB_FileFormatFlag_HasDWARF))
                {
                  break;
                }
              }
            }
            if(!got_dbg)
            {
              for(RB_FileNode *n = input_files_from_format_table[RB_FileFormat_ELF32].first; n != 0; n = n->next)
              {
                if(n->v->format_flags & RB_FileFormatFlag_HasDWARF)
                {
                  got_dbg = 1;
                  convert_params.dbg_name = n->v->path;
                  convert_params.dbg_data = n->v->data;
                  break;
                }
              }
            }
            if(!got_dbg)
            {
              for(RB_FileNode *n = input_files_from_format_table[RB_FileFormat_ELF64].first; n != 0; n = n->next)
              {
                if(n->v->format_flags & RB_FileFormatFlag_HasDWARF)
                {
                  got_dbg = 1;
                  convert_params.dbg_name = n->v->path;
                  convert_params.dbg_data = n->v->data;
                  break;
                }
              }
            }
            convert_params.subset_flags   = subset_flags;
            convert_params.deterministic  = cmd_line_has_flag(cmdline, str8_lit("deterministic"));
          }
          ProfScope("convert") bake_params = d2r_convert(arena, &convert_params);
          
          // rjf: no output path? -> pick one based on debug
          if(output_path.size == 0)
          {
            output_path = push_str8f(arena, "%S.rdi", str8_chop_last_dot(convert_params.dbg_name));
          }
        }
        
        //- rjf: PDB inputs => PDB -> RDI conversion
        if(!convert_done &&
           input_files_from_format_table[RB_FileFormat_PDB].count != 0)
        {
          convert_done = 1;
          log_infof("PDBs specified; producing RDI by converting PDB data\n");
          
          // rjf: get EXE/PDB file data
          RB_File *exe_file = rb_file_list_first(&input_files_from_format_table[RB_FileFormat_PE]);
          RB_File *pdb_file = rb_file_list_first(&input_files_from_format_table[RB_FileFormat_PDB]);
          String8 exe_path  = exe_file->path;
          String8 pdb_path  = pdb_file->path;
          String8 exe_data  = exe_file->data;
          String8 pdb_data  = pdb_file->data;
          
          // rjf: convert
          P2R_ConvertParams convert_params = {0};
          {
            convert_params.input_pdb_name = pdb_path;
            convert_params.input_exe_name = exe_path;
            convert_params.input_pdb_data = pdb_data;
            convert_params.input_exe_data = exe_data;
            convert_params.subset_flags   = subset_flags;
            convert_params.deterministic  = cmd_line_has_flag(cmdline, str8_lit("deterministic"));
          }
          ProfScope("convert") bake_params = p2r2_convert(arena, &convert_params);
          
          // rjf: no output path? -> pick one based on PDB
          if(output_path.size == 0) switch(output_kind)
          {
            default:{}break;
            case OutputKind_RDI:
            {
              output_path = push_str8f(arena, "%S.rdi", str8_chop_last_dot(convert_params.input_pdb_name));
            }break;
            case OutputKind_Breakpad:
            {
              output_path = push_str8f(arena, "%S.psym", str8_chop_last_dot(convert_params.input_pdb_name));
            }break;
          }
        }
      }
      
      //- rjf: no viable input paths
      if(!convert_done && cmdline->inputs.node_count != 0)
      {
        log_user_errorf("Could not load debug info from the specified inputs. You must provide either a valid PDB file or an executable image (PE, ELF) file with DWARF debug info.");
      }
      
      //- rjf: convert done => generate output
      if(convert_done) switch(output_kind)
      {
        default:{}break;
        
        //- rjf: generate RDI blobs
        case OutputKind_RDI:
        {
          // rjf: bake
          RDIM_BakeResults bake_results = {0};
          ProfScope("bake") bake_results = rdim2_bake(arena, &bake_params);
          
          // rjf: serialize
          RDIM_SerializedSectionBundle serialized_section_bundle = {0};
          ProfScope("serialize") serialized_section_bundle = rdim_serialized_section_bundle_from_bake_results(&bake_results);
          
          // rjf: compress
          RDIM_SerializedSectionBundle serialized_section_bundle__compressed = serialized_section_bundle;
          if(cmd_line_has_flag(cmdline, str8_lit("compress"))) ProfScope("compress")
          {
            serialized_section_bundle__compressed = rdim_compress(arena, &serialized_section_bundle);
          }
          
          // rjf: serialize
          String8List blobs = rdim_file_blobs_from_section_bundle(arena, &serialized_section_bundle__compressed);
          str8_list_concat_in_place(&output_blobs, &blobs);
        }break;
        
        //- rjf: generate breakpad text
        case OutputKind_Breakpad:
        {
          String8List dump = {0};
          
          //- rjf: kick off unit vmap baking
          P2B_BakeUnitVMapIn bake_unit_vmap_in = {&bake_params.units};
          ASYNC_Task *bake_unit_vmap_task = async_task_launch(arena, p2b_bake_unit_vmap_work, .input = &bake_unit_vmap_in);
          
          //- rjf: kick off line-table baking
          P2B_BakeLineTablesIn bake_line_tables_in = {&bake_params.line_tables};
          ASYNC_Task *bake_line_tables_task = async_task_launch(arena, p2b_bake_line_table_work, .input = &bake_line_tables_in);
          
          //- rjf: build unit -> line table idx array
          U64 unit_count = bake_params.units.total_count;
          U32 *unit_line_table_idxs = push_array(arena, U32, unit_count+1);
          {
            U64 dst_idx = 1;
            for(RDIM_UnitChunkNode *n = bake_params.units.first; n != 0; n = n->next)
            {
              for(U64 n_idx = 0; n_idx < n->count; n_idx += 1, dst_idx += 1)
              {
                unit_line_table_idxs[dst_idx] = rdim_idx_from_line_table(n->v[n_idx].line_table);
              }
            }
          }
          
          //- rjf: dump MODULE record
          str8_list_pushf(arena, &dump, "MODULE windows x86_64 %I64x %S\n", bake_params.top_level_info.exe_hash, bake_params.top_level_info.exe_name);
          
          //- rjf: dump FILE records
          ProfScope("dump FILE records")
          {
            for(RDIM_SrcFileChunkNode *n = bake_params.src_files.first; n != 0; n = n->next)
            {
              for(U64 idx = 0; idx < n->count; idx += 1)
              {
                U64 file_idx = rdim_idx_from_src_file(&n->v[idx]);
                String8 src_path = n->v[idx].path;
                str8_list_pushf(arena, &dump, "FILE %I64u %S\n", file_idx, src_path);
              }
            }
          }
          
          //- rjf: join unit vmap
          ProfBegin("join unit vmap");
          RDIM_UnitVMapBakeResult *bake_unit_vmap_out = async_task_join_struct(bake_unit_vmap_task, RDIM_UnitVMapBakeResult);
          RDI_VMapEntry *unit_vmap = bake_unit_vmap_out->vmap.vmap;
          U32 unit_vmap_count = bake_unit_vmap_out->vmap.count;
          ProfEnd();
          
          //- rjf: join line tables
          ProfBegin("join line table");
          RDIM_LineTableBakeResult *bake_line_tables_out = async_task_join_struct(bake_line_tables_task, RDIM_LineTableBakeResult);
          ProfEnd();
          
          //- rjf: kick off FUNC & line record dump tasks
          P2B_DumpProcChunkIn *dump_proc_chunk_in = push_array(arena, P2B_DumpProcChunkIn, bake_params.procedures.chunk_count);
          ASYNC_Task **dump_proc_chunk_tasks = push_array(arena, ASYNC_Task *, bake_params.procedures.chunk_count);
          ProfScope("kick off FUNC & line record dump tasks")
          {
            U64 task_idx = 0;
            for(RDIM_SymbolChunkNode *n = bake_params.procedures.first; n != 0; n = n->next, task_idx += 1)
            {
              dump_proc_chunk_in[task_idx].unit_vmap            = unit_vmap;
              dump_proc_chunk_in[task_idx].unit_vmap_count      = unit_vmap_count;
              dump_proc_chunk_in[task_idx].unit_line_table_idxs = unit_line_table_idxs;
              dump_proc_chunk_in[task_idx].unit_count           = unit_count;
              dump_proc_chunk_in[task_idx].line_tables_bake     = bake_line_tables_out;
              dump_proc_chunk_in[task_idx].chunk                = n;
              dump_proc_chunk_tasks[task_idx] = async_task_launch(arena, p2b_dump_proc_chunk_work, .input = &dump_proc_chunk_in[task_idx]);
            }
          }
          
          //- rjf: join FUNC & line record dump tasks
          ProfScope("join FUNC & line record dump tasks")
          {
            for(U64 idx = 0; idx < bake_params.procedures.chunk_count; idx += 1)
            {
              String8List *out = async_task_join_struct(dump_proc_chunk_tasks[idx], String8List);
              str8_list_concat_in_place(&dump, out);
            }
          }
          
          str8_list_concat_in_place(&output_blobs, &dump);
        }break;
      }
    }break;
    
    ////////////////////////////
    //- rjf: dump -> textual dump of inputs
    //
    case OutputKind_Dump:
    {
      B32 deterministic = cmd_line_has_flag(cmdline, str8_lit("deterministic"));
      
      //- rjf: no inputs => help
      if(lane_idx() == 0 && cmdline->inputs.node_count == 0)
      {
        fprintf(stderr, "All input files specified on the command line will be dumped. Currently, only\n");
        fprintf(stderr, "RDI files are supported.\n\n");
        
        fprintf(stderr, "-------------------------------------------------------------------------------\n\n");
        
        fprintf(stderr, "ARGUMENTS\n\n");
        
        fprintf(stderr, "--only:<comma delimited names>   Specifies that only the named subsets should\n");
        fprintf(stderr, "                                 be dumped. See below for a list of valid\n");
        fprintf(stderr, "                                 subset names.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "--omit:<comma delimited names>   Specifies that the named subsets should not\n");
        fprintf(stderr, "                                 be dumped. See below for a list of valid\n");
        fprintf(stderr, "                                 subset names.\n");
        fprintf(stderr, "\n");
        
        fprintf(stderr, "-------------------------------------------------------------------------------\n\n");
        
        fprintf(stderr, "RAD DEBUG INFO SUBSET NAMES\n\n");
#define X(name, name_lower, title) fprintf(stderr, " - " #name_lower "\n");
        RDI_DumpSubset_XList
#undef X
        fprintf(stderr, "\n");
        
        fprintf(stderr, "-------------------------------------------------------------------------------\n\n");
        
        fprintf(stderr, "DWARF INFO SUBSET NAMES\n\n");
#define X(name, name_lower, title) fprintf(stderr, " - " #name_lower "\n");
        DW_DumpSubset_XList
#undef X
        fprintf(stderr, "\n");
      }
      
      //- rjf: unpack dump subset flags
      RDI_DumpSubsetFlags rdi_dump_subset_flags = RDI_DumpSubsetFlag_All;
      DW_DumpSubsetFlags dw_dump_subset_flags = DW_DumpSubsetFlag_All;
      {
        String8List only_names = cmd_line_strings(cmdline, str8_lit("only"));
        if(only_names.node_count != 0)
        {
          rdi_dump_subset_flags = 0;
          dw_dump_subset_flags = 0;
        }
        for(String8Node *n = only_names.first; n != 0; n = n->next)
        {
          if(0){}
#define X(name, name_lower, title) else if(str8_match(n->string, str8_lit(#name_lower), 0)) { rdi_dump_subset_flags |= RDI_DumpSubsetFlag_##name; }
          RDI_DumpSubset_XList
#undef X
#define X(name, name_lower, title) else if(str8_match(n->string, str8_lit(#name_lower), 0)) { dw_dump_subset_flags |= DW_DumpSubsetFlag_##name; }
          DW_DumpSubset_XList
#undef X
        }
        String8List omit_names = cmd_line_strings(cmdline, str8_lit("omit"));
        for(String8Node *n = omit_names.first; n != 0; n = n->next)
        {
          if(0){}
#define X(name, name_lower, title) else if(str8_match(n->string, str8_lit(#name_lower), 0)) { rdi_dump_subset_flags &= ~RDI_DumpSubsetFlag_##name; }
          RDI_DumpSubset_XList
#undef X
#define X(name, name_lower, title) else if(str8_match(n->string, str8_lit(#name_lower), 0)) { dw_dump_subset_flags &= ~DW_DumpSubsetFlag_##name; }
          DW_DumpSubset_XList
#undef X
        }
      }
      
      //- rjf: dump input files in order
      for(RB_FileNode *n = input_files.first; n != 0; n = n->next)
      {
        RB_File *f = n->v;
        str8_list_pushf(arena, &output_blobs, "// %S (%S)\n\n", deterministic ? str8_skip_last_slash(f->path) : f->path, f->format ? rb_file_format_display_name_table[f->format] : str8_lit("Unsupported format"));
        
        //- rjf: unpack file parses
        Arch arch = Arch_Null;
        PE_BinInfo pe = {0};
        ELF_Bin elf = {0};
        DW_Input dw = {0};
        {
          if(f->format == RB_FileFormat_PE)
          {
            pe = pe_bin_info_from_data(arena, f->data);
            arch = pe.arch;
          }
          if(f->format == RB_FileFormat_ELF32 ||
             f->format == RB_FileFormat_ELF64)
          {
            elf = elf_bin_from_data(arena, f->data);
            arch = arch_from_elf_machine(elf.hdr.e_machine);
          }
          if(f->format_flags & RB_FileFormatFlag_HasDWARF)
          {
            if(f->format == RB_FileFormat_PE)
            {
              String8             raw_sections  = str8_substr(f->data, pe.section_table_range);
              U64                 section_count = raw_sections.size / sizeof(COFF_SectionHeader);
              COFF_SectionHeader *section_table = (COFF_SectionHeader *)raw_sections.str;
              String8 string_table = str8_substr(f->data, pe.string_table_range);
              dw = dw_input_from_coff_section_table(arena, f->data, string_table, section_count, section_table);
            }
            else if(f->format == RB_FileFormat_ELF32 ||
                    f->format == RB_FileFormat_ELF64)
            {
              dw = dw_input_from_elf_bin(arena, f->data, &elf);
            }
          }
        }
        
        //- rjf: dump file info based on format
        switch(f->format)
        {
          default:{}break;
          
          //- rjf: PDB
          case RB_FileFormat_PDB:
          {
            // TODO(rjf)
          }break;
          
          //- rjf: PE
          case RB_FileFormat_PE:
          {
            // TODO(rjf)
          }break;
          
          //- rjf: COFF OBJ
          case RB_FileFormat_COFF_OBJ:
          {
            // TODO(rjf)
          }break;
          
          //- rjf: COFF big OBJ
          case RB_FileFormat_COFF_BigOBJ:
          {
            // TODO(rjf)
          }break;
          
          //- rjf: COFF archive
          case RB_FileFormat_COFF_Archive:
          case RB_FileFormat_COFF_ThinArchive:
          {
            // TODO(rjf)
          }break;
          
          //- rjf: ELF
          case RB_FileFormat_ELF32:
          case RB_FileFormat_ELF64:
          {
            // TODO(rjf)
          }break;
          
          //- rjf: RDI file
          case RB_FileFormat_RDI:
          {
            RDI_Parsed rdi = {0};
            RDI_ParseStatus rdi_status = rdi_parse(f->data.str, f->data.size, &rdi);
            String8 error = {0};
            switch(rdi_status)
            {
              default:{}break;
              case RDI_ParseStatus_HeaderDoesNotMatch:      {log_user_errorf("RDI parse failure: header does not match\n");}break;
              case RDI_ParseStatus_UnsupportedVersionNumber:{log_user_errorf("RDI parse failure: unsupported version\n");}break;
              case RDI_ParseStatus_InvalidDataSecionLayout: {log_user_errorf("RDI parse failure: invalid data section layout\n");}break;
              case RDI_ParseStatus_Good:
              {
                String8List dump = rdi_dump_list_from_parsed(arena, &rdi, rdi_dump_subset_flags);
                str8_list_concat_in_place(&output_blobs, &dump);
              }break;
            }
          }break;
        }
        
        //- rjf: dump file extension info
        if(f->format_flags & RB_FileFormatFlag_HasDWARF)
        {
          str8_list_pushf(arena, &output_blobs, "// %S (%S) (DWARF)\n\n", deterministic ? str8_skip_last_slash(f->path) : f->path, f->format ? rb_file_format_display_name_table[f->format] : str8_lit("Unsupported format"));
          {
            String8List dump = dw_dump_list_from_sections(arena, &dw, arch, dw_dump_subset_flags);
            str8_list_concat_in_place(&output_blobs, &dump);
          }
        }
      }
    }break;
  }
  
  //////////////////////////////
  //- rjf: write outputs
  //
  if(lane_idx() == 0)
  {
    if(output_path.size != 0) ProfScope("write outputs [file]")
    {
      os_write_data_list_to_file_path(output_path, output_blobs);
      log_infof("Results written to %S", output_path);
    }
    else ProfScope("write outputs [stdout]")
    {
      for(String8Node *n = output_blobs.first; n != 0; n = n->next)
      {
        for(U64 off = 0; off < n->string.size;)
        {
          U64 size_to_write = Min(n->string.size - off, GB(2));
          fwrite(n->string.str + off, size_to_write, 1, stdout);
          off += size_to_write;
        }
      }
      log_info(str8_lit("Results written to stdout"));
    }
  }
  lane_sync();
  
  //////////////////////////////
  //- rjf: write info & errors
  //
  LogScopeResult log_scope = log_scope_end(arena);
  if(lane_idx() == 0)
  {
    if(cmd_line_has_flag(cmdline, str8_lit("verbose")) && log_scope.strings[LogMsgKind_Info].size != 0)
    {
      String8List lines = wrapped_lines_from_string(arena, log_scope.strings[LogMsgKind_Info], 80, 80, 0);
      for(String8Node *n = lines.first; n != 0; n = n->next)
      {
        fprintf(stderr, "%.*s\n", str8_varg(n->string));
      }
    }
    if(log_scope.strings[LogMsgKind_UserError].size != 0)
    {
      String8List lines = wrapped_lines_from_string(arena, log_scope.strings[LogMsgKind_UserError], 80, 80, 0);
      for(String8Node *n = lines.first; n != 0; n = n->next)
      {
        fprintf(stderr, "%.*s\n", str8_varg(n->string));
      }
    }
  }
}
