// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
lnk_error_obj(LNK_ErrorCode code, LNK_Obj *obj, char *fmt, ...)
{
  va_list args; va_start(args, fmt);
  String8 obj_path = obj ? obj->path : str8_lit("RADLINK");
  String8 lib_path = obj ? obj->lib_path : str8_zero();
  lnk_error_with_loc_fv(code, obj_path, lib_path, fmt, args);
  va_end(args);
}

internal void
lnk_error_multiply_defined_symbol(LNK_Obj *defn_obj, LNK_Obj *conf_obj, String8 symbol_name)
{
  lnk_error_obj(LNK_Error_MultiplyDefinedSymbol, defn_obj, "symbol %S is multiply defined in %S", symbol_name, conf_obj->path);
}

////////////////////////////////

internal LNK_ObjNodeArray
lnk_obj_list_reserve(Arena *arena, LNK_ObjList *list, U64 count)
{
  LNK_ObjNodeArray arr = {0};
  if (count) {
    arr.count = count;
    arr.v = push_array(arena, LNK_ObjNode, count);
    for (LNK_ObjNode *ptr = arr.v, *opl = arr.v + arr.count; ptr < opl; ++ptr) {
      SLLQueuePush(list->first, list->last, ptr);
    }
    list->count += count;
  } else {
    MemoryZeroStruct(&arr);
  }
  
  return arr;
}

internal
THREAD_POOL_TASK_FUNC(lnk_obj_initer)
{
  LNK_ObjIniter *task    = raw_task;
  LNK_InputObj  *input   = task->inputs[task_id];
  LNK_Obj       *obj     = &task->objs.v[task_id].data;
  U64            obj_idx = task->obj_id_base + task_id;

  //
  // parse obj header
  //
  COFF_FileHeaderInfo coff_info = coff_file_header_info_from_data(input->data);

  //
  // set & check machine compatibility
  //
  {
    if (task->machine == COFF_MachineType_Unknown) {
      ins_atomic_u32_eval_assign(&task->machine, coff_info.machine);
    }

    if (coff_info.machine != COFF_MachineType_Unknown && task->machine != coff_info.machine) {
      lnk_error_with_loc(LNK_Error_IncompatibleMachine, input->path, input->lib_path,
          "conflicting machine types expected %S but got %S",
          coff_string_from_machine_type(task->machine),
          coff_string_from_machine_type(coff_info.machine));
    }
  }

  //
  // extract COFF info
  //
  String8 raw_coff_section_table = str8_substr(input->data, coff_info.section_table_range);
  String8 raw_coff_symbol_table  = str8_substr(input->data, coff_info.symbol_table_range);
  String8 raw_coff_string_table  = str8_substr(input->data, coff_info.string_table_range);

  //
  // error check: section table / symbol table / string table
  //
  if (raw_coff_section_table.size != dim_1u64(coff_info.section_table_range)) {
    lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "corrupted file, unable to read section header table");
  }
  if (raw_coff_symbol_table.size != dim_1u64(coff_info.symbol_table_range)) {
    lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "corrupted file, unable to read symbol table");
  }
  if (raw_coff_string_table.size != dim_1u64(coff_info.string_table_range)) {
    lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "corrupted file, unable to read string table");
  }

  //
  // error check section headers
  //
  COFF_SectionHeader *coff_section_table = (COFF_SectionHeader *)raw_coff_section_table.str;
  for (U64 sect_idx = 0; sect_idx < coff_info.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *coff_sect_header = &coff_section_table[sect_idx];

    // read name
    String8 sect_name = coff_name_from_section_header(raw_coff_string_table, coff_sect_header);

    if (~coff_sect_header->flags & COFF_SectionFlag_CntUninitializedData) {
      if (coff_sect_header->fsize > 0) {
        Rng1U64 sect_range = rng_1u64(coff_sect_header->foff, coff_sect_header->foff + coff_sect_header->fsize);

        if (contains_1u64(coff_info.header_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(coff_info.header_range, sect_range.max-1))) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data (file offsets point into file header)", sect_name, sect_idx+1);
        }
        if (contains_1u64(coff_info.section_table_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(coff_info.section_table_range, sect_range.max-1))) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data (file offsets point into section header table)", sect_name, sect_idx+1);
        }
        if (contains_1u64(coff_info.symbol_table_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(coff_info.symbol_table_range, sect_range.max-1))) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data (file offsets point into symbol table)", sect_name, sect_idx+1);
        }
        if (dim_1u64(sect_range) != coff_sect_header->fsize) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data", sect_name, sect_idx+1);
        }
      }
    }
  }

  // fill out obj
  obj->data      = input->data;
  obj->path      = push_str8_copy(arena, input->path);
  obj->lib_path  = push_str8_copy(arena, input->lib_path);
  obj->input_idx = obj_idx;
  obj->header    = coff_info;
}

