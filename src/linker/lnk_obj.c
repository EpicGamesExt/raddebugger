// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

typedef struct LNK_CompressedObjCensus
{
  U64 obj_count;
  U64 raw_bytes;
  U64 section_bytes;
  U64 debug_s_bytes;
  U64 debug_t_bytes;
  U64 debug_p_bytes;
  U64 debug_h_bytes;
  U64 other_section_bytes;
  U64 symbol_bytes;
  U64 string_bytes;
  U64 section_table_bytes;
  U64 reloc_bytes;
} LNK_CompressedObjCensus;

global LNK_CompressedObjCensus g_lnk_compressed_obj_census;
global B32 g_lnk_compressed_obj_census_enabled;

internal String8
lnk_loc_from_obj(Arena *arena, LNK_Obj *obj)
{
  String8 obj_path = str8_skip_last_slash(obj ? obj->path : str8_lit("RADLINK"));
  String8 lib_path = str8_skip_last_slash(lnk_obj_get_lib_path(obj));
  String8 result;
  if (lib_path.size) {
    result = push_str8f(arena, "%S(%S)", lib_path, obj_path);
  } else {
    result = push_str8_copy(arena, obj_path);
  }
  return result;
}

internal void
lnk_error_obj(LNK_ErrorCode code, LNK_Obj *obj, char *fmt, ...)
{
  va_list args; va_start(args, fmt);
  String8 obj_path = obj ? obj->path : str8_zero();
  String8 lib_path = lnk_obj_get_lib_path(obj);
  lnk_error_with_loc_fv(code, obj_path, lib_path, fmt, args);
  va_end(args);
}

