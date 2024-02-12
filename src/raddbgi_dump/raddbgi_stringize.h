// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_STRINGIZE_H
#define RADDBGI_STRINGIZE_H

// TODO(allen): this depends on types from our base layer.
// we need to decide if we want this to be included in the "format" layer
// and therefore lifted off of the base layer, or if we want to put it in
// "base" or "dump" layers or something like that so that it can
// rely on Arena, String8, and String8List from the "base" layer.

////////////////////////////////
//~ RADDBG Stringize Helper Types

typedef struct RADDBGI_FilePathBundle{
  RADDBGI_FilePathNode *file_paths;
  U32 file_path_count;
} RADDBGI_FilePathBundle;

typedef struct RADDBGI_UDTMemberBundle{
  RADDBGI_Member *members;
  RADDBGI_EnumMember *enum_members;
  U32 member_count;
  U32 enum_member_count;
} RADDBGI_UDTMemberBundle;

typedef struct RADDBGI_ScopeBundle{
  RADDBGI_Scope *scopes;
  U64 *scope_voffs;
  RADDBGI_Local *locals;
  RADDBGI_LocationBlock *location_blocks;
  U8 *location_data;
  U32 scope_count;
  U32 scope_voff_count;
  U32 local_count;
  U32 location_block_count;
  U32 location_data_size;
} RADDBGI_ScopeBundle;

////////////////////////////////
//~ RADDBG Common Stringize Functions

static String8 raddbgi_string_from_data_section_tag(RADDBGI_DataSectionTag tag);
static String8 raddbgi_string_from_arch(RADDBGI_Arch arch);
static String8 raddbgi_string_from_language(RADDBGI_Language language);
static String8 raddbgi_string_from_type_kind(RADDBGI_TypeKind type_kind);
static String8 raddbgi_string_from_member_kind(RADDBGI_MemberKind member_kind);
static String8 raddbgi_string_from_local_kind(RADDBGI_LocalKind local_kind);

////////////////////////////////
//~ RADDBG Flags Stringize Functions

static void raddbgi_stringize_binary_section_flags(Arena *arena, String8List *out,
                                                   RADDBGI_BinarySectionFlags flags);

static void raddbgi_stringize_type_modifier_flags(Arena *arena, String8List *out,
                                                  RADDBGI_TypeModifierFlags flags);

static void raddbgi_stringize_user_defined_type_flags(Arena *arena, String8List *out,
                                                      RADDBGI_UserDefinedTypeFlags flags);

static void raddbgi_stringize_link_flags(Arena *arena, String8List *out,
                                         RADDBGI_LinkFlags flags);

////////////////////////////////
//~ RADDBG Compound Stringize Functions

static void
raddbgi_stringize_data_sections(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                                U32 indent_level);

static void
raddbgi_stringize_top_level_info(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                                 RADDBGI_TopLevelInfo *tli, U32 indent_level);

static void
raddbgi_stringize_binary_section(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                                 RADDBGI_BinarySection *bin_section, U32 indent_level);

static void
raddbgi_stringize_file_path(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                            RADDBGI_FilePathBundle *bundle, RADDBGI_FilePathNode *file_path,
                            U32 indent_level);

static void
raddbgi_stringize_source_file(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                              RADDBGI_SourceFile *source_file, U32 indent_level);

static void
raddbgi_stringize_unit(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                       RADDBGI_Unit *unit, U32 indent_level);

static void
raddbgi_stringize_type_node(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                            RADDBGI_TypeNode *type, U32 indent_level);

static void
raddbgi_stringize_udt(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                      RADDBGI_UDTMemberBundle *bundle, RADDBGI_UDT *udt,
                      U32 indent_level);

static void
raddbgi_stringize_global_variable(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                                  RADDBGI_GlobalVariable *global_variable, U32 indent_level);

static void
raddbgi_stringize_thread_variable(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                                  RADDBGI_ThreadVariable *thread_var,
                                  U32 indent_level);

static void
raddbgi_stringize_procedure(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                            RADDBGI_Procedure *proc, U32 indent_level);

static void
raddbgi_stringize_scope(Arena *arena, String8List *out, RADDBGI_Parsed *parsed,
                        RADDBGI_ScopeBundle *bundle, RADDBGI_Scope *scope, U32 indent_level);

#endif //RADDBGI_STRINGIZE_H