internal LNK_ObjNodeArray
lnk_obj_list_push_parallel(TP_Context        *tp,
                           TP_Arena          *arena,
                           LNK_ObjList       *obj_list,
                           COFF_MachineType   machine,
                           U64                input_count,
                           LNK_InputObj     **inputs)
{
  ProfBeginFunction();
  
  LNK_ObjIniter task = {0};
  task.inputs        = inputs;
  task.obj_id_base   = obj_list->count;
  task.objs          = lnk_obj_list_reserve(arena->v[0], obj_list, input_count);
  task.machine       = machine;
  tp_for_parallel(tp, arena, input_count, lnk_obj_initer, &task);
  
  ProfEnd();
  return task.objs;
}

internal LNK_Obj **
lnk_obj_arr_from_list(Arena *arena, LNK_ObjList list)
{
  LNK_Obj **arr = push_array_no_zero(arena, LNK_Obj *, list.count);
  U64 idx = 0;
  for (LNK_ObjNode *node = list.first; node != 0; node = node->next, ++idx) {
    arr[idx] = &node->data;
  }
  return arr;
}

internal COFF_ParsedSymbol
lnk_obj_match_symbol(LNK_Obj *obj, String8 match_name)
{
  COFF_ParsedSymbol result = {0};

  COFF_FileHeaderInfo coff_info = coff_file_header_info_from_data(obj->data);

  String8 raw_coff_symbol_table = str8_substr(obj->data, coff_info.symbol_table_range);
  String8 raw_coff_string_table = str8_substr(obj->data, coff_info.string_table_range);

  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < coff_info.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    void *symbol_ptr;
    if (coff_info.is_big_obj) {
      symbol_ptr = &((COFF_Symbol32 *)raw_coff_symbol_table.str)[symbol_idx];
      symbol     = coff_parse_symbol32(raw_coff_string_table, symbol_ptr);
    } else {
      symbol_ptr = &((COFF_Symbol16 *)raw_coff_symbol_table.str)[symbol_idx];
      symbol     = coff_parse_symbol16(raw_coff_string_table, symbol_ptr);
    }

    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);

    if (str8_match(symbol.name, match_name, 0)) {
      result = symbol;
      break;
    }
  }

  return result;
}

internal MSCRT_FeatFlags
lnk_obj_get_features(LNK_Obj *obj)
{
  return lnk_obj_match_symbol(obj, str8_lit("@feat.00")).value;
}

internal U32
lnk_obj_get_comp_id(LNK_Obj *obj)
{
  return lnk_obj_match_symbol(obj, str8_lit("@comp.id")).value;
}

internal U32
lnk_obj_get_vol_md(LNK_Obj *obj)
{
  return lnk_obj_match_symbol(obj, str8_lit("@vol.md")).value;
}

internal COFF_SectionHeader *
lnk_coff_section_header_from_section_number(LNK_Obj *obj, U64 section_number)
{
  COFF_SectionHeader *section_table  = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  COFF_SectionHeader *section_header = &section_table[section_number-1];
  return section_header;
}

