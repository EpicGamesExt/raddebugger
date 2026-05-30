// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_CmdOption
{
  struct LNK_CmdOption *next;
  String8               string;
  String8List           value_strings;
} LNK_CmdOption;

typedef struct LNK_CmdLine
{
  U64            option_count;
  LNK_CmdOption *first_option;
  LNK_CmdOption *last_option;
  String8List    input_list;
  String8List    raw_cmd_line;
} LNK_CmdLine;

typedef enum LNK_EnvVarRule
{
  LNK_EnvVarRule_Null,
  LNK_EnvVarRule_Batch,
  LNK_EnvVarRule_Bash,
#if OS_WINDOWS
  LNK_EnvVarRule_Current,
#elif OS_LINUX
  LNK_EnvVarRule_Current,
#else
# error "define env var rules"
#endif
} LNK_EnvVarRule;

typedef struct LNK_EnvVar
{
  LNK_EnvVarRule rule;
  String8        raw_value;
  String8List    chain_values;
} LNK_EnvVar;

// --- List Helpers ------------------------------------------------------------

internal void lnk_cmd_line_concat_in_place(LNK_CmdLine *list, LNK_CmdLine *to_concat);

// --- Command Line ------------------------------------------------------------

internal String8List     lnk_arg_list_parse_windows_rules        (Arena *arena, String8 string);
internal LNK_CmdLine     lnk_cmd_line_parse_windows_rules        (Arena *arena, String8List arg_list);
internal LNK_CmdLine     lnk_cmd_line_from_stringfv_windows_rules(Arena *arena, char *fmt, va_list args);
internal LNK_CmdLine     lnk_cmd_line_from_stringf_windows_rules (Arena *arena, char *fmt, ...);
internal LNK_CmdOption * lnk_cmd_line_push_option_list           (Arena *arena, LNK_CmdLine *cmd_line, String8 string, String8List value_strings);
internal LNK_CmdOption * lnk_cmd_line_push_option                (Arena *arena, LNK_CmdLine *cmd_line, char *string, char *value);
internal LNK_CmdOption * lnk_cmd_line_push_option_if_not_present (Arena *arena, LNK_CmdLine *cmd_line, char *string, char *value);
internal String8List     lnk_data_from_cmd_line                  (Arena *arena, LNK_CmdLine cmd_line);
internal LNK_CmdOption * lnk_cmd_line_option_from_string         (LNK_CmdLine cmd_line, String8 string);
internal B32             lnk_cmd_line_has_option_string          (LNK_CmdLine cmd_line, String8 string);
internal B32             lnk_cmd_line_has_option                 (LNK_CmdLine cmd_line, char *string);

// --- Env Vars ----------------------------------------------------------------

internal HashMap lnk_env_vars_from_process_info(Arena *arena, ProcessInfo *proc_info, LNK_EnvVarRule rule);

internal LNK_EnvVar * lnk_env_var_from_map       (HashMap *env, String8 key);
internal LNK_EnvVar * lnk_env_var_from_mapf      (HashMap *env, char *fmt, ...);
internal LNK_EnvVar * lnk_env_var_push           (Arena *arena, HashMap *env, String8 key, String8 value, LNK_EnvVarRule rule);
internal LNK_EnvVar * lnk_env_var_push_string    (Arena *arena, HashMap *env, String8 string, LNK_EnvVarRule rule);
internal LNK_EnvVar * lnk_env_var_push_batch     (Arena *arena, HashMap *env, String8 string);
internal LNK_EnvVar * lnk_env_var_push_bash      (Arena *arena, HashMap *env, String8 string);
internal LNK_EnvVar * lnk_env_var_pushf          (Arena *arena, HashMap *env, LNK_EnvVarRule rule, char *fmt, ...);
internal String8List  lnk_value_list_from_env_var(Arena *arena, LNK_EnvVar *env_var);
internal B32          lnk_env_var_to_u64         (HashMap *env, LNK_EnvVar *var, U64 *value_out);
internal String8      lnk_expand_env_vars_windows(Arena *arena, HashMap *env, String8 string);
