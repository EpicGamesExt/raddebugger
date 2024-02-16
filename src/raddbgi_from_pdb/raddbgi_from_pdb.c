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

#if 0

////////////////////////////////
//~ rjf: Conversion Implementation Helpers

//- rjf: pdb conversion context creation

internal P2R_Ctx *
p2r_ctx_alloc(P2R_CtxParams *params, RDIM_Root *out_root)
{
  Arena *arena = arena_alloc();
  P2R_Ctx *pdb_ctx = push_array(arena, P2R_Ctx, 1);
  pdb_ctx->arena = arena;
  pdb_ctx->arch = params->arch;
  pdb_ctx->addr_size = rdi_addr_size_from_arch(pdb_ctx->arch);
  pdb_ctx->hash = params->tpi_hash;
  pdb_ctx->leaf = params->tpi_leaf;
  pdb_ctx->sections = params->sections->sections;
  pdb_ctx->section_count = params->sections->count;
  pdb_ctx->root = out_root;
#define BKTCOUNT(x) ((x)?(u64_up_to_pow2(x)):(4096))
  pdb_ctx->fwd_map.buckets_count        = BKTCOUNT(params->fwd_map_bucket_count);
  pdb_ctx->frame_proc_map.buckets_count = BKTCOUNT(params->frame_proc_map_bucket_count);
  pdb_ctx->known_globals.buckets_count  = BKTCOUNT(params->known_global_map_bucket_count);
  pdb_ctx->link_names.buckets_count     = BKTCOUNT(params->link_name_map_bucket_count);
#undef BKTCOUNT
  pdb_ctx->fwd_map.buckets = push_array(pdb_ctx->arena, P2R_FwdNode *, pdb_ctx->fwd_map.buckets_count);
  pdb_ctx->frame_proc_map.buckets = push_array(pdb_ctx->arena, P2R_FrameProcNode *, pdb_ctx->frame_proc_map.buckets_count);
  pdb_ctx->known_globals.buckets = push_array(pdb_ctx->arena, P2R_KnownGlobalNode *, pdb_ctx->known_globals.buckets_count);
  pdb_ctx->link_names.buckets = push_array(pdb_ctx->arena, P2R_LinkNameNode *, pdb_ctx->link_names.buckets_count);
  return pdb_ctx;
}

//- rjf: pdb types and symbols

internal void
p2r_types_and_symbols(P2R_Ctx *pdb_ctx, P2R_TypesSymbolsParams *params)
{
  ProfBeginFunction();
  
  // convert types
  p2r_type_cons_main_passes(pdb_ctx);
  if(params->sym != 0)
  {
    p2r_gather_link_names(pdb_ctx, params->sym);
    p2r_symbol_cons(pdb_ctx, params->sym, 0);
  }
  U64 unit_count = params->unit_count;
  for(U64 i = 0; i < unit_count; i += 1)
  {
    CV_SymParsed *unit_sym = params->sym_for_unit[i];
    p2r_symbol_cons(pdb_ctx, unit_sym, 1 + i);
  }
  
  ProfEnd();
}

//- rjf: decoding helpers

internal U32
p2r_u32_from_numeric(P2R_Ctx *ctx, CV_NumericParsed *num)
{
  U64 n_u64 = cv_u64_from_numeric(num);
  U32 n_u32 = (U32)n_u64;
  if(n_u64 > 0xFFFFFFFF)
  {
    rdim_push_msgf(ctx->root, "constant too large");
    n_u32 = 0;
  }
  return(n_u32);
}

internal COFF_SectionHeader *
p2r_sec_header_from_sec_num(P2R_Ctx *ctx, U32 sec_num)
{
  COFF_SectionHeader *result = 0;
  if(0 < sec_num && sec_num <= ctx->section_count)
  {
    result = ctx->sections + sec_num - 1;
  }
  return(result);
}

//- rjf: type info

internal void
p2r_type_cons_main_passes(P2R_Ctx *ctx)
{
  ProfBeginFunction();
  CV_TypeId itype_first = ctx->leaf->itype_first;
  CV_TypeId itype_opl = ctx->leaf->itype_opl;
  
  // setup variadic itype -> node
  ProfScope("setup variadic itype -> node")
  {
    RDIM_Type *variadic_type = rdim_type_variadic(ctx->root);
    RDIM_Reservation *res = rdim_type_reserve_id(ctx->root, CV_TypeId_Variadic, CV_TypeId_Variadic);
    rdim_type_fill_id(ctx->root, res, variadic_type);
  }
  
  // resolve forward references
  ProfScope("resolve forward references")
  {
    for(CV_TypeId itype = itype_first; itype < itype_opl; itype += 1)
    {
      p2r_type_resolve_fwd(ctx, itype);
    }
  }
  
  // construct type info
  ProfScope("construct type info")
  {
    for(CV_TypeId itype = itype_first; itype < itype_opl; itype += 1)
    {
      p2r_type_resolve_itype(ctx, itype);
    }
  }
  
  // construct member info
  ProfScope("construct member info")
  {
    for(P2R_TypeRev *rev = ctx->member_revisit_first;
        rev != 0;
        rev = rev->next)
    {
      p2r_type_equip_members(ctx, rev->owner_type, rev->field_itype);
    }
  }
  
  // construct enum info
  ProfScope("construct enum info")
  {
    for(P2R_TypeRev *rev = ctx->enum_revisit_first;
        rev != 0;
        rev = rev->next)
    {
      p2r_type_equip_enumerates(ctx, rev->owner_type, rev->field_itype);
    }
  }
  
  // TODO(allen): equip udts with location information
  ProfEnd();
}

internal CV_TypeId
p2r_type_resolve_fwd(P2R_Ctx *ctx, CV_TypeId itype)
{
  ProfBeginFunction();
  Assert(ctx->leaf->itype_first <= itype && itype < ctx->leaf->itype_opl);
  
  CV_TypeId result = 0;
  
  CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[itype - ctx->leaf->itype_first];
  String8 data = ctx->leaf->data;
  if(range->off + range->hdr.size <= data.size)
  {
    U8 *first = data.str + range->off + 2;
    U64 cap = range->hdr.size - 2;
    
    // figure out if this itype resolves to another
    switch (range->hdr.kind)
    {
      default:break;
      
      case CV_LeafKind_CLASS:
      case CV_LeafKind_STRUCTURE:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafStruct) <= cap)
        {
          CV_LeafStruct *lf_struct = (CV_LeafStruct*)first;
          
          // size
          U8 *numeric_ptr = (U8*)(lf_struct + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          
          // name
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // unique name
          U8 *unique_name_ptr = name_ptr + name.size + 1;
          String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, first + cap);
          
          if(lf_struct->props & CV_TypeProp_FwdRef)
          {
            B32 do_unique_name_lookup = ((lf_struct->props & CV_TypeProp_Scoped) != 0) &&
            ((lf_struct->props & CV_TypeProp_HasUniqueName) != 0);
            if(do_unique_name_lookup)
            {
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, unique_name, 1);
            }
            else
            {
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, name, 0);
            }
          }
        }
      }break;
      
      case CV_LeafKind_CLASS2:
      case CV_LeafKind_STRUCT2:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafStruct2) <= cap)
        {
          CV_LeafStruct2 *lf_struct = (CV_LeafStruct2*)first;
          
          // size
          U8 *numeric_ptr = (U8*)(lf_struct + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          
          // name
          U8 *name_ptr = (U8 *)numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // unique name
          U8 *unique_name_ptr = name_ptr + name.size + 1;
          String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, first + cap);
          
          if(lf_struct->props & CV_TypeProp_FwdRef)
          {
            B32 do_unique_name_lookup = ((lf_struct->props & CV_TypeProp_Scoped) != 0) &&
            ((lf_struct->props & CV_TypeProp_HasUniqueName) != 0);
            if(do_unique_name_lookup)
            {
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, unique_name, 1);
            }
            else
            {
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, name, 0);
            }
          }
        }
      }break;
      
      case CV_LeafKind_UNION:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafUnion) <= cap)
        {
          CV_LeafUnion *lf_union = (CV_LeafUnion*)first;
          
          // size
          U8 *numeric_ptr = (U8*)(lf_union + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          
          // name
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // unique name
          U8 *unique_name_ptr = name_ptr + name.size + 1;
          String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, first + cap);
          
          if(lf_union->props & CV_TypeProp_FwdRef)
          {
            B32 do_unique_name_lookup = ((lf_union->props & CV_TypeProp_Scoped) != 0) &&
            ((lf_union->props & CV_TypeProp_HasUniqueName) != 0);
            if(do_unique_name_lookup)
            {
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, unique_name, 1);
            }
            else
            {
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, name, 0);
            }
          }
        }
      }break;
      
      case CV_LeafKind_ENUM:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafEnum) <= cap)
        {
          CV_LeafEnum *lf_enum = (CV_LeafEnum*)first;
          
          // name
          U8 *name_ptr = (U8*)(lf_enum + 1);
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // unique name
          U8 *unique_name_ptr = name_ptr + name.size + 1;
          String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, first + cap);
          
          if(lf_enum->props & CV_TypeProp_FwdRef)
          {
            B32 do_unique_name_lookup = ((lf_enum->props & CV_TypeProp_Scoped) != 0) &&
            ((lf_enum->props & CV_TypeProp_HasUniqueName) != 0);
            if(do_unique_name_lookup)
            {
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, unique_name, 1);
            }
            else
            {
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, name, 0);
            }
          }
        }
      }break;
    }
  }
  
  // save in map
  if(result != 0)
  {
    p2r_type_fwd_map_set(ctx->arena, &ctx->fwd_map, itype, result);
  }
  
  ProfEnd();
  return(result);
}

internal RDIM_Type*
p2r_type_resolve_itype(P2R_Ctx *ctx, CV_TypeId itype)
{
  B32 is_basic = (itype < 0x1000);
  
  // convert fwd references to real types
  if(!is_basic)
  {
    CV_TypeId resolved_itype = p2r_type_fwd_map_get(&ctx->fwd_map, itype);
    if(resolved_itype != 0)
    {
      itype = resolved_itype;
    }
  }
  
  // type handle from id
  RDIM_Type *result = rdim_type_from_id(ctx->root, itype, itype);
  
  // basic type
  if(result == 0 && is_basic)
  {
    result = p2r_type_cons_basic(ctx, itype);
  }
  
  // leaf decode
  if(result == 0 && (ctx->leaf->itype_first <= itype && itype < ctx->leaf->itype_opl))
  {
    result = p2r_type_cons_leaf_record(ctx, itype);
  }
  
  // never return null, return "nil" instead
  if(result == 0)
  {
    result = rdim_type_nil(ctx->root);
  }
  
  return(result);
}

internal void
p2r_type_equip_members(P2R_Ctx *ctx, RDIM_Type *owner_type, CV_TypeId field_itype)
{
  Temp scratch = scratch_begin(0, 0);
  
  String8 data = ctx->leaf->data;
  
  // field stack
  // TODO(allen): add notes about field tasks
  struct FieldTask{
    struct FieldTask *next;
    CV_TypeId itype;
  };
  struct FieldTask *handled = 0;
  struct FieldTask *todo = 0;
  {
    struct FieldTask *task = push_array(scratch.arena, struct FieldTask, 1);
    SLLStackPush(todo, task);
    task->itype = field_itype;
  }
  
  for(;;)
  {
    // exit condition
    if(todo == 0)
    {
      break;
    }
    
    // determine itype
    CV_TypeId field_itype = todo->itype;
    {
      struct FieldTask *task = todo;
      SLLStackPop(todo);
      SLLStackPush(handled, task);
    }
    
    // get leaf range
    // TODO(allen): error if this itype is bad
    U8 *first = 0;
    U64 cap = 0;
    if(ctx->leaf->itype_first <= field_itype && field_itype < ctx->leaf->itype_opl)
    {
      CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[field_itype - ctx->leaf->itype_first];
      // check valid arglist
      if(range->hdr.kind == CV_LeafKind_FIELDLIST &&
         range->off + range->hdr.size <= data.size)
      {
        first = data.str + range->off + 2;
        cap = range->hdr.size - 2;
      }
    }
    
    U64 cursor = 0;
    for(;cursor + sizeof(CV_LeafKind) <= cap;)
    {
      CV_LeafKind field_kind = *(CV_LeafKind*)(first + cursor);
      
      U64 list_item_off = cursor + 2;
      // if we hit an error or forget to set next cursor for a case
      // default to exiting the loop
      U64 list_item_opl_off = cap;
      
      switch (field_kind)
      {
        case CV_LeafKind_INDEX:
        {
          // TODO(allen): error if bad range
          if(list_item_off + sizeof(CV_LeafIndex) <= cap)
          {
            // compute whole layout
            CV_LeafIndex *index = (CV_LeafIndex*)(first + list_item_off);
            
            list_item_opl_off = list_item_off + sizeof(*index);
            
            // create new todo task
            CV_TypeId new_itype = index->itype;
            B32 is_new = 1;
            for(struct FieldTask *task = handled;
                task != 0;
                task = task->next)
            {
              if(task->itype == new_itype)
              {
                is_new = 0;
                break;
              }
            }
            if(is_new)
            {
              struct FieldTask *task = push_array(scratch.arena, struct FieldTask, 1);
              SLLStackPush(todo, task);
              task->itype = new_itype;
            }
          }
        }break;
        
        case CV_LeafKind_MEMBER:
        {
          // TODO(allen): error if bad range
          if(list_item_off + sizeof(CV_LeafMember) <= cap)
          {
            // compute whole layout
            CV_LeafMember *member = (CV_LeafMember*)(first + list_item_off);
            
            U64 offset_off = list_item_off + sizeof(*member);
            CV_NumericParsed offset = cv_numeric_from_data_range(first + offset_off, first + cap);
            
            U64 name_off = offset_off + offset.encoded_size;
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // emit member
            RDIM_Type *mem_type = p2r_type_resolve_itype(ctx, member->itype);
            U32 offset_u32 = p2r_u32_from_numeric(ctx, &offset);
            rdim_type_add_member_data_field(ctx->root, owner_type, name, mem_type, offset_u32);
          }
        }break;
        
        case CV_LeafKind_STMEMBER:
        {
          // TODO(allen): error if bad range
          if(list_item_off + sizeof(CV_LeafStMember) <= cap)
          {
            // compute whole layout
            CV_LeafStMember *stmember = (CV_LeafStMember*)(first + list_item_off);
            
            U64 name_off = list_item_off + sizeof(*stmember);
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // TODO(allen): handle attribs
            
            // emit member
            RDIM_Type *mem_type = p2r_type_resolve_itype(ctx, stmember->itype);
            rdim_type_add_member_static_data(ctx->root, owner_type, name, mem_type);
          }
        }break;
        
        case CV_LeafKind_METHOD:
        {
          // TODO(allen): error if bad range
          if(list_item_off + sizeof(CV_LeafMethod) <= cap)
          {
            // compute whole layout
            CV_LeafMethod *method = (CV_LeafMethod*)(first + list_item_off);
            
            U64 name_off = list_item_off + sizeof(*method);
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // extract method list
            U8 *first = 0;
            U64 cap = 0;
            
            // TODO(allen): error if bad itype
            CV_TypeId methodlist_itype = method->list_itype;
            if(ctx->leaf->itype_first <= methodlist_itype &&
               methodlist_itype < ctx->leaf->itype_opl)
            {
              CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[methodlist_itype - ctx->leaf->itype_first];
              
              // check valid methodlist
              if(range->hdr.kind == CV_LeafKind_METHODLIST &&
                 range->off + range->hdr.size <= data.size)
              {
                first = data.str + range->off + 2;
                cap = range->hdr.size - 2;
              }
            }
            
            // emit loop
            U64 cursor = 0;
            for(;cursor + sizeof(CV_LeafMethodListMember) <= cap;)
            {
              CV_LeafMethodListMember *method = (CV_LeafMethodListMember*)(first + cursor);
              
              CV_MethodProp prop = CV_FieldAttribs_ExtractMethodProp(method->attribs);
              
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
              
              // extract vbaseoff
              U64 next_cursor = cursor + sizeof(*method);
              U32 vbaseoff = 0;
              if(prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro)
              {
                if(cursor + sizeof(*method) + 4 <= cap)
                {
                  vbaseoff = *(U32*)(method + 1);
                }
                next_cursor += 4;
              }
              
              // update cursor
              cursor = next_cursor;
              
              // TODO(allen): handle attribs
              
              // emit
              RDIM_Type *mem_type = p2r_type_resolve_itype(ctx, method->itype);
              
              switch (prop)
              {
                default:
                {
                  rdim_type_add_member_method(ctx->root, owner_type, name, mem_type);
                }break;
                
                case CV_MethodProp_Static:
                {
                  rdim_type_add_member_static_method(ctx->root, owner_type, name, mem_type);
                }break;
                
                case CV_MethodProp_Virtual:
                case CV_MethodProp_PureVirtual:
                case CV_MethodProp_Intro:
                case CV_MethodProp_PureIntro:
                {
                  rdim_type_add_member_virtual_method(ctx->root, owner_type, name, mem_type);
                }break;
              }
            }
            
          }
        }break;
        
        case CV_LeafKind_ONEMETHOD:
        {
          // TODO(allen): error if bad range
          if(list_item_off + sizeof(CV_LeafOneMethod) <= cap)
          {
            // compute whole layout
            CV_LeafOneMethod *one_method = (CV_LeafOneMethod*)(first + list_item_off);
            
            CV_MethodProp prop = CV_FieldAttribs_ExtractMethodProp(one_method->attribs);
            
            U64 vbaseoff_off = list_item_off + sizeof(*one_method);
            U64 vbaseoff_opl_off = vbaseoff_off;
            U32 vbaseoff = 0;
            if(prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro)
            {
              vbaseoff = *(U32*)(first + vbaseoff_off);
              vbaseoff_opl_off += sizeof(vbaseoff);
            }
            
            U64 name_off = vbaseoff_opl_off;
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // TODO(allen): handle attribs
            
            // emit
            RDIM_Type *mem_type = p2r_type_resolve_itype(ctx, one_method->itype);
            
            switch (prop)
            {
              default:
              {
                rdim_type_add_member_method(ctx->root, owner_type, name, mem_type);
              }break;
              
              case CV_MethodProp_Static:
              {
                rdim_type_add_member_static_method(ctx->root, owner_type, name, mem_type);
              }break;
              
              case CV_MethodProp_Virtual:
              case CV_MethodProp_PureVirtual:
              case CV_MethodProp_Intro:
              case CV_MethodProp_PureIntro:
              {
                rdim_type_add_member_virtual_method(ctx->root, owner_type, name, mem_type);
              }break;
            }
          }
        }break;
        
        case CV_LeafKind_NESTTYPE:
        {
          // TODO(allen): error if bad range
          if(list_item_off + sizeof(CV_LeafNestType) <= cap)
          {
            // compute whole layout
            CV_LeafNestType *nest_type = (CV_LeafNestType*)(first + list_item_off);
            
            U64 name_off = list_item_off + sizeof(*nest_type);
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // emit member
            RDIM_Type *mem_type = p2r_type_resolve_itype(ctx, nest_type->itype);
            rdim_type_add_member_nested_type(ctx->root, owner_type, mem_type);
          }
        }break;
        
        case CV_LeafKind_NESTTYPEEX:
        {
          // TODO(allen): error if bad range
          if(list_item_off + sizeof(CV_LeafNestTypeEx) <= cap)
          {
            // compute whole layout
            CV_LeafNestTypeEx *nest_type = (CV_LeafNestTypeEx*)(first + list_item_off);
            
            U64 name_off = list_item_off + sizeof(*nest_type);
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // TODO(allen): handle attribs
            
            // emit member
            RDIM_Type *mem_type = p2r_type_resolve_itype(ctx, nest_type->itype);
            rdim_type_add_member_nested_type(ctx->root, owner_type, mem_type);
          }
        }break;
        
        case CV_LeafKind_BCLASS:
        {
          // TODO(allen): error if bad range
          if(list_item_off + sizeof(CV_LeafBClass) <= cap)
          {
            // compute whole layout
            CV_LeafBClass *bclass = (CV_LeafBClass*)(first + list_item_off);
            
            U64 offset_off = list_item_off + sizeof(*bclass);
            CV_NumericParsed offset = cv_numeric_from_data_range(first + offset_off, first + cap);
            
            list_item_opl_off = offset_off + offset.encoded_size;
            
            // TODO(allen): handle attribs
            
            // emit member
            RDIM_Type *base_type = p2r_type_resolve_itype(ctx, bclass->itype);
            U32 offset_u32 = p2r_u32_from_numeric(ctx, &offset);
            rdim_type_add_member_base(ctx->root, owner_type, base_type, offset_u32);
          }
        }break;
        
        case CV_LeafKind_VBCLASS:
        case CV_LeafKind_IVBCLASS:
        {
          // TODO(allen): error if bad range
          if(list_item_off + sizeof(CV_LeafVBClass) <= cap)
          {
            // compute whole layout
            CV_LeafVBClass *vbclass = (CV_LeafVBClass*)(first + list_item_off);
            
            U64 num1_off = list_item_off + sizeof(*vbclass);
            CV_NumericParsed num1 = cv_numeric_from_data_range(first + num1_off, first + cap);
            
            U64 num2_off = num1_off + num1.encoded_size;
            CV_NumericParsed num2 = cv_numeric_from_data_range(first + num2_off, first + cap);
            
            list_item_opl_off = num2_off + num2.encoded_size;
            
            // TODO(allen): handle attribs
            
            // emit member
            RDIM_Type *base_type = p2r_type_resolve_itype(ctx, vbclass->itype);
            U32 vbptr_offset_u32  = p2r_u32_from_numeric(ctx, &num1);
            U32 vtable_offset_u32 = p2r_u32_from_numeric(ctx, &num2);
            rdim_type_add_member_virtual_base(ctx->root, owner_type, base_type,
                                              vbptr_offset_u32, vtable_offset_u32);
          }
        }break;
        
        // discard cases - we don't currently do anything with these
        case CV_LeafKind_VFUNCTAB:
        {
          // TODO(rjf): error if bad range
          if(list_item_off + sizeof(CV_LeafVFuncTab) <= cap)
          {
            list_item_opl_off = list_item_off + sizeof(CV_LeafVFuncTab);
          }
        }break;
        
        // unhandled or invalid cases
        default:
        {
          String8 kind_str = cv_string_from_leaf_kind(field_kind);
          rdim_push_msgf(ctx->root, "unhandled/invalid case: equip_members -> %.*s",
                         str8_varg(kind_str));
        }break;
      }
      
      // update cursor
      U64 next_cursor = AlignPow2(list_item_opl_off, 4);
      cursor = next_cursor;
    }
  }
  
  scratch_end(scratch);
}

