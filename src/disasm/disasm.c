// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0xe34cd4ff

////////////////////////////////
//~ rjf: Instruction Decoding/Disassembling Type Functions

#if !defined(ZYDIS_H)
#include "third_party/zydis/zydis.h"
#include "third_party/zydis/zydis.c"
#endif

internal DASM_Inst
dasm_inst_from_code(Arena *arena, Arch arch, U64 vaddr, String8 code, DASM_Syntax syntax)
{
  DASM_Inst inst = {0};
  switch(arch)
  {
    default:{}break;
    
    //- rjf: x86/x64 disassembly
    case Arch_x86:
    case Arch_x64:
    {
      // rjf: determine zydis formatter style
      ZydisFormatterStyle style = ZYDIS_FORMATTER_STYLE_INTEL;
      switch(syntax)
      {
        default:{}break;
        case DASM_Syntax_Intel:{style = ZYDIS_FORMATTER_STYLE_INTEL;}break;
        case DASM_Syntax_ATT:  {style = ZYDIS_FORMATTER_STYLE_ATT;}break;
      }
      
      // rjf: disassemble one instruction
      ZydisDisassembledInstruction zinst = {0};
      ZyanStatus status = ZydisDisassemble(ZYDIS_MACHINE_MODE_LONG_64, vaddr, code.str, code.size, &zinst, style);
      
      // rjf: analyze
      DASM_InstFlags flags = 0;
      U64 jump_dest_vaddr = 0;
      {
        ZydisDecodedOperand *first_visible_op = (zinst.info.operand_count_visible > 0 ? &zinst.operands[0] : 0);
        ZydisDecodedOperand *first_op = (zinst.info.operand_count > 0 ? &zinst.operands[0] : 0);
        ZydisDecodedOperand *second_op = (zinst.info.operand_count > 1 ? &zinst.operands[1] : 0);
        if(first_visible_op != 0 && 
           (first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM8 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM16 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM32 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM64 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM16_32_64 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM32_32_64 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM16_32_32))
        {
          ZydisCalcAbsoluteAddress(&zinst.info, first_visible_op, vaddr, &jump_dest_vaddr);
        }
        if(first_op != 0 && second_op != 0 && first_op->type == ZYDIS_OPERAND_TYPE_REGISTER &&
           (first_op->reg.value == ZYDIS_REGISTER_RSP ||
            first_op->reg.value == ZYDIS_REGISTER_ESP ||
            first_op->reg.value == ZYDIS_REGISTER_SP))
        {
          flags |= DASM_InstFlag_ChangesStackPointer;
          if(second_op->type != ZYDIS_OPERAND_TYPE_IMMEDIATE)
          {
            flags |= DASM_InstFlag_ChangesStackPointerVariably;
          }
        }
        if(zinst.info.attributes & (ZYDIS_ATTRIB_HAS_REP|
                                    ZYDIS_ATTRIB_HAS_REPE|
                                    ZYDIS_ATTRIB_HAS_REPZ|
                                    ZYDIS_ATTRIB_HAS_REPNZ|
                                    ZYDIS_ATTRIB_HAS_REPNE))
        {
          flags |= DASM_InstFlag_Repeats;
        }
        switch(zinst.info.mnemonic)
        {
          case ZYDIS_MNEMONIC_CALL:
          {
            flags |= DASM_InstFlag_Call;
          }break;
          
          case ZYDIS_MNEMONIC_JB:
          case ZYDIS_MNEMONIC_JBE:
          case ZYDIS_MNEMONIC_JCXZ:
          case ZYDIS_MNEMONIC_JECXZ:
          case ZYDIS_MNEMONIC_JKNZD:
          case ZYDIS_MNEMONIC_JKZD:
          case ZYDIS_MNEMONIC_JL:
          case ZYDIS_MNEMONIC_JLE:
          case ZYDIS_MNEMONIC_JNB:
          case ZYDIS_MNEMONIC_JNBE:
          case ZYDIS_MNEMONIC_JNL:
          case ZYDIS_MNEMONIC_JNLE:
          case ZYDIS_MNEMONIC_JNO:
          case ZYDIS_MNEMONIC_JNP:
          case ZYDIS_MNEMONIC_JNS:
          case ZYDIS_MNEMONIC_JNZ:
          case ZYDIS_MNEMONIC_JO:
          case ZYDIS_MNEMONIC_JP:
          case ZYDIS_MNEMONIC_JRCXZ:
          case ZYDIS_MNEMONIC_JS:
          case ZYDIS_MNEMONIC_JZ:
          case ZYDIS_MNEMONIC_LOOP:
          case ZYDIS_MNEMONIC_LOOPE:
          case ZYDIS_MNEMONIC_LOOPNE:
          {
            flags |= DASM_InstFlag_Branch;
          }break;
          
          case ZYDIS_MNEMONIC_JMP:
          {
            flags |= DASM_InstFlag_UnconditionalJump;
          }break;
          
          case ZYDIS_MNEMONIC_RET:
          {
            flags |= DASM_InstFlag_Return;
          }break;
          
          case ZYDIS_MNEMONIC_PUSH:
          case ZYDIS_MNEMONIC_POP:
          {
            flags |= DASM_InstFlag_ChangesStackPointer;
          }break;
          
          default:
          {
            flags |= DASM_InstFlag_NonFlow;
          }break;
        }
      }
      
      // rjf: convert
      {
        inst.flags           = flags;
        inst.size            = zinst.info.length;
        inst.string          = push_str8_copy(arena, str8_cstring(zinst.text));
        inst.jump_dest_vaddr = jump_dest_vaddr;
      }
    }break;
  }
  return inst;
}

