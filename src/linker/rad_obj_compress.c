#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <oodle2.h>

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
#include "lnk_compressed_obj_format.h"

// This standalone reference writer emits one self-contained portable OBJ. It independently
// compresses every 512 KiB segment by default, preserves incompressible segments as direct-mapped
// raw runs, builds the linker sidecars, verifies every compressed round trip, and publishes via
// an atomic rename. A compiler-integrated writer can emit the same format without the raw input.

////////////////////////////////
// Minimal COFF declarations

#pragma pack(push, 1)
typedef struct ObjFileHeader {
  U16 machine, section_count; U32 time_stamp, symbol_table_foff, symbol_count;
  U16 optional_header_size, flags;
} ObjFileHeader;
typedef struct ObjBigHeader {
  U16 sig1, sig2, version, machine; U32 time_stamp; U8 magic[16], unused[16];
  U32 section_count, symbol_table_foff, symbol_count;
} ObjBigHeader;
typedef struct ObjSectionHeader {
  U8 name[8]; U32 vsize, voff, fsize, foff, relocs_foff, lines_foff;
  U16 reloc_count, line_count; U32 flags;
} ObjSectionHeader;
typedef struct ObjReloc {
  U32 apply_off, isymbol; U16 type;
} ObjReloc;
#pragma pack(pop)

typedef struct TypeBuild {
  LNK_CObjTypeIndex disk;
  LNK_CObjUdtHashIndex udt_disk;
  U32 *offsets;
  U16 *sizes;
  U16 *kinds;
  U8  *kind_codes;
  U16 kind_dictionary[256];
  U16 kind_count;
  int packed_kinds_ok;
  U64 *udt_hashes;
  U32 udt_cap;
  U32 cap;
} TypeBuild;

typedef struct BaseRelocBuild {
  LNK_CObjBaseRelocIndex disk;
  LNK_CObjBaseRelocEntry *entries;
  U32 cap;
  int supported;
} BaseRelocBuild;

typedef struct DebugSBuild {
  LNK_CObjDebugSIndex disk;
  LNK_CObjDebugSEntry *entries;
  LNK_CObjDebugSSummary *summaries;
  U32 cap;
  int supported;
} DebugSBuild;

////////////////////////////////
// File output

static int
write_at(FILE *file, U64 offset, const void *data, U64 size)
{
  if (_fseeki64(file, (long long)offset, SEEK_SET) != 0) { fprintf(stderr, "seek failed at %llu\n", offset); return 0; }
  size_t written = fwrite(data, 1, (size_t)size, file);
  if (written != size) { fprintf(stderr, "write failed at %llu: %zu/%llu\n", offset, written, size); return 0; }
  return 1;
}

////////////////////////////////
// Type sidecar construction

static U64
numeric_size(const U8 *p, const U8 *opl)
{
  if (p + 2 > opl) return 0;
  U16 k; memcpy(&k, p, 2);
  if (k < 0x8000) return 2;
  U32 n = 0;
  switch (k) {
  case 0x8000: n=1; break;
  case 0x8001: case 0x8002: case 0x801c: n=2; break;
  case 0x8003: case 0x8004: case 0x8005: n=4; break;
  case 0x8006: case 0x8009: case 0x800a: case 0x800c: n=8; break;
  case 0x8007: n=10; break;
  case 0x8008: n=16; break;
  case 0x800b: n=6; break;
  case 0x800d: n=16; break;
  case 0x800e: n=20; break;
  case 0x800f: n=32; break;
  case 0x8017: case 0x8018: n=16; break;
  default: return 0;
  }
  return p + 2 + n <= opl ? 2 + n : 0;
}

static U64
complete_udt_hash(U16 kind, const U8 *p, U64 size)
{
  const U8 *opl = p + size;
  U32 props = 0; U64 cursor = 0;
  if (kind == 0x1504 || kind == 0x1505 || kind == 0x1519) {
    if (size < 16) return 0; memcpy(&props, p + 2, 2); cursor = 16;
  } else if (kind == 0x1506) {
    if (size < 8) return 0; memcpy(&props, p + 2, 2); cursor = 8;
  } else if (kind == 0x1507) {
    if (size < 12) return 0; memcpy(&props, p + 2, 2); cursor = 12;
  } else if (kind == 0x1608 || kind == 0x1609) {
    // CV_LeafStruct2 ends in a U16 but has U32 alignment, so its serialized
    // fixed header includes two bytes of tail padding (sizeof == 20).  Using
    // the sum of the fields (18) makes the numeric parser start in padding and
    // silently drops every CLASS2/STRUCT2 complete-definition hash.
    if (size < 20) return 0; memcpy(&props, p, 4); cursor = 20;
  } else return 0;
  if (!(props & 0x200) || (props & 0x80)) return 0;
  if (kind != 0x1507) { U64 n = numeric_size(p + cursor, opl); if (!n) return 0; cursor += n; }
  const U8 *name_end = cursor < size ? memchr(p + cursor, 0, (size_t)(size - cursor)) : 0;
  if (!name_end) return 0;
  const U8 *unique = name_end + 1;
  const U8 *unique_end = unique < opl ? memchr(unique, 0, (size_t)(opl - unique)) : 0;
  if (!unique_end || unique_end == unique) return 0;
  U64 h = 5381;
  for (const U8 *c = unique; c < unique_end; ++c) h = ((h << 5) + h) ^ *c;
  return h | 1;
}