internal void
p2r_type_equip_enumerates(P2R_Ctx *ctx, RDIM_Type *owner_type, CV_TypeId field_itype)
{
  Temp scratch = scratch_begin(0, 0);
  
  String8 data = ctx->leaf->data;
  
  // field stack
  // TODO(allen): add notes about field tasks
  struct FieldTask{
    struct FieldTask *next;
    CV_TypeId itype;
  };
  struct FieldTask *handled = 0;
  struct FieldTask *todo = 0;
  {
    struct FieldTask *task = push_array(scratch.arena, struct FieldTask, 1);
    SLLStackPush(todo, task);
    task->itype = field_itype;
  }
  
  for(;;)
  {
    // exit condition
    if(todo == 0)
    {
      break;
    }
    
    // determine itype
    CV_TypeId field_itype = todo->itype;
    {
      struct FieldTask *task = todo;
      SLLStackPop(todo);
      SLLStackPush(handled, task);
    }
    
    // get leaf range
    // TODO(allen): error if this itype is bad
    U8 *first = 0;
    U64 cap = 0;
    if(ctx->leaf->itype_first <= field_itype && field_itype < ctx->leaf->itype_opl)
    {
      CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[field_itype - ctx->leaf->itype_first];
      // check valid arglist
      if(range->hdr.kind == CV_LeafKind_FIELDLIST &&
         range->off + range->hdr.size <= data.size)
      {
        first = data.str + range->off + 2;
        cap = range->hdr.size - 2;
      }
    }
    
    U64 cursor = 0;
    for(;cursor + sizeof(CV_LeafKind) <= cap;)
    {
      CV_LeafKind field_kind = *(CV_LeafKind*)(first + cursor);
      
      U64 list_item_off = cursor + 2;
      // if we hit an error or forget to set next cursor for a case
      // default to exiting the loop
      U64 list_item_opl_off = cap;
      
      switch (field_kind)
      {
        case CV_LeafKind_INDEX:
        {
          // TODO(allen): error if bad range
          if(list_item_off + sizeof(CV_LeafIndex) <= cap)
          {
            // compute whole layout
            CV_LeafIndex *index = (CV_LeafIndex*)(first + list_item_off);
            
            list_item_opl_off = list_item_off + sizeof(*index);
            
            // create new todo task
            CV_TypeId new_itype = index->itype;
            B32 is_new = 1;
            for(struct FieldTask *task = handled;
                task != 0;
                task = task->next)
            {
              if(task->itype == new_itype)
              {
                is_new = 0;
                break;
              }
            }
            if(is_new)
            {
              struct FieldTask *task = push_array(scratch.arena, struct FieldTask, 1);
              SLLStackPush(todo, task);
              task->itype = new_itype;
            }
          }
        }break;
        
        case CV_LeafKind_ENUMERATE:
        {
          // compute whole layout
          CV_LeafEnumerate *enumerate = (CV_LeafEnumerate*)(first + list_item_off);
          
          U64 val_off = list_item_off + sizeof(*enumerate);
          CV_NumericParsed val = cv_numeric_from_data_range(first + val_off, first + cap);
          
          U64 name_off = val_off + val.encoded_size;
          String8 name = str8_cstring_capped(first + name_off, first + cap);
          
          list_item_opl_off = name_off + name.size + 1;
          
          // TODO(allen): handle attribs
          
          // emit enum val
          U64 val_u64 = cv_u64_from_numeric(&val);
          rdim_type_add_enum_val(ctx->root, owner_type, name, val_u64);
        }break;
        
        // unhandled or invalid cases
        default:
        {
          String8 kind_str = cv_string_from_leaf_kind(field_kind);
          rdim_push_msgf(ctx->root, "unhandled/invalid case: equip_enumerates -> %.*s",
                         str8_varg(kind_str));
        }break;
      }
      
      // update cursor
      U64 next_cursor = AlignPow2(list_item_opl_off, 4);
      cursor = next_cursor;
    }
  }
  
  scratch_end(scratch);
}

internal RDIM_Type*
p2r_type_cons_basic(P2R_Ctx *ctx, CV_TypeId itype)
{
  Assert(itype < 0x1000);
  
  CV_BasicPointerKind basic_ptr_kind = CV_BasicPointerKindFromTypeId(itype);
  CV_BasicType basic_type_code = CV_BasicTypeFromTypeId(itype);
  
  RDIM_Reservation *basic_res = rdim_type_reserve_id(ctx->root, basic_type_code, basic_type_code);
  
  RDIM_Type *basic_type = 0;
  switch (basic_type_code)
  {
    case CV_BasicType_VOID:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_Void, str8_lit("void"));
    }break;
    
    case CV_BasicType_HRESULT:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_Handle, str8_lit("HRESULT"));
    }break;
    
    case CV_BasicType_RCHAR:
    case CV_BasicType_CHAR:
    case CV_BasicType_CHAR8:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_Char8, str8_lit("char"));
    }break;
    
    case CV_BasicType_UCHAR:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_UChar8, str8_lit("UCHAR"));
    }break;
    
    case CV_BasicType_WCHAR:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_UChar16, str8_lit("WCHAR"));
    }break;
    
    case CV_BasicType_CHAR16:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_Char16, str8_lit("CHAR16"));
    }break;
    
    case CV_BasicType_CHAR32:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_Char32, str8_lit("CHAR32"));
    }break;
    
    case CV_BasicType_BOOL8:
    case CV_BasicType_INT8:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_S8, str8_lit("S8"));
    }break;
    
    case CV_BasicType_BOOL16:
    case CV_BasicType_INT16:
    case CV_BasicType_SHORT:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_S16, str8_lit("S16"));
    }break;
    
    case CV_BasicType_BOOL32:
    case CV_BasicType_INT32:
    case CV_BasicType_LONG:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_S32, str8_lit("S32"));
    }break;
    
    case CV_BasicType_BOOL64:
    case CV_BasicType_INT64:
    case CV_BasicType_QUAD:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_S64, str8_lit("S64"));
    }break;
    
    case CV_BasicType_INT128:
    case CV_BasicType_OCT:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_S128, str8_lit("S128"));
    }break;
    
    case CV_BasicType_UINT8:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_U8, str8_lit("U8"));
    }break;
    
    case CV_BasicType_UINT16:
    case CV_BasicType_USHORT:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_U16, str8_lit("U16"));
    }break;
    
    case CV_BasicType_UINT32:
    case CV_BasicType_ULONG:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_U32, str8_lit("U32"));
    }break;
    
    case CV_BasicType_UINT64:
    case CV_BasicType_UQUAD:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_U64, str8_lit("U64"));
    }break;
    
    case CV_BasicType_UINT128:
    case CV_BasicType_UOCT:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_U128, str8_lit("U128"));
    }break;
    
    case CV_BasicType_FLOAT16:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_F16, str8_lit("F16"));
    }break;
    
    case CV_BasicType_FLOAT32:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_F32, str8_lit("F32"));
    }break;
    
    case CV_BasicType_FLOAT32PP:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_F32PP, str8_lit("F32PP"));
    }break;
    
    case CV_BasicType_FLOAT48:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_F48, str8_lit("F48"));
    }break;
    
    case CV_BasicType_FLOAT64:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_F64, str8_lit("F64"));
    }break;
    
    case CV_BasicType_FLOAT80:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_F80, str8_lit("F80"));
    }break;
    
    case CV_BasicType_FLOAT128:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_F128, str8_lit("F128"));
    }break;
    
    case CV_BasicType_COMPLEX32:
    {
      basic_type =
        rdim_type_basic(ctx->root, RDI_TypeKind_ComplexF32, str8_lit("ComplexF32"));
    }break;
    
    case CV_BasicType_COMPLEX64:
    {
      basic_type =
        rdim_type_basic(ctx->root, RDI_TypeKind_ComplexF64, str8_lit("ComplexF64"));
    }break;
    
    case CV_BasicType_COMPLEX80:
    {
      basic_type =
        rdim_type_basic(ctx->root, RDI_TypeKind_ComplexF80, str8_lit("ComplexF80"));
    }break;
    
    case CV_BasicType_COMPLEX128:
    {
      basic_type =
        rdim_type_basic(ctx->root, RDI_TypeKind_ComplexF128, str8_lit("ComplexF128"));
    }break;
    
    case CV_BasicType_PTR:
    {
      basic_type = rdim_type_basic(ctx->root, RDI_TypeKind_Handle, str8_lit("PTR"));
    }break;
  }
  
  // basic resolve
  rdim_type_fill_id(ctx->root, basic_res, basic_type);
  
  // wrap in constructed type
  RDIM_Type *constructed_type = 0;
  if(basic_ptr_kind != 0 && basic_type != 0)
  {
    RDIM_Reservation *constructed_res = rdim_type_reserve_id(ctx->root, itype, itype);
    
    switch (basic_ptr_kind)
    {
      case CV_BasicPointerKind_16BIT:
      case CV_BasicPointerKind_FAR_16BIT:
      case CV_BasicPointerKind_HUGE_16BIT:
      case CV_BasicPointerKind_32BIT:
      case CV_BasicPointerKind_16_32BIT:
      case CV_BasicPointerKind_64BIT:
      {
        constructed_type = rdim_type_pointer(ctx->root, basic_type, RDI_TypeKind_Ptr);
      }break;
    }
    
    // constructed resolve
    rdim_type_fill_id(ctx->root, constructed_res, constructed_type);
  }
  
  // select output
  RDIM_Type *result = basic_type;
  if(basic_ptr_kind != 0)
  {
    result = constructed_type;
  }
  
  return(result);
}

