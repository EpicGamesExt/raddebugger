internal String8
rdi_string_from_name_map_kind(RDI_NameMapKind kind)
{
  switch (kind) {
  case RDI_NameMapKind_NULL              : return str8_lit("NULL");
  case RDI_NameMapKind_GlobalVariables   : return str8_lit("GlobalVariables");
  case RDI_NameMapKind_ThreadVariables   : return str8_lit("ThreadVariables");
  case RDI_NameMapKind_Procedures        : return str8_lit("Procedures");
  case RDI_NameMapKind_Types             : return str8_lit("Types");
  case RDI_NameMapKind_LinkNameProcedures: return str8_lit("LinkNameProcedures");
  case RDI_NameMapKind_NormalSourcePaths : return str8_lit("NormalSourcePaths");
  }
  return str8_lit("");
}

