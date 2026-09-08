// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////
// MSVC IFC (header-unit module interface) reader
//
// radlink consumes the `.msvc.trait.debug-records` partition embedded in an
// MSVC `.ifc` (C++20 module / header-unit interface) file. That partition is a
// raw CodeView type-leaf stream (entity_size == 1, NO u32 signature, first leaf
// at offset 0, TI base 0x1000). A consuming `.obj` references this stream via
// LF_IFC_RECORD (0x1522) leaves -- see lnk_debug_info.c.
//
// File layout (microsoft/ifc-spec, FileHeader):
//   u8[4]  signature = { 0x54,0x51,0x45,0x1A }   ("TQE\x1a")
//   u8[32] content_hash (sha256)  -- record.GUID(16)++record.hash(16) == first 32 bytes here
//   u8     major, minor           -- assert 0.44
//   u8     abi
//   u8     arch                   -- 2 == x64
//   u32    cplusplus
//   u32    string_table_bytes (off), u32 string_table_size
//   u32    unit
//   u32    src_path (textoffset)
//   u32    global_scope
//   u32    toc                    -- offset to partition summary table
//   u32    partition_count
//   u8     internal_partition
// Partition summary entry (16 bytes): { u32 name(textoffset); u32 offset; u32 count; u32 entity_size }

typedef struct IFC_File
{
  String8 data;          // whole .ifc bytes (owning view into arena)
  String8 path;          // .ifc path (copied)
  U8      content_hash[32];
  String8 debug_records; // .msvc.trait.debug-records blob {ptr,size}; size 0 if absent
  B32     is_valid;
} IFC_File;

// Reads `path`, validates magic/version/arch, locates `.msvc.trait.debug-records`.
// On error fills *error_out and returns is_valid=0. Detects UBA-compressed inputs
// (magic "UBA\x01") and reports them (decompression is out of scope).
internal IFC_File ifc_file_read(Arena *arena, String8 path, String8 *error_out);