internal RDIM_Type*
p2r_type_cons_leaf_record(P2R_Ctx *ctx, CV_TypeId itype)
{
  Assert(ctx->leaf->itype_first <= itype && itype < ctx->leaf->itype_opl);
  
  RDIM_Reservation *res = rdim_type_reserve_id(ctx->root, itype, itype);
  
  CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[itype - ctx->leaf->itype_first];
  String8 data = ctx->leaf->data;
  
  RDIM_Type *result = 0;
  if(range->off + range->hdr.size <= data.size)
  {
    U8 *first = data.str + range->off + 2;
    U64 cap = range->hdr.size - 2;
    
    switch (range->hdr.kind)
    {
      case CV_LeafKind_MODIFIER:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafModifier) <= cap)
        {
          CV_LeafModifier *modifier = (CV_LeafModifier*)first;
          
          RDI_TypeModifierFlags flags = 0;
          if(modifier->flags & CV_ModifierFlag_Const)
          {
            flags |= RDI_TypeModifierFlag_Const;
          }
          if(modifier->flags & CV_ModifierFlag_Volatile)
          {
            flags |= RDI_TypeModifierFlag_Volatile;
          }
          
          RDIM_Type *direct_type = p2r_type_resolve_and_check(ctx, modifier->itype);
          if(flags != 0)
          {
            result = rdim_type_modifier(ctx->root, direct_type, flags);
          }
          else
          {
            result = direct_type;
          }
        }
      }break;
      
      case CV_LeafKind_POINTER:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafPointer) <= cap)
        {
          CV_LeafPointer *pointer = (CV_LeafPointer*)first;
          
          CV_PointerKind ptr_kind = CV_PointerAttribs_ExtractKind(pointer->attribs);
          CV_PointerMode ptr_mode = CV_PointerAttribs_ExtractMode(pointer->attribs);
          U32 ptr_size            = CV_PointerAttribs_ExtractSize(pointer->attribs);
          
          // TODO(allen): if ptr_mode in {PtrMem, PtrMethod} then output a member pointer instead
          
          // extract modifier flags
          RDI_TypeModifierFlags modifier_flags = 0;
          if(pointer->attribs & CV_PointerAttrib_Const)
          {
            modifier_flags |= RDI_TypeModifierFlag_Const;
          }
          if(pointer->attribs & CV_PointerAttrib_Volatile)
          {
            modifier_flags |= RDI_TypeModifierFlag_Volatile;
          }
          
          // determine type kind
          RDI_TypeKind type_kind = RDI_TypeKind_Ptr;
          if(pointer->attribs & CV_PointerAttrib_LRef)
          {
            type_kind = RDI_TypeKind_LRef;
          }
          else if(pointer->attribs & CV_PointerAttrib_RRef)
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
          
          RDIM_Type *direct_type = p2r_type_resolve_and_check(ctx, pointer->itype);
          RDIM_Type *ptr_type = rdim_type_pointer(ctx->root, direct_type, type_kind);
          
          result = ptr_type;
          if(modifier_flags != 0)
          {
            result = rdim_type_modifier(ctx->root, ptr_type, modifier_flags);
          }
        }
      }break;
      
      case CV_LeafKind_PROCEDURE:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafProcedure) <= cap)
        {
          CV_LeafProcedure *procedure = (CV_LeafProcedure*)first;
          
          Temp scratch = scratch_begin(0, 0);
          
          // TODO(allen): handle call_kind & attribs
          
          RDIM_Type *ret_type = p2r_type_resolve_and_check(ctx, procedure->ret_itype);
          
          RDIM_TypeList param_list = {0};
          p2r_type_resolve_arglist(scratch.arena, &param_list, ctx, procedure->arg_itype);
          
          result = rdim_type_proc(ctx->root, ret_type, &param_list);
          
          scratch_end(scratch);
        }
      }break;
      
      case CV_LeafKind_MFUNCTION:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafMFunction) <= cap)
        {
          CV_LeafMFunction *mfunction = (CV_LeafMFunction*)first;
          
          Temp scratch = scratch_begin(0, 0);
          
          // TODO(allen): handle call_kind & attribs
          // TODO(allen): preserve "this_adjust"
          
          RDIM_Type *ret_type = p2r_type_resolve_and_check(ctx, mfunction->ret_itype);
          
          RDIM_TypeList param_list = {0};
          p2r_type_resolve_arglist(scratch.arena, &param_list, ctx, mfunction->arg_itype);
          
          RDIM_Type *this_type = 0;
          if(mfunction->this_itype != 0)
          {
            this_type = p2r_type_resolve_and_check(ctx, mfunction->this_itype);
            result = rdim_type_method(ctx->root, this_type, ret_type, &param_list);
          }
          else
          {
            result = rdim_type_proc(ctx->root, ret_type, &param_list);
          }
          
          scratch_end(scratch);
        }
      }break;
      
      case CV_LeafKind_BITFIELD:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafBitField) <= cap)
        {
          CV_LeafBitField *bit_field = (CV_LeafBitField*)first;
          RDIM_Type *direct_type = p2r_type_resolve_and_check(ctx, bit_field->itype);
          result = rdim_type_bitfield(ctx->root, direct_type, bit_field->pos, bit_field->len);
        }
      }break;
      
      case CV_LeafKind_ARRAY:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafArray) <= cap)
        {
          CV_LeafArray *array = (CV_LeafArray*)first;
          
          // parse count
          U8 *numeric_ptr = (U8*)(array + 1);
          CV_NumericParsed array_count = cv_numeric_from_data_range(numeric_ptr, first + cap);
          
          U64 full_size = cv_u64_from_numeric(&array_count);
          
          RDIM_Type *direct_type = p2r_type_resolve_and_check(ctx, array->entry_itype);
          U64 count = full_size;
          if(direct_type != 0 && direct_type->byte_size != 0)
          {
            count /= direct_type->byte_size;
          }
          
          // build type
          result = rdim_type_array(ctx->root, direct_type, count);
        }
      }break;
      
      case CV_LeafKind_CLASS:
      case CV_LeafKind_STRUCTURE:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafStruct) <= cap)
        {
          CV_LeafStruct *lf_struct = (CV_LeafStruct*)first;
          
          // TODO(allen): handle props
          
          // size
          U8 *numeric_ptr = (U8*)(lf_struct + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          U64 size_u64 = cv_u64_from_numeric(&size);
          
          // name
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // incomplete type
          if(lf_struct->props & CV_TypeProp_FwdRef)
          {
            RDI_TypeKind type_kind = RDI_TypeKind_IncompleteStruct;
            if(range->hdr.kind == CV_LeafKind_CLASS)
            {
              type_kind = RDI_TypeKind_IncompleteClass;
            }
            result = rdim_type_incomplete(ctx->root, type_kind, name);
          }
          
          // complete type
          else
          {
            RDI_TypeKind type_kind = RDI_TypeKind_Struct;
            if(range->hdr.kind == CV_LeafKind_CLASS)
            {
              type_kind = RDI_TypeKind_Class;
            }
            result = rdim_type_udt(ctx->root, type_kind, name, size_u64);
            
            // remember to revisit this for members
            {
              P2R_TypeRev *rev = push_array(ctx->arena, P2R_TypeRev, 1);
              rev->owner_type = result;
              rev->field_itype = lf_struct->field_itype;
              SLLQueuePush(ctx->member_revisit_first, ctx->member_revisit_last, rev);
            }
          }
        }
      }break;
      
      case CV_LeafKind_CLASS2:
      case CV_LeafKind_STRUCT2:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafStruct2) <= cap)
        {
          CV_LeafStruct2 *lf_struct = (CV_LeafStruct2*)first;
          
          // TODO(allen): handle props
          
          // size
          U8 *numeric_ptr = (U8*)(lf_struct + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          U64 size_u64 = cv_u64_from_numeric(&size);
          
          // name
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // incomplete type
          if(lf_struct->props & CV_TypeProp_FwdRef)
          {
            RDI_TypeKind type_kind = RDI_TypeKind_IncompleteStruct;
            if(range->hdr.kind == CV_LeafKind_CLASS2)
            {
              type_kind = RDI_TypeKind_IncompleteClass;
            }
            result = rdim_type_incomplete(ctx->root, type_kind, name);
          }
          
          // complete type
          else
          {
            RDI_TypeKind type_kind = RDI_TypeKind_Struct;
            if(range->hdr.kind == CV_LeafKind_CLASS2)
            {
              type_kind = RDI_TypeKind_Class;
            }
            result = rdim_type_udt(ctx->root, type_kind, name, size_u64);
            
            // remember to revisit this for members
            {
              P2R_TypeRev *rev = push_array(ctx->arena, P2R_TypeRev, 1);
              rev->owner_type = result;
              rev->field_itype = lf_struct->field_itype;
              SLLQueuePush(ctx->member_revisit_first, ctx->member_revisit_last, rev);
            }
          }
        }
      }break;
      
      case CV_LeafKind_UNION:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafUnion) <= cap)
        {
          CV_LeafUnion *lf_union = (CV_LeafUnion*)first;
          
          // TODO(allen): handle props
          
          // size
          U8 *numeric_ptr = (U8*)(lf_union + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          U64 size_u64 = cv_u64_from_numeric(&size);
          
          // name
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // incomplete type
          if(lf_union->props & CV_TypeProp_FwdRef)
          {
            result =
              rdim_type_incomplete(ctx->root, RDI_TypeKind_IncompleteUnion, name);
          }
          
          // complete type
          else
          {
            result = rdim_type_udt(ctx->root, RDI_TypeKind_Union, name, size_u64);
            
            // remember to revisit this for members
            {
              P2R_TypeRev *rev = push_array(ctx->arena, P2R_TypeRev, 1);
              rev->owner_type = result;
              rev->field_itype = lf_union->field_itype;
              SLLQueuePush(ctx->member_revisit_first, ctx->member_revisit_last, rev);
            }
          }
        }
      }break;
      
      case CV_LeafKind_ENUM:
      {
        // TODO(allen): error if bad range
        if(sizeof(CV_LeafEnum) <= cap)
        {
          CV_LeafEnum *lf_enum = (CV_LeafEnum*)first;
          
          // TODO(allen): handle props
          
          // name
          U8 *name_ptr = (U8*)(lf_enum + 1);
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // incomplete type
          if(lf_enum->props & CV_TypeProp_FwdRef)
          {
            result = rdim_type_incomplete(ctx->root, RDI_TypeKind_IncompleteEnum, name);
          }
          
          // complete type
          else
          {
            RDIM_Type *direct_type = p2r_type_resolve_and_check(ctx, lf_enum->base_itype);
            result = rdim_type_enum(ctx->root, direct_type, name);
            
            // remember to revisit this for enumerates
            {
              P2R_TypeRev *rev = push_array(ctx->arena, P2R_TypeRev, 1);
              rev->owner_type = result;
              rev->field_itype = lf_enum->field_itype;
              SLLQueuePush(ctx->enum_revisit_first, ctx->enum_revisit_last, rev);
            }
          }
        }
      }break;
      
      // discard cases - we currently discard these these intentionally
      //                 so we mark them as "handled nil"
      case CV_LeafKind_VTSHAPE:
      case CV_LeafKind_VFTABLE:
      case CV_LeafKind_LABEL:
      {
        result = rdim_type_handled_nil(ctx->root);
      }break;
      
      // do nothing cases - these get handled in special passes and
      //                    they should not be appearing as direct types
      //                    or parameter types for any other types
      //                    so if the input data is valid we won't get
      //                    error messages even if they resolve to nil
      case CV_LeafKind_FIELDLIST:
      case CV_LeafKind_ARGLIST:
      case CV_LeafKind_METHODLIST:
      {}break;
      
      // Leaf Kinds
      case CV_LeafKind_MODIFIER_16t:
      case CV_LeafKind_POINTER_16t:
      case CV_LeafKind_ARRAY_16t:
      case CV_LeafKind_CLASS_16t:
      case CV_LeafKind_STRUCTURE_16t:
      case CV_LeafKind_UNION_16t:
      case CV_LeafKind_ENUM_16t:
      case CV_LeafKind_PROCEDURE_16t:
      case CV_LeafKind_MFUNCTION_16t:
      //case CV_LeafKind_VTSHAPE:
      case CV_LeafKind_COBOL0_16t:
      case CV_LeafKind_COBOL1:
      case CV_LeafKind_BARRAY_16t:
      //case CV_LeafKind_LABEL:
      case CV_LeafKind_NULL:
      case CV_LeafKind_NOTTRAN:
      case CV_LeafKind_DIMARRAY_16t:
      case CV_LeafKind_VFTPATH_16t:
      case CV_LeafKind_PRECOMP_16t:
      case CV_LeafKind_ENDPRECOMP:
      case CV_LeafKind_OEM_16t:
      case CV_LeafKind_TYPESERVER_ST:
      case CV_LeafKind_SKIP_16t:
      case CV_LeafKind_ARGLIST_16t:
      case CV_LeafKind_DEFARG_16t:
      case CV_LeafKind_LIST:
      case CV_LeafKind_FIELDLIST_16t:
      case CV_LeafKind_DERIVED_16t:
      case CV_LeafKind_BITFIELD_16t:
      case CV_LeafKind_METHODLIST_16t:
      case CV_LeafKind_DIMCONU_16t:
      case CV_LeafKind_DIMCONLU_16t:
      case CV_LeafKind_DIMVARU_16t:
      case CV_LeafKind_DIMVARLU_16t:
      case CV_LeafKind_REFSYM:
      case CV_LeafKind_BCLASS_16t:
      case CV_LeafKind_VBCLASS_16t:
      case CV_LeafKind_IVBCLASS_16t:
      case CV_LeafKind_ENUMERATE_ST:
      case CV_LeafKind_FRIENDFCN_16t:
      case CV_LeafKind_INDEX_16t:
      case CV_LeafKind_MEMBER_16t:
      case CV_LeafKind_STMEMBER_16t:
      case CV_LeafKind_METHOD_16t:
      case CV_LeafKind_NESTTYPE_16t:
      case CV_LeafKind_VFUNCTAB_16t:
      case CV_LeafKind_FRIENDCLS_16t:
      case CV_LeafKind_ONEMETHOD_16t:
      case CV_LeafKind_VFUNCOFF_16t:
      case CV_LeafKind_TI16_MAX:
      //case CV_LeafKind_MODIFIER:
      //case CV_LeafKind_POINTER:
      case CV_LeafKind_ARRAY_ST:
      case CV_LeafKind_CLASS_ST:
      case CV_LeafKind_STRUCTURE_ST:
      case CV_LeafKind_UNION_ST:
      case CV_LeafKind_ENUM_ST:
      //case CV_LeafKind_PROCEDURE:
      //case CV_LeafKind_MFUNCTION:
      case CV_LeafKind_COBOL0:
      case CV_LeafKind_BARRAY:
      case CV_LeafKind_DIMARRAY_ST:
      case CV_LeafKind_VFTPATH:
      case CV_LeafKind_PRECOMP_ST:
      case CV_LeafKind_OEM:
      case CV_LeafKind_ALIAS_ST:
      case CV_LeafKind_OEM2:
      case CV_LeafKind_SKIP:
      //case CV_LeafKind_ARGLIST:
      case CV_LeafKind_DEFARG_ST:
      //case CV_LeafKind_FIELDLIST:
      case CV_LeafKind_DERIVED:
      //case CV_LeafKind_BITFIELD:
      //case CV_LeafKind_METHODLIST:
      case CV_LeafKind_DIMCONU:
      case CV_LeafKind_DIMCONLU:
      case CV_LeafKind_DIMVARU:
      case CV_LeafKind_DIMVARLU:
      case CV_LeafKind_BCLASS:
      case CV_LeafKind_VBCLASS:
      case CV_LeafKind_IVBCLASS:
      case CV_LeafKind_FRIENDFCN_ST:
      case CV_LeafKind_INDEX:
      case CV_LeafKind_MEMBER_ST:
      case CV_LeafKind_STMEMBER_ST:
      case CV_LeafKind_METHOD_ST:
      case CV_LeafKind_NESTTYPE_ST:
      case CV_LeafKind_VFUNCTAB:
      case CV_LeafKind_FRIENDCLS:
      case CV_LeafKind_ONEMETHOD_ST:
      case CV_LeafKind_VFUNCOFF:
      case CV_LeafKind_NESTTYPEEX_ST:
      case CV_LeafKind_MEMBERMODIFY_ST:
      case CV_LeafKind_MANAGED_ST:
      case CV_LeafKind_ST_MAX:
      case CV_LeafKind_TYPESERVER:
      case CV_LeafKind_ENUMERATE:
      //case CV_LeafKind_ARRAY:
      //case CV_LeafKind_CLASS:
      //case CV_LeafKind_STRUCTURE:
      //case CV_LeafKind_UNION:
      //case CV_LeafKind_ENUM:
      case CV_LeafKind_DIMARRAY:
      case CV_LeafKind_PRECOMP:
      case CV_LeafKind_ALIAS:
      case CV_LeafKind_DEFARG:
      case CV_LeafKind_FRIENDFCN:
      case CV_LeafKind_MEMBER:
      case CV_LeafKind_STMEMBER:
      case CV_LeafKind_METHOD:
      case CV_LeafKind_NESTTYPE:
      case CV_LeafKind_ONEMETHOD:
      case CV_LeafKind_NESTTYPEEX:
      case CV_LeafKind_MEMBERMODIFY:
      case CV_LeafKind_MANAGED:
      case CV_LeafKind_TYPESERVER2:
      case CV_LeafKind_STRIDED_ARRAY:
      case CV_LeafKind_HLSL:
      case CV_LeafKind_MODIFIER_EX:
      case CV_LeafKind_INTERFACE:
      case CV_LeafKind_BINTERFACE:
      case CV_LeafKind_VECTOR:
      case CV_LeafKind_MATRIX:
      //case CV_LeafKind_VFTABLE:
      default:
      {
        String8 kind_str = cv_string_from_leaf_kind(range->hdr.kind);
        rdim_push_msgf(ctx->root, "pdbconv: unhandled leaf case %.*s (0x%x)",
                       str8_varg(kind_str), range->hdr.kind);
      }break;
    }
  }
  
  rdim_type_fill_id(ctx->root, res, result);
  
  return(result);
}

internal RDIM_Type*
p2r_type_resolve_and_check(P2R_Ctx *ctx, CV_TypeId itype)
{
  RDIM_Type *result = p2r_type_resolve_itype(ctx, itype);
  if(rdim_type_is_unhandled_nil(ctx->root, result))
  {
    rdim_push_msgf(ctx->root, "pdbconv: could not resolve itype (itype = %u)", itype);
  }
  return(result);
}

