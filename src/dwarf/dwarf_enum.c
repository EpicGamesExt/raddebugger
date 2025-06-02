// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
dw_string_from_expr_op(Arena *arena, DW_Version ver, DW_Ext ext, DW_ExprOp op)
{
  String8 result = {0};
  
#define X(_N,...) case DW_ExprOp_##_N: result = str8_lit(Stringify(_N)); goto exit;
  if (ext & DW_Ext_GNU) {
    switch (op) {
      DW_Expr_GNU_XList(X); 
    }
  }
  
  switch (ver) {
    case DW_Version_5: {
      switch (op) {
        DW_Expr_V5_XList(X)
      }
    } // fall-through
    case DW_Version_4: {
      switch (op) {
        DW_Expr_V4_XList(X)
      }
    } // fall-through
    case DW_Version_3: {
      switch (op) {
        DW_Expr_V3_XList(X)
      }
    } // fall-through
    case DW_Version_2:
    case DW_Version_1:
    case DW_Version_Null:
    break;
  }
#undef X
  
  result = push_str8f(arena, "%x", op);
  
  exit:;
  return result;
}

internal String8
dw_string_from_tag_kind(Arena *arena, DW_TagKind kind)
{
  switch (kind) {
    case DW_Tag_Null: return str8_lit("Null");
#define X(_N,_ID) case DW_Tag_##_N: return str8_lit(Stringify(_N));
    DW_Tag_V3_XList(X)
      DW_Tag_V5_XList(X)
      DW_Tag_GNU_XList(X)
#undef X
  }
  return push_str8f(arena, "%llx", kind);
}

internal String8
dw_string_from_attrib_kind(Arena *arena, DW_Version ver, DW_Ext ext, DW_AttribKind kind)
{
#define X(_N,...) case DW_Attrib_##_N: return str8_lit(Stringify(_N));
  
  while (ext) {
    U64 z = 64-clz64(ext);
    if (z == 0) {
      break;
    }
    U64 flag = 1 << (z-1);
    ext &= ~flag;
    
    switch (flag) {
      case DW_Ext_Null: break;
      case DW_Ext_GNU:   switch (kind) { DW_AttribKind_GNU_XList(X)   } break;
      case DW_Ext_LLVM:  switch (kind) { DW_AttribKind_LLVM_XList(X)  } break;
      case DW_Ext_APPLE: switch (kind) { DW_AttribKind_APPLE_XList(X) } break;
      case DW_Ext_MIPS:  switch (kind) { DW_AttribKind_MIPS_XList(X)  } break;
      default: InvalidPath; break;
    }
  }
  
  switch (ver) {
    case DW_Version_5: {
      switch (kind) {
        DW_AttribKind_V5_XList(X)
      }
    } // fall-through
    case DW_Version_4: {
      switch (kind) {
        DW_AttribKind_V4_XList(X)
      }
    } // fall-through
    case DW_Version_3: {
      switch (kind) {
        DW_AttribKind_V3_XList(X)
      }
    } // fall-through
    case DW_Version_2: {
      switch (kind) {
        DW_AttribKind_V2_XList(X)
      }
    } // fall-through
    case DW_Version_1: {
    } // fall-through
    case DW_Version_Null: break;
  }
#undef X
  
  return str8_zero();
}

internal String8
dw_string_from_form_kind(Arena *arena, DW_Version ver, DW_FormKind kind)
{
#define X(_N,...) case DW_Form_##_N: return str8_lit(Stringify(_N));
  switch (ver) {
    case DW_Version_5: {
      switch (kind) {
        DW_Form_V5_XList(X)
      }
    } // fall-through
    case DW_Version_4: {
      switch (kind) {
        DW_Form_V4_XList(X)
      }
    } // fall-through
    case DW_Version_3: 
    case DW_Version_2: {
      switch (kind) {
        DW_Form_V2_XList(X)
      }
    } // fall-through
    case DW_Version_Null: break;
  }
#undef X
  String8 result = push_str8f(arena, "%x", kind);
  return result;
}