internal void
lnk_error_input_obj(LNK_ErrorCode code, LNK_Input *input, char *fmt, ...)
{
  va_list args; va_start(args, fmt);
  LNK_LibMemberRef *link_member = input->link_member;
  LNK_Lib          *link_lib    = link_member ? link_member->lib : 0;
  lnk_error_with_loc_fv(code, input->path, link_lib ? link_lib->path : str8_zero(), fmt, args);
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
  LNK_Input     *input   = task->inputs[task_id];
  LNK_Obj       *obj     = &task->objs[task_id].data;

  //ProfBeginV("Init Obj [%S%s%S]", input->lib_path, (input->lib_path.size ? ": " : 0), input->path);

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
      lnk_error_input_obj(LNK_Error_IncompatibleMachine, input,
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
  // error check section table / symbol table / string table
  //
  if (raw_coff_section_table.size != dim_1u64(header.section_table_range)) {
    lnk_error_input_obj(LNK_Error_IllData, input, "corrupted file, unable to read section header table");
  }
  if (raw_coff_symbol_table.size != dim_1u64(header.symbol_table_range)) {
    lnk_error_input_obj(LNK_Error_IllData, input, "corrupted file, unable to read symbol table");
  }
  if (raw_coff_string_table.size != dim_1u64(header.string_table_range)) {
    lnk_error_input_obj(LNK_Error_IllData, input, "corrupted file, unable to read string table");
  }

  COFF_SectionHeader *section_headers = push_array(arena, COFF_SectionHeader, header.section_count_no_null + 1);
  MemoryCopy(section_headers + 1, raw_coff_section_table.str, raw_coff_section_table.size);

  //
  // section table pass
  //  - error check headers fields
  //  - collect section header flags
  //  - find debug info and meta-data leaders
  //
  COFF_SectionHeader *coff_section_table    = (COFF_SectionHeader *)raw_coff_section_table.str;
  U32                 debug_t_section_number      = 0;
  U32                 debug_p_section_number      = 0;
  U32                 debug_h_section_number      = 0;
  U32                 llvm_addrsig_section_number = 0;
  U64                 census_section_bytes = 0;
  U64                 census_debug_s_bytes = 0;
  U64                 census_debug_t_bytes = 0;
  U64                 census_debug_p_bytes = 0;
  U64                 census_debug_h_bytes = 0;
  U64                 census_reloc_bytes = 0;
  for EachIndex(sect_idx, header.section_count_no_null) {
    U64                 section_number    = sect_idx + 1;
    COFF_SectionHeader *coff_sect_header = &coff_section_table[sect_idx];
    COFF_SectionFlags  *section_flags    = &section_headers[section_number].flags;
    String8             sect_name        = coff_name_from_section_header(raw_coff_string_table, coff_sect_header);
    *section_flags = coff_sect_header->flags & ~3; // linker reserves low 2 bits for internal flags

    if (g_lnk_compressed_obj_census_enabled) {
      census_section_bytes += coff_sect_header->fsize;
      census_reloc_bytes += (U64)coff_sect_header->reloc_count * sizeof(COFF_Reloc);
      if (str8_match(sect_name, str8_lit(".debug$S"), 0)) { census_debug_s_bytes += coff_sect_header->fsize; }
      if (str8_match(sect_name, str8_lit(".debug$T"), 0)) { census_debug_t_bytes += coff_sect_header->fsize; }
      if (str8_match(sect_name, str8_lit(".debug$P"), 0)) { census_debug_p_bytes += coff_sect_header->fsize; }
      if (str8_match(sect_name, str8_lit(".debug$H"), 0)) { census_debug_h_bytes += coff_sect_header->fsize; }
    }

    if (str8_starts_with(sect_name, str8_lit(".debug$"))) {
      *section_flags |= LNK_SECTION_FLAG_DEBUG;
    }
    if (str8_ends_with(sect_name, str8_lit("$fo$"), 0) ||
        str8_ends_with(sect_name, str8_lit("$fo_rvas$"), 0) ||
        str8_ends_with(sect_name, str8_lit("$fo_bdd$"), 0)) {
      *section_flags |= COFF_SectionFlag_LnkInfo;
    }

    if (str8_match(sect_name, str8_lit(".debug$T"), 0)) {
      debug_t_section_number = section_number;
    } else if (str8_match(sect_name, str8_lit(".debug$P"), 0)) {
      debug_p_section_number = section_number;
    } else if (str8_match(sect_name, str8_lit(".debug$H"), 0)) {
      debug_h_section_number = section_number;
    }

    if (llvm_addrsig_section_number == 0 && str8_match(sect_name, str8_lit(".llvm_addrsig"), 0)) {
      llvm_addrsig_section_number = section_number;
    }

    if (~*section_flags & COFF_SectionFlag_CntUninitializedData) {
      if (coff_sect_header->fsize > 0) {
        Rng1U64 sect_range = rng_1u64(coff_sect_header->foff, coff_sect_header->foff + coff_sect_header->fsize);
        if (contains_1u64(header.header_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(header.header_range, sect_range.max-1))) {
          lnk_error_input_obj(LNK_Error_IllData, input, "header (%S No. %#llx) defines out of bounds section data (file offsets point into file header)", sect_name, sect_idx+1);
        }
        if (contains_1u64(header.section_table_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(header.section_table_range, sect_range.max-1))) {
          lnk_error_input_obj(LNK_Error_IllData, input, "header (%S No. %#llx) defines out of bounds section data (file offsets point into section header table)", sect_name, sect_idx+1);
        }
        if (contains_1u64(header.symbol_table_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(header.symbol_table_range, sect_range.max-1))) {
          lnk_error_input_obj(LNK_Error_IllData, input, "header (%S No. %#llx) defines out of bounds section data (file offsets point into symbol table)", sect_name, sect_idx+1);
        }
        if (dim_1u64(sect_range) != coff_sect_header->fsize) {
          lnk_error_input_obj(LNK_Error_IllData, input, "header (%S No. %#llx) defines out of bounds section data", sect_name, sect_idx+1);
        }
      }
    }
  }

  //
  // error check symbol table and cache name lengths for primary symbols
  //
  U64 primary_symbol_count = 0;
  {
    COFF_ParsedSymbol symbol;
    for (U64 symbol_idx = 0; symbol_idx < header.symbol_count; symbol_idx += 1 + symbol.aux_symbol_count) {
      symbol = coff_parse_symbol_no_name(header, raw_coff_symbol_table, symbol_idx);
      primary_symbol_count += 1;
    }
  }

  U32 *associated_section_offsets = push_array(arena, U32, header.section_count_no_null + 2);
  U64 symbol_block_count = CeilIntegerDiv(header.symbol_count, 64);
  LNK_ObjSymbolArray symbols = {
    .count           = primary_symbol_count,
    .primary_bits    = bit_array_init32(arena, header.symbol_count),
    .block_bases     = push_array_no_zero(arena, U32,                  symbol_block_count),
    .values          = push_array_no_zero(arena, U64,                  primary_symbol_count),
    .section_numbers = push_array_no_zero(arena, U32,                  primary_symbol_count),
    .types           = push_array_no_zero(arena, COFF_SymbolType,      primary_symbol_count),
    .storage_classes = push_array_no_zero(arena, COFF_SymStorageClass, primary_symbol_count),
    .aux_counts      = push_array_no_zero(arena, U8,                   primary_symbol_count),
    .name_offsets    = push_array_no_zero(arena, U32,                  primary_symbol_count),
    .name_sizes      = push_array_no_zero(arena, U32,                  primary_symbol_count),
  };
  {
    U64 next_block  = 0;
    U32 primary_idx = 0;
    COFF_ParsedSymbol symbol;
    for (U64 symbol_idx = 0; symbol_idx < header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {

      symbol = coff_parse_symbol(header, raw_coff_string_table, raw_coff_symbol_table, symbol_idx);

      // copy out primary symbols
      {
        U64 block_idx = symbol_idx >> 6;
        while (next_block <= block_idx) { symbols.block_bases[next_block++] = primary_idx; }

        bit_array_set_bit32(symbols.primary_bits, symbol_idx, 1);

        symbols.values         [primary_idx] = symbol.value;
        symbols.section_numbers[primary_idx] = symbol.section_number;
        symbols.types          [primary_idx] = symbol.type;
        symbols.storage_classes[primary_idx] = symbol.storage_class;
        symbols.aux_counts     [primary_idx] = symbol.aux_symbol_count;
        symbols.name_offsets   [primary_idx] = symbol.name.size ? safe_cast_u32(symbol.name.str - input->data.str) : 0;
        symbols.name_sizes     [primary_idx] = safe_cast_u32(symbol.name.size);

        primary_idx += 1;
      }

      COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
      if (interp == COFF_SymbolValueInterp_Regular) {
        if (symbol.section_number == 0 || symbol.section_number > header.section_count_no_null) {
          lnk_error_input_obj(LNK_Error_IllData, input, "symbol %S (No. 0x%x) points to an out of bounds section 0x%x", symbol.name, symbol_idx, symbol.section_number);
        }
        if (symbol.storage_class == COFF_SymStorageClass_Static && symbol.aux_symbol_count > 0) {
          COFF_ComdatSelectType select;
          U32 section_number = 0;
          coff_parse_secdef(symbol, header.is_big_obj, &select, &section_number, 0, 0);
          if (select == COFF_ComdatSelect_Associative) {
            if (section_number == 0 || section_number > header.section_count_no_null) {
              lnk_error_input_obj(LNK_Error_IllData, input, "section definition symbol %S (No. 0x%x) associates with an out of bounds section 0x%x", symbol.name, symbol_idx, section_number);
            } else if (symbol.section_number > 0 && symbol.section_number <= header.section_count_no_null) {
              associated_section_offsets[section_number + 1] += 1;
            }
          }
        }
      }
    }
    while (next_block < symbol_block_count) {
      symbols.block_bases[next_block++] = primary_idx;
    }
    Assert(primary_idx == primary_symbol_count);
  }

  // Convert per-parent association counts to CSR offsets. The offsets double as fill cursors
  // below; the backwards pass restores them and the old SLLStackPush sibling order.
  for (U64 section_number = 1; section_number <= header.section_count_no_null; section_number += 1) {
    associated_section_offsets[section_number + 1] += associated_section_offsets[section_number];
  }
  U32  associated_section_count   = associated_section_offsets[header.section_count_no_null + 1];
  U32 *associated_section_numbers = push_array_no_zero(arena, U32, associated_section_count);

  //
  // create symbol links to COMDAT sections
  //
  U32 *comdats;
  {
    comdats = push_array_no_zero(arena, U32, header.section_count_no_null + 1);
    MemorySet(comdats, 0xff, (header.section_count_no_null + 1) * sizeof(comdats[0]));

    COFF_ParsedSymbol symbol;
    for (U64 symbol_idx = 0; symbol_idx < header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
      symbol = coff_parse_symbol_no_name(header, raw_coff_symbol_table, symbol_idx);

      COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
      if (interp == COFF_SymbolValueInterp_Regular) {
        if (symbol.storage_class == COFF_SymStorageClass_Static) {
          if (symbol.section_number > 0 && symbol.section_number <= header.section_count_no_null) {
            COFF_SectionHeader *sect_header = &coff_section_table[symbol.section_number-1];
            if (symbol.aux_symbol_count) {
              COFF_ComdatSelectType selection      = COFF_ComdatSelect_Null;
              U32                   section_number = 0;
              U32                   section_length = 0;
              coff_parse_secdef(symbol, header.is_big_obj, &selection, &section_number, &section_length, 0);
              if (selection == COFF_ComdatSelect_Associative && section_number > 0 && section_number <= header.section_count_no_null) {
                U32 cursor = associated_section_offsets[section_number]++;
                associated_section_numbers[cursor] = symbol.section_number;
              }
              if (section_headers[symbol.section_number].flags & COFF_SectionFlag_LnkCOMDAT) {
                if (sect_header->fsize == section_length) {
                  if (comdats[symbol.section_number] == ~0) {
                    comdats[symbol.section_number] = symbol_idx;
                  } else {
                    lnk_error_input_obj(LNK_Error_IllData, input, "section definition symbo (No. 0x%llx) tries to ovewrite comdat", symbol_idx);
                  }
                } else {
                  lnk_error_input_obj(LNK_Error_IllData, input, "section size specified by section definition symbol (No 0x%llx) doesn't match size in section header (No. 0x%x); expected 0x%x got 0x%x", symbol_idx, symbol.section_number, section_length, sect_header->fsize);
                }
              }
            }
          } else {
            lnk_error_input_obj(LNK_Error_IllData, input, "section definition symbol (No. 0x%llx) has out of bounds section number 0x%x", symbol_idx, symbol.section_number);
          }
        }
      }
    }
  }
  for (U64 section_number = header.section_count_no_null; section_number > 0; section_number -= 1) {
    associated_section_offsets[section_number] = associated_section_offsets[section_number - 1];
    U32 min = associated_section_offsets[section_number];
    U32 max = associated_section_offsets[section_number + 1];
    for (U32 child_idx = 0; child_idx < (max - min) / 2; child_idx += 1) {
      Swap(U32, associated_section_numbers[min + child_idx], associated_section_numbers[max - child_idx - 1]);
    }
  }

  //
  // COMDAT loop checker
  //
  {
    Temp scratch = scratch_begin(&arena, 1);

    HashTable *visited_sections = hash_table_init(scratch.arena, 32);
    for (U32 section_number = 1; section_number <= header.section_count_no_null; section_number += 1) {
      for (U32 curr_section_number = section_number;;) {
        U32 symbol_idx = comdats[curr_section_number];

        // is section COMDAT?
        if (symbol_idx == max_U32) {
          break;
        }

        // extract COMDAT info for current section
        COFF_ParsedSymbol     symbol         = coff_parse_symbol_no_name(header, raw_coff_symbol_table, symbol_idx);
        COFF_ComdatSelectType select         = COFF_ComdatSelect_Null;
        U32                   associated_section_number = 0;
        coff_parse_secdef(symbol, header.is_big_obj, &select, &associated_section_number, 0, 0);

        if (select != COFF_ComdatSelect_Associative) {
          // section terminates at non-associative COMDAT -- no loop
          break;
        }

        // was section visited? -- loop found
        if (hash_table_search_u64(visited_sections, curr_section_number)) {
          COFF_ParsedSymbol symbol = coff_parse_symbol(header, raw_coff_string_table, raw_coff_symbol_table, comdats[section_number]);
          lnk_error_input_obj(LNK_Error_AssociativeLoop, input, "section symbol %S (No. 0x%x) does not terminate on a non-associate COMDAT symbol", symbol.name, comdats[section_number]);
          break;
        }

        // track visited sections
        hash_table_push_u64_u64(scratch.arena, visited_sections, curr_section_number, 0);

        // follow association
        Assert(associated_section_number > 0);
        curr_section_number = associated_section_number;
      }

      // purge hash table for next run
      hash_table_purge(visited_sections);
    }

    scratch_end(scratch);
  }

  B8 hotpatch = 0;
  if (header.machine == COFF_MachineType_X64) {
    hotpatch = 1;
  }
  //
  // extract obj features from compile symbol in .debug$S
  //
  else {
    Temp scratch = scratch_begin(&arena, 1);

    CV_Symbol comp_symbol = {0};
    for EachIndex(sect_idx, header.section_count_no_null) {
      COFF_SectionHeader *sect_header = &coff_section_table[sect_idx];
      if (section_headers[sect_idx + 1].flags & LNK_SECTION_FLAG_DEBUG) {
        String8 name = str8_cstring_capped(sect_header->name, sect_header->name+sizeof(sect_header->name));
        if (str8_match(name, str8_lit(".debug$S"), 0)) {
          Temp temp = temp_begin(scratch.arena);
          Rng1U64 debug_s_range = rng_1u64(sect_header->foff, sect_header->foff+sect_header->fsize);
          LNK_CObjDebugSView indexed = {0};
          if (lnk_compressed_obj_debug_s_index(input->compressed_obj, debug_s_range, &indexed)) {
            // COMPILE3 is expected in the first two records of a Symbols payload.  The sidecar
            // lets this early object-feature probe skip the otherwise full C13 header walk.
            for EachIndex(entry_idx, indexed.count) {
              LNK_CObjDebugSEntry *entry = &indexed.v[entry_idx];
              if (entry->kind != CV_C13SubSectionKind_Symbols) { continue; }
              String8 symbols = str8(input->data.str + entry->raw_payload_offset, entry->raw_payload_size);
              for (U64 cursor = 0, count = 0; cursor < symbols.size && count < 2; count += 1) {
                CV_SymbolHeader symbol_header;
                TryReadBreak(str8_deserial_read_struct(symbols, cursor, &symbol_header), cursor);
                if (symbol_header.kind == CV_SymKind_COMPILE3) {
                  String8 raw_symbol = str8_substr(symbols, r1u64(cursor, cursor + symbol_header.size + sizeof(CV_SymSize)));
                  comp_symbol = cv_symbol_from_ptr(raw_symbol.str);
                  goto found_comp_symbol;
                }
                cursor += symbol_header.size + sizeof(CV_SymSize);
                cursor  = AlignPow2(cursor, CV_SymbolAlign);
              }
            }
          } else {
            String8   debug_s_data = str8_substr(input->data, debug_s_range);
            CV_DebugS debug_s      = cv_debug_s_from_data(temp.arena, debug_s_data);
            cv_debug_s_tag_prov_sect(&debug_s, (U32)sect_idx);
            String8List symbols_list = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Symbols);
            for EachNode(symbols_n, String8Node, symbols_list.first) {
              for (U64 cursor = 0, count = 0; cursor < symbols_n->string.size && count < 2; count += 1) {
                CV_SymbolHeader symbol_header;
                TryReadBreak(str8_deserial_read_struct(symbols_n->string, cursor, &symbol_header), cursor);
                if (symbol_header.kind == CV_SymKind_COMPILE3) {
                  String8 raw_symbol = str8_substr(symbols_n->string, r1u64(cursor, cursor + symbol_header.size + sizeof(CV_SymSize)));
                  comp_symbol = cv_symbol_from_ptr(raw_symbol.str);
                  goto found_comp_symbol;
                }
                cursor += symbol_header.size + sizeof(CV_SymSize);
                cursor  = AlignPow2(cursor, CV_SymbolAlign);
              }
            }
          }
          temp_end(temp);
        }
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
  obj->coff = (LNK_ObjCoff){
    .data   = input->data,
    .header = header,
    .sections = {
      .count_no_null = header.section_count_no_null,
      .headers      = section_headers,
      .comdats      = comdats,
      .associated_section_offsets = associated_section_offsets,
      .associated_section_numbers = associated_section_numbers,
    },
    .symbols                     = symbols,
    .debug_t_section_number      = debug_t_section_number,
    .debug_p_section_number      = debug_p_section_number,
    .debug_h_section_number      = debug_h_section_number,
    .llvm_addrsig_section_number = llvm_addrsig_section_number,
    .hotpatch                    = hotpatch,
  };
  obj->compressed_obj          = input->compressed_obj;
  obj->path                    = push_str8_copy(arena, input->path);
  obj->exclude_from_debug_info = input->exclude_from_debug_info;
  obj->self                    = &task->objs[task_id];
  obj->link_member             = input->link_member;

  if (input->compressed_obj != 0 && g_lnk_compressed_obj_census_enabled) {
    U64 known_debug = census_debug_s_bytes + census_debug_t_bytes + census_debug_p_bytes + census_debug_h_bytes;
    ins_atomic_u64_inc_eval(&g_lnk_compressed_obj_census.obj_count);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.raw_bytes, input->data.size);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.section_bytes, census_section_bytes);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.debug_s_bytes, census_debug_s_bytes);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.debug_t_bytes, census_debug_t_bytes);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.debug_p_bytes, census_debug_p_bytes);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.debug_h_bytes, census_debug_h_bytes);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.other_section_bytes, census_section_bytes - known_debug);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.symbol_bytes, raw_coff_symbol_table.size);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.string_bytes, raw_coff_string_table.size);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.section_table_bytes, raw_coff_section_table.size);
    ins_atomic_u64_add_eval(&g_lnk_compressed_obj_census.reloc_bytes, census_reloc_bytes);
  }
}

