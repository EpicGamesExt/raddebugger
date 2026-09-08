// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

global T_Codec t_codec_bytes = {str8_lit_comp("bytes"), 0, t_codec_bytes_encode, t_codec_bytes_decode};
global T_Codec t_codec_text = {str8_lit_comp("text"), 0, t_codec_text_encode, t_codec_text_decode};
global T_OpSpec t_op_compare = {str8_lit_comp("compare"), t_op_compare_validate, t_op_compare_execute};

inline internal B32
t_result_is_ok(T_Result result)
{
  return result.code == T_ResultCode_Ok;
}

internal TxtPt
t_txt_pt_from_offset(String8 source, U64 offset)
{
  TxtPt result = {1, 1};

  U64 offset_cap = Min(offset, source.size);

  for (U64 i = 0; i < offset_cap; i += 1) {
    if (source.str[i] == '\n') {
      result.line += 1;
      result.column = 1;
    } else {
      result.column += 1;
    }
  }

  return result;
}

internal void
t_diagnostic_from_md_msg(T_Context *ctx, MD_Msg *msg)
{
  T_Diagnostic *diagnostic = push_array(ctx->arena, T_Diagnostic, 1);
  diagnostic->kind      = msg->kind;
  diagnostic->file_path = ctx->file_path;
  diagnostic->location  = t_txt_pt_from_offset(ctx->source, msg->node->src_offset);
  diagnostic->message   = str8_copy(ctx->arena, msg->string);

  SLLQueuePush(ctx->result.diagnostics.first, ctx->result.diagnostics.last, diagnostic);
  ctx->result.diagnostics.count += 1;

  if (ctx->result.code == T_ResultCode_Ok && msg->kind >= MD_MsgKind_Error) {
    ctx->result.code = T_ResultCode_ParseError;
  }
}

internal T_Result
t_context_errorfv(T_Context *ctx, T_ResultCode code, MD_Node *node, String8 operation, char *fmt, va_list args)
{
  T_Diagnostic *diagnostic = push_array(ctx->arena, T_Diagnostic, 1);
  diagnostic->kind      = MD_MsgKind_Error;
  diagnostic->file_path = ctx->file_path;
  diagnostic->location  = t_txt_pt_from_offset(ctx->source, md_node_is_nil(node) ? 0 : node->src_offset);
  diagnostic->operation = operation;
  diagnostic->message   = push_str8fv(ctx->arena, fmt, args);

  SLLQueuePush(ctx->result.diagnostics.first, ctx->result.diagnostics.last, diagnostic);
  ctx->result.diagnostics.count += 1;

  if (ctx->result.code == T_ResultCode_Ok) { ctx->result.code = code; }
  return ctx->result;
}

inline internal T_Result
t_context_errorf(T_Context *ctx, T_ResultCode code, MD_Node *node, String8 operation, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  T_Result result = t_context_errorfv(ctx, code, node, operation, fmt, args);
  va_end(args);
  return result;
}

inline internal T_Result
t_parse_errorf(T_ParseContext *ctx, T_ResultCode code, MD_Node *node, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  T_Result result = t_context_errorfv(ctx->run, code, node, ctx->operation, fmt, args);
  va_end(args);
  return result;
}

inline internal T_Codec *
t_codec_from_kind(T_SuiteSpec *suite, String8 kind)
{
  for EachIndex(i, suite->codec_count) {
    if (str8_match(suite->codecs[i].kind, kind, StringMatchFlag_CaseInsensitive)) { return &suite->codecs[i]; }
  }
  return 0;
}

inline internal T_OpSpec *
t_op_spec_from_name(T_SuiteSpec *suite, String8 name)
{
  for EachIndex(i, suite->op_count) {
    if (str8_match(suite->ops[i].name, name, StringMatchFlag_CaseInsensitive)) { return &suite->ops[i]; }
  }
  return 0;
}

inline internal T_Artifact *
t_artifact_from_name(T_Context *ctx, String8 name)
{
  for (T_Artifact *artifact = ctx->first_artifact; artifact != 0; artifact = artifact->next) {
    if (str8_match(artifact->name, name, 0)) { return artifact; }
  }
  return 0;
}

inline internal MD_Node *
t_child_from_string(MD_Node *node, char *string)
{
  return md_child_from_string(node, str8_cstring(string), StringMatchFlag_CaseInsensitive);
}

internal MD_Node *
t_scalar_from_node(MD_Node *node)
{
  return md_node_is_nil(node) ? &md_nil_node : node->first;
}

