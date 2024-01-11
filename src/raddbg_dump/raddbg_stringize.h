// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_STRINGIZE_H
#define RADDBG_STRINGIZE_H

// TODO(allen): this depends on types from our base layer.
// we need to decide if we want this to be included in the "format" layer
// and therefore lifted off of the base layer, or if we want to put it in
// "base" or "dump" layers or something like that so that it can
// rely on Arena, String8, and String8List from the "base" layer.

////////////////////////////////
//~ RADDBG Stringize Helper Types

typedef struct RADDBG_FilePathBundle{
  RADDBG_FilePathNode *file_paths;
  U32 file_path_count;
} RADDBG_FilePathBundle;

typedef struct RADDBG_UDTMemberBundle{
  RADDBG_Member *members;
  RADDBG_EnumMember *enum_members;
  U32 member_count;
  U32 enum_member_count;
} RADDBG_UDTMemberBundle;

typedef struct RADDBG_ScopeBundle{
  RADDBG_Scope *scopes;
  U64 *scope_voffs;
  RADDBG_Local *locals;
  RADDBG_LocationBlock *location_blocks;
  U8 *location_data;
  U32 scope_count;
  U32 scope_voff_count;
  U32 local_count;
  U32 location_block_count;
  U32 location_data_size;
} RADDBG_ScopeBundle;

////////////////////////////////
//~ RADDBG Common Stringize Functions

static String8 raddbg_string_from_data_section_tag(RADDBG_DataSectionTag tag);
static String8 raddbg_string_from_arch(RADDBG_Arch arch);
static String8 raddbg_string_from_language(RADDBG_Language language);
static String8 raddbg_string_from_type_kind(RADDBG_TypeKind type_kind);
static String8 raddbg_string_from_member_kind(RADDBG_MemberKind member_kind);
static String8 raddbg_string_from_local_kind(RADDBG_LocalKind local_kind);

////////////////////////////////
//~ RADDBG Flags Stringize Functions

static void raddbg_stringize_binary_section_flags(Arena *arena, String8List *out,
                                                  RADDBG_BinarySectionFlags flags);

static void raddbg_stringize_type_modifier_flags(Arena *arena, String8List *out,
                                                 RADDBG_TypeModifierFlags flags);

static void raddbg_stringize_user_defined_type_flags(Arena *arena, String8List *out,
                                                     RADDBG_UserDefinedTypeFlags flags);

static void raddbg_stringize_link_flags(Arena *arena, String8List *out,
                                        RADDBG_LinkFlags flags);

////////////////////////////////
//~ RADDBG Compound Stringize Functions

static void
raddbg_stringize_data_sections(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                               U32 indent_level);

static void
raddbg_stringize_top_level_info(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                                RADDBG_TopLevelInfo *tli, U32 indent_level);

static void
raddbg_stringize_binary_section(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                                RADDBG_BinarySection *bin_section, U32 indent_level);

static void
raddbg_stringize_file_path(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                           RADDBG_FilePathBundle *bundle, RADDBG_FilePathNode *file_path,
                           U32 indent_level);

static void
raddbg_stringize_source_file(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                             RADDBG_SourceFile *source_file, U32 indent_level);

static void
raddbg_stringize_unit(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                      RADDBG_Unit *unit, U32 indent_level);

static void
raddbg_stringize_type_node(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                           RADDBG_TypeNode *type, U32 indent_level);

static void
raddbg_stringize_udt(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                     RADDBG_UDTMemberBundle *bundle, RADDBG_UDT *udt,
                     U32 indent_level);

static void
raddbg_stringize_global_variable(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                                 RADDBG_GlobalVariable *global_variable, U32 indent_level);

static void
raddbg_stringize_thread_variable(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                                 RADDBG_ThreadVariable *thread_var,
                                 U32 indent_level);

static void
raddbg_stringize_procedure(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                           RADDBG_Procedure *proc, U32 indent_level);

static void
raddbg_stringize_scope(Arena *arena, String8List *out, RADDBG_Parsed *parsed,
                       RADDBG_ScopeBundle *bundle, RADDBG_Scope *scope, U32 indent_level);

#endif //RADDBG_STRINGIZE_H
