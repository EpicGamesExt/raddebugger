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

internal LNK_Obj **
lnk_array_from_obj_list(Arena *arena, LNK_ObjList list)
{
  LNK_Obj **arr = push_array_no_zero(arena, LNK_Obj *, list.count);
  U64 idx = 0;
  for (LNK_ObjNode *node = list.first; node != 0; node = node->next, ++idx) {
    arr[idx] = &node->data;
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

  ProfBeginV("Init Obj [%S%s%S]", input->lib_path, (input->lib_path.size ? ": " : 0), input->path);

  //
  // parse obj header
  //
  COFF_FileHeaderInfo header = coff_file_header_info_from_data(input->data);

  //
  // set & check machine compatibility
  //
  if (header.machine != COFF_MachineType_Unknown) {
    COFF_MachineType current_machine = ins_atomic_u32_eval_cond_assign(&task->machine, header.machine, COFF_MachineType_Unknown);
    if (current_machine != COFF_MachineType_Unknown && current_machine != header.machine) {
      lnk_error_with_loc(LNK_Error_IncompatibleMachine, input->path, input->lib_path,
          "conflicting machine types expected %S but got %S",
          coff_string_from_machine_type(current_machine),
          coff_string_from_machine_type(header.machine));
    }
  }

  //
  // extract COFF info
  //
  String8 raw_coff_section_table = str8_substr(input->data, header.section_table_range);
  String8 raw_coff_symbol_table  = str8_substr(input->data, header.symbol_table_range);
  String8 raw_coff_string_table  = str8_substr(input->data, header.string_table_range);

  //
  // error check: section table / symbol table / string table
  //
  if (raw_coff_section_table.size != dim_1u64(header.section_table_range)) {
    lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "corrupted file, unable to read section header table");
  }
  if (raw_coff_symbol_table.size != dim_1u64(header.symbol_table_range)) {
    lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "corrupted file, unable to read symbol table");
  }
  if (raw_coff_string_table.size != dim_1u64(header.string_table_range)) {
    lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "corrupted file, unable to read string table");
  }

  //
  // error check section headers
  //
  COFF_SectionHeader *coff_section_table = (COFF_SectionHeader *)raw_coff_section_table.str;
  for (U64 sect_idx = 0; sect_idx < header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *coff_sect_header = &coff_section_table[sect_idx];

    // read name
    String8 sect_name = coff_name_from_section_header(raw_coff_string_table, coff_sect_header);

    if (~coff_sect_header->flags & COFF_SectionFlag_CntUninitializedData) {
      if (coff_sect_header->fsize > 0) {
        Rng1U64 sect_range = rng_1u64(coff_sect_header->foff, coff_sect_header->foff + coff_sect_header->fsize);

        if (contains_1u64(header.header_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(header.header_range, sect_range.max-1))) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data (file offsets point into file header)", sect_name, sect_idx+1);
        }
        if (contains_1u64(header.section_table_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(header.section_table_range, sect_range.max-1))) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data (file offsets point into section header table)", sect_name, sect_idx+1);
        }
        if (contains_1u64(header.symbol_table_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(header.symbol_table_range, sect_range.max-1))) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data (file offsets point into symbol table)", sect_name, sect_idx+1);
        }
        if (dim_1u64(sect_range) != coff_sect_header->fsize) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data", sect_name, sect_idx+1);
        }
      }
    }
  }

  //
  // error check symbols
  //
  {
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(input->data, header.section_table_range).str;
    String8 string_table = str8_substr(input->data, header.string_table_range);
    String8 symbol_table = str8_substr(input->data, header.symbol_table_range);
    COFF_ParsedSymbol symbol;
    for (U64 symbol_idx = 0; symbol_idx < header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
      symbol = coff_parse_symbol(header, string_table, symbol_table, symbol_idx);
      COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
      if (interp == COFF_SymbolValueInterp_Regular) {
        if (symbol.section_number == 0 || symbol.section_number > header.section_count_no_null) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "symbol %S (No. 0x%x) points to an out of bounds section 0x%x", symbol.name, symbol_idx, symbol.section_number);
        }
        if (symbol.storage_class == COFF_SymStorageClass_Static && symbol.aux_symbol_count > 0) {
          COFF_ComdatSelectType select;
          U32 section_number = 0;
          coff_parse_secdef(symbol, header.is_big_obj, &select, &section_number, 0, 0);
          if (select == COFF_ComdatSelect_Associative) {
            if (section_number == 0 || section_number > header.section_count_no_null) {
              lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "section definition symbol %S (No. 0x%x) associates with an out of bounds section 0x%x", symbol.name, symbol_idx, symbol.section_number);
            }
          }
        }
      }
    }
  }

  //
  // create symbol links to COMDAT sections
  //
  U32 *comdats;
  {
    comdats = push_array_no_zero(arena, U32, header.section_count_no_null);
    MemorySet(comdats, 0xff, header.section_count_no_null * sizeof(comdats[0]));

    String8 string_table = str8_substr(input->data, header.string_table_range);
    String8 symbol_table = str8_substr(input->data, header.symbol_table_range);
    COFF_ParsedSymbol symbol;
    for (U64 symbol_idx = 0; symbol_idx < header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
      symbol = coff_parse_symbol(header, string_table, symbol_table, symbol_idx);

      COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
      if (interp == COFF_SymbolValueInterp_Regular) {
        if (symbol.storage_class == COFF_SymStorageClass_Static) {
          if (symbol.section_number > 0 && symbol.section_number <= header.section_count_no_null) {
            COFF_SectionHeader *sect_header = &coff_section_table[symbol.section_number-1];
            if (sect_header->flags & COFF_SectionFlag_LnkCOMDAT) {
              if (symbol.aux_symbol_count) {
                U32 section_length = 0;
                coff_parse_secdef(symbol, header.is_big_obj, 0, 0, &section_length, 0);
                if (sect_header->fsize == section_length) {
                  if (comdats[symbol.section_number-1] == ~0) {
                    comdats[symbol.section_number-1] = symbol_idx;
                  } else {
                    lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "section definition symbo (No. 0x%llx) tries to ovewrite comdat", symbol_idx);
                  }
                } else {
                  lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "section size specified by section definition symbol (No 0x%llx) doesn't match size in section header (No. 0x%x); expected 0x%x got 0x%x", symbol_idx, symbol.section_number, section_length, sect_header->fsize);
                }
              }
            }
          } else {
            lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "section definition symbol (No. 0x%llx) has out of bounds section number 0x%x", symbol_idx, symbol.section_number);
          }
        }
      }
    }
  }

  //
  // COMDAT loop checker
  //
  {
    Temp scratch = scratch_begin(&arena, 1);

    String8    string_table     = str8_substr(input->data, header.string_table_range);
    String8    symbol_table     = str8_substr(input->data, header.symbol_table_range);
    HashTable *visited_sections = hash_table_init(scratch.arena, 32);
    for (U64 sect_idx = 0; sect_idx < header.section_count_no_null; sect_idx += 1) {
      for (U32 curr_section = sect_idx;;) {
        U32 symbol_idx = comdats[curr_section];

        // is section COMDAT?
        if (symbol_idx == max_U32) {
          break;
        }

        // extract COMDAT info for current section
        COFF_ParsedSymbol     symbol         = coff_parse_symbol(header, string_table, symbol_table, symbol_idx);
        COFF_ComdatSelectType select         = COFF_ComdatSelect_Null;
        U32                   section_number = 0;
        coff_parse_secdef(symbol, header.is_big_obj, &select, &section_number, 0, 0);

        if (select != COFF_ComdatSelect_Associative) {
          // section terminates at non-associative COMDAT -- no loop
          break;
        }

        // was section visited? -- loop found
        if (hash_table_search_u64(visited_sections, curr_section)) {
          COFF_ParsedSymbol symbol = coff_parse_symbol(header, string_table, symbol_table, comdats[sect_idx]);
          lnk_error_with_loc(LNK_Error_AssociativeLoop, input->path, input->lib_path, "section symbol %S (No. 0x%x) does not terminate on a non-associate COMDAT symbol", symbol.name, comdats[sect_idx]);
          break;
        }

        // track visited sections
        hash_table_push_u64_u64(scratch.arena, visited_sections, curr_section, 0);

        // follow association
        Assert(section_number > 0);
        curr_section = section_number-1;
      }

      // purge hash table for next run
      hash_table_purge(visited_sections);
    }

    scratch_end(scratch);
  }
  
  //
  // extract obj features from compile symbol in .debug$S
  //
  B8 hotpatch = 0;
  if (header.machine == COFF_MachineType_X64) {
    hotpatch = 1;
  } else {
    Temp scratch = scratch_begin(&arena, 1);

    CV_Symbol comp_symbol = {0};
    for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
      COFF_SectionHeader *sect_header = &coff_section_table[sect_idx];
      String8 name = str8_cstring_capped_reverse(sect_header->name, sect_header->name+sizeof(sect_header->name));
      if (str8_match(name, str8_lit(".debug$S"), 0)) {
        Temp temp = temp_begin(scratch.arena);
        String8 debug_s_data = str8_substr(input->data, rng_1u64(sect_header->foff, sect_header->foff+sect_header->fsize));
        CV_DebugS debug_s = cv_parse_debug_s(temp.arena, debug_s_data);
        for (String8Node *symbols_n = debug_s.data_list[CV_C13SubSectionIdxKind_Symbols].first; symbols_n != 0; symbols_n = symbols_n->next) {
          CV_SymbolList symbol_list = {0};
          cv_parse_symbol_sub_section_capped(scratch.arena, &symbol_list, 0, symbols_n->string, CV_SymbolAlign, 2);
          if (symbol_list.first->data.kind == CV_SymKind_COMPILE3) {
            comp_symbol = symbol_list.first->data;
            goto found_comp_symbol;
          } else if (symbol_list.last->data.kind == CV_SymKind_COMPILE3) {
            comp_symbol = symbol_list.last->data;
            goto found_comp_symbol;
          }
        }
        temp_end(temp);
      }
    }
    found_comp_symbol:;

    if (comp_symbol.kind == CV_SymKind_COMPILE3 && comp_symbol.data.size >= sizeof(CV_SymCompile3)) {
      CV_SymCompile3 *comp = (CV_SymCompile3 *)comp_symbol.data.str;
      hotpatch = !!(comp->flags & CV_Compile3Flag_HotPatch);
    }

    scratch_end(scratch);
  }

  // fill out obj
  obj->data      = input->data;
  obj->path      = push_str8_copy(arena, input->path);
  obj->lib_path  = push_str8_copy(arena, input->lib_path);
  obj->input_idx = obj_idx;
  obj->header    = header;
  obj->comdats   = comdats;
  obj->hotpatch  = hotpatch;

  ProfEnd();
}