internal String8
t_scalar_string_from_node(MD_Node *node)
{
  MD_Node *scalar = t_scalar_from_node(node);
  return md_node_is_nil(scalar) ? str8_zero() : scalar->string;
}

internal void
t_context_absorb_result(T_Context *ctx, T_Result result)
{
  if (result.diagnostics.first != 0 &&
      result.diagnostics.first != ctx->result.diagnostics.first) {
    if (ctx->result.diagnostics.last == 0) {
      ctx->result.diagnostics = result.diagnostics;
    } else {
      ctx->result.diagnostics.last->next = result.diagnostics.first;
      ctx->result.diagnostics.last = result.diagnostics.last;
      ctx->result.diagnostics.count += result.diagnostics.count;
    }
  }
  if (ctx->result.code == T_ResultCode_Ok && result.code != T_ResultCode_Ok) { ctx->result.code = result.code; }
}

internal T_Result
t_script_parse(Arena *arena, TestCtx *test_ctx, T_SuiteSpec *suite, String8 file_path, String8 source, T_Context *ctx_out)
{
  MemoryZeroStruct(ctx_out);

  T_Context *ctx = ctx_out;
  ctx->arena     = arena;
  ctx->test_ctx  = test_ctx;
  ctx->suite     = suite;
  ctx->file_path = str8_copy(arena, file_path);
  ctx->source    = str8_copy(arena, source);

  // tokenize & parse mdesk 
  MD_TokenizeResult tokenize = md_tokenize_from_text(arena, ctx->source);
  MD_ParseResult    parse    = md_parse_from_text_tokens(arena, ctx->file_path, ctx->source, tokenize.tokens);

  // error check tokenize & parse
  for EachNode(msg, MD_Msg, tokenize.msgs.first) { t_diagnostic_from_md_msg(ctx, msg); }
  for EachNode(msg, MD_Msg, parse.msgs.first)    { t_diagnostic_from_md_msg(ctx, msg); }
  if (!t_result_is_ok(ctx->result)) { return ctx->result; }

  ctx->root = parse.root;

  // does script have expected root node?
  {
    MD_Node *test_node = ctx->root->first;
    if (md_node_is_nil(test_node) || !str8_matchi(test_node->string, str8_lit("test"))) {
      return t_context_errorf(ctx, T_ResultCode_ValidationError, test_node, str8_zero(), "expected one top-level 'test' node");
    }
    if (t_md_node_is_valid(test_node->next)) {
      return t_context_errorf(ctx, T_ResultCode_ValidationError, test_node->next, str8_zero(), "unexpected top-level node '%S'", test_node->next->string);
    }
    ctx->test = test_node;
  }
  
  // set up general parse context
  T_ParseContext parse_ctx = { arena, ctx, suite, ctx->file_path, ctx->source };

  //
  // @build
  //
  {
    MD_Node *build = t_child_from_string(ctx->test, "build");
    if (t_md_node_is_valid(build)) {
      parse_ctx.operation = str8_lit("build");
      T_Result result = t_build_parse(&parse_ctx, build, &ctx->build);
      t_context_absorb_result(ctx, result);
    }
  }

  //
  // @artifacts
  //
  {
    MD_Node *artifacts = t_child_from_string(ctx->test, "artifacts");

    for MD_EachNode(node, artifacts->first) {

      if (t_artifact_from_name(ctx, node->string) != 0) {
        t_context_errorf(ctx, T_ResultCode_ValidationError, node, str8_zero(), "duplicate artifact '%S'", node->string);
        continue;
      }

      MD_Node *definition = &md_nil_node;
      String8 file_name = {0};
      B32 has_file_name = 0;
      for
        MD_EachNode(child, node->first)
        {
          if (str8_match(child->string, str8_lit("file_name"), StringMatchFlag_CaseInsensitive)) {
            if (has_file_name) { t_context_errorf(ctx, T_ResultCode_ValidationError, child, str8_zero(), "artifact '%S' has duplicate file_name", node->string); }
            has_file_name = 1;
            if (md_node_is_nil(child->first) || !md_node_is_nil(child->first->next) || child->first->string.size == 0) {
              t_context_errorf(ctx, T_ResultCode_ValidationError, child, str8_zero(), "artifact '%S' file_name requires one value", node->string);
            } else {
              file_name = child->first->string;
              if (str8_find_needle(file_name, 0, str8_lit("/"), 0) < file_name.size || str8_find_needle(file_name, 0, str8_lit("\\"), 0) < file_name.size ||
                  str8_find_needle(file_name, 0, str8_lit(":"), 0) < file_name.size) {
                t_context_errorf(ctx, T_ResultCode_ValidationError, child, str8_zero(), "artifact '%S' file_name must be a relative file name", node->string);
              }
            }
          } else if (md_node_is_nil(definition)) {
            definition = child;
          } else {
            definition = &md_nil_node;
            break;
          }
        }
      if (md_node_is_nil(definition)) {
        t_context_errorf(ctx, T_ResultCode_ValidationError, node, str8_zero(), "artifact '%S' must contain exactly one codec definition", node->string);
        continue;
      }
      T_Codec *codec = t_codec_from_kind(suite, definition->string);
      if (codec == 0) {
        t_context_errorf(ctx, T_ResultCode_ValidationError, definition, str8_zero(), "unknown artifact codec '%S'", definition->string);
        continue;
      }
      T_Artifact *artifact = push_array(arena, T_Artifact, 1);
      artifact->name = node->string;
      artifact->file_name = file_name;
      artifact->codec = codec;
      artifact->definition = definition;
      if (file_name.size != 0) {
        for (T_Artifact *other = ctx->first_artifact; other != 0; other = other->next) {
          if (other->file_name.size != 0 && str8_match(other->file_name, file_name, StringMatchFlag_CaseInsensitive | StringMatchFlag_SlashInsensitive)) {
            t_context_errorf(ctx, T_ResultCode_ValidationError, node, str8_zero(), "artifact file_name '%S' is already used by '%S'", file_name, other->name);
            break;
          }
        }
      }
      SLLQueuePush(ctx->first_artifact, ctx->last_artifact, artifact);
      ctx->artifact_count += 1;
    }

    for (T_Artifact *artifact = ctx->first_artifact; artifact != 0; artifact = artifact->next) {
      if (artifact->codec->validate != 0) {
        T_Result result = artifact->codec->validate(&parse_ctx, artifact);
        t_context_absorb_result(ctx, result);
      }
      artifact->state = t_result_is_ok(ctx->result) ? T_ArtifactState_Validated : T_ArtifactState_Failed;
    }
  }

  //
  // @steps
  //
  {
    MD_Node *steps = t_child_from_string(ctx->test, "steps");
    ctx->command_count = md_node_is_nil(steps) ? 0 : md_child_count_from_node(steps);
    ctx->commands = push_array(arena, T_Command, ctx->command_count);
    U64 command_index = 0;
    for MD_EachNode(node, steps->first) {
      T_OpSpec *spec = t_op_spec_from_name(suite, node->string);
      if (spec == 0) {
        t_context_errorf(ctx, T_ResultCode_ValidationError, node, node->string, "unknown operation '%S'", node->string);
        continue;
      }
      T_Command *command = &ctx->commands[command_index++];
      command->spec = spec;
      command->arguments = node;
      command->location = t_txt_pt_from_offset(ctx->source, node->src_offset);
      command->order = command_index;
      if (spec->validate != 0) {
        parse_ctx.operation = spec->name;
        T_Result result = spec->validate(&parse_ctx, node);
        t_context_absorb_result(ctx, result);
      }
    }
    ctx->command_count = command_index;
  }
  return ctx->result;
}

