// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_DUMP_H
#define DWARF_DUMP_H

typedef enum
{
  DW_DumperCacheFlag_InfoRanges = (1 << 0),
  DW_DumperCacheFlag_CuArr      = (1 << 1),
  DW_DumperCacheFlag_LineVmMap  = (1 << 2),
  DW_DumperCacheFlag_TagTrees   = (1 << 3),
} DW_DumperCacheFlags;

typedef struct DW_Dumper
{
  Arena               *arena;
  Arch                 arch;
  B32                  is_relaxed;
  B32                  is_verbose;
  DW_Raw            *input;
  DW_DumperCacheFlags  cache_flags;
  struct {
    Rng1U64Array         info_ranges;
    DW_CompUnit         *cu_arr;
    HashTable           *line_vm_map;
    DW_TagTree          *tag_trees;
  } cache;
} DW_Dumper;

#define DW_PRINTER(name) void name(Arena *arena, String8List *strings, int indent, DW_Dumper *dumper)
typedef DW_PRINTER(DW_Printer);

////////////////////////////////

// cache
internal Rng1U64Array  dw_get_info_ranges(DW_Dumper *dumper);
internal DW_CompUnit * dw_get_cu_arr(DW_Dumper *dumper);
internal HashTable *   dw_get_line_vm_map(DW_Dumper *dumper);
internal DW_TagTree *  dw_get_tag_trees(DW_Dumper *dumper);

////////////////////////////////
//~ rjf: Stringification Helpers

internal String8List dw_string_list_from_cfi_program(Arena *arena, U64 cu_base, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format, U64 pc_begin, DW_CIE *cie, DW_DecodePtr *decode_ptr_func, void *deocde_ptr_ud, String8 program);
internal String8List dw_string_list_from_expression (Arena *arena, String8 raw_data, U64 cu_base, U64 addr_size, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format);

internal String8 dw_string_from_reg_off     (Arena *arena, Arch arch, DW_Reg reg_idx, S64 reg_off);
internal String8 dw_string_from_attrib_value(Arena *arena, DW_Raw *input, Arch arch, DW_CompUnit *cu, DW_LineVM *line_vm, DW_Attrib *attrib);
internal String8 dw_string_from_expression  (Arena *arena, String8 expr, U64 cu_base, U64 addr_size, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format);
internal String8 dw_string_from_cfa         (Arena *arena, Arch arch, U64 address_size, DW_Version version, DW_Ext ext, DW_Format format, DW_CFA cfa);
internal String8 dw_string_from_cfi_row     (Arena *arena, Arch arch, U64 address_size, DW_Version version, DW_Ext ext, DW_Format format, DW_CFI_Row *row);

#if 0
internal void dw_print_eh_frame         (Arena *arena, String8List *out, String8 indent, String8 raw_eh_frame, Arch arch, DW_Version ver, DW_Ext ext, EH_PtrCtx *ptr_ctx);
internal void dw_print_debug_loc        (Arena *arena, String8List *out, String8 indent, DW_Raw *input, Arch arch, ExecutableImageKind image_type, B32 relaxed);
internal void dw_print_debug_ranges     (Arena *arena, String8List *out, String8 indent, DW_Raw *input, Arch arch, ExecutableImageKind image_type, B32 relaxed);
internal void dw_print_debug_aranges    (Arena *arena, String8List *out, String8 indent, DW_Raw *input);
internal void dw_print_debug_addr       (Arena *arena, String8List *out, String8 indent, DW_Raw *input);
internal void dw_print_debug_loclists   (Arena *arena, String8List *out, String8 indent, DW_Raw *input, Rng1U64Array segment_vranges, Arch arch);
internal void dw_print_debug_rnglists   (Arena *arena, String8List *out, String8 indent, DW_Raw *input, Rng1U64Array segment_vranges);
internal void dw_print_debug_pubnames   (Arena *arena, String8List *out, String8 indent, DW_Raw *input);
internal void dw_print_debug_pubtypes   (Arena *arena, String8List *out, String8 indent, DW_Raw *input);
internal void dw_print_debug_line_str   (Arena *arena, String8List *out, String8 indent, DW_Raw *input);
internal void dw_print_debug_str_offsets(Arena *arena, String8List *out, String8 indent, DW_Raw *input);
#endif

////////////////////////////////
//~ rjf: Dump Entry Point

internal String8List dw_dump_list_from_sections(Arena *arena, DW_Raw *input, Arch arch, DW_SectionFlags dump_sections, B32 is_verbose);

#endif // DWARF_DUMP_H
