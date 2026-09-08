// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

global T_OpSpec t_op_repeat = {str8_lit_comp("repeat"), t_op_repeat_validate, t_op_repeat_execute};
global T_OpSpec t_op_compare_file = {str8_lit_comp("compare_file"), t_op_compare_file_validate, t_op_compare_file_execute};

internal B32
t_build_string_is_bool(String8 string)
{
  return t_bool_from_string(string) || str8_matchi(string, str8_lit("false")) || str8_match(string, str8_lit("0"), 0);
}

internal B32
t_build_safe_relative_path(String8 path)
{
  if (path.size == 0 || path.str[0] == '/' || path.str[0] == '\\' || str8_find_needle(path, 0, str8_lit(":"), 0) < path.size) { return 0; }
  for (U64 off = 0; off < path.size;) {
    U64 end = off;
    while (end < path.size && path.str[end] != '/' && path.str[end] != '\\') { end += 1; }
    String8 part = str8_substr(path, rng_1u64(off, end));
    if (str8_match(part, str8_lit(".."), 0)) { return 0; }
    off = end + 1;
  }
  return 1;
}

internal T_BuildCommandKind
t_build_command_kind_from_string(String8 string)
{
  if (str8_matchi(string, str8_lit("compile"))) { return T_BuildCommandKind_Compile; }
  if (str8_matchi(string, str8_lit("compile_link"))) { return T_BuildCommandKind_CompileLink; }
  if (str8_matchi(string, str8_lit("link"))) { return T_BuildCommandKind_Link; }
  if (str8_matchi(string, str8_lit("resource"))) { return T_BuildCommandKind_Resource; }
  if (str8_matchi(string, str8_lit("run"))) { return T_BuildCommandKind_Run; }
  if (str8_matchi(string, str8_lit("copy"))) { return T_BuildCommandKind_Copy; }
  return T_BuildCommandKind_Null;
}

internal String8
t_build_default_output(Arena *arena, String8 target_name, String8 target_kind, OperatingSystem os)
{
  if (target_name.size == 0) { target_name = str8_lit("main"); }
  if (str8_matchi(target_kind, str8_lit("shared_library"))) {
    return os == OperatingSystem_Windows ? str8f(arena, "%S.dll", target_name) : str8f(arena, "lib%S.so", target_name);
  }
  return os == OperatingSystem_Windows ? str8f(arena, "%S.exe", target_name) : str8_copy(arena, target_name);
}

internal void
t_build_declaration_list_parse(Arena *arena, MD_Node *container, T_BuildDeclarationList *list)
{
  for MD_EachNode(node, container->first) {
    T_BuildDeclaration *declaration = push_array(arena, T_BuildDeclaration, 1);
    declaration->name = node->string;
    declaration->definition = node;
    SLLQueuePush(list->first, list->last, declaration);
    list->count += 1;
  }
}

internal T_BuildOutput *
t_build_output_push(Arena *arena, T_BuildVariant *variant, String8 name, String8 path, MD_Node *definition)
{
  T_BuildOutput *output = push_array(arena, T_BuildOutput, 1);
  output->name = str8_copy(arena, name);
  output->path = str8_copy(arena, path);
  output->definition = definition;
  SLLQueuePush(variant->first_output, variant->last_output, output);
  variant->output_count += 1;
  return output;
}

internal T_BuildOutput *
t_build_primary_output(T_BuildVariant *variant)
{
  for (T_BuildOutput *output = variant->first_output; output != 0; output = output->next) {
    if (str8_matchi(output->name, str8_lit("executable"))) { return output; }
  }
  return variant->first_output;
}

internal String8
t_build_command_scalar(MD_Node *command)
{
  if (md_node_is_nil(command->first) || !md_node_is_nil(command->first->first)) { return str8_zero(); }
  return command->first->string;
}

