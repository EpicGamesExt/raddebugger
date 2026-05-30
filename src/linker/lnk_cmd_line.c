// Copyright (c) Epic Games Tools
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
  LNK_CmdOption *opt = push_array(arena, LNK_CmdOption, 1);
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

internal void
lnk_cmd_line_concat_in_place(LNK_CmdLine *list, LNK_CmdLine *to_concat)
{
  if (list->option_count > 0) {
    list->option_count += to_concat->option_count;
    list->last_option->next = to_concat->first_option;
    list->last_option = to_concat->last_option;
    str8_list_concat_in_place(&list->input_list, &to_concat->input_list);
    str8_list_concat_in_place(&list->raw_cmd_line, &to_concat->raw_cmd_line);
  } else {
    *list = *to_concat;
  }
  MemoryZeroStruct(to_concat);
}

internal LNK_CmdLine
lnk_cmd_line_parse_windows_rules(Arena *arena, String8List arg_list)
{
  Temp scratch = scratch_begin(&arena, 1);
  LNK_CmdLine cmd_line = {0};
  cmd_line.raw_cmd_line = str8_list_copy(arena, &arg_list);

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
      String8List value_list_unowned = str8_split_by_string_chars(scratch.arena, value_string, str8_lit(","), 0);
      String8List value_list = str8_list_copy(arena, &value_list_unowned);

      // push command
      option_name = push_str8_copy(arena, option_name);
      lnk_cmd_line_push_option_list(arena, &cmd_line, option_name, value_list);
    } else {
      str8_list_push(arena, &cmd_line.input_list, push_str8_copy(arena, arg));
    }
  }
  scratch_end(scratch);
  return cmd_line;
}

internal LNK_CmdLine
lnk_cmd_line_from_stringfv_windows_rules(Arena *arena, char *fmt, va_list args)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  String8List arg_list = lnk_arg_list_parse_windows_rules(scratch.arena, string);
  LNK_CmdLine result = lnk_cmd_line_parse_windows_rules(arena, arg_list);
  scratch_end(scratch);
  return result;
}

