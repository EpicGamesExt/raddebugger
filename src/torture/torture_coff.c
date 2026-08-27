// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal MD_Node *
t_coff_field(T_ParseContext *ctx, MD_Node *parent, char *name)
{
  MD_Node *result = &md_nil_node;
  U64 count = 0;
  for
    MD_EachNode(child, parent->first)
    {
      if (str8_match(child->string, str8_cstring(name), StringMatchFlag_CaseInsensitive)) {
        if (count == 0) { result = child; }
        count += 1;
      }
    }
  if (count > 1) { t_parse_errorf(ctx, T_ResultCode_ValidationError, result, "field '%s' may only appear once", name); }
  return result;
}

internal B32
t_coff_name_is_allowed(String8 name, char **allowed)
{
  for (char **item = allowed; *item != 0; item += 1) {
    if (str8_match(name, str8_cstring(*item), StringMatchFlag_CaseInsensitive)) { return 1; }
  }
  return 0;
}

internal void
t_coff_reject_unknown(T_ParseContext *ctx, MD_Node *node, char **allowed)
{
  for
    MD_EachNode(child, node->first)
    {
      if (!t_coff_name_is_allowed(child->string, allowed)) { t_parse_errorf(ctx, T_ResultCode_ValidationError, child, "unknown field '%S'", child->string); }
    }
}

internal B32
t_coff_scalar(T_ParseContext *ctx, MD_Node *parent, char *name, B32 required, String8 *value_out)
{
  MD_Node *field = t_coff_field(ctx, parent, name);
  if (md_node_is_nil(field)) {
    if (required) { t_parse_errorf(ctx, T_ResultCode_ValidationError, parent, "missing field '%s'", name); }
    return !required;
  }
  if (md_node_is_nil(field->first) || !md_node_is_nil(field->first->next)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, field, "field '%s' requires exactly one value", name);
    return 0;
  }
  *value_out = field->first->string;
  return 1;
}

internal B32
t_coff_u64(T_ParseContext *ctx, MD_Node *parent, char *name, B32 required, U64 default_value, U64 max_value, U64 *value_out)
{
  if (md_node_is_nil(t_coff_field(ctx, parent, name))) {
    *value_out = default_value;
    if (required) { t_parse_errorf(ctx, T_ResultCode_ValidationError, parent, "missing field '%s'", name); }
    return !required;
  }
  String8 string = {0};
  if (!t_coff_scalar(ctx, parent, name, required, &string)) {
    *value_out = default_value;
    return !required;
  }
  U64 value = 0;
  if (!try_u64_from_str8_c_rules(string, &value) || value > max_value) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, parent, name), "field '%s' must be an integer no greater than %llu", name, max_value);
    return 0;
  }
  *value_out = value;
  return 1;
}

internal B32
t_coff_bool(T_ParseContext *ctx, MD_Node *parent, char *name, B32 default_value, B32 *value_out)
{
  if (md_node_is_nil(t_coff_field(ctx, parent, name))) {
    *value_out = default_value;
    return 1;
  }
  String8 string = {0};
  if (!t_coff_scalar(ctx, parent, name, 0, &string)) {
    *value_out = default_value;
    return 1;
  }
  if (str8_match(string, str8_lit("true"), StringMatchFlag_CaseInsensitive)) {
    *value_out = 1;
  } else if (str8_match(string, str8_lit("false"), StringMatchFlag_CaseInsensitive)) {
    *value_out = 0;
  } else {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, parent, name), "field '%s' must be true or false", name);
    return 0;
  }
  return 1;
}

internal B32
t_coff_machine(T_ParseContext *ctx, MD_Node *parent, COFF_MachineType *machine_out)
{
  String8 string = {0};
  if (!t_coff_scalar(ctx, parent, "machine", 1, &string)) { return 0; }
  COFF_MachineType machine = coff_machine_from_string(string);
  if (machine == COFF_MachineType_Unknown && !str8_match(string, str8_lit("unknown"), StringMatchFlag_CaseInsensitive)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, parent, "machine"), "unknown COFF machine '%S'", string);
    return 0;
  }
  *machine_out = machine;
  return 1;
}

internal T_COFF_Section *
t_coff_section_from_id(T_COFF_Object *object, String8 id)
{
  for (T_COFF_Section *section = object->first_section; section != 0; section = section->next) {
    if (str8_match(section->id, id, 0)) { return section; }
  }
  return 0;
}

internal T_COFF_Symbol *
t_coff_symbol_from_id(T_COFF_Object *object, String8 id)
{
  for
    EachNode(symbol, T_COFF_Symbol, object->first_symbol)
    {
      if (str8_match(symbol->id, id, 0)) { return symbol; }
    }
  return 0;
}

internal COFF_SectionFlags
t_coff_permission_flag(String8 string)
{
  if (str8_match(string, str8_lit("read"), StringMatchFlag_CaseInsensitive)) { return COFF_SectionFlag_MemRead; }
  if (str8_match(string, str8_lit("write"), StringMatchFlag_CaseInsensitive)) { return COFF_SectionFlag_MemWrite; }
  if (str8_match(string, str8_lit("execute"), StringMatchFlag_CaseInsensitive)) { return COFF_SectionFlag_MemExecute; }
  return 0;
}

internal COFF_SectionFlags
t_coff_section_flag(String8 string)
{
  struct Map
  {
    char *name;
    COFF_SectionFlags flag;
  };
  local_persist struct Map map[] = {
      {"discardable", COFF_SectionFlag_MemDiscardable}, {"not_cached", COFF_SectionFlag_MemNotCached}, {"not_paged", COFF_SectionFlag_MemNotPaged},
      {"shared", COFF_SectionFlag_MemShared},           {"link_comdat", COFF_SectionFlag_LnkCOMDAT},   {"link_info", COFF_SectionFlag_LnkInfo},
      {"link_remove", COFF_SectionFlag_LnkRemove},      {"link_other", COFF_SectionFlag_LnkOther},     {"link_nreloc_overflow", COFF_SectionFlag_LnkNRelocOvfl},
      {"gp_relative", COFF_SectionFlag_GpRel},          {"type_no_pad", COFF_SectionFlag_TypeNoPad},   {"16bit", COFF_SectionFlag_Mem16Bit},
      {"locked", COFF_SectionFlag_MemLocked},           {"preload", COFF_SectionFlag_MemPreload},
  };
  for
    EachElement(i, map)
    {
      if (str8_match(string, str8_cstring(map[i].name), StringMatchFlag_CaseInsensitive)) { return map[i].flag; }
    }
  return 0;
}