internal T_BuildCommand *
t_build_command_parse(T_ParseContext *ctx, MD_Node *node)
{
  T_BuildCommandKind kind = t_build_command_kind_from_string(node->string);
  if (kind == T_BuildCommandKind_Null) { return 0; }

  T_BuildCommand *command = push_array(ctx->arena, T_BuildCommand, 1);
  command->kind = kind;
  command->definition = node;
  command->repeat_count = 1;
  command->timeout_ms = max_U64;
  command->expected_exit = str8_lit("0");
  command->output_mode = T_BuildOutputMode_Default;

  String8 scalar = t_build_command_scalar(node);
  if (scalar.size != 0) {
    command->arguments = scalar;
    command->inject_output = 1;
    return command;
  }

  char *allowed[] = {"args", "tool", "output", "artifact", "expect_exit", "when_previous_exit", "timeout_ms", "stdout_matches", "stderr_matches", "index", "repeat", "parallel", "produces", 0};
  for MD_EachNode(field, node->first) {
    B32 is_allowed = 0;
    for (U64 i = 0; allowed[i] != 0; i += 1) { is_allowed |= str8_matchi(field->string, str8_cstring(allowed[i])); }
    if (!is_allowed) { t_parse_errorf(ctx, T_ResultCode_ValidationError, field, "unknown build command field '%S'", field->string); }
    for (MD_Node *other = field->next; !md_node_is_nil(other); other = other->next) {
      if (str8_matchi(field->string, other->string)) {
        t_parse_errorf(ctx, T_ResultCode_ValidationError, other, "duplicate build command field '%S'", other->string);
        break;
      }
    }
  }

  MD_Node *args = t_child_from_string(node, "args");
  command->arguments = t_scalar_string_from_node(args);
  command->tool = t_scalar_string_from_node(t_child_from_string(node, "tool"));
  MD_Node *output = t_child_from_string(node, "output");
  if (!md_node_is_nil(output)) {
    command->output = t_scalar_string_from_node(output);
    command->output_mode = str8_matchi(command->output, str8_lit("none")) ? T_BuildOutputMode_None : T_BuildOutputMode_Explicit;
    command->inject_output = command->output_mode != T_BuildOutputMode_None;
  }
  command->artifact = t_scalar_string_from_node(t_child_from_string(node, "artifact"));
  if (command->artifact.size != 0 && command->output_mode != T_BuildOutputMode_Default) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "build command cannot contain both artifact and output");
  }
  command->expected_exit = t_scalar_string_from_node(t_child_from_string(node, "expect_exit"));
  if (command->expected_exit.size == 0) { command->expected_exit = str8_lit("0"); }
  command->when_previous_exit = t_scalar_string_from_node(t_child_from_string(node, "when_previous_exit"));
  command->stdout_pattern = t_scalar_string_from_node(t_child_from_string(node, "stdout_matches"));
  command->stderr_pattern = t_scalar_string_from_node(t_child_from_string(node, "stderr_matches"));
  command->index_name = t_scalar_string_from_node(t_child_from_string(node, "index"));
  MD_Node *parallel = t_child_from_string(node, "parallel");
  String8 parallel_string = t_scalar_string_from_node(parallel);
  if (!md_node_is_nil(parallel) && !t_build_string_is_bool(parallel_string)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, parallel, "build command parallel must be true or false");
  }
  command->parallel = t_bool_from_string(parallel_string);
  command->produces = t_child_from_string(node, "produces");

  MD_Node *repeat = t_child_from_string(node, "repeat");
  if (!md_node_is_nil(repeat) && !try_u64_from_str8_c_rules(t_scalar_string_from_node(repeat), &command->repeat_count)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, repeat, "build command repeat must be an integer");
  }
  MD_Node *timeout = t_child_from_string(node, "timeout_ms");
  if (!md_node_is_nil(timeout) && !try_u64_from_str8_c_rules(t_scalar_string_from_node(timeout), &command->timeout_ms)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, timeout, "build command timeout_ms must be an integer");
  }
  if (command->arguments.size == 0 && kind != T_BuildCommandKind_Copy) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "build command '%S' requires args", node->string);
  }
  U64 expected_exit_code = 0;
  if (!str8_matchi(command->expected_exit, str8_lit("any")) && !str8_matchi(command->expected_exit, str8_lit("nonzero")) &&
      !try_u64_from_str8_c_rules(command->expected_exit, &expected_exit_code)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_child_from_string(node, "expect_exit"), "invalid expected build command exit code");
  }
  if (command->when_previous_exit.size != 0 && !str8_matchi(command->when_previous_exit, str8_lit("any")) &&
      !str8_matchi(command->when_previous_exit, str8_lit("nonzero")) && !try_u64_from_str8_c_rules(command->when_previous_exit, &expected_exit_code)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_child_from_string(node, "when_previous_exit"), "invalid previous build command exit code");
  }
  if (command->tool.size != 0 && !str8_matchi(command->tool, str8_lit("cl")) && !str8_matchi(command->tool, str8_lit("msvc")) &&
      !str8_matchi(command->tool, str8_lit("clang")) && !str8_matchi(command->tool, str8_lit("gcc")) && !str8_matchi(command->tool, str8_lit("radlink"))) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_child_from_string(node, "tool"), "unknown build command tool '%S'", command->tool);
  }
  if (command->repeat_count == 0) { t_parse_errorf(ctx, T_ResultCode_ValidationError, repeat, "build command repeat must be greater than zero"); }
  if (command->repeat_count > 1 && command->index_name.size == 0) { command->index_name = str8_lit("index"); }
  return command;
}

