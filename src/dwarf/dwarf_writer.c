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

internal void
dw_lower_form(DW_WriterFormKind  form_kind,
              DW_WriterForm      form,
              DW_FormKind       *form_kind_out,
              DW_Form           *form_out)
{
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
  writer->arena         = arena;
  writer->arch          = arch;
  writer->format        = format;
  writer->version       = version;
  writer->cu_kind       = cu_kind;
  writer->address_size  = byte_size_from_arch(arch);
  writer->current       = 0;
  writer->abbrev_id_map = hash_table_init(arena, 0x2000);
  for EachElement(i, writer->sections) {
    str8_serial_begin(arena, &writer->sections[i].srl);
  }

  // write .debug_info header
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
    dw_serial_push_uleb128(scratch.arena, &srl, attrib->reader.form.kind);
    if (attrib->reader.form.kind == DW_Form_ImplicitConst) {
      dw_serial_push_sleb128(scratch.arena, &srl, attrib->reader.form.implicit_const);
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
  DW_WriterAttrib *attrib = dw_writer_attrib_chunk_list_push(writer->arena, &writer->attrib_chunk_list, 512);
  attrib->kind        = kind;
  attrib->writer.form = form;
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
dw_writer_push_attrib_line_ptr(DW_Writer *writer, DW_AttribKind kind, void *line_ptr)
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
dw_lower_attrib_forms(DW_Writer *writer, DW_WriterTag *tag)
{
  for EachNode(attrib, DW_WriterAttrib, tag->first_attrib) {
    switch (attrib->writer.form.kind) {
    case DW_WriterFormKind_Null: {} break;
    case DW_WriterFormKind_Flag: {
      attrib->reader.form.kind = DW_Form_Flag;
      attrib->reader.form.flag = attrib->writer.form.flag;
    } break;
    case DW_WriterFormKind_SInt: {
      switch (dw_int_enc_from_sint(attrib->writer.form.sint)) {
      case DW_IntEnc_Null: attrib->reader.form.kind = 0; break;
      case DW_IntEnc_1Byte: {
        attrib->reader.form.kind = DW_Form_Data1;
        attrib->reader.form.data = str8((U8 *)&attrib->writer.form.sint, sizeof(S8));
      } break;
      case DW_IntEnc_2Byte: {
        attrib->reader.form.kind = DW_Form_Data2;
        attrib->reader.form.data = str8((U8 *)&attrib->writer.form.sint, sizeof(S16));
      } break;
      case DW_IntEnc_4Byte: {
        attrib->reader.form.kind = DW_Form_Data4;
        attrib->reader.form.data = str8((U8 *)&attrib->writer.form.sint, sizeof(U32));
      } break;
      case DW_IntEnc_LEB128: {
        attrib->reader.form.kind  = DW_Form_SData;
        attrib->reader.form.sdata = attrib->writer.form.sint;
      } break;
      default: InvalidPath; break;
      }
    } break;
    case DW_WriterFormKind_UInt: {
      attrib->reader.form.kind  = DW_Form_UData;
      attrib->reader.form.udata = attrib->writer.form.uint;
    } break;
    case DW_WriterFormKind_Address: {
      Assert(writer->address_size <= sizeof(attrib->writer.form.address));
      attrib->reader.form.kind = DW_Form_Addr;
      attrib->reader.form.addr = push_str8_copy(writer->arena, str8((U8 *)&attrib->writer.form.address, writer->address_size));
    } break;
    case DW_WriterFormKind_Ref: {
      if (attrib->writer.form.ref->info_off == 0) {
        attrib->reader.form.kind = DW_Form_Ref8;
      } else {
        switch (dw_int_enc_from_uint(attrib->writer.form.ref->info_off)) {
        case DW_IntEnc_Null: break;
        case DW_IntEnc_1Byte: {
          attrib->reader.form.kind = DW_Form_Ref1;
          attrib->reader.form.ref  = (U8)attrib->writer.form.ref->info_off;
        } break;
        case DW_IntEnc_2Byte: {
          attrib->reader.form.kind = DW_Form_Ref2;
          attrib->reader.form.ref  = (U16)attrib->writer.form.ref->info_off;
        } break;
        case DW_IntEnc_4Byte: {
          attrib->reader.form.kind = DW_Form_Ref4;
          attrib->reader.form.ref  = (U32)attrib->writer.form.ref->info_off;
        } break;
        case DW_IntEnc_LEB128: {
          attrib->reader.form.kind = DW_Form_RefUData;
          attrib->reader.form.ref  = attrib->writer.form.ref->info_off;
        } break;
        default: InvalidPath; break;
        }
      }
    } break;
    case DW_WriterFormKind_LinePtr: {
      NotImplemented;
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
      attrib->reader.form.block = attrib->writer.form.block;
      switch (attrib->writer.form.block.size) {
      case 0 : break;
      case 1 : attrib->reader.form.kind = DW_Form_Block1; break;
      case 2 : attrib->reader.form.kind = DW_Form_Block2; break;
      case 4 : attrib->reader.form.kind = DW_Form_Block4; break;
      default: attrib->reader.form.kind = DW_Form_Block;  break;
      }
    } break;
    case DW_WriterFormKind_String: {
      attrib->reader.form.kind   = DW_Form_String;
      attrib->reader.form.string = attrib->writer.form.string;
    } break;
    case DW_WriterFormKind_ExprLoc: {
      attrib->reader.form.kind    = DW_Form_ExprLoc;
      attrib->reader.form.exprloc = attrib->writer.form.exprloc;
    } break;
    case DW_WriterFormKind_Implicit: {
      attrib->reader.form.kind           = DW_Form_ImplicitConst;
      attrib->reader.form.implicit_const = attrib->writer.form.implicit;
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
      switch (attrib->reader.form.kind) {
      case DW_Form_Addr: {
        str8_serial_push_string(writer->arena, debug_info, attrib->reader.form.addr);
      } break;
      case DW_Form_Block1:
      case DW_Form_Block2:
      case DW_Form_Block4: {
        str8_serial_push_string(writer->arena, debug_info, attrib->reader.form.block);
      } break;
      case DW_Form_Data1:
      case DW_Form_Data2:
      case DW_Form_Data4:
      case DW_Form_Data8: {
        str8_serial_push_string(writer->arena, debug_info, attrib->reader.form.data);
      } break;
      case DW_Form_String: {
        str8_serial_push_cstr(writer->arena, debug_info, attrib->reader.form.string);
      } break;
      case DW_Form_Flag: {
        str8_serial_push_u8(writer->arena, debug_info, attrib->reader.form.flag);
      } break;
      case DW_Form_SData: {
        dw_serial_push_sleb128(writer->arena, debug_info, attrib->reader.form.sdata);
      } break;
      case DW_Form_Strp: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->reader.form.sec_offset);
      } break;
      case DW_Form_UData: {
        dw_serial_push_uleb128(writer->arena, debug_info, attrib->reader.form.udata);
      } break;
      case DW_Form_RefAddr: {
        if (writer->version < DW_Version_3) {
          Assert(writer->address_size <= sizeof(attrib->reader.form.ref));
          str8_serial_push_string(writer->arena, debug_info, str8((U8 *)&attrib->reader.form.ref, writer->address_size));
        } else {
          dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->reader.form.ref);
        }
      } break;
      case DW_Form_Ref1: {
        str8_serial_push_u8(writer->arena, debug_info, (U8)attrib->reader.form.ref);
      } break;
      case DW_Form_Ref2: {
        str8_serial_push_u16(writer->arena, debug_info, (U16)attrib->reader.form.ref);
      } break;
      case DW_Form_Ref4: {
        str8_serial_push_u32(writer->arena, debug_info, (U32)attrib->reader.form.ref);
      } break;
      case DW_Form_Ref8: {
        if (attrib->writer.form.ref->info_off == 0) {
          // reserve 8 bytes
          void *value = str8_serial_push_u64(writer->arena, debug_info, 0);

          // push fixup
          dw_writer_fixup_list_push(writer->arena, &writer->fixups, (DW_WriterFixup){ .value = value, .attrib = attrib });
        }
      } break;
      case DW_Form_RefUData: {
        dw_serial_push_uleb128(writer->arena, debug_info, attrib->reader.form.ref);
      } break;
      case DW_Form_Indirect: {
        NotImplemented;
      } break;
      case DW_Form_SecOffset: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->reader.form.sec_offset);
      } break;
      case DW_Form_ExprLoc: {
        dw_serial_push_uleb128(writer->arena, debug_info, attrib->reader.form.exprloc.size);
        str8_serial_push_string(writer->arena, debug_info, attrib->reader.form.exprloc);
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
        dw_serial_push_uleb128(writer->arena, debug_info, attrib->reader.form.xval);
      } break;
      case DW_Form_RefSup4: {
        NotImplemented;
      } break;
      case DW_Form_StrpSup: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->reader.form.strp_sup);
      } break;
      case DW_Form_Data16: {
        str8_serial_push_string(writer->arena, debug_info, attrib->reader.form.data);
      } break;
      case DW_Form_LineStrp: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->reader.form.sec_offset);
      } break;
      case DW_Form_ImplicitConst: {
        // value is stored in the abbrev entry
      } break;
      case DW_Form_RefSup8: {
        NotImplemented;
      } break;
      case DW_Form_Strx1:
      case DW_Form_Addrx1: {
        str8_serial_push_u8(writer->arena, debug_info, (U8)attrib->reader.form.xval);
      } break;
      case DW_Form_Strx2:
      case DW_Form_Addrx2: {
        str8_serial_push_u16(writer->arena, debug_info, (U16)attrib->reader.form.xval);
      } break;
      case DW_Form_Strx3:
      case DW_Form_Addrx3: {
        str8_serial_push_string(writer->arena, debug_info, str8((U8 *)&attrib->reader.form.xval, 3));
      } break;
      case DW_Form_Strx4:
      case DW_Form_Addrx4: {
        str8_serial_push_u32(writer->arena, debug_info, (U32)attrib->reader.form.xval);
      } break;

      case DW_Form_GNU_StrpAlt: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->reader.form.sec_offset);
      } break;
      case DW_Form_GNU_RefAlt: {
        dw_serial_push_uint(writer->arena, debug_info, writer->format, attrib->reader.form.ref);
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
  dw_writer_emit_tag(writer, writer->root);

  // null terminate .debug_abbrev
  dw_serial_push_uleb128(writer->arena, &writer->sections[DW_Section_Abbrev].srl, 0);
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
    if (fixup->attrib->reader.form.kind == DW_Form_Ref8) {
      Assert(fixup->attrib->writer.form.ref->info_off != 0);
      MemoryCopy(fixup->value, &fixup->attrib->writer.form.ref->info_off, sizeof(U64));
      fixup->attrib->reader.form.ref = fixup->attrib->writer.form.ref->info_off;
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

