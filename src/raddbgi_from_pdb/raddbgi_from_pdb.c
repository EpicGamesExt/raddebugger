// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
p2r_end_of_cplusplus_container_name(String8 str)
{
  // NOTE: This finds the index one past the last "::" contained in str.
  //       if no "::" is contained in str, then the returned index is 0.
  //       The intent is that [0,clamp_bot(0,result - 2)) gives the
  //       "container name" and [result,str.size) gives the leaf name.
  U64 result = 0;
  if(str.size >= 2)
  {
    for(U64 i = str.size; i >= 2; i -= 1)
    {
      if(str.str[i - 2] == ':' && str.str[i - 1] == ':')
      {
        result = i;
        break;
      }
    }
  }
  return(result);
}

internal U64
p2r_hash_from_voff(U64 voff)
{
  U64 hash = (voff >> 3) ^ ((7 & voff) << 6);
  return hash;
}

////////////////////////////////
//~ rjf: Command Line -> Conversion Inputs

internal P2R_ConvertIn *
p2r_convert_in_from_cmd_line(Arena *arena, CmdLine *cmdline)
{
  P2R_ConvertIn *result = push_array(arena, P2R_ConvertIn, 1);
  
  //- rjf: get input pdb
  {
    String8 input_name = cmd_line_string(cmdline, str8_lit("pdb"));
    if(input_name.size == 0)
    {
      str8_list_push(arena, &result->errors, str8_lit("Missing required parameter: '--pdb:<pdb_file>'"));
    }
    if(input_name.size > 0)
    {
      String8 input_data = os_data_from_file_path(arena, input_name);
      if(input_data.size == 0)
      {
        str8_list_pushf(arena, &result->errors, "Could not load input PDB file from '%S'", input_name);
      }
      if(input_data.size != 0)
      {
        result->input_pdb_name = input_name;
        result->input_pdb_data = input_data;
      }
    }
  }
  
  //- rjf: get input exe
  {
    String8 input_name = cmd_line_string(cmdline, str8_lit("exe"));
    if(input_name.size > 0)
    {
      String8 input_data = os_data_from_file_path(arena, input_name);
      if(input_data.size == 0)
      {
        str8_list_pushf(arena, &result->errors, "Could not load input EXE file from '%S'", input_name);
      }
      if(input_data.size != 0)
      {
        result->input_exe_name = input_name;
        result->input_exe_data = input_data;
      }
    }
  }
  
  //- rjf: get output name
  {
    result->output_name = cmd_line_string(cmdline, str8_lit("out"));
  }
  
  //- rjf: error options
  if(cmd_line_has_flag(cmdline, str8_lit("hide_errors")))
  {
    String8List vals = cmd_line_strings(cmdline, str8_lit("hide_errors"));
    
    // if no values - set all to hidden
    if(vals.node_count == 0)
    {
      B8 *ptr  = (B8*)&result->hide_errors;
      B8 *opl = ptr + sizeof(result->hide_errors);
      for(;ptr < opl; ptr += 1)
      {
        *ptr = 1;
      }
    }
    
    // for each explicit value set the corresponding flag to hidden
    for(String8Node *node = vals.first; node != 0; node = node->next)
    {
      if(str8_match(node->string, str8_lit("input"), 0))
      {
        result->hide_errors.input = 1;
      }
      else if(str8_match(node->string, str8_lit("output"), 0))
      {
        result->hide_errors.output = 1;
      }
      else if(str8_match(node->string, str8_lit("parsing"), 0))
      {
        result->hide_errors.parsing = 1;
      }
      else if(str8_match(node->string, str8_lit("converting"), 0))
      {
        result->hide_errors.converting = 1;
      }
    }
  }
  
  //- rjf: dump options
  if(cmd_line_has_flag(cmdline, str8_lit("dump")))
  {
    result->dump = 1;
    String8List vals = cmd_line_strings(cmdline, str8_lit("dump"));
    if(vals.first == 0)
    {
      B8 *ptr = &result->dump__first;
      for(;ptr < &result->dump__last; ptr += 1)
      {
        *ptr = 1;
      }
    }
    else
    {
      for(String8Node *node = vals.first; node != 0; node = node->next)
      {
        if(str8_match(node->string, str8_lit("coff_sections"), 0))
        {
          result->dump_coff_sections = 1;
        }
        else if(str8_match(node->string, str8_lit("msf"), 0))
        {
          result->dump_msf = 1;
        }
        else if(str8_match(node->string, str8_lit("sym"), 0))
        {
          result->dump_sym = 1;
        }
        else if(str8_match(node->string, str8_lit("tpi_hash"), 0))
        {
          result->dump_tpi_hash = 1;
        }
        else if(str8_match(node->string, str8_lit("leaf"), 0))
        {
          result->dump_leaf = 1;
        }
        else if(str8_match(node->string, str8_lit("c13"), 0))
        {
          result->dump_c13 = 1;
        }
        else if(str8_match(node->string, str8_lit("contributions"), 0))
        {
          result->dump_contributions = 1;
        }
        else if(str8_match(node->string, str8_lit("table_diagnostics"), 0))
        {
          result->dump_table_diagnostics = 1;
        }
      }
    }
  }
  
  return result;
}

////////////////////////////////
//~ rjf: COFF <-> RADDBGI Canonical Conversions

internal RDI_BinarySectionFlags
rdi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags)
{
  RDI_BinarySectionFlags result = 0;
  if(flags & COFF_SectionFlag_MEM_READ)
  {
    result |= RDI_BinarySectionFlag_Read;
  }
  if(flags & COFF_SectionFlag_MEM_WRITE)
  {
    result |= RDI_BinarySectionFlag_Write;
  }
  if(flags & COFF_SectionFlag_MEM_EXECUTE)
  {
    result |= RDI_BinarySectionFlag_Execute;
  }
  return(result);
}

////////////////////////////////
//~ rjf: CodeView <-> RADDBGI Canonical Conversions

internal RDI_Arch
rdi_arch_from_cv_arch(CV_Arch cv_arch)
{
  RDI_Arch result = 0;
  switch(cv_arch)
  {
    case CV_Arch_8086: result = RDI_Arch_X86; break;
    case CV_Arch_X64:  result = RDI_Arch_X64; break;
    //case CV_Arch_8080: break;
    //case CV_Arch_80286: break;
    //case CV_Arch_80386: break;
    //case CV_Arch_80486: break;
    //case CV_Arch_PENTIUM: break;
    //case CV_Arch_PENTIUMII: break;
    //case CV_Arch_PENTIUMIII: break;
    //case CV_Arch_MIPS: break;
    //case CV_Arch_MIPS16: break;
    //case CV_Arch_MIPS32: break;
    //case CV_Arch_MIPS64: break;
    //case CV_Arch_MIPSI: break;
    //case CV_Arch_MIPSII: break;
    //case CV_Arch_MIPSIII: break;
    //case CV_Arch_MIPSIV: break;
    //case CV_Arch_MIPSV: break;
    //case CV_Arch_M68000: break;
    //case CV_Arch_M68010: break;
    //case CV_Arch_M68020: break;
    //case CV_Arch_M68030: break;
    //case CV_Arch_M68040: break;
    //case CV_Arch_ALPHA: break;
    //case CV_Arch_ALPHA_21164: break;
    //case CV_Arch_ALPHA_21164A: break;
    //case CV_Arch_ALPHA_21264: break;
    //case CV_Arch_ALPHA_21364: break;
    //case CV_Arch_PPC601: break;
    //case CV_Arch_PPC603: break;
    //case CV_Arch_PPC604: break;
    //case CV_Arch_PPC620: break;
    //case CV_Arch_PPCFP: break;
    //case CV_Arch_PPCBE: break;
    //case CV_Arch_SH3: break;
    //case CV_Arch_SH3E: break;
    //case CV_Arch_SH3DSP: break;
    //case CV_Arch_SH4: break;
    //case CV_Arch_SHMEDIA: break;
    //case CV_Arch_ARM3: break;
    //case CV_Arch_ARM4: break;
    //case CV_Arch_ARM4T: break;
    //case CV_Arch_ARM5: break;
    //case CV_Arch_ARM5T: break;
    //case CV_Arch_ARM6: break;
    //case CV_Arch_ARM_XMAC: break;
    //case CV_Arch_ARM_WMMX: break;
    //case CV_Arch_ARM7: break;
    //case CV_Arch_OMNI: break;
    //case CV_Arch_IA64_1: break;
    //case CV_Arch_IA64_2: break;
    //case CV_Arch_CEE: break;
    //case CV_Arch_AM33: break;
    //case CV_Arch_M32R: break;
    //case CV_Arch_TRICORE: break;
    //case CV_Arch_EBC: break;
    //case CV_Arch_THUMB: break;
    //case CV_Arch_ARMNT: break;
    //case CV_Arch_ARM64: break;
    //case CV_Arch_D3D11_SHADER: break;
  }
  return(result);
}

internal RDI_RegisterCode
rdi_reg_code_from_cv_reg_code(RDI_Arch arch, CV_Reg reg_code)
{
  RDI_RegisterCode result = 0;
  switch(arch)
  {
    case RDI_Arch_X86:
    {
      switch(reg_code)
      {
#define X(CVN,C,RDN,BP,BZ) case C: result = RDI_RegisterCode_X86_##RDN; break;
        CV_Reg_X86_XList(X)
#undef X
      }
    }break;
    case RDI_Arch_X64:
    {
      switch(reg_code)
      {
#define X(CVN,C,RDN,BP,BZ) case C: result = RDI_RegisterCode_X64_##RDN; break;
        CV_Reg_X64_XList(X)
#undef X
      }
    }break;
  }
  return(result);
}

internal RDI_Language
rdi_language_from_cv_language(CV_Language cv_language)
{
  RDI_Language result = 0;
  switch(cv_language)
  {
    case CV_Language_C:       result = RDI_Language_C; break;
    case CV_Language_CXX:     result = RDI_Language_CPlusPlus; break;
    //case CV_Language_FORTRAN: result = ; break;
    //case CV_Language_MASM:    result = ; break;
    //case CV_Language_PASCAL:  result = ; break;
    //case CV_Language_BASIC:   result = ; break;
    //case CV_Language_COBOL:   result = ; break;
    //case CV_Language_LINK:    result = ; break;
    //case CV_Language_CVTRES:  result = ; break;
    //case CV_Language_CVTPGD:  result = ; break;
    //case CV_Language_CSHARP:  result = ; break;
    //case CV_Language_VB:      result = ; break;
    //case CV_Language_ILASM:   result = ; break;
    //case CV_Language_JAVA:    result = ; break;
    //case CV_Language_JSCRIPT: result = ; break;
    //case CV_Language_MSIL:    result = ; break;
    //case CV_Language_HLSL:    result = ; break;
  }
  return(result);
}

internal RDI_TypeKind
rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch(basic_type)
  {
    case CV_BasicType_VOID: {result = RDI_TypeKind_Void;}break;
    case CV_BasicType_HRESULT: {result = RDI_TypeKind_Handle;}break;
    
    case CV_BasicType_RCHAR:
    case CV_BasicType_CHAR:
    case CV_BasicType_CHAR8:
    {result = RDI_TypeKind_Char8;}break;
    
    case CV_BasicType_UCHAR: {result = RDI_TypeKind_UChar8;}break;
    case CV_BasicType_WCHAR: {result = RDI_TypeKind_UChar16;}break;
    case CV_BasicType_CHAR16: {result = RDI_TypeKind_Char16;}break;
    case CV_BasicType_CHAR32: {result = RDI_TypeKind_Char32;}break;
    
    case CV_BasicType_BOOL8:
    case CV_BasicType_INT8:
    {result = RDI_TypeKind_S8;}break;
    
    case CV_BasicType_BOOL16:
    case CV_BasicType_INT16:
    case CV_BasicType_SHORT:
    {result = RDI_TypeKind_S16;}break;
    
    case CV_BasicType_BOOL32:
    case CV_BasicType_INT32:
    case CV_BasicType_LONG:
    {result = RDI_TypeKind_S32;}break;
    
    case CV_BasicType_BOOL64:
    case CV_BasicType_INT64:
    case CV_BasicType_QUAD:
    {result = RDI_TypeKind_S64;}break;
    
    case CV_BasicType_INT128:
    case CV_BasicType_OCT:
    {result = RDI_TypeKind_S128;}break;
    
    case CV_BasicType_UINT8: {result = RDI_TypeKind_U8;}break;
    
    case CV_BasicType_UINT16:
    case CV_BasicType_USHORT:
    {result = RDI_TypeKind_U16;}break;
    
    case CV_BasicType_UINT32:
    case CV_BasicType_ULONG:
    {result = RDI_TypeKind_U32;}break;
    
    case CV_BasicType_UINT64:
    case CV_BasicType_UQUAD:
    {result = RDI_TypeKind_U64;}break;
    
    case CV_BasicType_UINT128:
    case CV_BasicType_UOCT:
    {result = RDI_TypeKind_U128;}break;
    
    case CV_BasicType_FLOAT16:{result = RDI_TypeKind_F16;}break;
    case CV_BasicType_FLOAT32:{result = RDI_TypeKind_F32;}break;
    case CV_BasicType_FLOAT32PP:{result = RDI_TypeKind_F32PP;}break;
    case CV_BasicType_FLOAT48:{result = RDI_TypeKind_F48;}break;
    case CV_BasicType_FLOAT64:{result = RDI_TypeKind_F64;}break;
    case CV_BasicType_FLOAT80:{result = RDI_TypeKind_F80;}break;
    case CV_BasicType_FLOAT128:{result = RDI_TypeKind_F128;}break;
    case CV_BasicType_COMPLEX32:{result = RDI_TypeKind_ComplexF32;}break;
    case CV_BasicType_COMPLEX64:{result = RDI_TypeKind_ComplexF64;}break;
    case CV_BasicType_COMPLEX80:{result = RDI_TypeKind_ComplexF80;}break;
    case CV_BasicType_COMPLEX128:{result = RDI_TypeKind_ComplexF128;}break;
    case CV_BasicType_PTR:{result = RDI_TypeKind_Handle;}break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Location Info Building Helpers

internal RDIM_Location *
p2r_location_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegisterCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection)
{
  RDIM_Location *result = 0;
  if(0 <= offset && offset <= (S64)max_U16)
  {
    if(extra_indirection)
    {
      result = rdim_push_location_addr_addr_reg_plus_u16(arena, reg_code, (U16)offset);
    }
    else
    {
      result = rdim_push_location_addr_reg_plus_u16(arena, reg_code, (U16)offset);
    }
  }
  else
  {
    RDIM_EvalBytecode bytecode = {0};
    U32 regread_param = RDI_EncodeRegReadParam(reg_code, reg_byte_size, reg_byte_pos);
    rdim_bytecode_push_op(arena, &bytecode, RDI_EvalOp_RegRead, regread_param);
    rdim_bytecode_push_sconst(arena, &bytecode, offset);
    rdim_bytecode_push_op(arena, &bytecode, RDI_EvalOp_Add, 0);
    if(extra_indirection)
    {
      U64 addr_size = rdi_addr_size_from_arch(arch);
      rdim_bytecode_push_op(arena, &bytecode, RDI_EvalOp_MemRead, addr_size);
    }
    result = rdim_push_location_addr_bytecode_stream(arena, &bytecode);
  }
  return result;
}

