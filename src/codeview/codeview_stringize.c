// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ CodeView Common Stringize Functions

internal void
cv_stringize_numeric(Arena *arena, String8List *out, CV_NumericParsed *num){
  String8 numeric_kind_str = cv_string_from_numeric_kind(num->kind);
  str8_list_pushf(arena, out, "(%.*s)", str8_varg(numeric_kind_str));
  
  if (cv_numeric_fits_in_u64(num)){
    U64 n = cv_u64_from_numeric(num);
    str8_list_pushf(arena, out, "(%llu)", n);
  }
  else if (cv_numeric_fits_in_s64(num)){
    S64 n = cv_s64_from_numeric(num);
    str8_list_pushf(arena, out, "(%lld)", n);
  }
  else if (cv_numeric_fits_in_f64(num)){
    F64 n = cv_f64_from_numeric(num);
    str8_list_pushf(arena, out, "(%f)", n);
  }
}

internal void
cv_stringize_lvar_addr_range(Arena *arena, String8List *out, CV_LvarAddrRange *range){
  str8_list_pushf(arena, out, "{off=%x, sec=%u, len=%u}", range->off, range->sec, range->len);
}

internal void
cv_stringize_lvar_addr_gap(Arena *arena, String8List *out, CV_LvarAddrGap *gap){
  str8_list_pushf(arena, out, "{off=%x, len=%u}", gap->off, gap->len);
}

internal void
cv_stringize_lvar_addr_gap_list(Arena *arena, String8List *out, void *first, void *opl){
  U64 gap_count = ((U8*)first - (U8*)opl)/sizeof(CV_LvarAddrGap);
  if (gap_count > 0){
    str8_list_push(arena, out, str8_lit(" gaps=\n"));
    CV_LvarAddrGap *gap = (CV_LvarAddrGap*)first;
    CV_LvarAddrGap *opl = gap + gap_count;
    for (;gap < opl; gap += 1){
      str8_list_push(arena, out, str8_lit("  "));
      cv_stringize_lvar_addr_gap(arena, out, gap);
      str8_list_push(arena, out, str8_lit("\n"));
    }
  }
}

internal String8
cv_string_from_c13_sub_section_kind(CV_C13SubSectionKind kind){
  String8 result = str8_lit("UNRECOGNIZED_C13_SUB_SECTION_KIND");
  switch (kind){
    case 0: str8_lit("PARSE_ERROR"); break;
#define X(N,c) case CV_C13SubSectionKind_##N: result = str8_lit(#N); break;
    CV_C13SubSectionKindXList(X)
#undef X
  }
  return(result);
}

internal String8
cv_string_from_reg(CV_Arch arch, CV_Reg reg){
  String8 result = {0};
  switch (arch){
    default: result = str8_lit("<missing-regs-for-arch>"); break;
    
    case CV_Arch_8086:
    {
      switch (reg){
#define X(CVN,C,RDN,BP,BZ) case CV_Regx86_##CVN: result = str8_lit(#CVN); break;
        CV_Reg_X86_XList(X)
#undef X
      }
    }break;
    
    case CV_Arch_X64:
    {
      switch (reg){
#define X(CVN,C,RDN,BP,BZ) case CV_Regx64_##CVN: result = str8_lit(#CVN); break;
        CV_Reg_X64_XList(X)
#undef X
      }
    }break;
  }
  return(result);
}

internal String8
cv_string_from_pointer_kind(CV_PointerKind ptr_kind){
  String8 result = {0};
  switch (ptr_kind){
    default:                         result = str8_lit("<invalid-ptr-kind>"); break;
    case CV_PointerKind_Near:        result = str8_lit("Near"); break;
    case CV_PointerKind_Far:         result = str8_lit("Far"); break;
    case CV_PointerKind_Huge:        result = str8_lit("Huge"); break;
    case CV_PointerKind_BaseSeg:     result = str8_lit("BaseSeg"); break;
    case CV_PointerKind_BaseVal:     result = str8_lit("BaseVal"); break;
    case CV_PointerKind_BaseSegVal:  result = str8_lit("BaseSegVal"); break;
    case CV_PointerKind_BaseAddr:    result = str8_lit("BaseAddr"); break;
    case CV_PointerKind_BaseSegAddr: result = str8_lit("BaseSegAddr"); break;
    case CV_PointerKind_BaseType:    result = str8_lit("BaseType"); break;
    case CV_PointerKind_BaseSelf:    result = str8_lit("BaseSelf"); break;
    case CV_PointerKind_Near32:      result = str8_lit("Near32"); break;
    case CV_PointerKind_Far32:       result = str8_lit("Far32"); break;
    case CV_PointerKind_64:          result = str8_lit("64"); break;
  }
  return(result);
}

internal String8
cv_string_from_pointer_mode(CV_PointerMode ptr_mode){
  String8 result = {0};
  switch (ptr_mode){
    default:                       result = str8_lit("<invalid-ptr-mode>"); break;
    case CV_PointerMode_Ptr:       result = str8_lit("Ptr"); break;
    case CV_PointerMode_LRef:      result = str8_lit("LRef"); break;
    case CV_PointerMode_PtrMem:    result = str8_lit("PtrMem"); break;
    case CV_PointerMode_PtrMethod: result = str8_lit("PtrMethod"); break;
    case CV_PointerMode_RRef:      result = str8_lit("RRef"); break;
  }
  return(result);
}

internal String8
cv_string_from_hfa_kind(CV_HFAKind hfa_kind){
  String8 result = {0};
  switch (hfa_kind){
    default:                result = str8_lit("<invalid-hfa>"); break;
    case CV_HFAKind_None:   result = str8_lit("None"); break;
    case CV_HFAKind_Float:  result = str8_lit("Float"); break;
    case CV_HFAKind_Double: result = str8_lit("Double"); break;
    case CV_HFAKind_Other:  result = str8_lit("Other"); break;
  }
  return(result);
}

internal String8
cv_string_from_mo_com_udt_kind(CV_MoComUDTKind mo_com_udt_kind){
  String8 result = {0};
  switch (mo_com_udt_kind){
    default:                        result = str8_lit("<invalid-mocom>"); break;
    case CV_MoComUDTKind_None:      result = str8_lit("None"); break;
    case CV_MoComUDTKind_Ref:       result = str8_lit("Ref"); break;
    case CV_MoComUDTKind_Value:     result = str8_lit("Value"); break;
    case CV_MoComUDTKind_Interface: result = str8_lit("Interface"); break;
  }
  return(result);
}

////////////////////////////////
//~ CodeView Flags Stringize Functions

global char cv_stringize_spaces[] = "                                ";

#define SPACES cv_stringize_spaces

internal void
cv_stringize_modifier_flags(Arena *arena, String8List *out,
                            U32 indent, CV_ModifierFlags flags){
  if (flags & CV_ModifierFlag_Const){
    str8_list_pushf(arena, out, "%.*sConst\n", indent, SPACES);
  }
  if (flags & CV_ModifierFlag_Volatile){
    str8_list_pushf(arena, out, "%.*sVolatile\n", indent, SPACES);
  }
  if (flags & CV_ModifierFlag_Unaligned){
    str8_list_pushf(arena, out, "%.*sUnaligned\n", indent, SPACES);
  }
}

internal void
cv_stringize_type_props(Arena *arena, String8List *out,
                        U32 indent, CV_TypeProps props){
  if (props & CV_TypeProp_Packed){
    str8_list_pushf(arena, out, "%.*sPacked\n", indent, SPACES);
  }
  if (props & CV_TypeProp_HasConstructorsDestructors){
    str8_list_pushf(arena, out, "%.*sHasConstructorsDesctructors\n", indent, SPACES);
  }
  if (props & CV_TypeProp_OverloadedOperators){
    str8_list_pushf(arena, out, "%.*sOverloadedOperators\n", indent, SPACES);
  }
  if (props & CV_TypeProp_IsNested){
    str8_list_pushf(arena, out, "%.*sIsNested\n", indent, SPACES);
  }
  if (props & CV_TypeProp_ContainsNested){
    str8_list_pushf(arena, out, "%.*sContainsNested\n", indent, SPACES);
  }
  if (props & CV_TypeProp_OverloadedAssignment){
    str8_list_pushf(arena, out, "%.*sOverloadedAssignment\n", indent, SPACES);
  }
  if (props & CV_TypeProp_OverloadedCasting){
    str8_list_pushf(arena, out, "%.*sOverloadedCasting\n", indent, SPACES);
  }
  if (props & CV_TypeProp_FwdRef){
    str8_list_pushf(arena, out, "%.*sFwdRef\n", indent, SPACES);
  }
  if (props & CV_TypeProp_Scoped){
    str8_list_pushf(arena, out, "%.*sScoped\n", indent, SPACES);
  }
  if (props & CV_TypeProp_HasUniqueName){
    str8_list_pushf(arena, out, "%.*sHasUniqueName\n", indent, SPACES);
  }
  if (props & CV_TypeProp_Sealed){
    str8_list_pushf(arena, out, "%.*sSealed\n", indent, SPACES);
  }
  if (props & CV_TypeProp_Intrinsic){
    str8_list_pushf(arena, out, "%.*sIntrinsic\n", indent, SPACES);
  }
  
  CV_HFAKind hfa = CV_TypeProps_ExtractHFA(props);
  {
    String8 hfa_str = cv_string_from_hfa_kind(hfa);
    str8_list_pushf(arena, out, "%.*shfa=%.*s\n",
                    indent, SPACES, str8_varg(hfa_str));
  }
  
  CV_MoComUDTKind mo_com = CV_TypeProps_ExtractMOCOM(props);
  {
    String8 mo_com_str = cv_string_from_mo_com_udt_kind(mo_com);
    str8_list_pushf(arena, out, "%.*smocom=%.*s\n",
                    indent, SPACES, str8_varg(mo_com_str));
  }
}

