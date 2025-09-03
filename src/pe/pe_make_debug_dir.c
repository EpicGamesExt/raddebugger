// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
pe_make_debug_directory_pdb_obj(Arena *arena, COFF_MachineType machine, Guid guid, U32 age, COFF_TimeStamp time_stamp, String8 path)
{
  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(COFF_TimeStamp_Max, machine);

  String8          debug_data   = pe_make_debug_header_pdb70(obj_writer->arena, guid, age, path);
  COFF_ObjSection *debug_sect   = coff_obj_writer_push_section(obj_writer, str8_lit(".RAD_LINK_PE_DEBUG_DATA"), PE_DEBUG_DIR_SECTION_FLAGS, debug_data);
  COFF_ObjSymbol  *debug_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("PDB_DEBUG_HEADER_70"), 0, debug_sect);
  coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("RAD_LINK_PE_DEBUG_PDB"), 0, debug_sect);
  coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("RAD_LINK_PE_DEBUG_GUID_PDB"), OffsetOf(PE_CvHeaderPDB70, guid), debug_sect);

  PE_DebugDirectory *dir = push_array(obj_writer->arena, PE_DebugDirectory, 1);
  dir->time_stamp        = time_stamp;
  dir->type              = PE_DebugDirectoryType_CODEVIEW;
  dir->size              = debug_data.size;
  COFF_ObjSection *debug_dir_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".RAD_LINK_PE_DEBUG_DIR"), PE_DEBUG_DIR_SECTION_FLAGS, str8_struct(dir));
  coff_obj_writer_section_push_reloc_voff(obj_writer, debug_dir_sect, OffsetOf(PE_DebugDirectory, voff), debug_symbol);

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  return obj;
}

internal String8
pe_make_debug_directory_rdi_obj(Arena *arena, COFF_MachineType machine, Guid guid, U32 age, COFF_TimeStamp time_stamp, String8 path)
{
  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(COFF_TimeStamp_Max, machine);

  String8          debug_data   = pe_make_debug_header_rdi(obj_writer->arena, guid, path);
  COFF_ObjSection *debug_sect   = coff_obj_writer_push_section(obj_writer, str8_lit(".RAD_LINK_PE_DEBUG_DATA"), PE_DEBUG_DIR_SECTION_FLAGS, debug_data);
  COFF_ObjSymbol  *debug_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("PDB_DEBUG_HEADER_RDI"), 0, debug_sect);
  coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("RAD_LINK_PE_DEBUG_RDI"), 0, debug_sect);
  coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("RAD_LINK_PE_DEBUG_GUID_RDI"), OffsetOf(PE_CvHeaderRDI, guid), debug_sect);

  PE_DebugDirectory *dir = push_array(obj_writer->arena, PE_DebugDirectory, 1);
  dir->time_stamp = time_stamp;
  dir->type       = PE_DebugDirectoryType_CODEVIEW;
  dir->size       = debug_data.size;
  COFF_ObjSection *dir_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".RAD_LINK_PE_DEBUG_DIR"), PE_DEBUG_DIR_SECTION_FLAGS, str8_struct(dir));
  coff_obj_writer_section_push_reloc_voff(obj_writer, dir_sect, OffsetOf(PE_DebugDirectory, voff), debug_symbol);

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  return obj;
}

