// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "lib_rdi/rdi.c"
#include "lib_rdi/rdi_parse.c"

////////////////////////////////
//~ rjf: RDI Enum <=> Base Enum

internal Arch
arch_from_rdi_arch(RDI_Arch arch)
{
  Arch result = Arch_Null;
  switch((RDI_ArchEnum)arch)
  {
    case RDI_Arch_NULL:{}break;
    case RDI_Arch_X64:{result = Arch_x64;}break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Lookup Helpers

internal String8
str8_from_rdi_string_idx(RDI_Parsed *rdi, U32 idx)
{
  String8 result = {0};
  result.str = rdi_string_from_idx(rdi, idx, &result.size);
  return result;
}

internal String8
str8_from_rdi_path_node_idx(Arena *arena, RDI_Parsed *rdi, PathStyle path_style, U32 path_node_idx)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List path_parts = {0};
  for(RDI_FilePathNode *fpn = rdi_element_from_name_idx(rdi, FilePathNodes, path_node_idx);
      fpn != rdi_element_from_name_idx(rdi, FilePathNodes, 0);
      fpn = rdi_element_from_name_idx(rdi, FilePathNodes, fpn->parent_path_node))
  {
    String8 path_part = {0};
    path_part.str = rdi_string_from_idx(rdi, fpn->name_string_idx, &path_part.size);
    str8_list_push_front(scratch.arena, &path_parts, path_part);
  }
  String8 path = str8_path_list_join_by_style(arena, &path_parts, path_style);
  scratch_end(scratch);
  return path;
}

////////////////////////////////
//~ rjf: String <=> Enum

internal String8
rdi_string_from_data_section_kind(Arena *arena, RDI_SectionKind v)
{
  String8 result = {0};
  switch(v)
  {
    default:{result = push_str8f(arena, "<invalid RDI_SectionKind %u>", v);}break;
#define X(name, lower, type) case RDI_SectionKind_##name:{result = str8_lit(#name);}break;
    RDI_SectionKind_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_arch(Arena *arena, RDI_Arch v)
{
  String8 result = {0};
  switch(v)
  {
    default:{result = push_str8f(arena, "<invalid RDI_Arch %u>", v);} break;
#define X(name) case RDI_Arch_##name:{result = str8_lit(#name);} break;
    RDI_Arch_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_language(Arena *arena, RDI_Language v)
{
  String8 result = {0};
  switch(v)
  {
    default:{result = push_str8f(arena, "<invalid RDI_Language %u>", v);}break;
#define X(name) case RDI_Language_##name:{result = str8_lit(#name);}break;
    RDI_Language_XList
#undef X
  }
  return result;
}

#if 0
internal String8
rdi_string_from_type_kind(Arena *arena, RDI_TypeKind v)
{
  String8 result = {0};
  switch(v)
  {
    default:{result = push_str8f(arena, "<invalid RDI_TypeKind %u>", v);}break;
#define X(name) case RDI_TypeKind_##name:{result = str8_lit(#name);}break;
    RDI_TypeKind_XList
#undef X
  }
  return result;
}
#endif

internal String8
rdi_string_from_member_kind(Arena *arena, RDI_MemberKind v)
{
  String8 result = {0};
  switch(v)
  {
    default:{result = push_str8f(arena, "<invalid RDI_MemberKind %u>", v);}break;
#define X(name) case RDI_MemberKind_##name:{result = str8_lit(#name);}break;
    RDI_MemberKind_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_name_map_kind(RDI_NameMapKind kind)
{
  String8 result = {0};
  switch(kind)
  {
    default:{}break;
#define X(name) case RDI_NameMapKind_##name:{result = str8_lit(#name);}break;
    RDI_NameMapKind_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_reg_code_x64(U64 reg_code)
{
  String8 result = {0};
  switch(reg_code)
  {
    default:{}break;
#define X(name, value) case RDI_RegCodeX64_##name:{result = str8_lit(#name);}break;
    RDI_RegCodeX64_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_reg_code(Arena *arena, RDI_Arch arch, U64 reg_code)
{
  String8 result = {0};
  switch(arch)
  {
    default:
    case RDI_Arch_NULL: {result = push_str8f(arena, "??? (%llu)", reg_code);}break;
    case RDI_Arch_X64:  {result = rdi_string_from_reg_code_x64(reg_code);}break;
  }
  return result;
}

internal String8
rdi_string_from_eval_op(Arena *arena, RDI_EvalOp op)
{
  String8 result = {0};
  switch(op)
  {
    default:{result = push_str8f(arena, "%#x", op);}break;
#define X(name) case RDI_EvalOp_##name:{result = str8_lit(#name);}break;
    RDI_EvalOp_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_eval_type_group(Arena *arena, RDI_EvalTypeGroup eval_type_group)
{
  String8 result = {0};
  switch(eval_type_group)
  {
    default:{result = push_str8f(arena, "%#x", eval_type_group);}break;
#define X(name) case RDI_EvalTypeGroup_##name:{result = str8_lit(#name);}break;
    RDI_EvalTypeGroup_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_binary_section_flags(Arena *arena, RDI_BinarySectionFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
#define X(name) if (flags & RDI_BinarySectionFlag_##name) { flags &= ~RDI_BinarySectionFlag_##name; str8_list_push(scratch.arena, &list, str8_lit(#name)); }
  RDI_BinarySectionFlags_XList;
#undef X
  StringJoin join = {.sep = str8_lit("|")};
  String8 result = str8_list_join(arena, &list, &join);
  if(result.size == 0) { result = str8_lit("None"); }
  scratch_end(scratch);
  return result;
}

internal String8
rdi_string_from_type_modifier_flags(Arena *arena, RDI_TypeModifierFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
#define X(name) if(flags & RDI_TypeModifierFlag_##name) { flags &= ~RDI_TypeModifierFlag_##name; str8_list_push(scratch.arena, &list, str8_lit(#name)); }
  RDI_TypeModifierFlags_XList;
#undef X
  StringJoin join = {.sep = str8_lit("|")};
  String8 result = str8_list_join(arena, &list, &join);
  if(result.size == 0) { result = str8_lit("None"); }
  scratch_end(scratch);
  return result;
}

internal String8
rdi_string_from_udt_flags(Arena *arena, RDI_UDTFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
#define X(name) if (flags & RDI_UDTFlag_##name) { flags &= ~RDI_UDTFlag_##name; str8_list_push(scratch.arena, &list, str8_lit(#name)); }
  RDI_UDTFlags_XList;
#undef X
  StringJoin join = {.sep = str8_lit("|")};
  String8 result = str8_list_join(arena, &list, &join);
  if(result.size == 0) { result = str8_lit("None"); }
  scratch_end(scratch);
  return result;
}

internal String8
rdi_string_from_bytecode(Arena *arena, RDI_Arch arch, String8 bc)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List fmt = {0};
  for(U64 cursor = 0; cursor < bc.size; )
  {
    RDI_EvalOp op = RDI_EvalOp_Stop;
    cursor += str8_deserial_read_struct(bc, cursor, &op);
    
    U16 ctrlbits = rdi_eval_op_ctrlbits_table[op];
    U32 imm_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
    
    String8 imm = {0};
    cursor += str8_deserial_read_block(bc, cursor, imm_size, &imm);
    if (imm.size != imm_size) {
      str8_list_pushf(scratch.arena, &fmt, "(ERROR: not enough bytes to read immediate)");
      break;
    }
    
    String8 imm_fmt = {0};
    switch (op) {
      case RDI_EvalOp_Stop: goto exit;
      case RDI_EvalOp_Noop: break;
      case RDI_EvalOp_Cond: break;
      case RDI_EvalOp_Skip:  {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U16 *)imm.str);
      } break;
      case RDI_EvalOp_MemRead: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U8 *)imm.str);
      } break;
      case RDI_EvalOp_RegRead: {
        U32         regread   = *(U32 *)imm.str;
        RDI_RegCode reg_code  = Extract8(regread, 0);
        U8          byte_size = Extract8(regread, 1);
        U8          byte_off  = Extract8(regread, 2);
        String8     reg_str   = rdi_string_from_reg_code(scratch.arena, arch, reg_code);
        imm_fmt = push_str8f(scratch.arena, "%S+%I64u, Size: %u", reg_str, byte_off, byte_size);
      } break;
      case RDI_EvalOp_RegReadDyn: break;
      case RDI_EvalOp_FrameOff: {
        imm_fmt = push_str8f(scratch.arena, "%+lld", *(S64 *)imm.str);
      } break;
      case RDI_EvalOp_ModuleOff: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U32 *)imm.str);
      } break;
      case RDI_EvalOp_TLSOff: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U32 *)imm.str);
      } break;
      case RDI_EvalOp_ConstU8: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U8 *)imm.str);
      } break;
      case RDI_EvalOp_ConstU16: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U16 *)imm.str);
      } break;
      case RDI_EvalOp_ConstU32: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U32 *)imm.str);
      } break;
      case RDI_EvalOp_ConstU64: {
        imm_fmt = push_str8f(scratch.arena, "%llu", *(U64 *)imm.str);
      } break;
      case RDI_EvalOp_ConstU128: {
        imm_fmt = push_str8f(scratch.arena, "Lo: %llu, Hi: %llu", *(U64 *)imm.str, *((U64 *)imm.str + 1));
      } break;
      case RDI_EvalOp_ConstString: {
        U8      size   = *(U8 *)imm.str;
        String8 string = {0};
        cursor += str8_deserial_read_block(bc, cursor, size, &string);
        
        imm_fmt = push_str8f(scratch.arena, "(%u) \"%S\"", size, string);
      } break;
      case RDI_EvalOp_Abs:
      case RDI_EvalOp_Neg: 
      case RDI_EvalOp_Add:
      case RDI_EvalOp_Sub:
      case RDI_EvalOp_Mul:
      case RDI_EvalOp_Div:
      case RDI_EvalOp_Mod:
      case RDI_EvalOp_LShift:
      case RDI_EvalOp_RShift:
      case RDI_EvalOp_BitAnd:
      case RDI_EvalOp_BitOr:
      case RDI_EvalOp_BitXor:
      case RDI_EvalOp_BitNot:
      case RDI_EvalOp_LogAnd:
      case RDI_EvalOp_LogOr:
      case RDI_EvalOp_LogNot:
      case RDI_EvalOp_EqEq:
      case RDI_EvalOp_NtEq:
      case RDI_EvalOp_LsEq:
      case RDI_EvalOp_GrEq:
      case RDI_EvalOp_Less:
      case RDI_EvalOp_Grtr: {
        U8 eval_type_group = *(U8 *)imm.str;
        imm_fmt = rdi_string_from_eval_type_group(scratch.arena, eval_type_group);
      } break;
      case RDI_EvalOp_Trunc:
      case RDI_EvalOp_TruncSigned: {
        U8 trunc = *(U8 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", trunc);
      } break;
      case RDI_EvalOp_Convert: {
        U16 convert = *(U16 *)imm.str;
        U8 in  = Extract8(convert, 0);
        U8 out = Extract8(convert, 1);
        String8 in_str  = rdi_string_from_eval_type_group(scratch.arena, in);
        String8 out_str = rdi_string_from_eval_type_group(scratch.arena, out);
        imm_fmt = push_str8f(scratch.arena, "in: %S out: %S", in_str, out_str);
      } break;
      case RDI_EvalOp_Pick: {
        U8 pick = *(U8 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", pick);
      } break;
      case RDI_EvalOp_Pop: break;
      case RDI_EvalOp_Insert: {
        U8 insert = *(U8 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", insert);
      } break;
      case RDI_EvalOp_ValueRead: {
        U8 bytes_to_read = *(U8 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", bytes_to_read);
      } break;
      case RDI_EvalOp_ByteSwap: {
        U8 byte_size = *(U8 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", byte_size);
      } break;
      case RDI_EvalOp_CallSiteValue: {
        U32     call_site_bc_size = *(U32 *)imm.str;
        String8 call_site_bc      = {0};
        cursor += str8_deserial_read_block(bc, cursor, call_site_bc_size, &call_site_bc);
        
        String8 call_site_str = rdi_string_from_bytecode(scratch.arena, arch, call_site_bc);
        imm_fmt = push_str8f(scratch.arena, "%S", call_site_str);
      } break;
      case RDI_EvalOp_PartialValue: {
        U32 partial_value_size = *(U32 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", partial_value_size);
      } break;
      case RDI_EvalOp_PartialValueBit: {
        U64 partial_value = *(U64 *)imm.str;
        U32 bit_size = Extract32(partial_value, 0);
        U32 bit_off  = Extract32(partial_value, 1);
        imm_fmt = push_str8f(scratch.arena, "Off: %u, Size: %u", bit_size, bit_off);
      } break;
    }
    
    String8 op_str = rdi_string_from_eval_op(scratch.arena, op);
    if (imm_fmt.size) {
      str8_list_pushf(scratch.arena, &fmt, "RDI_EvalOp_%S(%S)", op_str, imm_fmt);
    } else {
      str8_list_pushf(scratch.arena, &fmt, "RDI_EvalOp_%S", op_str);
    }
  }
  exit:;
  
  String8 result = str8_list_join(arena, &fmt, &(StringJoin){.sep = str8_lit(", ")});
  
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: RDI Dumping

internal String8List
rdi_dump_list_from_parsed(Arena *arena, RDI_Parsed *rdi, RDI_DumpSubsetFlags flags)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  String8 indent = str8_lit("                                                                                                                                ");
  
  //////////////////////////////
  //- rjf: set up
  //
  typedef struct DumpSubsetOutputNode DumpSubsetOutputNode;
  struct DumpSubsetOutputNode
  {
    DumpSubsetOutputNode *next;
    RDI_DumpSubset subset;
    String8List *lane_strings;
  };
  local_persist DumpSubsetOutputNode *first_output_node = 0;
  local_persist DumpSubsetOutputNode *last_output_node = 0;
  local_persist String8List result_strings = {0};
  String8List *strings = 0;
#define dump(str)  str8_list_push(arena, strings, (str))
#define dumpf(...) str8_list_pushf(arena, strings, __VA_ARGS__)
#define DumpSubsetKind(kind) \
if(lane_idx() == 0)\
{\
DumpSubsetOutputNode *n = push_array(scratch.arena, DumpSubsetOutputNode, 1);\
SLLQueuePush(first_output_node, last_output_node, n);\
n->subset = (kind);\
n->lane_strings = push_array(scratch.arena, String8List, lane_count());\
}\
lane_sync();\
strings = &last_output_node->lane_strings[lane_idx()];\
lane_sync(); if(flags & (1ull<<(kind))) ProfScope(rdi_name_title_from_dump_subset_table[kind].str)
#define DumpSubset(name) DumpSubsetKind(RDI_DumpSubset_##name)
  
  //////////////////////////////
  //- rjf: dump data sections
  //
  DumpSubset(DataSections)
  {
    if(lane_idx() == 0) { dumpf("\n"); }
    Rng1U64 range = lane_range(rdi->sections_count);
    for EachInRange(idx, range)
    {
      Temp scratch = scratch_begin(&arena, 1);
      RDI_SectionKind  kind     = (RDI_SectionKind)idx;
      RDI_Section     *section  = &rdi->sections[idx];
      String8          kind_str = rdi_string_from_data_section_kind(scratch.arena, kind);
      dumpf("  {%#010llx  %10u  %10u  %*s} // data_section[%I64u]\n", section->off, section->encoded_size, section->unpacked_size, 24, kind_str.str, idx);
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: dump top-level-info
  //
  DumpSubset(TopLevelInfo)
  {
    if(lane_idx() == 0)
    {
      RDI_TopLevelInfo *tli = rdi_element_from_name_idx(rdi, TopLevelInfo, 0);
      Temp scratch = scratch_begin(&arena, 1);
      Guid guid = {0};
      MemoryCopy(&guid, &tli->guid, Min(sizeof guid, sizeof tli->guid));
      dumpf("\n");
      dumpf("  arch:          %S\n",      rdi_string_from_arch(scratch.arena, tli->arch));
      dumpf("  exe_name:      '%S'\n",    str8_from_rdi_string_idx(rdi, tli->exe_name_string_idx));
      dumpf("  voff_max:      %#08llx\n", tli->voff_max);
      dumpf("  producer_name: '%S'\n",    str8_from_rdi_string_idx(rdi, tli->producer_name_string_idx));
      dumpf("  guid:          %S\n",      string_from_guid(scratch.arena, guid));
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: dump binary sections
  //
  DumpSubset(BinarySections)
  {
    if(lane_idx() == 0)
    {
      dumpf("\n  // %-16s %-16s %-12s %-12s %-12s %s\n", "name", "flags", "voff_first", "voff_opl", "foff_first", "foff_opl");
    }
    U64 count = 0;
    RDI_BinarySection *v = rdi_table_from_name(rdi, BinarySections, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      Temp scratch = scratch_begin(&arena, 1);
      RDI_BinarySection *bin_section = &v[idx];
      String8 name = str8_from_rdi_string_idx(rdi, bin_section->name_string_idx);
      String8 flags = rdi_string_from_binary_section_flags(scratch.arena, bin_section->flags);
      dumpf("  {  %-16S %-16S 0x%-10I64x 0x%-10I64x 0x%-10I64x 0x%-10I64x  } // binary_section[%I64u]\n", 
            push_str8f(scratch.arena, "'%S'", name),
            push_str8f(scratch.arena, "`%S`", flags),
            bin_section->voff_first,
            bin_section->voff_opl,
            bin_section->foff_first,
            bin_section->foff_opl,
            idx);
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: dump file paths
  //
  DumpSubset(FilePaths)
  {
    if(lane_idx() == 0) { dumpf("\n"); }
    U64 count = 0;
    RDI_FilePathNode *v = rdi_table_from_name(rdi, FilePathNodes, &count);
    RDI_FilePathNode *nil = &v[0];
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_FilePathNode *root = &v[idx];
      if(root->parent_path_node != 0) { continue; }
      S64 depth = 1;
      for(RDI_FilePathNode *n = root, *rec_next = nil; n != nil; n = rec_next)
      {
        // rjf: dump
        if(n->source_file_idx == 0)
        {
          dumpf("%.*s'%S'%s // file_path_node[%I64u]\n",
                depth*2, indent.str,
                str8_from_rdi_string_idx(rdi, n->name_string_idx),
                n->first_child ? ":" : "",
                (U64)(n - v));
        }
        else
        {
          dumpf("%.*s'%S': source_file: %u // file_path_node[%I64u]\n", depth*2, indent.str, str8_from_rdi_string_idx(rdi, n->name_string_idx), n->source_file_idx, (U64)(n - v));
        }
        
        // rjf: find next node
        rec_next = nil;
        if(n->first_child)
        {
          dumpf("%.*s{\n", depth*2, indent.str);
          rec_next = rdi_element_from_name_idx(rdi, FilePathNodes, n->first_child);
          depth += 1;
        }
        else for(RDI_FilePathNode *p = n;
                 p != nil && p != root;
                 p = rdi_element_from_name_idx(rdi, FilePathNodes, p->parent_path_node), depth -= 1)
        {
          if(p->next_sibling)
          {
            rec_next = rdi_element_from_name_idx(rdi, FilePathNodes, p->next_sibling);
            break;
          }
          dumpf("%.*s}\n", (depth-1)*2, indent.str);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: dump source files
  //
  DumpSubset(SourceFiles)
  {
    U64 count = 0;
    RDI_SourceFile *v = rdi_table_from_name(rdi, SourceFiles, &count);
    U64 checksums_count[RDI_ChecksumKind_COUNT] = {0};
    RDI_U8 *checksums_data[RDI_ChecksumKind_COUNT] = {0};
    RDI_U64 checksums_element_sizes[RDI_ChecksumKind_COUNT] = {0};
    for EachEnumVal(RDI_ChecksumKind, k)
    {
      RDI_SectionKind section_kind = rdi_section_kind_from_checksum_kind(k);
      checksums_data[k] = rdi_section_raw_table_from_kind(rdi, section_kind, &checksums_count[k]);
      checksums_element_sizes[k] = rdi_section_element_size_table[section_kind];
    }
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_SourceFile *source_file = &v[idx];
      RDI_ChecksumKind checksum_kind = source_file->checksum_kind;
      RDI_U32 checksum_idx = source_file->checksum_idx;
      String8 checksum_kind_name = {0};
      switch(checksum_kind)
      {
        default:{checksum_kind_name = str8_lit("Null");}break;
#define X(name, s) case RDI_ChecksumKind_##name:{checksum_kind_name = str8_lit(#name);}break;
        RDI_ChecksumKind_XList
#undef X
      }
      String8 checksum_value = str8(checksums_data[checksum_kind] + checksums_element_sizes[checksum_kind]*checksum_idx, checksums_element_sizes[checksum_kind]);
      String8List checksum_vals = numeric_str8_list_from_data(arena, 16, checksum_value, 1);
      StringJoin join = {0};
      join.sep = str8_lit(", ");
      String8 checksum_val_string = str8_list_join(arena, &checksum_vals, &join);
      dumpf("\n  { file_path_node_idx: %4u, source_line_map: %4u, checksum_kind: %10S, checksum_value: %32S, path: %-64S } // source_file[%I64u]",
            source_file->file_path_node_idx,
            source_file->source_line_map_idx,
            checksum_kind_name,
            checksum_val_string,
            push_str8f(arena, "'%S'", str8_from_rdi_string_idx(rdi, source_file->normal_full_path_string_idx)),
            idx);
    }
    if(lane_idx() == lane_count()-1) { dumpf("\n"); }
  }
  
  //////////////////////////////
  //- rjf: dump units
  //
  DumpSubset(Units)
  {
    U64 count = 0;
    RDI_Unit *v = rdi_table_from_name(rdi, Units, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_Unit *unit = &v[idx];
      Temp scratch = scratch_begin(&arena, 1);
      dumpf("\n  // unit[%I64u]\n  {\n", idx);
      dumpf("    unit_name: '%S'\n", str8_from_rdi_string_idx(rdi, unit->unit_name_string_idx));
      dumpf("    compiler_name: '%S'\n", str8_from_rdi_string_idx(rdi, unit->compiler_name_string_idx));
      dumpf("    source_file_path: %u\n", unit->source_file_path_node);
      dumpf("    object_file_path: %u\n", unit->object_file_path_node);
      dumpf("    archive_file_path: %u\n", unit->archive_file_path_node);
      dumpf("    build_path: %u\n", unit->build_path_node);
      dumpf("    language: %S\n", rdi_string_from_language(scratch.arena, unit->language));
      dumpf("    line_table_idx: %u\n", unit->line_table_idx);
      dumpf("    procedures_idx_range: [%u, %u)\n", unit->procedures_first_idx, unit->procedures_first_idx + unit->procedures_count);
      dumpf("    global_variables_idx_range: [%u, %u)\n", unit->global_variables_first_idx, unit->global_variables_first_idx + unit->global_variables_count);
      dumpf("    thread_variables_idx_range: [%u, %u)\n", unit->thread_variables_first_idx, unit->thread_variables_first_idx + unit->thread_variables_count);
      dumpf("    constants_idx_range: [%u, %u)\n", unit->constants_first_idx, unit->constants_first_idx + unit->constants_count);
      dumpf("  }\n");
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: dump unit vmap
  //
  DumpSubset(UnitVMap)
  {
    if(lane_idx() == 0) { dumpf("\n"); }
    U64 count = 0;
    RDI_VMapEntry *v = rdi_table_from_name(rdi, UnitVMap, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      dumpf("  {0x%I64x => %I64u}\n", v[idx].voff, v[idx].idx);
    }
  }
  
  //////////////////////////////
  //- rjf: dump line tables
  //
  DumpSubset(LineTables)
  {
    U64 count = 0;
    RDI_LineTable *v = rdi_table_from_name(rdi, LineTables, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_LineTable *line_table = &v[idx];
      RDI_ParsedLineTable parsed_line_table = {0};
      rdi_parsed_from_line_table(rdi, line_table, &parsed_line_table);
      dumpf("\n  // line_table[%I64u]\n  {\n", idx);
      for EachIndex(line_idx, parsed_line_table.count)
      {
        U64         first = parsed_line_table.voffs[line_idx];
        U64         opl   = parsed_line_table.voffs[line_idx + 1];
        RDI_Line   *line  = parsed_line_table.lines + line_idx;
        RDI_Column *col   = 0;
        if(line_idx < parsed_line_table.col_count)
        {
          col = parsed_line_table.cols + line_idx;
        }
        if(col == 0)
        {
          dumpf("    { [0x%08I64x, 0x%08I64x) file: %u, line: %u }\n", first, opl, line->file_idx, line->line_num);
        }
        else
        {
          dumpf("    { [0x%08I64x, 0x%08I64x) file: %u, line: %u, columns: [%u, %u) }\n", first, opl, line->file_idx, line->line_num, col->col_first, col->col_opl);
        }
      }
      dumpf("  }\n");
    }
  }
  
  //////////////////////////////
  //- rjf: dump source line maps
  //
  DumpSubset(SourceLineMaps)
  {
    U64 count = 0;
    RDI_SourceLineMap *v = rdi_table_from_name(rdi, SourceLineMaps, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      Temp scratch = scratch_begin(&arena, 1);
      RDI_ParsedSourceLineMap line_map = {0};
      rdi_parsed_from_source_line_map(rdi, &v[idx], &line_map);
      dumpf("\n  // source_line_map[%I64u]\n  {\n", idx);
      for EachIndex(line_num_idx, line_map.count)
      {
        Temp temp = temp_begin(scratch.arena);
        String8List list = {0};
        U32 voff_lo = line_map.ranges[line_num_idx];
        U32 voff_hi = ClampTop(line_map.ranges[line_num_idx + 1], line_map.voff_count);
        for(U64 voff_idx = voff_lo; voff_idx < voff_hi; voff_idx += 1)
        {
          str8_list_pushf(temp.arena, &list, "%#llx", line_map.voffs[voff_idx]);
        }
        String8 voffs_string = str8_list_join(temp.arena, &list, &(StringJoin){.sep = str8_lit(", ")});
        dumpf("    %u: (%S)\n", line_map.nums[line_num_idx], voffs_string);
        temp_end(temp);
      }
      dumpf("  }\n");
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: dump type nodes
  //
  DumpSubset(TypeNodes)
  {
    U64 count = 0;
    RDI_TypeNode *v = rdi_table_from_name(rdi, TypeNodes, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      Temp scratch = scratch_begin(&arena, 1);
      RDI_TypeNode *type = &v[idx];
      String8 type_kind_str = {0};
      type_kind_str.str = rdi_string_from_type_kind(type->kind, &type_kind_str.size);
      dumpf("\n  // type[%I64u]\n  {\n", idx);
      dumpf("    kind: %S\n", type_kind_str);
      if(type->kind == RDI_TypeKind_Modifier)
      {
        dumpf("    flags: %S\n", rdi_string_from_type_modifier_flags(scratch.arena, type->flags));
      }
      else if(type->flags != 0)
      {
        dumpf("    flags: %#x (missing stringizer path)\n", type->flags);
      }
      dumpf("    byte_size: %u\n", type->byte_size);
      if(RDI_TypeKind_FirstBuiltIn <= type->kind && type->kind <= RDI_TypeKind_LastBuiltIn)
      {
        dumpf("    name: '%S'\n", str8_from_rdi_string_idx(rdi, type->built_in.name_string_idx));
      }
      else if(type->kind == RDI_TypeKind_Array)
      {
        dumpf("    constructed__direct_type: %u\n", type->constructed.direct_type_idx);
        dumpf("    constructed__array_count: %u\n", type->constructed.count);
      }
      else if(type->kind == RDI_TypeKind_Function)
      {
        U32  param_idx_count = 0;
        U32 *param_idx_array = rdi_idx_run_from_first_count(rdi, type->constructed.param_idx_run_first, type->constructed.count, &param_idx_count);
        String8List param_idx_strings = {0};
        for(U32 param_idx = 0; param_idx < param_idx_count; param_idx += 1)
        {
          str8_list_pushf(scratch.arena, &param_idx_strings, "%u", param_idx_array[param_idx]);
        }
        String8 param_idx_str = str8_list_join(scratch.arena, &param_idx_strings, &(StringJoin){.pre = str8_lit("["), .sep = str8_lit(", "), .post = str8_lit("]")});
        dumpf("    constructed__params: %S // idx_run[%u]\n", param_idx_str, type->constructed.param_idx_run_first);
        dumpf("    return_type: %u\n", type->constructed.direct_type_idx);
      }
      else if(type->kind == RDI_TypeKind_Method)
      {
        U32  param_idx_count = 0;
        U32 *param_idx_array = rdi_idx_run_from_first_count(rdi, type->constructed.param_idx_run_first, type->constructed.count, &param_idx_count);
        String8 this_type_str = str8_lit("\?\?\?");
        if(param_idx_count > 0)
        {
          this_type_str = push_str8f(scratch.arena, "%u", param_idx_array[0]);
          param_idx_count -= 1;
          param_idx_array += 1;
        }
        String8List param_idx_strings = {0};
        for(U32 param_idx = 0; param_idx < param_idx_count; param_idx += 1)
        {
          str8_list_pushf(scratch.arena, &param_idx_strings, "%u", param_idx_array[param_idx]);
        }
        String8 param_idx_str = str8_list_join(scratch.arena, &param_idx_strings, &(StringJoin){.pre = str8_lit("["), .sep = str8_lit(", "), .post = str8_lit("]")});
        dumpf("    constructed__this_type: %S // idx_run[%u]\n", this_type_str, type->constructed.param_idx_run_first);
        dumpf("    constructed__params: %S // idx_run[%u]\n", param_idx_str, type->constructed.param_idx_run_first);
        dumpf("    return_type: %u\n", type->constructed.direct_type_idx);
      }
      else if(RDI_TypeKind_FirstConstructed <= type->kind && type->kind <= RDI_TypeKind_LastConstructed)
      {
        dumpf("    constructed__direct_type: %u\n", type->constructed.direct_type_idx);
      }
      else if(RDI_TypeKind_FirstUserDefined <= type->kind && type->kind <= RDI_TypeKind_LastUserDefined)
      {
        dumpf("    name: '%S'\n", str8_from_rdi_string_idx(rdi, type->user_defined.name_string_idx));
        dumpf("    user_defined__direct_type: %u\n",   type->user_defined.direct_type_idx);
        dumpf("    user_defined__udt: %u\n",   type->user_defined.udt_idx);
      }
      else if(type->kind == RDI_TypeKind_Bitfield)
      {
        dumpf("    bitfield__off: %u\n", type->bitfield.off);
        dumpf("    bitfield__size: %u\n", type->bitfield.size);
      }
      dumpf("  }\n");
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: dump user defined types
  //
  DumpSubset(UserDefinedTypes)
  {
    U64 count = 0;
    RDI_UDT *v = rdi_table_from_name(rdi, UDTs, &count);
    U64 all_members_count = 0;
    RDI_Member *all_members = rdi_table_from_name(rdi, Members, &all_members_count);
    U64 all_enum_members_count = 0;
    RDI_EnumMember *all_enum_members = rdi_table_from_name(rdi, EnumMembers, &all_enum_members_count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_UDT *udt = &v[idx];
      Temp scratch = scratch_begin(&arena, 1);
      dumpf("\n  // udt[%I64u]\n  {\n", idx);
      dumpf("    self_type: %u\n", udt->self_type_idx);
      dumpf("    flags: `%S`\n", rdi_string_from_udt_flags(scratch.arena, udt->flags));
      if(udt->container_idx != 0 && (udt->container_flags & RDI_ContainerFlag_KindMask) == RDI_ContainerKind_Type)
      {
        dumpf("    container_type_idx: %u\n", udt->container_idx);
      }
      if(udt->container_idx != 0 && (udt->container_flags & RDI_ContainerFlag_KindMask) == RDI_ContainerKind_Scope)
      {
        dumpf("    container_scope_idx: %u\n", udt->container_idx);
      }
      if(udt->container_idx != 0 && (udt->container_flags & RDI_ContainerFlag_KindMask) == RDI_ContainerKind_Namespace)
      {
        dumpf("    container_namespace_idx: %u\n", udt->container_idx);
      }
      if(udt->file_idx != 0)
      {
        dumpf("    loc: {file: %u, line: %u, col: %u}\n", udt->file_idx, udt->line, udt->col);
      }
      if(udt->flags & RDI_UDTFlag_EnumMembers)
      {
        U32 member_hi = ClampTop(udt->member_first + udt->member_count, all_enum_members_count);
        U32 member_lo = ClampTop(udt->member_first, member_hi);
        if(member_lo < member_hi)
        {
          dumpf("    enum_members:\n");
          dumpf("    {\n");
          for(U32 enum_member_idx = member_lo; enum_member_idx < member_hi; enum_member_idx += 1)
          {
            RDI_EnumMember *enum_member = &all_enum_members[enum_member_idx];
            dumpf("      '%S': %I64u\n", str8_from_rdi_string_idx(rdi, enum_member->name_string_idx), enum_member->val);
          }
          dumpf("    }\n");
        }
      }
      else
      {
        U32 member_hi = ClampTop(udt->member_first + udt->member_count, all_members_count);
        U32 member_lo = ClampTop(udt->member_first, member_hi);
        if(member_lo < member_hi)
        {
          dumpf("    members:\n");
          dumpf("    {\n");
          for(U32 member_idx = member_lo; member_idx < member_hi; member_idx += 1)
          {
            RDI_Member *member = &all_members[member_idx];
            String8 kind_str = rdi_string_from_member_kind(scratch.arena, member->kind);
            String8 name_str = str8_from_rdi_string_idx(rdi, member->name_string_idx);
            dumpf("      '%S': { kind: %S, type: %u, off: %u }\n", name_str, kind_str, member->type_idx, member->off);
          }
          dumpf("    }\n");
        }
      }
      dumpf("  }\n");
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: dump global variables vmap
  //
  DumpSubset(GlobalVariablesVMap)
  {
    if(lane_idx() == 0) { dumpf("\n"); }
    U64 count = 0;
    RDI_VMapEntry *v = rdi_table_from_name(rdi, GlobalVMap, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      dumpf("  {0x%I64x => %I64u}\n", v[idx].voff, v[idx].idx);
    }
  }
  
  //////////////////////////////
  //- rjf: dump symbols
  //
  struct
  {
    RDI_DumpSubset subset;
    RDI_SectionKind section;
  }
  symbol_tables[] =
  {
    {RDI_DumpSubset_Procedures,      RDI_SectionKind_Procedures},
    {RDI_DumpSubset_GlobalVariables, RDI_SectionKind_GlobalVariables},
    {RDI_DumpSubset_ThreadVariables, RDI_SectionKind_ThreadVariables},
    {RDI_DumpSubset_LocalVariables,  RDI_SectionKind_LocalVariables},
    {RDI_DumpSubset_Constants,       RDI_SectionKind_Constants},
  };
  for EachElement(symbol_table_idx, symbol_tables)
  {
    U64 all_bytecode_size = 0;
    RDI_U8 *all_bytecode = rdi_table_from_name(rdi, LocationsBytecodeData, &all_bytecode_size);
    DumpSubsetKind(symbol_tables[symbol_table_idx].subset)
    {
      String8 table_name = rdi_name_lowercase_from_dump_subset_table[symbol_tables[symbol_table_idx].subset];
      RDI_TopLevelInfo *tli = rdi_element_from_name_idx(rdi, TopLevelInfo, 0);
      U64 count = 0;
      RDI_Symbol *v = (RDI_Symbol *)rdi_section_raw_table_from_kind(rdi, symbol_tables[symbol_table_idx].section, &count);
      Rng1U64 range = lane_range(count);
      for EachInRange(idx, range)
      {
        RDI_Symbol *symbol = &v[idx];
        Temp scratch = scratch_begin(&arena, 1);
        dumpf("\n  '%S': // %S[%I64u]\n  {\n", str8_from_rdi_string_idx(rdi, symbol->name_string_idx), table_name, idx);
        dumpf("    type_idx: %u\n", symbol->type_idx);
        if(symbol->link_name_string_idx != 0)
        {
          dumpf("    link_name: '%S'\n", str8_from_rdi_string_idx(rdi, symbol->link_name_string_idx));
        }
        if(symbol->root_scope_idx != 0)
        {
          dumpf("    root_scope_idx: %u\n", symbol->root_scope_idx);
        }
        if(symbol->container_idx != 0 && (symbol->container_flags & RDI_ContainerFlag_KindMask) == RDI_ContainerKind_Type)
        {
          dumpf("    container_type_idx: %u\n", symbol->container_idx);
        }
        if(symbol->container_idx != 0 && (symbol->container_flags & RDI_ContainerFlag_KindMask) == RDI_ContainerKind_Scope)
        {
          dumpf("    container_scope_idx: %u\n", symbol->container_idx);
        }
        if(symbol->container_idx != 0 && (symbol->container_flags & RDI_ContainerFlag_KindMask) == RDI_ContainerKind_Namespace)
        {
          dumpf("    container_namespace_idx: %u\n", symbol->container_idx);
        }
        if(rdi_kind_from_location(symbol->location) != RDI_LocationKind_NULL)
        {
          RDI_Location *locations = &symbol->location;
          RDI_LocationSetElement *locations_set_elements = 0;
          U64 locations_count = 1;
          U64 stride = sizeof(RDI_LocationSetElement);
          if(rdi_kind_from_location(symbol->location) == RDI_LocationKind_Set)
          {
            U64 set_element_first_idx = rdi_set_first_index_from_location(symbol->location);
            U64 set_element_count = rdi_set_count_from_location(symbol->location);
            locations_set_elements = rdi_element_from_name_idx(rdi, LocationsSetElements, set_element_first_idx);
            locations = &locations_set_elements[0].location;
            locations_count = set_element_count;
          }
          for EachIndex(location_idx, locations_count)
          {
            RDI_Location location = *(RDI_Location *)((RDI_U8 *)locations + stride*location_idx);
            Rng1U64 location_vaddr_range = r1u64(0, max_U64);
            if(locations_set_elements)
            {
              location_vaddr_range = r1u64(locations_set_elements[location_idx].voff_first, locations_set_elements[location_idx].voff_opl);
            }
            String8 location_kind_string = {0};
            if(0){}
#define X(n) else if(rdi_kind_from_location(location) == RDI_LocationKind_##n) { location_kind_string = str8_lit(#n); }
            RDI_LocationKind_XList
#undef X
            dumpf("    location:\n");
            dumpf("    {\n");
            dumpf("      kind: %S\n", location_kind_string);
            if(location_vaddr_range.min == 0 && location_vaddr_range.max == max_U64)
            {
              dumpf("      range: *always*\n");
            }
            else
            {
              dumpf("      range: [0x%I64x, 0x%I64x)\n", location_vaddr_range.min, location_vaddr_range.max);
            }
            switch((RDI_LocationKindEnum)rdi_kind_from_location(location))
            {
              case RDI_LocationKind_NULL:
              {
                dumpf("      *invalid* // null location kind\n");
              }break;
              case RDI_LocationKind_Set:
              {
                dumpf("      *invalid* // set inside of set\n");
              }break;
              case RDI_LocationKind_AddrBytecodeStream:
              case RDI_LocationKind_ValBytecodeStream:
              {
                U64 bytecode_data_off = rdi_bytecode_data_off_from_location(location);
                U64 bytecode_data_opl = all_bytecode_size;
                RDI_U8 *bytecode_ptr = rdi_element_from_name_idx(rdi, LocationsBytecodeData, bytecode_data_off);
                String8 bytecode = str8(bytecode_ptr, bytecode_data_opl - bytecode_data_off);
                String8 bytecode_stringification = rdi_string_from_bytecode(scratch.arena, tli->arch, bytecode);
                dumpf("      bytecode: { %S }\n", bytecode_stringification);
              }break;
              case RDI_LocationKind_AddrRegPlusU16:
              case RDI_LocationKind_AddrAddrRegPlusU16:
              {
                RDI_RegCode reg_code = rdi_regcode_from_location(location);
                U64 reg_off = rdi_regoff_from_location(location);
                String8 reg_code_string = rdi_string_from_reg_code(scratch.arena, tli->arch, reg_code);
                dumpf("      reg: %S\n", reg_code_string);
                dumpf("      reg_off: 0x%I64x\n", reg_off);
              }break;
              case RDI_LocationKind_ValReg:
              {
                RDI_RegCode reg_code = rdi_regcode_from_location(location);
                String8 reg_code_string = rdi_string_from_reg_code(scratch.arena, tli->arch, reg_code);
                dumpf("      reg: %S\n", reg_code_string);
              }break;
              case RDI_LocationKind_ModuleOff:
              {
                U64 off = rdi_voff_from_location(location);
                dumpf("      voff: 0x%I64x\n", off);
              }break;
              case RDI_LocationKind_TLSOff:
              {
                U64 off = rdi_toff_from_location(location);
                dumpf("      toff: 0x%I64x\n", off);
              }break;
              case RDI_LocationKind_ConstantDataOff:
              {
                U64 off = rdi_constant_data_off_from_location(location);
                dumpf("      constant_data_off: 0x%I64x\n", off);
              }break;
            }
            dumpf("    }\n");
          }
        }
        dumpf("  }\n");
        scratch_end(scratch);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: dump scopes
  //
  DumpSubset(Scopes)
  {
    if(lane_idx() == 0) { dumpf("\n"); }
    RDI_TopLevelInfo *tli = rdi_element_from_name_idx(rdi, TopLevelInfo, 0);
    U64 scope_voffs_count = 0;
    U64 *scope_voffs = rdi_table_from_name(rdi, ScopeVOffData,  &scope_voffs_count);
    U64 locals_count = 0;
    RDI_Symbol *locals = rdi_table_from_name(rdi, LocalVariables, &locals_count);
    U64 count = 0;
    RDI_Scope *v = rdi_table_from_name(rdi, Scopes, &count);
    RDI_Scope *nil = &v[0];
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      if(v[idx].parent_scope_idx != 0) { continue; }
      RDI_Scope *root = &v[idx];
      S64 depth = 1;
      for(RDI_Scope *scope = root, *rec_next = nil; scope != nil; scope = rec_next)
      {
        // rjf: scope list(voff_range) => string
        String8 voff_range_list_string = {0};
        {
          U32 voff_range_lo    = ClampTop(scope->voff_range_first, scope_voffs_count);
          U32 voff_range_hi    = ClampTop(scope->voff_range_opl,   scope_voffs_count);
          U32 voff_range_count = (voff_range_hi - voff_range_lo);
          U64 *voff_ptr         = scope_voffs + voff_range_lo;
          Temp scratch = scratch_begin(&arena, 1);
          String8List list = {0};
          for(U64 i = 0; i+2 <= voff_range_count; i += 2)
          {
            str8_list_pushf(scratch.arena, &list, "[%#llx, %#llx)", voff_ptr[i+0], voff_ptr[i+1]);
          }
          voff_range_list_string = str8_list_join(arena, &list, &(StringJoin){.sep = str8_lit(", ")});
          scratch_end(scratch);
        }
        
        // rjf: scope procedure -> name
        String8 procedure_name = str8_from_rdi_string_idx(rdi, rdi_element_from_name_idx(rdi, Procedures, scope->proc_idx)->name_string_idx);
        if(procedure_name.size == 0)
        {
          procedure_name = str8_lit("???");
        }
        
        // rjf: scope inline site -> name
        String8 inline_site_name = str8_from_rdi_string_idx(rdi, rdi_element_from_name_idx(rdi, InlineSites, scope->inline_site_idx)->name_string_idx);
        if(inline_site_name.size == 0)
        {
          inline_site_name = str8_lit("???");
        }
        
        // rjf: dump
        dumpf("%.*s// scope[%I64u]\n", depth*2, indent.str, (U64)(scope - v));
        dumpf("%.*s{\n", depth*2, indent.str);
        dumpf("%.*s  proc_idx: %u // %S\n", depth*2, indent.str, scope->proc_idx, procedure_name);
        dumpf("%.*s  first_child_scope_idx: %u\n", depth*2, indent.str, scope->first_child_scope_idx);
        dumpf("%.*s  next_sibling_scope_idx: %u\n", depth*2, indent.str, scope->next_sibling_scope_idx);
        if(scope->inline_site_idx != 0)
        {
          dumpf("%.*s  inline_site_idx: %u ('%S')\n", depth*2, indent.str, scope->inline_site_idx, inline_site_name);
        }
        dumpf("%.*s  voff_ranges: %S\n", depth*2, indent.str, voff_range_list_string);
        dumpf("%.*s  locals:\n", depth*2, indent.str);
        dumpf("%.*s  {\n", depth*2, indent.str);
        {
          U32 local_lo = ClampTop(scope->local_first, locals_count);
          U32 local_hi = ClampTop(local_lo + scope->local_count, locals_count);
          if(local_lo < local_hi)
          {
            for(U32 local_idx = local_lo; local_idx < local_hi; local_idx += 1)
            {
              Temp scratch = scratch_begin(&arena, 1);
              RDI_Symbol *local_ptr = &locals[local_idx];
              dumpf("%.*s    '%S': {type_idx: %u} // local_variable #%u\n", depth*2, indent.str, str8_from_rdi_string_idx(rdi, local_ptr->name_string_idx), local_ptr->type_idx, local_idx);
              scratch_end(scratch);
            }
          }
        }
        dumpf("%.*s  }\n", depth*2, indent.str);
        
        // rjf: get next recursion
        rec_next = nil;
        if(scope->first_child_scope_idx)
        {
          rec_next = rdi_element_from_name_idx(rdi, Scopes, scope->first_child_scope_idx);
          depth += 1;
        }
        else for(RDI_Scope *p = scope;
                 p != nil;
                 p = rdi_element_from_name_idx(rdi, Scopes, p->parent_scope_idx), depth -= 1)
        {
          dumpf("%.*s} // scope[/%I64u] \n", depth*2, indent.str, (U64)(p-v));
          if(p->next_sibling_scope_idx != 0)
          {
            rec_next = rdi_element_from_name_idx(rdi, Scopes, p->next_sibling_scope_idx);
            break;
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: dump scope vmap
  //
  DumpSubset(ScopeVMap)
  {
    if(lane_idx() == 0) { dumpf("\n"); }
    U64 count = 0;
    RDI_VMapEntry *v = rdi_table_from_name(rdi, ScopeVMap, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      dumpf("  {0x%I64x => %I64u}\n", v[idx].voff, v[idx].idx);
    }
  }
  
  //////////////////////////////
  //- rjf: dump inline sites
  //
  DumpSubset(InlineSites)
  {
    U64 count = 0;
    RDI_InlineSite *v = rdi_table_from_name(rdi, InlineSites, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_InlineSite *inline_site = &v[idx];
      Temp scratch = scratch_begin(&arena, 1);
      String8 inline_site_idx = push_str8f(scratch.arena, "inline_site[%u]",      idx);
      String8 type_idx        = push_str8f(scratch.arena, "type_idx: %u,",       inline_site->type_idx);
      String8 owner_type_idx  = push_str8f(scratch.arena, "owner_type_idx: %u,", inline_site->owner_type_idx);
      String8 line_table_idx  = push_str8f(scratch.arena, "line_table_idx: %u,", inline_site->line_table_idx);
      String8 name            = push_str8f(scratch.arena, "'%S'", str8_from_rdi_string_idx(rdi, inline_site->name_string_idx));
      dumpf("\n  { %-25S %-25S %-25S name: %-64S } // %S",
            type_idx,
            owner_type_idx,
            line_table_idx,
            name,
            inline_site_idx);
      scratch_end(scratch);
    }
    if(lane_idx() == lane_count()-1) { dumpf("\n"); }
  }
  
  //////////////////////////////
  //- rjf: dump name maps
  //
  DumpSubset(NameMaps)
  {
    Temp scratch = scratch_begin(&arena, 1);
    U64 count = 0;
    RDI_NameMap *v = rdi_table_from_name(rdi, NameMaps, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_ParsedNameMap name_map = {0};
      rdi_parsed_from_name_map(rdi, &v[idx], &name_map);
      dumpf("\n  // name_map[%I64u] (%S)\n  {\n", idx, rdi_string_from_name_map_kind(idx));
      for EachIndex(bucket_idx, name_map.bucket_count)
      {
        if(name_map.buckets[bucket_idx].node_count == 0) { continue; }
        dumpf("    %I64u:\n    {\n", bucket_idx, bucket_idx);
        RDI_NameMapNode *node_ptr = name_map.nodes + name_map.buckets[bucket_idx].first_node;
        RDI_NameMapNode *node_opl = node_ptr + name_map.buckets[bucket_idx].node_count;
        for(;node_ptr < node_opl; node_ptr += 1)
        {
          Temp temp = temp_begin(scratch.arena);
          String8 str = str8_from_rdi_string_idx(rdi, node_ptr->string_idx);
          String8 indices = {0};
          if(node_ptr->match_count == 1)
          {
            indices = push_str8f(temp.arena, "%u", node_ptr->match_idx_or_idx_run_first);
          }
          else
          {
            U32  idx_count = 0;
            U32 *idx_array = rdi_idx_run_from_first_count(rdi, node_ptr->match_idx_or_idx_run_first, node_ptr->match_count, &idx_count);
            String8List idx_strings = {0};
            for(U32 idx_i = 0; idx_i < idx_count; idx_i += 1)
            {
              U32 idx = idx_array[idx_i];
              str8_list_pushf(temp.arena, &idx_strings, "%u", idx);
            }
            String8 extra = push_str8f(temp.arena, " // idx_run[%u]", node_ptr->match_idx_or_idx_run_first);
            indices = str8_list_join(scratch.arena, &idx_strings, &(StringJoin){.sep = str8_lit(", "), .post = extra});
          }
          dumpf("      \"%S\": %S\n", str, indices);
          temp_end(temp);
        }
        dumpf("    }\n");
      }
      dumpf("  }\n");
    }
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: dump namespaces
  //
  DumpSubset(Namespaces)
  {
    U64 count = 0;
    RDI_Namespace *v = rdi_table_from_name(rdi, Namespaces, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_Namespace *ns = &v[idx];
      String8 container_kind_name = str8_lit("container_idx");
      switch(ns->container_flags & RDI_ContainerFlag_KindMask)
      {
        default:{}break;
        case RDI_ContainerKind_Type:
        {
          container_kind_name = str8_lit("container_udt_idx");
        }break;
        case RDI_ContainerKind_Namespace:
        {
          container_kind_name = str8_lit("container_namespace_idx");
        }break;
        case RDI_ContainerKind_Scope:
        {
          container_kind_name = str8_lit("container_scope_idx");
        }break;
      }
      dumpf("\n  '%S': {%S: %u} // namespaces[%I64u]", str8_from_rdi_string_idx(rdi, ns->name_string_idx), container_kind_name, ns->container_idx, idx);
    }
    if(lane_idx() == lane_count()-1) { dumpf("\n"); }
  }
  
  //////////////////////////////
  //- rjf: dump strings
  //
  DumpSubset(Strings)
  {
    U64 count = 0;
    U32 *v = rdi_table_from_name(rdi, StringTable, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      dumpf("\n  \"%S\" // string[%I64u]", str8_from_rdi_string_idx(rdi, idx), idx);
    }
    if(lane_idx() == lane_count()-1) { dumpf("\n"); }
  }
  
  //////////////////////////////
  //- rjf: join results
  //
  lane_sync();
  if(lane_idx() == 0)
  {
    for EachNode(n, DumpSubsetOutputNode, first_output_node)
    {
      String8List subset_strings = {0};
      for EachIndex(idx, lane_count())
      {
        str8_list_concat_in_place(&subset_strings, &n->lane_strings[idx]);
      }
      if(subset_strings.total_size != 0)
      {
        str8_list_pushf(arena, &result_strings, "////////////////////////////////\n//~ %S\n\n%S:\n{", rdi_name_title_from_dump_subset_table[n->subset], rdi_name_lowercase_from_dump_subset_table[n->subset]);
        str8_list_concat_in_place(&result_strings, &subset_strings);
        str8_list_pushf(arena, &result_strings, "}\n\n");
      }
    }
  }
  lane_sync();
  
#undef DumpSubset
#undef dumpf
#undef dump
  scratch_end(scratch);
  ProfEnd();
  return result_strings;
}

#if SUBPROGRAM_CONVERSION_TEST
internal String8
rdi_string_from_type(Arena *arena, RDI_Parsed *rdi, RDI_Symbol *proc, RDI_TypeNode *type)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8List fmt     = {0};
  String8List arr_fmt = {0};
  
  for (RDI_TypeNode *i = type, *n = 0; i != 0; i = n, n = 0) {
    if (RDI_TypeKind_FirstConstructed <= i->kind && i->kind <= RDI_TypeKind_LastConstructed) {
      n = rdi_element_from_name_idx(rdi, TypeNodes, i->constructed.direct_type_idx);
    }
    
    if (i->kind == RDI_TypeKind_Variadic) {
      str8_list_push_frontf(scratch.arena, &fmt, "...");
    } else if (RDI_TypeKind_FirstBuiltIn <= i->kind && i->kind <= RDI_TypeKind_LastBuiltIn) {
      String8 built_in_name = str8_from_rdi_string_idx(rdi, i->built_in.name_string_idx);
      str8_list_push_front(scratch.arena, &fmt, built_in_name);
    } else if (i->kind == RDI_TypeKind_Alias) {
      String8 alias_name = str8_from_rdi_string_idx(rdi, i->user_defined.name_string_idx);
      str8_list_push_front(scratch.arena, &fmt, alias_name);
    } else if (i->kind == RDI_TypeKind_Modifier) {
      for EachBit(f, i->flags) {
        str8_list_push_front(scratch.arena, &fmt, rdi_string_from_type_modifier_flags(scratch.arena, f));
      }
    } else if (i->kind == RDI_TypeKind_Ptr) {
      str8_list_push_front(scratch.arena, &fmt, str8_lit("*"));
    } else if (i->kind == RDI_TypeKind_LRef) {
      str8_list_push_front(scratch.arena, &fmt, str8_lit("&"));
    } else if (i->kind == RDI_TypeKind_RRef) {
      str8_list_push_front(scratch.arena, &fmt, str8_lit("&&"));
    } else if (i->kind == RDI_TypeKind_Array) {
      str8_list_push_frontf(scratch.arena, &arr_fmt, "[%llu]", i->constructed.count);
    } else if (i->kind == RDI_TypeKind_Function) {
      String8 *param_names = 0;
      String8  proc_name   = {0};
      if (proc) {
        RDI_TypeNode *proc_type = rdi_element_from_name_idx(rdi, TypeNodes, proc->type_idx);
        if (proc_type == i) {
          proc_name   = str8_from_rdi_string_idx(rdi, proc->name_string_idx);
          param_names = push_array(scratch.arena, String8, i->constructed.count);
          RDI_Scope *root_scope = rdi_element_from_name_idx(rdi, Scopes, proc->root_scope_idx);
          U64 param_idx = 0;
          for (U64 local_idx = root_scope->local_first; local_idx < root_scope->local_first + root_scope->local_count; local_idx += 1) {
            RDI_Local *local = rdi_element_from_name_idx(rdi, Locals, local_idx);
            if (local->kind == RDI_LocalKind_Parameter) {
              AssertAlways(param_idx < i->constructed.count);
              param_names[param_idx++] = str8_from_rdi_string_idx(rdi, local->name_string_idx);
            }
          }
        }
      }
      
      // format parameters
      String8List  params_fmt  = {0};
      U32          check_count = 0;
      U32         *idx_run     = rdi_idx_run_from_first_count(rdi, i->constructed.param_idx_run_first, i->constructed.count, &check_count);
      if (check_count == type->constructed.count) {
        for EachIndex(param_idx, i->constructed.count) {
          RDI_TypeNode *param_type   = rdi_element_from_name_idx(rdi, TypeNodes, idx_run[param_idx]);
          String8       param_string = rdi_string_from_type(scratch.arena, rdi, 0, param_type);
          if (param_names) {
            str8_list_pushf(scratch.arena, &params_fmt, "%S %S", param_string, param_names[param_idx]);
          } else {
            str8_list_push(scratch.arena, &params_fmt, param_string);
          }
        }
      } else {
        str8_list_pushf(scratch.arena, &params_fmt, "???");
      }
      
      // format signature
      String8 params = str8_list_join(scratch.arena, &params_fmt, &(StringJoin){.sep=str8_lit(", ")});
      str8_list_pushf(scratch.arena, &fmt, "(* %S)(%S)", proc_name, params);
    }
    
    if (arr_fmt.node_count && i->kind != RDI_TypeKind_Array) {
      str8_list_push_frontf(scratch.arena, &fmt, "(");
      str8_list_concat_in_place(&fmt, &arr_fmt);
      str8_list_pushf(scratch.arena, &fmt, ")");
    }
  }
  
  String8 result = str8_list_join(arena, &fmt, 0);
  scratch_end(scratch);
  return result;
}
#endif
