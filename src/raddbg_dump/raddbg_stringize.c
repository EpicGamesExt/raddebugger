// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ RADDBG Common Stringize Functions

static String8
raddbg_string_from_data_section_tag(RADDBG_DataSectionTag tag){
  String8 result = {0};
  switch (tag){
#define X(N,C) case C: result = str8_lit(#N); break;
#define Y(N)
    RADDBG_DataSectionTagXList(X,Y)
#undef X
#undef Y
  }
  return(result);
}

static String8
raddbg_string_from_arch(RADDBG_Arch arch){
  String8 result = {0};
  switch (arch){
    default:              result = str8_lit("<invalid-arch>"); break;
    case RADDBG_Arch_X86: result = str8_lit("x86"); break;
    case RADDBG_Arch_X64: result = str8_lit("x64"); break;
  }
  return(result);
}

static String8
raddbg_string_from_language(RADDBG_Language language){
  String8 result = {0};
  switch (language){
#define X(name,code) case code: result = str8_lit(#name); break;
    RADDBG_LanguageXList(X)
#undef X
  }
  return(result);
}

static String8
raddbg_string_from_type_kind(RADDBG_TypeKind type_kind){
  String8 result = {0};
  switch (type_kind){
    default: result = str8_lit("<invalid-type-kind>"); break;
#define X(name,code) case code: result = str8_lit(#name); break;
#define XZ(name,code,size) X(name,code)
#define Y(a,n)
    RADDBG_TypeKindXList(X,XZ,Y)
#undef X
#undef XZ
#undef Y
  }
  return(result);
}

static String8
raddbg_string_from_member_kind(RADDBG_MemberKind member_kind){
  String8 result = {0};
  switch (member_kind){
    default: result = str8_lit("<invalid-member-kind>"); break;
#define X(N,C) case C: result = str8_lit(#N); break;
    RADDBG_MemberKindXList(X)
#undef X
  }
  return(result);
}

static String8
raddbg_string_from_local_kind(RADDBG_LocalKind local_kind){
  String8 result = {0};
  switch (local_kind){
    default:                         result = str8_lit("<invalid-local-kind>"); break;
    case RADDBG_LocalKind_Parameter: result = str8_lit("Parameter");            break;
    case RADDBG_LocalKind_Variable:  result = str8_lit("Variable");             break;
  }
  return(result);
}


////////////////////////////////
//~ RADDBG Flags Stringize Functions

static void
raddbg_stringize_binary_section_flags(Arena *arena, String8List *out,
                                      RADDBG_BinarySectionFlags flags){
  if (flags == 0){
    str8_list_push(arena, out, str8_lit("0"));
  }
  if (flags & RADDBG_BinarySectionFlag_Read){
    str8_list_push(arena, out, str8_lit("Read "));
  }
  if (flags & RADDBG_BinarySectionFlag_Write){
    str8_list_push(arena, out, str8_lit("Write "));
  }
  if (flags & RADDBG_BinarySectionFlag_Execute){
    str8_list_push(arena, out, str8_lit("Execute "));
  }
}

static void
raddbg_stringize_type_modifier_flags(Arena *arena, String8List *out,
                                     RADDBG_TypeModifierFlags flags){
  if (flags == 0){
    str8_list_push(arena, out, str8_lit("0"));
  }
  if (flags & RADDBG_TypeModifierFlag_Const){
    str8_list_push(arena, out, str8_lit("Const "));
  }
  if (flags & RADDBG_TypeModifierFlag_Volatile){
    str8_list_push(arena, out, str8_lit("Volatile "));
  }
}

static void
raddbg_stringize_user_defined_type_flags(Arena *arena, String8List *out,
                                         RADDBG_UserDefinedTypeFlags flags){
  if (flags == 0){
    str8_list_push(arena, out, str8_lit("0"));
  }
  if (flags & RADDBG_UserDefinedTypeFlag_EnumMembers){
    str8_list_push(arena, out, str8_lit("EnumMembers "));
  }
}

