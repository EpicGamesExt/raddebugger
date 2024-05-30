RDI_PROC char *
rdi_cstring_from_name_map_kind(RDI_NameMapKind kind)
{
  switch(kind)
  {
  case RDI_NameMapKind_NULL:                return "NULL";
  case RDI_NameMapKind_GlobalVariables:     return "GlobalVariables";
  case RDI_NameMapKind_ThreadVariables:     return "ThreadVariables";
  case RDI_NameMapKind_Procedures:          return "Procedures";
  case RDI_NameMapKind_Types:               return "Types";
  case RDI_NameMapKind_LinkNameProcedures:  return "LinkNameProcedures";
  case RDI_NameMapKind_NormalSourcePaths:   return "NormalSourcePaths";
  }
  return "";
}
