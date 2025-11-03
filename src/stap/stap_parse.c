// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
stap_is_scale_valid(U64 scale)
{
  return IsPow2(scale) && scale <= 8;
}

internal U64
stap_parse_digit(U8 *begin, U8 *end, U64 *digit_out)
{
  U8 *ptr = begin;
  if (ptr < end && *ptr == '-') {
    ptr += 1;
  }
  for (;ptr < end; ptr += 1) {
    if (!char_is_digit(*ptr, 10)) { break; }
  }
  String8 digit_str = str8(begin, (U64)(ptr - begin));
  if (digit_out) {
    *digit_out = u64_from_str8(digit_str, 10);
  }
  return digit_str.size;
}

internal U64
stap_skip_whitespace(U8 *begin, U8 *end)
{
  U8 *ptr = begin;
  for (; ptr < end && *ptr == ' '; ptr += 1);
  U64 size = (U64)(ptr - begin);
  return size;
}

internal U64
stap_parse_ident(U8 *begin, U8 *end, String8 *ident_out)
{
  U8 *ptr = begin;
  for (; ptr < end; ptr += 1) {
    if (!char_is_alpha(*ptr) && !char_is_digit(*ptr, 10)) { break; }
  }
  String8 ident = str8(begin, (U64)(ptr - begin));
  if (ident_out) { *ident_out = ident; }
  return ident.size;
}

internal U64
stap_size_from_arg(String8 string)
{
  U64 tag_sep      = str8_find_needle(string, 0, str8_lit("@"), 0);
  U64 next_tag_sep = str8_find_needle(string, tag_sep + 1, str8_lit("@"), 0);
  U64 arg_end      = str8_find_needle_reverse(string, string.size - next_tag_sep, str8_lit(" "), 0);
  return arg_end == 0 ? string.size : arg_end;
}

internal String8List
stap_list_from_string(Arena *arena, String8 string)
{
  String8List result = {0};
  for (U64 cursor = 0, arg_size; cursor < string.size; cursor += arg_size) {
    arg_size = stap_size_from_arg(str8_skip(string, cursor));
    String8 arg = str8_substr(string, r1u64(cursor, cursor + arg_size));
    str8_list_push(arena, &result, arg);
  }
  return result;
}