////////////////////////////////
//~ rjf: Control Flow Analysis

internal DASM_CtrlFlowInfo
dasm_ctrl_flow_info_from_arch_vaddr_code(Arena *arena, DASM_InstFlags exit_points_mask, Arch arch, U64 vaddr, String8 code)
{
  Temp scratch = scratch_begin(&arena, 1);
  DASM_CtrlFlowInfo info = {0};
  for(U64 offset = 0; offset < code.size;)
  {
    DASM_Inst inst = dasm_inst_from_code(scratch.arena, arch, vaddr+offset, str8_skip(code, offset), DASM_Syntax_Intel);
    U64 inst_vaddr = vaddr+offset;
    offset += inst.size;
    info.total_size += inst.size;
    if(inst.flags & exit_points_mask)
    {
      DASM_CtrlFlowPoint point = {0};
      point.inst_flags = inst.flags;
      point.vaddr = inst_vaddr;
      point.jump_dest_vaddr = inst.jump_dest_vaddr;
      DASM_CtrlFlowPointNode *node = push_array(arena, DASM_CtrlFlowPointNode, 1);
      node->v = point;
      SLLQueuePush(info.exit_points.first, info.exit_points.last, node);
      info.exit_points.count += 1;
    }
  }
  scratch_end(scratch);
  return info;
}

////////////////////////////////
//~ rjf: Parameter Type Functions

internal B32
dasm_params_match(DASM_Params *a, DASM_Params *b)
{
  B32 result = (a->vaddr == b->vaddr &&
                a->arch == b->arch &&
                a->style_flags == b->style_flags &&
                a->syntax == b->syntax &&
                a->base_vaddr == b->base_vaddr &&
                di_key_match(&a->dbgi_key, &b->dbgi_key));
  return result;
}

////////////////////////////////
//~ rjf: Line Type Functions

internal void
dasm_line_chunk_list_push(Arena *arena, DASM_LineChunkList *list, U64 cap, DASM_Line *inst)
{
  DASM_LineChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, DASM_LineChunkNode, 1);
    node->v = push_array_no_zero(arena, DASM_Line, cap);
    node->cap = cap;
    SLLQueuePush(list->first, list->last, node);
    list->node_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], inst);
  node->count += 1;
  list->line_count += 1;
}

internal DASM_LineArray
dasm_line_array_from_chunk_list(Arena *arena, DASM_LineChunkList *list)
{
  DASM_LineArray array = {0};
  array.count = list->line_count;
  array.v = push_array_no_zero(arena, DASM_Line, array.count);
  U64 idx = 0;
  for(DASM_LineChunkNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopy(array.v+idx, n->v, sizeof(DASM_Line)*n->count);
    idx += n->count;
  }
  return array;
}

