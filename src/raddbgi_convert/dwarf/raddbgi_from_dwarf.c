// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "raddbgi_format/raddbgi_format.h"
#include "raddbgi_cons/raddbgi_cons_local.h"

#include "raddbgi_elf.h"
#include "raddbgi_dwarf.h"

#include "raddbgi_dwarf_stringize.h"

#include "raddbgi_from_dwarf.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "raddbgi_format/raddbgi_format.c"
#include "raddbgi_cons/raddbgi_cons_local.c"

#include "raddbgi_elf.c"
#include "raddbgi_dwarf.c"

#include "raddbgi_dwarf_stringize.c"

// TODO(allen): 
// [ ] need sample data for .debug_names

////////////////////////////////
//~ Program Parameters Parser

static DWARFCONV_Params*
dwarf_convert_params_from_cmd_line(Arena *arena, CmdLine *cmdline){
  DWARFCONV_Params *result = push_array(arena, DWARFCONV_Params, 1);
  result->unit_idx_max = ~0ull;
  
  // get input pdb
  {
    String8 input_name = cmd_line_string(cmdline, str8_lit("elf"));
    if (input_name.size == 0){
      str8_list_push(arena, &result->errors,
                     str8_lit("missing required parameter '--elf:<elf_file>'"));
    }
    
    if (input_name.size > 0){
      String8 input_data = os_data_from_file_path(arena, input_name);
      
      if (input_data.size == 0){
        str8_list_pushf(arena, &result->errors,
                        "could not load input file '%.*s'", str8_varg(input_name));
      }
      
      if (input_data.size != 0){
        result->input_elf_name = input_name;
        result->input_elf_data = input_data;
      }
    }
  }
  
  // get output name
  {
    result->output_name = cmd_line_string(cmdline, str8_lit("out"));
  }
  
  // error options
  if (cmd_line_has_flag(cmdline, str8_lit("hide_errors"))){
    String8List vals = cmd_line_strings(cmdline, str8_lit("hide_errors"));
    
    // if no values - set all to hidden
    if (vals.node_count == 0){
      B8 *ptr  = (B8*)&result->hide_errors;
      B8 *opl = ptr + sizeof(result->hide_errors);
      for (;ptr < opl; ptr += 1){
        *ptr = 1;
      }
    }
    
    // for each explicit value set the corresponding flag to hidden
    for (String8Node *node = vals.first;
         node != 0;
         node = node->next){
      if (str8_match(node->string, str8_lit("input"), 0)){
        result->hide_errors.input = 1;
      }
    }
    
  }
  
  // unit idx selector
  if (cmd_line_has_flag(cmdline, str8_lit("unit_idx"))){
    String8List vals = cmd_line_strings(cmdline, str8_lit("unit_idx"));
    
    // single value unit index
    if (vals.node_count == 1){
      U64 idx = u64_from_str8(vals.first->string, 10);
      result->unit_idx_min = idx;
      result->unit_idx_max = idx;
    }
    
    // range value unit index
    else if (vals.node_count >= 2){
      U64 idx_a = u64_from_str8(vals.first->string, 10);
      U64 idx_b = u64_from_str8(vals.first->next->string, 10);
      result->unit_idx_min = Min(idx_a, idx_b);
      result->unit_idx_max = Max(idx_a, idx_b);
    }
  }
  
  // dump options
  if (cmd_line_has_flag(cmdline, str8_lit("dump"))){
    result->dump = 1;
    
    String8List vals = cmd_line_strings(cmdline, str8_lit("dump"));
    if (vals.first == 0){
      B8 *ptr = &result->dump__first;
      for (; ptr < &result->dump__last; ptr += 1){
        *ptr = 1;
      }
    }
    else{
      for (String8Node *node = vals.first;
           node != 0;
           node = node->next){
        if (str8_match(node->string, str8_lit("header"), 0)){
          result->dump_header = 1;
        }
        else if (str8_match(node->string, str8_lit("sections"), 0)){
          result->dump_sections = 1;
        }
        else if (str8_match(node->string, str8_lit("segments"), 0)){
          result->dump_segments = 1;
        }
        else if (str8_match(node->string, str8_lit("symtab"), 0)){
          result->dump_symtab = 1;
        }
        else if (str8_match(node->string, str8_lit("dynsym"), 0)){
          result->dump_dynsym = 1;
        }
        else if (str8_match(node->string, str8_lit("debug_sections"), 0)){
          result->dump_debug_sections = 1;
        }
        else if (str8_match(node->string, str8_lit("debug_info"), 0)){
          result->dump_debug_info = 1;
        }
        else if (str8_match(node->string, str8_lit("debug_abbrev"), 0)){
          result->dump_debug_abbrev = 1;
        }
        else if (str8_match(node->string, str8_lit("debug_pubnames"), 0)){
          result->dump_debug_pubnames = 1;
        }
        else if (str8_match(node->string, str8_lit("debug_pubtypes"), 0)){
          result->dump_debug_pubtypes = 1;
        }
        else if (str8_match(node->string, str8_lit("debug_names"), 0)){
          result->dump_debug_names = 1;
        }
        else if (str8_match(node->string, str8_lit("debug_aranges"), 0)){
          result->dump_debug_aranges = 1;
        }
        else if (str8_match(node->string, str8_lit("debug_addr"), 0)){
          result->dump_debug_addr = 1;
        }
      }
    }
  }
  
  return(result);
}

