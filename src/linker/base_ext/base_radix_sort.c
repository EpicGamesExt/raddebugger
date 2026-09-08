// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define U64_RADIX_BITS 8
#define U64_RADIX_SIZE (1 << U64_RADIX_BITS)
#define U64_RADIX_PARALLEL_MIN 140000

typedef struct U64RadixTask
{
  Rng1U64 *ranges;
  U64     *src;
  U64     *dst;
  U64     *offsets;
  U32      shift;
} U64RadixTask;

internal
THREAD_POOL_TASK_FUNC(u64_radix_hist_task)
{
  U64RadixTask *task   = raw_task;
  U64          *counts = task->offsets + task_id * U64_RADIX_SIZE;
  MemoryZeroTyped(counts, U64_RADIX_SIZE);
  for EachInRange(i, task->ranges[task_id]) {
    U64 digit = (task->src[i] >> task->shift) & (U64_RADIX_SIZE - 1);
    counts[digit] += 1;
  }
}

internal
THREAD_POOL_TASK_FUNC(u64_radix_scatter_task)
{
  U64RadixTask *task    = raw_task;
  U64          *offsets = task->offsets + task_id * U64_RADIX_SIZE;
  for EachInRange(i, task->ranges[task_id]) {
    U64 value = task->src[i];
    U64 digit = (value >> task->shift) & (U64_RADIX_SIZE - 1);
    task->dst[offsets[digit]++] = value;
  }
}

internal void
u64_array_sort_radix_parallel(TP_Context *tp, U64 count, U64 *v)
{
  if (count < U64_RADIX_PARALLEL_MIN) {
    u64_array_sort(count, v);
    return;
  }

  U64 max_value = u64_array_max(count, v);
  if (max_value == 0) { return; }

  Temp scratch = scratch_begin(0,0);

  U64          task_count = tp->worker_count;
  U64         *buffer     = push_array_no_zero(scratch.arena, U64, count);
  U64         *offsets    = push_array_no_zero(scratch.arena, U64, task_count * U64_RADIX_SIZE);
  U64RadixTask task       = {
    .ranges  = tp_divide_work(scratch.arena, count, task_count),
    .src     = v,
    .dst     = buffer,
    .offsets = offsets,
  };

  U64 pass_count = (64 - clz64(max_value) + U64_RADIX_BITS - 1) / U64_RADIX_BITS;
  for EachIndex(pass_idx, pass_count) {
    task.shift = safe_cast_u32(pass_idx * U64_RADIX_BITS);
    tp_for_parallel(tp, 0, task_count, u64_radix_hist_task, &task);

    U64 cursor = 0;
    U64 populated_digit_count = 0;
    for EachIndex(digit, U64_RADIX_SIZE) {
      U64 digit_begin = cursor;
      for EachIndex(task_idx, task_count) {
        U64 *slot       = &offsets[task_idx * U64_RADIX_SIZE + digit];
        U64  slot_count = *slot;
        *slot = cursor;
        cursor += slot_count;
      }
      populated_digit_count += (cursor != digit_begin);
    }
    Assert(cursor == count);

    // A stable radix pass with a single populated digit is the identity transform.
    if (populated_digit_count == 1) { continue; }

    tp_for_parallel(tp, 0, task_count, u64_radix_scatter_task, &task);
    Swap(U64 *, task.src, task.dst);
  }

  if (task.src != v) {
    MemoryCopyTyped(v, task.src, count);
  }

  scratch_end(scratch);
}

#undef U64_RADIX_BITS
#undef U64_RADIX_SIZE
#undef U64_RADIX_PARALLEL_MIN