internal U64
dasm_line_array_idx_from_code_off__linear_scan(DASM_LineArray *array, U64 off)
{
  U64 result = 0;
  for(U64 idx = 0; idx < array->count; idx += 1)
  {
    U64 next_off = (idx+1 < array->count ? array->v[idx+1].code_off : max_U64);
    if(array->v[idx].code_off <= off && off < next_off)
    {
      result = idx;
      if(!(array->v[idx].flags & DASM_LineFlag_Decorative))
      {
        break;
      }
    }
  }
  return result;
}

internal U64
dasm_line_array_code_off_from_idx(DASM_LineArray *array, U64 idx)
{
  U64 off = 0;
  if(idx < array->count)
  {
    off = array->v[idx].code_off;
  }
  return off;
}

////////////////////////////////
//~ rjf: Artifact Cache Hooks / Lookups

typedef struct DASM_Artifact DASM_Artifact;
struct DASM_Artifact
{
  Arena *arena;
  DASM_Info info;
};

internal AC_Artifact
dasm_artifact_create(String8 key, B32 *retry_out)
{
  DASM_Artifact *artifact = 0;
  if(lane_idx() == 0)
  {
    Temp scratch = scratch_begin(0, 0);
    Access *access = access_open();
    DI_Scope *di_scope = di_scope_open();
    
    //- rjf: unpack key
    U128 hash = {0};
    DASM_Params params = {0};
    U64 key_read_off = 0;
    key_read_off += str8_deserial_read_struct(key, key_read_off, &hash);
    key_read_off += str8_deserial_read_struct(key, key_read_off, &params);
    params.dbgi_key.path.str = key.str + key_read_off;
    String8 data = c_data_from_hash(access, hash);
    
    //- rjf: get dbg info
    B32 stale = 0;
    RDI_Parsed *rdi = &rdi_parsed_nil;
    if(params.dbgi_key.path.size != 0)
    {
      rdi = di_rdi_from_key(di_scope, &params.dbgi_key, 1, 0);
      stale = (stale || (rdi == &rdi_parsed_nil));
    }
    
    //- rjf: data * arch * addr * dbg -> decode artifacts
    DASM_LineChunkList line_list = {0};
    String8List inst_strings = {0};
    switch(params.arch)
    {
      default:{}break;
      
      //- rjf: x86/x64 decoding
      case Arch_x64:
      case Arch_x86:
      {
        // rjf: disassemble
        RDI_SourceFile *last_file = &rdi_nil_element_union.source_file;
        RDI_Line *last_line = 0;
        for(U64 off = 0; off < data.size;)
        {
          // rjf: disassemble one instruction
          DASM_Inst inst = dasm_inst_from_code(scratch.arena, params.arch, params.vaddr+off, str8_skip(data, off), params.syntax);
          if(inst.size == 0)
          {
            break;
          }
          
          // rjf: push strings derived from voff -> line info
          if(params.style_flags & (DASM_StyleFlag_SourceFilesNames|DASM_StyleFlag_SourceLines) &&
             rdi != &rdi_parsed_nil)
          {
            U64 voff = (params.vaddr+off) - params.base_vaddr;
            U32 unit_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_UnitVMap, voff);
            RDI_Unit *unit = rdi_element_from_name_idx(rdi, Units, unit_idx);
            RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, unit->line_table_idx);
            RDI_ParsedLineTable unit_line_info = {0};
            rdi_parsed_from_line_table(rdi, line_table, &unit_line_info);
            U64 line_info_idx = rdi_line_info_idx_from_voff(&unit_line_info, voff);
            if(line_info_idx < unit_line_info.count)
            {
              RDI_Line *line = &unit_line_info.lines[line_info_idx];
              RDI_SourceFile *file = rdi_element_from_name_idx(rdi, SourceFiles, line->file_idx);
              String8 file_normalized_full_path = {0};
              file_normalized_full_path.str = rdi_string_from_idx(rdi, file->normal_full_path_string_idx, &file_normalized_full_path.size);
              if(file != last_file)
              {
                if(params.style_flags & DASM_StyleFlag_SourceFilesNames &&
                   file->normal_full_path_string_idx != 0 && file_normalized_full_path.size != 0)
                {
                  String8 inst_string = push_str8f(scratch.arena, "> %S", file_normalized_full_path);
                  DASM_Line inst = {u32_from_u64_saturate(off), DASM_LineFlag_Decorative, 0, r1u64(inst_strings.total_size + inst_strings.node_count,
                                                                                                   inst_strings.total_size + inst_strings.node_count + inst_string.size)};
                  dasm_line_chunk_list_push(scratch.arena, &line_list, 1024, &inst);
                  str8_list_push(scratch.arena, &inst_strings, inst_string);
                }
                if(params.style_flags & DASM_StyleFlag_SourceFilesNames && file->normal_full_path_string_idx == 0)
                {
                  String8 inst_string = str8_lit(">");
                  DASM_Line inst = {u32_from_u64_saturate(off), DASM_LineFlag_Decorative, 0, r1u64(inst_strings.total_size + inst_strings.node_count,
                                                                                                   inst_strings.total_size + inst_strings.node_count + inst_string.size)};
                  dasm_line_chunk_list_push(scratch.arena, &line_list, 1024, &inst);
                  str8_list_push(scratch.arena, &inst_strings, inst_string);
                }
                last_file = file;
              }
              if(line && line != last_line && file->normal_full_path_string_idx != 0 &&
                 params.style_flags & DASM_StyleFlag_SourceLines &&
                 file_normalized_full_path.size != 0)
              {
                FileProperties props = os_properties_from_file_path(file_normalized_full_path);
                if(props.modified != 0)
                {
                  // TODO(rjf): need redirection path - this may map to a different path on the local machine,
                  // need frontend to communicate path remapping info to this layer
                  C_Key key = fs_key_from_path_range_new(file_normalized_full_path, r1u64(0, max_U64), 0);
                  TXT_LangKind lang_kind = txt_lang_kind_from_extension(file_normalized_full_path);
                  U64 endt_us = max_U64;
                  U128 hash = {0};
                  TXT_TextInfo text_info = txt_text_info_from_key_lang(access, key, lang_kind, &hash);
                  stale = (stale || u128_match(hash, u128_zero()));
                  if(0 < line->line_num && line->line_num < text_info.lines_count)
                  {
                    String8 data = c_data_from_hash(access, hash);
                    String8 line_text = str8_skip_chop_whitespace(str8_substr(data, text_info.lines_ranges[line->line_num-1]));
                    if(line_text.size != 0)
                    {
                      String8 inst_string = push_str8f(scratch.arena, "> %S", line_text);
                      DASM_Line inst = {u32_from_u64_saturate(off), DASM_LineFlag_Decorative, 0, r1u64(inst_strings.total_size + inst_strings.node_count,
                                                                                                       inst_strings.total_size + inst_strings.node_count + inst_string.size)};
                      dasm_line_chunk_list_push(scratch.arena, &line_list, 1024, &inst);
                      str8_list_push(scratch.arena, &inst_strings, inst_string);
                    }
                  }
                }
                last_line = line;
              }
            }
          }
          
          // rjf: push line
          String8 addr_part = {0};
          if(params.style_flags & DASM_StyleFlag_Addresses)
          {
            addr_part = push_str8f(scratch.arena, "%s0x%016I64x  ", rdi != &rdi_parsed_nil ? "  " : "", params.vaddr+off);
          }
          String8 code_bytes_part = {0};
          if(params.style_flags & DASM_StyleFlag_CodeBytes)
          {
            String8List code_bytes_strings = {0};
            str8_list_push(scratch.arena, &code_bytes_strings, str8_lit("{"));
            for(U64 byte_idx = 0; byte_idx < inst.size || byte_idx < 16; byte_idx += 1)
            {
              if(byte_idx < inst.size)
              {
                str8_list_pushf(scratch.arena, &code_bytes_strings, "%02x%s ", (U32)data.str[off+byte_idx], byte_idx == inst.size-1 ? "}" : "");
              }
              else if(byte_idx < 8)
              {
                str8_list_push(scratch.arena, &code_bytes_strings, str8_lit("   "));
              }
            }
            str8_list_push(scratch.arena, &code_bytes_strings, str8_lit(" "));
            code_bytes_part = str8_list_join(scratch.arena, &code_bytes_strings, 0);
          }
          String8 symbol_part = {0};
          if(inst.jump_dest_vaddr != 0 && rdi != &rdi_parsed_nil && params.style_flags & DASM_StyleFlag_SymbolNames)
          {
            RDI_U32 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, inst.jump_dest_vaddr-params.base_vaddr);
            if(scope_idx != 0)
            {
              RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
              RDI_U32 procedure_idx = scope->proc_idx;
              RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, procedure_idx);
              String8 procedure_name = {0};
              procedure_name.str = rdi_string_from_idx(rdi, procedure->name_string_idx, &procedure_name.size);
              if(procedure_name.size != 0)
              {
                symbol_part = push_str8f(scratch.arena, " (%S)", procedure_name);
              }
            }
          }
          String8 inst_string = push_str8f(scratch.arena, "%S%S%S%S", addr_part, code_bytes_part, inst.string, symbol_part);
          DASM_Line line = {u32_from_u64_saturate(off), 0, inst.jump_dest_vaddr, r1u64(inst_strings.total_size + inst_strings.node_count,
                                                                                       inst_strings.total_size + inst_strings.node_count + inst_string.size)};
          dasm_line_chunk_list_push(scratch.arena, &line_list, 1024, &line);
          str8_list_push(scratch.arena, &inst_strings, inst_string);
          
          // rjf: increment
          off += inst.size;
        }
      }break;
    }
    
    //- rjf: artifacts -> value bundle
    Arena *info_arena = 0;
    DASM_Info info = {0};
    if(!stale)
    {
      //- rjf: produce joined text
      Arena *text_arena = arena_alloc();
      StringJoin text_join = {0};
      text_join.sep = str8_lit("\n");
      String8 text = str8_list_join(text_arena, &inst_strings, &text_join);
      
      //- rjf: produce unique key for this disassembly's text
      C_Key text_key = c_key_make(c_root_alloc(), c_id_make(0, 0));
      
      //- rjf: submit text data to hash store
      U128 text_hash = c_submit_data(text_key, &text_arena, text);
      
      //- rjf: produce value bundle
      info_arena = arena_alloc();
      info.text_key = text_key;
      info.lines = dasm_line_array_from_chunk_list(info_arena, &line_list);
    }
    
    //- rjf: if stale, retry
    if(stale)
    {
      retry_out[0] = 1;
    }
    
    //- rjf: fill result
    if(info_arena != 0)
    {
      artifact = push_array(info_arena, DASM_Artifact, 1);
      artifact->arena = info_arena;
      artifact->info = info;
    }
    
    di_scope_close(di_scope);
    access_close(access);
    scratch_end(scratch);
  }
  lane_sync_u64(&artifact, 0);
  AC_Artifact result = {0};
  result.u64[0] = (U64)artifact;
  return result;
}

