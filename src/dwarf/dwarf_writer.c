// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal DW_IntEnc
dw_int_enc_from_sint(S64 v)
{
  if ((S8)v == v)  return DW_IntEnc_1Byte;
  if ((S16)v == v) return DW_IntEnc_2Byte;
  if ((S32)v == v) return DW_IntEnc_4Byte;
  return DW_IntEnc_LEB128;
}

internal DW_IntEnc
dw_int_enc_from_uint(U64 v)
{
  if ((U8)v == v)  return DW_IntEnc_1Byte;
  if ((U16)v == v) return DW_IntEnc_2Byte;
  if ((U32)v == v) return DW_IntEnc_4Byte;
  return DW_IntEnc_LEB128;
}

internal U64
dw_size_from_sint(S64 v)
{
  DW_IntEnc enc = dw_int_enc_from_sint(v);
  switch (enc) {
  case DW_IntEnc_Null: break;
  case DW_IntEnc_1Byte:  return 1;
  case DW_IntEnc_2Byte:  return 2;
  case DW_IntEnc_4Byte:  return 4;
  case DW_IntEnc_LEB128: return dw_size_from_sleb128(v);
  default: InvalidPath; break;
  }
  return 0;
}

internal U64
dw_size_from_uint(U64 v)
{
  DW_IntEnc enc = dw_int_enc_from_uint(v);
  switch (enc) {
  case DW_IntEnc_Null: break;
  case DW_IntEnc_1Byte:  return 1;
  case DW_IntEnc_2Byte:  return 2;
  case DW_IntEnc_4Byte:  return 4;
  case DW_IntEnc_LEB128: return dw_size_from_uleb128(v);
  default: InvalidPath; break;
  }
  return 0;
}

internal DW_WriterFixupNode *
dw_writer_fixup_list_push(Arena *arena, DW_WriterFixupList *list, DW_WriterFixup v)
{
  DW_WriterFixupNode *n = push_array(arena, DW_WriterFixupNode, 1);
  n->v = v;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  return n;
}

internal DW_WriterAttrib *
dw_writer_attrib_chunk_list_push(Arena *arena, DW_WriterAttribChunkList *list, U64 cap)
{
  if (list->last == 0 || list->last->count >= list->last->max) {
    DW_WriterAttribChunk *n = push_array(arena, DW_WriterAttribChunk, 1);
    n->v = push_array_no_zero(arena, DW_WriterAttrib, cap);
    n->max = cap;
    SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }

  DW_WriterAttrib *result = &list->last->v[list->last->count++];
  MemoryZeroStruct(result);

  list->total_count += 1;

  return result;
}

internal DW_WriterTag *
dw_writer_tag_chunk_list_push(Arena *arena, DW_WriterTagChunkList *list, U64 cap)
{
  if (list->last == 0 || list->last->count >= list->last->max) {
    DW_WriterTagChunk *n = push_array(arena, DW_WriterTagChunk, 1);
    n->v = push_array_no_zero(arena, DW_WriterTag, cap);
    n->max = cap;
    SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }

  DW_WriterTag *result = &list->last->v[list->last->count++];
  MemoryZeroStruct(result);

  list->total_count += 1;

  return result;
}

internal DW_Writer *
dw_writer_begin(DW_Format format, DW_Version version, DW_CompUnitKind cu_kind, Arch arch)
{
  Arena *arena = arena_alloc();
  DW_Writer *writer = push_array(arena, DW_Writer, 1);
  writer->arena                 = arena;
  writer->arch                  = arch;
  writer->format                = format;
  writer->version               = version;
  writer->cu_kind               = cu_kind;
  writer->address_size          = byte_size_from_arch(arch);
  writer->current               = 0;
  writer->abbrev_id_map         = hash_table_init(arena, 0x2000);
  writer->line.min_inst_len     = min_instruction_size_from_arch(arch);
  writer->line.max_ops_per_inst = max_ops_per_instruction_from_arch(arch);
  writer->line.default_is_stmt  = 1;
  writer->line.line_base        = -5;
  writer->line.line_range       = 14;
  writer->line.opcode_base      = DW_StdOpcode_Count;
  writer->line.ln               = 1;
  Assert(writer->line.line_base < writer->line.line_range);
  for EachElement(i, writer->sections) {
    str8_serial_begin(arena, &writer->sections[i].srl);
  }

  // write info header
  {
    String8List *srl = &writer->sections[DW_Section_Info].srl;

    // length
    writer->sections[DW_Section_Info].length = dw_serial_push_length(writer->arena, srl, writer->format, 0);

    // version
    str8_serial_push_struct(writer->arena, srl, &writer->version);

    // compile unit kind
    str8_serial_push_struct(writer->arena, srl, &writer->cu_kind);

    // address size
    str8_serial_push_u8(writer->arena, srl, (U8)writer->address_size);
    
    // reserve 'abbrev_base'
    writer->abbrev_base_info_off = srl->total_size;
    dw_serial_push_uint(writer->arena, srl, writer->format, 0);

    if (writer->cu_kind == DW_CompUnitKind_Skeleton) {
      // TODO: write skeleton id
      NotImplemented;
    }
  }

  return writer;
}

internal void
dw_writer_end(DW_Writer **writer_ptr)
{
  arena_release((*writer_ptr)->arena);
  *writer_ptr = 0;
}