internal T_BuildVariant *
t_build_variant_parse(T_ParseContext *ctx, MD_Node *body, OperatingSystem os, String8 target_name, String8 target_kind)
{
  T_BuildVariant *variant = push_array(ctx->arena, T_BuildVariant, 1);
  variant->os = os;
  variant->definition = body;

  MD_Node *output = t_child_from_string(body, "output");
  MD_Node *outputs = t_child_from_string(body, "outputs");
  if (!md_node_is_nil(output) && !md_node_is_nil(outputs)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, output, "build variant cannot contain both output and outputs");
  } else if (!md_node_is_nil(output)) {
    String8 path = t_scalar_string_from_node(output);
    if (str8_matchi(path, str8_lit("none"))) { variant->output_none = 1; }
    else if (path.size == 0) { t_parse_errorf(ctx, T_ResultCode_ValidationError, output, "build output requires one path"); }
    else { t_build_output_push(ctx->arena, variant, str8_lit("executable"), path, output); }
  } else if (!md_node_is_nil(outputs)) {
    for MD_EachNode(node, outputs->first) {
      String8 path = t_scalar_string_from_node(node);
      if (path.size == 0) { t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "build output '%S' requires one path", node->string); }
      else { t_build_output_push(ctx->arena, variant, node->string, path, node); }
    }
  } else {
    OperatingSystem output_os = os == OperatingSystem_Null ? OperatingSystem_CURRENT : os;
    t_build_output_push(ctx->arena, variant, str8_lit("executable"), t_build_default_output(ctx->arena, target_name, target_kind, output_os), body);
  }

  MD_Node *commands = t_child_from_string(body, "commands");
  MD_Node *first = md_node_is_nil(commands) ? body->first : commands->first;
  for MD_EachNode(node, first) {
    T_BuildCommand *command = t_build_command_parse(ctx, node);
    if (command != 0) {
      SLLQueuePush(variant->first_command, variant->last_command, command);
      variant->command_count += 1;
    } else if (!md_node_is_nil(commands)) {
      t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "unknown build command '%S'", node->string);
    } else {
      char *metadata[] = {"output", "outputs", "kind", "source", "sources", "inputs", "depends", "generator", "resources", "packages", "defines", "link_args", "platforms", 0};
      B32 is_metadata = 0;
      for (U64 i = 0; metadata[i] != 0; i += 1) { is_metadata |= str8_matchi(node->string, str8_cstring(metadata[i])); }
      if (!is_metadata) { t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "unknown build declaration '%S'", node->string); }
    }
  }
  return variant;
}

internal void
t_build_target_parse_variants(T_ParseContext *ctx, T_BuildTarget *target, MD_Node *body)
{
  B32 has_platform = 0;
  for MD_EachNode(node, body->first) {
    OperatingSystem os = operating_system_from_string(node->string);
    has_platform |= os == OperatingSystem_Windows || os == OperatingSystem_Linux;
  }

  if (has_platform) {
    B32 has_windows = 0;
    B32 has_linux = 0;
    for MD_EachNode(node, body->first) {
      OperatingSystem os = operating_system_from_string(node->string);
      if (os == OperatingSystem_Windows || os == OperatingSystem_Linux) {
        B32 *seen = os == OperatingSystem_Windows ? &has_windows : &has_linux;
        if (*seen) { t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "duplicate build platform '%S'", node->string); }
        *seen = 1;
        T_BuildVariant *variant = t_build_variant_parse(ctx, node, os, target->name, target->kind);
        SLLQueuePush(target->first_variant, target->last_variant, variant);
        target->variant_count += 1;
      } else {
        char *metadata[] = {"kind", "source", "sources", "inputs", "depends", "generator", "resources", "packages", "defines", "link_args", 0};
        B32 is_metadata = 0;
        for (U64 i = 0; metadata[i] != 0; i += 1) { is_metadata |= str8_matchi(node->string, str8_cstring(metadata[i])); }
        if (!is_metadata) { t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "unknown build declaration '%S'", node->string); }
      }
    }
  } else {
    T_BuildVariant *variant = t_build_variant_parse(ctx, body, OperatingSystem_Null, target->name, target->kind);
    SLLQueuePush(target->first_variant, target->last_variant, variant);
    target->variant_count += 1;
  }
}

internal T_BuildTarget *
t_build_target_parse(T_ParseContext *ctx, MD_Node *node)
{
  T_BuildTarget *target = push_array(ctx->arena, T_BuildTarget, 1);
  target->name = node->string;
  target->definition = node;
  target->kind = t_scalar_string_from_node(t_child_from_string(node, "kind"));
  if (target->kind.size == 0) { target->kind = str8_lit("executable"); }
  t_build_target_parse_variants(ctx, target, node);
  return target;
}