internal void
p2r_type_resolve_arglist(Arena *arena, RDIM_TypeList *out,
                         P2R_Ctx *ctx, CV_TypeId arglist_itype)
{
  ProfBeginFunction();
  
  // get leaf range
  if(ctx->leaf->itype_first <= arglist_itype && arglist_itype < ctx->leaf->itype_opl)
  {
    CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[arglist_itype - ctx->leaf->itype_first];
    
    // check valid arglist
    String8 data = ctx->leaf->data;
    if(range->hdr.kind == CV_LeafKind_ARGLIST &&
       range->off + range->hdr.size <= data.size)
    {
      U8 *first = data.str + range->off + 2;
      U64 cap = range->hdr.size - 2;
      if(sizeof(CV_LeafArgList) <= cap)
      {
        
        // resolve parameters
        CV_LeafArgList *arglist = (CV_LeafArgList*)first;
        CV_TypeId *itypes = (CV_TypeId*)(arglist + 1);
        U32 max_count = (cap - sizeof(*arglist))/sizeof(CV_TypeId);
        U32 clamped_count = ClampTop(arglist->count, max_count);
        for(U32 i = 0; i < clamped_count; i += 1)
        {
          RDIM_Type *param_type = p2r_type_resolve_and_check(ctx, itypes[i]);
          rdim_type_list_push(arena,  out, param_type);
        }
        
      }
    }
  }
  
  ProfEnd();
}

internal RDIM_Type*
p2r_type_from_name(P2R_Ctx *ctx, String8 name)
{
  // TODO(rjf): no idea if this is correct
  CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, name, 0);
  RDIM_Type *result = rdim_type_from_id(ctx->root, cv_type_id, cv_type_id);
  return(result);
}

internal void
p2r_type_fwd_map_set(Arena *arena, P2R_FwdMap *map, CV_TypeId key, CV_TypeId val)
{
  U64 bucket_idx = key%map->buckets_count;
  
  // search for an existing match
  P2R_FwdNode *match = 0;
  for(P2R_FwdNode *node = map->buckets[bucket_idx];
      node != 0;
      node = node->next)
  {
    if(node->key == key)
    {
      match = node;
      break;
    }
  }
  
  // create a new node if no match
  if(match == 0)
  {
    match = push_array(arena, P2R_FwdNode, 1);
    SLLStackPush(map->buckets[bucket_idx], match);
    match->key = key;
    map->pair_count += 1;
    map->bucket_collision_count += (match->next != 0);
  }
  
  // set node's val
  match->val = val;
}

internal CV_TypeId
p2r_type_fwd_map_get(P2R_FwdMap *map, CV_TypeId key)
{
  U64 bucket_idx = key%map->buckets_count;
  
  // search for an existing match
  P2R_FwdNode *match = 0;
  for(P2R_FwdNode *node = map->buckets[bucket_idx];
      node != 0;
      node = node->next)
  {
    if(node->key == key)
    {
      match = node;
      break;
    }
  }
  
  // extract result
  CV_TypeId result = 0;
  if(match != 0)
  {
    result = match->val;
  }
  
  return(result);
}

//- symbols

internal U64
p2r_hash_from_local_user_id(U64 sym_hash, U64 id)
{
  U64 hash = id ^ (sym_hash<<1) ^ (sym_hash<<4);
  return hash;
}

internal U64
p2r_hash_from_scope_user_id(U64 sym_hash, U64 id)
{
  U64 hash = id ^ (sym_hash<<1) ^ (sym_hash<<4);
  return hash;
}

internal U64
p2r_hash_from_symbol_user_id(U64 sym_hash, U64 id)
{
  U64 hash = id/8 + id ^ (sym_hash<<1) ^ (sym_hash<<4);
  return hash;
}

internal void
p2r_symbol_cons(P2R_Ctx *ctx, CV_SymParsed *sym, U32 sym_unique_id)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  String8 data = sym->data;
  U64 user_id_base = (((U64)sym_unique_id) << 32);
  U64 sym_unique_id_hash = rdi_hash((U8*)&sym_unique_id, sizeof(sym_unique_id));
  
  //////////////////////////////
  //- rjf: PASS 1: map out data associations
  //
  ProfScope("map out data associations")
  {
    RDIM_Symbol *current_proc = 0;
    CV_RecRange *rec_ranges_first = sym->sym_ranges.ranges;
    CV_RecRange *rec_ranges_opl   = rec_ranges_first + sym->sym_ranges.count;
    for(CV_RecRange *rec_range = rec_ranges_first;
        rec_range < rec_ranges_opl;
        rec_range += 1)
    {
      //- rjf: rec range -> symbol info range
      U64 sym_off_first = rec_range->off + 2;
      U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
      
      //- rjf: skip invalid ranges
      if(sym_off_opl > data.size || sym_off_first > data.size || sym_off_first > sym_off_opl)
      {
        continue;
      }
      
      //- rjf: unpack symbol info
      CV_SymKind kind = rec_range->hdr.kind;
      U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
      void *sym_header_struct_base = data.str + sym_off_first;
      
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
          if(current_proc == 0) { break; }
          CV_SymFrameproc *frameproc = (CV_SymFrameproc*)sym_header_struct_base;
          P2R_FrameProcData data = {0};
          data.frame_size = frameproc->frame_size;
          data.flags = frameproc->flags;
          p2r_symbol_frame_proc_write(ctx, current_proc, &data);
        }break;
        
        //- rjf: LPROC32/GPROC32
        case CV_SymKind_LPROC32:
        case CV_SymKind_GPROC32:
        {
          U64 symbol_id = user_id_base + sym_off_first;
          U64 symbol_hash = p2r_hash_from_symbol_user_id(sym_unique_id_hash, symbol_id);
          current_proc = rdim_symbol_handle_from_user_id(ctx->root, symbol_id, symbol_hash);
        }break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: PASS 2: main symbol construction pass
  //
  ProfScope("main symbol construction pass")
  {
    RDIM_LocationSet *defrange_target = 0;
    B32 defrange_target_is_param = 0;
    U64 local_num = 1;
    U64 scope_num = 1;
    CV_RecRange *rec_ranges_first = sym->sym_ranges.ranges;
    CV_RecRange *rec_ranges_opl   = rec_ranges_first + sym->sym_ranges.count;
    for(CV_RecRange *rec_range = rec_ranges_first;
        rec_range < rec_ranges_opl;
        rec_range += 1)
    {
      //- rjf: rec range -> symbol info range
      U64 sym_off_first = rec_range->off + 2;
      U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
      
      //- rjf: skip invalid ranges
      if(sym_off_opl > data.size || sym_off_first > data.size || sym_off_first > sym_off_opl)
      {
        continue;
      }
      
      //- rjf: unpack symbol info
      CV_SymKind kind = rec_range->hdr.kind;
      U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
      void *sym_header_struct_base = data.str + sym_off_first;
      void *sym_data_opl = data.str + sym_off_opl;
      
      //- rjf: skip bad sizes
      if(sym_off_first + sym_header_struct_size > sym_off_opl)
      {
        continue;
      }
      
      //- rjf: unpack current state
      RDIM_Scope *current_scope = p2r_symbol_current_scope(ctx);
      RDIM_Symbol *current_procedure = 0;
      if(current_scope != 0)
      {
        current_procedure = current_scope->symbol;
      }
      
      //- rjf: consume symbol based on kind
      switch(kind)
      {
        default:{}break;
        
        //- rjf: END
        case CV_SymKind_END:
        {
          p2r_symbol_pop_scope(ctx);
          defrange_target = 0;
          defrange_target_is_param = 0;
        }break;
        
        //- rjf: BLOCK32
        case CV_SymKind_BLOCK32:
        {
          CV_SymBlock32 *block32 = (CV_SymBlock32*)sym_header_struct_base;
          
          // scope
          U64 scope_id = user_id_base + scope_num;
          scope_num += 1;
          U64 scope_hash = p2r_hash_from_scope_user_id(sym_unique_id_hash, scope_id);
          RDIM_Scope *block_scope = rdim_scope_handle_from_user_id(ctx->root, scope_id, scope_hash);
          rdim_scope_set_parent(ctx->root, block_scope, current_scope);
          p2r_symbol_push_scope(ctx, block_scope, current_procedure);
          
          // set voff range
          COFF_SectionHeader *section = p2r_sec_header_from_sec_num(ctx, block32->sec);
          if(section != 0)
          {
            U64 voff_first = section->voff + block32->off;
            U64 voff_last = voff_first + block32->len;
            rdim_scope_add_voff_range(ctx->root, block_scope, voff_first, voff_last);
          }
        }break;
        
        //- rjf: LDATA32/GDATA32
        case CV_SymKind_LDATA32:
        case CV_SymKind_GDATA32:
        {
          CV_SymData32 *data32 = (CV_SymData32*)sym_header_struct_base;
          String8 name = str8_cstring_capped(data32+1, sym_data_opl);
          
          // determine voff
          COFF_SectionHeader *section = p2r_sec_header_from_sec_num(ctx, data32->sec);
          U64 voff = ((section != 0)?section->voff:0) + data32->off;
          
          // deduplicate global variable symbols with the same name & offset
          // * PDB likes to have duplicates of these spread across
          // * different symbol streams so we deduplicate across the
          // * entire translation context.
          if(!p2r_known_global_lookup(&ctx->known_globals, name, voff))
          {
            p2r_known_global_insert(ctx->arena, &ctx->known_globals, name, voff);
            
            // type of variable
            RDIM_Type *type = p2r_type_resolve_itype(ctx, data32->itype);
            
            // container type
            RDIM_Type *container_type = 0;
            U64 container_name_opl = p2r_end_of_cplusplus_container_name(name);
            if(container_name_opl > 2)
            {
              String8 container_name = str8(name.str, container_name_opl - 2);
              container_type = p2r_type_from_name(ctx, container_name);
            }
            
            // container symbol
            RDIM_Symbol *container_symbol = 0;
            if(container_type == 0)
            {
              container_symbol = current_procedure;
            }
            
            // determine link kind
            B32 is_extern = (kind == CV_SymKind_GDATA32);
            
            // cons this symbol
            U64 symbol_id = user_id_base + sym_off_first;
            U64 symbol_hash = p2r_hash_from_symbol_user_id(sym_unique_id_hash, symbol_id);
            RDIM_Symbol *symbol = rdim_symbol_handle_from_user_id(ctx->root, symbol_id, symbol_hash);
            
            RDIM_SymbolInfo info = zero_struct;
            info.kind = RDIM_SymbolKind_GlobalVariable;
            info.name = name;
            info.type = type;
            info.is_extern = is_extern;
            info.offset = voff;
            info.container_type = container_type;
            info.container_symbol = container_symbol;
            
            rdim_symbol_set_info(ctx->root, symbol, &info);
          }
        }break;
        
        //- rjf: LPROC32/GPROC32
        case CV_SymKind_LPROC32:
        case CV_SymKind_GPROC32:
        {
          CV_SymProc32 *proc32 = (CV_SymProc32*)sym_header_struct_base;
          String8 name = str8_cstring_capped(proc32+1, sym_data_opl);
          RDIM_Type *type = p2r_type_resolve_itype(ctx, proc32->itype);
          
          // container type
          RDIM_Type *container_type = 0;
          U64 container_name_opl = p2r_end_of_cplusplus_container_name(name);
          if(container_name_opl > 2)
          {
            String8 container_name = str8(name.str, container_name_opl - 2);
            container_type = p2r_type_from_name(ctx, container_name);
          }
          
          // container symbol
          RDIM_Symbol *container_symbol = 0;
          if(container_type == 0)
          {
            container_symbol = current_procedure;
          }
          
          // get this symbol handle
          U64 symbol_id = user_id_base + sym_off_first;
          U64 symbol_hash = p2r_hash_from_symbol_user_id(sym_unique_id_hash, symbol_id);
          RDIM_Symbol *proc_symbol = rdim_symbol_handle_from_user_id(ctx->root, symbol_id, symbol_hash);
          
          // scope
          
          // NOTE: even if there could be a containing scope at this point (which should be
          //       illegal in C/C++ but not necessarily in another language) we would not pass
          //       it here because these scopes refer to the ranges of code that make up a
          //       procedure *not* the namespaces, so a procedure's root scope always has
          //       no parent.
          U64 scope_id = user_id_base + scope_num;
          U64 scope_hash = p2r_hash_from_scope_user_id(sym_unique_id_hash, scope_id);
          RDIM_Scope *root_scope = rdim_scope_handle_from_user_id(ctx->root, scope_id, scope_hash);
          p2r_symbol_push_scope(ctx, root_scope, proc_symbol);
          scope_num += 1;
          
          // set voff range
          U64 voff = 0;
          COFF_SectionHeader *section = p2r_sec_header_from_sec_num(ctx, proc32->sec);
          if(section != 0)
          {
            U64 voff_first = section->voff + proc32->off;
            U64 voff_last = voff_first + proc32->len;
            rdim_scope_add_voff_range(ctx->root, root_scope, voff_first, voff_last);
            
            voff = voff_first;
          }
          
          // link name
          String8 link_name = {0};
          if(voff != 0)
          {
            link_name = p2r_link_name_find(&ctx->link_names, voff);
          }
          
          // determine link kind
          B32 is_extern = (kind == CV_SymKind_GPROC32);
          
          // set symbol info
          RDIM_SymbolInfo info = zero_struct;
          info.kind = RDIM_SymbolKind_Procedure;
          info.name = name;
          info.link_name = link_name;
          info.type = type;
          info.is_extern = is_extern;
          info.container_type = container_type;
          info.container_symbol = container_symbol;
          info.root_scope = root_scope;
          
          rdim_symbol_set_info(ctx->root, proc_symbol, &info);
        }break;
        
        //- rjf: REGREL32
        case CV_SymKind_REGREL32:
        {
          // TODO(allen): hide this when it's redundant with better information
          // from a CV_SymKind_LOCAL record.
          
          CV_SymRegrel32 *regrel32 = (CV_SymRegrel32*)sym_header_struct_base;
          String8 name = str8_cstring_capped(regrel32+1, sym_data_opl);
          RDIM_Type *type = p2r_type_resolve_itype(ctx, regrel32->itype);
          
          // extract regrel's info
          CV_Reg cv_reg = regrel32->reg;
          U32 var_off = regrel32->reg_off;
          
          // need arch for analyzing register stuff
          RDI_Arch arch = ctx->arch;
          U64 addr_size = ctx->addr_size;
          
          // determine if this is a parameter
          RDI_LocalKind local_kind = RDI_LocalKind_Variable;
          {
            B32 is_stack_reg = 0;
            switch (arch)
            {
              case RDI_Arch_X86: is_stack_reg = (cv_reg == CV_Regx86_ESP); break;
              case RDI_Arch_X64: is_stack_reg = (cv_reg == CV_Regx64_RSP); break;
            }
            if(is_stack_reg)
            {
              U32 frame_size = 0xFFFFFFFF;
              if(current_procedure != 0)
              {
                P2R_FrameProcData *frameproc =
                  p2r_symbol_frame_proc_read(ctx, current_procedure);
                frame_size = frameproc->frame_size;
              }
              if(var_off > frame_size)
              {
                local_kind = RDI_LocalKind_Parameter;
              }
            }
          }
          
          // emit local
          U64 local_id = user_id_base + local_num;;
          U64 local_id_hash = p2r_hash_from_local_user_id(sym_unique_id_hash, local_id);
          RDIM_Local *local_var = rdim_local_handle_from_user_id(ctx->root, local_id, local_id_hash);
          local_num += 1;
          
          RDIM_LocalInfo info = {0};
          info.kind = local_kind;
          info.scope = current_scope;
          info.name = name;
          info.type = type;
          rdim_local_set_basic_info(ctx->root, local_var, &info);
          
          // add location to local
          {
            // will there be an extra indirection to the value
            B32 extra_indirection_to_value = 0;
            switch (arch)
            {
              case RDI_Arch_X86:
              {
                if(local_kind == RDI_LocalKind_Parameter &&
                   (type->byte_size > 4 || !IsPow2OrZero(type->byte_size)))
                {
                  extra_indirection_to_value = 1;
                }
              }break;
              
              case RDI_Arch_X64:
              {
                if(local_kind == RDI_LocalKind_Parameter &&
                   (type->byte_size > 8 || !IsPow2OrZero(type->byte_size)))
                {
                  extra_indirection_to_value = 1;
                }
              }break;
            }
            
            // get raddbg register code
            RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(arch, cv_reg);
            // TODO(allen): real byte_size & byte_pos from cv_reg goes here
            U32 byte_size = 8;
            U32 byte_pos = 0;
            
            // set location case
            RDIM_Location *loc =
              p2r_location_from_addr_reg_off(ctx, register_code, byte_size, byte_pos,
                                             (S64)(S32)var_off, extra_indirection_to_value);
            
            RDIM_LocationSet *locset = rdim_location_set_from_local(ctx->root, local_var);
            rdim_location_set_add_case(ctx->root, locset, 0, max_U64, loc);
          }
        }break;
        
        //- rjf: LTHREAD32/GTHREAD32
        case CV_SymKind_LTHREAD32:
        case CV_SymKind_GTHREAD32:
        {
          CV_SymThread32 *thread32 = (CV_SymThread32*)sym_header_struct_base;
          String8 name = str8_cstring_capped(thread32+1, sym_data_opl);
          U32 tls_off = thread32->tls_off;
          RDIM_Type *type = p2r_type_resolve_itype(ctx, thread32->itype);
          
          // container type
          RDIM_Type *container_type = 0;
          U64 container_name_opl = p2r_end_of_cplusplus_container_name(name);
          if(container_name_opl > 2)
          {
            String8 container_name = str8(name.str, container_name_opl - 2);
            container_type = p2r_type_from_name(ctx, container_name);
          }
          
          // container symbol
          RDIM_Symbol *container_symbol = 0;
          if(container_type == 0)
          {
            container_symbol = current_procedure;
          }
          
          // determine link kind
          B32 is_extern = (kind == CV_SymKind_GTHREAD32);
          
          // setup symbol
          U64 symbol_id = user_id_base + sym_off_first;
          U64 symbol_hash = p2r_hash_from_symbol_user_id(sym_unique_id_hash, symbol_id);
          RDIM_Symbol *symbol = rdim_symbol_handle_from_user_id(ctx->root, symbol_id, symbol_hash);
          
          RDIM_SymbolInfo info = zero_struct;
          info.kind = RDIM_SymbolKind_ThreadVariable;
          info.name = name;
          info.type = type;
          info.is_extern = is_extern;
          info.offset = tls_off;
          info.container_type = container_type;
          info.container_symbol = container_symbol;
          
          rdim_symbol_set_info(ctx->root, symbol, &info);
        }break;
        
        //- rjf: LOCAL
        case CV_SymKind_LOCAL:
        {
          CV_SymLocal *slocal = (CV_SymLocal*)sym_header_struct_base;
          String8 name = str8_cstring_capped(slocal+1, sym_data_opl);
          RDIM_Type *type = p2r_type_resolve_itype(ctx, slocal->itype);
          
          // determine how to handle
          B32 begin_a_global_modification = 0;
          if((slocal->flags & CV_LocalFlag_Global) ||
             (slocal->flags & CV_LocalFlag_Static))
          {
            begin_a_global_modification = 1;
          }
          
          // emit a global modification
          if(begin_a_global_modification)
          {
            // TODO(allen): add global modification symbols
            defrange_target = 0;
            defrange_target_is_param = 0;
          }
          
          // emit a local variable
          else
          {
            // local kind
            RDI_LocalKind local_kind = RDI_LocalKind_Variable;
            if(slocal->flags & CV_LocalFlag_Param)
            {
              local_kind = RDI_LocalKind_Parameter;
            }
            
            // emit local
            U64 local_id = user_id_base + local_num;
            U64 local_id_hash = p2r_hash_from_local_user_id(sym_unique_id_hash, local_id);
            RDIM_Local *local_var = rdim_local_handle_from_user_id(ctx->root, local_id, local_id_hash);
            local_num += 1;
            local_var->kind = local_kind;
            local_var->name = name;
            local_var->type = type;
            
            RDIM_LocalInfo info = {0};
            info.kind = local_kind;
            info.scope = current_scope;
            info.name = name;
            info.type = type;
            rdim_local_set_basic_info(ctx->root, local_var, &info);
            
            defrange_target = rdim_location_set_from_local(ctx->root, local_var);
            defrange_target_is_param = (local_kind == RDI_LocalKind_Parameter);
          }
        }break;
        
        //- rjf: DEFRANGE_REGISTESR
        case CV_SymKind_DEFRANGE_REGISTER:
        {
          if(defrange_target == 0) { break; }
          CV_SymDefrangeRegister *defrange_register = (CV_SymDefrangeRegister*)sym_header_struct_base;
          
          // TODO(allen): offset & size from cv_reg code
          RDI_Arch arch = ctx->arch;
          CV_Reg cv_reg = defrange_register->reg;
          RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(arch, cv_reg);
          
          // setup location
          RDIM_Location *location = rdim_location_val_reg(ctx->root, register_code);
          
          // extract range info
          CV_LvarAddrRange *range = &defrange_register->range;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register+1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          
          // emit locations
          p2r_location_over_lvar_addr_range(ctx, defrange_target, location,
                                            range, gaps, gap_count);
        }break;
        
        //- rjf: DEFRANGE_FRAMEPOINTER_REL
        case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL:
        {
          if(defrange_target == 0) { break; }
          CV_SymDefrangeFramepointerRel *defrange_fprel = (CV_SymDefrangeFramepointerRel*)sym_header_struct_base;
          
          // select frame pointer register
          CV_EncodedFramePtrReg encoded_fp_reg =
            p2r_cv_encoded_fp_reg_from_proc(ctx, current_procedure, defrange_target_is_param);
          RDI_RegisterCode fp_register_code =
            p2r_reg_code_from_arch_encoded_fp_reg(ctx->arch, encoded_fp_reg);
          
          // setup location
          B32 extra_indirection = 0;
          U32 byte_size = ctx->addr_size;
          U32 byte_pos = 0;
          S64 var_off = (S64)defrange_fprel->off;
          RDIM_Location *location =
            p2r_location_from_addr_reg_off(ctx, fp_register_code, byte_size, byte_pos,
                                           var_off, extra_indirection);
          
          // extract range info
          CV_LvarAddrRange *range = &defrange_fprel->range;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_fprel + 1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          
          // emit locations
          p2r_location_over_lvar_addr_range(ctx, defrange_target, location,
                                            range, gaps, gap_count);
        }break;
        
        //- rjf: DEFRANGE_SUBFIELD_REGISTER
        case CV_SymKind_DEFRANGE_SUBFIELD_REGISTER:
        {
          if(defrange_target == 0) { break; }
          CV_SymDefrangeSubfieldRegister *defrange_subfield_register = (CV_SymDefrangeSubfieldRegister*)sym_header_struct_base;
          
          // TODO(allen): full "subfield" location system
          if(defrange_subfield_register->field_offset == 0)
          {
            
            // TODO(allen): offset & size from cv_reg code
            RDI_Arch arch = ctx->arch;
            CV_Reg cv_reg = defrange_subfield_register->reg;
            RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(arch, cv_reg);
            
            // setup location
            RDIM_Location *location = rdim_location_val_reg(ctx->root, register_code);
            
            // extract range info
            CV_LvarAddrRange *range = &defrange_subfield_register->range;
            CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_subfield_register + 1);
            U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
            
            // emit locations
            p2r_location_over_lvar_addr_range(ctx, defrange_target, location,
                                              range, gaps, gap_count);
          }
        }break;
        
        //- rjf: DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE
        case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
        {
          if(defrange_target == 0) { break; }
          CV_SymDefrangeFramepointerRelFullScope *defrange_fprel_full_scope =
          (CV_SymDefrangeFramepointerRelFullScope*)sym_header_struct_base;
          
          // select frame pointer register
          CV_EncodedFramePtrReg encoded_fp_reg =
            p2r_cv_encoded_fp_reg_from_proc(ctx, current_procedure, defrange_target_is_param);
          RDI_RegisterCode fp_register_code =
            p2r_reg_code_from_arch_encoded_fp_reg(ctx->arch, encoded_fp_reg);
          
          // setup location
          B32 extra_indirection = 0;
          U32 byte_size = ctx->addr_size;
          U32 byte_pos = 0;
          S64 var_off = (S64)defrange_fprel_full_scope->off;
          RDIM_Location *location =
            p2r_location_from_addr_reg_off(ctx, fp_register_code, byte_size, byte_pos,
                                           var_off, extra_indirection);
          
          
          // emit location
          rdim_location_set_add_case(ctx->root, defrange_target, 0, max_U64, location);
        }break;
        
        //- rjf: DEFRANGE_REGISTER_REL
        case CV_SymKind_DEFRANGE_REGISTER_REL:
        {
          if(defrange_target == 0) { break; }
          CV_SymDefrangeRegisterRel *defrange_register_rel = (CV_SymDefrangeRegisterRel*)sym_header_struct_base;
          
          // TODO(allen): offset & size from cv_reg code
          RDI_Arch arch = ctx->arch;
          CV_Reg cv_reg = defrange_register_rel->reg;
          RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(arch, cv_reg);
          U32 byte_size = ctx->addr_size;
          U32 byte_pos = 0;
          
          B32 extra_indirection_to_value = 0;
          S64 var_off = defrange_register_rel->reg_off;
          
          // setup location
          RDIM_Location *location =
            p2r_location_from_addr_reg_off(ctx, register_code, byte_size, byte_pos,
                                           var_off, extra_indirection_to_value);
          
          // extract range info
          CV_LvarAddrRange *range = &defrange_register_rel->range;
          CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register_rel + 1);
          U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
          
          // emit locations
          p2r_location_over_lvar_addr_range(ctx, defrange_target, location,
                                            range, gaps, gap_count);
        }break;
        
        //- rjf: FILESTATIC
        case CV_SymKind_FILESTATIC:
        {
          CV_SymFileStatic *file_static = (CV_SymFileStatic*)sym_header_struct_base;
          String8 name = str8_cstring_capped(file_static+1, sym_data_opl);
          RDIM_Type *type = p2r_type_resolve_itype(ctx, file_static->itype);
          
          // TODO(allen): emit a global modifier symbol
          
          // defrange records from this point attach to this location information
          defrange_target = 0;
          defrange_target_is_param = 0;
        }break;
      }
    }
    
    //- rjf: non-empty scope stack? -> error
    {
      RDIM_Scope *scope = p2r_symbol_current_scope(ctx);
      if(scope != 0)
      {
        // TODO(allen): emit error
      }
    }
    
    //- rjf: clear scope stack
    p2r_symbol_clear_scope_stack(ctx);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