internal U64
dw_serial_push_form(Arena *arena, String8List *srl, DW_Version version, DW_Format format, U8 address_size, DW_WriterFixupList *fixups, DW_WriterXForm form)
{
  U64 start_off = srl->total_size;
  switch (form.reader.kind) {
  case DW_Form_Addr: {
    str8_serial_push_string(arena, srl, form.reader.addr);
  } break;
  case DW_Form_Block1:
  case DW_Form_Block2:
  case DW_Form_Block4: {
    str8_serial_push_string(arena, srl, form.reader.block);
  } break;
  case DW_Form_Data1:
  case DW_Form_Data2:
  case DW_Form_Data4:
  case DW_Form_Data8: {
    str8_serial_push_string(arena, srl, form.reader.data);
  } break;
  case DW_Form_String: {
    str8_serial_push_cstr(arena, srl, form.reader.string);
  } break;
  case DW_Form_Block: {
    dw_serial_push_uleb128(arena, srl, form.reader.block.size);
    str8_serial_push_string(arena, srl, form.reader.block);
  } break;
  case DW_Form_Flag: {
    str8_serial_push_u8(arena, srl, form.reader.flag);
  } break;
  case DW_Form_SData: {
    dw_serial_push_sleb128(arena, srl, form.reader.sdata);
  } break;
  case DW_Form_Strp: {
    dw_serial_push_uint(arena, srl, format, form.reader.sec_offset);
  } break;
  case DW_Form_UData: {
    dw_serial_push_uleb128(arena, srl, form.reader.udata);
  } break;
  case DW_Form_RefAddr: {
    if (version < DW_Version_3) {
      Assert(address_size <= sizeof(form.reader.ref));
      str8_serial_push_string(arena, srl, str8((U8 *)&form.reader.ref, address_size));
    } else {
      dw_serial_push_uint(arena, srl, format, form.reader.ref);
    }
  } break;
  case DW_Form_Ref1: {
    str8_serial_push_u8(arena, srl, (U8)form.reader.ref);
  } break;
  case DW_Form_Ref2: {
    str8_serial_push_u16(arena, srl, (U16)form.reader.ref);
  } break;
  case DW_Form_Ref4: {
    str8_serial_push_u32(arena, srl, (U32)form.reader.ref);
  } break;
  case DW_Form_Ref8: {
    if (form.writer.ref->info_off == 0) {
      // reserve 8 bytes
      void *value = str8_serial_push_u64(arena, srl, 0);

      // push fixup
      dw_writer_fixup_list_push(arena, fixups, (DW_WriterFixup){ .ptr = value, .tag = form.writer.ref });
    }
  } break;
  case DW_Form_RefUData: {
    dw_serial_push_uleb128(arena, srl, form.reader.ref);
  } break;
  case DW_Form_Indirect: {
    NotImplemented;
  } break;
  case DW_Form_SecOffset: {
    dw_serial_push_uint(arena, srl, format, form.reader.sec_offset);
  } break;
  case DW_Form_ExprLoc: {
    dw_serial_push_uleb128(arena, srl, form.reader.exprloc.size);
    str8_serial_push_string(arena, srl, form.reader.exprloc);
  } break;
  case DW_Form_FlagPresent: {
  } break;
  case DW_Form_RefSig8: {
    NotImplemented;
  } break;
  case DW_Form_Strx:
  case DW_Form_Addrx:
  case DW_Form_RngListx:
  case DW_Form_LocListx: {
    dw_serial_push_uleb128(arena, srl, form.reader.xval);
  } break;
  case DW_Form_RefSup4: {
    NotImplemented;
  } break;
  case DW_Form_StrpSup: {
    dw_serial_push_uint(arena, srl, format, form.reader.strp_sup);
  } break;
  case DW_Form_Data16: {
    str8_serial_push_string(arena, srl, form.reader.data);
  } break;
  case DW_Form_LineStrp: {
    dw_serial_push_uint(arena, srl, format, form.reader.sec_offset);
  } break;
  case DW_Form_ImplicitConst: {
    // value is stored in the abbrev entry
  } break;
  case DW_Form_RefSup8: {
    NotImplemented;
  } break;
  case DW_Form_Strx1:
  case DW_Form_Addrx1: {
    str8_serial_push_u8(arena, srl, (U8)form.reader.xval);
  } break;
  case DW_Form_Strx2:
  case DW_Form_Addrx2: {
    str8_serial_push_u16(arena, srl, (U16)form.reader.xval);
  } break;
  case DW_Form_Strx3:
  case DW_Form_Addrx3: {
    str8_serial_push_string(arena, srl, str8((U8 *)&form.reader.xval, 3));
  } break;
  case DW_Form_Strx4:
  case DW_Form_Addrx4: {
    str8_serial_push_u32(arena, srl, (U32)form.reader.xval);
  } break;

  case DW_Form_GNU_StrpAlt: {
    dw_serial_push_uint(arena, srl, format, form.reader.sec_offset);
  } break;
  case DW_Form_GNU_RefAlt: {
    dw_serial_push_uint(arena, srl, format, form.reader.ref);
  } break;

  default: InvalidPath; break;
  }
  return srl->total_size - start_off;
}

internal String8
dw_make_abbrev_entry(Arena *arena, DW_WriterTag *tag)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List srl = {0};
  str8_serial_begin(scratch.arena, &srl);

  // tag abbrev
  B8 has_children = tag->first_child != 0;
  dw_serial_push_uleb128(scratch.arena, &srl, tag->kind);
  str8_serial_push_string(scratch.arena, &srl, str8_struct(&has_children)); 

  // attrib abbrev
  for EachNode(attrib, DW_WriterAttrib, tag->first_attrib) {
    dw_serial_push_uleb128(scratch.arena, &srl, attrib->kind);
    dw_serial_push_uleb128(scratch.arena, &srl, attrib->form.reader.kind);
    if (attrib->form.reader.kind == DW_Form_ImplicitConst) {
      dw_serial_push_sleb128(scratch.arena, &srl, attrib->form.reader.implicit_const);
    }
  }

  // closing entry
  dw_serial_push_uleb128(scratch.arena, &srl, 0);
  dw_serial_push_uleb128(scratch.arena, &srl, 0);

  String8 tag_abbrev = str8_serial_end(arena, &srl);

  scratch_end(scratch);
  return tag_abbrev;
}

internal DW_WriterTag *
dw_writer_tag_begin(DW_Writer *writer, DW_TagKind kind)
{
  DW_WriterTag *tag = dw_writer_tag_chunk_list_push(writer->arena, &writer->tag_chunk_list, 512);
  tag->kind = kind;
  tag->parent = writer->current;

  // add tag to the tree
  if (writer->current) {
    SLLQueuePush(writer->current->first_child, writer->current->last_child, tag);
  } else {
    writer->root = tag;
  }
  writer->current = tag;
  
  return tag;
}

internal void
dw_writer_tag_end(DW_Writer *writer)
{
  // pop to parent
  writer->current = writer->current->parent;
}