////////////////////////////////
//~ Entry Point

static void
dump_symtab(Arena *arena, String8List *out, ELF_SymArray *symbols, String8 strtab,
            U32 indent){
  static char spaces[] = "                                ";
  
  U8 *str_first = strtab.str;
  U8 *str_opl   = strtab.str + strtab.size;
  
  ELF_Sym64 *symbol = symbols->symbols;
  U64 count = symbols->count;
  for (U64 i = 0; i < count; i += 1, symbol += 1){
    U8 *name_first = str_first + symbol->st_name;
    U8 *name_opl = name_first;
    for (;name_opl < str_opl && *name_opl != 0;) name_opl += 1;
    String8 name = str8_range(name_first, name_opl);
    
    ELF_SymbolBinding binding = ELF_SymBindingFromInfo(symbol->st_info);
    String8 binding_string = elf_string_from_symbol_binding(binding);
    
    ELF_SymbolType type = ELF_SymTypeFromInfo(symbol->st_info);
    String8 type_string = elf_string_from_symbol_type(type);
    
    ELF_SymbolVisibility vis = ELF_SymVisibilityFromOther(symbol->st_other);
    String8 vis_string = elf_string_from_symbol_visibility(vis);
    
    str8_list_pushf(arena, out,
                    "%.*ssymbol[%5llu] %6.*s %7.*s %9.*s 0x%08llx size=%-5llu sec=%-5u "
                    "%.*s\n",
                    indent, spaces, i,
                    str8_varg(binding_string), str8_varg(type_string),
                    str8_varg(vis_string),
                    symbol->st_value, symbol->st_size,
                    symbol->st_shndx, str8_varg(name));
  }
}