static int
is_type_section(const U8 name[8])
{
  static const U8 t[8] = {'.','d','e','b','u','g','$','T'};
  static const U8 p[8] = {'.','d','e','b','u','g','$','P'};
  return memcmp(name, t, 8) == 0 || memcmp(name, p, 8) == 0;
}

static int
build_type_indices(const U8 *file, U64 file_size, TypeBuild **builds_out, U32 *count_out)
{
  *builds_out = 0; *count_out = 0;
  if (file_size < sizeof(ObjFileHeader)) return 1;
  U64 section_off;
  U32 section_count;
  const ObjBigHeader *big = (const ObjBigHeader *)file;
  if (file_size >= sizeof(*big) && big->sig1 == 0 && big->sig2 == 0xffff && big->version >= 2) {
    section_off = sizeof(*big); section_count = big->section_count;
  } else {
    const ObjFileHeader *file_header = (const ObjFileHeader *)file;
    section_off = sizeof(*file_header); section_count = file_header->section_count;
  }
  if (section_off > file_size || (U64)section_count * sizeof(ObjSectionHeader) > file_size - section_off) return 0;
  TypeBuild *builds = (TypeBuild *)calloc(section_count ? section_count : 1, sizeof(*builds));
  if (!builds) return 0;
  const ObjSectionHeader *sections = (const ObjSectionHeader *)(file + section_off);
  U16 *kind_to_code = (U16 *)malloc(65536 * sizeof(U16));
  if (!kind_to_code) { free(builds); return 0; }
  U32 build_count = 0;
  for (U32 s = 0; s < section_count; ++s) {
    const ObjSectionHeader *sec = &sections[s];
    if (!is_type_section(sec->name) || sec->fsize < 4) continue;
    if (sec->foff > file_size || sec->fsize > file_size - sec->foff) { free(builds); return 0; }
    TypeBuild *b = &builds[build_count++];
    memset(kind_to_code, 0xff, 65536 * sizeof(U16));
    b->packed_kinds_ok = 1;
    b->disk.raw_section_offset = (U64)sec->foff + 4;
    b->disk.raw_section_size = sec->fsize - 4;
    const U8 *data = file + sec->foff + 4;
    U64 data_size = sec->fsize - 4;
    for (U64 cursor = 0; cursor + 4 <= data_size; ) {
      U16 size, kind;
      memcpy(&size, data + cursor, 2); memcpy(&kind, data + cursor + 2, 2);
      U64 stride = (U64)size + 2;
      if (size < 2 || stride > data_size - cursor || cursor > UINT32_MAX) break;
      if (b->disk.leaf_count == b->cap) {
        U32 new_cap = b->cap ? b->cap * 2 : 4096;
        U32 *new_offsets = (U32 *)realloc(b->offsets, (size_t)new_cap * sizeof(U32));
        if (!new_offsets) return 0;
        b->offsets = new_offsets;
        U16 *new_sizes = (U16 *)realloc(b->sizes, (size_t)new_cap * sizeof(U16));
        if (!new_sizes) return 0;
        b->sizes = new_sizes;
        U16 *new_kinds = (U16 *)realloc(b->kinds, (size_t)new_cap * sizeof(U16));
        if (!new_kinds) return 0;
        b->kinds = new_kinds;
        U8 *new_kind_codes = (U8 *)realloc(b->kind_codes, (size_t)new_cap);
        if (!new_kind_codes) return 0;
        b->kind_codes = new_kind_codes;
        b->cap = new_cap;
      }
      U32 i = b->disk.leaf_count++;
      b->offsets[i] = (U32)cursor; b->sizes[i] = size; b->kinds[i] = kind;
      U16 code = kind_to_code[kind];
      if (code == 0xffff) {
        if (b->kind_count == 256) {
          b->packed_kinds_ok = 0;
          code = 0;
        } else {
          code = b->kind_count++;
          kind_to_code[kind] = code;
          b->kind_dictionary[code] = kind;
        }
      }
      b->kind_codes[i] = (U8)code;
      U64 udt_hash = complete_udt_hash(kind, data + cursor + 4, (U64)size - 2);
      if (udt_hash) {
        if (b->udt_disk.hash_count == b->udt_cap) {
          U32 cap = b->udt_cap ? b->udt_cap * 2 : 1024;
          U64 *hashes = realloc(b->udt_hashes, (size_t)cap * sizeof(U64));
          if (!hashes) return 0;
          b->udt_hashes = hashes; b->udt_cap = cap;
        }
        b->udt_hashes[b->udt_disk.hash_count++] = udt_hash;
      }
      cursor += stride;
    }
  }
  free(kind_to_code);
  *builds_out = builds; *count_out = build_count;
  return 1;
}

////////////////////////////////
// .debug$S sidecar construction