internal T_Result
t_build_parse(T_ParseContext *ctx, MD_Node *build, T_BuildPlan **plan_out)
{
  T_BuildPlan *plan = push_array(ctx->arena, T_BuildPlan, 1);
  plan->definition = build;
  *plan_out = plan;

  MD_Node *targets = t_child_from_string(build, "targets");
  if (md_node_is_nil(targets)) {

    T_BuildTarget *target = push_array(ctx->arena, T_BuildTarget, 1);
    target->name       = str8_lit("main");
    target->kind       = str8_lit("executable");
    target->definition = build;

    t_build_target_parse_variants(ctx, target, build);

    plan->is_single_target = 1;
    SLLQueuePush(plan->first_target, plan->last_target, target);
    plan->target_count = 1;

    return ctx->run->result;
  }

  char *allowed[] = {"defaults", "values", "configurations", "features", "toolchains", "packages", "resources", "generators", "targets", 0};
  for MD_EachNode(node, build->first) {
    B32 is_allowed = 0;
    for (U64 i = 0; allowed[i] != 0; i += 1) { is_allowed |= str8_matchi(node->string, str8_cstring(allowed[i])); }
    if (!is_allowed) { t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "unknown project build declaration '%S'", node->string); }
  }

  plan->defaults = t_child_from_string(build, "defaults");
  t_build_declaration_list_parse(ctx->arena, t_child_from_string(build, "values"), &plan->values);
  t_build_declaration_list_parse(ctx->arena, t_child_from_string(build, "configurations"), &plan->configurations);
  t_build_declaration_list_parse(ctx->arena, t_child_from_string(build, "features"), &plan->features);
  t_build_declaration_list_parse(ctx->arena, t_child_from_string(build, "toolchains"), &plan->toolchains);
  t_build_declaration_list_parse(ctx->arena, t_child_from_string(build, "packages"), &plan->packages);
  t_build_declaration_list_parse(ctx->arena, t_child_from_string(build, "resources"), &plan->resources);
  t_build_declaration_list_parse(ctx->arena, t_child_from_string(build, "generators"), &plan->generators);
  for MD_EachNode(node, targets->first) {
    T_BuildTarget *target = t_build_target_parse(ctx, node);
    SLLQueuePush(plan->first_target, plan->last_target, target);
    plan->target_count += 1;
  }
  if (plan->target_count == 0) { t_parse_errorf(ctx, T_ResultCode_ValidationError, targets, "project build requires at least one target"); }
  return ctx->run->result;
}

internal T_BuildVariant *
t_build_current_variant(T_BuildTarget *target)
{
  T_BuildVariant *fallback = 0;
  for (T_BuildVariant *variant = target->first_variant; variant != 0; variant = variant->next) {
    if (variant->os == OperatingSystem_CURRENT) { return variant; }
    if (variant->os == OperatingSystem_Null) { fallback = variant; }
  }
  return fallback;
}

internal String8
t_script_expand(T_Context *ctx, MD_Node *node, String8 string, T_BuildVariant *variant, T_ScriptBinding *bindings)
{
  (void)node;
  if (variant == 0 && ctx->build != 0 && ctx->build->first_target != 0) { variant = t_build_current_variant(ctx->build->first_target); }
  Temp scratch = scratch_begin(&ctx->arena, 1);
  HashMap variables = {0};
  for (T_ScriptBinding *binding = bindings != 0 ? bindings : ctx->bindings; binding != 0; binding = binding->next) {
    lnk_env_var_push(scratch.arena, &variables, binding->name, binding->value, LNK_EnvVarRule_Current);
  }
  lnk_env_var_push(scratch.arena, &variables, str8_lit("source_dir"), t_src_path(), LNK_EnvVarRule_Current);
  lnk_env_var_push(scratch.arena, &variables, str8_lit("work_dir"), g_wdir, LNK_EnvVarRule_Current);
  lnk_env_var_push(scratch.arena, &variables, str8_lit("platform"), lower_from_str8(scratch.arena, string_from_operating_system(OperatingSystem_CURRENT)), LNK_EnvVarRule_Current);
  if (variant != 0) {
    T_BuildOutput *primary = t_build_primary_output(variant);
    if (primary != 0) {
      lnk_env_var_push(scratch.arena, &variables, str8_lit("output"), primary->path, LNK_EnvVarRule_Current);
      lnk_env_var_push(scratch.arena, &variables, str8_lit("build.output"), primary->path, LNK_EnvVarRule_Current);
    }
    for (T_BuildOutput *output = variant->first_output; output != 0; output = output->next) {
      lnk_env_var_push(scratch.arena, &variables, str8f(scratch.arena, "outputs.%S", output->name), output->path, LNK_EnvVarRule_Current);
      lnk_env_var_push(scratch.arena, &variables, str8f(scratch.arena, "build.outputs.%S", output->name), output->path, LNK_EnvVarRule_Current);
    }
  }
  if (ctx->build != 0) {
    for (T_BuildDeclaration *value = ctx->build->values.first; value != 0; value = value->next) {
      lnk_env_var_push(scratch.arena, &variables, str8f(scratch.arena, "values.%S", value->name), t_scalar_string_from_node(value->definition), LNK_EnvVarRule_Current);
    }
  }
  String8 result = lnk_expand_env_vars_windows(ctx->arena, &variables, string);
  scratch_end(scratch);
  return result;
}