internal void
t_coff_parse_named_flags(T_ParseContext *ctx, MD_Node *field, B32 permissions, COFF_SectionFlags *flags_io)
{
  if (md_node_is_nil(field)) { return; }
  COFF_SectionFlags seen = 0;
  for
    MD_EachNode(value, field->first)
    {
      COFF_SectionFlags flag = permissions ? t_coff_permission_flag(value->string) : t_coff_section_flag(value->string);
      if (flag == 0) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, value, "unknown COFF section %s '%S'", permissions ? "permission" : "flag", value->string);
      } else if (seen & flag) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, value, "duplicate COFF section %s '%S'", permissions ? "permission" : "flag", value->string);
      } else {
        seen |= flag;
      }
    }
  *flags_io |= seen;
}

internal B32
t_coff_reloc_type_from_string(COFF_MachineType machine, String8 string, COFF_RelocType *type_out)
{
  for
    EachIndex(value, 256)
    {
      String8 candidate = coff_string_from_reloc(machine, (COFF_RelocType)value);
      if (candidate.size != 0 && str8_match(candidate, string, StringMatchFlag_CaseInsensitive)) {
        *type_out = (COFF_RelocType)value;
        return 1;
      }
    }
  return 0;
}

internal T_COFF_SymbolKind
t_coff_symbol_kind_from_string(String8 string)
{
  struct Map
  {
    char *name;
    T_COFF_SymbolKind kind;
  };
  local_persist struct Map map[] = {
      {"external", T_COFF_SymbolKind_External},
      {"external_function", T_COFF_SymbolKind_ExternalFunction},
      {"static", T_COFF_SymbolKind_Static},
      {"section_definition", T_COFF_SymbolKind_SectionDefinition},
      {"weak", T_COFF_SymbolKind_Weak},
      {"absolute", T_COFF_SymbolKind_Absolute},
      {"undefined", T_COFF_SymbolKind_Undefined},
      {"undefined_function", T_COFF_SymbolKind_UndefinedFunction},
      {"undefined_section", T_COFF_SymbolKind_UndefinedSection},
      {"section", T_COFF_SymbolKind_Section},
      {"common", T_COFF_SymbolKind_Common},
  };
  for
    EachElement(i, map)
    {
      if (str8_match(string, str8_cstring(map[i].name), StringMatchFlag_CaseInsensitive)) { return map[i].kind; }
    }
  return T_COFF_SymbolKind_Null;
}

internal COFF_ComdatSelectType
t_coff_selection_from_string(String8 string)
{
  for (U64 value = COFF_ComdatSelect_Null; value <= COFF_ComdatSelect_Largest; value += 1) {
    if (str8_match(coff_string_from_comdat_select_type((COFF_ComdatSelectType)value), string, StringMatchFlag_CaseInsensitive)) { return (COFF_ComdatSelectType)value; }
  }
  return (COFF_ComdatSelectType)max_U32;
}

internal COFF_WeakExtType
t_coff_weak_search_from_string(String8 string)
{
  if (str8_match(string, str8_lit("no_library"), StringMatchFlag_CaseInsensitive)) { return COFF_WeakExt_NoLibrary; }
  if (str8_match(string, str8_lit("search_library"), StringMatchFlag_CaseInsensitive)) { return COFF_WeakExt_SearchLibrary; }
  if (str8_match(string, str8_lit("alias"), StringMatchFlag_CaseInsensitive)) { return COFF_WeakExt_SearchAlias; }
  if (str8_match(string, str8_lit("anti_dependency"), StringMatchFlag_CaseInsensitive)) { return COFF_WeakExt_AntiDependency; }
  return COFF_WeakExt_Null;
}

internal void
t_coff_parse_relocations(T_ParseContext *ctx, T_COFF_Object *object, T_COFF_Section *section, MD_Node *list)
{
  if (md_node_is_nil(list)) { return; }
  char *allowed[] = {"type", "offset", "symbol", 0};
  for
    MD_EachNode(node, list->first)
    {
      if (section->relocation_count >= max_U16) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "section '%S' exceeds the COFF relocation limit", section->id);
        continue;
      }
      t_coff_reject_unknown(ctx, node, allowed);
      T_COFF_Relocation *relocation = push_array(ctx->arena, T_COFF_Relocation, 1);
      relocation->node = node;
      relocation->id = node->string;
      String8 type = {0};
      U64 offset = 0;
      t_coff_scalar(ctx, node, "type", 1, &type);
      t_coff_u64(ctx, node, "offset", 1, 0, max_U32, &offset);
      t_coff_scalar(ctx, node, "symbol", 1, &relocation->symbol_id);
      relocation->offset = (U32)offset;
      if (!t_coff_reloc_type_from_string(object->machine, type, &relocation->type)) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, node, "type"), "relocation '%S' is invalid for machine %S", type,
                       coff_string_from_machine_type(object->machine));
      }
      SLLQueuePush(section->first_relocation, section->last_relocation, relocation);
      section->relocation_count += 1;
    }
}