internal COFF_ParsedSymbol
lnk_parsed_symbol_from_coff_symbol_idx(LNK_Obj *obj, U64 symbol_idx)
{
  String8 string_table = str8_substr(obj->data, obj->header.string_table_range);
  String8 symbol_table = str8_substr(obj->data, obj->header.symbol_table_range);

  COFF_ParsedSymbol result = {0};
  if (obj->header.is_big_obj) {
    result = coff_parse_symbol32(string_table, (COFF_Symbol32 *)symbol_table.str + symbol_idx);
  } else {
    result = coff_parse_symbol16(string_table, (COFF_Symbol16 *)symbol_table.str + symbol_idx);
  }
  
  return result;
}

internal COFF_SectionHeader *
lnk_section_header_from_section_number(LNK_Obj *obj, U64 section_number)
{
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  if (section_number > 0 && section_number <= obj->header.section_count_no_null) {
    return &section_table[section_number-1];
  }
  return 0;
}

internal String8List *
lnk_collect_obj_chunks_parallel(TP_Context *tp, TP_Arena *arena, U64 obj_count, LNK_Obj **obj_arr, String8 name, String8 postfix, B32 collect_discarded)
{
  NotImplemented;
  return 0;
}

internal String8List
lnk_collect_obj_chunks(Arena *arena, LNK_Obj *obj, String8 name, String8 postfix, B32 collect_discarded)
{
  NotImplemented;
  String8List result = {0};
  return result;
}