internal LNK_CmdLine
lnk_cmd_line_from_stringf_windows_rules(Arena *arena, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  LNK_CmdLine result = lnk_cmd_line_from_stringfv_windows_rules(arena, fmt, args);
  va_end(args);
  return result;
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

internal String8
lnk_env_var_chain_separator_from_rule(LNK_EnvVarRule rule)
{
  String8 result = {0};
  switch (rule) {
  case LNK_EnvVarRule_Batch: { result = str8_lit(";"); } break;
  case LNK_EnvVarRule_Bash:  { result = str8_lit(":"); } break;
  default: break;
  }
  return result;
}

internal LNK_EnvVar *
lnk_env_var_push(Arena *arena, HashMap *env_vars, String8 key, String8 value, LNK_EnvVarRule rule)
{
  LNK_EnvVar *env_var = lnk_env_var_from_map(env_vars, key);
  if (env_var == 0) {
    env_var = push_array(arena, LNK_EnvVar, 1);
    env_var->raw_value = value;
    env_var->rule      = rule;
    hash_map_push_path_raw(arena, env_vars, key, env_var);
  }
  return env_var;
}

internal LNK_EnvVar *
lnk_env_var_push_string(Arena *arena, HashMap *env_vars, String8 string, LNK_EnvVarRule rule)
{
  LNK_EnvVar *result = 0;

  string = str8_skip_chop_whitespace(string);

  // string -> (key, value)
  String8 key = {0};
  String8 val = {0};
  switch (rule) {
  case LNK_EnvVarRule_Batch: {
    if (string.size >= 2) {
      if (str8_match_wildcard(string, str8_lit("\"*\""), 0)) {
        string = str8_chop(str8_skip(string, 1), 1);
      }
    }

    U64 sep_idx = str8_find_needle(string, 0, str8_lit("="), 0);
    if (sep_idx < string.size) {
      // extract key and value 
      key = str8_skip_chop_whitespace(str8_prefix(string, sep_idx));
      val = str8_skip_chop_whitespace(str8_skip(string, sep_idx+1));

      // strip quotes
      if (str8_match_wildcard(val, str8_lit("\"*\""), 0) || // "*"
          str8_match_wildcard(val, str8_lit("\'*\'"), 0)) { // '*'
        val = str8_chop(str8_skip(val, 1), 1);
      }
    }
  } break;

  case LNK_EnvVarRule_Bash: {
    U64 sep_idx = str8_find_needle(string, 0, str8_lit("="), 0);
    if (sep_idx < string.size) {
      // extract key and value 
      key = str8_skip_chop_whitespace(str8_prefix(string, sep_idx));
      val = str8_skip_chop_whitespace(str8_skip(string, sep_idx+1));

      // strip quotes
      if (str8_match_wildcard(val, str8_lit("\"*\""), 0) || // "*"
          str8_match_wildcard(val, str8_lit("\'*\'"), 0)) { // '*'
        val = str8_chop(str8_skip(val, 1), 1);
      }
    }
  } break;

  default: break;
  }

  if (key.size > 0) {
    result = lnk_env_var_push(arena, env_vars, key, val, rule);
  }

  return result;
}

internal LNK_EnvVar *
lnk_env_var_push_batch(Arena *arena, HashMap *env_vars, String8 string)
{
  return lnk_env_var_push_string(arena, env_vars, string, LNK_EnvVarRule_Batch);
}

internal LNK_EnvVar *
lnk_env_var_push_bash(Arena *arena, HashMap *env_vars, String8 string)
{
  return lnk_env_var_push_string(arena, env_vars, string, LNK_EnvVarRule_Bash);
}

internal LNK_EnvVar *
lnk_env_var_batchf(Arena *arena, HashMap *env_vars, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(arena, fmt, args);
  va_end(args);
  LNK_EnvVar *result = lnk_env_var_push_string(arena, env_vars, string, LNK_EnvVarRule_Batch);
  return result;
}

internal LNK_EnvVar *
lnk_env_var_bashf(Arena *arena, HashMap *env_vars, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(arena, fmt, args);
  va_end(args);
  LNK_EnvVar *result = lnk_env_var_push_string(arena, env_vars, string, LNK_EnvVarRule_Bash);
  return result;
}

internal LNK_EnvVar *
lnk_env_var_from_mapf(HashMap *env_vars, char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 key = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  LNK_EnvVar *result = hash_map_search_path_raw(env_vars, key);
  scratch_end(scratch);
  return result;
}

internal LNK_EnvVar *
lnk_env_var_from_map(HashMap *env_vars, String8 key)
{
  return hash_map_search_path_raw(env_vars, key);
}

internal String8List
lnk_value_list_from_env_var(Arena *arena, LNK_EnvVar *env_var)
{
  return str8_split_by_string_chars(arena, env_var->raw_value, lnk_env_var_chain_separator_from_rule(env_var->rule), 0);
}

internal B32
lnk_env_var_to_u64(HashMap *env_vars, LNK_EnvVar *env_var, U64 *value_out)
{
  String8 value = str8_skip_chop_whitespace(env_var->raw_value);
  if (str8_match_wildcard(value, str8_lit("\"*\""), 0) ||
      str8_match_wildcard(value, str8_lit("\'*\'"), 0)) {
    value = str8_chop(str8_skip(value, 1), 1);
  }
  return try_u64_from_str8_c_rules(value, value_out);
}

internal String8
lnk_expand_env_vars_windows(Arena *arena, HashMap *env_vars, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List list = {0};
  for (U64 i = 0; i < string.size; ) {
    U64 open  = str8_find_needle(string, i,      str8_lit("%"), 0);
    U64 close = str8_find_needle(string, open+1, str8_lit("%"), 0);

    String8 text = str8_substr(string, rng_1u64(i, open));
    str8_list_push(scratch.arena, &list, text);
    i += text.size;

    if (open < close) {
      String8     env_var_name = str8_substr(string, rng_1u64(open+1, close));
      LNK_EnvVar *env_var      = lnk_env_var_from_map(env_vars, env_var_name);
      if (env_var) {
        str8_list_push(scratch.arena, &list, env_var->raw_value);
        i = close+1;
      } else {
        str8_list_pushf(scratch.arena, &list, "%%%S", env_var_name);
        i = close;
      }
    }
  }

  String8 result = str8_list_join(arena, &list, 0);

  scratch_end(scratch);
  return result;
}

internal HashMap
lnk_env_vars_from_process_info(Arena *arena, ProcessInfo *proc_info, LNK_EnvVarRule rule)
{
  HashMap env_vars = {0};
  for EachNode(node, String8Node, proc_info->environment.first) {
    lnk_env_var_push_string(arena, &env_vars, str8_copy(arena, node->string), rule);
  }
  return env_vars;
}

