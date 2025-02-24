// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8List
lnk_arg_list_parse_windows_rules(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8List list = {0};
  
  U8 *ptr = string.str;
  U8 *opl = string.str + string.size;
  while (ptr < opl) {
    // skip white space and new lines
    for (;;) {
      U64 size = (U64)(opl - ptr);
      UnicodeDecode uni = utf8_decode(ptr, size);
      if (uni.codepoint != ' ' && uni.codepoint != '\n' && uni.codepoint != '\r') {
        break;
      }
      ptr += uni.inc;
    }
    
    if (*ptr == '\0') {
      break;
    }
    
    String8List token_builder = {0};
    U8 *anchor = ptr;
    while (ptr < opl) {
      UnicodeDecode uni;
      
      uni = utf8_decode(ptr, (U64)(opl-ptr));
      if (uni.codepoint == '\0' || uni.codepoint == '\n' || uni.codepoint == '\r' || uni.codepoint == ' ') {
        break;
      }
      
      // handle string and strip quotes
      if (uni.codepoint == '"') {
        String8 text_before_quote = str8(anchor, (U64)(ptr - anchor));
        str8_list_push(scratch.arena, &token_builder, text_before_quote);
        
        // advance past starting quote
        ptr += uni.inc;
        anchor = ptr;
        
        U8 *quote_end = ptr;
        while (ptr < opl) {
          uni = utf8_decode(ptr, (U64)(opl - ptr));
          ptr += uni.inc;
          // skip escape char
          if (uni.codepoint == '\\') {
            uni = utf8_decode(ptr, (U64)(opl - ptr));
            ptr += uni.inc;
          } else if (uni.codepoint == '"' || uni.codepoint == '\0') {
            break; // found matching quote char
          }
          quote_end = ptr;
        }
        
        String8 text_inside_quotes = str8(anchor, (U64)(quote_end - anchor));
        str8_list_push(scratch.arena, &token_builder, text_inside_quotes);
        anchor = ptr;
      } else {
        ptr += uni.inc;
      }
    }
    
    // push remaining text 
    String8 text = str8(anchor, (U64)(ptr - anchor));
    str8_list_push(scratch.arena, &token_builder, text);
    
    // push token
    String8 token = str8_list_join(arena, &token_builder, NULL);
    if (token.size) {
      str8_list_push(arena,  &list, token);
    }
  }
  
  scratch_end(scratch);
  return list;
}

internal void
lnk_cmd_line_push_option_node(LNK_CmdLine *cmd_line, LNK_CmdOption *opt)
{
  SLLQueuePush(cmd_line->first_option, cmd_line->last_option, opt);
  cmd_line->option_count += 1;
}

internal LNK_CmdOption *
lnk_cmd_line_push_option_list(Arena *arena, LNK_CmdLine *cmd_line, String8 string, String8List value_strings)
{
  // fill out node
  LNK_CmdOption *opt = push_array_no_zero(arena, LNK_CmdOption, 1);
  opt->next          = 0;
  opt->string        = string;
  opt->value_strings = value_strings;

  // push node
  lnk_cmd_line_push_option_node(cmd_line, opt);

  return opt;
}

internal LNK_CmdOption *
lnk_cmd_line_push_option_string(Arena *arena, LNK_CmdLine *cmd_line, String8 string, String8 value)
{
  String8List value_list = str8_split_by_string_chars(arena, value, str8_lit(","), StringSplitFlag_KeepEmpties);
  LNK_CmdOption *opt = lnk_cmd_line_push_option_list(arena, cmd_line, string, value_list);
  return opt;
}

internal LNK_CmdOption *
lnk_cmd_line_push_option(Arena *arena, LNK_CmdLine *cmd_line, char *string, char *value)
{
  return lnk_cmd_line_push_option_string(arena, cmd_line, str8_cstring(string), str8_cstring(value));
}

internal LNK_CmdOption *
lnk_cmd_line_push_option_if_not_present(Arena *arena, LNK_CmdLine *cmd_line, char *string, char *value)
{
  if (!lnk_cmd_line_has_option(*cmd_line, string)) {
    return lnk_cmd_line_push_option(arena, cmd_line, string, value);
  }
  return 0;
}

