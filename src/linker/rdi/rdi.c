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

