# RAD Link portable compressed OBJ format

This document is the implementation contract for producing portable compressed OBJ files that the
current RAD Link compressed-OBJ reader accepts. It describes format version 1, identified by the
eight-byte ASCII magic `RLOBJ001`.

The intended reader and reference writer are:

- `src/linker/lnk_compressed_obj_format.h`
- `src/linker/lnk_compressed_obj.c`
- `src/linker/rad_obj_compress.c`

If this document and the reader ever disagree, the reader is authoritative. A format change must
use a new version or magic; silently changing version 1 would invalidate cached objects.

## 1. Implementation summary

For the tested production profile, the writer should:

1. Finish producing one normal COFF or BigObj byte stream in memory.
2. Split that byte stream into independent 512 KiB logical segments.
3. Compress each segment independently with Oodle Kraken, level Normal, and
   `spaceSpeedTradeoffBytes = 256`.
4. Store a segment raw when compression fails or does not make it smaller.
5. Emit the 64-byte `RLOBJ001` header, the 24-byte-per-segment directory, the performance
   sidecars described below, and then the segment payloads.
6. Verify every compressed segment by decoding it and comparing it with the original bytes.
7. Publish through a temporary file plus an atomic rename.

The following settings produced the measured 22.97 GiB FortniteClient and 24.39 GiB UEFN corpora:

| Setting | Required/recommended value |
|---|---|
| Container | `RLOBJ001`, version 1 |
| Byte order | Little-endian |
| Segment size | 512 KiB (`524288`) |
| Oodle compressor | Kraken (`OodleLZ_Compressor_Kraken`, value 8 in Oodle 2.9.16) |
| Oodle level | Normal |
| `spaceSpeedTradeoffBytes` | 256 |
| Segment dependencies | None; every segment is a separate Oodle stream |
| Incompressible data | Store raw |
| Filesystem features | None required; ordinary portable file |

All compressed OBJ inputs in a single RAD Link invocation **must use exactly the same segment
size**. The cache is initialized from the first compressed OBJ and rejects a later compressed OBJ
with a different segment size. Use 512 KiB consistently for the entire corpus.

This is **not** the UBA 12-byte-header format with an eight-byte size prefix between segments. UBA
payloads can use the same Oodle codec, but they must be repackaged into the `RLOBJ001` header and
directory layout described here before this RAD Link reader can consume them.

## 2. Compatibility levels

There are two useful conformance levels.

### 2.1 Core container: correct but not performance-equivalent

A minimal compatible file needs only:

- the fixed header;
- the segment directory;
- independently compressed or raw-stored segment payloads; and
- the `PORTABLE_RAW_MAP` header flag.

With no sidecar flags, set `header.reserved` to zero. RAD Link can reconstruct the original OBJ
view and link it correctly, but it must fault or decode more debug information through the generic
cache. This level is useful for bringing up and validating a new writer, but it is not expected to
match the benchmarked link time or memory use.

### 2.2 Optimized container: the measured production profile

To reproduce the measured behavior, also emit:

- the packed `.debug$T`/`.debug$P` leaf index;
- the complete-UDT hash index;
- the AMD64 base-relocation candidate index;
- the `.debug$S` subsection index; and
- the `.debug$S` summaries.

These sidecars are optimization hints derived entirely from the original OBJ. They do not replace
the original bytes. Incorrect sidecar data can change linker behavior, so omit an optional sidecar
until its implementation is known to be correct rather than emitting an approximation.

## 3. General encoding rules

- All integers are unsigned and little-endian.
- File offsets are absolute offsets from byte zero of the compressed file.
- Logical or raw offsets refer to offsets in the original uncompressed OBJ byte stream.
- `align_up(x, n)` means `(x + n - 1) & ~(n - 1)`, with `n` a power of two.
- All fields named `reserved` must be written as zero unless this document assigns them a meaning.
- Do not serialize the records with `#pragma pack(1)`. Use the byte offsets below, or use the C
  declarations with their normal layout and assert every size.
- The ordinary output file length must cover all metadata, stored payloads, and required raw-run
  padding. The format does not depend on sparse files, filesystem compression, bundles, or another
  OBJ.