internal void
lnk_obj_log_compressed_census(void)
{
  if (!g_lnk_compressed_obj_census_enabled || g_lnk_compressed_obj_census.obj_count == 0) { return; }
  LNK_CompressedObjCensus *c = &g_lnk_compressed_obj_census;
  lnk_log(LNK_Log_Timers,
          "[cobj census] objs=%llu raw=%.2f GiB sections=%.2f GiB debugS=%.2f GiB debugT=%.2f GiB debugP=%.2f GiB debugH=%.2f GiB other=%.2f GiB symbols=%.2f GiB strings=%.2f GiB sect_headers=%.2f GiB relocs=%.2f GiB",
          c->obj_count, (F64)c->raw_bytes/GB(1), (F64)c->section_bytes/GB(1),
          (F64)c->debug_s_bytes/GB(1), (F64)c->debug_t_bytes/GB(1), (F64)c->debug_p_bytes/GB(1),
          (F64)c->debug_h_bytes/GB(1), (F64)c->other_section_bytes/GB(1),
          (F64)c->symbol_bytes/GB(1), (F64)c->string_bytes/GB(1),
          (F64)c->section_table_bytes/GB(1), (F64)c->reloc_bytes/GB(1));
}

internal LNK_ObjNode *
lnk_obj_from_input_many(TP_Context *tp, TP_Arena *arena, LNK_Config *config, U64 inputs_count, LNK_Input **inputs)
{
  LNK_ObjNode *objs = 0;
  if (inputs_count) {
    char *census_env = getenv("RAD_COBJ_CENSUS");
    g_lnk_compressed_obj_census_enabled = census_env != 0 && census_env[0] != 0 && census_env[0] != '0';
    objs = push_array(arena->v[0], LNK_ObjNode, inputs_count);
    LNK_ObjIniter task = {
      .inputs            = inputs,
      .objs              = objs,
      .machine           = config->machine,
    };
    tp_for_parallel(tp, arena, inputs_count, lnk_obj_initer, &task);
  }
  return objs;
}

internal LNK_ObjNode *
lnk_obj_from_input(Arena *arena, LNK_Config *config, LNK_Input *input)
{
  Temp scratch = scratch_begin(&arena, 1);
  TP_Context  *tp       = tp_alloc(scratch.arena, 1, 1, str8_zero());
  TP_Arena     tp_arena = { .count = 1, .v = &arena };
  LNK_ObjNode *result   = lnk_obj_from_input_many(tp, &tp_arena, config, 1, &input);
  scratch_end(scratch);
  return result;
}

internal void
lnk_obj_list_push_node_many(LNK_ObjList *list, U64 count, LNK_ObjNode *nodes)
{
  for EachIndex(i, count) {
    DLLPushBack(list->first, list->last, &nodes[i]);
  }
  list->count += count;
}

internal void
lnk_obj_list_push_node(LNK_ObjList *list, LNK_ObjNode *node)
{
  lnk_obj_list_push_node_many(list, 1, node);
}

internal
THREAD_POOL_TASK_FUNC(lnk_input_coff_symbol_table)
{
  LNK_InputCoffSymbolTable *task = raw_task;
  LNK_Obj                  *obj  = task->objs[task_id];
  for LNK_EachCoffSymbol(it, obj) {
    U64               symbol_idx = it.symbol_idx;
    COFF_ParsedSymbol symbol     = it.v;
    COFF_SymbolValueInterpType interp = coff_interp_from_parsed_symbol(symbol);
    LNK_SymbolSearchType search_type = lnk_symbol_search_type_from_coff(obj, symbol, interp);
    switch (interp) {
    case COFF_SymbolValueInterp_Regular: {
      if (symbol.storage_class == COFF_SymStorageClass_External) {
        LNK_ObjSection section = lnk_obj_section_from_section_number(obj, symbol.section_number);
        if (*section.flags & COFF_SectionFlag_LnkRemove) {
          break;
        }
        LNK_Symbol *defn = lnk_make_symbol(arena, lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx), obj, symbol_idx, search_type);
        lnk_symbol_table_push_(task->symtab, arena, worker_id, defn);
      }
    } break;
    case COFF_SymbolValueInterp_Weak: {
      LNK_Symbol *defn = lnk_make_symbol(arena, lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx), obj, symbol_idx, search_type);
      lnk_symbol_table_push_(task->symtab, arena, worker_id, defn);
    } break;
    case COFF_SymbolValueInterp_Undefined: {
      if (symbol.storage_class == COFF_SymStorageClass_External) {
        LNK_Symbol *defn = lnk_make_symbol(arena, lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx), obj, symbol_idx, search_type);
        lnk_symbol_table_push_(task->symtab, arena, worker_id, defn);
      }
    } break;
    case COFF_SymbolValueInterp_Common: {
      LNK_Symbol *defn = lnk_make_symbol(arena, lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx), obj, symbol_idx, search_type);
      lnk_symbol_table_push_(task->symtab, arena, worker_id, defn);
    } break;
    case COFF_SymbolValueInterp_Abs: {
      if (symbol.storage_class == COFF_SymStorageClass_External) {
        LNK_Symbol *defn = lnk_make_symbol(arena, lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx), obj, symbol_idx, search_type);
        lnk_symbol_table_push_(task->symtab, arena, worker_id, defn);
      }
    } break;
    case COFF_SymbolValueInterp_Debug: {
      // not used
    } break;
    default: { InvalidPath; } break;
    }
  }
}

internal LNK_ObjSymbolRef *
lnk_symlinks_from_obj(Arena *arena, LNK_SymbolTable *symtab, LNK_Obj *obj)
{
  LNK_ObjSymbolRef *symlinks = push_array(arena, LNK_ObjSymbolRef, obj->coff.sections.count_no_null + 1);
  for LNK_EachCoffSymbol(it, obj) {
    U64               symbol_idx = it.symbol_idx;
    COFF_ParsedSymbol symbol     = it.v;
    COFF_SymbolValueInterpType interp = coff_interp_from_parsed_symbol(symbol);
    if (interp != COFF_SymbolValueInterp_Regular) { continue; }

    COFF_SectionFlags section_flags = obj->coff.sections.headers[symbol.section_number].flags;
    if (~section_flags & COFF_SectionFlag_LnkCOMDAT) { continue; }

    LNK_ObjSymbolRef *symlink = &symlinks[symbol.section_number];

    // external symbols
    if (symbol.storage_class == COFF_SymStorageClass_External && symbol.aux_symbol_count == 0) {
      String8 symbol_name = lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx);
      B32 can_set_symlink = (symlink->obj == 0 || symbol.value == 0);
      if (!can_set_symlink && symlink->obj == obj) {
        COFF_ParsedSymbol leader = lnk_parsed_symbol_from_coff_symbol_idx_no_name(symlink->obj, symlink->symbol_idx);
        B32 leader_is_same_section  = leader.section_number == symbol.section_number;
        B32 leader_is_static_anchor = (leader_is_same_section && leader.storage_class == COFF_SymStorageClass_Static && leader.aux_symbol_count == 0);
        B32 leader_is_vftable       = str8_starts_with(lnk_symbol_name_from_coff_symbol_idx(symlink->obj, symlink->symbol_idx), str8_lit(MSCRT_VFTABLE_SYMBOL_PREFIX));
        B32 current_is_vftable      = str8_starts_with(symbol_name, str8_lit(MSCRT_VFTABLE_SYMBOL_PREFIX));

        // prefer public symbols to local static anchors; prefer vftable public
        // symbols to other public symbols so ICF keeps vftables in their own color space
        can_set_symlink = (leader_is_static_anchor || (leader_is_same_section && current_is_vftable && !leader_is_vftable));
      }

      if (can_set_symlink) {
        LNK_SymbolHashTrie *link_symbol = lnk_symbol_table_search_(symtab, symbol_name);
        if (link_symbol) {
          *symlink = lnk_ref_from_symbol(link_symbol->symbol);
        }
      }
    }
    // static symbols
    else if (symbol.storage_class == COFF_SymStorageClass_Static) {
      if (symbol.aux_symbol_count == 0) {
        if (symlink->obj == 0) {
          *symlink = (LNK_ObjSymbolRef){ obj, symbol_idx };
        }
      }
    }
  }

  return symlinks;
}

internal U32List
lnk_obj_collect_associated_section_numbers(Arena *arena, LNK_Obj *obj, U32 root_section_number, COFF_SectionFlags skip_flags)
{
  Assert(1 <= root_section_number && root_section_number <= obj->coff.sections.count_no_null);
  Temp scratch = scratch_begin(&arena, 1);

  // track each child before enqueueing it because COFF associations can cycle
  HashMap  seen_hm = {0};
  U32List  queue   = {0};

  U32Node root_n = { root_section_number };
  u32_list_push_node(&queue, &root_n);
  hash_map_push_u64_u64(scratch.arena, &seen_hm, root_section_number, 1);

  // walk the complete descendant chain because associated COMDATs can nest
  for EachNode(parent_n, U32Node, queue.first) {
    U32Array associated_sections = lnk_obj_associated_sections_from_section_number(obj, parent_n->data);
    for EachIndex(associated_idx, associated_sections.count) {
      U32 child_section_number = associated_sections.v[associated_idx];

      if (child_section_number == 0)                                           { continue; }
      if (hash_map_search_u64_u64(&seen_hm, child_section_number))             { continue; }
      if (obj->coff.sections.headers[child_section_number].flags & skip_flags) { continue; }

      hash_map_push_u64_u64(scratch.arena, &seen_hm, child_section_number, 1);
      u32_list_push(arena, &queue, child_section_number);
    }
  }

  // return only child sections so callers choose whether the root participates
  U32List result = {0};
  if (queue.count > 1) {
    result.first = queue.first->next;
    result.last  = queue.last;
    result.count = queue.count - 1;
  }

  scratch_end(scratch);
  return result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_assign_comdat_symlinks_task)
{
  LNK_InputCoffSymbolTable *task = raw_task;
  LNK_Obj                  *obj  = task->objs[task_id];
  obj->symlinks = lnk_symlinks_from_obj(arena, task->symtab, obj);
}