p2r_gather_link_names(P2R_Ctx *ctx, CV_SymParsed *sym)
{
  ProfBeginFunction();
  // extract important values from parameters
  String8 data = sym->data;
  
  // loop
  CV_RecRange *rec_range = sym->sym_ranges.ranges;
  CV_RecRange *opl = rec_range + sym->sym_ranges.count;
  for(;rec_range < opl; rec_range += 1)
  {
    // symbol data range
    U64 opl_off_raw = rec_range->off + rec_range->hdr.size;
    U64 opl_off = ClampTop(opl_off_raw, data.size);
    
    U64 off_raw = rec_range->off + 2;
    U64 off = ClampTop(off_raw, opl_off);
    
    U8 *first = data.str + off;
    U64 cap = (opl_off - off);
    
    CV_SymKind kind = rec_range->hdr.kind; 
    switch (kind)
    {
      default: break;
      
      case CV_SymKind_PUB32:
      {
        if(sizeof(CV_SymPub32) > cap)
        {
          // TODO(allen): error
        }
        else
        {
          CV_SymPub32 *pub32 = (CV_SymPub32*)first;
          
          // name
          String8 name = str8_cstring_capped((char*)(pub32 + 1), first + cap);
          
          // calculate voff
          U64 voff = 0;
          COFF_SectionHeader *section = p2r_sec_header_from_sec_num(ctx, pub32->sec);
          if(section != 0)
          {
            voff = section->voff + pub32->off;
          }
          
          // save link name
          p2r_link_name_save(ctx->arena, &ctx->link_names, voff, name);
        }
      }break;
    }
  }
  ProfEnd();
}

// "frameproc" map

internal void
p2r_symbol_frame_proc_write(P2R_Ctx *ctx,RDIM_Symbol *key,P2R_FrameProcData *data)
{
  ProfBeginFunction();
  U64 key_int = IntFromPtr(key);
  P2R_FrameProcMap *map = &ctx->frame_proc_map;
  U32 bucket_idx = key_int%map->buckets_count;
  
  // find match
  P2R_FrameProcNode *match = 0;
  for(P2R_FrameProcNode *node = map->buckets[bucket_idx];
      node != 0;
      node = node->next)
  {
    if(node->key == key)
    {
      match = node;
      break;
    }
  }
  
  // if there is already a match emit error
  if(match != 0)
  {
    // TODO(allen): error
  }
  
  // insert new association if no match
  if(match == 0)
  {
    match = push_array(ctx->arena, P2R_FrameProcNode, 1);
    SLLStackPush(map->buckets[bucket_idx], match);
    match->key = key;
    MemoryCopyStruct(&match->data, data);
    map->pair_count += 1;
    map->bucket_collision_count += (match->next != 0);
  }
  ProfEnd();
}

internal P2R_FrameProcData*
p2r_symbol_frame_proc_read(P2R_Ctx *ctx, RDIM_Symbol *key)
{
  U64 key_int = IntFromPtr(key);
  P2R_FrameProcMap *map = &ctx->frame_proc_map;
  U32 bucket_idx = key_int%map->buckets_count;
  
  // find match
  P2R_FrameProcData *result = 0;
  for(P2R_FrameProcNode *node = map->buckets[bucket_idx];
      node != 0;
      node = node->next)
  {
    if(node->key == key)
    {
      result = &node->data;
      break;
    }
  }
  
  return(result);
}

// scope stack
internal void
p2r_symbol_push_scope(P2R_Ctx *ctx, RDIM_Scope *scope, RDIM_Symbol *symbol)
{
  P2R_ScopeNode *node = ctx->scope_node_free;
  if(node == 0)
  {
    node = push_array(ctx->arena, P2R_ScopeNode, 1);
  }
  else
  {
    SLLStackPop(ctx->scope_node_free);
  }
  SLLStackPush(ctx->scope_stack, node);
  node->scope = scope;
  node->symbol = symbol;
}

internal void
p2r_symbol_pop_scope(P2R_Ctx *ctx)
{
  P2R_ScopeNode *node = ctx->scope_stack;
  if(node != 0)
  {
    SLLStackPop(ctx->scope_stack);
    SLLStackPush(ctx->scope_node_free, node);
  }
}

internal void
p2r_symbol_clear_scope_stack(P2R_Ctx *ctx)
{
  for(;;)
  {
    P2R_ScopeNode *node = ctx->scope_stack;
    if(node == 0)
    {
      break;
    }
    SLLStackPop(ctx->scope_stack);
    SLLStackPush(ctx->scope_node_free, node);
  }
}

// PDB/C++ name parsing helper

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

// known global set

internal U64
p2r_known_global_hash(String8 name, U64 voff)
{
  U64 result = 5381 ^ voff;
  U8 *ptr = name.str;
  U8 *opl = ptr + name.size;
  for(; ptr < opl; ptr += 1)
  {
    result = ((result << 5) + result) + *ptr;
  }
  return(result);
}

internal B32
p2r_known_global_lookup(P2R_KnownGlobalSet *set, String8 name, U64 voff)
{
  U64 hash = p2r_known_global_hash(name, voff);
  U64 bucket_idx = hash%set->buckets_count;
  
  P2R_KnownGlobalNode *match = 0;
  for(P2R_KnownGlobalNode *node = set->buckets[bucket_idx];
      node != 0;
      node = node->next)
  {
    if(node->hash == hash &&
       node->key_voff == voff &&
       str8_match(node->key_name, name, 0))
    {
      match = node;
      break;
    }
  }
  
  B32 result = (match != 0);
  return(result);
}

internal void
p2r_known_global_insert(Arena *arena, P2R_KnownGlobalSet *set, String8 name, U64 voff)
{
  U64 hash = p2r_known_global_hash(name, voff);
  U64 bucket_idx = hash%set->buckets_count;
  
  P2R_KnownGlobalNode *match = 0;
  for(P2R_KnownGlobalNode *node = set->buckets[bucket_idx];
      node != 0;
      node = node->next)
  {
    if(node->hash == hash &&
       node->key_voff == voff &&
       str8_match(node->key_name, name, 0))
    {
      match = node;
      break;
    }
  }
  
  if(match == 0)
  {
    P2R_KnownGlobalNode *node = push_array(arena, P2R_KnownGlobalNode, 1);
    SLLStackPush(set->buckets[bucket_idx], node);
    node->key_name = push_str8_copy(arena, name);
    node->key_voff = voff;
    node->hash = hash;
    set->global_count += 1;
    set->bucket_collision_count += (node->next != 0);
  }
}

// location info helpers

internal RDIM_Location*
p2r_location_from_addr_reg_off(P2R_Ctx *ctx,
                               RDI_RegisterCode reg_code,
                               U32 reg_byte_size,
                               U32 reg_byte_pos,
                               S64 offset,
                               B32 extra_indirection)
{
  RDIM_Location *result = 0;
  if(0 <= offset && offset <= (S64)max_U16)
  {
    if(extra_indirection)
    {
      result = rdim_location_addr_addr_reg_plus_u16(ctx->root, reg_code, (U16)offset);
    }
    else
    {
      result = rdim_location_addr_reg_plus_u16(ctx->root, reg_code, (U16)offset);
    }
  }
  else
  {
    Arena *arena = ctx->arena;
    
    RDIM_EvalBytecode bytecode = {0};
    U32 regread_param = RDI_EncodeRegReadParam(reg_code, reg_byte_size, reg_byte_pos);
    rdim_bytecode_push_op(arena, &bytecode, RDI_EvalOp_RegRead, regread_param);
    rdim_bytecode_push_sconst(arena, &bytecode, offset);
    rdim_bytecode_push_op(arena, &bytecode, RDI_EvalOp_Add, 0);
    if(extra_indirection)
    {
      rdim_bytecode_push_op(arena, &bytecode, RDI_EvalOp_MemRead, ctx->addr_size);
    }
    
    result = rdim_location_addr_bytecode_stream(ctx->root, &bytecode);
  }
  
  return(result);
}