internal void
t_coff_parse_sections(T_ParseContext *ctx, T_COFF_Object *object, MD_Node *list)
{
  if (md_node_is_nil(list)) { return; }
  char *allowed[] = {"name", "permissions", "content", "alignment", "flags", "raw_flags", "data", "relocations", 0};
  for
    MD_EachNode(node, list->first)
    {
      if (object->section_count >= max_U16) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "object exceeds the COFF section limit");
        continue;
      }
      t_coff_reject_unknown(ctx, node, allowed);
      if (node->string.size == 0 || t_coff_section_from_id(object, node->string) != 0) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "section IDs must be non-empty and unique");
        continue;
      }
      T_COFF_Section *section = push_array(ctx->arena, T_COFF_Section, 1);
      section->node = node;
      section->id = node->string;
      t_coff_scalar(ctx, node, "name", 1, &section->name);
      t_coff_parse_named_flags(ctx, t_coff_field(ctx, node, "permissions"), 1, &section->flags);
      String8 content = {0};
      if (t_coff_scalar(ctx, node, "content", 1, &content)) {
        if (str8_match(content, str8_lit("code"), StringMatchFlag_CaseInsensitive)) {
          section->flags |= COFF_SectionFlag_CntCode;
        } else if (str8_match(content, str8_lit("initialized_data"), StringMatchFlag_CaseInsensitive)) {
          section->flags |= COFF_SectionFlag_CntInitializedData;
        } else if (str8_match(content, str8_lit("uninitialized_data"), StringMatchFlag_CaseInsensitive)) {
          section->flags |= COFF_SectionFlag_CntUninitializedData;
        } else {
          t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, node, "content"), "unknown COFF section content kind '%S'", content);
        }
      }
      U64 alignment = 0;
      if (!md_node_is_nil(t_coff_field(ctx, node, "alignment"))) {
        t_coff_u64(ctx, node, "alignment", 0, 0, 8192, &alignment);
        COFF_SectionFlags align_flags = coff_section_flag_from_align_size(alignment);
        if (align_flags == 0) {
          t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, node, "alignment"), "unsupported COFF section alignment %llu", alignment);
        } else {
          section->flags |= align_flags;
        }
      }
      t_coff_parse_named_flags(ctx, t_coff_field(ctx, node, "flags"), 0, &section->flags);
      U64 raw_flags = 0;
      if (!md_node_is_nil(t_coff_field(ctx, node, "raw_flags"))) {
        t_coff_u64(ctx, node, "raw_flags", 0, 0, max_U32, &raw_flags);
        COFF_SectionFlags align_mask = COFF_SectionFlag_AlignMask << COFF_SectionFlag_AlignShift;
        COFF_SectionFlags content_mask = COFF_SectionFlag_CntCode | COFF_SectionFlag_CntInitializedData | COFF_SectionFlag_CntUninitializedData;
        if ((section->flags & raw_flags) || ((section->flags & align_mask) && (raw_flags & align_mask)) || ((section->flags & content_mask) && (raw_flags & content_mask))) {
          t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, node, "raw_flags"), "raw_flags conflicts with typed section flags");
        }
        COFF_SectionFlags raw_content = (COFF_SectionFlags)raw_flags & content_mask;
        if ((raw_content & (raw_content - 1)) != 0 || COFF_SectionFlags_ExtractAlign(raw_flags) == COFF_SectionFlag_AlignMask) {
          t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, node, "raw_flags"), "raw_flags contains an invalid content kind or alignment");
        }
        section->flags |= (COFF_SectionFlags)raw_flags;
      }
      if (section->flags & COFF_SectionFlag_LnkNRelocOvfl) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "link_nreloc_overflow is writer-owned and cannot be specified");
      }
      MD_Node *data = t_coff_field(ctx, node, "data");
      if (md_node_is_nil(data)) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "section requires data");
      } else {
        t_bytes_from_producer(ctx->run, data, &section->data);
      }
      SLLQueuePush(object->first_section, object->last_section, section);
      object->section_count += 1;
      t_coff_parse_relocations(ctx, object, section, t_coff_field(ctx, node, "relocations"));
    }
}