internal void
lnk_assign_comdat_symlinks(TP_Context *tp, TP_Arena *arena, LNK_SymbolTable *symtab, U64 objs_count, LNK_Obj **objs)
{
  ProfBeginFunction();
  LNK_InputCoffSymbolTable task = { .symtab = symtab, .objs = objs };
  tp_for_parallel(tp, arena, objs_count, lnk_assign_comdat_symlinks_task, &task);
  ProfEnd();
}

internal void
lnk_push_obj_symbols(TP_Context *tp, TP_Arena *arena, LNK_SymbolTable *symtab, U64 objs_count, LNK_Obj **objs)
{
  ProfBeginFunction();
  LNK_InputCoffSymbolTable task = { .symtab = symtab, .objs = objs };
  tp_for_parallel(tp, arena, objs_count, lnk_input_coff_symbol_table, &task);
  ProfEnd();
}

internal COFF_ParsedSymbol
lnk_obj_match_symbol(LNK_Obj *obj, String8 match_name)
{
  for LNK_EachCoffSymbol(it, obj) {
    COFF_ParsedSymbol symbol = it.v;
    symbol.name = lnk_symbol_name_from_coff_symbol_idx(obj, it.symbol_idx);
    if (str8_match(symbol.name, match_name, 0)) {
      return symbol;
    }
  }
  return (COFF_ParsedSymbol){0};
}

internal MSCRT_FeatFlags
lnk_obj_get_features(LNK_Obj *obj)
{
  return lnk_obj_match_symbol(obj, str8_lit("@feat.00")).value;
}

internal LNK_Lib *
lnk_obj_get_lib(LNK_Obj *obj)
{
  return obj->link_member ? obj->link_member->lib : 0;
}

internal String8
lnk_obj_get_lib_path(LNK_Obj *obj)
{
  String8 lib_path = {0};
  if (obj) {
    LNK_Lib *lib = lnk_obj_get_lib(obj);
    lib_path = lib ? lib->path : str8_zero();
  }
  return lib_path;
}

internal U32
lnk_obj_get_removed_section_number(LNK_Obj *obj)
{
  return obj->coff.header.is_big_obj ? LNK_REMOVED_SECTION_NUMBER_32 : LNK_REMOVED_SECTION_NUMBER_16;
}

internal B32
lnk_obj_get_comdat_symlink_from_section_number(LNK_Obj *obj, U64 section_number, LNK_ObjSymbolRef *symlink_out)
{
  Assert(1 <= section_number && section_number <= obj->coff.sections.count_no_null);
  LNK_ObjSymbolRef symlink = obj->symlinks[section_number];
  B32 is_valid = symlink.obj != 0;
  if (is_valid && symlink_out) {
    *symlink_out = symlink;
  }
  return is_valid;
}

internal U32Array
lnk_obj_associated_sections_from_section_number(LNK_Obj *obj, U32 section_number)
{
  Assert(section_number <= obj->coff.sections.count_no_null);
  U32 min = obj->coff.sections.associated_section_offsets[section_number];
  U32 max = obj->coff.sections.associated_section_offsets[section_number + 1];
  U32Array result = { .count = max - min };
  if (result.count) {
    result.v = obj->coff.sections.associated_section_numbers + min;
  }
  return result;
}
internal String8
lnk_obj_section_data_from_number(LNK_Obj *obj, U64 section_number)
{
  Assert(1 <= section_number && section_number <= obj->coff.sections.count_no_null);
  if (obj->section_data_copies != 0 && obj->section_data_copies[section_number].size != 0) {
    return obj->section_data_copies[section_number];
  }
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->coff.data, obj->coff.header.section_table_range).str;
  COFF_SectionHeader *section       = &section_table[section_number-1];
  Rng1U64             range         = r1u64s(section->foff, section->fsize);
  String8             direct        = lnk_compressed_obj_direct_range(obj->compressed_obj, range);
  return direct.size ? direct : str8_substr(obj->coff.data, range);
}

internal U64
lnk_obj_foff_from_section_data_ptr(LNK_Obj *obj, void *ptr)
{
  U64 data_min = IntFromPtr(obj->coff.data.str);
  U64 ptr_v    = IntFromPtr(ptr);
  Assert(data_min <= ptr_v && ptr_v < data_min + obj->coff.data.size);
  return ptr_v - data_min;
}

internal String8
lnk_obj_section_name_from_section_number(LNK_Obj *obj, U64 section_number)
{
  Assert(1 <= section_number && section_number <= obj->coff.sections.count_no_null);
  COFF_SectionHeader *section_header = &obj->coff.sections.headers[section_number];
  return coff_name_from_section_header(lnk_coff_string_table_from_obj(obj), section_header);
}

internal LNK_ObjSection
lnk_obj_section_from_section_number(LNK_Obj *obj, U64 section_number)
{
  Assert(1 <= section_number && section_number <= obj->coff.sections.count_no_null);
  COFF_SectionHeader *header = &obj->coff.sections.headers[section_number];
  LNK_ObjSection section = {
    .obj            = obj,
    .section_number = section_number,
    .header         = header,
    .flags          = &header->flags,
    .vrange         = rng_1u64(header->voff, header->voff + header->vsize),
    .frange         = rng_1u64(header->foff, header->foff + header->fsize),
    .reloc_count    = header->reloc_count,
  };
  return section;
}

internal force_inline B32
lnk_obj_section_iter_next(LNK_Obj *obj, LNK_ObjSectionIter *it)
{
  U64 section_number = it->v.section_number + 1;
  B32 is_valid = section_number <= obj->coff.sections.count_no_null;
  if (is_valid) {
    it->v = lnk_obj_section_from_section_number(obj, section_number);
  }
  return is_valid;
}

internal COFF_RelocArray
lnk_coff_relocs_from_section_header(LNK_Obj *obj, COFF_SectionHeader *section_header)
{
  COFF_RelocInfo   reloc_info = coff_reloc_info_from_section_header(obj->coff.data, section_header);
  COFF_Reloc      *relocs     = (COFF_Reloc *)(obj->coff.data.str + reloc_info.array_off);
  COFF_RelocArray  result     = { .count = reloc_info.count, .v = relocs };
  return result;
}

internal String8
lnk_coff_string_table_from_obj(LNK_Obj *obj)
{
  return str8_substr(obj->coff.data, obj->coff.header.string_table_range);
}

internal String8
lnk_coff_symbol_table_from_obj(LNK_Obj *obj)
{
  return str8_substr(obj->coff.data, obj->coff.header.symbol_table_range);
}

internal COFF_RelocArray
lnk_coff_reloc_info_from_section_number(LNK_Obj *obj, U64 section_number)
{
  LNK_ObjSection   section    = lnk_obj_section_from_section_number(obj, section_number);
  COFF_RelocInfo   reloc_info = coff_reloc_info_from_section_header(obj->coff.data, section.header);
  COFF_Reloc      *relocs     = str8_deserial_get_raw_ptr(obj->coff.data, reloc_info.array_off, sizeof(*relocs)*reloc_info.count);
  COFF_RelocArray  result     = { .count = reloc_info.count, .v = relocs };
  return result;
}

internal B32
lnk_try_comdat_props_from_section_number(LNK_Obj *obj, U32 section_number, COFF_ComdatSelectType *select_out, U32 *section_number_out, U32 *section_length_out, U32 *check_sum_out)
{
  Assert(1 <= section_number && section_number <= obj->coff.sections.count_no_null);
  U32 symbol_idx = obj->coff.sections.comdats[section_number];
  if (symbol_idx != max_U32) {
    COFF_ParsedSymbol secdef = lnk_parsed_symbol_from_coff_symbol_idx_no_name(obj, symbol_idx);
    coff_parse_secdef(secdef, obj->coff.header.is_big_obj, select_out, section_number_out, section_length_out, check_sum_out);
    return 1;
  }
  return 0;
}

internal COFF_SectionHeader *
lnk_coff_section_header_from_section_number(LNK_Obj *obj, U64 section_number)
{
  Assert(1 <= section_number && section_number <= obj->coff.sections.count_no_null);
  return &obj->coff.sections.headers[section_number];
}

internal force_inline U64
lnk_obj_primary_symbol_idx_from_coff_symbol_idx(LNK_Obj *obj, U64 symbol_idx)
{
  U64 block_idx = symbol_idx >> 6;
  U64 word_idx  = symbol_idx >> 5;
  U32 bit_idx   = symbol_idx & 31;
  U32Array primary_bits = obj->coff.symbols.primary_bits;
  Assert(bit_array_get_bit32(primary_bits, symbol_idx));

  U64 primary_idx = obj->coff.symbols.block_bases[block_idx];
  if (word_idx & 1) {
    primary_idx += count_bits_set32(primary_bits.v[word_idx - 1]);
  }
  primary_idx += count_bits_set32(primary_bits.v[word_idx] & ((1u << bit_idx) - 1));
  return primary_idx;
}

internal force_inline B32
lnk_obj_symbol_iter_next(LNK_Obj *obj, LNK_ObjSymbolIter *it)
{
  if (it->next_primary_idx >= obj->coff.symbols.count) {
    Assert(it->next_symbol_idx >= obj->coff.header.symbol_count);
    return 0;
  }

  Assert(it->next_symbol_idx < obj->coff.header.symbol_count);
  U64 symbol_size = obj->coff.header.is_big_obj ? sizeof(COFF_Symbol32) : sizeof(COFF_Symbol16);
  it->symbol_idx  = it->next_symbol_idx;
  it->primary_idx = it->next_primary_idx;
  it->v = (COFF_ParsedSymbol){
    .value            = obj->coff.symbols.values[it->primary_idx],
    .section_number   = obj->coff.symbols.section_numbers[it->primary_idx],
    .type             = obj->coff.symbols.types[it->primary_idx],
    .storage_class    = obj->coff.symbols.storage_classes[it->primary_idx],
    .aux_symbol_count = obj->coff.symbols.aux_counts[it->primary_idx],
    .raw_symbol       = lnk_coff_symbol_table_from_obj(obj).str + it->symbol_idx * symbol_size,
  };
  it->next_symbol_idx  += 1 + it->v.aux_symbol_count;
  it->next_primary_idx += 1;
  return 1;
}

