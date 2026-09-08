# Portable compressed OBJ files

## Overview

RAD Link can optionally consume independently compressed, self-contained OBJ files. The format is
intended for targets whose raw OBJ corpus is large enough that disk traffic and file-backed pages
dominate link time and peak working set.

Each source OBJ remains one ordinary portable file. The format does not use sparse allocation,
filesystem compression, bundles, shared manifests, or dependencies between OBJ files. Raw OBJ
files, compressed OBJ files, and ordinary `.lib` inputs can coexist in one link.

Oodle support is opt-in. This repository does not contain or distribute Oodle headers, libraries,
or binaries. An Oodle-enabled build consumes a separately supplied SDK through `OODLE_SDK_DIR`.
The ordinary RAD Link release build has no Oodle compile-time or link-time dependency.

## Measured results

These are process-start-to-process-exit measurements from the final reviewed reader. File sizes are
ordinary file lengths on disk; neither corpus uses filesystem compression or sparse allocation.
Peak commit and peak working set are process peaks sampled at 50 ms intervals.

| Target and input | Warm wall | Peak commit | Peak working set | OBJ corpus |
|---|---:|---:|---:|---:|
| FortniteClient, raw mmap | 15.36 s | 18.12 GiB | 95.50 GiB | 81.01 GiB |
| FortniteClient, compressed | 13.27 s | 15.06 GiB | 35.90 GiB | 22.97 GiB |
| UEFN DLL, raw mmap | 23.55 s | 28.45 GiB | 111.58 GiB | 89.22 GiB |
| UEFN DLL, compressed | 21.92 s | 22.27 GiB | 47.86 GiB | 24.39 GiB |

For the cold-cache comparison, the OS file cache was purged separately before each link:

| Target | Raw mmap | Compressed |
|---|---:|---:|
| FortniteClient | 28.51 s | 14.87 s |
| UEFN DLL | 30.56 s | 21.26 s |

The compressed format reduced the tested corpora to roughly 28% of their original size and cut
peak working set by 57-62%. Warm results vary by roughly 0.5-1.0 seconds because PDB output is
asynchronous; a fully cached raw mmap link can occasionally be faster. The cold-cache and memory
improvements were consistent.

Correctness was checked against raw-OBJ control links using the same output paths:

- FortniteClient EXE SHA-256 (raw and compressed):
  `1E126B7C8FAA2B730C380D9CF61131CE2E9175FAEC37A833D474952C5168D8F3`
- FortniteClient PDB SHA-256 (raw and compressed):
  `39E040E711381CE48CDDE5238E7909F2083DE5DF785BF9FAAA819061AE6F56D7`
- UEFN DLL SHA-256 (raw and compressed):
  `71EFFECB950BBF6E6DC6CBFFD20BA434E69388F024CAC21E083F0EA48DC7D1B5`

## Build configurations

Build the ordinary release without Oodle:

```bat
build.bat radlink release
```

This succeeds without an Oodle SDK even if `OODLE_SDK_DIR` is unset. If a compressed OBJ is passed
to this binary it exits with a clear unsupported-input error.

To opt into compressed OBJ support, point at an external SDK and pass the explicit build option:

```bat
set OODLE_SDK_DIR=C:\path\to\OodleDataCompression\Sdk
build.bat radlink release oodle
```

The standalone reference writer also requires the external SDK:

```bat
build.bat rad_obj_compress release
```

From an x64 Visual Studio developer command prompt, `scripts\build_cobj_test.bat` builds both
Oodle-enabled tools and copies them to `out_cobj`. `scripts\run_cobj_smoke.bat` compresses a small
OBJ, links both versions, and byte-compares the resulting executables.

## Selected format

The measured writer settings are:

| Setting | Value |
|---|---:|
| Codec | Kraken |
| Compression level | Normal |
| Independent segment size | 512 KiB |
| Space/speed tradeoff | 256 |
| Incompressible segments | Stored raw |

Each file contains:

1. a fixed header describing the original COFF size and segment geometry;
2. a segment directory with stored offsets, stored sizes, raw sizes, and raw/compressed flags;
3. compact indexes for type leaves, complete UDT hashes, base relocations, and `.debug$S` data;
4. independently decodable segment payloads.

The declarations are in `src\linker\lnk_compressed_obj_format.h`. The complete writer-facing byte
layout and sidecar contract are documented in `docs\compressed_obj_format.md`. All integers are
little-endian, and all file offsets and sidecar ranges are validated before the logical OBJ view is
published.

