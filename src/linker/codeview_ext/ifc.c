// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

read_only global U8 g_ifc_signature[4] = { 0x54, 0x51, 0x45, 0x1A };
read_only global U8 g_uba_signature[4] = { 0x55, 0x42, 0x41, 0x01 }; // "UBA\x01"

internal IFC_File
ifc_file_read(Arena *arena, String8 path, String8 *error_out)
{
  IFC_File ifc = {0};
  ifc.path = push_str8_copy(arena, path);

  B8      was_read = 0;
  String8 data     = lnk_read_data_from_file_path(arena, 0, path, &was_read);
  ifc.data = data;
  if ( ! was_read) {
    *error_out = push_str8f(arena, "unable to read IFC '%S'", path);
    return ifc;
  }
  if (data.size < 4) {
    *error_out = push_str8f(arena, "IFC '%S' is too small (%llu bytes)", path, data.size);
    return ifc;
  }

  // detect UBA-compressed input (out of scope)
  if (MemoryMatch(data.str, g_uba_signature, sizeof(g_uba_signature))) {
    *error_out = push_str8f(arena, "IFC '%S' is UBA-compressed (magic 'UBA\\x01'); materialize a raw .ifc (UBA decompress unsupported)", path);
    return ifc;
  }

  // validate signature
  if ( ! MemoryMatch(data.str, g_ifc_signature, sizeof(g_ifc_signature))) {
    *error_out = push_str8f(arena, "IFC '%S' has bad signature (expected 54 51 45 1A)", path);
    return ifc;
  }

  // --- parse header ---
  U64 off = 4;
  if (off + 32 > data.size) { goto truncated; }
  MemoryCopy(ifc.content_hash, data.str + off, 32);
  off += 32;

  if (off + 4 > data.size) { goto truncated; }
  U8 major = data.str[off+0];
  U8 minor = data.str[off+1];
  U8 abi   = data.str[off+2]; (void)abi;
  U8 arch  = data.str[off+3];
  off += 4;

  // assert version 0.44 + x64; error otherwise (encoding proven only for these)
  if ( ! (major == 0 && minor == 44)) {
    *error_out = push_str8f(arena, "IFC '%S' unsupported version %u.%u (expected 0.44)", path, major, minor);
    return ifc;
  }
  if (arch != 2) {
    *error_out = push_str8f(arena, "IFC '%S' unsupported architecture %u (expected 2 == x64)", path, arch);
    return ifc;
  }

  U32 cplusplus;           off += str8_deserial_read_struct(data, off, &cplusplus); (void)cplusplus;
  U32 string_table_bytes;  off += str8_deserial_read_struct(data, off, &string_table_bytes);
  U32 string_table_size;   off += str8_deserial_read_struct(data, off, &string_table_size);
  U32 unit;                off += str8_deserial_read_struct(data, off, &unit); (void)unit;
  U32 src_path;            off += str8_deserial_read_struct(data, off, &src_path); (void)src_path;
  U32 global_scope;        off += str8_deserial_read_struct(data, off, &global_scope); (void)global_scope;
  U32 toc;                 off += str8_deserial_read_struct(data, off, &toc);
  U32 partition_count;     off += str8_deserial_read_struct(data, off, &partition_count);
  if (off > data.size) { goto truncated; }

  // string table
  if ((U64)string_table_bytes + string_table_size > data.size) {
    *error_out = push_str8f(arena, "IFC '%S' string table out of bounds", path);
    return ifc;
  }
  String8 string_table = str8(data.str + string_table_bytes, string_table_size);

  // --- partition summary table ---
  String8 needle = str8_lit(".msvc.trait.debug-records");
  U64     po     = toc;
  for (U32 i = 0; i < partition_count; ++i, po += 16) {
    if (po + 16 > data.size) { goto truncated; }
    U32 name_off, p_off, count, entity_size;
    str8_deserial_read_struct(data, po + 0,  &name_off);
    str8_deserial_read_struct(data, po + 4,  &p_off);
    str8_deserial_read_struct(data, po + 8,  &count);
    str8_deserial_read_struct(data, po + 12, &entity_size);

    if (name_off >= string_table.size) { continue; }
    String8 name = str8_cstring((char *)string_table.str + name_off);
    if (str8_match(name, needle, 0)) {
      // entity_size == 1 -> count is a byte length
      if ((U64)p_off + count > data.size) {
        *error_out = push_str8f(arena, "IFC '%S' debug-records partition out of bounds", path);
        return ifc;
      }
      ifc.debug_records = str8(data.str + p_off, count);
      break;
    }
  }

  if (ifc.debug_records.size == 0) {
    *error_out = push_str8f(arena, "IFC '%S' has no '.msvc.trait.debug-records' partition", path);
    return ifc;
  }

  ifc.is_valid = 1;
  return ifc;

truncated:
  *error_out = push_str8f(arena, "IFC '%S' is truncated", path);
  return ifc;
}