internal DW_WriterAttrib *
dw_writer_push_attrib(DW_Writer *writer, DW_AttribKind kind, DW_WriterForm form)
{
  // validate attribute-form encoding
  B32            is_enc_legal    = 0;
  DW_AttribClass attrib_class = dw_attrib_class_from_attrib(writer->version, DW_Ext_Null, kind);
  for (DW_AttribClass f = attrib_class; f != 0; f &= f - 1) {
    DW_AttribClass v = f & -f;
    switch (v) {
    case DW_AttribClass_Null:          { is_enc_legal = (form.kind == DW_WriterFormKind_Null);                                        } break;
    case DW_AttribClass_Undefined:     { is_enc_legal = 0;                                                                            } break;
    case DW_AttribClass_Address:       { is_enc_legal = (form.kind == DW_WriterFormKind_Address);                                     } break;
    case DW_AttribClass_Block:         { is_enc_legal = (form.kind == DW_WriterFormKind_Block);                                       } break;
    case DW_AttribClass_Const:         { is_enc_legal = (form.kind == DW_WriterFormKind_SInt ||
                                                         form.kind == DW_WriterFormKind_UInt ||
                                                         form.kind == DW_WriterFormKind_Implicit);                                    } break;
    case DW_AttribClass_ExprLoc:       { is_enc_legal = (form.kind == DW_WriterFormKind_ExprLoc);                                     } break;
    case DW_AttribClass_Flag:          { is_enc_legal = (form.kind == DW_WriterFormKind_Flag);                                        } break;
    case DW_AttribClass_LinePtr:       { is_enc_legal = (form.kind == DW_WriterFormKind_LinePtr);                                     } break;
    case DW_AttribClass_LocListPtr:    { is_enc_legal = 0; /* TODO */                                                                 } break;
    case DW_AttribClass_MacPtr:        { is_enc_legal = (form.kind == DW_WriterFormKind_MacPtr);                                      } break;
    case DW_AttribClass_RngListPtr:    { is_enc_legal = (form.kind == DW_WriterFormKind_RngListPtr);                                  } break;
    case DW_AttribClass_Reference:     { is_enc_legal = (form.kind == DW_WriterFormKind_Ref);                                         } break;
    case DW_AttribClass_String:        { is_enc_legal = (form.kind == DW_WriterFormKind_String);                                      } break;
    case DW_AttribClass_LocList:       { is_enc_legal = 0; /* TODO */                                                                 } break;
    case DW_AttribClass_RngList:       { is_enc_legal = 0; /* TODO */                                                                 } break;
    case DW_AttribClass_StrOffsetsPtr: { is_enc_legal = 0; /* TODO */                                                                 } break;
    case DW_AttribClass_AddrPtr:       { is_enc_legal = 0; /* TODO */                                                                 } break;
    default:                           { InvalidPath;                                                                                 } break;
    }
    if (is_enc_legal) { break; }
  }
  Assert(is_enc_legal);

  DW_WriterAttrib *attrib = dw_writer_attrib_chunk_list_push(writer->arena, &writer->attrib_chunk_list, 512);
  attrib->kind        = kind;
  attrib->form.writer = form;
  SLLQueuePush(writer->current->first_attrib, writer->current->last_attrib, attrib);
  writer->current->attrib_count += 1;
  return attrib;
}

internal DW_WriterAttrib *
dw_writer_push_attrib_address(DW_Writer *writer, DW_AttribKind kind, U64 address)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_Address, .address = address });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_block(DW_Writer *writer, DW_AttribKind kind, String8 block)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_Block, .block = str8_copy(writer->arena, block) });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_data(DW_Writer *writer, DW_AttribKind kind, String8 data)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_Data, .data = str8_copy(writer->arena, data) });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_string(DW_Writer *writer, DW_AttribKind kind, String8 string)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_String, .string = string });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_stringf(DW_Writer *writer, DW_AttribKind kind, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(writer->arena, fmt, args);
  va_end(args);
  return dw_writer_push_attrib_string(writer, kind, string);
}

internal DW_WriterAttrib *
dw_writer_push_attrib_flag(DW_Writer *writer, DW_AttribKind kind, B8 flag)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_Flag, .flag = flag });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_sint(DW_Writer *writer, DW_AttribKind kind, S64 sint)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_SInt, .sint = sint});
}

internal DW_WriterAttrib *
dw_writer_push_attrib_uint(DW_Writer *writer, DW_AttribKind kind, U64 uint)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_UInt, .uint = uint });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_enum(DW_Writer *writer, DW_AttribKind kind, S64 e)
{
  return dw_writer_push_attrib_sint(writer, kind, e);
}

internal DW_WriterAttrib *
dw_writer_push_attrib_ref(DW_Writer *writer, DW_AttribKind kind, DW_WriterTag *ref)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_Ref, .ref = ref });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_exprloc(DW_Writer *writer, DW_AttribKind kind, String8 exprloc)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_ExprLoc, .exprloc = str8_copy(writer->arena, exprloc) });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_expression(DW_Writer *writer, DW_AttribKind kind, DW_ExprEnc *encs, U64 encs_count)
{
  String8 exprloc = dw_encode_expr(writer->arena, writer->arch, writer->format, encs, encs_count);
  return dw_writer_push_attrib_exprloc(writer, kind, exprloc);
}

internal DW_WriterAttrib *
dw_writer_push_attrib_line_ptr(DW_Writer *writer, DW_AttribKind kind, U64 line_ptr)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_LinePtr, .line_ptr = line_ptr });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_mac_ptr(DW_Writer *writer, DW_AttribKind kind, void *mac_ptr)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_MacPtr, .mac_ptr = mac_ptr });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_rng_list_ptr(DW_Writer *writer, DW_AttribKind kind, void *rng_list_ptr)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_RngListPtr, .rng_list_ptr = rng_list_ptr });
}

internal DW_WriterAttrib *
dw_writer_push_attrib_implicit(DW_Writer *writer, DW_AttribKind kind, S64 implicit)
{
  return dw_writer_push_attrib(writer, kind, (DW_WriterForm){ .kind = DW_WriterFormKind_Implicit, .implicit = implicit });
}