#if 0
static void
dump_entry_tree(Arena *arena, String8List *out,
                DWARF_Parsed *dwarf, DWARF_InfoUnit *unit,
                DWARF_InfoEntry *entry, U32 indent){
  static char spaces[] = "                                ";
  
  DWARF_AbbrevDecl *abbrev_decl = entry->abbrev_decl;
  
  // tag
  DWARF_Tag tag = abbrev_decl->tag;
  String8 tag_string = dwarf_string_from_tag(tag);
  str8_list_pushf(arena, out, "%.*sentry(@%llx) TAG %.*s\n",
                  indent, spaces, entry->info_offset, str8_varg(tag_string));
  
  // attributes
  U32                     attrib_count = abbrev_decl->attrib_count;
  DWARF_AbbrevAttribSpec *attrib_spec  = abbrev_decl->attrib_specs;
  DWARF_InfoAttribVal    *attrib_val   = entry->attrib_vals;
  for (U32 i = 0; i < attrib_count; i += 1, attrib_spec += 1, attrib_val += 1){
    // attribute name
    DWARF_AttributeName name = attrib_spec->name;
    String8 name_string = dwarf_string_from_attribute_name(name);
    str8_list_pushf(arena, out, "%.*sATTR %.*s ", indent + 4, spaces, str8_varg(name_string));
    
    // attribute value
    switch (attrib_spec->form){
      default:
      {
        String8 form_string = dwarf_string_from_attribute_form(attrib_spec->form);
        str8_list_pushf(arena, out, "<form: %.*s> {%llu, 0x%p}\n",
                        str8_varg(form_string), attrib_val->val, attrib_val->dataptr);
      }break;
      
      case DWARF_AttributeForm_strp:
      {
        String8 str = {0};
        
        String8 data = dwarf->debug_data[DWARF_SectionCode_Str];
        U64 off = attrib_val->val;
        if (off < data.size){
          U8 *start = data.str + off;
          U8 *opl = data.str + data.size;
          U8 *ptr = start;
          for (;ptr < opl && *ptr != 0;) ptr += 1;
          str = str8_range(start, ptr);
        }
        
        str8_list_pushf(arena, out, "'%.*s'\n", str8_varg(str));
      }break;
      
      case DWARF_AttributeForm_sec_offset:
      {
        DWARF_AttributeClassFlags attr_classes1 = dwarf_attribute_class_from_name(name);
        DWARF_AttributeClassFlags attr_classes2 = DWARF_AttributeClassFlag_sec_offset_classes;
        DWARF_AttributeClassFlags attr_classes = attr_classes1&attr_classes2;
        
        DWARF_SectionCode sec_code = DWARF_SectionCode_Null;
        if (unit->dwarf_version == 5){
          switch (attr_classes){
            case DWARF_AttributeClassFlag_addrptr: sec_code = DWARF_SectionCode_Addr; break;
            case DWARF_AttributeClassFlag_lineptr: sec_code = DWARF_SectionCode_Line; break;
            case DWARF_AttributeClassFlag_loclist: sec_code = DWARF_SectionCode_LocLists; break;
            case DWARF_AttributeClassFlag_loclistsptr: sec_code = DWARF_SectionCode_LocLists; break;
            case DWARF_AttributeClassFlag_macptr:  sec_code = DWARF_SectionCode_Macro; break;
            case DWARF_AttributeClassFlag_rnglist: sec_code = DWARF_SectionCode_RngLists; break;
            case DWARF_AttributeClassFlag_rnglistsptr: sec_code = DWARF_SectionCode_RngLists; break;
            case DWARF_AttributeClassFlag_stroffsetsptr: sec_code = DWARF_SectionCode_StrOffsets; break;
          }
        }
        else if (unit->dwarf_version == 4){
          switch (attr_classes){
            case DWARF_AttributeClassFlag_lineptr: sec_code = DWARF_SectionCode_Line; break;
            case DWARF_AttributeClassFlag_loclist: sec_code = DWARF_SectionCode_Loc; break;
            case DWARF_AttributeClassFlag_macptr:  sec_code = DWARF_SectionCode_MacInfo; break;
            case DWARF_AttributeClassFlag_rnglist: sec_code = DWARF_SectionCode_Ranges; break;
          }
        }
        
        String8 sec_name = dwarf_name_from_debug_section(dwarf, sec_code);
        str8_list_pushf(arena, out, "sec(%.*s) + %llu\n", str8_varg(sec_name), attrib_val->val);
      }break;
      
      case DWARF_AttributeForm_ref1:
      case DWARF_AttributeForm_ref2:
      case DWARF_AttributeForm_ref4:
      case DWARF_AttributeForm_ref8:
      case DWARF_AttributeForm_ref_udata:
      {
        str8_list_pushf(arena, out, "entry(@%llx)\n", attrib_val->val);
      }break;
      
      case DWARF_AttributeForm_addr:
      {
        str8_list_pushf(arena, out, "0x%llx\n", attrib_val->val);
      }break;
      
      case DWARF_AttributeForm_exprloc:
      {
        str8_list_pushf(arena, out, "expression\n");
        // TODO(allen): dwarf expression dumping
      }break;
      
      case DWARF_AttributeForm_strx1:
      case DWARF_AttributeForm_strx2:
      case DWARF_AttributeForm_strx3:
      case DWARF_AttributeForm_strx4:
      {
        String8 str = {0};
        
        U32 idx = attrib_val->val;
        U64 str_offsets_off = unit->str_offsets_base + idx*unit->offset_size;
        
        String8 str_offsets = dwarf->debug_data[DWARF_SectionCode_StrOffsets];
        if (str_offsets_off + unit->offset_size < str_offsets.size){
          U64 off = 0;
          MemoryCopy(&off, str_offsets.str + str_offsets_off, unit->offset_size);
          
          String8 data = dwarf->debug_data[DWARF_SectionCode_Str];
          if (off < data.size){
            U8 *start = data.str + off;
            U8 *opl = data.str + data.size;
            U8 *ptr = start;
            for (;ptr < opl && *ptr != 0;) ptr += 1;
            str = str8_range(start, ptr);
          }
        }
        
        str8_list_pushf(arena, out, "'%.*s'\n", str8_varg(str));
      }break;
      
      case DWARF_AttributeForm_addrx:
      case DWARF_AttributeForm_addrx1:
      case DWARF_AttributeForm_addrx2:
      case DWARF_AttributeForm_addrx3:
      case DWARF_AttributeForm_addrx4:
      {
        U64 address = 0;
        
        U32 idx = attrib_val->val;
        U64 address_off = unit->addr_base + idx*unit->address_size;
        
        String8 data = dwarf->debug_data[DWARF_SectionCode_Addr];
        if (address_off + unit->address_size < data.size){
          MemoryCopy(&address, data.str + address_off, unit->address_size);
        }
        
        str8_list_pushf(arena, out, "0x%x\n", address);
      }break;
      
      case DWARF_AttributeForm_rnglistx:
      {
        U64 rnglist_off = unit->rnglists_base + attrib_val->val;
        int x = 0;
      }break;
      
      case DWARF_AttributeForm_data1:
      case DWARF_AttributeForm_data2:
      case DWARF_AttributeForm_data4:
      case DWARF_AttributeForm_data8:
      case DWARF_AttributeForm_data16:
      case DWARF_AttributeForm_udata:
      case DWARF_AttributeForm_implicit_const:
      case DWARF_AttributeForm_flag:
      case DWARF_AttributeForm_flag_present:
      {
        str8_list_pushf(arena, out, "%llu\n", attrib_val->val);
      }break;
      
      case DWARF_AttributeForm_sdata:
      {
        str8_list_pushf(arena, out, "%lld\n", (S64)attrib_val->val);
      }break;
      
      case DWARF_AttributeForm_string:
      {
        str8_list_pushf(arena, out, "'%.*s'\n", (int)attrib_val->val, attrib_val->dataptr);
      }break;
    }
  }
  
  // dump children
  for (DWARF_InfoEntry *child = entry->first_child;
       child != 0;
       child = child->next_sibling){
    dump_entry_tree(arena, out, dwarf, unit, child, indent + 1);
  }
}
#endif