internal force_inline COFF_ParsedSymbol
lnk_parsed_symbol_from_coff_symbol_idx_no_name(LNK_Obj *obj, U64 symbol_idx)
{
  U64 primary_idx = lnk_obj_primary_symbol_idx_from_coff_symbol_idx(obj, symbol_idx);
  U64 symbol_size = obj->coff.header.is_big_obj ? sizeof(COFF_Symbol32) : sizeof(COFF_Symbol16);
  COFF_ParsedSymbol result = {
    .value            = obj->coff.symbols.values[primary_idx],
    .section_number   = obj->coff.symbols.section_numbers[primary_idx],
    .type             = obj->coff.symbols.types[primary_idx],
    .storage_class    = obj->coff.symbols.storage_classes[primary_idx],
    .aux_symbol_count = obj->coff.symbols.aux_counts[primary_idx],
    .raw_symbol       = lnk_coff_symbol_table_from_obj(obj).str + symbol_idx * symbol_size,
  };
  return result;
}

internal force_inline String8
lnk_symbol_name_from_coff_symbol_idx(LNK_Obj *obj, U64 symbol_idx)
{
  U64 primary_idx = lnk_obj_primary_symbol_idx_from_coff_symbol_idx(obj, symbol_idx);
  return str8(obj->coff.data.str + obj->coff.symbols.name_offsets[primary_idx], obj->coff.symbols.name_sizes[primary_idx]);
}

internal force_inline COFF_ParsedSymbol
lnk_parsed_symbol_from_coff_symbol_idx(LNK_Obj *obj, U64 symbol_idx)
{
  COFF_ParsedSymbol result = lnk_parsed_symbol_from_coff_symbol_idx_no_name(obj, symbol_idx);
  result.name = lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx);
  return result;
}

// Drop the obj's patched debug-section copies: zero each entry so
// lnk_obj_section_data_from_number
// falls back to the (still-mapped) input view. The copy BYTES live on the shared
// per-worker SECT_DATA_COPIES arenas (see lnk_obj_reloc_patcher) and are handed back
// wholesale via arena_release at the caller; only call once every reader of the patched
// bytes is done. Idempotent.
internal void
lnk_obj_drop_section_data_copies(LNK_Obj *obj)
{
  if (obj->section_data_copies == 0) { return; }
  for (U32 section_number = 1; section_number <= obj->coff.sections.count_no_null; section_number += 1) {
    obj->section_data_copies[section_number] = str8_zero();
  }
}

// Resolve a .debug$S subsection provenance record (streaming-ring P1) back to its bytes.
// Returns exactly what the parse consumed: lnk_obj_section_data_from_number prefers the reloc-patched
// private copy when one exists (the parsed node slices point into that same copy), otherwise
// the raw mapped input. Synthetic provenance (linker-made bytes, is_synthetic) and untagged
// records (sect_idx == CV_DebugSProvSect_Nil) have no backing section to resolve through:
// returns str8_zero and the caller must fall back to the node's String8.
internal String8
lnk_resolve_debug_s_node(LNK_Obj *obj, CV_DebugSProvNode *prov)
{
  if (prov == 0 || prov->is_synthetic || prov->sect_idx == CV_DebugSProvSect_Nil) {
    return str8_zero();
  }
  U64     section_number = (U64)prov->sect_idx + 1;
  String8 sect_data      = lnk_obj_section_data_from_number(obj, section_number);
  return str8_substr(sect_data, rng_1u64(prov->off, prov->off + prov->size));
}

// reloc patch ordering: sort each section's relocs by apply_off so the RMW write stream into
// the destination buffer is monotone-forward (HW-prefetchable) instead of scattered in on-disk
// table order. apply_off is the primary key; orig_idx is a tiebreak so the total order is
// deterministic and the final bytes are identical to the unsorted patch order (each reloc
// writes its own disjoint field; only a pathological same-apply_off overlap could depend on
// order, and the orig_idx tiebreak preserves the original sequence there too).
typedef struct LNK_RelocSortKey
{
  COFF_Reloc reloc;
  U32        orig_idx;
} LNK_RelocSortKey;

internal int
lnk_reloc_sort_key_is_before(void *raw_a, void *raw_b)
{
  LNK_RelocSortKey *a = raw_a, *b = raw_b;
  if (a->reloc.apply_off != b->reloc.apply_off) {
    return a->reloc.apply_off < b->reloc.apply_off;
  }
  return a->orig_idx < b->orig_idx;
}

// Applies one section's relocations into `section_data` -- a writable buffer holding that
// section's bytes (the section's slice of the image at image-build time, a private patched
// copy for non-$S debug sections, or the streaming-ring window copy at module-write time).
// Factored out of lnk_obj_reloc_patcher so the image patch pass and the P3.3 window fill
// share one definition: same reloc order, same symbol resolution, same skip semantics
// (debug-section relocs against removed sections are silently dropped), byte-identical
// application at either call time (relocs + symbol tables + image section table are all
// immutable after the image build).
internal void
lnk_obj_apply_relocs_to_buffer(LNK_Obj *obj, U64 section_number, COFF_SectionHeader *section_header, String8 section_data, U64 image_base, COFF_SectionHeader **image_section_table)
{
  Assert(1 <= section_number && section_number <= obj->coff.sections.count_no_null);
  COFF_RelocArray relocs = lnk_coff_relocs_from_section_header(obj, section_header);
  if (relocs.count == 0) { return; }

  Temp scratch = scratch_begin(0, 0);
  COFF_SectionFlags section_flags = obj->coff.sections.headers[section_number].flags;

  // apply relocs (sorted by apply_off for monotone-forward writes)
  LNK_RelocSortKey *sorted_relocs = push_array_no_zero(scratch.arena, LNK_RelocSortKey, relocs.count);
  for EachIndex(reloc_idx, relocs.count) {
    sorted_relocs[reloc_idx].reloc    = relocs.v[reloc_idx];
    sorted_relocs[reloc_idx].orig_idx = (U32)reloc_idx;
  }
  radsort(sorted_relocs, relocs.count, lnk_reloc_sort_key_is_before);
  for EachIndex(reloc_idx, relocs.count) {
    COFF_Reloc *reloc = &sorted_relocs[reloc_idx].reloc;

    // error check relocation
    if (obj->coff.header.machine == COFF_MachineType_X64) {
      if (reloc->type > COFF_Reloc_X64_Last) {
        lnk_error_obj(LNK_Error_IllegalRelocation, obj, "unknown relocation type 0x%x", reloc->type);
      }
    } else if (obj->coff.header.machine != COFF_MachineType_Unknown) {
      lnk_not_implemented("relocation patching is not implemented for %S", coff_string_from_machine_type(obj->coff.header.machine));
      continue;
    }

    // compute virtual offsets
    U64 reloc_voff = section_header->voff + reloc->apply_off;

    // compute symbol location values
    U32 symbol_secnum = 0;
    U32 symbol_secoff = 0;
    S64 symbol_voff   = 0;
    {
      COFF_ParsedSymbol          symbol = lnk_parsed_symbol_from_coff_symbol_idx_no_name(obj, reloc->isymbol);
      COFF_SymbolValueInterpType interp = coff_interp_from_parsed_symbol(symbol);
      if (interp == COFF_SymbolValueInterp_Regular) {
        if (symbol.section_number == lnk_obj_get_removed_section_number(obj)) {
          if (~section_flags & LNK_SECTION_FLAG_DEBUG) {
            String8 sect_name   = lnk_obj_section_name_from_section_number(obj, section_number);
            String8 symbol_name = lnk_symbol_name_from_coff_symbol_idx(obj, reloc->isymbol);
            lnk_error_obj(LNK_Error_RelocationAgainstRemovedSection, obj, "relocating against symbol that is in a removed section (symbol: %S, reloc-section: %S 0x%llx, reloc-index: 0x%llx)", symbol_name, sect_name, section_number, reloc_idx);
          }
          continue;
        }
        symbol_secnum = symbol.section_number;
        symbol_secoff = symbol.value;
        symbol_voff   = safe_cast_u32((U64)image_section_table[symbol.section_number]->voff + (U64)symbol_secoff);
      } else if (interp == COFF_SymbolValueInterp_Abs) {
        // There aren't enough bits in COFF symbol to store full image base address,
        // so we special case __ImageBase. A better solution would be to add
        // a 64-bit symbol format to COFF.
        if (str8_match(lnk_symbol_name_from_coff_symbol_idx(obj, reloc->isymbol), str8_lit("__ImageBase"), 0)) {
          symbol.value = image_base;
        }
        symbol_secnum = 0;
        symbol_secoff = 0;
        symbol_voff   = (S64)symbol.value - (S64)image_base;
      } else if (interp == COFF_SymbolValueInterp_Weak) {
        // unresolved weak
      } else if (interp == COFF_SymbolValueInterp_Undefined) {
        // unresolved undefined
      } else {
        InvalidPath;
      }
    }

    // pick reloc value
    COFF_RelocValue reloc_value = {0};
    switch (obj->coff.header.machine) {
    case COFF_MachineType_Unknown: {} break;
    case COFF_MachineType_X64: { reloc_value = coff_pick_reloc_value_x64(reloc->type, image_base, reloc_voff, symbol_secnum, symbol_secoff, symbol_voff); } break;
    default: { NotImplemented; } break;
    }

    // read addend
    Assert(reloc_value.size <= section_data.size);
    U64 raw_addend = 0;
    str8_deserial_read(section_data, reloc->apply_off, &raw_addend, reloc_value.size, 1);

    // compute new reloc value
    S64 addend       = extend_sign64(raw_addend, reloc_value.size);
    U64 reloc_result = reloc_value.value + addend;

    // commit new reloc value
    MemoryCopy(section_data.str + reloc->apply_off, &reloc_result, reloc_value.size);
  }
  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_collect_obj_chunks_task)
{
  LNK_SectionCollector *task = raw_task;
  LNK_Obj              *obj  = task->objs[task_id];

  // Optional 0-based section-index sidecar, kept in the same order as out_lists.
  if (task->out_sect_indices != 0) {
    U64 match_count = 0;
    for LNK_EachCoffSection(count_it, obj) {
      if (*count_it.v.flags & COFF_SectionFlag_LnkRemove && !task->collect_discarded) { continue; }
      if (str8_match(lnk_obj_section_name_from_section_number(obj, count_it.v.section_number), task->name, 0)) { match_count += 1; }
    }
    task->out_sect_indices[task_id].count = 0;
    task->out_sect_indices[task_id].v = push_array_no_zero(arena, U32, match_count ? match_count : 1);
  }

  for LNK_EachCoffSection(it, obj) {
    LNK_ObjSection section = it.v;

    if (*section.flags & COFF_SectionFlag_LnkRemove && !task->collect_discarded) {
      continue;
    }

    String8 section_name = lnk_obj_section_name_from_section_number(obj, section.section_number);
    if (str8_match(section_name, task->name, 0)) {
      String8 section_data = lnk_obj_section_data_from_number(obj, section.section_number);
      str8_list_push(arena, &task->out_lists[task_id], section_data);
      if (task->out_sect_indices != 0) {
        U32Array *indices = &task->out_sect_indices[task_id];
        indices->v[indices->count++] = safe_cast_u32(section.section_number - 1);
      }
    }
  }
}