internal T_Result
t_script_execute(T_Context *ctx)
{
  if (!t_result_is_ok(ctx->result)) { return ctx->result; }

  for (T_Artifact *artifact = ctx->first_artifact; artifact != 0; artifact = artifact->next) {
    if (artifact->codec->encode == 0) { continue; }
    T_Result result = artifact->codec->encode(ctx, artifact);
    t_context_absorb_result(ctx, result);
    if (!t_result_is_ok(ctx->result)) {
      artifact->state = T_ArtifactState_Failed;
      return ctx->result;
    }
    artifact->state = T_ArtifactState_Materialized;
    if (artifact->file_name.size != 0 && !t_write_file(artifact->file_name, artifact->data)) {
      artifact->state = T_ArtifactState_Failed;
      return t_context_errorf(ctx, T_ResultCode_IoError, artifact->definition, str8_zero(), "unable to materialize artifact '%S'", artifact->file_name);
    }
  }

  B32 began = 0;
  if (ctx->suite->begin != 0) {
    began = 1;
    T_Result result = ctx->suite->begin(ctx, ctx->test);
    t_context_absorb_result(ctx, result);
    if (!t_result_is_ok(ctx->result)) { goto exit; }
  }
  {
    T_Result result = t_build_execute(ctx);
    t_context_absorb_result(ctx, result);
    if (!t_result_is_ok(ctx->result)) { goto exit; }
  }
  for EachIndex(i, ctx->command_count) {
    T_Command *command = &ctx->commands[i];
    T_Result result = command->spec->execute(ctx, command->arguments);
    t_context_absorb_result(ctx, result);
    if (!t_result_is_ok(ctx->result)) { break; }
  }

exit:;
  if (began && ctx->suite->end != 0) { ctx->suite->end(ctx); }
  return ctx->result;
}