internal String8
stap_parse_args_x64(String8 string, STAP_Arg *arg_out)
{
  String8 error = str8_lit("unknown parse error");
  U64 sep_pos = str8_find_needle(string, 0, str8_lit("@"), 0);
  if (sep_pos < string.size) {
    STAP_Arg arg = {0};

    String8 tag  = str8_prefix(string, sep_pos + 1);
    String8 oper = str8_skip(string, tag.size);

    STAP_ArgValueType arg_value_type       = STAP_ArgValueType_Null;
    U64               arg_value_size       = 0;
    B32               infer_arg_value_size = 1;
    U64               tag_size             = 0;
    {
      U8 *ptr = tag.str, *end = tag.str + tag.size;

      // is signed?
      if (ptr >= end) { goto operand_parse_exit; }
      if (*ptr == '-') {
        arg_value_type = STAP_ArgValueType_S;
        ptr += 1;
      }

      // parse optional size
      if (ptr >= end) { goto operand_parse_exit; }
      if (char_is_digit(*ptr, 10)) {
        arg_value_size = *ptr - '0';
        if (!stap_is_scale_valid(arg_value_size)) {
          error = str8_lit("invalid operand size");
          goto operand_parse_exit;
        }
        ptr += 1;
        infer_arg_value_size = 0;
      }

      // is this float?
      if (ptr >= end) { goto operand_parse_exit; }
      if (*ptr == 'f') {
        if (arg_value_type != STAP_ArgValueType_Null) {
          error = str8_lit("illegal combination of modifiers '-' and 'f'");
          goto operand_parse_exit;
        }
        ptr += 1;
        arg_value_type = STAP_ArgValueType_F;
      }

      // assume default value type to be unsigned
      if (arg_value_type == STAP_ArgValueType_Null) {
        arg_value_type = STAP_ArgValueType_U;
      }

      if (ptr >= end) { goto operand_parse_exit; }
      if (*ptr != '@') {
        error = str8_lit("failed to find @");
        goto operand_parse_exit;
      }
      ptr += 1;

      tag_size = (U64)(ptr - tag.str);
    }

    U64 memory_ref_start = str8_find_needle_reverse(oper, 0, str8_lit("("), 0);
    // memory operand
    if (memory_ref_start > 0 && memory_ref_start < oper.size) {
      //
      // Expected syntax for memory operand: { [Disp] (Base [',' Index [',' Scale]]) } | { $IMM } | { %reg }
      //
      //  Disp:  { [+|-]Digits } | { Expr }
      //  Base:  %reg
      //  Index: %reg
      //  Scale: 1|2|4|8
      //  Expr:  A small C-like infix expression
      //

      // find closing ')'
      U64 memory_ref_end = str8_find_needle(oper, memory_ref_start+1, str8_lit(")"), 0);
      if (memory_ref_end > oper.size) {
        error = str8_lit("missing closing ')'");
        goto operand_parse_exit;
      }
      if (memory_ref_end >= oper.size) {
        error = str8_lit("expression does not terminate after ')'");
        goto operand_parse_exit;
      }

      // parse displacement
      String8 disp_str = str8_skip_chop_whitespace(str8_prefix(oper, memory_ref_start-1));
      if (disp_str.size > 0 && !str8_is_integer_signed(disp_str, 10)) {
        error = str8_lit("displacement is too complicated, only integral displacement is supported");
        goto operand_parse_exit;
      }
      S64 disp = s64_from_str8(disp_str, 10);

      // parse base, index, scale
      String8 base_index_scale = str8_skip_chop_whitespace(str8_substr(oper, r1u64(memory_ref_start, memory_ref_end)));
      U64     first_comma      = str8_find_needle(base_index_scale, 0, str8_lit(","), 0);
      U64     second_comma     = str8_find_needle(base_index_scale, first_comma+1, str8_lit(","), 0);
      String8 base_str         = str8_skip_chop_whitespace(str8_substr(base_index_scale, r1u64(0, first_comma)));
      String8 index_str        = str8_skip_chop_whitespace(str8_substr(base_index_scale, r1u64(first_comma + 1, second_comma)));
      String8 scale_str        = str8_skip_chop_whitespace(str8_substr(base_index_scale, r1u64(second_comma + 1, memory_ref_end)));

      // syntax check
      if(first_comma >= base_index_scale.size && base_str.size == 0) {
        error = str8_lit("missing base register");
        goto operand_parse_exit;
      }
      if (first_comma < base_index_scale.size && index_str.size == 0) {
        error = str8_lit("missing index register");
        goto operand_parse_exit;
      }
      if (second_comma < base_index_scale.size && scale_str.size == 0) {
        error = str8_lit("missing scale");
        goto operand_parse_exit;
      }
      if (base_str.size > 0 && !str8_match(str8_lit("%"), base_str, StringMatchFlag_RightSideSloppy)) {
        error = str8_lit("invalid base register");
        goto operand_parse_exit;
      }
      if (first_comma < base_index_scale.size && (index_str.size < 2 || !str8_match(str8_lit("%"), index_str, StringMatchFlag_RightSideSloppy))) {
        error = str8_lit("invalid index register");
        goto operand_parse_exit;
      }
      if (second_comma < base_index_scale.size && (scale_str.size > 0 && !str8_is_integer(scale_str, 10))) {
        error = str8_lit("invalid of scale (expected unsigned integral type)");
        goto operand_parse_exit;
      }

      // stip '%' prefix
      base_str  = str8_skip(base_str, 1);
      index_str = str8_skip(index_str, 1);

      // parse base
      B8  base_is_alias = 0;
      U32 base_reg_code = 0;
      if (base_str.size) {
        base_reg_code = regs_reg_code_from_name(Arch_x64, base_str);
        if (base_reg_code == 0) {
          base_reg_code = regs_alias_code_from_name(Arch_x64, base_str);
          base_is_alias = base_reg_code != 0;
        }
        if (base_reg_code == 0) {
          error = str8_lit("unknown base register");
          goto operand_parse_exit;
        }
      }

      // parse index
      B8  index_is_alias = 0;
      U32 index_reg_code = 0;
      if (index_str.size) {
        index_reg_code = regs_reg_code_from_name(Arch_x64, index_str);
        if (index_reg_code == 0) {
          index_reg_code = regs_alias_code_from_name(Arch_x64, index_str);
          index_is_alias = index_reg_code != 0;
        }
        if (index_reg_code == 0) {
          error = str8_lit("unknown index register");
        }
      }
      
      // parse scale
      U64 scale = 1;
      if (scale_str.size) {
        scale = u64_from_str8(scale_str, 10);
        if (!stap_is_scale_valid(scale)) {
          error = str8_lit("invalid scale value (expected 1/2/4/8)");
          goto operand_parse_exit;
        }
      }

      if (infer_arg_value_size) {
        error = str8_lit("memory operands must have a sice");
        goto operand_parse_exit;
      }

      // fill out memory ref portion
      arg.type                      = STAP_ArgType_MemoryRef;
      arg.memory_ref.disp           = disp;
      arg.memory_ref.base.reg_code  = base_reg_code;
      arg.memory_ref.base.is_alias  = base_is_alias;
      arg.memory_ref.index.reg_code = index_reg_code;
      arg.memory_ref.index.is_alias = index_is_alias;
      arg.memory_ref.scale          = scale;
    }
    // $imm
    else if (str8_match(str8_lit("$"), oper, StringMatchFlag_RightSideSloppy)) {
      String8 imm_str = str8_skip(oper, 1);

      U64 imm_size = str8_find_needle(imm_str, 0, str8_lit(" "), 0);
      if (imm_size > imm_str.size ) {
        imm_size = imm_str.size;
      }
      imm_str = str8_prefix(imm_str, imm_size);

      if (imm_str.size == 0) {
        goto operand_parse_exit;
      }

      U64 imm       = 0;
      B32 is_parsed = 0;
      switch (arg_value_type) {
      case STAP_ArgValueType_Null:
      case STAP_ArgValueType_U:
      case STAP_ArgValueType_F: {
        is_parsed = try_u64_from_str8_c_rules(imm_str, &imm);
      } break;
      case STAP_ArgValueType_S: {
        is_parsed = try_s64_from_str8_c_rules(imm_str, (S64 *)&imm);
      } break;
      default: { InvalidPath; } break;
      }

      if (!is_parsed) {
        error = str8_lit("failed to parse immediate");
        goto operand_parse_exit;
      }

      arg.type = STAP_ArgType_Imm;
      arg.imm  = imm;
    }
    // %reg
    else if (str8_match(str8_lit("%"), oper, StringMatchFlag_RightSideSloppy)) {
      // skip % and strip white space
      String8 reg_str = str8_skip_chop_whitespace(str8_skip(oper, 1));
      if (reg_str.size == 0) {
        goto operand_parse_exit;
      }

      B8  is_reg_alias = 0;
      U32 reg_code     = regs_reg_code_from_name(Arch_x64, reg_str);
      if (reg_code == 0) {
        reg_code = regs_alias_code_from_name(Arch_x64, reg_str);
        is_reg_alias = reg_code != 0;
      }
      if (reg_code == 0) {
        error = str8_lit("invalid register name");
        goto operand_parse_exit;
      }

      arg.type         = STAP_ArgType_Reg;
      arg.reg.is_alias = is_reg_alias;
      arg.reg.reg_code = reg_code;
    } else {
      goto operand_parse_exit;
    }

    // fill out value portion
    arg.value_type = arg_value_type == STAP_ArgValueType_Null ? STAP_ArgValueType_U : arg_value_type;
    arg.value_size = arg_value_size;

    // write output
    if (arg_out) {
      *arg_out = arg;
    }

    // clear error tracker
    error = str8_zero();

operand_parse_exit:;
  } else {
    error = str8_lit("invalid argument string");
  }

  return error;
}