internal String8
t_build_command_output(T_Context *ctx, T_BuildVariant *variant, T_BuildCommand *command, T_ScriptBinding *binding)
{
  if (command->output_mode == T_BuildOutputMode_None) { return str8_zero(); }
  if (command->output_mode == T_BuildOutputMode_Explicit) {
    return t_script_expand(ctx, command->definition, command->output, variant, binding);
  }
  if (command->artifact.size != 0) {
    T_Artifact *artifact = t_artifact_from_name(ctx, command->artifact);
    if (artifact == 0 || artifact->file_name.size == 0) {
      t_context_errorf(ctx, T_ResultCode_ValidationError, command->definition, str8_lit("build"), "unknown output artifact '%S'", command->artifact);
      return str8_zero();
    }
    return artifact->file_name;
  }
  if (!command->inject_output) { return str8_zero(); }
  T_BuildOutput *output = t_build_primary_output(variant);
  return output == 0 ? str8_zero() : t_script_expand(ctx, output->definition, output->path, variant, binding);
}

internal String8
t_build_command_arguments(T_Context *ctx, T_BuildVariant *variant, T_BuildCommand *command, T_ScriptBinding *binding)
{
  String8 args = t_script_expand(ctx, command->definition, command->arguments, variant, binding);
  String8 output = t_build_command_output(ctx, variant, command, binding);
  if (output.size == 0 || !command->inject_output) { return args; }
  OperatingSystem os = variant->os == OperatingSystem_Null ? OperatingSystem_CURRENT : variant->os;
  B32 msvc_style = os == OperatingSystem_Windows && (command->tool.size == 0 || str8_matchi(command->tool, str8_lit("cl")) || str8_matchi(command->tool, str8_lit("msvc")));
  switch (command->kind) {
    case T_BuildCommandKind_Compile:
      return msvc_style ? str8f(ctx->arena, "%S /c /Fo:\"%S\"", args, output) : str8f(ctx->arena, "%S -c -o \"%S\"", args, output);
    case T_BuildCommandKind_CompileLink:
      return msvc_style ? str8f(ctx->arena, "%S /Fe:\"%S\"", args, output) : str8f(ctx->arena, "%S -o \"%S\"", args, output);
    case T_BuildCommandKind_Link:
      return str8f(ctx->arena, "%S /out:\"%S\"", args, output);
    default: break;
  }
  return args;
}

internal String8
t_build_command_tool(T_BuildVariant *variant, T_BuildCommand *command)
{
  if (command->tool.size != 0) {
    if (str8_matchi(command->tool, str8_lit("cl")) || str8_matchi(command->tool, str8_lit("msvc"))) { return t_cl_path(); }
    if (str8_matchi(command->tool, str8_lit("clang"))) { return t_clang_path(); }
    if (str8_matchi(command->tool, str8_lit("gcc"))) { return t_gcc_path(); }
    if (str8_matchi(command->tool, str8_lit("radlink"))) { return t_radlink_path(); }
    return str8_zero();
  }
  switch (command->kind) {
    case T_BuildCommandKind_Compile:
    case T_BuildCommandKind_CompileLink: return OperatingSystem_CURRENT == OperatingSystem_Windows ? t_cl_path() : t_clang_path();
    case T_BuildCommandKind_Link: return t_radlink_path();
    default: break;
  }
  return str8_zero();
}

internal B32
t_build_exit_matches(String8 expected, U64 actual)
{
  if (str8_matchi(expected, str8_lit("any"))) { return 1; }
  if (str8_matchi(expected, str8_lit("nonzero"))) { return actual != 0; }
  U64 value = 0;
  return try_u64_from_str8_c_rules(expected, &value) && value == actual;
}

internal T_Result
t_build_load_artifact_output(T_Context *ctx, String8 output_path)
{
  for (T_Artifact *artifact = ctx->first_artifact; artifact != 0; artifact = artifact->next) {
    if (artifact->file_name.size != 0 && str8_match(artifact->file_name, output_path, StringMatchFlag_CaseInsensitive | StringMatchFlag_SlashInsensitive)) {
      artifact->data = t_read_file(ctx->arena, output_path);
      if (artifact->data.size == 0) {
        artifact->state = T_ArtifactState_Failed;
        return t_context_errorf(ctx, T_ResultCode_IoError, artifact->definition, str8_lit("build"), "unable to read output artifact '%S'", output_path);
      }
      artifact->state = T_ArtifactState_Materialized;
    }
  }
  return ctx->result;
}