internal void
t_coff_parse_symbols(T_ParseContext *ctx, T_COFF_Object *object, MD_Node *list)
{
  if (md_node_is_nil(list)) { return; }
  for
    MD_EachNode(node, list->first)
    {
      if (node->string.size == 0 || t_coff_symbol_from_id(object, node->string) != 0) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "symbol IDs must be non-empty and unique");
        continue;
      }
      T_COFF_Symbol *symbol = push_array(ctx->arena, T_COFF_Symbol, 1);
      symbol->node = node;
      symbol->id = node->string;
      String8 kind = {0};
      t_coff_scalar(ctx, node, "kind", 1, &kind);
      symbol->kind = t_coff_symbol_kind_from_string(kind);
      if (symbol->kind == T_COFF_SymbolKind_Null) { t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, node, "kind"), "unknown COFF symbol kind '%S'", kind); }
      char *known_fields[] = {"kind", "name", "section", "value", "size", "selection", "associate", "fallback", "search", "storage", 0};
      char *regular_fields[] = {"kind", "name", "section", "value", 0};
      char *section_definition_fields[] = {"kind", "section", "selection", "associate", 0};
      char *weak_fields[] = {"kind", "name", "fallback", "search", 0};
      char *absolute_fields[] = {"kind", "name", "value", "storage", 0};
      char *undefined_fields[] = {"kind", "name", 0};
      char *undefined_section_fields[] = {"kind", "name", "value", 0};
      char *section_fields[] = {"kind", "name", "section", 0};
      char *common_fields[] = {"kind", "name", "size", 0};
      char **allowed = known_fields;
      switch (symbol->kind) {
      case T_COFF_SymbolKind_External:
      case T_COFF_SymbolKind_ExternalFunction:
      case T_COFF_SymbolKind_Static:
        allowed = regular_fields;
        break;
      case T_COFF_SymbolKind_SectionDefinition:
        allowed = section_definition_fields;
        break;
      case T_COFF_SymbolKind_Weak:
        allowed = weak_fields;
        break;
      case T_COFF_SymbolKind_Absolute:
        allowed = absolute_fields;
        break;
      case T_COFF_SymbolKind_Undefined:
      case T_COFF_SymbolKind_UndefinedFunction:
        allowed = undefined_fields;
        break;
      case T_COFF_SymbolKind_UndefinedSection:
        allowed = undefined_section_fields;
        break;
      case T_COFF_SymbolKind_Section:
        allowed = section_fields;
        break;
      case T_COFF_SymbolKind_Common:
        allowed = common_fields;
        break;
      default:
        break;
      }
      t_coff_reject_unknown(ctx, node, allowed);
      t_coff_scalar(ctx, node, "name", 0, &symbol->name);
      t_coff_scalar(ctx, node, "section", 0, &symbol->section_id);
      t_coff_scalar(ctx, node, "associate", 0, &symbol->associate_id);
      t_coff_scalar(ctx, node, "fallback", 0, &symbol->fallback_id);
      U64 value = 0, size = 0;
      t_coff_u64(ctx, node, "value", 0, 0, max_U32, &value);
      t_coff_u64(ctx, node, "size", 0, 0, max_U32, &size);
      symbol->value = (U32)value;
      symbol->size = (U32)size;
      String8 selection = {0};
      if (!md_node_is_nil(t_coff_field(ctx, node, "selection")) && t_coff_scalar(ctx, node, "selection", 0, &selection)) {
        symbol->selection = t_coff_selection_from_string(selection);
        if (symbol->selection == (COFF_ComdatSelectType)max_U32) {
          t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, node, "selection"), "unknown COMDAT selection '%S'", selection);
        }
      }
      String8 search = {0};
      if (!md_node_is_nil(t_coff_field(ctx, node, "search")) && t_coff_scalar(ctx, node, "search", 0, &search)) {
        symbol->weak_search = t_coff_weak_search_from_string(search);
        if (symbol->weak_search == COFF_WeakExt_Null) {
          t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, node, "search"), "unknown weak search policy '%S'", search);
        }
      }
      String8 storage = {0};
      if (!md_node_is_nil(t_coff_field(ctx, node, "storage")) && t_coff_scalar(ctx, node, "storage", 0, &storage)) {
        if (str8_match(storage, str8_lit("external"), StringMatchFlag_CaseInsensitive)) {
          symbol->storage_class = COFF_SymStorageClass_External;
        } else if (str8_match(storage, str8_lit("static"), StringMatchFlag_CaseInsensitive)) {
          symbol->storage_class = COFF_SymStorageClass_Static;
        } else {
          t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, node, "storage"), "absolute symbol storage must be external or static");
        }
      }
      SLLQueuePush(object->first_symbol, object->last_symbol, symbol);
      object->symbol_count += 1;
    }

  for (T_COFF_Symbol *symbol = object->first_symbol; symbol != 0; symbol = symbol->next) {
    B32 needs_name = symbol->kind != T_COFF_SymbolKind_SectionDefinition;
    B32 needs_section = symbol->kind == T_COFF_SymbolKind_External || symbol->kind == T_COFF_SymbolKind_ExternalFunction || symbol->kind == T_COFF_SymbolKind_Static ||
                        symbol->kind == T_COFF_SymbolKind_SectionDefinition || symbol->kind == T_COFF_SymbolKind_Section;
    if (needs_name && symbol->name.size == 0) { t_parse_errorf(ctx, T_ResultCode_ValidationError, symbol->node, "symbol '%S' requires name", symbol->id); }
    if (needs_section && t_coff_section_from_id(object, symbol->section_id) == 0) {
      t_parse_errorf(ctx, T_ResultCode_ValidationError, symbol->node, "symbol '%S' references unknown section '%S'", symbol->id, symbol->section_id);
    }
    if (symbol->kind == T_COFF_SymbolKind_SectionDefinition) {
      if (md_node_is_nil(t_coff_field(ctx, symbol->node, "selection"))) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, symbol->node, "section definition requires selection");
      }
      if (symbol->selection == (COFF_ComdatSelectType)max_U32) { continue; }
      if (symbol->selection == COFF_ComdatSelect_Associative && t_coff_section_from_id(object, symbol->associate_id) == 0) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, symbol->node, "associative section definition requires a valid associate section");
      }
    }
    if (symbol->kind == T_COFF_SymbolKind_Weak) {
      T_COFF_Symbol *fallback = t_coff_symbol_from_id(object, symbol->fallback_id);
      if (fallback == 0 || fallback == symbol || fallback->encoded != (COFF_ObjSymbol *)1) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, symbol->node, "weak symbol fallback '%S' must refer to an earlier symbol", symbol->fallback_id);
      }
      if (symbol->weak_search == COFF_WeakExt_Null) { t_parse_errorf(ctx, T_ResultCode_ValidationError, symbol->node, "weak symbol requires search policy"); }
    }
    if (symbol->kind == T_COFF_SymbolKind_Absolute && md_node_is_nil(t_coff_field(ctx, symbol->node, "storage"))) {
      t_parse_errorf(ctx, T_ResultCode_ValidationError, symbol->node, "absolute symbol requires storage");
    }
    if (symbol->kind == T_COFF_SymbolKind_UndefinedSection && md_node_is_nil(t_coff_field(ctx, symbol->node, "value"))) {
      t_parse_errorf(ctx, T_ResultCode_ValidationError, symbol->node, "undefined section symbol requires value");
    }
    if (symbol->kind == T_COFF_SymbolKind_Common && md_node_is_nil(t_coff_field(ctx, symbol->node, "size"))) {
      t_parse_errorf(ctx, T_ResultCode_ValidationError, symbol->node, "common symbol requires size");
    }
    symbol->encoded = (COFF_ObjSymbol *)1;
  }
  for (T_COFF_Symbol *symbol = object->first_symbol; symbol != 0; symbol = symbol->next) { symbol->encoded = 0; }

  for
    EachNode(section, T_COFF_Section, object->first_section)
    {
    for
      EachNode(relocation, T_COFF_Relocation, section->first_relocation)
      {
        if (t_coff_symbol_from_id(object, relocation->symbol_id) == 0) {
          t_parse_errorf(ctx, T_ResultCode_ValidationError, relocation->node, "relocation references unknown symbol '%S'", relocation->symbol_id);
        }
      }
    }
}

internal T_COFF_Object *
t_coff_parse_object(T_ParseContext *ctx, MD_Node *node)
{
  char *allowed[] = {"machine", "timestamp", "sections", "symbols", "directives", 0};
  t_coff_reject_unknown(ctx, node, allowed);
  T_COFF_Object *object = push_array(ctx->arena, T_COFF_Object, 1);
  object->node = node;
  t_coff_machine(ctx, node, &object->machine);
  U64 timestamp = 0;
  t_coff_u64(ctx, node, "timestamp", 0, 0, max_U32, &timestamp);
  object->timestamp = (COFF_TimeStamp)timestamp;
  t_coff_parse_sections(ctx, object, t_coff_field(ctx, node, "sections"));
  t_coff_parse_symbols(ctx, object, t_coff_field(ctx, node, "symbols"));
  MD_Node *directives = t_coff_field(ctx, node, "directives");
  for
    MD_EachNode(directive_node, directives->first)
    {
      if (!str8_match(directive_node->string, str8_lit("directive"), StringMatchFlag_CaseInsensitive) || md_node_is_nil(directive_node->first) ||
          !md_node_is_nil(directive_node->first->next)) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, directive_node, "directives must contain 'directive: value' entries");
        continue;
      }
      T_COFF_Directive *directive = push_array(ctx->arena, T_COFF_Directive, 1);
      directive->node = directive_node;
      directive->string = directive_node->first->string;
      SLLQueuePush(object->first_directive, object->last_directive, directive);
      object->directive_count += 1;
    }
  return object;
}