internal T_Result
t_byte_producer_error(T_Context *ctx, MD_Node *node, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  T_Result result = t_context_errorfv(ctx, T_ResultCode_ValidationError, node, str8_zero(), fmt, args);
  va_end(args);
  return result;
}

internal B32
t_hex_digit_value(U8 c, U8 *value_out)
{
  if (c >= '0' && c <= '9') {
    *value_out = c - '0';
    return 1;
  }
  if (c >= 'a' && c <= 'f') {
    *value_out = c - 'a' + 10;
    return 1;
  }
  if (c >= 'A' && c <= 'F') {
    *value_out = c - 'A' + 10;
    return 1;
  }
  return 0;
}

internal T_Result
t_bytes_from_producer_list(T_Context *ctx, MD_Node *first, String8 *data_out)
{
  String8List parts = {0};
  for
    MD_EachNode(node, first)
    {
      String8 part = {0};
      T_Result result = t_bytes_from_producer(ctx, node, &part);
      if (!t_result_is_ok(result)) { return result; }
      str8_list_push(ctx->arena, &parts, part);
    }
  *data_out = str8_list_join(ctx->arena, &parts, 0);
  return ctx->result;
}

internal T_Result
t_bytes_from_producer(T_Context *ctx, MD_Node *producer, String8 *data_out)
{
  if (str8_match(producer->string, str8_lit("data"), StringMatchFlag_CaseInsensitive)) {
    if (md_node_is_nil(producer->first) || !md_node_is_nil(producer->first->next)) { return t_byte_producer_error(ctx, producer, "data must contain exactly one byte producer"); }
    return t_bytes_from_producer(ctx, producer->first, data_out);
  }

  if (str8_match(producer->string, str8_lit("concat"), StringMatchFlag_CaseInsensitive)) { return t_bytes_from_producer_list(ctx, producer->first, data_out); }

  U64 size_prefix = 0;
  if (str8_match(producer->string, str8_lit("size16le"), StringMatchFlag_CaseInsensitive)) { size_prefix = 2; }
  if (str8_match(producer->string, str8_lit("size32le"), StringMatchFlag_CaseInsensitive)) { size_prefix = 4; }
  if (size_prefix != 0) {
    if (md_node_is_nil(producer->first) || !md_node_is_nil(producer->first->next)) { return t_byte_producer_error(ctx, producer, "size prefix requires one byte producer"); }
    String8 payload = {0};
    T_Result result = t_bytes_from_producer(ctx, producer->first, &payload);
    U64 max_size = size_prefix == 2 ? max_U16 : max_U32;
    if (!t_result_is_ok(result)) { return result; }
    if (payload.size > max_size) { return t_byte_producer_error(ctx, producer, "payload is too large for size prefix"); }
    U8 *bytes = push_array_no_zero(ctx->arena, U8, size_prefix + payload.size);
    for EachIndex(i, size_prefix) { bytes[i] = (U8)(payload.size >> (i * 8)); }
    MemoryCopy(bytes + size_prefix, payload.str, payload.size);
    *data_out = str8(bytes, size_prefix + payload.size);
    return ctx->result;
  }

  if (str8_match(producer->string, str8_lit("align4"), StringMatchFlag_CaseInsensitive)) {
    if (md_node_is_nil(producer->first) || !md_node_is_nil(producer->first->next)) { return t_byte_producer_error(ctx, producer, "align4 requires one byte producer"); }
    String8 payload = {0};
    T_Result result = t_bytes_from_producer(ctx, producer->first, &payload);
    if (!t_result_is_ok(result)) { return result; }
    U64 aligned_size = AlignPow2(payload.size, 4);
    U8 *bytes = push_array(ctx->arena, U8, aligned_size);
    MemoryCopy(bytes, payload.str, payload.size);
    *data_out = str8(bytes, aligned_size);
    return ctx->result;
  }

  if (str8_match(producer->string, str8_lit("text"), StringMatchFlag_CaseInsensitive)) {
    MD_Node *value = producer->first;
    if (md_node_is_nil(value) || !md_node_is_nil(value->next)) { return t_byte_producer_error(ctx, producer, "text requires one value"); }
    *data_out = str8_copy(ctx->arena, value->string);
    return ctx->result;
  }

  if (str8_match(producer->string, str8_lit("work_path"), StringMatchFlag_CaseInsensitive)) {
    String8 file_name = t_scalar_string_from_node(producer);
    if (file_name.size == 0 || str8_find_needle(file_name, 0, str8_lit("/"), 0) < file_name.size || str8_find_needle(file_name, 0, str8_lit("\\"), 0) < file_name.size ||
        str8_find_needle(file_name, 0, str8_lit(":"), 0) < file_name.size) {
      return t_byte_producer_error(ctx, producer, "work_path requires a relative file name");
    }
    *data_out = t_make_file_path(ctx->arena, file_name);
    return ctx->result;
  }

  if (str8_match(producer->string, str8_lit("hex"), StringMatchFlag_CaseInsensitive)) {
    MD_Node *value = producer->first;
    if (md_node_is_nil(value) || !md_node_is_nil(value->next)) { return t_byte_producer_error(ctx, producer, "hex requires one string"); }
    U8 *bytes = push_array_no_zero(ctx->arena, U8, value->string.size / 2 + 1);
    U64 count = 0;
    U8 high = 0;
    B32 has_high = 0;
    for
      EachIndex(i, value->string.size)
      {
        U8 c = value->string.str[i];
        if (char_is_space(c) || c == '_') { continue; }
        U8 digit = 0;
        if (!t_hex_digit_value(c, &digit)) { return t_byte_producer_error(ctx, value, "invalid hex digit '%c'", c); }
        if (!has_high) {
          high = digit;
          has_high = 1;
        } else {
          bytes[count++] = (high << 4) | digit;
          has_high = 0;
        }
      }
    if (has_high) { return t_byte_producer_error(ctx, value, "hex producer contains an odd number of digits"); }
    *data_out = str8(bytes, count);
    return ctx->result;
  }

  if (str8_match(producer->string, str8_lit("zero"), StringMatchFlag_CaseInsensitive)) {
    U64 count = 0;
    if (!try_u64_from_str8_c_rules(t_scalar_string_from_node(producer), &count)) { return t_byte_producer_error(ctx, producer, "zero requires a byte count"); }
    *data_out = str8(push_array(ctx->arena, U8, count), count);
    return ctx->result;
  }

  if (str8_match(producer->string, str8_lit("repeat"), StringMatchFlag_CaseInsensitive)) {
    MD_Node *value_node = producer->first;
    MD_Node *count_node = md_node_is_nil(value_node) ? &md_nil_node : value_node->next;
    U64 value = 0;
    U64 count = 0;
    if (md_node_is_nil(value_node) || md_node_is_nil(count_node) || !md_node_is_nil(count_node->next) || !try_u64_from_str8_c_rules(value_node->string, &value) || value > max_U8 ||
        !try_u64_from_str8_c_rules(count_node->string, &count)) {
      return t_byte_producer_error(ctx, producer, "repeat requires a byte value and count");
    }
    U8 *bytes = push_array_no_zero(ctx->arena, U8, count);
    MemorySet(bytes, (U8)value, count);
    *data_out = str8(bytes, count);
    return ctx->result;
  }

  U64 integer_size = 0;
  if (str8_match(producer->string, str8_lit("u16le"), StringMatchFlag_CaseInsensitive)) { integer_size = 2; }
  if (str8_match(producer->string, str8_lit("u32le"), StringMatchFlag_CaseInsensitive)) { integer_size = 4; }
  if (str8_match(producer->string, str8_lit("u64le"), StringMatchFlag_CaseInsensitive)) { integer_size = 8; }
  if (integer_size != 0) {
    U64 value = 0;
    if (!try_u64_from_str8_c_rules(t_scalar_string_from_node(producer), &value) || (integer_size < 8 && value >= (1ull << (integer_size * 8)))) {
      return t_byte_producer_error(ctx, producer, "%S value is out of range", producer->string);
    }
    U8 *bytes = push_array_no_zero(ctx->arena, U8, integer_size);
    for
      EachIndex(i, integer_size) { bytes[i] = (U8)(value >> (i * 8)); }
    *data_out = str8(bytes, integer_size);
    return ctx->result;
  }

  if (md_node_is_nil(producer->first)) {
    *data_out = str8_copy(ctx->arena, producer->string);
    return ctx->result;
  }
  return t_byte_producer_error(ctx, producer, "unknown byte producer '%S'", producer->string);
}