- Direct compressed inputs must currently use the `.obj` or `.o` extension. Use `.obj` for the
  Windows toolchain. RAD Link's command-line parser classifies the path by extension before reading
  the magic, so a path ending in `.radobj` is rejected as an unknown file format even when its
  contents are a valid `RLOBJ001` container. The stored bytes are a RAD Link container rather than
  a COFF file, and other linkers will not understand it.

Recommended compile-time checks for a C or C++ implementation are:

```cpp
static_assert(sizeof(LNK_CObjHeader)         == 64);
static_assert(sizeof(LNK_CObjSegment)        == 24);
static_assert(sizeof(LNK_CObjTypeIndex)      == 40);
static_assert(sizeof(LNK_CObjUdtHashIndex)   == 16);
static_assert(sizeof(LNK_CObjBaseRelocIndex) == 16);
static_assert(sizeof(LNK_CObjBaseRelocEntry) == 16);
static_assert(sizeof(LNK_CObjDebugSIndex)    == 16);
static_assert(sizeof(LNK_CObjDebugSEntry)    == 16);
static_assert(sizeof(LNK_CObjDebugSSummary)  == 16);
```

## 4. Top-level file layout

The reference writer uses this order:

```text
0
+-------------------------------+
| 64-byte header                |
+-------------------------------+ 8-byte aligned
| segment directory            | segment_count * 24 bytes
+-------------------------------+ 8-byte aligned
| optional sidecar directories |
+-------------------------------+ 8-byte aligned
| optional sidecar payloads    |
+-------------------------------+ 8-byte aligned
| segment payloads and padding |
+-------------------------------+ file length
```

Sidecar and segment offsets make the format relocatable inside the file. A production writer
should nevertheless use the canonical order above, keep ranges non-overlapping, and monotonically
advance a checked 64-bit layout cursor.

## 5. Fixed header

The header is exactly 64 bytes.

| Offset | Type | Field | Value and meaning |
|---:|---:|---|---|
| 0 | `U64` | `magic` | `0x3130304A424F4C52`; bytes spell `RLOBJ001` |
| 8 | `U32` | `version` | `1` |
| 12 | `U32` | `header_size` | `64` |
| 16 | `U64` | `raw_size` | Exact byte length of the original OBJ; must be nonzero |
| 24 | `U32` | `segment_size` | Power of two, at least 64 KiB; use 512 KiB |
| 28 | `U32` | `segment_count` | `ceil(raw_size / segment_size)` |
| 32 | `U64` | `directory_offset` | Absolute offset of the segment directory; canonical value is 64 |
| 40 | `U64` | `data_offset` | Canonical first segment-payload position before payload alignment |
| 48 | `U32` | `compressor` | Oodle compressor enum; write 8 for Kraken |
| 52 | `U32` | `flags` | Container flags plus type-index count in bits 16-31 |
| 56 | `U64` | `reserved` | When sidecars exist, absolute offset of the sidecar directory block; otherwise zero |

`data_offset` and `compressor` are descriptive in the version-1 reader; individual directory
entries locate payloads and Oodle identifies its stream internally. Writers must still populate
both fields correctly so future readers, diagnostics, and validators can rely on them.

### 5.1 Header flags

| Value | Name | Meaning |
|---:|---|---|
| `0x00000001` | `TYPE_INDEX` | One or more type-index directory records are present |
| `0x00000002` | retired | Never set; the portable reader rejects it |
| `0x00000004` | `UDT_HASH_INDEX` | One UDT-hash directory follows each type-index directory |
| `0x00000008` | `BASE_RELOC_INDEX` | A base-relocation directory is present |
| `0x00000010` | `PORTABLE_RAW_MAP` | Required for every version-1 portable file |
| `0x00000040` | `PACKED_TYPE_SIDECAR` | Type sidecars use the packed representation |
| `0x00000080` | `PACKED_TYPE_OFFSETS_V2` | Packed offset representation is version 2 |
| `0x00002000` | `DEBUG_S_INDEX` | A `.debug$S` directory is present |
| `0x00004000` | `DEBUG_S_SUMMARY` | One summary follows each `.debug$S` entry |
| `0xffff0000` | type count | Number of type-index records, shifted left by 16 |

All unlisted bits must be zero. The reader rejects unknown bits.

Flag dependencies are:

- `UDT_HASH_INDEX` requires `TYPE_INDEX`.
- `DEBUG_S_SUMMARY` requires `DEBUG_S_INDEX`.
- `PACKED_TYPE_SIDECAR` and `PACKED_TYPE_OFFSETS_V2` must either both be set or both be clear.
- A nonzero type count requires `TYPE_INDEX`.
- `TYPE_INDEX` requires a nonzero type count, with a maximum of 65535.

The packed flags have no purpose without a type index; a new writer should leave them clear when
there are no type records.

## 6. Segment directory

The segment directory contains `segment_count` consecutive 24-byte entries. Entry `i` describes
logical bytes beginning at `i * segment_size` in the original OBJ.

| Entry offset | Type | Field | Meaning |
|---:|---:|---|---|
| 0 | `U64` | `file_offset` | Absolute offset of the stored payload |
| 8 | `U32` | `stored_size` | Number of meaningful bytes stored at `file_offset` |
| 12 | `U32` | `raw_size` | Number of original bytes represented by this segment |
| 16 | `U32` | `flags` | Zero for Oodle data; `0x1` for raw data |
| 20 | `U32` | `reserved` | Zero |

The only segment flag is:

```text
LNK_COBJ_SEGMENT_RAW = 0x00000001
```

For every entry:

```text
raw_offset = i * segment_size
raw_size   = min(segment_size, header.raw_size - raw_offset)
```

`stored_size` must be nonzero, and `[file_offset, file_offset + stored_size)` must be inside the
file. If `RAW` is set, `stored_size` must equal `raw_size`. If `RAW` is clear, the stored bytes must
be one complete Oodle stream that decodes to exactly `raw_size` bytes.

The writer should choose raw storage when `OodleLZ_Compress` returns a nonpositive length or a
length greater than or equal to `raw_size`. This prevents compression from growing the file.

### 6.1 Oodle call contract

The tested writer initializes options from:

```cpp
OodleLZ_Compressor compressor = OodleLZ_Compressor_Kraken;
OodleLZ_CompressionLevel level = OodleLZ_CompressionLevel_Normal;
OodleLZ_CompressOptions options =
    *OodleLZ_CompressOptions_GetDefault(compressor, level);
options.spaceSpeedTradeoffBytes = 256;
OodleLZ_CompressOptions_Validate(&options);
```

It calls `OodleLZ_Compress` once per segment, with no dictionary base, long-range matcher, or shared
state. Do not continue an Oodle stream from one segment into the next. RAD Link may request any
segment first and may decode several segments concurrently.

RAD Link decodes with fuzz safety enabled, quantum CRC checking disabled, no caller scratch, and
`OodleLZ_Decode_Unthreaded`. The writer currently does not request Oodle quantum CRCs. Integrity of
the complete portable object should be provided by the cache system's normal content hash.

Do not mix Oodle codecs within one object. Version 1 has only one header-level compressor field,
even though the current Oodle decode entry point can identify a stream without consulting it.

### 6.2 Raw-segment placement

Raw segments are mapped directly from the portable file into RAD Link's logical OBJ address range.
Their layout therefore has additional requirements beyond the basic directory validation.

For each maximal run of consecutive raw segments:

1. Align the first raw segment's `file_offset` to 64 KiB.
2. Store full raw segments contiguously at `segment_size` intervals.
3. Because `segment_size` is a power of two and at least 64 KiB, every segment in the run remains
   64 KiB aligned.
4. If the final OBJ segment is raw and shorter than `segment_size`, extend the physical file through
   `align_up(raw_size, 64 KiB)` bytes for that final segment. Padding bytes may be zero and are not
   part of `stored_size`.

Canonical payload-cursor logic is:

```text
in_raw_run = false
cursor = header.data_offset

for each segment:
    if storing raw:
        if !in_raw_run:
            cursor = align_up(cursor, 64 KiB)
        entry.file_offset = cursor
        write raw_size bytes
        cursor += is_final_segment
                    ? align_up(raw_size, 64 KiB)
                    : segment_size
        in_raw_run = true
    else:
        in_raw_run = false
        cursor = align_up(cursor, 8)
        entry.file_offset = cursor
        write stored_size compressed bytes
        cursor += stored_size
```