internal void
dw_line_inst_list_push_node(DW_LineInstList *list, DW_LineInstNode *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal DW_LineInstNode *
dw_line_inst_list_push(Arena *arena, DW_LineInstList *list, DW_LineInst v)
{
  DW_LineInstNode *node = push_array(arena, DW_LineInstNode, 1);
  node->v = v;
  dw_line_inst_list_push_node(list, node);
  return node;
}

internal String8
dw_data_from_line_insts(Arena *arena, U8 address_size, DW_LineInstList insts)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List srl = {0};
  str8_serial_begin(scratch.arena, &srl);

  for EachNode(inst_n, DW_LineInstNode, insts.first) {
    DW_LineInst *inst = &inst_n->v;

    str8_serial_push_struct(scratch.arena, &srl, &inst->opcode);
    switch (inst->opcode) {
    case DW_StdOpcode_Copy: {} break;
    case DW_StdOpcode_AdvancePc: {
      dw_serial_push_uleb128(arena, &srl, inst->advance_pc);
    } break;
    case DW_StdOpcode_AdvanceLine: {
      dw_serial_push_sleb128(arena, &srl, inst->advance_line);
    } break;
    case DW_StdOpcode_SetFile: {
      dw_serial_push_uleb128(arena, &srl, inst->set_file->file_idx);
    } break;
    case DW_StdOpcode_SetColumn: {
      dw_serial_push_uleb128(arena, &srl, inst->set_column);
    } break;
    case DW_StdOpcode_NegateStmt: {} break;
    case DW_StdOpcode_SetBasicBlock: {} break;
    case DW_StdOpcode_ConstAddPc: {} break;
    case DW_StdOpcode_FixedAdvancePc: {
      str8_serial_push_u16(arena, &srl, inst->fixed_advance_pc);
    } break;
    case DW_StdOpcode_SetPrologueEnd: {} break;
    case DW_StdOpcode_SetEpilogueBegin: {} break;
    case DW_StdOpcode_SetIsa: {
      dw_serial_push_uleb128(arena, &srl, inst->set_isa);
    } break;
    case DW_StdOpcode_ExtendedOpcode: {
      // get ext size
      U64 ext_size = 0;
      switch (inst->ext) {
      case DW_ExtOpcode_EndSequence: { ext_size = 0; } break;
      case DW_ExtOpcode_SetAddress:  { ext_size = address_size; } break;
      case DW_ExtOpcode_DefineFile: { 
        NotImplemented;
      } break;
      case DW_ExtOpcode_SetDiscriminator: {
        ext_size = dw_size_from_uleb128(inst->set_discriminator);
      } break;
      default: { InvalidPath; } break;
      }
      ext_size += sizeof(DW_ExtOpcode);

      // write ext header
      dw_serial_push_uleb128(arena, &srl, ext_size);
      str8_serial_push_struct(arena, &srl, &inst->ext);

      // write ext operands
      switch (inst->ext) {
      case DW_ExtOpcode_EndSequence: {} break;
      case DW_ExtOpcode_SetAddress: {
        str8_serial_push_string(arena, &srl, str8_copy(arena, str8((U8 *)&inst->set_address, address_size)));
      } break;
      case DW_ExtOpcode_DefineFile: {
        NotImplemented;
      } break;
      case DW_ExtOpcode_SetDiscriminator: {
        dw_serial_push_uleb128(arena, &srl, inst->set_discriminator);
      } break;
      default: { InvalidPath; } break;
      }
    } break;
    // special opcode
    default: { } break;
    }
  }

  String8 data = str8_serial_end(arena, &srl);
  scratch_end(scratch);
  return data;
}

internal void
dw_writer_line_emit(DW_Writer *writer, DW_WriterFile *file, U64 ln, U64 col, U64 addr)
{
  if (writer->line.file != file) {
    dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNS_set_file(file));
  }

  S64 col_delta = (S64)col - (S64)writer->line.col;
  if (col_delta != 0) { dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNS_set_column(col_delta)); }

  Assert(addr <= max_S64);
  S64 addr_delta = (S64)addr - (S64)writer->line.addr;
  S64 ln_delta   = (S64)ln   - (S64)writer->line.ln;

  S64 max_ln_delta   = writer->line.line_range + writer->line.line_base;
  S64 max_addr_delta = (max_U8 - writer->line.opcode_base - (ln_delta - writer->line.line_base)) / writer->line.line_range;
  if ((ln_delta != 0 || addr_delta != 0) &&
      writer->line.line_base <= ln_delta && ln_delta < max_ln_delta && // check line number window
      0 <= addr_delta && addr_delta <= max_addr_delta) {               // check address window
    U8 opcode = (ln_delta - writer->line.line_base) + (addr_delta * writer->line.line_range) + writer->line.opcode_base;
    dw_line_inst_list_push(writer->arena, &writer->line.line_insts, (DW_LineInst){ .opcode = opcode });
  } else {
    if (addr_delta != 0) { dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNS_advance_pc(addr_delta)); }
    if (ln_delta   != 0) { dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNS_advance_line(ln_delta)); }
    if (addr_delta != 0 || ln_delta != 0 || col_delta != 0) {
      dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNS_copy());
    }
  }

  // update registers
  writer->line.file = file;
  writer->line.ln   = ln;
  writer->line.col  = col;
  writer->line.addr = addr;
}

internal void
dw_writer_line_set_address(DW_Writer *writer, U64 address)
{
  dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNE_set_address(address));
}

internal void
dw_writer_line_set_discriminator(DW_Writer *writer, U64 d)
{
  dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNE_set_discriminator(d));
}

internal void
dw_writer_line_end_sequence(DW_Writer *writer)
{
  writer->line.file = 0;
  writer->line.ln   = 1;
  writer->line.addr = 0;
  writer->line.col  = 0;
  dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNE_end_sequence());
}

internal void
dw_writer_line_set_prologue_end(DW_Writer *writer)
{
  dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNS_set_prologue_end());
}

internal void
dw_writer_line_epilogue_begin(DW_Writer *writer)
{
  dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNS_set_epilogue_begin());
}

internal void
dw_writer_line_set_isa(DW_Writer *writer, U64 isa)
{
  dw_line_inst_list_push(writer->arena, &writer->line.line_insts, DW_LNS_set_isa(isa));
}

internal DW_WriterFile *
dw_writer_new_file(DW_Writer *writer, String8 path)
{
  DW_WriterFile *file = push_array(writer->arena, DW_WriterFile, 1);
  file->path = path;

  SLLQueuePush(writer->line.first_file, writer->line.last_file, file);
  writer->line.file_count += 1;
  return file;
}