static int
debug_s_is_global_symbol(U16 kind)
{
  return kind == 0x1107 || kind == 0x0102 || kind == 0x0202 || kind == 0x1008 ||
         kind == 0x110d || kind == 0x020e || kind == 0x100f || kind == 0x1113;
}

static int
debug_s_is_typedef(U16 kind)
{
  return kind == 0x0004 || kind == 0x1003 || kind == 0x1108;
}

static int
debug_s_is_scope(U16 kind)
{
  return kind == 0x1110 || kind == 0x110f || kind == 0x1103 || kind == 0x1102 ||
         kind == 0x114d || kind == 0x115d || kind == 0x1104 || kind == 0x1132 ||
         kind == 0x1147 || kind == 0x1146;
}

static int
debug_s_is_end(U16 kind)
{
  return kind == 0x0006 || kind == 0x114f || kind == 0x114e;
}

static void
debug_s_summarize_symbols(const U8 *data, U64 size, U64 *module_scope_depth, LNK_CObjDebugSSummary *out)
{
  U64 cand_depth = 0;
  int cand_active = 1;
  for (U64 cursor = 0; cursor + 4 <= size; ) {
    U16 record_size, kind;
    memcpy(&record_size, data + cursor, 2);
    memcpy(&kind, data + cursor + 2, 2);
    U64 raw_size = (U64)record_size + 2;
    U64 pdb_size = (raw_size + 3) & ~(U64)3;
    // OBJ .debug$S symbol records are byte-packed (CV_SymbolAlign == 1).  PDB module
    // streams rewrite them at PDB_SYMBOL_ALIGN == 4, so input traversal and output sizing
    // deliberately use different strides.
    if (record_size < 2 || raw_size > size - cursor) break;

    if (cand_active && (debug_s_is_global_symbol(kind) || (cand_depth == 0 && debug_s_is_typedef(kind)))) {
      out->gsi_candidate_count += 1;
    }
    if (kind == 0x1110 || kind == 0x110f || kind == 0x1147 || kind == 0x1146) {
      out->proc_ref_count += 1;
    }
    if (kind == 0x113e || kind == 0x1111 || kind == 0x110c || kind == 0x110d ||
        kind == 0x1112 || kind == 0x1113 || kind == 0x1153 || kind == 0x1107) {
      out->flags |= LNK_COBJ_DEBUG_S_SUMMARY_HAS_LOCALS;
    }
    if (cand_active) {
      if (debug_s_is_scope(kind)) cand_depth += 1;
      else if (debug_s_is_end(kind)) {
        if (cand_depth == 0) cand_active = 0;
        else cand_depth -= 1;
      }
    }

    int is_module = kind != 0x0007 && !debug_s_is_global_symbol(kind) &&
                    !(debug_s_is_typedef(kind) && *module_scope_depth == 0) && kind != 0x1176;
    if (is_module) {
      if (debug_s_is_scope(kind)) *module_scope_depth += 1;
      else if (debug_s_is_end(kind) && *module_scope_depth) *module_scope_depth -= 1;
      out->module_symbol_size += (U32)pdb_size;
    }
    cursor += raw_size;
  }
}

static int
build_debug_s_index(const U8 *file, U64 file_size, DebugSBuild *build)
{
  memset(build, 0, sizeof(*build));
  if (file_size < sizeof(ObjFileHeader)) return 1;
  U64 section_off;
  U32 section_count;
  const ObjBigHeader *big = (const ObjBigHeader *)file;
  if (file_size >= sizeof(*big) && big->sig1 == 0 && big->sig2 == 0xffff && big->version >= 2) {
    section_off = sizeof(*big); section_count = big->section_count;
  } else {
    const ObjFileHeader *h = (const ObjFileHeader *)file;
    section_off = sizeof(*h) + h->optional_header_size; section_count = h->section_count;
  }
  if (section_off > file_size || (U64)section_count * sizeof(ObjSectionHeader) > file_size - section_off) return 0;
  const ObjSectionHeader *sections = (const ObjSectionHeader *)(file + section_off);
  static const U8 debug_s_name[8] = {'.','d','e','b','u','g','$','S'};
  U64 module_scope_depth = 0;
  for (U32 sect_idx = 0; sect_idx < section_count; ++sect_idx) {
    const ObjSectionHeader *sec = &sections[sect_idx];
    if (memcmp(sec->name, debug_s_name, 8) != 0) continue;
    if (sec->foff > file_size || sec->fsize > file_size - sec->foff) return 0;
    if (sec->fsize < sizeof(U32)) continue;
    U64 c13_base = (U64)sec->foff + sizeof(U32); // skip CV_Signature
    U64 c13_size = sec->fsize - sizeof(U32);
    U64 cursor = 0;
    while (cursor + 2 * sizeof(U32) <= c13_size) {
      U32 kind, payload_size;
      memcpy(&kind, file + c13_base + cursor, sizeof(kind));
      memcpy(&payload_size, file + c13_base + cursor + sizeof(kind), sizeof(payload_size));
      U64 payload_rel = cursor + 2 * sizeof(U32);
      U64 clamped_size = payload_size < c13_size - payload_rel ? payload_size : c13_size - payload_rel;
      U64 payload_off = c13_base + payload_rel;
      // Match cv_debug_s_from_data: ignored records do not create nodes, all unknown kinds are
      // retained in the NULL bucket, and an overrun is represented by a clamped String8.
      if (!(kind & 0x80000000u)) {
        if (build->disk.entry_count == build->cap) {
          U32 new_cap = build->cap ? build->cap * 2 : 256;
          LNK_CObjDebugSEntry *entries = realloc(build->entries, (size_t)new_cap * sizeof(*entries));
          if (!entries) return 0;
          build->entries = entries;
          LNK_CObjDebugSSummary *summaries = realloc(build->summaries, (size_t)new_cap * sizeof(*summaries));
          if (!summaries) return 0;
          build->summaries = summaries;
          build->cap = new_cap;
        }
        if (payload_off > UINT32_MAX || clamped_size > UINT32_MAX) return 0;
        U32 entry_idx = build->disk.entry_count++;
        LNK_CObjDebugSEntry *dst = &build->entries[entry_idx];
        dst->raw_section_offset = sec->foff;
        dst->raw_payload_offset = (U32)payload_off;
        dst->raw_payload_size = (U32)clamped_size;
        dst->kind = kind;
        LNK_CObjDebugSSummary *summary = &build->summaries[entry_idx];
        memset(summary, 0, sizeof(*summary));
        if (kind == 0xF1) {
          debug_s_summarize_symbols(file + payload_off, clamped_size, &module_scope_depth, summary);
        }
      }
      if ((U64)payload_size > UINT64_MAX - payload_rel) break;
      U64 next = payload_rel + payload_size;
      if (next > UINT64_MAX - 3) break;
      cursor = (next + 3) & ~(U64)3;
      if (cursor <= payload_rel) break;
    }
  }
  build->supported = 1;
  return 1;
}