internal STAP_ArgArray
stap_arg_array_from_string(Arena *arena, Arch arch, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List arg_strings = stap_list_from_string(scratch.arena, string);
  STAP_ArgArray result = {0};
  result.v = push_array(arena, STAP_Arg, arg_strings.node_count);
  for EachNode(n, String8Node, arg_strings.first) {
    STAP_Arg *arg = &result.v[result.count++];

    switch (arch) {
    case Arch_Null: {} break;
    case Arch_x64: {
      stap_parse_args_x64(n->string, arg);
    } break;
    case Arch_x86:
    case Arch_arm32:
    case Arch_arm64: { NotImplemented; } break;
    default: { InvalidPath; } break;
    }
  }
  scratch_end(scratch);
  return result;
}

internal B32
stap_read_arg(STAP_Arg         arg,
              Arch             arch,
              void            *reg_block,
              STAP_MemoryRead *memory_read,
              void            *memory_read_ctx,
              void            *raw_value)
{
  AssertAlways(arg.value_size <= 8);

  B32 is_value_read = 0;

  switch (arg.type) {
  case STAP_ArgType_Null: break;
  case STAP_ArgType_Imm: {
    MemoryCopy(raw_value, &arg.imm, arg.value_size);
    is_value_read = 1;
  } break;
  case STAP_ArgType_Reg: {
    Rng1U64 range     = regs_range_from_code(arch, arg.reg.is_alias, arg.reg.reg_code);
    U64     copy_size = Min(arg.value_size, dim_1u64(range));
    MemoryCopy(raw_value, (U8 *)reg_block + range.min, copy_size);
    is_value_read = 1;
  } break;
  case STAP_ArgType_MemoryRef: {
    U64 base = 0;
    if(arg.memory_ref.base.reg_code) {
      Rng1U64 range     = regs_range_from_code(arch, arg.memory_ref.base.is_alias, arg.memory_ref.base.reg_code);
      U64     copy_size = Min(sizeof(base), dim_1u64(range));
      MemoryCopy(&base, (U8 *)reg_block + range.min, copy_size);
    }

    U64 index = 0;
    if(arg.memory_ref.index.reg_code) {
      Rng1U64 range     = regs_range_from_code(arch, arg.memory_ref.index.is_alias, arg.memory_ref.index.reg_code);
      U64     copy_size = Min(sizeof(base), dim_1u64(range));
      MemoryCopy(&index, (U8 *)reg_block + range.min, copy_size);
    }

    U64 addr = arg.memory_ref.disp + (base + index * arg.memory_ref.scale);
    if (memory_read(addr, raw_value, arg.value_size, memory_read_ctx)) {
      is_value_read = 1;
    }
  } break;
  default: { InvalidPath; } break;
  }

  if (arg.value_size < 8) {
    if (arg.value_type == STAP_ArgValueType_S) {
      *(U64 *)raw_value = extend_sign64(*(U64 *)raw_value, arg.value_size);
    } else if (arg.value_type == STAP_ArgValueType_F) {
      Assert(arg.value_size == 4);
      *(F64 *)raw_value = *(F32 *)raw_value;
    }
  }

  return is_value_read;
}

internal B32
stap_read_arg_u(STAP_Arg arg, Arch arch, void *reg_block, STAP_MemoryRead *memory_read, void *memory_read_ctx, U64 *u_out)
{
  return stap_read_arg(arg, arch, reg_block, memory_read, memory_read_ctx, u_out);
}

internal B32
stap_read_arg_s(STAP_Arg arg, Arch arch, void *reg_block, STAP_MemoryRead *memory_read, void *memory_read_ctx, S64 *s_out)
{
  return stap_read_arg(arg, arch, reg_block, memory_read, memory_read_ctx, s_out);
}

internal B32
stap_read_arg_f(STAP_Arg arg, Arch arch, void *reg_block, STAP_MemoryRead *memory_read, void *memory_read_ctx, F64 *f_out)
{
  return stap_read_arg(arg, arch, reg_block, memory_read, memory_read_ctx, f_out);
}