internal CV_EncodedFramePtrReg
p2r_cv_encoded_fp_reg_from_proc(P2R_Ctx *ctx, RDIM_Symbol *proc, B32 param_base)
{
  CV_EncodedFramePtrReg result = 0;
  if(proc != 0)
  {
    P2R_FrameProcData *frame_proc = p2r_symbol_frame_proc_read(ctx, proc);
    CV_FrameprocFlags flags = frame_proc->flags;
    if(param_base)
    {
      result = CV_FrameprocFlags_ExtractParamBasePointer(flags);
    }
    else
    {
      result = CV_FrameprocFlags_ExtractLocalBasePointer(flags);
    }
  }
  return(result);
}

internal RDI_RegisterCode
p2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg)
{
  RDI_RegisterCode result = 0;
  
  switch (arch)
  {
    case RDI_Arch_X86:
    {
      switch (encoded_reg)
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
      switch (encoded_reg)
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
p2r_location_over_lvar_addr_range(P2R_Ctx *ctx,
                                  RDIM_LocationSet *locset,
                                  RDIM_Location *location,
                                  CV_LvarAddrRange *range,
                                  CV_LvarAddrGap *gaps, U64 gap_count)
{
  // extract range info
  U64 voff_first = 0;
  U64 voff_opl = 0;
  {
    COFF_SectionHeader *section = p2r_sec_header_from_sec_num(ctx, range->sec);
    if(section != 0)
    {
      voff_first = section->voff + range->off;
      voff_opl = voff_first + range->len;
    }
  }
  
  // emit ranges
  CV_LvarAddrGap *gap_ptr = gaps;
  U64 voff_cursor = voff_first;
  for(U64 i = 0; i < gap_count; i += 1, gap_ptr += 1)
  {
    U64 voff_gap_first = voff_first + gap_ptr->off;
    U64 voff_gap_opl   = voff_gap_first + gap_ptr->len;
    if(voff_cursor < voff_gap_first)
    {
      rdim_location_set_add_case(ctx->root, locset, voff_cursor, voff_gap_first, location);
    }
    voff_cursor = voff_gap_opl;
  }
  
  if(voff_cursor < voff_opl)
  {
    rdim_location_set_add_case(ctx->root, locset, voff_cursor, voff_opl, location);
  }
}

// link names

internal void
p2r_link_name_save(Arena *arena, P2R_LinkNameMap *map, U64 voff, String8 name)
{
  U64 hash = (voff >> 3) ^ ((7 & voff) << 6);
  U64 bucket_idx = hash%map->buckets_count;
  
  P2R_LinkNameNode *node = push_array(arena, P2R_LinkNameNode, 1);
  SLLStackPush(map->buckets[bucket_idx], node);
  node->voff = voff;
  node->name = push_str8_copy(arena, name);
  map->link_name_count += 1;
  map->bucket_collision_count += (node->next != 0);
}

internal String8
p2r_link_name_find(P2R_LinkNameMap *map, U64 voff)
{
  U64 hash = (voff >> 3) ^ ((7 & voff) << 6);
  U64 bucket_idx = hash%map->buckets_count;
  
  String8 result = {0};
  for(P2R_LinkNameNode *node = map->buckets[bucket_idx];
      node != 0;
      node = node->next)
  {
    if(node->voff == voff)
    {
      result = node->name;
      break;
    }
  }
  
  return(result);
}

#endif

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
  //- rjf: parse tpi hash
  //
  PDB_TpiHashParsed *tpi_hash = 0;
  if(tpi != 0) ProfScope("parse tpi hash")
  {
    String8 hash_data = msf_data_from_stream(msf, tpi->hash_sn);
    String8 aux_data = msf_data_from_stream(msf, tpi->hash_sn_aux);
    tpi_hash = pdb_tpi_hash_from_data(arena, strtbl, tpi, hash_data, aux_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse tpi leaves
  //
  CV_LeafParsed *tpi_leaf = 0;
  if(tpi != 0) ProfScope("parse tpi leaves")
  {
    String8 leaf_data = pdb_leaf_data_from_tpi(tpi);
    tpi_leaf = cv_leaf_from_data(arena, leaf_data, tpi->itype_first);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse ipi hash
  //
  PDB_TpiHashParsed *ipi_hash = 0;
  if(ipi != 0) ProfScope("parse ipi hash")
  {
    String8 hash_data = msf_data_from_stream(msf, ipi->hash_sn);
    String8 aux_data = msf_data_from_stream(msf, ipi->hash_sn_aux);
    ipi_hash = pdb_tpi_hash_from_data(arena, strtbl, ipi, hash_data, aux_data);
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: parse ipi leaves
  //
  CV_LeafParsed *ipi_leaf = 0;
  if(ipi != 0) ProfScope("parse ipi leaves")
  {
    String8 leaf_data = pdb_leaf_data_from_tpi(ipi);
    ipi_leaf = cv_leaf_from_data(arena, leaf_data, ipi->itype_first);
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
  if(comp_units != 0) ProfScope("parse symbols")
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
  //- rjf: hash exe
  //
  U64 exe_hash = 0;
  if(in->input_exe_data.size > 0) ProfScope("hash exe")
  {
    exe_hash = rdi_hash(in->input_exe_data.str, in->input_exe_data.size);
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
  //- rjf: build unit array
  //
  RDIM_UnitArray units = {0};
  ProfScope("build unit array")
  {
    //- rjf: allocate
    units.count = comp_unit_count;
    units.v = push_array(arena, RDIM_Unit, units.count);
    
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
      
      //- rjf: fill basic output unit info
      RDIM_Unit *dst_unit = &units.v[comp_unit_idx];
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
        RDIM_Unit *unit = &units.v[contrib_ptr->mod];
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
    if(symbol_count_prediction < 128)
    {
      symbol_count_prediction = 128;
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 1: produce type forward resolution map
  //
  U64 type_fwd_map_count = 0;
  CV_TypeId *type_fwd_map = 0;
  CV_TypeId itype_first = 0;
  CV_TypeId itype_opl   = tpi_leaf->itype_opl;
  ProfScope("types pass 1: produce type forward resolution map")
  {
    type_fwd_map_count = (U64)itype_opl;
    type_fwd_map = push_array(arena, CV_TypeId, type_fwd_map_count);
    for(CV_TypeId itype = itype_first; itype < itype_opl; itype += 1)
    {
      //- rjf: determine if this itype resolves to another
      CV_TypeId itype_fwd = 0;
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
          default:{}break;
          
          //- rjf: CLASS/STRUCTURE
          case CV_LeafKind_CLASS:
          case CV_LeafKind_STRUCTURE:
          {
            // rjf: unpack leaf
            CV_LeafStruct *lf_struct = (CV_LeafStruct *)itype_leaf_first;
            U8 *numeric_ptr = (U8 *)(lf_struct + 1);
            CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
            U8 *name_ptr = numeric_ptr + size.encoded_size;
            String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
            U8 *unique_name_ptr = name_ptr + name.size + 1;
            String8 unique_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
            
            // rjf: has fwd ref flag -> lookup itype that this itype resolves to
            if(lf_struct->props & CV_TypeProp_FwdRef)
            {
              B32 do_unique_name_lookup = (((lf_struct->props & CV_TypeProp_Scoped) != 0) &&
                                           ((lf_struct->props & CV_TypeProp_HasUniqueName) != 0));
              itype_fwd = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
            }
          }break;
          
          //- rjf: CLASS2/STRUCT2
          case CV_LeafKind_CLASS2:
          case CV_LeafKind_STRUCT2:
          {
            // rjf: unpack leaf
            CV_LeafStruct2 *lf_struct = (CV_LeafStruct2 *)itype_leaf_first;
            U8 *numeric_ptr = (U8 *)(lf_struct + 1);
            CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
            U8 *name_ptr = (U8 *)numeric_ptr + size.encoded_size;
            String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
            U8 *unique_name_ptr = name_ptr + name.size + 1;
            String8 unique_name = str8_cstring_capped(unique_name_ptr, itype_leaf_opl);
            
            // rjf: has fwd ref flag -> lookup itype that this itype resolves to
            if(lf_struct->props & CV_TypeProp_FwdRef)
            {
              B32 do_unique_name_lookup = (((lf_struct->props & CV_TypeProp_Scoped) != 0) &&
                                           ((lf_struct->props & CV_TypeProp_HasUniqueName) != 0));
              itype_fwd = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
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
              itype_fwd = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
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
              itype_fwd = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, do_unique_name_lookup?unique_name:name, do_unique_name_lookup);
            }
          }break;
        }
      }
      
      //- rjf: if the forwarded itype is nonzero & in TPI range -> save to map
      if(itype_fwd != 0 && itype_first <= itype_fwd && itype_fwd < itype_opl)
      {
        type_fwd_map[itype-itype_first] = itype_fwd;
      }
    }
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 2: construct all root/stub types from TPI
  //
  // this does NOT gather the following information, which is done by
  // subsequent passes, as they all require full resolution of all itypes first
  // (to allow for early itypes to reference later itypes):
  //
  // - function/method type parameters
  // - modifier byte sizes
  // - bitfield direct type byte sizes
  // - struct/class/union members
  // - array counts
  // - direct type forward resolution
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
  RDIM_TypeArray itype_types = {0};     // root type for per-TPI-itype
  RDIM_TypeChunkList extra_types = {0}; // extra supplementary types we build, which do not have any itypes
  ProfScope("types pass 2: construct all root/stub types from TPI")
  {
    RDI_U64 extra_types_chunk_cap = 1024;
    itype_types.count = (U64)(itype_opl-itype_first);
    itype_types.v = push_array(arena, RDIM_Type, itype_types.count);
#define p2r_type_ptr_from_itype(itype) ((itype_first <= (itype) && (itype) < itype_opl) ? (&itype_types.v[(type_fwd_map[(itype)-itype_first] ? type_fwd_map[(itype)-itype_first] : (itype))-itype_first]) : 0)
    for(CV_TypeId itype = itype_first; itype < itype_opl; itype += 1)
    {
      RDIM_Type *dst_type = &itype_types.v[itype-itype_first];
      B32 itype_is_basic = (itype < 0x1000);
      
      //////////////////////////
      //- rjf: build basic type
      //
      if(itype_is_basic)
      {
        // rjf: unpack itype
        CV_BasicPointerKind cv_basic_ptr_kind  = CV_BasicPointerKindFromTypeId(itype);
        CV_BasicType        cv_basic_type_code = CV_BasicTypeFromTypeId(itype);
        
        // rjf: get basic type slot, fill if unfilled
        RDIM_Type *basic_type = &itype_types.v[cv_basic_type_code-itype_first];
        if(basic_type->kind == RDI_TypeKind_NULL)
        {
          RDI_TypeKind type_kind = rdi_type_kind_from_cv_basic_type(cv_basic_type_code);
          U32 byte_size = rdi_size_from_basic_type_kind(type_kind);
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
          dst_type->kind        = RDI_TypeKind_Ptr;
          dst_type->idx         = (RDI_U32)(itype-itype_first);
          dst_type->byte_size   = arch_addr_size;
          dst_type->direct_type = basic_type;
        }
      }
      
      //////////////////////////
      //- rjf: build non-basic type
      //
      if(!itype_is_basic)
      {
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
              CV_LeafModifier *modifier = (CV_LeafModifier *)itype_leaf_first;
              
              // rjf: cv -> rdi flags
              RDI_TypeModifierFlags flags = 0;
              if(modifier->flags & CV_ModifierFlag_Const)    {flags |= RDI_TypeModifierFlag_Const;}
              if(modifier->flags & CV_ModifierFlag_Volatile) {flags |= RDI_TypeModifierFlag_Volatile;}
              
              // rjf: fill type
              dst_type->kind        = RDI_TypeKind_Modifier;
              dst_type->flags       = flags;
              dst_type->direct_type = p2r_type_ptr_from_itype(modifier->itype);
            }break;
            
            //- rjf: POINTER
            case CV_LeafKind_POINTER:
            {
              // TODO(rjf): if ptr_mode in {PtrMem, PtrMethod} then output a member pointer instead
              
              // rjf: unpack leaf
              CV_LeafPointer *pointer = (CV_LeafPointer *)itype_leaf_first;
              RDIM_Type *direct_type = p2r_type_ptr_from_itype(pointer->itype);
              CV_PointerKind ptr_kind = CV_PointerAttribs_ExtractKind(pointer->attribs);
              CV_PointerMode ptr_mode = CV_PointerAttribs_ExtractMode(pointer->attribs);
              U32            ptr_size = CV_PointerAttribs_ExtractSize(pointer->attribs);
              
              // rjf: cv -> rdi modifier flags
              RDI_TypeModifierFlags modifier_flags = 0;
              if(pointer->attribs & CV_PointerAttrib_Const)    {modifier_flags |= RDI_TypeModifierFlag_Const;}
              if(pointer->attribs & CV_PointerAttrib_Volatile) {modifier_flags |= RDI_TypeModifierFlag_Volatile;}
              
              // rjf: cv info -> rdi pointer type kind
              RDI_TypeKind type_kind = RDI_TypeKind_Ptr;
              {
                if(pointer->attribs & CV_PointerAttrib_LRef)
                {
                  type_kind = RDI_TypeKind_LRef;
                }
                else if(pointer->attribs & CV_PointerAttrib_RRef)
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
              if(modifier_flags != 0)
              {
                RDIM_Type *pointer_type = rdim_type_chunk_list_push(arena, &extra_types, extra_types_chunk_cap);
                dst_type->kind             = RDI_TypeKind_Modifier;
                dst_type->flags            = modifier_flags;
                dst_type->direct_type      = pointer_type;
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
              
              // rjf: push revisit task for full byte size of modifier type
              if(modifier_flags != 0)
              {
                P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
                SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
                t->base_type = dst_type;
              }
            }break;
            
            //- rjf: PROCEDURE
            case CV_LeafKind_PROCEDURE:
            {
              // TODO(rjf): handle call_kind & attribs
              
              // rjf: unpack leaf
              CV_LeafProcedure *procedure = (CV_LeafProcedure *)itype_leaf_first;
              RDIM_Type *ret_type = p2r_type_ptr_from_itype(procedure->ret_itype);
              
              // rjf: fill type
              dst_type->kind        = RDI_TypeKind_Function;
              dst_type->byte_size   = arch_addr_size;
              dst_type->count       = procedure->arg_count;
              dst_type->direct_type = ret_type;
              
              // rjf: push revisit task for parameters
              P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
              SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
              t->base_type   = dst_type;
              t->field_itype = procedure->arg_itype;
            }break;
            
            //- rjf: MFUNCTION
            case CV_LeafKind_MFUNCTION:
            {
              // TODO(rjf): handle call_kind & attribs
              // TODO(rjf): preserve "this_adjust"
              
              // rjf: unpack leaf
              CV_LeafMFunction *mfunction = (CV_LeafMFunction *)itype_leaf_first;
              RDIM_Type *ret_type  = p2r_type_ptr_from_itype(mfunction->ret_itype);
              
              // rjf: fill type
              dst_type->kind        = (mfunction->this_itype != 0) ? RDI_TypeKind_Method : RDI_TypeKind_Function;
              dst_type->byte_size   = arch_addr_size;
              dst_type->count       = mfunction->arg_count;
              dst_type->direct_type = ret_type;
              
              // rjf: push revisit task for parameters/this
              P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
              SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
              t->base_type   = dst_type;
              t->field_itype = mfunction->arg_itype;
              t->this_itype  = mfunction->this_itype;
            }break;
            
            //- rjf: BITFIELD
            case CV_LeafKind_BITFIELD:
            {
              // rjf: unpack leaf
              CV_LeafBitField *bit_field = (CV_LeafBitField *)itype_leaf_first;
              RDIM_Type *direct_type = p2r_type_ptr_from_itype(bit_field->itype);
              
              // rjf: fill type
              dst_type->kind        = RDI_TypeKind_Bitfield;
              dst_type->off         = bit_field->pos;
              dst_type->count       = bit_field->len;
              dst_type->direct_type = direct_type;
              
              // rjf: push revisit task for byte size
              P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
              SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
              t->base_type = dst_type;
            }break;
            
            //- rjf: ARRAY
            case CV_LeafKind_ARRAY:
            {
              // rjf: unpack leaf
              CV_LeafArray *array = (CV_LeafArray *)itype_leaf_first;
              RDIM_Type *direct_type = p2r_type_ptr_from_itype(array->entry_itype);
              U8 *numeric_ptr = (U8*)(array + 1);
              CV_NumericParsed array_count = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
              U64 full_size = cv_u64_from_numeric(&array_count);
              
              // rjf: fill type
              dst_type->kind        = RDI_TypeKind_Array;
              dst_type->direct_type = direct_type;
              dst_type->byte_size   = full_size;
              
              // rjf: push revisit task for full count
              P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
              SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
              t->base_type = dst_type;
            }break;
            
            //- rjf: CLASS/STRUCTURE
            case CV_LeafKind_CLASS:
            case CV_LeafKind_STRUCTURE:
            {
              // TODO(rjf): handle props
              
              // rjf: unpack leaf
              CV_LeafStruct *lf_struct = (CV_LeafStruct *)itype_leaf_first;
              U8 *numeric_ptr = (U8*)(lf_struct + 1);
              CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
              U64 size_u64 = cv_u64_from_numeric(&size);
              U8 *name_ptr = numeric_ptr + size.encoded_size;
              String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
              
              // rjf: fill type
              if(lf_struct->props & CV_TypeProp_FwdRef)
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
              if(!(lf_struct->props & CV_TypeProp_FwdRef))
              {
                P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
                SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
                t->base_type   = dst_type;
                t->field_itype = lf_struct->field_itype;
              }
            }break;
            
            //- rjf: CLASS2/STRUCT2
            case CV_LeafKind_CLASS2:
            case CV_LeafKind_STRUCT2:
            {
              // TODO(rjf): handle props
              
              // rjf: unpack leaf
              CV_LeafStruct2 *lf_struct = (CV_LeafStruct2 *)itype_leaf_first;
              U8 *numeric_ptr = (U8*)(lf_struct + 1);
              CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
              U64 size_u64 = cv_u64_from_numeric(&size);
              U8 *name_ptr = numeric_ptr + size.encoded_size;
              String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
              
              // rjf: fill type
              if(lf_struct->props & CV_TypeProp_FwdRef)
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
              if(!(lf_struct->props & CV_TypeProp_FwdRef))
              {
                P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
                SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
                t->base_type   = dst_type;
                t->field_itype = lf_struct->field_itype;
              }
            }break;
            
            //- rjf: UNION
            case CV_LeafKind_UNION:
            {
              // TODO(rjf): handle props
              
              // rjf: unpack leaf
              CV_LeafUnion *lf_union = (CV_LeafUnion *)itype_leaf_first;
              U8 *numeric_ptr = (U8*)(lf_union + 1);
              CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, itype_leaf_opl);
              U64 size_u64 = cv_u64_from_numeric(&size);
              U8 *name_ptr = numeric_ptr + size.encoded_size;
              String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
              
              // rjf: fill type
              if(lf_union->props & CV_TypeProp_FwdRef)
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
              if(!(lf_union->props & CV_TypeProp_FwdRef))
              {
                P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
                SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
                t->base_type   = dst_type;
                t->field_itype = lf_union->field_itype;
              }
            }break;
            
            //- rjf: ENUM
            case CV_LeafKind_ENUM:
            {
              // TODO(rjf): handle props
              
              // rjf: unpack leaf
              CV_LeafEnum *lf_enum = (CV_LeafEnum *)itype_leaf_first;
              RDIM_Type *direct_type = p2r_type_ptr_from_itype(lf_enum->base_itype);
              U8 *name_ptr = (U8 *)(lf_enum + 1);
              String8 name = str8_cstring_capped(name_ptr, itype_leaf_opl);
              
              // rjf: fill type
              if(lf_enum->props & CV_TypeProp_FwdRef)
              {
                dst_type->kind = RDI_TypeKind_IncompleteEnum;
                dst_type->name = name;
              }
              else
              {
                dst_type->kind        = RDI_TypeKind_Enum;
                dst_type->direct_type = direct_type;
                dst_type->name        = name;
              }
              
              // rjf: push revisit task for enumerates/size
              if(!(lf_enum->props & CV_TypeProp_FwdRef))
              {
                P2R_TypeIdRevisitTask *t = push_array(scratch.arena, P2R_TypeIdRevisitTask, 1);
                SLLQueuePush(first_itype_revisit_task, last_itype_revisit_task, t);
                t->base_type   = dst_type;
                t->field_itype = lf_enum->field_itype;
              }
            }break;
          }
        }
      }
    }
#undef p2r_type_ptr_from_itype
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: types pass 3: attach cross-itype-relationship data to all types, build UDTs
  //
  // given the root/stub types for all itypes, this pass takes care of the
  // following extra pieces of per-type information:
  //
  // - function/method type parameters
  // - modifier byte sizes
  // - bitfield direct type byte sizes
  // - struct/class/union members
  // - array counts
  // - direct type forward resolution
  //
  RDIM_UDTChunkList udts = {0};
  ProfScope("types pass 3: attach cross-itype-relationship data to all types, build UDTs")
  {
    RDI_U64 udts_chunk_cap = 1024;
#define p2r_type_ptr_from_itype(itype) ((itype_first <= (itype) && (itype) < itype_opl) ? (&itype_types.v[(type_fwd_map[(itype)-itype_first] ? type_fwd_map[(itype)-itype_first] : (itype))-itype_first]) : 0)
    for(P2R_TypeIdRevisitTask *task = first_itype_revisit_task; task != 0; task = task->next)
    {
      RDIM_Type *dst_type = task->base_type;
      switch(dst_type->kind)
      {
        default:{}break;
        
        ////////////////////////
        //- rjf: bitfields/modifiers -> calculate byte size
        //
        case RDI_TypeKind_Bitfield:
        case RDI_TypeKind_Modifier:
        if(dst_type->direct_type != 0)
        {
          dst_type->byte_size = dst_type->direct_type->byte_size;
        }break;
        
        ////////////////////////
        //- rjf: functions -> equip parameters
        //
        case RDI_TypeKind_Function:
        {
          // rjf: unpack arglist range
          CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[task->field_itype-itype_first];
          if(range->hdr.kind != CV_LeafKind_ARGLIST ||
             range->hdr.size<2 ||
             range->off + range->hdr.size > tpi_leaf->data.size)
          {
            break;
          }
          U8 *arglist_first = tpi_leaf->data.str + range->off + 2;
          U8 *arglist_opl   = arglist_first+range->hdr.size-2;
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
        
        ////////////////////////
        //- rjf: methods -> equip this ptr + parameters
        //
        case RDI_TypeKind_Method:
        {
          // rjf: unpack arglist range
          CV_RecRange *range = &tpi_leaf->leaf_ranges.ranges[task->field_itype-itype_first];
          if(range->hdr.kind != CV_LeafKind_ARGLIST ||
             range->hdr.size<2 ||
             range->off + range->hdr.size > tpi_leaf->data.size)
          {
            break;
          }
          U8 *arglist_first = tpi_leaf->data.str + range->off + 2;
          U8 *arglist_opl   = arglist_first+range->hdr.size-2;
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
          params[0] = p2r_type_ptr_from_itype(task->this_itype);
          
          // rjf: fill dst type
          dst_type->count = arglist_itypes_count+1;
          dst_type->param_types = params;
        }break;
        
        ////////////////////////
        //- rjf: arrays -> calculate array count based on direct type size
        //
        case RDI_TypeKind_Array:
        if(dst_type->direct_type != 0 && dst_type->direct_type->byte_size != 0)
        {
          dst_type->count = dst_type->byte_size/dst_type->direct_type->byte_size;
        }break;
        
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
            dst_udt = dst_type->udt = rdim_udt_chunk_list_push(arena, &udts, udts_chunk_cap);
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
                U8 *field_leaf_opl   = field_leaf_first+field_leaf_header_size;
                next_read_ptr = field_leaf_opl;
                
                // rjf: skip out-of-bounds fields
                if(field_leaf_opl > field_list_opl)
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
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
                          RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
                          mem->kind = RDI_MemberKind_Method;
                          mem->name = name;
                          mem->type = method_type;
                        }break;
                        case CV_MethodProp_Static:
                        {
                          RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
                          mem->kind = RDI_MemberKind_StaticMethod;
                          mem->name = name;
                          mem->type = method_type;
                        }break;
                        case CV_MethodProp_Virtual:
                        case CV_MethodProp_PureVirtual:
                        case CV_MethodProp_Intro:
                        case CV_MethodProp_PureIntro:
                        {
                          RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
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
                        RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
                        mem->kind = RDI_MemberKind_Method;
                        mem->name = name;
                        mem->type = method_type;
                      }break;
                      
                      case CV_MethodProp_Static:
                      {
                        RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
                        mem->kind = RDI_MemberKind_StaticMethod;
                        mem->name = name;
                        mem->type = method_type;
                      }break;
                      
                      case CV_MethodProp_Virtual:
                      case CV_MethodProp_PureVirtual:
                      case CV_MethodProp_Intro:
                      case CV_MethodProp_PureIntro:
                      {
                        RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
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
                    RDIM_UDTMember *mem = rdim_udt_push_member(arena, &udts, dst_udt);
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
            dst_udt = dst_type->udt = rdim_udt_chunk_list_push(arena, &udts, udts_chunk_cap);
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
                U8 *field_leaf_opl   = field_leaf_first+field_leaf_header_size;
                next_read_ptr = field_leaf_opl;
                
                // rjf: skip out-of-bounds fields
                if(field_leaf_opl > field_list_opl)
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
                    RDIM_UDTEnumVal *enum_val = rdim_udt_push_enum_val(arena, &udts, dst_udt);
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
#undef p2r_type_ptr_from_itype
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
  //- rjf: produce symbols from all sym streams
  //
  RDIM_SymbolChunkList all_procedures = {0};
  RDIM_SymbolChunkList all_global_variables = {0};
  RDIM_SymbolChunkList all_thread_variables = {0};
  RDIM_ScopeChunkList all_scopes = {0};
  ProfScope("produce symbols from all sym streams")
  {
#define p2r_type_ptr_from_itype(itype) ((itype_first <= (itype) && (itype) < itype_opl) ? (&itype_types.v[(type_fwd_map[(itype)-itype_first] ? type_fwd_map[(itype)-itype_first] : (itype))-itype_first]) : 0)
    
    ////////////////////////////
    //- rjf: produce array of all symbol streams
    //
    U64 syms_count = 1 + comp_unit_count;
    CV_SymParsed **syms = push_array(scratch.arena, CV_SymParsed *, syms_count);
    {
      syms[0] = sym;
      for(U64 comp_unit_idx = 0; comp_unit_idx < comp_unit_count; comp_unit_idx += 1)
      {
        syms[comp_unit_idx+1] = sym_for_unit[comp_unit_idx];
      }
    }
    
    ////////////////////////////
    //- rjf: produce symbols
    //
    for(U64 sym_idx = 0; sym_idx < syms_count; sym_idx += 1)
    {
      CV_SymParsed *sym = syms[sym_idx];
      Temp scratch = scratch_begin(&arena, 1);
      
      //////////////////////////
      //- rjf: set up outputs for this sym stream
      //
      RDIM_SymbolChunkList sym_procedures = {0};
      RDIM_SymbolChunkList sym_global_variables = {0};
      RDIM_SymbolChunkList sym_thread_variables = {0};
      RDIM_ScopeChunkList sym_scopes = {0};
      U64 sym_procedures_chunk_cap = 1024;
      U64 sym_global_variables_chunk_cap = 1024;
      U64 sym_thread_variables_chunk_cap = 1024;
      U64 sym_scopes_chunk_cap = 1024;
      
      //////////////////////////
      //- rjf: symbols pass 1: produce procedure frame info map (procedure -> frame info)
      //
      U64 procedure_frameprocs_count = 0;
      U64 procedure_frameprocs_cap   = sym->sym_ranges.count;
      CV_SymFrameproc **procedure_frameprocs = push_array_no_zero(scratch.arena, CV_SymFrameproc *, procedure_frameprocs_cap);
      ProfScope("symbols pass 1: produce procedure frame info map (procedure -> frame info)")
      {
        U64 procedure_num = 0;
        CV_RecRange *rec_ranges_first = sym->sym_ranges.ranges;
        CV_RecRange *rec_ranges_opl   = rec_ranges_first + sym->sym_ranges.count;
        for(CV_RecRange *rec_range = rec_ranges_first;
            rec_range < rec_ranges_opl;
            rec_range += 1)
        {
          //- rjf: rec range -> symbol info range
          U64 sym_off_first = rec_range->off + 2;
          U64 sym_off_opl   = rec_range->off + rec_range->hdr.size;
          
          //- rjf: skip invalid ranges
          if(sym_off_opl > sym->data.size || sym_off_first > sym->data.size || sym_off_first > sym_off_opl)
          {
            continue;
          }
          
          //- rjf: unpack symbol info
          CV_SymKind kind = rec_range->hdr.kind;
          U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
          void *sym_header_struct_base = sym->data.str + sym_off_first;
          
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
        CV_RecRange *rec_ranges_first = sym->sym_ranges.ranges;
        CV_RecRange *rec_ranges_opl   = rec_ranges_first + sym->sym_ranges.count;
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
          if(sym_off_opl > sym->data.size || sym_off_first > sym->data.size || sym_off_first > sym_off_opl)
          {
            continue;
          }
          
          //- rjf: unpack symbol info
          CV_SymKind kind = rec_range->hdr.kind;
          U64 sym_header_struct_size = cv_header_struct_size_from_sym_kind(kind);
          void *sym_header_struct_base = sym->data.str + sym_off_first;
          void *sym_data_opl = sym->data.str + sym_off_opl;
          
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
                COFF_SectionHeader *section = (0 < block32->sec && block32->sec <= coff_sections->count) ? &coff_sections->sections[block32->sec-1] : 0;
                if(section != 0)
                {
                  U64 voff_first = section->voff + block32->off;
                  U64 voff_last = voff_first + block32->len;
                  RDIM_Rng1U64 voff_range = {voff_first, voff_last};
                  rdim_rng1u64_list_push(arena, &scope->voff_ranges, voff_range);
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
              COFF_SectionHeader *section = (0 < data32->sec && data32->sec <= coff_sections->count) ? &coff_sections->sections[data32->sec-1] : 0;
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
                  CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, name, 0);
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
                symbol->kind             = RDIM_SymbolKind_GlobalVariable;
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
                CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, name, 0);
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
                COFF_SectionHeader *section = (0 < proc32->sec && proc32->sec <= coff_sections->count) ? &coff_sections->sections[proc32->sec-1] : 0;
                if(section != 0)
                {
                  U64 voff_first = section->voff + proc32->off;
                  U64 voff_last = voff_first + proc32->len;
                  RDIM_Rng1U64 voff_range = {voff_first, voff_last};
                  rdim_rng1u64_list_push(arena, &procedure_root_scope->voff_ranges, voff_range);
                }
              }
              
              // rjf: root scope voff minimum range -> link name
              String8 link_name = {0};
              if(procedure_root_scope->voff_ranges.min != 0)
              {
                U64 voff = procedure_root_scope->voff_ranges.min;
                U64 hash = p2r_hash_from_voff(voff);
                U64 bucket_idx = hash%link_name_map.buckets_count;
                P2R_LinkNameNode *node = 0;
                for(P2R_LinkNameNode *n = link_name_map.buckets[bucket_idx]; n != 0; n = n->next)
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
              procedure_symbol->kind             = RDIM_SymbolKind_Procedure;
              procedure_symbol->is_extern        = (kind == CV_SymKind_GPROC32);
              procedure_symbol->name             = name;
              procedure_symbol->link_name        = link_name;
              procedure_symbol->type             = type;
              procedure_symbol->container_symbol = container_symbol;
              procedure_symbol->container_type   = container_type;
              
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
                switch(arch)
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
                switch(arch)
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
                RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(arch, cv_reg);
                // TODO(rjf): real byte_size & byte_pos from cv_reg goes here
                U32 byte_size = 8;
                U32 byte_pos = 0;
                
                // rjf: set location case
                RDIM_Location *loc = p2r_location_from_addr_reg_off(arena, arch, register_code, byte_size, byte_pos, (S64)(S32)var_off, extra_indirection_to_value);
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
                CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(tpi_hash, tpi_leaf, name, 0);
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
              tvar->kind             = RDIM_SymbolKind_ThreadVariable;
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
              COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= coff_sections->count) ? &coff_sections->sections[range->sec-1] : 0;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register+1);
              U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
              RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(arch, cv_reg);
              
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
              COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= coff_sections->count) ? &coff_sections->sections[range->sec-1] : 0;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_fprel + 1);
              U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
              
              // rjf: select frame pointer register
              CV_EncodedFramePtrReg encoded_fp_reg = p2r_cv_encoded_fp_reg_from_frameproc(frameproc, defrange_target_is_param);
              RDI_RegisterCode fp_register_code = p2r_reg_code_from_arch_encoded_fp_reg(arch, encoded_fp_reg);
              
              // rjf: build location
              B32 extra_indirection = 0;
              U32 byte_size = arch_addr_size;
              U32 byte_pos = 0;
              S64 var_off = (S64)defrange_fprel->off;
              RDIM_Location *location = p2r_location_from_addr_reg_off(arena, arch, fp_register_code, byte_size, byte_pos, var_off, extra_indirection);
              
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
              COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= coff_sections->count) ? &coff_sections->sections[range->sec-1] : 0;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_subfield_register + 1);
              U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
              RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(arch, cv_reg);
              
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
              RDI_RegisterCode fp_register_code = p2r_reg_code_from_arch_encoded_fp_reg(arch, encoded_fp_reg);
              
              // rjf: build location
              B32 extra_indirection = 0;
              U32 byte_size = arch_addr_size;
              U32 byte_pos = 0;
              S64 var_off = (S64)defrange_fprel_full_scope->off;
              RDIM_Location *location = p2r_location_from_addr_reg_off(arena, arch, fp_register_code, byte_size, byte_pos, var_off, extra_indirection);
              
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
              RDI_RegisterCode register_code = rdi_reg_code_from_cv_reg_code(arch, cv_reg);
              CV_LvarAddrRange *range = &defrange_register_rel->range;
              COFF_SectionHeader *range_section = (0 < range->sec && range->sec <= coff_sections->count) ? &coff_sections->sections[range->sec-1] : 0;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register_rel + 1);
              U64 gap_count = ((U8*)sym_data_opl - (U8*)gaps) / sizeof(*gaps);
              
              // rjf: build location
              // TODO(rjf): offset & size from cv_reg code
              U32 byte_size = arch_addr_size;
              U32 byte_pos = 0;
              B32 extra_indirection_to_value = 0;
              S64 var_off = defrange_register_rel->reg_off;
              RDIM_Location *location = p2r_location_from_addr_reg_off(arena, arch, register_code, byte_size, byte_pos, var_off, extra_indirection_to_value);
              
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
      //- rjf: merge this stream's outputs with collated list
      //
      rdim_symbol_chunk_list_concat_in_place(&all_procedures, &sym_procedures);
      rdim_symbol_chunk_list_concat_in_place(&all_global_variables, &sym_global_variables);
      rdim_symbol_chunk_list_concat_in_place(&all_thread_variables, &sym_thread_variables);
      rdim_scope_chunk_list_concat_in_place(&all_scopes, &sym_scopes);
      
      scratch_end(scratch);
    }
    
#undef p2r_type_ptr_from_itype
  }
  
  //////////////////////////////////////////////////////////////
  //- rjf: fill output
  //
  P2R_ConvertOut *out = push_array(arena, P2R_ConvertOut, 1);
  {
    out->top_level_info   = top_level_info;
    out->binary_sections  = binary_sections;
    rdim_unit_chunk_list_push_array(arena, &out->units, &units);
    rdim_type_chunk_list_push_array(arena, &out->types, &itype_types);
    rdim_type_chunk_list_concat_in_place(&out->types, &extra_types);
    out->udts             = udts;
    out->global_variables = all_global_variables;
    out->thread_variables = all_thread_variables;
    out->procedures       = all_procedures;
    out->scopes           = all_scopes;
  }
  
  //~ TODO(rjf): OLD vvvvvvvvvvvvvvvvvvv
#if 0
  
  // output generation
  P2R_Ctx *p2r_ctx = 0;
  if(in->output_name.size > 0)
  {
    // setup root
    RDIM_RootParams root_params = {0};
    root_params.addr_size = addr_size;
    
    root_params.bucket_count_units = comp_unit_count;
    root_params.bucket_count_symbols = symbol_count_prediction;
    root_params.bucket_count_scopes = symbol_count_prediction;
    root_params.bucket_count_locals = symbol_count_prediction*2;
    root_params.bucket_count_types = tpi->itype_opl;
    root_params.bucket_count_type_constructs = tpi->itype_opl;
    
    RDIM_Root *root = rdim_root_alloc(&root_params);
    out->root = root;
    
    // top level info
    {
      // calculate voff max
      U64 voff_max = 0;
      {        
        COFF_SectionHeader *coff_sec_ptr = coff_sections->sections;
        COFF_SectionHeader *coff_ptr_opl = coff_sec_ptr + coff_section_count;
        for(;coff_sec_ptr < coff_ptr_opl; coff_sec_ptr += 1)
        {
          U64 sec_voff_max = coff_sec_ptr->voff + coff_sec_ptr->vsize;
          voff_max = Max(voff_max, sec_voff_max);
        }
      }
      
      // set top level info
      RDIM_TopLevelInfo tli = {0};
      tli.architecture = architecture;
      tli.exe_name = in->input_exe_name;
      tli.exe_hash = exe_hash;
      tli.voff_max = voff_max;
      
      rdim_set_top_level_info(root, &tli);
    }
    
    
    // setup binary sections
    {
      COFF_SectionHeader *coff_ptr = coff_sections->sections;
      COFF_SectionHeader *coff_opl = coff_ptr + coff_section_count;
      for(;coff_ptr < coff_opl; coff_ptr += 1)
      {
        char *name_first = (char*)coff_ptr->name;
        char *name_opl   = name_first + sizeof(coff_ptr->name);
        String8 name = str8_cstring_capped(name_first, name_opl);
        RDI_BinarySectionFlags flags =
          rdi_binary_section_flags_from_coff_section_flags(coff_ptr->flags);
        rdim_add_binary_section(root, name, flags,
                                coff_ptr->voff, coff_ptr->voff + coff_ptr->vsize,
                                coff_ptr->foff, coff_ptr->foff + coff_ptr->fsize);
      }
    }
    
    
    // setup compilation units
    {
      PDB_CompUnit **units = comp_units->units;
      for(U64 i = 0; i < comp_unit_count; i += 1)
      {
        PDB_CompUnit *unit = units[i];
        CV_SymParsed *unit_sym = sym_for_unit[i];
        CV_C13Parsed *unit_c13 = c13_for_unit[i];
        
        // resolve names
        String8 raw_name = unit->obj_name;
        
        String8 unit_name = raw_name;
        {
          U64 first_after_slashes = 0;
          for(S64 i = unit_name.size - 1; i >= 0; i -= 1)
          {
            if(unit_name.str[i] == '/' || unit_name.str[i] == '\\')
            {
              first_after_slashes = i + 1;
              break;
            }
          }
          unit_name = str8_range(raw_name.str + first_after_slashes,
                                 raw_name.str + raw_name.size);
        }
        
        String8 obj_name = raw_name;
        if(str8_match(obj_name, str8_lit("* Linker *"), 0) ||
           str8_match(obj_name, str8_lit("Import:"),
                      StringMatchFlag_RightSideSloppy))
        {
          MemoryZeroStruct(&obj_name);
        }
        
        String8 compiler_name = unit_sym->info.compiler_name;
        String8 archive_file = unit->group_name;
        
        // extract langauge
        RDI_Language lang = rdi_language_from_cv_language(sym->info.language);
        
        // basic per unit info
        RDIM_Unit *unit_handle = rdim_unit_handle_from_user_id(root, i, i);
        
        RDIM_UnitInfo info = {0};
        info.unit_name = unit_name;
        info.compiler_name = compiler_name;
        info.object_file = obj_name;
        info.archive_file = archive_file;
        info.language = lang;
        
        rdim_unit_set_info(root, unit_handle, &info);
        
        // unit's line info
        for(CV_C13SubSectionNode *node = unit_c13->first_sub_section;
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
              RDIM_LineSequence seq = {0};
              seq.file_name  = lines->file_name;
              seq.voffs      = lines->voffs;
              seq.line_nums  = lines->line_nums;
              seq.col_nums   = lines->col_nums;
              seq.line_count = lines->line_count;
              rdim_unit_add_line_sequence(root, unit_handle, &seq);
            }
          }
        }
      }
    }
    
    
    // unit vmap ranges
    {
      PDB_CompUnitContribution *contrib_ptr = comp_unit_contributions->contributions;
      PDB_CompUnitContribution *contrib_opl = contrib_ptr + comp_unit_contribution_count;
      for(;contrib_ptr < contrib_opl; contrib_ptr += 1)
      {
        if(contrib_ptr->mod < root->unit_count)
        {
          RDIM_Unit *unit_handle = rdim_unit_handle_from_user_id(root, contrib_ptr->mod, contrib_ptr->mod);
          rdim_unit_vmap_add_range(root, unit_handle,
                                   contrib_ptr->voff_first,
                                   contrib_ptr->voff_opl);
        }
      }
    }
    
    // rjf: produce pdb conversion context
    {
      P2R_CtxParams p = {0};
      {
        p.arch = architecture;
        p.tpi_hash = tpi_hash;
        p.tpi_leaf = tpi_leaf;
        p.sections = coff_sections;
        p.fwd_map_bucket_count          = tpi->itype_opl/10;
        p.frame_proc_map_bucket_count   = symbol_count_prediction;
        p.known_global_map_bucket_count = symbol_count_prediction;
        p.link_name_map_bucket_count    = symbol_count_prediction;
      }
      p2r_ctx = p2r_ctx_alloc(&p, root);
    }
    
    // types & symbols
    {
      P2R_TypesSymbolsParams p = {0};
      p.sym = sym;
      p.sym_for_unit = sym_for_unit;
      p.unit_count = comp_unit_count;
      p2r_types_and_symbols(p2r_ctx, &p);
    }
    
    // conversion errors
    if(!in->hide_errors.converting)
    {
      for(RDIM_Msg *msg = rdim_first_msg_from_root(root);
          msg != 0;
          msg = msg->next)
      {
        str8_list_push(arena, &out->errors, msg->string);
      }
    }
  }
  
  // dump
  if(in->dump) ProfScope("dump")
  {
    String8List dump = {0};
    
    // EXE
    if(out->good_parse)
    {
      str8_list_push(arena, &dump,
                     str8_lit("################################"
                              "################################\n"
                              "EXE INFO:\n"));
      {
        str8_list_pushf(arena, &dump, "HASH: %016llX\n", exe_hash);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // MSF
    if(in->dump_msf)
    {
      if(msf != 0)
      {
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "MSF:\n"));
        
        str8_list_pushf(arena, &dump, " block_size=%llu\n", msf->block_size);
        str8_list_pushf(arena, &dump, " block_count=%llu\n", msf->block_count);
        str8_list_pushf(arena, &dump, " stream_count=%llu\n", msf->stream_count);
        
        String8 *stream_ptr = msf->streams;
        U64 stream_count = msf->stream_count;
        for(U64 i = 0; i < stream_count; i += 1, stream_ptr += 1)
        {
          str8_list_pushf(arena, &dump, "  stream[%u].size=%llu\n",
                          i, stream_ptr->size);
        }
        
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // DBI
    if(in->dump_sym)
    {
      if(sym != 0)
      {
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DBI SYM:\n"));
        cv_stringize_sym_parsed(arena, &dump, sym);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // TPI
    if(in->dump_tpi_hash)
    {
      if(tpi_hash != 0)
      {
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "TPI HASH:\n"));
        pdb_stringize_tpi_hash(arena, &dump, tpi_hash);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      
      if(ipi_hash != 0)
      {
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "IPI HASH:\n"));
        pdb_stringize_tpi_hash(arena, &dump, ipi_hash);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // LEAF
    if(in->dump_leaf)
    {
      if(tpi_leaf != 0)
      {
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "TPI LEAF:\n"));
        cv_stringize_leaf_parsed(arena, &dump, tpi_leaf);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      
      if(ipi_leaf != 0)
      {
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "IPI LEAF:\n"));
        cv_stringize_leaf_parsed(arena, &dump, ipi_leaf);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // BINARY SECTIONS
    if(in->dump_coff_sections)
    {
      if(coff_sections != 0)
      {
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "COFF SECTIONS:\n"));
        COFF_SectionHeader *section_ptr = coff_sections->sections;
        for(U64 i = 0; i < coff_section_count; i += 1, section_ptr += 1)
        {
          // TODO(allen): probably should pull this out into a separate stringize path
          // for the coff section type
          char *first = (char*)section_ptr->name;
          char *opl   = first + sizeof(section_ptr->name);
          String8 name = str8_cstring_capped(first, opl);
          str8_list_pushf(arena, &dump, " %.*s:\n", str8_varg(name));
          str8_list_pushf(arena, &dump, "  vsize=%u\n", section_ptr->vsize);
          str8_list_pushf(arena, &dump, "  voff =0x%x\n", section_ptr->voff);
          str8_list_pushf(arena, &dump, "  fsize=%u\n", section_ptr->fsize);
          str8_list_pushf(arena, &dump, "  foff =0x%x\n", section_ptr->foff);
          str8_list_pushf(arena, &dump, "  relocs_foff=0x%x\n", section_ptr->relocs_foff);
          str8_list_pushf(arena, &dump, "  lines_foff =0x%x\n", section_ptr->lines_foff);
          str8_list_pushf(arena, &dump, "  reloc_count=%u\n", section_ptr->reloc_count);
          str8_list_pushf(arena, &dump, "  line_count =%u\n", section_ptr->line_count);
          // TODO(allen): better flags
          str8_list_pushf(arena, &dump, "  flags=%x\n", section_ptr->flags);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
      }
    }
    
    // UNITS
    if(comp_units != 0)
    {
      B32 dump_sym = in->dump_sym;
      B32 dump_c13 = in->dump_c13;
      
      B32 dump_units = (dump_sym || dump_c13);
      
      if(dump_units)
      {
        PDB_CompUnit **unit_ptr = comp_units->units;
        for(U64 i = 0; i < comp_unit_count; i += 1, unit_ptr += 1)
        {
          str8_list_push(arena, &dump,
                         str8_lit("################################"
                                  "################################\n"));
          String8 name = (*unit_ptr)->obj_name;
          String8 group_name = (*unit_ptr)->group_name;
          str8_list_pushf(arena, &dump, "[%llu] %.*s\n(%.*s):\n",
                          i, str8_varg(name), str8_varg(group_name));
          if(dump_sym)
          {
            cv_stringize_sym_parsed(arena, &dump, sym_for_unit[i]);
          }
          if(dump_c13)
          {
            cv_stringize_c13_parsed(arena, &dump, c13_for_unit[i]);
          }
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
      }
    }
    
    // UNIT CONTRIBUTIONS
    if(comp_unit_contributions != 0)
    {
      if(in->dump_contributions)
      {
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "UNIT CONTRIBUTIONS:\n"));
        PDB_CompUnitContribution *contrib_ptr = comp_unit_contributions->contributions;
        for(U64 i = 0; i < comp_unit_contribution_count; i += 1, contrib_ptr += 1)
        {
          str8_list_pushf(arena, &dump,
                          " { mod = %5u; voff_first = %08llx; voff_opl = %08llx; }\n",
                          contrib_ptr->mod, contrib_ptr->voff_first, contrib_ptr->voff_opl);
        }
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // rjf: dump table diagnostics
    if(in->dump_table_diagnostics)
    {
      str8_list_push(arena, &dump,
                     str8_lit("################################"
                              "################################\n"
                              "TABLE DIAGNOSTICS:\n"));
      struct
      {
        String8 name;
        U64 bucket_count;
        U64 value_count;
        U64 collision_count;
      }
      table_info[] =
      {
        {str8_lit("p2r_ctx fwd_map"),        p2r_ctx?p2r_ctx->fwd_map.buckets_count:0,         p2r_ctx?p2r_ctx->fwd_map.pair_count:0,         p2r_ctx?p2r_ctx->fwd_map.bucket_collision_count:0},
        {str8_lit("p2r_ctx frame_proc_map"), p2r_ctx?p2r_ctx->frame_proc_map.buckets_count:0,  p2r_ctx?p2r_ctx->frame_proc_map.pair_count:0,  p2r_ctx?p2r_ctx->frame_proc_map.bucket_collision_count:0},
        {str8_lit("p2r_ctx known_globals"),  p2r_ctx?p2r_ctx->known_globals.buckets_count:0,   p2r_ctx?p2r_ctx->known_globals.global_count:0, p2r_ctx?p2r_ctx->known_globals.bucket_collision_count:0},
        {str8_lit("p2r_ctx link_names"),     p2r_ctx?p2r_ctx->link_names.buckets_count:0,      p2r_ctx?p2r_ctx->link_names.link_name_count:0, p2r_ctx?p2r_ctx->link_names.bucket_collision_count:0},
        {str8_lit("rdim_root unit_map"),         out->root->unit_map.buckets_count,          out->root->unit_map.pair_count,          out->root->unit_map.bucket_collision_count},
        {str8_lit("rdim_root symbol_map"),       out->root->symbol_map.buckets_count,        out->root->symbol_map.pair_count,        out->root->symbol_map.bucket_collision_count},
        {str8_lit("rdim_root scope_map"),        out->root->scope_map.buckets_count,         out->root->scope_map.pair_count,         out->root->scope_map.bucket_collision_count},
        {str8_lit("rdim_root local_map"),        out->root->local_map.buckets_count,         out->root->local_map.pair_count,         out->root->local_map.bucket_collision_count},
        {str8_lit("rdim_root type_from_id_map"), out->root->type_from_id_map.buckets_count,  out->root->type_from_id_map.pair_count,  out->root->type_from_id_map.bucket_collision_count},
        {str8_lit("rdim_root construct_map"),    out->root->construct_map.buckets_count,     out->root->construct_map.pair_count,     out->root->construct_map.bucket_collision_count},
      };
      for(U64 idx = 0; idx < ArrayCount(table_info); idx += 1)
      {
        str8_list_pushf(arena, &dump, "%S: %I64u values in %I64u buckets, with %I64u collisions (%f fill)\n",
                        table_info[idx].name,
                        table_info[idx].value_count,
                        table_info[idx].bucket_count,
                        table_info[idx].collision_count,
                        (F64)table_info[idx].value_count / (F64)table_info[idx].bucket_count);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    out->dump = dump;
  }
  
#endif
  
  scratch_end(scratch);
  return out;
}
