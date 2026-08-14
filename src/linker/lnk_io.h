#pragma once

typedef U32 LNK_IO_Flags;
enum
{
  LNK_IO_Flags_MemoryMapFilesReadOnly  = (1 << 0),
  LNK_IO_Flags_MemoryMapFilesReadWrite = (1 << 1),
};

typedef struct
{
  LNK_IO_Flags io_flags;
  String8Array path_arr;
  String8Array data_arr;
  File        *handle_arr;
  U64         *size_arr;
  U64         *off_arr;
  U8          *buffer;
  B8          *was_read;
} LNK_DiskReader;

typedef struct
{
  String8List data;
} LNK_FileArtifact;

typedef struct LNK_BackgroundFile LNK_BackgroundFile;

typedef struct
{
  Arena              *queue_arena;
  GuardedRing        *queue;
  GuardedRing        *decommit_queue;
  Thread              thread;
  Thread              decommit_thread;
  LNK_BackgroundFile *file_first;
  LNK_BackgroundFile *file_last;
  B32                 is_running;
  B32                 is_decommit_running;
  U64                 begin_time_us;   // Timers telemetry anchor
  U64                 bytes_enqueued;  // atomic; producer side
  U64                 bytes_completed; // atomic; writer thread
  U64                 decommit_bytes_enqueued;  // atomic; writer thread
  U64                 decommit_bytes_completed; // atomic; decommit thread
  U64                 jobs_enqueued;   // atomic; producer side
  U64                 jobs_completed;  // writer thread
  U64                 writes_issued;   // writer thread; post-coalesce WriteFile count
} LNK_BackgroundFileWriter;

// --- Shared File API ---------------------------------------------------------

shared_function int      lnk_open_file_read(char *path, uint64_t path_size, void *handle_buffer, uint64_t handle_buffer_max);
shared_function int      lnk_open_file_write(char *path, uint64_t path_size, void *handle_buffer, uint64_t handle_buffer_max);
shared_function void     lnk_close_file(void *raw_handle);
shared_function uint64_t lnk_size_from_file(void *raw_handle);
shared_function uint64_t lnk_read_file(void *raw_handle, void *buffer, uint64_t buffer_max);
shared_function uint64_t lnk_write_file(void *raw_handle, uint64_t offset, void *buffer, uint64_t buffer_size);

// --- IO Functions ------------------------------------------------------------

internal File      lnk_file_open_with_rename_permissions(String8 path);
internal B32       lnk_file_set_delete_on_close(File handle, B32 delete_file);
internal B32       lnk_file_rename(File handle, String8 new_name);

internal String8      lnk_read_data_from_file_path(Arena *arena, LNK_IO_Flags io_flags, String8 path, B8 *was_read_out);
internal String8Array lnk_read_data_from_file_path_parallel(TP_Context *tp, Arena *arena, LNK_IO_Flags io_flags, String8Array path_arr, B8 *was_read);

internal void lnk_write_data_list_to_file_path(String8 path, String8 temp_path, String8List list);
internal void lnk_write_data_to_file_path(String8 path, String8 temp_path, String8 data);
internal String8 lnk_data_from_file_artifact(Arena *arena, LNK_FileArtifact *artifact);

// --- Background Writer -------------------------------------------------------

internal void lnk_background_file_writer_begin      (LNK_BackgroundFileWriter *writer);
internal LNK_BackgroundFile *lnk_background_file_writer_begin_file(LNK_BackgroundFileWriter *writer, String8 path, String8 temp_path);
internal void lnk_background_file_writer_enqueue    (LNK_BackgroundFileWriter *writer, LNK_BackgroundFile *file, U64 file_off, String8 data, B32 decommit_after_write);
internal void lnk_background_file_writer_end_file   (LNK_BackgroundFileWriter *writer, LNK_BackgroundFile *file, U64 expected_byte_count);
internal void lnk_background_file_writer_end        (LNK_BackgroundFileWriter *writer);
