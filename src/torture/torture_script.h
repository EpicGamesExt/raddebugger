// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct T_Context T_Context;
typedef struct T_ParseContext T_ParseContext;
typedef struct T_Codec T_Codec;
typedef struct T_OpSpec T_OpSpec;
typedef struct T_SuiteSpec T_SuiteSpec;
typedef struct T_Artifact T_Artifact;
typedef struct T_BuildPlan T_BuildPlan;
typedef struct T_ScriptBinding T_ScriptBinding;

typedef enum T_ArtifactState
{
  T_ArtifactState_Declared,
  T_ArtifactState_Validated,
  T_ArtifactState_Materialized,
  T_ArtifactState_Failed,
} T_ArtifactState;

typedef enum T_ResultCode
{
  T_ResultCode_Ok,
  T_ResultCode_ParseError,
  T_ResultCode_ValidationError,
  T_ResultCode_IoError,
  T_ResultCode_Mismatch,
} T_ResultCode;

typedef enum T_DiagnosticKind
{
  T_DiagnosticKind_Null,
  T_DiagnosticKind_Note,
  T_DiagnosticKind_Warning,
  T_DiagnosticKind_Error,
} T_DiagnosticKind;

typedef struct T_Diagnostic T_Diagnostic;
struct T_Diagnostic
{
  T_Diagnostic *next;
  MD_MsgKind    kind;
  String8       file_path;
  TxtPt         location;
  String8       operation;
  String8       message;
};

typedef struct T_DiagnosticList T_DiagnosticList;
struct T_DiagnosticList
{
  T_Diagnostic *first;
  T_Diagnostic *last;
  U64 count;
};

typedef struct T_Result T_Result;
struct T_Result
{
  T_ResultCode     code;
  T_DiagnosticList diagnostics;
};

struct T_Codec
{
  String8 kind;
  T_Result (*validate)(T_ParseContext *ctx, T_Artifact *artifact);
  T_Result (*encode)(T_Context *ctx, T_Artifact *artifact);
  T_Result (*decode)(T_Context *ctx, T_Artifact *artifact, MD_Node **semantic_tree_out);
};

struct T_OpSpec
{
  String8 name;
  T_Result (*validate)(T_ParseContext *ctx, MD_Node *arguments);
  T_Result (*execute)(T_Context *ctx, MD_Node *arguments);
};

typedef struct T_Command T_Command;
struct T_Command
{
  T_OpSpec *spec;
  MD_Node *arguments;
  TxtPt location;
  U64 order;
};

struct T_SuiteSpec
{
  String8 name;
  T_Codec *codecs;
  U64 codec_count;
  T_OpSpec *ops;
  U64 op_count;
  T_Result (*begin)(T_Context *ctx, MD_Node *test);
  void (*end)(T_Context *ctx);
};

struct T_Artifact
{
  T_Artifact *next;
  String8 name;
  String8 file_name;
  T_Codec *codec;
  MD_Node *definition;
  String8 data;
  void *codec_data;
  T_ArtifactState state;
};

struct T_Context
{
  Arena           *arena;
  TestCtx         *test_ctx;
  T_SuiteSpec     *suite;
  String8          file_path;
  String8          source;
  MD_Node         *root;
  MD_Node         *test;
  T_Artifact      *first_artifact;
  T_Artifact      *last_artifact;
  U64             artifact_count;
  T_BuildPlan     *build;
  T_ScriptBinding *bindings;
  T_Command       *commands;
  U64              command_count;
  T_Result         result;
  void            *suite_data;
};

struct T_ParseContext
{
  Arena       *arena;
  T_Context   *run;
  T_SuiteSpec *suite;
  String8      file_path;
  String8      source;
  String8      operation;
};

internal B32 t_result_is_ok(T_Result result);
internal T_Result t_context_errorf(T_Context *ctx, T_ResultCode code, MD_Node *node, String8 operation, char *fmt, ...);
internal T_Result t_parse_errorf(T_ParseContext *ctx, T_ResultCode code, MD_Node *node, char *fmt, ...);

internal T_Codec *t_codec_from_kind(T_SuiteSpec *suite, String8 kind);
internal T_OpSpec *t_op_spec_from_name(T_SuiteSpec *suite, String8 name);
internal T_Artifact *t_artifact_from_name(T_Context *ctx, String8 name);

internal T_Result t_script_parse(Arena *arena, TestCtx *test_ctx, T_SuiteSpec *suite, String8 file_path, String8 source, T_Context *ctx_out);
internal T_Result t_script_execute(T_Context *ctx);
internal T_Result t_bytes_from_producer(T_Context *ctx, MD_Node *producer, String8 *data_out);

internal T_Result t_codec_bytes_encode(T_Context *ctx, T_Artifact *artifact);
internal T_Result t_codec_bytes_decode(T_Context *ctx, T_Artifact *artifact, MD_Node **semantic_tree_out);
internal T_Result t_codec_text_encode(T_Context *ctx, T_Artifact *artifact);
internal T_Result t_codec_text_decode(T_Context *ctx, T_Artifact *artifact, MD_Node **semantic_tree_out);
internal T_Result t_op_compare_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_op_compare_execute(T_Context *ctx, MD_Node *arguments);
internal T_Result t_semantic_match(T_Context *ctx, MD_Node *expected, MD_Node *actual);

global T_Codec t_codec_bytes;
global T_Codec t_codec_text;
global T_OpSpec t_op_compare;