internal void
dw_lower_attrib_forms(DW_Writer *writer, DW_WriterTag *tag)
{
  for EachNode(attrib, DW_WriterAttrib, tag->first_attrib) {
    switch (attrib->form.writer.kind) {
    case DW_WriterFormKind_Null: {} break;
    case DW_WriterFormKind_Flag: {
      attrib->form.reader.kind = DW_Form_Flag;
      attrib->form.reader.flag = attrib->form.writer.flag;
    } break;
    case DW_WriterFormKind_SInt: {
      switch (dw_int_enc_from_sint(attrib->form.writer.sint)) {
      case DW_IntEnc_Null: attrib->form.reader.kind = 0; break;
      case DW_IntEnc_1Byte: {
        attrib->form.reader.kind = DW_Form_Data1;
        attrib->form.reader.data = str8((U8 *)&attrib->form.writer.sint, sizeof(S8));
      } break;
      case DW_IntEnc_2Byte: {
        attrib->form.reader.kind = DW_Form_Data2;
        attrib->form.reader.data = str8((U8 *)&attrib->form.writer.sint, sizeof(S16));
      } break;
      case DW_IntEnc_4Byte: {
        attrib->form.reader.kind = DW_Form_Data4;
        attrib->form.reader.data = str8((U8 *)&attrib->form.writer.sint, sizeof(U32));
      } break;
      case DW_IntEnc_LEB128: {
        attrib->form.reader.kind  = DW_Form_SData;
        attrib->form.reader.sdata = attrib->form.writer.sint;
      } break;
      default: InvalidPath; break;
      }
    } break;
    case DW_WriterFormKind_UInt: {
      attrib->form.reader.kind  = DW_Form_UData;
      attrib->form.reader.udata = attrib->form.writer.uint;
    } break;
    case DW_WriterFormKind_Address: {
      Assert(writer->address_size <= sizeof(attrib->form.writer.address));
      attrib->form.reader.kind = DW_Form_Addr;
      attrib->form.reader.addr = push_str8_copy(writer->arena, str8((U8 *)&attrib->form.writer.address, writer->address_size));
    } break;
    case DW_WriterFormKind_Ref: {
      if (attrib->form.writer.ref->info_off == 0) {
        attrib->form.reader.kind = DW_Form_Ref8;
      } else {
        switch (dw_int_enc_from_uint(attrib->form.writer.ref->info_off)) {
        case DW_IntEnc_Null: break;
        case DW_IntEnc_1Byte: {
          attrib->form.reader.kind = DW_Form_Ref1;
          attrib->form.reader.ref  = (U8)attrib->form.writer.ref->info_off;
        } break;
        case DW_IntEnc_2Byte: {
          attrib->form.reader.kind = DW_Form_Ref2;
          attrib->form.reader.ref  = (U16)attrib->form.writer.ref->info_off;
        } break;
        case DW_IntEnc_4Byte: {
          attrib->form.reader.kind = DW_Form_Ref4;
          attrib->form.reader.ref  = (U32)attrib->form.writer.ref->info_off;
        } break;
        case DW_IntEnc_LEB128: {
          attrib->form.reader.kind = DW_Form_RefUData;
          attrib->form.reader.ref  = attrib->form.writer.ref->info_off;
        } break;
        default: InvalidPath; break;
        }
      }
    } break;
    case DW_WriterFormKind_LinePtr: {
      attrib->form.reader.kind       = DW_Form_SecOffset;
      attrib->form.reader.sec_offset = attrib->form.writer.line_ptr;
    } break;
    case DW_WriterFormKind_MacPtr: {
      NotImplemented;
    } break;
    case DW_WriterFormKind_RngListPtr: {
      NotImplemented;
    } break;
    case DW_WriterFormKind_Data: {
      NotImplemented;
    } break;
    case DW_WriterFormKind_Block: {
      attrib->form.reader.block = attrib->form.writer.block;
      switch (attrib->form.writer.block.size) {
      case 0 : break;
      case 1 : attrib->form.reader.kind = DW_Form_Block1; break;
      case 2 : attrib->form.reader.kind = DW_Form_Block2; break;
      case 4 : attrib->form.reader.kind = DW_Form_Block4; break;
      default: attrib->form.reader.kind = DW_Form_Block;  break;
      }
    } break;
    case DW_WriterFormKind_String: {
      attrib->form.reader.kind   = DW_Form_String;
      attrib->form.reader.string = attrib->form.writer.string;
    } break;
    case DW_WriterFormKind_ExprLoc: {
      attrib->form.reader.kind    = DW_Form_ExprLoc;
      attrib->form.reader.exprloc = attrib->form.writer.exprloc;
    } break;
    case DW_WriterFormKind_Implicit: {
      attrib->form.reader.kind           = DW_Form_ImplicitConst;
      attrib->form.reader.implicit_const = attrib->form.writer.implicit;
    } break;
    default: { InvalidPath; } break;
    }
  }
}