internal LNK_CmdLine
lnk_cmd_line_parse_windows_rules(Arena *arena, String8List arg_list)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_CmdLine cmd_line = {0};

  for (String8Node *arg_node = arg_list.first; arg_node != 0; arg_node = arg_node->next) {
    String8 arg = arg_node->string;
    B32 is_option = str8_match_lit("/", arg, StringMatchFlag_RightSideSloppy) ||
                    str8_match_lit("-", arg, StringMatchFlag_RightSideSloppy);
    if (is_option) {
      U64 param_start_pos = str8_find_needle(arg, 0, str8_lit(":"), 0);
      String8 option_name = str8_chop(arg, arg.size - param_start_pos);

      // remove '/' or '-' from option name
      option_name = str8_skip(option_name, 1);

      // skip ':'
      String8 value_string = str8_skip(arg, param_start_pos + 1);

      // make value list
      String8List value_list = str8_split_by_string_chars(arena, value_string, str8_lit(","), 0);

      // push command
      lnk_cmd_line_push_option_list(arena, &cmd_line, option_name, value_list);
    } else {
      str8_list_push(arena, &cmd_line.input_list, arg);
    }
  }
  
  scratch_end(scratch);
  return cmd_line;
}

internal LNK_CmdOption *
lnk_cmd_line_option_from_string(LNK_CmdLine cmd_line, String8 string)
{
  LNK_CmdOption *opt;
  for (opt = cmd_line.first_option; opt != NULL; opt = opt->next) {
    if (str8_match(string, opt->string, StringMatchFlag_CaseInsensitive)) {
      break;
    }
  }
  return opt;
}

internal B32
lnk_cmd_line_has_option_string(LNK_CmdLine cmd_line, String8 string)
{
  LNK_CmdOption *opt = lnk_cmd_line_option_from_string(cmd_line, string);
  B32 has_option = (opt != 0);
  return has_option;
}

internal B32
lnk_cmd_line_has_option(LNK_CmdLine cmd_line, char *string)
{
  return lnk_cmd_line_has_option_string(cmd_line, str8_cstring(string));
}

internal String8List
lnk_unwrap_rsp(Arena *arena, String8List arg_list)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List result = {0};

  for (String8Node *curr = arg_list.first; curr != 0; curr = curr->next) {
    B32 is_rsp = str8_match_lit("@", curr->string, StringMatchFlag_RightSideSloppy);
    if (is_rsp) {
      // remove "@"
      String8 name = str8_skip(curr->string, 1);

      if (os_file_path_exists(name)) {
        // read rsp from disk
        String8 file = lnk_read_data_from_file_path(scratch.arena, name);
        
        // parse rsp
        String8List rsp_args = lnk_arg_list_parse_windows_rules(scratch.arena, file);
        
        // handle case where rsp references another rsp
        String8List list = lnk_unwrap_rsp(arena, rsp_args);

        // push arguments from rsp
        list = str8_list_copy(arena, &list);
        str8_list_concat_in_place(&result, &list);
       } else {
        lnk_error(LNK_Error_Cmdl, "unable to find rsp: %S", name);
      }
    } else {
      // push regular argument
      String8 str = push_str8_copy(arena, curr->string);
      str8_list_push(arena, &result, str);
    }
  }
  
  scratch_end(scratch);
  return result;
}

internal String8List
lnk_data_from_cmd_line(Arena *arena, LNK_CmdLine cmd_line)
{
  String8List result = {0};

  for (LNK_CmdOption *opt = cmd_line.first_option; opt != 0; opt = opt->next) {
    // separate directives
    if (opt != cmd_line.first_option) {
      str8_list_pushf(arena, &result, " ");
    }

    // push new directive
    str8_list_pushf(arena, &result, "/%.*s", str8_varg(opt->string));

    // do we have arguments?
    if (opt->value_strings.node_count > 0) {
      str8_list_pushf(arena, &result, ":");

      for (String8Node *value_node = opt->value_strings.first; value_node != 0; value_node = value_node->next) {
        // separate arguments
        if (value_node != opt->value_strings.first) {
          str8_list_pushf(arena, &result, ",");
        }

        // push argument
        B32 has_spaces = str8_find_needle(value_node->string, 0, str8_lit(" "), StringMatchFlag_CaseInsensitive) < value_node->string.size;
        if (has_spaces) {
          str8_list_pushf(arena, &result, "\"%.*s\"", str8_varg(value_node->string));
        } else {
          str8_list_pushf(arena, &result, "%.*s", str8_varg(value_node->string));
        }
      }
    }
  }

  // append inputs
  for (String8Node *input_node = cmd_line.input_list.first; input_node != 0; input_node = input_node->next) {
    if (input_node != cmd_line.input_list.first) {
      str8_list_pushf(arena, &result, " ");
    }
    str8_list_pushf(arena, &result, "\"%.*s\"", str8_varg(input_node->string));
  }

  return result;
}