internal MD_Node *
t_push_md_child(Arena *arena, MD_Node *parent, String8 string)
{
  MD_Node *node = md_push_node(arena, MD_NodeKind_Main, MD_NodeFlag_Identifier, string, string, 0);
  md_node_push_child(parent, node);
  return node;
}

internal T_Result
t_codec_bytes_encode(T_Context *ctx, T_Artifact *artifact)
{
  MD_Node *data = t_child_from_string(artifact->definition, "data");
  if (md_node_is_nil(data)) { return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_zero(), "bytes artifact requires data"); }
  return t_bytes_from_producer(ctx, data, &artifact->data);
}

internal T_Result
t_codec_bytes_decode(T_Context *ctx, T_Artifact *artifact, MD_Node **semantic_tree_out)
{
  String8 data = artifact->data;
  MD_Node *root = md_push_node(ctx->arena, MD_NodeKind_Main, MD_NodeFlag_Identifier, str8_lit("bytes"), str8_lit("bytes"), 0);
  MD_Node *size = t_push_md_child(ctx->arena, root, str8_lit("size"));
  t_push_md_child(ctx->arena, size, str8f(ctx->arena, "%llu", data.size));
  MD_Node *hex = t_push_md_child(ctx->arena, root, str8_lit("hex"));
  U8 *hex_data = push_array_no_zero(ctx->arena, U8, data.size * 2);
  local_persist U8 digits[] = "0123456789abcdef";
  for
    EachIndex(i, data.size)
    {
      hex_data[i * 2 + 0] = digits[data.str[i] >> 4];
      hex_data[i * 2 + 1] = digits[data.str[i] & 15];
    }
  t_push_md_child(ctx->arena, hex, str8(hex_data, data.size * 2));
  *semantic_tree_out = root;
  return ctx->result;
}