internal void
dw_writer_emit_tag(DW_Writer *writer, DW_WriterTag *tag)
{
  Assert(tag->info_off == 0);

  dw_lower_attrib_forms(writer, tag);

  // make abbrev id for the tag
  U64 abbrev_id;
  {
    Temp temp = temp_begin(writer->arena);
    String8 abbrev_entry = dw_make_abbrev_entry(temp.arena, tag);

    if ( ! hash_table_search_string_u64(writer->abbrev_id_map, abbrev_entry, &abbrev_id)) {
      abbrev_id = writer->abbrev_id_map->count + 1;

      hash_table_push_string_u64(writer->arena, writer->abbrev_id_map, abbrev_entry, abbrev_id);

      // push abbrev entry
      dw_serial_push_uleb128(writer->arena, &writer->sections[DW_Section_Abbrev].srl, abbrev_id);
      str8_serial_push_string(writer->arena, &writer->sections[DW_Section_Abbrev].srl, abbrev_entry);
    } else {
      temp_end(temp);
    }
  }

  tag->abbrev_id = abbrev_id;
  tag->info_off  = writer->sections[DW_Section_Info].srl.total_size;

  // emit tag abbrev id
  dw_serial_push_uleb128(writer->arena, &writer->sections[DW_Section_Info].srl, tag->abbrev_id);

  // emit attribs
  {
    String8List *debug_info = &writer->sections[DW_Section_Info].srl;
    for EachNode(attrib, DW_WriterAttrib, tag->first_attrib) {
      switch (attrib->form.reader.kind) {
      case DW_Form_Addr: {
        str8_serial_push_string(writer->arena, debug_info, attrib->form.reader.addr);
      } break;
      case DW_Form_Block1:
      case DW_Form_Block2:
      case DW_Form_Block4: {
        str8_serial_push_string(writer->arena, debug_info, attrib->form.reader.block);
      } break;
      case DW_Form_Data1:
      case DW_Form_Data2:
      case DW_Form_Data4:
      case DW_Form_Data8: {
        str8_serial_push_string(writer->arena, debug_info, attrib->form.reader.data);
      } break;
      case DW_Form_String: {
        str8_serial_push_cstr(writer->arena, debug_info, attrib->form.reader.string);
      } break;
      case DW_Form_Flag: {
        str8_serial_push_u8(writer->arena, debug_info, attrib->form.reader.flag);
      } break;
      case DW_Form_SData: {
        dw_serial_push_sleb128(writer->arena, debug_info, attrib->form.reader.sdata);
      } break;
      case DW_Form_Strp: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->form.reader.sec_offset);
      } break;
      case DW_Form_UData: {
        dw_serial_push_uleb128(writer->arena, debug_info, attrib->form.reader.udata);
      } break;
      case DW_Form_RefAddr: {
        if (writer->version < DW_Version_3) {
          Assert(writer->address_size <= sizeof(attrib->form.reader.ref));
          str8_serial_push_string(writer->arena, debug_info, str8((U8 *)&attrib->form.reader.ref, writer->address_size));
        } else {
          dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->form.reader.ref);
        }
      } break;
      case DW_Form_Ref1: {
        str8_serial_push_u8(writer->arena, debug_info, (U8)attrib->form.reader.ref);
      } break;
      case DW_Form_Ref2: {
        str8_serial_push_u16(writer->arena, debug_info, (U16)attrib->form.reader.ref);
      } break;
      case DW_Form_Ref4: {
        str8_serial_push_u32(writer->arena, debug_info, (U32)attrib->form.reader.ref);
      } break;
      case DW_Form_Ref8: {
        if (attrib->form.writer.ref->info_off == 0) {
          // reserve 8 bytes
          void *value = str8_serial_push_u64(writer->arena, debug_info, 0);

          // push fixup
          dw_writer_fixup_list_push(writer->arena, &writer->fixups, (DW_WriterFixup){ .tag = attrib->form.writer.ref, .ptr = value });
        }
      } break;
      case DW_Form_RefUData: {
        dw_serial_push_uleb128(writer->arena, debug_info, attrib->form.reader.ref);
      } break;
      case DW_Form_Indirect: {
        NotImplemented;
      } break;
      case DW_Form_SecOffset: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->form.reader.sec_offset);
      } break;
      case DW_Form_ExprLoc: {
        dw_serial_push_uleb128(writer->arena, debug_info, attrib->form.reader.exprloc.size);
        str8_serial_push_string(writer->arena, debug_info, attrib->form.reader.exprloc);
      } break;
      case DW_Form_FlagPresent: {
      } break;
      case DW_Form_RefSig8: {
        NotImplemented;
      } break;
      case DW_Form_Strx:
      case DW_Form_Addrx:
      case DW_Form_RngListx:
      case DW_Form_LocListx: {
        dw_serial_push_uleb128(writer->arena, debug_info, attrib->form.reader.xval);
      } break;
      case DW_Form_RefSup4: {
        NotImplemented;
      } break;
      case DW_Form_StrpSup: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->form.reader.strp_sup);
      } break;
      case DW_Form_Data16: {
        str8_serial_push_string(writer->arena, debug_info, attrib->form.reader.data);
      } break;
      case DW_Form_LineStrp: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->form.reader.sec_offset);
      } break;
      case DW_Form_ImplicitConst: {
        // value is stored in the abbrev entry
      } break;
      case DW_Form_RefSup8: {
        NotImplemented;
      } break;
      case DW_Form_Strx1:
      case DW_Form_Addrx1: {
        str8_serial_push_u8(writer->arena, debug_info, (U8)attrib->form.reader.xval);
      } break;
      case DW_Form_Strx2:
      case DW_Form_Addrx2: {
        str8_serial_push_u16(writer->arena, debug_info, (U16)attrib->form.reader.xval);
      } break;
      case DW_Form_Strx3:
      case DW_Form_Addrx3: {
        str8_serial_push_string(writer->arena, debug_info, str8((U8 *)&attrib->form.reader.xval, 3));
      } break;
      case DW_Form_Strx4:
      case DW_Form_Addrx4: {
        str8_serial_push_u32(writer->arena, debug_info, (U32)attrib->form.reader.xval);
      } break;

      case DW_Form_GNU_StrpAlt: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->form.reader.sec_offset);
      } break;
      case DW_Form_GNU_RefAlt: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->form.reader.ref);
      } break;

      default: InvalidPath; break;
      }
    }
  }

  // write children
  for EachNode(child, DW_WriterTag, tag->first_child) {
    dw_writer_emit_tag(writer, child);
  }

  // close tag
  if (tag->first_child) {
    dw_serial_push_uleb128(writer->arena, &writer->sections[DW_Section_Info].srl, 0);
  }
}

