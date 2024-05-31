// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_DUMP_H
#define RADDBGI_DUMP_H

////////////////////////////////
//~ rjf: RADDBG Stringize Helper Types

typedef struct RDI_FilePathBundle RDI_FilePathBundle;
struct RDI_FilePathBundle
{
  RDI_FilePathNode *file_paths;
  U32 file_path_count;
};

typedef struct RDI_UDTMemberBundle RDI_UDTMemberBundle;
struct RDI_UDTMemberBundle
{
  RDI_Member *members;
  RDI_EnumMember *enum_members;
  U32 member_count;
  U32 enum_member_count;
};

typedef struct RDI_ScopeBundle RDI_ScopeBundle;
struct RDI_ScopeBundle
{
  RDI_Scope *scopes;
  U64 *scope_voffs;
  RDI_Local *locals;
  RDI_LocationBlock *location_blocks;
  U8 *location_data;
  U32 scope_count;
  U32 scope_voff_count;
  U32 local_count;
  U32 location_block_count;
  U32 location_data_size;
};

////////////////////////////////
//~ rjf: RADDBGI Enum -> String Functions

internal String8 rdi_string_from_data_section_tag(RDI_DataSectionTag tag);
internal String8 rdi_string_from_arch(RDI_Arch arch);
internal String8 rdi_string_from_language(RDI_Language language);
internal String8 rdi_string_from_type_kind(RDI_TypeKind type_kind);
internal String8 rdi_string_from_member_kind(RDI_MemberKind member_kind);
internal String8 rdi_string_from_local_kind(RDI_LocalKind local_kind);

////////////////////////////////
//~ rjf: RADDBGI Flags -> String Functions

internal void rdi_stringize_type_modifier_flags(Arena *arena, String8List *out, RDI_TypeModifierFlags flags);
internal void rdi_stringize_user_defined_type_flags(Arena *arena, String8List *out, RDI_UserDefinedTypeFlags flags);
internal void rdi_stringize_link_flags(Arena *arena, String8List *out, RDI_LinkFlags flags);

////////////////////////////////
//~ rjf: RADDBG Compound Stringize Functions

internal void rdi_stringize_data_sections(Arena *arena, String8List *out, RDI_Parsed *parsed, U32 indent_level);
internal void rdi_stringize_top_level_info(Arena *arena, String8List *out, RDI_Parsed *parsed, RDI_TopLevelInfo *tli, U32 indent_level);
internal void rdi_stringize_binary_sections(Arena *arena, String8List *out, RDI_Parsed *rdi, U32 indent_level);
internal void rdi_stringize_file_path(Arena *arena, String8List *out, RDI_Parsed *parsed, RDI_FilePathBundle *bundle, RDI_FilePathNode *file_path, U32 indent_level);
internal void rdi_stringize_source_file(Arena *arena, String8List *out, RDI_Parsed *parsed, RDI_SourceFile *source_file, U32 indent_level);
internal void rdi_stringize_unit(Arena *arena, String8List *out, RDI_Parsed *parsed, RDI_Unit *unit, U32 indent_level);
internal void rdi_stringize_type_node(Arena *arena, String8List *out, RDI_Parsed *parsed, RDI_TypeNode *type, U32 indent_level);
internal void rdi_stringize_udt(Arena *arena, String8List *out, RDI_Parsed *parsed, RDI_UDTMemberBundle *bundle, RDI_UDT *udt, U32 indent_level);
internal void rdi_stringize_global_variable(Arena *arena, String8List *out, RDI_Parsed *parsed, RDI_GlobalVariable *global_variable, U32 indent_level);
internal void rdi_stringize_thread_variable(Arena *arena, String8List *out, RDI_Parsed *parsed, RDI_ThreadVariable *thread_var, U32 indent_level);
internal void rdi_stringize_procedure(Arena *arena, String8List *out, RDI_Parsed *parsed, RDI_Procedure *proc, U32 indent_level);
internal void rdi_stringize_scope(Arena *arena, String8List *out, RDI_Parsed *parsed, RDI_ScopeBundle *bundle, RDI_Scope *scope, U32 indent_level);
internal void rdi_stringize_checksums(Arena *arena, String8List *out, RDI_Parsed *rdi, U32 indent_level);

#endif // RADDBGI_DUMP_H