static void
raddbg_stringize_link_flags(Arena *arena, String8List *out, RADDBG_LinkFlags flags){
  if (flags == 0){
    str8_list_push(arena, out, str8_lit("0"));
  }
  if (flags & RADDBG_LinkFlag_External){
    str8_list_push(arena, out, str8_lit("External "));
  }
  if (flags & RADDBG_LinkFlag_TypeScoped){
    str8_list_push(arena, out, str8_lit("TypeScoped "));
  }
  if (flags & RADDBG_LinkFlag_ProcScoped){
    str8_list_push(arena, out, str8_lit("ProcScoped "));
  }
}


////////////////////////////////
//~ RADDBG Compound Stringize Functions

static char raddbg_stringize_spaces[] = "                                ";

static void
raddbg_stringize_data_sections(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                               U32 indent_level){
  U64 data_section_count = parsed->dsec_count;
  RADDBG_DataSection *ptr = parsed->dsecs;
  for (U64 i = 0; i < data_section_count; i += 1, ptr += 1){
    String8 tag_str = raddbg_string_from_data_section_tag(ptr->tag);
    str8_list_pushf(arena, out, "%.*sdata_section[%5u] = {0x%08llx, %7u, %7u} %.*s\n",
                    indent_level, raddbg_stringize_spaces,
                    i, ptr->off, ptr->encoded_size, ptr->unpacked_size, str8_varg(tag_str));
  }
}