internal T_Result
t_codec_text_encode(T_Context *ctx, T_Artifact *artifact)
{
  MD_Node *data = t_child_from_string(artifact->definition, "data");
  if (md_node_is_nil(data)) { return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_zero(), "text artifact requires data"); }
  return t_bytes_from_producer(ctx, data, &artifact->data);
}

internal T_Result
t_codec_text_decode(T_Context *ctx, T_Artifact *artifact, MD_Node **semantic_tree_out)
{
  String8 data = artifact->data;
  MD_Node *root = md_push_node(ctx->arena, MD_NodeKind_Main, MD_NodeFlag_Identifier, str8_lit("text"), str8_lit("text"), 0);
  MD_Node *value = t_push_md_child(ctx->arena, root, str8_lit("data"));
  t_push_md_child(ctx->arena, value, str8_copy(ctx->arena, data));
  *semantic_tree_out = root;
  return ctx->result;
}

internal T_Result
t_op_compare_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  MD_Node *left = t_child_from_string(arguments, "left");
  MD_Node *right = t_child_from_string(arguments, "right");
  String8 left_name = t_scalar_string_from_node(left);
  String8 right_name = t_scalar_string_from_node(right);
  if (left_name.size == 0) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, arguments, "compare requires left");
  } else if (t_artifact_from_name(ctx->run, left_name) == 0) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, left, "unknown artifact '%S'", left_name);
  }
  if (right_name.size == 0) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, arguments, "compare requires right");
  } else if (t_artifact_from_name(ctx->run, right_name) == 0) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, right, "unknown artifact '%S'", right_name);
  }
  return ctx->run->result;
}

internal T_Result
t_op_compare_execute(T_Context *ctx, MD_Node *arguments)
{
  String8 left_name = t_scalar_string_from_node(t_child_from_string(arguments, "left"));
  String8 right_name = t_scalar_string_from_node(t_child_from_string(arguments, "right"));
  T_Artifact *left = t_artifact_from_name(ctx, left_name);
  T_Artifact *right = t_artifact_from_name(ctx, right_name);
  if (!str8_match(left->data, right->data, 0)) {
    U64 mismatch = 0;
    U64 common_size = Min(left->data.size, right->data.size);
    while (mismatch < common_size && left->data.str[mismatch] == right->data.str[mismatch]) { mismatch += 1; }
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("compare"), "artifacts '%S' and '%S' differ at byte %llu (sizes %llu and %llu)", left_name, right_name,
                            mismatch, left->data.size, right->data.size);
  }
  return ctx->result;
}