internal String8List *
lnk_collect_obj_sections(TP_Context *tp, TP_Arena *arena, U64 objs_count, LNK_Obj **objs, String8 name, B32 collect_discarded, U32Array **sect_indices_out)
{
  LNK_SectionCollector task = {0};
  task.objs              = objs;
  task.name              = name;
  task.collect_discarded = collect_discarded;
  task.out_lists         = push_array(arena->v[0], String8List, objs_count);
  if (sect_indices_out != 0) {
    task.out_sect_indices = push_array(arena->v[0], U32Array, objs_count);
  }
  tp_for_parallel(tp, arena, objs_count, lnk_collect_obj_chunks_task, &task);
  if (sect_indices_out != 0) {
    *sect_indices_out = task.out_sect_indices;
  }
  return task.out_lists;
}

internal B32
lnk_obj_is_before(void *raw_a, void *raw_b)
{
  LNK_Obj *a = raw_a, *b = raw_b;
  return a->input_idx < b->input_idx;
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
    directive->value         = push_str8_copy(arena, opt->value);

    LNK_DirectiveList *directive_list = &directive_info->v[cmd_switch->type];
    SLLQueuePush(directive_list->first, directive_list->last, directive);
    ++directive_list->count;
  }
  
  scratch_end(scratch);
}

internal String8List
lnk_raw_directives_from_obj(Arena *arena, LNK_Obj *obj)
{
  String8List drectve_data = {0};
  for LNK_EachCoffSection(it, obj) {
    LNK_ObjSection section = it.v;
    if (*section.flags & COFF_SectionFlag_LnkInfo) {
      String8 section_name = lnk_obj_section_name_from_section_number(obj, section.section_number);
      if (str8_match(section_name, str8_lit(".drectve"), 0)) {
        if (*section.flags & COFF_SectionFlag_CntUninitializedData) {
          lnk_error_obj(LNK_Error_IllData, obj, ".drectve section header has flag COFF_SectionFlag_CntUninitializedData");
          break;
        }
        if (dim_1u64(section.frange) < 3) {
          lnk_error_obj(LNK_Error_IllData, obj, "not enough bytes to parse .drectve");
          break;
        }
        if (section.reloc_count > 0) {
          lnk_error_obj(LNK_Error_IllData, obj, ".drectve must not have relocations");
          break;
        }
        str8_list_push(arena, &drectve_data, lnk_obj_section_data_from_number(obj, section.section_number));
      }
    }
  }
  return drectve_data;
}

internal LNK_DirectiveInfo
lnk_directive_info_from_raw_directives(Arena *arena, LNK_Obj *obj, String8List raw_directives)
{
  LNK_DirectiveInfo directive_info = {0};
  for (String8Node *drectve_n = raw_directives.first; drectve_n != 0; drectve_n = drectve_n->next) {
    lnk_parse_msvc_linker_directive(arena, obj, &directive_info, drectve_n->string);
  }
  return directive_info;
}

force_inline int
lnk_section_offset_symbol_is_before(void *raw_a, void *raw_b)
{
  LNK_SectionOffsetSymbol *a = raw_a, *b = raw_b;
  if (a->key == b->key) {
    return a->symbol_idx < b->symbol_idx;
  }
  return a->key < b->key;
}

internal LNK_ObjSymbolMap *
lnk_symbol_map_from_obj(Arena *arena, LNK_Obj *obj)
{
  // count functions
  U64 count = 0;
  for LNK_EachCoffSymbol(it, obj) {
    COFF_ParsedSymbol symbol = it.v;
    if (coff_interp_from_parsed_symbol(symbol) == COFF_SymbolValueInterp_Regular && COFF_SymbolType_IsFunc(symbol.type)) {
      count += 1;
    }
  }

  LNK_ObjSymbolMap *map = push_array(arena, LNK_ObjSymbolMap, 1);
  map->v = push_array_no_zero(arena, LNK_SectionOffsetSymbol, count);

  for LNK_EachCoffSymbol(it, obj) {
    COFF_ParsedSymbol symbol = it.v;
    if (coff_interp_from_parsed_symbol(symbol) == COFF_SymbolValueInterp_Regular && COFF_SymbolType_IsFunc(symbol.type)) {
      LNK_SectionOffsetSymbol *entry = &map->v[map->count++];
      entry->key                     = Compose64Bit(symbol.section_number, symbol.value);
      entry->symbol_idx              = safe_cast_u32(it.symbol_idx);
    }
  }
  Assert(map->count == count);

  radsort(map->v, map->count, lnk_section_offset_symbol_is_before);

  return map;
}

internal U32
lnk_symbol_from_section_offset(LNK_ObjSymbolMap *map, U32 section_number, U32 offset)
{
  U64 key = Compose64Bit(section_number, offset);

  U64 min = 0;
  U64 opl = map->count;
  while (min < opl) {
    U64 mid = min + (opl - min) / 2;
    if (map->v[mid].key <= key) {
      min = mid + 1;
    } else {
      opl = mid;
    }
  }

  U32 symbol_idx = max_U32;
  if (min > 0) {
    LNK_SectionOffsetSymbol *entry = &map->v[min - 1];
    if (entry->key >> 32 == section_number) {
      symbol_idx = entry->symbol_idx;
    }
  }
  return symbol_idx;
}

internal CV_DebugS
lnk_debug_s_from_obj(Arena *arena, LNK_Obj *obj)
{
  // (single loop so each parse can tag provenance with its section index; the old
  // collect-then-parse split had no side effects between the loops)
  CV_DebugS debug_s = {0};
  for LNK_EachCoffSection(it, obj) {
    LNK_ObjSection section      = it.v;
    String8        section_name = lnk_obj_section_name_from_section_number(obj, section.section_number);
    if (!str8_match(section_name, str8_lit(".debug$S"), 0)) { continue; }

    // parse & merge sub sections
    String8   raw_debug_s = lnk_obj_section_data_from_number(obj, section.section_number);
    CV_DebugS ds          = cv_debug_s_from_data(arena, raw_debug_s);
    cv_debug_s_tag_prov_sect(&ds, safe_cast_u32(section.section_number - 1));
    cv_debug_s_concat_in_place(&debug_s, &ds);

    // make sure there is one string table
    String8List string_data_list = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_StringTable);
    if (string_data_list.node_count > 1) {
      break;
    }

    // make sure there is one file checksum table
    String8List checksum_data_list = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_FileChksms);
    if (checksum_data_list.node_count > 1) {
      continue;
    }
  }

  cv_debug_s_validate_prov(&debug_s);
  return debug_s;
}

force_inline int
lnk_line_table_block_is_before(void *raw_a, void *raw_b)
{
  LNK_LineTableBlock *a = raw_a, *b = raw_b;
  U64 key_a = Compose64Bit(a->header->sec_idx, a->header->sec_off_lo);
  U64 key_b = Compose64Bit(b->header->sec_idx, b->header->sec_off_lo);
  if (key_a == key_b) { return a->header->sec_off_hi < b->header->sec_off_hi; }
  return key_a < key_b;
}

force_inline int
lnk_inline_range_is_before(void *raw_a, void *raw_b)
{
  LNK_InlineRange *a = raw_a, *b = raw_b;
  if (a->key_min != b->key_min) { return a->key_min < b->key_min; }
  return a->site_idx < b->site_idx;
}

