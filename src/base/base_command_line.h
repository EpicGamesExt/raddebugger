// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_COMMAND_LINE_H
#define BASE_COMMAND_LINE_H

////////////////////////////////
//~ rjf: Parsed Command Line Types

typedef struct CmdLineOpt CmdLineOpt;
struct CmdLineOpt
{
  CmdLineOpt *next;
  CmdLineOpt *hash_next;
  U64 hash;
  String8 string;
  String8List value_strings;
  String8 value_string;
};

typedef struct CmdLineOptList CmdLineOptList;
struct CmdLineOptList
{
  U64 count;
  CmdLineOpt *first;
  CmdLineOpt *last;
};

typedef struct CmdLine CmdLine;
struct CmdLine
{
  String8 exe_name;
  CmdLineOptList options;
  String8List inputs;
  U64 option_table_size;
  CmdLineOpt **option_table;
};

////////////////////////////////
//~ NOTE(rjf): Command Line Option Parsing

internal U64              cmd_line_hash_from_string(String8 string);
internal CmdLineOpt**     cmd_line_slot_from_string(CmdLine *cmd_line, String8 string);
internal CmdLineOpt*      cmd_line_opt_from_slot(CmdLineOpt **slot, String8 string);
internal void             cmd_line_push_opt(CmdLineOptList *list, CmdLineOpt *var);
internal CmdLineOpt*      cmd_line_insert_opt(Arena *arena, CmdLine *cmd_line, String8 string, String8List values);
internal CmdLine          cmd_line_from_string_list(Arena *arena, String8List arguments);
internal CmdLineOpt*      cmd_line_opt_from_string(CmdLine *cmd_line, String8 name);
internal String8List      cmd_line_strings(CmdLine *cmd_line, String8 name);
internal String8          cmd_line_string(CmdLine *cmd_line, String8 name);
internal B32              cmd_line_has_flag(CmdLine *cmd_line, String8 name);
internal B32              cmd_line_has_argument(CmdLine *cmd_line, String8 name);

#endif // BASE_COMMAND_LINE_H