internal B32
t_u64_from_scalar(MD_Node *node, U64 *value_out)
{
  return try_u64_from_str8_c_rules(t_scalar_string_from_node(node), value_out);
}

internal T_Result
t_semantic_match_all(T_Context *ctx, MD_Node *expected, MD_Node *actual, MD_Node *tag, String8 path, String8 field_path)
{
  U64 dot = str8_find_needle(field_path, 0, str8_lit("."), 0);
  String8 field = str8_prefix(field_path, dot);
  String8 rest = dot < field_path.size ? str8_skip(field_path, dot + 1) : str8_zero();
  if (field.size == 0) {
    return t_context_errorf(ctx, T_ResultCode_ValidationError, tag, str8_zero(), "all requires a non-empty field path");
  }

  if (str8_match(field, str8_lit("*"), 0)) {
    for MD_EachNode(child, actual->first) {
      String8 child_path = str8f(ctx->arena, "%S.%S", path, child->string);
      if (rest.size == 0) {
        B32 matches = 0;
        for (MD_Node *allowed = tag->first->next; !md_node_is_nil(allowed); allowed = allowed->next) {
          if (str8_match(t_scalar_string_from_node(child), allowed->string, 0)) { matches = 1; break; }
        }
        if (!matches) { return t_context_errorf(ctx, T_ResultCode_Mismatch, expected, str8_zero(), "%S: value is not allowed", child_path); }
      } else {
        T_Result result = t_semantic_match_all(ctx, expected, child, tag, child_path, rest);
        if (!t_result_is_ok(result)) { return result; }
      }
    }
    return ctx->result;
  }

  MD_Node *child = md_child_from_string(actual, field, 0);
  String8 child_path = str8f(ctx->arena, "%S.%S", path, field);
  if (md_node_is_nil(child)) { return t_context_errorf(ctx, T_ResultCode_Mismatch, expected, str8_zero(), "%S: expected field is missing", child_path); }
  if (rest.size != 0) { return t_semantic_match_all(ctx, expected, child, tag, child_path, rest); }

  B32 matches = 0;
  for (MD_Node *allowed = tag->first->next; !md_node_is_nil(allowed); allowed = allowed->next) {
    if (str8_match(t_scalar_string_from_node(child), allowed->string, 0)) { matches = 1; break; }
  }
  if (!matches) { return t_context_errorf(ctx, T_ResultCode_Mismatch, expected, str8_zero(), "%S: value is not allowed", child_path); }
  return ctx->result;
}

