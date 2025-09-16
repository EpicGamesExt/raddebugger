// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "lib_rdi/rdi.c"
#include "lib_rdi/rdi_parse.c"

////////////////////////////////
//~ rjf: Lookup Helpers

internal String8
str8_from_rdi_string_idx(RDI_Parsed *rdi, U32 idx)
{
  String8 result = {0};
  result.str = rdi_string_from_idx(rdi, idx, &result.size);
  return result;
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

internal String8
rdi_string_from_local_kind(Arena *arena, RDI_LocalKind v)
{
  String8 result = {0};
  switch(v)
  {
    default:{result = push_str8f(arena, "<invalid RDI_LocalKind %u>", v);}break;
#define X(name) case RDI_LocalKind_##name:{result = str8_lit(#name);}break;
    RDI_LocalKind_XList
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
rdi_string_from_reg_code_x86(U64 reg_code)
{
  String8 result = {0};
  switch(reg_code)
  {
    default:{}break;
#define X(name, value) case RDI_RegCodeX86_##name:{result = str8_lit(#name);}break;
    RDI_RegCodeX86_XList
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
    case RDI_Arch_X86:  {result = rdi_string_from_reg_code_x86(reg_code);}break;
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
rdi_string_from_link_flags(Arena *arena, RDI_LinkFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
#define X(name) if (flags & RDI_LinkFlag_##name) { flags &= ~RDI_LinkFlag_##name; str8_list_push(scratch.arena, &list, str8_lit(#name)); }
  RDI_LinkFlags_XList;
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

internal String8List
rdi_strings_from_locations(Arena *arena, RDI_Parsed *rdi, RDI_Arch arch, Rng1U64 location_block_range)
{
  String8List strings = {0};
  Temp scratch = scratch_begin(&arena, 1);
  U64 location_block_count = 0;
  U64 location_data_size   = 0;
  RDI_LocationBlock *location_block_array = rdi_table_from_name(rdi, LocationBlocks, &location_block_count);
  RDI_U8 *location_data        = rdi_table_from_name(rdi, LocationData,   &location_data_size);
  Rng1U64 location_block_range_clamped = r1u64(ClampTop(location_block_range.min, location_block_count),
                                               ClampTop(location_block_range.max, location_block_count));
  for(U64 block_idx = location_block_range_clamped.min;
      block_idx < location_block_range_clamped.max;
      block_idx +=1)
  {
    String8 qualifier = {0};
    String8 location_info = {0};
    RDI_LocationBlock *block_ptr = &location_block_array[block_idx];
    if(block_ptr->scope_off_first == 0 && block_ptr->scope_off_opl == max_U32)
    {
      qualifier = str8_lit("*always*");
    }
    else
    {
      qualifier = push_str8f(scratch.arena, "[%#08x, %#08x): ", block_ptr->scope_off_first, block_ptr->scope_off_opl);
    }
    if(block_ptr->location_data_off >= location_data_size)
    {
      location_info = push_str8f(scratch.arena, "<bad-location-data-offset %x>", block_ptr->location_data_off);
    }
    else
    {
      U8               *loc_data_opl = location_data + location_data_size;
      U8               *loc_base_ptr = location_data + block_ptr->location_data_off;
      RDI_LocationKind  kind         = *(RDI_LocationKind *)loc_base_ptr;
      switch(kind)
      {
        default:
        {
          location_info = push_str8f(scratch.arena, "\?\?\? (%u)", kind);
        }break;
        case RDI_LocationKind_AddrBytecodeStream:
        {
          String8 bc     = str8_range(loc_base_ptr + 1, loc_data_opl);
          String8 bc_str = rdi_string_from_bytecode(scratch.arena, arch, bc);
          location_info = push_str8f(scratch.arena, "AddrBytecodeStream(%S)", bc_str);
        }break;
        case RDI_LocationKind_ValBytecodeStream:
        {
          String8 bc     = str8_range(loc_base_ptr + 1, loc_data_opl);
          String8 bc_str = rdi_string_from_bytecode(scratch.arena, arch, bc);
          location_info = push_str8f(scratch.arena, "ValBytecodeStream(%S)", bc_str);
        }break;
        case RDI_LocationKind_AddrRegPlusU16:
        {
          if(loc_base_ptr + sizeof(RDI_LocationRegPlusU16) > loc_data_opl)
          {
            location_info = push_str8f(scratch.arena, "AddrRegPlusU16(\?\?\?)");
          }
          else
          {
            RDI_LocationRegPlusU16 *loc = (RDI_LocationRegPlusU16*)loc_base_ptr;
            location_info = push_str8f(scratch.arena, "AddrRegPlusU16(reg: %S, off: %u)", rdi_string_from_reg_code(scratch.arena, arch, loc->reg_code), loc->offset);
          }
        }break;
        case RDI_LocationKind_AddrAddrRegPlusU16:
        {
          if(loc_base_ptr + sizeof(RDI_LocationRegPlusU16) > loc_data_opl)
          {
            location_info = push_str8f(scratch.arena, "AddrAddrRegPlusU16(\?\?\?)");
          }
          else
          {
            RDI_LocationRegPlusU16 *loc = (RDI_LocationRegPlusU16 *)loc_base_ptr;
            location_info = push_str8f(scratch.arena, "AddrAddrRegisterPlusU16(reg: %S, off: %u)", rdi_string_from_reg_code(scratch.arena, arch, loc->reg_code), loc->offset);
          }
        }break;
        case RDI_LocationKind_ValReg:
        {
          if(loc_base_ptr + sizeof(RDI_LocationReg) > loc_data_opl)
          {
            location_info = push_str8f(scratch.arena, "ValReg(\?\?\?)");
          }
          else
          {
            RDI_LocationReg *loc = (RDI_LocationReg*)loc_base_ptr;
            location_info = push_str8f(scratch.arena, "ValReg(reg: %S)", rdi_string_from_reg_code(scratch.arena, arch, loc->reg_code));
          }
        } break;
      }
    }
    str8_list_pushf(arena, &strings, "%S: %S", qualifier, location_info);
  }
  scratch_end(scratch);
  return strings;
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
#define DumpSubset(name) \
if(lane_idx() == 0)\
{\
DumpSubsetOutputNode *n = push_array(scratch.arena, DumpSubsetOutputNode, 1);\
SLLQueuePush(first_output_node, last_output_node, n);\
n->subset = RDI_DumpSubset_##name;\
n->lane_strings = push_array(scratch.arena, String8List, lane_count());\
}\
lane_sync();\
strings = &last_output_node->lane_strings[lane_idx()];\
lane_sync(); if(flags & RDI_DumpSubsetFlag_##name) ProfScope(#name)
  
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
      dumpf("\n");
      dumpf("  arch:          %S\n",      rdi_string_from_arch(scratch.arena, tli->arch));
      dumpf("  exe_name:      '%S'\n",    str8_from_rdi_string_idx(rdi, tli->exe_name_string_idx));
      dumpf("  voff_max:      %#08llx\n", tli->voff_max);
      dumpf("  producer_name: '%S'\n",    str8_from_rdi_string_idx(rdi, tli->producer_name_string_idx));
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
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_SourceFile *source_file = &v[idx];
      dumpf("\n  { file_path_node_idx: %4u, source_line_map: %4u, path: %-192S } // source_file[%I64u]",
            source_file->file_path_node_idx,
            source_file->source_line_map_idx,
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
      dumpf("    source_file_path: %u\n",   unit->source_file_path_node);
      dumpf("    object_file_path: %u\n",   unit->object_file_path_node);
      dumpf("    archive_file_path: %u\n",   unit->archive_file_path_node);
      dumpf("    build_path: %u\n",   unit->build_path_node);
      dumpf("    language: %S\n",   rdi_string_from_language(scratch.arena, unit->language));
      dumpf("    line_table_idx: %u\n",   unit->line_table_idx);
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
  //- rjf: dump global variables
  //
  DumpSubset(GlobalVariables)
  {
    U64 count = 0;
    RDI_GlobalVariable *v = rdi_table_from_name(rdi, GlobalVariables, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_GlobalVariable *gvar = &v[idx];
      Temp scratch = scratch_begin(&arena, 1);
      dumpf("\n  '%S': // global_variable[%I64u]\n  {\n", str8_from_rdi_string_idx(rdi, gvar->name_string_idx), idx);
      dumpf("    link_flags:    `%S`\n",    rdi_string_from_link_flags(scratch.arena, gvar->link_flags));
      dumpf("    voff:          %#08x\n", gvar->voff);
      dumpf("    type_idx:      %u\n",    gvar->type_idx);
      dumpf("    container_idx: %u\n",    gvar->container_idx);
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
  //- rjf: dump thread variables
  //
  DumpSubset(ThreadVariables)
  {
    U64 count = 0;
    RDI_ThreadVariable *v = rdi_table_from_name(rdi, ThreadVariables, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_ThreadVariable *tvar = &v[idx];
      Temp scratch = scratch_begin(&arena, 1);
      dumpf("\n  '%S': // thread_variable[%I64u]\n  {\n", str8_from_rdi_string_idx(rdi, tvar->name_string_idx), idx);
      dumpf("    link_flags:    `%S`\n",    rdi_string_from_link_flags(scratch.arena, tvar->link_flags));
      dumpf("    tls_off:       %#08x\n", tvar->tls_off);
      dumpf("    type_idx:      %u\n",    tvar->type_idx);
      dumpf("    container_idx: %u\n",    tvar->container_idx);
      dumpf("  }\n");
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: dump constants
  //
  DumpSubset(Constants)
  {
    U64 count = 0;
    RDI_Constant *v = rdi_table_from_name(rdi, Constants, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_Constant *cnst = &v[idx];
      dumpf("\n  '%S': // constant[%I64u]\n  {\n", str8_from_rdi_string_idx(rdi, cnst->name_string_idx), idx);
      dumpf("    type_idx: %u\n", cnst->type_idx);
      dumpf("  }\n");
    }
  }
  
  //////////////////////////////
  //- rjf: dump procedures
  //
  DumpSubset(Procedures)
  {
    RDI_TopLevelInfo *tli = rdi_element_from_name_idx(rdi, TopLevelInfo, 0);
    U64 count = 0;
    RDI_Procedure *v = rdi_table_from_name(rdi, Procedures, &count);
    Rng1U64 range = lane_range(count);
    for EachInRange(idx, range)
    {
      RDI_Procedure *proc = &v[idx];
      Temp scratch = scratch_begin(&arena, 1);
      dumpf("\n  '%S': // procedure[%I64u]\n  {\n", str8_from_rdi_string_idx(rdi, proc->name_string_idx), idx);
      dumpf("    link_name: '%S'\n", str8_from_rdi_string_idx(rdi, proc->link_name_string_idx));
      dumpf("    link_flags: `%S`\n",   rdi_string_from_link_flags(scratch.arena, proc->link_flags));
      dumpf("    type_idx: %u\n",   proc->type_idx);
      dumpf("    root_scope_idx: %u\n",   proc->root_scope_idx);
      dumpf("    container_idx: %u\n",   proc->container_idx);
      if(proc->frame_base_location_first != 0)
      {
        String8List frame_base_location_strings = rdi_strings_from_locations(scratch.arena, rdi, tli->arch, r1u64(proc->frame_base_location_first, proc->frame_base_location_opl));
        dumpf("    frame_base: // (first: %u, opl: %u)\n", proc->frame_base_location_first, proc->frame_base_location_opl);
        dumpf("    {\n");
        for(String8Node *n = frame_base_location_strings.first; n != 0; n = n->next)
        {
          dumpf("      %S\n", n->string);
        }
        dumpf("    }\n");
      }
      dumpf("  }\n");
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: dump scopes
  //
  DumpSubset(Scopes)
  {
    dumpf("\n");
    RDI_TopLevelInfo *tli = rdi_element_from_name_idx(rdi, TopLevelInfo, 0);
    U64 scope_voffs_count = 0;
    U64 *scope_voffs = rdi_table_from_name(rdi, ScopeVOffData,  &scope_voffs_count);
    U64 locals_count = 0;
    RDI_Local *locals = rdi_table_from_name(rdi, Locals, &locals_count);
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
              RDI_Local *local_ptr = &locals[local_idx];
              dumpf("%.*s    '%S': // local[%u]\n", depth*2, indent.str, str8_from_rdi_string_idx(rdi, local_ptr->name_string_idx), local_idx);
              dumpf("%.*s    {\n", depth*2, indent.str);
              dumpf("%.*s      kind: %S\n", depth*2, indent.str, rdi_string_from_local_kind(scratch.arena, local_ptr->kind));
              dumpf("%.*s      type_idx: %u\n", depth*2, indent.str, local_ptr->type_idx);
              dumpf("%.*s      locations:\n", depth*2, indent.str);
              dumpf("%.*s      {\n", depth*2, indent.str);
              if(local_ptr->location_first < local_ptr->location_opl)
              {
                String8List locations_strings = rdi_strings_from_locations(arena, rdi, tli->arch, r1u64(local_ptr->location_first, local_ptr->location_opl));
                for(String8Node *n = locations_strings.first; n != 0; n = n->next)
                {
                  dumpf("%.*s        %S\n", depth*2, indent.str, n->string);
                }
              }
              dumpf("%.*s      }\n", depth*2, indent.str);
              dumpf("%.*s    }\n", depth*2, indent.str);
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