internal COFF_ImportByType
t_coff_import_by_from_string(String8 string)
{
  if (str8_match(string, str8_lit("ordinal"), StringMatchFlag_CaseInsensitive)) { return COFF_ImportBy_Ordinal; }
  if (str8_match(string, str8_lit("name"), StringMatchFlag_CaseInsensitive)) { return COFF_ImportBy_Name; }
  if (str8_match(string, str8_lit("name_no_prefix"), StringMatchFlag_CaseInsensitive)) { return COFF_ImportBy_NameNoPrefix; }
  if (str8_match(string, str8_lit("undecorate"), StringMatchFlag_CaseInsensitive)) { return COFF_ImportBy_Undecorate; }
  return (COFF_ImportByType)max_U32;
}

internal void
t_coff_parse_library_member(T_ParseContext *ctx, T_COFF_Library *library, MD_Node *node)
{
  char *allowed[] = {"path", "object", "import", "dll_import", 0};
  t_coff_reject_unknown(ctx, node, allowed);
  T_COFF_LibraryMember *member = push_array(ctx->arena, T_COFF_LibraryMember, 1);
  member->node = node;
  member->id = node->string;
  MD_Node *object_node = t_coff_field(ctx, node, "object");
  MD_Node *import_node = t_coff_field(ctx, node, "import");
  MD_Node *dll_node = t_coff_field(ctx, node, "dll_import");
  U64 kind_count = !md_node_is_nil(object_node) + !md_node_is_nil(import_node) + !md_node_is_nil(dll_node);
  if (kind_count != 1) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "library member requires exactly one object, import, or dll_import definition");
  } else if (!md_node_is_nil(object_node)) {
    char *object_allowed[] = {"path", "object", 0};
    t_coff_reject_unknown(ctx, node, object_allowed);
    member->kind = T_COFF_LibraryMemberKind_Object;
    t_coff_scalar(ctx, node, "path", 0, &member->path);
    if (member->path.size == 0) { member->path = str8f(ctx->arena, "%S.obj", member->id); }
    member->object = t_coff_parse_object(ctx, object_node);
  } else if (!md_node_is_nil(import_node)) {
    char *member_allowed[] = {"import", 0};
    t_coff_reject_unknown(ctx, node, member_allowed);
    member->kind = T_COFF_LibraryMemberKind_Import;
    char *import_allowed[] = {"dll", "name", "machine", "timestamp", "type", "lookup", "hint", "ordinal", 0};
    t_coff_reject_unknown(ctx, import_node, import_allowed);
    t_coff_scalar(ctx, import_node, "dll", 1, &member->dll);
    t_coff_scalar(ctx, import_node, "name", 1, &member->name);
    t_coff_machine(ctx, import_node, &member->machine);
    U64 timestamp = 0;
    t_coff_u64(ctx, import_node, "timestamp", 0, 0, max_U32, &timestamp);
    member->timestamp = (COFF_TimeStamp)timestamp;
    String8 type = {0};
    t_coff_scalar(ctx, import_node, "type", 1, &type);
    member->import_type = coff_import_header_type_from_string(type);
    if (member->import_type == COFF_ImportType_Invalid) {
      t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, import_node, "type"), "unsupported import type '%S'", type);
    }
    String8 lookup = {0};
    t_coff_scalar(ctx, import_node, "lookup", 1, &lookup);
    member->import_by = t_coff_import_by_from_string(lookup);
    if (member->import_by == (COFF_ImportByType)max_U32) {
      t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, import_node, "lookup"), "unknown import lookup '%S'", lookup);
    }
    U64 hint_or_ordinal = 0;
    char *number_field = member->import_by == COFF_ImportBy_Ordinal ? "ordinal" : "hint";
    char *name_import_allowed[] = {"dll", "name", "machine", "timestamp", "type", "lookup", "hint", 0};
    char *ordinal_import_allowed[] = {"dll", "name", "machine", "timestamp", "type", "lookup", "ordinal", 0};
    t_coff_reject_unknown(ctx, import_node, member->import_by == COFF_ImportBy_Ordinal ? ordinal_import_allowed : name_import_allowed);
    t_coff_u64(ctx, import_node, number_field, 0, 0, max_U16, &hint_or_ordinal);
    member->hint_or_ordinal = (U16)hint_or_ordinal;
  } else if (!md_node_is_nil(dll_node)) {
    char *member_allowed[] = {"dll_import", 0};
    t_coff_reject_unknown(ctx, node, member_allowed);
    member->kind = T_COFF_LibraryMemberKind_DllImport;
    char *dll_allowed[] = {"name", "machine", "timestamp", 0};
    t_coff_reject_unknown(ctx, dll_node, dll_allowed);
    t_coff_scalar(ctx, dll_node, "name", 1, &member->name);
    t_coff_machine(ctx, dll_node, &member->machine);
    if (member->machine != COFF_MachineType_X64) {
      t_parse_errorf(ctx, T_ResultCode_ValidationError, t_coff_field(ctx, dll_node, "machine"), "dll_import currently supports only x64");
    }
    U64 timestamp = 0;
    t_coff_u64(ctx, dll_node, "timestamp", 0, 0, max_U32, &timestamp);
    member->timestamp = (COFF_TimeStamp)timestamp;
  }
  SLLQueuePush(library->first_member, library->last_member, member);
  library->member_count += 1;
}

