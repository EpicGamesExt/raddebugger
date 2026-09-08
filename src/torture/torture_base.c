////////////////////////////////
// Generic Parser Helpers

internal B32
t_md_node_is_valid(MD_Node *node)
{
  return !md_node_is_nil(node);
}

internal B32
t_bool_is_string_true(String8 string)
{
  return str8_matchi(string, str8_lit("true")) || str8_matchi(string, str8_lit("1"));
}

internal B32
t_bool_is_string_false(String8 string)
{
  return str8_matchi(string, str8_lit("false")) || str8_matchi(string, str8_lit("0"));
}

internal B32
t_bool_from_string(String8 string)
{
  if (str8_matchi(string, str8_lit("false"))) {
    return 0;
  }
  return (B32)u32_from_str8(string, 10);
}

internal String8
t_hex_from_data(Arena *arena, String8 data)
{
  U64 hex_size = data.size * 2;
  U8 *hex      = push_array_no_zero(arena, U8, hex_size);
  for EachIndex(i, data.size) {
    local_persist U8 digits[] = "0123456789abcdef";
    hex[i * 2 + 0] = digits[data.str[i] >> 4];
    hex[i * 2 + 1] = digits[data.str[i] & 15];
  }
  return str8(hex, hex_size);
}

////////////////////////////////
// Codec Helpers

internal MD_Node *
t_codec_child(MD_Node *node, char *name)
{
  return md_child_from_string(node, str8_cstring(name), StringMatchFlag_CaseInsensitive);
}

internal String8
t_codec_scalar(MD_Node *node)
{
  return md_node_is_nil(node) || md_node_is_nil(node->first) ? str8_zero() : node->first->string;
}

internal MD_Node *
t_codec_push_node(Arena *arena, MD_Node *parent, String8 string)
{
  MD_Node *node = md_push_node(arena, MD_NodeKind_Main, MD_NodeFlag_Identifier, string, string, 0);
  if (parent != 0) { md_node_push_child(parent, node); }
  return node;
}

internal MD_Node *
t_codec_push_field(T_Context *ctx, MD_Node *parent, char *name, String8 value)
{
  MD_Node *field = t_codec_push_node(ctx->arena, parent, str8_cstring(name));
  t_codec_push_node(ctx->arena, field, str8_copy(ctx->arena, value));
  return field;
}

internal MD_Node *
t_codec_push_u64(T_Context *ctx, MD_Node *parent, char *name, U64 value)
{
  return t_codec_push_field(ctx, parent, name, str8f(ctx->arena, "%llu", value));
}

internal MD_Node *
t_codec_push_u64_s8(T_Context *ctx, MD_Node *parent, String8 name, U64 value)
{
  MD_Node *field = t_codec_push_node(ctx->arena, parent, name);
  t_codec_push_node(ctx->arena, field, str8f(ctx->arena, "%llu", value));
  return field;
}

internal B32
t_bool_from_scalar(MD_Node *node, B32 *value_out)
{
  String8 value = t_codec_scalar(node);

  if (t_bool_is_string_true(value)) {
    *value_out = 1;
    return 1;
  }

  if (t_bool_is_string_false(value)) {
    *value_out = 0;
    return 1;
  }

  return 0;
}