////////////////////////////////
// Base-relocation sidecar construction

static int
build_base_reloc_index(const U8 *file, U64 file_size, BaseRelocBuild *build)
{
  memset(build, 0, sizeof(*build));
  if (file_size < sizeof(ObjFileHeader)) return 1;
  U64 section_off;
  U32 section_count;
  U16 machine;
  const ObjBigHeader *big = (const ObjBigHeader *)file;
  if (file_size >= sizeof(*big) && big->sig1 == 0 && big->sig2 == 0xffff && big->version >= 2) {
    section_off = sizeof(*big); section_count = big->section_count; machine = big->machine;
  } else {
    const ObjFileHeader *h = (const ObjFileHeader *)file;
    section_off = sizeof(*h); section_count = h->section_count; machine = h->machine;
  }
  // RAD Link's base-reloc builder currently supports x64 only as well.
  if (machine != 0x8664) return 1;
  build->supported = 1;
  if (section_off > file_size || (U64)section_count * sizeof(ObjSectionHeader) > file_size - section_off) return 0;
  const ObjSectionHeader *sections = (const ObjSectionHeader *)(file + section_off);
  for (U32 sect_idx = 0; sect_idx < section_count; ++sect_idx) {
    const ObjSectionHeader *sec = &sections[sect_idx];
    U64 reloc_off = sec->relocs_foff;
    U64 reloc_count = sec->reloc_count;
    if ((sec->flags & 0x01000000u) && sec->reloc_count == 0xffff) {
      if (reloc_off > file_size || sizeof(ObjReloc) > file_size - reloc_off) return 0;
      const ObjReloc *counter = (const ObjReloc *)(file + reloc_off);
      if (counter->apply_off == 0) return 0;
      reloc_count = counter->apply_off - 1;
      reloc_off += sizeof(ObjReloc);
    }
    if (reloc_off > file_size || reloc_count > (file_size - reloc_off) / sizeof(ObjReloc)) return 0;
    const ObjReloc *relocs = (const ObjReloc *)(file + reloc_off);
    for (U64 i = 0; i < reloc_count; ++i) {
      U8 addr_size = relocs[i].type == 1 ? 8 : relocs[i].type == 2 ? 4 : 0;
      if (!addr_size) continue;
      if (build->disk.entry_count == build->cap) {
        U32 new_cap = build->cap ? build->cap * 2 : 1024;
        LNK_CObjBaseRelocEntry *entries = realloc(build->entries, (size_t)new_cap * sizeof(*entries));
        if (!entries) return 0;
        build->entries = entries;
        build->cap = new_cap;
      }
      LNK_CObjBaseRelocEntry *dst = &build->entries[build->disk.entry_count++];
      memset(dst, 0, sizeof(*dst));
      dst->sect_idx = sect_idx;
      dst->apply_off = relocs[i].apply_off;
      dst->isymbol = relocs[i].isymbol;
      dst->addr_size = addr_size;
    }
  }
  return 1;
}

////////////////////////////////
// Packed type-offset encoding