static void
raddbg_stringize_top_level_info(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                                RADDBG_TopLevelInfo *tli, U32 indent_level){
  String8 arch_str = raddbg_string_from_arch(tli->architecture);
  String8 exe_name = {0};
  exe_name.str = raddbg_string_from_idx(parsed, tli->exe_name_string_idx, &exe_name.size);
  
  str8_list_pushf(arena, out, "%.*sarchitecture=%.*s\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(arch_str));
  str8_list_pushf(arena, out, "%.*sexe_name='%.*s'\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(exe_name));
  str8_list_pushf(arena, out, "%.*svoff_max=0x%08llx\n",
                  indent_level, raddbg_stringize_spaces, tli->voff_max);
}

static void
raddbg_stringize_binary_section(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                                RADDBG_BinarySection *bin_section, U32 indent_level){
  String8 name = {0};
  name.str = raddbg_string_from_idx(parsed, bin_section->name_string_idx, &name.size);
  str8_list_pushf(arena, out, "%.*sname='%.*s'\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(name));
  
  str8_list_pushf(arena, out, "%.*sflags=", indent_level, raddbg_stringize_spaces);
  raddbg_stringize_binary_section_flags(arena, out, bin_section->flags);
  str8_list_pushf(arena, out, "\n");
  
  str8_list_pushf(arena, out, "%.*svoff_first=0x%08x\n",
                  indent_level, raddbg_stringize_spaces, bin_section->voff_first);
  str8_list_pushf(arena, out, "%.*svoff_opl  =0x%08x\n",
                  indent_level, raddbg_stringize_spaces, bin_section->voff_opl);
  str8_list_pushf(arena, out, "%.*sfoff_first=0x%08x\n",
                  indent_level, raddbg_stringize_spaces, bin_section->foff_first);
  str8_list_pushf(arena, out, "%.*sfoff_opl  =0x%08x\n",
                  indent_level, raddbg_stringize_spaces, bin_section->foff_opl);
}

static void
raddbg_stringize_file_path(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                           RADDBG_FilePathBundle *bundle, RADDBG_FilePathNode *file_path,
                           U32 indent_level){
  String8 name = {0};
  name.str = raddbg_string_from_idx(parsed, file_path->name_string_idx, &name.size);
  
  U32 this_idx = (U32)(file_path - bundle->file_paths);
  
  if (file_path->source_file_idx == 0){
    str8_list_pushf(arena, out, "%.*s[%u] '%.*s'\n",
                    indent_level, raddbg_stringize_spaces,
                    this_idx, str8_varg(name));
  }
  else{
    str8_list_pushf(arena, out, "%.*s[%u] '%.*s'; source_file=%u\n",
                    indent_level, raddbg_stringize_spaces,
                    this_idx, str8_varg(name), file_path->source_file_idx);
  }
  
  for (U32 child = file_path->first_child;
       child != 0;){
    // get node for child
    RADDBG_FilePathNode *child_node = 0;
    if (child < bundle->file_path_count){
      child_node = bundle->file_paths + child;
    }
    if (child_node == 0){
      break;
    }
    
    // stringize child
    raddbg_stringize_file_path(arena, out, parsed, bundle, child_node, indent_level + 1);
    
    // increment iterator
    child = child_node->next_sibling;
  }
}

static void
raddbg_stringize_source_file(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                             RADDBG_SourceFile *source_file, U32 indent_level){
  // extract line map data
  RADDBG_ParsedLineMap line_map = {0};
  raddbg_line_map_from_source_file(parsed, source_file, &line_map);
  
  // stringize line map data
  str8_list_pushf(arena, out, "%.*slines:\n", indent_level, raddbg_stringize_spaces);
  
  for (U32 i = 0; i < line_map.count; i += 1){
    U32 line_num = line_map.nums[i];
    
    U32 digit_count = 1;
    if (line_num > 0){
      U32 x = line_num;
      for (;;){
        x /= 10;
        if (x == 0){
          break;
        }
        digit_count += 1;
      }
    }
    
    str8_list_pushf(arena, out, "%.*s %u: ",
                    indent_level, raddbg_stringize_spaces, line_num);
    
    U32 first = line_map.ranges[i];
    U32 opl_raw = line_map.ranges[i + 1];
    U32 opl = ClampTop(opl_raw, line_map.voff_count);
    for (U32 j = first; j < opl; j += 1){
      if (j == first){
        str8_list_pushf(arena, out, "0x%08x\n", line_map.voffs[j]);
      }
      else{
        str8_list_pushf(arena, out, "%.*s0x%08x\n",
                        indent_level + digit_count + 3, raddbg_stringize_spaces,
                        line_map.voffs[j]);
      }
    }
  }
}

static void
raddbg_stringize_unit(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                      RADDBG_Unit *unit, U32 indent_level){
  String8 unit_name = {0};
  unit_name.str = raddbg_string_from_idx(parsed, unit->unit_name_string_idx, &unit_name.size);
  String8 compiler_name = {0};
  compiler_name.str = raddbg_string_from_idx(parsed, unit->compiler_name_string_idx,
                                             &compiler_name.size);
  
  str8_list_pushf(arena, out, "%.*sunit_name='%.*s'\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(unit_name));
  str8_list_pushf(arena, out, "%.*scompiler_name='%.*s'\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(compiler_name));
  
  str8_list_pushf(arena, out, "%.*ssource_file_path=%u\n",
                  indent_level, raddbg_stringize_spaces, unit->source_file_path_node);
  str8_list_pushf(arena, out, "%.*sobject_file_path=%u\n",
                  indent_level, raddbg_stringize_spaces, unit->object_file_path_node);
  str8_list_pushf(arena, out, "%.*sarchive_file_path=%u\n",
                  indent_level, raddbg_stringize_spaces, unit->archive_file_path_node);
  str8_list_pushf(arena, out, "%.*sbuild_path=%u\n",
                  indent_level, raddbg_stringize_spaces, unit->build_path_node);
  
  String8 language_str = raddbg_string_from_language(unit->language);
  str8_list_pushf(arena, out, "%.*slanguage=%.*s\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(language_str));
  
  // extract line info data
  RADDBG_ParsedLineInfo line_info = {0};
  raddbg_line_info_from_unit(parsed, unit, &line_info);
  
  
  // stringize line info
  str8_list_pushf(arena, out, "%.*slines:\n", indent_level, raddbg_stringize_spaces);
  
  for (U32 i = 0; i < line_info.count; i += 1){
    U64 first = line_info.voffs[i];
    U64 opl   = line_info.voffs[i + 1];
    RADDBG_Line *line = line_info.lines + i;
    RADDBG_Column *col = 0;
    if (i < line_info.col_count){
      col = line_info.cols + i;
    }
    
    if (col == 0){
      str8_list_pushf(arena, out, "%.*s [0x%08llx,0x%08llx) file=%u; line=%u\n",
                      indent_level, raddbg_stringize_spaces,
                      first, opl, line->file_idx, line->line_num);
    }
    else{
      str8_list_pushf(arena, out, "%.*s [0x%08llx,0x%08llx) file=%u; line=%u; columns=[%u,%u)\n",
                      indent_level, raddbg_stringize_spaces,
                      first, opl, line->file_idx, line->line_num,
                      col->col_first, col->col_opl);
    }
  }
}

static void
raddbg_stringize_type_node(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                           RADDBG_TypeNode *type, U32 indent_level){
  RADDBG_TypeKind kind = type->kind;
  String8 type_kind_str = raddbg_string_from_type_kind(kind);
  
  str8_list_pushf(arena, out, "%.*skind=%.*s\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(type_kind_str));
  
  switch (type->kind){
    case RADDBG_TypeKind_Modifier:
    {
      str8_list_pushf(arena, out, "%.*sflags=", indent_level, raddbg_stringize_spaces);
      raddbg_stringize_type_modifier_flags(arena, out, type->flags);
      str8_list_push(arena, out, str8_lit("\n"));
    }break;
    
    default:
    {
      if (type->flags != 0){
        str8_list_pushf(arena, out, "%.*sflags=%x (missing stringizer path)",
                        indent_level, raddbg_stringize_spaces, type->flags);
      }
    }break;
  }
  
  str8_list_pushf(arena, out, "%.*sbyte_size=%u\n",
                  indent_level, raddbg_stringize_spaces, type->byte_size);
  
  if (RADDBG_TypeKind_FirstBuiltIn <= kind &&
      kind <= RADDBG_TypeKind_LastBuiltIn){
    String8 name = {0};
    name.str = raddbg_string_from_idx(parsed, type->built_in.name_string_idx, &name.size);
    str8_list_pushf(arena, out, "%.*sbuilt_in.name='%.*s'\n",
                    indent_level, raddbg_stringize_spaces, str8_varg(name));
  }
  
  else if (RADDBG_TypeKind_FirstConstructed <= kind &&
           kind <= RADDBG_TypeKind_LastConstructed){
    str8_list_pushf(arena, out, "%.*sconstructed.direct_type=%u\n",
                    indent_level, raddbg_stringize_spaces, type->constructed.direct_type_idx);
    
    if (type->kind == RADDBG_TypeKind_Array){
      str8_list_pushf(arena, out, "%.*sconstructed.array_count=%u\n",
                      indent_level, raddbg_stringize_spaces, type->constructed.count);
    }
    
    if (type->kind == RADDBG_TypeKind_Function ||
        type->kind == RADDBG_TypeKind_Method){
      U32 run_first = type->constructed.param_idx_run_first;
      U32 run_count_raw = type->constructed.count;
      
      U32 run_count = 0;
      U32 *run = raddbg_idx_run_from_first_count(parsed, run_first, run_count_raw, &run_count);
      
      U32 this_type_idx = 0;
      if (run_count > 0 && type->kind == RADDBG_TypeKind_Method){
        this_type_idx = run[0];
        run += 1;
        run_count -= 1;
      }
      
      if (this_type_idx != 0){
        str8_list_pushf(arena, out, "%.*sconstructed.this_type=%u\n",
                        indent_level, raddbg_stringize_spaces, this_type_idx);
      }
      
      str8_list_pushf(arena, out, "%.*sconstructed.params={",
                      indent_level, raddbg_stringize_spaces);
      
      if (run_count > 0){
        U32 last = run_count - 1;
        for (U32 j = 0; j < last; j += 1){
          str8_list_pushf(arena, out, " %u,", run[j]);
        }
        str8_list_pushf(arena, out, " %u ", run[last]);
      }
      
      str8_list_push(arena, out, str8_lit("}\n"));
    }
  }
  
  else if (RADDBG_TypeKind_FirstUserDefined <= kind &&
           kind <= RADDBG_TypeKind_LastUserDefined){
    String8 name = {0};
    name.str = raddbg_string_from_idx(parsed, type->user_defined.name_string_idx, &name.size);
    str8_list_pushf(arena, out, "%.*suser_defined.name='%.*s'\n",
                    indent_level, raddbg_stringize_spaces, str8_varg(name));
    str8_list_pushf(arena, out, "%.*suser_defined.direct_type=%u\n",
                    indent_level, raddbg_stringize_spaces,
                    type->user_defined.direct_type_idx);
    str8_list_pushf(arena, out, "%.*suser_defined.udt=%u\n",
                    indent_level, raddbg_stringize_spaces,
                    type->user_defined.udt_idx);
  }
  
  else if (kind == RADDBG_TypeKind_Bitfield){
    str8_list_pushf(arena, out, "%.*sbitfield.off=%u\n",
                    indent_level, raddbg_stringize_spaces, type->bitfield.off);
    str8_list_pushf(arena, out, "%.*sbitfield.size=%u\n",
                    indent_level, raddbg_stringize_spaces, type->bitfield.size);
  }
}

static void
raddbg_stringize_udt(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                     RADDBG_UDTMemberBundle *member_bundle, RADDBG_UDT *udt,
                     U32 indent_level){
  str8_list_pushf(arena, out, "%.*sself_type=%u\n",
                  indent_level, raddbg_stringize_spaces, udt->self_type_idx);
  
  str8_list_pushf(arena, out, "%.*sflags=", indent_level, raddbg_stringize_spaces);
  raddbg_stringize_user_defined_type_flags(arena, out, udt->flags);
  str8_list_push(arena, out, str8_lit("\n"));
  
  if (udt->file_idx != 0){
    str8_list_pushf(arena, out, "%.*sloc={file=%u; line=%u; col=%u}\n",
                    indent_level, raddbg_stringize_spaces,
                    udt->file_idx, udt->line, udt->col);
  }
  
  // enum members
  if (udt->flags & RADDBG_UserDefinedTypeFlag_EnumMembers){
    U32 first_raw = udt->member_first;
    U32 opl_raw = first_raw + udt->member_count;
    U32 opl = ClampTop(opl_raw, member_bundle->enum_member_count);
    U32 first = ClampTop(first_raw, opl);
    
    if (first < opl){
      str8_list_pushf(arena, out, "%.*smembers={\n", indent_level, raddbg_stringize_spaces);
      RADDBG_EnumMember *enum_member = member_bundle->enum_members + first;
      for (U32 i = first; i < opl; i += 1, enum_member += 1){
        String8 name = {0};
        name.str = raddbg_string_from_idx(parsed, enum_member->name_string_idx, &name.size);
        str8_list_pushf(arena, out, "%.*s '%.*s' %llu\n",
                        indent_level, raddbg_stringize_spaces,
                        str8_varg(name), enum_member->val);
      }
      str8_list_pushf(arena, out, "%.*s}\n", indent_level, raddbg_stringize_spaces);
    }
  }
  
  // field members
  else{
    U32 first_raw = udt->member_first;
    U32 opl_raw = first_raw + udt->member_count;
    U32 opl = ClampTop(opl_raw, member_bundle->member_count);
    U32 first = ClampTop(first_raw, opl);
    
    if (first < opl){
      str8_list_pushf(arena, out, "%.*smembers={\n", indent_level, raddbg_stringize_spaces);
      RADDBG_Member *member = member_bundle->members + first;
      for (U32 i = first; i < opl; i += 1, member += 1){
        str8_list_pushf(arena, out, "%.*s {\n", indent_level, raddbg_stringize_spaces);
        
        String8 kind_str = raddbg_string_from_member_kind(member->kind);
        str8_list_pushf(arena, out, "%.*s  kind=%.*s\n",
                        indent_level, raddbg_stringize_spaces, str8_varg(kind_str));
        
        if (member->name_string_idx != 0){
          String8 name = {0};
          name.str = raddbg_string_from_idx(parsed, member->name_string_idx, &name.size);
          str8_list_pushf(arena, out, "%.*s  name='%.*s'\n",
                          indent_level, raddbg_stringize_spaces, str8_varg(name));
        }
        
        str8_list_pushf(arena, out, "%.*s  type=%u\n",
                        indent_level, raddbg_stringize_spaces, member->type_idx);
        str8_list_pushf(arena, out, "%.*s  off=%u\n",
                        indent_level, raddbg_stringize_spaces, member->off);
        
        str8_list_pushf(arena, out, "%.*s }\n", indent_level, raddbg_stringize_spaces);
      }
      str8_list_pushf(arena, out, "%.*s}\n", indent_level, raddbg_stringize_spaces);
    }
  }
}

static void
raddbg_stringize_global_variable(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                                 RADDBG_GlobalVariable *global_variable, U32 indent_level){
  String8 name = {0};
  name.str = raddbg_string_from_idx(parsed, global_variable->name_string_idx, &name.size);
  str8_list_pushf(arena, out, "%.*sname='%.*s'\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(name));
  
  str8_list_pushf(arena, out, "%.*slink_flags=", indent_level, raddbg_stringize_spaces);
  raddbg_stringize_link_flags(arena, out, global_variable->link_flags);
  str8_list_push(arena, out, str8_lit("\n"));
  
  str8_list_pushf(arena, out, "%.*svoff=0x%08llx\n",
                  indent_level, raddbg_stringize_spaces, global_variable->voff);
  
  str8_list_pushf(arena, out, "%.*stype_idx=%u\n",
                  indent_level, raddbg_stringize_spaces, global_variable->type_idx);
  
  str8_list_pushf(arena, out, "%.*scontainer_idx=%u\n",
                  indent_level, raddbg_stringize_spaces, global_variable->container_idx);
}

static void
raddbg_stringize_thread_variable(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                                 RADDBG_ThreadVariable *thread_var,
                                 U32 indent_level){
  String8 name = {0};
  name.str = raddbg_string_from_idx(parsed, thread_var->name_string_idx, &name.size);
  str8_list_pushf(arena, out, "%.*sname='%.*s'\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(name));
  
  str8_list_pushf(arena, out, "%.*slink_flags=", indent_level, raddbg_stringize_spaces);
  raddbg_stringize_link_flags(arena, out, thread_var->link_flags);
  str8_list_push(arena, out, str8_lit("\n"));
  
  str8_list_pushf(arena, out, "%.*stls_off=0x%08x\n",
                  indent_level, raddbg_stringize_spaces, thread_var->tls_off);
  
  str8_list_pushf(arena, out, "%.*stype_idx=%u\n",
                  indent_level, raddbg_stringize_spaces, thread_var->type_idx);
  
  str8_list_pushf(arena, out, "%.*scontainer_idx=%u\n",
                  indent_level, raddbg_stringize_spaces, thread_var->container_idx);
}

static void
raddbg_stringize_procedure(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                           RADDBG_Procedure *proc, U32 indent_level){
  String8 name = {0};
  name.str = raddbg_string_from_idx(parsed, proc->name_string_idx, &name.size);
  str8_list_pushf(arena, out, "%.*sname='%.*s'\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(name));
  
  String8 link_name = {0};
  link_name.str = raddbg_string_from_idx(parsed, proc->link_name_string_idx, &link_name.size);
  str8_list_pushf(arena, out, "%.*slink_name='%.*s'\n",
                  indent_level, raddbg_stringize_spaces, str8_varg(link_name));
  
  str8_list_pushf(arena, out, "%.*slink_flags=", indent_level, raddbg_stringize_spaces);
  raddbg_stringize_link_flags(arena, out, proc->link_flags);
  str8_list_push(arena, out, str8_lit("\n"));
  
  str8_list_pushf(arena, out, "%.*stype_idx=%u\n",
                  indent_level, raddbg_stringize_spaces, proc->type_idx);
  
  str8_list_pushf(arena, out, "%.*sroot_scope_idx=%u\n",
                  indent_level, raddbg_stringize_spaces, proc->root_scope_idx);
  
  str8_list_pushf(arena, out, "%.*scontainer_idx=%u\n",
                  indent_level, raddbg_stringize_spaces, proc->container_idx);
}

static void
raddbg_stringize_scope(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                       RADDBG_ScopeBundle *bundle, RADDBG_Scope *scope, U32 indent_level){
  
  U32 this_idx = (U32)(scope - bundle->scopes);
  
  str8_list_pushf(arena, out, "%.*s[%u]\n",
                  indent_level, raddbg_stringize_spaces, this_idx);
  
  str8_list_pushf(arena, out, "%.*s proc_idx=%u\n",
                  indent_level, raddbg_stringize_spaces, scope->proc_idx);
  
  // voff ranges
  {
    U32 voff_range_first_raw = scope->voff_range_first;
    U32 voff_range_opl_raw = scope->voff_range_opl;
    U32 voff_range_opl_clamped = ClampTop(voff_range_opl_raw, bundle->scope_voff_count);
    U32 voff_range_first = ClampTop(voff_range_first_raw, voff_range_opl_clamped);
    U32 voff_range_opl = voff_range_opl_clamped;
    if ((voff_range_opl - voff_range_first) % 2 == 1){
      voff_range_opl -= 1;
    }
    
    U64 *voff_ptr = bundle->scope_voffs + voff_range_first;
    
    if (voff_range_opl - voff_range_first > 2){
      str8_list_pushf(arena, out, "%.*s voff_ranges={\n",
                      indent_level, raddbg_stringize_spaces);
      for (U32 i = voff_range_first; i < voff_range_opl; i += 2, voff_ptr += 2){
        str8_list_pushf(arena, out, "%.*s  [0x%08llx, 0x%08llx)\n",
                        indent_level, raddbg_stringize_spaces,
                        voff_ptr[0], voff_ptr[1]);
      }
      str8_list_pushf(arena, out, "%.*s }\n",
                      indent_level, raddbg_stringize_spaces);
    }
    else if (voff_range_opl - voff_range_first == 2){
      str8_list_pushf(arena, out, "%.*s voff_range=[0x%08llx, 0x%08llx)\n",
                      indent_level, raddbg_stringize_spaces,
                      voff_ptr[0], voff_ptr[1]);
    }
  }
  
  // locals
  {
    U32 local_first = scope->local_first;
    U32 local_opl_raw = local_first + scope->local_count;
    U32 local_opl = ClampTop(local_opl_raw, bundle->local_count);
    
    if (local_first < local_opl){
      RADDBG_Local *local_ptr = bundle->locals + local_first;
      for (U32 i = local_first; i < local_opl; i += 1, local_ptr += 1){
        str8_list_pushf(arena, out, "%.*s local[%u]\n",
                        indent_level, raddbg_stringize_spaces, i);
        
        String8 local_kind_str = raddbg_string_from_local_kind(local_ptr->kind);
        str8_list_pushf(arena, out, "%.*s  kind=%.*s\n",
                        indent_level, raddbg_stringize_spaces, str8_varg(local_kind_str));
        
        String8 name = {0};
        name.str = raddbg_string_from_idx(parsed, local_ptr->name_string_idx, &name.size);
        str8_list_pushf(arena, out, "%.*s  name='%.*s'\n",
                        indent_level, raddbg_stringize_spaces, str8_varg(name));
        
        str8_list_pushf(arena, out, "%.*s  type_idx=%u\n",
                        indent_level, raddbg_stringize_spaces, local_ptr->type_idx);
        
        U32 location_first = local_ptr->location_first;
        U32 location_opl_raw = local_ptr->location_opl;
        U32 location_opl = ClampTop(location_opl_raw, bundle->location_block_count);
        
        if (location_first < location_opl){
          str8_list_pushf(arena, out, "%.*s  locations:\n", indent_level, raddbg_stringize_spaces);
          RADDBG_LocationBlock *block_ptr = bundle->location_blocks + location_first;
          for (U32 j = location_first; j < location_opl; j += 1, block_ptr += 1){
            if (block_ptr->scope_off_first == 0 && block_ptr->scope_off_opl == max_U32){
              str8_list_pushf(arena, out, "%.*s   case *always*:\n", indent_level, raddbg_stringize_spaces);
            }
            else{
              str8_list_pushf(arena, out, "%.*s   case [0x%08x, 0x%08x):\n",
                              indent_level, raddbg_stringize_spaces,
                              block_ptr->scope_off_first, block_ptr->scope_off_opl);
            }
            
            if (block_ptr->location_data_off >= bundle->location_data_size){
              str8_list_pushf(arena, out, "%.*s    <bad-location-data-offset>\n",
                              indent_level, raddbg_stringize_spaces);
            }
            else{
              str8_list_pushf(arena, out, "%.*s    ", indent_level, raddbg_stringize_spaces);
              
              U8 *loc_data_opl = bundle->location_data + bundle->location_data_size;
              U8 *loc_base_ptr = bundle->location_data + block_ptr->location_data_off;
              RADDBG_LocationKind kind = (RADDBG_LocationKind)*loc_base_ptr;
              switch (kind){
                default:
                {
                  str8_list_pushf(arena, out, "<invalid-location-kind>\n");
                }break;
                
                case RADDBG_LocationKind_AddrBytecodeStream:
                {
                  str8_list_pushf(arena, out, "AddrBytecodeStream\n");
                  str8_list_pushf(arena, out, "%.*s     ", indent_level, raddbg_stringize_spaces);
                  U8 *bytecode_ptr = loc_base_ptr + 1;
                  for (;bytecode_ptr < loc_data_opl && *bytecode_ptr != 0; bytecode_ptr += 1){
                    str8_list_pushf(arena, out, "%02x ", *bytecode_ptr);
                  }
                  str8_list_pushf(arena, out, "\n");
                }break;
                
                case RADDBG_LocationKind_ValBytecodeStream:
                {
                  str8_list_pushf(arena, out, "ValBytecodeStream\n");
                  str8_list_pushf(arena, out, "%.*s     ", indent_level, raddbg_stringize_spaces);
                  U8 *bytecode_ptr = loc_base_ptr + 1;
                  for (;bytecode_ptr < loc_data_opl && *bytecode_ptr != 0; bytecode_ptr += 1){
                    str8_list_pushf(arena, out, "%02x ", *bytecode_ptr);
                  }
                  str8_list_pushf(arena, out, "\n");
                }break;
                
                case RADDBG_LocationKind_AddrRegisterPlusU16:
                {
                  if (loc_base_ptr + sizeof(RADDBG_LocationRegisterPlusU16) > loc_data_opl){
                    str8_list_pushf(arena, out, "AddrRegisterPlusU16( <invalid-encoding> )\n");
                  }
                  else{
                    RADDBG_LocationRegisterPlusU16 *loc = (RADDBG_LocationRegisterPlusU16*)loc_base_ptr;
                    str8_list_pushf(arena, out, "AddrRegisterPlusU16(reg: %u, off: %u)\n",
                                    loc->register_code, loc->offset);
                  }
                }break;
                
                case RADDBG_LocationKind_AddrAddrRegisterPlusU16:
                {
                  if (loc_base_ptr + sizeof(RADDBG_LocationRegisterPlusU16) > loc_data_opl){
                    str8_list_pushf(arena, out, "AddrAddrRegisterPlusU16( <invalid-encoding> )\n");
                  }
                  else{
                    RADDBG_LocationRegisterPlusU16 *loc = (RADDBG_LocationRegisterPlusU16*)loc_base_ptr;
                    str8_list_pushf(arena, out, "AddrAddrRegisterPlusU16(reg: %u, off: %u)\n",
                                    loc->register_code, loc->offset);
                  }
                }break;
                
                case RADDBG_LocationKind_ValRegister:
                {
                  if (loc_base_ptr + sizeof(RADDBG_LocationRegister) > loc_data_opl){
                    str8_list_pushf(arena, out, "ValRegister( <invalid-encoding> )\n");
                  }
                  else{
                    RADDBG_LocationRegister *loc = (RADDBG_LocationRegister*)loc_base_ptr;
                    str8_list_pushf(arena, out, "ValRegister(reg: %u)\n", loc->register_code);
                  }
                }break;
              }
            }
          }
        }
      }
    }
  }
  
  // TODO(allen): static locals
  
  for (U32 child = scope->first_child_scope_idx;
       child != 0;){
    // get scope for child
    RADDBG_Scope *child_scope = 0;
    if (child < bundle->scope_count){
      child_scope = bundle->scopes + child;
    }
    if (child_scope == 0){
      break;
    }
    
    // stringize child
    raddbg_stringize_scope(arena, out, parsed, bundle, child_scope, indent_level + 1);
    
    // increment iterator
    child = child_scope->next_sibling_scope_idx;
  }
  
  str8_list_pushf(arena, out, "%.*s[/%u]\n",
                  indent_level, raddbg_stringize_spaces, this_idx);
}