internal T_Result
t_build_validate_command_result(T_Context *ctx, T_BuildVariant *variant, T_BuildCommand *command, T_ScriptBinding *binding, U64 exit_code, String8 stdout_data, String8 stderr_data)
{
  if (!t_build_exit_matches(command->expected_exit, exit_code)) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, command->definition, str8_lit("build"), "build command exited with %llu, expected %S\n%S", exit_code,
                            command->expected_exit, stderr_data);
  }
  StringMatchFlags flags = StringMatchFlag_CaseInsensitive | StringMatchFlag_SlashInsensitive;
  if (command->stdout_pattern.size != 0 && !str8_match_wildcard(stdout_data, command->stdout_pattern, flags)) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, command->definition, str8_lit("build"), "stdout does not match '%S'\n%S", command->stdout_pattern, stdout_data);
  }
  if (command->stderr_pattern.size != 0 && !str8_match_wildcard(stderr_data, command->stderr_pattern, flags)) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, command->definition, str8_lit("build"), "stderr does not match '%S'\n%S", command->stderr_pattern, stderr_data);
  }
  String8 output = t_build_command_output(ctx, variant, command, binding);
  if (output.size != 0 && exit_code == 0) {
    String8 full_path = t_make_file_path(ctx->arena, output);
    if (!file_path_exists(full_path)) { return t_context_errorf(ctx, T_ResultCode_IoError, command->definition, str8_lit("build"), "expected output '%S' was not produced", output); }
    T_Result result = t_build_load_artifact_output(ctx, output);
    if (!t_result_is_ok(result)) { return result; }
  }
  MD_Node *first_produced = command->produces == 0 ? &md_nil_node : command->produces->first;
  for MD_EachNode(produced, first_produced) {
    String8 path = t_script_expand(ctx, produced, produced->string, variant, binding);
    if (!file_path_exists(t_make_file_path(ctx->arena, path))) {
      return t_context_errorf(ctx, T_ResultCode_IoError, produced, str8_lit("build"), "expected output '%S' was not produced", path);
    }
  }
  return ctx->result;
}

internal void
t_build_remove_command_outputs(T_Context *ctx, T_BuildVariant *variant, T_BuildCommand *command, T_ScriptBinding *binding)
{
  String8 output = t_build_command_output(ctx, variant, command, binding);
  if (output.size != 0) {
    if (!t_build_safe_relative_path(output)) {
      t_context_errorf(ctx, T_ResultCode_ValidationError, command->definition, str8_lit("build"), "output path '%S' must be work-relative", output);
      return;
    }
    t_delete_file(output);
  }
  MD_Node *first_produced = command->produces == 0 ? &md_nil_node : command->produces->first;
  for MD_EachNode(produced, first_produced) {
    String8 path = t_script_expand(ctx, produced, produced->string, variant, binding);
    if (path.size != 0) {
      if (!t_build_safe_relative_path(path)) {
        t_context_errorf(ctx, T_ResultCode_ValidationError, produced, str8_lit("build"), "produced path '%S' must be work-relative", path);
        return;
      }
      t_delete_file(path);
    }
  }
}

internal T_Result
t_build_execute_command(T_Context *ctx, T_BuildVariant *variant, T_BuildCommand *command, U64 index, U64 *exit_code_out)
{
  T_ScriptBinding binding = {0};
  if (command->repeat_count > 1) {
    binding.name = command->index_name;
    binding.value = str8f(ctx->arena, "%llu", index);
  }
  t_build_remove_command_outputs(ctx, variant, command, &binding);
  if (!t_result_is_ok(ctx->result)) { return ctx->result; }

  String8 tool = t_build_command_tool(variant, command);
  if (tool.size == 0) { return t_context_errorf(ctx, T_ResultCode_ValidationError, command->definition, str8_lit("build"), "build command is not executable yet"); }
  String8 args = t_build_command_arguments(ctx, variant, command, &binding);
  U64 timeout_us = command->timeout_ms == max_U64 || command->timeout_ms > max_U64 / 1000 ? max_U64 : command->timeout_ms * 1000;
  if (!t_invoke(tool, args, timeout_us)) { return t_context_errorf(ctx, T_ResultCode_IoError, command->definition, str8_lit("build"), "unable to launch '%S'", tool); }
  *exit_code_out = g_last_exit_code;
  if (g_last_exit_code == max_U64) { return t_context_errorf(ctx, T_ResultCode_Mismatch, command->definition, str8_lit("build"), "build command did not exit before timeout"); }
  return t_build_validate_command_result(ctx, variant, command, &binding, g_last_exit_code, g_output, g_errors);
}

