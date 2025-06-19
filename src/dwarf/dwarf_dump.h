// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_DUMP_H
#define DWARF_DUMP_H

internal String8List dw_string_list_from_expression  (Arena *arena, String8 raw_data, U64 cu_base, U64 address_size, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format);
internal String8     dw_format_expression_single_line(Arena *arena, String8 raw_data, U64 cu_base, U64 address_size, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format);
internal String8     dw_format_eh_ptr_enc            (Arena *arena, DW_EhPtrEnc enc);
internal void        dw_print_cfi_program            (Arena *arena, String8List *out, String8 indent, String8 raw_data, DW_CIEUnpacked *cie, DW_EhPtrCtx *ptr_ctx, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format);

internal void dw_print_eh_frame         (Arena *arena, String8List *out, String8 indent, String8 raw_eh_frame, Arch arch, DW_Version ver, DW_Ext ext, DW_EhPtrCtx *ptr_ctx);
internal void dw_print_debug_info       (Arena *arena, String8List *out, String8 indent, DW_Input *input, DW_ListUnitInput lu_input, Arch arch, B32 relaxed);
internal void dw_print_debug_abbrev     (Arena *arena, String8List *out, String8 indent, DW_Input *input);
internal void dw_print_debug_line       (Arena *arena, String8List *out, String8 indent, DW_Input *input, DW_ListUnitInput lu_input, B32 relaxed);
internal void dw_print_debug_str        (Arena *arena, String8List *out, String8 indent, DW_Input *input);
internal void dw_print_debug_loc        (Arena *arena, String8List *out, String8 indent, DW_Input *input, Arch arch, ExecutableImageKind image_type, B32 relaxed);
internal void dw_print_debug_ranges     (Arena *arena, String8List *out, String8 indent, DW_Input *input, Arch arch, ExecutableImageKind image_type, B32 relaxed);
internal void dw_print_debug_aranges    (Arena *arena, String8List *out, String8 indent, DW_Input *input);
internal void dw_print_debug_addr       (Arena *arena, String8List *out, String8 indent, DW_Input *input);
internal void dw_print_debug_loclists   (Arena *arena, String8List *out, String8 indent, DW_Input *input, Rng1U64Array segment_vranges, Arch arch);
internal void dw_print_debug_rnglists   (Arena *arena, String8List *out, String8 indent, DW_Input *input, Rng1U64Array segment_vranges);
internal void dw_print_debug_pubnames   (Arena *arena, String8List *out, String8 indent, DW_Input *input);
internal void dw_print_debug_pubtypes   (Arena *arena, String8List *out, String8 indent, DW_Input *input);
internal void dw_print_debug_line_str   (Arena *arena, String8List *out, String8 indent, DW_Input *input);
internal void dw_print_debug_str_offsets(Arena *arena, String8List *out, String8 indent, DW_Input *input);

#endif // DWARF_DUMP_H