int
main(int argc, char **argv){
  local_persist TCTX main_thread_tctx = {0};
  tctx_init_and_equip(&main_thread_tctx);
  
#if PROFILE_TELEMETRY
  U64 tm_data_size = GB(1);
  U8 *tm_data = os_reserve(tm_data_size);
  os_commit(tm_data, tm_data_size);
  tmLoadLibrary(TM_RELEASE);
  tmSetMaxThreadCount(1024);
  tmInitialize(tm_data_size, tm_data);
#endif
  
  ThreadName("[main]");
  
  Arena *arena = arena_alloc();
  String8List args = os_string_list_from_argcv(arena, argc, argv);
  CmdLine cmdline = cmd_line_from_string_list(arena, args);
  
  ProfBeginCapture("raddbgi_from_dwarf");
  
  // parse arguments
  DWARFCONV_Params *params = dwarf_convert_params_from_cmd_line(arena, &cmdline);
  
  // show input errors
  if (params->errors.node_count > 0 &&
      !params->hide_errors.input){
    for (String8Node *node = params->errors.first;
         node != 0;
         node = node->next){
      fprintf(stdout, "error(input): %.*s\n", str8_varg(node->string));
    }
  }
  
  // will we try to parse an input file?
  B32 try_parse_input = (params->errors.node_count == 0);
  
  // track parse success
  B32 successful_parse = 1;
  
#define PARSE_CHECK_ERROR(p,fmt,...) do{ if ((p) == 0){ \
successful_parse = 0; \
fprintf(stdout, "error(parsing): " fmt "\n", __VA_ARGS__); \
} }while(0)
  
  // parse elf
  ELF_Parsed *elf = 0;
  if (try_parse_input) ProfScope("parse elf"){
    elf = elf_parsed_from_data(arena, params->input_elf_data);
    PARSE_CHECK_ERROR(elf, "ELF");
  }
  
  // parse strtab
  String8 strtab = {0};
  if (elf != 0) ProfScope("parse strtab"){
    strtab = elf_section_data_from_idx(elf, elf->strtab_idx);
  }
  
  // parse symtab
  ELF_SymArray symtab = {0};
  if (elf != 0) ProfScope("parse symtab"){
    String8 data = elf_section_data_from_idx(elf, elf->symtab_idx);
    symtab = elf_sym_array_from_data(arena, elf->elf_class, data);
  }
  
  // parse dynsym
  ELF_SymArray dynsym = {0};
  if (elf != 0) ProfScope("parse dynsym"){
    String8 data = elf_section_data_from_idx(elf, elf->dynsym_idx);
    dynsym = elf_sym_array_from_data(arena, elf->elf_class, data);
  }
  
  // parse dwarf
  DWARF_Parsed *dwarf = 0;
  if (elf != 0) ProfScope("parse dwarf"){
    dwarf = dwarf_parsed_from_elf(arena, elf);
    PARSE_CHECK_ERROR(dwarf, "DWARF");
  }
  
  // parse info
  DWARF_InfoParsed *info = 0;
  if (dwarf != 0){
    String8 data = dwarf->debug_data[DWARF_SectionCode_Info];
    if (data.size > 0) ProfScope("parse .debug_info"){
      info = dwarf_info_from_data(arena, data);
      PARSE_CHECK_ERROR(info, "DEBUG INFO");
    }
  }
  
  // parse pubnames
  DWARF_PubNamesParsed *pubnames = 0;
  if (dwarf != 0){
    String8 data = dwarf->debug_data[DWARF_SectionCode_PubNames];
    if (data.size) ProfScope("parse .debug_pubnames"){
      pubnames = dwarf_pubnames_from_data(arena, data);
      PARSE_CHECK_ERROR(pubnames, "DEBUG PUBNAMES");
    }
  }
  
  // parse pubtypes
  DWARF_PubNamesParsed *pubtypes = 0;
  if (dwarf != 0){
    String8 data = dwarf->debug_data[DWARF_SectionCode_PubTypes];
    if (data.size) ProfScope("parse .debug_pubtypes"){
      pubtypes = dwarf_pubnames_from_data(arena, data);
      PARSE_CHECK_ERROR(pubtypes, "DEBUG PUBTYPES");
    }
  }
  
  // parse names
  DWARF_NamesParsed *names = 0;
  if (dwarf != 0){
    String8 data = dwarf->debug_data[DWARF_SectionCode_Names];
    if (data.size) ProfScope("parse .debug_names"){
      names = dwarf_names_from_data(arena, data);
      PARSE_CHECK_ERROR(names, "DEBUG NAMES");
    }
  }
  
  // parse aranges
  DWARF_ArangesParsed *aranges = 0;
  if (dwarf != 0){
    String8 data = dwarf->debug_data[DWARF_SectionCode_Aranges];
    if (data.size) ProfScope("parse .debug_aranges"){
      aranges = dwarf_aranges_from_data(arena, data);
      PARSE_CHECK_ERROR(aranges, "DEBUG ARANGES");
    }
  }
  
  // parse addr
  DWARF_AddrParsed *addr = 0;
  if (dwarf != 0){
    String8 data = dwarf->debug_data[DWARF_SectionCode_Addr];
    if (data.size) ProfScope("parse .debug_addr"){
      addr = dwarf_addr_from_data(arena, data);
      PARSE_CHECK_ERROR(addr, "DEBUG ADDR");
    }
  }
  