internal T_COFF_Library *
t_coff_parse_library(T_ParseContext *ctx, MD_Node *node)
{
  char *allowed[] = {"timestamp", "mode", "second_linker_member", "members", 0};
  t_coff_reject_unknown(ctx, node, allowed);
  T_COFF_Library *library = push_array(ctx->arena, T_COFF_Library, 1);
  library->node = node;
  U64 timestamp = 0, mode = 0;
  t_coff_u64(ctx, node, "timestamp", 0, 0, max_U32, &timestamp);
  t_coff_u64(ctx, node, "mode", 0, 0, max_U16, &mode);
  t_coff_bool(ctx, node, "second_linker_member", 0, &library->second_linker_member);
  library->timestamp = (COFF_TimeStamp)timestamp;
  library->mode = (U16)mode;
  MD_Node *members = t_coff_field(ctx, node, "members");
  for
    MD_EachNode(member, members->first) { t_coff_parse_library_member(ctx, library, member); }
  if (library->second_linker_member) {
    U64 emitted_member_count = 0;
    for (T_COFF_LibraryMember *member = library->first_member; member != 0; member = member->next) {
      emitted_member_count += member->kind == T_COFF_LibraryMemberKind_DllImport ? 3 : 1;
    }
    if (emitted_member_count > max_U16) { t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "library exceeds the second linker member limit"); }
  }
  return library;
}

internal T_Result
t_coff_validate(T_ParseContext *ctx, T_Artifact *artifact)
{
  ctx->operation = str8_lit("coff");
  MD_Node *definition = artifact->definition;
  char *allowed[] = {"object", "library", 0};
  t_coff_reject_unknown(ctx, definition, allowed);
  MD_Node *object = t_coff_field(ctx, definition, "object");
  MD_Node *library = t_coff_field(ctx, definition, "library");
  if (md_node_is_nil(object) == md_node_is_nil(library)) {
    return t_parse_errorf(ctx, T_ResultCode_ValidationError, definition, "coff requires exactly one object or library definition");
  }
  T_COFF_Model *model = push_array(ctx->arena, T_COFF_Model, 1);
  if (!md_node_is_nil(object)) {
    model->kind = T_COFF_ModelKind_Object;
    model->object = t_coff_parse_object(ctx, object);
  } else {
    model->kind = T_COFF_ModelKind_Library;
    model->library = t_coff_parse_library(ctx, library);
  }
  artifact->codec_data = model;
  return ctx->run->result;
}

internal String8
t_coff_encode_object(T_Context *ctx, T_COFF_Object *object)
{
  COFF_ObjWriter *writer = coff_obj_writer_alloc(object->timestamp, object->machine);
  for (T_COFF_Section *section = object->first_section; section != 0; section = section->next) {
    section->encoded = coff_obj_writer_push_section(writer, section->name, section->flags, section->data);
  }
  for (T_COFF_Directive *directive = object->first_directive; directive != 0; directive = directive->next) { coff_obj_writer_push_directive(writer, directive->string); }
  for (T_COFF_Symbol *symbol = object->first_symbol; symbol != 0; symbol = symbol->next) {
    T_COFF_Section *section = t_coff_section_from_id(object, symbol->section_id);
    switch (symbol->kind) {
    case T_COFF_SymbolKind_External:
      symbol->encoded = coff_obj_writer_push_symbol_extern(writer, symbol->name, symbol->value, section->encoded);
      break;
    case T_COFF_SymbolKind_ExternalFunction:
      symbol->encoded = coff_obj_writer_push_symbol_extern_func(writer, symbol->name, symbol->value, section->encoded);
      break;
    case T_COFF_SymbolKind_Static:
      symbol->encoded = coff_obj_writer_push_symbol_static(writer, symbol->name, symbol->value, section->encoded);
      break;
    case T_COFF_SymbolKind_SectionDefinition: {
      if (symbol->selection == COFF_ComdatSelect_Associative) {
        T_COFF_Section *associate = t_coff_section_from_id(object, symbol->associate_id);
        symbol->encoded = coff_obj_writer_push_symbol_associative(writer, section->encoded, associate->encoded);
      } else {
        symbol->encoded = coff_obj_writer_push_symbol_secdef(writer, section->encoded, symbol->selection);
      }
    } break;
    case T_COFF_SymbolKind_Weak: {
      T_COFF_Symbol *fallback = t_coff_symbol_from_id(object, symbol->fallback_id);
      symbol->encoded = coff_obj_writer_push_symbol_weak(writer, symbol->name, symbol->weak_search, fallback->encoded);
    } break;
    case T_COFF_SymbolKind_Absolute:
      symbol->encoded = coff_obj_writer_push_symbol_abs(writer, symbol->name, symbol->value, symbol->storage_class);
      break;
    case T_COFF_SymbolKind_Undefined:
      symbol->encoded = coff_obj_writer_push_symbol_undef(writer, symbol->name);
      break;
    case T_COFF_SymbolKind_UndefinedFunction:
      symbol->encoded = coff_obj_writer_push_symbol_undef_func(writer, symbol->name);
      break;
    case T_COFF_SymbolKind_UndefinedSection:
      symbol->encoded = coff_obj_writer_push_symbol_undef_sect(writer, symbol->name, symbol->value);
      break;
    case T_COFF_SymbolKind_Section:
      symbol->encoded = coff_obj_writer_push_symbol_sect(writer, symbol->name, section->encoded);
      break;
    case T_COFF_SymbolKind_Common:
      symbol->encoded = coff_obj_writer_push_symbol_common(writer, symbol->name, symbol->size);
      break;
    default:
      break;
    }
  }
  for (T_COFF_Section *section = object->first_section; section != 0; section = section->next) {
    for (T_COFF_Relocation *relocation = section->first_relocation; relocation != 0; relocation = relocation->next) {
      T_COFF_Symbol *symbol = t_coff_symbol_from_id(object, relocation->symbol_id);
      coff_obj_writer_section_push_reloc(writer, section->encoded, relocation->offset, symbol->encoded, relocation->type);
    }
  }
  String8 result = coff_obj_writer_serialize(ctx->arena, writer);
  coff_obj_writer_release(&writer);
  return result;
}

