// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "radbin/generated/radbin.meta.c"

////////////////////////////////
//~ rjf: Top-Level Entry Point

internal void
rb_entry_point(CmdLine *cmdline)
{
  Arena *arena = arena_alloc();
  ASYNC_Root *async_root = async_root_alloc();
  Log *log = log_alloc();
  log_select(log);
  log_scope_begin();
  
  //////////////////////////////
  //- rjf: analyze command line input files
  //
  typedef struct File File;
  struct File
  {
    File *next;
    RB_FileFormat format;
    String8 path;
  };
  File *first_input_file = 0;
  File *last_input_file = 0;
  for(String8Node *n = cmdline->inputs.first; n != 0; n = n->next)
  {
    OS_Handle file = os_file_open(OS_AccessFlag_Read, n->string);
    RB_FileFormat file_format = RB_FileFormat_Null;
    
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
      FileProperties props = os_properties_from_file(file);
      
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
        file_format = RB_FileFormat_ELF;
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
    
    //- rjf: log file recognition
    if(file_format != RB_FileFormat_Null)
    {
      log_infof("%S recognized as %S\n", n->string, rb_file_format_display_name_table[file_format]);
    }
    else
    {
      log_infof("%S was not recognized as a supported format.\n", n->string);
    }
    
    //- rjf: push to list
    File *file_n = push_array(arena, File, 1);
    file_n->path = n->string;
    file_n->format = file_format;
    SLLQueuePush(first_input_file, last_input_file, file_n);
    
    os_file_close(file);
  }
  
  //////////////////////////////
  //- rjf: bucket input files by format
  //
  String8List input_paths_from_format_table[RB_FileFormat_COUNT] = {0};
  for(File *f = first_input_file; f != 0; f = f->next)
  {
    str8_list_push(arena, &input_paths_from_format_table[f->format], f->path);
  }
  
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
    else if(str8_match(str8_skip_last_dot(output_path), str8_lit("rdi"), StringMatchFlag_CaseInsensitive))
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
  if(output_kind == OutputKind_Null || cmdline->inputs.node_count == 0)
  {
    fprintf(stderr, "-------------------------------------------------------------------------------\n");
    fprintf(stderr, "%s\n\n", BUILD_TITLE);
    if(output_kind != OutputKind_Null)
    {
      fprintf(stderr, "%.*s Help\n", str8_varg(output_kind_info[output_kind].title));
      fprintf(stderr, "To see top-level options for radbin, run the binary with no arguments.\n\n");
    }
  }
  
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
    {
      fprintf(stderr, "This utility provides a number of operations relating to executable image and\n");
      fprintf(stderr, "debug information formats. It can convert debug information to the RAD Debug\n");
      fprintf(stderr, "Info (.rdi) format. It can also parse and dump textualized contents of several\n");
      fprintf(stderr, "formats, including PDB, PE, ELF, and RDI. It also supports converting debug\n");
      fprintf(stderr, "information to the textual Breakpad format.\n\n");
      
      fprintf(stderr, "The following top-level arguments are accepted:\n\n");
      
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
    //- rjf: RDI -> conversion based on inputs
    //
    case OutputKind_RDI:
    {
      //- rjf: unpack subset flags
      RDIM_SubsetFlags subset_flags = 0xffffffff;
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
      }
      
      //- rjf: no inputs => help
      if(cmdline->inputs.node_count == 0)
      {
        fprintf(stderr, "The following arguments are accepted:\n\n");
        
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
        
        fprintf(stderr, "RAD Debug Info Subsets:\n");
        fprintf(stderr, " - binary_sections                Sections in the executable image\n");
        fprintf(stderr, " - units                          Compilation unit info\n");
        fprintf(stderr, " - procedures                     Procedure info\n");
        fprintf(stderr, " - global_variables               Global variable info\n");
        fprintf(stderr, " - thread_variables               Thread-local variable info\n");
        fprintf(stderr, " - scopes                         Scope info\n");
        fprintf(stderr, " - locals                         Local variable info\n");
        fprintf(stderr, " - types                          Type nodes\n");
        fprintf(stderr, " - udts                           User-defined-type (UDT) info\n");
        fprintf(stderr, " - line_info                      Source code line <-> virtual offset mapping\n");
        fprintf(stderr, " - global_variable_name_map       The name -> global variable table\n");
        fprintf(stderr, " - thread_variable_name_map       The name -> thread variable table\n");
        fprintf(stderr, " - procedure_name_map             The name -> procedure table\n");
        fprintf(stderr, " - constant_name_map              The name -> constant table\n");
        fprintf(stderr, " - type_name_map                  The name -> user-defined-type table\n");
        fprintf(stderr, " - link_name_procedure_name_map   The link_name -> procedure table\n");
        fprintf(stderr, " - normal_source_path_name_map    The path -> source file table\n");
      }
      
      //- rjf: PDB inputs => PDB -> RDI conversion
      else if(input_paths_from_format_table[RB_FileFormat_PDB].node_count != 0)
      {
        log_infof("PDBs specified; producing RDI by converting PDB data\n");
        
        // rjf: convert
        P2R_ConvertParams convert_params = {0};
        {
          convert_params.input_pdb_name = str8_list_first(&input_paths_from_format_table[RB_FileFormat_PDB]);
          convert_params.input_exe_name = str8_list_first(&input_paths_from_format_table[RB_FileFormat_PE]);
          convert_params.input_pdb_data = os_data_from_file_path(arena, convert_params.input_pdb_name);
          convert_params.input_exe_data = os_data_from_file_path(arena, convert_params.input_exe_name);
          convert_params.subset_flags   = subset_flags;
          convert_params.deterministic  = cmd_line_has_flag(cmdline, str8_lit("deterministic"));
        }
        RDIM_BakeParams bake_params = {0};
        ProfScope("convert") bake_params = p2r_convert(arena, async_root, &convert_params);
        
        // rjf: bake
        RDIM_BakeResults bake_results = {0};
        ProfScope("bake") bake_results = rdim_bake(arena, async_root, &bake_params);
        
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
        
        // rjf: no output path? -> pick one based on PDB
        if(output_path.size == 0)
        {
          output_path = push_str8f(arena, "%S.rdi", str8_chop_last_dot(convert_params.input_pdb_name));
        }
      }
      
      //- rjf: no viable input paths
      else
      {
        log_user_errorf("Could not create an RDI file from the specified inputs. You must provide either a valid PDB file or an executable image (PE, ELF) file with DWARF information.");
      }
    }break;
    
    ////////////////////////////
    //- rjf: dump -> textual dump of inputs
    //
    case OutputKind_Dump:
    {
      //- rjf: no inputs => help
      if(cmdline->inputs.node_count == 0)
      {
        fprintf(stderr, "All input files specified on the command line will be dumped. The following\n");
        fprintf(stderr, "formats are currently supported: PE, COFF, RDI, and ELF\n\n");
      }
      
      //- rjf: dump input files in order
      String8List dump = {0};
      for(File *f = first_input_file; f != 0; f = f->next)
      {
        
      }
      
      //- rjf: join with output
      str8_list_concat_in_place(&output_blobs, &dump);
    }break;
    
    ////////////////////////////
    //- rjf: breakpad -> conversion based on inputs
    //
    case OutputKind_Breakpad:
    {
      //- rjf: no inputs => help
      if(cmdline->inputs.node_count == 0)
      {
        fprintf(stderr, "Pass a path to a PDB file, and optionally its associated PE file, for which\n");
        fprintf(stderr, "a Breakpad file should be generated.\n");
      }
      
      //- rjf: PDB inputs => PDB -> Breakpad conversion
      else if(input_paths_from_format_table[RB_FileFormat_PDB].node_count != 0)
      {
        log_infof("PDBs specified; producing Breakpad by converting PDB data\n");
        
        // rjf: convert
        P2R_ConvertParams convert_params = {0};
        {
          convert_params.input_pdb_name = str8_list_first(&input_paths_from_format_table[RB_FileFormat_PDB]);
          convert_params.input_exe_name = str8_list_first(&input_paths_from_format_table[RB_FileFormat_PE]);
          convert_params.input_pdb_data = os_data_from_file_path(arena, convert_params.input_pdb_name);
          convert_params.input_exe_data = os_data_from_file_path(arena, convert_params.input_exe_name);
          convert_params.subset_flags   = RDIM_SubsetFlag_All & ~(RDIM_SubsetFlag_Types|RDIM_SubsetFlag_UDTs);
          convert_params.deterministic  = cmd_line_has_flag(cmdline, str8_lit("deterministic"));
        }
        RDIM_BakeParams bake_params = {0};
        ProfScope("convert") bake_params = p2r_convert(arena, async_root, &convert_params);
        
        //- rjf: produce breakpad text
        String8List dump = {0};
        ProfScope("dump breakpad text")
        {
          p2b_async_root = async_root;
          RDIM_BakeParams *params = &bake_params;
          
          //- rjf: kick off unit vmap baking
          P2B_BakeUnitVMapIn bake_unit_vmap_in = {&params->units};
          ASYNC_Task *bake_unit_vmap_task = async_task_launch(arena, p2b_bake_unit_vmap_work, .input = &bake_unit_vmap_in);
          
          //- rjf: kick off line-table baking
          P2B_BakeLineTablesIn bake_line_tables_in = {&params->line_tables};
          ASYNC_Task *bake_line_tables_task = async_task_launch(arena, p2b_bake_line_table_work, .input = &bake_line_tables_in);
          
          //- rjf: build unit -> line table idx array
          U64 unit_count = params->units.total_count;
          U32 *unit_line_table_idxs = push_array(arena, U32, unit_count+1);
          {
            U64 dst_idx = 1;
            for(RDIM_UnitChunkNode *n = params->units.first; n != 0; n = n->next)
            {
              for(U64 n_idx = 0; n_idx < n->count; n_idx += 1, dst_idx += 1)
              {
                unit_line_table_idxs[dst_idx] = rdim_idx_from_line_table(n->v[n_idx].line_table);
              }
            }
          }
          
          //- rjf: dump MODULE record
          str8_list_pushf(arena, &dump, "MODULE windows x86_64 %I64x %S\n", params->top_level_info.exe_hash, params->top_level_info.exe_name);
          
          //- rjf: dump FILE records
          ProfScope("dump FILE records")
          {
            for(RDIM_SrcFileChunkNode *n = params->src_files.first; n != 0; n = n->next)
            {
              for(U64 idx = 0; idx < n->count; idx += 1)
              {
                U64 file_idx = rdim_idx_from_src_file(&n->v[idx]);
                String8 src_path = n->v[idx].normal_full_path;
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
          P2B_DumpProcChunkIn *dump_proc_chunk_in = push_array(arena, P2B_DumpProcChunkIn, params->procedures.chunk_count);
          ASYNC_Task **dump_proc_chunk_tasks = push_array(arena, ASYNC_Task *, params->procedures.chunk_count);
          ProfScope("kick off FUNC & line record dump tasks")
          {
            U64 task_idx = 0;
            for(RDIM_SymbolChunkNode *n = params->procedures.first; n != 0; n = n->next, task_idx += 1)
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
            for(U64 idx = 0; idx < params->procedures.chunk_count; idx += 1)
            {
              String8List *out = async_task_join_struct(dump_proc_chunk_tasks[idx], String8List);
              str8_list_concat_in_place(&dump, out);
            }
          }
        }
        
        //- rjf: join with out
        str8_list_concat_in_place(&output_blobs, &dump);
        
        // rjf: no output path? -> pick one based on PDB
        if(output_path.size == 0)
        {
          output_path = push_str8f(arena, "%S.psyms", str8_chop_last_dot(convert_params.input_pdb_name));
        }
      }
      
      //- rjf: no viable input paths
      else
      {
        log_user_errorf("Could not create a Breakpad file from the specified inputs. You must provide either a valid PDB file or an executable image (PE, ELF) file with DWARF information.");
      }
    }break;
  }
  
  //////////////////////////////
  //- rjf: write info & errors
  //
  LogScopeResult log_scope = log_scope_end(arena);
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
  
  //////////////////////////////
  //- rjf: write outputs
  //
  if(output_path.size != 0) ProfScope("write outputs [file]")
  {
    OS_Handle output_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, output_path);
    U64 off = 0;
    for(String8Node *n = output_blobs.first; n != 0; n = n->next)
    {
      os_file_write(output_file, r1u64(off, off+n->string.size), n->string.str);
      off += n->string.size;
    }
    os_file_close(output_file);
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
  }
}
