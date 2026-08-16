// Portable segmented-object format shared by the writer and RAD Link reader.
// Files are self-contained, all integers are little endian, and each compressed
// segment is independently decodable.

#pragma once

#define LNK_COBJ_MAGIC                              0x3130304A424F4C52ull /* "RLOBJ001" */
#define LNK_COBJ_VERSION                            1u
#define LNK_COBJ_SEGMENT_RAW                        0x00000001u
#define LNK_COBJ_FLAG_TYPE_INDEX                    0x00000001u
#define LNK_COBJ_FLAG_RETIRED_SPARSE_RAW_VIEW       0x00000002u // Reserved; never accepted by the portable reader.
#define LNK_COBJ_FLAG_UDT_HASH_INDEX                0x00000004u
#define LNK_COBJ_FLAG_BASE_RELOC_INDEX              0x00000008u
#define LNK_COBJ_FLAG_PORTABLE_RAW_MAP              0x00000010u
#define LNK_COBJ_FLAG_PACKED_TYPE_SIDECAR           0x00000040u
#define LNK_COBJ_FLAG_PACKED_TYPE_OFFSETS_V2        0x00000080u
#define LNK_COBJ_FLAG_DEBUG_S_INDEX                 0x00002000u
#define LNK_COBJ_FLAG_DEBUG_S_SUMMARY               0x00004000u
#define LNK_COBJ_PACKED_TYPE_OFFSET_V2_SHIFT 9u
#define LNK_COBJ_TYPE_INDEX_COUNT_SHIFT 16u
#define LNK_COBJ_TYPE_INDEX_COUNT_MASK  0xffff0000u

typedef struct LNK_CObjHeader
{
  U64 magic;
  U32 version;
  U32 header_size;
  U64 raw_size;
  U32 segment_size;
  U32 segment_count;
  U64 directory_offset;
  U64 data_offset;
  U32 compressor;
  U32 flags;
  U64 reserved;
} LNK_CObjHeader;

typedef struct LNK_CObjSegment
{
  U64 file_offset;
  U32 stored_size;
  U32 raw_size;
  U32 flags;
  U32 reserved;
} LNK_CObjSegment;

// Optional leaf index for .debug$T/.debug$P sections.  The three arrays are
// deliberately stored separately: type dedup scans only kinds, while offsets
// and sizes are touched only for the winning leaves.  All offsets are file
// offsets in the compressed container; leaf offsets are relative to the first
// byte after the four-byte CodeView signature.
typedef struct LNK_CObjTypeIndex
{
  U64 raw_section_offset;
  U32 raw_section_size;
  U32 leaf_count;
  // Normal layout: U32 offsets, U16 sizes, U16 kinds.
  // PACKED_TYPE_SIDECAR:
  //   U32 group[ceil(leaf_count/512)][2] = {absolute_base, payload_offset|is_u24};
  //   followed by U16 deltas for normal groups and U24 deltas for rare wide groups;
  //   sizes_file_offset -> U16 kind_dictionary[256] (sizes derive from adjacent offsets);
  //   kinds_file_offset -> U8 kind_code[leaf_count].
  U64 offsets_file_offset;
  U64 sizes_file_offset;
  U64 kinds_file_offset;
} LNK_CObjTypeIndex;

typedef struct LNK_CObjUdtHashIndex
{
  U64 hashes_file_offset;
  U32 hash_count;
  U32 reserved;
} LNK_CObjUdtHashIndex;

// Compact candidates for PE base relocation generation.  The compressor filters the full COFF
// relocation streams down to address relocations; RAD Link still applies section-liveness and
// symbol-interpretation checks because those depend on the final link.
typedef struct LNK_CObjBaseRelocIndex
{
  U64 entries_file_offset;
  U32 entry_count;
  U32 reserved;
} LNK_CObjBaseRelocIndex;

typedef struct LNK_CObjBaseRelocEntry
{
  U32 sect_idx;
  U32 apply_off;
  U32 isymbol;
  U8  addr_size;
  U8  reserved[3];
} LNK_CObjBaseRelocEntry;

// Compact directory for C13 subsection payloads.  Parsing .debug$S normally walks every
// subsection header in the logical COFF view, which needlessly decodes all compressed symbol
// segments just to construct String8 slices.  Entries are grouped by raw_section_offset and let
// the linker construct those slices without touching payload pages.  COFF section file offsets
// are U32 by definition, so the complete entry stays at 16 bytes.
typedef struct LNK_CObjDebugSIndex
{
  U64 entries_file_offset;
  U32 entry_count;
  U32 reserved;
} LNK_CObjDebugSIndex;

typedef struct LNK_CObjDebugSEntry
{
  U32 raw_section_offset;
  U32 raw_payload_offset;
  U32 raw_payload_size;
  U32 kind;
} LNK_CObjDebugSEntry;

// Size-only module/GSI prepass results for the corresponding DEBUG_S_INDEX entry.  These values
// depend only on symbol size/kind fields, which relocation and type-index fixups never alter.
typedef struct LNK_CObjDebugSSummary
{
  U32 module_symbol_size;
  U32 gsi_candidate_count;
  U32 proc_ref_count;
  U32 flags;
} LNK_CObjDebugSSummary;

#define LNK_COBJ_DEBUG_S_SUMMARY_HAS_LOCALS 0x1u