internal T_Result
t_build_execute_parallel(T_Context *ctx, T_BuildVariant *variant, T_BuildCommand *command, U64 *exit_code_out)
{
  U64              count       = command->repeat_count;
  T_Controller    *controllers = push_array(ctx->arena, T_Controller, count);
  T_ScriptBinding *bindings    = push_array(ctx->arena, T_ScriptBinding, count);

  B32 launched_all = 1;
  for EachIndex(index, count) {
    bindings[index].name  = command->index_name;
    bindings[index].value = str8f(ctx->arena, "%llu", index);
    t_build_remove_command_outputs(ctx, variant, command, &bindings[index]);
  }

  if (!t_result_is_ok(ctx->result)) { return ctx->result; }

  for EachIndex(index, count) {
    String8     tool     = t_build_command_tool(variant, command);
    String8     args     = t_build_command_arguments(ctx, variant, command, &bindings[index]);
    String8List cmd_line = lnk_arg_list_parse_windows_rules(ctx->arena, args);
    str8_list_push_front(ctx->arena, &cmd_line, tool);
    
    T_ControllerLaunchParams params = {
      .cmd_line    = cmd_line,
      .path        = g_wdir,
      .inherit_env = 1,
      .consoleless = 1,
    };

    if (t_controller_launch(ctx->arena, &controllers[index], &params).code != T_ControllerResultCode_Ok) {
      launched_all = 0;
      break;
    }
  }

  if (!launched_all) {
    for EachIndex(index, count) { t_controller_close(&controllers[index]); }
    return t_context_errorf(ctx, T_ResultCode_IoError, command->definition, str8_lit("build"), "unable to launch parallel build command");
  }

  U64 timeout_us = command->timeout_ms == max_U64 || command->timeout_ms > max_U64 / 1000 ? max_U64 : command->timeout_ms * 1000;
  U64 endt_us    = timeout_us == max_U64 ? max_U64 : now_time_us() + timeout_us;
  B32 timed_out  = 0;

  for (;;) {

    U64 exited_count = 0;

    for EachIndex(index, count) {
      T_ControllerResult result = t_controller_pump(&controllers[index]);
      if (result.code != T_ControllerResultCode_Ok) {
        for EachIndex(close_index, count) {
          t_controller_close(&controllers[close_index]);
        }

        return t_context_errorf(ctx,
                                T_ResultCode_IoError,
                                command->definition,
                                str8_lit("build"),
                                "unable to capture parallel build command output");
      }
      exited_count += controllers[index].exited;
    }

    if (exited_count == count) { break; }

    // TODO: comprehensive solution to the waiting problem
    if (endt_us != max_U64 && now_time_us() >= endt_us) {
      timed_out = 1;
      break;
    }
    sleep_ms(1);
  }

  for EachIndex(index, count) { t_controller_close(&controllers[index]); }

  if (timed_out) {
    return t_context_errorf(ctx,
                            T_ResultCode_Mismatch,
                            command->definition,
                            str8_lit("build"),
                            "parallel build command did not exit before timeout");
  }

  *exit_code_out = 0;
  for EachIndex(index, count) {
    if (controllers[index].exit_code != 0) { *exit_code_out = controllers[index].exit_code; }
    T_Result result = t_build_validate_command_result(ctx,
                                                       variant,
                                                       command,
                                                       &bindings[index],
                                                       controllers[index].exit_code,
                                                       t_controller_stdout(ctx->arena, &controllers[index]),
                                                       t_controller_stderr(ctx->arena, &controllers[index]));
    if (!t_result_is_ok(result)) { return result; }
  }

  return ctx->result;
}

internal T_Result
t_build_execute(T_Context *ctx)
{
  if (ctx->build == 0 || ctx->build->first_target == 0) { return ctx->result; }

  if (!ctx->build->is_single_target) {
    return t_context_errorf(ctx,
                            T_ResultCode_ValidationError,
                            ctx->build->definition,
                            str8_lit("build"),
                            "project build declarations require target selection or a build-script codec");
  }

  T_BuildTarget  *target  = ctx->build->first_target;
  T_BuildVariant *variant = t_build_current_variant(target);

  if (variant == 0) {
    return t_context_errorf(ctx,
                            T_ResultCode_ValidationError,
                            target->definition,
                            str8_lit("build"),
                            "build target has no variant for the current platform");
  }

  U64 previous_exit = max_U64;
  B32 has_previous_exit = 0;
  for EachNode(command, T_BuildCommand, variant->first_command) {
    if (command->when_previous_exit.size != 0) {
      if (!has_previous_exit) {
        return t_context_errorf(ctx, T_ResultCode_ValidationError, command->definition, str8_lit("build"), "conditional build command has no previous command");
      }
      if (!t_build_exit_matches(command->when_previous_exit, previous_exit)) { continue; }
    }
    if (command->parallel && command->repeat_count > 1) {
      T_Result result = t_build_execute_parallel(ctx, variant, command, &previous_exit);
      if ( ! t_result_is_ok(result)) { return result; }
      has_previous_exit = 1;
      continue;
    }

    for EachIndex(index, command->repeat_count) {
      T_Result result = t_build_execute_command(ctx, variant, command, index, &previous_exit);
      if ( ! t_result_is_ok(result)) { return result; }
      has_previous_exit = 1;
    }
  }

  return ctx->result;
}