internal void
cv_stringize_pointer_attribs(Arena *arena, String8List *out,
                             U32 indent, CV_PointerAttribs attribs){
  if (attribs & CV_PointerAttrib_IsFlat){
    str8_list_pushf(arena, out, "%.*sIsFlat\n", indent, SPACES);
  }
  if (attribs & CV_PointerAttrib_Volatile){
    str8_list_pushf(arena, out, "%.*sVolatile\n", indent, SPACES);
  }
  if (attribs & CV_PointerAttrib_Const){
    str8_list_pushf(arena, out, "%.*sConst\n", indent, SPACES);
  }
  if (attribs & CV_PointerAttrib_Unaligned){
    str8_list_pushf(arena, out, "%.*sUnaligned\n", indent, SPACES);
  }
  if (attribs & CV_PointerAttrib_Restricted){
    str8_list_pushf(arena, out, "%.*sRestricted\n", indent, SPACES);
  }
  if (attribs & CV_PointerAttrib_MOCOM){
    str8_list_pushf(arena, out, "%.*sMOCOM\n", indent, SPACES);
  }
  if (attribs & CV_PointerAttrib_LRef){
    str8_list_pushf(arena, out, "%.*sLRef\n", indent, SPACES);
  }
  if (attribs & CV_PointerAttrib_RRef){
    str8_list_pushf(arena, out, "%.*sRRef\n", indent, SPACES);
  }
  
  CV_PointerKind kind = CV_PointerAttribs_ExtractKind(attribs);
  {
    String8 kind_str = cv_string_from_pointer_kind(kind);
    str8_list_pushf(arena, out, "%.*skind=%.*s\n",
                    indent, SPACES, str8_varg(kind_str));
  }
  
  CV_PointerMode mode = CV_PointerAttribs_ExtractMode(attribs);
  {
    String8 mode_str = cv_string_from_pointer_mode(mode);
    str8_list_pushf(arena, out, "%.*smode=%.*s\n",
                    indent, SPACES, str8_varg(mode_str));
  }
  
  U32 size = CV_PointerAttribs_ExtractSize(attribs);
  str8_list_pushf(arena, out, "%.*ssize=%u\n",
                  indent, SPACES, size);
}

internal void
cv_stringize_local_flags(Arena *arena, String8List *out,
                         U32 indent, CV_LocalFlags flags){
  if (flags & CV_LocalFlag_Param){
    str8_list_pushf(arena, out, "%.*sParam\n", indent, SPACES);
  }
  if (flags & CV_LocalFlag_AddrTaken){
    str8_list_pushf(arena, out, "%.*sAddrTaken\n", indent, SPACES);
  }
  if (flags & CV_LocalFlag_Compgen){
    str8_list_pushf(arena, out, "%.*sCompgen\n", indent, SPACES);
  }
  if (flags & CV_LocalFlag_Aggregate){
    str8_list_pushf(arena, out, "%.*sAggregate\n", indent, SPACES);
  }
  if (flags & CV_LocalFlag_PartOfAggregate){
    str8_list_pushf(arena, out, "%.*sPartOfAggregate\n", indent, SPACES);
  }
  if (flags & CV_LocalFlag_Aliased){
    str8_list_pushf(arena, out, "%.*sAliased\n", indent, SPACES);
  }
  if (flags & CV_LocalFlag_Alias){
    str8_list_pushf(arena, out, "%.*sAlias\n", indent, SPACES);
  }
  if (flags & CV_LocalFlag_Retval){
    str8_list_pushf(arena, out, "%.*sRetval\n", indent, SPACES);
  }
  if (flags & CV_LocalFlag_OptOut){
    str8_list_pushf(arena, out, "%.*sOptOut\n", indent, SPACES);
  }
  if (flags & CV_LocalFlag_Global){
    str8_list_pushf(arena, out, "%.*sGlobal\n", indent, SPACES);
  }
  if (flags & CV_LocalFlag_Static){
    str8_list_pushf(arena, out, "%.*sStatic\n", indent, SPACES);
  }
}


#undef SPACES

////////////////////////////////
//~ CodeView Sym Stringize Functions

internal void
cv_stringize_sym_parsed(Arena *arena, String8List *out, CV_SymParsed *sym){
  CV_StringizeSymParams params = {0};
  params.arch = sym->info.arch;
  
  cv_stringize_sym_array(arena, out, &sym->sym_ranges, sym->data, &params);
}