internal LNK_ObjLineMap *
lnk_line_map_from_obj(Arena *arena, LNK_Obj *obj)
{
  Temp scratch = scratch_begin(&arena, 1);

  //
  // build .debug$S section file offset -> relocation value
  //
#define DebugRelocValueFromFileOffset(foff) hash_map_search_u64_u64(&debug_reloc_hm, foff)
  HashMap debug_reloc_hm = {0};
  for LNK_EachCoffSection(it, obj) {
    LNK_ObjSection section = it.v;
    if (!str8_match(lnk_obj_section_name_from_section_number(obj, section.section_number), str8_lit(".debug$S"), 0)) { continue; }

    String8         section_data = lnk_obj_section_data_from_number(obj, section.section_number);
    COFF_RelocArray relocs       = lnk_coff_relocs_from_section_header(obj, section.header);
    for EachIndex(reloc_idx, relocs.count) {
      COFF_Reloc       *reloc = &relocs.v[reloc_idx];
      COFF_ParsedSymbol symbol = lnk_parsed_symbol_from_coff_symbol_idx_no_name(obj, reloc->isymbol);
      if (coff_interp_from_parsed_symbol(symbol) != COFF_SymbolValueInterp_Regular) { continue; }

      U64 reloc_size  = 0;
      U64 reloc_value = 0;
      if ((obj->coff.header.machine == COFF_MachineType_X64 && reloc->type == COFF_Reloc_X64_Section) ||
          (obj->coff.header.machine == COFF_MachineType_X86 && reloc->type == COFF_Reloc_X86_Section)) {
        reloc_size  = sizeof(U16);
        reloc_value = symbol.section_number;
      } else if ((obj->coff.header.machine == COFF_MachineType_X64 && reloc->type == COFF_Reloc_X64_SecRel) ||
                 (obj->coff.header.machine == COFF_MachineType_X86 && reloc->type == COFF_Reloc_X86_SecRel)) {
        reloc_size  = sizeof(U32);
        reloc_value = symbol.value;
      }

      if (reloc_size > 0 && reloc->apply_off + reloc_size <= section_data.size) {
        U64 addend = 0;
        str8_deserial_read(section_data, reloc->apply_off, &addend, reloc_size, 1);
        U64 foff  = section.frange.min + reloc->apply_off;
        U64 value = reloc_value + extend_sign64(addend, reloc_size);
        U64 *existing = hash_map_search_u64_u64(&debug_reloc_hm, foff);
        if (existing) {
          *existing = value;
        } else {
          hash_map_push_u64_u64(scratch.arena, &debug_reloc_hm, foff, value);
        }
      }
    }
  }

  CV_DebugS   debug_s        = lnk_debug_s_from_obj(scratch.arena, obj);
  String8List raw_lines_list = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Lines);
  String8List raw_checksums  = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_FileChksms);
  String8List raw_strings    = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_StringTable);

  //
  // estimate requried line table block capacity
  //
  U64 line_table_blocks_cap = 0;
  for EachNode(raw_lines_node, String8Node, raw_lines_list.first) {
    Temp temp = temp_begin(scratch.arena);
    CV_C13LinesHeaderList parsed_list = cv_c13_lines_from_sub_sections(temp.arena, raw_lines_node->string, rng_1u64(0, raw_lines_node->string.size));
    line_table_blocks_cap += parsed_list.count;
    temp_end(temp);
  }

  LNK_ObjLineMap *map = push_array(arena, LNK_ObjLineMap, 1);
  map->arena             = arena;
  map->debug_checksums   = str8_list_first(&raw_checksums);
  map->debug_strings     = str8_list_first(&raw_strings);
  map->line_table_blocks = push_array(arena, LNK_LineTableBlock, line_table_blocks_cap);
  map->tables            = push_array(arena, LNK_LineTable, line_table_blocks_cap);

  for EachNode(raw_lines_node, String8Node, raw_lines_list.first) {
    Temp temp = temp_begin(scratch.arena);
    String8               raw_lines   = raw_lines_node->string;
    CV_C13LinesHeaderList parsed_list = cv_c13_lines_from_sub_sections(temp.arena, raw_lines, rng_1u64(0, raw_lines.size));
    for EachNode(header_node, CV_C13LinesHeaderNode, parsed_list.first) {
      if (header_node->v.line_count == 0) { continue; }
      LNK_LineTableBlock *block = &map->line_table_blocks[map->line_table_block_count++];
      block->raw_lines          = raw_lines;
      block->header             = push_array(arena, CV_C13LinesHeader, 1);
      *block->header = header_node->v;

      // resovle lines header
      {
        CV_C13LinesHeader *header      = block->header;
        U64                header_foff = lnk_obj_foff_from_section_data_ptr(obj, raw_lines.str) + header->header_off;

        U64 *sec_off = DebugRelocValueFromFileOffset(header_foff + OffsetOf(CV_C13SubSecLinesHeader, sec_off));
        if (sec_off) {
          U64 len            = header->sec_off_hi - header->sec_off_lo;
          header->sec_off_lo = *sec_off;
          header->sec_off_hi = *sec_off + len;
        }

        U64 *sec = DebugRelocValueFromFileOffset(header_foff + OffsetOf(CV_C13SubSecLinesHeader, sec));
        if (sec) {
          header->sec_idx = *sec;
        }
      }
    }
    temp_end(temp);
  }
  Assert(map->line_table_block_count <= line_table_blocks_cap);
  radsort(map->line_table_blocks, map->line_table_block_count, lnk_line_table_block_is_before);

  for (U64 block_idx = 0; block_idx < map->line_table_block_count;) {
    LNK_LineTableBlock *block   = &map->line_table_blocks[block_idx];
    U64                 key_min = Compose64Bit(block->header->sec_idx, block->header->sec_off_lo);
    U64                 key_max = Compose64Bit(block->header->sec_idx, block->header->sec_off_hi);

    U64 block_opl = block_idx + 1;
    for (; block_opl < map->line_table_block_count; block_opl += 1) {
      LNK_LineTableBlock *next = &map->line_table_blocks[block_opl];
      if (next->header->sec_idx    != block->header->sec_idx ||
          next->header->sec_off_lo != block->header->sec_off_lo ||
          next->header->sec_off_hi != block->header->sec_off_hi) {
        break;
      }
    }

    LNK_LineTable *table = &map->tables[map->table_count++];
    table->key_min       = key_min;
    table->key_max       = key_max;
    table->prefix_max    = map->table_count > 1 ? Max(map->tables[map->table_count - 2].prefix_max, key_max) : key_max;
    table->block_first   = block_idx;
    table->block_count   = block_opl - block_idx;

    block_idx = block_opl;
  }

  //
  // build inlinee id -> inlinee name
  //
  HashMap inlinee_hm = {0};
  if (obj->coff.debug_t_section_number) {
    String8      raw_debug_t = lnk_obj_section_data_from_number(obj, obj->coff.debug_t_section_number);
    CV_Signature debug_t_sig = cv_signature_from_debug_s(raw_debug_t);
    if (debug_t_sig == CV_Signature_C13) {
      CV_DebugT debug_t = cv_debug_t_from_data(scratch.arena, str8_skip(raw_debug_t, sizeof(CV_Signature)), CV_LeafAlign);

      CV_TypeIndex ti_base = CV_MinComplexTypeIndex;
      if (cv_debug_t_is_pch(&debug_t)) {
        CV_Leaf        precomp_leaf = cv_debug_t_get_leaf(&debug_t, 0);
        CV_PrecompInfo precomp      = cv_precomp_info_from_leaf(precomp_leaf);

        // discard LF_PRECOMP
        debug_t.offsets += 1;
        debug_t.count   -= 1;

        ti_base = precomp.start_index + precomp.leaf_count;
      }

      for EachIndex(leaf_idx, debug_t.count) {
        CV_Leaf leaf = cv_debug_t_get_leaf(&debug_t, leaf_idx);

        String8 name = str8_zero();
        if (leaf.kind == CV_LeafKind_FUNC_ID) {
          String8 data = str8_substr(leaf.data, r1u64(sizeof(CV_LeafFuncId), leaf.data.size));
          name = str8_cstring_capped(data.str, data.str + data.size);
        } else if (leaf.kind == CV_LeafKind_MFUNC_ID) {
          String8 data = str8_substr(leaf.data, r1u64(sizeof(CV_LeafMFuncId), leaf.data.size));
          name = str8_cstring_capped(data.str, data.str + data.size);
        }

        if (name.size) {
          U64 inlinee_itype = ti_base + leaf_idx;
          hash_map_push_u64_string(scratch.arena, &inlinee_hm, inlinee_itype, name);
        }
      }
    }
  }

  //
  // build inlinee lines accelerator
  //
  String8List                   raw_inlinee_lines   = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_InlineeLines);
  CV_C13InlineeLinesParsedList  inlinee_lines       = cv_c13_inlinee_lines_from_sub_sections(scratch.arena, raw_inlinee_lines);
  CV_InlineeLinesAccel         *inlinee_lines_accel = cv_c13_make_inlinee_lines_accel(scratch.arena, inlinee_lines);

  //
  // estimate required inline site capacity
  //
  String8List raw_symbols     = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Symbols);
  U64         inline_site_cap = 0;
  for EachNode(raw_symbols_node, String8Node, raw_symbols.first) {
    for (U64 cursor = 0; cursor < raw_symbols_node->string.size;) {
      CV_Symbol symbol = {0};
      TryReadBreak(cv_read_symbol(raw_symbols_node->string, cursor, CV_SymbolAlign, &symbol), cursor);
      inline_site_cap += symbol.kind == CV_SymKind_INLINESITE || symbol.kind == CV_SymKind_INLINESITE2;
    }
  }

  //
  // extract inline site info from the .debug$S sections
  //
  typedef struct LNK_InlineRangeNode LNK_InlineRangeNode;
  struct LNK_InlineRangeNode {
    LNK_InlineRangeNode *next;
    LNK_InlineRange      v;
  };
  LNK_InlineRangeNode *range_first = 0, *range_last = 0;
  map->inline_sites = push_array(arena, LNK_InlineSite, inline_site_cap);
  for EachNode(raw_symbols_node, String8Node, raw_symbols.first) {
    U32  proc_section            = 0;
    U32  proc_offset             = 0;
    U64  inline_site_stack_count = 0;
    U32 *inline_site_stack       = push_array_no_zero(scratch.arena, U32, inline_site_cap);
    for (U64 cursor = 0; cursor < raw_symbols_node->string.size;) {
      CV_Symbol symbol = {0};
      TryReadBreak(cv_read_symbol(raw_symbols_node->string, cursor, CV_SymbolAlign, &symbol), cursor);

      if (CV_IsProc32(symbol.kind)) {
        CV_SymProc32 *proc = str8_deserial_get_raw_ptr(symbol.data, 0, sizeof(*proc));
        if (proc) {
          U64  proc_foff = lnk_obj_foff_from_section_data_ptr(obj, symbol.data.str);
          U64 *off_reloc = DebugRelocValueFromFileOffset(proc_foff + OffsetOf(CV_SymProc32, off));
          U64 *sec_reloc = DebugRelocValueFromFileOffset(proc_foff + OffsetOf(CV_SymProc32, sec));
          proc_offset             = off_reloc ? safe_cast_u32(*off_reloc) : proc->off;
          proc_section            = sec_reloc ? safe_cast_u32(*sec_reloc) : proc->sec;
          inline_site_stack_count = 0;
        }
      } else if (symbol.kind == CV_SymKind_INLINESITE || symbol.kind == CV_SymKind_INLINESITE2) {
        U64 header_size = symbol.kind == CV_SymKind_INLINESITE ? sizeof(CV_SymInlineSite) : sizeof(CV_SymInlineSite2);
        if (proc_section && symbol.data.size >= header_size) {
          CV_ItemId  inlinee         = symbol.kind == CV_SymKind_INLINESITE ? ((CV_SymInlineSite *)symbol.data.str)->inlinee : ((CV_SymInlineSite2 *)symbol.data.str)->inlinee;
          String8    raw_annots      = str8_skip(symbol.data, header_size);
          String8   *inlinee_name    = hash_map_search_u64_string(&inlinee_hm, inlinee);
          U32        inline_site_idx = safe_cast_u32(map->inline_site_count++);

          LNK_InlineSite *inline_site     = &map->inline_sites[inline_site_idx];
          inline_site->parent_idx_plus_one = inline_site_stack_count ? inline_site_stack[inline_site_stack_count - 1] + 1 : 0;
          inline_site->depth               = inline_site->parent_idx_plus_one ? map->inline_sites[inline_site->parent_idx_plus_one - 1].depth + 1 : 1;
          inline_site->inlinee             = inlinee;
          inline_site->name                = inlinee_name ? *inlinee_name : str8_zero();
          inline_site_stack[inline_site_stack_count++] = inline_site_idx;

          CV_C13InlineeLinesParsed *inlinee_info = cv_c13_inlinee_lines_accel_find(inlinee_lines_accel, inlinee);
          if (inlinee_info) {
            //
            // decode inline site line table
            //
            typedef struct LNK_InlineLineNode LNK_InlineLineNode;
            struct LNK_InlineLineNode {
              LNK_InlineLineNode *next;
              CV_Line             v;
            };
            LNK_InlineLineNode      *line_first       = 0;
            LNK_InlineLineNode      *line_last        = 0;
            LNK_InlineRangeNode     *last_site_range  = 0;
            U32                      current_file_off = inlinee_info->file_off;
            CV_C13InlineSiteDecoder  decoder          = cv_c13_inline_site_decoder_init(inlinee_info->file_off, inlinee_info->first_source_ln, proc_offset);
            for (;;) {
              U64 old_cursor = decoder.cursor;

              CV_C13InlineSiteDecoderStep step = cv_c13_inline_site_decoder_step(&decoder, raw_annots);
              if (step.flags == 0 || decoder.cursor <= old_cursor) { break; }
              if (step.flags & CV_C13InlineSiteDecoderStepFlag_EmitFile) {
                current_file_off = step.file_off;
              }

              if (step.range.min < step.range.max) {
                if ((step.flags & CV_C13InlineSiteDecoderStepFlag_EmitRange) ||
                    ((step.flags & CV_C13InlineSiteDecoderStepFlag_ExtendLastRange) && last_site_range == 0)) {
                  LNK_InlineRangeNode *node = push_array(scratch.arena, LNK_InlineRangeNode, 1);
                  node->v.key_min  = Compose64Bit(proc_section, step.range.min);
                  node->v.key_max  = Compose64Bit(proc_section, step.range.max);
                  node->v.site_idx = inline_site_idx;
                  SLLQueuePush(range_first, range_last, node);
                  last_site_range = node;
                  map->inline_range_count += 1;
                }
              }

              if ((step.flags & CV_C13InlineSiteDecoderStepFlag_ExtendLastRange) && last_site_range) {
                last_site_range->v.key_max = Compose64Bit(proc_section, step.range.max);
              }

              if (step.flags & CV_C13InlineSiteDecoderStepFlag_EmitLine) {
                LNK_InlineLineNode *node = push_array(scratch.arena, LNK_InlineLineNode, 1);
                node->v.voff     = step.line_voff;
                node->v.file_off = current_file_off;
                node->v.line_num = step.ln;
                SLLQueuePush(line_first, line_last, node);
                inline_site->line_count += 1;
              }
            }

            // line list -> sorted line array
            inline_site->lines = push_array_no_zero(arena, CV_Line, inline_site->line_count);
            U64 line_idx = 0;
            for EachNode(line, LNK_InlineLineNode, line_first) { inline_site->lines[line_idx++] = line->v; }
            radsort(inline_site->lines, inline_site->line_count, cv_c13_voff_map_is_before);
          }
        }
      } else if (symbol.kind == CV_SymKind_CALLEES) {
        if (inline_site_stack_count && symbol.data.size >= sizeof(CV_SymFunctionList)) {
          CV_SymFunctionList *func_list    = (CV_SymFunctionList *)symbol.data.str;
          U64                 callee_count = Min(func_list->count, (symbol.data.size - sizeof(*func_list)) / sizeof(CV_ItemId));
          CV_ItemId          *callees      = (CV_ItemId *)(func_list + 1);

          LNK_InlineSite *inline_site = &map->inline_sites[inline_site_stack[inline_site_stack_count - 1]];
          inline_site->callee_count = callee_count;
          inline_site->callee_names = push_array(arena, String8, callee_count);
          for EachIndex(callee_idx, callee_count) {
            String8 *callee_name = hash_map_search_u64_string(&inlinee_hm, callees[callee_idx]);
            inline_site->callee_names[callee_idx] = callee_name ? *callee_name : str8_zero();
          }
        }
      } else if (symbol.kind == CV_SymKind_INLINESITE_END) {
        if (inline_site_stack_count > 0) {
          inline_site_stack_count -= 1;
        }
      } else if (symbol.kind == CV_SymKind_END || symbol.kind == CV_SymKind_PROC_ID_END) {
        proc_section            = 0;
        proc_offset             = 0;
        inline_site_stack_count = 0;
      }
    }
  }

  //
  // fill out & sort inline site ranges
  //
  {
    map->inline_ranges = push_array_no_zero(arena, LNK_InlineRange, map->inline_range_count);
    U64 range_idx = 0;
    for EachNode(range, LNK_InlineRangeNode, range_first) {
      map->inline_ranges[range_idx++] = range->v;
    }
    radsort(map->inline_ranges, map->inline_range_count, lnk_inline_range_is_before);

    for EachIndex(i, map->inline_range_count) {
      map->inline_ranges[i].prefix_max = i ?
        Max(map->inline_ranges[i - 1].prefix_max, map->inline_ranges[i].key_max) :
        map->inline_ranges[i].key_max;
    }
  }

