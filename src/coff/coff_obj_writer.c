internal COFF_ObjWriter*
coff_obj_writer_alloc(COFF_TimeStamp time_stamp, COFF_MachineType machine)
{
  Arena *arena = arena_alloc();
  COFF_ObjWriter *obj_writer = push_array(arena, COFF_ObjWriter, 1);
  obj_writer->arena          = arena;
  obj_writer->time_stamp     = time_stamp;
  obj_writer->machine   = machine;
  return obj_writer;
}

internal void
coff_obj_writer_release(COFF_ObjWriter **obj_writer)
{
  arena_release((*obj_writer)->arena);
  *obj_writer = 0;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol(COFF_ObjWriter *obj_writer, String8 name, U32 value, COFF_SymbolLocation loc, COFF_SymbolType type, COFF_SymStorageClass storage_class)
{
  COFF_ObjSymbolNode *n = push_array(obj_writer->arena, COFF_ObjSymbolNode, 1);
  SLLQueuePush(obj_writer->symbol_first, obj_writer->symbol_last, n);
  obj_writer->symbol_count += 1;

  COFF_ObjSymbol *s = &n->v;
  s->name           = name;
  s->value          = value;
  s->loc            = loc;
  s->type           = type;
  s->storage_class  = storage_class;

  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_extern(COFF_ObjWriter *obj_writer, String8 name, U32 value, COFF_ObjSection *section)
{
  COFF_SymbolLocation loc = {0};
  loc.type                = COFF_SymbolLocation_Section;
  loc.u.section           = section;
  COFF_ObjSymbol *s = coff_obj_writer_push_symbol(obj_writer, name, value, loc, (COFF_SymbolType){0}, COFF_SymStorageClass_External);
  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_static(COFF_ObjWriter *obj_writer, String8 name, U32 off, COFF_ObjSection *section)
{
  COFF_SymbolLocation loc = {0};
  loc.type                = COFF_SymbolLocation_Section;
  loc.u.section           = section;

  COFF_SymbolType symtype = {0};

  COFF_ObjSymbol *s = coff_obj_writer_push_symbol(obj_writer, name, off, loc, symtype, COFF_SymStorageClass_Static);
  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_secdef(COFF_ObjWriter *obj_writer, COFF_ObjSection *section, COFF_ComdatSelectType selection)
{
  COFF_ObjSymbol *s = coff_obj_writer_push_symbol_static(obj_writer, section->name, 0, section);

  COFF_ObjSymbolSecDef *sd = push_array(obj_writer->arena, COFF_ObjSymbolSecDef, 1);
  sd->selection            = selection;

  str8_list_push(obj_writer->arena, &s->aux_symbols, str8_struct(sd));

  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_weak(COFF_ObjWriter *obj_writer, String8 name, COFF_WeakExtType characteristics, COFF_ObjSymbol *tag)
{
  COFF_SymbolLocation loc     = {0};
  COFF_SymbolType     symtype = {0};
  COFF_ObjSymbol *s = coff_obj_writer_push_symbol(obj_writer, name, COFF_Symbol_UndefinedSection, loc, symtype, COFF_SymStorageClass_WeakExternal);

  COFF_ObjSymbolWeak *weak_ext = push_array(obj_writer->arena, COFF_ObjSymbolWeak, 1);
  weak_ext->tag                = tag;
  weak_ext->characteristics    = characteristics;

  str8_list_push(obj_writer->arena, &s->aux_symbols, str8_struct(weak_ext));

  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_abs(COFF_ObjWriter *obj_writer, String8 name, U32 value, COFF_SymStorageClass storage_class)
{
  COFF_SymbolLocation loc = {0};
  loc.type = COFF_SymbolLocation_Abs;
  COFF_ObjSymbol *s = coff_obj_writer_push_symbol(obj_writer, name, value, loc, (COFF_SymbolType){0}, storage_class);
  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_undef(COFF_ObjWriter *obj_writer, String8 name)
{
  COFF_SymbolType type = {0};
  COFF_SymbolLocation loc = {0};
  loc.type = COFF_SymbolLocation_Undef;
  COFF_ObjSymbol *s = coff_obj_writer_push_symbol(obj_writer, name, 0, loc, type, COFF_SymStorageClass_External);
  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_undef_section(COFF_ObjWriter *obj_writer, String8 name, COFF_SectionFlags flags)
{
  COFF_SymbolType type = {0};
  COFF_SymbolLocation loc = { COFF_SymbolLocation_Undef };
  COFF_ObjSymbol *s = coff_obj_writer_push_symbol(obj_writer, name, flags, loc, type, COFF_SymStorageClass_Section);
  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_undef_func(COFF_ObjWriter *obj_writer, String8 name)
{
  COFF_SymbolType type = {0};
  type.u.msb = COFF_SymDType_Func;
  COFF_SymbolLocation loc = {0};
  loc.type = COFF_SymbolLocation_Undef;
  COFF_ObjSymbol *s = coff_obj_writer_push_symbol(obj_writer, name, 0, loc, type, COFF_SymStorageClass_External);
  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_undef_sect(COFF_ObjWriter *obj_writer, String8 name, U32 value)
{
  COFF_SymbolType type = {0};
  COFF_SymbolLocation loc = {0};
  loc.type = COFF_SymbolLocation_Undef;
  COFF_ObjSymbol *s = coff_obj_writer_push_symbol(obj_writer, name, value, loc, type, COFF_SymStorageClass_Section);
  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_sect(COFF_ObjWriter *obj_writer, String8 name, COFF_ObjSection *sect)
{
  COFF_SymbolType type = {0};
  COFF_SymbolLocation loc = {0};
  loc.type      = COFF_SymbolLocation_Section;
  loc.u.section = sect;

  // strip align flags
  COFF_SectionFlags expected_flags = sect->flags & ~(COFF_SectionFlag_AlignMask << COFF_SectionFlag_AlignShift); 

  COFF_ObjSymbol *s = coff_obj_writer_push_symbol(obj_writer, name, expected_flags, loc, type, COFF_SymStorageClass_Section);
  return s;
}

internal COFF_ObjSymbol *
coff_obj_writer_push_symbol_common(COFF_ObjWriter *obj_writer, String8 name, U32 size)
{
  COFF_SymbolType type = {0};
  COFF_SymbolLocation loc = {0};
  loc.type = COFF_SymbolLocation_Common;
  COFF_ObjSymbol *s = coff_obj_writer_push_symbol(obj_writer, name, size, loc, type, COFF_SymStorageClass_External);
  return s;
}

internal COFF_ObjSection *
coff_obj_writer_push_section(COFF_ObjWriter *obj_writer, String8 name, COFF_SectionFlags flags, String8 data)
{
  COFF_ObjSectionNode *sect_n = push_array(obj_writer->arena, COFF_ObjSectionNode, 1);
  SLLQueuePush(obj_writer->sect_first, obj_writer->sect_last, sect_n);
  obj_writer->sect_count += 1;

  COFF_ObjSection *sect = &sect_n->v;
  sect->name            = name;
  sect->flags           = flags;

  str8_list_push(obj_writer->arena, &sect->data, data);

  return sect;
}

internal COFF_ObjReloc*
coff_obj_writer_section_push_reloc(COFF_ObjWriter *obj_writer, COFF_ObjSection *sect, U32 apply_off, COFF_ObjSymbol *symbol, COFF_RelocType type)
{
  COFF_ObjRelocNode *reloc_n = push_array(obj_writer->arena, COFF_ObjRelocNode, 1);
  SLLQueuePush(sect->reloc_first, sect->reloc_last, reloc_n);
  sect->reloc_count += 1;

  COFF_ObjReloc *reloc = &reloc_n->v;
  reloc->apply_off     = apply_off;
  reloc->symbol        = symbol;
  reloc->type          = type;

  return reloc;
}

internal void
coff_obj_writer_push_directive(COFF_ObjWriter *obj_writer, String8 directive)
{
  if (obj_writer->drectve_sect == 0) {
    local_persist const U8 bom_sig[]  = { ' ', ' ', ' ' };
    obj_writer->drectve_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".drectve"), COFF_SectionFlag_LnkInfo|COFF_SectionFlag_LnkRemove|COFF_SectionFlag_Align1Bytes, str8_array_fixed(bom_sig));
  }
  String8List *data = &obj_writer->drectve_sect->data;
  str8_list_push(obj_writer->arena, data, directive);
  str8_list_pushf(obj_writer->arena, data, " ");
}

internal int
coff_obj_section_is_before(void *raw_a, void *raw_b)
{
  COFF_ObjSection **a = raw_a;
  COFF_ObjSection **b = raw_b;
  return (*a)->section_number < (*b)->section_number;
}

internal String8
coff_obj_writer_serialize(Arena *arena, COFF_ObjWriter *obj_writer)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List srl = {0};

  String8List string_table = {0};
  U32 *string_table_size = push_array(scratch.arena, U32, 1);
  *string_table_size = sizeof(*string_table_size);
  str8_list_push(scratch.arena, &string_table, str8_struct(string_table_size));

  //
  // assing section numbers
  //
  U64               obj_sections_count;
  COFF_ObjSection **obj_sections;
  {
    obj_sections_count = obj_writer->sect_count;
    obj_sections       = push_array(scratch.arena, COFF_ObjSection *, obj_writer->sect_count);
    U64 sect_idx = 0;
    for (COFF_ObjSectionNode *sect_n = obj_writer->sect_first; sect_n != 0; sect_n = sect_n->next, sect_idx += 1) {
      COFF_ObjSection *sect = &sect_n->v;
      sect->section_number = sect_idx+1;
      obj_sections[sect_idx] = sect;

    }
  }
  AssertAlways(obj_sections_count <= max_U16);

  //
  // serialize symbol table
  //
  String8List symbol_table = {0};
  {
    U64 symbol_idx = 0;
    for (COFF_ObjSymbolNode *symbol_n = obj_writer->symbol_first; symbol_n != 0; symbol_n = symbol_n->next) {
      COFF_ObjSymbol *s = &symbol_n->v;

      // assign symbol index
      s->idx = symbol_idx++;

      COFF_Symbol16 *d = push_array(scratch.arena, COFF_Symbol16, 1);
      str8_list_push(scratch.arena, &symbol_table, str8_struct(d));

      COFF_SymbolName name = {0};
      // long name
      if (s->name.size > sizeof(name.short_name)) {
        U64 string_table_offset = string_table.total_size;
        str8_list_push_cstr(scratch.arena, &string_table, s->name);

        name.long_name.zeroes = 0;
        name.long_name.string_table_offset = safe_cast_u32(string_table_offset);
      }
      // short name
      else {
        MemoryCopyStr8(name.short_name, s->name);
        MemoryZeroTyped(name.short_name + s->name.size, sizeof(name.short_name) - s->name.size);
      }

      // symbol header
      AssertAlways(s->aux_symbols.node_count <= max_U8);
      d->name             = name;
      d->value            = s->value;
      switch (s->loc.type) {
      case COFF_SymbolLocation_Null: break;
      case COFF_SymbolLocation_Section: d->section_number = safe_cast_u16(s->loc.u.section->section_number); break;
      case COFF_SymbolLocation_Abs:     d->section_number = COFF_Symbol_AbsSection16;         break;
      case COFF_SymbolLocation_Undef:   d->section_number = COFF_Symbol_UndefinedSection;     break;
      }
      d->type             = s->type;
      d->storage_class    = s->storage_class;
      d->aux_symbol_count = 0;

      U64 start_symbol_idx = symbol_idx;
      if (s->storage_class == COFF_SymStorageClass_WeakExternal) {
        if (s->aux_symbols.node_count > 0) {
          COFF_ObjSymbolWeak *s_weak = (COFF_ObjSymbolWeak *)s->aux_symbols.first->string.str;
          COFF_SymbolWeakExt *d_weak = push_array(scratch.arena, COFF_SymbolWeakExt, 1);
          d_weak->tag_index       = s_weak->tag->idx;
          d_weak->characteristics = s_weak->characteristics;

          str8_list_push(scratch.arena, &symbol_table, str8_struct(d_weak));
          symbol_idx += 1;
        }
      } else if (s->storage_class == COFF_SymStorageClass_Static) {
        if (s->aux_symbols.node_count > 0) {
          Assert(s->loc.type == COFF_SymbolLocation_Section);
          COFF_ObjSection *sect = s->loc.u.section;

          COFF_ObjSymbolSecDef *s_sd = (COFF_ObjSymbolSecDef *)s->aux_symbols.first->string.str;
          COFF_SymbolSecDef    *d_sd = push_array(scratch.arena, COFF_SymbolSecDef, 1);

          d_sd->length                = safe_cast_u32(sect->data.total_size);
          d_sd->number_of_relocations = (U16)sect->reloc_count;
          d_sd->check_sum             = 0;
          d_sd->number_lo             = (U16)sect->section_number;
          d_sd->selection             = s_sd->selection;

          str8_list_push(scratch.arena, &symbol_table, str8_struct(d_sd));
          symbol_idx += 1;
        }
      }

      U8 processed_aux_symbol_count = (U8)(symbol_idx - start_symbol_idx);

      for (U64 aux_idx = processed_aux_symbol_count; aux_idx < s->aux_symbols.node_count; aux_idx += 1) {
        COFF_Symbol16 *a = push_array(scratch.arena, COFF_Symbol16, 1);
        str8_list_push(scratch.arena, &symbol_table, str8_struct(a));
      }

      d->aux_symbol_count = (U8)s->aux_symbols.node_count;
    }
  }

  //
  // file header
  //
  COFF_FileHeader *file_header      = push_array(scratch.arena, COFF_FileHeader, 1);
  file_header->machine              = obj_writer->machine;
  file_header->section_count        = obj_sections_count;
  file_header->time_stamp           = obj_writer->time_stamp;
  file_header->symbol_table_foff    = 0;
  file_header->symbol_count         = safe_cast_u32(symbol_table.node_count);
  file_header->optional_header_size = 0;
  file_header->flags                = 0;
  str8_list_push(scratch.arena, &srl, str8_struct(file_header));

  //
  // section table
  //

  COFF_SectionHeader *sectab = push_array(scratch.arena, COFF_SectionHeader, obj_sections_count);
  str8_list_push(scratch.arena, &srl, str8_array(sectab, obj_sections_count));
  {
    for (U64 sect_idx = 0; sect_idx < obj_sections_count; sect_idx += 1) {
      COFF_ObjSection    *s = obj_sections[sect_idx];
      COFF_SectionHeader *d = &sectab[sect_idx];

      // section name
      String8 sect_name = s->name;
      if (sect_name.size > sizeof(d->name)) {
        U64 sect_name_off = string_table.total_size;
        str8_list_push_cstr(scratch.arena, &string_table, sect_name);

        sect_name = push_str8f(scratch.arena, "/%u", sect_name_off);
        AssertAlways(sect_name.size <= sizeof(d->name));
      }

      // section data
      U64 data_foff = 0;
      U64 data_size = 0;
      if (s->data.total_size > 0) {
        data_foff = srl.total_size;
        data_size = s->data.total_size;
        str8_list_concat_in_place(&srl, &s->data);
      }

      // section relocs
      U64 relocs_foff = 0;
      if (s->reloc_count) {
        AssertAlways(s->reloc_count <= max_U16);
        COFF_Reloc *relocs    = push_array(scratch.arena, COFF_Reloc, s->reloc_count);
        U64         reloc_idx = 0;
        for (COFF_ObjRelocNode *reloc_n = s->reloc_first; reloc_n != 0; reloc_n = reloc_n->next, reloc_idx += 1) {
          COFF_ObjReloc *rs = &reloc_n->v;
          COFF_Reloc    *rd = &relocs[reloc_idx];
          rd->apply_off = rs->apply_off;
          rd->isymbol   = rs->symbol->idx;
          rd->type      = rs->type;
        }
        relocs_foff = srl.total_size;
        str8_list_push(scratch.arena, &srl, str8_array(relocs, s->reloc_count));
      }

      // section header
      MemoryCopyStr8(d->name, sect_name);
      MemoryZeroTyped(d->name + sect_name.size, sizeof(d->name) - sect_name.size);
      d->vsize       = 0;
      d->voff        = 0;
      d->fsize       = data_size;
      d->foff        = data_foff;
      d->relocs_foff = relocs_foff;
      d->lines_foff  = 0;
      d->reloc_count = safe_cast_u32(s->reloc_count);
      d->line_count  = 0;
      d->flags       = s->flags;
    }
  }

  //
  // symbol table
  //
  if (symbol_table.total_size || string_table.total_size > sizeof(*string_table_size)) {
    file_header->symbol_table_foff = srl.total_size;
    str8_list_concat_in_place(&srl, &symbol_table);
  }

  //
  // string table
  //
  if (string_table.total_size) {
    *string_table_size = safe_cast_u32(string_table.total_size);
    str8_list_concat_in_place(&srl, &string_table);
  }

  //
  // join
  //
  String8 obj = str8_list_join(arena, &srl, 0);

  scratch_end(scratch);
  return obj;
}


