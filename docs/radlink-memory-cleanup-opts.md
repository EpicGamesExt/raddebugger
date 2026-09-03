# Compressed-input memory and cleanup optimizations

This is a follow-up to [#892](https://github.com/EpicGames/raddebugger/pull/892),
which depends on [#842](https://github.com/EpicGames/raddebugger/pull/842).
Review the three implementation commits independently; no compressed-object
format, default cache capacity, or PDB output/drain semantics change here.

| Commit | Change | Review focus |
|---|---|---|
| `afaea94e` | Object-scoped debug-relocation decode window | Lifetime, fallback, stable relocation ordering |
| `5d8676d7` | Independent input-unmap worker limit | Default compatibility and joined last-reader boundary |
| `87123aa6` | Sharded decoded-cache backing | Slot-to-shard offsets, write-group alignment, generation ownership |

## 1. Decode debug relocations without populating the shared cache

The module-writing path reads immutable relocation records only to copy them
into its existing sort array. Reading through the compressed logical view can
materialize shared-cache segments unnecessarily.

Reuse one relocation decode window for the whole object, separate from the
section-data window, and copy at most 4,096 records per batch. Adjacent small
tables can share a decoded segment without repeatedly decoding it. The window
is released when that object's debug-section processing finishes.

Raw objects and image-time relocation application retain the logical-view
path. Failed explicit copies also fall back to the logical view. Original
relocation indices remain the sort tie-breaker. The existing complete sort
array is unchanged; the added scratch is a bounded window and batch, not
another full relocation-table copy.

This is not a claim of lower total process commit. Earlier Engine observations
showed lower working set but up to about 2.9 GiB higher process commit. Faster
PDB-data production getting ahead of the writer is one possible explanation,
not an established cause.

## 2. Tune cleanup independently of debug parsing

`/RAD_UNMAP_WORKERS:#` limits both early and final input-map release. It does not
change their ordering or the joined last-reader boundary.

- Unspecified: inherit the final `/RAD_DEBUG_WORKERS` value, preserving existing behavior.
- `0`: uncapped by this option; the existing worker pool remains the limit.
- Positive value: cap the cleanup workers, independently of the parsing cap.

For compressed inputs, `/RAD_COBJ_ONE_SHOT:NO` is needed to exercise explicit
cleanup instead of skipping it at process exit. Four workers is a measurement
configuration, not a new default or a universal recommendation. Output-writer
draining is untouched, including when UBA supplies shared-memory output.

## 3. Split cache backing into independently reclaimed sections

One large pagefile section can leave its final unmapper doing most of the
section's page reclamation. Replace the single backing handle with an array of
sections targeting 256 MiB each. This size is provisional.

Shard boundaries contain complete 64-slot write groups. Large segment sizes
may therefore require a shard larger than 256 MiB. The last shard contains only
the remaining slots; total cache capacity is not rounded up. Both grouped and
fallback per-slot writable views use the same shard-local offsets as the
read-only logical views. Logical addresses, slot indices, and eviction order
are unchanged.

Generation replacement stays at the existing joined/quiescent boundary.
Closing old backing handles does not invalidate still-mapped frozen views;
those views retain ownership until unmapped. Partial allocation failure closes
the handles already allocated. Failed freeze setup releases its new backing
and retains the existing destructive-reset fallback.

This does not add a final active-cache release pass. More section handles and
kernel metadata are a tradeoff; neither a peak-memory reduction nor a full-link
shutdown improvement has yet been measured for this change.

## Evidence and limitations

These are earlier observations from a busy shared machine, not controlled
benchmarks of the combined PR head. Other work ran concurrently and inputs
were not held immutable across captures.

| Change | Observed benefit | Limitation |
|---|---|---|
| Relocation window | Engine working-set peaks: baseline 49.59-49.74 GiB; candidate 44.69-47.05 GiB | Process commit: 22.88-22.99 versus 23.07-25.80 GiB. No demonstrated end-to-end speedup. |
| Cleanup cap 20 to 4 | Cleanup CPU attribution: 26.17 to 8.23 CPU-s, about 69% less | Cleanup wall time increased from 4.34 to 5.11 s. Not a latency win. |
| Sharded backing | Motivated by a 2.45 s final worker interval reclaiming a pagefile section | Full Engine/Common, memory, allocation-failure stress, and UBA validation remain open. |

Measure wall time through actual process completion: Windows can continue
kernel teardown after the trace's Process/End event. Internal phase totals are
not end-to-end wall time. Track both working set and process commit, plus system
commit: the process counter does not account for all shared pagefile-backed
cache charge. Do not attribute other processes' system-commit changes to radlink.

## Correctness coverage

- `compressed_debug_reloc_parity`: overflow relocation counts, reversed ordering,
  many small debug sections, and exact raw/compressed EXE/PDB parity. Four pool
  workers remain available while cleanup caps 1, 4, and 0 are exercised.
- `compressed_cache_shard_boundary`: crosses the 256 MiB boundary, includes a
  partial final write group, and verifies raw/compressed EXE/PDB parity with
  generation freezing both enabled and disabled.
- The combined development implementation passed 161 supported tests in both
  release and debug, with zero failures/crashes and 12 explicitly excluded
  MSVC-invoking cases. Sixteen old/new artifact-parity links and sixteen
  concurrent shared-pool links also matched their controls and executed.

Large-input ICF nondeterminism and Linux/Wine/UBA reliability remain unresolved
as documented in the dependency PRs. The existing full GitHub matrix is not
green; local supported-subset checks do not replace it. These changes are
proposed for review, not presented as production-qualified improvements.
