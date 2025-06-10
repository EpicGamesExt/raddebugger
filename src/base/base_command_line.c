// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Command Line Parsing Functions

internal CmdLineOpt **
cmd_line_slot_from_string(CmdLine *cmd_line, String8 string)
{
  CmdLineOpt **slot = 0;
  if(cmd_line->option_table_size != 0)
  {
    U64 hash = u64_hash_from_str8(string);
    U64 bucket = hash % cmd_line->option_table_size;
    slot = &cmd_line->option_table[bucket];
  }
  return slot;
}

internal CmdLineOpt *
cmd_line_opt_from_slot(CmdLineOpt **slot, String8 string)
{
  CmdLineOpt *result = 0;
  for(CmdLineOpt *var = *slot; var; var = var->hash_next)
  {
    if(str8_match(string, var->string, 0))
    {
      result = var;
      break;
    }
  }
  return result;
}

internal void
cmd_line_push_opt(CmdLineOptList *list, CmdLineOpt *var)
{
  SLLQueuePush(list->first, list->last, var);
  list->count += 1;
}

internal CmdLineOpt *
cmd_line_insert_opt(Arena *arena, CmdLine *cmd_line, String8 string, String8List values)
{
  CmdLineOpt *var = 0;
  CmdLineOpt **slot = cmd_line_slot_from_string(cmd_line, string);
  CmdLineOpt *existing_var = cmd_line_opt_from_slot(slot, string);
  if(existing_var != 0)
  {
    var = existing_var;
  }
  else
  {
    var = push_array(arena, CmdLineOpt, 1);
    var->hash_next = *slot;
    var->hash = u64_hash_from_str8(string);
    var->string = push_str8_copy(arena, string);
    var->value_strings = values;
    StringJoin join = {0};
    join.pre = str8_lit("");
    join.sep = str8_lit(",");
    join.post = str8_lit("");
    var->value_string = str8_list_join(arena, &var->value_strings, &join);
    *slot = var;
    cmd_line_push_opt(&cmd_line->options, var);
  }
  return var;
}

internal CmdLine
cmd_line_from_string_list(Arena *arena, String8List command_line)
{
  CmdLine parsed = {0};
  parsed.exe_name = command_line.first->string;
  parsed.option_table_size = 64;
  parsed.option_table = push_array(arena, CmdLineOpt *, parsed.option_table_size);
  
  //- rjf: parse options / inputs
  B32 after_passthrough_option = 0;
  B32 first_passthrough = 1;
  for(String8Node *node = command_line.first->next, *next = 0; node != 0; node = next)
  {
    next = node->next;
    
    //- rjf: look at --, -, or / (only on Windows) at the start of an
    // argument to determine if it's a flag option. all arguments after a
    // single "--" (with no trailing string on the command line will be
    // considered as passthrough input strings.
    B32 is_option = 0;
    String8 option_name = node->string;
    if(!after_passthrough_option)
    {
      is_option = 1;
      if(str8_match(node->string, str8_lit("--"), 0))
      {
        after_passthrough_option = 1;
        is_option = 0;
      }
      else if(str8_match(str8_prefix(node->string, 2), str8_lit("--"), 0))
      {
        option_name = str8_skip(option_name, 2);
      }
      else if(str8_match(str8_prefix(node->string, 1), str8_lit("-"), 0))
      {
        option_name = str8_skip(option_name, 1);
      }
      else if(operating_system_from_context() == OperatingSystem_Windows &&
              str8_match(str8_prefix(node->string, 1), str8_lit("/"), 0))
      {
        option_name = str8_skip(option_name, 1);
      }
      else
      {
        is_option = 0;
      }
    }
    
    //- rjf: string is an option
    if(is_option)
    {
      // rjf: unpack option prefix
      B32 has_values = 0;
      U64 value_signifier_position1 = str8_find_needle(option_name, 0, str8_lit(":"), 0);
      U64 value_signifier_position2 = str8_find_needle(option_name, 0, str8_lit("="), 0);
      U64 value_signifier_position = Min(value_signifier_position1, value_signifier_position2);
      String8 value_portion_this_string = str8_skip(option_name, value_signifier_position+1);
      if(value_signifier_position < option_name.size)
      {
        has_values = 1;
      }
      option_name = str8_prefix(option_name, value_signifier_position);
      
      // rjf: parse option's values
      String8List values = {0};
      if(has_values)
      {
        for(String8Node *n = node; n; n = n->next)
        {
          next = n->next;
          String8 string = n->string;
          if(n == node)
          {
            string = value_portion_this_string;
          }
          U8 splits[] = { ',' };
          String8List values_in_this_string = str8_split(arena, string, splits, ArrayCount(splits), 0);
          for(String8Node *sub_val = values_in_this_string.first; sub_val; sub_val = sub_val->next)
          {
            str8_list_push(arena, &values, sub_val->string);
          }
          if(!str8_match(str8_postfix(n->string, 1), str8_lit(","), 0) &&
             (n != node || value_portion_this_string.size != 0))
          {
            break;
          }
        }
      }
      
      // rjf: store
      cmd_line_insert_opt(arena, &parsed, option_name, values);
    }
    
    //- rjf: default path - treat as a passthrough input
    else if(!str8_match(node->string, str8_lit("--"), 0) || !first_passthrough)
    {
      str8_list_push(arena, &parsed.inputs, node->string);
      first_passthrough = 0;
    }
  }
  
  //- rjf: fill argc/argv
  parsed.argc = command_line.node_count;
  parsed.argv = push_array(arena, char *, parsed.argc);
  {
    U64 idx = 0;
    for(String8Node *n = command_line.first; n != 0; n = n->next)
    {
      parsed.argv[idx] = (char *)push_str8_copy(arena, n->string).str;
      idx += 1;
    }
  }
  
  return parsed;
}

internal CmdLineOpt *
cmd_line_opt_from_string(CmdLine *cmd_line, String8 name)
{
  return cmd_line_opt_from_slot(cmd_line_slot_from_string(cmd_line, name), name);
}

internal String8List 
cmd_line_strings(CmdLine *cmd_line, String8 name)
{
  String8List result = {0};
  CmdLineOpt *var = cmd_line_opt_from_string(cmd_line, name);
  if(var != 0)
  {
    result = var->value_strings;
  }
  return result;
}

internal String8     
cmd_line_string(CmdLine *cmd_line, String8 name)
{
  String8 result = {0};
  CmdLineOpt *var = cmd_line_opt_from_string(cmd_line, name);
  if(var != 0)
  {
    result = var->value_string;
  }
  return result;
}

internal B32
cmd_line_has_flag(CmdLine *cmd_line, String8 name)
{
  CmdLineOpt *var = cmd_line_opt_from_string(cmd_line, name);
  return (var != 0);
}

internal B32
cmd_line_has_argument(CmdLine *cmd_line, String8 name)
{
  CmdLineOpt *var = cmd_line_opt_from_string(cmd_line, name);
  return (var != 0 && var->value_strings.node_count > 0);
}