The 512 KiB segment size was the measured knee for both targets. Smaller segments reduced sparse
decode amplification but added too many decoder calls; larger segments saved little disk space and
decoded too many unused bytes.

## Reader design

The reader preserves RAD Link's pointer-based COFF parsers while bounding private commit:

1. Input classification checks one magic value. Ordinary OBJs stay on the existing mmap path.
2. `VirtualAlloc2` reserves a logical range matching the original OBJ using placeholders.
3. Raw-stored segments map directly from the portable file at their logical offsets.
4. A read fault on a compressed segment decodes into a bounded pagefile-backed slot and maps that
   slot into the placeholder. A pinned write-copy path preserves rare in-place COFF patches.
5. Type and `.debug$S` sidecars let dominant debug passes stream compressed ranges directly into
   their final destinations instead of repeatedly faulting the generic cache.
6. At the PDB boundary, the first cache generation is frozen and later misses use a smaller second
   generation. This keeps published pointers stable and eliminates measured generic redecodes.

The best measured UEFN configuration used a 17 GiB first generation and a 6 GiB second generation.
The equivalent explicit RAD Link options are:

```bat
/RAD_COBJ_CACHE_GIB:17
/RAD_COBJ_CACHE_SHRINK_GIB:6
/RAD_COBJ_CACHE_FREEZE
/RAD_COBJ_TRIM_WS
/RAD_COBJ_ONE_SHOT
```

FortniteClient needed 27,727 first-generation segments (13.54 GiB) and ran with a 14+1 GiB
configuration. Cache capacity is not part of the file format. When these options are absent, RAD
Link derives a bounded capacity from physical memory and available commit, then caps it to the
logical size of the compressed inputs. Environment variables remain available as development
overrides; precedence is command line, environment, then the adaptive policy.

`/RAD_COBJ_ONE_SHOT` is only appropriate for `radlink.exe` because the process is about to exit.
Leave it unset in embedded or long-lived hosts. `/RAD_LOG:TIMERS` prints the final cache, decode,
eviction, and timing statistics when diagnostics are needed; ordinary links remain silent.

## Create and link a corpus

Convert the direct OBJ entries in a response file while leaving switches and `.lib` paths intact:

```powershell
& .\scripts\cobj_compress_rsp.ps1 `
  -ResponseFile C:\path\target.rsp `
  -OutputDirectory D:\scratch\compressed-objs `
  -Compressor .\out_cobj\rad_obj_compress.exe `
  -Workers 16 `
  -SegmentKiB 512 `
  -Codec kraken `
  -SpaceSpeedTradeoff 256 `
  -CompressionLevel normal
```

The output directory receives one independently portable file per OBJ and a rewritten
`compressed.rsp`. Use a fresh directory when changing the format or writer settings.

Measure a link with process wall time and peak memory:

```powershell
& .\scripts\bench_cobj.ps1 `
  -Rsp D:\scratch\compressed-objs\compressed.rsp `
  -Output D:\scratch\link\target.exe `
  -WorkingDirectory C:\path\to\link\working-directory `
  -CacheGiB 17 `
  -CacheShrinkGiB 6 `
  -Tag compressed_warm
```

`scripts\stage_raw_rsp.ps1`, `scripts\warm_rsp.ps1`, and `scripts\retarget_rsp.ps1` support fair
raw/compressed comparisons. `scripts\test_link_determinism.ps1` checks both image and PDB hashes
across repeated links.

## Writer and integrity invariants

A production writer must preserve these rules:

1. Each output is self-contained and has ordinary non-sparse file length.
2. Segment size is a power of two and every segment is independently decodable.
3. Directory and sidecar offsets are aligned, ordered where required, and bounds-checked.
4. Type offsets are relative to the first byte after the CodeView signature.
5. The `.debug$S` index follows the reader's accepted-subsection rules exactly.
6. Incompressible segments may be stored raw without changing logical segment geometry.
7. Write a temporary file, validate every compressed round trip, and atomically rename it.

The reference writer decodes and byte-compares every compressed segment before publishing. The
container does not currently enable Oodle quantum CRCs, and raw-stored segments bypass the decoder.
Production caches should retain their normal whole-file content hash verification when uploading
and downloading these objects; that covers metadata, compressed payloads, and raw payloads without
adding work to the linker's hot path.

## Current limitations

- The lazy placeholder reader is Windows-only.
- Oodle must be supplied and licensed separately; it is not part of this repository.
- Direct compressed OBJ inputs are supported. Compressed members inside `.lib` archives are not.
- The cache is process-global and tuned for the one-link command-line executable.
