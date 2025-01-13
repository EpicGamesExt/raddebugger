// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
lnk_alt_name_list_concat_in_place(LNK_AltNameList *list, LNK_AltNameList *to_concat)
{
  str8_list_concat_in_place(&list->from_list, &to_concat->from_list);
  str8_list_concat_in_place(&list->to_list, &to_concat->to_list);
}

internal LNK_MergeDirectiveNode *
lnk_merge_directive_list_push(Arena *arena, LNK_MergeDirectiveList *list, LNK_MergeDirective data)
{
  LNK_MergeDirectiveNode *node = push_array_no_zero(arena, LNK_MergeDirectiveNode, 1);
  node->data                   = data;
  node->next                   = 0;
  
  SLLQueuePush(list->first, list->last, node);
  ++list->count;
  
  return node;
}

////////////////////////////////

internal B32
lnk_is_directive_legal(LNK_CmdSwitchType type)
{
  static B32 init_table = 1;
  static B8  is_legal[LNK_CmdSwitch_Count];
  if (init_table) {
    init_table = 0;
    is_legal[LNK_CmdSwitch_AlternateName]      = 1;
    is_legal[LNK_CmdSwitch_DefaultLib]         = 1;
    is_legal[LNK_CmdSwitch_DisallowLib]        = 1;
    is_legal[LNK_CmdSwitch_EditAndContinue]    = 1;
    is_legal[LNK_CmdSwitch_Entry]              = 1;
    is_legal[LNK_CmdSwitch_Export]             = 1;
    is_legal[LNK_CmdSwitch_FailIfMismatch]     = 1;
    is_legal[LNK_CmdSwitch_GuardSym]           = 1;
    is_legal[LNK_CmdSwitch_Include]            = 1;
    is_legal[LNK_CmdSwitch_InferAsanLibs]      = 1;
    is_legal[LNK_CmdSwitch_InferAsanLibsNo]    = 1;
    is_legal[LNK_CmdSwitch_ManifestDependency] = 1;
    is_legal[LNK_CmdSwitch_Merge]              = 1;
    is_legal[LNK_CmdSwitch_NoDefaultLib]       = 1;
    is_legal[LNK_CmdSwitch_Release]            = 1;
    is_legal[LNK_CmdSwitch_Section]            = 1;
    is_legal[LNK_CmdSwitch_Stack]              = 1;
    is_legal[LNK_CmdSwitch_SubSystem]          = 1;
    is_legal[LNK_CmdSwitch_ThrowingNew]        = 1;
  }
  return is_legal[type];
}

internal void
lnk_parse_directives(Arena *arena, LNK_DirectiveInfo *directive_info, String8 buffer, String8 obj_path)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 to_parse;
  {
    local_persist const U8 bom_sig[]   = { 0xEF, 0xBB, 0xBF };
    local_persist const U8 ascii_sig[] = { 0x20, 0x20, 0x20 };
    if (MemoryMatch(buffer.str, &bom_sig[0], sizeof(bom_sig))) {
      to_parse = str8_zero();
      lnk_not_implemented("TODO: support for BOM encoding");
    } else if (MemoryMatch(buffer.str, &ascii_sig[0], sizeof(ascii_sig))) {
      to_parse = str8_skip(buffer, sizeof(ascii_sig));
    } else {
      to_parse = buffer;
    }
  }
  
  String8List arg_list = lnk_arg_list_parse_windows_rules(scratch.arena, to_parse);
  LNK_CmdLine cmd_line = lnk_cmd_line_parse_windows_rules(scratch.arena, arg_list);

  for (LNK_CmdOption *opt = cmd_line.first_option; opt != 0; opt = opt->next) {
    LNK_CmdSwitchType type = lnk_cmd_switch_type_from_string(opt->string);

    if (type == LNK_CmdSwitch_Null) {
      lnk_error(LNK_Warning_UnknownDirective, "%S: unknown directive \"%S\"", obj_path, opt->string);
      continue;
    }
    if (!lnk_is_directive_legal(type)) {
      lnk_error(LNK_Warning_IllegalDirective, "%S: illegal directive \"%S\"", obj_path, opt->string);
      continue;
    }

    LNK_Directive *directive = push_array_no_zero(arena, LNK_Directive, 1);
    directive->next          = 0;
    directive->id            = push_str8_copy(arena, opt->string);
    directive->value_list    = str8_list_copy(arena, &opt->value_strings);

    LNK_DirectiveList *directive_list = &directive_info->v[type];
    SLLQueuePush(directive_list->first, directive_list->last, directive);
    ++directive_list->count;
  }
  
  scratch_end(scratch);
}

internal String8List
lnk_parse_default_lib_directive(Arena *arena, LNK_DirectiveList *dir_list)
{
  ProfBeginFunction();
  String8List default_libs = {0};
  
  for (LNK_Directive *dir = dir_list->first; dir != 0; dir = dir->next) {
    for (String8Node *i = dir->value_list.first; i != 0; i = i->next) {
      String8 lib_path = i->string;
      
      // is there lib extension?
      String8 ext = str8_skip_last_dot(lib_path);
      if (ext.size == lib_path.size) { // TODO: fix string_extension_from_path, if there is no extension it should return zero
        lib_path = push_str8f(arena, "%S.lib", lib_path);
      } else {
        lib_path = push_str8_copy(arena, lib_path);
      }
      
      str8_list_push(arena, &default_libs, lib_path);
    }
  }
  
  ProfEnd();
  return default_libs;
}

internal LNK_ExportParse *
lnk_parse_export_direcive(Arena *arena, LNK_ExportParseList *list, String8List value_list, LNK_Obj *obj)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  LNK_ExportParse *parse = 0;

  // parse directive
  String8 name  = str8(0,0);
  String8 alias = str8(0,0);
  String8 type  = coff_string_from_import_header_type(COFF_ImportHeaderType_CODE);
  if (value_list.node_count > 0) {
    String8List dir_split = str8_split_by_string_chars(scratch.arena, value_list.first->string, str8_lit("="), 0);
    B32 is_export_valid = value_list.node_count <= 2 && value_list.node_count > 0;
    if (is_export_valid) {
      if (dir_split.node_count > 0) {
        name = dir_split.last->string;
      }
      if (dir_split.node_count == 2) {
        alias = dir_split.first->string;
      }
      if (value_list.node_count == 2) {
        type = value_list.last->string;
      }
    }
  }
  
  // prase error check
  if (name.size == 0) {
    String8 dir = str8_list_join(scratch.arena, &value_list, 0);
    lnk_error_obj(LNK_Error_IllData, obj, "invalid export directive \"%S\"", dir);
    goto exit;
  }
  
  parse = push_array_no_zero(arena, LNK_ExportParse, 1);
  parse->next  = 0;
  parse->name  = name;
  parse->alias = alias;
  parse->type  = type;
  
  SLLQueuePush(list->first, list->last, parse);
  ++list->count;
  
exit:;
  scratch_end(scratch);
  ProfEnd();
  return parse;
}

internal B32
lnk_parse_merge_directive(String8 string, LNK_MergeDirective *out)
{
  Temp scratch = scratch_begin(0, 0);
  B32 is_parse_ok = 0;
  
  String8List list = str8_split_by_string_chars(scratch.arena, string, str8_lit("="), 0);
  if (list.node_count == 2) {
    out->src = list.first->string;
    out->dst = list.last->string;
    is_parse_ok = 1;
  }
  
  scratch_end(scratch);
  return is_parse_ok;
}