internal String8
dw_string_from_language(Arena *arena, DW_Language kind)
{
  switch (kind) {
#define X(_N,_ID) case DW_Language_##_N: return str8_lit(Stringify(_N));
    DW_Language_XList(X)
#undef X
  }
  return push_str8f(arena, "%x", kind);
}

internal String8
dw_string_from_inl(Arena *arena, DW_InlKind kind)
{
  switch (kind) {
#define X(_N,_ID) case _ID: return str8_lit(Stringify(_N));
    DW_Inl_XList(X)
#undef X
  }
  return push_str8f(arena, "%x", kind);
}

internal String8
dw_string_from_access_kind(Arena *arena, DW_AccessKind kind)
{
  switch (kind) {
#define X(_N,_ID) case _ID: return str8_lit(Stringify(_N));
    DW_AccessKind_XList(X)
#undef X
  }
  return push_str8f(arena, "%llx", kind);
}

internal String8
dw_string_from_calling_convetion(Arena *arena, DW_CallingConventionKind kind)
{
  switch (kind) {
#define X(_N,_ID) case _ID: return str8_lit(Stringify(_N));
    DW_CallingConventionKind_XList(X)
#undef X
  }
  return push_str8f(arena, "%llx", kind);
}

internal String8
dw_string_from_attrib_type_encoding(Arena *arena, DW_ATE kind)
{
  switch (kind) {
#define X(_N,_ID) case _ID: return str8_lit(Stringify(_N));
    DW_ATE_XList(X)
#undef X
  }
  return push_str8f(arena, "%llx", kind);
}

internal String8
dw_string_from_std_opcode(Arena *arena, DW_StdOpcode kind)
{
  switch (kind) {
#define X(_N,_ID) case DW_StdOpcode_##_N: return str8_lit(Stringify(_N));
    DW_StdOpcode_XList(X)
#undef X
  }
  return push_str8f(arena, "%x", kind);
}

internal String8
dw_string_from_ext_opcode(Arena *arena, DW_ExtOpcode kind)
{
  switch (kind) {
#define X(_N,_ID) case DW_ExtOpcode_##_N: return str8_lit(Stringify(_N));
    DW_ExtOpcode_XList(X)
#undef X
    default: InvalidPath; break;
  }
  return push_str8f(arena, "%x", kind);
}

internal String8
dw_string_from_loc_list_entry_kind(Arena *arena, DW_LLE kind)
{
  NotImplemented;
  return str8_zero();
}

internal String8
dw_string_from_section_kind(Arena *arena, DW_SectionKind kind)
{
  NotImplemented;
  return str8_zero();
}

internal String8
dw_string_from_rng_list_entry_kind(Arena *arena, DW_RLE kind)
{
  NotImplemented;
  return str8_zero();
}

internal String8
dw_string_from_register(Arena *arena, Arch arch, U64 reg_id)
{
  String8 reg_str = str8_zero();
  switch (arch) {
    case Arch_Null: break;
    case Arch_x86: {
      switch (reg_id) {
#define X(_N, _ID, ...) case DW_RegX86_##_N: reg_str = str8_lit(Stringify(_N)); break;
        DW_Regs_X86_XList(X)
#undef X
      }
    } break;
    case Arch_x64: {
      switch (reg_id) {
#define X(_N, _ID, ...) case DW_RegX64_##_N: reg_str = str8_lit(Stringify(_N)); break;
        DW_Regs_X64_XList(X)
#undef X
      }
    } break;
    case Arch_arm32: NotImplemented; break;
    case Arch_arm64: NotImplemented; break;
    default: InvalidPath; break;
  }
  if (reg_str.size == 0) {
    reg_str = push_str8f(arena, "%#llx", reg_id);
  }
  return reg_str;
}