internal void
cv_stringize_sym_range(Arena *arena, String8List *out,
                       CV_RecRange *range, String8 data,
                       CV_StringizeSymParams *p){
  U64 opl_off = range->off + range->hdr.size;
  if (opl_off > data.size){
    str8_list_push(arena, out, str8_lit("bad symbol range\n"));
  }
  
  if (opl_off <= data.size){
    // [off]: kind
    {
      String8 kind_str = cv_string_from_sym_kind(range->hdr.kind);
      str8_list_pushf(arena, out, "[%06x]: %.*s\n",
                      range->off + 2, str8_varg(kind_str));
    }
    
    // details
    U8 *first = data.str + range->off + 2;
    U64 cap = range->hdr.size - 2;
    
    switch (range->hdr.kind){
      default:break;
      
      case CV_SymKind_COMPILE:
      {
        if (sizeof(CV_SymCompile) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymCompile *compile = (CV_SymCompile*)first;
          
          // machine
          String8 machine = cv_string_from_arch(compile->machine);
          str8_list_pushf(arena, out, " machine=%.*s\n",
                          str8_varg(machine));
          
          // flags
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " flags=%x\n", compile->flags);
          
          // ver_str
          String8 ver_str = str8_cstring_capped((char*)(compile + 1), first + cap);
          str8_list_pushf(arena, out, " ver_str='%.*s'\n", str8_varg(ver_str));
        }
      }break;
      
      case CV_SymKind_END:
      {
        // no contents
      }break;
      
      case CV_SymKind_FRAMEPROC:
      {
        if (sizeof(CV_SymFrameproc) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymFrameproc *frameproc = (CV_SymFrameproc*)first;
          
          // frame sizes and offsets
          str8_list_pushf(arena, out, " frame_size=%u\n",
                          frameproc->frame_size);
          str8_list_pushf(arena, out, " pad_size=%u\n",
                          frameproc->pad_size);
          str8_list_pushf(arena, out, " pad_off=%u\n",
                          frameproc->pad_off);
          str8_list_pushf(arena, out, " save_reg_size=%u\n",
                          frameproc->save_reg_size);
          str8_list_pushf(arena, out, " eh_off=%x\n",
                          frameproc->eh_off);
          
          // eh section
          str8_list_pushf(arena, out, " eh_sec=%u\n",
                          frameproc->eh_sec);
          
          // flags
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " flags=%x\n", frameproc->flags);
        }
      }break;
      
      case CV_SymKind_OBJNAME:
      {
        if (sizeof(CV_SymObjname) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymObjname *objname = (CV_SymObjname*)first;
          
          // sig
          str8_list_pushf(arena, out, " sig=%u\n", objname->sig);
          
          // name
          String8 name = str8_cstring_capped((char*)(objname + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_THUNK32:
      {
        if (sizeof(CV_SymThunk32) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymThunk32 *thunk32 = (CV_SymThunk32*)first;
          
          // members
          str8_list_pushf(arena, out, " parent=%x\n", thunk32->parent);
          str8_list_pushf(arena, out, " end=%x\n", thunk32->end);
          str8_list_pushf(arena, out, " next=%x\n", thunk32->next);
          str8_list_pushf(arena, out, " off=%u\n", thunk32->off);
          str8_list_pushf(arena, out, " sec=%u\n", thunk32->sec);
          str8_list_pushf(arena, out, " len=%u\n", thunk32->len);
          
          // ord
          // TODO(allen): better ord path
          str8_list_pushf(arena, out, " ord=%u\n", thunk32->ord);
          
          // name
          String8 name = str8_cstring_capped((char*)(thunk32 + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
          
          // variant
          String8 variant = str8_cstring_capped(name.str + name.size + 1, first + cap);
          str8_list_pushf(arena, out, " variant='%.*s'\n", str8_varg(variant));
        }
      }break;
      
      case CV_SymKind_BLOCK32:
      {
        if (sizeof(CV_SymBlock32) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymBlock32 *block32 = (CV_SymBlock32*)first;
          
          // block attributes
          str8_list_pushf(arena, out, " parent=%x\n", block32->parent);
          str8_list_pushf(arena, out, " end=%x\n", block32->end);
          str8_list_pushf(arena, out, " len=%u\n", block32->len);
          str8_list_pushf(arena, out, " off=%x\n", block32->off);
          str8_list_pushf(arena, out, " sec=%u\n", block32->sec);
          
          // name
          String8 name = str8_cstring_capped((char*)(block32 + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n",
                          str8_varg(name));
        }
      }break;
      
      case CV_SymKind_LABEL32:
      {
        if (sizeof(CV_SymLabel32) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymLabel32 *label32 = (CV_SymLabel32*)first;
          
          // label attributes
          str8_list_pushf(arena, out, " off=%x\n", label32->off);
          str8_list_pushf(arena, out, " sec=%u\n", label32->sec);
          
          // flags
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " flags=%x\n", label32->flags);
          
          // name
          String8 name = str8_cstring_capped((char*)(label32 + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n",
                          str8_varg(name));
        }
      }break;
      
      case CV_SymKind_CONSTANT:
      {
        if (sizeof(CV_SymConstant) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymConstant *constant = (CV_SymConstant*)first;
          
          // itype
          str8_list_pushf(arena, out, " itype=%u\n", constant->itype);
          
          // num
          U8 *numeric_ptr = (U8*)(constant + 1);
          CV_NumericParsed numeric = cv_numeric_from_data_range(numeric_ptr, first + cap);
          str8_list_push(arena, out, str8_lit(" num="));
          cv_stringize_numeric(arena, out, &numeric);
          str8_list_push(arena, out, str8_lit("\n"));
          
          // name
          U8 *name_ptr = numeric_ptr + numeric.encoded_size;
          String8 name = str8_cstring_capped((char*)(name_ptr), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_UDT:
      {
        if (sizeof(CV_SymUDT) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymUDT *udt = (CV_SymUDT*)first;
          
          // itype
          str8_list_pushf(arena, out, " itype=%u\n", udt->itype);
          
          // name
          String8 name = str8_cstring_capped((char*)(udt + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_LDATA32:
      case CV_SymKind_GDATA32:
      {
        if (sizeof(CV_SymData32) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymData32 *data32 = (CV_SymData32*)first;
          
          // itype, off & sec
          str8_list_pushf(arena, out, " itype=%u\n off=%x\n sec=%u\n",
                          data32->itype, data32->off, data32->sec);
          
          // name
          String8 name = str8_cstring_capped((char*)(data32 + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_PUB32:
      {
        if (sizeof(CV_SymPub32) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymPub32 *pub32 = (CV_SymPub32*)first;
          
          // flags
          CV_PubFlags flags = pub32->flags;
          str8_list_push(arena, out, str8_lit(" flags="));
          if (flags == 0){
            str8_list_push(arena, out, str8_lit("0|"));
          }
          else{
            if (flags&CV_PubFlag_Code){
              str8_list_push(arena, out, str8_lit("Code|"));
            }
            if (flags&CV_PubFlag_Function){
              str8_list_push(arena, out, str8_lit("Function|"));
            }
            if (flags&CV_PubFlag_ManagedCode){
              str8_list_push(arena, out, str8_lit("ManagedCode|"));
            }
            if (flags&CV_PubFlag_MSIL){
              str8_list_push(arena, out, str8_lit("MSIL|"));
            }
          }
          str8_list_push(arena, out, str8_lit("\n"));
          
          // off & sec
          str8_list_pushf(arena, out, " off=%x\n sec=%u\n", pub32->off, pub32->sec);
          
          // name
          String8 name = str8_cstring_capped((char*)(pub32 + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_LPROC32:
      case CV_SymKind_GPROC32:
      {
        if (sizeof(CV_SymProc32) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymProc32 *proc32 = (CV_SymProc32*)first;
          
          // proc attributes
          str8_list_pushf(arena, out, " parent=%x\n", proc32->parent);
          str8_list_pushf(arena, out, " end=%x\n", proc32->end);
          str8_list_pushf(arena, out, " next=%x\n", proc32->next);
          str8_list_pushf(arena, out, " len=%u\n", proc32->len);
          str8_list_pushf(arena, out, " dbg_start=%x\n", proc32->dbg_start);
          str8_list_pushf(arena, out, " dbg_end=%x\n", proc32->dbg_end);
          str8_list_pushf(arena, out, " itype=%u\n", proc32->itype);
          str8_list_pushf(arena, out, " off=%x\n", proc32->off);
          str8_list_pushf(arena, out, " sec=%u\n", proc32->sec);
          
          // flags
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " flags=%x\n", proc32->flags);
          
          // name
          String8 name = str8_cstring_capped((char*)(proc32 + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_REGREL32:
      {
        if (sizeof(CV_SymRegrel32) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymRegrel32 *regrel32 = (CV_SymRegrel32*)first;
          
          // regrel attributes
          str8_list_pushf(arena, out, " reg_off=%u\n", regrel32->reg_off);
          str8_list_pushf(arena, out, " itype=%u\n", regrel32->itype);
          
          // reg
          String8 reg = cv_string_from_reg(p->arch, regrel32->reg);
          str8_list_pushf(arena, out, " reg=%.*s\n", str8_varg(reg));
          
          // name
          String8 name = str8_cstring_capped((char*)(regrel32 + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_LTHREAD32:
      case CV_SymKind_GTHREAD32:
      {
        if (sizeof(CV_SymThread32) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymThread32 *thread32 = (CV_SymThread32*)first;
          
          // itype, tls_off, tls_seg
          str8_list_pushf(arena, out, " itype=%u\n tls_off=%x\n tls_seg=%u\n",
                          thread32->itype, thread32->tls_off, thread32->tls_seg);
          
          // name
          String8 name = str8_cstring_capped((char*)(thread32 + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_COMPILE2:
      {
        if (sizeof(CV_SymCompile2) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymCompile2 *compile2 = (CV_SymCompile2*)first;
          
          // flags
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " flags=%x\n", compile2->flags);
          
          // machine
          String8 machine = cv_string_from_arch(compile2->machine);
          str8_list_pushf(arena, out, " machine=%.*s\n",
                          str8_varg(machine));
          
          // ver
          str8_list_pushf(arena, out,
                          " ver_fe_major=%u\n ver_fe_minor=%u\n ver_fe_build=%u\n"
                          " ver_major=%u\n ver_minor=%u\n ver_build=%u\n",
                          compile2->ver_fe_major, compile2->ver_fe_minor, compile2->ver_fe_build,
                          compile2->ver_major, compile2->ver_minor, compile2->ver_build);
          
          // ver_str
          String8 ver_str = str8_cstring_capped((char*)(compile2 + 1), first + cap);
          str8_list_pushf(arena, out, " ver_str='%.*s'\n", str8_varg(ver_str));
        }
      }break;
      
      case CV_SymKind_UNAMESPACE:
      {
        CV_SymUNamespace *unamespace = (CV_SymUNamespace*)first;
        
        // name
        String8 name = str8_cstring_capped((char*)(unamespace), first + cap);
        str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
      }break;
      
      case CV_SymKind_PROCREF:
      case CV_SymKind_DATAREF:
      case CV_SymKind_LPROCREF:
      {
        if (sizeof(CV_SymRef2) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymRef2 *ref2 = (CV_SymRef2*)first;
          
          // suc_name, sym_off & imod
          str8_list_pushf(arena, out, " suc_name=%u\n sym_off=%x\n imod=%u\n",
                          ref2->suc_name, ref2->sym_off, ref2->imod);
          
          // name
          String8 name = str8_cstring_capped((char*)(ref2 + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_TRAMPOLINE:
      {
        if (sizeof(CV_SymTrampoline) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymTrampoline *trampoline = (CV_SymTrampoline*)first;
          
          // kind
          // TODO(allen): better kind path
          str8_list_pushf(arena, out, " kind=%u\n", trampoline->kind);
          
          // members
          str8_list_pushf(arena, out, " thunk_size=%u\n", trampoline->thunk_size);
          str8_list_pushf(arena, out, " thunk_sec_off=%x\n", trampoline->thunk_sec_off);
          str8_list_pushf(arena, out, " target_sec_off=%x\n", trampoline->target_sec_off);
          str8_list_pushf(arena, out, " thunk_sec=%u\n", trampoline->thunk_sec);
          str8_list_pushf(arena, out, " target_sec=%u\n", trampoline->target_sec);
        }
      }break;
      
      case CV_SymKind_SECTION:
      {
        if (sizeof(CV_SymSection) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymSection *section = (CV_SymSection*)first;
          
          // members
          str8_list_pushf(arena, out, " sec_index=%u\n", section->sec_index);
          str8_list_pushf(arena, out, " align=%u\n", section->align);
          str8_list_pushf(arena, out, " pad=%u\n", section->pad);
          str8_list_pushf(arena, out, " rva=%x\n", section->rva);
          str8_list_pushf(arena, out, " size=%u\n", section->size);
          str8_list_pushf(arena, out, " characteristics=%x\n", section->characteristics);
          
          // name
          String8 name = str8_cstring_capped((char*)(section + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_COFFGROUP:
      {
        if (sizeof(CV_SymCoffGroup) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymCoffGroup *coff_group = (CV_SymCoffGroup*)first;
          
          // members
          str8_list_pushf(arena, out, " size=%u\n", coff_group->size);
          str8_list_pushf(arena, out, " characteristics=%x\n", coff_group->characteristics);
          str8_list_pushf(arena, out, " off=%x\n", coff_group->off);
          str8_list_pushf(arena, out, " sec=%u\n", coff_group->sec);
          
          // name
          String8 name = str8_cstring_capped((char*)(coff_group + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_CALLSITEINFO:
      {
        if (sizeof(CV_SymCallSiteInfo) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymCallSiteInfo *callsiteinfo = (CV_SymCallSiteInfo*)first;
          
          // callsite info attributes
          str8_list_pushf(arena, out, " off=%x\n", callsiteinfo->off);
          str8_list_pushf(arena, out, " sec=%u\n", callsiteinfo->sec);
          str8_list_pushf(arena, out, " pad=%u\n", callsiteinfo->pad);
          str8_list_pushf(arena, out, " itype=%u\n", callsiteinfo->itype);
        }
      }break;
      
      case CV_SymKind_FRAMECOOKIE:
      {
        if (sizeof(CV_SymFrameCookie) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymFrameCookie *framecookie = (CV_SymFrameCookie*)first;
          
          // off
          str8_list_pushf(arena, out, " off=%x\n", framecookie->off);
          
          // reg
          String8 reg = cv_string_from_reg(p->arch, framecookie->reg);
          str8_list_pushf(arena, out, " reg=%.*s\n",
                          str8_varg(reg));
          
          // kind
          // TODO(allen): better kind path
          str8_list_pushf(arena, out, " kind=%x\n", framecookie->kind);
          
          // flags
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " flags=%x\n", framecookie->flags);
        }
      }break;
      
      case CV_SymKind_COMPILE3:
      {
        if (sizeof(CV_SymCompile3) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymCompile3 *compile3 = (CV_SymCompile3*)first;
          
          // flags
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " flags=%x\n", compile3->flags);
          
          // machine
          String8 machine = cv_string_from_arch(compile3->machine);
          str8_list_pushf(arena, out, " machine=%.*s\n",
                          str8_varg(machine));
          
          // ver
          str8_list_pushf(arena, out,
                          " ver_fe_major=%u\n ver_fe_minor=%u\n ver_fe_build=%u\n"
                          " ver_major=%u\n ver_minor=%u\n ver_build=%u\n"
                          " ver_qfe=%u\n",
                          compile3->ver_fe_major, compile3->ver_fe_minor, compile3->ver_fe_build,
                          compile3->ver_major, compile3->ver_minor, compile3->ver_build);
          // ver_str
          String8 ver_str = str8_cstring_capped((char*)(compile3 + 1), first + cap);
          str8_list_pushf(arena, out, " ver_str='%.*s'\n", str8_varg(ver_str));
        }
      }break;
      
      case CV_SymKind_ENVBLOCK:
      {
        if (sizeof(CV_SymEnvBlock) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymEnvBlock *envblock = (CV_SymEnvBlock*)first;
          
          // flags
          str8_list_pushf(arena, out, " flags=%x\n", envblock->flags);
          
          // name
          str8_list_pushf(arena, out, " rgsz=\n");
          char *name_ptr = (char*)(envblock + 1);
          for (;;){
            String8 name = str8_cstring_capped(name_ptr, first + cap);
            if (name.size == 0){
              break;
            }
            str8_list_pushf(arena, out, "  '%.*s'\n", str8_varg(name));
            name_ptr += name.size + 1;
          }
        }
      }break;
      
      case CV_SymKind_LOCAL:
      {
        if (sizeof(CV_SymLocal) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymLocal *slocal = (CV_SymLocal*)first;
          
          // itype
          str8_list_pushf(arena, out, " itype=%u\n", slocal->itype);
          
          // flags
          str8_list_pushf(arena, out, " flags={\n");
          cv_stringize_local_flags(arena, out, 2, slocal->flags);
          str8_list_pushf(arena, out, " }\n");
          
          // name
          String8 name = str8_cstring_capped((char*)(slocal + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_DEFRANGE_REGISTER:
      {
        if (sizeof(CV_SymDefrangeRegister) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymDefrangeRegister *defrange_register = (CV_SymDefrangeRegister*)first;
          
          // reg
          String8 reg = cv_string_from_reg(p->arch, defrange_register->reg);
          str8_list_pushf(arena, out, " reg=%.*s\n", str8_varg(reg));
          
          // range attribs
          // TODO(allen): better range attribs
          str8_list_pushf(arena, out, " attribs=%x\n", defrange_register->attribs);
          
          // addr range
          str8_list_push(arena, out, str8_lit(" range="));
          cv_stringize_lvar_addr_range(arena, out, &defrange_register->range);
          str8_list_push(arena, out, str8_lit("\n"));
          
          // gaps
          cv_stringize_lvar_addr_gap_list(arena, out, defrange_register + 1, first + cap);
        }
      }break;
      
      case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL:
      {
        if (sizeof(CV_SymDefrangeFramepointerRel) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymDefrangeFramepointerRel *defrange_fprel = (CV_SymDefrangeFramepointerRel*)first;
          
          // off
          str8_list_pushf(arena, out, " off=%u\n", defrange_fprel->off);
          
          // addr range
          str8_list_push(arena, out, str8_lit(" range="));
          cv_stringize_lvar_addr_range(arena, out, &defrange_fprel->range);
          str8_list_push(arena, out, str8_lit("\n"));
          
          // gaps
          cv_stringize_lvar_addr_gap_list(arena, out, defrange_fprel + 1, first + cap);
        }
      }break;
      
      case CV_SymKind_DEFRANGE_SUBFIELD_REGISTER:
      {
        if (sizeof(CV_SymDefrangeSubfieldRegister) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymDefrangeSubfieldRegister *defrange_subfield_register = (CV_SymDefrangeSubfieldRegister*)first;
          
          // reg
          String8 reg = cv_string_from_reg(p->arch, defrange_subfield_register->reg);
          str8_list_pushf(arena, out, " reg=%.*s\n", str8_varg(reg));
          
          // range attribs
          // TODO(allen): better range attribs
          str8_list_pushf(arena, out, " attribs=%x\n", defrange_subfield_register->attribs);
          
          // offset
          str8_list_pushf(arena, out, " field_offset=%u\n",
                          defrange_subfield_register->field_offset);
          
          // addr range
          str8_list_push(arena, out, str8_lit(" range="));
          cv_stringize_lvar_addr_range(arena, out, &defrange_subfield_register->range);
          str8_list_push(arena, out, str8_lit("\n"));
          
          // gaps
          cv_stringize_lvar_addr_gap_list(arena, out, defrange_subfield_register + 1, first + cap);
        }
      }break;
      
      case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
      {
        if (sizeof(CV_SymDefrangeFramepointerRelFullScope) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymDefrangeFramepointerRelFullScope *defrange_fprel_full_scope =
          (CV_SymDefrangeFramepointerRelFullScope*)first;
          
          // off
          str8_list_pushf(arena, out, " off=%u\n", defrange_fprel_full_scope->off);
        }
      }break;
      
      case CV_SymKind_DEFRANGE_REGISTER_REL:
      {
        if (sizeof(CV_SymDefrangeRegisterRel) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymDefrangeRegisterRel *defrange_register_rel = (CV_SymDefrangeRegisterRel*)first;
          
          // reg
          String8 reg = cv_string_from_reg(p->arch, defrange_register_rel->reg);
          str8_list_pushf(arena, out, " reg=%.*s\n", str8_varg(reg));
          
          // flags
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " flags=%x\n", defrange_register_rel->flags);
          
          // reg off
          str8_list_pushf(arena, out, " reg_off=%u\n", defrange_register_rel->reg_off);
          
          // addr range
          str8_list_push(arena, out, str8_lit(" range="));
          cv_stringize_lvar_addr_range(arena, out, &defrange_register_rel->range);
          str8_list_push(arena, out, str8_lit("\n"));
          
          // gaps
          cv_stringize_lvar_addr_gap_list(arena, out, defrange_register_rel + 1, first + cap);
        }
      }break;
      
      case CV_SymKind_BUILDINFO:
      {
        if (sizeof(CV_SymBuildInfo) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymBuildInfo *buildinfo = (CV_SymBuildInfo*)first;
          
          // item id
          str8_list_pushf(arena, out, " id=%u\n", buildinfo->id);
        }
      }break;
      
      case CV_SymKind_INLINESITE:
      {
        if (sizeof(CV_SymInlineSite) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymInlineSite *inlinesite = (CV_SymInlineSite*)first;
          
          // members
          str8_list_pushf(arena, out, " parent=%x\n", inlinesite->parent);
          str8_list_pushf(arena, out, " end=%x\n", inlinesite->end);
          str8_list_pushf(arena, out, " inlinee=%u\n", inlinesite->inlinee);
          
          // binary annotation
          // TODO(allen):
        }
      }break;
      
      case CV_SymKind_INLINESITE_END:
      {
        // no contents
      }break;
      
      case CV_SymKind_FILESTATIC:
      {
        if (sizeof(CV_SymFileStatic) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymFileStatic *file_static = (CV_SymFileStatic*)first;
          
          // members
          str8_list_pushf(arena, out, " itype=%u\n", file_static->itype);
          str8_list_pushf(arena, out, " mod_offset=%x\n", file_static->mod_offset);
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " flags=%x\n", file_static->flags);
          
          // name
          String8 name = str8_cstring_capped((char*)(file_static + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_SymKind_CALLEES:
      case CV_SymKind_CALLERS:
      {
        if (sizeof(CV_SymFunctionList) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymFunctionList *functions = (CV_SymFunctionList*)first;
          
          // count
          str8_list_pushf(arena, out, " count=%u\n", functions->count);
          
          // functions
          U32 function_count_max = (cap - sizeof(*functions))/sizeof(CV_TypeId);
          U32 function_count = ClampTop(functions->count, function_count_max);
          if (function_count > 0){
            str8_list_push(arena, out, str8_lit(" functions=\n"));
            CV_TypeId *func = (CV_TypeId*)(functions + 1);
            CV_TypeId *opl = func + function_count;
            for (;func < opl; func += 1){
              str8_list_pushf(arena, out, "  %u\n", *func);
            }
          }
          
          // invocations
          U32 invocation_count_max = (cap - sizeof(*functions) - function_count*sizeof(CV_TypeId))/sizeof(U32);
          U32 invocation_count = ClampTop(functions->count, invocation_count_max);
          if (invocation_count > 0){
            str8_list_push(arena, out, str8_lit(" invocations=\n"));
            U32 *inv = (CV_TypeId*)(functions + 1);
            U32 *opl = inv + invocation_count;
            for (;inv < opl; inv += 1){
              str8_list_pushf(arena, out, "  %u\n", *inv);
            }
          }
        }
      }break;
      
      case CV_SymKind_HEAPALLOCSITE:
      {
        if (sizeof(CV_SymHeapAllocSite) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymHeapAllocSite *heap_alloc_site = (CV_SymHeapAllocSite*)first;
          
          // members
          str8_list_pushf(arena, out, " off=%x\n", heap_alloc_site->off);
          str8_list_pushf(arena, out, " sec=%u\n", heap_alloc_site->sec);
          str8_list_pushf(arena, out, " call_inst_len=%u\n", heap_alloc_site->call_inst_len);
          str8_list_pushf(arena, out, " itype=%u\n", heap_alloc_site->itype);
        }
      }break;
      
      case CV_SymKind_INLINEES:
      {
        if (sizeof(CV_SymInlinees) > cap){
          str8_list_push(arena, out, str8_lit(" bad symbol range\n"));
        }
        else{
          CV_SymInlinees *inlinees = (CV_SymInlinees*)first;
          
          // count
          str8_list_pushf(arena, out, " count=%u\n", inlinees->count);
          
          // desc
          U32 desc_count = (cap - sizeof(*inlinees))/sizeof(U32);
          if (desc_count > 0){
            str8_list_pushf(arena, out, " desc=\n");
            U32 *desc = (U32*)(inlinees + 1);
            U32 *desc_opl = desc + desc_count;
            for (;desc < desc_opl; desc += 1){
              str8_list_pushf(arena, out, "  %u\n", *desc);
            }
          }
        }
      }break;
      
      case CV_SymKind_REGISTER_16t:
      case CV_SymKind_CONSTANT_16t:
      case CV_SymKind_UDT_16t:
      case CV_SymKind_SSEARCH:
      case CV_SymKind_SKIP:
      case CV_SymKind_CVRESERVE:
      case CV_SymKind_OBJNAME_ST:
      case CV_SymKind_ENDARG:
      case CV_SymKind_COBOLUDT_16t:
      case CV_SymKind_MANYREG_16t:
      case CV_SymKind_RETURN:
      case CV_SymKind_ENTRYTHIS:
      case CV_SymKind_BPREL16:
      case CV_SymKind_LDATA16:
      case CV_SymKind_GDATA16:
      case CV_SymKind_PUB16:
      case CV_SymKind_LPROC16:
      case CV_SymKind_GPROC16:
      case CV_SymKind_THUNK16:
      case CV_SymKind_BLOCK16:
      case CV_SymKind_WITH16:
      case CV_SymKind_LABEL16:
      case CV_SymKind_CEXMODEL16:
      case CV_SymKind_VFTABLE16:
      case CV_SymKind_REGREL16:
      case CV_SymKind_BPREL32_16t:
      case CV_SymKind_LDATA32_16t:
      case CV_SymKind_GDATA32_16t:
      case CV_SymKind_PUB32_16t:
      case CV_SymKind_LPROC32_16t:
      case CV_SymKind_GPROC32_16t:
      case CV_SymKind_THUNK32_ST:
      case CV_SymKind_BLOCK32_ST:
      case CV_SymKind_WITH32_ST:
      case CV_SymKind_LABEL32_ST:
      case CV_SymKind_CEXMODEL32:
      case CV_SymKind_VFTABLE32_16t:
      case CV_SymKind_REGREL32_16t:
      case CV_SymKind_LTHREAD32_16t:
      case CV_SymKind_GTHREAD32_16t:
      case CV_SymKind_SLINK32:
      case CV_SymKind_LPROCMIPS_16t:
      case CV_SymKind_GPROCMIPS_16t:
      case CV_SymKind_PROCREF_ST:
      case CV_SymKind_DATAREF_ST:
      case CV_SymKind_ALIGN:
      case CV_SymKind_LPROCREF_ST:
      case CV_SymKind_OEM:
      case CV_SymKind_TI16_MAX:
      case CV_SymKind_CONSTANT_ST:
      case CV_SymKind_UDT_ST:
      case CV_SymKind_COBOLUDT_ST:
      case CV_SymKind_MANYREG_ST:
      case CV_SymKind_BPREL32_ST:
      case CV_SymKind_LDATA32_ST:
      case CV_SymKind_GDATA32_ST:
      case CV_SymKind_PUB32_ST:
      case CV_SymKind_LPROC32_ST:
      case CV_SymKind_GPROC32_ST:
      case CV_SymKind_VFTABLE32:
      case CV_SymKind_REGREL32_ST:
      case CV_SymKind_LTHREAD32_ST:
      case CV_SymKind_GTHREAD32_ST:
      case CV_SymKind_LPROCMIPS_ST:
      case CV_SymKind_GPROCMIPS_ST:
      case CV_SymKind_COMPILE2_ST:
      case CV_SymKind_MANYREG2_ST:
      case CV_SymKind_LPROCIA64_ST:
      case CV_SymKind_GPROCIA64_ST:
      case CV_SymKind_LOCALSLOT_ST:
      case CV_SymKind_PARAMSLOT_ST:
      case CV_SymKind_ANNOTATION:
      case CV_SymKind_GMANPROC_ST:
      case CV_SymKind_LMANPROC_ST:
      case CV_SymKind_RESERVED1:
      case CV_SymKind_RESERVED2:
      case CV_SymKind_RESERVED3:
      case CV_SymKind_RESERVED4:
      case CV_SymKind_LMANDATA_ST:
      case CV_SymKind_GMANDATA_ST:
      case CV_SymKind_MANFRAMEREL_ST:
      case CV_SymKind_MANREGISTER_ST:
      case CV_SymKind_MANSLOT_ST:
      case CV_SymKind_MANMANYREG_ST:
      case CV_SymKind_MANREGREL_ST:
      case CV_SymKind_MANMANYREG2_ST:
      case CV_SymKind_MANTYPREF:
      case CV_SymKind_UNAMESPACE_ST:
      case CV_SymKind_ST_MAX:
      case CV_SymKind_WITH32:
      case CV_SymKind_REGISTER:
      case CV_SymKind_COBOLUDT:
      case CV_SymKind_MANYREG:
      case CV_SymKind_BPREL32:
      case CV_SymKind_LPROCMIPS:
      case CV_SymKind_GPROCMIPS:
      case CV_SymKind_MANYREG2:
      case CV_SymKind_LPROCIA64:
      case CV_SymKind_GPROCIA64:
      case CV_SymKind_LOCALSLOT:
      case CV_SymKind_PARAMSLOT:
      case CV_SymKind_LMANDATA:
      case CV_SymKind_GMANDATA:
      case CV_SymKind_MANFRAMEREL:
      case CV_SymKind_MANREGISTER:
      case CV_SymKind_MANSLOT:
      case CV_SymKind_MANMANYREG:
      case CV_SymKind_MANREGREL:
      case CV_SymKind_MANMANYREG2:
      case CV_SymKind_ANNOTATIONREF:
      case CV_SymKind_TOKENREF:
      case CV_SymKind_GMANPROC:
      case CV_SymKind_LMANPROC:
      case CV_SymKind_MANCONSTANT:
      case CV_SymKind_ATTR_FRAMEREL:
      case CV_SymKind_ATTR_REGISTER:
      case CV_SymKind_ATTR_REGREL:
      case CV_SymKind_ATTR_MANYREG:
      case CV_SymKind_SEPCODE:
      case CV_SymKind_DEFRANGE_2005:
      case CV_SymKind_DEFRANGE2_2005:
      case CV_SymKind_EXPORT:
      case CV_SymKind_DISCARDED:
      case CV_SymKind_DEFRANGE:
      case CV_SymKind_DEFRANGE_SUBFIELD:
      case CV_SymKind_LPROC32_ID:
      case CV_SymKind_GPROC32_ID:
      case CV_SymKind_LPROCMIPS_ID:
      case CV_SymKind_GPROCMIPS_ID:
      case CV_SymKind_LPROCIA64_ID:
      case CV_SymKind_GPROCIA64_ID:
      case CV_SymKind_PROC_ID_END:
      case CV_SymKind_DEFRANGE_HLSL:
      case CV_SymKind_GDATA_HLSL:
      case CV_SymKind_LDATA_HLSL:
      case CV_SymKind_LPROC32_DPC:
      case CV_SymKind_LPROC32_DPC_ID:
      case CV_SymKind_DEFRANGE_DPC_PTR_TAG:
      case CV_SymKind_DPC_SYM_TAG_MAP:
      case CV_SymKind_ARMSWITCHTABLE:
      case CV_SymKind_POGODATA:
      case CV_SymKind_INLINESITE2:
      case CV_SymKind_MOD_TYPEREF:
      case CV_SymKind_REF_MINIPDB:
      case CV_SymKind_PDBMAP:
      case CV_SymKind_GDATA_HLSL32:
      case CV_SymKind_LDATA_HLSL32:
      case CV_SymKind_GDATA_HLSL32_EX:
      case CV_SymKind_LDATA_HLSL32_EX:
      case CV_SymKind_FASTLINK:
      {
        str8_list_push(arena, out, str8_lit(" no stringizer path\n"));
      }break;
    }
  }
}

internal void
cv_stringize_sym_array(Arena *arena, String8List *out,
                       CV_RecRangeArray *ranges, String8 data,
                       CV_StringizeSymParams *p){
  CV_RecRange *ptr = ranges->ranges;
  CV_RecRange *opl = ranges->ranges + ranges->count;
  for (;ptr < opl; ptr += 1){
    cv_stringize_sym_range(arena, out, ptr, data, p);
    str8_list_push(arena, out, str8_lit("\n"));
  }
}

////////////////////////////////
//~ CodeView Leaf Stringize Functions

internal void
cv_stringize_leaf_parsed(Arena *arena, String8List *out, CV_LeafParsed *leaf){
  CV_StringizeLeafParams params = {0};
  
  cv_stringize_leaf_array(arena, out, &leaf->leaf_ranges, leaf->itype_first,
                          leaf->data, &params);
}

internal void
cv_stringize_leaf_range(Arena *arena, String8List *out,
                        CV_RecRange *range, CV_TypeId itype, String8 data,
                        CV_StringizeLeafParams *p){
  U64 opl_off = range->off + range->hdr.size;
  if (opl_off > data.size){
    str8_list_push(arena, out, str8_lit("bad leaf range\n"));
  }
  
  if (opl_off <= data.size){
    // [off] (itype): kind
    {
      String8 kind_str = cv_string_from_leaf_kind(range->hdr.kind);
      str8_list_pushf(arena, out, "[%06x] (%u): %.*s\n",
                      range->off + 2, itype, str8_varg(kind_str));
    }
    
    // details
    U8 *first = data.str + range->off + 2;
    U64 cap = range->hdr.size - 2;
    
    switch (range->hdr.kind){
      case CV_LeafKind_VTSHAPE:
      {
        if (sizeof(CV_LeafVTShape) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafVTShape *vtshape = (CV_LeafVTShape*)first;
          
          str8_list_pushf(arena, out, " count=%u\n", vtshape->count);
          
          str8_list_push(arena, out, str8_lit(" shapes=\n"));
          U8 *shapes = (U8*)(vtshape + 1);
          U32 max_count = (cap - sizeof(*vtshape))*2;
          U32 clamped_count = ClampTop(vtshape->count, max_count);
          for (U32 i = 0; i < clamped_count; i += 1){
            U32 j = (i >> 1);
            U8 s = shapes[j];
            if (j & 1){
              s >>= 4;
            }
            CV_VirtualTableShape shape = (s & 0xF);
            // TODO(allen): better shape path
            str8_list_pushf(arena, out, "  %u\n", shape);
          }
        }
      }break;
      
      case CV_LeafKind_LABEL:
      {
        if (sizeof(CV_LeafLabel) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafLabel *label = (CV_LeafLabel*)first;
          
          // TODO(allen): better LabelKind path
          str8_list_pushf(arena, out, " kind=%x\n", label->kind);
        }
      }break;
      
      case CV_LeafKind_MODIFIER:
      {
        if (sizeof(CV_LeafModifier) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafModifier *modifier = (CV_LeafModifier*)first;
          
          str8_list_pushf(arena, out, " itype=%u\n", modifier->itype);
          str8_list_pushf(arena, out, " flags={\n");
          cv_stringize_modifier_flags(arena, out, 2, modifier->flags);
          str8_list_pushf(arena, out, " }\n");
        }
      }break;
      
      case CV_LeafKind_POINTER:
      {
        if (sizeof(CV_LeafPointer) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafPointer *pointer = (CV_LeafPointer*)first;
          
          str8_list_pushf(arena, out, " itype=%u\n", pointer->itype);
          str8_list_pushf(arena, out, " attribs={\n");
          cv_stringize_pointer_attribs(arena, out, 2, pointer->attribs);
          str8_list_pushf(arena, out, " }\n");
        }
      }break;
      
      case CV_LeafKind_PROCEDURE:
      {
        if (sizeof(CV_LeafProcedure) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafProcedure *procedure = (CV_LeafProcedure*)first;
          
          str8_list_pushf(arena, out, " ret_itype=%u\n", procedure->ret_itype);
          // TODO(allen): better CallKind path
          str8_list_pushf(arena, out, " call_kind=%u\n", procedure->call_kind);
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " attribs=%x\n", procedure->attribs);
          str8_list_pushf(arena, out, " arg_count=%u\n", procedure->arg_count);
          str8_list_pushf(arena, out, " arg_itype=%u\n", procedure->arg_itype);
        }
      }break;
      
      case CV_LeafKind_MFUNCTION:
      {
        if (sizeof(CV_LeafMFunction) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafMFunction *mfunction = (CV_LeafMFunction*)first;
          
          str8_list_pushf(arena, out, " ret_itype=%u\n", mfunction->ret_itype);
          str8_list_pushf(arena, out, " class_itype=%u\n", mfunction->class_itype);
          str8_list_pushf(arena, out, " this_itype=%u\n", mfunction->this_itype);
          // TODO(allen): better CallKind path
          str8_list_pushf(arena, out, " call_kind=%u\n", mfunction->call_kind);
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " attribs=%x\n", mfunction->attribs);
          str8_list_pushf(arena, out, " arg_count=%u\n", mfunction->arg_count);
          str8_list_pushf(arena, out, " arg_itype=%u\n", mfunction->arg_itype);
          str8_list_pushf(arena, out, " this_adjust=%d\n", mfunction->this_adjust);
        }
      }break;
      
      case CV_LeafKind_ARGLIST:
      {
        if (sizeof(CV_LeafArgList) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafArgList *arg_list = (CV_LeafArgList*)first;
          
          str8_list_pushf(arena, out, " count=%u\n", arg_list->count);
          str8_list_push(arena, out, str8_lit(" itypes=\n"));
          
          CV_TypeId *itypes = (CV_TypeId*)(arg_list + 1);
          U32 max_count = (cap - sizeof(*arg_list))/sizeof(U32);
          U32 clamped_count = ClampTop(arg_list->count, max_count);
          for (U32 i = 0; i < clamped_count; i += 1){
            str8_list_pushf(arena, out, "  %u\n", itypes[i]);
          }
        }
      }break;
      
      case CV_LeafKind_FIELDLIST:
      {
        U64 cursor = 0;
        for (;cursor + sizeof(CV_LeafKind) <= cap;){
          CV_LeafKind field_kind = *(CV_LeafKind*)(first + cursor);
          String8 field_kind_str = cv_string_from_leaf_kind(field_kind);
          
          str8_list_pushf(arena, out, " field kind: %.*s\n",
                          str8_varg(field_kind_str));
          
          U64 list_item_off = cursor + 2;
          
          // if we hit an error or forget to set next cursor for a case
          // default to exiting the loop
          U64 list_item_opl_off = cap;
          
          switch (field_kind){
            default:
            {
              str8_list_push(arena, out, str8_lit("  unexpected field kind\n"));
            }break;
            
            case CV_LeafKind_MEMBER:
            {
              if (list_item_off + sizeof(CV_LeafMember) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafMember *member = (CV_LeafMember*)(first + list_item_off);
                
                U64 num_off = list_item_off + sizeof(*member);
                CV_NumericParsed num = cv_numeric_from_data_range(first + num_off, first + cap);
                
                U64 name_off = num_off + num.encoded_size;
                String8 name = str8_cstring_capped(first + name_off, first + cap);
                
                list_item_opl_off = name_off + name.size + 1;
                
                // print data
                // TODO(allen): better flags path
                str8_list_pushf(arena, out, "  attribs=%x\n", member->attribs);
                str8_list_pushf(arena, out, "  itype=%u\n", member->itype);
                str8_list_push(arena, out, str8_lit("  offset="));
                cv_stringize_numeric(arena, out, &num);
                str8_list_push(arena, out, str8_lit("\n"));
                str8_list_pushf(arena, out, "  name='%.*s'\n", str8_varg(name));
              }
            }break;
            
            case CV_LeafKind_STMEMBER:
            {
              if (list_item_off + sizeof(CV_LeafStMember) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafStMember *stmember = (CV_LeafStMember*)(first + list_item_off);
                
                U64 name_off = list_item_off + sizeof(*stmember);
                String8 name = str8_cstring_capped(first + name_off, first + cap);
                
                list_item_opl_off = name_off + name.size + 1;
                
                // print data
                // TODO(allen): better flags path
                str8_list_pushf(arena, out, "  attribs=%x\n", stmember->attribs);
                str8_list_pushf(arena, out, "  itype=%u\n", stmember->itype);
                str8_list_pushf(arena, out, "  name='%.*s'\n", str8_varg(name));
              }
            }break;
            
            case CV_LeafKind_METHOD:
            {
              if (list_item_off + sizeof(CV_LeafMethod) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafMethod *method = (CV_LeafMethod*)(first + list_item_off);
                
                U64 name_off = list_item_off + sizeof(*method);
                String8 name = str8_cstring_capped(first + name_off, first + cap);
                
                list_item_opl_off = name_off + name.size + 1;
                
                // print data
                str8_list_pushf(arena, out, "  count=%u\n", method->count);
                str8_list_pushf(arena, out, "  list_itype=%u\n", method->list_itype);
                str8_list_pushf(arena, out, "  name='%.*s'\n", str8_varg(name));
              }
            }break;
            
            case CV_LeafKind_ONEMETHOD:
            {
              if (list_item_off + sizeof(CV_LeafOneMethod) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafOneMethod *one_method = (CV_LeafOneMethod*)(first + list_item_off);
                
                U64 vbaseoff_off = list_item_off + sizeof(*one_method);
                U64 vbaseoff_opl_off = vbaseoff_off;
                U32 vbaseoff = 0;
                {
                  CV_MethodProp prop = CV_FieldAttribs_ExtractMethodProp(one_method->attribs);
                  if (prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro){
                    vbaseoff = *(U32*)(first + vbaseoff_off);
                    vbaseoff_opl_off += sizeof(vbaseoff);
                  }
                }
                
                U64 name_off = vbaseoff_opl_off;
                String8 name = str8_cstring_capped(first + name_off, first + cap);
                
                list_item_opl_off = name_off + name.size + 1;
                
                // print data
                // TODO(allen): better flags path
                str8_list_pushf(arena, out, "  attribs=%x\n", one_method->attribs);
                str8_list_pushf(arena, out, "  itype=%u\n", one_method->itype);
                str8_list_pushf(arena, out, "  vbaseoff=%u\n", vbaseoff);
                str8_list_pushf(arena, out, "  name='%.*s'\n", str8_varg(name));
              }
            }break;
            
            case CV_LeafKind_ENUMERATE:
            {
              if (list_item_off + sizeof(CV_LeafEnumerate) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafEnumerate *enumerate = (CV_LeafEnumerate*)(first + list_item_off);
                
                U64 num_off = list_item_off + sizeof(*enumerate);
                CV_NumericParsed num = cv_numeric_from_data_range(first + num_off, first + cap);
                
                U64 name_off = num_off + num.encoded_size;
                String8 name = str8_cstring_capped(first + name_off, first + cap);
                
                list_item_opl_off = name_off + name.size + 1;
                
                // print data
                // TODO(allen): better flags path
                str8_list_pushf(arena, out, "  attribs=%x\n", enumerate->attribs);
                str8_list_push(arena, out, str8_lit("  val="));
                cv_stringize_numeric(arena, out, &num);
                str8_list_push(arena, out, str8_lit("\n"));
                str8_list_pushf(arena, out, "  name='%.*s'\n", str8_varg(name));
              }
            }break;
            
            case CV_LeafKind_NESTTYPE:
            {
              if (list_item_off + sizeof(CV_LeafNestType) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafNestType *nest_type = (CV_LeafNestType*)(first + list_item_off);
                
                U64 name_off = list_item_off + sizeof(*nest_type);
                String8 name = str8_cstring_capped(first + name_off, first + cap);
                
                list_item_opl_off = name_off + name.size + 1;
                
                // print data
                str8_list_pushf(arena, out, "  itype=%u\n", nest_type->itype);
                str8_list_pushf(arena, out, "  name='%.*s'\n", str8_varg(name));
              }
            }break;
            
            case CV_LeafKind_NESTTYPEEX:
            {
              if (list_item_off + sizeof(CV_LeafNestTypeEx) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafNestTypeEx *nest_type = (CV_LeafNestTypeEx*)(first + list_item_off);
                
                U64 name_off = list_item_off + sizeof(*nest_type);
                String8 name = str8_cstring_capped(first + name_off, first + cap);
                
                list_item_opl_off = name_off + name.size + 1;
                
                // print data
                // TODO(allen): better flags printing
                str8_list_pushf(arena, out, "  attribs=%x\n", nest_type->attribs);
                str8_list_pushf(arena, out, "  itype=%u\n", nest_type->itype);
                str8_list_pushf(arena, out, "  name='%.*s'\n", str8_varg(name));
              }
            }break;
            
            case CV_LeafKind_BCLASS:
            {
              if (list_item_off + sizeof(CV_LeafBClass) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafBClass *bclass = (CV_LeafBClass*)(first + list_item_off);
                
                U64 num_off = list_item_off + sizeof(*bclass);
                CV_NumericParsed num = cv_numeric_from_data_range(first + num_off, first + cap);
                
                list_item_opl_off = num_off + num.encoded_size;
                
                // print data
                // TODO(allen): better flags printing
                str8_list_pushf(arena, out, "  attribs=%x\n", bclass->attribs);
                str8_list_pushf(arena, out, "  itype=%u\n", bclass->itype);
                str8_list_push(arena, out, str8_lit("  offset="));
                cv_stringize_numeric(arena, out, &num);
                str8_list_push(arena, out, str8_lit("\n"));
              }
            }break;
            
            case CV_LeafKind_VBCLASS:
            case CV_LeafKind_IVBCLASS:
            {
              if (list_item_off + sizeof(CV_LeafVBClass) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafVBClass *vbclass = (CV_LeafVBClass*)(first + list_item_off);
                
                U64 num1_off = list_item_off + sizeof(*vbclass);
                CV_NumericParsed num1 = cv_numeric_from_data_range(first + num1_off, first + cap);
                
                U64 num2_off = num1_off + num1.encoded_size;
                CV_NumericParsed num2 = cv_numeric_from_data_range(first + num2_off, first + cap);
                
                list_item_opl_off = num2_off + num2.encoded_size;
                
                // print data
                // TODO(allen): better flags printing
                str8_list_pushf(arena, out, "  attribs=%x\n", vbclass->attribs);
                str8_list_pushf(arena, out, "  itype=%u\n", vbclass->itype);
                str8_list_pushf(arena, out, "  vbptr_itype=%u\n", vbclass->vbptr_itype);
                str8_list_push(arena, out, str8_lit("  vbptr_off="));
                cv_stringize_numeric(arena, out, &num1);
                str8_list_push(arena, out, str8_lit("\n"));
                str8_list_push(arena, out, str8_lit("  vtable_off="));
                cv_stringize_numeric(arena, out, &num2);
                str8_list_push(arena, out, str8_lit("\n"));
              }
            }break;
            
            case CV_LeafKind_INDEX:
            {
              if (list_item_off + sizeof(CV_LeafIndex) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafIndex *index = (CV_LeafIndex*)(first + list_item_off);
                
                list_item_opl_off = list_item_off + sizeof(*index);
                
                // print data
                str8_list_pushf(arena, out, "  itype=%u\n", index->itype);
              }
            }break;
            
            case CV_LeafKind_VFUNCTAB:
            {
              if (list_item_off + sizeof(CV_LeafVFuncTab) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafVFuncTab *vfunctab = (CV_LeafVFuncTab*)(first + list_item_off);
                
                list_item_opl_off = list_item_off + sizeof(*vfunctab);
                
                // print data
                str8_list_pushf(arena, out, "  itype=%u\n", vfunctab->itype);
              }
            }break;
            
            case CV_LeafKind_VFUNCOFF:
            {
              if (list_item_off + sizeof(CV_LeafVFuncOff) > cap){
                str8_list_push(arena, out, str8_lit("  bad field list range\n"));
              }
              else{
                // compute whole layout
                CV_LeafVFuncOff *vfuncoff = (CV_LeafVFuncOff*)(first + list_item_off);
                
                list_item_opl_off = list_item_off + sizeof(*vfuncoff);
                
                // print data
                str8_list_pushf(arena, out, "  itype=%u\n", vfuncoff->itype);
                str8_list_pushf(arena, out, "  off=%u\n", vfuncoff->off);
              }
            }break;
          }
          
          // update cursor
          U64 next_cursor = AlignPow2(list_item_opl_off, 4);
          cursor = next_cursor;
        }
      }break;
      
      case CV_LeafKind_BITFIELD:
      {
        if (sizeof(CV_LeafBitField) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafBitField *bit_field = (CV_LeafBitField*)first;
          
          str8_list_pushf(arena, out, " itype=%u\n", bit_field->itype);
          str8_list_pushf(arena, out, " len=%u\n", bit_field->len);
          str8_list_pushf(arena, out, " pos=%u\n", bit_field->pos);
        }
      }break;
      
      case CV_LeafKind_METHODLIST:
      {
        U64 cursor = 0;
        for (;cursor + sizeof(CV_LeafMethodListMember) <= cap;){
          CV_LeafMethodListMember *method = (CV_LeafMethodListMember*)(first + cursor);
          
          // extract vbaseoff
          U64 next_cursor = cursor + sizeof(*method);
          U32 vbaseoff = 0;
          {
            CV_MethodProp prop = CV_FieldAttribs_ExtractMethodProp(method->attribs);
            if (prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro){
              if (cursor + sizeof(*method) + 4 <= cap){
                vbaseoff = *(U32*)(method + 1);
              }
              next_cursor += 4;
            }
          }
          
          // print
          // TODO(allen): better flags path
          str8_list_pushf(arena, out, " method\n", method->attribs);
          str8_list_pushf(arena, out, "  attribs=%x\n", method->attribs);
          str8_list_pushf(arena, out, "  itype=%u\n", method->itype);
          str8_list_pushf(arena, out, "  vbaseoff=%u\n", vbaseoff);
          
          // update cursor
          cursor = next_cursor;
        }
      }break;
      
      case CV_LeafKind_ARRAY:
      {
        if (sizeof(CV_LeafArray) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafArray *array = (CV_LeafArray*)first;
          
          str8_list_pushf(arena, out, " entry_itype=%u\n", array->entry_itype);
          str8_list_pushf(arena, out, " index_itype=%u\n", array->index_itype);
          
          // count
          U8 *numeric_ptr = (U8*)(array + 1);
          CV_NumericParsed array_count = cv_numeric_from_data_range(numeric_ptr, first + cap);
          str8_list_pushf(arena, out, " count=");
          cv_stringize_numeric(arena, out, &array_count);
          str8_list_push(arena, out, str8_lit("\n"));
        }
      }break;
      
      case CV_LeafKind_CLASS:
      case CV_LeafKind_STRUCTURE:
      {
        if (sizeof(CV_LeafStruct) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafStruct *lf_struct = (CV_LeafStruct*)first;
          
          str8_list_pushf(arena, out, " count=%u\n", lf_struct->count);
          str8_list_pushf(arena, out, " props=%x (\n", lf_struct->props);
          cv_stringize_type_props(arena, out, 2, lf_struct->props);
          str8_list_pushf(arena, out, " )\n");
          str8_list_pushf(arena, out, " field_itype=%u\n", lf_struct->field_itype);
          str8_list_pushf(arena, out, " derived_itype=%u\n", lf_struct->derived_itype);
          str8_list_pushf(arena, out, " vshape_itype=%u\n", lf_struct->vshape_itype);
          
          U8 *numeric_ptr = (U8*)(lf_struct + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          str8_list_pushf(arena, out, " size=");
          cv_stringize_numeric(arena, out, &size);
          str8_list_push(arena, out, str8_lit("\n"));
          
          String8 name = str8_cstring_capped((U8*)(numeric_ptr + size.encoded_size), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
          
          String8 unique_name = str8_cstring_capped(name.str + name.size + 1, first + cap);
          str8_list_pushf(arena, out, " unique_name='%.*s'\n", str8_varg(unique_name));
        }
      }break;
      
      case CV_LeafKind_UNION:
      {
        if (sizeof(CV_LeafUnion) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafUnion *lf_union = (CV_LeafUnion*)first;
          
          str8_list_pushf(arena, out, " count=%u\n", lf_union->count);
          str8_list_pushf(arena, out, " props=%x (\n", lf_union->props);
          cv_stringize_type_props(arena, out, 2, lf_union->props);
          str8_list_pushf(arena, out, " )\n");
          str8_list_pushf(arena, out, " field_itype=%u\n", lf_union->field_itype);
          
          U8 *numeric_ptr = (U8*)(lf_union + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          str8_list_pushf(arena, out, " size=");
          cv_stringize_numeric(arena, out, &size);
          str8_list_push(arena, out, str8_lit("\n"));
          
          String8 name = str8_cstring_capped((U8*)(numeric_ptr + size.encoded_size), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
          
          String8 unique_name = str8_cstring_capped(name.str + name.size + 1, first + cap);
          str8_list_pushf(arena, out, " unique_name='%.*s'\n", str8_varg(unique_name));
        }
      }break;
      
      case CV_LeafKind_ENUM:
      {
        if (sizeof(CV_LeafEnum) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafEnum *lf_enum = (CV_LeafEnum*)first;
          
          str8_list_pushf(arena, out, " count=%u\n", lf_enum->count);
          str8_list_pushf(arena, out, " props=%x (\n", lf_enum->props);
          cv_stringize_type_props(arena, out, 2, lf_enum->props);
          str8_list_pushf(arena, out, " )\n");
          str8_list_pushf(arena, out, " base_itype=%u\n", lf_enum->base_itype);
          str8_list_pushf(arena, out, " field_itype=%u\n", lf_enum->field_itype);
          
          String8 name = str8_cstring_capped((U8*)(lf_enum + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
          
          String8 unique_name = str8_cstring_capped(name.str + name.size + 1, first + cap);
          str8_list_pushf(arena, out, " unique_name='%.*s'\n", str8_varg(unique_name));
        }
      }break;
      
      case CV_LeafKind_VFTABLE:
      {
        if (sizeof(CV_LeafVFTable) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafVFTable *vftable = (CV_LeafVFTable*)first;
          
          str8_list_pushf(arena, out, " owner_itype=%u\n", vftable->owner_itype);
          str8_list_pushf(arena, out, " base_table_itype=%u\n", vftable->base_table_itype);
          str8_list_pushf(arena, out, " offset_in_object_layout=%u\n",
                          vftable->offset_in_object_layout);
          str8_list_pushf(arena, out, " names_len=%u\n", vftable->names_len);
          
          U64 names_cap = Min(sizeof(*vftable) + vftable->names_len, cap);
          
          str8_list_push(arena, out, str8_lit(" names=\n"));
          U8 *ptr = (U8*)(vftable + 1);
          U8 *opl = first + names_cap;
          for (;ptr < opl;){
            String8 name = str8_cstring_capped(ptr, opl);
            str8_list_pushf(arena, out, "  '%.*s'\n", str8_varg(name));
            ptr += name.size + 1;
          }
        }
      }break;
      
      case CV_LeafKind_CLASS2:
      case CV_LeafKind_STRUCT2:
      {
        if (sizeof(CV_LeafStruct2) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafStruct2 *struct2 = (CV_LeafStruct2*)first;
          
          str8_list_pushf(arena, out, " props=%x (\n", struct2->props);
          cv_stringize_type_props(arena, out, 2, struct2->props);
          str8_list_pushf(arena, out, " )\n");
          str8_list_pushf(arena, out, " unknown1=%u\n", struct2->unknown1);
          str8_list_pushf(arena, out, " field_itype=%u\n", struct2->field_itype);
          str8_list_pushf(arena, out, " derived_itype=%u\n", struct2->derived_itype);
          str8_list_pushf(arena, out, " vshape_itype=%u\n", struct2->vshape_itype);
          str8_list_pushf(arena, out, " unknown2=0x%x\n", struct2->unknown2);
          //str8_list_pushf(arena, out, " size=%u\n", struct2->size);
          
          String8 name = str8_cstring_capped((U8*)(struct2 + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
          
          String8 unique_name = str8_cstring_capped(name.str + name.size + 1, first + cap);
          str8_list_pushf(arena, out, " unique_name='%.*s'\n", str8_varg(unique_name));
        }
      }break;
      
      case CV_LeafIDKind_FUNC_ID:
      {
        if (sizeof(CV_LeafFuncId) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafFuncId *func_id = (CV_LeafFuncId*)first;
          
          str8_list_pushf(arena, out, " scope_string_id=%u\n", func_id->scope_string_id);
          str8_list_pushf(arena, out, " itype=%u\n", func_id->itype);
          
          String8 name = str8_cstring_capped((U8*)(func_id + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_LeafIDKind_MFUNC_ID:
      {
        if (sizeof(CV_LeafMFuncId) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafMFuncId *mfunc_id = (CV_LeafMFuncId*)first;
          
          str8_list_pushf(arena, out, " owner_itype=%u\n", mfunc_id->owner_itype);
          str8_list_pushf(arena, out, " itype=%u\n", mfunc_id->itype);
          
          String8 name = str8_cstring_capped((U8*)(mfunc_id + 1), first + cap);
          str8_list_pushf(arena, out, " name='%.*s'\n", str8_varg(name));
        }
      }break;
      
      case CV_LeafIDKind_BUILDINFO:
      {
        if (sizeof(CV_LeafBuildInfo) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafBuildInfo *build_info = (CV_LeafBuildInfo*)first;
          
          str8_list_pushf(arena, out, " count=%u\n", build_info->count);
          
          CV_ItemId *item_ids = (CV_ItemId*)(build_info + 1);
          str8_list_pushf(arena, out, " items=\n");
          U32 max_count = (cap - sizeof(*build_info))/sizeof(CV_ItemId);
          U32 clamped_count = ClampTop(build_info->count, max_count);
          for (U32 i = 0; i < clamped_count; i += 1){
            CV_ItemId item_id = item_ids[i];
            str8_list_pushf(arena, out, "  %u\n", item_id);
          }
        }
      }break;
      
      case CV_LeafIDKind_SUBSTR_LIST:
      {
        if (sizeof(CV_LeafSubstrList) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafSubstrList *substr_list = (CV_LeafSubstrList*)first;
          
          str8_list_pushf(arena, out, " count=%u\n", substr_list->count);
          
          str8_list_pushf(arena, out, " items=\n");
          U32 max_count = (cap - sizeof(CV_LeafSubstrList))/sizeof(CV_ItemId);
          CV_ItemId *item_ids = (CV_ItemId*)(substr_list + 1);
          U32 clamped_count = ClampTop(substr_list->count, max_count);
          for (U32 i = 0; i < clamped_count; i += 1){
            CV_ItemId item_id = item_ids[i];
            str8_list_pushf(arena, out, "  %u\n", item_id);
          }
        }
      }break;
      
      case CV_LeafIDKind_STRING_ID:
      {
        if (sizeof(CV_LeafStringId) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafStringId *string_id = (CV_LeafStringId*)first;
          
          str8_list_pushf(arena, out, " substr_list_id=%u\n", string_id->substr_list_id);
          
          String8 string = str8_cstring_capped((U8*)(string_id + 1), first + cap);
          str8_list_pushf(arena, out, " string='%.*s'\n", str8_varg(string));
        }
      }break;
      
      case CV_LeafIDKind_UDT_SRC_LINE:
      {
        if (sizeof(CV_LeafUDTSrcLine) > cap){
          str8_list_push(arena, out, str8_lit(" bad leaf range\n"));
        }
        else{
          CV_LeafUDTSrcLine *udt_src_line = (CV_LeafUDTSrcLine*)first;
          
          str8_list_pushf(arena, out, " udt_itype=%u\n", udt_src_line->udt_itype);
          str8_list_pushf(arena, out, " src_string_id=%u\n", udt_src_line->src_string_id);
          str8_list_pushf(arena, out, " line=%u\n", udt_src_line->line);
        }
      }break;
      
      
      default:
      
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
      //case CV_LeafKind_INDEX:
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
      
      // Leaf ID Kinds
      //case CV_LeafIDKind_FUNC_ID:
      //case CV_LeafIDKind_MFUNC_ID:
      //case CV_LeafIDKind_BUILDINFO:
      //case CV_LeafIDKind_SUBSTR_LIST:
      //case CV_LeafIDKind_STRING_ID:
      //case CV_LeafIDKind_UDT_SRC_LINE:
      case CV_LeafIDKind_UDT_MOD_SRC_LINE:
      
      {
        str8_list_push(arena, out, str8_lit(" no stringizer path\n"));
      }break;
    }
  }
}

internal void
cv_stringize_leaf_array(Arena *arena, String8List *out,
                        CV_RecRangeArray *ranges, CV_TypeId itype_first, String8 data,
                        CV_StringizeLeafParams *p){
  CV_RecRange *ptr = ranges->ranges;
  CV_RecRange *opl = ranges->ranges + ranges->count;
  CV_TypeId itype = itype_first;
  for (;ptr < opl; ptr += 1, itype += 1){
    cv_stringize_leaf_range(arena, out, ptr, itype, data, p);
    str8_list_push(arena, out, str8_lit("\n"));
  }
}

////////////////////////////////
//~ CodeView C13 Stringize Functions

internal void
cv_stringize_c13_parsed(Arena *arena, String8List *out, CV_C13Parsed *c13){
  for(CV_C13SubSectionNode *node = c13->first_sub_section;
      node != 0;
      node = node->next)
  {
    String8 kind_str = cv_string_from_c13_sub_section_kind(node->kind);
    str8_list_pushf(arena, out, "C13 Sub Section [%llx] (%.*s):\n",
                    node->off, str8_varg(kind_str));
    
    switch(node->kind)
    {
      case CV_C13SubSectionKind_Lines:
      {
        if (node->lines_first == 0)
        {
          str8_list_push(arena, out, str8_lit(" failed to extract info\n"));
        }
        else for(CV_C13LinesParsedNode *n = node->lines_first; n != 0; n = n->next)
        {
          CV_C13LinesParsed *lines = &n->v;
          
          str8_list_pushf(arena, out, " section:    %u\n", lines->sec_idx);
          str8_list_pushf(arena, out, " file off:   %u\n", lines->file_off);
          str8_list_pushf(arena, out, " file name:  %.*s\n", str8_varg(lines->file_name));
          str8_list_pushf(arena, out, " line count: %u\n", lines->line_count);
          
          U64 base_off = lines->secrel_base_off;
          U64 *line_offs = lines->voffs;
          U32 *line_nums = lines->line_nums;
          
          U32 line_count = lines->line_count;
          for (U32 i = 0; i < line_count; i += 1){
            str8_list_pushf(arena, out, "  {secrel_off=%llx, line_num=%u}\n",
                            line_offs[i], line_nums[i]);
          }
          
          str8_list_pushf(arena, out, "  {secrel_off=%x, ender}\n", line_offs[line_count]);
        }
      }break;
      
      case CV_C13SubSectionKind_FileChksms:
      {
        str8_list_push(arena, out, str8_lit(" no stringizer path\n"));
      }break;
      
      case CV_C13SubSectionKind_InlineeLines:
      {
        str8_list_push(arena, out, str8_lit(" no stringizer path\n"));
      }break;
      
      default:
      {
        str8_list_push(arena, out, str8_lit(" no stringizer path\n"));
      }break;
    }
    
    str8_list_push(arena, out, str8_lit("\n"));
  }
}