internal CV_EncodedFramePtrReg
p2r_cv_encoded_fp_reg_from_frameproc(CV_SymFrameproc *frameproc, B32 param_base)
{
  CV_EncodedFramePtrReg result = 0;
  CV_FrameprocFlags flags = frameproc->flags;
  if(param_base)
  {
    result = CV_FrameprocFlags_ExtractParamBasePointer(flags);
  }
  else
  {
    result = CV_FrameprocFlags_ExtractLocalBasePointer(flags);
  }
  return result;
}

internal RDI_RegisterCode
p2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg)
{
  RDI_RegisterCode result = 0;
  switch(arch)
  {
    case RDI_Arch_X86:
    {
      switch(encoded_reg)
      {
        case CV_EncodedFramePtrReg_StackPtr:
        {
          // TODO(allen): support CV_AllReg_VFRAME
          // TODO(allen): error
        }break;
        case CV_EncodedFramePtrReg_FramePtr:
        {
          result = RDI_RegisterCode_X86_ebp;
        }break;
        case CV_EncodedFramePtrReg_BasePtr:
        {
          result = RDI_RegisterCode_X86_ebx;
        }break;
      }
    }break;
    case RDI_Arch_X64:
    {
      switch(encoded_reg)
      {
        case CV_EncodedFramePtrReg_StackPtr:
        {
          result = RDI_RegisterCode_X64_rsp;
        }break;
        case CV_EncodedFramePtrReg_FramePtr:
        {
          result = RDI_RegisterCode_X64_rbp;
        }break;
        case CV_EncodedFramePtrReg_BasePtr:
        {
          result = RDI_RegisterCode_X64_r13;
        }break;
      }
    }break;
  }
  return(result);
}

internal void
p2r_location_over_lvar_addr_range(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_LocationSet *locset, RDIM_Location *location, CV_LvarAddrRange *range, COFF_SectionHeader *section, CV_LvarAddrGap *gaps, U64 gap_count)
{
  //- rjf: extract range info
  U64 voff_first = 0;
  U64 voff_opl = 0;
  if(section != 0)
  {
    voff_first = section->voff + range->off;
    voff_opl = voff_first + range->len;
  }
  
  //- rjf: emit ranges
  CV_LvarAddrGap *gap_ptr = gaps;
  U64 voff_cursor = voff_first;
  for(U64 i = 0; i < gap_count; i += 1, gap_ptr += 1)
  {
    U64 voff_gap_first = voff_first + gap_ptr->off;
    U64 voff_gap_opl   = voff_gap_first + gap_ptr->len;
    if(voff_cursor < voff_gap_first)
    {
      RDIM_Rng1U64 voff_range = {voff_cursor, voff_gap_first};
      rdim_location_set_push_case(arena, scopes, locset, voff_range, location);
    }
    voff_cursor = voff_gap_opl;
  }
  
  //- rjf: emit remaining range
  if(voff_cursor < voff_opl)
  {
    RDIM_Rng1U64 voff_range = {voff_cursor, voff_opl};
    rdim_location_set_push_case(arena, scopes, locset, voff_range, location);
  }
}

////////////////////////////////
//~ rjf: Initial Parsing & Preparation Pass Tasks

internal void *
p2r_exe_hash_task__entry_point(Arena *arena, void *p)
{
  P2R_EXEHashIn *in = (P2R_EXEHashIn *)p;
  U64 *out = push_array(arena, U64, 1);
  ProfScope("hash exe") *out = rdi_hash(in->exe_data.str, in->exe_data.size);
  return out;
}

internal void *
p2r_tpi_hash_parse_task__entry_point(Arena *arena, void *p)
{
  P2R_TPIHashParseIn *in = (P2R_TPIHashParseIn *)p;
  void *out = 0;
  ProfScope("parse tpi hash") out = pdb_tpi_hash_from_data(arena, in->strtbl, in->tpi, in->hash_data, in->aux_data);
  return out;
}

internal void *
p2r_tpi_leaf_parse_task__entry_point(Arena *arena, void *p)
{
  P2R_TPILeafParseIn *in = (P2R_TPILeafParseIn *)p;
  void *out = 0;
  ProfScope("parse tpi leaf") out = cv_leaf_from_data(arena, in->leaf_data, in->itype_first);
  return out;
}

internal void *
p2r_symbol_stream_parse_task__entry_point(Arena *arena, void *p)
{
  P2R_SymbolStreamParseIn *in = (P2R_SymbolStreamParseIn *)p;
  void *out = 0;
  ProfScope("parse symbol stream") out = cv_sym_from_data(arena, in->data, 4);
  return out;
}

////////////////////////////////
//~ rjf: Type Forward Resolution Map Build / Thread