internal LNK_ObjNodeArray
lnk_obj_list_push_parallel(TP_Context        *tp,
                           TP_Arena          *arena,
                           LNK_ObjList       *list,
                           COFF_MachineType   machine,
                           U64                input_count,
                           LNK_InputObj     **inputs)
{
  ProfBeginFunction();

  // store base id
  U64 obj_id_base = list->count;

  // reserve obj nodes
  LNK_ObjNodeArray objs = {0};
  if (input_count > 0) {
    objs.count = input_count;
    objs.v = push_array(arena->v[0], LNK_ObjNode, input_count);
    for (LNK_ObjNode *ptr = objs.v, *opl = objs.v + input_count; ptr < opl; ++ptr) {
      SLLQueuePush(list->first, list->last, ptr);
    }
    list->count += input_count;
  }
  
  // fill out & run task
  LNK_ObjIniter task = {0};
  task.inputs        = inputs;
  task.obj_id_base   = obj_id_base;
  task.objs          = objs;
  task.machine       = machine;
  tp_for_parallel(tp, arena, input_count, lnk_obj_initer, &task);
  
  ProfEnd();
  return objs;
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
        COFF_SectionHeader *sect_header = lnk_coff_section_header_from_section_number(obj, symbol.section_number);
        if (sect_header->flags & COFF_SectionFlag_LnkRemove) {
          break;
        }
        LNK_Symbol *defn = lnk_make_defined_symbol(arena, symbol.name, obj, symbol_idx);
        lnk_symbol_table_push_(task->symtab, arena, worker_id, LNK_SymbolScope_Defined, defn);
      }
    } break;
    case COFF_SymbolValueInterp_Weak: {
      LNK_Symbol *defn = lnk_make_defined_symbol(arena, symbol.name, obj, symbol_idx);
      lnk_symbol_table_push_(task->symtab, arena, worker_id, LNK_SymbolScope_Defined, defn);

      lnk_symbol_list_push(arena, &task->weak_lists[worker_id], defn);
    } break;
    case COFF_SymbolValueInterp_Common: {
      LNK_Symbol *defn = lnk_make_defined_symbol(arena, symbol.name, obj, symbol_idx);
      lnk_symbol_table_push_(task->symtab, arena, worker_id, LNK_SymbolScope_Defined, defn);
    } break;
    case COFF_SymbolValueInterp_Abs: {
      if (symbol.storage_class == COFF_SymStorageClass_External) {
        LNK_Symbol *defn = lnk_make_defined_symbol(arena, symbol.name, obj, symbol_idx);
        lnk_symbol_table_push_(task->symtab, arena, worker_id, LNK_SymbolScope_Defined, defn);
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

internal B32
lnk_is_coff_section_debug(LNK_Obj *obj, U64 sect_idx)
{
  String8 string_table = str8_substr(obj->data, obj->header.string_table_range);
  COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(obj, sect_idx+1);
  
  String8 full_name = coff_name_from_section_header(string_table, section_header);
  String8 name, postfix;
  coff_parse_section_name(full_name, &name, &postfix);

  B32 is_debug = str8_match(name, str8_lit(".debug"), 0);
  return is_debug;
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

internal
THREAD_POOL_TASK_FUNC(lnk_collect_obj_chunks_task)
{
  LNK_SectionCollector *task = raw_task;
  LNK_Obj              *obj  = task->objs[task_id];

  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  String8             string_table  = str8_substr(obj->data, obj->header.string_table_range);
  for (U32 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *section_header = &section_table[sect_idx];

    if (section_header->flags & COFF_SectionFlag_LnkRemove) {
      if (!task->collect_discarded) {
        continue;
      }
    }

    String8 section_name = coff_name_from_section_header(string_table, section_header);
    if (str8_match(section_name, task->name, 0)) {
      String8 section_data = str8_substr(obj->data, rng_1u64(section_header->foff, section_header->foff + section_header->fsize));
      str8_list_push(arena, &task->out_lists[task_id], section_data);
    }
  }
}

internal String8List *
lnk_collect_obj_sections(TP_Context *tp, TP_Arena *arena, U64 objs_count, LNK_Obj **objs, String8 name, B32 collect_discarded)
{
  LNK_SectionCollector task = {0};
  task.objs              = objs;
  task.name              = name;
  task.collect_discarded = collect_discarded;
  task.out_lists         = push_array(arena->v[0], String8List, objs_count);
  tp_for_parallel(tp, arena, objs_count, lnk_collect_obj_chunks_task, &task);
  return task.out_lists;
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