The final file length is the final cursor, including raw-run padding. Do not rely on a sparse range
to provide the padding.

## 7. Optional sidecar directory block

When any sidecar is present, `header.reserved` points to one tightly packed directory block. The
block has no separate header. Its record order is determined entirely by `header.flags`:

```text
LNK_CObjTypeIndex[type_count]          if TYPE_INDEX
LNK_CObjUdtHashIndex[type_count]       if UDT_HASH_INDEX
LNK_CObjBaseRelocIndex                 if BASE_RELOC_INDEX
LNK_CObjDebugSIndex                    if DEBUG_S_INDEX
```

There is no padding between these directory record arrays. Align the end of the complete block to
8 bytes before writing sidecar payload arrays. Sidecar payload offsets are absolute and each
payload should begin at an 8-byte-aligned file offset.

## 8. Type leaf sidecar

Emit one `LNK_CObjTypeIndex` for each `.debug$T` or `.debug$P` COFF section that has at least the
four-byte CodeView signature. Preserve COFF section order.

The record is 40 bytes:

| Offset | Type | Field | Meaning |
|---:|---:|---|---|
| 0 | `U64` | `raw_section_offset` | Original OBJ offset of the first byte after the 4-byte CodeView signature |
| 8 | `U32` | `raw_section_size` | Section raw size minus 4 |
| 12 | `U32` | `leaf_count` | Number of valid leaf records indexed |
| 16 | `U64` | `offsets_file_offset` | Offset array or packed-offset block |
| 24 | `U64` | `sizes_file_offset` | Size array, or packed kind dictionary |
| 32 | `U64` | `kinds_file_offset` | Kind array, or packed kind-code array |

For a section whose `PointerToRawData` is `foff` and `SizeOfRawData` is `fsize`:

```text
raw_section_offset = foff + 4
raw_section_size   = fsize - 4
```

Starting at `raw_section_offset`, parse leaves as follows:

```text
cursor = 0
while cursor + 4 <= raw_section_size:
    size = read_u16(cursor + 0)
    kind = read_u16(cursor + 2)
    stride = size + 2
    if size < 2 or stride > raw_section_size - cursor:
        stop
    append offset=cursor, size=size, kind=kind
    cursor += stride
```

Leaf offsets are relative to the first byte after the CodeView signature, not relative to the OBJ
or compressed container. The stored 16-bit `size` is the CodeView record size field: it excludes
the size field itself and includes the kind and payload.

### 8.1 Normal type representation

When the packed flags are clear, write three separate arrays:

```text
offsets_file_offset -> U32 offsets[leaf_count]
sizes_file_offset   -> U16 sizes[leaf_count]
kinds_file_offset   -> U16 kinds[leaf_count]
```

### 8.2 Packed type representation, version 2

The tested profile uses the packed representation for every type section if every section has at
most 256 distinct leaf kinds. If any type section exceeds that limit, clear both packed flags and
use the normal representation for all type sections in that object.

Packed offsets use groups of 512 leaves:

```text
group_size  = 512
group_count = ceil(leaf_count / 512)
```

At `offsets_file_offset`, write `group_count` pairs of little-endian `U32` values:

```text
struct Group {
    U32 absolute_base;
    U32 payload_offset_and_width;
};
```

The group directory is followed by padding to an eight-byte boundary relative to
`offsets_file_offset`, then the delta payload:

```text
group_bytes    = group_count * 8
payload_start  = offsets_file_offset + align_up(group_bytes, 8)
```

For each group:

- `absolute_base` is the first leaf offset in that group.
- Select a two-byte delta if the last offset minus the base is at most `0xffff`; otherwise select a
  three-byte delta. Offsets are monotonically increasing, so checking the last delta is sufficient.
- `payload_offset_and_width & ~1` is the byte offset from `payload_start` to this group's deltas.
- Bit zero is 0 for two-byte deltas and 1 for three-byte deltas.
- Write one little-endian two- or three-byte delta per leaf, including a zero delta for the first
  leaf.
- A three-byte delta must not exceed `0xffffff`.

The second and third packed arrays repurpose the legacy field names:

```text
sizes_file_offset -> U16 kind_dictionary[256]  // exactly 512 bytes
kinds_file_offset -> U8  kind_code[leaf_count]
```