internal void
dasm_artifact_destroy(AC_Artifact artifact)
{
  DASM_Artifact *dasm_artifact = (DASM_Artifact *)artifact.u64[0];
  if(dasm_artifact == 0) { return; }
  c_close_key(dasm_artifact->info.text_key);
  arena_release(dasm_artifact->arena);
}

internal DASM_Info
dasm_info_from_hash_params(Access *access, U128 hash, DASM_Params *params)
{
  DASM_Info info = {0};
  {
    Temp scratch = scratch_begin(0, 0);
    
    // rjf: form key
    String8List key_parts = {0};
    str8_list_push(scratch.arena, &key_parts, str8_struct(&hash));
    str8_list_push(scratch.arena, &key_parts, str8_struct(params));
    str8_list_push(scratch.arena, &key_parts, params->dbgi_key.path);
    String8 key = str8_list_join(scratch.arena, &key_parts, 0);
    
    // rjf: get info
    AC_Artifact artifact = ac_artifact_from_key(access, key, dasm_artifact_create, dasm_artifact_destroy, 0, .gen = fs_change_gen());
    DASM_Artifact *dasm_artifact = (DASM_Artifact *)artifact.u64[0];
    if(dasm_artifact)
    {
      info = dasm_artifact->info;
    }
    
    scratch_end(scratch);
  }
  return info;
}

internal DASM_Info
dasm_info_from_key_params(Access *access, C_Key key, DASM_Params *params, U128 *hash_out)
{
  DASM_Info result = {0};
  for(U64 rewind_idx = 0; rewind_idx < C_KEY_HASH_HISTORY_COUNT; rewind_idx += 1)
  {
    U128 hash = c_hash_from_key(key, rewind_idx);
    result = dasm_info_from_hash_params(access, hash, params);
    if(result.lines.count != 0)
    {
      if(hash_out)
      {
        *hash_out = hash;
      }
      break;
    }
  }
  return result;
}