internal void
lnk_parse_msvc_linker_directive(Arena *arena, LNK_Obj *obj, LNK_DirectiveInfo *directive_info, String8 buffer)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 to_parse;
  {
    local_persist const U8 bom_sig[]   = { 0xEF, 0xBB, 0xBF };
    local_persist const U8 ascii_sig[] = { 0x20, 0x20, 0x20 };
    if (MemoryMatch(buffer.str, &bom_sig[0], sizeof(bom_sig))) {
      to_parse = str8_zero();
      lnk_error_obj(LNK_Error_IllData, obj, "TODO: support for BOM encoding");
    } else if (MemoryMatch(buffer.str, &ascii_sig[0], sizeof(ascii_sig))) {
      to_parse = str8_skip(buffer, sizeof(ascii_sig));
    } else {
      to_parse = buffer;
    }
  }
  
  String8List arg_list = lnk_arg_list_parse_windows_rules(scratch.arena, to_parse);
  LNK_CmdLine cmd_line = lnk_cmd_line_parse_windows_rules(scratch.arena, arg_list);

  for (LNK_CmdOption *opt = cmd_line.first_option; opt != 0; opt = opt->next) {
    LNK_CmdSwitch *cmd_switch = lnk_cmd_switch_from_string(opt->string);

    if (cmd_switch == 0) {
      lnk_error_obj(LNK_Warning_UnknownDirective, obj, "unknown directive \"%S\"", opt->string);
      continue;
    }
    if (!cmd_switch->is_legal_directive) {
      lnk_error_obj(LNK_Warning_IllegalDirective, obj, "illegal directive \"%S\"", opt->string);
      continue;
    }

    LNK_Directive *directive = push_array_no_zero(arena, LNK_Directive, 1);
    directive->next          = 0;
    directive->id            = str8_cstring(cmd_switch->name);
    directive->value_list    = str8_list_copy(arena, &opt->value_strings);

    LNK_DirectiveList *directive_list = &directive_info->v[cmd_switch->type];
    SLLQueuePush(directive_list->first, directive_list->last, directive);
    ++directive_list->count;
  }
  
  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_input_coff_symbol_table)
{
  LNK_InputCoffSymbolTable *task = raw_task;
  LNK_Obj                  *obj  = &task->objs.v[task_id].data;

  //
  // parse obj header
  //
  COFF_FileHeaderInfo header = coff_file_header_info_from_data(obj->data);

  //
  // extract COFF info
  //
  String8 raw_coff_section_table = str8_substr(obj->data, header.section_table_range);
  String8 raw_coff_symbol_table  = str8_substr(obj->data, header.symbol_table_range);
  String8 raw_coff_string_table  = str8_substr(obj->data, header.string_table_range);

  COFF_ParsedSymbol symbol = {0};
  for (U64 symbol_idx = 0; symbol_idx < header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    // read symbol
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);

    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    switch (interp) {
    case COFF_SymbolValueInterp_Regular: {
      if (symbol.storage_class == COFF_SymStorageClass_External) {
        LNK_Symbol *defn = lnk_make_defined_symbol(arena, symbol.name, obj, symbol_idx);
        U64         hash = lnk_symbol_hash(symbol.name);
        lnk_symbol_table_push_(task->symtab, arena, worker_id, LNK_SymbolScope_Defined, hash, defn);
      }
    } break;
    case COFF_SymbolValueInterp_Weak: {
      LNK_Symbol *defn = lnk_make_defined_symbol(arena, symbol.name, obj, symbol_idx);
      U64         hash = lnk_symbol_hash(symbol.name);
      lnk_symbol_table_push_(task->symtab, arena, worker_id, LNK_SymbolScope_Defined, hash, defn);

      lnk_symbol_list_push(arena, &task->weak_lists[task_id], defn);
    } break;
    case COFF_SymbolValueInterp_Common: {
      LNK_Symbol *defn = lnk_make_defined_symbol(arena, symbol.name, obj, symbol_idx);
      U64         hash = lnk_symbol_hash(symbol.name);
      lnk_symbol_table_push_(task->symtab, arena, worker_id, LNK_SymbolScope_Defined, hash, defn);
    } break;
    case COFF_SymbolValueInterp_Abs: {
      if (symbol.storage_class == COFF_SymStorageClass_External) {
        LNK_Symbol *defn = lnk_make_defined_symbol(arena, symbol.name, obj, symbol_idx);
        U64         hash = lnk_symbol_hash(symbol.name);
        lnk_symbol_table_push_(task->symtab, arena, worker_id, LNK_SymbolScope_Defined, hash, defn);
      }
    } break;
    case COFF_SymbolValueInterp_Undefined: {
      LNK_Symbol *s = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Defined, symbol.name);
      if (s == 0) {
        if (symbol.storage_class == COFF_SymStorageClass_External) {
          LNK_Symbol *undef = lnk_make_undefined_symbol(arena, symbol.name, obj);
          lnk_symbol_list_push(arena, &task->undef_lists[worker_id], undef);
        } else if (symbol.storage_class == COFF_SymStorageClass_Section) {
          // lookup is performed during image patching step
        } else {
          Assert(!"unexpected storage class on undefined symbol");
        }
      }
    } break;
    case COFF_SymbolValueInterp_Debug: {
      // not used
    } break;
    default: { InvalidPath; } break;
    }
  }
}

internal LNK_SymbolInputResult
lnk_input_obj_symbols(TP_Context *tp, TP_Arena *arena, LNK_SymbolTable *symtab, LNK_ObjNodeArray objs)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  LNK_InputCoffSymbolTable task = {0};
  task.symtab                   = symtab;
  task.objs                     = objs;
  task.weak_lists               = push_array(scratch.arena, LNK_SymbolList, tp->worker_count);
  task.undef_lists              = push_array(scratch.arena, LNK_SymbolList, tp->worker_count);
  tp_for_parallel(tp, arena, objs.count, lnk_input_coff_symbol_table, &task);

  LNK_SymbolInputResult result = {0};
  SLLConcatInPlaceArray(&result.weak_symbols,  task.weak_lists,  tp->worker_count);
  SLLConcatInPlaceArray(&result.undef_symbols, task.undef_lists, tp->worker_count);

  scratch_end(scratch);
  ProfEnd();
  return result;
}