internal T_Result
t_semantic_match_node(T_Context *ctx, MD_Node *expected, MD_Node *actual, String8 path)
{
  for MD_EachNode(tag, expected->first_tag) {
    if (str8_match(tag->string, str8_lit("exists"), StringMatchFlag_CaseInsensitive)) { continue; }
    if (str8_match(tag->string, str8_lit("count"), StringMatchFlag_CaseInsensitive)) {
      U64 count = 0;
      if (!try_u64_from_str8_c_rules(t_scalar_string_from_node(tag), &count) || md_child_count_from_node(actual) != count) {
        return t_context_errorf(ctx, T_ResultCode_Mismatch, expected, str8_zero(), "%S: child count does not match", path);
      }
      continue;
    }
    if (str8_match(tag->string, str8_lit("all"), StringMatchFlag_CaseInsensitive)) {
      MD_Node *field_path = tag->first;
      if (md_node_is_nil(field_path) || md_node_is_nil(field_path->next)) {
        return t_context_errorf(ctx, T_ResultCode_ValidationError, tag, str8_zero(), "all requires a field path and at least one allowed value");
      }
      T_Result result = t_semantic_match_all(ctx, expected, actual, tag, path, field_path->string);
      if (!t_result_is_ok(result)) { return result; }
      continue;
    }
    if (str8_match(tag->string, str8_lit("contains"), StringMatchFlag_CaseInsensitive)) {
      String8 needle = t_scalar_string_from_node(tag);
      String8 value = t_scalar_string_from_node(actual);
      if (str8_find_needle(value, 0, needle, 0) >= value.size) {
        return t_context_errorf(ctx, T_ResultCode_Mismatch, expected, str8_zero(), "%S: '%S' does not contain '%S'", path, value, needle);
      }
      continue;
    }
    if (str8_match(tag->string, str8_lit("starts_with"), StringMatchFlag_CaseInsensitive)) {
      String8 prefix = t_scalar_string_from_node(tag);
      String8 value = t_scalar_string_from_node(actual);
      if (!str8_starts_with(value, prefix)) {
        return t_context_errorf(ctx, T_ResultCode_Mismatch, expected, str8_zero(), "%S: '%S' does not start with '%S'", path, value, prefix);
      }
      continue;
    }
    if (str8_match(tag->string, str8_lit("aligned"), StringMatchFlag_CaseInsensitive)) {
      U64 align = 0;
      U64 value = 0;
      if (!try_u64_from_str8_c_rules(t_scalar_string_from_node(tag), &align) || align == 0 || !t_u64_from_scalar(actual, &value) || value % align != 0) {
        return t_context_errorf(ctx, T_ResultCode_Mismatch, expected, str8_zero(), "%S: value is not aligned", path);
      }
      continue;
    }
    if (str8_match(tag->string, str8_lit("range"), StringMatchFlag_CaseInsensitive)) {
      MD_Node *min_node = tag->first;
      MD_Node *max_node = md_node_is_nil(min_node) ? &md_nil_node : min_node->next;
      U64 min = 0, max = 0, value = 0;
      if (md_node_is_nil(max_node) || !md_node_is_nil(max_node->next) || !try_u64_from_str8_c_rules(min_node->string, &min) ||
          !try_u64_from_str8_c_rules(max_node->string, &max) || !t_u64_from_scalar(actual, &value) || value < min || value > max) {
        return t_context_errorf(ctx, T_ResultCode_Mismatch, expected, str8_zero(), "%S: value is outside expected range", path);
      }
      continue;
    }
    if (str8_match(tag->string, str8_lit("bits_set"), StringMatchFlag_CaseInsensitive) || str8_match(tag->string, str8_lit("bits_clear"), StringMatchFlag_CaseInsensitive)) {
      U64 mask = 0, value = 0;
      B32 bits_set = str8_match(tag->string, str8_lit("bits_set"), StringMatchFlag_CaseInsensitive);
      if (!try_u64_from_str8_c_rules(t_scalar_string_from_node(tag), &mask) || !t_u64_from_scalar(actual, &value) || (bits_set ? (value & mask) != mask : (value & mask) != 0)) {
        return t_context_errorf(ctx, T_ResultCode_Mismatch, expected, str8_zero(), "%S: required bits are not %s", path, bits_set ? "set" : "clear");
      }
      continue;
    }
    if (str8_match(tag->string, str8_lit("absent"), StringMatchFlag_CaseInsensitive)) { continue; }
    return t_context_errorf(ctx, T_ResultCode_ValidationError, tag, str8_zero(), "unknown semantic predicate '%S'", tag->string);
  }

  for MD_EachNode(expected_child, expected->first) {
    MD_Node *actual_child = md_child_from_string(actual, expected_child->string, 0);
    String8 child_path = path.size == 0 ? expected_child->string : str8f(ctx->arena, "%S.%S", path, expected_child->string);
    B32 expect_absent = !md_node_is_nil(md_tag_from_string(expected_child, str8_lit("absent"), StringMatchFlag_CaseInsensitive));
    if (expect_absent) {
      if (!md_node_is_nil(actual_child)) { return t_context_errorf(ctx, T_ResultCode_Mismatch, expected_child, str8_zero(), "%S: field must be absent", child_path); }
      continue;
    }
    if (md_node_is_nil(actual_child)) { return t_context_errorf(ctx, T_ResultCode_Mismatch, expected_child, str8_zero(), "%S: expected field is missing", child_path); }
    T_Result result = t_semantic_match_node(ctx, expected_child, actual_child, child_path);
    if (!t_result_is_ok(result)) { return result; }
  }

  return ctx->result;
}

internal T_Result
t_semantic_match(T_Context *ctx, MD_Node *expected, MD_Node *actual)
{
  if (!str8_match(expected->string, actual->string, 0)) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, expected, str8_zero(), "expected '%S', got '%S'", expected->string, actual->string);
  }
  return t_semantic_match_node(ctx, expected, actual, expected->string);
}