Assign dictionary codes in first-seen leaf order. Zero-fill unused dictionary entries. A leaf's
kind is `kind_dictionary[kind_code[i]]`. Leaf sizes are derived from adjacent offsets; the final
leaf ends at `raw_section_size`.

## 9. Complete-UDT hash sidecar

When `UDT_HASH_INDEX` is set, write exactly `type_count` consecutive 16-byte directory records
immediately after all `LNK_CObjTypeIndex` records:

| Offset | Type | Field | Meaning |
|---:|---:|---|---|
| 0 | `U64` | `hashes_file_offset` | Absolute offset of `U64` hashes, or zero when count is zero |
| 8 | `U32` | `hash_count` | Number of hashes |
| 12 | `U32` | `reserved` | Zero |

Each record corresponds by index to the type-index record at the same index. Hashes are appended in
leaf order, but only for complete UDT definitions with nonempty unique names.

The reference algorithm operates on the leaf payload after the two-byte kind field:

| Leaf kind | Minimum payload bytes | Properties field | Cursor after fixed header |
|---:|---:|---|---:|
| `0x1504`, `0x1505`, `0x1519` | 16 | `U16` at `payload + 2` | 16 |
| `0x1506` | 8 | `U16` at `payload + 2` | 8 |
| `0x1507` | 12 | `U16` at `payload + 2` | 12 |
| `0x1608`, `0x1609` | 20 | `U32` at `payload + 0` | 20 |

Ignore other kinds. Require property bit `0x0200` and reject property bit `0x0080`. For every kind
except `0x1507`, skip one CodeView numeric value at the cursor. Then skip the first NUL-terminated
name and hash the following nonempty NUL-terminated unique name.

CodeView numeric length is:

- two bytes when the first `U16` is below `0x8000`;
- otherwise two bytes for the numeric tag plus payload sizes: `0x8000:1`,
  `0x8001/0x8002/0x801c:2`, `0x8003/0x8004/0x8005:4`,
  `0x8006/0x8009/0x800a/0x800c:8`, `0x8007:10`, `0x8008:16`, `0x800b:6`,
  `0x800d:16`, `0x800e:20`, `0x800f:32`, and `0x8017/0x8018:16`.

Hash the unique-name bytes with wrapping 64-bit arithmetic:

```text
h = 5381
for byte in unique_name:
    h = ((h << 5) + h) XOR byte
h = h OR 1
```

The `OR 1` is mandatory because zero is the linker's empty-set sentinel.

## 10. AMD64 base-relocation sidecar

This optional sidecar is supported for AMD64 COFF machine `0x8664`. Omit the flag on unsupported
machines.

The 16-byte directory is:

| Offset | Type | Field | Meaning |
|---:|---:|---|---|
| 0 | `U64` | `entries_file_offset` | Absolute entry-array offset, or zero when count is zero |
| 8 | `U32` | `entry_count` | Number of candidate relocations |
| 12 | `U32` | `reserved` | Zero |

Each 16-byte entry is:

| Offset | Type | Field | Meaning |
|---:|---:|---|---|
| 0 | `U32` | `sect_idx` | Zero-based COFF section index |
| 4 | `U32` | `apply_off` | Relocation's section-relative application offset |
| 8 | `U32` | `isymbol` | COFF symbol-table index |
| 12 | `U8` | `addr_size` | 8 for `IMAGE_REL_AMD64_ADDR64`, 4 for `IMAGE_REL_AMD64_ADDR32` |
| 13 | `U8[3]` | `reserved` | Zero |

Scan sections in COFF order and relocations in table order. Retain only AMD64 relocation type 1
(`ADDR64`) and type 2 (`ADDR32`). RAD Link still applies symbol interpretation and section
liveness checks after reading this candidate list.

Support `IMAGE_SCN_LNK_NRELOC_OVFL` (`0x01000000`) in the usual COFF way. When the section header's
relocation count is `0xffff`, the first relocation is the overflow counter; require its
`VirtualAddress`/`apply_off` to be nonzero, use `apply_off - 1` real entries, and skip the counter.

## 11. `.debug$S` sidecar

The 16-byte directory is:

| Offset | Type | Field | Meaning |
|---:|---:|---|---|
| 0 | `U64` | `entries_file_offset` | Absolute entry-array offset, or zero when count is zero |
| 8 | `U32` | `entry_count` | Number of retained C13 subsections |
| 12 | `U32` | `reserved` | Zero |

Each 16-byte entry is:

| Offset | Type | Field | Meaning |
|---:|---:|---|---|
| 0 | `U32` | `raw_section_offset` | Original OBJ offset of the `.debug$S` section, including its signature |
| 4 | `U32` | `raw_payload_offset` | Original OBJ offset of the subsection payload |
| 8 | `U32` | `raw_payload_size` | Payload length, clamped to bytes remaining in the section |
| 12 | `U32` | `kind` | C13 subsection kind |

Scan `.debug$S` sections in COFF section order. Skip the four-byte CodeView signature and parse
subsections as:

```text
cursor = 0
while cursor + 8 <= bytes_after_signature:
    kind         = read_u32(cursor + 0)
    payload_size = read_u32(cursor + 4)
    payload_rel  = cursor + 8
    clamped_size = min(payload_size, bytes_after_signature - payload_rel)

    if (kind & 0x80000000) == 0:
        append entry for this payload

    cursor = align_up(payload_rel + payload_size, 4)
    stop on arithmetic overflow or failure to advance
```

Unknown kinds with bit 31 clear are retained. Kinds with bit 31 set are ignored. Entries must be
grouped by and nondecreasing in `raw_section_offset`, which naturally follows from scanning COFF
sections in order.

### 11.1 `.debug$S` summary array

When `DEBUG_S_SUMMARY` is set, one 16-byte summary exists for every debug-S entry. It begins at:

```text
summary_offset = align_up(entries_file_offset + entry_count * 16, 8)
```

There is no summary offset in the directory; the reader derives it using this formula.

| Offset | Type | Field | Meaning |
|---:|---:|---|---|
| 0 | `U32` | `module_symbol_size` | Four-byte-aligned size of symbols retained in the PDB module stream |
| 4 | `U32` | `gsi_candidate_count` | Number of candidate GSI records |
| 8 | `U32` | `proc_ref_count` | Number of procedure-reference candidates |
| 12 | `U32` | `flags` | Bit 0 means the subsection contains local data |

Write an all-zero summary for subsection kinds other than `0xF1` (symbols). For an `0xF1` payload,
walk byte-packed symbol records. Each record starts with `U16 record_size, U16 kind`; its input
stride is `record_size + 2`, while its contribution to `module_symbol_size` is that stride aligned
up to four bytes. Stop at a record smaller than four bytes or one that overruns the payload.

Use these kind sets:

```text
global = {1107,0102,0202,1008,110d,020e,100f,1113}
typedef = {0004,1003,1108}
scope = {1110,110f,1103,1102,114d,115d,1104,1132,1147,1146}
end = {0006,114f,114e}
procedure-reference = {1110,110f,1147,1146}
has-locals = {113e,1111,110c,110d,1112,1113,1153,1107}
```

For `gsi_candidate_count`, begin each symbol subsection with candidate scope depth zero and active
state true. Count a global kind, or a typedef at candidate depth zero, while active. Scope kinds
increment candidate depth. An end kind decrements a positive depth; an end at depth zero makes the
candidate state inactive for the rest of that subsection.

Maintain a separate module scope depth across all symbol subsections in the OBJ. A symbol belongs
to the module stream unless it is kind `0x0007`, a global kind, a typedef at module depth zero, or
kind `0x1176`. For a retained symbol, update module depth for scope/end kinds and add
`align_up(record_size + 2, 4)` to `module_symbol_size`.

Set summary flag bit `0x1` if any symbol kind is in the `has-locals` set.

## 12. Canonical layout algorithm

The following order matches the reference writer:

```text
cursor = align_up(64, 8)

header.directory_offset = cursor
cursor = align_up(cursor + segment_count * 24, 8)

if any sidecar directory is present:
    header.reserved = cursor
    cursor += type_count * 40                       if TYPE_INDEX
    cursor += type_count * 16                       if UDT_HASH_INDEX
    cursor += 16                                    if BASE_RELOC_INDEX
    cursor += 16                                    if DEBUG_S_INDEX
    cursor = align_up(cursor, 8)
else:
    header.reserved = 0

for each type index:
    assign offsets block; align cursor to 8
    assign sizes/dictionary block; align cursor to 8
    assign kinds/codes block; align cursor to 8
    assign UDT hashes block if nonempty; align cursor to 8

assign base-relocation entries if nonempty; align cursor to 8
assign debug-S entries if nonempty; align cursor to 8
assign debug-S summaries; align cursor to 8

header.data_offset = cursor
write segment payloads using the rules in section 6
rewrite the completed segment directory
truncate the file to the final payload cursor
```

Use checked 64-bit addition and multiplication at every step. Reject any value that cannot be
represented by its destination field.

## 13. Reader acceptance rules

Before publishing a container, validate at least the same conditions RAD Link applies:

### Header and directory

- File size is at least 64 bytes.
- Magic, version, and header size are exact.
- `raw_size` is nonzero.
- `segment_size` is a power of two and at least 64 KiB.
- `segment_count == ceil(raw_size / segment_size)` without arithmetic overflow.
- The directory range is in the file.
- `PORTABLE_RAW_MAP` is set, no unknown flags are set, and all flag dependencies hold.
- Every segment has the expected raw size, a nonzero stored size, known flags, and an in-file
  payload range.
- Every raw segment has `stored_size == raw_size`.
- Every raw run satisfies the 64 KiB mapping rules in section 6.2.

### Type sidecars

- The sidecar directory and every payload array are in the file.
- Every indexed raw section range is inside the original OBJ.
- Packed group directories and payload ranges are in bounds.
- Every packed descriptor selects width 2 or 3 through bit zero, points inside the delta payload,
  and has enough bytes for its group.
- Offsets are monotonic, inside the indexed raw section, and reproduce the actual leaf boundaries.
- Packed kind codes resolve to the intended dictionary entry.

### Other sidecars

- Every UDT hash, base-relocation entry, debug-S entry, and summary array is in the file.
- Debug-S payload ranges are inside the original OBJ.
- Debug-S entries are nondecreasing by `raw_section_offset`.
- Sidecar contents are regenerated from the exact raw OBJ bytes and tested against a link without
  sidecars.

Finally, decode every compressed segment with the same Oodle decode options as RAD Link and compare
it byte-for-byte with the corresponding original segment.

## 14. Publication and cache integrity

Write to a temporary path in the destination directory. Flush and close it, complete all validation,
then atomically replace the destination. A failed conversion must never leave a file beginning with
`RLOBJ001` at the final path.

Version 1 has no container checksum and does not enable Oodle quantum CRCs. The artifact cache must
continue to verify its normal whole-file content hash on upload and download. That hash covers the
header, sidecars, compressed payloads, raw payloads, and alignment padding.

The artifact key must distinguish at least:

- container magic and version;
- segment size;
- codec and compression level;
- compression options, including space/speed tradeoff; and
- sidecar schema/implementation version.

Even when two settings decode to the same original OBJ, they are different stored artifacts.

## 15. Bring-up and correctness test plan

Use this order for a new writer:

1. Emit a segment-only container with all segments compressed, avoiding raw mapping initially.
2. Decode every segment in a standalone validator and reconstruct the original OBJ byte-for-byte.
3. Link a small target from raw OBJs and compressed OBJs; compare image and PDB hashes.
4. Enable raw fallback and test files whose first, middle, and final segments are raw, including a
   short final raw segment.
5. Link a response file mixing raw OBJs, compressed direct OBJ inputs, and ordinary `.lib` files.
6. Add sidecars one at a time, comparing image and PDB hashes after each addition.
7. Convert a complete large corpus with one uniform 512 KiB segment size and run warm and cold-cache
   performance tests.
8. Corrupt each metadata field in validator tests and confirm rejection rather than an out-of-bounds
   read or a partially accepted container.

Compressed direct OBJ inputs can coexist with raw direct OBJ inputs and ordinary `.lib` files.
Compressed members inside a `.lib` archive are not supported by the current reader.

The repository's reference smoke path is:

```bat
scripts\build_cobj_test.bat
scripts\run_cobj_smoke.bat
```

For corpus conversion and deterministic image/PDB comparisons, see
`docs/compressed_obj_workflow.md` and the scripts under `scripts/`.