internal T_Result
t_op_repeat_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  MD_Node *count = t_child_from_string(arguments, "count");
  MD_Node *steps = t_child_from_string(arguments, "steps");
  U64 value = 0;
  if (md_node_is_nil(count) || !try_u64_from_str8_c_rules(t_scalar_string_from_node(count), &value) || value == 0) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, count, "repeat requires a positive integer count");
  }
  if (md_node_is_nil(steps)) { t_parse_errorf(ctx, T_ResultCode_ValidationError, arguments, "repeat requires steps"); }
  MD_Node *index = t_child_from_string(arguments, "index");
  if (!md_node_is_nil(index) && t_scalar_string_from_node(index).size == 0) { t_parse_errorf(ctx, T_ResultCode_ValidationError, index, "repeat index requires a name"); }
  for MD_EachNode(node, steps->first) {
    T_OpSpec *spec = t_op_spec_from_name(ctx->suite, node->string);
    if (spec == 0) { t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "unknown repeated operation '%S'", node->string); }
    else if (spec->validate != 0) {
      String8 saved = ctx->operation;
      ctx->operation = spec->name;
      T_Result result = spec->validate(ctx, node);
      t_context_absorb_result(ctx->run, result);
      ctx->operation = saved;
    }
  }
  return ctx->run->result;
}

internal T_Result
t_op_repeat_execute(T_Context *ctx, MD_Node *arguments)
{
  U64 count = 0;
  try_u64_from_str8_c_rules(t_scalar_string_from_node(t_child_from_string(arguments, "count")), &count);
  String8 index_name = t_scalar_string_from_node(t_child_from_string(arguments, "index"));
  if (index_name.size == 0) { index_name = str8_lit("index"); }
  MD_Node *steps = t_child_from_string(arguments, "steps");
  for (U64 index = 0; index < count; index += 1) {
    T_ScriptBinding binding = {.next = ctx->bindings, .name = index_name, .value = str8f(ctx->arena, "%llu", index)};
    T_ScriptBinding *saved = ctx->bindings;
    ctx->bindings = &binding;
    for MD_EachNode(node, steps->first) {
      T_OpSpec *spec = t_op_spec_from_name(ctx->suite, node->string);
      T_Result result = spec->execute(ctx, node);
      t_context_absorb_result(ctx, result);
      if (!t_result_is_ok(ctx->result)) { break; }
    }
    ctx->bindings = saved;
    if (!t_result_is_ok(ctx->result)) { break; }
  }
  return ctx->result;
}

internal T_Result
t_op_compare_file_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  String8 left = t_scalar_string_from_node(t_child_from_string(arguments, "left"));
  String8 right = t_scalar_string_from_node(t_child_from_string(arguments, "right"));
  if (left.size == 0) { t_parse_errorf(ctx, T_ResultCode_ValidationError, arguments, "compare_file requires left"); }
  if (right.size == 0) { t_parse_errorf(ctx, T_ResultCode_ValidationError, arguments, "compare_file requires right"); }
  return ctx->run->result;
}

internal T_Result
t_op_compare_file_execute(T_Context *ctx, MD_Node *arguments)
{
  String8 left = t_script_expand(ctx, arguments, t_scalar_string_from_node(t_child_from_string(arguments, "left")), 0, 0);
  String8 right = t_script_expand(ctx, arguments, t_scalar_string_from_node(t_child_from_string(arguments, "right")), 0, 0);
  if (!t_build_safe_relative_path(left) || !t_build_safe_relative_path(right)) {
    return t_context_errorf(ctx, T_ResultCode_ValidationError, arguments, str8_lit("compare_file"), "paths must be work-relative");
  }
  String8 left_path = t_make_file_path(ctx->arena, left);
  String8 right_path = t_make_file_path(ctx->arena, right);
  if (!file_path_exists(left_path)) { return t_context_errorf(ctx, T_ResultCode_IoError, arguments, str8_lit("compare_file"), "file '%S' does not exist", left); }
  if (!file_path_exists(right_path)) { return t_context_errorf(ctx, T_ResultCode_IoError, arguments, str8_lit("compare_file"), "file '%S' does not exist", right); }
  String8 left_data = t_read_file(ctx->arena, left);
  String8 right_data = t_read_file(ctx->arena, right);
  if (!str8_match(left_data, right_data, 0)) {
    U64 mismatch = 0;
    U64 common_size = Min(left_data.size, right_data.size);
    while (mismatch < common_size && left_data.str[mismatch] == right_data.str[mismatch]) { mismatch += 1; }
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("compare_file"), "files '%S' and '%S' differ at byte %llu (sizes %llu and %llu)", left, right,
                            mismatch, left_data.size, right_data.size);
  }
  return ctx->result;
}