#if 0
  // parse abbrev
  DWARF_AbbrevParsed *abbrev = 0;
  if (dwarf != 0){
    String8 data = dwarf->debug_data[DWARF_SectionCode_Abbrev];
    if (data.size > 0) ProfScope("parse .debug_abbrev"){
      DWARF_AbbrevParams abbrev_params = {0};
      abbrev_params.unit_idx_min = params->unit_idx_min;
      abbrev_params.unit_idx_max = params->unit_idx_max;
      abbrev = dwarf_abbrev_from_data(arena, data, &abbrev_params);
      PARSE_CHECK_ERROR(abbrev, "DEBUG ABBREV");
    }
  }
  
  // parse info
  DWARF_InfoParsed *info = 0;
  if (abbrev != 0){
    String8 data = dwarf->debug_data[DWARF_SectionCode_Info];
    if (data.size > 0) ProfScope("parse .debug_info"){
      DWARF_InfoParams info_params = {0};
      info_params.unit_idx_min = params->unit_idx_min;
      info_params.unit_idx_max = params->unit_idx_max;
      info = dwarf_info_from_data(arena, data, &info_params, abbrev);
      PARSE_CHECK_ERROR(info, "DEBUG INFO");
    }
  }
#endif
  
  // dump
  if (params->dump) ProfScope("dump"){
    String8List dump = {0};
    
    // ELF
    if (params->dump_header){
      if (elf != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "ELF:\n"));
        
        // TODO: better stringizers for fields here
        str8_list_pushf(arena, &dump, " elf_class=%u\n", elf->elf_class);
        str8_list_pushf(arena, &dump, " arch=%u\n", elf->arch);
        str8_list_pushf(arena, &dump, " section_count=%llu\n", elf->section_count);
        str8_list_pushf(arena, &dump, " segment_count=%llu\n", elf->segment_count);
        str8_list_pushf(arena, &dump, " vbase=0x%llx\n", elf->vbase);
        str8_list_pushf(arena, &dump, " entry_vaddr=0x%llx\n", elf->vbase);
        
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // SECTIONS
    if (params->dump_sections){
      if (elf != 0){
        ELF_SectionArray section_array = elf_section_array_from_elf(elf);
        String8Array section_name_array = elf_section_name_array_from_elf(elf);
        
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "SECTIONS:\n"));
        
        ELF_Shdr64 *sec = section_array.sections;
        String8 *sec_name = section_name_array.strings;
        U64 count = section_array.count;
        for (U64 i = 0 ; i < count; i += 1, sec += 1, sec_name += 1){
          String8 type_string = elf_string_from_section_type(sec->sh_type);
          
          // TODO: better stringizers for fields here
          str8_list_pushf(arena, &dump, " section[%llu]:\n", i);
          str8_list_pushf(arena, &dump, "  name='%.*s'\n", str8_varg(*sec_name));
          str8_list_pushf(arena, &dump, "  type=%.*s\n", str8_varg(type_string));
          str8_list_pushf(arena, &dump, "  flags=0x%llx\n", sec->sh_flags);
          str8_list_pushf(arena, &dump, "  addr=0x%llx\n", sec->sh_addr);
          str8_list_pushf(arena, &dump, "  offset=0x%llx\n", sec->sh_offset);
          str8_list_pushf(arena, &dump, "  size=%llu\n", sec->sh_size);
          str8_list_pushf(arena, &dump, "  link=%u\n", sec->sh_link);
          str8_list_pushf(arena, &dump, "  info=%u\n", sec->sh_info);
          str8_list_pushf(arena, &dump, "  addralign=0x%llx\n", sec->sh_addralign);
          str8_list_pushf(arena, &dump, "  entsize=%llu\n", sec->sh_entsize);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
      }
    }
    
    // SYMTAB
    if (symtab.count > 0 && params->dump_symtab){
      str8_list_push(arena, &dump,
                     str8_lit("################################"
                              "################################\n"
                              "SYMTAB:\n"));
      str8_list_pushf(arena, &dump, " section: %llu\n", elf->symtab_idx);
      dump_symtab(arena, &dump, &symtab, strtab, 1);
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // DYNSYM
    if (dynsym.count > 0 && params->dump_dynsym){
      str8_list_push(arena, &dump,
                     str8_lit("################################"
                              "################################\n"
                              "DYNSYM:\n"));
      str8_list_pushf(arena, &dump, " section: %llu\n", elf->dynsym_idx);
      dump_symtab(arena, &dump, &dynsym, strtab, 1);
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // SEGMENTS
    if (params->dump_segments){
      if (elf != 0){
        ELF_SegmentArray segment_array = elf_segment_array_from_elf(elf);
        
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "SEGMENTS:\n"));
        
        ELF_Phdr64 *segments = segment_array.segments;
        U64 count = segment_array.count;
        for (U64 i = 0 ; i < count; i += 1){
          ELF_Phdr64 *seg = segments + i;
          
          // TODO: better stringizers for fields here
          str8_list_pushf(arena, &dump, " segment[%llu]:\n", i);
          str8_list_pushf(arena, &dump, "  p_type=%u\n", seg->p_type);
          str8_list_pushf(arena, &dump, "  p_flags=0x%x\n", seg->p_flags);
          str8_list_pushf(arena, &dump, "  p_offset=0x%llx\n", seg->p_offset);
          str8_list_pushf(arena, &dump, "  p_vaddr=0x%llx\n", seg->p_vaddr);
          str8_list_pushf(arena, &dump, "  p_paddr=0x%llx\n", seg->p_paddr);
          str8_list_pushf(arena, &dump, "  p_filesz=%llu\n", seg->p_filesz);
          str8_list_pushf(arena, &dump, "  p_memsz=%llu\n", seg->p_memsz);
          str8_list_pushf(arena, &dump, "  p_align=%llu\n", seg->p_align);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
      }
    }
    
    // DEBUG SECTIONS
    if (params->dump_debug_sections){
      if (dwarf != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DEBUG SECTIONS:\n"));
        
        U32 *debug_section_idx = dwarf->debug_section_idx;
        String8 *debug_data = dwarf->debug_data;
        for (U32 i = 1; i < DWARF_SectionCode_COUNT; i += 1, debug_data += 1){
          U32 idx = debug_section_idx[i];
          String8 name = dwarf_string_from_section_code(i);
          str8_list_pushf(arena, &dump, " %-10.*s section_idx=%u\n", str8_varg(name), idx);
        }
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // DEBUG INFO
    if (params->dump_debug_info){
      if (info != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DEBUG INFO:\n"));
        
        U32 i = 0;
        for (DWARF_InfoUnit *unit = info->unit_first;
             unit != 0;
             unit = unit->next, i += 1){
          str8_list_pushf(arena, &dump, " unit[%u]:\n", i);
          dwarf_stringize_info(arena, &dump, unit, 2);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
        
      }
    }
    
    // DEBUG PUBNAMES
    if (params->dump_debug_pubnames){
      if (pubnames != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DEBUG PUBNAMES:\n"));
        
        U32 i = 0;
        for (DWARF_PubNamesUnit *unit = pubnames->unit_first;
             unit != 0;
             unit = unit->next, i += 1){
          str8_list_pushf(arena, &dump, " unit[%u]:\n", i);
          dwarf_stringize_pubnames(arena, &dump, unit, 2);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
        
      }
    }
    
    // DEBUG PUBTYPES
    if (params->dump_debug_pubtypes){
      if (pubtypes != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DEBUG PUBTYPES:\n"));
        
        U32 i = 0;
        for (DWARF_PubNamesUnit *unit = pubtypes->unit_first;
             unit != 0;
             unit = unit->next, i += 1){
          str8_list_pushf(arena, &dump, " unit[%u]:\n", i);
          dwarf_stringize_pubnames(arena, &dump, unit, 2);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
        
      }
    }
    
    // DEBUG NAMES
    if (params->dump_debug_names){
      if (names != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DEBUG NAMES:\n"));
        
        U32 i = 0;
        for (DWARF_NamesUnit *unit = names->unit_first;
             unit != 0;
             unit = unit->next, i += 1){
          str8_list_pushf(arena, &dump, " unit[%u]:\n", i);
          dwarf_stringize_names(arena, &dump, unit, 2);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
        
      }
    }
    
    // DEBUG ARANGES
    if (params->dump_debug_aranges){
      if (aranges != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DEBUG ARANGES:\n"));
        
        U32 i = 0;
        for (DWARF_ArangesUnit *unit = aranges->unit_first;
             unit != 0;
             unit = unit->next, i += 1){
          str8_list_pushf(arena, &dump, " unit[%u]:\n", i);
          dwarf_stringize_aranges(arena, &dump, unit, 2);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
        
      }
    }
    
    // DEBUG ADDR
    if (params->dump_debug_addr){
      if (addr != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DEBUG ADDR:\n"));
        
        U32 i = 0;
        for (DWARF_AddrUnit *unit = addr->unit_first;
             unit != 0;
             unit = unit->next, i += 1){
          str8_list_pushf(arena, &dump, " unit[%u]:\n", i);
          dwarf_stringize_addr(arena, &dump, unit, 2);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
        
      }
    }
    
#if 0
    // DEBUG ABBREV
    if (params->dump_debug_abbrev){
      if (abbrev != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DEBUG ABBREV:\n"));
        
        U32 i = 0;
        for (DWARF_AbbrevUnit *unit = abbrev->unit_first;
             unit != 0;
             unit = unit->next, i += 1){
          U32 j = 0;
          for (DWARF_AbbrevDecl *abbrev_decl = unit->first;
               abbrev_decl != 0;
               abbrev_decl = abbrev_decl->next, j += 1){
            String8 tag_string = dwarf_string_from_tag(abbrev_decl->tag);
            
            str8_list_pushf(arena, &dump, " unit[%u],abbrev[%u]:\n", i, j);
            str8_list_pushf(arena, &dump, "  code=%llu\n", abbrev_decl->abbrev_code);
            str8_list_pushf(arena, &dump, "  tag=%.*s\n", str8_varg(tag_string));
            str8_list_pushf(arena, &dump, "  has_children=%u\n", abbrev_decl->has_children);
            str8_list_pushf(arena, &dump, "  attrib_count=%u\n", abbrev_decl->attrib_count);
            str8_list_pushf(arena, &dump, "  attribs:\n", abbrev_decl->attrib_count);
            
            U32 attrib_count = abbrev_decl->attrib_count;
            DWARF_AbbrevAttribSpec *attrib_spec = abbrev_decl->attrib_specs;
            for (U32 k = 0; k < attrib_count; k += 1, attrib_spec += 1){
              String8 name_string = dwarf_string_from_attribute_name(attrib_spec->name);
              String8 form_string = dwarf_string_from_attribute_form(attrib_spec->form);
              
              str8_list_pushf(arena, &dump, "   [%-14.*s %-10.*s]\n", 
                              str8_varg(name_string), str8_varg(form_string));
            }
            
            str8_list_push(arena, &dump, str8_lit("\n"));
          }
        }
        
      }
    }
#endif
    
#if 0
    // DEBUG INFO
    if (params->dump_debug_info){
      if (info != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DEBUG INFO:\n"));
        
        U32 i = 0;
        for (DWARF_InfoUnit *unit = info->unit_first;
             unit != 0;
             unit = unit->next, i += 1){
          str8_list_pushf(arena, &dump, " unit[%u]:\n", i);
          str8_list_pushf(arena, &dump, "  [header]\n");
          str8_list_pushf(arena, &dump, "  version=%u\n", unit->dwarf_version);
          str8_list_pushf(arena, &dump, "  offset_size=%u\n", unit->offset_size);
          str8_list_pushf(arena, &dump, "  address_size=%u\n", unit->address_size);
          str8_list_pushf(arena, &dump, "  [extracted attributes]\n");
          str8_list_pushf(arena, &dump, "  langauge=%u\n", (U32)unit->language);
          str8_list_pushf(arena, &dump, "  line_info_offset=%llu\n", unit->line_info_offset);
          str8_list_pushf(arena, &dump, "  vbase=0x%llx\n", unit->vbase);
          str8_list_pushf(arena, &dump, "  str_offsets_base=%llu\n", unit->str_offsets_base);
          str8_list_pushf(arena, &dump, "  addr_base=%llu\n", unit->addr_base);
          str8_list_pushf(arena, &dump, "  rnglists_base=%llu\n", unit->rnglists_base);
          str8_list_pushf(arena, &dump, "  loclists_base=%llu\n", unit->loclists_base);
          dump_entry_tree(arena, &dump, dwarf, unit, unit->entry_root, 2);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
        
      }
    }
#endif
    
    // print dump
    for (String8Node *node = dump.first;
         node != 0;
         node = node->next){
      fwrite(node->string.str, 1, node->string.size, stdout);
    }
  }
  
  ProfEndCapture();
  return(0);
}
