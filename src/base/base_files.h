// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_FILES_H
#define BASE_FILES_H

typedef U32 FileIterFlags;
enum
{
  FileIterFlag_SkipFolders     = (1 << 0),
  FileIterFlag_SkipFiles       = (1 << 1),
  FileIterFlag_SkipHiddenFiles = (1 << 2),
  FileIterFlag_Done            = (1 << 31),
};

typedef struct FileIter FileIter;
struct FileIter
{
  FileIterFlags flags;
  U8 memory[800];
};

typedef struct FileInfo FileInfo;
struct FileInfo
{
  String8 name;
  FileProperties props;
};

// nick: on-disk file identifier
typedef struct FileID FileID;
struct FileID
{
  U64 v[3];
};

typedef struct File File;
struct File
{
  U64 u64[1];
};

typedef struct FileMap FileMap;
struct FileMap
{
  U64 u64[1];
};

////////////////////////////////
//~ rjf: Handle Type Functions

internal File file_zero(void);
internal B32 file_match(File a, File b);

////////////////////////////////
//~ rjf: Filesystem Helpers (Helpers, Implemented Once)

internal String8 data_from_file_path(Arena *arena, String8 path);
internal B32     write_data_to_file_path(String8 path, String8 data);
internal B32     write_data_list_to_file_path(String8 path, String8List list);
internal B32     append_data_to_file_path(String8 path, String8 data);
internal FileID  id_from_file_path(String8 path);
internal S64     file_id_compare(FileID a, FileID b);
internal String8 string_from_file_range(Arena *arena, File file, Rng1U64 range);
internal String8 file_read_cstring(Arena *arena, File file, U64 off);

////////////////////////////////
//~ rjf: @per_os_impl File System (Implemented Per-OS)

//- rjf: files
internal File           file_open(AccessFlags flags, String8 path);
internal void           file_close(File file);
internal U64            file_read(File file, Rng1U64 rng, void *out_data);
#define file_read_struct(f, off, ptr) file_read((f), r1u64((off), (off)+sizeof(*(ptr))), (ptr))
internal U64            file_write(File file, Rng1U64 rng, void *data);
internal B32            file_set_times(File file, DateTime time);
internal FileProperties properties_from_file(File file);
internal FileID         id_from_file(File file);
internal B32            file_reserve_size(File file, U64 size);
internal B32            delete_file_at_path(String8 path);
internal B32            copy_file_path(String8 dst, String8 src);
internal B32            move_file_path(String8 dst, String8 src);
internal String8        full_path_from_path(Arena *arena, String8 path);
internal B32            file_path_exists(String8 path);
internal B32            folder_path_exists(String8 path);
internal FileProperties properties_from_file_path(String8 path);

//- rjf: file maps
internal FileMap file_map_open(AccessFlags flags, File file);
internal void    file_map_close(FileMap map);
internal void *  file_map_view_open(FileMap map, AccessFlags flags, Rng1U64 range);
internal void    file_map_view_close(FileMap map, void *ptr, Rng1U64 range);

//- rjf: directory iteration
internal FileIter *file_iter_begin(Arena *arena, String8 path, FileIterFlags flags);
internal B32 file_iter_next(Arena *arena, FileIter *iter, FileInfo *info_out);
internal void file_iter_end(FileIter *iter);

//- rjf: directory creation
internal B32 make_directory(String8 path);

#endif // BASE_FILES_H