#undef DebugRelocValueFromFileOff
  scratch_end(scratch);
  return map;
}

internal CV_Line *
lnk_lines_from_section_offset(LNK_ObjLineMap *map, U64 section_number, U64 offset, U64 *line_count_out)
{
  U64 key = Compose64Bit(safe_cast_u32(section_number), safe_cast_u32(offset));

  // upper bound search
  U64 min = 0;
  U64 opl = map->table_count;
  while (min < opl) {
    U64 mid = min + (opl - min) / 2;
    if (map->tables[mid].key_min <= key) {
      min = mid + 1;
    } else {
      opl = mid;
    }
  }

  // backward search until containing interval is found
  LNK_LineTable *table = 0;
  while (min > 0) {
    LNK_LineTable *candidate = &map->tables[--min];
    if (key < candidate->key_max) {
      table = candidate;
      break;
    }
    if (min == 0 || map->tables[min - 1].prefix_max <= key) {
      break;
    }
  }

  CV_Line *lines      = 0;
  U64      line_count = 0;
  if (table) {
    if (table->accel == 0) {
      Temp scratch = scratch_begin(&map->arena, 1);

      // unpack lines from line table blocks
      CV_LineArray *line_arrays = push_array_no_zero(scratch.arena, CV_LineArray, table->block_count);
      for EachIndex(i, table->block_count) {
        LNK_LineTableBlock *block = &map->line_table_blocks[table->block_first + i];
        CV_C13LinesHeader header = *block->header;
        header.sec_off_hi -= header.sec_off_lo;
        header.sec_off_lo = 0;
        line_arrays[i] = cv_c13_line_array_from_data(scratch.arena, block->raw_lines, 0, header);
      }

      // cache line table accel
      table->accel = cv_c13_make_lines_accel(map->arena, table->block_count, line_arrays);

      scratch_end(scratch);
    }

    // match (section, offset) => lines
    U32 table_offset = safe_cast_u32(key - table->key_min);
    lines = cv_line_from_voff(table->accel, table_offset, &line_count);
  }

  if (line_count_out) {
    *line_count_out = line_count;
  }

  return lines;
}

internal LNK_InlineSite *
lnk_inline_site_from_section_offset(LNK_ObjLineMap *map, U64 section_number, U64 offset, LNK_Symbol *callee_symbol)
{
  U64 key = Compose64Bit(safe_cast_u32(section_number), safe_cast_u32(offset));

  // find upper bound
  U64 min = 0;
  U64 opl = map->inline_range_count;
  while (min < opl) {
    U64 mid = min + (opl - min) / 2;
    if (map->inline_ranges[mid].key_min <= key) {
      min = mid + 1;
    } else {
      opl = mid;
    }
  }

  // hacky demangle
  String8 callee_name = callee_symbol ? callee_symbol->name : str8_zero();
  if (str8_starts_with(callee_name, str8_lit("__imp_"))) {
    callee_name = str8_skip(callee_name, 6);
  }
  if (str8_starts_with(callee_name, str8_lit("?"))) {
    U64 name_opl = str8_find_needle(callee_name, 1, str8_lit("@"), 0);
    callee_name = str8_substr(callee_name, r1u64(1, name_opl));
  }

  // scan back to the lower bound and match inline sites
  LNK_InlineSite *site                     = 0;
  LNK_InlineSite *callee_site              = 0;
  B32             callee_site_is_ambiguous = 0;
  while (min > 0) {
    LNK_InlineRange *range = &map->inline_ranges[--min];
    if (key < range->key_max) {
      LNK_InlineSite *candidate = &map->inline_sites[range->site_idx];

      if (site == 0 || site->depth < candidate->depth) {
        site = candidate;
      }

      for EachIndex(callee_idx, candidate->callee_count) {
        String8 pdb_callee_name = candidate->callee_names[callee_idx];
        if (str8_match(callee_name, pdb_callee_name, 0)) {

          // candidate matches callee
          if (callee_site == 0 || callee_site->depth < candidate->depth) {
            callee_site              = candidate;
            callee_site_is_ambiguous = 0;
          } else if (candidate->depth == callee_site->depth && candidate != callee_site) {
            callee_site_is_ambiguous = 1;
          }

          break;
        }
      }
    }

    if (min == 0 || map->inline_ranges[min - 1].prefix_max <= key) {
      break;
    }
  }

  return (callee_site && !callee_site_is_ambiguous) ? callee_site : site;
}

internal CV_Line *
lnk_line_from_inline_site(LNK_InlineSite *site, U32 offset)
{
  U64 min = 0;
  U64 opl = site->line_count;
  while (min < opl) {
    U64 mid = min + (opl - min) / 2;
    if (site->lines[mid].voff <= offset) {
      min = mid + 1;
    } else {
      opl = mid;
    }
  }
  return min > 0 ? &site->lines[min - 1] : 0;
}