static U64
packed_v2_offset_storage_size(TypeBuild *b)
{
  U64 group_size = (U64)1 << LNK_COBJ_PACKED_TYPE_OFFSET_V2_SHIFT;
  U64 group_count = ((U64)b->disk.leaf_count + group_size - 1) / group_size;
  U64 directory_bytes = group_count * 2 * sizeof(U32);
  U64 payload_bytes = 0;
  for (U64 group_idx = 0; group_idx < group_count; ++group_idx) {
    U64 first = group_idx * group_size;
    U64 remain = (U64)b->disk.leaf_count - first;
    U64 count = group_size < remain ? group_size : remain;
    U32 base = b->offsets[first];
    U32 last_delta = b->offsets[first + count - 1] - base;
    payload_bytes += count * (last_delta <= 0xffffu ? 2 : 3);
  }
  return ((directory_bytes + 7) & ~(U64)7) + payload_bytes;
}

////////////////////////////////
// Entry point

int
main(int argc, char **argv)
{
  if (argc < 3 || argc > 7) {
    fprintf(stderr, "usage: rad_obj_compress <input.obj> <output.obj> [segment-kib=512] [selkie|mermaid|kraken] [space-speed=256] [superfast|veryfast|fast|normal|optimal1..5]\n");
    return 2;
  }
  U32 segment_size = argc >= 4 ? (U32)strtoul(argv[3], 0, 10) * 1024u : 512u * 1024u;
  OodleLZ_Compressor compressor = OodleLZ_Compressor_Kraken;
  if (argc >= 5) {
    if      (_stricmp(argv[4], "mermaid") == 0) compressor = OodleLZ_Compressor_Mermaid;
    else if (_stricmp(argv[4], "selkie")  == 0) compressor = OodleLZ_Compressor_Selkie;
    else if (_stricmp(argv[4], "kraken")  == 0) compressor = OodleLZ_Compressor_Kraken;
    else { fprintf(stderr, "invalid compressor: %s\n", argv[4]); return 2; }
  }
  OodleLZ_CompressionLevel compression_level = OodleLZ_CompressionLevel_Normal;
  if (argc >= 7) {
    if      (_stricmp(argv[6], "superfast") == 0) compression_level = OodleLZ_CompressionLevel_SuperFast;
    else if (_stricmp(argv[6], "veryfast")  == 0) compression_level = OodleLZ_CompressionLevel_VeryFast;
    else if (_stricmp(argv[6], "fast")      == 0) compression_level = OodleLZ_CompressionLevel_Fast;
    else if (_stricmp(argv[6], "normal")    == 0) compression_level = OodleLZ_CompressionLevel_Normal;
    else if (_stricmp(argv[6], "optimal1")  == 0) compression_level = OodleLZ_CompressionLevel_Optimal1;
    else if (_stricmp(argv[6], "optimal2")  == 0) compression_level = OodleLZ_CompressionLevel_Optimal2;
    else if (_stricmp(argv[6], "optimal3")  == 0) compression_level = OodleLZ_CompressionLevel_Optimal3;
    else if (_stricmp(argv[6], "optimal4")  == 0) compression_level = OodleLZ_CompressionLevel_Optimal4;
    else if (_stricmp(argv[6], "optimal5")  == 0) compression_level = OodleLZ_CompressionLevel_Optimal5;
    else { fprintf(stderr, "invalid compression level: %s\n", argv[6]); return 2; }
  }
  OodleLZ_CompressOptions options = *OodleLZ_CompressOptions_GetDefault(compressor, compression_level);
  options.spaceSpeedTradeoffBytes = argc >= 6 ? (OO_S32)strtol(argv[5], 0, 10) : 256;
  int packed_sidecar = 1;
  OodleLZ_CompressOptions_Validate(&options);
  if (segment_size < 64 * 1024 || (segment_size & (segment_size - 1)) != 0) {
    fprintf(stderr, "segment size must be a power of two and at least 64 KiB\n");
    return 2;
  }

  HANDLE input_file = CreateFileA(argv[1], GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  LARGE_INTEGER input_size = {0};
  if (input_file == INVALID_HANDLE_VALUE || !GetFileSizeEx(input_file, &input_size) || input_size.QuadPart <= 0) {
    fprintf(stderr, "unable to open input: %s\n", argv[1]); return 1;
  }
  U64 raw_size = (U64)input_size.QuadPart;
  HANDLE input_mapping = CreateFileMappingW(input_file, 0, PAGE_READONLY, 0, 0, 0);
  U8 *mapped_input = input_mapping ? (U8 *)MapViewOfFile(input_mapping, FILE_MAP_READ, 0, 0, 0) : 0;
  U8 *input_data = mapped_input;
  if (!input_data) { fprintf(stderr, "unable to map input: %s\n", argv[1]); return 1; }
  if (raw_size >= sizeof(LNK_CObjHeader) && ((LNK_CObjHeader *)input_data)->magic == LNK_COBJ_MAGIC) {
    fprintf(stderr, "input is already a compressed object: %s\n", argv[1]);
    return 1;
  }
  TypeBuild *type_builds = 0;
  U32 type_count = 0;
  if (!build_type_indices(input_data, raw_size, &type_builds, &type_count) || type_count > 0xffff) {
    fprintf(stderr, "unable to build type sidecar: %s\n", argv[1]); return 1;
  }
  if (packed_sidecar) {
    for (U32 i = 0; i < type_count; ++i) {
      if (!type_builds[i].packed_kinds_ok) { packed_sidecar = 0; break; }
    }
  }
  BaseRelocBuild base_relocs = {0};
  if (!build_base_reloc_index(input_data, raw_size, &base_relocs)) {
    fprintf(stderr, "unable to build base relocation sidecar: %s\n", argv[1]); return 1;
  }
  DebugSBuild debug_s_index = {0};
  if (!build_debug_s_index(input_data, raw_size, &debug_s_index)) {
    fprintf(stderr, "unable to build .debug$S sidecar: %s\n", argv[1]); return 1;
  }
  U64 segment_count_u64 = (raw_size + segment_size - 1) / segment_size;
  if (segment_count_u64 > UINT32_MAX) { fprintf(stderr, "input has too many segments\n"); return 1; }
  U32 segment_count = (U32)segment_count_u64;
  LNK_CObjSegment *directory = (LNK_CObjSegment *)calloc(segment_count, sizeof(*directory));
  U8 *verify = (U8 *)malloc(segment_size);
  OO_SINTa comp_cap = OodleLZ_GetCompressedBufferSizeNeeded(compressor, segment_size);
  U8 *comp = (U8 *)malloc((size_t)comp_cap);
  size_t temp_name_size = strlen(argv[2]) + 5;
  char *temp_name = (char *)malloc(temp_name_size);
  if (!temp_name) { fprintf(stderr, "unable to allocate output path\n"); return 1; }
  snprintf(temp_name, temp_name_size, "%s.tmp", argv[2]);
  FILE *out = fopen(temp_name, "wb+");
  if (!directory || !verify || !comp || !out) { fprintf(stderr, "allocation/output open failed\n"); return 1; }

  LNK_CObjHeader header = {0};
  header.magic = LNK_COBJ_MAGIC;
  header.version = LNK_COBJ_VERSION;
  header.header_size = sizeof(header);
  header.raw_size = raw_size;
  header.segment_size = segment_size;
  header.segment_count = segment_count;
  U64 layout_cursor = (sizeof(header) + 7) & ~(U64)7;
  U64 directory_bytes = (U64)segment_count * sizeof(*directory);
  header.directory_offset = layout_cursor;
  layout_cursor = (layout_cursor + directory_bytes + 7) & ~(U64)7;
  header.flags = LNK_COBJ_FLAG_PORTABLE_RAW_MAP;
  if (packed_sidecar) {
    header.flags |= LNK_COBJ_FLAG_PACKED_TYPE_SIDECAR | LNK_COBJ_FLAG_PACKED_TYPE_OFFSETS_V2;
  }
  if (type_count || base_relocs.supported || debug_s_index.supported) {
    U64 dirs_bytes = 0;
    header.flags |= LNK_COBJ_FLAG_TYPE_INDEX | LNK_COBJ_FLAG_UDT_HASH_INDEX |
                    (type_count << LNK_COBJ_TYPE_INDEX_COUNT_SHIFT);
    if (type_count == 0) {
      header.flags &= ~(LNK_COBJ_FLAG_TYPE_INDEX | LNK_COBJ_FLAG_UDT_HASH_INDEX);
    }
    dirs_bytes += (U64)type_count * (sizeof(LNK_CObjTypeIndex) + sizeof(LNK_CObjUdtHashIndex));
    if (base_relocs.supported) {
      header.flags |= LNK_COBJ_FLAG_BASE_RELOC_INDEX;
      dirs_bytes += sizeof(LNK_CObjBaseRelocIndex);
    }
    if (debug_s_index.supported) {
      header.flags |= LNK_COBJ_FLAG_DEBUG_S_INDEX | LNK_COBJ_FLAG_DEBUG_S_SUMMARY;
      dirs_bytes += sizeof(LNK_CObjDebugSIndex);
    }
    header.reserved = layout_cursor;
    layout_cursor = (layout_cursor + dirs_bytes + 7) & ~(U64)7;
    for (U32 i = 0; i < type_count; ++i) {
      TypeBuild *b = &type_builds[i];
      U64 offset_count = b->disk.leaf_count;
      U64 offsets_bytes = offset_count * sizeof(U32);
      U64 sizes_bytes = (U64)b->disk.leaf_count * sizeof(U16);
      U64 kinds_bytes = (U64)b->disk.leaf_count * sizeof(U16);
      if (packed_sidecar) {
        offsets_bytes = packed_v2_offset_storage_size(b);
        sizes_bytes = 256 * sizeof(U16);
        kinds_bytes = offset_count;
      }
      U64 hashes_bytes = (U64)b->udt_disk.hash_count * sizeof(U64);
      b->disk.offsets_file_offset = layout_cursor;
      layout_cursor = (layout_cursor + offsets_bytes + 7) & ~(U64)7;
      b->disk.sizes_file_offset = layout_cursor;
      layout_cursor = (layout_cursor + sizes_bytes + 7) & ~(U64)7;
      b->disk.kinds_file_offset = layout_cursor;
      layout_cursor = (layout_cursor + kinds_bytes + 7) & ~(U64)7;
      b->udt_disk.hashes_file_offset = hashes_bytes ? layout_cursor : 0;
      layout_cursor = (layout_cursor + hashes_bytes + 7) & ~(U64)7;
    }
    if (base_relocs.supported) {
      U64 entry_bytes = (U64)base_relocs.disk.entry_count * sizeof(LNK_CObjBaseRelocEntry);
      base_relocs.disk.entries_file_offset = entry_bytes ? layout_cursor : 0;
      layout_cursor = (layout_cursor + entry_bytes + 7) & ~(U64)7;
    }
    if (debug_s_index.supported) {
      U64 entry_bytes = (U64)debug_s_index.disk.entry_count * sizeof(LNK_CObjDebugSEntry);
      debug_s_index.disk.entries_file_offset = entry_bytes ? layout_cursor : 0;
      layout_cursor = (layout_cursor + entry_bytes + 7) & ~(U64)7;
      U64 summary_bytes = (U64)debug_s_index.disk.entry_count * sizeof(LNK_CObjDebugSSummary);
      layout_cursor = (layout_cursor + summary_bytes + 7) & ~(U64)7;
    }
  }
  header.compressor = (U32)compressor;
  header.data_offset = layout_cursor;
  if (!write_at(out, 0, &header, sizeof(header))) return 1;
  U64 sidecar_cursor = header.reserved;
  for (U32 i = 0; i < type_count; ++i) {
    if (!write_at(out, sidecar_cursor, &type_builds[i].disk, sizeof(LNK_CObjTypeIndex))) return 1;
    sidecar_cursor += sizeof(LNK_CObjTypeIndex);
  }
  for (U32 i = 0; i < type_count; ++i) {
    if (!write_at(out, sidecar_cursor, &type_builds[i].udt_disk, sizeof(LNK_CObjUdtHashIndex))) return 1;
    sidecar_cursor += sizeof(LNK_CObjUdtHashIndex);
  }
  if (base_relocs.supported) {
    if (!write_at(out, sidecar_cursor, &base_relocs.disk, sizeof(base_relocs.disk))) return 1;
    sidecar_cursor += sizeof(base_relocs.disk);
    U64 entry_bytes = (U64)base_relocs.disk.entry_count * sizeof(LNK_CObjBaseRelocEntry);
    if (entry_bytes && !write_at(out, base_relocs.disk.entries_file_offset, base_relocs.entries, entry_bytes)) return 1;
  }
  if (debug_s_index.supported) {
    if (!write_at(out, sidecar_cursor, &debug_s_index.disk, sizeof(debug_s_index.disk))) return 1;
    sidecar_cursor += sizeof(debug_s_index.disk);
    U64 entry_bytes = (U64)debug_s_index.disk.entry_count * sizeof(LNK_CObjDebugSEntry);
    if (entry_bytes && !write_at(out, debug_s_index.disk.entries_file_offset, debug_s_index.entries, entry_bytes)) return 1;
    U64 summary_offset = (debug_s_index.disk.entries_file_offset + entry_bytes + 7) & ~(U64)7;
    U64 summary_bytes = (U64)debug_s_index.disk.entry_count * sizeof(LNK_CObjDebugSSummary);
    if (summary_bytes && !write_at(out, summary_offset, debug_s_index.summaries, summary_bytes)) return 1;
  }
  for (U32 i = 0; i < type_count; ++i) {
    TypeBuild *b = &type_builds[i];
    if (packed_sidecar) {
      U64 group_size = (U64)1 << LNK_COBJ_PACKED_TYPE_OFFSET_V2_SHIFT;
      U64 group_count = ((U64)b->disk.leaf_count + group_size - 1) / group_size;
      U64 group_bytes = group_count * 2 * sizeof(U32);
      U64 payload_off = b->disk.offsets_file_offset + ((group_bytes + 7) & ~(U64)7);
      U64 storage_bytes = packed_v2_offset_storage_size(b);
      U64 payload_cap = storage_bytes - (payload_off - b->disk.offsets_file_offset);
      U32 *groups = calloc(group_count ? (size_t)group_count * 2 : 1, sizeof(U32));
      U8 *deltas = malloc((size_t)(payload_cap ? payload_cap : 1));
      if (!groups || !deltas) return 1;
      U64 payload_cursor = 0;
      for (U64 group_idx = 0; group_idx < group_count; ++group_idx) {
        U64 first = group_idx * group_size;
        U64 remain = (U64)b->disk.leaf_count - first;
        U64 count = group_size < remain ? group_size : remain;
        U32 base = b->offsets[first];
        U32 last_delta = b->offsets[first + count - 1] - base;
        U32 width = last_delta <= 0xffffu ? 2 : 3;
        groups[group_idx*2 + 0] = base;
        if (payload_cursor > UINT32_MAX) { fprintf(stderr, "packed v2 payload offset overflow\n"); return 1; }
        groups[group_idx*2 + 1] = (U32)payload_cursor | (width == 3);
        for (U64 i = 0; i < count; ++i) {
          U32 delta = b->offsets[first + i] - base;
          if ((width == 2 && delta > 0xffffu) || delta > 0xffffffu) {
            fprintf(stderr, "packed v2 type offset overflow\n"); return 1;
          }
          deltas[payload_cursor++] = (U8)(delta >> 0);
          deltas[payload_cursor++] = (U8)(delta >> 8);
          if (width == 3) { deltas[payload_cursor++] = (U8)(delta >> 16); }
        }
      }
      if (!write_at(out, b->disk.offsets_file_offset, groups, group_bytes) ||
          !write_at(out, payload_off, deltas, payload_cursor) ||
          !write_at(out, b->disk.sizes_file_offset, b->kind_dictionary, sizeof(b->kind_dictionary)) ||
          !write_at(out, b->disk.kinds_file_offset, b->kind_codes, b->disk.leaf_count)) return 1;
      free(groups);
      free(deltas);
      continue;
    }
    if (!write_at(out, b->disk.offsets_file_offset, b->offsets,
                  (U64)b->disk.leaf_count * sizeof(U32)) ||
        !write_at(out, b->disk.sizes_file_offset, b->sizes, (U64)b->disk.leaf_count * sizeof(U16)) ||
        !write_at(out, b->disk.kinds_file_offset, b->kinds, (U64)b->disk.leaf_count * sizeof(U16))) return 1;
    if (b->udt_disk.hash_count &&
        !write_at(out, b->udt_disk.hashes_file_offset, b->udt_hashes,
                  (U64)b->udt_disk.hash_count * sizeof(U64))) return 1;
  }

  U64 payload_cursor = header.data_offset;
  int in_raw_run = 0;
  for (U32 seg_idx = 0; seg_idx < segment_count; ++seg_idx) {
    U32 raw_len = (U32)((raw_size - (U64)seg_idx * segment_size) < segment_size ?
                        (raw_size - (U64)seg_idx * segment_size) : segment_size);
    U64 seg_min = (U64)seg_idx * segment_size;
    U8 *raw = input_data + seg_min;
    LNK_CObjSegment *entry = &directory[seg_idx];
    entry->raw_size = raw_len;
    OO_SINTa comp_len = OodleLZ_Compress(compressor, raw, raw_len, comp,
                                         compression_level, &options, 0, 0, 0, 0);
    if (comp_len <= 0 || comp_len >= raw_len) {
      entry->flags |= LNK_COBJ_SEGMENT_RAW;
      entry->stored_size = raw_len;
      if (!in_raw_run) { payload_cursor = (payload_cursor + 65535) & ~(U64)65535; }
      entry->file_offset = payload_cursor;
      if (!write_at(out, entry->file_offset, raw, raw_len)) return 1;
      U64 mapped_len = (seg_idx + 1 == segment_count) ? ((raw_len + 65535) & ~(U64)65535) : segment_size;
      payload_cursor += mapped_len;
      in_raw_run = 1;
    } else {
      in_raw_run = 0;
      entry->stored_size = (U32)comp_len;
      OO_SINTa verify_len = OodleLZ_Decompress(comp, comp_len, verify, raw_len,
                                                OodleLZ_FuzzSafe_Yes, OodleLZ_CheckCRC_No, OodleLZ_Verbosity_None,
                                                0, 0, 0, 0, 0, 0, OodleLZ_Decode_Unthreaded);
      if (verify_len != raw_len || memcmp(raw, verify, raw_len) != 0) {
        fprintf(stderr, "round-trip verification failed in segment %u\n", seg_idx);
        return 1;
      }
      payload_cursor = (payload_cursor + 7) & ~(U64)7;
      entry->file_offset = payload_cursor;
      if (!write_at(out, entry->file_offset, comp, (U64)comp_len)) return 1;
      payload_cursor += (U64)comp_len;
    }
  }
  if (!write_at(out, header.directory_offset, directory, (U64)segment_count * sizeof(*directory))) return 1;
  if (_chsize_s(_fileno(out), payload_cursor) != 0) {
    fprintf(stderr, "unable to finalize portable container size\n"); return 1;
  }
  fclose(out);
  UnmapViewOfFile(mapped_input);
  CloseHandle(input_mapping);
  CloseHandle(input_file);
  if (!MoveFileExA(temp_name, argv[2], MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    fprintf(stderr, "unable to publish output: %s\n", argv[2]);
    DeleteFileA(temp_name);
    return 1;
  }
  fprintf(stdout, "%s: %.2f MiB -> %.2f MiB (%.1f%%), %u segments\n", argv[1],
          (double)raw_size / (1024.0 * 1024.0), (double)payload_cursor / (1024.0 * 1024.0),
          raw_size ? 100.0 * (double)payload_cursor / (double)raw_size : 0.0, segment_count);
  for (U32 i = 0; i < type_count; ++i) { free(type_builds[i].offsets); free(type_builds[i].sizes); free(type_builds[i].kinds); free(type_builds[i].kind_codes); free(type_builds[i].udt_hashes); }
  free(debug_s_index.summaries); free(debug_s_index.entries); free(base_relocs.entries);
  free(type_builds); free(temp_name); free(comp); free(verify); free(directory);
  return 0;
}
