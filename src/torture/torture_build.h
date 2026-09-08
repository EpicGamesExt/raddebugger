// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef enum T_BuildCommandKind
{
  T_BuildCommandKind_Null,
  T_BuildCommandKind_Compile,
  T_BuildCommandKind_CompileLink,
  T_BuildCommandKind_Link,
  T_BuildCommandKind_Resource,
  T_BuildCommandKind_Run,
  T_BuildCommandKind_Copy,
} T_BuildCommandKind;

typedef enum T_BuildOutputMode
{
  T_BuildOutputMode_Default,
  T_BuildOutputMode_Explicit,
  T_BuildOutputMode_None,
} T_BuildOutputMode;

typedef struct T_BuildDeclaration T_BuildDeclaration;
struct T_BuildDeclaration
{
  T_BuildDeclaration *next;
  String8 name;
  MD_Node *definition;
};

typedef struct T_BuildDeclarationList T_BuildDeclarationList;
struct T_BuildDeclarationList
{
  T_BuildDeclaration *first;
  T_BuildDeclaration *last;
  U64 count;
};

typedef struct T_BuildOutput T_BuildOutput;
struct T_BuildOutput
{
  T_BuildOutput *next;
  String8 name;
  String8 path;
  MD_Node *definition;
};

typedef struct T_BuildCommand T_BuildCommand;
struct T_BuildCommand
{
  T_BuildCommand *next;
  T_BuildCommandKind kind;
  String8 tool;
  String8 arguments;
  String8 artifact;
  String8 output;
  T_BuildOutputMode output_mode;
  String8 index_name;
  U64 repeat_count;
  U64 timeout_ms;
  String8 expected_exit;
  String8 when_previous_exit;
  String8 stdout_pattern;
  String8 stderr_pattern;
  MD_Node *produces;
  MD_Node *definition;
  B32 parallel;
  B32 inject_output;
};

typedef struct T_BuildVariant T_BuildVariant;
struct T_BuildVariant
{
  T_BuildVariant *next;
  OperatingSystem os;
  T_BuildOutput *first_output;
  T_BuildOutput *last_output;
  U64 output_count;
  T_BuildCommand *first_command;
  T_BuildCommand *last_command;
  U64 command_count;
  MD_Node *definition;
  B32 output_none;
};

typedef struct T_BuildTarget T_BuildTarget;
struct T_BuildTarget
{
  T_BuildTarget *next;
  String8 name;
  String8 kind;
  T_BuildVariant *first_variant;
  T_BuildVariant *last_variant;
  U64 variant_count;
  MD_Node *definition;
};

struct T_BuildPlan
{
  B32 is_single_target;
  MD_Node *definition;
  MD_Node *defaults;
  T_BuildDeclarationList values;
  T_BuildDeclarationList configurations;
  T_BuildDeclarationList features;
  T_BuildDeclarationList toolchains;
  T_BuildDeclarationList packages;
  T_BuildDeclarationList resources;
  T_BuildDeclarationList generators;
  T_BuildTarget *first_target;
  T_BuildTarget *last_target;
  U64 target_count;
};

typedef struct T_ScriptBinding T_ScriptBinding;
struct T_ScriptBinding
{
  T_ScriptBinding *next;
  String8 name;
  String8 value;
};

internal T_Result t_build_parse(T_ParseContext *ctx, MD_Node *build, T_BuildPlan **plan_out);
internal T_Result t_build_execute(T_Context *ctx);
internal String8 t_script_expand(T_Context *ctx, MD_Node *node, String8 string, T_BuildVariant *variant, T_ScriptBinding *bindings);

internal T_Result t_op_repeat_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_op_repeat_execute(T_Context *ctx, MD_Node *arguments);
internal T_Result t_op_compare_file_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_op_compare_file_execute(T_Context *ctx, MD_Node *arguments);

global T_OpSpec t_op_repeat;
global T_OpSpec t_op_compare_file;