internal void
p2r_itype_fwd_map_fill(P2R_ITypeFwdMapFillIn *in)
{
  for(CV_TypeId itype = in->itype_first; itype < in->itype_opl; itype += 1)
  {
    //- rjf: determine if this itype resolves to another
    CV_TypeId itype_fwd = 0;
    CV_RecRange *range = &in->tpi_leaf->leaf_ranges.ranges[itype-in->itype_first];
    CV_LeafKind kind = range->hdr.kind;
    U64 header_struct_size = cv_header_struct_size_from_leaf_kind(kind);
    if(range->off+range->hdr.size <= in->tpi_leaf->data.size &&
       range->off+2+header_struct_size <= in->tpi_leaf->data.size &&
       range->hdr.size >= 2)
    {
      U8 *itype_leaf_first = in->tpi_leaf->data.str + range->off+2;
      U8 *itype_leaf_opl   = itype_leaf_first + range->hdr.size-2;
      switch(kind)
      {
        default:{}break;
        
        //- rjf: CLASS/STRUCTURE
        case CV_LeafKind_CLASS:
        case CV_LeafKind_STRUCTURE:
        {
          // rjf: unpack leaf header
          CV_LeafStruct *lf_struct = (CV_LeafStruct *)itype_leaf_first;
          
          // rjf: has fwd ref flag -> lookup itype that this itype resolves to
          if(lf_struct->props & CV_TypeProp_FwdRef)
          {
            // rjf: unpack rest of leaf
            U8 *numeric_ptr = (U8 *)(lf_struct + 1);
            CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
            U8 *name_ptr = numeric_ptr + size.encoded_size;
            String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
            U8 *unique_name_ptr = name_ptr + name.size + 1;
            String8 unique_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
            
            // rjf: lookup
            B32 do_unique_name_lookup = (((lf_struct->props & CV_TypeProp_Scoped) != 0) &&
                                         ((lf_struct->props & CV_TypeProp_HasUniqueName) != 0));
            itype_fwd = pdb_tpi_first_itype_from_name(in->tpi_hash, in->tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
          }
        }break;
        
        //- rjf: CLASS2/STRUCT2
        case CV_LeafKind_CLASS2:
        case CV_LeafKind_STRUCT2:
        {
          // rjf: unpack leaf header
          CV_LeafStruct2 *lf_struct = (CV_LeafStruct2 *)itype_leaf_first;
          
          // rjf: has fwd ref flag -> lookup itype that this itype resolves to
          if(lf_struct->props & CV_TypeProp_FwdRef)
          {
            // rjf: unpack rest of leaf
            U8 *numeric_ptr = (U8 *)(lf_struct + 1);
            CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
            U8 *name_ptr = (U8 *)numeric_ptr + size.encoded_size;
            String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
            U8 *unique_name_ptr = name_ptr + name.size + 1;
            String8 unique_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
            
            // rjf: lookup
            B32 do_unique_name_lookup = (((lf_struct->props & CV_TypeProp_Scoped) != 0) &&
                                         ((lf_struct->props & CV_TypeProp_HasUniqueName) != 0));
            itype_fwd = pdb_tpi_first_itype_from_name(in->tpi_hash, in->tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
          }
        }break;
        
        //- rjf: UNION
        case CV_LeafKind_UNION:
        {
          // rjf: unpack leaf
          CV_LeafUnion *lf_union = (CV_LeafUnion *)itype_leaf_first;
          U8 *numeric_ptr = (U8 *)(lf_union + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
          U8 *unique_name_ptr = name_ptr + name.size + 1;
          String8 unique_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
          
          // rjf: has fwd ref flag -> lookup itype that this itype resolves tos
          if(lf_union->props & CV_TypeProp_FwdRef)
          {
            B32 do_unique_name_lookup = (((lf_union->props & CV_TypeProp_Scoped) != 0) &&
                                         ((lf_union->props & CV_TypeProp_HasUniqueName) != 0));
            itype_fwd = pdb_tpi_first_itype_from_name(in->tpi_hash, in->tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
          }
        }break;
        
        //- rjf: ENUM
        case CV_LeafKind_ENUM:
        {
          // rjf: unpack leaf
          CV_LeafEnum *lf_enum = (CV_LeafEnum*)itype_leaf_first;
          U8 *name_ptr = (U8 *)(lf_enum + 1);
          String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
          U8 *unique_name_ptr = name_ptr + name.size + 1;
          String8 unique_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
          
          // rjf: has fwd ref flag -> lookup itype that this itype resolves to
          if(lf_enum->props & CV_TypeProp_FwdRef)
          {
            B32 do_unique_name_lookup = (((lf_enum->props & CV_TypeProp_Scoped) != 0) &&
                                         ((lf_enum->props & CV_TypeProp_HasUniqueName) != 0));
            itype_fwd = pdb_tpi_first_itype_from_name(in->tpi_hash, in->tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
          }
        }break;
      }
    }
    
    //- rjf: if the forwarded itype is nonzero & in TPI range -> save to map
    if(itype_fwd != 0 && itype_fwd < in->tpi_leaf->itype_opl)
    {
      in->itype_fwd_map[itype] = itype_fwd;
    }
  }
}

internal void
p2r_itype_fwd_map_fill_task_thread__entry_point(void *p)
{
  P2R_ITypeFwdMapFillTaskBatch *batch = (P2R_ITypeFwdMapFillTaskBatch *)p;
  ThreadName("[p2r] itype fwd map fill thread");
  for(;;)
  {
    U64 next_task_num = ins_atomic_u64_inc_eval(batch->num_tasks_taken_ptr);
    if(next_task_num > batch->tasks_count || 1 > next_task_num)
    {
      break;
    }
    P2R_ITypeFwdMapFillTask *task = &batch->tasks[next_task_num-1];
    ProfScope("convert (task #%I64u)", next_task_num)
    {
      p2r_itype_fwd_map_fill(&task->fill_in);
    }
  }
}

////////////////////////////////
//~ rjf: Symbol Stream Conversion Path & Thread

internal P2R_SymbolStreamConvertOut *
p2r_symbol_stream_convert(Arena *arena, P2R_SymbolStreamConvertIn *in)
{
  Temp scratch = scratch_begin(&arena, 1);
#define p2r_type_ptr_from_itype(itype) (((itype) < in->tpi_leaf->itype_opl) ? (in->itype_type_ptrs[(in->itype_fwd_map[(itype)] ? in->itype_fwd_map[(itype)] : (itype))]) : 0)
  
  //////////////////////////
  //- rjf: set up outputs for this sym stream
  //
  U64 sym_procedures_chunk_cap = 1024;
  U64 sym_global_variables_chunk_cap = 1024;
  U64 sym_thread_variables_chunk_cap = 1024;
  U64 sym_scopes_chunk_cap = 1024;
  RDIM_SymbolChunkList sym_procedures = {0};
  RDIM_SymbolChunkList sym_global_variables = {0};
  RDIM_SymbolChunkList sym_thread_variables = {0};
  RDIM_ScopeChunkList sym_scopes = {0};
  
  //////////////////////////
  //- rjf: symbols pass 1: produce procedure frame info map (procedure -> frame info)
  //
  U64 procedure_frameprocs_count = 0;
  U64 procedure_frameprocs_cap   = (in->sym_ranges_opl - in->sym_ranges_first);
  CV_SymFrameproc **procedure_frameprocs = push_array_no_zero(scratch.arena, CV_SymFrameproc *, procedure_frameprocs_cap);
  ProfScope("symbols pass 1: produce procedure frame info map (procedure -> frame info)")
  {
    U64 procedure_num = 0;
    CV_RecRange *rec_ranges_first = in->sym->sym_ranges.ranges + in->sym_ranges_first;
    CV_RecRange *rec_ranges_opl   = in->sym->sym_ranges.ranges + in->sym_ranges_opl;
    for(CV_RecRange *rec_range = rec_ranges_first;
        rec_range < rec_ranges_opl;
        rec_range += 1)
    {
      //- rjf: rec range -> symbol info range
      U64 sym_off_first = rec_range->off + 2;
      U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
      
      //- rjf: skip invalid ranges
      if(sym_off_opl > in->sym->data.size || sym_off_first > in->sym->data.size || sym_off_first > sym_off_opl)
      {
        continue;
      }
      
      //- rjf: unpack symbol info
      CV_SymKind kind = rec_range->hdr.kind;
      U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
      void *sym_header_struct_base = in->sym->data.str + sym_off_first;
      
      //- rjf: skip bad sizes
      if(sym_off_first + sym_header_struct_size > sym_off_opl)
      {
        continue;
      }
      
      //- rjf: consume symbol based on kind
      switch(kind)
      {
        default:{}break;
        
        //- rjf: FRAMEPROC
        case CV_SymKind_FRAMEPROC:
        {
          if(procedure_num == 0) { break; }
          if(procedure_num > procedure_frameprocs_cap) { break; }
          CV_SymFrameproc *frameproc = (CV_SymFrameproc*)sym_header_struct_base;
          procedure_frameprocs[procedure_num-1] = frameproc;
          procedure_frameprocs_count = Max(procedure_frameprocs_count, procedure_num);
        }break;
        
        //- rjf: LPROC32/GPROC32
        case CV_SymKind_LPROC32:
        case CV_SymKind_GPROC32:
        {
          procedure_num += 1;
        }break;
      }
    }
    U64 scratch_overkill = sizeof(procedure_frameprocs[0])*(procedure_frameprocs_cap-procedure_frameprocs_count);
    arena_put_back(scratch.arena, scratch_overkill);
  }
  
  //////////////////////////
  //- rjf: symbols pass 2: construct all symbols, given procedure frame info map
  //
  ProfScope("symbols pass 2: construct all symbols, given procedure frame info map")
  {
    RDIM_LocationSet *defrange_target = 0;
    B32 defrange_target_is_param = 0;
    U64 procedure_num = 0;
    CV_RecRange *rec_ranges_first = in->sym->sym_ranges.ranges + in->sym_ranges_first;
    CV_RecRange *rec_ranges_opl   = in->sym->sym_ranges.ranges + in->sym_ranges_opl;
    typedef struct P2R_ScopeNode P2R_ScopeNode;
    struct P2R_ScopeNode
    {
      P2R_ScopeNode *next;
      RDIM_Scope *scope;
    };
    P2R_ScopeNode *top_scope_node = 0;
    P2R_ScopeNode *free_scope_node = 0;
    for(CV_RecRange *rec_range = rec_ranges_first;
        rec_range < rec_ranges_opl;
        rec_range += 1)
    {
      //- rjf: rec range -> symbol info range
      U64 sym_off_first = rec_range->off + 2;
      U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
      
      //- rjf: skip invalid ranges
      if(sym_off_opl > in->sym->data.size || sym_off_first > in->sym->data.size || sym_off_first > sym_off_opl)
      {
        continue;
      }
      
      //- rjf: unpack symbol info
      CV_SymKind kind = rec_range->hdr.kind;
      U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
      void *sym_header_struct_base = in->sym->data.str + sym_off_first;
      void *sym_data_opl = in->sym->data.str + sym_off_opl;
      
      //- rjf: skip bad sizes
      if(sym_off_first + sym_header_struct_size > sym_off_opl)
      {
        continue;
      }
      
      //- rjf: consume symbol based on kind
      switch(kind)
      {
        default:{}break;
        
        //- rjf: END
        case CV_SymKind_END:
        {
          P2R_ScopeNode *n = top_scope_node;
          if(n != 0)
          {
            SLLStackPop(top_scope_node);
            SLLStackPush(free_scope_node, n);
          }
          defrange_target = 0;
          defrange_target_is_param = 0;
        }break;
        
        //- rjf: BLOCK32
        case CV_SymKind_BLOCK32:
        {
          // rjf: unpack sym
          CV_SymBlock32 *block32 = (CV_SymBlock32 *)sym_header_struct_base;
          
          // rjf: build scope, insert into current parent scope
          RDIM_Scope *scope = rdim_scope_chunk_list_push(arena, &sym_scopes, sym_scopes_chunk_cap);
          {
            if(top_scope_node == 0)
            {
              // TODO(rjf): log
            }
            if(top_scope_node != 0)
            {
              RDIM_Scope *top_scope = top_scope_node->scope;
              SLLQueuePush_N(top_scope->first_child, top_scope->last_child, scope, next_sibling);
              scope->parent_scope = top_scope;
              scope->symbol = top_scope->symbol;
            }
            COFF_SectionHeader *section = (0 < block32->sec && block32->sec <= in->coff_sections->count) ? &in->coff_sections->sections[block32->sec-1] : 0;
            if(section != 0)
            {
              U64 voff_first = section->voff + block32->off;
              U64 voff_last = voff_first + block32->len;
              RDIM_Rng1U64 voff_range = {voff_first, voff_last};
              rdim_scope_push_voff_range(arena, &sym_scopes, scope, voff_range);
            }
          }
          
          // rjf: push this scope to scope stack
          {
            P2R_ScopeNode *node = free_scope_node;
            if(node != 0) { SLLStackPop(free_scope_node); }
            else { node = push_array_no_zero(scratch.arena, P2R_ScopeNode, 1); }
            node->scope = scope;
            SLLStackPush(top_scope_node, node);
          }
        }break;
        
        //- rjf: LDATA32/GDATA32
        case CV_SymKind_LDATA32:
        case CV_SymKind_GDATA32:
        {
          // rjf: unpack sym
          CV_SymData32 *data32 = (CV_SymData32 *)sym_header_struct_base;
          String8 name = str8_cstring_capped(data32+1, sym_data_opl);
          COFF_SectionHeader *section = (0 < data32->sec && data32->sec <= in->coff_sections->count) ? &in->coff_sections->sections[data32->sec-1] : 0;
          U64 voff = (section ? section->voff : 0) + data32->off;
          
          // rjf: determine if this is an exact duplicate global
          //
          // PDB likes to have duplicates of these spread across different
          // symbol streams so we deduplicate across the entire translation
          // context.
          //
          B32 is_duplicate = 0;
          {
            // TODO(rjf): @important global symbol dedup
          }
          
          // rjf: is not duplicate -> push new global
          if(!is_duplicate)
          {
            // rjf: unpack global variable's type
            RDIM_Type *type = p2r_type_ptr_from_itype(data32->itype);
            
            // rjf: unpack global's container type
            RDIM_Type *container_type = 0;
            U64 container_name_opl = p2r_end_of_cplusplus_container_name(name);
            if(container_name_opl > 2)
            {
              String8 container_name = str8(name.str, container_name_opl - 2);
              CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(in->tpi_hash, in->tpi_leaf, name, 0);
              container_type = p2r_type_ptr_from_itype(cv_type_id);
            }
            
            // rjf: unpack global's container symbol
            RDIM_Symbol *container_symbol = 0;
            if(container_type == 0 && top_scope_node != 0)
            {
              container_symbol = top_scope_node->scope->symbol;
            }
            
            // rjf: build symbol
            RDIM_Symbol *symbol = rdim_symbol_chunk_list_push(arena, &sym_global_variables, sym_global_variables_chunk_cap);
            symbol->is_extern        = (kind == CV_SymKind_GDATA32);
            symbol->name             = name;
            symbol->type             = type;
            symbol->offset           = voff;
            symbol->container_symbol = container_symbol;
            symbol->container_type   = container_type;
          }
        }break;
        
        //- rjf: LPROC32/GPROC32
        case CV_SymKind_LPROC32:
        case CV_SymKind_GPROC32:
        {
          // rjf: unpack sym
          CV_SymProc32 *proc32 = (CV_SymProc32 *)sym_header_struct_base;
          String8 name = str8_cstring_capped(proc32+1, sym_data_opl);
          RDIM_Type *type = p2r_type_ptr_from_itype(proc32->itype);
          
          // rjf: unpack proc's container type
          RDIM_Type *container_type = 0;
          U64 container_name_opl = p2r_end_of_cplusplus_container_name(name);
          if(container_name_opl > 2)
          {
            String8 container_name = str8(name.str, container_name_opl - 2);
            CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(in->tpi_hash, in->tpi_leaf, name, 0);
            container_type = p2r_type_ptr_from_itype(cv_type_id);
          }
          
          // rjf: unpack proc's container symbol
          RDIM_Symbol *container_symbol = 0;
          if(container_type == 0 && top_scope_node != 0)
          {
            container_symbol = top_scope_node->scope->symbol;
          }
          
          // rjf: build procedure's root scope
          //
          // NOTE: even if there could be a containing scope at this point (which should be
          //       illegal in C/C++ but not necessarily in another language) we would not use
          //       it here because these scopes refer to the ranges of code that make up a
          //       procedure *not* the namespaces, so a procedure's root scope always has
          //       no parent.
          RDIM_Scope *procedure_root_scope = rdim_scope_chunk_list_push(arena, &sym_scopes, sym_scopes_chunk_cap);
          {
            COFF_SectionHeader *section = (0 < proc32->sec && proc32->sec <= in->coff_sections->count) ? &in->coff_sections->sections[proc32->sec-1] : 0;
            if(section != 0)
            {
              U64 voff_first = section->voff + proc32->off;
              U64 voff_last = voff_first + proc32->len;
              RDIM_Rng1U64 voff_range = {voff_first, voff_last};
              rdim_scope_push_voff_range(arena, &sym_scopes, procedure_root_scope, voff_range);
            }
          }
          
          // rjf: root scope voff minimum range -> link name
          String8 link_name = {0};
          if(procedure_root_scope->voff_ranges.min != 0)
          {
            U64 voff = procedure_root_scope->voff_ranges.min;
            U64 hash = p2r_hash_from_voff(voff);
            U64 bucket_idx = hash%in->link_name_map->buckets_count;
            P2R_LinkNameNode *node = 0;
            for(P2R_LinkNameNode *n = in->link_name_map->buckets[bucket_idx]; n != 0; n = n->next)
            {
              if(n->voff == voff)
              {
                link_name = n->name;
                break;
              }
            }
          }
          
          // rjf: build procedure symbol
          RDIM_Symbol *procedure_symbol = rdim_symbol_chunk_list_push(arena, &sym_procedures, sym_procedures_chunk_cap);
          procedure_symbol->is_extern        = (kind == CV_SymKind_GPROC32);
          procedure_symbol->name             = name;
          procedure_symbol->link_name        = link_name;
          procedure_symbol->type             = type;
          procedure_symbol->container_symbol = container_symbol;
          procedure_symbol->container_type   = container_type;
          procedure_symbol->root_scope       = procedure_root_scope;
          
          // rjf: fill root scope's symbol
          procedure_root_scope->symbol = procedure_symbol;
          
          // rjf: push scope to scope stack
          {
            P2R_ScopeNode *node = free_scope_node;
            if(node != 0) { SLLStackPop(free_scope_node); }
            else { node = push_array_no_zero(scratch.arena, P2R_ScopeNode, 1); }
            node->scope = procedure_root_scope;
            SLLStackPush(top_scope_node, node);
          }
          
          // rjf: increment procedure counter
          procedure_num += 1;
        }break;
        
        //- rjf: REGREL32
        case CV_SymKind_REGREL32:
        {
          // TODO(rjf): apparently some of the information here may end up being
          // redundant with "better" information from  CV_SymKind_LOCAL record.
          // we don't currently handle this, but if those cases arise then it
          // will obviously be better to prefer the better information from both
          // records.
          
          // rjf: no containing scope? -> malformed data; locals cannot be produced
          // outside of a containing scope
          if(top_scope_node == 0)
          {
            break;
          }
          
          // rjf: unpack sym
          CV_SymRegrel32 *regrel32 = (CV_SymRegrel32 *)sym_header_struct_base;
          String8 name = str8_cstring_capped(regrel32+1, sym_data_opl);
          RDIM_Type *type = p2r_type_ptr_from_itype(regrel32->itype);
          CV_Reg cv_reg = regrel32->reg;
          U32 var_off = regrel32->reg_off;
          
          // rjf: determine if this is a parameter
          RDI_LocalKind local_kind = RDI_LocalKind_Variable;
          {
            B32 is_stack_reg = 0;
            switch(in->arch)
            {
              default:{}break;
              case RDI_Arch_X86:{is_stack_reg = (cv_reg == CV_Regx86_ESP);}break;
              case RDI_Arch_X64:{is_stack_reg = (cv_reg == CV_Regx64_RSP);}break;
            }
            if(is_stack_reg)
            {
              U32 frame_size = 0xFFFFFFFF;
              if(procedure_num != 0 && procedure_frameprocs[procedure_num-1] != 0 && procedure_num < procedure_frameprocs_count)
              {
                CV_SymFrameproc *frameproc = procedure_frameprocs[procedure_num-1];
                frame_size = frameproc->frame_size;
              }
              if(var_off > frame_size)
              {
                local_kind = RDI_LocalKind_Parameter;
              }
            }
          }
          
          // rjf: build local
          RDIM_Scope *scope = top_scope_node->scope;
          RDIM_Local *local = rdim_scope_push_local(arena, &sym_scopes, scope);
          local->kind = local_kind;
          local->name = name;
          local->type = type;
          
          // rjf: add location info to local
          {
            // rjf: determine if we need an extra indirection to the value
            B32 extra_indirection_to_value = 0;
            switch(in->arch)
            {
              case RDI_Arch_X86:
              {
                extra_indirection_to_value = (local_kind == RDI_LocalKind_Parameter && (type->byte_size > 4 || !IsPow2OrZero(type->byte_size)));
              }break;
              case RDI_Arch_X64:
              {
                extra_indirection_to_value = (local_kind == RDI_LocalKind_Parameter && (type->byte_size > 8 || !IsPow2OrZero(type->byte_size)));
              }break;
            }
            
            // rjf: get raddbg register code
            RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(in->arch, cv_reg);
            // TODO(rjf): real byte_size & byte_pos from cv_reg goes here
            U32 byte_size = 8;
            U32 byte_pos = 0;
            
            // rjf: set location case
            RDIM_Location *loc = p2r_location_from_addr_reg_off(arena, in->arch, register_code, byte_size, byte_pos, (S64)(S32)var_off, extra_indirection_to_value);
            RDIM_Rng1U64 voff_range = {0, max_U64};
            rdim_location_set_push_case(arena, &sym_scopes, &local->locset, voff_range, loc);
          }
        }break;
        
        //- rjf: LTHREAD32/GTHREAD32
        case CV_SymKind_LTHREAD32:
        case CV_SymKind_GTHREAD32:
        {
          // rjf: unpack sym
          CV_SymThread32 *thread32 = (CV_SymThread32 *)sym_header_struct_base;
          String8 name = str8_cstring_capped(thread32+1, sym_data_opl);
          U32 tls_off = thread32->tls_off;
          RDIM_Type *type = p2r_type_ptr_from_itype(thread32->itype);
          
          // rjf: unpack thread variable's container type
          RDIM_Type *container_type = 0;
          U64 container_name_opl = p2r_end_of_cplusplus_container_name(name);
          if(container_name_opl > 2)
          {
            String8 container_name = str8(name.str, container_name_opl - 2);
            CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(in->tpi_hash, in->tpi_leaf, name, 0);
            container_type = p2r_type_ptr_from_itype(cv_type_id);
          }
          
          // rjf: unpack thread variable's container symbol
          RDIM_Symbol *container_symbol = 0;
          if(container_type == 0 && top_scope_node != 0)
          {
            container_symbol = top_scope_node->scope->symbol;
          }
          
          // rjf: build symbol
          RDIM_Symbol *tvar = rdim_symbol_chunk_list_push(arena, &sym_thread_variables, sym_thread_variables_chunk_cap);
          tvar->name             = name;
          tvar->type             = type;
          tvar->is_extern        = (kind == CV_SymKind_GTHREAD32);
          tvar->offset           = tls_off;
          tvar->container_type   = container_type;
          tvar->container_symbol = container_symbol;
        }break;
        
        //- rjf: LOCAL
        case CV_SymKind_LOCAL:
        {
          // rjf: no containing scope? -> malformed data; locals cannot be produced
          // outside of a containing scope
          if(top_scope_node == 0)
          {
            break;
          }
          
          // rjf: unpack sym
          CV_SymLocal *slocal = (CV_SymLocal *)sym_header_struct_base;
          String8 name = str8_cstring_capped(slocal+1, sym_data_opl);
          RDIM_Type *type = p2r_type_ptr_from_itype(slocal->itype);
          
          // rjf: determine if this symbol encodes the beginning of a global modification
          B32 is_global_modification = 0;
          if((slocal->flags & CV_LocalFlag_Global) ||
             (slocal->flags & CV_LocalFlag_Static))
          {
            is_global_modification = 1;
          }
          
          // rjf: is global modification -> emit global modification symbol
          if(is_global_modification)
          {
            // TODO(rjf): add global modification symbols
            defrange_target = 0;
            defrange_target_is_param = 0;
          }
          
          // rjf: is not a global modification -> emit a local variable
          if(!is_global_modification)
          {
            // rjf: determine local kind
            RDI_LocalKind local_kind = RDI_LocalKind_Variable;
            if(slocal->flags & CV_LocalFlag_Param)
            {
              local_kind = RDI_LocalKind_Parameter;
            }
            
            // rjf: build local
            RDIM_Scope *scope = top_scope_node->scope;
            RDIM_Local *local = rdim_scope_push_local(arena, &sym_scopes, scope);
            local->kind = local_kind;
            local->name = name;
            local->type = type;
            
            // rjf: save defrange target, for subsequent defrange symbols
            defrange_target = &local->locset;
            defrange_target_is_param = (local_kind == RDI_LocalKind_Parameter);
          }
        }break;
        
        //- rjf: DEFRANGE_REGISTESR
        case CV_SymKind_DEFRANGE_REGISTER:
        {
          // rjf: no defrange target? -> somehow we got to a defrange symbol without first seeing
          // a local - break immediately
          if(defrange_target == 0)
          {
            break;
          }
          
          // rjf: unpack sym
          CV_SymDefrangeRegister *defrange_register = (CV_SymDefrangeRegister*)sym_header_struct_base;
          CV_Reg cv_reg = defrange_register->reg;
          CV_LvarAddrRange *range = &defrange_register->range;
          COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= in->coff_sections->count) ? &in->coff_sections->sections[range->sec-1] : 0;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register+1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(in->arch, cv_reg);
          
          // rjf: build location
          RDIM_Location *location = rdim_push_location_val_reg(arena, register_code);
          
          // rjf: emit locations over ranges
          p2r_location_over_lvar_addr_range(arena, &sym_scopes, defrange_target, location, range, range_section, gaps, gap_count);
        }break;
        
        //- rjf: DEFRANGE_FRAMEPOINTER_REL
        case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL:
        {
          // rjf: no defrange target? -> somehow we got to a defrange symbol without first seeing
          // a local - break immediately
          if(defrange_target == 0)
          {
            break;
          }
          
          // rjf: find current procedure's frameproc
          CV_SymFrameproc *frameproc = 0;
          if(procedure_num != 0 && procedure_frameprocs[procedure_num-1] != 0 && procedure_num < procedure_frameprocs_count)
          {
            frameproc = procedure_frameprocs[procedure_num-1];
          }
          
          // rjf: no current valid frameproc? -> somehow we got a to a framepointer-relative defrange
          // without having an actually active procedure - break
          if(frameproc == 0)
          {
            break;
          }
          
          // rjf: unpack sym
          CV_SymDefrangeFramepointerRel *defrange_fprel = (CV_SymDefrangeFramepointerRel*)sym_header_struct_base;
          CV_LvarAddrRange *range = &defrange_fprel->range;
          COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= in->coff_sections->count) ? &in->coff_sections->sections[range->sec-1] : 0;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_fprel + 1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          
          // rjf: select frame pointer register
          CV_EncodedFramePtrReg encoded_fp_reg = p2r_cv_encoded_fp_reg_from_frameproc(frameproc, defrange_target_is_param);
          RDI_RegisterCode fp_register_code = p2r_reg_code_from_arch_encoded_fp_reg(in->arch, encoded_fp_reg);
          
          // rjf: build location
          B32 extra_indirection = 0;
          U32 byte_size = rdi_addr_size_from_arch(in->arch);
          U32 byte_pos = 0;
          S64 var_off = (S64)defrange_fprel->off;
          RDIM_Location *location = p2r_location_from_addr_reg_off(arena, in->arch, fp_register_code, byte_size, byte_pos, var_off, extra_indirection);
          
          // rjf: emit locations over ranges
          p2r_location_over_lvar_addr_range(arena, &sym_scopes, defrange_target, location, range, range_section, gaps, gap_count);
        }break;
        
        //- rjf: DEFRANGE_SUBFIELD_REGISTER
        case CV_SymKind_DEFRANGE_SUBFIELD_REGISTER:
        {
          // rjf: no defrange target? -> somehow we got to a defrange symbol without first seeing
          // a local - break immediately
          if(defrange_target == 0)
          {
            break;
          }
          
          // rjf: unpack sym
          CV_SymDefrangeSubfieldRegister *defrange_subfield_register = (CV_SymDefrangeSubfieldRegister*)sym_header_struct_base;
          CV_Reg cv_reg = defrange_subfield_register->reg;
          CV_LvarAddrRange *range = &defrange_subfield_register->range;
          COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= in->coff_sections->count) ? &in->coff_sections->sections[range->sec-1] : 0;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_subfield_register + 1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(in->arch, cv_reg);
          
          // rjf: skip "subfield" location info - currently not supported
          if(defrange_subfield_register->field_offset != 0)
          {
            break;
          }
          
          // rjf: build location
          RDIM_Location *location = rdim_push_location_val_reg(arena, register_code);
          
          // rjf: emit locations over ranges
          p2r_location_over_lvar_addr_range(arena, &sym_scopes, defrange_target, location, range, range_section, gaps, gap_count);
        }break;
        
        //- rjf: DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE
        case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
        {
          // rjf: no defrange target? -> somehow we got to a defrange symbol without first seeing
          // a local - break immediately
          if(defrange_target == 0)
          {
            break;
          }
          
          // rjf: find current procedure's frameproc
          CV_SymFrameproc *frameproc = 0;
          if(procedure_num != 0 && procedure_frameprocs[procedure_num-1] != 0 && procedure_num < procedure_frameprocs_count)
          {
            frameproc = procedure_frameprocs[procedure_num-1];
          }
          
          // rjf: no current valid frameproc? -> somehow we got a to a framepointer-relative defrange
          // without having an actually active procedure - break
          if(frameproc == 0)
          {
            break;
          }
          
          // rjf: unpack sym
          CV_SymDefrangeFramepointerRelFullScope *defrange_fprel_full_scope = (CV_SymDefrangeFramepointerRelFullScope*)sym_header_struct_base;
          CV_EncodedFramePtrReg encoded_fp_reg = p2r_cv_encoded_fp_reg_from_frameproc(frameproc, defrange_target_is_param);
          RDI_RegisterCode fp_register_code = p2r_reg_code_from_arch_encoded_fp_reg(in->arch, encoded_fp_reg);
          
          // rjf: build location
          B32 extra_indirection = 0;
          U32 byte_size = rdi_addr_size_from_arch(in->arch);
          U32 byte_pos = 0;
          S64 var_off = (S64)defrange_fprel_full_scope->off;
          RDIM_Location *location = p2r_location_from_addr_reg_off(arena, in->arch, fp_register_code, byte_size, byte_pos, var_off, extra_indirection);
          
          // rjf: emit location over ranges
          RDIM_Rng1U64 voff_range = {0, max_U64};
          rdim_location_set_push_case(arena, &sym_scopes, defrange_target, voff_range, location);
        }break;
        
        //- rjf: DEFRANGE_REGISTER_REL
        case CV_SymKind_DEFRANGE_REGISTER_REL:
        {
          // rjf: no defrange target? -> somehow we got to a defrange symbol without first seeing
          // a local - break immediately
          if(defrange_target == 0)
          {
            break;
          }
          
          // rjf: unpack sym
          CV_SymDefrangeRegisterRel *defrange_register_rel = (CV_SymDefrangeRegisterRel*)sym_header_struct_base;
          CV_Reg cv_reg = defrange_register_rel->reg;
          RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(in->arch, cv_reg);
          CV_LvarAddrRange *range = &defrange_register_rel->range;
          COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= in->coff_sections->count) ? &in->coff_sections->sections[range->sec-1] : 0;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register_rel + 1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          
          // rjf: build location
          // TODO(rjf): offset & size from cv_reg code
          U32 byte_size = rdi_addr_size_from_arch(in->arch);
          U32 byte_pos = 0;
          B32 extra_indirection_to_value = 0;
          S64 var_off = defrange_register_rel->reg_off;
          RDIM_Location *location = p2r_location_from_addr_reg_off(arena, in->arch, register_code, byte_size, byte_pos, var_off, extra_indirection_to_value);
          
          // rjf: emit locations over ranges
          p2r_location_over_lvar_addr_range(arena, &sym_scopes, defrange_target, location, range, range_section, gaps, gap_count);
        }break;
        
        //- rjf: FILESTATIC
        case CV_SymKind_FILESTATIC:
        {
          CV_SymFileStatic *file_static = (CV_SymFileStatic*)sym_header_struct_base;
          String8 name = str8_cstring_capped(file_static+1, sym_data_opl);
          RDIM_Type *type = p2r_type_ptr_from_itype(file_static->itype);
          // TODO(rjf): emit a global modifier symbol
          defrange_target = 0;
          defrange_target_is_param = 0;
        }break;
      }
    }
  }
  
  //////////////////////////
  //- rjf: allocate & fill output
  //
  P2R_SymbolStreamConvertOut *out = push_array(arena, P2R_SymbolStreamConvertOut, 1);
  {
    out->procedures       = sym_procedures;
    out->global_variables = sym_global_variables;
    out->thread_variables = sym_thread_variables;
    out->scopes           = sym_scopes;
  }
  
#undef p2r_type_ptr_from_itype
  scratch_end(scratch);
  return out;
}

internal void
p2r_symbol_stream_convert_task_thread__entry_point(void *p)
{
  P2R_SymbolStreamTaskBatch *batch = (P2R_SymbolStreamTaskBatch *)p;
  ThreadName("[p2r] unit symbol thread");
  for(;;)
  {
    U64 next_task_num = ins_atomic_u64_inc_eval(batch->num_tasks_taken_ptr);
    if(next_task_num > batch->tasks_count || 1 > next_task_num)
    {
      break;
    }
    P2R_SymbolStreamTask *task = &batch->tasks[next_task_num-1];
    ProfScope("convert (task #%I64u)", next_task_num)
    {
      task->convert_out = p2r_symbol_stream_convert(task->out_arena, &task->convert_in);
    }
  }
}

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal P2R_ConvertOut *
p2r_convert(Arena *arena, P2R_ConvertIn *in)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse MSF structure
  //
  MSF_Parsed *msf = 0;
  if(in->input_pdb_data.size != 0) ProfScope("parse MSF structure")
  {
    msf = msf_parsed_from_data(arena, in->input_pdb_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse PDB auth_guid & named streams table
  //
  PDB_NamedStreamTable *named_streams = 0;
  COFF_Guid auth_guid = {0};
  if(msf != 0) ProfScope("parse PDB auth_guid & named streams table")
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 info_data = msf_data_from_stream(msf, PDB_FixedStream_PdbInfo);
    PDB_Info *info = pdb_info_from_data(scratch.arena, info_data);
    named_streams = pdb_named_stream_table_from_info(arena, info);
    MemoryCopyStruct(&auth_guid, &info->auth_guid);
    scratch_end(scratch);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse PDB strtbl
  //
  PDB_Strtbl *strtbl = 0;
  if(named_streams != 0) ProfScope("parse PDB strtbl")
  {
    MSF_StreamNumber strtbl_sn = named_streams->sn[PDB_NamedStream_STRTABLE];
    String8 strtbl_data = msf_data_from_stream(msf, strtbl_sn);
    strtbl = pdb_strtbl_from_data(arena, strtbl_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse dbi
  //
  PDB_DbiParsed *dbi = 0;
  if(msf != 0) ProfScope("parse dbi")
  {
    String8 dbi_data = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
    dbi = pdb_dbi_from_data(arena, dbi_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse tpi
  //
  PDB_TpiParsed *tpi = 0;
  if(msf != 0) ProfScope("parse tpi")
  {
    String8 tpi_data = msf_data_from_stream(msf, PDB_FixedStream_Tpi);
    tpi = pdb_tpi_from_data(arena, tpi_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse ipi
  //
  PDB_TpiParsed *ipi = 0;
  if(msf != 0) ProfScope("parse ipi")
  {
    String8 ipi_data = msf_data_from_stream(msf, PDB_FixedStream_Ipi);
    ipi = pdb_tpi_from_data(arena, ipi_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse coff sections
  //
  PDB_CoffSectionArray *coff_sections = 0;
  U64 coff_section_count = 0;
  if(dbi != 0) ProfScope("parse coff sections")
  {
    MSF_StreamNumber section_stream = dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER];
    String8 section_data = msf_data_from_stream(msf, section_stream);
    coff_sections = pdb_coff_section_array_from_data(arena, section_data);
    coff_section_count = coff_sections->count;
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse gsi
  //
  PDB_GsiParsed *gsi = 0;
  if(dbi != 0) ProfScope("parse gsi")
  {
    String8 gsi_data = msf_data_from_stream(msf, dbi->gsi_sn);
    gsi = pdb_gsi_from_data(arena, gsi_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse psi
  //
  PDB_GsiParsed *psi_gsi_part = 0;
  if(dbi != 0) ProfScope("parse psi")
  {
    String8 psi_data = msf_data_from_stream(msf, dbi->psi_sn);
    String8 psi_data_gsi_part = str8_range(psi_data.str + sizeof(PDB_PsiHeader), psi_data.str + psi_data.size);
    psi_gsi_part = pdb_gsi_from_data(arena, psi_data_gsi_part);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: do independent parsing & preparation passes
  //
  U64 exe_hash = 0;
  PDB_TpiHashParsed *tpi_hash = 0;
  CV_LeafParsed *tpi_leaf = 0;
  PDB_TpiHashParsed *ipi_hash = 0;
  CV_LeafParsed *ipi_leaf = 0;
  ProfScope("do independent parsing & preparation passess")
  {
    //- rjf: form task inputs
    P2R_EXEHashIn exe_hash_in = {in->input_exe_data};
    P2R_TPIHashParseIn tpi_hash_in = {0};
    {
      tpi_hash_in.strtbl    = strtbl;
      tpi_hash_in.tpi       = tpi;
      tpi_hash_in.hash_data = msf_data_from_stream(msf, tpi->hash_sn);
      tpi_hash_in.aux_data  = msf_data_from_stream(msf, tpi->hash_sn_aux);
    }
    P2R_TPILeafParseIn tpi_leaf_in = {0};
    {
      tpi_leaf_in.leaf_data   = pdb_leaf_data_from_tpi(tpi);
      tpi_leaf_in.itype_first = tpi->itype_first;
    }
    P2R_TPIHashParseIn ipi_hash_in = {0};
    {
      ipi_hash_in.strtbl    = strtbl;
      ipi_hash_in.tpi       = ipi;
      ipi_hash_in.hash_data = msf_data_from_stream(msf, ipi->hash_sn);
      ipi_hash_in.aux_data  = msf_data_from_stream(msf, ipi->hash_sn_aux);
    }
    P2R_TPILeafParseIn ipi_leaf_in = {0};
    {
      ipi_leaf_in.leaf_data   = pdb_leaf_data_from_tpi(ipi);
      ipi_leaf_in.itype_first = ipi->itype_first;
    }
    
    //- rjf: kick off tasks
    TS_Ticket exe_hash_ticket = ts_kickoff(p2r_exe_hash_task__entry_point, &exe_hash_in);
    TS_Ticket tpi_hash_ticket = ts_kickoff(p2r_tpi_hash_parse_task__entry_point, &tpi_hash_in);
    TS_Ticket tpi_leaf_ticket = ts_kickoff(p2r_tpi_leaf_parse_task__entry_point, &tpi_leaf_in);
    TS_Ticket ipi_hash_ticket = ts_kickoff(p2r_tpi_hash_parse_task__entry_point, &ipi_hash_in);
    TS_Ticket ipi_leaf_ticket = ts_kickoff(p2r_tpi_leaf_parse_task__entry_point, &ipi_leaf_in);
    
    //- rjf: join tasks
    exe_hash = *ts_join_struct(exe_hash_ticket, max_U64, U64);
    tpi_hash =  ts_join_struct(tpi_hash_ticket, max_U64, PDB_TpiHashParsed);
    tpi_leaf =  ts_join_struct(tpi_leaf_ticket, max_U64, CV_LeafParsed);
    ipi_hash =  ts_join_struct(ipi_hash_ticket, max_U64, PDB_TpiHashParsed);
    ipi_leaf =  ts_join_struct(ipi_leaf_ticket, max_U64, CV_LeafParsed);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse sym
  //
  CV_SymParsed *sym = 0;
  if(dbi != 0) ProfScope("parse sym")
  {
    String8 sym_data = msf_data_from_stream(msf, dbi->sym_sn);
    sym = cv_sym_from_data(arena, sym_data, 4);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse compilation units
  //
  PDB_CompUnitArray *comp_units = 0;
  U64 comp_unit_count = 0;
  if(dbi != 0) ProfScope("parse compilation units")
  {
    String8 mod_info_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo);
    comp_units = pdb_comp_unit_array_from_data(arena, mod_info_data);
    comp_unit_count = comp_units->count;
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse dbi's section contributions
  //
  PDB_CompUnitContributionArray *comp_unit_contributions = 0;
  U64 comp_unit_contribution_count = 0;
  if(dbi != 0 && coff_sections != 0) ProfScope("parse dbi section contributions")
  {
    String8 section_contribution_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_SecCon);
    comp_unit_contributions = pdb_comp_unit_contribution_array_from_data(arena, section_contribution_data, coff_sections);
    comp_unit_contribution_count = comp_unit_contributions->count;
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse syms for each compilation unit
  //
  CV_SymParsed **sym_for_unit = push_array(arena, CV_SymParsed*, comp_unit_count);
  if(comp_units != 0) ProfScope("parse syms for each compilation unit")
  {
    PDB_CompUnit **unit_ptr = comp_units->units;
    for(U64 i = 0; i < comp_unit_count; i += 1, unit_ptr += 1)
    {
      String8 sym_data = pdb_data_from_unit_range(msf, *unit_ptr, PDB_DbiCompUnitRange_Symbols);
      sym_for_unit[i] = cv_sym_from_data(arena, sym_data, 4);
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse c13 for each compilation unit
  //
  CV_C13Parsed **c13_for_unit = push_array(arena, CV_C13Parsed*, comp_unit_count);
  if(comp_units != 0) ProfScope("parse c13s")
  {
    PDB_CompUnit **unit_ptr = comp_units->units;
    for(U64 i = 0; i < comp_unit_count; i += 1, unit_ptr += 1)
    {
      CV_C13Parsed *unit_c13 = 0;
      {
        String8 c13_data = pdb_data_from_unit_range(msf, *unit_ptr, PDB_DbiCompUnitRange_C13);
        unit_c13 = cv_c13_from_data(arena, c13_data, strtbl, coff_sections);
      }
      c13_for_unit[i] = unit_c13;
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: calculate EXE's max voff
  //
  U64 exe_voff_max = 0;
  {        
    COFF_SectionHeader *coff_sec_ptr = coff_sections->sections;
    COFF_SectionHeader *coff_ptr_opl = coff_sec_ptr + coff_section_count;
    for(;coff_sec_ptr < coff_ptr_opl; coff_sec_ptr += 1)
    {
      U64 sec_voff_max = coff_sec_ptr->voff + coff_sec_ptr->vsize;
      exe_voff_max = Max(exe_voff_max, sec_voff_max);
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: determine architecture
  //
  RDI_Arch arch = RDI_Arch_NULL;
  U64 arch_addr_size = 0;
  {
    // TODO(rjf): in some cases, the first compilation unit has a zero
    // architecture, as it's sometimes used as a "nil" unit. this causes bugs
    // in later stages of conversion - particularly, this was detected via
    // busted location info. so i've converted this to a scan-until-we-find-an-
    // architecture. however, this may still be fundamentally insufficient,
    // because Nick has informed me that x86 units can be linked with x64
    // units, meaning the appropriate architecture at any point in time is not
    // a top-level concept, and is rather dependent on to which compilation
    // unit particular symbols belong. so in the future, to support that (odd)
    // case, we'll need to not only have this be a top-level "contextual" piece
    // of info, but to use the appropriate compilation unit's architecture when
    // possible. assuming, of course, that we care about supporting that case.
    for(U64 comp_unit_idx = 0; comp_unit_idx < comp_unit_count; comp_unit_idx += 1)
    {
      if(sym_for_unit[comp_unit_idx] != 0)
      {
        arch = rdi_arch_from_cv_arch(sym_for_unit[comp_unit_idx]->info.arch);
        if(arch != RDI_Arch_NULL)
        {
          break;
        }
      }
    }
    arch_addr_size = rdi_addr_size_from_arch(arch);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce top-level-info
  //
  RDIM_TopLevelInfo top_level_info = {0};
  {
    top_level_info.arch     = arch;
    top_level_info.exe_name = in->input_exe_name;
    top_level_info.exe_hash = exe_hash;
    top_level_info.voff_max = exe_voff_max;
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: build binary sections list
  //
  RDIM_BinarySectionList binary_sections = {0};
  ProfScope("build binary section list")
  {
    COFF_SectionHeader *coff_ptr = coff_sections->sections;
    COFF_SectionHeader *coff_opl = coff_ptr + coff_section_count;
    for(;coff_ptr < coff_opl; coff_ptr += 1)
    {
      char *name_first = (char*)coff_ptr->name;
      char *name_opl   = name_first + sizeof(coff_ptr->name);
      RDIM_BinarySection *sec = rdim_binary_section_list_push(arena, &binary_sections);
      sec->name       = str8_cstring_capped(name_first, name_opl);
      sec->flags      = rdi_binary_section_flags_from_coff_section_flags(coff_ptr->flags);
      sec->voff_first = coff_ptr->voff;
      sec->voff_opl   = coff_ptr->voff+coff_ptr->vsize;
      sec->foff_first = coff_ptr->foff;
      sec->foff_opl   = coff_ptr->foff+coff_ptr->fsize;
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: build units
  //
  RDIM_UnitChunkList units = {0};
  ProfScope("build unit array")
  {
    U64 units_chunk_cap = comp_unit_count;
    
    //- rjf: pass 1: fill basic per-unit info & line info
    for(U64 comp_unit_idx = 0; comp_unit_idx < comp_unit_count; comp_unit_idx += 1)
    {
      PDB_CompUnit *pdb_unit     = comp_units->units[comp_unit_idx];
      CV_SymParsed *pdb_unit_sym = sym_for_unit[comp_unit_idx];
      CV_C13Parsed *pdb_unit_c13 = c13_for_unit[comp_unit_idx];
      
      //- rjf: produce unit name
      String8 unit_name = pdb_unit->obj_name;
      if(unit_name.size != 0)
      {
        String8 unit_name_past_last_slash = str8_skip_last_slash(unit_name);
        if(unit_name_past_last_slash.size != 0)
        {
          unit_name = unit_name_past_last_slash;
        }
      }
      
      //- rjf: produce obj name
      String8 obj_name = pdb_unit->obj_name;
      if(str8_match(obj_name, str8_lit("* Linker *"), 0) ||
         str8_match(obj_name, str8_lit("Import:"), StringMatchFlag_RightSideSloppy))
      {
        MemoryZeroStruct(&obj_name);
      }
      
      //- rjf: build unit
      RDIM_Unit *dst_unit = rdim_unit_chunk_list_push(arena, &units, units_chunk_cap);
      dst_unit->unit_name     = unit_name;
      dst_unit->compiler_name = pdb_unit_sym->info.compiler_name;
      dst_unit->object_file   = obj_name;
      dst_unit->archive_file  = pdb_unit->group_name;
      dst_unit->language      = rdi_language_from_cv_language(sym->info.language);
      
      //- rjf: fill unit line info
      for(CV_C13SubSectionNode *node = pdb_unit_c13->first_sub_section;
          node != 0;
          node = node->next)
      {
        if(node->kind == CV_C13_SubSectionKind_Lines)
        {
          for(CV_C13LinesParsedNode *lines_n = node->lines_first;
              lines_n != 0;
              lines_n = lines_n->next)
          {
            CV_C13LinesParsed *lines = &lines_n->v;
            RDIM_LineSequence *seq = rdim_line_sequence_list_push(arena, &dst_unit->line_sequences);
            seq->file_name  = lines->file_name;
            seq->voffs      = lines->voffs;
            seq->line_nums  = lines->line_nums;
            seq->col_nums   = lines->col_nums;
            seq->line_count = lines->line_count;
          }
        }
      }
    }
    
    //- rjf: pass 2: build per-unit voff ranges from comp unit contributions table
    PDB_CompUnitContribution *contrib_ptr = comp_unit_contributions->contributions;
    PDB_CompUnitContribution *contrib_opl = contrib_ptr + comp_unit_contribution_count;
    for(;contrib_ptr < contrib_opl; contrib_ptr += 1)
    {
      if(contrib_ptr->mod < comp_unit_count)
      {
        RDIM_Unit *unit = &units.first->v[contrib_ptr->mod];
        RDIM_Rng1U64 range = {contrib_ptr->voff_first, contrib_ptr->voff_opl};
        rdim_rng1u64_list_push(arena, &unit->voff_ranges, range);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: predict symbol count
  //
  U64 symbol_count_prediction = 0;
  ProfScope("predict symbol count")
  {
    U64 rec_range_count = 0;
    if(sym != 0)
    {
      rec_range_count += sym->sym_ranges.count;
    }
    for(U64 comp_unit_idx = 0; comp_unit_idx < comp_unit_count; comp_unit_idx += 1)
    {
      CV_SymParsed *unit_sym = sym_for_unit[comp_unit_idx];
      rec_range_count += unit_sym->sym_ranges.count;
    }
    symbol_count_prediction = rec_range_count/8;
    if(symbol_count_prediction < 256)
    {
      symbol_count_prediction = 256;
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 1: produce type forward resolution map
  //
  // this map is used to resolve usage of "incomplete structs" in codeview's
  // type info. this often happens when e.g. "struct Foo" is used to refer to
  // a later-defined "Foo", which actually contains members  and so on. we want
  // to hook types up to their actual destination complete types wherever
  // possible, and so this map can be used to do that in subsequent stages.
  //
  CV_TypeId *itype_fwd_map = 0;
  CV_TypeId itype_first = tpi_leaf->itype_first;
  CV_TypeId itype_opl = tpi_leaf->itype_opl;
  ProfScope("types pass 1: produce type forward resolution map")
  {
    ////////////////////////////
    //- rjf: allocate forward resolution map
    //
    itype_fwd_map = push_array(arena, CV_TypeId, (U64)itype_opl);
    
    ////////////////////////////
    //- rjf: produce task batch
    //
    U64 num_tasks_taken = 0;
    P2R_ITypeFwdMapFillTaskBatch task_batch = {0};
    {
      U64 task_size_itypes = 4096;
      task_batch.tasks_count = (U64)itype_opl/task_size_itypes;
      task_batch.tasks = push_array(scratch.arena, P2R_ITypeFwdMapFillTask, task_batch.tasks_count);
      task_batch.num_tasks_taken_ptr = &num_tasks_taken;
      for(U64 task_idx = 0; task_idx < task_batch.tasks_count; task_idx += 1)
      {
        P2R_ITypeFwdMapFillTask *task = &task_batch.tasks[task_idx];
        task->fill_in.tpi_hash    = tpi_hash;
        task->fill_in.tpi_leaf    = tpi_leaf;
        task->fill_in.itype_first = task_idx*task_size_itypes;
        task->fill_in.itype_opl   = task->fill_in.itype_first + task_size_itypes;
        task->fill_in.itype_opl   = ClampTop(task->fill_in.itype_opl, itype_opl);
        task->fill_in.itype_fwd_map = itype_fwd_map;
      }
    }
    
    ////////////////////////////
    //- rjf: launch task threads
    //
    U64 task_threads_count = os_logical_core_count()-1;
    OS_Handle *task_threads = push_array(scratch.arena, OS_Handle, task_threads_count);
    for(U64 idx = 0; idx < task_threads_count; idx += 1)
    {
      task_threads[idx] = os_launch_thread(p2r_itype_fwd_map_fill_task_thread__entry_point, &task_batch, 0);
    }
    
    ////////////////////////////
    //- rjf: perform tasks on the main thread
    //
    ProfScope("perform tasks on the main thread")
    {
      for(;;)
      {
        U64 next_task_num = ins_atomic_u64_inc_eval(task_batch.num_tasks_taken_ptr);
        if(next_task_num > task_batch.tasks_count || 1 > next_task_num)
        {
          break;
        }
        P2R_ITypeFwdMapFillTask *task = &task_batch.tasks[next_task_num-1];
        ProfScope("convert (task #%I64u)", next_task_num)
        {
          p2r_itype_fwd_map_fill(&task->fill_in);
        }
      }
    }
    
    ////////////////////////////
    //- rjf: wait for task threads
    //
    ProfScope("wait for task threads")
    {
      for(U64 idx = 0; idx < task_threads_count; idx += 1)
      {
        os_thread_wait(task_threads[idx], max_U64);
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 2: produce per-itype itype chain
  //
  // this pass is to ensure that subsequent passes always produce types for
  // dependent itypes first - guaranteeing raddbgi's "only reference backward"
  // rule (which eliminates cycles). each itype slot gets a list of itypes,
  // starting with the deepest dependency - when types are produced per-itype,
  // this chain is walked, so that deeper dependencies are built first, and
  // as such, always show up *earlier* in the actually built types.
  //
  typedef struct P2R_TypeIdChain P2R_TypeIdChain;
  struct P2R_TypeIdChain
  {
    P2R_TypeIdChain *next;
    CV_TypeId itype;
  };
  P2R_TypeIdChain **itype_chains = 0;
  ProfScope("types pass 2: produce per-itype itype chain (for producing dependent types first)")
  {
    Temp scratch = scratch_begin(&arena, 1);
    itype_chains = push_array(arena, P2R_TypeIdChain *, (U64)itype_opl);
    for(CV_TypeId itype = 0; itype < itype_opl; itype += 1)
    {
      //- rjf: push initial itype - should be final-visited-itype for this itype
      {
        P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
        c->itype = itype;
        SLLStackPush(itype_chains[itype], c);
      }
      
      //- rjf: skip basic types for dependency walk
      if(itype < itype_first)
      {
        continue;
      }
      
      //- rjf: walk dependent types, push to chain
      P2R_TypeIdChain start_walk_task = {0, itype};
      P2R_TypeIdChain *first_walk_task = &start_walk_task;
      P2R_TypeIdChain *last_walk_task = &start_walk_task;
      for(P2R_TypeIdChain *walk_task = first_walk_task;
          walk_task != 0;
          walk_task = walk_task->next)
      {
        CV_TypeId walk_itype = itype_fwd_map[walk_task->itype] ? itype_fwd_map[walk_task->itype] : walk_task->itype;
        if(walk_itype < itype_first)
        {
          continue;
        }
        CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[walk_itype-itype_first];
        CV_LeafKind kind = range->hdr.kind;
        U64 header_struct_size = cv_header_struct_size_from_leaf_kind(kind);
        if(range->off+range->hdr.size <= tpi_leaf->data.size &&
           range->off+2+header_struct_size <= tpi_leaf->data.size &&
           range->hdr.size >= 2)
        {
          U8 *itype_leaf_first = tpi_leaf->data.str + range->off+2;
          U8 *itype_leaf_opl   = itype_leaf_first + range->hdr.size-2;
          switch(kind)
          {
            default:{}break;
            
            //- rjf: MODIFIER
            case CV_LeafKind_MODIFIER:
            {
              CV_LeafModifier *lf = (CV_LeafModifier *)itype_leaf_first;
              
              // rjf: push dependent itype to chain
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->itype;
                SLLStackPush(itype_chains[itype], c);
              }
              
              // rjf: push task to walk dependency itype
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
            }break;
            
            //- rjf: POINTER
            case CV_LeafKind_POINTER:
            {
              CV_LeafModifier *lf = (CV_LeafModifier *)itype_leaf_first;
              
              // rjf: push dependent itype to chain
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->itype;
                SLLStackPush(itype_chains[itype], c);
              }
              
              // rjf: push task to walk dependency itype
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
            }break;
            
            //- rjf: PROCEDURE
            case CV_LeafKind_PROCEDURE:
            {
              CV_LeafProcedure *lf = (CV_LeafProcedure *)itype_leaf_first;
              
              // rjf: push dependent itypes to chain
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->ret_itype;
                SLLStackPush(itype_chains[itype], c);
              }
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->arg_itype;
                SLLStackPush(itype_chains[itype], c);
              }
              
              // rjf: push task to walk dependency itypes
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->ret_itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->arg_itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
            }break;
            
            //- rjf: MFUNCTION
            case CV_LeafKind_MFUNCTION:
            {
              CV_LeafMFunction *lf = (CV_LeafMFunction *)itype_leaf_first;
              
              // rjf: push dependent itypes to chain
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->ret_itype;
                SLLStackPush(itype_chains[itype], c);
              }
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->arg_itype;
                SLLStackPush(itype_chains[itype], c);
              }
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->this_itype;
                SLLStackPush(itype_chains[itype], c);
              }
              
              // rjf: push task to walk dependency itypes
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->ret_itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->arg_itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->this_itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
            }break;
            
            //- rjf: BITFIELD
            case CV_LeafKind_BITFIELD:
            {
              CV_LeafBitField *lf = (CV_LeafBitField *)itype_leaf_first;
              
              // rjf: push dependent itype to chain
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->itype;
                SLLStackPush(itype_chains[itype], c);
              }
              
              // rjf: push task to walk dependency itype
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
            }break;
            
            //- rjf: ARRAY
            case CV_LeafKind_ARRAY:
            {
              CV_LeafArray *lf = (CV_LeafArray *)itype_leaf_first;
              
              // rjf: push dependent itypes to chain
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->entry_itype;
                SLLStackPush(itype_chains[itype], c);
              }
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->index_itype;
                SLLStackPush(itype_chains[itype], c);
              }
              
              // rjf: push task to walk dependency itypes
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->entry_itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->index_itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
            }break;
            
            //- rjf: ENUM
            case CV_LeafKind_ENUM:
            {
              CV_LeafEnum *lf = (CV_LeafEnum *)itype_leaf_first;
              
              // rjf: push dependent itypes to chain
              {
                P2R_TypeIdChain *c = push_array(arena, P2R_TypeIdChain, 1);
                c->itype = lf->base_itype;
                SLLStackPush(itype_chains[itype], c);
              }
              
              // rjf: push task to walk dependency itypes
              {
                P2R_TypeIdChain *c = push_array(scratch.arena, P2R_TypeIdChain, 1);
                c->itype = lf->base_itype;
                SLLQueuePush(first_walk_task, last_walk_task, c);
              }
            }break;
          }
        }
      }
    }
    scratch_end(scratch);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 3: construct all types from TPI
  //
  // this doesn't gather struct/class/union/enum members, which is done by
  // subsequent passes, to build RADDBGI "UDT" information, which is distinct
  // from regular type info.
  //
  typedef struct P2R_TypeIdRevisitTask P2R_TypeIdRevisitTask;
  struct P2R_TypeIdRevisitTask
  {
    P2R_TypeIdRevisitTask *next;
    RDIM_Type *base_type;
    CV_TypeId field_itype;
    CV_TypeId this_itype;
  };
  P2R_TypeIdRevisitTask *first_itype_revisit_task = 0;
  P2R_TypeIdRevisitTask *last_itype_revisit_task = 0;
  RDIM_Type **itype_type_ptrs = 0;
  RDIM_TypeChunkList all_types = {0};
#define p2r_type_ptr_from_itype(itype) (((itype) < itype_opl) ? (itype_type_ptrs[(itype_fwd_map[(itype)] ? itype_fwd_map[(itype)] : (itype))]) : 0)
  ProfScope("types pass 3: construct all root/stub types from TPI")
  {
    itype_type_ptrs = push_array(arena, RDIM_Type *, (U64)(itype_opl));
    for(CV_TypeId root_itype = 0; root_itype < itype_opl; root_itype += 1)
    {
      for(P2R_TypeIdChain *itype_chain = itype_chains[root_itype];
          itype_chain != 0;
          itype_chain = itype_chain->next)
      {
        CV_TypeId itype = (root_itype != itype_chain->itype && itype_chain->itype < itype_opl && itype_fwd_map[itype_chain->itype]) ? itype_fwd_map[itype_chain->itype] : itype_chain->itype;
        B32 itype_is_basic = (itype < 0x1000);
        
        //////////////////////////
        //- rjf: skip forward-reference itypes - all future resolutions will
        // reference whatever this itype resolves to, and so there is no point
        // in filling out this slot
        //
        if(itype_fwd_map[root_itype] != 0)
        {
          continue;
        }
        
        //////////////////////////
        //- rjf: skip already produced dependencies
        //
        if(itype_type_ptrs[itype] != 0)
        {
          continue;
        }
        
        //////////////////////////
        //- rjf: build basic type
        //
        if(itype_is_basic)
        {
          RDIM_Type *dst_type = 0;
          
          // rjf: unpack itype
          CV_BasicPointerKind cv_basic_ptr_kind  = CV_BasicPointerKindFromTypeId(itype);
          CV_BasicType        cv_basic_type_code = CV_BasicTypeFromTypeId(itype);
          
          // rjf: get basic type slot, fill if unfilled
          RDIM_Type *basic_type = itype_type_ptrs[cv_basic_type_code];
          if(basic_type == 0)
          {
            RDI_TypeKind type_kind = rdi_type_kind_from_cv_basic_type(cv_basic_type_code);
            U32 byte_size = rdi_size_from_basic_type_kind(type_kind);
            basic_type = dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
            if(byte_size == 0xffffffff)
            {
              byte_size = arch_addr_size;
            }
            basic_type->kind      = type_kind;
            basic_type->name      = cv_type_name_from_basic_type(cv_basic_type_code);
            basic_type->byte_size = byte_size;
          }
          
          // rjf: nonzero ptr kind -> form ptr type to basic tpye
          if(cv_basic_ptr_kind != 0)
          {
            dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
            dst_type->kind        = RDI_TypeKind_Ptr;
            dst_type->byte_size   = arch_addr_size;
            dst_type->direct_type = basic_type;
          }
          
          // rjf: fill this itype's slot with the finished type
          itype_type_ptrs[itype] = dst_type;
        }
        
        //////////////////////////
        //- rjf: build non-basic type
        //
        if(!itype_is_basic && itype >= itype_first)
        {
          RDIM_Type *dst_type = 0;
          CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[itype-itype_first];
          CV_LeafKind kind = range->hdr.kind;
          U64 header_struct_size = cv_header_struct_size_from_leaf_kind(kind);
          if(range->off+range->hdr.size <= tpi_leaf->data.size &&
             range->off+2+header_struct_size <= tpi_leaf->data.size &&
             range->hdr.size >= 2)
          {
            U8 *itype_leaf_first = tpi_leaf->data.str + range->off+2;
            U8 *itype_leaf_opl   = itype_leaf_first + range->hdr.size-2;
            switch(kind)
            {
              //- rjf: MODIFIER
              case CV_LeafKind_MODIFIER:
              {
                // rjf: unpack leaf
                CV_LeafModifier *lf = (CV_LeafModifier *)itype_leaf_first;
                
                // rjf: cv -> rdi flags
                RDI_TypeModifierFlags flags = 0;
                if(lf->flags & CV_ModifierFlag_Const)    {flags |= RDI_TypeModifierFlag_Const;}
                if(lf->flags & CV_ModifierFlag_Volatile) {flags |= RDI_TypeModifierFlag_Volatile;}
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                dst_type->kind        = RDI_TypeKind_Modifier;
                dst_type->flags       = flags;
                dst_type->direct_type = p2r_type_ptr_from_itype(lf->itype);
                dst_type->byte_size   = dst_type->direct_type ? dst_type->direct_type->byte_size : 0;
              }break;
              
              //- rjf: POINTER
              case CV_LeafKind_POINTER:
              {
                // TODO(rjf): if ptr_mode in {PtrMem, PtrMethod} then output a member pointer instead
                
                // rjf: unpack leaf
                CV_LeafPointer *lf = (CV_LeafPointer *)itype_leaf_first;
                RDIM_Type *direct_type = p2r_type_ptr_from_itype(lf->itype);
                CV_PointerKind ptr_kind = CV_PointerAttribs_ExtractKind(lf->attribs);
                CV_PointerMode ptr_mode = CV_PointerAttribs_ExtractMode(lf->attribs);
                U32            ptr_size = CV_PointerAttribs_ExtractSize(lf->attribs);
                
                // rjf: cv -> rdi modifier flags
                RDI_TypeModifierFlags modifier_flags = 0;
                if(lf->attribs & CV_PointerAttrib_Const)    {modifier_flags |= RDI_TypeModifierFlag_Const;}
                if(lf->attribs & CV_PointerAttrib_Volatile) {modifier_flags |= RDI_TypeModifierFlag_Volatile;}
                
                // rjf: cv info -> rdi pointer type kind
                RDI_TypeKind type_kind = RDI_TypeKind_Ptr;
                {
                  if(lf->attribs & CV_PointerAttrib_LRef)
                  {
                    type_kind = RDI_TypeKind_LRef;
                  }
                  else if(lf->attribs & CV_PointerAttrib_RRef)
                  {
                    type_kind = RDI_TypeKind_RRef;
                  }
                  if(ptr_mode == CV_PointerMode_LRef)
                  {
                    type_kind = RDI_TypeKind_LRef;
                  }
                  else if(ptr_mode == CV_PointerMode_RRef)
                  {
                    type_kind = RDI_TypeKind_RRef;
                  }
                }
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                if(modifier_flags != 0)
                {
                  RDIM_Type *pointer_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                  dst_type->kind             = RDI_TypeKind_Modifier;
                  dst_type->flags            = modifier_flags;
                  dst_type->direct_type      = pointer_type;
                  dst_type->byte_size        = arch_addr_size;
                  pointer_type->kind         = type_kind;
                  pointer_type->byte_size    = arch_addr_size;
                  pointer_type->direct_type  = direct_type;
                }
                else
                {
                  dst_type->kind        = type_kind;
                  dst_type->byte_size   = arch_addr_size;
                  dst_type->direct_type = direct_type;
                }
              }break;
              
              //- rjf: PROCEDURE
              case CV_LeafKind_PROCEDURE:
              {
                // TODO(rjf): handle call_kind & attribs
                
                // rjf: unpack leaf
                CV_LeafProcedure *lf = (CV_LeafProcedure *)itype_leaf_first;
                RDIM_Type *ret_type = p2r_type_ptr_from_itype(lf->ret_itype);
                
                // rjf: fill type's basics
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                dst_type->kind        = RDI_TypeKind_Function;
                dst_type->byte_size   = arch_addr_size;
                dst_type->direct_type = ret_type;
                
                // rjf: unpack arglist range
                CV_RecRange *arglist_range = &tpi_leaf->leaf_ranges.ranges[lf->arg_itype-itype_first];
                if(arglist_range->hdr.kind != CV_LeafKind_ARGLIST ||
                   arglist_range->hdr.size<2 ||
                   arglist_range->off + arglist_range->hdr.size > tpi_leaf->data.size)
                {
                  break;
                }
                U8 *arglist_first = tpi_leaf->data.str + arglist_range->off + 2;
                U8 *arglist_opl   = arglist_first+arglist_range->hdr.size-2;
                if(arglist_first + sizeof(CV_LeafArgList) > arglist_opl)
                {
                  break;
                }
                
                // rjf: unpack arglist info
                CV_LeafArgList *arglist = (CV_LeafArgList*)arglist_first;
                CV_TypeId *arglist_itypes_base = (CV_TypeId *)(arglist+1);
                U32 arglist_itypes_count = arglist->count;
                
                // rjf: build param type array
                RDIM_Type **params = push_array(arena, RDIM_Type *, arglist_itypes_count);
                for(U32 idx = 0; idx < arglist_itypes_count; idx += 1)
                {
                  params[idx] = p2r_type_ptr_from_itype(arglist_itypes_base[idx]);
                }
                
                // rjf: fill dst type
                dst_type->count = arglist_itypes_count;
                dst_type->param_types = params;
              }break;
              
              //- rjf: MFUNCTION
              case CV_LeafKind_MFUNCTION:
              {
                // TODO(rjf): handle call_kind & attribs
                // TODO(rjf): preserve "this_adjust"
                
                // rjf: unpack leaf
                CV_LeafMFunction *lf = (CV_LeafMFunction *)itype_leaf_first;
                RDIM_Type *ret_type  = p2r_type_ptr_from_itype(lf->ret_itype);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                dst_type->kind        = (lf->this_itype != 0) ? RDI_TypeKind_Method : RDI_TypeKind_Function;
                dst_type->byte_size   = arch_addr_size;
                dst_type->direct_type = ret_type;
                
                // rjf: unpack arglist range
                CV_RecRange *arglist_range = &tpi_leaf->leaf_ranges.ranges[lf->arg_itype-itype_first];
                if(arglist_range->hdr.kind != CV_LeafKind_ARGLIST ||
                   arglist_range->hdr.size<2 ||
                   arglist_range->off + arglist_range->hdr.size > tpi_leaf->data.size)
                {
                  break;
                }
                U8 *arglist_first = tpi_leaf->data.str + arglist_range->off + 2;
                U8 *arglist_opl   = arglist_first+arglist_range->hdr.size-2;
                if(arglist_first + sizeof(CV_LeafArgList) > arglist_opl)
                {
                  break;
                }
                
                // rjf: unpack arglist info
                CV_LeafArgList *arglist = (CV_LeafArgList*)arglist_first;
                CV_TypeId *arglist_itypes_base = (CV_TypeId *)(arglist+1);
                U32 arglist_itypes_count = arglist->count;
                
                // rjf: build param type array
                RDIM_Type **params = push_array(arena, RDIM_Type *, arglist_itypes_count+1);
                for(U32 idx = 0; idx < arglist_itypes_count; idx += 1)
                {
                  params[idx+1] = p2r_type_ptr_from_itype(arglist_itypes_base[idx]);
                }
                params[0] = p2r_type_ptr_from_itype(lf->this_itype);
                
                // rjf: fill dst type
                dst_type->count = arglist_itypes_count+1;
                dst_type->param_types = params;
              }break;
              
              //- rjf: BITFIELD
              case CV_LeafKind_BITFIELD:
              {
                // rjf: unpack leaf
                CV_LeafBitField *lf = (CV_LeafBitField *)itype_leaf_first;
                RDIM_Type *direct_type = p2r_type_ptr_from_itype(lf->itype);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                dst_type->kind        = RDI_TypeKind_Bitfield;
                dst_type->off         = lf->pos;
                dst_type->count       = lf->len;
                dst_type->byte_size   = direct_type?direct_type->byte_size:0;
                dst_type->direct_type = direct_type;
              }break;
              
              //- rjf: ARRAY
              case CV_LeafKind_ARRAY:
              {
                // rjf: unpack leaf
                CV_LeafArray *lf = (CV_LeafArray *)itype_leaf_first;
                RDIM_Type *direct_type = p2r_type_ptr_from_itype(lf->entry_itype);
                U8 *numeric_ptr = (U8*)(lf + 1);
                CV_NumericParsed array_count = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
                U64 full_size = cv_u64_from_numeric(&array_count);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                dst_type->kind        = RDI_TypeKind_Array;
                dst_type->direct_type = direct_type;
                dst_type->byte_size   = full_size;
                dst_type->count       = (direct_type && direct_type->byte_size) ? (dst_type->byte_size/direct_type->byte_size) : 0;
              }break;
              
              //- rjf: CLASS/STRUCTURE
              case CV_LeafKind_CLASS:
              case CV_LeafKind_STRUCTURE:
              {
                // TODO(rjf): handle props
                
                // rjf: unpack leaf
                CV_LeafStruct *lf = (CV_LeafStruct *)itype_leaf_first;
                U8 *numeric_ptr = (U8*)(lf + 1);
                CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
                U64 size_u64 = cv_u64_from_numeric(&size);
                U8 *name_ptr = numeric_ptr + size.encoded_size;
                String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                if(lf->props & CV_TypeProp_FwdRef)
                {
                  dst_type->kind = (kind == CV_LeafKind_CLASS ? RDI_TypeKind_IncompleteClass : RDI_TypeKind_IncompleteStruct);
                  dst_type->name = name;
                }
                else
                {
                  dst_type->kind      = (kind == CV_LeafKind_CLASS ? RDI_TypeKind_Class : RDI_TypeKind_Struct);
                  dst_type->byte_size = (U32)size_u64;
                  dst_type->name      = name;
                }
                
                // rjf: push revisit task for members
                if(!(lf->props & CV_TypeProp_FwdRef))
                {
                  P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
                  SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
                  t->base_type   = dst_type;
                  t->field_itype = lf->field_itype;
                }
              }break;
              
              //- rjf: CLASS2/STRUCT2
              case CV_LeafKind_CLASS2:
              case CV_LeafKind_STRUCT2:
              {
                // TODO(rjf): handle props
                
                // rjf: unpack leaf
                CV_LeafStruct2 *lf = (CV_LeafStruct2 *)itype_leaf_first;
                U8 *numeric_ptr = (U8*)(lf + 1);
                CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
                U64 size_u64 = cv_u64_from_numeric(&size);
                U8 *name_ptr = numeric_ptr + size.encoded_size;
                String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                if(lf->props & CV_TypeProp_FwdRef)
                {
                  dst_type->kind = (kind == CV_LeafKind_CLASS2 ? RDI_TypeKind_IncompleteClass : RDI_TypeKind_IncompleteStruct);
                  dst_type->name = name;
                }
                else
                {
                  dst_type->kind      = (kind == CV_LeafKind_CLASS2 ? RDI_TypeKind_Class : RDI_TypeKind_Struct);
                  dst_type->byte_size = (U32)size_u64;
                  dst_type->name      = name;
                }
                
                // rjf: push revisit task for members
                if(!(lf->props & CV_TypeProp_FwdRef))
                {
                  P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
                  SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
                  t->base_type   = dst_type;
                  t->field_itype = lf->field_itype;
                }
              }break;
              
              //- rjf: UNION
              case CV_LeafKind_UNION:
              {
                // TODO(rjf): handle props
                
                // rjf: unpack leaf
                CV_LeafUnion *lf = (CV_LeafUnion *)itype_leaf_first;
                U8 *numeric_ptr = (U8*)(lf + 1);
                CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
                U64 size_u64 = cv_u64_from_numeric(&size);
                U8 *name_ptr = numeric_ptr + size.encoded_size;
                String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                if(lf->props & CV_TypeProp_FwdRef)
                {
                  dst_type->kind = RDI_TypeKind_IncompleteUnion;
                  dst_type->name = name;
                }
                else
                {
                  dst_type->kind      = RDI_TypeKind_Union;
                  dst_type->byte_size = (U32)size_u64;
                  dst_type->name      = name;
                }
                
                // rjf: push revisit task for members
                if(!(lf->props & CV_TypeProp_FwdRef))
                {
                  P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
                  SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
                  t->base_type   = dst_type;
                  t->field_itype = lf->field_itype;
                }
              }break;
              
              //- rjf: ENUM
              case CV_LeafKind_ENUM:
              {
                // TODO(rjf): handle props
                
                // rjf: unpack leaf
                CV_LeafEnum *lf = (CV_LeafEnum *)itype_leaf_first;
                RDIM_Type *direct_type = p2r_type_ptr_from_itype(lf->base_itype);
                U8 *name_ptr = (U8 *)(lf + 1);
                String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
                
                // rjf: fill type
                dst_type = rdim_type_chunk_list_push(arena, &all_types, (U64)itype_opl);
                if(lf->props & CV_TypeProp_FwdRef)
                {
                  dst_type->kind = RDI_TypeKind_IncompleteEnum;
                  dst_type->name = name;
                }
                else
                {
                  dst_type->kind        = RDI_TypeKind_Enum;
                  dst_type->direct_type = direct_type;
                  dst_type->byte_size   = direct_type ? direct_type->byte_size : 0;
                  dst_type->name        = name;
                }
                
                // rjf: push revisit task for enumerates
                if(!(lf->props & CV_TypeProp_FwdRef))
                {
                  P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
                  SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
                  t->base_type   = dst_type;
                  t->field_itype = lf->field_itype;
                }
              }break;
            }
          }
          
          //- rjf: store finalized type to this itype's slot
          itype_type_ptrs[itype] = dst_type;
        }
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 4: build UDTs
  //
  RDIM_UDTChunkList all_udts = {0};
  ProfScope("types pass 4: build UDTs")
  {
    RDI_U64 udts_chunk_cap = 1024;
    for(P2R_TypeIdRevisitTask *task = first_itype_revisit_task; task != 0; task = task->next)
    {
      RDIM_Type *dst_type = task->base_type;
      switch(dst_type->kind)
      {
        default:{}break;
        
        ////////////////////////
        //- rjf: structs/unions/classes -> equip members
        //
        case RDI_TypeKind_Struct:
        case RDI_TypeKind_Union:
        case RDI_TypeKind_Class:
        {
          //- rjf: grab UDT info
          RDIM_UDT *dst_udt = dst_type->udt;
          if(dst_udt == 0)
          {
            dst_udt = dst_type->udt = rdim_udt_chunk_list_push(arena, &all_udts, udts_chunk_cap);
            dst_udt->self_type = dst_type;
          }
          
          //- rjf: gather all fields
          typedef struct FieldListTask FieldListTask;
          struct FieldListTask
          {
            FieldListTask *next;
            CV_TypeId itype;
          };
          FieldListTask start_fl_task = {0, task->field_itype};
          FieldListTask *fl_todo_stack = &start_fl_task;
          FieldListTask *fl_done_stack = 0;
          for(;fl_todo_stack != 0;)
          {
            //- rjf: take & unpack task
            FieldListTask *fl_task = fl_todo_stack;
            SLLStackPop(fl_todo_stack);
            SLLStackPush(fl_done_stack, fl_task);
            CV_TypeId field_list_itype = fl_task->itype;
            
            //- rjf: skip bad itypes
            if(field_list_itype < itype_first || itype_opl <= field_list_itype)
            {
              continue;
            }
            
            //- rjf: field list itype -> range
            CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[field_list_itype-itype_first];
            
            //- rjf: skip bad headers
            if(range->off+range->hdr.size > tpi_leaf->data.size ||
               range->hdr.size < 2 ||
               range->hdr.kind != CV_LeafKind_FIELDLIST)
            {
              continue;
            }
            
            //- rjf: loop over all fields
            {
              U8 *field_list_first = tpi_leaf->data.str+range->off+2;
              U8 *field_list_opl = field_list_first+range->hdr.size-2;
              for(U8 *read_ptr = field_list_first, *next_read_ptr = field_list_opl;
                  read_ptr < field_list_opl;
                  read_ptr = next_read_ptr)
              {
                // rjf: unpack field
                CV_LeafKind field_kind = *(CV_LeafKind *)read_ptr;
                U64 field_leaf_header_size = cv_header_struct_size_from_leaf_kind(field_kind);
                U8 *field_leaf_first = read_ptr+2;
                U8 *field_leaf_opl   = field_leaf_first+range->hdr.size-2;
                next_read_ptr = field_leaf_opl;
                
                // rjf: skip out-of-bounds fields
                if(field_leaf_first+field_leaf_header_size > field_list_opl)
                {
                  continue;
                }
                
                // rjf: process field
                switch(field_kind)
                {
                  //- rjf: unhandled/invalid cases
                  default:
                  {
                    // TODO(rjf): log
                  }break;
                  
                  //- rjf: INDEX
                  case CV_LeafKind_INDEX:
                  {
                    // rjf: unpack leaf
                    CV_LeafIndex *lf = (CV_LeafIndex *)field_leaf_first;
                    CV_TypeId new_itype = lf->itype;
                    
                    // rjf: determine if index itype is new
                    B32 is_new = 1;
                    for(FieldListTask *t = fl_done_stack; t != 0; t = t->next)
                    {
                      if(t->itype == new_itype)
                      {
                        is_new = 0;
                        break;
                      }
                    }
                    
                    // rjf: if new -> push task to follow new itype
                    if(is_new)
                    {
                      FieldListTask *new_task = push_array(scratch.arena, FieldListTask, 1);
                      SLLStackPush(fl_todo_stack, new_task);
                      new_task->itype = new_itype;
                    }
                  }break;
                  
                  //- rjf: MEMBER
                  case CV_LeafKind_MEMBER:
                  {
                    // TODO(rjf): log on bad offset
                    
                    // rjf: unpack leaf
                    CV_LeafMember *lf = (CV_LeafMember *)field_leaf_first;
                    U8 *offset_ptr = (U8 *)(lf+1);
                    CV_NumericParsed offset = cv_numeric_from_data_range(offset_ptr, field_leaf_opl);
                    U64 offset64 = cv_u64_from_numeric(&offset);
                    U8 *name_ptr = offset_ptr + offset.encoded_size;
                    String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                    
                    // rjf: bump next read pointer past variable length parts
                    next_read_ptr = name.str+name.size+1;
                    
                    // rjf: emit member
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                    mem->kind = RDI_MemberKind_DataField;
                    mem->name = name;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
                    mem->off  = (U32)offset64;
                  }break;
                  
                  //- rjf: STMEMBER
                  case CV_LeafKind_STMEMBER:
                  {
                    // TODO(rjf): handle attribs
                    
                    // rjf: unpack leaf
                    CV_LeafStMember *lf = (CV_LeafStMember *)field_leaf_first;
                    U8 *name_ptr = (U8 *)(lf+1);
                    String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                    
                    // rjf: bump next read pointer past variable length parts
                    next_read_ptr = name.str+name.size+1;
                    
                    // rjf: emit member
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                    mem->kind = RDI_MemberKind_StaticData;
                    mem->name = name;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
                  }break;
                  
                  //- rjf: METHOD
                  case CV_LeafKind_METHOD:
                  {
                    // rjf: unpack leaf
                    CV_LeafMethod *lf = (CV_LeafMethod *)field_leaf_first;
                    U8 *name_ptr = (U8 *)(lf+1);
                    String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                    
                    // rjf: bump next read pointer past variable length parts
                    next_read_ptr = name.str+name.size+1;
                    
                    //- rjf: method list itype -> range
                    CV_RecRange *method_list_range = &tpi_leaf->leaf_ranges.ranges[lf->list_itype-itype_first];
                    
                    //- rjf: skip bad method lists
                    if(method_list_range->off+method_list_range->hdr.size > tpi_leaf->data.size ||
                       method_list_range->hdr.size < 2 ||
                       method_list_range->hdr.kind != CV_LeafKind_METHODLIST)
                    {
                      break;
                    }
                    
                    //- rjf: loop through all methods & emit members
                    U8 *method_list_first = tpi_leaf->data.str + method_list_range->off + 2;
                    U8 *method_list_opl   = method_list_first + method_list_range->hdr.size-2;
                    for(U8 *method_read_ptr = method_list_first, *next_method_read_ptr = method_list_opl;
                        method_read_ptr < method_list_opl;
                        method_read_ptr = next_method_read_ptr)
                    {
                      CV_LeafMethodListMember *method = (CV_LeafMethodListMember*)method_read_ptr;
                      CV_MethodProp prop = CV_FieldAttribs_ExtractMethodProp(method->attribs);
                      RDIM_Type *method_type = p2r_type_ptr_from_itype(method->itype);
                      next_method_read_ptr = (U8 *)(method+1);
                      
                      // TODO(allen): PROBLEM
                      // We only get offsets for virtual functions (the "vbaseoff") from
                      // "Intro" and "PureIntro". In C++ inheritance, when we have a chain
                      // of inheritance (let's just talk single inheritance for now) the
                      // first class in the chain that introduces a new virtual function
                      // has this "Intro" method. If a later class in the chain redefines
                      // the virtual function it only has a "Virtual" method which does
                      // not update the offset. There is a "Virtual" and "PureVirtual"
                      // variant of "Virtual". The "Pure" in either case means there
                      // is no concrete procedure. When there is no "Pure" the method
                      // should have a corresponding procedure symbol id.
                      //
                      // The issue is we will want to mark all of our virtual methods as
                      // virtual and give them an offset, but that means we have to do
                      // some extra figuring to propogate offsets from "Intro" methods
                      // to "Virtual" methods in inheritance trees. That is - IF we want
                      // to start preserving the offsets of virtuals. There is room in
                      // the method struct to make this work, but for now I've just
                      // decided to drop this information. It is not urgently useful to
                      // us and greatly complicates matters.
                      
                      // rjf: read vbaseoff
                      U32 vbaseoff = 0;
                      if(prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro)
                      {
                        if(next_method_read_ptr+4 <= method_list_opl)
                        {
                          vbaseoff = *(U32 *)next_method_read_ptr;
                        }
                        next_method_read_ptr += 4;
                      }
                      
                      // rjf: emit method
                      switch(prop)
                      {
                        default:
                        {
                          RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                          mem->kind = RDI_MemberKind_Method;
                          mem->name = name;
                          mem->type = method_type;
                        }break;
                        case CV_MethodProp_Static:
                        {
                          RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                          mem->kind = RDI_MemberKind_StaticMethod;
                          mem->name = name;
                          mem->type = method_type;
                        }break;
                        case CV_MethodProp_Virtual:
                        case CV_MethodProp_PureVirtual:
                        case CV_MethodProp_Intro:
                        case CV_MethodProp_PureIntro:
                        {
                          RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                          mem->kind = RDI_MemberKind_VirtualMethod;
                          mem->name = name;
                          mem->type = method_type;
                        }break;
                      }
                    }
                    
                  }break;
                  
                  //- rjf: ONEMETHOD
                  case CV_LeafKind_ONEMETHOD:
                  {
                    // TODO(rjf): handle attribs
                    
                    // rjf: unpack leaf
                    CV_LeafOneMethod *lf = (CV_LeafOneMethod *)field_leaf_first;
                    CV_MethodProp prop = CV_FieldAttribs_ExtractMethodProp(lf->attribs);
                    U8 *vbaseoff_ptr = (U8 *)(lf+1);
                    U8 *vbaseoff_opl_ptr = vbaseoff_ptr;
                    U32 vbaseoff = 0;
                    if(prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro)
                    {
                      vbaseoff = *(U32 *)(vbaseoff_ptr);
                      vbaseoff_opl_ptr += sizeof(U32);
                    }
                    U8 *name_ptr = vbaseoff_opl_ptr;
                    String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                    RDIM_Type *method_type = p2r_type_ptr_from_itype(lf->itype);
                    
                    // rjf: bump next read pointer past variable length parts
                    next_read_ptr = name.str+name.size+1;
                    
                    // rjf: emit method
                    switch(prop)
                    {
                      default:
                      {
                        RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                        mem->kind = RDI_MemberKind_Method;
                        mem->name = name;
                        mem->type = method_type;
                      }break;
                      
                      case CV_MethodProp_Static:
                      {
                        RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                        mem->kind = RDI_MemberKind_StaticMethod;
                        mem->name = name;
                        mem->type = method_type;
                      }break;
                      
                      case CV_MethodProp_Virtual:
                      case CV_MethodProp_PureVirtual:
                      case CV_MethodProp_Intro:
                      case CV_MethodProp_PureIntro:
                      {
                        RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                        mem->kind = RDI_MemberKind_VirtualMethod;
                        mem->name = name;
                        mem->type = method_type;
                      }break;
                    }
                  }break;
                  
                  //- rjf: NESTTYPE
                  case CV_LeafKind_NESTTYPE:
                  {
                    // rjf: unpack leaf
                    CV_LeafNestType *lf = (CV_LeafNestType *)field_leaf_first;
                    U8 *name_ptr = (U8 *)(lf+1);
                    String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                    
                    // rjf: bump next read pointer past variable length parts
                    next_read_ptr = name.str+name.size+1;
                    
                    // rjf: emit member
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                    mem->kind = RDI_MemberKind_NestedType;
                    mem->name = name;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
                  }break;
                  
                  //- rjf: NESTTYPEEX
                  case CV_LeafKind_NESTTYPEEX:
                  {
                    // TODO(rjf): handle attribs
                    
                    // rjf: unpack leaf
                    CV_LeafNestTypeEx *lf = (CV_LeafNestTypeEx *)field_leaf_first;
                    U8 *name_ptr = (U8 *)(lf+1);
                    String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                    
                    // rjf: bump next read pointer past variable length parts
                    next_read_ptr = name.str+name.size+1;
                    
                    // rjf: emit member
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                    mem->kind = RDI_MemberKind_NestedType;
                    mem->name = name;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
                  }break;
                  
                  //- rjf: BCLASS
                  case CV_LeafKind_BCLASS:
                  {
                    // TODO(rjf): log on bad offset
                    
                    // rjf: unpack leaf
                    CV_LeafBClass *lf = (CV_LeafBClass *)field_leaf_first;
                    U8 *offset_ptr = (U8 *)(lf+1);
                    CV_NumericParsed offset = cv_numeric_from_data_range(offset_ptr, field_leaf_opl);
                    U64 offset64 = cv_u64_from_numeric(&offset);
                    
                    // rjf: bump next read pointer past variable length parts
                    next_read_ptr = offset_ptr+offset.encoded_size;
                    
                    // rjf: emit member
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                    mem->kind = RDI_MemberKind_Base;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
                    mem->off  = (U32)offset64;
                  }break;
                  
                  //- rjf: VBCLASS/IVBCLASS
                  case CV_LeafKind_VBCLASS:
                  case CV_LeafKind_IVBCLASS:
                  {
                    // TODO(rjf): log on bad offsets
                    // TODO(rjf): handle attribs
                    // TODO(rjf): offsets?
                    
                    // rjf: unpack leaf
                    CV_LeafVBClass *lf = (CV_LeafVBClass *)field_leaf_first;
                    U8 *num1_ptr = (U8 *)(lf+1);
                    CV_NumericParsed num1 = cv_numeric_from_data_range(num1_ptr, field_leaf_opl);
                    U8 *num2_ptr = num1_ptr + num1.encoded_size;
                    CV_NumericParsed num2 = cv_numeric_from_data_range(num2_ptr, field_leaf_opl);
                    
                    // rjf: emit member
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &all_udts, dst_udt);
                    mem->kind = RDI_MemberKind_VirtualBase;
                    mem->type = p2r_type_ptr_from_itype(lf->itype);
                  }break;
                  
                  //- rjf: VFUNCTAB
                  case CV_LeafKind_VFUNCTAB:
                  {
                    CV_LeafVFuncTab *lf = (CV_LeafVFuncTab *)field_leaf_first;
                    // NOTE(rjf): currently no-op this case
                    (void)lf;
                  }break;
                }
                
                // rjf: align-up next field
                next_read_ptr = (U8 *)AlignPow2((U64)next_read_ptr, 4);
              }
            }
          }
        }break;
        
        ////////////////////////
        //- rjf: enums -> equip enumerates
        //
        case RDI_TypeKind_Enum:
        {
          //- rjf: grab UDT info
          RDIM_UDT *dst_udt = dst_type->udt;
          if(dst_udt == 0)
          {
            dst_udt = dst_type->udt = rdim_udt_chunk_list_push(arena, &all_udts, udts_chunk_cap);
            dst_udt->self_type = dst_type;
          }
          
          //- rjf: gather all fields
          typedef struct FieldListTask FieldListTask;
          struct FieldListTask
          {
            FieldListTask *next;
            CV_TypeId itype;
          };
          FieldListTask start_fl_task = {0, task->field_itype};
          FieldListTask *fl_todo_stack = &start_fl_task;
          FieldListTask *fl_done_stack = 0;
          for(;fl_todo_stack != 0;)
          {
            //- rjf: take & unpack task
            FieldListTask *fl_task = fl_todo_stack;
            SLLStackPop(fl_todo_stack);
            SLLStackPush(fl_done_stack, fl_task);
            CV_TypeId field_list_itype = fl_task->itype;
            
            //- rjf: skip bad itypes
            if(field_list_itype < itype_first || itype_opl <= field_list_itype)
            {
              continue;
            }
            
            //- rjf: field list itype -> range
            CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[field_list_itype-itype_first];
            
            //- rjf: skip bad headers
            if(range->off+range->hdr.size > tpi_leaf->data.size ||
               range->hdr.size < 2 ||
               range->hdr.kind != CV_LeafKind_FIELDLIST)
            {
              continue;
            }
            
            //- rjf: loop over all fields
            {
              U8 *field_list_first = tpi_leaf->data.str+range->off+2;
              U8 *field_list_opl = field_list_first+range->hdr.size-2;
              for(U8 *read_ptr = field_list_first, *next_read_ptr = field_list_opl;
                  read_ptr < field_list_opl;
                  read_ptr = next_read_ptr)
              {
                // rjf: unpack field
                CV_LeafKind field_kind = *(CV_LeafKind *)read_ptr;
                U64 field_leaf_header_size = cv_header_struct_size_from_leaf_kind(field_kind);
                U8 *field_leaf_first = read_ptr+2;
                U8 *field_leaf_opl   = field_leaf_first+range->hdr.size-2;
                next_read_ptr = field_leaf_opl;
                
                // rjf: skip out-of-bounds fields
                if(field_leaf_first+field_leaf_header_size > field_list_opl)
                {
                  continue;
                }
                
                // rjf: process field
                switch(field_kind)
                {
                  //- rjf: unhandled/invalid cases
                  default:
                  {
                    // TODO(rjf): log
                  }break;
                  
                  //- rjf: INDEX
                  case CV_LeafKind_INDEX:
                  {
                    // rjf: unpack leaf
                    CV_LeafIndex *lf = (CV_LeafIndex *)field_leaf_first;
                    CV_TypeId new_itype = lf->itype;
                    
                    // rjf: determine if index itype is new
                    B32 is_new = 1;
                    for(FieldListTask *t = fl_done_stack; t != 0; t = t->next)
                    {
                      if(t->itype == new_itype)
                      {
                        is_new = 0;
                        break;
                      }
                    }
                    
                    // rjf: if new -> push task to follow new itype
                    if(is_new)
                    {
                      FieldListTask *new_task = push_array(scratch.arena, FieldListTask, 1);
                      SLLStackPush(fl_todo_stack, new_task);
                      new_task->itype = new_itype;
                    }
                  }break;
                  
                  //- rjf: ENUMERATE
                  case CV_LeafKind_ENUMERATE:
                  {
                    // TODO(rjf): attribs
                    
                    // rjf: unpack leaf
                    CV_LeafEnumerate *lf = (CV_LeafEnumerate *)field_leaf_first;
                    U8 *val_ptr = (U8 *)(lf+1);
                    CV_NumericParsed val = cv_numeric_from_data_range(val_ptr, field_leaf_opl);
                    U64 val64 = cv_u64_from_numeric(&val);
                    U8 *name_ptr = val_ptr + val.encoded_size;
                    String8 name = str8_cstring_capped(name_ptr, field_leaf_opl);
                    
                    // rjf: bump next read pointer past variable length parts
                    next_read_ptr = name.str+name.size+1;
                    
                    // rjf: emit member
                    RDIM_UDTEnumVal *enum_val = rdim_udt_push_enum_val(arena, &all_udts, dst_udt);
                    enum_val->name = name;
                    enum_val->val  = val64;
                  }break;
                }
                
                // rjf: align-up next field
                next_read_ptr = (U8 *)AlignPow2((U64)next_read_ptr, 4);
              }
            }
          }
        }break;
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce link name map
  //
  P2R_LinkNameMap link_name_map = {0};
  ProfScope("produce link name map")
  {
    link_name_map.buckets_count = symbol_count_prediction;
    link_name_map.buckets       = push_array(arena, P2R_LinkNameNode *, link_name_map.buckets_count);
    CV_RecRange *rec_ranges_first = sym->sym_ranges.ranges;
    CV_RecRange *rec_ranges_opl   = rec_ranges_first + sym->sym_ranges.count;
    for(CV_RecRange *rec_range = rec_ranges_first;
        rec_range < rec_ranges_opl;
        rec_range += 1)
    {
      //- rjf: unpack symbol range info
      CV_SymKind kind = rec_range->hdr.kind;
      U64 header_struct_size = cv_header_struct_size_from_sym_kind(kind);
      U8 *sym_first = sym->data.str + rec_range->off + 2;
      U8 *sym_opl   = sym_first + rec_range->hdr.size;
      
      //- rjf: skip bad ranges
      if(sym_opl > sym->data.str + sym->data.size || sym_first + header_struct_size > sym->data.str + sym->data.size)
      {
        continue;
      }
      
      //- rjf: consume symbol
      switch(kind)
      {
        default:{}break;
        case CV_SymKind_PUB32:
        {
          // rjf: unpack sym
          CV_SymPub32 *pub32 = (CV_SymPub32 *)sym_first;
          String8 name = str8_cstring_capped(pub32+1, sym_opl);
          COFF_SectionHeader *section = (0 < pub32->sec && pub32->sec <= coff_sections->count) ? &coff_sections->sections[pub32->sec-1] : 0;
          U64 voff = 0;
          if(section != 0)
          {
            voff = section->voff + pub32->off;
          }
          
          // rjf: commit to link name map
          U64 hash = p2r_hash_from_voff(voff);
          U64 bucket_idx = hash%link_name_map.buckets_count;
          P2R_LinkNameNode *node = push_array(arena, P2R_LinkNameNode, 1);
          SLLStackPush(link_name_map.buckets[bucket_idx], node);
          node->voff = voff;
          node->name = name;
          link_name_map.link_name_count += 1;
          link_name_map.bucket_collision_count += (node->next != 0);
        }break;
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: produce symbols from all streams
  //
  RDIM_SymbolChunkList all_procedures = {0};
  RDIM_SymbolChunkList all_global_variables = {0};
  RDIM_SymbolChunkList all_thread_variables = {0};
  RDIM_ScopeChunkList all_scopes = {0};
  ProfScope("produce symbols from all streams")
  {
    ////////////////////////////
    //- rjf: produce per-task info
    //
    U64 global_stream_subdivision_tasks_count = (sym->sym_ranges.count+65535)/65536;
    U64 global_stream_syms_per_task = sym->sym_ranges.count/global_stream_subdivision_tasks_count;
    U64 tasks_count = comp_unit_count + global_stream_subdivision_tasks_count;
    P2R_SymbolStreamTask *tasks = push_array(scratch.arena, P2R_SymbolStreamTask, tasks_count);
    {
      for(U64 idx = 0; idx < tasks_count; idx += 1)
      {
        tasks[idx].convert_in.arch            = arch;
        tasks[idx].convert_in.coff_sections   = coff_sections;
        tasks[idx].convert_in.tpi_hash        = tpi_hash;
        tasks[idx].convert_in.tpi_leaf        = tpi_leaf;
        tasks[idx].convert_in.itype_fwd_map   = itype_fwd_map;
        tasks[idx].convert_in.itype_type_ptrs = itype_type_ptrs;
        tasks[idx].convert_in.link_name_map   = &link_name_map;
        tasks[idx].out_arena = arena_alloc();
        if(idx < global_stream_subdivision_tasks_count)
        {
          tasks[idx].convert_in.sym             = sym;
          tasks[idx].convert_in.sym_ranges_first= idx*global_stream_syms_per_task;
          tasks[idx].convert_in.sym_ranges_opl  = tasks[idx].convert_in.sym_ranges_first + global_stream_syms_per_task;
          tasks[idx].convert_in.sym_ranges_opl  = ClampTop(tasks[idx].convert_in.sym_ranges_opl, sym->sym_ranges.count);
        }
        else
        {
          tasks[idx].convert_in.sym             = sym_for_unit[idx-global_stream_subdivision_tasks_count];
          tasks[idx].convert_in.sym_ranges_first= 0;
          tasks[idx].convert_in.sym_ranges_opl  = sym_for_unit[idx-global_stream_subdivision_tasks_count]->sym_ranges.count;
        }
      }
    }
    
    ////////////////////////////
    //- rjf: produce task batch info
    //
    U64 num_tasks_taken = 0;
    P2R_SymbolStreamTaskBatch task_batch = {0};
    {
      task_batch.tasks               = tasks;
      task_batch.tasks_count         = tasks_count;
      task_batch.num_tasks_taken_ptr = &num_tasks_taken;
    }
    
    ////////////////////////////
    //- rjf: launch task threads
    //
    U64 task_threads_count = os_logical_core_count()-1;
    OS_Handle *task_threads = push_array(scratch.arena, OS_Handle, task_threads_count);
    for(U64 idx = 0; idx < task_threads_count; idx += 1)
    {
      task_threads[idx] = os_launch_thread(p2r_symbol_stream_convert_task_thread__entry_point, &task_batch, 0);
    }
    
    ////////////////////////////
    //- rjf: perform tasks on the main thread
    //
    ProfScope("perform tasks on the main thread")
    {
      for(;;)
      {
        U64 next_task_num = ins_atomic_u64_inc_eval(task_batch.num_tasks_taken_ptr);
        if(next_task_num > task_batch.tasks_count || 1 > next_task_num)
        {
          break;
        }
        P2R_SymbolStreamTask *task = &task_batch.tasks[next_task_num-1];
        ProfScope("convert (task #%I64u)", next_task_num)
        {
          task->convert_out = p2r_symbol_stream_convert(task->out_arena, &task->convert_in);
        }
      }
    }
    
    ////////////////////////////
    //- rjf: wait for task threads
    //
    ProfScope("wait for task threads")
    {
      for(U64 idx = 0; idx < task_threads_count; idx += 1)
      {
        os_thread_wait(task_threads[idx], max_U64);
      }
    }
    
    ////////////////////////////
    //- rjf: merge artifacts from all tasks
    //
    ProfScope("merge artifacts from all tasks")
    {
      for(U64 idx = 0; idx < tasks_count; idx += 1)
      {
        arena_absorb(arena, tasks[idx].out_arena);
        rdim_symbol_chunk_list_concat_in_place(&all_procedures,       &tasks[idx].convert_out->procedures);
        rdim_symbol_chunk_list_concat_in_place(&all_global_variables, &tasks[idx].convert_out->global_variables);
        rdim_symbol_chunk_list_concat_in_place(&all_thread_variables, &tasks[idx].convert_out->thread_variables);
        rdim_scope_chunk_list_concat_in_place(&all_scopes,            &tasks[idx].convert_out->scopes);
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: fill output
  //
  P2R_ConvertOut *out = push_array(arena, P2R_ConvertOut, 1);
  {
    out->top_level_info   = top_level_info;
    out->binary_sections  = binary_sections;
    out->units            = units;
    out->types            = all_types;
    out->udts             = all_udts;
    out->global_variables = all_global_variables;
    out->thread_variables = all_thread_variables;
    out->procedures       = all_procedures;
    out->scopes           = all_scopes;
  }
  
  scratch_end(scratch);
  return out;
}