internal String8
t_coff_encode_library(T_Context *ctx, T_COFF_Library *library)
{
  COFF_LibWriter *writer = coff_lib_writer_alloc();
  for (T_COFF_LibraryMember *member = library->first_member; member != 0; member = member->next) {
    switch (member->kind) {
    case T_COFF_LibraryMemberKind_Object:
      coff_lib_writer_push_obj(writer, member->path, t_coff_encode_object(ctx, member->object));
      break;
    case T_COFF_LibraryMemberKind_Import:
      coff_lib_writer_push_import(writer, member->machine, member->timestamp, member->dll, member->import_by, member->name, member->hint_or_ordinal, member->import_type);
      break;
    case T_COFF_LibraryMemberKind_DllImport: {
      String8 dll_name = str8_chop_last_dot(member->name);
      String8 debug = lnk_make_linker_debug_symbols(writer->arena, member->machine);
      coff_lib_writer_push_obj(writer, dll_name, pe_make_import_entry_obj(writer->arena, dll_name, member->timestamp, member->machine, debug));
      coff_lib_writer_push_obj(writer, dll_name, pe_make_null_import_descriptor_obj(writer->arena, member->timestamp, member->machine, debug));
      coff_lib_writer_push_obj(writer, dll_name, pe_make_null_thunk_data_obj(writer->arena, dll_name, member->timestamp, member->machine, debug));
    } break;
    default:
      break;
    }
  }
  String8 result = coff_lib_writer_serialize(ctx->arena, writer, library->timestamp, library->mode, library->second_linker_member);
  coff_lib_writer_release(&writer);
  return result;
}

internal T_Result
t_coff_encode(T_Context *ctx, T_Artifact *artifact)
{
  T_COFF_Model *model = artifact->codec_data;
  if (model->kind == T_COFF_ModelKind_Object) {
    artifact->data = t_coff_encode_object(ctx, model->object);
  } else if (model->kind == T_COFF_ModelKind_Library) {
    artifact->data = t_coff_encode_library(ctx, model->library);
  }
  return ctx->result;
}

internal T_Result
t_coff_decode_object(T_Context *ctx, T_Artifact *artifact, String8 data, MD_Node *object)
{
  COFF_FileHeaderInfo header = coff_file_header_info_from_data(data);
  if (header.section_table_range.max > data.size || header.symbol_table_range.max > data.size || header.string_table_range.max > data.size ||
      dim_1u64(header.section_table_range) != header.section_count_no_null * sizeof(COFF_SectionHeader) ||
      dim_1u64(header.symbol_table_range) != header.symbol_count * header.symbol_size) {
    return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("coff"), "COFF object has invalid table ranges");
  }

  String8 string_table = str8_substr(data, header.string_table_range);
  String8 symbol_table = str8_substr(data, header.symbol_table_range);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(data, header.section_table_range).str;
  String8 machine = coff_string_from_machine_type(header.machine);
  if (machine.size == 0 && header.machine == COFF_MachineType_Unknown) { machine = str8_lit("Unknown"); }
  t_codec_push_field(ctx, object, "machine", machine);
  t_codec_push_field(ctx, object, "big_object", header.is_big_obj ? str8_lit("true") : str8_lit("false"));
  t_codec_push_u64(ctx, object, "section_count", header.section_count_no_null);
  t_codec_push_u64(ctx, object, "symbol_record_count", header.symbol_count);

  MD_Node *sections = t_codec_push_node(ctx->arena, object, str8_lit("sections"));
  for
    EachIndex(section_idx, header.section_count_no_null)
    {
      COFF_SectionHeader *section_header = &section_table[section_idx];
      MD_Node *section = t_codec_push_node(ctx->arena, sections, str8f(ctx->arena, "section_%llu", section_idx + 1));
      String8 section_name = {0};
      if (!t_codec_coff_section_name(string_table, section_header, &section_name)) {
        return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("coff"), "COFF section %llu has an invalid name", section_idx + 1);
      }
      t_codec_push_field(ctx, section, "name", section_name);
      t_codec_push_u64(ctx, section, "virtual_size", section_header->vsize);
      t_codec_push_u64(ctx, section, "virtual_offset", section_header->voff);
      t_codec_push_u64(ctx, section, "file_size", section_header->fsize);
      t_codec_push_u64(ctx, section, "file_offset", section_header->foff);
      t_codec_push_u64(ctx, section, "alignment", coff_align_size_from_section_flags(section_header->flags));
      t_codec_push_u64(ctx, section, "raw_flags", section_header->flags);

      String8 section_data = str8_zero();
      if (!(section_header->flags & COFF_SectionFlag_CntUninitializedData) && section_header->fsize != 0) {
        if (section_header->foff > data.size || section_header->fsize > data.size - section_header->foff) {
          return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("coff"), "COFF section %llu has an invalid data range", section_idx + 1);
        }
        section_data = str8_substr(data, rng_1u64(section_header->foff, section_header->foff + section_header->fsize));
      }
      t_codec_push_field(ctx, section, "data", t_hex_from_data(ctx->arena, section_data));

      COFF_RelocInfo reloc_info = coff_reloc_info_from_section_header(data, section_header);
      if (reloc_info.array_off > data.size || reloc_info.count > (data.size - reloc_info.array_off) / sizeof(COFF_Reloc)) {
        return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("coff"), "COFF section %llu has an invalid relocation range", section_idx + 1);
      }
      MD_Node *relocations = t_codec_push_node(ctx->arena, section, str8_lit("relocations"));
      COFF_Reloc *reloc_array = (COFF_Reloc *)(data.str + reloc_info.array_off);
    for
      EachIndex(reloc_idx, reloc_info.count)
      {
        COFF_Reloc *reloc = &reloc_array[reloc_idx];
        MD_Node *relocation = t_codec_push_node(ctx->arena, relocations, str8f(ctx->arena, "relocation_%llu", reloc_idx));
        t_codec_push_u64(ctx, relocation, "offset", reloc->apply_off);
        t_codec_push_u64(ctx, relocation, "symbol_index", reloc->isymbol);
        String8 type = coff_string_from_reloc(header.machine, reloc->type);
        if (type.size != 0) { t_codec_push_field(ctx, relocation, "type", type); }
        t_codec_push_u64(ctx, relocation, "raw_type", reloc->type);
      }
    }

  MD_Node *symbols = t_codec_push_node(ctx->arena, object, str8_lit("symbols"));
  U64 symbol_idx = 0;
  U64 primary_symbol_count = 0;
  while (symbol_idx < header.symbol_count) {
    COFF_ParsedSymbol symbol = coff_parse_symbol(header, string_table, symbol_table, (U32)symbol_idx);
    if (symbol.aux_symbol_count > header.symbol_count - symbol_idx - 1) {
      return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("coff"), "COFF symbol %llu has an invalid auxiliary record count", symbol_idx);
    }
    MD_Node *symbol_node = t_codec_push_node(ctx->arena, symbols, str8f(ctx->arena, "symbol_%llu", primary_symbol_count));
    t_codec_push_u64(ctx, symbol_node, "record_index", symbol_idx);
    t_codec_push_field(ctx, symbol_node, "name", symbol.name);
    t_codec_push_u64(ctx, symbol_node, "value", symbol.value);
    t_codec_push_u64(ctx, symbol_node, "section_number", symbol.section_number);
    t_codec_push_u64(ctx, symbol_node, "raw_type", symbol.type.v);
    t_codec_push_field(ctx, symbol_node, "storage_class", coff_string_from_sym_storage_class(symbol.storage_class));
    t_codec_push_u64(ctx, symbol_node, "raw_storage_class", symbol.storage_class);
    t_codec_push_u64(ctx, symbol_node, "auxiliary_record_count", symbol.aux_symbol_count);
    if (symbol.aux_symbol_count != 0) {
      U64 aux_off = (symbol_idx + 1) * header.symbol_size;
      String8 aux_data = str8_substr(symbol_table, rng_1u64(aux_off, aux_off + symbol.aux_symbol_count * header.symbol_size));
      t_codec_push_field(ctx, symbol_node, "auxiliary_data", t_hex_from_data(ctx->arena, aux_data));
    }
    symbol_idx += 1 + symbol.aux_symbol_count;
    primary_symbol_count += 1;
  }
  t_codec_push_u64(ctx, object, "symbol_count", primary_symbol_count);
  return ctx->result;
}