internal void
dw_writer_emit(DW_Writer *writer)
{
  // line
  {
    // push line table terminator
    dw_writer_line_end_sequence(writer);

    // find comp dir in the compile unit tag
    String8 comp_dir  = {0};
    String8 comp_name = {0};
    Assert(writer->root->kind == DW_TagKind_CompileUnit);
    for EachNode(attrib, DW_WriterAttrib, writer->root->first_attrib) {
      if (attrib->kind == DW_AttribKind_CompDir) {
        AssertAlways(attrib->form.writer.kind == DW_WriterFormKind_String);
        comp_dir = str8_chop_last_slash(attrib->form.writer.string);
      } else if (attrib->kind == DW_AttribKind_Name) {
        AssertAlways(attrib->form.writer.kind == DW_WriterFormKind_String);
        comp_name = attrib->form.writer.string;
      }
    }

    String8List dir_table_srl = {0};
    str8_serial_begin(writer->arena, &dir_table_srl);
    {
      Temp scratch = scratch_begin(0, 0);
      String8List *srl = &dir_table_srl;

      // (13) directory_entry_format_count
      str8_serial_push_u8(writer->arena, srl, 1);
      // (14) directory_entry_format
      dw_serial_push_uleb128(writer->arena, srl, DW_LNCT_Path);
      dw_serial_push_uleb128(writer->arena, srl, DW_Form_String);

      // dedup directories
      String8List  dirs   = {0};
      HashTable   *dir_ht = hash_table_init(scratch.arena, writer->line.file_count + 1);
      // first entry must be compile unit directory
      hash_table_push_string_u64(scratch.arena, dir_ht, comp_dir, dir_ht->count);
      if (comp_dir.size == 2 && char_is_alpha(comp_dir.str[0]) && comp_dir.str[1] == ':') {
        str8_list_pushf(scratch.arena, &dirs, "%S/", comp_dir);
      } else {
        str8_list_push(scratch.arena, &dirs, comp_dir);
      }
      for EachNode(file, DW_WriterFile, writer->line.first_file) {
        String8 path = str8_chop_last_slash(file->path);
        if (path.size == 0) {
           continue;
        }
        if ( ! hash_table_search_string_u64(dir_ht, path, &file->dir_idx)) {
          // @dir_idx
          file->dir_idx = hash_table_push_string_u64(scratch.arena, dir_ht, path, dir_ht->count)->v.value_u64;
          if (path.size == 2 && char_is_alpha(path.str[0]) && path.str[1] == ':') {
            str8_list_pushf(scratch.arena, &dirs, "%S/", path);
          } else {
            str8_list_push(scratch.arena, &dirs, path);
          }
        }
      }

      // (15) directories_count
      dw_serial_push_uleb128(writer->arena, srl, dirs.node_count);

      // (16) directories
      for EachNode(dir_n, String8Node, dirs.first) {
        dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, (DW_WriterXForm) { .reader = { .kind = DW_Form_String, .string = dir_n->string } });
      }

      scratch_end(scratch);
    }

    String8List file_table_srl = {0};
    str8_serial_begin(writer->arena, &file_table_srl);
    {
      Temp scratch = scratch_begin(0, 0);
      String8List *srl = &file_table_srl;

      struct { DW_LNCT lnct; DW_FormKind form_kind; } encs[] = {
        { DW_LNCT_Path,           DW_Form_String },
        { DW_LNCT_DirectoryIndex, DW_Form_UData  },
        { DW_LNCT_TimeStamp,      DW_Form_Null   }, // DW_Form_UData
        { DW_LNCT_Size,           DW_Form_Null   }, // DW_Form_UData
        { DW_LNCT_MD5,            DW_Form_Null   }, // DW_Form_Block
        { DW_LNCT_LLVM_Source,    DW_Form_Null   }  // DW_Form_String
      };

      HashTable *encs_ht = hash_table_init(scratch.arena, ArrayCount(encs) * 2);
      for EachElement(i, encs) { hash_table_push_u64_raw(scratch.arena, encs_ht, encs[i].lnct, &encs[i].form_kind); }

      // enable encodings in use
      for EachNode(file, DW_WriterFile, writer->line.first_file) {
        if (file->time_stamp  != 0)                { *(DW_FormKind *)hash_table_search_u64_raw(encs_ht, DW_LNCT_TimeStamp  ) = DW_Form_UData;  }
        if (file->size        != 0)                { *(DW_FormKind *)hash_table_search_u64_raw(encs_ht, DW_LNCT_Size       ) = DW_Form_UData;  }
        if ( ! u128_match(file->md5, u128_zero())) { *(DW_FormKind *)hash_table_search_u64_raw(encs_ht, DW_LNCT_MD5        ) = DW_Form_Block;  }
        if (file->source.size != 0)                { *(DW_FormKind *)hash_table_search_u64_raw(encs_ht, DW_LNCT_LLVM_Source) = DW_Form_String; }
      }

      // count needed encodings
      U8 enc_count = 0;
      for EachElement(i, encs) { if (encs[i].form_kind != DW_Form_Null) { enc_count += 1; } }

      // (17) file_name_entry_format_count
      str8_serial_push_u8(writer->arena, srl, enc_count);

      // (18) file_name_entry_format
      for EachElement(i, encs) {
        if (encs[i].form_kind == DW_Form_Null) { continue; }
        dw_serial_push_uleb128(writer->arena, srl, encs[i].lnct     );
        dw_serial_push_uleb128(writer->arena, srl, encs[i].form_kind);
      }

      // (19) file_names_count
      dw_serial_push_uleb128(writer->arena, srl, writer->line.file_count);

      // (20) file_names
      if (comp_name.size) {
        for (DW_WriterFile *n = writer->line.first_file, *p = 0; n != 0; p = n, n = n->next) {
          String8 file_name = str8_skip_last_slash(n->path);
          if (str8_match(file_name, comp_name, 0)) {
            if (p == 0) {
              // file is already first
            } else {
              // move compile unit file to be first entry in the table
              p->next = n->next;
              n->next = writer->line.first_file;
              writer->line.first_file = n;
              if (writer->line.last_file == n) {
                writer->line.last_file = p;
              }
            }
            break;
          }
        }
      }
      U64 file_idx = 0;
      for EachNode(file, DW_WriterFile, writer->line.first_file) {
        static DW_WriterXForm null_form_udata  = { .reader = { .kind = DW_Form_UData  } };
        static DW_WriterXForm null_form_string = { .reader = { .kind = DW_Form_String } };
        static DW_WriterXForm null_form_block  = { .reader = { .kind = DW_Form_Block  } };

        // @file_idx
        file->file_idx = file_idx++;

        // path
        Assert(*(DW_FormKind *)hash_table_search_u64_raw(encs_ht, DW_LNCT_Path) == DW_Form_String);
        String8 file_name = str8_skip_last_slash(file->path);
        DW_WriterXForm path_form = { .reader = { .kind = DW_Form_String, .string = file_name } };
        dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, path_form);

        // directory index
        Assert(*(DW_FormKind *)hash_table_search_u64_raw(encs_ht, DW_LNCT_DirectoryIndex) == DW_Form_UData);
        DW_WriterXForm dir_idx_form = { .reader = { .kind = DW_Form_UData, .udata = file->dir_idx } };
        dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, dir_idx_form);

        // time stamp
        if (*(DW_FormKind *)hash_table_search_u64_raw(encs_ht, DW_LNCT_TimeStamp) == DW_Form_UData) {
          if (file->time_stamp) {
            DW_WriterXForm time_stamp_form = { .reader = { .kind = DW_Form_UData, .udata = file->time_stamp } };
            dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, time_stamp_form);
          } else {
            dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, null_form_udata);
          }
        }

        // size
        if (*(DW_FormKind *)hash_table_search_u64_raw(encs_ht, DW_LNCT_Size) == DW_Form_UData) {
          if (file->size) {
            DW_WriterXForm size_form = { .reader = { .kind = DW_Form_UData, .udata = file->size } };
            dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, size_form);
          } else {
            dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, null_form_udata);
          }
        }

        // MD5
        if (*(DW_FormKind *)hash_table_search_u64_raw(encs_ht, DW_LNCT_MD5) == DW_Form_Block) {
          if ( ! u128_match(file->md5, u128_zero())) {
            DW_WriterXForm md5_form = { .reader = { .kind = DW_Form_Block, .block = str8_struct(&file->md5) } };
            dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, md5_form);
          } else {
            dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, null_form_block);
          }
        }

        // LLVM Source
        if (*(DW_FormKind *)hash_table_search_u64_raw(encs_ht, DW_LNCT_LLVM_Source) == DW_Form_String) {
          if (file->source.size) {
            DW_WriterXForm source_form = { .reader = { .kind = DW_Form_String, .string = file->source } };
            dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, source_form);
          } else {
            dw_serial_push_form(writer->arena, srl, writer->version, writer->format, writer->address_size, 0, null_form_string);
          }
        }
      }

      scratch_end(scratch);
    }

    {
      String8List *srl = &writer->sections[DW_Section_Line].srl;
      // (1) unit_length
      writer->sections[DW_Section_Line].length = dw_serial_push_length(writer->arena, srl, writer->format, 0);
      // (2) version
      str8_serial_push_struct(writer->arena, srl, &writer->version);
      // (3) address_size
      str8_serial_push_struct(writer->arena, srl, &writer->address_size);
      // (4) segment_selector_size
      str8_serial_push_struct(writer->arena, srl, &writer->segsel_size);
      // (5) header_length
      void *header_length_ptr    = dw_serial_push_uint(writer->arena, srl, writer->format, 0);
      U64   header_length_offset = srl->total_size;
      // (6) minimum_instruction_length
      str8_serial_push_struct(writer->arena, srl, &writer->line.min_inst_len);
      // (7) maximum_operations_per_instruction
      str8_serial_push_struct(writer->arena, srl, &writer->line.max_ops_per_inst);
      // (8) default_is_stmt
      str8_serial_push_struct(writer->arena, srl, &writer->line.default_is_stmt);
      // (9) line_base
      str8_serial_push_struct(writer->arena, srl, &writer->line.line_base);
      // (10) line_range
      str8_serial_push_struct(writer->arena, srl, &writer->line.line_range);
      // (11) opcode_base
      str8_serial_push_struct(writer->arena, srl, &writer->line.opcode_base);
      // (12) standard_opcode_lengths
      U8 *std_op_lens = push_array(writer->arena, U8, writer->line.opcode_base - 1);
      for (U64 i = 1; i < writer->line.opcode_base; i += 1) { std_op_lens[i-1] = dw_length_from_std_opcode(i); }
      str8_serial_push_string(writer->arena, srl, str8(std_op_lens, writer->line.opcode_base - 1));
      // directory table
      str8_list_concat_in_place(srl, &dir_table_srl);
      // file table
      str8_list_concat_in_place(srl, &file_table_srl);

      // fixup header length
      U64 bytes_to_line_program = srl->total_size - header_length_offset;
      if (writer->format == DW_Format_32Bit) { *(U32 *)header_length_ptr = bytes_to_line_program; }
      else                                   { *(U64 *)header_length_ptr = bytes_to_line_program; }

      // line program
      String8 line_program_data = dw_data_from_line_insts(writer->arena, writer->address_size, writer->line.line_insts);
      str8_serial_push_string(writer->arena, srl, line_program_data);
    }
  }

  // info
  {
    dw_writer_emit_tag(writer, writer->root);

    // null terminate .debug_abbrev
    dw_serial_push_uleb128(writer->arena, &writer->sections[DW_Section_Abbrev].srl, 0);

    // fixup units lengths
    for EachElement(i, writer->sections) {
      if (writer->sections[i].length) {
        if (writer->format == DW_Format_64Bit) {
          U64 *length      = writer->sections[i].length;
          U64  length_size = sizeof(U64) + sizeof(U32);
          Assert(writer->sections[i].srl.total_size >= length_size);
          *length = writer->sections[i].srl.total_size - length_size;
        } else {
          U32 *length      = writer->sections[i].length;
          U32  length_size = sizeof(U32);
          Assert(writer->sections[i].srl.total_size >= length_size);
          *length = safe_cast_u32(writer->sections[i].srl.total_size - length_size);
        }
      }
    }

    // fixup forward references in .debug_info
    for EachNode(fixup_n, DW_WriterFixupNode, writer->fixups.first) {
      DW_WriterFixup *fixup = &fixup_n->v;
      Assert(fixup->tag->info_off != 0);
      MemoryCopy(fixup->ptr, &fixup->tag->info_off, sizeof(U64));
    }
  }
}

#ifdef OBJ_H
internal void
dw_writer_emit_to_obj(DW_Writer *writer, OBJ *obj)
{
  dw_writer_emit(writer);

  // push obj sections
  OBJ_Section *sections[DW_Section_Count] = {0};
  for EachElement(i, writer->sections) {
    if (writer->sections[i].srl.total_size) {
      sections[i] = obj_push_section(obj, dw_name_string_from_section_kind(i), OBJ_SectionFlag_Read|OBJ_SectionFlag_Write);
    }
  }
 
  // push OBJ relocation for 'abbrev base'
  OBJ_Section *debug_info = sections[DW_Section_Info];
  obj_push_reloc(obj, debug_info, OBJ_RelocKind_SecRel, writer->abbrev_base_info_off, sections[DW_Section_Abbrev]->symbol);

  // copy section data from writer to obj
  for EachElement(i, writer->sections) {
    if (writer->sections[i].srl.total_size > 0) {
      String8 data = str8_serial_end(obj->arena, &writer->sections[i].srl);
      str8_list_push(obj->arena, &sections[i]->data, data);
    }
  }

  // zero out writer sections
  MemoryZeroArray(writer->sections);
}
#endif

