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
  Thread *threads = push_array(scratch.arena, Thread, threads_count);
  RB_ThreadParams *threads_params = push_array(scratch.arena, RB_ThreadParams, threads_count);
  Barrier barrier = barrier_alloc(threads_count);
  U64 broadcast_val = 0;
  for EachIndex(idx, threads_count)
  {
    threads_params[idx].cmdline = cmdline;
    threads_params[idx].lane_ctx.lane_idx         = idx;
    threads_params[idx].lane_ctx.lane_count       = threads_count;
    threads_params[idx].lane_ctx.barrier          = barrier;
    threads_params[idx].lane_ctx.broadcast_memory = &broadcast_val;
    threads[idx] = thread_launch(rb_thread_entry_point, &threads_params[idx]);
  }
  for EachIndex(idx, threads_count)
  {
    thread_join(threads[idx], max_U64);
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
        if(dw_is_dwarf_present_coff_section_table(string_table, section_count, section_table))
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
          subset_flags = (RDIM_SubsetFlag_Units|RDIM_SubsetFlag_Procedures|RDIM_SubsetFlag_Scopes|RDIM_SubsetFlag_LineInfo|RDIM_SubsetFlag_InlineLineInfo);
        }break;
      }
      
      //- rjf: convert inputs to RDI info
      B32 convert_done = 0;
      RDIM_BakeParams pdb_bake_params = {0};
      RDIM_BakeParams dwarf_bake_params = {0};
      {
        //- rjf: PE inputs w/ DWARF, or ELF inputs => DWARF -> RDI conversion
        if(((input_files_from_format_table[RB_FileFormat_PE].count != 0 &&
             input_files_from_format_table[RB_FileFormat_PE].first->v->format_flags & RB_FileFormatFlag_HasDWARF) ||
            (input_files_from_format_table[RB_FileFormat_ELF32].count != 0 ||
             input_files_from_format_table[RB_FileFormat_ELF64].count != 0)))
        {
          convert_done = 1;
          log_infof("PEs w/ DWARF, or ELFs specified; converting DWARF data to RDI\n");
          
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
          ProfScope("convert") dwarf_bake_params = d2r_convert(arena, &convert_params);
        }
        
        //- rjf: PDB inputs => PDB -> RDI conversion
        if(input_files_from_format_table[RB_FileFormat_PDB].count != 0)
        {
          convert_done = 1;
          log_infof("PDBs specified; converting PDB data to RDI\n");
          
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
          ProfScope("convert") pdb_bake_params = p2r_convert(arena, &convert_params);
        }
      }
      lane_sync();
      
      //- rjf: join conversion artifacts
      RDIM_BakeParams *bake_params = 0;
      if(lane_idx() == 0)
      {
        bake_params = push_array(arena, RDIM_BakeParams, 1);
        rdim_bake_params_concat_in_place(bake_params, &pdb_bake_params);
        rdim_bake_params_concat_in_place(bake_params, &dwarf_bake_params);
      }
      lane_sync_u64(&bake_params, 0);
      
      //- rjf: no output path? -> pick one based on input files
      if(output_path.size == 0)
      {
        String8 output_path__noext = {0};
        if(output_path__noext.size == 0) { output_path__noext = str8_chop_last_dot(rb_file_list_first(&input_files_from_format_table[RB_FileFormat_PDB])->path); }
        if(output_path__noext.size == 0) { output_path__noext = str8_chop_last_dot(rb_file_list_first(&input_files_from_format_table[RB_FileFormat_PE])->path); }
        if(output_path__noext.size == 0) { output_path__noext = str8_chop_last_dot(rb_file_list_first(&input_files_from_format_table[RB_FileFormat_ELF64])->path); }
        if(output_path__noext.size == 0) { output_path__noext = str8_chop_last_dot(rb_file_list_first(&input_files_from_format_table[RB_FileFormat_ELF32])->path); }
        switch(output_kind)
        {
          default:{}break;
          case OutputKind_RDI:
          {
            output_path = push_str8f(arena, "%S.rdi", output_path__noext);
          }break;
          case OutputKind_Breakpad:
          {
            output_path = push_str8f(arena, "%S.psym", output_path__noext);
          }break;
        }
      }
      
      //- rjf: no viable input paths
      if(!convert_done && cmdline->inputs.node_count != 0)
      {
        log_user_errorf("Could not load debug info from the specified inputs. You must provide either a valid PDB file or an executable image (PE, ELF) file with DWARF debug info.");
      }
      
      //- rjf: bake
      RDIM_BakeResults bake_results = {0};
      if(convert_done) ProfScope("bake")
      {
        bake_results = rdim_bake(arena, bake_params);
      }
      
      //- rjf: convert done => generate output
      if(convert_done) switch(output_kind)
      {
        default:{}break;
        
        //- rjf: generate RDI blobs
        case OutputKind_RDI:
        {
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
          //- rjf: set up shared state
          typedef struct P2B_Shared P2B_Shared;
          struct P2B_Shared
          {
            String8List dump;
            String8List *lane_chunk_file_dumps;
            String8List *lane_chunk_func_dumps;
          };
          local_persist P2B_Shared *p2b_shared = 0;
          if(lane_idx() == 0)
          {
            p2b_shared = push_array(arena, P2B_Shared, 1);
            p2b_shared->lane_chunk_file_dumps = push_array(arena, String8List, lane_count()*bake_params->src_files.chunk_count);
            p2b_shared->lane_chunk_func_dumps = push_array(arena, String8List, lane_count()*bake_params->procedures.chunk_count);
          }
          lane_sync();
          
          //- rjf: dump MODULE record
          if(lane_idx() == 0)
          {
            // rjf: pick name to identify module
            String8 module_name_string = bake_params->top_level_info.exe_name;
            if(module_name_string.size == 0 && input_files.first != 0)
            {
              module_name_string = input_files.first->v->path;
            }
            
            // rjf: pick string for unique code
            String8 unique_identifier_string = {0};
            if(unique_identifier_string.size == 0 && bake_params->top_level_info.exe_hash != 0)
            {
              unique_identifier_string = str8f(arena, "%I64x", bake_params->top_level_info.exe_hash);
            }
            if(unique_identifier_string.size == 0 && input_files.first != 0 && input_files.first->v->format == RB_FileFormat_PDB)
            {
              Temp scratch = scratch_begin(&arena, 1);
              String8 msf_data = input_files.first->v->data;
              MSF_RawStreamTable *st = msf_raw_stream_table_from_data(scratch.arena, msf_data);
              String8 info_data = msf_data_from_stream_number(scratch.arena, msf_data, st, PDB_FixedStream_Info);
              PDB_Info *info = pdb_info_from_data(scratch.arena, info_data);
              if(info != 0 && info_data.size >= sizeof(PDB_InfoHeader))
              {
                PDB_InfoHeader *info_header = (PDB_InfoHeader *)info_data.str;
                Guid guid = info->auth_guid;
                unique_identifier_string = str8f(arena, "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%u",
                                                 guid.data1,
                                                 guid.data2,
                                                 guid.data3,
                                                 guid.data4[0],
                                                 guid.data4[1],
                                                 guid.data4[2],
                                                 guid.data4[3],
                                                 guid.data4[4],
                                                 guid.data4[5],
                                                 guid.data4[6],
                                                 guid.data4[7],
                                                 info_header->age);
              }
              scratch_end(scratch);
            }
            
            // rjf: push record
            str8_list_pushf(arena, &p2b_shared->dump, "MODULE windows x86_64 %S %S\n", unique_identifier_string, module_name_string);
          }
          
          //- rjf: dump FILE records
          ProfScope("dump FILE records")
          {
            U64 chunk_idx = 0;
            for EachNode(n, RDIM_SrcFileChunkNode, bake_params->src_files.first)
            {
              Rng1U64 range = lane_range(n->count);
              for EachInRange(idx, range)
              {
                U64 file_idx = rdim_idx_from_src_file(&n->v[idx]);
                String8 src_path = n->v[idx].path;
                str8_list_pushf(arena, &p2b_shared->lane_chunk_file_dumps[lane_idx()*bake_params->src_files.chunk_count + chunk_idx], "FILE %I64u %S\n", file_idx, src_path);
              }
              chunk_idx += 1;
            }
          }
          
          //- rjf: dump FUNC records
          ProfScope("dump FUNC records")
          {
            U64 chunk_idx = 0;
            for EachNode(n, RDIM_SymbolChunkNode, bake_params->procedures.first)
            {
              String8List *out = &p2b_shared->lane_chunk_func_dumps[lane_idx()*bake_params->procedures.chunk_count + chunk_idx];
              Rng1U64 range = lane_range(n->count);
              for EachInRange(idx, range)
              {
                // NOTE(rjf): breakpad does not support multiple voff ranges per procedure.
                RDIM_Symbol *proc = &n->v[idx];
                RDIM_Scope *root_scope = proc->root_scope;
                if(root_scope != 0 && root_scope->voff_ranges.first != 0)
                {
                  // rjf: dump function record
                  RDIM_Rng1U64 voff_range = root_scope->voff_ranges.first->v;
                  str8_list_pushf(arena, out, "FUNC %I64x %I64x %I64x %S\n", voff_range.min, voff_range.max-voff_range.min, 0ull, proc->name);
                  
                  // rjf: dump function lines
                  U64 unit_idx = rdi_vmap_idx_from_voff(bake_results.unit_vmap.vmap.vmap, bake_results.unit_vmap.vmap.count, voff_range.min);
                  if(0 < unit_idx && unit_idx <= bake_results.units.units_count)
                  {
                    U32 line_table_idx = bake_results.units.units[unit_idx].line_table_idx;
                    if(0 < line_table_idx && line_table_idx <= bake_results.line_tables.line_tables_count)
                    {
                      // rjf: unpack unit line info
                      RDI_LineTable *line_table = &bake_results.line_tables.line_tables[line_table_idx];
                      RDI_ParsedLineTable line_info =
                      {
                        bake_results.line_tables.line_table_voffs + line_table->voffs_base_idx,
                        bake_results.line_tables.line_table_lines + line_table->lines_base_idx,
                        0,
                        line_table->lines_count,
                        0
                      };
                      for(U64 voff = voff_range.min, last_voff = 0;
                          voff < voff_range.max && voff > last_voff;)
                      {
                        RDI_U64 line_info_idx = rdi_line_info_idx_from_voff(&line_info, voff);
                        if(line_info_idx < line_info.count)
                        {
                          RDI_Line *line = &line_info.lines[line_info_idx];
                          U64 line_voff_min = line_info.voffs[line_info_idx];
                          U64 line_voff_opl = line_info.voffs[line_info_idx+1];
                          if(line->file_idx != 0)
                          {
                            str8_list_pushf(arena, out, "%I64x %I64x %I64u %I64u\n",
                                            line_voff_min,
                                            line_voff_opl-line_voff_min,
                                            (U64)line->line_num,
                                            (U64)line->file_idx);
                          }
                          last_voff = voff;
                          voff = line_voff_opl;
                        }
                        else
                        {
                          break;
                        }
                      }
                    }
                  }
                }
              }
              chunk_idx += 1;
            }
          }
          
          //- rjf: join
          lane_sync();
          if(lane_idx() == 0)
          {
            for EachIndex(chunk_idx, bake_params->src_files.chunk_count)
            {
              for EachIndex(ln_idx, lane_count())
              {
                str8_list_concat_in_place(&p2b_shared->dump, &p2b_shared->lane_chunk_file_dumps[ln_idx*bake_params->src_files.chunk_count + chunk_idx]);
              }
            }
            for EachIndex(chunk_idx, bake_params->procedures.chunk_count)
            {
              for EachIndex(ln_idx, lane_count())
              {
                str8_list_concat_in_place(&p2b_shared->dump, &p2b_shared->lane_chunk_func_dumps[ln_idx*bake_params->procedures.chunk_count + chunk_idx]);
              }
            }
          }
          lane_sync();
          output_blobs = p2b_shared->dump;
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
        if(lane_idx() == 0)
        {
          str8_list_pushf(arena, &output_blobs, "// %S (%S)\n\n", deterministic ? str8_skip_last_slash(f->path) : f->path, f->format ? rb_file_format_display_name_table[f->format] : str8_lit("Unsupported format"));
        }
        lane_sync();
        
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
            switch(rdi_status)
            {
              default:{}break;
              case RDI_ParseStatus_HeaderDoesNotMatch:      {log_user_errorf("RDI parse failure: header does not match\n");}break;
              case RDI_ParseStatus_UnsupportedVersionNumber:{log_user_errorf("RDI parse failure: unsupported version\n");}break;
              case RDI_ParseStatus_InvalidDataSecionLayout: {log_user_errorf("RDI parse failure: invalid data section layout\n");}break;
              case RDI_ParseStatus_Good:
              {
                String8List dump = rdi_dump_list_from_parsed(arena, &rdi, rdi_dump_subset_flags);
                if(lane_idx() == 0)
                {
                  str8_list_concat_in_place(&output_blobs, &dump);
                }
              }break;
            }
          }break;
        }
        
        //- rjf: dump file extension info
        if(f->format_flags & RB_FileFormatFlag_HasDWARF)
        {
          if(lane_idx() == 0)
          {
            str8_list_pushf(arena, &output_blobs, "// %S (%S) (DWARF)\n\n", deterministic ? str8_skip_last_slash(f->path) : f->path, f->format ? rb_file_format_display_name_table[f->format] : str8_lit("Unsupported format"));
          }
          lane_sync();
          {
            String8List dump = dw_dump_list_from_sections(arena, &dw, arch, dw_dump_subset_flags);
            if(lane_idx() == 0)
            {
              str8_list_concat_in_place(&output_blobs, &dump);
            }
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
      B32 is_written = os_write_data_list_to_file_path(output_path, output_blobs);
      if(is_written)
      {
        log_infof("Results written to %S", output_path);
      }
      else
      {
        log_user_errorf("ERROR: failed to write file %S\n", output_path);
      }
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