internal T_Result
t_coff_decode(T_Context *ctx, T_Artifact *artifact, MD_Node **semantic_tree_out)
{
  String8 data = artifact->data;
  MD_Node *root = t_codec_push_node(ctx->arena, 0, str8_lit("coff"));
  if (coff_is_obj(data) || coff_is_big_obj(data)) {
    MD_Node *object = t_codec_push_node(ctx->arena, root, str8_lit("object"));
    T_Result result = t_coff_decode_object(ctx, artifact, data, object);
    if (!t_result_is_ok(result)) { return result; }
  } else if (coff_is_regular_archive(data)) {
    COFF_ArchiveParse archive_parse = coff_regular_archive_parse_from_data(data);
    if (archive_parse.error.size != 0) {
      return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("coff"), "invalid COFF archive: %S", archive_parse.error);
    }
    MD_Node *library = t_codec_push_node(ctx->arena, root, str8_lit("library"));
    t_codec_push_field(ctx, library, "second_linker_member", archive_parse.has_second_header ? str8_lit("true") : str8_lit("false"));
    t_codec_push_field(ctx, library, "long_names", archive_parse.long_names.size != 0 ? str8_lit("true") : str8_lit("false"));
    t_codec_push_u64(ctx, library, "first_symbol_count", archive_parse.first_member.symbol_count);
    if (archive_parse.has_second_header) { t_codec_push_u64(ctx, library, "second_symbol_count", archive_parse.second_member.symbol_count); }
    MD_Node *members = t_codec_push_node(ctx->arena, library, str8_lit("members"));
    U64 member_idx = 0;
    U64 offset = coff_regular_archive_member_iter_init(data);
    COFF_ArchiveMember member = {0};
    while (coff_regular_archive_member_iter_next(data, &offset, &member)) {
      if (str8_match_lit("/", member.header.name, 0) || str8_match_lit("//", member.header.name, 0)) { continue; }
      COFF_DataType member_type = coff_data_type_from_data(member.data);
      MD_Node *member_node = t_codec_push_node(ctx->arena, members, str8f(ctx->arena, "member_%llu", member_idx));
      t_codec_push_field(ctx, member_node, "name", coff_decode_member_name(archive_parse.long_names, member.header.name));
      t_codec_push_u64(ctx, member_node, "timestamp", member.header.time_stamp);
      t_codec_push_u64(ctx, member_node, "user_id", member.header.user_id);
      t_codec_push_u64(ctx, member_node, "group_id", member.header.group_id);
      t_codec_push_field(ctx, member_node, "mode", member.header.mode);
      if (member_type == COFF_DataType_Import) {
        COFF_ParsedArchiveImportHeader import = {0};
        COFF_ImportHeader *raw_import = str8_deserial_get_raw_ptr(member.data, 0, sizeof(*raw_import));
        B32 import_is_valid = raw_import != 0 && raw_import->data_size <= member.data.size - sizeof(*raw_import);
        if (import_is_valid) {
          String8 import_data = str8_substr(member.data, rng_1u64(sizeof(*raw_import), sizeof(*raw_import) + raw_import->data_size));
          String8 ignored = {0};
          U64 cursor = str8_deserial_read_cstr(import_data, 0, &ignored);
          cursor += str8_deserial_read_cstr(import_data, cursor, &ignored);
          import_is_valid = cursor == import_data.size;
        }
        if (!import_is_valid || coff_parse_import(member.data, 0, &import) == 0) {
          return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("coff"), "archive member %llu has an invalid import header", member_idx);
        }
        MD_Node *import_node = t_codec_push_node(ctx->arena, member_node, str8_lit("import"));
        t_codec_push_field(ctx, import_node, "machine", coff_string_from_machine_type(import.machine));
        t_codec_push_field(ctx, import_node, "dll", import.dll_name);
        t_codec_push_field(ctx, import_node, "name", import.func_name);
        t_codec_push_u64(ctx, import_node, "timestamp", import.time_stamp);
        t_codec_push_u64(ctx, import_node, "hint_or_ordinal", import.hint_or_ordinal);
        t_codec_push_field(ctx, import_node, "type", coff_string_from_import_header_type(import.type));
        t_codec_push_u64(ctx, import_node, "lookup", import.import_by);
      } else {
        MD_Node *object = t_codec_push_node(ctx->arena, member_node, str8_lit("object"));
        T_Result result = t_coff_decode_object(ctx, artifact, member.data, object);
        if (!t_result_is_ok(result)) { return result; }
      }
      member_idx += 1;
    }
    t_codec_push_u64(ctx, library, "member_count", member_idx);
  } else {
    return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("coff"), "artifact is not a supported COFF object or archive");
  }
  *semantic_tree_out = root;
  return ctx->result;
}
